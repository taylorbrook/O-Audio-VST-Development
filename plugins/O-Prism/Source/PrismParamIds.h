/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

    // Index of the "Custom" entry in the tuningPreset choice parameter.
    // Must stay in sync with the StringArray in PluginProcessor::createParameterLayout()
    // (12-TET, Pythagorean, Zarlino, Meantone 1/4, Werckmeister III, Kirnberger III,
    //  Vallotti, Well Tempered, Just Intonation, Bohlen-Pierce, Custom = 10).
    static constexpr int kCustomTuningPresetIndex = 10;

    // ─── Oscillator IDs (prefix + shared suffixes) ───

    inline juce::StringArray oscIds (const juce::String& prefix)
    {
        juce::StringArray ids;
        for (const auto* s : { "Table", "Pos", "Level", "Pan", "Coarse",
                                "Fine", "Phase", "Unison", "Detune", "Width",
                                "WarpType", "WarpAmt" })
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
        all.addArray (oscIds ("oscA"));                                           // 12
        all.addArray (oscIds ("oscB"));                                           // 12
        all.addArray ({ "subShape", "subOctave", "subLevel",
                        "noiseType", "noiseLevel", "subRouting" });               //  6
        all.addArray ({ "ampAttack", "ampDecay", "ampSustain", "ampRelease" });   //  4
        all.addArray ({ "filtAttack", "filtDecay", "filtSustain",
                        "filtRelease", "filtAEnvDepth", "filtBEnvDepth" });       //  6
        all.addArray (filterIds ("filtA"));                                       //  5
        all.addArray (filterIds ("filtB"));                                       //  5
        all.addArray ({ "filtRouting" });                                         //  1
        all.addArray ({ "tuningPreset", "tonic", "masterTune", "octaveStretch",
                        "pitchBendRange", "glideMode", "glideTime" });           //  7
        all.addArray ({ "reverbSize", "reverbDamp", "reverbPredelay",
                        "reverbMix", "reverbModDepth", "reverbModRate" });       //  6
        all.addArray ({ "delayTime", "delayFeedback", "delayMode",
                        "delayMix" });                                           //  4
        all.addArray ({ "chorusRate", "chorusDepth", "chorusMix" });              //  3
        all.addArray ({ "distType", "distDrive", "distMix" });                   //  3
        all.addArray ({ "eqLowGain", "eqMidGain", "eqMidFreq",
                        "eqHighGain" });                                         //  4
        all.addArray ({ "lfo1Rate", "lfo1Shape", "lfo1Division",
                        "lfo2Rate", "lfo2Shape", "lfo2Division",
                        "lfo3Rate", "lfo3Shape", "lfo3Division",
                        "lfo4Rate", "lfo4Shape", "lfo4Division" });            // 12
        all.addArray (modMatrixSliderIds());                                     // 48
        all.addArray ({ "masterVol", "oscMix", "velocityCurve" });                 //  3
        return all;                                                              // 126
    }

} // namespace PrismParamIds
