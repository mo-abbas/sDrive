#include "OffRoadClipper.h"
#include "PointCloud.h"
#include "PolyFit.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

int OffRoadClipper::Sign(float value)
{
    return (0 < value) - (value < 0);
}

float OffRoadClipper::AverageRoadHeight()
{
    double sum = 0;
    int count = 0;

    for (int channel = 0; channel < pointCloud.size(); channel ++) // front and back only
    {
        for (int i = 0; i < pointCloud[channel].rows; i++)
        {
            for (int j = 0; j < pointCloud[channel].rows; j++)
            {
                Vec3f point = pointCloud[channel].at<Vec3f>(i, j);
                if (road[channel].at<uchar>(i, j) == 255)
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

void OffRoadClipper::GetRoadBorders()
{
    pair<Mat, Mat> points = GetWorldPoints(road[0], FRONT);
    Mat leftPoints = points.first;
    Mat rightPoints = points.second;

	pt::ptree config;

	if (fs::exists("config.ini"))
		pt::ini_parser::read_ini("config.ini", config);
	else
		throw new exception("config file not found.");

	int order = config.get<int>("settings.order");

    if (road.size() > 1)
    {
        points = GetWorldPoints(road[1], RIGHT);
        rightPoints.push_back(points.first);

        points = GetWorldPoints(road[2], BACK);
        leftPoints.push_back(points.second);
        rightPoints.push_back(points.first);

        points = GetWorldPoints(road[3], LEFT);
        leftPoints.push_back(points.first);
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
        Polyfit(leftPoints.col(1), leftPoints.col(0), leftCoefficients, order);
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
        Polyfit(rightPoints.col(1), rightPoints.col(0), rightCoefficients, order);
    }
}

pair<Mat, Mat> OffRoadClipper::GetWorldPoints(Road& road, Direction direction)
{
    int channel = (int)direction;

    Mat leftPoints(0, 2, CV_32F);
    Mat rightPoints(0, 2, CV_32F);

    for (int i = 0; i < road.left.size(); i++)
    {
        Point location = road.left[i];
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

    for (int i = 0; i < road.right.size(); i++)
    {
        Point location = road.right[i];
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

int OffRoadClipper::GetInRoadDirection(Mat& polyCoefficients, Direction direction)
{
    int offset = 50;
    if (direction == RIGHT)
    {
        offset *= -1;
    }

    // X at Z = 0
    float value = EvaluatePolynomial(0, polyCoefficients);

    // The polynomial is X = F(Z), so to get the sign of the needed direction
    // we use X = F(Z) + offset and use it in the equation sign(X - F(Z))
    float X = value + offset;

    // true  -> 1 * 2 - 1 = 1
    // false -> 0 * 2 - 1 = -1
    return Sign(X - value);
}

// direction is either -1 or +1
bool OffRoadClipper::PointIsInDirection(Vec3f point, Mat& polyCoefficients, int direction)
{
    float value = EvaluatePolynomial(point[2], polyCoefficients);
    return Sign(point[0] - value) == direction;
}

float OffRoadClipper::EucliedianDistance(Vec3f point)
{
    return sqrt(point[0] * point[0] + point[2] * point[2]);
}

OffRoadClipper::OffRoadClipper(vector<Road>& road, PointCloud& pointCloud) : pointCloud(pointCloud)
{
    this->road = road;

    GetRoadBorders();
}

float OffRoadClipper::EvaluatePolynomial(float x, Mat& polyCoefficients)
{
    float result = 0;

    for (int i = 0; i < polyCoefficients.rows; i++)
    {
        result += pow(x, i) * polyCoefficients.at<float>(i, 0);
    }

    return result;
}

void OffRoadClipper::AdjustFromPrevious(Mat previousLeft, Mat previousRight)
{
    leftCoefficients = 0.8 * previousLeft + 0.2 * leftCoefficients;
    rightCoefficients = 0.8 * previousRight + 0.2 * rightCoefficients;
}

// also returns the average height of the road
void OffRoadClipper::Clip()
{
    int leftInRoadSign = GetInRoadDirection(leftCoefficients, LEFT);
    int rightInRoadSign = GetInRoadDirection(rightCoefficients, RIGHT);
    roadAverageHeight = AverageRoadHeight();

    for (int channel = 0; channel < pointCloud.size(); channel++)
    {
        for (int i = 0; i < pointCloud[channel].rows; i++)
        {
            for (int j = 0; j < pointCloud[channel].cols; j++)
            {
                Vec3f& point = pointCloud[channel].at<Vec3f>(i, j);
                bool isRoad = point[1] <= (roadAverageHeight + ROAD_HEIGHT_THRESHOLD);
                bool isFar = EucliedianDistance(point) > DISTANCE_THRESHOLD;

                if (isRoad || isFar || !PointIsInDirection(point, leftCoefficients, leftInRoadSign) || !PointIsInDirection(point, rightCoefficients, rightInRoadSign))
                {
                    point[0] = 0;
                    point[1] = 0;
                    point[2] = 0;
                }
            }
        }
    }
}