/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 PLACEHOLDER: wraps a GenericAudioProcessorEditor so all 21
    parameters are editable in any host. Stage 3 replaces this file and
    PluginEditor.cpp with mockups/v1-PluginEditor.{h,cpp} (WebView UI),
    which already use this class name and constructor.

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OSimpleWavetableAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OSimpleWavetableAudioProcessorEditor (OSimpleWavetableAudioProcessor&);
    ~OSimpleWavetableAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OSimpleWavetableAudioProcessor& processorRef;
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleWavetableAudioProcessorEditor)
};
