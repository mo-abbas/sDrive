#pragma once
#include <opencv2/core/core.hpp>
using namespace cv;

class Box
{
    const float WIDTH = 2.0f;
    const float LENGTH = 4.64f;

    int Sign(const float x) const;
    void AdjustPoints(float& maxPoint, float& minPoint, float delta);

public:
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;

    Box();

    void Adjust();
    void Update(Vec3f point);

    Vec2f Get2DCenter();

    Box& operator=(const Box& box);
};