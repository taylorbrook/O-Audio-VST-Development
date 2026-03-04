/*
  ==============================================================================

    PrismParamIds.h
    Shared parameter ID definitions for O-Prism

    Single source of truth for parameter ID strings used by
    PluginEditor (relay/attachment creation). Mirrors the grouping
    in PluginProcessor's createXxxParameters() helper functions.

    When adding or removing parameters, update the relevant section here.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace PrismParamIds
{
    static constexpr int kNumModSlots = 16;

    // ─── Oscillator IDs (prefix + shared suffixes) ───

    inline juce::StringArray oscIds (const juce::String& prefix)
    {
        juce::StringArray ids;
        for (const auto* s : { "Table", "Pos", "Level", "Pan", "Coarse",
                                "Fine", "Phase", "Unison", "Detune", "Width" })
            ids.add (prefix + s);
        return ids;
    }

    // ─── Filter IDs (prefix + shared suffixes) ───

    inline juce::StringArray filterIds (const juce::String& prefix)
    {
        juce::StringArray ids;
        for (const auto* s : { "Type", "Cutoff", "Res", "Drive", "KeyTrack" })
            ids.add (prefix + s);
        return ids;
    }

    // ─── Mod Matrix Slider IDs (loop-generated: Src, Dst, Amt per slot) ───

    inline juce::StringArray modMatrixSliderIds()
    {
        juce::StringArray ids;
        for (int i = 0; i < kNumModSlots; ++i)
        {
            auto p = "modSlot" + juce::String (i);
            ids.add (p + "Src");
            ids.add (p + "Dst");
            ids.add (p + "Amt");
        }
        return ids;
    }

    // ─── Mod Slot Toggle IDs (loop-generated) ───

    inline juce::StringArray modSlotToggleIds()
    {
        juce::StringArray ids;
        for (int i = 0; i < kNumModSlots; ++i)
            ids.add ("modSlot" + juce::String (i) + "On");
        return ids;
    }

    // ─── Bypass Toggle IDs ───

    inline juce::StringArray bypassToggleIds()
    {
        return { "reverbBypass", "delayBypass", "chorusBypass",
                 "distBypass", "eqBypass" };
    }

    // ─── All Slider Param IDs (matches createParameterLayout order) ───

    inline juce::StringArray allSliderIds()
    {
        juce::StringArray all;
        all.addArray (oscIds ("oscA"));                                           // 10
        all.addArray (oscIds ("oscB"));                                           // 10
        all.addArray ({ "subShape", "subOctave", "subLevel",
                        "noiseType", "noiseLevel" });                             //  5
        all.addArray ({ "ampAttack", "ampDecay", "ampSustain", "ampRelease" });   //  4
        all.addArray ({ "filtAttack", "filtDecay", "filtSustain",
                        "filtRelease", "filtEnvDepth" });                         //  5
        all.addArray (filterIds ("filtA"));                                       //  5
        all.addArray (filterIds ("filtB"));                                       //  5
        all.addArray ({ "filtRouting" });                                         //  1
        all.addArray ({ "tuningPreset", "tonic", "masterTune", "octaveStretch",
                        "pitchBendRange", "glideMode", "glideTime" });           //  7
        all.addArray ({ "reverbSize", "reverbDamp", "reverbPredelay",
                        "reverbMix" });                                          //  4
        all.addArray ({ "delayTime", "delayFeedback", "delayMode",
                        "delayMix" });                                           //  4
        all.addArray ({ "chorusRate", "chorusDepth", "chorusMix" });              //  3
        all.addArray ({ "distType", "distDrive", "distMix" });                   //  3
        all.addArray ({ "eqLowGain", "eqMidGain", "eqMidFreq",
                        "eqHighGain" });                                         //  4
        all.addArray ({ "lfo1Rate", "lfo1Shape", "lfo2Rate", "lfo2Shape",
                        "lfo3Rate", "lfo3Shape", "lfo4Rate", "lfo4Shape" });   //  8
        all.addArray (modMatrixSliderIds());                                     // 48
        all.addArray ({ "masterVol", "oscMix", "polyphony" });                   //  3
        return all;                                                              // 124
    }

} // namespace PrismParamIds
