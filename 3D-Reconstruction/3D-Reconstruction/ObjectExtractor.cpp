#include "ObjectExtractor.h"

float ObjectExtractor::locationDifference(Vec3f a, Vec3f b)
{
    float result = 0;
    for (int i = 0; i < 3; i++)
    {
        result += (a.val[i] - b.val[i]) * (a.val[i] - b.val[i]);
    }

    return sqrt(result);
}

void ObjectExtractor::convertPointsToGlobalOrigin(vector<Mat>& points, Direction direction)
{
    // translate to the global origin
    points[0] -= baseline / 2;
    points[2] -= baseline / 2;

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

Mat ObjectExtractor::convertDisparityToXYZ(Mat disparity, Direction currentDirection)
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
            if (disparity.at<float>(i, j) < 10)
            {
                // the (baseline / 2) will be removed when the co-ordinates are converted to the global origin
                X.at<float>(i, j) = baseline / 2;
                Y.at<float>(i, j) = 0;
                Z.at<float>(i, j) = baseline / 2;
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
    result[0] += 0.5f;
    result[2] += 0.5f;

    return result;
}

bool ObjectExtractor::collidesWithTheRightCamera(Vec3f point, Direction currentDirection)
{
    // convert the point to the camera's local origin
    point = convertPointToLocalOrigin(point, currentDirection);

    float x = point[0];
    float z = point[2];

    // check if the point is to the left of the left border z = x + 1
    if (z - x - 1 > 0)
        return false;

    // check if the point is to the right of the right border z = 1 - x
    if (z + x - 1 < 0)
        return false;

    return true;
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
                    if (locationDifference(point, left) < THRESHOLD)
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

                    if (locationDifference(point, top) < THRESHOLD)
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
                        if (locationDifference(point, rightCameraPoint) < THRESHOLD)
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
ObjectExtractor::ObjectExtractor(int width, int height, float baseline, float focalLength)
{
    this->baseline = baseline;
    this->focalLength = focalLength;

    this->height = height;
    this->width = width;
}

vector<Mat> ObjectExtractor::getPointCloud(vector<Mat> disparityVector)
{
    vector<Mat> pointCloud;
    for (int i = 0; i < disparityVector.size(); i++)
    {
        pointCloud.push_back(convertDisparityToXYZ(disparityVector[i], (Direction)i));
    }

    return pointCloud;
}

vector<Vec2i> ObjectExtractor::boxTo2D(Box box, Direction direction)
{
    vector<Vec2i> points;
    float Y[2] = { box.minY[1], box.maxY[1] };

    Vec3f temp;

    for (int i = 0; i < 2; i++)
    {
        temp = box.maxX;
        temp[1] = Y[i];
        points.push_back(getCameraProjection(temp, direction));

        temp = box.maxZ;
        temp[1] = Y[i];
        points.push_back(getCameraProjection(temp, direction));

        temp = box.minX;
        temp[1] = Y[i];
        points.push_back(getCameraProjection(temp, direction));

        temp = box.minZ;
        temp[1] = Y[i];
        points.push_back(getCameraProjection(temp, direction));
    }

    return points;
}

Vec2i adjustPoint(Vec2i point, float slope, float c)
{
    //placeholder
    int width = 0;
    int height = 0;

    if (point[0] < 0 && point[1] < 0)
    {
        if (c >= 0 && c < height)
        {
            point[0] = 0;
            point[1] = c;
        }
        else
        {
            point[1] = 0;
            point[0] = -c / slope;
        }
    }
    else if (point[0] < 0 && point[1] >= height)
    {
        if (c >= 0 && c < height)
        {
            point[0] = 0;
            point[1] = c;
        }
        else
        {
            point[1] = height - 1;
            point[0] = (point[1] - c) / slope;
        }
    }
    else if (point[0] >= width && point[1] < 0)
    {
        float yInter = slope * width + c;
        if (yInter >= 0 && yInter < height)
        {
            point[0] = width - 1;
            point[1] = yInter;
        }
        else
        {
            point[1] = 0;
            point[0] = -c / slope;
        }
    }
    else if (point[0] >= width && point[1] >= height)
    {
        float yInter = slope * width + c;
        if (yInter >= 0 && yInter < height)
        {
            point[0] = width - 1;
            point[1] = yInter;
        }
        else
        {
            point[1] = height - 1;
            point[0] = (point[1] - c) / slope;
        }
    }
    else if (point[0] < 0)
    {
        point[0] = 0;
        point[1] = c;
    }
    else if (point[0] >= width)
    {
        point[]
    }
}


vector<Vec2i> adjustPoints(Vec2i first, Vec2i second)
{
    //placeholder
    int width = 0;
    int height = 0;

    vector<Vec2i> result;
    bool firstInBounds = first[0] >= 0 && first[1] >= 0 && first[0] < width && first[1] < height;
    bool secondInBounds = second[0] >= 0 && second[1] >= 0 && second[0] < width && second[1] < height;
    if (firstInBounds && secondInBounds)
    {
        result.push_back(first);
        result.push_back(second);
        return result;
    }

    float slope = (second[1] - first[1]) / (second[0] - first[0]);
    float c = second[1] - slope * second[0];

    if (first[0] < 0 && first[1] < 0)
    {
        first
    }
}

void drawLines(Mat image, vector<Vec2i> points)
{
    
}

vector<Box> ObjectExtractor::getObjects(vector<Mat> disparityVector, bool visualize)
{
    vector<Mat> pointCloud = getPointCloud(disparityVector);
    UnionSet unionSet((int)pointCloud.size(), pointCloud[0].cols, pointCloud[0].rows);
    segmentImage(pointCloud, unionSet);

    map<int, Box> setToBoxMap;

    for (int view = 0; view < pointCloud.size(); view++)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int setNumber = unionSet.findSet(view, i, j);
                Vec3f point = pointCloud[view].at<Vec3f>(i, j);

                if (unionSet.getCount(setNumber) >= 1500)
                {
                    setToBoxMap[setNumber].update(point);
                }
            }
        }
    }

    vector<Box> result(setToBoxMap.size());
    auto j = setToBoxMap.begin();
    for (int i = 0; j != setToBoxMap.end(); ++j, ++i)
    {
        result[i] = j->second;
    }

    if (visualize)
        unionSet.visualize();

    return result;
}