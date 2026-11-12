# O-GranulatorInstrument — HANDS-ON-2.6 (human gate)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.6 engine (Record-in capture by Manual / Threshold / Next Note, `<Slots>` state with file / rec / factory / embedded sources, relink, the 2 factory stubs), with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** Logic (AU `aumu OuGI OuDv`), Ableton Live and Reaper (VST3), and the Standalone where a row says so.
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred with HANDS-ON-2.1 / 2.2 / 2.3 / 2.4 / 2.5a / 2.5b.** On 2026-11-08 the user deferred all hands-on testing until the Stage 3 UI is in place. Run this sheet together with the earlier ones then. Until Stage 3 there is no record button, relink dialog or embed toggle: drive Record-in through the generic editor (Record Target / Arm / Trigger / Threshold / Length / Length Sync / Gain) and host automation.

Defaults to know before you start:
- **Arm is one-shot.** Arm Off→On arms once. After the take (kept or discarded) the recorder is idle even though Arm still shows On; turn it Off and On again for another take. Arm Off while waiting cancels; Arm Off while recording stops (≥ 0.5 s is kept, shorter is discarded).
- **Triggers.** Manual starts at the arm. Threshold starts 10 ms before the first sample above Record Threshold (so the transient is kept). Next Note starts on the next note that starts a new voice (a Legato pitch change does not).
- **Length.** Record Length in seconds; Record Length Sync (bars) only with Sync Mode = Tempo **and** the transport playing. 30 s at most (15 s at 192 kHz).
- **Silence is discarded.** A take whose level stays below −90 dBFS is dropped and the slot is unchanged ("No sidechain signal"). With no sidechain bus the arm does nothing ("Sidechain unavailable").
- **The sidechain never reaches the output** — not while recording, not with the plugin bypassed (the bypass leak is fixed in 2.6).
- **Where takes go:** a 32-bit float WAV in `~/Music/Ouaricon/O-GranulatorInstrument/Recordings/GI-Rec-<date>-<time>-S<n>.wav`. The session also embeds it (24-bit FLAC) unless it is over 16 MB.
- **Hot takes:** a take over 0 dBFS (Record Gain up to +12 dB) is kept unclipped in the WAV; the embedded copy is clipped at 0 dBFS (PD3).

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Logic AU capture | Logic: route a vocal / drum track to the plugin's sidechain. For each trigger (Manual, Threshold −30 dB, Next Note) arm into Slot 1–4 and capture 4 s. Play a chord on the recorded slot | Each take lands in its slot (the slot plays the take polyphonically). A WAV appears in the Recordings folder per take | | | |
| 2 | Logic AU unrouted | Same instrument, sidechain menu set to None. Arm (Manual) | Nothing is captured ("No sidechain signal"), the slot is unchanged, nothing of the sidechain is audible | | | |
| 3 | VST3 sidechain | Ableton Live and Reaper: route audio into the VST3 sidechain (aux) input; capture once. Then bypass the plugin with audio still routed | The take lands. With the plugin bypassed, no sidechain audio bleeds into the output | | | |
| 4 | Threshold feel | Threshold at −40 and −20 dB on drums and on voice | The take starts just before the hit / word; the attack is not cut (10 ms pre-roll) | | | |
| 5 | Next Note | Next Note trigger: play a note to start; then repeat in Legato with overlapping notes | The take starts with the played note. In Legato, a pitch change on a held key does not start it | | | |
| 6 | Bars | Sync Mode Tempo, Length Sync 2 bars, transport playing; then stopped | Playing: exactly 2 bars at the session tempo. Stopped: Record Length seconds | | | |
| 7 | Save / reopen | A session with Slot 1 = a file, Slot 2 = a recording, Slot 3 = factory (factory sources are reachable through factory presets, whose lanes load `factory:<id>` sources; e.g. Pads · Fifth Bloom puts Drone on lane III) *(rewritten at Stage 4)*, Slot 4 = a recording whose WAV you then delete. Save, close, reopen | All slots come back and sound; Slot 4 comes back from the embedded copy. Loop regions and the custom scale come back too | | | |
| 8 | Moved / deleted files | Move a recording WAV to another folder inside Recordings (or rename it); reopen. Then delete a user file a slot uses; reopen | The moved / renamed recording is found again. The deleted user file leaves its slot empty ("missing"), the plugin stays stable, and saving keeps the reference | | | |
| 9 | Hot take | Record Gain +12 dB on a loud source; reload the session with the WAV deleted | Audible clipping only on the blob-only reload (the WAV master is unclipped) — PD3, by design | | | |
| 10 | Bypass mid-take | Logic: bypass the plugin while recording, then unbypass | A take ≥ 0.5 s is kept up to the bypass; if Logic stops rendering while bypassed, the take has a gap at that point (PD10) | | | |
| 11 | Save time | Logic: 4 slots of ~11 MB embedded takes (30 s stereo at 48 k); save the project a few times | Saves stay usable (note the time); no hang | | | |
| 12 | Old patches | N/A *(rewritten at Stage 4)*: no pre-1.0 sessions exist | Harness-covered (TB7 / state rows) | | | |
| 13 | Swap dip (S6) | Hold a chord on a sounding slot; commit a take into that slot, and separately reopen a state that changes its file | No click. A brief level dip (old grains fade in 5 ms, new grains fade in through their windows) is the 2.1 swap rule; note whether it is noticeable (CK6a/b judged on raw curvature by user decision 2026-11-09; local κ 3.6× / 3.8× is the dip) | | | |

Listening notes: capture timing (rows 1, 4, 5): ______ · threshold feel (row 4): ______ · relink behaviour (row 8): ______ · save time (row 11): ______ · swap dip (row 13): ______

Result: ______ (all ✅ → Phase 2.6 hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
