/*
   This file is part of the Ouaricon Audio webview-relay-manager module.
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

    WebViewRelayManager.h
    Ouaricon Module System - WebView Relay Lifecycle

    Ensures correct member declaration order for WebView relay/attachment pattern.
    CRITICAL: Prevents release build crashes from attachment destruction order.

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include <string>

/**
 * WebView Relay Manager
 *
 * This class manages the lifecycle of WebView relays and attachments,
 * ensuring they are created and destroyed in the correct order.
 *
 * ============================================================================
 * ⚠️ CRITICAL PATTERN: MEMBER DECLARATION ORDER ⚠️
 * ============================================================================
 *
 * C++ destroys members in REVERSE declaration order.
 * WebSliderParameterAttachment::~WebSliderParameterAttachment() calls
 * evaluateJavascript() on the WebView during destruction.
 *
 * WRONG ORDER (causes crash):
 *   webView → relays → attachments
 *   Destruction: attachments (calls webView) → relays → webView (already dead!)
 *
 * CORRECT ORDER:
 *   relays → webView → attachments
 *   Destruction: attachments (calls webView - still alive) → webView → relays
 *
 * ============================================================================
 *
 * Usage Option 1: Manual Pattern (Recommended for full control)
 *
 *   class MyEditor : public juce::AudioProcessorEditor {
 *       // 1️⃣ RELAYS FIRST
 *       std::unique_ptr<juce::WebSliderRelay> gainRelay;
 *       std::unique_ptr<juce::WebToggleButtonRelay> bypassRelay;
 *
 *       // 2️⃣ WEBVIEW SECOND
 *       std::unique_ptr<juce::WebBrowserComponent> webView;
 *
 *       // 3️⃣ ATTACHMENTS LAST
 *       std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;
 *       std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;
 *   };
 *
 * Usage Option 2: Managed Pattern (Using this class)
 *
 *   class MyEditor : public juce::AudioProcessorEditor {
 *       WebViewRelayManager relayManager;  // Handles all relay/attachment lifecycle
 *   };
 *
 *   // In constructor:
 *   relayManager.createSliderRelay("gain");
 *   relayManager.createToggleRelay("bypass");
 *   relayManager.initializeWebView(resourceProvider);
 *   relayManager.createSliderAttachment("gain", *parameters.getParameter("gain"));
 *   relayManager.createToggleAttachment("bypass", *parameters.getParameter("bypass"));
 */
class WebViewRelayManager
{
public:
    WebViewRelayManager() = default;
    ~WebViewRelayManager() = default;

    //==========================================================================
    // Step 1: Create Relays
    //==========================================================================

    /**
     * Create a slider relay for a parameter.
     * Must be called BEFORE initializeWebView().
     */
    juce::WebSliderRelay* createSliderRelay(const juce::String& paramId)
    {
        auto relay = std::make_unique<juce::WebSliderRelay>(paramId);
        auto* ptr = relay.get();
        sliderRelays.push_back(std::move(relay));
        sliderRelayMap[paramId.toStdString()] = ptr;
        return ptr;
    }

    /**
     * Create a toggle button relay for a parameter.
     * Must be called BEFORE initializeWebView().
     */
    juce::WebToggleButtonRelay* createToggleRelay(const juce::String& paramId)
    {
        auto relay = std::make_unique<juce::WebToggleButtonRelay>(paramId);
        auto* ptr = relay.get();
        toggleRelays.push_back(std::move(relay));
        toggleRelayMap[paramId.toStdString()] = ptr;
        return ptr;
    }

    //==========================================================================
    // Step 2: Initialize WebView
    //==========================================================================

    /**
     * Initialize WebView with all registered relays.
     * Must be called AFTER all relays are created.
     *
     * @param resourceProvider  Lambda that provides resources for URLs
     * @return Pointer to the created WebView
     */
    template<typename ResourceProviderFunc>
    juce::WebBrowserComponent* initializeWebView(ResourceProviderFunc resourceProvider)
    {
        juce::WebBrowserComponent::Options options;
        options = options.withNativeIntegrationEnabled();
        options = options.withResourceProvider(resourceProvider);

        // Add all slider relays
        for (const auto& relay : sliderRelays)
        {
            options = options.withOptionsFrom(*relay);
        }

        // Add all toggle relays
        for (const auto& relay : toggleRelays)
        {
            options = options.withOptionsFrom(*relay);
        }

        webView = std::make_unique<juce::WebBrowserComponent>(options);
        return webView.get();
    }

    //==========================================================================
    // Step 3: Create Attachments
    //==========================================================================

    /**
     * Create a slider parameter attachment.
     * Must be called AFTER initializeWebView().
     *
     * @param paramId  Parameter ID (must match a created relay)
     * @param param    Reference to the RangedAudioParameter
     * @param undo     Optional undo manager (usually nullptr)
     */
    void createSliderAttachment(const juce::String& paramId,
                                 juce::RangedAudioParameter& param,
                                 juce::UndoManager* undo = nullptr)
    {
        auto it = sliderRelayMap.find(paramId.toStdString());
        if (it == sliderRelayMap.end())
        {
            jassertfalse; // Relay not found - did you forget to create it?
            return;
        }

        auto attachment = std::make_unique<juce::WebSliderParameterAttachment>(
            param, *it->second, undo);
        sliderAttachments.push_back(std::move(attachment));
    }

    /**
     * Create a toggle button parameter attachment.
     * Must be called AFTER initializeWebView().
     */
    void createToggleAttachment(const juce::String& paramId,
                                 juce::RangedAudioParameter& param,
                                 juce::UndoManager* undo = nullptr)
    {
        auto it = toggleRelayMap.find(paramId.toStdString());
        if (it == toggleRelayMap.end())
        {
            jassertfalse; // Relay not found
            return;
        }

        auto attachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            param, *it->second, undo);
        toggleAttachments.push_back(std::move(attachment));
    }

    //==========================================================================
    // Access
    //==========================================================================

    /** Get the WebView component. */
    juce::WebBrowserComponent* getWebView() { return webView.get(); }
    const juce::WebBrowserComponent* getWebView() const { return webView.get(); }

    /** Get a slider relay by ID. */
    juce::WebSliderRelay* getSliderRelay(const juce::String& paramId)
    {
        auto it = sliderRelayMap.find(paramId.toStdString());
        return (it != sliderRelayMap.end()) ? it->second : nullptr;
    }

    /** Get a toggle relay by ID. */
    juce::WebToggleButtonRelay* getToggleRelay(const juce::String& paramId)
    {
        auto it = toggleRelayMap.find(paramId.toStdString());
        return (it != toggleRelayMap.end()) ? it->second : nullptr;
    }

private:
    //==========================================================================
    // Member Declaration Order is CRITICAL
    // Destruction happens in REVERSE order
    //==========================================================================

    // 1️⃣ RELAYS FIRST (destroyed last)
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;
    std::map<std::string, juce::WebSliderRelay*> sliderRelayMap;
    std::map<std::string, juce::WebToggleButtonRelay*> toggleRelayMap;

    // 2️⃣ WEBVIEW SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (destroyed first - can safely use webView)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewRelayManager)
};


//==============================================================================
// Convenience Macros for Manual Pattern
//==============================================================================

/**
 * Macro to declare relay members in correct order.
 * Use in private section of Editor class.
 *
 * OUARICON_DECLARE_RELAY(gain)
 * expands to:
 * std::unique_ptr<juce::WebSliderRelay> gainRelay;
 */
#define OUARICON_DECLARE_RELAY(name) \
    std::unique_ptr<juce::WebSliderRelay> name##Relay

#define OUARICON_DECLARE_TOGGLE_RELAY(name) \
    std::unique_ptr<juce::WebToggleButtonRelay> name##Relay

/**
 * Macro to declare attachment members.
 *
 * OUARICON_DECLARE_ATTACHMENT(gain)
 * expands to:
 * std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;
 */
#define OUARICON_DECLARE_ATTACHMENT(name) \
    std::unique_ptr<juce::WebSliderParameterAttachment> name##Attachment

#define OUARICON_DECLARE_TOGGLE_ATTACHMENT(name) \
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> name##Attachment

/**
 * Macro to create relay in constructor.
 *
 * OUARICON_CREATE_RELAY(gain, "GAIN")
 */
#define OUARICON_CREATE_RELAY(name, paramId) \
    name##Relay = std::make_unique<juce::WebSliderRelay>(paramId)

#define OUARICON_CREATE_TOGGLE_RELAY(name, paramId) \
    name##Relay = std::make_unique<juce::WebToggleButtonRelay>(paramId)

/**
 * Macro to create attachment in constructor.
 *
 * OUARICON_CREATE_ATTACHMENT(gain, parameters.getParameter("GAIN"))
 */
#define OUARICON_CREATE_ATTACHMENT(name, param) \
    name##Attachment = std::make_unique<juce::WebSliderParameterAttachment>( \
        *param, *name##Relay, nullptr)

#define OUARICON_CREATE_TOGGLE_ATTACHMENT(name, param) \
    name##Attachment = std::make_unique<juce::WebToggleButtonParameterAttachment>( \
        *param, *name##Relay, nullptr)
