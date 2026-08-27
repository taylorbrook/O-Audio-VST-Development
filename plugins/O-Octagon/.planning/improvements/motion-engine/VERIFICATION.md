# Verification: Motion Engine — generative trajectories for the source puck

**Plugin:** O-Octagon
**Milestone:** motion-engine
**Created:** 2026-08-27
**Phase:** Verify

---

## Verification Summary

**Status:** ✅ VERIFIED (PASSED WITH NOTES — operator listening / Logic gates remain)
**Version:** 1.7.0 → 1.8.0

Every machine-checkable number in SUMMARY.md was **re-measured from a fresh run**, not copied.
All of them reproduce. No source file was modified by this phase; nothing was committed.

| Gate | SUMMARY.md claims | Re-measured (this phase) | Match |
|---|---|---|---|
| Render harness (`O-Octagon-render-test`) | 73 / 0 | **73 probe(s), 0 failure(s)** | ✓ |
| Geometry/unit target (`O-Octagon-geometry-test`) | 57 / 0 | **57 probe(s), 0 failure(s)** | ✓ |
| `node plugins/O-Octagon/tests/ui_frontend_check.js` | 43 / 43 | **ALL SECTIONS PASS — 43 sections**, exit 0 | ✓ |
| `node plugins/O-Octagon/tests/ui_layout_check.js` | 31 / 31 | **ALL SECTIONS PASS — 31 sections**, exit 0 | ✓ |
| `node scripts/check-i18n.js --plugin O-Octagon` | pass | **ALL CHECKS PASS — 1 localized plugin(s)** (canon v1), exit 0 | ✓ |
| Build warnings (`OuariconOctagon_VST3`, `_AU`, both test targets) | zero | **0 `warning:` lines on a CLEAN rebuild (130 ninja steps)** | ✓ |
| `./scripts/build-and-install.sh O-Octagon` | exit 0, no orphan | **exit 0**, no `⚠ Sweeping ALTERNATE-variant` line, VST3 + AU installed as `O-Octagon-dev.*` | ✓ |
| `auval -v aufx OuOc OuDv` | PASS | **`* * PASS`** | ✓ |
| pluginval strictness 10 on `~/Library/Audio/Plug-Ins/VST3/O-Octagon-dev.vst3` | SUCCESS | **SUCCESS**, exit 0 | ✓ |
| DC digest | `0xb8c5a2d0c7518204` | render log: `digest 0xb8c5a2d0c7518204 vs v1.7.0 0xb8c5a2d0c7518204; motionSolves 0` | ✓ |

Note on the ninja target names: SUMMARY/PLAN say "four build targets"; the actual ninja names are
`OuariconOctagon_VST3`, `OuariconOctagon_AU`, `O-Octagon-render-test`, `O-Octagon-geometry-test`
(`O-Octagon_VST3` does not exist — the juce_add_plugin target is `OuariconOctagon`, product
`O-Octagon-dev`). `build-and-install.sh` resolves this itself.

---

## Requirements Verification

Requirement-by-requirement, mapped to source evidence and to the probe that holds it.

| Req | Requirement (CONTEXT.md) | Source evidence | Probe / gate | Status |
|---|---|---|---|---|
| **R1** | Six paths, correct in venue metres, one phase accumulator; Figure-8 closes; Sweep fold has no saw wrap | `Source/DSP/MotionPath.h:104-178` — `evaluate()` cases `orbit` (:118), `figure8` (:124), `sweep` (:130, `fold()` :94), `drift` (:142), `pendulum` (:151), `spiral` (:157); all `z = height·sin t` (D4); rotation by `angleDeg` (:178) | Unit **MP1** figure8-closes (0,0)→(0,0); **MP2** sweep-fold max step 0.003906 m = limit; **MP3** orbit radius err 2.4e-7; **MP4** spiral half-cycle step 0.0023 m; **MP5** angle rotates; render **DE/DF** all six paths | ✓ SATISFIED (audible distinctness = operator) |
| **R2** | `srcX/srcY/srcZ` never written; host automation moves the anchor and the path follows | Offset applied in `GainStage.cpp:328-369` (`updateControl (snapshot, p, offset, motionOn)`), `zEff` at :439; no `setValueNotifyingHost` on position params | **DD** anchor-never-written: 3000/3000 blocks bit-unchanged, motionSolves 3128; **DI** anchor-step-moves-whole-path: offset series bit-identical with/without an srcX step, renders differ | ✓ SATISFIED |
| **R3** | Motion OFF bit-identical to v1.7.0 | `GainStage.cpp:413-419` dirty-check gains `&& ! motionOn`; :433-439 "else arm is the v1.7.0 call, character for character"; `SourceShaper.cpp:29-43` `shape()` delegates to `shapeAt()` | **DC** motion-off-matches-v1.7.0: digest `0xb8c5a2d0c7518204` == v1.7.0 constant, `motionSolves 0`; CU (v1.4.0 digest) still green; NC1 recorded in SUMMARY | ✓ SATISFIED |
| **R4** | Tempo sync + PPQ determinism; two bounces identical incl. Drift; mid-timeline start lands at correct phase | `MotionClock.h:121-150` `cyclesAt()` — PPQ extrapolated per boundary, no accumulator; `PluginProcessor.cpp:765-785` reads `getPlayHead()` once per block → `HostClock`; `PerlinNoise.h` seeded, pure | **DF** synced 6 paths bit-identical across block sizes; **DG** seed 7 twice bit-identical, seed 8 differs; **DH** start at ppq 38.4 vs tail-of-render-from-0: worst diff 0.000000000 m; **DJ** synced holds after stop, Free keeps moving; unit **MP7/MP8** | ✓ SATISFIED (DH at 38.4 not 37.5 — see Deviations) |
| **R5** | Block-size invariance 64/256/1024 | `GainStage.cpp:312-333` boundaries keyed to `absoluteSampleCounter`, `MotionClock.h:33-45` "NO ACCUMULATOR" | **DE** 6 paths × {64}/{256}/{1024}/ragged + mid-render rate change 1.0→1.7 Hz: all bit-identical, every boundary solved 1024/1024; **DF** synced; **DL** CX shape + Spiral ragged vs 4096 bit-identical; NC2 recorded in SUMMARY (8 failures with an accumulator) | ✓ SATISFIED |
| **R6** | Trace + ghost anchor + live puck, same generator (no JS reimplementation) | `PluginEditor.cpp:1142-1156` native fn 27 `getMotionTrace` evaluates `oo::motion::evaluate` for 128 pts; `:1133-1136` `getMeters` carries `motion` + `motionOn`; `roomplan.js:213-214, 270-283, 517-554` translate+project only; `GainStage.h:465-471` `liveOffset` atomics | Layout **§32**: group 113 px both panels, ghost drawn, trace 129 pts, puck ON trace (1.412 px worst), Drift tail; frontend **§41** no generator on the page; frontend **§3** native count 27 | ✓ SATISFIED |
| **R7** | Every motion control an APVTS param; presets round-trip; pre-1.8.0 presets load motion-off | `PluginProcessor.cpp:68-76` `makeBool`/`makeChoice`; `:174` `setSkewForCentre (0.3f)`; `:178-181` the ten ids; `PresetPolicy.h:89-94` `kAuthored` = 16 incl. ten motion ids, `static_assert (kPreserved + kAuthored == kCount)`; `:147-149` factory rows default motion off | **DK** v1.7.0-shaped 18-key preset: 18/18 bit-exact, motionOn OFF, 10/10 defaults; **DN** 8 factory names, Wander/Slow Orbit carry motion, Chamber after → OFF; CP still green | ✓ SATISFIED |

### CONTEXT.md success criteria

| # | Criterion | Result |
|---|---|---|
| 1 | Six paths correct + audibly distinct | Offsets ✓ (MP1–MP5, DE/DF). **Audible distinctness → operator** |
| 2 | Position params unchanged across a cycle | ✓ DD |
| 3 | Digest at motionOn=0 bit-identical to v1.7.0 | ✓ DC (`0xb8c5a2d0c7518204`) |
| 4 | Two bounces identical, Drift included | ✓ DG (offline). **Logic bounce twins → operator** |
| 5 | Same at 64/256/1024 | ✓ DE, DF, DL |
| 6 | Map trace + ghost + live puck | ✓ layout §32, frontend §41 (stub). **Live WebView eyeball → operator** |
| 7 | Preset round-trip; pre-1.8.0 loads motion-off | ✓ DK, DN |
| 8 | Build without warnings | ✓ 0 warnings, clean rebuild, 130 steps |
| 9 | Pluginval level 5+ / strictness 10 | ✓ strictness 10 SUCCESS locally (CI Windows = on publish) |
| 10 | `auval` passes after install | ✓ `* * PASS` |

---

## Version bump / files

- `plugins/O-Octagon/CMakeLists.txt:15` — `VERSION 1.8.0` ✓
- `plugins/O-Octagon/CHANGELOG.md:3` — `## v1.8.0 (2026-08-27)` ✓
- `plugins/O-Octagon/.planning/parameter-spec.md:160, 212` — v1.8.0 amendment, `motion` group, 28 params ✓
- `plugins/O-Octagon/NOTES.md:376-377` — v1.8.0 bullet with gate counts ✓
- `git diff --stat v1.7.0-O-Octagon -- plugins/O-Octagon/Source` → **17 files changed, 1115 insertions(+), 34 deletions(-)** (non-empty; the three new headers are untracked and therefore additional) ✓
- New files present: `Source/DSP/MotionPath.h` (7450 B), `Source/DSP/MotionClock.h` (7225 B), `Source/DSP/PerlinNoise.h` (3128 B) ✓
- `#include` lines in the three: `PerlinNoise.h`, `Vec.h`, `<cmath>`, `<cstdint>`, `<cstring>` only — **no JUCE include**. (A case-insensitive grep hits the three "No JUCE" doc comments; SUMMARY's "0 files" was a case-sensitive `grep -l juce`, which is 0. Both statements are true.) ✓
- No hyphenated filenames added under `Source/` ✓
- Latest tag is still `v1.7.0-O-Octagon`; `v1.8.0-O-Octagon` is for the orchestrator's commit step.

---

## Real-time safety spot-check (motion branch)

Grep of `GainStage.cpp`, `SourceShaper.cpp`, `MotionPath.h`, `MotionClock.h`, `PerlinNoise.h` for
`new`, `std::vector`, `juce::String`, `std::string`, `lock`, `mutex`, `malloc`, `std::function`:
**only comments and pre-existing v1.7.0 text match** — no code hit in the motion path.

- `MotionPath.h:104` `evaluate()` is `noexcept`, stateless; `MotionClock.h:121` `cyclesAt()` `noexcept`, no accumulator.
- `PerlinNoise.h:88` storage is `uint8_t perm[512]` — fixed array; `seed()` is a bounded table fill.
- `GainStage.cpp:344-350` reseeds **only when the seed changes**, at a control boundary.
- `GainStage.h:465-471` members `PerlinNoise perlin`, `motion::MotionClockState motionClock`, `std::atomic<float> liveOffset[3]` — the UI reads the offset lock-free.
- `PluginProcessor.cpp:765-785` reads `getPlayHead()` once per block into a stack `HostClock`; no `isSafeMode()` gate (DM proves SAFE-mode inaudibility).

**WebView member order** (`PluginEditor.h:190-208`): `sliderRelays`, `toggleRelays` (:197), `comboRelays` (:198) → `webView` → `sliderAttachments` (:204), `toggleAttachments` (:207), `comboAttachments` (:208). Frontend §11 measures the same by character offset (relays 10479/10939/11014 < webView 11093 < attachments 11194/11365/11459) ✓

---

## SUMMARY.md deviations — confirmed in code

1. **Tabbed Position | Motion panel, not a seventh group.** `index.html:365-370` `group-tabs` role=tablist with `gtab-position` / `gtab-motion`; `:372` `#panel-position`; `:412` `#panel-motion` (`group-body--dense`, `hidden`). No `#group-motion` exists. Layout §32 measures Position 113 == Motion 113 px, column not overflowing ✓
2. **DE asserts every boundary solved.** `tests/render-harness/main.cpp:6765-6773` compares `motionSolves` against `4 * (total / 64)`; log line reads "every boundary solved (1024/1024)" ✓
3. **DH at PPQ 38.4.** log: "start at ppq 38.4 vs tail of a render from 0" ✓ — accepted design consequence of the counter-aligned grid, documented in CHANGELOG "Compatibility".
4. **MP2 bound `<= 2R/2048 · 1.001`.** `tests/unit/main.cpp:3621`; log "max step 0.003906 m (limit 0.003906)" ✓
5. Extra probes DL/DM/DN present and green ✓

---

## Build Verification

```bash
cd build && ninja -t clean OuariconOctagon_VST3 OuariconOctagon_AU O-Octagon-render-test O-Octagon-geometry-test
ninja OuariconOctagon_VST3 OuariconOctagon_AU O-Octagon-render-test O-Octagon-geometry-test
```
**Result:** Success (exit 0), 130 steps · **Warnings:** none (0 `warning:` lines)

### Installation
`./scripts/build-and-install.sh O-Octagon` → exit 0 in 23 s; cache cleared by the script;
**VST3 installed:** ✓ `~/Library/Audio/Plug-Ins/VST3/O-Octagon-dev.vst3` ·
**AU installed:** ✓ `~/Library/Audio/Plug-Ins/Components/O-Octagon-dev.component` · no alternate-variant orphan.

## Pluginval / auval
- `auval -v aufx OuOc OuDv` → **PASS**
- `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-Octagon-dev.vst3` → **SUCCESS** (level 10 ⊃ level 5)

---

## Regression Check

**Baseline:** v1.7.0 (tag `v1.7.0-O-Octagon`).
- All 61 pre-existing render probes green inside the 73 (CU v1.4.0 digest, CP twelve-preserved, CX/CW, CY/CZ/DA/DB monitor fold).
- All 49 pre-existing geometry probes green inside the 57.
- Pre-existing layout sections 1–31 and frontend sections 1–40 green.
- **Old presets load:** ✓ (DK) · **Values preserved:** ✓ 18/18 bit-exact.

---

## Issues Found

### Critical (blocking)
None.

### Non-critical (noted)
1. **`ui_frontend_check.js` has two headings numbered "section 41"** (log lines 177 and 462: the new "no path generator in the page" and the pre-existing "rake line drawn ONLY bbMinY → bbMaxY"). The total still reports 43 sections and both pass; cosmetic numbering collision only. Severity: Low. Recommendation: fix later.
2. `build-and-install.sh` builds VST3 + AU only — the Standalone `.app` is stale (known repo pattern). Not a motion-engine issue; matters only for the operator's Standalone eyeball gate below.
3. The installed bundles came from the earlier incremental build; the clean rebuild used for the warning count is the same source tree (no edits between), so the artefacts are equivalent.

---

## Gates left to the operator

- **Logic bounce twins:** motion on a track, bounce twice → `cmp` byte-identical; bounce again at a different I/O buffer size → identical. (Offline twins already green: DG, DE/DF, DH.)
- **Standalone / host lane:** `motionOn` lane reads On/Off, `motionPath` reads path names; in the WebView console `Juce.getNativeFunction("getMotionTrace")()` returns 128 points for Orbit, 0 for Drift; eyeball trace + ghost + live puck in a real WKWebView (stub twin: layout §32). Rebuild Standalone first (`ninja OuariconOctagon_Standalone`).
- **Listening:** hall / headphone (MONITOR fold) pass over the six paths — R1's "audibly distinct".
- **CI Windows pluginval strictness 10** runs on publish.

---

## Verification Checklist

- [x] All requirements from CONTEXT.md verified (R1–R7; R1 audibility and R6 live-host eyeball are operator gates)
- [x] Build succeeds without errors or warnings (clean rebuild)
- [x] Pluginval passes strictness 10; auval PASS
- [x] UI gates: frontend 43/43, layout 31/31, i18n pass
- [x] Audio: render 73/0, unit 57/0
- [x] No regressions (all pre-existing probes/sections green; DC digest unmoved)
- [x] Preset compatibility confirmed (DK)

---

## Final Status

### PASSED WITH NOTES ⚠ → ✅ VERIFIED for commit

All machine-checkable requirements re-measured and reproduced. Known non-blocking note: duplicate
"section 41" heading in `ui_frontend_check.js`.

**Next steps:**
1. Path-scoped commit: `git commit -- plugins/O-Octagon` (re-check `git branch --show-current` and `git status --short` immediately before; another session has O-Tapestop/O-Bitrot/O-MultiBandCompressor files dirty)
2. Tag `v1.8.0-O-Octagon`
3. Update the O-Octagon row in `PLUGINS.md`, clear `activeMilestone`
4. Operator gates above (Logic bounce twins, Standalone eyeball, listening)

---

*Generated by improve-milestone verify phase*
