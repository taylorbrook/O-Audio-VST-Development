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

    O-ParametricEQ - display model (Stage 2, Phase 2.6; PLAN-2.6 P8;
    RESEARCH-2.6 Q4; ARCHITECTURE "True-response display")

    The section packets the page draws the curve from, designed on the
    message thread by this model's own eq::Designer with the processor's
    designer code, at the processor's prepared rate and layout, from the
    processor's own parameter values. The page composes the packets with
    the closed forms (G9); nothing in the curve comes from processBlock, so
    it is right with the transport stopped.

    Polling, not listeners. poll() reads the 196 parameter atomics through
    the processor's own raw-value cache (readBandSnapshot: the eight raw
    band values and the raw gain_scale, in the order the audio thread reads
    them), compares each band's nine-float snapshot by bit pattern with the
    last one, and designs only the bands that changed. A change of the
    prepared rate or layout marks all 24. There is no parameter listener
    and no async updater:
      - a parameter listener fires on whatever thread set the value (the
        audio thread under host automation) and would design there;
      - polling covers host automation, a preset load and a state restore
        with one rule. A one-shot push after a preset load goes stale (suite
        lesson); the page pulls allPackets() on init instead.

    Inputs: eq::effectiveBandInput (raw, gainScaleRaw, stereoLayout), the one
    definition the processor's makeInput also calls (BandParams.h).

    Raw parameter access: this class is a FRIEND of the processor. It calls
    the processor's private readBandSnapshot and readGainScaleRaw, so the
    raw-value cache stays off the public interface and there is one raw read
    path.

    The rate. preparedRate and preparedStereo are atomics the processor
    writes in prepareToPlay (release, after the taps' generation bump).
    Before the first prepare (preparedRate 0) the model designs at 48 kHz,
    stereo, and the header says rateKnown: false (the page can draw it,
    labelled). The polish decision depends on the rate (Designer::prepare),
    so the model never guesses a rate once one is known.

    `active` mirrors the engine: false when the band input is inactive
    (disabled or bypassed), and false when the band sleeps, i.e. a gain-
    bearing type (Bell, Low Shelf, High Shelf, Tilt) at exactly 0 dB
    effective gain. That is SlotBand's private gainIsZero rule, restated
    here as isGainType (type) && sameValue (gainDbEff, 0.0). The engine runs
    no design for such a band (after its 50 ms sleep count), so the display
    must not draw one. The model still designs every band (an inactive band
    keeps a defined design the page may show as a ghost); `active` says
    whether the engine runs it.

    Packets. packetFor (band) ->
      { band (0 ... 23), type (eq::Type index), placement (effective
        eq::Placement index), active,
        sections: [[g, k, mL, mB, mH] ...], onePoles: [[G, cL, cH] ...] }
    one entry per designed section in both arrays (numSections entries, in
    order). A section with no SVF half sends that half as the neutral values
    g = k = mL = mB = mH = 1, whose response (1 + x + x^2) / (1 + x + x^2)
    is 1; a section with no one-pole half sends G = cL = cH = 1, response
    (1 + x) / (1 + x) = 1. Halves are never omitted. (The designer writes
    the same ones into an absent half, with its flag false.)
    headerVar() -> { sampleRate, stereo, rateKnown }. allPackets() -> the 24
    packets in index order, for the page's init pull.

    The display shows the target. The audio reaches it within one 20 ms
    ramp, or 50 ms for a crossfade under a listen pre-ramp; the page is told
    nothing about ramps.

    Precision. A var double goes through JUCE's serialiseDouble: about 16
    significant digits in the normal range, 15 decimals in scientific form
    outside [1e-5, 1e6]. Rounding every parameter that way moves the drawn
    responses by <= 1.5e-8 dB (tap_bench `json`, 400 random 1 - 24 band
    draws per rate), far inside G9 (b)'s 0.01 dB. Designs are finite
    (DSP-06), so no NaN reaches JSON; the harness asserts it, this class does
    not guard it.

    Message thread only. No lock; the processor is read through atomics.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <cstddef>

#include "../dsp/BandParams.h"
#include "../dsp/Designer.h"
#include "../dsp/EqTypes.h"

class OuariconParametricEQAudioProcessor;

class DisplayModel
{
public:
    static constexpr int numBands = 24;
    static constexpr double fallbackRate = 48000.0;

    /** Nothing is designed until the first poll(), which reports all 24. */
    explicit DisplayModel (OuariconParametricEQAudioProcessor&);

    /** Reads the parameters and the prepared rate and layout; designs the
        bands whose snapshot changed (all 24 on the first call and after a
        rate or layout change). Returns the changed band indices, ascending. */
    juce::Array<int> poll();

    /** The packet of band `band` (0 ... 23) as of the last poll. An index
        out of range returns a void var. */
    juce::var packetFor (int band) const;

    /** { sampleRate, stereo, rateKnown } as of the last poll. */
    juce::var headerVar() const;

    /** The 24 packets, in index order. */
    juce::var allPackets() const;

    //==========================================================================
    // Read only, for the render harness (an index out of range reads band 0)
    //==========================================================================

    /** The model's design of the band (designer.design of its input's
        settings), as of the last poll. */
    const eq::BandDesign& getBandDesign (int band) const noexcept   { return entryAt (band).design; }

    /** The band input (effectiveBandInput) the design was made from. */
    const eq::BandInput& getBandInput (int band) const noexcept     { return entryAt (band).input; }

    /** The packet's `active`. */
    bool isBandActive (int band) const noexcept                     { return entryAt (band).active; }

    double getSampleRate() const noexcept   { return sampleRate; }
    bool isStereo() const noexcept          { return stereo; }
    bool isRateKnown() const noexcept       { return rateKnown; }

    /** Bands designed since construction. */
    int getDesignCount() const noexcept     { return designCount; }

    /** The engine's sleep rule (SlotBand::gainIsZero), restated. */
    static bool sleeps (const eq::BandInput&) noexcept;

private:
    struct Entry
    {
        std::array<float, 9> snapshot {};   // eight raw values + raw gain_scale, compared by bit pattern
        bool valid = false;
        eq::BandInput input {};
        eq::BandDesign design {};
        bool active = false;
    };

    const Entry& entryAt (int band) const noexcept
    {
        return entries[static_cast<std::size_t> ((band >= 0 && band < numBands) ? band : 0)];
    }

    OuariconParametricEQAudioProcessor& processor;
    eq::Designer designer;

    std::array<Entry, static_cast<std::size_t> (numBands)> entries;

    double sampleRate = fallbackRate;
    bool stereo = true;
    bool rateKnown = false;
    bool designerReady = false;
    int designCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DisplayModel)
};
