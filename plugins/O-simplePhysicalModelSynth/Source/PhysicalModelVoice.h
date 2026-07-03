/*
  ==============================================================================

    O-simplePhysicalModelSynth - Physical Model Voice

    One polyphonic voice of the physical-modeling synth: a swappable exciter
    (Pluck / Strike / Bow) drives one of two resonators (Karplus-Strong String /
    Modal bank). The same per-sample excitation e[n] feeds whichever resonator is
    selected, so cross-driving (any exciter → any resonator) falls out for free.

    Stage 2.1 (this revision): Pluck → String is live (first audio). Strike, Bow,
    and the Modal resonator land in 2.2 / 2.3.

    Naming guard: PhysicalModelVoice / PhysicalModelSound deliberately do NOT
    shadow any juce:: type (the SamplerVoice/SamplerSound ambiguity hazard).

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL custom prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast. Header-only ⇒ no extra .cpp in the render-harness.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "StringResonator.h"
#include "ModalResonator.h"
#include "PluckExciter.h"
#include "StrikeExciter.h"
#include "BowExciter.h"
#include "VizTap.h"
#include <atomic>

//==============================================================================
// Per-block parameter snapshot pushed from the processor (read once via
// getRawParameterValue()->load(), never per-sample APVTS access on the audio
// thread). `damping`/`decay` are the EFFECTIVE values (after the Material macro).
struct PhysicalModelParams
{
    int   excitationType     = 0;      // 0 Pluck, 1 Strike, 2 Bow
    int   resonatorType      = 0;      // 0 String, 1 Modal
    float excitationPosition = 25.0f;  // %
    float excitationColor    = 60.0f;  // %
    float bowForce           = 50.0f;  // %
    float inharmonicity      = 0.0f;   // %  (modal)
    float modeBrightness     = 50.0f;  // %  (modal)
    float damping            = 60.0f;  // %
    float decay              = 70.0f;  // %
    int   coarseTune         = 0;      // semitones
    float fineTune           = 0.0f;   // cents
    float ampAttack          = 0.001f; // s
    float ampRelease         = 0.2f;   // s
    float velToBrightness    = 60.0f;  // %
};

//==============================================================================
class PhysicalModelSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class PhysicalModelVoice : public juce::SynthesiserVoice
{
public:
    PhysicalModelVoice() = default;
    ~PhysicalModelVoice() override = default;

    void setVoiceIndex (int idx) noexcept { voiceIndex = idx; }
    void setVizTap (VizTap* t) noexcept { vizTap = t; }
    void setTriggerCounter (std::atomic<juce::uint64>* c) noexcept { triggerCounter = c; }

    juce::uint64 getTriggerSeq() const noexcept { return triggerSeq; }

    // Called by the processor on the lead (most-recently-triggered) voice each block.
    // Audio-thread copy-only: scalar loop energy + the 8 modal (f_k, amp_k) stems.
    void publishViz() noexcept
    {
        if (vizTap == nullptr) return;
        vizTap->publishLoopEnergy (loopEnergyEst);
        vizTap->publishStems (modal.getModeFreqs(), modal.getModeAmps());
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<PhysicalModelSound*> (sound) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare, dispatched from the processor's prepareToPlay
    // via dynamic_cast. Sizes the delay line / resets filters at the real rate.
    void prepareToPlay (double sr, int maxBlockSize)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        string.prepare (sr, maxBlockSize);
        modal.prepare  (sr);
        pluck.prepare  (sr, voiceIndex);
        strike.prepare (sr);
        bow.prepare    (sr, voiceIndex);
        ampEnv.setSampleRate (sr);
    }

    //==========================================================================
    // Block-start param push from the processor. Stores the snapshot and applies
    // the values that the resonator/exciter read continuously (damping, decay).
    void setParams (const PhysicalModelParams& p) noexcept
    {
        params = p;

        string.setDamping (p.damping);
        string.setDecay   (p.decay);

        // Bow is sustained — let live Bow Force / Color changes apply mid-note.
        bow.setParams (p.bowForce / 100.0f, velLevel,
                       juce::jlimit (0.0f, 100.0f, p.excitationColor) / 100.0f);

        if (isVoiceActive())
        {
            const float f0 = computeF0();
            string.setFrequency (f0);
            modal.setParams (f0, p.decay, p.inharmonicity, p.modeBrightness);   // live modal updates
        }

        ampParams.attack  = juce::jmax (0.0f,    p.ampAttack);
        ampParams.decay   = 0.0f;
        ampParams.sustain = 1.0f;                                   // resonator provides decay
        ampParams.release = juce::jmax (0.005f,  p.ampRelease);
        ampEnv.setParameters (ampParams);
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int pitchWheel) override
    {
        currentNote   = midiNote;
        velLevel      = juce::jlimit (0.0f, 1.0f, velocity);
        pitchWheelPos = pitchWheel;
        noteHeld      = true;
        loopEnergyEst = 0.0f;
        if (triggerCounter != nullptr)
            triggerSeq = triggerCounter->fetch_add (1, std::memory_order_relaxed);  // lead-voice ordering

        const float f0 = computeF0();

        string.reset();
        string.setFrequency (f0);
        string.setDamping (params.damping);
        string.setDecay   (params.decay);

        modal.reset();
        modal.setParams (f0, params.decay, params.inharmonicity, params.modeBrightness);

        triggerExciter (f0);

        ampParams.attack  = juce::jmax (0.0f,   params.ampAttack);
        ampParams.decay   = 0.0f;
        ampParams.sustain = 1.0f;
        ampParams.release = juce::jmax (0.005f, params.ampRelease);
        ampEnv.setParameters (ampParams);
        ampEnv.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        noteHeld = false;              // Bow lifts → string rings out instead of being driven
        if (allowTailOff)
            ampEnv.noteOff();          // tail rings out; freed when the env goes idle
        else
            clearCurrentNote();        // hard steal
    }

    void pitchWheelMoved (int newValue) override { pitchWheelPos = newValue; }
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out,
                          int startSample, int numSamples) override
    {
        if (! ampEnv.isActive())
            return;

        const int  numCh   = out.getNumChannels();
        const bool isBow   = (params.excitationType == kBow);
        const bool isModal = (params.resonatorType  == kModal);
        const bool bowing  = isBow && noteHeld;     // drive while held; ring out once lifted

        for (int i = 0; i < numSamples; ++i)
        {
            // The SAME excitation e drives whichever resonator is selected
            // (cross-driving, FUNC-04). Pluck/Strike seed a decaying burst; Bow feeds
            // continuous friction-noise while held, then 0 (string/modes ring out).
            const float e = isBow ? (bowing ? bow.drive() : 0.0f) : exciterSample();

            float dry;
            if (isModal)
                dry = modal.processSample (e * (isBow ? kModalBowDrive : kModalDrive));
            else
                dry = string.processAdditive (isBow ? e * kBowDrive : e);

            const float env = ampEnv.getNextSample();
            const float s   = dry * env;

            // KS circulating-energy estimate for the loop-diagram viz (UI-02).
            loopEnergyEst += 0.001f * (std::abs (dry) - loopEnergyEst);

            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, s);

            if (! ampEnv.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    // Excitation-type indices (mirror the APVTS "Pluck/Strike/Bow" choice order).
    static constexpr int kPluck = 0, kStrike = 1, kBow = 2;
    // Resonator-type indices (mirror the APVTS "String/Modal" choice order).
    static constexpr int kString = 0, kModal = 1;

    // Modal drive scaling: the constant-skirt bandpass peak gain is Q, but a short
    // exciter burst only briefly excites it, so the bank needs a healthy drive to
    // ring audibly while staying bounded (the isfinite + Q-clamp guards backstop it).
    static constexpr float kModalDrive    = 0.5f;    // Pluck / Strike burst → Modal
    static constexpr float kModalBowDrive = 5.0f;    // Bow friction-noise → Modal (sustains)

    // Excitation drive into the resonator. Tuned so a default-velocity pluck is
    // clearly audible (RMS well above the −34 dB makes-sound floor) while staying
    // bounded well under full-scale at max velocity / max decay.
    static constexpr float kExciterDrive = 1.4f;

    // Continuous bow noise-drive gain into the KS loop (sustained excitation builds
    // up over the loop, so it needs far less per-sample amplitude than a one-shot burst).
    static constexpr float kBowDrive = 10.0f;

    //==========================================================================
    float computeF0() const noexcept
    {
        const float semis = static_cast<float> (currentNote) - 69.0f
                          + static_cast<float> (params.coarseTune)
                          + params.fineTune / 100.0f;
        return 440.0f * std::pow (2.0f, semis / 12.0f);
    }

    // Excitation Color 0–100% → brightness cutoff (log), raised by velocity with
    // depth = velToBrightness (shared across all exciters in later phases).
    float exciterCutoff() const noexcept
    {
        const float c        = juce::jlimit (0.0f, 100.0f, params.excitationColor) / 100.0f;
        const float base     = 500.0f * std::pow (16000.0f / 500.0f, c);     // 500 Hz … 16 kHz
        const float depth    = juce::jlimit (0.0f, 100.0f, params.velToBrightness) / 100.0f;
        const float velMult  = 1.0f + 2.0f * depth * velLevel;               // harder = brighter
        return juce::jlimit (200.0f, 18000.0f, base * velMult);
    }

    void triggerExciter (float f0) noexcept
    {
        const float amp     = kExciterDrive * (0.3f + 0.7f * velLevel);      // velocity → loudness
        const float pos     = params.excitationPosition / 100.0f;
        const float color01 = juce::jlimit (0.0f, 100.0f, params.excitationColor) / 100.0f;
        const float cutoff  = exciterCutoff();                               // color + velocity → brightness

        switch (params.excitationType)
        {
            case kStrike:
                strike.reset();
                strike.trigger (f0, pos, color01, cutoff, amp);
                break;
            case kBow:
                bow.reset();
                bow.setParams (params.bowForce / 100.0f, velLevel, color01);
                break;
            default:   // kPluck
                pluck.reset();
                pluck.trigger (f0, pos, cutoff, amp);
                break;
        }
    }

    // Per-sample excitation for the non-Bow (additive) paths.
    float exciterSample() noexcept
    {
        return (params.excitationType == kStrike) ? strike.processSample()
                                                  : pluck.processSample();
    }

    //==========================================================================
    StringResonator string;
    ModalResonator   modal;
    PluckExciter     pluck;
    StrikeExciter    strike;
    BowExciter       bow;
    juce::ADSR             ampEnv;
    juce::ADSR::Parameters ampParams;

    PhysicalModelParams params;

    VizTap*                     vizTap         = nullptr;
    std::atomic<juce::uint64>*  triggerCounter = nullptr;
    juce::uint64                triggerSeq     = 0;
    float                       loopEnergyEst  = 0.0f;

    double sampleRate    = 44100.0;
    int    voiceIndex    = 0;
    int    currentNote   = 69;
    int    pitchWheelPos = 8192;
    float  velLevel      = 1.0f;
    bool   noteHeld      = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhysicalModelVoice)
};
