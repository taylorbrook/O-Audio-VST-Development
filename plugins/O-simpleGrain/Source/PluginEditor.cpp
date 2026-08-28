/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain - Plugin Editor (implementation)

    Stage 3 (GUI): Ouaricon "Granular Field Guide" WebView UI. 15 slider relays
    (grain / spray-scatter / amp ADSR / output) + 2 combo relays (sourceSample,
    windowShape) + 2 toggle relays (freeze, adsrEnabled) bound two-way to the
    APVTS. The five
    fixed-name source native functions (dropSessionStart/AddFile/CommitFile/
    CommitFolder, loadSourceFromFileChooser) bridge the JS drag-drop / picker to
    the processor's already-built decode path.

    Phase 3.1: layout + 18-param binding + load-your-own wiring. The 30 Hz Timer
    body (four visualizations + readout) lands in Phase 3.2 — it is an empty stub
    here.

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
OSimpleGrainAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities
    // (fleurons, en-dashes, arrows); a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize, "application/javascript; charset=utf-8");

    // v1.3.0 — the interface copy table (English + French). Served from the same
    // provider as app.js, which imports it; a branch missing here is a BLANK page
    // rather than an English one, because the import fails to resolve and module
    // evaluation never starts. NOTE the namespace: the UI resources live in
    // UIBinaryData (the O-simpleGrain_Samples target owns BinaryData:: for the
    // four embedded .wav sources), so this is UIBinaryData::i18n_js.
    if (url == "/js/i18n.js")
        return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

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

    if (url == "/img/insects.png")
        return makeBinaryResource (UIBinaryData::insects_png, UIBinaryData::insects_pngSize, "image/png");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleGrainAudioProcessorEditor::OSimpleGrainAudioProcessorEditor (OSimpleGrainAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleGrain::ParamIDs;

    // 19 APVTS params = 15 float sliders + 2 combos + 2 toggles.
    // (sourceSample / windowShape are choices; freeze / adsrEnabled are bools — NOT sliders.)
    const juce::StringArray sliderIds {
        grainSize, density, position, scan,
        pitchSpray, positionSpray, scatter, grainPitch, panSpray, velToDensity,
        ampAttack, ampDecay, ampSustain, ampRelease,
        outputLevel
    };
    const juce::StringArray comboIds  { sourceSample, windowShape };
    const juce::StringArray toggleIds { freeze, adsrEnabled };

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
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        // Engine constants pushed once (IN-05): the JS grain meter reads
        // window.__JUCE__.initialisationData.grainCap[0] instead of duplicating
        // kGlobalGrainCap — a C++ change can no longer desync the readout.
        .withInitialisationData ("grainCap", OSimpleGrainAudioProcessor::kGlobalGrainCap);

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── Native functions ───────────────────────────────────────────────────
    // Source drag-drop + file-picker. The NAMES ARE FIXED by the shared module
    // and the processor's C++ handlers — do NOT rename. Each forwards to the
    // matching processor method (which decodes/resamples/publishes off the audio
    // thread) and completes with the returned bool ("ok"); the JS toasts on false.
    options = options
        .withNativeFunction ("dropSessionStart", [this] (const juce::Array<juce::var>& args, auto complete) {
            const juce::String sessionId  = args.size() > 0 ? args[0].toString() : juce::String();
            const juce::String folderName = args.size() > 1 ? args[1].toString() : juce::String();
            complete (juce::var (processorRef.dropSessionStart (sessionId, folderName)));
        })
        .withNativeFunction ("dropSessionAddFile", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 3)
                complete (juce::var (processorRef.dropSessionAddFile (
                    args[0].toString(), args[1].toString(), args[2].toString())));
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("dropSessionCommitFile", [this] (const juce::Array<juce::var>& args, auto complete) {
            // grain signature: (sessionId, filename, base64) — NOT (sessionId, name, midi, vel).
            if (args.size() >= 3)
                complete (juce::var (processorRef.dropSessionCommitFile (
                    args[0].toString(), args[1].toString(), args[2].toString())));
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("dropSessionCommitFolder", [this] (const juce::Array<juce::var>& args, auto complete) {
            const juce::String sessionId = args.size() > 0 ? args[0].toString() : juce::String();
            complete (juce::var (processorRef.dropSessionCommitFolder (sessionId)));
        })
        .withNativeFunction ("loadSourceFromFileChooser", [this] (auto&, auto complete) {
            processorRef.loadSourceFromFileChooser();   // async; opens its own FileChooser
            complete (juce::var (true));
        })
        // Whether the last load was truncated to the 10 s source cap (UI notice).
        .withNativeFunction ("wasLastLoadTruncated", [this] (auto&, auto complete) {
            complete (juce::var (processorRef.wasLastLoadTruncated()));
        })
        // Sample rate for the spectrum's frequency-axis labels (used in 3.2).
        .withNativeFunction ("getSampleRate", [this] (auto&, auto complete) {
            complete (processorRef.getCurrentSampleRate());
        })
        // Source min/max envelope for the UI-02 waveform background. The JS fetches
        // it on load + at boot (not per frame); a flat [min,max,…] var array in
        // [-1,1] (~512 pairs). Read-only message-thread snapshot — audio untouched.
        .withNativeFunction ("getSourceThumbnail", [this] (const juce::Array<juce::var>& args, auto complete) {
            const int numPairs = (args.size() > 0 && (int) args[0] > 0) ? (int) args[0] : 512;
            const auto env = processorRef.getSourceThumbnail (numPairs);
            juce::Array<juce::var> arr;
            arr.ensureStorageAllocated ((int) env.size());
            for (float v : env) arr.add (v);
            complete (juce::var (std::move (arr)));
        })
        // Concept-preset tour (FUNC-06). Forwards the button label to the
        // processor's full-APVTS snapshot; the relays/attachments sync every
        // knob/combo/toggle back to the page automatically (no DOM poking).
        .withNativeFunction ("applyFactoryPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                processorRef.applyFactoryPreset (args[0].toString());
            complete (juce::var (true));
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
        // ── Interface-language pair (v1.3.0) ───────────────────────────────
        // Plain withNativeFunction, no relay: the language is not a parameter
        // and must not reach a DAW automation lane. PULLED once by the page at
        // init; nothing pushes from here or from the 30 Hz viz timer, and
        // applyFactoryPreset sets parameters only, so no preset path can change
        // it behind the page's back.
        //
        // The WebView is never hidden in this editor (no setVisible anywhere in
        // Source/), so these completions always settle
        // (critical_webview_completion_gated_on_isvisible).
        .withNativeFunction ("getUiLanguage", [this] (auto&, auto complete) {
            complete (juce::var (OSimpleGrainAudioProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        })
        .withNativeFunction ("setUiLanguage", [this] (const juce::Array<juce::var>& args, auto complete) {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OSimpleGrainAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (OSimpleGrainAudioProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder MUST be a writable temp dir (DAW hosts deny the
    // default location → silent IE fallback → blank page). Unique prefix so the
    // stale-session reaper can't touch another plugin's in-flight sessions.
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimpleGrain_WebView"))
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

    // Snapshot the source version at open — the page's boot fetch covers the
    // current source; the timer only signals decodes that happen after this.
    lastSourceVersion = processorRef.getSourceVersion();

    setSize (900, 760);   // 2×2 grid + side rail + keyboard; the frame scrolls on shorter screens
    startTimerHz (30);
}

OSimpleGrainAudioProcessorEditor::~OSimpleGrainAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer (30 Hz): run the analyzer + push the four viz events ───────────────
// Everything here runs on the MESSAGE thread. The audio thread only copies into
// the VizRing / fills the GrainCloudFrame (lock-free); the FFT + scope downsample
// happen inside vizAnalyzer.process below (PERF-01 / invariant 7). The analyzer
// copies the scope window BEFORE its in-place FFT (invariant 8 — VizAnalyzer.h:88).
void OSimpleGrainAudioProcessorEditor::timerCallback()
{
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());

    if (webView == nullptr)
        return;

    // 1. Scope + spectrum (UI-04) — verbatim FM pattern (256 dB bins / 128 pts).
    {
        const auto& spec  = vizAnalyzer.getSpectrum();   // 256 bins, ~[-100, 0] dB
        const auto& scope = vizAnalyzer.getScope();       // 128 pts, [-1, 1]

        juce::Array<juce::var> specArr, scopeArr;
        specArr.ensureStorageAllocated ((int) spec.size());
        scopeArr.ensureStorageAllocated ((int) scope.size());
        for (float v : spec)  specArr.add (v);
        for (float v : scope) scopeArr.add (v);

        webView->emitEventIfBrowserIsVisible ("spectrumUpdate", juce::var (std::move (specArr)));
        webView->emitEventIfBrowserIsVisible ("scopeUpdate",    juce::var (std::move (scopeArr)));
    }

    // 2. Grain cloud + playheads (UI-01/02) — read the published frame, build a
    //    DynamicObject { count, grains:[[readPosNorm,sizeMs,pitchSemis,pan,spawn],…],
    //    playheadNorm, positionNorm, positionSprayNorm, frozen }.
    {
        const GrainCloudFrame& f = processorRef.getGrainCloudBuffer().read();

        juce::Array<juce::var> grains;
        grains.ensureStorageAllocated (f.count);
        for (int i = 0; i < f.count; ++i)
        {
            const GrainEvent& g = f.events[(size_t) i];
            juce::Array<juce::var> tuple;
            tuple.ensureStorageAllocated (5);
            tuple.add (g.readPosNorm);
            tuple.add (g.sizeMs);
            tuple.add (g.pitchSemis);
            tuple.add (g.pan);
            tuple.add (g.spawnSample);
            grains.add (juce::var (std::move (tuple)));
        }

        auto* cloud = new juce::DynamicObject();
        cloud->setProperty ("count",             f.count);
        cloud->setProperty ("grains",            juce::var (std::move (grains)));
        cloud->setProperty ("playheadNorm",      f.playheadNorm);
        cloud->setProperty ("positionNorm",      f.positionNorm);
        cloud->setProperty ("positionSprayNorm", f.positionSprayNorm);
        cloud->setProperty ("frozen",            f.frozen);

        webView->emitEventIfBrowserIsVisible ("grainCloudUpdate", juce::var (cloud));
    }

    // 3. Grain meter (UI-05) — active grain count over kGlobalGrainCap (192).
    webView->emitEventIfBrowserIsVisible ("grainMeterUpdate",
                                          juce::var (processorRef.getActiveGrainCount()));

    // 4. Source-change signal (IN-08) — a successful decodeAndPublish bumped the
    //    processor's version counter; tell the page so it refetches the waveform
    //    thumbnail + truncation status event-driven (the old fixed 300 ms /
    //    1200 ms timers raced slow decodes and long picker browses).
    {
        const auto v = processorRef.getSourceVersion();
        if (v != lastSourceVersion)
        {
            lastSourceVersion = v;
            webView->emitEventIfBrowserIsVisible ("sourceChanged", juce::var());
        }
    }

    // NB: windowInsetUpdate is NOT pushed here — the window only changes on the
    // windowShape combo, so the inset is recomputed JS-side on that change
    // (invariant: no per-frame inset push — Pitfall 4).
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleGrainAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleGrainAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
