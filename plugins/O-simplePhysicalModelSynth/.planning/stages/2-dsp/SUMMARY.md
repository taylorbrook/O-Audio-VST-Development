# Stage 2 (DSP) — Execution Summary

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 2 of 4 (DSP Implementation)
**Phase:** execute
**Date:** 2026-06-26
**Result:** ✅ Complete — the silent shell now sounds. All three harness gates green (23/23 checks), pluginval strictness-10 SUCCESS.

---

## What was built

The full audio engine `EXCITATION → RESONATOR → MATERIAL/DAMPING`: a swappable exciter
(**Pluck / Strike / Bow**) drives one of two resonators (**Karplus-Strong String** /
**8-mode Modal bank**) — the same per-sample `e[n]` feeds whichever is selected, so
cross-driving (any exciter → any resonator, FUNC-04) falls out for free. All 17 wired params
now drive real DSP. Phased + harness-gated per CONTEXT D3 (2.1 → 2.2 → 2.3).

## Files

**New** (`Source/`, all header-only DSP — keeps the render-harness clean):

| File | Role |
|------|------|
| `OnePoleLPF.h` | Shared bilinear one-pole (O-Lyrica port) — loop damping, Pluck brightness, Strike hardness |
| `PositionComb.h` | Shared feed-forward position comb `y=x−x[n−D]` (O-Lyrica port) — Pluck + Strike |
| `StringResonator.h` | KS loop: Thiran `DelayLine` + loop LPF + shadow crossfade + **exact phase-delay tuning** + DC blocker |
| `PluckExciter.h` | Hann noise burst → position comb → brightness LPF |
| `StrikeExciter.h` | Band-limited raised-cosine mallet → position comb → hardness LPF |
| `BowExciter.h` | Friction-weighted band-limited noise drive (Stribeck coupling from the `bow-friction` module) |
| `BowNoiseGenerator.h` | Copied verbatim from O-Bowed (bandpass bow noise) |
| `ModalResonator.h` | 8× RBJ constant-skirt bandpass + Fletcher inharmonicity + T60→Q + tilt + isfinite/Q guards |
| `VizTap.h` | `VizRing` (O-simpleFM port) + loop-energy scalar + 8 modal-stem atomics (Stage-3 taps) |

**Edited:** `PhysicalModelVoice.h` (exciter→resonator routing, amp ADSR, velocity, viz publish),
`PluginProcessor.{h,cpp}` (cached param push, Material macro listener, master gain, VizRing write + lead-voice publish),
`CMakeLists.txt` (`ouaricon_add_module bow-friction` + header source list),
`tests/render-harness/main.cpp` (autocorrelation pitch probe + 23 acceptance checks),
`modules/registry.yaml` (added as a `bow-friction` consumer).

---

## Harness gates (re-run at the end of each phase — CONTEXT D3)

```
GATE 2.1  makes-sound, finite, no-blowup, tuning ±5¢ @ C1/C3/C5/C7 ........ PASS
GATE 2.2  bow-sustains, bow-bounded(maxForce+maxDecay), decay-shortens,
          no-DC, strike character, strike-hi-clean ......................... PASS
GATE 2.3  modal-pluck/strike/bow (cross-driving), inharmonicity-stretches,
          modal-rings, modal-decay-tracks-Decay ............................ PASS
```

Final full run: **ALL PASS — 0 failures (23 checks)**. Tuning accuracy: C1 −0.00¢, C3 0.67¢,
C5 0.05¢, C7 1.76¢. pluginval `--strictness-level 10` → **SUCCESS** (incl. parameter fuzz,
which exercises the Material macro write-back).

## Key implementation decisions (deviations / findings worth carrying)

1. **Exact phase-delay tuning (not DC group-delay).** The planned `τ_lpf = fs/(2π·fc)` DC
   approximation pushed C7 **+12.1¢ sharp**. Replaced with the loop LPF's exact phase delay at
   the fundamental (`τφ = −∠H(ω)/ω`) → C7 **1.76¢**. This is the resonance-correct condition.

2. **Bow = friction-weighted noise drive, NOT a friction junction (CONTEXT D2 fallback).**
   Memoryless STK friction self-oscillates only in a **dual-rail waveguide** (boundary
   sign-inversions form the Helmholtz corner — confirmed against O-Bowed `writeJunction`). In
   v1.0's **single KS loop** it settles into a stick state (vΔ→0) and decays. So the bow
   sustains via continuous band-limited friction noise whose coupling the Stribeck curve sets
   (the documented D2 fallback). The `bow-friction` module **is** used (sets coupling); the
   friction-junction path returns with the Waveguide string in v1.1. `processBow` was removed
   to avoid dead code in the v1.0 path.

3. **Material macro on the message thread.** Implemented as an APVTS listener on `material`
   that writes back `damping` + `decay` (so both knobs visibly co-move) — never touches the
   audio thread. Survives pluginval fuzz.

4. **`stringModel` ships KS-only (CONTEXT D1).** The choice param is wired but exposes only
   Karplus-Strong in v1.0; Waveguide (DSP-06) is the deferred v1.1 `nice`. No contract break —
   the param stays for forward compatibility. (The one param that does not yet alter the sound.)

5. **Consolidation:** `OnePoleLPF` and `PositionComb` extracted to shared headers (used by 2–3
   classes each) — no duplicated DSP.

## Success criteria (Stage 2 exit)

- [x] Note-on → plucked tone rings + decays; **f0 ±5¢ at C1/C3/C5/C7** (autocorrelation)
- [x] Pluck=plucked, Strike=mallet, Bow=sustained (rings out when lifted); exciter swap changes attack/drive
- [x] Material sweeps steel→nylon; Damping+Decay co-move; harder velocity = brighter/stronger
- [x] Modal = inharmonic struck-bar/bell; each exciter drives Modal; Inharmonicity 0%≈bar, high%≈bell
- [x] Feedback clamped < 1 — bounded/finite at max Decay AND max Bow Force; no click/DC/buzz/alias
- [x] Viz taps running (loop-energy + 8 modal stems), audio-thread copy-only
- [x] Render-harness re-buildable + ALL PASS; pluginval clean

## Notes for Stage 3 (GUI)

- Read viz via `processor.getVizTap()`: `waveform` ring (scope/FFT), `getLoopEnergy()` (UI-02
  dampening-loop diagram), `getStemFreq/Amp(k)` for k∈[0,8) (UI-05 modal stems).
- Editor is still the `GenericAudioProcessorEditor` placeholder behind the `#if JUCE_WEB_BROWSER`
  seam — re-run the render-harness at the START of Stage 4 (it compiles `PluginProcessor.cpp`
  under `JUCE_WEB_BROWSER=0`; the WebView editor must stay out of that TU).
- UI should grey `inharmonicity`/`modeBrightness` in String mode and `stringModel`/Position-pickup
  in Modal mode (per-resonator no-ops).
