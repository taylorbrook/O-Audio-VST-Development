/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - Editor (Stage 1 placeholder — WebView UI lands in Stage 3)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OBitrotAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBitrotAudioProcessorEditor(OBitrotAudioProcessor&);
    ~OBitrotAudioProcessorEditor() override;

    void resized() override;

private:
    OBitrotAudioProcessor& processorRef;

    // Stage 1 placeholder: generic parameter list (WebView is Stage 3)
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBitrotAudioProcessorEditor)
};
