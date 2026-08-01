/*
   This file is part of O-AnalogSaturation, an Ouaricon Audio plugin.
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

    O-AnalogSaturation - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <atomic>

class OAnalogSaturationAudioProcessor : public juce::AudioProcessor,
                                        private juce::AsyncUpdater
{
public:
    OAnalogSaturationAudioProcessor();
    ~OAnalogSaturationAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-AnalogSaturation"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    // WR-05: the peak/shelf IIR filters ring and the magnetic model retains state, so the
    // output does not settle instantly — report a small tail so hosts don't truncate the
    // decay on offline bounce/freeze.
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // VU Meter levels (atomic for thread-safe access from editor)
    // Uses peak level like TapeAge (getMagnitude), not RMS
    std::atomic<float> inputLevelDB { -100.0f };   // Peak level in dB
    std::atomic<float> outputLevelDB { -100.0f };  // Peak level in dB

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Oversampling (MID=2x, HIGH=4x, LOW=none)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingMid;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingHigh;

    int currentQuality = 1;  // Track current quality mode (0=LOW, 1=MID, 2=HIGH)

    // IN-03: per-model drive/hardness/normalization tuning constants. Drive range is the
    // amount added to unity input gain as intensity sweeps 0→100% (drive = 1 + wetMix*range).
    static constexpr float DIODE_DRIVE_RANGE        = 6.0f;
    static constexpr float DIODE_HARDNESS           = 0.7f;   // waveshaper knee exponent
    static constexpr float TRANSFORMER_DRIVE_RANGE  = 7.5f;
    static constexpr float TUBE_DRIVE_RANGE         = 4.5f;
    static constexpr float TUBE_OUTPUT_NORMALIZATION = 1.2f;  // recover level lost to asymmetric clip
    static constexpr float MAGNETIC_DRIVE_RANGE     = 3.0f;

    // TRANSFORMER model filters and parameters
    std::vector<juce::dsp::IIR::Filter<float>> transformerLFBumpFilters;
    std::vector<juce::dsp::IIR::Filter<float>> transformerHFSheenFilters;
    static constexpr float TRANSFORMER_CORE_SATURATION = 0.8f;

    // TUBE model filters
    std::vector<juce::dsp::IIR::Filter<float>> tubePresenceFilters;

    // MAGNETIC model (Jiles-Atherton hysteresis)
    std::vector<juce::dsp::IIR::Filter<float>> magneticHeadBumpFilters;
    std::vector<juce::dsp::IIR::Filter<float>> magneticHFRolloffFilters;

    // CR-01: These tone filters run INSIDE the oversampled nonlinear path, so their
    // coefficients must be designed at the rate that path actually executes at
    // (base * osFactor). Precompute one immutable coefficient set per Quality
    // (index 0=LOW/1x, 1=MID/2x, 2=HIGH/4x) and swap the active set when Quality
    // changes. Assigning a Coefficients::Ptr is a ref-count op (no allocation), so
    // the swap is real-time safe. Coefficient objects are shared across channels.
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> transformerLFBumpCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> transformerHFSheenCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> tubePresenceCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> magneticHeadBumpCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> magneticHFRolloffCoeffs;
    void applyQualityToneCoeffs(int quality);
    static float osFactorForQuality(int quality);  // 1 / 2 / 4 for LOW / MID / HIGH
    std::vector<float> magneticM;
    std::vector<float> magneticHPrev;
    static constexpr float MAGNETIC_MS = 1.0f;
    static constexpr float MAGNETIC_A = 0.4f;
    static constexpr float MAGNETIC_ALPHA = 0.01f;
    static constexpr float MAGNETIC_K = 0.2f;
    static constexpr float MAGNETIC_C = 0.8f;

    // CR-01 addendum: the Jiles-Atherton integrator is per-sample, so an absolute
    // per-sample deltaH clamp behaves differently at 1x/2x/4x (a transient split across
    // more oversampled steps is clamped less), making MAGNETIC change character with
    // Quality. Express the clamp as a fixed max field-change-per-second by scaling the
    // base per-sample limit down by the oversampling factor, so the realized slew limit
    // is identical across Quality. Updated in applyQualityToneCoeffs().
    static constexpr float MAGNETIC_DELTAH_CLAMP_BASE = 0.3f;  // per-sample limit at base rate (LOW)
    float magneticDeltaHClamp = MAGNETIC_DELTAH_CLAMP_BASE;

    // IN-02: single shared small-argument threshold below which both the Langevin function
    // and its derivative switch to their series/limit forms (avoids catastrophic
    // cancellation in coth(x)-1/x and keeps L and L' consistent in the crossover window).
    static constexpr float LANGEVIN_TAYLOR_THRESHOLD = 1e-4f;

    // Auto-gain RMS envelopes
    std::vector<float> inputRMSEnvelope;
    std::vector<float> outputRMSEnvelope;
    double sampleRateHz = 48000.0;
    static constexpr float AUTOGAIN_TIME_CONSTANT_SECONDS = 0.1f;  // 100 ms
    // CR-02: the RMS envelopes update once per block, so the one-pole coefficient
    // must be derived from the ACTUAL block length (not a per-sample constant),
    // keeping the realized time constant ~100 ms regardless of host block size.
    float autoGainBlockCoeff(int numSamples) const;

    // WR-03: the compensation gain is computed once per block; applied as a flat multiply
    // it steps at block boundaries (zipper/click on transients). Ramp it per sample toward
    // the block's target with a short smoothing time instead.
    static constexpr float AUTOGAIN_SMOOTHING_SECONDS = 0.02f;  // 20 ms per-sample ramp
    std::vector<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>> autoGainSmoothed;

    // WR-02: keep a clean, base-rate dry copy and mix it in AFTER downsampling so the dry
    // path is not colored by the oversampler's anti-imaging/anti-aliasing FIRs. The dry
    // copy is delayed by the oversampler latency (dryDelay) so dry and wet stay
    // phase-aligned; at LOW quality the latency is 0 and the delay line is bypassed.
    juce::AudioBuffer<float> dryBuffer;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay;
    int currentLatencySamples = 0;
    int computeLatencyForQuality(int quality) const;

    // WR-01: setLatencySamples() triggers updateHostDisplay(), which is not real-time safe
    // from the audio callback. On a Quality change the audio thread stores the new latency
    // and triggers this AsyncUpdater; the actual host notification happens on the message
    // thread in handleAsyncUpdate().
    std::atomic<int> pendingLatencySamples { 0 };
    void handleAsyncUpdate() override;

    // Processing helpers
    float calculatePeakDB(const juce::AudioBuffer<float>& buffer);
    void captureInputRMS(const juce::AudioBuffer<float>& buffer);
    void processSaturationDirect(juce::AudioBuffer<float>& buffer, int model, float intensity);
    void processSaturationBlock(juce::dsp::AudioBlock<float>& block, int model, float intensity);
    float processSample(float input, int model, float intensity, int channel);
    void mixDryWet(juce::AudioBuffer<float>& wetBuffer, const juce::AudioBuffer<float>& dry, float intensity);
    void applyAutoGain(juce::AudioBuffer<float>& buffer, bool enabled);

    // Saturation model implementations
    float processDiodeSample(float input, float intensity);
    float processTransformerSample(float input, float intensity, int channel);
    float processTubeSample(float input, float intensity, int channel);
    float processMagneticSample(float input, float intensity, int channel);
    float langevinFunction(float x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OAnalogSaturationAudioProcessor)
};
