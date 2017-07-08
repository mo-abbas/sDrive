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
    vector<Road> roadVector;

    bool single = false;

    if (single)
    {
        Mat disp = imread(d[0], -1);
        Mat road = imread(r[0], -1);

        disp.convertTo(disp, CV_32F);

        roadVector.push_back(Road(road, FRONT));
        disparityVector.push_back(disp);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            Mat disp = imread(d[i], -1);
            Mat road = imread(r[i], -1);

            disp.convertTo(disp, CV_32F);

            disparityVector.push_back(disp);
            roadVector.push_back(Road(road, (Direction)i));
        }
    }

    // kitti pixel size is 4.65 * 10^-6
    // kitti focal length is 4 * 10-4
    // kitti FOV is 90 degrees
    // kitti baseline is 0.54m
    // kitti resolution is typically 1248x384

    float fovx = 90.f;
    float baseline = 1.f;
    float pixelSize = 1.f;
    float focalLength = baseline / 2.0f;
    Vec3f cameraLocation(-focalLength, 0, -focalLength);

    PointCloud pointCloud(disparityVector, fovx, baseline, focalLength, pixelSize, cameraLocation);
    pointCloud.ClipExtraPoints(roadVector);

    ObjectExtractor oe(pointCloud);
    vector<Mat> boxes = oe.VisualizeBoxes();
    vector<Mat> segments = oe.VisualizeSegmentation();

    for (int i = 0; i < boxes.size(); i++)
    {
        imshow("boxes " + to_string(i), boxes[i]);
    }

    for (int i = 0; i < boxes.size(); i++)
    {
        imshow("segments " + to_string(i), segments[i]);
    }

    waitKey(0);
}
