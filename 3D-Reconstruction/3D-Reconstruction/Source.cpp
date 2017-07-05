#include <iostream>
#include <ctime>

#include "ObjectExtractor.h"

using namespace cv;
using namespace std;

void removeRoadTest(string dispPath, float baseline, float focalLength)
{
    Mat disparity = imread(dispPath, -1);
    int height = disparity.rows;
    int width = disparity.cols;

    disparity.convertTo(disparity, CV_32F);

    Mat Y(height, width, CV_32F);
    Mat Z = width * baseline / (2 * disparity);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (disparity.at<float>(i, j) < 10)
            {
                Z.at<float>(i, j) = 0;
            }

            Y.at<float>(i, j) = (0.5f - i * 1.f / height) * Z.at<float>(i, j) / focalLength;
        }
    }

    /* Remove road automatically */
    Mat road;
    threshold(Y, road, -2.5f, 255, THRESH_BINARY);

    road.convertTo(road, CV_8U);
    disparity.convertTo(disparity, CV_8U);

    bitwise_and(disparity, road, disparity);

    imshow("bla", disparity);
    waitKey(0);
}

int main()
{
    string d[] = { "00008_front.png", "00008_right.png", "00008_back.png", "00008_left.png" };
    string r[] = { "00008_front_road.png", "00008_right_road.png", "00008_back_road.png", "00008_left_road.png" };

    vector<Mat> disparityVector;
    vector<Mat> roadVector;

    for (int i = 0; i < 4; i++)
    {
        Mat disp = imread(d[i], -1);
        Mat road = imread(r[i], -1);

        disp.convertTo(disp, CV_32F);

        if (i % 2 == 0)
            roadVector.push_back(road);

        disparityVector.push_back(disp);
    }

    //return 0;

    float baseline = 1.0f;
    float focalLength = baseline / 2.0f;
    Vec3f cameraLocation(-focalLength, 0, -focalLength);
    ObjectExtractor oe(960, 540, 90.0, baseline, focalLength, cameraLocation);

    bool visualize = true;

    if (!visualize)
    {
        auto a = clock();
        oe.getObjects(disparityVector, roadVector, visualize);
        cout << clock() - a << endl;
    }
    else
    {
        oe.getObjects(disparityVector, roadVector, visualize);
    }
}
