#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

enum Direction;

class Road
{
    Direction direction;

    struct FrontAndBackSorting {
        bool operator() (Point i, Point j) 
        {
            if (i.y == j.y)
                return i.x < j.x;
            else
                return i.y > j.y; 
        }
    } frontAndBackSortingObject;

    struct SidesSorting {
        bool operator() (Point i, Point j)
        {
            if (i.x == j.x)
                return i.y < j.y;
            else
                return i.x > j.x;
        }
    } sidesSortingObject;

    vector<Point> getContour();
    void calculateSidesBorders();
    void calculateFrontOrBackBorders();

public:
    Mat road;
    vector<Point> left, right;

    Road(Mat road, Direction direction);

    template <class T>
    T at(int row, int col)
    {
        return road.at<T>(row, col);
    }
};