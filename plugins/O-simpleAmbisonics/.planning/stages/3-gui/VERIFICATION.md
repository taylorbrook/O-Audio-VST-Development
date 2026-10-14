# Stage 3: GUI - Verification

## Verification Date

2026-10-14

## Method

Single-cycle stage (3.1 + 3.2 → visual checkpoint approved 2026-10-13 → 3.3). Every SUMMARY claim re-run independently from a forced rebuild (`PluginEditor.cpp` + `PluginProcessor.cpp` touched) on the **installed** bundles (VST3 + AU `cmp`-identical to the build after `build-and-install.sh`). Code invariants from PLAN §Success criteria re-inspected against `git diff 87841228 HEAD`.

## Goal-Backward Analysis

### Original Goals (CONTEXT.md §Requirements Confirmed + ROADMAP §Stage 3)

1. **UI-01** — B-format channel wall: 4 / 9 / 16 rows by `orderActive`, banded by order, SN3D icons, signed bars + peak overlay; front → left swaps X → Y with W constant.
2. **UI-02** — source on a sphere: top-down + side views, draggable puck (one gesture per drag, clean ±180 wrap), yaw turns the compass not the puck.
3. **UI-03 / UI-04 / UI-05** — polar beam sharpening 1 → 3; signal-path stations with live channel counts; decoder view following `decoderMix`.
4. **UI-06 / UI-08** — pedagogical tooltips on every control; 8 live lesson buttons that never touch `outputLevel`.
5. **UI-07** — single page, signal-path order, resizable 0.6–1.5× at fixed aspect, `editorScale` + `uiLanguage` persisted.
6. **i18n** — en / fr / zh-Hans, glossary first, all lint / width gates green on the real tree.
7. **No regression** — no `processBlock` / engine change; both Stage-2 digests stay verdicts; PERF-01 and pluginval hold with the WebView editor.

### Deliverables (SUMMARY.md + code inspection)

1. `index.html` B-format wall + `js/shicons.js` (16 generated SN3D icons, `3d` style kept at checkpoint); `MeterFramePayload.h` quantised 30 Hz payload.
2. Top-down / side canvases with pointer drags through slider relay gestures, heading rose from R·x̂, pole hold.
3. Beam overlay, five stations, KU100 / cardioid crossfade.
4. 35 tooltips; `applyFactoryPreset` (8 recipes, `outputLevel` skipped in reset and recipe, begin / set / end per changed param).
5. Scale pattern with `editorScale` (display-clamped, remove-on-copy) and `uiLanguage` in state XML.
6. Glossary section (own commit `bf3e12d9`), 149 fr / zh entries, 16 copy width fixes + layout pins, `i18n-states.json` (17 states).
7. `processBlock`, `prepareToPlay`, `Source/dsp/*` untouched (diff hunks only around `createEditor` and state get / set; the one hunk headed `processBlock` is the `createEditor` guard below it).

### Goal Achievement

| Goal | Status | Evidence (re-run 2026-10-14) |
|------|--------|-------------------------------|
| 1. B-format wall | ✅ Achieved | harness PL2 (left: Y 1000 / X 0); frame-probe A1 (W / X / Y 0.997 / 0.997 / 0), A2, A3 (bands 2/3/4, 16 icons); neg-control `fuma` FAILS A1 as designed; icons `--check` 0 |
| 2. Sphere + drags | ✅ Achieved | frame-probe A4–A7, A9 (A6 1/1 gesture, 0 rear samples, 90.00; A7 45.00); neg-control `pole` FAILS A9 as designed |
| 3. Beam / stations / decoder | ✅ Achieved | A8 (109.5° → 68.5° → 49.5°), A11, A10 (16.33 % decoder pixels) |
| 4. Tooltips + lessons | ✅ Achieved (page→C++ hands-on pending) | boot-all-uis `--strict-tips` 0 DEAD / 0 late; tooltip-clamp-probe ALL PASS; harness ST6; lesson-row-budget natives arm ALL PASS |
| 5. Layout / scale / persistence | ✅ Achieved | ST3 / ST4; `verify_3_state.py` S1–S3 PASS on installed VST3; visual checkpoint approved by user |
| 6. i18n | ✅ Achieved | check-i18n / fr-lint / zh-lint / check-ui-labels (seed=overrides) all exit 0 |
| 7. No regression | ✅ Achieved | harness 251/251 ×2, digests 2.1 `9189d911a1fe98d5` + 2.2 `7f35bc9f91e321b9` PASS, 2.3 `edd1acfd21f54bcd` reproduced; pluginval s10 SUCCESS ×2 VST3 + ×2 AU; `verify_2_2` / `verify_2_3` exit 0 |

## Requirements Verification

**Stage:** stage-3
**Requirements for this stage:** 8 total (3 must, 4 should, 1 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: B-format channel view | must | ✅ Complete | 4/9/16 rows (A3, PL2 bands); front → left X → Y, W constant (PL2 + A1) |
| UI-02: Source-on-a-sphere | must | ✅ Complete | drag → az / el with one gesture each (A6 / A7); yaw turns compass not puck (A5). Real-host gesture recording carried to the human list |
| UI-03: Polar plot | should | ✅ Complete | A8 narrows 1 → 3 |
| UI-04: Signal-path diagram | should | ✅ Complete | A11 station highlight + 4/9/16 labels |
| UI-05: Decoder view | nice | ✅ Complete | A10 crossfade on `decoderMix` |
| UI-06: Tooltips | should | ✅ Complete | strict-tips 0 dead / 0 late; clamp 1180×780 + 708×468; all CONTEXT topics present (SUMMARY T11) |
| UI-07: Layout | must | ✅ Complete | single page, signal-path order, 1180×780 design frame, scale 0.6–1.5 (ST3 / ST4, S1–S3); visual checkpoint approved |
| UI-08: Preset tour | should | ✅ Complete | ST6 (84 changes / 8 lessons, `outputLevel` raw unchanged, gestures balanced); natives arm 8 ids; fr slack 74 px |

**Requirements Summary:**
- ✅ Complete: 8
- ⚠️ Partial: 0
- ⏸️ Deferred: 0
- ❌ Failed: 0

Re-gated carries: PERF-01 (harness PERF-01 0 allocations, `processBlock` unchanged), COMPAT-01 (pluginval s10 now constructs the WebView editor — SUCCESS ×4).

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3, AU, Standalone, render-test) | ✅ Pass | forced rebuild, 0 warnings |
| Harness ×2 | ✅ Pass | 251/251, logs identical except PERF-02 (204× / 200×) |
| Digests | ✅ Pass | 2.1 + 2.2 verdicts PASS, 2.3 reproduced |
| Install | ✅ Pass | VST3 + AU `cmp`-identical to build |
| auval `aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval s10 ×2 VST3 + ×2 AU | ✅ Pass | SUCCESS ×4 |
| `verify_2_2.py` / `verify_2_3.py` | ✅ Pass | exit 0 |
| `verify_2_1.py` | ⚠️ 22/25 | 3 V3 Binaural checks stale since 2.3 (see Issues) — not a Stage-3 regression |
| `verify_3_state.py` | ✅ Pass | S1–S3 |
| check-i18n / i18n-fr-lint / i18n-zh-lint | ✅ Pass | exit 0; 149 zh at `'mt'` counted |
| check-ui-labels | ✅ Pass | `seed=overrides`, ALL CHECKS PASSED |
| boot-all-uis `--strict-tips` | ✅ Pass | clean 1/1, 0 DEAD, 0 late, 0 i18n diagnostics |
| frame-probe | ✅ Pass | 13/13; negative controls fuma / pole / idle each FAIL as designed |
| lesson-row-budget | ✅ Pass | ALL PASS |
| tooltip-clamp-probe | ✅ Pass | ALL PASS |
| `generate_sh_icons.mjs --check` | ✅ Pass | no drift |
| Code invariants | ✅ Pass | `setSize (1180, 780)` last numeric; `kStaleTicksIdle = 3`; 0 `'mousedown'`; one `juce_add_binary_data`; `outputLevel` skipped in `applyFactoryPreset` |

## Human Verification (non-blocking; carried to Stage 4 hands-on with COMPAT-02)

- [ ] Logic R1: stopped non-armed track — well dims, puck / bars / beam follow params; record whether Logic suspends `processBlock`
- [ ] Logic R2: rear-crossing drag recorded as automation — record whether playback sweeps through the front
- [ ] Logic R3: CPU editor open (static) vs closed
- [ ] Logic: automation moves UI; one drag = one gesture; close mid-drag ends it; hide / re-show resumes meters; save / reopen keeps size + language; 10× close / reopen
- [ ] Standalone: Tab / arrows; release outside window; double-click default; hover-help persists; fr / zh relabel; Reduce Motion; **each lesson moves exactly its recipe, never Output** (page → C++ not drivable headless); Overhead at order 3 doesn't clip
- [ ] D16 reflow by eye: "1.00 m · in front", orbit line, tilt line, band-tag column, lesson row
- [ ] Optional: fr copy review (re-run lesson-row-budget + check-ui-labels after)
- [ ] R9: long host blocks (> ≈ 100 ms) flipping to idle — report if seen

## Issues Found

- **`verify_2_1.py` V3 Binaural arm (3 checks) is stale since Phase 2.3.** It asserts a single-sample 0.75 impulse at index 100, valid only for the 2.1 Binaural placeholder; the real MagLS convolver puts the onset at 192 with HRTF smear. `verify_2_3.py` already asserts the real Binaural onset / latency (exit 0). The Stage-2 roll-up's "25 / 35 / 42 all passing" was true for `verify_2_1` only at the 2.1 verify. **Recommendation:** narrow V3 to Stereo (keep its 3 Stereo checks) with a header note pointing Binaural to `verify_2_3` V-onset. Needs the user's say-so (Stage-2 verdict file). Non-blocking.
- **Known nits (user-visible, not fixed):** en `label.encReadN` "(3+1)² = 16 channels" ellipsised by 8.0 px at order 3 (candidate: "= {ch} ch"); chevron / puck proximity when azimuth ≈ yaw. Both seen and accepted at the checkpoint — Stage 4 polish candidates.
- fr `reviewed: false` and zh `'mt'` — the ship bar (`'bt'` / native review) is Stage 4.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers:** none. Carried to Stage 4: the human hands-on list above (with COMPAT-02 Logic / Ableton), the `verify_2_1` V3 decision, the two nits, i18n review flags, FUNC-08 preset bank, Windows build.
