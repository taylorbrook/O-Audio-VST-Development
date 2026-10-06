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

    O-simpleWavetable - Editor Implementation (Stage 1 placeholder)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OSimpleWavetableAudioProcessorEditor::OSimpleWavetableAudioProcessorEditor (OSimpleWavetableAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      genericEditor (p)
{
    juce::ignoreUnused (processorRef);   // used by the Stage 3 WebView editor

    // The generic editor scrolls its parameter list in a Viewport, so all
    // 21 rows are reachable at any size.
    addAndMakeVisible (genericEditor);
    setSize (420, 640);
}

OSimpleWavetableAudioProcessorEditor::~OSimpleWavetableAudioProcessorEditor() = default;

void OSimpleWavetableAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OSimpleWavetableAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
