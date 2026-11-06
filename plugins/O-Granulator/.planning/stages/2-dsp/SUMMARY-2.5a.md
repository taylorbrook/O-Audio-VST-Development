# Stage 2 — DSP · Phase 2.5a (Tempo sync, LFO/env/macro sources, modulation matrix) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-05 · **Branch:** `main`
**Inputs:** PLAN-2.5a.md (T1–T14, P46–P59) · RESEARCH-2.5a.md (R39–R58) · CONTEXT-2.5a.md (D13–D16) · sign-offs R44 / R45 / R49 / R50 (at plan)

**Gate:** intra-stage (2.4 → 2.5a); 2.4 verified at `42d30b01`. The six contract checksums matched at entry; `Source/`, `tests/`, `CMakeLists.txt` and the modules were byte-identical to `42d30b01`. `run-gate.sh` critical checks (build, pluginval) PASS.

**Commits:**
- `4cce4f49` T1 — the N2 null (`tests/n2/`), baseline A 225/225 (two runs identical). No `Source/` change.
- `9db8a07b` T2–T5 — MockPlayHead v2 + block splits, plumbing, `TempoTracker.h`, the Tempo clock, P50 effective values.
- `e82b13f3` T6 — `ModSources.h` (LFO ×4, env follower, macros), source telemetry. *(Its harness file already calls the T7 matrix units, so that one commit's harness compiles only from `10c8053a`.)*
- `10c8053a` T7–T11 — `ModMatrix.h`, ramped / latched / tick wiring, reverb atomics, telemetry; MF / PH / ZQ / TL / N8 / M5 / Z5 / C5 rows; N2n-MAC re-stimulated.
- this commit — T12–T14: `tests/hostcache/run.py`, DAW-2.5a.md, this summary, STATUS.

## What was built

| File | Role |
|---|---|
| `Source/dsp/TempoTracker.h` **(new)** | `TransportSnapshot` (POD, one per host block), `TempoTracker`: held anchor (R39), R42 rules, R44 host bars, `relockEpoch`; `floorMod`, `fracPart` (no `fmod`) |
| `Source/dsp/StreamScheduler.h` | `setTempo` (epoch fold like R4), `processTempo`: absolute-time grid clock, slot tracked, telescoping jitter; Free = `GrainScheduler` verbatim |
| `Source/dsp/ModSources.h` **(new)** | `Lfo` (six R51 shapes, `kLfoStream`, Free accumulator / Tempo direct, phase ramp on the shortest arc), `EnvFollower`, source ids |
| `Source/dsp/ModMatrix.h` **(new)** | 16 slots, residual ramps, sparse offsets, range table + raw pointers, engine ↔ raw maps, telemetry writer |
| `Source/dsp/Engine.h/.cpp` | tracker update per host block; `tickSources` → `matrixTick` at every tick; `applyTempoEffective`; ramped substitutes (`ExactRamp::setCell`); latched spawn copy; modulated reverb atomics / fb cutoffs; Diag + TestHooks + trace ring |
| `Source/dsp/ControlGrid.h` | 2.5a pointers + GridParams fields (choices via `lround`, clamped); `ExactRamp::setCell` |
| `Source/dsp/EngineConstants.h` | 2.5a constants, `kTempoDivBeats` / `kTempoDivBars`, `tempoDivBeats` |
| `Source/PluginProcessor.cpp` | 79 new pointers; playhead read once per block (the only call site); matrix ranges from the `AudioParameterFloat` objects |
| `tests/n2/` **(new)** | N2 null vs `42d30b01` + N2c, N2p, N2n-TMP / LFO / MAC / ENV |
| `tests/hostcache/run.py` **(new)** | HA gate on the installed VST3 (pedalboard via `uv`, ctypes CFRunLoop pump, JUCE state codec) |
| `tests/render-harness/engine_gates_25a.h`, `engine_gates_25ab.h` **(new)**, `main.cpp` | MockPlayHead v2 / `Transport`, `--only25a`, 74 new rows |

No module changed (`git diff --stat 42d30b01 -- modules` empty).

## Gate results

**Harness: 299 PASS, 0 FAIL** (225 carried + 74 new). Carried-row diff vs baseline A: **empty at every task**. 0 warnings (harness, plugin, install).

### Out of tree

| Row | Result | Measured |
|---|---|---|
| N2 free / capped | PASS | max\|Δ\| 0 both; landed / raw checksums equal; below knee |
| N2c seed + 1 | PASS (differs) | 568,972 diffs |
| N2p zero-depth routes | **IDENTICAL** at every task | R46 by construction |
| N2n-TMP / LFO / MAC / ENV | all DIFFER, `--strict` PASS | TMP flipped at T5; LFO, MAC, ENV with the matrix (T7–T9 landed together) |
| N1 `--strict`, N0 `--strict`, R12 | PASS | unchanged |

### New rows (selection; all PASS)

| Row | Measured |
|---|---|
| TT0 mock v2 | tempo map continuous 1e-12; loop / locate / pre-roll / 3/4 → 7/8; 6 events on block starts in 5 variants |
| U9a divisions | 22 entries == parsed strings; prefixes 20 / 19 / 22 |
| TT1 / TT1n | grid ppq bitwise across 32/441/512/4096/var at 120/133/97.3 bpm, 0 re-anchors; per-block origin differs at 29.6 % (R39: 30–52 %) |
| TT2–TT5 | step 1/0; loop, locate, start 1/1 (5 variants); stopped virtual ppq; 120 bpm fallback; trapezoid 0 re-locks on 60→180 bpm (constant-pps predictor misses by 161 samples, INFO); bars 4/4, 3/4, 7/8, change, pickup; pre-roll |
| TS1 / TS1n | 19 divisions × 8 transports: onsets == analytic grid, worst |Δ| **0**, 13,158 onsets, 3 boundary slots excluded; per-tick ceil drops 5 slots |
| TS2–TS6 | enable / Free→Tempo on the next slot, epoch folded; jitter 50 % within J·I/2; R45 clamps; quotas from effective density; stopped 40 onsets; FUNC-04b 0.5 / 200 g/s |
| LS1–LS5 | shapes bitwise; smooth random continuous; Free accumulator no jump (LS2n jumps 0.600 cycle); Tempo direct bitwise over 64 bars; 359°→1° = +2°; pre-roll S&H |
| EV1–EV3, MC1 | follower bitwise vs the one-pole, 63 % at τ, −20 dBFS → 0.667, silence → 0; env_gain; return not tapped; input_gain −24 → −0.400; frozen tracks; NaN scrub → 0, audio bitwise; macro linear over 30 cells |
| U9b–g | layout; range parity 7168/7168 bitwise; sum before clamp; residual continuous (U9en step 0.49); maps ≤ 8 ulp; round trip moves 1339/7168 values |
| MF1–MF5 | 13 ramped dests ±50 % within 5.2e-7 rel; base ramp preserved; 23 latched dests ±40 % **bitwise** (1690 spawns); tick consumers == knob at the modulated value; Tempo ×2 on the grid, size 250 ms |
| MF6 / MF7 | 9 sources × 5 classes × ±100 % direction + range 90/90; Pitch/Onset inert (0 diffs); removal 12/12 within 8.2e-4 / sample (MF7n 0.0156); removal κ 0.95× held |
| PH1–PH3 | Δφ == previous-cell increment (≤ 3.4e-15); PH1n jumps 0.300 cycle; self / cross deterministic with one-cell lag; Accum until the loop re-lock, then Direct bitwise |
| ZQ1 / ZQ2 | mix worst 0.952 of the analytic slope bound; latched 36/36 exact; mix κ 1.00× smooth ref (ZQ2n 19.85×); latched κ within 2.0× the same per-grain span |
| TL1 | telemetry == v01 / base01; 220 params bitwise unchanged over 10 s |
| N8 / N8c / N8n | 441/512/4096/var vs 32 max\|Δ\| 0 (4 re-anchors, 2 re-locks, 13 route changes, 24 routed, 533 tempo onsets); seed control 274,526 diffs; perBlockOrigin 49,019 diffs |
| M5 | 0 allocations (liveness 1; 15 route changes, 3 re-locks) |
| Z5 | 60 s × 48 / 44.1 / 192 k: finite, 0 subnormal (432 scans each), wet ≤ A (max 1.9953), out ≤ its gain bound (8.88), 0 scrubs / kills |
| C5 (INFO) | **16.26 %** of one core (C4 re-run 17.52 %) — headroom 13.74 % to 30 % |

### Validation (T12 / T13)
- Static: no `std::vector|new|malloc|make_unique|std::function` in the three new headers; no `fmod` in TempoTracker / ModSources / StreamScheduler; no APVTS write in `Source/dsp`; one playhead call site (`processBlock`); modules unchanged.
- `build-and-install.sh O-Granulator`: 0 warnings, dual-variant sweep, caches cleared. `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10 VST3 + AU PASS.
- **HA** (installed VST3): HA1 69/69, HA2 152 untouched params unchanged, HA3 state XML 69/69 — PASS.
- `DAW-2.5a.md` written (items 1–8) for verify.

## Errata (ARCHITECTURE / ROADMAP not edited — checksums)
1. R39 held anchor vs ARCH §17 per-block `ppqOrigin`.
2. R40 slot tracking vs ARCH "`ceil (ppq/div)·div` at every grid point".
3. R41 plugin-local Tempo clock; ARCH "re-time if > 0.5 sample off" → recomputed every piece. G3 stays optional.
4. R42 trapezoid prediction; stopped transport ignores host ppq.
5. R44 (signed off) host time signature bars.
6. R45 (signed off) Tempo size clamp 2 s; density multiplier in power-of-two steps in [0.5, 200] g/s. Stage 3 tooltip note.
7. R46 sparse evaluation; unrouted dests bypass the round trip.
8. R47 / R48 modulation after the base ramp; latched offsets lerped at the spawn sample.
9. R49 (signed off) per-destination residual vs ARCH §16 per-slot shadow.
10. R50 (signed off) Free LFOs always accumulate; Direct is Tempo-only.
11. R51 `kLfoStream`, cosine smooth random; S&H repeats per loop pass in Tempo-direct.
12. R54 reverb_mix ≤ 0.001 clears the tank; reverb_size smoothed 100 ms.
13. R55 no Choice-backed destinations; Stage 1 golden reused.
14. R56 mock playhead v2; tempo ramps are DAW-only.

## Execution deviations (for review at verify)
1. **Discontinuity predictor = step OR trapezoid** (R42 named the trapezoid only). A host tempo *step* reports the new bpm at a block start after a block that ran at the old tempo; the trapezoid alone misses it by n·Δpps/2 (≈ 400 samples at 4096) and would re-lock. A re-lock now needs the host to miss both predictions by > 1 ms. TT2 (step: 0 re-locks) and TT3 (ramp: 0 re-locks) cover both.
2. **Bar-frame change re-anchors** (not in P48): a host moving `ppqPositionOfLastBarStart` without a signature change (a pickup bar) re-anchors so `barOrigin` follows. TT4 "pickup bar".
3. **Slot re-derivation only at re-lock / enable / mode switch / division change** — not at a plain re-anchor (a tempo change keeps ppq continuous; re-deriving there risks a jittered double). P49 listed "re-anchor".
4. **TS1n stimulus**: 133 bpm / 1/16 T instead of 120 bpm / 1/16 — at 120 bpm every grid point is an integer sample and the ε-ceil is exact, so the plan's stimulus is vacuous. The per-tick ceil drops 5 slots at 133 bpm.
5. **TS3 mean bound**: the plan's "mean interval within 1e-9 relative" can't hold — telescoping leaves J·I·(u_N − u_0)/(N − 1). The row asserts that bound, every onset within J·I/2 of its slot, and the exact count.
6. **TS1 boundary slots**: a slot due inside the last sample before a transport event fires on the event's first sample against the new anchor; 3 such slots are excluded from both sides and reported.
7. **N2n-MAC at −1 %** (plan: +1 %): the N1/N2 recipe has mix at 100 %, so +1 % clamps at the range end and the control stays identical. Re-stimulated, it differs.
8. **Z5 peak gate** split: the plan's "peak ≤ float(A)" can't hold with `output_gain` (+12 dB) and `mix` among the routed dests and a 0 dBFS dry. Gated instead: the post-knee wet ≤ float(A) (new test-only `Diag::maxWetPeak`; max 1.9953) and the output ≤ √(1 + A²)·dbToGain(12) = 8.885 (max 8.879 at 192 k).
9. **ZQ2 latched references**: a depth-0 render is not a same-motion reference (pitch spread alone raises curvature by f²). References are the same per-grain span without the matrix: spray 100 %, pitch_rand 24, filter_rand 100 % (cutoff routed at 20 % so its span 312–7918 Hz matches ±2 oct).
10. **Matrix range table from the parameter objects** (`getNormalisableRange()`), not re-built from the contract rows — bitwise the same source; U9c proves parity.
11. **Per-task commit split**: T7–T9 landed together (the N2n LFO / MAC / ENV flips were not observed one task at a time); T6's commit carries harness rows that compile only from the next commit.
12. Units were written against the implementation rather than a stub first; non-vacuity is shown by the negative controls (TT1n, TS1n, LS2n, U9en, MF7n, PH1n, ZQ2n, N8n), each of which fails its gate.

## Carried
- **2.5b:** onset detector + burst (D14), decimator + MPM, absolute pitch follow, Pitch / Onset sources live; N3 null vs the 2.5a tree.
- **2.6:** full PERF-02 budget gate; `getTailLengthSeconds()`; full QUAL-01 click sweep incl. route edits under all sources.
- **3.x:** R45 clamp note in the size-sync tooltip; mod rings draw v01; telemetry accessors (`getDestTelemetry`, `getSrcTelemetry`) feed the UI.
- **Verify:** run DAW-2.5a.md items 1–8 (D16).
- **Out of scope:** O-GrainScatter stopped-transport re-fire (R43) — separate GrainScatter issue.
- W2o unchanged: widen seeds, never the threshold.
