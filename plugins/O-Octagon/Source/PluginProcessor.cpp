/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

//==============================================================================
namespace
{
    // Two-argument NormalisableRange<float>(min, max) yields interval 0, skew 1. The 4-argument
    // form with an explicit 1.0f skew is deliberately NOT used: it reads as though a skew were
    // intended and invites a future "fix". All 17 ranges are linear by design (parameter-spec.md).
    juce::NormalisableRange<float> linearRange (float lo, float hi)
    {
        return juce::NormalisableRange<float> (lo, hi);
    }

    std::unique_ptr<juce::AudioParameterFloat> makeFloat (const juce::String& id,
                                                          const juce::String& name,
                                                          juce::NormalisableRange<float> range,
                                                          float defaultValue,
                                                          const juce::String& label = {})
    {
        auto attributes = juce::AudioParameterFloatAttributes();

        if (label.isNotEmpty())
            attributes = attributes.withLabel (label);

        // The version hint (second ParameterID argument) is mandatory in JUCE 8.
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 },
                                                            name,
                                                            range,
                                                            defaultValue,
                                                            attributes);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OOctagonProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Five groups (PLAN P1). The headline gesture of this plugin is automating eight weights;
    // a flat 17-entry menu buries them. Group membership does not participate in parameter
    // identity, so this stays reversible if Stage 4 host testing disagrees.

    // ── Position ────────────────────────────────────────────────────────────────
    // srcX/srcY display NORMALISED 0.00–1.00 in the host lane and take no label. Metres are a
    // Stage 3.1 UI-side conversion: the metre readout depends on the live venue bounding box, and
    // a host value→text lambda is captured at construction and therefore cannot read a live venue
    // (pattern_webview_knob_readout_scaled_value). Do not add a this-capturing lambda here.
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "position", "Position", "|",
        makeFloat ("srcX",  "Source X", linearRange (0.0f, 1.0f), 0.5f),
        makeFloat ("srcY",  "Source Y", linearRange (0.0f, 1.0f), 0.5f),
        makeFloat ("srcZ",  "Source Z", linearRange (-2.0f, 8.0f), 0.0f, "m"),
        makeFloat ("width", "Width",    linearRange (0.0f, 6.0f), 0.0f, "m")));

    // ── Solve ───────────────────────────────────────────────────────────────────
    // "dB/2x" rather than "dB/doubling": Logic truncates the unit field hard and the prose form
    // does not survive it (RESEARCH §3.2).
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "solve", "Solve", "|",
        makeFloat ("rolloff", "Rolloff", linearRange (3.0f, 6.0f), 4.0f, "dB/2x"),
        makeFloat ("blur",    "Blur",    linearRange (0.0f, 1.0f), 0.10f)));

    // ── Weights ─────────────────────────────────────────────────────────────────
    // No label, deliberately: these are DBAP weights, not percentages of anything. Inventing
    // "norm" or "%" would misrepresent them.
    auto weights = std::make_unique<juce::AudioProcessorParameterGroup> ("weights", "Weights", "|");

    for (int i = 1; i <= 8; ++i)
        weights->addChild (makeFloat ("w" + juce::String (i),
                                      "Weight " + juce::String (i),
                                      linearRange (0.0f, 1.0f),
                                      1.0f));

    layout.add (std::move (weights));

    // ── Space ───────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "space", "Space", "|",
        makeFloat ("hullAtten", "Hull Atten", linearRange (0.0f, 3.0f), 1.0f, "dB/m"),
        makeFloat ("airAmount", "Air",        linearRange (0.0f, 1.0f), 0.35f)));

    // ── Output ──────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "output", "Output", "|",
        makeFloat ("outputGain", "Output", linearRange (-24.0f, 12.0f), 0.0f, "dB")));

    return layout;
}

//==============================================================================
OOctagonProcessor::OOctagonProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::mono(),          true)
                          .withOutput ("Output", juce::AudioChannelSet::create7point1(), true)),
      // State root is "OOctagon" — the architecture's identifier, NOT the sibling O-Orbit's
      // "OOrbitParams" idiom. Phase 2.1 attaches the VENUE child to this exact node; changing it
      // later orphans every saved session and every .venue file written in between.
      // THIS IDENTIFIER MUST NEVER CHANGE.
      apvts (*this, nullptr, juce::Identifier ("OOctagon"), createParameterLayout())
{
    srcXParam       = apvts.getRawParameterValue ("srcX");
    srcYParam       = apvts.getRawParameterValue ("srcY");
    srcZParam       = apvts.getRawParameterValue ("srcZ");
    widthParam      = apvts.getRawParameterValue ("width");
    rolloffParam    = apvts.getRawParameterValue ("rolloff");
    blurParam       = apvts.getRawParameterValue ("blur");

    for (int i = 0; i < 8; ++i)
        weightParam[i] = apvts.getRawParameterValue ("w" + juce::String (i + 1));

    hullAttenParam  = apvts.getRawParameterValue ("hullAtten");
    airAmountParam  = apvts.getRawParameterValue ("airAmount");
    outputGainParam = apvts.getRawParameterValue ("outputGain");
}

OOctagonProcessor::~OOctagonProcessor() = default;

//==============================================================================
bool OOctagonProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This function is load-bearing well beyond its length: JUCE DERIVES the entire AU
    // channel-config set from it (RESEARCH F2) —
    //     AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}
    // and auval exercises all six. Widening or narrowing this predicate silently changes what
    // auval tests and what Logic offers.

    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;

    // Real mode — the three 8-channel containers Logic exposes.
    if (out == juce::AudioChannelSet::create7point1())       return true;   // primary
    if (out == juce::AudioChannelSet::create7point1SDDS())   return true;   // Logic/Emagic order
    if (out == juce::AudioChannelSet::create5point1point2()) return true;   // third 8-ch container

    // SAFE mode — defined, non-destructive, clearly signposted. Load-bearing for AU (the (n,1) and
    // (n,2) configs above), not only for Standalone on a 2-channel interface (COMPAT-04).
    if (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        return true;

    // Everything else is rejected — including octagonal(), which JUCE offers as an 8-channel
    // candidate and which Logic ignores.
    return false;
}

//==============================================================================
void OOctagonProcessor::prepareToPlay (double, int)
{
    // Nothing to prepare at Stage 1: no channel map, no filters, no smoothers, no control grid.
    // Parameter names are omitted (rather than cast to void) to satisfy -Wunused-parameter.
    //
    // NEVER call setLatencySamples() here — latency is zero and getLatencySamples() is
    // non-virtual in JUCE 8.
}

void OOctagonProcessor::releaseResources()
{
}

void OOctagonProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // ── PHASE-2.2-REPLACE: ──────────────────────────────────────────────────────────────
    // Stage 1 placeholder. Writes the mono-summed dry input to EVERY output channel at unity,
    // by RAW BUFFER INDEX, so that a successful 8-channel negotiation is visible on Logic's
    // surround meters (FUNC-01/COMPAT-01).
    //
    // This is the ONLY hardcoded output index in the plugin and it is deleted wholesale by the
    // GainStage inner loop at Phase 2.2. It must acquire no dependants. Phase 2.1's "zero
    // hardcoded output indices outside ChannelMap" gate is EXPECTED to fail against this block —
    // retire it, do not grandfather it.
    // ────────────────────────────────────────────────────────────────────────────────────

    const int numSamples = buffer.getNumSamples();

    // Bound by buffer.getNumChannels(), NOT 8 and NOT getTotalNumOutputChannels(). On a 3–7
    // output device canonicalChannelSet(n) is rejected, Debug asserts, and Release KEEPS the 7.1
    // layout while the buffer holds only n channels (RESEARCH F3). getTotalNumOutputChannels()
    // returns 8 in exactly that state — it is the accessor that lies. A loop bounded by 8 would
    // write past the end of the buffer.
    const int numOut = buffer.getNumChannels();
    const int numIn  = juce::jmin (getTotalNumInputChannels(), numOut);

    for (int n = 0; n < numSamples; ++n)
    {
        // Read BEFORE writing: out[0] and in[0] alias the same memory. Writing all outputs
        // channel-major would destroy in[1] before it is summed. Sample-interleaved read-then-
        // write avoids needing a scratch buffer for a block that gets deleted.
        //
        // numIn == 0 yields silence rather than a read of channel 0 — defensive, costs one branch.
        // Correct at 1 and 2 output channels, not only 8: auval exercises all six AU configs.
        const float s = numIn >= 2 ? 0.5f * (buffer.getSample (0, n) + buffer.getSample (1, n))
                      : numIn == 1 ? buffer.getSample (0, n)
                                   : 0.0f;

        for (int ch = 0; ch < numOut; ++ch)
            buffer.setSample (ch, n, s);
    }
}

//==============================================================================
bool OOctagonProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OOctagonProcessor::createEditor()
{
    // No PluginEditor.{h,cpp} at Stage 1. The generic editor renders all 17 parameters with their
    // names, ranges, defaults and units — which IS the Stage 1 exit criterion — and gives the
    // Standalone build a non-empty window for the COMPAT-04 eyeball check. It is deleted at
    // Phase 3.1; nothing may come to depend on it.
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
const juce::String OOctagonProcessor::getName() const       { return "O-Octagon"; }

bool OOctagonProcessor::acceptsMidi() const                 { return false; }
bool OOctagonProcessor::producesMidi() const                { return false; }
bool OOctagonProcessor::isMidiEffect() const                { return false; }
double OOctagonProcessor::getTailLengthSeconds() const      { return 0.0; }

int OOctagonProcessor::getNumPrograms()                     { return 1; }
int OOctagonProcessor::getCurrentProgram()                  { return 0; }
void OOctagonProcessor::setCurrentProgram (int)             {}
const juce::String OOctagonProcessor::getProgramName (int)  { return {}; }
void OOctagonProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void OOctagonProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // This code is written once and does NOT change when Phase 2.1 adds the VENUE child — the
    // child rides along inside copyState() automatically. That is the whole reason it is written
    // now rather than at 2.1.
    auto state = apvts.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OOctagonProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

    // ── Forward-compatibility notes for Phase 2.1, recorded here so 2.1 need not rediscover them:
    //
    // 1. Stage 1 sessions carry NO VENUE child. Every session saved between now and Phase 2.1
    //    restores a tree holding 17 parameters and nothing else. readVenueFromState() must
    //    therefore treat a missing OR PARTIAL VENUE node as "use defaults" — not as an error —
    //    and must not assume 8 SPEAKER children exist.
    //
    // 2. Phase 2.1 calls readVenueFromState() AFTER replaceState(), and rebuildChannelMap()
    //    after both.
    //
    // No AsyncUpdater, no deferral and no guard flag at Stage 1 — there is nothing to defer.
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OOctagonProcessor();
}
