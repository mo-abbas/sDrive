#include "PointCloud.h"
#include "OffRoadClipper.h"

Mat PointCloud::ConvertDisparityToXYZ(Mat& disparity, Direction currentDirection)
{
    int height = disparity.rows;
    int width = disparity.cols;

    Mat X(height, width, CV_32F);
    Mat Y(height, width, CV_32F);
    Mat Z = focalLength * baseline / disparity;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (disparity.at<float>(i, j) < DISPARITY_THRESHOLD)
            {
                // the leftCameraLocation will be removed when the co-ordinates are converted to the global origin
                X.at<float>(i, j) = -leftCameraLocation[0];
                Y.at<float>(i, j) = 0;
                Z.at<float>(i, j) = -leftCameraLocation[2];
            }
            else
            {
                X.at<float>(i, j) = (j - width / 2.f) * baseline / disparity.at<float>(i, j);
                Y.at<float>(i, j) = (height / 2.f - i) * baseline / disparity.at<float>(i, j);
            }
        }
    }

    vector<Mat> coordinates;
    coordinates.push_back(X);
    coordinates.push_back(Y);
    coordinates.push_back(Z);

    ConvertPointsToGlobalOrigin(coordinates, currentDirection);

    Mat cloud;
    merge(coordinates, cloud);

    return cloud;
}

void PointCloud::ConvertPointsToGlobalOrigin(vector<Mat>& points, Direction direction)
{
    // translate to the global origin
    points[0] += leftCameraLocation[0];
    points[2] += leftCameraLocation[2];

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

// The disparity vector has the format Front, Right, Back, Left
PointCloud::PointCloud(vector<Mat>& disparityVector, float fovx, float baseline, float focalLength, float pixelSize, Vec3f leftCameraLocation)
{
    this->height = disparityVector[0].rows;
    this->width = disparityVector[0].cols;

    this->fovx = fovx;
    this->baseline = baseline;
    this->pixelSize = pixelSize;
    this->focalLength = focalLength;
    this->averageRoadHeight = 0;

    this->disparityVector = disparityVector;
    this->leftCameraLocation = leftCameraLocation;

    for (int i = 0; i < disparityVector.size(); i++)
    {
        pointCloud.push_back(ConvertDisparityToXYZ(disparityVector[i], (Direction)i));
    }
}

Vec2i PointCloud::ProjectPointTo2D(Vec3f point)
{

    int x = int(point[0] * (focalLength / pixelSize) / point[2] + width / 2.f);
    int y = int(height / 2.f - point[1] * (focalLength / pixelSize) / point[2]);

    Vec2i result;
    result[0] = x;
    result[1] = y;

    return result;
}

Vec2f PointCloud::GetCameraProjection(Vec3f point, Direction direction)
{
    Vec3f newPoint = ConvertPointToLocalOrigin(point, direction);
    return ProjectPointTo2D(newPoint);
}

Vec3f PointCloud::ConvertPointToLocalOrigin(Vec3f point, Direction direction)
{
    Vec3f result;
    result[1] = point[1];

    switch (direction)
    {
    case FRONT:
        result[0] = point[0];
        result[2] = point[2];
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
    result[0] -= leftCameraLocation[0];
    result[2] -= leftCameraLocation[2];

    return result;
}

Direction PointCloud::PointDirectionInView(Vec3f point)
{
    float x = point[0];
    float z = point[2];

    float temp = tan((fovx / 2.0f) * M_PI / 180);

    // check if the point is behind the right border z = m * x
    if (z - tan((fovx / 2.0f) * M_PI / 180) * x < 0)
        return x >= 0 ? RIGHT : LEFT;

    // check if the point is behind the left border z = - m * x
    if (z + tan((fovx / 2.0f) * M_PI / 180) * x < 0)
        return x >= 0 ? RIGHT : LEFT;

    return FRONT;
}

Vec3f PointCloud::IntersectionWithCameraBorders(Vec3f point1, Vec3f point2, Direction direction)
{
    /*
    * This calculates the intersection of a line with a plane.
    * The line is represented by the parametric equations
    * x = x1 + a * t
    * y = y1 + b * t
    * z = z1 + c * t
    * Where a, b anc c are calculated using the difference between the two ends of the line
    * e.g. a = x2 - x1, b = y2 - y1, c = z2 - z1
    *
    * The plane equation is represented as A * x + B * y + C * z + D = 0
    * so to solve the equations we find that t = (A * x1 + B * y1 + C * z1 + D) / (A * a + B * b + C * c)
    */

    Vec3f result;

    float a = point2[0] - point1[0];
    float b = point2[1] - point1[1];
    float c = point2[2] - point1[2];

    float A;
    if (direction == LEFT)
        A = tan((fovx / 2.0f) * M_PI / 180);
    else
        A = -tan((fovx / 2.0f) * M_PI / 180);

    float B = 0;
    float C = 1;
    float D = 0;

    float t = -(A * point1[0] + B * point1[1] + C * point1[2] + D) / (A * a + B * b + C * c);
    result[0] = point1[0] + a * t;
    result[1] = point1[1] + b * t;
    result[2] = point1[2] + c * t;

    return result;
}

float PointCloud::GetAverageRoadHeight()
{
    return averageRoadHeight;
}

pair<Mat, Mat> PointCloud::GetRoadBorders()
{
    return make_pair(leftRoadCoefficients, rightRoadCoefficients);
}

void PointCloud::ClipExtraPoints(vector<Road>& roadVector)
{
    OffRoadClipper clipper(roadVector, pointCloud);
    averageRoadHeight = clipper.Clip();

    leftRoadCoefficients = clipper.leftCoefficients;
    rightRoadCoefficients = clipper.rightCoefficients;
}