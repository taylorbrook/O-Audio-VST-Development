/*
  ==============================================================================

    DroneStringEngine.h
    O-Bowed - Processor-Level Drone String Engine (1-4 Always-Bowed Strings)
    Ouaricon Audio
    Developer: Taylor Brook

    Owns 1-4 WaveguideString + BowModel + HyperbolicFriction instances that
    run independently of MIDI. Per-string panning, tuning (cents offset from
    reference pitch), and ±5% bow parameter variation for natural character.

  ==============================================================================
*/

#pragma once
#include "WaveguideString.h"
#include "BowModel.h"
#include "HyperbolicFriction.h"
#include <array>

class DroneStringEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Parameter updates (called once per processBlock)
    void setStringCount (int count);
    void setTuning (int stringIndex, float cents);
    void setReferencePitch (float hz);
    void setBowSpeed (float speed);
    void setBowPressure (float pressure);
    void setBowPosition (float position);
    void setRosin (float rosin);
    void setBrightness (float brightness);
    void setInfiniteSustain (float sustain);
    void setStringGauge (float gauge);

    // Per-sample stereo output (adds to outL/outR)
    void processSample (float& outL, float& outR);

    // Bridge output for sympathetic excitation (mono sum of all drones pre-pan)
    float getLastBridgeOutput() const noexcept { return lastBridgeOutput; }

private:
    static constexpr int MAX_DRONES = 4;
    static constexpr float HALF_PI = 1.5707963f;

    struct DroneString {
        WaveguideString waveguide;
        BowModel bow;
        HyperbolicFriction friction;
        float panL = 0.707f;
        float panR = 0.707f;
        float speedVariation = 1.0f;
        float pressureVariation = 1.0f;
        float targetFrequency = 440.0f;
        bool active = false;
    };

    std::array<DroneString, MAX_DRONES> drones;
    int activeCount = 0;

    float referencePitch = 440.0f;
    float tuningCents[MAX_DRONES] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Shared bow params (before per-string variation)
    float sharedBowSpeed = 0.2f;
    float sharedBowPressure = 0.5f;

    float lastBridgeOutput = 0.0f;
    double currentSampleRate = 44100.0;

    void updatePanPositions();
    void initVariations();
    void updateDroneFrequency (int index);

    // Pan lookup: [stringCount-1][stringIndex] -> pan position (0.0=L, 1.0=R)
    static constexpr float panTable[4][4] = {
        { 0.5f,  0.0f,  0.0f,  0.0f },   // 1 string: center
        { 0.35f, 0.65f, 0.0f,  0.0f },   // 2 strings
        { 0.25f, 0.50f, 0.75f, 0.0f },   // 3 strings
        { 0.20f, 0.40f, 0.60f, 0.80f }   // 4 strings
    };
};
