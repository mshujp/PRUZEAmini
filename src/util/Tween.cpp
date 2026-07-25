#include "../PRUZEAmini.h"
#include <algorithm>

using namespace PRUZEAmini;

float Tween::lerp(float from, float to, float t)
{
	t = std::max(0.0f,std::min(t,1.0f));
    return from + (to - from) * t;
}

float Tween::apply(float t, Ease ease)
{
    t = std::max(0.0f,std::min(t,1.0f));

    switch (ease)
    {
        case LINEAR:      return t;
        case EASE_IN:     return t * t;
        case EASE_OUT:    return t * (2.0f - t);
        case EASE_IN_OUT: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case EASE_OUT_BACK:
        {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            float tminus1 = t - 1.0f;
            const float t2 = tminus1 * tminus1;
            return 1.0f + c3 * t2 * tminus1 + c1 * t2;
        }
        case EASE_OUT_BOUNCE:
        {
            const float n1 = 7.5625f;
            const float d1 = 2.75f;

            if (t < 1.0f / d1)
            {
                return n1 * t * t;
            }
            else if (t < 2.0f / d1)
            {
                t -= 1.5f / d1;
                return n1 * t * t + 0.75f;
            }
            else if (t < 2.5f / d1)
            {
                t -= 2.25f / d1;
                return n1 * t * t + 0.9375f;
            } else {
                t -= 2.625f / d1;
                return n1 * t * t + 0.984375f;
            }
        }
    }
    return t;
}

float Tween::value(float from, float to, float t, Ease ease)
{
    float easedT = apply(t, ease);
    return from + (to - from) * easedT;
}
