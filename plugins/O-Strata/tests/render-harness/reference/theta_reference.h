// Copied from Source/dsp/WavetableOscillator.cpp at efae0bfc (Source/ identical at
// 378da673) before its deletion in Phase 2.1 — the H1 θ-parity reference.
//
// Header-only, JUCE-free analytic θ pipeline: applyWarp (Bend pow (phase, 1 + 3a),
// FM frac (phase + fmIn·a)), the Sync / Window branch (syncRatio = 1 + 3a, master
// re-seed on wrap, × sin (π·masterPhase)), the unison laws (gain 1/√n, centerIndex,
// detuneFactor 2^(pos·detune·50/1200), pan cos / sin (panNorm·π/2)), the LCG
// resetWithRandomPhases with an explicit seed and resetWithPhase.
//
// renderCosReference() produces y[n] = Σ_partials gain·pan·cos (2π·θ_i[n]) through the
// SAME 5 Hz one-pole DC blocker the oscillator applies after its unison sum (plan
// Decision 3; RESEARCH §2.6 blocker i). The FM row runs two oscillators: B is the
// modulator, its mono DC-blocked output of the PREVIOUS sample feeds A's phase
// (StrataVoice: oscA.setFMInput (lastOscBOut)).

#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace theta_ref
{
    constexpr double kPi     = 3.141592653589793;
    constexpr double kTwoPi  = 6.283185307179586;
    constexpr double kHalfPi = 1.5707963267948966;
    constexpr int    kMaxUnison = 4;

    enum class WarpType { Off = 0, Sync, Bend, FM, Window };

    struct OscParams
    {
        double   frequency  = 261.6256;
        int      unison     = 1;
        float    detune     = 0.2f;
        float    width      = 0.5f;
        WarpType warp       = WarpType::Off;
        float    warpAmount = 0.0f;
        double   startPhase = 0.25;   // resetWithPhase; < 0 → resetWithRandomPhases (seed)
        uint32_t seed       = 0;
    };

    struct Osc
    {
        double fs = 48000.0;
        double phaseIncrement = 0.0;
        double phaseAccumulators[kMaxUnison] = {};
        double masterPhases[kMaxUnison] = {};
        double unisonDetuneFactors[kMaxUnison] = { 1.0, 1.0, 1.0, 1.0 };
        double unisonPanL[kMaxUnison] = { 1.0, 1.0, 1.0, 1.0 };
        double unisonPanR[kMaxUnison] = { 1.0, 1.0, 1.0, 1.0 };
        double unisonGain = 1.0;
        int    unisonCount = 1;
        WarpType warpType = WarpType::Off;
        float  warpAmount = 0.0f;
        double fmInput = 0.0;
        // DC blocker (per oscillator, after the unison sum)
        double R = 1.0, x1L = 0, y1L = 0, x1R = 0, y1R = 0;

        void prepare (double sampleRate, const OscParams& p)
        {
            fs = sampleRate;
            phaseIncrement = p.frequency / fs;
            R = 1.0 - kTwoPi * 5.0 / fs;
            setUnison (p.unison, p.detune, p.width);
            warpType = p.warp;
            warpAmount = p.warpAmount;
            if (p.startPhase >= 0.0) resetWithPhase (p.startPhase);
            else                     resetWithRandomPhases (p.seed);
            x1L = y1L = x1R = y1R = 0.0;
            fmInput = 0.0;
        }

        // WavetableOscillator::setUnison, verbatim laws
        void setUnison (int count, float detune, float width)
        {
            unisonCount = count < 1 ? 1 : (count > kMaxUnison ? kMaxUnison : count);
            if (unisonCount == 1)
            {
                unisonDetuneFactors[0] = 1.0; unisonPanL[0] = 1.0; unisonPanR[0] = 1.0; unisonGain = 1.0;
                return;
            }
            unisonGain = 1.0 / std::sqrt (static_cast<double> (unisonCount));
            const double centerIndex = (unisonCount - 1) / 2.0;
            const double normFactor = centerIndex;
            for (int i = 0; i < unisonCount; ++i)
            {
                const double normalizedPos = (i - centerIndex) / normFactor;
                unisonDetuneFactors[i] = std::pow (2.0, normalizedPos * detune * 50.0 / 1200.0);
                double panNorm = (normalizedPos * width + 1.0) * 0.5;
                panNorm = panNorm < 0.0 ? 0.0 : (panNorm > 1.0 ? 1.0 : panNorm);
                unisonPanL[i] = std::cos (panNorm * kHalfPi);
                unisonPanR[i] = std::sin (panNorm * kHalfPi);
            }
        }

        void resetWithPhase (double phase)
        {
            for (int i = 0; i < kMaxUnison; ++i) { phaseAccumulators[i] = phase; masterPhases[i] = phase; }
        }

        // LCG, explicit seed (TerrainOscillator::resetWithRandomPhases (seed != 0))
        void resetWithRandomPhases (uint32_t seed)
        {
            for (int i = 0; i < kMaxUnison; ++i)
            {
                seed = seed * 1664525u + 1013904223u;
                phaseAccumulators[i] = static_cast<double> (seed) / 4294967296.0;
                masterPhases[i] = phaseAccumulators[i];
            }
        }

        // WavetableOscillator::applyWarp, verbatim
        double applyWarp (double phase) const
        {
            switch (warpType)
            {
                case WarpType::Bend:
                {
                    const double exponent = 1.0 + static_cast<double> (warpAmount) * 3.0;
                    return std::pow (phase, exponent);
                }
                case WarpType::FM:
                {
                    double warped = phase + fmInput * static_cast<double> (warpAmount);
                    warped = warped - std::floor (warped);
                    return warped;
                }
                default:
                    return phase;
            }
        }

        static double cosOf (double phase)
        {
            if (! std::isfinite (phase)) return 0.0;
            phase -= std::floor (phase);
            return std::cos (kTwoPi * phase);
        }

        // WavetableOscillator::getNextSampleStereo with readSample → cos (2πφ), then the DC blocker
        void next (double& outL, double& outR)
        {
            outL = 0.0; outR = 0.0;
            const bool isSyncMode = (warpType == WarpType::Sync || warpType == WarpType::Window)
                                    && warpAmount > 0.001f;
            for (int i = 0; i < unisonCount; ++i)
            {
                const double readPhase = phaseAccumulators[i];
                if (isSyncMode)
                {
                    double sample = cosOf (readPhase) * unisonGain;
                    if (warpType == WarpType::Window)
                        sample *= std::sin (kPi * masterPhases[i]);
                    outL += sample * unisonPanL[i];
                    outR += sample * unisonPanR[i];

                    const double masterInc = phaseIncrement * unisonDetuneFactors[i];
                    masterPhases[i] += masterInc;
                    const double syncRatio = 1.0 + static_cast<double> (warpAmount) * 3.0;
                    phaseAccumulators[i] += masterInc * syncRatio;
                    if (phaseAccumulators[i] >= 1.0)
                        phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
                    if (masterPhases[i] >= 1.0)
                    {
                        masterPhases[i] -= std::floor (masterPhases[i]);
                        phaseAccumulators[i] = masterPhases[i] * syncRatio;
                        phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
                    }
                }
                else
                {
                    const double warped = applyWarp (readPhase);
                    const double sample = cosOf (warped) * unisonGain;
                    outL += sample * unisonPanL[i];
                    outR += sample * unisonPanR[i];
                    phaseAccumulators[i] += phaseIncrement * unisonDetuneFactors[i];
                    if (phaseAccumulators[i] >= 1.0)
                        phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
                }
            }
            // 5 Hz one-pole DC blocker, per channel (TerrainOscillator output conditioning)
            const double bL = outL - x1L + R * y1L; x1L = outL; y1L = bL; outL = bL;
            const double bR = outR - x1R + R * y1R; x1R = outR; y1R = bR; outR = bR;
        }
    };

    /** Renders N samples of oscillator A (params a). When a.warp == FM, oscillator B
        (params b, same pipeline) is the modulator: A's fmInput for sample n is B's
        mono output of sample n − 1 (0 for n = 0), as StrataVoice does. */
    inline void renderCosReference (const OscParams& a, const OscParams& b, int N, double fs,
                                    std::vector<double>& outL, std::vector<double>& outR)
    {
        Osc A, B;
        A.prepare (fs, a);
        B.prepare (fs, b);
        outL.assign ((size_t) N, 0.0);
        outR.assign ((size_t) N, 0.0);
        double lastB = 0.0;
        for (int n = 0; n < N; ++n)
        {
            A.fmInput = lastB;
            double l, r;
            A.next (l, r);
            outL[(size_t) n] = l; outR[(size_t) n] = r;
            double bl, br;
            B.next (bl, br);
            lastB = (bl + br) * 0.5;
        }
    }
}
