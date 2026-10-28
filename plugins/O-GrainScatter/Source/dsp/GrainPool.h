/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>
#include "DelayBuffer.h"
#include "FreezeManager.h"
#include "SpatialEncoder.h"

struct GrainParams
{
    double positionInSamples = 0.0;   // Delay offset in samples (v3.1.0: double)
    float playbackRate = 1.0f;
    float panPosition = 0.5f;         // 0=left, 1=right
    int grainLengthSamples = 4410;
    float amplitude = 1.0f;           // Per-grain amplitude (0-1)
    bool reverse = false;
    bool readFromFrozen = false;
    int frozenSlot = -1;              // v3.0.4: FreezeManager slot, -1 = live delay line
    double frozenBase = 0.0;          // v3.0.4: snapshot position at spawn (head - delay)

    // Spatial (used when spatial_mode != Off)
    float azimuth = 0.0f;             // radians, 0=front
    float elevation = 0.0f;           // radians, 0=horizon
    float distance = 0.5f;            // 0-1 normalized
    int trajectoryType = 0;           // 0=Static, 1=Orbital, 2=Spiral, 3=Random
};

struct GrainVoice
{
    bool active = false;
    double readPosition = 0.0;          // v3.1.0: double (12 s ring precision)
    float playbackRate = 1.0f;
    float panPosition = 0.5f;
    int samplesRemaining = 0;
    int grainLengthSamples = 0;
    float amplitude = 1.0f;           // Per-grain amplitude (0-1)
    bool reverse = false;
    bool readFromFrozen = false;
    double positionOffset = 0.0;      // Base position in delay buffer (v3.1.0: double)
    int frozenSlot = -1;              // v3.0.4: latched at spawn; the grain finishes on it after release
    double frozenBase = 0.0;

    // Spatial state
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float distance = 0.5f;
    SmoothedSHCoeffs shCoeffs;
    int trajectoryType = 0;
    unsigned int rngState = 0;          // LCG state for random trajectory
    float dopplerFactor = 1.0f;         // Per-grain Doppler pitch mod (computed from angular velocity)
    // v3.3.0 band-limited reader: octave level latched at spawn from playbackRate (which
    // already carries the freeze detune), and the sinc set for the residual rate on it.
    // The stereo path keeps sincSet; the spatial path re-picks it per sample from
    // playbackRate * dopplerFactor, never the level.
    std::uint8_t level = 0;
    std::uint8_t sincSet = 0;
    float stealGain = 1.0f;             // v3.0.5: steal fade-out gain (1 = not fading)
    float stealStep = 0.0f;             // v3.0.5: per-sample decrement, > 0 while fading
};

// Grain envelope shapes
enum GrainShape
{
    Hann = 0,
    Triangle,
    Trapezoid,
    Tukey,
    Blackman,
    ExponentialDecay,
    NumShapes
};

class GrainPool
{
public:
    static constexpr int MaxVoices = 64;      // grains at full level (and the viz snapshot size)
    // v3.0.5: a stolen grain is moved into one of these spare slots and fades out over
    // kStealFadeMs there, while the new grain takes the slot it always took. Steal-free
    // renders are bit-identical to v3.0.4; a steal is no longer a hard cut.
    static constexpr int kFadeSlots = 8;
    static constexpr int kNumSlots = MaxVoices + kFadeSlots;
    static constexpr double kStealFadeMs = 5.0;

    void setSampleRate (double sampleRate)
    {
        stealFadeStep = static_cast<float> (1.0 / juce::jmax (1.0, sampleRate * kStealFadeMs / 1000.0));
    }

    void setGrainShape (int shape) { grainShape = shape; }

    // IN-05: deactivate all voices and reset round-robin. Called from prepareToPlay()
    // (and reset()) so a sample-rate/block-size change without a host reset() can't leave
    // grains active with stale grainLengthSamples/readPosition.
    void clearVoices()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.samplesRemaining = 0;
            v.stealGain = 1.0f;
            v.stealStep = 0.0f;
        }
        nextVoice = 0;
    }

    // v3.3.0: the reader's level / set choice (grain-engine BandlimitedReader).
    static int latchLevel (float rate)
    {
        return ouaricon::grain::latchLevel (static_cast<double> (rate), DelayBuffer::kMaxLevel);
    }

    static int sincSetFor (float rate, int level)
    {
        return ouaricon::grain::SincBank::setFor (std::ldexp (static_cast<double> (rate), -level));
    }

    // Compute envelope value for a given phase (0..1) and shape
    static float computeEnvelope (float phase, int shape)
    {
        switch (shape)
        {
            case GrainShape::Hann:
            default:
                return 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase));

            case GrainShape::Triangle:
                return 1.0f - std::abs (2.0f * phase - 1.0f);

            case GrainShape::Trapezoid:
            {
                // Linear ramp 0-20%, flat 20-80%, ramp 80-100%
                if (phase < 0.2f)
                    return phase * 5.0f;
                if (phase > 0.8f)
                    return (1.0f - phase) * 5.0f;
                return 1.0f;
            }

            case GrainShape::Tukey:
            {
                // Tukey window with alpha=0.5 (cosine taper first/last 25%, flat middle 50%)
                constexpr float alpha = 0.5f;
                if (phase < alpha * 0.5f)
                    return 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase / alpha));
                if (phase > 1.0f - alpha * 0.5f)
                    return 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * (1.0f - phase) / alpha));
                return 1.0f;
            }

            case GrainShape::Blackman:
                return 0.42f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase)
                             + 0.08f * std::cos (2.0f * juce::MathConstants<float>::twoPi * phase);

            case GrainShape::ExponentialDecay:
                return std::exp (-5.0f * phase);
        }
    }

    // v3.2.0: window moments over normalised grain time, mu1 = integral of w, mu2 = integral
    // of w^2. They set the density compensation: coherent grains sum as N*mu1 in amplitude,
    // incoherent grains as sqrt(N*mu2). Checked numerically against computeEnvelope() by
    // tests/density-gain.
    struct WindowMoments { float mu1, mu2; };

    static WindowMoments windowMoments (int shape)
    {
        switch (shape)
        {
            case GrainShape::Hann:
            default:                           return { 0.5f,    0.375f };
            case GrainShape::Triangle:         return { 0.5f,    1.0f / 3.0f };
            case GrainShape::Trapezoid:        return { 0.8f,    1.0f - 4.0f * 0.2f / 3.0f };   // ramp a = 0.2
            case GrainShape::Tukey:            return { 0.75f,   1.0f - 5.0f * 0.5f / 8.0f };   // alpha = 0.5
            case GrainShape::Blackman:         return { 0.42f,   0.42f * 0.42f + 0.5f * 0.5f / 2.0f + 0.08f * 0.08f / 2.0f };
            case GrainShape::ExponentialDecay: return { (1.0f - std::exp (-5.0f)) / 5.0f, (1.0f - std::exp (-10.0f)) / 10.0f };
        }
    }

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
        {
            targetVoice = oldestVoice;
            moveToFadeSlot (voices[static_cast<size_t> (targetVoice)]);
        }

        auto& v = voices[static_cast<size_t> (targetVoice)];
        v.active = true;
        v.stealGain = 1.0f;
        v.stealStep = 0.0f;
        v.positionOffset = params.positionInSamples;
        v.playbackRate = params.playbackRate;
        v.panPosition = params.panPosition;
        v.grainLengthSamples = params.grainLengthSamples;
        v.samplesRemaining = params.grainLengthSamples;
        v.amplitude = params.amplitude;
        v.reverse = params.reverse;
        v.readFromFrozen = params.readFromFrozen;
        v.frozenSlot = params.readFromFrozen ? params.frozenSlot : -1;
        v.frozenBase = params.frozenBase;
        v.level = static_cast<std::uint8_t> (latchLevel (params.playbackRate));
        v.sincSet = static_cast<std::uint8_t> (sincSetFor (params.playbackRate, v.level));

        // Start position
        if (v.reverse)
            v.readPosition = static_cast<double> (params.grainLengthSamples - 1);
        else
            v.readPosition = 0.0;

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

    // v3.0.5: copy a stolen grain into a spare slot and start its fade. With every spare
    // already fading (more than kFadeSlots steals inside one fade time), the spare
    // nearest silence is reused.
    void moveToFadeSlot (const GrainVoice& victim)
    {
        int slot = -1;
        float minGain = 2.0f;
        for (int i = MaxVoices; i < kNumSlots; ++i)
        {
            const auto& f = voices[static_cast<size_t> (i)];
            if (! f.active) { slot = i; break; }
            if (f.stealGain < minGain) { minGain = f.stealGain; slot = i; }
        }

        auto& f = voices[static_cast<size_t> (slot)];
        f = victim;
        if (f.stealStep <= 0.0f)
            f.stealStep = stealFadeStep;
    }

    // v3.0.4: true while any sounding grain still reads this freeze slot.
    bool isFrozenSlotInUse (int slot) const
    {
        for (const auto& v : voices)
            if (v.active && v.frozenSlot == slot)
                return true;
        return false;
    }

    void prepareSpatial (float sampleRate)
    {
        for (auto& v : voices)
            v.shCoeffs.prepare (sampleRate, 5.0f);
    }

    // Returns the live grain count this sample (v3.2.0); a fading steal counts as its gain.
    float processSample (const DelayBuffer& delayBuf, const FreezeManager& freezeMgr,
                         float& outL, float& outR)
    {
        outL = 0.0f;
        outR = 0.0f;
        float live = 0.0f;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Grain envelope
            float phase = 1.0f - static_cast<float> (v.samplesRemaining)
                                / static_cast<float> (v.grainLengthSamples);
            float envelope = computeEnvelope (phase, grainShape);

            // Read sample from delay buffer or frozen buffer
            float sample = 0.0f;
            float sampleR = 0.0f;

            if (v.frozenSlot >= 0)
            {
                const double pos = v.frozenBase + v.readPosition;
                freezeMgr.readStereo (v.frozenSlot, v.level, pos, v.sincSet, sample, sampleR);
            }
            else
            {
                // Compensate for the advancing write head: elapsed tracks how many
                // samples have passed since grain spawn, so (elapsed - readPosition)
                // cancels out the write-head drift at playbackRate 1.0, yielding a
                // constant delay tap that streams real audio through the grain window.
                // The reader clamps the tap to margin(level) behind the head.
                double elapsed = static_cast<double> (v.grainLengthSamples - v.samplesRemaining);
                double delaySamples = v.positionOffset + elapsed - v.readPosition;
                delayBuf.readStereo (v.level, delaySamples, v.sincSet, sample, sampleR);
            }

            live += v.stealGain;
            sample  *= envelope * v.amplitude * v.stealGain;
            sampleR *= envelope * v.amplitude * v.stealGain;

            // Pan: equal-power
            float panL = std::cos (v.panPosition * juce::MathConstants<float>::halfPi);
            float panR = std::sin (v.panPosition * juce::MathConstants<float>::halfPi);

            outL += sample * panL;
            outR += sampleR * panR;

            // Advance read position
            if (v.reverse)
                v.readPosition -= static_cast<double> (v.playbackRate);
            else
                v.readPosition += static_cast<double> (v.playbackRate);

            --v.samplesRemaining;
            if (v.samplesRemaining <= 0)
                v.active = false;

            if (v.stealStep > 0.0f)
            {
                v.stealGain -= v.stealStep;
                if (v.stealGain <= 0.0f)
                    v.active = false;
            }
        }

        return live;
    }

    // Spatial processing: outputs mono per-grain into a 16-channel HOA3 bus.
    // hoaBus must point to kHOA3Channels (16) float arrays, each of at least 1 sample.
    // This is the per-sample equivalent for spatial mode.
    float processSampleSpatial (const DelayBuffer& delayBuf, const FreezeManager& freezeMgr,
                                float* hoaBus)
    {
        float live = 0.0f;
        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Grain envelope
            float phase = 1.0f - static_cast<float> (v.samplesRemaining)
                                / static_cast<float> (v.grainLengthSamples);
            float envelope = computeEnvelope (phase, grainShape);

            // Read sample (stereo -> mono). v3.3.0: Doppler moves the residual rate inside
            // the grain, so the sinc set follows it per sample (the level stays latched;
            // the reader clamps the tap to margin(level), which Doppler could otherwise
            // drive past the write head).
            const float advanceRate = v.playbackRate * v.dopplerFactor;
            const int set = sincSetFor (advanceRate, v.level);
            float monoIn = 0.0f;

            if (v.frozenSlot >= 0)
            {
                const double pos = v.frozenBase + v.readPosition;
                monoIn = freezeMgr.readMono (v.frozenSlot, v.level, pos, set);
            }
            else
            {
                double elapsed = static_cast<double> (v.grainLengthSamples - v.samplesRemaining);
                double delaySamples = v.positionOffset + elapsed - v.readPosition;
                monoIn = delayBuf.readMono (v.level, delaySamples, set);
            }

            live += v.stealGain;
            float mono = monoIn * envelope * v.amplitude * v.stealGain;

            // Distance attenuation (kDistanceScale controls attenuation steepness).
            // IN-10: grain gain uses v.distance FROZEN at spawn, whereas the processor's
            // distance-LPF uses the live per-block Distance value — so moving Distance
            // changes the LPF immediately but only affects grain gain for grains spawned
            // afterward. Intended (per-grain snapshot for gain, continuous filter for tone).
            static constexpr float kDistanceScale = 3.0f;
            float distGain = 1.0f / (1.0f + v.distance * kDistanceScale);
            mono *= distGain;

            // Advance SH coefficient smoothing
            v.shCoeffs.advance();

            // Encode into HOA3 bus
            for (int ch = 0; ch < kHOA3Channels; ++ch)
                hoaBus[ch] += mono * v.shCoeffs.current[ch];

            // Advance read position (with Doppler pitch modulation)
            double advance = static_cast<double> (advanceRate);
            if (v.reverse)
                v.readPosition -= advance;
            else
                v.readPosition += advance;

            --v.samplesRemaining;
            if (v.samplesRemaining <= 0)
                v.active = false;

            if (v.stealStep > 0.0f)
            {
                v.stealGain -= v.stealStep;
                if (v.stealGain <= 0.0f)
                    v.active = false;
            }
        }

        return live;
    }

    // Update SH smoothing time for all voices (call once per block when parameter changes)
    void setSpatialSmoothTime (float smoothTimeMs)
    {
        for (auto& v : voices)
            v.shCoeffs.setSmoothTime (smoothTimeMs);
    }

    const std::array<GrainVoice, kNumSlots>& getVoices() const { return voices; }
    std::array<GrainVoice, kNumSlots>& getVoices() { return voices; }

private:
    std::array<GrainVoice, kNumSlots> voices {};
    int nextVoice = 0;
    float stealFadeStep = 1.0f / 220.5f;
    int grainShape = GrainShape::Hann;
};
