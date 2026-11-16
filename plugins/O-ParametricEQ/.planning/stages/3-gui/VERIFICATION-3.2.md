# Stage 3: GUI — Phase 3.2 (interactive response display) - Verification

## Verification Date

2026-11-16 (verify session). Everything below was run against `47d767d3` (0.8.0). Before the run, `git status` showed no changes under `plugins/O-ParametricEQ`.

What the verify session did:

- **Independent harness re-run.**
  - The harness objects were deleted and recompiled (`verify-3.2/run-harness.sh`).
  - Then: the designer harness, the render harness default, and the render harness `--long`. Tallies are in `verify-3.2/harness-tallies.txt`.
- **Independent no-audio-change check.**
  - The session took the 16 XR-lid sessions dumped by the 0.8.0 harness and compared their SHA-256 hashes with the 0.5.0 hashes from verify 2.6 (`verify-3.2/sessions-080.sha`).
  - XR-lid then ran against that dump.
- **Independent G9 (b).**
  - The session made its own `--dump-g9` dump (260 draws) and ran `tests/g9b/g9b.mjs` on the shipping page (`verify-3.2/g9b-output.txt`).
  - It also read the runner's lift code, its stubs and its controls.
- **Code read:**
  - the full `c77d014d..47d767d3` diff of `PluginEditor.h/.cpp`, `index.html`, `i18n.js` and `i18n-states.json`;
  - `DisplayModel.h` (its threading and polling contract);
  - `createBand`, `deleteBand`, the wheel and double-click handlers, `setLabel` / `applyLabel`.
- **Independent gates:**
  - all five UI gates;
  - the native-function diff, using this session's own grep;
  - the static greps;
  - `diff -rq` of the installed bundles against the artefacts.
- **Validators**, run on the installed bundles after the bench work: pluginval strictness 10 (VST3, AU) and `auval -v aufx OuPQ OuDv`.
- **Mutation run** (`verify-3.2/mutants32.py`, `mutants32-results.json`):
  - 20 one-line mutants, none of them among execute's seven: 5 in the editor, 15 in the page. 16 are scored against a named row; 4 are probes (is there any row for this at all?).
  - Each ran through a scratch copy of the execute-3.2 editor bench, with the screen unlocked and under `caffeinate`.
  - The unmodified control ran first and last: ALL-PASS both times.
  - One extra gate mutant ran through `check-ui-labels` on a scratch root.
- **Execute's committed bench JSON re-scored:** `verdict32.py` over `results/rel-1…5.json` and `dbg.json` gives ALL-PASS for all 6, and `dbg.log` has 0 assertions.

Nothing in the repo source changed. Scratch builds, logs and the mutable source copy are in the session scratchpad.

Scope: Phase 3.2. Stage 3 stays open (3.3).

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 3.2; PLAN-3.2 Goal)

1. **True curve.** The editor pushes `eqSections` from its own `DisplayModel`. The page draws the sum from the packets. G9 (b) holds it to the C++ within 0.01 dB on all 260 draws, mono included.
2. **Live with the transport stopped.** A host edit with no `processBlock` reaches the painted curve within 2 ticks.
3. **Nodes.** Create, drag, wheel Q, double-click reset, delete. Each action opens one gesture pair per parameter it writes, and none on a parameter it cannot write.
4. **Strip.** Typed entry lands exactly on every range end. A double-click reset lands on the default.
5. **U5.** The 25th-band click shows a keyed tip at the pointer, in three languages.
6. Traffic only on change; a full push on request. No Debug assertion with events live.
7. No audio change. 0.8.0 builds, installs and validates.

### Deliverables (from SUMMARY-3.2.md, confirmed in the tree)

1. **Editor.**
   - `displayModel` is declared after the attachments, so it is destroyed first.
   - `fullPushPending = true`.
   - `requestEqSections` sets the flag.
   - `pageReady` starts the timer at 60 Hz.
   - `timerCallback`: `poll()`, then all 24 bands if the flag is set, else the changed bands, else nothing. One emit per tick, carrying `{ sampleRate, stereo, rateKnown, bands }` with `band` 1-based.
   - The bench hook exists only under `OPEQ_EDITOR_BENCH`.
2. **Page.**
   - P3: `composeSum` composes the `run` list (packet `active`, packet placement) and reads no relay. `sumView` uses the packet placement. `draw()` passes `running` (or `[]` under global bypass).
   - P5: `d.gain` is decided once at the threshold, and the begin / write / end all depend on it.
   - P7: `placeTip` refactor, `showBandsFullTip`, `clearPointTip`, the re-place observer, `.tip-point`.
   - `parseEntry` accepts exponents.
3. **`i18n.js`:** `bands-full` in en, fr (`reviewed: false`) and zh-Hans (`'mt'`).
4. **Tests:**
   - `--dump-g9` in the render harness;
   - `tests/g9b/` with the 0.7.0 `composeSum` fixture;
   - i18n state 5, appended last.
5. **Bench and mutants:** `execute-3.2/editor-bench/`, `execute-3.2/mutants/`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 True curve (UI-02) | ✅ Achieved | **G9 (b), this session's own dump:** worst **1.372e-8 dB** over **148 956** points (gate 0.01).<br>• Per rate: 44.1 k 5.16e-9, 48 k 8.30e-10, 88.2 k 7.56e-10, 96 k 3.50e-10, 192 k 1.37e-8. Mono set 875 points, 0 NaN.<br>• All guards pass. All 7 controls fire: k × 1.01 at 19.7 dB; onePoles at 172.9; L / M order at 238.0; Side-as-Mid at 236.6; `active` removed at 256.8; 5a identical (0 dB over 2035 points); 5b 0.7.0 `composeSum` at 59.5 dB.<br>• The runner lifts the three functions by name from the shipping file, exactly once each. Its relay stub throws if `composeSum` reads anything but placement, and the C++ side comes from the dump.<br>• Painted curve: the execute bench probes (2075 / 2075 columns over 250 preset loads, E-stopped, E-mono) re-score ALL-PASS.<br>• Verify mutant V5 (header `stereo` forced true) is caught by E-mono. |
| 2 Transport stopped | ✅ Achieved | E-stopped: execute measured 0.1 – 14.2 ms to the emit (bound 37.3).<br>• V4 (`changed` ignored, only full pushes) is caught by E-stopped, E-mono, E-eq-init and E-lag.<br>• `DisplayModel::poll` reads the parameter atomics, not anything `processBlock` writes (code read). |
| 3 Nodes, gestures (UI-01) | ✅ Achieved in-process; host recording by hand | E-create, E-drag, E-wheel, E-dblclick and E-delete pass in all 6 committed runs. These verify mutants are caught:<br>• V9: gain gesture ignores scale → E-drag.<br>• V10: ignores type → E-drag.<br>• V11: gain end skipped → E-drag.<br>• V16: Q not reset on create → E-create.<br>• V17: create guard removed from double-click → E-dblclick.<br>• V18: a gesture per wheel event → E-wheel.<br>• V19: delete selects nothing → E-delete.<br>Two survivors, V15 / V20: see Issue 2. **Recording in Logic and in a VST3 host: Taylor (ROADMAP criterion 4).** |
| 4 Strip ends, resets (UI-04, DSP-06) | ✅ Achieved | E-ends 20 / 20 exact (`get() ==`). V14 (the exponent parse reverted) is caught by E-ends. E-dblclick 7 / 7, and execute's E15 is caught. |
| 5 U5 tip | ✅ Achieved (look by hand) | E-full 10 / 10, and V12 (tip gated by hover help) is caught.<br>• `check-ui-labels` state 5 measures `bands-full` in fr and zh-Hans: it reports an fr overlap note on the tip, so the state is not vacuous.<br>• `i18n-zh-lint` counts 52 `mt` entries (51 + `bands-full`).<br>• The re-place observer is held by code only: Issue 3. |
| 6 Traffic, Debug | ✅ Achieved | E-idle / E-eq-idle: 0 emits over 10 s.<br>• V3 (flag never cleared) is caught by E-idle and E-eq-idle.<br>• V2 (`band` 0-based) is caught by E-eq-init (both rows), E-stopped and E-lag.<br>• E-eq-reload catches execute's `fullPushPending` mutant.<br>• The Debug run has 0 assertions and 0 leak lines. |
| 7 No audio change, validates | ✅ Achieved | **16 / 16 XR-lid sessions SHA-256-equal to 0.5.0** (session's own dump).<br>• Designer 150 / 0 / 18.<br>• Render 166 / 0 default and 166 / 0 `--long`.<br>• `git diff c77d014d 47d767d3 -- Source/PluginProcessor.* Source/dsp Source/analysis`: empty.<br>• Installed `-dev` bundles are `diff -rq` identical to the artefacts: 0.8.0, built 23:17, after every source edit; no unsuffixed orphan.<br>• pluginval 10: VST3 `SUCCESS`, AU `SUCCESS` (1 warning line, the stock AU one).<br>• `auval`: `AU VALIDATION SUCCEEDED`. |

## Requirements Verification

**Stage:** stage-3. This phase closes the criteria of UI-01, UI-02, UI-04 and DSP-06's last criterion. FUNC-08, UI-03, UI-06 and UI-07 are 3.3.

| Requirement | Priority | Status | Acceptance criteria |
|-------------|----------|--------|---------------------|
| UI-01 Interactive display | must | ✅ Complete | **Create:** E-create, freq ≤ 1e-4 relative, gain to 0.1. **Drag:** one gesture pair per written parameter; Low Cut and 0 % scale open none on gain. **Scroll Q:** one pair per burst. Gestures are proven in-process (`parameterGestureChanged`). The Logic / VST3 lane recording is a hands-on row. |
| UI-02 True curve (≤ 0.1 dB vs measured, every rate) | must | ✅ Complete | The chain of three: G9 (a) engine vs C++ closed form 2.03e-3 dB (Stage 2, re-run here in render 166 / 0); G9 (b) C++ vs page 1.37e-8 dB at all five rates and mono; the page's painted ink vs C++ at the probed columns. Total ≤ 2.1e-3 dB, inside 0.1. |
| UI-04 Strip, typed entry, double-click reset | should | ✅ Complete | E-ends 20 / 20 exact; E-dblclick 7 / 7 (VERIFICATION-3.1 Issue 1 closed). |
| DSP-06 Parameter ranges | must | ✅ Complete | "All ranges reachable from UI, automation and typed entry": typed entry E-ends; UI drag clamps to the live range (`setScaled`); automation by normalised 0 / 1 (harness rows). The second criterion was met in Stage 2. |
| UI-03, UI-06, UI-07, FUNC-08 | — | ⏸️ Pending (3.3) | |

**Requirements Summary:** Complete 4 (UI-01, UI-02, UI-04, DSP-06), Partial 0, Pending (3.3) 4, Failed 0.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness rebuild | ✅ | `build rc 0` after the objects were deleted |
| Designer | ✅ 150 / 0 / 18 | |
| Render default / `--long` | ✅ 166 / 0, 166 / 0 | XR-lid on this session's dump |
| Audio vs 0.5.0 | ✅ 16 / 16 | `verify-3.2/sessions-080.sha` |
| G9 (b) | ✅ PASS | 1.372e-8 dB, 7 / 7 controls, all guards |
| UI gates ×5 | ✅ all 0 | `check-i18n` (49 plugins), `i18n-fr-lint`, `i18n-zh-lint` (52 `mt`), `check-ui-labels` (state 5 measured), `boot-all-uis --strict-tips`: 0 DEAD, 0 late, 0 warn, 0 failed |
| Native functions | ✅ 19 / 19 | identical, page + `preset-manager.js` vs `PluginEditor.cpp` |
| Relay reads in `composeSum` / `sumView` | ✅ 0 / 0 | |
| `OPEQ_EDITOR_BENCH` | ✅ | 0 hits in `build.ninja`; only the editor files reference it |
| Module JS untracked | ✅ | |
| `bands-full` English literal in `index.html` | ✅ none | |
| Install | ✅ | identical, 0.8.0, `-dev` only |
| pluginval 10 VST3 / AU | ✅ SUCCESS / SUCCESS | |
| auval | ✅ | |
| Committed bench JSON re-scored | ✅ 6 / 6 ALL-PASS | |
| Verify mutation run | ✅ 13 / 16 caught, controls ALL-PASS ×2 | survivors and probes below |

### Mutation run (verify)

| Mutant | Target | Result |
|---|---|---|
| V1 timer at 30 Hz | E-stopped / E-lag | **survived** (Issue 1) |
| V2 `band` sent 0-based | E-eq-init | caught: E-eq-init ×2, E-stopped, E-lag |
| V3 `fullPushPending` never cleared | E-eq-idle | caught: E-idle, E-eq-idle |
| V4 `changed` ignored | E-stopped | caught: E-stopped, E-mono, E-eq-init ×2, E-lag |
| V5 header `stereo` forced true | E-mono | caught |
| V9 `d.gain` ignores gain scale | E-drag | caught |
| V10 `d.gain` ignores type | E-drag | caught |
| V11 gain gesture end skipped | E-drag | caught |
| V12 U5 tip gated by hover help | E-full | caught |
| V14 exponent parse reverted | E-ends | caught |
| **V15** create keeps the old placement | E-create | **survived** (Issue 2) |
| V16 create keeps the old Q | E-create | caught |
| V17 double-click create guard removed | E-dblclick | caught |
| V18 a gesture per wheel event | E-wheel | caught |
| V19 delete selects nothing | E-delete | caught |
| **V20** create does not clear bypass | E-create | **survived** (Issue 2) |
| *probe* V6 `active` filter dropped | — | no bench row. **Held by G9 (b) control 6** (256.8 dB), so covered |
| *probe* V7 global bypass ignored (sum not flat) | — | no row (Issue 4) |
| *probe* V8 `sumView` always LL | — | no row (Issue 4) |
| *probe* V13 hover tip keeps the point tip's binding | — | no row (Issue 3) |
| *gate* removing the point-tip re-place observer | `check-ui-labels` | **survived**: rc 0. The gate sees the fr geometry change (new overlap notes) but classes the tip as a floating layer (Issue 3). Scratch-root control rc 0 |

Note: the first scoring pass reported V2 as a survivor because the row names carry a suffix ("E-eq-init (every open)"). The script was fixed and the results re-scored from the recorded verdicts. No run was repeated.

## Human Verification

For Taylor, in Logic (AU) and one VST3 host. These come from SUMMARY-3.2's carry-forward, plus 3.1's open items.

- [x] Touch mode: a Bell node drag records the `freq` and `gain` lanes together, and one ⌘Z undoes it.
- [x] A Low Cut drag records only `freq`. A Bell at gain scale 0 % records only `freq`.
- [x] A wheel burst on a node records one Q stretch.
- [x] Click-to-create in Latch writes the new band.
- [x] The curve follows automation with the transport stopped.
- [x] Mono track, bands set Left / Right: the curve is the Stereo one, and the badges still show L / R.
- [x] Click with 24 bands in use: the tip in en / fr / zh-Hans, near the bottom-right corner too.
- [x] Curve and node look against the mockup at 800 × 500 and 1600 × 1000.
- [x] From 3.1, still open:
  - opens at 960 × 600 in Logic and in a VST3 host;
  - the timing-flag log, 5 cold opens, ≤ 500 ms;
  - a DAW preset round trip (output gain and bypass untouched);
  - undo history after a preset load;
  - language switch at 800 × 500;
  - plate look;
  - the `withKeepPageLoadedWhenBrowserIsHidden` call.

**Result, 2026-11-16:** Taylor reports all items pass in the DAW. The `withKeepPageLoadedWhenBrowserIsHidden` call stays as built (keep, the recommendation); no change was asked for.

## Issues Found

1. **The 60 Hz rate is not gated (minor; 3.3).**
   - V1 (`startTimerHz (30)`) passes E-stopped (bound 2 ticks + 4 ms = 37.3 ms) and E-lag (bound 50 ms) at one rep.
   - The bounds allow a 30 Hz timer. The code is right.
   - Recommendation: when 3.3 adds its 60 fps row, measure the median tick interval of `eqSections` during a drag, at ≤ 20 ms.
2. **E-create cannot see placement or bypass (minor; 3.3 or Stage 4 bench edit).**
   - The row moves Q and slope off their defaults before the click, but leaves type (Bell), placement (Stereo) and bypass (off) at their defaults. Its `placement == 0`, `type == 0` and `! bypass` checks are therefore vacuous, and V15 and V20 survive.
   - The code resets all three (`createBand`, read).
   - Recommendation: before the click, seed band 1 as High Shelf / Side / bypassed. Real use hits this whenever a deleted band that had been set up is re-created.
3. **Point-tip housekeeping is held by code only (minor; Stage 4 i18n review).**
   - (a) Removing the re-place observer (execute deviation 3) survives `check-ui-labels`. The gate measures the tip after a live language switch: the fr geometry changes, with new overlaps on `knob-gain_scale` and the output knob cell, but [7] / [8] treat the tip as an opaque floating panel.
   - (b) V13: a hover tip shown while a point tip was up keeps the `bands-full` binding. No row checks this.
   - Both need a language change during the tip's 1.6 s window to show at all.
   - Recommendation: one bench row. Show the tip, switch to fr, and require its rect to equal a fresh placement in fr. Then hover a control and require `tooltipEl.dataset.i18n` to be unset.
4. **Two curve choices have no row (minor; 3.3).**
   - V7: the global bypass flat line.
   - V8: the view follows the selected band's placement (LL / RR / MM / SS), which matters on mixed L / R / M / S.
   - Both are 3.1-era behaviour that 3.2 rewrote, and both are correct on reading.
   - Recommendation: two pixel-probe rows in the 3.3 bench (bypass on → the curve is flat at 0 dB; select a Side band with Mid and Side bands present → the ink = the SS composition).
5. **Cosmetic:** `hideTooltip` removes `.tip-point` at the start of the 110 ms fade. The fading text goes from 11.5 px to the body's 12 px for those frames. This is too small to see in practice; noted for the Stage 4 polish pass.

Carried forward unchanged (no new findings):
- Stage 4: fr / zh-Hans review of `bands-full`; host text units; the E16 / R2 preset rows.
- 3.3: `draw()` takes 9 – 10 ms on a 192-section draw (per-band cache), and listen hooks onto drag start / end.

No blocker. Nothing failed.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 3.2). Every automated criterion in ROADMAP 3.2 is met and was reproduced independently. The Logic / VST3 automation recording (ROADMAP criterion 4) and the other hands-on rows were confirmed by Taylor on 2026-11-16: all pass.

**Ready for next phase:** Yes. Phase 3.3 (analyzer, meter, fills, listen). Its bench should take in the rows from Issues 1, 2 and 4.

**Blockers:** none.
