# Stage 3 — GUI · Phase 3.3 (MOD page, voice section, meters) — Verification

## Verification Date

2026-11-11

Every automated check below was re-run in the verify session against `77d5ad04`. The plugin tree was clean before and after.
- Touched `PluginProcessor.cpp`, `PluginEditor.cpp`, `Engine.cpp`, `SlotLoader.cpp`, the recorder source and `index.html`; rebuilt the harness, param-dump, VST3, AU and Standalone (rc 0, **0 warnings in plugin sources**); reinstalled the shipping build with `build-and-install.sh`.
- Parity reference re-extracted from the pin (O-Granulator `82406fce`) and rebuilt in a fresh scratch dir.
- A 3.2 baseline harness was rebuilt from `1a3cbcf9` for the carried-row diff.
- Logic closed throughout; Dorico 6 open (not hosting this plugin).

No automated result here is copied from SUMMARY-3.3.md. The D47 hands-on session (HANDS-ON-2.1–2.6, 3.1, 3.2, 3.3 + the Logic AU pass) is human and **not** self-certified.

## Goal-Backward Analysis

### Original Goals (CONTEXT-3.3 / PLAN-3.3 success criteria)

1. MOD page shows the 17 sources from `srcG`; chip glow, read-outs, rings and loop-edge ticks agree with the engine.
2. Drag-to-modulate from every dock chip onto a knob and a loop handle; every drop target accepts a drop.
3. Mono badge rule == the engine's per-voice × global predicates, every pair and LFO mode.
4. Rings move for per-voice routes from the newest voice; rest at base with no voice (per-voice destinations).
5. Voice strip: 16 rows keyed by serial, slot 16–19 voices shown, releases, steals visible ≥ 1 poll and ≤ 300 ms + 1 poll, `newest` from the engine rule.
6. Env scopes, LFO views (dot on the curve at any phase), `wantSt` clamp marker, grain warn bit live.
7. Output and SC meters L / R, max since read, one reader, known SC levels exact, output bit-exact.
8. `limit` truthful; heavy hint 256 on / 128 off.
9. D49: no vacuous state, CM6 collision-proof, UPM6b note in D45 wording.
10. 0 audio-thread allocations; Stage 2, 3.1, 3.2 unchanged apart from PD20.
11. Five UI gates green with 46 states; both file pairs `cmp` clean.
12. HANDS-ON-3.3 + HANDS-ON-SESSION-3.3 ready for the one session (D47).

### Deliverables (SUMMARY-3.3.md)

C1 `0389000c` (engine / recorder / loader / processor, 3.3 encoder, `ui_gates_33`, carried amendments, overrides), C2 `bbbc6de3` (LFO-dot defect, editor notes), C3 `34f8e83e` (closure [12] / [15] / [16], `--mod-only` probe rows, 46 states, zh-Hans clamp fit), C4 `77d5ad04` (docs).

Code read in verify:
- **One reader:** `buildUiTelemetry` (`PluginProcessor.cpp:1173–1552`) holds the only calls to `frames.read` (:1204), `stolenCount.load` (:1231 / :1237), `takeOutPeaks` (:1460), `takeScPeaks` (:1519). `pushSteal` is called only from `stealVoice`. Editor diff is comments only.
- **Scope:** `git diff --stat 1a3cbcf9 HEAD` — 29 files, all under `plugins/O-GranulatorInstrument/`; nothing under `Source/dsp/forked/`; `CMakeLists.txt` untouched. Contract hashes 7 / 7 == STATUS frontmatter. 0 debt markers in changed files.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. 17 sources, rings, ticks | ✅ Achieved (processor + stub) | USRC1, UMN1, ULH1 PASS (UMN1n, ULH1n fire); probe MV1–MV3 PASS. |
| 2. Drag-to-modulate | ✅ Achieved (stub) | DM1 (17 chips × knob + loop handle, real pointer sequences); DM2 census 137 == runtime 137, 125 / 125 destinations; DM2 NC fails. Real WKWebView = HANDS-ON-3.3 rows 1–2. |
| 3. Mono badge rule | ✅ Achieved | UMB1 6,375 / 6,375; MV5; closure [16] (page rule vs `ui_gates_33` pageRule, 16 LFO-mode combos). |
| 4. Rings from newest voice | ✅ Achieved | UMN1 (per-voice dest at base on voiceless polls); MV2. Mono routes hold the last value (deviation 7, Known limits). |
| 5. Voice strip | ✅ Achieved (processor + stub) | UVR1 / UVR2 / UVS1–UVS3 PASS, UVR1n / UVR2n / UVS1n fire; VC1–VC3 PASS. The 0.3 s hold by eye = HANDS-ON-3.3 rows 7–9. |
| 6. Env / LFO / clamp | ✅ Achieved | UME1, ULF1, UWS1 PASS (UWS1n fires); MV4 (NC fails), MV6, VC2. |
| 7. Meters | ✅ Achieved | UMT1 bit-exact, UMT2 (UMT2n fires), UMT3 exact SC levels, UMT4 (UMT4n fires); MT1 (NC fails); one reader by code read. Logic SC routing = HANDS-ON-3.3 row 15 + L1. |
| 8. `limit` / heavy hint | ✅ Achieved | ULM1 32 / 64 / 128 / 256; HV1. |
| 9. D49 | ✅ Achieved | ST1: 46 states, the 29 with a target hold at +150 and +450 ms (ST1 NC fails); UCM6b PASS, UCM6bn fires; UPM6b text in D45 wording. |
| 10. Unchanged / 0 allocs | ✅ Achieved | Harness 431 / 0, carried rows 0 missing; UAL2 0 allocs (liveness fires); parity 168 / 0 + GV 2 / 0; renders + legacy byte-identical; param-dump identical; drift 0 / 13; X2 0; `--cpu26` within gate. |
| 11. UI gates | ✅ Achieved | See Automated Checks. |
| 12. Hands-on ready | ⏳ Pending (human) | HANDS-ON-3.3 (18 rows) + HANDS-ON-SESSION-3.3 written; session not run. |

## Requirements Verification

**Stage:** stage-3. Requirements for this stage: 2 (UI-01, UI-02; both `should`).

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: v2 visual language; lanes with drag-drop, loop handles, scan playhead, grain dots | should | ⚠️ Partial (unchanged) | Automated complete at 3.2 verify. Open: HANDS-ON-3.1 + 3.2 (D47 session); Windows drop (Stage 4). |
| UI-02: drag-to-modulate onto knobs; MOD page shows instrument sources | should | ⚠️ Partial | Automated: every criterion proven on processor + stub page (DM1, DM2, USRC1, UMN1, UMB1, MV1–MV6). Open: real WKWebView drag and rings by eye (HANDS-ON-3.3 rows 1–6). |

REQUIREMENTS.md: UI-02 pending → **partial**. Both close to complete when their hands-on files are ✅. Totals: 6 complete, 17 partial, 2 pending, 0 failed.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness, param-dump, VST3, AU, Standalone) | ✅ Pass | rc 0 after the touch-rebuild; **0 warnings** in plugin sources. A full rebuild (SC-off / restore) shows 2 warnings outside plugin sources: JUCE VST3 SDK `funknown.h` (shadow) and `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp:192` (delete via non-virtual dtor). |
| build-and-install.sh | ✅ Pass | rc 0; only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Render harness | ✅ Pass | **431 PASS / 0 FAIL**, rc 0, 0 NOT FIRED, 183 controls fired. 29 `ug33` rows PASS. 3.3 NCs UVR1n, UVR2n, UVS1n, UWS1n, UMN1n, ULH1n, UMT2n, UMT4n, UCM6bn fire; UAL1 + UAL2 liveness fire; all earlier controls fire. |
| Carried rows vs 3.2 baseline (rebuilt from `1a3cbcf9`, 402 / 0) | ✅ Pass | 0 missing. Text differs on UTL32, UST1, UCN1, UPM6b (PD20, planned) and 5 measured-value rows: H4, RS3, SR1 (async-landing counts; RS3 / SR1 re-run alone on both binaries → identical 204 / 137088), CM1, USV2 (wall-clock). No result changed. |
| UPL2 / UPB2 (INFO) | ℹ️ | JSON 14,974 B (== SUMMARY); build 11.3 µs, encode 24.8 µs, text 528 µs (pull 564 µs < 1 ms; ran beside the parity build); voice loop 263 ns. |
| Parity (fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures, 332 outputs; **168 / 0**; GV **2 / 0**. |
| Render goldens | ✅ Pass | == `renders-2.4.sha256`; legacy == `renders-2.2.sha256` (14 / 14 each). |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv` (261). |
| Fork drift | ✅ Pass | 13 leaves, 0 drift. |
| UAL2 | ✅ Pass | 0 allocations (256 grains, 12 steals, per-voice LFOs, L / R peaks). |
| `--cpu26` (run alone) | ✅ Pass | **C25m 29.62 %** ≤ 40 (28.74–30.31); **C25tm 6.91 %** ≤ 12; C26rec 28.93 % (INFO). Higher than SUMMARY (27.54 / 6.47) but the no-routes baseline C25w rose with it (28.20 vs 26.38 at 2.6): C25m − C25w = +1.41 vs +1.47 at 2.6 — machine state, not the publish loop. |
| X2 | ✅ Pass | Shipping VST3 / AU / Standalone 0 (fresh and restored); harness control 10. |
| `generic-overrides.json` | ✅ Pass | 148,737 B, twice identical, == committed. |
| `ui_frontend_check.js` | ✅ Pass | ALL PASS **155**; 34 = 34; 176 / 70 / 15; [12] live encoder; [16] green. `--nc` NC1–NC4 each fail (NC4 → `[16] lfoPh`). |
| `ui_binding_probe.js` | ✅ Pass | 261 / 261 × 2; VE 14 / 14; lanes 12 / 12; mod 14 / 14; DM2 137 == 137; 0 page errors. `--nc` 6 / 6 fail on their own rows (s2_drive 260, PX2, MV4, DM2, MT1, ST1). |
| check-i18n · fr-lint · zh-lint | ✅ Pass | 0 · 0 · 0 (456 zh at `mt`). |
| check-ui-labels | ✅ Pass | `param-dump (261 rows) + overrides`, `states: default + 46`; 1977 / 0. |
| boot-all-uis `--strict-tips` | ✅ Pass | rc 0; 0 warn, 0 failed, 0 DEAD, 0 late (this plugin only). |
| auval `aumu OuGI OuDv` | ✅ Pass | SUCCEEDED `[1, 2] [2, 2]`, before and after restore; `ZZZZ` control rc 2. 4 pre-existing "Parameter did not retain default value" warnings on the LFO-rate params (layout unchanged since Stage 1). |
| pluginval 10 VST3, SC on | ✅ Pass | SUCCESS, 2 ch in, 0 `!!!`. |
| pluginval 10 AU, SC on | ✅ Pass | SUCCESS; one `!!!` "Current program is -1". |
| pluginval 10 VST3, SC off | ✅ Pass | SUCCESS, **0 ch in**, 0 `!!!`. |
| pluginval 10 AU, SC off | ✅ Pass | SUCCESS, same single `!!!`; 2 ch (AU wrapper, as before). |
| Restore + `auval -a` | ✅ Pass | Cache `OFF`; `aumu OuGI OuDv - Ouaricon Audio Development: O-GranulatorInstrument-dev`. |
| `cmp` | ✅ Pass | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js`. |
| `~/Music/Ouaricon/` | ✅ Pass | Absent before, after every run, at the end. |
| Git | ✅ Pass | `main` @ `77d5ad04`; plugin tree clean; no `O-GranulatorInstrument*` tag. |

Not run: Standalone smoke shot, quality gate (phase inside the stage).

## Human Verification (D47 — one session, `HANDS-ON-SESSION-3.3.md`)

- [ ] HANDS-ON-2.1–2.6 (79 rows)
- [ ] HANDS-ON-3.1 (16 rows)
- [ ] HANDS-ON-3.2 (27 rows; row 26 = fr `vstate.held` "tenue", your call)
- [ ] HANDS-ON-3.3 (18 rows). Highest value: 1–2 (real drag), 7 (0.3 s steal by eye), 12 (LFO dots), 15 (Logic SC L / R)
- [ ] Logic AU pass L1–L6

## Issues Found

None blocking. INFO:

- **Steal-ring overrun check is not a correct seqlock** (`GiTelemetry.h:196` `pushSteal`, reader `PluginProcessor.cpp:1229–1241`). The writer stores the serial before bumping the count, so a reader can see an overwritten serial with the old count and accept it. Only reachable with > 16 steals between two polls; effect is a newer real steal taken for an older one (one old hold lost). No test covers the overrun (UAL2 peaks at 12). Fix later: bump-then-write or a release fence before the slot store.
- **DM2 census == runtime can't fail on its own** (same selector both sides); the 125 / 125 destination coverage is what makes DM2's NC fail. Fine as is; don't drop the coverage clause.
- **`lfoPh` can serialize as 1.0** (4-decimal rounding of ≥ 0.99995), outside [0, 1); draws the same as 0 on a periodic shape.
- **Failed-take name reuse within one second** (SUMMARY open item; `UCM6b-same-slot` INFO row reproduces it).
- **4 auval LFO-rate default warnings** — pre-existing, not recorded in earlier verifies. Stage 4 check.
- Carried (D50, Stage 4): `WavFailed` silent after a take, one toast for several lanes, save worker join on a hung disk, `output_gain` default 7.15e-07 dB, Windows drop, blobs in presets, factory presets, float WAV.
- Deviations 1–14 in SUMMARY-3.3 reviewed and accepted (PD14 / PD20 planned).
- Foreign working-tree changes not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate independently verified green. The D47 hands-on session (79 + 61 rows + Logic 6) is pending (human).

**Stage 3 verdict:** automated complete across 3.1–3.3. Stage 3 closes (UI-01 / UI-02 → complete) when the session's files are ✅. A ❌ in a 2.x file reopens that DSP phase; a ❌ in 3.x routes to `/plugin-execute O-GranulatorInstrument 3-gui`.

**Blockers:** none on the automated side. Open: the D47 session.
