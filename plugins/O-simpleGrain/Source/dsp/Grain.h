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
    float  pan           = 0.5f;   // 0 = L .. 1 = R (equal-power)
    int    shape         = 4;      // window LUT index (0=rect .. 4=Hann)
    int    age           = 0;      // ++ per sample — for steal-oldest

    // Anti-aliasing one-pole state (per grain). The AA filter is a no-op
    // pass-through in Phase 2.1 (Task 3); Phase 2.2 fills its body. aaState is
    // reset to 0 on spawn.
    float  aaState       = 0.0f;
};
