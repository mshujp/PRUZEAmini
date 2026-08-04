#include "../PRUZEAmini.h"
#include <cmath>
#include <Arduino.h>

namespace PRUZEAmini
{
namespace Math
{

// Template implementation of clamp
template<typename T>
T clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Explicit instantiations for common types to avoid linking errors
template int clamp<int>(int value, int min, int max);
template long clamp<long>(long value, long min, long max);
template float clamp<float>(float value, float min, float max);
template double clamp<double>(double value, double min, double max);
template int8_t clamp<int8_t>(int8_t value, int8_t min, int8_t max);
template uint8_t clamp<uint8_t>(uint8_t value, uint8_t min, uint8_t max);
template int16_t clamp<int16_t>(int16_t value, int16_t min, int16_t max);
template uint16_t clamp<uint16_t>(uint16_t value, uint16_t min, uint16_t max);
// template int32_t clamp<int32_t>(int32_t value, int32_t min, int32_t max);
template uint32_t clamp<uint32_t>(uint32_t value, uint32_t min, uint32_t max);
template int64_t clamp<int64_t>(int64_t value, int64_t min, int64_t max);
template uint64_t clamp<uint64_t>(uint64_t value, uint64_t min, uint64_t max);

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float moveTowards(float current, float target, float maxDelta)
{
    if (fabsf(target - current) <= maxDelta)
    {
        return target;
    }
    return current + ((target > current) ? maxDelta : -maxDelta);
}

float moveTowardsAngle(float current, float target, float maxDelta)
{
    float delta = target - current;
    // Wrap to -PI to PI
    delta = wrap(delta, -3.14159265f, 3.14159265f);
    
    if (fabsf(delta) <= maxDelta)
    {
        return target;
    }
    
    float result = current + ((delta > 0.0f) ? maxDelta : -maxDelta);
    return wrap(result, -3.14159265f, 3.14159265f);
}

float length(float x, float y)
{
    return sqrtf(x * x + y * y);
}

float lengthSquared(float x, float y)
{
    return x * x + y * y;
}

float distance(float ax, float ay, float bx, float by)
{
    return length(bx - ax, by - ay);
}

float distanceSquared(float ax, float ay, float bx, float by)
{
    return lengthSquared(bx - ax, by - ay);
}

void normalize(float& x, float& y)
{
    float len = length(x, y);
    if (len > 0.0f)
    {
        x /= len;
        y /= len;
    }
}

float dot(float ax, float ay, float bx, float by)
{
    return ax * bx + ay * by;
}

float wrap(float value, float min, float max)
{
    float range = max - min;
    if (range <= 0.0f) return min;
    
    float val = fmodf(value - min, range);
    if (val < 0.0f)
    {
        val += range;
    }
    return val + min;
}

float sin(float radians)
{
    return sinf(radians);
}

float cos(float radians)
{
    return cosf(radians);
}

void rotate(float x, float y, float radians, float& outX, float& outY)
{
    float s = sinf(radians);
    float c = cosf(radians);
    outX = x * c - y * s;
    outY = x * s + y * c;
}

float angle(float x, float y)
{
    return atan2f(y, x);
}

float degToRad(float degrees)
{
    return degrees * (Math::Pi / 180.0f);
}

float radToDeg(float radians)
{
    return radians * (180.0f / Math::Pi);
}

int random(int max)
{
    if (max <= 0) return 0;
    return static_cast<int>(::random(max));
}

int random(int min, int max) {
    if (min >= max) return min;
    return static_cast<int>(::random(min, max));
}

float randomFloat() {
    return static_cast<float>(::random(0x7FFFFFFF)) / 2147483648.0f;
}

float randomFloat(float max) {
    if (max <= 0.0f) return 0.0f;
    return randomFloat() * max;
}

float randomFloat(float min, float max) {
    if (min >= max) return min;
    return min + randomFloat() * (max - min);
}

} // namespace Math
} // namespace PRUZEAmini
