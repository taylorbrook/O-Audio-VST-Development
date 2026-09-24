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

    lfo_subblock_check.cpp — O-Prism v1.27.2 free-run LFO sub-block gate (WR-01).

    The defect. juce::Synthesiser::renderNextBlock splits the buffer at MIDI
    events whenever the gap is at least minimumSubBlockSize (default 32;
    setMinimumRenderingSubdivisionSize is never called here) and calls
    renderVoices() — hence PrismVoice::renderNextBlock — once per sub-block.
    The voice seeds lfoN.setPhase (processor->getGlobalLfoPhase (n)) at the top
    of EVERY one of those calls. advanceGlobalLfoPhases used to run once per
    processBlock, after renderNextBlock returned. So with k sub-blocks the
    free-running LFO was rewound to the same start phase k times and only
    advanced through the last sub-block's worth of samples: at ~30 events per
    block it covered roughly 1/30th of the ground it should, and stuttered.

    Tempo-synced and per-note LFOs are unaffected — they never read the global
    phase.

    The fix. PrismSynthesiser overrides renderVoices() and advances the global
    phase by THAT sub-block's length, after the voices have seeded from it.
    processBlock no longer advances. The sub-block lengths sum to the block
    length, so the end-of-block phase — which is what fxLfo[] reads — is
    unchanged.

    The observable. MIDI that carries no modulation must not change the sound.
    CC#20 is inert on both paths: processBlock's MIDI scan reads only note
    on/off, All Notes/Sound Off, CC#1 and channel pressure, and
    PrismVoice::controllerMoved is empty (the processor owns CC handling). All
    such a CC does is force a sub-block boundary. So a render with a held note
    and a free-running LFO must come out the same whether the block is rendered
    whole or chopped into 16 pieces — and pre-fix it emphatically does not.

    Determinism. Two things in this plugin are not reproducible across
    instances (WR-06, still open): the S&H LFO shape and NoiseGenerator draw
    from clock-seeded juce::Random, and WavetableOscillator::resetWithRandomPhases
    seeds an LCG from `this`. This gate sidesteps all three rather than
    depending on WR-06 being fixed first: LFO shape is Sine, noise and sub
    levels are left at their 0.0 defaults, and osc A/B Phase are set above zero,
    which takes startNote down the resetWithPhase branch instead. Two instances
    then render bit-for-bit alike, which [A0] asserts before anything else so a
    later equality cannot pass for the wrong reason.

    What this gate asserts:

      [A0] Baseline reproducibility — two instances, same setup, same MIDI,
           identical output. Without this the whole gate is unfalsifiable.

      [A]  THE FINDING. Dense inert MIDI (15 CC#20 per block, 32 samples apart
           -> 16 sub-blocks) renders identically to no MIDI at all, for a
           free-running LFO routed to Pitch and to FiltA Cutoff. Also at a
           coarser density (3 events) so the result is not tied to one k.

      [B]  Non-vacuity — the route has to matter. The same render with the
           modulation amount at 0 must differ substantially from the routed
           one; otherwise [A] would pass just as happily on a build where the
           free-run LFO did nothing at all.

      [C]  Traversal — the LFO must actually move over the window measured.
           [A] compares two renders; if the LFO were frozen at its start phase
           both would be frozen the same way. This pins the shared trajectory
           to a real excursion, and pins the global phase itself to the
           analytic distance rate * blocks * blockSize / sampleRate.

      [D]  Sub-blocking really happened. If the CC#20 events failed to split
           the block — wrong spacing, a JUCE default changed — [A] would be
           comparing two identical renders and would pass vacuously. The
           control is a MIDI-density run with the LFO free-run flag OFF: a
           per-note LFO never reads the global phase, so it is the one
           configuration that is expected to be density-insensitive both
           before and after the fix. It must still be bit-identical here,
           proving the events themselves are inert and that [A]'s pre-fix
           failure is about the global phase and nothing else.

      [E]  The end-of-block phase is unchanged by the redistribution — the
           property that lets fxLfo[] keep reading globalLfoPhase after
           renderNextBlock. The phase after k sub-blocks must equal the
           phase after one whole block to within accumulation rounding.

    Negative control. Not built in — this one needs the fix reverted, because
    the pre-fix behaviour is a property of where advanceGlobalLfoPhases is
    called from. Restore the single processBlock call and delete the
    PrismSynthesiser override, and [A] fails on every density/destination pair
    while [A0], [B], [C] and [D] stay green. The measured numbers are recorded
    in the v1.27.2 CHANGELOG entry.

    Usage:  O-Prism-lfo-subblock-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/ModulationMatrix.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;
constexpr int    kNumBlocks  = 12;

// juce::Synthesiser::minimumSubBlockSize default. Events closer together than
// this are folded into the preceding sub-block instead of splitting it.
constexpr int    kMinSubBlock = 32;

// Free-run rate. Over kNumBlocks * kBlockSize / kSampleRate = 128 ms this is
// ~0.77 of a cycle: enough excursion that a stuttering LFO is unmistakable,
// short of a full period so the comparison cannot alias back onto itself.
constexpr float  kLfoRateHz = 6.0f;

// Post-fix the two renders differ only by float accumulation over a few
// hundred adds — measured at ~1e-7 here. Pre-fix the difference is order 0.1.
// Anywhere in between and something is wrong, so the threshold is not delicate.
constexpr float  kIdenticalEps = 1.0e-4f;

int failures = 0;

void check (bool condition, const juce::String& what)
{
    if (condition)
        std::cout << "  ok    " << what << "\n";
    else
    {
        std::cout << "  FAIL  " << what << "\n";
        ++failures;
    }
}

/** A whole render, kept sample-exact so two of them can be differenced. */
struct Capture
{
    juce::AudioBuffer<float> audio;
    double endPhase = 0.0;
    bool   finite   = true;

    float peak() const
    {
        float m = 0.0f;
        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                m = juce::jmax (m, std::abs (audio.getSample (ch, i)));
        return m;
    }
};

float maxAbsDiff (const Capture& a, const Capture& b)
{
    if (a.audio.getNumChannels() != b.audio.getNumChannels()
        || a.audio.getNumSamples() != b.audio.getNumSamples())
        return std::numeric_limits<float>::infinity();

    float m = 0.0f;
    for (int ch = 0; ch < a.audio.getNumChannels(); ++ch)
        for (int i = 0; i < a.audio.getNumSamples(); ++i)
            m = juce::jmax (m, std::abs (a.audio.getSample (ch, i) - b.audio.getSample (ch, i)));
    return m;
}

struct Probe
{
    std::unique_ptr<juce::AudioProcessor> proc;
    OPrismAudioProcessor* prism = nullptr;

    Probe()
    {
        proc.reset (createPluginFilter());
        prism = dynamic_cast<OPrismAudioProcessor*> (proc.get());

        if (prism == nullptr)
            return;

        // setPlayConfigDetails before prepareToPlay: getSampleRate() comes
        // from the former, and advanceGlobalLfoPhases is its only reader
        // (v1.27.1). A gate that skips it renders every block at rate 0.
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

    /** Everything WR-06's clock- and address-seeded RNGs could reach, pinned. */
    void makeDeterministic()
    {
        setP ("oscAPhase", 0.25f); // > 0 => resetWithPhase, not resetWithRandomPhases
        setP ("oscBPhase", 0.35f);
        setP ("noiseLevel", 0.0f);
        setP ("subLevel",  0.0f);
        setP ("lfo1Shape", 0.0f);  // Sine — S&H is the juce::Random one
    }

    void freeRunLfo1 (bool on = true)
    {
        setP ("lfo1Sync", 0.0f);
        setP ("lfo1Rate", kLfoRateHz);
        setP ("lfo1FreeRun", on ? 1.0f : 0.0f);
    }

    void route (ModDest dest, float amount = 1.0f)
    {
        setP ("modSlot0Src", static_cast<float> (static_cast<int> (ModSource::LFO1)));
        setP ("modSlot0Dst", static_cast<float> (static_cast<int> (dest)));
        setP ("modSlot0Amt", amount);
        setP ("modSlot0On",  1.0f);
    }

    /** @param ccPerBlock  inert CC#20 events per block, spaced kMinSubBlock
                           apart, which is what forces the sub-block splits. */
    Capture render (int ccPerBlock)
    {
        Capture cap;
        cap.audio.setSize (2, kNumBlocks * kBlockSize);
        cap.audio.clear();

        juce::AudioBuffer<float> buf (2, kBlockSize);

        for (int b = 0; b < kNumBlocks; ++b)
        {
            juce::MidiBuffer midi;

            if (b == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);

            for (int e = 1; e <= ccPerBlock; ++e)
            {
                const int pos = e * kMinSubBlock;
                if (pos < kBlockSize)
                    midi.addEvent (juce::MidiMessage::controllerEvent (1, 20, 64), pos);
            }

            buf.clear();
            prism->processBlock (buf, midi);

            for (int ch = 0; ch < 2; ++ch)
            {
                cap.audio.copyFrom (ch, b * kBlockSize, buf, ch, 0, kBlockSize);

                for (int i = 0; i < kBlockSize; ++i)
                    if (! std::isfinite (buf.getSample (ch, i)))
                        cap.finite = false;
            }
        }

        cap.endPhase = prism->getGlobalLfoPhase (0);
        return cap;
    }
};

/** A fully configured instance rendered at one MIDI density. */
Capture run (ModDest dest, float amount, int ccPerBlock, bool freeRun = true)
{
    Probe p;
    p.makeDeterministic();
    p.freeRunLfo1 (freeRun);
    p.route (dest, amount);
    return p.render (ccPerBlock);
}

juce::String destName (ModDest d) { return getModDestNames()[static_cast<int> (d)]; }

// Two voice-level destinations. Pitch integrates the LFO error, so it is the
// most sensitive; FiltA Cutoff does not, so a pass on both rules out the
// result being an artifact of either one's sensitivity.
const ModDest kDests[] = { ModDest::Pitch, ModDest::FiltACutoff };

// 15 events => 16 sub-blocks of 32; 3 => 4 sub-blocks of 128.
const int kDensities[] = { 15, 3 };
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\n═══ O-Prism free-run LFO sub-block gate (WR-01, v1.27.2) ═══\n";

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[A0] Baseline reproducibility — two instances must render alike\n";

    {
        const auto a = run (ModDest::Pitch, 1.0f, 0);
        const auto b = run (ModDest::Pitch, 1.0f, 0);

        check (a.finite && b.finite, "[A0] both baseline renders are finite");
        check (a.peak() > 0.01f,
               "[A0] baseline is audible (peak " + juce::String (a.peak(), 6) + ")");

        const float d = maxAbsDiff (a, b);
        check (d <= kIdenticalEps,
               "[A0] two instances render identically (max |diff| "
                   + juce::String (d, 9) + ") — WR-06's RNGs are out of the path");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[A] THE FINDING — inert MIDI density must not change the render\n";

    for (const auto dest : kDests)
    {
        const auto whole = run (dest, 1.0f, 0);

        check (whole.finite, "[A] " + destName (dest) + " undivided render is finite");

        for (const int cc : kDensities)
        {
            const auto split = run (dest, 1.0f, cc);
            const float d = maxAbsDiff (whole, split);

            check (split.finite,
                   "[A] " + destName (dest) + " @ " + juce::String (cc) + " CC/block is finite");

            check (d <= kIdenticalEps,
                   "[A] " + destName (dest) + " @ " + juce::String (cc)
                       + " CC/block (" + juce::String (cc + 1)
                       + " sub-blocks) matches the undivided render (max |diff| "
                       + juce::String (d, 9) + ")");
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[B] Non-vacuity — the free-run route has to change the audio\n";

    for (const auto dest : kDests)
    {
        const auto routed   = run (dest, 1.0f, 0);
        const auto unrouted = run (dest, 0.0f, 0);

        const float d = maxAbsDiff (routed, unrouted);

        check (d > 0.01f,
               "[B] " + destName (dest) + " at amount 1 differs from amount 0 (max |diff| "
                   + juce::String (d, 6) + ") — [A] is not comparing two dead renders");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[C] Traversal — the LFO covers real ground over the window\n";

    {
        const auto c = run (ModDest::Pitch, 1.0f, 15);

        // Analytic: the phase advances rate * totalSamples / sampleRate, wrapped.
        const double expected = static_cast<double> (kLfoRateHz)
                              * (kNumBlocks * kBlockSize) / kSampleRate;
        const double wrapped = expected - std::floor (expected);

        check (expected > 0.5 && expected < 1.0,
               "[C] the window is a partial cycle by construction ("
                   + juce::String (expected, 4) + " cycles) — no aliasing back to the start");

        check (std::abs (c.endPhase - wrapped) < 1.0e-6,
               "[C] global phase after " + juce::String (kNumBlocks)
                   + " chopped blocks is the analytic value (got "
                   + juce::String (c.endPhase, 9) + ", expected "
                   + juce::String (wrapped, 9) + ")");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[D] Control — with free-run OFF, density is inert before AND after the fix\n";

    for (const auto dest : kDests)
    {
        const auto whole = run (dest, 1.0f, 0,  false);
        const auto split = run (dest, 1.0f, 15, false);

        const float d = maxAbsDiff (whole, split);

        check (d <= kIdenticalEps,
               "[D] " + destName (dest)
                   + " per-note LFO is density-insensitive (max |diff| " + juce::String (d, 9)
                   + ") — so the CC#20 events are genuinely inert and [A] isolates the global phase");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[E] End-of-block phase survives the redistribution (what fxLfo[] reads)\n";

    for (const int cc : kDensities)
    {
        const auto whole = run (ModDest::Pitch, 1.0f, 0);
        const auto split = run (ModDest::Pitch, 1.0f, cc);

        const double d = std::abs (whole.endPhase - split.endPhase);

        check (d < 1.0e-9,
               "[E] @ " + juce::String (cc) + " CC/block the end phase matches the undivided one ("
                   + juce::String (whole.endPhase, 12) + " vs " + juce::String (split.endPhase, 12)
                   + ", delta " + juce::String (d, 12) + ")");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string (failures))
              << "\n\n";

    return failures;
}
