#include "OffRoadClipper.h"
#include "ObjectExtractor.h"
#include "PolyFit.h"

int OffRoadClipper::sign(float value)
{
    return (0 < value) - (value < 0);
}

OffRoadClipper::OffRoadClipper(vector<Mat> road, vector<Mat> pointCloud)
{
    this->road = road;
    this->pointCloud = pointCloud;
}

pair<Mat, Mat> OffRoadClipper::getWorldPoints(Road& roadDetector, Direction direction)
{
    int channel = (int)direction;

    Mat leftPoints(0, 2, CV_32F);
    Mat rightPoints(0, 2, CV_32F);

    for (int i = 0; i < roadDetector.left.size(); i++)
    {
        Point location = roadDetector.left[i];
        Vec3f point = pointCloud[channel].at<Vec3f>(location.y, location.x);
        if (point[0] > FLT_EPSILON || point[0] < -FLT_EPSILON ||
            point[1] > FLT_EPSILON || point[1] < -FLT_EPSILON ||
            point[2] > FLT_EPSILON || point[2] < -FLT_EPSILON)
        {
            Mat temp(1, 2, CV_32F);
            temp.at<float>(0, 0) = point[0];
            temp.at<float>(0, 1) = point[2];
            leftPoints.push_back(temp);
        }
    }

    for (int i = 0; i < roadDetector.right.size(); i++)
    {
        Point location = roadDetector.right[i];
        Vec3f point = pointCloud[channel].at<Vec3f>(location.y, location.x);
        if (point[0] > FLT_EPSILON || point[0] < -FLT_EPSILON ||
            point[1] > FLT_EPSILON || point[1] < -FLT_EPSILON ||
            point[2] > FLT_EPSILON || point[2] < -FLT_EPSILON)
        {
            Mat temp(1, 2, CV_32F);
            temp.at<float>(0, 0) = point[0];
            temp.at<float>(0, 1) = point[2];
            rightPoints.push_back(temp);
        }
    }

    return make_pair(leftPoints, rightPoints);
}

pair<Mat, Mat> OffRoadClipper::getRoadBorders()
{
    Road frontRoadDetector;
    Road backRoadDetector;

    frontRoadDetector.getContours(road[0]);
    pair<Mat, Mat> points = getWorldPoints(frontRoadDetector, FRONT);
    Mat leftPoints = points.first;
    Mat rightPoints = points.second;

    if (road.size() > 1)
    {
        backRoadDetector.getContours(road[1]);
        pair<Mat, Mat> points = getWorldPoints(backRoadDetector, BACK);
        leftPoints.push_back(points.second);
        rightPoints.push_back(points.first);
    }

    if (leftPoints.rows < 5)
    {
        leftCoefficients = Mat(2, 1, CV_32F);
        leftCoefficients.at<float>(0, 0) = -3;
        leftCoefficients.at<float>(1, 0) =  0;
    }
    else
    {
        // We consider the dependent value is the X-axis and the independent is the Z-axis
        polyfit(leftPoints.col(1), leftPoints.col(0), leftCoefficients, 1);
    }

    if (rightPoints.rows < 5)
    {
        rightCoefficients = Mat(2, 1, CV_32F);
        rightCoefficients.at<float>(0, 0) = 3;
        rightCoefficients.at<float>(1, 0) = 0;
    }
    else
    {
        // We consider the dependent value is the X-axis and the independent is the Z-axis
        polyfit(rightPoints.col(1), rightPoints.col(0), rightCoefficients, 1);
    }


    return make_pair(leftCoefficients, rightCoefficients);
}

int OffRoadClipper::getInRoadDirection(Mat& polyCoefficients, Direction direction)
{
    int offset = 50;
    if (direction == RIGHT)
    {
        offset *= -1;
    }

    // X at Z = 0
    float value = evaluatePolynomial(0, polyCoefficients);

    // The polynomial is X = F(Z), so to get the sign of the needed direction
    // we use X = F(Z) + offset and use it in the equation sign(X - F(Z))
    float X = value + offset;

    // true  -> 1 * 2 - 1 = 1
    // false -> 0 * 2 - 1 = -1
    return sign(X - value);
}

// direction is either -1 or +1
bool OffRoadClipper::pointIsInDirection(Vec3f point, Mat& polyCoefficients, int direction)
{
    float value = evaluatePolynomial(point[2], polyCoefficients);
    return sign(point[0] - value) == direction;
}

float OffRoadClipper::evaluatePolynomial(float x, Mat& polyCoefficients)
{
    float result = 0;

    for (int i = 0; i < polyCoefficients.rows; i++)
    {
        result += pow(x, i) * polyCoefficients.at<float>(i, 0);
    }

    return result;
}

float OffRoadClipper::averageRoadHeight()
{
    double sum = 0;
    int count = 0;

    for (int channel = 0; channel < pointCloud.size(); channel += 2) // front and back only
    {
        for (int i = 0; i < pointCloud[channel].rows; i++)
        {
            for (int j = 0; j < pointCloud[channel].rows; j++)
            {
                Vec3f point = pointCloud[channel].at<Vec3f>(i, j);
                if (road[channel / 2].at<uchar>(i, j) == 255)
                {
                    if (point[0] > FLT_EPSILON || point[0] < -FLT_EPSILON ||
                        point[1] > FLT_EPSILON || point[1] < -FLT_EPSILON ||
                        point[2] > FLT_EPSILON || point[2] < -FLT_EPSILON)
                    {
                        sum += point[1];
                        count++;
                    }
                }
            }
        }
    }

    return float(sum / count);
}

float OffRoadClipper::eucliedianDistance(Vec3f point)
{
    return sqrt(point[0] * point[0] + point[2] * point[2]);
}

// also returns the average height of the road
float OffRoadClipper::clip()
{
    pair<Mat, Mat> coefficients = getRoadBorders();
    int leftInRoad = getInRoadDirection(coefficients.first, LEFT);
    int rightInRoad = getInRoadDirection(coefficients.first, RIGHT);
    float averageHeight = averageRoadHeight();
    const float HEIGHT_THRESHOLD = 0.3f;

    for (int channel = 0; channel < pointCloud.size(); channel++)
    {
        for (int i = 0; i < pointCloud[channel].rows; i++)
        {
            for (int j = 0; j < pointCloud[channel].cols; j++)
            {
                Vec3f& point = pointCloud[channel].at<Vec3f>(i, j);
                bool isRoad = point[1] <= (averageHeight + HEIGHT_THRESHOLD);
                bool isFar = eucliedianDistance(point) > 20;

                if (isRoad || isFar || !pointIsInDirection(point, coefficients.first, leftInRoad) || !pointIsInDirection(point, coefficients.second, rightInRoad))
                {
                    point[0] = 0;
                    point[1] = 0;
                    point[2] = 0;
                }
            }
        }
    }

    return averageHeight;
}