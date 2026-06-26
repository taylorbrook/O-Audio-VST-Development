/*
  ==============================================================================

    O-simpleBeatmaker - SequencerClock (host transport -> firing step columns)

    Reads the host transport (passed in from processBlock, where getPosition() is
    legal) once per block and enumerates every step-column boundary that falls in
    the block's ppq window [ppqStart, ppqStart+blockPpq). Straight time only — the
    per-hit Δt is the TimingFeelEngine's job.

    Pattern alignment is PERIOD-aligned to the global ppq origin
    (barStart = ppqStart − fmod(ppqStart, patternLen*0.25)), which is robust for
    8 / 16 / 32-step patterns (last-bar-start alignment breaks 2-bar / 32-step
    patterns). Phase is defined identically for synced and free-run
    (phase = fmod(ppq*4, patternLength)), so the free-run <-> host-synced handoff
    is seamless (no audible catch-up).

    Stateless enumeration per block (derived purely from ppqStart + a half-open
    window) => a step boundary on a block edge fires exactly once, in the block
    where it is the start — no double-fire, no miss (Probe 4). Free-run integrates
    an internal phase at `tempo` BPM when the host is absent/stopped.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace OSimpleBeatmaker
{
    struct FiringColumn
    {
        int         stepIndex;
        int         nominalOffsetInBlock;
        juce::int32 nominalSampleInBar;
    };

    class SequencerClock
    {
    public:
        void prepareToPlay (double fsIn) noexcept
        {
            fs = fsIn;
            freeRunStepPos = 0.0;
            lastBlockEndPpq = 0.0;
            haveLast = false;
        }

        // Enumerate firing columns for this block. Returns the count written to
        // `out` (<= maxOut). `playheadPhaseOut` = fractional step index at block
        // start; `discontinuityOut` = host ppq jumped (loop/relocate).
        int process (FiringColumn* out, int maxOut,
                     bool synced, bool playing, double bpm, double ppqStart,
                     int patternLength, int numSamples,
                     float& playheadPhaseOut, bool& discontinuityOut) noexcept
        {
            int count = 0;
            discontinuityOut = false;
            playheadPhaseOut = 0.0f;

            if (bpm <= 1.0) bpm = 120.0;
            const double samplesPerPpq  = (60.0 / bpm) * fs;
            const double samplesPer16th = samplesPerPpq * 0.25;
            const double barLenSteps    = (double) patternLength;
            if (samplesPer16th <= 0.0 || patternLength <= 0) return 0;

            if (synced && playing)
            {
                const double blockPpq  = (double) numSamples / samplesPerPpq;
                const double barLenPpq = patternLength * 0.25;
                const double barStart  = ppqStart - std::fmod (ppqStart, barLenPpq);

                if (haveLast && std::abs (ppqStart - lastBlockEndPpq) > 0.125)
                    discontinuityOut = true;

                const double lo = ppqStart;
                const double hi = ppqStart + blockPpq;

                for (int bar = -1; bar <= 1 && count < maxOut; ++bar)
                {
                    const double barRepStartPpq = barStart + bar * barLenPpq;
                    for (int k = 0; k < patternLength && count < maxOut; ++k)
                    {
                        const double stepPpq = barRepStartPpq + k * 0.25;
                        if (stepPpq >= lo - 1.0e-9 && stepPpq < hi - 1.0e-9)
                        {
                            // Anchor the in-block offset to the bar-relative grid
                            // position (nominalSampleInBar) so the emitted MIDI
                            // offset and the viz value agree EXACTLY at half-sample
                            // grid boundaries (a naive (stepPpq-ppqStart)*spp loses
                            // the .5 to subtraction error and rounds the other way).
                            const juce::int32 nsib = (juce::int32) std::llround (k * samplesPer16th);
                            int off = (int) std::llround ((barRepStartPpq - ppqStart) * samplesPerPpq)
                                      + (int) nsib;
                            off = juce::jlimit (0, numSamples - 1, off);
                            out[count++] = { k, off, nsib };
                        }
                    }
                }

                playheadPhaseOut = wrapPhase (ppqStart * 4.0, barLenSteps);
                freeRunStepPos   = wrapPhase (hi * 4.0, barLenSteps);   // keep ready for free-run handoff
                lastBlockEndPpq  = hi;
                haveLast         = true;
            }
            else
            {
                // Free-run: integrate an internal step phase at `tempo` BPM.
                const double blockSteps = (double) numSamples / samplesPer16th;
                const double startPos   = freeRunStepPos;
                const double endPos     = startPos + blockSteps;

                int m = (int) std::ceil (startPos - 1.0e-9);
                for (; (double) m < endPos - 1.0e-9 && count < maxOut; ++m)
                {
                    const int         k    = ((m % patternLength) + patternLength) % patternLength;
                    const juce::int32 nsib = (juce::int32) std::llround (k * samplesPer16th);
                    // Anchor to the bar-relative grid position (see synced path).
                    const double repStartStep = (double) m - (double) k;
                    int off = (int) std::llround ((repStartStep - startPos) * samplesPer16th)
                              + (int) nsib;
                    off = juce::jlimit (0, numSamples - 1, off);
                    out[count++] = { k, off, nsib };
                }

                freeRunStepPos = endPos;
                while (freeRunStepPos >= barLenSteps) freeRunStepPos -= barLenSteps;
                playheadPhaseOut = (float) startPos;
                haveLast = false;
            }

            return count;
        }

    private:
        static float wrapPhase (double v, double period) noexcept
        {
            double p = std::fmod (v, period);
            if (p < 0.0) p += period;
            return (float) p;
        }

        double fs = 44100.0;
        double freeRunStepPos = 0.0;
        double lastBlockEndPpq = 0.0;
        bool   haveLast = false;
    };
}
