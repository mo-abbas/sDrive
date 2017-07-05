#pragma once
#include <opencv2/core/core.hpp>
#include "ObjectExtractor.h"
#include "Road.h"

using namespace cv;
using namespace std;

class OffRoadClipper
{
    vector<Mat> road;
    vector<Mat> pointCloud;

    int sign(float value);
    float averageRoadHeight();
    pair<Mat, Mat> getRoadBorders();
    pair<Mat, Mat> getWorldPoints(Road& roadDetector, Direction direction);
    int getInRoadDirection(Mat& polyCoefficients, Direction direction);
    bool pointIsInDirection(Vec3f point, Mat& polyCoefficients, int direction);
    float eucliedianDistance(Vec3f point);

public:
    Mat leftCoefficients, rightCoefficients;

    OffRoadClipper(vector<Mat> road, vector<Mat> pointCloud);
    float evaluatePolynomial(float x, Mat& polyCoefficients);
    float clip();
};