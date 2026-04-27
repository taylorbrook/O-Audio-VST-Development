/*
  ==============================================================================

    BowModel.h
    Ouaricon (bow-friction module) - Bow Excitation Envelope Model
    Ouaricon Audio
    Developer: Taylor Brook

    Generates bow velocity (v_bow) and bow force (F_bow) signals from
    MIDI input and parameter values. One-pole envelope smoothing for
    click-free attack/release. Velocity-dependent attack time (5-50ms).

    Plugin-agnostic: same envelope behaviour for both O-Bowed and
    O-Contrabass consumers. Phase 2.1b extraction.

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

    // Parameter values
    float bowSpeedParam = 0.2f;
    float bowPressureParam = 0.5f;

    bool bowActive = false;
};
