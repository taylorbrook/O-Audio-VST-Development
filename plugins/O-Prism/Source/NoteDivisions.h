#pragma once

// Note division multipliers: beats per cycle, shared by the tempo-synced
// global LFOs (PluginProcessor::advanceGlobalLfoPhases), the tempo-synced
// delay (WR-03), and the per-voice LFOs (PrismVoice::renderNextBlock).
// Index order matches getLfoDivisionNames():
//   1/1, 1/2, 1/4, 1/8, 1/16, 1/32,
//   1/1D, 1/2D, 1/4D, 1/8D, 1/16D, 1/32D,
//   1/1T, 1/2T, 1/4T, 1/8T, 1/16T, 1/32T
namespace NoteDiv
{
    inline constexpr float kDivBeats[18] = {
        4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f,                   // straight
        6.0f, 3.0f, 1.5f, 0.75f, 0.375f, 0.1875f,                // dotted (1.5x)
        2.6667f, 1.3333f, 0.6667f, 0.3333f, 0.1667f, 0.0833f     // triplet (2/3x)
    };
}
