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

    int GetRank(int index);
    void SetRank(int index, int rank);
    void SetParent(int index, int setNumber);

public:
    UnionSet(int images, int width, int height);

    int FindSet(int index);
    int FindSet(int image, int row, int col);

    bool IsSameSet(int index1, int index2);
    void UnionSets(int index1, int index2);
    void SetAsBackground(int index);
    bool IsBackground(int setNumber);

    int GetCount(int setNumber);
    vector<Mat> Visualize();
};