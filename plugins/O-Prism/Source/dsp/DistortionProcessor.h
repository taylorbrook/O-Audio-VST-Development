/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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
/*
  ==============================================================================

    DistortionProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class DistortionProcessor
{
public:
    DistortionProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setType (int type);
    void setDrive (float drive);
    void setMix (float mix);

    /** Exact path latency (59.5 at 4x steep FIR); the dry path is aligned to it. */
    float getLatencyInSamples() const { return static_cast<float> (oversampling.getLatencyInSamples()); }

    /** What the host is told: the exact figure rounded (59.5 -> 60). */
    int getReportedLatencySamples() const { return static_cast<int> (std::lround (getLatencyInSamples())); }

private:
    void applyDistortion (juce::dsp::AudioBlock<float>& block);
    void processChunk (juce::dsp::AudioBlock<float> chunk);
    void processDryOnly (juce::dsp::AudioBlock<float> chunk);
    float delayDrySample (size_t ch, float x) noexcept;

    /** Channel count the oversampler is built for. The oversampled block can
        therefore never present more channels than this, which is what bounds
        `adaaPrevU` below -- the two MUST stay in agreement.
    */
    static constexpr size_t kNumChannels = 2;

    juce::dsp::Oversampling<float> oversampling;

    /** v1.30.1: the oversampler's stage buffers are sized by initProcessing()
        and only jassert the block length, so a host block longer than the
        prepared one overran them in Release. process() runs the core in chunks
        no longer than this. */
    int preparedBlockSize = 0;

    /** v1.30.1: the dry path, replacing juce::dsp::DryWetMixer. The 4x steep FIR
        cascade is 59.5 samples; DryWetMixer delayed the dry copy with a Thiran
        allpass, whose phase is exact only near DC, so the dry/wet sum combed at
        HF. Here the half sample comes from a linear-phase windowed-sinc FIR (32
        taps, Kaiser beta 6: exactly 15.5 samples, flat to +/-0.01 dB below
        21 kHz) and an integer delay line supplies the other 44. Same recipe as
        O-AnalogSaturation v1.7.0. */
    static constexpr int kHalfFirTaps = 32;
    std::array<float, kHalfFirTaps> halfFirCoeffs {};
    std::array<std::array<float, 2 * kHalfFirTaps>, kNumChannels> halfFirHistory {}; // mirrored ring
    int halfFirPos = 0;
    bool dryUsesHalfFir = false;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay;
    juce::AudioBuffer<float> dryBuffer;

    /** Linear dry/wet law with a 50 ms ramp -- DryWetMixer's defaults. */
    juce::SmoothedValue<float> mixSmoothed;
    float targetMix = 0.0f;

    /** v1.30.1: the wet path sleeps while the mix is ~0 (distMix defaults to 0,
        so an un-bypassed default patch would otherwise pay 4x oversampling for
        nothing). Asleep, only the dry path runs -- so the output still carries
        the reported latency, where v1.30.0 skipped the whole stage and jumped
        59.5 samples early. On waking, the reset oversampler emits zeros for its
        latency, then the signal starts mid-cycle; ramping the wet in over that
        onset is a step, so the mix is HELD at 0 until the wet path is primed. */
    bool wetAsleep = true;
    int wakeHoldRemaining = 0;

    /** Previous oversampled input sample per channel, for the antialiased Fold
        (IN-09). Tracked for EVERY distortion type, not only Fold, so that
        switching into Fold never differences against a sample left over from
        whenever Fold last ran. Cleared by `prepare()` and `reset()`.
    */
    double adaaPrevU[kNumChannels] { 0.0, 0.0 };

    int distType = 0;
    float driveAmount = 0.0f;
};
