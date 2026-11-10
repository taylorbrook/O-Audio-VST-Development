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
#include "dsp/GiHash.h"

#include <algorithm>
#include <cmath>
#include <limits>

// Phase 3.1 (PD15): the WebView editor under JUCE_WEB_BROWSER, the generic
// editor otherwise. The consoles build with JUCE_WEB_BROWSER=0, drop
// PluginEditor.cpp and must never see WebView types.
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

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
    // PLAN-2.6: a published commit with a failed WAV → the recorder's reason
    // (an atomic flag; the worker never touches audio state otherwise).
    loader->onCommitted = [rec = &engine->getRecorder()] (int, bool wavFailed)
    {
        if (wavFailed)
            rec->noteWavFailed();
    };

    updateSidechainChannelCount();

    // Stage 3 (PLAN-3.1 PD2): preset FILES. The seed, the custom scale, the
    // masks and the slot SOURCES (never a blob, PD1) travel in customState.
    // output_gain is the user's trim; rec_arm must never see the Off → On edge
    // a preset apply would produce (reset to default, then the saved value —
    // that edge arms a capture, D34). Neither is written, reset or applied.
    // No directory I/O here: the manager creates its folders lazily (auval).
    // No factory presets (Stage 4).
    presetManager.setCustomStateCallbacks ([this] { return presetCustomState(); },
                                           [this] (const juce::var& v) { applyPresetCustomState (v); });
    presetManager.setExcludedParameters ({ "output_gain", "rec_arm" });
    presetManager.setValidatedApply (true);

    // PLAN-2.6 PD7: the commit poll (never AsyncUpdater: it locks + reallocs
    // on the audio thread).
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
    {
        recPollTimer = std::make_unique<RecPollTimer> (*this);
        recPollTimer->startTimer (20);
    }
    else
    {
        recPollThread = std::make_unique<RecPollThread> (*this);
        recPollThread->startThread();
    }
}

GranulatorInstrumentProcessor::~GranulatorInstrumentProcessor()
{
    // PLAN-3.2 PD19: the save worker goes FIRST (it calls into the engine's
    // tape snapshot and owns the buffers the audio thread may hold). Its waits
    // are all exit-polled, so the join is bounded by one WAV chunk write. The
    // audio callback is stopped while a processor is destroyed and the worker
    // is joined, so rule (b) of the snapshot's lifetime invariant applies:
    // resetAudioStopped(), THEN free. What is still undelivered completes here.
    saveAlive->store (false, std::memory_order_release);
    if (saveWorker != nullptr)
        saveWorker->stopThread (-1);
    saveWorker.reset();
    {
        const juce::ScopedLock sl (saveLock);
        engine->tapeSnapshot().resetAudioStopped();
        saveBufL.free();
        saveBufR.free();
        saveLent = false;
        if (saveJob != nullptr)   // queued, never started
        {
            SaveResult r;
            r.error = "cancelled";
            saveDone.emplace_back (std::move (saveJob->completion), r);
            saveJob.reset();
        }
        saveInFlight = false;
    }
    deliverSaveCompletions();

    // Explicit order: stop the commit poll first (it calls the loader and the
    // engine), then stop and join the loader worker, then free the engine's
    // tapes; the handoff frees the rest.
    if (recPollThread != nullptr)
        recPollThread->stopThread (2000);
    recPollThread.reset();
    recPollTimer.reset();
    loader.reset();
    engine.reset();
}

void GranulatorInstrumentProcessor::pollRecorder()
{
    auto take = std::make_shared<ogi::SlotLoader::Pristine>();
    ogi::RecCommit c;
    if (! engine->getRecorder().takeCommit (take->pcm, c))
        return;
    take->rate = c.rate;
    loader->requestCommit (c.slot, std::move (take));
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

    // PLAN-3.2 PD18 / PD19: the audio callback is stopped here, so the buffer
    // of a timed-out save that the audio thread never acknowledged can go
    // (rule (b)). A save IN FLIGHT is left alone: its copy resumes or aborts
    // with the first block, or times out.
    reclaimSaveBufferAudioStopped();

    // Engine: pool, voices, feedback return (zeroed), ramps. Loaded tapes stay;
    // until their rebuild lands the reader corrects the pitch by tapeRate / fs.
    setEngineDestRanges();   // PLAN-2.5a PD3: host thread, process() not running
    engine->prepare (sampleRate, samplesPerBlock);

    // Host thread: SlotInfo only under the loader lock; SR rebuilds are queued
    // via callAsync and deduped by (slot, rate) on the message thread.
    loader->setSessionRate (sampleRate);
}

void GranulatorInstrumentProcessor::releaseResources()
{
    reclaimSaveBufferAudioStopped();   // PLAN-3.2 PD18: audio stopped (rule (b))
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

    // PLAN-2.6 PD19: channels >= 2 are never outputs and are cleared here.
    // Channels 0 / 1 hold the in-place sidechain: the engine copies it into a
    // scratch per 32-grid piece, then zeroes and renders the output (the
    // sidechain never reaches the output, D9 / QUAL-02). Only the NEGOTIATED
    // sidechain channels are read (ch 1 of a mono sidechain is host garbage).
    const int n = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    const int scCh = sidechainChannels.load (std::memory_order_relaxed);
    for (int ch = 2; ch < nCh; ++ch)
        buffer.clear (ch, 0, n);

    // Render the stereo output. The output bus is stereo by
    // isBusesLayoutSupported; a buffer narrower than 2 is cleared (silent).
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

    // PLAN-3.2 PD11: the tempo for the UI's `rec.capped`, as the tracker will
    // read this snapshot (bpm 0 = not valid → its 120 bpm fallback). Two
    // relaxed stores; nothing on the audio thread reads them back.
    uiBpm.store (ts.valid && std::isfinite (ts.ppq) ? ts.bpm : 0.0, std::memory_order_relaxed);
    uiBarQuarters.store (4.0 * static_cast<double> (ts.num > 0 ? ts.num : 4) / static_cast<double> (ts.den > 0 ? ts.den : 4),
                         std::memory_order_relaxed);

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

    if (n > 0 && nCh >= 2)
    {
        float* ch0 = buffer.getWritePointer (0);
        float* ch1 = buffer.getWritePointer (1);
        engine->process (ch0, ch1, n, midiMessages, getSeed(), ts, neEvents.data(), numNe,
                         scCh >= 1 ? ch0 : nullptr, scCh >= 2 ? ch1 : nullptr);
    }
    else
    {
        buffer.clear();   // the engine was not called: nothing of the input survives
    }

    // MIDI is consumed (producesMidi() == false).
    midiMessages.clear();

    handoff.blockExited();
}

void GranulatorInstrumentProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    vst3Extensions.neCount = 0;   // P6: a bypassed block's NE events never replay
    vst3Extensions.neDropped = 0;
   #if OGI_TEST_HOOKS
    if (engine->hooks.baseBypass)
    {
        // LK2n: the JUCE base clears only channels >= the main-input count
        // (juce_AudioProcessor.cpp:601-602); the Sidechain IS the main input
        // bus, so a stereo sidechain passes straight to the output (RS1).
        AudioProcessor::processBlockBypassed (buffer, midiMessages);
        return;
    }
   #endif
    // PLAN-2.6 PD20 (RS1 leak fix): every channel cleared — the in-place
    // buffer holds the sidechain, which never reaches the output. The base is
    // never called.
    buffer.clear();
    juce::ignoreUnused (midiMessages);   // MIDI passes untouched (producesMidi() == false)
    engine->bypassedBlock();
}

juce::AudioProcessorEditor* GranulatorInstrumentProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new GranulatorInstrumentEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // headless consoles
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

void GranulatorInstrumentProcessor::requestRelink (int slot, const juce::File& file)
{
    loader->requestRelink (slot, file);
}

void GranulatorInstrumentProcessor::setSlotEmbed (int slot, bool on)
{
    loader->setSlotEmbed (slot, on);
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
    bumpScaleRevision();   // PLAN-3.1 PD4 (UI only; the engine reads customVersion)
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
    bumpScaleRevision();   // PLAN-3.1 PD4
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
    bumpScaleRevision();   // PLAN-3.1 PD4
}

//==============================================================================
// State codec
//
//   <OGranulatorInstrument uiLanguage="fr" ...PARAM children...>
//     <Engine seed="-1234" stateVersion="1" modDestTableVersion="1"/>
//     <CustomScale name="..." text="edo 19" period="1200" version="3"/>
//     <DegreeMasks> <Mask stream="1" collection="19" bits="8000000000000001"/> ... </DegreeMasks>
//     <Slots>                      (PLAN-2.6 PD21; only non-empty slots, fixed attribute order)
//       <Slot index="0" source="rec:/…/GI-Rec-….wav" name="…" fileSize="123" contentHash="0123456789abcdef"
//             embed="1" trimmed="0" embedded="…base64 FLAC-24…"/>
//     </Slots>
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
    const juce::Identifier kEngineTag ("Engine"), kSlotsTag ("Slots"), kSlotTag ("Slot"),
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

    // PLAN-2.6 PD21: the loader's records, copied under its lock (never
    // encodes: a blob still encoding → embed="1" without `embedded`).
    juce::ValueTree slots (kSlotsTag);
    for (const auto& x : loader->getSavedSlots())
    {
        juce::ValueTree t (kSlotTag);
        t.setProperty ("index", juce::String (x.index), nullptr);
        t.setProperty ("source", x.source, nullptr);
        t.setProperty ("name", x.name, nullptr);
        t.setProperty ("fileSize", juce::String (x.fileSize), nullptr);
        if (x.hasHash)
            t.setProperty ("contentHash", ogi::gihash::toHex (x.hash), nullptr);
        t.setProperty ("embed", x.embed ? "1" : "0", nullptr);
        t.setProperty ("trimmed", x.trimmed ? "1" : "0", nullptr);
        if (x.embed && x.blob.isNotEmpty())
            t.setProperty ("embedded", x.blob, nullptr);
        slots.appendChild (t, nullptr);
    }
    state.appendChild (slots, nullptr);

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
    // PLAN-2.6 PD21: <Slots> parsed BEFORE the strip (P7); applied below.
    const auto slots = parseSlots (tree.getChildWithName (kSlotsTag));

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

    // Records written at once + resolve jobs queued (host thread; never decodes).
    loader->restoreSlots (slots);
}

std::vector<ogi::SavedSlot> GranulatorInstrumentProcessor::parseSlots (const juce::ValueTree& slotsTree) const
{
    // R§6.1: every property is an XML string var. index = exactly one digit
    // 0–3; a known source prefix or the slot is empty; embed / trimmed as "1"
    // or "true" (never isBool, P6); fileSize digits only; contentHash exactly
    // 16 hex (never getLargeIntValue, P5); embedded bounded + base64 alphabet.
    std::vector<ogi::SavedSlot> out;
    if (! slotsTree.isValid())
        return out;

    bool isBool = false;
   #if OGI_TEST_HOOKS
    isBool = testParseIsBool;
   #endif
    auto flag = [isBool] (const juce::var& v, bool dflt)
    {
        if (v.isVoid())
            return dflt;
        if (isBool)
            return v.isBool() ? static_cast<bool> (v) : dflt;   // ST5n: XML never yields a bool
        const auto s = v.toString().trim();
        return s == "1" || s.equalsIgnoreCase ("true");
    };

    for (const auto& c : slotsTree)
    {
        if (! c.hasType (kSlotTag))
            continue;
        const auto idx = c.getProperty ("index").toString().trim();
        if (idx.length() != 1 || ! idx.containsOnly ("0123"))
            continue;

        ogi::SavedSlot x;
        x.index  = idx.getIntValue();
        x.source = c.getProperty ("source").toString();
        if (! (x.source.startsWith ("factory:") || x.source.startsWith ("file:") || x.source.startsWith ("rec:")))
            continue;   // unknown scheme → empty
        x.name = c.getProperty ("name").toString();

        const auto fsz = c.getProperty ("fileSize").toString().trim();
        if (fsz.isNotEmpty() && fsz.length() <= 18 && fsz.containsOnly ("0123456789"))
            x.fileSize = fsz.getLargeIntValue();

        x.hasHash = ogi::gihash::fromHex (c.getProperty ("contentHash").toString().trim(), x.hash);
        x.embed   = flag (c.getProperty ("embed"), false);
        x.trimmed = flag (c.getProperty ("trimmed"), false);

        const auto blob = c.getProperty ("embedded");
        if (! blob.isVoid())
        {
            const auto b = blob.toString();
            if (b.length() <= ogi::SlotLoader::kBlobMaxChars && (b.length() % 4) == 0
                && b.containsOnly ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="))
                x.blob = b;
        }
        out.push_back (std::move (x));
    }
    return out;
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

    bumpScaleRevision();   // PLAN-3.1 PD4: one more after the whole apply (session restore and preset load)
}

//==============================================================================
// Presets (PLAN-3.1 PD1 / PD2; the parent's P99 / P100 wiring plus the slots).
// Same encodings as the session codec: the seed as an int64 decimal STRING (a
// JSON number is a double — it loses bits above 2^53 — and getLargeIntValue
// clamps a uint64 above INT64_MAX), masks as 16 hex digits, only non-default
// ones written. Slots: the loader's records WITHOUT the blob (sign-off
// 2026-11-10: sources, no blobs), fileSize as a decimal string, contentHash as
// 16 hex digits.
//
// Message thread. Nothing here decodes: restoreSlots writes the records under
// the loader lock and queues one resolve job per changed slot on its worker.
// The manager's XML session calls are never used (no <CustomState> child can
// reach the live tree).
//==============================================================================
namespace
{
    /** A JSON flag: a bool, a number (non-zero), or the codec's "1" / "true". */
    bool presetFlag (const juce::var& v)
    {
        if (v.isBool())
            return static_cast<bool> (v);
        if (v.isInt() || v.isInt64() || v.isDouble())
            return static_cast<double> (v) != 0.0;
        if (v.isString())
        {
            const auto s = v.toString().trim();
            return s == "1" || s.equalsIgnoreCase ("true");
        }
        return false;
    }

    /** customState.slots → SavedSlot records. Every field is type-checked;
        a malformed entry is skipped (its slot then counts as not listed). An
        `embedded` key in a hand-edited file is never read (PD1). */
    std::vector<ogi::SavedSlot> parsePresetSlots (const juce::Array<juce::var>& list)
    {
        std::vector<ogi::SavedSlot> out;
        for (const auto& v : list)
        {
            auto* o = v.getDynamicObject();
            if (o == nullptr)
                continue;

            int index = -1;
            const auto iv = o->getProperty ("index");
            if (iv.isInt() || iv.isInt64())
            {
                const auto i64 = static_cast<juce::int64> (iv);
                if (i64 >= 0 && i64 < ogi::kTapeSlots)
                    index = static_cast<int> (i64);
            }
            else if (iv.isString())
            {
                const auto t = iv.toString().trim();
                if (t.length() == 1 && t.containsOnly ("0123"))
                    index = t.getIntValue();
            }
            if (index < 0)
                continue;

            const auto sv = o->getProperty ("source");
            if (! sv.isString())
                continue;

            ogi::SavedSlot x;
            x.index  = index;
            x.source = sv.toString();
            if (! (x.source.startsWith ("factory:") || x.source.startsWith ("file:") || x.source.startsWith ("rec:")))
                continue;   // unknown scheme → not listed

            const auto nv = o->getProperty ("name");
            if (nv.isString())
                x.name = nv.toString();

            const auto fv = o->getProperty ("fileSize");
            if (fv.isString())
            {
                const auto t = fv.toString().trim();
                if (t.isNotEmpty() && t.length() <= 18 && t.containsOnly ("0123456789"))
                    x.fileSize = t.getLargeIntValue();
            }
            else if (fv.isInt() || fv.isInt64())
            {
                x.fileSize = std::max (static_cast<juce::int64> (0), static_cast<juce::int64> (fv));
            }

            const auto hv = o->getProperty ("contentHash");
            x.hasHash = hv.isString() && ogi::gihash::fromHex (hv.toString().trim(), x.hash);
            x.embed   = presetFlag (o->getProperty ("embed"));
            x.trimmed = presetFlag (o->getProperty ("trimmed"));
            out.push_back (std::move (x));
        }
        return out;
    }
}

juce::var GranulatorInstrumentProcessor::presetCustomState() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("seed", juce::String (static_cast<juce::int64> (getSeed())));

    const auto c = getCustomScale();
    auto* cs = new juce::DynamicObject();
    cs->setProperty ("name", c.name);
    cs->setProperty ("text", c.text);
    root->setProperty ("custom", juce::var (cs));

    juce::Array<juce::var> maskList;
    for (int s = 1; s <= Ids::kStreams; ++s)
        for (int col = 1; col < Ids::kNumCollections; ++col)
        {
            const uint64_t mask = getDegreeMask (s, col);
            if (mask == kAllOn)
                continue;
            auto* o = new juce::DynamicObject();
            o->setProperty ("stream", s);
            o->setProperty ("collection", col);
            o->setProperty ("bits", ogi::gihash::toHex (mask));
            maskList.add (juce::var (o));
        }
    root->setProperty ("masks", maskList);

    // PD1: the loader's records, copied under its lock; the blob is dropped.
    // Always present (an empty array = "this preset has no sources").
    juce::Array<juce::var> slotList;
    for (const auto& x : loader->getSavedSlots())
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("index", x.index);
        o->setProperty ("source", x.source);
        o->setProperty ("name", x.name);
        o->setProperty ("fileSize", juce::String (x.fileSize));
        if (x.hasHash)
            o->setProperty ("contentHash", ogi::gihash::toHex (x.hash));
        o->setProperty ("embed", x.embed);
        o->setProperty ("trimmed", x.trimmed);
        slotList.add (juce::var (o));
    }
    root->setProperty ("slots", slotList);
    return juce::var (root);
}

void GranulatorInstrumentProcessor::applyPresetCustomState (const juce::var& customState)
{
    // Only an int64 decimal STRING is a seed; anything else counts as absent
    // (a JSON number has already lost its low bits).
    juce::var custom, masks, seedVar, slotsVar;
    if (auto* o = customState.getDynamicObject())
    {
        custom = o->getProperty ("custom");
        masks  = o->getProperty ("masks");
        const auto sv = o->getProperty ("seed");
        if (sv.isString())
            seedVar = sv;
        slotsVar = o->getProperty ("slots");
    }

    applyNonParameterState (custom, masks, seedVar);   // defaults first; bumps the scale revision

    // PD1 + PLAN-3.2 PD15 (D45): a `slots` array → the listed slots are
    // restored (an identical source is a no-op); a slot the list has no source
    // for is LEFT ALONE (clearUnlisted = false: a parameter-only preset never
    // wipes a loaded lane or an evolved tape). Absent (or not an array) → all
    // slots stay as they are. The session path keeps clearing.
    if (const auto* list = slotsVar.getArray())
        loader->restoreSlots (parsePresetSlots (*list), false);
}

//==============================================================================
// Stage 3 / 3.1: what the page polls (PLAN-3.1 PD3 – PD6, PD9). Message thread;
// only the loader lock and relaxed atomics — nothing the audio thread waits on.
//==============================================================================
std::uint32_t GranulatorInstrumentProcessor::getOverviewRev (int slot0) const
{
    // The count is read FIRST: a publish that lands after it leaves this value
    // stale, and the next poll asks again.
    const std::uint64_t published = handoff.getPublishedCount();
    return overviewRevFor (slot0, loader->getSlotInfo (slot0), published);
}

std::uint32_t GranulatorInstrumentProcessor::overviewRevFor (int slot0, const ogi::SlotInfo& info, std::uint64_t published) const noexcept
{
    // PD6 (3.1) is generation · 2 + loaded. Reading SlotLoader.cpp, `generation`
    // moves at the REQUEST of a load / relink / commit / restore, not at the
    // publish, and never for a Reset or a rate rebuild (startRebuildLocked
    // reuses the slot's generation). A reload of a loaded slot would therefore
    // move the revision once, before its new overview exists, and never again.
    // So the handoff's publish count is folded in: it advances inside the same
    // loader-locked step that installs the overview (publishResult) and on
    // every clear. It is global, so a publish on one slot also moves the other
    // three (a redundant fetch, never a missed one).
    //
    // PLAN-3.2 PD6: + ovWrites[slot], the engine's count of record-head cells
    // folded into the live overview, so the lane refetches while feedback
    // rewrites its tape (the page limits itself to 10 Hz per lane).
    //
    // 2 · (generation + published) + loaded + writes: every term but `loaded`
    // only grows, and `loaded` falls only together with a generation bump.
    std::uint32_t rev = static_cast<std::uint32_t> ((info.generation + published) * 2u + (info.loaded ? 1u : 0u));
    bool withWrites = true;
   #if OGI_TEST_HOOKS
    withWrites = ! testOvRevNoWrites;   // UOV2n
   #endif
    if (withWrites)
        rev += engine->getTelemetry().getOvWrites (slot0);
    return rev;
}

std::uint32_t GranulatorInstrumentProcessor::getSlotStateRevision()
{
    const std::uint64_t published = handoff.getPublishedCount();
    std::array<ogi::SlotInfo, ogi::kTapeSlots> infos;
    for (int s = 0; s < ogi::kTapeSlots; ++s)
        infos[static_cast<std::size_t> (s)] = loader->getSlotInfo (s);
    return slotRevisionFor (infos, published);
}

std::uint32_t GranulatorInstrumentProcessor::slotRevisionFor (const std::array<ogi::SlotInfo, ogi::kTapeSlots>& infos,
                                                              std::uint64_t published)
{
    // PD3: a fingerprint, not a counter bumped at each SlotInfo write (about
    // 15 sites, several on the worker). FNV-style mixing of what the page
    // shows; the tape-identity term (generation + the publish count, see
    // overviewRevFor) makes a same-source reload and a Reset visible.
    // PLAN-3.2 PD7: the cached dirty flag is part of it (bit 7 of the flag
    // word; 0 while clean, so a clean slot hashes as in 3.1) — the page pulls
    // slot state on slotRev, so the Reset highlight follows the flag.
   #if OGI_TEST_HOOKS
    const bool tapeTerm  = ! testSlotRevNoGeneration;   // SR1n
    const bool dirtyTerm = ! testSlotRevNoDirty;        // UOV2n-b
   #else
    const bool tapeTerm = true, dirtyTerm = true;
   #endif

    std::uint64_t h = 0xcbf29ce484222325ull;
    const auto mix = [&h] (std::uint64_t v) noexcept
    {
        h = (h ^ v) * 0x100000001b3ull;
        h ^= h >> 29;
    };

    if (tapeTerm)
        mix (published);

    for (int s = 0; s < ogi::kTapeSlots; ++s)
    {
        const auto& info = infos[static_cast<std::size_t> (s)];
        if (tapeTerm)
            mix (info.generation);
        mix ((info.loaded ? 1u : 0u) | (info.loading ? 2u : 0u) | (info.missing ? 4u : 0u) | (info.embed ? 8u : 0u)
             | (info.hasBlob ? 16u : 0u) | (info.tooLarge ? 32u : 0u) | (info.trimmed ? 64u : 0u)
             | (dirtyTerm && slotDirty[static_cast<std::size_t> (s)] ? 128u : 0u));
        mix (static_cast<std::uint64_t> (info.source.hashCode64()));
        mix (static_cast<std::uint64_t> (info.name.hashCode64()));
        mix (static_cast<std::uint64_t> (info.lastError.hashCode64()));
    }

    if (! slotFingerprintValid || h != slotFingerprint)
    {
        slotFingerprint = h;
        slotFingerprintValid = true;
        ++slotRevision;
    }
    return slotRevision;
}

bool GranulatorInstrumentProcessor::getSlotOverview (int slot0, float* peaks, std::uint8_t* dirty, int n, std::uint32_t& rev) const
{
    rev = getOverviewRev (slot0);   // BEFORE the copy: a stale answer is re-fetched, never kept

    if (n > 0 && peaks != nullptr)
        std::fill_n (peaks, n, 0.0f);
    if (n > 0 && dirty != nullptr)
        std::fill_n (dirty, n, static_cast<std::uint8_t> (0));
    if (n <= 0 || peaks == nullptr)
        return false;

    // The loader's overview: one (min, max) pair per 512 tape frames.
    const auto info  = loader->getSlotInfo (slot0);
    const auto pairs = loader->getOverview (slot0);
    const auto bins  = static_cast<std::int64_t> (pairs.size() / 2);
    if (! info.loaded || bins <= 0)
        return false;

    // PLAN-3.2 PD7: the engine's live bins (same 512-frame grid) replace the
    // pristine pair where the record head rewrote the CURRENT tape (epoch-
    // valid; a stale epoch reads false and the pristine pair stands).
    const auto& live = engine->getTelemetry().overview;

    for (int i = 0; i < n; ++i)
    {
        // Source bins [b0, b1) of peak i (at least one: a short tape repeats bins).
        const std::int64_t b0 = static_cast<std::int64_t> (i) * bins / n;
        const std::int64_t b1 = std::max (b0 + 1, std::min (bins, static_cast<std::int64_t> (i + 1) * bins / n));
        float pk = 0.0f, moved = 0.0f;
        for (std::int64_t b = b0; b < b1; ++b)
        {
            const float pLo = pairs[static_cast<std::size_t> (2 * b)], pHi = pairs[static_cast<std::size_t> (2 * b + 1)];
            float lo = pLo, hi = pHi;
            if (live.read (slot0, static_cast<int> (b), lo, hi))
                moved = std::max (moved, std::abs (lo - pLo) + std::abs (hi - pHi));
            pk = std::max (pk, std::max (std::abs (lo), std::abs (hi)));
        }
        peaks[i] = std::min (1.0f, pk);
        if (dirty != nullptr)
        {
            const float d = std::isfinite (moved) ? moved : kDirtyFullScale;
            dirty[i] = static_cast<std::uint8_t> (std::min (255.0f, std::round (255.0f * d / kDirtyFullScale)));
        }
    }
    return true;
}

void GranulatorInstrumentProcessor::updateSlotDirty (int slot0, const ogi::SlotInfo& info, std::uint64_t published, double nowMs)
{
    // PD7: `slots[i].dirty` is a real difference test (some live bin of the
    // current tape differs from its pristine bin), evaluated only when the
    // head wrote (ovWrites), the tape changed (epoch: bumped by the audio
    // thread when it takes a tape, which invalidates every live bin) or the
    // loader's side changed, and at most every kDirtyMinMs per slot. A key
    // that moved inside the limit stays "moved" until it is evaluated.
    const auto i = static_cast<std::size_t> (slot0);
    const auto& tel = engine->getTelemetry();
    DirtyKey key;
    key.writes = tel.getOvWrites (slot0);
    key.epoch  = tel.overview.epoch (slot0);
    key.tape   = info.generation + published;
    key.loaded = info.loaded;

    if (slotDirtyKeyValid[i] && (key == slotDirtyKey[i] || nowMs - slotDirtyEvalMs[i] < kDirtyMinMs))
        return;

    bool differs = false;
    if (info.loaded)
    {
        const auto pairs = loader->getOverview (slot0);
        const int bins = static_cast<int> (pairs.size() / 2);
        for (int b = 0; b < bins && ! differs; ++b)
        {
            float lo = 0.0f, hi = 0.0f;
            if (tel.overview.read (slot0, b, lo, hi))
                differs = ! juce::exactlyEqual (lo, pairs[static_cast<std::size_t> (2 * b)])
                       || ! juce::exactlyEqual (hi, pairs[static_cast<std::size_t> (2 * b + 1)]);
        }
    }
    slotDirty[i] = differs;
    slotDirtyKey[i] = key;          // the key read BEFORE the bins: a write during the scan re-evaluates
    slotDirtyKeyValid[i] = true;
    slotDirtyEvalMs[i] = nowMs;
}

//==============================================================================
// Stage 3 / 3.2: the telemetry builder (PLAN-3.2 PD2 – PD5, PD9 – PD13).
//==============================================================================
GranulatorInstrumentProcessor::UiTelemetry GranulatorInstrumentProcessor::buildUiTelemetry (double nowMs)
{
    // Two editors may poll (RESEARCH-3.2 §10): one reader at a time on the
    // single-reader frame buffer and on the consuming peak.
    const juce::ScopedLock sl (telemetryLock);

    UiTelemetry t;
    auto& tel = engine->getTelemetry();
    auto& recorder = engine->getRecorder();
    const double fs = engine->getSampleRate();

    const auto raw = [this] (const juce::String& id, float fallback)
    {
        const auto* v = parameters.getRawParameterValue (id);
        return v != nullptr ? v->load (std::memory_order_relaxed) : fallback;
    };

    // ── revisions: the publish count FIRST, then one SlotInfo copy per slot ──
    const std::uint64_t published = handoff.getPublishedCount();
    std::array<ogi::SlotInfo, ogi::kTapeSlots> infos;
    for (int s = 0; s < ogi::kTapeSlots; ++s)
        infos[static_cast<std::size_t> (s)] = loader->getSlotInfo (s);
    for (int s = 0; s < ogi::kTapeSlots; ++s)
        updateSlotDirty (s, infos[static_cast<std::size_t> (s)], published, nowMs);   // before the fingerprint (PD7)

    t.scaleRev = getScaleStateRevision();
    t.slotRev  = slotRevisionFor (infos, published);
    for (int s = 0; s < ogi::kTapeSlots; ++s)
        t.lanes[static_cast<std::size_t> (s)].ovRev = overviewRevFor (s, infos[static_cast<std::size_t> (s)], published);

    // ── the grain frame: THE one read (PD12) ─────────────────────────────────
    const ogi::GiUiFrame& f = tel.frames.read();

    // PD4: stale when seqHead has not moved for kStaleMs on the caller's
    // clock. The first observation counts as a move.
    if (! frameSeqSeen || f.seqHead != frameSeqLast)
    {
        frameSeqSeen = true;
        frameSeqLast = f.seqHead;
        frameSeqMovedMs = nowMs;
    }
    t.stale = nowMs - frameSeqMovedMs >= kStaleMs;

    // Which streams have a tape (the loader's view; the engine writes scan 0
    // for a stream without one, which would draw a playhead at x = 0).
    std::array<bool, ogi::kGiStreams> hasTape {};
    for (int s = 0; s < ogi::kGiStreams; ++s)
        hasTape[static_cast<std::size_t> (s)] = infos[static_cast<std::size_t> (s)].loaded && infos[static_cast<std::size_t> (s)].frames > 0;

    if (! t.stale)
    {
        // PD2: 20 rows; state 1 = counted (note >= 0).
        for (int k = 0; k < UiTelemetry::kVoices; ++k)
        {
            const auto& u = f.voices[k];
            auto& v = t.voices[static_cast<std::size_t> (k)];
            if (u.note < 0)
                continue;   // a free row: state 0, note −1, scans −1
            v.state = 1;
            v.note  = static_cast<int> (u.note);
            v.amp   = std::isfinite (u.amp) ? u.amp : 0.0f;
            for (int s = 0; s < ogi::kGiStreams; ++s)
                if (hasTape[static_cast<std::size_t> (s)] && std::isfinite (u.scan[s]))
                    v.scan[static_cast<std::size_t> (s)] = juce::jlimit (0.0f, 1.0f, u.scan[s]);
        }

        // PD1: the reader-side derivations (parent PluginProcessor.cpp grains).
        // The frame's gainL / gainR are spread pan × gain_rand ONLY (the stream
        // pan is applied on the stream bus, Engine.cpp spawn), so the stream
        // pan is added here, once: the BASE sN_pan parameter (−100…100 → ±1;
        // pan modulation is not followed — the mod telemetry is 3.3).
        std::array<double, ogi::kGiStreams> streamPan {};
        for (int s = 0; s < ogi::kGiStreams; ++s)
            streamPan[static_cast<std::size_t> (s)] = juce::jlimit (-1.0, 1.0, static_cast<double> (raw (Ids::stream (s + 1, "pan"), 0.0f)) / 100.0);

        constexpr double kPanScale = 4.0 / juce::MathConstants<double>::pi;
        const int n = juce::jlimit (0, UiTelemetry::kMaxGrains, static_cast<int> (f.count));
        int out = 0;
        for (int i = 0; i < n; ++i)
        {
            const auto& u = f.grains[i];
            if (u.stream >= ogi::kGiStreams)
                continue;
            const auto stream0 = static_cast<std::size_t> (u.stream);
            auto& g = t.grains[static_cast<std::size_t> (out++)];
            g.stream  = static_cast<int> (u.stream) + 1;   // 1-based on the wire
            g.normPos = std::isfinite (u.normPos) ? u.normPos : 0.0f;
            // panGains inverse: θ = (p + 1) · π / 4. Equal gains (spread 0) are
            // exactly 0 — atan2 (1, 1) · 4 / π − 1 leaves an ulp residue otherwise.
            const double grainPan = juce::exactlyEqual (u.gainL, u.gainR)
                                        ? 0.0
                                        : kPanScale * std::atan2 (static_cast<double> (u.gainR), static_cast<double> (u.gainL)) - 1.0;
            const double pan = juce::jlimit (-1.0, 1.0, grainPan + streamPan[stream0]);
            g.pan  = std::isfinite (pan) ? static_cast<float> (pan) : 0.0f;
            const double level = std::sqrt ((static_cast<double> (u.gainL) * u.gainL + static_cast<double> (u.gainR) * u.gainR) * 0.5);
            g.gain = std::isfinite (level) ? static_cast<float> (level) : 0.0f;
            const float env = engine->grainWindow (static_cast<int> (u.windowStep), u.tiltT, u.tiltA, u.tiltB, static_cast<double> (u.phase)) * u.gain;
            g.env  = std::isfinite (env) ? env : 0.0f;
            g.lengthSec = fs > 0.0 ? static_cast<float> (static_cast<double> (u.lenSamples) / fs) : 0.0f;
            g.voice = static_cast<int> (u.voice);
            g.flags = static_cast<int> (u.flags & 1u);   // reversed only (PD1: the killing bit is never sent)
            if ((u.flags & 2u) == 0)                      // PD3: neither killing nor in a fade slot
                ++t.counts[stream0];
        }
        t.grainCount = out;
    }

    // ── PD5: the feedback head ───────────────────────────────────────────────
    // Shown when the slot has a tape and (the head is not parked, or freeze is
    // on with feedback > 0: the head parks when frozen and the page draws it
    // dashed). Nothing before the engine published its first frame (the
    // telemetry atomics still read 0 = "not parked").
    if (f.seqHead != 0)
    {
        const bool frozenWithFeedback = raw ("freeze", 0.0f) > 0.5f && raw ("feedback", 0.0f) > 0.0f;
        for (int s = 0; s < ogi::kTapeSlots; ++s)
        {
            const auto i = static_cast<std::size_t> (s);
            if (! hasTape[i])
                continue;
            const bool parked = (tel.swapState[i].load (std::memory_order_relaxed) & 4u) != 0;
            if (parked && ! frozenWithFeedback)
                continue;
            const double pos = static_cast<double> (tel.headPos[i].load (std::memory_order_relaxed)) / static_cast<double> (infos[i].frames);
            t.lanes[i].hasFb = true;
            t.lanes[i].fb = static_cast<float> (juce::jlimit (0.0, 1.0, pos));
        }
    }

    // ── PD9 – PD11: Record-in ────────────────────────────────────────────────
    t.rec.bus = getSidechainChannelCount() > 0 ? 1 : 0;
    t.rec.run = recRunHold.update (recorder.getPackedState(), nowMs);

    const int target = recorder.getTargetTelemetry();
    const int taken  = recorder.getFramesTelemetry();
    if (target > 0)
    {
        t.rec.slot = juce::jlimit (0, ogi::kTapeSlots - 1, recorder.getTargetSlotTelemetry()) + 1;
        t.rec.pos  = static_cast<float> (juce::jlimit (0.0, 1.0, static_cast<double> (taken) / static_cast<double> (target)));
        t.rec.len  = fs > 0.0 ? static_cast<float> (static_cast<double> (target) / fs) : 0.0f;
        if (t.rec.run == 2)
        {
            auto& lane = t.lanes[static_cast<std::size_t> (t.rec.slot - 1)];
            lane.hasRec = true;
            lane.rec = t.rec.pos;
        }
    }

    // PD10: THE one consumer of the post-gain sidechain peak.
    float peak = recorder.takeScPeak();
    if (! std::isfinite (peak))
        peak = 0.0f;
    if (peak >= ogi::GiRecorder::kPresenceLin)
    {
        signalSeen = true;
        signalLastMs = nowMs;
    }
    t.rec.signal = signalSeen && nowMs - signalLastMs <= kSignalHoldMs;
    t.rec.peak = peak;

    // PD11: Tempo length mode, not recording, and the take the recorder would
    // start — its OWN length function, at the block's tempo — exceeds F_max.
    if (raw ("sync_mode", 0.0f) > 0.5f && t.rec.run != 2 && fs > 0.0)
    {
        const double b = uiBpm.load (std::memory_order_relaxed);
        const double bpm = (b > 0.0 && std::isfinite (b)) ? b : ogi::kTempoFallbackBpm;
        const double pps = bpm / 60.0 / fs;   // TempoTracker::update's expression
        const auto len = ogi::GiRecorder::takeLengthFrames (true, static_cast<int> (std::lround (raw ("rec_length_sync", 2.0f))),
                                                            uiBarQuarters.load (std::memory_order_relaxed), pps, 0.0, fs);
        t.rec.capped = len > static_cast<std::int64_t> (ogi::GiRecorder::maxFramesFor (fs));
    }

    return t;
}

//==============================================================================
// Stage 3 / 3.2: slot actions (PLAN-3.2 PD14, PD16).
//==============================================================================
juce::String GranulatorInstrumentProcessor::clearSlot (int slot0)
{
    if (slot0 < 0 || slot0 >= ogi::kTapeSlots)
        return "badSlot";

    // A commit job arrives after the clear and would refill the lane: refuse
    // while a take is running or being copied out into THIS slot. Armed is
    // allowed (the take then lands in an empty lane).
    const auto& recorder = engine->getRecorder();
    const auto state = static_cast<ogi::RecState> (recorder.getPackedState() & 0xffu);
    if ((state == ogi::RecState::Recording || state == ogi::RecState::Committing)
        && recorder.getTargetSlotTelemetry() == slot0)
        return "recording";

    return loader->requestClear (slot0) == ogi::SlotLoader::ClearResult::badSlot ? juce::String ("badSlot") : juce::String();
}

bool GranulatorInstrumentProcessor::importDroppedFile (int slot0, const juce::File& tempFile, const juce::String& displayName)
{
    return loader->requestImport (slot0, tempFile, displayName);
}

//==============================================================================
// Stage 3 / 3.2: Save recording (PLAN-3.2 PD19, D46) over the engine's
// GiTapeSnapshot (PD18).
//
// THREADS. saveRecordingTo runs on the message thread (guards, then one queued
// SaveJob). ONE worker thread (SaveWorker, started at the first save, joined
// first in the destructor) runs the job: allocate → request → wait → write.
// The audio thread copies the tape (Engine::serviceTapeSnapshot). prepareToPlay
// / releaseResources run on a host thread with the callback stopped.
//
// LOCK. saveLock guards saveJob / saveInFlight / saveLent / saveBufL / saveBufR
// / saveDone and EVERY owner call into the snapshot (request, cancel, release,
// resetAudioStopped). The audio thread never takes it.
//
// BUFFER LIFETIME (the invariant of GiTapeSnapshot.h). saveBufL / R are freed
// at exactly four places, each under saveLock:
//   1. after release() returned true           (rule (a): the audio thread
//      wrote a terminal state as its last access) — reclaimSaveBufferLocked
//      and the Done / Aborted paths of runSaveJob;
//   2. prepareToPlay / releaseResources, ONLY when saveLent (no job running,
//      so no other owner thread is in the snapshot) — resetAudioStopped()
//      first (rule (b));
//   3. the destructor, after the worker is joined — resetAudioStopped() first
//      (rule (b));
//   4. a request() that returned false (nothing was retained).
// Never on a timeout. Every exit of runSaveJob:
//   allocation failed        nothing lent                     → writeFailed
//   request() false          4.                               → busy
//   Done                     release() → write from the buffer → free
//                                                             → ok | writeFailed | cancelled (exit during the write)
//   Aborted                  release() → free                 → changed
//   no progress for 2000 ms  cancel(); it returned false (the audio thread
//                            finished first) → the Done / Aborted path;
//                            else wait ≤ 100 ms for the acknowledgement:
//                            release() true → free; false → saveLent = true,
//                            the buffer STAYS (audio may resume and write the
//                            chunk it was in, then acknowledge)  → timeout
//   worker told to exit      cancel(); terminal → release() → free; else
//                            saveLent = true (the destructor frees under
//                            rule (b))                         → cancelled
// While saveLent, saveRecordingTo answers `busy` unless release() succeeds at
// that moment; the worker's idle loop also retries release() every 100 ms, so
// the buffer goes soon after audio resumes. saveInFlight stays true from the
// queueing until the result is recorded, so only one job ever owns the buffer.
//==============================================================================
namespace
{
    juce::String slotSourcePath (const juce::String& source)
    {
        if (source.startsWith ("file:") || source.startsWith ("rec:"))
            return source.fromFirstOccurrenceOf (":", false, false);
        return {};
    }
}

bool GranulatorInstrumentProcessor::reclaimSaveBufferLocked() noexcept
{
    if (! saveLent)
        return true;
    if (! engine->tapeSnapshot().release())   // not terminal: the audio thread still owes its acknowledgement
        return false;
    saveBufL.free();
    saveBufR.free();
    saveLent = false;
    return true;
}

void GranulatorInstrumentProcessor::reclaimSaveBufferAudioStopped() noexcept
{
    const juce::ScopedLock sl (saveLock);
    if (! saveLent)
        return;   // nothing lent, or a job in flight owns the request (left alone)
    engine->tapeSnapshot().resetAudioStopped();
    saveBufL.free();
    saveBufR.free();
    saveLent = false;
}

void GranulatorInstrumentProcessor::saveRecordingTo (int slot0, const juce::File& file, SaveCompletion completion)
{
    const auto refuse = [&completion] (const char* error)
    {
        SaveResult r;
        r.error = error;
        if (completion)
            completion (r);
    };

    // Guards, in order (PD19).
    if (slot0 < 0 || slot0 >= ogi::kTapeSlots)
        return refuse ("notRecording");
    const auto info = loader->getSlotInfo (slot0);
    if (! info.source.startsWith ("rec:"))
        return refuse ("notRecording");
    if (! info.loaded || info.frames <= 0 || ! (info.tapeRate > 0.0))
        return refuse ("empty");

    {
        const juce::ScopedLock sl (saveLock);
        if (saveInFlight || ! reclaimSaveBufferLocked())
            return refuse ("busy");
    }

    const auto target = file.withFileExtension ("wav");
    for (int s = 0; s < ogi::kTapeSlots; ++s)
    {
        const auto path = slotSourcePath (s == slot0 ? info.source : loader->getSlotInfo (s).source);
        if (path.isNotEmpty() && juce::File::isAbsolutePath (path) && juce::File (path) == target)
            return refuse ("inUse");
    }

    {
        // The message thread is the only caller, so nothing was queued since
        // the busy check above.
        const juce::ScopedLock sl (saveLock);
        saveInFlight = true;
        saveJob = std::make_unique<SaveJob>();
        saveJob->slot0 = slot0;
        saveJob->file = target;
        saveJob->frames = info.frames;
        saveJob->rate = info.tapeRate;
        saveJob->completion = std::move (completion);
    }

    if (saveWorker == nullptr)
    {
        saveWorker = std::make_unique<SaveWorker> (*this);
        saveWorker->startThread();
    }
    saveWorker->notify();
}

void GranulatorInstrumentProcessor::saveWorkerRun()
{
    while (! juce::Thread::currentThreadShouldExit())
    {
        std::unique_ptr<SaveJob> job;
        bool lent = false;
        {
            const juce::ScopedLock sl (saveLock);
            job = std::move (saveJob);
            if (job == nullptr)
                lent = ! reclaimSaveBufferLocked();   // a timed-out save: free once the audio thread acknowledged
        }

        if (job == nullptr)
        {
            saveWorker->wait (lent ? 100 : -1);   // notify() from saveRecordingTo / signalThreadShouldExit
            continue;
        }

        const auto result = runSaveJob (*job);
        finishSave (std::move (job->completion), result);
    }
}

GranulatorInstrumentProcessor::SaveResult GranulatorInstrumentProcessor::runSaveJob (const SaveJob& job)
{
    using Snap = ogi::GiTapeSnapshot;
    auto& snap = engine->tapeSnapshot();
    const auto fail = [] (const char* error)
    {
        SaveResult r;
        r.error = error;
        return r;
    };

    {
        // saveBufL / R are free here: saveRecordingTo queued this job only
        // after reclaimSaveBufferLocked() returned true, and saveInFlight has
        // kept every other owner out since.
        const juce::ScopedLock sl (saveLock);
        saveBufL.malloc (static_cast<std::size_t> (job.frames));
        saveBufR.malloc (static_cast<std::size_t> (job.frames));
        if (saveBufL.get() == nullptr || saveBufR.get() == nullptr)
        {
            saveBufL.free();
            saveBufR.free();
            return fail ("writeFailed");
        }
        if (! snap.request (job.slot0, saveBufL.get(), saveBufR.get(), job.frames))
        {
            saveBufL.free();   // nothing was retained by the snapshot
            saveBufR.free();
            return fail ("busy");
        }
    }

    // The buffers are the audio thread's until a terminal state + release().
    const auto releaseAndFree = [this, &snap]
    {
        const juce::ScopedLock sl (saveLock);
        const bool released = snap.release();
        jassertquiet (released);
        if (released)
        {
            saveBufL.free();
            saveBufR.free();
        }
        else
        {
            saveLent = true;   // cannot happen from a terminal state; never free without the release
        }
    };
    const auto finishDone = [this, &snap, &job, &fail]
    {
        {
            const juce::ScopedLock sl (saveLock);
            const bool released = snap.release();   // Done → Idle: the buffers are ours, and stay ours
            jassertquiet (released);                // (saveInFlight keeps every other owner out)
            if (! released)
            {
                saveLent = true;
                return fail ("writeFailed");
            }
        }
        const auto error = writeSaveWav (job);
        {
            const juce::ScopedLock sl (saveLock);
            saveBufL.free();
            saveBufR.free();
        }
        SaveResult r;
        r.ok = error.isEmpty();
        r.error = error;
        if (r.ok)
            r.path = job.file.getFullPathName();
        return r;
    };

    std::uint32_t lastProgress = snap.progress();
    double lastMoveMs = juce::Time::getMillisecondCounterHiRes();

    for (;;)
    {
        const auto st = snap.state();
        if (st == Snap::Done)
            return finishDone();
        if (st == Snap::Aborted)
        {
            releaseAndFree();
            return fail ("changed");
        }

        const bool exiting = juce::Thread::currentThreadShouldExit();
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const auto progress = snap.progress();
        if (progress != lastProgress)
        {
            lastProgress = progress;
            lastMoveMs = nowMs;
        }

        if (exiting || nowMs - lastMoveMs >= kSaveNoProgressMs)
        {
            bool owed;
            {
                const juce::ScopedLock sl (saveLock);
                owed = snap.cancel();   // true: the audio thread now owes a Cancelled
            }
            if (! owed)
            {
                // The audio thread reached a terminal state before the cancel.
                if (snap.state() == Snap::Done && ! exiting)
                    return finishDone();
                const bool changed = snap.state() == Snap::Aborted;
                releaseAndFree();
                return fail (exiting ? "cancelled" : (changed ? "changed" : "timeout"));
            }

            // Bounded wait for the acknowledgement (none while the host idles
            // the callback, and none at all once the worker must exit).
            const double until = nowMs + 100.0;
            while (! exiting && ! Snap::isTerminal (snap.state()) && juce::Time::getMillisecondCounterHiRes() < until
                   && ! juce::Thread::currentThreadShouldExit())
                juce::Thread::sleep (2);
            {
                const juce::ScopedLock sl (saveLock);
                if (snap.release())
                {
                    saveBufL.free();
                    saveBufR.free();
                }
                else
                {
                    saveLent = true;   // NOT freed: the audio thread may still write into it when it resumes
                }
            }
            return fail (exiting ? "cancelled" : "timeout");
        }

        saveWorker->wait (2);
    }
}

juce::String GranulatorInstrumentProcessor::writeSaveWav (const SaveJob& job)
{
    // Worker thread; the snapshot is Idle and saveBufL / R hold [0, frames).
    // 24-bit stereo at the tape rate, through a ".part" sibling, then renamed
    // over the target (the chooser already asked about overwriting).
    const auto part = job.file.getSiblingFile (job.file.getFileName() + ".part");
    job.file.getParentDirectory().createDirectory();
    part.deleteFile();

    juce::String error;
    {
        auto fos = std::make_unique<juce::FileOutputStream> (part);
        if (fos->failedToOpen())
            return "writeFailed";
        std::unique_ptr<juce::OutputStream> os (std::move (fos));
        juce::WavAudioFormat wav;
        const auto opts = juce::AudioFormatWriterOptions{}
                              .withSampleRate (job.rate)
                              .withNumChannels (2)
                              .withBitsPerSample (24);
        auto w = wav.createWriterFor (os, opts);
        if (w == nullptr)
        {
            os.reset();
            part.deleteFile();
            return "writeFailed";
        }

        // Samples beyond ±1.0 clip (PD19, R§12.8): clamped here, explicitly.
        constexpr int kChunk = 32768;
        juce::HeapBlock<float> cl (kChunk), cr (kChunk);
        const float* const chans[2] = { cl.get(), cr.get() };
        for (int pos = 0; pos < job.frames && error.isEmpty(); pos += kChunk)
        {
            if (juce::Thread::currentThreadShouldExit())
            {
                error = "cancelled";
                break;
            }
            const int n = std::min (kChunk, job.frames - pos);
            for (int i = 0; i < n; ++i)
            {
                const float l = saveBufL[pos + i], r = saveBufR[pos + i];
                cl[i] = std::isfinite (l) ? juce::jlimit (-1.0f, 1.0f, l) : 0.0f;
                cr[i] = std::isfinite (r) ? juce::jlimit (-1.0f, 1.0f, r) : 0.0f;
            }
            if (! w->writeFromFloatArrays (chans, 2, n))
                error = "writeFailed";
        }
        if (error.isEmpty() && ! w->flush())
            error = "writeFailed";
        w.reset();   // closes the file (header length written)
    }

    // 6 bytes per frame after the header: a short file is a failed write.
    if (error.isEmpty() && part.getSize() < static_cast<juce::int64> (job.frames) * 6)
        error = "writeFailed";
    if (error.isEmpty() && ! part.moveFileTo (job.file))
        error = "writeFailed";
    if (error.isNotEmpty())
        part.deleteFile();
    return error;
}

void GranulatorInstrumentProcessor::finishSave (SaveCompletion completion, const SaveResult& result)
{
    {
        const juce::ScopedLock sl (saveLock);
        saveDone.emplace_back (std::move (completion), result);
        saveInFlight = false;
    }

    // To the message thread. Without a MessageManager (a console) the worker
    // delivers; the destructor delivers whatever is left (the flag keeps a
    // late message off a dead processor).
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
    {
        juce::MessageManager::callAsync ([alive = saveAlive, this]
        {
            if (alive->load (std::memory_order_acquire))
                deliverSaveCompletions();
        });
    }
    else
    {
        deliverSaveCompletions();
    }
}

void GranulatorInstrumentProcessor::deliverSaveCompletions()
{
    std::vector<std::pair<SaveCompletion, SaveResult>> done;
    {
        const juce::ScopedLock sl (saveLock);
        done.swap (saveDone);
    }
    for (auto& d : done)
        if (d.first)
            d.first (d.second);
}

void GranulatorInstrumentProcessor::getWindowCurve (int stream0, float* out, int n) const noexcept
{
    if (out == nullptr || n <= 0)
        return;
    const int stream = juce::jlimit (0, Ids::kStreams - 1, stream0) + 1;
    const auto* w  = parameters.getRawParameterValue (Ids::stream (stream, "window"));
    const auto* sk = parameters.getRawParameterValue (Ids::stream (stream, "window_skew"));
    engine->windowCurve (w  != nullptr ? w->load (std::memory_order_relaxed)  : 0.0f,
                         sk != nullptr ? sk->load (std::memory_order_relaxed) : 0.0f,
                         out, n);
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

bool GranulatorInstrumentProcessor::commitSync()
{
    auto take = std::make_shared<ogi::SlotLoader::Pristine>();
    ogi::RecCommit c;
    if (! engine->getRecorder().takeCommit (take->pcm, c))
        return false;
    take->rate = c.rate;
    juce::String error;
    return loader->commitSync (c.slot, std::move (take), error);
}

GranulatorInstrumentProcessor::SaveResult GranulatorInstrumentProcessor::saveRecordingSync (int slot0, const juce::File& file,
                                                                                            const std::function<void()>& pump)
{
    // The harness has a MessageManager but no running loop: the callAsync of
    // finishSave never dispatches, so this loop delivers on the calling thread.
    struct Box
    {
        std::atomic<bool> done { false };
        SaveResult result;
    };
    auto box = std::make_shared<Box>();
    saveRecordingTo (slot0, file, [box] (const SaveResult& r)
    {
        box->result = r;
        box->done.store (true, std::memory_order_release);
    });
    while (! box->done.load (std::memory_order_acquire))
    {
        if (pump)
        {
            pump();
            juce::Thread::yield();
        }
        else
        {
            juce::Thread::sleep (1);
        }
        deliverSaveCompletions();
    }
    return box->result;
}

bool GranulatorInstrumentProcessor::saveBufferHeldForTest() const
{
    const juce::ScopedLock sl (saveLock);
    return saveBufL.get() != nullptr || saveBufR.get() != nullptr;
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
