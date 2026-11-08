/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

// Stage 1 (D8): the GenericAudioProcessorEditor on every target. Phase 3.1
// swaps the WebView editor into the JUCE_WEB_BROWSER branch of createEditor()
// and includes its header under the same guard (consoles build with
// JUCE_WEB_BROWSER=0 and must never see WebView types).

namespace Ids = GiParamIds;

//==============================================================================
juce::AudioProcessor::BusesProperties GranulatorInstrumentProcessor::makeBuses()
{
    // D9: the Record-in sidechain is the only input bus (VST3 kAux through
    // GiVst3Extensions). It never reaches the output.
    return BusesProperties()
        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), OGI_SC_DEFAULT_ACTIVE != 0)
        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true);
}

GranulatorInstrumentProcessor::GranulatorInstrumentProcessor()
    : AudioProcessor (makeBuses()),
      parameters (*this, nullptr, juce::Identifier ("OGranulatorInstrument"), createParameterLayout())
{
    // Per-instance seed; a restored session overrides it (setStateInformation).
    seed.store (static_cast<uint64_t> (juce::Random::getSystemRandom().nextInt64()), std::memory_order_relaxed);

    // builtIns() is a function-local static: build it here, never first on the audio thread.
    (void) PitchCollections::builtIns();

    updateSidechainChannelCount();
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GranulatorInstrumentProcessor::createParameterLayout()
{
    using Kind = Ids::Kind;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& e : Ids::registrationOrder())
    {
        const auto& d = *e.def;
        const juce::ParameterID pid { e.id, 1 };

        switch (d.kind)
        {
            case Kind::Float:
            {
                // start / end / interval / skew only; setSkewForCentre for the
                // log-like rows. Interval is 1 on the 6 contract rows, 0 elsewhere.
                juce::NormalisableRange<float> range (d.lo, d.hi, d.interval, d.skew);
                if (d.centre > 0.0f)
                    range.setSkewForCentre (d.centre);
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, e.name, range, d.def));
                break;
            }
            case Kind::Choice:
            {
                const auto& choices = d.choices != nullptr ? *d.choices : Ids::modDestChoices();
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, e.name, choices, juce::roundToInt (d.def)));
                break;
            }
            case Kind::Bool:
            {
                // s1_enable defaults On; every other Bool Off.
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, e.name, Ids::boolDefaultFor (e)));
                break;
            }
        }
    }
    return layout;
}

//==============================================================================
void GranulatorInstrumentProcessor::updateSidechainChannelCount() noexcept
{
    const auto* sc = getBus (true, 0);
    sidechainChannels.store (sc != nullptr && sc->isEnabled() ? sc->getNumberOfChannels() : 0,
                             std::memory_order_relaxed);
}

void GranulatorInstrumentProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    // JUCE 8: getLatencySamples() is non-virtual; report through the setter, here only.
    setLatencySamples (0);

    // From the NEGOTIATED layout, never from the buffer width (a host may hand
    // a 2-channel buffer to any layout).
    updateSidechainChannelCount();
}

bool GranulatorInstrumentProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // AudioChannelSet is a bitset: compare sets with ==, never by size().
    using CS = juce::AudioChannelSet;
    if (layouts.getMainOutputChannelSet() != CS::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    return in == CS::disabled() || in == CS::mono() || in == CS::stereo();
}

void GranulatorInstrumentProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Keep the note-expression raw-event queue cycling (a no-op outside VST3).
    vst3Extensions.drainAndUpdate();

    // MIDI is consumed and not acted on in Stage 1 (producesMidi() == false).
    midiMessages.clear();

    // Silent shell: clear EVERY channel the buffer carries (outputs and
    // input-only channels alike). No parameter and no sidechain sample is read.
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);
}

juce::AudioProcessorEditor* GranulatorInstrumentProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    // Phase 3.1: the WebView editor replaces this line.
    return new juce::GenericAudioProcessorEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // headless consoles
#endif
}

//==============================================================================
// State codec
//
//   <OGranulatorInstrument uiLanguage="fr" ...PARAM children...>
//     <Engine seed="-1234" stateVersion="1" modDestTableVersion="1"/>
//     <Slots/>                     (Slot children: Phase 2.1, keyed by "index")
//   </OGranulatorInstrument>
//
// - Written onto the copyState() COPY; the live tree is never mutated from a host thread.
// - No `id` attribute on any custom child: APVTS treats a top-level child's
//   `id` as a parameter ID.
// - seed as an int64 STRING (getLargeIntValue clamps a uint64 above INT64_MAX).
// - Every property comes back from XML as a string var: isVoid() guards, then
//   parse; never isBool().
// - CustomScale / DegreeMasks are reserved in the strip list for Phase 2.2.
//==============================================================================
namespace
{
    const juce::Identifier kEngineTag ("Engine"), kSlotsTag ("Slots"),
                           kCustomScaleTag ("CustomScale"), kDegreeMasksTag ("DegreeMasks");
    const juce::Identifier kUiLanguageAttr ("uiLanguage");

    void removeCustomChildren (juce::ValueTree& tree)
    {
        for (int i = tree.getNumChildren(); --i >= 0;)
        {
            const auto type = tree.getChild (i).getType();
            if (type == kEngineTag || type == kSlotsTag || type == kCustomScaleTag || type == kDegreeMasksTag)
                tree.removeChild (i, nullptr);
        }
    }
}

void GranulatorInstrumentProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    removeCustomChildren (state);   // never duplicate children restored into the live tree

    state.setProperty (kUiLanguageAttr, languageCode (uiLanguage.load (std::memory_order_relaxed)), nullptr);

    juce::ValueTree eng (kEngineTag);
    eng.setProperty ("seed", juce::String (static_cast<juce::int64> (getSeed())), nullptr);
    eng.setProperty ("stateVersion", Ids::kStateVersion, nullptr);
    eng.setProperty ("modDestTableVersion", Ids::kModDestTableVersion, nullptr);
    state.appendChild (eng, nullptr);

    state.appendChild (juce::ValueTree (kSlotsTag), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GranulatorInstrumentProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;   // a rejected blob changes nothing

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid())
        return;

    // Read the non-parameter state BEFORE stripping it from the tree.
    const juce::ValueTree eng = tree.getChildWithName (kEngineTag);
    const juce::var lang = tree.getProperty (kUiLanguageAttr);
    // <Slots>: Phase 2.1 parses its <Slot index> children and enqueues loader jobs.

    removeCustomChildren (tree);
    tree.removeProperty (kUiLanguageAttr, nullptr);
    parameters.replaceState (tree);

    // Non-parameter state back to defaults: Stage 1 holds none besides the
    // seed, which is kept (there is no default seed) unless the blob has one.

    if (! lang.isVoid())
    {
        const auto s = lang.toString().trim();
        uiLanguage.store (s.isNotEmpty() && s.containsOnly ("0123456789") ? juce::jlimit (0, 2, s.getIntValue())
                                                                          : languageIndex (s),
                          std::memory_order_relaxed);
    }

    if (eng.isValid())
    {
        // stateVersion / modDestTableVersion: 1 is the only version so far.
        const juce::var seedVar = eng.getProperty ("seed");
        if (! seedVar.isVoid())
        {
            const auto s = seedVar.toString().trim();
            if (s.isNotEmpty() && s.containsOnly ("-0123456789"))
                setSeed (static_cast<uint64_t> (s.getLargeIntValue()));
        }
    }
}

//==============================================================================
#if OGI_TEST_HOOKS
bool GranulatorInstrumentProcessor::loadSlotSync (int slot, const juce::File& file, juce::String& error)
{
    juce::ignoreUnused (slot, file);
    error = "loadSlotSync: stub until 2.1";
    return false;
}

bool GranulatorInstrumentProcessor::loadSlotSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                                                  const juce::String& name, juce::String& error)
{
    juce::ignoreUnused (slot, pcm, sourceRate, name);
    error = "loadSlotSync: stub until 2.1";
    return false;
}
#endif

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranulatorInstrumentProcessor();
}
