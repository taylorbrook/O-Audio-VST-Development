# Execute Summary: motion-engine (O-Octagon 1.7.0 → 1.8.0)

**Date:** 2026-08-27 · **Branch:** main (trunk-based, no worktree) · **Commit:** deferred to the
orchestrator after verify — nothing was committed by execute.

---

## Tasks completed — verification results

| # | Task | Result |
|---|------|--------|
| 1 | R3 digest anchor | **DC** added in CU's shape. `kV170Digest = 0xb8c5a2d0c7518204ull`, captured at commit `2e03020e` with `git diff --stat v1.7.0-O-Octagon -- plugins/O-Octagon/Source` **empty** (checked before each of the three capture runs). Baseline before any edit: render 61/0, unit 49/0. Two consecutive runs of DC passed (62/62). |
| 3 | `MotionPath.h`, `MotionClock.h`, `PerlinNoise.h` + unit probes | `grep -l juce` on the three → 0 files; no hyphenated filename. **MP1–MP8** added → geometry target **57/57** (was 49). Zero warnings (two `-Wfloat-equal` hits fixed with bitwise compares). |
| 2 | Parameters (kCount 18 → 28), partition, spec | `makeBool` / `makeChoice` helpers; `motionRate` = `setSkewForCentre (0.3f)`; `kAuthored` 6 → 16; `parameter-spec.md` v1.8.0 amendment. **DK** added (v1.7.0-shaped 18-key preset → motionOn OFF, 18/18 bit-exact, 10/10 motion at default). Render **63/63** with DC's digest unmoved at 28 params; CP still "the TWELVE BIT-UNCHANGED". |
| 4 | `shapeAt()` + motion branch in `updateControl()` | `shape()` now delegates to `shapeAt()` — **DC's digest did not move**, so the verbatim-`shape()` fallback was not needed. Effective Z reaches both `solveSubPoint` calls. `instr::motionSolves` added. **DD** (3000/3000 blocks anchor unchanged; srcZ 2 ≡ srcZ 0 + Height 2 at sin t = 1 bit-identical) and **DI** (offset series bit-identical with/without an srcX step) green. |
| 5 | Host clock + harness playhead | `processBlock` reads `getPlayHead()` once per block → `motion::HostClock` → `gainStage.process (…, &clock)`; no `isSafeMode()` gate. `HarnessPlayHead` derives PPQ from elapsed samples (never accumulates). **Risk 7 check:** full suite with the class present but uninstalled = 63/63, PASS-list diff vs baseline is only DC/DK. **DJ**, **DH**, **DM** (SAFE) green. |
| 6 | R4/R5 gates | **DE** (6 paths × {64}/{256}/{1024}/ragged, Free-mode rate change mid-render, every boundary solved 1024/1024), **DF** (synced rolling), **DG** (Drift seed 7 twice identical, seed 8 differs), **DL** (CX + CW re-run with motion on) green. |
| 7 | `getMotionTrace` (27th native fn) + live offset on `getMeters` | `PluginEditor.cpp` evaluates `oo::motion::evaluate` for 128 points; `getMeters` carries `motion: [x,y,z]` + `motionOn`. Stub fixture labelled "TRANSCRIPTION FOR THE STUB ONLY". Frontend §3 = 27. `grep -c evaluate Source/ui/public/js/*.js` = 0. |
| 8 | Controls (layout measured first) | **Step 0 result: the plan's layout does not fit** (see Deviations). Tabbed Position \| Motion panel instead. Typed relays/attachments in the mandated order; `bindToggle` / `bindCombo`; FORMAT +7; EN+FR help for 12 keys. `ui_layout_check` **31/31** (§21 coarse 592 ≤ 592 at DPR 1 and 2, §22 elevation last), `ui_frontend_check` **43/43**, `check-i18n` **pass**. |
| 9 | Map: trace, ghost, live puck, Drift tail, elevation Z | New layout **§32**: live puck within **1.412 px** of the rendered trace over 8 ticks, moved, 129 points; Size step re-renders 233 → 300 px; Drift = no trace + 8-point tail; ghost/hidden restore on off. New frontend **§41**: no generator on the page. |
| 10 | Factory presets + docs | "Slow Orbit" and "Wander" rows (row struct gained a defaulted `Motion` field). **DN** green: 8 names, Wander → Drift/Free/0.05 Hz/6 m/seed 7, Slow Orbit → Orbit/4 Bars/8 m/0.8/1 m, Chamber after → OFF. CHANGELOG v1.8.0, NOTES bullet. |
| 11 | Bump, install, validate | `CMakeLists.txt` VERSION 1.8.0. `build-and-install.sh O-Octagon` exit 0, no ALTERNATE-variant orphan; `auval -v aufx OuOc OuDv` **PASS**; pluginval strictness 10 on `O-Octagon-dev.vst3` **SUCCESS**. |
| 12 | Final sweep | Render **73/0**, unit **57/0**, frontend **43/43**, layout **31/31**, i18n **pass**, all four build targets **zero `warning:` lines**, branch `main`. Commit deferred. |

Render harness: 61 → **73** (DC, DD, DE, DF, DG, DH, DI, DJ, DK, DL, DM, DN). Geometry: 49 → **57**.

## Negative controls (each applied temporarily, restored, restore verified by shasum)

- **NC1** — delete `&& ! motionOn` from the dirty check: first run only **DG** failed ("seed 8 IDENTICAL — seed inert"); DE passed vacuously because its two events forced a solve each and `motionSolves > 0` held on a frozen offset. **DE was tightened** to `motionSolves == 4 × total/64` (every boundary). Re-run: **DE fails** (`motionSolves 12 != 1024` on all six paths) **and DG fails**; restore → 72/0.
- **NC2** — replace `cyclesAt` with a per-block accumulator `+= rate · numSamples / sr`: **8 failures** (DD, DI, DE, DF, DG, DH, DJ, DL); restore → 72/0.

## Files changed (all under `plugins/O-Octagon/`)

New: `Source/DSP/MotionPath.h`, `Source/DSP/MotionClock.h`, `Source/DSP/PerlinNoise.h`, `.planning/improvements/motion-engine/SUMMARY.md`.
Modified: `CMakeLists.txt`, `CHANGELOG.md`, `NOTES.md`, `.planning/parameter-spec.md`, `.planning/improvements/motion-engine/{PLAN.md,STATUS.yaml}`, `Source/DSP/{DbapSolver.h,GainStage.h,GainStage.cpp,SourceShaper.h,SourceShaper.cpp}`, `Source/Data/PresetPolicy.h`, `Source/{PluginProcessor.h,PluginProcessor.cpp,PluginEditor.h,PluginEditor.cpp}`, `Source/ui/public/{index.html,css/styles.css,js/app.js,js/roomplan.js,js/elevation.js,js/meters.js,js/i18n.js}`, `tests/render-harness/main.cpp`, `tests/unit/main.cpp`, `tests/ui-stub/juce-stub.js`, `tests/ui_frontend_check.js`, `tests/ui_layout_check.js`.
Not touched: `PLUGINS.md`, anything outside `plugins/O-Octagon/`.

## Deviations from PLAN.md (with reasons)

1. **Layout — no seventh group fits; Motion is a tabbed panel of the Position group.** Step 0 measured the plan's `#group-motion` (5 rows) at column scrollHeight **756 vs 592**. The column had 13 px in hand (`.controls-column`'s v1.5.0 note) and the elevation stage is `flex: none` at a fixed 125, so the pre-declared dense 4-row fallback (~143 px) could not fit either. Chosen: a `Position | Motion` tab pair in the group's title row (Scenes' STORE precedent, ~4 px) and a 3-column dense Motion panel of **nine** visible cells = 3 rows = Position's 3 rows; Seed replaces Phase only while Path is Drift. Measured: group 113 px on both panels, column 592 ≤ 592 at DPR 1 and 2. `#group-elevation` stays last. Layout §32 holds the equal-height premise. Plan text referring to `#group-motion` maps to `#panel-motion`.
2. **DH uses PPQ 38.4, not 37.5.** 37.5 beats = 900000 samples, not a multiple of 64; the control grid is aligned to the absolute sample counter by design (`GainStage.h`), so a bounce from 0 and a bounce from 37.5 evaluate the path at instants 32 samples apart — a real, designed 9.2 mm the 5 ms ramps absorb, not a phase error. 38.4 = 14400 × 64 puts both renders' boundaries on the same instants; worst diff 0.000000000 m.
3. **DC's scenario adds an `srcZ` event at 4096·2+1000.** CU's exact scenario at ragged sizes digests to CU's own constant (QUAL-03 makes the chop invisible), which would have made DC a second copy of one number. The added event also moves both Z consumers under motion-off.
4. **DE asserts every boundary solved, not `> 0`** — found by NC1 (above).
5. **MP2's bound is `<= 2R/2048 · 1.001`.** The plan's `< 2R/2048` is exactly the ideal per-sample speed of a linear sweep (4R per cycle / 4096); the fold hits it exactly, a saw wrap would show 2R.
6. **Task 2's `ui_frontend_check` literal update** was made and the expected failures recorded (6 FAILs) rather than left to Task 8.
7. **Extra probes beyond the plan's list:** DL (CX/CW with motion on), DM (SAFE mode), DN (factory presets) — the plan named these checks without probe letters.
8. **`HostClock` carries `playing` separately from `ppqValid`** (O-Orbit conflates them): a stopped host that still supplies a PPQ holds at that PPQ (constant, where playback resumes) with no held-state needed; the held value covers only a host that stops supplying PPQ.

## Gates left to the operator

- **Logic:** motion on a track, bounce twice → `cmp` byte-identical; bounce at a different buffer size → identical. (Offline twins in the harness: DG, DE/DF.)
- **Standalone / host lane:** `motionOn` reads On/Off, `motionPath` reads path names; `Juce.getNativeFunction("getMotionTrace")()` in the WebView console returns 128 points for Orbit, 0 for Drift (stub twin held by layout §32).
- Hall / headphone listening of the six paths (R1's "audibly distinct").

## Known issues / notes

- A concurrent session is working on O-Tapestop in this checkout: `git status` showed `PLUGINS.md` and `plugins/O-Tapestop/**` modified mid-run and `PLUGINS.md` clean again at the end — **none of it is this session's work** (PLUGINS.md was not edited here). The orchestrator's commit must be path-scoped to `plugins/O-Octagon` (plus `PLUGINS.md` only after its own row is updated) and must re-check `git status --short` immediately before committing.
- `.planning/STATUS.md` was already modified at session start (pre-existing).
- The plan's literal `grep -c "evaluate" Source/ui/public/js/*.js` reads 4, all in **pre-existing v1.7.0 comments** (Playwright's `page.evaluate` API name in app.js, "inert data, evaluated once" in i18n.js ×2, "has evaluated" in app.js). No page module calls `evaluate(`; frontend §41 asserts that on comment-stripped code and `renderTrace()` carries no trigonometry or noise.
- The stub's `motionRate` skew is JUCE's derived 0.2644 (asserted to 5e-4); the page never reads it (readouts go through `getScaledValue()`).
- Accepted per plan: a tempo change mid-block re-syncs at the next block start (CHANGELOG).

## Final `git status --short` (at the end of execute)

```
 M .claude/agent-memory/gui-agent.md            (pre-existing)
 M .claude/commands/publish.md                  (pre-existing)
 M .claude/frontmatter-issues.txt               (pre-existing)
 M .claude/resource-index.json                  (pre-existing)
 M .claude/skills/plugin-publishing/SKILL.md    (pre-existing)
 M plugins/O-Octagon/.planning/STATUS.md        (pre-existing)
 M plugins/O-Octagon/.planning/parameter-spec.md
 M plugins/O-Octagon/CHANGELOG.md
 M plugins/O-Octagon/CMakeLists.txt
 M plugins/O-Octagon/NOTES.md
 M plugins/O-Octagon/Source/DSP/DbapSolver.h
 M plugins/O-Octagon/Source/DSP/GainStage.cpp
 M plugins/O-Octagon/Source/DSP/GainStage.h
 M plugins/O-Octagon/Source/DSP/SourceShaper.cpp
 M plugins/O-Octagon/Source/DSP/SourceShaper.h
 M plugins/O-Octagon/Source/Data/PresetPolicy.h
 M plugins/O-Octagon/Source/PluginEditor.cpp
 M plugins/O-Octagon/Source/PluginEditor.h
 M plugins/O-Octagon/Source/PluginProcessor.cpp
 M plugins/O-Octagon/Source/PluginProcessor.h
 M plugins/O-Octagon/Source/ui/public/css/styles.css
 M plugins/O-Octagon/Source/ui/public/index.html
 M plugins/O-Octagon/Source/ui/public/js/app.js
 M plugins/O-Octagon/Source/ui/public/js/elevation.js
 M plugins/O-Octagon/Source/ui/public/js/i18n.js
 M plugins/O-Octagon/Source/ui/public/js/meters.js
 M plugins/O-Octagon/Source/ui/public/js/roomplan.js
 M plugins/O-Octagon/tests/render-harness/main.cpp
 M plugins/O-Octagon/tests/ui-stub/juce-stub.js
 M plugins/O-Octagon/tests/ui_frontend_check.js
 M plugins/O-Octagon/tests/ui_layout_check.js
 M plugins/O-Octagon/tests/unit/main.cpp
 M plugins/O-Tapestop/...  (7 files — other session)
?? .gsd/                                        (pre-existing)
?? plugins/O-Octagon/.planning/improvements/
?? plugins/O-Octagon/Source/DSP/MotionClock.h
?? plugins/O-Octagon/Source/DSP/MotionPath.h
?? plugins/O-Octagon/Source/DSP/PerlinNoise.h
?? plugins/O-Tapestop/...  (4 files — other session)
```
