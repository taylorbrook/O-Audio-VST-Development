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
 // #include "PluginEditor.h"   // Stage 3: GranulatorEditor
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
        }

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
    }
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

    // The seed is adopted by the engine at its next grid point.
    engine.process (in, numIn, out, numOut, n, getSeed());

    for (int ch = numOut; ch < numCh; ++ch)
        buffer.clear (ch, 0, n);
}

juce::AudioProcessorEditor* GranulatorProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new juce::GenericAudioProcessorEditor (*this);   // Stage 3: new GranulatorEditor (*this)
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

    // Non-parameter state: defaults FIRST, so anything absent from the blob
    // reads as default rather than as whatever this instance held.
    resetNonParameterState();

    if (! lang.isVoid())
    {
        const auto s = lang.toString().trim();
        uiLanguage.store (s.isNotEmpty() && s.containsOnly ("0123456789") ? juce::jlimit (0, 2, s.getIntValue())
                                                                          : languageIndex (s),
                          std::memory_order_relaxed);
    }

    if (eng.isValid())
    {
        const auto s = eng.getProperty ("seed");
        if (! s.isVoid() && s.toString().isNotEmpty() && s.toString().containsOnly ("-0123456789"))
            setSeed (static_cast<uint64_t> (s.toString().getLargeIntValue()));
        // stateVersion / modDestTableVersion: 1 is the only version so far (migrations land here).
    }

    if (cs.isValid())
    {
        const auto text = cs.getProperty ("text");
        if (! text.isVoid() && text.toString().isNotEmpty())
        {
            juce::String badToken;
            const auto name = cs.getProperty ("name");
            setCustomScale (name.isVoid() ? juce::String() : name.toString(), text.toString(), badToken);   // failure keeps the 12-EDO default
        }
    }

    if (dm.isValid())
        for (const auto& m : dm)
        {
            if (! m.hasType (kMask)) continue;
            const auto sv = m.getProperty ("stream"), cv = m.getProperty ("collection"), bv = m.getProperty ("bits");
            if (sv.isVoid() || cv.isVoid() || bv.isVoid()) continue;
            const auto bits = bv.toString();
            if (bits.length() != 16 || ! bits.containsOnly ("0123456789abcdefABCDEF")) continue;
            setDegreeMask (static_cast<int> (sv), static_cast<int> (cv), static_cast<uint64_t> (bits.getHexValue64()));
        }

    bumpScaleRevision();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranulatorProcessor();
}
