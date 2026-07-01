/*
  ==============================================================================

    trim_gain_check.cpp
    O-MicrotonalSampler — v1.23.0 TrimTable loudness-trim math tests.

    Manual run:
        ninja O-MicrotonalSampler_TrimGainCheck && \
        ./build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_TrimGainCheck
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------
    The per-technique + per-(technique,layer) trim table that the voice folds
    into its layer weights (velocity mode) and DynLayer gains (CC mode). The
    DSP itself is a plain multiply by gainFor(); these tests pin the gain math
    that drives it:

      1. Defaults: a fresh TrimTable returns EXACTLY 1.0 for every
         (technique, layer) → bit-identical to v1.22.0 when untouched.
      2. Single-axis: a −6 dB technique master with 0 dB layer ≈ 0.5012
         (and a −6 dB layer with 0 dB master likewise).
      3. Additive combine: technique −6 dB AND layer −6 dB → −12 dB ≈ 0.2512,
         i.e. dbToGain(tech)·dbToGain(layer) == dbToGain(tech + layer).
      4. Boost: +6 dB ≈ 1.995.
      5. Out-of-range indices return unity (audio path never perturbed).

    NB: clamping to ±kTrimMaxDb lives in the processor setters, not gainFor();
    this test confirms gainFor honours whatever is stored and that the table's
    storage default is 0 dB.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "../SampleMap.h"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
    int failed = 0;

    void check (bool cond, const std::string& desc)
    {
        if (cond)
            std::cout << "  PASS: " << desc << "\n";
        else
        {
            std::cout << "  FAIL: " << desc << "\n";
            ++failed;
        }
    }

    // Approximate equality for linear-gain comparisons.
    bool near (float a, float b, float tol = 1.0e-3f)
    {
        return std::abs (a - b) <= tol;
    }
}

int main()
{
    std::cout << "== trim_gain_check ==\n";

    // ---- 1. Defaults are exactly unity ----------------------------------
    {
        TrimTable t;
        bool allUnity = true;
        for (int tech = 0; tech < kMaxTechniques; ++tech)
            for (int layer = 0; layer < kMaxVelocityLayers; ++layer)
                if (t.gainFor (tech, layer) != 1.0f)
                    allUnity = false;
        check (allUnity, "fresh TrimTable: gainFor == 1.0 for every (tech,layer)");
    }

    // ---- 2. Single-axis trims -------------------------------------------
    {
        TrimTable t;
        t.techniqueDb[3].store (-6.0f);
        check (near (t.gainFor (3, 0), 0.5012f),
               "technique[3] = -6 dB, layer 0 dB -> ~0.5012");
        check (near (t.gainFor (3, 1), 0.5012f),
               "technique[3] applies to ALL its layers (layer 1 too)");
        check (t.gainFor (2, 0) == 1.0f,
               "a different technique is unaffected");
    }
    {
        TrimTable t;
        t.layerDb[0][2].store (-6.0f);   // ord, mf
        check (near (t.gainFor (0, 2), 0.5012f),
               "layer[ord][mf] = -6 dB, master 0 dB -> ~0.5012");
        check (t.gainFor (0, 1) == 1.0f,
               "sibling layer of the same technique is unaffected");
    }

    // ---- 3. Additive combine (tech + layer) -----------------------------
    {
        TrimTable t;
        t.techniqueDb[0].store (-6.0f);
        t.layerDb[0][2].store (-6.0f);
        check (near (t.gainFor (0, 2), 0.2512f),
               "tech -6 dB AND layer -6 dB -> -12 dB ~0.2512 (additive)");
        // Equivalence: combined == product of the two independent gains.
        const float techOnly  = juce::Decibels::decibelsToGain (-6.0f);
        const float layerOnly = juce::Decibels::decibelsToGain (-6.0f);
        check (near (t.gainFor (0, 2), techOnly * layerOnly),
               "gainFor(tech+layer) == dbToGain(tech) * dbToGain(layer)");
    }

    // ---- 4. Boost --------------------------------------------------------
    {
        TrimTable t;
        t.techniqueDb[5].store (+6.0f);
        check (near (t.gainFor (5, 0), 1.9953f, 2.0e-3f),
               "technique[5] = +6 dB -> ~1.995");
    }

    // ---- 5. Out-of-range indices return unity ---------------------------
    {
        TrimTable t;
        t.techniqueDb[0].store (-12.0f);   // would change in-range results
        check (t.gainFor (-1, 0) == 1.0f,  "negative technique -> unity");
        check (t.gainFor (kMaxTechniques, 0) == 1.0f, "tech == kMaxTechniques -> unity");
        check (t.gainFor (0, -1) == 1.0f,  "negative layer -> unity");
        check (t.gainFor (0, kMaxVelocityLayers) == 1.0f, "layer == kMaxVelocityLayers -> unity");
    }

    std::cout << "== trim_gain_check: " << failed << " failed ==\n";
    return failed;
}
