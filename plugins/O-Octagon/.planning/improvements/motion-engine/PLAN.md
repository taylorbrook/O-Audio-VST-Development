---
milestone: motion-engine
domain: mixed
execute_agent: general-purpose
version_bump: minor
base_version: 1.7.0
target_version: 1.8.0
created: 2026-08-27
---

# Plan: Motion Engine — generative trajectories for the source puck

**Plugin:** O-Octagon
**Milestone:** motion-engine
**Created:** 2026-08-27
**Phase:** Execute — complete 2026-08-27 (see SUMMARY.md for deviations)

---

## Summary

**Improvement:** Six generative paths (Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral) produce an
(x, y, z) offset in venue metres added downstream of `srcX/srcY/srcZ`, which stay the anchor
(D1/D2); tempo-synced and PPQ-locked with a seeded, deterministic Drift (D7); trace + ghost anchor +
live puck on the venue map (D6).
**Approach:** A header-only, JUCE-free generator (`MotionPath.h`) evaluated at each boundary of the
existing absolute 64-sample control grid from absolute position (never integrated per call), injected
into `SourceShaper` through a new metres entry `shapeAt()` behind a `motionOn` branch, with the same
generator serving the map trace through one native function and the live puck riding `getMeters`.
**Version:** 1.7.0 → 1.8.0 (minor)

**Verified baseline:** `git diff --stat v1.7.0-O-Octagon HEAD -- plugins/O-Octagon/Source plugins/O-Octagon/tests`
is empty — the working tree IS the v1.7.0 DSP. Task 1 exploits that: the R3 digest is captured in place
before any edit, so no second checkout is needed.

### Plan calls on RESEARCH.md's open items (labelled)

| Item | Call |
|---|---|
| Layout (Risk 1) | **Compact `#group-motion` inserted directly after `#group-position`** (`index.html:366`), before `#group-solve`; body = 2-col `.group-body` grid, 5 rows: (On toggle, Path select) / (Sync select, Rate) / (Size, Ratio) / (Angle, Height) / (Phase, Seed). **Measured on the stub in Task 8 step 0 before any C++ editor change.** Fallback if `ui_layout_check` §21 fails: a 3-col `.group-body--dense` variant (label 64 px) dropping the row count to 4. |
| `motionAngle` | **In** — 10 parameters, `kCount` 18 → 28. |
| `motionSize` semantics | **Extent (diameter)**, `R = size / 2`. "6 m" reads as the ring's width on the map. |
| Drift on the map | **Trailing tail** of the last 48 polled positions, no trace. |
| **Preset policy — OVERRIDES RESEARCH** | RESEARCH puts the ten ids in `kPreserved`; that contradicts R7 ("save/load round-trips motion state", "motion state travels in presets"). **The ten go in `kAuthored`** (`PresetPolicy.h:84`) so presets carry motion. The six factory rows stay 6-column and write nothing for motion — WR-01's reset-to-default lands `motionOn = 0` on every factory load, so all six stay audibly identical to v1.7.0 (probe CP unchanged). Pre-1.8.0 user presets omit the keys → defaults → motion off (R7 second clause). |
| Preset migration gate | **No `preset-manager` migration hook** — no range moved (`pattern_preset_migration_per_param_version_gate` binds only when one does). The "gate" is probe DK: load a v1.7.0-shaped preset (18 keys) and assert `motionOn == 0` plus the 18 bit-unchanged. |
| SAFE-mode instrumentation | Add `oo::instr::motionSolves` beside `monitorSamples` (`DbapSolver.h:102`) — counts `updateControl()` passes that took the motion branch. DC asserts it is 0; DE asserts it is > 0. |

### RESEARCH.md citations corrected against the tree

| RESEARCH said | Actual |
|---|---|
| `updateControl()` at `GainStage.cpp:777-830`; dirty check `:794`; `solveSubPoint` Z `:828-829`; `renderChunk` `:630+` | `updateControl()` **`GainStage.cpp:342`**; dirty check **`:357-359`**; `shaper::shape` call **`:369-371`**; `solveSubPoint (…, p[srcZ], …)` ×2 **`:393-394`**; `renderChunk` **`:607`**. The grid walk `:311-337` is right. |
| `metresToPx()` at `roomplan.js:74` | **`roomplan.js:62`**; `:74-79` is the JS `normToMetres` twin |
| `oo::instr` in GainStage | `namespace instr` is **`DbapSolver.h:57`** (`resetCounters()` `:113`) |
| "v1.3.0 preset migration hook" precedent | Fine, but the ten ids belong in `kAuthored`, not `kPreserved` (above) |
| Native count 26 → 27 | Confirmed: `ui_frontend_check.js:246` asserts 26 today |
| Harness 61 probes | `grep -c 'check ("'` = 66 call sites (some multi-line); the number the plan gates is "**the count before Task 1 + the new probes, all green**", not 61 |

---

## Task Breakdown

### Task 1: Anchor the R3 digest on the untouched v1.7.0 DSP

**Outcome:** Probe **DC** exists in `tests/render-harness/main.cpp` in the CU shape (`:5710-5782`):
motion-off scenario, `bufferDigest()`, a transcribed `kV170Digest` constant, `motionSolves == 0`
(placeholder `0` until Task 4 adds the counter — the assertion is written now, compiled against the
counter later), non-silent (`live`). The constant is captured by running the probe **before any DSP
file is edited** and transcribing the printed digest, with the capture commit hash in the comment.
Scenario: same puck/width/air/weights as CU plus `srcZ = 1.4`, ragged sizes `{ 1, 7, 64, 333, 4096 }`,
events on `srcX`, `blur`, `rolloff`; 4 × 4096 samples.

**Verification:**
- [x] `cmake --build build --target O-Octagon-render-test && ./build/plugins/O-Octagon/tests/render-harness/O-Octagon-render-test_artefacts/Release/O-Octagon-render-test` — all pre-existing probes green, DC prints its digest — **61/61 pre-existing green; DC printed `0xb8c5a2d0c7518204`** (an srcZ event at 4096·2+1000 was added so DC is not CU's constant at ragged sizes)
- [x] `kV170Digest` transcribed into the source with the commit hash; a second run passes DC — **`0xb8c5a2d0c7518204ull`, commit `2e03020e`; two consecutive runs 62/62**
- [x] `git diff --stat v1.7.0-O-Octagon -- plugins/O-Octagon/Source` is still empty at the moment of capture — **empty, checked before each of the three capture runs**

**Dependencies:** None
**Estimated effort:** Small

---

### Task 2: Parameter group `motion` (kCount 18 → 28) and preset partition

**Outcome:** `params::Index` + `id()` table (`GainStage.h:61-92`) gain `motionOn, motionPath,
motionSync, motionRate, motionSize, motionRatio, motionAngle, motionHeight, motionPhase, motionSeed`
in that order at the end; `kCount == 28` static_assert (`GainStage.h:73`). `createParameterLayout()`
(`PluginProcessor.cpp:67`) adds a `motion` group with new `makeBool`/`makeChoice` helpers beside
`makeFloat` (`:46`): ranges per RESEARCH table (`motionRate` 0.01–4 Hz **skew for centre 0.3 Hz** —
the first non-linear range; `motionSeed` step 1, 1–64; Sync choices `Free` + O-Orbit's 14 divisions
in `MotionEngine.h` order; Path choices `Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral`). Choice
strings are ASCII (`String(const char*)` is ASCII-only). `kAuthored` (`PresetPolicy.h:84`) grows to
16; `kPreserved` untouched; partition static_assert holds. `snapshotParameters()`
(`PluginProcessor.cpp:305`) works unchanged (Bool/Choice raw values are floats). `.planning/parameter-spec.md`
gets the 28-row table as an amendment.

**Verification:**
- [x] Plugin + both test targets compile with zero warnings — **zero `warning:` lines, all four targets**
- [x] Render harness: every pre-existing probe green (DC included); probe CP still passes with 12 preserved — **63/63; DC digest unchanged at kCount 28; CP "the TWELVE BIT-UNCHANGED"**
- [x] Probe **DK** added — **PASS: 18/18 bit-exact (dyadic k/64 fixture so the JSON text round-trip is exact), motionOn OFF, 10/10 at default**
- [x] `ui_frontend_check.js` §15/§16 literals updated 18 → 28 — **failed as expected at Task 2 (6 FAILs: layout 23/25 parsed, DOM 18, app 18); green after Task 8**

**Dependencies:** Task 1
**Estimated effort:** Medium

---

### Task 3: `MotionPath.h`, `PerlinNoise.h`, `MotionClock.h` + unit probes

**Outcome:** Three header-only, JUCE-free files in `Source/DSP/`:
- `PerlinNoise.h` — byte-copy of `plugins/O-Orbit/Source/DSP/PerlinNoise.h` (93 lines, `<cmath>`/`<cstdint>` only) with a provenance comment.
- `MotionPath.h` — `struct MotionParams { int path; float sizeM, ratio, angleDeg, heightM, phaseDeg; }`, `Vec3 evaluate (const MotionParams&, double cycles, const PerlinNoise&) noexcept` implementing the six equations of RESEARCH (R = size/2, t = 2π·frac(cycles) + phase, rotation by angle, `z = height·sin t`, Drift = fbm at `cycles`, `+1000`, `+2000`). `cyclic (path)` returns false only for Drift.
- `MotionClock.h` — `struct HostClock { double bpm = 120.0; double ppq = 0.0; bool ppqValid = false; bool playing = false; }`; `double cyclesAt (uint64_t absoluteSample, double sr, const HostClock&, int syncIndex, float rateHz, MotionClockState&) noexcept` per RESEARCH Q1/Q7: Free = `rate·t + phaseBase`; Synced+rolling = extrapolated PPQ × multiplier; Synced+stopped = hold last; Synced+no PPQ = free-run at `bpm/60·mult`. `MotionClockState` holds only `phaseBase`, `lastRate`, `heldCycles` — **no accumulator**. Free-mode rate change re-bases at the grid boundary.

Unit probes in `tests/unit/main.cpp` (geometry target, `tests/unit/CMakeLists.txt` needs no source change — headers only):
Figure-8 closes bit-exact at cycles 0 vs 1; Sweep max step over 4096 samples of one cycle `< 2R/2048`;
Orbit ratio 1 `|p| == R` to 1e-6; Spiral continuous at the half-cycle; `angle = 90°` maps Sweep x→y;
Drift same seed → bit-identical, different seed → differs; `cyclesAt` at sample N identical via one
call vs N/64 grid steps (Free and Synced); Synced+stopped holds.

**Verification:**
- [x] geometry target — all pre-existing + 8 new probes green — **57/57 (49 + MP1–MP8); MP2's bound is `<= 2R/2048 · 1.001` because the fold hits the ideal per-sample speed exactly**
- [x] `grep -l juce` on the three headers returns nothing — **0 files**
- [x] No filename contains a hyphen — **0 hyphenated names in Source/DSP**

**Dependencies:** None (parallel with Task 2)
**Estimated effort:** Medium

---

### Task 4: `SourceShaper::shapeAt()` and the motion branch in `GainStage::updateControl()`

**Outcome:**
- `SourceShaper.h:87` / `.cpp:29`: `shapeAt (v, Vec2 metres, srcZ, widthMetres)` holds steps 2–6; `shape()` becomes `return shapeAt (v, plane::normToMetres (…), srcZ, width);`. **If DC's digest moves after this refactor alone, revert `shape()` to verbatim and keep `shapeAt` beside it** (`pattern_reanchor_cross_version_digest_probe` — never re-record).
- `GainStage.h`: `HostClock` passed into `process()` (`:224-226`) as a new defaulted trailing parameter `const HostClock* clock = nullptr` (every harness call site compiles unchanged, as `monitorOn` did); members `PerlinNoise perlin`, `MotionClockState motionClock`, `int seededWith {-1}`, three `std::atomic<float> liveOffset[3]`.
- `GainStage.cpp:311-337`: at `phase == 0`, compute `cycles = cyclesAt (absoluteSampleCounter, sr, clock, …)` and `offset = evaluate (…)` **only when `p[motionOn] > 0.5f`**; publish `liveOffset`; call `updateControl (snapshot, p, offset, motionOn)`.
- `updateControl()` (`:342`): dirty check (`:357-359`) becomes `haveSolved && !motionOn && …`; then
  `motionOn ? shapeAt (snapshot, normToMetres(…) + offset.xy, p[srcZ] + offset.z, width)` : the verbatim `:369-371` call. Effective Z goes to **both** `solveSubPoint` calls (`:393-394`). `instr::countMotionSolve()` inside the motion branch only.
- Re-seed `perlin` when `motionSeed` changes (message-free, at the grid boundary; `seed()` is a 512-entry table fill — bounded, no allocation).
- RT-safety: no allocation, no lock, `noexcept` throughout.

**Verification:**
- [x] Render harness: DC green (digest unchanged, `motionSolves == 0`); every pre-existing probe green — **63/63 after the shapeAt() refactor; `shape()` delegates (no verbatim fallback needed)**
- [x] Probe **DD** — **PASS: 3000/3000 blocks unchanged, motionSolves 3128; companion: srcZ 2 == srcZ 0 + Height 2 at sin t = 1 bit-identical (both Z consumers)**
- [x] Probe **DI** — **PASS: 1500 × 3 offsets bit-identical with/without the step; renders differ**
- [x] **NC1** run once, restored (shasum identical) — **DE fails (`motionSolves 12 != 1024` per path) and DG fails; first run only DG caught it, so DE was tightened to assert EVERY boundary solved**
- [x] Probe CW re-run with motion on — **probe DL clause 2: width 0 + motion on + decorr 0.8 → decorrSamples 0**

**Dependencies:** Task 2, Task 3, Task 1
**Estimated effort:** Large

---

### Task 5: Host clock plumbing + SAFE / no-PPQ / stopped behaviour

**Outcome:** `PluginProcessor.cpp::processBlock` reads `getPlayHead()` once per block exactly as
`O-Orbit/PluginProcessor.cpp:561-574` (`getPosition()` → `getBpm()`, `getPpqPosition()`,
`getIsPlaying()`), packs a `HostClock`, and passes it to `gainStage.process()` (`:708-709`).
`processBlockBypassed()` (`:622`) is untouched (motion does not run through a bypassed plugin — nothing
renders). **No `isSafeMode()` gate**: the unmapped `renderChunk` branch never reads a position, so
motion is inaudible in SAFE mode by construction and the map still animates (RESEARCH Q3). The
harness gains `HarnessPlayHead : juce::AudioPlayHead` and `renderInto()` (`main.cpp:369`) advances
PPQ per block when a `bpm` is set (default: no playhead installed → every existing probe renders with
`ppqValid = false`).

**Verification:**
- [x] Full suite with `HarnessPlayHead` present but uninstalled — **63/63, PASS list diff vs baseline = only DC/DK added**
- [x] Probe **DJ** — **PASS: synced moving while rolling, holds after stop; Free keeps moving**
- [x] Probe **DH** — **PASS at PPQ 38.4 (= 14400 × 64), worst diff 0.000000000 m. 37.5 beats = 900000 samples is not a grid multiple: the counter-aligned grid samples the path 32 samples apart between the two renders (a designed 9 mm) — see Deviations**
- [x] SAFE-mode probe — **DM PASS: stereo fold, motion on vs off bit-identical, motionSolves 192 (branch ran)**

**Dependencies:** Task 4
**Estimated effort:** Medium

---

### Task 6: R4/R5 gates — determinism and block-size invariance

**Outcome:** Probes **DE** (Free mode, each of the six paths, `{64}` vs `{256}` vs `{1024}` vs
`{1, 7, 64, 333, 4096}` — `bitIdentical()`, `motionSolves > 0`, live), **DF** (same, Sync 1/4 @
120 BPM, playhead rolling), **DG** (two renders, Drift, seed 7, identical; seed 8 differs), plus the
DE-shaped probe with a **Free-mode rate-change event** mid-render (the re-base is the one place a
hidden accumulator could creep back in).

**Verification:**
- [x] Render harness exits 0; DE/DF/DG green — **DE 6 paths × 4 chops with a rate change, every boundary solved 1024/1024; DF 6 paths synced rolling; DG seed 7 twice identical, seed 8 differs**
- [x] **NC2** run once, restored (shasum identical) — **8 probes fail: DD, DI, DE, DF, DG, DH, DJ, DL**
- [x] CX with motion on — **probe DL clause 1: CX shape + Spiral 0.8 Hz, ragged vs 4096 bit-identical, chains clocked, motion ran**

**Dependencies:** Task 5
**Estimated effort:** Medium

---

### Task 7: C++ → JS: `getMotionTrace` native function + live offset on `getMeters`

**Outcome:** `PluginEditor.cpp`: native function **(27) `getMotionTrace`** — reads the six shape
parameters on the message thread, evaluates `MotionPath::evaluate` for N = 128 points over one cycle
(`cycles = i/128`), returns `{ cyclic, points: [[x, y, z]…] }` in **anchor-relative metres**; for Drift
returns `{ cyclic: false, points: [] }`. `getMeters` (`:1089`) gains `motion: [x, y, z]` from
`GainStage::liveOffset` and `motionOn: bool`. Native count literal in `ui_frontend_check.js:246` 26 → 27.
The stub (`tests/ui-stub/juce-stub.js`): `NATIVE_FNS.getMotionTrace` evaluates a JS **transcription
for the stub only** (marked as such — it is a fixture, not the page's generator) and `readMeters()`
(`:382`) emits a slowly orbiting `motion` triple so the puck moves headless.

**Verification:**
- [x] `ui_frontend_check.js` §3 native count = 27 — **PASS**
- [x] `grep -c "evaluate" Source/ui/public/js/*.js` = 0 — **0; §41 also asserts no trig/noise in `renderTrace()`**
- [ ] Manual: `Juce.getNativeFunction("getMotionTrace")()` in the WebView console returns 128 points for Orbit, 0 for Drift — **operator gate, pending** (the stub twin is held by layout §32: 129 rendered points for Orbit, 0 for Drift)

**Dependencies:** Task 3, Task 4
**Estimated effort:** Medium

---

### Task 8: WebView controls — layout measured first, relays before attachments

**Outcome:**
- **Step 0 (layout, stub only):** add `#group-motion` to `index.html` after `:366` with the 5-row body of the plan call; run `node plugins/O-Octagon/tests/ui_layout_check.js` — §8 (1100 × 720), §21 (elevation strip fits), §22 (`#group-elevation` last) must pass. If §21 fails, switch to the dense fallback and re-measure. **Do not touch C++ until this passes.**
- `PluginEditor.h:190-196`: add `std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays; std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;` **before** `webView`, and the two attachment vectors **after** `sliderAttachments`. `PluginEditor.cpp:274` loop: the float loop skips the three non-float ids by `params::Index` (`motionOn`, `motionPath`, `motionSync`) and builds the three typed relays; attachments at `:1476-1485` mirror the split.
- `app.js`: `bindToggle`/`bindCombo` using `Juce.getToggleState` / `Juce.getComboBoxState`; `FORMAT` (`:135`) +7 float entries (`motionRate` Hz dp 2, `motionSize` m dp 1, `motionAngle`/`motionPhase` ° dp 0, `motionSeed` dp 0); re-fetch trace on `valueChangedEvent` of the six shape params; Seed cell hidden unless Path = Drift.
- Stub: `getComboBoxState`/`getToggleState` (`juce-stub.js:520-525`) implemented; `RANGES` +7 floats; choice/bool tables added.
- `ui_frontend_check.js` §11 (`:491`) extended: `toggleRelays;`/`comboRelays;` before `webView;`, both attachment vectors after; §16 asserts `sliderRelays` covers exactly `kCount − 3` and names the three.
- i18n: EN + FR hover help for the 10 controls in `i18n.js` `TIP_BINDINGS` (`:451`).
- `styles.css`: motion group, `.cell-toggle`, `.cell-select`.

**Verification:**
- [x] `ui_layout_check.js` — **31/31 sections; §21 coarse 592 ≤ 592 at DPR 1 and 2; §22 elevation last. Step 0 measured the plan's group at 756 vs 592 → tabbed panel (see Deviations)**
- [x] `ui_frontend_check.js` — **43/43 sections (§11 typed relay order, §15 28 ranges + CHOICES diff, §16 kCount−3 split, §41 new)**
- [x] `check-i18n.js --plugin O-Octagon` — **ALL CHECKS PASS (59 I18N entries, 69 bindings)**
- [ ] Plugin builds with zero warnings — **yes**; Standalone / host-lane naming check — **operator gate, pending** (auval lists the parameters; pluginval fuzzed them)

**Dependencies:** Task 2, Task 7
**Estimated effort:** Large

---

### Task 9: Venue map — trace, ghost anchor, live puck, Drift tail; elevation strip live Z

**Outcome:** `roomplan.js`: a `<path id="motion-trace">` on `#plan-geometry`, built from
`getMotionTrace` points + the anchor's metres from the existing JS `normToMetres` twin (`:79`) through
`metresToPx` (`:62`) — translation is the only arithmetic. Existing `#puck` gets class `puck--ghost`
when `motionOn`; new `#puck-live` on `#plan-controls` placed from the polled `motion` triple each meters
tick (`meters.js` `onLevels` gains the motion payload). Drift: a 48-point JS ring drawn as a fading
polyline. `elevation.js` `drawMarker()` (`:283`) adds `motion[2]` to the source height when on.
Trace re-renders on `relayout()` and on venue change (`venueGen`).

**Verification:**
- [x] `ui_layout_check.js` §32 — **worst 1.412 px over 8 ticks, puck moved, trace 129 points, Size step re-renders (233 → 300 px), Drift: no trace + 8-point tail**
- [x] `ui_frontend_check.js` §41 — **PASS**
- [x] Stub page — **held by §32 (Size step, ghost class, Drift tail)**

**Dependencies:** Task 8
**Estimated effort:** Medium

---

### Task 10: Factory presets and docs

**Outcome:** Two new factory rows in `factoryDefs()` (`PresetPolicy.h:138`) that DO set motion —
requires the row struct to carry the ten motion fields with defaults (off) for the existing six rows:
`"Slow Orbit"` (Orbit, Sync 4 Bars, 8 m, ratio 0.8, height 1 m) and `"Wander"` (Drift, Free 0.05 Hz,
6 m, seed 7). `CHANGELOG.md` v1.8.0 entry in the v1.7.0 shape; `NOTES.md` dated entry with gate
counts; `parameter-spec.md` amendment (Task 2) cross-checked.

**Verification:**
- [x] CP and DK green; **DN: Chamber loaded after Wander → motion OFF, seed back to 1**
- [x] **DN: 8 factory names; Wander → Drift, Free 0.05 Hz, 6 m, seed 7; Slow Orbit → Orbit, 4 Bars, 8 m, ratio 0.8, height 1 m** (WebView load itself: operator)

**Dependencies:** Task 2, Task 4
**Estimated effort:** Small

---

### Task 11: Version bump, build, install, pluginval, auval

**Outcome:** `CMakeLists.txt:15` `VERSION 1.8.0`; installed and validated.

**Verification:**
- [x] `build-and-install.sh O-Octagon` — **exit 0, no ALTERNATE-variant warning; O-Octagon-dev.vst3 / .component installed**
- [x] `auval -a` shows `aufx OuOc OuDv O-Octagon-dev`; `auval -v aufx OuOc OuDv` — **PASS**
- [x] pluginval strictness 10 on `O-Octagon-dev.vst3` — **SUCCESS**
- [ ] Logic double-bounce `cmp` + different buffer size — **operator gate, pending** (offline twins: DG, DE/DF)

**Dependencies:** Tasks 6, 9, 10
**Estimated effort:** Small

---

### Task 12: Final gate sweep and commit

**Outcome:** Every gate named in this plan run in one sitting and recorded in NOTES.md; path-scoped commit.

**Verification:**
- [x] Render harness exit 0 — **73/73 = 61 + DC, DD, DE, DF, DG, DH, DI, DJ, DK, DL, DM, DN**
- [x] Geometry target exit 0 — **57/57**
- [x] `ui_frontend_check.js` 43/43, `ui_layout_check.js` 31/31, `check-i18n.js` pass — **all exit 0 in the final sweep**
- [ ] `git branch --show-current` = main (**confirmed**); commit **deferred to the orchestrator after verify** (not run by execute)

**Dependencies:** Task 11
**Estimated effort:** Small

---

## Dependency Graph

```
Task 1 (digest anchor) ──┬──> Task 2 (params) ──┬────────────────────────┐
                         │                      │                        │
Task 3 (headers+unit) ───┴──────────────────────┴──> Task 4 (shaper+grid)┤
                                                      │                  │
                                                      v                  v
                                              Task 5 (clock/SAFE)   Task 7 (native fn) ──> Task 8 (controls) ──> Task 9 (map)
                                                      │                                                              │
                                                      v                                                              │
                                              Task 6 (R4/R5 gates)   Task 10 (presets/docs)                          │
                                                      └──────────────────┴──────────────────────────────────────────┴──> Task 11 ──> Task 12
```

**Parallelizable:** Task 3 with Tasks 1–2; Task 7 with Task 5; Tasks 6, 8–9, 10 with each other after Task 5/7.

---

## Execution Order

| Order | Task | Dependencies | Parallelizable With |
|-------|------|--------------|---------------------|
| 1 | Task 1 digest anchor | — | Task 3 |
| 1 | Task 3 headers + unit | — | Task 1, 2 |
| 2 | Task 2 parameters | 1 | Task 3 |
| 3 | Task 4 shaper + grid branch | 1, 2, 3 | — |
| 4 | Task 5 clock / SAFE / stopped | 4 | Task 7 |
| 4 | Task 7 native fn + meters | 3, 4 | Task 5 |
| 5 | Task 6 R4/R5 gates | 5 | Task 8, 10 |
| 5 | Task 8 controls (layout first) | 2, 7 | Task 6, 10 |
| 5 | Task 10 presets/docs | 2, 4 | Task 6, 8 |
| 6 | Task 9 map | 8 | — |
| 7 | Task 11 bump/install/validate | 6, 9, 10 | — |
| 8 | Task 12 sweep + commit | 11 | — |

---

## Risk Notes

1. **Block-size invariance (R5) — highest risk.** Any surviving accumulator (rate re-base, Perlin
   time, PPQ extrapolation) makes the render buffer-size dependent.
   - Impact: bounces differ per host buffer; the plugin's fixed-media use case is broken.
   - Mitigation: Task 3 makes phase a pure function of absolute position with a unit probe on 1×N vs N/64; Task 6's DE/DF use ragged sizes `{1, 7, 64, 333, 4096}` and the negative control NC2 proves the probe targets the branch.
2. **`shape()` refactor moves the v1.7.0 digest.**
   - Impact: R3 fails silently if re-recorded.
   - Mitigation: Task 1 captures the digest before any edit; Task 4 falls back to a verbatim `shape()` beside `shapeAt`; never re-record.
3. **Controls column budget (1100 × 720, elevation strip flex:1 last child).**
   - Impact: §21 fires (699 vs 592 precedent), or the fix silently vacates §21 by inserting after `#group-elevation`.
   - Mitigation: Task 8 step 0 measures on the stub before C++; the dense fallback is pre-declared; group inserted after Position, never after Elevation.
4. **First non-float relays re-open the O-ReverseDelay relay-type bug.**
   - Mitigation: Task 8 extends §11/§16 to the new vectors; the stub implements both state getters (`pattern_juce_webview_backend_stub_direction`).
5. **Harness playhead changes existing probes.** Mitigation: Task 5 runs the full suite with the class present but not installed before any motion probe.
6. **Effective Z reaching only one of the two consumers.** Mitigation: Task 4 names both `solveSubPoint` sites (`:393-394`); DD's companion check compares the proximity cue with `srcZ = z` vs `srcZ = 0, height = z`.
7. **Tempo change mid-block.** Accepted: re-syncs at the next block start; documented in CHANGELOG.

---

## Domain Agent Instructions

**Execute Agent:** general-purpose

**Domain-specific rules to follow:**
- **RT safety:** nothing in `processBlock`/`process`/`updateControl` allocates, locks, or throws; the motion evaluation is `noexcept` and runs only at `phase == 0`; `PerlinNoise::seed()` is a bounded table fill and may run at the boundary.
- **Never integrate per call.** Phase = f(absoluteSampleCounter, HostClock). No `+= rate·n/sr` anywhere.
- **Motion off is a branch, not `+ 0.0f`.** The v1.7.0 call at `GainStage.cpp:369-371` stays verbatim on the off path; the dirty-check predicate at `:357-359` is the v1.7.0 predicate when off.
- **Relays before attachments** (`PluginEditor.h:170-196`): new relay vectors above `webView`, new attachment vectors below `sliderAttachments`; three-arg attachment ctor with `nullptr`.
- **Preset migration:** no range moved → no migration hook; if a range ever moves later, each param needs its own version gate (`pattern_preset_migration_per_param_version_gate`).
- **`juce::String(const char*)` is ASCII-only** — choice names, ids, and the FR tooltip copy go through the i18n JS table, never a C++ literal with accents.
- **Digest discipline:** `kV170Digest` is transcribed from the pre-edit run (Task 1) and never re-recorded from a failing build.
- **No JS path generator.** The page translates and projects (`metresToPx`); the stub's transcription is a fixture and must be labelled as such.
- **`#group-elevation` stays the controls column's last child.**
- **Commit path-scoped** (`git commit -- plugins/O-Octagon PLUGINS.md`), re-checking `git branch --show-current` and `git status --short` immediately before.

**Files the agent should read first:**
- `plugins/O-Octagon/.planning/improvements/motion-engine/CONTEXT.md`, `RESEARCH.md` — decisions and settled questions (this plan overrides only the preset partition)
- `plugins/O-Octagon/Source/DSP/GainStage.h:61-136, 219-226, 400-435` and `GainStage.cpp:297-400` — the grid, `updateControl`, the two Z sites
- `plugins/O-Octagon/Source/DSP/SourceShaper.h/.cpp` — the `normToMetres` seam at `.cpp:35`
- `plugins/O-Octagon/Source/DSP/DbapSolver.h:57-120` — `instr` counters
- `plugins/O-Octagon/Source/PluginProcessor.cpp:46-160, 305-320, 690-712` — parameter factory, snapshot sanitiser, `process()` call site
- `plugins/O-Octagon/Source/Data/PresetPolicy.h:63-96, 138-180` — partition and factory rows
- `plugins/O-Octagon/Source/PluginEditor.h:170-199`, `PluginEditor.cpp:262-300, 1078-1110, 1468-1500` — relay/attachment order, `getMeters`
- `plugins/O-Octagon/Source/ui/public/js/roomplan.js:25-120, 460-485`, `app.js:135-280, 1112-1139`, `elevation.js:283-300`
- `plugins/O-Octagon/tests/render-harness/main.cpp:337-470, 5710-5782, 6030-6075` — `setParam`, `renderInto`, `bufferDigest`, CU and CX shapes
- `plugins/O-Octagon/tests/ui_frontend_check.js:215-250, 491-520, 630-735`; `tests/ui_layout_check.js:1101-1200`
- `plugins/O-Orbit/Source/DSP/MotionEngine.h/.cpp`, `PerlinNoise.h`, `PluginProcessor.cpp:555-575` — vocabulary and playhead read (re-derive, do not copy `advance()`)

---

## Success Criteria

From CONTEXT.md, the improvement is successful when:

1. [ ] All six paths (D3) generate correct metric offsets and are audibly distinct (R1) — unit probes, Task 3
2. [ ] `srcX`/`srcY`/`srcZ` read back unchanged across a full cycle with motion running (R2) — probe DD
3. [ ] Render digest at `motionOn = 0` is **bit-identical** to the v1.7.0 binary (R3) — probe DC
4. [ ] Two offline bounces of the same session are identical, Drift included (R4/D7) — probe DG
5. [ ] Same session at 64 / 256 / 1024 samples renders the same (R5) — probes DE/DF
6. [ ] Venue map shows trace + ghost anchor + live puck, matching the audible path (R6) — Task 9 gate section
7. [ ] Motion state round-trips in presets; pre-1.8.0 presets load motion-off (R7) — probes CP/DK
8. [ ] Build succeeds without warnings
9. [ ] Pluginval passes (Level 5+, and strictness 10 on CI Windows — `pattern_ci_pluginval10_catches_latent_nan`)
10. [ ] `auval -a | grep -i octagon` passes after install
11. [ ] All tasks completed and verified

---

## Approval

```
Approve this plan?

1. Yes, proceed with execution
2. No, revise the plan
3. No, cancel milestone
4. Other

Choose (1-4): _
```

---

*Generated by improve-milestone plan phase*
