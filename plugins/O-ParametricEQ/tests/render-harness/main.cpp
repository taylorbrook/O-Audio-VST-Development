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
      L4   bypass parameter; latency 0 at 44.1 k and 96 k; tail 2.0 (amended in Phase 2.2)
      L5   float defaults: normalised exactly 0.5; linear raw exact; freq / q raw within 2e-4
      B1   bus layouts: (1,1) and (2,2) accepted; (2,1), (1,2), (0,2), (2,0), 5.1 refused
      P1   pass-through, memcmp-equal: mono / stereo x blocks 1, 64, 512, 4096 x
           bypass Off / On / every parameter random, and through processBlockBypassed.
           Amended in Phase 2.2: the processBlock "random" variant switches every
           bNN_enabled off and bypass off after the draw (the engine now acts on
           an enabled band); the processBlockBypassed rows keep the full draw
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

    Stage 2, Phase 2.2 (PLAN-2.2 T7): the engine through the plugin's float
    output. Every closed-form reference is designed from the raw parameter
    values (rawOf) through eq::makeBandInput and eq::Designer::design. There is
    no audio golden: a bit-identity row compares two renders of this binary.
    A row named "...n" is a negative control and PASSES when the control fires.
    [INFO] lines are printed and never counted.

      E1   FUNC-01: 24 bells at 48 kHz vs the 24 closed forms; each band disabled in turn
      E1n  the full render against each 23-band closed form must fail
      E2   FUNC-02: eight types at 44.1 and 96 kHz, mono and stereo, <= 0.01 dB above -80 dB
      E2n  the bell against the same bell 1 % higher must fail
      N1   DSP-06: eight types at 25 and 30 kHz, 44.1 kHz: finite, closed form
      N2   24 mixed bands at 11 - 30 kHz, 22.05 and 44.1 kHz: finite, peak <= +40 dB
      T1a  G4 (a): no band enabled, noise + denormal tail: memcmp-equal, 7 block patterns
      T1b  G4 (b): Left bands leave the right channel memcmp-equal; and the converse
      T1bn the same bands on Stereo must fail
      T1c  G4 (c): Mid bands leave the side, Side bands the mid, <= -130 dB re the larger peak
      T1cn the same bands on Left must fail
      T1g  G4 (g): block sizes 32, 64, 512, 2048, 1 and a random sequence: bit-identical
      T1m  mono layout: every placement value equals the Stereo-placement render
      Z1   G8: latency 0, nothing before the impulse, predicted first-sample gain
      A1   G6 static: no allocation inside processBlock (malloc_logger, this thread)
      A1n  a planted allocation inside the window is counted
      K1   host blocks of 1024 on a 256 prepare == blocks of 256
      H1   hard bypass: buffer untouched; state cleared
      U1   a gain edit reaches the new closed form
      U2   re-enabling a band starts from cleared state
      U3   inert edits (slope on Bell / Notch / Band Pass, Q at 6 dB/oct, placement on mono)
      V1   DSP-07 engine half: the dynamic offset reads exactly 0
      X-cpu  settled CPU readings, INFO

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "dsp/EqTypes.h"
#include "dsp/BandParams.h"
#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/Band.h"
#include "dsp/Router.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined (__APPLE__)
 #include <pthread.h>

 // libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach
 // to). Same declaration as the designer harness (rows A1 / A1n).
 extern "C"
 {
     typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                     uintptr_t result, uint32_t numHotFramesToSkip);
     extern malloc_logger_t* malloc_logger;
 }
#endif

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

    // "b07_freq" from (7, "freq").
    juce::String bandId (int band, const char* suffix)
    {
        return "b" + juce::String (band).paddedLeft ('0', 2) + "_" + suffix;
    }

    void disableEveryBand (juce::AudioProcessor& p)
    {
        for (int band = 1; band <= Processor::kNumBands; ++band)
            setNormalised (p, bandId (band, "enabled"), 0.0f);
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
            // Amended in Phase 2.2 (PLAN-2.2 P9, P10): the engine rings.
            row (p->getTailLengthSeconds() == 2.0, "L4 tail 2.0", fmtJ (p->getTailLengthSeconds()));
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

    // What the per-block hook of render() sees, and may change.
    struct BlockInfo
    {
        int index = 0;              // block number, from 0
        int position = 0;           // index of the block's first sample
        bool viaBypassed = false;   // entry point for this block; starts at render()'s argument
    };

    using BlockHook = std::function<void (BlockInfo&)>;

    /** The four trailing arguments are Phase 2.2 additions. Their defaults are
        the Stage 1 behaviour, bit for bit: 48 kHz, no hook, every block
        `blockSize` long, prepared at `blockSize`.

        sampleRate         the rate handed to prepareToPlay
        beforeBlock        called before each block is filled and processed
                           (mid-render parameter edits; it may switch the
                           block's entry point)
        blockSequence      when not empty: the block lengths, cycled, each
                           limited to 1 ... blockSize
        preparedBlockSize  when > 0: the size handed to prepareToPlay instead
                           of blockSize (a host block larger than prepared) */
    juce::AudioBuffer<float> render (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input,
                                     int blockSize, bool viaBypassed,
                                     double sampleRate = kRenderRate,
                                     const BlockHook& beforeBlock = {},
                                     const std::vector<int>& blockSequence = {},
                                     int preparedBlockSize = 0)
    {
        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, blockSize);
        juce::MidiBuffer midi;

        p.prepareToPlay (sampleRate, preparedBlockSize > 0 ? preparedBlockSize : blockSize);

        int index = 0;

        for (int position = 0; position < length; ++index)
        {
            const int wanted = blockSequence.empty()
                                   ? blockSize
                                   : juce::jlimit (1, blockSize, blockSequence[(size_t) index % blockSequence.size()]);
            const int n = juce::jmin (wanted, length - position);

            BlockInfo current;
            current.index = index;
            current.position = position;
            current.viaBypassed = viaBypassed;

            if (beforeBlock)
                beforeBlock (current);

            block.setSize (numChannels, n, false, false, true);

            for (int channel = 0; channel < numChannels; ++channel)
                block.copyFrom (channel, 0, input, channel, position, n);

            if (current.viaBypassed)
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

        // Amended in Phase 2.2 (PLAN-2.2 P10): through processBlock the engine
        // acts on an enabled band and on the bypass switch, so "random settings
        // pass through" is no longer a property of the plugin. The variant keeps
        // the full draw (output_gain and auto_gain included) and then switches
        // every bNN_enabled off and bypass off. The stimulus guard is unchanged.
        // The processBlockBypassed rows keep the full draw.
        const char* const amendedRandom = "all random, every bNN_enabled off, bypass off";

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
                        {
                            setNormalised (*p, "bypass", 1.0f);
                        }
                        else if (variant == 2)
                        {
                            setAllRandom (*p, 0x5EED0000 + blockSize);

                            if (! viaBypassed)
                            {
                                disableEveryBand (*p);
                                setNormalised (*p, "bypass", 0.0f);
                            }
                        }

                        const bool stimulusOk = variant == 0 ? countNonDefault (*p) == 0
                                              : variant == 1 ? findParam (*p, "bypass")->getValue() == 1.0f
                                                             : countNonDefault (*p) > kMinNonDefault;

                        const auto output = render (*p, input, blockSize, viaBypassed);

                        tally.check (layoutOk && stimulusOk && sameAudio (output, input),
                                     juce::String (variant == 2 && ! viaBypassed ? amendedRandom : variants[variant])
                                         + " block " + juce::String (blockSize)
                                         + (layoutOk ? "" : " (layout refused)")
                                         + (stimulusOk ? "" : " (stimulus not applied)"));
                    }

                row (tally.bad == 0 && tally.total == 12,
                     juce::String ("P1 ") + layout.name + (viaBypassed ? " processBlockBypassed" : " processBlock")
                         + (viaBypassed ? ": out memcmp-equal to in (blocks 1/64/512/4096 x Off/On/random)"
                                        : ": out memcmp-equal to in (blocks 1/64/512/4096 x Off/On/all random, every bNN_enabled off, bypass off)"),
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

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.2: the engine through the plugin (PLAN-2.2 T7)
    //==========================================================================
    //==========================================================================

    // Printed, never counted: the last line stays "N PASS, M FAIL".
    void info (const juce::String& text)
    {
        std::printf ("  [INFO] %s\n", text.toRawUTF8());
        std::fflush (stdout);
    }

    juce::String fmtG (double value, int digits = 3)
    {
        char text[64];
        std::snprintf (text, sizeof (text), "%.*g", digits, value);
        return juce::String (text);
    }

    // Choice indices (parameter-spec; eq::Type and eq::Placement carry the same values).
    constexpr int kBell = 0, kLowShelf = 1, kHighShelf = 2, kLowCut = 3, kHighCut = 4,
                  kNotch = 5, kBandPass = 6, kTilt = 7;
    constexpr int kStereo = 0, kLeft = 1, kRight = 2, kMid = 3, kSide = 4;

    const char* const kTypeNames[8] = { "Bell", "Low Shelf", "High Shelf", "Low Cut",
                                        "High Cut", "Notch", "Band Pass", "Tilt" };
    const char* const kPlacementNames[5] = { "Stereo", "Left", "Right", "Mid", "Side" };

    constexpr float kNoise12 = 0.25f;       // white noise peak, -12 dBFS
    constexpr int kNoiseLength = 16384;     // the noise part of "noise+tail"
    constexpr int kTailLength = 2048;       // the denormal part
    constexpr int kSequenceMax = 1024;      // largest block of the random block sequence

    //==========================================================================
    // Band settings
    //==========================================================================
    struct BandSpec
    {
        int type = 0;
        float freq = 1000.0f;
        float gain = 0.0f;
        float q = 1.0f;
        int slope = 1;
        int placement = 0;
    };

    BandSpec makeSpec (int type, float freq, float gain, float q, int slope, int placement)
    {
        BandSpec s;
        s.type = type;
        s.freq = freq;
        s.gain = gain;
        s.q = q;
        s.slope = slope;
        s.placement = placement;
        return s;
    }

    // Writes the band's six settings, clears its bypass and enables it.
    void setBand (juce::AudioProcessor& p, int band, const BandSpec& s)
    {
        setRaw (p, bandId (band, "type"), (float) s.type);
        setRaw (p, bandId (band, "freq"), s.freq);
        setRaw (p, bandId (band, "gain"), s.gain);
        setRaw (p, bandId (band, "q"), s.q);
        setRaw (p, bandId (band, "slope"), (float) s.slope);
        setRaw (p, bandId (band, "placement"), (float) s.placement);
        setRaw (p, bandId (band, "bypass"), 0.0f);
        setRaw (p, bandId (band, "enabled"), 1.0f);
    }

    bool setLayout (juce::AudioProcessor& p, int channels)
    {
        const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
        return p.setBusesLayout (busLayout (set, set));
    }

    // A fresh instance on a mono or stereo layout with bands 1 ... numBands set
    // from specOf (0 ... numBands - 1).
    template <typename SpecFn>
    std::unique_ptr<Processor> makeBands (int channels, int numBands, SpecFn specOf)
    {
        auto p = make();

        if (! setLayout (*p, channels))
            row (false, "harness: layout refused (" + juce::String (channels) + " channels)");

        for (int index = 0; index < numBands; ++index)
            setBand (*p, index + 1, specOf (index));

        return p;
    }

    // The 24-band mixed setting of T1g, K1, H1 and V1: every type, every slope,
    // every placement, gains +-6 dB (PLAN-2.2 P12: bounded). Cuts sit at the
    // band edges and the band passes are wide, so the output keeps its level.
    BandSpec mixedSpec (int index)
    {
        static const float qs[4] = { 0.7f, 1.0f, 1.4f, 2.0f };

        BandSpec s;
        s.type = index % 8;
        s.slope = (index * 3 + 1) % 8;
        s.placement = index % 5;
        s.freq = 40.0f * std::pow (400.0f, (float) ((index * 7) % 24) / 23.0f);
        s.gain = ((index / 2 + index / 8) % 2 == 0) ? 6.0f : -6.0f;
        s.q = qs[index % 4];

        if (s.type == kLowCut)
            s.freq = 30.0f + 15.0f * (float) (index / 8);

        if (s.type == kHighCut)
            s.freq = 12000.0f + 3000.0f * (float) (index / 8);

        if (s.type == kBandPass)
            s.q = 0.25f;

        return s;
    }

    // The six bands of T1b / T1bn, on one placement.
    BandSpec channelBandSpec (int index, int placement)
    {
        const BandSpec table[6] =
        {
            makeSpec (kBell,      500.0f,   6.0f, 1.5f, 1, kStereo),
            makeSpec (kLowShelf,  150.0f,   4.0f, 1.0f, 3, kStereo),
            makeSpec (kHighShelf, 6000.0f, -5.0f, 1.0f, 1, kStereo),
            makeSpec (kLowCut,    60.0f,    0.0f, 1.0f, 5, kStereo),
            makeSpec (kNotch,     2000.0f,  0.0f, 4.0f, 1, kStereo),
            makeSpec (kTilt,      1000.0f,  3.0f, 1.0f, 1, kStereo)
        };

        BandSpec s = table[index % 6];
        s.placement = placement;
        return s;
    }

    // The bells of T1c / T1cn: Q 1, 40 Hz - 16 kHz log-spaced (one band: 800 Hz).
    BandSpec msBellSpec (int index, int numBands, float gainDb, int placement)
    {
        const float freq = numBands > 1 ? 40.0f * std::pow (400.0f, (float) index / (float) (numBands - 1)) : 800.0f;
        return makeSpec (kBell, freq, gainDb, 1.0f, 1, placement);
    }

    //==========================================================================
    // References: raw values -> makeBandInput -> Designer::design
    //==========================================================================
    eq::RawBand rawBandOf (Processor& p, int band)
    {
        eq::RawBand raw;
        raw.enabled   = rawOf (p, bandId (band, "enabled"));
        raw.bypass    = rawOf (p, bandId (band, "bypass"));
        raw.type      = rawOf (p, bandId (band, "type"));
        raw.freq      = rawOf (p, bandId (band, "freq"));
        raw.gain      = rawOf (p, bandId (band, "gain"));
        raw.q         = rawOf (p, bandId (band, "q"));
        raw.slope     = rawOf (p, bandId (band, "slope"));
        raw.placement = rawOf (p, bandId (band, "placement"));
        return raw;
    }

    // band is 1 ... 24. freqFactor scales the raw frequency (E2n only).
    // getBandDynOffsetDb is read with a 0-based index.
    eq::BandInput inputOf (Processor& p, int band, float freqFactor = 1.0f)
    {
        eq::RawBand raw = rawBandOf (p, band);
        raw.freq *= freqFactor;
        return eq::makeBandInput (raw, rawOf (p, "gain_scale"), p.getBandDynOffsetDb (band - 1));
    }

    int activeBands (Processor& p)
    {
        int n = 0;

        for (int band = 1; band <= Processor::kNumBands; ++band)
            if (inputOf (p, band).active)
                ++n;

        return n;
    }

    int enabledBands (Processor& p)
    {
        int n = 0;

        for (int band = 1; band <= Processor::kNumBands; ++band)
            if (rawOf (p, bandId (band, "enabled")) >= 0.5f)
                ++n;

        return n;
    }

    // Designer::prepare before the first design at every rate.
    std::unique_ptr<eq::Designer> designerFor (double rate)
    {
        auto designer = std::make_unique<eq::Designer>();
        designer->prepare (rate);
        return designer;
    }

    // PLAN-2.2 P11: 160 log-spaced points, 20 Hz to min (20 kHz, 0.499 fs).
    std::vector<double> responseGrid (double rate)
    {
        constexpr int numPoints = 160;
        const double top = juce::jmin (20000.0, 0.499 * rate);
        std::vector<double> hz;

        for (int j = 0; j < numPoints; ++j)
            hz.push_back (20.0 * std::pow (top / 20.0, (double) j / (double) (numPoints - 1)));

        return hz;
    }

    // Sum in dB of the closed forms of the active bands (all on one lane: the
    // rows that use this keep every band on Stereo). excludeBand (1 ... 24)
    // leaves one band out; 0 leaves none out.
    std::vector<double> chainRefDb (Processor& p, const eq::Designer& designer, double rate,
                                    const std::vector<double>& hz, int excludeBand = 0, float freqFactor = 1.0f)
    {
        std::vector<double> db (hz.size(), 0.0);

        for (int band = 1; band <= Processor::kNumBands; ++band)
        {
            if (band == excludeBand)
                continue;

            const auto in = inputOf (p, band, freqFactor);

            if (! in.active)
                continue;

            eq::BandDesign design {};
            designer.design (in.settings, design);

            for (size_t j = 0; j < hz.size(); ++j)
                db[j] += eq::bandMagDb (design, 2.0 * eq::pi * hz[j] / rate);
        }

        return db;
    }

    //==========================================================================
    // Impulse response and single-bin DFT (ported from research-2.2 engine_bench modeIr)
    //==========================================================================
    struct Impulse
    {
        std::vector<std::vector<float>> channels;
        bool tailReached = false;
        bool finite = true;
        double seconds = 0.0;

        bool ok() const     { return tailReached && finite; }
    };

    /** PLAN-2.2 P11: an impulse on every channel (amplitudes[channel]) at
        sample 0, rendered in 1 s steps until the last 10 ms is at least
        tailDb below the peak. Cap 20 s: tailReached stays false there, and
        the calling row fails. */
    Impulse renderImpulse (juce::AudioProcessor& p, int numChannels, double rate,
                           const float* amplitudes, double tailDb = 140.0)
    {
        constexpr int blockSize = 512;
        constexpr int capSeconds = 20;
        const int step = (int) std::lround (rate);
        const int tailLength = (int) std::lround (rate * 0.010);
        const double tailRatio = std::pow (10.0, -tailDb / 20.0);

        Impulse result;
        result.channels.resize ((size_t) numChannels);

        juce::AudioBuffer<float> block (numChannels, blockSize);
        juce::MidiBuffer midi;

        p.prepareToPlay (rate, blockSize);

        int position = 0;

        for (int second = 1; second <= capSeconds; ++second)
        {
            const int end = second * step;

            while (position < end)
            {
                const int n = juce::jmin (blockSize, end - position);
                block.setSize (numChannels, n, false, false, true);
                block.clear();

                if (position == 0)
                    for (int channel = 0; channel < numChannels; ++channel)
                        block.setSample (channel, 0, amplitudes[channel]);

                p.processBlock (block, midi);

                for (int channel = 0; channel < numChannels; ++channel)
                {
                    const float* data = block.getReadPointer (channel);
                    auto& store = result.channels[(size_t) channel];
                    store.insert (store.end(), data, data + n);
                }

                position += n;
            }

            double peak = 0.0, tail = 0.0;

            for (const auto& channel : result.channels)
            {
                const int total = (int) channel.size();

                for (int i = 0; i < total; ++i)
                {
                    const double v = std::abs ((double) channel[(size_t) i]);

                    if (! std::isfinite (v))
                        result.finite = false;

                    peak = juce::jmax (peak, v);

                    if (i >= total - tailLength)
                        tail = juce::jmax (tail, v);
                }
            }

            result.seconds = (double) position / rate;

            if (! result.finite)
                break;

            if (peak > 0.0 && tail <= peak * tailRatio)
            {
                result.tailReached = true;
                break;
            }
        }

        p.releaseResources();
        return result;
    }

    juce::String describe (const Impulse& ir)
    {
        return fmtG (ir.seconds) + " s rendered" + (ir.tailReached ? "" : ", TAIL NOT REACHED")
             + (ir.finite ? "" : ", NOT FINITE");
    }

    bool sameImpulse (const Impulse& a, const Impulse& b)
    {
        if (a.channels.size() != b.channels.size())
            return false;

        for (size_t channel = 0; channel < a.channels.size(); ++channel)
        {
            const auto& x = a.channels[channel];
            const auto& y = b.channels[channel];

            if (x.size() != y.size() || std::memcmp (x.data(), y.data(), sizeof (float) * x.size()) != 0)
                return false;
        }

        return true;
    }

    // Magnitude in dB at each frequency: a running phasor, renormalised every
    // 4096 samples. `amplitude` is the impulse's.
    std::vector<double> dftDb (const std::vector<float>& h, double amplitude, double rate, const std::vector<double>& hz)
    {
        std::vector<double> db;

        for (const double f : hz)
        {
            const double w = 2.0 * eq::pi * f / rate;
            const double cr = std::cos (w), ci = -std::sin (w);
            double pr = 1.0, pim = 0.0, ar = 0.0, ai = 0.0;

            for (size_t n = 0; n < h.size(); ++n)
            {
                const double v = (double) h[n];
                ar += v * pr;
                ai += v * pim;

                const double t = pr * cr - pim * ci;
                pim = pr * ci + pim * cr;
                pr = t;

                if ((n & 4095u) == 4095u)
                {
                    const double m = std::sqrt (pr * pr + pim * pim);
                    pr /= m;
                    pim /= m;
                }
            }

            db.push_back (eq::powerToDb ((ar * ar + ai * ai) / (amplitude * amplitude)));
        }

        return db;
    }

    std::vector<std::vector<double>> measureImpulse (const Impulse& ir, const float* amplitudes, double rate,
                                                     const std::vector<double>& hz)
    {
        std::vector<std::vector<double>> measured;

        for (size_t channel = 0; channel < ir.channels.size(); ++channel)
            measured.push_back (dftDb (ir.channels[channel], (double) amplitudes[channel], rate, hz));

        return measured;
    }

    // Worst |measured - reference| over every channel, where the reference is
    // above -80 dB.
    struct Score
    {
        double worst = 0.0;
        int scored = 0;
        bool invalid = false;

        bool within (double limitDb) const   { return ! invalid && scored > 0 && worst <= limitDb; }

        // For the "...n" rows: a real miss, not an empty or broken comparison.
        bool misses (double limitDb) const   { return ! invalid && scored > 0 && worst > limitDb; }
    };

    Score scoreChannels (const std::vector<std::vector<double>>& measured, const std::vector<double>& reference)
    {
        Score s;

        if (measured.empty())
            s.invalid = true;

        for (const auto& channel : measured)
        {
            if (channel.size() != reference.size())
            {
                s.invalid = true;
                continue;
            }

            for (size_t j = 0; j < reference.size(); ++j)
                if (reference[j] > -80.0)
                {
                    const double d = std::abs (channel[j] - reference[j]);
                    ++s.scored;

                    if (! std::isfinite (d))
                        s.invalid = true;
                    else
                        s.worst = juce::jmax (s.worst, d);
                }
        }

        return s;
    }

    //==========================================================================
    // Stimuli and comparators
    //==========================================================================
    juce::AudioBuffer<float> makeNoiseAt (int numChannels, int length, juce::int64 seed, float amplitude)
    {
        juce::AudioBuffer<float> buffer (numChannels, length);
        juce::Random random (seed);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* data = buffer.getWritePointer (channel);

            for (int i = 0; i < length; ++i)
                data[i] = (random.nextFloat() * 2.0f - 1.0f) * amplitude;
        }

        return buffer;
    }

    /** "noise+tail": white noise at -12 dBFS, then 2048 samples with magnitudes
        in 1e-41 ... 1e-39, random sign. The tail is written by bit pattern
        (a denormal float is bits x 1.4013e-45), so it does not depend on this
        thread's floating-point mode. */
    juce::AudioBuffer<float> makeNoiseTail (int numChannels, juce::int64 seed)
    {
        constexpr int bitsMin = 7137;       // 1.0001e-41
        constexpr int bitsMax = 713600;     // 9.9997e-40

        auto buffer = makeNoiseAt (numChannels, kNoiseLength + kTailLength, seed, kNoise12);
        juce::Random random (seed + 0x7A11);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* data = buffer.getWritePointer (channel) + kNoiseLength;

            for (int i = 0; i < kTailLength; ++i)
            {
                auto bits = (std::uint32_t) (bitsMin + random.nextInt (bitsMax - bitsMin + 1));

                if (random.nextBool())
                    bits |= 0x80000000u;

                std::memcpy (data + i, &bits, sizeof (bits));
            }
        }

        return buffer;
    }

    // False when this thread flushes denormal results to zero: the stimulus
    // and the comparisons need it not to.
    bool keepsDenormals()
    {
        volatile float a = 1.0e-30f;
        volatile float b = 1.0e-10f;
        const float product = a * b;
        return product != 0.0f && std::fpclassify (product) == FP_SUBNORMAL;
    }

    // Every sample of the last 2048 on every channel is a non-zero denormal
    // float in 1e-41 ... 1e-39.
    bool hasDenormalTail (const juce::AudioBuffer<float>& buffer)
    {
        if (! keepsDenormals() || buffer.getNumSamples() < kTailLength || buffer.getNumChannels() < 1)
            return false;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* data = buffer.getReadPointer (channel, buffer.getNumSamples() - kTailLength);

            for (int i = 0; i < kTailLength; ++i)
            {
                const double magnitude = std::abs ((double) data[i]);

                if (std::fpclassify (data[i]) != FP_SUBNORMAL || magnitude < 1.0e-41 || magnitude > 1.0e-39)
                    return false;
            }
        }

        return true;
    }

    bool allFinite (const juce::AudioBuffer<float>& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* data = buffer.getReadPointer (channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                if (! std::isfinite (data[i]))
                    return false;
        }

        return true;
    }

    bool sameChannel (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int channel)
    {
        return a.getNumSamples() == b.getNumSamples()
            && channel < a.getNumChannels() && channel < b.getNumChannels()
            && std::memcmp (a.getReadPointer (channel), b.getReadPointer (channel),
                            sizeof (float) * (size_t) a.getNumSamples()) == 0;
    }

    // n samples of every channel: a from startA against b from startB.
    bool sameFrom (const juce::AudioBuffer<float>& a, int startA, const juce::AudioBuffer<float>& b, int startB, int n)
    {
        if (a.getNumChannels() != b.getNumChannels() || n <= 0 || startA < 0 || startB < 0
            || startA + n > a.getNumSamples() || startB + n > b.getNumSamples())
            return false;

        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            if (std::memcmp (a.getReadPointer (channel, startA), b.getReadPointer (channel, startB),
                             sizeof (float) * (size_t) n) != 0)
                return false;

        return true;
    }

    juce::AudioBuffer<float> sliceOf (const juce::AudioBuffer<float>& source, int start, int length)
    {
        juce::AudioBuffer<float> out (source.getNumChannels(), length);

        for (int channel = 0; channel < source.getNumChannels(); ++channel)
            out.copyFrom (channel, 0, source, channel, start, length);

        return out;
    }

    // Seeded block lengths in 1 ... maxSize.
    std::vector<int> randomBlockSequence (juce::int64 seed, int maxSize)
    {
        juce::Random random (seed);
        std::vector<int> sizes;

        for (int i = 0; i < 97; ++i)
            sizes.push_back (1 + random.nextInt (maxSize));

        return sizes;
    }

    // pattern > 0: that block size throughout. pattern == 0: the seeded random
    // sequence (prepared at its largest block).
    juce::AudioBuffer<float> renderPattern (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input, int pattern)
    {
        if (pattern > 0)
            return render (p, input, pattern, false);

        return render (p, input, kSequenceMax, false, kRenderRate, {}, randomBlockSequence (0x5E0BE9, kSequenceMax));
    }

    juce::String patternName (int pattern)
    {
        return pattern > 0 ? "block " + juce::String (pattern) : juce::String ("random sequence");
    }

    //==========================================================================
    // E. response (FUNC-01, FUNC-02)
    //==========================================================================
    void responseRows()
    {
        section ("E. engine response through the plugin (Phase 2.2: FUNC-01, FUNC-02)");

        constexpr double limitDb = 0.01;
        const float amplitudes[2] = { 1.0f, 0.5f };     // distinct, so a channel mix-up shows

        // E1, E1n
        {
            const double rate = 48000.0;
            const auto designer = designerFor (rate);
            const auto hz = responseGrid (rate);

            auto p = makeBands (2, 24, [] (int index)
            {
                return makeSpec (kBell, 40.0f * std::pow (400.0f, (float) index / 23.0f),
                                 index % 2 == 0 ? 3.0f : -3.0f, 2.0f, 1, kStereo);
            });

            const auto full = renderImpulse (*p, 2, rate, amplitudes);
            const auto fullMeasured = measureImpulse (full, amplitudes, rate, hz);
            const auto fullScore = scoreChannels (fullMeasured, chainRefDb (*p, *designer, rate, hz));

            row (full.ok() && activeBands (*p) == 24 && fullScore.within (limitDb),
                 "E1 (i) 24 bells +-3 dB, Q 2, 48 kHz stereo: response vs the 24 closed forms <= 0.01 dB above -80 dB",
                 "worst " + fmtG (fullScore.worst) + " dB over " + juce::String (fullScore.scored) + " points; " + describe (full));

            // E1n: the comparison can see one missing band.
            Tally control;
            double smallestMiss = std::numeric_limits<double>::infinity();

            for (int band = 1; band <= Processor::kNumBands; ++band)
            {
                const auto s = scoreChannels (fullMeasured, chainRefDb (*p, *designer, rate, hz, band));
                control.check (s.misses (limitDb), "without band " + juce::String (band) + ": " + fmtG (s.worst) + " dB");
                smallestMiss = juce::jmin (smallestMiss, s.worst);
            }

            // E1 (ii): each band disabled in turn.
            Tally independent;
            double worstPartial = 0.0;

            for (int band = 1; band <= Processor::kNumBands; ++band)
            {
                setRaw (*p, bandId (band, "enabled"), 0.0f);

                const auto partial = renderImpulse (*p, 2, rate, amplitudes);
                const auto s = scoreChannels (measureImpulse (partial, amplitudes, rate, hz),
                                              chainRefDb (*p, *designer, rate, hz));
                const bool differs = ! sameImpulse (partial, full);

                independent.check (partial.ok() && activeBands (*p) == 23 && differs && s.within (limitDb),
                                   "band " + juce::String (band) + (differs ? "" : " (render unchanged)")
                                       + ": " + fmtG (s.worst) + " dB, " + describe (partial));
                worstPartial = juce::jmax (worstPartial, s.worst);

                setRaw (*p, bandId (band, "enabled"), 1.0f);
            }

            row (independent.bad == 0 && independent.total == 24,
                 "E1 (ii) each band disabled in turn: render differs from the full one, matches its 23-band closed form <= 0.01 dB",
                 independent.detail() + "  worst " + fmtG (worstPartial) + " dB");

            row (control.bad == 0 && control.total == 24,
                 "E1n the full render against each 23-band closed form FAILS the 0.01 dB test (control fires)",
                 control.detail() + "  smallest miss " + fmtG (smallestMiss) + " dB");
        }

        // E2, E2n (PLAN-2.2 P11)
        {
            struct Case { const char* name; BandSpec spec; };

            const Case cases[] =
            {
                { "Bell 1 kHz +12 dB Q 1",              makeSpec (kBell,      1000.0f,  12.0f, 1.0f,  1, kStereo) },
                { "Low Shelf 200 Hz +9 dB 24 dB/oct",   makeSpec (kLowShelf,  200.0f,    9.0f, 1.0f,  3, kStereo) },
                { "High Shelf 8 kHz -12 dB 12 dB/oct",  makeSpec (kHighShelf, 8000.0f, -12.0f, 1.0f,  1, kStereo) },
                { "Low Cut 80 Hz 48 dB/oct",            makeSpec (kLowCut,    80.0f,     0.0f, 1.0f,  5, kStereo) },
                { "High Cut 6 kHz 96 dB/oct",           makeSpec (kHighCut,   6000.0f,   0.0f, 1.0f,  7, kStereo) },
                { "Notch 1 kHz Q 10",                   makeSpec (kNotch,     1000.0f,   0.0f, 10.0f, 1, kStereo) },
                { "Band Pass 500 Hz Q 2",               makeSpec (kBandPass,  500.0f,    0.0f, 2.0f,  1, kStereo) },
                { "Tilt 1 kHz +6 dB 12 dB/oct",         makeSpec (kTilt,      1000.0f,   6.0f, 1.0f,  1, kStereo) }
            };

            Tally higher;

            for (const double rate : { 44100.0, 96000.0 })
            {
                const auto designer = designerFor (rate);
                const auto hz = responseGrid (rate);

                for (const int channels : { 1, 2 })
                {
                    Tally tally;
                    double worst = 0.0;
                    const juce::String where = fmtG (rate / 1000.0, 4) + " kHz " + (channels == 1 ? "mono" : "stereo");

                    for (const auto& c : cases)
                    {
                        auto p = makeBands (channels, 1, [&c] (int) { return c.spec; });

                        const auto ir = renderImpulse (*p, channels, rate, amplitudes);
                        const auto measured = measureImpulse (ir, amplitudes, rate, hz);
                        const auto s = scoreChannels (measured, chainRefDb (*p, *designer, rate, hz));

                        tally.check (ir.ok() && activeBands (*p) == 1 && s.within (limitDb),
                                     juce::String (c.name) + ": " + fmtG (s.worst) + " dB, " + describe (ir));
                        worst = juce::jmax (worst, s.worst);

                        // The figures the summary needs: worst error per type, rate and layout.
                        std::printf ("      E2 %-16s %-34s worst %-9s dB  (%d points above -80 dB, %s)\n",
                                     where.toRawUTF8(), c.name, fmtG (s.worst).toRawUTF8(), s.scored,
                                     describe (ir).toRawUTF8());

                        // E2n: the bell against the same bell 1 % higher in frequency.
                        if (&c == &cases[0])
                        {
                            const auto h = scoreChannels (measured, chainRefDb (*p, *designer, rate, hz, 0, 1.01f));
                            higher.check (h.misses (limitDb), where + ": " + fmtG (h.worst) + " dB");
                        }
                    }

                    row (tally.bad == 0 && tally.total == 8,
                         "E2 " + where + ": 8 types vs the closed form <= 0.01 dB above -80 dB; tail >= 140 dB down",
                         tally.detail() + "  worst " + fmtG (worst) + " dB");
                }
            }

            row (higher.bad == 0 && higher.total == 4,
                 "E2n the bell against the same bell 1 % higher in frequency FAILS the 0.01 dB test (control fires)",
                 higher.detail());
        }
    }

    //==========================================================================
    // N. above Nyquist (DSP-06)
    //==========================================================================
    void nyquistRows()
    {
        section ("N. above Nyquist (Phase 2.2: DSP-06)");

        // N1
        {
            const double rate = 44100.0;
            const auto designer = designerFor (rate);
            const auto hz = responseGrid (rate);
            const float amplitude[1] = { 1.0f };

            Tally tally;
            double worst = 0.0;
            int fewestPoints = std::numeric_limits<int>::max();

            for (int type = 0; type < 8; ++type)
                for (const float freq : { 25000.0f, 30000.0f })
                    for (const float gain : { 12.0f, -12.0f })
                    {
                        const float q = (type == kNotch || type == kBandPass) ? 0.5f : 1.0f;
                        auto p = makeBands (1, 1, [&] (int) { return makeSpec (type, freq, gain, q, 1, kStereo); });

                        const auto ir = renderImpulse (*p, 1, rate, amplitude);
                        const auto s = scoreChannels (measureImpulse (ir, amplitude, rate, hz),
                                                      chainRefDb (*p, *designer, rate, hz));

                        tally.check (ir.ok() && activeBands (*p) == 1 && s.within (0.01),
                                     juce::String (kTypeNames[type]) + " " + fmtG ((double) freq, 5) + " Hz "
                                         + fmtG ((double) gain) + " dB: " + fmtG (s.worst) + " dB, "
                                         + juce::String (s.scored) + " points, " + describe (ir));
                        worst = juce::jmax (worst, s.worst);
                        fewestPoints = juce::jmin (fewestPoints, s.scored);
                    }

            row (tally.bad == 0 && tally.total == 32,
                 "N1 44.1 kHz mono: 8 types at 25 kHz and 30 kHz, +-12 dB, 12 dB/oct: finite; closed form <= 0.01 dB above -80 dB",
                 tally.detail() + "  worst " + fmtG (worst) + " dB; fewest points scored " + juce::String (fewestPoints));
        }

        // N2
        {
            Tally tally;
            juce::String figures;

            for (const double rate : { 22050.0, 44100.0 })
            {
                const int length = (int) std::lround (rate * 5.0);
                auto input = makeNoiseAt (2, length, 0x0E2A0002, kNoise12);

                for (int channel = 0; channel < 2; ++channel)
                    input.clear (channel, length / 2, length - length / 2);     // noise, then silence

                auto p = makeBands (2, 24, [] (int index)
                {
                    return makeSpec (index % 8, 11000.0f * std::pow (30000.0f / 11000.0f, (float) index / 23.0f),
                                     ((index + index / 8) % 2 == 0) ? 6.0f : -6.0f, 1.0f, (index * 3) % 8, kStereo);
                });

                const auto output = render (*p, input, 512, false, rate);

                const bool finite = allFinite (output);
                const double inPeak = (double) input.getMagnitude (0, length);
                const double outPeak = finite ? (double) output.getMagnitude (0, length) : 0.0;
                const double peakDb = 20.0 * std::log10 (juce::jmax (outPeak, 1.0e-300) / inPeak);
                const bool changed = ! sameAudio (output, input);

                tally.check (finite && changed && activeBands (*p) == 24 && peakDb <= 40.0,
                             fmtG (rate / 1000.0, 5) + " kHz" + (finite ? "" : " NOT FINITE")
                                 + (changed ? "" : " (output equals input)") + " peak " + fmtG (peakDb, 4) + " dB");

                figures += (figures.isEmpty() ? "" : "; ") + fmtG (rate / 1000.0, 5) + " kHz peak "
                         + fmtG (peakDb, 4) + " dB re input";
            }

            row (tally.bad == 0 && tally.total == 2,
                 "N2 24 mixed bands at 11 - 30 kHz, +-6 dB, 22.05 and 44.1 kHz, 5 s noise then silence: finite; peak <= +40 dB re input",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // T. transparency (G4 a, b, c, g; mono placement)
    //==========================================================================
    struct MsFigures
    {
        double untouchedDb = 0.0;   // the component the bands must leave, re the larger peak
        double touchedDb = 0.0;     // the component they act on, re the larger peak
        double outPeakDb = 0.0;     // output peak re input peak
    };

    // sideBands: the bands act on the side, so the mid is the untouched one.
    MsFigures msFigures (const juce::AudioBuffer<float>& in, const juce::AudioBuffer<float>& out, bool sideBands)
    {
        double inPeak = 0.0, outPeak = 0.0, midError = 0.0, sideError = 0.0;

        for (int i = 0; i < in.getNumSamples(); ++i)
        {
            const double inL = (double) in.getSample (0, i),   inR = (double) in.getSample (1, i);
            const double outL = (double) out.getSample (0, i), outR = (double) out.getSample (1, i);

            inPeak = juce::jmax (inPeak, std::abs (inL), std::abs (inR));
            outPeak = juce::jmax (outPeak, std::abs (outL), std::abs (outR));

            const double mid = std::abs (0.5 * (outL + outR) - 0.5 * (inL + inR));
            const double side = std::abs (0.5 * (outL - outR) - 0.5 * (inL - inR));

            // A NaN must not be lost in a max.
            if (! (mid <= midError))
                midError = mid;

            if (! (side <= sideError))
                sideError = side;
        }

        const double reference = juce::jmax (inPeak, outPeak);
        const double midDb = 20.0 * std::log10 (juce::jmax (midError, 1.0e-300) / reference);
        const double sideDb = 20.0 * std::log10 (juce::jmax (sideError, 1.0e-300) / reference);

        MsFigures f;
        f.untouchedDb = std::isnan (midError) || std::isnan (sideError) ? std::numeric_limits<double>::quiet_NaN()
                                                                        : (sideBands ? midDb : sideDb);
        f.touchedDb = sideBands ? sideDb : midDb;
        f.outPeakDb = 20.0 * std::log10 (juce::jmax (outPeak, 1.0e-300) / inPeak);
        return f;
    }

    void transparencyRows()
    {
        section ("T. transparency (Phase 2.2: G4 a, b, c, g; mono placement)");

        // Block sizes 1, 16, 64, 512, 1024, 4096 and the seeded random sequence (0).
        const int t1Patterns[] = { 1, 16, 64, 512, 1024, 4096, 0 };

        // T1a
        for (const int channels : { 1, 2 })
        {
            const auto input = makeNoiseTail (channels, 0x71A00000 + channels);

            Tally tally;
            tally.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");

            for (const int pattern : t1Patterns)
            {
                auto p = make();
                const bool layoutOk = setLayout (*p, channels);

                setAllRandom (*p, 0x71A05EED + pattern);
                disableEveryBand (*p);
                setNormalised (*p, "bypass", 0.0f);         // the engine path, not the hard bypass
                setRaw (*p, "output_gain", 0.0f);

                const bool stimulusOk = countNonDefault (*p) > kMinNonDefault && enabledBands (*p) == 0
                                     && sameBits (rawOf (*p, "output_gain"), 0.0f) && rawOf (*p, "bypass") < 0.5f;

                const auto output = renderPattern (*p, input, pattern);

                tally.check (layoutOk && stimulusOk && sameAudio (output, input),
                             patternName (pattern) + (layoutOk ? "" : " (layout refused)")
                                 + (stimulusOk ? "" : " (stimulus not applied)"));
            }

            row (tally.bad == 0 && tally.total == 8,
                 juce::String ("T1a ") + (channels == 1 ? "mono" : "stereo")
                     + ": no band enabled, the rest random, output 0 dB, noise + denormal tail: out memcmp-equal to in (7 block patterns)",
                 tally.detail());
        }

        // T1b, T1bn
        {
            const auto input = makeNoiseTail (2, 0x71B00002);

            Tally tally, control;
            tally.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");

            for (const int placement : { kLeft, kRight })
            {
                const int touched = placement == kLeft ? 0 : 1;
                const juce::String side = kPlacementNames[placement];

                for (const int pattern : t1Patterns)
                {
                    auto p = makeBands (2, 6, [placement] (int index) { return channelBandSpec (index, placement); });
                    const auto output = renderPattern (*p, input, pattern);

                    const bool untouchedSame = sameChannel (output, input, 1 - touched);
                    const bool touchedDiffers = ! sameChannel (output, input, touched);

                    tally.check (untouchedSame && touchedDiffers,
                                 side + " bands, " + patternName (pattern)
                                     + (untouchedSame ? "" : ": the other channel changed")
                                     + (touchedDiffers ? "" : ": the band's channel did not change"));

                    // T1bn: the same six bands on Stereo.
                    auto q = makeBands (2, 6, [] (int index) { return channelBandSpec (index, kStereo); });
                    const auto both = renderPattern (*q, input, pattern);

                    control.check (! sameChannel (both, input, 1 - touched),
                                   "Stereo bands, " + patternName (pattern) + ": channel "
                                       + juce::String (1 - touched) + " still memcmp-equal");
                }
            }

            row (tally.bad == 0 && tally.total == 15,
                 "T1b 6 Left bands, noise + denormal tail: right channel memcmp-equal, left differs; and the converse (7 block patterns)",
                 tally.detail());

            row (control.bad == 0 && control.total == 14,
                 "T1bn the same bands on Stereo FAIL the untouched-channel comparison (control fires)",
                 control.detail());
        }

        // T1c, T1cn
        {
            constexpr int length = 131072;
            const auto input = makeNoiseAt (2, length, 0x71C00002, kNoise12);

            struct Config { const char* name; int numBands; float gainDb; int placement; };

            const Config configs[] =
            {
                { "24 Mid bells +12 dB",  24, 12.0f, kMid },
                { "1 Mid bell +30 dB",    1,  30.0f, kMid },
                { "24 Side bells +12 dB", 24, 12.0f, kSide },
                { "1 Side bell +30 dB",   1,  30.0f, kSide }
            };

            Tally tally, control;

            for (const auto& c : configs)
            {
                const bool sideBands = c.placement == kSide;

                auto p = makeBands (2, c.numBands, [&c] (int index) { return msBellSpec (index, c.numBands, c.gainDb, c.placement); });
                const auto f = msFigures (input, render (*p, input, 512, false), sideBands);

                tally.check (f.untouchedDb <= -130.0 && f.touchedDb > -40.0,
                             juce::String (c.name) + ": untouched " + fmtG (f.untouchedDb, 4) + " dB, touched "
                                 + fmtG (f.touchedDb, 4) + " dB");

                info (juce::String ("T1c ") + c.name + ": " + (sideBands ? "mid" : "side") + " error "
                          + fmtG (f.untouchedDb, 4) + " dB re the larger peak (limit -130); "
                          + (sideBands ? "side" : "mid") + " change " + fmtG (f.touchedDb, 4)
                          + " dB (must exceed -40); output peak " + fmtG (f.outPeakDb, 4) + " dB re input");

                // T1cn: the same bells on Left.
                auto q = makeBands (2, c.numBands, [&c] (int index) { return msBellSpec (index, c.numBands, c.gainDb, kLeft); });
                const auto g = msFigures (input, render (*q, input, 512, false), sideBands);

                control.check (g.untouchedDb > -130.0,
                               juce::String (c.name) + " moved to Left: " + fmtG (g.untouchedDb, 4) + " dB");

                info (juce::String ("T1cn ") + c.name + " moved to Left: " + (sideBands ? "mid" : "side") + " error "
                          + fmtG (g.untouchedDb, 4) + " dB re the larger peak");
            }

            row (tally.bad == 0 && tally.total == 4,
                 "T1c Mid bells leave the side, Side bells leave the mid: <= -130 dB re max (input peak, output peak); touched > -40 dB",
                 tally.detail());

            row (control.bad == 0 && control.total == 4,
                 "T1cn the same bells on Left FAIL the untouched-component test (control fires)",
                 control.detail());
        }

        // T1g
        {
            const int patterns[] = { 32, 64, 512, 2048, 1, 0 };
            const auto input = makeNoiseAt (2, kNoiseLength + 777, 0x71600002, kNoise12);

            juce::AudioBuffer<float> reference;
            Tally tally;
            double peakDb = 0.0;

            for (const int pattern : patterns)
            {
                auto p = makeBands (2, 24, mixedSpec);
                const auto output = renderPattern (*p, input, pattern);

                if (reference.getNumSamples() == 0)
                {
                    reference = output;

                    const double outPeak = (double) output.getMagnitude (0, output.getNumSamples());
                    peakDb = 20.0 * std::log10 (juce::jmax (outPeak, 1.0e-300) / (double) kNoise12);

                    tally.check (allFinite (output) && outPeak > 0.0 && ! sameAudio (output, input) && activeBands (*p) == 24,
                                 patternName (pattern) + ": the setting does not act on the audio");
                }
                else
                {
                    tally.check (sameAudio (output, reference), patternName (pattern) + " differs from block 32");
                }
            }

            row (tally.bad == 0 && tally.total == 6,
                 "T1g 24-band mixed setting (all types, slopes, placements; +-6 dB): blocks 32 / 64 / 512 / 2048 / 1 / random sequence bit-identical",
                 tally.detail() + "  output peak " + fmtG (peakDb, 4) + " dB re the noise peak");
        }

        // T1m
        {
            const auto input = makeNoiseAt (1, kNoiseLength, 0x71D00001, kNoise12);

            juce::AudioBuffer<float> stereoPlacement;
            Tally tally;

            for (int placement = 0; placement < 5; ++placement)
            {
                auto p = makeBands (1, 1, [placement] (int) { return makeSpec (kBell, 1000.0f, 6.0f, 1.0f, 1, placement); });
                const auto output = render (*p, input, 512, false);

                if (placement == kStereo)
                    stereoPlacement = output;

                const bool placementSet = sameBits (rawOf (*p, bandId (1, "placement")), (float) placement);
                const bool differs = ! sameAudio (output, input);
                const bool sameAsStereo = sameAudio (output, stereoPlacement);

                tally.check (placementSet && differs && sameAsStereo,
                             juce::String (kPlacementNames[placement]) + (placementSet ? "" : " (placement not applied)")
                                 + (differs ? "" : " (output equals input)")
                                 + (sameAsStereo ? "" : " (differs from the Stereo-placement render)"));
            }

            row (tally.bad == 0 && tally.total == 5,
                 "T1m mono layout, one band per placement value: output differs from input, memcmp-equal to the Stereo-placement render",
                 tally.detail());
        }
    }

    //==========================================================================
    // Z. latency and the first sample (G8)
    //==========================================================================
    struct Coverage
    {
        unsigned types = 0, slopes = 0, placements = 0;

        bool complete() const   { return types == 0xFFu && slopes == 0xFFu && placements == 0x1Fu; }
    };

    // 1 ... 24 bands on a seeded random subset: every type, slope and placement;
    // frequency and Q over their full ranges; gains +-12 dB (PLAN-2.2 P12).
    void applyRandomBands (juce::AudioProcessor& p, juce::int64 seed, Coverage& coverage)
    {
        juce::Random random (seed);
        int order[Processor::kNumBands];

        for (int i = 0; i < Processor::kNumBands; ++i)
            order[i] = i + 1;

        for (int i = Processor::kNumBands - 1; i > 0; --i)
            std::swap (order[i], order[random.nextInt (i + 1)]);

        const int numBands = 1 + random.nextInt (Processor::kNumBands);

        for (int k = 0; k < numBands; ++k)
        {
            BandSpec s;
            s.type = random.nextInt (8);
            s.slope = random.nextInt (8);
            s.placement = random.nextInt (5);
            s.freq = 10.0f * std::pow (3000.0f, random.nextFloat());
            s.q = 0.025f * std::pow (1600.0f, random.nextFloat());
            s.gain = -12.0f + 24.0f * random.nextFloat();
            setBand (p, order[k], s);

            coverage.types |= 1u << s.type;
            coverage.slopes |= 1u << s.slope;
            coverage.placements |= 1u << s.placement;
        }
    }

    constexpr int kImpulseIndex = 37;

    struct FirstSample
    {
        int firstNonZero[2] = { -1, -1 };
        double predicted[2] = { 0.0, 0.0 };
    };

    /** One Z1 case: an impulse at index 37 (1.0 on L, 0.5 on R), blocks of 16
        so the impulse is not in the first block. Checks latency 0, exact
        silence before the impulse, and the output at the impulse index
        against the predicted first-sample gain: per-band
        Band::firstSampleGain() scalars with Router::applyFirstSample applied
        in band index order over the active bands, within 1e-6 relative where
        the prediction is at least 1e-30 in magnitude. */
    FirstSample checkFirstSample (Processor& p, const eq::Designer& designer, double rate, int channels, int length,
                                  const juce::String& label, Tally& tally, int& compared)
    {
        const float amplitudes[2] = { 1.0f, 0.5f };

        juce::AudioBuffer<float> input (channels, length);
        input.clear();

        for (int channel = 0; channel < channels; ++channel)
            input.setSample (channel, kImpulseIndex, amplitudes[channel]);

        const auto output = render (p, input, 16, false, rate);

        double l = (double) amplitudes[0];
        double r = (double) amplitudes[1];

        for (int band = 1; band <= Processor::kNumBands; ++band)
        {
            const auto in = inputOf (p, band);

            if (! in.active)
                continue;

            eq::BandDesign design {};
            designer.design (in.settings, design);

            eq::Band engineBand;
            engineBand.setDesign (design);
            eq::Router::applyFirstSample (engineBand.firstSampleGain(), in.placement, channels == 2, l, r);
        }

        FirstSample result;
        result.predicted[0] = l;
        result.predicted[1] = r;

        bool silentBefore = true, gainOk = true;
        juce::String note;

        for (int channel = 0; channel < channels; ++channel)
        {
            const float* y = output.getReadPointer (channel);

            for (int i = 0; i < kImpulseIndex; ++i)
                if (y[i] != 0.0f)
                    silentBefore = false;

            for (int i = 0; i < length; ++i)
                if (y[i] != 0.0f)
                {
                    result.firstNonZero[channel] = i;
                    break;
                }

            const double predicted = result.predicted[channel];

            if (std::abs (predicted) >= 1.0e-30)
            {
                ++compared;
                const double got = (double) y[kImpulseIndex];

                if (! (std::abs (got - predicted) <= 1.0e-6 * std::abs (predicted)))
                {
                    gainOk = false;
                    note += " ch " + juce::String (channel) + " predicted " + fmtJ (predicted) + " got " + fmtJ (got);
                }
            }
        }

        const int latency = p.getLatencySamples();

        tally.check (latency == 0 && silentBefore && gainOk,
                     label + (latency == 0 ? juce::String() : " latency " + juce::String (latency))
                         + (silentBefore ? "" : " output before the impulse") + note);

        return result;
    }

    void latencyRows()
    {
        section ("Z. latency and first sample (Phase 2.2: G8)");

        Tally tally;
        Coverage coverage;
        int compared = 0;

        for (const double rate : { 44100.0, 96000.0, 192000.0 })
        {
            const auto designer = designerFor (rate);

            for (int config = 0; config < 20; ++config)
                for (const int channels : { 1, 2 })
                {
                    auto p = makeBands (channels, 0, mixedSpec);
                    applyRandomBands (*p, 0x21000000 + config, coverage);

                    checkFirstSample (*p, *designer, rate, channels, 256,
                                      "config " + juce::String (config) + " " + fmtG (rate / 1000.0, 4) + " kHz "
                                          + (channels == 1 ? "mono" : "stereo") + " (" + juce::String (activeBands (*p)) + " bands)",
                                      tally, compared);
                }
        }

        // The three forced cases of RESEARCH-2.2 Q3: the first-sample gain is
        // below the smallest float (2.0e-47, 1.2e-57) or a float denormal
        // (1.2e-41), so the float output starts late. Latency and the silence
        // before the impulse are still checked; the index is INFO.
        struct Forced { const char* name; float freq; double rate; };

        const Forced forced[] =
        {
            { "High Cut 10 Hz 96 dB/oct Q 0.025, 44.1 kHz", 10.0f,  44100.0 },
            { "High Cut 10 Hz 96 dB/oct Q 0.025, 192 kHz",  10.0f,  192000.0 },
            { "High Cut 100 Hz 96 dB/oct Q 0.025, 192 kHz", 100.0f, 192000.0 }
        };

        juce::StringArray forcedLines;

        for (const auto& f : forced)
        {
            const auto designer = designerFor (f.rate);
            juce::String line = juce::String ("Z1 forced, ") + f.name + " (impulse at index "
                              + juce::String (kImpulseIndex) + "): first non-zero index";

            for (const int channels : { 1, 2 })
            {
                auto p = makeBands (channels, 1, [&f] (int) { return makeSpec (kHighCut, f.freq, 0.0f, 0.025f, 7, kStereo); });

                const auto first = checkFirstSample (*p, *designer, f.rate, channels, kImpulseIndex + 1024,
                                                     juce::String (f.name) + (channels == 1 ? " mono" : " stereo"),
                                                     tally, compared);

                if (channels == 1)
                    line += " mono " + juce::String (first.firstNonZero[0]) + " (predicted first-sample gain "
                          + fmtG (first.predicted[0]) + ");";
                else
                    line += " stereo L " + juce::String (first.firstNonZero[0]) + ", R " + juce::String (first.firstNonZero[1]);
            }

            forcedLines.add (line);
        }

        row (tally.bad == 0 && tally.total == 126 && compared > 0 && coverage.complete(),
             "Z1 20 random configurations x 44.1 / 96 / 192 kHz x mono / stereo + 3 forced High Cuts: latency 0, silent before index 37, first-sample gain within 1e-6",
             tally.detail() + "  " + juce::String (compared) + " gains compared (predicted >= 1e-30)"
                 + (coverage.complete() ? "" : "  (the draw missed a type, slope or placement)"));

        for (const auto& line : forcedLines)
            info (line + "  (-1 = no non-zero sample in the render)");
    }

    //==========================================================================
    // A. allocation (G6, static parameters)
    //==========================================================================
   #if defined (__APPLE__)
    // volatile: the compiler knows the allocator as a builtin that cannot touch
    // globals and would otherwise fold "armed = true; ...; armed = false".
    volatile bool allocArmed = false;
    volatile int allocCount = 0;
    volatile int freeCount = 0;
    pthread_t allocThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (! allocArmed || pthread_equal (pthread_self(), allocThread) == 0)
            return;

        if ((type & 2u) != 0)       // MALLOC_LOG_TYPE_ALLOCATE
            allocCount = allocCount + 1;

        if ((type & 4u) != 0)       // MALLOC_LOG_TYPE_DEALLOCATE
            freeCount = freeCount + 1;
    }

    // 24 bands at 96 dB/oct: the five slope types, every placement, gains +-3 dB.
    BandSpec allocSpec (int index)
    {
        static const int types[5] = { kLowShelf, kHighShelf, kLowCut, kHighCut, kTilt };

        BandSpec s;
        s.type = types[index % 5];
        s.slope = 7;
        s.placement = (index + index / 5) % 5;
        s.freq = 60.0f * std::pow (200.0f, (float) ((index * 7) % 24) / 23.0f);
        s.gain = (index % 2 == 0) ? 3.0f : -3.0f;
        s.q = 1.0f;

        if (s.type == kLowCut)
            s.freq = 30.0f + 10.0f * (float) (index / 5);

        if (s.type == kHighCut)
            s.freq = 12000.0f + 1500.0f * (float) (index / 5);

        return s;
    }
   #endif

    struct AllocResult
    {
        bool available = false;
        int allocs = 0, frees = 0;
        int probeAllocs = 0, probeFrees = 0;
        int blocks = 0;
        bool audioOk = false;
    };

    /** The logger counts on this thread (the one that calls processBlock) and
        only while armed; it is armed around processBlock alone. Every
        parameter write happens between windows: a write notifies listeners
        and may allocate legitimately. A separate window with a deliberate
        allocation proves the hook live. `plant` puts one deliberate
        allocation inside a processBlock window (A1n). */
    AllocResult runAllocWindow (bool plant)
    {
        AllocResult result;

       #if defined (__APPLE__)
        constexpr int blockSize = 512;

        auto p = makeBands (2, 24, allocSpec);
        const auto noise = makeNoiseAt (2, blockSize, 0x0A110C8, kNoise12);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        p->prepareToPlay (kRenderRate, blockSize);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        // Liveness: the hook must see a deliberate allocation and its release.
        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.probeAllocs = allocCount;
        result.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;

        const auto armedBlock = [&]()
        {
            for (int channel = 0; channel < 2; ++channel)
                block.copyFrom (channel, 0, noise, channel, 0, blockSize);      // outside the window

            allocArmed = true;
            p->processBlock (block, midi);

            if (plantNext)
            {
                void* volatile planted = std::malloc (64);
                std::free (planted);
            }

            allocArmed = false;
            plantNext = false;
            ++result.blocks;
        };

        // The write is outside the window; the block that acts on it is inside.
        const auto change = [&] (const juce::String& id, float raw)
        {
            setRaw (*p, id, raw);
            armedBlock();
        };

        armedBlock();                                   // the first block after prepareToPlay (24 designs)
        const bool firstActs = ! sameAudio (block, noise);

        for (int i = 0; i < 200; ++i)                   // settled
            armedBlock();

        change (bandId (1, "gain"), -2.0f);             // continuous
        change (bandId (1, "freq"), 180.0f);
        change (bandId (1, "q"), 2.5f);
        change ("gain_scale", 80.0f);                   // continuous, every band redesigned
        change (bandId (2, "type"), (float) kBell);     // structural: type
        change (bandId (6, "slope"), 3.0f);             // structural: slope
        change (bandId (7, "placement"), (float) kStereo);  // structural: placement
        change (bandId (3, "enabled"), 0.0f);           // enable off / on
        change (bandId (3, "enabled"), 1.0f);
        change (bandId (4, "bypass"), 1.0f);            // band bypass on / off
        change (bandId (4, "bypass"), 0.0f);
        change ("bypass", 1.0f);                        // global bypass on / off
        const bool bypassPasses = sameAudio (block, noise);
        change ("bypass", 0.0f);
        const bool resumed = ! sameAudio (block, noise);

        malloc_logger = previous;
        p->releaseResources();

        result.available = true;
        result.allocs = allocCount;
        result.frees = freeCount;
        result.audioOk = firstActs && bypassPasses && resumed;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void allocationRows()
    {
        section ("A. allocation (Phase 2.2: G6, static parameters)");

        const AllocResult clean = runAllocWindow (false);

        if (! clean.available)
        {
            info ("A1 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A1n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = clean.probeAllocs >= 1 && clean.probeFrees >= 1;

        row (hookLive && clean.audioOk && clean.allocs == 0 && clean.frees == 0,
             "A1 processBlock, 24 bands x 96 dB/oct: first block, 200 settled, the block after each kind of change: 0 allocations, 0 frees",
             juce::String (clean.allocs) + " allocations, " + juce::String (clean.frees) + " frees in "
                 + juce::String (clean.blocks) + " windows (malloc_logger, this thread); hook "
                 + (hookLive ? "live" : "DEAD") + " (probe: " + juce::String (clean.probeAllocs) + " / "
                 + juce::String (clean.probeFrees) + ")" + (clean.audioOk ? "" : "; the engine did not act as expected"));

        const AllocResult planted = runAllocWindow (true);

        row (planted.allocs >= 1 && planted.frees >= 1,
             "A1n a planted allocation inside the processBlock window is counted (control fires)",
             juce::String (planted.allocs) + " allocations, " + juce::String (planted.frees) + " frees");
    }

    //==========================================================================
    // K, H, U, V. host block size and the 2.2 state rules
    //==========================================================================
    // Gain in dB of a steady tone over [start, start + length): the output's
    // single-bin projection over the input's. length must hold whole cycles.
    double toneGainDb (const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output,
                       int start, int length, double hz, double rate)
    {
        double inRe = 0.0, inIm = 0.0, outRe = 0.0, outIm = 0.0;

        for (int i = 0; i < length; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) (start + i) / rate;
            const double c = std::cos (phase), s = std::sin (phase);
            const double x = (double) input.getSample (0, start + i);
            const double y = (double) output.getSample (0, start + i);

            inRe += x * c;
            inIm -= x * s;
            outRe += y * c;
            outIm -= y * s;
        }

        return eq::powerToDb ((outRe * outRe + outIm * outIm) / (inRe * inRe + inIm * inIm));
    }

    struct EditResult
    {
        bool identical = false;     // the render with the edit equals the render without
        bool applied = false;       // the raw value did change
        bool audible = false;       // the band acts on the audio at all
    };

    // One band; the same input rendered without and with one parameter edit
    // before block 10 (blocks of 512).
    EditResult renderWithEdit (int channels, const BandSpec& spec, const char* suffix, float newRaw)
    {
        const auto input = makeNoiseAt (channels, 12000, 0x0E3A0000 + channels, kNoise12);
        const auto id = bandId (1, suffix);

        auto a = makeBands (channels, 1, [&spec] (int) { return spec; });
        auto b = makeBands (channels, 1, [&spec] (int) { return spec; });

        const auto plain = render (*a, input, 512, false);
        const auto edited = render (*b, input, 512, false, kRenderRate, [&] (BlockInfo& current)
        {
            if (current.index == 10)
                setRaw (*b, id, newRaw);
        });

        EditResult result;
        result.identical = sameAudio (plain, edited);
        result.applied = ! sameBits (rawOf (*a, id), rawOf (*b, id));
        result.audible = ! sameAudio (plain, input);
        return result;
    }

    void blockAndStateRows()
    {
        section ("K / H / U / V. host block size and the 2.2 state rules");

        // K1
        {
            const auto input = makeNoiseAt (2, 17000, 0x4B100002, kNoise12);

            auto a = makeBands (2, 24, mixedSpec);
            auto b = makeBands (2, 24, mixedSpec);

            const auto oversized = render (*a, input, 1024, false, kRenderRate, {}, {}, 256);
            const auto prepared = render (*b, input, 256, false);
            const bool acts = ! sameAudio (prepared, input);

            row (acts && sameAudio (oversized, prepared),
                 "K1 prepare 256, feed 1024-sample blocks, 24-band setting: bit-identical to the same input in 256-sample blocks",
                 acts ? juce::String() : juce::String ("the setting does not act on the audio"));
        }

        // H1
        {
            constexpr int blockSize = 512, firstBypassed = 10, firstResumed = 20;
            const auto input = makeNoiseTail (2, 0x48100002);       // 36 blocks of 512
            const int total = input.getNumSamples();
            const int bypassAt = firstBypassed * blockSize;
            const int resumeAt = firstResumed * blockSize;

            auto steady = makeBands (2, 24, mixedSpec);
            const auto steadyOut = render (*steady, input, blockSize, false);

            // (i) the buffer is untouched
            Tally untouched;
            untouched.check (hasDenormalTail (input), "stimulus: the tail is not denormal non-zero, or this thread flushes to zero");
            untouched.check (! sameAudio (steadyOut, input) && activeBands (*steady) == 24, "control: with bypass off the 24 bands do not act");

            {
                auto p = makeBands (2, 24, mixedSpec);
                setNormalised (*p, "bypass", 1.0f);
                untouched.check (sameAudio (render (*p, input, blockSize, false), input), "bypass on, processBlock");

                auto q = makeBands (2, 24, mixedSpec);
                untouched.check (sameAudio (render (*q, input, blockSize, true), input), "processBlockBypassed");
            }

            row (untouched.bad == 0 && untouched.total == 4,
                 "H1 (i) 24 bands enabled, noise + denormal tail: bypass on via processBlock, and processBlockBypassed: out memcmp-equal to in",
                 untouched.detail());

            // (ii) the state was cleared
            auto fresh = makeBands (2, 24, mixedSpec);
            const auto freshOut = render (*fresh, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

            Tally resumed;
            resumed.check (! sameFrom (steadyOut, resumeAt, freshOut, 0, total - resumeAt),
                           "control: an uninterrupted render equals a fresh one (state does not matter)");

            for (const bool viaParameter : { true, false })
            {
                auto p = makeBands (2, 24, mixedSpec);

                const auto output = render (*p, input, blockSize, false, kRenderRate, [&] (BlockInfo& current)
                {
                    if (viaParameter)
                    {
                        if (current.index == firstBypassed)
                            setNormalised (*p, "bypass", 1.0f);

                        if (current.index == firstResumed)
                            setNormalised (*p, "bypass", 0.0f);
                    }
                    else
                    {
                        current.viaBypassed = current.index >= firstBypassed && current.index < firstResumed;
                    }
                });

                const juce::String route = viaParameter ? "bypass parameter" : "processBlockBypassed";

                resumed.check (sameFrom (output, 0, steadyOut, 0, bypassAt), route + ": output before the bypass differs from the steady render");
                resumed.check (sameFrom (output, bypassAt, input, bypassAt, resumeAt - bypassAt), route + ": bypassed blocks differ from the input");
                resumed.check (sameFrom (output, resumeAt, freshOut, 0, total - resumeAt), route + ": output after the bypass differs from a fresh instance");
            }

            row (resumed.bad == 0 && resumed.total == 7,
                 "H1 (ii) after bypass off, the output from that sample on is memcmp-equal to a fresh instance fed the remaining input",
                 resumed.detail());
        }

        // U1
        {
            constexpr int blockSize = 512, editBlock = 47, window = 4800;   // 100 cycles of 1 kHz at 48 kHz
            const double rate = kRenderRate;
            const int editAt = editBlock * blockSize;
            const int oneSecond = (int) std::lround (rate);
            const int total = editAt + oneSecond + window;

            juce::AudioBuffer<float> input (1, total);

            for (int i = 0; i < total; ++i)
                input.setSample (0, i, kNoise12 * (float) std::sin (2.0 * eq::pi * 1000.0 * (double) i / rate));

            const auto designer = designerFor (rate);
            const std::vector<double> at { 1000.0 };

            auto p = makeBands (1, 1, [] (int) { return makeSpec (kBell, 1000.0f, 6.0f, 1.0f, 1, kStereo); });
            const double oldRef = chainRefDb (*p, *designer, rate, at)[0];

            const auto output = render (*p, input, blockSize, false, rate, [&] (BlockInfo& current)
            {
                if (current.index == editBlock)
                    setRaw (*p, bandId (1, "gain"), -9.0f);
            });

            const double newRef = chainRefDb (*p, *designer, rate, at)[0];
            const double before = toneGainDb (input, output, editAt - window, window, 1000.0, rate);
            const double after = toneGainDb (input, output, editAt + oneSecond, window, 1000.0, rate);

            row (std::abs (after - newRef) <= 0.01 && std::abs (before - oldRef) <= 0.01 && std::abs (newRef - oldRef) > 1.0,
                 "U1 gain edit between blocks on a 1 kHz bell, 1 kHz sine: steady-state gain 1 s later equals the new closed form within 0.01 dB",
                 "after " + fmtG (after, 6) + " dB (closed form " + fmtG (newRef, 6) + "); before " + fmtG (before, 6)
                     + " dB (closed form " + fmtG (oldRef, 6) + ")");
        }

        // U2
        {
            constexpr int blockSize = 512, offBlock = 10;
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x0E2B0002, kNoise12);
            const int total = input.getNumSamples();
            const int offAt = offBlock * blockSize;
            const int resumeAt = offAt + blockSize;

            const auto spec = makeSpec (kLowShelf, 300.0f, 9.0f, 1.0f, 3, kStereo);
            const auto one = [&spec] (int) { return spec; };

            auto p = makeBands (2, 1, one);

            const auto output = render (*p, input, blockSize, false, kRenderRate, [&] (BlockInfo& current)
            {
                if (current.index == offBlock)
                    setRaw (*p, bandId (1, "enabled"), 0.0f);

                if (current.index == offBlock + 1)
                    setRaw (*p, bandId (1, "enabled"), 1.0f);
            });

            auto steady = makeBands (2, 1, one);
            const auto steadyOut = render (*steady, input, blockSize, false);

            auto fresh = makeBands (2, 1, one);
            const auto freshOut = render (*fresh, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

            const bool before = sameFrom (output, 0, steadyOut, 0, offAt);
            const bool during = sameFrom (output, offAt, input, offAt, blockSize);
            const bool after = sameFrom (output, resumeAt, freshOut, 0, total - resumeAt);
            const bool stateMatters = ! sameFrom (steadyOut, resumeAt, freshOut, 0, total - resumeAt);

            row (before && during && after && stateMatters,
                 "U2 band disabled for one block, then enabled: output from the re-enable on memcmp-equal to a fresh instance fed the remaining input",
                 juce::String (before ? "" : "output before the edit differs from the steady render; ")
                     + (during ? "" : "the disabled block is not the input; ")
                     + (after ? "" : "output after the re-enable differs from a fresh instance; ")
                     + (stateMatters ? "" : "control: the steady render equals the fresh one"));
        }

        // U3
        {
            struct Edit { const char* name; int channels; BandSpec spec; const char* suffix; float newRaw; };

            const Edit inert[] =
            {
                { "slope on a Bell",           2, makeSpec (kBell,     1000.0f, 6.0f, 1.0f, 1, kStereo), "slope",     5.0f },
                { "slope on a Notch",          2, makeSpec (kNotch,    1000.0f, 0.0f, 4.0f, 1, kStereo), "slope",     6.0f },
                { "slope on a Band Pass",      2, makeSpec (kBandPass, 500.0f,  0.0f, 2.0f, 1, kStereo), "slope",     3.0f },
                { "Q on a 6 dB/oct Low Cut",   2, makeSpec (kLowCut,   200.0f,  0.0f, 1.0f, 0, kStereo), "q",         4.0f },
                { "placement on mono",         1, makeSpec (kBell,     1000.0f, 6.0f, 1.0f, 1, kStereo), "placement", (float) kMid }
            };

            Tally tally;

            for (const auto& e : inert)
            {
                const auto r = renderWithEdit (e.channels, e.spec, e.suffix, e.newRaw);
                tally.check (r.identical && r.applied && r.audible,
                             juce::String (e.name) + (r.identical ? "" : " changed the output")
                                 + (r.applied ? "" : " (edit not applied)") + (r.audible ? "" : " (band does not act)"));
            }

            // Control: the same edit where it is not inert.
            const auto live = renderWithEdit (2, makeSpec (kLowCut, 200.0f, 0.0f, 1.0f, 1, kStereo), "slope", 5.0f);
            tally.check (! live.identical && live.applied && live.audible, "control: a slope edit on a Low Cut left the output unchanged");

            row (tally.bad == 0 && tally.total == 6,
                 "U3 inert edits mid-render (slope on Bell / Notch / Band Pass, Q at 6 dB/oct, placement on mono): bit-identical; a Low Cut slope edit changes the output",
                 tally.detail());
        }

        // V1
        {
            Tally tally;

            auto fresh = make();

            for (int band = 0; band < Processor::kNumBands; ++band)
                tally.check (fresh->getBandDynOffsetDb (band) == 0.0, "fresh instance, band " + juce::String (band + 1));

            auto p = makeBands (2, 24, mixedSpec);
            const auto input = makeNoiseAt (2, (int) std::lround (kRenderRate), 0x56100002, kNoise12);
            const auto output = render (*p, input, 512, false);

            for (int band = 0; band < Processor::kNumBands; ++band)
                tally.check (p->getBandDynOffsetDb (band) == 0.0, "after a 1 s render, band " + juce::String (band + 1));

            const bool acts = ! sameAudio (output, input);

            row (tally.bad == 0 && tally.total == 48 && acts,
                 "V1 getBandDynOffsetDb reads exactly 0.0 for all 24 bands, fresh and after a 1 s render (arithmetic: designer-harness EN-dyn)",
                 tally.detail() + (acts ? "" : "  (the render did not act on the audio)"));
        }
    }

    //==========================================================================
    // X. CPU, settled (INFO; PLAN-2.2 P15)
    //==========================================================================
    void cpuInfo()
    {
        section ("X. CPU, settled (INFO, not gated)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9000002, kNoise12);

        struct Config { const char* name; int numBands; int type; int slope; float gain; const char* beside; };

        // Settings as research-2.2 engine_bench modeCpu.
        const Config configs[] =
        {
            { "0 bands",                0,  kBell,      1, 0.0f, "no bench figure" },
            { "24 cuts x 96 dB/oct",    24, kHighCut,   7, 0.0f, "bench 2.4 sample-major; budget 10" },
            { "24 shelves x 96 dB/oct", 24, kHighShelf, 7, 6.0f, "bench 4.7 sample-major; budget 14" },
            { "24 bells",               24, kBell,      1, 6.0f, "bench 0.70 sample-major" },
            { "1 band",                 1,  kBell,      1, 6.0f, "no bench figure" }
        };

        for (const auto& c : configs)
        {
            auto p = makeBands (2, c.numBands, [&c] (int index)
            {
                const float freq = c.type == kHighCut ? 20000.0f - 300.0f * (float) index
                                 : c.numBands > 1     ? 30.0f * std::pow (500.0f, (float) index / 23.0f)
                                                      : 1000.0f;
                return makeSpec (c.type, freq, c.gain, 1.0f, c.slope, kStereo);
            });

            juce::AudioBuffer<float> block (2, blockSize);
            juce::MidiBuffer midi;
            double seconds = 0.0;

            p->prepareToPlay (rate, blockSize);

            for (int k = 0; k < numBlocks; ++k)
            {
                for (int channel = 0; channel < 2; ++channel)
                    block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                const auto t0 = std::chrono::steady_clock::now();
                p->processBlock (block, midi);
                const auto t1 = std::chrono::steady_clock::now();

                seconds += std::chrono::duration<double> (t1 - t0).count();
            }

            p->releaseResources();

            info (juce::String ("X-cpu 48 kHz stereo, 512-sample blocks, 20 s, ") + c.name + ": "
                      + fmtG (100.0 * seconds / audioSeconds, 3) + " % of one core (" + c.beside + ")"
                      + (allFinite (block) ? "" : "  OUTPUT NOT FINITE"));
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

    // Stage 2, Phase 2.2 (PLAN-2.2 T7)
    responseRows();
    nyquistRows();
    transparencyRows();
    latencyRows();
    allocationRows();
    blockAndStateRows();
    cpuInfo();

    std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
    std::fflush (stdout);
    return juce::jmin (failures, 255);
}
