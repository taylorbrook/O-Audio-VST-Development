/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    PluginEditor.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StrataParamIds.h"
#include "TerrainViewFeed.h"

class OStrataAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit OStrataAudioProcessorEditor (OStrataAudioProcessor&);
    ~OStrataAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OStrataAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member declaration order (C++ destroys in reverse)
    // 1. Relays destroyed LAST
    // 2. WebView destroyed SECOND
    // 3. Attachments destroyed FIRST (WebView still alive — safe)
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS (destroyed last)
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;   // 8
    std::unique_ptr<juce::WebToggleButtonRelay> delaySyncRelay;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> lfoSyncRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> lfoFreeRunRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> bypassRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> modSlotToggleRelays;

    // 2. WEBVIEW (destroyed second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first — WebView still alive)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;   // 8
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delaySyncAttachment;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> lfoSyncAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> lfoFreeRunAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> bypassAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> modSlotToggleAttachments;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);

    // Native function registration
    juce::WebBrowserComponent::Options addNativeFunctions (
        juce::WebBrowserComponent::Options options);

    // 30 Hz timer: the five pushed events, every one through emitEventIfBrowserIsVisible —
    // never evaluateJavascript (Stage 3 plan Decisions 8, 35):
    //   heldNotes        every tick, change-gated (TrueKeys)
    //   terrainState     every tick, change-gated — the ring's newest slot {θ, x, y, h}
    //   terrainStatus    even ticks (≤ 15 Hz), struct compare
    //   terrainCycle     even ticks, only when a new COMPLETE cycle exists
    //   terrainHeightmap ticks % 3 (≤ 10 Hz), key-gated — the active surface, 64 × 64 R32F
    // `force` (requestTerrainRepush, or a moved stateGeneration after a preset apply /
    // session restore — Decision 34) bypasses every gate and tick filter once.
    void timerCallback() override;
    void pushHeldNotes (bool force);
    void pushStatus (int osc, bool force);
    void pushCycle (int osc);
    void pushState (int osc, bool force);
    void pushHeightmap (int osc, bool force);
    std::vector<std::pair<int, double>> lastSentNotes;
    OStrataAudioProcessor::TerrainStatus lastStatus[2];
    bool hasLastStatus[2] = { false, false };
    TerrainViewFeed::CycleScratch cycleScratch[2];
    std::array<float, TerrainViewFeed::kCyclePoints * 3> cycleOut {};
    uint32_t lastWriteIndexState[2] = { 0, 0 };
    TerrainViewFeed::Playhead lastState[2];
    bool hasLastState[2] = { false, false };
    uint64_t lastHeightmapKey[2] = { 0, 0 };
    std::array<float, TerrainViewFeed::kHeightmapSize> heightmapOut {};
    uint32_t lastStateGeneration = 0;
    // Page → C++ handshake: `requestTerrainRepush` (boot and __refreshAllControls)
    // sets this; the next tick forces all five pushes (the page-load drop window,
    // RESEARCH §2.5). Written on the message thread by the native fn, read by the timer.
    bool repushPending = false;
    uint32_t tickCount = 0;

    // ─── Round B (plan Decisions 39, 40): PNG import from the page ───
    // A successful import selects Imported… on osc?Terrain from HERE (the processor API
    // never touches a parameter) inside one begin / end gesture — skipped when it already
    // is, so no spurious undo step.
    void selectImportedTerrain (int osc);
    static constexpr juce::int64 kMaxImportBytes = 2 * 1024 * 1024;   // the 2 MiB cap (both natives; the page pre-checks drops)

    // ─── Round B (plan Decision 37): PERF-03 — `reportViewPerf (json)` from the page →
    //     DBG + one line per report in ~/Library/Logs/O-Strata/view-perf.log (the Release
    //     WKWebView has no inspector; the log file is the verdict path). Lazily created. ───
    void logViewPerf (const juce::String& json);
    std::unique_ptr<juce::FileLogger> perfLog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OStrataAudioProcessorEditor)
};
