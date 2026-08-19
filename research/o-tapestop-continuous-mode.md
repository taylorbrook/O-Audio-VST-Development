# O-Tapestop Continuous Mode — Deep Research Synthesis

**Date:** 2026-08-16 · **Target:** O-Tapestop v1.1.0 (MINOR) · **Research:** Level 3 (3 parallel agents: market survey, DSP design, codebase-fit)

## Feature

MODE gains a third choice **Continuous**: while ENGAGE is latched, tape speed `r` moves continuously
(no single gesture). CHARACTER choice param — **Wobble / Random / Glitch** — plus RATE (Sync/Free),
DEPTH, CHAOS knobs. ENGAGE off → resync to dry via existing machinery.

## Architecture decision: B — modulation generator (`State::ContinuousMotion`)

Three candidates evaluated against `TapestopTransport`:

| | Cost | Sonic ceiling | Verdict |
|---|---|---|---|
| A: Looping scratch-envelope LUT | S | Low-Med | **Reject.** Per-pass LUT regeneration breaks the atomic message-thread handoff (audio thread can't signal wraps fast enough — "Random" degenerates to a repeating loop). Events phase-locked to loop length. LUT mean ≠ 1 → unbounded debt drift; fixing it needs B's debt servo anyway. |
| **B: Per-sample generator state** | **M** | **High** | **Recommended.** New `ContinuousMotion` state slots into the engage/release/resync topology exactly like ScratchPass did. Bypassed bitwise-dry, trim blend, resync contracts untouched. |
| C: Auto-retrigger existing gestures | S | Low | **Reject.** Vocabulary is "dip toward full stop + splice back" with fixed 1.25× catchup garnish; cannot express wobble/flutter depth, no reverse, no r > 1. (Could return later as a 4th "Stutter" character inside B.) |

## Character algorithms (from ChowTapeModel dissection + literature)

Composite generator, run at ~1 kHz control rate with linear interp to audio rate (content < 100 Hz):

```
m(t) = D_wow · sin(φ_w)                                        // φ̇_w = 2πf_w(1+jitter), jitter re-drawn per cycle, pow(rand,1.25) skew
     + D_flut · [0.56·sin(φ_f) + 0.20·sin(2φ_f) + 0.24·sin(3φ_f)]   // ChowTape motor stack 230:80:99 normalized
     + D_drift · x_OU(t)                                        // Ornstein-Uhlenbeck: x' = α·x + σ√(1−α²)·gauss(), α = exp(−1/(τ·fs))
r(t) = 1 + m(t) + u_servo(t)
```

- **Wobble** — deterministic sine stack; chaos morphs order→disorder (per-cycle freq jitter 0.5·chaos,
  amp jitter 0.3·chaos, OU drift mix, filtered-noise flutter band above chaos 0.6). Defaults: f_wow 1.2 Hz,
  m_peak 0.4%, flutter 12 Hz. Zero-mean → debt-safe (worst case d_peak = m/(2πf) ≈ 0.19 s).
- **Random** — OU process; RATE → correlation time τ = 1/(2π·RATE); chaos stacks 3 octaves (τ, τ/4, τ/16
  at 1, 0.5·chaos, 0.25·chaos²) and widens excursion clip 1σ(tanh)→3σ. Post-LP 20 Hz for C1 smoothness.
- **Glitch** — tempo-grid event scheduler, one Bernoulli draw per cell, p = chaos², monophonic (no overlap;
  min inter-jump interval ≥ xfLen preserves the 2-voice contract). Event menu: tapestop-dip (reuse
  (1−u)^p curves), reverse-flick, speed-jump {2.0, 1.5, 0.5}, half-speed drag, stutter-repeat
  (position jumps splice at clamp(slice/4, 3 ms, 50 ms)), resync-snap. Extreme events gated above chaos 0.5.
  Optional seeded per-cell hash for loop-repeatable glitches (Glitch 2 precedent).

**Continuity rule (verified):** rate steps with continuous position are click-free (C1 corner, −12 dB/oct);
position jumps are not and must use the 2-voice splice. Invariant: **r may step; P may not.** 2 ms slew
on glitch r steps as polish. Codebase already ships this ("palindrome corner" in ScratchPass).

## Debt management (26 s ring)

- **Wobble:** zero-mean, safe. First upswing from Bypassed clamps at live head (d≈0) — accept or bias first cycle down.
- **Random:** rate mean-reverts but position debt random-walks (σ_d ≈ 0.85 s/hour at 1%, worse at 3σ).
  **Weak debt servo required:** `r += k_s·(d_target − d)`, k_s ≈ 0.2 /s, contribution clamped ≤ 0.2%
  (≈3.5 cents, inaudible); gives σ_e ≈ 22 ms. Servo highpasses rate noise below 0.03 Hz only.
- **Glitch:** events accumulate one-sided debt (reverse-flick +2T; stutter +(n−1)L; speed-jump −T).
  Three layers: (1) debt-biased event selection `w·exp(−λ·d·sign)`, λ ≈ 0.5/s; (2) soft budget ~3 s
  suppresses debt-positive events; (3) hard resync ~6 s fires `enterResync()`-style 50 ms crossfade-skip
  at the next grid boundary — reads as a deliberate "catch" splice, safety doubles as an event type.
- Mode entry applies the existing `max(readAbsFrac, totalWritten − maxDebt)` clamp (SpinUp-entry template).

## Parameters (all appended; existing IDs untouched)

| ID | Type | Notes |
|---|---|---|
| MODE | existing Choice + "Continuous" | see compatibility below |
| CHARACTER | Choice {Wobble, Random, Glitch} | |
| CONT_RATE_SYNC_DIV / CONT_RATE_HZ | Choice (reuse syncDivisionChoices) / Float 0.05–20 Hz log | Sync/Free twin pattern; Wobble = LFO f, Random = 1/(2πτ), Glitch = grid cell |
| CONT_DEPTH | percentRange | `m_peak = 0.001·10^(2.08·depth)` → 0.1%…12% (±2 ¢ … ±2 st). Glitch: event intensity |
| CONT_CHAOS | percentRange | per-character morph (jitter/octave-stack/probability) |

Depth/rate updated live on the absolute 16-sample grid (toneTrack pattern); character + seeds latched at engage edge.

## Release path

**SpinUp-path** (recommended over resync-direct): `ContinuousMotion` case in `release()` seeds
`r0 = clamp(currentR, 0, 1)` → existing `u = pow(r0, 1/p)` → SpinUp → Catchup → ResyncXfade.
Reuses all resync machinery incl. debt clamp; gives START_CURVE/START time meaning in this mode.

## Compatibility (the real cross-cutting risk)

JUCE choice normalization: 2 choices → Scratch stores n=1.0; 3 choices → n=1.0 decodes as index 2 = Continuous.

1. **DAW sessions: SAFE** — APVTS stores the unnormalised index.
2. **Preset manager: BREAKS** — `OuariconPresetManager.h:281/340` save/load normalized. Needs a NEW
   version-gated migration (per-param gate pattern): presetVersion < 1.1.0 → remap MODE n 1.0→0.5.
   Harness probe: canned v1.0 preset with n=1.0 must decode to Scratch. Factory presets regenerate via WR-04.
3. **VST3 MODE automation lanes: silently repoint** (stored normalized; stepCount 2→3). MODE is a setup
   control, not the performance param — release-note the caveat rather than introducing MODE2.
4. **Code:** `PluginProcessor.cpp:485` `pMode > 0.5f` boolean decode must become index-based 3-way; UI MODE
   segment control becomes 3-way.
5. Every existing factory preset must gain the 5 new param IDs (WR-01 reset defense).

## Integration points

- Enum + tick case + servo: `TapestopTransport.h` (enum ~97, tick switch ~320, clamp ~404, refactor
  `enterResync` → `spliceCarrierTo(newPos, rLatch)` helper for glitch jumps, ~491).
- `engageContinuous(...)` modeled on `engageScratch()` (~229): takeover from every state, running crossfade untouched.
- Params: `createParameterLayout()` after ENV block (`PluginProcessor.cpp:311`); atomics in `PluginProcessor.h:186`.
- Engage-edge branch beside `PluginProcessor.cpp:488-499`; RNG: one seeded stream per purpose
  (noise / event-timing / event-target), seeded at engage edge — P0 determinism + block-size invariance by construction.
- TONE_TRACK works unchanged (16-sample grid follows |r|). MIX < 100% in Continuous = tape flanging vs dry —
  in-character, document, don't fix.

## Harness extensions (tests/render-harness)

P0/P1a/P1b per character (enforces RNG discipline) · P2 bitwise-null after Continuous release ·
P4c debt-bound probe (Random depth-max held ≫ ring; window must span several wobble periods) ·
P6 discontinuity scan incl. min-inter-jump edge · zipper probes with liveness gates on all new knobs ·
preset-migration probe.

## Market context (naming/ranges validated against field)

Wow 0.3–4 Hz / flutter 6–20 Hz split is universal (RC-20, ChowTape, SketchCassette, Satin);
subtle = 0.1–0.5% deviation (Satin DIN spec), broken/melted = multi-semitone; nonlinear depth laws
(ChowTape depth³) keep the knob's lower half believable. Preset vocabulary in the wild: wobble, drift,
drunk, warped, melted, seasick, dying battery, sticky capstan, damaged tape, VCR. Seeded repeatable
randomness (Glitch 2, Goodhertz) is valued. Glitch 2's Tape Stop "Play Mode: Stop/Start cycling" is the
closest existing "continuous tapestop" and confirms the event-scheduler framing.

## Proposed factory presets (Continuous mode)

- **Subtle Wobble** — Wobble, 1.2 Hz free, depth ~0.35 (≈0.4%), chaos 0.15
- **Warped Record** — Wobble, synced 1 bar, depth 0.6, chaos 0.35
- **Drunk Tape** — Random, rate 0.5 Hz, depth 0.55, chaos 0.5
- **Seasick** — Random, rate 0.15 Hz, depth 0.75, chaos 0.7
- **Glitch** — Glitch, 1/8 grid, depth 0.6, chaos 0.55
- **Total Meltdown** — Glitch, 1/16 grid, depth 0.9, chaos 0.95
