# Stage 3: GUI — Phase 3.2 (Waveform lanes, file I/O UI, Record-in UI) - Context

## Discussion Summary

**Date:** 2026-11-10
**Participants:** User, Claude
**Phase goal (ROADMAP):** UI-01 — per-stream waveform lanes with drag-drop, loop handles, scan playheads and grain dots; file I/O and Record-in from the UI.

## Starting state (observed)

- Phase 3.1 verified ⚠️ PARTIAL (`VERIFICATION-3.1.md`): every automated gate green; HANDS-ON-3.1 (16 rows) not run.
- The v1 page is live in the WebView at 1200 × 800: 261 parameters bound, 34 native functions registered, presets, slot actions (load / relink / reset / reset all / embed), value entry. `index.html` == `mockups/v1-ui.html` (byte-identical; `cmp` is a 3.2 entry check).
- Lanes show name, badges, loop handles and the **pristine** waveform (`getSlotOverview`, 656 peaks, refetched on `ovRev`).
- 3.1 placeholders that 3.2 fills:
  - `getTelemetry`: `rec.run` is always 0; no record head, no playheads, no grain dots; `dirty` is always false; `encodedBytes` is 0 (or 16 MiB + 1 when too large).
  - `clearSlot` and `saveRecording` refuse (`{ ok: false, error: "unavailable" }`).
  - No consuming feeds are read (`readUiFrame`, `takeScPeak` / `takeScInPeak`, `takeOutPeak`), by PD7.
- Drop on a lane works but loads from the drop session's temp dir (`ogranulatorinstrument-drop-*`, reaped after about 5 min); a session saved afterwards reopens with that slot missing unless embedded.
- A committed take is already a WAV on disk: `~/Music/Ouaricon/O-GranulatorInstrument/Recordings/GI-Rec-<stamp>-S<n>.wav`, referenced as `rec:<path>` (`SlotLoader.cpp:834, 997`).
- Engine side already present: `GiTelemetry` (`headPos[4]`, `GiUiFrame`, `scInPeak`, `outPeak`), `getRecordState` (`state | reason << 8 | heldArm << 16`), `GiRecorder::takeScPeak` (post-gain), live overview. `SlotLoader` has no `requestClear` (clear exists only inside `restoreSlots` through `handoff.publishClear`).

## Requirements Confirmed

- **UI-01** closes in this phase: lanes with drag-drop, loop handles, scan playhead per voice, grain dots by stream.
- Loop handles, playheads, record head and grain dots track the engine through the existing ~30 Hz `getTelemetry` pull. No `juce::Timer` push, no `emitEvent`.
- Drag-drop and the folder button each load every slot on macOS (WKWebView path).
- Relink restores a missing file from the UI.
- Record-in reads "No sidechain" without a bus and "no signal" with a silent one; Armed / Recording / Committing / Done / Discarded show from engine state.
- The evolving overview: the lane redraws while recording and while feedback rewrites the tape; `dirty[656]` marks rewritten regions.
- `clearSlot` empties a slot and is refused while that slot is recording (spec). No confirmation dialog: a take is already on disk, and Save (D46) is the way to keep an evolved tape.
- Stage 2 audio must not move: harness 374 / 0 carried rows, parity 168 / 0 + GV 2 / 0, renders-2.4 + legacy byte-identical, param-dump identical, fork drift 0 / 13, X2 0.
- The five UI gates stay green on the real tree; new live `[data-i18n]` nodes get states in `tests/i18n-states.json` (D42 carried): Record-in run states, lane toasts, trimmed / missing / relink badges.

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D43 | HANDS-ON-3.1 (16 rows) | **Folded into 3.2 verify.** Run once together with HANDS-ON-3.2 on the finished lanes. A ❌ in a 3.1 row routes to `/plugin-execute O-GranulatorInstrument 3-gui` for 3.1's code. 3.1 stays verify-partial until then. HANDS-ON-2.1–2.6 still run at 3.3 verify (D39). | One session on a UI where lanes are observable. Accepted risk: 3.2 builds on host plumbing (DAW automation both ways, the real WKWebView console) that no automation has exercised. |
| D44 | Durable drops | **Copy to an Imports folder.** On drop commit the file is copied to `~/Music/Ouaricon/O-GranulatorInstrument/Imports/`, named by content hash so a repeat drop reuses one copy, and the slot references it as an ordinary `file:` source. Embed stays the user's choice. No automatic cleanup of the folder. | WKWebView does not expose the original path. A plain `file:` source reuses the whole 2.6 persistence / relink ladder with no new scheme. |
| D45 | Preset slots | **Empty entries leave lanes alone.** A preset replaces only the lanes it has a source for; a lane the preset saved as empty keeps what is loaded (including a feedback-written tape). Absent `slots` key still leaves all lanes alone (PD1 / UPM6b). Amends the 3.1 behaviour flagged in VERIFICATION-3.1; a preset can no longer guarantee an empty lane. | Loading a parameter-only preset must not wipe loaded lanes or an unsaved evolved tape. |
| D46 | Save recording | **Writes the lane as it sounds now:** the working copy, feedback writes included, as 24-bit WAV at the tape rate, to a file the user chooses. The button stays where v1 puts it (`rec:` lanes only). The pristine take is already in Recordings. | The only way to keep an evolved tape. Departs from the spec wording "writes a `rec:` slot" in content only; the LOCKED spec is not edited. |

### Defaults applied (not asked)

| Topic | Default | Note |
|-------|---------|------|
| Design latitude | D40 carries: v1 locked, defects only. No new controls on the page. | Playheads, grain dots, record head and the Record-in panel render as v1 draws them. |
| `rec.run` mapping | RESEARCH-3.1 §3: Armed / Recording / Committing → 1 / 2 / 3; Idle + `Committed` → 4 (done); Idle + `NoSignal` / `TooShort` → 5 (discarded); Idle + anything else → 0. Reconciles SUMMARY-2.6 dev. 13. | Hold time for 4 / 5 is a research item (proposed: until the next arm or about 4 s, whichever is first). |
| Sidechain peak for "signal" | Post-gain `takeScPeak` (what would go to tape; the level the Threshold trigger sees). Resolves SUMMARY-2.6 dev. 12 for Record-in; the 3.3 meter uses the same feed unless research finds a reason not to. | One consumer per consuming feed. |
| Windows | Drop / browser on Windows is checked in Stage 4 (PLAN-3.1). 3.2 keeps the Windows block and the module's WebView2 path intact; ROADMAP's "and Windows" criterion is deferred, not closed. | No Windows machine in the loop for 3.2. |
| Scan-mode `aria-valuetext` | Fixed in 3.2 (re-run `updateChoiceKnobVisual` over `CHOICE_KNOB_IDS` after a language apply). A defect under D40. | Applied to `index.html` and `v1-ui.html` alike. |
| `output_gain` default 7.15e-07 dB | Left as is. Fixing the range moves the param-dump (LOCKED contract). | Revisit Stage 4. |
| `savePresetWithDialog` sub-folder quirk | Left as is (parent pattern, low impact). | |
| `grain_limit` "heavy on CPU" hint at 256 | 3.3 scope. | |
| Version | 0.1.0 unchanged; no tag. | |

## Constraints Identified

- `getTelemetry` stays one ~30 Hz pull, one reader per consuming feed (`readUiFrame`, `takeScPeak`). The editor never touches the pool, voices or tapes directly; new processor accessors are message-thread, lock-free readers or the loader lock.
- `clearSlot` goes through the loader on the same audio-owned handoff as a load (`SlotLoader::requestClear` over `handoff.publishClear`); never a direct write from the message thread.
- Save (D46) reads the working copy off the audio thread without tearing a tape that feedback is writing: copy through the loader / handoff, write on a background thread; the native function completes on every path (busy / cancelled / error).
- D44 copy runs off the message thread or is bounded; a failed copy (disk full, no permission) refuses the drop with a toast, never leaves a temp-path source. Non-ASCII paths (`String::fromUTF8`, never `String (const char*)`).
- D45 changes `restoreSlots` behaviour for presets only; session restore keeps clearing sourced slots that the state lists as empty (SUMMARY-2.6 dev. 8). UPM6 / UPM6b rows are amended, not deleted; a negative control must show an empty preset entry no longer clears.
- No parameter writes from `parameterChanged` listeners; loop handles set each parameter directly (already so in 3.1).
- Canvas text waits on `document.fonts.load`; grain dots / playheads are canvas and gate-blind for AA.
- WebView is never hidden (completions are dropped while hidden); every new promise path completes.
- Recordings / Imports guards in the harness: tests never write to the real `~/Music/Ouaricon/O-GranulatorInstrument/` (override dir with a non-ASCII component, as PD17 of 2.6).
- Test hooks stay out of shipping binaries (X2 0).
- Module JS under `Source/ui/public/modules/` is a gitignored build artefact. Path-scoped commits on `main`; cache clear + dual-variant sweep on every install.
- pluginval 10 VST3 + AU and auval re-run with the sidechain on and off.

## Open Questions (for research)

- `GiUiFrame` contents vs what the page's lane painter reads (`index.html:4284` voice rows `[state, note, amp, scan1..scan4, wantSt]`; grain dots by stream): exact JSON shape, caps (grains per frame, 16 voices), and whether playheads belong to 3.2 with voices' other fields left empty until 3.3.
- Live overview: where the working-copy peaks and `dirty[]` come from (existing live overview in `GiTelemetry` vs a loader fold), refetch cadence (≤ 10 Hz per spec) and what moves `ovRev` during recording / feedback.
- Record head: `headPos[4]` → lane x; behaviour at the loop wrap and during the swap.
- `rec.run` 4 / 5 hold: is the engine's reason sticky until the next arm, or does the editor need a timestamp? `rec.pos` / progress source for the Recording bar (`index.html:3943-3952`).
- D44: does `webview-drop-streaming` 1.0.0 hand over a committed file that can be moved (rename) rather than copied; hash to name (reuse `GiHash` XXH64 `contentHash`); extension kept; collision / partial-copy handling; what the 2.6 relink ladder does when the Imports copy is later deleted.
- D46: the cheapest tear-free read of a working copy (snapshot through the handoff vs pause feedback writes); WAV writer bit depth / channel count; default folder for the chooser.
- `SlotLoader::requestClear`: interaction with an in-flight load, a recording on another slot, and `slotRev` / `ovRev`.
- Exact `encodedBytes` for the Embed tool's 16 MB comparison (cost of computing it; cache per `contentHash`).
- How the parent (O-Granulator 3.2) proved lanes headless (stub telemetry frames, probe rows) and which of its gates port.
- Which `[data-i18n]` nodes become live in 3.2 and the states needed to measure them in en / fr / zh-Hans.

## Carried items (not 3.2 scope)

- MOD sources, mono badges, mod rings, voice strip, meters, `grain_limit` heavy hint (3.3).
- HANDS-ON-2.1–2.6 and the Logic AU DAW pass (3.3 verify, D39).
- Blobs in presets (PD1 revisit), factory sources + presets (FUNC-08), Windows drop / browser check, `output_gain` default, cross-host sidechain (QUAL-02), COMPAT-02 (Stage 4).

## Next Phase

Ready for: research phase (Phase 3.2)
