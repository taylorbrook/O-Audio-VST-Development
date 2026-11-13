# Stage 4 — Polish / Validation — Verification, Rework R1 (per-stream feedback + reverb)

## Verification Date

2026-11-12

Supersedes the verdict of `VERIFICATION.md` (2026-11-11, pre-R1). Every automated check below was re-run in this session on `main` @ `428cca63` (plugin tree = C7 `c4276496`, clean before and after except the untracked `stages/3-gui/gate-report.json`, not from this session).
- All plugin `.cpp` files and the harness sources were touched and rebuilt: VST3, AU, Standalone, harness, param-dump, rc 0.
- The parity reference was re-extracted from the pin (O-Granulator `82406fce`) and rebuilt in a scratch directory.
- Logic and Live were closed throughout.

No automated result is copied from SUMMARY-R1. The D51 hands-on session (`HANDS-ON-SESSION-4.md`, 243 run rows) is human and **not** self-certified. The Windows CI arm (D54) was not run (T32 held, no push).

Not re-done:
- A row-by-row text diff of the 454 carried rows against `B3.full.txt`. The evidence here is the ID set (454 of 454 present, all PASS) and 0 FAIL.
- The Standalone look at the FX page in the real WebView (not clickable from the shell). It is in the hands-on sheet.

## Goal-Backward Analysis

### Original Goals (CONTEXT-R1 D60–D69, PLAN-R1 success criteria)

1. F1 ("Select a sidechain input") committed on its own, built, installed.
2. v2 mockup approved (D65) before the page was edited; the landed page is the approved variant.
3. Per-stream feedback and reverb, 8 globals removed: 293 parameters (200 / 74 / 19), 150 destinations, table version 2; spec, extractor, goldens and checksums agree.
4. Feedback 0 and the switch Off write nothing; Pre is bounded; a muted stream's tail rings out and its loop tank stops writing; Reset flushes the loop tank.
5. Per-voice sources reach the new destinations through the newest voice; the mono badge agrees everywhere.
6. No click on the feedback switch, the Pre / Post switch, or a mute with reverb on.
7. Render goldens byte-identical; parity 168 / 0; the 454 carried rows pass.
8. 38 single-stream presets bitwise and 16 multi-stream presets within −110 dB of the pre-R1 baseline; 10 showcase presets re-voiced.
9. C25m ≤ 40 %; every preset T ≤ 12 % / W ≤ 40 %; C25mPre recorded.
10. Record-in Arm returns to Off after a commit, a discard or an arm that cannot start, from the message thread, visible to the host; validators unaffected.
11. Five UI gates green with 48 states in three languages; bindings and closure at 293; the three v2 pairs `cmp` clean.
12. Build, install, auval (4 LFO-rate warnings exactly), pluginval 10 sidechain on / off, X2, Windows grep gate.
13. Only `forked/ControlGrid.h` moved among the forked leaves; no module, `scripts/` or `CMakeLists.txt` change.
14. Docs and release metadata; VERSION 1.0.0, no tag, no push.

### Deliverables (SUMMARY-R1)

C0 `a7ad2199` (F1), C1 `9b00a1d4` (baseline B0), C2 `c50944d5` (v2 mockup), C3 `db26325f` (B1 DSP), `1fb0e668` + C4 `082045f2` (Arm auto-clear), `86689980` (D65 record), C5 `1c5e7d70` (293-parameter surface, page, bank copy), C6 `4c89247c` (showcase, R1 rows, `resetTailHold`), C7 `c4276496` (docs).

Code read in verify:
- **Arm auto-clear** (`PluginProcessor.cpp:244–274`, `PluginProcessor.h:899–925`): one write site, `autoClearRecArm()`, asserted on the message thread, called only from `RecPollTimer::timerCallback` after `pollRecorder()`. `RecPollThread::run` calls `pollRecorder()` only. No listener and no `AsyncUpdater` is involved. `endGen` is read before `armGen`; the write is a begin / set / end gesture and is skipped when the parameter already reads Off.
- **Gates A and B** (`Engine.cpp:4086–4185`, `writeReverbAtomics`): both derived at every tick, unconditionally, with the 60 ms fade holds. Gate B requires Pre, the switch On, an audible stream, a feedback amount not settled at 0, no Reset hold, and Mix > 0.
- **Tanks:** `reverbA[s].process` is called for all four streams on every piece (`Engine.cpp:4346–4368`); `reverbB[s].process` likewise on the tap (`:3857–3882`). The finite guards run only while a gate or its fade hold is on.
- **Stream term** (`Engine.cpp:4317–4344`): the level / audibility / pan statements keep their 2.1 shape; the term is stored per stream, then summed.
- **`ControlGrid.h`:** the diff vs C0 is the 8 global rows moved into `Stream` with `fbEnable` / `reverbPre` added and `kGiModDests` 126 → 150, with the same clamps and conversions. It matches the manifest's R1 note; local sha256 == the manifest column (`c88bc291…`).
- **State functions:** one hunk, comment-only (`PluginProcessor.cpp:782`). See Issue 2.
- **Scope:** `git diff --name-only a7ad2199..HEAD` under the plugin lists 82 files, all inside the scope fence. Nothing under `modules/`, `scripts/`, or the plugin's `CMakeLists.txt` moved.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. F1 | ✅ Achieved (automated) | C0 is its own commit; built and installed. By eye = S3.3-L1 – L3, 3.3-15, LV2. |
| 2. v2 approval | ✅ Achieved | D65 recorded in STATUS (`86689980`, before C5); `index.html`, `js/i18n.js`, `tests/i18n-states.json` `cmp`-equal to the three `v2-*` files; `MOCKUP-ONLY` count 0. |
| 3. Contract | ✅ Achieved | Extractor into scratch: `OK: 293 params (200 Float / 74 Choice / 19 Bool), 150 dests, 18 sources`, three goldens `cmp`-identical. param-dump == `.planning/params.tsv`. Independent join vs the Stage 4 pin: 237 identical, 16 `modM_dest` rows differ in `numSteps` only (126 → 150), 8 gone, 32 new Float rows equal their old global row except id and name, 4 Bool + 4 Choice new, survivor order kept. G1–G6, D1–D5 in the harness. 7 / 7 contract hashes == STATUS. |
| 4. Feedback / Pre safety | ✅ Achieved | FL1p 0 bound violations; FL1sp written ≤ 0.7000; FL3p; FL4p / FL4t / FL4m; FL5p; TG1 (9600 scrubs, 0 non-finite downstream); RS2p == pristine at 200 ms and 2 s; MU1; IN1; SI1 −185.5 dBFS; TK1; each control fired. |
| 5. Destination class | ✅ Achieved | DC1 0 mismatches of 150; MC1; DR1 149 / 149; probe DM2 161 / 161 and 149 / 149; frontend ALL PASS. |
| 6. Clicks | ✅ Achieved (automated) | CK7 κ 0.945× / 1.315×; CK8 1.227× / 1.400×; CK9 1.166× / 0.288×; controls fired. Listening = sheet rows. |
| 7. No regression | ✅ Achieved | `renders-2.4`, legacy `renders-2.2`, 7 pinned `renders-2.1` lines byte-identical, both `ctrl FIRED`. Harness 512 PASS / 0 FAIL. Parity 168 / 0 incl. 2 GV rows; the 34 PB / PR lines equal `b0-parity.txt` with an empty diff. |
| 8. Presets | ✅ Achieved (automated) | The 64 B0 raws match `b0-preset-renders.sha256` (64 / 64). PT1 38 / 38 bitwise; PT2 16 / 16, worst null −132.57 dB, worst \|g − 1\| 4.66e-09; PT2n −9.75 dB and PT2n2 −6.42 dB fire; 10 showcase names printed as excluded. FP2 spread 6.32 dB. Audition = Pass 2. |
| 9. CPU | ✅ Achieved | Run alone. `--cpu25`: C25w 28.47 %, C25t 5.80 %. `--cpu26`: C25m 29.99 %, C25tm 6.75 %, C26rec 30.13 %, C25mPre 31.21 % (INFO). `--cpu4` 66 PASS / 0: T median 4.39 %, p90 6.65 %, max 9.13 % (Fifth Bloom); W max 10.32 %. |
| 10. Arm auto-clear | ✅ Achieved (automated) | AC1–AC8 + 6 controls in the harness; AC9 in the probe; code read above; 0 `rec_arm` lines in 13 pluginval logs; auval's 4 warnings are the LFO rates only. Host view = sheet rows. |
| 11. UI gates | ✅ Achieved | See Automated Checks. |
| 12. Validators | ✅ Achieved | See Automated Checks. Three seeds per format per sidechain setting (execute ran one at T27). |
| 13. Fork / scope | ✅ Achieved | Drift 0 / 13; under `forked/` only `ControlGrid.h` and `FORK-MANIFEST.md` moved. |
| 14. Docs / release | ✅ Achieved | One `## [1.0.0] - 2026-11-12` heading; CMake `VERSION 1.0.0`, installed `Info.plist` 1.0.0; PLUGINS.md `🚧 Stage 4 \| 1.0.0`; 0 `*GranulatorInstrument*` tags; sheet tally 7 + 111 + 71 + 46 + 8 = 243 run rows, 16 `R1-` rows. |
| Windows (carried, D54) | ⚠️ Partial (by decision) | Grep gate rc 0; 6 negative controls each rc 1. No Windows compile has happened; pending `/publish`. |
| Hands-on (carried, D51) | ⏳ Pending (human) | Sheet re-authored; session not run on the R1 build. |

## Requirements Verification

**Stage:** stage-4. REQUIREMENTS.md evidence lines rewritten per PD34; **no status moved**. Checksum re-pinned in STATUS.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-04: feedback into per-stream working copies | must | ✅ Complete (evidence rewritten) | Per-stream and Pre rows added (FL1p … IN1); carried FL / CL / RS rows green. |
| DSP-05: reverb | should | ✅ Complete (description + evidence rewritten) | Now per stream, Pre / Post. Parity evidence unchanged. |
| DSP-06: mod matrix | must | ✅ Complete (evidence rewritten) | 150 destinations; DR1 149 / 149, DC1, MC1. |
| FUNC-07: Record-in | should | ⚠️ Partial | Auto-clear and F1 automated arm green. Open: Logic / Live rows. |
| FUNC-08: factory content | should | ⚠️ Partial | Bank on the per-stream parameters, FP0–FP6. Open: Pass 2 audition. |
| PERF-02: CPU budget | must | ⚠️ Partial | C25m 29.99 %, C25mPre 31.21 %, bank within budget. Open: Logic CPU meter row. |
| QUAL-02, COMPAT-02, FUNC-01–06, FUNC-09, DSP-02 / 03 / 07, UI-01, UI-02, PERF-03, QUAL-01 | — | ⚠️ Partial (unchanged) | Automated arms green; each closes on its hands-on pass. UI-01's Windows arm and COMPAT-02 stay open by decision (D54, D57). |

Totals unchanged: 6 complete, 19 partial, 0 pending, 0 failed.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3, AU, Standalone, harness, param-dump) | ✅ Pass | rc 0 after touching every plugin `.cpp`; **0 warnings in `Source/`**. 14 conversion warnings in harness headers (`parity_gates_22 / 23.h`, `engine_gates_25a / b / d.h`), the rest SDK headers. |
| Render harness | ✅ Pass | **512 PASS / 0 FAIL**, rc 0, 0 NOT FIRED, 0 INVALID. The 454 baseline IDs (`B.ids.txt`) all present and PASS; the 58 new rows counted by name (14 AC, 44 DSP rows and controls). HM1 green. |
| `--egr1` | ✅ Pass | 58 / 0. |
| Render goldens | ✅ Pass | 2.4, legacy 2.2, 7 pinned 2.1 lines identical. |
| Parity (fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures, 332 outputs; 168 / 0. |
| Preset renders vs B0 | ✅ Pass | `--cmp-preset-renders … --exclude-showcase`: 4 / 0. Without the flag PT2 reads 16 / 26, as expected (the 10 re-voiced presets). |
| Contract extractor | ✅ Pass | Scratch output == the three goldens. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv` (293). |
| `--dump-ui-overrides` | ✅ Pass | Twice identical, == committed. |
| Fork drift | ✅ Pass | 13 leaves, 0 drift. |
| CPU (`--cpu25`, `--cpu26`, `--cpu4`, alone) | ✅ Pass | Values in goal 9. |
| `ui_frontend_check.js` | ✅ Pass | ALL PASS; `--nc` 4 / 4 fail. |
| `ui_binding_probe.js` | ✅ Pass | 293 / 293 both ways; value entry 15 / 15; lanes 12 / 12; toasts 3 / 3; arm 1 / 1 (AC9); fb 1 / 1 (FD1); mod 14 / 14; DM2 161 / 161, 149 / 149; VE15 smallest text gap 8.7 px. `--nc` run this time: 8 controls live. |
| check-i18n · fr-lint · zh-lint | ✅ Pass | 0 · 0 · 0 (446 zh entries at `mt`). |
| check-ui-labels | ✅ Pass | `param-dump (293 rows) + overrides`, `states: default + 48`. |
| boot-all-uis `--strict-tips` | ✅ Pass | rc 0; 0 warn, 0 failed, 0 DEAD, 0 late. |
| `preset_name_fit.js` | ✅ Pass | PN1 en / fr / zh-Hans, 64 names, 0 overflow (widest 139 / 139 px). |
| build-and-install.sh | ✅ Pass | rc 0; installed VST3 + AU binaries `cmp`-identical to the build; only the `-dev` variant on disk. |
| auval `aumu OuGI OuDv` | ✅ Pass | SUCCEEDED before the gate variant and after the restore; exactly 4 "did not retain default value" warnings, LFO 1–4 Rate. |
| pluginval 10, sidechain on | ✅ Pass | VST3 ×3 seeds SUCCESS, 2 ch in, 0 `!!!`; AU ×3 SUCCESS, the one carried `!!!` "Current program is -1". Seeds `0x1a2b3c`, `0x51ced`, `0x7e57`. |
| pluginval 10, sidechain off | ✅ Pass | `OGI_GATE_SC_DEFAULT_OFF=ON`: VST3 ×3 SUCCESS with **0 ch in**; AU ×3 SUCCESS (2 ch, AU wrapper, as before). |
| Restore | ✅ Pass | Cache variable `OFF`; shipping build rebuilt and reinstalled, installed == built; pluginval 10 VST3 SUCCESS with 2 ch in; `auval -a` lists one `O-GranulatorInstrument-dev`. |
| X2 | ✅ Pass | Shipping VST3 / AU / Standalone: 0 hook symbols, 0 `stub_` strings; harness control 5 symbols / 9 strings. |
| Factory folder | ✅ Pass | 64 `.json` + 2 sentinels, byte-identical to `--dump-factory-presets` from this build; 0 files with a bare `reverb_mix` / `feedback` / `output_gain` key. See Issue 1. |
| Windows grep gate | ✅ Pass | rc 0; NC1–NC6 each rc 1. |
| Real home | ✅ Pass | `~/Library/O-GranulatorInstrument` and `~/Music/Ouaricon` listings identical before and after the harness, parity, CPU, param-dump and UI runs. (The install's first instantiation then rewrote the Factory folder, as designed.) |
| Git | ✅ Pass | `main`; plugin tree clean before the verify edits; no tag. |

## Human Verification (D51 — one session, `HANDS-ON-SESSION-4.md`, 243 run rows)

- [ ] Pass 0 Setup (7 rows; delete pre-R1 user presets, do not reopen pre-R1 projects)
- [ ] Pass 1 Standalone (111 rows, incl. section 1d: the 16 R1 rows)
- [ ] Pass 2 Preset audition (71 rows; the 10 showcase presets have per-stream "expect" text)
- [ ] Pass 3 Logic AU (46 rows, incl. the F1 retests S3.3-L1 – L3)
- [ ] Pass 4 Live VST3 (8 rows, incl. LV2)
- Pass 5 (7 rows) is deferred, not run: Windows, MPE hardware, Dorico, Reaper.

## Issues Found

None blocking.

1. **The bank installed at the end of execute was not HEAD's bank.** Before this session's install the Factory folder carried `.bank-hash` `39997c18…` (written 19:21; "Drones · Deep Well" 1706 B; 59 files with `s1_reverb_mix`). This session's rebuild from the clean tree stamped `331e7660…` ("Deep Well" 1180 B; 57 files) and its content equals the harness dump of HEAD byte for byte. The old files are overwritten, so the cause is inferred from the sizes and counts: they fit the copied bank before the 10 showcase re-voices (C6). SUMMARY-R1's install line was therefore taken on a build without the showcase set. The installed build is now correct; any listening done between 19:21 and this verify heard the copied presets, not the showcase ones.
2. **SUMMARY-R1 says "no hunk in `getStateInformation` / `setStateInformation`".** There is one, comment-only (`PluginProcessor.cpp:782`, the `modDestTableVersion` note). No code moved; OS2 holds.
3. **PT2 needs `--exclude-showcase`.** PLAN-R1's verify list names the bare command; without the flag the row fails 16 / 26 by design. Not a defect, but the flag belongs in the recipe.
4. Residual risks from SUMMARY-R1 stand as recorded, not re-measured beyond the INFO lines: Pre keeps writing 9.7 s after note-off (PreTail; row R1-PRE1); Reset in Pre with notes held; the loop-tank tail survives a new file or a recording; FL2p is green but weak at its stimulus (16 voices −19.5 dB under 1 voice); pre-R1 sessions and user presets re-map silently (row R1-S1); 33 of 150 French destination titles clip in the 98 px select; Broken Radio `s1_level` 5.7 of 6.0.
5. Carried: PN1 zero margin (139 / 139 px); `--cpu4` W uses each preset's own polyphony, not 16 voices; 14 harness-header warnings; Windows unproven until `/publish`.
6. Deviations 1–15 in SUMMARY-R1 reviewed and accepted.
7. Foreign working-tree changes (`modules/registry.yaml`, agent memory, `.planning/STATE.md`, another plugin's files) not touched.

## Stage Verdict

**Status:** ⚠️ PARTIAL. Every automated gate independently verified green on the R1 tree. The D51 hands-on session (243 run rows) and the D54 Windows CI arm are open.

**Ready for next stage:** the shipping build is installed and release-ready on the automated side. 1.0.0 should not be published before the hands-on session.

**Blockers:** none on the automated side. Open: `HANDS-ON-SESSION-4.md` (a ❌ routes per the sheet's triage table), then the Windows validate run at `/publish`.
