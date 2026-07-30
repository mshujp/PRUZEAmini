#pragma once

#include "PRUZEAmini.h"

#include <stdint.h>

namespace PRUZEAmini {

class MidiPlayer
{
public:
    static constexpr uint8_t MAX_TRACKS = 32;
    static constexpr uint8_t MAX_VOICES = 16;

    MidiPlayer();

    bool play(const Audio::Midi* midi, uint32_t sampleRate);
    void stop();
    bool isPlaying() const;

    // Generates signed 16-bit mono PCM samples.
    // Returns the number of samples written. The remainder is filled with zero.
    uint32_t render(int16_t* output, uint32_t sampleCount);

private:
    enum EventType : uint8_t
    {
        EVENT_NONE,
        EVENT_NOTE_ON,
        EVENT_NOTE_OFF,
        EVENT_CONTROL_CHANGE,
        EVENT_PROGRAM_CHANGE,
        EVENT_PITCH_BEND,
        EVENT_TEMPO,
        EVENT_END_OF_TRACK
    };

    struct Event
    {
        EventType type = EVENT_NONE;
        uint32_t tick = 0;
        uint8_t channel = 0;
        uint8_t data1 = 0;
        uint8_t data2 = 0;
        uint32_t tempoUsecPerQuarter = 500000;
    };

    struct TrackCursor
    {
        uint32_t start = 0;
        uint32_t end = 0;
        uint32_t cursor = 0;
        uint32_t nextTick = 0;
        uint8_t runningStatus = 0;
        bool finished = true;
        bool eventReady = false;
        Event event;
    };

    struct ChannelState
    {
        uint8_t program = 0;
        uint8_t volume = 100;
        uint8_t expression = 127;
        bool sustain = false;
        int16_t pitchBend = 0; // -8192 to 8191
    };

    struct Voice
    {
        bool active = false;
        bool releasing = false;
        bool sustained = false;
        bool oneShot = false;
        uint8_t note = 0;
        uint8_t channel = 0;
        uint8_t velocity = 0;
        uint8_t waveform = 0;
        uint32_t age = 0;
        uint32_t remainingSamples = 0;
        uint32_t noiseState = 1;
        float baseFrequency = 0.0f;
        float phase = 0.0f;
        float phaseStep = 0.0f;
        float envelope = 0.0f;
        float releaseStep = 0.0f;
    };

    const Audio::Midi* currentMidi = nullptr;
    const uint8_t* data = nullptr;
    uint32_t dataSize = 0;
    uint32_t outputSampleRate = 22050;
    uint16_t ticksPerQuarter = 96;
    uint32_t tempoUsecPerQuarter = 500000;
    uint32_t currentTick = 0;
    uint64_t samplesUntilEvent = 0;
    uint32_t voiceAgeCounter = 0;
    uint8_t trackCount = 0;
    uint8_t pendingTrack = 0;
    uint8_t repeatIndex = 0;
    bool eventReady = false;
    bool playing = false;
    Event nextEvent;
    TrackCursor tracks[MAX_TRACKS];
    ChannelState channels[16];
    Voice voices[MAX_VOICES];

    bool openFile();
    bool restartTracks();
    bool prepareNextEvent();
    bool readTrackEvent(uint8_t trackIndex);
    bool readTrackByte(TrackCursor& track, uint8_t& value);
    bool readTrackVariableLength(TrackCursor& track, uint32_t& value);
    bool skipTrackBytes(TrackCursor& track, uint32_t count);
    bool readBigEndian16(uint32_t& cursor, uint16_t& value) const;
    bool readBigEndian32(uint32_t& cursor, uint32_t& value) const;
    uint64_t ticksToSamples(uint32_t ticks) const;
    void finishOrRepeat();
    void processEvent(const Event& event);
    void controlChange(uint8_t channel, uint8_t controller, uint8_t value);
    void noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void noteOff(uint8_t channel, uint8_t note);
    void releaseSustained(uint8_t channel);
    void allNotesOff(uint8_t channel, bool immediate);
    void updateChannelPitch(uint8_t channel);
    void clearState();
    void clearVoices();
    Voice* allocateVoice(uint8_t channel, uint8_t note);
    static float noteFrequency(uint8_t note);
    static uint8_t waveformForProgram(uint8_t program);
    static float renderWave(Voice& voice);
};

} // namespace PRUZEAmini
