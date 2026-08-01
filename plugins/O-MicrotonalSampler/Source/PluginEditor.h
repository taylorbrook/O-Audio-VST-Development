/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    O-MicrotonalSampler - Editor (Phase 3.1: WebView shell)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 3.1 replaces the Phase 2.2 placeholder editor with a WebView-based
    UI that delivers the Stage 3 surface: tabbed shell (Sample Map / Tuning
    / About), 7 APVTS sliders bound via WebSliderRelay/Attachment, embedded
    read-only TuningPanel (carried verbatim from O-Bells), and a sampleMap
    JSON broadcast scaffold ready for the Phase 3.2 grid.

    ⚠️ MEMBER ORDER ⚠️
    Members destroy in REVERSE declaration order. Attachments call
    evaluateJavascript() during destruction, so the WebView must still exist
    when attachments die. Therefore: relays → webView → attachments.

    Cross-platform WebView (memory):
      - JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 must be defined (CMake).
      - withUserDataFolder() is mandatory on Windows for plugin host hosts.
      - Resource provider receives PATHS, not URLs — direct equality checks.

    The editor also implements juce::FileDragAndDropTarget. The skeleton
    in 3.1 returns silently; full routing lands in Phase 3.3.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "WebViewDropStreaming.h"  // v1.13.0 — shared module (ARCH-02)
#include "DropRouting.h"           // v1.23.5 — pure drop hit-test (IN-02/03)

class OMicrotonalSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               public juce::FileDragAndDropTarget,
                                               private juce::Timer
{
public:
    explicit OMicrotonalSamplerAudioProcessorEditor (OMicrotonalSamplerAudioProcessor& p);
    ~OMicrotonalSamplerAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized() override;

    // FileDragAndDropTarget skeletons (full routing in Phase 3.3 per RESEARCH §RQ3-6).
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped           (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter          (const juce::StringArray& files, int x, int y) override;
    void fileDragMove           (const juce::StringArray& files, int x, int y) override;
    void fileDragExit           (const juce::StringArray& files) override;

    // v1.7.1: 30 Hz tick — diff the synth's active-notes bitmask against the
    // previous snapshot and push tuningNoteOn / tuningNoteOff events plus a
    // tuningHeldNotes payload (notes + frequencies) to the WebView so the
    // TuningPanel Circle / Polar / TrueKeys views can highlight in real time.
    void timerCallback() override;

private:
    OMicrotonalSamplerAudioProcessor& processorRef;

    // ============================================================
    // 0. DROP-SESSION MANAGER  (must outlive WebView — its native-function
    //    lambdas are captured by the WebView's NativeFunction registry)
    //
    // v1.13.0: WKWebView drag-drop content-streaming pattern extracted to
    // the shared modules/core/webview-drop-streaming module. The 4
    // dropSession* native functions, session lifecycle, 5-min stale-temp-dir
    // reaper, and DropSessionGuard validators all live there now. This
    // editor just constructs one SessionManager with two commit callbacks
    // bridging to processor.loadSampleFolder / loadSingleSample, and
    // splices its native functions into the registry below.
    // See REVIEW-architecture.md §"Extract drag-drop streaming → shared module".
    // ============================================================
    std::unique_ptr<Ouaricon::WebViewDropStreaming::SessionManager> dropSessions;

    // ============================================================
    // 1. RELAYS FIRST  (no dependencies — must outlive attachments)
    // ============================================================
    std::unique_ptr<juce::WebSliderRelay> attackRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> sustainRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseRelay;
    std::unique_ptr<juce::WebSliderRelay> polyphonyRelay;
    std::unique_ptr<juce::WebSliderRelay> velocityCrossfadeRelay;
    std::unique_ptr<juce::WebSliderRelay> expressionRelay;       // v1.7.0
    std::unique_ptr<juce::WebComboBoxRelay> dynamicsModeRelay;   // v1.21.0 (choice)
    std::unique_ptr<juce::WebSliderRelay> dynamicRangeRelay;     // v1.22.0
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // ============================================================
    // 2. WEBVIEW SECOND  (depends on relays via withOptionsFrom)
    // ============================================================
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ============================================================
    // 3. ATTACHMENTS LAST  (depend on both relays and webView)
    // ============================================================
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> polyphonyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> velocityCrossfadeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> expressionAttachment;     // v1.7.0
    std::unique_ptr<juce::WebComboBoxParameterAttachment> dynamicsModeAttachment; // v1.21.0
    std::unique_ptr<juce::WebSliderParameterAttachment> dynamicRangeAttachment;   // v1.22.0
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    // v1.0.2 attempted a JUCE-Component drag overlay here; it failed
    // because WebView OS rendering sits above JUCE Components.
    // v1.0.3 moved drag-drop to the JS layer; v1.0.4 streams file
    // content through the bridge into a session-scoped temp dir
    // because WKWebView strips file paths from JS DataTransfer.
    // v1.13.0: pattern lives in modules/core/webview-drop-streaming;
    // session state, the 5-min reaper, and DropSessionGuard validators
    // are owned by the SessionManager declared above (#0).

    // Resource provider — explicit URL→BinaryData mapping (memory pattern,
    // matches O-Bells PluginEditor.cpp:941-998).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // v1.12.4: Data-driven native function registry. Replaces 42 inline
    // `.withNativeFunction(...)` chained calls in the constructor (~1400
    // lines of organic v1.5.0–v1.12.0 growth). The constructor iterates
    // this vector once and forwards each entry to the Options builder.
    // Behaviour is unchanged — every entry preserves its original name,
    // capture list, and body verbatim. See REVIEW-architecture.md §1.
    // Pattern reusable in O-Bells / O-Lyrica (similar editor boilerplate).
    std::vector<std::pair<juce::Identifier, juce::WebBrowserComponent::NativeFunction>>
        buildNativeFunctionRegistry();

    // Phase 3.2-onward — cell layout shadow published by JS via the
    // reportCellLayout native function. 3.1 stores; 3.3 hit-tests on drop.
    // v1.23.5: CellRect + hit-test moved to DropRouting.h (unit-testable).
    juce::Array<oms::CellRect> cellLayout;
    juce::Rectangle<int>       folderZoneRect;

    // v1.7.1: previous active-notes bitmask snapshot (MIDI 0..63 / 64..127).
    // The 30 Hz timerCallback diffs the current snapshot against these to
    // emit per-note on/off events. Initialised to zero so the first tick
    // emits note-ons for any notes already held at editor open.
    juce::uint64 prevActiveNotesLow  = 0;
    juce::uint64 prevActiveNotesHigh = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OMicrotonalSamplerAudioProcessorEditor)
};
