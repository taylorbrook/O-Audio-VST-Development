/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    SpectralFreeze.h
    O-Freeze v2.9.0 - the Spectral engine (STFT freeze, 8192 / 4x, Hann x Hann).
    v2.9.1 - the engage work is spread over steps kPrimeSpacing samples apart.

    Analysis runs on the write-head clock (one frame per kHop samples written),
    resynthesis on the synth clock (one frame per kHop samples output). Every
    random draw is CounterRng(seed, counter, slot) on this class's own hop /
    engage counters, so a render is a pure function of the seed, the input and
    the parameters, whatever the host block size.

    No APVTS access, no allocation outside prepare(): the FFT object is built in
    the constructor and every buffer is sized for 2 channels in prepare().

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cstdint>
#include <vector>

class SpectralFreeze
{
public:
    static constexpr int kOrder = 13;
    static constexpr int kSize = 1 << kOrder;           // W = 8192
    static constexpr int kHop = kSize / 4;              // H = 2048
    static constexpr int kBins = kSize / 2 + 1;         // 4097
    static constexpr int kMaxAvg = 16;
    static constexpr int kMaxChannels = 2;
    static constexpr int kOlaSize = 2 * kSize;          // output ring (power of two)

    // v2.9.1 engage priming: one step every kPrimeSpacing samples on the synth clock
    // (fallback analyses kFallbackPerStep at a time, then average + IF, peaks, the
    // per-engage draws, and the 4 primed frames one each), so no host block of 32 or
    // more carries more than about one frame's work. Output starts on the last step,
    // through a kOnsetFadeSeconds linear ramp (the processor's 50 ms fade is already
    // a few ms in by then, so starting at full scale would step).
    static constexpr int kPrimeSpacing = 32;
    static constexpr int kFallbackPerStep = 2;
    static constexpr int kPrimeStages = 7;              // average, peaks, draws, 4 frames
    static constexpr double kOnsetFadeSeconds = 0.005;

    // RNG slots, passed in so the processor's RngSlot enum stays the one registry
    struct Slots { uint32_t phase, drift, engage; };

    /** Per-hop controls, sampled at the hop sample. */
    struct Controls
    {
        float beta = 0.0f;          // diffusion 0..1 (LFO already applied, clamped)
        float drift01 = 0.0f;       // DRIFT 0..1 -> per-bin walk depth
        float detuneCents = 0.0f;   // DETUNE half-width in cents
    };

    SpectralFreeze();

    void prepare(double sampleRate, int numChannels, Slots slots);

    /** New seed: both counters restart (same rule as the granular streams). */
    void setSeed(uint64_t seed) noexcept;

    /** Rolling analysis restarts (engine switched to Spectral, or prepare). */
    void restartAnalysis() noexcept;

    /** The write head has just advanced to writePosAfter. Every kHop advances this
        analyses the kSize samples that end there. */
    void onWriteAdvanced(const juce::AudioBuffer<float>& ring, int writePosAfter) noexcept;

    /** Engage: average avgFrames frames (on-demand fallback from the ring if the
        rolling analysis holds fewer), estimate IF / peaks, draw per-engage values,
        prime the OLA with 4 frames. v2.9.1: this only arms the steps; processSample()
        runs them (the ring must stay unwritten meanwhile: the write head is held while
        anything renders), with the controls given here for the primed frames. */
    void capture(const juce::AudioBuffer<float>& ring, int writePos, int avgFrames, const Controls& c) noexcept;

    /** Engaged (priming or sounding). */
    bool isRunning() const noexcept { return running; }
    void stop() noexcept { running = false; priming = false; }

    /** One output sample per channel (out[ch]); synthesises a frame every kHop. Silent
        while priming. */
    void processSample(float* out, const Controls& c) noexcept;

    /** Samples from capture() to the first output sample (the last prime step). */
    static constexpr int primeDelaySamples(int fallbackAnalyses) noexcept
    {
        return ((fallbackAnalyses + kFallbackPerStep - 1) / kFallbackPerStep + kPrimeStages - 1) * kPrimeSpacing;
    }

    // Read-only probes for the test harness (cheap, never used by the DSP)
    int getFramesValid() const noexcept { return framesValid; }
    uint64_t getHopCounter() const noexcept { return hopCounter; }
    uint64_t getEngageCounter() const noexcept { return engageCounter; }
    int getRollingFrames() const noexcept { return rollingFrames; }
    int getFallbackCaptures() const noexcept { return fallbackCaptures; }
    int getCaptures() const noexcept { return captures; }
    bool isPriming() const noexcept { return priming; }

    /** beta -> makeup gain (D2 half-compensation of the incoherent-overlap level loss). */
    static float makeupFor(float beta) noexcept;

    // D2 refit (milestone spectral-diffusion-freeze SUMMARY.md): the planned 0.5
    // (+3.01 dB at beta 1) left a chord at -2.97 dB re beta 0 and pink noise at
    // +1.60; 0.57 (+3.67 dB at beta 1) centres both inside +-3 dB (chord about
    // -2.1 to -2.5, pink +2.1 to +2.4 at beta 1).
    static constexpr float kMakeupAmount = 0.57f;
    // Spectral beta 0 already lands within 1 dB of the granular density law on the
    // gate stimulus (-0.3 dB, power mean of 6 granular seeds), so no trim.
    static constexpr float kSpectralTrim = 1.0f;

private:
    void analyseFrame(const juce::AudioBuffer<float>& ring, int endPos) noexcept;
    void primeStep() noexcept;
    void captureAverage() noexcept;   // magnitudes, phase, IF
    void capturePeaks() noexcept;     // peak regions, lock offsets
    void captureDraws() noexcept;     // per-engage detune / walk-rate draws
    void synthesiseFrame(int offset, const Controls& c) noexcept;
    float uniform(uint64_t counter, uint32_t slot) const noexcept;

    juce::dsp::FFT fft { kOrder };
    double sampleRate = 48000.0;
    int numChannels = kMaxChannels;
    Slots slots { 0, 0, 0 };
    uint64_t seed = 0;
    uint64_t hopCounter = 0;          // one per synthesised frame: phase noise, drift walk
    uint64_t engageCounter = 0;       // one per capture: detune factor, walk rate

    std::vector<float> window;        // periodic Hann, analysis = synthesis
    std::vector<float> work;          // 2 * kSize, the FFT's real-only buffer

    // Rolling analysis: power of the last kMaxAvg frames, complex of the last two
    std::vector<float> powerRing;     // [kMaxAvg][kMaxChannels][kBins]
    std::vector<float> specRing;      // [2][kMaxChannels][2 * kBins] interleaved re, im
    int ringHead = 0;                 // power slot the next frame writes
    int specLatest = 1;               // spec slot holding the newest frame
    int framesValid = 0;
    int analysisCountdown = kHop;
    int rollingFrames = 0;
    int fallbackCaptures = 0;
    int captures = 0;

    // Capture state
    std::vector<float> mag;           // [ch][k] averaged magnitude
    std::vector<double> omegaH;       // [ch][k] estimated phase advance per hop (rad)
    std::vector<float> phase;         // [ch][k] running phase, wrapped to (-pi, pi]
    std::vector<float> delta;         // [ch][k] phase offset from the bin's peak at capture
    std::vector<int> peakOf;          // [k] the peak that bin k is locked to (shared L/R)
    std::vector<float> power;         // [k] L+R power, peak picking scratch
    std::vector<int> peaks;           // peak list scratch
    std::vector<float> detuneR;       // [k] per-engage detune draw in [-1, 1)
    std::vector<float> walkA;         // [k] OU pole per hop
    std::vector<float> walkB;         // [k] sqrt(1 - a^2) * sqrt(3)
    std::vector<float> walk;          // [k] unit-variance OU state
    std::vector<float> gainK;         // [k] per-hop drift gain scratch
    std::vector<float> noiseK;        // [k] per-hop phase noise scratch
    std::vector<float> factorK;       // [k] per-hop detune factor scratch
    std::vector<float> advWrapped;    // [ch][k] princarg(omegaH), the no-detune advance
    std::vector<float> sinK, cosK;    // [k] spectrum-build scratch
    float dcValue[kMaxChannels] {};   // DC and Nyquist stay real (signed magnitudes)
    float nyqValue[kMaxChannels] {};

    // Overlap-add output
    std::vector<float> ola;           // [ch][kOlaSize]
    int olaPos = 0;
    int synthCountdown = 0;
    bool running = false;

    // Engage priming (v2.9.1)
    const juce::AudioBuffer<float>* primeRing = nullptr;
    int primeWritePos = 0;
    int primeAvg = 1;
    int primeAnalysesLeft = 0;        // fallback frames still to analyse
    int primeStage = 0;               // next of the kPrimeStages stages
    int primeCountdown = 0;
    Controls primeControls;
    bool priming = false;
    int onsetLength = 1;
    int onsetPos = 0;                 // onsetLength = ramp done
};
