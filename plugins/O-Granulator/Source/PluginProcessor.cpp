/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"      // GranulatorEditor — plugin target only (consoles build JUCE_WEB_BROWSER=0)
#endif

namespace Ids = GranulatorParamIds;

//==============================================================================
GranulatorProcessor::GranulatorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OGranulator"), createParameterLayout())
{
    // Per-instance seed; a restored session overrides it (setStateInformation).
    seed.store (static_cast<uint64_t> (juce::Random::getSystemRandom().nextInt64()), std::memory_order_relaxed);

    // builtIns() is a function-local static: build it here, never first on the audio thread.
    (void) PitchCollections::builtIns();

    for (auto& m : degreeMasks)
        m.store (kAllOn, std::memory_order_relaxed);
    {
        juce::String error, badToken;
        const bool ok = PitchCollections::tableFromText (defaultCustomText(), customTable, error, badToken);
        jassertquiet (ok);
        customText = defaultCustomText();
    }

    // Engine parameter pointers (raw denormalised atomics, cached once).
    // Phase 2.2: the five globals plus the 19 DSP parameters of every stream
    // (2.1's set + mute, solo, scale, window, window_skew, stereo_spread, gain_rand).
    // Phase 2.3: + the 10 per-grain FX parameters of every stream.
    {
        auto raw = [this] (const juce::String& id)
        {
            auto* p = parameters.getRawParameterValue (id);
            jassert (p != nullptr);
            return p;
        };

        ogran::ParamPointers p;
        p.inputGain    = raw ("input_gain");
        p.bufferLength = raw ("buffer_length");
        p.mix          = raw ("mix");
        p.outputGain   = raw ("output_gain");
        p.grainLimit   = raw ("grain_limit");
        // Phase 2.4: freeze, the feedback network and the output reverb.
        p.freeze       = raw ("freeze");
        p.feedback     = raw ("feedback");
        p.fbLowCut     = raw ("fb_low_cut");
        p.fbHighCut    = raw ("fb_high_cut");
        p.fbSaturation = raw ("fb_saturation");
        p.fbDiffusion  = raw ("fb_diffusion");
        p.reverbMix    = raw ("reverb_mix");
        p.reverbSize   = raw ("reverb_size");
        p.reverbDamp   = raw ("reverb_damp");

        for (int n = 1; n <= Ids::kStreams; ++n)
        {
            auto& s = p.s[n - 1];
            s.enable      = raw (Ids::stream (n, "enable"));
            s.level       = raw (Ids::stream (n, "level"));
            s.pan         = raw (Ids::stream (n, "pan"));
            s.position    = raw (Ids::stream (n, "position"));
            s.spray       = raw (Ids::stream (n, "spray"));
            s.size        = raw (Ids::stream (n, "size"));
            s.sizeRand    = raw (Ids::stream (n, "size_rand"));
            s.density     = raw (Ids::stream (n, "density"));
            s.jitter      = raw (Ids::stream (n, "jitter"));
            s.pitch       = raw (Ids::stream (n, "pitch"));
            s.pitchRand   = raw (Ids::stream (n, "pitch_rand"));
            s.reverseProb = raw (Ids::stream (n, "reverse_prob"));
            s.mute        = raw (Ids::stream (n, "mute"));
            s.solo        = raw (Ids::stream (n, "solo"));
            s.scale       = raw (Ids::stream (n, "scale"));
            s.window      = raw (Ids::stream (n, "window"));
            s.windowSkew  = raw (Ids::stream (n, "window_skew"));
            s.spread      = raw (Ids::stream (n, "stereo_spread"));
            s.gainRand    = raw (Ids::stream (n, "gain_rand"));
            // 2.3 per-grain FX
            s.filterType   = raw (Ids::stream (n, "filter_type"));
            s.filterCutoff = raw (Ids::stream (n, "filter_cutoff"));
            s.filterRand   = raw (Ids::stream (n, "filter_rand"));
            s.filterRes    = raw (Ids::stream (n, "filter_res"));
            s.drive        = raw (Ids::stream (n, "drive"));
            s.crush        = raw (Ids::stream (n, "crush"));
            s.crushProb    = raw (Ids::stream (n, "crush_prob"));
            s.ringFreq     = raw (Ids::stream (n, "ring_freq"));
            s.ringRand     = raw (Ids::stream (n, "ring_rand"));
            s.ringMix      = raw (Ids::stream (n, "ring_mix"));
            // 2.5a tempo sync
            s.sizeSync     = raw (Ids::stream (n, "size_sync"));
            s.densitySync  = raw (Ids::stream (n, "density_sync"));
            // 2.5b onset trigger + pitch follow (PLAN-2.5b P73)
            s.onsetTrigger = raw (Ids::stream (n, "onset_trigger"));
            s.follow       = raw (Ids::pitchFollow (n));
        }
        // Phase 2.5b: scale root (absolute follow, R70) and the onset threshold.
        p.scaleRoot      = raw ("scale_root");
        p.onsetThreshold = raw ("onset_threshold");

        // Phase 2.5a: sync mode, LFO / env / macro sources, the 16-slot matrix.
        p.syncMode = raw ("sync_mode");
        for (int n = 1; n <= Ids::kLfos; ++n)
        {
            auto& l = p.lfo[n - 1];
            l.shape    = raw (Ids::lfo (n, "shape"));
            l.rate     = raw (Ids::lfo (n, "rate"));
            l.rateSync = raw (Ids::lfo (n, "rate_sync"));
            l.phase    = raw (Ids::lfo (n, "phase"));
        }
        p.envAttack  = raw ("env_attack");
        p.envRelease = raw ("env_release");
        p.envGain    = raw ("env_gain");
        for (int n = 1; n <= Ids::kMacros; ++n)
            p.macro[n - 1] = raw (Ids::macro (n));
        for (int m = 1; m <= Ids::kModSlots; ++m)
        {
            auto& md = p.mod[m - 1];
            md.source = raw (Ids::mod (m, "source"));
            md.dest   = raw (Ids::mod (m, "dest"));
            md.depth  = raw (Ids::mod (m, "depth"));
        }
        static_assert (Ids::kLfos == ogran::kLfos && Ids::kMacros == ogran::kMacros && Ids::kModSlots == ogran::kModSlots
                       && Ids::kModDests == ogran::kModDests, "engine 2.5a sizes must match the parameter contract");

        // P11: the engine reads masks (relaxed) and copies the custom table under
        // a non-blocking try-lock when customVersion moves. Index layouts agree:
        static_assert (maskIndex (1, 0) == ogran::scaleMaskIndex (0, 0)
                       && maskIndex (4, 31) == ogran::scaleMaskIndex (3, 31)
                       && Ids::kNumCollections == ogran::kNumScaleChoices
                       && Ids::kStreams == ogran::kMaxStreams
                       && Ids::scaleChoiceToId (5) == ogran::scaleChoiceToId (5)
                       && Ids::scaleChoiceToId (6) == ogran::scaleChoiceToId (6)
                       && Ids::scaleChoiceToId (31) == ogran::scaleChoiceToId (31),
                       "engine scale indexing must match the processor's mask / choice layout");
        ogran::ScaleSource scale;
        scale.masks         = degreeMasks.data();
        scale.lock          = &customLock;
        scale.customTable   = &customTable;
        scale.customVersion = &customVersion;

        engine.bind (p, scale);

        // Phase 2.5a: the matrix range table from the parameter objects themselves
        // (the same contract rows as createParameterLayout — R46 parity, U9c) and
        // the raw atomic of every destination (message thread, once).
        for (int d = 1; d < Ids::kModDests; ++d)
        {
            const juce::String id (Ids::kModDestIds[d]);
            auto* f = dynamic_cast<juce::AudioParameterFloat*> (parameters.getParameter (id));
            jassert (f != nullptr);
            if (f != nullptr)
                engine.setModDestination (d, f->getNormalisableRange(), raw (id));
        }
    }

    // Stage 3 (PLAN-3.1 P100): preset FILES. Sign-offs: the seed travels in
    // customState (a preset reproduces its texture); output_gain is the user's
    // trim — never written by a preset, never reset by a load
    // (feedback_presets_never_set_output_gain). No directory I/O here: the
    // manager creates its folders lazily (auval).
    presetManager.setCustomStateCallbacks ([this] { return presetCustomState(); },
                                           [this] (const juce::var& v) { applyPresetCustomState (v); });
    presetManager.setExcludedParameters ({ "output_gain" });
    presetManager.setValidatedApply (true);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GranulatorProcessor::createParameterLayout()
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
                // Interval 0 everywhere; plain skew, or setSkewForCentre for the
                // log-like rows. Never a lambda range (the WebView slider frontend
                // sees only start / end / skew / interval).
                juce::NormalisableRange<float> range (d.start, d.end, 0.0f, d.skew);
                if (d.centre > 0.0f)
                    range.setSkewForCentre (d.centre);
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, e.name, range, d.def));
                break;
            }
            case Kind::Choice:
            {
                const auto& choices = d.choices != nullptr ? *d.choices : Ids::modDestChoices();
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, e.name, choices, static_cast<int> (d.def)));
                break;
            }
            case Kind::Bool:
            {
                // s1_enable defaults On; s2…s4 Off.
                const bool def = juce::String (d.id) == "enable" ? (e.instance == 1) : (d.def != 0.0f);
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, e.name, def));
                break;
            }
        }
    }
    return layout;
}

//==============================================================================
void GranulatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // JUCE 8: getLatencySamples() is non-virtual — report through the setter.
    setLatencySamples (0);

    // Decide from the NEGOTIATED layout, never from buffer width (a host may
    // hand a mono-out bus a 2-channel buffer).
    busInChannels  = getMainBusNumInputChannels();
    busOutChannels = getMainBusNumOutputChannels();
    monoToStereo   = busInChannels == 1 && busOutChannels == 2;

    // Message thread: allocates the 41 s ring (D2) and resets every clock/ramp.
    engine.prepare (sampleRate, samplesPerBlock);
}

bool GranulatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in.isDisabled() || out.isDisabled())
        return false;

    const auto mono = juce::AudioChannelSet::mono(), stereo = juce::AudioChannelSet::stereo();
    const bool inOk  = in  == mono || in  == stereo;
    const bool outOk = out == mono || out == stereo;
    return inOk && outOk && in.size() <= out.size();   // mono→mono, mono→stereo, stereo→stereo
}

void GranulatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Every loop is bounded by the buffer's own channel count (Standalone can
    // hand 3–7 channel buffers); the engine's channel counts come from the BUS.
    const int numCh  = buffer.getNumChannels();
    const int n      = buffer.getNumSamples();
    const int numIn  = juce::jmin (busInChannels,  numCh);
    const int numOut = juce::jmin (busOutChannels, numCh);

    if (n <= 0)
        return;

    if (numIn < 1 || numOut < 1 || ! engine.isPrepared())
    {
        // Not prepared / degenerate layout: Stage 1 pass-through.
        for (int ch = juce::jmax (numIn, 0); ch < numCh; ++ch)
        {
            if (monoToStereo && ch == 1 && numIn == 1)
                buffer.copyFrom (1, 0, buffer, 0, 0, n);   // mono input feeds both outputs
            else
                buffer.clear (ch, 0, n);
        }
        return;
    }

    const float* in[2] = { buffer.getReadPointer (0), buffer.getReadPointer (numIn > 1 ? 1 : 0) };
    float* out[2]      = { buffer.getWritePointer (0), numOut > 1 ? buffer.getWritePointer (1) : nullptr };

    // 2.5a: the host transport, read ONCE per block (by value — no allocation).
    // The only playhead read in the plugin (PLAN-2.5a T12).
    ogran::TransportSnapshot ts;
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

    // The seed is adopted by the engine at its next grid point.
    engine.process (in, numIn, out, numOut, n, getSeed(), ts);

    // 3.3 (P130, R128): the out meter — the final output, channels [0, numOut);
    // mono out → R = L. Bypass never reaches here (the meters then read 0).
    {
        const auto peakOf = [&buffer, n] (int ch)
        {
            const auto r = juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (ch), n);
            return juce::jmax (-r.getStart(), r.getEnd());
        };
        const float pL = peakOf (0);
        ogran::pushMax (outPkL, pL);
        ogran::pushMax (outPkR, numOut > 1 ? peakOf (1) : pL);
    }

    for (int ch = numOut; ch < numCh; ++ch)
        buffer.clear (ch, 0, n);
}

juce::AudioProcessorEditor* GranulatorProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new GranulatorEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // headless consoles
#endif
}

//==============================================================================
// Scale state
//==============================================================================
const juce::String& GranulatorProcessor::defaultCustomText()
{
    static const juce::String t ("100 200 300 400 500 600 700 800 900 1000 1100 1200");
    return t;
}

GranulatorProcessor::CustomScale GranulatorProcessor::getCustomScale() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return { customName, customText, customTable.period, customVersion.load (std::memory_order_relaxed) };
}

PitchCollections::Table GranulatorProcessor::getCustomTable() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return customTable;
}

juce::String GranulatorProcessor::setCustomScale (const juce::String& name, const juce::String& text, juce::String& badToken)
{
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
    bumpScaleRevision();
    return {};
}

const PitchCollections::Table* GranulatorProcessor::getCollectionTable (int choice) const
{
    const int id = Ids::scaleChoiceToId (choice);
    if (id < 0 || choice >= Ids::kNumCollections)
        return nullptr;
    if (id == PitchCollections::Custom)
        return &customTable;   // message thread: setCustomScale runs there too
    return &PitchCollections::builtIns()[static_cast<size_t> (id)];
}

int GranulatorProcessor::degreeCountFor (int collection) const
{
    if (collection == PitchCollections::Custom)
    {
        const juce::SpinLock::ScopedLockType sl (customLock);
        return customTable.count;
    }
    const auto* t = getCollectionTable (collection);
    return t != nullptr ? t->count : 64;
}

uint64_t GranulatorProcessor::getDegreeMask (int stream, int collection) const noexcept
{
    if (stream < 1 || stream > Ids::kStreams || collection < 1 || collection >= Ids::kNumCollections)
        return kAllOn;
    return degreeMasks[static_cast<size_t> (maskIndex (stream, collection))].load (std::memory_order_relaxed);
}

void GranulatorProcessor::setDegreeMask (int stream, int collection, uint64_t mask) noexcept
{
    if (stream < 1 || stream > Ids::kStreams || collection < 1 || collection >= Ids::kNumCollections)
        return;
    degreeMasks[static_cast<size_t> (maskIndex (stream, collection))]
        .store (canonicalMask (mask, degreeCountFor (collection)), std::memory_order_relaxed);
    bumpScaleRevision();
}

void GranulatorProcessor::resetNonParameterState()
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
//   <OGranulator uiLanguage="fr" …PARAM children…>
//     <Engine seed="-1234" stateVersion="1" modDestTableVersion="1"/>
//     <CustomScale name="…" text="edo 19" period="1200" version="3"/>
//     <DegreeMasks> <Mask stream="1" collection="19" bits="8000000000000001"/> … </DegreeMasks>
//   </OGranulator>
//
// - Written onto the copyState() COPY; the live tree is never mutated from a host thread.
// - No `id` attribute on custom children: APVTS treats any child with an `id`
//   as a parameter.
// - seed as an int64 STRING (getLargeIntValue clamps uint64 > INT64_MAX); masks
//   as 16 hex digits, only non-default (≠ all-on) ones written.
// - Every property comes back from XML as a string var: isVoid() guards, then parse.
//==============================================================================
namespace
{
    const juce::Identifier kEngine ("Engine"), kCustomScale ("CustomScale"), kDegreeMasks ("DegreeMasks"), kMask ("Mask");

    void removeCustomChildren (juce::ValueTree& tree)
    {
        for (int i = tree.getNumChildren(); --i >= 0;)
        {
            const auto type = tree.getChild (i).getType();
            if (type == kEngine || type == kCustomScale || type == kDegreeMasks)
                tree.removeChild (i, nullptr);
        }
    }
}

void GranulatorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    removeCustomChildren (state);   // never duplicate children restored into the live tree

    state.setProperty ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_relaxed)), nullptr);

    juce::ValueTree eng (kEngine);
    eng.setProperty ("seed", juce::String (static_cast<juce::int64> (getSeed())), nullptr);
    eng.setProperty ("stateVersion", Ids::kStateVersion, nullptr);
    eng.setProperty ("modDestTableVersion", Ids::kModDestTableVersion, nullptr);
    state.appendChild (eng, nullptr);

    const auto custom = getCustomScale();
    juce::ValueTree cs (kCustomScale);
    cs.setProperty ("name", custom.name, nullptr);
    cs.setProperty ("text", custom.text, nullptr);
    cs.setProperty ("period", custom.period, nullptr);
    cs.setProperty ("version", static_cast<int> (custom.version & 0x7fffffffu), nullptr);
    state.appendChild (cs, nullptr);

    juce::ValueTree dm (kDegreeMasks);
    for (int s = 1; s <= Ids::kStreams; ++s)
        for (int c = 1; c < Ids::kNumCollections; ++c)
        {
            const uint64_t mask = getDegreeMask (s, c);
            if (mask != kAllOn)
                dm.appendChild (juce::ValueTree (kMask, { { "stream", s }, { "collection", c },
                                                          { "bits", juce::String::toHexString (static_cast<juce::int64> (mask)).paddedLeft ('0', 16) } }),
                                nullptr);
        }
    state.appendChild (dm, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GranulatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    const auto eng  = tree.getChildWithName (kEngine);
    const auto cs   = tree.getChildWithName (kCustomScale);
    const auto dm   = tree.getChildWithName (kDegreeMasks);
    const auto lang = tree.getProperty ("uiLanguage");
    removeCustomChildren (tree);
    parameters.replaceState (tree);

    if (! lang.isVoid())
    {
        const auto s = lang.toString().trim();
        uiLanguage.store (s.isNotEmpty() && s.containsOnly ("0123456789") ? juce::jlimit (0, 2, s.getIntValue())
                                                                          : languageIndex (s),
                          std::memory_order_relaxed);
    }

    // The XML children adapted into the shared apply path's inputs (P98): the
    // property vars are passed through untouched (XML gives strings), so the
    // parse below is the Stage 1 codec's, byte for byte.
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
            if (! m.hasType (kMask)) continue;
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

//==============================================================================
// Non-parameter apply (P98) — ONE implementation for the session codec and
// the preset customLoad.
//==============================================================================
void GranulatorProcessor::applyNonParameterState (const juce::var& custom, const juce::var& masks, const juce::var& seedVar)
{
    // Defaults FIRST, so anything absent reads as default rather than as
    // whatever this instance held. The seed is not part of the reset (there is
    // no default seed): absent keeps the live one.
    resetNonParameterState();

    if (! seedVar.isVoid() && seedVar.toString().isNotEmpty() && seedVar.toString().containsOnly ("-0123456789"))
        setSeed (static_cast<uint64_t> (seedVar.toString().getLargeIntValue()));

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
            if (o == nullptr) continue;
            const auto sv = o->getProperty ("stream"), cv = o->getProperty ("collection"), bv = o->getProperty ("bits");
            if (sv.isVoid() || cv.isVoid() || bv.isVoid()) continue;
            const auto bits = bv.toString();
            if (bits.length() != 16 || ! bits.containsOnly ("0123456789abcdefABCDEF")) continue;
            setDegreeMask (static_cast<int> (sv), static_cast<int> (cv), static_cast<uint64_t> (bits.getHexValue64()));
        }

    bumpScaleRevision();
}

//==============================================================================
// Presets (P99 / P100). Same encodings as the session codec: seed as an int64
// decimal STRING (a JSON number is a double — it loses bits above 2^53 — and
// getLargeIntValue clamps uint64 > INT64_MAX), masks as 16 hex digits, only
// non-default ones written.
//==============================================================================
juce::var GranulatorProcessor::presetCustomState() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("seed", juce::String (static_cast<juce::int64> (getSeed())));

    const auto c = getCustomScale();
    auto* cs = new juce::DynamicObject();
    cs->setProperty ("name", c.name);
    cs->setProperty ("text", c.text);
    root->setProperty ("custom", juce::var (cs));

    juce::Array<juce::var> list;
    for (int s = 1; s <= Ids::kStreams; ++s)
        for (int col = 1; col < Ids::kNumCollections; ++col)
        {
            const uint64_t mask = getDegreeMask (s, col);
            if (mask == kAllOn) continue;
            auto* o = new juce::DynamicObject();
            o->setProperty ("stream", s);
            o->setProperty ("collection", col);
            o->setProperty ("bits", juce::String::toHexString (static_cast<juce::int64> (mask)).paddedLeft ('0', 16));
            list.add (juce::var (o));
        }
    root->setProperty ("masks", list);
    return juce::var (root);
}

void GranulatorProcessor::applyPresetCustomState (const juce::var& customState)
{
    // P99: only an int64 decimal STRING is a seed; anything else counts as absent.
    juce::var custom, masks, seedVar;
    if (auto* o = customState.getDynamicObject())
    {
        custom = o->getProperty ("custom");
        masks  = o->getProperty ("masks");
        const auto sv = o->getProperty ("seed");
        if (sv.isString())
            seedVar = sv;
    }
    applyNonParameterState (custom, masks, seedVar);
}

//==============================================================================
// Editor live data (Stage 3)
//==============================================================================
bool GranulatorProcessor::readTelemetry (Telemetry& t) const
{
    return readTelemetry (t, juce::Time::getMillisecondCounterHiRes());
}

bool GranulatorProcessor::readTelemetry (Telemetry& t, double nowMs) const
{
    t = Telemetry {};
    t.scaleRev = getScaleStateRevision();

    for (int i = 0; i < Telemetry::kSources; ++i)
        t.sources[(size_t) i] = engine.getSrcTelemetry (i + 1);

    const float note = engine.getPitchNote();
    if (note > 0.0f)
    {
        t.pitchMidi    = note;
        t.pitchCents   = std::round (100.0f * (note - std::round (note)));
        t.pitchClarity = engine.getPitchConfident() ? 1.0f : 0.0f;
    }

    // Routing from the APVTS (message thread): a destination is routed when any
    // slot has source > 0 and that destination. The engine's destTelemetry is
    // the modulated v01 for a routed destination.
    for (int m = 1; m <= Ids::kModSlots; ++m)
    {
        const auto* src = parameters.getRawParameterValue (Ids::mod (m, "source"));
        const auto* dst = parameters.getRawParameterValue (Ids::mod (m, "dest"));
        if (src == nullptr || dst == nullptr) continue;
        const int s = juce::roundToInt (src->load (std::memory_order_relaxed));
        const int d = juce::roundToInt (dst->load (std::memory_order_relaxed));
        if (s > 0 && d > 0 && d < Telemetry::kDests)
            t.modRouted[(size_t) d] = true;
    }
    for (int d = 1; d < Telemetry::kDests; ++d)
        if (t.modRouted[(size_t) d])
            t.modNorm[(size_t) d] = engine.getDestTelemetry (d);

    // ── 3.2 buffer strip (P117): one snapshot, every derivation from it ─────
    if (! engine.isPrepared())
        return false;

    const juce::SpinLock::ScopedLockType sl (telemetryLock);

    // ── 3.3 (P131): meters, onset, LFO phase, loop gain ────────────────────
    {
        const bool stale = nowMs - lastReadMs > 250.0;   // R130 (the first read included)
        lastReadMs = nowMs;
        const auto finite0 = [] (float v) { return std::isfinite (v) ? v : 0.0f; };
        const float oL = finite0 (outPkL.exchange (0.0f, std::memory_order_relaxed));
        const float oR = finite0 (outPkR.exchange (0.0f, std::memory_order_relaxed));
        const float iP = finite0 (engine.consumeInPeak());
        t.hasMeters = true;
        if (! stale)
        {
            t.outPeakL = oL;
            t.outPeakR = oR;
            t.inPeak   = iP;
        }

        const uint32_t c = engine.onsetCountNow();   // R131: counter + reader wall clock
        if (c != lastOnsetCount)
        {
            lastOnsetCount = c;
            if (! stale)
            {
                t.onsetFired = true;
                pulseStartMs = nowMs;
            }
        }
        t.hasOnset   = true;
        t.onsetPulse = stale ? 0.0f : (float) juce::jlimit (0.0, 1.0, 1.0 - (nowMs - pulseStartMs) / 200.0);

        for (int n = 0; n < 4; ++n)                  // R132
            t.lfoPhase[(size_t) n] = engine.lfoPhase (n);

        const float k = engine.loopK(), g = engine.loopAgc();   // R133
        t.hasLoopGain = ! (juce::exactlyEqual (k, 0.0f) || engine.isFrozenTel());
        t.loopGainDb  = t.hasLoopGain ? (float) (20.0 * std::log10 ((double) k * (double) g))
                                      : std::numeric_limits<float>::quiet_NaN();
    }

    const ogran::UiSnapshot& snap = engine.readUiSnapshot();
    if (! (snap.fs > 0.0))
        return true;   // prepared, nothing published yet: has* stay false (the page draws the base knobs)

    const double fs = snap.fs;
    t.hasBuffer = true;
    t.frozen    = snap.frozen;
    t.spanSec   = (float) (snap.spanSamples / fs);
    for (int s = 0; s < 4; ++s)
        t.bands[(size_t) s] = { snap.bands[s][0], snap.bands[s][1] };

    // Wave: absolute-cell bins over [head − span, head), D31 dB map (R121, R122).
    ogran::binWave (engine.getPeakRing(), snap.head, snap.spanSamples, t.wave.data(), Telemetry::kWaveBins);
    t.waveCount = Telemetry::kWaveBins;

    // Grains (R119): delay in the wave's time domain (the SNAPSHOT head).
    constexpr double kPanScale = 4.0 / juce::MathConstants<double>::pi;
    const int n = juce::jlimit (0, Telemetry::kMaxGrains, (int) snap.count);
    for (int i = 0; i < n; ++i)
    {
        const auto& u = snap.grains[i];
        auto& g = t.grains[(size_t) i];
        const int s = juce::jlimit (1, 4, (int) u.stream1);
        g.stream    = u.stream1;
        g.delaySec  = (float) std::max (0.0, ((double) snap.head - u.readAbs) / fs);
        g.pitchSt   = (float) (12.0 * std::log2 (u.rate));
        // panGains inverse: θ = (p + 1)·π/4. Equal gains (spread 0 / centre) are
        // exactly 0 — atan2 (1, 1)·4/π − 1 leaves an ulp residue otherwise.
        const double grainPan = juce::exactlyEqual (u.gainL, u.gainR) ? 0.0
                              : kPanScale * std::atan2 ((double) u.gainR, (double) u.gainL) - 1.0;
        g.pan       = (float) juce::jlimit (-1.0, 1.0, grainPan + (double) snap.streamPan[s - 1]);
        g.gain      = (float) std::sqrt (((double) u.gainL * u.gainL + (double) u.gainR * u.gainR) * 0.5);
        g.env       = engine.grainWindow (u.windowStep, u.tiltT, u.tiltA, u.tiltB, u.phase) * u.killGain;
        g.lengthSec = (float) ((double) u.lenSamples / fs);
        if (! u.killing && u.stream1 >= 1 && u.stream1 <= 4)
            ++t.counts[(size_t) (u.stream1 - 1)];
    }
    t.grainCount = n;
    t.hasCounts  = true;
    t.limit      = snap.limit;
    return true;
}

void GranulatorProcessor::getWindowCurve (int stream, float* out, int numPoints) const noexcept
{
    if (out == nullptr || numPoints <= 0)
        return;
    stream = juce::jlimit (1, Ids::kStreams, stream);
    const auto* w  = parameters.getRawParameterValue (Ids::stream (stream, "window"));
    const auto* sk = parameters.getRawParameterValue (Ids::stream (stream, "window_skew"));
    engine.windowCurve (w  != nullptr ? w->load (std::memory_order_relaxed)  : 0.0f,
                        sk != nullptr ? sk->load (std::memory_order_relaxed) : 0.0f,
                        out, numPoints);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranulatorProcessor();
}
