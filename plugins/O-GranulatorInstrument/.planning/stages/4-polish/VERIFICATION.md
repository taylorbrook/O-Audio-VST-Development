# Stage 4 — Polish / Validation — Verification

## Verification Date

2026-11-11

Every automated check below was re-run in the verify session against `19f1a108`. The plugin tree was clean before and after (one untracked `stages/3-gui/gate-report.json`, not from this session).
- Touched `PluginProcessor.cpp`, `PluginEditor.cpp`, `Engine.cpp`, `SlotLoader.cpp`, `index.html`, the harness `main.cpp` and `factory_gen.cpp`; rebuilt the harness, param-dump, VST3, AU and Standalone (rc 0); reinstalled the shipping build with `build-and-install.sh`.
- Parity reference re-extracted from the pin (O-Granulator `82406fce`) and rebuilt in a fresh scratch dir.
- Logic and Live closed throughout.

No automated result here is copied from SUMMARY.md. The D51 hands-on session (`HANDS-ON-SESSION-4.md`) is human and **not** self-certified. The Windows CI arm (D54) was not run: T19 was held, no push.

Not re-done: a 3.3 baseline harness was **not** rebuilt for a row-by-row text diff of the 431 carried rows. The evidence is the count (454 PASS = 431 + the 23 new rows, each counted by name) and 0 FAIL.

## Goal-Backward Analysis

### Original Goals (CONTEXT D51–D59 / PLAN success criteria)

1. 8 generated factory sources ship within budget with full-PCM pins; stubs are harness-only.
2. 64 factory presets load from fresh, after dirty state and after play, round-trip through save, never touch `output_gain`, resolve lanes without disk, switch without clicks.
3. The bank refreshes on any bank change at a fixed version; factory files cannot be overwritten from the editor.
4. Every factory preset meets T ≤ 12 % and W ≤ 40 %; the `--cpu26` regression rows hold.
5. `output_gain`, `rec_gain`, `s1`–`s4_level` default to exactly 0 dB; param-dump moved only in those 6 `defaultText` cells; renders unchanged.
6. auval warnings frozen at the 4 LFO rates and documented.
7. D55a–c: `wavFailed` toast, one toast per failing lane, bounded close on a hung disk.
8. The harness never writes the real home.
9. Five UI gates green with 47 states; preset names fit in EN / FR / zh-Hans; both file pairs `cmp` clean.
10. Windows WebView2 grep gate green; Windows CI validate run green, or recorded as pending `/publish`.
11. VERSION 1.0.0, single-heading CHANGELOG, PLUGINS.md / REQUIREMENTS / FORK-MANIFEST updated, no tag.
12. `HANDS-ON-SESSION-4.md` ready for the one combined human session (D51).

### Deliverables (SUMMARY.md)

C1 `bc46d052` (temp HOME, snap-only ranges, AU1, detached writer), C2 `5667a95a` (`wavFailed` + toast chain, 8 sources, stubs → fixtures), C3 `63ad0c5d` (bank, stamp, save guard, VERSION 1.0.0, FP rows, `--cpu4`), C4 `f0516bca` (Windows gate, toast probe rows, PN1), C5 `1b17f41c` + `19f1a108` (docs, T19 held).

Code read in verify:
- **Scope:** `git diff --name-only bf236204 HEAD` — 58 files, all under `plugins/O-GranulatorInstrument/` except `PLUGINS.md` and `modules/registry.yaml`. Under `Source/dsp/forked/` only `FORK-MANIFEST.md` moved. 0 debt markers in added source lines.
- **Bank stamp** (`PluginProcessor.cpp:147–167`): on a missing or different `.bank-hash`, only `*.json` directly in the factory dir plus the two sentinels are deleted; the stamp is written last. Compiled out of the param-dump console (`OGI_NO_FACTORY_WRITE`, `:122`).
- **Save guard** (`PluginProcessor.cpp:169–180`): a target equal to or under the factory dir returns false. The editor's Save-As path calls it (`PluginEditor.cpp:1081`); the by-name path still goes through the manager's own `savePreset`.
- **Snap-only range** (`GiParamIds.h`, `exactZeroDbRange`): clamp, then |v| < 1e-5 → 0, on exactly the six rows.
- **Presets:** `validateSpec` rejects `output_gain` / `rec_arm`; 0 of the 64 installed `.json` files contain `output_gain`.
- Contract hashes 7 / 7 == STATUS frontmatter.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Factory sources | ✅ Achieved | FSP1 8 / 8 (FSP1n → 7 / 8); `shasum -c factory-sources.sha256` 8 / 8 OK; Σ 3 455 333 B, largest 865 564 B; shipping binaries carry 0 `stub_` strings (harness control 9). |
| 2. 64 presets | ✅ Achieved (automated) | FP0 336 / 336; FP1 64 / 64 (FP1n, FP1n2 fire); FP2 spread 6.32 dB; FP3 504 onsets, 0 off-grid; FP4 320 / 320 bitwise; FP4b 64 / 64; FP6 worst 0.938× (FP6n 9.85×). Audition = SESSION-4 Pass 2. |
| 3. Stamp + guard | ✅ Achieved | FP5 (real path under the temp home, stamp arm); PG1 (PG1n overwrites); code read above. |
| 4. CPU | ✅ Achieved | `--cpu4` 66 PASS / 0: 64 / 64 presets, T max 8.63 %, W max 10.41 %; C25m 28.88 %, C25tm 6.77 %. See Issue 1 for what W covers. |
| 5. Exact 0 dB | ✅ Achieved | AU1 "PD8 rows exact 0 6 / 6"; param-dump == `.planning/params.tsv`; the diff vs `77d5ad04` is exactly 6 lines, `defaultText` only; renders + legacy byte-identical. |
| 6. auval | ✅ Achieved | PASS; exactly 4 "did not retain default value" warnings (retrievedValue 0.328712 = the LFO rates); AU1 set == {lfo1..4_rate} (AU1n → 5); CHANGELOG "Known validator notes". |
| 7. D55a–c | ✅ Achieved (automated) | UWF1 (UWF1n fires); probe TC1 `{n:2}` at +150 ms, `{n:3}` at +1750 ms (TC1n fires), TW1; USV3 dtor 9.2 ms (USV3n 3011.5 ms). By eye = SESSION-4 1c rows. |
| 8. Home hygiene | ✅ Achieved | HM1 (HM1n fires); an independent `ls -laR` of `~/Library/O-GranulatorInstrument` and `~/Music/Ouaricon` before / after the full run is identical. param-dump run left the real bank listing unchanged. |
| 9. UI gates | ✅ Achieved | See Automated Checks. |
| 10. Windows | ⚠️ Partial (by decision) | Grep gate rc 0; 6 negative controls each rc 1. **No Windows / x86_64 compile has happened** — T19 held; D54 CI arm pending `/publish`. |
| 11. Release metadata | ✅ Achieved | CMake `VERSION 1.0.0`, installed `Info.plist` 1.0.0; `grep -c '^## \['` = 1; CI awk extract 26 lines; PLUGINS.md `🚧 Stage 4 \| 1.0.0`; FORK-MANIFEST carries `e77d29c8`; 0 `*GranulatorInstrument*` tags. |
| 12. Hands-on sheet | ⏳ Pending (human) | Sheet present; all 140 carried rows (2.1–3.3) + 6 Logic rows found by ID, 0 missing; old sheets carry the superseded header. Session not run. |

## Requirements Verification

**Stage:** stage-4. Requirements for this stage: FUNC-08 (should), COMPAT-02 (nice), QUAL-02 (nice), plus every remaining `partial` row.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-08: factory sources + presets that load out of the box | should | ⚠️ Partial | Automated arm complete (FSP1, FP0–FP6, first-run bank on disk: 64 `.json` + 2 sentinels). Open: SESSION-4 Pass 2 audition. |
| QUAL-02: sidechain absent / unrouted degrades cleanly | nice | ⚠️ Partial | pluginval 10 with the sidechain off: VST3 + AU SUCCESS. Open: Logic AU unrouted key bus + Live VST3 no aux (SESSION-4 Pass 3 / 4). |
| COMPAT-02: MPE / VST3 note expression | nice | ⚠️ Partial (stays, D57) | 2.5b harness rows still green in the 454. No DAW pass this stage; carried to 1.0.x. |
| PERF-02: CPU budget at 16 × 4 | must | ⚠️ Partial | Automated verdict green (`--cpu4`, C25m). Open: Logic CPU meter row. |
| FUNC-01–07, FUNC-09, DSP-02 / 03 / 07, UI-01, UI-02, PERF-03, QUAL-01 | — | ⚠️ Partial (unchanged) | Automated arms unchanged and green; each closes on its SESSION-4 pass. UI-01's Windows arm stays open (D54). |

REQUIREMENTS.md: **no status moved at verify** (checksum unchanged). Totals: 6 complete, 19 partial, 0 pending, 0 failed.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness, param-dump, VST3, AU, Standalone) | ✅ Pass | rc 0; **0 warnings in `Source/`**. 14 `-Wimplicit-int-float-conversion` warnings in harness headers (`parity_gates_22/23.h`, `engine_gates_25a/b/d.h`); the rest are SDK headers. |
| build-and-install.sh | ✅ Pass | rc 0; installed VST3 + AU binaries `cmp`-identical to the build; only the `-dev` variant on disk. |
| Render harness | ✅ Pass | **454 PASS / 0 FAIL**, rc 0, 0 NOT FIRED, 192 controls fired, 193 s. The 23 Stage 4 rows all PASS. |
| Parity (fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures, 332 outputs; **168 / 0**; GV **2 / 0**. |
| Render goldens | ✅ Pass | == `renders-2.4.sha256`; legacy == `renders-2.2.sha256` (14 / 14 each). |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv` (261). |
| Fork drift | ✅ Pass | 13 leaves, 0 drift. |
| `--cpu4` (run alone) | ✅ Pass | 64 / 64. T median 4.39 %, p90 6.53 %, max 8.63 % (Hollow Wind); W max 10.41 % (Reverse River). C25m 28.88 % ≤ 40; C25tm 6.77 % ≤ 12. One preset (Sand Drift) took a second run. |
| X2 | ✅ Pass | Shipping VST3 / AU / Standalone: 0 hook symbols, 0 `stub_` strings; harness control 12 symbols / 9 strings. |
| `generic-overrides.json` | ✅ Pass | 148 653 B, twice identical, == committed. |
| `ui_frontend_check.js` | ✅ Pass | ALL PASS; `--nc` 4 / 4 fail. |
| `ui_binding_probe.js` | ✅ Pass | 261 / 261 × 2; VE 14 / 14; lanes 12 / 12; toasts 3 / 3; mod 14 / 14; DM2 137 == 137, 125 / 125; 0 page errors. `--nc` not re-run this session. |
| `preset_name_fit.js` | ✅ Pass | PN1 en / fr / zh-Hans, 64 names, 0 overflow; PN1n overflows (215 / 152 px). |
| check-i18n · fr-lint · zh-lint | ✅ Pass | 0 · 0 · 0 (457 zh at `mt`). |
| check-ui-labels | ✅ Pass | `param-dump (261 rows) + overrides`, `states: default + 47`; ALL CHECKS PASSED. |
| boot-all-uis `--strict-tips` | ✅ Pass | rc 0; 0 warn, 0 failed, 0 DEAD, 0 late. |
| auval `aumu OuGI OuDv` | ✅ Pass | SUCCEEDED, before and after the restore; exactly 4 warnings; `ZZZZ` control rc 2. |
| pluginval 10 VST3, SC on | ✅ Pass | SUCCESS, 2 ch in, 0 `!!!`. |
| pluginval 10 AU, SC on | ✅ Pass | SUCCESS; one `!!!` "Current program is -1". |
| pluginval 10 VST3, SC off | ✅ Pass | SUCCESS, **0 ch in**, 0 `!!!` (`OGI_GATE_SC_DEFAULT_OFF=ON` variant). |
| pluginval 10 AU, SC off | ✅ Pass | SUCCESS, same single `!!!`; 2 ch (AU wrapper, as before). |
| Restore + `auval -a` | ✅ Pass | Cache `OFF`; shipping build rebuilt and reinstalled (installed == built, 2 ch in, 0 hook symbols); auval PASS with 4 warnings; `aumu OuGI OuDv - Ouaricon Audio Development: O-GranulatorInstrument-dev`. |
| Windows grep gate | ✅ Pass | rc 0; NC1–NC6 each rc 1. |
| `cmp` | ✅ Pass | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js`. |
| CHANGELOG | ✅ Pass | 1 heading; CI extract 26 lines. |
| Real home | ✅ Pass | `~/Library/O-GranulatorInstrument` listing unchanged across the harness and param-dump; `~/Music/Ouaricon` absent throughout. |
| Git | ✅ Pass | `main` @ `19f1a108`; plugin tree clean; no `O-GranulatorInstrument*` tag. |

Not run: Standalone preset-menu smoke (WKWebView cannot be clicked from automation; SESSION-4 row), the Windows CI validate dispatch (held), the probe's `--nc` controls.

## Human Verification (D51 — one session, `HANDS-ON-SESSION-4.md`)

- [ ] Pass 0 Setup (6 rows)
- [ ] Pass 1 Standalone (103 rows: engine 2.1–2.5b, UI 3.1–3.3, new D55 / D59 rows)
- [ ] Pass 2 Preset audition (71 rows)
- [ ] Pass 3 Logic AU (41 rows; closes QUAL-02's AU arm, FUNC-06 / 07)
- [ ] Pass 4 Live VST3 (6 rows)
- Pass 5 (7 rows) is deferred, not run: Windows, MPE hardware, Dorico, Reaper.

## Issues Found

None blocking.

1. **`--cpu4` W does not reach 16 voices.** W holds 16 keys, but each preset's own `polyphony` applies: 6 on the Drones and two Motion presets, 4 on the Textures, mono on two Expressive presets, the default 8 elsewhere. For about 20 presets W equals T (same live grain count); for the rest W is 8 voices (live grains scale by 8 / 6). So no factory preset is measured at 16 × 4. PERF-02 still holds on the evidence: the saturated 16-voice case is C25m 28.88 % ≤ 40, and doubling the worst W (10.41 %) stays near 21 %. The W column should be read as "the preset as authored", not "16 voices". A later pass could force `polyphony` 16 for W.
2. **PN1 has zero margin.** The widest name ("Expressive · Wheel Morph") measures 139 / 139 px in all three languages. Any longer name, or a font metric change, overflows.
3. **Harness header warnings.** 14 int-to-float conversion warnings in test headers; none in `Source/`. SUMMARY's "0 warnings in plugin sources" holds for `Source/` only.
4. **Windows is unproven.** The first Windows / x86_64 compile happens at `/publish`. The grep gate pins source lines; it cannot catch an MSVC error.
5. Carried, recorded in SUMMARY and CHANGELOG: detached WAV writer vs a host unloading the bundle; `~SlotLoader` 10 s join on a hung disk; FLAC regeneration identity claimed on arm64 only; the `customStateOf` dangling-pointer pattern still present in O-Granulator `Source/FactoryPresets.h:867–868` (worth an `/improve` there).
6. Deviations 1–13 in SUMMARY reviewed and accepted.
7. Foreign working-tree changes (`modules/registry.yaml`, agent memory, `.planning/STATE.md`) not touched.

## Stage Verdict

**Status:** ⚠️ PARTIAL. Every automated gate independently verified green. The D51 hands-on session (227 run rows) and the D54 Windows CI arm are open.

**Ready for next stage:** the build is installed and release-ready on the automated side; 1.0.0 should not be published before the hands-on session.

**Blockers:** none on the automated side. Open: `HANDS-ON-SESSION-4.md` (a ❌ routes per the sheet's triage table: `PLAN-4.F` gap wave, CHANGELOG known issue, or a row rewrite), then the Windows validate run at `/publish`.

## Addendum — hands-on findings, 2026-11-11 (session in progress, verdict still PARTIAL)

The D51 session was started and is not finished. Two findings so far (sheet triage table, F1 / F2).

**F1 — Record-in gave no instruction when no side-chain source is selected (Logic).** By design: the host owns side-chain routing and a plugin cannot preselect it. Fixed in source as UI copy: `rec.noSignal` now reads "Select a sidechain input" / "Choisir une sidechain" / "请选择侧链输入", and the `rec-status` tooltip leads with the instruction; `v1-i18n.js` synced. Re-run on the change: check-i18n, fr-lint, zh-lint, check-ui-labels (default + 47 states), `ui_frontend_check.js` — all green. **Not** done: build, install, the rest of the automated bar, the by-eye retest (S3.3-L1 – L3, LV2, 3.3-15), CHANGELOG line, commit.

Noticed while tracing F1, not changed: `rec_arm` stays On after a commit or discard (D34 one-shot), so the next click on Arm only turns it Off, and the lane "Record" button (`index.html:3835`) does nothing while Arm is On. Candidate for the rework cycle.

**F2 — per-stream feedback and reverb (scope change, user decision).** The global FEEDBACK NETWORK + OUTPUT REVERB band on the FX · FEEDBACK page is to be redone: each stream column carries its own feedback controls (Amount, Low Cut, High Cut, Saturate, Diffuse) and its own reverb (Mix, Size, Damp). Decided: before 1.0.0; the globals are removed (no master Amount, no output reverb); Freeze and Reset All stay global.

Inputs for the discuss phase:
- Parameters: the 8 globals (`feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`, `reverb_mix`, `reverb_size`, `reverb_damp`) become 32 per-stream rows; 261 → 285. `parameter-spec.md`, `params.tsv`, the param-dump golden and the 126-entry destination table (8 global entries → 32 per-stream) all re-pin. Nothing is published, so the "append-only" rule can be broken once, deliberately.
- DSP: the four feedback chains and record heads are already per stream; only their parameter sources change. Reverb goes from one output tank to four per-stream tanks — CPU to be measured against the 40 % / 12 % budgets (C25m, `--cpu4`). Whether the reverb sits before or after the feedback tap is open (today the reverb is outside the loop).
- Modulation: these destinations are per stream but not per voice, so D4 (newest voice, "mono" badge) still applies.
- UI: the stream columns need 8 more knobs each in a 1200 × 800 frame; the freed band gives roughly one knob row, two are needed. Needs a mockup pass (the "fig." cells, knob pitch, or the frame height are the levers). An optional per-stream feedback on / off came up in the same conversation.
- Presets: 10 factory presets set `feedback` and 59 set `reverb_mix`; all 64 specs are rewritten and FP0–FP6 / `--cpu4` rerun. Render goldens with feedback or reverb on re-pin; feedback-0 / reverb-0 renders should stay byte-identical.
- Hands-on: rows 2.4-1 – 2.4-7, 2.5a-7, 2.5a-11, 3.2-10 – 3.2-12 and the Pass 2 audition are provisional until this lands; the sheet is regenerated at the end of the rework.
