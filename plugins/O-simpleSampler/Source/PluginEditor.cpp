/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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

    O-simpleSampler - Plugin Editor (implementation)

    Stage 3 (GUI): Ouaricon "Keyboard Sampler Field Guide" WebView UI. 17 slider
    relays (region / loop / pitch / vintage / filter / amp / output) + 2 combo
    relays (loopMode, pitchMode) + 1 toggle relay (reverse) bound
    two-way to the APVTS. Eight native functions bridge the JS drag-drop / picker /
    waveform thumbnail / on-screen keyboard / concept-preset tour to the processor
    (which decodes/resamples/publishes the source OFF the audio thread, queues UI
    MIDI through a MidiMessageCollector, and snapshots factory presets into the APVTS).

    Phase 3.1: layout + 20-param binding + load-your-own + keyboard. Phase 3.2: the
    30 Hz Timer viz push (interactive waveform editor + filter curve + amp-ADSR +
    scope). Phase 3.3: per-control tooltips + the applyFactoryPreset preset-tour hook.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // UI resources (distinct from the samples' BinaryData.h)

// ── Resource provider ───────────────────────────────────────────────────────
// The WebView2/WKWebView resource callback receives a BARE PATH ("/", "/index.html",
// "/js/app.js", ...). Compare by direct string equality — never strip a scheme/host
// (a bare path has none, which would collapse every lookup to an empty string).
namespace
{
    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OSimpleSamplerAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities
    // (fleurons, en-dashes, arrows); a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    // Shared drag-drop streaming module (base64 primitives) — imported by app.js
    // as ./modules/webview-drop-streaming.js (relative to /js/app.js).
    if (url == "/js/modules/webview-drop-streaming.js")
        return makeBinaryResource (UIBinaryData::webviewdropstreaming_js,
                                   UIBinaryData::webviewdropstreaming_jsSize, "application/javascript; charset=utf-8");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleSamplerAudioProcessorEditor::OSimpleSamplerAudioProcessorEditor (OSimpleSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleSampler::ParamIDs;

    // 20 APVTS params = 17 sliders + 2 combos + 1 toggle. NB: the string IDs
    // "start"/"end" are carried by the C++ identifiers regionStart/regionEnd (the
    // juce::end shadow fix); the relays/attachments/DOM all use the STRING ids.
    const juce::StringArray sliderIds {
        regionStart, regionEnd, loopStart, loopEnd, loopCrossfade,
        rootKey, tune, fine,
        vintage,
        filterCutoff, filterResonance,
        ampAttack, ampDecay, ampSustain, ampRelease, velToAmp,
        outputLevel
    };
    const juce::StringArray comboIds  { loopMode, pitchMode };
    const juce::StringArray toggleIds { reverse };

    // 1. RELAYS (before the WebView) ----------------------------------------
    for (const auto& id : sliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : comboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));
    for (const auto& id : toggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration -------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── Native functions ───────────────────────────────────────────────────
    // Source drag-drop (Start -> Chunk -> Commit) + file-picker + on-screen
    // keyboard. Every name here has a matching JS getNativeFunction call AND a
    // processor method (grep-clean — a dead native fn passes build/auval silently).
    options = options
        .withNativeFunction ("dropSampleStart", [this] (const juce::Array<juce::var>& args, auto complete) {
            const juce::String sessionId = args.size() > 0 ? args[0].toString() : juce::String();
            const juce::String name      = args.size() > 1 ? args[1].toString() : juce::String();
            complete (juce::var (processorRef.dropSampleStart (sessionId, name)));
        })
        .withNativeFunction ("dropSampleChunk", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 3)
                complete (juce::var (processorRef.dropSampleChunk (
                    args[0].toString(), args[1].toString(), args[2].toString())));
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("dropSampleCommit", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
                complete (juce::var (processorRef.dropSampleCommit (
                    args[0].toString(),
                    args.size() > 1 ? args[1].toString() : juce::String(),
                    args.size() > 2 ? args[2].toString() : juce::String())));
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("loadSourceFromFileChooser", [this] (auto&, auto complete) {
            processorRef.loadSourceFromFileChooser();   // async; opens its own FileChooser
            complete (juce::var (true));
        })
        // Whether the last load was truncated to the 30 s source cap (UI notice).
        .withNativeFunction ("wasLastLoadTruncated", [this] (auto&, auto complete) {
            complete (juce::var (processorRef.wasLastLoadTruncated()));
        })
        // On-screen keyboard → synth. args: [noteNumber, isNoteOn, velocity?].
        // Queued through the processor's MidiMessageCollector and merged into
        // processBlock's MIDI stream (identical path to host MIDI).
        .withNativeFunction ("uiMidi", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
                processorRef.handleUiMidi ((int) args[0], (bool) args[1],
                                           args.size() >= 3 ? (float) args[2] : 0.8f);
            complete (juce::var());
        })
        // Source min/max envelope for the Phase-3.2 waveform-editor background. The
        // JS fetches it on load + at boot (NOT per frame); a flat [min,max,…] var
        // array in [-1,1] (512 pairs). Read-only message-thread snapshot — the audio
        // thread is untouched.
        .withNativeFunction ("getSourceThumbnail", [this] (const juce::Array<juce::var>& args, auto complete) {
            const int numPairs = (args.size() > 0 && (int) args[0] > 0) ? (int) args[0] : 512;
            const auto env = processorRef.getSourceThumbnail (numPairs);
            juce::Array<juce::var> arr;
            arr.ensureStorageAllocated ((int) env.size());
            for (float v : env) arr.add (v);
            complete (juce::var (std::move (arr)));
        })
        // Concept-preset tour (FUNC-07). Forwards the button label to the processor's
        // full-APVTS snapshot; the relays/attachments sync every knob/combo/toggle
        // back to the page automatically (no DOM poking). Preset param VALUES are
        // authored in Stage 4 — the hook resets-to-defaults + resyncs the UI today.
        .withNativeFunction ("applyFactoryPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                processorRef.applyFactoryPreset (args[0].toString());
            complete (juce::var (true));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder MUST be a writable temp dir (DAW hosts deny the
    // default location → silent IE fallback → blank page). Unique prefix per plugin.
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("O-simpleSampler_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor + nullptr undoManager) ----
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control; catch in debug
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < comboIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (comboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < toggleIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (toggleIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            toggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *toggleRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (980, 720);   // headline waveform + 7-group rack + keyboard; frame scrolls on shorter screens
    startTimerHz (30);    // plumbing for Phase 3.2 viz push (body is an empty stub now)
}

OSimpleSamplerAudioProcessorEditor::~OSimpleSamplerAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer (30 Hz): push the live viz events ──────────────────────────────────
// Everything here runs on the MESSAGE thread. The audio thread only copies into
// the VizRing (lock-free); the FFT + scope downsample happen inside
// vizAnalyzer.process below (PERF-01 — no FFT/alloc on the audio thread). The
// analyzer copies the scope window BEFORE its in-place FFT (SamplerVizAnalyzer.h).
// Three pushes: the live waveform playhead, the filter-response curve params, and
// the output scope. The amp-ADSR shape is reconstructed JS-side from the 4 amp
// params (no per-frame push) and its dot is gated by playheadUpdate note-activity.
void OSimpleSamplerAudioProcessorEditor::timerCallback()
{
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());

    if (webView == nullptr)
        return;

    // 1. Live waveform playhead (UI-01) — lead-voice read position, normalized
    //    [0,1] over the live [start,end] region (0 when nothing sounds). The JS
    //    overlays it on the source waveform; Repitch vs Stretch differ purely in
    //    HOW this value advances (pitch-coupled vs ~1×), which is the UI-02 cue.
    webView->emitEventIfBrowserIsVisible ("playheadUpdate",
                                          juce::var (processorRef.getDisplayPlayhead()));

    // 2. Filter-response curve (QUAL-02) — push the lead-voice cutoff/k (= 1/Q =
    //    JUCE's R2) + the sample rate. The JS draws the closed-form 12 dB/oct LP
    //    magnitude with the SAME Ω = tan(πf/fs)/g math the audio thread uses
    //    (SubFilterCurve::magnitudeDb semantics), so the curve matches what is heard.
    {
        auto* curve = new juce::DynamicObject();
        curve->setProperty ("cutoffHz", (double) processorRef.getDisplayCutoffHz());
        curve->setProperty ("k",        (double) processorRef.getDisplayK());
        curve->setProperty ("sr",       processorRef.getCurrentSampleRate());
        webView->emitEventIfBrowserIsVisible ("filterCurveUpdate", juce::var (curve));
    }

    // 3. Output scope (UI polish) — 128 pts in [-1,1]. The narrow OUTPUT cell shows
    //    the post-gain oscilloscope (the analyzer's FFT/spectrum is computed but the
    //    single output canvas renders the scope — clearest at this size).
    {
        const auto& scope = vizAnalyzer.getScope();   // 128 pts, [-1, 1]
        juce::Array<juce::var> scopeArr;
        scopeArr.ensureStorageAllocated ((int) scope.size());
        for (float v : scope) scopeArr.add (v);
        webView->emitEventIfBrowserIsVisible ("scopeUpdate", juce::var (std::move (scopeArr)));
    }
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleSamplerAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleSamplerAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
