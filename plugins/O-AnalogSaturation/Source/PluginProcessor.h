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
    // v1.7.0: PEAK-HOLD between editor reads. The audio thread only ever raises
    // the value; the editor's 30 Hz timer takes it with exchange(-100). Before,
    // each block overwrote the last, so the meter saw only the final block of
    // every ~33 ms and dropped the peaks in between.
    std::atomic<float> inputLevelDB { -100.0f };   // Peak level in dB
    std::atomic<float> outputLevelDB { -100.0f };  // Peak level in dB
    static void raisePeak(std::atomic<float>& a, float db) noexcept
    {
        float prev = a.load(std::memory_order_relaxed);
        while (db > prev && ! a.compare_exchange_weak(prev, db, std::memory_order_relaxed)) {}
    }

    // ------------------------------------------------------------------------
    // v1.2.0 — the UI language. 0 = en, 1 = fr; v1.5.0 adds 2.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile (juce::String is not trivially copyable), so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec, three branches as of v1.5.0. languageIndex() maps anything
        the branches below do not name to 0, so a hand-edited session or an
        unexpected argument from the page degrades to English rather than being
        stored unvalidated.

        THE STORED FORM IS THE BCP-47 TAG, NOT THE INDEX, and the Simplified
        Chinese tag is spelled here exactly as the page's <option value> and
        i18n.js LANGUAGES spell it — one spelling crosses the whole boundary, so
        nothing has to translate between two of them. The two literals below are
        the ONLY occurrences of that tag in this file; the gate counts them,
        which is why this sentence does not spell it a third time.

        PURE ASCII, AND THAT IS THE CONTRACT. Not one Han character exists
        anywhere under Source/ — every Chinese string lives in
        ui/public/js/i18n.js, and the one in the markup is written as numeric
        character references. This header names the language by its tag, which
        is Latin. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Oversampling (MID=2x, HIGH=4x, LOW=none)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingMid;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingHigh;

    int currentQuality = 1;  // Track current quality mode (0=LOW, 1=MID, 2=HIGH)

    // v1.7.0: parameter atomics, looked up once in the constructor instead of by
    // string on every block.
    std::atomic<float>* intensityParam = nullptr;
    std::atomic<float>* modelParam     = nullptr;
    std::atomic<float>* qualityParam   = nullptr;
    std::atomic<float>* autogainParam  = nullptr;

    // v1.7.0: the oversamplers, the dry buffer and the intensity ramp are sized for
    // the block length prepareToPlay() announced. A host that then sends a LONGER
    // block (it happens: offline bounce, some hosts' first block) used to overrun
    // the oversamplers' internal buffers — a jassert in Debug, a heap write past
    // the end in Release. processBlock() now walks any block in chunks of at most
    // this many samples, so nothing on the audio thread ever outgrows its allocation.
    int preparedBlockSize = 512;

    // v1.7.0: INTENSITY drives both the per-model input gain and the dry/wet mix.
    // Read once per block and applied flat, a knob move or automation step jumped
    // both at the block boundary (a click ~290x the steady waveform curvature).
    // Ramp it per base-rate sample; the oversampled path reads the ramp at
    // i / osFactor.
    static constexpr float INTENSITY_SMOOTHING_SECONDS = 0.02f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> intensitySmoothed;
    std::vector<float> intensityRamp;

    // v1.7.0: a MODEL change used to swap the waveshaper and its tone filters on
    // one sample, into filter/hysteresis state left over from whenever that model
    // last ran. Now the incoming model's state is cleared and it is crossfaded in
    // against the outgoing one (both run for the fade). A further change arriving
    // mid-fade waits for the fade to finish, so the fade never restarts from a jump.
    static constexpr float MODEL_FADE_SECONDS = 0.01f;
    int currentModel = 0;
    int fadeFromModel = 0;
    float modelFadeTotal = 1.0f;   // fade length in base-rate samples
    float modelFadeDone  = 1.0f;   // >= modelFadeTotal means no fade in progress
    void resetModelState(int model);

    // v1.7.0: a QUALITY change swaps oversamplers with different latencies (0 / 49 /
    // 59.5 samples) and resets them, which cannot be crossfaded without combing. The
    // output is ducked to silence over 5 ms, the switch happens while silent, and
    // the output ramps back in over 5 ms.
    static constexpr float QUALITY_DUCK_SECONDS = 0.005f;
    // The ramp-in waits out the new path's latency first: the oversampler and the
    // dry delay were just reset, so for that many samples they emit the zeros they
    // were cleared to and then the signal starts mid-cycle — ramping over that
    // onset is a step.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> qualityDuck;
    int qualityDuckHold = 0;   // samples still held at zero after a switch
    void switchQuality(int quality);

    // IN-03: per-model drive/hardness/normalization tuning constants. Drive range is the
    // amount added to unity input gain as intensity sweeps 0→100% (drive = 1 + driveCurve(wetMix)*range).
    static constexpr float DIODE_DRIVE_RANGE        = 6.0f;
    static constexpr float DIODE_HARDNESS           = 0.7f;   // waveshaper knee exponent
    static constexpr float TRANSFORMER_DRIVE_RANGE  = 7.5f;
    static constexpr float TUBE_DRIVE_RANGE         = 4.5f;
    static constexpr float TUBE_OUTPUT_NORMALIZATION = 1.2f;  // recover level lost to asymmetric clip
    static constexpr float MAGNETIC_DRIVE_RANGE     = 3.0f;

    // v1.8.0: drive curve. Linear to 50% (identical to v1.7.0 there, including the
    // default), then bends up with matching slope so 100% reaches twice the old drive
    // range: f(w) = w + 4(w - 0.5)^2 above 0.5, f(1) = 2. drive = 1 + f(wetMix) * range.
    static float driveCurve(float wetMix) noexcept
    {
        const float over = juce::jmax(0.0f, wetMix - 0.5f);
        return wetMix + 4.0f * over * over;
    }

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
    int currentLatencySamples = 0;          // what the host is told (whole samples)
    int computeLatencyForQuality(int quality) const;

    // v1.7.0: the 4x cascade's latency is 59.5 samples, not a whole number. The dry
    // delay above is integer-only, so the dry copy sat half a sample early and the
    // dry/wet sum combed (-1.1 dB at 19 kHz, 50% intensity, 48 kHz). JUCE's own
    // integer-latency mode adds a Thiran allpass to the WET path, whose phase is
    // only exact near DC (still -0.8 dB at 19 kHz). Instead the dry path gets the
    // missing half sample from a linear-phase windowed-sinc FIR (32 taps, Kaiser
    // beta 6: 15.5 samples of exact delay, flat to +/-0.01 dB below 21 kHz), and
    // the integer delay line supplies the rest. The host is told the rounded total.
    static constexpr int HALF_FIR_TAPS = 32;
    std::array<float, HALF_FIR_TAPS> halfFirCoeffs {};
    std::vector<std::array<float, 2 * HALF_FIR_TAPS>> halfFirHistory;  // mirrored ring
    int halfFirPos = 0;
    bool dryUsesHalfFir = false;
    int dryIntegerDelay = 0;
    float oversamplerLatency(int quality) const;   // exact, may be fractional
    void configureDryPath(int quality);

    // WR-01: setLatencySamples() triggers updateHostDisplay(), which is not real-time safe
    // from the audio callback. On a Quality change the audio thread stores the new latency
    // and triggers this AsyncUpdater; the actual host notification happens on the message
    // thread in handleAsyncUpdate().
    std::atomic<int> pendingLatencySamples { 0 };
    void handleAsyncUpdate() override;

    // Processing helpers
    float calculatePeakDB(const juce::AudioBuffer<float>& buffer);
    void captureInputRMS(const juce::AudioBuffer<float>& buffer);
    void processChunk(juce::dsp::AudioBlock<float> chunk);
    void processWet(juce::dsp::AudioBlock<float>& block, int osFactor);
    float processSample(float input, int model, float intensity, int channel);
    void mixDryWet(juce::dsp::AudioBlock<float>& wet);
    void applyAutoGain(juce::AudioBuffer<float>& buffer, bool enabled);

    // Saturation model implementations
    float processDiodeSample(float input, float intensity);
    float processTransformerSample(float input, float intensity, int channel);
    float processTubeSample(float input, float intensity, int channel);
    float processMagneticSample(float input, float intensity, int channel);
    float langevinFunction(float x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OAnalogSaturationAudioProcessor)
};
