/*
  ==============================================================================

    ToneHoleSystem.h
    O-Wind - Tier 2 Tone Hole System (Keefe 3-Port Scattering Junctions)
    Ouaricon Audio
    Developer: Taylor Brook

    Models 8 tone hole junctions inline with the bore waveguide.
    Each junction uses 2nd-order IIR filters for open and closed states.
    Scattering coefficients: R = (Z_hole - Z_bore) / (Z_hole + Z_bore)
    Supports half-holing (continuous 0-1 parameter) for pitch bending
    and cross-fingering for alternate timbres.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class ToneHoleSystem
{
public:
    static constexpr int numHoles = 8;

    struct HoleState
    {
        float openAmount = 0.0f;  // 0 = closed, 1 = open
    };

    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;

        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (maxBlockSize),
            1
        };

        for (int i = 0; i < numHoles; ++i)
        {
            openFilters[static_cast<size_t> (i)].prepare (spec);
            closedFilters[static_cast<size_t> (i)].prepare (spec);

            // Default: all holes closed
            holeStates[static_cast<size_t> (i)].openAmount = 0.0f;

            // Set default coefficients
            updateHoleCoefficients (i);
        }
    }

    void reset()
    {
        for (int i = 0; i < numHoles; ++i)
        {
            openFilters[static_cast<size_t> (i)].reset();
            closedFilters[static_cast<size_t> (i)].reset();
        }
    }

    // Set hole state for a specific junction (0 = closed, 1 = open)
    void setHoleOpen (int holeIndex, float openAmount)
    {
        if (holeIndex >= 0 && holeIndex < numHoles)
            holeStates[static_cast<size_t> (holeIndex)].openAmount = juce::jlimit (0.0f, 1.0f, openAmount);
    }

    // Set all holes from a fingering pattern (array of 8 open amounts)
    void setFingering (const std::array<float, numHoles>& fingering)
    {
        for (int i = 0; i < numHoles; ++i)
            holeStates[static_cast<size_t> (i)].openAmount = juce::jlimit (0.0f, 1.0f, fingering[static_cast<size_t> (i)]);
    }

    // Process one sample: modifies the forward and backward traveling waves
    // at each tone hole junction inline with the bore
    void processSample (float& pPlus, float& pMinus)
    {
        for (int i = 0; i < numHoles; ++i)
        {
            float openAmt = holeStates[static_cast<size_t> (i)].openAmount;

            if (openAmt < 0.001f)
                continue;  // fully closed: no scattering

            // Scattering at junction:
            // Open hole reflection: R = (Z_hole - Z_bore) / (Z_hole + Z_bore)
            // Approximate: fully open hole has R ~ -0.4 to -0.6 (energy escapes)
            // Fully closed: R ~ 0 (no effect, wave passes through)

            // Interpolate reflection coefficient based on open amount
            float reflCoeff = -openAmt * 0.5f;  // 0 to -0.5

            // Process through open/closed filter pair
            float openComponent = openFilters[static_cast<size_t> (i)].processSample (pPlus) * openAmt;
            float closedComponent = closedFilters[static_cast<size_t> (i)].processSample (pPlus) * (1.0f - openAmt);
            float filteredPlus = openComponent + closedComponent;

            // Apply scattering
            float transmittedPlus = pPlus + reflCoeff * filteredPlus;
            float reflectedMinus = reflCoeff * pPlus;

            // Output from junction
            pPlus = transmittedPlus;
            pMinus += reflectedMinus;
        }
    }

    bool isEnabled() const { return enabled; }
    void setEnabled (bool shouldEnable) { enabled = shouldEnable; }

private:
    void updateHoleCoefficients (int holeIndex)
    {
        if (sampleRate <= 0.0)
            return;

        // Each hole has a characteristic frequency based on position
        // Holes further from open end have lower characteristic frequency
        float holeFreq = 1000.0f + static_cast<float> (holeIndex) * 500.0f;
        float clampedFreq = juce::jlimit (200.0f, static_cast<float> (sampleRate * 0.45), holeFreq);

        // Open hole: lowpass (energy radiates through hole at high frequencies)
        *openFilters[static_cast<size_t> (holeIndex)].coefficients =
            juce::dsp::IIR::Coefficients<float> (
                juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, clampedFreq, 0.707f));

        // Closed hole: slight highpass (closed hole acts as short shunt)
        *closedFilters[static_cast<size_t> (holeIndex)].coefficients =
            juce::dsp::IIR::Coefficients<float> (
                juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sampleRate, clampedFreq * 0.3f, 0.5f));
    }

    double sampleRate = 44100.0;
    bool enabled = false;

    std::array<HoleState, numHoles> holeStates {};
    std::array<juce::dsp::IIR::Filter<float>, numHoles> openFilters;
    std::array<juce::dsp::IIR::Filter<float>, numHoles> closedFilters;
};
