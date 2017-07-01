#pragma once

#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

class UnionSet
{
private:
    vector<Mat> parent;
    vector<Mat> rank;
    vector<Mat> count;

    int width;
    int height;
    int backgroundSet;

    int getRank(int index);
    void setRank(int index, int rank);
    void setParent(int index, int setNumber);

public:
    UnionSet(int images, int width, int height);

    int findSet(int index);
    int findSet(int image, int row, int col);

    bool isSameSet(int index1, int index2);
    void unionSet(int index1, int index2);
    void setBackground(int index);
    bool isBackground(int setNumber);

    int getCount(int setNumber);
    void visualize();
};