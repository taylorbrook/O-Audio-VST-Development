/*
  ==============================================================================

    O-simpleSampler - Visualization Tap (real-time-safe spectrum + scope +
    closed-form filter magnitude curve)

    PORTED VERBATIM from O-simpleSubtractive/Source/SubVizAnalyzer.h (Phase 2.2a).
    Landed now for the Stage-3 headline filter curve: the audio thread already
    publishes the lead-voice `displayCutoffHz` and `displayK` (= 1/Q = JUCE's R2)
    atomics, and SubFilterCurve::magnitudeDb below evaluates EXACTLY the magnitude
    the running juce::dsp::StateVariableTPTFilter produces (k = 1/Q), so the curve
    matches the audio by construction (QUAL-02). For O-simpleSampler the filter is a
    single 12 dB/oct low-pass → call with type=0 (LP), slope=1 (12 dB). The FFT/scope
    ring is reused as-is; the WebView emit is wired in Stage 3.

    PERF-01: the audio thread is COPY-ONLY into a pre-allocated lock-free ring;
    NO allocation, NO FFT, NO locks on the audio thread. The FFT + curve run on
    the message thread (editor Timer, 30 Hz — wired in Stage 3).

    Three pieces:
      - VizRing       : audio-thread writer / message-thread reader ring (verbatim
                        from O-simpleFM FmVizAnalyzer).
      - computeMagnitudeDb : the CLOSED-FORM SVF magnitude (Ω = tan(πf/fs)/g) using
                        the SAME g/k/type/slope math the audio thread uses → the
                        headline filter curve matches the running filter exactly
                        (QUAL-02 by construction). Callable from the render-harness
                        to validate curve-vs-measured.
      - SubVizAnalyzer : message-thread FFT (4096 / Blackman-Harris) + scope
                        downsampler + filter-curve evaluator.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include <cmath>

//==============================================================================
// Lock-free overwrite ring. Power-of-two size + bitmask wrap.
class VizRing
{
public:
    static constexpr int kSize = 8192;           // >= FFT 4096 + scope window
    static_assert ((kSize & (kSize - 1)) == 0, "VizRing size must be power of two");

    void write (const float* data, int n) noexcept            // AUDIO THREAD
    {
        int w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            buffer[(size_t) w].store (data[i], std::memory_order_relaxed);
            w = (w + 1) & (kSize - 1);
        }
        writePos.store (w, std::memory_order_release);
    }

    void readLatest (float* dest, int n) const noexcept        // MESSAGE THREAD
    {
        const int w = writePos.load (std::memory_order_acquire);
        int start = (w - n) & (kSize - 1);
        for (int i = 0; i < n; ++i)
            dest[i] = buffer[(size_t) ((start + i) & (kSize - 1))].load (std::memory_order_relaxed);
    }

private:
    std::array<std::atomic<float>, (size_t) kSize> buffer {};
    std::atomic<int> writePos { 0 };
};

//==============================================================================
// Closed-form SVF magnitude in dB at display frequency `f`, for a filter whose
// cutoff is `fcDisplay`, resonance coefficient `k = 1/Q`, given type/slope.
// Uses Ω = tan(πf/fs)/g with g = tan(π·fcDisplay/fs) — the prewarped normalized
// frequency that makes the response EQUAL the running digital filter (peak lands
// exactly on cutoff, no warping error). Type/slope match SubVoice's enums.
namespace SubFilterCurve
{
    // type: 0=LP 1=HP 2=BP 3=Notch ; slope: 0=6dB 1=12dB 2=24dB
    inline double magnitudeDb (double f, double fcDisplay, double k,
                               int type, int slope, double fs) noexcept
    {
        f         = juce::jlimit (1.0, 0.5 * fs - 1.0, f);
        fcDisplay = juce::jlimit (20.0, 0.45 * fs, fcDisplay);

        const double g  = std::tan (juce::MathConstants<double>::pi * fcDisplay / fs);
        const double tf = std::tan (juce::MathConstants<double>::pi * f / fs);
        const double W  = tf / g;            // normalized frequency Ω

        double mag;
        if (slope == 0) // 6 dB — one pole (no resonance); BP/Notch degrade to 12 dB
        {
            const double d1 = std::sqrt (1.0 + W * W);
            if (type == 0)      mag = 1.0 / d1;       // LP
            else if (type == 1) mag = W / d1;         // HP
            else
            {
                // matches SubVoice's 6 dB BP/Notch fallback to a single SVF stage
                const double dd = std::sqrt ((1.0 - W * W) * (1.0 - W * W) + (k * W) * (k * W));
                mag = (type == 2) ? (k * W) / dd : std::abs (1.0 - W * W) / dd;
            }
        }
        else
        {
            const double dd = std::sqrt ((1.0 - W * W) * (1.0 - W * W) + (k * W) * (k * W));
            if (type == 0)      mag = 1.0 / dd;                       // LP
            else if (type == 1) mag = (W * W) / dd;                   // HP
            else if (type == 2) mag = (k * W) / dd;                   // BP (unity peak)
            else                mag = std::abs (1.0 - W * W) / dd;    // Notch

            if (slope == 2) mag *= mag;   // 24 dB = two identical cascaded stages
        }

        return juce::Decibels::gainToDecibels (mag + 1.0e-9, -120.0);
    }
}

//==============================================================================
class SubVizAnalyzer
{
public:
    static constexpr int kFftOrder     = 12;          // 4096
    static constexpr int kFftSize      = 1 << kFftOrder;
    static constexpr int kScopeWindow  = 1024;
    static constexpr int kScopePoints  = 128;
    static constexpr int kSpectrumBins = 256;
    static constexpr int kCurveBins    = 256;         // filter-curve display bins (log-f)

    SubVizAnalyzer()
        : fft (kFftOrder),
          window ((size_t) kFftSize, juce::dsp::WindowingFunction<float>::blackmanHarris)
    {
        spectrum.assign (kSpectrumBins, -100.0f);
        scope.assign    (kScopePoints, 0.0f);
        curve.assign    (kCurveBins, -60.0f);
    }

    // Message thread (editor Timer, Stage 3). Reads the ring, fills scope + spectrum.
    void process (const VizRing& ring, double sampleRate) noexcept
    {
        // Scope FIRST — performFrequencyOnlyForwardTransform clobbers its buffer.
        ring.readLatest (scopeRaw.data(), kScopeWindow);
        const int per = kScopeWindow / kScopePoints;
        for (int i = 0; i < kScopePoints; ++i)
        {
            float m = 0.0f;
            for (int j = 0; j < per; ++j)
            {
                const float s = scopeRaw[(size_t) (i * per + j)];
                if (std::abs (s) > std::abs (m)) m = s;
            }
            scope[(size_t) i] = juce::jlimit (-1.0f, 1.0f, m);
        }

        ring.readLatest (work.data(), kFftSize);
        window.multiplyWithWindowingTable (work.data(), (size_t) kFftSize);
        fft.performFrequencyOnlyForwardTransform (work.data());

        const float minHz = 20.0f;
        const float maxHz = (float) (sampleRate * 0.5);
        for (int b = 0; b < kSpectrumBins; ++b)
        {
            const float frac = (float) b / (float) (kSpectrumBins - 1);
            const float hz   = minHz * std::pow (maxHz / minHz, frac);
            const int   bin  = juce::jlimit (0, kFftSize / 2 - 1,
                                             (int) std::round (hz * (float) kFftSize / (float) sampleRate));
            const float mag  = work[(size_t) bin] / (float) kFftSize;
            const float db   = juce::Decibels::gainToDecibels (mag + 1.0e-9f, -100.0f);

            const float prev  = spectrum[(size_t) b];
            const float coeff = (db > prev) ? 0.5f : 0.1f;
            spectrum[(size_t) b] = prev + coeff * (db - prev);
        }

        frameCount.fetch_add (1, std::memory_order_relaxed);
    }

    // Evaluate the closed-form filter curve from the live lead-voice atomics.
    void updateCurve (double cutoffHz, double k, int type, int slope, double sampleRate) noexcept
    {
        const double minHz = 20.0, maxHz = sampleRate * 0.5;
        for (int b = 0; b < kCurveBins; ++b)
        {
            const double frac = (double) b / (double) (kCurveBins - 1);
            const double hz   = minHz * std::pow (maxHz / minHz, frac);
            curve[(size_t) b] = (float) SubFilterCurve::magnitudeDb (hz, cutoffHz, k, type, slope, sampleRate);
        }
    }

    const std::vector<float>& getSpectrum() const noexcept { return spectrum; }
    const std::vector<float>& getScope()    const noexcept { return scope; }
    const std::vector<float>& getCurve()    const noexcept { return curve; }
    int  getFrameCount() const noexcept { return frameCount.load (std::memory_order_relaxed); }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, (size_t) kFftSize * 2> work {};
    std::array<float, (size_t) kScopeWindow> scopeRaw {};
    std::vector<float> spectrum, scope, curve;
    std::atomic<int> frameCount { 0 };
};
