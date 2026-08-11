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

    for (int i = 1; i <= ochan::kNumSpeakers; ++i)
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

    // ochan::kNumSpeakers rather than a literal 8: this is a speaker-count loop, and the COMPAT-03
    // gate greps for bare channel-count literals. Naming it costs nothing and removes the need for
    // a reader (or the gate) to decide whether this particular 8 is a channel index.
    for (int i = 0; i < ochan::kNumSpeakers; ++i)
        weightParam[i] = apvts.getRawParameterValue ("w" + juce::String (i + 1));

    hullAttenParam  = apvts.getRawParameterValue ("hullAtten");
    airAmountParam  = apvts.getRawParameterValue ("airAmount");
    outputGainParam = apvts.getRawParameterValue ("outputGain");

    // Give apvts.state a VENUE child from birth, on the message thread, so that:
    //   - a session saved before prepareToPlay() still carries a complete room;
    //   - readVenueFromState() and writeToState() are never the ones mutating the ValueTree from
    //     whatever thread a host chooses to call prepareToPlay() on.
    // The venue is already at its §OQ4 defaults — VenueModel's constructor put it there.
    venue.writeToState (apvts.state);

    hull.build (venue.speakerPositions());

    publishSnapshot();
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
    // Parameter names are omitted (rather than cast to void) to satisfy -Wunused-parameter.
    //
    // NEVER call setLatencySamples() here — latency is zero and getLatencySamples() is
    // non-virtual in JUCE 8.
    //
    // Order is fixed: the venue defines the labels, and the map is built from the labels against
    // the layout the host has just negotiated.
    readVenueFromState();
    rebuildChannelMap();

    preparedYet = true;
}

void OOctagonProcessor::releaseResources()
{
}

//==============================================================================
bool OOctagonProcessor::mappedOutputAvailable (int numOutputChannels) const noexcept
{
    return numOutputChannels == ochan::kNumSpeakers
        && ! mapInvalid.load (std::memory_order_acquire);
}

//==============================================================================
void OOctagonProcessor::rebuildChannelMap()
{
    const auto outSet = getBusesLayout().getMainOutputChannelSet();

   #if JUCE_DEBUG
    // Layer 1 of the three-layer strategy, called here as well as from the unit target so that a
    // developer who never builds the tests still trips it on the first prepareToPlay().
    {
        juce::String whyNot;

        if (! ochan::verifyEnumBitOrder (outSet, &whyNot))
        {
            DBG ("O-Octagon channel-map Layer 1 FAILED: " << whyNot);
            jassertfalse;
        }
    }
   #endif

    // buildSpeakerToBuffer() leaves speakerToBuffer untouched unless it fully succeeds, so the
    // failure path below really does retain the LAST VALID map rather than half of a rejected one.
    const bool ok = ochan::buildSpeakerToBuffer (outSet, venue.labelTypes(), speakerToBuffer);

    mapInvalid.store (! ok, std::memory_order_release);

    // Published either way: on failure the snapshot carries the retained map, and the audio thread
    // is stopped from using it by mappedOutputAvailable(), not by stale data.
    publishSnapshot();
}

void OOctagonProcessor::readVenueFromState()
{
    // A missing OR partial VENUE node yields the §OQ4 defaults per attribute — never zeros, never
    // an error. Every session saved during Stage 1 takes exactly that path.
    venue.readFromState (apvts.state);

    hull.build (venue.speakerPositions());

    publishSnapshot();
}

void OOctagonProcessor::publishSnapshot()
{
    oo::VenueSnapshot snapshot;

    snapshot.spk             = venue.speakerPositions();
    snapshot.speakerToBuffer = speakerToBuffer;

    for (int i = 0; i < ochan::kNumSpeakers; ++i)
    {
        // trimLin is CARRIED here but APPLIED nowhere in Phase 2.1 — trim application is FUNC-07
        // at Phase 2.3. Putting it in the snapshot now costs nothing and means 2.3 adds a
        // multiply, not a plumbing change.
        snapshot.trimLin[(size_t) i] = venue.trimLin (i);
        snapshot.hullPts[(size_t) i] = hull.getHullPoint (i);
    }

    snapshot.hullCount    = hull.getNumHullPoints();
    snapshot.hullEpsCross = hull.getCrossEpsilon();

    snapshot.centroid  = venue.centroid();
    snapshot.rigScale  = venue.rigScale();
    snapshot.bbMinX    = venue.bbMinX();
    snapshot.bbMaxX    = venue.bbMaxX();
    snapshot.bbMinY    = venue.bbMinY();
    snapshot.bbMaxY    = venue.bbMaxY();
    snapshot.rakeFront = venue.rakeFront();
    snapshot.rakeRear  = venue.rakeRear();

    venuePublisher.publish (snapshot);
}

void OOctagonProcessor::applyVenueEdit (const oo::VenueModel& newVenue)
{
    venue = newVenue;
    venue.writeToState (apvts.state);

    hull.build (venue.speakerPositions());

    // A label edit can change the map, so the single construction site runs — and publishes.
    if (preparedYet)
        rebuildChannelMap();
    else
        publishSnapshot();
}

void OOctagonProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Phase 2.1: the mono sum still reaches every output at unity, but it now reaches them THROUGH
    // THE CHANNEL MAP. The Stage-1 placeholder block that wrote by raw buffer index is retired, not
    // grandfathered — its greppable marker token is gone from this file entirely.
    //
    // All 8 lanes still carry IDENTICAL signal, and that is correct here — independence needs the
    // DBAP solve and is FUNC-01 at Phase 2.2. Do not read a moving meter as evidence of routing.

    const int numSamples = buffer.getNumSamples();

    // Bound by buffer.getNumChannels(), NOT 8 and NOT getTotalNumOutputChannels(). On a 3-7 output
    // device canonicalChannelSet(n) is rejected, Debug asserts, and Release KEEPS the 7.1 layout
    // while the buffer holds only n channels (RESEARCH F3). getTotalNumOutputChannels() returns 8
    // in exactly that state — it is the accessor that lies.
    const int numOut = buffer.getNumChannels();
    const int numIn  = juce::jmin (getTotalNumInputChannels(), numOut);

    // Read BEFORE writing: out[0] and in[0] alias the same memory, so writing outputs channel-major
    // would destroy in[1] before it is summed. Summing per sample keeps that ordering without a
    // scratch buffer. The loop also removes the last literal channel indices from this function —
    // the Stage-1 version named input channels 0 and 1 explicitly.
    const float inScale = numIn > 0 ? 1.0f / static_cast<float> (numIn) : 0.0f;

    const auto monoSumAt = [&buffer, numIn, inScale] (int n)
    {
        float sum = 0.0f;

        for (int ch = 0; ch < numIn; ++ch)
            sum += buffer.getSample (ch, n);

        return sum * inScale;                    // numIn == 0 yields silence, not a read of ch 0
    };

    // Acquired ONCE per block and held. Re-reading mid-block would let a venue edit land between
    // two speakers of one write.
    const auto& snapshot = venuePublisher.read();

    // The G1 branch, stated in exactly one place (mappedOutputAvailable) and consumed here.
    // Phase 2.2's GainStage calls the same helper rather than re-deriving the condition.
    if (mappedOutputAvailable (numOut))
    {
        for (int n = 0; n < numSamples; ++n)
        {
            const float s = monoSumAt (n);

            for (int i = 0; i < ochan::kNumSpeakers; ++i)
                buffer.setSample (snapshot.speakerToBuffer[(size_t) i], n, s);
        }
    }
    else
    {
        // SAFE mode. Load-bearing for AU, not only for Standalone on a 2-channel interface: JUCE
        // derives the (1,1), (1,2), (2,1) and (2,2) AU configs from isBusesLayoutSupported() and
        // auval exercises all of them (RESEARCH F2). Also the F3 path, where the map is valid but
        // the buffer is narrower than it.
        for (int n = 0; n < numSamples; ++n)
        {
            const float s = monoSumAt (n);

            for (int ch = 0; ch < numOut; ++ch)
                buffer.setSample (ch, n, s);
        }
    }
}

//==============================================================================
bool OOctagonProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OOctagonProcessor::createEditor()
{
    // No PluginEditor.{h,cpp} yet. The generic editor renders all 17 parameters with their names,
    // ranges, defaults and units, and gives the Standalone build a non-empty window for the
    // COMPAT-04 eyeball check. It is deleted at Phase 3.1; nothing may come to depend on it.
    //
    // ── The #if is PROVABLY INERT TODAY, and that is exactly why it goes in now (G8) ────────────
    // Both arms are identical because there is no WebView editor yet. The render-harness target
    // compiles this TU with JUCE_WEB_BROWSER=0, under which WebBrowserComponent's types do not
    // exist. When Phase 3.1 swaps the real editor into the #else arm, the harness keeps building.
    // Added after the swap instead, it would be a build break in a target nobody is looking at
    // (pattern_render_harness_breaks_on_webview_editor).
   #if JUCE_WEB_BROWSER
    return new juce::GenericAudioProcessorEditor (*this);
   #else
    return new juce::GenericAudioProcessorEditor (*this);
   #endif
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

    // ── ORDERING HAZARD (ARCHITECTURE §4.1). This sequence is not interchangeable ───────────────
    //
    // 1. replaceState() above swaps in the restored tree, VENUE child and all.
    // 2. readVenueFromState() re-derives the geometry and the hull from whatever that tree carries.
    //    A session written during Stage 1 has NO VENUE child; it restores to the §OQ4 defaults
    //    silently, per attribute, and that is the common case for every project saved so far.
    // 3. Only then can the map be rebuilt, because the labels it resolves come from the venue.
    readVenueFromState();

    // Normalise the tree: a missing or partial VENUE node is written back complete, so the next
    // getStateInformation() is self-describing and a Stage-1 session is upgraded exactly once.
    venue.writeToState (apvts.state);

    // The map is built from the NEGOTIATED layout, so it can only be built once the host has
    // negotiated one. A host that calls setStateInformation() before prepareToPlay() gets the
    // rebuild deferred to prepareToPlay() rather than a second construction site appearing here.
    //
    // This is a plain flag, not an AsyncUpdater: there is no queued apply that could stomp the
    // state we have just restored, and therefore no cancelPendingUpdate() obligation
    // (pattern_asyncupdater_guard_flag_needs_cancel).
    if (preparedYet)
        rebuildChannelMap();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OOctagonProcessor();
}
