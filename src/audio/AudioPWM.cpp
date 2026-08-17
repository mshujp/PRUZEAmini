#include "AudioPWM.h"

#include <Arduino.h>
#include <algorithm>

#if defined(ARDUINO_ARCH_RP2040)
#include <PWMAudio.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <esp_arduino_version.h>
#include <esp_timer.h>
#endif

using namespace PRUZEAmini;

namespace
{
#if defined(ARDUINO_ARCH_ESP32)
constexpr uint8_t PWM_CHANNEL = 7;
constexpr uint32_t PWM_CARRIER_FREQUENCY = 125000;

bool beginEspPwm(int8_t pin)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    return ledcAttach(pin, PWM_CARRIER_FREQUENCY, 8);
#else
    if (ledcSetup(PWM_CHANNEL, PWM_CARRIER_FREQUENCY, 8) == 0) return false;
    ledcAttachPin(pin, PWM_CHANNEL);
    return true;
#endif
}

void writeEspPwm(int8_t pin, uint32_t duty)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin, duty);
#else
    ledcWrite(PWM_CHANNEL, duty);
#endif
}

void endEspPwm(int8_t pin)
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcDetach(pin);
#else
    ledcDetachPin(pin);
#endif
}
#endif
}

bool AudioPWM::begin()
{
    if (started) return true;
    if (pin < 0) return false;

#if defined(ARDUINO_ARCH_RP2040)
    if (mode == AudioPWMMode::DAC)
    {
        auto* pwmAudio = new PWMAudio(static_cast<pin_size_t>(pin), false);
        if (pwmAudio == nullptr) return false;
        pwmAudio->setBuffers(3, 128);
        if (!pwmAudio->begin(SAMPLE_RATE))
        {
            delete pwmAudio;
            return false;
        }
        device = pwmAudio;
    }
#elif defined(ARDUINO_ARCH_ESP32)
    if (mode == AudioPWMMode::DAC)
    {
        if (!beginEspPwm(pin)) return false;
        writeEspPwm(pin, 128);
    }
#else
    return false;
#endif

    if (mode == AudioPWMMode::BUZZER)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    started = true;
    return true;
}

void AudioPWM::end()
{
    if (!started) return;

#if defined(ARDUINO_ARCH_RP2040)
    if (mode == AudioPWMMode::DAC && device != nullptr)
    {
        static_cast<PWMAudio*>(device)->end();
        delete static_cast<PWMAudio*>(device);
        device = nullptr;
    }
#elif defined(ARDUINO_ARCH_ESP32)
    if (mode == AudioPWMMode::DAC)
    {
        writeEspPwm(pin, 128);
        endEspPwm(pin);
    }
#endif

    if (mode == AudioPWMMode::BUZZER) noTone(pin);
    pinMode(pin, OUTPUT);
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

bool AudioPWM::pcmSamples(const int16_t* samples, uint32_t sampleCount)
{
    return mode == AudioPWMMode::BUZZER
        ? pcmSamplesBuzzer(samples, sampleCount)
        : pcmSamplesDAC(samples, sampleCount);
}

bool AudioPWM::pcmSamplesBuzzer(const int16_t* samples, uint32_t sampleCount)
{
    if (!started || samples == nullptr || sampleCount == 0) return true;

    const bool audible = getVolumeLevel() > 0;
    uint32_t deadlineUsec = micros();
    uint32_t timingRemainder = 0;

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        digitalWrite(pin, audible && samples[i] > 0 ? HIGH : LOW);

        timingRemainder += 1000000u;
        deadlineUsec += timingRemainder / SAMPLE_RATE;
        timingRemainder %= SAMPLE_RATE;
        while (static_cast<int32_t>(micros() - deadlineUsec) < 0)
        {
        }
    }

    digitalWrite(pin, LOW);
    return true;
}

bool AudioPWM::pcmSamplesDAC(const int16_t* samples, uint32_t sampleCount)
{
    if (!started || samples == nullptr || sampleCount == 0) return true;

#if defined(ARDUINO_ARCH_RP2040)
    auto* pwmAudio = static_cast<PWMAudio*>(device);
    const bool audible = getVolumeLevel() > 0;
    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        const int16_t sample = audible
            ? samples[i]
            : 0;
        if (pwmAudio->write(sample, true) == 0) return false;
    }
    return true;
#elif defined(ARDUINO_ARCH_ESP32)
    const bool audible = getVolumeLevel() > 0;
    int64_t deadlineUsec = esp_timer_get_time();
    uint32_t timingRemainder = 0;

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        const int32_t scaled = audible
            ? static_cast<int32_t>(samples[i])
            : 0;
        const int32_t duty = 128 + (scaled >> 8);
        writeEspPwm(pin, static_cast<uint32_t>(std::clamp<int32_t>(duty, 0, 255)));

        timingRemainder += 1000000u;
        deadlineUsec += timingRemainder / SAMPLE_RATE;
        timingRemainder %= SAMPLE_RATE;
        while (esp_timer_get_time() < deadlineUsec)
        {
        }
    }

    writeEspPwm(pin, 128);
    return true;
#else
    return false;
#endif
}
