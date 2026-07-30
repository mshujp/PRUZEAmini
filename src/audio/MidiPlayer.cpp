#include "MidiPlayer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace PRUZEAmini;

namespace {
constexpr uint32_t MIDI_HEADER_CHUNK = 0x4D546864u; // MThd
constexpr uint32_t MIDI_TRACK_CHUNK  = 0x4D54726Bu; // MTrk
constexpr uint32_t DEFAULT_TEMPO_USEC = 500000u;
constexpr float PI2 = 6.2831853071795864769f;
}

MidiPlayer::MidiPlayer()
{
    clearState();
}

bool MidiPlayer::play(const Audio::Midi* midi, uint32_t sampleRate)
{
    stop();
    if (midi == nullptr || midi->data == nullptr || midi->size < 22 || sampleRate == 0) return false;

    currentMidi = midi;
    data = midi->data;
    dataSize = midi->size;
    outputSampleRate = sampleRate;
    repeatIndex = 0;

    if (!openFile())
    {
        stop();
        return false;
    }

    playing = true;
    return true;
}

void MidiPlayer::stop()
{
    currentMidi = nullptr;
    data = nullptr;
    dataSize = 0;
    clearState();
}

bool MidiPlayer::isPlaying() const
{
    return playing;
}

void MidiPlayer::clearState()
{
    outputSampleRate = outputSampleRate == 0 ? 22050 : outputSampleRate;
    ticksPerQuarter = 96;
    tempoUsecPerQuarter = DEFAULT_TEMPO_USEC;
    currentTick = 0;
    samplesUntilEvent = 0;
    voiceAgeCounter = 0;
    trackCount = 0;
    pendingTrack = 0;
    repeatIndex = 0;
    eventReady = false;
    playing = false;
    nextEvent = Event{};
    for (TrackCursor& track : tracks) track = TrackCursor{};
    for (ChannelState& channel : channels) channel = ChannelState{};
    clearVoices();
}

bool MidiPlayer::readBigEndian16(uint32_t& cursor, uint16_t& value) const
{
    if (cursor + 2 > dataSize) return false;
    value = static_cast<uint16_t>((static_cast<uint16_t>(data[cursor]) << 8) |
                                  static_cast<uint16_t>(data[cursor + 1]));
    cursor += 2;
    return true;
}

bool MidiPlayer::readBigEndian32(uint32_t& cursor, uint32_t& value) const
{
    if (cursor + 4 > dataSize) return false;
    value = (static_cast<uint32_t>(data[cursor]) << 24) |
            (static_cast<uint32_t>(data[cursor + 1]) << 16) |
            (static_cast<uint32_t>(data[cursor + 2]) << 8) |
             static_cast<uint32_t>(data[cursor + 3]);
    cursor += 4;
    return true;
}

bool MidiPlayer::openFile()
{
    uint32_t cursor = 0;
    uint32_t chunkId = 0;
    uint32_t headerLength = 0;
    uint16_t format = 0;
    uint16_t fileTrackCount = 0;
    uint16_t division = 0;

    if (!readBigEndian32(cursor, chunkId) || chunkId != MIDI_HEADER_CHUNK) return false;
    if (!readBigEndian32(cursor, headerLength) || headerLength < 6 || cursor + headerLength > dataSize) return false;
    if (!readBigEndian16(cursor, format) || !readBigEndian16(cursor, fileTrackCount) || !readBigEndian16(cursor, division)) return false;
    if ((format != 0 && format != 1) || fileTrackCount == 0 || fileTrackCount > MAX_TRACKS) return false;
    if ((format == 0 && fileTrackCount != 1) || (division & 0x8000u) != 0 || division == 0) return false;
    cursor = 8u + headerLength;

    ticksPerQuarter = division;
    trackCount = static_cast<uint8_t>(fileTrackCount);

    for (uint8_t i = 0; i < trackCount; ++i)
    {
        uint32_t trackLength = 0;
        if (!readBigEndian32(cursor, chunkId) || chunkId != MIDI_TRACK_CHUNK) return false;
        if (!readBigEndian32(cursor, trackLength) || trackLength > dataSize - cursor) return false;
        tracks[i].start = cursor;
        tracks[i].end = cursor + trackLength;
        cursor += trackLength;
    }

    return restartTracks();
}

bool MidiPlayer::restartTracks()
{
    tempoUsecPerQuarter = DEFAULT_TEMPO_USEC;
    currentTick = 0;
    samplesUntilEvent = 0;
    voiceAgeCounter = 0;
    eventReady = false;
    nextEvent = Event{};
    for (ChannelState& channel : channels) channel = ChannelState{};
    clearVoices();

    for (uint8_t i = 0; i < trackCount; ++i)
    {
        TrackCursor& track = tracks[i];
        track.cursor = track.start;
        track.nextTick = 0;
        track.runningStatus = 0;
        track.finished = false;
        track.eventReady = false;
        track.event = Event{};
        if (!readTrackEvent(i)) return false;
    }

    return prepareNextEvent();
}

bool MidiPlayer::readTrackByte(TrackCursor& track, uint8_t& value)
{
    if (track.cursor >= track.end || track.cursor >= dataSize) return false;
    value = data[track.cursor++];
    return true;
}

bool MidiPlayer::readTrackVariableLength(TrackCursor& track, uint32_t& value)
{
    value = 0;
    for (uint8_t i = 0; i < 4; ++i)
    {
        uint8_t byte = 0;
        if (!readTrackByte(track, byte)) return false;
        value = (value << 7) | static_cast<uint32_t>(byte & 0x7Fu);
        if ((byte & 0x80u) == 0) return true;
    }
    return false;
}

bool MidiPlayer::skipTrackBytes(TrackCursor& track, uint32_t count)
{
    if (count > track.end - track.cursor) return false;
    track.cursor += count;
    return true;
}

bool MidiPlayer::readTrackEvent(uint8_t trackIndex)
{
    if (trackIndex >= trackCount) return false;
    TrackCursor& track = tracks[trackIndex];
    track.eventReady = false;
    track.event = Event{};

    if (track.finished) return true;
    if (track.cursor >= track.end)
    {
        track.event.type = EVENT_END_OF_TRACK;
        track.event.tick = track.nextTick;
        track.eventReady = true;
        return true;
    }

    uint32_t deltaTicks = 0;
    if (!readTrackVariableLength(track, deltaTicks)) return false;
    track.nextTick += deltaTicks;
    track.event.tick = track.nextTick;

    uint8_t first = 0;
    if (!readTrackByte(track, first)) return false;

    uint8_t status = first;
    bool firstIsData = false;
    if (first < 0x80u)
    {
        if (track.runningStatus < 0x80u || track.runningStatus >= 0xF0u) return false;
        status = track.runningStatus;
        firstIsData = true;
    }
    else if (status < 0xF0u)
    {
        track.runningStatus = status;
    }

    if (status == 0xFFu)
    {
        track.runningStatus = 0;
        uint8_t metaType = 0;
        uint32_t length = 0;
        if (!readTrackByte(track, metaType) || !readTrackVariableLength(track, length)) return false;

        if (metaType == 0x2Fu)
        {
            if (!skipTrackBytes(track, length)) return false;
            track.event.type = EVENT_END_OF_TRACK;
        }
        else if (metaType == 0x51u && length == 3)
        {
            uint8_t a = 0, b = 0, c = 0;
            if (!readTrackByte(track, a) || !readTrackByte(track, b) || !readTrackByte(track, c)) return false;
            track.event.type = EVENT_TEMPO;
            track.event.tempoUsecPerQuarter = (static_cast<uint32_t>(a) << 16) |
                                               (static_cast<uint32_t>(b) << 8) |
                                                static_cast<uint32_t>(c);
        }
        else
        {
            if (!skipTrackBytes(track, length)) return false;
            track.event.type = EVENT_NONE;
        }

        track.eventReady = true;
        return true;
    }

    if (status == 0xF0u || status == 0xF7u)
    {
        track.runningStatus = 0;
        uint32_t length = 0;
        if (!readTrackVariableLength(track, length) || !skipTrackBytes(track, length)) return false;
        track.event.type = EVENT_NONE;
        track.eventReady = true;
        return true;
    }

    const uint8_t message = status & 0xF0u;
    track.event.channel = status & 0x0Fu;
    uint8_t data1 = 0;
    uint8_t data2 = 0;

    if (firstIsData) data1 = first;
    else if (!readTrackByte(track, data1)) return false;

    if (message != 0xC0u && message != 0xD0u)
    {
        if (!readTrackByte(track, data2)) return false;
    }

    track.event.data1 = data1;
    track.event.data2 = data2;

    switch (message)
    {
        case 0x80u: track.event.type = EVENT_NOTE_OFF; break;
        case 0x90u: track.event.type = data2 == 0 ? EVENT_NOTE_OFF : EVENT_NOTE_ON; break;
        case 0xB0u: track.event.type = EVENT_CONTROL_CHANGE; break;
        case 0xC0u: track.event.type = EVENT_PROGRAM_CHANGE; break;
        case 0xE0u: track.event.type = EVENT_PITCH_BEND; break;
        default: track.event.type = EVENT_NONE; break;
    }

    track.eventReady = true;
    return true;
}

uint64_t MidiPlayer::ticksToSamples(uint32_t ticks) const
{
    if (ticks == 0) return 0;
    const uint64_t numerator = static_cast<uint64_t>(ticks) *
                               static_cast<uint64_t>(tempoUsecPerQuarter) *
                               static_cast<uint64_t>(outputSampleRate);
    const uint64_t denominator = static_cast<uint64_t>(ticksPerQuarter) * 1000000ull;
    return std::max<uint64_t>(1ull, (numerator + denominator / 2ull) / denominator);
}

bool MidiPlayer::prepareNextEvent()
{
    uint8_t selected = MAX_TRACKS;
    uint32_t selectedTick = 0xFFFFFFFFu;

    for (uint8_t i = 0; i < trackCount; ++i)
    {
        const TrackCursor& track = tracks[i];
        if (!track.finished && track.eventReady && track.event.tick < selectedTick)
        {
            selected = i;
            selectedTick = track.event.tick;
        }
    }

    if (selected == MAX_TRACKS)
    {
        eventReady = false;
        return false;
    }

    pendingTrack = selected;
    nextEvent = tracks[selected].event;
    samplesUntilEvent = ticksToSamples(selectedTick - currentTick);
    eventReady = true;
    return true;
}

void MidiPlayer::finishOrRepeat()
{
    bool allFinished = true;
    for (uint8_t i = 0; i < trackCount; ++i)
    {
        if (!tracks[i].finished)
        {
            allFinished = false;
            break;
        }
    }
    if (!allFinished) return;

    const bool repeat = currentMidi != nullptr &&
        (currentMidi->playCount == 0 || static_cast<uint16_t>(repeatIndex + 1u) < currentMidi->playCount);

    if (repeat)
    {
        if (currentMidi->playCount != 0) ++repeatIndex;
        if (!restartTracks()) playing = false;
    }
    else
    {
        playing = false;
        eventReady = false;
        for (Voice& voice : voices)
        {
            if (voice.active)
            {
                voice.releasing = true;
                voice.releaseStep = std::max(voice.releaseStep, 1.0f / (0.12f * outputSampleRate));
            }
        }
    }
}

void MidiPlayer::processEvent(const Event& event)
{
    switch (event.type)
    {
        case EVENT_NOTE_ON: noteOn(event.channel, event.data1, event.data2); break;
        case EVENT_NOTE_OFF: noteOff(event.channel, event.data1); break;
        case EVENT_CONTROL_CHANGE: controlChange(event.channel, event.data1, event.data2); break;
        case EVENT_PROGRAM_CHANGE: channels[event.channel].program = event.data1; break;
        case EVENT_PITCH_BEND:
            channels[event.channel].pitchBend = static_cast<int16_t>(((static_cast<uint16_t>(event.data2) << 7) | event.data1) - 8192);
            updateChannelPitch(event.channel);
            break;
        case EVENT_TEMPO:
            if (event.tempoUsecPerQuarter > 0) tempoUsecPerQuarter = event.tempoUsecPerQuarter;
            break;
        default: break;
    }
}

void MidiPlayer::controlChange(uint8_t channel, uint8_t controller, uint8_t value)
{
    ChannelState& state = channels[channel];
    switch (controller)
    {
        case 7: state.volume = value; break;
        case 11: state.expression = value; break;
        case 64:
        {
            const bool wasSustain = state.sustain;
            state.sustain = value >= 64;
            if (wasSustain && !state.sustain) releaseSustained(channel);
            break;
        }
        case 120: allNotesOff(channel, true); break;
        case 121:
            state.volume = 100;
            state.expression = 127;
            state.sustain = false;
            state.pitchBend = 0;
            releaseSustained(channel);
            updateChannelPitch(channel);
            break;
        case 123: allNotesOff(channel, false); break;
        default: break;
    }
}

MidiPlayer::Voice* MidiPlayer::allocateVoice(uint8_t channel, uint8_t note)
{
    for (Voice& voice : voices)
    {
        if (voice.active && voice.channel == channel && voice.note == note) return &voice;
    }
    for (Voice& voice : voices)
    {
        if (!voice.active) return &voice;
    }

    Voice* candidate = &voices[0];
    for (Voice& voice : voices)
    {
        if (voice.releasing && !candidate->releasing) candidate = &voice;
        else if (voice.releasing == candidate->releasing)
        {
            if (voice.envelope < candidate->envelope) candidate = &voice;
            else if (voice.envelope == candidate->envelope && voice.age < candidate->age) candidate = &voice;
        }
    }
    return candidate;
}

void MidiPlayer::noteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (note > 127 || velocity == 0) return;

    Voice* voice = allocateVoice(channel, note);
    *voice = Voice{};
    voice->active = true;
    voice->note = note;
    voice->channel = channel;
    voice->velocity = velocity;
    voice->age = ++voiceAgeCounter;
    voice->noiseState = 0x1234567u ^ (static_cast<uint32_t>(note) << 16) ^ voiceAgeCounter;
    voice->envelope = 1.0f;

    if (channel == 9)
    {
        voice->oneShot = true;
        if (note == 35 || note == 36)
        {
            voice->waveform = 4; // kick
            voice->baseFrequency = note == 35 ? 72.0f : 86.0f;
            voice->remainingSamples = outputSampleRate / 5u;
            voice->releaseStep = 1.0f / std::max<uint32_t>(1u, voice->remainingSamples);
        }
        else
        {
            voice->waveform = 3; // noise percussion
            const bool cymbal = note >= 46;
            voice->remainingSamples = cymbal ? outputSampleRate / 3u : outputSampleRate / 12u;
            voice->releaseStep = 1.0f / std::max<uint32_t>(1u, voice->remainingSamples);
        }
        return;
    }

    voice->waveform = waveformForProgram(channels[channel].program);
    voice->baseFrequency = noteFrequency(note);
    voice->releaseStep = 1.0f / (0.12f * static_cast<float>(outputSampleRate));
    const float bendSemitones = (static_cast<float>(channels[channel].pitchBend) / 8192.0f) * 2.0f;
    const float frequency = voice->baseFrequency * std::pow(2.0f, bendSemitones / 12.0f);
    voice->phaseStep = frequency / static_cast<float>(outputSampleRate);
}

void MidiPlayer::noteOff(uint8_t channel, uint8_t note)
{
    if (channel == 9) return;
    for (Voice& voice : voices)
    {
        if (voice.active && voice.channel == channel && voice.note == note && !voice.oneShot)
        {
            if (channels[channel].sustain) voice.sustained = true;
            else voice.releasing = true;
        }
    }
}

void MidiPlayer::releaseSustained(uint8_t channel)
{
    for (Voice& voice : voices)
    {
        if (voice.active && voice.channel == channel && voice.sustained)
        {
            voice.sustained = false;
            voice.releasing = true;
        }
    }
}

void MidiPlayer::allNotesOff(uint8_t channel, bool immediate)
{
    for (Voice& voice : voices)
    {
        if (!voice.active || voice.channel != channel) continue;
        if (immediate) voice = Voice{};
        else
        {
            voice.sustained = false;
            voice.releasing = true;
        }
    }
}

void MidiPlayer::updateChannelPitch(uint8_t channel)
{
    const float bendSemitones = (static_cast<float>(channels[channel].pitchBend) / 8192.0f) * 2.0f;
    const float multiplier = std::pow(2.0f, bendSemitones / 12.0f);
    for (Voice& voice : voices)
    {
        if (voice.active && voice.channel == channel && !voice.oneShot)
        {
            voice.phaseStep = (voice.baseFrequency * multiplier) / static_cast<float>(outputSampleRate);
        }
    }
}

void MidiPlayer::clearVoices()
{
    for (Voice& voice : voices) voice = Voice{};
}

float MidiPlayer::noteFrequency(uint8_t note)
{
    return 440.0f * std::pow(2.0f, (static_cast<int>(note) - 69) / 12.0f);
}

uint8_t MidiPlayer::waveformForProgram(uint8_t program)
{
    // General MIDI programs are reduced to lightweight waveform families.
    if (program <= 15) return 1;  // piano / chromatic: triangle
    if (program <= 39) return 2;  // organ / guitar / bass: saw
    if (program <= 55) return 1;  // strings / ensemble: triangle
    if (program <= 79) return 2;  // brass / reed / pipe: saw
    if (program <= 95) return 0;  // synth lead / pad: square
    return 1;                     // effects / ethnic / percussion: triangle
}

float MidiPlayer::renderWave(Voice& voice)
{
    switch (voice.waveform)
    {
        case 1: // triangle
            return 1.0f - 4.0f * std::fabs(voice.phase - 0.5f);
        case 2: // saw
            return voice.phase * 2.0f - 1.0f;
        case 3: // noise
            voice.noiseState = voice.noiseState * 1664525u + 1013904223u;
            return static_cast<float>(static_cast<int32_t>(voice.noiseState >> 16) - 32768) / 32768.0f;
        case 4: // kick: falling sine
            return std::sin(voice.phase * PI2);
        default: // square
            return voice.phase < 0.5f ? -1.0f : 1.0f;
    }
}

uint32_t MidiPlayer::render(int16_t* output, uint32_t sampleCount)
{
    if (output == nullptr || sampleCount == 0) return 0;
    std::fill(output, output + sampleCount, static_cast<int16_t>(0));
    if (currentMidi == nullptr) return sampleCount;

    const float gain = std::clamp(currentMidi->gain, 0.0f, 1.0f);

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        while (playing && eventReady && samplesUntilEvent == 0)
        {
            currentTick = nextEvent.tick;
            const uint8_t trackIndex = pendingTrack;
            const Event event = nextEvent;
            eventReady = false;

            if (event.type == EVENT_END_OF_TRACK)
            {
                tracks[trackIndex].finished = true;
                tracks[trackIndex].eventReady = false;
            }
            else
            {
                processEvent(event);
                if (!readTrackEvent(trackIndex))
                {
                    playing = false;
                    eventReady = false;
                    break;
                }
            }

            finishOrRepeat();
            if (playing && !eventReady && !prepareNextEvent()) finishOrRepeat();
        }

        float mixed = 0.0f;
        uint8_t activeCount = 0;
        for (Voice& voice : voices)
        {
            if (!voice.active) continue;
            ++activeCount;

            const ChannelState& channel = channels[voice.channel];
            const float velocityGain = static_cast<float>(voice.velocity) / 127.0f;
            const float channelGain = (static_cast<float>(channel.volume) / 127.0f) *
                                      (static_cast<float>(channel.expression) / 127.0f);
            mixed += renderWave(voice) * voice.envelope * velocityGain * channelGain;

            if (voice.waveform == 4)
            {
                const float progress = voice.remainingSamples > 0
                    ? static_cast<float>(voice.remainingSamples) / static_cast<float>(std::max<uint32_t>(1u, outputSampleRate / 5u))
                    : 0.0f;
                const float kickFrequency = voice.baseFrequency * (0.45f + progress * 0.75f);
                voice.phaseStep = kickFrequency / static_cast<float>(outputSampleRate);
            }

            voice.phase += voice.phaseStep;
            while (voice.phase >= 1.0f) voice.phase -= 1.0f;

            if (voice.oneShot)
            {
                if (voice.remainingSamples > 0) --voice.remainingSamples;
                voice.envelope -= voice.releaseStep;
                if (voice.remainingSamples == 0 || voice.envelope <= 0.0f) voice = Voice{};
            }
            else if (voice.releasing)
            {
                voice.envelope -= voice.releaseStep;
                if (voice.envelope <= 0.0f) voice = Voice{};
            }
        }

        const float divisor = activeCount > 0 ? std::sqrt(static_cast<float>(activeCount)) : 1.0f;
        mixed = std::clamp((mixed / divisor) * gain * 0.72f, -1.0f, 1.0f);
        output[i] = static_cast<int16_t>(mixed * 30000.0f);

        if (eventReady && samplesUntilEvent > 0) --samplesUntilEvent;
    }

    bool anyVoice = false;
    for (const Voice& voice : voices) anyVoice = anyVoice || voice.active;
    if (!playing && !anyVoice) currentMidi = nullptr;

    return sampleCount;
}
