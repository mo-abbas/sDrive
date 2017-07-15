#pragma once
#include <opencv2/core/core.hpp>
#include "ObjectExtractor.h"
#include "Road.h"

using namespace cv;
using namespace std;

class OffRoadClipper
{
    vector<Road> road;
    vector<Mat> pointCloud;

    const float DISTANCE_THRESHOLD = 20.f;
    const float ROAD_HEIGHT_THRESHOLD = 0.3f;

    int Sign(float value);

    float AverageRoadHeight();

    pair<Mat, Mat> GetRoadBorders();
    pair<Mat, Mat> GetWorldPoints(Road& roadDetector, Direction direction);

    int GetInRoadDirection(Mat& polyCoefficients, Direction direction);
    bool PointIsInDirection(Vec3f point, Mat& polyCoefficients, int direction);
    
    float EucliedianDistance(Vec3f point);

public:
    Mat leftCoefficients, rightCoefficients;

    OffRoadClipper(vector<Road>& road, vector<Mat>& pointCloud);
    float EvaluatePolynomial(float x, Mat& polyCoefficients);
    float Clip();
};