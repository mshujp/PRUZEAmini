#include "AudioStub.h"

using namespace PRUZEAmini;

bool AudioStub::begin()
{
    return true;
}

void AudioStub::end()
{
}

uint32_t AudioStub::sampleRate() const
{
    return SAMPLE_RATE;
}

bool AudioStub::toneSamples(int startFrequency, int endFrequency, uint32_t totalSamples, uint32_t& writtenSamples, float startVolumeScale, float endVolumeScale)
{
    (void)startFrequency;
    (void)endFrequency;
    (void)startVolumeScale;
    (void)endVolumeScale;
    writtenSamples = totalSamples;
    return true;
}

bool AudioStub::pcmSamples(const int16_t* samples, uint32_t sampleCount)
{
    (void)samples;
    (void)sampleCount;
    return true;
}
