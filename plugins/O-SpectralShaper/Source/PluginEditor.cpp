/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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

    O-SpectralShaper - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OSpectralShaperAudioProcessorEditor::OSpectralShaperAudioProcessorEditor(
    OSpectralShaperAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS)
    // ============================================================================

    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("ATTACK_TIME");
    sustainTimeRelay = std::make_unique<juce::WebSliderRelay>("SUSTAIN_TIME");
    sensitivityRelay = std::make_unique<juce::WebSliderRelay>("SENSITIVITY");
    lookaheadEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("LOOKAHEAD_ENABLED");
    lookaheadTimeRelay = std::make_unique<juce::WebSliderRelay>("LOOKAHEAD_TIME");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    auto makeDialogResult = [](bool success, const juce::String& name) -> juce::var {
        auto* result = new juce::DynamicObject();
        result->setProperty("success", success);
        result->setProperty("name", name);
        return juce::var(result);
    };

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*sustainTimeRelay)
            .withOptionsFrom(*sensitivityRelay)
            .withOptionsFrom(*lookaheadEnabledRelay)
            .withOptionsFrom(*lookaheadTimeRelay)
            .withOptionsFrom(*outputGainRelay)
            .withNativeFunction("setAttackCurve", [this](const juce::Array<juce::var>& args, auto) {
                handleCurveUpdate(args, &OSpectralShaperAudioProcessor::setAttackCurve);
            })
            .withNativeFunction("setSustainCurve", [this](const juce::Array<juce::var>& args, auto) {
                handleCurveUpdate(args, &OSpectralShaperAudioProcessor::setSustainCurve);
            })
            // Tooltip preference (v1.5.0). The WebView pulls the stored value on
            // init rather than the editor pushing it, which would race the load.
            .withNativeFunction("setTooltipsEnabled", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                processorRef.setTooltipsEnabled(static_cast<bool>(args[0]));
                complete(juce::var(true));  // settle the JS promise; unsettled ones accumulate per click
            })
            .withNativeFunction("getTooltipsEnabled", [this](const juce::Array<juce::var>&, auto complete) {
                complete(juce::var(processorRef.getTooltipsEnabled()));
            })
            // Preset Manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this, makeDialogResult](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );
                // CR-01: capture a SafePointer, not raw `this`/`complete`. If the editor
                // is torn down while the dialog is open, bail with a bare return — the
                // `complete` callback is owned by the (now-dead) WebView Impl, so calling
                // it (even complete(false)) is itself a use-after-free.
                juce::Component::SafePointer<OSpectralShaperAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete, makeDialogResult](const juce::FileChooser& fc) {
                        if (safeThis == nullptr) return;
                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            complete(makeDialogResult(false, ""));
                            return;
                        }
                        auto presetName = results.getFirst().getFileNameWithoutExtension();
                        bool success = safeThis->processorRef.presetManager.savePreset(presetName);
                        complete(makeDialogResult(success, success ? presetName : juce::String()));
                    }
                );
            })
            .withNativeFunction("loadPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("getPresetList", [this](auto&, auto complete) {
                auto list = processorRef.presetManager.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : list)
                    arr.add(name);
                complete(juce::var(arr));
            })
            .withNativeFunction("getCurrentPreset", [this](auto&, auto complete) {
                complete(processorRef.presetManager.getCurrentPresetName());
            })
            .withNativeFunction("selectNextPreset", [this](auto&, auto complete) {
                auto next = processorRef.presetManager.getNextPreset();
                complete(next);
            })
            .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
                auto prev = processorRef.presetManager.getPreviousPreset();
                complete(prev);
            })
            .withNativeFunction("deletePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.deletePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("isFactoryPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
                else
                    complete(false);
            })
            // getPresetListGrouped (v1.6.0) — the preset MENU's only data source.
            //
            // Returns an ARRAY of { category, presets: [...] }, deliberately not
            // an object keyed by category: an object would make section order
            // depend on JS string-key insertion-order semantics surviving the
            // C++ → JSON → JS round-trip, and the alternative (a CATEGORY_ORDER
            // list in JS) is a mirror of the C++ spans that would drift the
            // first time a category is added. An array carries the order in the
            // data itself.
            //
            // Factory sections come first, in NARRATIVE order
            // (factoryCategoryOrder), then "User" holding everything
            // getPresetList() returned that is not a known factory name —
            // alphabetical, because getPresetList() sorts. Cross-checked
            // against the live list rather than emitted blind, so a factory
            // preset whose file failed to write is absent from the menu instead
            // of listed and un-loadable. Empty categories are skipped.
            .withNativeFunction("getPresetListGrouped", [this](auto&, auto complete) {
                const auto allPresets = processorRef.presetManager.getPresetList();

                juce::Array<juce::var> sections;
                juce::StringArray claimed;

                juce::String openCategory;
                juce::Array<juce::var> openNames;

                const auto flushSection = [&sections, &openCategory, &openNames]()
                {
                    if (openCategory.isNotEmpty() && ! openNames.isEmpty())
                    {
                        auto* section = new juce::DynamicObject();
                        section->setProperty("category", openCategory);
                        section->setProperty("presets", openNames);
                        sections.add(juce::var(section));
                    }
                    openNames.clear();
                };

                for (const auto& [presetName, categoryLabel] : processorRef.factoryCategoryOrder)
                {
                    if (categoryLabel != openCategory)
                    {
                        flushSection();
                        openCategory = categoryLabel;
                    }

                    if (allPresets.contains(presetName))
                    {
                        openNames.add(juce::var(presetName));
                        claimed.add(presetName);
                    }
                }
                flushSection();

                juce::Array<juce::var> userNames;
                for (const auto& presetName : allPresets)
                    if (! claimed.contains(presetName))
                        userNames.add(juce::var(presetName));

                if (! userNames.isEmpty())
                {
                    auto* section = new juce::DynamicObject();
                    section->setProperty("category", "User");
                    section->setProperty("presets", userNames);
                    sections.add(juce::var(section));
                }

                complete(juce::var(sections));
            })
            .withNativeFunction("loadPresetFromFile", [this, makeDialogResult](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );
                // CR-01: SafePointer guard — see savePresetWithDialog above. Bare return
                // on teardown; do NOT call complete() (owned by the dead WebView Impl).
                juce::Component::SafePointer<OSpectralShaperAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete, makeDialogResult](const juce::FileChooser& fc) {
                        if (safeThis == nullptr) return;
                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            complete(makeDialogResult(false, ""));
                            return;
                        }
                        auto file = results.getFirst();
                        bool success = safeThis->processorRef.presetManager.loadPresetFromFile(file);
                        complete(makeDialogResult(success, success ? file.getFileNameWithoutExtension() : juce::String()));
                    }
                );
            })
    );

    // ============================================================================
    // 3. Create attachments LAST (connect parameters to relays)
    // ============================================================================

    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MIX"), *mixRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("ATTACK_TIME"), *attackTimeRelay, nullptr);
    sustainTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("SUSTAIN_TIME"), *sustainTimeRelay, nullptr);
    sensitivityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("SENSITIVITY"), *sensitivityRelay, nullptr);
    lookaheadEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LOOKAHEAD_ENABLED"), *lookaheadEnabledRelay, nullptr);
    lookaheadTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LOOKAHEAD_TIME"), *lookaheadTimeRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    addAndMakeVisible(*webView);

    // Set editor size (700x500 for Stage 3 full UI)
    setSize(700, 500);

    // Baseline the curves revision BEFORE the timer starts: the initial curve
    // push at editor-open lives in parentHierarchyChanged(), so the poll only
    // needs to catch revisions bumped after construction.
    lastSentCurvesRevision = processorRef.getCurvesRevision();

    // Start 60fps timer for visualization updates (Phase 3.3)
    startTimerHz(60);
}

OSpectralShaperAudioProcessorEditor::~OSpectralShaperAudioProcessorEditor()
{
    // Stop timer before destruction (CRITICAL for thread safety)
    stopTimer();
}

void OSpectralShaperAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OSpectralShaperAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OSpectralShaperAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;

        // Send initial curve data to JavaScript after a short delay
        // SafePointer prevents dangling-this crash if editor is destroyed within 100ms
        auto safeThis = juce::Component::SafePointer<OSpectralShaperAudioProcessorEditor>(this);
        juce::Timer::callAfterDelay(100, [safeThis]() {
            if (safeThis == nullptr) return;
            safeThis->sendAttackCurveToJS();
            safeThis->sendSustainCurveToJS();
        });
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OSpectralShaperAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // CSS
    if (url == "/css/styles.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::styles_css, BinaryData::styles_cssSize),
            juce::String("text/css")
        };
    }

    // JavaScript modules
    if (url == "/js/app.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::app_js, BinaryData::app_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/RotaryKnob.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::RotaryKnob_js, BinaryData::RotaryKnob_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/CurveEditor.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::CurveEditor_js, BinaryData::CurveEditor_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/FreehandCurve.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::FreehandCurve_js, BinaryData::FreehandCurve_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/NodeCurve.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::NodeCurve_js, BinaryData::NodeCurve_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/Spectrogram.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::Spectrogram_js, BinaryData::Spectrogram_jsSize),
            juce::String("text/javascript")
        };
    }

    // Images
    if (url == "/images/paper-bg.webp") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paperbg_webp, BinaryData::paperbg_webpSize),
            juce::String("image/webp")
        };
    }

    if (url == "/images/slug-overlay.webp") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::slugoverlay_webp, BinaryData::slugoverlay_webpSize),
            juce::String("image/webp")
        };
    }

    // Preset Manager JS module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

void OSpectralShaperAudioProcessorEditor::handleCurveUpdate(
    const juce::Array<juce::var>& args,
    void (OSpectralShaperAudioProcessor::*setter)(const std::array<float, 32>&))
{
    if (args.size() != 32) return;

    std::array<float, 32> curveData;
    for (int i = 0; i < 32; ++i)
    {
        auto val = static_cast<float>(args[i]);
        curveData[i] = std::isnan(val) ? 0.0f : juce::jlimit(-1.0f, 1.0f, val);
    }

    (processorRef.*setter)(curveData);
}

void OSpectralShaperAudioProcessorEditor::sendCurveToJS(
    const char* functionName, const std::array<float, 32>& curve)
{
    if (!webView) return;

    juce::String jsArray = "[";
    for (size_t i = 0; i < curve.size(); ++i) {
        jsArray += juce::String(curve[i]);
        if (i < curve.size() - 1) jsArray += ",";
    }
    jsArray += "]";

    webView->evaluateJavascript(
        juce::String("if (window.") + functionName + ") window." + functionName + "(" + jsArray + ");");
}

void OSpectralShaperAudioProcessorEditor::sendAttackCurveToJS()
{
    sendCurveToJS("setAttackCurveFromCPP", processorRef.getAttackCurve());
}

void OSpectralShaperAudioProcessorEditor::sendSustainCurveToJS()
{
    sendCurveToJS("setSustainCurveFromCPP", processorRef.getSustainCurve());
}

void OSpectralShaperAudioProcessorEditor::emitVisualizationFrame(
    const OSpectralShaperAudioProcessor::VisualizationFrame& frame)
{
    if (!webView) return;

    // Bail BEFORE building the payload, not after. emitEventIfBrowserIsVisible()
    // already drops the event when the browser is hidden, but the ~4 kB JSON
    // below (289 float→String conversions) is built unconditionally on the way
    // to it, and it is the dominant per-frame cost when a backlog drains. The
    // gate is isVisible() — deliberately the exact condition JUCE itself tests
    // in emitEventIfBrowserIsVisible(), not the stricter isShowing(), so this
    // can only skip frames that were going to be discarded anyway.
    if (!webView->isVisible()) return;

    // Pre-allocate to avoid repeated heap allocations at 60fps
    // 257 bins * ~10 chars + 32 bands * ~10 chars + overhead ≈ 3200 bytes
    juce::String json;
    json.preallocateBytes(4096);
    json << "{\"fft\":[";
    for (size_t i = 0; i < frame.fftMagnitudes.size(); ++i)
    {
        if (i > 0) json << ',';
        json << juce::String(frame.fftMagnitudes[i], 6);
    }
    json << "],\"transients\":[";
    for (size_t i = 0; i < frame.transientActivity.size(); ++i)
    {
        if (i > 0) json << ',';
        json << juce::String(frame.transientActivity[i], 6);
    }
    json << "]}";

    webView->emitEventIfBrowserIsVisible("visualizationUpdate", json);
}

void OSpectralShaperAudioProcessorEditor::timerCallback()
{
    // Re-send the curves when a preset load or session restore replaced them
    // (curvesRevision is bumped only by the customLoad state callback, never
    // by UI drag-edits, so this cannot fight an in-progress drag). Gated on
    // hasNavigated: before navigation there is no page to evaluate against —
    // lastSentCurvesRevision stays behind and the send happens once there is.
    if (const auto revision = processorRef.getCurvesRevision();
        hasNavigated && revision != lastSentCurvesRevision)
    {
        lastSentCurvesRevision = revision;
        sendAttackCurveToJS();
        sendSustainCurveToJS();
    }

    auto& fifo = processorRef.getVisualizationFifo();
    const auto& buffer = processorRef.getVisualizationBuffer();

    // Snapshot the pending count ONCE, then read exactly that many frames.
    //
    // This was `while (fifo.getNumReady() > 0)`, which re-asks the audio thread
    // how much is pending on every iteration. At realtime that terminates:
    // HOP_SIZE 256 at 48 kHz produces 187 frames/sec against this 60 Hz timer,
    // so ~3 frames drain per tick and the count reaches zero. But processBlock
    // is not rate-limited during an offline bounce or a validator's editor test
    // — the producer then outruns the message thread, getNumReady() never
    // returns 0, and timerCallback() never returns. The message loop starves and
    // the WebView never finishes opening: Windows pluginval strictness 10 hung
    // in "Open editor whilst processing" until its 10-minute timeout and failed
    // the v1.6.1 release build. A snapshot bounds the work per tick to what was
    // pending when the tick began, so frames arriving mid-drain wait for the
    // next tick instead of extending this one.
    const int ready = fifo.getNumReady();

    if (ready <= 0)
        return;

    int start1, size1, start2, size2;
    fifo.prepareToRead(ready, start1, size1, start2, size2);
    const int total = size1 + size2;

    // Emit at most maxVisualizationFramesPerTick, taking the NEWEST of the
    // snapshot. A backlog deeper than that only builds when the producer is
    // running faster than realtime, and those columns are already stale by the
    // time they would be drawn. The cap is well clear of every realtime rate:
    // 187 frames/sec at 48 kHz is ~3 per tick, and even 192 kHz is ~13, so
    // nothing is dropped during normal playback at any supported sample rate.
    const int firstToEmit = juce::jmax(0, total - maxVisualizationFramesPerTick);

    for (int i = firstToEmit; i < total; ++i)
    {
        const int index = i < size1 ? start1 + i : start2 + (i - size1);
        emitVisualizationFrame(buffer[static_cast<size_t>(index)]);
    }

    // Retire the whole snapshot, including frames the cap skipped — otherwise
    // the dropped ones stay ready and every later tick re-reads the same
    // backlog, which reinstates the stall this fix removes.
    fifo.finishedRead(total);
}
