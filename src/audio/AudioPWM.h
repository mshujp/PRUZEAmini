#pragma once

#include "AudioBase.h"

namespace PRUZEAmini {

class AudioPWM : public AudioBase
{
private:
    static constexpr uint16_t SAMPLE_RATE = 22050;

    int8_t pin = -1;
    AudioPWMMode mode = AudioPWMMode::BUZZER;
    void* device = nullptr;
    bool started = false;

    uint32_t sampleRate() const override { return SAMPLE_RATE; }
    bool toneSamples(int from, int to, uint32_t total, uint32_t& written, float startGain, float endGain) override;
    bool pcmSamples(const int16_t* samples, uint32_t sampleCount) override;
    bool pcmSamplesBuzzer(const int16_t* samples, uint32_t sampleCount);
    bool pcmSamplesDAC(const int16_t* samples, uint32_t sampleCount);

public:
    explicit AudioPWM(const AudioPWMConfig& config) : pin(config.pwmPin), mode(config.mode) {}

    uint8_t getVolumeSteps() const override { return 2; }
    bool begin() override;
    void end() override;
};

} // namespace PRUZEAmini
