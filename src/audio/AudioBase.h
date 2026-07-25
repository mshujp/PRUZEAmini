#pragma once

#include "PRUZEAmini.h"
#include "../util/SpinLock.h"

#include <atomic>

namespace PRUZEAmini {

class AudioBase : public Audio
{
private:
    const Sound* triggerSE = nullptr;
    float triggerSEGain = 1.0f;

    std::atomic<bool> triggerSEPending{false};

    const Music* music = nullptr;
    uint16_t musicNoteIndex = 0;
    uint8_t musicRepeatIndex = 0;
    uint32_t musicNoteWrittenSamples = 0;

    std::atomic<bool> musicActive{false};
    std::atomic<int8_t> volumeLevel{1};
    std::atomic<int8_t> volumeBeforeMute{1};

    mutable SpinLock stateLock;

    const Sound* takeTriggerSE(float& gain);
    bool playSound(const Sound* sound, float gain);
    bool playMusicStep();

protected:
    virtual uint32_t sampleRate() const = 0;
    virtual bool toneSamples(int startFrequency, int endFrequency, uint32_t totalSamples, uint32_t& writtenSamples, float startVolumeScale, float endVolumeScale) = 0;
    bool hasPendingSE() const;

public:
    AudioBase() = default;
    virtual ~AudioBase() = default;

    AudioBase(const AudioBase&) = delete;
    AudioBase& operator=(const AudioBase&) = delete;

    virtual bool begin() = 0;
    virtual void end() = 0;

    void playSE(const Sound* sound, float gain) override;
    void playMusic(const Music* music) override;
    void stopMusic() override;

    virtual uint8_t getVolumeSteps() const = 0; // 1: On Only or Off Only  2: Mute + Max volume   >2: Mute and multiple volume. (e.g. 0 to 3)
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
