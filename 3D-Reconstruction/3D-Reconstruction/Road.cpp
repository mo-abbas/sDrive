#include <opencv2/imgproc/imgproc.hpp>

#include "Road.h"
#include "PointCloud.h"

vector<Point> Road::getContour()
{
    Mat image = road.clone();

    //delete left and right white pixels
    for (int i = 0; i < image.rows; i++)
    {
        image.at<uchar>(i, 0) = 0;
        image.at<uchar>(i, image.cols - 1) = 0;
    }

    //delete top and bottom white pixels
    for (int i = 0; i < image.cols; i++)
    {
        image.at<uchar>(0, i) = 0;
        image.at<uchar>(image.rows - 1, i) = 0;
    }

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    /// Find contours
    findContours(image, contours, hierarchy, RETR_LIST, CHAIN_APPROX_NONE, Point(0, 0));

    //find contour with the max area
    double maxArea = 0;
    int maxAreaPos = 0;
    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        if (area > maxArea)
        {
            maxArea = area;
            maxAreaPos = i;
        }
    }

    vector<Point>& contour = contours[maxAreaPos];

    if (direction == FRONT || direction == BACK)
    {
        sort(contour.begin(), contour.end(), frontAndBackSortingObject);
    }
    else
    {
        sort(contour.begin(), contour.end(), sidesSortingObject);
    }

    return contour;
}

void Road::calculateSidesBorders()
{
    vector<Point> contour = getContour();
    vector<int> minVector(road.cols);

    for (int i = (int)contour.size() - 1, j = 0; i >= 0; i--)
    {
        j = 0;
        int minVal = (int)1e7;
        while (i - j >= 0 && contour[i].x == contour[i - j].x)
        {
            if (contour[i - j].y < minVal)
                minVal = contour[i - j].y;
            j++;
        }

        i -= j;

        if (j != 0)
            i++;

        minVector[contour[i].y] = minVal;
    }

    vector<Point>& border = direction == LEFT ? left : right;

    for (int i = 0; i < contour.size(); i++)
    {
        // skip duplicate points
        while (i < contour.size() - 1 && contour[i] == contour[i + 1]) i++;

        Point point = contour[i];
        if (point.y <= minVector[point.x] && point.y > 10 && point.y < road.rows - 10)
        {
            border.push_back(point);
        }
    }
}

void Road::calculateFrontOrBackBorders()
{
    vector<Point> contour = getContour();

    int firstFivePercent = 0;
    int highestPointY = contour.back().y;
    for (int i = (int)contour.size() - 1; i >= 0 && highestPointY + road.rows / 20 > contour[i].y; i--)
    {
        firstFivePercent++;
    }

    contour.erase(contour.end() - firstFivePercent, contour.end());

    vector<int> minVector(road.rows);
    vector<int> maxVector(road.rows);

    for (int i = (int)contour.size() - 1, j = 0; i >= 0; i--)
    {
        j = 0;
        int minVal = 15000, maxVal = -1;
        while (i - j >= 0 && contour[i].y == contour[i - j].y)
        {
            if (contour[i - j].x < minVal)
                minVal = contour[i - j].x;
            if (contour[i - j].x > maxVal)
                maxVal = contour[i - j].x;
            j++;
        }

        i -= j;

        if (j != 0)
            i++;

        minVector[contour[i].y] = minVal;
        maxVector[contour[i].y] = maxVal;
    }

    for (int i = 0; i < contour.size(); i++)
    {
        // skip duplicate points
        while (i < contour.size() - 1 && contour[i] == contour[i + 1]) i++;

        Point point = contour[i];
        if (point.x <= minVector[point.y] && point.x > 10 && point.x < road.cols - 10)
        {
            left.push_back(point);
        }
        else if (point.x >= maxVector[point.y] && point.x > 10 && point.x < road.cols - 10)
        {
            right.push_back(point);
        }
    }
}

Road::Road(Mat road, Direction direction)
{
    this->road = road.clone();
    this->direction = direction;

    switch (direction)
    {
    case FRONT:
    case BACK:
        calculateFrontOrBackBorders();
        break;
    case RIGHT:
    case LEFT:
        calculateSidesBorders();
    }
}