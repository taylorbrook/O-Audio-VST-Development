/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - render harness (Stage 1 gate G11; Stage 2 extends it)

      O-ParametricEQ-render-harness                     run every row
      O-ParametricEQ-render-harness --golden-dir <dir>  compare L1 against <dir>/params.tsv
      O-ParametricEQ-render-harness --dump-params       print the layout golden, exit 0

    Exit code = number of FAIL rows (capped at 255). Last line: "N PASS, M FAIL".

    The golden (tests/golden/params.tsv) is this program's own --dump-params
    output, so on its own it only proves the layout has not moved. It is tied
    to parameter-spec.md by tests/golden/check_spec.py, independently of this
    file.

      L1   196 parameters; dump == <golden-dir>/params.tsv byte for byte
      L2   version hint 1; automatable; 50 Bool / 72 Choice / 74 Float; 25 groups in order
      L3   none of the 72 reserved dynamic-EQ IDs is registered
      L4   bypass parameter; latency 0 at 44.1 k and 96 k; tail 0
      L5   float defaults: normalised exactly 0.5; linear raw exact; freq / q raw within 2e-4
      B1   bus layouts: (1,1) and (2,2) accepted; (2,1), (1,2), (0,2), (2,0), 5.1 refused
      P1   pass-through, memcmp-equal: mono / stereo x blocks 1, 64, 512, 4096 x
           bypass Off / On / every parameter random, and through processBlockBypassed
      P1n  vacuity guard for P1: one flipped bit fails the comparator; input RMS > -20 dBFS
      S1   state round trip: 196 getValue() bit-equal, UI members, byte-equal re-save
      S2   XML shape
      S3   two reopens with a UI change between them
      S4   unknown parameter loads and is saved back
      S5   missing parameter loads at its default (with a pre-set guard)
      S6   stateVersion="2" loads; the next save writes "1"
      S7   wrong root tag / truncated / empty blob: nothing changes
      S8   UI values validated by type

    S1 is exact by decision (plan P13). If it misses by one ULP on a skewed
    float, the row prints every ID with both values: do not loosen it.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#if defined (__clang__)
 #pragma clang diagnostic ignored "-Wfloat-equal"
#endif

// juce_audio_plugin_client (which normally declares it) is not linked into a
// console target.
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#ifndef GOLDEN_DIR
 #error "GOLDEN_DIR must point at tests/golden"
#endif

namespace
{
    using Processor = OuariconParametricEQAudioProcessor;

    constexpr int kExpectedParams = 196;
    constexpr int kExpectedBools = 50, kExpectedChoices = 72, kExpectedFloats = 74;
    constexpr int kExpectedGroups = 25;

    // "The stimulus was applied" guard for the seeded-random rows. A uniform
    // random normalised value leaves about 163 of the 196 away from default
    // (half of the Bools and a share of the Choices land on it).
    constexpr int kMinNonDefault = 100;

    //==========================================================================
    int failures = 0, passes = 0;

    void row (bool ok, const juce::String& name, const juce::String& detail = {})
    {
        std::printf ("  [%s] %-70s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        std::fflush (stdout);

        if (ok) ++passes; else ++failures;
    }

    void section (const char* title)
    {
        std::printf ("\n-- %s\n", title);
    }

    // Collects the sub-failures of a many-item row; keeps the first few as notes.
    struct Tally
    {
        int total = 0, bad = 0;
        juce::StringArray notes;

        void check (bool ok, const juce::String& what)
        {
            ++total;

            if (! ok)
            {
                ++bad;

                if (notes.size() < 4)
                    notes.add (what);
            }
        }

        juce::String detail() const
        {
            juce::String text = juce::String (total - bad) + "/" + juce::String (total);

            if (bad > 0)
                text += "  e.g. " + notes.joinIntoString ("; ");

            return text;
        }
    };

    std::string fmt (double value)
    {
        char text[64];
        std::snprintf (text, sizeof (text), "%.9g", value);
        return text;
    }

    juce::String fmtJ (double value)
    {
        return juce::String (fmt (value));
    }

    bool sameBits (float a, float b)
    {
        return std::memcmp (&a, &b, sizeof (float)) == 0;
    }

    //==========================================================================
    std::unique_ptr<Processor> make()
    {
        std::unique_ptr<juce::AudioProcessor> base (createPluginFilter());
        auto* typed = dynamic_cast<Processor*> (base.get());

        if (typed == nullptr)
        {
            std::printf ("FATAL: createPluginFilter() did not return the O-ParametricEQ processor\n");
            std::exit (255);
        }

        base.release();
        return std::unique_ptr<Processor> (typed);
    }

    std::vector<juce::RangedAudioParameter*> rangedParams (juce::AudioProcessor& p)
    {
        std::vector<juce::RangedAudioParameter*> list;

        for (auto* parameter : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                list.push_back (r);

        return list;
    }

    juce::RangedAudioParameter* findParam (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* r : rangedParams (p))
            if (r->getParameterID() == id)
                return r;

        return nullptr;
    }

    // The host side: a normalised value, through the host-notifying path.
    void setNormalised (juce::AudioProcessor& p, const juce::String& id, float normalised)
    {
        if (auto* r = findParam (p, id))
            r->setValueNotifyingHost (normalised);
        else
            row (false, "harness: no parameter '" + id + "'");
    }

    // An engineering value, converted and written the same way.
    void setRaw (juce::AudioProcessor& p, const juce::String& id, float raw)
    {
        if (auto* r = findParam (p, id))
            r->setValueNotifyingHost (r->convertTo0to1 (raw));
        else
            row (false, "harness: no parameter '" + id + "'");
    }

    void setAllRandom (juce::AudioProcessor& p, juce::int64 seed)
    {
        juce::Random random (seed);

        for (auto* r : rangedParams (p))
        {
            const auto x = random.nextFloat();

            // Discrete parameters get a LEGAL normalised value (one a host or the
            // parameter's own UI can produce). juce::AudioParameterBool stores an
            // off-grid value such as 0.37 verbatim in getValue() while the state
            // tree holds the thresholded 0 / 1, so an off-grid stimulus can never
            // round-trip bit-equal. Continuous parameters keep the raw draw.
            r->setValueNotifyingHost (r->isDiscrete() ? r->convertTo0to1 (r->convertFrom0to1 (x)) : x);
        }
    }

    std::vector<float> valuesOf (juce::AudioProcessor& p)
    {
        std::vector<float> values;

        for (auto* r : rangedParams (p))
            values.push_back (r->getValue());

        return values;
    }

    int countNonDefault (juce::AudioProcessor& p)
    {
        int n = 0;

        for (auto* r : rangedParams (p))
            if (! sameBits (r->getValue(), r->getDefaultValue()))
                ++n;

        return n;
    }

    float rawOf (Processor& p, const juce::String& id)
    {
        if (auto* raw = p.parameters.getRawParameterValue (id))
            return raw->load();

        row (false, "harness: no raw value for '" + id + "'");
        return std::numeric_limits<float>::quiet_NaN();
    }

    //==========================================================================
    // UI state (the six non-parameter members)
    //==========================================================================
    struct UiState
    {
        int language = 0, analyzer = 0, range = 0, band = 0, width = 0, height = 0;

        bool operator== (const UiState& other) const
        {
            return language == other.language && analyzer == other.analyzer && range == other.range
                && band == other.band && width == other.width && height == other.height;
        }

        juce::String describe() const
        {
            return "lang " + juce::String (language) + " mode " + juce::String (analyzer)
                 + " range " + juce::String (range) + " band " + juce::String (band)
                 + " size " + juce::String (width) + "x" + juce::String (height);
        }
    };

    UiState uiOf (const Processor& p)
    {
        UiState s;
        s.language = p.uiLanguage.load();
        s.analyzer = p.analyzerMode.load();
        s.range    = p.uiDisplayRange.load();
        s.band     = p.uiSelectedBand.load();
        s.width    = p.uiEditorWidth.load();
        s.height   = p.uiEditorHeight.load();
        return s;
    }

    void setUi (Processor& p, const UiState& s)
    {
        p.uiLanguage.store (s.language);
        p.analyzerMode.store (s.analyzer);
        p.uiDisplayRange.store (s.range);
        p.uiSelectedBand.store (s.band);
        p.uiEditorWidth.store (s.width);
        p.uiEditorHeight.store (s.height);
    }

    UiState makeUi (int language, int analyzer, int range, int band, int width, int height)
    {
        UiState s;
        s.language = language; s.analyzer = analyzer; s.range = range;
        s.band = band; s.width = width; s.height = height;
        return s;
    }

    //==========================================================================
    // State blobs
    //==========================================================================
    juce::MemoryBlock stateOf (juce::AudioProcessor& p)
    {
        juce::MemoryBlock block;
        p.getStateInformation (block);
        return block;
    }

    void loadState (juce::AudioProcessor& p, const juce::MemoryBlock& block)
    {
        p.setStateInformation (block.getData(), (int) block.getSize());
    }

    std::unique_ptr<juce::XmlElement> xmlOf (const juce::MemoryBlock& block)
    {
        return juce::AudioProcessor::getXmlFromBinary (block.getData(), (int) block.getSize());
    }

    juce::MemoryBlock blobOf (const juce::XmlElement& xml)
    {
        juce::MemoryBlock block;
        juce::AudioProcessor::copyXmlToBinary (xml, block);
        return block;
    }

    int countChildren (const juce::XmlElement& xml, const char* tag)
    {
        int n = 0;

        for (auto* child : xml.getChildIterator())
            if (child->hasTagName (tag))
                ++n;

        return n;
    }

    juce::XmlElement* paramChild (juce::XmlElement& xml, const juce::String& id)
    {
        for (auto* child : xml.getChildIterator())
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == id)
                return child;

        return nullptr;
    }

    //==========================================================================
    // Layout dump (the golden)
    //==========================================================================
    using GroupMap = std::map<const juce::AudioProcessorParameter*, juce::String>;

    void walkGroups (const juce::AudioProcessorParameterGroup& group, const juce::String& groupId,
                     GroupMap& groupOf, juce::StringArray& order)
    {
        for (const auto* node : group)
        {
            if (auto* sub = node->getGroup())
            {
                order.add (sub->getID());
                walkGroups (*sub, sub->getID(), groupOf, order);
            }
            else if (auto* parameter = node->getParameter())
            {
                groupOf[parameter] = groupId;
            }
        }
    }

    const char* typeOf (juce::AudioProcessorParameter* parameter)
    {
        if (dynamic_cast<juce::AudioParameterFloat*> (parameter) != nullptr)   return "Float";
        if (dynamic_cast<juce::AudioParameterChoice*> (parameter) != nullptr)  return "Choice";
        if (dynamic_cast<juce::AudioParameterBool*> (parameter) != nullptr)    return "Bool";
        return "Other";
    }

    // A cell can hold neither a tab nor a line break.
    std::string cell (const juce::String& text)
    {
        if (text.isEmpty())
            return "-";

        return text.replace ("\t", " ").replace ("\r", " ").replace ("\n", " ").toStdString();
    }

    /** Columns: index, id, name, group, type, versionHint, start, end, interval,
        skew, defaultNorm, defaultRaw, numSteps, choices, automatable.
        Floats %.9g; choices joined with '|'; empty cells '-'; "\n" endings.
        The range columns are filled for Float rows only: a Bool or Choice
        range is JUCE's own, not something the layout chooses. */
    std::string buildDump (juce::AudioProcessor& p)
    {
        GroupMap groupOf;
        juce::StringArray groupOrder;
        walkGroups (p.getParameterTree(), {}, groupOf, groupOrder);

        std::string out;
        out += "# GENERATED by O-ParametricEQ-render-harness --dump-params; do not edit\n";
        out += "index\tid\tname\tgroup\ttype\tversionHint\tstart\tend\tinterval\tskew\t"
               "defaultNorm\tdefaultRaw\tnumSteps\tchoices\tautomatable\n";

        int index = 0;

        for (auto* parameter : p.getParameters())
        {
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (parameter);
            const auto found = groupOf.find (parameter);

            out += std::to_string (index++);
            out += '\t';
            out += cell (r != nullptr ? r->getParameterID() : juce::String ("<no-id>"));
            out += '\t';
            out += cell (parameter->getName (128));
            out += '\t';
            out += cell (found != groupOf.end() ? found->second : juce::String());
            out += '\t';
            out += typeOf (parameter);
            out += '\t';
            out += std::to_string (parameter->getVersionHint());
            out += '\t';

            if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (parameter))
            {
                const auto& range = f->getNormalisableRange();
                out += fmt ((double) range.start);    out += '\t';
                out += fmt ((double) range.end);      out += '\t';
                out += fmt ((double) range.interval); out += '\t';
                out += fmt ((double) range.skew);     out += '\t';
            }
            else
            {
                out += "-\t-\t-\t-\t";
            }

            const float defaultNorm = parameter->getDefaultValue();
            out += fmt ((double) defaultNorm);
            out += '\t';
            out += r != nullptr ? fmt ((double) r->convertFrom0to1 (defaultNorm)) : std::string ("-");
            out += '\t';
            out += std::to_string (parameter->getNumSteps());
            out += '\t';

            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (parameter))
                out += cell (c->choices.joinIntoString ("|"));
            else
                out += "-";

            out += '\t';
            out += parameter->isAutomatable() ? "1" : "0";
            out += '\n';
        }

        return out;
    }

    //==========================================================================
    // L. layout
    //==========================================================================
    void layoutRows (const juce::File& goldenDir)
    {
        section ("L. parameter layout");

        auto p = make();
        const auto& params = p->getParameters();
        const auto ranged = rangedParams (*p);

        // L1
        {
            const auto dump = buildDump (*p);
            const auto goldenFile = goldenDir.getChildFile ("params.tsv");
            juce::MemoryBlock golden;
            const bool readOk = goldenFile.existsAsFile() && goldenFile.loadFileAsData (golden);
            const bool same = readOk && golden.getSize() == dump.size()
                           && std::memcmp (golden.getData(), dump.data(), dump.size()) == 0;

            juce::String detail = juce::String (params.size()) + " params";

            if (! readOk)
            {
                detail += "; cannot read " + goldenFile.getFullPathName();
            }
            else if (! same)
            {
                // Name the first differing line.
                const std::string goldenText (static_cast<const char*> (golden.getData()), golden.getSize());
                size_t a = 0, b = 0;
                int line = 1;

                while (a < dump.size() && b < goldenText.size())
                {
                    const auto ea = dump.find ('\n', a);
                    const auto eb = goldenText.find ('\n', b);
                    const auto la = dump.substr (a, ea == std::string::npos ? std::string::npos : ea - a);
                    const auto lb = goldenText.substr (b, eb == std::string::npos ? std::string::npos : eb - b);

                    if (la != lb || ea == std::string::npos || eb == std::string::npos)
                        break;

                    a = ea + 1;
                    b = eb + 1;
                    ++line;
                }

                detail += "; differs from " + goldenFile.getFullPathName() + " at line " + juce::String (line)
                        + " (plugin " + juce::String ((int) dump.size()) + " bytes, golden "
                        + juce::String ((int) golden.getSize()) + " bytes)";
            }

            row (params.size() == kExpectedParams && (int) ranged.size() == kExpectedParams && same,
                 "L1 196 parameters; dump == golden params.tsv byte for byte", detail);
        }

        // L2
        {
            Tally hints, automatable;
            int numFloat = 0, numChoice = 0, numBool = 0;

            for (auto* parameter : params)
            {
                const juce::String name = parameter->getName (64);
                hints.check (parameter->getVersionHint() == 1, name + " hint " + juce::String (parameter->getVersionHint()));
                automatable.check (parameter->isAutomatable(), name);

                const juce::String type (typeOf (parameter));
                numFloat  += type == "Float"  ? 1 : 0;
                numChoice += type == "Choice" ? 1 : 0;
                numBool   += type == "Bool"   ? 1 : 0;
            }

            row (hints.bad == 0 && hints.total == kExpectedParams, "L2 every version hint is 1", hints.detail());
            row (automatable.bad == 0 && automatable.total == kExpectedParams, "L2 every parameter automatable", automatable.detail());
            row (numBool == kExpectedBools && numChoice == kExpectedChoices && numFloat == kExpectedFloats,
                 "L2 types 50 Bool / 72 Choice / 74 Float",
                 juce::String (numBool) + " / " + juce::String (numChoice) + " / " + juce::String (numFloat));

            GroupMap groupOf;
            juce::StringArray order;
            walkGroups (p->getParameterTree(), {}, groupOf, order);

            juce::StringArray expected { "global" };

            for (int n = 1; n <= Processor::kNumBands; ++n)
                expected.add ("band" + juce::String (n).paddedLeft ('0', 2));

            Tally grouped;

            for (auto* parameter : params)
            {
                const auto found = groupOf.find (parameter);
                grouped.check (found != groupOf.end() && found->second.isNotEmpty(), parameter->getName (64));
            }

            row (order == expected && order.size() == kExpectedGroups && grouped.bad == 0,
                 "L2 25 groups: global, band01 ... band24, in order; every parameter in one",
                 juce::String (order.size()) + " groups, grouped " + grouped.detail());
        }

        // L3
        {
            Tally reserved;

            for (int n = 1; n <= Processor::kNumBands; ++n)
                for (const char* suffix : { "dyn_range", "dyn_threshold", "dyn_auto" })
                {
                    const juce::String id = "b" + juce::String (n).paddedLeft ('0', 2) + "_" + suffix;
                    reserved.check (p->parameters.getParameter (id) == nullptr, id);
                }

            // Guard: the lookup does find a registered ID.
            const bool lookupWorks = p->parameters.getParameter ("b01_freq") != nullptr
                                  && p->parameters.getParameter ("b24_placement") != nullptr;

            row (reserved.bad == 0 && reserved.total == 72 && lookupWorks,
                 "L3 none of the 72 reserved dynamic-EQ IDs is registered", reserved.detail());
        }

        // L4
        {
            auto* bypass = findParam (*p, "bypass");
            row (bypass != nullptr && p->getBypassParameter() == bypass,
                 "L4 getBypassParameter() is the parameter with ID 'bypass'");

            p->prepareToPlay (44100.0, 512);
            const int latency44 = p->getLatencySamples();
            p->releaseResources();
            p->prepareToPlay (96000.0, 512);
            const int latency96 = p->getLatencySamples();
            p->releaseResources();

            row (latency44 == 0 && latency96 == 0, "L4 latency 0 after prepareToPlay at 44.1 k and 96 k",
                 juce::String (latency44) + " / " + juce::String (latency96));
            row (p->getTailLengthSeconds() == 0.0, "L4 tail 0", fmtJ (p->getTailLengthSeconds()));
        }

        // L5 (a fresh instance: nothing has been written)
        {
            auto q = make();
            Tally norm, linear, skewed;
            juce::String actual;

            for (auto* r : rangedParams (*q))
            {
                if (dynamic_cast<juce::AudioParameterFloat*> (r) == nullptr)
                    continue;

                const auto id = r->getParameterID();
                const float defaultNorm = r->getDefaultValue();
                const float converted = r->convertFrom0to1 (defaultNorm);
                const float live = rawOf (*q, id);

                norm.check (sameBits (defaultNorm, 0.5f), id + " norm " + fmtJ ((double) defaultNorm));

                const bool isFreq = id.endsWith ("_freq");
                const bool isQ    = id.endsWith ("_q");

                if (isFreq || isQ)
                {
                    const double target = isFreq ? 1000.0 : 1.0;
                    const bool ok = std::abs ((double) converted - target) / target <= 2e-4
                                 && std::abs ((double) live - target) / target <= 2e-4;
                    skewed.check (ok, id + " raw " + fmtJ ((double) live));

                    if (id == "b01_freq" || id == "b01_q")
                        actual += (actual.isEmpty() ? "" : ", ") + id + " raw " + fmtJ ((double) live)
                                + " (convertFrom0to1 " + fmtJ ((double) converted) + ")";
                }
                else
                {
                    const float target = id == "gain_scale" ? 100.0f : 0.0f;
                    linear.check (sameBits (converted, target) && sameBits (live, target),
                                  id + " raw " + fmtJ ((double) live));
                }
            }

            row (norm.bad == 0 && norm.total == kExpectedFloats, "L5 normalised default of every float exactly 0.5", norm.detail());
            row (linear.bad == 0 && linear.total == 2 + Processor::kNumBands,
                 "L5 linear raw defaults exact (output_gain 0, gain 0, gain_scale 100)", linear.detail());
            row (skewed.bad == 0 && skewed.total == 2 * Processor::kNumBands,
                 "L5 raw freq / q defaults within 2e-4 relative of 1000 / 1.0", skewed.detail() + "  " + actual);
        }
    }

    //==========================================================================
    // B. buses
    //==========================================================================
    juce::AudioProcessor::BusesLayout busLayout (const juce::AudioChannelSet& in, const juce::AudioChannelSet& out)
    {
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (in);
        layout.outputBuses.add (out);
        return layout;
    }

    void busRows()
    {
        section ("B. bus layouts");
        using CS = juce::AudioChannelSet;

        auto p = make();

        const bool accepted = p->checkBusesLayoutSupported (busLayout (CS::mono(), CS::mono()))
                           && p->checkBusesLayoutSupported (busLayout (CS::stereo(), CS::stereo()));

        Tally refused;
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::stereo(), CS::mono())), "(2,1)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::mono(), CS::stereo())), "(1,2)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::disabled(), CS::stereo())), "(0,2)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::stereo(), CS::disabled())), "(2,0)");
        refused.check (! p->checkBusesLayoutSupported (busLayout (CS::create5point1(), CS::create5point1())), "5.1 -> 5.1");

        row (accepted, "B1 accepts (1,1) and (2,2)");
        row (refused.bad == 0, "B1 refuses (2,1), (1,2), (0,2), (2,0), 5.1 -> 5.1", refused.detail());
        row (p->getMainBusNumInputChannels() == 2 && p->getMainBusNumOutputChannels() == 2,
             "B1 default layout is (2,2)");
    }

    //==========================================================================
    // P. pass-through
    //==========================================================================
    constexpr int kRenderLength = 9001;   // not a multiple of any block size used
    constexpr double kRenderRate = 48000.0;

    juce::AudioBuffer<float> makeNoise (int numChannels, juce::int64 seed)
    {
        juce::AudioBuffer<float> buffer (numChannels, kRenderLength);
        juce::Random random (seed);

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < kRenderLength; ++i)
                buffer.setSample (channel, i, (random.nextFloat() * 2.0f - 1.0f) * 0.5f);

        return buffer;
    }

    // The P1 comparator: channel count, length and every byte.
    bool sameAudio (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return false;

        const auto bytes = sizeof (float) * (size_t) a.getNumSamples();

        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            if (std::memcmp (a.getReadPointer (channel), b.getReadPointer (channel), bytes) != 0)
                return false;

        return true;
    }

    juce::AudioBuffer<float> render (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input,
                                     int blockSize, bool viaBypassed)
    {
        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, blockSize);
        juce::MidiBuffer midi;

        p.prepareToPlay (kRenderRate, blockSize);

        for (int position = 0; position < length;)
        {
            const int n = juce::jmin (blockSize, length - position);
            block.setSize (numChannels, n, false, false, true);

            for (int channel = 0; channel < numChannels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (viaBypassed)
                p.processBlockBypassed (block, midi);
            else
                p.processBlock (block, midi);

            for (int channel = 0; channel < numChannels; ++channel)
                output.copyFrom (channel, position, block, channel, 0, n);

            position += n;
        }

        p.releaseResources();
        return output;
    }

    void passThroughRows()
    {
        section ("P. pass-through");
        using CS = juce::AudioChannelSet;

        struct Layout { const char* name; int channels; };
        const Layout layouts[] = { { "mono", 1 }, { "stereo", 2 } };
        const int blockSizes[] = { 1, 64, 512, 4096 };
        const char* const variants[] = { "bypass Off", "bypass On", "all random" };

        for (const auto& layout : layouts)
        {
            const auto input = makeNoise (layout.channels, 0x0E0A11CE + layout.channels);
            const auto set = layout.channels == 1 ? CS::mono() : CS::stereo();

            for (const bool viaBypassed : { false, true })
            {
                Tally tally;

                for (int variant = 0; variant < 3; ++variant)
                    for (const int blockSize : blockSizes)
                    {
                        auto p = make();
                        const bool layoutOk = p->setBusesLayout (busLayout (set, set));

                        if (variant == 1)
                            setNormalised (*p, "bypass", 1.0f);
                        else if (variant == 2)
                            setAllRandom (*p, 0x5EED0000 + blockSize);

                        const bool stimulusOk = variant == 0 ? countNonDefault (*p) == 0
                                              : variant == 1 ? findParam (*p, "bypass")->getValue() == 1.0f
                                                             : countNonDefault (*p) > kMinNonDefault;

                        const auto output = render (*p, input, blockSize, viaBypassed);

                        tally.check (layoutOk && stimulusOk && sameAudio (output, input),
                                     juce::String (variants[variant]) + " block " + juce::String (blockSize)
                                         + (layoutOk ? "" : " (layout refused)")
                                         + (stimulusOk ? "" : " (stimulus not applied)"));
                    }

                row (tally.bad == 0 && tally.total == 12,
                     juce::String ("P1 ") + layout.name + (viaBypassed ? " processBlockBypassed" : " processBlock")
                         + ": out memcmp-equal to in (blocks 1/64/512/4096 x Off/On/random)",
                     tally.detail());
            }
        }

        // P1n: the comparator and the stimulus are not vacuous.
        {
            const auto input = makeNoise (2, 0x0E0A11CE + 2);

            auto flipped = input;
            std::uint32_t bits = 0;
            float* sample = flipped.getWritePointer (1) + 4321;
            std::memcpy (&bits, sample, sizeof (bits));
            bits ^= 1u;                                     // one least-significant bit
            std::memcpy (sample, &bits, sizeof (bits));

            juce::AudioBuffer<float> shorter (2, kRenderLength - 1);
            shorter.clear();

            row (sameAudio (input, input) && ! sameAudio (flipped, input) && ! sameAudio (shorter, input),
                 "P1n comparator FAILS on one flipped bit (and on a length mismatch)");

            const double rmsDb = (double) juce::Decibels::gainToDecibels (input.getRMSLevel (0, 0, kRenderLength), -200.0f);
            const double peak = (double) input.getMagnitude (0, kRenderLength);
            row (rmsDb > -20.0 && peak <= 0.5 && peak > 0.45, "P1n input RMS > -20 dBFS, peak 0.5",
                 fmtJ (rmsDb) + " dBFS, peak " + fmtJ (peak));
        }
    }

    //==========================================================================
    // S. state codec
    //==========================================================================
    // Bit-exact comparison of all 196 normalised values. Prints EVERY mismatch
    // (plan P13 wants the IDs and the deltas, not a sample of them).
    bool sameValuesExact (juce::AudioProcessor& a, juce::AudioProcessor& b, const char* rowId, juce::String& detail)
    {
        const auto pa = rangedParams (a);
        const auto pb = rangedParams (b);

        if (pa.size() != pb.size())
        {
            detail = "parameter counts differ";
            return false;
        }

        int bad = 0;

        for (size_t i = 0; i < pa.size(); ++i)
        {
            const float va = pa[i]->getValue();
            const float vb = pb[i]->getValue();

            if (! sameBits (va, vb))
            {
                ++bad;
                std::printf ("      %s mismatch %-16s expected %s  got %s  delta %s\n", rowId,
                             pa[i]->getParameterID().toRawUTF8(), fmt ((double) va).c_str(), fmt ((double) vb).c_str(),
                             fmt ((double) vb - (double) va).c_str());
            }
        }

        detail = juce::String ((int) pa.size() - bad) + "/" + juce::String ((int) pa.size()) + " bit-equal";
        return bad == 0;
    }

    // "Known values applied": S4 and S6 are about their own subject, so they
    // use a 1e-6 window on the normalised value. Exactness is S1's job alone.
    bool sameValuesClose (juce::AudioProcessor& a, juce::AudioProcessor& b, juce::String& detail)
    {
        const auto pa = rangedParams (a);
        const auto pb = rangedParams (b);
        Tally tally;

        for (size_t i = 0; i < pa.size() && i < pb.size(); ++i)
            tally.check (std::abs (pa[i]->getValue() - pb[i]->getValue()) <= 1.0e-6f, pa[i]->getParameterID());

        detail = tally.detail();
        return pa.size() == pb.size() && tally.bad == 0 && tally.total == kExpectedParams;
    }

    bool sameValuesAsSnapshot (juce::AudioProcessor& p, const std::vector<float>& snapshot)
    {
        const auto now = valuesOf (p);

        if (now.size() != snapshot.size())
            return false;

        for (size_t i = 0; i < now.size(); ++i)
            if (! sameBits (now[i], snapshot[i]))
                return false;

        return true;
    }

    void stateRows()
    {
        section ("S. state codec");

        // S1 + S2 share one saved state.
        {
            auto a = make();
            setAllRandom (*a, 0x51A7E001);
            const auto uiA = makeUi (2, 3, 30, 17, 1234, 777);
            setUi (*a, uiA);
            const int nonDefault = countNonDefault (*a);

            const auto blobA = stateOf (*a);

            auto b = make();
            loadState (*b, blobA);

            juce::String detail;
            const bool exact = sameValuesExact (*a, *b, "S1", detail);
            row (exact && nonDefault > kMinNonDefault,"S1 round trip: 196 getValue() bit-equal in a fresh instance",
                 detail + "  (" + juce::String (nonDefault) + " non-default in the source)");

            const auto uiB = uiOf (*b);
            row (uiB == uiA, "S1 UI members restored (mode 3, range 30, band 17, 1234 x 777, zh-Hans)", uiB.describe());

            const auto blobB = stateOf (*b);
            const bool sameBlob = blobA == blobB;

            if (! sameBlob)
            {
                auto xa = xmlOf (blobA);
                auto xb = xmlOf (blobB);

                if (xa != nullptr && xb != nullptr)
                    for (auto* child : xa->getChildIterator())
                        if (child->hasTagName ("PARAM"))
                        {
                            const auto id = child->getStringAttribute ("id");
                            auto* other = paramChild (*xb, id);
                            const auto va = child->getStringAttribute ("value");
                            const auto vb = other != nullptr ? other->getStringAttribute ("value") : juce::String ("<missing>");

                            if (va != vb)
                                std::printf ("      S1 re-save differs %-16s A %s  A->B %s\n",
                                             id.toRawUTF8(), va.toRawUTF8(), vb.toRawUTF8());
                        }
            }

            row (sameBlob, "S1 'A -> save' and 'A -> B -> save' are byte-equal",
                 juce::String ((int) blobA.getSize()) + " / " + juce::String ((int) blobB.getSize()) + " bytes");

            // S2
            const auto xml = xmlOf (blobA);
            const bool parsed = xml != nullptr;
            row (parsed && xml->hasTagName ("OParametricEQ"), "S2 root tag OParametricEQ",
                 parsed ? xml->getTagName() : juce::String ("no XML"));

            if (parsed)
            {
                row (xml->getStringAttribute ("stateVersion") == "1"
                         && xml->getStringAttribute ("pluginVersion").isNotEmpty()
                         && xml->getStringAttribute ("uiLanguage") == "zh-Hans",
                     "S2 stateVersion=\"1\", pluginVersion, uiLanguage=\"zh-Hans\"",
                     "stateVersion " + xml->getStringAttribute ("stateVersion")
                         + ", pluginVersion " + xml->getStringAttribute ("pluginVersion")
                         + ", uiLanguage " + xml->getStringAttribute ("uiLanguage"));

                const int numParam = countChildren (*xml, "PARAM");
                const int numUi = countChildren (*xml, "UI");
                auto* ui = xml->getChildByName ("UI");

                row (numParam == kExpectedParams && numUi == 1 && xml->getNumChildElements() == kExpectedParams + 1,
                     "S2 exactly 196 PARAM and exactly one UI child",
                     juce::String (numParam) + " PARAM, " + juce::String (numUi) + " UI, "
                         + juce::String (xml->getNumChildElements()) + " children");

                row (ui != nullptr && ! ui->hasAttribute ("id")
                         && ui->getIntAttribute ("analyzerMode", -1) == 3
                         && ui->getIntAttribute ("displayRange", -1) == 30
                         && ui->getIntAttribute ("selectedBand", -1) == 17
                         && ui->getIntAttribute ("editorWidth", -1) == 1234
                         && ui->getIntAttribute ("editorHeight", -1) == 777,
                     "S2 UI child: no 'id' attribute; five values present");
            }
            else
            {
                row (false, "S2 attributes / children (no XML to inspect)");
            }
        }

        // S3: two reopens with a UI change between them.
        {
            auto a = make();
            setUi (*a, makeUi (1, 1, 6, 5, 1000, 700));

            auto b = make();
            loadState (*b, stateOf (*a));                    // reopen 1
            const bool first = uiOf (*b) == makeUi (1, 1, 6, 5, 1000, 700);

            const auto changed = makeUi (0, 3, 30, 9, 1500, 900);
            setUi (*b, changed);                             // the change
            const auto blob2 = stateOf (*b);

            auto c = make();
            loadState (*c, blob2);                           // reopen 2, fresh instance
            const bool second = uiOf (*c) == changed;

            setUi (*b, makeUi (2, 0, 12, 1, 801, 501));
            loadState (*b, blob2);                           // reopen 2, same instance
            const bool secondSame = uiOf (*b) == changed;

            const auto xmlB = xmlOf (blob2);
            const auto xmlC = xmlOf (stateOf (*c));
            const auto xmlB2 = xmlOf (stateOf (*b));
            const bool oneUi = xmlB != nullptr && xmlC != nullptr && xmlB2 != nullptr
                            && countChildren (*xmlB, "UI") == 1 && countChildren (*xmlC, "UI") == 1
                            && countChildren (*xmlB2, "UI") == 1
                            && countChildren (*xmlC, "PARAM") == kExpectedParams;

            row (first && second && secondSame, "S3 second reopen shows the changed UI values",
                 "fresh: " + uiOf (*c).describe());
            row (oneUi, "S3 still exactly one UI child after each reopen");
        }

        // S4: unknown parameter.
        {
            auto a = make();
            setAllRandom (*a, 0x51A7E004);
            auto xml = xmlOf (stateOf (*a));
            bool ok = xml != nullptr;
            juce::String detail = "no XML";

            if (ok)
            {
                auto* extra = xml->createNewChildElement ("PARAM");
                extra->setAttribute ("id", "b01_dyn_range");
                extra->setAttribute ("value", "6");

                auto b = make();
                loadState (*b, blobOf (*xml));

                const bool applied = sameValuesClose (*a, *b, detail);
                const auto saved = xmlOf (stateOf (*b));
                auto* kept = saved != nullptr ? paramChild (*saved, "b01_dyn_range") : nullptr;

                ok = applied && countNonDefault (*b) > kMinNonDefault
                  && b->parameters.getParameter ("b01_dyn_range") == nullptr
                  && kept != nullptr && kept->getDoubleAttribute ("value", -1.0) == 6.0
                  && countChildren (*saved, "PARAM") == kExpectedParams + 1
                  && countChildren (*saved, "UI") == 1;

                detail += kept != nullptr ? "  unknown child kept" : "  unknown child LOST";
            }

            row (ok, "S4 unknown parameter: loads, known values applied, child present in the next save", detail);
        }

        // S5: missing parameter.
        {
            auto source = make();
            setRaw (*source, "b07_q", 4.0f);                 // a known value that must still arrive
            setRaw (*source, "b03_gain", -7.5f);
            auto xml = xmlOf (stateOf (*source));
            int removed = 0;

            if (xml != nullptr)
                for (const char* id : { "b07_gain", "b07_freq" })
                    if (auto* child = paramChild (*xml, id))
                    {
                        xml->removeChildElement (child, true);
                        ++removed;
                    }

            auto target = make();
            setRaw (*target, "b07_gain", 12.0f);
            setRaw (*target, "b07_freq", 5000.0f);

            auto* gain = findParam (*target, "b07_gain");
            auto* freq = findParam (*target, "b07_freq");
            auto* q    = findParam (*target, "b07_q");

            // Guard: without the pre-set, "equals the default afterwards" proves nothing.
            const bool preset = gain != nullptr && freq != nullptr && q != nullptr
                             && ! sameBits (gain->getValue(), gain->getDefaultValue())
                             && ! sameBits (freq->getValue(), freq->getDefaultValue())
                             && std::abs (rawOf (*target, "b07_gain") - 12.0f) < 1.0e-3f
                             && std::abs (rawOf (*target, "b07_freq") - 5000.0f) < 1.0f;

            bool ok = false;
            juce::String detail = "blob / pre-set guard failed (removed " + juce::String (removed) + ")";

            if (xml != nullptr && removed == 2 && preset)
            {
                loadState (*target, blobOf (*xml));

                auto fresh = make();
                const float gainRaw = rawOf (*target, "b07_gain");
                const float freqRaw = rawOf (*target, "b07_freq");
                const float freshFreqRaw = rawOf (*fresh, "b07_freq");
                const bool freqBitEqual = sameBits (freqRaw, freshFreqRaw);

                const bool gainOk = sameBits (gainRaw, 0.0f) && sameBits (gain->getValue(), 0.5f);
                const bool freqOk = std::abs ((double) freqRaw - (double) freshFreqRaw) / (double) freshFreqRaw <= 1.0e-6;
                const bool knownOk = std::abs (rawOf (*target, "b07_q") - 4.0f) < 1.0e-3f
                                  && std::abs (rawOf (*target, "b03_gain") + 7.5f) < 1.0e-3f;

                ok = gainOk && freqOk && knownOk;
                detail = "b07_gain raw " + fmtJ ((double) gainRaw) + " norm " + fmtJ ((double) gain->getValue())
                       + "; b07_freq raw " + fmtJ ((double) freqRaw) + " vs fresh " + fmtJ ((double) freshFreqRaw)
                       + (freqBitEqual ? " (bit-equal)" : " (NOT bit-equal)")
                       + " norm " + fmtJ ((double) freq->getValue())
                       + (knownOk ? "" : "; known values NOT applied");
            }

            row (ok, "S5 missing parameter loads at its default (pre-set non-default first)", detail);
        }

        // S6: a later state version.
        {
            auto a = make();
            setAllRandom (*a, 0x51A7E006);
            auto xml = xmlOf (stateOf (*a));
            bool ok = xml != nullptr;
            juce::String detail = "no XML";

            if (ok)
            {
                xml->setAttribute ("stateVersion", "2");

                auto b = make();
                loadState (*b, blobOf (*xml));

                const bool applied = sameValuesClose (*a, *b, detail);
                const auto saved = xmlOf (stateOf (*b));
                const auto version = saved != nullptr ? saved->getStringAttribute ("stateVersion") : juce::String ("no XML");

                ok = applied && countNonDefault (*b) > kMinNonDefault && version == "1";
                detail += "  next save stateVersion " + version;
            }

            row (ok, "S6 stateVersion=\"2\": loads, values applied, next save writes \"1\"", detail);
        }

        // S7: blobs that must change nothing.
        {
            auto source = make();
            setAllRandom (*source, 0x51A7E0A7);              // different from the target on purpose
            setUi (*source, makeUi (2, 3, 30, 24, 1600, 1000));
            const auto valid = stateOf (*source);

            auto target = make();
            setAllRandom (*target, 0x51A7E007);
            const auto uiBefore = makeUi (1, 1, 6, 11, 900, 550);
            setUi (*target, uiBefore);
            const auto snapshot = valuesOf (*target);
            const bool startsNonDefault = countNonDefault (*target) > kMinNonDefault &&! sameValuesAsSnapshot (*source, snapshot);

            Tally tally;

            // Wrong root tag (otherwise a complete, valid state).
            if (auto xml = xmlOf (valid))
            {
                xml->setTagName ("NotOParametricEQ");
                loadState (*target, blobOf (*xml));
                tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "wrong root tag");
            }
            else
            {
                tally.check (false, "no XML for the wrong-root case");
            }

            // Truncated blob (half of a valid one).
            target->setStateInformation (valid.getData(), (int) (valid.getSize() / 2));
            tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "truncated");

            // Truncated inside the 8-byte header.
            target->setStateInformation (valid.getData(), 5);
            tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "5-byte blob");

            // Empty blob.
            const char nothing = 0;
            target->setStateInformation (&nothing, 0);
            tally.check (sameValuesAsSnapshot (*target, snapshot) && uiOf (*target) == uiBefore, "empty");

            // Control: the valid blob DOES change the target (the checks above are not vacuous).
            loadState (*target, valid);
            const bool control = ! sameValuesAsSnapshot (*target, snapshot) && ! (uiOf (*target) == uiBefore);

            row (tally.bad == 0 && startsNonDefault && control,
                 "S7 wrong root / truncated / empty blob: 196 values, UI and language unchanged",
                 tally.detail() + (control ? "" : "  (control: the valid blob changed nothing)")
                     + (startsNonDefault ? "" : "  (starting state not distinct)"));
        }

        // S8: UI validation.
        {
            auto source = make();
            auto xml = xmlOf (stateOf (*source));
            auto* ui = xml != nullptr ? xml->getChildByName ("UI") : nullptr;
            bool invalidOk = false, absentOk = false;
            juce::String detail = "no XML / UI child";

            if (ui != nullptr)
            {
                ui->setAttribute ("analyzerMode", "9");
                ui->setAttribute ("displayRange", "7");
                ui->setAttribute ("selectedBand", "99");
                ui->setAttribute ("editorWidth", "99999");
                ui->setAttribute ("editorHeight", "1");
                xml->setAttribute ("uiLanguage", "xx");

                auto target = make();
                setUi (*target, makeUi (1, 1, 6, 3, 900, 550));
                loadState (*target, blobOf (*xml));

                // Invalid values leave their members alone; the size clamps;
                // an unknown language code is English.
                const auto after = uiOf (*target);
                invalidOk = after == makeUi (0, 1, 6, 3, 1600, 500);
                detail = "invalid: " + after.describe();

                // No UI child and no uiLanguage attribute: nothing moves.
                xml->removeChildElement (ui, true);
                xml->removeAttribute ("uiLanguage");

                auto target2 = make();
                const auto kept = makeUi (2, 3, 30, 21, 1111, 888);
                setUi (*target2, kept);
                loadState (*target2, blobOf (*xml));

                absentOk = uiOf (*target2) == kept && countChildren (*xml, "UI") == 0;
                detail += "; absent: " + uiOf (*target2).describe();
            }

            row (invalidOk, "S8 invalid mode / range / band ignored; width clamps to 1600; 'xx' -> en", detail);
            row (absentOk, "S8 no UI child and no uiLanguage: every UI member unchanged");
        }
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    bool dumpParams = false;
    juce::File goldenDir (juce::String::fromUTF8 (GOLDEN_DIR));

    for (int a = 1; a < argc; ++a)
    {
        const auto arg = juce::String::fromUTF8 (argv[a]);

        if (arg == "--dump-params")
        {
            dumpParams = true;
        }
        else if (arg == "--golden-dir" && a + 1 < argc)
        {
            goldenDir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[++a]));
        }
        else
        {
            std::fprintf (stderr, "usage: %s [--dump-params] [--golden-dir <dir>]\n", argv[0]);
            return 255;
        }
    }

    juce::ScopedJuceInitialiser_GUI juceInit;

    if (dumpParams)
    {
        // stdout only, "\n" endings only: the caller redirects this into
        // tests/golden/params.tsv.
        auto p = make();
        std::cout << buildDump (*p);
        std::cout.flush();
        return 0;
    }

    std::printf ("O-ParametricEQ render harness (Stage 1)  golden dir: %s\n", goldenDir.getFullPathName().toRawUTF8());

    layoutRows (goldenDir);
    busRows();
    passThroughRows();
    stateRows();

    std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
    std::fflush (stdout);
    return juce::jmin (failures, 255);
}
