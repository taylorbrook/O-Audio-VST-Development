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

    NoiseGenerator.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "NoiseGenerator.h"
#include "MathConstants.h"
#include <cmath>

void NoiseGenerator::setSeed (juce::uint32 seed)
{
    // 0x9E3779B9 is the golden-ratio constant; adding it decorrelates the two
    // channel seeds in the HIGH bits, which an LCG carries better than the low
    // ones a small XOR would have touched.
    rngSeedL = seed;
    rngSeedR = seed + 0x9E3779B9u;
    randomL.setSeed (static_cast<juce::int64> (rngSeedL));
    randomR.setSeed (static_cast<juce::int64> (rngSeedR));
}

void NoiseGenerator::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    digitalHoldSamples = std::max (1, static_cast<int> (sampleRate / 5512.0));

    // WR-05 (v1.28.0): investigated and NOT changed. See the retraction note at
    // the pink coefficients in getNextSampleStereo().

    // WR-06: rewind both streams to their seeds. prepare() runs once per
    // render, so two bounces of the same project draw identical noise.
    // Deliberately NOT done in reset(): reset() fires on every note-on, and
    // re-seeding there would restart the same noise burst on each note, which
    // reads as a pitched artefact rather than as noise.
    randomL.setSeed (static_cast<juce::int64> (rngSeedL));
    randomR.setSeed (static_cast<juce::int64> (rngSeedR));
}

void NoiseGenerator::reset()
{
    b0L = b1L = b2L = 0.0;
    b0R = b1R = b2R = 0.0;
    brownStateL = 0.0;
    brownStateR = 0.0;
    digitalHoldValueL = digitalHoldValueR = 0.0;
    digitalCounterL = digitalCounterR = 0;
    vinylBP1L = vinylBP2L = 0.0;
    vinylBP1R = vinylBP2R = 0.0;
    crackleDecayL = crackleDecayR = 0.0;
    windLFOPhase = 0.0;
    windLPStateL = windLPStateR = 0.0;
    windBrownStateL = windBrownStateR = 0.0;
}

void NoiseGenerator::setType (int type)
{
    currentType = type;
}

void NoiseGenerator::getNextSampleStereo (double& outL, double& outR)
{
    double whiteL = randomL.nextDouble() * 2.0 - 1.0;
    double whiteR = randomR.nextDouble() * 2.0 - 1.0;

    switch (currentType)
    {
        case 0: // White
            outL = whiteL * 0.3;
            outR = whiteR * 0.3;
            return;

        case 1: // Pink (Paul Kellet economy) — independent filter states per channel
        {
            // WR-05 RETRACTED in v1.28.0 — do NOT "fix" these constants.
            //
            // The review found that these three poles are published for
            // 44.1 kHz and that Brown, Vinyl and Wind all scale their
            // coefficients by the rate while Pink does not, and concluded the
            // bed is "measurably brighter at 48 kHz and noticeably so at
            // 96 kHz". The mechanism is real for a pole in isolation; the
            // conclusion does not survive measurement.
            //
            // Octave-band power per Hz, 125 Hz to 16 kHz, relative to 44.1 kHz
            // (see tests/dsp_quality_check.cpp, which gates these numbers):
            //
            //   as shipped                        48 kHz 0.085 dB   96 kHz 0.546 dB
            //   poles warped a' = a^(44100/fs)    48 kHz 0.651 dB   96 kHz 4.993 dB
            //   that plus the white-PSD rescale   48 kHz 0.283 dB   96 kHz 1.615 dB
            //
            // Every correction is WORSE than doing nothing. Preserving each
            // pole's frequency and each section's DC gain is not the same as
            // preserving the SUMMED response, and the sum is what is audible:
            // the three sections plus the flat direct term happen to combine
            // into a magnitude that is already nearly rate-invariant, because
            // the w-compression at a higher rate offsets the 1/fs fall in white
            // noise power per Hz almost exactly.
            //
            // 0.085 dB at 48 kHz is not "measurably brighter" by any standard.
            b0L = 0.99765 * b0L + whiteL * 0.0990460;
            b1L = 0.96300 * b1L + whiteL * 0.2965164;
            b2L = 0.57000 * b2L + whiteL * 1.0526913;
            outL = (b0L + b1L + b2L + whiteL * 0.1848) * 0.11;

            b0R = 0.99765 * b0R + whiteR * 0.0990460;
            b1R = 0.96300 * b1R + whiteR * 0.2965164;
            b2R = 0.57000 * b2R + whiteR * 1.0526913;
            outR = (b0R + b1R + b2R + whiteR * 0.1848) * 0.11;
            return;
        }

        case 2: // Brown — independent integrator states per channel
        {
            double rateScale = 44100.0 / currentSampleRate;
            brownStateL += whiteL * 0.02 * rateScale;
            brownStateL *= 0.998;
            outL = std::tanh (brownStateL * 2.0) * 0.35;

            brownStateR += whiteR * 0.02 * rateScale;
            brownStateR *= 0.998;
            outR = std::tanh (brownStateR * 2.0) * 0.35;
            return;
        }

        case 3: // Digital (sample-and-hold, quantized) — independent holds per channel
        {
            if (++digitalCounterL >= digitalHoldSamples)
            {
                digitalCounterL = 0;
                double raw = randomL.nextDouble();
                digitalHoldValueL = (std::floor (raw * 8.0) / 7.0) * 2.0 - 1.0;
            }
            if (++digitalCounterR >= digitalHoldSamples)
            {
                digitalCounterR = 0;
                double raw = randomR.nextDouble();
                digitalHoldValueR = (std::floor (raw * 8.0) / 7.0) * 2.0 - 1.0;
            }
            outL = digitalHoldValueL * 0.3;
            outR = digitalHoldValueR * 0.3;
            return;
        }

        case 4: // Vinyl (bandpass white + crackle) — independent filter/crackle per channel
        {
            double cutNorm = 2000.0 / currentSampleRate;
            double fb = 0.5;

            vinylBP1L += cutNorm * (whiteL - vinylBP1L - fb * vinylBP2L);
            vinylBP2L += cutNorm * vinylBP1L;
            double vinylL = vinylBP2L * 1.2;

            vinylBP1R += cutNorm * (whiteR - vinylBP1R - fb * vinylBP2R);
            vinylBP2R += cutNorm * vinylBP1R;
            double vinylR = vinylBP2R * 1.2;

            // Poisson crackle (~3 per second) — independent per channel
            if (randomL.nextDouble() < 3.0 / currentSampleRate)
                crackleDecayL = (randomL.nextDouble() * 0.3 + 0.3) * (randomL.nextBool() ? 1.0 : -1.0);
            crackleDecayL *= 0.95;
            vinylL += crackleDecayL;

            if (randomR.nextDouble() < 3.0 / currentSampleRate)
                crackleDecayR = (randomR.nextDouble() * 0.3 + 0.3) * (randomR.nextBool() ? 1.0 : -1.0);
            crackleDecayR *= 0.95;
            vinylR += crackleDecayR;

            outL = std::tanh (vinylL) * 0.35;
            outR = std::tanh (vinylR) * 0.35;
            return;
        }

        case 5: // Wind (LFO-modulated lowpass on brown) — shared LFO, independent brown + LP per channel
        {
            double rateScale = 44100.0 / currentSampleRate;
            windBrownStateL += whiteL * 0.02 * rateScale;
            windBrownStateL *= 0.998;
            windBrownStateR += whiteR * 0.02 * rateScale;
            windBrownStateR *= 0.998;

            // Shared LFO at 0.2Hz modulates cutoff 50-550Hz (same for both channels)
            windLFOPhase += 0.2 / currentSampleRate;
            if (windLFOPhase >= 1.0) windLFOPhase -= 1.0;
            double lfoVal = (std::sin (windLFOPhase * kTwoPi) + 1.0) * 0.5;
            double cutoff = 50.0 + lfoVal * 500.0;
            double alpha = cutoff / (cutoff + currentSampleRate / kTwoPi);

            windLPStateL += alpha * (windBrownStateL - windLPStateL);
            windLPStateR += alpha * (windBrownStateR - windLPStateR);

            outL = std::tanh (windLPStateL * 2.0) * 0.35;
            outR = std::tanh (windLPStateR * 2.0) * 0.35;
            return;
        }
    }

    outL = whiteL * 0.3;
    outR = whiteR * 0.3;
}
