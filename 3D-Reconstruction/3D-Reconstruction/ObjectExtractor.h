#pragma once
#include "UnionSet.h"

enum Direction
{
    FRONT = 0,
    RIGHT = 1,
    BACK = 2,
    LEFT = 3
};

struct Box
{
    Vec3f minX;
    Vec3f maxX;
    Vec3f minY;
    Vec3f maxY;
    Vec3f minZ;
    Vec3f maxZ;

    Box()
    {
        minX[0] = 1e7;
        maxX[0] = -1e7;
        minY[1] = 1e7;
        maxY[1] = -1e7;
        minZ[2] = 1e7;
        maxZ[2] = -1e7;
    }

    void update(Vec3f point)
    {
        if (point[0] > maxX[0])
            maxX = point;
        else if (point[0] < minX[0])
            minX = point;

        if (point[1] > maxY[1])
            maxY = point;
        else if (point[1] < minY[1])
            minY = point;

        if (point[2] > maxZ[2])
            maxZ = point;
        else if (point[2] < maxZ[2])
            minZ = point;
    }
};

class ObjectExtractor
{
private:
    int width;
    int height;
    float baseline;
    float focalLength;
    const float INF = 1e10;
    const float THRESHOLD = 0.3f;

    float locationDifference(Vec3f a, Vec3f b);
    void convertPointsToGlobalOrigin(vector<Mat>& points, Direction direction);

    Mat convertDisparityToXYZ(Mat disparity, Direction currentDirection);

    Direction getRightDirection(Direction currentDirection);

    Vec2f getCameraProjection(Vec3f point, Direction currentDirection);

    Vec3f convertPointToLocalOrigin(Vec3f point, Direction direction);

    bool collidesWithTheRightCamera(Vec3f point, Direction currentDirection);

    Vec2i projectPointTo2D(Vec3f point);

    void segmentImage(vector<Mat>& pointCloud, UnionSet& unionSet);

    vector<Mat> getPointCloud(vector<Mat> disparityVector);

    vector<Vec2i> boxTo2D(Box box, Direction direction);

public:

    // The disparity vector has the format Front, Right, Back, Left
    ObjectExtractor(int width, int height, float baseline, float focalLength);

    vector<Box> getObjects(vector<Mat> disparityVector, bool visualize = false);
};