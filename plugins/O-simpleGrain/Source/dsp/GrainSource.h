/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain - Grain Source (v1.7.0)

    The published, immutable source: the engine-rate buffer plus its band-limited
    octave mipmap (grain-engine BandlimitedReader). Built whole OFF the audio
    thread by build() before it is published, so the audio thread sees the buffer
    and its levels as one object behind one pointer and can never pair a new
    buffer with an old mipmap.

    Grains at rate <= 1 still read `buffer` through the 4-point Lagrange (renders
    with no up-transposition are bit-identical to v1.6.0). Grains at rate > 1 read
    `mip` at the octave level latched from their rate, through the 16-tap sinc set
    of the residual rate — this replaces the v1.6.0 post-read one-pole, which
    rolled off at 6 dB/oct and left the aliases in.

    Mipmap build: a temporary MipRing is fed kPreRoll copies of the first sample,
    the source, then kTail copies of the last sample, and a MipSnapshot captures
    the region whose origin is the first source sample. The edge padding makes a
    read before the start or past the end return the edge value on every level —
    the same hold the Lagrange clamp gives — and covers the cascade's settling:
    level l's impulse response spans ~127 (2^l - 1) level-0 samples (8001 at
    l = 6), so the pre-roll zero history has died out before b = 0.

    Channel 0 only (the engine reads mono). MipSnapshot is stereo, so channel 1
    of every level is allocated and zero.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "BandlimitedReader.h"   // grain-engine 1.1.0 (modules/synthesis/grain-engine/cpp)

struct GrainSource
{
    // 6 octave levels: residual rate <= 2 (every audible spurious <= -60 dB) up to
    // rate 128, best effort to 256. The key alone reaches 2^(67/12) ~ 48 from C3.
    static constexpr int kMipLevels = 6;
    using Ring     = ouaricon::grain::MipRing<kMipLevels>;
    using Snapshot = ouaricon::grain::MipSnapshot<kMipLevels>;

    static constexpr int kPreRoll  = 16384;                              // > 2 x 8001 settle
    static constexpr int kEndDelay = Ring::margin (kMipLevels);         // 4482: every level settled
    static constexpr int kTail     = kEndDelay + 8 * (1 << kMipLevels) + 3200;   // hold past the end

    juce::AudioBuffer<float> buffer;   // engine-rate source (channel 0 is the engine's)
    Snapshot mip;                      // every octave level of channel 0, origin = sample 0

    int getNumSamples() const noexcept { return buffer.getNumSamples(); }

    // Builds the mipmap from channel 0 of `buffer`. Allocates; message / host
    // threads only, before the source is published.
    void build()
    {
        const int n = buffer.getNumSamples();
        if (n <= 0)
            return;

        const float* x = buffer.getReadPointer (0);
        const int total = kPreRoll + n + kTail;

        Ring ring;
        ring.prepare (total);
        for (int i = 0; i < kPreRoll; ++i) ring.push (x[0], 0.0f);
        for (int i = 0; i < n; ++i)        ring.push (x[i], 0.0f);
        for (int i = 0; i < kTail; ++i)    ring.push (x[n - 1], 0.0f);

        const int len = total - kEndDelay - kPreRoll;     // source + held tail
        mip.prepare (len);
        mip.captureFrom (ring, kEndDelay, len);
        jassert (mip.getOrigin() == kPreRoll);
    }
};
