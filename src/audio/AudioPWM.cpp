#include "AudioPWM.h"

#include <Arduino.h>
#include <algorithm>

using namespace PRUZEAmini;

bool AudioPWM::begin()
{
    if (started) return true;
    if (pin < 0) return false;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    started = true;
    return true;
}

void AudioPWM::end()
{
    if (!started) return;

    noTone(pin);
    digitalWrite(pin, LOW);
    started = false;
}

bool AudioPWM::toneSamples(int from, int to, uint32_t total, uint32_t& written, float startGain, float endGain)
{
    if (!started || total == 0) return true;

    const uint32_t chunk = std::max<uint32_t>(1, SAMPLE_RATE / 100);
    while (written < total)
    {
        const uint32_t sampleCount = std::min(chunk, total - written);
        const float progress = float(written + sampleCount / 2) / float(total);
        const int frequency = int(from + (to - from) * progress);
        const float gain = startGain + (endGain - startGain) * progress;

        if (frequency > 0 && gain > 0 && !isMuted()) tone(pin, frequency);
        else noTone(pin);

        delayMicroseconds((sampleCount * 1000000u) / SAMPLE_RATE);
        written += sampleCount;

        if (hasPendingSE())
        {
            noTone(pin);
            return false;
        }
    }

    noTone(pin);
    digitalWrite(pin, LOW);
    return true;
}
