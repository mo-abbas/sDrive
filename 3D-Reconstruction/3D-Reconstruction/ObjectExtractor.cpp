#include "ObjectExtractor.h"
#include "OffRoadClipper.h"
#include <iostream>
#include <math.h>

#define M_PI           3.14159265358979323846  /* pi */

float ObjectExtractor::euclidianDistance(Vec3f a, Vec3f b)
{
    float result = 0;
    for (int i = 0; i < 3; i++)
    {
        result += (a.val[i] - b.val[i]) * (a.val[i] - b.val[i]);
    }

    return sqrt(result);
}

float ObjectExtractor::euclidianDistance(Vec2f a, Vec2f b)
{
    float result = 0;
    for (int i = 0; i < 2; i++)
    {
        result += (a.val[i] - b.val[i]) * (a.val[i] - b.val[i]);
    }

    return sqrt(result);
}

void ObjectExtractor::convertPointsToGlobalOrigin(vector<Mat>& points, Direction direction)
{
    // translate to the global origin
    points[0] += leftCameraLocation[0];
    points[2] += leftCameraLocation[2];

    Mat x = points[0];
    Mat z = points[2];

    switch (direction)
    {
    case FRONT:
        break;

    case BACK:
        // rotate twice
        points[0] *= -1;
        points[2] *= -1;
        break;

    case LEFT:
        // rotate clockwise to look to the front side
        swap(points[0], points[2]);
        points[0] *= -1;
        break;

    case RIGHT:
        // rotate anti-clockwise to look to the front side
        swap(points[0], points[2]);
        points[2] *= -1;
        break;
    }
}

Mat ObjectExtractor::convertDisparityToXYZ(Mat& disparity, Direction currentDirection)
{
    int height = disparity.rows;
    int width = disparity.cols;

    Mat X(height, width, CV_32F);
    Mat Y(height, width, CV_32F);
    Mat Z = width * baseline / (2 * disparity);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (disparity.at<float>(i, j) < DISPARITY_THRESHOLD)
            {
                // the leftCameraLocation will be removed when the co-ordinates are converted to the global origin
                X.at<float>(i, j) = -leftCameraLocation[0];
                Y.at<float>(i, j) = 0;
                Z.at<float>(i, j) = -leftCameraLocation[2];
            }
            else
            {
                X.at<float>(i, j) = (j * 1.f / width - 0.5f) * Z.at<float>(i, j) / focalLength;
                Y.at<float>(i, j) = (0.5f - i * 1.f / height) * Z.at<float>(i, j) / focalLength;
            }
        }
    }

    vector<Mat> coordinates;
    coordinates.push_back(X);
    coordinates.push_back(Y);
    coordinates.push_back(Z);

    convertPointsToGlobalOrigin(coordinates, currentDirection);

    Mat cloud;
    merge(coordinates, cloud);

    return cloud;
}

Direction ObjectExtractor::getRightDirection(Direction currentDirection)
{
    return Direction(((int)currentDirection + 1) % 4);
}

Vec2f ObjectExtractor::getCameraProjection(Vec3f point, Direction direction)
{
    Vec3f newPoint = convertPointToLocalOrigin(point, direction);
    return projectPointTo2D(newPoint);
}

Vec3f ObjectExtractor::convertPointToLocalOrigin(Vec3f point, Direction direction)
{
    Vec3f result;
    result[1] = point[1];

    switch (direction)
    {
    case FRONT:
        result[0] = point[0];
        result[2] = point[2];
        break;

    case BACK:
        // rotate twice
        result[0] = -point[0];
        result[2] = -point[2];
        break;

    case LEFT:
        // rotate anti-clockwise to look to the left side
        result[0] = point[2];
        result[2] = -point[0];
        break;

    case RIGHT:
        // rotate clockwise to look to the right side
        result[0] = -point[2];
        result[2] = point[0];
        break;
    }


    // translate to the local origin
    // The local origin is always behind the global origin on the left after the rotation
    result[0] -= leftCameraLocation[0];
    result[2] -= leftCameraLocation[2];

    return result;
}

Direction ObjectExtractor::pointDirectionInView(Vec3f point)
{
    float x = point[0];
    float z = point[2];

    float temp = tan((fovx / 2.0) * M_PI / 180);
    // check if the point is behind the right border z = m * x
    if (z - tan((fovx / 2.0) * M_PI / 180) * x < 0)
        return x >= 0 ? RIGHT : LEFT;

    // check if the point is behind the left border z = - m * x
    if (z + tan((fovx / 2.0) * M_PI / 180) * x < 0)
        return x >= 0 ? RIGHT : LEFT;

    return FRONT;
}

bool ObjectExtractor::collidesWithTheRightCamera(Vec3f point, Direction currentDirection)
{
    // convert the point to the camera's local origin
    Direction rightDirection = getRightDirection(currentDirection);
    point = convertPointToLocalOrigin(point, rightDirection);
    return pointDirectionInView(point) != FRONT;
}

Vec2i ObjectExtractor::projectPointTo2D(Vec3f point)
{
    int x = int((point[0] * focalLength / point[2] + 0.5f) * width);
    int y = int((0.5f - point[1] * focalLength / point[2]) * height);

    Vec2i result;
    result[0] = x;
    result[1] = y;

    return result;
}

void ObjectExtractor::segmentImage(vector<Mat>& pointCloud, UnionSet& unionSet)
{
    int height = pointCloud[0].rows;
    int width = pointCloud[0].cols;
    bool fourViews = pointCloud.size() == 4;

    for (int view = 0; view < pointCloud.size(); view++)
    {
        Direction currentDirection = (Direction)view;

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                Vec3f point = pointCloud[view].at<Vec3f>(i, j);


                // compares the point with (0, y, 0)
                if (point[0] <= FLT_EPSILON && point[0] >= -FLT_EPSILON && point[2] <= FLT_EPSILON && point[2] >= -FLT_EPSILON)
                {
                    int imageNumber = (int)currentDirection;
                    int index = imageNumber * width * height + i * width + j;
                    unionSet.setBackground(index);

                    continue;
                }

                if (j > 0)
                {
                    Vec3f left = pointCloud[view].at<Vec3f>(i, j - 1);
                    if (euclidianDistance(point, left) < THRESHOLD)
                    {
                        int imageNumber = (int)currentDirection;
                        int index1 = imageNumber * width * height + i * width + j;
                        int index2 = imageNumber * width * height + i * width + j - 1;

                        unionSet.unionSet(index1, index2);
                    }
                }

                if (i > 0)
                {
                    Vec3f top = pointCloud[view].at<Vec3f>(i - 1, j);

                    if (euclidianDistance(point, top) < THRESHOLD)
                    {
                        int imageNumber = (int)currentDirection;
                        int index1 = imageNumber * width * height + i * width + j;
                        int index2 = imageNumber * width * height + (i - 1) * width + j;

                        unionSet.unionSet(index1, index2);
                    }
                }

                if (fourViews && collidesWithTheRightCamera(point, currentDirection))
                {
                    Direction rightDirection = getRightDirection(currentDirection);
                    int rightImage = (int)rightDirection;
                    Vec2i rightCameraProjection = getCameraProjection(point, rightDirection);

                    // left, down, right, up, center
                    int dx[] = { -1, 0, 1, 0, 0 };
                    int dy[] = { 0, 1, 0, -1, 0 };

                    int imageNumber = (int)currentDirection;
                    int index1 = imageNumber * width * height + i * width + j;
                    int index2 = rightImage  *  width * height;

                    for (int k = 0; k < 5; k++)
                    {
                        int x = rightCameraProjection[0] + dx[k];
                        int y = rightCameraProjection[1] + dy[k];

                        if (x < 0 || y < 0 || x >= width || y >= height)
                        {
                            continue;
                        }

                        Vec3f rightCameraPoint = pointCloud[rightImage].at<Vec3f>(y, x);
                        if (euclidianDistance(point, rightCameraPoint) < THRESHOLD)
                        {
                            unionSet.unionSet(index1, index2 + y * width + x);
                        }
                    }
                }
            }
        }
    }
}

// The disparity vector has the format Front, Right, Back, Left
ObjectExtractor::ObjectExtractor(int width, int height, float fovx, float baseline, float focalLength, Vec3f leftCameraLocation)
{
    this->height = height;
    this->width = width;

    this->fovx = fovx;
    this->baseline = baseline;
    this->focalLength = focalLength;
    
    this->leftCameraLocation = leftCameraLocation;
}

vector<Mat> ObjectExtractor::getPointCloud(vector<Mat>& disparityVector)
{
    vector<Mat> pointCloud;
    for (int i = 0; i < disparityVector.size(); i++)
    {
        pointCloud.push_back(convertDisparityToXYZ(disparityVector[i], (Direction)i));
    }

    return pointCloud;
}



vector<Vec3f> ObjectExtractor::boxToVertices(Box box)
{
    Vec3f vertex;
    vector<Vec3f> points;
    float Y[2] = { box.minY[1], box.maxY[1] };

    for (int i = 0; i < 2; i++)
    {

        vertex[1] = Y[i];

        vertex[0] = box.maxX[0];
        vertex[2] = box.maxZ[2];
        points.push_back(vertex);

        vertex[0] = box.maxX[0];
        vertex[2] = box.minZ[2];
        points.push_back(vertex);

        vertex[0] = box.minX[0];
        vertex[2] = box.minZ[2];
        points.push_back(vertex);

        vertex[0] = box.minX[0];
        vertex[2] = box.maxZ[2];
        points.push_back(vertex);
    }

    return points;
}

Vec3f ObjectExtractor::intersectionWithCameraBorders(Vec3f point1, Vec3f point2, Direction direction)
{
    /*
    * This calculates the intersection of a line with a plane.
    * The line is represented by the parametric equations
    * x = x1 + a * t
    * y = y1 + b * t
    * z = z1 + c * t
    * Where a, b anc c are calculated using the difference between the two ends of the line
    * e.g. a = x2 - x1, b = y2 - y1, c = z2 - z1
    *
    * The plane equation is represented as A * x + B * y + C * z + D = 0
    * so to solve the equations we find that t = (A * x1 + B * y1 + C * z1 + D) / (A * a + B * b + C * c)
    */

    Vec3f result;

    float a = point2[0] - point1[0];
    float b = point2[1] - point1[1];
    float c = point2[2] - point1[2];

    float A;
    if (direction == LEFT)
        A = tan((fovx / 2.0) * M_PI / 180);
    else
        A = -tan((fovx / 2.0) * M_PI / 180);

    float B = 0;
    float C = 1;
    float D = 0;

    float t = -(A * point1[0] + B * point1[1] + C * point1[2] + D) / (A * a + B * b + C * c);
    result[0] = point1[0] + a * t;
    result[1] = point1[1] + b * t;
    result[2] = point1[2] + c * t;

    return result;
}

void ObjectExtractor::drawLine(Mat& image, Vec3f a, Vec3f b, Direction direction)
{
    a = convertPointToLocalOrigin(a, direction);
    b = convertPointToLocalOrigin(b, direction);

    Direction aDirection = pointDirectionInView(a);
    Direction bDirection = pointDirectionInView(b);

    if (aDirection != FRONT && bDirection != FRONT)
    {
        return;
    }

    if (aDirection != FRONT)
    {
        a = intersectionWithCameraBorders(a, b, aDirection);
    }
    else if (bDirection != FRONT)
    {
        b = intersectionWithCameraBorders(a, b, bDirection);
    }
    
    Vec2i point1 = projectPointTo2D(a);
    Vec2i point2 = projectPointTo2D(b);

    if (aDirection == FRONT)
    {
        circle(image, point1, 3, Scalar(0, 255, 0), -1);
    }

    if (bDirection == FRONT)
    {
        circle(image, point2, 3, Scalar(0, 255, 0), -1);
    }

    line(image, point1, point2, Scalar(0, 255, 0));
}

void ObjectExtractor::drawCuboid(Mat& image, vector<Vec3f>& points, Direction direction)
{
    // draw the first square
    for (int i = 0; i < 4; i++)
    {
        drawLine(image, points[i], points[(i + 1) % 4], direction);
    }

    // draw the second square
    for (int i = 0; i < 4; i++)
    {
        drawLine(image, points[i + 4], points[(i + 1) % 4 + 4], direction);
    }

    // connect the two sqaures
    for (int i = 0; i < 4; i++)
    {
        drawLine(image, points[i], points[i + 4], direction);
    }
}

void ObjectExtractor::visualizeBoxes(vector<Mat>& disparityVector, vector<Box> boxes)
{
    vector<Mat> images(disparityVector.size());
    for (int i = 0; i < disparityVector.size(); i++)
    {
        disparityVector[i].convertTo(images[i], CV_8U);
        cvtColor(images[i], images[i], COLOR_GRAY2BGR);
    }

    for (int i = 0; i < images.size(); i++)
    {
        for (int j = 0; j < boxes.size(); j++)
        {
            vector<Vec3f> points = boxToVertices(boxes[j]);
            drawCuboid(images[i], points, (Direction)i);
        }

        imshow("object detection " + to_string(i), images[i]);
        waitKey(0);
    }
}

vector<Box> ObjectExtractor::getBoxesFromPointCloud(vector<Mat>& pointCloud, UnionSet& unionSet, float heightThreshold)
{
    map<int, Box> setToBoxMap;

    for (int view = 0; view < pointCloud.size(); view++)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int setNumber = unionSet.findSet(view, i, j);
                Vec3f point = pointCloud[view].at<Vec3f>(i, j);

                if (unionSet.getCount(setNumber) >= OBJECT_SIZE_THRESHOLD && !unionSet.isBackground(setNumber))
                {
                    setToBoxMap[setNumber].update(point);
                }
            }
        }
    }

    vector<Box> result;
    result.reserve(setToBoxMap.size());

    auto j = setToBoxMap.begin();
    for (int i = 0; j != setToBoxMap.end(); ++j, ++i)
    {
        if (j->second.maxY[1] >= heightThreshold)
        {
            result.push_back(j->second);
        }
    }

    return result;
}

vector<Box> ObjectExtractor::getObjects(vector<Mat>& disparityVector, vector<Mat>& roadVector, bool visualize)
{
    vector<Mat> pointCloud = getPointCloud(disparityVector);
    OffRoadClipper clipper(roadVector, pointCloud);

    float averageRoadHeight = clipper.clip();
    float heightThreshold = averageRoadHeight + 1.0;

    UnionSet unionSet((int)pointCloud.size(), pointCloud[0].cols, pointCloud[0].rows);
    segmentImage(pointCloud, unionSet);

    vector<Box> boxes = getBoxesFromPointCloud(pointCloud, unionSet, heightThreshold);

    if (visualize)
    {
        unionSet.visualize();
        visualizeBoxes(disparityVector, boxes);
    }

    return boxes;
}