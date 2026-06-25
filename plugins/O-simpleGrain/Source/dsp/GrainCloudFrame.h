/*
  ==============================================================================

    O-simpleGrain - Grain Cloud Frame (viz tap payload)

    The fixed-size POD each grain-event tap carries from the audio thread to the
    Stage-3 WebView visuals: the grain-cloud scatter (UI-01) + source-waveform
    playheads (UI-02). One GrainCloudFrame is filled per block in processBlock
    (each voice appends a GrainEvent at spawn, bounded by kMaxEvents — extras
    dropped, never grown), then handed off via TripleBuffer<GrainCloudFrame>
    (lock-free SPSC, copied verbatim from O-GrainScatter).

    No heap, no dynamic growth — a fixed std::array of events. The audio thread
    is copy-only; the message-thread editor Timer read()s the published frame.

    Fields per RESEARCH §7.2 / ARCHITECTURE §Thread Boundaries
    ("Grain events (pos,size,pitch,pan,time)").

  ==============================================================================
*/

#pragma once
#include <array>

//==============================================================================
// One spawned grain's display state. POD — trivially copyable, no heap.
struct GrainEvent
{
    float readPosNorm = 0.0f;   // grain.readPos / sourceLen ∈ [0,1] — cloud X / waveform playhead
    float sizeMs      = 0.0f;   // grain length in ms                — cloud dot size
    float pitchSemis  = 0.0f;   // (grainPitch + spray) st, relative  — cloud Y / colour
    float pan         = 0.5f;   // 0..1                               — cloud lateral
    int   spawnSample = 0;      // sample offset within the block      — cloud time axis
};

//==============================================================================
// One block's worth of grain events + global read-head state for the visuals.
// Fixed-size POD (kMaxEvents grains across all 8 voices in one block; extras
// are dropped, never grown — RT-safe).
struct GrainCloudFrame
{
    static constexpr int kMaxEvents = 256;          // >= one block's spawns across 8 voices

    std::array<GrainEvent, (size_t) kMaxEvents> events {};
    int   count             = 0;                    // valid events this frame (<= kMaxEvents)

    float playheadNorm      = 0.0f;                 // global playhead / sourceLen — the live playhead line
    float positionNorm      = 0.0f;                 // resting point — for the shaded spray range
    float positionSprayNorm = 0.0f;                 // ± shade half-width (fraction of source length)
    bool  frozen            = false;                // freeze-point marker
};
