/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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

    O-simpleBeatmaker - Plugin Editor (implementation)

    Stage 3 (GUI): WebView UI. 29 slider relays + 1 combo relay + 12 toggle relays
    bound two-way to the APVTS; the 6x32 step grid driven through native functions
    (setStep / getGrid / clearGrid); the lock-free VizAnalyzer drained on a 60 Hz
    Timer into one "frame" event (playhead + this frame's hits + transport).

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

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
OSimpleBeatmakerAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities (♪, en-dashes,
    // ▲) for the grid marks and labels; a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (BinaryData::styles_css, BinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (BinaryData::app_js, BinaryData::app_jsSize, "application/javascript; charset=utf-8");

    // v1.1.0 — the interface copy table (English + French). Served from the same
    // provider as app.js, which imports it; a branch missing here is a BLANK page
    // rather than an English one, because the import fails to resolve and module
    // evaluation never starts. check-i18n assertion 8 checks both halves.
    if (url == "/js/i18n.js")
        return makeBinaryResource (BinaryData::i18n_js, BinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleBeatmakerAudioProcessorEditor::OSimpleBeatmakerAudioProcessorEditor (OSimpleBeatmakerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleBeatmaker;
    using namespace OSimpleBeatmaker::ParamIDs;

    // ── Build the param-ID lists (single source of truth: BeatmakerIDs) ──────
    // 29 sliders: 5 global + 4 per voice; 1 combo; 12 toggles: 2 per voice.
    sliderIds = { swing, humanize, quantizeStrength, tempo, outputLevel };
    for (int v = 0; v < kNumVoices; ++v)
    {
        sliderIds.add (voiceParamID (v, sufTune));
        sliderIds.add (voiceParamID (v, sufDecay));
        sliderIds.add (voiceParamID (v, sufTone));
        sliderIds.add (voiceParamID (v, sufLevel));
    }
    comboIds = { patternLength };
    for (int v = 0; v < kNumVoices; ++v)
    {
        toggleIds.add (voiceParamID (v, sufMute));
        toggleIds.add (voiceParamID (v, sufSolo));
    }

    // 1. RELAYS (before the WebView) -----------------------------------------
    for (const auto& id : sliderIds) sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : comboIds)  comboRelays .push_back (std::make_unique<juce::WebComboBoxRelay> (id));
    for (const auto& id : toggleIds) toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration --------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& r : sliderRelays) options = options.withOptionsFrom (*r);
    for (const auto& r : comboRelays)  options = options.withOptionsFrom (*r);
    for (const auto& r : toggleRelays) options = options.withOptionsFrom (*r);

    // ── Step-grid native functions (WebView ↔ std::atomic grid) ─────────────
    options = options
        // setStep(voice, step, velocity) — 0 = off, 1..127 = on@velocity.
        .withNativeFunction ("setStep", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 3)
                processorRef.setStep ((int) args[0], (int) args[1], (int) args[2]);
            complete (juce::var());
        })
        // getGrid() — flat row-major 6×32 velocity array for paint-on-load / restore.
        .withNativeFunction ("getGrid", [this] (auto&, auto complete) {
            using namespace OSimpleBeatmaker;
            juce::Array<juce::var> cells;
            cells.ensureStorageAllocated (kNumVoices * kMaxSteps);
            for (int v = 0; v < kNumVoices; ++v)
                for (int s = 0; s < kMaxSteps; ++s)
                    cells.add (processorRef.getStep (v, s));
            complete (juce::var (cells));
        })
        .withNativeFunction ("clearGrid", [this] (auto&, auto complete) {
            processorRef.clearGrid();
            complete (juce::var());
        })
        // applyPreset(index) — load a concept-isolating factory preset (FUNC-05):
        // sets the timing-feel params (host-notifying → knobs update) + stamps the
        // grid. JS calls refreshGridFromBackend() after to repaint the velocities.
        .withNativeFunction ("applyPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
                processorRef.applyConceptPreset ((int) args[0]);
            complete (juce::var());
        })
        // Sample rate for the timing lane's tempo-normalised Δt (samples → step fraction).
        .withNativeFunction ("getSampleRate", [this] (auto&, auto complete) {
            complete (processorRef.getCurrentSampleRate());
        })
        // getParameterDefaults() — { paramID: normalisedDefault } map for the knobs'
        // dblclick-to-default (project pattern: propertiesChanged carries no default).
        .withNativeFunction ("getParameterDefaults", [this] (auto&, auto complete) {
            auto* obj = new juce::DynamicObject();
            for (auto* p : processorRef.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                    obj->setProperty (rp->paramID, rp->getDefaultValue());
            complete (juce::var (obj));
        })
        // ── Interface-language pair (v1.1.0) ───────────────────────────────
        // Plain withNativeFunction, no relay: the language is not a parameter
        // and must not reach a DAW automation lane. PULLED once by the page at
        // init; nothing pushes from here or from the 60 Hz frame timer, and
        // applyConceptPreset() sets parameters and grid cells only, so no lesson
        // preset can change it behind the page's back.
        //
        // The WebView is never hidden in this editor (no setVisible anywhere in
        // Source/), so these completions always settle
        // (critical_webview_completion_gated_on_isvisible).
        .withNativeFunction ("getUiLanguage", [this] (auto&, auto complete) {
            complete (juce::var (OSimpleBeatmakerAudioProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        })
        .withNativeFunction ("setUiLanguage", [this] (const juce::Array<juce::var>& args, auto complete) {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OSimpleBeatmakerAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (OSimpleBeatmakerAudioProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

   #if JUCE_WINDOWS
    // DAW hosts often deny WebView2 its default user-data folder → silent IE fallback
    // → blank page. Pin it to a temp dir (project memory: Windows WebView2 data folder).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimpleBeatmaker_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView) -------------------------------------
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead knob; catch in debug
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (*param, *sliderRelays[(size_t) i], nullptr));
    }
    for (int i = 0; i < comboIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (comboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (*param, *comboRelays[(size_t) i], nullptr));
    }
    for (int i = 0; i < toggleIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (toggleIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            toggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (*param, *toggleRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    setResizeLimits (860, 640, 1920, 1400);
    setSize (1060, 900);   // wide enough to seat a 16-step grid; the grid scrolls at 32 steps
    startTimerHz (60);
}

OSimpleBeatmakerAudioProcessorEditor::~OSimpleBeatmakerAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer: drain the viz ring, push one frame snapshot to the page ──────────
void OSimpleBeatmakerAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto& viz = processorRef.getVizAnalyzer();
    const int got = viz.drain (vizScratch.data(), (int) vizScratch.size());

    // hits emitted since the last frame (QUAL-02: each carries the Δt baked into
    // the audio — applied − nominal — never a UI recomputation of the feel formula).
    juce::Array<juce::var> hits;
    hits.ensureStorageAllocated (got);
    for (int i = 0; i < got; ++i)
    {
        const auto& ev = vizScratch[(size_t) i];
        auto* h = new juce::DynamicObject();
        h->setProperty ("v",   (int) ev.voiceIndex);
        h->setProperty ("s",   (int) ev.stepIndex);
        h->setProperty ("vel", (int) ev.velocity);
        h->setProperty ("src", (int) ev.source);
        h->setProperty ("d",   (int) (ev.appliedSampleInBar - ev.nominalSampleInBar));
        hits.add (juce::var (h));
    }

    auto* frame = new juce::DynamicObject();
    frame->setProperty ("ph",   (double) viz.getPlayheadStepPhase());
    frame->setProperty ("bpm",  processorRef.getLastKnownBpm());
    frame->setProperty ("sr",   processorRef.getCurrentSampleRate());   // live: tracks host SR changes
    frame->setProperty ("sync", processorRef.isHostSynced());
    frame->setProperty ("hits", juce::var (hits));

    webView->emitEventIfBrowserIsVisible ("frame", juce::var (frame));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleBeatmakerAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleBeatmakerAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
