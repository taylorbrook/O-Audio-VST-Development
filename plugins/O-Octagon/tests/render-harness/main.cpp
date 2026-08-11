/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

    O-Octagon render harness — the PLUGIN-level half of the Phase 2.1 gate.

    Instantiates OOctagonProcessor directly, negotiates bus layouts programmatically, and renders
    offline. No DAW, no hardware, no MIDI.

      Q  Unity gain through all 8 outputs at create7point1() — MEASURED, not inspected
      R  1, 2 and 8 output channels: finite, non-crashing, correct SAFE/REAL selection
      S  The F3 hazard DIRECTLY — 7.1 layout, buffer of 3..7 channels, getTotalNumOutputChannels()
         still reporting 8. The state G1 describes, now a tested path rather than a reasoned one
      T  Session round-trip with a NON-DEFAULT venue — edited coordinates and a permuted label map
      U  A Stage-1-shaped session (no VENUE child at all) restores to the §OQ4 defaults

    Exit 0 iff every probe passes.

  ==============================================================================
*/
#include <JuceHeader.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "PluginProcessor.h"

//==============================================================================
namespace
{

int failures = 0;
int probes   = 0;

void check (const char* name, bool ok, const juce::String& detail)
{
    ++probes;

    if (! ok)
        ++failures;

    std::printf ("  [%s] %-30s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
}

bool near (float a, float b, float tol) noexcept
{
    return std::abs (a - b) <= tol;
}

/** Bit-exact float comparison via the object representation. No `==`, so no -Wfloat-equal. */
bool bitExact (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof (float)) == 0;
}

constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;

/** Negotiates a layout on a fresh processor and prepares it. Returns false if the predicate
    rejected the layout, which is itself a useful assertion. */
bool negotiate (OOctagonProcessor& proc,
                const juce::AudioChannelSet& in,
                const juce::AudioChannelSet& out)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (in);
    layout.outputBuses.add (out);

    if (! proc.setBusesLayout (layout))
        return false;

    proc.setRateAndBufferSizeDetails (kSampleRate, kBlockSize);
    proc.prepareToPlay (kSampleRate, kBlockSize);
    return true;
}

/** A deterministic, non-trivial test signal — a sine at an irrational-ish period so no block
    boundary lands on a zero crossing. */
float testSample (int n) noexcept
{
    return 0.5f * std::sin (static_cast<float> (n) * 0.0731f);
}

bool allFinite (const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int n = 0; n < b.getNumSamples(); ++n)
            if (! std::isfinite (b.getSample (ch, n)))
                return false;

    return true;
}

} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("\nO-Octagon render harness — Phase 2.1 (Geometry Core)\n");
    std::printf ("====================================================\n\n");

    const auto mono = juce::AudioChannelSet::mono();
    const auto set71 = juce::AudioChannelSet::create7point1();

    //==========================================================================
    // Q — Unity gain through all 8 outputs, MEASURED.
    //
    // Closes Stage-1 issue 3, where unity was confirmed by reading the code. All 8 lanes carry
    // IDENTICAL signal at this phase and that is correct — independence is FUNC-01 at Phase 2.2 —
    // but the LEVEL is a real claim and is now a measurement.
    {
        OOctagonProcessor proc;
        const bool negotiated = negotiate (proc, mono, set71);

        juce::AudioBuffer<float> buffer (8, kBlockSize);
        juce::MidiBuffer midi;

        buffer.clear();

        for (int n = 0; n < kBlockSize; ++n)
            buffer.setSample (0, n, testSample (n));

        proc.processBlock (buffer, midi);

        float worstErr = 0.0f;
        bool  everyChannelWritten = true;

        for (int ch = 0; ch < 8; ++ch)
        {
            float chErr = 0.0f;

            for (int n = 0; n < kBlockSize; ++n)
                chErr = std::max (chErr, std::abs (buffer.getSample (ch, n) - testSample (n)));

            worstErr = std::max (worstErr, chErr);

            if (buffer.getMagnitude (ch, 0, kBlockSize) < 1.0e-6f)
                everyChannelWritten = false;
        }

        const bool ok = negotiated && everyChannelWritten && worstErr <= 1.0e-6f && allFinite (buffer);

        check ("Q unity-gain-8-outputs", ok,
               juce::String (negotiated ? "7.1 negotiated, " : "LAYOUT REJECTED, ")
                   + "max |out - in| = " + juce::String (worstErr, 9) + " across all 8 lanes"
                   + (everyChannelWritten ? "" : ", A LANE IS SILENT"));
    }

    //==========================================================================
    // R — 1, 2 and 8 output channels, constructed programmatically. No hardware needed.
    //
    // Closes Stage-1 issue 4. JUCE derives the AU channel-config set from
    // isBusesLayoutSupported(), so auval exercises (1,1), (1,2), (1,8), (2,1), (2,2) and (2,8) —
    // the SAFE path is load-bearing for AU, not only for Standalone on a stereo interface.
    {
        struct Case { const char* name; juce::AudioChannelSet out; int channels; bool expectMapped; };

        const std::array<Case, 3> cases
            { { { "mono",   juce::AudioChannelSet::mono(),   1, false },
                { "stereo", juce::AudioChannelSet::stereo(), 2, false },
                { "7.1",    set71,                           8, true  } } };

        bool ok = true;
        juce::String detail;

        for (const auto& c : cases)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, mono, c.out))
            {
                ok = false;
                detail << c.name << ": REJECTED; ";
                continue;
            }

            juce::AudioBuffer<float> buffer (c.channels, kBlockSize);
            juce::MidiBuffer midi;
            buffer.clear();

            for (int n = 0; n < kBlockSize; ++n)
                buffer.setSample (0, n, testSample (n));

            proc.processBlock (buffer, midi);

            // The REAL path is selected iff the buffer is 8 channels AND the map is valid; the
            // mapped case must additionally be a permutation, i.e. every lane written.
            const bool mapValid = ! proc.isChannelMapInvalid();
            bool everyLaneWritten = true;

            for (int ch = 0; ch < c.channels; ++ch)
                if (buffer.getMagnitude (ch, 0, kBlockSize) < 1.0e-6f)
                    everyLaneWritten = false;

            const bool good = allFinite (buffer) && everyLaneWritten && mapValid == c.expectMapped;

            ok = ok && good;
            detail << c.name << "(" << c.channels << "ch) " << (good ? "ok" : "BAD") << "; ";
        }

        check ("R bus-layouts-1-2-8", ok, detail);
    }

    //==========================================================================
    // S — THE F3 HAZARD, DIRECTLY.
    //
    // Standalone on a 3-7 output device: canonicalChannelSet(n) is rejected by the predicate, Debug
    // asserts, and RELEASE KEEPS THE 7.1 LAYOUT while the buffer arrives with n channels. In that
    // state mapInvalid is false and speakerToBuffer holds indices up to 7 — a valid map is NOT
    // evidence of an 8-channel buffer (G1).
    //
    // Both halves are asserted: that the hazardous state is genuinely reproduced (the accessor
    // really does report 8 while the buffer does not), and that nothing writes out of bounds.
    {
        bool ok = true;
        bool reproducedHazard = true;
        juce::String detail;

        for (int n = 3; n <= 7; ++n)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, mono, set71))
            {
                ok = false;
                detail << "7.1 rejected; ";
                break;
            }

            // Two extra canary channels beyond what the AudioBuffer will reference. A per-sample
            // overrun inside a channel, or a stray write through a stale pointer, disturbs them.
            std::vector<std::vector<float>> storage (static_cast<size_t> (n) + 2,
                                                     std::vector<float> (kBlockSize, 0.0f));
            constexpr float canary = -12345.0f;

            for (size_t c = static_cast<size_t> (n); c < storage.size(); ++c)
                std::fill (storage[c].begin(), storage[c].end(), canary);

            std::vector<float*> pointers;

            for (int c = 0; c < n; ++c)
                pointers.push_back (storage[(size_t) c].data());

            for (int s = 0; s < kBlockSize; ++s)
                storage[0][(size_t) s] = testSample (s);

            juce::AudioBuffer<float> buffer (pointers.data(), n, kBlockSize);
            juce::MidiBuffer midi;

            // THE HAZARD, stated as an assertion rather than a comment: the accessor lies here.
            if (! (proc.getTotalNumOutputChannels() == 8 && buffer.getNumChannels() == n))
                reproducedHazard = false;

            proc.processBlock (buffer, midi);

            bool canariesIntact = true;

            for (size_t c = static_cast<size_t> (n); c < storage.size(); ++c)
                for (float v : storage[c])
                    if (! bitExact (v, canary))
                        canariesIntact = false;

            const bool good = allFinite (buffer) && canariesIntact;

            ok = ok && good;

            if (! good)
                detail << n << "ch BAD; ";
        }

        ok = ok && reproducedHazard;

        if (ok)
            detail = "3..7 ch buffers under a 7.1 layout: no OOB, finite, hazard state confirmed "
                     "(getTotalNumOutputChannels()==8 while the buffer is narrower)";
        else if (! reproducedHazard)
            detail << "the F3 state was NOT reproduced — this probe would be vacuous";

        check ("S f3-narrow-buffer-hazard", ok, detail);
    }

    //==========================================================================
    // T — Session round-trip with a NON-DEFAULT venue.
    //
    // Edited coordinates AND a permuted label map, because the identity map round-trips
    // identically whether or not the label layer works at all.
    {
        OOctagonProcessor source;
        negotiate (source, mono, set71);

        oo::VenueModel edited = source.getVenue();

        for (int i = 0; i < 8; ++i)
            edited.setSpeakerPosition (i, { 1.5f + static_cast<float> (i) * 1.375f,
                                            2.25f + static_cast<float> (i) * 2.125f,
                                            3.875f + static_cast<float> (i) * 0.0625f });

        edited.setRake (0.875f, 4.625f);
        edited.setName ("Round-trip fixture");

        for (int i = 0; i < 8; ++i)
            edited.setSpeakerTrimDb (i, -3.5f + static_cast<float> (i) * 0.75f);

        // Rotate the label map by one — a non-identity assignment under create7point1().
        const std::array<const char*, 8> rotated
            { "R", "C", "Lfe", "Lss", "Rss", "Lrs", "Rrs", "L" };

        for (int i = 0; i < 8; ++i)
            edited.setSpeakerLabel (i, rotated[(size_t) i]);

        source.applyVenueEdit (edited);

        juce::MemoryBlock blob;
        source.getStateInformation (blob);

        OOctagonProcessor restored;
        negotiate (restored, mono, set71);
        restored.setStateInformation (blob.getData(), static_cast<int> (blob.getSize()));

        const auto& a = source.getVenue();
        const auto& b = restored.getVenue();

        bool same = bitExact (a.rakeFront(), b.rakeFront()) && bitExact (a.rakeRear(), b.rakeRear());
        int  mismatches = 0;

        for (int i = 0; i < 8; ++i)
        {
            const auto pa = a.speaker (i);
            const auto pb = b.speaker (i);

            const bool speakerSame = bitExact (pa.x, pb.x) && bitExact (pa.y, pb.y)
                                  && bitExact (pa.z, pb.z)
                                  && bitExact (a.trimDb (i), b.trimDb (i))
                                  && a.labelAbbreviation (i) == b.labelAbbreviation (i);

            if (! speakerSame)
                ++mismatches;

            same = same && speakerSame;
        }

        // The restored map must be the rotated one, not the identity — proving the label layer
        // survived the round trip rather than the venue silently falling back to defaults.
        const bool mapValid = ! restored.isChannelMapInvalid();
        const bool nonDefault = ! bitExact (b.speaker (0).x, oo::VenueModel {}.speaker (0).x);

        const bool ok = same && mismatches == 0 && mapValid && nonDefault;

        check ("T venue-session-round-trip", ok,
               juce::String ("42 values, ") + juce::String (mismatches) + " speaker mismatch(es)"
                   + (mapValid ? ", map valid" : ", MAP INVALID")
                   + (nonDefault ? ", venue is non-default" : ", VENUE FELL BACK TO DEFAULTS"));
    }

    //==========================================================================
    // U — A Stage-1-shaped session: 17 parameters and NO VENUE child at all.
    //
    // This is not an edge case. Every project saved between Stage 1 and now takes exactly this
    // path, and it must produce the §OQ4 default venue silently, without error.
    {
        OOctagonProcessor source;
        negotiate (source, mono, set71);

        juce::MemoryBlock blob;
        source.getStateInformation (blob);

        // Strip the VENUE child to manufacture a Stage-1-shaped blob from a current one.
        auto xml = juce::AudioProcessor::getXmlFromBinary (blob.getData(),
                                                           static_cast<int> (blob.getSize()));

        bool strippedOne = false;

        if (xml != nullptr)
        {
            if (auto* venueElement = xml->getChildByName (oo::VenueModel::venueTag.toString()))
            {
                xml->removeChildElement (venueElement, true);
                strippedOne = true;
            }
        }

        juce::MemoryBlock stage1Blob;

        if (xml != nullptr)
            juce::AudioProcessor::copyXmlToBinary (*xml, stage1Blob);

        OOctagonProcessor restored;
        negotiate (restored, mono, set71);
        restored.setStateInformation (stage1Blob.getData(), static_cast<int> (stage1Blob.getSize()));

        const oo::VenueModel reference;
        const auto& v = restored.getVenue();

        bool same = near (v.rakeFront(), reference.rakeFront(), 1.0e-6f)
                 && near (v.rakeRear(),  reference.rakeRear(),  1.0e-6f);

        for (int i = 0; i < 8; ++i)
        {
            const auto pa = v.speaker (i);
            const auto pb = reference.speaker (i);

            same = same && bitExact (pa.x, pb.x) && bitExact (pa.y, pb.y) && bitExact (pa.z, pb.z)
                        && v.labelAbbreviation (i) == reference.labelAbbreviation (i);
        }

        const bool ok = strippedOne && same && ! restored.isChannelMapInvalid();

        check ("U stage1-session-defaults", ok,
               juce::String (strippedOne ? "VENUE stripped, " : "NOTHING TO STRIP — probe vacuous, ")
                   + (same ? "restored to §OQ4 defaults" : "DID NOT restore to defaults")
                   + (restored.isChannelMapInvalid() ? ", MAP INVALID" : ", map valid"));
    }

    //==========================================================================
    std::printf ("\n----------------------------------------------------\n");
    std::printf ("  %d probe(s), %d failure(s)\n\n", probes, failures);

    return failures == 0 ? 0 : 1;
}
