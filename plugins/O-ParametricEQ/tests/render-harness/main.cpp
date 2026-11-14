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

    Stage 2, Phase 2.3 (PLAN-2.3 T7): continuous modulation through the
    plugin's float output.

      O-ParametricEQ-render-harness --long              S5 at 600 s per rate (default 60 s)

    The G3 verdict is a ratio to a same-motion reference that lives in this
    file (refRender: the control trajectory re-stated here, then a design on
    every sample and the settled recurrences; PLAN-2.3 P11). The no-smoothing
    control is hardRender (the 2.2 rule). Both are fed the plugin's own float
    input, widened, and compared in double. Mid-render parameter edits of
    these rows are stores to the raw parameter values the processor reads
    (parameters.getRawParameterValue), which a worker thread may do; rows
    with many renders run them on worker threads, one plugin instance per
    job, constructed on the main thread, and print in order afterwards.
    The IDs B1, S5 and S6 below are PLAN-2.3's; they are not the Stage 1 rows
    of the same name above (bus layouts, state codec).

      M1   G3: 13 motions x 50 Hz / 1 kHz x blocks 32 / 512, local kappa <= 4.0x refRender
      M1h  hardRender at block 512 reads >= 10x refRender (24 readings; 2 void, printed)
      M1k  motion 8 at 96 kHz (K = 32) and 192 kHz (K = 64) <= 4.0x
      M2   gain_scale 100 -> 25 on three Stereo bells: steady-state gains; kappa of the move
      M2c  the same edit on a Low Cut + Notch: bit-identical to no edit
      M3   bNN_bypass on, then off, on the only active band
      M4   re-prepare at another rate == a fresh instance at that rate
      M5   Mid / Side bell: gain of each component at the bell centre
      M6   a gain edit on a ringing bell keeps the state (<= 10x the ring's own curvature)
      M6n  a type change at the same sample does not (>= 100x)
      B1   events on multiples of 2048 (f, gain, Q, gain_scale): blocks 32 / 64 / 512 / 2048
           bit-identical at four rates; differs from the render without events
      TK   the tick clock runs through bypass, no-active-band and processBlockBypassed blocks
      S5   G5: seeded random stress at 8 rates: stack guard, finite, peak printed, bands off
      S6   a NaN / +inf input sample does not stick
      A2   G6 under stress: no allocation inside processBlock (malloc_logger, this thread)
      A2n  a planted allocation inside the window is counted
      N7   G7 float: noise floor; silence costs no more than signal
      X-cpu2  sweeping CPU readings, INFO

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
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <thread>
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

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.3: continuous modulation through the plugin (PLAN-2.3 T7)
    //==========================================================================
    //==========================================================================

    // --long: S5 runs the contract's 600 s per rate instead of 60 s (PLAN-2.3 P13).
    bool longRunRequested = false;

    //==========================================================================
    // Raw parameter values, as the processor reads them
    //==========================================================================
    struct BandHandles
    {
        std::atomic<float>* enabled = nullptr;
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* slope = nullptr;
        std::atomic<float>* placement = nullptr;
    };

    /** The raw values of one instance, looked up once on the main thread. A
        mid-render edit of the 2.3 rows is a store to one of these: it is what
        the processor reads at the next block start, it allocates nothing and
        a worker thread may do it. Values stored are legal ones (in range;
        choices and toggles as whole numbers). */
    struct Handles
    {
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* gainScale = nullptr;
        std::array<BandHandles, (size_t) Processor::kNumBands> band;
    };

    Handles handlesOf (Processor& p)
    {
        Handles h;
        bool complete = true;

        const auto fetch = [&p, &complete] (const juce::String& id)
        {
            std::atomic<float>* const value = p.parameters.getRawParameterValue (id);
            complete = complete && value != nullptr;
            return value;
        };

        h.bypass = fetch ("bypass");
        h.gainScale = fetch ("gain_scale");

        for (int b = 0; b < Processor::kNumBands; ++b)
        {
            BandHandles& d = h.band[(size_t) b];
            d.enabled   = fetch (bandId (b + 1, "enabled"));
            d.bypass    = fetch (bandId (b + 1, "bypass"));
            d.type      = fetch (bandId (b + 1, "type"));
            d.freq      = fetch (bandId (b + 1, "freq"));
            d.gain      = fetch (bandId (b + 1, "gain"));
            d.q         = fetch (bandId (b + 1, "q"));
            d.slope     = fetch (bandId (b + 1, "slope"));
            d.placement = fetch (bandId (b + 1, "placement"));
        }

        if (! complete)
        {
            std::printf ("FATAL: a raw parameter value is missing\n");
            std::exit (255);
        }

        return h;
    }

    eq::RawBand rawOfHandles (const BandHandles& h)
    {
        eq::RawBand raw;
        raw.enabled   = h.enabled->load();
        raw.bypass    = h.bypass->load();
        raw.type      = h.type->load();
        raw.freq      = h.freq->load();
        raw.gain      = h.gain->load();
        raw.q         = h.q->load();
        raw.slope     = h.slope->load();
        raw.placement = h.placement->load();
        return raw;
    }

    //==========================================================================
    // Workers
    //==========================================================================
    /** Runs job (0) ... job (count - 1) on up to 12 threads, this one
        included. A job owns everything it writes: its own plugin instance
        (constructed beforehand, on the main thread) and its own result slot.
        Jobs call prepareToPlay / processBlock, store raw parameter values and
        run harness code. They print nothing and touch no tally. */
    template <typename JobFn>
    void parallelFor (int count, JobFn&& job)
    {
        if (count <= 0)
            return;

        const int hardware = (int) std::thread::hardware_concurrency();
        const int workers = juce::jlimit (1, 12, juce::jmin (count, hardware > 0 ? hardware : 1));
        std::atomic<int> next { 0 };

        const auto work = [&next, &job, count]()
        {
            for (;;)
            {
                const int mine = next.fetch_add (1);

                if (mine >= count)
                    return;

                job (mine);
            }
        };

        std::vector<std::thread> pool;

        for (int w = 1; w < workers; ++w)
            pool.emplace_back (work);

        work();

        for (auto& thread : pool)
            thread.join();
    }

    //==========================================================================
    // Rendering for the 2.3 rows
    //==========================================================================
    /** render() with the block length from sizeAt (index, position) and a
        hook before each block (raw parameter stores; the entry point). The
        instance is prepared here at `prepared`, which is also the longest
        block. No std::function, so a worker thread may run it. It does not
        release the instance: M4 prepares again on top of a render. */
    template <typename SizeFn, typename HookFn>
    juce::AudioBuffer<float> renderWith (juce::AudioProcessor& p, const juce::AudioBuffer<float>& input, double rate,
                                         int prepared, SizeFn&& sizeAt, HookFn&& hook)
    {
        const int numChannels = input.getNumChannels();
        const int length = input.getNumSamples();

        juce::AudioBuffer<float> output (numChannels, length);
        output.clear();
        juce::AudioBuffer<float> block (numChannels, prepared);
        juce::MidiBuffer midi;

        p.prepareToPlay (rate, prepared);

        int index = 0;

        for (int position = 0; position < length; ++index)
        {
            const int n = juce::jlimit (1, juce::jmin (prepared, length - position), (int) sizeAt (index, position));

            BlockInfo current;
            current.index = index;
            current.position = position;
            current.viaBypassed = false;

            hook (current);

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

        return output;
    }

    std::vector<float> channelOf (const juce::AudioBuffer<float>& buffer, int channel)
    {
        const float* data = buffer.getReadPointer (channel);
        return std::vector<float> (data, data + buffer.getNumSamples());
    }

    std::vector<double> widened (const juce::AudioBuffer<float>& buffer, int channel)
    {
        const float* data = buffer.getReadPointer (channel);
        std::vector<double> out ((size_t) buffer.getNumSamples());

        for (size_t i = 0; i < out.size(); ++i)
            out[i] = (double) data[i];

        return out;
    }

    // First sample index at which two renders differ on any channel, by bit
    // pattern; -1 when they are the same. A shape mismatch reads 0.
    int firstDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return 0;

        for (int i = 0; i < a.getNumSamples(); ++i)
            for (int channel = 0; channel < a.getNumChannels(); ++channel)
                if (! sameBits (a.getReadPointer (channel)[i], b.getReadPointer (channel)[i]))
                    return i;

        return -1;
    }

    int firstDifference (const std::vector<double>& a, const std::vector<double>& b)
    {
        if (a.size() != b.size())
            return 0;

        for (size_t i = 0; i < a.size(); ++i)
            if (std::memcmp (&a[i], &b[i], sizeof (double)) != 0)
                return (int) i;

        return -1;
    }

    // Power of one channel at hz over [start, start + length): single bin.
    double binPower (const juce::AudioBuffer<float>& buffer, int channel, int start, int length, double hz, double rate)
    {
        const float* data = buffer.getReadPointer (channel);
        double re = 0.0, im = 0.0;

        for (int i = 0; i < length; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) (start + i) / rate;
            const double v = (double) data[start + i];
            re += v * std::cos (phase);
            im -= v * std::sin (phase);
        }

        return re * re + im * im;
    }

    // The same for the component 0.5 (L + sign R) of a stereo buffer
    // (sign +1: mid; -1: side).
    double componentPower (const juce::AudioBuffer<float>& buffer, double sign, int start, int length, double hz, double rate)
    {
        const float* left = buffer.getReadPointer (0);
        const float* right = buffer.getReadPointer (1);
        double re = 0.0, im = 0.0;

        for (int i = 0; i < length; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) (start + i) / rate;
            const double v = 0.5 * ((double) left[start + i] + sign * (double) right[start + i]);
            re += v * std::cos (phase);
            im -= v * std::sin (phase);
        }

        return re * re + im * im;
    }

    double ratioOf (double value, double reference)
    {
        return (reference > 0.0 && std::isfinite (value) && std::isfinite (reference))
                   ? value / reference
                   : std::numeric_limits<double>::infinity();
    }

    //==========================================================================
    // Local kappa (gate G3; ported from research-2.3 ramp_bench localKappa):
    // the largest, over 5 ms frames, of max |second difference| / frame RMS.
    // Frames below -80 dBFS RMS are skipped.
    //==========================================================================
    double localKappa (const std::vector<double>& y, double rate, size_t from, size_t to)
    {
        const auto frame = (size_t) std::lround (0.005 * rate);

        if (y.size() < 3 || frame == 0)
            return 0.0;

        const size_t end = juce::jmin (to, y.size() - 1);
        double worst = 0.0;

        for (size_t a = juce::jmax (from, (size_t) 1); a + frame < end; a += frame)
        {
            double energy = 0.0, d2 = 0.0;

            for (size_t i = a; i < a + frame; ++i)
            {
                energy += y[i] * y[i];
                d2 = juce::jmax (d2, std::abs (y[i + 1] - 2.0 * y[i] + y[i - 1]));
            }

            const double rms = std::sqrt (energy / (double) frame);

            if (rms < 1.0e-4)
                continue;

            worst = juce::jmax (worst, d2 / rms);
        }

        return worst;
    }

    //==========================================================================
    // The control grid, re-stated in harness code (PLAN-2.3 P1, P11). The
    // reference below takes these two numbers as arguments; M1 checks that
    // they are the engine's at all eight rates.
    //==========================================================================
    int harnessGrid (double rate)
    {
        return rate <= 48000.0 ? 16 : (rate <= 96000.0 ? 32 : 64);
    }

    int harnessRampTicks (double rate)
    {
        return juce::jmax (1, (int) std::lround (0.020 * rate / (double) harnessGrid (rate)));
    }

    //==========================================================================
    // References (PLAN-2.3 P11). They share no code with the engine's ramp.
    //==========================================================================
    /** What a host block start reads, block by block: the settings of each
        band in series (band 0 first), on one lane. blockStart[0] is 0. */
    struct RefScript
    {
        std::vector<int> blockStart;
        std::vector<std::vector<eq::BandSettings>> settings;    // [block][band]
    };

    bool sameSettings (const eq::BandSettings& a, const eq::BandSettings& b)
    {
        return a.type == b.type && a.slopeIndex == b.slopeIndex
            && eq::sameValue (a.freqHz, b.freqHz)
            && eq::sameValue (a.gainDbEff, b.gainDbEff)
            && eq::sameValue (a.q, b.q);
    }

    double clampRange (double v, double lo, double hi)
    {
        return (v >= lo) ? ((v <= hi) ? v : hi) : lo;
    }

    /** The same-motion reference: the specified control trajectory, then a
        design on EVERY sample and the settled recurrences.

        The trajectory, as PLAN-2.3 P2, P3 and P8 state it:
          - three values per band: ln f, effective gain in dB, ln Q;
          - settings seen at a block start that differ from the ones the ramp
            is heading for wait for the next grid tick (a tick is a sample
            where the clock is a multiple of `grid`; a block that starts on a
            tick is served by that tick);
          - at that tick the ramp restarts from its current value: per-tick
            steps (goal - current) / rampTicks, rampTicks ticks to go. Each
            tick while running moves the value by one step; the last one
            assigns the goal itself;
          - a tick's value is the END of the grid interval that starts there
            (the one-interval lag): inside the interval the control values
            run linearly from the previous tick's value, sample j of the
            interval (0 ... grid - 1) at (j + 1) / grid of the way;
          - while moving, the design settings are exp (ln f), the gain and
            exp (ln Q), each clamped to its range; the sample that ends the
            last interval, and every sample after it, is designed from the
            goal's own settings.

        The clock counts from clockAtStart at input[0]. One lane, bands in
        series, double throughout. */
    std::vector<double> refRender (const std::vector<float>& input, double rate, int grid, int rampTicks,
                                   const RefScript& script, std::uint64_t clockAtStart = 0)
    {
        struct Track
        {
            eq::BandSettings goal {};
            eq::BandSettings waiting {};
            bool hasWaiting = false;

            double lnF = 0.0, gainDb = 0.0, lnQ = 0.0;                  // at the latest tick
            double prevLnF = 0.0, prevGainDb = 0.0, prevLnQ = 0.0;      // at the tick before it
            double goalLnF = 0.0, goalGainDb = 0.0, goalLnQ = 0.0;
            double stepLnF = 0.0, stepGainDb = 0.0, stepLnQ = 0.0;      // per tick
            int ticksLeft = 0;
            bool moving = false;        // this interval runs between two tick values
            bool endsOnGoal = false;    // and its end is the goal itself

            double ic1[eq::maxSections] = {};
            double ic2[eq::maxSections] = {};
            double s[eq::maxSections] = {};
        };

        if (grid < 1 || rampTicks < 1 || script.blockStart.empty()
            || script.blockStart.size() != script.settings.size() || script.blockStart[0] != 0)
            return {};

        const size_t numBands = script.settings[0].size();

        for (const auto& perBlock : script.settings)
            if (perBlock.size() != numBands)
                return {};

        const auto designer = designerFor (rate);
        std::vector<Track> tracks (numBands);
        std::vector<double> out (input.size(), 0.0);

        const auto gridLength = (std::uint64_t) grid;
        const double perSample = 1.0 / (double) grid;
        const double ticks = (double) rampTicks;
        size_t block = 0;

        for (size_t n = 0; n < input.size(); ++n)
        {
            // A host block starts here: the parameters are read.
            while (block < script.blockStart.size() && (size_t) script.blockStart[block] == n)
            {
                for (size_t b = 0; b < numBands; ++b)
                {
                    Track& t = tracks[b];
                    const eq::BandSettings& seen = script.settings[block][b];

                    if (block == 0)
                    {
                        t.goal = seen;
                        t.goalLnF = std::log (clampRange (seen.freqHz, eq::rawFreqMin, eq::rawFreqMax));
                        t.goalGainDb = clampRange (seen.gainDbEff, eq::gainEffMin, eq::gainEffMax);
                        t.goalLnQ = std::log (clampRange (seen.q, eq::rawQMin, eq::rawQMax));
                        t.lnF = t.goalLnF;
                        t.gainDb = t.goalGainDb;
                        t.lnQ = t.goalLnQ;
                        t.prevLnF = t.lnF;
                        t.prevGainDb = t.gainDb;
                        t.prevLnQ = t.lnQ;
                    }
                    else if (sameSettings (seen, t.goal))
                    {
                        t.hasWaiting = false;
                    }
                    else
                    {
                        t.waiting = seen;
                        t.hasWaiting = true;
                    }
                }

                ++block;
            }

            const int phase = (int) ((clockAtStart + (std::uint64_t) n) % gridLength);

            // A grid tick.
            if (phase == 0)
            {
                for (auto& t : tracks)
                {
                    t.prevLnF = t.lnF;
                    t.prevGainDb = t.gainDb;
                    t.prevLnQ = t.lnQ;

                    if (t.hasWaiting)
                    {
                        t.goal = t.waiting;
                        t.hasWaiting = false;

                        t.goalLnF = std::log (clampRange (t.goal.freqHz, eq::rawFreqMin, eq::rawFreqMax));
                        t.goalGainDb = clampRange (t.goal.gainDbEff, eq::gainEffMin, eq::gainEffMax);
                        t.goalLnQ = std::log (clampRange (t.goal.q, eq::rawQMin, eq::rawQMax));

                        t.stepLnF = (t.goalLnF - t.lnF) / ticks;
                        t.stepGainDb = (t.goalGainDb - t.gainDb) / ticks;
                        t.stepLnQ = (t.goalLnQ - t.lnQ) / ticks;
                        t.ticksLeft = rampTicks;
                    }

                    if (t.ticksLeft > 1)
                    {
                        t.lnF += t.stepLnF;
                        t.gainDb += t.stepGainDb;
                        t.lnQ += t.stepLnQ;
                        --t.ticksLeft;
                        t.moving = true;
                        t.endsOnGoal = false;
                    }
                    else if (t.ticksLeft == 1)
                    {
                        t.lnF = t.goalLnF;
                        t.gainDb = t.goalGainDb;
                        t.lnQ = t.goalLnQ;
                        t.ticksLeft = 0;
                        t.moving = true;
                        t.endsOnGoal = true;
                    }
                    else
                    {
                        t.moving = false;
                        t.endsOnGoal = false;
                    }
                }
            }

            double v = (double) input[n];
            const bool lastOfInterval = phase == grid - 1;

            for (auto& t : tracks)
            {
                eq::BandSettings now = t.goal;

                if (t.moving && ! (t.endsOnGoal && lastOfInterval))
                {
                    double lnF = t.lnF, gainDb = t.gainDb, lnQ = t.lnQ;

                    if (! lastOfInterval)
                    {
                        const double u = (double) (phase + 1) * perSample;
                        lnF = t.prevLnF + u * (t.lnF - t.prevLnF);
                        gainDb = t.prevGainDb + u * (t.gainDb - t.prevGainDb);
                        lnQ = t.prevLnQ + u * (t.lnQ - t.prevLnQ);
                    }

                    now.freqHz = clampRange (std::exp (lnF), eq::rawFreqMin, eq::rawFreqMax);
                    now.gainDbEff = clampRange (gainDb, eq::gainEffMin, eq::gainEffMax);
                    now.q = clampRange (std::exp (lnQ), eq::rawQMin, eq::rawQMax);
                }

                eq::BandDesign design {};
                designer->design (now, design);

                const int count = juce::jlimit (0, (int) eq::maxSections, design.numSections);

                for (int i = 0; i < count; ++i)
                {
                    const eq::SectionRuntime runtime = eq::toRuntime (design.params[(size_t) i]);

                    if (runtime.hasSvf)
                        v = eq::svfTick (runtime.svf, t.ic1[i], t.ic2[i], v);

                    if (runtime.hasOnePole)
                        v = eq::onePoleTick (runtime.onePole, t.s[i], v);
                }
            }

            out[n] = v;
        }

        return out;
    }

    /** The no-smoothing control: the Phase 2.2 rule. The settings a block
        start reads are designed there and held for the block; the filter
        state is kept. One lane, bands in series, double throughout. */
    std::vector<double> hardRender (const std::vector<float>& input, double rate, const RefScript& script)
    {
        struct Stage
        {
            eq::BandSettings held {};
            bool designed = false;
            int numSections = 0;
            eq::SectionRuntime runtime[eq::maxSections];

            double ic1[eq::maxSections] = {};
            double ic2[eq::maxSections] = {};
            double s[eq::maxSections] = {};
        };

        if (script.blockStart.empty() || script.blockStart.size() != script.settings.size()
            || script.blockStart[0] != 0)
            return {};

        const size_t numBands = script.settings[0].size();

        for (const auto& perBlock : script.settings)
            if (perBlock.size() != numBands)
                return {};

        const auto designer = designerFor (rate);
        std::vector<Stage> stages (numBands);
        std::vector<double> out (input.size(), 0.0);
        size_t block = 0;

        for (size_t n = 0; n < input.size(); ++n)
        {
            while (block < script.blockStart.size() && (size_t) script.blockStart[block] == n)
            {
                for (size_t b = 0; b < numBands; ++b)
                {
                    Stage& st = stages[b];
                    const eq::BandSettings& seen = script.settings[block][b];

                    if (st.designed && sameSettings (seen, st.held))
                        continue;

                    eq::BandDesign design {};
                    designer->design (seen, design);

                    st.numSections = juce::jlimit (0, (int) eq::maxSections, design.numSections);

                    for (int i = 0; i < st.numSections; ++i)
                        st.runtime[i] = eq::toRuntime (design.params[(size_t) i]);

                    st.held = seen;
                    st.designed = true;
                }

                ++block;
            }

            double v = (double) input[n];

            for (auto& st : stages)
                for (int i = 0; i < st.numSections; ++i)
                {
                    if (st.runtime[i].hasSvf)
                        v = eq::svfTick (st.runtime[i].svf, st.ic1[i], st.ic2[i], v);

                    if (st.runtime[i].hasOnePole)
                        v = eq::onePoleTick (st.runtime[i].onePole, st.s[i], v);
                }

            out[n] = v;
        }

        return out;
    }

    //==========================================================================
    // M1, M1h, M1k. G3 sweep rows (PLAN-2.3 P12; ported from ramp_bench g3)
    //==========================================================================
    struct Motion
    {
        const char* name;
        int number;             // 1 ... 13, as PLAN-2.3 P12 numbers them
        int type, slope;
        double fA, fB, gA, gB, qA, qB;
        double sweepSeconds;
        int pattern;            // 0: A | sweep | hold | back | hold.  1: there and back with no hold, twice
    };

    std::vector<Motion> motionsFor (double tone)
    {
        return {
            { "Bell +18 dB Q 1, 10 Hz <-> 30 kHz in 50 ms, p0",             1,  kBell,      1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 0 },
            { "Bell +18 dB Q 1, 10 Hz <-> 30 kHz in 50 ms, p1",             2,  kBell,      1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 1 },
            { "Bell at the tone Q 1, -30 <-> +30 dB in 20 ms",              3,  kBell,      1, tone, tone, -30.0, 30.0, 1.0, 1.0, 0.020, 0 },
            { "Bell +18 dB at 2x the tone, Q 0.025 <-> 40 in 20 ms",        4,  kBell,      1, 2.0 * tone, 2.0 * tone, 18.0, 18.0, 0.025, 40.0, 0.020, 0 },
            { "Low Cut 96 dB/oct Q 1, 10 Hz <-> 30 kHz in 50 ms, p0",       5,  kLowCut,    7, 10.0, 30000.0, 0.0, 0.0, 1.0, 1.0, 0.050, 0 },
            { "Low Cut 96 dB/oct Q 1, 10 Hz <-> 30 kHz in 50 ms, p1",       6,  kLowCut,    7, 10.0, 30000.0, 0.0, 0.0, 1.0, 1.0, 0.050, 1 },
            { "Low Cut 96 dB/oct at 30 Hz, Q 0.025 <-> 40 in 20 ms",        7,  kLowCut,    7, 30.0, 30.0, 0.0, 0.0, 0.025, 40.0, 0.020, 0 },
            { "High Shelf 12 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p0", 8,  kHighShelf, 1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 0 },
            { "High Shelf 12 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p1", 9,  kHighShelf, 1, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 1 },
            { "High Shelf 12 dB/oct at tone / 2, -30 <-> +30 dB in 20 ms",  10, kHighShelf, 1, 0.5 * tone, 0.5 * tone, -30.0, 30.0, 1.0, 1.0, 0.020, 0 },
            { "High Shelf 12 dB/oct +18 dB at the tone, Q 0.025 <-> 40",    11, kHighShelf, 1, tone, tone, 18.0, 18.0, 0.025, 40.0, 0.020, 0 },
            { "High Shelf 96 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p0", 12, kHighShelf, 7, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 0 },
            { "High Shelf 96 dB/oct +18 dB, 10 Hz <-> 30 kHz in 50 ms, p1", 13, kHighShelf, 7, 10.0, 30000.0, 18.0, 18.0, 1.0, 1.0, 0.050, 1 }
        };
    }

    // Where the parameter stands at `seconds`: 0 at A, 1 at B.
    double motionPhase (const Motion& m, double seconds)
    {
        const double lead = 0.5;
        const double sweep = m.sweepSeconds;

        if (seconds < lead)
            return 0.0;

        double t = seconds - lead;

        if (m.pattern == 0)
        {
            if (t < sweep)
                return t / sweep;

            t -= sweep;

            if (t < 0.4)
                return 1.0;

            t -= 0.4;

            if (t < sweep)
                return 1.0 - t / sweep;

            return 0.0;
        }

        t = std::fmod (t, sweep + 0.5);

        if (t < 0.5 * sweep)
            return t / (0.5 * sweep);

        if (t < sweep)
            return 2.0 - t / (0.5 * sweep);

        return 0.0;
    }

    double motionSeconds (const Motion& m)
    {
        return m.pattern == 0 ? 0.5 + m.sweepSeconds + 0.4 + m.sweepSeconds + 0.4
                              : 0.5 + 2.0 * (m.sweepSeconds + 0.5);
    }

    struct SweepJob
    {
        Motion motion;
        double tone;
        int blockSize;
        double rate;
        bool withHard;
    };

    struct SweepReading
    {
        bool valid = false;         // renders complete and finite, reference kappa above 0
        double pluginKappa = 0.0;
        double refKappa = 0.0;
        double hardKappa = 0.0;
        bool hasHard = false;
    };

    /** One reading: a sine at -6 dBFS through one band of a mono instance,
        the moved parameter written at every block start (read once per block,
        as the processor does); the first 0.3 s is not scored. The reference
        and the control get the same float input and the settings
        eq::makeBandInput makes of the same raw values. */
    SweepReading runSweep (Processor& p, const Handles& h, const SweepJob& job)
    {
        const Motion& m = job.motion;
        const double rate = job.rate;
        const int blockSize = job.blockSize;
        const int length = (int) std::lround (motionSeconds (m) * rate);
        const int numBlocks = (length + blockSize - 1) / blockSize;
        const BandHandles& hb = h.band[0];

        juce::AudioBuffer<float> input (1, length);

        {
            float* x = input.getWritePointer (0);

            for (int i = 0; i < length; ++i)
                x[i] = (float) (0.5 * std::sin (2.0 * eq::pi * job.tone * (double) i / rate));
        }

        std::vector<float> freqs ((size_t) numBlocks), gains ((size_t) numBlocks), qs ((size_t) numBlocks);
        RefScript script;

        const float scale = h.gainScale->load();
        eq::RawBand raw = rawOfHandles (hb);

        const double lnFA = std::log (m.fA), lnFB = std::log (m.fB);
        const double lnQA = std::log (m.qA), lnQB = std::log (m.qB);

        for (int k = 0; k < numBlocks; ++k)
        {
            const auto at = (size_t) k;
            const double u = motionPhase (m, (double) (k * blockSize) / rate);

            freqs[at] = juce::jlimit (10.0f, 30000.0f, (float) std::exp (lnFA + u * (lnFB - lnFA)));
            gains[at] = juce::jlimit (-30.0f, 30.0f, (float) (m.gA + u * (m.gB - m.gA)));
            qs[at] = juce::jlimit (0.025f, 40.0f, (float) std::exp (lnQA + u * (lnQB - lnQA)));

            raw.freq = freqs[at];
            raw.gain = gains[at];
            raw.q = qs[at];

            script.blockStart.push_back (k * blockSize);
            script.settings.push_back ({ eq::makeBandInput (raw, scale, 0.0).settings });
        }

        const auto output = renderWith (p, input, rate, blockSize,
                                        [blockSize] (int, int) { return blockSize; },
                                        [&hb, &freqs, &gains, &qs] (BlockInfo& current)
                                        {
                                            const auto slot = (size_t) current.index;

                                            if (slot < freqs.size())
                                            {
                                                hb.freq->store (freqs[slot]);
                                                hb.gain->store (gains[slot]);
                                                hb.q->store (qs[slot]);
                                            }
                                        });

        const auto from = (size_t) std::lround (0.3 * rate);
        const auto source = channelOf (input, 0);
        const auto yPlugin = widened (output, 0);
        const auto yRef = refRender (source, rate, harnessGrid (rate), harnessRampTicks (rate), script);

        SweepReading r;
        r.pluginKappa = localKappa (yPlugin, rate, from, yPlugin.size());
        r.refKappa = localKappa (yRef, rate, from, yRef.size());

        bool hardOk = true;

        if (job.withHard)
        {
            const auto yHard = hardRender (source, rate, script);
            r.hardKappa = localKappa (yHard, rate, from, yHard.size());
            r.hasHard = true;
            hardOk = yHard.size() == yPlugin.size() && std::isfinite (r.hardKappa);
        }

        r.valid = allFinite (output) && yRef.size() == yPlugin.size() && hardOk
               && std::isfinite (r.pluginKappa) && std::isfinite (r.refKappa) && r.refKappa > 0.0;
        return r;
    }

    juce::String sweepLabel (const SweepJob& job)
    {
        return "#" + juce::String (job.motion.number) + " " + job.motion.name + ", " + fmtG (job.tone, 4)
             + " Hz, block " + juce::String (job.blockSize);
    }

    // The tick-clock script shared by TK (the plugin) and M1's guard (the
    // reference): idle to sample 1000, a 0 dB bell from there, one more block
    // boundary, and a 0 -> +24 dB gain edit seen at the block that starts on
    // it. Expected first sample that differs from the unedited render, at
    // 48 kHz (K = 16), counted from prepareToPlay.
    struct TickCase
    {
        int boundary;
        int expected;
    };

    constexpr TickCase kTickCases[3] = { { 1036, 1040 }, { 1041, 1056 }, { 1024, 1024 } };
    constexpr int kTickActiveFrom = 1000;
    constexpr int kTickLength = 4096;

    // refRender on that script (the part from sample 1000 on): the index,
    // counted from prepareToPlay, of the first sample the edit changes.
    int referenceStartIndex (int boundary)
    {
        const double rate = 48000.0;
        const int length = kTickLength - kTickActiveFrom;
        const auto noise = makeNoiseAt (1, length, 0x7C4000 + boundary, kNoise12);

        eq::RawBand raw;
        raw.enabled = 1.0f;
        raw.bypass = 0.0f;
        raw.type = (float) kBell;
        raw.freq = 1000.0f;
        raw.gain = 0.0f;
        raw.q = 1.0f;
        raw.slope = 1.0f;
        raw.placement = (float) kStereo;

        const eq::BandSettings flat = eq::makeBandInput (raw, 100.0f, 0.0).settings;
        raw.gain = 24.0f;
        const eq::BandSettings boosted = eq::makeBandInput (raw, 100.0f, 0.0).settings;

        RefScript plain, edited;
        int position = 0;

        for (int index = 0; position < length; ++index)
        {
            plain.blockStart.push_back (position);
            plain.settings.push_back ({ flat });
            edited.blockStart.push_back (position);
            edited.settings.push_back ({ index == 0 ? flat : boosted });

            position += index == 0 ? boundary - kTickActiveFrom : 64;
        }

        const auto source = channelOf (noise, 0);
        const auto grid = harnessGrid (rate);
        const auto ticks = harnessRampTicks (rate);
        const auto a = refRender (source, rate, grid, ticks, plain, (std::uint64_t) kTickActiveFrom);
        const auto b = refRender (source, rate, grid, ticks, edited, (std::uint64_t) kTickActiveFrom);

        const int first = (a.size() == source.size() && ! a.empty()) ? firstDifference (a, b) : -1;
        return first < 0 ? -1 : first + kTickActiveFrom;
    }

    void sweepRows()
    {
        section ("M1. G3 sweep rows through processBlock (Phase 2.3: DSP-04; PLAN-2.3 P11, P12)");

        // Guards on the reference itself (PLAN-2.3 R3): its grid and ramp
        // length are the engine's at every rate, and its "a target seen at a
        // block start acts at the next tick" rule lands on the plan's sample
        // (TK asserts the same three indices on the plugin).
        Tally guard;

        for (const double fs : { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
            guard.check (harnessGrid (fs) == eq::controlGridSize (fs) && harnessRampTicks (fs) == eq::rampLengthTicks (fs),
                         "harness grid / ramp length differ from the engine's at " + fmtG (fs, 6) + " Hz");

        juce::String starts;

        for (const auto& c : kTickCases)
        {
            const int at = referenceStartIndex (c.boundary);
            guard.check (at == c.expected, "reference: edit seen at " + juce::String (c.boundary) + " acts at "
                                               + juce::String (at) + ", expected " + juce::String (c.expected));
            starts += (starts.isEmpty() ? "" : " / ") + juce::String (at);
        }

        // 52 readings at 48 kHz, then motion 8 at 50 Hz, block 32, on the two other grids.
        const double tones[2] = { 50.0, 1000.0 };
        std::vector<SweepJob> jobs;

        for (const double tone : tones)
            for (const auto& m : motionsFor (tone))
                for (const int blockSize : { 32, 512 })
                    jobs.push_back ({ m, tone, blockSize, 48000.0, blockSize == 512 });

        const size_t numMain = jobs.size();

        for (const double fs : { 96000.0, 192000.0 })
            jobs.push_back ({ motionsFor (50.0)[7], 50.0, 32, fs, false });

        // Instances and settings on this thread; renders on the workers.
        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (const auto& job : jobs)
        {
            const Motion& m = job.motion;

            instances.push_back (makeBands (1, 1, [&m] (int)
            {
                return makeSpec (m.type, (float) m.fA, (float) m.gA, (float) m.qA, m.slope, kStereo);
            }));

            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<SweepReading> readings (jobs.size());

        parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &readings] (int j)
        {
            const auto slot = (size_t) j;
            readings[slot] = runSweep (*instances[slot], handles[slot], jobs[slot]);
        });

        // M1
        Tally sweeps;
        double worstSweep = 0.0;
        juce::String worstSweepAt;

        for (size_t j = 0; j < numMain; ++j)
        {
            const SweepReading& r = readings[j];
            const double ratio = ratioOf (r.pluginKappa, r.refKappa);
            const juce::String label = sweepLabel (jobs[j]);

            std::printf ("      M1  %-88s plugin %.3e  reference %.3e  plugin / reference %.3f%s\n",
                         label.toRawUTF8(), r.pluginKappa, r.refKappa, ratio, r.valid ? "" : "  INVALID");

            sweeps.check (r.valid && ratio <= 4.0, label + ": " + fmtG (ratio, 4) + "x");

            if (! (ratio <= worstSweep))
            {
                worstSweep = ratio;
                worstSweepAt = label;
            }
        }

        row (sweeps.bad == 0 && sweeps.total == 52 && guard.bad == 0 && guard.total == 11,
             "M1 13 motions x 50 Hz / 1 kHz x blocks 32 / 512, 48 kHz: local kappa of the float output <= 4.0x the same-motion per-sample reference",
             sweeps.detail() + "  worst " + fmtG (worstSweep, 4) + "x (" + worstSweepAt + "); reference guards " + guard.detail()
                 + " (reference first acts at " + starts + ", expected 1040 / 1056 / 1024)");

        // M1h: the no-smoothing control, at block 512 (at block 32 the 2.2
        // staircase is already a fine grid; RESEARCH-2.3 Q1).
        Tally hard;
        int voids = 0;
        double weakestHard = std::numeric_limits<double>::infinity();
        juce::String voidFigures;

        for (size_t j = 0; j < numMain; ++j)
        {
            const SweepReading& r = readings[j];

            if (! r.hasHard)
                continue;

            const SweepJob& job = jobs[j];
            const double ratio = ratioOf (r.hardKappa, r.refKappa);
            const juce::String label = sweepLabel (job);

            // Void by construction (ARCHITECTURE G3, PLAN-2.3 P12): printed, not counted.
            const bool lowTone = job.tone < 100.0;
            const bool isVoid = (job.motion.number == 5 && lowTone) || (job.motion.number == 7 && ! lowTone);

            std::printf ("      M1h %-88s hard %.3e  reference %.3e  hard / reference %.2f%s%s\n",
                         label.toRawUTF8(), r.hardKappa, r.refKappa, ratio, r.valid ? "" : "  INVALID",
                         isVoid ? (lowTone ? "  VOID, not counted (the tone dies during the sweep)"
                                           : "  VOID, not counted (the Q of a 30 Hz cut barely moves a 1 kHz tone)")
                                : "");

            if (isVoid)
            {
                ++voids;
                voidFigures += (voidFigures.isEmpty() ? "" : "; ") + juce::String ("motion ") + juce::String (job.motion.number)
                             + " at " + fmtG (job.tone, 4) + " Hz reads " + fmtG (ratio, 3) + "x";
                continue;
            }

            hard.check (r.valid && ratio >= 10.0, label + ": " + fmtG (ratio, 4) + "x");
            weakestHard = juce::jmin (weakestHard, ratio);
        }

        row (hard.bad == 0 && hard.total == 24 && voids == 2,
             "M1h no smoothing (hardRender, block 512, the plugin's input and settings) reads >= 10x the same-motion reference (control fires)",
             hard.detail() + "  weakest " + fmtG (weakestHard, 4) + "x; void, printed and not counted: " + voidFigures);

        // M1k
        Tally grids;
        juce::String gridFigures;

        for (size_t j = numMain; j < jobs.size(); ++j)
        {
            const SweepReading& r = readings[j];
            const double ratio = ratioOf (r.pluginKappa, r.refKappa);
            const juce::String label = sweepLabel (jobs[j]) + ", " + fmtG (jobs[j].rate / 1000.0, 4) + " kHz (K = "
                                     + juce::String (harnessGrid (jobs[j].rate)) + ")";

            std::printf ("      M1k %-88s plugin %.3e  reference %.3e  plugin / reference %.3f%s\n",
                         label.toRawUTF8(), r.pluginKappa, r.refKappa, ratio, r.valid ? "" : "  INVALID");

            grids.check (r.valid && ratio <= 4.0, label + ": " + fmtG (ratio, 4) + "x");
            gridFigures += (gridFigures.isEmpty() ? "" : "; ") + fmtG (jobs[j].rate / 1000.0, 4) + " kHz " + fmtG (ratio, 4) + "x";
        }

        row (grids.bad == 0 && grids.total == 2,
             "M1k motion 8 at 50 Hz, block 32, at 96 kHz (K = 32) and 192 kHz (K = 64): <= 4.0x the same-motion reference",
             grids.detail() + "  " + gridFigures);
    }

    //==========================================================================
    // M2 - M6. The gaps carried from VERIFICATION-2.2 (issues 1 - 4)
    //==========================================================================
    struct KeepFigures
    {
        double ownD2 = 0.0;     // the ring's largest second difference in the 50 ms before the edit
        double after = 0.0;     // largest |y[n] - (2 y[n-1] - y[n-2])| over the 20 ms from the edit
        double ringPeak = 0.0;  // the ring's peak in the 50 ms before the edit

        double ratio() const    { return ratioOf (after, ownD2); }
    };

    KeepFigures keepFigures (const juce::AudioBuffer<float>& out, int editAt, double rate)
    {
        KeepFigures f;
        const float* y = out.getReadPointer (0);
        const int span = (int) std::lround (0.020 * rate);
        const int lookBack = (int) std::lround (0.050 * rate);

        const auto secondDifference = [y] (int i)
        {
            return std::abs ((double) y[i + 1] - 2.0 * (double) y[i] + (double) y[i - 1]);
        };

        for (int i = editAt - lookBack; i + 1 < editAt; ++i)
        {
            f.ownD2 = juce::jmax (f.ownD2, secondDifference (i));
            f.ringPeak = juce::jmax (f.ringPeak, std::abs ((double) y[i]));
        }

        for (int n = editAt; n < editAt + span; ++n)
            f.after = juce::jmax (f.after, secondDifference (n - 1));

        return f;
    }

    // RM-keep's stimulus through the plugin: a bell at 40 Hz, Q 20, +24 dB,
    // mono, rung by a 10 ms burst; blocks of 480, so sample 24000 starts a
    // block (and is a tick). edit 0: none. 1: gain -> +18 dB. 2: Bell -> Low Shelf.
    constexpr int kKeepBlock = 480;
    constexpr int kKeepEditBlock = 50;

    juce::AudioBuffer<float> keepRender (const juce::AudioBuffer<float>& burst, double rate, int edit)
    {
        auto p = makeBands (1, 1, [] (int) { return makeSpec (kBell, 40.0f, 24.0f, 20.0f, 1, kStereo); });
        const Handles h = handlesOf (*p);
        const BandHandles& hb = h.band[0];

        return renderWith (*p, burst, rate, kKeepBlock,
                           [] (int, int) { return kKeepBlock; },
                           [&hb, edit] (BlockInfo& current)
                           {
                               if (current.index != kKeepEditBlock)
                                   return;

                               if (edit == 1)
                                   hb.gain->store (18.0f);

                               if (edit == 2)
                                   hb.type->store ((float) kLowShelf);
                           });
    }

    void carriedGapRows()
    {
        section ("M2 - M6. gain scale, band bypass, re-prepare, Mid / Side response, kept state (Phase 2.3; VERIFICATION-2.2 issues 1 - 4)");

        // M2: gain_scale 100 -> 25 on three Stereo bells, three sines at their centres.
        {
            constexpr int blockSize = 512, editBlock = 235, window = 4800;
            const double rate = 48000.0;
            const int editAt = editBlock * blockSize;               // 120320: a block start and a tick
            const int oneSecond = (int) std::lround (rate);
            const int total = editAt + oneSecond + window;
            const int numBlocks = (total + blockSize - 1) / blockSize;

            const double tones[3] = { 50.0, 500.0, 5000.0 };
            const float bellGains[3] = { 24.0f, 6.0f, 0.0f };
            const double nominalAfter[3] = { 6.0, 1.5, 0.0 };

            juce::AudioBuffer<float> input (2, total);

            for (int i = 0; i < total; ++i)
            {
                double v = 0.0;

                for (const double tone : tones)
                    v += 0.1 * std::sin (2.0 * eq::pi * tone * (double) i / rate);

                input.setSample (0, i, (float) v);
                input.setSample (1, i, (float) v);
            }

            auto p = makeBands (2, 3, [&tones, &bellGains] (int index)
            {
                return makeSpec (kBell, (float) tones[index], bellGains[index], 8.0f, 1, kStereo);
            });

            const Handles h = handlesOf (*p);
            const auto designer = designerFor (rate);
            const std::vector<double> at (std::begin (tones), std::end (tones));
            const auto closedBefore = chainRefDb (*p, *designer, rate, at);

            RefScript script;

            for (int k = 0; k < numBlocks; ++k)
            {
                const float scale = k < editBlock ? 100.0f : 25.0f;
                std::vector<eq::BandSettings> perBand;

                for (size_t b = 0; b < 3; ++b)
                    perBand.push_back (eq::makeBandInput (rawOfHandles (h.band[b]), scale, 0.0).settings);

                script.blockStart.push_back (k * blockSize);
                script.settings.push_back (perBand);
            }

            const auto output = renderWith (*p, input, rate, blockSize,
                                            [] (int, int) { return 512; },
                                            [&h] (BlockInfo& current)
                                            {
                                                if (current.index == 235)
                                                    h.gainScale->store (25.0f);
                                            });

            const auto closedAfter = chainRefDb (*p, *designer, rate, at);
            const bool editApplied = sameBits (rawOf (*p, "gain_scale"), 25.0f);

            Tally steady;
            juce::String figures;

            for (size_t i = 0; i < 3; ++i)
            {
                const double before = toneGainDb (input, output, editAt - window, window, tones[i], rate);
                const double after = toneGainDb (input, output, editAt + oneSecond, window, tones[i], rate);

                steady.check (std::abs (after - closedAfter[i]) <= 0.01 && std::abs (closedAfter[i] - nominalAfter[i]) <= 0.01,
                              fmtG (tones[i], 4) + " Hz after: " + fmtG (after, 6) + " dB, closed form " + fmtG (closedAfter[i], 6)
                                  + ", nominal " + fmtG (nominalAfter[i], 3));
                steady.check (std::abs (before - closedBefore[i]) <= 0.01,
                              fmtG (tones[i], 4) + " Hz before: " + fmtG (before, 6) + " dB, closed form " + fmtG (closedBefore[i], 6));

                figures += (figures.isEmpty() ? "" : "; ") + fmtG (tones[i], 4) + " Hz " + fmtG (before, 6) + " -> "
                         + fmtG (after, 6) + " dB (closed form " + fmtG (closedAfter[i], 6) + ")";
            }

            const bool scaleBites = std::abs (closedBefore[0] - closedAfter[0]) > 1.0 && std::abs (closedBefore[1] - closedAfter[1]) > 1.0;

            row (steady.bad == 0 && steady.total == 6 && editApplied && scaleBites && allFinite (output) && activeBands (*p) == 3,
                 "M2 (i) three Stereo bells +24 / +6 / 0 dB, gain_scale 100 -> 25 mid-render: steady-state gains 1 s later are +6 / +1.5 / 0 dB within 0.01 dB of the closed forms",
                 steady.detail() + "  " + figures + (editApplied ? "" : "  (edit not applied)"));

            // (ii) the move itself
            const auto from = (size_t) std::lround (1.0 * rate);
            const auto source = channelOf (input, 0);
            const auto yPlugin = widened (output, 0);
            const auto yRef = refRender (source, rate, harnessGrid (rate), harnessRampTicks (rate), script);
            const auto yHard = hardRender (source, rate, script);

            const double pluginKappa = localKappa (yPlugin, rate, from, yPlugin.size());
            const double refKappa = localKappa (yRef, rate, from, yRef.size());
            const double hardKappa = localKappa (yHard, rate, from, yHard.size());
            const double ratio = ratioOf (pluginKappa, refKappa);

            // L = R in, one Stereo band chain: the two lanes must stay equal while the bands ramp (PLAN-2.3 P4).
            const bool lanesEqual = std::memcmp (output.getReadPointer (0), output.getReadPointer (1),
                                                 sizeof (float) * (size_t) total) == 0;

            row (yRef.size() == yPlugin.size() && refKappa > 0.0 && ratio <= 4.0 && lanesEqual && allFinite (output),
                 "M2 (ii) the same move: local kappa of the float output <= 4.0x the same-motion reference (refRender); L and R stay memcmp-equal on L = R input",
                 "plugin " + fmtG (pluginKappa, 4) + ", reference " + fmtG (refKappa, 4) + ", plugin / reference "
                     + fmtG (ratio, 4) + "x; no smoothing (hardRender) reads " + fmtG (ratioOf (hardKappa, refKappa), 4) + "x"
                     + (lanesEqual ? "" : "; L AND R DIFFER"));
        }

        // M2c: the same edit where gain is inert (PLAN-2.3 P6).
        {
            constexpr int blockSize = 512, editBlock = 10;
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x3B2C0002, kNoise12);

            // Gain +12 dB on both, so the effective gain would move from 12 to 3 dB were it read.
            const auto specOf = [] (int index, bool bellInstead)
            {
                if (index == 0)
                    return makeSpec (kLowCut, 80.0f, 12.0f, 1.0f, 3, kStereo);

                return makeSpec (bellInstead ? kBell : kNotch, 1000.0f, 12.0f, 4.0f, 1, kStereo);
            };

            const auto renderPair = [&input, &specOf] (bool bellInstead, bool withEdit, bool& applied)
            {
                auto q = makeBands (2, 2, [&specOf, bellInstead] (int index) { return specOf (index, bellInstead); });
                const Handles hq = handlesOf (*q);

                auto rendered = renderWith (*q, input, kRenderRate, 512,
                                            [] (int, int) { return 512; },
                                            [&hq, withEdit] (BlockInfo& current)
                                            {
                                                if (withEdit && current.index == 10)
                                                    hq.gainScale->store (25.0f);
                                            });

                applied = sameBits (hq.gainScale->load(), 25.0f);
                return rendered;
            };

            bool plainApplied = false, editApplied = false, bellPlainApplied = false, bellEditApplied = false;
            const auto plain = renderPair (false, false, plainApplied);
            const auto edited = renderPair (false, true, editApplied);
            const auto bellPlain = renderPair (true, false, bellPlainApplied);
            const auto bellEdited = renderPair (true, true, bellEditApplied);

            const bool identical = sameAudio (plain, edited);
            const bool acts = ! sameAudio (plain, input);
            const int bellFirst = firstDifference (bellPlain, bellEdited);
            const bool controlFires = bellFirst >= editBlock * blockSize;

            row (identical && acts && editApplied && ! plainApplied && bellEditApplied && ! bellPlainApplied && controlFires,
                 "M2c gain_scale 100 -> 25 with a Low Cut and a Notch as the only bands: output bit-identical to the render without the edit; with a Bell for the Notch it differs",
                 juce::String (identical ? "" : "the edit changed the output; ") + (acts ? "" : "the bands do not act; ")
                     + (editApplied ? "" : "edit not applied; ")
                     + "control: Bell in place of the Notch first differs at sample " + juce::String (bellFirst)
                     + " (edit seen at " + juce::String (editBlock * blockSize) + ")");
        }

        // M3: band bypass on, then off, on the only active band.
        {
            constexpr int blockSize = 512, onBlock = 10, offBlock = 20;
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x3B300002, kNoise12);
            const int total = input.getNumSamples();
            const int onAt = onBlock * blockSize;
            const int offAt = offBlock * blockSize;

            const auto spec = makeSpec (kLowShelf, 300.0f, 9.0f, 1.0f, 3, kStereo);
            const auto one = [&spec] (int) { return spec; };

            auto p = makeBands (2, 1, one);
            const Handles h = handlesOf (*p);

            const auto output = renderWith (*p, input, kRenderRate, 512,
                                            [] (int, int) { return 512; },
                                            [&h] (BlockInfo& current)
                                            {
                                                if (current.index == 10)
                                                    h.band[0].bypass->store (1.0f);

                                                if (current.index == 20)
                                                    h.band[0].bypass->store (0.0f);
                                            });

            auto steady = makeBands (2, 1, one);
            const auto steadyOut = render (*steady, input, blockSize, false);

            auto fresh = makeBands (2, 1, one);
            const auto freshOut = render (*fresh, sliceOf (input, offAt, total - offAt), blockSize, false);

            Tally tally;
            tally.check (sameFrom (output, 0, steadyOut, 0, onAt), "output before the bypass differs from the steady render");
            tally.check (sameFrom (output, onAt, input, onAt, offAt - onAt), "bypass on: the output is not the input from that block");
            tally.check (sameFrom (output, offAt, freshOut, 0, total - offAt), "bypass off: the output differs from a fresh instance fed the remaining input");
            tally.check (! sameFrom (steadyOut, onAt, input, onAt, offAt - onAt), "control: the band does not act on the audio");
            tally.check (! sameFrom (steadyOut, offAt, freshOut, 0, total - offAt), "control: an uninterrupted render equals a fresh one (state does not matter)");
            tally.check (activeBands (*p) == 1 && activeBands (*steady) == 1, "the band is not the only active one at the end");

            row (tally.bad == 0 && tally.total == 6,
                 "M3 bNN_bypass on the only active band, on at block 10, off at block 20: on, out memcmp-equal to in from that block; off, memcmp-equal to a fresh instance fed the remaining input",
                 tally.detail());
        }

        // M4: prepare at 44.1 kHz, render, prepare at 96 kHz, render.
        {
            const auto in44 = makeNoiseAt (2, 44100, 0x3B440002, kNoise12);
            const auto in96 = makeNoiseAt (2, 96000, 0x3B960002, kNoise12);
            const auto six = [] (int index) { return channelBandSpec (index, kStereo); };
            const auto blocks512 = [] (int, int) { return 512; };
            const auto noHook = [] (BlockInfo&) {};

            auto reused = makeBands (2, 6, six);
            const auto first = renderWith (*reused, in44, 44100.0, 512, blocks512, noHook);
            const auto second = renderWith (*reused, in96, 96000.0, 512, blocks512, noHook);

            auto fresh = makeBands (2, 6, six);
            const auto freshOut = renderWith (*fresh, in96, 96000.0, 512, blocks512, noHook);

            // Control: the same input through designs made for 44.1 kHz is another signal.
            auto stale = makeBands (2, 6, six);
            const auto staleOut = renderWith (*stale, in96, 44100.0, 512, blocks512, noHook);

            const bool same = sameAudio (second, freshOut);
            const bool acts = ! sameAudio (first, in44) && ! sameAudio (second, in96) && activeBands (*reused) == 6;
            const bool rateMatters = ! sameAudio (freshOut, staleOut);

            row (same && acts && rateMatters,
                 "M4 one instance: prepare 44.1 kHz, render 1 s with 6 bands, prepare 96 kHz, render: memcmp-equal to a fresh instance prepared at 96 kHz",
                 (same ? juce::String()
                       : "differs from the fresh instance (first at sample " + juce::String (firstDifference (second, freshOut)) + "); ")
                     + (acts ? "" : "the bands do not act; ")
                     + (rateMatters ? "control: a 44.1 kHz preparation renders the same input differently"
                                    : "control: the rate does not change the render"));
        }

        // M5: the response of a Mid band and of a Side band, through the plugin.
        {
            constexpr int window = 4800;                    // 80 cycles of 800 Hz at 48 kHz
            const double rate = kRenderRate;
            const double tone = 800.0;
            const int settle = (int) std::lround (rate);
            const int total = settle + window;

            // L and R differ in level and phase, so the mid and the side both carry the tone.
            juce::AudioBuffer<float> input (2, total);

            for (int i = 0; i < total; ++i)
            {
                const double phase = 2.0 * eq::pi * tone * (double) i / rate;
                input.setSample (0, i, (float) (0.1 * std::sin (phase)));
                input.setSample (1, i, (float) (0.05 * std::cos (phase)));
            }

            const auto designer = designerFor (rate);
            const std::vector<double> at { tone };

            Tally tally;
            juce::String figures;

            for (const int placement : { kMid, kSide })
            {
                auto p = makeBands (2, 1, [placement] (int index) { return msBellSpec (index, 1, 30.0f, placement); });
                const auto output = render (*p, input, 512, false);

                const double bellGain = (double) rawOf (*p, bandId (1, "gain"));
                const double closedForm = chainRefDb (*p, *designer, rate, at)[0];

                const double midDb = eq::powerToDb (componentPower (output, 1.0, settle, window, tone, rate)
                                                        / componentPower (input, 1.0, settle, window, tone, rate));
                const double sideDb = eq::powerToDb (componentPower (output, -1.0, settle, window, tone, rate)
                                                         / componentPower (input, -1.0, settle, window, tone, rate));

                const double own = placement == kMid ? midDb : sideDb;
                const double other = placement == kMid ? sideDb : midDb;
                const juce::String name = kPlacementNames[placement];

                tally.check (std::abs (own - bellGain) <= 0.01,
                             name + " bell: its own component reads " + fmtG (own, 7) + " dB, bell gain " + fmtG (bellGain, 4));
                tally.check (std::abs (other) <= 0.01,
                             name + " bell: the other component reads " + fmtG (other, 4) + " dB");

                figures += (figures.isEmpty() ? "" : "; ") + name + " bell: own component " + fmtG (own, 7)
                         + " dB (bell gain " + fmtG (bellGain, 4) + ", closed form at 800 Hz " + fmtG (closedForm, 7)
                         + "), other component " + fmtG (other, 3) + " dB";
            }

            row (tally.bad == 0 && tally.total == 4,
                 "M5 T1c's single bell (800 Hz, +30 dB, Q 1) on Mid, then on Side: the gain of its own component at the centre equals the bell gain, the other component's is 0 dB, both within 0.01 dB",
                 tally.detail() + "  " + figures);
        }

        // M6, M6n: a continuous change keeps the state, a structural one does not.
        {
            const double rate = 48000.0;
            const int total = (int) std::lround (rate);
            const int editAt = kKeepEditBlock * kKeepBlock;         // 24000

            juce::AudioBuffer<float> burst (1, total);
            burst.clear();

            for (int i = 0; i < 480; ++i)
                burst.setSample (0, i, (float) (0.5 * std::sin (2.0 * eq::pi * 40.0 * (double) i / rate)));

            const auto unedited = keepRender (burst, rate, 0);
            const auto gainEdit = keepRender (burst, rate, 1);
            const auto typeEdit = keepRender (burst, rate, 2);

            const KeepFigures kept = keepFigures (gainEdit, editAt, rate);
            const KeepFigures cleared = keepFigures (typeEdit, editAt, rate);

            const int gainFirst = firstDifference (unedited, gainEdit);
            const int typeFirst = firstDifference (unedited, typeEdit);

            row (allFinite (gainEdit) && kept.ownD2 > 0.0 && gainFirst == editAt && kept.ratio() <= 10.0,
                 "M6 bell 40 Hz Q 20 +24 dB rung by a burst, gain -> +18 dB during the ring: |y[n] - (2 y[n-1] - y[n-2])| over the 20 ms after the edit <= 10x the ring's own largest second difference",
                 fmtG (kept.ratio(), 4) + "x (" + fmtG (kept.after, 4) + " against " + fmtG (kept.ownD2, 4) + "; ring peak "
                     + fmtG (kept.ringPeak, 4) + "); the edit first changes sample " + juce::String (gainFirst)
                     + " (seen at " + juce::String (editAt) + ")");

            row (allFinite (typeEdit) && cleared.ownD2 > 0.0 && typeFirst == editAt && cleared.ratio() >= 100.0,
                 "M6n a type change (Bell -> Low Shelf) at the same sample reads >= 100x on the same metric: a structural change is not kept (control fires)",
                 fmtG (cleared.ratio(), 4) + "x (" + fmtG (cleared.after, 4) + " against " + fmtG (cleared.ownD2, 4)
                     + "); the edit first changes sample " + juce::String (typeFirst));
        }
    }

    //==========================================================================
    // B1, TK. Block identity with moving parameters; the tick clock (PLAN-2.3 P7, P10)
    //==========================================================================
    struct BlockEvent
    {
        float freq[4];
        float gain[4];
        float q[4];
        float scale;
    };

    struct Xorshift
    {
        std::uint64_t state = 0x9E3779B97F4A7C15ull;

        double uniform()        // [0, 1)
        {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            return (double) (state >> 11) * (1.0 / 9007199254740992.0);
        }
    };

    // RM-blk's script (ramp_bench blk): frequency, gain and Q of four bands
    // redrawn over their full ranges at each of 96 events; plus gain_scale.
    std::vector<BlockEvent> makeBlockEvents()
    {
        Xorshift draw;
        draw.state = 777;

        std::vector<BlockEvent> events (96);

        for (auto& e : events)
        {
            for (int b = 0; b < 4; ++b)
            {
                e.freq[b] = juce::jlimit (10.0f, 30000.0f, (float) std::exp (std::log (10.0) + draw.uniform() * std::log (3000.0)));
                e.gain[b] = juce::jlimit (-30.0f, 30.0f, (float) (-30.0 + 60.0 * draw.uniform()));
                e.q[b] = juce::jlimit (0.025f, 40.0f, (float) std::exp (std::log (0.025) + draw.uniform() * std::log (1600.0)));
            }

            e.scale = juce::jlimit (0.0f, 200.0f, (float) (200.0 * draw.uniform()));
        }

        return events;
    }

    BandSpec blockIdentitySpec (int index)
    {
        const BandSpec table[4] =
        {
            makeSpec (kBell,      200.0f,   6.0f, 1.0f, 1, kStereo),
            makeSpec (kHighShelf, 4000.0f, -9.0f, 1.0f, 7, kStereo),    // 96 dB/oct
            makeSpec (kLowCut,    60.0f,    0.0f, 2.0f, 5, kStereo),    // 48 dB/oct
            makeSpec (kTilt,      900.0f,   4.0f, 1.0f, 3, kStereo)     // 24 dB/oct
        };

        return table[index % 4];
    }

    // events == nullptr: the same render with no event.
    juce::AudioBuffer<float> blockIdentityRender (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input,
                                                  double rate, int blockSize, const std::vector<BlockEvent>* events)
    {
        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, events] (BlockInfo& current)
                           {
                               // An event is seen by the block that starts on it.
                               if (events == nullptr || current.position % 2048 != 0)
                                   return;

                               const auto slot = (size_t) (current.position / 2048);

                               if (slot >= events->size())
                                   return;

                               const BlockEvent& e = (*events)[slot];
                               h.gainScale->store (e.scale);

                               for (size_t b = 0; b < 4; ++b)
                               {
                                   h.band[b].freq->store (e.freq[b]);
                                   h.band[b].gain->store (e.gain[b]);
                                   h.band[b].q->store (e.q[b]);
                               }
                           });
    }

    juce::AudioBuffer<float> tickRender (const juce::AudioBuffer<float>& input, int idleMode, int boundary, bool withEdit)
    {
        // A 0 dB bell, enabled. idleMode 0: global bypass on to sample 1000.
        // 1: the band disabled to sample 1000. 2: processBlockBypassed to sample 1000.
        auto p = makeBands (2, 1, [] (int) { return makeSpec (kBell, 1000.0f, 0.0f, 1.0f, 1, kStereo); });
        const Handles h = handlesOf (*p);

        if (idleMode == 0)
            h.bypass->store (1.0f);

        if (idleMode == 1)
            h.band[0].enabled->store (0.0f);

        return renderWith (*p, input, 48000.0, 1024,
                           [boundary] (int index, int)
                           {
                               return index == 0 ? kTickActiveFrom : (index == 1 ? boundary - kTickActiveFrom : 64);
                           },
                           [&h, idleMode, withEdit] (BlockInfo& current)
                           {
                               if (current.index == 0 && idleMode == 2)
                                   current.viaBypassed = true;

                               if (current.index == 1)
                               {
                                   if (idleMode == 0)
                                       h.bypass->store (0.0f);

                                   if (idleMode == 1)
                                       h.band[0].enabled->store (1.0f);
                               }

                               if (current.index == 2 && withEdit)
                                   h.band[0].gain->store (24.0f);
                           });
    }

    void blockAndTickRows()
    {
        section ("B1 / TK. block identity with moving parameters; the tick clock (Phase 2.3: PLAN-2.3 P7, P10)");

        // B1 (PLAN-2.3; not the bus-layout rows of Stage 1)
        {
            constexpr int length = 2048 * 96;
            const double rates[4] = { 44100.0, 48000.0, 96000.0, 192000.0 };
            const int sizes[4] = { 32, 64, 512, 2048 };
            const auto input = makeNoiseAt (2, length, 0x0B1E0002, kNoise12);
            const auto events = makeBlockEvents();

            struct Job { double rate; int blockSize; bool withEvents; };
            std::vector<Job> jobs;

            for (const double rate : rates)
            {
                for (const int blockSize : sizes)
                    jobs.push_back ({ rate, blockSize, true });

                jobs.push_back ({ rate, 512, false });
            }

            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (size_t j = 0; j < jobs.size(); ++j)
            {
                instances.push_back (makeBands (2, 4, blockIdentitySpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<juce::AudioBuffer<float>> outputs (jobs.size());

            parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &outputs, &input, &events] (int j)
            {
                const auto slot = (size_t) j;
                outputs[slot] = blockIdentityRender (*instances[slot], handles[slot], input, jobs[slot].rate,
                                                     jobs[slot].blockSize, jobs[slot].withEvents ? &events : nullptr);
            });

            Tally tally;

            for (size_t r = 0; r < 4; ++r)
            {
                const size_t base = r * 5;
                const juce::String where = fmtG (rates[r] / 1000.0, 4) + " kHz";

                tally.check (allFinite (outputs[base]) && ! sameAudio (outputs[base], input), where + ": not finite, or the bands do not act");

                for (size_t k = 1; k < 4; ++k)
                    tally.check (sameAudio (outputs[base + k], outputs[base]),
                                 where + ": block " + juce::String (sizes[k]) + " differs from block 32, first at sample "
                                     + juce::String (firstDifference (outputs[base + k], outputs[base])));

                // The render without events is compared at its own block size (512 is index 2).
                tally.check (! sameAudio (outputs[base + 4], outputs[base + 2]), where + ": the render without events is the same (the events do not bite)");
            }

            row (tally.bad == 0 && tally.total == 20,
                 "B1 four bands, 96 events on multiples of 2048 (f, gain, Q, gain_scale redrawn): blocks 32 / 64 / 512 / 2048 bit-identical at 44.1 / 48 / 96 / 192 kHz; differs from the render without events",
                 tally.detail());
        }

        // TK (PLAN-2.3 R4: the only row that sees a clock that stops on an early return)
        {
            const auto input = makeNoiseAt (2, kTickLength, 0x71C40002, kNoise12);
            const char* const idleNames[3] = { "(a) bypass on", "(b) no active band", "(c) processBlockBypassed" };

            Tally tally;
            juce::String figures;

            for (const auto& c : kTickCases)
            {
                figures += (figures.isEmpty() ? "" : "; ") + juce::String ("edit at ") + juce::String (c.boundary) + ":";

                for (int idleMode = 0; idleMode < 3; ++idleMode)
                {
                    const auto plain = tickRender (input, idleMode, c.boundary, false);
                    const auto edited = tickRender (input, idleMode, c.boundary, true);

                    const bool idleUntouched = sameFrom (plain, 0, input, 0, kTickActiveFrom)
                                            && sameFrom (edited, 0, input, 0, kTickActiveFrom);
                    const int first = firstDifference (plain, edited);

                    tally.check (idleUntouched && allFinite (edited) && first == c.expected,
                                 juce::String (idleNames[idleMode]) + ", boundary and edit at " + juce::String (c.boundary)
                                     + ": first differing sample " + juce::String (first) + ", expected " + juce::String (c.expected)
                                     + (idleUntouched ? "" : " (the idle block is not the input)"));

                    figures += " " + juce::String (first);
                }

                figures += " (expected " + juce::String (c.expected) + ")";
            }

            row (tally.bad == 0 && tally.total == 9,
                 "TK 1000 samples idle by (a) bypass on, (b) no active band, (c) processBlockBypassed, then a bell and a 0 -> +24 dB edit, 48 kHz: first sample that differs from the unedited render is 1040 (edit at 1036), 1056 (at 1041), 1024 (at 1024)",
                 tally.detail() + "  " + figures);
        }
    }

    //==========================================================================
    // S5, S6, A2. Random stress (G5, PLAN-2.3 P13; ported from ramp_bench g5),
    // non-finite input (P9), allocation under stress (G6)
    //==========================================================================
    constexpr int kStressPrepared = 4096;
    constexpr double kStressStackLimitDb = 600.0;

    struct StressBand
    {
        float type = 0.0f;
        float slope = 1.0f;
        float placement = 0.0f;
        float enabled = 1.0f;
        float bypass = 0.0f;
        float freq = 1000.0f;
        float gain = 0.0f;
        float q = 1.0f;
    };

    /** The seeded draw. Every host block: gain_scale, and the frequency, gain
        and Q of all 24 bands, 10 % of the draws on each range end. Every 50th
        block (the first included): type, slope, placement, enable and band
        bypass as well. The audio has its own stream, so the parameter draw
        can be replayed without it (the stack guard does). */
    struct StressDraw
    {
        Xorshift random;
        float scale = 100.0f;
        StressBand bands[Processor::kNumBands];

        void nextBlock (bool restructure)
        {
            const double su = random.uniform();
            scale = (float) (su < 0.1 ? 0.0 : (su > 0.9 ? 200.0 : 200.0 * random.uniform()));

            for (auto& s : bands)
            {
                if (restructure)
                {
                    s.type = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
                    s.slope = (float) juce::jmin (7, (int) (random.uniform() * 8.0));
                }

                const double u = random.uniform();
                const double f = u < 0.1 ? 10.0
                                         : (u > 0.9 ? 30000.0 : std::exp (std::log (10.0) + random.uniform() * std::log (3000.0)));
                const double w = random.uniform();
                const double g = w < 0.1 ? -30.0 : (w > 0.9 ? 30.0 : -30.0 + 60.0 * random.uniform());
                const double z = random.uniform();
                const double sharpness = z < 0.1 ? 0.025
                                                 : (z > 0.9 ? 40.0 : std::exp (std::log (0.025) + random.uniform() * std::log (1600.0)));

                s.freq = juce::jlimit (10.0f, 30000.0f, (float) f);
                s.gain = juce::jlimit (-30.0f, 30.0f, (float) g);
                s.q = juce::jlimit (0.025f, 40.0f, (float) sharpness);

                if (restructure)
                {
                    s.enabled = random.uniform() > 0.15 ? 1.0f : 0.0f;
                    s.placement = (float) juce::jmin (4, (int) (random.uniform() * 5.0));
                    s.bypass = random.uniform() < 0.10 ? 1.0f : 0.0f;
                }
            }
        }

        eq::RawBand rawBand (int b) const
        {
            const StressBand& s = bands[b];

            eq::RawBand raw;
            raw.enabled = s.enabled;
            raw.bypass = s.bypass;
            raw.type = s.type;
            raw.freq = s.freq;
            raw.gain = s.gain;
            raw.q = s.q;
            raw.slope = s.slope;
            raw.placement = s.placement;
            return raw;
        }
    };

    // Block sizes 4096, 512, 64, 7, 1, cycling every 2 s.
    int stressBlockSize (std::int64_t position, double rate)
    {
        static const int sizes[5] = { 4096, 512, 64, 7, 1 };
        return sizes[(size_t) ((double) position / rate / 2.0) % 5];
    }

    // The static stack of the current draw: the sum over active bands of the
    // peak of eq::bandMagDb of the band's target design, floor 0 dB per band
    // (64 log-spaced points from 5 Hz to 0.4999 fs, plus the band's own frequency).
    double stressStackDb (const StressDraw& draw, const eq::Designer& designer, double rate)
    {
        const double top = 0.4999 * rate;
        double stack = 0.0;

        for (int b = 0; b < Processor::kNumBands; ++b)
        {
            const eq::BandInput in = eq::makeBandInput (draw.rawBand (b), draw.scale, 0.0);

            if (! in.active)
                continue;

            eq::BandDesign design {};
            designer.design (in.settings, design);

            double peak = 0.0;

            for (int i = 0; i < 64; ++i)
            {
                const double hz = 5.0 * std::pow (top / 5.0, (double) i / 63.0);
                peak = juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * hz / rate));
            }

            peak = juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * juce::jmin (in.settings.freqHz, top) / rate));
            stack += peak;
        }

        return stack;
    }

    // The largest static stack of the whole draw, taken at the blocks of 512
    // and 4096 samples: the parameter stream replayed without audio.
    double stressLargestStackDb (double rate, double seconds, std::uint64_t seed)
    {
        const auto designer = designerFor (rate);
        const auto total = (std::int64_t) std::llround (seconds * rate);

        StressDraw draw;
        draw.random.state = seed;

        double largest = 0.0;
        std::int64_t position = 0, blocks = 0;

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);

            draw.nextBlock ((blocks % 50) == 0);

            if (size >= 512)
                largest = juce::jmax (largest, stressStackDb (draw, *designer, rate));

            position += m;
            ++blocks;
        }

        return largest;
    }

    struct StressSeed
    {
        bool ok = false;                // a draw below the limit was found
        int attempt = 0;                // 0 is the first seed; above 0 the seed was changed
        std::uint64_t seed = 0;
        double largestStackDb = 0.0;    // of the draw in use
        double rejectedStackDb = 0.0;   // the largest stack among the rejected draws
    };

    /** PLAN-2.3 P13: the float output overflows near +770 dB, so the largest
        static stack of the whole draw must stay below +600 dB. A draw that
        does not is a stimulus failure: the seed is changed (attempt + 1),
        and the change is recorded in the row's output. Nothing else is
        changed for it. */
    StressSeed chooseStressSeed (double rate, double seconds)
    {
        StressSeed chosen;

        for (int attempt = 0; attempt < 32; ++attempt)
        {
            std::uint64_t seed = (0x9E3779B97F4A7C15ull ^ (std::uint64_t) rate)
                               + (std::uint64_t) attempt * 0xD1B54A32D192ED03ull;

            if (seed == 0)
                seed = 1;

            const double stack = stressLargestStackDb (rate, seconds, seed);

            chosen.attempt = attempt;
            chosen.seed = seed;
            chosen.largestStackDb = stack;

            if (stack < kStressStackLimitDb)
            {
                chosen.ok = true;
                return chosen;
            }

            chosen.rejectedStackDb = juce::jmax (chosen.rejectedStackDb, stack);
        }

        return chosen;
    }

    void writeStress (const Handles& h, const StressDraw& draw, bool structureToo)
    {
        h.gainScale->store (draw.scale);

        for (size_t b = 0; b < h.band.size(); ++b)
        {
            const StressBand& s = draw.bands[b];
            const BandHandles& hb = h.band[b];

            hb.freq->store (s.freq);
            hb.gain->store (s.gain);
            hb.q->store (s.q);

            if (structureToo)
            {
                hb.type->store (s.type);
                hb.slope->store (s.slope);
                hb.placement->store (s.placement);
                hb.enabled->store (s.enabled);
                hb.bypass->store (s.bypass);
            }
        }
    }

    struct StressOutcome
    {
        std::int64_t nonFinite = 0;     // output samples
        std::int64_t blocks = 0;
        std::int64_t actingBlocks = 0;  // blocks whose output differs from their input
        double peak = 0.0;              // of the finite output samples
        double inputPeak = 0.0;
    };

    /** The stress render: stereo, prepared at 4096, noise at -12 dBFS with one
        second of silence in five. Parameters are stored before each block;
        `process (buffer, midi)` runs the block (A2 arms its window there). */
    template <typename ProcessFn>
    StressOutcome stressLoop (Processor& p, const Handles& h, double rate, double seconds, std::uint64_t seed,
                              ProcessFn&& process)
    {
        StressOutcome out;

        StressDraw draw;
        draw.random.state = seed;

        Xorshift audio;
        audio.state = seed ^ 0xA5A5A5A55A5A5A5Aull;

        if (audio.state == 0)
            audio.state = 1;

        const auto total = (std::int64_t) std::llround (seconds * rate);

        juce::AudioBuffer<float> block (2, kStressPrepared);
        std::vector<float> keptLeft ((size_t) kStressPrepared), keptRight ((size_t) kStressPrepared);
        juce::MidiBuffer midi;

        h.bypass->store (0.0f);
        p.prepareToPlay (rate, kStressPrepared);

        std::int64_t position = 0;

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);
            const bool silent = std::fmod ((double) position / rate, 5.0) > 4.0;
            const bool restructure = (out.blocks % 50) == 0;

            draw.nextBlock (restructure);
            writeStress (h, draw, restructure);

            block.setSize (2, m, false, false, true);
            float* const left = block.getWritePointer (0);
            float* const right = block.getWritePointer (1);

            for (int i = 0; i < m; ++i)
            {
                const float l = silent ? 0.0f : (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);
                const float r = silent ? 0.0f : (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);

                left[i] = l;
                right[i] = r;
                keptLeft[(size_t) i] = l;
                keptRight[(size_t) i] = r;
                out.inputPeak = juce::jmax (out.inputPeak, (double) std::abs (l), (double) std::abs (r));
            }

            process (block, midi);

            const float* const outLeft = block.getReadPointer (0);
            const float* const outRight = block.getReadPointer (1);
            bool acts = false;

            for (int i = 0; i < m; ++i)
            {
                const float l = outLeft[i];
                const float r = outRight[i];

                acts = acts || ! sameBits (l, keptLeft[(size_t) i]) || ! sameBits (r, keptRight[(size_t) i]);

                if (! std::isfinite (l) || ! std::isfinite (r))
                {
                    ++out.nonFinite;
                    continue;
                }

                out.peak = juce::jmax (out.peak, (double) std::abs (l), (double) std::abs (r));
            }

            if (acts)
                ++out.actingBlocks;

            position += m;
            ++out.blocks;
        }

        return out;
    }

    struct StressResult
    {
        StressSeed chosen;
        StressOutcome outcome;
        bool ran = false;
        bool offFinite = true;
        double offEqualFromMs = 0.0;    // after "all bands off": the last block that still differed from its input ends here
    };

    StressResult runStress (Processor& p, const Handles& h, double rate, double seconds)
    {
        StressResult result;
        result.chosen = chooseStressSeed (rate, seconds);

        if (! result.chosen.ok)
            return result;

        result.outcome = stressLoop (p, h, rate, seconds, result.chosen.seed,
                                     [&p] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
                                     {
                                         p.processBlock (buffer, midiMessages);
                                     });
        result.ran = true;

        // Every band off: the output must be the input again within 100 ms.
        for (const auto& hb : h.band)
            hb.enabled->store (0.0f);

        constexpr int blockSize = 512;
        const int numBlocks = (int) std::ceil (0.5 * rate / (double) blockSize);

        Xorshift audio;
        audio.state = result.chosen.seed ^ 0x0FF0FF0FF0FF0FF0ull;

        if (audio.state == 0)
            audio.state = 1;

        juce::AudioBuffer<float> block (2, blockSize), kept (2, blockSize);
        juce::MidiBuffer midi;
        int lastUnequalEnd = 0;

        for (int k = 0; k < numBlocks; ++k)
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                float* const data = block.getWritePointer (channel);

                for (int i = 0; i < blockSize; ++i)
                    data[i] = (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);

                kept.copyFrom (channel, 0, block, channel, 0, blockSize);
            }

            p.processBlock (block, midi);

            if (! allFinite (block))
                result.offFinite = false;

            if (! sameAudio (block, kept))
                lastUnequalEnd = (k + 1) * blockSize;
        }

        result.offEqualFromMs = 1000.0 * (double) lastUnequalEnd / rate;
        p.releaseResources();
        return result;
    }

    void stressRows()
    {
        section ("S5 / S6. random stress and non-finite input through processBlock (Phase 2.3: G5; PLAN-2.3 P9, P13)");

        // S5 (PLAN-2.3; not the state-codec row of Stage 1)
        {
            const double seconds = longRunRequested ? 600.0 : 60.0;
            const double rates[8] = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };

            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (int i = 0; i < 8; ++i)
            {
                instances.push_back (makeBands (2, 0, mixedSpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<StressResult> results (8);

            parallelFor (8, [&instances, &handles, &results, &rates, seconds] (int j)
            {
                const auto slot = (size_t) j;
                results[slot] = runStress (*instances[slot], handles[slot], rates[slot], seconds);
            });

            Tally tally;
            int reseeded = 0;
            double largestPeakDb = -std::numeric_limits<double>::infinity();
            double largestStackDb = 0.0;

            for (size_t r = 0; r < 8; ++r)
            {
                const StressResult& s = results[r];
                const juce::String where = fmtG (rates[r] / 1000.0, 5) + " kHz";

                if (s.chosen.attempt > 0 || ! s.chosen.ok)
                {
                    ++reseeded;
                    info ("S5 STIMULUS at " + where + ": the draw of an earlier seed reached a static stack of "
                              + fmtG (s.chosen.rejectedStackDb, 5) + " dB (limit +600); "
                              + (s.chosen.ok ? "seed changed to attempt " + juce::String (s.chosen.attempt) + " (0x"
                                                   + juce::String::toHexString ((juce::int64) s.chosen.seed) + "), nothing else changed"
                                             : juce::String ("no seed in 32 attempts stays below it")));
                }

                if (! s.chosen.ok || ! s.ran)
                {
                    tally.check (false, where + ": STIMULUS FAILURE, the stack guard does not hold (largest static stack "
                                            + fmtG (s.chosen.largestStackDb, 5) + " dB)");
                    continue;
                }

                const double peakDb = 20.0 * std::log10 (juce::jmax (s.outcome.peak, 1.0e-300) / juce::jmax (s.outcome.inputPeak, 1.0e-300));
                largestPeakDb = juce::jmax (largestPeakDb, peakDb);
                largestStackDb = juce::jmax (largestStackDb, s.chosen.largestStackDb);

                std::printf ("      S5 %-9s %g s, seed attempt %d: largest static stack %.1f dB (guard: below +600); %lld non-finite samples in %lld blocks (%lld acting); peak %+.1f dB re input peak; all bands off: memcmp-equal to input from %.1f ms\n",
                             where.toRawUTF8(), seconds, s.chosen.attempt, s.chosen.largestStackDb,
                             (long long) s.outcome.nonFinite, (long long) s.outcome.blocks, (long long) s.outcome.actingBlocks,
                             peakDb, s.offEqualFromMs);

                tally.check (s.outcome.nonFinite == 0 && s.offFinite && s.outcome.actingBlocks > 0 && s.offEqualFromMs <= 100.0,
                             where + ": " + juce::String ((juce::int64) s.outcome.nonFinite) + " non-finite samples"
                                 + (s.outcome.actingBlocks > 0 ? "" : "; the bands never act")
                                 + "; bands off, equal to input from " + fmtG (s.offEqualFromMs, 4) + " ms");
            }

            row (tally.bad == 0 && tally.total == 8,
                 "S5 G5 through processBlock: 24 bands, every parameter redrawn (P13), blocks 4096 / 512 / 64 / 7 / 1, 8 rates: stack guard holds; every sample finite; all bands off -> memcmp-equal to input within 100 ms",
                 tally.detail() + "  " + fmtG (seconds, 4) + " s per rate" + (longRunRequested ? " (--long)" : "")
                     + "; largest peak " + fmtG (largestPeakDb, 4) + " dB re input peak; largest static stack "
                     + fmtG (largestStackDb, 4) + " dB; seed changed at " + juce::String (reseeded) + " of 8 rates");
        }

        // S6 (PLAN-2.3; not the state-codec row of Stage 1)
        {
            constexpr int blockSize = 512, badBlock = 10, window = 48000;
            const double rate = kRenderRate;
            const int total = 3 * 48000;
            const int badAt = badBlock * blockSize + 100;
            const int nextBlock = (badBlock + 1) * blockSize;
            const int steadyFrom = 2 * 48000;
            const double bins[3] = { 100.0, 1000.0, 5000.0 };

            const auto clean = makeNoiseAt (2, total, 0x56A00002, kNoise12);

            auto fresh = makeBands (2, 6, mixedSpec);
            const auto freshOut = render (*fresh, clean, blockSize, false);

            Tally tally;
            juce::String figures;

            struct Bad { const char* name; float value; };
            const Bad bads[2] = { { "NaN", std::numeric_limits<float>::quiet_NaN() },
                                  { "+inf", std::numeric_limits<float>::infinity() } };

            for (const auto& bad : bads)
            {
                juce::AudioBuffer<float> poisoned (clean);
                poisoned.setSample (0, badAt, bad.value);

                auto p = makeBands (2, 6, mixedSpec);
                const auto output = render (*p, poisoned, blockSize, false);

                // The stimulus reached the engine: nothing is replaced, so its block is not finite.
                const bool reached = ! allFinite (sliceOf (output, badBlock * blockSize, blockSize));
                const bool finiteAfter = allFinite (sliceOf (output, nextBlock, total - nextBlock));

                double worstDb = 0.0;
                bool binsFinite = true;

                for (int channel = 0; channel < 2; ++channel)
                    for (const double hz : bins)
                    {
                        const double db = eq::powerToDb (binPower (output, channel, steadyFrom, window, hz, rate)
                                                             / binPower (freshOut, channel, steadyFrom, window, hz, rate));

                        // A NaN must not be lost in a max.
                        if (! std::isfinite (db))
                            binsFinite = false;
                        else
                            worstDb = juce::jmax (worstDb, std::abs (db));
                    }

                tally.check (reached, juce::String (bad.name) + ": the block holding it is finite (the stimulus did not reach the engine)");
                tally.check (finiteAfter, juce::String (bad.name) + ": output not finite from the next block on");
                tally.check (binsFinite && worstDb <= 0.01,
                             juce::String (bad.name) + ": from 2 s on " + fmtG (worstDb, 3) + " dB from a fresh instance"
                                 + (binsFinite ? "" : " (a bin is not finite)"));

                figures += (figures.isEmpty() ? "" : "; ") + juce::String (bad.name) + ": from 2 s on within "
                         + fmtG (worstDb, 3) + " dB of a fresh instance (6 bins)";
            }

            tally.check (activeBands (*fresh) == 6 && ! sameAudio (freshOut, clean), "the 6 bands do not act");

            row (tally.bad == 0 && tally.total == 7,
                 "S6 one block holding a NaN sample, then noise; the same with +inf; 6 mixed bands: output finite from the next block on; from 2 s on within 0.01 dB (single bin) of a fresh instance",
                 tally.detail() + "  " + figures);
        }
    }

    AllocResult runStressAllocWindow (bool plant)
    {
        AllocResult result;

       #if defined (__APPLE__)
        const double rate = 48000.0;
        const double seconds = 20.0;

        const StressSeed chosen = chooseStressSeed (rate, seconds);
        auto p = makeBands (2, 0, mixedSpec);
        const Handles h = handlesOf (*p);

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

        // The draw, the parameter stores, the fill and the scan are outside
        // the window (stressLoop); the window is processBlock alone, the
        // first block after prepareToPlay included.
        const StressOutcome outcome = stressLoop (*p, h, rate, seconds, chosen.seed,
                                                  [&p, &plantNext] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
                                                  {
                                                      allocArmed = true;
                                                      p->processBlock (buffer, midiMessages);

                                                      if (plantNext)
                                                      {
                                                          void* volatile planted = std::malloc (64);
                                                          std::free (planted);
                                                      }

                                                      allocArmed = false;
                                                      plantNext = false;
                                                  });

        malloc_logger = previous;
        p->releaseResources();

        result.available = true;
        result.allocs = allocCount;
        result.frees = freeCount;
        result.blocks = (int) outcome.blocks;
        result.audioOk = chosen.ok && outcome.nonFinite == 0 && outcome.actingBlocks > 0;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void stressAllocationRows()
    {
        section ("A2. allocation under stress (Phase 2.3: G6, PERF-01)");

        const AllocResult clean = runStressAllocWindow (false);

        if (! clean.available)
        {
            info ("A2 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A2n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = clean.probeAllocs >= 1 && clean.probeFrees >= 1;

        row (hookLive && clean.audioOk && clean.allocs == 0 && clean.frees == 0,
             "A2 processBlock under stress, 20 s of the S5 draw at 48 kHz, first block included, parameter writes outside the window: 0 allocations, 0 frees",
             juce::String (clean.allocs) + " allocations, " + juce::String (clean.frees) + " frees in "
                 + juce::String (clean.blocks) + " windows (malloc_logger, this thread); hook "
                 + (hookLive ? "live" : "DEAD") + " (probe: " + juce::String (clean.probeAllocs) + " / "
                 + juce::String (clean.probeFrees) + ")"
                 + (clean.audioOk ? "" : "; the stress did not run as expected (stack guard, non-finite output, or no acting block)"));

        const AllocResult planted = runStressAllocWindow (true);

        row (planted.allocs >= 1 && planted.frees >= 1,
             "A2n a planted allocation inside the processBlock window is counted (control fires)",
             juce::String (planted.allocs) + " allocations, " + juce::String (planted.frees) + " frees");
    }

    //==========================================================================
    // N7. Noise floor and silence (G7 float, PLAN-2.3 P14; ported from ramp_bench g7)
    //==========================================================================
    struct ToneResidual
    {
        double residualDb = 0.0;    // RMS of what is left after the tone is removed, dBFS
        double amplitude = 0.0;     // of the fitted tone
    };

    // Least-squares removal of the hz component over [start, end).
    ToneResidual toneResidual (const float* y, int start, int end, double hz, double rate)
    {
        long double ss = 0.0L, cc = 0.0L, sc = 0.0L, ys = 0.0L, yc = 0.0L;

        for (int i = start; i < end; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) i / rate;
            const double sn = std::sin (phase), cs = std::cos (phase);
            const double v = (double) y[i];

            ss += (long double) (sn * sn);
            cc += (long double) (cs * cs);
            sc += (long double) (sn * cs);
            ys += (long double) (v * sn);
            yc += (long double) (v * cs);
        }

        const long double det = ss * cc - sc * sc;
        const long double a = (ys * cc - yc * sc) / det;
        const long double b = (yc * ss - ys * sc) / det;
        long double r2 = 0.0L;

        for (int i = start; i < end; ++i)
        {
            const double phase = 2.0 * eq::pi * hz * (double) i / rate;
            const long double e = (long double) y[i] - a * (long double) std::sin (phase) - b * (long double) std::cos (phase);
            r2 += e * e;
        }

        ToneResidual result;
        result.residualDb = 10.0 * std::log10 ((double) (r2 / (long double) (end - start)) + 1.0e-300);
        result.amplitude = std::sqrt ((double) (a * a + b * b));
        return result;
    }

    double medianOf (std::vector<double>& values)
    {
        if (values.empty())
            return 0.0;

        const auto middle = values.begin() + (std::ptrdiff_t) (values.size() / 2);
        std::nth_element (values.begin(), middle, values.end());
        return *middle;
    }

    struct SilenceCost
    {
        double noiseMedian = 0.0;       // seconds per block
        double silenceMedian = 0.0;
        bool finite = true;

        double ratio() const    { return ratioOf (silenceMedian, noiseMedian); }
    };

    // 24 bells at 10 - 33 Hz, Q 40, +12 dB, 192 kHz stereo, blocks of 512:
    // 10 s of noise (median block time over its last 5 s), then 60 s of
    // silence (median over its last 5 s). Only processBlock is timed.
    SilenceCost silenceCostTrial()
    {
        constexpr int blockSize = 512;
        const double rate = 192000.0;
        const int blocksPerSecond = (int) (rate / (double) blockSize);     // 375
        const int noiseBlocks = 10 * blocksPerSecond;
        const int silenceBlocks = 60 * blocksPerSecond;
        const int scored = 5 * blocksPerSecond;

        auto p = makeBands (2, 24, [] (int index) { return makeSpec (kBell, 10.0f + (float) index, 12.0f, 40.0f, 1, kStereo); });
        const auto noise = makeNoiseAt (2, blockSize, 0x0E7000002, kNoise12);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        std::vector<double> noiseTimes, silenceTimes;
        noiseTimes.reserve ((size_t) scored);
        silenceTimes.reserve ((size_t) scored);

        SilenceCost cost;
        p->prepareToPlay (rate, blockSize);

        for (int k = 0; k < noiseBlocks + silenceBlocks; ++k)
        {
            const bool withNoise = k < noiseBlocks;

            if (withNoise)
            {
                for (int channel = 0; channel < 2; ++channel)
                    block.copyFrom (channel, 0, noise, channel, 0, blockSize);
            }
            else
            {
                block.clear();
            }

            const auto t0 = std::chrono::steady_clock::now();
            p->processBlock (block, midi);
            const auto t1 = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double> (t1 - t0).count();

            if (withNoise && k >= noiseBlocks - scored)
                noiseTimes.push_back (elapsed);

            if (! withNoise && k >= noiseBlocks + silenceBlocks - scored)
                silenceTimes.push_back (elapsed);

            if (k == noiseBlocks - 1 || k == noiseBlocks + silenceBlocks - 1)
                cost.finite = cost.finite && allFinite (block);
        }

        p->releaseResources();

        cost.noiseMedian = medianOf (noiseTimes);
        cost.silenceMedian = medianOf (silenceTimes);
        return cost;
    }

    void noiseFloorRows()
    {
        section ("N7. noise floor and silence (Phase 2.3: G7 float, DSP-05)");

        // (i) 192 kHz, one bell at 10 Hz, Q 40, +12 dB, static; a 1 kHz sine at
        // -6 dBFS with a 2 s raised-cosine fade-in, 7 s; the 1 kHz component
        // removed from the last 3 s by least squares.
        {
            const double rate = 192000.0;
            const int length = (int) std::lround (7.0 * rate);
            const int start = (int) std::lround (4.0 * rate);

            juce::AudioBuffer<float> input (1, length);

            {
                float* x = input.getWritePointer (0);

                for (int i = 0; i < length; ++i)
                {
                    const double t = (double) i / rate;
                    const double fade = t < 2.0 ? 0.5 - 0.5 * std::cos (eq::pi * t / 2.0) : 1.0;
                    x[i] = (float) (0.5 * fade * std::sin (2.0 * eq::pi * 1000.0 * t));
                }
            }

            auto p = makeBands (1, 1, [] (int) { return makeSpec (kBell, 10.0f, 12.0f, 40.0f, 1, kStereo); });
            const auto output = render (*p, input, 512, false, rate);

            const ToneResidual out = toneResidual (output.getReadPointer (0), start, length, 1000.0, rate);
            const ToneResidual in = toneResidual (input.getReadPointer (0), start, length, 1000.0, rate);

            const bool stimulusOk = activeBands (*p) == 1 && allFinite (output) && ! sameAudio (output, input)
                                 && std::abs (out.amplitude - 0.5) < 0.005;

            row (stimulusOk && out.residualDb <= -140.0,
                 "N7 (i) bell 10 Hz Q 40 +12 dB at 192 kHz, 1 kHz sine at -6 dBFS: residual of the float output after removing 1 kHz, last 3 s of 7, <= -140 dBFS RMS",
                 fmtG (out.residualDb, 5) + " dBFS RMS (fitted amplitude " + fmtG (out.amplitude, 6) + "); the float input's own residual is "
                     + fmtG (in.residualDb, 5) + " dBFS" + (stimulusOk ? "" : "; stimulus guard failed"));
        }

        // (ii) wall clock (PLAN-2.3 R6): medians, three trials, the best ratio.
        {
            double best = std::numeric_limits<double>::infinity();
            bool finite = true;
            juce::String trials;

            for (int trial = 0; trial < 3; ++trial)
            {
                const SilenceCost cost = silenceCostTrial();
                best = juce::jmin (best, cost.ratio());
                finite = finite && cost.finite;

                trials += (trials.isEmpty() ? "" : "; ") + fmtG (cost.ratio(), 4) + "x (silence "
                        + fmtG (cost.silenceMedian * 1.0e6, 4) + " us, noise " + fmtG (cost.noiseMedian * 1.0e6, 4) + " us per block)";
            }

            row (finite && best <= 1.2,
                 "N7 (ii) 24 bells 10 - 33 Hz Q 40, 192 kHz stereo: median block time over the last 5 s of a 60 s silence <= 1.2x the median with noise (best of three)",
                 "best " + fmtG (best, 4) + "x; trials: " + trials + (finite ? "" : "; OUTPUT NOT FINITE"));
        }
    }

    //==========================================================================
    // X2. CPU, sweeping (INFO; PLAN-2.3 P17; ported from ramp_bench cpu)
    //==========================================================================
    void cpuInfoSweeping()
    {
        section ("X2. CPU, sweeping (INFO, not gated; G10 binds in Stage 4)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9300002, kNoise12);

        struct Config { const char* name; int type; int slope; float gain; const char* benchSweeping; const char* benchSettled; };

        // Settings and motion as research-2.3 ramp_bench modeCpu (RESEARCH-2.3 Q5).
        const Config configs[] =
        {
            { "24 cuts x 96 dB/oct (PERF-02)", kHighCut,   7, 0.0f, "4.35", "2.48" },
            { "24 shelves x 96 dB/oct",        kHighShelf, 7, 9.0f, "23.0", "5.10" },
            { "24 bells",                      kBell,      1, 9.0f, "4.46", "0.71" }
        };

        for (const auto& c : configs)
        {
            double percent[2] = { 0.0, 0.0 };       // sweeping, settled
            bool finite = true;

            for (int mode = 0; mode < 2; ++mode)
            {
                const bool sweeping = mode == 0;

                auto p = makeBands (2, 24, [&c] (int index)
                {
                    return makeSpec (c.type, 40.0f * std::pow (300.0f, (float) index / 23.0f), c.gain, 1.0f, c.slope, kStereo);
                });

                const Handles h = handlesOf (*p);
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                double seconds = 0.0;

                p->prepareToPlay (rate, blockSize);

                for (int k = 0; k < numBlocks; ++k)
                {
                    if (sweeping)
                    {
                        // A 2 s triangle in log frequency, 40 Hz - 12 kHz, one phase per
                        // band; gain-bearing types move their gain with it. Written
                        // before every block, so every band redesigns at every tick.
                        const double position = (double) (k * blockSize) / rate;

                        for (int b = 0; b < Processor::kNumBands; ++b)
                        {
                            const double phase = std::fmod (position / 2.0 + (double) b / 24.0, 1.0);
                            const double triangle = phase < 0.5 ? 2.0 * phase : 2.0 - 2.0 * phase;
                            const BandHandles& hb = h.band[(size_t) b];

                            hb.freq->store ((float) (40.0 * std::pow (300.0, triangle)));

                            if (c.gain != 0.0f)
                                hb.gain->store ((float) ((double) c.gain * (0.5 + triangle)));
                        }
                    }

                    for (int channel = 0; channel < 2; ++channel)
                        block.copyFrom (channel, 0, noise, channel, 0, blockSize);

                    const auto t0 = std::chrono::steady_clock::now();
                    p->processBlock (block, midi);
                    const auto t1 = std::chrono::steady_clock::now();

                    seconds += std::chrono::duration<double> (t1 - t0).count();
                }

                p->releaseResources();

                percent[mode] = 100.0 * seconds / audioSeconds;
                finite = finite && allFinite (block);
            }

            info (juce::String ("X-cpu2 48 kHz stereo, 512-sample blocks, 20 s, ") + c.name + ": sweeping "
                      + fmtG (percent[0], 3) + " % of one core (bench " + c.benchSweeping + "; line 35), settled "
                      + fmtG (percent[1], 3) + " % (bench " + c.benchSettled + "; compare 2.2's X-cpu)"
                      + (finite ? "" : "  OUTPUT NOT FINITE"));
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
        else if (arg == "--long")
        {
            longRunRequested = true;
        }
        else
        {
            std::fprintf (stderr, "usage: %s [--dump-params] [--golden-dir <dir>] [--long]\n", argv[0]);
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

    // Stage 2, Phase 2.3 (PLAN-2.3 T7)
    sweepRows();
    carriedGapRows();
    blockAndTickRows();
    stressRows();
    stressAllocationRows();
    noiseFloorRows();
    cpuInfoSweeping();

    std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
    std::fflush (stdout);
    return juce::jmin (failures, 255);
}
