/*
  ==============================================================================

    O-simpleGrain - Grain Voice (one polyphonic granular voice)

    A juce::SynthesiserVoice that owns a preallocated bounded grain pool
    (std::array<Grain, kMaxGrainsPerVoice>), a per-sample scheduler, a per-voice
    juce::Random, and a juce::ADSR amp envelope. Each held MIDI note transposes
    the grain cloud (key-tracked resample); grains read windowed fragments from
    the shared static source buffer and are overlap-added into the output.

    Phase 2.1: core grain engine + overlap-add + amp ADSR + key resample.
    Phase 2.2 (this file): per-voice spray & scatter via the per-voice juce::Random
    (position/pitch/period/pan), velToDensity into the effective density, and the
    anti-aliasing one-pole filled in the grain read loop. The processor advances a
    moving/freezable global playhead and pushes its block-start value via
    setPlayhead — the voice spawn signature is unchanged from 2.1.

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

    // --- Phase 2.2 spray & scatter (per-grain RNG; processor pushes these) ---
    float positionSpray = 0.0f;  // 0..100 % of source length — read-start jitter
    float pitchSpray    = 0.0f;  // 0..12 st — ± per-grain rate jitter
    float scatter       = 0.0f;  // 0..100 % — scheduler period jitter (sync↔async)
    float panSpray      = 0.0f;  // 0..100 % — per-grain stereo spread
    float velToDensity  = 0.0f;  // 0..100 % (stored 0..1) — velocity -> density depth

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
    // Block param-push from the processor (once per block). Carries the 2.1 core
    // params + the 2.2 spray/scatter/velToDensity fields. Voices never touch APVTS.
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
    // per block — the block-start position of the moving/freezable global read
    // head. Grains spawned this block read from here (± position spray). The
    // processor advances/freezes the playhead per sample; the voice only reads
    // this per-block snapshot, so the spawn signature is unchanged from 2.1.
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
        // (0..1 depth) makes harder notes spawn a denser cloud: a note above the
        // mid-velocity (0.5) thickens, below thins. velLevel is fixed at note-on,
        // so the base interval is constant across the block; scatter jitters the
        // ACTUAL interval per spawn (below) — the sync↔async axis (DSP-05).
        const float effectiveDensity = juce::jlimit (1.0f, 200.0f,
            params.density * (1.0f + params.velToDensity * (velLevel - 0.5f) * 2.0f));
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
                // Scatter (RESEARCH §2.6): jitter the period ± scatter% of the
                // base interval. 0% = constant period (synchronous → discrete
                // sidebands); 100% = ±full random (asynchronous → noise).
                const float jitter = (params.scatter * 0.01f) * baseInterval
                                   * (rng.nextFloat() * 2.0f - 1.0f);
                samplesUntilNextGrain = juce::jmax (1, (int) (baseInterval + jitter));
            }

            // --- Overlap-add the active grains -------------------------------
            float outL = 0.0f, outR = 0.0f;

            for (auto& g : grains)
            {
                if (! g.active)
                    continue;

                const float env = (windowLuts != nullptr) ? windowLuts->read (g.shape, g.phase) : 0.0f;
                float src = readSourceLagrange (sourcePtr, sourceLen, g.readPos);
                src = aaOnePole (src, g.rate, g.aaState);   // band-limit if rate > 1 (DSP-08)

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
        g.shape         = params.windowShape;

        // --- Position spray: scatter the read start ± positionSpray% of the
        // source length around the current playhead. Under freeze the playhead is
        // pinned, so spray still scatters reads WITHIN the frozen region → a
        // frozen pad shimmers rather than buzzing on one grain (FUNC-03).
        const float posSprayRand = (rng.nextFloat() * 2.0f - 1.0f)
                                 * (params.positionSpray * 0.01f) * (float) sourceLen;
        g.readPos = playheadPos + posSprayRand;

        // --- Pitch spray: fresh ± pitchSpray semitones per grain, combined with
        // the key rate and the global grainPitch offset (decision #2 — all three
        // multiply into the read increment). rate > 1 = up-transposed (the AA
        // one-pole engages on the read; see aaOnePole).
        const float pitchSprayRand = (rng.nextFloat() * 2.0f - 1.0f) * params.pitchSpray;
        g.rate = voiceRate * std::pow (2.0f, (params.grainPitch + pitchSprayRand) / 12.0f);

        // --- Pan spray: equal-power pan scattered ± panSpray% around centre.
        g.pan = juce::jlimit (0.0f, 1.0f,
            0.5f + (rng.nextFloat() * 2.0f - 1.0f) * (params.panSpray * 0.01f) * 0.5f);

        // AA one-pole state primed to the first read sample so the filter starts
        // settled (no spurious attack transient on up-transposed grains).
        g.aaState = readSourceLagrange (sourcePtr, sourceLen, g.readPos);

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

    // Anti-aliasing one-pole (RESEARCH §5 / DSP-08). Band-limits up-transposed
    // grains (rate > 1) before the fractional read crosses Nyquist; bypassed at
    // rate <= 1 (no down-transposition aliasing). Cutoff tracks playback rate:
    // fc = 0.5*fs/rate (Nyquist of the source scaled by the read rate). One-pole
    // y += g*(x - y), g = 1 - exp(-2π*fc/fs). `state` is the per-grain aaState
    // (primed to the first read sample on spawn). Keeps high pitch-spray grains
    // clean without global oversampling — zero added latency.
    float aaOnePole (float x, float rate, float& state) const noexcept
    {
        if (rate <= 1.0f)
        {
            state = x;                          // keep state coherent for the bypass→engage edge
            return x;
        }

        const float fc = 0.5f * (float) sampleRate / rate;
        const float g  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sampleRate);
        state += g * (x - state);
        return state;
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
