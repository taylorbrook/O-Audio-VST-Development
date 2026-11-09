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

// OGI_DEV_LOADER (CONTEXT-2.1 D13, PLAN-2.1 P4) is defined =1 on the PLUGIN
// target only (CMake option, default ON until Phase 3.1). The consoles never
// see it and build with JUCE_WEB_BROWSER=0, so they compile the generic editor
// branch and never include the dev-loader header.
#ifndef OGI_DEV_LOADER
 #define OGI_DEV_LOADER 0
#endif

#if JUCE_WEB_BROWSER && OGI_DEV_LOADER
 #include "PluginEditorDevLoader.h"   // removed in Phase 3.1 with OGI_DEV_LOADER
#endif

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

    // Scale state defaults (D17): every mask all-on, the 12-EDO custom scale.
    for (auto& m : degreeMasks)
        m.store (kAllOn, std::memory_order_relaxed);
    {
        juce::String error, badToken;
        const bool ok = PitchCollections::tableFromText (defaultCustomText(), customTable, error, badToken);
        jassertquiet (ok);
        customText = defaultCustomText();
    }

    // Phase 2.2 engine + slot store. registerBasicFormats here (message thread);
    // reader creation happens on the loader worker.
    formatManager.registerBasicFormats();
    engine = std::make_unique<ogi::Engine> (handoff);
    engine->bind (parameters);
    setEngineDestRanges();

    // PD3: the engine reads the masks (relaxed) and copies the custom table under
    // a non-blocking try-lock when customVersion moves. The index layouts agree:
    static_assert ([]
                   {
                       for (int s = 1; s <= Ids::kStreams; ++s)
                           for (int c = 0; c < Ids::kNumCollections; ++c)
                               if (maskIndex (s, c) != ogi::scaleMaskIndex (s - 1, c))
                                   return false;
                       for (int c = 0; c < Ids::kNumCollections; ++c)
                           if (Ids::scaleChoiceToId (c) != ogi::scaleChoiceToId (c))
                               return false;
                       return true;
                   }()
                   && Ids::kNumCollections == ogi::kNumScaleChoices
                   && Ids::kStreams == ogi::kGiStreams,
                   "engine scale indexing must match the processor's mask / choice layout");
    ogi::ScaleSource scale;
    scale.masks         = degreeMasks.data();
    scale.lock          = &customLock;
    scale.customTable   = &customTable;
    scale.customVersion = &customVersion;
    engine->bindScale (scale);

    loader = std::make_unique<ogi::SlotLoader> (formatManager, handoff);

    updateSidechainChannelCount();
}

GranulatorInstrumentProcessor::~GranulatorInstrumentProcessor()
{
    // Explicit order (members would do the same): stop and join the loader
    // worker first, then free the engine's tapes; the handoff frees the rest.
    loader.reset();
    engine.reset();
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
/** PLAN-2.5a PD3: the matrix range table from the parameter objects themselves
    (the same contract rows as createParameterLayout) and the raw atomic of every
    destination. Message / host thread only (process() not running). */
void GranulatorInstrumentProcessor::setEngineDestRanges()
{
    for (int d = 1; d < Ids::kModDests; ++d)
    {
        const juce::String id (Ids::kModDestIds[d]);
        auto* f = dynamic_cast<juce::AudioParameterFloat*> (parameters.getParameter (id));
        jassert (f != nullptr);
        if (f != nullptr)
            engine->setDestRange (d, f->getNormalisableRange(), parameters.getRawParameterValue (id));
    }
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
    // JUCE 8: getLatencySamples() is non-virtual; report through the setter, here only.
    setLatencySamples (0);

    // From the NEGOTIATED layout, never from the buffer width (a host may hand
    // a 2-channel buffer to any layout).
    updateSidechainChannelCount();

    // Engine: pool, voices, feedback return (zeroed), ramps. Loaded tapes stay;
    // until their rebuild lands the reader corrects the pitch by tapeRate / fs.
    setEngineDestRanges();   // PLAN-2.5a PD3: host thread, process() not running
    engine->prepare (sampleRate, samplesPerBlock);

    // Host thread: SlotInfo only under the loader lock; SR rebuilds are queued
    // via callAsync and deduped by (slot, rate) on the message thread.
    loader->setSessionRate (sampleRate);
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
    // Entry / exit counters bracket EVERY path (SlotHandoff reaper quiescence).
    handoff.blockEntered();
    juce::ScopedNoDenormals noDenormals;

    // Keep the note-expression raw-event queue cycling (a no-op outside VST3).
    vst3Extensions.drainAndUpdate();

    // Clear EVERY channel the buffer carries first (outputs and input-only
    // channels alike): the in-place buffer holds the sidechain, which never
    // reaches the output (D9). The sidechain is not read in 2.1.
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);

    // Render into the (cleared) stereo output. The output bus is stereo by
    // isBusesLayoutSupported; a buffer narrower than 2 stays silent.
    // PLAN-2.5a PD12: the host transport, read ONCE per block by value (no
    // allocation). The only playhead read in the plugin; no playhead → the
    // tracker's own 120 bpm virtual transport.
    ogi::TransportSnapshot ts;
    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            const auto bpm = pos->getBpm();
            const auto ppq = pos->getPpqPosition();
            ts.valid   = bpm.hasValue() && ppq.hasValue();
            ts.playing = pos->getIsPlaying();
            ts.bpm     = bpm.orFallback (0.0);
            ts.ppq     = ppq.orFallback (0.0);
            if (const auto bar = pos->getPpqPositionOfLastBarStart())
            {
                ts.hasLastBar = true;
                ts.lastBarPpq = *bar;
            }
            if (const auto sig = pos->getTimeSignature())
            {
                ts.num = sig->numerator;
                ts.den = sig->denominator;
            }
        }
    }

    // PLAN-2.5b PD6: this block's stamped VST3 raw events (NoteOn / NoteOff
    // with noteIds, Note Expression values), converted for the engine. Empty
    // outside VST3 (AU / Standalone deliver none).
    int numNe = 0;
    for (int i = 0; i < vst3Extensions.neCount; ++i)
    {
        const auto& r = vst3Extensions.neBuf[(size_t) i];
        using Kind = juce::VST3ClientExtensions::Vst3RawEvent::Kind;
        ogi::GiNeEvent g;
        g.offset  = (int) r.sampleOffset;
        g.type    = r.kind == Kind::NoteOn ? 0 : (r.kind == Kind::NoteOff ? 1 : 2);
        g.channel = (int) r.channel + 1;   // VST3 channel 0-15 → MIDI 1-16
        g.pitch   = (int) r.pitch;
        g.noteId  = (int) r.noteId;
        g.typeId  = r.typeId;
        g.value   = r.value;
        neEvents[(size_t) numNe++] = g;
    }
    vst3Extensions.neCount = 0;
   #if OGI_TEST_HOOKS
    engine->hooks.neDropped += vst3Extensions.neDropped;
   #endif
    vst3Extensions.neDropped = 0;

    if (n > 0 && buffer.getNumChannels() >= 2)
        engine->process (buffer.getWritePointer (0), buffer.getWritePointer (1), n, midiMessages, getSeed(), ts,
                         neEvents.data(), numNe);

    // MIDI is consumed (producesMidi() == false).
    midiMessages.clear();

    handoff.blockExited();
}

void GranulatorInstrumentProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    vst3Extensions.neCount = 0;   // P6: a bypassed block's NE events never replay
    vst3Extensions.neDropped = 0;
    AudioProcessor::processBlockBypassed (buffer, midiMessages);
}

juce::AudioProcessorEditor* GranulatorInstrumentProcessor::createEditor()
{
#if JUCE_WEB_BROWSER && OGI_DEV_LOADER
    // D13 audition path until Phase 3.1 (the WebView editor replaces it).
    return new DevLoaderEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // headless consoles / OGI_DEV_LOADER off
#endif
}

//==============================================================================
void GranulatorInstrumentProcessor::requestSlotLoad (int slot, const juce::File& file)
{
    loader->requestSlotLoad (slot, file);
}

ogi::SlotInfo GranulatorInstrumentProcessor::getSlotInfo (int slot) const
{
    return loader->getSlotInfo (slot);
}

void GranulatorInstrumentProcessor::requestReset (int slot)
{
    loader->requestReset (slot);
}

//==============================================================================
// Scale state (D17; parent 82406fce PluginProcessor.cpp:355-445, names only changed)
const juce::String& GranulatorInstrumentProcessor::defaultCustomText()
{
    static const juce::String t ("100 200 300 400 500 600 700 800 900 1000 1100 1200");
    return t;
}

GranulatorInstrumentProcessor::CustomScale GranulatorInstrumentProcessor::getCustomScale() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return { customName, customText, customTable.period, customVersion.load (std::memory_order_relaxed) };
}

PitchCollections::Table GranulatorInstrumentProcessor::getCustomTable() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return customTable;
}

juce::String GranulatorInstrumentProcessor::setCustomScale (const juce::String& name, const juce::String& text, juce::String& badToken)
{
    // Parse OUTSIDE the lock; swap under it.
    PitchCollections::Table t;
    juce::String error;
    if (! PitchCollections::tableFromText (text, t, error, badToken))
        return error.isNotEmpty() ? error : juce::String ("badPitch");

    {
        const juce::SpinLock::ScopedLockType sl (customLock);
        customTable = t;
        customText  = text.trim();
        customName  = name.substring (0, kMaxCustomNameChars);
    }
    // The degrees re-numbered: every stream's Custom mask starts from all-on.
    for (int s = 1; s <= Ids::kStreams; ++s)
        degreeMasks[static_cast<size_t> (maskIndex (s, PitchCollections::Custom))].store (kAllOn, std::memory_order_relaxed);
    customVersion.fetch_add (1, std::memory_order_acq_rel);
    return {};
}

const PitchCollections::Table* GranulatorInstrumentProcessor::getCollectionTable (int choice) const
{
    const int id = Ids::scaleChoiceToId (choice);
    if (id < 0 || choice >= Ids::kNumCollections)
        return nullptr;
    if (id == PitchCollections::Custom)
        return &customTable;   // message thread: setCustomScale runs there too
    return &PitchCollections::builtIns()[static_cast<size_t> (id)];
}

int GranulatorInstrumentProcessor::degreeCountFor (int collection) const
{
    if (collection == PitchCollections::Custom)
    {
        const juce::SpinLock::ScopedLockType sl (customLock);
        return customTable.count;
    }
    const auto* t = getCollectionTable (collection);
    return t != nullptr ? t->count : 64;
}

uint64_t GranulatorInstrumentProcessor::getDegreeMask (int stream, int collection) const noexcept
{
    if (stream < 1 || stream > Ids::kStreams || collection < 1 || collection >= Ids::kNumCollections)
        return kAllOn;
    return degreeMasks[static_cast<size_t> (maskIndex (stream, collection))].load (std::memory_order_relaxed);
}

void GranulatorInstrumentProcessor::setDegreeMask (int stream, int collection, uint64_t mask) noexcept
{
    if (stream < 1 || stream > Ids::kStreams || collection < 1 || collection >= Ids::kNumCollections)
        return;
    degreeMasks[static_cast<size_t> (maskIndex (stream, collection))]
        .store (canonicalMask (mask, degreeCountFor (collection)), std::memory_order_relaxed);
}

void GranulatorInstrumentProcessor::resetNonParameterState()
{
    juce::String error, badToken;
    PitchCollections::Table t;
    PitchCollections::tableFromText (defaultCustomText(), t, error, badToken);
    {
        const juce::SpinLock::ScopedLockType sl (customLock);
        customTable = t;
        customText  = defaultCustomText();
        customName  = {};
    }
    customVersion.fetch_add (1, std::memory_order_acq_rel);
    for (auto& m : degreeMasks)
        m.store (kAllOn, std::memory_order_relaxed);
}

//==============================================================================
// State codec
//
//   <OGranulatorInstrument uiLanguage="fr" ...PARAM children...>
//     <Engine seed="-1234" stateVersion="1" modDestTableVersion="1"/>
//     <CustomScale name="..." text="edo 19" period="1200" version="3"/>
//     <DegreeMasks> <Mask stream="1" collection="19" bits="8000000000000001"/> ... </DegreeMasks>
//     <Slots/>                     (Slot children: Phase 2.6, keyed by "index")
//   </OGranulatorInstrument>
//
// - Written onto the copyState() COPY; the live tree is never mutated from a host thread.
// - No `id` attribute on any custom child: APVTS treats a top-level child's
//   `id` as a parameter ID.
// - seed as an int64 STRING (getLargeIntValue clamps a uint64 above INT64_MAX).
// - Scale state (D17, PD4): SIBLING children <CustomScale> / <DegreeMasks>
//   (the parent's names; this refines CONTEXT D17's "keys on <Engine>").
//   Masks as 16 hex digits (%016x), only non-default (≠ all-on) ones written,
//   read back ONLY as exactly 16 hex digits through getHexValue64 (never
//   getLargeIntValue: it clamps bit 63, P17). version is written & 0x7fffffff.
// - Restore reads Engine / CustomScale / DegreeMasks BEFORE stripping them,
//   strips before replaceState (a stale child never shadows a reopen, P19),
//   then applyNonParameterState resets every non-parameter value to its
//   default FIRST (absent = default, never "leave as is", P18).
// - Every property comes back from XML as a string var: isVoid() guards, then
//   parse; never isBool().
//==============================================================================
namespace
{
    const juce::Identifier kEngineTag ("Engine"), kSlotsTag ("Slots"),
                           kCustomScaleTag ("CustomScale"), kDegreeMasksTag ("DegreeMasks"), kMaskTag ("Mask");
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
   #if OGI_TEST_HOOKS
    if (! testNoStateStrip)
   #endif
    removeCustomChildren (state);   // never duplicate children restored into the live tree

    state.setProperty (kUiLanguageAttr, languageCode (uiLanguage.load (std::memory_order_relaxed)), nullptr);

    juce::ValueTree eng (kEngineTag);
    eng.setProperty ("seed", juce::String (static_cast<juce::int64> (getSeed())), nullptr);
    eng.setProperty ("stateVersion", Ids::kStateVersion, nullptr);
    eng.setProperty ("modDestTableVersion", Ids::kModDestTableVersion, nullptr);
    state.appendChild (eng, nullptr);

    const auto custom = getCustomScale();
    juce::ValueTree cs (kCustomScaleTag);
    cs.setProperty ("name", custom.name, nullptr);
    cs.setProperty ("text", custom.text, nullptr);
    cs.setProperty ("period", custom.period, nullptr);
    cs.setProperty ("version", static_cast<int> (custom.version & 0x7fffffffu), nullptr);
    state.appendChild (cs, nullptr);

    juce::ValueTree dm (kDegreeMasksTag);
    for (int s = 1; s <= Ids::kStreams; ++s)
        for (int c = 1; c < Ids::kNumCollections; ++c)
        {
            const uint64_t mask = getDegreeMask (s, c);
            if (mask != kAllOn)
                dm.appendChild (juce::ValueTree (kMaskTag, { { "stream", s }, { "collection", c },
                                                             { "bits", juce::String::toHexString (static_cast<juce::int64> (mask)).paddedLeft ('0', 16) } }),
                                nullptr);
        }
    state.appendChild (dm, nullptr);

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
    const juce::ValueTree cs  = tree.getChildWithName (kCustomScaleTag);
    const juce::ValueTree dm  = tree.getChildWithName (kDegreeMasksTag);
    const juce::var lang = tree.getProperty (kUiLanguageAttr);
    // <Slots>: Phase 2.6 parses its <Slot index> children and enqueues loader jobs.

   #if OGI_TEST_HOOKS
    if (! testNoStateStrip)
   #endif
    removeCustomChildren (tree);
    tree.removeProperty (kUiLanguageAttr, nullptr);
    parameters.replaceState (tree);

    if (! lang.isVoid())
    {
        const auto s = lang.toString().trim();
        uiLanguage.store (s.isNotEmpty() && s.containsOnly ("0123456789") ? juce::jlimit (0, 2, s.getIntValue())
                                                                          : languageIndex (s),
                          std::memory_order_relaxed);
    }

    // The XML children adapted into the shared apply path's inputs (parent P98):
    // the property vars pass through untouched (XML gives strings).
    juce::var custom, masks, seedVar;
    if (eng.isValid())
        seedVar = eng.getProperty ("seed");   // stateVersion / modDestTableVersion: 1 is the only version so far
    if (cs.isValid())
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", cs.getProperty ("name"));
        o->setProperty ("text", cs.getProperty ("text"));
        custom = juce::var (o);
    }
    if (dm.isValid())
    {
        juce::Array<juce::var> list;
        for (const auto& m : dm)
        {
            if (! m.hasType (kMaskTag))
                continue;
            auto* o = new juce::DynamicObject();
            o->setProperty ("stream",     m.getProperty ("stream"));
            o->setProperty ("collection", m.getProperty ("collection"));
            o->setProperty ("bits",       m.getProperty ("bits"));
            list.add (juce::var (o));
        }
        masks = list;
    }

    applyNonParameterState (custom, masks, seedVar);
}

void GranulatorInstrumentProcessor::applyNonParameterState (const juce::var& custom, const juce::var& masks, const juce::var& seedVar)
{
    // Defaults FIRST (P18), so anything absent reads as default rather than as
    // whatever this instance held. The seed is not part of the reset (there is
    // no default seed): absent keeps the live one.
   #if OGI_TEST_HOOKS
    if (! testNoStateReset)
   #endif
    resetNonParameterState();

    if (! seedVar.isVoid())
    {
        const auto s = seedVar.toString().trim();
        if (s.isNotEmpty() && s.containsOnly ("-0123456789"))
            setSeed (static_cast<uint64_t> (s.getLargeIntValue()));
    }

    if (auto* c = custom.getDynamicObject())
    {
        const auto text = c->getProperty ("text");
        if (! text.isVoid() && text.toString().isNotEmpty())
        {
            juce::String badToken;
            const auto name = c->getProperty ("name");
            setCustomScale (name.isVoid() ? juce::String() : name.toString(), text.toString(), badToken);   // failure keeps the 12-EDO default
        }
    }

    if (const auto* list = masks.getArray())
        for (const auto& m : *list)
        {
            auto* o = m.getDynamicObject();
            if (o == nullptr)
                continue;
            const auto sv = o->getProperty ("stream"), cv = o->getProperty ("collection"), bv = o->getProperty ("bits");
            if (sv.isVoid() || cv.isVoid() || bv.isVoid())
                continue;
            const auto bits = bv.toString();
            if (bits.length() != 16 || ! bits.containsOnly ("0123456789abcdefABCDEF"))
                continue;   // P17: exactly 16 hex digits, or ignored (all-on stays)
            setDegreeMask (static_cast<int> (sv), static_cast<int> (cv), static_cast<uint64_t> (bits.getHexValue64()));
        }
}

//==============================================================================
#if OGI_TEST_HOOKS
bool GranulatorInstrumentProcessor::loadSlotSync (int slot, const juce::File& file, juce::String& error)
{
    return loader->loadSync (slot, file, error);
}

bool GranulatorInstrumentProcessor::loadSlotSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                                                  const juce::String& name, juce::String& error)
{
    return loader->loadSync (slot, pcm, sourceRate, name, error);
}

bool GranulatorInstrumentProcessor::resetSlotSync (int slot)
{
    juce::String error;
    if (slot == -1)
    {
        bool ok = true;
        for (int s = 0; s < ogi::kTapeSlots; ++s)
            ok = loader->resetSync (s, error) && ok;
        return ok;
    }
    return loader->resetSync (slot, error);
}
#endif

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranulatorInstrumentProcessor();
}
