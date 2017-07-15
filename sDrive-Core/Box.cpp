#include "Box.h"

int Box::Sign(const float x) const
{
    return (x > 0) - (x < 0);
}

void Box::AdjustPoints(float& maxPoint, float& minPoint, float delta)
{
    if (Sign(minPoint) != Sign(maxPoint))
    {
        float difference = maxPoint - minPoint;

        minPoint += minPoint / difference * delta;
        maxPoint += maxPoint / difference * delta;
    }
    else if (Sign(minPoint) == -1)
    {
        minPoint -= delta;
    }
    else
    {
        maxPoint += delta;
    }
}

Box::Box()
{
    minX = 1e7;
    maxX = -1e7;
    minY = 1e7;
    maxY = -1e7;
    minZ = 1e7;
    maxZ = -1e7;
}

void Box::Adjust()
{
    float deltaX = 0;
    if (maxX - minX != WIDTH)
    {
        deltaX = WIDTH - (maxX - minX);
    }

    float deltaZ = 0;
    if (maxZ - minZ != LENGTH)
    {
        deltaZ = LENGTH - (maxZ - minZ);
    }

    AdjustPoints(maxX, minX, deltaX);
    AdjustPoints(maxZ, minZ, deltaZ);
}

void Box::Update(Vec3f point)
{
    if (point[0] > maxX)
        maxX = point[0];
    if (point[0] < minX)
        minX = point[0];

    if (point[1] > maxY)
        maxY = point[1];
    if (point[1] < minY)
        minY = point[1];

    if (point[2] > maxZ)
        maxZ = point[2];
    if (point[2] < minZ)
        minZ = point[2];
}

Vec2f Box::Get2DCenter()
{
    Vec2f point;
    point[0] = (maxX + minX) / 2;
    point[1] = (maxZ + minZ) / 2;

    return point;
}

Box& Box::operator=(const Box& box)
{
    this->minX = box.minX;
    this->maxX = box.maxX;
    this->minY = box.minY;
    this->maxY = box.maxY;
    this->minZ = box.minZ;
    this->maxZ = box.maxZ;

    return *this;
}