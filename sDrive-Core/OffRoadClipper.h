#pragma once
#include <opencv2/core/core.hpp>
#include "ObjectExtractor.h"
#include "Road.h"

using namespace cv;
using namespace std;

class PointCloud;

class OffRoadClipper
{
    vector<Road> road;
    PointCloud& pointCloud;

    const float DISTANCE_THRESHOLD = 25.f;
    const float ROAD_HEIGHT_THRESHOLD = 0.3f;

    int Sign(float value);

    float AverageRoadHeight();

    void GetRoadBorders();
    pair<Mat, Mat> GetWorldPoints(Road& roadDetector, Direction direction);

    int GetInRoadDirection(Mat& polyCoefficients, Direction direction);
    bool PointIsInDirection(Vec3f point, Mat& polyCoefficients, int direction);
    
    float EucliedianDistance(Vec3f point);

public:
    Mat leftCoefficients, rightCoefficients;
    float roadAverageHeight;

    OffRoadClipper(vector<Road>& road, PointCloud& pointCloud);
    float EvaluatePolynomial(float x, Mat& polyCoefficients);
    void AdjustFromPrevious(Mat previousLeft, Mat previousRight);
    void Clip();
};