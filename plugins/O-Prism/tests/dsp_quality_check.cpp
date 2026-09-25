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

    dsp_quality_check.cpp — O-Prism v1.28.0 DSP-quality gate.

    Covers the five Warning-tier findings closed in v1.28.0. Each section is
    self-contained and states its own observable; three of the five carry a
    BUILT-IN negative control (the pre-fix arithmetic reproduced inside this
    file), so they fail on a build where the fix is absent without anyone
    reverting a source file.

    ── WR-06  Two clock-seeded RNGs made any render non-reproducible ─────────

    LFO::random (Sample & Hold) and NoiseGenerator::randomL/randomR were
    default-constructed juce::Random, whose constructor calls setSeedRandomly()
    — high-resolution ticks, wall clock and the object address. Every
    instantiation drew a different stream, so two bounces of the same project
    with S&H on an LFO or any noise level above zero differed sample-for-sample,
    and the plugin could never have a byte-stable offline render gate at all.
    WavetableOscillator::resetWithRandomPhases was the milder third case: an
    LCG seeded from `this`, stable within a process but moving with the heap
    layout, so it varied across runs and builds.

    Observable: a fresh instance is now reproducible. [D] renders two
    independent instances with all three RNG paths ENGAGED and requires the
    outputs to be bit-identical — which is the finding stated directly, since
    pre-fix each instance seeded itself from the clock.

    This is also the section the other four depend on. WR-06 was scheduled
    first precisely so the rest could be compared byte-for-byte; the review's
    own batching table says as much.

    ── WR-02  Mipmap level chosen from the un-warped frequency ───────────────

    readSample() picked its level from the member `frequency`, but Sync runs the
    slave accumulator at phaseIncrement * (1 + 3*warpAmount) — up to 4x — and
    Bend applies pow(phase, 1 + 3*warpAmount), compressing the cycle into its
    head. Both generate harmonics far above the unwarped spectrum while reading
    a table band-limited for the base pitch, so the surplus folded: Sync at warp
    1.0 around C6 aliased audibly and cleaned up as warp swept back to 0.

    Observable: getLevelSelectFrequency(), the quantity the fix introduces. [W]
    pins it exactly for every warp mode and for both sides of the 0.001f floor
    that gates Sync. There is deliberately NO threshold-based spectral assertion
    here: aliasing in a sync'd or phase-distorted oscillator is periodic at the
    master f0, so it lands ON the harmonics rather than between them, and no
    inharmonic-energy metric can separate it. The audible improvement is a
    band-limiting change and is recorded as a human listening row, not gated.

    ── WR-04  Mono output dropped the left delay line — RETRACTED ──────────

    rightData aliases leftData when the block has one channel, so `rightData[i]
    = wetR` overwrote `leftData[i] = wetL`. The review concluded that "in
    PingPong mode the two lines carry genuinely different signal, so mono
    ping-pong is the right line only". True in stereo, false in mono — and mono
    is the only case where the aliasing happens. In mono rightData aliases
    leftData for READING too, so inputL == inputR; DelayProcessor is symmetric
    end to end (same maximum delay, same lowpass type and 8 kHz cutoff on both
    feedback filters, ONE shared delaySamples smoother driving both reads, both
    feedback states reset to 0) and PingPong's cross-feedback is itself
    symmetric. Nothing breaks the symmetry, so wetL == wetR for all time and the
    store that was overwritten held a bit-identical value.

    The prescribed guard is applied anyway — it is free, it makes the mono
    intent explicit, and it is the correct arithmetic if an asymmetric mono path
    is ever added — but it fixes no live defect and changes no audio. [M1] and
    [M2] assert exactly that, and [M3] pins the arithmetic that would matter if
    the lines ever did diverge.

    ── WR-05  Pink noise had no rate correction — RETRACTED ────────────────

    The three Paul Kellet poles are published for 44.1 kHz, and Brown, Vinyl and
    Wind all scale their coefficients by the rate while Pink does not. The
    mechanism is real for a pole in isolation. The conclusion — "measurably
    brighter at 48 kHz and noticeably so at 96 kHz" — is not: measured over
    octave bands from 125 Hz to 16 kHz, the shipped filter moves by 0.085 dB at
    48 kHz and 0.546 dB at 96 kHz, and the 96 kHz residual changes sign band to
    band rather than tilting. Both prescribed corrections make it WORSE (poles
    warped: 0.651 / 4.993 dB; warped plus the white-noise PSD rescale: 0.283 /
    1.615 dB), because neither preserves the SUMMED response, and the sum is
    what is audible. The coefficients are unchanged; [P-neg] runs both
    corrections through the same measurement so re-applying either one fails.

    ── WR-08  Glide interpolated in linear Hz, not in pitch ──────────────────

    currentFreq = currentFreq*c + targetFreq*(1-c) is a one-pole on FREQUENCY.
    Perceived pitch is logarithmic, so a C2->C5 glide crawled through the bottom
    octave and crossed the top one in a fraction of the time. For a plugin whose
    premise is microtonal pitch accuracy that is the wrong domain.

    Observable, with a built-in negative control: post-fix the CENTS remaining
    decays by a constant factor per sample, so [G1] a constant pitch half-life
    across disjoint windows, and [G2] an up-glide and a down-glide over the same
    interval are mirror images in the pitch domain. Pre-fix neither holds, and
    the pre-fix one-pole is reproduced in this file to prove it: it must fail
    both.

    Usage:  O-Prism-dsp-quality-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/DelayProcessor.h"
#include "dsp/GlideProcessor.h"
#include "dsp/LFO.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/NoiseGenerator.h"
#include "dsp/SVFFilter.h"
#include "dsp/WavetableData.h"
#include "dsp/WavetableFactory.h"
#include "dsp/WavetableOscillator.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
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

juce::String num (double v, int places = 6)
{
    return juce::String (v, places);
}

// ═════════════════════════════════════════════════════════════════════════
//  WR-06 — determinism
// ═════════════════════════════════════════════════════════════════════════

constexpr double kProcRate  = 48000.0;
constexpr int    kProcBlock = 512;
constexpr int    kProcBlocks = 10;

/** One full processor render, kept sample-exact so two can be differenced. */
struct Render
{
    juce::AudioBuffer<float> audio;
    bool finite = true;

    float peak() const
    {
        float m = 0.0f;
        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                m = juce::jmax (m, std::abs (audio.getSample (ch, i)));
        return m;
    }
};

float maxAbsDiff (const Render& a, const Render& b)
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

/** What a determinism render engages. Each flag turns on one RNG path. */
struct RngConfig
{
    bool noise       = false; // NoiseGenerator::randomL/randomR
    bool sampleHold  = false; // LFO::random
    bool randomPhase = false; // WavetableOscillator::resetWithRandomPhases
    float modAmount  = 1.0f;  // 0 => the route is inert (non-vacuity control)
};

/** A configured instance. Two of these are deliberately held ALIVE AT ONCE in
    the determinism comparison: pre-fix, resetWithRandomPhases seeded its LCG
    from `this`, and two instances created and destroyed in sequence can be
    handed the same heap address — the second would then reproduce the first's
    phases and the comparison would pass on pre-fix code for the wrong reason
    (pattern_distinct_buffer_addresses_are_not_an_allocation_bound). Coexisting
    instances cannot share an address. */
struct Probe
{
    std::unique_ptr<juce::AudioProcessor> proc;
    OPrismAudioProcessor* prism = nullptr;

    explicit Probe (const RngConfig& cfg)
    {
        proc.reset (createPluginFilter());
        prism = dynamic_cast<OPrismAudioProcessor*> (proc.get());

        if (prism == nullptr)
        {
            check (false, "createPluginFilter() returned an OPrismAudioProcessor");
            return;
        }

        // setPlayConfigDetails BEFORE prepareToPlay: getSampleRate() comes from
        // the former, and a gate that skips it renders every block at rate 0
        // (v1.27.1).
        prism->setPlayConfigDetails (0, 2, kProcRate, kProcBlock);
        prism->prepareToPlay (kProcRate, kProcBlock);
        configure (cfg);
    }

    void setP (const juce::String& id, float plain) const
    {
        if (auto* p = prism->getAPVTS().getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (plain));
        else
            check (false, "parameter exists: " + id);
    }

    void configure (const RngConfig& cfg) const;
    Render render() const;
};

void Probe::configure (const RngConfig& cfg) const
{

    // Random start phases are taken when the Phase parameter is 0 — startNote
    // branches to resetWithRandomPhases there and to resetWithPhase otherwise.
    setP ("oscAPhase", cfg.randomPhase ? 0.0f : 0.25f);
    setP ("oscBPhase", cfg.randomPhase ? 0.0f : 0.35f);

    setP ("noiseLevel", cfg.noise ? 0.8f : 0.0f);
    setP ("noiseType",  1.0f); // Pink — the WR-05 filter, and two juce::Random streams
    setP ("subLevel",   0.0f);

    // Sample & Hold is the LFO shape that draws from juce::Random. Route LFO1
    // at a rate fast enough to draw many times over the window.
    setP ("lfo1Shape", cfg.sampleHold ? 4.0f : 0.0f);
    setP ("lfo1Sync",  0.0f);
    setP ("lfo1Rate",  20.0f);
    setP ("modSlot0Src", static_cast<float> (static_cast<int> (ModSource::LFO1)));
    setP ("modSlot0Dst", static_cast<float> (static_cast<int> (ModDest::Pitch)));
    setP ("modSlot0Amt", cfg.modAmount);
    setP ("modSlot0On",  1.0f);

}

Render Probe::render() const
{
    Render r;
    r.audio.setSize (2, kProcBlocks * kProcBlock);
    r.audio.clear();

    juce::AudioBuffer<float> buf (2, kProcBlock);

    for (int b = 0; b < kProcBlocks; ++b)
    {
        juce::MidiBuffer midi;

        // A three-note chord so more than one voice — and so more than one
        // seed — is exercised.
        if (b == 0)
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, 55, 0.9f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 8);
            midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 16);
        }

        buf.clear();
        prism->processBlock (buf, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            r.audio.copyFrom (ch, b * kProcBlock, buf, ch, 0, kProcBlock);

            for (int i = 0; i < kProcBlock; ++i)
                if (! std::isfinite (buf.getSample (ch, i)))
                    r.finite = false;
        }
    }

    return r;
}

void checkDeterminism()
{
    std::cout << "\nWR-06 — deterministic RNG seeding\n";

    struct Case { const char* name; RngConfig cfg; };

    const Case cases[] = {
        { "noise only",              RngConfig { true,  false, false, 1.0f } },
        { "S&H LFO only",            RngConfig { false, true,  false, 1.0f } },
        { "random start phases only",RngConfig { false, false, true,  1.0f } },
        { "all three together",      RngConfig { true,  true,  true,  1.0f } },
    };

    for (const auto& c : cases)
    {
        // Both alive before either renders — see the Probe comment.
        const Probe pa (c.cfg);
        const Probe pb (c.cfg);
        const auto a = pa.render();
        const auto b = pb.render();

        // Non-vacuity first: an identity between two silent renders proves
        // nothing, so require real signal and finiteness before comparing.
        check (a.finite && b.finite, juce::String ("[D0] ") + c.name + ": both renders finite");
        check (a.peak() > 0.01f,
               juce::String ("[D0] ") + c.name + ": render is non-silent (peak "
               + num (a.peak(), 4) + ")");

        // Bit-identity: maxAbsDiff is non-negative, so "not greater than zero"
        // is exactly "equal" here and avoids -Wfloat-equal.
        const float d = maxAbsDiff (a, b);
        check (! (d > 0.0f),
               juce::String ("[D] ") + c.name
               + ": two independent instances render BIT-IDENTICALLY (max |diff| "
               + num (d, 9) + ")");
    }

    // The route has to matter, or [D] would pass on a build where the whole
    // modulation path was dead.
    const Probe pRouted (RngConfig { true, true, true, 1.0f });
    const Probe pInert  (RngConfig { true, true, true, 0.0f });
    const auto routed = pRouted.render();
    const auto inert  = pInert.render();
    check (maxAbsDiff (routed, inert) > 1.0e-3f,
           "[D-nv] the modulation route is live (amount 1 vs 0 differ by "
           + num (maxAbsDiff (routed, inert), 6) + ")");

    // ── Unit level: the seed both works and SEPARATES streams. ────────────
    // "Different seeds differ" is this section's own non-vacuity: it rules out
    // a fix that made everything reproducible by making it constant.

    {
        LFO a, b, c;
        a.setSeed (0x11111111u); b.setSeed (0x11111111u); c.setSeed (0x22222222u);
        for (auto* l : { &a, &b, &c })
        {
            l->prepare (kProcRate);
            l->setShape (LFO::Shape::SampleAndHold);
            l->setRate (500.0f); // many S&H draws over the window
            l->reset();
        }

        std::vector<float> va, vb, vc;
        for (int i = 0; i < 4096; ++i)
        {
            va.push_back (a.getNextSample());
            vb.push_back (b.getNextSample());
            vc.push_back (c.getNextSample());
        }

        check (va == vb, "[D-lfo] LFO: equal seeds give an identical S&H stream");
        check (va != vc, "[D-lfo] LFO: different seeds give a different S&H stream");

        // > 0.0f rather than != : the intent is "differs at all", and the
        // inequality form keeps -Wfloat-equal quiet without weakening it.
        const bool varied = std::any_of (va.begin(), va.end(),
                                         [&] (float v) { return std::abs (v - va.front()) > 0.0f; });
        check (varied, "[D-lfo] LFO: the S&H stream actually varies (not a constant)");

        // The determinism property that matters for a bounce: prepare() rewinds.
        a.prepare (kProcRate);
        a.setShape (LFO::Shape::SampleAndHold);
        a.setRate (500.0f);
        a.reset();
        std::vector<float> again;
        for (int i = 0; i < 4096; ++i)
            again.push_back (a.getNextSample());
        check (again == va, "[D-lfo] LFO: prepare() rewinds the stream to the seed");
    }

    {
        NoiseGenerator a, b, c;
        a.setSeed (0x33333333u); b.setSeed (0x33333333u); c.setSeed (0x44444444u);
        for (auto* n : { &a, &b, &c }) { n->prepare (kProcRate); n->setType (1); n->reset(); }

        std::vector<double> al, ar, bl, cl;
        for (int i = 0; i < 4096; ++i)
        {
            double l = 0.0, r = 0.0;
            a.getNextSampleStereo (l, r); al.push_back (l); ar.push_back (r);
            b.getNextSampleStereo (l, r); bl.push_back (l);
            c.getNextSampleStereo (l, r); cl.push_back (l);
        }

        check (al == bl, "[D-noise] NoiseGenerator: equal seeds give an identical stream");
        check (al != cl, "[D-noise] NoiseGenerator: different seeds give a different stream");
        check (al != ar, "[D-noise] NoiseGenerator: L and R stay decorrelated "
                         "(a shared seed would collapse the bed to mono)");

        a.prepare (kProcRate); a.setType (1); a.reset();
        std::vector<double> again;
        for (int i = 0; i < 4096; ++i)
        {
            double l = 0.0, r = 0.0;
            a.getNextSampleStereo (l, r);
            again.push_back (l);
        }
        check (again == al, "[D-noise] NoiseGenerator: prepare() rewinds both streams");
    }

    {
        auto lib = WavetableFactory::createFactoryLibrary();
        check (! lib.empty(), "[D-osc] factory library is non-empty");

        if (! lib.empty())
        {
            const auto* table = lib[0].table.get();

            const auto phasesOf = [table] (uint32_t seed)
            {
                WavetableOscillator o;
                o.setWavetable (table);
                o.prepare (kProcRate);
                o.setFrequency (220.0);
                o.setPosition (0.0f);
                o.setUnison (4, 0.5f, 0.5f);
                o.setPhaseSeed (seed);
                o.resetWithRandomPhases();

                std::vector<double> out;
                for (int i = 0; i < 1024; ++i)
                {
                    double l = 0.0, r = 0.0;
                    o.getNextSampleStereo (l, r);
                    out.push_back (l);
                }
                return out;
            };

            const auto s1 = phasesOf (0x55555555u);
            const auto s1b = phasesOf (0x55555555u);
            const auto s2 = phasesOf (0x66666666u);

            // Note on how these two read pre-fix: each oscillator is built and
            // destroyed inside phasesOf, so all three calls get the same stack
            // address. The `this`-seeded LCG therefore produced the SAME phases
            // every time, which makes the equal-seeds assertion pass for the
            // wrong reason and puts the whole pre-fix failure on the
            // different-seeds one. That is the recorded negative-control result.
            check (s1 == s1b, "[D-osc] resetWithRandomPhases: equal seeds give identical phases");
            check (s1 != s2,  "[D-osc] resetWithRandomPhases: different seeds give different "
                              "phases (voices must not go phase-coherent)");
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════
//  WR-02 — mipmap level selection under warp
// ═════════════════════════════════════════════════════════════════════════

void checkWarpLevelSelect()
{
    std::cout << "\nWR-02 — mipmap level follows the warped spectrum\n";

    auto lib = WavetableFactory::createFactoryLibrary();
    if (lib.empty())
    {
        check (false, "[W] factory library is non-empty");
        return;
    }

    constexpr double kF0 = 440.0;

    const auto scaleFor = [&lib] (WarpType type, float amount)
    {
        WavetableOscillator o;
        o.setWavetable (lib[0].table.get());
        o.prepare (48000.0);
        o.setFrequency (kF0);
        o.setWarpType (type);
        o.setWarpAmount (amount);
        return o.getLevelSelectFrequency() / o.getFrequency();
    };

    const auto close = [] (double a, double b) { return std::abs (a - b) < 1.0e-9; };

    // Off and FM are unchanged: FM's index is fmInput, which moves every
    // sample, so no cached scale describes it.
    check (close (scaleFor (WarpType::Off, 0.0f), 1.0),  "[W] Off, amount 0.0   -> scale 1x");
    check (close (scaleFor (WarpType::Off, 1.0f), 1.0),  "[W] Off, amount 1.0   -> scale 1x");
    check (close (scaleFor (WarpType::FM,  1.0f), 1.0),  "[W] FM, amount 1.0    -> scale 1x (index is per-sample)");

    // Sync: the slave runs at 1 + 3*amount.
    check (close (scaleFor (WarpType::Sync, 1.0f), 4.0),  "[W] Sync, amount 1.0  -> scale 4x");
    check (close (scaleFor (WarpType::Sync, 0.5f), 2.5),  "[W] Sync, amount 0.5  -> scale 2.5x");

    // The 0.001f floor must match getNextSampleStereo()'s isSyncMode EXACTLY.
    // Below it the sync branch is not taken and the accumulator runs at the
    // base rate, so scaling the level there would band-limit a spectrum that
    // nothing widened.
    check (close (scaleFor (WarpType::Sync, 0.0005f), 1.0),
           "[W] Sync, amount 0.0005 (below the 0.001 floor) -> scale 1x, mirroring isSyncMode");
    check (scaleFor (WarpType::Sync, 0.002f) > 1.0,
           "[W] Sync, amount 0.002 (above the floor) -> scale > 1x");

    // Window shares the sync accumulator, so it shares the scale.
    check (close (scaleFor (WarpType::Window, 1.0f), 4.0), "[W] Window, amount 1.0 -> scale 4x");
    check (close (scaleFor (WarpType::Window, 0.0005f), 1.0),
           "[W] Window, amount 0.0005 -> scale 1x (same floor as Sync)");

    // Bend: the pow() exponent as the harmonic proxy, continuous with Off at 0.
    check (close (scaleFor (WarpType::Bend, 0.0f), 1.0), "[W] Bend, amount 0.0  -> scale 1x (continuous with Off)");
    check (close (scaleFor (WarpType::Bend, 1.0f), 4.0), "[W] Bend, amount 1.0  -> scale 4x");

    // setWarpType must not leave a stale scale behind — it has an internal
    // change guard, so the recompute sits outside it
    // (pattern_conditional_coeff_update_leaks_enabled_flag).
    {
        WavetableOscillator o;
        o.setWavetable (lib[0].table.get());
        o.prepare (48000.0);
        o.setFrequency (kF0);

        o.setWarpType (WarpType::Sync);
        o.setWarpAmount (1.0f);
        check (close (o.getLevelSelectFrequency(), 4.0 * kF0), "[W] Sync 1.0 engaged -> 4x");

        o.setWarpType (WarpType::Off);
        check (close (o.getLevelSelectFrequency(), kF0),
               "[W] switching to Off clears the scale (no stale 4x behind the change guard)");

        o.setWarpType (WarpType::Bend);
        check (close (o.getLevelSelectFrequency(), 4.0 * kF0),
               "[W] switching to Bend picks the scale up from the retained amount");
    }

    // The warped oscillator must still render finite, non-silent audio — the
    // level shift moves which mipmap is read, and reading past the level array
    // would be the obvious way to get this wrong.
    {
        WavetableOscillator o;
        o.setWavetable (lib[0].table.get());
        o.prepare (48000.0);
        o.setFrequency (8000.0); // high enough that 4x saturates the level clamp
        o.setWarpType (WarpType::Sync);
        o.setWarpAmount (1.0f);
        o.setPosition (0.0f);
        o.setUnison (1, 0.0f, 0.0f);
        o.reset();

        bool finite = true;
        double peak = 0.0;
        for (int i = 0; i < 8192; ++i)
        {
            double l = 0.0, r = 0.0;
            o.getNextSampleStereo (l, r);
            finite = finite && std::isfinite (l) && std::isfinite (r);
            peak = std::max (peak, std::abs (l));
        }

        check (finite, "[W] Sync 4x at 8 kHz renders finite (level clamp holds)");
        check (peak > 1.0e-4, "[W] Sync 4x at 8 kHz renders non-silent (peak " + num (peak, 6) + ")");
    }
}

// ═════════════════════════════════════════════════════════════════════════
//  WR-04 — mono delay must sum both lines
// ═════════════════════════════════════════════════════════════════════════

void checkMonoDelay()
{
    std::cout << "\nWR-04 — mono delay sums both lines\n";

    constexpr double kRate = 48000.0;
    constexpr int    kN    = 4096;

    // An impulse train, so the two lines carry clearly distinguishable signal
    // and the ping-pong cross-feedback has something to bounce.
    const auto makeInput = [] (std::vector<float>& v)
    {
        v.assign (kN, 0.0f);
        for (int i = 0; i < kN; i += 1024)
            v[static_cast<size_t> (i)] = 1.0f;
    };

    const auto configure = [] (DelayProcessor& d, int numChannels)
    {
        juce::dsp::ProcessSpec spec { kRate, static_cast<juce::uint32> (kN),
                                      static_cast<juce::uint32> (numChannels) };
        d.prepare (spec);
        d.reset();
        d.setMode (1);        // PingPong — the mode where L and R genuinely differ
        d.setTime (0.05f);
        d.setFeedback (0.7f);
        d.setMix (1.0f);      // fully wet, so the dry path cannot mask the bug
    };

    std::vector<float> in;
    makeInput (in);

    // Mono run.
    std::vector<float> mono = in;
    {
        DelayProcessor d;
        configure (d, 1);
        float* ptrs[1] = { mono.data() };
        juce::dsp::AudioBlock<float> block (ptrs, 1, static_cast<size_t> (kN));
        d.process (block);
    }

    // Stereo run with the SAME signal on both inputs. delayL is fed from
    // inputL and delayR from inputR, so both lines see exactly what they saw
    // in the mono run — the internal state is identical and the comparison is
    // exact rather than approximate.
    std::vector<float> stL = in, stR = in;
    {
        DelayProcessor d;
        configure (d, 2);
        float* ptrs[2] = { stL.data(), stR.data() };
        juce::dsp::AudioBlock<float> block (ptrs, 2, static_cast<size_t> (kN));
        d.process (block);
    }

    double maxDiffSum = 0.0, maxDiffRightOnly = 0.0, maxLR = 0.0, peak = 0.0;
    for (int i = 0; i < kN; ++i)
    {
        const double sum = 0.5 * (static_cast<double> (stL[static_cast<size_t> (i)])
                                + static_cast<double> (stR[static_cast<size_t> (i)]));
        maxDiffSum       = std::max (maxDiffSum, std::abs (mono[static_cast<size_t> (i)] - sum));
        maxDiffRightOnly = std::max (maxDiffRightOnly,
                                     std::abs (static_cast<double> (mono[static_cast<size_t> (i)])
                                             - static_cast<double> (stR[static_cast<size_t> (i)])));
        maxLR            = std::max (maxLR, std::abs (static_cast<double> (stL[static_cast<size_t> (i)])
                                                    - static_cast<double> (stR[static_cast<size_t> (i)])));
        peak             = std::max (peak, std::abs (static_cast<double> (mono[static_cast<size_t> (i)])));
    }

    check (peak > 1.0e-3, "[M0] the mono delay output is non-silent (peak " + num (peak, 6) + ")");

    // [M1] WHY WR-04 IS INERT, measured rather than argued. The review reasoned
    // that "in PingPong mode the two lines carry genuinely different signal, so
    // mono ping-pong is the right line only". That is true in stereo and FALSE
    // in mono, which is the only case where the write aliasing happens. In mono
    // rightData aliases leftData for READING too, so inputL == inputR; the
    // processor is symmetric end to end (same maximum delay, same lowpass type
    // and 8 kHz cutoff on both feedback filters, one shared delaySamples
    // smoother driving both reads, both feedback states reset to 0), and
    // PingPong's cross-feedback is itself symmetric. Nothing ever breaks the
    // symmetry, so wetL == wetR for all time and the discarded store held a
    // value bit-identical to the one that replaced it.
    check (! (maxLR > 0.0),
           "[M1] equal inputs keep the two delay lines BIT-IDENTICAL (max |L-R| "
           + num (maxLR, 9) + ") — this is why the aliasing was inert, not a live defect");

    // [M2] So the fix must be behaviour-preserving: no existing session changes.
    // Post-fix mono writes 0.5*(wetL+wetR); pre-fix it wrote wetR. With the two
    // equal, both equal each other AND the sum, bit for bit.
    check (maxDiffSum < 1.0e-9,
           "[M2] mono output == 0.5 * (stereoL + stereoR) (max |diff| "
           + num (maxDiffSum, 9) + ")");
    check (maxDiffRightOnly < 1.0e-9,
           "[M2] mono output == the right line alone, i.e. the PRE-FIX result too "
           "(max |diff| " + num (maxDiffRightOnly, 9) + ") — the fix changes no audio");

    // [M3] What the guard actually buys. It is load-bearing only if the two
    // lines can ever diverge on a one-channel block. They cannot today, but the
    // divergence condition is one asymmetric input away, so pin the arithmetic
    // that would then matter: with DIFFERENT signal on the two lines, the sum
    // and the right line are substantially different answers, and post-fix code
    // takes the sum.
    {
        std::vector<float> dL (static_cast<size_t> (kN), 0.0f);
        std::vector<float> dR (static_cast<size_t> (kN), 0.0f);
        for (int i = 0; i < kN; i += 1024)
            dL[static_cast<size_t> (i)] = 1.0f;           // impulses on L only
        for (int i = 512; i < kN; i += 1024)
            dR[static_cast<size_t> (i)] = 1.0f;           // offset impulses on R

        DelayProcessor d;
        configure (d, 2);
        float* ptrs[2] = { dL.data(), dR.data() };
        juce::dsp::AudioBlock<float> block (ptrs, 2, static_cast<size_t> (kN));
        d.process (block);

        double diverge = 0.0, sumVsRight = 0.0;
        for (int i = 0; i < kN; ++i)
        {
            const double l = dL[static_cast<size_t> (i)];
            const double r = dR[static_cast<size_t> (i)];
            diverge    = std::max (diverge, std::abs (l - r));
            sumVsRight = std::max (sumVsRight, std::abs (0.5 * (l + r) - r));
        }

        check (diverge > 1.0e-2,
               "[M3] asymmetric input DOES make the two lines diverge (max |L-R| "
               + num (diverge, 6) + ") — so the guard has a failure mode to guard");
        check (sumVsRight > 1.0e-2,
               "[M3] and there the sum and the right line differ (max |diff| "
               + num (sumVsRight, 6) + "), which is the case the pre-fix store would have lost");
    }
}

// ═════════════════════════════════════════════════════════════════════════
//  WR-05 — pink noise must not move with the sample rate
// ═════════════════════════════════════════════════════════════════════════

constexpr int kFftOrder = 12;              // 4096-point
constexpr int kFftSize  = 1 << kFftOrder;

/** Welch-averaged power spectrum of a sample vector, normalised per bin. */
std::vector<double> powerSpectrum (const std::vector<double>& x)
{
    juce::dsp::FFT fft (kFftOrder);
    std::vector<double> acc (static_cast<size_t> (kFftSize / 2), 0.0);

    // Hann window; 50% overlap.
    std::vector<float> window (static_cast<size_t> (kFftSize));
    for (int i = 0; i < kFftSize; ++i)
        window[static_cast<size_t> (i)] =
            static_cast<float> (0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi
                                                      * i / (kFftSize - 1)));

    int frames = 0;
    std::vector<float> buf (static_cast<size_t> (2 * kFftSize));

    for (size_t start = 0; start + static_cast<size_t> (kFftSize) <= x.size();
         start += static_cast<size_t> (kFftSize / 2))
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        for (int i = 0; i < kFftSize; ++i)
            buf[static_cast<size_t> (i)] =
                static_cast<float> (x[start + static_cast<size_t> (i)])
                * window[static_cast<size_t> (i)];

        fft.performFrequencyOnlyForwardTransform (buf.data());

        for (int i = 0; i < kFftSize / 2; ++i)
        {
            const double m = buf[static_cast<size_t> (i)];
            acc[static_cast<size_t> (i)] += m * m;
        }

        ++frames;
    }

    if (frames > 0)
        for (auto& v : acc)
            v /= frames;

    return acc;
}

/** Mean periodogram power in [lo, hi), normalised so the result is comparable
    ACROSS sample rates.

    Two normalisations, and both are needed:

      - Mean rather than sum over bins. A band in absolute Hz spans fewer bins
        at 96 kHz than at 44.1 kHz, and a sum would read low for that reason
        alone.

      - Divide by the sample rate. The periodogram of a filtered white sequence
        estimates |H(f)|^2 * sigma^2 * sum(w^2), NOT power per Hz: white noise
        of fixed amplitude has fs-independent TOTAL power, so its power per Hz
        is 2*sigma^2/fs. Hence PSD_out = |H|^2 * 2*sigma^2/fs, i.e. the
        periodogram over fs is what tracks the PSD. Skipping this term puts a
        flat 10*log10(96/44.1) = 3.37 dB offset on every band and the gate
        fails its own subject. */
double bandPowerPerHz (const std::vector<double>& spec, double sampleRate,
                       double lo, double hi)
{
    const double binHz = sampleRate / kFftSize;
    const int    first = std::max (1, static_cast<int> (std::ceil (lo / binHz)));
    const int    last  = std::min (static_cast<int> (spec.size()) - 1,
                                   static_cast<int> (std::floor (hi / binHz)));

    if (last <= first)
        return 0.0;

    double sum = 0.0;
    for (int i = first; i <= last; ++i)
        sum += spec[static_cast<size_t> (i)];

    return sum / (last - first + 1) / sampleRate;
}

/** A Paul Kellet economy pink filter with SUPPLIED coefficients, so the gate
    can run the shipped set and each prescribed "correction" through one
    measurement path. */
std::vector<double> renderPinkWith (const double a[3], const double g[3], double direct,
                                    int numSamples, juce::uint32 seed)
{
    juce::Random rng (static_cast<juce::int64> (seed));
    double b0 = 0.0, b1 = 0.0, b2 = 0.0;
    std::vector<double> out;
    out.reserve (static_cast<size_t> (numSamples));

    for (int i = 0; i < numSamples; ++i)
    {
        const double white = rng.nextDouble() * 2.0 - 1.0;
        b0 = a[0] * b0 + white * g[0];
        b1 = a[1] * b1 + white * g[1];
        b2 = a[2] * b2 + white * g[2];
        out.push_back ((b0 + b1 + b2 + white * direct) * 0.11);
    }

    return out;
}

std::vector<double> renderNoise (double sampleRate, int type, int numSamples)
{
    NoiseGenerator n;
    n.setSeed (0x5EED1234u);
    n.prepare (sampleRate);
    n.setType (type);
    n.reset();

    std::vector<double> out;
    out.reserve (static_cast<size_t> (numSamples));

    for (int i = 0; i < numSamples; ++i)
    {
        double l = 0.0, r = 0.0;
        n.getNextSampleStereo (l, r);
        out.push_back (l);
    }

    return out;
}

void checkPinkRate()
{
    std::cout << "\nWR-05 — RETRACTED: pink noise is already rate-invariant\n";

    // Octave bands in ABSOLUTE Hz, 125 Hz to 16 kHz.
    struct Band { const char* name; double lo, hi; };
    const Band bands[] = {
        { "125 Hz", 88.0,    177.0 },
        { "250 Hz", 177.0,   354.0 },
        { "500 Hz", 354.0,   707.0 },
        { "1 kHz",  707.0,   1414.0 },
        { "2 kHz",  1414.0,  2828.0 },
        { "4 kHz",  2828.0,  5657.0 },
        { "8 kHz",  5657.0,  11314.0 },
        { "16 kHz", 11314.0, 20000.0 },
    };

    // The pink slope is only clean below the third pole (~4 kHz); above it the
    // economy filter's flat direct term takes over, so the slope control is
    // measured over the first four bands only.
    constexpr size_t kSlopeBands = 4;

    const auto measure = [&] (const std::vector<double>& x, double rate)
    {
        const auto spec = powerSpectrum (x);
        std::vector<double> db;
        for (const auto& b : bands)
            db.push_back (10.0 * std::log10 (std::max (bandPowerPerHz (spec, rate, b.lo, b.hi),
                                                       1.0e-300)));
        return db;
    };

    const auto worstVs = [&] (const std::vector<double>& x, const std::vector<double>& ref)
    {
        double w = 0.0;
        for (size_t i = 0; i < ref.size(); ++i)
            w = std::max (w, std::abs (x[i] - ref[i]));
        return w;
    };

    constexpr double kSeconds = 8.0;
    const auto samplesFor = [] (double rate) { return static_cast<int> (rate * kSeconds); };

    const auto pink44 = measure (renderNoise (44100.0, 1, samplesFor (44100.0)), 44100.0);

    // ── Controls on the measurement itself ────────────────────────────────
    const double slope44 = (pink44[kSlopeBands - 1] - pink44[0]) / (kSlopeBands - 1);
    check (slope44 < -2.0 && slope44 > -4.0,
           "[P0] the 44.1 kHz pink spectrum really slopes at ~-3 dB/octave over 125 Hz - 1 kHz ("
           + num (slope44, 3) + " dB/oct)");

    const auto white44 = measure (renderNoise (44100.0, 0, samplesFor (44100.0)), 44100.0);
    const double whiteSlope = (white44[kSlopeBands - 1] - white44[0]) / (kSlopeBands - 1);
    check (std::abs (whiteSlope) < 0.5,
           "[P0] White comes out flat, so the band measurement is not inventing a slope ("
           + num (whiteSlope, 3) + " dB/oct)");

    // ── [P] THE RETRACTION. The shipped filter is already rate-invariant. ──
    //
    // The review's claim was that the bed is "measurably brighter at 48 kHz and
    // noticeably so at 96 kHz". It is not: at 48 kHz nothing moves at all, and
    // at 96 kHz the deviation is a sub-dB ripple that changes sign band to band
    // rather than a tilt. 1.0 dB is the ceiling this gate holds the shipped
    // path to; the measured values sit far inside it.
    constexpr double kTolDb = 1.0;

    struct RateCase { double rate; };
    const RateCase rates[] = { { 48000.0 }, { 96000.0 } };

    double worstShipped = 0.0;

    for (const auto& rc : rates)
    {
        const auto m = measure (renderNoise (rc.rate, 1, samplesFor (rc.rate)), rc.rate);
        const double w = worstVs (m, pink44);
        worstShipped = std::max (worstShipped, w);

        check (w < kTolDb,
               "[P] as shipped, " + num (rc.rate / 1000.0, 1)
               + " kHz stays within " + num (kTolDb, 1) + " dB of 44.1 kHz across 125 Hz - 16 kHz "
               "(worst band " + num (w, 3) + " dB)");
    }

    // ── [P-neg] Both prescribed corrections are WORSE. ────────────────────
    //
    // This is the assertion that keeps WR-05 shut. Either correction reads as an
    // obvious improvement on paper — preserve the pole frequencies, then also
    // correct for white noise's 1/fs power per Hz — and both make the match
    // worse, because neither preserves the SUMMED response that is audible.
    // Anyone re-applying the review's prescription trips this.
    constexpr double kRefRate  = 44100.0;
    constexpr double kRefPole[3] = { 0.99765, 0.96300, 0.57000 };
    constexpr double kRefGain[3] = { 0.0990460, 0.2965164, 1.0526913 };
    constexpr double kRefDirect  = 0.1848;

    for (const auto& rc : rates)
    {
        const double warp     = kRefRate / rc.rate;
        const double psdScale = std::sqrt (rc.rate / kRefRate);

        double poleW[3], gainW[3], gainWPsd[3];
        for (int i = 0; i < 3; ++i)
        {
            poleW[i]    = std::pow (kRefPole[i], warp);
            gainW[i]    = kRefGain[i] * ((1.0 - poleW[i]) / (1.0 - kRefPole[i]));
            gainWPsd[i] = gainW[i] * psdScale;
        }

        const int n = samplesFor (rc.rate);

        const double wWarp = worstVs (measure (renderPinkWith (poleW, gainW, kRefDirect,
                                                               n, 0x5EED1234u), rc.rate), pink44);
        const double wBoth = worstVs (measure (renderPinkWith (poleW, gainWPsd, kRefDirect * psdScale,
                                                               n, 0x5EED1234u), rc.rate), pink44);

        const auto shipped = measure (renderNoise (rc.rate, 1, n), rc.rate);
        const double wShip = worstVs (shipped, pink44);

        std::cout << "        " << num (rc.rate / 1000.0, 1) << " kHz worst band — shipped "
                  << num (wShip, 3) << " dB | poles warped " << num (wWarp, 3)
                  << " dB | warped + PSD rescale " << num (wBoth, 3) << " dB\n";

        check (wWarp > wShip,
               "[P-neg] at " + num (rc.rate / 1000.0, 1) + " kHz, warping the poles is WORSE than "
               "shipping them unchanged (" + num (wWarp, 3) + " vs " + num (wShip, 3) + " dB)");
        check (wBoth > wShip,
               "[P-neg] at " + num (rc.rate / 1000.0, 1) + " kHz, warping the poles AND rescaling "
               "for white-noise PSD is also worse (" + num (wBoth, 3) + " vs " + num (wShip, 3) + " dB)");
    }

    std::cout << "        shipped worst band deviation over both rates: "
              << num (worstShipped, 3) << " dB\n";
}

// ═════════════════════════════════════════════════════════════════════════
//  WR-08 — glide interpolates in pitch, not in linear Hz
// ═════════════════════════════════════════════════════════════════════════

constexpr double kGlideRate = 48000.0;
constexpr double kGlideTime = 0.5;
constexpr double kC2 = 65.40639132514966;
constexpr double kC5 = 523.2511306011972;
constexpr int    kGlideSamples = 24000; // 0.5 s

std::vector<double> glideTrajectory (double from, double to)
{
    GlideProcessor g;
    g.prepare (kGlideRate);
    g.setMode (2); // Always
    g.setTime (kGlideTime);
    g.setTarget (from, false); // snap to the start
    g.startFrom (from);
    g.setTarget (to, true);

    std::vector<double> out;
    out.reserve (static_cast<size_t> (kGlideSamples));
    for (int i = 0; i < kGlideSamples; ++i)
        out.push_back (g.getNextFrequency());
    return out;
}

/** The PRE-FIX glide: a one-pole on FREQUENCY. Reproduced here so WR-08 needs
    no source revert to demonstrate its own negative control. */
std::vector<double> glideTrajectoryPreFix (double from, double to)
{
    const double coeff = std::exp (-1.0 / (kGlideTime * kGlideRate));
    double cur = from;

    std::vector<double> out;
    out.reserve (static_cast<size_t> (kGlideSamples));
    for (int i = 0; i < kGlideSamples; ++i)
    {
        cur = cur * coeff + to * (1.0 - coeff);
        out.push_back (cur);
    }
    return out;
}

/** Cents still to travel, as a fraction of the whole interval. */
std::vector<double> centsRemainingFraction (const std::vector<double>& freq,
                                            double from, double to)
{
    const double total = std::log2 (to / from);
    std::vector<double> out;
    out.reserve (freq.size());
    for (double f : freq)
        out.push_back (std::log2 (to / f) / total);
    return out;
}

void checkGlideDomain()
{
    std::cout << "\nWR-08 — glide runs in the pitch domain\n";

    const auto up   = glideTrajectory (kC2, kC5);
    const auto down = glideTrajectory (kC5, kC2);

    // Sanity / non-vacuity: the glide must actually traverse the interval.
    check (std::abs (up.front() - kC2) / kC2 < 0.01,
           "[G0] the up-glide starts at C2 (" + num (up.front(), 4) + " Hz)");
    // After exactly one time constant a one-pole has covered 1 - 1/e = 63.2 %
    // of its distance — and post-fix that distance is measured in PITCH, so the
    // frequency lands at C2 * 2^(3 octaves * (1 - 1/e)) = 243.49 Hz, not at C5.
    // Asserting the law rather than "nearly arrived" makes this a real check on
    // the domain: the pre-fix glide covers 63.2 % of the HERTZ in the same time,
    // which is 354.7 Hz.
    const double octaves = std::log2 (kC5 / kC2);
    const double expectedAtTau = kC2 * std::exp2 (octaves * (1.0 - std::exp (-1.0)));
    check (std::abs (up.back() - expectedAtTau) / expectedAtTau < 1.0e-3,
           "[G0] after one time constant the glide has covered 63.2 % of the PITCH distance ("
           + num (up.back(), 4) + " Hz, expected " + num (expectedAtTau, 4) + " Hz)");
    check (std::is_sorted (up.begin(), up.end()), "[G0] the up-glide is monotonic");

    {
        GlideProcessor g;
        g.prepare (kGlideRate);
        g.setMode (0); // Off
        g.setTime (kGlideTime);
        g.setTarget (kC2, false);
        g.setTarget (kC5, true);
        check (std::abs (g.getNextFrequency() - kC5) < 1.0e-9,
               "[G0] mode Off still snaps to the target immediately");
    }

    // [G1] Constant pitch half-life. Post-fix the CENTS remaining decays by a
    // fixed factor per sample, so ln(err) is linear in n with the same slope in
    // every window. Pre-fix the HZ error is the geometric one and the cents
    // slope drifts.
    const auto slopeOverWindows = [] (const std::vector<double>& freq,
                                      double from, double to)
    {
        const auto frac = centsRemainingFraction (freq, from, to);
        std::vector<double> slopes;

        // Four disjoint windows over the first half of the glide, where the
        // remaining fraction is still well clear of float noise.
        for (int w = 0; w < 4; ++w)
        {
            const int a = 500 + w * 2500;
            const int b = a + 2000;
            const double fa = frac[static_cast<size_t> (a)];
            const double fb = frac[static_cast<size_t> (b)];

            if (fa <= 0.0 || fb <= 0.0)
                return std::vector<double> {};

            slopes.push_back (std::log (fb / fa) / (b - a));
        }

        return slopes;
    };

    const auto sUp  = slopeOverWindows (up, kC2, kC5);
    const auto sPre = slopeOverWindows (glideTrajectoryPreFix (kC2, kC5), kC2, kC5);

    const auto spread = [] (const std::vector<double>& v)
    {
        if (v.empty())
            return std::numeric_limits<double>::infinity();
        const auto mn = *std::min_element (v.begin(), v.end());
        const auto mx = *std::max_element (v.begin(), v.end());
        return std::abs (mx - mn) / std::abs (mn);
    };

    check (spread (sUp) < 0.01,
           "[G1] the pitch half-life is constant across four disjoint windows (spread "
           + num (100.0 * spread (sUp), 4) + " %)");

    check (spread (sPre) > 0.05,
           "[G1-neg] the PRE-FIX linear-Hz one-pole has a drifting pitch half-life (spread "
           + num (100.0 * spread (sPre), 3) + " %) — the built-in negative control");

    // [G2] Direction symmetry, which is the user-visible complaint. In the
    // pitch domain an up-glide and a down-glide over the same interval must
    // have the SAME cents-remaining trajectory. In the Hz domain they do not:
    // descending covers hertz fast and then crawls, ascending crawls and then
    // rushes.
    const auto fUp   = centsRemainingFraction (up,   kC2, kC5);
    const auto fDown = centsRemainingFraction (down, kC5, kC2);

    double worstSym = 0.0;
    for (size_t i = 0; i < fUp.size(); ++i)
        worstSym = std::max (worstSym, std::abs (fUp[i] - fDown[i]));

    check (worstSym < 1.0e-6,
           "[G2] up- and down-glides are mirror images in the pitch domain (max |diff| "
           + num (worstSym, 9) + ")");

    const auto pUp   = centsRemainingFraction (glideTrajectoryPreFix (kC2, kC5), kC2, kC5);
    const auto pDown = centsRemainingFraction (glideTrajectoryPreFix (kC5, kC2), kC5, kC2);

    double worstSymPre = 0.0;
    for (size_t i = 0; i < pUp.size(); ++i)
        worstSymPre = std::max (worstSymPre, std::abs (pUp[i] - pDown[i]));

    check (worstSymPre > 0.05,
           "[G2-neg] the PRE-FIX glide is direction-asymmetric (max |diff| "
           + num (worstSymPre, 4) + ") — the built-in negative control");

    // [G3] The pitch midpoint is the geometric mean, not the arithmetic one.
    // At the sample where half the CENTS are covered the frequency must be
    // sqrt(C2*C5) = 185.0 Hz; the Hz-domain glide passes 294.3 Hz there.
    const double geoMean = std::sqrt (kC2 * kC5);
    size_t halfIdx = 0;
    while (halfIdx + 1 < fUp.size() && fUp[halfIdx] > 0.5)
        ++halfIdx;

    check (std::abs (up[halfIdx] - geoMean) / geoMean < 1.0e-4,
           "[G3] at the halfway point in CENTS the frequency is the geometric mean ("
           + num (up[halfIdx], 4) + " Hz vs " + num (geoMean, 4) + " Hz)");

    check (std::abs (0.5 * (kC2 + kC5) - geoMean) / geoMean > 0.1,
           "[G3-nv] the geometric and arithmetic means are far enough apart for [G3] to "
           "discriminate (" + num (geoMean, 2) + " Hz vs " + num (0.5 * (kC2 + kC5), 2) + " Hz)");
}

// ═════════════════════════════════════════════════════════════════════════
//  Info-tier sweep (v1.28.1) — IN-04/IN-05 (SVF), IN-06 (mod matrix), IN-08
// ═════════════════════════════════════════════════════════════════════════

/** v1.28.0's SVFFilter arithmetic, reproduced verbatim as a BUILT-IN NEGATIVE
    CONTROL for IN-04 and IN-05.

    IN-04 moved the `h` coefficient out of the per-sample path and IN-05 folded
    `processNotch` into the shared core as case 6. Both are claimed to be
    bit-identical, and a claim of bit-identity needs something to be identical
    TO. A hardcoded golden hash cannot serve: this gate builds at -O3, where
    arm64 FMA contraction can legitimately change the last bits of an
    expression that a -O0 capture would not, so a fixed digest would fail for a
    reason that has nothing to do with the refactor. Instead the old code runs
    beside the new one under the same flags, in the same translation unit, and
    the two must agree to the bit.

    Kept deliberately ugly and un-refactored — the per-sample `h`, the
    duplicated notch body, the literal 0.707 inlined twice. That IS the thing
    under test. Do not tidy it. */
struct LegacySVF
{
    double currentSampleRate = 44100.0;
    double cutoffHz = 20000.0;
    double resonance = 0.0;
    double driveAmount = 0.0;
    int filterType = 0;
    double ic1eq_1 = 0.0, ic2eq_1 = 0.0;
    double ic1eq_2 = 0.0, ic2eq_2 = 0.0;
    double g = 0.0, R2 = 0.0;
    bool coeffsDirty = true;

    void reset() { ic1eq_1 = ic2eq_1 = ic1eq_2 = ic2eq_2 = 0.0; }

    void updateCoefficients()
    {
        const double maxCutoff = std::min (20000.0, 0.49 * currentSampleRate);
        double fc = std::max (20.0, std::min (maxCutoff, cutoffHz));
        g = std::tan (3.141592653589793 * fc / currentSampleRate);
        double svfRes = 1.0 / (1.0 + resonance * 19.0);
        R2 = 2.0 * svfRes;
    }

    void prepare (double sr) { currentSampleRate = sr; updateCoefficients(); coeffsDirty = false; }

    double processSingleSVF (double input, double& s1, double& s2)
    {
        double h = 1.0 / (1.0 + R2 * g + g * g);
        double yHP = h * (input - s1 * (R2 + g) - s2);
        double yBP = yHP * g + s1;
        s1 = yHP * g + yBP;
        double yLP = yBP * g + s2;
        s2 = yBP * g + yLP;

        switch (filterType)
        {
            case 0: return yLP;
            case 1: return yLP;
            case 2: return yHP;
            case 3: return yHP;
            case 4: return yBP;
            case 5: return yBP;
            default: return yLP;
        }
    }

    double processNotch (double input, double& s1, double& s2)
    {
        double h = 1.0 / (1.0 + R2 * g + g * g);
        double yHP = h * (input - s1 * (R2 + g) - s2);
        double yBP = yHP * g + s1;
        s1 = yHP * g + yBP;
        double yLP = yBP * g + s2;
        s2 = yBP * g + yLP;
        return yLP + yHP;
    }

    double processSample (double input)
    {
        if (coeffsDirty) { updateCoefficients(); coeffsDirty = false; }

        if (driveAmount > 0.0)
            input = std::tanh (input * (1.0 + driveAmount * 9.0));

        if (filterType == 6)
            return processNotch (input, ic1eq_1, ic2eq_1);

        if (filterType == 0 || filterType == 2 || filterType == 4)
            return processSingleSVF (input, ic1eq_1, ic2eq_1);

        double stage1 = processSingleSVF (input, ic1eq_1, ic2eq_1);

        double h = 1.0 / (1.0 + 2.0 * 0.707 * g + g * g);
        double yHP = h * (stage1 - ic1eq_2 * (2.0 * 0.707 + g) - ic2eq_2);
        double yBP = yHP * g + ic1eq_2;
        ic1eq_2 = yHP * g + yBP;
        double yLP = yBP * g + ic2eq_2;
        ic2eq_2 = yBP * g + yLP;

        switch (filterType)
        {
            case 1: return yLP;
            case 3: return yHP;
            case 5: return yBP;
            default: return stage1;
        }
    }
};

void checkSvfRefactor()
{
    std::cout << "\n── IN-04 / IN-05: SVF coefficient caching and notch fold ──\n";

    const double rates[]   = { 44100.0, 48000.0, 96000.0 };
    const double cutoffs[] = { 20.0, 100.0, 440.0, 2000.0, 8000.0, 20000.0 };
    const double resos[]   = { 0.0, 0.25, 0.5, 0.9, 1.0 };
    const double drives[]  = { 0.0, 0.5, 1.0 };

    long   compared    = 0;
    long   mismatches  = 0;
    double worstAbs    = 0.0;
    long   notchCompared = 0;
    long   cascadeCompared = 0;
    double signalEnergy = 0.0;

    for (double fs : rates)
    for (int type = 0; type <= 6; ++type)
    for (double fc : cutoffs)
    for (double res : resos)
    for (double dr : drives)
    {
        SVFFilter now;
        now.setType (type);
        now.setCutoff (fc);
        now.setResonance (res);
        now.setDrive (dr);
        now.prepare (fs);
        now.reset();

        LegacySVF old;
        old.filterType = type;
        old.cutoffHz = fc;
        old.resonance = res;
        old.driveAmount = dr;
        old.prepare (fs);
        old.reset();

        for (int i = 0; i < 256; ++i)
        {
            const double t = i / fs;
            const double x = 0.5 * std::sin (2.0 * 3.141592653589793 * 220.0 * t)
                           + 0.3 * std::sin (2.0 * 3.141592653589793 * 3300.0 * t)
                           + (i == 0   ? 1.0 : 0.0)
                           + (i >= 128 ? 0.2 : 0.0);

            const double a = now.processSample (x);
            const double b = old.processSample (x);

            ++compared;
            signalEnergy += std::abs (b);

            // |a-b| > 0 rather than a != b: same predicate for finite values,
            // and it keeps -Wfloat-equal quiet, matching [D] above.
            const double d = std::abs (a - b);

            if (d > 0.0)
            {
                ++mismatches;
                worstAbs = std::max (worstAbs, d);
            }

            if (type == 6)            ++notchCompared;
            if (type == 1 || type == 3 || type == 5) ++cascadeCompared;
        }
    }

    // [S1] The refactor is bit-identical, not merely close. `!=` on doubles, no
    //      epsilon — a cached coefficient that drifted by one ULP would show.
    check (mismatches == 0,
           juce::String ("[S1] refactored SVF is BIT-identical to the v1.28.0 arithmetic over ")
           + juce::String (compared) + " samples (" + juce::String (mismatches)
           + " mismatches, worst |diff| " + num (worstAbs, 12) + ")");

    // [S2] Not a vacuous pass: the sweep has to actually exercise the two
    //      branches the refactor touched, and carry signal while doing it.
    check (notchCompared > 0 && cascadeCompared > 0,
           juce::String ("[S2] sweep covers the folded notch (") + juce::String (notchCompared)
           + " samples) and the 24 dB cascade (" + juce::String (cascadeCompared) + ")");

    check (signalEnergy > 1.0,
           juce::String ("[S3] excitation is not silence - summed |out| ") + num (signalEnergy, 3));
}

void checkModMatrixActiveSlots()
{
    std::cout << "\n── IN-06: mod matrix walks only active slots ──\n";

    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());
    auto* prism = dynamic_cast<OPrismAudioProcessor*> (proc.get());

    if (prism == nullptr)
    {
        check (false, "[N] processor cast failed");
        return;
    }

    auto& apvts = prism->getAPVTS();

    ModulationMatrix matrix;
    matrix.setAPVTS (&apvts);

    auto setSlot = [&] (int slot, const char* field, float norm)
    {
        const auto id = "modSlot" + juce::String (slot) + field;
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (norm);
    };

    // Route slot 0: LFO1 -> OscAPos, full amount, enabled.
    const auto srcNorm = static_cast<float> (static_cast<int> (ModSource::LFO1))
                       / static_cast<float> (ModulationMatrix::kNumSources - 1);
    const auto dstNorm = static_cast<float> (static_cast<int> (ModDest::OscAPos))
                       / static_cast<float> (ModulationMatrix::kNumDests - 1);

    setSlot (0, "Src", srcNorm);
    setSlot (0, "Dst", dstNorm);
    setSlot (0, "Amt", 1.0f);
    setSlot (0, "On",  1.0f);

    matrix.updateFromAPVTS();
    matrix.setSourceValue (ModSource::LFO1, 1.0f);
    matrix.evaluate();

    const float routedOffset = matrix.getModOffset (ModDest::OscAPos);

    // [N1] The route has to be live, or everything after it is vacuous.
    check (matrix.isDestinationRouted (ModDest::OscAPos) && routedOffset != 0.0f,
           juce::String ("[N1] routed destination accumulates (offset ") + num (routedOffset, 6) + ")");

    // ── THE REGRESSION IN-06 COULD INTRODUCE ─────────────────────────────
    // evaluate() now clears only the destinations updateFromAPVTS() found
    // routed. Turn the slot off: OscAPos leaves the routed set, so nothing in
    // evaluate() will ever zero it again. The transition clear in
    // updateFromAPVTS() - zeroing exactly the destinations that just left the
    // routed set - is the only thing standing between that and a permanently
    // stuck modulation offset. NEGATIVE CONTROL: delete that loop and [N2]
    // fails with the offset frozen at the value printed by [N1].
    //
    // It must stay a TRANSITION clear, not a blanket wipe: updateFromAPVTS()
    // runs per MIDI sub-block and PrismVoice reads ModDest::Pitch one sample
    // late, so wiping a still-routed destination here makes the render
    // density-sensitive. lfo-subblock-check [A]/[D] is the gate for that half.
    setSlot (0, "On", 0.0f);

    matrix.updateFromAPVTS();
    matrix.setSourceValue (ModSource::LFO1, 1.0f);
    matrix.evaluate();

    const float strandedOffset = matrix.getModOffset (ModDest::OscAPos);

    check (! matrix.isDestinationRouted (ModDest::OscAPos),
           "[N2a] destination left the routed set when its slot was disabled");

    // std::abs(x) > 0 is exactly "non-zero" for a finite float, and avoids
    // -Wfloat-equal (same idiom as [D]).
    check (! (std::abs (strandedOffset) > 0.0f),
           juce::String ("[N2] a destination that STOPPED being routed reads exactly 0.0 (got ")
           + num (strandedOffset, 9) + ", was " + num (routedOffset, 6) + ")");

    // [N3] The same must hold on the very first sample of the block, not just
    //      eventually — getModOffset is read per sample and nothing re-zeroes
    //      an unrouted destination mid-block.
    bool firstSampleClean = true;
    for (int i = 0; i < 64; ++i)
    {
        matrix.setSourceValue (ModSource::LFO1, 1.0f);
        matrix.evaluate();
        if (std::abs (matrix.getModOffset (ModDest::OscAPos)) > 0.0f)
            firstSampleClean = false;
    }

    check (firstSampleClean,
           "[N3] stays 0.0 across 64 per-sample evaluate() calls within the block");

    // [N4] Accumulation order is preserved. Two slots onto one destination sum
    //      in ascending slot order both before and after the compaction; float
    //      addition is not associative, so this is a real constraint, not a
    //      formality.
    setSlot (0, "Src", srcNorm);
    setSlot (0, "Dst", dstNorm);
    setSlot (0, "Amt", 1.0f);
    setSlot (0, "On",  1.0f);
    setSlot (5, "Src", static_cast<float> (static_cast<int> (ModSource::LFO2))
                     / static_cast<float> (ModulationMatrix::kNumSources - 1));
    setSlot (5, "Dst", dstNorm);
    setSlot (5, "Amt", 1.0f);
    setSlot (5, "On",  1.0f);

    matrix.updateFromAPVTS();
    matrix.setSourceValue (ModSource::LFO1, 1.0f / 3.0f);
    matrix.setSourceValue (ModSource::LFO2, 1.0f / 7.0f);
    matrix.evaluate();

    const float summed = matrix.getModOffset (ModDest::OscAPos);

    // The amounts come from the same atomics updateFromAPVTS() read, so the
    // expectation is built from the matrix's own inputs rather than a
    // hand-copied constant.
    const float amt0 = apvts.getRawParameterValue ("modSlot0Amt")->load();
    const float amt5 = apvts.getRawParameterValue ("modSlot5Amt")->load();
    const float expected = (0.0f + (1.0f / 3.0f) * amt0) + (1.0f / 7.0f) * amt5;

    check (! (std::abs (summed - expected) > 0.0f),
           juce::String ("[N4] two routes onto one destination sum in ascending slot order, bit-exact (")
           + num (summed, 9) + " vs " + num (expected, 9) + ")");
}

void checkReadSampleClamp()
{
    std::cout << "\n── IN-08: readSample frame-index clamp ──\n";

    // [X1] The arithmetic that produced the out-of-bounds read, reproduced here
    //      so the premise is asserted rather than asserted-about. This is the
    //      built-in negative control for the clamp: if the wrap did NOT round
    //      to exactly 1.0, or if kTableSize - 1 were the wrong bound, these
    //      fail.
    double phase = -1e-20;
    phase -= std::floor (phase);

    check (! (std::abs (phase - 1.0) > 0.0),
           juce::String ("[X1] wrapping -1e-20 yields EXACTLY 1.0, not 1.0-eps (got ")
           + num (phase, 17) + ")");

    const double samplePos = phase * static_cast<double> (WavetableData::kTableSize);
    const int idxUnclamped = static_cast<int> (samplePos);

    check (idxUnclamped == WavetableData::kTableSize,
           juce::String ("[X2] unclamped idx0 is ") + juce::String (idxUnclamped)
           + " — one past the last sample of a " + juce::String (WavetableData::kFrameSize)
           + "-element frame, so idx0+1 reads index " + juce::String (idxUnclamped + 1));

    const int idxClamped = std::min (idxUnclamped, WavetableData::kTableSize - 1);

    check (idxClamped + 1 <= WavetableData::kTableSize,
           juce::String ("[X3] the clamp keeps idx0+1 (") + juce::String (idxClamped + 1)
           + ") inside the frame (max index " + juce::String (WavetableData::kTableSize) + ")");

    // [X4] And it changes no audio. At idx0 = kTableSize-1 the fractional part
    //      is exactly 1.0, so the interpolation lands on the guard sample —
    //      which setGuardSamples() holds equal to sample 0. Same value the
    //      out-of-bounds version returned, which is why IN-08 is a safety fix
    //      and not an audio change. Driven through the real object: the public
    //      resetWithPhase() takes an unvalidated phase, which is how a caller
    //      could reach this at all.
    auto lib = WavetableFactory::createFactoryLibrary();

    if (lib.empty() || lib[0].table == nullptr)
    {
        check (false, "[X4] factory library unavailable");
        return;
    }

    const auto* table = lib[0].table.get();

    WavetableOscillator osc;
    osc.prepare (48000.0);
    osc.setWavetable (table);
    osc.setPosition (0.0f);
    osc.setWarpType (WarpType::Off);
    osc.setFrequency (440.0);
    osc.setUnison (1, 0.0f, 0.0f);
    osc.resetWithPhase (-1e-20);

    double outL = 0.0, outR = 0.0;
    osc.getNextSampleStereo (outL, outR);

    check (std::isfinite (outL) && std::isfinite (outR),
           juce::String ("[X4] a negative start phase renders finite (") + num (outL, 9) + ")");

    // [X5] The guard sample is the wrap point at EVERY level, which is what
    //      makes the clamp's redirect the correct read rather than merely an
    //      in-bounds one.
    int guardMismatches = 0;
    for (int level = 0; level < WavetableData::kNumMipmapLevels; ++level)
        for (int frame = 0; frame < table->numFrames; ++frame)
            if (std::abs (table->getSample (level, frame, WavetableData::kTableSize)
                          - table->getSample (level, frame, 0)) > 0.0f)
                ++guardMismatches;

    check (guardMismatches == 0,
           juce::String ("[X5] guard sample == sample 0 at every level and frame (")
           + juce::String (guardMismatches) + " mismatches over "
           + juce::String (WavetableData::kNumMipmapLevels * table->numFrames) + " frames)");

    // [X6] Audio-neutrality, stated level-agnostically. readSample interpolates
    //      across mipmap LEVELS as well as samples and frames - at 440 Hz and
    //      48 kHz it blends levels 4 and 5 - so comparing against level 0's
    //      sample 0 would be comparing the wrong quantity. Phase 1.0 and phase
    //      0.0 are the same point on the table, so the two renders must agree
    //      bit-for-bit. Post-clamp: idx0 = 2047 with frac exactly 1.0, and
    //      a + 1.0*(b-a) == b exactly for float operands widened to double.
    WavetableOscillator ref;
    ref.prepare (48000.0);
    ref.setWavetable (table);
    ref.setPosition (0.0f);
    ref.setWarpType (WarpType::Off);
    ref.setFrequency (440.0);
    ref.setUnison (1, 0.0f, 0.0f);
    ref.resetWithPhase (0.0);

    double refL = 0.0, refR = 0.0;
    ref.getNextSampleStereo (refL, refR);

    check (! (std::abs (outL - refL) > 0.0) && ! (std::abs (outR - refR) > 0.0),
           juce::String ("[X6] phase -1e-20 renders bit-identically to phase 0.0, so the clamp is "
                         "audio-neutral (") + num (outL, 12) + " vs " + num (refL, 12) + ")");

    check (std::abs (refL) > 0.0,
           juce::String ("[X6-nv] and that wrap point is a non-zero sample, so [X6] is not "
                         "comparing two zeroes (") + num (refL, 12) + ")");

    // [X7] THE NEGATIVE CONTROL, and the only assertion here that a clamp
    //      removal can fail.
    //
    //      [X1]-[X3] pin the arithmetic and [X6] pins audio-neutrality, but
    //      none of them can detect the defect: without the clamp, idx0 is 2048
    //      and frac is exactly 0.0, so the interpolation is
    //      `a + 0.0 * (b - a)` and the out-of-bounds b is multiplied away. The
    //      read happens; the value does not change. That is precisely why the
    //      bug sat latent.
    //
    //      It becomes observable if b is INFINITY, because 0.0 * inf is NaN.
    //      Index 2049 of a frame is index 0 of the next frame's slot, so
    //      poisoning frame 1's sample 0 with infinity puts an inf exactly where
    //      the overflowing read lands and nowhere a legitimate read reaches
    //      (position 0.0 gives frameFrac 0.0, and every in-bounds lookup at
    //      idx0 >= 2047 hits the guard, not sample 0). Unclamped -> NaN out;
    //      clamped -> finite.
    //
    //      Three frames, not two, so that frame 1's own idx0+1 lands in frame 2
    //      rather than off the end of the buffer — the test must not itself
    //      depend on an out-of-bounds read.
    WavetableData poisoned;
    poisoned.allocate (3);

    for (int level = 0; level < WavetableData::kNumMipmapLevels; ++level)
        for (int frame = 0; frame < poisoned.numFrames; ++frame)
            for (int i = 0; i < WavetableData::kTableSize; ++i)
                poisoned.setSample (level, frame, i,
                                    static_cast<float> (0.25 * std::sin (6.283185307179586
                                                        * static_cast<double> (i)
                                                        / WavetableData::kTableSize)));

    poisoned.setGuardSamples();

    for (int level = 0; level < WavetableData::kNumMipmapLevels; ++level)
        poisoned.setSample (level, 1, 0, std::numeric_limits<float>::infinity());

    WavetableOscillator trap;
    trap.prepare (48000.0);
    trap.setWavetable (&poisoned);
    trap.setPosition (0.0f);
    trap.setWarpType (WarpType::Off);
    trap.setFrequency (440.0);
    trap.setUnison (1, 0.0f, 0.0f);
    trap.resetWithPhase (-1e-20);

    double trapL = 0.0, trapR = 0.0;
    trap.getNextSampleStereo (trapL, trapR);

    check (std::isfinite (trapL) && std::isfinite (trapR),
           juce::String ("[X7] with an infinity planted one past frame 0, a negative start phase "
                         "still renders finite - the clamp never reads index ")
           + juce::String (WavetableData::kFrameSize) + " (" + num (trapL, 12) + ")");

    // [X7-nv] Prove the trap is armed: the same infinity IS reachable through a
    //         legitimate in-bounds read, so its absence above is the clamp
    //         working rather than the poison having failed to take.
    trap.setPosition (0.5f);
    trap.resetWithPhase (0.0);

    double armedL = 0.0, armedR = 0.0;
    trap.getNextSampleStereo (armedL, armedR);

    check (! std::isfinite (armedL),
           juce::String ("[X7-nv] the planted infinity is live - reading frame 1 sample 0 in "
                         "bounds does render non-finite (") + num (armedL, 12) + ")");
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "O-Prism v1.28.1 DSP-quality gate — WR-02, WR-04, WR-05, WR-06, WR-08"
                 " + IN-04/05/06/08\n";

    checkDeterminism();
    checkWarpLevelSelect();
    checkMonoDelay();
    checkPinkRate();
    checkGlideDomain();
    checkSvfRefactor();
    checkModMatrixActiveSlots();
    checkReadSampleClamp();

    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED"
                                        : "FAILED: " + std::to_string (failures) + " check(s)")
              << "\n";
    return failures;
}
