/*
  ==============================================================================

    BowModel.h
    O-Contrabass - Bow Excitation Envelope Model (bass-tuned)
    Ouaricon Audio
    Developer: Taylor Brook

    Inline copy of O-Bowed/Source/DSP/BowModel.h with bass-tuned member-init
    defaults: bowSpeedParam = 0.15 (was 0.2), bowPressureParam = 1.0 (was 0.5).
    Envelope coefficients, attack/release semantics, retrigger reset, and
    ratio-preserving setters carry over unchanged. Promotion to the shared
    bow-friction module is Phase 2.1b's job.

  ==============================================================================
*/

#pragma once

class BowModel
{
public:
    void prepare (double sampleRate);
    void startBow (float velocity);     // called on note-on (velocity 0-1)
    void stopBow();                      // called on note-off (allowTailOff=true)
    void reset();                        // called on hard stop (allowTailOff=false)
    void updateEnvelope();               // called per-sample
    void setBowSpeed (float speed);      // from APVTS
    void setBowPressure (float pressure); // from APVTS

    float getBowVelocity() const noexcept { return v_bow; }
    float getBowForce() const noexcept { return F_bow; }
    bool isActive() const noexcept;

private:
    double sampleRate = 44100.0;

    // Envelope state
    float v_bow = 0.0f;          // current bow velocity
    float F_bow = 0.0f;          // current bow force
    float v_bow_target = 0.0f;   // target from parameter + velocity
    float F_bow_target = 0.0f;   // target from parameter

    // Smoothing coefficients (one-pole)
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Bass-tuned parameter defaults (RESEARCH.md §1.4)
    float bowSpeedParam = 0.15f;     // bass default (was 0.2 on treble)
    float bowPressureParam = 1.0f;   // bass default (was 0.5 on treble)

    bool bowActive = false;
};
