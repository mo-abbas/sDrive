#include "ObjectExtractor.h"
#include "OffRoadClipper.h"
#include <iostream>
#include <math.h>

float ObjectExtractor::EuclidianDistance(Vec3f a, Vec3f b)
{
    float result = 0;
    for (int i = 0; i < 3; i++)
    {
        result += (a.val[i] - b.val[i]) * (a.val[i] - b.val[i]);
    }

    return sqrt(result);
}

Direction ObjectExtractor::GetRightDirection(Direction currentDirection)
{
    return Direction(((int)currentDirection + 1) % 4);
}

bool ObjectExtractor::CollidesWithTheRightCamera(Vec3f point, Direction currentDirection)
{
    // convert the point to the camera's local origin
    Direction rightDirection = GetRightDirection(currentDirection);
    point = pointCloud.ConvertPointToLocalOrigin(point, rightDirection);
    return pointCloud.PointDirectionInView(point) != FRONT;
}

vector<Vec3f> ObjectExtractor::BoxToVertices(Box box)
{
    Vec3f vertex;
    vector<Vec3f> points;
    float Y[2] = { box.minY, box.maxY };

    for (int i = 0; i < 2; i++)
    {

        vertex[1] = Y[i];

        vertex[0] = box.maxX;
        vertex[2] = box.maxZ;
        points.push_back(vertex);

        vertex[0] = box.maxX;
        vertex[2] = box.minZ;
        points.push_back(vertex);

        vertex[0] = box.minX;
        vertex[2] = box.minZ;
        points.push_back(vertex);

        vertex[0] = box.minX;
        vertex[2] = box.maxZ;
        points.push_back(vertex);
    }

    return points;
}

void ObjectExtractor::DrawLine(Mat& image, Vec3f a, Vec3f b, Direction direction)
{
    a = pointCloud.ConvertPointToLocalOrigin(a, direction);
    b = pointCloud.ConvertPointToLocalOrigin(b, direction);

    Direction aDirection = pointCloud.PointDirectionInView(a);
    Direction bDirection = pointCloud.PointDirectionInView(b);

    if (aDirection != FRONT && bDirection != FRONT)
    {
        return;
    }

    if (aDirection != FRONT)
    {
        a = pointCloud.IntersectionWithCameraBorders(a, b, aDirection);
    }
    else if (bDirection != FRONT)
    {
        b = pointCloud.IntersectionWithCameraBorders(a, b, bDirection);
    }
    
    Vec2i point1 = pointCloud.ProjectPointTo2D(a);
    Vec2i point2 = pointCloud.ProjectPointTo2D(b);

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

void ObjectExtractor::DrawCuboid(Mat& image, vector<Vec3f>& points, Direction direction)
{
    // draw the first square
    for (int i = 0; i < 4; i++)
    {
        DrawLine(image, points[i], points[(i + 1) % 4], direction);
    }

    // draw the second square
    for (int i = 0; i < 4; i++)
    {
        DrawLine(image, points[i + 4], points[(i + 1) % 4 + 4], direction);
    }

    // connect the two sqaures
    for (int i = 0; i < 4; i++)
    {
        DrawLine(image, points[i], points[i + 4], direction);
    }
}

void ObjectExtractor::SegmentImage()
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
                    unionSet.SetAsBackground(index);

                    continue;
                }

                if (j > 0)
                {
                    Vec3f left = pointCloud[view].at<Vec3f>(i, j - 1);
                    if (EuclidianDistance(point, left) < THRESHOLD)
                    {
                        int imageNumber = (int)currentDirection;
                        int index1 = imageNumber * width * height + i * width + j;
                        int index2 = imageNumber * width * height + i * width + j - 1;

                        unionSet.UnionSets(index1, index2);
                    }
                }

                if (i > 0)
                {
                    Vec3f top = pointCloud[view].at<Vec3f>(i - 1, j);

                    if (EuclidianDistance(point, top) < THRESHOLD)
                    {
                        int imageNumber = (int)currentDirection;
                        int index1 = imageNumber * width * height + i * width + j;
                        int index2 = imageNumber * width * height + (i - 1) * width + j;

                        unionSet.UnionSets(index1, index2);
                    }
                }

                if (fourViews && CollidesWithTheRightCamera(point, currentDirection))
                {
                    Direction rightDirection = GetRightDirection(currentDirection);
                    int rightImage = (int)rightDirection;
                    Vec2i rightCameraProjection = pointCloud.GetCameraProjection(point, rightDirection);

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
                        if (EuclidianDistance(point, rightCameraPoint) < THRESHOLD)
                        {
                            unionSet.UnionSets(index1, index2 + y * width + x);
                        }
                    }
                }
            }
        }
    }
}

void ObjectExtractor::GetBoxesFromPointCloud()
{
    map<int, Box> setToBoxMap;
    float averageRoadHeight = pointCloud.GetAverageRoadHeight();
    float heightThreshold = averageRoadHeight + 1.0f;

    for (int view = 0; view < pointCloud.size(); view++)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int setNumber = unionSet.FindSet(view, i, j);
                Vec3f point = pointCloud[view].at<Vec3f>(i, j);

                if (unionSet.GetCount(setNumber) >= OBJECT_SIZE_THRESHOLD && !unionSet.IsBackground(setNumber))
                {
                    setToBoxMap[setNumber].Update(point);
                }
            }
        }
    }

    vector<Box> result;
    result.reserve(setToBoxMap.size());

    auto j = setToBoxMap.begin();
    for (int i = 0; j != setToBoxMap.end(); ++j, ++i)
    {
        if (j->second.maxY >= heightThreshold)
        {
            j->second.Adjust();
            result.push_back(j->second);
        }
    }

    boxes = result;
}

void ObjectExtractor::DetectObjects()
{
    SegmentImage();
    GetBoxesFromPointCloud();
}

ObjectExtractor::ObjectExtractor(PointCloud& pointCloud) : pointCloud(pointCloud), unionSet(pointCloud.size(), pointCloud[0].cols, pointCloud[0].rows)
{
    this->width = pointCloud[0].cols;
    this->height = pointCloud[0].rows;

    this->DetectObjects();
}

vector<Vec2f> ObjectExtractor::GetCars()
{
    vector<Vec2f> result(boxes.size());
    for (int i = 0; i < boxes.size(); i++)
    {
        result[i] = boxes[i].Get2DCenter();
    }
    
    return result;
}

pair<Mat, Mat> ObjectExtractor::GetRoadBorders()
{
    return pointCloud.GetRoadBorders();
}

vector<Mat> ObjectExtractor::VisualizeBoxes()
{
    vector<Mat> images(pointCloud.disparityVector.size());
    for (int i = 0; i < pointCloud.disparityVector.size(); i++)
    {
        pointCloud.disparityVector[i].convertTo(images[i], CV_8U);
        cvtColor(images[i], images[i], COLOR_GRAY2BGR);
    }

    for (int i = 0; i < images.size(); i++)
    {
        for (int j = 0; j < boxes.size(); j++)
        {
            vector<Vec3f> points = BoxToVertices(boxes[j]);
            DrawCuboid(images[i], points, (Direction)i);
        }
    }

    return images;
}

vector<Mat> ObjectExtractor::VisualizeSegmentation()
{
    return unionSet.Visualize();
}