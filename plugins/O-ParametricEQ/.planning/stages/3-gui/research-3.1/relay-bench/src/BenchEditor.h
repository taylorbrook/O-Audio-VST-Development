#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include "PluginProcessor.h"
#include "analysis/DisplayModel.h"

namespace opeq
{
    inline constexpr int kNumBands = 24;

    inline juce::String bandPrefix (int bandIndex)
    {
        return "b" + juce::String (bandIndex + 1).paddedLeft ('0', 2) + "_";
    }

    // Verbatim structure of v1-PluginEditor-TEMPLATE.h
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
              placementRelay (prefix + "placement") {}

        juce::WebToggleButtonRelay enabledRelay;
        juce::WebToggleButtonRelay bypassRelay;
        juce::WebComboBoxRelay     typeRelay;
        juce::WebSliderRelay       freqRelay;
        juce::WebSliderRelay       gainRelay;
        juce::WebSliderRelay       qRelay;
        juce::WebComboBoxRelay     slopeRelay;
        juce::WebComboBoxRelay     placementRelay;

        [[nodiscard]] juce::WebBrowserComponent::Options addTo (juce::WebBrowserComponent::Options options)
        {
            return options.withOptionsFrom (enabledRelay).withOptionsFrom (bypassRelay)
                          .withOptionsFrom (typeRelay).withOptionsFrom (freqRelay)
                          .withOptionsFrom (gainRelay).withOptionsFrom (qRelay)
                          .withOptionsFrom (slopeRelay).withOptionsFrom (placementRelay);
        }
        JUCE_DECLARE_NON_COPYABLE (BandRelays)
    };

    struct BandAttachments
    {
        BandAttachments (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, BandRelays& r)
            : enabledAttachment   (*apvts.getParameter (prefix + "enabled"),   r.enabledRelay,   nullptr),
              bypassAttachment    (*apvts.getParameter (prefix + "bypass"),    r.bypassRelay,    nullptr),
              typeAttachment      (*apvts.getParameter (prefix + "type"),      r.typeRelay,      nullptr),
              freqAttachment      (*apvts.getParameter (prefix + "freq"),      r.freqRelay,      nullptr),
              gainAttachment      (*apvts.getParameter (prefix + "gain"),      r.gainRelay,      nullptr),
              qAttachment         (*apvts.getParameter (prefix + "q"),         r.qRelay,         nullptr),
              slopeAttachment     (*apvts.getParameter (prefix + "slope"),     r.slopeRelay,     nullptr),
              placementAttachment (*apvts.getParameter (prefix + "placement"), r.placementRelay, nullptr) {}

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
}

struct BenchTimings
{
    double relaysMs = 0, optionsMs = 0, webViewMs = 0, attachMs = 0, restMs = 0, totalMs = 0;
    int numRelays = 0, numEventListeners = 0;
    int64_t estUserScriptBytes = 0, estEventIdScriptBytes = 0, estInitDataBytes = 0;
};

class BenchEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    using BenchCallback = std::function<void (const juce::var&, double nowMs)>;

    BenchEditor (OuariconParametricEQAudioProcessor&, bool fallback, BenchCallback);
    ~BenchEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

    juce::WebBrowserComponent* getWebView() { return webView.get(); }
    void setStreaming (bool on) { if (on) startTimerHz (30); else stopTimer(); }

    BenchTimings timings;
    std::map<juce::String, int> nativeCalls;
    juce::StringArray missingResources;
    int resourceRequests = 0;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    juce::WebBrowserComponent::Options addNativeFunctions (juce::WebBrowserComponent::Options);

    OuariconParametricEQAudioProcessor& audioProcessor;
    bool fallback;
    BenchCallback onBench;
    DisplayModel displayModel { audioProcessor };
    bool fullPush = false;

    // Relays -> WebView -> attachments (template order)
    std::unique_ptr<juce::WebToggleButtonRelay> bypassRelay;
    std::unique_ptr<juce::WebSliderRelay>       outputGainRelay;
    std::unique_ptr<juce::WebSliderRelay>       gainScaleRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoGainRelay;
    std::array<std::unique_ptr<opeq::BandRelays>, opeq::kNumBands> bandRelays;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       outputGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       gainScaleAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoGainAttachment;
    std::array<std::unique_ptr<opeq::BandAttachments>, opeq::kNumBands> bandAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BenchEditor)
};
