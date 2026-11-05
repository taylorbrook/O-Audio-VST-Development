/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

// Stage 1: createEditor() returns juce::GenericAudioProcessorEditor until
// Stage 3 adds the WebView editor (.planning/mockups/v2-PluginEditor.h, class
// GranulatorEditor) here. What already lives here is the editor's resource map
// over the embedded UI (UIBinaryData), so the binary-data target, the served
// paths and check-i18n [8] are settled now and Stage 3 only wires
// withResourceProvider() to it. Plugin target only (consoles skip this TU).

#include <JuceHeader.h>
#include <optional>

/** Resource provider for the WebView: `url` is a BARE PATH ("/", "/js/i18n.js"),
    never a full URL. std::nullopt = 404. */
std::optional<juce::WebBrowserComponent::Resource> granulatorUiResource (const juce::String& url);
