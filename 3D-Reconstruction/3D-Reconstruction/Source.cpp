#include <iostream>
#include <ctime>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

//int width = 960;
//int height = 540;
double baseline = 1.0;
double focalLength = baseline / 2.0;

class UnionSet
{
private:
    vector<Mat> parent;
    vector<Mat> rank;
    vector<Mat> count;

    int width;
    int height;

    int& getRank(int index)
    {
        int image = index / (height * width);
        int row = (index % (height * width)) / width;
        int col = (index % (height * width)) % width;

        return rank[image].at<int>(row, col);
    }

    void setParent(int index, int setNumber)
    {
        int image = index / (height * width);
        int row = (index % (height * width)) / width;
        int col = (index % (height * width)) % width;

        int setImage = setNumber / (height * width);
        int setRow = (setNumber % (height * width)) / width;
        int setCol = (setNumber % (height * width)) % width;

        parent[image].at<int>(row, col) = setNumber;
        count[setImage].at<int>(setRow, setCol) += count[image].at<int>(row, col);
    }

public:
    UnionSet(int images, int width, int height)
    {
        for (int i = 0; i < images; i++)
        {
            parent.push_back(Mat(height, width, CV_32SC1));
            count.push_back(Mat(height, width, CV_32SC1, Scalar(1)));
            rank.push_back(Mat(height, width, CV_32SC1, Scalar(0)));
        }

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

    int findSet(int index)
    {
        int image = index / (height * width);
        int row = (index % (height * width)) / width;
        int col = (index % (height * width)) % width;

        int& value = parent[image].at<int>(row, col);

        return (value == index) ? index : (value = findSet(value));
    }

    int findSet(int image, int row, int col)
    {
        return findSet(image * (height * width) + row * width + col);
    }

    bool isSameSet(int index1, int index2)
    {
        return findSet(index1) == findSet(index2);
    }

    void unionSet(int index1, int index2)
    {
        if (!isSameSet(index1, index2))
        {
            int parent1 = findSet(index1);
            int parent2 = findSet(index2);

            int& rank1 = getRank(parent1);
            int& rank2 = getRank(parent2);

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
                rank2++;
                setParent(parent2, parent1);
            }
        }
    }

    void visualize()
    {
        map<int, int> mymap;
        int colorNumber = 0;

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

                        if (count[setImage].at<int>(setRow, setCol) >= 100)
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
                    int value = (mymap[setNumber] * 1.f / mymap.size()) * maxColor;
                    pixel[0] = value & 0xFF;
                    pixel[1] = (value >> 8) & 0xFF;
                    pixel[2] = value >> 16;
                }
            }

            imshow(to_string(view), output);
            waitKey(0);
        }
    }
};

class ObjectExtractor
{
private:

    enum Direction
    {
        FRONT = 0,
        RIGHT = 1,
        BACK = 2,
        LEFT = 3
    };

    int width;
    int height;
    double baseline;
    double focalLength;
    const float INF = 1e10;
    const double THRESHOLD = 0.3;

    bool fourViews;
    vector<Mat> disparityVector;

    double locationDifference(Vec3f a, Vec3f b)
    {
        return sqrt((a[2] - b[2]) * (a[2] - b[2]));

        double result = 0;
        for (int i = 0; i < 3; i++)
        {
            result += (a.val[i] - b.val[i]) * (a.val[i] - b.val[i]);
        }

        return sqrt(result);
    }

    void convertPointsToGlobalOrigin(vector<Mat>& points, Direction direction)
    {
        // translate to the global origin
        points[0] -= baseline / 2;
        points[2] -= baseline / 2;

        Mat x = points[0];
        Mat z = points[2];

        switch (direction)
        {
        case FRONT:
            break;

        case BACK:
            // rotate twice
            points[0] *= -1;
            points[2] *= -1;
            break;

        case LEFT:
            // rotate clockwise to look to the front side
            swap(points[0], points[2]);
            points[0] *= -1;
            break;

        case RIGHT:
            // rotate anti-clockwise to look to the front side
            swap(points[0], points[2]);
            points[2] *= -1;
            break;
        }
    }

    Mat convertDisparityToXYZ(Mat disparity, Direction currentDirection)
    {
        int height = disparity.rows;
        int width = disparity.cols;

        Mat X(height, width, CV_32F);
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

                X.at<float>(i, j) = (j * 1.f / width - 0.5) * Z.at<float>(i, j) / focalLength;
                Y.at<float>(i, j) = (0.5 - i * 1.f / height) * Z.at<float>(i, j) / focalLength;
            }
        }

        vector<Mat> coordinates;
        coordinates.push_back(X);
        coordinates.push_back(Y);
        coordinates.push_back(Z);

        convertPointsToGlobalOrigin(coordinates, currentDirection);

        Mat cloud;
        merge(coordinates, cloud);

        return cloud;
    }

    Direction getRightDirection(Direction currentDirection)
    {
        return Direction(((int)currentDirection + 1) % 4);
    }

    Vec2f getRightCameraProjection(Vec3f point, Direction currentDirection)
    {
        Direction newDirection = getRightDirection(currentDirection);

        Vec3f rightCameraPoint = convertPointToLocalOrigin(point, newDirection);
        return projectPointTo2D(rightCameraPoint);
    }

    Vec3f convertPointToLocalOrigin(Vec3f point, Direction direction)
    {
        Vec3f result;
        result[1] = point[1];

        switch (direction)
        {
        case FRONT:
            break;

        case BACK:
            // rotate twice
            result[0] = -point[0];
            result[2] = -point[2];
            break;

        case LEFT:
            // rotate anti-clockwise to look to the left side
            result[0] = point[2];
            result[2] = -point[0];
            break;

        case RIGHT:
            // rotate clockwise to look to the right side
            result[0] = -point[2];
            result[2] = point[0];
            break;
        }


        // translate to the local origin
        // The local origin is always behind the global origin on the left after the rotation
        result[0] += 0.5;
        result[2] += 0.5;

        return result;
    }

    bool collidesWithTheRightCamera(Vec3f point, Direction currentDirection)
    {
        // convert the point to the camera's local origin
        point = convertPointToLocalOrigin(point, currentDirection);

        float x = point[0];
        float z = point[2];

        // check if the point is to the left of the left border z = x + 1
        if (z - x - 1 > 0)
            return false;

        // check if the point is to the right of the right border z = 1 - x
        if (z + x - 1 < 0)
            return false;

        return true;
    }

    Vec2i projectPointTo2D(Vec3f point)
    {
        int x = (point[0] * focalLength / point[2] + 0.5) * width;
        int y = (0.5 - point[1] * focalLength / point[2]) * height;

        x = max(0, x);
        x = min(width - 1, x);

        y = max(0, y);
        y = min(height - 1, y);

        Vec2i result;
        result[0] = x;
        result[1] = y;

        return result;
    }

    void segmentImage(vector<Mat>& pointCloud, UnionSet& unionSet)
    {
        int height = pointCloud[0].rows;
        int width = pointCloud[0].cols;

        for (int view = 0; view == 0 || (fourViews && view < 4); view++)
        {
            Direction currentDirection = (Direction)view;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    if (view == 2 && i == 350 && j == 950)
                    {
                        view = 2;
                    }

                    Vec3f point = pointCloud[view].at<Vec3f>(i, j);

                    if (j > 0)
                    {
                        Vec3f left = pointCloud[view].at<Vec3f>(i, j - 1);
                        if (locationDifference(point, left) < THRESHOLD)
                        {
                            int imageNumber = (int)currentDirection;
                            int index1 = imageNumber * width * height + i * width + j;
                            int index2 = imageNumber * width * height + i * width + j - 1;

                            unionSet.unionSet(index1, index2);
                        }
                    }

                    if (i > 0)
                    {
                        Vec3f top = pointCloud[view].at<Vec3f>(i - 1, j);

                        if (locationDifference(point, top) < THRESHOLD)
                        {
                            int imageNumber = (int)currentDirection;
                            int index1 = imageNumber * width * height + i * width + j;
                            int index2 = imageNumber * width * height + (i - 1) * width + j;

                            unionSet.unionSet(index1, index2);
                        }
                    }

                    if (fourViews && collidesWithTheRightCamera(point, currentDirection))
                    {
                        int rightImage = (int)getRightDirection(currentDirection);
                        Vec2i rightCameraProjection = getRightCameraProjection(point, currentDirection);

                        int dx[] = { -1, 0, 1, 0, 0 };
                        int dy[] = { 0, 1, 0, -1, 0 };

                        int imageNumber = (int)currentDirection;
                        int index1 = imageNumber * width * height + i * width + j;
                        int index2 = rightImage  *  width * height;

                        for (int k = 0; k < 5; k++)
                        {
                            int x = rightCameraProjection[0] + dx[k];
                            int y = rightCameraProjection[1] + dy[k];

                            if (x < 0 || y < 0 || x >= width || y >= height)
                            {
                                continue;
                            }

                            Vec3f rightCameraPoint = pointCloud[rightImage].at<Vec3f>(y, x);
                            if (locationDifference(point, rightCameraPoint) < THRESHOLD)
                            {
                                unionSet.unionSet(index1, index2 + y * width + x);
                            }
                        }
                    }
                }
            }
        }
    }

public:

    // The disparity vector has the format Front, Right, Back, Left
    ObjectExtractor(vector<Mat> disparity, double baseline, double focalLength)
    {
        this->baseline = baseline;
        this->focalLength = focalLength;
        disparityVector = disparity;
        fourViews = true;

        height = disparity[0].rows;
        width = disparity[0].cols;
    }

    ObjectExtractor(Mat disparity, double baseline, double focalLength)
    {
        this->baseline = baseline;
        this->focalLength = focalLength;
        disparityVector.push_back(disparity);
        fourViews = false;

        height = disparity.rows;
        width = disparity.cols;
    }

    UnionSet segment()
    {
        vector<Mat> pointCloud;

        for (int i = 0; i < 4; i++)
        {
            pointCloud.push_back(convertDisparityToXYZ(disparityVector[i], (Direction)i));
        }

        UnionSet unionSet(4, pointCloud[0].cols, pointCloud[0].rows);
        segmentImage(pointCloud, unionSet);

        return unionSet;
    }
};

void removeRoadTest(string dispPath)
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

            Y.at<float>(i, j) = (0.5 - i * 1.f / height) * Z.at<float>(i, j) / focalLength;
        }
    }

    /* Remove road automatically */
    Mat road;
    threshold(Y, road, -2.5, 255, THRESH_BINARY);

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

    vector<Mat> disparity;
    for (int i = 0; i < 4; i++)
    {
        Mat disp = imread(d[i], -1);
        Mat road = imread(r[i], -1);

        road = 255 - road;  //invert road
        bitwise_and(disp, road, disp);
        disp.convertTo(disp, CV_32F);

        disparity.push_back(disp);
    }

    ObjectExtractor oe(disparity, 1.0, 0.5);

    auto a = clock();
    UnionSet us = oe.segment();
    cout << clock() - a << endl;
    us.visualize();

    /*********** Visualize the results ***********/
    //map<int, int> mymap;
    //int colorNumber = 0;

    //for (int i = 0; i < height; i++)
    //{
    //    for (int j = 0; j < width; j++)
    //    {
    //        int index = i * width + j;
    //        int setNumber = unionSet.findSet(index);

    //        if (mymap.find(setNumber) == mymap.end())
    //        {
    //            mymap[setNumber] = colorNumber++;
    //        }
    //    }
    //}

    //Mat output(height, width, CV_8UC3);
    //int maxColor = 16777216;

    //for (int i = 0; i < height; i++)
    //{
    //    for (int j = 0; j < width; j++)
    //    {
    //        int index = i * width + j;
    //        int setNumber = unionSet.findSet(index);

    //        Vec3b& pixel = output.at<Vec3b>(i, j);
    //        int value = (mymap[setNumber] * 1.f / mymap.size()) * maxColor;
    //        pixel[0] = value & 0xFF;
    //        pixel[1] = (value >> 8) & 0xFF;
    //        pixel[2] = value >> 16;
    //    }
    //}

    ////cout << clock() - a << endl;

    //imshow("Display window", output);
    //waitKey(0);
}