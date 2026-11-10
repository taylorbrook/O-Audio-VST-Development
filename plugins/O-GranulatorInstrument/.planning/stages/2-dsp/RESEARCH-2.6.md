# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.6 — RESEARCH

**Researched:** 2026-11-09
**Scope:** Phase 2.6 only (D33–D36 + the binding Defaults): Record-in capture (Manual / Threshold / Next Note), the commit hand-off, `<Slots>` state + resolve order + relink + FLAC-24 embedding, `factory:` sources (BinaryData target + 2 generated stubs), the 2.6 telemetry feeds, and the law-gate proof arms. There is no parent equivalent (O-Granulator @ `82406fce` has no Record-in), so there is no parity arm.
**Confidence:** HIGH on code facts (tree at `0e965186`, engine at `c7efbee7`; JUCE 8.0.15 at `~/JUCE`, pristine + the NE patch, which touches no FLAC / bypass / format code). HIGH on the FLAC / quantiser facts: a scratchpad probe linked against the pinned JUCE modules and ran (results below). MEDIUM on the designs in §3–§8: they are designed here, not built. Every claim is tagged **VERIFIED** (read or run this session, `path:line`) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.6.md` (D33–D36, Defaults, test criteria, constraints, 8 open questions); `RESEARCH-2.5b.md` (format); `SUMMARY-2.5b.md` / `VERIFICATION-2.5b.md` (gate conventions); `research/ARCHITECTURE.md` §1, §5, §6, §7, §17, §22, Processing Chain, File I/O and Sources, State Persistence, Record-In Availability, Threading Model, Telemetry, A7, Memory, Gate Catalogue; `ROADMAP.md` § 2.6; `REQUIREMENTS.md` (FUNC-06/07/08, PERF-01/03, QUAL-01/02); `parameter-spec.md` (rec_* rows, S8, `:353`); `params.tsv:12, 179-185`
- Plugin: `Source/{PluginProcessor.h, PluginProcessor.cpp, GiParamIds.h}`, `Source/dsp/{Engine.h, Engine.cpp, SlotLoader.h, SlotLoader.cpp, SlotHandoff.h, SlotTape.h, MipTape.h, GiRecordHead.h, KaiserResampler.h, GiConstants.h, GiSpawner.h, VoiceGrain.h}`, `forked/{UiTelemetry.h, ControlGrid.h, TempoTracker.h, EngineConstants.h, FORK-MANIFEST.md}`, `CMakeLists.txt`, `tests/{fork-drift-check.sh, render-harness/main.cpp, engine_gates_21.h, engine_gates_22.h, engine_gates_23.h, engine_gates_24b.h}`
- Repo: `scripts/param-dump/ParamDump.cmake`; `plugins/O-Strata/Source/PluginProcessor.cpp:1220-1361` (SHA-256 + Base64 + missing / Locate precedent); `plugins/O-simpleSampler/Source/PluginProcessor.cpp:362-422, 666-714` (`embedded:` + `MemoryInputStream` + standard Base64 precedent); `troubleshooting/patterns/juce8-critical-patterns.md` §4
- JUCE 8.0.15: `juce_audio_formats/codecs/juce_FlacAudioFormat.cpp`, `format/juce_AudioFormatWriter.cpp`, `format/juce_AudioFormatReader.cpp`, `juce_audio_basics/buffers/juce_FloatVectorOperations.cpp`, `juce_core/maths/juce_MathsFunctions.h`, `juce_core/memory/juce_MemoryBlock.cpp`, `juce_events/broadcasters/juce_AsyncUpdater.cpp`, `juce_events/native/juce_MessageQueue_mac.h`, `juce_audio_processors_headless/processors/juce_AudioProcessor.{h,cpp}`, `juce_audio_plugin_client/{..._VST3.cpp, ..._AU_1.mm}`, `extras/Build/juce_build_tools/utils/juce_BuildHelperFunctions.cpp`
- Memory: AsyncUpdater offline gap, operator-new counter blind to HeapBlock / async post, `getLargeIntValue` clamps uint64, ValueTree XML round trip loses type, AU unrouted sidechain = silence, generation reaper freezes on host idle, TextureForge swap lock, BinaryData strips hyphens, dual BinaryData namespace, `replaceWithText` CRLF, JUCE mac home = NSHomeDirectory, `String(const char*)` ASCII-only, pedalboard verify
- Web: xxHash `LICENSE` (BSD 2-Clause), `doc/xxhash_spec.md` (XXH64 algorithm, permissive doc licence), LLVM `xxhashTest.cpp` known-answer vectors

**Probe (scratchpad, not in the repo):** `juce_core` + `juce_audio_basics` + `juce_audio_formats` from `~/JUCE` compiled as separate TUs, arm64, `-O1`. It also ran a standalone XXH64 + quantiser probe. The outputs are quoted where used.

## User Constraints (CONTEXT-2.6; binding)
- **D33:** factory sources in 2.6 = the path + 2 synthetic stubs. Enable the `FactorySourceData` `juce_add_binary_data` target with its own namespace and header (hyphen stripping; `BinaryData` namespace collision). Fill it with 2 small generated FLACs (deterministic, licence-clean, checked-in generator; e.g. a harmonic tone and a noise texture, a few seconds each). Build the `factory:<id>` table and resolution. A `factory:` load decodes from memory with **no disk access**. The curated 6–8 sources (≤ 8 MB) and the presets are FUNC-08, Stage 4; they replace the stubs by id.
- **D34:** `rec_arm` is one-shot. Off→On = Idle → Armed. After a commit (or a discard) the engine returns to Idle and stays there while `rec_arm` is still On; another capture needs a new Off→On edge. The engine never writes APVTS. On→Off while Armed cancels; On→Off while Recording stops (≥ 0.5 s commits, shorter is discarded).
- **D35:** Threshold has a 10 ms pre-roll. While Armed in Threshold mode, a preallocated 10 ms ring runs on the post-`rec_gain` sidechain. The capture starts 10 ms before the first sample where `|sc·recGain|` exceeds `rec_threshold`, clipped at the arm sample. Pre-roll counts toward `rec_length`. Manual (arm edge) and Next Note (note-on offset) are sample-exact, with no pre-roll.
- **D36:** embedding as ARCH, 24-bit FLAC. Recordings embed by default; user files are opt-in per slot. `embed` is a native action and `<Slots>` state, never a parameter. Factory sources never embed. The blob is base64 FLAC-24 of the **pristine** at its native rate. Encoded > 16 MB → not embedded (`embed` stays set, `embedded` absent) and the slot resolves by path / hash. "Bit-identical" means (1) the decoded blob == the 24-bit-quantised pristine, sample for sample, and (2) save → load → save re-emits the **same bytes** (the blob is carried, never re-encoded). The float32 WAV on disk stays the master.
- **Defaults (binding):** run state Idle / Armed / Recording / Committing (an engine atomic, never a parameter; an arm edge during Committing is held, then goes Armed). Length = `rec_length` s, or `rec_length_sync` × host bar only under `sync_mode` Tempo **and** host playing, cap `F_max(fs)`, bar length latched at the start sample. Next Note = the first note-on after arming that mints a serial or starts a cloud (a Legato same-serial pitch change does not count). D6 presence: take peak < −90 dBFS → discarded, slot unchanged, reason "No sidechain signal". No input bus → arm ignored, Idle, "Sidechain unavailable". The sidechain **never** reaches the output, in every state and layout. Commit = audio thread sets `captureReady(frames)`; off-audio copy-out; pristine = capture at fs; the same `MipTape::writeRun` path; publish via `SlotHandoff` (new-source rule); a 32-bit float WAV in `userMusicDirectory/Ouaricon/O-GranulatorInstrument/Recordings/`; source `rec:<abs path>`; no audio-thread lock or allocation. Recording into a playing slot is allowed. The `<Slots>` schema (source / name / fileSize / contentHash / embed / embedded / trimmed; no working copy, no rootNoteHint). contentHash = xxHash64 of the first 1 MB of 24-bit-quantised PCM. Resolve order: path (exists + size) → blob → Recordings filename + hash → `factory:` → missing; a missing slot keeps its source string. Relink = `requestRelink(slot, File)`. `setStateInformation` only parses + enqueues; an empty source clears the slot through the publish path. The encode runs on the worker; `getStateInformation` never encodes. The telemetry feeds list. True bypass byte-identical; harness 300/0, `--parity` 168/0 + GV 2/0, param-dump identical, drift 0/13. PERF-01 alloc span, host-idle bound, local-κ click gates, block-size null (3 triggers, pre-roll included), CPU regression + INFO while Recording, HANDS-ON-2.6 written and deferred, explicit Standalone rebuild.
- **Out of scope:** curated sources + presets (Stage 4), relink / load / record UI and lanes (3.2), Record-in as a mod source, embedding the working copy, cross-host DAW sidechain verification.

## Project Constraints
- Trunk on `main`. Path-scoped commits only (`git commit -- plugins/O-GranulatorInstrument`). Re-check `git branch --show-current && git status --short` right before each commit. Foreign dirty paths at research time (**VERIFIED**, session-start snapshot): `.claude/agent-memory/{dsp,gui}-agent.md`, `.planning/STATE.md`, `modules/registry.yaml`, untracked `.claude/compaction-snapshot.md`, `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`. Never sweep them in. Never tag.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument` (it clears the AU cache and sweeps the `-dev` variant). Long commands go in the background (600 s watchdog). Rebuild the Standalone explicitly before any Standalone check.
- New code goes in Gi / instrument-owned files. `SlotLoader.*`, `SlotHandoff.h`, `Engine.*` and `PluginProcessor.*` are not forked (FORK-MANIFEST lists 13 `forked/` leaves only), so editing them leaves drift at 0/13. No module edit is needed.
- No external package is installed. XXH64 is written from the spec, not vendored. Package legitimacy audit: **none installed**.

---

## Summary (what the plan must absorb)

1. **The sidechain leaks to the output TODAY on host bypass.** `processBlockBypassed` (`PluginProcessor.cpp:287-292`) calls the base, and JUCE's `processBypassed` only clears channels `[getMainBusNumInputChannels(), getTotalNumOutputChannels())` (`juce_AudioProcessor.cpp:601-602`). JUCE sees the Sidechain as the main input bus (bus 0, `juce_AudioProcessor.h:772`). So with a stereo SC it clears nothing, and with a mono SC it leaves ch 0. Both wrappers call it when the plugin has no bypass parameter: VST3 `juce_audio_plugin_client_VST3.cpp:3774-3777`, AU `..._AU_1.mm:2192-2196`. **Fix:** clear every channel in the override (after the NE drop). Gate LK2 must fire on the base-class behaviour. **VERIFIED.** [correction: ARCH §1 "the output is cleared" holds for `processBlock` only]
2. **JUCE's float FLAC writer path cannot produce the D36 reference.** `convertFloatsToInts` = `roundToInt (INT_MAX · x)` with clip at ±1 (`juce_AudioFormatWriter.cpp:73-88`). Then the FLAC writer applies an arithmetic `>> 8` (`juce_FlacAudioFormat.cpp:458-472`), so the 24-bit value is floor(round32 / 256), not round-to-nearest. The decoder gives `(q << 8) · (1/0x7fffffff f)` = **q · 2⁻²³ exactly** (`juce_AudioFormatReader.cpp:48-55`, `juce_FloatVectorOperations.cpp:1167-1180`; float(0x7fffffff) = 2³¹). Probe: re-quantising decoded 24-bit floats through the float writer path returns **q − 1 for 4 194 302 of 16 777 216 codes** (every q ≥ 4 194 306). On a 30 s stereo buffer the float path differs from round-to-nearest in **1 218 346 / 2 880 000** samples. **Use our own quantiser `Q(x) = clamp (nearbyint (x · 2²³), −2²³, 2²³ − 1)` and feed the writer integers** (`AudioFormatWriter::write (const int**, n)` with `q << 8`). Probe: Q(deq(q)) == q for all 2²⁴ codes, and the blob decodes **bitwise** to Q(x)·2⁻²³ (0 mismatches over 2.88 M samples). **VERIFIED (run).** §5.
3. **FLAC-24 encode is deterministic within a build; it is not proven across platforms.** Two runs gave identical bytes at quality 0 / 5 / 8. Quality index 0 == index 5, because JUCE only calls `set_compression_level` when index > 0 (`juce_FlacAudioFormat.cpp:417-418`). STREAMINFO MD5 is computed and patched back through `MemoryOutputStream::setPosition` (`:504-526`). The vendor string is the constant `reference libFLAC 1.4.3 20230623` (`flac/libFLAC/format.c:58`): no timestamp. Cross-arch bytes may differ (float LPC analysis, NEON vs x86 intrinsics, `:126-150`). D36's carry rule makes that irrelevant: a blob is never re-encoded. Factory pins must pin the **decoded PCM hash**, not the file bytes. **VERIFIED (run)** / **INFERRED** (cross-arch).
4. **xxHash64: write it from the spec in `GiHash.h` (~40 lines). Do not vendor.** The xxHash library is BSD-2-Clause (AGPL-compatible). The spec document grants free use. Probe: our implementation matches the 4 LLVM known-answer vectors (`""` → `ef46db3751d8e999`, `"foo"` → `33bf00a859c4ba3f`, `"bar"` → `48a37c90ad27a659`, 36-char → `69196c1b3af0bff9`), which covers both the < 32-byte and the stripe path. Canonical hashed stream: §5.3. **VERIFIED (run)**. Alternative: `juce::SHA256` truncated, the O-Strata precedent (`O-Strata/Source/PluginProcessor.cpp:1220`); it needs `juce_cryptography` on the plugin target. **[decision for plan]**
5. **Capture placement: inside `Engine::process`, per piece.** At each piece, copy `sc[pos, b)` into a fixed 2×32 scratch, then zero `out[pos, b)` (ARCH §1, literally). Then run the event loop (Next Note stamps arrive from the two serial-mint sites, `Engine.cpp:2237` / `:2578`). Then `recorder.runPiece` over the scratch, then `mixPiece`. `mixPiece` **assigns** every output sample of the piece (`Engine.cpp:3850-3852`), so moving the clear from `processBlock` (`PluginProcessor.cpp:217-222`) into the engine leaves renders-2.4 byte-identical. `processBlock` passes `scL = ch0` (when SC ≥ 1 ch) and `scR = ch1` (when 2 ch), from the negotiated `sidechainChannels`. It clears every channel when the engine is not called. **VERIFIED** (code) / **INFERRED** (design). §3.
6. **Arm edge = block start (not the grid tick).** Read the `rec_arm` / `rec_target` / `rec_trigger` raw atomics once at the top of `Engine::process`. The harness cuts blocks at `s.at (t)` (`engine_gates_21.h:473-475`), so the stamp equals the event time at every block size. A host only changes parameters between blocks, so the block start is the most exact sample available. **INFERRED** **[decision for plan]**.
7. **Commit hand-off: atomic `ready` + an off-audio poll. AsyncUpdater is not RT-safe here.** `triggerAsyncUpdate` → `MessageQueue::post` → `ReferenceCountedArray<…, CriticalSection>::add`: a lock plus a possible realloc (`juce_MessageQueue_mac.h:65-72`, `juce_AsyncUpdater.cpp:74-84`). Recommend a message-thread `juce::Timer` (20 ms) that copies out only (memcpy ≤ 23 MB). Then `SlotLoader::requestCommit` builds the tape on the **worker**, along with the WAV, hash and FLAC. Without a MessageManager, use a fallback watcher thread (the `SlotLoader` precedent, `SlotLoader.cpp:122-126, 376-389`). The harness gets a `commitSync()` hook (the `loadSync` pattern) plus `pump()` rows. **VERIFIED** (JUCE) / **[decision for plan]** [correction to ARCH §22: the message thread does not build the tape. ARCH Threading Model already puts the tape build on the worker]
8. **The capture buffer is ≤ 23.04 MB, not ≈ 46 MB.** `F_max(fs) = min (⌊30·fs⌋, kSlotMaxFrames = 2 880 000)` (`GiConstants.h:66-67`; S8: 15 s at 192 k). Stereo float = 23.04 MB at ≥ 96 k, 11.52 MB at 48 k. It is allocated grow-only in `prepareToPlay`. **VERIFIED** [correction to CONTEXT constraint "≈ 46 MB at 192 k"]
9. **Any `prepareToPlay` interrupts a capture, not just an SR change.** `Engine::prepare` resets `absPos = 0`, the voices and the serials (`Engine.cpp:81-100`). Q6 recommendation: Recording → **commit at the old rate** (≥ 0.5 s and presence; the commit record carries its rate, and the loader resamples a native ≠ session pristine anyway). Armed → cancel (reason Interrupted). Committing survives, and prepare skips the grow while `ready != 0`. **VERIFIED** / **[decision for plan]**. §4.4.
10. **There is no slot-clear mechanism today.** `pending[s]` exchange `nullptr` means "nothing pending" (`SlotHandoff.h:88-93`). A restore with an empty source and a missing resolve on a loaded slot both need one. Recommend a static, never-freed **clear sentinel** tape published through the same exchange. The engine kill-fades the current tape's grains (5 ms), retires it, and sets `current[s] = nullptr`. Ordering stays the single-slot exchange. **VERIFIED** (gap) / **INFERRED** (design). §6.4.
11. **Telemetry: the "2.1 grain snapshot" does not exist.** Today there are only the 2.5a PD16 relaxed atomics `destTel[126]`, `srcTel[18]`, `lfoPhaseTel[4]` (`Engine.h:836-840`, `Engine.cpp:1542-1559`). `forked/UiTelemetry.h` is "ADAPTED (pending 2.6)" and compiled nowhere (grep). Its `UiSnapshot` is the parent's ring-domain buffer strip (`:97-113`). Recommend: reclassify `UiTelemetry.h` to **VERBATIM** and reuse `TripleBuffer` / `pushMax`, which are templated and so have no capacity limit (`:54-80, :216-220`). Put the instrument frame in a new `GiTelemetry.h` (288 grains, ≈ 5.3 KB per frame). Drift stays 0/13. **VERIFIED** [correction to CONTEXT "the 2.1 grain snapshot is extended"]. §8.
12. **`juce_add_binary_data` traps are confirmed, plus one more.** Identifiers keep `[A-Za-z0-9_]`, turn spaces and dots into `_`, and drop hyphens (`juce_BuildHelperFunctions.cpp:227-233`), so `stub_tone.flac` → `stub_tone_flac`. The consoles inherit the plugin's non-JUCE link targets except names ending `UIResources` (`ParamDump.cmake:221-226`), so harness and param-dump link `…_FactorySources` automatically. This only works if `target_link_libraries` runs **before** `ouaricon_add_processor_console` (it does at `CMakeLists.txt:172-178` vs `:206+`). **VERIFIED.**
13. **Harness re-scope.** Once `rec_*` are consumed, every one of the 261 IDs is read, so N2 / N2s / FN3 (`bound == 254 && touched == 7`, `engine_gates_21.h:2154`) go vacuous. Re-scope them as **`readSet26` = 261 bound; touched = the 7 `rec_*` randomised with a silent sidechain → output bitwise identical**. The D6 discard now proves it. R1's random-parameter mode (`main.cpp:738-744`) also starts reaching `rec_arm` with a loud stimulus. Pin `rec_arm` = Off there and move capture-on leak coverage to LK1. **VERIFIED** / **[decision for plan]**.
14. **`rec_gain` defaults to 7e-7 dB, not 0** (`params.tsv:185`). Use `ControlGrid`'s `dbToGain` (`snapDb`: |dB| < 1e-3 → 0, `ControlGrid.h:141-142`). Otherwise the default capture is scaled by float(1 + 8.1e-8) = 1 + 1.19e-7, and "PCM == sc × g" holds only with an odd g. **VERIFIED.**

---

## 1. Exact current state — **VERIFIED**

| piece | today (`0e965186`) | 2.6 |
|---|---|---|
| `processBlock` | clears **every** channel first (`PluginProcessor.cpp:217-222`); engine gets out ptrs only (`:277-279`) | clear ch ≥ 2; pass `scL/scR` + count; engine copies sc → scratch then zeroes `out[piece]`; clear all if the engine is not called |
| `processBlockBypassed` | NE drop + **base** (leaks SC, Summary 1) (`:287-292`) | NE drop + `buffer.clear()` + `engine->bypassedBlock()` (§4.5) |
| `sidechainChannels` | from the negotiated bus in ctor + `prepareToPlay` (`:171-176, :185`) | the recorder's availability input (0 → "Sidechain unavailable") |
| `isBusesLayoutSupported` | out stereo; in disabled / mono / stereo (`:197-206`) | unchanged |
| `Engine::process` | `(outL, outR, n, midi, seed, ts, ne, numNe)` (`Engine.h:468-470`); pieces at the 32-grid, MIDI/NE merged cursor, `mixPiece` per piece (`Engine.cpp:608-711`) | + `scL, scR` (default nullptr); scratch copy + `out` zero per piece; `recorder.runPiece` before `mixPiece` |
| serial mints | `startVoice` `:2237`, `retriggerVoice` `:2578` (only these two) | Next Note hook at both |
| `getStateInformation` | appends an empty `<Slots/>` (`PluginProcessor.cpp:492`) | `<Slot index …>` per non-empty slot from the loader records (under the loader lock) |
| `setStateInformation` | `// Phase 2.6` marker (`:513`); `<Slots>` stripped before `replaceState` (`:445-453, :518`) | read `<Slots>` before the strip; write records + enqueue resolve jobs |
| `SlotInfo` | name, frames, rates, trimmed, loaded, loading, lastError, generation (`SlotLoader.h:63-76`): no source / missing / hash | + source, missing, embed, hasBlob, contentHash, hashMismatch, relinkMismatch |
| `SlotLoader` | 1 worker; decode-by-File only (`SlotLoader.cpp:156-213`); reaper Timer 100 ms only if a MessageManager exists (`:122-126`); `callAsync` SR rebuild with a no-MM fallback (`:376-389`) | + memory decode (blob / factory), commit job, resolve job, relink, embed toggle, hash, WAV, FLAC |
| `SlotHandoff` | `pending[4]` exchange, SPSC retire ring 16, deferred + entry/exit counters (`SlotHandoff.h:85-140, 190-216`) | + clear sentinel (never deleted) |
| `takeTapes` | waits for a parked head; new source → `killGrainsOfTape` (5 ms); same-gen rebuild → 20 ms dual-read (`Engine.cpp:911-972`) | + sentinel branch (kill + retire + `current = nullptr`); a commit is a new source |
| telemetry | `destTel` / `srcTel` / `lfoPhaseTel` (`Engine.h:836-840`) | + `GiTelemetry` (§8) |
| CMake | `FactorySourceData` target commented (`CMakeLists.txt:165-178`); UI target commented (Phase 3.1) | enable `…_FactorySources` (NAMESPACE `FactorySourceData`, HEADER_NAME `FactorySourceData.h`), link before the console block |
| params | `rec_*` never bound (`ControlGrid.h:198`); `sync_mode` latched in `gp` (`:281, 380, 491, 597`) | `rec_*` bound in the new `GiRecorder` (raw atomics), **not** in `ControlGrid.h`, so no manifest row changes |
| harness | Session: 2-ch block cleared each block; `beforeBlock (s, pos, n)` after MIDI gather, before `processBlock` (`engine_gates_21.h:404-406, 478-492`); `pump()` = `CFRunLoopRunInMode` (`:142-156`); alloc hook `malloc_logger`, thread-scoped, armed around `processBlock` from the 2nd block (`:98-118, 493-499`) | SC injection through `beforeBlock`; `commitSync`; a Recordings-dir override in `make()` |

## 2. Run-state machine and triggers (D34, D35, Defaults)

### 2.1 States — **INFERRED** (design over the binding Defaults)
`Idle → Armed → Recording → Committing → Idle`. These are published as `std::atomic<uint32_t> recState` = state | (reason << 8) | (heldArm << 16). Reason codes: `None, SidechainUnavailable, NoSignal, TooShort, Cancelled, Interrupted, Bypassed, Committed, WavFailed`. parameter-spec `:353` names "Done / Discarded" states. Those map to Idle + a reason; CONTEXT wins, and Stage 3 binds to the reason. [reconcile, Stage 3]

| event (sample) | Idle | Armed | Recording | Committing |
|---|---|---|---|---|
| arm Off→On (block start) | SC ch = 0 → stay, `SidechainUnavailable`; else Armed, `armAt = blockStart`, latch target + trigger | — | — | set `heldArm` (D: held, not dropped) |
| arm On→Off (block start) | — | Idle, `Cancelled` | stop at blockStart (§2.3) | clear `heldArm` |
| Manual | — | → Recording at `armAt` (same sample) | — | — |
| Threshold crossing c (`\|x\| > thr`, per sample) | — | → Recording, start = `max (armAt, c − P)` (ring copied in) | — | — |
| Next Note at t (serial minted) | — | → Recording at t (first one wins) | — | — |
| length reached | — | — | stop at the end sample | — |
| `ready` seen 0 (block start) | — | — | — | Idle; if `heldArm` → Armed at blockStart |
| `prepareToPlay` | — | Idle, `Interrupted` | stop (commit at old rate, §4.4) | survives |
| bypassed block | — | Idle, `Bypassed` | stop (§4.5) | survives |

- D34 one-shot: the edge detector holds `prevArm` and only an Off→On edge arms. `prepare` sets `prevArm` = the current value, so an arm held On never re-arms on its own.
- No-bus arm: `prevArm` updates too. Enabling the bus later while the arm is still On does nothing until a new edge.

### 2.2 Per-sample capture (`runPiece`) — **INFERRED**
- `x[n] = sc[n] · g[n]` in float. g = the `rec_gain` ExactRamp at the 32-grid (20 ms, `dbToGain` with `snapDb`). This is the stored PCM, so "PCM == sc × g" is bitwise while g is settled. Mono SC: one channel, captured **mono** (pristine 1 ch; "mono stays mono"). **[decision for plan]** (vs duplicating L = R)
- Threshold: `thrLin = Decibels::decibelsToGain (rec_threshold)`, latched at the grid tick. Crossing = the first n with `max (|xL|, |xR|) > thrLin` (strict). It is −60 dB at most, so it never fires on silence by construction.
- Pre-roll ring: `P = lround (0.010 · fs)` frames (441 / 480 / 960 / 1920), stereo, allocated in prepare. It is filled per sample while Armed + Threshold. At the crossing, copy the last `min (P, c − armAt)` samples into `cap[0 …)` and continue at c.
- Running peak (D6) over the whole take, pre-roll included. Stop → frames ≥ `minFrames = ceil (0.5·fs)` **and** peak ≥ 10^(−90/20) → commit, else discard (`TooShort` / `NoSignal`), slot untouched.
- Length (latched at the start sample): Free, or Tempo with the host stopped → `llround (rec_length · fs)`. Tempo + `tracker.isPlaying()` → `llround (bars · barLen · 60 · fs / bpm)` from the tracker anchor (`TempoTracker.h` `a.bpm / a.barLen`). Both capped at `capFrames = min (F_max (fs), capacity)`. **VERIFIED** (tracker fields) / **INFERRED**.
- `rec_target` and `rec_trigger` latch at Armed entry (no retarget mid-take). `rec_threshold` / `rec_gain` latch at grid ticks. `rec_length` / `rec_length_sync` / `sync_mode` latch at the start sample. **[decision for plan]**

### 2.3 Next Note (Mono / Legato)
The hook sits in `startVoice` (`Engine.cpp:2237`) and `retriggerVoice` (`:2578`), the only two `++serialCounter` sites (grep). Poly note-ons, steals, the PD9 re-strike, a Mono retrigger and a Mono note-on on a releasing cloud all mint, so they count. Legato `moveCloudPitch` and the D30 fallback do not mint, so they do not count. A note-on that finds no voice slot (`allocateVoice` → −1) mints nothing and does not count (documented). **VERIFIED** (sites) / **INFERRED**.

## 3. Capture placement in `processBlock` / `Engine::process` (Q3)

### 3.1 Order per piece `[pos, b)` — **INFERRED** on **VERIFIED** structure
1. `cellOff == 0` → `gridTick (t)`: threshold / gain ramps latch here.
2. **Sidechain → scratch, then clear:** `scS[ch][cellOff + i] = sc*[pos + i]` (nullptr → 0; mono → R = L), then `fill (outL + pos, outL + b, 0)` and the same for R. The buffer is in place, so `outL` and `scL` alias for a mono or stereo SC.
3. The merged MIDI / NE event loop + `runSegment` (`Engine.cpp:676-705`). A Next Note stamps `pendingStart = t`.
4. `recorder.runPiece (cellOff, cellOff + len, t, scS)`: per sample, in absolute order, handling the start / stop stamps inside the piece.
5. `mixPiece` assigns `out[pos, b)` (`:707`, `:3850-3852`).

Why this works:
- (a) The scratch read precedes every write of that piece, and no earlier piece writes ahead.
- (b) The explicit zero keeps the leak guarantee local even if a future `mixPiece` accumulates.
- (c) The bytes are identical to the old top-of-block clear, because `mixPiece` overwrites every sample. **renders-2.4 / legacy stay byte-identical.**
- (d) Everything is absolute-sample-ordered, so it is block-size invariant.
- (e) No allocation: the 2×32 scratch is a member.

### 3.2 `processBlock`
```
blockEntered; ScopedNoDenormals; drainAndUpdate
n, nCh, scCh = sidechainChannels (relaxed)
for ch in [2, nCh): clear                         // never an output
… transport, NE conversion (unchanged) …
if (n > 0 && nCh >= 2)  engine->process (ch0, ch1, n, midi, seed, ts, ne, numNe,
                                         scCh >= 1 ? ch0 : nullptr, scCh >= 2 ? ch1 : nullptr)
else                    buffer.clear()
midi.clear(); blockExited
```
The engine's early-return guard (`Engine.cpp:612-613`) must zero the outputs too, or `processBlock` clears on that path.

## 4. Commit hand-off (Q4), host idle, SR change (Q6), bypass

### 4.1 Mechanism — **VERIFIED** (JUCE) / **INFERRED** (design)
- **Audio thread (stop):** fill `commit = {slot, frames, rate = fs, channels, startAbs}`, then `ready.store (frames, release)`, state Committing. Nothing else.
- **Poll (message-thread `juce::Timer`, 20 ms):** `f = ready.load (acquire)`; if f > 0 → under `captureLock` (off-audio only) copy `cap[0, f)` into a heap `Pristine {pcm, rate, name}` (≤ 23 MB memcpy, ≈ ms), then `ready.store (0, release)`, then `loader->requestCommit (slot, pristine)`.
- **Worker job (commit):** hash → WAV (float32, Recordings dir, unique `GI-Rec-YYYYMMDD-HHMMSS-S<n>.wav` via `getNonexistentSibling`) → `buildTape` (equal rates = bit-exact copy, `KaiserResampler.h:47, 94-96`) → **publish with source / fileSize / hash set atomically under the loader lock** → FLAC-24 encode → blob cached (generation-checked).
- **Order: WAV before publish**, so a save between them never records a source that has no file. A WAV failure still publishes: source `rec:<intended path>`, the blob carries the take, reason `WavFailed`. **[decision for plan]**

### 4.2 Why not AsyncUpdater / why not the message thread for the build
- `triggerAsyncUpdate` posts through the mac `MessageQueue`: `ReferenceCountedArray<MessageBase, CriticalSection>::add` (`juce_MessageQueue_mac.h:65-72`). That is a lock plus a realloc on growth, on the audio thread. The memory `operator_new_counter_blind…` already records the first-block post realloc. The offline-gap memory adds pacing on top. **Rejected.**
- `MipTape` build of 2.88 M frames (4 levels of 127-tap halfband) is hundreds of ms. On the message thread it would freeze the UI. ARCH Threading Model already assigns "tape build, overview, WAV write, FLAC encode" to the worker.
- **Offline renders:** the Timer still fires in a DAW offline bounce (the message thread lives). The swap lands at a wall-clock-dependent render position, exactly as in realtime. Correctness does not depend on pacing (the audio thread never waits on the commit). A held re-arm during an offline bounce also resolves at a wall-clock-dependent sample (documented). **Without a MessageManager** (`getInstanceWithoutCreating() == nullptr`, the `SlotLoader.cpp:124, 378` precedent), start a small `juce::Thread` watcher doing the same poll. **[decision for plan]**
- **Harness:** `commitSync()` (OGI_TEST_HOOKS) runs poll + job on the calling thread, for deterministic rows. One CM row also exercises the real Timer through `pump()` (the `engine_gates_21.h:1847` precedent). R1 never pumps, so it never commits (Summary 13).

### 4.3 Host-idle bound — **INFERRED** on **VERIFIED** handoff
The copy-out and publish never need the audio thread. `pending` holds ≤ 1 tape per slot by exchange, and superseded tapes free on quiescence (`SlotHandoff.h:194-216`; entries == exits with no block running). Gate CM3: stop `processBlock` right after `ready` is set (mid-commit) and right after the publish (mid-swap). Then do 50 relinks + 50 state restores with the loop pumped: pending ≤ 1 per slot, deferred freed, `SlotTape::liveCount` bounded. Resume audio: the take lands, the retire ring drains to 0. Control: `testNoEntryCounter` (H2's) → deferred grows.

### 4.4 `prepareToPlay` while capturing (Q6) — **[decision for plan]**
`Engine::prepare` hard-resets the clock and voices (`Engine.cpp:85-100`), so *any* prepare (SR or block-size change, re-activation) interrupts.
- **Recommended:**
  - Recording → stop at the last processed sample, then the normal commit / discard rule. The commit carries `rate = fs_old`. The loader builds the tape at the new session rate via the Kaiser path, and the pristine stays at fs_old.
  - Armed → Idle, `Interrupted`.
  - Committing → untouched.
  - Buffer: under `captureLock`, grow-only to `F_max (fs_new)`. Skip the grow while `ready != 0`; `capFrames = min (F_max, capacity)` until the next prepare.
- **Alternative:** discard on any prepare. This is simpler, but a buffer-size change loses the take.

The `Engine::prepare` setter runs with audio stopped (JUCE contract), so it can touch recorder state like the rest of `prepare`.

### 4.5 Bypass while capturing — **[decision for plan]**
`processBlockBypassed` (audio thread) calls `engine->bypassedBlock()`: Armed → cancel (`Bypassed`), Recording → stop (commit ≥ 0.5 s). Hosts that stop calling render during bypass (possible in Logic) instead splice a gap into the take on unbypass (documented; HANDS-ON row).

## 5. Hash, quantiser, FLAC-24 (Q1, Q2)

### 5.1 Quantiser and reference — **VERIFIED (run)**
- `Q(x) = (int32) clamp (nearbyint ((double) x · 8388608.0), −8388608.0, 8388607.0)` (round-half-even in the default FP mode). `deq(q) = q · 2⁻²³`. This is exactly what JUCE's FLAC reader returns. The 24-bit-quantised pristine = `deq (Q (pristine))`.
- Write path: `writer->write (int* const[] {q_L << 8, q_R << 8, nullptr}, n)` in chunks (the writer copies into `HeapBlock`s per call, `:455-470`, which is fine on the worker). **Never** `writeFromAudioSampleBuffer` for FLAC (Summary 2).
- Hot takes: the capture is float (`rec_gain` up to +12 dB, so |x| > 1 is possible). The WAV keeps it; the blob clips at ±1 through Q. Rec: keep the capture unclamped (the WAV is the master; FUNC-07 "PCM == sc × g" stays exact). Document that a blob-only reload of a take over 0 dBFS is clipped. **[decision for plan]** (vs clamping the capture at ±1)

### 5.2 FLAC-24 memory paths — **VERIFIED (run)**
- Encode: `MemoryBlock mb; { std::unique_ptr<OutputStream> os (new MemoryOutputStream (mb, false)); auto w = FlacAudioFormat().createWriterFor (os, AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (C).withBitsPerSample (24).withQualityOptionIndex (5)); … w->write (…); }`. The writer's destructor finishes and patches STREAMINFO (seek-back works on `MemoryOutputStream`), and `mb` outlives the writer. Use quality **5, explicitly** (index 0 is silently also level 5).
- Decode (blob and factory, no disk): `std::unique_ptr<AudioFormatReader> r (FlacAudioFormat().createReaderFor (new MemoryInputStream (data, size, false), true))` (not deprecated, `juce_FlacAudioFormat.h:69`). The existing chunked `reader->read (float* dst[], …)` (`SlotLoader.cpp:192-206`) can be factored into `decodeReader (reader, name, fsSession, out, cancel)`, shared by File / blob / factory.
- Base64: `juce::Base64::toBase64` / `convertFromBase64` (RFC 4648). **Not** `MemoryBlock::toBase64Encoding`, which writes a JUCE-private `"<size>." + custom alphabet` form (`juce_MemoryBlock.cpp:366-381`). O-Strata (`:1287, :1327`) and O-simpleSampler (`:666-714`) are the precedents.
- Measured sizes (stereo, quality 5): 30 s @ 48 k full-scale noise **8.64 MB** (base64 11.53 MB). 15 s @ 192 k full-scale noise **17.29 MB > 16 MiB**: a natural stimulus for the "> 16 MB omitted" row. 30 s @ 48 k noise + sine = 4.93 MB. **16 MB := 16 777 216 FLAC bytes (pre-base64)**. **[decision for plan]**
- Input bound (V5): reject an `embedded` attribute longer than 22 369 624 chars before decoding (16 MiB · 4/3, rounded to a multiple of 4). The decoded length is capped by `nativeFrameCap` (`SlotLoader.cpp:146-154`).

### 5.3 contentHash (XXH64) — **VERIFIED (run)** / **[decision for plan]**
- Canonical byte stream: an 8-byte header `uint32 LE C` (1 or 2, the pristine's channel count) + `uint32 LE K`, with `K = min (F, 131072)` frames. Then K frames × C channels of `int32 LE Q(x)` (sign-extended), interleaved L, R. Stereo = exactly 1 MiB of samples. XXH64 seed 0. Persisted as **16 lowercase hex digits** (`toHexString ((int64) h).paddedLeft ('0', 16)`), parsed only as exactly 16 hex digits through `getHexValue64`. That is the degree-mask precedent (`PluginProcessor.cpp:487, 598-601`), and it never goes through `getLargeIntValue`.
- The hash is computed on the **pristine** (native rate), so it is independent of the session rate and of trim (the 30 s / 2.88 M trim never reaches the first 131 072 frames). Blob-loaded == file-loaded because Q(deq(q)) == q (probe). A 16-bit or 24-bit file source hashes the same as its own 24-bit blob.
- `GiHash.h`: XXH64 from `xxhash_spec.md` (primes, `round`, `mergeAccumulator`, the 8 / 4 / 1-byte tails, the avalanche; little-endian lane reads via byte assembly, so it is endian-safe). KA gate with the 4 LLVM vectors + a 1-bit-flip control.
- Compressed sources (MP3 / Ogg) can decode differently across platforms, so their hash may not match on another OS. Rung 1 (path + size) still resolves them (INFO).

## 6. `<Slots>` state, resolve, relink, embed

### 6.1 Schema + type-safe parse — **VERIFIED** (precedents) / **INFERRED**
`<Slots><Slot index="0" source="rec:/…/GI-Rec-….wav" name="…" fileSize="1234567" contentHash="0123456789abcdef" embed="1" trimmed="0" embedded="…base64…"/></Slots>`. Fixed attribute order (ValueTree keeps insertion order). Only non-empty slots are written; an absent slot means empty (the P18 "absent = default" rule). No `id` attribute (`PluginProcessor.cpp:424-425`).

Parse:
- `index`: exactly one digit 0–3.
- `source`: a prefix in {`factory:`, `file:`, `rec:`}, else treated as empty.
- `embed` / `trimmed`: `isVoid()` → default, else `s == "1" || s.equalsIgnoreCase ("true")`. Never `isBool()` (memory: XML gives back strings).
- `fileSize`: digits only → `getLargeIntValue` (< 2⁶³ fine).
- `contentHash`: exactly 16 hex digits, else "unknown" (no hash rungs).
- `embedded`: the length bound (§5.2) and the Base64 alphabet, else ignored.

`setStateInformation` reads `<Slots>` **before** `removeCustomChildren` (as for `<Engine>`, `:509-518`).

### 6.2 Records and the save/restore race — **INFERRED**
`SlotRecord` gains `SlotState {source, name, fileSize, contentHash, embed, blob (juce::String, refcounted), trimmed, missing, hashMismatch, relinkMismatch, tooLarge}`. It is written under the loader lock only, by the host thread (restore), the worker (publish / encode) and the message thread (relink / embed / clear).
- **Restore writes the records immediately** (the source string + carried blob), then enqueues the resolve job. A `getStateInformation` right after `setStateInformation` (common in hosts) re-emits the same bytes before the resolve finishes. Missing slots re-save unchanged.
- `getStateInformation` copies the records under the lock and emits them. It never encodes. A blob that is not finished yet → `embed="1"` without `embedded` (Default).
- **Identical-source restore = no-op** (same source string + same contentHash + loaded): no job, so the working copy is not reset by a host undo / re-send. **[decision for plan]**

### 6.3 Resolve job (worker) — rungs, bounds (Q5)
1. **Path:** `file:` / `rec:` path exists **and** `getSize() == fileSize` → decode → hash. On a mismatch, load anyway and set `hashMismatch` (an edited file). **[decision for plan]**
2. **Blob:** `embedded` present → Base64 → FLAC decode from memory → hash must equal `contentHash`. A corrupt blob or a mismatch → next rung.
3. **Recordings search** (any scheme; cheap): (a) `Recordings/<filename>` exists → decode the first K frames → hash match → accept. (b) Else enumerate the Recordings dir **flat (depth 0; the writer never makes subfolders)**, `*.wav`, **≤ 1024 entries** (`RangedDirectoryIterator`, stop at the bound). Prefilter by `fileSize == stored`, hash **≤ 16** candidates (first K frames each), first match wins. Cancellable through the job's `shouldExit` / generation. It runs only here, on the worker, never on a host or message thread. **[decision for plan]** (bounds)
4. **`factory:<id>`:** table lookup → memory decode (§7). An unknown id → missing.
5. **Missing:** publish the **clear sentinel** if the slot held a tape. `SlotInfo.missing = true`, records kept, plugin stable.

On success the source string is kept as stored (a resolve via rung 2 / 3 keeps `rec:<old path>`; only relink changes it). **[decision for plan]**: rewrite the path when rung 3 found the file elsewhere (rec.: yes, with a `relocated` flag; the save then points at the found file).

### 6.4 Clear sentinel — **INFERRED**
`static SlotTape kClearTape` (frames 0, never deleted). `SlotHandoff::publishClear (s)` = `publish (s, &kClearTape)`. `sweepLocked`, the dtor and the deferred list must skip it. `takeTapes`: `p == clear` → `killGrainsOfTape (old)`, retire old, `current[s] = nullptr`, `headRelatch[s] = false`. A pending clear superseded by a load is dropped, not freed. Gate RS6 plus H-style counts.

### 6.5 Relink + embed actions (message thread)
- `requestRelink (slot, File)` → job: decode, hash.
  - Match → keep the scheme, `embed` and the carried blob; source = scheme + the new path.
  - Mismatch → still load: source `file:<path>`, `embed` off, blob dropped, `relinkMismatch = true` (UI flag).

  **[decision for plan]**
- `setSlotEmbed (slot, bool)` → on: queue an encode if there is no blob; off: drop the blob. Factory → ignored.
- Carry rule: a blob loaded from state is re-emitted verbatim while the slot's decoded hash equals `contentHash`. A new pristine (commit / load / relink-mismatch) invalidates it and re-encodes when `embed`. A > 16 MiB result sets `tooLarge` (cached per generation, no re-encode loop).

### 6.6 Recordings directory + harness isolation (Q5, memory) — **VERIFIED** (memory) / **INFERRED**
`SlotLoader::setRecordingsDirectory (File)` defaults to `File::getSpecialLocation (userMusicDirectory).getChildFile ("Ouaricon/O-GranulatorInstrument/Recordings")`. It is resolved lazily (first commit / rung 3), **never in the constructor**, so param-dump, auval and pluginval construction touch nothing.
- The harness sets the override in `make()` for every processor, to a `gTmp` subdir with a **non-ASCII** component (e.g. `Récordings ✓`; build the path from `juce::File` / `String::fromUTF8`, never `String (const char*)`).
- The harness guard row lists the real `~/Music/Ouaricon/O-GranulatorInstrument/Recordings` (count + newest mtime) at start and end: unchanged. No `CFFIXED_USER_HOME` re-exec is needed, because nothing resolves the default in the harness.
- **Risk:** pluginval's random automation can arm with noise input and commit a WAV into the real `~/Music`. Verify must snapshot that folder before and after pluginval and auval.

## 7. Factory sources (D33, Q8)

- **CMake** — **VERIFIED** (traps) / **INFERRED**:
  ```
  juce_add_binary_data(OuariconGranulatorInstrument_FactorySources
      NAMESPACE FactorySourceData  HEADER_NAME FactorySourceData.h
      SOURCES Resources/factory/stub_tone.flac Resources/factory/stub_noise.flac)
  target_link_libraries(OuariconGranulatorInstrument PRIVATE OuariconGranulatorInstrument_FactorySources)
  ```
  This must sit before the `OUARICON_BUILD_TESTS` block: consoles carry it (`ParamDump.cmake:221-226`). Symbols: `FactorySourceData::stub_tone_flac` / `…Size`. Use ids with **no hyphens** (stripped).
- **Table** (`GiFactory.h`): `{ "stub_tone", FactorySourceData::stub_tone_flac, …Size, kHashTone }, { "stub_noise", … }`, explicit symbols (a rename is a compile error, the O-simpleSampler `:362-363` precedent), never `getNamedResource` strings. Factory never embeds and is never trimmed.
- **Generator = a harness mode `--gen-factory <dir>`** (rec. over Python). It uses the same JUCE FLAC encoder and the same int path + Q as the plugin. No external tool or Python FLAC library is needed (`flac` 1.x is on PATH at `/opt/homebrew/bin/flac`, but it is a different encoder build). It writes `Resources/factory/*.flac`, which are checked in.
  - `stub_tone`: **mono**, 48 kHz, 3.0 s, 220 Hz, harmonics k = 1…8 at 1/k, −12 dBFS peak, 10 ms raised-cosine ends.
  - `stub_noise`: **stereo**, 48 kHz, 3.0 s, the harness `hashNoise` (splitmix, `engine_gates_21.h:326-334`), one-pole LP, −18 dBFS RMS.
  - Mono + stereo covers both pristine paths. Size ≈ 0.25 + 0.86 MB.
- **Pins:** (1) `kHashTone` / `kHashNoise` = XXH64 of the decoded PCM, in `GiFactory.h` and checked by FA1 on every platform (FLAC decode is exact). (2) The `.flac` sha256 in `tests/golden/factory-stubs.sha256`, re-checked by `--gen-factory` on mac arm64 only (INFO elsewhere, Summary 3). **[decision for plan]**
- **"No disk access" proof:** the factory decode takes `(const void*, size_t)` only, so it is structural. Add a loader `fsTouches` test counter, incremented by every rung that calls `File` I/O. FA2: a factory resolve → 0. Control: a `file:` resolve → > 0.

## 8. Telemetry (Q7)

| feed | exists? | 2.6 |
|---|---|---|
| 125 dest values (G / newest voice V), 17 sources, 4 LFO phases | **yes**, `destTel` / `srcTel` / `lfoPhaseTel`, written at ticks (`Engine.cpp:1542-1559`, called `:1653`) | unchanged |
| record state + reason + heldArm | no | `atomic<uint32_t> recState` (§2.1) |
| capture progress | no | `atomic<int64_t> recFrames`, `recTarget` (relaxed; per block) |
| sidechain peak / output peak (max since read) | no | `atomic<float>` + `pushMax` (`UiTelemetry.h:216-220`); the reader `exchange (0)` |
| per-slot head position + swap / reset state | no | `atomic<int64_t> headPos[4]`; `atomic<uint8_t> swapState[4]` (pending / xfade / parked / waiting) |
| per-slot evolving overview | no | `GiOverviewLive`: per slot `kOvBins = ceil (2 880 000 / 512) = 5625` × {min, max} `atomic<float>` + `atomic<uint32_t> seq[bin]` + `tapeEpoch[s]` (bumped at each take). The head writes the bin it covers (≤ 2 per cell). The reader uses a bin only when `seq == tapeEpoch`, else the pristine overview. Constructor-allocated (≈ 4 × 5625 × 12 B = 270 KB), never reallocated (the PeakRing rule, `UiTelemetry.h:115-123`) |
| grain snapshot + per-voice | no (**no 2.1 snapshot exists**) | `GiTelemetry.h`: `TripleBuffer<GiUiFrame>`. `GiUiGrain {uint8 voice, uint8 stream, uint8 flags (reversed, killing), float normPos, float gain}` × **288** (`GiPool::kNumSlots`, `VoiceGrain.h:63`) + 20 × `{int8 note, float amp, float scan[4]}` + seqHead / seqTail. ≈ 5.3 KB per frame, 16 KB total. Published at each block end |

`TripleBuffer<T>` holds three `T` inline, so capacity = `sizeof (T)` and there is no limit to raise (`UiTelemetry.h:54-80`). Reuse `UiTelemetry.h` **unchanged** (reclassify VERBATIM in the manifest; Check 2 already treats "pending" as VERBATIM, `fork-drift-check.sh:178`). Do not edit `kMaxGrains`: the instrument frame is its own type. **[decision for plan]**

## 9. Proof arms (gate inventory for the plan)

New files: **`engine_gates_26a.h`** (capture / commit / leak) and **`engine_gates_26b.h`** (state / resolve / factory / hash / telemetry / bypass), `--eg26` dev flag, **`--cpu26`**, **`--gen-factory`**. SC injection writes `s.blk` in `beforeBlock` (`engine_gates_21.h:491-492`). Every row prints a firing control (the suite convention).

| family | rows | control that must fire |
|---|---|---|
| RC (FUNC-07) | 3 triggers × 4 slots: PCM (tape level 0, `MipTape::levelSample`, `MipTape.h:118-123`) == sc × g **bitwise** over `[start, start + frames)`; a 4-note chord on the recorded slot plays 4 rates; mono SC → 1-ch pristine | injected-offset hook (start + 1) differs |
| TH (D35) | start == crossing − P; crossing at arm + 3 ms → start == arm (clip); Manual == arm sample; Next Note == note sample; pre-roll counts toward length | `prerollOff` → start == crossing |
| OS (D34) | held-On stays Idle after commit; a new edge captures; Off while Armed cancels; Off while Recording ≥ 0.5 s commits, 0.49 s discards (TooShort); arm during Committing is held → Armed when free | `armLevelTrigger` (level, not edge) re-arms |
| LN | seconds; bars under Tempo + playing (bar latched at start; a tempo change mid-take ignored); Tempo + stopped → seconds; cap `F_max` at 44.1 / 96 / 192 k (1 323 000 / 2 880 000 / 2 880 000) | cap-off hook overruns → counted |
| MN | Mono retrigger counts, Legato pitch change does not, D30 fallback does not | `nnAnyNote` counts Legato |
| QD (QUAL-02) | silent stereo SC: every trigger → discard `NoSignal`, slot tape pointer + `hashForTest` unchanged; no bus (disabled) → arm inert, `SidechainUnavailable`; −89 dBFS take commits, −91 discards | −89 dBFS stimulus commits |
| LK (QUAL-02) | **LK1**: loud SC (noise 0.7 + 997 Hz full scale) × {stereo, mono + host garbage on ch 1, disabled + garbage on both} × every state (Idle / Armed / Recording / Committing) with notes on a loaded slot: output == the SC-silent render **bitwise** over the pre-commit span; with no notes == +0.0f. **LK2**: `processBlockBypassed` with loud SC → +0.0f. R1 pinned `rec_arm` Off (Summary 13) | LK2n: the base-class bypass leaks (fires today); LK1n: `scNoClear` hook |
| CM (PERF-03) | commit via `commitSync` and via Timer + `pump`; offline (`setNonRealtime (true)`) commit lands; **CM3 host-idle** (§4.3); **CM4** prepare mid-Recording → committed at the old rate (pristine rate == fs_old, tape at fs_new), Armed → Interrupted, Committing survives a prepare | `testNoEntryCounter` grows |
| ST (FUNC-06, D36) | 4 sources (file, rec, factory, embedded-only) + loop regions + scale state restore; blob decode == deq(Q(pristine)) bitwise; **save → load → save byte-identical** (incl. before the resolve finishes); 17.3 MB take → `embedded` absent, falls back to path; hash > 2⁶³ round trip; `embed="0"/"1"` strings; malformed hash / index / blob ignored; identical-source restore = no swap | `reencodeOnLoad` (quality 8) changes bytes; `parseIsBool` restores defaults |
| RS (FUNC-06) | each rung: path + size; size mismatch → blob; Recordings exact name; renamed file found by size + hash (≤ 16); factory; missing (sentinel clear, stable 2 s render, reference re-saved); relink match keeps embed / blob; relink mismatch flagged `file:`; listing bound 1024 honoured | `searchUnbounded` counts > 1024 |
| FA (D33) | both stubs decode, hash == pinned; `fsTouches == 0`; stub symbols / ids | `file:` resolve `fsTouches > 0` |
| HS | XXH64 4 KA vectors; Q(deq(q)) == q ∀ 2²⁴; blob-loaded hash == file-loaded hash (float32 WAV, 16-bit WAV, 24-bit WAV of one signal) | 1-bit flip; the JUCE float-writer path (4 194 302 mismatches) |
| AL (PERF-01) | 0 audio-thread allocs across arm → record → commit → swap, discard, cancel, threshold pre-roll, state save / load (embedded), missing resolve (sentinel), factory load, prepare mid-record, bypassed blocks | liveness probe (A1 pattern) |
| CK (QUAL-01) | local κ (`eg24b::kappa`, `engine_gates_24b.h:70-77`): **CK6a** commit swap into a sounding slot (tonal A → tonal B) and **CK6b** state-load swap, each vs held pre (A only) / post (B only) references ≤ 2.0× (the 2.5a RE pattern) | `swapKillHard` (kill step 1 sample) > 2.0× |
| BS | Manual / Threshold / Next Note scripts (+ notes, + Tempo) at 32 / 441 / 512 / 4096 / var: start / stop samples and captured PCM bitwise | different SC noise seed differs |
| TL | recState / progress / peaks track a scripted take; grain frame count ≤ 288, seqHead == seqTail; overview bins epoch-tagged after a feedback pass | torn-read probe (write without publish) |
| TB | true bypass: `--dump-render-hashes` == renders-2.4, legacy == 2.2 / 2.1-pinned; N2 / N2s / FN3 re-scoped to readSet26 (261 / rec_* × silent SC); `--parity` 168/0 + GV 2/0; param-dump identical; drift 0/13 + NC-a / NC-b | carried |
| CPU | `--cpu26`: C25m ≤ 40 %, C25tm ≤ 12 % (regression); **INFO** C25m while Recording (Threshold + 4 s take in flight) | witnesses as 2.5a |
| guard | the real `~/Music/…/Recordings` unchanged across the harness | — |

## 10. Pitfalls
1. **Base-class bypass passes the sidechain through** (Summary 1). This is a live bug and not reachable by any current row.
2. **The FLAC float writer path** is not round-to-nearest and not idempotent on 24-bit audio. Using it breaks D36 (1) and the hash agreement (Summary 2).
3. **`MemoryBlock::toBase64Encoding`** is JUCE-private. Use `juce::Base64`.
4. **FLAC quality index 0 == 5.** Set 5 explicitly so a future "fastest" edit doesn't silently do nothing.
5. **`getLargeIntValue` on the hash** clamps above 2⁶³. Use 16-hex + `getHexValue64` only.
6. **XML bools come back as strings.** Never `isBool()`.
7. **Reading `<Slots>` after `removeCustomChildren`** finds nothing (the strip is `:445-453`).
8. **`getState` right after `setState`** must not drop slots still resolving (§6.2).
9. **R1 random params + loud SC + a commit path that does not need pumping** (a watcher thread) would land a tape and fail R1. Pin `rec_arm` Off in R1.
10. **N2 / N2s / FN3 go vacuous** once `rec_*` are read (`touched == 0`). Re-scope them, don't delete them.
11. **`rec_gain` default 7e-7 dB.** Go through `snapDb`.
12. **Capture-buffer grow while Committing** frees the data under the copy-out. Use grow-only + skip + `captureLock`; prepare never waits on the message thread (it may *be* the message thread → deadlock).
13. **The arm edge read from `gp` (grid-latched)** silently moves Manual starts to ceilCell(t). Stamp at block start.
14. **Mono SC + in-place buffer:** ch 1 holds host garbage (R1 already injects 12345.0f, `main.cpp:763-764`). Read only `sidechainChannels` channels.
15. **Clear-sentinel deletion:** the sweep / dtor / deferred list deleting the static sentinel is a crash. Skip it everywhere.
16. **`juce_add_binary_data`:** use underscores, not hyphens. Link before the console block, or the harness / param-dump fail to link `FactorySourceData::*`.
17. **pluginval / auval writing real recordings** into `~/Music` (§6.6).
18. **State size:** 4 × ≤ 22.4 MB base64 ≈ 90 MB XML, built on every host `getStateInformation` (some hosts call it on every undo point). D36 is locked; measure save / load time as INFO, and put Logic save time in HANDS-ON.
19. **Non-ASCII Recordings paths:** `String (const char*)` is ASCII-only (memory). Paths come from `juce::File`.
20. **`replaceWithText` CRLF:** byte-compare state as `MemoryBlock`s in memory. Never as text files written with the default line feed.

## 11. Decisions for plan (summary)
1. Hash: own XXH64 in `GiHash.h` from the spec + KA gate (rec.) vs vendoring `xxhash.h` (BSD-2) vs `juce::SHA256` truncated (O-Strata precedent, + `juce_cryptography`).
2. Canonical hash stream: header {C, K}, `int32 LE Q(x)` interleaved, `K = min (F, 131072)`, seed 0, 16 hex.
3. Hot takes: keep the float capture unclamped and let the blob clip at ±1 (rec.) vs clamp the capture.
4. 16 MB := 16 777 216 FLAC bytes (pre-base64); quality 5 explicit.
5. Arm edge stamped at block start (rec.) vs grid tick; latch points: target / trigger at Armed, threshold / gain at ticks (gain 20 ms ExactRamp), length / sync at start.
6. Mono SC → mono pristine (rec.) vs L = R stereo.
7. Commit: 20 ms message-thread Timer copy-out + worker job (rec.); watcher thread only without a MessageManager; harness `commitSync`. AsyncUpdater rejected.
8. WAV before publish (rec.); WAV failure still publishes with the blob + `WavFailed`.
9. `prepareToPlay`: Recording → commit at the old rate (rec.) vs discard; Armed → cancel; Committing survives; grow-only buffer.
10. Bypassed block: Armed → cancel, Recording → stop / commit (rec.).
11. Clear sentinel through the pending exchange (rec.).
12. Identical-source restore = no-op (rec.).
13. Rung 1 hash mismatch → load + flag; rung 3 bounds (depth 0, 1024 entries, ≤ 16 hashes, exact name first); rung 3 hit rewrites the path (+ `relocated`).
14. Relink: match keeps scheme / embed / blob; mismatch → `file:`, embed off, flagged.
15. Telemetry: `UiTelemetry.h` reclassified VERBATIM, reused; new `GiTelemetry.h` (288 grains, per-voice, overview bins with epoch).
16. Factory: harness `--gen-factory`; `stub_tone` (mono) + `stub_noise` (stereo), 48 k, 3 s; pins = decoded XXH64 (all platforms) + file sha256 (mac arm64 INFO).
17. Harness: readSet26 re-scope of N2 / N2s / FN3; R1 `rec_arm` pinned Off; Recordings override in `make()` + the real-dir guard row; `--cpu26`.
18. `rec_*` bound in `GiRecorder` (raw atomics), not `ControlGrid.h` (no manifest re-hash).

## 12. Targets (projection)
- Harness **300 → ≈ 355** (RC 4, TH 3, OS 5, LN 4, MN 1, QD 3, LK 2, CM 4, ST 7, RS 8, FA 2, HS 3, AL 1, CK 2, BS 1, TL 3, guard 1 ≈ 54; N2 / N2s / FN3 re-scoped, R1 pinned).
- `--parity` 168/0 + GV 2/0 carried (no parity arm). renders-2.4 + legacy byte-identical; param-dump identical; drift 0/13 (UiTelemetry reclassified; no forked file edited).
- `--cpu26`: C25m ≤ 40 %, C25tm ≤ 12 % + INFO while Recording.
- auval PASS; pluginval 10 VST3 + AU with SC active (2 ch) and inactive (`OGI_GATE_SC_DEFAULT_OFF`), with a `~/Music/…/Recordings` before / after snapshot.
- New files:
  - `Source/dsp/{GiRecorder.h, GiHash.h, GiTelemetry.h, GiFactory.h}`
  - `Resources/factory/{stub_tone, stub_noise}.flac`
  - `tests/render-harness/engine_gates_26{a,b}.h`
  - `tests/golden/factory-stubs.sha256`
  - `HANDS-ON-2.6.md` (deferred)

## Sources
- JUCE 8.0.15 local tree (paths cited inline): FLAC writer / reader, AudioFormatWriter / Reader conversions, `roundToInt`, MemoryBlock base64, AsyncUpdater, mac MessageQueue, `AudioProcessor::processBypassed`, VST3 / AU bypass call sites, `makeBinaryDataIdentifierName`. **HIGH**
- Probe (scratchpad, run 2026-11-09): FLAC determinism, quality 0 == 5, memory round trip, sizes, quantiser idempotence, XXH64 vectors. **HIGH**
- [xxHash LICENSE (BSD 2-Clause)](https://github.com/Cyan4973/xxHash/blob/dev/LICENSE) · [xxHash spec](https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md) · [LLVM xxhashTest.cpp vectors](https://llvm.googlesource.com/llvm-project/+/dda73336ad22bd0b5ecda17040c50fb10fcbe5fb/llvm/unittests/Support/xxhashTest.cpp) · empty-input vector also in the [Linux kernel xxhash commit](https://android-kvm.googlesource.com/linux/+/67882e76492483bafa9b1b1648bb031e9abe5185%5E%21). **HIGH** (cross-checked by the probe)
- Sibling precedents: O-Strata (SHA-256 + Base64 + missing / Locate), O-simpleSampler (`embedded:` + MemoryInputStream + standard Base64). **HIGH**

**Valid until:** the next JUCE bump (FLAC writer / bypass semantics) or any change to `Engine::process` piece order.
