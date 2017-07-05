#pragma once

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;

class Road
{
    int thresh = 100;
    int max_thresh = 255;

    struct sortClass {
        bool operator() (Point i, Point j) 
        {
            if (i.y == j.y)
                return i.x < j.x;
            else
                return i.y > j.y; 
        }
    } sortingObject;

public:
    Mat src;
    vector<Point> left, right;
    Point leftPlanB, rightPlanB;

    void getContours(Mat image)
    {
        leftPlanB = Point(0, 0);
        rightPlanB = Point(0, 0);

        src = image;
        
        //delete most right and left white pixels
        for (int i = 0; i < src.rows; i++)
        {
            src.at<uchar>(i, 0) = 0;
            src.at<uchar>(i, src.cols - 1) = 0;
        }

        //delete most bottom white pixels

        for (int i = 0; i < src.cols; i++)
        {
            src.at<uchar>(src.rows - 1, i) = 0;
        }

        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;

        /// Find contours
        findContours(src, contours, hierarchy, RETR_LIST, CHAIN_APPROX_NONE, Point(0, 0));

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
        
        vector<Point>& road = contours[maxAreaPos];
        sort(road.begin(), road.end(), sortingObject);

        int firstFivePercent = 0;
        int highestPointY = road.back().y;
        for (int i = road.size() - 1; i >= 0 && highestPointY + src.rows / 20 > road[i].y; i--)
        {
            firstFivePercent++;
        }

        road.erase(road.end() - firstFivePercent, road.end());
        line(src, Point(0, road.back().y), Point(src.cols - 1, road.back().y), Scalar(255, 255, 0));

        highestPointY = road.back().y;
        vector<int> minVector(src.rows);
        vector<int> maxVector(src.rows);

        for (int i = road.size() - 1, j = 0; i >= 0; i--)
        {
            j = 0;
            int minVal = 15000, maxVal = -1;
            while (i - j >= 0 && road[i].y == road[i - j].y)
            {
                if (road[i - j].x < minVal)
                    minVal = road[i - j].x;
                if (road[i - j].x > maxVal)
                    maxVal = road[i - j].x;
                j++;
            }

            i -= j;

            if (j != 0)
                i++;

            minVector[road[i].y] = minVal;
            maxVector[road[i].y] = maxVal;
        }

        for (int i = 0; i < road.size(); i++)
        {
            if (road[i].x < 5 && road[i].y > leftPlanB.y)
            {
                leftPlanB = road[i];
            }

            // skip duplicate points
            while (i < road.size() - 1 && road[i] == road[i + 1]) i++;

            if (road[i].x > src.cols - 5 && road[i].y > rightPlanB.y)
            {
                rightPlanB = road[i];
            }

            Point point = road[i];
            if (point.x <= minVector[point.y] && point.x > 10 && point.x < src.cols - 10)
            {
                left.push_back(point);
            }
            else if (point.x >= maxVector[point.y] && point.x > 10 && point.x < src.cols - 10)
            {
                right.push_back(point);
            }
        }
    }
};