#include "UnionSet.h"
#include <iostream>
#include <ctime>

int UnionSet::GetRank(int index)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    return rank[image].at<int>(row, col);
}

void UnionSet::SetRank(int index, int rank)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    this->rank[image].at<int>(row, col) = rank;
}

void UnionSet::SetParent(int index, int setNumber)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    int setImage = setNumber / (height * width);
    int setRow = (setNumber % (height * width)) / width;
    int setCol = (setNumber % (height * width)) % width;

    parent[image].at<int>(row, col) = setNumber;
    count[setImage].at<int>(setRow, setCol) += count[image].at<int>(row, col);
    count[image].at<int>(row, col) = 0;
}

UnionSet::UnionSet(int images, int width, int height)
{
    for (int i = 0; i < images; i++)
    {
        parent.push_back(Mat(height, width, CV_32SC1));
        count.push_back(Mat(height, width, CV_32SC1, Scalar(1)));
        rank.push_back(Mat(height, width, CV_32SC1, Scalar(0)));
    }

    // add 1 pixel for the background set
    parent.push_back(Mat(1, 1, CV_32SC1));
    count.push_back(Mat(1, 1, CV_32SC1, Scalar(1)));
    rank.push_back(Mat(1, 1, CV_32SC1));

    backgroundSet = images * width * height;
    parent.back().at<int>(0, 0) = backgroundSet;
    rank.back().at<int>(0, 0) = backgroundSet;

    for (int k = 0; k < images; k++)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                parent[k].at<int>(i, j) = k * width * height + i * width + j;
            }
        }
    }

    this->width = width;
    this->height = height;
}

int UnionSet::FindSet(int index)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    int& value = parent[image].at<int>(row, col);

    return (value == index) ? index : (value = FindSet(value));
}

int UnionSet::FindSet(int image, int row, int col)
{
    return FindSet(image * (height * width) + row * width + col);
}

bool UnionSet::IsSameSet(int index1, int index2)
{
    return FindSet(index1) == FindSet(index2);
}

void UnionSet::UnionSets(int index1, int index2)
{
    if (!IsSameSet(index1, index2))
    {
        int parent1 = FindSet(index1);
        int parent2 = FindSet(index2);

        int rank1 = GetRank(parent1);
        int rank2 = GetRank(parent2);

        if (rank1 > rank2)
        {
            SetParent(parent2, parent1);
        }
        else if (rank2 > rank1)
        {
            SetParent(parent1, parent2);
        }
        else
        {
            SetRank(parent2, rank2 + 1);
            SetParent(parent1, parent2);
        }
    }
}

void UnionSet::SetAsBackground(int index)
{
    this->UnionSets(index, backgroundSet);
}

bool UnionSet::IsBackground(int setNumber)
{
    return setNumber == backgroundSet;
}

int UnionSet::GetCount(int index)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    return count[image].at<int>(row, col);
}

vector<Mat> UnionSet::Visualize()
{
    map<int, int> mymap;
    vector<Mat> output(parent.size() - 1);

    int colorNumber = 0;

    for (int view = 0; view < parent.size() - 1; view++)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int index = view * width * height + i * width + j;
                int setNumber = FindSet(index);

                if (mymap.find(setNumber) == mymap.end())
                {
                    int setImage = setNumber / (height * width);
                    int setRow = (setNumber % (height * width)) / width;
                    int setCol = (setNumber % (height * width)) % width;

                    if (count[setImage].at<int>(setRow, setCol) >= 1500)
                        mymap[setNumber] = colorNumber++;
                    else
                        mymap[setNumber] = 0;
                }
            }
        }
    }
    
    int maxColor = (1 << 8) * (1 << 8) * (1 << 8);

    for (int view = 0; view < parent.size() - 1; view++)
    {
        output[view] = Mat(height, width, CV_8UC3);

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int index = view * width * height + i * width + j;
                int setNumber = FindSet(index);

                Vec3b& pixel = output[view].at<Vec3b>(i, j);
                int value = int((mymap[setNumber] * 1.f / mymap.size()) * maxColor);
                pixel[0] = value & 0xFF;
                pixel[1] = (value >> 8) & 0xFF;
                pixel[2] = value >> 16;
            }
        }
    }

    return output;
}