/*
  ==============================================================================

    O-simpleGrain - Grain Voice (one polyphonic granular voice)

    A juce::SynthesiserVoice that owns a preallocated bounded grain pool
    (std::array<Grain, kMaxGrainsPerVoice>), a per-sample scheduler, a per-voice
    juce::Random, and a juce::ADSR amp envelope. Each held MIDI note transposes
    the grain cloud (key-tracked resample); grains read windowed fragments from
    the shared static source buffer and are overlap-added into the output.

    Phase 2.1 scope: core grain engine + overlap-add + amp ADSR + key resample.
    The read head is a static resting point the processor pushes per block
    (setPlayhead); scan/freeze motion lands in 2.2. Spray/scatter/AA are wired as
    pass-throughs here so 2.2 fills them without re-touching the hot loop.

    Real-time safety (every line of renderNextBlock): NO allocation / lock / I/O.
    Preallocated pool; steal-oldest when full (never resize). Per-voice Random.
    ADSR setSampleRate BEFORE setParameters in the (non-virtual) prepareToPlay.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleFM / O-Bassoon pattern).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include "PluginProcessor.h"            // engine constants (kMaxGrainsPerVoice, kRootNote)
#include "GrainSound.h"
#include "dsp/Grain.h"
#include "dsp/WindowLuts.h"
#include "dsp/LagrangeInterpolation.h"

//==============================================================================
// Block-pushed parameter bundle. Defined generously so Phase 2.2 can add
// spray/scatter/velToDensity fields with low churn (the voice ignores any field
// it does not yet use). Voices never touch the APVTS — the processor reads it
// once per block and calls setParams(...).
struct GrainVoiceParams
{
    float grainSizeMs = 30.0f;   // 2..200 ms
    float density     = 40.0f;   // grains/sec (clamped >= 1 by the scheduler)
    int   windowShape = 4;       // 0=rect .. 4=Hann
    float grainPitch  = 0.0f;    // global transposition in semitones

    // --- Phase 2.2 (declared now, UNUSED in 2.1) -------------------------
    // float positionSpray = 0.0f;  // % of source length
    // float pitchSpray    = 0.0f;  // ± semitones
    // float scatter       = 0.0f;  // % period jitter
    // float panSpray      = 0.0f;  // % stereo spread
    // float velToDensity  = 0.0f;  // velocity -> density depth

    juce::ADSR::Parameters amp { 0.01f, 0.3f, 0.8f, 0.4f };
};

//==============================================================================
class GrainVoice : public juce::SynthesiserVoice
{
public:
    GrainVoice() = default;
    ~GrainVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<GrainSound*> (sound) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare. setSampleRate MUST precede setParameters.
    void prepareToPlay (double sr, int /*maxBlockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        ampEnv.setSampleRate (sr);              // BEFORE setParameters (RT-safety gate 8)
        ampEnv.setParameters (ampParams);

        // Clear the pool so a sample-rate change never leaves stale grains running.
        for (auto& g : grains) g.active = false;
        nextGrain = 0;
        samplesUntilNextGrain = 0;
    }

    // Shared window-LUT table set (owned by the processor). Set once after
    // construction (a const pointer the read loop dereferences — no copy).
    void setWindowLuts (const WindowLuts* luts) noexcept { windowLuts = luts; }

    //==========================================================================
    // Block param-push from the processor (once per block). 2.1 fields only.
    void setParams (const GrainVoiceParams& p) noexcept
    {
        params = p;
        ampParams = p.amp;
        ampEnv.setParameters (ampParams);
    }

    // Source snapshot the processor sets once per block (it holds the shared_ptr
    // alive for the whole block; the voice only reads the raw pointer/len). A
    // null source -> the voice renders silence (no grains read).
    void setSource (const float* src, int len) noexcept
    {
        sourcePtr = src;
        sourceLen = len;
    }

    // Abstract "current playhead value" (in source samples) the processor sets
    // per block. 2.1 pushes the static resting point (position% * sourceLen);
    // 2.2 promotes this to a moving/freezable playhead WITHOUT changing the
    // voice's spawn signature.
    void setPlayhead (float posSamples) noexcept { playheadPos = posSamples; }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        // Key-tracked resample: held note transposes the cloud (C3 = recorded pitch).
        voiceRate = std::pow (2.0f, (float) (midiNote - OSimpleGrainAudioProcessor::kRootNote) / 12.0f);

        // Clear the pool — a new note starts a fresh cloud.
        for (auto& g : grains) g.active = false;
        nextGrain = 0;
        samplesUntilNextGrain = 0;               // fire a grain on the first sample

        velLevel = juce::jlimit (0.0f, 1.0f, velocity);
        ampEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();
            for (auto& g : grains) g.active = false;
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! ampEnv.isActive())
            return;

        const int numCh = out.getNumChannels();

        // Effective density -> base inter-grain interval (samples). velToDensity
        // is 2.2; for now effectiveDensity == density.
        const float effectiveDensity = juce::jlimit (1.0f, 200.0f, params.density);
        const float baseInterval = (float) sampleRate / effectiveDensity;

        // Grain length in samples (clamp >= 2 so phaseInc is finite).
        const float lenSamp = juce::jmax (2.0f, (params.grainSizeMs * 0.001f) * (float) sampleRate);
        const float phaseInc = 1.0f / lenSamp;

        for (int i = 0; i < numSamples; ++i)
        {
            // --- Scheduler: fire a grain when the countdown elapses -----------
            if (--samplesUntilNextGrain <= 0)
            {
                spawnGrain (lenSamp, phaseInc);
                // Scatter jitter is 2.2 — constant period for now (clamp >= 1).
                samplesUntilNextGrain = juce::jmax (1, (int) baseInterval);
            }

            // --- Overlap-add the active grains -------------------------------
            float outL = 0.0f, outR = 0.0f;

            for (auto& g : grains)
            {
                if (! g.active)
                    continue;

                const float env = (windowLuts != nullptr) ? windowLuts->read (g.shape, g.phase) : 0.0f;
                float src = readSourceLagrange (sourcePtr, sourceLen, g.readPos);
                src = aaOnePole (src, g.rate, g.aaState);   // 2.1: no-op pass-through (2.2 fills it)

                const float s = src * env;
                const float panL = std::cos (g.pan * juce::MathConstants<float>::halfPi);
                const float panR = std::sin (g.pan * juce::MathConstants<float>::halfPi);

                outL += s * panL;
                outR += s * panR;

                g.readPos += g.rate;
                g.phase   += g.phaseInc;
                ++g.age;
                if (g.phase >= 1.0f)
                    g.active = false;             // grain done
            }

            // --- Voice amplitude (amp ADSR * velocity) -----------------------
            const float ampVal = ampEnv.getNextSample() * velLevel;
            outL *= ampVal;
            outR *= ampVal;

            // Add into the output buffer (mono: sum L+R for a centred image).
            if (numCh >= 2)
            {
                out.addSample (0, startSample + i, outL);
                out.addSample (1, startSample + i, outR);
            }
            else if (numCh == 1)
            {
                out.addSample (0, startSample + i, 0.5f * (outL + outR));
            }
        }

        // Voice lifetime keyed on the amp envelope only.
        if (! ampEnv.isActive())
            clearCurrentNote();
    }

private:
    //==========================================================================
    // Spawn a grain: find an inactive slot, else steal the oldest (max age).
    // Bounded by kMaxGrainsPerVoice -> never allocates, never xruns (PERF-02).
    void spawnGrain (float lenSamp, float phaseInc) noexcept
    {
        constexpr int N = OSimpleGrainAudioProcessor::kMaxGrainsPerVoice;

        int target = -1, oldest = 0, maxAge = -1;
        for (int k = 0; k < N; ++k)
        {
            const int idx = (nextGrain + k) % N;
            if (! grains[(size_t) idx].active) { target = idx; break; }
            if (grains[(size_t) idx].age > maxAge) { maxAge = grains[(size_t) idx].age; oldest = idx; }
        }
        if (target < 0)
            target = oldest;                      // steal oldest

        auto& g = grains[(size_t) target];
        g.active        = true;
        g.age           = 0;
        g.phase         = 0.0f;
        g.phaseInc      = phaseInc;
        g.lengthSamples = lenSamp;
        g.aaState       = 0.0f;

        // Read start = the current playhead (position-only this phase; 2.2 adds
        // position spray). Grains advance independently from here by their rate.
        g.readPos = playheadPos;

        // Combined transposition: key rate * global grainPitch (pitch spray is 2.2).
        g.rate = voiceRate * std::pow (2.0f, params.grainPitch / 12.0f);

        // Centred pan (pan spray is 2.2).
        g.pan   = 0.5f;
        g.shape = params.windowShape;

        nextGrain = (target + 1) % N;
    }

    // Random-access fractional read into the static source (clamp at bounds —
    // a grain that runs off the end tapers out via its window, RESEARCH §2.5).
    static float readSourceLagrange (const float* src, int len, float pos) noexcept
    {
        if (src == nullptr || len <= 0)
            return 0.0f;

        const int   i0   = (int) pos;
        const float frac = pos - (float) i0;
        const int   im1  = juce::jlimit (0, len - 1, i0 - 1);
        const int   ip0  = juce::jlimit (0, len - 1, i0);
        const int   ip1  = juce::jlimit (0, len - 1, i0 + 1);
        const int   ip2  = juce::jlimit (0, len - 1, i0 + 2);
        return lagrangeInterpolate (src[im1], src[ip0], src[ip1], src[ip2], frac);
    }

    // Anti-aliasing one-pole. Phase 2.1: NO-OP pass-through (the call site is
    // wired in the hot loop so Phase 2.2 Task 8 only fills the body — it will
    // band-limit when rate > 1 via fc = 0.5*fs/rate). Keeping the signature and
    // call site fixed avoids re-touching the render loop in 2.2.
    float aaOnePole (float x, float /*rate*/, float& /*state*/) const noexcept
    {
        return x;
    }

    //==========================================================================
    double sampleRate = 44100.0;
    float  voiceRate  = 1.0f;            // key-tracked read-increment ratio
    float  velLevel   = 1.0f;

    // Source snapshot (raw pointer set per block; processor holds the shared_ptr).
    const float* sourcePtr = nullptr;
    int          sourceLen = 0;

    // Current playhead (resting point in 2.1; moving/freezable in 2.2).
    float playheadPos = 0.0f;

    // Preallocated bounded grain pool + scheduler state.
    std::array<Grain, OSimpleGrainAudioProcessor::kMaxGrainsPerVoice> grains {};
    int nextGrain = 0;
    int samplesUntilNextGrain = 0;

    // Per-voice RNG (no shared/global RNG — RT-safety gate 4). Unused in 2.1;
    // present so spray/scatter in 2.2 needs no new member.
    juce::Random rng;

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams { 0.01f, 0.3f, 0.8f, 0.4f };

    GrainVoiceParams params;
    const WindowLuts* windowLuts = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainVoice)
};
