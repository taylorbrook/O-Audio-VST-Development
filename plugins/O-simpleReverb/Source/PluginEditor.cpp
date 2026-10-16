/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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

    O-simpleReverb - Editor Implementation (v3.0.0)

    Mirrors O-simpleAmbisonics v1.0.1 (relays, natives, bare-path resource
    provider, 3-arg attachments, gesture close on destruction, WebView2
    user-data folder on Windows, fixed-aspect resizable frame). The meterFrame
    text comes from MeterFramePayload.h, shared with render-check so the UI
    fixtures are the editor's exact bytes.

    BinaryData.h is included in THIS file only: render-check and param-dump
    compile PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and without the UI
    binary-data library.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace
{
    constexpr int kTimerHz = 30;

    // Usable height of the display under the mouse (where the host is about to
    // open the editor), minus room for the host's own plugin-window title bar
    // and header. 0 when no display is known.
    float availableDisplayHeight()
    {
        constexpr float hostChromeAllowance = 90.0f;

        const auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* display = displays.getDisplayForPoint(juce::Desktop::getMousePosition().toFloat());
        if (display == nullptr)
            display = displays.getPrimaryDisplay();
        if (display == nullptr)
            return 0.0f;

        return display->userBounds.getHeight() - hostChromeAllowance;
    }

    auto makeBinaryResource(const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*>(data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(bytes, bytes + size),
            juce::String(mimeType)
        };
    }

    // 3-arg attachment (nullptr UndoManager), or nullptr + a debug assert on ID
    // drift - a missing parameter is a silently dead control, never a crash.
    template <typename Attachment, typename Relay>
    std::unique_ptr<Attachment> attach(juce::AudioProcessorValueTreeState& apvts,
                                       const char* paramId, Relay& relay)
    {
        auto* p = apvts.getParameter(paramId);
        jassert(p != nullptr);
        return p != nullptr ? std::make_unique<Attachment>(*p, relay, nullptr) : nullptr;
    }

    juce::var stringsToVar(const juce::StringArray& names)
    {
        juce::Array<juce::var> out;
        for (const auto& n : names)
            out.add(n);
        return juce::var(std::move(out));
    }

    // savePresetWithDialog: preset-manager.js resolves it, the family page
    // never binds it (saving is the Save button). Registered so no promise can
    // hang and the bridge stays exact (pattern_webview_native_fn_bridge_gap).
    juce::var dialogResult(bool success, const juce::String& name)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("success", success);
        o->setProperty("name", name);
        return juce::var(o);
    }

    bool isNumber(const juce::var& v) { return v.isDouble() || v.isInt() || v.isInt64(); }

    const char* engineName(OSimpleReverbAudioProcessor::TypePreset::Engine e)
    {
        using E = OSimpleReverbAudioProcessor::TypePreset::Engine;
        return e == E::Plate ? "plate" : e == E::Spring ? "spring" : "fdn";
    }

    const char* eqKindName(OSimpleReverbAudioProcessor::TypePreset::EqType t)
    {
        using Q = OSimpleReverbAudioProcessor::TypePreset::EqType;
        switch (t) {
            case Q::LowShelf:  return "lowShelf";
            case Q::HighShelf: return "highShelf";
            case Q::Peak:      return "peak";
            case Q::HighPass:  return "highPass";
            case Q::None:      break;
        }
        return "none";
    }
}

// ── Resource provider ───────────────────────────────────────────────────────
// WKWebView / WebView2 hand the callback a BARE PATH ("/", "/index.html",
// "/js/i18n.js"). Compare by string equality - never strip a scheme/host.
// Every file listed in juce_add_binary_data(O-simpleReverb_UIResources) has a
// branch here; a missing one is a silent 404 (the font falls back to Times,
// the i18n import fails and the page is BLANK).
std::optional<juce::WebBrowserComponent::Resource>
OSimpleReverbAudioProcessorEditor::getResource(const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeBinaryResource(BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    // The copy table the inline controller imports (check-i18n [8]).
    if (url == "/js/i18n.js")
        return makeBinaryResource(BinaryData::i18n_js, BinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource(BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource(BinaryData::check_native_interop_js,
                                  BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    // preset-manager.js: copied from the module into Source/ui/public/modules/
    // at configure time (gitignored), embedded since v3.0.0. The page imports
    // it DYNAMICALLY, so a missing branch disables the bar, never the page.
    if (url == "/modules/preset-manager.js")
        return makeBinaryResource(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource(BinaryData::insects_png, BinaryData::insects_pngSize, "image/png");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // faces under /fonts/ where its relative url('../fonts/...') lands. Hyphens
    // are STRIPPED from the BinaryData symbols.
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource(BinaryData::ebgaramond_css, BinaryData::ebgaramond_cssSize, "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource(BinaryData::EBGaramondRegular_woff2, BinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource(BinaryData::EBGaramondItalic_woff2, BinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource(BinaryData::EBGaramondBold_woff2, BinaryData::EBGaramondBold_woff2Size, "font/woff2");

    return std::nullopt;
}

// ── The well models ─────────────────────────────────────────────────────────
double OSimpleReverbAudioProcessorEditor::modelRate() const
{
    const double fs = processorRef.getSampleRate();
    return fs > 0.0 ? fs : 48000.0;
}

// Once at page init: the six types as the DSP holds them. The page draws the
// RT60 slope from baseT60 x DECAY (the DSP's own law, driveSlot) so the slope
// keeps answering a DECAY gesture while the host idles the audio thread.
juce::var OSimpleReverbAudioProcessorEditor::reverbModel() const
{
    juce::Array<juce::var> types;
    const juce::StringArray names { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" };
    for (int i = 0; i < 6; ++i) {
        const auto& t = OSimpleReverbAudioProcessor::typePresets[i];
        auto* eq = new juce::DynamicObject();
        eq->setProperty("kind", eqKindName(t.eqType));
        eq->setProperty("hz", t.eqFreq);
        eq->setProperty("db", t.eqGain);
        eq->setProperty("q", t.eqQ);
        auto* o = new juce::DynamicObject();
        o->setProperty("name", names[i]);
        o->setProperty("engine", engineName(t.engine));
        o->setProperty("baseT60", t.baseT60);
        o->setProperty("preDelayMs", t.preDelayMs);
        o->setProperty("earlySpanMs", t.earlySpanMs);
        o->setProperty("earlyLevel", t.earlyLevel);
        o->setProperty("sizeLo", t.sizeLo);
        o->setProperty("sizeHi", t.sizeHi);
        o->setProperty("modulated", t.fdn.modMs > 0.0f);
        o->setProperty("eq", juce::var(eq));
        types.add(juce::var(o));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty("types", juce::var(std::move(types)));
    root->setProperty("sampleRate", modelRate());
    return juce::var(root);
}

juce::var OSimpleReverbAudioProcessorEditor::earlyReflectionTaps(int typeIndex, float sizePercent)
{
    const double fs = modelRate();
    if (! juce::exactlyEqual(fs, erModelRate)) {
        erModel.prepare(fs, OSimpleReverbAudioProcessor::kEarlyModelSpanMs);   // message thread: allocates once per rate
        erModelRate = fs;
    }
    const auto taps = OSimpleReverbAudioProcessor::earlyTaps(erModel, fs, typeIndex, sizePercent);

    auto side = [&taps](const auto& ms, const auto& g) {
        juce::Array<juce::var> out;
        for (int k = 0; k < taps.numTaps; ++k) {
            auto* o = new juce::DynamicObject();
            o->setProperty("ms", ms[(size_t) k]);
            o->setProperty("g", g[(size_t) k]);
            out.add(juce::var(o));
        }
        return juce::var(std::move(out));
    };
    juce::Array<juce::var> echoes;
    for (int i = 0; i < taps.numSpringEchoes; ++i)
        echoes.add(taps.springEchoMs[(size_t) i]);

    auto* root = new juce::DynamicObject();
    root->setProperty("type", juce::jlimit(0, 5, typeIndex));
    root->setProperty("engine", engineName(taps.engine));
    root->setProperty("preDelayMs", taps.preDelayMs);
    root->setProperty("earlyLevel", taps.earlyLevel);
    root->setProperty("left", side(taps.msL, taps.gainL));
    root->setProperty("right", side(taps.msR, taps.gainR));
    root->setProperty("springEchoesMs", juce::var(std::move(echoes)));
    return juce::var(root);
}

// 128 log-spaced bins, 20 Hz to min (20 kHz, 0.49 fs). `db` is the whole tone
// stage (Low Cut included only while IN); `lowCutDb` is the Low Cut alone, so
// the page can draw it dimmed while OUT; `typeEqDb` and `characterDb` are the
// other two parts, drawn as their own faint curves.
juce::var OSimpleReverbAudioProcessorEditor::toneCurve(int typeIndex, float characterValue, float lowCutHz, bool lowCutOn) const
{
    const double fs = modelRate();
    const double lo = 20.0, hi = juce::jmin(20000.0, 0.49 * fs);
    juce::Array<juce::var> hz, db, cut, eq, character;
    for (int i = 0; i < OSimpleReverbAudioProcessor::kToneBins; ++i) {
        const double f = lo * std::pow(hi / lo, (double) i / (double) (OSimpleReverbAudioProcessor::kToneBins - 1));
        hz.add(f);
        db.add(processorRef.toneGainDb(typeIndex, characterValue, lowCutHz, lowCutOn, fs, f));
        cut.add(OSimpleReverbAudioProcessor::lowCutGainDb(lowCutHz, fs, f));
        eq.add(OSimpleReverbAudioProcessor::typeEqGainDb(typeIndex, fs, f));
        character.add(processorRef.characterGainDb(characterValue, fs, f));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty("hz", juce::var(std::move(hz)));
    root->setProperty("db", juce::var(std::move(db)));
    root->setProperty("lowCutDb", juce::var(std::move(cut)));
    root->setProperty("typeEqDb", juce::var(std::move(eq)));
    root->setProperty("characterDb", juce::var(std::move(character)));
    // The readouts' numbers, from the same statics (no JS copy of the maps).
    root->setProperty("warmCutoffHz", (double) OSimpleReverbAudioProcessor::warmCutoffHz(characterValue, fs));
    root->setProperty("brightDb", (double) OSimpleReverbAudioProcessor::brightGainDb(characterValue));
    return juce::var(root);
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleReverbAudioProcessorEditor::OSimpleReverbAudioProcessorEditor(OSimpleReverbAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      // 1) RELAYS - initialised in DECLARATION order, before the WebView exists.
      typeRelay      (std::make_unique<juce::WebComboBoxRelay>    ("TYPE")),
      characterRelay (std::make_unique<juce::WebSliderRelay>      ("CHARACTER")),
      wetRelay       (std::make_unique<juce::WebSliderRelay>      ("WET")),
      dryRelay       (std::make_unique<juce::WebSliderRelay>      ("DRY")),
      decayRelay     (std::make_unique<juce::WebSliderRelay>      ("DECAY")),
      sizeRelay      (std::make_unique<juce::WebSliderRelay>      ("SIZE")),
      lpFreqRelay    (std::make_unique<juce::WebSliderRelay>      ("LPFREQ")),
      lpOnRelay      (std::make_unique<juce::WebToggleButtonRelay>("LPON"))
{
    // 2) WEBVIEW options -------------------------------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        // Every relay - slider, combo AND toggle. A toggle relay left out here
        // makes getToggleState() on the page bind to a state nothing updates.
        .withOptionsFrom(*typeRelay)
        .withOptionsFrom(*characterRelay)
        .withOptionsFrom(*wetRelay)
        .withOptionsFrom(*dryRelay)
        .withOptionsFrom(*decayRelay)
        .withOptionsFrom(*sizeRelay)
        .withOptionsFrom(*lpFreqRelay)
        .withOptionsFrom(*lpOnRelay);

    // -- Native functions ------------------------------------------------------
    // EVERY name the page (or preset-manager.js) calls through
    // getNativeFunction() is registered here - 20 names:
    //   page:            getParameterDefaults, getUiLanguage, setUiLanguage,
    //                    uiReady, applyFactoryPreset, getPresetCatalog,
    //                    getPluginVersion
    //   well models:     getReverbModel, getEarlyReflectionTaps, getToneCurve
    //   preset-manager:  savePreset, savePresetWithDialog, loadPreset,
    //                    loadPresetFromFile, getPresetList, getCurrentPreset,
    //                    selectNextPreset, selectPreviousPreset, deletePreset,
    //                    isFactoryPreset
    // Arguments are untrusted: types are checked here and every path completes.

    // dblclick reset: the relay's properties carry start / end / skew but no
    // default, and a JS default table would drift from C++. Engineering units.
    options = options.withNativeFunction("getParameterDefaults",
        [this](auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            for (const auto* paramId : OSimpleReverbAudioProcessor::kParamIds)
                if (auto* param = processorRef.parameters.getParameter(paramId))
                    obj->setProperty(paramId, param->convertFrom0to1(param->getDefaultValue()));
            complete(juce::var(obj));
        });

    // Interface language: plain natives, no relay - the language is not a
    // parameter and must not reach an automation lane. The page PULLS it once.
    options = options.withNativeFunction("getUiLanguage",
        [this](auto&, auto complete)
        {
            complete(juce::var(OSimpleReverbAudioProcessor::languageCode(
                processorRef.uiLanguage.load(std::memory_order_acquire))));
        });

    options = options.withNativeFunction("setUiLanguage",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            // languageIndex() maps anything but "fr" / "zh-Hans" to en.
            if (args.size() > 0)
                processorRef.uiLanguage.store(
                    OSimpleReverbAudioProcessor::languageIndex(args[0].toString()),
                    std::memory_order_release);
            complete(juce::var(OSimpleReverbAudioProcessor::languageCode(
                processorRef.uiLanguage.load(std::memory_order_acquire))));
        });

    // The version for the gear popover: JucePlugin_VersionString is the CMake
    // VERSION, so the page cannot drift from the bundle.
    options = options.withNativeFunction("getPluginVersion",
        [](auto&, auto complete) { complete(juce::String(JucePlugin_VersionString)); });

    // The page's handshake: called once its event listeners exist, and again
    // whenever it becomes visible. Both events are forced; emitPresetState
    // snapshots the counters BEFORE it reads the identity, so a transition
    // inside this window re-sends on the next tick (a duplicate, never a loss).
    options = options.withNativeFunction("uiReady",
        [this](auto&, auto complete)
        {
            emitFrame(true);            // the stored frame: no read(), no tracker tick
            emitPresetState(true);
            complete(juce::var(true));
        });

    // Lessons by STABLE table id (the captions localize; the ids do not). The
    // relays sync the controls back; the lit button and the caption follow the
    // presetState push, never the click.
    options = options.withNativeFunction("applyFactoryPreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && args[0].isString()
                         && processorRef.applyFactoryPreset(args[0].toString());
            complete(juce::var(ok));
        });

    options = options.withNativeFunction("getPresetCatalog",
        [this](auto&, auto complete) { complete(processorRef.getPresetCatalog()); });

    // -- Well models -----------------------------------------------------------
    options = options.withNativeFunction("getReverbModel",
        [this](auto&, auto complete) { complete(reverbModel()); });

    // (type, sizePercent). Called by the page on a TYPE or SIZE change, latest
    // wins, one call in flight.
    options = options.withNativeFunction("getEarlyReflectionTaps",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            const int type = args.size() > 0 && isNumber(args[0]) ? (int) args[0] : 1;
            const float size = args.size() > 1 && isNumber(args[1]) ? (float) (double) args[1] : 50.0f;
            complete(earlyReflectionTaps(juce::jlimit(0, 5, type), juce::jlimit(0.0f, 100.0f, size)));
        });

    // (type, character, lowCutHz, lowCutOn).
    options = options.withNativeFunction("getToneCurve",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            const int type = args.size() > 0 && isNumber(args[0]) ? (int) args[0] : 1;
            const float character = args.size() > 1 && isNumber(args[1]) ? (float) (double) args[1] : 0.0f;
            const float cutHz = args.size() > 2 && isNumber(args[2]) ? (float) (double) args[2] : 200.0f;
            const bool cutOn = args.size() > 3 && (args[3].isBool() || isNumber(args[3])) && (bool) args[3];
            complete(toneCurve(juce::jlimit(0, 5, type), juce::jlimit(-100.0f, 100.0f, character),
                               juce::jlimit(20.0f, 400.0f, cutHz), cutOn));
        });

    // -- Presets (preset-manager.js) -------------------------------------------
    options = options.withNativeFunction("getPresetList",
        [this](auto&, auto complete) { complete(stringsToVar(processorRef.getPresetWalkOrder())); });

    options = options.withNativeFunction("getCurrentPreset",
        [this](auto&, auto complete) { complete(juce::var(processorRef.getPresetName())); });

    // The neighbour in the walk order, NOT loaded: preset-manager.js loads the
    // returned name through loadPreset.
    options = options.withNativeFunction("selectNextPreset",
        [this](auto&, auto complete) { complete(juce::var(processorRef.getNeighbourPreset(1))); });

    options = options.withNativeFunction("selectPreviousPreset",
        [this](auto&, auto complete) { complete(juce::var(processorRef.getNeighbourPreset(-1))); });

    options = options.withNativeFunction("loadPreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && args[0].isString()
                         && processorRef.loadPresetByName(args[0].toString());
            complete(juce::var(ok));
        });

    options = options.withNativeFunction("savePreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && args[0].isString()
                         && processorRef.saveUserPreset(args[0].toString());
            complete(juce::var(ok));
        });

    options = options.withNativeFunction("deletePreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && args[0].isString()
                         && processorRef.deleteUserPreset(args[0].toString());
            complete(juce::var(ok));
        });

    // Table lookup, never the disk.
    options = options.withNativeFunction("isFactoryPreset",
        [](const juce::Array<juce::var>& args, auto complete)
        {
            complete(juce::var(args.size() > 0 && args[0].isString()
                               && OSimpleReverbAudioProcessor::isFactoryPresetName(args[0].toString())));
        });

    options = options.withNativeFunction("savePresetWithDialog",
        [](auto&, auto complete) { complete(dialogResult(false, {})); });

    // The gear menu's "Load file...": a preset .json from anywhere (v2's
    // import path, kept). CR-01: the chooser can outlive the editor - capture a
    // SafePointer and bail with a bare return on the null path; `complete` is
    // owned by the destroyed WebView, so calling it would itself be a UAF.
    options = options.withNativeFunction("loadPresetFromFile",
        [this](auto&, auto complete)
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Preset", processorRef.presetManager.getPresetsDirectory(), "*.json");
            juce::Component::SafePointer<OSimpleReverbAudioProcessorEditor> safeThis(this);
            fileChooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete](const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    const auto file = fc.getResult();
                    const bool ok = file.existsAsFile() && safeThis->processorRef.loadPresetFile(file);
                    complete(dialogResult(ok, ok ? safeThis->processorRef.getPresetName() : juce::String()));
                });
        });

   #if JUCE_WINDOWS
    // The backend MUST be named: Backend::defaultBackend is the legacy IE
    // control on Windows, which ignores withWinWebView2Options and has no
    // resource provider (critical_webview2_runtime_gotchas_windows). The
    // user-data folder is ALWAYS a writable, plugin-scoped temp dir: hosts
    // deny the default location -> silent fallback -> blank page. Static
    // linking is JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 in CMakeLists.
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                                        .getChildFile("O-simpleReverb_WebView"))
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent>(options);

    // 3) ATTACHMENTS - after the WebView, same order as the declarations.
    auto& apvts = processorRef.parameters;
    typeAttachment      = attach<juce::WebComboBoxParameterAttachment>    (apvts, "TYPE",      *typeRelay);
    characterAttachment = attach<juce::WebSliderParameterAttachment>      (apvts, "CHARACTER", *characterRelay);
    wetAttachment       = attach<juce::WebSliderParameterAttachment>      (apvts, "WET",       *wetRelay);
    dryAttachment       = attach<juce::WebSliderParameterAttachment>      (apvts, "DRY",       *dryRelay);
    decayAttachment     = attach<juce::WebSliderParameterAttachment>      (apvts, "DECAY",     *decayRelay);
    sizeAttachment      = attach<juce::WebSliderParameterAttachment>      (apvts, "SIZE",      *sizeRelay);
    lpFreqAttachment    = attach<juce::WebSliderParameterAttachment>      (apvts, "LPFREQ",    *lpFreqRelay);
    lpOnAttachment      = attach<juce::WebToggleButtonParameterAttachment>(apvts, "LPON",      *lpOnRelay);

    // Gesture tracking for the destructor (see PluginEditor.h).
    jassert(processorRef.getParameters().size() == (int) gestureOpen.size());
    for (auto* param : processorRef.getParameters())
        param->addListener(this);

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Read BEFORE the first setSize: resized() records the live scale.
    const float storedScale = processorRef.editorScale.load(std::memory_order_relaxed);

    // The 1180 x 780 design frame - the page lays out at exactly this size and
    // scales itself (CSS transform on #plugin) to whatever the window is. The
    // headless UI gates parse this literal, so it stays the numeric setSize and
    // the LAST numeric setSize in the file.
    setSize (1180, 780);

    // Resizable at a fixed 1180:780 aspect, 0.6x..1.5x. A fresh editor fits the
    // display it opens on; a size the user chose persists in the session,
    // clamped to what THIS display fits.
    setResizable(true, true);
    setResizeLimits(juce::roundToInt(designW * minScale), juce::roundToInt(designH * minScale),
                    juce::roundToInt(designW * maxScale), juce::roundToInt(designH * maxScale));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio((double) designW / (double) designH);

    const float scale = storedScale > 0.0f ? juce::jmin(storedScale, fitMaxForDisplay())
                                           : fitScaleForDisplay();
    if (! juce::approximatelyEqual(scale, 1.0f))
        setSize(juce::roundToInt(designW * scale), juce::roundToInt(designH * scale));

    // Drain one read() before the timer starts: with no editor open nothing
    // consumes hasNewData(), so the audio side has been accumulating since the
    // last close. Then start from an idle frame; the first tick emits.
    processorRef.meterFrames.read();
    current        = osr::ui::quantise(osr::MeterFrame {}, false);
    lastFrameHash  = 0;
    forceFrameEmit = true;

    // Snapshot the preset counters: the page's uiReady covers the current
    // state; the timer only signals changes that happen after this.
    lastPresetRevision = processorRef.getPresetRevision();
    lastPresetModified = processorRef.isPresetModified();

    startTimerHz(kTimerHz);
}

OSimpleReverbAudioProcessorEditor::~OSimpleReverbAudioProcessorEditor()
{
    closeOpenGestures();                           // a slider drag cut short by the close
    for (auto* param : processorRef.getParameters())
        param->removeListener(this);
    stopTimer();
}

// Any thread per the JUCE contract; here every gesture source is the message
// thread (the attachments, applyFactoryPreset).
void OSimpleReverbAudioProcessorEditor::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
    if (parameterIndex >= 0 && parameterIndex < (int) gestureOpen.size())
        gestureOpen[(size_t) parameterIndex].store(gestureIsStarting, std::memory_order_relaxed);
}

void OSimpleReverbAudioProcessorEditor::closeOpenGestures()
{
    const auto& params = processorRef.getParameters();
    for (int i = 0; i < params.size() && i < (int) gestureOpen.size(); ++i)
        if (gestureOpen[(size_t) i].exchange(false, std::memory_order_relaxed))
            params[i]->endChangeGesture();
}

// ── meterFrame: { inDb, wetDb, outPeakL, outPeakR, spanMs, live } ───────────
// Sent as ONE JSON string (the page JSON.parse()s it).
void OSimpleReverbAudioProcessorEditor::emitFrame(bool force)
{
    if (webView == nullptr)
        return;

    const auto h = osr::ui::hash(current);
    if (! force && h == lastFrameHash)
        return;                                   // a steady scene emits nothing

    const int len = osr::ui::format(current, payloadBuf, (int) sizeof(payloadBuf));
    if (len <= 0)
        return;

    lastFrameHash = h;
    webView->emitEventIfBrowserIsVisible("meterFrame", juce::var(juce::String::fromUTF8(payloadBuf, len)));
}

// ── presetState { name, id, factory, modified } ─────────────────────────────
// Emitted when the revision changed or `modified` flipped (a one-shot push
// goes stale on a lesson click or a session restore).
void OSimpleReverbAudioProcessorEditor::emitPresetState(bool force)
{
    if (webView == nullptr)
        return;

    const auto rev = processorRef.getPresetRevision();
    const bool mod = processorRef.isPresetModified();
    if (! force && rev == lastPresetRevision && mod == lastPresetModified)
        return;
    lastPresetRevision = rev;
    lastPresetModified = mod;

    auto* o = new juce::DynamicObject();
    o->setProperty("name",     processorRef.getPresetName());
    o->setProperty("id",       processorRef.getPresetId());
    o->setProperty("factory",  processorRef.isPresetFactory());
    o->setProperty("modified", mod);
    webView->emitEventIfBrowserIsVisible("presetState", juce::var(o));
}

// ── Timer (30 Hz, message thread) - the only sampler ────────────────────────
// sampleFrame() polls hasNewData() BEFORE read() (the read consumes it; the
// audio side's reset keys off that) and applies the live rule. Read every tick
// whether or not anything is emitted.
void OSimpleReverbAudioProcessorEditor::timerCallback()
{
    processorRef.refreshPresetTargetsIfNeeded();
    current = osr::ui::sampleFrame(processorRef.meterFrames, liveTracker);
    emitFrame(std::exchange(forceFrameEmit, false));
    emitPresetState(false);
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleReverbAudioProcessorEditor::paint(juce::Graphics&)
{
    // WebView fills the editor - nothing to paint.
}

void OSimpleReverbAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());

    processorRef.editorScale.store((float) getWidth() / (float) designW, std::memory_order_relaxed);
}

float OSimpleReverbAudioProcessorEditor::fitScaleForDisplay()
{
    const float available = availableDisplayHeight();
    if (available <= 0.0f)
        return 1.0f;
    return juce::jlimit(minScale, 1.0f, available / (float) designH);
}

float OSimpleReverbAudioProcessorEditor::fitMaxForDisplay()
{
    const float available = availableDisplayHeight();
    if (available <= 0.0f)
        return 1.0f;
    return juce::jlimit(minScale, maxScale, available / (float) designH);
}
