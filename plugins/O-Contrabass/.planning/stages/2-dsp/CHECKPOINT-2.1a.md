# Phase 2.1a Checkpoint — Gate FAILED on render-harness (RMS check)

**Date:** 2026-04-26
**Status:** PAUSED — awaiting re-research before any DSP change.
**Owner action:** user-selected "option 4: pause and re-research" after harness FAIL.

---

## What's Green

- Source files (Tasks 1-7) authored per PLAN.md (10 created, 3 modified — full list in PLAN.md "Files To Create / Modify").
- Build: `O-Contrabass_VST3`, `O-Contrabass_AU`, `O-Contrabass-render-test` all link clean. Only pre-existing note-expression module warnings (unchanged from Stage 1).
- `auval -v aumu OCbs OuDv` → AU VALIDATION SUCCEEDED.
- `pluginval --strictness-level 10 --validate-in-process` → SUCCESS (full state restore + parameter thread safety + fuzz pass).
- HANDOFF.json (Stage 1→2) created and validates against schema; `run-gate.sh O-Contrabass 1-foundation 2-dsp --skip-review` → GATE PASSED (exit 0).
- gate-report at `plugins/O-Contrabass/.planning/stages/1-foundation/gate-report.json`.

## What's Red

**Render-harness `O-Contrabass-render-test --note 28 --sustain 60 --infinite-sustain 1.0` exits 1.**

JSON summary at `/tmp/e1-max-sustain.json`:
```
peak=0.011192 (-39 dBFS)
rmsMid_s5_s6 = 0.0
rmsFinal_lastSecond = 0.0
nanCount = 0
infCount = 0
blockTime_max_over_median = 4.47   ← under 5× threshold
pass_nan       = true
pass_peak      = true
pass_blockTime = true
pass_rms       = false   ← only failure
```

## Failure Signature

This is **NOT** the failure mode PLAN.md §"Risks #1" anticipated. PLAN expected runaway/NaN at max INFINITE_SUSTAIN; documented fallback was clamping the loop-gain ceiling. Observed failure is the **opposite** — string excited but never reaches steady-state Helmholtz oscillation. Lowering the ceiling makes it worse, not better.

## Root-Cause Hypothesis (NOT yet validated by research)

Single-rail topology (PLAN.md Task 3 simplification) under-injects bow energy compared to O-Bowed's split-rail.

In O-Bowed (`plugins/O-Bowed/Source/DSP/WaveguideString.cpp:131-142`):
```cpp
float toBridge = nutReflection + newVelocity;   // both rails get
float toNeck   = bridgeReflection + newVelocity; // the same injection
```
Each round-trip carries `newVelocity` twice (once per rail).

In O-Contrabass single-rail (`plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:144`):
```cpp
float x = incoming + newVelocity;   // single rail; one injection per round-trip
```
Only one `newVelocity` per round-trip.

At BOW_POSITION=0.10 (default, sul-ponticello-leaning bass operating point), Schelleng's minimum-bow-force scales as ~1/β; with half the effective injection, F_bow=1.0 N at β=0.10 may sit below the playable region.

**Hypothesis is unverified.** Could also be:
- Sign-convention error (boundary reflection placement vs LP-filter placement vs DC-blocker placement).
- DC blocker in-loop killing the steady-state injection (architecture-mandated placement; never validated against single-rail).
- BowModel attack envelope timing wrong at 2× sample rate.
- Saturator `x/sqrt(1+x²)` placement in the loop bounding the recursive growth.
- Bass-tuned friction defaults (`mu_s=0.85`, `mu_d=0.25`, `v_0=0.05`) producing weaker injection than treble defaults at low velocities.

## What Needs Research

Before any code change, re-research these (in order of likely impact):

1. **Single-rail vs split-rail energy budget.** Is single-rail with 2× injection compensation literally equivalent to split-rail at the steady state, or only at certain frequencies? Cite a paper or textbook (Smith's *Physical Audio Signal Processing*, ch. 6 candidate).
2. **DC blocker in the loop.** O-Bowed has NO in-loop DC blocker. ARCHITECTURE.md mandates one (post-saturator). Does this kill steady-state oscillation, or is it transparent at f0=41.2 Hz?
3. **Saturator placement.** O-Bowed uses `4·tanh(x/4)` on the WRITE path (outside the round-trip recursion). ARCHITECTURE.md mandates `x/sqrt(1+x²)` IN the loop. Are these dynamically equivalent at low signal levels?
4. **Bass-tuned friction floor.** With `mu_d=0.25` (lower than O-Bowed's 0.30), is steady-state injection above the Schelleng minimum-bow-force at β=0.10?
5. **Single-rail bow-position physics.** In single-rail topology, what does β=0.10 actually mean? The bow contact point is implicit (collapsed into the bridge end). Does β only modulate friction-junction impedance, or should it also affect the delay length?

Possible research outputs:
- A definitive answer on whether single-rail can sustain Helmholtz at all (or whether PLAN.md's "single-rail" simplification was always going to fail and split-rail is required).
- Specific parameter / topology change with citations.
- Or: confirmation that single-rail + 2× injection is the answer, with derivation.

## Options Already Considered (and rejected for now)

| # | Option | Rejected because |
|---|--------|---|
| 1 | Multiply newVelocity × 2 | Not yet validated by research; could mask a deeper bug. |
| 2 | Switch to split-rail | Big PLAN.md deviation; needs research-locked decision before re-architecting. |
| 3 | Tune friction defaults more aggressively | Empirical hack; would mask root cause. |
| 4 | **Re-research before changing code** ← user-selected | (this) |

## Files Currently In an Intermediate State

The Phase 2.1a source files (Tasks 1-7) are committed to the working tree but **NOT** committed to git yet. They build clean and pass auval/pluginval, but the harness fails. Do not delete — they are the substrate the research will guide us to fix.

| File | LOC | Status |
|---|---|---|
| `Source/DSP/HyperbolicFriction.h` | 58 | Verbatim O-Bowed port + 3 init-list edits |
| `Source/DSP/BowModel.h` | 53 | Verbatim O-Bowed port + 2 init-list edits |
| `Source/DSP/BowModel.cpp` | 99 | Verbatim port |
| `Source/DSP/WaveguideString.h` | 95 | NEW (bass-adapted single-rail) |
| `Source/DSP/WaveguideString.cpp` | 198 | NEW — **suspect file** |
| `Source/BowedContrabassVoice.h` | 89 | NEW |
| `Source/BowedContrabassVoice.cpp` | 175 | NEW (also reviewable for sign-convention) |
| `Source/OContrabassMPESynthesiser.h` | 53 | NEW |
| `tests/render-harness/CMakeLists.txt` | 79 | NEW (extended with JucePlugin_* defines) |
| `tests/render-harness/main.cpp` | 247 | NEW |
| `Source/PluginProcessor.{h,cpp}` | — | Modified (synth wired in) |
| `CMakeLists.txt` | — | Modified (DSP sources + render-harness option) |

## Resume Recipe (for next session)

```
/clear
/plugin-research O-Contrabass 2-dsp     ← investigate root cause per above hypotheses
# OR
/research                                ← if research-skill / deep-research is preferred
```

Once research lands a decision, return to `/plugin-execute O-Contrabass 2-dsp` and continue from Task 8 gate (the source files are already in place; only `WaveguideString.cpp` and possibly `BowedContrabassVoice.cpp` need editing).

After the harness PASSes:
- Task 8 sign-off (this gate)
- Phase 2.1b (Tasks 9-16: module extraction)
- Phase 2.1c (Tasks 17-19: dispersion)
- Phase 2.1 final verification (Task 20)
- `/plugin-verify O-Contrabass 2-dsp` to close Phase 2.1.

## Validated Artifacts To Keep

- HANDOFF.json (Stage 1→2) — works; do not regenerate
- gate-report.json — proof Stage 1→2 transition is clean
- /tmp/e1-max-sustain.{wav,json} — diagnostic output; reproduce by re-running the harness
