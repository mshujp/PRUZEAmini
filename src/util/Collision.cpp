#include <cmath>
#include "../PRUZEAmini.h"

using namespace PRUZEAmini;

bool Collision::pointRect(float px, float py, float rx, float ry, float rw, float rh)
{
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

bool Collision::rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return (ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by);
}

bool Collision::circleCircle(float ax, float ay, float ar, float bx, float by, float br)
{
    float dx = ax - bx;
    float dy = ay - by;
    float distanceSq = (dx * dx) + (dy * dy);
    float radiusSum = ar + br;
    
    return distanceSq <= (radiusSum * radiusSum);
}

bool Collision::circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh)
{
    float closestX = cx;
    if (closestX < rx)         closestX = rx;
    else if (closestX > rx + rw) closestX = rx + rw;

    float closestY = cy;
    if (closestY < ry)         closestY = ry;
    else if (closestY > ry + rh) closestY = ry + rh;

    float dx = cx - closestX;
    float dy = cy - closestY;
    float distanceSq = (dx * dx) + (dy * dy);

    return distanceSq <= (radius * radius);
}
