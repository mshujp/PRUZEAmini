#include "AudioI2S.h"

#include <Arduino.h>
#include <algorithm>

#if defined(ARDUINO_ARCH_RP2040)
#include <I2S.h>
#elif defined(ARDUINO_ARCH_ESP32)
#if __has_include(<ESP_I2S.h>)
#include <ESP_I2S.h>
#define PRUZEA_MINI_ESP_I2S_CLASS 1
#else
#include <driver/i2s.h>
#define PRUZEA_MINI_ESP_I2S_LEGACY 1
#endif
#endif

using namespace PRUZEAmini;

AudioI2S::~AudioI2S()
{
    end();
}

bool AudioI2S::begin()
{
    if (started) return true;
    if (bclkPin < 0 || dataPin < 0) return false;

#if defined(ARDUINO_ARCH_RP2040)
    I2S* i2s = new I2S(OUTPUT);
    if (i2s == nullptr) return false;

    i2s->setBCLK(bclkPin);
    i2s->setDATA(dataPin);
    i2s->setBitsPerSample(16);
    i2s->setBuffers(3, 128);
    if (!i2s->begin(SAMPLE_RATE))
    {
        delete i2s;
        return false;
    }
    device = i2s;
#elif defined(ARDUINO_ARCH_ESP32) && defined(PRUZEA_MINI_ESP_I2S_CLASS)
    I2SClass* i2s = new I2SClass();
    if (i2s == nullptr) return false;

    i2s->setPins(bclkPin, wsPin, dataPin, -1, -1);
    if (!i2s->begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO))
    {
        delete i2s;
        return false;
    }
    device = i2s;
#elif defined(ARDUINO_ARCH_ESP32)
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = SAMPLE_RATE;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.dma_buf_count = 3;
    config.dma_buf_len = 128;
    config.tx_desc_auto_clear = true;

    if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) return false;

    i2s_pin_config_t pinConfig = {};
    pinConfig.bck_io_num = bclkPin;
    pinConfig.ws_io_num = wsPin;
    pinConfig.data_out_num = dataPin;
    pinConfig.data_in_num = I2S_PIN_NO_CHANGE;
    if (i2s_set_pin(I2S_NUM_0, &pinConfig) != ESP_OK)
    {
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }
#else
    return false;
#endif

    phase = 0;
    started = true;
    return true;
}

void AudioI2S::end()
{
    if (!started) return;

#if defined(ARDUINO_ARCH_RP2040)
    static_cast<I2S*>(device)->end();
    delete static_cast<I2S*>(device);
#elif defined(ARDUINO_ARCH_ESP32) && defined(PRUZEA_MINI_ESP_I2S_CLASS)
    static_cast<I2SClass*>(device)->end();
    delete static_cast<I2SClass*>(device);
#elif defined(ARDUINO_ARCH_ESP32)
    i2s_driver_uninstall(I2S_NUM_0);
#endif

    device = nullptr;
    started = false;
}

bool AudioI2S::toneSamples(int from, int to, uint32_t total, uint32_t& written, float startGain, float endGain)
{
    if (!started || total == 0) return true;

    int16_t buffer[256];
    while (written < total)
    {
        const uint32_t frames = std::min<uint32_t>(128, total - written);
        for (uint32_t i = 0; i < frames; ++i)
        {
            const float progress = float(written + i) / float(total);
            const float frequency = from + (to - from) * progress;
            const float gain = startGain + (endGain - startGain) * progress;
            int amplitude = 0;

            switch (getVolumeLevel())
            {
            case 1:
                amplitude = 600;
                break;
            case 2:
                amplitude = 1500;
                break;
            case 3:
                amplitude = 3000;
                break;
            default:
                break;
            }

            const int16_t sample = (frequency > 0 && amplitude > 0)
                ? (phase < 0.5f ? -amplitude : amplitude)
                : 0;
            phase += frequency / SAMPLE_RATE;
            while (phase >= 1) phase -= 1;

            buffer[i * 2] = sample * gain;
            buffer[i * 2 + 1] = sample * gain;
        }

#if defined(ARDUINO_ARCH_RP2040)
        I2S* i2s = static_cast<I2S*>(device);
        for (uint32_t i = 0; i < frames; ++i)
        {
            i2s->write16(buffer[i * 2], buffer[i * 2 + 1]);
        }
#elif defined(ARDUINO_ARCH_ESP32) && defined(PRUZEA_MINI_ESP_I2S_CLASS)
        static_cast<I2SClass*>(device)->write(reinterpret_cast<uint8_t*>(buffer), frames * 4);
#elif defined(ARDUINO_ARCH_ESP32)
        size_t bytesWritten = 0;
        i2s_write(I2S_NUM_0, buffer, frames * 4, &bytesWritten, portMAX_DELAY);
#endif

        written += frames;
        if (hasPendingSE()) return false;
    }
    return true;
}
