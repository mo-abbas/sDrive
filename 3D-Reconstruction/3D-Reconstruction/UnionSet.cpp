#include "UnionSet.h"
#include <iostream>
#include <ctime>

int UnionSet::getRank(int index)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    return rank[image].at<int>(row, col);
}

void UnionSet::setParent(int index, int setNumber)
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

void UnionSet::setRank(int index, int rank)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    this->rank[image].at<int>(row, col) = rank;
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

int UnionSet::findSet(int index)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    int& value = parent[image].at<int>(row, col);

    return (value == index) ? index : (value = findSet(value));
}

int UnionSet::findSet(int image, int row, int col)
{
    return findSet(image * (height * width) + row * width + col);
}

bool UnionSet::isSameSet(int index1, int index2)
{
    return findSet(index1) == findSet(index2);
}

void UnionSet::unionSet(int index1, int index2)
{
    if (!isSameSet(index1, index2))
    {
        int parent1 = findSet(index1);
        int parent2 = findSet(index2);

        int rank1 = getRank(parent1);
        int rank2 = getRank(parent2);

        if (rank1 > rank2)
        {
            setParent(parent2, parent1);
        }
        else if (rank2 > rank1)
        {
            setParent(parent1, parent2);
        }
        else
        {
            setRank(parent2, rank2 + 1);
            setParent(parent1, parent2);
        }
    }
}

void UnionSet::visualize()
{
    map<int, int> mymap;
    int colorNumber = 0;

    auto a = clock();

    for (int view = 0; view < 4; view++)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int index = view * width * height + i * width + j;
                int setNumber = findSet(index);

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
    
    int maxColor = 16777216;

    for (int view = 0; view < 4; view++)
    {
        Mat output(height, width, CV_8UC3);

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                int index = view * width * height + i * width + j;
                int setNumber = findSet(index);

                Vec3b& pixel = output.at<Vec3b>(i, j);
                int value = int((mymap[setNumber] * 1.f / mymap.size()) * maxColor);
                pixel[0] = value & 0xFF;
                pixel[1] = (value >> 8) & 0xFF;
                pixel[2] = value >> 16;
            }
        }

        imshow(to_string(view), output);
        waitKey(0);
    }
}

void UnionSet::setBackground(int index)
{
    this->unionSet(index, backgroundSet);
}

int UnionSet::getCount(int index)
{
    int image = index / (height * width);
    int row = (index % (height * width)) / width;
    int col = (index % (height * width)) % width;

    return count[image].at<int>(row, col);
}