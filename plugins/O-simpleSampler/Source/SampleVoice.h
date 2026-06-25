/*
  ==============================================================================

    O-simpleSampler - Sample Voice (one polyphonic Repitch read head)

    A custom juce::SynthesiserVoice (NOT juce::SamplerVoice) that plays the shared
    source buffer through a fractional-read varispeed ("Repitch") head: each held
    MIDI note sets a read increment (keyRatio) relative to the live Root Key, the
    head reads the source with 4-point Lagrange interpolation, an anti-alias
    one-pole band-limits up-transposed notes, and a per-voice juce::ADSR + a
    velocity-sensitivity blend (velToAmp) shape the amplitude. Confined to the
    [startSamp, endSamp) region (one-shot in Phase 2.1 — no loop yet).

    Phase 2.1 (this file): Repitch read head + region isolation + AA + amp ADSR +
    velToAmp. Phase 2.2 adds loop/reverse/Stretch/Vintage/filter (not here yet).

    NB: named SampleVoice, NOT SamplerVoice. juce::SamplerVoice exists in
    juce_audio_formats, and the generated JuceHeader.h does `using namespace
    juce;`, so a class literally named `SamplerVoice` is AMBIGUOUS at every
    unqualified use (same class of bug as the regionStart/regionEnd vs juce::end
    collision). Keep this name `Sampler`-free.

    Real-time safety (every line of renderNextBlock): NO allocation / lock / I/O.
    The voice holds a RAW const float* + int snapshot of the source (the processor
    owns the shared_ptr and keeps it alive for the whole block) — it never owns or
    resizes an AudioBuffer. juce::ScopedNoDenormals + a final std::isfinite scrub
    live on the processor side.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleGrain / O-simpleSubtractive pattern). ADSR setSampleRate
    MUST precede setParameters (JUCE-8 order gate).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "SampleSound.h"
#include "dsp/LagrangeInterpolation.h"   // global lagrangeInterpolate(ym1,y0,y1,y2,frac)

//==============================================================================
// Block-pushed parameter bundle. The processor reads the APVTS atomics once per
// block and calls setParams(...) — voices never touch the APVTS. Region bounds
// arrive as absolute sample indices in the SOURCE frame (the processor computes
// them from the start/end % of the published source length).
struct SamplerVoiceParams
{
    int   rootKey   = 60;       // live Root Key (MIDI) — keyRatio reference (NOT kRootNote)
    int   tune      = 0;        // coarse transpose (semitones)
    float fine      = 0.0f;     // fine transpose (cents)
    int   startSamp = 0;        // region start (samples, source frame)
    int   endSamp   = 0;        // region end   (samples, source frame, exclusive)
    float velToAmp  = 50.0f;    // velocity sensitivity 0–100 %
    juce::ADSR::Parameters amp; // amp ADSR (A/D/S/R)
};

//==============================================================================
class SampleVoice : public juce::SynthesiserVoice
{
public:
    SampleVoice() = default;
    ~SampleVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SampleSound*> (s) != nullptr;
    }

    //==========================================================================
    // Block param-push from the processor (once per block). Re-apply the amp ADSR
    // parameters live so envelope-time knobs track without a note retrigger.
    void setParams (const SamplerVoiceParams& p) noexcept
    {
        params = p;
        ampEnv.setParameters (p.amp);
    }

    // Source snapshot the processor sets once per block: a raw pointer + length
    // (the processor holds the owning shared_ptr alive for the whole block). The
    // voice only ever READS this — it never owns an AudioBuffer. Null → silence.
    void setSource (const float* src, int len) noexcept
    {
        sourcePtr = src;
        sourceLen = len;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare (dispatched by the processor via dynamic_cast).
    // setSampleRate MUST precede setParameters (RT-safety / JUCE-8 order gate).
    void prepareToPlay (double sr, int /*blockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        ampEnv.setSampleRate (sr);
        ampEnv.setParameters (params.amp);
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        const double fs = getSampleRate();

        // Repitch read-increment (keyRatio) relative to the LIVE Root Key + tune +
        // fine (never the kRootNote constant). 1.0 ⇒ the note that matches rootKey
        // plays at the recorded pitch; higher notes read faster (up-transpose).
        voiceRate = std::pow (2.0,
            (double) (midiNote - params.rootKey + params.tune + params.fine * 0.01) / 12.0);

        // Start the read head at the region start (one-shot in 2.1).
        readPos = (double) params.startSamp;

        // Velocity sensitivity blend (net-new, RESEARCH §8): v=0 ⇒ level-independent
        // of velocity (1.0); v=1 ⇒ full velocity; default 50 % ⇒ 0.5 + 0.5·velocity.
        const float v = juce::jlimit (0.0f, 1.0f, params.velToAmp * 0.01f);
        velLevel = (1.0f - v) + v * juce::jlimit (0.0f, 1.0f, velocity);

        // AA one-pole: engage only for up-transposed notes (voiceRate > 1). The
        // cutoff fc = 0.5·fs/rate and its smoothing coefficient depend only on the
        // (per-note-constant) rate, so both are computed ONCE here — no per-sample
        // std::exp in the render loop (RESEARCH §2.3 / DSP-02). State primed to the
        // first read sample so the filter starts settled (no attack transient).
        aaEngaged = (voiceRate > 1.0);
        aaCoeff   = aaEngaged
                  ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                           * (0.5f * (float) fs / (float) voiceRate) / (float) fs)
                  : 0.0f;
        aaState   = readSourceLagrange (sourcePtr, sourceLen, (float) readPos);

        ampEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();                 // release tail (click-free note-off)
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();                   // hard stop (voice steal)
        }
    }

    void pitchWheelMoved (int) override {}    // no bend in Phase 2.1
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        // Voice lifetime is keyed on the amp envelope (a finished release frees it).
        if (! ampEnv.isActive())
            return;

        // No usable source / empty region → free the voice (never read OOB).
        if (sourcePtr == nullptr || sourceLen <= 0 || params.endSamp <= params.startSamp)
        {
            ampEnv.reset();
            clearCurrentNote();
            return;
        }

        const int numCh = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            // Fractional Lagrange read → anti-alias one-pole (band-limits up-pitch).
            float s = readSourceLagrange (sourcePtr, sourceLen, (float) readPos);
            s = aaOnePole (s, aaCoeff, aaEngaged, aaState);

            // VCA: amp ADSR × velocity-sensitivity level.
            const float a = ampEnv.getNextSample() * velLevel;
            float outv = s * a;
            if (! std::isfinite (outv)) outv = 0.0f;

            // Mono source → duplicated across all output channels (centred image).
            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, outv);

            readPos += voiceRate;

            // One-shot region end (no loop in 2.1): stop at endSamp and free.
            if (readPos >= (double) params.endSamp)
            {
                ampEnv.reset();
                break;
            }
        }

        // Free the voice once the amp envelope has finished (release tail done, or
        // the region-end reset above).
        if (! ampEnv.isActive())
            clearCurrentNote();
    }

private:
    //==========================================================================
    // Random-access fractional read into the source (clamp at bounds — a read off
    // the region/loop boundary saturates to the edge sample, no OOB). Ported
    // verbatim from O-simpleGrain/Source/GrainVoice.h (RESEARCH §2.2).
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

    // Anti-aliasing one-pole (RESEARCH §2.3 / DSP-02). Band-limits up-transposed
    // reads before the fractional read crosses Nyquist. The cutoff, its smoothing
    // coefficient, and the engage decision are precomputed ON note-on (constant per
    // note — see startNote), so this hot-path helper is one branch + one
    // multiply-add, NO transcendental. When not engaged the read passes through and
    // `state` is kept coherent for a later engage edge. Ported verbatim from
    // O-simpleGrain/Source/GrainVoice.h.
    static float aaOnePole (float x, float coeff, bool engaged, float& state) noexcept
    {
        if (! engaged)
        {
            state = x;                        // keep state coherent for the bypass→engage edge
            return x;
        }

        state += coeff * (x - state);         // one-pole y += g*(x - y), g precomputed
        return state;
    }

    //==========================================================================
    juce::ADSR         ampEnv;
    SamplerVoiceParams params;

    // Source snapshot (raw pointer set per block; processor holds the shared_ptr).
    const float* sourcePtr = nullptr;
    int          sourceLen = 0;

    double voiceRate = 1.0;     // Repitch read-increment ratio (keyRatio)
    double readPos   = 0.0;     // current fractional source position
    float  velLevel  = 1.0f;    // velocity-sensitivity-blended amplitude

    // AA one-pole state (engage + coeff computed once per note in startNote).
    bool   aaEngaged = false;
    float  aaCoeff   = 0.0f;
    float  aaState   = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleVoice)
};
