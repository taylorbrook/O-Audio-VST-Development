---
plugin: O-GranulatorInstrument
stage: 3
phase: "3.2"
title: "Live lanes, file I/O UI, Record-in UI: playheads, grain dots, feedback and record heads, evolving overview, Clear, Save recording, durable drops, preset slot rule"
executed: 2026-11-10
version: 0.1.0 (unchanged, PD30; no tag)
commits: C1 7a446ab5 (engine / loader / processor surface + harness rows + overrides) · C2 c13119a0 (editor natives + page defects + two toast keys) · C3 399fd122 (closure + probe rows, 38 states, fr fit fix) · C4 (this summary, HANDS-ON-3.2, STATUS)
result: executed — every automated gate green; Stage 2 audio unchanged; HANDS-ON-3.2 written for verify (run with HANDS-ON-3.1, D43)
---

# O-GranulatorInstrument — Stage 3 / Phase 3.2 SUMMARY

The lanes are live. The page now draws scan playheads per voice, grain dots by stream (position, pan, envelope, length), the feedback head, the record head and the evolving overview with dirty ticks, all from one processor-side telemetry builder. The Record-in panel follows engine state, Clear and Save recording work, drops are moved into an Imports folder and survive a session round trip, and a preset replaces only the lanes it has a source for. Stage 2 audio did not move. UI-01 is closable at verify; no requirement was moved here.

## What was built

- **Engine (T2–T3, C1; audio-neutral).**
  - `GiUiGrain` carries `gainL, gainR, phase, lenSamples, windowStep, tiltT / A / B` (field copies in `publishTelemetry`; `GiUiFrame` 3,960 → 12,024 bytes). `Engine::grainWindow` reads the immutable morph bank on the message thread.
  - `GiTelemetry::ovWrites[4]`, bumped once per `foldOverview` call that had a tape.
  - `Source/dsp/GiTapeSnapshot.h`: the engine owns the request, the caller owns the buffers. The audio thread copies at most 16,384 frames of level 0 per block at the end of `process` (and in `bypassedBlock`), and leaves Requested / Copying only by compare-exchange, so a cancel is always acknowledged (`Cancelled`).
- **Loader (T4, C1).** `requestClear` / `clearLocked`; `restoreSlots (slots, clearUnlisted = true)`; `requestImport` as a worker job (streaming XXH64 of the whole file, `Imports/<16 hex>.<ext>`, `.part` then rename, display name carried, failure leaves the lane as it was and sets `lastError`); `setImportsDirectory`; `SlotInfo::encodedBytes`; `gihash::Xxh64Stream`.
- **Processor (T5, C1).**
  - `buildUiTelemetry (nowMs)` behind `telemetryLock`: the only reader of the frame and the sidechain peak. Stale frame at 250 ms, `GiRecRunHold` 4,000 ms, signal hold 2,000 ms, all on an injected clock.
  - Pan = clamp (grain pan + stream pan, −1, 1); the frame's `gainL / gainR` do not contain the stream pan, so the reader adds `sN_pan` once.
  - `ovRev` = `2·(generation + published) + loaded + ovWrites`. Live bins replace pristine bins in `getSlotOverview`; `dirty[i]` = `min (255, round (255·d / 2))` (`kDirtyFullScale` 2.0; the page lights a tick above 5 %, i.e. from byte 13). `slots[i].dirty` is an exact compare, re-evaluated at most every 100 ms, and is bit 7 of the slot fingerprint.
  - Tempo atomics (bpm, bar length) stored once per `processBlock`; `rec.capped` uses `GiRecorder::takeLengthFrames`, the function the recorder itself calls.
  - `clearSlot` (refused with `recording` while the recorder is Recording or Committing into that slot), `importDroppedFile`, `saveRecordingTo` (one persistent worker; guards `notRecording`, `empty`, `busy`, `inUse`; 2,000 ms no-progress limit; 24-bit stereo WAV at the tape rate via `.part`; completion on every path).
- **Editor (T8, C2).** `getTelemetry` → `buildUiTelemetry`; `getSlotState` with real dirty flags; `clearSlot`; `saveRecording` (chooser first, `busy` on a second click, `SafePointer` on each hop); `onCommitFile` → `importDroppedFile`. Still 34 natives, no timer, no `emitEvent`, no `unavailable` answer.
- **Page (T9, C2; mirrored in `v1-ui.html`).** `fetchOverview` stores the answer's `rev`; `toast.loadFailed` when `slots[i].error` changes to non-empty (not for an error already there at the first pull); `toast.saveFailed` on a failed save other than `cancelled`; scan-mode `aria-valuetext` re-localized in the `init()` language wrapper. Two toast keys × three languages.
- **Proof (T6, T11, T12; C1 / C3).** `tests/render-harness/ui_gates_32.h` (28 rows), PD24 guard, 3.2 overrides fixture, closure 99 → 128 checks, 12 probe rows, 19 → 38 states.

## Gate results (final tree)

| Gate | Result |
|---|---|
| Quality gate 2-dsp → 3-gui | not re-run (phase inside the stage; passed at 3.1) |
| Harness | **402 / 0**, 0 NOT FIRED; carried rows vs baseline **B**: diff empty apart from CM6 (see deviation 1) and the UPM6 title; 28 new rows PASS; 10 `n` rows FIRED. Same extract at T7 and T14 |
| Parity | **168 / 0** + GV **2 / 0** (fresh parent build, T1, T7 and T14) |
| Renders | renders-2.4 + legacy **byte-identical** after T2, T3, T4, T5, at T7 and on the final tree |
| param-dump | **identical** (261) |
| Fork drift | **0 / 13** |
| Allocation | UAL1: 0 on the audio thread with 256 grains in the frame and a snapshot copying |
| Test directories | real `~/Music/Ouaricon/O-GranulatorInstrument/` absent before and after every run; PD24 guard returns 254 when the Imports redirect is removed |
| Build | VST3 + AU + Standalone + consoles rc 0, **0 warnings in plugin sources** (the two known warnings outside the plugin remain: VST3 SDK `funknown.h`, `NoteExpression_VST3.cpp:192`) |
| auval | PASS (`aumu OuGI OuDv`) |
| pluginval 10 | VST3 SUCCESS (0 `!!!`) + AU SUCCESS (1 `!!!`, "Current program is -1"), **sidechain on (2 ch) and off (VST3 0 ch)**; shipping build restored and reinstalled. Run at T10 and again at T14 — T14 result: see "T14 validators" below |
| X2 | 0 test-hook symbols in the installed VST3 and AU (control: harness 31) |
| `generic-overrides.json` | 146,298 bytes; twice byte-identical; equal to the committed file |
| `ui_frontend_check.js` | ALL PASS (128 checks); 34 = 34; no `unavailable`; all 3 NCs FAIL |
| `ui_binding_probe.js` | **261 / 261** both ways; VE **14 / 14**; lanes **12 / 12** (LN1–LN7, PX1–PX5); NC at 260 and NC 0-based grains (PX2) both FAIL |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 (456 zh entries at `reviewed: 'mt'`) |
| check-ui-labels | ALL CHECKS PASSED; `seed=param-dump (261 rows) + overrides`, `states: default + 38` |
| boot-all-uis `--strict-tips` | completes; 0 DEAD, 0 late |
| `cmp` | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js` |

### T14 validators

Final tree, installed shipping build: auval PASS; pluginval 10 VST3 SUCCESS (0 `!!!`, 2 ch in) + AU SUCCESS (1 `!!!`); gate variant `OGI_GATE_SC_DEFAULT_OFF=ON`: VST3 SUCCESS (0 ch in) + AU SUCCESS (reports 2 ch, the AU wrapper, as at 2.6 / 3.1); cache restored to `OFF` (0 gate-variant warnings), shipping build reinstalled, auval PASS again, VST3 reads 2 ch in, `auval -a` lists `aumu OuGI OuDv`, X2 0 / 0. The first configure of the gate variant printed a transient GitHub fetch error (FetchContent, `irlba`); ninja regenerated and the variant built (0 ch in confirms it).

### New harness rows (`--ug32`, 28 PASS)

UGD1 (64 / 64 dots bitwise against the pool and the spawn log), UGD2 (pan / gain / env / length; stream pan added once), UCN1 (939 / 939 frames equal to the census), UPH1, UFB1, UOV2 (2,250 folds, 141 / 141 `ovRev` moves, 0 wrong dirty peaks, `slotRev` flips with the flag, Reset clears), URC1 (0→1→2→3→4→0 and 0→2→5→0; hold 4,000 ms), URC2 (signal hold; capped equals the recorder's truncation at two tempi), UCL1, UPM6c, UIM1, UIM2 (3 / 3 failure kinds leave the lane as it was), UIM3 (28 / 28 stream == one-shot; equal `contentHash`, distinct names; superseded import leaves no `.part`), USV1 (0 / 120,000 diffs against the tape through the same 24-bit writer; guards; swap → `changed`), USV2 (`timeout` at about 2,104 ms → `busy` → resume acknowledged → save ok; reclaim in `prepareToPlay`; destroyed with the buffer lent), UEB1, UST1, UAL1, UTL32.
Negative controls, each FIRED (10 in `--ug32`): UGD1n, UCN1n, UPH1n, UFB1n, UOV2n, UOV2n-b, URC1n, UPM6cn, UIM1n and the control inside UAL1 (liveness malloc). The four 3.1 controls (UPM2n, UPM3n, UPM5n, USR1n) still fire.

INFO: **UPL1** widest telemetry 13,710 bytes of JSON at 256 grain rows; per pull `buildUiTelemetry` 4.1 µs, `telemetryToVar` 19.2 µs, JSON text 438 µs (about 0.46 ms, below the 1 ms report line). **UPB1** publish loop 314 ns → 520 ns per block at 256 slots (a harness re-enactment; the old frame code is gone).

## Deviations (each with its reason)

1. **Baseline B has one FAIL: CM6 failed once in the T1 full run** (373 / 1) on the unmodified tree, and passed in three `--eg26` re-runs and in both later full runs (402 / 0). The take file name has one-second resolution (`GI-Rec-<stamp>-S1.wav`), so CM5's take in the same wall second can leave a file under the name CM6 expects to be missing. Treated as a pre-existing timing flake in a carried row, not as a pre-flight mismatch; I did not stop for it. Not fixed (the row and the naming are 2.6 code).
2. **`vstate.held` French changed from "enfoncée" to "tenue"** (D40 defect, found by check-ui-labels as soon as real voice rows reached the Voice strip: the label overlapped the voice number by 2.9 px in the 67 px cell). "tenue" is on the French glossary's forbidden list (reserved against the Sustain stage), so the entry carries a `termNote`. **Wording for Taylor to confirm** (HANDS-ON-3.2 row 26).
3. **Three existing states are vacuous and were left in place:** `rec-wait-level`, `rec-no-sidechain`, `rec-recording` set labels with `__setLabel`, and the live poll overwrites them within 200 ms (measured). Three stub-driven states were added beside them (`…-live`), so `rec.waitLevel`, `rec.noSidechain` and `rec.capped` are measured again. The plan forbade altering the 19.
4. **Carried checks edited:** UPM6 (`ui_gates_31.h`) now loads lanes 2 / 4 and asserts they survive the preset (title tail changed); closure check [12] requires the 3.2 `getTelemetry` call. UPM6b's printed note ("a `slots` array clears unlisted slots") is now out of date.
5. **Save chooser starts beside the take**, not in a resolved Recordings folder: the processor has no shipping accessor for it. For a take made on this machine that is the Recordings folder.
6. **Snapshot also served in `bypassedBlock`** (a Save completes while bypassed) and the abort test also compares the tape epoch (a new tape at a recycled address). One extra state `Arming` makes `request()` safe against two callers.
7. **`requestClear` is wider than PD14:** it also cancels a load in flight on an empty lane and supersedes pending imports. `alreadyEmpty` answers ok.
8. **Import details:** `loading` stays false during hash and move and goes true at takeover; a decode failure after takeover bumps the generation like any failed load; a different-size file already at the target is replaced; `buildAndPublish` writes the carried name into the pristine so a rate-change rebuild keeps it.
9. **`GiRecRunHold` lives in `PluginProcessor.h`** (the processor owns it; `GiUiJson.h` includes that header). A change between two Idle words into a non-hold reason leaves a running hold alone.
10. **Builder choices beyond the PD wording:** `fb` also requires a first block; `rec.slot` is 0 before any take; `rec.capped` keys on `sync_mode` only, bpm falls back to 120; a stale frame's first observation counts as a move.
11. **Overrides fixture is part real, part hand-built:** revisions, `ovRev`, `fb`, live overview and `dirty` come from a real run on a pinned seed; the voice, the 10 grains, counts and the rec block are written into the `UiTelemetry` struct and passed through `telemetryToVar`. Grains sit on streams 3 / 4 although those slots hold no tape.
12. **UCN1 stimulus** is two tape swaps into sounding slots (a `grain_limit` drop produces no killing grains in the frame).
13. **Audio-neutral checkpoints were taken after each of T2, T3, T4, T5** by the executing agent, as planned. T9 was written beside Wave 1 and committed in C2; T11 / T12 ran beside T10.
14. **T10 smoke is one window shot of the default page** (1200 × 800, rendered, Record-in reads "No signal" from real telemetry, lanes empty). Loading the tone and holding a note needs synthetic input on a machine in use; playheads and dots in the real WebView are HANDS-ON rows 7–9.
15. **The restore step of the T10 validator run failed once** (network outage during the FetchContent update of the reconfigure) and left the gate variant installed; it was reconfigured, rebuilt, reinstalled and re-checked (2 ch in, auval PASS, X2 0) before C2. Dorico 6 was open during the validators (not hosting this plugin).
16. **USV2 was not run under ASan** (no sanitizer variant is configured for the harness).
17. **Probe additions:** `--lanes-only`, a `MutationObserver` on `#toast` installed before the page's first script, PX4 threshold 90 / 255 (the same stub edit toggles the tint).
18. Agents appended lines to `.claude/agent-memory/gui-agent.md` (a foreign dirty path; never staged).

No audio-moving change, no contract edit, no module / `scripts/` / forked-file edit, no design change beyond the PD8 / PD17 / PD21 page edits and deviation 2.

## Open items for verify / later phases

- **Not exercised by anything automated:** the real WKWebView drop into Imports, the native Save chooser, Save with the host idling the audio thread, `callAsync` completion delivery in a host, two editors polling, devicePixelRatio 2 for the pixel rows, the frozen (dashed) feedback head on the page, `encodedBytes` → `toobig` class. HANDS-ON-3.2 covers the first four.
- **`WavFailed` after a take stays silent** (PD17; Stage 4).
- **A save that throws in `callNative`** shows no toast (console only).
- **Several lanes failing in one pull** show one toast (one toast element).
- **Hung disk write at plugin close:** the save worker is joined without a timeout; only file I/O is unbounded.
- **CM6 flake** (deviation 1): give the take name sub-second resolution or make the row tolerate a name collision.
- **Vacuous states** (deviation 3): remove or rewrite the three `__setLabel` recorder states.
- Carried from 3.1: `output_gain` default 7.15e-07 dB; Windows drop / browser check (Stage 4).

## Requirements

None moved in 3.2. UI-01 is ready to close at verify once HANDS-ON-3.1 + HANDS-ON-3.2 are run.

## HANDS-ON-3.2

`stages/3-gui/HANDS-ON-3.2.md`, 27 rows + the list of expected 3.2 placeholders. Run by Taylor at 3.2 verify together with the 16 rows of HANDS-ON-3.1 (D43).
