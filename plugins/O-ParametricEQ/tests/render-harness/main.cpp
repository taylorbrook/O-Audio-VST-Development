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
      M6n  retired in Phase 2.4 (a type change is a crossfade): see XR-rest, XR-g3h
      B1   events on multiples of 2048 (f, gain, Q, gain_scale): blocks 32 / 64 / 512 / 2048
           bit-identical at four rates; differs from the render without events
      TK   the tick clock runs through bypass, no-active-band and processBlockBypassed blocks
      S5   G5: seeded random stress at 8 rates: stack guard, finite, peak printed, bands off
      S6   a NaN / +inf input sample does not stick
      A2   G6 under stress: no allocation inside processBlock (malloc_logger, this thread)
      A2n  a planted allocation inside the window is counted
      N7   G7 float: noise floor; silence costs no more than signal
      X-cpu2  sweeping CPU readings, INFO

    Stage 2, Phase 2.4 (PLAN-2.4 T7): structural crossfades, the neutral
    sleep and the soft bypass through the plugin's float output.

      O-ParametricEQ-render-harness --long              S5 at 600 s per rate (default 60 s);
                                                        its every-block run is 10 s in both

    A step row is scored against the two held renders of the same instance
    (PLAN-2.4 P13): (a) the largest second difference, (b) local kappa at
    1 kHz, (c) the hard control. Two harness renders carry the rest:
    fadeRefRender (PLAN-2.4 P12: two static chains by Designer::design ->
    toRuntime -> svfTick / onePoleTick, the placement algebra and the weight
    formula re-stated in this file; none of the two-slot band, the fade
    table, the bypass fade, eq::Band or eq::Router) and hardStepRender
    (PLAN-2.4 P10: the Phase 2.3 rule on plain eq::Band + eq::Router). Both
    are fed the plugin's own float input, widened, and compared in double.
    Rows with many renders run on worker threads, one plugin instance per
    thread, constructed on the main thread; every job rewrites all the
    parameters it uses. N below is the fade length, round (0.020 fs).

      XR-g3   G3 step rows: 242 + 15 band-bypass rows x 50 Hz / 1 kHz x blocks 32 / 512:
              largest second difference <= 2.0x the louder held render
      XR-g3k  the same renders at 1 kHz: local kappa <= 2.5x held; 50 Hz printed
      XR-g3h  hardStepRender at block 512 reads >= 10x held; three void readings printed
      XR-g3b  the Global bypass group through processBlockBypassed
      XR-ref  30 first fades at 48 and 96 kHz against fadeRefRender, <= -120 dB re the peak
      XR-rest after a type change on the ringing bell the output from the fade's end equals
              a fresh instance started at the fade's first sample
      XR-g4d  G4 (d): 24 bells at 0 dB; 24 bells 6 dB -> 0, 200 ms later: memcmp-equal
      XR-g4e  G4 (e): gain_scale -> 0 %, 200 ms later: memcmp-equal
      XR-g4f  G4 (f): bypass on, by the parameter and by processBlockBypassed: equal from
              exactly N samples after the block that sees it
      XR-g4n  (d), (e) with a 0.001 dB gain; (f) read 5 ms after the toggle: must differ
      XR-g2   G2 live: each of the 8 slopes of both cuts reached from its neighbour
      XR-byp  a parameter changed while bypassed is in effect, settled, after un-bypass
      XR-bypn the same compare against the parameters from before the bypass must differ
      XR-big  host blocks of 4096 on a prepared size of 512 == prepared 4096
      XR-prep re-prepare with a fade and a ramp in flight, then moves in blocks of 500
      XR-blk  structural, continuous, gain_scale and bypass events on multiples of 2048:
              blocks 32 / 64 / 512 / 2048 bit-identical at four rates
      A3      G6 with fades: no allocation inside processBlock / processBlockBypassed
      A3n     a planted allocation inside the window is counted
      X-cpu3  CPU with fades, INFO

    Existing rows amended in Phase 2.4, by name (PLAN-2.4 P18; each carries a
    comment at its code): H1 (ii), M3 and U2 compare from the toggle + N;
    A1 checks the bypassed block after the fade has ended; S5 runs the P14
    draw (structure with probability 1/50 per band and block, global bypass
    1/200, plus a run that redraws the structure at every block); M6n is
    retired. TK, by Taylor's call in the execute session: the on-tick case
    (edit at 1024) first differs at 1026 (the 0 dB bell sleeps; its wake
    fade's first two weights round away in float).

    Step-row parameters: every step render runs gain_scale 200 % with the raw
    gain at half the bench's effective gain, so the effective gains are the
    bench's exactly (Tilt +-36 dB lies outside the raw +-30 dB range).

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
#include "dsp/Fade.h"

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

        // Amended in Phase 2.4 (PLAN-2.4 P18): global bypass is an N-sample
        // fade to the dry input, so "the bypassed block equals the input" is
        // checked on the first block after the fade has ended. The blocks of
        // the fade (and of the fade back) stay inside the allocation window.
        const int fadeBlocks = (eq::fadeLengthSamples (kRenderRate) + blockSize - 1) / blockSize;   // the toggle block included

        for (int i = 1; i < fadeBlocks; ++i)
            armedBlock();

        armedBlock();                                   // the first block after the fade has ended
        const bool bypassPasses = sameAudio (block, noise);
        change ("bypass", 0.0f);
        const bool resumed = ! sameAudio (block, noise);

        for (int i = 1; i < fadeBlocks; ++i)
            armedBlock();

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
            // Amended in Phase 2.4 (PLAN-2.4 P18): global bypass is an N-sample
            // fade to the dry input and un-bypass an N-sample fade back onto
            // bands started from rest. The bypassed span is the input from
            // the toggle + N, and the output equals a fresh instance started
            // at the un-bypass sample from un-bypass + N, bit for bit, by
            // both routes. N = eq::fadeLengthSamples (fs).
            const int fadeN = eq::fadeLengthSamples (kRenderRate);

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
                resumed.check (sameFrom (output, bypassAt + fadeN, input, bypassAt + fadeN, resumeAt - bypassAt - fadeN),
                               route + ": the bypassed span differs from the input from the toggle + N");
                resumed.check (sameFrom (output, resumeAt + fadeN, freshOut, fadeN, total - resumeAt - fadeN),
                               route + ": output from un-bypass + N differs from a fresh instance started at the un-bypass sample");
            }

            row (resumed.bad == 0 && resumed.total == 7,
                 "H1 (ii) bypassed: memcmp-equal to the input from the toggle + N; after bypass off: from un-bypass + N memcmp-equal to a fresh instance started at the un-bypass sample (both routes)",
                 resumed.detail() + "  N = " + juce::String (fadeN));
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
        // Amended in Phase 2.4 (PLAN-2.4 P18): a disable and an enable are
        // N-sample crossfades. The band stays disabled for at least N samples
        // (two blocks of 512 at 48 kHz); the disabled span is the input from
        // the disable + N; from the re-enable + N the output equals a fresh
        // instance started at the re-enable. N = eq::fadeLengthSamples (fs).
        {
            constexpr int blockSize = 512, offBlock = 10, offBlocks = 2;
            const int fadeN = eq::fadeLengthSamples (kRenderRate);
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x0E2B0002, kNoise12);
            const int total = input.getNumSamples();
            const int offAt = offBlock * blockSize;
            const int resumeAt = offAt + offBlocks * blockSize;

            const auto spec = makeSpec (kLowShelf, 300.0f, 9.0f, 1.0f, 3, kStereo);
            const auto one = [&spec] (int) { return spec; };

            auto p = makeBands (2, 1, one);

            const auto output = render (*p, input, blockSize, false, kRenderRate, [&] (BlockInfo& current)
            {
                if (current.index == offBlock)
                    setRaw (*p, bandId (1, "enabled"), 0.0f);

                if (current.index == offBlock + offBlocks)
                    setRaw (*p, bandId (1, "enabled"), 1.0f);
            });

            auto steady = makeBands (2, 1, one);
            const auto steadyOut = render (*steady, input, blockSize, false);

            auto fresh = makeBands (2, 1, one);
            const auto freshOut = render (*fresh, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

            const bool longEnough = resumeAt - offAt >= fadeN;
            const bool before = sameFrom (output, 0, steadyOut, 0, offAt);
            const bool during = longEnough && sameFrom (output, offAt + fadeN, input, offAt + fadeN, resumeAt - offAt - fadeN);
            const bool after = sameFrom (output, resumeAt + fadeN, freshOut, fadeN, total - resumeAt - fadeN);
            const bool stateMatters = ! sameFrom (steadyOut, resumeAt, freshOut, 0, total - resumeAt);

            row (before && during && after && stateMatters,
                 "U2 band disabled for at least N samples, then enabled: disabled span memcmp-equal to the input from the disable + N; from the re-enable + N memcmp-equal to a fresh instance started at the re-enable",
                 juce::String (before ? "" : "output before the edit differs from the steady render; ")
                     + (longEnough ? "" : "the band is disabled for fewer than N samples; ")
                     + (during ? "" : "the disabled span is not the input from the disable + N; ")
                     + (after ? "" : "output from the re-enable + N differs from a fresh instance; ")
                     + (stateMatters ? "" : "control: the steady render equals the fresh one; ")
                     + "N = " + juce::String (fadeN));
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

    // TK's expected first differing sample through the plugin, per kTickCases
    // entry. Amended in Phase 2.4 by Taylor's call (execute session
    // 2026-11-14): the 0 dB bell sleeps; the wake fade starts at 1024 (gated
    // exactly in double by XF-wake) but w(0), w(1) times one ramp step are
    // below float resolution, so the float output first differs at 1026.
    // Kept apart from kTickCases, whose `expected` M1's reference guard also
    // reads (the reference has no sleep and still acts at 1024).
    constexpr int kTickExpectedPlugin[3] = { 1040, 1056, 1026 };
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

            // Amended in Phase 2.4 (PLAN-2.4 P18): band bypass on and off are
            // N-sample crossfades. On: the output is the input from the toggle
            // + N; off: from the toggle + N it equals a fresh instance started
            // at the toggle. N = eq::fadeLengthSamples (fs).
            const int fadeN = eq::fadeLengthSamples (kRenderRate);

            Tally tally;
            tally.check (sameFrom (output, 0, steadyOut, 0, onAt), "output before the bypass differs from the steady render");
            tally.check (sameFrom (output, onAt + fadeN, input, onAt + fadeN, offAt - onAt - fadeN), "bypass on: the output is not the input from the toggle + N");
            tally.check (sameFrom (output, offAt + fadeN, freshOut, fadeN, total - offAt - fadeN), "bypass off: from the toggle + N the output differs from a fresh instance started at the toggle");
            tally.check (! sameFrom (steadyOut, onAt, input, onAt, offAt - onAt), "control: the band does not act on the audio");
            tally.check (! sameFrom (steadyOut, offAt, freshOut, 0, total - offAt), "control: an uninterrupted render equals a fresh one (state does not matter)");
            tally.check (activeBands (*p) == 1 && activeBands (*steady) == 1, "the band is not the only active one at the end");

            row (tally.bad == 0 && tally.total == 6,
                 "M3 bNN_bypass on the only active band, on at block 10, off at block 20: on, out memcmp-equal to in from the toggle + N; off, from the toggle + N memcmp-equal to a fresh instance started at the toggle",
                 tally.detail() + "  N = " + juce::String (fadeN));
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

            const KeepFigures kept = keepFigures (gainEdit, editAt, rate);

            const int gainFirst = firstDifference (unedited, gainEdit);

            row (allFinite (gainEdit) && kept.ownD2 > 0.0 && gainFirst == editAt && kept.ratio() <= 10.0,
                 "M6 bell 40 Hz Q 20 +24 dB rung by a burst, gain -> +18 dB during the ring: |y[n] - (2 y[n-1] - y[n-2])| over the 20 ms after the edit <= 10x the ring's own largest second difference",
                 fmtG (kept.ratio(), 4) + "x (" + fmtG (kept.after, 4) + " against " + fmtG (kept.ownD2, 4) + "; ring peak "
                     + fmtG (kept.ringPeak, 4) + "); the edit first changes sample " + juce::String (gainFirst)
                     + " (seen at " + juce::String (editAt) + ")");

            // M6n retired in Phase 2.4 (PLAN-2.4 P18): a type change is now a
            // 20 ms crossfade, so it no longer reads >= 100x. Replaced by
            // XR-rest (the incoming slot starts from rest at the fade's first
            // sample, on this same stimulus) and by the hard control XR-g3h.
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

            for (size_t tc = 0; tc < 3; ++tc)
            {
                // Amended in Phase 2.4 by Taylor's call (see kTickExpectedPlugin).
                const TickCase c { kTickCases[tc].boundary, kTickExpectedPlugin[tc] };

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
                 "TK 1000 samples idle by (a) bypass on, (b) no active band, (c) processBlockBypassed, then a bell and a 0 -> +24 dB edit, 48 kHz: first sample that differs from the unedited render is 1040 (edit at 1036), 1056 (at 1041), 1026 (at 1024; the 0 dB bell sleeps and its wake fade's first two weights round away in float)",
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

    // (Phase 2.4: the 2.3 S5 driver runStress was replaced by runStress24
    // below, PLAN-2.4 P18; the 2.3 draw and loop above stay for A2.)

    //==========================================================================
    // The P14 draw (PLAN-2.4 P14, C10; ported from fade_bench g5): S5's
    // generator, plus per band type / slope / placement / enable / band bypass
    // redrawn with probability 1 / structEvery per block (1: every block), and
    // the global bypass toggled with probability 1/200 per block. Used by the
    // amended S5 and by A3. The 2.3 draw above stays as it was (A2 runs it).
    //==========================================================================
    struct StressDraw24
    {
        Xorshift random;
        float scale = 100.0f;
        bool bypass = false;
        int toggles = 0;
        StressBand bands[Processor::kNumBands];

        void nextBlock (int structEvery)
        {
            const double su = random.uniform();
            scale = (float) (su < 0.1 ? 0.0 : (su > 0.9 ? 200.0 : 200.0 * random.uniform()));

            if (random.uniform() < 1.0 / 200.0)
            {
                bypass = ! bypass;
                ++toggles;
            }

            const double structProbability = 1.0 / (double) juce::jmax (1, structEvery);

            for (auto& s : bands)
            {
                const bool restructure = structEvery <= 1 || random.uniform() < structProbability;

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

    // The peak of one band's target design (floor 0 dB; 0 for an inactive
    // band): the per-band term of stressStackDb, on one input.
    double stressBandPeakDb (const eq::BandInput& in, const eq::Designer& designer, double rate)
    {
        if (! in.active)
            return 0.0;

        const double top = 0.4999 * rate;
        eq::BandDesign design {};
        designer.design (in.settings, design);

        double peak = 0.0;

        for (int i = 0; i < 64; ++i)
        {
            const double hz = 5.0 * std::pow (top / 5.0, (double) i / 63.0);
            peak = juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * hz / rate));
        }

        return juce::jmax (peak, eq::bandMagDb (design, 2.0 * eq::pi * juce::jmin (in.settings.freqHz, top) / rate));
    }

    // True when a band's structural fields (type, slope, placement, enable,
    // band bypass) differ between two draws: the change that starts a fade.
    bool stressStructureChanged (const StressBand& a, const StressBand& b)
    {
        return a.type != b.type || a.slope != b.slope || a.placement != b.placement
            || a.enabled != b.enabled || a.bypass != b.bypass;
    }

    // PLAN-2.4 P14: the largest static stack of the whole P14 draw, at the
    // blocks of 512 and 4096 samples, with the stack taking per FADING band the
    // larger peak of its two slots (RESEARCH-2.4 C10, which P14 adopts). Only
    // a structural change starts a fade; a continuous redraw ramps on one
    // slot, as in 2.3, and counts its latest request only. Execute-session
    // deviation (2026-11-14): the first T7 form took the larger of the two
    // latest requests on every band at every block, which charged a second
    // slot to bands that have none and left no seed under +600 dB at
    // 176.4 kHz in 32 attempts. Here each band follows the engine's fade
    // schedule (PLAN-2.4 P4): a structural change with no fade in flight
    // starts an N-sample fade whose outgoing slot is the request just before
    // the change; a change during a fade is latched and starts one more fade
    // at that fade's end, its outgoing slot the request then in force. While
    // a fade is in flight the band's term is the larger of the latest request
    // and the outgoing slot.
    double stressLargestStackDb24 (double rate, double seconds, std::uint64_t seed, int structEvery)
    {
        const auto designer = designerFor (rate);
        const auto total = (std::int64_t) std::llround (seconds * rate);
        const auto fadeN = (std::int64_t) eq::fadeLengthSamples (rate);

        StressDraw24 draw;
        draw.random.state = seed;

        double largest = 0.0;
        std::int64_t position = 0;

        double outgoingPeak[Processor::kNumBands] = {};
        std::int64_t fadeEnd[Processor::kNumBands] = {};
        bool latched[Processor::kNumBands] = {};

        while (position < total)
        {
            const int size = stressBlockSize (position, rate);
            const int m = (int) juce::jmin ((std::int64_t) size, total - position);

            const StressDraw24 previous = draw;
            draw.nextBlock (structEvery);

            for (int b = 0; b < Processor::kNumBands; ++b)
            {
                // A latched change whose fade has started since the last
                // block (at the previous fade's end, inside the last block):
                // its outgoing slot ran the request of that block.
                if (latched[b] && position >= fadeEnd[b])
                {
                    latched[b] = false;
                    outgoingPeak[b] = stressBandPeakDb (eq::makeBandInput (previous.rawBand (b), previous.scale, 0.0), *designer, rate);
                    fadeEnd[b] += fadeN;
                }

                if (! stressStructureChanged (previous.bands[b], draw.bands[b]))
                    continue;

                if (position >= fadeEnd[b])
                {
                    outgoingPeak[b] = stressBandPeakDb (eq::makeBandInput (previous.rawBand (b), previous.scale, 0.0), *designer, rate);
                    fadeEnd[b] = position + fadeN;
                }
                else
                {
                    latched[b] = true;
                }
            }

            if (size >= 512)
            {
                double stack = 0.0;

                for (int b = 0; b < Processor::kNumBands; ++b)
                {
                    const double now = stressBandPeakDb (eq::makeBandInput (draw.rawBand (b), draw.scale, 0.0), *designer, rate);
                    stack += position < fadeEnd[b] ? juce::jmax (now, outgoingPeak[b]) : now;
                }

                largest = juce::jmax (largest, stack);
            }

            position += m;
        }

        return largest;
    }

    // As chooseStressSeed, on the P14 draw. The every-block run (structEvery
    // 1) takes its own seeds.
    StressSeed chooseStressSeed24 (double rate, double seconds, int structEvery)
    {
        StressSeed chosen;

        for (int attempt = 0; attempt < 32; ++attempt)
        {
            std::uint64_t seed = ((0x9E3779B97F4A7C15ull ^ (std::uint64_t) rate)
                                  + (std::uint64_t) attempt * 0xD1B54A32D192ED03ull)
                               ^ (structEvery <= 1 ? 0x2B992DDFA23249D6ull : 0x0ull);

            if (seed == 0)
                seed = 1;

            const double stack = stressLargestStackDb24 (rate, seconds, seed, structEvery);

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

    /** The P14 stress render: as stressLoop, on the P14 draw, every value
        stored before every block. alternateRoute: every second bypassed span
        arrives through processBlockBypassed (the bypass parameter then stays
        off) instead of through the parameter. `process (buffer, midi,
        viaBypassed)` runs the block (A3 arms its window there). */
    template <typename ProcessFn>
    StressOutcome stressLoop24 (Processor& p, const Handles& h, double rate, double seconds, std::uint64_t seed,
                                int structEvery, bool alternateRoute, ProcessFn&& process)
    {
        StressOutcome out;

        StressDraw24 draw;
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

            draw.nextBlock (structEvery);

            const bool viaBypassed = alternateRoute && draw.bypass && (draw.toggles % 4) == 1;

            h.gainScale->store (draw.scale);
            h.bypass->store (draw.bypass && ! viaBypassed ? 1.0f : 0.0f);

            for (size_t b = 0; b < h.band.size(); ++b)
            {
                const StressBand& s = draw.bands[b];
                const BandHandles& hb = h.band[b];

                hb.freq->store (s.freq);
                hb.gain->store (s.gain);
                hb.q->store (s.q);
                hb.type->store (s.type);
                hb.slope->store (s.slope);
                hb.placement->store (s.placement);
                hb.enabled->store (s.enabled);
                hb.bypass->store (s.bypass);
            }

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

            process (block, midi, viaBypassed);

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

    /** runStress on the P14 draw: the stress, then every band off and the
        global bypass off, 0.5 s in blocks of 512. */
    StressResult runStress24 (Processor& p, const Handles& h, double rate, double seconds, int structEvery)
    {
        StressResult result;
        result.chosen = chooseStressSeed24 (rate, seconds, structEvery);

        if (! result.chosen.ok)
            return result;

        result.outcome = stressLoop24 (p, h, rate, seconds, result.chosen.seed, structEvery, false,
                                       [&p] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool viaBypassed)
                                       {
                                           if (viaBypassed)
                                               p.processBlockBypassed (buffer, midiMessages);
                                           else
                                               p.processBlock (buffer, midiMessages);
                                       });
        result.ran = true;

        // Every band off and the bypass off: the output must be the input again within 100 ms.
        h.bypass->store (0.0f);

        for (const auto& hb : h.band)
            hb.enabled->store (0.0f);

        constexpr int offBlockSize = 512;
        const int numBlocks = (int) std::ceil (0.5 * rate / (double) offBlockSize);

        Xorshift audio;
        audio.state = result.chosen.seed ^ 0x0FF0FF0FF0FF0FF0ull;

        if (audio.state == 0)
            audio.state = 1;

        juce::AudioBuffer<float> block (2, offBlockSize), kept (2, offBlockSize);
        juce::MidiBuffer midi;
        int lastUnequalEnd = 0;

        for (int k = 0; k < numBlocks; ++k)
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                float* const data = block.getWritePointer (channel);

                for (int i = 0; i < offBlockSize; ++i)
                    data[i] = (float) ((audio.uniform() * 2.0 - 1.0) * 0.25);

                kept.copyFrom (channel, 0, block, channel, 0, offBlockSize);
            }

            p.processBlock (block, midi);

            if (! allFinite (block))
                result.offFinite = false;

            if (! sameAudio (block, kept))
                lastUnequalEnd = (k + 1) * offBlockSize;
        }

        result.offEqualFromMs = 1000.0 * (double) lastUnequalEnd / rate;
        p.releaseResources();
        return result;
    }

    void stressRows()
    {
        section ("S5 / S6. random stress and non-finite input through processBlock (Phase 2.3: G5; PLAN-2.3 P9, P13)");

        // S5 (PLAN-2.3; not the state-codec row of Stage 1)
        // Amended in Phase 2.4 (PLAN-2.4 P18, P14): the P14 draw (structure
        // per band with probability 1/50 per block, global bypass toggled with
        // probability 1/200 per block), 60 s per rate (--long 600 s), plus the
        // every-block run (structure redrawn at every block), 10 s per rate in
        // both; the stack guard takes per band the larger peak of its two
        // latest requests. Verdict text unchanged; the tally holds 16 runs.
        {
            const double seconds = longRunRequested ? 600.0 : 60.0;
            const double everyBlockSeconds = 10.0;
            const double rates[8] = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };

            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (int i = 0; i < 16; ++i)
            {
                instances.push_back (makeBands (2, 0, mixedSpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<StressResult> results (16);

            parallelFor (16, [&instances, &handles, &results, &rates, seconds, everyBlockSeconds] (int j)
            {
                const auto slot = (size_t) j;
                const bool everyBlock = j >= 8;
                results[slot] = runStress24 (*instances[slot], handles[slot], rates[slot % 8],
                                             everyBlock ? everyBlockSeconds : seconds, everyBlock ? 1 : 50);
            });

            Tally tally;
            int reseeded = 0;
            double largestPeakDb = -std::numeric_limits<double>::infinity();
            double largestStackDb = 0.0;

            for (size_t r = 0; r < 16; ++r)
            {
                const StressResult& s = results[r];
                const bool everyBlock = r >= 8;
                const double runSeconds = everyBlock ? everyBlockSeconds : seconds;
                const juce::String where = fmtG (rates[r % 8] / 1000.0, 5) + " kHz" + (everyBlock ? " every-block" : "");

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
                             where.toRawUTF8(), runSeconds, s.chosen.attempt, s.chosen.largestStackDb,
                             (long long) s.outcome.nonFinite, (long long) s.outcome.blocks, (long long) s.outcome.actingBlocks,
                             peakDb, s.offEqualFromMs);

                tally.check (s.outcome.nonFinite == 0 && s.offFinite && s.outcome.actingBlocks > 0 && s.offEqualFromMs <= 100.0,
                             where + ": " + juce::String ((juce::int64) s.outcome.nonFinite) + " non-finite samples"
                                 + (s.outcome.actingBlocks > 0 ? "" : "; the bands never act")
                                 + "; bands off, equal to input from " + fmtG (s.offEqualFromMs, 4) + " ms");
            }

            row (tally.bad == 0 && tally.total == 16,
                 "S5 G5 through processBlock: 24 bands, every parameter redrawn (P13), blocks 4096 / 512 / 64 / 7 / 1, 8 rates: stack guard holds; every sample finite; all bands off -> memcmp-equal to input within 100 ms",
                 tally.detail() + "  P14 draw (PLAN-2.4): " + fmtG (seconds, 4) + " s per rate" + (longRunRequested ? " (--long)" : "")
                     + " + every-block run " + fmtG (everyBlockSeconds, 3) + " s per rate"
                     + "; largest peak " + fmtG (largestPeakDb, 4) + " dB re input peak; largest static stack (fading bands: larger of their two slots) "
                     + fmtG (largestStackDb, 4) + " dB; seed changed at " + juce::String (reseeded) + " of 16 runs");
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

    //==========================================================================
    //==========================================================================
    // Stage 2, Phase 2.4: structural crossfades and bypass (PLAN-2.4 T7)
    //==========================================================================
    //==========================================================================

    //==========================================================================
    // Workers with an index
    //==========================================================================
    /** How many workers (and so plugin instances) a pool of `count` jobs uses. */
    int workerCount (int count)
    {
        const int hardware = (int) std::thread::hardware_concurrency();
        return juce::jlimit (1, 12, juce::jmin (juce::jmax (1, count), hardware > 0 ? hardware : 1));
    }

    /** parallelFor with the worker's index: job (index, worker), worker in
        0 ... workers - 1, one job at a time per worker. The caller constructs
        one plugin instance per worker on the main thread; a job rewrites every
        parameter it uses before it renders. Jobs print nothing. */
    template <typename JobFn>
    void parallelForWorkers (int count, int workers, JobFn&& job)
    {
        if (count <= 0 || workers <= 0)
            return;

        std::atomic<int> next { 0 };

        const auto work = [&next, &job, count] (int worker)
        {
            for (;;)
            {
                const int mine = next.fetch_add (1);

                if (mine >= count)
                    return;

                job (mine, worker);
            }
        };

        std::vector<std::thread> pool;

        for (int w = 1; w < workers; ++w)
            pool.emplace_back (work, w);

        work (0);

        for (auto& thread : pool)
            thread.join();
    }

    //==========================================================================
    // Readings (fade_bench maxD2 and kappa2)
    //==========================================================================
    /** The largest |y[n + 1] - 2 y[n] + y[n - 1]| from `from` on. */
    double maxD2Of (const std::vector<double>& y, size_t from)
    {
        double d = 0.0;

        for (size_t i = juce::jmax (from, (size_t) 1); i + 1 < y.size(); ++i)
        {
            const double v = std::abs (y[i + 1] - 2.0 * y[i] + y[i - 1]);

            if (! (v <= d))
                d = v;
        }

        return d;
    }

    struct StereoReading
    {
        double d2 = 0.0;        // largest second difference, worse channel
        double kappa = 0.0;     // local kappa, worse channel
    };

    StereoReading readStereo (const std::vector<double>& left, const std::vector<double>& right, double rate, size_t from)
    {
        StereoReading r;
        r.d2 = juce::jmax (maxD2Of (left, from), maxD2Of (right, from));
        r.kappa = juce::jmax (localKappa (left, rate, from, left.size()), localKappa (right, rate, from, right.size()));
        return r;
    }

    StereoReading readStereo (const juce::AudioBuffer<float>& y, double rate, size_t from)
    {
        return readStereo (widened (y, 0), widened (y, 1), rate, from);
    }

    //==========================================================================
    // The step rows (PLAN-2.4 P11; fade_bench stepRows, deep, stimulus)
    //==========================================================================
    struct StepCfg
    {
        eq::BandSettings s { eq::Type::bell, 1000.0, 0.0, 1.0, 0 };
        eq::Placement p = eq::Placement::stereo;
        bool active = true;
        bool bypass = false;        // global
    };

    struct StepRow
    {
        std::string name;
        int group = 0;
        StepCfg a, b;
        int copies = 1;
        bool viaBandBypass = false;     // an inactive band is bNN_bypass on (else bNN_enabled off)
        bool voidAt1k = false;          // P13 (c): void at 1 kHz (the 10 Hz, Q 40 bell)
    };

    constexpr int kGroupType = 0, kGroupSlope = 1, kGroupPlace = 2, kGroupEnable = 3, kGroupBypass = 4,
                  kGroupEnable24 = 5, kGroupBandBypass = 6, kNumStepGroups = 7;

    const char* const kStepGroupNames[kNumStepGroups] = { "Type pairs", "Slope pairs", "Placement pairs", "Enable",
                                                          "Global bypass", "Enable, 24 bells", "Band bypass" };

    constexpr double kStepRate = 48000.0;
    constexpr double kStepLead = 0.5, kStepSeconds = 0.100;
    constexpr int kNumSteps = 8;

    // Every step render runs gain_scale 200 % with the raw gain at half the
    // bench's effective gain: gEff = raw * 200 / 100 is then the bench's value
    // exactly, also where it lies outside the raw +-30 dB (Tilt +36 / -36).
    constexpr float kStepGainScale = 200.0f;

    StepCfg deepCfg (eq::Type t, double tone, double gain = 18.0)
    {
        StepCfg c;
        c.s.type = t;
        c.s.gainDbEff = gain;
        c.s.q = 1.0;
        c.s.slopeIndex = eq::usesSlope (t) ? 1 : 0;

        switch (t)
        {
            case eq::Type::bell:       c.s.freqHz = tone;  break;
            case eq::Type::lowShelf:   c.s.freqHz = 2.0 * tone;  break;
            case eq::Type::highShelf:  c.s.freqHz = 0.5 * tone;  break;
            case eq::Type::lowCut:     c.s.freqHz = tone;  c.s.slopeIndex = 3;  break;
            case eq::Type::highCut:    c.s.freqHz = tone;  c.s.slopeIndex = 3;  break;
            case eq::Type::notch:      c.s.freqHz = 1.15 * tone;  break;
            case eq::Type::bandPass:   c.s.freqHz = 3.0 * tone;  break;
            case eq::Type::tilt:       c.s.freqHz = 0.25 * tone;  c.s.gainDbEff = 2.0 * gain > 60.0 ? 60.0 : 2.0 * gain;  break;
        }

        return c;
    }

    std::string typeNameOf (eq::Type t)            { return kTypeNames[(int) t]; }
    std::string placementNameOf (eq::Placement p)  { return kPlacementNames[(int) p]; }

    /** The 15 subjects of the Enable, Global bypass and band-bypass groups. */
    std::vector<std::pair<std::string, StepCfg>> stepSubjects (double tone)
    {
        std::vector<std::pair<std::string, StepCfg>> subjects;
        subjects.push_back ({ "Bell +18", deepCfg (eq::Type::bell, tone) });
        subjects.push_back ({ "Bell -18", deepCfg (eq::Type::bell, tone, -18.0) });
        { StepCfg c = deepCfg (eq::Type::bell, tone);  c.s.q = 40.0;  subjects.push_back ({ "Bell +18 Q 40", c }); }
        { StepCfg c = deepCfg (eq::Type::bell, tone);  c.s.freqHz = 10.0;  c.s.q = 40.0;  c.s.gainDbEff = 12.0;  subjects.push_back ({ "Bell 10 Hz Q 40 +12", c }); }
        { StepCfg c = deepCfg (eq::Type::lowCut, tone);  c.s.freqHz = 30.0;  c.s.slopeIndex = 7;  subjects.push_back ({ "LowCut 96 at 30 Hz", c }); }
        { StepCfg c = deepCfg (eq::Type::lowCut, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "LowCut 96 at tone", c }); }
        { StepCfg c = deepCfg (eq::Type::lowCut, tone);  c.s.slopeIndex = 7;  c.s.freqHz = 0.8 * tone;  c.s.q = 4.0;  subjects.push_back ({ "LowCut 96 Q 4 below tone", c }); }
        { StepCfg c = deepCfg (eq::Type::highCut, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighCut 96 at tone", c }); }
        { StepCfg c = deepCfg (eq::Type::highCut, tone);  c.s.slopeIndex = 0;  subjects.push_back ({ "HighCut 6 at tone", c }); }
        subjects.push_back ({ "HighShelf 12 +18", deepCfg (eq::Type::highShelf, tone) });
        { StepCfg c = deepCfg (eq::Type::highShelf, tone);  c.s.slopeIndex = 7;  subjects.push_back ({ "HighShelf 96 +18", c }); }
        subjects.push_back ({ "LowShelf 12 -18", deepCfg (eq::Type::lowShelf, tone, -18.0) });
        subjects.push_back ({ "Notch near tone", deepCfg (eq::Type::notch, tone) });
        subjects.push_back ({ "BandPass at 3x tone", deepCfg (eq::Type::bandPass, tone) });
        subjects.push_back ({ "Tilt 12 +36", deepCfg (eq::Type::tilt, tone) });
        return subjects;
    }

    const char* const kVoidSubject = "Bell 10 Hz Q 40 +12";

    /** fade_bench stepRows ("all"), in its order, plus the band-bypass group:
        242 + 15 rows. */
    std::vector<StepRow> stepRowsFor (double tone)
    {
        std::vector<StepRow> rows;
        const eq::Type types[8] = { eq::Type::bell, eq::Type::lowShelf, eq::Type::highShelf, eq::Type::lowCut,
                                    eq::Type::highCut, eq::Type::notch, eq::Type::bandPass, eq::Type::tilt };

        const auto add = [&rows] (const std::string& name, int group, const StepCfg& a, const StepCfg& b,
                                  int copies, bool viaBandBypass, bool voidAt1k)
        {
            StepRow r;
            r.name = name;
            r.group = group;
            r.a = a;
            r.b = b;
            r.copies = copies;
            r.viaBandBypass = viaBandBypass;
            r.voidAt1k = voidAt1k;
            rows.push_back (r);
        };

        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                add ("type " + typeNameOf (types[i]) + " <> " + typeNameOf (types[j]), kGroupType,
                     deepCfg (types[i], tone), deepCfg (types[j], tone), 1, false, false);

        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
                if (eq::isGainType (types[i]) || eq::isGainType (types[j]))
                    add ("type -18 " + typeNameOf (types[i]) + " <> " + typeNameOf (types[j]), kGroupType,
                         deepCfg (types[i], tone, -18.0), deepCfg (types[j], tone, -18.0), 1, false, false);

        for (const eq::Type t : { eq::Type::lowCut, eq::Type::highCut, eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt })
            for (int i = 0; i < 8; ++i)
                for (int j = i + 1; j < 8; ++j)
                {
                    StepCfg a = deepCfg (t, tone), b = a;
                    a.s.slopeIndex = i;
                    b.s.slopeIndex = j;
                    add ("slope " + typeNameOf (t) + " " + std::to_string (6 * eq::slopeOrder[i]) + " <> "
                             + std::to_string (6 * eq::slopeOrder[j]), kGroupSlope, a, b, 1, false, false);
                }

        for (int v = 0; v < 2; ++v)
            for (int i = 0; i < 5; ++i)
                for (int j = i + 1; j < 5; ++j)
                {
                    StepCfg a = v == 0 ? deepCfg (eq::Type::bell, tone) : deepCfg (eq::Type::lowCut, tone);

                    if (v == 1)
                        a.s.slopeIndex = 7;

                    StepCfg b = a;
                    a.p = (eq::Placement) i;
                    b.p = (eq::Placement) j;
                    add (std::string ("place ") + (v == 0 ? "Bell +18 " : "LowCut 96 ") + placementNameOf (a.p) + " <> "
                             + placementNameOf (b.p), kGroupPlace, a, b, 1, false, false);
                }

        const auto subjects = stepSubjects (tone);

        for (const auto& sub : subjects)
        {
            StepCfg off = sub.second;
            off.active = false;
            add ("enable " + sub.first, kGroupEnable, off, sub.second, 1, false, sub.first == kVoidSubject);
        }

        for (const auto& sub : subjects)
        {
            StepCfg byp = sub.second;
            byp.bypass = true;
            add ("global bypass, " + sub.first, kGroupBypass, sub.second, byp, 1, false, sub.first == kVoidSubject);
        }

        {
            StepCfg c = deepCfg (eq::Type::bell, tone, 1.0);
            c.s.q = 4.0;
            StepCfg byp = c;
            byp.bypass = true;
            add ("global bypass, 24 x Bell +1 Q 4", kGroupBypass, c, byp, 24, false, false);

            StepCfg off = c;
            off.active = false;
            add ("enable 24 x Bell +1 Q 4", kGroupEnable24, off, c, 24, false, false);
        }

        // The render harness's own group: the same 15 subjects through bNN_bypass.
        for (const auto& sub : subjects)
        {
            StepCfg off = sub.second;
            off.active = false;
            add ("band bypass " + sub.first, kGroupBandBypass, off, sub.second, 1, true, sub.first == kVoidSubject);
        }

        return rows;
    }

    /** 0: A, 1: B. motion 0: A, then B / A alternating every 100 ms from
        0.5 s, 8 steps (fade_bench renderSteps, its float arithmetic included);
        1: held at A; 2: held at B. */
    int stepState (int motion, int position, double rate)
    {
        if (motion == 2)
            return 1;

        if (motion == 1)
            return 0;

        const double t = (double) position / rate;

        if (t < kStepLead)
            return 0;

        const int k = (int) ((t - kStepLead) / kStepSeconds);
        return k < kNumSteps ? ((k + 1) & 1) : (kNumSteps & 1);
    }

    int stepLength (double rate)
    {
        return (int) ((kStepLead + (double) kNumSteps * kStepSeconds + 0.3) * rate);
    }

    /** L: the tone at -6 dBFS; R: the same tone at -10.5 dBFS, 1.1 rad later. */
    juce::AudioBuffer<float> stepStimulus (double tone, double rate, int length)
    {
        juce::AudioBuffer<float> buffer (2, length);
        float* const left = buffer.getWritePointer (0);
        float* const right = buffer.getWritePointer (1);

        for (int i = 0; i < length; ++i)
        {
            const double t = (double) i / rate;
            left[i] = (float) (0.5 * std::sin (2.0 * eq::pi * tone * t));
            right[i] = (float) (0.3 * std::sin (2.0 * eq::pi * tone * t + 1.1));
        }

        return buffer;
    }

    /** The raw values of band `band` (0-based) for configuration c of row r. */
    eq::RawBand stepRawOf (const StepRow& r, const StepCfg& c, int band)
    {
        eq::RawBand raw;
        raw.type = (float) (int) c.s.type;
        raw.freq = (float) c.s.freqHz;
        raw.gain = (float) (c.s.gainDbEff / 2.0);
        raw.q = (float) c.s.q;
        raw.slope = (float) c.s.slopeIndex;
        raw.placement = (float) (int) c.p;

        if (band >= r.copies)
        {
            raw.enabled = 0.0f;
            raw.bypass = 0.0f;
        }
        else if (r.viaBandBypass)
        {
            raw.enabled = 1.0f;
            raw.bypass = c.active ? 0.0f : 1.0f;
        }
        else
        {
            raw.enabled = c.active ? 1.0f : 0.0f;
            raw.bypass = 0.0f;
        }

        return raw;
    }

    void storeRaw (const BandHandles& hb, const eq::RawBand& raw)
    {
        hb.enabled->store (raw.enabled);
        hb.bypass->store (raw.bypass);
        hb.type->store (raw.type);
        hb.freq->store (raw.freq);
        hb.gain->store (raw.gain);
        hb.q->store (raw.q);
        hb.slope->store (raw.slope);
        hb.placement->store (raw.placement);
    }

    /** Every parameter the step rows use. bypassViaEntry: the global bypass
        arrives through processBlockBypassed, the parameter stays off. */
    void storeStepCfg (const Handles& h, const StepRow& r, const StepCfg& c, bool bypassViaEntry)
    {
        h.gainScale->store (kStepGainScale);
        h.bypass->store (c.bypass && ! bypassViaEntry ? 1.0f : 0.0f);

        for (int b = 0; b < Processor::kNumBands; ++b)
            storeRaw (h.band[(size_t) b], stepRawOf (r, c, b));
    }

    eq::BandInput stepInputOf (const StepRow& r, const StepCfg& c, int band)
    {
        eq::BandInput in = eq::makeBandInput (stepRawOf (r, c, band), kStepGainScale, 0.0);
        in.placement = eq::effectivePlacement (in.placement, true);
        return in;
    }

    juce::AudioBuffer<float> renderStepMotion (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input,
                                               double rate, const StepRow& r, int motion, int blockSize, bool bypassViaEntry)
    {
        storeStepCfg (h, r, r.a, bypassViaEntry);

        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, &r, motion, rate, bypassViaEntry] (BlockInfo& current)
                           {
                               const StepCfg& c = stepState (motion, current.position, rate) != 0 ? r.b : r.a;
                               storeStepCfg (h, r, c, bypassViaEntry);
                               current.viaBypassed = bypassViaEntry && c.bypass;
                           });
    }

    //==========================================================================
    // hardStepRender (PLAN-2.4 P10): the Phase 2.3 rule on plain eq::Band +
    // eq::Router, double. Per host block start, per band: structural ->
    // snapTo + reset (inactive: state cleared, not processed); continuous ->
    // setTarget, ramped on the tick grid; global bypass a hard switch (the
    // block untouched, every band's state cleared and its ramp snapped).
    //==========================================================================
    struct HardStepScript
    {
        std::vector<int> blockStart;
        std::vector<std::vector<eq::BandInput>> bands;      // [block][band]
        std::vector<char> bypass;                           // [block]
    };

    HardStepScript stepScriptOf (const StepRow& r, double rate, int length, int blockSize)
    {
        HardStepScript script;

        for (int position = 0; position < length; position += blockSize)
        {
            const StepCfg& c = stepState (0, position, rate) != 0 ? r.b : r.a;
            std::vector<eq::BandInput> perBand;

            for (int b = 0; b < r.copies; ++b)
                perBand.push_back (stepInputOf (r, c, b));

            script.blockStart.push_back (position);
            script.bands.push_back (perBand);
            script.bypass.push_back ((char) (c.bypass ? 1 : 0));
        }

        return script;
    }

    std::array<std::vector<double>, 2> hardStepRender (const std::vector<double>& inL, const std::vector<double>& inR,
                                                       bool stereo, double rate, const HardStepScript& script)
    {
        if (script.blockStart.empty() || script.blockStart[0] != 0
            || script.bands.size() != script.blockStart.size() || script.bypass.size() != script.blockStart.size())
            return {};

        std::array<std::vector<double>, 2> out { inL, inR };

        const size_t numBands = script.bands[0].size();
        eq::Designer designer;
        designer.prepare (rate);

        const int grid = eq::controlGridSize (rate);
        const int rampTicks = eq::rampLengthTicks (rate);

        std::vector<eq::Band> bands (numBands);
        std::vector<eq::BandInput> held (numBands);
        std::vector<char> designed (numBands, 0);

        for (auto& band : bands)
        {
            band.setControlGrid (grid, rampTicks);
            band.reset();
        }

        const size_t length = inL.size();

        for (size_t k = 0; k < script.blockStart.size(); ++k)
        {
            const auto start = (size_t) script.blockStart[k];
            const size_t end = k + 1 < script.blockStart.size() ? juce::jmin (length, (size_t) script.blockStart[k + 1]) : length;

            if (start >= length || script.bands[k].size() != numBands)
                break;

            for (size_t b = 0; b < numBands; ++b)
            {
                const eq::BandInput& next = script.bands[k][b];
                eq::Band& band = bands[b];

                if (designed[b] == 0)
                {
                    if (next.active)
                    {
                        band.snapTo (next.settings, designer);
                        band.reset();
                    }

                    held[b] = next;
                    designed[b] = 1;
                    continue;
                }

                const eq::BandChange change = eq::classifyBandChange (held[b], next);

                if (change == eq::BandChange::structural)
                {
                    if (next.active)
                        band.snapTo (next.settings, designer);
                    else
                        band.snapToTarget (designer);

                    band.reset();
                }
                else if (change == eq::BandChange::continuous && next.active)
                {
                    band.setTarget (next.settings);
                }

                held[b] = next;
            }

            if (script.bypass[k] != 0)
            {
                for (auto& band : bands)
                {
                    band.reset();
                    band.snapToTarget (designer);
                }

                continue;
            }

            size_t position = start;

            while (position < end)
            {
                const auto phase = (int) (position % (size_t) grid);

                if (phase == 0)
                    for (size_t b = 0; b < numBands; ++b)
                        if (held[b].active)
                            bands[b].controlTick (designer);

                const size_t m = juce::jmin (end - position, (size_t) (grid - phase));

                for (size_t b = 0; b < numBands; ++b)
                    if (held[b].active)
                        eq::Router::process (bands[b], held[b].placement, out[0].data() + position,
                                             stereo ? out[1].data() + position : nullptr, (int) m, stereo);

                position += m;
            }
        }

        return out;
    }

    //==========================================================================
    // fadeRefRender (PLAN-2.4 P12). Two static chains, each Designer::design
    // -> toRuntime -> svfTick / onePoleTick; the placement algebra of the
    // ARCHITECTURE "Processing Chain" table and the weight formula of P2,
    // re-stated here. The outgoing chain runs from sample 0, the incoming one
    // from rest at the fade's first sample; an inactive side is the dry input.
    // No two-slot band, fade table, bypass fade, eq::Band or eq::Router.
    //==========================================================================
    /** N re-stated: 20 ms rounded to nearest, at least 1. */
    int refFadeLength (double rate)
    {
        return juce::jmax (1, (int) std::lround (0.020 * rate));
    }

    /** The weight of the incoming side at fade sample n of N, re-stated. */
    double refFadeWeight (int n, int fadeLength)
    {
        const double angle = eq::pi * (double) (n + 1) / (double) (fadeLength + 1);
        return 0.5 - 0.5 * std::cos (angle);
    }

    struct RefChain
    {
        int numSections = 0;
        eq::SectionRuntime runtime[eq::maxSections];
        double ic1[2][eq::maxSections] = {};
        double ic2[2][eq::maxSections] = {};
        double s[2][eq::maxSections] = {};

        double tick (int lane, double x)
        {
            double v = x;

            for (int i = 0; i < numSections; ++i)
            {
                if (runtime[i].hasSvf)
                    v = eq::svfTick (runtime[i].svf, ic1[lane][i], ic2[lane][i], v);

                if (runtime[i].hasOnePole)
                    v = eq::onePoleTick (runtime[i].onePole, s[lane][i], v);
            }

            return v;
        }
    };

    void prepareRefChain (RefChain& chain, const eq::Designer& designer, const eq::BandInput& in)
    {
        chain.numSections = 0;

        if (! in.active)
            return;

        eq::BandDesign design {};
        designer.design (in.settings, design);
        chain.numSections = juce::jlimit (0, (int) eq::maxSections, design.numSections);

        for (int i = 0; i < chain.numSections; ++i)
            chain.runtime[i] = eq::toRuntime (design.params[(size_t) i]);
    }

    /** One sample of one side: Stereo L <- F(L), R <- F(R); Left / Right one
        lane; Mid m = (L + R) / 2, d = F(m) - m, L += d, R += d; Side
        s = (L - R) / 2, d = F(s) - s, L += d, R -= d. Mono: lane 0. */
    void refApply (RefChain& chain, const eq::BandInput& in, bool stereo, double l, double r, double& yl, double& yr)
    {
        yl = l;
        yr = r;

        if (! in.active)
            return;

        if (! stereo)
        {
            yl = chain.tick (0, l);
            return;
        }

        switch (in.placement)
        {
            case eq::Placement::left:
                yl = chain.tick (0, l);
                return;

            case eq::Placement::right:
                yr = chain.tick (1, r);
                return;

            case eq::Placement::mid:
            {
                const double m = 0.5 * (l + r);
                const double d = chain.tick (0, m) - m;
                yl = l + d;
                yr = r + d;
                return;
            }

            case eq::Placement::side:
            {
                const double sv = 0.5 * (l - r);
                const double d = chain.tick (0, sv) - sv;
                yl = l + d;
                yr = r - d;
                return;
            }

            case eq::Placement::stereo:
                break;
        }

        yl = chain.tick (0, l);
        yr = chain.tick (1, r);
    }

    std::array<std::vector<double>, 2> fadeRefRender (const std::vector<double>& inL, const std::vector<double>& inR,
                                                      double rate, const eq::BandInput& from, const eq::BandInput& to,
                                                      size_t fadeStart, int fadeLength)
    {
        if (inL.size() != inR.size() || fadeLength < 1)
            return {};

        const auto designer = designerFor (rate);
        auto outgoing = std::make_unique<RefChain>();
        auto incoming = std::make_unique<RefChain>();
        prepareRefChain (*outgoing, *designer, from);
        prepareRefChain (*incoming, *designer, to);

        std::array<std::vector<double>, 2> out { std::vector<double> (inL.size(), 0.0), std::vector<double> (inL.size(), 0.0) };

        for (size_t i = 0; i < inL.size(); ++i)
        {
            double al = 0.0, ar = 0.0;
            refApply (*outgoing, from, true, inL[i], inR[i], al, ar);

            if (i < fadeStart)
            {
                out[0][i] = al;
                out[1][i] = ar;
                continue;
            }

            double bl = 0.0, br = 0.0;
            refApply (*incoming, to, true, inL[i], inR[i], bl, br);

            const size_t into = i - fadeStart;

            if (into < (size_t) fadeLength)
            {
                const double w = refFadeWeight ((int) into, fadeLength);
                out[0][i] = al + w * (bl - al);
                out[1][i] = ar + w * (br - ar);
            }
            else
            {
                out[0][i] = bl;
                out[1][i] = br;
            }
        }

        return out;
    }

    //==========================================================================
    // XR-g3, XR-g3k, XR-g3h, XR-g3b. G3 step rows (PLAN-2.4 P11, P13)
    //==========================================================================
    struct StepJob
    {
        int tone = 0;               // 0: 50 Hz, 1: 1 kHz
        int rowIndex = 0;
        int blockSize = 32;
        bool viaEntry = false;      // XR-g3b: the bypassed blocks arrive through processBlockBypassed
        bool withHard = false;      // hardStepRender (block 512)
    };

    struct StepReading
    {
        bool valid = false;
        double dHeld = 0.0, dEng = 0.0, dHard = 0.0;
        double kHeld = 0.0, kEng = 0.0;
        bool hasHard = false;
    };

    /** One reading: held A, held B and the step render of one instance, the
        first 0.3 s not scored; the hard control on the same float input. */
    StepReading runStepJob (Processor& p, const Handles& h, const StepRow& r, const juce::AudioBuffer<float>& input,
                            const StepJob& job)
    {
        const double rate = kStepRate;
        const auto from = (size_t) std::lround (0.3 * rate);

        const auto heldA = renderStepMotion (p, h, input, rate, r, 1, job.blockSize, job.viaEntry);
        const auto heldB = renderStepMotion (p, h, input, rate, r, 2, job.blockSize, job.viaEntry);
        const auto engine = renderStepMotion (p, h, input, rate, r, 0, job.blockSize, job.viaEntry);

        const StereoReading a = readStereo (heldA, rate, from);
        const StereoReading b = readStereo (heldB, rate, from);
        const StereoReading e = readStereo (engine, rate, from);

        StepReading q;
        q.dHeld = juce::jmax (a.d2, b.d2);
        q.kHeld = juce::jmax (a.kappa, b.kappa);
        q.dEng = e.d2;
        q.kEng = e.kappa;

        bool hardOk = true;

        if (job.withHard)
        {
            const auto script = stepScriptOf (r, rate, input.getNumSamples(), job.blockSize);
            const auto hard = hardStepRender (widened (input, 0), widened (input, 1), true, rate, script);

            hardOk = hard[0].size() == (size_t) input.getNumSamples() && hard[1].size() == hard[0].size();

            if (hardOk)
                q.dHard = juce::jmax (maxD2Of (hard[0], from), maxD2Of (hard[1], from));

            hardOk = hardOk && std::isfinite (q.dHard);
            q.hasHard = true;
        }

        q.valid = allFinite (heldA) && allFinite (heldB) && allFinite (engine) && hardOk
               && q.dHeld > 0.0 && q.kHeld > 0.0 && std::isfinite (q.dEng) && std::isfinite (q.kEng);
        return q;
    }

    void stepRowsXr()
    {
        section ("XR-g3. G3 step rows through the plugin (Phase 2.4: DSP-04, FUNC-01, FUNC-04; PLAN-2.4 P11, P13)");

        const double tones[2] = { 50.0, 1000.0 };
        const int length = stepLength (kStepRate);
        const juce::AudioBuffer<float> inputs[2] = { stepStimulus (tones[0], kStepRate, length),
                                                     stepStimulus (tones[1], kStepRate, length) };
        const std::vector<StepRow> rows[2] = { stepRowsFor (tones[0]), stepRowsFor (tones[1]) };

        std::vector<StepJob> jobs;

        for (int t = 0; t < 2; ++t)
            for (size_t i = 0; i < rows[t].size(); ++i)
                for (const int blockSize : { 32, 512 })
                {
                    StepJob job;
                    job.tone = t;
                    job.rowIndex = (int) i;
                    job.blockSize = blockSize;
                    job.withHard = blockSize == 512;
                    jobs.push_back (job);
                }

        const size_t numMain = jobs.size();

        for (int t = 0; t < 2; ++t)
            for (size_t i = 0; i < rows[t].size(); ++i)
                if (rows[t][i].group == kGroupBypass)
                    for (const int blockSize : { 32, 512 })
                    {
                        StepJob job;
                        job.tone = t;
                        job.rowIndex = (int) i;
                        job.blockSize = blockSize;
                        job.viaEntry = true;
                        jobs.push_back (job);
                    }

        // One instance per worker, constructed here; the renders on the workers.
        const int workers = workerCount ((int) jobs.size());
        std::vector<std::unique_ptr<Processor>> instances;
        std::vector<Handles> handles;

        for (int w = 0; w < workers; ++w)
        {
            instances.push_back (makeBands (2, 0, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<StepReading> readings (jobs.size());

        parallelForWorkers ((int) jobs.size(), workers, [&jobs, &instances, &handles, &readings, &rows, &inputs] (int j, int w)
        {
            const StepJob& job = jobs[(size_t) j];
            readings[(size_t) j] = runStepJob (*instances[(size_t) w], handles[(size_t) w],
                                               rows[job.tone][(size_t) job.rowIndex], inputs[job.tone], job);
        });

        struct Agg
        {
            int n = 0;
            double worstD2 = 0.0;
            juce::String worstD2At;
            double worstK = 0.0;
            int kOver2 = 0;
            double minHard = std::numeric_limits<double>::infinity();
            int voids = 0;
        };

        Agg agg[2][kNumStepGroups];
        Tally g3, g3k, g3h;
        int voidCount = 0;
        juce::String voidFigures;
        double worstK50 = 0.0, worstD2All = 0.0, worstK1k = 0.0, weakestHard = std::numeric_limits<double>::infinity();
        int k50Over2 = 0;

        for (size_t j = 0; j < numMain; ++j)
        {
            const StepJob& job = jobs[j];
            const StepRow& r = rows[job.tone][(size_t) job.rowIndex];
            const StepReading& q = readings[j];
            const juce::String label = fmtG (tones[job.tone], 4) + " Hz, block " + juce::String (job.blockSize) + ", " + juce::String (r.name);

            const double dRatio = ratioOf (q.dEng, q.dHeld);
            const double kRatio = ratioOf (q.kEng, q.kHeld);

            g3.check (q.valid && dRatio <= 2.0, label + ": " + fmtG (dRatio, 4) + "x" + (q.valid ? "" : " INVALID"));

            if (! (dRatio <= worstD2All))
                worstD2All = dRatio;

            if (job.tone == 1)
            {
                g3k.check (q.valid && kRatio <= 2.5, label + ": " + fmtG (kRatio, 4) + "x" + (q.valid ? "" : " INVALID"));

                if (! (kRatio <= worstK1k))
                    worstK1k = kRatio;
            }
            else
            {
                if (! (kRatio <= worstK50))
                    worstK50 = kRatio;

                if (! (kRatio <= 2.0))
                    ++k50Over2;
            }

            Agg& a = agg[job.tone][r.group];
            ++a.n;

            if (! (dRatio <= a.worstD2))
            {
                a.worstD2 = dRatio;
                a.worstD2At = juce::String (r.name) + " b" + juce::String (job.blockSize);
            }

            if (! (kRatio <= a.worstK))
                a.worstK = kRatio;

            if (! (kRatio <= 2.0))
                ++a.kOver2;

            if (q.hasHard)
            {
                const double hRatio = ratioOf (q.dHard, q.dHeld);
                const bool isVoid = r.voidAt1k && job.tone == 1;

                if (isVoid)
                {
                    ++voidCount;
                    ++a.voids;
                    voidFigures += (voidFigures.isEmpty() ? "" : "; ") + juce::String (r.name) + " reads " + fmtG (hRatio, 3) + "x";
                    std::printf ("      XR-g3h %-80s hard / held %.2f  VOID, not counted (the 10 Hz, Q 40 bell does nothing to a 1 kHz tone)\n",
                                 label.toRawUTF8(), hRatio);
                    continue;
                }

                g3h.check (q.valid && hRatio >= 10.0, label + ": " + fmtG (hRatio, 4) + "x");
                a.minHard = juce::jmin (a.minHard, hRatio);
                weakestHard = juce::jmin (weakestHard, hRatio);
            }
        }

        // The bench's table, per group, float beside double (RESEARCH-2.4 Q1).
        for (int t = 0; t < 2; ++t)
            for (int g = 0; g < kNumStepGroups; ++g)
            {
                const Agg& a = agg[t][g];
                std::printf ("      XR-g3 %6.0f Hz  %-17s n %3d | d2 eng / held worst %6.3f (%s) | kappa eng / held worst %6.2f, above 2.0: %3d | hard / held lowest at block 512 %9.1f | void %d\n",
                             tones[t], kStepGroupNames[g], a.n, a.worstD2, a.worstD2At.toRawUTF8(), a.worstK, a.kOver2,
                             a.minHard, a.voids);
            }

        row (g3.bad == 0 && g3.total == 1028,
             "XR-g3 G3 step rows through processBlock: 242 rows + 15 band-bypass rows x 50 Hz / 1 kHz x blocks 32 / 512: largest second difference <= 2.0x the louder held render",
             g3.detail() + "  worst " + fmtG (worstD2All, 4) + "x (bench 1.53x at 50 Hz, 1.02x at 1 kHz)");

        row (g3k.bad == 0 && g3k.total == 514,
             "XR-g3k the same renders, 1 kHz: local kappa <= 2.5x the larger held reference",
             g3k.detail() + "  worst " + fmtG (worstK1k, 4) + "x (bench 2.20x)");

        info ("XR-g3k 50 Hz (printed, not gated, PLAN-2.4 P13 (b)): local kappa engine / held worst " + fmtG (worstK50, 4)
                  + "x; " + juce::String (k50Over2) + " of 514 readings above 2.0x (bench, 484 readings: worst 20.5x, 202 above 2.0x)");

        row (g3h.bad == 0 && g3h.total == 511 && voidCount == 3,
             "XR-g3h hardStepRender (the 2.3 rule, the plugin's float input) at block 512 reads >= 10x the louder held render (control fires); void readings printed, not counted",
             g3h.detail() + "  weakest " + fmtG (weakestHard, 4) + "x (bench 274x / 17.9x); void: " + voidFigures);

        // XR-g3b
        Tally g3b, g3bk;
        double worstB = 0.0, worstBk = 0.0, worstBk50 = 0.0;

        for (size_t j = numMain; j < jobs.size(); ++j)
        {
            const StepJob& job = jobs[j];
            const StepRow& r = rows[job.tone][(size_t) job.rowIndex];
            const StepReading& q = readings[j];
            const juce::String label = fmtG (tones[job.tone], 4) + " Hz, block " + juce::String (job.blockSize) + ", " + juce::String (r.name);

            const double dRatio = ratioOf (q.dEng, q.dHeld);
            const double kRatio = ratioOf (q.kEng, q.kHeld);

            g3b.check (q.valid && dRatio <= 2.0, label + ": " + fmtG (dRatio, 4) + "x" + (q.valid ? "" : " INVALID"));
            worstB = juce::jmax (worstB, dRatio);

            if (job.tone == 1)
            {
                g3bk.check (q.valid && kRatio <= 2.5, label + ": kappa " + fmtG (kRatio, 4) + "x");
                worstBk = juce::jmax (worstBk, kRatio);
            }
            else
            {
                worstBk50 = juce::jmax (worstBk50, kRatio);
            }
        }

        row (g3b.bad == 0 && g3b.total == 64 && g3bk.bad == 0 && g3bk.total == 32,
             "XR-g3b the Global bypass group through processBlockBypassed: (a) second difference <= 2.0x held, both tones; (b) 1 kHz local kappa <= 2.5x held",
             "(a) " + g3b.detail() + " worst " + fmtG (worstB, 4) + "x; (b) " + g3bk.detail() + " worst " + fmtG (worstBk, 4)
                 + "x; 50 Hz kappa worst " + fmtG (worstBk50, 4) + "x (printed)");
    }

    //==========================================================================
    // XR-ref, XR-rest. The engine does the specified fade (PLAN-2.4 P12)
    //==========================================================================
    /** XF-ref's 30 rows: 6 type pairs, 12 slope pairs (6 and 18 dB/oct among
        them), 6 placement pairs, 3 enables, 3 disables; the deep settings at
        1 kHz. */
    std::vector<StepRow> refRowsFor()
    {
        using T = eq::Type;
        const double tone = 1000.0;
        std::vector<StepRow> rows;

        const auto add = [&rows] (const std::string& name, const StepCfg& a, const StepCfg& b)
        {
            StepRow r;
            r.name = name;
            r.a = a;
            r.b = b;
            rows.push_back (r);
        };

        const std::pair<T, T> typePairs[6] = { { T::bell, T::lowShelf }, { T::highShelf, T::lowCut }, { T::highCut, T::notch },
                                               { T::bandPass, T::tilt }, { T::bell, T::highCut }, { T::lowCut, T::tilt } };

        for (const auto& tp : typePairs)
            add ("type " + typeNameOf (tp.first) + " -> " + typeNameOf (tp.second), deepCfg (tp.first, tone), deepCfg (tp.second, tone));

        struct SlopePair { T type; int from, to; };

        const SlopePair slopePairs[12] = { { T::lowCut, 0, 2 }, { T::lowCut, 2, 7 }, { T::lowCut, 5, 0 },
                                           { T::highCut, 0, 3 }, { T::highCut, 2, 5 }, { T::highCut, 7, 2 },
                                           { T::lowShelf, 0, 1 }, { T::lowShelf, 2, 6 }, { T::highShelf, 1, 2 },
                                           { T::highShelf, 0, 7 }, { T::tilt, 2, 4 }, { T::tilt, 0, 3 } };

        for (const auto& sp : slopePairs)
        {
            StepCfg a = deepCfg (sp.type, tone), b = a;
            a.s.slopeIndex = sp.from;
            b.s.slopeIndex = sp.to;
            add ("slope " + typeNameOf (sp.type) + " " + std::to_string (6 * eq::slopeOrder[sp.from]) + " -> "
                     + std::to_string (6 * eq::slopeOrder[sp.to]), a, b);
        }

        struct PlacePair { bool cut; int from, to; };

        const PlacePair placePairs[6] = { { false, 0, 1 }, { false, 1, 2 }, { false, 2, 3 }, { false, 3, 4 }, { false, 4, 0 }, { true, 0, 3 } };

        for (const auto& pp : placePairs)
        {
            StepCfg a = pp.cut ? deepCfg (T::lowCut, tone) : deepCfg (T::bell, tone);

            if (pp.cut)
                a.s.slopeIndex = 7;

            StepCfg b = a;
            a.p = (eq::Placement) pp.from;
            b.p = (eq::Placement) pp.to;
            add (std::string ("place ") + (pp.cut ? "LowCut 96 " : "Bell +18 ") + placementNameOf (a.p) + " -> " + placementNameOf (b.p), a, b);
        }

        {
            StepCfg shelf96 = deepCfg (T::highShelf, tone);
            shelf96.s.slopeIndex = 7;
            const std::pair<std::string, StepCfg> enables[3] = { { "Bell +18", deepCfg (T::bell, tone) }, { "HighShelf 96 +18", shelf96 },
                                                                 { "Tilt 12 +36", deepCfg (T::tilt, tone) } };

            for (const auto& e : enables)
            {
                StepCfg off = e.second;
                off.active = false;
                add ("enable " + e.first, off, e.second);
            }
        }

        {
            StepCfg cut96 = deepCfg (T::lowCut, tone);
            cut96.s.slopeIndex = 7;
            const std::pair<std::string, StepCfg> disables[3] = { { "LowCut 96 at tone", cut96 }, { "Notch near tone", deepCfg (T::notch, tone) },
                                                                  { "LowShelf 12 -18", deepCfg (T::lowShelf, tone, -18.0) } };

            for (const auto& e : disables)
            {
                StepCfg off = e.second;
                off.active = false;
                add ("disable " + e.first, e.second, off);
            }
        }

        return rows;
    }

    struct RefReading
    {
        bool valid = false;
        double errorDb = 0.0;       // largest |plugin - reference| re the output peak
        int changeAt = 0;
    };

    /** Stereo noise, blocks of 64; the change at sample 4096 (48 kHz) or 8192
        (96 kHz): a block start and a tick. Scored from the render start to
        50 ms after the fade. */
    RefReading runRefJob (Processor& p, const Handles& h, const StepRow& r, double rate)
    {
        RefReading q;
        q.changeAt = rate > 48000.0 ? 8192 : 4096;

        const int changeAt = q.changeAt;
        const int fadeN = refFadeLength (rate);
        const int length = changeAt + fadeN + (int) std::lround (0.050 * rate);
        const auto input = makeNoiseAt (2, length, 0x7E5F0002, kNoise12);

        storeStepCfg (h, r, r.a, false);

        const auto output = renderWith (p, input, rate, 64,
                                        [] (int, int) { return 64; },
                                        [&h, &r, changeAt] (BlockInfo& current)
                                        {
                                            storeStepCfg (h, r, current.position < changeAt ? r.a : r.b, false);
                                        });

        const auto reference = fadeRefRender (widened (input, 0), widened (input, 1), rate,
                                              stepInputOf (r, r.a, 0), stepInputOf (r, r.b, 0), (size_t) changeAt, fadeN);

        if (reference[0].size() != (size_t) length || reference[1].size() != (size_t) length || ! allFinite (output))
            return q;

        double error = 0.0, peak = 0.0;

        for (int channel = 0; channel < 2; ++channel)
        {
            const float* y = output.getReadPointer (channel);
            const auto& ref = reference[(size_t) channel];

            for (int i = 0; i < length; ++i)
            {
                const double v = (double) y[i];
                const double d = std::abs (v - ref[(size_t) i]);

                if (! (d <= error))
                    error = d;

                peak = juce::jmax (peak, std::abs (v));
            }
        }

        q.valid = peak > 0.0 && std::isfinite (error);
        q.errorDb = 20.0 * std::log10 (juce::jmax (error, 1.0e-300) / juce::jmax (peak, 1.0e-300));
        return q;
    }

    void fadeReferenceRowsXr()
    {
        section ("XR-ref / XR-rest. the specified fade through the plugin (Phase 2.4: PLAN-2.4 P12; VERIFICATION-2.3 issue 4)");

        // XR-ref
        {
            const auto rows = refRowsFor();
            const double rates[2] = { 48000.0, 96000.0 };

            Tally guard;

            for (const double fs : { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
                guard.check (refFadeLength (fs) == eq::fadeLengthSamples (fs),
                             "harness N differs from the engine's at " + fmtG (fs, 6) + " Hz");

            struct Job { size_t rowIndex; double rate; };
            std::vector<Job> jobs;

            for (const double rate : rates)
                for (size_t i = 0; i < rows.size(); ++i)
                    jobs.push_back ({ i, rate });

            const int workers = workerCount ((int) jobs.size());
            std::vector<std::unique_ptr<Processor>> instances;
            std::vector<Handles> handles;

            for (int w = 0; w < workers; ++w)
            {
                instances.push_back (makeBands (2, 0, mixedSpec));
                handles.push_back (handlesOf (*instances.back()));
            }

            std::vector<RefReading> readings (jobs.size());

            parallelForWorkers ((int) jobs.size(), workers, [&jobs, &instances, &handles, &readings, &rows] (int j, int w)
            {
                const Job& job = jobs[(size_t) j];
                readings[(size_t) j] = runRefJob (*instances[(size_t) w], handles[(size_t) w], rows[job.rowIndex], job.rate);
            });

            Tally tally;
            double worst = -std::numeric_limits<double>::infinity();
            juce::String worstAt;

            for (size_t j = 0; j < jobs.size(); ++j)
            {
                const RefReading& q = readings[j];
                const juce::String label = fmtG (jobs[j].rate / 1000.0, 4) + " kHz " + juce::String (rows[jobs[j].rowIndex].name);

                tally.check (q.valid && q.errorDb <= -120.0, label + ": " + fmtG (q.errorDb, 4) + " dB" + (q.valid ? "" : " INVALID"));

                if (! (q.errorDb <= worst))
                {
                    worst = q.errorDb;
                    worstAt = label;
                }
            }

            row (tally.bad == 0 && tally.total == 60 && guard.bad == 0 && guard.total == 8,
                 "XR-ref 30 first fades (6 type, 12 slope, 6 placement, 3 enable, 3 disable) at 48 and 96 kHz, noise: float output against fadeRefRender <= -120 dB re the output peak, render start to 50 ms after the fade",
                 tally.detail() + "  worst " + fmtG (worst, 4) + " dB (" + worstAt + "); harness N guard " + guard.detail());
        }

        // XR-rest (VERIFICATION-2.3 issue 4): RM-keep's stimulus, the type
        // change of keepRender (Bell -> Low Shelf at block 50, sample 24000).
        {
            const double rate = 48000.0;
            const int total = (int) std::lround (rate);
            const int editAt = kKeepEditBlock * kKeepBlock;
            const int fadeN = eq::fadeLengthSamples (rate);

            juce::AudioBuffer<float> burst (1, total);
            burst.clear();

            for (int i = 0; i < 480; ++i)
                burst.setSample (0, i, (float) (0.5 * std::sin (2.0 * eq::pi * 40.0 * (double) i / rate)));

            const auto unedited = keepRender (burst, rate, 0);
            const auto typeEdit = keepRender (burst, rate, 2);

            auto fresh = makeBands (1, 1, [] (int) { return makeSpec (kLowShelf, 40.0f, 24.0f, 20.0f, 1, kStereo); });
            const auto freshOut = renderWith (*fresh, sliceOf (burst, editAt, total - editAt), rate, kKeepBlock,
                                              [] (int, int) { return kKeepBlock; },
                                              [] (BlockInfo&) {});

            const bool after = sameFrom (typeEdit, editAt + fadeN, freshOut, fadeN, total - editAt - fadeN);
            const bool before = sameFrom (typeEdit, 0, unedited, 0, editAt);
            const bool differsFromOld = ! sameFrom (typeEdit, editAt, unedited, editAt, fadeN);
            const bool differsFromNew = ! sameFrom (typeEdit, editAt, freshOut, 0, fadeN);
            const bool ringing = ! sameFrom (unedited, editAt, freshOut, 0, total - editAt);

            row (after && before && differsFromOld && differsFromNew && ringing && allFinite (typeEdit),
                 "XR-rest bell 40 Hz Q 20 +24 dB ringing, type -> Low Shelf at sample 24000: from the fade's end memcmp-equal to a fresh instance on the new parameters started at the fade's first sample",
                 juce::String (after ? "" : "differs from the fresh instance after the fade (first at sample "
                                                 + juce::String (firstDifference (sliceOf (typeEdit, editAt, total - editAt), freshOut)) + " after the edit); ")
                     + (before ? "" : "differs before the edit; ")
                     + (differsFromOld ? "" : "control: the fade equals the outgoing render; ")
                     + (differsFromNew ? "" : "control: the fade equals the fresh instance; ")
                     + (ringing ? "" : "control: the ring does not matter; ")
                     + "N = " + juce::String (fadeN));
        }
    }

    //==========================================================================
    // XR-g4d, XR-g4e, XR-g4f, XR-g4n. G4 (d), (e), (f) on the float output
    //==========================================================================
    constexpr int kG4Length = 36000;            // 0.75 s at 48 kHz
    constexpr int kG4EventAt = 14400;           // 0.3 s: the event is seen by the first block starting there or later

    // 24 bells as fade_bench g4: 40 Hz x 1.3^b, Q 0.5 + 0.2 b.
    BandSpec g4BellSpec (int index, float gainDb)
    {
        return makeSpec (kBell, 40.0f * std::pow (1.3f, (float) index), gainDb, 0.5f + 0.2f * (float) index, 1, kStereo);
    }

    // 24 gain-bearing bands at +-12 dB: bells, shelves, tilt, every slope.
    BandSpec g4GainSpec (int index)
    {
        static const int types[4] = { kBell, kLowShelf, kHighShelf, kTilt };
        return makeSpec (types[index % 4], 40.0f * std::pow (1.3f, (float) index), (index % 2 == 1) ? 12.0f : -12.0f,
                         0.5f + 0.2f * (float) index, index % 8, kStereo);
    }

    struct EventRender
    {
        juce::AudioBuffer<float> output;
        int firedAt = -1;           // the first sample of the block that saw the event
    };

    /** A render of one pattern (a block size, or 0: the seeded random
        sequence) with one event, made by `edit` before the first block that
        starts at or after eventAt. viaBypassedAfter: that block and every
        later one arrive through processBlockBypassed. Main thread. */
    template <typename EditFn>
    EventRender renderEvent (Processor& p, const juce::AudioBuffer<float>& input, int pattern, int eventAt,
                             bool viaBypassedAfter, EditFn&& edit)
    {
        EventRender result;
        int fired = -1;

        const BlockHook hook = [&fired, &edit, eventAt, viaBypassedAfter] (BlockInfo& current)
        {
            if (fired < 0 && current.position >= eventAt)
            {
                fired = current.position;
                edit();
            }

            if (viaBypassedAfter && fired >= 0)
                current.viaBypassed = true;
        };

        result.output = pattern > 0 ? render (p, input, pattern, false, kRenderRate, hook)
                                    : render (p, input, kSequenceMax, false, kRenderRate, hook, randomBlockSequence (0x5E0BE9, kSequenceMax));
        result.firedAt = fired;
        return result;
    }

    /** Output memcmp-equal to the input from `from` to the end (false for a
        `from` outside the render). */
    bool equalFrom (const juce::AudioBuffer<float>& output, const juce::AudioBuffer<float>& input, int from)
    {
        return from >= 0 && from < input.getNumSamples() && sameFrom (output, from, input, from, input.getNumSamples() - from);
    }

    /** The last sample index at which any channel differs by bit pattern; -1 when none does. */
    int lastDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return a.getNumSamples();

        for (int i = a.getNumSamples(); --i >= 0;)
            for (int channel = 0; channel < a.getNumChannels(); ++channel)
                if (! sameBits (a.getReadPointer (channel)[i], b.getReadPointer (channel)[i]))
                    return i;

        return -1;
    }

    void transparencyRowsXr()
    {
        section ("XR-g4. G4 (d), (e), (f) on the float output (Phase 2.4: QUAL-01, FUNC-04, FUNC-05)");

        const int patterns[7] = { 1, 16, 64, 512, 1024, 4096, 0 };
        const int after200 = (int) std::lround (0.200 * kRenderRate);
        const int after100 = (int) std::lround (0.100 * kRenderRate);
        const int after5 = (int) std::lround (0.005 * kRenderRate);
        const int fadeN = eq::fadeLengthSamples (kRenderRate);

        // XR-g4d
        {
            Tally tally;
            juce::String figures;

            for (const int channels : { 1, 2 })
            {
                const auto input = makeNoiseAt (channels, kG4Length, 0x64D00000 + channels, kNoise12);

                for (const int pattern : patterns)
                {
                    const juce::String where = (channels == 1 ? juce::String ("mono ") : juce::String ("stereo ")) + patternName (pattern);

                    // (d0) enabled at exactly 0 dB from the first block.
                    {
                        auto p = makeBands (channels, 24, [] (int index) { return g4BellSpec (index, 0.0f); });
                        const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [] {});
                        tally.check (sameAudio (r.output, input) && activeBands (*p) == 24, where + ": 24 bells on at 0 dB from the start differ from the input");
                    }

                    // (d0) disabled, then enabled at exactly 0 dB at the event.
                    {
                        auto p = makeBands (channels, 24, [] (int index) { return g4BellSpec (index, 0.0f); });
                        const Handles h = handlesOf (*p);

                        for (const auto& hb : h.band)
                            hb.enabled->store (0.0f);

                        const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h]
                        {
                            for (const auto& hb : h.band)
                                hb.enabled->store (1.0f);
                        });

                        tally.check (sameAudio (r.output, input) && r.firedAt >= 0 && activeBands (*p) == 24,
                                     where + ": 24 bells enabled at 0 dB mid-render differ from the input");
                    }

                    // (d1) 24 bells 6 dB -> 0.
                    {
                        auto p = makeBands (channels, 24, [] (int index) { return g4BellSpec (index, 6.0f); });
                        const Handles h = handlesOf (*p);

                        const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h]
                        {
                            for (const auto& hb : h.band)
                                hb.gain->store (0.0f);
                        });

                        const bool acted = r.firedAt > 0 && ! sameFrom (r.output, 0, input, 0, r.firedAt);
                        const bool equal = r.firedAt >= 0 && equalFrom (r.output, input, r.firedAt + after200);
                        const int last = lastDifference (r.output, input);

                        tally.check (acted && equal, where + ": 24 bells 6 -> 0 dB"
                                                         + (acted ? "" : " (the bells did not act before the edit)")
                                                         + (equal ? "" : " differ 200 ms after the edit"));

                        if (channels == 2)
                            figures += (figures.isEmpty() ? "" : ", ") + patternName (pattern) + " "
                                     + fmtG (1000.0 * (double) (last + 1 - r.firedAt) / kRenderRate, 4) + " ms";
                    }
                }
            }

            row (tally.bad == 0 && tally.total == 42,
                 "XR-g4d G4 (d): 24 bells on at exactly 0 dB (from the start, and enabled mid-render) memcmp-equal to the input throughout; 24 bells 6 -> 0 dB memcmp-equal 200 ms after the edit (blocks 1 / 16 / 64 / 512 / 1024 / 4096 / random, mono and stereo)",
                 tally.detail() + "  stereo 6 -> 0 dB: equal from " + figures + " after the edit (bench 85.7 - 88.3)");
        }

        // XR-g4e
        {
            Tally tally;
            juce::String figures;
            const auto input = makeNoiseAt (2, kG4Length, 0x64E00002, kNoise12);

            for (const int pattern : patterns)
            {
                auto p = makeBands (2, 24, g4GainSpec);
                const Handles h = handlesOf (*p);

                const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h] { h.gainScale->store (0.0f); });

                const bool acted = r.firedAt > 0 && ! sameFrom (r.output, 0, input, 0, r.firedAt);
                const bool equal = r.firedAt >= 0 && equalFrom (r.output, input, r.firedAt + after200);
                const int last = lastDifference (r.output, input);

                tally.check (acted && equal, patternName (pattern) + (acted ? "" : " (the bands did not act before the edit)")
                                                 + (equal ? "" : ": differs 200 ms after gain_scale -> 0 %"));

                figures += (figures.isEmpty() ? "" : ", ") + patternName (pattern) + " "
                         + fmtG (1000.0 * (double) (last + 1 - r.firedAt) / kRenderRate, 4) + " ms";
            }

            row (tally.bad == 0 && tally.total == 7,
                 "XR-g4e G4 (e): 24 gain-bearing bands at +-12 dB (bells, shelves, tilt), gain_scale -> 0 %: memcmp-equal to the input 200 ms later (7 block patterns, stereo)",
                 tally.detail() + "  equal from " + figures + " after the edit (bench 87.0 - 89.1)");
        }

        // XR-g4f
        {
            Tally tally;
            const auto input = makeNoiseAt (2, kG4Length, 0x64F00002, kNoise12);

            for (const bool viaEntry : { false, true })
                for (const int pattern : patterns)
                {
                    auto p = makeBands (2, 24, g4GainSpec);
                    const Handles h = handlesOf (*p);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, viaEntry, [&h, viaEntry]
                    {
                        if (! viaEntry)
                            h.bypass->store (1.0f);
                    });

                    const bool equal = r.firedAt >= 0 && equalFrom (r.output, input, r.firedAt + after100);
                    const int last = lastDifference (r.output, input);
                    const bool exact = r.firedAt >= 0 && last + 1 == r.firedAt + fadeN;

                    tally.check (equal && exact, juce::String (viaEntry ? "processBlockBypassed, " : "parameter, ") + patternName (pattern)
                                                     + (equal ? "" : ": differs 100 ms after the toggle")
                                                     + (exact ? "" : ": equal from " + juce::String (last + 1 - r.firedAt)
                                                                         + " samples after the toggle, not N = " + juce::String (fadeN)));
                }

            row (tally.bad == 0 && tally.total == 14,
                 "XR-g4f G4 (f): 24 bands at +-12 dB, bypass on by the parameter and by processBlockBypassed: memcmp-equal 100 ms later, and equal from exactly N samples after the block that sees it (7 block patterns)",
                 tally.detail() + "  N = " + juce::String (fadeN));
        }

        // XR-g4n
        {
            Tally tally;
            const auto input = makeNoiseAt (2, kG4Length, 0x64A00002, kNoise12);

            for (const int pattern : patterns)
            {
                // (d) with one band at 0.001 dB instead of 0
                {
                    auto p = makeBands (2, 24, [] (int index) { return g4BellSpec (index, 6.0f); });
                    const Handles h = handlesOf (*p);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h]
                    {
                        for (size_t b = 0; b < h.band.size(); ++b)
                            h.band[b].gain->store (b == 0 ? 0.001f : 0.0f);
                    });

                    tally.check (r.firedAt >= 0 && ! equalFrom (r.output, input, r.firedAt + after200),
                                 "(d) one bell at 0.001 dB, " + patternName (pattern) + ": still memcmp-equal 200 ms later");
                }

                // (e) gain_scale to 0.001 / 12 of 100 %: the 12 dB bands at 0.001 dB
                {
                    auto p = makeBands (2, 24, g4GainSpec);
                    const Handles h = handlesOf (*p);
                    const auto scale = (float) (100.0 * 0.001 / 12.0);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, false, [&h, scale] { h.gainScale->store (scale); });

                    tally.check (r.firedAt >= 0 && ! equalFrom (r.output, input, r.firedAt + after200),
                                 "(e) gain_scale -> " + fmtG ((double) scale, 3) + " %, " + patternName (pattern) + ": still memcmp-equal 200 ms later");
                }

                // (f) read 5 ms after the toggle, both routes
                for (const bool viaEntry : { false, true })
                {
                    auto p = makeBands (2, 24, g4GainSpec);
                    const Handles h = handlesOf (*p);

                    const auto r = renderEvent (*p, input, pattern, kG4EventAt, viaEntry, [&h, viaEntry]
                    {
                        if (! viaEntry)
                            h.bypass->store (1.0f);
                    });

                    tally.check (r.firedAt >= 0 && ! equalFrom (r.output, input, r.firedAt + after5),
                                 juce::String ("(f) ") + (viaEntry ? "processBlockBypassed, " : "parameter, ") + patternName (pattern)
                                     + ": already memcmp-equal 5 ms after the toggle");
                }
            }

            row (tally.bad == 0 && tally.total == 28,
                 "XR-g4n (d) with one bell at 0.001 dB, (e) with gain_scale leaving the 12 dB bands at 0.001 dB, (f) read 5 ms after the toggle: each differs from the input (control fires)",
                 tally.detail());
        }
    }

    //==========================================================================
    // XR-g2. G2 live: slope changes on a running band (DSP-03)
    //==========================================================================
    /** The magnitude in dB of an impulse response (amplitude 1) at hz. */
    double irMagDb (const std::vector<float>& h, double rate, double hz)
    {
        return dftDb (h, 1.0, rate, { hz })[0];
    }

    /** G2s's crossing, on a measured response: the frequency in [fA, fB]
        where the magnitude passes targetDb, by bisection in log f. */
    bool irCrossing (const std::vector<float>& h, double rate, double fA, double fB, double targetDb, double& fOut)
    {
        double la = std::log (fA), lb = std::log (fB);
        double ea = irMagDb (h, rate, fA) - targetDb;
        const double eb = irMagDb (h, rate, fB) - targetDb;

        if (! std::isfinite (ea) || ! std::isfinite (eb) || (ea > 0.0) == (eb > 0.0))
            return false;

        for (int i = 0; i < 50; ++i)
        {
            const double lm = 0.5 * (la + lb);
            const double em = irMagDb (h, rate, std::exp (lm)) - targetDb;

            if ((em > 0.0) == (ea > 0.0))
            {
                la = lm;
                ea = em;
            }
            else
            {
                lb = lm;
            }
        }

        fOut = std::exp (0.5 * (la + lb));
        return true;
    }

    struct SlopeReading
    {
        bool ok = false;
        bool shortSpan = false;     // measured against the prototype (G2s-short)
        double cornerDb = 0.0;
        double slopeError = 0.0;    // dB/oct
        juce::String note;
    };

    /** One band at 500 Hz, Q 1, at slope `fromSlope`, mono, 48 kHz, blocks of
        512, noise; the slope set to `toSlope` at block 30; the noise stops at
        the fade's end; an impulse 100 ms later, its response read over 1 s. */
    SlopeReading runSlopeChange (int type, int fromSlope, int toSlope)
    {
        constexpr int blockSize = 512, changeBlock = 30;
        const double rate = kRenderRate;
        const double fc = 500.0;
        const int changeAt = changeBlock * blockSize;
        const int fadeEnd = changeAt + eq::fadeLengthSamples (rate);
        const int impulseAt = fadeEnd + (int) std::lround (0.100 * rate);
        const int irLength = (int) std::lround (rate);
        const int total = impulseAt + irLength;

        auto input = makeNoiseAt (1, total, 0x62200000 + type * 16 + toSlope, kNoise12);
        input.clear (0, fadeEnd, total - fadeEnd);
        input.setSample (0, impulseAt, 1.0f);

        auto p = makeBands (1, 1, [type, fromSlope, fc] (int) { return makeSpec (type, (float) fc, 0.0f, 1.0f, fromSlope, kStereo); });
        const Handles h = handlesOf (*p);

        const auto output = renderWith (*p, input, rate, blockSize,
                                        [] (int, int) { return blockSize; },
                                        [&h, toSlope] (BlockInfo& current)
                                        {
                                            if (current.index == changeBlock)
                                                h.band[0].slope->store ((float) toSlope);
                                        });

        SlopeReading r;

        if (! allFinite (output))
        {
            r.note = "not finite";
            return r;
        }

        const float* y = output.getReadPointer (0);
        const std::vector<float> ir (y + impulseAt, y + impulseAt + irLength);

        // G2s: -3.0103 dB at the corner (Q 1), the slope between the -20 and
        // -50 dB points against 6.02 N (harness-local table, not eq::slopeOrder).
        static const double nominalDbPerOct[8] = { 6.0, 12.0, 18.0, 24.0, 36.0, 48.0, 72.0, 96.0 };
        const double cornerTarget = 10.0 * std::log10 (0.5);
        const double nominal = 20.0 * std::log10 (2.0) * (nominalDbPerOct[toSlope] / 6.0);

        r.cornerDb = irMagDb (ir, rate, fc);
        const bool cornerOk = std::abs (r.cornerDb - cornerTarget) <= 0.10;

        double f20 = 0.0, f50 = 0.0;
        const bool lowCut = type == kLowCut;
        const bool measurable = lowCut ? (irCrossing (ir, rate, fc * 1.0e-3, fc, -20.0, f20) && irCrossing (ir, rate, fc * 1.0e-3, fc, -50.0, f50))
                                       : (irCrossing (ir, rate, fc, 20000.0, -20.0, f20) && irCrossing (ir, rate, fc, 20000.0, -50.0, f50));

        bool slopeOk = false;

        if (measurable)
        {
            const double slope = 30.0 / std::abs (std::log2 (f50 / f20));
            r.slopeError = std::abs (slope - nominal);
            slopeOk = r.slopeError <= 1.0;
        }
        else if (! lowCut)
        {
            // G2s-short: from the -20 dB point to 20 kHz, against the prototype.
            r.shortSpan = true;
            const double fEnd = 20000.0;
            double fStart = 0.0;

            if (irCrossing (ir, rate, fc, fEnd, -20.0, fStart) && std::log2 (fEnd / fStart) >= 1.0)
            {
                eq::RawBand raw = rawOfHandles (h.band[0]);
                const eq::BandInput in = eq::makeBandInput (raw, h.gainScale->load(), 0.0);
                const auto designer = designerFor (rate);
                eq::BandDesign design {};
                designer->design (in.settings, design);

                const double octaves = std::log2 (fEnd / fStart);
                const double wA = 2.0 * eq::pi * fStart / rate;
                const double wB = 2.0 * eq::pi * fEnd / rate;
                const double measuredSlope = (irMagDb (ir, rate, fStart) - irMagDb (ir, rate, fEnd)) / octaves;
                const double analogSlope = (eq::analogBandMagDb (design, wA) - eq::analogBandMagDb (design, wB)) / octaves;

                r.slopeError = std::abs (measuredSlope - analogSlope);
                slopeOk = r.slopeError <= 1.0;
            }
            else
            {
                r.note = "unmeasurable (span under one octave)";
            }
        }
        else
        {
            r.note = "a low cut without its -20 / -50 dB points";
        }

        r.ok = cornerOk && slopeOk;
        return r;
    }

    void liveSlopeRowsXr()
    {
        section ("XR-g2. G2 live: slope changes on a running band (Phase 2.4: DSP-03)");

        Tally tally;
        double worstCorner = 0.0, worstSlope = 0.0;
        int shortCount = 0;

        for (const int type : { kLowCut, kHighCut })
            for (int slope = 0; slope < 8; ++slope)
            {
                const int neighbour = slope > 0 ? slope - 1 : 1;
                const SlopeReading r = runSlopeChange (type, neighbour, slope);
                const double cornerError = std::abs (r.cornerDb - 10.0 * std::log10 (0.5));

                tally.check (r.ok, juce::String (kTypeNames[type]) + " " + juce::String (6 * eq::slopeOrder[slope]) + " dB/oct from "
                                       + juce::String (6 * eq::slopeOrder[neighbour]) + ": corner " + fmtG (r.cornerDb, 5) + " dB, slope error "
                                       + fmtG (r.slopeError, 3) + " dB/oct" + (r.shortSpan ? " (G2s-short)" : "")
                                       + (r.note.isEmpty() ? juce::String() : " " + r.note));

                worstCorner = juce::jmax (worstCorner, cornerError);
                worstSlope = juce::jmax (worstSlope, r.slopeError);
                shortCount += r.shortSpan ? 1 : 0;
            }

        row (tally.bad == 0 && tally.total == 16,
             "XR-g2 Low Cut and High Cut at 500 Hz, each of the 8 slopes reached from its neighbour on a running band; impulse 100 ms after the fade: corner -3.01 +- 0.10 dB; slope within 1.0 dB/oct (-20 / -50 dB points, else G2s-short against the prototype)",
             tally.detail() + "  worst corner error " + fmtG (worstCorner, 3) + " dB, worst slope error " + fmtG (worstSlope, 3)
                 + " dB/oct; " + juce::String (shortCount) + " cells measured G2s-short");
    }

    //==========================================================================
    // XR-byp, XR-bypn, XR-big, XR-prep. Changed while bypassed; VERIFICATION-2.3 issue 3
    //==========================================================================
    constexpr int kBypBands = 7;        // six mixed bands, and band 7 disabled until the edit

    /** The edits of one XR-byp variant. 0, 1: type, slope, placement, gain,
        frequency, Q and enable across the bands; 2: frequency, gain and Q
        only (SUMMARY-2.3 deviation 4); 3: a gain move on band 1 (made before
        the bypass, in the middle of whose ramp the bypass engages). */
    void applyBypEdits (const Handles& h, int variant)
    {
        if (variant == 3)
        {
            h.band[0].gain->store (-3.0f);
            return;
        }

        if (variant != 2)
        {
            h.band[0].type->store ((float) kHighShelf);     // Bell -> High Shelf
            h.band[1].slope->store (6.0f);                  // Low Shelf 36 -> 72 dB/oct
            h.band[2].placement->store ((float) kMid);      // High Shelf Right -> Mid
            h.band[6].enabled->store (1.0f);                // Band Pass enabled
        }

        h.band[2].gain->store (4.5f);                       // High Shelf -6 -> +4.5 dB
        h.band[3].freq->store (45.0f);                      // Low Cut 30 -> 45 Hz
        h.band[4].q->store (2.0f);                          // High Cut Q 0.7 -> 2
    }

    std::unique_ptr<Processor> makeBypBands()
    {
        auto p = makeBands (2, kBypBands, mixedSpec);
        setRaw (*p, bandId (kBypBands, "enabled"), 0.0f);
        return p;
    }

    void bypassRowsXr()
    {
        section ("XR-byp / XR-big / XR-prep. changed while bypassed; oversize host blocks; re-prepare (Phase 2.4; VERIFICATION-2.3 issue 3)");

        // XR-byp, XR-bypn
        {
            constexpr int blockSize = 512, rampBlock = 9, bypassBlock = 10, editBlock = 14, resumeBlock = 20;
            const int fadeN = eq::fadeLengthSamples (kRenderRate);
            const auto input = makeNoiseAt (2, 36 * blockSize, 0x6B790002, kNoise12);
            const int total = input.getNumSamples();
            const int bypassAt = bypassBlock * blockSize;
            const int resumeAt = resumeBlock * blockSize;
            const char* const names[4] = { "bypass parameter, structural + continuous edits", "processBlockBypassed, structural + continuous edits",
                                           "bypass parameter, continuous edits only", "bypass parameter, engaged in the middle of a gain ramp" };

            Tally tally, control;

            for (int variant = 0; variant < 4; ++variant)
            {
                const bool viaEntry = variant == 1;
                auto p = makeBypBands();
                const Handles h = handlesOf (*p);

                const auto output = render (*p, input, blockSize, false, kRenderRate, [&h, variant, viaEntry] (BlockInfo& current)
                {
                    if (variant == 3 && current.index == rampBlock)
                        applyBypEdits (h, variant);

                    if (current.index == bypassBlock && ! viaEntry)
                        h.bypass->store (1.0f);

                    if (current.index == editBlock && variant != 3)
                        applyBypEdits (h, variant);

                    if (current.index == resumeBlock && ! viaEntry)
                        h.bypass->store (0.0f);

                    if (viaEntry)
                        current.viaBypassed = current.index >= bypassBlock && current.index < resumeBlock;
                });

                // Fresh instances started at the un-bypass sample: on the edited
                // parameters (XR-byp) and on those from before (XR-bypn).
                auto edited = makeBypBands();
                applyBypEdits (handlesOf (*edited), variant);
                const auto editedOut = render (*edited, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

                auto before = makeBypBands();
                const auto beforeOut = render (*before, sliceOf (input, resumeAt, total - resumeAt), blockSize, false);

                const bool bypassed = sameFrom (output, bypassAt + fadeN, input, bypassAt + fadeN, resumeAt - bypassAt - fadeN);
                const bool resumed = sameFrom (output, resumeAt + fadeN, editedOut, fadeN, total - resumeAt - fadeN);
                const bool stale = sameFrom (output, resumeAt + fadeN, beforeOut, fadeN, total - resumeAt - fadeN);

                tally.check (bypassed && resumed,
                             juce::String (names[variant]) + (bypassed ? "" : ": the bypassed span is not the input from the toggle + N")
                                 + (resumed ? "" : ": from un-bypass + N differs from a fresh instance on the edited parameters"));

                control.check (! stale, juce::String (names[variant]) + ": equals a fresh instance on the parameters from before the "
                                            + (variant == 3 ? "gain move" : "bypass"));
            }

            row (tally.bad == 0 && tally.total == 4,
                 "XR-byp six mixed bands, noise: edited while fully bypassed (type, slope, placement, gain, frequency, Q, enable; or continuous only; or bypass engaged mid-ramp), both routes: from un-bypass + N memcmp-equal to a fresh instance on the edited parameters started at the un-bypass sample",
                 tally.detail() + "  N = " + juce::String (fadeN));

            row (control.bad == 0 && control.total == 4,
                 "XR-bypn the same compare against a fresh instance on the parameters from before the bypass (variant 4: before the gain move) differs (control fires)",
                 control.detail());
        }

        // XR-big (VERIFICATION-2.3 issue 3: the tick phase across chunks)
        {
            constexpr int hostBlock = 4096;
            const auto input = makeNoiseAt (2, 14 * hostBlock, 0x6B160002, kNoise12);

            const auto script = [] (Processor& target)
            {
                return [&target] (BlockInfo& current)
                {
                    switch (current.index)
                    {
                        case 2:  setRaw (target, bandId (1, "gain"), -3.0f);
                                 setRaw (target, bandId (2, "type"), (float) kBell);  break;
                        case 4:  setRaw (target, bandId (3, "slope"), 7.0f);
                                 setRaw (target, bandId (4, "placement"), (float) kSide);
                                 setRaw (target, bandId (5, "freq"), 15000.0f);  break;
                        case 6:  setRaw (target, bandId (6, "enabled"), 0.0f);
                                 setRaw (target, "bypass", 1.0f);  break;
                        case 8:  setRaw (target, "bypass", 0.0f);
                                 setRaw (target, bandId (6, "enabled"), 1.0f);  break;
                        case 10: setRaw (target, bandId (1, "freq"), 700.0f);
                                 setRaw (target, bandId (2, "q"), 3.0f);  break;
                        default: break;
                    }
                };
            };

            auto a = makeBands (2, 6, mixedSpec);
            auto b = makeBands (2, 6, mixedSpec);

            const auto oversized = render (*a, input, hostBlock, false, kRenderRate, script (*a), {}, 512);
            const auto prepared = render (*b, input, hostBlock, false, kRenderRate, script (*b));
            const bool acts = ! sameAudio (prepared, input);

            row (acts && sameAudio (oversized, prepared) && allFinite (prepared),
                 "XR-big host blocks of 4096 on a prepared size of 512, 6 bands with moves, structural events and a bypass toggle: bit-identical to the same script prepared at 4096",
                 sameAudio (oversized, prepared) ? juce::String (acts ? "" : "the setting does not act on the audio")
                                                 : "first differs at sample " + juce::String (firstDifference (oversized, prepared)));
        }

        // XR-prep (VERIFICATION-2.3 issue 3: the clock and the bands after a re-prepare)
        {
            constexpr int blockSize = 500;
            const double rate = kRenderRate;
            const auto first = makeNoiseAt (2, (int) std::lround (rate), 0x6B700002, kNoise12);
            const auto second = makeNoiseAt (2, (int) std::lround (0.5 * rate), 0x6B710002, kNoise12);
            const auto blocks500 = [] (int, int) { return blockSize; };

            // In flight at the end of the first render: a type fade and a gain ramp.
            const auto inFlight = [] (const Handles& target)
            {
                target.band[0].type->store ((float) kHighShelf);
                target.band[1].gain->store (3.0f);
            };

            const auto moves = [] (const Handles& target, int index)
            {
                if (index == 3)
                    target.band[2].gain->store (2.0f);

                if (index == 7)
                    target.band[3].freq->store (50.0f);

                if (index == 11)
                    target.band[4].type->store ((float) kLowShelf);
            };

            auto reused = makeBands (2, 6, mixedSpec);
            const Handles hr = handlesOf (*reused);

            const auto firstOut = renderWith (*reused, first, rate, blockSize, blocks500, [&hr, &inFlight] (BlockInfo& current)
            {
                if (current.index == 95)
                    inFlight (hr);
            });

            const auto secondOut = renderWith (*reused, second, rate, blockSize, blocks500, [&hr, &moves] (BlockInfo& current)
            {
                moves (hr, current.index);
            });

            auto fresh = makeBands (2, 6, mixedSpec);
            const Handles hf = handlesOf (*fresh);
            inFlight (hf);

            const auto freshOut = renderWith (*fresh, second, rate, blockSize, blocks500, [&hf, &moves] (BlockInfo& current)
            {
                moves (hf, current.index);
            });

            reused->releaseResources();
            fresh->releaseResources();

            const bool same = sameAudio (secondOut, freshOut);
            const bool acts = ! sameAudio (secondOut, second) && allFinite (firstOut);

            row (same && acts,
                 "XR-prep re-prepare at the same rate after 1 s (a type fade and a gain ramp in flight), then two moves and one type change in blocks of 500: memcmp-equal to a fresh instance",
                 (same ? juce::String() : "first differs at sample " + juce::String (firstDifference (secondOut, freshOut)) + "; ")
                     + (acts ? "" : "the bands do not act"));
        }
    }

    //==========================================================================
    // XR-blk. Block identity with structural events (PLAN-2.4 P9; fade_bench blk)
    //==========================================================================
    struct BlkBand
    {
        float type = 0.0f, slope = 1.0f, placement = 0.0f, enabled = 1.0f;
        float freq = 1000.0f, gain = 0.0f, q = 1.0f;
    };

    struct BlkEvent
    {
        BlkBand bands[6];
        float scale = 100.0f;
        float bypass = 0.0f;
    };

    /** XF-blk's script: 160 events; at each, every band redraws type and
        slope (p 0.5), placement (0.3), enable (0.3), frequency, gain (exactly
        0 with p 0.25) and Q; the global bypass toggles (p 0.1); and here also
        gain_scale (p 0.3, 0 - 200 %). The state carries from event to event. */
    std::vector<BlkEvent> makeStructuralEvents()
    {
        Xorshift draw;
        draw.state = 777;

        BlkEvent current;
        std::vector<BlkEvent> events;

        for (int e = 0; e < 160; ++e)
        {
            for (auto& c : current.bands)
            {
                if (draw.uniform() < 0.5)
                {
                    c.type = (float) juce::jmin (7, (int) (draw.uniform() * 8.0));
                    c.slope = (float) juce::jmin (7, (int) (draw.uniform() * 8.0));
                }

                if (draw.uniform() < 0.3)
                    c.placement = (float) juce::jmin (4, (int) (draw.uniform() * 5.0));

                if (draw.uniform() < 0.3)
                    c.enabled = draw.uniform() > 0.3 ? 1.0f : 0.0f;

                c.freq = juce::jlimit (10.0f, 30000.0f, (float) std::exp (std::log (10.0) + draw.uniform() * std::log (3000.0)));
                c.gain = draw.uniform() < 0.25 ? 0.0f : juce::jlimit (-30.0f, 30.0f, (float) (-30.0 + 60.0 * draw.uniform()));
                c.q = juce::jlimit (0.025f, 40.0f, (float) std::exp (std::log (0.025) + draw.uniform() * std::log (1600.0)));
            }

            if (draw.uniform() < 0.1)
                current.bypass = current.bypass >= 0.5f ? 0.0f : 1.0f;

            if (draw.uniform() < 0.3)
                current.scale = juce::jlimit (0.0f, 200.0f, (float) (200.0 * draw.uniform()));

            events.push_back (current);
        }

        return events;
    }

    void storeBlkEvent (const Handles& h, const BlkEvent& e)
    {
        h.gainScale->store (e.scale);
        h.bypass->store (e.bypass);

        for (size_t b = 0; b < 6; ++b)
        {
            const BlkBand& c = e.bands[b];
            const BandHandles& hb = h.band[b];
            hb.type->store (c.type);
            hb.slope->store (c.slope);
            hb.placement->store (c.placement);
            hb.enabled->store (c.enabled);
            hb.bypass->store (0.0f);
            hb.freq->store (c.freq);
            hb.gain->store (c.gain);
            hb.q->store (c.q);
        }
    }

    // withEvents false: event 0 at sample 0 and nothing after it.
    juce::AudioBuffer<float> structuralBlockRender (Processor& p, const Handles& h, const juce::AudioBuffer<float>& input,
                                                    double rate, int blockSize, const std::vector<BlkEvent>& events, bool withEvents)
    {
        storeBlkEvent (h, events[0]);

        return renderWith (p, input, rate, blockSize,
                           [blockSize] (int, int) { return blockSize; },
                           [&h, &events, withEvents] (BlockInfo& current)
                           {
                               if (current.position % 2048 != 0)
                                   return;

                               const auto slot = (size_t) (current.position / 2048);

                               if (slot >= events.size() || (! withEvents && slot > 0))
                                   return;

                               storeBlkEvent (h, events[slot]);
                           });
    }

    void structuralBlockRowsXr()
    {
        section ("XR-blk. block identity with structural events (Phase 2.4: PLAN-2.4 P9)");

        constexpr int length = 2048 * 160;
        const double rates[4] = { 44100.0, 48000.0, 96000.0, 192000.0 };
        const int sizes[4] = { 32, 64, 512, 2048 };
        const auto input = makeNoiseAt (2, length, 0x6B1C0002, kNoise12);
        const auto events = makeStructuralEvents();

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
            instances.push_back (makeBands (2, 6, mixedSpec));
            handles.push_back (handlesOf (*instances.back()));
        }

        std::vector<juce::AudioBuffer<float>> outputs (jobs.size());

        parallelFor ((int) jobs.size(), [&instances, &handles, &jobs, &outputs, &input, &events] (int j)
        {
            const auto slot = (size_t) j;
            outputs[slot] = structuralBlockRender (*instances[slot], handles[slot], input, jobs[slot].rate,
                                                   jobs[slot].blockSize, events, jobs[slot].withEvents);
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

            tally.check (! sameAudio (outputs[base + 4], outputs[base + 2]), where + ": the render without events is the same (the events do not bite)");
        }

        row (tally.bad == 0 && tally.total == 20,
             "XR-blk six bands, 160 events on multiples of 2048 (type, slope, placement, enable, f, gain incl. exactly 0, Q, gain_scale, bypass): blocks 32 / 64 / 512 / 2048 bit-identical at 44.1 / 48 / 96 / 192 kHz; differs from the render without events",
             tally.detail());
    }

    //==========================================================================
    // A3, A3n. G6 with fades (PERF-01)
    //==========================================================================
    struct FadeAllocResult
    {
        AllocResult base;
        int viaEntryBlocks = 0;
    };

    /** The window is processBlock or processBlockBypassed alone, the first
        block after prepareToPlay included; the draw, the parameter stores, the
        fill and the scan stay outside it (stressLoop24). 20 s of the P14
        every-block draw at 48 kHz, every second bypassed span through
        processBlockBypassed. `plant`: one allocation inside the first window. */
    FadeAllocResult runFadeAllocWindow (bool plant)
    {
        FadeAllocResult result;

       #if defined (__APPLE__)
        const double rate = 48000.0;
        const double seconds = 20.0;

        const StressSeed chosen = chooseStressSeed24 (rate, seconds, 1);
        auto p = makeBands (2, 0, mixedSpec);
        const Handles h = handlesOf (*p);

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        allocArmed = true;
        {
            void* volatile probe = std::malloc (64);
            std::free (probe);
        }
        allocArmed = false;
        result.base.probeAllocs = allocCount;
        result.base.probeFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        bool plantNext = plant;
        int viaEntry = 0;

        const StressOutcome outcome = stressLoop24 (*p, h, rate, seconds, chosen.seed, 1, true,
                                                    [&p, &plantNext, &viaEntry] (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool viaBypassed)
                                                    {
                                                        allocArmed = true;

                                                        if (viaBypassed)
                                                            p->processBlockBypassed (buffer, midiMessages);
                                                        else
                                                            p->processBlock (buffer, midiMessages);

                                                        if (plantNext)
                                                        {
                                                            void* volatile planted = std::malloc (64);
                                                            std::free (planted);
                                                        }

                                                        allocArmed = false;
                                                        plantNext = false;

                                                        if (viaBypassed)
                                                            ++viaEntry;
                                                    });

        malloc_logger = previous;
        p->releaseResources();

        result.base.available = true;
        result.base.allocs = allocCount;
        result.base.frees = freeCount;
        result.base.blocks = (int) outcome.blocks;
        result.base.audioOk = chosen.ok && outcome.nonFinite == 0 && outcome.actingBlocks > 0;
        result.viaEntryBlocks = viaEntry;
       #else
        juce::ignoreUnused (plant);
       #endif

        return result;
    }

    void fadeAllocationRowsXr()
    {
        section ("A3. allocation with fades and bypass (Phase 2.4: G6, PERF-01)");

        const FadeAllocResult clean = runFadeAllocWindow (false);

        if (! clean.base.available)
        {
            info ("A3 allocation gate SKIPPED: no malloc_logger on this platform");
            info ("A3n allocation control SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = clean.base.probeAllocs >= 1 && clean.base.probeFrees >= 1;

        row (hookLive && clean.base.audioOk && clean.viaEntryBlocks > 0 && clean.base.allocs == 0 && clean.base.frees == 0,
             "A3 processBlock and processBlockBypassed, 20 s of the P14 every-block draw at 48 kHz (structure at every block, bypass toggles by both routes), first block included: 0 allocations, 0 frees",
             juce::String (clean.base.allocs) + " allocations, " + juce::String (clean.base.frees) + " frees in "
                 + juce::String (clean.base.blocks) + " windows (" + juce::String (clean.viaEntryBlocks)
                 + " through processBlockBypassed; malloc_logger, this thread); hook " + (hookLive ? "live" : "DEAD")
                 + " (probe: " + juce::String (clean.base.probeAllocs) + " / " + juce::String (clean.base.probeFrees) + ")"
                 + (clean.base.audioOk ? "" : "; the stress did not run as expected (stack guard, non-finite output, or no acting block)"));

        const FadeAllocResult planted = runFadeAllocWindow (true);

        row (planted.base.allocs >= 1 && planted.base.frees >= 1,
             "A3n a planted allocation inside the processBlock / processBlockBypassed window is counted (control fires)",
             juce::String (planted.base.allocs) + " allocations, " + juce::String (planted.base.frees) + " frees");
    }

    //==========================================================================
    // X-cpu3. CPU with fades (INFO; PLAN-2.4 P17; fade_bench cpu, RESEARCH-2.4 Q8)
    //==========================================================================
    void cpuInfoFading()
    {
        section ("X3. CPU with fades (INFO, not gated; G10 binds in Stage 4)");

        constexpr int blockSize = 512;
        const double rate = 48000.0;
        const int numBlocks = (int) (20.0 * rate / (double) blockSize);
        const double audioSeconds = (double) numBlocks * (double) blockSize / rate;
        const auto noise = makeNoiseAt (2, blockSize, 0x0C9400002, kNoise12);

        struct Config { const char* name; int type; int slope; int type2; int slope2; const char* bench; };

        const Config configs[] =
        {
            { "24 cuts x 96 dB/oct (Low Cut <> High Cut)",       kLowCut,    7, kHighCut,   7, "2.65 / 5.17 / 2.71 / -" },
            { "24 shelves x 96 dB/oct (High <> Low)",            kHighShelf, 7, kLowShelf,  7, "4.93 / 9.88 / 5.06 / 0.04" },
            { "24 bells <> 12 dB/oct shelves",                   kBell,      0, kHighShelf, 1, "0.67 / 1.74 / 0.87 / 0.04" },
            { "24 shelves 96 <> 72 dB/oct",                      kHighShelf, 7, kHighShelf, 6, "4.97 / 8.39 / 4.26 / 0.04" }
        };

        const char* const modeNames[4] = { "settled", "always fading", "one fade per second", "24 asleep" };

        for (const auto& c : configs)
        {
            double percent[4] = { 0.0, 0.0, 0.0, 0.0 };
            bool finite = true;

            for (int mode = 0; mode < 4; ++mode)
            {
                auto p = makeBands (2, 24, [&c, mode] (int index)
                {
                    return makeSpec (c.type, 40.0f * std::pow (1.28f, (float) index), mode == 3 ? 0.0f : 6.0f, 1.0f, c.slope, kStereo);
                });

                const Handles h = handlesOf (*p);
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                double seconds = 0.0;

                p->prepareToPlay (rate, blockSize);

                for (int k = 0; k < numBlocks; ++k)
                {
                    const double t = (double) (k * blockSize) / rate;
                    bool second = false;

                    if (mode == 1)
                        second = (((long) (t / 0.0215)) & 1) != 0;       // a new request as soon as each fade has ended

                    if (mode == 2)
                        second = (((long) t) & 1) != 0;

                    for (const auto& hb : h.band)
                    {
                        hb.type->store ((float) (second ? c.type2 : c.type));
                        hb.slope->store ((float) (second ? c.slope2 : c.slope));
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

            juce::String line = juce::String ("X-cpu3 48 kHz stereo, 512-sample blocks, 20 s, ") + c.name + ":";

            for (int mode = 0; mode < 4; ++mode)
                line += juce::String (" ") + modeNames[mode] + " " + fmtG (percent[mode], 3) + " %" + (mode < 3 ? ";" : "");

            info (line + " of one core (bench settled / always fading / one per second / asleep: " + c.bench
                      + " %; compare X-cpu2 settled above)" + (finite ? "" : "  OUTPUT NOT FINITE"));
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

    // Stage 2, Phase 2.4 (PLAN-2.4 T7)
    stepRowsXr();
    fadeReferenceRowsXr();
    transparencyRowsXr();
    liveSlopeRowsXr();
    bypassRowsXr();
    structuralBlockRowsXr();
    fadeAllocationRowsXr();
    cpuInfoFading();

    std::printf ("\n%d PASS, %d FAIL\n", passes, failures);
    std::fflush (stdout);
    return juce::jmin (failures, 255);
}
