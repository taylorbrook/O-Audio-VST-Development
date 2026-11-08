/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

// ============================================================================
// DevLoaderEditor — the Phase 2.1 audition editor (CONTEXT-2.1 D13, PLAN-2.1
// T10 / P4).
//
// Removed in Phase 3.1 together with `OGI_DEV_LOADER` and the `createEditor`
// branch.
//
// Layout: 4 "Load slot N" buttons with one status line each above a
// GenericAudioProcessorEditor (all 261 parameters, scrollable).
//
// Load path = the message-thread slot API shared with Phase 3.2:
// GranulatorInstrumentProcessor::requestSlotLoad (slot, file). Status lines
// are polled at 10 Hz through getSlotInfo (slot) (a copy taken under the
// loader lock). The processor never calls into the editor.
//
// File chooser lifetime (RESEARCH-2.1 §6, P14): ONE std::unique_ptr
// <FileChooser>, re-made on every click (a chooser must not be launched twice),
// plus pendingSlot. ~FileChooser nulls its async callback, so destroying the
// chooser in the editor's destructor makes a still-open dialog's callback
// harmless; the lambda also holds a SafePointer and early-outs when the editor
// is gone or the result is empty.
//
// Compiled into the PLUGIN target only. The consoles drop every source whose
// path matches "PluginEditor" (scripts/param-dump/ParamDump.cmake), and
// createEditor() selects this class only under JUCE_WEB_BROWSER && OGI_DEV_LOADER.
// ============================================================================
class GranulatorInstrumentProcessor;

class DevLoaderEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit DevLoaderEditor (GranulatorInstrumentProcessor& processor);
    ~DevLoaderEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    static constexpr int kSlots = 4;

private:
    void timerCallback() override;
    void chooseFileForSlot (int slot);
    void refreshStatus();

    GranulatorInstrumentProcessor& proc;
    juce::GenericAudioProcessorEditor generic;

    std::array<juce::TextButton, kSlots> loadButtons;
    std::array<juce::Label, kSlots>      statusLines;
    std::array<juce::String, kSlots>     lastStatus;

    std::unique_ptr<juce::FileChooser> chooser;   // re-made per click
    int pendingSlot = -1;                        // the slot the open chooser loads into

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DevLoaderEditor)
};
