# Stage 3 — GUI · Phase 3.2 (live lanes, file I/O UI, Record-in UI) — Verification

## Verification Date

2026-11-10

I re-ran every automated check below myself in the verify session, against `1a3cbcf9`. The plugin tree was clean before and after.
- I touched `PluginProcessor.cpp`, `PluginEditor.cpp`, `Engine.cpp`, `SlotLoader.cpp` and `index.html`, rebuilt the harness, param-dump, VST3, AU and Standalone (rc 0, **0 warnings**), and reinstalled with `build-and-install.sh`.
- I re-extracted and re-built the parity reference from the pin into a fresh scratch dir.
- Dorico 6 was open during the validators (not hosting this plugin), as during execute. No Logic session.

No automated result here is copied from SUMMARY-3.2.md. HANDS-ON-3.1 (16 rows) and HANDS-ON-3.2 (27 rows) are human gates and are **not** self-certified.

## Goal-Backward Analysis

### Original Goals (CONTEXT-3.2 / PLAN-3.2 success criteria)

1. Playheads for every sounding voice and grain dots by stream (position, pan, envelope, length) from the engine frame; streams 1-based on the page.
2. Feedback head and record head track the engine; overview redraws while feedback writes; `dirty` ticks only where the tape changed, cleared by Reset.
3. Record-in panel from engine state (No sidechain / no signal / Armed / Recording / Committing / Done / Discarded); Done / Discarded fall back after 4 s.
4. `clearSlot` empties a lane through the handoff; refused while that lane records.
5. `saveRecording` writes the working copy as 24-bit stereo WAV; completes on every path including `timeout` (D46).
6. Drops moved to Imports under the file hash, referenced as `file:`, keep their name, survive a session round trip; a failed drop leaves the lane and toasts (D44).
7. A preset replaces only the lanes it has a source for (D45).
8. `encodedBytes` exact after an embed.
9. One reader per consuming feed; 0 audio-thread allocations.
10. Stage 2 and 3.1 unchanged.
11. Five UI gates green with the new states; both file pairs `cmp` clean.
12. HANDS-ON-3.2 ready (run with HANDS-ON-3.1, D43).

### Deliverables (SUMMARY-3.2.md)

C1 `7a446ab5` (engine / loader / processor surface, `ui_gates_32.h`, overrides), C2 `c13119a0` (editor natives, page defects, two toast keys), C3 `399fd122` (closure + probe rows, 38 states, fr `vstate.held` fit fix), C4 `1a3cbcf9` (docs).

Code read in verify:
- `Source/dsp/GiTapeSnapshot.h` whole file and `Engine::serviceTapeSnapshot` (`Engine.cpp:830`): the audio thread leaves Requested / Copying only by compare-exchange, so a cancel that lands mid-copy always ends in `Cancelled`; Aborted on tape pointer, frame count **or** tape epoch change; one relaxed load per block when idle; read-only on the tape. Served in `process` and `bypassedBlock`.
- `PluginProcessor.cpp` save flow (`saveRecordingTo`, `saveWorkerRun`, `runSaveJob`, `reclaimSaveBuffer*`): buffers freed only after `release()` succeeded or via `resetAudioStopped()` with audio stopped and the worker joined (destructor joins first). A timeout without acknowledgement sets `saveLent` and keeps the buffer; the next save answers `busy` until the late ack arrives. Guards `notRecording` / `empty` / `busy` / `inUse` in plan order. The PD18 lifetime invariant holds as written.
- `clearSlot`: refused (`recording`) only while Recording / Committing into that slot; Armed allowed. `restoreSlots (…, clearUnlisted)`: presets pass `false` (D45), session restore keeps clearing.
- Scope audit `git diff --stat 6ea16b17 HEAD`: nothing outside `plugins/O-GranulatorInstrument/`; nothing under `Source/dsp/forked/`; `CMakeLists.txt` untouched; contract hashes (spec, spec body, brief, requirements, draft, architecture, roadmap) == STATUS frontmatter.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Playheads + grain dots | ✅ Achieved (processor + stub) | UGD1 / UGD2 / UPH1 / UCN1 PASS with UGD1n / UPH1n / UCN1n firing. Probe PX1 (playhead at the expected column, none for `scan −1`), PX2 (stream 1 → lane 1, stream 4 → lane 4); **NC 0-based rows FAILS PX2**. Real WebView with held notes = HANDS-ON rows 7–9. |
| 2. Feedback / record head, overview, dirty | ✅ Achieved (processor + stub) | UFB1, UOV2 PASS (UFB1n, UOV2n, UOV2n-b fire). PX3 feedback head, PX4 record head + tint, PX5 dirty ticks; LN6 refetch on `ovRev`. Frozen (dashed) head = HANDS-ON row 11. |
| 3. Record-in panel | ✅ Achieved (processor + stub) | URC1 (run sequence + 4,000 ms hold, URC1n fires), URC2 (signal hold, capped = recorder truncation). LN1 / LN2 (13 bus / run / signal / trigger states in page precedence). Real sidechain = HANDS-ON rows 13–17. |
| 4. `clearSlot` | ✅ Achieved | UCL1 PASS; guard read in code; LN3 calls with 1-based slot. Clear during a live take = HANDS-ON row 18. |
| 5. Save recording | ✅ Achieved (processor) | USV1 (0 / 120,000 diffs vs the tape through the 24-bit writer; guards; swap → `changed`), USV2 (`timeout` → `busy` → late ack → save ok; reclaim in `prepareToPlay`; destroyed with the buffer lent). LN5 toast on `timeout` / `writeFailed`, none on `cancelled`. Native chooser and Logic idling = HANDS-ON rows 19–23. |
| 6. Durable drops | ✅ Achieved (processor) | UIM1 (UIM1n fires), UIM2 (3 / 3 failure kinds leave the lane), UIM3 (stream XXH64 == one-shot, superseded import leaves no `.part`). LN4 `toast.loadFailed` on change only. Real WKWebView drop = HANDS-ON rows 1–6. |
| 7. D45 preset rule | ✅ Achieved | UPM6c PASS, **UPM6cn fires** (an empty preset entry no longer clears); UPM6 / UPM6b PASS. Hands-on row 24. |
| 8. `encodedBytes` | ✅ Achieved | UEB1 PASS. |
| 9. One reader, 0 allocs | ✅ Achieved | UAL1: **0** audio-thread allocations with 256 grains publishing and a snapshot copying (9 chunks, save ok, 144,000 frames); its liveness malloc control fires. `buildUiTelemetry` is the only reader of the frame and `takeScPeak` (code read). |
| 10. Stage 2 / 3.1 unchanged | ✅ Achieved | Harness **402 / 0** (374 carried + 28), 0 NOT FIRED; parity 168 / 0 + GV 2 / 0; renders-2.4 + legacy byte-identical; param-dump identical; drift 0 / 13; X2 0; probe 261 × 2 + VE 14. |
| 11. UI gates | ✅ Achieved | See Automated Checks. `cmp` both pairs clean. |
| 12. HANDS-ON-3.2 | ⏳ Pending (human) | 27 rows written; not run. HANDS-ON-3.1 (16 rows) also pending (D43). |

## Requirements Verification

**Stage:** stage-3. Requirements for this stage: 2 (UI-01, UI-02; both `should`).

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: v2 visual language; lanes with drag-drop, loop handles, scan playhead, grain dots | should | ⚠️ Partial | Automated: every element proven on processor + stub page (PX1–PX5, LN1–LN7, UGD / UPH / UIM rows). Open: real WKWebView drop and on-screen playheads / dots with held notes (HANDS-ON-3.2 rows 1–9), Windows drop (Stage 4, deferred by CONTEXT-3.2). |
| UI-02: drag-to-modulate; MOD page sources | should | ⏸️ Pending (3.3) | Not in 3.2. |

REQUIREMENTS.md: UI-01 pending → **partial**. It moves to complete when HANDS-ON-3.1 + 3.2 pass (Windows criterion stays a Stage 4 item). Totals: 6 complete, 16 partial, 3 pending, 0 failed.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness, param-dump, VST3, AU, Standalone) | ✅ Pass | rc 0 after the touch-rebuild; **0 warnings**. |
| build-and-install.sh | ✅ Pass | rc 0 (shipping, SC-off variant, restore). Only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Render harness | ✅ Pass | **402 PASS / 0 FAIL**, rc 0, 0 NOT FIRED. 45 U-rows PASS. 13 `n` rows each fire (UGD1n, UCN1n, UPH1n, UFB1n, UOV2n, UOV2n-b, URC1n, UPM6cn, UIM1n + the four 3.1 controls); UAL1's control fires. CM6 PASS this run. |
| UPL1 / UPB1 (INFO) | ℹ️ | Widest telemetry 13,710 B; `buildUiTelemetry` 6.5 µs, `telemetryToVar` 22.7 µs, JSON text 495 µs per pull (under the 1 ms report line; harness ran beside the parity build). |
| Parity (fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures, 332 outputs; **168 / 0**; `--rows GV` **2 / 0**. |
| Render goldens | ✅ Pass | `--dump-render-hashes` == `renders-2.4.sha256`; legacy == `renders-2.2.sha256`. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv` (261). |
| Fork drift | ✅ Pass | 13 leaves, 0 drift. |
| X2 symbol exclusion | ✅ Pass | 0 `ogran::` / test-hook symbols (`loadSlotSync`, `…ForTest`) in the installed VST3 and AU; control: the harness has 10 by the same pattern. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. `ZZZZ` control rc 2. |
| pluginval 10 VST3, SC on | ✅ Pass | SUCCESS, rc 0, 2 ch in, 0 `!!!`. |
| pluginval 10 AU, SC on | ✅ Pass | SUCCESS, rc 0; only `!!!` is "Current program is -1". |
| pluginval 10 VST3, SC off (`OGI_GATE_SC_DEFAULT_OFF=ON`) | ✅ Pass | SUCCESS, rc 0, **0 ch in**, 0 `!!!`. |
| pluginval 10 AU, SC off | ✅ Pass | SUCCESS, rc 0, same single `!!!`; reports 2 ch in (AU wrapper, as at 2.6 / 3.1). |
| Restore + auval | ✅ Pass | Cache back to `OFF`, shipping build reinstalled (rc 0, 0 warnings); auval PASS `[1, 2] [2, 2]`; `auval -a` lists `aumu OuGI OuDv`; X2 0 / 0 on the restored binaries. |
| `generic-overrides.json` | ✅ Pass | Generated twice, byte-identical, == committed (146,298 B). |
| `ui_frontend_check.js` | ✅ Pass | ALL PASS (128 checks), rc 0. `--nc`: NC1, NC2, NC3 (telemetry key renamed) each FAIL. |
| `ui_binding_probe.js` | ✅ Pass | 261 / 261 both ways; VE 14 / 14; lanes 12 / 12 (LN1–LN7, PX1–PX5); 0 page errors. `--nc`: fails at 260 (`s2_drive`); 0-based grain rows fail PX2 only. |
| check-i18n · fr-lint · zh-lint | ✅ Pass | 0 · 0 · 0 (456 zh entries at `mt`, counted). |
| check-ui-labels | ✅ Pass | rc 0; `seed=param-dump (261 rows) + overrides`; `states: default + 38` (file has 38). |
| boot-all-uis `--strict-tips` | ✅ Pass | rc 0; 0 warn, 0 failed, 0 DEAD, 0 late. |
| `cmp` | ✅ Pass | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js`. |
| Recordings / Imports guard | ✅ Pass | `~/Music/Ouaricon/` absent after every run. |
| Git | ✅ Pass | `main`; no uncommitted plugin change; no `O-GranulatorInstrument*` tag. |

Not re-run: `--cpu26` (no audio-path change: renders byte-identical; UAL1 covers the new block-path code's allocation side).

## Human Verification

- [ ] HANDS-ON-3.1 rows 1–16 (`stages/3-gui/HANDS-ON-3.1.md`) — folded in here (D43).
- [ ] HANDS-ON-3.2 rows 1–27 (`stages/3-gui/HANDS-ON-3.2.md`). Highest value: rows 1–4 (real drop → Imports → reopen → relink), 7–9 (playheads / dots on screen), 13 / 15 (record head, Discarded hold), 20 (Save with Logic idling the track), 26 (French "tenue" — wording is yours).
- [ ] HANDS-ON-2.1–2.6 (79 rows): 3.3 verify (D39).

## Issues Found

None blocking. For your attention:

- **`vstate.held` fr "tenue" sits on the glossary's forbidden list** (reserved for Sustain) and carries a `termNote`. Your call at HANDS-ON-3.2 row 26.
- **Three vacuous i18n states** (`rec-wait-level`, `rec-no-sidechain`, `rec-recording`): the live poll overwrites their labels within 200 ms. Live replacements exist beside them; remove or rewrite the three at 3.3.
- **CM6 timing flake** (2.6 code): one-second take-name resolution can collide with CM5's take. Passed here; fix the naming or the row before it bites a gate run.
- **UPM6b's printed note** ("a `slots` array clears unlisted slots") is now wrong under D45 — cosmetic, the row's predicate is right.
- **X2 control count differs from SUMMARY** (10 here vs 31): different grep pattern, same conclusion (shipping 0, harness > 0).
- Carried, unchanged: `WavFailed` after a take stays silent (Stage 4); one toast element for several failing lanes; a save that throws in `callNative` logs only; save worker joined without timeout on a hung disk at close; `output_gain` default 7.15e-07 dB; Windows drop check (Stage 4).
- Deviations 1–18 in SUMMARY-3.2 reviewed and accepted. Deviation 6 (snapshot served in `bypassedBlock`, epoch check) is a correctness gain over the plan.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, O-GrainScatter) not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green. HANDS-ON-3.1 (16) + HANDS-ON-3.2 (27) are pending (human).

**Ready for next phase (3.3):** Yes on the automated evidence. A ❌ in either HANDS-ON file routes to `/plugin-execute O-GranulatorInstrument 3-gui`.

**Blockers:** none. Open human gates: HANDS-ON-3.1 + 3.2; HANDS-ON-2.1–2.6 (3.3 verify).
