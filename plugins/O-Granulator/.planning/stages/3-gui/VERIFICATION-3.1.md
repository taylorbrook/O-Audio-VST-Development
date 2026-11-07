# Stage 3 — GUI · Phase 3.1 (Layout + controls + bindings) — Verification

## Verification Date

2026-11-06

Every check below was re-run in this verify session on a clean tree at `4526261e`. `git diff 4526261e HEAD` on `plugins/O-Granulator`, `modules` and `scripts` was empty, all six contract checksums matched the STATUS frontmatter, and `Source/ui/public/index.html` / `js/i18n.js` are byte-identical to `mockups/v2-ui.html` / `v2-i18n.js`. The harness and param-dump were touch-rebuilt first (0 warnings). No result here is copied from SUMMARY-3.1.md. Logs are in the session scratchpad `v31/`.

**Machine load note:** other sessions were running (load average ≈ 20). The harness wall time was 21 min 57 s (execute: 7 min 30 s). Only the C7 timing row is load-sensitive; it passed with 1 of 7 runs flagged contended (see below). Every other row is a deterministic verdict.

## Goal-Backward Analysis

### Original Goals (CONTEXT-3.1.md D26–D29 / PLAN-3.1.md success criteria)

1. `GranulatorEditor` builds and loads in Standalone / AU / VST3; fixed 1200 × 800; pluginval 10 + auval PASS.
2. 220 relays / attachments match APVTS by ID and TYPE (static closure, NC fails).
3. 21 = 21 native functions; no promise can hang (D26).
4. The page binds all 220 both ways under the C++-seeded stub (probe 220/220 ×2, NC fails).
5. Preset manager live: round trip incl. scale + masks + seed; `output_gain` untouched; absent custom state resets the scale, keeps the seed; two-reopen clean.
6. `getWindowCurve` is the engine's curve; telemetry fills the 3.1 fields, omits the rest.
7. All five UI gates green with overrides + the interaction states (D29).
8. Audio unchanged: carried diff empty, N-suites + R12 + HA PASS.
9. HANDS-ON-3.1.md run (UI-01 completes there).

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 Editor ships | ✅ Achieved | `build-and-install.sh` rc 0, **0 warnings**, VST3 + AU + Standalone (`O-Granulator-dev.app`) built. `setResizable (false, false)`, `setSize (1200, 800)`. auval SUCCEEDED, pluginval 10 VST3 + AU SUCCESS incl. "Open editor whilst processing" |
| 2 ID / type closure | ✅ Achieved | `ui_frontend_check.js` rc 0 (70 PASS, 0 FAIL): fresh param-dump == editor tables == page tables, 140 / 59 / 21, both directions. `--negative-controls` rc 0: NC1 (ID moved to `kToggleIds`) exit 3, NC2 (`getWindowCurve` removed) exit 2 |
| 3 Native functions | ✅ Achieved | 21 = 21 in the closure. Static read: every `launchAsync` callback hoists a `SafePointer` and bare-returns on a dead editor; `fileDialogOpen` is reset on every live path; the preset `savePresetWithDialog` routes inside-user-dir saves through the factory-guarded `savePreset` |
| 4 Two-way binding | ✅ Achieved | `ui_binding_probe.js` rc 0: **host → UI 220 / 220, UI → host 220 / 220**, page errors 0. `--nc` (bindKnob `s2_drive` removed) rc 2: 219 / 220 both ways, fails on exactly `s2_drive` |
| 5 Presets | ✅ Achieved | PM1 219 params 0 diffs, scale + 3 masks + seed equal, rev 1 → 6. PM2 −12.000 dB survives, no key in file. PM3 reset + seed 1234 kept. PM4 reopen1 equal, final equal, one child each, 0 live custom children. PM5 `fedcba9876543211` exact (file `-81985529216486895`). NCs PM2n 0.000 dB / PM3n seed 0 / PM5n 0x77 — each fails its predicate |
| 6 Window + telemetry | ✅ Achieved | WC1 worst 5.86e-7 vs closed-form Hann. WC2 5/5. TL31 18/18 (pitch 69.03, LFO1 0.263, macro2 0.630; 3.2 / 3.3 fields zero / NaN) |
| 7 UI gates | ✅ Achieved | overrides regenerated twice byte-identical, `git diff` clean; check-i18n 0 · fr-lint 0 · zh-lint 0; check-ui-labels rc 0, banner `seed=param-dump (220 rows) + overrides`, `states: default + 9`; boot-all-uis `--strict-tips` rc 0, 0 warn, 0 failed, 0 DEAD, 0 late |
| 8 Audio unchanged | ✅ Achieved | Harness **474 PASS, 0 FAIL**, rc 0. Carried extract == execute T11 extract exactly, and == B modulo the extract's whitespace trim / HR3 timing mask (no verdict or value moved). W2o **+0.483 dB** unchanged. N3 / N2 / N1 / N0 `--strict` + R12 rc 0, free + capped max\|Δ\| 0, every control differs. HA 79/79, 142 untouched, 79/79. Session-codec bytes (`codec_bytes.py`, installed VST3) identical to B on all 9 fixtures incl. second reopens |
| 9 Hands-on | ✅ Achieved | HANDS-ON-3.1.md A–G all PASS (Taylor, 2026-11-06). Buffer strip empty = §H expected 3.2 stub (NOTE) |

## Deviation Review (SUMMARY-3.1 deviations 1–10)

I read the processor / engine / editor diffs against `9831f498`.

- **1 — TL1 → TL31** (name clash with 2.5a TL1). **Accepted.**
- **2 — `createEditor` switched at T5** (consoles keep the generic editor under `JUCE_WEB_BROWSER=0`). **Accepted.**
- **3 — `--dump-ui-overrides` landed at T4, freed-temporary fix at T7.** Output byte-identical, re-confirmed ×2 here. **Accepted.**
- **4 — `codec_bytes.py`.** Strengthens P98 beyond HA; re-run here, identical to B. **Accepted.**
- **5 — Carried extract explicit** (C-timing detail dropped, HR3 prepare masked). Timing values aren't verdicts. **Accepted.**
- **6 — `has*` flags + shared `telemetryToVar`; `frozen` not emitted.** The page never reads it; ufc [12] pins the emitted ⊆ read key set. **Accepted.**
- **7 — 9 states (8 + matrix); eval clicks for two states.** More coverage than planned. **Accepted.**
- **8 — PM3n callback swap; PM5n JSON number; preset seeds accepted only as strings.** Matches P99. **Accepted.**
- **9 — Override sliders carry `label`.** **Accepted.**
- **10 — check-ui-labels coverage 341 / 2344.** 2000 unmeasured are closed-select matrix options (not visible), and 4 are stream-gated readouts. **Accepted, carried:** add an s2–s4-enabled state once counts are live (3.3).

Static notes (no action): the P98 refactor moves `resetNonParameterState()` after the `uiLanguage` read. That is safe because the reset doesn't touch `uiLanguage`, and the codec-byte gate confirms it. `readTelemetry` reads routing from the APVTS raw values (message thread, copy-only), and `Engine::windowCurve` is const on the immutable bank.

## Requirements Verification

**Stage:** stage-3, Phase 3.1 of 3.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: All parameters reachable; per-stream controls | must | ✅ Complete (partial → complete) | Layer 1 static closure, layer 2 220 × 2 probe, layer 3 real-WebView hands-on (relay ↔ attachment, host automation, preset recall, two reopens) all PASS |
| UI-02: Buffer waveform / grains | should | ⏸️ Deferred | Phase 3.2 |
| UI-03: Mod matrix view + mod amount on targets | should | ⏸️ Deferred | Phase 3.3 (routing + modNorm already live in 3.1) |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | brief `f9c748a8`, requirements `ad7cb8dd`, draft `d0b1457e`, param-spec `f5ce3595`, architecture `f0cf753b`, roadmap `1060dae0` |
| Page == v2 mockup | ✅ Pass | `cmp` clean ×2 |
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness (default) | ✅ Pass | **474 PASS, 0 FAIL**, rc 0 |
| C7 CPU | ✅ Pass | median **16.95 %** (15.75–18.16, spread 2.41, 1 contended — machine load ≈ 20); execute 17.12 / 17.20 %. Accepted on the PASS. The witness calibration harness item (2.6 decision c) stays carried |
| Carried diff | ✅ Pass | == T11 exactly; == B modulo extract whitespace / HR3 mask |
| N3 / N2 / N1 / N0 `--strict` | ✅ Pass | rc 0 ×4, free + capped max\|Δ\| 0; N3p-THR / N3p-Z / N2p identical; all Nn controls differ; N3 liveness > 0 |
| R12 | ✅ Pass | free + capped max\|Δ\| 0 |
| param-dump | ✅ Pass | `git diff --exit-code .planning/params.tsv` clean |
| `generic-overrides.json` | ✅ Pass | 59 combos / 140 sliders / 199 defaults; byte-identical ×2; diff-clean |
| `ui_frontend_check.js` + NCs | ✅ Pass | 0 failures; NC1 exit 3, NC2 exit 2 |
| `ui_binding_probe.js` + NC | ✅ Pass | 220 / 220 ×2; NC 219 / 220, rc 2 |
| check-i18n · fr · zh | ✅ Pass | 0 · 0 · 0 |
| check-ui-labels | ✅ Pass | rc 0; `+ overrides`; `default + 9` |
| boot-all-uis `--strict-tips` | ✅ Pass | 0 warn, 0 failed, 0 DEAD, 0 late |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0, 0 warnings, caches cleared; `-dev` VST3 + component installed |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED; Tail Time PASS |
| pluginval 10 VST3 / AU (in-process) | ✅ Pass | SUCCESS ×2; editor test ran; taillength inf |
| HA host-cache (installed VST3) | ✅ Pass | HA1 79/79, HA2 142 untouched, HA3 79/79 |
| Session-codec bytes | ✅ Pass | 9 / 9 fixtures identical to B |
| modules / scripts / page vs Stage 2 | ✅ Pass | no changes |

## Human Verification — HANDS-ON-3.1.md

Run by Taylor, 2026-11-06 (Standalone + DAW): **all items PASS.**

| Section | Verdict |
|---|---|
| A Load and look (A1–A6) | ✅ PASS |
| B UI → host (B1–B10) | ✅ PASS |
| C Host → UI (C1–C4) | ✅ PASS |
| D Presets (D1–D5) | ✅ PASS |
| E Two reopens (E1–E2) | ✅ PASS |
| F Robustness (F1–F3) | ✅ PASS |
| G Hands-on §9 (G1–G9) | ✅ PASS |

Notes:
- **Buffer strip shows no waveform or grains.** Expected 3.1 stub (§H): the wave ring and grain seqlock are Phase 3.2 scope (UI-02). NOTE, not a defect.
- **Reverb seemed on by default.** It was not a defect: `reverb_mix` defaults to 0 % (spec, processor, page), and at 0 % the tank never runs (R35). The instance carried a value from an earlier test (Taylor). Closed with no change. Side observation, logged for 3.3 polish only if wanted: the "fig. decay" figure draws at 25 % amplitude when mix is 0 (`index.html` `drawReverbFigure`, `0.25 + 0.75·mx`).

## Issues Found

- **None blocking** in the automated set.
- **Carried:** an s2–s4-enabled i18n state (3.3, deviation 10); the DAW pass (D27, 3.3 verify); Env 1e30 pin → Stage 4; harness items C7 calibration + Z6a event-aware `renderPool`; W2o rule (widen seeds, never the threshold).

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 3.1). Every automated gate passes on an independent re-run, the hands-on passes, and deviations 1–10 are accepted.

**Ready for next phase:** Yes — Phase 3.2 (wave ring + grain seqlock → buffer display, UI-02).

**Blockers:** none.
