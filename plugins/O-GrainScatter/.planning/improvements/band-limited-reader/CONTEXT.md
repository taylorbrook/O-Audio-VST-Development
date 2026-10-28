# Context: Band-limited grain reader (mipmap + polyphase sinc)

**Plugin:** O-GrainScatter (first consumer) + `modules/synthesis/grain-engine` (new class)
**Milestone:** band-limited-reader
**Created:** 2026-10-27
**Phase:** Discuss
**Base version:** O-GrainScatter 3.2.0, grain-engine 1.0.0

---

## Original Request

> band-limited reader in grain-engine: mipmapped half-band octave levels on the ring write path + 4–8 polyphase sinc sets for the residual ratio, group-delay compensated, unity DC per phase; GrainScatter (+36 st) first (research §3.3)

Source: `research/granular-synthesis-2026-update.md` §3.3, roadmap item #5, gap §2.2 item 2 ("No engine band-limits pitched-up grains"; GrainScatter: none, up to +36 st, Doppler 2×).

---

## Current state (facts from the code)

- Live ring: `Source/dsp/DelayBuffer.h`, 12 s stereo, double read position, Lagrange-4 (`readSample`). No filtering at any rate.
- Freeze: `Source/dsp/FreezeManager.h`, 2 s snapshot slots copied from the ring via `copyRegion`; `readSample(slot, ch, pos)`.
- Grain rate: `PitchEngine` clamps total pitch to ±36 st → base rate 0.125–8 (`PitchEngine.h:327`).
- Doppler: `v.dopplerFactor` ∈ [0.5, 2], recomputed per block from angular velocity (`PluginProcessor.cpp:852`) and multiplied into the per-sample advance (`GrainPool.h:405`). So effective rate moves *inside* a grain and peaks at 16×.
- Read sites: `GrainPool.h` ~303–316 and ~373–382 (two render paths: stereo and spatial/mono).

---

## Decisions (user, 2026-10-27)

| Question | Decision |
|---|---|
| Rate moving mid-grain (Doppler) | **Latch octave level ℓ at onset from the base rate** (ℓ = ⌊log₂ r_base⌋, clamped ≥ 0). Pick the polyphase kernel set **per sample from the live residual** r′ = r_base·doppler / 2^ℓ. Kernel sets span r′ ∈ [0.5, 4), so Doppler never forces a level switch inside a grain. |
| Rate ≤ 1 | **Sinc everywhere.** Level 0, cutoff 1.0 for r′ ≤ 1. No interpolator switch when Doppler crosses unity. Unity pitch changes vs v3.2.0 (Lagrange-4 → sinc), so the gates measure spectra and alias rejection, not a null against 3.2.0. |
| Freeze snapshot | **Band-limit it too.** Capture copies the mip levels with level 0 (≈2× snapshot memory, preallocated in `prepare`). |
| Tap budget | **16 taps for every grain.** One quality, deterministic. No tiering; CPU measured in verify, tiering only revisited if the measurement demands it. |

---

## Requirements

1. **`BandlimitedReader` (or `MipRing` + `PolyphaseSinc`) in grain-engine**
   - Mip levels ℓ = 1..L built on the write path by a half-band ÷2 cascade (L = 3 covers base rate 8; level 0 = the raw ring). Header-only, alloc-free after `prepare`, double read positions.
   - Polyphase windowed-sinc table: 16 taps, cutoff sets for r′ ∈ [1, 4) (4–8 log-spaced sets; r′ ≤ 1 uses cutoff 1.0), fractional phases interpolated or finely tabulated.
   - **Every phase of every set normalised to unity DC** (JUCE WindowedSinc is 0.990 — `critical_juce_windowedsinc_dc_gain_is_0990`).
   - **Half-band group delay compensated:** p_ℓ = p₀ / 2^ℓ − D_ℓ (accumulated through the cascade), so a grain read at level ℓ is time-aligned with level 0 — no time jump between grains latched at different levels.
   - Acceptance: module unit tests — DC gain of every phase = 1 ± 1e-6; level-ℓ impulse/transient lands at the same output time as level 0 (≤ 0.05 sample); stopband of the half-band and of each sinc set meets the spec set in research; write path and read are alloc-free.

2. **GrainScatter live ring migrates to the reader**
   - `DelayBuffer` (or its replacement) feeds the cascade on `pushSample`; both grain render paths read through the band-limited reader with ℓ latched at spawn and r′ from `playbackRate * dopplerFactor`.
   - Read guard respects the cascade: a level-ℓ read near the write head needs D_ℓ·2^ℓ samples of settled history; the existing read-head floor is extended by that (a few ms), no plugin latency added.
   - Acceptance: aliasing of a swept/high sine at +12/+24/+36 st (and +36 st with Doppler 2×) drops vs v3.2.0 by a stated margin (target ≥ 60 dB alias rejection in the audible band); passband flat to a stated bound at unity; no time jump at an octave boundary (grain at r = 1.99 vs 2.01 aligned).

3. **Freeze snapshot carries the levels**
   - Capture copies every mip level for the frozen region; frozen grains read through the same reader with snapshot coordinates (wrap/crossfade behaviour from v3.0.4 kept).
   - Acceptance: frozen +36 st grain shows the same alias rejection as live.

4. **No regressions in existing gates**
   - `tests/blocksize-null`: still bit-exact across block sizes (cascade state must be block-size invariant).
   - `tests/density-gain`: still passes (sinc unity DC keeps levels; rows re-baselined only if the measured change is the interpolator's passband, documented).
   - pluginval strictness 10 VST3, AU registers, no audio-thread allocation.

5. **Module release**
   - grain-engine 1.0.0 → 1.1.0 (new optional part, existing classes untouched), `module.yaml` `used_by` gains O-GrainScatter, module tests extended.

---

## Scope Boundaries

**In scope:** the reader class in grain-engine; GrainScatter live ring + freeze snapshot; module tests; GrainScatter gates (alias, alignment, CPU) with a v3.2.0 negative control; CHANGELOGs.

**Out of scope:** migrating O-simpleGrain / TextureForge / Polystutter / Freeze to the reader (later releases, one per `/improve`); tiered tap counts; mid-grain level switching; any parameter, UI or preset change; moving GrainScatter's other DSP onto the module.

---

## User Preferences / constraints

- Sound quality first, but CPU must be measured (16 taps × all grains × both channels); report it in verify.
- No parameter/state/preset change → presets load unchanged. Timbre change at unity is accepted and must be documented.
- Real-time safety: all tables built in `prepare`/static init, no allocation on the audio thread.
- Expected bump: O-GrainScatter **MINOR → 3.3.0** (audible change, no state change) — confirm in plan.

## Open questions for research

- Half-band design: linear-phase FIR (constant integer/half-integer delay, easy compensation) vs polyphase IIR (cheaper, non-constant delay breaks requirement 1). Expect FIR; choose length vs stopband.
- Number of sinc sets (4 vs 8) over r′ ∈ [1, 4) and the phase resolution (table size vs linear phase interpolation), given 16 taps.
- Exact read-guard arithmetic at the read-head floor with ℓ ≤ 3, and whether the spatial path's grain count makes CPU a problem (measure the max grain cap).
- Whether the freeze snapshot is cheaper to rebuild from level 0 at capture (off the audio thread?) or to copy from the live levels.
