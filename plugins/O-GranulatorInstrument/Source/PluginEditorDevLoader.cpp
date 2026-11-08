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
// Removed in Phase 3.1 together with `OGI_DEV_LOADER` and the `createEditor` branch.
#include "PluginEditorDevLoader.h"
#include "PluginProcessor.h"

namespace
{
    constexpr int kWidth      = 800;
    constexpr int kHeight     = 700;
    constexpr int kMargin     = 10;
    constexpr int kRowHeight  = 26;
    constexpr int kRowGap     = 6;
    constexpr int kButtonW    = 120;
    constexpr int kHeaderRows = DevLoaderEditor::kSlots;

    juce::String describe (int slot, const ogi::SlotInfo& s)
    {
        juce::String line = "Slot " + juce::String (slot + 1) + ": ";

        if (s.loaded)
        {
            line << s.name
                 << "  |  " << juce::String (s.frames) << " frames @ " << juce::String (s.tapeRate, 0) << " Hz"
                 << "  (native " << juce::String (s.nativeFrames) << " @ " << juce::String (s.nativeRate, 0) << " Hz"
                 << (s.trimmed ? ", TRIMMED)" : ")");
        }
        else
        {
            line << "empty";
        }

        if (s.loading)
            line << "  |  loading...";
        if (s.lastError.isNotEmpty())
            line << "  |  error: " << s.lastError;
        return line;
    }
}

//==============================================================================
DevLoaderEditor::DevLoaderEditor (GranulatorInstrumentProcessor& p)
    : juce::AudioProcessorEditor (p),
      proc (p),
      generic (p)
{
    for (int s = 0; s < kSlots; ++s)
    {
        auto& b = loadButtons[(size_t) s];
        b.setButtonText ("Load slot " + juce::String (s + 1));
        b.onClick = [this, s] { chooseFileForSlot (s); };
        addAndMakeVisible (b);

        auto& l = statusLines[(size_t) s];
        l.setJustificationType (juce::Justification::centredLeft);
        l.setMinimumHorizontalScale (0.6f);
        addAndMakeVisible (l);
    }

    addAndMakeVisible (generic);

    refreshStatus();
    setResizable (true, true);
    setResizeLimits (600, 400, 2400, 2000);
    setSize (kWidth, kHeight);
    startTimerHz (10);
}

DevLoaderEditor::~DevLoaderEditor()
{
    stopTimer();
    // ~FileChooser nulls its async callback: a dialog still open when the
    // editor closes can no longer call back into this object (P14).
    chooser.reset();
    pendingSlot = -1;
}

//==============================================================================
void DevLoaderEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const int headerBottom = kMargin + kHeaderRows * (kRowHeight + kRowGap);
    g.setColour (getLookAndFeel().findColour (juce::Label::textColourId).withAlpha (0.25f));
    g.drawHorizontalLine (headerBottom, (float) kMargin, (float) (getWidth() - kMargin));
}

void DevLoaderEditor::resized()
{
    auto area = getLocalBounds().reduced (kMargin);

    for (int s = 0; s < kSlots; ++s)
    {
        auto row = area.removeFromTop (kRowHeight);
        loadButtons[(size_t) s].setBounds (row.removeFromLeft (kButtonW));
        row.removeFromLeft (kMargin);
        statusLines[(size_t) s].setBounds (row);
        area.removeFromTop (kRowGap);
    }

    area.removeFromTop (kMargin / 2);
    generic.setBounds (area);
}

//==============================================================================
void DevLoaderEditor::chooseFileForSlot (int slot)
{
    if (slot < 0 || slot >= kSlots)
        return;

    // Re-made on every click: the previous chooser (and its callback) dies here.
    pendingSlot = slot;
    chooser = std::make_unique<juce::FileChooser> ("Load slot " + juce::String (slot + 1),
                                                   juce::File(),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");

    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync (flags, [safe = juce::Component::SafePointer<DevLoaderEditor> (this), slot] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;                                  // editor gone (belt and braces; the dtor nulled the callback)

        const juce::File f = fc.getResult();
        if (f == juce::File())
            return;                                  // cancelled / empty result

        if (safe->pendingSlot == slot)
            safe->pendingSlot = -1;
        safe->proc.requestSlotLoad (slot, f);
        safe->refreshStatus();
    });
}

void DevLoaderEditor::timerCallback()
{
    refreshStatus();
}

void DevLoaderEditor::refreshStatus()
{
    for (int s = 0; s < kSlots; ++s)
    {
        const auto text = describe (s, proc.getSlotInfo (s));
        if (text != lastStatus[(size_t) s])
        {
            lastStatus[(size_t) s] = text;
            statusLines[(size_t) s].setText (text, juce::dontSendNotification);
        }
    }
}
