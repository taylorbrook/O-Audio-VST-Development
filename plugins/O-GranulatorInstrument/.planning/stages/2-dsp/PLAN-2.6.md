---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.6 Execution Plan"
created: 2026-11-09
juce_version: "8.0.15"
phase: "2.6"
summary: "Eight waves: (W0) pre-flight + baselines (harness 300/0, renders-2.4 / legacy, parity 168/0 + GV 2/0, param-dump) and --cpu25a on the UNMODIFIED 2.5b engine; (W1) leak fix first: processBlockBypassed clears every channel + scratch-copy / per-piece clear moved into Engine::process (renders byte-identical), readSet26 re-scope, R1 rec_arm pinned Off; (W2) GiRecorder (one-shot arm at block start, Manual / Threshold 10 ms pre-roll / Next Note at the two serial-mint sites, length seconds / bars, D6 presence, grow-only capture buffer) + commit hand-off (20 ms Timer copy-out, watcher without a MessageManager, worker builds tape + WAV + hash + FLAC, WAV before publish) + clear sentinel -> commit C1; (W3) GiHash XXH64 + own 24-bit quantiser + FLAC-24 memory paths, <Slots> parse / emit, resolve order (path -> blob -> Recordings search -> factory -> missing), relink, embed carry rule; (W4) FactorySources BinaryData target + --gen-factory stubs (tone mono, noise stereo) + GiFactory table; (W5) GiTelemetry (UiTelemetry reclassified VERBATIM); (W6) gates eg26a + eg26b (~55 rows), harness 300 -> ~355, parity carried, --cpu26; (W7) install, validators with SC on / off + ~/Music Recordings snapshot, HANDS-ON-2.6 (deferred), SUMMARY / STATUS, commit C2. Checkpoint (renders-2.4 + legacy byte-identical) after every engine task. No tag; version 0.1.0; no new params; CMakeLists.txt edited (FactorySources only)."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.6 Execution Plan (Record-in, `<Slots>` state, factory sources)

## Goal

**FUNC-07:** capture the sidechain into `rec_target` by Manual / Threshold / Next Note (D34 one-shot, D35 pre-roll). **FUNC-06:** file references persist; a missing file relinks without a crash; `factory:` sources resolve; recordings embed as FLAC-24 (D36). **QUAL-02 engine path:** clean degradation with no bus or an unrouted bus, and the sidechain **never** reaches the output in any state (including host bypass — a live leak today, RS1). **FUNC-08 groundwork:** the `factory:` path end to end with 2 generated stubs (D33). Telemetry feeds for Stage 3.

There is no parent equivalent (O-Granulator @ `82406fce` has no Record-in), so there is **no parity arm**. Proof is by law gates, leak gates, click gates against same-motion controls, the block-size null, the allocation gate, the host-idle bound, and true bypass.

**Out of scope** (CONTEXT-2.6): curated factory set + presets (FUNC-08, Stage 4); relink / load / record UI and lanes (3.2); Record-in as a mod source; embedding the working copy; cross-host DAW sidechain checks (HANDS-ON-2.6, Stage 4).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.6.md` | D33–D36, Defaults, test criteria, constraints. **Binding, not reopened.** D11–D32 still bind. |
| `stages/2-dsp/RESEARCH-2.6.md` | Summary 1–14 (cited **RSx**), §1–§10 (**R§x**), Pitfalls **P1–P20** (R§10), decisions R§11. Authoritative over ARCHITECTURE where marked [correction] (ARCH §1 bypass clear, ARCH §22 tape build on the message thread, CONTEXT "≈ 46 MB", "2.1 grain snapshot"). This plan's PDs decide every R§11 item. |
| `stages/2-dsp/{PLAN,SUMMARY,VERIFICATION}-2.5b.md` | What exists: harness 300/0, `--parity` 168/0 + GV 2/0, `--cpu25a` C25m 27.62 % / C25tm 6.56 %, hook / counter conventions, checkpoint pattern, local κ, `Session::beforeBlock`. |
| `research/ARCHITECTURE.md` | §5, §6, §22, File I/O and Sources, State Persistence, Record-In Availability, Threading Model, Telemetry, A7. |
| `parameter-spec.md` (LOCKED) + `tests/golden/params.tsv` | `rec_*` rows (`params.tsv:179-185`), `sync_mode`, `sN_loop_*`. **Not edited.** |
| This plugin | `Source/{PluginProcessor.h,PluginProcessor.cpp,GiParamIds.h}`, `Source/dsp/{Engine.h,Engine.cpp,SlotLoader.h,SlotLoader.cpp,SlotHandoff.h,SlotTape.h,MipTape.h,GiRecordHead.h,KaiserResampler.h,GiConstants.h,VoiceGrain.h}`, `Source/dsp/forked/{UiTelemetry.h,ControlGrid.h,TempoTracker.h,FORK-MANIFEST.md}`, `CMakeLists.txt`, `tests/render-harness/{main.cpp,engine_gates_21.h,engine_gates_24b.h,engine_gates_25d.h}`, `tests/fork-drift-check.sh`. |
| References (read, never edit) | `plugins/O-Strata/Source/PluginProcessor.cpp:1220-1361` (Base64 + missing / locate); `plugins/O-simpleSampler/Source/PluginProcessor.cpp:362-422, 666-714` (`MemoryInputStream` + Base64); `~/JUCE/modules/juce_audio_formats/codecs/juce_FlacAudioFormat.cpp:417-526`; `~/JUCE/modules/juce_audio_processors_headless/processors/juce_AudioProcessor.cpp:601-602`; xxHash spec (R Sources). |

Plan-time facts (verified 2026-11-09): last code commit under `Source/ tests/ CMakeLists.txt` = **`c7efbee7`**; `Engine.cpp` 3855 lines (`prepare:81`, `process:608`, `gridTick:715`, `takeTapes:911`, `startVoice:2227`, `retriggerVoice:2551`, `mixPiece:3640`); `Engine.h` 889 lines (`EngineTestHooks:224`, `kLawKeySize:702`, `destTel:837`); `PluginProcessor.cpp` 638 lines (`processBlock:208`, `processBlockBypassed:287` → base at `:291`, `getStateInformation:456`, `setStateInformation:498`, `// Phase 2.6` marker `:513`); `SlotLoader.cpp` 640, `SlotHandoff.h` 236; `CMakeLists.txt` FactorySources block commented `:172-178`, tests block `:212-223`; `readSet25b` at `engine_gates_21.h:317`, N2 literal `:2154`. Test headers `engine_gates_26a.h` / `26b.h` are free. No `Resources/` directory exists yet. **Line numbers drift as tasks land — re-grep, never trust them blindly.**

---

## Plan-phase decisions (resolve R§11)

- **PD1 [R§11.1 adopted] Hash = own XXH64 in `Source/dsp/GiHash.h`** from the spec (~40 lines; little-endian lanes by byte assembly). Not vendored; no `juce_cryptography`. KA gate with the 4 LLVM vectors + 1-bit-flip control.
- **PD2 [R§11.2 adopted] Canonical hash stream** = header `uint32 LE C` + `uint32 LE K`, `K = min (F, 131072)`, then K × C `int32 LE Q(x)` interleaved; seed 0; persisted as **16 lowercase hex**, parsed only as exactly 16 hex through `getHexValue64` (never `getLargeIntValue`). Computed on the **pristine** at its native rate.
- **PD3 [R§11.3 adopted] Hot takes stay unclamped.** Capture and WAV keep |x| > 1; the quantiser `Q(x) = (int32) clamp (nearbyint ((double) x · 8388608.0), −8388608.0, 8388607.0)` clips the blob and the hash stream. SUMMARY + HANDS-ON note: a blob-only reload of a take over 0 dBFS is clipped.
- **PD4 [R§11.4 adopted] FLAC.** Quality index **5 explicitly**; written through `AudioFormatWriter::write (const int**, n)` with `q << 8`, chunked, **never** `writeFromAudioSampleBuffer` (RS2). Embed limit **16 777 216 FLAC bytes** (pre-base64). Decode from `MemoryInputStream` for blob and factory. `juce::Base64`, never `MemoryBlock::toBase64Encoding`. `embedded` attribute > 22 369 624 chars → ignored before decoding.
- **PD5 [R§11.5 adopted] Stamps and latches.** `rec_arm` / `rec_target` / `rec_trigger` read from raw atomics once at the top of `Engine::process`; the arm edge is stamped at **block start**. Latches: target + trigger at Armed entry; threshold + gain at grid ticks (gain = 20 ms ExactRamp through `ControlGrid`'s `dbToGain` / `snapDb`, RS14); `rec_length` / `rec_length_sync` / `sync_mode` + bar length at the start sample.
- **PD6 [R§11.6 adopted] Mono sidechain → mono pristine** (1 ch). Only `sidechainChannels` channels are read; ch 1 of a mono SC is host garbage (P14).
- **PD7 [R§11.7 adopted] Commit hand-off.** Audio thread: fill `commit {slot, frames, rate, channels, startAbs}`, `ready.store (frames, release)`, state Committing — nothing else. Off-audio poll: message-thread `juce::Timer` 20 ms; without a MessageManager (`getInstanceWithoutCreating() == nullptr`) a small `juce::Thread` watcher doing the same poll. Poll copies `cap[0, f)` under `captureLock` into a heap pristine, `ready.store (0)`, then `SlotLoader::requestCommit`. **AsyncUpdater rejected** (lock + realloc on the audio thread). Harness `commitSync()` (OGI_TEST_HOOKS) runs poll + job on the caller.
- **PD8 [R§11.8 adopted] Worker commit job order:** hash → WAV (float32, `Recordings/GI-Rec-YYYYMMDD-HHMMSS-S<n>.wav` via `getNonexistentSibling`) → `buildTape` (equal rates = bit-exact) → publish with source / fileSize / hash under the loader lock → FLAC-24 encode → blob cached (generation-checked). WAV failure still publishes: `rec:<intended path>`, blob carries the take, reason `WavFailed`.
- **PD9 [R§11.9 adopted] `prepareToPlay` during a capture.** Recording → stop at the last processed sample, normal commit / discard rule, commit carries `rate = fs_old` (loader resamples to the session rate; pristine stays at fs_old). Armed → Idle, `Interrupted`. Committing untouched. Buffer under `captureLock`, grow-only to `F_max (fs_new)`; grow skipped while `ready != 0`; `capFrames = min (F_max, capacity)`. `prevArm` = current value (an arm held On never re-arms by itself). Prepare never waits on the message thread.
- **PD10 [R§11.10 adopted] Bypassed block** (`engine->bypassedBlock()`): Armed → cancel `Bypassed`; Recording → stop (commit ≥ 0.5 s). Logic-style "no render during bypass" splices a gap on unbypass (documented; HANDS-ON row).
- **PD11 [R§11.11 adopted] Clear sentinel.** `static SlotTape kClearTape` (frames 0, never deleted); `SlotHandoff::publishClear (s)`. `sweepLocked`, the dtor and the deferred list skip it. `takeTapes`: sentinel → `killGrainsOfTape (old)`, retire old, `current[s] = nullptr`, `headRelatch[s] = false`. A pending clear superseded by a load is dropped, not freed. Used by an empty-source restore and a missing resolve on a loaded slot.
- **PD12 [R§11.12 adopted] Identical-source restore = no-op** (same source + same contentHash + loaded → no job; the working copy is not reset by a host re-send).
- **PD13 [R§11.13 adopted] Resolve rungs.** Rung 1 path + size; a hash mismatch still loads and sets `hashMismatch`. Rung 3: exact `Recordings/<filename>` first, then a flat (depth 0) `*.wav` scan **≤ 1024 entries**, size prefilter, **≤ 16** hashed candidates, first match wins, cancellable. A rung-3 hit **rewrites the path** and sets `relocated`. Rung 2 / 3 / 4 otherwise keep the stored source string. Rung 3 runs only on the worker.
- **PD14 [R§11.14 adopted] Relink.** Hash match → keep scheme, `embed`, carried blob; source = scheme + new path. Mismatch → still loads; `file:<path>`, `embed` off, blob dropped, `relinkMismatch = true`.
- **PD15 [R§11.15 adopted] Telemetry.** `forked/UiTelemetry.h` **reclassified VERBATIM** (FORK-MANIFEST row only; file bytes unchanged) and reused (`TripleBuffer`, `pushMax`). New `Source/dsp/GiTelemetry.h`: `recState` (state | reason << 8 | heldArm << 16), `recFrames` / `recTarget`, SC + output peaks (`pushMax`, reader `exchange (0)`), `headPos[4]`, `swapState[4]`, `GiOverviewLive` (5625 bins × min / max per slot, `seq` + `tapeEpoch`, ctor-allocated), `TripleBuffer<GiUiFrame>` (288 grains + 20 voices; published per block end). Reason codes: `None, SidechainUnavailable, NoSignal, TooShort, Cancelled, Interrupted, Bypassed, Committed, WavFailed` (parameter-spec `:353` "Done / Discarded" map to Idle + reason — reconcile in Stage 3).
- **PD16 [R§11.16 adopted] Factory.** Harness mode `--gen-factory <dir>` (same JUCE encoder, int path, Q). `stub_tone`: mono, 48 kHz, 3.0 s, 220 Hz, k = 1…8 at 1/k, −12 dBFS peak, 10 ms raised-cosine ends. `stub_noise`: stereo, 48 kHz, 3.0 s, harness `hashNoise`, one-pole LP, −18 dBFS RMS. Pins: decoded-PCM XXH64 `kHashTone` / `kHashNoise` in `GiFactory.h` (all platforms) + file sha256 in `tests/golden/factory-stubs.sha256` (mac arm64 only; INFO elsewhere). Ids with underscores only. Table uses explicit symbols, never `getNamedResource`.
- **PD17 [R§11.17 adopted] Harness re-scope.** `readSet26()` = 261 bound; N2 / N2s / FN3 → "touched = the 7 `rec_*` randomised with a **silent** SC → output bitwise identical" (literals 261 / 7). R1 random-param mode pins `rec_arm` Off. Recordings override set in `make()` for every processor to a `gTmp` subdir with a non-ASCII component (`Récordings ✓`, built via `juce::File` / `String::fromUTF8`). Guard row: real `~/Music/Ouaricon/O-GranulatorInstrument/Recordings` (count + newest mtime) unchanged start → end. Any other carried row needing a re-scope → **S8**.
- **PD18 [R§11.18 adopted] `rec_*` bound in `GiRecorder`** (raw atomics), **not** `ControlGrid.h` → no forked-file edit, no manifest re-hash (beyond PD15's classification).
- **PD19 Capture placement (RS5, R§3).** Per piece `[pos, b)` in `Engine::process`: grid tick → copy `sc[pos, b)` into a member 2×32 scratch (nullptr → 0; mono → R = L for the threshold detector only), then **zero `out[pos, b)`** → event loop (Next Note stamps at `startVoice` / `retriggerVoice`) → `recorder.runPiece` over the scratch → `mixPiece`. `processBlock`: clear ch ≥ 2; pass `scL = ch0` if SC ≥ 1, `scR = ch1` if SC ≥ 2; if the engine is not called, `buffer.clear()`. The engine's early-return guard zeroes the outputs too. Removes the top-of-block clear — byte-identical because `mixPiece` assigns every sample.
- **PD20 Leak fix lands first (RS1).** `processBlockBypassed`: NE drop → `buffer.clear()` → `engine->bypassedBlock()`. Never calls the base. Hook `baseBypass` (calls the base instead) = LK2n control, which must fire.
- **PD21 Restore / save race (R§6.2).** `setStateInformation` reads `<Slots>` **before** `removeCustomChildren`, writes `SlotState` records immediately (source + carried blob), then enqueues resolve jobs; never decodes inline. `getStateInformation` copies records under the loader lock and emits them; never encodes; a blob not finished → `embed="1"` without `embedded`. Parse rules per R§6.1 (index one digit 0–3; known prefix else empty; `embed` / `trimmed` as strings `"1"` / `"true"`; fileSize digits only; hash 16 hex; blob bound + alphabet). Fixed attribute order; only non-empty slots written.
- **PD22 Recordings directory.** `SlotLoader::setRecordingsDirectory (File)`; the default (`userMusicDirectory/Ouaricon/O-GranulatorInstrument/Recordings`) is resolved **lazily** at first commit / rung 3, never in a constructor.
- **PD23 Test hooks** (under `OGI_TEST_HOOKS`, in `EngineTestHooks` / a `LoaderTestHooks`):
  - Switches: `baseBypass`, `scNoClear`, `startOffset` (+1 sample), `prerollOff`, `armLevelTrigger`, `capOff`, `nnAnyNote`, `swapKillHard`, `reencodeOnLoad` (quality 8), `parseIsBool`, `searchUnbounded`, `floatWriterPath`, `testNoEntryCounter` (carried).
  - Counters: `recArms`, `recStarts`, `recCommits`, `recDiscards`, `recCancels`, `heldArms`, `commitsPublished`, `clearsPublished`, `resolveRung[5]`, `fsTouches`, `dirEntriesScanned`, `candidatesHashed`, `encodes`, `blobsCarried`. `resetCounters()` zeroes them.
  - Probes: `getRecorderForTest()` (state, reason, armAt, start, stop, peak, capFrames), `getCaptureForTest (ch, i)`, `getSlotStateForTest (s)`, `hashForTest (s)`, `commitSync()`.
- **PD24 Housekeeping.**
  - Version **0.1.0**; **no tag**. No new params, no `<Engine>` keys → param-dump byte-identical.
  - `CMakeLists.txt` **edited**: FactorySources target + link only, placed before the tests block (RS12). New sources are headers except as the agent needs (`.cpp` additions go in the existing glob / list — the agent reports any CMake list edit).
  - Not edited: every `forked/` file (UiTelemetry reclassified in the manifest only); `modules/*`; `MidiCursor.h`; `cpu_calib.h`; `tests/parity/*`; pg22–pg25a; eg22–eg25d except N2s / FN3 literals and the R1 pin.
  - **Two commits**, path-scoped: **C1** after W2 (leak fix + capture + commit + sentinel; harness 300/0, bypass held) and **C2** at the end.
  - HANDS-ON-2.6 written and deferred post-Stage-3 with 2.1–2.5b.
  - Standalone rebuilt explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check.

---

## Tasks

**Roles.** **[agent]** = dsp-agent writes files (no Bash). **[run]** = build / gate runner: builds, gates, validators, hash refreshes, commits. **BG** = `run_in_background`, log to `$S/<name>.log` (`$S` = the executing session's scratchpad). `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. Paths relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-path rules (every engine task).** No locks (except the 2.2 scale try-lock), no allocation, no vector growth, no `std::function`, no `File` I/O, no message posting on the audio thread; fixed arrays only. The engine never writes APVTS. `SlotInfo` writes always under the loader lock; nothing publishes a bare pointer. Capture timing is absolute-sample-ordered.

**Checkpoint (after every engine task T3–T9).** Build the harness (BG); `$H --dump-render-hashes "$S/ck-<T>.sha256"` == `renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`. A move → **S3** attributed to that task. Renders are **never re-pinned**.

### Wave 0 — pre-flight, baselines, CPU reference

**T1. Pre-flight + baselines** [run]
- Output `$S/preflight.txt`. Any mismatch → STOP and ask.
  - **Late changes:** `git log --oneline c7efbee7..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` empty.
  - **Git:** branch `main`; record `git status --short`; foreign paths never staged (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, `plugins/O-GrainScatter/*`, any other session's files).
  - **Contracts:** `parameter-spec.md` sha256 (file + body) = STATUS `contract_checksums`; `shasum -a 256 Source/dsp/forked/*.h` = FORK-MANIFEST col 5.
  - **Baseline (BG):** build harness + param-dump; `$H` = **300 / 0**, 0 NOT FIRED → `$S/h-25b.log`, duplicate-ID set → `$S/dup-25b.txt`; param-dump == `.planning/params.tsv` → `$S/pd-25b.txt`; render dumps == `renders-2.4` / `renders-2.2`; drift 0 / 13.
  - **Parity (BG):** `tests/parity/run.sh "$S/parity"` rc 0; `$H --parity "$S/parity/out"` **168 / 0**, `--rows GV` **2 / 0**; residual lines → `$S/res-base.txt`.
  - **Recordings guard:** snapshot `~/Music/Ouaricon/O-GranulatorInstrument/Recordings` (exists?, count, newest mtime) → `$S/rec-dir-before.txt`.
  - Standalone phony target present; `/opt/homebrew/bin/flac` presence recorded (INFO only — not used).

**T2. `--cpu25a` on the unmodified 2.5b engine** [run]
- Depends on T1 with every BG job finished. **No `Source/` edit before this is done.**
- `$H --golden-dir tests/golden --cpu25a` alone (BG) → `$S/cpu25a-base.log`; valid witnesses (≤ 3 tries, then STOP). Record C25m / C25tm.

### Wave 1 — leak fix + plumbing, no behaviour

**T3. Bypass leak fix + sidechain plumbing + capture placement (PD19, PD20)** [agent]
- Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- `processBlockBypassed` per PD20 (`bypassedBlock()` a no-op stub for now). `processBlock` per R§3.2: clear ch ≥ 2, pass `scL / scR` from the negotiated `sidechainChannels`, `buffer.clear()` on the not-called path. `Engine::process` gains `const float* scL = nullptr, const float* scR = nullptr` (2.5b call unchanged); per piece: scratch copy then zero `out[pos, b)` before the event loop; early-return guard zeroes outputs. Hooks `baseBypass`, `scNoClear`.
- Done: checkpoint passes; full `$H` 300 / 0. The agent reports the before / after of `processBlock`, `processBlockBypassed` and the per-piece prologue.

**T4. Harness re-scope + SC injection plumbing (PD17)** [agent]
- Files: `tests/render-harness/engine_gates_21.h`, `engine_gates_22.h`, `engine_gates_23.h`, `main.cpp`.
- Depends on T3.
- `readSet26()` after `readSet25b()`; N2 / N2s / FN3 → 261 / 7 with a silent-SC stimulus (they stay passing now: `rec_*` are still unread → wire the titles to `readSet26` and the touched set; T6 makes them meaningful). R1: `rec_arm` pinned Off. `Session`: optional SC injection buffer filled in `beforeBlock` (stereo / mono / disabled layouts via `sidechainChannels` override in tests), Recordings override in `make()`, guard row helper.
- Done: `$H` 300 / 0 (row count unchanged); checkpoint passes.

### Wave 2 — capture + commit (commit C1)

**T5. Clear sentinel + commit plumbing in the loader (PD7, PD8 publish half, PD11, PD22)** [agent]
- Files: `Source/dsp/SlotHandoff.h`, `Source/dsp/SlotLoader.h`, `Source/dsp/SlotLoader.cpp`, `Source/dsp/Engine.cpp` (`takeTapes` sentinel branch).
- Depends on T4.
- `kClearTape` + `publishClear`; sweep / dtor / deferred skip it; `takeTapes` branch. `SlotLoader::requestCommit (slot, Pristine)` job: `buildTape` from a pristine at its own rate → publish (new-source rule) with `SlotState.source = rec:<path>`. WAV writer (float32, unique name) **before** publish; `WavFailed` path. `setRecordingsDirectory`, lazy default. Decode refactor `decodeReader (reader, …)` shared by File / memory (R§5.2) — File path behaviour unchanged. Hash / FLAC hooks left as stubs (T8).
- Done: checkpoint passes; full `$H` 300 / 0.

**T6. `GiRecorder` — state machine, triggers, length, presence, buffer (D34, D35, PD5, PD6, PD9, PD10)** [agent]
- Files: `Source/dsp/GiRecorder.h` (new), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/PluginProcessor.cpp` (raw-atomic binding of the 7 `rec_*`, `bypassedBlock`, poll Timer / watcher, `prepareToPlay` grow).
- Depends on T5.
- States + reasons per R§2.1 table; `prevArm` edge detector; no-bus arm inert (`SidechainUnavailable`). `runPiece` per R§2.2: `x = sc · g`, threshold crossing (strict, `max (|xL|, |xR|)`), pre-roll ring `P = lround (0.010 · fs)` (prepare-allocated), running peak, length seconds / bars (tracker `isPlaying` + anchor, latched at start), cap `min (F_max (fs), capacity)`, stop → ≥ `ceil (0.5 · fs)` frames **and** peak ≥ −90 dBFS → commit else discard. Next Note hook at `startVoice` + `retriggerVoice` only. Committing → Idle when `ready == 0` at block start; held arm → Armed. Poll + `captureLock` copy-out + `requestCommit`; watcher thread when no MessageManager; `commitSync()` hook. Hooks `startOffset`, `prerollOff`, `armLevelTrigger`, `capOff`, `nnAnyNote`; counters per PD23.
- Done: checkpoint passes (arm Off in every recipe); full `$H` 300 / 0; N2 / N2s / FN3 now genuinely touch 7 with a silent SC (D6 discard proves output identity).

**T7. Commit C1** [run]
- Depends on T6. `git branch --show-current` (= `main`) + `git status --short` immediately before; `git status --short -- plugins/O-GranulatorInstrument` lists only T3–T6 files; `git add plugins/O-GranulatorInstrument/Source/dsp/GiRecorder.h`; `git commit -- plugins/O-GranulatorInstrument` with `phase: O-GranulatorInstrument 2-dsp/2.6 record-in capture (bypass SC leak fixed; per-piece SC scratch + clear; GiRecorder one-shot arm, Manual / Threshold 10 ms pre-roll / Next Note, D6 presence; Timer copy-out + worker tape/WAV commit; clear sentinel; readSet26 261/7; harness 300/0; renders-2.4 + legacy byte-identical)`. `git log -1 --stat` only plugin paths. **No tag.**

### Wave 3 — hash, FLAC, state, resolve

**T8. `GiHash.h` + quantiser + FLAC-24 memory paths (PD1–PD4)** [agent]
- Files: `Source/dsp/GiHash.h` (new: XXH64, `Q`, `deq`, `contentHash (pristine)`), `Source/dsp/SlotLoader.cpp` (hash at every load / commit; FLAC encode job after publish; blob cache per generation; `tooLarge`; memory decode from Base64 blob).
- Depends on T7.
- Hook `floatWriterPath` (encode via `writeFromAudioSampleBuffer`) = HS2 control.
- Done: checkpoint passes; full `$H` 300 / 0.

**T9. `<Slots>` state + resolve order + relink + embed (PD12–PD14, PD21)** [agent]
- Files: `Source/PluginProcessor.cpp`, `Source/dsp/SlotLoader.h`, `Source/dsp/SlotLoader.cpp`.
- Depends on T8.
- `SlotState` fields per R§6.2 (+ `relocated`) under the loader lock. Parse / emit per PD21. Resolve job rungs 1–5 per PD13 (rung 4 calls `GiFactory` lookup — empty table until T10 → missing). `requestRelink (slot, File)`, `setSlotEmbed (slot, bool)` (factory ignored), carry rule (blob re-emitted verbatim while decoded hash == `contentHash`). `SlotInfo` += source / missing / embed / hasBlob / contentHash / hashMismatch / relinkMismatch / relocated. `fsTouches` incremented at every `File` I/O site. Hooks `reencodeOnLoad`, `parseIsBool`, `searchUnbounded`.
- Done: checkpoint passes (no `<Slots>` content in recipes → bypass); full `$H` 300 / 0.

### Wave 4 — factory sources

**T10. FactorySources target + generator + table (D33, PD16)** [agent writes; runner generates + pins]
- Files: `CMakeLists.txt` (uncomment / write the `juce_add_binary_data` block `NAMESPACE FactorySourceData HEADER_NAME FactorySourceData.h`, link **before** the tests block), `Source/dsp/GiFactory.h` (new: `{ "stub_tone", FactorySourceData::stub_tone_flac, …Size, kHashTone }, { "stub_noise", … }`), `tests/render-harness/main.cpp` (`--gen-factory <dir>`; refuses to overwrite unless `--force`).
- Depends on T9.
- Bootstrap order (the target needs the files to configure): (1) agent writes `GiFactory.h` with `kHash* = 0` + the generator, CMake block still commented; (2) runner builds the harness, runs `$H --gen-factory Resources/factory`, records the two decoded hashes and the two file sha256 → `tests/golden/factory-stubs.sha256`; (3) agent writes the hashes into `GiFactory.h` and enables the CMake block; (4) runner reconfigures + rebuilds; `$H --gen-factory "$S/gen2"` byte-identical to the checked-in files (mac arm64).
- Done: harness + param-dump link (`FactorySourceData::*` resolved through the console link inheritance); checkpoint passes; full `$H` 300 / 0; param-dump identical.

### Wave 5 — telemetry

**T11. `GiTelemetry.h` + manifest reclassification (PD15)** [agent; runner verifies drift]
- Files: `Source/dsp/GiTelemetry.h` (new), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp` (writers at block end / per cell for overview bins), `Source/dsp/GiRecordHead.h` (head position + bin writes, if the head owns them), `Source/dsp/forked/FORK-MANIFEST.md` (UiTelemetry row → VERBATIM; col 5 unchanged).
- Depends on T10.
- Done: checkpoint passes; drift 0 / 13 + NC-a / NC-b fire; full `$H` 300 / 0.

### Wave 6 — gates

**T12. `engine_gates_26a.h` — capture, triggers, one-shot, length, presence, leaks, commit** [agent]
- Files: `tests/render-harness/engine_gates_26a.h` (new, `eg26a::runAll()`).
- Depends on T11. SC injected through `beforeBlock`. Every row prints its firing control.

  | ID | Stimulus | Pass | Control |
  |---|---|---|---|
  | RC1 | Manual × slots 0–3, stereo noise SC, `rec_gain` +6 dB | tape level 0 (`MipTape::levelSample`) == sc × g **bitwise** over `[start, start + frames)` | RC1n `startOffset` → differs |
  | RC2 | Threshold × slots 0–3 | as RC1 incl. pre-roll | — |
  | RC3 | Next Note × slots 0–3 | as RC1 from the note sample | — |
  | RC4 | recorded slot + 4-note chord; mono SC take | 4 distinct grain rates; mono SC → 1-ch pristine | — |
  | TH1 | crossing at arm + 200 ms | start == crossing − P (441 @ 44.1 k) | TH1n `prerollOff` → start == crossing |
  | TH2 | crossing at arm + 3 ms | start == arm sample (clip) | — |
  | TH3 | Manual; Next Note; pre-roll counts toward length | start == arm / note sample; frames == length incl. pre-roll | — |
  | OS1 | arm held On after a commit, 2 s | stays Idle, `recCommits` 1 | OS1n `armLevelTrigger` → re-arms |
  | OS2 | Off→On again | second capture lands | — |
  | OS3 | Off while Armed | Idle, `Cancelled`, slot unchanged | — |
  | OS4 | Off while Recording at 0.5 s / 0.49 s | commits / discards `TooShort` | — |
  | OS5 | arm edge during Committing (no pump) | held → Armed when `ready` clears, at that block start | — |
  | LN1 | Free, `rec_length` 1.25 s | frames == `llround (1.25 · fs)` | — |
  | LN2 | Tempo + playing, 2 bars @ 120 bpm 4/4; tempo change mid-take | frames == latched bar length × 2; change ignored | — |
  | LN3 | Tempo + stopped | seconds | — |
  | LN4 | cap at 44.1 / 96 / 192 k, `rec_length` 30 s | 1 323 000 / 2 880 000 / 2 880 000 | LN4n `capOff` → overrun counted |
  | MN1 | Next Note: Mono retrigger counts; Legato pitch change + D30 fallback do not | start sample per rule | MN1n `nnAnyNote` → Legato counts |
  | QD1 | silent stereo SC, each trigger | discard `NoSignal`; slot tape pointer + `hashForTest` unchanged | QD1n −89 dBFS take commits |
  | QD2 | bus disabled (0 ch), arm edge | Idle, `SidechainUnavailable`; enabling later with arm still On does nothing | — |
  | QD3 | −89 / −91 dBFS takes | commit / discard | — |
  | LK1 | loud SC (noise 0.7 + 997 Hz FS) × {stereo, mono + garbage ch 1, disabled + garbage both} × {Idle, Armed, Recording, Committing}, notes on a loaded slot | output == SC-silent render **bitwise** over the pre-commit span; no notes → +0.0f | LK1n `scNoClear` → leaks |
  | LK2 | `processBlockBypassed` with loud SC, all 3 layouts | output +0.0f | LK2n `baseBypass` → leaks (the pre-T3 behaviour) |
  | CM1 | commit via `commitSync`; via Timer + `pump()` | tape lands; WAV exists in the override dir; source `rec:<path>`; non-ASCII path round-trips | — |
  | CM2 | `setNonRealtime (true)` + pump | commit lands | — |
  | CM3 | host idle mid-commit and mid-swap; 50 relinks + 50 restores pumped | pending ≤ 1 / slot; `SlotTape::liveCount` bounded; resume → retire ring 0 | CM3n `testNoEntryCounter` → deferred grows |
  | CM4 | prepare mid-Recording (44.1 → 96 k); prepare while Armed; prepare while Committing | committed, pristine rate 44.1 k, tape at 96 k; `Interrupted`; Committing survives | — |
  | CM5 | bypassed blocks while Armed / Recording | `Bypassed` cancel / commit ≥ 0.5 s | — |
  | CM6 | WAV dir unwritable | publishes, reason `WavFailed`, blob carries the take after T8 | — |

  Rows: 28 + 8 controls (RC1n, TH1n, OS1n, LN4n, MN1n, QD1n, LK1n, LK2n) + CM3n = **≈ 37** (exact count recorded in SUMMARY).

**T13. `engine_gates_26b.h` — state, resolve, factory, hash, alloc, clicks, block size, telemetry, bypass + `--cpu26`** [agent]
- Files: `tests/render-harness/engine_gates_26b.h` (new, `eg26b::runAll()`, `eg26b::cpu26()`).
- Depends on T12.
  - **ST1** — restore 4 sources (file, rec, factory, embedded-only) + loop regions + scale state → all audible, `SlotInfo` correct.
  - **ST2** — blob decode == `deq (Q (pristine))` bitwise. **ST2n** `floatWriterPath` → mismatches.
  - **ST3, ST3n** — save → load → save byte-identical (as `MemoryBlock`s), including a save **before** the resolve finishes. `reencodeOnLoad` → bytes change.
  - **ST4** — 15 s @ 192 k full-scale noise take (17.3 MB FLAC) → `embedded` absent, `embed="1"`, falls back to path; `tooLarge` set once (no re-encode loop).
  - **ST5** — hash > 2⁶³ round trip; `embed="0"/"1"/"true"` strings; malformed index / hash / blob / oversize attribute ignored. **ST5n** `parseIsBool` → defaults restored (fires).
  - **ST6** — identical-source restore = no swap (tape pointer + working copy unchanged).
  - **ST7** — empty-source restore on a loaded slot → sentinel clear, 5 ms kill, `current = nullptr`, no crash over 2 s.
  - **RS1–RS7** — rung 1 path + size; size mismatch → blob; Recordings exact name; renamed file found by size + hash (`relocated`, path rewritten); factory; missing (sentinel clear, stable 2 s render, reference re-saved unchanged); relink match keeps embed / blob; relink mismatch → `file:`, embed off, `relinkMismatch`; rung-1 hash mismatch → `hashMismatch`.
  - **RS8, RS8n** — 1100-file Recordings dir: ≤ 1024 scanned, ≤ 16 hashed. `searchUnbounded` → > 1024.
  - **FA1** — both stubs decode; hash == pinned; ids / symbols resolve. **FA2, FA2n** — factory resolve `fsTouches == 0`; `file:` resolve > 0.
  - **HS1** — XXH64 4 KA vectors; **HS1n** 1-bit flip differs. **HS2** — Q(deq(q)) == q ∀ 2²⁴ codes. **HS3** — blob-loaded hash == file-loaded hash for float32 / 16-bit / 24-bit WAV of one signal.
  - **AL1** — 0 audio-thread allocs across arm → record → commit → swap, discard, cancel, pre-roll, state save / load (embedded), missing resolve (sentinel), factory load, prepare mid-record, bypassed blocks; every PD23 counter > 0 (liveness).
  - **CK6a, CK6b** (local κ, 5 ms frames, ≤ 2.0× vs held pre / post references) — commit swap into a sounding slot (tonal A → tonal B); state-load swap. **CK6n** `swapKillHard` > 2.0× with `ctrl FIRED`.
  - **BS1, BS1n** — Manual / Threshold / Next Note scripts (+ notes, + Tempo) at 32 / 441 / 512 / 4096 / variable: start / stop samples + captured PCM bitwise; different SC seed differs.
  - **TL1** — `recState` / progress / peaks track a scripted take. **TL2** — grain frame count ≤ 288, `seqHead == seqTail`. **TL3** — overview bins epoch-tagged after a feedback pass; stale epoch ignored. **TL1n** torn-read probe (write without publish) detected.
  - **TB7** — true bypass via the shipping path: renders-2.4 + legacy byte-identical with SC bus present + idle recorder.
  - **GD1** — real `~/Music/…/Recordings` unchanged across the whole harness run.
  - `cpu26()`: `--cpu25a` rows re-run (C25m ≤ 40 %, C25tm ≤ 12 %) + **C26rec INFO** = C25m while Recording (Threshold, 4 s take in flight).
  - Rows ≈ **22 + 7 controls ≈ 29** (exact count in SUMMARY).

**T14. `main.cpp` wiring** [agent]
- Files: `tests/render-harness/main.cpp`.
- Depends on T13. `#include` eg26a / eg26b after eg25d; `runAll()` in order; `--eg26` dev flag (26a + 26b only); `--cpu26` (rc = INVALID count); banner + usage incl. `--gen-factory`.

**T15. Build + full harness** [run; agent fixes]
- Depends on T14.
- BG build; `$H` → **≈ 355–366 PASS / 0 FAIL**, 0 NOT FIRED (300 + eg26a + eg26b; record exact) → `$S/h-26.log`; duplicate-ID set ⊇ `$S/dup-25b.txt` with no new duplicates; TB0 / TB3 / TB4 / TB5 / TB6 / TB7, RB1 7/7, FR1 14/14; param-dump == `$S/pd-25b.txt`; drift 0 / 13 + NC-a / NC-b; X1 (compdb) 0 parent refs; GD1 pass.
- RC / TH / OS / LN / MN / QD / CM / ST / RS / FA / HS / BS / AL / TL failure → **S5**; LK → **S4**; CK → **S6**. **Never loosen a threshold or re-pin a render.**

**T16. Parity carried** [run]
- Depends on T15. BG: `tests/parity/run.sh` (incremental); `$H --parity` **168 / 0**, GV 2 / 0; residuals == `$S/res-base.txt` to 0.1 dB. Any move → **S3**.

### Wave 7 — CPU, install, validators, hands-on, bookkeeping

**T17. `--cpu26`** [run]
- Depends on T16 (nothing else running; Logic closed). `$H --golden-dir tests/golden --cpu26` alone (BG) → `$S/cpu26.log`: **C25m ≤ 40 %**, **C25tm ≤ 12 %**, witnesses valid, Δ vs T2 (INFO); C26rec INFO. NOVERDICT ≤ 3 tries, then STOP. Valid-witness FAIL → **S2**.

**T18. Install + Standalone + validators + X2 + Recordings guard** [run]
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG). `ninja -C build OuariconGranulatorInstrument_Standalone` (BG); `.app` newer than T15's harness, alive after 6 s. `auval -v aumu OuGI OuDv` PASS + ZZZZ control. pluginval 10 VST3 + AU, one at a time, BG, `--strictness-level 10 --timeout-ms 600000`, Logic closed — **twice each: SC active (2 ch) and SC inactive (`OGI_GATE_SC_DEFAULT_OFF`)**. X2: `nm -C` on installed binaries + `$H` → 0 `ogran::`. Snapshot `~/Music/…/Recordings` → `$S/rec-dir-after.txt`; any new file vs `$S/rec-dir-before.txt` → record it, delete only files created during T18 (by mtime window), and flag in SUMMARY (P17).
- SC-inactive build: the existing gate-only CMake option `OGI_GATE_SC_DEFAULT_OFF` (`CMakeLists.txt:194`, `PluginProcessor.h:33`) in a separate build dir; never installed as the shipping binary — reinstall the default build after.

**T19. `HANDS-ON-2.6.md` (deferred)** [agent]
- Files: `stages/2-dsp/HANDS-ON-2.6.md`, HANDS-ON-2.5b layout, "Deferred with HANDS-ON-2.1–2.5b" banner. Rows:
  1. Logic AU: capture with a routed sidechain, each trigger; captured slot plays polyphonically.
  2. Logic AU unrouted sidechain: arm → "No sidechain signal", slot unchanged, nothing audible from the SC.
  3. Ableton + Reaper VST3 sidechain routing; no SC bleed with the plugin bypassed.
  4. Threshold feel at −40 / −20 dB on drums and voice; pre-roll keeps the transient.
  5. Next Note: record starts with the played note; Legato note changes don't start it.
  6. Bars length under a playing transport; seconds when stopped.
  7. Save / reopen a session with file, rec, factory and embedded slots.
  8. Move a recording on disk → reopen → found via the Recordings search / blob; delete it → missing + relink.
  9. Hot take > 0 dBFS → blob-only reload clipped (PD3) — audible check.
  10. Bypass during a take in Logic (gap splice, PD10).
  11. Logic save time with 4 × ~11 MB embedded blobs (P18).
  12. 2.1–2.5b patches sound unchanged with an idle recorder.
- Not self-certified.

**T20. SUMMARY, STATUS, commit C2** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.6.md`, `.planning/STATUS.md` (`stage_2_phase_2_6_executed_hands_on_pending`; next = 2.6 verify).
- SUMMARY: every row with margin + control; counts vs targets; the RS1 leak fix (before / after LK2n); factory hashes; FLAC sizes; κ table; C25m / C25tm / C26rec + Δ vs T2; state save / load time (INFO); PD1–PD24 as decided; every deviation with its reason.
- **Commit C2:** branch + status immediately before; plugin status lists only this plan's files (anything else → STOP); `git add` new paths (`GiHash.h`, `GiFactory.h`, `GiTelemetry.h`, `Resources/factory/*.flac`, `tests/golden/factory-stubs.sha256`, `engine_gates_26a.h`, `engine_gates_26b.h`, `SUMMARY-2.6.md`, `HANDS-ON-2.6.md`); `git commit -- plugins/O-GranulatorInstrument`. Never `-A` / `-a`. `git log -1 --stat` only plugin paths. **No tag.**

---

## Dependency graph

```
T1 → T2 → T3 → T4 → T5 → T6 → T7 (C1) → T8 → T9 → T10 → T11 → T12 → T13 → T14 → T15 → T16 → T17 → T18 → T19 → T20 (C2)
           (checkpoint after each of T3–T11; full $H after every engine task)
```

- T2 finishes before any `Source/` edit.
- T3 → T11 strictly sequential (shared `Engine.*`, `SlotLoader.*`, `PluginProcessor.cpp`).
- T10 has an internal agent ↔ runner bootstrap (generate → pin → enable).
- T17 runs alone after every build and parity job.

## Execution notes

- **Agent split.** dsp-agent writes T3–T6, T8–T14 code, T19, T20's SUMMARY. The runner does every build, checkpoint, gate, generation, validator, hash refresh and both commits.
- **Background:** ninja, cmake reconfigure (T10), `run.sh`, `build-and-install.sh`, the Standalone link, pluginval (×4), cold auval, `--cpu25a`, `--cpu26`, the full harness (600 s watchdog).
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.6 gate)

| Criterion | Proven by | Target |
|---|---|---|
| Capture into each slot by each trigger; PCM == sc × g; plays polyphonically (FUNC-07) | RC1–RC4, RC1n | bitwise |
| Threshold start = crossing − 10 ms, clipped at arm; Manual / Next Note sample-exact (D35) | TH1–TH3, TH1n, MN1, MN1n | ± 0 samples |
| One-shot arm; cancel; stop ≥ 0.5 s commits, shorter discards (D34) | OS1–OS5, OS1n | exact |
| Bars under Tempo + playing, seconds otherwise; cap `F_max` | LN1–LN4, LN4n | exact frames |
| Silent SC discards, slot bitwise unchanged; no bus = inert; SC never in output in any state (QUAL-02) | QD1–QD3, LK1, LK2 (+ controls) | bitwise / +0.0f |
| Save / reopen restores 4 sources, loops, scale; missing → relink, reference kept, stable (FUNC-06) | ST1, ST6, ST7, RS1–RS7 | all rungs exercised |
| Blob == 24-bit-quantised pristine; save → load → save byte-identical; > 16 MB omitted (D36) | ST2–ST5, HS2, HS3 (+ controls) | bitwise / identical bytes |
| Recordings search bounded; hash-mismatch relink flagged | RS4, RS7, RS8, RS8n | ≤ 1024 / ≤ 16 |
| `factory:` loads with no disk access (D33) | FA1, FA2, FA2n | `fsTouches` 0 |
| Allocation gate; host-idle bound (PERF-01 / PERF-03) | AL1, CM3, CM3n | 0 allocs; bounded |
| Commit paths: Timer, offline, no-MM watcher, prepare mid-record, bypass, WAV failure | CM1–CM6 | land / reason codes |
| Click gates vs same-motion controls (QUAL-01) | CK6a, CK6b, CK6n | ≤ 2.0×; control > 2.0× |
| Block-size null incl. pre-roll + seed control | BS1, BS1n | 0 diffs / differs |
| Telemetry feeds | TL1–TL3, TL1n | tracked; torn read caught |
| True bypass byte-identical; param-dump identical; drift 0 / 13 | TB0 / TB3–TB7, checkpoints, carried rows | identical |
| CPU regression | `--cpu26` | C25m ≤ 40 %, C25tm ≤ 12 % |
| Carried | harness, parity, GV | **300 → ≈ 360 / 0**; parity 168 / 0; GV 2 / 0 |
| auval PASS; pluginval 10 VST3 + AU with SC on and off; parent code excluded; real Recordings untouched | T18, X1, X2, GD1 | all PASS |
| HANDS-ON-2.6 written, deferred | T19 | 12 rows |

**Requirements this phase moves** (verify sets the final status): **FUNC-06** → automated arm complete; **FUNC-07** → automated arm complete (feel = HANDS-ON); **QUAL-02** engine path in (cross-host = Stage 4); **FUNC-08** groundwork (factory path) in; **PERF-01 / PERF-03** record / state arm in; **QUAL-01** commit / state-load click gates in.

## Pitfalls → task + gate (RESEARCH P1–P20)

| # | Pitfall | Task | Gate |
|---|---|---|---|
| P1 | Base-class bypass passes the SC | T3 (PD20) | LK2 + LK2n |
| P2 | FLAC float writer path | T8 (PD4) | ST2 + ST2n, HS3 |
| P3 | `MemoryBlock::toBase64Encoding` | T8 / T9 | ST3 |
| P4 | Quality index 0 == 5 | T8 | ST3 (bytes) |
| P5 | `getLargeIntValue` on the hash | T9 (PD2) | ST5 |
| P6 | XML bools as strings | T9 | ST5 + ST5n |
| P7 | `<Slots>` read after the strip | T9 (PD21) | ST1 |
| P8 | getState right after setState | T9 (PD21) | ST3 |
| P9 | R1 + loud SC commits | T4 (PD17) | R1 carried |
| P10 | N2 / N2s / FN3 vacuous | T4 / T6 | N2, N2s, FN3 |
| P11 | `rec_gain` 7e-7 dB default | T6 (PD5) | RC1 |
| P12 | Buffer grow while Committing | T6 (PD9) | CM4, AL1 |
| P13 | Arm edge via grid latch | T6 (PD5) | TH3, BS1 |
| P14 | Mono SC garbage ch 1 | T3 / T6 (PD6) | LK1 mono, RC4 |
| P15 | Sentinel deleted | T5 (PD11) | ST7, CM3 |
| P16 | BinaryData hyphens / link order | T10 | FA1, T10 link |
| P17 | Validators writing to `~/Music` | T18 | GD1, rec-dir snapshot |
| P18 | State size / save time | T20 INFO | HANDS-ON 11 |
| P19 | Non-ASCII Recordings path | T4 / T5 | CM1 |
| P20 | `replaceWithText` CRLF | T13 | ST3 (in-memory compare) |

## Stop-and-ask conditions (execute; never pre-decided)

- **S2 — CPU regression.** C25m > 40 % or C25tm > 12 % with valid witnesses. STOP with the per-tick profile.
- **S3 — bypass broken.** A checkpoint, TB0 / TB3–TB7, RB1 / FR1, a carried 2.5b row or a parity residual moves. STOP and find the statement-shape change. **Never re-baseline.**
- **S4 — leak.** Any LK row non-zero, or LK2n / LK1n cannot fire. STOP: QUAL-02 stability guarantee.
- **S5 — law arm** (RC / TH / OS / LN / MN / QD / CM / ST / RS / FA / HS / BS / AL / TL). Structural; STOP with the diagnosis. Never loosen.
- **S6 — clicks.** CK6a / CK6b over 2.0×, or CK6n cannot fire after one stimulus fix. STOP: a kill-fade change amends the 2.1 swap rule — consent required.
- **S8 — scope.** A forked-file byte edit or a module edit appears necessary; a carried row other than N2 / N2s / FN3 / R1 needs a re-scope; a param or `<Engine>` key change appears necessary. STOP: the user's call.
- **Inherited:** T1 contract / late-change mismatch; INVALID after one stimulus step; three NOVERDICTs; unexpected paths at either commit; any new file in the real `~/Music/…/Recordings` during the harness.

## Risks

- **R1 Leak regression.** The fix moves the output clear into the engine; LK1 / LK2 with hard controls on every layout × state pin it, and LK2n fires on the old behaviour.
- **R2 Bypass shape.** Nine engine / loader tasks touch the audio path; per-task checkpoints + full `$H` attribute any move.
- **R3 Threading.** Copy-out on the message thread, build on the worker, host-thread restore; every `SlotInfo` / `SlotState` write under the loader lock; CM3 + AL1 bound it.
- **R4 Factory bootstrap.** CMake needs the FLACs before the target exists; T10's ordered bootstrap avoids a broken configure.
- **R5 Cross-arch FLAC bytes.** Pins are decoded-PCM hashes; file sha256 is mac arm64 only (INFO elsewhere). Blob carry means bytes never need to reproduce.
- **R6 Validators touching real user folders.** Recordings snapshot before / after T18; lazy directory resolution keeps construction side-effect-free.
- **R7 Concurrent sessions.** Foreign dirty paths exist; both commits path-scoped with branch + status re-checked immediately before.
- **R8 Test hooks in shipping.** All under `OGI_TEST_HOOKS`; X2 and validators run on the shipping build.
