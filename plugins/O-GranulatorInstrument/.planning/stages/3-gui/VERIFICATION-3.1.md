# Stage 3 — GUI · Phase 3.1 (layout, controls, bindings) — Verification

## Verification Date

2026-11-10

I re-ran every automated check below myself in the verify session, against `c000f12a`. The plugin tree was clean beforehand and is clean after.
- I touched `PluginProcessor.cpp`, `PluginEditor.cpp`, `Engine.cpp` and `index.html`, then rebuilt the harness, param-dump and Standalone (rc 0, 0 warnings) and reinstalled with `build-and-install.sh`.
- I re-extracted and re-built the parity reference from the pin into a fresh scratch dir.
- No DAW was open during pluginval.

No automated result here is copied from SUMMARY-3.1.md. HANDS-ON-3.1 (16 rows) is a human gate and is **not** self-certified.

## Goal-Backward Analysis

### Original Goals (CONTEXT-3.1 / PLAN-3.1)

1. The v1 page in the WebView editor at a fixed 1200 × 800; all four pages render.
2. All 261 parameters bound two-way by ID and type (176 slider / 70 combo / 15 toggle).
3. All 34 native functions registered; each completes on every path; `clearSlot` / `saveRecording` refuse cleanly (D37).
4. Presets: round trip incl. scale, masks, seed, slot sources (no blobs, PD1); `output_gain` and `rec_arm` untouched; two reopens clean.
5. Slot actions live: load, relink, reset, reset all, embed; lane shows name, badges, pristine waveform.
6. `getTelemetry` emits the exact 3.1 shape; `getWindowCurve` is the engine's curve.
7. Value entry (D41) on 161 knobs; `index.html` == `v1-ui.html`.
8. DevLoader and `OGI_DEV_LOADER` removed (D38).
9. Five UI gates green on the real tree with overrides + 19 states (D42).
10. Stage 2 audio does not move.
11. HANDS-ON-3.1 ready.

### Deliverables (SUMMARY-3.1.md)

C1 `35524e1b` (processor UI surface, `GiUiJson.h`, harness rows, overrides dump), C2 `12b210a4` (editor, CMake, DevLoader removed), C3 `6ea16b17` (value entry, closure + probe scripts, 19 states), C4 `c000f12a` (docs).

Code read in verify:
- `PluginEditor.cpp` whole file: member / constructor order relays → `dropSessions` → `webView` → attachments; 34 `withNativeFunction` registrations by literal name; every native completes on every path (async dialogs: hoisted `SafePointer`, bare return only when the editor is gone, `busy` / `cancelled` otherwise); slot conversion 1…4 → 0…3 only in `slotArg` (+ `onCommitFile`, `getWindowCurve`); explicit bare-path resource ladder (10); Windows block with `withBackend (webview2)` + `withUserDataFolder`; `setResizable (false, false)` + `setSize (1200, 800)`; no timer, no `emitEvent`, WebView never hidden.
- `PluginProcessor.cpp:786-936` preset custom state: seed as int64 string; `slots` always written without a blob; key present → `restoreSlots`, absent → untouched; malformed entries skipped; `embedded` never read. `:942-1048` revisions / overview / window curve: message thread, loader lock + atomics only.
- `GiUiJson.h`: no WebView type; telemetry is always the full PD8 object.
- Page `index.html:3717-3912`: slot actions re-pull state; overview fetch keyed on `ovRev`, a stale answer is re-fetched (C++ reads `rev` before the copy; the page stores the telemetry's `rev`, which is never newer than the answer).
- Scope audit `git diff --stat e4987f8a HEAD`: nothing outside `plugins/O-GranulatorInstrument/`; nothing under `Source/dsp/forked/`; CMake diff is exactly the three PD14 edits; contract hashes == STATUS frontmatter.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Editor 1200 × 800 | ✅ Achieved (automated) | Standalone launched: window 1202 × 858 (1200 × 800 content + title bar + Standalone notice bar); window-only capture read: STREAMS page rendered in the real WKWebView, readouts bound, four window figures drawn, EB Garamond text, no blank region. Other three pages: headless only (19 states × 3 languages) → HANDS-ON row 1. pluginval editor pass ×4. |
| 2. 261 bindings | ✅ Achieved (stub) | Closure: 176 / 70 / 15, APVTS == editor == page by ID and type. Probe: **261 / 261** host → UI, **261 / 261** UI → host, 0 page errors. NC (one `bindKnob` removed) FAILS at 260 on `s2_drive`. Real relay ↔ attachment plumbing in a host = HANDS-ON rows 2–3. |
| 3. 34 natives | ✅ Achieved | Closure [3] 34 = 34 both directions; NC2 (`getWindowCurve` removed) FAILS. Completion on every path: code read. |
| 4. Presets | ✅ Achieved | UPM1 259 / 259 params + scale + masks + seed; UPM2 (+ UPM2n fires); UPM3 0 arms (+ UPM3n arms, so the stimulus is live); UPM4 two reopens; UPM5 (+ UPM5n); UPM6 sources restored, no `embedded`, 6504 B; UPM6b absent key leaves slots. |
| 5. Slot actions | ✅ Achieved (processor + stub) | USR1 revision 8 / 8 events, 0 / 100 idle moves (+ USR1n fires); USR2 `ovRev` 7 / 7; UOV1 overview max == −12 dBFS, worst diff vs independent fold 0. From the real UI = HANDS-ON row 7. |
| 6. Telemetry / window curve | ✅ Achieved | UTL31 exact key set, `rec.bus` by layout 3 / 3; UWC1 max err 5.9e-7 vs Hann, UWC2. |
| 7. Value entry | ✅ Achieved | VE **14 / 14**; `cmp index.html v1-ui.html` identical; `cmp i18n.js v1-i18n.js` identical. |
| 8. DevLoader gone | ✅ Achieved | Files deleted; closure [14] 0 tokens under `Source/` + `CMakeLists.txt`; 0 `DevLoader` symbols in the installed binaries. |
| 9. UI gates | ✅ Achieved | See Automated Checks. |
| 10. Stage 2 unchanged | ✅ Achieved | Harness 374 / 0 (357 carried + 17 new), parity 168 / 0 + GV 2 / 0, renders-2.4 + legacy byte-identical, param-dump identical, drift 0 / 13, X2 0. |
| 11. HANDS-ON-3.1 | ⏳ Pending (human) | 16 rows written; not run. |

## Requirements Verification

**Stage:** stage-3. Requirements for this stage: 2 (UI-01, UI-02; both `should`). 3.1 closes neither by design (CONTEXT-3.1).

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: visual language; lanes with drag-drop, loop handles, playhead, grain dots | should | ⏸️ Pending (3.2) | In: v1 visual language, loop handles bound, pristine waveform. Open: playhead, grain dots, durable drop. |
| UI-02: drag-to-modulate; MOD page sources | should | ⏸️ Pending (3.3) | Not in 3.1. |

No status cell in REQUIREMENTS.md changes. Totals unchanged from VERIFICATION-2.6: 6 complete, 15 partial, 4 pending, 0 failed.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness, param-dump, Standalone) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings. |
| build-and-install.sh | ✅ Pass | rc 0, 0 warnings, ×3 (shipping, SC-off variant, restore). Only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Render harness | ✅ Pass | **374 PASS / 0 FAIL**, rc 0, 0 NOT FIRED. 17 U-rows PASS; UPM2n / UPM3n / UPM5n / USR1n each fire. |
| Parity (fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures; **168 / 0**; `--rows GV` **2 / 0**. |
| Render goldens | ✅ Pass | `--dump-render-hashes` == `renders-2.4.sha256`; legacy == `renders-2.2.sha256`; again after the shipping restore. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv` (261). |
| Fork drift | ✅ Pass | 13 leaves, 0 drift. |
| X2 symbol exclusion | ✅ Pass | 0 `ogran::` and 0 test-hook symbols in the installed VST3 and AU (before and after the restore). Control: the harness has 18. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`, before and after the restore. `ZZZZ` control rc 2. |
| pluginval 10 VST3, SC on | ✅ Pass | SUCCESS, rc 0, 2 ch in, 0 `!!!`. |
| pluginval 10 AU, SC on | ✅ Pass | SUCCESS, rc 0; only `!!!` is "Current program is -1". |
| pluginval 10 VST3, SC off (`OGI_GATE_SC_DEFAULT_OFF=ON`) | ✅ Pass | SUCCESS, rc 0, **0 ch in**, 0 `!!!`. |
| pluginval 10 AU, SC off | ✅ Pass | SUCCESS, rc 0, same single `!!!`; reports 2 ch in (AU wrapper, as at 2.6). Cache restored to `OFF`, shipping build reinstalled. |
| `generic-overrides.json` | ✅ Pass | Generated twice, byte-identical (141,773 B); `git diff --exit-code` clean. |
| `ui_frontend_check.js` | ✅ Pass | ALL PASS (99), rc 0. `--nc`: NC1 (`glide` → toggle) and NC2 both FAIL their check. |
| `ui_binding_probe.js` | ✅ Pass | 261 / 261 both ways; VE 14 / 14; 0 page errors. `--nc` fails at 260. |
| check-i18n · fr-lint · zh-lint | ✅ Pass | 0 · 0 · 0 (454 zh entries at `mt`, counted). |
| check-ui-labels | ✅ Pass | rc 0; `seed=param-dump (261 rows) + overrides`; `states: default + 19`. |
| boot-all-uis `--strict-tips` | ✅ Pass | rc 0; 0 warn, 0 failed, 0 DEAD, 0 late. |
| Standalone smoke (real WKWebView) | ✅ Pass | Default page captured window-only and read; see Goal 1. |
| Recordings guard | ✅ Pass | `~/Music/Ouaricon/O-GranulatorInstrument` absent after every run. |
| Git | ✅ Pass | `main`; no uncommitted plugin change; no `O-GranulatorInstrument*` tag. |

Not re-run: `--cpu26` (not a 3.1 gate; renders are byte-identical and the editor adds nothing to the audio path).

## Human Verification

- [ ] HANDS-ON-3.1 rows 1–16 (`stages/3-gui/HANDS-ON-3.1.md`). Highest value: row 3 (automation both ways in a DAW — the one path no automation covers), row 4 / 6 (preset + reopen), row 7 (slot actions from the UI), row 15 (Inspector console, no 404).
- [ ] HANDS-ON-2.1–2.6 (79 rows): run once at 3.3 verify (D39).

## Issues Found

None blocking. For your decision:

- **Loading any saved preset replaces all four slots.** A preset always writes a `slots` array, and a present key clears every slot it does not list (PD1 as signed). A preset saved with empty lanes therefore empties loaded lanes, including an unsaved feedback-written tape. Consistent with the sign-off; flagging it because HANDS-ON row 4 will show it.
- **`savePresetWithDialog` into a sub-folder of the user presets folder** saves by name into the top-level folder instead (`isAChildOf` is true for nested paths, `PluginEditor.cpp:944`). Parent pattern; low impact.
- **Baseline B is not reproducible in verify.** SUMMARY's "357 carried rows: diff empty" compared against an extract in the execute scratchpad. Verified here as 374 = 357 + 17 with 0 FAIL and byte-identical renders instead.
- **Plan / SUMMARY wording:** "param-dump == `tests/golden/params.tsv`" names the wrong file. That file is the spec-format golden the harness row G1 reads; the param-dump reference is `.planning/params.tsv` (identical).
- **Stale `OGI_DEV_LOADER:BOOL=ON` in `build/CMakeCache.txt`.** The option is gone, so the entry is inert.
- **"heavy on CPU" hint shows at the default Grain Limit (256)** on the STREAMS page. 3.3 scope (`grain_limit` heavy hint); noted so it is not read as a 3.1 defect.
- Carried from SUMMARY, unchanged: dropped files are not durable (3.2); scan-mode `aria-valuetext` not re-localized (D40 call; the probe prints it as a NOTE); `output_gain` default reads 7.15e-07 dB.
- Deviations 1–17 in SUMMARY-3.1 reviewed and accepted. Deviation 2 (`ovRev` folds the publish count) is the right call: USR2 proves it and the page-side read confirms a redundant fetch at worst.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, O-GrainScatter) not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green. HANDS-ON-3.1 (human, 16 rows) is pending.

**Ready for next phase (3.2):** Yes on the automated evidence. A ❌ in HANDS-ON-3.1 routes to `/plugin-execute O-GranulatorInstrument 3-gui`.

**Blockers:** none. Open human gates: HANDS-ON-3.1; HANDS-ON-2.1–2.6 (3.3 verify).
