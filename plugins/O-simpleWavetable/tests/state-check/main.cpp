/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable state-check - Stage 1 processor-level probes.

    Neither auval nor pluginval reloads a state and inspects a hand-added
    property, and the stale IMPORTED_BANK child needs TWO reopens to show.
    This console driver constructs OSimpleWavetableAudioProcessor directly
    (the editor TU is never compiled: JUCE_WEB_BROWSER=0) and checks:

      P0 param-contract   21 params, spec order, exact IDs, UTF-8 bank text,
                          output_level "-inf" both ways, default texts
      P1 uiLanguage       2 and 1 survive a fresh-instance load; persisted
                          as the CODE string ("zh-Hans" / "fr")
      P2 uiLanguage       absent attribute -> 0; unknown code "de" -> 0
      P3 IMPORTED_BANK    1 or 2 children load; params restored; the live
                          APVTS tree holds 0 such children
      P4 two-reopen       load -> save -> load -> save: 0 children each save
                          (Stage 2.4 flips this to exactly 1)
      P5 non-default      9 non-default params survive a fresh-instance load
      P6 silent render    note-on / pitch-wheel / 20-note load / UI MIDI:
                          every sample exactly 0.0f; 0-sample block is safe
      P7 shell identity   instrument MIDI flags, 0 in / 1 out bus, latency 0,
                          tail 5 s, bus-layout support
      P8 hostile blobs    garbage / wrong root / zero-length: no crash and
                          nothing changes

    One line per probe: "PASS Pn <name>" or "FAIL Pn <name>: <detail>".
    Exit 0 only if every probe passes. Explicit checks only; jassert is
    never the test mechanism (a Debug build additionally logs any JUCE
    assertion to stderr, which the orchestrator gates on separately).

    Off by default; -DOUARICON_BUILD_TESTS=ON.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>

namespace
{
    using Proc = OSimpleWavetableAudioProcessor;
    namespace ids = OSimpleWavetable::ParamIDs;

    constexpr double kFs    = 48000.0;
    constexpr int    kBlock = 512;
    constexpr float  kTol   = 1.0e-4f;

    const char* const kBankTag = "IMPORTED_BANK";

    //==========================================================================
    std::unique_ptr<Proc> makePrepared()
    {
        auto p = std::make_unique<Proc>();
        p->setPlayConfigDetails (0, 2, kFs, kBlock);
        p->prepareToPlay (kFs, kBlock);
        return p;
    }

    juce::RangedAudioParameter* paramOf (Proc& p, const char* id)
    {
        return p.getAPVTS().getParameter (id);
    }

    void setParam (Proc& p, const char* id, float realValue)
    {
        if (auto* rp = paramOf (p, id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
    }

    float rawOf (Proc& p, const char* id)
    {
        if (auto* v = p.getAPVTS().getRawParameterValue (id))
            return v->load();
        return std::numeric_limits<float>::quiet_NaN();
    }

    bool approxEq (float a, float b)
    {
        return std::isfinite (a) && std::isfinite (b) && std::abs (a - b) <= kTol;
    }

    juce::MemoryBlock saveState (Proc& p)
    {
        juce::MemoryBlock mb;
        p.getStateInformation (mb);
        return mb;
    }

    void loadState (Proc& p, const juce::MemoryBlock& mb)
    {
        p.setStateInformation (mb.getData(), static_cast<int> (mb.getSize()));
    }

    std::unique_ptr<juce::XmlElement> blobToXml (const juce::MemoryBlock& mb)
    {
        return juce::AudioProcessor::getXmlFromBinary (mb.getData(), static_cast<int> (mb.getSize()));
    }

    juce::MemoryBlock xmlToBlob (const juce::XmlElement& xml)
    {
        juce::MemoryBlock mb;
        juce::AudioProcessor::copyXmlToBinary (xml, mb);
        return mb;
    }

    int countBankChildren (const juce::XmlElement& xml)
    {
        int n = 0;
        for (int i = 0; i < xml.getNumChildElements(); ++i)
            if (auto* e = xml.getChildElement (i))
                if (e->hasTagName (kBankTag))
                    ++n;
        return n;
    }

    int countBankChildren (const juce::ValueTree& tree)
    {
        int n = 0;
        for (int i = 0; i < tree.getNumChildren(); ++i)
            if (tree.getChild (i).hasType (kBankTag))
                ++n;
        return n;
    }

    void addFakeBank (juce::XmlElement& xml)
    {
        auto* e = xml.createNewChildElement (kBankTag);
        e->setAttribute ("version",   "1");
        e->setAttribute ("filename",  "fake.wav");
        e->setAttribute ("numFrames", "3");
        e->setAttribute ("encoding",  "flac16");
        e->setAttribute ("data",      "AAAA");
    }

    // A saved blob with position = 0.25 (non-default) and numChildren fake
    // IMPORTED_BANK children appended. Empty block on failure.
    juce::MemoryBlock makeBankBlob (int numChildren, juce::StringArray& problems)
    {
        auto src = makePrepared();
        setParam (*src, ids::position, 0.25f);
        auto xml = blobToXml (saveState (*src));
        if (xml == nullptr)
        {
            problems.add ("source save produced no XML");
            return {};
        }

        for (int i = 0; i < numChildren; ++i)
            addFakeBank (*xml);

        if (countBankChildren (*xml) != numChildren)
            problems.add ("fixture holds " + juce::String (countBankChildren (*xml))
                          + " children, wanted " + juce::String (numChildren));

        return xmlToBlob (*xml);
    }

    //==========================================================================
    bool probeP0 (juce::String& detail)
    {
        juce::StringArray problems;
        Proc proc;

        const auto& ps = proc.getParameters();
        if (ps.size() != 21)
            problems.add ("count=" + juce::String (ps.size()));

        if (static_cast<size_t> (ps.size()) == ids::all.size())
        {
            for (int i = 0; i < ps.size(); ++i)
            {
                const juce::String want (ids::all[static_cast<size_t> (i)]);
                auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (ps[i]);
                const juce::String got = withId != nullptr ? withId->getParameterID() : juce::String ("<no-id>");
                if (got != want)
                    problems.add ("index " + juce::String (i) + " id=" + got + " want=" + want);
            }
        }

        // bank: UTF-8 arrow intact, Imported last.
        const juce::String wantSaw (juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw"));
        if (auto* bankP = dynamic_cast<juce::AudioParameterChoice*> (paramOf (proc, ids::bank)))
        {
            const bool hasBytes = std::strstr (bankP->choices[0].toRawUTF8(), "\xE2\x86\x92") != nullptr;
            if (! hasBytes)                          problems.add ("bank[0] lacks bytes E2 86 92");
            if (bankP->choices[0] != wantSaw)        problems.add ("bank[0] != 'Sine -> Saw' (UTF-8)");
            if (bankP->choices.size() != 6)          problems.add ("bank choices=" + juce::String (bankP->choices.size()));
            else if (bankP->choices[5] != "Imported") problems.add ("bank[5]=" + bankP->choices[5]);
        }
        else
        {
            problems.add ("bank is not an AudioParameterChoice");
        }

        // output_level: "-inf" at the floor, and parsed back to the floor.
        if (auto* outP = paramOf (proc, ids::outputLevel))
        {
            const auto t = outP->getText (0.0f, 64);
            if (t != "-inf")
                problems.add ("output_level text(0)='" + t + "'");
            const float back = outP->getValueForText ("-inf");
            if (! juce::exactlyEqual (back, 0.0f))
                problems.add ("output_level valueForText(-inf)=" + juce::String (back, 6));
        }
        else
        {
            problems.add ("output_level missing");
        }

        auto checkDefaultText = [&proc, &problems] (const char* id, const char* want)
        {
            if (auto* p = paramOf (proc, id))
            {
                const auto t = p->getText (p->getDefaultValue(), 64);
                if (t != want)
                    problems.add (juce::String (id) + " defaultText='" + t + "' want='" + want + "'");
            }
            else
            {
                problems.add (juce::String (id) + " missing");
            }
        };
        checkDefaultText (ids::bitDepth, "Full");
        checkDefaultText (ids::lfoDiv,   "1/1");
        checkDefaultText (ids::lfoShape, "Triangle");

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool probeP1 (juce::String& detail)
    {
        juce::StringArray problems;

        for (const int lang : { 2, 1 })
        {
            const juce::String wantCode = lang == 2 ? "zh-Hans" : "fr";

            auto src = makePrepared();
            src->uiLanguage.store (lang);
            const auto mb = saveState (*src);

            auto xml = blobToXml (mb);
            const juce::String attr = xml != nullptr ? xml->getStringAttribute ("uiLanguage")
                                                     : juce::String ("<no xml>");
            if (attr != wantCode)
                problems.add ("lang " + juce::String (lang) + " persisted as '" + attr + "' want '" + wantCode + "'");

            auto fresh = makePrepared();
            loadState (*fresh, mb);
            const int got = fresh->uiLanguage.load();
            if (got != lang)
                problems.add ("lang " + juce::String (lang) + " restored as " + juce::String (got));
        }

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool probeP2 (juce::String& detail)
    {
        juce::StringArray problems;

        auto src = makePrepared();
        src->uiLanguage.store (2);
        auto xml = blobToXml (saveState (*src));
        if (xml == nullptr)
        {
            detail = "source save produced no XML";
            return false;
        }

        // Absent attribute: a fresh instance stays at its default (0).
        xml->removeAttribute ("uiLanguage");
        {
            auto fresh = makePrepared();
            loadState (*fresh, xmlToBlob (*xml));
            const int got = fresh->uiLanguage.load();
            if (got != 0)
                problems.add ("absent -> " + juce::String (got) + " want 0");
        }

        // Unknown code: the allow-list maps it to 0. The instance starts at 2
        // so a restore that silently skipped would be caught.
        xml->setAttribute ("uiLanguage", "de");
        {
            auto fresh = makePrepared();
            fresh->uiLanguage.store (2);
            loadState (*fresh, xmlToBlob (*xml));
            const int got = fresh->uiLanguage.load();
            if (got != 0)
                problems.add ("'de' -> " + juce::String (got) + " want 0");
        }

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool probeP3 (juce::String& detail)
    {
        juce::StringArray problems;

        for (const int n : { 1, 2 })
        {
            const auto blob = makeBankBlob (n, problems);
            if (blob.getSize() == 0)
                continue;

            auto fresh = makePrepared();
            loadState (*fresh, blob);

            const float pos = rawOf (*fresh, ids::position);
            if (! approxEq (pos, 0.25f))
                problems.add (juce::String (n) + " child(ren): position=" + juce::String (pos, 6) + " want 0.25");

            const int live = countBankChildren (fresh->getAPVTS().state);
            if (live != 0)
                problems.add (juce::String (n) + " child(ren): live tree holds " + juce::String (live));
        }

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool probeP4 (juce::String& detail)
    {
        juce::StringArray problems;

        const auto blob = makeBankBlob (2, problems);
        if (blob.getSize() == 0)
        {
            detail = problems.joinIntoString ("; ");
            return false;
        }

        auto first = makePrepared();
        loadState (*first, blob);
        const auto save1 = saveState (*first);

        auto second = makePrepared();
        loadState (*second, save1);
        const auto save2 = saveState (*second);

        int saveIndex = 0;
        for (const auto* mb : { &save1, &save2 })
        {
            ++saveIndex;
            auto xml = blobToXml (*mb);
            if (xml == nullptr)
            {
                problems.add ("save " + juce::String (saveIndex) + " produced no XML");
                continue;
            }
            const int n = countBankChildren (*xml);
            if (n != 0)
                problems.add ("save " + juce::String (saveIndex) + " holds " + juce::String (n) + " children");
        }

        // Non-vacuity: the parameters did ride through both reopens.
        const float pos = rawOf (*second, ids::position);
        if (! approxEq (pos, 0.25f))
            problems.add ("after two reopens position=" + juce::String (pos, 6) + " want 0.25");

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    struct Setting
    {
        const char* id;
        float value;
        bool discrete;
    };

    bool probeP5 (juce::String& detail)
    {
        juce::StringArray problems;

        const Setting settings[] = {
            { ids::bank,        5.0f,   true  },
            { ids::interp,      0.0f,   true  },
            { ids::bandlimit,   0.0f,   true  },
            { ids::bitDepth,    9.0f,   true  },
            { ids::lfoDiv,      15.0f,  true  },
            { ids::voiceMode,   1.0f,   true  },
            { ids::position,    0.37f,  false },
            { ids::envAmount,   -0.5f,  false },
            { ids::outputLevel, -60.0f, false },
        };

        auto matches = [] (const Setting& s, float got)
        {
            return s.discrete ? juce::exactlyEqual (got, s.value) : approxEq (got, s.value);
        };

        auto src = makePrepared();
        for (const auto& s : settings)
            setParam (*src, s.id, s.value);

        // Sanity: the values actually took on the source instance.
        for (const auto& s : settings)
        {
            const float got = rawOf (*src, s.id);
            if (! matches (s, got))
                problems.add (juce::String ("source ") + s.id + "=" + juce::String (got, 6));
        }

        const auto mb = saveState (*src);
        auto fresh = makePrepared();
        loadState (*fresh, mb);

        for (const auto& s : settings)
        {
            const float got = rawOf (*fresh, s.id);
            if (! matches (s, got))
                problems.add (juce::String (s.id) + "=" + juce::String (got, 6)
                              + " want " + juce::String (s.value, 6));
        }

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    void fillOnes (juce::AudioBuffer<float>& buf)
    {
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            juce::FloatVectorOperations::fill (buf.getWritePointer (ch), 1.0f, buf.getNumSamples());
    }

    bool allExactZero (const juce::AudioBuffer<float>& buf)
    {
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* d = buf.getReadPointer (ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
                if (! std::isfinite (d[i]) || ! juce::exactlyEqual (d[i], 0.0f))
                    return false;
        }
        return true;
    }

    bool probeP6 (juce::String& detail)
    {
        juce::StringArray problems;
        auto proc = makePrepared();
        juce::AudioBuffer<float> buf (2, kBlock);

        // Block 1: note-on, pitch-wheel, note-off.
        {
            fillOnes (buf);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
            midi.addEvent (juce::MidiMessage::pitchWheel (1, 12000), 100);
            midi.addEvent (juce::MidiMessage::noteOff (1, 60), 400);
            proc->processBlock (buf, midi);
            if (! allExactZero (buf))
                problems.add ("note-on/pitch-wheel block not silent");
        }

        // Block 2: 20 simultaneous note-ons (more than the 16 voices -> stealing).
        {
            fillOnes (buf);
            juce::MidiBuffer midi;
            for (int i = 0; i < 20; ++i)
                midi.addEvent (juce::MidiMessage::noteOn (1, 40 + i, 0.7f), i);
            proc->processBlock (buf, midi);
            if (! allExactZero (buf))
                problems.add ("20-note block not silent");
        }

        // Block 3: zero samples must return cleanly (collector asserts > 0).
        {
            juce::AudioBuffer<float> empty (2, 0);
            juce::MidiBuffer midi;
            proc->processBlock (empty, midi);
        }

        // Block 4: hostile UI MIDI (NaN velocity, out-of-range note).
        {
            proc->handleUiMidi (60, true, std::numeric_limits<float>::quiet_NaN());
            proc->handleUiMidi (200, true, 0.8f);
            fillOnes (buf);
            juce::MidiBuffer midi;
            proc->processBlock (buf, midi);
            if (! allExactZero (buf))
                problems.add ("UI-MIDI block not silent");
        }

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool probeP7 (juce::String& detail)
    {
        juce::StringArray problems;
        auto proc = makePrepared();

        if (! proc->acceptsMidi())  problems.add ("acceptsMidi false");
        if (proc->producesMidi())   problems.add ("producesMidi true");
        if (proc->isMidiEffect())   problems.add ("isMidiEffect true");

        if (proc->getBusCount (true) != 0)
            problems.add ("input buses=" + juce::String (proc->getBusCount (true)));
        if (proc->getBusCount (false) != 1)
            problems.add ("output buses=" + juce::String (proc->getBusCount (false)));

        if (proc->getLatencySamples() != 0)
            problems.add ("latency=" + juce::String (proc->getLatencySamples()));
        if (! juce::exactlyEqual (proc->getTailLengthSeconds(), 5.0))
            problems.add ("tail=" + juce::String (proc->getTailLengthSeconds()));

        auto layoutWithOut = [] (const juce::AudioChannelSet& out)
        {
            juce::AudioProcessor::BusesLayout l;
            l.outputBuses.add (out);
            return l;
        };

        if (! proc->checkBusesLayoutSupported (layoutWithOut (juce::AudioChannelSet::stereo())))
            problems.add ("stereo out rejected");
        if (! proc->checkBusesLayoutSupported (layoutWithOut (juce::AudioChannelSet::mono())))
            problems.add ("mono out rejected");
        if (proc->checkBusesLayoutSupported (layoutWithOut (juce::AudioChannelSet::create5point1())))
            problems.add ("5.1 out accepted");

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool probeP8 (juce::String& detail)
    {
        juce::StringArray problems;
        auto proc = makePrepared();

        setParam (*proc, ids::position, 0.6f);
        setParam (*proc, ids::bank, 3.0f);
        proc->uiLanguage.store (1);

        auto unchanged = [&proc] (juce::String& why)
        {
            const float pos  = rawOf (*proc, ids::position);
            const float bnk  = rawOf (*proc, ids::bank);
            const int   lang = proc->uiLanguage.load();
            const bool ok = approxEq (pos, 0.6f) && juce::exactlyEqual (bnk, 3.0f) && lang == 1;
            why = "position=" + juce::String (pos, 6) + " bank=" + juce::String (bnk, 1)
                + " uiLanguage=" + juce::String (lang);
            return ok;
        };

        juce::String why;
        if (! unchanged (why))
            problems.add ("precondition: " + why);

        // Garbage bytes (deterministic).
        {
            juce::MemoryBlock garbage (257);
            auto* bytes = static_cast<unsigned char*> (garbage.getData());
            for (size_t i = 0; i < garbage.getSize(); ++i)
                bytes[i] = static_cast<unsigned char> ((i * 37u + 11u) & 0xffu);
            loadState (*proc, garbage);
            if (! unchanged (why))
                problems.add ("garbage: " + why);
        }

        // Valid XML, wrong root tag (looks like state otherwise).
        {
            juce::XmlElement foo ("FOO");
            foo.setAttribute ("uiLanguage", "zh-Hans");
            auto* child = foo.createNewChildElement ("PARAM");
            child->setAttribute ("id", "position");
            child->setAttribute ("value", "0.1");
            addFakeBank (foo);
            loadState (*proc, xmlToBlob (foo));
            if (! unchanged (why))
                problems.add ("wrong-root: " + why);
        }

        // Zero-length blob.
        {
            const char dummy = 0;
            proc->setStateInformation (&dummy, 0);
            if (! unchanged (why))
                problems.add ("zero-length: " + why);
        }

        detail = problems.joinIntoString ("; ");
        return problems.isEmpty();
    }

    //==========================================================================
    bool report (const char* tag, const char* name, bool (*probe) (juce::String&))
    {
        juce::String detail;
        const bool ok = probe (detail);

        if (ok)
            std::cout << "PASS " << tag << " " << name << "\n";
        else
            std::cout << "FAIL " << tag << " " << name << ": " << detail.toStdString() << "\n";

        std::cout.flush();
        return ok;
    }
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    int failures = 0;

    if (! report ("P0", "param-contract",          probeP0)) ++failures;
    if (! report ("P1", "uiLanguage-roundtrip",    probeP1)) ++failures;
    if (! report ("P2", "uiLanguage-absent-unknown", probeP2)) ++failures;
    if (! report ("P3", "imported-bank-tolerance", probeP3)) ++failures;
    if (! report ("P4", "imported-bank-two-reopen", probeP4)) ++failures;
    if (! report ("P5", "non-default-params",      probeP5)) ++failures;
    if (! report ("P6", "silent-render",           probeP6)) ++failures;
    if (! report ("P7", "shell-identity",          probeP7)) ++failures;
    if (! report ("P8", "hostile-blobs",           probeP8)) ++failures;

    return failures == 0 ? 0 : 1;
}
