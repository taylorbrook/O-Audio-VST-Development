/*
  ==============================================================================

    OuariconLicenseUI.h
    Ouaricon Module System - License Activation Overlay

    Native JUCE component that overlays the plugin editor until a valid
    license is activated. Auto-hides when licensed.

    Usage:
      // In PluginEditor constructor:
      licenseOverlay = std::make_unique<OuariconLicenseOverlay> (licenseManager);
      addAndMakeVisible (licenseOverlay.get());

      // In PluginEditor::resized():
      licenseOverlay->setBounds (getLocalBounds());

  ==============================================================================
*/

#pragma once

#include "OuariconLicense.h"
#include <juce_gui_basics/juce_gui_basics.h>

class OuariconLicenseOverlay : public juce::Component,
                               private OuariconLicense::Listener
{
public:
    OuariconLicenseOverlay (OuariconLicense& license)
        : licenseManager (license)
    {
        licenseManager.addListener (this);

        // Title
        titleLabel.setText ("Enter License Key", juce::dontSendNotification);
        titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (titleLabel);

        // Subtitle
        subtitleLabel.setText ("Paste your license key to activate this plugin",
                               juce::dontSendNotification);
        subtitleLabel.setFont (juce::FontOptions (13.0f));
        subtitleLabel.setJustificationType (juce::Justification::centred);
        subtitleLabel.setColour (juce::Label::textColourId,
                                 juce::Colour (0xffaaaaaa));
        addAndMakeVisible (subtitleLabel);

        // License key input
        keyInput.setMultiLine (false);
        keyInput.setReturnKeyStartsNewLine (false);
        keyInput.setTextToShowWhenEmpty ("OUA-XXXX-XXXX-XXXX",
                                         juce::Colour (0xff666666));
        keyInput.setFont (juce::FontOptions (18.0f));
        keyInput.setJustification (juce::Justification::centred);
        keyInput.setColour (juce::TextEditor::backgroundColourId,
                            juce::Colour (0xff2a2a2a));
        keyInput.setColour (juce::TextEditor::outlineColourId,
                            juce::Colour (0xff444444));
        keyInput.setColour (juce::TextEditor::textColourId,
                            juce::Colours::white);
        keyInput.setColour (juce::TextEditor::focusedOutlineColourId,
                            juce::Colour (0xff6a9fd8));
        keyInput.setInputRestrictions (16, "OUAoua-0123456789ABCDEFabcdef");
        addAndMakeVisible (keyInput);

        // Activate button
        activateButton.setButtonText ("Activate");
        activateButton.setColour (juce::TextButton::buttonColourId,
                                   juce::Colour (0xff4a7fb5));
        activateButton.setColour (juce::TextButton::textColourOnId,
                                   juce::Colours::white);
        activateButton.onClick = [this]() { onActivateClicked(); };
        addAndMakeVisible (activateButton);

        // Status label
        statusLabel.setFont (juce::FontOptions (13.0f));
        statusLabel.setJustificationType (juce::Justification::centred);
        statusLabel.setColour (juce::Label::textColourId,
                                juce::Colour (0xffaaaaaa));
        addAndMakeVisible (statusLabel);

        // Initially visible only if not licensed
        updateVisibility();
    }

    ~OuariconLicenseOverlay() override
    {
        licenseManager.removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        // Semi-transparent dark overlay
        g.fillAll (juce::Colour (0xf0111111));

        // Card background
        auto cardBounds = getCardBounds();
        g.setColour (juce::Colour (0xff1e1e1e));
        g.fillRoundedRectangle (cardBounds.toFloat(), 12.0f);

        // Card border
        g.setColour (juce::Colour (0xff333333));
        g.drawRoundedRectangle (cardBounds.toFloat(), 12.0f, 1.0f);
    }

    void resized() override
    {
        auto card = getCardBounds();
        auto area = card.reduced (30, 20);

        titleLabel.setBounds (area.removeFromTop (32));
        area.removeFromTop (4);
        subtitleLabel.setBounds (area.removeFromTop (20));
        area.removeFromTop (20);
        keyInput.setBounds (area.removeFromTop (40));
        area.removeFromTop (16);

        auto buttonArea = area.removeFromTop (36);
        auto buttonWidth = juce::jmin (200, buttonArea.getWidth());
        activateButton.setBounds (buttonArea.withSizeKeepingCentre (buttonWidth, 36));

        area.removeFromTop (12);
        statusLabel.setBounds (area.removeFromTop (20));
    }

private:
    OuariconLicense& licenseManager;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::TextEditor keyInput;
    juce::TextButton activateButton;
    juce::Label statusLabel;

    juce::Rectangle<int> getCardBounds() const
    {
        auto w = juce::jmin (420, getWidth() - 40);
        auto h = 260;
        return juce::Rectangle<int> (w, h)
                   .withCentre (getLocalBounds().getCentre());
    }

    void onActivateClicked()
    {
        auto key = keyInput.getText().trim().toUpperCase();

        if (key.isEmpty())
        {
            showStatus ("Please enter a license key", true);
            return;
        }

        // Basic format validation: OUA-XXXX-XXXX-XXXX
        if (! key.matchesWildcard ("OUA-????-????-????", false))
        {
            showStatus ("Invalid format. Expected: OUA-XXXX-XXXX-XXXX", true);
            return;
        }

        activateButton.setEnabled (false);
        activateButton.setButtonText ("Activating...");
        showStatus ("Contacting license server...", false);

        licenseManager.activate (key, [this] (bool success, juce::String message)
        {
            activateButton.setEnabled (true);
            activateButton.setButtonText ("Activate");
            showStatus (message, ! success);

            if (success)
                updateVisibility();
        });
    }

    void showStatus (const juce::String& message, bool isError)
    {
        statusLabel.setText (message, juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId,
                                isError ? juce::Colour (0xffdd4444)
                                        : juce::Colour (0xffaaaaaa));
    }

    void updateVisibility()
    {
        setVisible (! licenseManager.isLicensed());
    }

    // OuariconLicense::Listener
    void licenseStatusChanged (OuariconLicense&, OuariconLicense::Status) override
    {
        juce::MessageManager::callAsync ([this]() { updateVisibility(); });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuariconLicenseOverlay)
};
