/*
  ==============================================================================

    O-simpleGrain - Grain POD

    A single grain's state. A preallocated std::array<Grain, kMaxGrainsPerVoice>
    lives in every GrainVoice; grains are spawned (find-inactive / steal-oldest)
    and advanced per sample by the overlap-add render loop. Never heap-allocated
    on the audio thread.

    Forward-phase model (ARCHITECTURE §Core Components), NOT GrainScatter's
    samplesRemaining countdown: each sample we read the static source at readPos,
    then readPos += rate; phase += phaseInc; ++age. A grain is done when phase>=1.

    Fields per RESEARCH §2.1.

  ==============================================================================
*/

#pragma once

struct Grain
{
    bool   active        = false;
    float  readPos       = 0.0f;   // absolute fractional position in the SOURCE buffer (samples)
    float  rate          = 1.0f;   // read increment = voiceRate * 2^((grainPitch + spray)/12)
    float  phase         = 0.0f;   // window phase 0..1
    float  phaseInc      = 0.0f;   // = 1.0f / lengthSamples
    float  lengthSamples = 0.0f;   // grainSize ms * fs
    float  pan           = 0.5f;   // 0 = L .. 1 = R (equal-power) — kept for the viz tap
    int    shape         = 4;      // window LUT index (0=rect .. 4=Hann)
    int    age           = 0;      // ++ per sample — for steal-oldest

    // Precomputed equal-power pan gains (pan is fixed for a grain's life, so the
    // cos/sin are evaluated ONCE on spawn — never per sample in the render loop).
    float  panL          = 0.70710677f;   // cos(pan*π/2)
    float  panR          = 0.70710677f;   // sin(pan*π/2)

    // Anti-aliasing one-pole (per grain). The cutoff depends only on `rate`, which
    // is constant for the grain, so the smoothing coefficient is precomputed ONCE
    // on spawn (no per-sample std::exp in the render loop). `aaEngaged` is true only
    // for up-transposed grains (rate > 1); otherwise the read is passed through.
    // `aaState` is primed to the first read sample on spawn.
    float  aaCoeff       = 0.0f;
    bool   aaEngaged     = false;
    float  aaState       = 0.0f;
};
