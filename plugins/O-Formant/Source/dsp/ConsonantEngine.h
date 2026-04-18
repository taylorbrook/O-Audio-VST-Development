/*
  ==============================================================================

    ConsonantEngine.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Place/manner articulation model for consonant synthesis.
    X axis (Place): spectral center frequency via resonant bandpass pair and
                    place-dependent plosive burst spectral template.
    Y axis (Manner): temporal profile from plosive burst to sustained fricative.
    Dedicated consonant envelope (Attack/Hold/Decay) triggered at note onset
    with timing derived from manner parameter (Klatt-informed values).

    v1.23.0: consonant realism overhaul
      - Stevens-Blumstein plosive burst spectral templates
        (LP@800Hz labial / HP@3.5kHz alveolar / BP@2kHz velar) crossfaded
        by place
      - VOT / aspiration phase for voiceless stops: (1-voicing)*80+5 ms
        aspiration window with separate noise output routed through the
        cascade (vowel) bank by FormantVoice
      - Klatt MOD voiced-fricative noise gating (50% square gate at F0)

    Consonant noise is routed through FricationFormantBank by FormantVoice.
    Aspiration noise is routed through CascadeFormantBank by FormantVoice.

  ==============================================================================
*/

#pragma once
#include "FormantBiquad.h"
#include <JuceHeader.h>
#include <cmath>

class ConsonantEngine
{
public:
    void prepare (double sr, int voiceIndex) noexcept
    {
        sampleRate = sr;
        random = juce::Random (voiceIndex * 37 + 23);
        burstSamplesRemaining = 0;
        onsetSamplesRemaining = 0;
        onsetTotalSamples = static_cast<int> (0.025f * static_cast<float> (sr));
        aspirationSamplesRemaining = 0;
        aspirationTotalSamples = 0;
        consonantPhase = EnvPhase::Off;
        consonantEnvSample = 0;
        cachedGlottalPhase = 0.0f;
        currentAspirationNoise = 0.0f;
        burstLpState = 0.0f;
        burstHpLpState = 0.0f;

        updateCoefficients (0.5f, 0.5f, 0.5f, sr);
        setBurstTemplateCoefficients (sr);
    }

    void reset() noexcept
    {
        placeFilter1.reset();
        placeFilter2.reset();
        burstBpFilter.reset();
        burstSamplesRemaining = 0;
        onsetSamplesRemaining = 0;
        aspirationSamplesRemaining = 0;
        burstAmplitude = 0.0f;
        currentOnsetSuppression = 0.0f;
        currentAspirationNoise = 0.0f;
        consonantPhase = EnvPhase::Off;
        consonantEnvSample = 0;
        burstLpState = 0.0f;
        burstHpLpState = 0.0f;
    }

    void triggerBurst (float velocity) noexcept
    {
        burstSamplesRemaining = cachedBurstDuration;
        onsetSamplesRemaining = onsetTotalSamples;
        burstAmplitude = velocity;

        // VOT / aspiration phase: only for voiceless plosives
        // (voicing < 0.5 AND manner < 0.3)
        if (cachedVoicing < 0.5f && cachedManner < 0.3f)
        {
            // Voiceless-strength factor: 1.0 at voicing=0, 0.0 at voicing=0.5
            float voicelessFactor = juce::jlimit (0.0f, 1.0f,
                                                  (0.5f - cachedVoicing) * 2.0f);
            // VOT duration: (1-voicing)*80 + 5 ms  (Lisker-Abramson 1964)
            float votMs = (1.0f - cachedVoicing) * 80.0f + 5.0f;
            // consonantVOT param scales 0-2x (default 0.5 -> 1.0x)
            votMs *= 2.0f * cachedVOTScale * voicelessFactor;
            aspirationTotalSamples = juce::jmax (1, static_cast<int> (votMs * 0.001f
                                                                      * static_cast<float> (sampleRate)));
            aspirationSamplesRemaining = aspirationTotalSamples;
        }
        else
        {
            aspirationSamplesRemaining = 0;
            aspirationTotalSamples = 0;
        }

        // Start dedicated consonant envelope
        consonantPhase = EnvPhase::Attack;
        consonantEnvSample = 0;
    }

    // Block-rate coefficient update: place = X (0-1), manner = Y (0-1)
    // When auto mode is on, envelope timing is derived from manner.
    void updateCoefficients (float place, float manner, float voicing, double sr) noexcept
    {
        float nyquist = static_cast<float> (sr * 0.5) - 100.0f;

        cachedPlace = place;

        // Place of articulation -> spectral center frequency
        // Labial(0)=500Hz -> Alveolar(0.33)=3kHz -> Palatal(0.67)=6kHz -> Velar(1.0)=2kHz
        float centerFreq = computePlaceFrequency (place);
        centerFreq = juce::jmin (centerFreq, nyquist);

        float q = computePlaceQ (place);

        auto bp1 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, centerFreq, q);
        placeFilter1.setCoefficients (bp1);

        float secondaryFreq = juce::jmin (centerFreq * 1.5f, nyquist);
        auto bp2 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, secondaryFreq, q * 0.5f);
        placeFilter2.setCoefficients (bp2);

        // Manner of articulation -> temporal parameters
        cachedManner = manner;
        cachedVoicing = voicing;

        // Burst duration: plosive(0)=8ms, fricative(1)=80ms
        float burstMs = 0.008f + manner * 0.072f;
        cachedBurstDuration = juce::jmax (1, static_cast<int> (burstMs * static_cast<float> (sr)));

        // Burst decay rate: plosive(0)=12 (sharp), fricative(1)=2 (gentle)
        cachedBurstDecayRate = 12.0f - manner * 10.0f;

        // Consonant envelope timing derived from manner (Klatt-informed)
        // Plosive(0): 1ms attack, 0ms hold, 15ms decay  -> ~16ms total
        // Fricative(1): 40ms attack, 60ms hold, 40ms decay -> ~140ms total
        float envAttackMs = 1.0f + manner * 39.0f;
        float envHoldMs   = manner * 60.0f;
        float envDecayMs  = 15.0f + manner * 25.0f;
        float srF = static_cast<float> (sr);
        cachedAttackSamples = juce::jmax (1, static_cast<int> (envAttackMs * 0.001f * srF));
        cachedHoldSamples   = static_cast<int> (envHoldMs * 0.001f * srF);
        cachedDecaySamples  = juce::jmax (1, static_cast<int> (envDecayMs * 0.001f * srF));
    }

    // Override envelope timing with user-specified values (called when auto is off)
    void setManualEnvelope (float attackMs, float holdMs, float decayMs, double sr) noexcept
    {
        float srF = static_cast<float> (sr);
        cachedAttackSamples = juce::jmax (1, static_cast<int> (attackMs * 0.001f * srF));
        cachedHoldSamples   = static_cast<int> (holdMs * 0.001f * srF);
        cachedDecaySamples  = juce::jmax (1, static_cast<int> (decayMs * 0.001f * srF));
    }

    // Inject glottal phase from FormantVoice for Klatt MOD voiced-fricative gating
    void setGlottalPhase (float phase01) noexcept
    {
        cachedGlottalPhase = phase01;
    }

    // Set VOT scale factor (0-1, default 0.5 = 1.0x nominal VOT duration)
    void setVOTScale (float vot01) noexcept
    {
        cachedVOTScale = juce::jlimit (0.0f, 1.0f, vot01);
    }

    inline float getNextSample (float consonantLevel) noexcept
    {
        // -------- Aspiration phase: voice-suppressed noise for voiceless stops
        // Generates a separate noise stream that FormantVoice routes through
        // the cascade (vowel) bank so it is shaped by the opening vocal tract.
        if (aspirationSamplesRemaining > 0)
        {
            float aspProgress = 1.0f - static_cast<float> (aspirationSamplesRemaining)
                                       / static_cast<float> (juce::jmax (1, aspirationTotalSamples));
            // Raised-sine envelope: smooth rise-peak-fall over the VOT window
            float aspEnv = std::sin (aspProgress * juce::MathConstants<float>::pi);
            float rawNoise = random.nextFloat() * 2.0f - 1.0f;
            currentAspirationNoise = rawNoise * aspEnv * 0.6f * burstAmplitude;
            --aspirationSamplesRemaining;
        }
        else
        {
            currentAspirationNoise = 0.0f;
        }

        // -------- Onset suppression envelope (glottal source damping)
        if (onsetSamplesRemaining > 0)
        {
            float onsetProgress = 1.0f - static_cast<float> (onsetSamplesRemaining)
                                         / static_cast<float> (juce::jmax (1, onsetTotalSamples));
            // Voiceless: full suppression. Voiced plosive: 70% suppression (voice bar at 30%).
            // Voiced fricative: no suppression.
            float voicelessFactor = 1.0f - cachedVoicing * (0.3f + 0.7f * cachedManner);
            currentOnsetSuppression = std::exp (-6.0f * onsetProgress)
                                      * burstAmplitude * voicelessFactor;
            --onsetSamplesRemaining;
        }
        else
        {
            currentOnsetSuppression = 0.0f;
        }

        // Aspiration phase also fully suppresses the glottal source
        if (aspirationSamplesRemaining > 0 || currentAspirationNoise > 0.0f)
            currentOnsetSuppression = juce::jmax (currentOnsetSuppression, burstAmplitude);

        // -------- Early-out: no consonant activity
        bool burstActive = burstSamplesRemaining > 0;
        bool envActive = consonantPhase != EnvPhase::Off;

        if (consonantLevel < 0.001f && ! burstActive && ! envActive)
            return 0.0f;

        // -------- White noise source
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Klatt MOD: 50% square-wave gate at F0 for voiced fricatives.
        // Makes /z/, /v/, /Z/ sound distinct from voiceless + voice-bar bleed.
        if (cachedVoicing > 0.5f && cachedManner > 0.3f)
        {
            float modGate = (cachedGlottalPhase < 0.5f) ? 1.0f : 0.5f;
            noise *= modGate;
        }

        // Place-of-articulation spectral shaping (dual resonant bandpass).
        // Kept for back-compat: contributes to sustained fricative shape but
        // the primary frication coloration now comes from FricationFormantBank
        // applied downstream in FormantVoice.
        float shaped = placeFilter1.processSample (noise)
                       + 0.4f * placeFilter2.processSample (noise);

        // Continuous component: fricatives produce sustained noise (scales with manner)
        float output = consonantLevel * cachedManner * shaped;

        // -------- Burst component: plosive onset transient
        if (burstSamplesRemaining > 0)
        {
            float progress = 1.0f - static_cast<float> (burstSamplesRemaining)
                                    / static_cast<float> (juce::jmax (1, cachedBurstDuration));
            float burstEnv = std::exp (-cachedBurstDecayRate * progress) * burstAmplitude;

            // Stevens-Blumstein place-dependent burst templates (plosives only)
            // For plosives (manner < 0.3), replace the generic dual-BPF shape
            // with a place-crossfaded template; for fricatives, keep shape.
            float burstShaped;
            if (cachedManner < 0.3f)
            {
                burstShaped = applyBurstTemplate (noise, cachedPlace);
                // Crossfade between template and generic shape as manner approaches 0.3
                float blend = juce::jlimit (0.0f, 1.0f, cachedManner / 0.3f);
                burstShaped = (1.0f - blend) * burstShaped + blend * shaped;
            }
            else
            {
                burstShaped = shaped;
            }

            // Burst amplitude boosted for plosives: 2x at manner=0, 1x at manner=1
            output += burstEnv * (2.0f - cachedManner) * burstShaped;
            --burstSamplesRemaining;
        }

        // Always gate output with dedicated consonant envelope
        output *= advanceEnvelope();

        return juce::jlimit (-1.0f, 1.0f, output);
    }

    // Separate aspiration noise output (routed through cascade bank by FormantVoice)
    float getAspirationNoise() const noexcept { return currentAspirationNoise; }

    // Suppression factor for glottal source during plosive onset / aspiration
    float getOnsetSuppression() const noexcept { return currentOnsetSuppression; }

    // Continuous suppression for voiceless fricatives during full consonant envelope.
    // Returns 0 (no suppression) to 1 (full suppression).
    float getContinuousSuppression() const noexcept
    {
        if (consonantPhase == EnvPhase::Off)
            return 0.0f;
        float fricativeFactor = cachedManner;          // 0 for plosives, 1 for fricatives
        float voicelessFactor = 1.0f - cachedVoicing;  // 1 for voiceless, 0 for voiced
        return fricativeFactor * voicelessFactor;
    }

private:
    // 3-phase consonant envelope state machine
    enum class EnvPhase { Off, Attack, Hold, Decay };

    float advanceEnvelope() noexcept
    {
        switch (consonantPhase)
        {
            case EnvPhase::Attack:
            {
                float val = static_cast<float> (consonantEnvSample + 1)
                            / static_cast<float> (cachedAttackSamples);
                if (++consonantEnvSample >= cachedAttackSamples)
                {
                    consonantPhase = (cachedHoldSamples > 0)
                                         ? EnvPhase::Hold
                                         : EnvPhase::Decay;
                    consonantEnvSample = 0;
                }
                return juce::jmin (val, 1.0f);
            }
            case EnvPhase::Hold:
            {
                if (++consonantEnvSample >= cachedHoldSamples)
                {
                    consonantPhase = EnvPhase::Decay;
                    consonantEnvSample = 0;
                }
                return 1.0f;
            }
            case EnvPhase::Decay:
            {
                float progress = static_cast<float> (consonantEnvSample)
                                 / static_cast<float> (cachedDecaySamples);
                float val = std::exp (-5.0f * progress);
                if (++consonantEnvSample >= cachedDecaySamples)
                    consonantPhase = EnvPhase::Off;
                return val;
            }
            case EnvPhase::Off:
            default:
                return 0.0f;
        }
    }

    // ---- Stevens-Blumstein plosive burst spectral templates ----
    // One-pole LP (for /p b/ diffuse-falling) + HP derived from second LP
    // (for /t d/ diffuse-rising) + biquad BP (for /k g/ compact).
    // Crossfaded by place: 0->LP, 0.5->HP, 1.0->BP
    void setBurstTemplateCoefficients (double sr) noexcept
    {
        float srF = static_cast<float> (sr);
        // LP @ 800 Hz (labial)
        burstLpAlpha = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 800.0f / srF);
        // HP derived as: x - LP_at_3.5kHz(x)
        burstHpLpAlpha = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 3500.0f / srF);
        // BP Q=3 @ 2 kHz (velar)
        auto bpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, 2000.0f, 3.0f);
        burstBpFilter.setCoefficients (bpCoeffs);
    }

    inline float applyBurstTemplate (float noise, float place) noexcept
    {
        // LP one-pole (labial): y[n] = y[n-1] + alpha * (x - y[n-1])
        burstLpState += burstLpAlpha * (noise - burstLpState);
        float lp = burstLpState;

        // HP: x - LP(x) at cutoff 3.5 kHz
        burstHpLpState += burstHpLpAlpha * (noise - burstHpLpState);
        float hp = noise - burstHpLpState;

        // BP Q=3 @ 2 kHz (velar)
        float bp = burstBpFilter.processSample (noise);

        // Crossfade LP -> HP -> BP across place axis [0, 0.5, 1]
        if (place < 0.5f)
        {
            float t = place * 2.0f;
            return (1.0f - t) * lp + t * hp;
        }
        float t = (place - 0.5f) * 2.0f;
        return (1.0f - t) * hp + t * bp;
    }

    // Piecewise linear place frequency mapping
    static float computePlaceFrequency (float place) noexcept
    {
        if (place <= 0.33f)
        {
            float t = place / 0.33f;
            return 500.0f + t * 2500.0f;   // Labial 500 -> Alveolar 3000
        }
        if (place <= 0.67f)
        {
            float t = (place - 0.33f) / 0.34f;
            return 3000.0f + t * 3000.0f;  // Alveolar 3000 -> Palatal 6000
        }
        float t = (place - 0.67f) / 0.33f;
        return 6000.0f - t * 4000.0f;      // Palatal 6000 -> Velar 2000
    }

    // Place-dependent filter Q: narrow for alveolar/palatal, broad for labial/velar
    static float computePlaceQ (float place) noexcept
    {
        if (place <= 0.33f)
        {
            float t = place / 0.33f;
            return 1.5f + t * 2.5f;        // Labial 1.5 -> Alveolar 4.0
        }
        if (place <= 0.67f)
        {
            float t = (place - 0.33f) / 0.34f;
            return 4.0f - t * 1.0f;        // Alveolar 4.0 -> Palatal 3.0
        }
        float t = (place - 0.67f) / 0.33f;
        return 3.0f - t * 1.0f;            // Palatal 3.0 -> Velar 2.0
    }

    juce::Random random;
    double sampleRate = 44100.0;

    FormantBiquad placeFilter1;
    FormantBiquad placeFilter2;

    // Burst template filters
    FormantBiquad burstBpFilter;       // velar BP Q=3 @ 2kHz
    float burstLpState = 0.0f;         // one-pole LP (labial) state
    float burstHpLpState = 0.0f;       // one-pole LP used to derive HP (alveolar)
    float burstLpAlpha = 0.0f;
    float burstHpLpAlpha = 0.0f;

    float cachedPlace = 0.5f;
    float cachedManner = 0.5f;
    float cachedVoicing = 0.5f;
    float cachedGlottalPhase = 0.0f;
    float cachedVOTScale = 0.5f;       // user param, 0.5 -> nominal Klatt VOT
    int cachedBurstDuration = 353;
    float cachedBurstDecayRate = 7.0f;

    int burstSamplesRemaining = 0;
    float burstAmplitude = 0.0f;

    int onsetSamplesRemaining = 0;
    int onsetTotalSamples = 1103;
    float currentOnsetSuppression = 0.0f;

    // VOT / aspiration state (voiceless plosives only)
    int aspirationSamplesRemaining = 0;
    int aspirationTotalSamples = 0;
    float currentAspirationNoise = 0.0f;

    // Consonant envelope state
    EnvPhase consonantPhase = EnvPhase::Off;
    int consonantEnvSample = 0;
    int cachedAttackSamples = 44;
    int cachedHoldSamples = 0;
    int cachedDecaySamples = 661;
};
