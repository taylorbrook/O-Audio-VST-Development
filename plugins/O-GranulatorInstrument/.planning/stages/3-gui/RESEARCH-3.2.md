# O-GranulatorInstrument — Stage 3 (GUI) — Phase 3.2 — RESEARCH

**Researched:** 2026-11-10
**Scope:** Phase 3.2 only (D43–D46, closes UI-01): live lanes (playheads, grain dots, feedback head, record head, evolving overview), Record-in states, `clearSlot`, `saveRecording`, durable drops, preset slot rule. MOD sources, voice strip, meters, mod rings and the `grain_limit` hint stay 3.3.
**Tree:** `main` at `fa89eff7`. Parent O-Granulator at its current head.
**Confidence:** HIGH on code facts (read this session, `path:line`). MEDIUM on the designs in §2–§8 (designed here, not built). Nothing was built or run; the page was not opened. Each claim is **VERIFIED** (read) or **INFERRED** (reasoning).
**Abbreviations:** I = `Source/ui/public/index.html`; T = `.planning/mockups/v1-PluginEditor.cpp`; P = `plugins/O-Granulator`; D = `Source/dsp`.

**Inputs read:** `CONTEXT-3.2.md`, `RESEARCH-3.1.md`, `SUMMARY-3.1.md`, ROADMAP 3.2, `parameter-spec.md:325-355`; `D/{GiTelemetry.h,GiRecorder.h,SlotHandoff.h,SlotLoader.h,SlotTape.h,VoiceGrain.h,GiHash.h,forked/Grain.h,forked/UiTelemetry.h}`, `D/Engine.cpp` (publishTelemetry, foldOverview, takeTapes, retireScan, updateHeads), `D/SlotLoader.cpp` (request / publish / commit / restore / embed), `D/MipTape.h` (accessors); `Source/{PluginProcessor.h,PluginProcessor.cpp,PluginEditor.cpp,GiUiJson.h}`; I (slots, drop, telemetry, `renderRec`, `drawLane`, counts); T `:592-705`; `modules/core/webview-drop-streaming/cpp/{WebViewDropStreaming.h,DropSessionGuard.h}` + its JS single-file flow; `tests/i18n-states.json`, `tests/render-harness/engine_gates_26b.h` (TL rows); `P/.planning/stages/3-gui/SUMMARY-3.2.md`, `P/Source/PluginProcessor.cpp:670-790`.

---

## Summary (what the plan must absorb)

1. **The engine's grain frame does not carry what the page draws.** The page reads `[stream 1–4, normPos, pan, env, gain, lengthSec, voice, flags]` (I:4294-4308); `GiUiGrain` has only `voice, stream (0-based), flags, normPos, gain = killGain` (`GiTelemetry.h:57-62`). No pan, no window envelope, no length. Without them every dot sits on the centre line at a fixed size and brightness. The Grain already holds the inputs (`gainL/gainR, age, frac, invLen, len, windowStep, tiltT/A/B`, `forked/Grain.h`). Extend `GiUiGrain` and derive on the reader side exactly as the parent does (`P/Source/PluginProcessor.cpp:769-786`). §2. **VERIFIED** (gap) / **INFERRED** (fix). **[decision for plan]**
2. **Flag bit 2 means two different things.** Engine: "killing / fade slot" (`Engine.cpp:788`). Page: "pitch hit the ±48 st clamp", drawn in the warning colour (I:4306). Emitted as is, every fading grain turns red. No clamp marker exists on a grain. 3.2 emits bit 1 (reversed) only, uses "killing" to exclude the grain from `counts`, and leaves the clamp bit 0 (the voice-level `wantSt` is 3.3). **VERIFIED.**
3. **Playheads work from the existing frame.** `GiUiVoice` has `note, amp, scan[4]`; the page needs `[state, note, amp, scan1..4, wantSt]` and draws a playhead only when `state != 0` and `scan >= 0` (I:4284-4293). Emit `state = 1` for a counted voice, `scan = −1` for a stream without a tape (the engine writes 0 there, which would draw a line at x = 0), `wantSt = 0`. Held vs releasing (state 2) is not in the frame; add one bit or leave every counted voice as 1 until 3.3. §2. **VERIFIED.**
4. **The live overview exists; only its revision is missing.** `GiOverviewLive` holds head-rewritten bins per slot, epoch-tagged, readable from any thread (`GiTelemetry.h:76-134`). The head writes only while feedback > 0 and the head runs (it parks at feedback 0 or run 0, `Engine.cpp:3735-3738`), so untouched tapes have no live bins. `ovRev` today moves only on a publish (`PluginProcessor.cpp:942-961`), so the lane would never redraw during feedback. Add one relaxed per-slot write counter in `GiTelemetry`, bumped in `foldOverview`, and fold it into `ovRev`. `dirty` = how far a live bin differs from the pristine bin, computed in the reader. §3. **VERIFIED** / **INFERRED.**
5. **There is no evolving overview while Record-in captures.** The capture goes to the recorder's private buffer (`GiRecorder.h:322-330`); the slot's tape is replaced only after commit. `lanes[i].rec` (I:4317-4322) can show the progress head (`frames / target`) over the old lane content; the new waveform appears at commit through the normal publish. CONTEXT's "the lane redraws while recording" is met by the head and tint, not by a growing waveform, unless a capture overview is added (not recommended). §4. **VERIFIED.** **[flag for plan / user]**
6. **`rec.run` 4 / 5 needs an editor-side hold.** The recorder's reason is sticky: it is cleared only in `enterArmed` (`GiRecorder.h:433`). Mapping Idle + `Committed` → 4 straight through would show "Done" forever after the first take, and "Discarded" would mask "no signal" permanently. Hold 4 / 5 for a fixed time from the observed transition (edge-detected in the editor), then fall to 0. §4. **VERIFIED.**
7. **`clearSlot` is a 15-line extraction.** The body is the `w == nullptr` branch of `restoreSlots` (`SlotLoader.cpp:1255-1272`). Refuse while the recorder is Recording or Committing into that slot: a commit job arrives after the clear and would refill the lane. §5. **VERIFIED.**
8. **D45 is one flag on `restoreSlots`.** The preset path (`PluginProcessor.cpp:934-935`) passes "leave unlisted slots alone"; the session path keeps clearing. §5. **VERIFIED.**
9. **D44: move, don't copy; hash the file bytes, not the decoded head.** The module never touches the file after `onCommitFile` (`WebViewDropStreaming.h:432-459`), so a rename into Imports is safe and normally O(1). `gihash::contentHash` covers only the first 131,072 frames (`GiHash.h:58`): equal-length stems with a silent head would collide and the second drop would silently load the first file. Name by XXH64 of the whole file. The lane name must be carried separately or the lane shows the hash. §6. **VERIFIED.**
10. **A failed load or drop is silent today.** The page never reads `slots[i].error` (no reader in I) and has no toast key for it; the module's commit result is ignored by its JS (`webview-drop-streaming.js:382`). CONTEXT requires a toast on a refused drop, so 3.2 adds one key (en / fr / zh-Hans) and a page reader. This is the one new string of the phase. §6. **VERIFIED.** **[decision for plan]**
11. **D46: read the working copy on the audio thread, in chunks.** The tape is audio-owned (`current[]` is a plain pointer; tapes are freed by sweeps on other threads), so any off-thread read needs a pin protocol and still races the head's writes. Recommended: the message thread hands the engine a buffer, the audio thread copies level 0 into it a chunk per block, a worker writes the WAV. One writer thread, no lifetime question. Cost: it cannot complete while the host is not calling `processBlock`; that case answers `busy` after a timeout. §7. **INFERRED.** **[decision for plan]**
12. **Exact `encodedBytes` is free once a blob exists** (base64 length × 3 / 4) and unknowable before the encode. Keep 0 before, exact after, cap + 1 on `tooLarge`. §8. **VERIFIED.**
13. **The parent proved lanes in the harness, not in the page.** Dots against the spawn log, counts against a census, torn-read and allocation rows, payload size and read cost as INFO, then the overrides file regenerated and the closure script extended with "emitted and read" key rows (`P/…/SUMMARY-3.2.md`). All of it ports. §9. **VERIFIED.**

---

## 1. Current state — VERIFIED

| Piece | Today | 3.2 |
|---|---|---|
| `getTelemetry` | 3.1 shape, placeholders (`GiUiJson.h:189-248`) | real `lanes[].fb / .rec`, `voices`, `grains`, `counts`, `limit`, `rec.*` |
| Consuming feeds | unread | `frames.read()` and `takeScPeak()` read by the editor only (one consumer each) |
| `getSlotOverview` | pristine fold, `dirty` all 0 (`PluginProcessor.cpp:1006-1035`) | pristine + live bins, real `dirty` |
| `ovRev` | `2·(generation + published) + loaded` | + per-slot live write count |
| `clearSlot`, `saveRecording` | refuse `unavailable` (`PluginEditor.cpp:793-859`) | live |
| Drop | loads from the session temp dir (`PluginEditor.cpp:554-559`) | moved to Imports, `file:` source |
| Preset slots | unlisted sourced slots cleared | left alone (D45) |
| `slots[i].dirty`, `.encodedBytes` | `false`, 0 / cap + 1 (`GiUiJson.h:129-144`) | real |

Thread facts the designs rest on:
- `TripleBuffer<GiUiFrame>` is single-reader; one frame per host block; the reader always gets a complete frame (`GiTelemetry.h:37-39`, row TL2).
- `headPos[4]`, `swapState[4]` (bit 2 = head parked), recorder `packed / framesTel / targetTel / slotTel` are relaxed atomics (`Engine.cpp:761-769`, `GiRecorder.h:398-401`).
- `SlotLoader.*`, `Engine.*`, `GiTelemetry.h`, `PluginProcessor.*` are not forked files; fork drift stays 0 / 13.

## 2. Lanes: voices, grains, heads

**Page contract (I:4229-4323).**

| Key | Page reads | Source |
|---|---|---|
| `lanes[i].fb` | number 0…1 → ochre feedback head; dashed when `freeze` is on; `null` → none | `headPos[i] / frames` |
| `lanes[i].rec` | number 0…1 → record head + tint, lane marked recording; `null` → none | recorder progress (§4) |
| `lanes[i].lo / .hi` | modulated loop region; absent → the handles | omit in 3.2 (mod telemetry is 3.3) |
| `voices[k]` | `[state, note, amp, scan1..4, wantSt]`; playhead when `state` ≠ 0 and `scan ≥ 0`; alpha from `amp`; larger cap when `state == 1` | `GiUiVoice` |
| `grains[]` | `[stream 1–4, normPos, pan −1…1, env, gain, lengthSec, voice, flags]` | `GiUiGrain` (extended) |
| `counts[4]`, `limit` | caption "total / limit" and per-stream bars (I:4518-4538) | frame census; `limit` 0 → the page parses the `grain_limit` choice |

**Feedback head.** Emit `fb` when the slot has a tape and (the head is not parked, or `freeze` is on with feedback > 0), else `null`. The head parks when frozen, so "not parked" alone would hide the dashed frozen head the page draws (I:4274-4281). At a loop wrap the value jumps from `hi` to `lo`; the page draws a line per frame, no interpolation, so no special case. During a swap the head is parked (`takeTapes` waits for it, `Engine.cpp:1055-1063`) and `fb` is `null`. `frames` comes from `SlotInfo::frames` (message thread); a one-poll mismatch around a publish is clamped to 0…1.

**Voices.** 20 slots in the frame, 16 counted at most. Emit 20 rows (the page iterates what it gets; the template's 16 was a guess, T:625-633) or the counted ones only. `newest` stays −1 and `menv` `[]` in 3.2 (voice strip and env dots are 3.3; `drawAmpFigure` reads `voices` too, I:4180-4187, and falls back cleanly when `menv` is empty: held → stage 3). Side effect to accept or suppress: with real `voices`, the amp figure starts showing level dots in 3.2. **[decision for plan: accept (recommended) or keep `voices` state at 0 for that figure — not possible separately, so accept]**

**Grains — the gap.** Recommended frame extension (audio thread stays copy-only; `GiUiFrame` grows from about 3.5 KB to about 10 KB, still one POD copy per block):

| New `GiUiGrain` field | From | Reader derives |
|---|---|---|
| `gainL, gainR` | `g.gainL / gainR` | `pan` = `atan2 (R, L)·4/π − 1` (exactly 0 when equal, the parent's GD2 fix) + stream pan; `gain` = `sqrt ((L² + R²) / 2)` |
| `phase` | `(g.age + g.frac)·g.invLen` | `env` = bank window at `(windowStep, tilt)` × `killGain` (needs an `Engine::grainWindow` reader as the parent's; the bank is immutable after construction) |
| `lenSamples` | `g.len` | `lengthSec = len / fs` |
| `windowStep, tiltT/A/B` | same | as above |

`stream` goes out 1-based (the page compares to `n`, I:4298; the engine stores 0-based). Cheaper alternative: no frame change, `pan = 0, env = 1, lengthSec = 0.1`; dots then mark position only. It closes UI-01's letter ("grain dots by stream") but drops the v1 look. **[decision for plan: extend (recommended)]**

**Counts.** `counts[s]` = grains in the frame with stream s that are neither in a fade slot nor killing (the parent's rule). `limit` 0.

**Payload.** Worst case 288 grain rows of 8 numbers at 30 Hz. The parent measured 16 KB for 128 grains and about 10–18 µs per read (PL1). Expect about 35 KB here; measure as an INFO row and round floats to 3–4 decimals in the encoder if the JSON build shows up.

**Stale frames.** When the host stops calling `processBlock` the last frame stays in the buffer and the dots freeze on screen. The parent zeroes consuming values when the previous read is older than 250 ms; here compare the frame's `seqHead` across polls and send empty `voices / grains` once it has not moved for about 250 ms. **INFERRED.**

## 3. Evolving overview and `dirty`

- **Reader.** `getSlotOverview` keeps its pristine fold and, per pristine bin `b`, replaces the pair with `tel.overview.read (slot, b, lo, hi)` when that returns true (epoch-valid). Bin size matches by construction (`kOverviewBin` = 512 in both, `GiConstants.h:71`, `GiTelemetry.h:78`). Cost per fetch: up to 5,625 pairs of relaxed loads, at most 10 Hz per lane (the page's `OVERVIEW_MIN_MS`, I:1861, 3899).
- **`dirty[i]`.** For each of the 656 peaks: max over its bins of `|liveLo − pristLo| + |liveHi − pristHi|`, scaled to 0…255 (the page lights a tick above 5 %, I:4260-4261). A valid live bin that equals pristine (feedback writing back what it read) stays clean. `slots[i].dirty` (Reset button highlight, I:3768-3770) = any live bin differs; compute it in the same fold and cache per `ovRev`, or define it as "any epoch-valid bin" (cheaper, slightly over-reports). **[decision for plan]**
- **Revision.** Add `std::atomic<std::uint32_t> ovWrites[kTapeSlots]` to `GiTelemetry`, incremented once per `foldOverview` call (relaxed; the engine never reads it). New `ovRev` = old formula with the count added. The page limits refetches to 10 Hz per lane and re-asks on the next frame when limited, so a counter moving every cell is fine. Reset and new tapes already move `ovRev` through the publish count and bump the epoch (`Engine.cpp:1079, 1097`).
- **Page defect to fix (D40 class).** `fetchOverview` stores the telemetry `rev` it was called with, not the answer's `rev` (I:3906). With a revision that moves every poll this is harmless (it refetches anyway), but the answer's `rev` is the correct value to store; one-line change, mirrored in `v1-ui.html`.
- **Audio neutrality.** One relaxed increment in a telemetry path; renders must stay byte-identical (checkpoint after this task).

## 4. Record-in panel

**Mapping** (page order of tests: bus → 2 → 3 → 1 → 5 → `!signal` → 4 → 0, I:3925-3934):

| Engine (`getRecordState`) | `run` |
|---|---|
| Armed / Recording / Committing | 1 / 2 / 3 |
| Idle, reason `Committed`, within the hold | 4 |
| Idle, reason `NoSignal` or `TooShort`, within the hold | 5 |
| Idle otherwise (`None`, `Cancelled`, `Interrupted`, `Bypassed`, `SidechainUnavailable`, `WavFailed`) | 0 |

- **Hold.** The reason never clears by itself. The editor keeps `lastPacked` and a timestamp; a change of the packed word into Idle + {Committed, NoSignal, TooShort} starts a 4 s hold; any other state ends it. A first poll that already finds Idle + Committed (editor opened later) starts no hold. Because 5 outranks "no signal" on the page, the hold must end or "Discarded" hides the live signal state.
- **`WavFailed`.** The take is published but its WAV was not written (`SlotLoader.cpp:1013-1016`); the lane works, a reload loses it unless embedded (recordings embed by default, D36). No page state exists for it. Leave silent in 3.2 or show it with the new failure toast (§6). **[decision for plan]**
- **`rec.slot`.** `getTargetSlotTelemetry() + 1`, written at take start (`GiRecorder.h:480`); before the first take the page falls back to the `rec_target` choice.
- **`rec.pos`, `rec.len`.** `pos = frames / target`, `len = target / fs` from `framesTel / targetTel` (updated per piece, `GiRecorder.h:345`). Needs the engine sample rate on the message thread (`Engine::getSampleRate`, a plain double set in prepare; read it through a processor accessor that already exists for the harness or add `getSampleRate()` use — the processor's own `getSampleRate()` is the same value).
- **`lanes[slot].rec`.** `pos` while run = 2 for the target slot, else `null`. It marks the lane recording even when the lane is empty (I:4237-4238, 3743-3745).
- **`rec.signal`.** Post-gain `takeScPeak()` (CONTEXT default). The template comment says "rose above −90 dBFS within the last 2 s" (T:682): keep a last-above timestamp in the editor; threshold `GiRecorder::kPresenceLin`. The peak is mono-summed (`max (|L|, |R|)`, `GiRecorder.h:296`), so `rec.peak = [p, p]`; the page's two bars move together. 3.3 owns the meter; sending the peak now is harmless and avoids a second consumer later. Note `scPeakPush` runs only inside `runPiece`, i.e. only while `processBlock` runs with the engine active; with audio idle the peak is 0 and the panel reads "no signal", which is correct.
- **`rec.capped`.** Shown only in Tempo mode while not recording (I:3948). True when `bars × barLength > F_max`. The message thread has no tempo today (the transport snapshot is built inside `processBlock`, `PluginProcessor.cpp:273-283`). Either add two relaxed atomics on the processor (bpm, bar length in quarters) written per block, or send `false` and let the cap show only as the truncated `len` during the take. **[decision for plan: atomics (recommended; the string and state already exist in v1)]**
- **Evolving lane during capture.** Not available (Summary 5).

## 5. `clearSlot` and the preset rule

**`SlotLoader::requestClear (slot)`** (message thread): extract the `w == nullptr` branch of `restoreSlots` into `clearLocked (s)`; call it from both. It cancels the active job, bumps the generation (a job in flight is superseded and dropped at `publishResult`, `SlotLoader.cpp:631-632`), publishes the clear sentinel when a tape is loaded, and resets the record keeping the generation. The slot fingerprint and `ovRev` move through the generation and publish count, so the lane and state re-pull with no new code. A missing slot (reference kept, nothing loaded) clears to empty too: `r.info.source` is non-empty, `loaded` false, so no sentinel is published.

Interactions:
- **In-flight load on the same slot:** cancelled and superseded. **VERIFIED** (mechanism).
- **Recording into that slot:** refuse in the editor when state ∈ {Recording, Committing} and `slotTel == slot` (`{ ok: false, error: "recording" }`). Armed is allowed (the take would land later in an empty lane, which is what arming an empty lane does anyway).
- **Recording into another slot:** independent.
- **A harness-loaded buffer without a source** is skipped by the current branch (`:1260`). `requestClear` should clear by `loaded || source`, or UI-less rows cannot exercise it.
- The page disables Clear on an empty lane (I:3763-3766) and shows no toast on refusal; fine under D40.

**D45.** `restoreSlots (slots, bool clearUnlisted = true)`; `applyPresetCustomState` passes `false`. Session restore unchanged (SUMMARY-2.6 dev. 8). UPM6 / UPM6b are amended; the negative control flips the flag back and shows the lane cleared.

## 6. Durable drops (D44)

**Flow** (`onCommitFile`, message thread, `PluginEditor.cpp:554-559`):
1. Hash the committed temp file: XXH64 over all bytes. The module caps a file at 256 MiB (`DropSessionGuard.h:64`); a one-shot read + `gihash::xxh64` is simple but holds the file in memory once more, a chunked loop needs a streaming XXH64 (about 40 lines). At ~1 GB/s a worst-case file is ~0.25 s. Run hash + move on the loader worker, not inline. **[decision for plan: worker job kind `Import` (recommended) vs inline for small files]**
2. Target `Imports/<hash16>.<ext>` (extension kept, lower-cased). Exists with the same size → reuse, delete the temp file. Otherwise move: `moveFileTo` (rename; temp and `~/Music` are on the same volume on a default macOS install, JUCE falls back to copy + delete across volumes). Write through a `.part` sibling then rename, so a crash never leaves a short file under a hash name.
3. Load as `file:<Imports path>` with the **display name = the dropped file's name** (the temp file keeps it: `relPath`). `runLoadFile` sets `ns.name = file.getFileName()` (`SlotLoader.cpp:932`), so it needs a name override parameter; `SavedSlot::name` already persists and restores it.
4. Any failure (directory, move, disk full) → no load, the slot is left as it was, and the page gets the failure toast. Never fall back to loading the temp path.

- **Imports directory.** `…/O-GranulatorInstrument/Imports`, sibling of Recordings, resolved lazily; add `setImportsDirectory` beside `setRecordingsDirectory` so the harness redirects it (same guard as `main.cpp:95-117`, with a non-ASCII path component).
- **Deleted import later.** Rung 1 fails; rung 2 only if embedded; rung 3 searches the Recordings folder only; → missing, relink badge. Relink by `fileSize` / hash works as for any `file:` source. No ladder change needed. **VERIFIED** (`SlotLoader.cpp:1082`, spec `:331`).
- **Reporting failure to the page.** The module's JS ignores the commit result, and `onCommitFile` returns void. Two options: (a) the page reads `slots[i].error` after its post-drop `pullSlotState` and shows a toast when it changes to non-empty (also covers decode failures from the file browser, silent today); (b) wrap the commit native function. (a) needs `lastError` set by the import failure path and one new key `toast.loadFailed` with `{ n }`. **Recommend (a).** New key → `i18n.js` en / fr / zh-Hans, glossary + lint, and a state (§9).
- **Folder button** (`loadSlot` chooser) keeps referencing the file in place (`file:<original path>`); only drops import. **VERIFIED** against D44's wording.
- **Windows.** The WebView2 path of the module is untouched; no Windows check in 3.2 (CONTEXT).

## 7. Save recording (D46)

**What is read.** Level 0 of the slot's `MipTape`, `frames` × 2 channels at `tape->rate` (a tape is always stereo; mono sources are built L = R, `SlotLoader.h:49`). Up to 2.88 M frames = 23 MB of float.

**Options.**

| Option | Tear / lifetime | Cost | Verdict |
|---|---|---|---|
| A. Audio-thread chunked copy into a message-allocated buffer; worker writes the WAV | none: one thread reads and writes the tape; a tape swap mid-copy aborts | engine: a request struct + ~30 lines in the block path; about 130 KB memcpy per block at 16 k frames per block (≈ 180 blocks for a full tape); needs a running audio thread | **recommended** |
| B. Pin the tape (atomic pin checked in `retireScan`), read on a worker | lifetime solvable with a store-then-recheck handshake, but the read races the head's writes (formally a data race) and every free path (`~Engine`, prepare) must honour the pin | less engine code in the block path, more in retirement | fallback if A's idle case is unacceptable |
| C. Read `current[s]` from the message thread | use-after-free possible | — | rejected |

**Design A details.**
- Request: `{ slot, buffer L/R (worker- or message-allocated to SlotInfo::frames), cursor, state }` behind one atomic state word (Idle → Requested → Copying → Done / Aborted). The audio thread checks one relaxed atomic per block when idle.
- The copy sweeps far faster than the 1× head, so it crosses the head at most once; every copied frame is a value the tape held during the copy window. No half-written frame is possible.
- Abort when the slot's tape changes (take, clear, reset) or `frames` differs from the request; the native function answers `{ ok: false, error: "changed" }`.
- Completion: the editor polls the state (the existing 30 Hz native call can drive it, or a short `juce::Timer` owned by the save flow) and hands the buffer to a background job that writes the file; `complete` fires from the message thread on every path: cancelled chooser, `busy` (another dialog or save in flight), `changed`, `timeout` (no audio callback for about 2 s), write error, ok + path. `SafePointer` on every async hop (3.1 pattern).
- Order: chooser first (the user may cancel), snapshot second, so the saved audio is "as it sounds" at confirm time.

**File.** WAV, 24-bit integer, stereo, tape rate. Feedback writes are clamped to ±0.7 (`kAgcCeil`, `forked/EngineConstants.h:146`), but the pristine take is float and can exceed ±1.0 with `rec_gain` up; 24-bit clips those samples. Accept (D46 says 24-bit) and note it in HANDS-ON, or write float when the snapshot peak exceeds 1.0. **[decision for plan: accept the clip (recommended, matches D46)]**

**Chooser.** Save mode, `warnAboutOverwriting`, start in the Recordings folder, default name `<take stem>-edit.wav`. **Refuse the slot's own `rec:` path** (and any path another slot references): overwriting the pristine under a live reference makes the next reload a hash mismatch.

**Non-ASCII paths.** `juce::File` round-trips; never build a path through `String (const char*)`.

**Guard.** `scheme == "rec"` stays the entry condition (`PluginEditor.cpp:853`); an evolved `file:` lane has no Save in v1 (D46: button where v1 puts it).

## 8. `encodedBytes`

- Known only after a FLAC encode. With a blob: bytes = `blob.length() · 3 / 4` minus padding. Store it in `SlotInfo` where the blob is installed (`publishResult`, `runEncode`, `restoreSlots`) rather than copying a 22 M-character string per state pull (`getBlob`).
- Without a blob: 0. The page's `toobig` class (I:3761) then lights only after an attempted embed, as in 3.1; estimating before the encode would mean encoding on load, which D36 avoids.
- `tooLarge` keeps cap + 1. `embeddable` already derives from the same number (`GiUiJson.h:143`).

## 9. Verification method

**Harness rows (new file `ui_gates_32.h`, U-prefixed; ported from the parent's GD / CN / TB / PL set).**

| Row | Checks | Negative control |
|---|---|---|
| UGD1 | each emitted dot's `normPos` against the engine's own `readAbs / frames` for a scripted note; stream 1-based | stream left 0-based → stream-1 dots 0 |
| UGD2 | pan / gain / env derivations: spread 0 → pan exactly 0, gain 1; hard pan → ±1; env against the bank | |
| UCN1 | `counts` sum == an independent pool census every frame; killing grains excluded | killing grains counted → sum > census |
| UPH1 | playhead `scan` for a held note inside [lo, hi]; `−1` for a stream without a tape | 0 emitted for no tape |
| UFB1 | `fb` null at feedback 0, tracks `headPos / frames` with feedback on, non-null and still when frozen | |
| UOV2 | live bins replace pristine after feedback writes; `dirty` > 0 only where the tape changed; `ovRev` moves while the head writes and after Reset `dirty` returns to 0 | no write counter → `ovRev` static |
| URC1 | `run` 1 / 2 / 3 from a scripted take; 4 held then 0; 5 for a silent take then 0; `pos` monotonic 0 → 1; `lanes[slot].rec` only for the target | no hold → 4 forever |
| UCL1 | clear empties a loaded slot through the sentinel; refused while recording into it; an in-flight load is superseded | |
| UPM6c | D45: an empty preset entry leaves a loaded lane and its working copy | flag flipped → cleared |
| UIM1 | drop import: file lands in the redirected Imports dir under its hash, source is `file:`, name is the original, a repeat drop reuses one file, session round trip reloads it; non-ASCII directory | name not carried → hash shown |
| UIM2 | import failure (unwritable dir) → slot unchanged, `error` set, no temp-path source | |
| USV1 | save snapshot equals the tape level 0 bit-exactly at feedback 0; with feedback on every sample lies within the tape's values over the window; WAV reads back 24-bit stereo at the tape rate; own-path refused; tape swap → `changed`; no audio → `timeout` | |
| UEB1 | `encodedBytes` 0 before embed, exact after, cap + 1 when too large | |
| INFO UPL1 | widest telemetry JSON and read cost with 256 grains | |
| M8-style | 0 allocations on the audio thread with the frame extension and a save in flight | |

Carried set after every engine-touching task: harness 374 / 0 carried rows unchanged, parity 168 / 0 + GV 2 / 0, renders-2.4 + legacy byte-identical, param-dump identical, drift 0 / 13, X2 0; auval; pluginval 10 VST3 + AU with the sidechain on and off. `GiUiFrame` changes size, so row TL2 (`engine_gates_26b.h:1006-1042`) is re-read for a size assumption.

**Page proof.**
- `--dump-ui-overrides` regenerated with a 3.2-shaped telemetry object (one voice, a few grains per stream, `fb`, a recording lane, real `rec`), still byte-identical twice.
- `ui_frontend_check.js`: add "emitted and read" rows for every 3.2 key (`fb`, `rec`, `voices`, `grains`, `counts`, `rec.run / slot / pos / len / signal / capped / peak`, `dirty`, `encodedBytes`, `error`) and a row that no `unavailable` answer remains.
- `ui_binding_probe.js`: unchanged counts (261 × 2, VE 14); add lane rows driven by the stub: recording class on the target lane, each `rec-status` class for run 0–5 and bus 0, Clear / Save / Relink buttons call their natives with the 1-based slot, the failure toast appears when `error` turns non-empty.
- Canvas content (dots, heads, dirty ticks) is not assertable by the label gates; one pixel-probe row per feature on the stub page (a non-background pixel at the expected x) is cheap and catches the 0-based / 1-based class of bug. Canvas text is not involved in 3.2.

**i18n states (`tests/i18n-states.json`, 19 today).** Covered now: `rec.waitLevel`, `rec.capped`, `rec.noSidechain`, `rec.recording`, badges, drag-over. Add states for the keys that become reachable: `rec.armed`, `rec.waitNote`, `rec.committing`, `rec.done`, `rec.discarded`, `rec.noSignal`, `rec.slotN` (progress line while recording), `lane.recording` / `lane.missing` hints, and the toasts `toast.loading`, `toast.dropOneFile`, `toast.tooLarge`, `toast.slotReset`, `toast.allReset`, plus the new failure toast. Drive them through the stub's telemetry / slot state where possible rather than `__setLabel`, so the state also proves the page path; keep `__setLabel` for the widest-string checks. Run the five gates on the unmodified tree first (baseline), as in 3.1.

**Human (HANDS-ON-3.2 + the 16 rows of HANDS-ON-3.1, D43).** Real WKWebView drop on each lane; quit and reopen after a drop (lane restored from Imports); delete the import → relink; record with Manual / Threshold / Next Note and read each status; silent sidechain → Discarded then "no signal"; no sidechain bus → "No sidechain"; feedback on → head moves, ticks appear, Reset clears them; Save → open the WAV elsewhere; Clear during a take is refused; a parameter-only preset leaves lanes alone; `v1-ui.html` == `index.html`.

## 10. Pitfalls specific to 3.2

- **One consumer per consuming feed.** `frames.read()` and `takeScPeak()` are read only inside `getTelemetry`; the overrides dump and harness rows must use their own processor instance, never a second reader on a live one.
- **Two editors** (some hosts open a second view): two readers on a single-reader `TripleBuffer`. Put the read behind a processor-side reader lock as the parent does (`telemetryLock`, `P/Source/PluginProcessor.cpp:707`), message thread only.
- **`voices` with `scan = 0`** draws a playhead at the lane's left edge for every tape-less stream; emit −1.
- **`rec.bus`** stays 0 / 1; `lanes[i].rec` and `.fb` stay `null` (not 0) when absent; `counts` stays four numbers (3.1 shapes, RESEARCH-3.1 §3).
- **Every new native path completes**, including the save flow's timeout; no completion while the WebView is hidden is expected because the editor is never hidden (3.1), but the save poll must not depend on a page call arriving.
- **`index.html` and `mockups/v1-ui.html` stay byte-identical** (entry and exit `cmp`); the language-apply wrapper lives in `init()`, not in the canon region (3.1 deviation 8). The scan-mode `aria-valuetext` fix goes in that wrapper.
- **Imports / Recordings in tests:** redirect both before any processor work; a test that writes to the real `~/Music/Ouaricon/…` is a defect.
- **Move across volumes** degrades to copy + delete inside JUCE; still off the message thread.
- **Preset + recording:** `rec_arm` stays excluded from presets; D45 does not change that.
- **Test hooks** for the new rows stay under `OGI_TEST_HOOKS` (X2 0).
- **Not verified here:** that `GiUiFrame` at ~10 KB keeps the publish cost in the noise (the parent's C7 moved by less than its run-to-run spread with a 7 KB snapshot); the real WKWebView drop after the move (hands-on); behaviour of Logic when the instrument track is idle during a Save (drives the option A timeout).

## 11. Answers to the CONTEXT open questions

| Question | Answer |
|---|---|
| `GiUiFrame` vs the lane painter | §2. Voices fit (state synthesised, `scan −1` for no tape). Grains lack pan / env / length and use 0-based streams; flag bit 2 conflicts. Playheads belong to 3.2; `newest`, `menv`, `wantSt` stay empty until 3.3. Caps: 288 grains, 20 voice rows. |
| Live overview source, cadence, `ovRev` | §3. `GiOverviewLive` over the pristine fold; `dirty` = live ≠ pristine; a new per-slot write counter moves `ovRev`; the page already limits to 10 Hz. |
| Record head at the wrap and during a swap | §2. Jumps at the wrap (no interpolation on the page); `null` while parked for a swap. |
| `rec.run` 4 / 5 hold; `pos` source | §4. The reason is sticky → editor-side 4 s hold from the observed edge. `pos = framesTel / targetTel`. |
| D44 move vs copy, hash, collisions, deleted import | §6. Move (rename); XXH64 of the whole file, not `contentHash`; `.part` then rename; a deleted import goes missing → relink, no ladder change. |
| D46 read, format, folder | §7. Audio-thread chunked copy-out; 24-bit stereo WAV at the tape rate; chooser starts in Recordings; own path refused. |
| `requestClear` interactions | §5. Supersedes an in-flight load; refused while recording / committing into the slot; revisions move through the existing terms. |
| Exact `encodedBytes` | §8. Free after an encode, 0 before. |
| Parent's headless lane proof | §9. Harness rows on the C++ telemetry + overrides + closure "emitted and read" rows; all port. |
| `[data-i18n]` nodes going live | §9. Six `rec.*` states, `rec.slotN`, two lane hints, five toasts, one new toast key. |

## 12. Decisions for the plan

1. **Grain frame:** extend `GiUiGrain` for pan / env / length (recommended) or ship position-only dots.
2. **Save read path:** A, audio-thread chunked copy-out with a `timeout` answer when the host is idle (recommended), or B, pin + worker read.
3. **Failure toast:** add `toast.loadFailed` and a page reader of `slots[i].error` (recommended; required by the CONTEXT constraint for refused drops). Also show `WavFailed` through it, or leave that silent.
4. **Capture overview:** accept head + tint only during Record-in (recommended), or add a capture overview feed. **Worth a one-line confirmation from Taylor at plan time**, since CONTEXT reads "the lane redraws while recording".
5. **`rec.capped` while idle:** two processor atomics for tempo (recommended) or `false`.
6. **`slots[i].dirty`:** real difference test cached per `ovRev` (recommended) or "any live bin".
7. **Import job:** hash + move on the loader worker (recommended) or inline.
8. **24-bit clip above 0 dBFS in Save:** accept (recommended, D46) or switch to float when the peak exceeds 1.0.
9. **Voice rows:** 20 rows, counted = state 1 (recommended); a releasing bit can wait for 3.3.
10. **Hold time for run 4 / 5:** 4 s (CONTEXT proposal).
