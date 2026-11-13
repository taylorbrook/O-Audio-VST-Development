# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.6 — CONTEXT

**Date:** 2026-11-09
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:**
- `parameter-spec.md` (LOCKED, 261 params; `rec_target` Slot 1–4, `rec_arm`, `rec_trigger` Manual / Threshold / Next Note, `rec_threshold` −60…0 dB, `rec_length` 0.5–30 s, `rec_length_sync` 1–32 bars, `rec_gain` −24…+12 dB, `sync_mode`; `sN_loop_start` / `sN_loop_end` are parameters)
- `REQUIREMENTS.md` (FUNC-06, FUNC-07; QUAL-02 engine path; FUNC-08 groundwork only, the set + presets are Stage 4; PERF-01 / PERF-03 / QUAL-01 arms)
- `research/ARCHITECTURE.md` §5 (Slot loader), §6 (Slot handoff + reaper), §22 (Record-in capture), File I/O and Sources, State Persistence (`<Slots>`, resolve order), Record-In Availability (D6), Threading Model, Telemetry, A7 (factory BinaryData)
- `ROADMAP.md` § Phase 2.6
- `stages/2-dsp/CONTEXT-2.5b.md` (D29–D32), `SUMMARY-2.5b.md`, `VERIFICATION-2.5b.md`

## Starting point (code at 7ed74b2f)

- **Built in 2.1–2.4:** `SlotLoader.{h,cpp}` (1-worker pool, (slot, source, generation) jobs, Kaiser resample, mip tape build, overview, `SlotInfo` under a lock the audio thread never takes, SR rebuild via `callAsync`, 100 ms reaper), `SlotHandoff.h` (`pending[4]` exchange + SPSC retire ring), the 2.1 rebuild crossfade + `requestReset`, `GiRecordHead.h` (the 2.4 feedback write head; parked during swaps).
- **Not built:** the Record-in capture (no `rec_*` consumer in the engine), `<Slots>` parse / emit (`getStateInformation` appends an empty `<Slots>`; `setStateInformation` has a `// Phase 2.6` marker), embedding, hash, relink search, `factory:` resolution (CMake carries the `FactorySourceData` target commented out; the factory table is empty, so `factory:` resolves to missing), the Recordings-folder WAV writer, the 2.6 telemetry feeds.
- **Loop regions** are already APVTS parameters, so they persist through the APVTS path. 2.6 only proves the round trip.
- **No parent equivalent for capture.** O-Granulator @ `82406fce` has no Record-in. Proof is by law gates.

## Goal (2.6)

FUNC-06: file references persist, a missing file relinks without a crash, factory sources resolve. FUNC-07: capture the sidechain into a target slot by Manual / Threshold / Next Note. QUAL-02 engine path: the plugin degrades cleanly with no bus, or with a bus that has nothing routed. FUNC-08 groundwork: the `factory:` path works end to end. Telemetry feeds exist for Stage 3.

## Decisions (locked this session)

### D33 — Factory sources in 2.6 = the path + 2 synthetic stubs
- Enable the `FactorySourceData` `juce_add_binary_data` target (own namespace and header, per the two traps: hyphen stripping, and the `BinaryData` namespace collision with the UI target).
- Fill it with **2 small generated FLACs**. They are deterministic and licence-clean, made by a checked-in generator script (for example a harmonic tone and a noise texture, a few seconds each).
- Build the `factory:<id>` table and resolution. A `factory:` load decodes from memory with **no disk access**.
- The curated 6–8 sources (≤ 8 MB) and the presets are **FUNC-08, Stage 4**. They replace the stubs by id.

### D34 — `rec_arm` is one-shot
- An `rec_arm` Off→On edge goes **Idle → Armed**. After a commit (or a discard), the engine returns to **Idle** and stays there while `rec_arm` is still On. Another capture needs a new Off→On edge.
- The engine never writes APVTS. Stage 3's UI turns the button off through its relay, which is a user gesture. Host automation can toggle the parameter for repeat takes.
- An On→Off edge while **Armed** cancels. An On→Off edge while **Recording** stops: a take of ≥ 0.5 s commits, and a shorter one is discarded (ARCH §22).
- **Amended by D66 (Stage 4 R1):** `rec_arm` no longer stays On after a take. When a take commits, is discarded or cannot start, the processor's message-thread poll timer writes the parameter back to Off as a host-visible gesture, so one click starts the next take. An arm that is still waiting (Threshold, Next Note) is left alone, and a user cancel is not rewritten. The engine still never writes APVTS and the one-shot rule in the audio path is unchanged.

### D35 — Threshold trigger has a 10 ms pre-roll
- While Armed in Threshold mode, a preallocated ring of 10 ms at fs runs on the post-`rec_gain` sidechain. The capture starts **10 ms before** the first sample where `|sc·recGain|` exceeds `rec_threshold`. If fewer than 10 ms have passed since arming, it starts at the arm sample.
- Pre-roll counts toward `rec_length`.
- **Manual and Next Note stay sample-exact**: Manual starts at the arm edge sample and Next Note at the note-on offset, with no pre-roll.

### D36 — Embedding as ARCH, 24-bit FLAC
- **Recordings embed by default. User files are opt-in** per slot. The `embed` flag is a native action and `<Slots>` state, never a parameter. Factory sources never embed.
- The blob is base64 FLAC-24 of the **pristine** at its native rate. If the encoded size is > 16 MB, it is not embedded (`embed` stays set, `embedded` absent) and the slot resolves by path / hash.
- **"Round-trips bit-identical"** means two things. First, the decoded blob equals the 24-bit-quantised pristine, sample for sample. Second, a save → load → save re-emits the **same bytes**: the blob is carried, never re-encoded. The float32 WAV on disk stays the master copy of a recording.

## Defaults (no decision needed; binding for plan)

- **Run state** Idle / Armed / Recording / Committing. It is engine state, published to the UI as an atomic, and is never a parameter. Committing → (copy-out done) → Idle. While the buffer is busy, an arm edge waits (it is held, not dropped) and then goes Armed.
- **Length:** `rec_length` seconds. Bars (`rec_length_sync` × the host bar length) apply only while `sync_mode` = Tempo **and** the host is playing; otherwise seconds. The cap is the capture buffer `F_max(fs)` (30 s at fs, sized in `prepareToPlay`). Bar length is latched at the start sample.
- **Next Note:** the first note-on after arming. It also plays normally. In Mono / Legato, a note event that mints a new serial or starts a cloud counts. A Legato pitch-only change (same serial) does not count.
- **Signal presence (D6):** the running peak of the post-gain sidechain over the take. A take with peak < **−90 dBFS** is **discarded** and the slot is unchanged. The UI gets the reason code "No sidechain signal". Threshold never fires on silence by construction.
- **No input bus** (`getBus(true,0)` null, disabled or 0 ch): `rec_arm` is ignored and the state stays Idle with the reason "Sidechain unavailable". The sidechain is **never** passed to the output (the buffer is cleared). This holds in every state, including VST3 aux routing.
- **Commit path (ARCH §22 / §6):** the audio thread sets `captureReady(frames)`. The message thread copies out, builds the tape (pristine = the capture at fs, no resample) through the same `MipTape::writeRun` path, and publishes it to `rec_target` via `SlotHandoff` (new-source rule: the stream's live grains get the 5 ms kill fade; new spawns read the new tape). The worker writes a 32-bit float WAV to `userMusicDirectory/Ouaricon/O-GranulatorInstrument/Recordings/` with a unique timestamped name. The slot `source` = `rec:<abs path>`. The audio thread never takes a lock and never allocates.
- **Recording into a playing slot** is allowed. The slot keeps playing its old tape until the swap. The 2.4 feedback head on that slot is parked across the swap, as in every swap.
- **`<Slots>` schema (ARCH State Persistence):** per slot `source` (`factory:<id>` | `file:<abs>` | `rec:<abs>` | empty), `name`, `fileSize`, `contentHash`, `embed`, `embedded`, `trimmed`. The working copy (feedback-written tape) is **not** saved; on reload it restarts from pristine. `rootNoteHint` is not written in v1.
- **contentHash** = xxHash64 of the first 1 MB of decoded PCM (ARCH). JUCE has no xxHash, so a header-only port (or a documented swap to an equivalent 64-bit hash) is a research item. The hash must be defined on the 24-bit-quantised PCM so a blob-loaded and a file-loaded source agree.
- **Resolve order on load:** absolute path (exists **and** size matches) → embedded blob → filename + hash search in the Recordings folder → `factory:` → **missing**. A missing slot is empty, `SlotInfo.missing = true` (UI relink, Stage 3.2), and the plugin stays stable. The source string is **kept** in state, so a missing slot re-saves its reference unchanged (no silent loss on a save before relinking).
- **Relink** is a message-thread API (`requestRelink(slot, File)`). It loads the file and, if the hash matches, keeps the slot's other state. A hash mismatch still loads, and the result is flagged for the UI.
- **`setStateInformation`** (host thread) only parses and enqueues loader jobs. It never decodes inline and never touches audio-owned tapes. Restoring a `<Slots>` with an empty source clears that slot through the same publish path.
- **Embedding and FLAC encode** run on the worker. `getStateInformation` never encodes: it emits the cached blob (D36), or omits the blob if encoding has not finished. A save that races a running encode records `embed` without the blob, and the next save carries it.
- **Telemetry (engine side only; UI is Stage 3):** record state + reason code, capture progress (frames / target), sidechain peak (max since last read), output peak, per-slot record-head position and overview deltas, swap / reset state, per-voice note / amp / scan centres, and the newest-voice destination and source values. These are atomics or triple buffers with no locks. The 2.1 grain snapshot is extended as needed.
- **True bypass:** with no `<Slots>` content and Record-in idle, renders-2.4 + the legacy renders stay **byte-identical**. Harness 300/0, `--parity` 168/0 + GV 2/0 carried, and the param-dump stays byte-identical (no new params). Fork drift 0/13 (new code goes in Gi files).
- **PERF-01 allocation gate** spans: arm → record → commit → swap, discard, cancel, state save / load (incl. embedded), missing-file resolve, factory load, and an SR change during a record.
- **Host-idle bound** (ARCH §6): with the audio thread idled mid-commit and mid-swap, pending ≤ 1 tape per slot and the retire ring drains when audio resumes. There is no unbounded growth.
- **Click gates (QUAL-01):** local κ (2.4 S6 precedent) on the record-commit swap into a sounding slot and on a state-load swap, each against a same-motion control and a hard control that must fire.
- **Block-size null:** the capture start / stop sample and the captured PCM are bitwise identical at 32 / 441 / 512 / 4096 / variable block sizes for all three triggers. The pre-roll is included.
- **CPU:** no new verdict (capture is a copy). C25m ≤ 40 % and C25tm ≤ 12 % are re-run as regression rows, with an INFO row for C25m while Recording.
- **HANDS-ON-2.6** is written (capture in Logic AU with and without routing, Ableton / Reaper VST3 sidechain, threshold feel, relink after moving a file, reopen a session) and **deferred to post-Stage-3**, with 2.1–2.5b deferred too (user override 2026-11-08).
- Rebuild the Standalone explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check.

## Test criteria (Phase 2.6 gate)

- [ ] Capture into each target slot by each trigger. The captured slot plays polyphonically and its PCM equals the injected sidechain × gain over the expected sample span (FUNC-07)
- [ ] Threshold: the start is 10 ms before the crossing, clipped at the arm sample. Manual / Next Note are sample-exact (D35)
- [ ] One-shot: arm still On after a commit stays Idle. A new Off→On edge captures again. Off while Armed cancels; Off while Recording commits a take of ≥ 0.5 s and discards a shorter one (D34)
- [ ] Bars length under Tempo + playing, seconds otherwise. The cap holds at `F_max`
- [ ] Unrouted (silent) sidechain → the take is discarded and the slot is bitwise unchanged. With no bus the arm is inert. The output **never** contains sidechain audio in any state (QUAL-02 engine path)
- [ ] Save / reopen restores 4 sources (file, rec, factory, embedded), loop regions and the scale state. A missing file gives `missing` + relink, keeps its reference on re-save, and the plugin stays stable (FUNC-06)
- [ ] The embedded blob decodes to the 24-bit-quantised pristine bit for bit, and save → load → save is byte-identical (D36). A blob over 16 MB is omitted and the slot falls back down the resolve order
- [ ] Every resolve-order rung is exercised, including the Recordings-folder hash search and a hash-mismatch relink
- [ ] `factory:` stubs load with **no disk access** (D33)
- [ ] Allocation gate across commit, discard, swap, state save / load, factory and missing resolves, and an SR change mid-record. The host-idle bound holds
- [ ] Click gates vs same-motion controls. Block-size null on capture spans + PCM, with a different-seed control
- [ ] True bypass byte-identical; harness 300/0, parity 168/0 + GV 2/0 carried; param-dump identical; drift 0/13
- [ ] C25m ≤ 40 %, C25tm ≤ 12 % (regression)
- [ ] auval PASS; pluginval 10 VST3 + AU SUCCESS, with SC active (2 ch) and inactive (0 ch)

## Constraints and risks to flag

- **The sidechain must never reach the output.** This is a stability guarantee (QUAL-02). Every state, layout, and the VST3 aux-routing case needs a bitwise-zero leak gate with an injected loud sidechain.
- **AU unrouted = silence, not absence** (memory `critical_au_hands_unrouted_sidechain_silence`). Bus presence is only a prerequisite; the −90 dBFS presence check decides.
- **`setStateInformation` / `prepareToPlay` on host threads.** Every `SlotInfo` write locks, and nothing publishes a bare pointer (memory `pattern_texture_forge_swap_needs_lock_if_prepare_publishes`).
- **getLargeIntValue clamps uint64:** store `contentHash` as hex or int64 (memory `critical_juce_getlargeintvalue_clamps_unsigned_64`).
- **ValueTree XML round trip loses types** (memory `critical_valuetree_xml_roundtrip_loses_type`). `embed` and `trimmed` come back as strings, so parse them explicitly.
- **juce_add_binary_data traps:** hyphens are stripped from resource names, and two targets in the default `BinaryData` namespace collide at link time.
- **The capture buffer is up to 30 s × fs × 2 ch** (≈ 46 MB at 192 k). It is allocated in `prepareToPlay` only, and a resize while Committing must wait for the copy-out.
- **`File::replaceWithText` defaults to CRLF** (memory). Any byte-compared state dump must write "\n".

## Open questions for research

1. xxHash64: is there a header-only port with a licence compatible with AGPL, or should we define our own 64-bit hash? It must hash the 24-bit-quantised PCM.
2. JUCE `FlacAudioFormat` writer at 24-bit: is it deterministic (same bytes for the same PCM across runs and platforms)? Do we need memory writer / reader paths for embed and for factory loads with no disk?
3. Where the capture sits in `processBlock` relative to the bus clear, MIDI cursor (Next Note offset) and grid ticks, and how the arm edge is sample-stamped from a block-rate parameter (first sample of the block vs smoothed).
4. The commit hand-off: atomic `captureReady` + message-thread polling (timer vs `AsyncUpdater`; memory: AsyncUpdater offline gap). Offline / non-realtime renders must still commit.
5. Recordings-folder search: a scan bound (file count, depth) and when it runs (worker, at load).
6. SR change while Armed / Recording: abort the take (and with what reason) or finish at the old rate.
7. Telemetry layout: which feeds exist already (`UiTelemetry.h` fork) and which are new. Is there enough triple-buffer capacity?
8. The generator for the 2 factory stubs: Python or a harness mode? It needs a pinned output hash.

## Out of scope for 2.6

- The curated factory source set and factory presets (FUNC-08, Stage 4)
- Relink / load / record UI, drag-drop wiring, and the lanes display (Stage 3.2)
- Record-in as a mod source (v1 out of scope)
- Embedding the working copy (feedback-written tape)
- DAW sidechain verification across hosts (HANDS-ON-2.6, then Stage 4)

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 2-dsp`)
