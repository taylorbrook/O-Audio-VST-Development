/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop — Plugin Editor

    Stage 1 placeholder: juce::GenericAudioProcessorEditor over the 14-param
    APVTS (O-Bitrot decision). Stage 3 swaps this for the Ouaricon WebView UI —
    the swap is mechanical because this header is only ever included from
    inside an #if JUCE_WEB_BROWSER guard in PluginProcessor.cpp; the Stage-2
    render harness builds the processor with JUCE_WEB_BROWSER=0 and no editor
    sources (pattern_render_harness_breaks_on_webview_editor).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TapestopEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit TapestopEditor(TapestopProcessor&);
    ~TapestopEditor() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapestopEditor)
};
