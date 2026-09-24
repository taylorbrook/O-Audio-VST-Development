/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    fx_mod_nan_check.cpp — O-Prism v1.27.1 modulation finiteness gate.

    Origin. The v1.27.0 CHANGELOG left an open finding: "routing any source to
    Reverb/Delay/Chorus/Dist Mix or Master Vol at full amount produces NaN on
    the FIRST block", blamed on juce::jlimit passing NaN through. That
    diagnosis was wrong on both counts, and this gate is the result of chasing
    it properly.

    What was actually happening. getSampleRate() is read in exactly ONE place
    in the whole plugin — processBlock, feeding advanceGlobalLfoPhases. It is
    set by AudioProcessor::setPlayConfigDetails, NOT by prepareToPlay. Two of
    O-Prism's four console gates (edit_rotation_check, wavetable_cow_check)
    called only prepareToPlay, so they rendered every block with
    getSampleRate() == 0. That makes rateHz/sampleRate infinite, and the wrap
    `phase -= std::floor (phase)` turns one infinity into a NaN, because
    inf - floor(inf) is NaN. The NaN is STICKY: every later block adds to it.

    globalLfoPhase then feeds fxLfo[], whose output is the LFO1-4 source of
    the PROCESSOR-level matrix — and those five destinations are precisely the
    ones consumed from fxModMatrix in processBlock. The other twenty are
    voice-level, read from the voice's own matrix and its own LFOs, which are
    prepared from prepareToPlay's ARGUMENT and so never saw a zero rate. That
    is the whole of "only these five". jlimit was a bystander: it never had a
    finite value to clamp. No host reaches this — every JUCE wrapper publishes
    the rate before the first block — so the original report was a harness
    artifact, not a user-reachable bug.

    v1.27.1 fixes both halves: advanceGlobalLfoPhases guards the rate and the
    increment (and recovers a phase that is already non-finite instead of
    staying poisoned), and the two gates now declare the rate like a host.

    What this gate asserts:

      [A] Under the host contract, EVERY one of the 25 destinations renders
          finite and still audible with a source routed at full amount. This
          is the report's own claim, tested across the whole enum rather than
          the five that happened to show it.

      [B] Every one of the 10 sources into each of the 5 processor-level
          destinations renders finite — 50 combinations. [A] fixes the source
          and sweeps destinations; this fixes the destination and sweeps
          sources, so neither axis is assumed.

      [C] THE NEGATIVE CONTROL, and the reason this gate is not vacuous. An
          instance that never declares its rate — getSampleRate() == 0,
          exactly what the two gates used to do — must STILL render finite
          audio and keep finite LFO phases. This fails on pre-v1.27.1 code
          without touching a line of source: it is the guard's own contract,
          not a reverted-file experiment that someone has to remember to run.

      [D] Recovery, the half prevention does not cover. Render blocks at rate
          0, THEN declare a real rate. D1 publishes the rate and nothing else:
          the phases must become finite and advance, where pre-fix they are
          NaN and NaN plus a finite increment is still NaN. D2 lets the host
          re-prepare as a real one would — prepareToPlay refills the phases
          with 0, so pre-fix they LOOK recovered while the output does not,
          because the NaN has reached masterVolSmoothed and the effect tails
          and neither is cleared there. D2 is the one that shows why the fix
          has to be at the source: downstream state keeps the poison after
          the phases themselves are clean.

      [E] Tempo sync, all 18 divisions, finite. The sync arm computes rateHz
          as 1/(beats*60/bpm) and so has its own division; a zero in
          kDivBeats or a zero bpm would land in the same wrap.

      [F] Non-vacuity: the five processor-level routes must actually CHANGE
          the rendered audio. Without this, [A] and [B] would pass just as
          happily on a build where fxModMatrix was dead and every offset was
          a hard zero — which is the one other way to make a NaN go away.

    Usage:  O-Prism-fx-mod-nan-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/ModulationMatrix.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;
constexpr int    kNumBlocks  = 8;

// Well clear of both the ~0.04 floor the quietest routed configuration
// renders at and anything denormal.
constexpr double kAudibleRms = 1.0e-4;

int failures = 0;

void check (bool condition, const juce::String& what)
{
    if (condition)
    {
        std::cout << "  ok    " << what << "\n";
    }
    else
    {
        std::cout << "  FAIL  " << what << "\n";
        ++failures;
    }
}

struct Render
{
    bool   finite   = true;
    int    badBlock = -1;
    double rms      = 0.0;
};

struct Probe
{
    std::unique_ptr<juce::AudioProcessor> proc;
    OPrismAudioProcessor* prism = nullptr;

    /** declareRate mirrors what a host does: setPlayConfigDetails publishes
        the rate to AudioProcessor::getSampleRate(); prepareToPlay does not.
        Passing false is [C]'s negative control, not an oversight. */
    explicit Probe (bool declareRate = true)
    {
        proc.reset (createPluginFilter());
        prism = dynamic_cast<OPrismAudioProcessor*> (proc.get());

        if (prism == nullptr)
            return;

        if (declareRate)
            prism->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);

        prism->prepareToPlay (kSampleRate, kBlockSize);
    }

    void setP (const juce::String& id, float plain)
    {
        if (auto* p = prism->getAPVTS().getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (plain));
        else
            check (false, "parameter exists: " + id);
    }

    void route (int source, int dest, float amount = 1.0f)
    {
        setP ("modSlot0Src", static_cast<float> (source));
        setP ("modSlot0Dst", static_cast<float> (dest));
        setP ("modSlot0Amt", amount);
        setP ("modSlot0On",  1.0f);
    }

    Render render (int numBlocks = kNumBlocks, bool noteOn = true)
    {
        juce::AudioBuffer<float> buf (2, kBlockSize);
        juce::MidiBuffer midi;

        if (noteOn)
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);

        Render r;
        double sum = 0.0;
        int    n   = 0;

        for (int b = 0; b < numBlocks; ++b)
        {
            buf.clear();
            prism->processBlock (buf, midi);
            midi.clear();

            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                for (int i = 0; i < buf.getNumSamples(); ++i)
                {
                    const float v = buf.getSample (ch, i);

                    if (! std::isfinite (v))
                    {
                        if (r.finite)
                        {
                            r.finite   = false;
                            r.badBlock = b;
                        }
                    }
                    else
                    {
                        sum += static_cast<double> (v) * v;
                        ++n;
                    }
                }
        }

        r.rms = n > 0 ? std::sqrt (sum / static_cast<double> (n)) : 0.0;
        return r;
    }

    bool phasesFinite() const
    {
        for (int i = 0; i < 4; ++i)
            if (! std::isfinite (prism->getGlobalLfoPhase (i)))
                return false;

        return true;
    }
};

juce::String destName (int d) { return getModDestNames()[d]; }
juce::String srcName  (int s) { return getModSourceNames()[s]; }

// The five destinations consumed from the PROCESSOR-level matrix in
// processBlock rather than inside the voice — the set the v1.27.0 report
// named, derived from the enum so a new one cannot silently escape.
const int kProcessorDests[] = {
    static_cast<int> (ModDest::ReverbMix),
    static_cast<int> (ModDest::DelayMix),
    static_cast<int> (ModDest::ChorusMix),
    static_cast<int> (ModDest::DistMix),
    static_cast<int> (ModDest::MasterVol)
};
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\n═══ O-Prism FX modulation finiteness gate (v1.27.1) ═══\n";

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[A] Every destination renders finite and audible (source = LFO1, amount +1)\n";

    for (int d = 1; d < static_cast<int> (ModDest::NumDests); ++d)
    {
        Probe p;

        if (p.prism == nullptr)
        {
            check (false, "[A] createPluginFilter() returned an OPrismAudioProcessor");
            return failures;
        }

        p.route (static_cast<int> (ModSource::LFO1), d);
        const auto r = p.render();

        check (r.finite,
               "[A] " + destName (d) + " renders finite"
                   + (r.finite ? juce::String()
                               : " (first non-finite block " + juce::String (r.badBlock) + ")"));

        check (r.rms > kAudibleRms,
               "[A] " + destName (d) + " still audible (rms " + juce::String (r.rms, 6) + ")");

        check (p.phasesFinite(), "[A] " + destName (d) + " leaves the global LFO phases finite");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[B] Every source into each processor-level destination renders finite\n";

    for (const int d : kProcessorDests)
    {
        for (int s = 1; s < static_cast<int> (ModSource::NumSources); ++s)
        {
            Probe p;
            p.route (s, d);
            const auto r = p.render();

            check (r.finite, "[B] " + srcName (s) + " -> " + destName (d) + " renders finite");
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[C] NEGATIVE CONTROL — an undeclared sample rate must not poison anything\n";

    {
        Probe p (false); // getSampleRate() == 0, exactly what the old gates did

        check (p.prism->getSampleRate() == 0.0,
               "[C] the control really does leave getSampleRate() at 0 (got "
                   + juce::String (p.prism->getSampleRate()) + ") — not a vacuous pass");

        p.route (static_cast<int> (ModSource::LFO1), static_cast<int> (ModDest::ReverbMix));
        const auto r = p.render();

        check (p.phasesFinite(),
               "[C] global LFO phases stay finite at rate 0 (phase0 "
                   + juce::String (p.prism->getGlobalLfoPhase (0), 6) + ")");

        check (r.finite,
               "[C] output stays finite at rate 0"
                   + (r.finite ? juce::String()
                               : " (first non-finite block " + juce::String (r.badBlock) + ")"));

        check (r.rms > kAudibleRms,
               "[C] the instrument still renders at rate 0 (rms " + juce::String (r.rms, 6)
                   + ") — the voices take the rate from prepareToPlay's argument");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[D] Recovery — a bad rate must not poison the instance permanently\n";

    // D1: the rate is published late and NOTHING else is re-initialised. This
    // is the phase state on its own — pre-fix it is NaN by now and NaN plus a
    // finite increment is still NaN, so the LFOs never come back.
    {
        Probe p (false);
        p.route (static_cast<int> (ModSource::LFO1), static_cast<int> (ModDest::ReverbMix));
        p.render(); // blocks under the bad rate

        p.prism->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
        const auto r = p.render();

        check (p.phasesFinite(),
               "[D1] phases are finite once a real rate arrives, with no re-prepare (phase0 "
                   + juce::String (p.prism->getGlobalLfoPhase (0), 6) + ")");

        check (p.prism->getGlobalLfoPhase (0) > 0.0,
               "[D1] and they ADVANCE rather than sitting frozen at 0");

        check (r.finite, "[D1] output is finite once a real rate arrives");
        check (r.rms > kAudibleRms,
               "[D1] and audible (rms " + juce::String (r.rms, 6) + ")");
    }

    // D2: the same, but the host does a full re-prepare — which is what a real
    // one would do. prepareToPlay refills globalLfoPhase with 0, so the phases
    // alone look recovered even pre-fix; the output does NOT, because the NaN
    // has by then reached masterVolSmoothed and the effect tails, and neither
    // is cleared by prepareToPlay. That is what made the poisoning permanent
    // rather than merely a bad block, and it is why the guard has to keep the
    // NaN from ever being produced instead of mopping it up downstream.
    {
        Probe p (false);
        p.route (static_cast<int> (ModSource::LFO1), static_cast<int> (ModDest::ReverbMix));
        p.render();

        p.prism->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
        p.prism->prepareToPlay (kSampleRate, kBlockSize);

        const auto r = p.render();

        check (r.finite, "[D2] output is finite after a full re-prepare");
        check (r.rms > kAudibleRms,
               "[D2] and audible after a full re-prepare (rms " + juce::String (r.rms, 6)
                   + ") — pre-fix this stayed at 0 forever");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[E] Tempo sync — all 18 divisions finite\n";

    for (int div = 0; div < 18; ++div)
    {
        Probe p;
        p.setP ("lfo1Sync", 1.0f);
        p.setP ("lfo1Division", static_cast<float> (div));
        p.route (static_cast<int> (ModSource::LFO1), static_cast<int> (ModDest::ReverbMix));

        const auto r = p.render();

        check (r.finite && p.phasesFinite(),
               "[E] division " + juce::String (div) + " finite (phase0 "
                   + juce::String (p.prism->getGlobalLfoPhase (0), 6) + ")");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[F] Non-vacuity — the processor-level routes actually change the audio\n";

    {
        Probe base;
        const auto reference = base.render();

        check (reference.finite && reference.rms > kAudibleRms,
               "[F] unrouted reference is finite and audible (rms "
                   + juce::String (reference.rms, 6) + ")");

        for (const int d : kProcessorDests)
        {
            Probe p;
            p.route (static_cast<int> (ModSource::LFO1), d);
            const auto r = p.render();

            const double rel = std::abs (r.rms - reference.rms)
                             / std::max (reference.rms, 1.0e-12);

            check (rel > 0.005,
                   "[F] " + destName (d) + " changed the rendered level (rms "
                       + juce::String (reference.rms, 6) + " -> " + juce::String (r.rms, 6)
                       + ", " + juce::String (rel * 100.0, 2) + "%)");
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string (failures))
              << "\n\n";

    return failures;
}
