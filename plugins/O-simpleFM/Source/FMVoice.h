/*
  ==============================================================================

    O-simpleFM - FM Voice (2-operator phase modulation)

    One polyphonic voice: a modulator operator (with DX7 self-feedback) phase-
    modulates a carrier operator; the carrier is shaped by an independent amp
    ADSR (which also governs voice lifetime). A second, independent mod ADSR
    scales the modulation index over the note.

    Convention: RADIANS internally (1:1 with Bessel/Chowning math). Never mix
    with normalized-turns. PM (modulator added to phase AFTER integration), not
    true FM — this is what keeps feedback stable.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL custom prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-Bassoon pattern).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Operator.h"

//==============================================================================
class FMSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class FMVoice : public juce::SynthesiserVoice
{
public:
    FMVoice() = default;
    ~FMVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<FMSound*> (sound) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare. setSampleRate MUST precede setParameters.
    void prepareToPlay (double sr, int /*maxBlockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        ampEnv.setSampleRate (sr);
        modEnv.setSampleRate (sr);
        ampEnv.setParameters (ampParams);
        modEnv.setParameters (modParams);

        indexSmoothed.reset    (sr, 0.02);
        feedbackSmoothed.reset (sr, 0.02);
        ratioSmoothed.reset    (sr, 0.02);
        ceilSmoothed.reset     (sr, 0.01);
        ceilSmoothed.setCurrentAndTargetValue (1.0e9f);
    }

    //==========================================================================
    // Block param-push from the processor (once per block).
    void setParams (float ratio, bool ratioSnap,
                    float indexNorm, float feedbackNorm,
                    bool  modFixedModeIn, float modFixedHzIn,
                    float modEnvDepth, float velToIndexAmt,
                    const juce::ADSR::Parameters& ap,
                    const juce::ADSR::Parameters& mp) noexcept
    {
        float r = ratioSnap ? std::round (ratio) : ratio;
        r = juce::jmax (0.001f, r);
        ratioSmoothed.setTargetValue (r);

        // Perceptual index taper: finer control in the dense low end.
        const float baseIndex = 20.0f * std::pow (juce::jlimit (0.0f, 1.0f, indexNorm), 1.7f);
        indexSmoothed.setTargetValue (baseIndex);

        // feedback 0..1 -> coeff max ~pi radians, x^1.5 taper (bottom stays gentle).
        const float fbCoeff = juce::MathConstants<float>::pi
                            * std::pow (juce::jlimit (0.0f, 1.0f, feedbackNorm), 1.5f);
        feedbackSmoothed.setTargetValue (fbCoeff);

        fixedMode  = modFixedModeIn;
        fixedHz    = modFixedHzIn;
        envDepth   = juce::jlimit (0.0f, 1.0f, modEnvDepth);
        velToIndex = juce::jlimit (0.0f, 1.0f, velToIndexAmt);

        ampParams = ap;  ampEnv.setParameters (ampParams);
        modParams = mp;  modEnv.setParameters (modParams);
    }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int pitchWheel) override
    {
        currentNote   = midiNote;
        pitchWheelPos = pitchWheel;
        updateCarrierFrequency();

        velLevel = juce::jlimit (0.0f, 1.0f, velocity);

        // Reset feedback history ONLY — never reset carrier/mod phase mid-life.
        fbPrev1 = fbPrev2 = 0.0f;

        ampEnv.noteOn();
        modEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
            modEnv.noteOff();
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();
            modEnv.reset();
            fbPrev1 = fbPrev2 = 0.0f;
        }
    }

    void pitchWheelMoved (int v) override { pitchWheelPos = v; updateCarrierFrequency(); }
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! ampEnv.isActive())
            return;

        const double twoPi   = juce::MathConstants<double>::twoPi;
        const double nyquist = 0.5 * sampleRate;
        const int    numCh   = out.getNumChannels();

        // Key-tracked index ceiling (Carson): highest component
        // fc + (I+1)*fm <= 0.9*Nyquist  =>  I <= (0.9*Nyq - fc)/fm - 1.
        // Computed once per render call (target ~constant within a block) so the
        // smoother ramps toward it instead of being re-armed every sample.
        const double fmCeil = fixedMode ? (double) fixedHz
                                        : (carrierHz * (double) ratioSmoothed.getTargetValue());
        float ceilTarget = 1.0e9f;
        if (fmCeil > 1.0e-6)
            ceilTarget = (float) juce::jmax (0.0, (0.9 * nyquist - carrierHz) / fmCeil - 1.0);
        ceilSmoothed.setTargetValue (ceilTarget);

        for (int i = 0; i < numSamples; ++i)
        {
            const float modEnvVal = modEnv.getNextSample();   // [0,1]
            const float ampEnvVal = ampEnv.getNextSample();   // [0,1]

            const double ratioCur = (double) ratioSmoothed.getNextValue();
            const double fm = fixedMode ? (double) fixedHz : (carrierHz * ratioCur);

            // --- Modulator with DX7 two-sample-average self-feedback --------
            const float fbCoeff = feedbackSmoothed.getNextValue();
            const float avg     = 0.5f * (fbPrev1 + fbPrev2);
            float modOut = OSimpleFM::fastSine ((float) modPhase + fbCoeff * avg);
            if (! std::isfinite (modOut)) modOut = 0.0f;      // kill NaN at source
            modOut = juce::jlimit (-1.0f, 1.0f, modOut);      // clamp HISTORY, not just output
            fbPrev2 = fbPrev1; fbPrev1 = modOut;

            modPhase += twoPi * fm / sampleRate;
            if      (modPhase >= twoPi) modPhase -= twoPi;
            else if (modPhase <  0.0)   modPhase += twoPi;

            // --- Index: base * mod-env (multiplicative) * velocity, ceiling --
            const float baseIndex = indexSmoothed.getNextValue();
            const float velFactor = (1.0f - velToIndex) + velToIndex * velLevel;
            const float iInst = baseIndex * ((1.0f - envDepth) + envDepth * modEnvVal) * velFactor;

            // Key-tracked index ceiling (Carson): highest component
            // fc + (I+1)*fm <= 0.9*Nyquist  =>  I <= (0.9*Nyq - fc)/fm - 1
            float ceil = 1.0e9f;
            if (fm > 1.0e-6)
                ceil = (float) ((0.9 * nyquist - carrierHz) / fm - 1.0);
            ceilSmoothed.setTargetValue (juce::jmax (0.0f, ceil));
            const float effIndex = juce::jmin (iInst, ceilSmoothed.getNextValue());

            // --- Carrier ----------------------------------------------------
            float carOut = OSimpleFM::fastSine ((float) carPhase + effIndex * modOut);
            carPhase += twoPi * carrierHz / sampleRate;
            if (carPhase >= twoPi) carPhase -= twoPi;

            const float sample = carOut * ampEnvVal * velLevel;

            for (int ch = 0; ch < numCh; ++ch)
                out.addSample (ch, startSample + i, sample);
        }

        // Voice lifetime keyed on AMP envelope only (a long mod release must
        // not keep a silent voice alive).
        if (! ampEnv.isActive())
        {
            clearCurrentNote();
            fbPrev1 = fbPrev2 = 0.0f;
        }
    }

private:
    void updateCarrierFrequency()
    {
        const double bendSemis = ((double) pitchWheelPos - 8192.0) / 8192.0
                               * 2.0; // +-2 semitone bend range
        const double base = juce::MidiMessage::getMidiNoteInHertz (currentNote);
        carrierHz = base * std::pow (2.0, bendSemis / 12.0);
    }

    double sampleRate = 44100.0;
    double carPhase   = 0.0, modPhase = 0.0;
    double carrierHz  = 440.0;
    int    currentNote = 69, pitchWheelPos = 8192;

    bool   fixedMode  = false;
    float  fixedHz    = 220.0f;
    float  envDepth   = 1.0f;
    float  velToIndex = 0.0f;
    float  velLevel   = 1.0f;

    float  fbPrev1 = 0.0f, fbPrev2 = 0.0f;

    juce::ADSR ampEnv, modEnv;
    juce::ADSR::Parameters ampParams { 0.01f, 0.3f, 0.8f, 0.3f };
    juce::ADSR::Parameters modParams { 0.01f, 0.3f, 0.0f, 0.3f };

    juce::SmoothedValue<float> indexSmoothed    { 0.0f };
    juce::SmoothedValue<float> feedbackSmoothed { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> ratioSmoothed { 1.0f };
    juce::SmoothedValue<float> ceilSmoothed     { 1.0e9f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FMVoice)
};
