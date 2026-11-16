/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
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

    O-ParametricEQ - Editor (WebView UI), Phase 3.2

    Window: 960 x 600 default, resizable 800 x 500 to 1600 x 1000, aspect free.
    Parameters: 196 host parameters = 196 relays + 196 attachments
                (74 slider, 50 toggle, 72 combo). parameter-spec.md is the
                contract.

    The 196 relays are built from loops over 24 bands. Each band is one
    BandRelays / BandAttachments struct built from its ID prefix
    ("b01_" ... "b24_").

    Phase 3.1 (PLAN-3.1 P2): bindings, 19 native functions, the pageReady
    timer gate. Phase 3.2 (PLAN-3.2 P2): the 60 Hz timer emits eqSections
    from an editor-owned DisplayModel. 3.3 adds the analyzer (an editor-owned
    SpectrumAnalyzer) and the output meter.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include "PluginProcessor.h"

class DisplayModel;

namespace opeq
{
    inline constexpr int kNumBands = 24;

    /** "b01_" ... "b24_" for band index 0 ... 23. Must match createParameterLayout(). */
    inline juce::String bandPrefix (int bandIndex)
    {
        return "b" + juce::String (bandIndex + 1).paddedLeft ('0', 2) + "_";
    }

    //==========================================================================
    /** The eight relays of one band, in parameter-spec.md order.
        Constructed from the ID prefix; each relay name is the full parameter ID. */
    struct BandRelays
    {
        explicit BandRelays (const juce::String& prefix)
            : enabledRelay   (prefix + "enabled"),
              bypassRelay    (prefix + "bypass"),
              typeRelay      (prefix + "type"),
              freqRelay      (prefix + "freq"),
              gainRelay      (prefix + "gain"),
              qRelay         (prefix + "q"),
              slopeRelay     (prefix + "slope"),
              placementRelay (prefix + "placement")
        {
        }

        juce::WebToggleButtonRelay enabledRelay;
        juce::WebToggleButtonRelay bypassRelay;
        juce::WebComboBoxRelay     typeRelay;
        juce::WebSliderRelay       freqRelay;
        juce::WebSliderRelay       gainRelay;
        juce::WebSliderRelay       qRelay;
        juce::WebComboBoxRelay     slopeRelay;
        juce::WebComboBoxRelay     placementRelay;

        /** Registers all eight with the WebView options. Every relay kind needs
            withOptionsFrom, toggles and combos included, or the page logs
            "unknown to the backend" and the control never updates. */
        [[nodiscard]] juce::WebBrowserComponent::Options
            addTo (juce::WebBrowserComponent::Options options)
        {
            return options.withOptionsFrom (enabledRelay)
                          .withOptionsFrom (bypassRelay)
                          .withOptionsFrom (typeRelay)
                          .withOptionsFrom (freqRelay)
                          .withOptionsFrom (gainRelay)
                          .withOptionsFrom (qRelay)
                          .withOptionsFrom (slopeRelay)
                          .withOptionsFrom (placementRelay);
        }

        JUCE_DECLARE_NON_COPYABLE (BandRelays)
    };

    //==========================================================================
    /** The eight attachments of one band. Three-argument attachment constructors
        (parameter, relay, undo manager) are required in JUCE 8. */
    struct BandAttachments
    {
        BandAttachments (juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& prefix,
                         BandRelays& relays)
            : enabledAttachment   (*apvts.getParameter (prefix + "enabled"),   relays.enabledRelay,   nullptr),
              bypassAttachment    (*apvts.getParameter (prefix + "bypass"),    relays.bypassRelay,    nullptr),
              typeAttachment      (*apvts.getParameter (prefix + "type"),      relays.typeRelay,      nullptr),
              freqAttachment      (*apvts.getParameter (prefix + "freq"),      relays.freqRelay,      nullptr),
              gainAttachment      (*apvts.getParameter (prefix + "gain"),      relays.gainRelay,      nullptr),
              qAttachment         (*apvts.getParameter (prefix + "q"),         relays.qRelay,         nullptr),
              slopeAttachment     (*apvts.getParameter (prefix + "slope"),     relays.slopeRelay,     nullptr),
              placementAttachment (*apvts.getParameter (prefix + "placement"), relays.placementRelay, nullptr)
        {
        }

        juce::WebToggleButtonParameterAttachment enabledAttachment;
        juce::WebToggleButtonParameterAttachment bypassAttachment;
        juce::WebComboBoxParameterAttachment     typeAttachment;
        juce::WebSliderParameterAttachment       freqAttachment;
        juce::WebSliderParameterAttachment       gainAttachment;
        juce::WebSliderParameterAttachment       qAttachment;
        juce::WebComboBoxParameterAttachment     slopeAttachment;
        juce::WebComboBoxParameterAttachment     placementAttachment;

        JUCE_DECLARE_NON_COPYABLE (BandAttachments)
    };

   #if defined (OPEQ_EDITOR_BENCH)
    //==========================================================================
    /** Editor-bench hooks (PLAN-3.1 P10). Compiled ONLY when the bench target
        defines OPEQ_EDITOR_BENCH; the plugin build never does. The bench app
        fills these before it constructs the editor. */
    namespace bench
    {
        struct Hooks
        {
            /** Injected with withUserScript when not empty. */
            juce::String userScript;

            /** Called by the bench native function with its first argument and
                the message-thread time in ms (Time::getMillisecondCounterHiRes). */
            std::function<void (const juce::var&, double)> onReport;

            /** Called on the message thread for every eqSections emit, with
                the 0-based bands it carries (PLAN-3.2 E-lag / E-stopped). */
            std::function<void (const juce::Array<int>&)> onEqSections;
        };

        Hooks& hooks();
    }
   #endif
}

//==============================================================================
class OuariconParametricEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit OuariconParametricEQAudioProcessorEditor (OuariconParametricEQAudioProcessor&);
    ~OuariconParametricEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Window limits from v1-ui.yaml. Aspect is free; the page reflows.
    static constexpr int kDefaultWidth  = 960;
    static constexpr int kDefaultHeight = 600;
    static constexpr int kMinWidth      = 800;
    static constexpr int kMinHeight     = 500;
    static constexpr int kMaxWidth      = 1600;
    static constexpr int kMaxHeight     = 1000;

private:
    void timerCallback() override;

    /** Resource provider: explicit URL -> embedded data map, bare paths. */
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    /** Every native function the page calls (19). One missing registration
        leaves a promise that never settles: the control is dead while build,
        auval and pluginval all pass. */
    juce::WebBrowserComponent::Options addNativeFunctions (juce::WebBrowserComponent::Options);

    /** Appends one line to ~/Library/O-ParametricEQ/Logs/ui-timing.log; only
        called when the flag file existed at construction (P10). */
    void writeTimingLog (double pagePaintMs);

    // Reference to processor
    OuariconParametricEQAudioProcessor& audioProcessor;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Order: Relays -> WebView -> Attachments
    // Members are destroyed in REVERSE order of declaration. Attachments call
    // into the WebView while they are destroyed, so they must go first.
    // Wrong order crashes in release builds only.
    // ========================================================================

    // 1. RELAYS FIRST (no dependencies)
    // Global (4)
    std::unique_ptr<juce::WebToggleButtonRelay> bypassRelay;
    std::unique_ptr<juce::WebSliderRelay>       outputGainRelay;
    std::unique_ptr<juce::WebSliderRelay>       gainScaleRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoGainRelay;
    // Bands (24 x 8 = 192), index 0 = band 1
    std::array<std::unique_ptr<opeq::BandRelays>, opeq::kNumBands> bandRelays;

    // 2. WEBVIEW SECOND (depends on relays)
    // Null on a Windows machine where WebView2 cannot start; every use outside
    // the constructor is guarded with `if (webView != nullptr)`.
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on relays and webView)
    // Global (4)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       outputGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       gainScaleAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoGainAttachment;
    // Bands (24 x 8 = 192)
    std::array<std::unique_ptr<opeq::BandAttachments>, opeq::kNumBands> bandAttachments;

    // ------------------------------------------------------------------------
    // Everything below has no bearing on the relay / WebView / attachment order.
    // ------------------------------------------------------------------------

    // The response display's section packets (message thread only). Polled by
    // timerCallback; it reads the processor's parameter atomics, nothing else.
    std::unique_ptr<DisplayModel> displayModel;

    // Set by requestEqSections: the next tick emits all 24 bands.
    bool fullPushPending = true;

    // Set by the page's pageReady call (its states are bound); the 60 Hz timer
    // starts there, never before, and sends nothing while this is false
    // (an event before the page loads asserts on window.__JUCE__.backend).
    bool pageIsReady = false;

    // Preset save / load dialogs (must outlive the async dialog).
    std::unique_ptr<juce::FileChooser> fileChooser;
    // One native dialog at a time; a flag, not a `fileChooser != nullptr` test
    // (see O-AnalogEQ PluginEditor.h, IN-08, for why).
    bool presetDialogInFlight = false;

    // Timing log (P10): decided once in the constructor, message thread.
    bool   timingLogEnabled = false;
    double constructedAtMs  = 0.0;

    // Windows only: shown instead of the WebView when WebView2 cannot start
    // (webView stays null). Null on macOS and whenever the WebView exists.
    std::unique_ptr<juce::HyperlinkButton> webView2RuntimeLink;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuariconParametricEQAudioProcessorEditor)
};
