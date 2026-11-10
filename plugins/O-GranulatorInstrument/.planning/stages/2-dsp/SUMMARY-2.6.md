---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.6"
title: "Record-in capture, <Slots> state + resolve + relink + embed, factory sources, telemetry"
executed: 2026-11-09
version: 0.1.0 (unchanged, PD24)
fork_pin: 82406fce
commits: C1 a8669ebc (engine + loader + processor + factory + telemetry; harness 300/0) + C2 (gates eg26a / eg26b, swapKillHard hook, hands-on, this summary)
result: executed — every automated gate green; S6 (CK6 click gates) hit and resolved by the user; HANDS-ON-2.6 written and deferred with 2.1–2.5b
---

# O-GranulatorInstrument — Stage 2 / Phase 2.6 SUMMARY

The instrument now records its sidechain into any of the four slots, by Manual, Threshold (10 ms pre-roll) or Next Note, with a one-shot arm. Slot references persist in `<Slots>` (file / rec / factory / embedded FLAC-24) and resolve on reopen through path → blob → Recordings search → factory → missing, with relink. Two generated factory stubs load from memory. The sidechain no longer leaks to the output on host bypass (a live bug before 2.6). With no `<Slots>` content and an idle recorder, every 2.5b artefact is unchanged bit for bit.

There is no parent equivalent (O-Granulator has no Record-in), so the proof is law gates, leak gates with hard controls, click gates, the block-size null, the allocation gate, the host-idle bound and true bypass.

## What was built

- **Leak fix + sidechain plumbing (T3, PD19 / PD20).**
  - `processBlockBypassed`: NE drop → `buffer.clear()` → `engine->bypassedBlock()`. The JUCE base is never called. The base clears only channels ≥ the main-input count, and the Sidechain *is* the main input bus, so before 2.6 a stereo sidechain went straight to the output whenever the host bypassed the plugin (RS1). Hook `baseBypass` restores the old behaviour as LK2n.
  - `processBlock`: clears channels ≥ 2 only; passes `scL` / `scR` from the negotiated `sidechainChannels` (ch 1 of a mono SC is never read); clears the whole buffer when the engine is not called.
  - `Engine::process (…, scL, scR)`: per 32-grid piece, the sidechain is copied into a 2 × 32 scratch, then `out[pos, b)` is zeroed, then the event loop, the recorder and `mixPiece` run. `mixPiece` assigns every sample, so moving the clear into the engine is byte-identical (renders-2.4 / legacy unchanged). Hook `scNoClear` (no zero + an accumulating mixer) is LK1n.
- **`GiRecorder.h` (T6).** Idle / Armed / Recording / Committing with reasons (None, SidechainUnavailable, NoSignal, TooShort, Cancelled, Interrupted, Bypassed, Committed, WavFailed). The 7 `rec_*` are raw atomics bound here (PD18, no ControlGrid edit). Arm / target / trigger are read at the block start, and the edge is stamped there. Threshold / gain latch at grid ticks (gain = a 20 ms ExactRamp through `dbToGain`, so the 7e-7 dB default is exactly unity). Length / sync latch at the start sample (bars from the tracker anchor under Tempo + playing). Pre-roll ring `lround (0.010 fs)`, D6 presence −90 dBFS, minimum 0.5 s, cap `F_max (fs)`. Grow-only capture buffer (uninitialised `new float[]`, 2 × 2.88 M max) under `captureLock`, never grown while a copy-out is pending. Next Note hooks sit in `startVoice` / `retriggerVoice` (the only serial mints). `prepare` stops a take and commits it at the old rate; Armed → Interrupted; Committing survives. `bypassedBlock`: Armed → Bypassed, Recording → stop.
- **Commit hand-off (T6, PD7 / PD8).** The audio thread fills `commit` and does `ready.store (frames, release)`. The processor polls every 20 ms: a message-thread `juce::Timer`, or a `juce::Thread` watcher when no MessageManager exists. The poll copies the take out under `captureLock`, clears `ready`, then calls `SlotLoader::requestCommit`. Worker job order: hash → float32 WAV (`GI-Rec-YYYYMMDD-HHMMSS-S<n>.wav`, `getNonexistentSibling`, **before** publish) → tape → publish with source `rec:<path>` under the loader lock → FLAC-24 encode → blob cached for that generation. A WAV failure still publishes (blob carries the take); the recorder reports WavFailed through a lock-free flag. Harness `commitSync()` runs the poll + job on the caller.
- **Clear sentinel (T5, PD11).** `SlotHandoff::clearTape()` (function-local static, constructed in the handoff constructor) + `publishClear`. Publish / sweep / dtor / ring skip it; a superseded clear is dropped. `takeTapes`: the old tape's grains get the 5 ms kill, the old tape retires, `current = nullptr`.
- **`GiHash.h` (T8, PD1–PD3).** XXH64 from the spec (byte-assembled lanes); `quantise` = clamp (nearbyint (x · 2²³)); `dequantise` = q · 2⁻²³; `contentHash` over `uint32 C | uint32 K | K·C int32 LE Q(x)`, K = min (F, 131072), seed 0; 16-hex persistence through `fromHex` (never `getLargeIntValue`).
- **Loader (T5 / T8 / T9).** `SlotInfo` += source, fileSize, hasHash / contentHash, embed, hasBlob, tooLarge, missing, hashMismatch, relinkMismatch, relocated, wavFailed, resolvedBy. Job kinds Load / Rebuild / Commit / Resolve / Relink / Encode. Every state write lands in the same locked step as its publish. `decodeReader` is shared by file and memory decoding. `encodeFlac24` writes Q codes × 256 through `write (const int**)`, quality 5 explicitly (the float path is a test control only). Embed limit 16 777 216 FLAC bytes. `restoreSlots` writes the records at once and queues resolves; an identical-source restore is a no-op (PD12); an empty-source restore clears a **sourced** slot through the sentinel. Resolve rungs: path + size (a hash mismatch still loads, flagged); blob (hash-verified); Recordings exact name then a flat `*.wav` scan (≤ 1024 entries, size prefilter, ≤ 16 hashed; a hit rewrites the path, `relocated`); factory; missing (sentinel, reference kept). `requestRelink` (match keeps scheme / embed / blob; mismatch → `file:`, embed off, flagged). `setSlotEmbed`. The Recordings directory resolves lazily (`setRecordingsDirectory` override; never in a constructor).
- **State codec (T9, PD21).** `<Slots>` is parsed **before** the strip. `index` must be one digit 0–3; known prefixes only; `embed` / `trimmed` are `"1"` / `"true"` strings (hook `testParseIsBool` = SS5n); fileSize digits only; hash 16 hex; `embedded` ≤ 22 369 624 chars, base64 alphabet, length % 4. Emit: fixed attribute order (index, source, name, fileSize, contentHash, embed, trimmed, embedded); only sourced slots; never encodes.
- **Factory (T10, D33, PD16).** `juce_add_binary_data (… NAMESPACE FactorySourceData HEADER_NAME FactorySourceData.h)`, linked before the console block (harness + param-dump inherit it). `GiFactory.h` (explicit symbols; `__has_include` keeps it compiling before the target exists). Harness `--gen-factory <dir> [--force]`.

  | Stub | Format | FLAC bytes | Decoded XXH64 (pinned, all platforms) | File sha256 (mac arm64) |
  |---|---|---|---|---|
  | `stub_tone` | mono 48 k 3 s, 220 Hz k = 1…8 at 1/k, −12 dBFS peak, 10 ms raised-cosine ends | 124 716 | `6d117c4cc9d31ec3` | `45af0e3d…f63763` |
  | `stub_noise` | stereo 48 k 3 s, splitmix noise, one-pole LP (0.25), −18 dBFS RMS | 777 546 | `e6c6444b102af6b8` | `6ef45b36…90fa0b5` |

  A second `--gen-factory` run is byte-identical to the checked-in files.
- **Telemetry (T11, PD15).** `GiTelemetry.h`: `scInPeak` / `outPeak` (`pushMax`, reader `exchange (0)`), `headPos[4]`, `swapState[4]` (pending / crossfade / parked / waiting), `GiOverviewLive` (5625 bins × min / max per slot + seq, tagged with `tapeEpoch`, bumped at every take), `TripleBuffer<GiUiFrame>` (≤ 288 grains + 20 voices, published at every block end). The recorder publishes `recState` (state | reason << 8 | heldArm << 16), frames / target. `forked/UiTelemetry.h` is reclassified VERBATIM in the manifest (bytes unchanged).
- **Harness (T4 / T12–T14).** `readSet26` (261) with `recSet26` / `touchRec26`. N2 / N2s / FN3 now randomise the 7 `rec_*` with rec_arm forced On over a silent sidechain (output bitwise identical; the D6 discard does the work). R1 pins rec_arm Off. Every processor's Recordings folder is a temp dir with a non-ASCII component (`Récordings ✓`), deleted at exit. New files `engine_gates_26a.h` (28 rows) and `engine_gates_26b.h` (29 rows); `--eg26`, `--cpu26`, `--gen-factory`.

## Gate results

| Gate | Result |
|---|---|
| Build | harness, param-dump, VST3, AU, Standalone rc 0; **0 warnings** |
| Checkpoints | renders-2.4 **byte-identical** and legacy == renders-2.2 after T6, T11 and T15. Nothing re-pinned |
| Default harness | **357 PASS / 0 FAIL**, **0 NOT FIRED** (300 + 28 eg26a + 29 eg26b). Duplicate-ID set == the 2.5b set |
| `--parity` | **168 / 0**, `--rows GV` **2 / 0**; every residual line identical to T1 |
| param-dump | byte-identical to T1 |
| Drift | 0 / 13 (UiTelemetry now VERBATIM; no forked byte edited) |
| `--cpu26` (alone) | **C25m 29.26 %** (≤ 40; witnesses valid: PV dests 16 / tick, recomputes 100 %, pair-mode 4, live 243.2); **C25tm 6.71 %** (≤ 12; live 80.2). C25w 27.84 %, C25t 5.79 %. Δ vs T2 (28.90 / 6.77): +0.36 / −0.06 points. **C26rec 28.44 % INFO** (C25m with a Manual take in flight) |
| T18 | `build-and-install.sh` rc 0 (dual-variant sweep). Standalone rebuilt explicitly (binary newer than the harness), alive after 6 s. **auval PASS** (ZZZZ control rc 2). **pluginval 10, SC active (2 ch): VST3 SUCCESS** (0 `!!!`) + **AU SUCCESS** (1 `!!!`: pluginval's generic "current program is -1"). **SC inactive** (`-DOGI_GATE_SC_DEFAULT_OFF=ON`, the Stage 1 same-build-dir precedent): **VST3 + AU SUCCESS** (same single AU warning); cache restored to `OFF` (0 gate-variant warnings), shipping build reinstalled, auval PASS again. **X2** 0 `ogran::` in the installed VST3, AU and the harness. Real `~/Music/…/Recordings` absent before and after (no validator wrote a take). Note: Logic was open during the run (the plan asks for it closed); every validator passed regardless |

### eg26a (28 / 0) — capture, triggers, one-shot, length, presence, leaks, commit
- **RC1–RC3** Manual / Threshold / Next Note × slots 0–3 at +6 dB (g = 1.995262): tape level 0 == sc × g, **0 diffs** over 48 000 frames × 2 ch each; starts 4807 (arm) / 13927 (crossing − 480) / 5807 (note). **RC1n** startOffset → start 4808.
- **RC4** 4 distinct rates over 200 grains on the recorded slot; mono SC → 1-ch pristine, tape L == R == x.
- **TH1** start 18733 = crossing 19213 − 480; **TH1n** prerollOff → 19213. **TH2** crossing +144 → start == arm. **TH3** Manual +0, Next Note == note sample, Threshold frames 48 000 incl. pre-roll.
- **OS1** 1 commit, 1 arm, Idle after 2 s held On; **OS1n** armLevelTrigger → 2 arms. **OS2** second take at the new edge, 0 diffs. **OS3** Cancelled, tape pointer unchanged. **OS4** 0.50 s → Committed (24 000), 0.49 s → TooShort. **OS5** held during Committing → Armed at the first block start after the copy-out.
- **LN1** 60 000 frames. **LN2** 192 000 (2 bars @ 120, a 90 bpm change mid-take ignored). **LN3** 48 000 (stopped → seconds). **LN4** 1 323 000 / 2 880 000 / 2 880 000; **LN4n** capOff → overrun at 192 k.
- **MN1** Mono start +1000 (retrigger), Legato +4000 (the fresh note; the legato move and the D30 fallback did not count); **MN1n** nnAnyNote → +1000.
- **QD1** silent SC: Manual / Next Note discard NoSignal, Threshold stays Armed; tape + hash unchanged; **QD1n** −89 dBFS commits. **QD2** disabled bus → SidechainUnavailable, 0 arms; re-enabling with the arm On does nothing. **QD3** −89 commits, −91 NoSignal.
- **LK1** loud SC (noise 0.7 + 997 Hz FS) × {stereo, mono + garbage, disabled} × {Idle, Armed, Recording, Committing}: **12 / 12** output == SC-silent render bitwise; with no notes +0.0f. **LK1n** scNoClear → 120 000 samples leak.
- **LK2** `processBlockBypassed` → +0.0f on all 3 layouts. **LK2n** baseBypass (the pre-2.6 behaviour) → the sidechain leaks.
- **CM1** commitSync and Timer + pump: tape exact, WAV in the override dir, non-ASCII path round-trips. **CM2** non-realtime + pump lands. **CM3** host idle mid-commit / mid-swap, 50 relinks + 50 restores: pending max 1, deferred 0, live tapes max 2, resume lands + retiring drains; **CM3n** testNoEntryCounter → deferred 50. **CM4** prepare mid-take at 44.1 → 96 k: committed, pristine 44.1 k (44 100 frames), tape 96 k; Armed → Interrupted; Committing survives. **CM5** Armed → Bypassed; Recording 0.625 s → Committed. **CM6** WAV fails: published, wavFailed, blob present, no file, reason WavFailed.

### eg26b (29 / 0) — state, resolve, factory, hash, alloc, clicks, block size, telemetry, bypass
- **SS1** restore file / rec / factory / embedded-only: rungs 1 / 1 / 4 / 2, 4 / 4 loaded and sounding (15 grains per stream), loop 10–80 and the `edo 19` custom scale restored.
- **SS2** blob == deq (Q (pristine)), 0 / 96 000; **SS2n** float writer path → 42 783 mismatches.
- **SS3** save → load → save identical before the resolve, after it, and over a second hop (298 450 bytes, blob present); **SS3n** reencodeOnLoad (q 8) → `<Slots>` bytes differ.
- **SS4** 15 s @ 192 k full-scale take: tooLarge, 1 encode, `embed="1"` without `embedded`, restore via rung 1.
- **SS5** hash `fedcba9876543210` round trip; `embed="true"` / `trimmed="1"` parsed; index 4 / x / 01, `http:`, a bad hash, a bad-alphabet blob, an oversize blob and fileSize `12x` ignored; **SS5n** parseIsBool → defaults.
- **SS6** identical-source restore: same tape pointer, 0 resolve jobs. **SS7** empty-source restore: 1 sentinel clear, slot empty, 2 s finite, silent tail.
- **RL1** rung 1. **RL2** size mismatch → blob, reference kept. **RL3** Recordings exact name, path rewritten, relocated. **RL4** renamed file found by size + hash. **RL5** factory, hash == pinned. **RL6** missing: sentinel clear, stable 2 s, `<Slot>` re-saved identical. **RL7** relink match keeps scheme / embed / blob; mismatch → `file:`, embed off, flagged; rung-1 edited file → hashMismatch. **RL8** 1100 same-size files: 17 scanned, 16 hashed; **RL8n** searchUnbounded → 1100 / 1100.
- **FA1** both stubs decode, hashes == pins. **FA2** factory resolve fsTouches 0; **FA2n** `file:` → 2.
- **HS1** XXH64 `""` / foo / bar / 36-char / 62-char match the reference library; **HS1n** 1-bit flip differs. **HS2** Q (deq (q)) == q for all 2²⁴ codes; **HS2n** JUCE float path moves 349 524 of 1 398 101 sampled codes. **HS3** blob hash == file hash for float32 / 16-bit / 24-bit WAV.
- **AL26** 0 audio-thread allocations across Threshold pre-roll → record → commit → swap, discard, cancel, embedded save / load, a missing resolve (sentinel), a factory load, prepare mid-record and bypassed blocks; liveness: arms 5, starts 4, commits 3, discards 1, cancels 1, published 2, missing 1, factory 1, clears 1.
- **Click gates (S6 decision, below):**

  | Row | max \|d2\| ratio vs held refs | local κ (INFO) | Hard control |
  |---|---|---|---|
  | CK6a commit swap (220 → 330 Hz) | 1.202 | 3.58× | CK6n swapKillHard: \|d2\| 57.3×, κ 2325× |
  | CK6b state-load swap | 1.202 | 3.83× | (CK6n) |

- **BS26** 3 triggers × {Free, Tempo} × 441 / 512 / 4096 / var vs 32: 24 / 24 start, stop and PCM bitwise; **BS26n** SC seed + 1 → 96 000 differ.
- **TL1** recState / frames / target / sc peak track a take (Recording 24 000 / 48 000, Committing / Committed, Idle after the copy-out); **TL1n** torn frame rejected. **TL2** 40 reads complete, count ≤ 288. **TL3** 94 / 94 bins rewritten with feedback on; a Reset bumps the epoch 2 → 3 and 87 bins read stale.
- **TB7** renders-2.4 14 / 14 + legacy 14 / 14 with the SC bus present and the recorder idle. **GD1** the real `~/Music/…/Recordings` absent at start and end.

## Plan decisions as executed
PD1–PD24 as written, except the deviations below.

## Deviations (each with its reason)
1. **S6, user decision 2026-11-09: CK6a / CK6b judged on raw window max |d2|.** Local κ read 3.58× / 3.83×. The diagnostic showed raw curvature at the swap (0.00044) below the B-only reference's steady maximum (0.00079), while the frame RMS dips to 0.018 (references 0.058 / 0.115). The 2.1 swap rule (5 ms kill; new grains fade in through their windows) leaves a short level dip, and that dip shrinks κ's RMS denominator. The rows now gate |d2| ≤ 2.0× the held references (1.20×), report κ as INFO, and keep swapKillHard as the hard control. The swap rule is unchanged. HANDS-ON-2.6 row 13 asks for a listening check of the dip.
2. **Commit grouping.** C1 (`a8669ebc`) holds T3–T11 code (leak fix, recorder, loader incl. hash / FLAC / state / resolve, factory, telemetry), not T3–T6. It was checkpointed and the full harness was 300 / 0 before the commit. C2 holds the gates (T12–T14), the swapKillHard hook (GiSpawner + Engine) and the docs.
3. **Checkpoint grouping.** Checkpoints ran after T6 (with T3–T5 + loader), after T11 (with T8–T10) and at T15. All held, so nothing needed bisecting.
4. **T2 ordering.** Source edits started while `--cpu25a` ran; the measured binary was a frozen copy of the unmodified 2.5b build (`$S/bin25b/H`), and nothing was rebuilt until T2 finished.
5. **Row IDs renamed** (the plan forbids new duplicates): ST1–ST7 → **SS1–SS7**, RS1–RS8 → **RL1–RL8** (RS and RV both collide with earlier rows), AL1 → **AL26**, BS1 → **BS26**.
6. **HS1 vector.** The plan / research literal `69196c1b3af0bff9` is not the XXH64 of `"abcdefghijklmnopqrstuvwxyz0123456789"`. GiHash gives `64f23ecf1609b766`, and so does the reference library (PyPI xxhash 4.1.0, run in an isolated venv). The gate uses the reference values and adds a 62-char vector. `""`, foo and bar matched the plan from the start.
7. **SS3 compares with the `<CustomScale version>` counter removed.** Every restore bumps this 2.2 change counter (pre-existing behaviour; not slot state), so whole-state bytes can never be identical. Everything else, `<Slots>` included, is compared byte for byte. SS3's take is tonal: white noise encodes as verbatim FLAC subframes at both quality 5 and 8, which made SS3n vacuous.
8. **Empty-source restore clears only *sourced* slots.** A slot loaded from a harness buffer (`loadSlotSync (pcm)`, no source string) is never persisted, so a restore leaves it alone. In the plugin every load has a source, so PD11's rule applies unchanged there. This keeps every carried row that restores state over buffer-loaded slots byte-identical.
9. **N2 / N2s / FN3 force rec_arm On** after randomising the 7 `rec_*`. A random arm value would otherwise often be Off and leave the rows vacuous. FL7 (eg24, not to be edited) still randomises the 7 with `readSet25b` and passes (silent SC).
10. **Stimulus fixes (gate design, not law):** AL26's missing resolve needed a non-matching hash (the rung-3 search otherwise found the real take); TL3 ran 64 samples after a Reset where the swap waits ≤ 20 ms for the head to park; HS2n samples every 12th code to stay under the 30 s decode cap.
11. **Test hooks / probes** added under `OGI_TEST_HOOKS`: recorder (`startOffset`, `prerollOff`, `armLevelTrigger`, `capOff`, `nnAnyNote`, counters, `probe()`, `captureAt`), engine (`baseBypass`, `scNoClear`, `swapKillHard`, `spawnerForTest`), loader (`reencodeOnLoad`, `searchUnbounded`, `floatWriterPath`, `failWav`, counters, `waitForIdle`, `getPristine`, `getBlob`), processor (`testParseIsBool`, `commitSync`).
12. **Telemetry recorder peak.** The recorder keeps a post-gain sidechain peak (`takeScPeak`) next to GiTelemetry's raw-input `scInPeak`; Stage 3 picks one.
13. **parameter-spec `:353` "Done / Discarded"** states map to Idle + reason (PD15); reconcile in Stage 3.

## Requirements moved (verify sets the final status)
FUNC-06 automated arm complete. FUNC-07 automated arm complete (feel = HANDS-ON-2.6). QUAL-02 engine path in (incl. the bypass leak fix); cross-host = Stage 4. FUNC-08 groundwork (factory path + 2 stubs) in. PERF-01 record / state allocation arm (AL26); PERF-03 commit / host-idle arm (CM3, BS26). QUAL-01 commit / state-load click gates (CK6, S6 decision).

## State size (INFO, P18)
A 1 s stereo 48 k take embeds as ≈ 288 k base64 chars (tonal SS3 take). The SS4-sized worst case (15 s @ 192 k full-scale noise) is not embedded (> 16 MiB). Logic save time with 4 large blobs is HANDS-ON-2.6 row 11.

## HANDS-ON-2.6
Written (13 rows), deferred post-Stage-3 with 2.1–2.5b. Not self-certified.
