/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - RngBank (Stage 2, Phase 2.1)

    Thirteen named juce::Random streams — one per stochastic subsystem. A
    single shared audio-thread RNG breaks block-size invariance the moment two
    subsystems interleave their draws differently at different block sizes
    (pattern_rng_stream_interleave_blocksize), so every subsystem owns a
    stream and draws are consumed either at ticks/packets in fixed roll order
    or, for the continuous beds, on a private fixed per-sample schedule.

    Stream k is seeded splitmix64(SEED * 0x9E3779B97F4A7C15 + k) — decorrelated
    derived seeds from the single user SEED parameter. reseed() is called from
    prepareToPlay and from the per-block seed-change detection (allocation-free,
    audio-thread safe).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class RngBank
{
public:
    enum StreamId
    {
        arbitration = 0,
        tape,
        cd,
        vinyl,
        packet,
        jitter,
        dither,
        artifactSynth,

        // v1.4.0. APPENDED, never inserted: stream k is seeded from a function
        // of k alone, so adding an id at the END leaves every pre-existing
        // stream's seed — and therefore every render made with an old SEED —
        // bit-identical. Inserting one would renumber the streams after it.
        //
        // wow is the ONE stream drawn outside a tick: the wow/flutter bed
        // redraws partial amplitudes on its own fixed sample counter. That is
        // safe precisely because it is dedicated — the interleave hazard is
        // two subsystems SHARING a stream at different block-relative
        // instants (pattern_rng_stream_interleave_blocksize), and a private
        // stream on a sample-count schedule is block-size invariant.
        wow,

        // v1.5.0. Appended for exactly the same reason, and drawn on exactly
        // the same terms as `wow`: each media-noise bed owns a PRIVATE stream
        // and draws a fixed count per sample on its own sample schedule. That
        // is block-size invariant — the interleave hazard is two subsystems
        // SHARING a stream at different block-relative instants
        // (pattern_rng_stream_interleave_blocksize), not per-sample draws as
        // such (crush jitter and dither have drawn per-sample since 2.4).
        //
        // The hum in the codec bed takes no draws at all — it is a pure
        // function of the sample count, by design.
        tapeBed,       // hiss (stereo: 2 draws/sample, decorrelated channels)
        vinylBed,      // rumble + Poisson micro-ticks (mono)
        codecBed,      // crackle bursts (mono; the mains hum is RNG-free)
        comfort,       // packet comfort noise (stereo)

        // v1.7.0. Appended on exactly the same terms: the vinyl pop taxonomy's
        // rare scratch class is a 2-8 ms NOISE BURST, so it needs a draw per
        // sample while it runs, and the artifactSynth stream it would otherwise
        // share is consumed at TICK instants. Mixing the two on one stream is
        // the block-size interleave hazard verbatim
        // (pattern_rng_stream_interleave_blocksize); a private stream advanced
        // on a sample counter from a sample-accurate trigger is invariant.
        scratch,       // vinyl scratch-class noise burst (mono)

        numStreams
    };

    // Allocation-free; safe on the audio thread (seed-change detection) and
    // in prepareToPlay.
    void reseed (int seed) noexcept
    {
        for (int k = 0; k < numStreams; ++k)
        {
            const auto base = static_cast<juce::uint64> (static_cast<juce::int64> (seed))
                                  * 0x9E3779B97F4A7C15ULL
                              + static_cast<juce::uint64> (k);
            streams[k].setSeed (static_cast<juce::int64> (splitmix64 (base)));
        }
    }

    juce::Random& get (StreamId id) noexcept { return streams[id]; }

private:
    static juce::uint64 splitmix64 (juce::uint64 z) noexcept
    {
        z += 0x9E3779B97F4A7C15ULL;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    juce::Random streams[numStreams];
};
