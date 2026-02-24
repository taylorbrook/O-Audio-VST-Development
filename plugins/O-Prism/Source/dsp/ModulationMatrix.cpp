/*
  ==============================================================================

    ModulationMatrix.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    16-slot modulation routing matrix with per-sample evaluation

  ==============================================================================
*/

#include "ModulationMatrix.h"

void ModulationMatrix::setAPVTS (juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;
}

void ModulationMatrix::updateFromAPVTS()
{
    if (parameters == nullptr)
        return;

    for (int i = 0; i < kNumSlots; ++i)
    {
        auto prefix = "modSlot" + juce::String (i);

        slots[static_cast<size_t> (i)].source =
            static_cast<int> (parameters->getRawParameterValue (prefix + "Src")->load());
        slots[static_cast<size_t> (i)].dest =
            static_cast<int> (parameters->getRawParameterValue (prefix + "Dst")->load());
        slots[static_cast<size_t> (i)].amount =
            parameters->getRawParameterValue (prefix + "Amt")->load();
        slots[static_cast<size_t> (i)].enabled =
            parameters->getRawParameterValue (prefix + "On")->load() > 0.5f;
    }
}

void ModulationMatrix::setSourceValue (ModSource source, float value)
{
    auto idx = static_cast<size_t> (source);
    if (idx < sourceValues.size())
        sourceValues[idx] = value;
}

void ModulationMatrix::evaluate()
{
    destOffsets.fill (0.0f);

    for (int i = 0; i < kNumSlots; ++i)
    {
        const auto& slot = slots[static_cast<size_t> (i)];

        if (! slot.enabled || slot.source == 0 || slot.dest == 0)
            continue;

        auto srcIdx = static_cast<size_t> (slot.source);
        auto dstIdx = static_cast<size_t> (slot.dest);

        if (srcIdx < sourceValues.size() && dstIdx < destOffsets.size())
            destOffsets[dstIdx] += sourceValues[srcIdx] * slot.amount;
    }
}

float ModulationMatrix::getModOffset (ModDest dest) const
{
    auto idx = static_cast<size_t> (dest);
    if (idx < destOffsets.size())
        return destOffsets[idx];
    return 0.0f;
}

void ModulationMatrix::clearOffsets()
{
    destOffsets.fill (0.0f);
}
