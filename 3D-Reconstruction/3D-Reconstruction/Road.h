#pragma once

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;

class road
{
    int thresh = 100;
    int max_thresh = 255;

    struct sortClass {
        bool operator() (Point i, Point j) { return (i.y>j.y); }
    } myobject;

public:
    Mat src; Mat src_gray;
    vector<Point> left, right;

    void getContours()
    {
        Mat canny_output;
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;

        /// Detect edges using canny
        Canny(src_gray, canny_output, thresh, thresh * 2, 3);
        /// Find contours
        findContours(src_gray, contours, hierarchy, CV_RETR_TREE, CHAIN_APPROX_NONE, Point(0, 0));

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

        sort(contours[maxAreaPos].begin(), contours[maxAreaPos].end(), myobject);
        int first5 = 0; // contours[maxAreaPos].size() / 10;
        int highestPointY = contours[maxAreaPos].back().y;
        for (int i = contours[maxAreaPos].size() - 1; i >= 0 && highestPointY + src.rows / 20 > contours[maxAreaPos][i].y; i--)
        {
            first5++;
        }

        contours[maxAreaPos].erase(contours[maxAreaPos].end() - first5, contours[maxAreaPos].end());
        int x = contours[maxAreaPos].size() - 1;
        line(src, Point(0, contours[maxAreaPos][x].y), Point(src.cols - 1, contours[maxAreaPos][x].y), Scalar(255, 255, 0));
        //int j = 1;
        //while (contours[maxAreaPos][x].y == contours[maxAreaPos][x - j].y)
        //{
        //    j++;
        //}
        //contours[maxAreaPos].erase(contours[maxAreaPos].end() - j, contours[maxAreaPos].end());

        //x -= j;
        //j = 1;
        int avgMinVal = 0, avgMaxVal = 0;
        int count = 0;
        highestPointY = contours[maxAreaPos].back().y;
        for (int i = contours[maxAreaPos].size() - 1, j = 0; i >= 0 && highestPointY + src.rows / 20 > contours[maxAreaPos][i].y; i--)
        {
            j = 0;
            int minVal = 15000, maxVal = -1;
            while (i - j >= 0 && contours[maxAreaPos][i].y == contours[maxAreaPos][i - j].y)
            {
                if (contours[maxAreaPos][i - j].x < minVal)
                    minVal = contours[maxAreaPos][i - j].x;
                if (contours[maxAreaPos][i - j].x > maxVal)
                    maxVal = contours[maxAreaPos][i - j].x;
                j++;
            }

            i -= j;

            if (j != 0)
                i++;

            count++;
            avgMinVal += minVal;
            avgMaxVal += maxVal;
        }

        int minVal = avgMinVal / count, maxVal = avgMaxVal / count;
        line(src, Point(minVal, 0), Point(minVal, src.rows - 1), Scalar(0, 255, 0));
        line(src, Point(maxVal, 0), Point(maxVal, src.rows - 1), Scalar(0, 0, 255));
        vector<Point> uniqueCont;

        vector<vector<bool>> check(src.cols, vector<bool>(src.rows, false));
        for (int i = 0; i < contours[maxAreaPos].size(); i++)
        {
            bool point = check[contours[maxAreaPos][i].x][contours[maxAreaPos][i].y];
            if (!point)
            {
                uniqueCont.push_back(Point(contours[maxAreaPos][i].x, contours[maxAreaPos][i].y));
                check[contours[maxAreaPos][i].x][contours[maxAreaPos][i].y] = true;
            }
        }

        for (int i = 0; i < uniqueCont.size(); i++)
        {
            if (uniqueCont[i].x < minVal && uniqueCont[i].x > 10 && uniqueCont[i].x < src.cols - 10)
            {
                left.push_back(uniqueCont[i]);
            }
            else if (uniqueCont[i].x > maxVal && uniqueCont[i].x > 10 && uniqueCont[i].x < src.cols - 10)
            {
                right.push_back(uniqueCont[i]);
            }
        }
    }

    road(string imageName)
    {
        src = imread(imageName, 1);

        /// Convert image to gray and blur it
        cvtColor(src, src_gray, CV_BGR2GRAY);
       // blur(src_gray, src_gray, Size(3, 3));

        //delete most right and left white pixels
        for (int i = 0; i < src.rows; i++)
        {
            Scalar intensty = src_gray.at<uchar>(i, 0);
            Scalar intensty2 = src_gray.at<uchar>(i, src.cols - 1);
            if (intensty.val[0] != 0)
            {
                src_gray.at<uchar>(i, 0) = 0;
            }
            if (intensty2.val[0] != 0)
            {
                src_gray.at<uchar>(i, src.cols - 1) = 0;
            }
        }

        //delete most bottom white pixels

        for (int i = 0; i < src.cols; i++)
        {
            Scalar intensty = src_gray.at<uchar>(src.rows - 1, i);
            if (intensty.val[0] != 0)
            {
                src_gray.at<uchar>(src.rows - 1, i) = 0;
            }
        }

        getContours();
    }
};