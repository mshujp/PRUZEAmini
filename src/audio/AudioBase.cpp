#include "AudioBase.h"

#include <algorithm>
#include <cstddef>

using namespace PRUZEAmini;

namespace {

using CriticalSectionLock = SpinLockGuard;
template<class T>T clampValue(T value,T low,T high){return value<low?low:(value>high?high:value);}

template<std::size_t N>
constexpr Audio::Sound makeSound(const Audio::SoundStep (&steps)[N])
{
    return { steps, static_cast<uint16_t>(N) };
}

static constexpr Audio::SoundStep SE_NO_1_STEPS[]  = { {1200,1200,30,1.0f,1.0f}, {1800,1800,20,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_2_STEPS[]  = { {300,300,40,1.0f,1.0f}, {150,150,30,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_3_STEPS[]  = { {800,800,60,1.0f,1.0f}, {1200,1200,60,1.0f,1.0f}, {1600,1600,100,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_4_STEPS[]  = { {1000,1000,50,1.0f,1.0f}, {1300,1300,50,1.0f,1.0f}, {1500,1500,50,1.0f,1.0f}, {2000,2000,150,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_5_STEPS[]  = { {600,600,30,1.0f,1.0f}, {400,400,30,1.0f,1.0f}, {600,600,30,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_6_STEPS[]  = { {1500,1500,40,1.0f,1.0f}, {1000,1000,40,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_7_STEPS[]  = { {2500,2500,20,1.0f,1.0f}, {0,0,10,0.0f,0.0f}, {2500,2500,20,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_8_STEPS[]  = { {400,400,50,1.0f,1.0f}, {600,600,50,1.0f,1.0f}, {800,800,50,1.0f,1.0f}, {1000,1000,50,1.0f,1.0f}, {1200,1200,50,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_9_STEPS[]  = { {2000,2000,20,1.0f,1.0f}, {1500,1500,20,1.0f,1.0f}, {2000,2000,20,1.0f,1.0f}, {1500,1500,20,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_10_STEPS[] = { {100,100,80,1.0f,1.0f}, {80,80,120,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_11_STEPS[] = { {500,500,20,1.0f,1.0f}, {1000,1000,20,1.0f,1.0f}, {1500,1500,20,1.0f,1.0f}, {2000,2000,20,1.0f,1.0f}, {2500,2500,40,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_12_STEPS[] = { {1800,1800,100,1.0f,1.0f}, {1400,1400,100,1.0f,1.0f}, {1100,1100,100,1.0f,1.0f}, {900,900,200,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_13_STEPS[] = { {200,150,80,1.0f,1.0f}, {0,0,50,0.0f,0.0f}, {200,150,120,1.0f,1.0f} };

} // namespace

const Audio::Sound Audio::SE::NO_1  = makeSound(SE_NO_1_STEPS);
const Audio::Sound Audio::SE::NO_2  = makeSound(SE_NO_2_STEPS);
const Audio::Sound Audio::SE::NO_3  = makeSound(SE_NO_3_STEPS);
const Audio::Sound Audio::SE::NO_4  = makeSound(SE_NO_4_STEPS);
const Audio::Sound Audio::SE::NO_5  = makeSound(SE_NO_5_STEPS);
const Audio::Sound Audio::SE::NO_6  = makeSound(SE_NO_6_STEPS);
const Audio::Sound Audio::SE::NO_7  = makeSound(SE_NO_7_STEPS);
const Audio::Sound Audio::SE::NO_8  = makeSound(SE_NO_8_STEPS);
const Audio::Sound Audio::SE::NO_9  = makeSound(SE_NO_9_STEPS);
const Audio::Sound Audio::SE::NO_10 = makeSound(SE_NO_10_STEPS);
const Audio::Sound Audio::SE::NO_11 = makeSound(SE_NO_11_STEPS);
const Audio::Sound Audio::SE::NO_12 = makeSound(SE_NO_12_STEPS);
const Audio::Sound Audio::SE::NO_13 = makeSound(SE_NO_13_STEPS);

void AudioBase::setVolumeLevel(int8_t level)
{
    const uint8_t volumeSteps = getVolumeSteps();
    const int8_t maxLevel = volumeSteps > 0 ? static_cast<int8_t>(volumeSteps - 1) : 0;
    const int8_t clampedLevel = clampValue(level, static_cast<int8_t>(0), maxLevel);

    volumeLevel.store(clampedLevel, std::memory_order_relaxed);
    if (clampedLevel > 0)
    {
        volumeBeforeMute.store(clampedLevel, std::memory_order_relaxed);
    }
}

void AudioBase::upVolume()
{
    setVolumeLevel(getVolumeLevel() + 1);
}

void AudioBase::downVolume()
{
    setVolumeLevel(getVolumeLevel() - 1);
}

int8_t AudioBase::getVolumeLevel() const
{
    return volumeLevel.load(std::memory_order_relaxed);
}

void AudioBase::setMute(bool mute)
{
    if (mute)
    {
        const int8_t previousLevel = volumeLevel.exchange(0, std::memory_order_relaxed);
        if (previousLevel > 0)
        {
            volumeBeforeMute.store(previousLevel, std::memory_order_relaxed);
        }
        return;
    }

    if (isMuted())
    {
        setVolumeLevel(volumeBeforeMute.load(std::memory_order_relaxed));
    }
}

bool AudioBase::isMuted() const
{
    return volumeLevel.load(std::memory_order_relaxed) == 0;
}

void AudioBase::toggleMute()
{
    setMute(!isMuted());
}

const Audio::Sound* AudioBase::takeTriggerSE(float& gain)
{
    CriticalSectionLock lock(stateLock);

    const Sound* sound = triggerSE;
    gain = triggerSEGain;

    triggerSE = nullptr;
    triggerSEGain = 1.0f;
    triggerSEPending.store(false, std::memory_order_release);

    return sound;
}

bool AudioBase::hasPendingSE() const
{
    return triggerSEPending.load(std::memory_order_acquire);
}

void AudioBase::playSE(const Sound* sound, float gain)
{
    if (sound == nullptr || sound->steps == nullptr || sound->stepCount == 0) return;

    CriticalSectionLock lock(stateLock);

    triggerSE = sound;
    triggerSEGain = clampValue(gain, 0.0f, 1.0f);
    triggerSEPending.store(true, std::memory_order_release);
}

void AudioBase::playMusic(const Music* newMusic)
{
    CriticalSectionLock lock(stateLock);

    music = newMusic;
    musicNoteIndex = 0;
    musicRepeatIndex = 0;
    musicNoteWrittenSamples = 0;

    const bool playable = newMusic != nullptr && newMusic->notes != nullptr && newMusic->noteCount > 0;

    musicActive.store(playable, std::memory_order_release);
}

void AudioBase::stopMusic()
{
    CriticalSectionLock lock(stateLock);

    music = nullptr;
    musicNoteIndex = 0;
    musicRepeatIndex = 0;
    musicNoteWrittenSamples = 0;
    musicActive.store(false, std::memory_order_release);
}

bool AudioBase::playSound(const Sound* sound, float gain)
{
    if (sound == nullptr || sound->steps == nullptr || sound->stepCount == 0) return true;

    const float clampedGain = clampValue(gain, 0.0f, 1.0f);

    for (uint16_t i = 0; i < sound->stepCount; ++i)
    {
        const SoundStep& step = sound->steps[i];
        if (step.durationMsec == 0) continue;

        uint32_t writtenSamples = 0;
        const uint32_t totalSamples =
            (sampleRate() * static_cast<uint32_t>(step.durationMsec)) / 1000u;

        if (totalSamples == 0) continue;

        const bool completed = toneSamples(
            step.startFrequency,
            step.endFrequency,
            totalSamples,
            writtenSamples,
            clampValue(step.startVolume, 0.0f, 1.0f) * clampedGain,
            clampValue(step.endVolume, 0.0f, 1.0f) * clampedGain
        );

        if (!completed) return false;
    }

    return true;
}

bool AudioBase::playMusicStep()
{
    const Music* localMusic = nullptr;
    uint8_t localRepeatIndex = 0;
    uint16_t localNoteIndex = 0;
    uint32_t localWrittenSamples = 0;
    float localMusicVolume = 0.5f;

    {
        CriticalSectionLock lock(stateLock);

        localMusic = music;
        localNoteIndex = musicNoteIndex;
        localRepeatIndex = musicRepeatIndex;
        localWrittenSamples = musicNoteWrittenSamples;
        localMusicVolume = localMusic == nullptr ? 0.0f : clampValue(localMusic->gain, 0.0f, 1.0f);
    }

    if (localMusic == nullptr || localMusic->notes == nullptr || localMusic->noteCount == 0)
    {
        musicActive.store(false, std::memory_order_release);
        return true;
    }

    if (localMusic->bpm == 0)
    {
        CriticalSectionLock lock(stateLock);

        if (music == localMusic)
        {
            music = nullptr;
            musicNoteIndex = 0;
            musicRepeatIndex = 0;
            musicNoteWrittenSamples = 0;
            musicActive.store(false, std::memory_order_release);
        }
        return true;
    }

    if (localNoteIndex >= localMusic->noteCount)
    {
        CriticalSectionLock lock(stateLock);

        if (music == localMusic && musicNoteIndex == localNoteIndex)
        {
            if (localMusic->playCount == 0 || localRepeatIndex + 1 < localMusic->playCount)
            {
                musicNoteIndex = 0;
                musicRepeatIndex = localMusic->playCount == 0 ? 0 : static_cast<uint8_t>(localRepeatIndex + 1);
                musicNoteWrittenSamples = 0;
            }
            else
            {
                music = nullptr;
                musicNoteIndex = 0;
                musicRepeatIndex = 0;
                musicNoteWrittenSamples = 0;
                musicActive.store(false, std::memory_order_release);
            }
        }
        return true;
    }

    const ToneNote& note = localMusic->notes[localNoteIndex];
    uint32_t durationUnits = static_cast<uint32_t>(note.duration);
    uint16_t nextNoteIndex = static_cast<uint16_t>(localNoteIndex + 1);

    while (note.frequency != ToneNote::REST
        && nextNoteIndex < localMusic->noteCount
        && localMusic->notes[nextNoteIndex - 1].tie
        && localMusic->notes[nextNoteIndex].frequency == note.frequency)
    {
        durationUnits += static_cast<uint32_t>(localMusic->notes[nextNoteIndex].duration);
        ++nextNoteIndex;
    }

    const uint64_t numerator = static_cast<uint64_t>(sampleRate()) * 60u * durationUnits;
    const uint32_t totalSamples = static_cast<uint32_t>(numerator / (static_cast<uint64_t>(localMusic->bpm) * static_cast<uint32_t>(ToneNote::Q)));

    if (totalSamples == 0)
    {
        CriticalSectionLock lock(stateLock);

        if (music == localMusic && musicNoteIndex == localNoteIndex)
        {
            musicNoteIndex = nextNoteIndex;
            musicNoteWrittenSamples = 0;
        }
        return true;
    }

    const bool completed = toneSamples(
        note.frequency,
        note.frequency,
        totalSamples,
        localWrittenSamples,
        localMusicVolume,
        localMusicVolume
    );

    {
        CriticalSectionLock lock(stateLock);

        if (music == localMusic && musicNoteIndex == localNoteIndex)
        {
            if (completed || localWrittenSamples >= totalSamples)
            {
                musicNoteIndex = nextNoteIndex;
                musicNoteWrittenSamples = 0;
            }
            else
            {
                musicNoteWrittenSamples = localWrittenSamples;
            }
        }
    }

    return completed;
}

void AudioBase::update()
{
    if (hasPendingSE())
    {
        float seGain = 1.0f;
        const Sound* sound = takeTriggerSE(seGain);

        if (sound != nullptr)
        {
            playSound(sound, seGain);
        }
    }

    if (hasPendingSE()) return;

    if (musicActive.load(std::memory_order_acquire))
    {
        playMusicStep();
    }
}
