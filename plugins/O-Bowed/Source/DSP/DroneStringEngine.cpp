/*
  ==============================================================================

    DroneStringEngine.cpp
    O-Bowed - Processor-Level Drone String Engine
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "DroneStringEngine.h"
#include <juce_core/juce_core.h>
#include <cmath>

//==============================================================================
void DroneStringEngine::prepare (double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    for (auto& d : drones)
    {
        d.waveguide.prepare (sampleRate, maxBlockSize);
        d.bow.prepare (sampleRate);
        d.active = false;
    }

    activeCount = 0;
    initVariations();
}

//==============================================================================
void DroneStringEngine::reset()
{
    for (auto& d : drones)
    {
        d.waveguide.reset();
        d.bow.reset();
        d.active = false;
    }
    activeCount = 0;
    lastBridgeOutput = 0.0f;
}

//==============================================================================
void DroneStringEngine::initVariations()
{
    juce::Random rng (42);  // deterministic seed
    for (auto& d : drones)
    {
        d.speedVariation    = 0.95f + rng.nextFloat() * 0.10f;
        d.pressureVariation = 0.95f + rng.nextFloat() * 0.10f;
    }
}

//==============================================================================
void DroneStringEngine::setStringCount (int count)
{
    count = juce::jlimit (1, MAX_DRONES, count);
    if (count == activeCount)
        return;

    // Activate new drones
    for (int i = activeCount; i < count; ++i)
    {
        updateDroneFrequency (i);
        drones[i].waveguide.trigger (drones[i].targetFrequency);
        drones[i].bow.startBow (0.5f);  // constant moderate velocity
        drones[i].bow.setBowSpeed (sharedBowSpeed * drones[i].speedVariation);
        drones[i].bow.setBowPressure (sharedBowPressure * drones[i].pressureVariation);
        drones[i].active = true;
    }

    // Deactivate excess drones
    for (int i = count; i < activeCount; ++i)
    {
        drones[i].waveguide.reset();
        drones[i].bow.reset();
        drones[i].active = false;
    }

    activeCount = count;
    updatePanPositions();
}

//==============================================================================
void DroneStringEngine::setTuning (int stringIndex, float cents)
{
    if (stringIndex < 0 || stringIndex >= MAX_DRONES)
        return;

    if (std::abs (cents - tuningCents[stringIndex]) < 0.01f)
        return;

    tuningCents[stringIndex] = cents;

    if (drones[stringIndex].active)
    {
        updateDroneFrequency (stringIndex);
        drones[stringIndex].waveguide.trigger (drones[stringIndex].targetFrequency);
    }
}

//==============================================================================
void DroneStringEngine::setReferencePitch (float hz)
{
    if (std::abs (hz - referencePitch) < 0.01f)
        return;

    referencePitch = hz;

    for (int i = 0; i < activeCount; ++i)
    {
        updateDroneFrequency (i);
        drones[i].waveguide.trigger (drones[i].targetFrequency);
    }
}

//==============================================================================
void DroneStringEngine::setBowSpeed (float speed)
{
    sharedBowSpeed = speed;
    for (int i = 0; i < activeCount; ++i)
        drones[i].bow.setBowSpeed (speed * drones[i].speedVariation);
}

void DroneStringEngine::setBowPressure (float pressure)
{
    sharedBowPressure = pressure;
    for (int i = 0; i < activeCount; ++i)
        drones[i].bow.setBowPressure (pressure * drones[i].pressureVariation);
}

void DroneStringEngine::setBowPosition (float position)
{
    for (int i = 0; i < activeCount; ++i)
        drones[i].waveguide.setBowPosition (position);
}

void DroneStringEngine::setRosin (float rosin)
{
    for (int i = 0; i < activeCount; ++i)
        drones[i].friction.setRosin (rosin);
}

void DroneStringEngine::setBrightness (float brightness)
{
    for (int i = 0; i < activeCount; ++i)
        drones[i].waveguide.setBrightness (brightness);
}

void DroneStringEngine::setInfiniteSustain (float sustain)
{
    for (int i = 0; i < activeCount; ++i)
        drones[i].waveguide.setInfiniteSustain (sustain);
}

void DroneStringEngine::setStringGauge (float gauge)
{
    for (int i = 0; i < activeCount; ++i)
        drones[i].friction.setStringImpedance (gauge);
}

//==============================================================================
void DroneStringEngine::processSample (float& outL, float& outR)
{
    float bridgeSum = 0.0f;

    for (int i = 0; i < activeCount; ++i)
    {
        auto& d = drones[i];
        if (! d.active)
            continue;

        d.bow.updateEnvelope();
        float v_bow = d.bow.getBowVelocity();
        float F_bow = d.bow.getBowForce();
        float sample = d.waveguide.processSample (v_bow, F_bow, d.friction);
        sample = juce::jlimit (-1.0f, 1.0f, sample);

        bridgeSum += sample;
        outL += sample * d.panL;
        outR += sample * d.panR;
    }

    lastBridgeOutput = bridgeSum;
}

//==============================================================================
void DroneStringEngine::updatePanPositions()
{
    if (activeCount <= 0)
        return;

    int idx = juce::jlimit (0, 3, activeCount - 1);
    for (int i = 0; i < activeCount; ++i)
    {
        float pan = panTable[idx][i];
        drones[i].panL = std::cos (pan * HALF_PI);
        drones[i].panR = std::sin (pan * HALF_PI);
    }
}

//==============================================================================
void DroneStringEngine::updateDroneFrequency (int index)
{
    drones[index].targetFrequency = referencePitch
        * std::pow (2.0f, tuningCents[index] / 1200.0f);
}
