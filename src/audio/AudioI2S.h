#pragma once

#include "AudioBase.h"

namespace PRUZEAmini {

class AudioI2S : public AudioBase
{
private:
    static constexpr uint16_t SAMPLE_RATE = 22050;

    int8_t bclkPin = -1;
    int8_t wsPin = -1;
    int8_t dataPin = -1;
    void* device = nullptr;
    float phase = 0;
    bool started = false;

    uint32_t sampleRate() const override { return SAMPLE_RATE; }
    bool toneSamples(int from, int to, uint32_t total, uint32_t& written, float startGain, float endGain) override;
    bool pcmSamples(const int16_t* samples, uint32_t sampleCount) override;

public:
    explicit AudioI2S(const AudioI2SConfig& config) : bclkPin(config.bclkPin), wsPin(config.wsPin), dataPin(config.dataPin) {}
    ~AudioI2S() override;

    uint8_t getVolumeSteps() const override { return 4; }
    bool begin() override;
    void end() override;
};

} // namespace PRUZEAmini
