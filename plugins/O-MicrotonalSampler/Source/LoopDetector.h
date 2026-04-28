/*
  ==============================================================================

    LoopDetector.h
    Microtonal Sample Engine - Loop region auto-detection (Phase 2.5, RQ-2 / D2-4)
    Ouaricon Audio
    Developer: Taylor Brook

    Pure-function RMS scan + zero-crossing snap + variance/length defensive
    guards. Runs ONCE per slot on the loader thread (PERF-01: no allocation
    or analysis in the audio path). Output populates `SampleSlot::loopStart`
    and `SampleSlot::loopEnd`; an invalid result reverts the slot to one-shot
    (loopEnd == 0 in the voice).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace LoopDetector
{
    struct LoopRegion
    {
        int  loopStart = 0;
        int  loopEnd   = 0;
        bool valid     = false;
    };

    // Scan `buf` (mono mixdown of channel 0 + channel 1 if stereo) for a
    // steady low-RMS region in the latter 60% of the sample, then snap the
    // boundaries to zero crossings. Returns `valid = false` (one-shot fallback)
    // when:
    //   - The buffer is too short to host the analysis window (EC-7).
    //   - The variance gate fails (no region significantly quieter than the mean).
    //   - No same-direction zero-crossing is found within the search window for
    //     the loopEnd target length, even after re-scanning at a shrunken length.
    //   - The resulting region is < 16 samples (8-sample crossfade would consume
    //     half the loop — defensive guard from RESEARCH Phase-2.5 Gate 5 item 4).
    LoopRegion detectLoop (const juce::AudioBuffer<float>& buf, double sampleRate);
}
