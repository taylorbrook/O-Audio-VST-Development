# Stage 2 (DSP) — PLAN

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 2 of 4 — DSP Implementation
**Phase:** plan
**Date:** 2026-06-26
**Inputs:** `stages/2-dsp/CONTEXT.md` (scope D1–D3), `stages/2-dsp/RESEARCH.md` (port-ready code, R1–R8), `research/ARCHITECTURE.md` (immutable contract), Stage-1 build (`Source/`).

---

## Goal

Fill the silent Stage-1 shell with the real audio engine: `EXCITATION → RESONATOR →
MATERIAL/DAMPING`. Deliver a tunable, decaying, dynamically-responsive physical-modeling
synth across **String (Karplus-Strong)** and **Modal** resonators with three exciters
(**Pluck / Strike / Bow**). All 17 already-wired params now drive real DSP. Correctness
is gated by the offline render-harness using an **autocorrelation** pitch probe (±5 cents),
**re-run at the end of each of the three phases** (CONTEXT D3) before the next begins.

**Scope (LOCKED):** 3 must-phases — **2.1** (KS String + Pluck), **2.2** (Strike + Bow +
Material + Velocity + Position), **2.3** (Modal + cross-driving). Waveguide (2.4 / DSP-06)
is **deferred to v1.1**; `stringModel` ships exposing KS only (no contract break).

**The DSP design is immutable** (ARCHITECTURE.md, all 8 questions resolved). This plan does
not re-litigate DSP — it sequences the port-ready code RESEARCH.md already extracted.

---

## Approach (one paragraph)

Build DSP as **header-only** classes under `Source/` (keeps the render-harness clean — no
extra `.cpp`). Each voice owns one `StringResonator` (KS loop) and one `ModalResonator`
(8-mode biquad bank); a single computed `e[n]` from the selected exciter drives whichever
resonator `resonatorType` selects (cross-driving falls out for free — FUNC-04). The
processor reads params via `getRawParameterValue()->load()` and pushes them to voices at
block start; the Material macro writes back `damping`+`decay` so both knobs visibly co-move.
Stability is authored in (clamps, tanh-sat, DC-blocker, isfinite-guard, Q-clamp) because the
references lack it. Viz taps (copy-only atomics + ring) are wired through Stage 2, consumed
in Stage 3.

---

## Tasks

### Phase 2.0 — Prerequisites (setup)

**1. [ ] Add `bow-friction` shared module + copy `BowNoiseGenerator.h`** (R2, R8)
- Run `/module-add O-simplePhysicalModelSynth bow-friction` → adds `ouaricon_add_module(...)`
  to `CMakeLists.txt` and registers in the plugin's modules manifest. Module is dep-free,
  `cpp_standard 20`, already consumed by O-Bowed + O-Contrabass.
- Copy `O-Bowed/Source/DSP/BowNoiseGenerator.h` verbatim → `Source/BowNoiseGenerator.h`
  (bandpass-filtered noise drive: Bow→Modal driver **and** String-bow fallback).
- Files: `CMakeLists.txt`, `Source/BowNoiseGenerator.h` (new)
- Depends on: none

### Phase 2.1 — Core KS String + Tuning + Pluck

> First audio. Verifies FUNC-01, DSP-01, DSP-02, DSP-03 (partial), FUNC-02 (Pluck).

**2. [ ] Create `StringResonator.h`** — the KS loop (RESEARCH §1.1–1.4, §1.6–1.7)
- `OnePoleLPF` POD struct — **port verbatim** from `O-Lyrica/.../WaveguideString.h:191-216`
  (POD-ness is load-bearing for the shadow crossfade).
- `juce::dsp::DelayLine<float, Thiran>` — topology from `O-Bowed/.../WaveguideString.{h,cpp}`
  (sizing `fs/20+100`; **clamp `setDelay(max(2.0f, N))`** — Thiran needs ≥2 samples).
- Loop per sample: `out=pop; s=lpf(out); push(g*s + eSustain)`.
- Group-delay comp: `τ_lpf = fs/(2π·fc)`, `N = fs/f0 − τ_lpf` (Thiran absorbs the fraction —
  do NOT subtract a separate phase-delay term). **Recompute N on `damping` change too** (cutoff↔τ coupling).
- Feedback from Decay: `g = lerp(0.80, 0.999, decay/100)`, hard-clamped `< 1`.
- Damping→cutoff: `fc = 12000·pow(1500/12000, damping/100)` (log; more damping = darker).
- Shadow-filter crossfade (64 samples) — **port verbatim** `O-Lyrica/.../WaveguideString.h:229-233 + .cpp:140-161`; snapshot+restart inline when the audio-thread target cutoff differs.
- DC blocker — **author new** (F1: none to port): `y=x−x1+0.995·y1` on String output.
- Files: `Source/StringResonator.h` (new)
- Depends on: none

**3. [ ] Create `PluckExciter.h`** — noise burst + brightness LPF + position comb (RESEARCH §1.5)
- White noise (`juce::Random`, per-voice seed) × ~5–15 ms ramp/ADSR burst → brightness
  `OnePoleLPF` (cutoff from `excitationColor`, raised by velocity via `velToBrightness`).
- Position comb `y=x−x[n−D]` (linear-interp fractional) — **port** `O-Lyrica/.../PluckExciter.cpp:129-144`;
  `D = jlimit(0.05,0.95,position)·(fs/f0)`, `MAX_COMB_DELAY=8192`.
- Files: `Source/PluckExciter.h` (new)
- Depends on: none

**4. [ ] Wire Pluck→String chain into `PhysicalModelVoice.h`** (RESEARCH §9; ARCHITECTURE §7)
- Replace the silent `renderNextBlock` no-op. Per-voice: own a `StringResonator` + `PluckExciter`
  + `juce::ADSR` (amp). `startNote`: compute `f0 = 440·2^((note−69)/12)·2^((coarse+fine/100)/12)`,
  seed the pluck burst into the loop, trigger amp ADSR. `stopNote`: ADSR release (tail rings —
  no longer immediate `clearCurrentNote`); free voice when ADSR idle.
- `prepareToPlay`: size delay line / reset filters at known `sampleRate`.
- `juce::ScopedNoDenormals` already at processor scope; voice adds into the buffer.
- Files: `Source/PhysicalModelVoice.h` (edit)
- Depends on: Tasks 2, 3

**5. [ ] Push params to voices each block in `PluginProcessor.cpp`** (RESEARCH §9)
- Read all live params via `getRawParameterValue(id)->load()` at block start (no per-sample APVTS).
- Add a `setParams(...)` (or a small param struct) on the voice; push before `synth.renderNextBlock`.
- Wire `outputLevel` (dB→gain) on the master buffer; `coarse/fine` into f0; `ampAttack/ampRelease`
  into the voice ADSR; `damping`/`decay`/`excitationColor`/`excitationPosition` into String/Pluck.
- Files: `Source/PluginProcessor.cpp` (edit), `Source/PhysicalModelVoice.h` (param setter)
- Depends on: Task 4

**6. [ ] Harness 2.1 — invert to `makes-sound` + autocorrelation ±5-cent gate** (RESEARCH §5, R8)
- `tests/render-harness/main.cpp` only — **CMake stays untouched** (already `JUCE_WEB_BROWSER=0`,
  drops `PluginEditor.cpp`; do not regress).
- Invert `shell-silent`→`makes-sound` (`r > 0.02` RMS). Add `render()`/`rms`/`peakAbs`/`allFinite`
  helpers (O-simpleFM pattern); analysis window early/short (post-attack, pre-noise-floor).
- Add `autocorrPitchHz` **verbatim** (`O-simpleGrain/.../main.cpp:112-141`); bracket search per
  note `[f0·0.8, f0·1.25]`; assert `|cents| ≤ 5` at **C1/C3/C5/C7**.
- Build `-DOUARICON_BUILD_TESTS=ON`.
- Files: `tests/render-harness/main.cpp` (edit)
- Depends on: Task 5

**━━ GATE 2.1 ━━ Harness re-buildable + ALL PASS (tuning ±5¢ at C1/C3/C5/C7, makes-sound, finite). Do not start 2.2 until green.**

### Phase 2.2 — Strike + Bow + Material + Velocity + Position

> All three exciters on the String; material/dynamics complete. Verifies FUNC-02 (all), FUNC-06, DSP-07, DSP-08, QUAL-01.

**7. [ ] Create `StrikeExciter.h`** — band-limited raised-cosine mallet (RESEARCH §2.1, **build new**)
- Raised-cosine (Hann) windowed impulse, width `W` samples (harder = narrower → brighter):
  `burst[n]=0.5·(1−cos(2π(n+1)/(W+1)))` (smooth on/off → no DC step, band-limited — DSP-08).
- Hardness `OnePoleLPF` (cutoff from `excitationColor`); velocity raises width+cutoff via `velToBrightness`.
- Files: `Source/StrikeExciter.h` (new)
- Depends on: none (parallel with Task 8)

**8. [ ] Create `BowExciter.h`** — module friction + KS injection (RESEARCH §2.2–2.3, R2; CONTEXT D2)
- Wrap module `HyperbolicFriction` (`computeReflectionCoefficient`); **memoryless-first** —
  validate a basic sustained tone before refining.
- KS injection (adapt O-Bowed `writeJunction`): read loop velocity → `vDelta=vBow−vString` →
  `rho` (clamp 0.85 then 0.99) → `fVel=2ρ/(1−ρ)` → `inject=copysign(min(fVel,|vDelta|),vDelta)` →
  `tanh` soft-sat (sat=4) → push. `bowForce`→`F_bow`.
- **Stability guards (port all):** rho clamps, tanh-sat, denormal flush `if(|y|<1e-15) y=0`, final `jlimit(-1,1)`.
- Fallback (D2): if the table destabilizes the loop / reads poorly → sustained `BowNoiseGenerator` drive into the loop.
- Files: `Source/BowExciter.h` (new), uses `Source/BowNoiseGenerator.h` (Task 1)
- Depends on: Task 1

**9. [ ] Switchable exciter + Position comb in the voice** (RESEARCH §2.5, §3.6; FUNC-02)
- `excitationType` selects Pluck/Strike/Bow producing the single `e[n]`. Pluck/Strike seed the
  loop and decay (`eSustain→0`); Bow injects continuously (sustains while held — no ADSR decay).
- Excitation Position comb applied on the KS path. Swapping exciter changes only attack/drive.
- Files: `Source/PhysicalModelVoice.h` (edit)
- Depends on: Tasks 7, 8

**10. [ ] Material macro + velocity dynamics in `PluginProcessor.cpp`** (RESEARCH §2.4–2.5; DSP-07, FUNC-06)
- Material macro (constants resolved §2.4): `fc=exp(lerp(ln10000,ln2000,material/100))`,
  `g=lerp(0.995,0.93,material/100)`; **write back** visible `damping%`/`decay%` so both knobs
  co-move. Apply before the loop reads `damping`/`decay`. Decide UI-driven vs block-start apply (note §2.4).
- Velocity (0–127→0–1) scales exciter amplitude AND raises exciter-LPF cutoff; depth = `velToBrightness` (shared across all 3 exciters).
- Files: `Source/PluginProcessor.cpp` (edit), `Source/PhysicalModelVoice.h` (velocity hook)
- Depends on: Task 9

**11. [ ] Harness 2.2 — exciter character + stability** (RESEARCH §5 item 5; QUAL-01, DSP-08)
- Add asserts: **Bow-sustains** (RMS steady while held), **Bow+max-Decay+max-Force finite/bounded**,
  **decay-shortens-with-Decay**, **no-DC** (mean≈0 post-DC-blocker), **Strike-no-top-octave-alias**
  (add oversampling ONLY if this trips — no oversampling v1.0).
- Files: `tests/render-harness/main.cpp` (edit)
- Depends on: Task 10

**━━ GATE 2.2 ━━ Harness ALL PASS incl. Bow bounded at max Force + max Decay, no click/DC/alias. Do not start 2.3 until green.**

### Phase 2.3 — Modal resonator + cross-driving

> Second resonator engine, driven by the SAME `e[n]`. Verifies FUNC-03, FUNC-04, DSP-04, DSP-05.

**12. [ ] Create `ModalResonator.h`** — 8× exciter-driven biquad bank (RESEARCH §3, **build new**; R3, R4, R7)
- `ModeBiquad` direct-form struct with **isfinite guard** (port from `O-Bassoon/.../ModeBank.h:61-89`).
- **RBJ constant-skirt bandpass** coeffs (peak gain ∝ Q — the antidote to "inaudible sustain");
  recompute at **block rate**, global `~1/Σamp_k` normalization.
- Inharmonicity (Fletcher stretch, NOT O-Bells tables): `f_k=f0·k·√(1+B·k²)`, `B=(inharm/100)·0.012`.
- T60→Q **derive ourselves**: `Q_k=0.4548·f_k·T60_k`, `T60_k=baseT60·DECAY_MULTIPLIERS[k]`,
  `baseT60=lerp(0.3,6.0,decay/100)`; **clamp Q ≤ 500** (authored — references lack it).
- Reuse **data only** from O-Bells: `DECAY_MULTIPLIERS[8]` (`BellVoice.h:74-75`) + amplitude tilt
  (`BellVoice.cpp:744-758`, `modeBrightness`→per-mode output gains).
- Reserve fallback (R7): if modes ring too quietly → O-Bassoon `strike()` state-injection.
- Files: `Source/ModalResonator.h` (new)
- Depends on: none (parallel with 2.2)

**13. [ ] Resonator switch + cross-driving in the voice** (RESEARCH §3.6; FUNC-03, FUNC-04)
- `resonatorType` routes the single `e[n]` to String (KS loop) or Modal (bandpass bank).
  Modal is feed-forward (no stability loop). Bow→Modal uses continuous `BowNoiseGenerator` noise
  as `e[n]` (sustains the ring); Pluck/Strike→Modal ring-and-decay per T60.
- `inharmonicity`/`modeBrightness` are no-ops in String; `stringModel`/Position-pickup no-ops in Modal.
- Files: `Source/PhysicalModelVoice.h` (edit), `Source/PluginProcessor.cpp` (modal param push)
- Depends on: Task 12 (and Tasks 8/9 for Bow→Modal driver)

**14. [ ] Harness 2.3 — modal sound + cross-driving + inharmonicity** (RESEARCH §5)
- Add asserts: **modal makes-sound** on each exciter (cross-driving — Pluck/Strike/Bow all drive Modal),
  modal note rings + decays per Decay, **inharmonicity stretches partials** (0%≈harmonic/bar, high%≈bell).
- Files: `tests/render-harness/main.cpp` (edit)
- Depends on: Task 13

**━━ GATE 2.3 ━━ Harness ALL PASS incl. each exciter drives Modal, inharmonicity audible. ━━**

### Cross-cutting — Viz wiring (spans 2.1→2.3, confirmed end of stage)

**15. [ ] Create `VizTap.h` + lead-voice publish + processor write** (RESEARCH §6; PERF-01)
- `VizRing` 8192-sample lock-free ring — **port verbatim** `O-simpleFM/Source/FmVizAnalyzer.h:30-58`
  (copy-only write, relaxed stores + release writePos). Fully allocated at construction.
- **Author new** (not in FmVizAnalyzer): `std::atomic<float>` **loop-energy scalar** (KS circulating
  energy, UI-02) — land during 2.1; fixed-size 8-entry `(f_k, amp_k)` **modal-stem array** — land during 2.3.
- Lead voice = most-recently-triggered (ARCHITECTURE §7) publishes both. **Audio-thread copy-only,
  no alloc/lock/FFT.** FFT/scope/stems are computed on the Stage-3 editor Timer, not here.
- Files: `Source/VizTap.h` (new), `Source/PhysicalModelVoice.h` (publish), `Source/PluginProcessor.cpp` (VizRing write)
- Depends on: Task 4 (loop-energy), Task 12 (stem array) — **confirm taps running by GATE 2.3**

---

## File Manifest

**New** (`Source/`, all header-only):
- `StringResonator.h` (T2) · `PluckExciter.h` (T3) · `StrikeExciter.h` (T7) · `BowExciter.h` (T8)
  · `BowNoiseGenerator.h` (T1, copied) · `ModalResonator.h` (T12) · `VizTap.h` (T15)

**Edit:**
- `Source/PhysicalModelVoice.h` — exciter→resonator chain, amp ADSR, velocity, per-voice state, viz publish (T4, T9, T13, T15)
- `Source/PluginProcessor.cpp` — param `load()` reads + push, Material macro, lead-voice tracking, VizRing write (T5, T10, T13, T15)
- `CMakeLists.txt` — `ouaricon_add_module(... bow-friction)` (T1; `juce_dsp` already linked)
- `tests/render-harness/main.cpp` — §5 harness changes (T6, T11, T14); **CMakeLists untouched**

---

## Dependency Graph

```
T1 (module + BowNoiseGen) ──────────────► T8 (Bow)
T2 (StringResonator) ─┐
T3 (PluckExciter) ────┴► T4 (voice: Pluck→String) ─► T5 (processor push) ─► T6 (harness 2.1) ═GATE 2.1═
                                                                                      │
T7 (Strike) ─┐                                                                        ▼
T8 (Bow) ────┴► T9 (switchable exciter + Position) ─► T10 (Material+velocity) ─► T11 (harness 2.2) ═GATE 2.2═
                                                                                      │
T12 (ModalResonator, parallelizable) ─► T13 (resonator switch + cross-drive) ─► T14 (harness 2.3) ═GATE 2.3═
T15 (Viz) spans: loop-energy after T4, stems after T12, confirmed at GATE 2.3
```

---

## Success Criteria (Stage 2 exit — carried from CONTEXT §Success Criteria)

- [ ] Note-on → plucked tone rings + decays; **f0 within ±5 cents at C1/C3/C5/C7** (autocorrelation harness)
- [ ] **Pluck**=plucked, **Strike**=mallet, **Bow**=sustained (no decay while held); swapping exciter changes only attack/drive
- [ ] **Material** sweeps steel→nylon in one gesture; Damping+Decay visibly co-move; harder velocity = brighter/stronger
- [ ] **Modal** = inharmonic struck-bar/bell; each exciter drives Modal (cross-driving); Inharmonicity 0%≈bar, high%≈bell
- [ ] Feedback clamped **< 1** — bounded/finite at max Decay AND max Bow Force (QUAL-01); no click/DC/buzz/alias
- [ ] **Viz taps running** (loop-energy + modal-stem snapshots), audio-thread copy-only
- [ ] **Render-harness re-buildable + ALL PASS** (all three phase gates); **pluginval clean**

---

## Risk Register (carried from RESEARCH §8)

| Risk | Phase | Mitigation (in plan) |
|------|-------|----------------------|
| KS tuning drift > ±5¢ (Thiran top octave) | 2.1 | Autocorr gate; fallback per-note offline correction table |
| Bow friction blow-up (NaN/screech) | 2.2 | rho clamps + tanh-sat + denormal flush + jlimit; harness asserts bounded at max Force+Decay; noise-drive fallback |
| Modal "inaudible sustain" (−50 dB) | 2.3 | Constant-skirt RBJ (peak gain∝Q) → energetic drive → O-Bassoon `strike()` fallback |
| Zipper clicks on cutoff change | 2.1 | 64-sample shadow-filter crossfade |
| Strike top-octave aliasing | 2.2 | Harness audit; oversampling ONLY if it trips |
| Render-harness breaks on WebView editor | all | CMake already `JUCE_WEB_BROWSER=0` + drops `PluginEditor.cpp` — do not regress |
