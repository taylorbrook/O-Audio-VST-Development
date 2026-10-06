/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Plugin Editor (implementation)
    Ouaricon Audio
    Developer: Taylor Brook

    Adapted from mockups/v1-PluginEditor.cpp (PLAN Task 3). Mirrors
    O-simpleAdditive / O-simpleGrain: bare-path resource provider, the `Juce`
    namespace on the page, 3-arg attachments, WebView2 user-data folder always
    set on Windows.

    Import (ARCHITECTURE A8): importFromFile / importFromMemory run on the
    processor's worker pool; the worker publishes the bank, then the Imported
    auto-select runs on the message thread through an AsyncUpdater (D-M). The
    import status is POLLED by version on this editor's timer (D-E).

    BinaryData.h is included in THIS file only: the console test targets
    compile every other .cpp without the UIResources library (P7).

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "VizPayload.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace
{
    constexpr int kTimerHz = 30;

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }

    // 3-arg attachment, or nullptr (and a debug assert) on ID drift - a missing
    // parameter is a silently dead control, never a crash.
    template <typename Attachment, typename Relay>
    std::unique_ptr<Attachment> attach (juce::AudioProcessorValueTreeState& apvts,
                                        const char* paramId, Relay& relay)
    {
        auto* p = apvts.getParameter (paramId);
        jassert (p != nullptr);
        return p != nullptr ? std::make_unique<Attachment> (*p, relay, nullptr) : nullptr;
    }
}

// -- Resource provider --------------------------------------------------------
// WKWebView / WebView2 hand the callback a BARE PATH ("/", "/index.html",
// "/js/i18n.js"). Compare by string equality - never strip a scheme/host.
// charset=utf-8 on every text resource (the page uses non-ASCII glyphs).
std::optional<juce::WebBrowserComponent::Resource>
OSimpleWavetableAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    // The copy table the inline controller imports. A missing branch is a BLANK
    // page (the module import fails, so init() never runs) - check-i18n [8].
    if (url == "/js/i18n.js")
        return makeBinaryResource (BinaryData::i18n_js, BinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    // webview-drop-streaming (copied to Source/ui/public/modules/ by
    // ouaricon_add_module). The page imports it LAZILY on a drop, so a miss here
    // costs the drop path only - but it is still a miss. Hyphens are stripped
    // from the BinaryData symbol (critical_binary_data_strips_hyphens).
    if (url == "/modules/webview-drop-streaming.js")
        return makeBinaryResource (BinaryData::webviewdropstreaming_js,
                                   BinaryData::webviewdropstreaming_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource (BinaryData::insects_png, BinaryData::insects_pngSize, "image/png");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // faces under /fonts/ where its relative URLs land (O-simpleAdditive R5).
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (BinaryData::ebgaramond_css, BinaryData::ebgaramond_cssSize, "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (BinaryData::EBGaramondRegular_woff2, BinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (BinaryData::EBGaramondItalic_woff2, BinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (BinaryData::EBGaramondBold_woff2, BinaryData::EBGaramondBold_woff2Size, "font/woff2");

    return std::nullopt;
}

// -- Construction -------------------------------------------------------------
OSimpleWavetableAudioProcessorEditor::OSimpleWavetableAudioProcessorEditor (OSimpleWavetableAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      // 1) RELAYS - initialised in DECLARATION order, before the WebView exists
      bankRelay        (std::make_unique<juce::WebComboBoxRelay>     ("bank")),
      positionRelay    (std::make_unique<juce::WebSliderRelay>       ("position")),
      interpRelay      (std::make_unique<juce::WebToggleButtonRelay> ("interp")),
      bandlimitRelay   (std::make_unique<juce::WebToggleButtonRelay> ("bandlimit")),
      bitDepthRelay    (std::make_unique<juce::WebComboBoxRelay>     ("bit_depth")),
      lfoRateRelay     (std::make_unique<juce::WebSliderRelay>       ("lfo_rate")),
      lfoSyncRelay     (std::make_unique<juce::WebComboBoxRelay>     ("lfo_sync")),
      lfoDivRelay      (std::make_unique<juce::WebComboBoxRelay>     ("lfo_div")),
      lfoShapeRelay    (std::make_unique<juce::WebComboBoxRelay>     ("lfo_shape")),
      lfoDepthRelay    (std::make_unique<juce::WebSliderRelay>       ("lfo_depth")),
      menvAttackRelay  (std::make_unique<juce::WebSliderRelay>       ("menv_attack")),
      menvDecayRelay   (std::make_unique<juce::WebSliderRelay>       ("menv_decay")),
      menvSustainRelay (std::make_unique<juce::WebSliderRelay>       ("menv_sustain")),
      menvReleaseRelay (std::make_unique<juce::WebSliderRelay>       ("menv_release")),
      envAmountRelay   (std::make_unique<juce::WebSliderRelay>       ("env_amount")),
      ampAttackRelay   (std::make_unique<juce::WebSliderRelay>       ("amp_attack")),
      ampDecayRelay    (std::make_unique<juce::WebSliderRelay>       ("amp_decay")),
      ampSustainRelay  (std::make_unique<juce::WebSliderRelay>       ("amp_sustain")),
      ampReleaseRelay  (std::make_unique<juce::WebSliderRelay>       ("amp_release")),
      voiceModeRelay   (std::make_unique<juce::WebComboBoxRelay>     ("voice_mode")),
      outputLevelRelay (std::make_unique<juce::WebSliderRelay>       ("output_level"))
{
    // 2) WEBVIEW options -------------------------------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        // Every relay - slider, combo AND toggle. A toggle relay left out here
        // makes getToggleState() on the page bind to a state nothing updates.
        .withOptionsFrom (*bankRelay)
        .withOptionsFrom (*positionRelay)
        .withOptionsFrom (*interpRelay)
        .withOptionsFrom (*bandlimitRelay)
        .withOptionsFrom (*bitDepthRelay)
        .withOptionsFrom (*lfoRateRelay)
        .withOptionsFrom (*lfoSyncRelay)
        .withOptionsFrom (*lfoDivRelay)
        .withOptionsFrom (*lfoShapeRelay)
        .withOptionsFrom (*lfoDepthRelay)
        .withOptionsFrom (*menvAttackRelay)
        .withOptionsFrom (*menvDecayRelay)
        .withOptionsFrom (*menvSustainRelay)
        .withOptionsFrom (*menvReleaseRelay)
        .withOptionsFrom (*envAmountRelay)
        .withOptionsFrom (*ampAttackRelay)
        .withOptionsFrom (*ampDecayRelay)
        .withOptionsFrom (*ampSustainRelay)
        .withOptionsFrom (*ampReleaseRelay)
        .withOptionsFrom (*voiceModeRelay)
        .withOptionsFrom (*outputLevelRelay);

    // -- Native functions -----------------------------------------------------
    // EVERY name the page calls through getNativeFunction() is registered here;
    // an unregistered one never settles its promise and its control is silently
    // dead while build, auval and pluginval all pass. The page calls:
    //   getParameterDefaults, getUiLanguage, setUiLanguage, uiReady,
    //   importAudio, importDroppedAudio, uiMidi, applyFactoryPreset.

    // dblclick reset: the relay's properties carry no default, and a JS default
    // table would drift from C++. Sliders report the engineering default;
    // choices and bools report convertFrom0to1 (default) = the index / 0|1.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            for (const auto* paramId : OSimpleWavetable::ParamIDs::all)
                if (auto* param = processorRef.getAPVTS().getParameter (paramId))
                    obj->setProperty (paramId, param->convertFrom0to1 (param->getDefaultValue()));
            complete (juce::var (obj));
        });

    // Interface language: plain natives, no relay - the language is not a
    // parameter and must not reach an automation lane. The page PULLS it once.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (OSimpleWavetableAudioProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" or "zh-Hans" to en,
            // so an unexpected argument degrades to English, never stored raw.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OSimpleWavetableAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);
            complete (juce::var());
        });

    // The page's handshake: called once its event listeners exist, and again
    // whenever it becomes visible. Events emitted before the page listens are
    // lost, so state is re-sent on request rather than assumed delivered.
    // Order (P10): bankUpdate -> importStatus -> (next tick) forced cycleUpdate.
    // The counters are refreshed AFTER the emits so the next tick does not
    // resend the same bank.
    options = options.withNativeFunction ("uiReady",
        [this] (auto&, auto complete)
        {
            emitBankUpdate (true);
            emitImportStatus();
            forceCycleEmit = true;
            lastBankGeneration = processorRef.getBankDisplayGeneration();
            lastBankIndex      = processorRef.getSelectedBankIndex();
            lastImportVersion  = processorRef.getImportStatusVersion();
            complete (juce::var (true));
        });

    // Import button -> native FileChooser. Completes IMMEDIATELY: progress and
    // the result reach the page as importStatus events.
    options = options.withNativeFunction ("importAudio",
        [this] (auto&, auto complete)
        {
            complete (juce::var (true));

            auto chooser = std::make_shared<juce::FileChooser> (
                "Import audio", juce::File{}, "*.wav;*.wave;*.aif;*.aiff;*.flac;*.ogg");
            juce::Component::SafePointer<OSimpleWavetableAudioProcessorEditor> safeThis (this);

            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser] (const juce::FileChooser& fc)
                {
                    // Editor gone while the dialog was open: bail WITHOUT touching
                    // the bridge (pattern_webview_launchasync_safepointer_no_complete).
                    if (safeThis == nullptr) return;

                    const auto file = fc.getResult();
                    if (file.existsAsFile())
                        safeThis->processorRef.importFromFile (file);
                });
        });

    // Drop path: WKWebView strips file paths, so the page sends the bytes as
    // standard base64. The processor caps, decodes (Base64::convertFromBase64)
    // and reports a localized error code through importStatus (D-Z).
    options = options.withNativeFunction ("importDroppedAudio",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() < 2)
            {
                complete (juce::var (false));
                return;
            }

            complete (juce::var (processorRef.importFromBase64 (args[0].toString(), args[1].toString())));
        });

    // On-screen + computer keyboard -> synth. args: [note, isNoteOn, velocity?].
    // handleUiMidi clamps the note and guards the velocity.
    options = options.withNativeFunction ("uiMidi",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() >= 2)
                processorRef.handleUiMidi ((int) args[0], (bool) args[1],
                                           args.size() >= 3 ? (float) args[2] : 0.8f);
            complete (juce::var());
        });

    // Lesson presets by STABLE id (the button captions localize; the ids do
    // not). One pass: every parameter goes to its default or its recipe value,
    // output_level is never touched (D-Y); the relays sync the controls back.
    options = options.withNativeFunction ("applyFactoryPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && processorRef.applyFactoryPreset (args[0].toString());
            forceCycleEmit = true;   // the bank change itself reaches the page through the index watch
            complete (juce::var (ok));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder ALWAYS set to a writable temp dir: hosts deny the
    // default location -> silent fallback -> blank page.
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                     .getChildFile ("OsimpleWavetable_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3) ATTACHMENTS - after the WebView, same order as the declarations.
    //    3-arg constructor (nullptr UndoManager) through attach(), which asserts
    //    on ID drift in debug and leaves the control unbound (never a null
    //    dereference) in release.
    namespace ids = OSimpleWavetable::ParamIDs;
    auto& apvts = processorRef.getAPVTS();
    bankAttachment        = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::bank,        *bankRelay);
    positionAttachment    = attach<juce::WebSliderParameterAttachment>       (apvts, ids::position,    *positionRelay);
    interpAttachment      = attach<juce::WebToggleButtonParameterAttachment> (apvts, ids::interp,      *interpRelay);
    bandlimitAttachment   = attach<juce::WebToggleButtonParameterAttachment> (apvts, ids::bandlimit,   *bandlimitRelay);
    bitDepthAttachment    = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::bitDepth,    *bitDepthRelay);
    lfoRateAttachment     = attach<juce::WebSliderParameterAttachment>       (apvts, ids::lfoRate,     *lfoRateRelay);
    lfoSyncAttachment     = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::lfoSync,     *lfoSyncRelay);
    lfoDivAttachment      = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::lfoDiv,      *lfoDivRelay);
    lfoShapeAttachment    = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::lfoShape,    *lfoShapeRelay);
    lfoDepthAttachment    = attach<juce::WebSliderParameterAttachment>       (apvts, ids::lfoDepth,    *lfoDepthRelay);
    menvAttackAttachment  = attach<juce::WebSliderParameterAttachment>       (apvts, ids::menvAttack,  *menvAttackRelay);
    menvDecayAttachment   = attach<juce::WebSliderParameterAttachment>       (apvts, ids::menvDecay,   *menvDecayRelay);
    menvSustainAttachment = attach<juce::WebSliderParameterAttachment>       (apvts, ids::menvSustain, *menvSustainRelay);
    menvReleaseAttachment = attach<juce::WebSliderParameterAttachment>       (apvts, ids::menvRelease, *menvReleaseRelay);
    envAmountAttachment   = attach<juce::WebSliderParameterAttachment>       (apvts, ids::envAmount,   *envAmountRelay);
    ampAttackAttachment   = attach<juce::WebSliderParameterAttachment>       (apvts, ids::ampAttack,   *ampAttackRelay);
    ampDecayAttachment    = attach<juce::WebSliderParameterAttachment>       (apvts, ids::ampDecay,    *ampDecayRelay);
    ampSustainAttachment  = attach<juce::WebSliderParameterAttachment>       (apvts, ids::ampSustain,  *ampSustainRelay);
    ampReleaseAttachment  = attach<juce::WebSliderParameterAttachment>       (apvts, ids::ampRelease,  *ampReleaseRelay);
    voiceModeAttachment   = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::voiceMode,   *voiceModeRelay);
    outputLevelAttachment = attach<juce::WebSliderParameterAttachment>       (apvts, ids::outputLevel, *outputLevelRelay);

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Snapshot the counters: the page's uiReady covers the current state; the
    // timer only signals changes that happen after this.
    bankThumbs.points.reserve ((size_t) WavetableBank::kMaxFrames * (size_t) WavetableBank::kThumbSize);
    lastImportVersion  = processorRef.getImportStatusVersion();
    lastBankGeneration = processorRef.getBankDisplayGeneration();
    lastBankIndex      = processorRef.getSelectedBankIndex();

    // Fixed frame, <= 800 px tall. The headless UI gates parse this literal,
    // so it stays the numeric setSize.
    setSize (1120, 780);
    setResizable (false, false);

    startTimerHz (kTimerHz);
}

OSimpleWavetableAudioProcessorEditor::~OSimpleWavetableAudioProcessorEditor()
{
    stopTimer();
}

// -- C++ -> page pushes -------------------------------------------------------
// Shapes: VizPayload.h. Every push is safe at any time on the message thread.

// bankUpdate { bank, imported, numFrames, filename, frames: [[128] x N] }
// On bank change / import publish / restore / uiReady only (up to 256 x 128).
void OSimpleWavetableAudioProcessorEditor::emitBankUpdate (bool force)
{
    if (webView == nullptr)
        return;

    processorRef.getBankThumbnails (bankThumbs);
    const auto h = viz::bankHash (bankThumbs);
    if (! force && h == lastBankHash)
        return;                                   // D-T: the audio thread's follow-up gen bump
    lastBankHash = h;

    webView->emitEventIfBrowserIsVisible ("bankUpdate", viz::bankToVar (bankThumbs));
}

// importStatus { state: "idle"|"busy"|"done"|"error", filename, frames, error }
void OSimpleWavetableAudioProcessorEditor::emitImportStatus()
{
    if (webView == nullptr)
        return;

    webView->emitEventIfBrowserIsVisible ("importStatus", viz::importToVar (processorRef.getImportStatus()));
}

// cycleUpdate: hash over the quantized payload (D-P). Build + hash allocate
// nothing; the var is built only when something the page draws changed.
void OSimpleWavetableAudioProcessorEditor::emitCycleUpdate (bool force)
{
    if (webView == nullptr)
        return;

    processorRef.buildCycleView (cycleView);
    const auto h = viz::cycleHash (cycleView);
    if (! force && h == lastCycleHash)
        return;
    lastCycleHash = h;

    webView->emitEventIfBrowserIsVisible ("cycleUpdate", viz::cycleToVar (cycleView));
}

// -- Timer (30 Hz, message thread) --------------------------------------------
// Order every tick (P10, D-T): bankUpdate -> importStatus -> cycleUpdate.
void OSimpleWavetableAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    // P2: the generation bump for a bank-PARAM change happens on the audio
    // thread only; a host that idles the instrument would leave the stack
    // stale, so the APVTS index is watched here as well.
    const auto gen = processorRef.getBankDisplayGeneration();
    const int  idx = processorRef.getSelectedBankIndex();
    if (gen != lastBankGeneration || idx != lastBankIndex)
    {
        lastBankGeneration = gen;
        lastBankIndex      = idx;
        emitBankUpdate (false);
        forceCycleEmit = true;
    }

    if (const auto v = processorRef.getImportStatusVersion(); v != lastImportVersion)
    {
        lastImportVersion = v;
        emitImportStatus();
    }

    emitCycleUpdate (std::exchange (forceCycleEmit, false));
}

// -- Layout -------------------------------------------------------------------
void OSimpleWavetableAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor - nothing to paint.
}

void OSimpleWavetableAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
