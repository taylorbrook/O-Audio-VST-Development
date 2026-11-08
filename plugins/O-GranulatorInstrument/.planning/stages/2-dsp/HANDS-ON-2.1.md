# O-GranulatorInstrument — HANDS-ON-2.1 (human gate)

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.1 engine with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** the Standalone **and** one DAW (Logic preferred; AU `aumu OuGI OuDv`).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred (2026-11-08).** The user overrode D14 and ran Phase 2.2 execute first. This sheet and HANDS-ON-2.2 run together once the Stage 3 UI is in place.

Defaults to know before you start:
- Only stream 1 plays, and it reads **slot 1**. Slots 2–4 load and report status, but nothing plays from them until 2.2.
- There is no spray, jitter, window morph, FX, feedback, reverb, modulation, glide or MPE yet. The sound will be plainer than the final instrument.
- Voice gain is −6 dB and provisional. It is re-tuned in the 2.2 listening pass.

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Load each slot | Click **Load slot 1…4** and pick a WAV/AIFF/FLAC in each | Each status line shows name, frames, native rate and the trimmed flag | | | |
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
| 13 | Session reopen | Save the project with slots loaded, close and reopen it | Slots come back **empty**, which is expected until 2.6 (`<Slots>` state) | | | |

Result: ______ (all ✅ → Phase 2.1 hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
