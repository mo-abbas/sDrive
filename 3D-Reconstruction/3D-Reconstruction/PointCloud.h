#pragma once

#include <opencv2/core/core.hpp>
#include "Road.h"

using namespace cv;
using namespace std;

#define M_PI           3.14159265358979323846f  /* pi */

enum Direction
{
    FRONT = 0,
    RIGHT = 1,
    BACK = 2,
    LEFT = 3
};

class PointCloud
{
private:
    int width;
    int height;
    float fovx;
    float baseline;
    float pixelSize;
    float focalLength;

    float averageRoadHeight;

    vector<Mat> pointCloud;
    Vec3f leftCameraLocation;

    Mat leftRoadCoefficients;
    Mat rightRoadCoefficients;

    const int DISPARITY_THRESHOLD = 10;

    Mat ConvertDisparityToXYZ(Mat& disparity, Direction currentDirection);
    void ConvertPointsToGlobalOrigin(vector<Mat>& points, Direction direction);

public:
    vector<Mat> disparityVector;

    PointCloud(vector<Mat>& disparityVector, float fovx, float baseline, float focalLength, float pixelSize, Vec3f leftCameraLocation);

    Vec2i ProjectPointTo2D(Vec3f point);
    Vec2f GetCameraProjection(Vec3f point, Direction direction);
    Vec3f ConvertPointToLocalOrigin(Vec3f point, Direction direction);

    Direction PointDirectionInView(Vec3f point);
    Vec3f IntersectionWithCameraBorders(Vec3f point1, Vec3f point2, Direction direction);

    float GetAverageRoadHeight();
    pair<Mat, Mat> GetRoadBorders();

    void ClipExtraPoints(vector<Road>& roadVector);

    Mat operator[] (int i)
    {
        return pointCloud[i];
    }

    int size()
    {
        return (int)pointCloud.size();
    }
};