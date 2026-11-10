---
title: "O-GranulatorInstrument Stage 3 (GUI) — Phase 3.2 Execution Plan"
created: 2026-11-10
juce_version: "8.0.15"
phase: "3.2"
summary: "Five waves: (W0) pre-flight, Stage 2 + 3.1 baselines, UI gates on the unmodified tree; (W1) engine / loader / processor surface, each step checked audio-neutral: grain frame extension + overview write counter, tape snapshot copy-out, loader clear / import / preset flag / encodedBytes, processor telemetry builder + live overview + save flow, JSON encoders, harness rows ui_gates_32 + overrides dump, commit C1; (W2) editor natives live (telemetry, clearSlot, saveRecording, durable drop) + page defects and two toast keys, build / install / validators, commit C2; (W3) UI proof: overrides, closure rows, lane probe + pixel rows, i18n states, five gates, commit C3; (W4) regression, HANDS-ON-3.2, SUMMARY / STATUS, commit C4. Version 0.1.0, no tag, no new params, no forked-file edit, no module edit."
stages: [3]
agents: [gui-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 3 / Phase 3.2 Execution Plan (lanes, file I/O UI, Record-in UI)

## Goal

The lanes go live and UI-01 becomes closable at verify: scan playheads per voice, grain dots by stream (position, pan, envelope, length), the feedback head, the record head, the evolving overview with `dirty` ticks, the Record-in panel driven by engine state, `clearSlot`, `saveRecording` (D46), durable drops (D44), and the preset slot rule (D45). **Stage 2 audio does not move.**

**Out of scope:** MOD sources, mod rings, mono badges, voice strip (`newest`, `menv`, `wantSt`), meters (`out`), modulated loop region (`lanes[].lo / .hi`), `grain_limit` hint (3.3); HANDS-ON-2.1–2.6 and the Logic DAW pass (3.3 verify, D39); Windows drop check, blobs in presets, factory presets, `output_gain` default (Stage 4).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/3-gui/CONTEXT-3.2.md` | D43–D46, defaults, constraints. **Binding, not reopened.** |
| `stages/3-gui/RESEARCH-3.2.md` | Summary 1–13 (cited **RSx**), §1–§12 (**R§x**). Line numbers there are from `fa89eff7`. |
| `stages/3-gui/PLAN-3.1.md`, `SUMMARY-3.1.md` | PD7, PD8, PD10–PD13, PD18–PD21 carry; deviations 2, 3, 8 matter here (`ovRev` formula, fingerprint, language-apply wrapper in `init()`). |
| This plugin | `Source/{PluginProcessor.h,PluginProcessor.cpp,PluginEditor.h,PluginEditor.cpp,GiUiJson.h}`, `Source/dsp/{GiTelemetry.h,GiRecorder.h,SlotLoader.h,SlotLoader.cpp,SlotHandoff.h,SlotTape.h,MipTape.h,GiHash.h,Engine.h,Engine.cpp}`, `Source/dsp/forked/{Grain.h,UiTelemetry.h,WindowMorphBank.h}` (read only), `Source/ui/public/{index.html,js/i18n.js}`, `tests/render-harness/{main.cpp,ui_gates_31.h,engine_gates_26b.h}`, `tests/{ui_frontend_check.js,ui_binding_probe.js,i18n-states.json}`. |
| Parent (read, never edit) | `plugins/O-Granulator/`: `Source/PluginProcessor.cpp:670-790` (telemetry reader, `telemetryLock`, grain derivations, stale zeroing), `.planning/stages/3-gui/{PLAN,SUMMARY}-3.2.md`, harness GD / CN / TB / PL rows, its 3.2 closure rows. |
| Repo | `scripts/{check-i18n,i18n-fr-lint,i18n-zh-lint,check-ui-labels,boot-all-uis,serve-ui}.js`, the French glossary, `modules/core/webview-drop-streaming/cpp/{WebViewDropStreaming.h,DropSessionGuard.h}` (read only). |

Plan-time facts (2026-11-10): last code commit under `Source/ tests/ CMakeLists.txt` = **`6ea16b17`**. `PluginProcessor.h` 396 / `.cpp` 1094, `PluginEditor.cpp` 1095 (`onCommitFile:554`, `clearSlot:793`, `saveRecording:849`), `GiUiJson.h` 316, `GiTelemetry.h` 150 (`GiUiGrain:54`, `GiUiVoice:61`), `SlotLoader.h` 354 / `.cpp` 1468, `Engine.cpp` 4034, `GiHash.h` 208 (`xxh64:90`), `index.html` 5073 (== `mockups/v1-ui.html`, `cmp` clean), `i18n.js` 3215, harness `main.cpp` 1458, `ui_gates_31.h` 989, `i18n-states.json` 19 states. Harness 374 / 0. **Line numbers drift — re-grep.**

---

## Sign-off (Taylor, 2026-11-10, at plan)

| Item | Decision |
|---|---|
| R§12.4 lane during capture | **Record head + tint only.** No capture-overview feed. The new waveform appears at commit through the normal publish. |
| R§12.2 Save read path | **Audio-thread chunked copy-out.** Save answers `timeout` when the host is not calling `processBlock`; the user retries with the track live. |

## Plan-phase decisions (resolve R§12)

- **PD1 [R§12.1] Grain frame extended.** `GiUiGrain` gains `gainL, gainR, phase ((age + frac) · invLen), lenSamples, windowStep, tiltT, tiltA, tiltB`; `gain` stays the kill gain. The audio thread copies fields only, no math beyond `phase`. `stream` stays 0-based in the frame. Derivation on the reader side, as the parent (`P/Source/PluginProcessor.cpp:769-786`):
  - `pan` from `atan2 (R, L) · 4/π − 1`, exactly 0 when `L == R`; the executor checks whether `gainL / gainR` already contain the stream pan and never adds it twice (UGD2 pins it);
  - `gain = sqrt ((L² + R²) / 2)`; `env` = bank window at (`windowStep`, tilt, `phase`) × kill gain through a new `Engine::grainWindow (…) const noexcept` over the immutable morph bank; `lengthSec = lenSamples / fs`;
  - `stream` emitted **1-based**; `flags` emitted = reversed bit only. The engine's killing bit is never sent (the page reads that value as "pitch clamp").
- **PD2 [R§12.9] Voices.** 20 rows `[state, note, amp, scan1..4, 0]`; `state = 1` when `note ≥ 0`, else 0; `scan = −1` for a stream whose slot has no tape; no releasing bit (3.3). `newest` −1, `menv` `[]`. The amp figure starts showing level dots as a side effect (accepted, R§2).
- **PD3 Counts.** `counts[s]` = frame grains of stream s that are neither killing nor in a fade slot; `limit` 0.
- **PD4 Stale frame.** When the frame's `seqHead` has not moved for ≥ 250 ms, send `voices` as 20 free rows, `grains` `[]`, `counts` zeros. Clock injected (`nowMs` argument), never read inside the builder.
- **PD5 Feedback head.** `lanes[i].fb = clamp (headPos[i] / frames, 0, 1)` when the slot has a tape and (the head is not parked, or `freeze` is on with that lane's feedback > 0); else `null`. `lo / hi` omitted.
- **PD6 Overview revision.** `GiTelemetry::ovWrites[kTapeSlots]` (`std::atomic<uint32_t>`, relaxed), incremented once per `foldOverview` call; the engine never reads it. `ovRev` = the 3.1 formula (SUMMARY-3.1 dev. 2) + `ovWrites[slot]`.
- **PD7 [R§12.6] Live overview + `dirty`.** `getSlotOverview` keeps the pristine fold and replaces a bin pair with `tel.overview.read (…)` when that returns true. `dirty[i]` = max over the peak's bins of `|liveLo − pristLo| + |liveHi − pristHi|`, sent as `min (255, round (255 · d / 2))`; the executor confirms the scale against the page's 5 % tick threshold (I:4260) and records the constant. `slots[i].dirty` = any bin differs (real test, not "any live bin"): evaluated inside the telemetry build, per slot only when `ovWrites` moved and at most 10 Hz, cached; the flag is **folded into the slot fingerprint** so `slotRev` moves when it flips (the page pulls slot state on `slotRev`, not on `ovRev`).
- **PD8 Page fix: `fetchOverview` stores the answer's `rev`** (R§3). D40 defect class.
- **PD9 [R§12.10] Record-in mapping and hold.** R§4 table. A small pure struct (`GiRecRunHold`, in `GiUiJson.h` or beside it) takes `(packed, nowMs)` and returns `run`: an observed change into Idle + {Committed} → 4, Idle + {NoSignal, TooShort} → 5, held **4000 ms**, then 0; any non-Idle state ends the hold; a first observation that is already Idle starts no hold. `rec.slot = slotTel + 1`; `pos = framesTel / targetTel` (0 when target is 0), `len = targetTel / fs`; `lanes[slot].rec = pos` only while `run == 2`, else `null`; `rec.bus` as 3.1.
- **PD10 Signal.** `takeScPeak()` (post-gain) is consumed once per telemetry build; `rec.signal` = the peak reached `GiRecorder::kPresenceLin` within the last 2000 ms (timestamp kept by the builder); `rec.peak = [p, p]`. No other consumer anywhere (3.3's meter reuses this value).
- **PD11 [R§12.5] `rec.capped`.** Two relaxed atomics on the processor (bpm, bar length in quarter notes), stored once per `processBlock` from the transport snapshot. `capped` = Tempo length mode, `run != 2`, and the take length for the current `rec_bars` exceeds `F_max`. The length comes from **the same function the recorder uses** (extract it to a shared static if it is inline today); never a second formula.
- **PD12 One telemetry builder on the processor.** `buildUiTelemetry (double nowMs)` (message thread) holds `telemetryLock` (two editors, R§10), is the **only** caller of `frames.read()` and `takeScPeak()`, owns the PD4 / PD9 / PD10 state, and returns a plain struct that `GiUiJson.h::telemetryToVar` encodes. The editor calls it with `Time::getMillisecondCounterHiRes()`; the harness calls it with a scripted clock on its own processor instance. Floats rounded to 4 decimals in the encoder.
- **PD13 `getTelemetry` 3.2 shape.** The PD8 object of PLAN-3.1 with: real `lanes[].fb / .rec`, `voices` (PD2), `grains` rows `[stream, normPos, pan, env, gain, lengthSec, voice, flags]`, `counts`, real `rec.*`. Unchanged for 3.3: `newest`, `menv`, `limit` 0, `out` `[0,0]`, `sources` `[]`, `modNorm` `{}`. Absent stays `null`, never 0.
- **PD14 `clearSlot`.** `SlotLoader::requestClear (slot)`: the `w == nullptr` branch of `restoreSlots` extracted to `clearLocked (s)` and called from both; clears when `loaded || !source.isEmpty()` (a harness-loaded buffer without a source clears too). The processor wraps it: refuse with `"recording"` when the recorder is Recording or Committing and `slotTel == slot`; Armed is allowed. Native answers `{ ok }` / `{ ok: false, error: "recording" | "badSlot" }`.
- **PD15 D45.** `restoreSlots (slots, bool clearUnlisted = true)`; `applyPresetCustomState` passes `false`. Session restore unchanged. UPM6 keeps its name and now asserts the D45 behaviour for the unlisted lanes (slot 2 / 4 untouched); UPM6b unchanged.
- **PD16 [R§12.7] D44 import = loader worker job.** `SlotLoader::requestImport (slot, tempFile, displayName)`:
  1. streaming XXH64 over the whole file (new `gihash::Xxh64Stream` in `GiHash.h`, 64 KiB reads; never a one-shot read of a 256 MiB file);
  2. target `Imports/<16 hex>.<ext lower-cased>`; exists with the same size → reuse, delete the temp file; else move the temp file to `<target>.part` (`moveFileTo`), then rename to the target;
  3. continue as a `file:` load of the target with `name = displayName` (name override on the load path; `SavedSlot::name` already persists);
  4. any failure → no load, the slot keeps its tape and source, `lastError` set, the `.part` and the temp file removed; **never** a temp-path source. A superseded or cancelled import removes its `.part` too.
  `setImportsDirectory` beside `setRecordingsDirectory`; default `…/O-GranulatorInstrument/Imports`, resolved lazily, no directory I/O in a constructor. The folder button keeps referencing files in place.
- **PD17 [R§12.3] Failure toasts: two new keys.** `toast.loadFailed` and `toast.saveFailed`, both with `{ n }`, en / fr / zh-Hans (`reviewed: 'mt'` for zh), glossary checked, lints green. `toast.loadFailed`: the page shows it when `slots[i].error` changes to non-empty on a state pull (covers failed drops and failed browser loads). `toast.saveFailed`: shown when `saveRecording` answers `ok: false` with anything but `cancelled`. **One key more than RESEARCH proposed**: a Save that can time out by design (sign-off) must not fail silently. `WavFailed` after a take stays silent (the take is live and embedded by default, D36); recorded as an open item.
- **PD18 [R§12.2] Tape snapshot (engine).** One request object owned by the processor and handed to the engine: `{ slot, L, R (preallocated to SlotInfo::frames), frames, cursor, progress counter }` behind one atomic state (Idle → Requested → Copying → Done | Aborted; Cancel requested from the worker).
  - The audio thread tests one relaxed atomic per block when idle; when active it copies at most **16,384 frames** of level 0 per `processBlock`, bumps the progress counter, and aborts (`changed`) when the slot's tape pointer or frame count differs from the one captured at Requested → Copying. No allocation, no lock.
  - **Buffer lifetime invariant:** the buffer is never freed while the audio thread may still hold it. It is released only after the audio thread acknowledged Done / Aborted / Cancel, or in `prepareToPlay` / `releaseResources` / the destructor. After a timeout it may stay allocated until audio resumes; a new save meanwhile answers `busy`.
- **PD19 Save flow (processor).** `saveRecordingTo (slot0, file, completion)`:
  - guards in order: slot scheme is `rec` (`notRecording`), nothing loaded (`empty`), a save in flight (`busy`), target equals the slot's own `rec:` file or any path another slot references (`inUse`);
  - a single background worker allocates the buffer, raises the request, waits on the state with a **2000 ms no-progress** limit (progress counter, not wall time since the start), then writes WAV **24-bit, stereo, tape rate** (`.wav` forced; samples above ±1.0 clip, R§12.8 accepted), to a `.part` sibling then rename;
  - completion on the message thread on every path: `{ ok: true, path }` or `{ ok: false, error: cancelled | busy | notRecording | empty | inUse | changed | timeout | writeFailed }`; the worker is stopped before the engine is destroyed;
  - no `juce::Timer`, and nothing depends on a page call arriving. The harness drives it through a sync hook under `OGI_TEST_HOOKS`.
  - Editor: chooser first (save mode, `warnAboutOverwriting`, starts in the Recordings folder, default `<take stem>-edit.wav`), then `saveRecordingTo`; `SafePointer` on each async hop; a second click while a chooser or save is open answers `busy`.
- **PD20 `encodedBytes`.** `SlotInfo::encodedBytes` set where a blob is installed (`publishResult`, `runEncode`, `restoreSlots`) = decoded size from the base64 length minus padding; 0 without a blob; cap + 1 when `tooLarge`. No `getBlob` copy on a state pull.
- **PD21 Scan-mode `aria-valuetext`** re-localized after a language apply: `updateChoiceKnobVisual` over `CHOICE_KNOB_IDS` inside the existing `init()` wrapper, not in the canon region.
- **PD22 Page and i18n stay mirrored.** Every page edit (PD8, PD17 reader, PD21) lands in `Source/ui/public/index.html` **and** `mockups/v1-ui.html`; the two keys land in `js/i18n.js` **and** `mockups/v1-i18n.js`. `cmp` clean on both pairs at exit. No other page change except a defect a gate finds (D40, recorded, mirrored).
- **PD23 Harness rows `tests/render-harness/ui_gates_32.h`** (processor only, `U` prefix, names grepped for collisions first; `--ug32`). R§9 table, with these additions:
  - **UGD1 / UGD2 / UCN1 / UPH1 / UFB1 / UOV2 / URC1 / UCL1 / UPM6c / UIM1 / UIM2 / USV1 / UEB1** as R§9, each with the listed negative control (`n` suffix; must FAIL its predicate). Add **UFB1n** (freeze on, "not parked" rule alone → `fb` null) and **UOV2n-b** (dirty flag not folded into the fingerprint → `slotRev` static).
  - **UST1** stale frame: no `processBlock` for 300 scripted ms → empty voices / grains / counts; resumes on the next block.
  - **URC2** `rec.signal`: true within 2 s of a peak above `kPresenceLin`, false after; `rec.capped` true / false across the `F_max` boundary at two tempi, equal to the recorder's own truncation of a real take.
  - **UIM3** streaming XXH64 == `gihash::xxh64` one-shot for lengths 0, 1, 31, 32, 33, 65,535, 65,536, 65,537 and a 3 MiB buffer; two equal-length files with identical first 131,072 frames and different tails land under different names. A superseded import leaves no `.part`.
  - **USV2** lifetime: timeout with audio stopped → `timeout`, a second save → `busy`, audio resumes → acknowledged, a third save succeeds; run under the harness's ASan variant if one is configured, else recorded as not run.
  - **UAL1** 0 allocations on the audio thread with the extended frame publishing 256 grains and a snapshot copying (existing allocation hook).
  - **UTL32** `telemetryToVar` key set == PD13; `fb` / `rec` null when absent; `counts` four numbers; grain rows eight numbers, stream in 1…4.
  - **INFO UPL1** widest JSON bytes and build cost at 256 grains; **INFO UPB1** `publishTelemetry` cost before / after the frame extension.
  - Row **TL2** (`engine_gates_26b.h`) is re-read for a frame-size assumption; a carried row may be touched only to keep it compiling, its name and verdict must not change, and the edit is a recorded deviation.
  - Expected harness total **374 + new rows**; the executor records the number.
- **PD24 Test directories.** Recordings **and** Imports are redirected (path with a non-ASCII component) before any processor work in every harness mode; the existing guard (`main.cpp`, 2.6 PD17) is extended to Imports and fails the run if either resolves under the real `~/Music/Ouaricon/`.
- **PD25 Overrides.** `--dump-ui-overrides` regenerated with a 3.2 fixture from the real encoders on a scripted clock: one held voice, 2–3 grains per stream, `fb` on lane 1, lane 2 recording (`run` 2, `pos` 0.4), live `dirty` on lane 1, one slot with `error`. Still LF, no clock, no absolute path, twice byte-identical, committed, diff-clean on every gate run.
- **PD26 Closure rows (`ui_frontend_check.js`).** "Emitted and read" for `fb, rec, voices, grains, counts, rec.run / slot / pos / len / signal / capped / peak, dirty, encodedBytes, error`; no `"unavailable"` literal left in `PluginEditor.cpp`; 34 = 34 unchanged; both new toast keys present in three languages and referenced by the page. Existing NCs keep failing; one new NC: a telemetry key renamed in the encoder FAILS.
- **PD27 Probe rows (`ui_binding_probe.js`).** Counts unchanged (261 × 2, VE 14). New rows on per-row stub answers:
  - **LN1** recording class on the target lane only; **LN2** each `rec-status` class for `run` 0–5 and `bus` 0, in the page's precedence order; **LN3** Clear / Save / Relink call their natives with the 1-based slot, Save absent on a `file:` lane; **LN4** `toast.loadFailed` appears when `error` turns non-empty and not again while it is unchanged; **LN5** `toast.saveFailed` on `timeout`, none on `cancelled`; **LN6** `fetchOverview` refetches when `ovRev` moves and stores the answer's `rev`; **LN7** scan-mode `aria-valuetext` is French after a switch to fr;
  - **PX1–PX5** pixel rows on the stub page, one per canvas feature: a playhead, a grain dot of stream 1 and of stream 4 (the 0-based / 1-based class), the feedback head, the record head, a dirty tick — a non-background pixel at the expected x within ± 2 px, and background where the feature is absent.
  - **NC:** the stub's grain rows sent 0-based → PX stream row FAILS.
- **PD28 i18n states.** Add the states R§9 lists (`rec.armed`, `rec.waitNote`, `rec.committing`, `rec.done`, `rec.discarded`, `rec.noSignal`, `rec.slotN`, the `lane.recording` / `lane.missing` hints, toasts `loading`, `dropOneFile`, `tooLarge`, `slotReset`, `allReset`, `loadFailed`, `saveFailed`), driven through stub telemetry / slot state where the page path allows, `__setLabel` only for widest-string checks. Each state throws if its node is not visible. The runner banner must equal the file's count; the executor records it.
- **PD29 Gate order:** regenerate overrides (diff-clean) → `ui_frontend_check.js` → `ui_binding_probe.js` → check-i18n → fr-lint → zh-lint → check-ui-labels → boot-all-uis `--strict-tips`. Exit 77 = not run = stop.
- **PD30 Housekeeping.** Version **0.1.0**; **no tag**. No new params; param-dump byte-identical. No edit to `forked/*`, `modules/*`, `scripts/*`, contracts. `PluginProcessor.h` stays free of WebView types. Test hooks under `OGI_TEST_HOOKS` only. Four path-scoped commits C1–C4 on `main`; `git branch --show-current` + `git status --short` immediately before each; never `git add -A`; foreign dirty paths (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, `plugins/O-GrainScatter/*`) never staged; `Source/ui/public/modules/` never committed.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/dsp/GiTelemetry.h` | `GiUiGrain` fields (PD1); `ovWrites` (PD6) |
| `Source/dsp/Engine.h/.cpp` | frame copy (PD1), `grainWindow` reader, `ovWrites` bump in `foldOverview`, snapshot copy-out in the block path (PD18) |
| `Source/dsp/GiTapeSnapshot.h` (new) | request object + state (PD18) |
| `Source/dsp/GiHash.h` | `Xxh64Stream` (PD16) |
| `Source/dsp/SlotLoader.h/.cpp` | `requestClear` / `clearLocked` (PD14), `restoreSlots` flag (PD15), `requestImport` + Imports dir + name override (PD16), `encodedBytes` (PD20) |
| `Source/dsp/GiRecorder.h` | only if the take-length function must be exposed (PD11) |
| `Source/PluginProcessor.h/.cpp` | `buildUiTelemetry` + `telemetryLock` (PD12), live overview + dirty + fingerprint (PD6, PD7), tempo atomics (PD11), `clearSlot` guard (PD14), preset flag (PD15), `importDroppedFile`, `saveRecordingTo` + worker (PD19) |
| `Source/GiUiJson.h` | 3.2 telemetry encoder, `GiRecRunHold`, slot `dirty` / `encodedBytes` (PD9, PD13, PD20) |
| `Source/PluginEditor.cpp` | `getTelemetry`, `clearSlot`, `saveRecording`, `onCommitFile` live; header notes updated |
| `Source/ui/public/index.html`, `mockups/v1-ui.html` | PD8, PD17 readers, PD21 (identical) |
| `Source/ui/public/js/i18n.js`, `mockups/v1-i18n.js` | two keys × three languages (identical) |
| `tests/render-harness/ui_gates_32.h` (new), `main.cpp` | PD23–PD25 |
| `tests/ui-stub/generic-overrides.json` | regenerated (PD25) |
| `tests/{ui_frontend_check.js,ui_binding_probe.js,i18n-states.json}` | PD26–PD28 |

---

## Tasks

**Roles.** **[agent]** = gui-agent writes files. **[run]** = builds, gates, validators, commits. **BG** = `run_in_background`, log to `$S/<name>.log` (`$S` = the executing session's scratchpad); anything over ~5 min runs BG (600 s watchdog). `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. Paths relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-neutral checkpoint (after T2, T3, T4, T5 — one per task, not batched: these touch the block path).** Build the harness (BG); `$H --dump-render-hashes "$S/ck-<T>.sha256"` == `tests/golden/renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`; param-dump == `tests/golden/params.tsv`. A move → STOP, attribute to that task. Renders are never re-pinned.

### Wave 0 — pre-flight

1. **T1 — pre-flight + baselines** [run]. Output `$S/preflight.txt`; any mismatch → STOP and ask.
   - Contracts re-hashed against STATUS frontmatter; `git log --oneline 6ea16b17..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` empty; branch `main`; `git status --short` recorded.
   - `cmp` `index.html` / `v1-ui.html` and `js/i18n.js` / `v1-i18n.js`.
   - Baselines on the unmodified tree: harness **374 / 0** (BG; carried extract = sorted `[PASS]/[FAIL]` rows without timing detail = **B**), parity 168 / 0 + GV 2 / 0, render hashes, param-dump, fork drift 0 / 13.
   - UI baseline: overrides regenerate diff-clean; closure 99; probe 261 × 2 + VE 14; five gates green (`default + 19`).
   - `node -e "require('playwright')"` resolves.
   - Depends on: none

### Wave 1 — engine, loader, processor (audio-neutral)

2. **T2 — grain frame + overview counter** (PD1, PD6) [agent]. Checkpoint.
   - Files: `Source/dsp/GiTelemetry.h`, `Source/dsp/Engine.h/.cpp`
   - Depends on: T1
3. **T3 — tape snapshot copy-out** (PD18) [agent]. Checkpoint.
   - Files: `Source/dsp/GiTapeSnapshot.h`, `Source/dsp/Engine.h/.cpp`
   - Depends on: T2 (same files; serial)
4. **T4 — loader: clear, preset flag, import, `encodedBytes`** (PD14, PD15, PD16, PD20) [agent]. Checkpoint.
   - Files: `Source/dsp/SlotLoader.h/.cpp`, `Source/dsp/GiHash.h`
   - Depends on: T1 (independent of T2 / T3; may run beside them)
5. **T5 — processor surface + encoders** (PD2–PD5, PD7, PD9–PD13, PD19 processor half) [agent]. Checkpoint (tempo atomics are written in `processBlock`).
   - Files: `Source/PluginProcessor.h/.cpp`, `Source/GiUiJson.h`, `Source/dsp/GiRecorder.h` (PD11 only)
   - Depends on: T3, T4
6. **T6 — harness rows + guard + overrides** (PD23, PD24, PD25) [agent + run]. `ui_gates_32.h`, `--ug32`, Imports guard, 3.2 fixture; overrides generated twice, byte-identical.
   - Files: `tests/render-harness/ui_gates_32.h`, `main.cpp`, `tests/ui-stub/generic-overrides.json`
   - Depends on: T5
7. **T7 — neutral proof + commit C1** [run]. Harness full set (BG): carried extract diff vs **B** empty except the amended UPM6 predicate (same name, PASS); new rows PASS; every `n` row FAILS its predicate; 0 NOT FIRED; parity 168 / 0 + GV 2 / 0; renders + legacy byte-identical; param-dump identical; drift 0 / 13. The editor still compiles against the processor (the two refusing natives untouched until T8). Commit path-scoped.
   - Depends on: T6

### Wave 2 — editor and page

8. **T8 — editor natives live** (PD12 call site, PD14, PD16, PD19 editor half) [agent]. `getTelemetry` → `buildUiTelemetry`; `clearSlot`; `saveRecording`; `onCommitFile` → `importDroppedFile (slot, file, file.getFileName())` built with `String::fromUTF8`-safe paths; every native completes on every path; file header notes 6 and the drop note updated.
   - Files: `Source/PluginEditor.cpp` (`.h` if a member is needed)
   - Depends on: T7
9. **T9 — page defects + toasts** (PD8, PD17, PD21, PD22) [agent + run]. Both HTML files, both i18n files; `cmp` clean on both pairs; check-i18n, fr-lint, zh-lint green before moving on.
   - Files: `Source/ui/public/index.html`, `.planning/mockups/v1-ui.html`, `Source/ui/public/js/i18n.js`, `.planning/mockups/v1-i18n.js`
   - Depends on: T1 (source-only; may be prepared beside Wave 1, applied after C1)
10. **T10 — build, install, validate + commit C2** [run]. Reconfigure only if a source file was added to a target; build VST3 + AU + Standalone + consoles, **0 warnings in plugin sources**. `./scripts/build-and-install.sh O-GranulatorInstrument` (cache clear, dual-variant sweep); `auval` PASS; pluginval 10 VST3 + AU with the sidechain on, then `OGI_GATE_SC_DEFAULT_OFF` (PLAN-2.6 W7 procedure), shipping build restored + reinstalled; X2 = 0. Standalone smoke (window capture only, never a region): load the factory tone, hold a note → one shot with playheads and dots visible, read it; confirm no file appeared under the real `Imports/` from any automated step. Commit.
    - Depends on: T8, T9

### Wave 3 — UI proof

11. **T11 — closure + probe rows** (PD26, PD27) [agent + run]. Both scripts with old and new NCs.
    - Files: `tests/ui_frontend_check.js`, `tests/ui_binding_probe.js`
    - Depends on: T10
12. **T12 — i18n states** (PD28) [agent + run].
    - Files: `tests/i18n-states.json`
    - Depends on: T10
13. **T13 — full gate run + commit C3** (PD29) [run]. All green with exit codes recorded; a new collision or fit failure against T1's baseline is a D40 defect: fixed in both HTML files, the sequence re-run, the install refreshed. Commit.
    - Depends on: T11, T12

### Wave 4 — regression + docs

14. **T14 — regression on the final tree** [run]. Harness full set (BG) vs **B** + new rows; parity; renders + legacy; param-dump; drift 0 / 13; X2 0; auval; pluginval 10 VST3 + AU, sidechain on and off; final install of the shipping build; `auval -a | grep -i granulatorinstrument`; `cmp` both pairs.
    - Depends on: T13
15. **T15 — docs + commit C4** [agent + run].
    - `HANDS-ON-3.2.md` (run by Taylor at 3.2 verify together with the 16 rows of HANDS-ON-3.1, D43): R§9 "Human" list, plus — Save with the track idle in Logic → `toast.saveFailed`, retry live works; Save of a take recorded hot (clip note, PD19); a drop on each of the four lanes; drop the same file twice → one file in Imports; lane name is the dropped file's name; quit / reopen restores from Imports; delete the import → relink; a failed load shows the toast in fr and zh-Hans; grain dots follow Pan Spread and Reverse; playheads on every held voice; record head sweeps and the waveform appears at commit (sign-off); "Done" / "Discarded" fall back after about 4 s; the amp figure's level dots (PD2 side effect); expected 3.2 placeholders (voice strip, MOD dots, meters, no modulated loop region).
    - `SUMMARY-3.2.md` (complex template): rows, counts, NC results, UPL1 / UPB1 numbers, deviations, open items (`WavFailed` silent, Windows).
    - STATUS.md → `stage_3_phase_3_2_execute_complete`, next `/plugin-verify O-GranulatorInstrument 3-gui`. REQUIREMENTS untouched (UI-01 moves at verify).
    - Depends on: T14

---

## Dependency graph

```
T1 ─┬─ T2 ── T3 ─┐
    ├─ T4 ───────┴─ T5 ── T6 ── T7 ── T8 ─┐
    └─ T9 ────────────────────────────────┴─ T10 ─┬─ T11 ─┐
                                                  └─ T12 ─┴─ T13 ── T14 ── T15
```

---

## Gate criteria (3.2)

| Gate | Pass |
|---|---|
| Harness | carried extract diff vs B **empty** (UPM6 same name, PASS); new `ui_gates_32` rows PASS; every `n` row FAILS its predicate; 0 NOT FIRED |
| Parity | 168 / 0 + GV 2 / 0 |
| Renders | renders-2.4 + legacy byte-identical (after T2, T3, T4, T5 and on the final tree) |
| param-dump | identical (261) |
| Fork drift | 0 / 13 |
| Allocation | UAL1: 0 on the audio thread (frame + snapshot) |
| Test directories | nothing written under the real `~/Music/Ouaricon/O-GranulatorInstrument/` by any automated step |
| Build | VST3 + AU + Standalone + consoles, 0 warnings in plugin sources |
| auval; pluginval 10 VST3 + AU | PASS / SUCCESS, sidechain on and off |
| X2 | 0 test-hook symbols in shipping binaries |
| `generic-overrides.json` | twice byte-identical; diff-clean vs committed |
| `ui_frontend_check.js` | 0 failures; 176 / 70 / 15; 34 = 34; no `unavailable`; all NCs FAIL |
| `ui_binding_probe.js` | 261 / 261 × 2; VE 14 / 14; LN1–LN7, PX1–PX5 PASS; NCs FAIL |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 |
| check-ui-labels | 0; banner `(261 rows) + overrides`, states = the file's count |
| boot-all-uis `--strict-tips` | completes, 0 DEAD, 0 late |
| `cmp` | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js` |

Exit 77 anywhere = not run = not a pass.

---

## Risks / watch items

- **The block path is touched three times** (frame copy, write counter, snapshot). Each has its own checkpoint; a render move is attributed to one task and never re-pinned.
- **Snapshot buffer lifetime (PD18).** The timeout path is the dangerous one: freeing on the worker while the audio thread resumes is a use-after-free. USV2 exercises resume-after-timeout; the invariant is "release only after an audio-thread acknowledgement or with audio stopped".
- **Progress, not wall time.** Both the stale-frame test (PD4) and the save timeout (PD19) key on a counter that the audio thread moves; a host that idles the callback must read as stale / timeout, never as a hang.
- **Hold masks "no signal" (PD9).** `run` 5 outranks the signal test on the page; URC1's negative control (no hold expiry) must show the mask.
- **Vacuous rows.** UOV2 needs feedback loud enough that live bins differ from pristine; UCN1 needs killing grains present in the frame; URC2 needs a peak above `kPresenceLin`. Each row asserts its stimulus fired before asserting the result.
- **`slotRev` vs `ovRev` (PD7).** A `dirty` flag that is not in the fingerprint leaves the Reset highlight stale; UOV2n-b is the proof.
- **Hash scope (PD16).** `contentHash` covers the first 131,072 frames only; naming an import by it loads the wrong file silently. UIM3 holds the file-byte hash in place.
- **Move across volumes** degrades to copy + delete; still on the worker. A crash mid-move leaves only a `.part`.
- **Stub probe ≠ real WebView.** The WKWebView drop after the move, the chooser, and host idling during Save are hands-on only.
- **Payload.** About 35 KB of JSON at 30 Hz in the worst case; UPL1 is INFO, but a build cost above about 1 ms per pull is reported, not accepted silently.
- **D40.** Any page change beyond PD8 / PD17 / PD21 needs a failing gate as its reason.

## Deviations protocol

Record every departure in SUMMARY-3.2 (what, why, evidence, which gate). Stop and ask only for: an audio-moving change, a contract edit, a module or `scripts/` change, a forked-file edit, a v1 design change beyond a defect (D40), or a snapshot design that cannot keep the PD18 lifetime invariant.

## Carried (not 3.2)

3.3: `newest`, `menv`, `wantSt`, releasing voice state, `sources`, `modNorm`, `out` meters, `lanes[].lo / .hi`, `grain_limit` hint, the remaining unmeasured `[data-i18n]` states, HANDS-ON-2.1–2.6 + Logic DAW pass (D39). Stage 4: `WavFailed` surfacing, blobs in presets, factory presets, Windows drop / browser check, `output_gain` default, float WAV for hot takes.

## Success criteria (gate for verify)

- [ ] Playheads for every sounding voice and grain dots by stream (position, pan, envelope, length) from the engine frame; stream numbering proven 1-based on the page (PX rows)
- [ ] Feedback head and record head track the engine; the overview redraws while feedback writes; `dirty` ticks appear only where the tape changed and clear on Reset
- [ ] Record-in panel: No sidechain / no signal / Armed / Recording / Committing / Done / Discarded from engine state; Done and Discarded fall back after 4 s
- [ ] `clearSlot` empties a lane through the handoff and is refused while that lane records
- [ ] `saveRecording` writes the working copy as 24-bit stereo WAV and completes on every path, including `timeout`
- [ ] A dropped file is moved to Imports under its file hash, referenced as `file:`, keeps its name, survives a session round trip; a failed drop leaves the lane as it was and shows the toast
- [ ] A preset replaces only the lanes it has a source for (D45)
- [ ] `encodedBytes` exact after an embed
- [ ] No consuming feed has two readers; 0 audio-thread allocations
- [ ] Stage 2 and 3.1 unchanged: harness carried diff empty, parity, renders, param-dump, drift, X2, 261 × 2, VE 14
- [ ] All five UI gates green with the new states; both file pairs `cmp` clean
- [ ] HANDS-ON-3.2.md ready for verify (with HANDS-ON-3.1, D43)
