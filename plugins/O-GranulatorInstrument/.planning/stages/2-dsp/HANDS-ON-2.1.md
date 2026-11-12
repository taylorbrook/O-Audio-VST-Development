# O-GranulatorInstrument — HANDS-ON-2.1 (human gate)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.1 engine. The WebView UI is the editor (lanes accept drop and browse); the `DevLoaderEditor` no longer exists *(rewritten at Stage 4)*.
**Run in:** the Standalone **and** one DAW (Logic preferred; AU `aumu OuGI OuDv`).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred (2026-11-08).** The user overrode D14 and ran Phase 2.2 execute first. This sheet and HANDS-ON-2.2 run together once the Stage 3 UI is in place.

Defaults to know before you start:
- Only stream 1 plays, and it reads **slot 1**. Slots 2–4 load and report status, but nothing plays from them until 2.2.
- There is no spray, jitter, window morph, FX, feedback, reverb, modulation, glide or MPE yet. The sound will be plainer than the final instrument.
- Voice gain is the value measured against O-Granulator in 2.2 (HANDS-ON-2.2 row 6), no longer a provisional −6 dB *(rewritten at Stage 4)*.

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Load each lane | Drop a WAV/AIFF/FLAC on each of lanes I–IV, or use each lane's browse button *(rewritten at Stage 4)* | Each lane shows the file's name and waveform; a > 30 s file shows the trimmed badge | | | |
| 2 | Chord | Load a pitched sample into slot 1 (set Root to its pitch) and hold a 4-note chord | Four distinct pitched clouds | | | |
| 3 | Root / key tracking | Play the root key, then set Key Track to 0 and play several keys | Root = the file's pitch; with Key Track 0 every key sounds the same pitch | | | |
| 4 | Steals | Set Polyphony 4 and play 6+ distinct notes legato | Notes beyond 4 steal the oldest; no clicks | | | |
| 5 | ADSR | Sweep Attack, Decay, Sustain and Release with short and long settings | Audible and click-free; Velocity→Amp scales level with velocity | | | |
| 6 | Scan modes | Scan Rate 100 % then 0 %; switch Retrigger / Free-run / Ping-pong | 100 % moves through the file in real time; 0 % holds; Retrigger restarts per note, Free-run continues, Ping-pong reflects | | | |
| 7 | Loop region | Drag Loop Start past Loop End; make the region tiny | Plays the sorted region; never silent or stuck (10 ms minimum) | | | |
| 8 | Load while sounding | Hold notes and load a new file into slot 1 | Switches to the new file with a short fade; no click or dropout | | | |
| 9 | > 30 s file | Load a file longer than 30 s | Status line shows **trimmed**; plays the first 30 s | | | |
| 10 | Session rate change | Standalone: hold a note and change the audio device rate (e.g. 48 → 96 k) in Audio Settings | Pitch holds after the switch (short crossfade); no crash | | — | |
| 11 | Grain Limit | Hold a dense chord and lower Grain Limit 256 → 32 | Density thins quickly; no clicks or stuck grains | | | |
| 12 | **UAF row** | Click **Load slot N** to open the chooser, close the plugin window (DAW) or editor, then pick a file | No crash, and no load happens | | | |
| 13 | Session reopen | Save the project with slots loaded, close and reopen it | Slots come back loaded (slots restore from the session) *(rewritten at Stage 4)* | | | |

Result: ______ (all ✅ → Phase 2.1 hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
