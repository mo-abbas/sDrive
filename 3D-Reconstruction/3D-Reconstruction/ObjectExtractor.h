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
        if (abs(point[2] - (-8.5169495)) < 1e-5)
            int x = 2;

        if (point[0] > maxX[0])
            maxX = point;
        if (point[0] < minX[0])
            minX = point;

        if (point[1] > maxY[1])
            maxY = point;
        if (point[1] < minY[1])
            minY = point;

        if (point[2] > maxZ[2])
            maxZ = point;
        if (point[2] < minZ[2])
            minZ = point;
    }
};

class ObjectExtractor
{
private:
    int width;
    int height;
    float fovx;
    float baseline;
    float focalLength;
    Vec3f leftCameraLocation;

    const float INF = 1e10;
    const float THRESHOLD = 0.3f;
    const int DISPARITY_THRESHOLD = 10;
    const int OBJECT_SIZE_THRESHOLD = 1500;

    float euclidianDistance(Vec2f a, Vec2f b);
    float euclidianDistance(Vec3f a, Vec3f b);
    void convertPointsToGlobalOrigin(vector<Mat>& points, Direction direction);

    Mat convertDisparityToXYZ(Mat& disparity, Direction currentDirection);

    Direction getRightDirection(Direction currentDirection);

    Vec2f getCameraProjection(Vec3f point, Direction currentDirection);

    Vec3f convertPointToLocalOrigin(Vec3f point, Direction direction);

    bool collidesWithTheRightCamera(Vec3f point, Direction currentDirection);

    Vec2i projectPointTo2D(Vec3f point);

    void segmentImage(vector<Mat>& pointCloud, UnionSet& unionSet);

    vector<Mat> getPointCloud(vector<Mat>& disparityVector);

    vector<Box> getBoxesFromPointCloud(vector<Mat>& pointCloud, UnionSet& unionSet, float heightThreshold);

    vector<Vec2f> getImageBorderIntersections(Vec2i a, Vec2i b);

    Direction pointDirectionInView(Vec3f point);

    Vec3f intersectionWithCameraBorders(Vec3f point1, Vec3f point2, Direction direction);

    vector<Vec3f> ObjectExtractor::boxToVertices(Box box);

    void drawLine(Mat& image, Vec3f a, Vec3f b, Direction direction);

    void drawCuboid(Mat& image, vector<Vec3f>& points, Direction direction);

    void visualizeBoxes(vector<Mat>& disparityVector, vector<Box> boxes);

public:

    // The disparity vector has the format Front, Right, Back, Left
    ObjectExtractor(int width, int height, float fovx, float baseline, float focalLength, Vec3f leftCameraLocation);

    vector<Box> getObjects(vector<Mat>& disparityVector, vector<Mat>& roadVector, bool visualize = false);
};