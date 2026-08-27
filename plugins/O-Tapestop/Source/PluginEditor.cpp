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

    O-Tapestop — Plugin Editor (implementation)

    See PluginEditor.h for the Stage-3 contract summary. The relay/attachment
    machinery, resource provider and WinWebView2 options are the O-ReverseDelay
    patterns; the 30 Hz transportFrame push is the O-Polystutter pattern.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // distinct namespace — see CMakeLists juce_add_binary_data

namespace
{
    // Parameter IDs — must match createParameterLayout() exactly. An ID typo
    // is a silently dead control, so every lookup below is jassert'ed.
    const juce::StringArray kSliderIds {
        "STOP_FREE_MS", "STOP_CURVE",
        "START_FREE_MS", "START_CURVE",
        "ENV_FREE_MS",
        "CONT_RATE_HZ", "CONT_DEPTH", "CONT_CHAOS",
        "TONE_TRACK", "MIX", "OUTPUT_GAIN"
    };

    // MODE, SYNC_MODE and CHARACTER render as segment groups, the four
    // divisions as selects — the relay type is the same either way (all
    // seven are AudioParameterChoice).
    const juce::StringArray kComboIds {
        "MODE", "SYNC_MODE", "CHARACTER",
        "STOP_SYNC_DIV", "START_SYNC_DIV", "ENV_SYNC_DIV",
        "CONT_RATE_SYNC_DIV"
    };

    // ENGAGE is the plugin's only AudioParameterBool and therefore its only
    // WebToggleButtonRelay. The relay TYPE has to match the parameter type —
    // a bool bound through a slider/combo relay attaches cleanly and then
    // never updates.
    const juce::StringArray kToggleIds { "ENGAGE" };

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

// ── Resource provider ───────────────────────────────────────────────────────
// The WKWebView / WebView2 resource callback receives a BARE PATH ("/",
// "/js/app.js", ...) — there is no scheme or host to strip, and stripping one
// would collapse every lookup to an empty string. Match by direct equality
// and never hard-code juce:// vs https://juce.backend
// (critical_webview_resource_provider_and_schemes).
std::optional<juce::WebBrowserComponent::Resource>
TapestopEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page uses UTF-8 entities
    // (fleurons, en-dashes, ×-glyphs) that mojibake without it on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize,
                                   "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize,
                                   "application/javascript; charset=utf-8");

    // v1.5.0 — the hover-help copy table, imported by app.js as './i18n.js'.
    // This branch, the juce_add_binary_data SOURCES entry and the import all
    // land in one commit: any two of the three leaves the page 404ing at
    // runtime with nothing failing at build time.
    if (url == "/js/i18n.js")
        return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize,
                                   "application/javascript; charset=utf-8");

    // Underscore name on purpose: juce_add_binary_data STRIPS hyphens, so an
    // envelope-editor.js would have become `envelopeeditor_js`
    // (critical_binary_data_strips_hyphens).
    if (url == "/js/envelope_editor.js")
        return makeBinaryResource (UIBinaryData::envelope_editor_js,
                                   UIBinaryData::envelope_editor_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared preset-manager module (Stage 4). The symbol drops the filename's
    // hyphen — juce_add_binary_data strips them
    // (critical_binary_data_strips_hyphens).
    if (url == "/modules/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js,
                                   UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/img/shell.png")
        return makeBinaryResource (UIBinaryData::shell_png, UIBinaryData::shell_pngSize,
                                   "image/png");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
TapestopEditor::TapestopEditor (TapestopProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // 1. RELAYS (must exist before the WebView) ------------------------------
    for (const auto& id : kSliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));

    for (const auto& id : kComboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));

    for (const auto& id : kToggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration --------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);

    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);

    // Registers "ENGAGE" in initialisationData.__juce__toggles — the list
    // Juce.getToggleState() checks before it will build a ToggleState. Miss
    // this and the page throws on load rather than merely losing a control.
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── NATIVE FUNCTIONS — exactly 15 ──────────────────────────────────────
    // getParameterDefaults + commitEnvelope + requestEnvelope (Stage 3) +
    // setTooltipsEnabled + getTooltipsEnabled (v1.4.0) + the 10 preset fns
    // below (Stage 4). Grep-diffed against js/app.js AND
    // modules/preset-manager.js at the gate: an unregistered fn leaves its
    // control silently dead while build, auval and pluginval all pass
    // (pattern_webview_native_fn_bridge_gap).

    // Dblclick-reset needs each parameter's default in ENGINEERING units: the
    // properties payload pushed to the page carries start/end/skew but no
    // default, and a hardcoded JS default table would drift from the C++
    // NormalisableRange (pattern_webview_knob_readout_scaled_value). The
    // three FREE_MS knobs are skewed 0.35, so this round-trip is what keeps
    // their resets exact.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();

            for (const auto& id : kSliderIds)
            {
                if (auto* param = processorRef.parameters.getParameter (id))
                    obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            }

            complete (juce::var (obj));
        });

    // Envelope commit (UI-01). Completes SYNCHRONOUSLY with the sanitized
    // toJson() so the page redraws the post-sanitize truth (clamps, sort,
    // endpoint pins applied) — any JS/C++ disagreement resolves to C++.
    // Capturing `this` is safe: the completion never outlives this call
    // (Stage-4 FileChoosers are where SafePointer becomes mandatory).
    options = options.withNativeFunction ("commitEnvelope",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString())
            {
                complete (juce::var (processorRef.getScratchEnvelopeJson()));
                return;
            }

            processorRef.commitScratchEnvelopeJson (args[0].toString());

            // The commit itself bumped uiEnvGeneration; swallow it here so
            // the timer does not push a redundant envelopeState right after
            // this echo (both run on the message thread — no race).
            lastEnvGeneration = processorRef.uiEnvGeneration.load (std::memory_order_relaxed);

            complete (juce::var (processorRef.getScratchEnvelopeJson()));
        });

    // Page init: the current envelope (session restore / editor reopen).
    options = options.withNativeFunction ("requestEnvelope",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.getScratchEnvelopeJson()));
        });

    // ── HOVER-HELP PREFERENCE — 2 (v1.4.0) ─────────────────────────────────
    // The "?" toggle's state is a UI preference, not a parameter, so it round-
    // trips through these two fns and the processor's state property rather
    // than through a relay. Both complete synchronously.

    options = options.withNativeFunction ("setTooltipsEnabled",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                processorRef.tooltipsEnabled.store ((bool) args[0],
                                                    std::memory_order_release);

            complete (juce::var (processorRef.tooltipsEnabled.load (
                                     std::memory_order_acquire)));
        });

    // PULLED by the page at init, never pushed. A push from the constructor or
    // from the first timer tick fires before js/app.js has evaluated, so the
    // preference silently never arrives and the toggle reads OFF on every
    // reopen — the O-FreqPulse WR-01 bug, avoided here by construction.
    options = options.withNativeFunction ("getTooltipsEnabled",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.tooltipsEnabled.load (
                                     std::memory_order_acquire)));
        });

    // ── v1.5.0: the hover-help LANGUAGE pair ────────────────────────────────
    //
    // Same shape and same discipline as the toggle pair above: plain
    // withNativeFunction, no relay, PULLED once by the page at init. No push
    // from this constructor, no timer, no poll().then(poll) and no revision
    // counter — the language is not preset content, and
    // OuariconPresetManager::loadPreset walks preset["parameters"] only, so no
    // preset path can change it behind the page's back.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (TapestopProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (auto& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    TapestopProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (TapestopProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    // ── PRESET NATIVE FUNCTIONS — 10 (Stage 4) ─────────────────────────────
    // Exactly the names modules/preset-manager.js requests; total registered
    // surface is now 15 and the grep-diff parity gate runs at 15↔15. The
    // synchronous eight capture `this` (completion never outlives the call).
    // The two DIALOG fns defer their completion into a FileChooser callback:
    // shared_ptr chooser captured into its own callback, SafePointer HOISTED
    // to a local (MSVC rejects SafePointer(this) init-captures in nested
    // lambdas), and on a dead editor the callback RETURNS — even
    // complete(false) would UAF the dead WebView impl
    // (pattern_webview_launchasync_safepointer_no_complete). Both dialog fns
    // complete with {success, name} OBJECTS — the JS reads result.success /
    // result.name; a bare bool reads as failure even when the file was
    // written.

    options = options.withNativeFunction ("savePreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.savePreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    options = options.withNativeFunction ("savePresetWithDialog",
        [this] (auto&, auto complete)
        {
            auto userDir = processorRef.presetManager.getUserPresetsDirectory();
            userDir.createDirectory();

            auto chooser = std::make_shared<juce::FileChooser> ("Save Preset", userDir, "*.json");
            juce::Component::SafePointer<TapestopEditor> safeThis (this);

            chooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        // savePresetToFile HONORS the chosen path (the
                        // O-DigiDelay bug was a dialog whose destination a
                        // savePreset(name) call ignored).
                        const auto file = results.getReference (0);
                        const bool ok = safeThis->processorRef.presetManager.savePresetToFile (file);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    }

                    complete (juce::var (result));
                });
        });

    options = options.withNativeFunction ("loadPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.loadPreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    options = options.withNativeFunction ("loadPresetFromFile",
        [this] (auto&, auto complete)
        {
            auto presetsDir = processorRef.presetManager.getPresetsDirectory();

            auto chooser = std::make_shared<juce::FileChooser> ("Load Preset", presetsDir, "*.json");
            juce::Component::SafePointer<TapestopEditor> safeThis (this);

            chooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        const auto file = results.getReference (0);
                        const bool ok = safeThis->processorRef.presetManager.loadPresetFromFile (file);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    }

                    complete (juce::var (result));
                });
        });

    options = options.withNativeFunction ("getPresetList",
        [this] (auto&, auto complete)
        {
            juce::Array<juce::var> list;
            for (const auto& name : processorRef.presetManager.getPresetList())
                list.add (juce::var (name));
            complete (juce::var (list));
        });

    options = options.withNativeFunction ("getCurrentPreset",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.presetManager.getCurrentPresetName()));
        });

    options = options.withNativeFunction ("selectNextPreset",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.presetManager.getNextPreset()));
        });

    options = options.withNativeFunction ("selectPreviousPreset",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.presetManager.getPreviousPreset()));
        });

    options = options.withNativeFunction ("deletePreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.deletePreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    options = options.withNativeFunction ("isFactoryPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.isFactoryPreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

   #if JUCE_WINDOWS
    // WebView2's default user-data folder is denied in most DAW hosts; a
    // failed construction falls back to the IE backend with no resource
    // provider, which presents as a blank page and no error
    // (critical_webview2_runtime_gotchas_windows).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OTapestop_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor, nullptr undoManager) ----
    for (int i = 0; i < kSliderIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kSliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < kComboIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kComboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < kToggleIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kToggleIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            toggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *toggleRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Live readback: one timer, one event, both consumers (ratio bar +
    // envelope playhead). Seed the generation so a fresh editor does not push
    // a spurious envelopeState — the page pulls via requestEnvelope at init.
    lastEnvGeneration = processorRef.uiEnvGeneration.load (std::memory_order_relaxed);
    startTimerHz (30);

    // Fixed 860 × 580 — must stay in sync with styles.css (html/body and
    // .frame both read 860px/580px). PLAN Locked Decision: never resize.
    setSize (860, 580);
}

TapestopEditor::~TapestopEditor()
{
    stopTimer();
}

// ── 30 Hz readback push ─────────────────────────────────────────────────────
void TapestopEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("state", processorRef.uiState.load (std::memory_order_relaxed));
    obj->setProperty ("ratio", (double) processorRef.uiRatio.load (std::memory_order_relaxed));
    obj->setProperty ("phase", (double) processorRef.uiScratchPhase.load (std::memory_order_relaxed));

    // IfBrowserIsVisible: completions/events are dropped while hidden, which
    // is exactly right for live data — nothing queues, nothing hangs
    // (critical_webview_completion_gated_on_isvisible).
    webView->emitEventIfBrowserIsVisible ("transportFrame", juce::var (obj));

    // Envelope changed under the editor (host session reload while open,
    // harness commit, future preset load): push the sanitized JSON.
    const auto gen = processorRef.uiEnvGeneration.load (std::memory_order_relaxed);
    if (gen != lastEnvGeneration)
    {
        lastEnvGeneration = gen;
        webView->emitEventIfBrowserIsVisible ("envelopeState",
                                              juce::var (processorRef.getScratchEnvelopeJson()));
    }
}

// ── Layout ──────────────────────────────────────────────────────────────────
void TapestopEditor::paint (juce::Graphics&)
{
    // The WebView fills the editor — nothing to paint.
}

void TapestopEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
