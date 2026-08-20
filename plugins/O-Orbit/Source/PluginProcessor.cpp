/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

OOrbitProcessor::OOrbitProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OOrbitParams"), createParameterLayout())
{
    // Cache parameter pointers
    pathParam          = parameters.getRawParameterValue ("path");
    speedParam         = parameters.getRawParameterValue ("speed");
    widthParam         = parameters.getRawParameterValue ("width");
    depthParam         = parameters.getRawParameterValue ("depth");
    tiltParam          = parameters.getRawParameterValue ("tilt");
    phaseParam         = parameters.getRawParameterValue ("phase");
    elevEnableParam    = parameters.getRawParameterValue ("elevation_enable");
    elevRangeParam     = parameters.getRawParameterValue ("elevation_range");
    tempoSyncParam     = parameters.getRawParameterValue ("tempo_sync");

    speakerLayoutParam = parameters.getRawParameterValue ("speaker_layout");
    distanceParam      = parameters.getRawParameterValue ("distance");
    airAbsorptionParam = parameters.getRawParameterValue ("air_absorption");
    attenCurveParam    = parameters.getRawParameterValue ("attenuation_curve");
    centerDivergeParam = parameters.getRawParameterValue ("center_diverge");

    sourceModeParam    = parameters.getRawParameterValue ("source_mode");
    lrOffsetParam      = parameters.getRawParameterValue ("lr_offset");
    mixParam           = parameters.getRawParameterValue ("mix");

    // ── Factory preset bank (B1, v1.1.0) ────────────────────────────────────
    // The 12 presets keep their v1.0.0 names and engineering-unit values
    // (getFactoryPresets() is still the single authoring source); here they
    // are converted through each parameter's own NormalisableRange into the
    // normalized domain the preset-manager stores — raw-fraction authoring
    // would ignore the skew on speed and distance.
    const auto& factory = getFactoryPresets();

    std::vector<OuariconPresetManager::FactoryPresetDef> factoryDefs;
    factoryDefs.reserve (factory.size());
    for (const auto& preset : factory)
    {
        OuariconPresetManager::FactoryPresetDef def;
        def.name = preset.name;
        for (const auto& [paramId, value] : preset.values)
            if (auto* p = parameters.getParameter (paramId))
                def.parameters[paramId] = p->convertTo0to1 (value);
        factoryDefs.push_back (std::move (def));
    }

    // Category spans over the authored order — inclusive [first, last], must
    // tile [0, size). A preset added without extending a span would show up
    // under "User" in the menu beside the user's own saves.
    struct CategorySpan { const char* label; int first, last; };
    static constexpr CategorySpan categorySpans[] {
        { "Stereo",   0,  4 },
        { "Surround", 5,  7 },
        { "Creative", 8, 11 },
    };

    {
        int expectedFirst = 0;
        for (const auto& span : categorySpans)
        {
            jassert (span.first == expectedFirst);
            jassert (span.last < (int) factoryDefs.size());
            for (int i = span.first; i <= span.last; ++i)
                factoryCategoryOrder.push_back ({ factoryDefs[(size_t) i].name, span.label });
            expectedFirst = span.last + 1;
        }
        jassert (expectedFirst == (int) factoryDefs.size());
        juce::ignoreUnused (expectedFirst);
    }

    // Sentinel-gated: file writes happen only when JucePlugin_VersionString
    // changes — auval/pluginval scan-storm safe.
    presetManager.initializeFactoryPresets (factoryDefs);
}

OOrbitProcessor::~OOrbitProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout OOrbitProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── Motion Group ──
    auto motionGroup = std::make_unique<juce::AudioProcessorParameterGroup> (
        "motion", "Motion", "|");

    // v1.1.0 appends "Ping-Pong" (C3) — append-only, never reorder: APVTS
    // sessions persist the choice INDEX, so 0-3 keep their meaning.
    motionGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "path", 1 }, "Path",
        juce::StringArray { "Orbit", "Pendulum", "Linear", "Drift", "Ping-Pong" }, 0));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "speed", 1 }, "Speed",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.5f), 1.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "width", 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 360.0f), 180.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tilt", 1 }, "Tilt",
        juce::NormalisableRange<float> (-90.0f, 90.0f), 0.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phase", 1 }, "Phase",
        juce::NormalisableRange<float> (0.0f, 360.0f), 0.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "elevation_enable", 1 }, "Elevation", false));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "elevation_range", 1 }, "Elev Range",
        juce::NormalisableRange<float> (0.0f, 90.0f), 45.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tempo_sync", 1 }, "Tempo Sync",
        juce::StringArray { "Off", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
                            "1/4T", "1/4", "1/4D", "1/2", "1/2D", "1 Bar", "2 Bars", "4 Bars" },
        0));

    layout.add (std::move (motionGroup));

    // ── Spatial Group ──
    auto spatialGroup = std::make_unique<juce::AudioProcessorParameterGroup> (
        "spatial", "Spatial", "|");

    spatialGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "speaker_layout", 1 }, "Speaker Layout",
        juce::StringArray { "Stereo", "Quad", "5.1", "7.1", "5.1.4", "7.1.4", "Hexaphonic", "Octaphonic" },
        0));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.1f, 30.0f, 0.0f, 0.5f), 1.0f));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "air_absorption", 1 }, "Air Absorption",
        juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "attenuation_curve", 1 }, "Atten Curve",
        juce::StringArray { "Linear", "Inverse", "Inverse Square" }, 1));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "center_diverge", 1 }, "Center Diverge",
        juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));

    layout.add (std::move (spatialGroup));

    // ── Mix Group ──
    auto mixGroup = std::make_unique<juce::AudioProcessorParameterGroup> (
        "mix", "Mix", "|");

    mixGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "source_mode", 1 }, "Source Mode",
        juce::StringArray { "Mono", "L+R Split" }, 0));

    mixGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lr_offset", 1 }, "L/R Offset",
        juce::NormalisableRange<float> (0.0f, 360.0f), 180.0f));

    mixGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));

    layout.add (std::move (mixGroup));

    return layout;
}

const juce::String OOrbitProcessor::getName() const           { return JucePlugin_Name; }
bool OOrbitProcessor::acceptsMidi() const                     { return false; }
bool OOrbitProcessor::producesMidi() const                    { return false; }
bool OOrbitProcessor::isMidiEffect() const                    { return false; }
double OOrbitProcessor::getTailLengthSeconds() const          { return 0.0; }

const std::vector<OOrbitProcessor::FactoryPreset>& OOrbitProcessor::getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets =
    {
        // ── Stereo Presets ──
        { "Slow Orbit", {
            { "path", 0.0f }, { "speed", 0.5f }, { "width", 180.0f }, { "depth", 0.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 1.0f },
            { "air_absorption", 30.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Fast Spiral", {
            { "path", 0.0f }, { "speed", 4.0f }, { "width", 360.0f }, { "depth", 50.0f },
            { "tilt", 15.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 2.0f },
            { "air_absorption", 40.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Pendulum Swing", {
            { "path", 1.0f }, { "speed", 1.0f }, { "width", 120.0f }, { "depth", 0.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 1.0f },
            { "air_absorption", 20.0f }, { "attenuation_curve", 0.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Ambient Drift", {
            { "path", 3.0f }, { "speed", 0.3f }, { "width", 90.0f }, { "depth", 30.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 3.0f },
            { "air_absorption", 60.0f }, { "attenuation_curve", 2.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 80.0f }
        }},
        { "Tempo Quarter", {
            { "path", 0.0f }, { "speed", 1.0f }, { "width", 180.0f }, { "depth", 0.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 8.0f }, { "speaker_layout", 0.0f }, { "distance", 1.0f },
            { "air_absorption", 25.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},

        // ── Surround Presets ──
        { "5.1 Orbit", {
            { "path", 0.0f }, { "speed", 0.8f }, { "width", 360.0f }, { "depth", 20.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 2.0f }, { "distance", 2.0f },
            { "air_absorption", 35.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 20.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "7.1.4 Height Sweep", {
            { "path", 2.0f }, { "speed", 0.3f }, { "width", 180.0f }, { "depth", 40.0f },
            { "tilt", 45.0f }, { "phase", 0.0f }, { "elevation_enable", 1.0f }, { "elevation_range", 60.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 5.0f }, { "distance", 3.0f },
            { "air_absorption", 50.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 10.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Quad Drift", {
            { "path", 3.0f }, { "speed", 0.5f }, { "width", 120.0f }, { "depth", 25.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 1.0f }, { "distance", 2.0f },
            { "air_absorption", 40.0f }, { "attenuation_curve", 2.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 90.0f }
        }},

        // ── Creative Presets ──
        { "L+R Split Wide", {
            { "path", 0.0f }, { "speed", 0.8f }, { "width", 360.0f }, { "depth", 0.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 1.5f },
            { "air_absorption", 30.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 0.0f },
            { "source_mode", 1.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Deep Space", {
            { "path", 0.0f }, { "speed", 0.15f }, { "width", 360.0f }, { "depth", 80.0f },
            { "tilt", 10.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 20.0f },
            { "air_absorption", 90.0f }, { "attenuation_curve", 2.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Tight Focus", {
            { "path", 0.0f }, { "speed", 1.5f }, { "width", 30.0f }, { "depth", 10.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 0.0f }, { "speaker_layout", 0.0f }, { "distance", 0.5f },
            { "air_absorption", 10.0f }, { "attenuation_curve", 0.0f }, { "center_diverge", 50.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }},
        { "Rhythmic Bounce", {
            { "path", 1.0f }, { "speed", 2.0f }, { "width", 150.0f }, { "depth", 30.0f },
            { "tilt", 0.0f }, { "phase", 0.0f }, { "elevation_enable", 0.0f }, { "elevation_range", 45.0f },
            { "tempo_sync", 5.0f }, { "speaker_layout", 0.0f }, { "distance", 1.5f },
            { "air_absorption", 25.0f }, { "attenuation_curve", 1.0f }, { "center_diverge", 0.0f },
            { "source_mode", 0.0f }, { "lr_offset", 180.0f }, { "mix", 100.0f }
        }}
    };

    return presets;
}

// v1.1.0: the legacy Programs API is collapsed to a single program — the 12
// factory presets now live in the preset-manager bank (categorized menu,
// prev/next, user saves) like the rest of the suite.
int OOrbitProcessor::getNumPrograms()                          { return 1; }
int OOrbitProcessor::getCurrentProgram()                       { return 0; }
void OOrbitProcessor::setCurrentProgram (int)                  {}
const juce::String OOrbitProcessor::getProgramName (int)       { return "Default"; }
void OOrbitProcessor::changeProgramName (int, const juce::String&) {}
bool OOrbitProcessor::hasEditor() const                       { return true; }

juce::AudioProcessorEditor* OOrbitProcessor::createEditor()
{
    return new OOrbitEditor (*this);
}

bool OOrbitProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto mainInput  = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput.isDisabled() || mainOutput.isDisabled())
        return false;

    // Input: mono or stereo
    if (mainInput != juce::AudioChannelSet::mono()
        && mainInput != juce::AudioChannelSet::stereo())
        return false;

    // Output: 2-24 channels
    int numOut = mainOutput.size();
    return numOut >= 2 && numOut <= 24;
}

float OOrbitProcessor::shortestArc (float from, float to)
{
    float diff = std::fmod (to - from + 540.0f, 360.0f) - 180.0f;
    return from + diff;
}

float OOrbitProcessor::wrapAngle (float angle)
{
    float result = std::fmod (angle + 180.0f, 360.0f);
    if (result < 0.0f) result += 360.0f;
    return result - 180.0f;
}

void OOrbitProcessor::handleAsyncUpdate()
{
    // Called on message thread when speaker layout changes via preset dropdown
    if (useCustomLayout)
        return;  // Custom layout takes precedence — don't overwrite with preset

    int layoutIndex = static_cast<int> (speakerLayoutParam->load());
    currentLayout = SpeakerPresets::getPreset (layoutIndex);
    lastSpeakerLayoutIndex = layoutIndex;
    applyLayout (currentLayout);
}

void OOrbitProcessor::applyLayout (const SpeakerLayout& layout)
{
    // Queue layout for audio thread to consume (avoids data races on vbapRenderer/downmixEngine)
    {
        const juce::SpinLock::ScopedLockType lock (pendingLayoutLock);
        pendingLayout = layout;
    }
    layoutPending.store (true, std::memory_order_release);

    // VBAP background thread recomputation is safe (uses its own lock-free exchange)
    if (layout.getChannelCount() >= 4)
        vbapThread.requestRecomputation (layout);
}

void OOrbitProcessor::applyLayoutOnAudioThread (const SpeakerLayout& layout)
{
    currentLayout = layout;
    layoutNumSpeakers = currentLayout.getChannelCount();
    vbapRenderer.prepare (currentLayout);
    downmixEngine.prepare (currentLayout, getTotalNumOutputChannels());
}

void OOrbitProcessor::setCustomSpeakerLayout (const SpeakerLayout& layout)
{
    useCustomLayout = true;
    customLayout = layout;
    applyLayout (customLayout);
}

void OOrbitProcessor::addSpeakerToLayout (float azimuth, float elevation, float distance, const juce::String& label)
{
    if (! useCustomLayout)
    {
        customLayout = currentLayout;
        useCustomLayout = true;
    }
    customLayout.speakers.push_back ({ azimuth, elevation, distance, label, false });
    applyLayout (customLayout);
}

void OOrbitProcessor::removeSpeakerFromLayout (int index)
{
    if (! useCustomLayout)
    {
        customLayout = currentLayout;
        useCustomLayout = true;
    }
    if (index >= 0 && index < (int) customLayout.speakers.size() && customLayout.speakers.size() > 2)
    {
        customLayout.speakers.erase (customLayout.speakers.begin() + index);
        applyLayout (customLayout);
    }
}

void OOrbitProcessor::moveSpeakerInLayout (int index, float azimuth, float elevation, float distance)
{
    if (! useCustomLayout)
    {
        customLayout = currentLayout;
        useCustomLayout = true;
    }
    if (index >= 0 && index < (int) customLayout.speakers.size())
    {
        auto& spk = customLayout.speakers[(size_t) index];
        spk.azimuth   = azimuth;
        spk.elevation = juce::jlimit (-90.0f, 90.0f, elevation);
        spk.distance  = juce::jlimit (0.1f, 30.0f, distance);
        customLayout.is3D = std::any_of (customLayout.speakers.begin(), customLayout.speakers.end(),
                                         [] (const Speaker& s) { return s.elevation != 0.0f; });
        applyLayout (customLayout);
    }
}

juce::File OOrbitProcessor::getLayoutsDirectory() const
{
    // Sibling of the preset store: ~/Library/Ouaricon Orbit/Layouts/
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
        .getChildFile ("Library")
        .getChildFile ("Ouaricon Orbit")
        .getChildFile ("Layouts");
}

void OOrbitProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setLatencySamples (0);

    motionEngine.prepare (sampleRate);
    distanceModel.prepare (sampleRate);
    distanceModelR.prepare (sampleRate);

    // Initialize speaker layout
    if (! useCustomLayout)
    {
        int layoutIndex = static_cast<int> (speakerLayoutParam->load());
        currentLayout = SpeakerPresets::getPreset (layoutIndex);
        lastSpeakerLayoutIndex = layoutIndex;
    }
    layoutNumSpeakers = currentLayout.getChannelCount();
    vbapRenderer.prepare (currentLayout);

    // Start VBAP compute thread and trigger initial gain table generation
    if (! vbapThread.isThreadRunning())
        vbapThread.startThread (juce::Thread::Priority::normal);
    if (layoutNumSpeakers >= 4)
        vbapThread.requestRecomputation (currentLayout);

    // Initialize auto-downmix engine
    downmixEngine.prepare (currentLayout, getTotalNumOutputChannels());

    // Allocate buffers (spatialBuffer sized to max 24 speakers to avoid runtime reallocation)
    dryBuffer.setSize (2, samplesPerBlock);
    spatialBuffer.setSize (24, samplesPerBlock);

    // Smoothed values: 20ms ramp, snapped to the current param so playback
    // doesn't start with a fade-in from zero
    mixSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (mixParam->load() / 100.0f);

    // Reset gain smoothing
    previousGains.fill (0.0f);
    currentGains.fill (0.0f);
    previousGainsR.fill (0.0f);
    currentGainsR.fill (0.0f);

    // Clear any pending layout (we just applied the current one)
    layoutPending.store (false, std::memory_order_relaxed);
}

void OOrbitProcessor::releaseResources() {}

void OOrbitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    if (numSamples == 0)
        return;

    // Apply any pending layout change from message thread
    if (layoutPending.load (std::memory_order_acquire))
    {
        SpeakerLayout newLayout;
        {
            const juce::SpinLock::ScopedLockType lock (pendingLayoutLock);
            newLayout = pendingLayout;
        }
        layoutPending.store (false, std::memory_order_release);
        applyLayoutOnAudioThread (newLayout);
    }

    // Update VBAP gain table from background thread (lock-free try)
    vbapExchange.updateAudioThreadData();
    if (auto* vbapData = vbapExchange.getActiveData())
    {
        vbapRenderer.setExternalGainTable (
            vbapData->gainTable.data(),
            (int) vbapData->gainTable.size(),
            vbapData->numOutputChannels,
            vbapData->aziRes,
            vbapData->elevRes,
            vbapData->is3D,
            vbapData->speakerToChannelMap.data(),
            vbapData->numVBAPSpeakers);
    }

    // Check for speaker layout change
    int layoutIndex = static_cast<int> (speakerLayoutParam->load());
    if (layoutIndex != lastSpeakerLayoutIndex)
        triggerAsyncUpdate();

    // Read parameters
    int pathIdx           = static_cast<int> (pathParam->load());
    float speedVal        = speedParam->load();
    float widthVal        = widthParam->load();
    float depthVal        = depthParam->load();
    float tiltVal         = tiltParam->load();
    float phaseVal        = phaseParam->load();
    bool elevEnabled      = elevEnableParam->load() > 0.5f;
    float elevRange       = elevRangeParam->load();
    int tempoSyncIdx      = static_cast<int> (tempoSyncParam->load());
    float dist            = distanceParam->load();
    float airAbs          = airAbsorptionParam->load();
    int attenCurve        = static_cast<int> (attenCurveParam->load());
    float diverge         = centerDivergeParam->load() / 100.0f;
    int sourceMode        = static_cast<int> (sourceModeParam->load());
    float lrOffset        = lrOffsetParam->load();
    float mixVal          = mixParam->load() / 100.0f;

    // Read host BPM and beat position (C1: PPQ lock while the transport rolls)
    double hostBpm = 120.0;
    double hostPpq = 0.0;
    bool   ppqValid = false;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;

            if (auto ppq = pos->getPpqPosition())
            {
                hostPpq = *ppq;
                ppqValid = pos->getIsPlaying();
            }
        }
    }

    // Update motion engine parameters
    motionEngine.setPath (pathIdx);
    motionEngine.setSpeed (speedVal);
    motionEngine.setWidth (widthVal);
    motionEngine.setDepth (depthVal);
    motionEngine.setTilt (tiltVal);
    motionEngine.setPhase (phaseVal);
    motionEngine.setElevationEnabled (elevEnabled);
    motionEngine.setElevationRange (elevRange);
    motionEngine.setTempoSync (tempoSyncIdx);
    motionEngine.setHostBpm (hostBpm);
    motionEngine.setHostPpq (hostPpq, ppqValid);

    // Update VBAP renderer
    vbapRenderer.setCenterDiverge (diverge);

    // Copy input to dry buffer for dry/wet mix
    dryBuffer.setSize (totalNumInputChannels, numSamples, false, false, true);
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // Advance motion engine and get current state for this block
    motionEngine.advance (numSamples);
    MotionState endState = motionEngine.getCurrentState();

    // Update distance model: the Distance param scales the motion engine's
    // distance (a Depth-driven multiplier around 1.0), so Depth produces
    // audible near/far motion
    float effectiveDist = std::max (0.1f, dist * endState.distance);
    distanceModel.updateDistance (effectiveDist, airAbs, attenCurve, numSamples);
    distanceModelR.updateDistance (effectiveDist, airAbs, attenCurve, numSamples);

    // Save previous gains for smoothing, compute new block gains
    previousGains = currentGains;
    previousGainsR = currentGainsR;

    // Compute VBAP gains at end position for this block
    currentGains.fill (0.0f);
    vbapRenderer.computeGains (endState.azimuth, endState.elevation,
                               currentGains.data(), layoutNumSpeakers);

    if (sourceMode == 1) // L+R Split
    {
        float rAz = wrapAngle (endState.azimuth + lrOffset);
        currentGainsR.fill (0.0f);
        vbapRenderer.computeGains (rAz, endState.elevation,
                                   currentGainsR.data(), layoutNumSpeakers);
    }

    // Clear spatial buffer (clamped to buffer capacity — resizing done in applyLayout)
    int spatialChannels = std::min (layoutNumSpeakers, (int) spatialBuffer.getNumChannels());
    for (int ch = 0; ch < spatialChannels; ++ch)
        spatialBuffer.clear (ch, 0, numSamples);

    // Per-sample processing
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float t = (float) sample / (float) numSamples;

        // Interpolated VBAP gains
        float gains[24] = {};
        for (int s = 0; s < spatialChannels; ++s)
            gains[s] = previousGains[(size_t) s] + t * (currentGains[(size_t) s] - previousGains[(size_t) s]);

        if (sourceMode == 0) // Mono
        {
            // Sum input to mono
            float inputL = (totalNumInputChannels > 0) ? buffer.getSample (0, sample) : 0.0f;
            float inputR = (totalNumInputChannels > 1) ? buffer.getSample (1, sample) : inputL;
            float mono = (inputL + inputR) * 0.5f;

            // Apply distance model
            float processed = distanceModel.processSample (mono);

            // Distribute to speakers via VBAP gains
            for (int s = 0; s < spatialChannels; ++s)
                spatialBuffer.addSample (s, sample, processed * gains[s]);
        }
        else // L+R Split
        {
            float inputL = (totalNumInputChannels > 0) ? buffer.getSample (0, sample) : 0.0f;
            float inputR = (totalNumInputChannels > 1) ? buffer.getSample (1, sample) : inputL;

            // L source at current position
            float processedL = distanceModel.processSample (inputL);

            // R source gains (interpolated)
            float gainsR[24] = {};
            for (int s = 0; s < spatialChannels; ++s)
                gainsR[s] = previousGainsR[(size_t) s] + t * (currentGainsR[(size_t) s] - previousGainsR[(size_t) s]);

            float processedR = distanceModelR.processSample (inputR);

            // Accumulate both sources
            for (int s = 0; s < spatialChannels; ++s)
            {
                spatialBuffer.addSample (s, sample, processedL * gains[s]);
                spatialBuffer.addSample (s, sample, processedR * gainsR[s]);
            }
        }
    }

    // Copy spatial buffer to output, with auto-downmix if needed
    int outChannels = totalNumOutputChannels;
    buffer.clear();

    if (outChannels >= spatialChannels)
    {
        // Direct mapping: copy spatial channels to output
        for (int ch = 0; ch < spatialChannels; ++ch)
            buffer.copyFrom (ch, 0, spatialBuffer, ch, 0, numSamples);
    }
    else if (downmixEngine.isActive())
    {
        // Use DownmixEngine for energy-preserving fold-down
        const float* srcPtrs[24] = {};
        float* destPtrs[24] = {};
        for (int ch = 0; ch < spatialChannels && ch < 24; ++ch)
            srcPtrs[ch] = spatialBuffer.getReadPointer (ch);
        for (int ch = 0; ch < outChannels && ch < 24; ++ch)
            destPtrs[ch] = buffer.getWritePointer (ch);

        downmixEngine.process (srcPtrs, destPtrs, numSamples);
    }
    else
    {
        // Fallback: copy what fits
        for (int ch = 0; ch < std::min (outChannels, spatialChannels); ++ch)
            buffer.copyFrom (ch, 0, spatialBuffer, ch, 0, numSamples);
    }

    // Apply dry/wet mix: wet scales on ALL output channels (keeps the spatial
    // balance constant as mix decreases), dry blends only into its native input
    // channels. Mix is smoothed per-sample to avoid zipper under automation.
    mixSmoothed.setTargetValue (mixVal);
    int dryChannels = std::min (totalNumOutputChannels, totalNumInputChannels);
    for (int s = 0; s < numSamples; ++s)
    {
        float m = mixSmoothed.getNextValue();
        float dryGain = 1.0f - m;

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float wet = m * buffer.getSample (ch, s);
            float dry = (ch < dryChannels) ? dryGain * dryBuffer.getSample (ch, s) : 0.0f;
            buffer.setSample (ch, s, wet + dry);
        }
    }

    // Update UI motion snapshot (relaxed atomics, read by editor timer)
    uiAzimuthL.store (endState.azimuth, std::memory_order_relaxed);
    uiElevationL.store (endState.elevation, std::memory_order_relaxed);
    uiDistance.store (effectiveDist, std::memory_order_relaxed);

    if (sourceMode == 1)
    {
        float rAz = wrapAngle (endState.azimuth + lrOffset);
        uiAzimuthR.store (rAz, std::memory_order_relaxed);
        uiElevationR.store (endState.elevation, std::memory_order_relaxed);
    }
    else
    {
        uiAzimuthR.store (endState.azimuth, std::memory_order_relaxed);
        uiElevationR.store (endState.elevation, std::memory_order_relaxed);
    }
}

void OOrbitProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // v1.1.0: hover-help preference and current preset name ride the same
    // tree as plain properties (not parameters).
    state.setProperty ("tooltipsEnabled",
                       tooltipsEnabled.load (std::memory_order_acquire), nullptr);
    state.setProperty ("currentPreset", presetManager.getCurrentPresetName(), nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    if (useCustomLayout)
    {
        auto* customXml = xml->createNewChildElement ("CustomLayout");
        customXml->setAttribute ("name", customLayout.name);
        customXml->setAttribute ("is3D", customLayout.is3D);

        for (const auto& spk : customLayout.speakers)
        {
            auto* spkXml = customXml->createNewChildElement ("Speaker");
            spkXml->setAttribute ("azimuth", spk.azimuth);
            spkXml->setAttribute ("elevation", spk.elevation);
            spkXml->setAttribute ("distance", spk.distance);
            spkXml->setAttribute ("label", spk.label);
            spkXml->setAttribute ("isLFE", spk.isLFE);
        }
    }

    copyXmlToBinary (*xml, destData);
}

void OOrbitProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
    {
        // Check for custom layout before restoring APVTS
        auto* customXml = xmlState->getChildByName ("CustomLayout");
        if (customXml != nullptr)
        {
            customLayout.name = customXml->getStringAttribute ("name", "Custom");
            customLayout.is3D = customXml->getBoolAttribute ("is3D", false);
            customLayout.speakers.clear();

            for (auto* spkXml : customXml->getChildIterator())
            {
                if (spkXml->hasTagName ("Speaker"))
                {
                    Speaker spk;
                    spk.azimuth   = (float) spkXml->getDoubleAttribute ("azimuth", 0.0);
                    spk.elevation = (float) spkXml->getDoubleAttribute ("elevation", 0.0);
                    spk.distance  = (float) spkXml->getDoubleAttribute ("distance", 1.0);
                    spk.label     = spkXml->getStringAttribute ("label", "?");
                    spk.isLFE     = spkXml->getBoolAttribute ("isLFE", false);
                    customLayout.speakers.push_back (spk);
                }
            }

            useCustomLayout = true;

            // Remove custom XML so APVTS doesn't see it
            xmlState->removeChildElement (customXml, true);
        }

        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));

        // v1.1.0: hover-help preference. A pre-1.1.0 session has no property —
        // getProperty returns a VOID var and the default (OFF) stands.
        // isVoid() is the only correct gate: the XML round-trip rebuilds the
        // property as a STRING var, so isBool()/isInt() are false for every
        // saved session (critical_valuetree_xml_roundtrip_loses_type). var's
        // bool conversion handles all forms once existence is known.
        const juce::var tips = parameters.state.getProperty ("tooltipsEnabled");
        if (! tips.isVoid())
            tooltipsEnabled.store ((bool) tips, std::memory_order_release);

        const juce::var presetName = parameters.state.getProperty ("currentPreset");
        if (! presetName.isVoid())
            presetManager.setCurrentPresetName (presetName.toString());

        if (useCustomLayout)
            applyLayout (customLayout);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OOrbitProcessor();
}
