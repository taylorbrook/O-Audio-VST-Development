/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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

    O-AnalogEQ — DSP render harness (v1.5.2)

    Gates the three DSP fixes from the v1.5.0 code review, plus the preset-save
    guard added in v1.5.2. Every DSP verdict is read off RENDERED AUDIO, never
    off a flag or a pointer: a bypass that "took" because a boolean changed is
    not a bypass that sounds right (pattern_pointer_moved_is_not_audio_changed).

    G1 — CR-02. isBusesLayoutSupported must REFUSE asymmetric negotiation.
         Before the fix the base class returned true for everything, so a host
         offering 2 in / 1 out got four ProcessorDuplicators prepared with ONE
         mono filter each while JUCE sized the buffer at max(in,out) = 2, and
         ProcessorDuplicator::process indexed processors[1] out of range on an
         OwnedArray — nullptr, dereferenced on the audio thread, Release only.
         Driving (2,2) alone would pass on the broken build too, so the
         asymmetric pair is what carries this gate
         (pattern_gate_stimulus_below_threshold_is_vacuous).

    G2 — WR-05. output_gain must RAMP. Measured on a DC bed with all four bands
         off, because then the gain stage is the only thing in the chain that
         can produce a nonzero first difference: the expected smoothed step and
         the unsmoothed jump differ by the ramp length, which is ~1440 samples,
         so the two hypotheses are three orders of magnitude apart and the
         threshold is DERIVED from the ramp rather than copied from another
         plugin (pattern_zipper_gate_absolute_step_is_gain_staging_dependent).

    G3 — WR-06. Band on/off must crossfade, and a bypassed band must still be
         fed. Two separate failures, one mechanism:
           G3a the falling edge does not click — max|first difference| through
               the toggle stays near the signal's own slew, which is measured on
               this same render rather than assumed.
           G3b re-enabling after a loud passage does not burst. The old code
               left z-1/z-2 holding loud samples through the whole quiet passage;
               a fed filter has tracked the quiet signal and has nothing to dump.

    G4 — WR-07 follow-up. The factory-preset guard. Not an audio gate: the
         quantity is a refusal string, and it is asserted directly rather than
         through a proxy because the value the dialog branches on IS the
         predicate. It lives in PresetSaveGuard.h precisely so this target can
         reach it without compiling the WebView editor TU.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PresetSaveGuard.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 64;
    constexpr float  kRampSecs   = 0.03f;      // kSmoothingSeconds in the processor

    int failures = 0;
    int passes   = 0;

    void check (bool ok, const juce::String& what, const juce::String& detail = {})
    {
        if (ok) { ++passes; std::printf ("  PASS: %s\n", what.toRawUTF8()); }
        else
        {
            ++failures;
            std::printf ("  FAIL: %s%s%s\n", what.toRawUTF8(),
                         detail.isEmpty() ? "" : " — ",
                         detail.isEmpty() ? "" : detail.toRawUTF8());
        }
    }

    void setParam (OuariconAnalogEQAudioProcessor& p, const juce::String& id, float normalised)
    {
        if (auto* prm = p.parameters.getParameter (id))
            prm->setValueNotifyingHost (normalised);
        else
            check (false, "parameter '" + id + "' exists");
    }

    // Largest absolute sample-to-sample step in [from, to). This is the quantity
    // a click IS: a discontinuity is a first difference far outside what the
    // signal itself can produce at its own frequency and level.
    float maxFirstDiff (const std::vector<float>& x, size_t from, size_t to)
    {
        float m = 0.0f;
        to = std::min (to, x.size());
        for (size_t i = std::max<size_t> (from, 1); i < to; ++i)
            m = std::max (m, std::abs (x[i] - x[i - 1]));
        return m;
    }

    float peakIn (const std::vector<float>& x, size_t from, size_t to)
    {
        float m = 0.0f;
        to = std::min (to, x.size());
        for (size_t i = from; i < to; ++i) m = std::max (m, std::abs (x[i]));
        return m;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::printf ("\nO-AnalogEQ DSP render harness (v1.5.2)\n");
    std::printf ("sample rate %.0f Hz, block %d\n\n", kSampleRate, kBlockSize);

    // ════════════════════════════════════════════════════════════════════════
    // G1 — CR-02: asymmetric bus layouts are refused
    // ════════════════════════════════════════════════════════════════════════
    std::printf ("-- G1 (CR-02) bus layout negotiation\n");
    {
        OuariconAnalogEQAudioProcessor proc;

        auto layoutFor = [] (int nIn, int nOut)
        {
            juce::AudioProcessor::BusesLayout l;
            l.inputBuses .add (juce::AudioChannelSet::canonicalChannelSet (nIn));
            l.outputBuses.add (juce::AudioChannelSet::canonicalChannelSet (nOut));
            return l;
        };

        // The symmetric pair must still be accepted, or the fix has broken the
        // plugin rather than secured it.
        check (proc.checkBusesLayoutSupported (layoutFor (2, 2)),
               "[G1] stereo in / stereo out is ACCEPTED");
        check (proc.checkBusesLayoutSupported (layoutFor (1, 1)),
               "[G1] mono in / mono out is ACCEPTED");

        // THE ASSERTIONS THAT CARRY THIS GATE. Both were true on the broken
        // build, and (2,1) is the exact config that null-dereferenced.
        check (! proc.checkBusesLayoutSupported (layoutFor (2, 1)),
               "[G1] stereo in / MONO out is REFUSED (the CR-02 crash config)");
        check (! proc.checkBusesLayoutSupported (layoutFor (1, 2)),
               "[G1] mono in / STEREO out is REFUSED");
    }

    // ════════════════════════════════════════════════════════════════════════
    // G2 — WR-05: output_gain ramps rather than stepping
    // ════════════════════════════════════════════════════════════════════════
    std::printf ("\n-- G2 (WR-05) output_gain zipper\n");
    {
        OuariconAnalogEQAudioProcessor proc;

        // All bands OFF and analog OFF, so the chain is dry -> outputGain and
        // nothing else in it can create a first difference. A DC bed then makes
        // the gain ramp the ONLY signal present, which is what lets the
        // threshold be derived instead of guessed.
        for (auto* id : { "lf_on", "lmf_on", "hmf_on", "hf_on", "analog" })
            setParam (proc, id, 0.0f);
        setParam (proc, "output_gain", 0.5f);            // 0 dB

        proc.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
        proc.prepareToPlay (kSampleRate, kBlockSize);

        constexpr float kDC = 0.5f;
        const int totalBlocks = 200;
        const int stepBlock   = 60;

        std::vector<float> out;
        out.reserve (static_cast<size_t> (totalBlocks * kBlockSize));

        juce::AudioBuffer<float> buf (2, kBlockSize);
        juce::MidiBuffer midi;

        for (int b = 0; b < totalBlocks; ++b)
        {
            // Preset "Surgical Cut" moves output_gain 0.5 -> 0.542, i.e.
            // 0 dB -> +1.008 dB. That is the real step the review measured.
            if (b == stepBlock) setParam (proc, "output_gain", 0.542f);

            for (int ch = 0; ch < 2; ++ch)
                juce::FloatVectorOperations::fill (buf.getWritePointer (ch), kDC, kBlockSize);

            proc.processBlock (buf, midi);
            const auto* r = buf.getReadPointer (0);
            for (int i = 0; i < kBlockSize; ++i) out.push_back (r[i]);
        }

        const size_t stepAt = static_cast<size_t> (stepBlock * kBlockSize);
        const float  before = out[stepAt - 4];
        const float  after  = out[out.size() - 1];

        // NON-VACUITY FIRST. If the gain never actually moved, every smoothness
        // assertion below would pass on a signal that did nothing.
        const float expectedAfter = kDC * juce::Decibels::decibelsToGain (1.008f);
        check (std::abs (before - kDC) < 1.0e-3f,
               "[G2] the bed really is at unity before the step",
               juce::String (before, 6));
        check (std::abs (after - expectedAfter) < 2.0e-3f,
               "[G2] the gain really did reach +1.008 dB — the step is not a no-op",
               "got " + juce::String (after, 6) + ", expected " + juce::String (expectedAfter, 6));

        // DERIVED THRESHOLD. Over a kRampSecs ramp the per-sample increment is
        // (deltaGain / rampSamples) * DC; unsmoothed it is deltaGain * DC in ONE
        // sample. Those differ by rampSamples (1440 here), so a 10x allowance
        // over the smoothed figure still sits ~100x below the broken build.
        const float rampSamples  = kRampSecs * static_cast<float> (kSampleRate);
        const float deltaGain    = juce::Decibels::decibelsToGain (1.008f) - 1.0f;
        const float smoothedStep = kDC * deltaGain / rampSamples;
        const float jumpStep     = kDC * deltaGain;
        const float allowed      = smoothedStep * 10.0f;

        const float measured = maxFirstDiff (out, stepAt - kBlockSize, out.size());

        std::printf ("     ramp %.0f samples | smoothed/sample %.3e | unsmoothed jump %.3e | measured %.3e\n",
                     rampSamples, smoothedStep, jumpStep, measured);

        check (measured < allowed,
               "[G2] max per-sample step through the gain change is ramp-sized, not jump-sized",
               "measured " + juce::String (measured, 8) + " vs allowed " + juce::String (allowed, 8)
                 + " (a zero-ramp Gain would read " + juce::String (jumpStep, 8) + ")");

        // The separation is what makes the verdict mean something: assert the
        // gate could actually tell the two hypotheses apart on this stimulus.
        check (jumpStep > allowed * 10.0f,
               "[G2] the gate SEPARATES ramped from stepped by >10x — it is not passing by "
               "measuring something too small to distinguish them",
               "jump " + juce::String (jumpStep, 8) + " vs allowed " + juce::String (allowed, 8));
    }

    // ════════════════════════════════════════════════════════════════════════
    // G3 — WR-06: band bypass crossfades, and a bypassed band is still fed
    // ════════════════════════════════════════════════════════════════════════
    std::printf ("\n-- G3 (WR-06) band on/off crossfade + stale state\n");
    {
        // ---- G3a: the falling edge does not click ---------------------------
        {
            OuariconAnalogEQAudioProcessor proc;

            for (auto* id : { "lmf_on", "hmf_on", "hf_on", "analog" })
                setParam (proc, id, 0.0f);
            setParam (proc, "lf_on",   1.0f);
            setParam (proc, "lf_gain", 1.0f);      // full boost: the biggest shelf to remove
            setParam (proc, "lf_freq", 0.577f);
            setParam (proc, "output_gain", 0.5f);

            proc.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
            proc.prepareToPlay (kSampleRate, kBlockSize);

            const int totalBlocks = 400;
            const int toggleBlock = 200;
            const double freq = 80.0;              // inside the LF shelf
            const float  amp  = 0.5f;

            std::vector<float> out;
            out.reserve (static_cast<size_t> (totalBlocks * kBlockSize));

            juce::AudioBuffer<float> buf (2, kBlockSize);
            juce::MidiBuffer midi;
            double phase = 0.0;
            const double inc = 2.0 * juce::MathConstants<double>::pi * freq / kSampleRate;

            for (int b = 0; b < totalBlocks; ++b)
            {
                if (b == toggleBlock) setParam (proc, "lf_on", 0.0f);

                for (int i = 0; i < kBlockSize; ++i)
                {
                    const float s = amp * static_cast<float> (std::sin (phase));
                    phase += inc;
                    buf.setSample (0, i, s);
                    buf.setSample (1, i, s);
                }

                proc.processBlock (buf, midi);
                const auto* r = buf.getReadPointer (0);
                for (int i = 0; i < kBlockSize; ++i) out.push_back (r[i]);
            }

            const size_t toggleAt = static_cast<size_t> (toggleBlock * kBlockSize);
            const size_t rampLen  = static_cast<size_t> (kRampSecs * kSampleRate);

            // SELF-CALIBRATING BASELINE. The sine's own slew is measured on this
            // very render, in a steady window before the toggle, so the verdict
            // does not depend on a hard-coded number that a level or frequency
            // change would quietly invalidate.
            const float baseline = maxFirstDiff (out, toggleAt - 4 * rampLen, toggleAt - rampLen);
            const float through  = maxFirstDiff (out, toggleAt - 8, toggleAt + rampLen + 8);

            std::printf ("     steady slew %.3e | through the toggle %.3e\n", baseline, through);

            check (baseline > 1.0e-4f,
                   "[G3a] the steady baseline is non-trivial — the gate is measuring real signal",
                   juce::String (baseline, 8));
            check (through < baseline * 1.5f,
                   "[G3a] switching a boosted LF shelf OFF adds no discontinuity — max step through "
                   "the toggle stays within 1.5x the signal's own slew",
                   "through " + juce::String (through, 8) + " vs baseline "
                     + juce::String (baseline, 8));
        }

        // ---- G3b: no stale-state burst on re-enable -------------------------
        {
            OuariconAnalogEQAudioProcessor proc;

            for (auto* id : { "lmf_on", "hmf_on", "hf_on", "analog" })
                setParam (proc, id, 0.0f);
            setParam (proc, "lf_on",   1.0f);
            setParam (proc, "lf_gain", 1.0f);
            setParam (proc, "lf_freq", 0.577f);
            setParam (proc, "output_gain", 0.5f);

            proc.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
            proc.prepareToPlay (kSampleRate, kBlockSize);

            const double freq = 80.0;
            const double inc  = 2.0 * juce::MathConstants<double>::pi * freq / kSampleRate;
            double phase = 0.0;

            juce::AudioBuffer<float> buf (2, kBlockSize);
            juce::MidiBuffer midi;
            std::vector<float> out;

            // Phase A loud + band ON, phase B quiet + band OFF (long enough that
            // a FED filter has fully forgotten the loud passage), phase C quiet +
            // band back ON. The old code carried A's samples in z-1/z-2 straight
            // across B and dumped them into C.
            const int blocksA = 300, blocksB = 300, blocksC = 200;
            const float loud = 0.9f, quiet = 0.02f;

            auto render = [&] (int blocks, float amp)
            {
                for (int b = 0; b < blocks; ++b)
                {
                    for (int i = 0; i < kBlockSize; ++i)
                    {
                        const float s = amp * static_cast<float> (std::sin (phase));
                        phase += inc;
                        buf.setSample (0, i, s);
                        buf.setSample (1, i, s);
                    }
                    proc.processBlock (buf, midi);
                    const auto* r = buf.getReadPointer (0);
                    for (int i = 0; i < kBlockSize; ++i) out.push_back (r[i]);
                }
            };

            render (blocksA, loud);
            setParam (proc, "lf_on", 0.0f);
            render (blocksB, quiet);
            const size_t reEnableAt = out.size();
            setParam (proc, "lf_on", 1.0f);
            render (blocksC, quiet);

            // Settled quiet level measured at the END of phase C, after the
            // crossfade has completed, so the reference is the level the band is
            // SUPPOSED to produce rather than the dry level.
            const float settled = peakIn (out, out.size() - 40 * kBlockSize, out.size());
            const float burst   = peakIn (out, reEnableAt, reEnableAt + static_cast<size_t> (0.03 * kSampleRate));
            const float loudRef = peakIn (out, static_cast<size_t> ((blocksA - 40) * kBlockSize),
                                               static_cast<size_t> (blocksA * kBlockSize));

            std::printf ("     loud phase peak %.4f | settled quiet peak %.5f | first 30 ms after re-enable %.5f\n",
                         loudRef, settled, burst);

            check (loudRef > 0.5f && settled > 0.0f,
                   "[G3b] the stimulus really does swing loud then quiet — the contrast exists",
                   "loud " + juce::String (loudRef, 5) + ", settled " + juce::String (settled, 6));

            check (burst <= settled * 1.5f,
                   "[G3b] re-enabling the band after a LOUD passage produces no burst — the first "
                   "30 ms stays within 1.5x the settled quiet level, because a band that is always "
                   "fed holds no stale z-1/z-2",
                   "burst " + juce::String (burst, 6) + " vs settled " + juce::String (settled, 6));
        }
    }

    // ════════════════════════════════════════════════════════════════════════
    // G4 — WR-07 follow-up: the factory-preset guard savePresetToFile() lacks
    // ════════════════════════════════════════════════════════════════════════
    //
    // v1.5.1 closed WR-07 by swapping savePreset(name) -> savePresetToFile(file).
    // That fixed the discarded directory and silently dropped the
    // isFactoryPreset() early-return savePreset() carried, because the two APIs
    // are not interchangeable. v1.5.2 re-imposes it at the call site through
    // oaeq::presetSaveRefusal.
    //
    // Gated on the PREDICATE, which is the value the dialog callback branches
    // on, rather than on a downstream proxy for it. The callback itself lives
    // in the editor TU, which JUCE_WEB_BROWSER=0 keeps out of this target — the
    // reason the predicate was lifted into its own header at all.
    std::printf ("\n-- G4 (WR-07 follow-up) factory-preset save guard\n");
    {
        OuariconAnalogEQAudioProcessor proc;
        const auto& pm = proc.presetManager;

        const auto factoryDir = pm.getFactoryPresetsDirectory();
        const auto userDir    = pm.getUserPresetsDirectory();

        // NON-VACUITY FIRST. Every assertion below is about a real factory
        // preset name; if the bank is empty they all pass over nothing.
        juce::String factoryName;
        for (const auto& n : pm.getPresetList())
            if (pm.isFactoryPreset (n)) { factoryName = n; break; }

        check (factoryName.isNotEmpty(),
               "[G4] the factory bank is non-empty — without a real factory name every "
               "assertion below would be vacuous",
               "using \"" + factoryName + "\"");

        if (factoryName.isNotEmpty())
        {
            // Failure 1: lands in User/, returns true, and loadPreset() never
            // reaches it because it searches Factory/ first.
            check (oaeq::presetSaveRefusal (userDir.getChildFile (factoryName + ".json"), pm).isNotEmpty(),
                   "[G4] a FACTORY name saved into the User library is REFUSED — loadPreset() "
                   "searches Factory/ first, so the write would be unreachable forever");

            // The same target typed without an extension must be judged
            // identically: savePresetToFile() appends .json itself, so a guard
            // that skipped the fixup would wave through the path it just refused.
            check (oaeq::presetSaveRefusal (userDir.getChildFile (factoryName), pm).isNotEmpty(),
                   "[G4] ... and so is that same target typed WITHOUT the .json extension");

            // Failure 2: overwriting the factory JSON outright.
            check (oaeq::presetSaveRefusal (factoryDir.getChildFile (factoryName + ".json"), pm).isNotEmpty(),
                   "[G4] writing into the Factory directory is REFUSED");

            check (oaeq::presetSaveRefusal (factoryDir.getChildFile ("sub").getChildFile ("x.json"), pm).isNotEmpty(),
                   "[G4] ... including anywhere beneath it, not only directly in it");

            // THE OVER-BLOCKING ARM, and the reason the guard is location-aware.
            // WR-07 exists to make arbitrary-path export work; a name-only
            // isFactoryPreset() reject would refuse this and re-break it.
            // Nothing outside the two library directories can shadow anything.
            check (oaeq::presetSaveRefusal (
                       juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)
                           .getChildFile (factoryName + ".json"), pm).isEmpty(),
                   "[G4] exporting under a factory NAME to a directory outside the library is "
                   "ALLOWED — location-aware, so the WR-07 fix is not re-broken");
        }

        // The common path. A guard that simply refused everything would pass
        // every arm above; this is what separates a class from a blanket.
        check (oaeq::presetSaveRefusal (userDir.getChildFile ("My Mix Bus EQ.json"), pm).isEmpty(),
               "[G4] an ordinary user preset name in the User library is ALLOWED");
    }

    std::printf ("\n%s   (%d passed)\n\n",
                 failures == 0 ? "== ALL CHECKS PASSED ==" : "== CHECKS FAILED ==", passes);
    return failures == 0 ? 0 : 1;
}
