# O-Contrabass — Stage 4 Subjective Audition (FUNC-03 · DSP-10 · FUNC-04 QA)

**Date:** 2026-07-15
**Host (locked):** Logic Pro (AU) — Dorico is covered separately by `Resources/dorico/SMOKE-TEST.md`.
**Build under test:** `O-Contrabass-dev` v1.0.0 (installed VST3 + AU). DSP is FROZEN —
19/19 render goldens byte-identical, so there is no "before" to A/B against; the A/B is
**preset-switch within one instance** + **compare vs your own reference libraries**.
**Method:** R38 probe-table format (`stages/2-dsp/RESEARCH.md:7139-7197`).

This audition is the human gate for three requirements. It doubles as **FUNC-04 preset QA**
(audition all 10, accept or request a tweak). Record the outcome in the `PASS?` / `Notes`
columns, then transcribe the CONFIRM/REVISE verdict into `STATUS.md` carry-forward + the
REQUIREMENTS verify comments on FUNC-03 / DSP-10 / FUNC-04.

---

## Setup

1. **Re-seed presets fresh** (they only re-seed when the version string changes, which it
   won't within v1.0.0):
   ```bash
   rm -rf ~/Library/O-Contrabass/Presets/Factory/
   ```
   Then load an instance (auval, Standalone, or Logic) once to re-seed, OR just open the
   plugin in Logic — first instantiation seeds all 10.
2. Open O-Contrabass on a software-instrument track in Logic. Confirm the preset bar lists
   **10 presets** (flat alphabetical: Cinematic Bass Sustain first).
3. Have reference material ready: **Spitfire Albion / CSS / VSL** (orchestral arco),
   **Stephen O'Malley / Tony Conrad** (drone).

---

## Core probes (FUNC-03 + DSP-10)

| # | Probe | Preset | Material | Reference | PASS criterion | PASS? | Notes |
|---|-------|--------|----------|-----------|----------------|-------|-------|
| P1 | Orchestral arco (FUNC-03a) | Cinematic Bass Sustain | Sustained E1 + A1 arco, ~8 s | Spitfire / CSS / VSL | "same sonic family" as a real orchestral bass sustain | ☐ | |
| P2 | Slow attack (DSP-10) | Cinematic Bass Sustain | Held legato note; listen to onset | — | **no note-on click**; natural swell (~1168 ms onset character) | ☐ | |
| P3 | Drone (FUNC-03b) | Infinite Drone | Hold a low chord; let it evolve 20 s+ | O'Malley / Conrad | "evolving drone in the spirit of" — sustains + moves | ☐ | |
| P4 | A/B switch (FUNC-03c) | Cinematic ↔ Infinite Drone | Switch preset mid-session, no host retune | — | **both credible from one engine/instance** | ☐ | |

**FUNC-03 verdict** (needs P1 + P3 + P4): ☐ CONFIRM  ☐ REVISE → ____________________
**DSP-10 verdict** (needs P2): ☐ CONFIRM  ☐ REVISE → ____________________

---

## Preset QA ×8 (FUNC-04 — remaining presets)

Audition each; accept or note a tweak. Tweaks loop back to Task 1 (edit the
`FactoryPresetDef` in `Source/PluginProcessor.cpp`), **then `rm -rf ~/Library/O-Contrabass/Presets/Factory/`
before reinstall** so the edit re-seeds (version-sentinel hazard).

| # | Preset | Intent | Sounds right? | Tweak requested |
|---|--------|--------|---------------|-----------------|
| Q1 | Section Bass | Damped, wide — ensemble blend | ☐ | |
| Q2 | Solo Arco Bass | Rosin/bow-noise forward, expressive vibrato | ☐ | |
| Q3 | Pianissimo Bass | Light bow, sul-tasto, quiet | ☐ | |
| Q4 | Forte Bass | Heavy pressure, bright, near-bridge bite | ☐ | |
| Q5 | Just-Intoned Drone | Recalls `DETUNE_A=+204 / D=−14 / G=+182` | ☐ | |
| Q6 | Scordatura Bass | C–G–D–A fifths retune (`DETUNE_E=−400 / A=−200 / G=+200`) | ☐ | |
| Q7 | Sub Drone | Heavy sub-harmonics, very dark, 2 low strings | ☐ | |
| Q8 | Dark Pad Bass | Damped, dark, slow swell, wide | ☐ | |

**Skew round-trip check** (do once, any preset): open the generic/parameter view after
loading a preset and confirm a skewed param reads its intended engineering value — e.g.
**Forte Bass → BOW_PRESSURE ≈ 3.2** (not ~0.15). A wrong value here = skew bug.

**FUNC-04 verdict** (10/10 accepted or all tweaks applied): ☐ CONFIRM  ☐ REVISE

---

## Optional objective anchor

Archive a few `--out` harness renders (e.g. Cinematic default, Infinite Drone) as a
repeatable reference (O-Bassoon precedent) — not required for sign-off, but handy if a
future v1.1 change needs an ear-anchor.

---

## Sign-off transcription (fill after auditioning)

```
FUNC-03: [CONFIRM|REVISE] — <one line>
DSP-10:  [CONFIRM|REVISE] — <one line>
FUNC-04: [CONFIRM|REVISE] — <presets accepted / tweaks applied>
Logic CPU spot-read (PERF-02 corroboration): __% at default preset, 256-block
```
