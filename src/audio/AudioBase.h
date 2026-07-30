#pragma once

#include "PRUZEAmini.h"
#include "MidiPlayer.h"
#include "../util/SpinLock.h"

#include <atomic>

namespace PRUZEAmini {

class AudioBase : public Audio
{
private:
    static constexpr uint16_t AUDIO_BUFFER_SAMPLES = 128;
    static constexpr int32_t TONE_AMPLITUDE = 10000;
    static constexpr float SE_MIX_GAIN = 0.70f;

    const Sound* triggerSE = nullptr;
    float triggerSEGain = 1.0f;
    std::atomic<bool> triggerSEPending{false};

    const Sound* activeSE = nullptr;
    float activeSEGain = 1.0f;
    uint16_t seStepIndex = 0;
    uint32_t seStepWrittenSamples = 0;
    uint32_t sePhase = 0;

    const Music* music = nullptr;
    uint16_t musicNoteIndex = 0;
    uint8_t musicRepeatIndex = 0;
    uint32_t musicNoteWrittenSamples = 0;
    uint32_t musicPhase = 0;

    MidiPlayer midiPlayer;

    int16_t bgmBuffer[AUDIO_BUFFER_SAMPLES] = {};
    int16_t seBuffer[AUDIO_BUFFER_SAMPLES] = {};
    int16_t mixBuffer[AUDIO_BUFFER_SAMPLES] = {};

    std::atomic<bool> musicActive{false};
    std::atomic<bool> midiActive{false};
    std::atomic<int8_t> volumeLevel{1};
    std::atomic<int8_t> volumeBeforeMute{1};

    mutable SpinLock stateLock;

    const Sound* takeTriggerSE(float& gain);
    void startPendingSE();
    void resetActiveSE();

    void renderMusicSamples(int16_t* output, uint32_t sampleCount);
    void renderMidiSamples(int16_t* output, uint32_t sampleCount);
    void renderSESamples(int16_t* output, uint32_t sampleCount);
    void mixSamples(int16_t* output, const int16_t* bgm, const int16_t* se, uint32_t sampleCount, bool hasBgm, bool hasSE);

    static int16_t renderTriangle(uint32_t phase, float volume);
    static uint32_t phaseStepForFrequency(uint32_t frequency, uint32_t sampleRate);
    static int16_t clampSample(int32_t sample);

protected:
    virtual uint32_t sampleRate() const = 0;

    // Kept for compatibility with existing audio backends.
    // AudioBase now uses pcmSamples() for SE, ToneNote music, and MIDI so they can be mixed.
    virtual bool toneSamples(int startFrequency, int endFrequency, uint32_t totalSamples, uint32_t& writtenSamples, float startVolumeScale, float endVolumeScale) = 0;
    virtual bool pcmSamples(const int16_t* samples, uint32_t sampleCount) = 0;

    bool hasPendingSE() const;

public:
    AudioBase() = default;
    virtual ~AudioBase() = default;

    AudioBase(const AudioBase&) = delete;
    AudioBase& operator=(const AudioBase&) = delete;

    virtual bool runsAsAudioWorker() const { return true; }
    virtual bool begin() = 0;
    virtual void end() = 0;

    void playSE(const Sound* sound, float gain) override;
    void stopSE();
    void playMusic(const Music* music) override;
    void playMidi(const Midi* midi) override;
    void stopMusic() override;

    virtual uint8_t getVolumeSteps() const = 0;
    int8_t getVolumeLevel() const;
    void setVolumeLevel(int8_t level);
    void upVolume();
    void downVolume();
    void setMute(bool mute);
    bool isMuted() const;
    void toggleMute();

    void update();
};

} // namespace PRUZEAmini
