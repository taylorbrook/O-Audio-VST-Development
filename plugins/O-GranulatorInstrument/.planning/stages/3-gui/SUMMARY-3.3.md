---
plugin: O-GranulatorInstrument
stage: 3
phase: "3.3"
title: "MOD page, voice section, meters: 17 sources, drag-to-modulate, rings and loop ticks from the newest voice, mono badges, 16-row voice strip with releases and steals, env / LFO views, wantSt clamp, output + SC meters L / R, limit"
executed: 2026-11-11
version: 0.1.0 (unchanged, PD28; no tag)
commits: C1 0389000c (engine / recorder / loader / processor + 3.3 encoder + ui_gates_33 + carried amendments + overrides) · C2 bbbc6de3 (LFO-dot defect + editor notes; validators) · C3 34f8e83e (closure [12] / [15] / [16], probe --mod-only rows, 46 states, zh-Hans clamp fit fix) · C4 (this summary, HANDS-ON-3.3, HANDS-ON-SESSION-3.3, STATUS)
result: executed — every automated gate green; Stage 2 audio unchanged; HANDS-ON-3.3 + the one-session checklist written for verify (D47)
---

# O-GranulatorInstrument — Stage 3 / Phase 3.3 SUMMARY

The MOD page and the voice section are live. All 17 sources come from the engine (`srcG`). Drag-to-modulate works from every dock chip onto every knob and loop handle. Rings and loop-edge ticks follow routed-only `modNorm`, and the mono badge rule equals the engine's predicates. The voice strip shows 16 rows keyed by voice serial, with releases, pedal holds and steals (300 ms hold). The env scopes, the LFO dots (now on the curve at the phase offset), the `+48` clamp marker and the grain warn bit are all live. Output and sidechain meters read L / R, max since read. The grain caption uses the true `limit`. Everything rides the one ~30 Hz `getTelemetry` pull through `buildUiTelemetry`. The three 3.2 carry-overs (D49) are fixed. Stage 2 audio did not move. UI-02 is closable at verify; no requirement was moved here.

## What was built

- **Engine (T2, C1; audio-neutral).** `GiUiVoice` + `state / serial / menv[2] / menvStage[2] / ampStage / lfoPh[4] / wantSt[4]`, `GiUiFrame::newestSerial` (80-byte voice, 13,152-byte frame; trivially copyable). Steal ring (16 × u64 + `stolenCount`, release store) written only in `stealVoice` before `forceReleaseVoice`. `outPeakL / R` + `takeOutPeaks` replace `outPeak / takeOutPeak`. `lfoPerVoiceActive (n)` evaluated once per n outside the voice loop. Test accessors `srcPerVoiceNowForTest / destVoiceNowForTest` (hooks only).
- **Recorder + loader (T3, C1).** `pkL / pkR` beside the unchanged threshold accumulator; `takeScPeaks`. Take file = `getNonexistentSibling (false)` **before** the write branch (CM6, D49b); `legacyTakeName` hook re-enacts the old order.
- **Processor (T4, C1).** `GiVoiceRowMap` (pure, clock injected, hold 300 ms as ctor argument) beside `GiRecRunHold`; steal cursor; `UiTelemetry` 3.3 (`kVoices` 16); `buildUiTelemetry` 3.3: row map, `newest` (counted rows only), `menv`, `lfoPh` (always, `[global, row0…15]`), row column 8 = first enabled stream with a tape, grain bit 2 from the engine slot's `wantSt`, `sources` = `srcG`, routed-only `modNorm`, lane `lo / hi` only when a loop dest is routed, `limit = 32 << grain_limit`, `out [L, R]`, `rec.peak [L, R]`. Still the only reader of `frames.read`, `takeOutPeaks`, `takeScPeaks` and the steal cursor.
- **Encoder (T4, C1).** Live `telemetryToVar (UiTelemetry)` emits the PD13 shape (14 keys; lane 3 + `lo / hi`; 8 rec keys). The 3.1 placeholder is frozen (PD14); UTL31 untouched and passing.
- **Page (T7 C2, T12 C3; mirrored in `v1-ui.html`).** LFO dot Y = `lfoShapeAt (shape, (pp + ph0) % 1)` (PD16). One zh-Hans CSS line for the clamp marker (D48, see deviation 9).
- **Editor (T7, C2).** Comments only: the consuming feeds `buildUiTelemetry` owns.
- **Proof (T5, T9–T11; C1 / C3).** `ui_gates_33.h` (29 rows), carried amendments, 3.3 overrides fixture, closure 128 → 155 checks with [16], `--mod-only` probe group (14 rows), states 38 → 46.

## Gate results (final tree)

| Gate | Result |
|---|---|
| Quality gate | not re-run (phase inside the stage; passed at 3.1) |
| Harness | **431 / 0** (402 carried + 29 new), 0 NOT FIRED; carried row names vs baseline **B**: 0 missing; UTL32 / UST1 / UCN1 amended (PD20, PASS), UPM6b text. Same at T6 and T13 |
| Parity | **168 / 0** + GV **2 / 0** (fresh parent build at the pin, 0 self-check failures; T1, T6, T13) |
| Renders | renders-2.4 + legacy **byte-identical** after T2, T3, T4, T5, at T6 and on the final tree |
| param-dump | **identical** to `.planning/params.tsv` (261) |
| Fork drift | **0 / 13** |
| Allocation | UAL2: 0 on the audio thread (256 grains, 16 voices, 12 steals, per-voice LFOs, both peak splits); liveness control fired. UAL1 unchanged, PASS |
| CPU `--cpu26` | **C25m 27.54 %** ≤ 40 (margin 12.46); **C25tm 6.47 %** ≤ 12; C26rec 27.86 % (INFO) — unchanged vs 2.6 (27.85 / 6.55) |
| Test directories | real `~/Music/Ouaricon/O-GranulatorInstrument/` absent before and after every run |
| Build | VST3 + AU + Standalone + consoles rc 0, **0 warnings in plugin sources** |
| auval | PASS (`aumu OuGI OuDv`), twice (T8, T13) and after the restore |
| pluginval 10 | VST3 SUCCESS (0 `!!!`) + AU SUCCESS (1 `!!!`, "Current program is -1"); **SC on (2 ch) and gate variant off (VST3 0 ch; AU reports 2, the wrapper, as before)**; cache restored to `OFF`, shipping reinstalled. T8 and T13 |
| X2 | 0 test-hook symbols in the installed VST3 and AU (control: harness 8 by the same pattern) |
| `generic-overrides.json` | 148,737 bytes; twice byte-identical; diff-clean at T12 |
| `ui_frontend_check.js` | ALL PASS (**155** checks); [12] on the live encoder; [16] green; 4 / 4 NCs FAIL (new NC4: `lfoPh` removed → [16]) |
| `ui_binding_probe.js` | **261 / 261** both ways; VE **14 / 14**; lanes **12 / 12**; mod **14 / 14**; DM2 census **137 / 137** targets (== runtime selector count), 125 / 125 destinations; 6 / 6 NCs FAIL, each on its own row |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 |
| check-ui-labels | ALL CHECKS PASSED; `param-dump (261 rows) + overrides`, `states: default + 46` |
| boot-all-uis `--strict-tips` | completes; 0 DEAD, 0 late |
| `cmp` | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js` |

### New harness rows (`--ug33`, 29 PASS)

UVR1 (515 counted observations, 0 missing, 0 row moves; 20 polls with a counted voice in slot 16–19, shown every time), UVR2 (79 / 79; −1 on all 6 polls after All Sound Off with a stolen row held), UVS1 (victim state 3 on 10 polls from +32 to +320 ms, empty at +352 ms; new note in its own row — the steal is on a block start, so the 3 comes only from ring + hold), UVS2, UVS3 (shrink / All Sound Off / Mono add 0; 2 steals add 2), UWS1 (column 8 = 50; bit 2 on 16 / 16 stream-1 grains, 0 / 16 key-track-0 grains), UME1 (161 / 161 bit-equal), ULF1 (global 164 / 164, per-voice 71 / 71), USRC1 (918 / 918; Key / Bend / Slide signs), UMN1 (key set {1, 2, 3, 119}; per-voice dest at base on 51 / 51 voiceless polls), ULH1 (41 / 41; lanes 2–4 never carry `lo / hi`), UMB1 (6,375 / 6,375 pairs), UMT1 (125 / 125 bit-exact; the processor writes nothing after `Engine::process`), UMT2 (R / L −400 dB, L −8.96 dBFS), UMT3 (0.5 and 0.1 exact, 997 Hz in bounds), UMT4, ULM1 (32 / 64 / 128 / 256), UCM6b (14 seeded names incl. this second's; failed take `…S1_2.wav` collides with none), UAL2, UTL33.
Negative controls, each FIRED: UVR1n, UVR2n, UVS1n, UWS1n, UMN1n, ULH1n, UMT2n, UMT4n, UCM6bn + the liveness malloc inside UAL2. All 3.1 / 3.2 controls still fire.

INFO: **UPL2** widest JSON 14,974 bytes (256 grains, 16 rows, 16 `modNorm` keys); per pull build 9.6 µs, encode 21.5 µs, JSON text 466 µs (≈ 0.50 ms, under the 1 ms line). **UPB2** voice loop at 20 non-free slots 30 → 242 ns per block (+212 ns, 0.004 % of a 256-sample block; harness re-enactment).

### Probe rows (`--mod-only`, 14 PASS)

VC1–VC3, MV1–MV6, DM1 (17 chips × knob + loop handle by real pointer sequences; depth 40), DM2 (137 drops), MT1, HV1, ST1 (46 states in file order; the 29 that name a target hold at +150 and +450 ms, ≥ 13 polls each).

## Deviations (each with its reason)

1. **PD14 (planned):** the 3.1 placeholder is frozen; [12] / [15] read the live encoder.
2. **PD20 (planned):** UTL32 asserts the 13 3.2 keys are *present* (not an exact set) and ignores lane `lo / hi`; its 3.3-owned asserts moved to UTL33; grain flags compared on bit 0. UST1 `== kVoices` — its title / detail also say 16 rows (slightly beyond PD20). UCN1 asserts `limit == 32 << grain_limit` and saw 256 on 751 frames, 32 on 188. UPM6b text uses an ASCII "- D45" (`juce::String (const char*)` is ASCII-only).
3. **NC hooks added in T4** (all `OGI_TEST_HOOKS`): `testSetStealHoldMs`, `testRowMapIdentity`, `testClampFromStream0`, `testModNormAll`, `testLoHiAlways`, `testPeaksMax`, `testPeaksNoExchange` (+ `GiRecorder::peekScPeaksForTest`).
4. **Row-map rules the plan left open:** a stolen serial stays state 3 and its hold starts once; a stolen voice without a row takes only an empty or expired row (never evicts); a counted voice evicting a stolen-held row drops its slot mapping (that voice's grains → `voice −1`). Ring reader: acquire load, relaxed copies, acquire fence, re-read, discard `c2 − c ≥ 16`; drained on every poll incl. stale ones.
5. **Range guards:** `grain_limit` index clamped 0–3; `lo / hi` fall back to 0 / 1 on a non-finite value. Frame constants `kUiLfos 4 / kUiModEnvs 2` with `static_assert`s in `Engine.cpp`, so `GiTelemetry.h` does not include the forked `ControlGrid.h`.
6. **param-dump reference is `.planning/params.tsv`**, not `tests/golden/params.tsv` (a spec extract in another format), as at 3.2 verify.
7. **Mono routes do not rest at base.** A per-voice source on a global destination reads the engine's held `srcG`, so its ring stays at the last value after the last voice. UMN1 tests "rests at base" on a per-voice destination (R§5). CONTEXT-3.3's sentence "with no voice sounding, the dot sits at the base value" is true only for per-voice destinations; HANDS-ON-3.3 row 5 + Known limits say so.
8. **i18n states beyond PD18 wording:** `write-held`, `write-per-lap`, `stream-off-head` run on the FX page (where `#write-*` / `#fxcount-s2` live); `streams-2-4-on` sets `grain_limit` 3 through its state before asserting `#heavy.on` (check-ui-labels' state pass cycles every combo, so the index is 1 at that point) and asserts stream II is off first; `__giState` gained real-click `c.toggle`, `c.slider`, `c.page` and extra targets, and records `window.__giTargets` for ST1; `rec-recording-live` also checks `{n: 'IV'}`.
9. **D48 page defect (C3):** check-ui-labels [4] zh-Hans — `vstate.clamp` "+48 半音 · 限制" is 70.8 px in the 63 px `.vcell .tr` line (no state showed it before `voice-clamp`). Fix: `html:lang(zh-Hans) .vcell .tr.clamp { left: 0; right: 0; font-size: 10px; }`, both HTML files; no i18n key changed.
10. **Closure / probe choices:** [12] also checks every `frames.read / takeOutPeaks / takeScPeaks / stolenCount.load` sits inside `buildUiTelemetry`; [16] runs the page's own `isPerVoiceSource / isGlobalDest` text against `ui_gates_33.h`'s `pageRule` (34,000 pairs, all 16 LFO-mode combinations); the class gate derives 14 ids + 4 id-less nodes from 60 functions reachable from the poll, and checks itself against the three 3.2 vacuous states (4 / 4). DM2 also requires every destination covered; the ST1 NC appends the old state at the end (at its old position the fixture's `run: 2` shows the same key); MV1 reads the inline chip colour (transition); MV6 checks dot-on-curve for attack and sustain only (decay / release drawn on a linear approximation, v1 by design).
11. **T8 smoke** is one window shot of the default page (1202 × 858 pt, "No signal", grains 0 / 256, heavy hint on, no page error). The MOD and VOICE pages were not navigated in the real WebView (needs synthetic clicks on a machine in use); HANDS-ON-3.3 covers them.
12. **T1 closure baseline** not re-run on the unmodified tree (the T2–T4 agent had started editing the C++ it reads); carried from VERIFICATION-3.2 (128 ALL PASS on the same code).
13. **Sign-off (meter reading):** no answer from Taylor; executed on the plan default (UMT1–UMT4 + MT1).
14. Agents did not append to `.claude/agent-memory/*` (foreign dirty paths).

No audio-moving change, no contract edit, no module / `scripts/` / forked-file / `CMakeLists.txt` edit, no new param, no timer / `emitEvent`.

## Open items for verify / later phases

- **Hands-on (D47):** one session at 3.3 verify per `HANDS-ON-SESSION-3.3.md` — 2.1–2.6 (79), 3.1 (16), 3.2 (27, incl. row 26 fr "tenue"), 3.3 (18), Logic AU pass (6).
- **Not exercised by anything automated:** real WKWebView pointer capture across pages, DPR 2 pixel rows, Logic side-chain routing, the 0.3 s steal hold seen by eye.
- **Failed-take name reuse:** a successful take on the same slot within the same second can take a failed take's intended name (that file never existed). Only a session / preset saved inside that second could point at it. Left as INFO.
- **Stale `data-i18n-vars` on `#vtr-N`** after a clamp clears: nothing reads it, no gate fails.
- Carried (D50, Stage 4): `WavFailed` silent after a take, one toast for several failing lanes, save worker joined without timeout on a hung disk, `output_gain` default 7.15e-07 dB, Windows drop / browser check, blobs in presets, factory presets, float WAV for hot takes.

## Requirements

None moved in 3.3. UI-01 and UI-02 close at verify once the session's hands-on files are ✅.
