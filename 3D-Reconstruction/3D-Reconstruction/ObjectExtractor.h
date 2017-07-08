#pragma once
#include "Box.h"
#include "UnionSet.h"
#include "PointCloud.h"

class ObjectExtractor
{
private:
    int width;
    int height;
    float fovx;
    float baseline;
    float pixelSize;
    float focalLength;

    UnionSet unionSet;
    vector<Box> boxes;
    PointCloud& pointCloud;
    Vec3f leftCameraLocation;

    const float INF = 1e10f;
    const float THRESHOLD = 0.3f;
    const int DISPARITY_THRESHOLD = 10;
    const int OBJECT_SIZE_THRESHOLD = 1500;

    float EuclidianDistance(Vec3f a, Vec3f b);

    Direction GetRightDirection(Direction currentDirection);
    bool CollidesWithTheRightCamera(Vec3f point, Direction currentDirection);

    vector<Vec3f> ObjectExtractor::BoxToVertices(Box box);

    void DrawLine(Mat& image, Vec3f a, Vec3f b, Direction direction);
    void DrawCuboid(Mat& image, vector<Vec3f>& points, Direction direction);

    void SegmentImage();
    void GetBoxesFromPointCloud();

    void DetectObjects();

public:
    ObjectExtractor(PointCloud& pointCloud);

    vector<Vec2f> GetCars();
    pair<Mat, Mat> GetRoadBorders();

    vector<Mat> VisualizeBoxes();
    vector<Mat> VisualizeSegmentation();
};