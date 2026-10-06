# Stage 3: GUI - Verification

## Verification Date

2026-10-06

## Method

Goal-backward against CONTEXT.md and the PLAN.md success criteria S1–S15. Every automated claim in SUMMARY.md was **re-run independently** in this verify session, at HEAD `f4eea85a` with the plugin tree clean.

- **Build:**
  - The installed `-dev` VST3 and AU are byte-identical (`diff -rq`) to the Release artefacts.
  - `strings | grep -c cycleUpdate` is 4 on both the installed VST3 and the Standalone.
  - `nm -gU | grep ForTesting` is 0 on both.
  - The Standalone's mtime is later than the VST3 artefact's.
- **Offline gates:** a fresh out-of-repo Debug tree (R-DBG) with 0 plugin warnings. All 6 drivers and `dsp-check --alloc-check` were re-run.
- **UI gates:** all five were run on the real tree.
- **Host checks** on the installed binaries:
  - targeted `auval`
  - pluginval VST3 and AU at strictness 10, with the editor tests on
- **New independent check: binary null test.** `git archive 850df9b9` (the final Stage 2 code) was built out of tree as a Release VST3. A new pedalboard probe (`uv run --python 3.12 --with pedalboard`) rendered 6 cases through it and through the **installed** Stage 3 VST3, each binary in its own process.
- **New independent check: bridge cross-check.** This does not reuse the execute-phase S-diffs. It compares:
  - `ParamIDs::all` with the editor relays and attachments and with the page's binding tables
  - `withNativeFunction` names with the page's `getNativeFunction` calls
  - the page's `data-preset` ids with the `applyFactoryPreset` ids
- **Code review:** the post-execute critic review (UI, architecture, and DSP for the audio-thread changes only; read-only). See Issues Found.

## Goal-Backward Analysis

### Original Goals (CONTEXT.md)

1. **3.1 Layout and Bindings:**
   - mockup v1 integrated byte-identical
   - all 21 params bound two-way: 13 slider, 6 combo and 2 toggle relays
   - all 8 natives registered
   - fixed 1120 × 780
2. **3.2 Visualization Panels:**
   - the bank stack with its highlight and marker, the quantized current cycle, and harmonics 1–32 with the band-limit ceiling
   - pushed at 30 Hz from the message thread
   - zero audio-thread cost
3. **3.3 Import UX, Tooltips, i18n:**
   - import from the button and from a drop, showing the filename and frame count
   - the empty-Imported state
   - localized errors
   - plain-language tooltips in en, fr and zh-Hans
   - all five UI gates green
4. **Lesson buttons:** wired live, and they never touch `output_level`.
5. **Cadence and venue:**
   - 3.1 + 3.2, then a visual checkpoint, then 3.3
   - a Standalone hands-on and a Logic pass
6. **Regression:** the Stage 2 DSP is untouched.

### Deliverables (SUMMARY.md + code)

1. **The editor** (`PluginEditor.{h,cpp}`):
   - 21 relays, then `webView`, then 21 attachments
   - 8 `withNativeFunction` calls
   - `setSize (1120, 780)` and `setResizable (false, false)`
2. **Viz rendering:** `CycleView.{h,cpp}` (`CycleRenderer`, FFT order 11) and `VizPayload.h` (an FNV-1a hash of the quantized payload).
3. **Viz push:** `kTimerHz = 30`, with `bankUpdate`, then `importStatus`, then `cycleUpdate`.
4. **Processor additions:** `applyFactoryPreset`, `importFromBase64`, the display atomics, and `WtVoice::lastNote`.
5. **The page:** `index.html` and `i18n.js` (142 zh-Hans entries at `mt`), `tests/i18n-states.json`, and `tests/viz-check`.
6. **Human checkpoints:** Task 12, Task 17 and Task 18 were all signed off by Taylor on 2026-10-06.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 3.1 Layout + bindings | ✅ Achieved | The bridge cross-check gives 21/21 bound: 15 `data-param` knobs (13 slider + the stepped `bit_depth` and `lfo_div`), 2 `TOGGLE_IDS`, 3 `SEGMENT_COMBO_IDS`, and the `bank` select. 8/8 natives match by name. Fixed size confirmed. boot-all-uis shows 0 DEAD. |
| 3.2 Viz panels | ✅ Achieved | viz-check ALL PASS on the re-run: G-VIZ-EXACT, CEIL, SILENT, NOTE, IMPORTED and ALLOC (a)/(b), with N1–N4 and SILENT-NC firing. `kTimerHz = 30`. |
| 3.3 Import / tooltips / i18n | ✅ Achieved | viz-check G-DROP, G-DROP-N1 and G-IMPORT-ERR PASS. All five UI gates green (below). Tasks 17 and 18 hands-on passed. |
| Lesson buttons | ✅ Achieved | G-LESSON PASS, with its NC firing. 5/5 `data-preset` ids match the C++ recipe ids. The apply loop skips `output_level`. |
| Cadence / venue | ✅ Achieved | Task 12 signed off before Part 2. Standalone and Logic hands-on recorded in SUMMARY and STATUS. |
| Stage 2 untouched | ✅ Achieved | **Binary null test, 6/6 bit-exact** against the out-of-tree Stage 2 build. R-GOLD values are identical to the Stage 2 reference table. `WtVoice.h` and `PluginProcessor.cpp` have additions only (+5/−0 and +217/−0 since `850df9b9`). |

## Success Criteria (PLAN S1–S15)

| # | Status | Evidence (this session unless noted) |
|---|--------|---------------------------------------|
| S1 Controls ↔ params, automation → UI | ✅ | Bridge cross-check 21/21; boot-all-uis 0 DEAD; Task 17 and Logic automation (Task 18, Taylor) |
| S2 Bridge complete | ✅ | 8/8 natives; 5/5 lesson ids; check-ui-labels "every requested resource was served"; Logic hide/re-show (Task 18) |
| S3 UI-01 | ✅ | G-VIZ-EXACT, SILENT, NOTE and IMPORTED PASS; Task 12 |
| S4 UI-02 | ✅ | G-VIZ-EXACT cycle within 2e-4, including the 3-bit rows; N1 fires; Task 12 |
| S5 UI-03 | ✅ | G-VIZ-EXACT within 0.02 dB of an independent DFT; G-VIZ-CEIL; N2–N4 fire; Task 12 |
| S6 PERF-03 | ✅ | ALLOC (a) 0 audio-thread allocations, (b) 0 when idle; the idle-quiet hash; `kTimerHz = 30`; `--alloc-check` 0 |
| S7 Lessons | ✅ | G-LESSON + NC; id match |
| S8 UI-04 | ✅ | G-DROP (a–d), G-DROP-N1, G-IMPORT-ERR and G-VIZ-IMPORTED; check-ui-labels across 8 states; Tasks 17 and 18 (including save/reopen with Imported) |
| S9 UI-05 | ✅ | `--strict-tips` 0 DEAD and 0 late; check-i18n 0 FAIL; fr CLEAN; zh 142 entries, 0 findings (named) |
| S10 UI-06 | ✅ | check-ui-labels ALL CHECKS PASSED, including the [7] pins; 1120 × 780 non-resizable; Task 17 |
| S11 Stage 2 untouched | ✅ | Null test 6/6 bit-exact; R-GOLD; auval; pluginval s10 VST3 and AU with the editor; 0 `ForTesting` symbols |
| S12 Hosts accept it | ✅ | Logic pass (Task 18, Taylor): save/reopen with Imported, editor lifecycle, reload |
| S13 Taylor signed off | ✅ | Tasks 12, 17 and 18 |
| S14 Scoped commits | ✅ | `f53925c4` and `f4eea85a` touch only `plugins/O-simpleWavetable/**`, plus 1 PLUGINS.md row in Part 2. `modules/` is absent from the HEAD tree (0 files). No tag. |
| S15 Contracts consistent | ✅ | ARCHITECTURE Stage 3 Amendments 14–18 are present (Part 2 diff +10 lines) |

## Requirements Verification

**Stage:** stage-3
**Requirements for this stage:** 7 total (4 must, 3 should)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: Bank panel, frame highlight, Position marker | must | ✅ Complete | S3 |
| UI-02: Current quantized cycle panel | must | ✅ Complete | S4 |
| UI-03: Harmonics 1–32 incl. band-limit level | must | ✅ Complete | S5 |
| UI-04: Import button with filename + frame count | must | ✅ Complete | S8 |
| UI-05: Plain-language localized tooltips | should | ✅ Complete | S9. zh-Hans is at `reviewed: 'mt'`; a native review is a ship-bar item, not a Stage 3 gate. |
| UI-06: Projector-readable single page, suite-consistent | should | ✅ Complete | S10 |
| PERF-03: ≥ 30 fps panels, no audio-thread stall | should | ✅ Complete | S6 |

**Requirements Summary:**
- ✅ Complete: 7
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 0 for this stage. FUNC-08, PERF-02, COMPAT-02 and QUAL-04 remain at Stage 4.
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Debug build (R-DBG, fresh tree) | ✅ Pass | exit 0, 0 `plugins/O-simpleWavetable` warnings |
| bank / dsp / mod / import / viz | ✅ Pass | each exits 0 and ends with `<x>-check: ALL PASS` |
| state-check | ✅ Pass | 11 PASS, 0 FAIL |
| `dsp-check --alloc-check` | ✅ Pass | G-ALLOC 0 audio-thread allocations |
| JUCE assertions | ✅ Pass | 0 in every log |
| R-GOLD | ✅ Pass | G-PITCH 0.0009 c, G-Q2-C8 −114.3 dB, G-Q2-SWEEP −74.9 dB, G-Q2-PULSE −67.0 dB, G-POLY stolen −88.5 dB, G-RETRIG 0.00856/0.00856, G-VEL −11.905 dB, G-BLOCK bit-identical, G-CLICK 1.414/1.144, G-STEAL 1.076×, G-SWITCH-TAIL 0.656×, G-RETRIG-VEL 0.589×. All equal the Stage 2 reference. |
| **Binary null test** (new) | ✅ Pass | Installed Stage 3 VST3 vs out-of-tree `850df9b9` Release VST3 at 48 kHz, 3.5 s stereo, max abs diff 0.0 in all 6 cases (see below). The Stage 3 binary rendered twice gives bit-identical output (deterministic). Control: different cases differ by up to 0.43, so the comparison can fail. |
| check-i18n | ✅ Pass | exit 0, 44 PASS, 0 FAIL, "ALL CHECKS PASS" |
| i18n-fr-lint | ✅ Pass | CLEAN, 179 entries, 0 findings |
| i18n-zh-lint | ✅ Pass | O-simpleWavetable named, 142 entries, 0 findings (all at `mt`, counted, not failed) |
| check-ui-labels | ✅ Pass | exit 0; 8 states; no page error; every resource served; "ALL CHECKS PASSED" |
| boot-all-uis `--strict-tips` | ✅ Pass | exit 0; the O-simpleWavetable row is 1120 × 780; 0 warnings, 0 DEAD, 0 late |
| auval (`aumu OSiW OuDv`) | ✅ Pass | AU VALIDATION SUCCEEDED; only the 2 known benign skew-default warnings |
| pluginval VST3, strictness 10 | ✅ Pass | exit 0, SUCCESS, 0 FAILED, editor tests on |
| pluginval AU, strictness 10 | ✅ Pass | exit 0, SUCCESS, 0 FAILED, editor tests on |

The 6 null-test cases:
- the default 4-note chord
- Sine→Saw at Position 1 with Band-limit Off, a C6–C7 run
- the LFO at full depth
- 3-bit with Interp Off
- Mono legato
- a 17-note steal

## Human Verification

Recorded during execute (SUMMARY.md §Human checkpoints) and not repeated here:

- [x] Task 12 visual checkpoint, on the Release Standalone (Taylor, 2026-10-06)
- [x] Task 17 Standalone hands-on: keyboard, import, drop, refusals, empty state, language switch, Reduce Motion (Taylor, 2026-10-06)
- [x] Task 18 Logic AU pass: automation → UI, hide/re-show, save/reopen with Imported, reload (Taylor, 2026-10-06)
- [ ] Optional, carried over from Stage 2 Task 23 by ear: the 17-note steal and a fast Mono retrigger

## Issues Found

Critic review (UI + architecture + DSP-on-touched-code): **0 blockers, 5 warnings, 13 notes.** I re-read W1 and W5 in the code and confirmed both. The critic re-read W1–W3. **Taylor's decision (2026-10-06): VERIFIED, with W1–W5 and N1–N13 carried into Stage 4 as entry items.** None of them affects a stage-3 acceptance criterion.

| ID | Location | Defect → Stage 4 fix |
|----|----------|----------------------|
| W1 | `PluginEditor.cpp:230-235` | The `uiReady` handler reads the gen, index and import-version counters **after** it emits. If an import completes in that window, its `done` status is swallowed: the page stays `importBusy` and Import and drop are disabled until a reload. → Read the counters first, then emit (as `timerCallback` does). |
| W2 | `index.html:1144` + `PluginProcessor.cpp:1121-1126` | The 96 MB drop cap: the base64 string (about 128M chars) is decoded synchronously into an unsized `MemoryBlock`, which freezes the message thread for seconds. The importer keeps only 524,288 samples. → Measure first, then lower the cap, pre-size the block, or decode on the `importPool`. |
| W3 | `index.html:1484`, `:2358` | The stepped-knob and bank-stack wheel step at least once per event, so a trackpad flick jumps `bit_depth` from Full to 3. → Accumulate px and step every `WHEEL_PX_PER_NUDGE`. |
| W4 | `index.html:2250/2263` + `uiReady` order | A stale `tooShort` error reappears on a built-in bank after the editor reopens. → Reset the import status to idle when the selection leaves Imported. |
| W5 | `index.html:2563-2582`, `PluginEditor.cpp:360-363` | A note held from the UI keyboard sticks if the host closes the editor mid-hold: no keyup arrives, `blur` does not fire on teardown, and the destructor only calls `stopTimer()`. → Track UI-held notes in `handleUiMidi`, and release them in the editor destructor. |

Notes N1–N13, in brief:
- **N1:** the FileChooser is not a member (a double click opens two dialogs).
- **N2:** the display atomics can be torn for one tick.
- **N3:** `getBankThumbnails` reads `importedOwner` under the lock (allowed by Amendment 18).
- **N4:** `releaseResources` does not clear the display state.
- **N5:** a pending auto-select can override a lesson.
- **N6:** a lesson clicked during Mono playback hard-stops the voice (2 ms tail, by design).
- **N7:** `sanitiseName` keeps C1 and bidi characters.
- **N8:** two gesture owners on `position`.
- **N9:** a newer-format session shows "import failed" (needs an `unsupported` key).
- **N10:** the lesson highlight goes stale.
- **N11:** `#octDown` and `#octUp` have no tooltip.
- **N12:** `fmtDb` uses ASCII `-inf`.
- **N13:** one automation gesture per step on stepped knobs.

Full text is in the critic report summarised above.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes (Stage 4: Polish)

**Blockers:** none.

**Stage 4 entry items:**
- critic W1–W5 and N1–N13 (above)
- the existing Stage 4 backlog: Stage 2 W3, W4 and notes 3–8; FUNC-08 / §A9 recipes; PERF-02; COMPAT-02; QUAL-04
- VERSION 1.0.0, CHANGELOG and CODE_REVIEW
