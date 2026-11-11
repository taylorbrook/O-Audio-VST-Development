# O-GranulatorInstrument — HANDS-ON-3.2 (human gate)

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 3.2: live lanes (playheads, grain dots, feedback head, record head, evolving overview with dirty ticks), Record-in panel from engine state, Clear, Save recording, durable drops, preset slot rule.
**Run in:** the Standalone and Logic (AU `aumu OuGI OuDv`). Rebuild the Standalone first: `ninja -C build OuariconGranulatorInstrument_Standalone`.
**Run together with** the 16 rows of `HANDS-ON-3.1.md` (D43). A ❌ in a 3.1 row routes to `/plugin-execute O-GranulatorInstrument 3-gui` for 3.1's code.
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌. HANDS-ON-2.1–2.6 and the Logic DAW pass run once at 3.3 verify (D39).

## What automation already covers (do not repeat by hand)

Harness (processor, scripted clock): dots against the engine's own grains, counts against a pool census, playheads, feedback head, live overview + dirty, Record-in run states and the 4 s hold, Clear, the preset rule, imports (hash name, reuse, failure, superseded), Save (bit-exact, guards, timeout / busy / resume), 0 audio-thread allocations. Headless page: lane classes, every Record-in status, button → native wiring, both failure toasts, overview refetch, pixel rows for each canvas feature, label fit in en / fr / zh-Hans over the state file. **Not covered:** a real WKWebView drop, the native chooser, a host that idles the audio thread during Save, anything you judge by eye or ear.

## Rows

| # | Where | Do | Expect | ✅ / ❌ |
|---|---|---|---|---|
| 1 | STREAMS | Drop an audio file from Finder on each of the four lanes, one at a time | Each lane loads; the lane name is the dropped file's name (not a hash) | |
| 2 | Finder | Open `~/Music/Ouaricon/O-GranulatorInstrument/Imports/` after row 1; drop the same file again on another lane | One file per distinct drop, named `<16 hex>.<ext>`; the repeat adds no file | |
| 3 | Standalone or DAW | After a drop, save the session / project, quit, reopen | The lane comes back from Imports with the same name and waveform | |
| 4 | Finder + UI | Delete that file from Imports, reopen the session | The lane reads missing with the relink badge; Relink to the original file restores it | |
| 5 | STREAMS | Drop a file that cannot be decoded (e.g. a text file renamed `.wav`) on a loaded lane | The lane keeps what it had; a toast says the file could not be loaded | |
| 6 | Gear → Français, then 简体中文 | Repeat row 5 | The toast reads correctly and fits in both languages | |
| 7 | STREAMS | Load the factory tone or a file, hold several notes | One playhead per held voice on each enabled lane, moving inside the loop region; none on an empty lane | |
| 8 | STREAMS | While holding notes, sweep Stereo (pan spread) from 0 to 100 %, then Reverse from 0 to 100 % | Dots sit on the centre line at 0 % spread and fan out vertically as it rises; dots change to the reversed look as Reverse rises; dot size follows Size; dots fade in and out with the window | |
| 9 | STREAMS | Enable streams II–IV with tapes loaded | Dots appear on the lane of their own stream only (I on lane I … IV on lane IV) | |
| 10 | FX · FEEDBACK + STREAMS | Raise a lane's Feedback, hold a note | The ochre feedback head moves along the lane; the waveform redraws as the tape is rewritten; dirty ticks appear only where it changed; Reset on that lane is highlighted | |
| 11 | STREAMS | With row 10 running, turn FREEZE on | The head stops and draws dashed; with Feedback at 0 there is no head | |
| 12 | STREAMS | Press Reset on the evolved lane | Pristine waveform returns; ticks clear; Reset highlight goes | |
| 13 | Record-in, sidechain routed with signal | Manual: Arm | Armed → Recording (record head sweeps the target lane over the old content, tint on) → Committing → Done; the new waveform appears at commit, not during capture (sign-off); "Done" falls back after about 4 s | |
| 14 | Record-in | Threshold, then Next Note | Status reads waiting for level / waiting for a note until the trigger, then as row 13 | |
| 15 | Record-in, sidechain routed but silent | Manual: Arm, let it run | Discarded for about 4 s, then "no signal" | |
| 16 | Record-in, no sidechain bus (Standalone, or a track without one) | Look at the panel | "No sidechain"; Arm does nothing harmful | |
| 17 | Record-in, Tempo length | Set bars so the take exceeds the 30 s cap at the host tempo, not recording | The capped note shows; at a faster tempo or fewer bars it goes | |
| 18 | STREAMS | Press Clear on a loaded lane; then press Clear on the target lane during a take | First: the lane empties. Second: refused, the take completes | |
| 19 | A `rec:` lane | Press Save, choose a name, confirm (track live / notes or transport running) | A 24-bit stereo WAV at the chosen path; open it elsewhere: it is the lane as it sounds now, feedback writes included | |
| 20 | Logic, track idle (not selected / transport stopped so the instrument is not processing) | Press Save on a `rec:` lane | A toast says the recording could not be saved; with the track live the retry works | |
| 21 | A take recorded hot (Record-in Gain up, clipping) | Save | The file clips at 0 dBFS (24-bit integer, accepted at plan PD19); note what you hear | |
| 22 | A `file:` lane | Look for Save | No Save button (v1 puts it on `rec:` lanes only) | |
| 23 | Save chooser | Try to save over the take's own file in Recordings | Refused (toast); the take is untouched | |
| 24 | Preset bar | Load lanes I and II, let feedback evolve lane I, then load a parameter-only preset, then a preset that has a source for lane III only | Lanes I and II (and the evolved tape) stay; lane III loads (D45) | |
| 25 | VOICE / amp figure | Hold notes | Level dots appear on the amp figure (PD2 side effect) — acceptable or not? | |
| 26 | VOICE, Français | Hold a note, read the voice cell | "tenue" beside the voice number, no overlap (was "enfoncée", too wide — wording is yours to confirm) | |
| 27 | Standalone (Web Inspector) | Console + network while doing rows 1, 13, 19 | No error, no 404 | |

## Expected 3.2 placeholders (not defects)

Voice strip details (newest marker, envelope dots, `wantSt`, releasing state), MOD source dots and mod rings, mono badges, output meters, the modulated loop region on lanes, the `grain_limit` hint: all 3.3. Windows drop / browser check, blobs in presets, factory presets, `output_gain` default, a visible notice when a take's WAV could not be written: Stage 4.
