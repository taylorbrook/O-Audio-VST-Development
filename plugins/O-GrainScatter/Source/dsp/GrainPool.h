#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include "DelayBuffer.h"
#include "FreezeManager.h"
#include "SpatialEncoder.h"

struct GrainParams
{
    float positionInSamples = 0.0f;   // Delay offset in samples
    float playbackRate = 1.0f;
    float panPosition = 0.5f;         // 0=left, 1=right
    int grainLengthSamples = 4410;
    bool reverse = false;
    bool readFromFrozen = false;

    // Spatial (used when spatial_mode != Off)
    float azimuth = 0.0f;             // radians, 0=front
    float elevation = 0.0f;           // radians, 0=horizon
    float distance = 0.5f;            // 0-1 normalized
    int trajectoryType = 0;           // 0=Static, 1=Orbital, 2=Spiral, 3=Random
};

struct GrainVoice
{
    bool active = false;
    float readPosition = 0.0f;
    float playbackRate = 1.0f;
    float panPosition = 0.5f;
    int samplesRemaining = 0;
    int grainLengthSamples = 0;
    bool reverse = false;
    bool readFromFrozen = false;
    float positionOffset = 0.0f;      // Base position in delay buffer

    // Spatial state
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float distance = 0.5f;
    SmoothedSHCoeffs shCoeffs;
    int trajectoryType = 0;
    unsigned int rngState = 0;          // LCG state for random trajectory
};

class GrainPool
{
public:
    static constexpr int MaxVoices = 64;

    void spawnGrain (const GrainParams& params)
    {
        // Round-robin: find next inactive voice, or steal oldest
        int targetVoice = -1;
        int oldestVoice = 0;
        int maxAge = 0;

        for (int i = 0; i < MaxVoices; ++i)
        {
            int idx = (nextVoice + i) % MaxVoices;
            if (!voices[static_cast<size_t> (idx)].active)
            {
                targetVoice = idx;
                break;
            }
            int age = voices[static_cast<size_t> (idx)].grainLengthSamples
                    - voices[static_cast<size_t> (idx)].samplesRemaining;
            if (age > maxAge)
            {
                maxAge = age;
                oldestVoice = idx;
            }
        }

        if (targetVoice < 0)
            targetVoice = oldestVoice;

        auto& v = voices[static_cast<size_t> (targetVoice)];
        v.active = true;
        v.positionOffset = params.positionInSamples;
        v.playbackRate = params.playbackRate;
        v.panPosition = params.panPosition;
        v.grainLengthSamples = params.grainLengthSamples;
        v.samplesRemaining = params.grainLengthSamples;
        v.reverse = params.reverse;
        v.readFromFrozen = params.readFromFrozen;

        // Start position
        if (v.reverse)
            v.readPosition = static_cast<float> (params.grainLengthSamples - 1);
        else
            v.readPosition = 0.0f;

        // Spatial initialization: snap SH coefficients (no smoothing at grain onset)
        v.azimuth = params.azimuth;
        v.elevation = params.elevation;
        v.distance = params.distance;
        v.trajectoryType = params.trajectoryType;
        v.shCoeffs.setTarget (params.azimuth, params.elevation);
        v.shCoeffs.snapToTarget();
        v.rngState = static_cast<unsigned int> (targetVoice * 7919 + v.grainLengthSamples);

        nextVoice = (targetVoice + 1) % MaxVoices;
    }

    void prepareSpatial (float sampleRate)
    {
        for (auto& v : voices)
            v.shCoeffs.prepare (sampleRate, 5.0f);
    }

    void processSample (const DelayBuffer& delayBuf, const FreezeManager& freezeMgr,
                        float& outL, float& outR)
    {
        outL = 0.0f;
        outR = 0.0f;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Hann window envelope
            float phase = 1.0f - static_cast<float> (v.samplesRemaining)
                                / static_cast<float> (v.grainLengthSamples);
            float envelope = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase));

            // Read sample from delay buffer or frozen buffer
            float sample = 0.0f;
            float sampleR = 0.0f;

            if (v.readFromFrozen && freezeMgr.isActive())
            {
                sample  = freezeMgr.readSample (0, v.readPosition);
                sampleR = freezeMgr.readSample (1, v.readPosition);
            }
            else
            {
                // Compensate for the advancing write head: elapsed tracks how many
                // samples have passed since grain spawn, so (elapsed - readPosition)
                // cancels out the write-head drift at playbackRate 1.0, yielding a
                // constant delay tap that streams real audio through the grain window.
                float elapsed = static_cast<float> (v.grainLengthSamples - v.samplesRemaining);
                float delaySamples = v.positionOffset + elapsed - v.readPosition;
                sample  = delayBuf.readSample (0, delaySamples);
                sampleR = delayBuf.readSample (1, delaySamples);
            }

            sample  *= envelope;
            sampleR *= envelope;

            // Pan: equal-power
            float panL = std::cos (v.panPosition * juce::MathConstants<float>::halfPi);
            float panR = std::sin (v.panPosition * juce::MathConstants<float>::halfPi);

            outL += sample * panL;
            outR += sampleR * panR;

            // Advance read position
            if (v.reverse)
                v.readPosition -= v.playbackRate;
            else
                v.readPosition += v.playbackRate;

            --v.samplesRemaining;
            if (v.samplesRemaining <= 0)
                v.active = false;
        }
    }

    // Spatial processing: outputs mono per-grain into a 16-channel HOA3 bus.
    // hoaBus must point to kHOA3Channels (16) float arrays, each of at least 1 sample.
    // This is the per-sample equivalent for spatial mode.
    void processSampleSpatial (const DelayBuffer& delayBuf, const FreezeManager& freezeMgr,
                               float* hoaBus)
    {
        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Hann window envelope
            float phase = 1.0f - static_cast<float> (v.samplesRemaining)
                                / static_cast<float> (v.grainLengthSamples);
            float envelope = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase));

            // Read sample (stereo -> mono)
            float sampleL = 0.0f;
            float sampleR = 0.0f;

            if (v.readFromFrozen && freezeMgr.isActive())
            {
                sampleL = freezeMgr.readSample (0, v.readPosition);
                sampleR = freezeMgr.readSample (1, v.readPosition);
            }
            else
            {
                float elapsed = static_cast<float> (v.grainLengthSamples - v.samplesRemaining);
                float delaySamples = v.positionOffset + elapsed - v.readPosition;
                sampleL = delayBuf.readSample (0, delaySamples);
                sampleR = delayBuf.readSample (1, delaySamples);
            }

            float mono = (sampleL + sampleR) * 0.5f * envelope;

            // Distance attenuation
            float distGain = 1.0f / (1.0f + v.distance * 3.0f);
            mono *= distGain;

            // Advance SH coefficient smoothing
            v.shCoeffs.advance();

            // Encode into HOA3 bus
            for (int ch = 0; ch < kHOA3Channels; ++ch)
                hoaBus[ch] += mono * v.shCoeffs.current[ch];

            // Advance read position
            if (v.reverse)
                v.readPosition -= v.playbackRate;
            else
                v.readPosition += v.playbackRate;

            --v.samplesRemaining;
            if (v.samplesRemaining <= 0)
                v.active = false;
        }
    }

    int getActiveCount() const
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        return count;
    }

    const std::array<GrainVoice, MaxVoices>& getVoices() const { return voices; }
    std::array<GrainVoice, MaxVoices>& getVoices() { return voices; }

private:
    std::array<GrainVoice, MaxVoices> voices {};
    int nextVoice = 0;
};
