# O-GranulatorInstrument — HANDS-ON-SESSION-4 (one human session at Stage 4 verify, D51)

**Build:** 1.0.0 dev (`O-GranulatorInstrument-dev`), the WebView UI is the editor (lanes accept drop and browse), 64 factory presets, 8 factory sources.
**Replaces** every earlier sheet: `2-dsp/HANDS-ON-2.1 … 2.6`, `3-gui/HANDS-ON-3.1 … 3.3` and `3-gui/HANDS-ON-SESSION-3.3` (kept for history only). Each carried row keeps its source ID (`2.1-4` = HANDS-ON-2.1 row 4; `S3.3-L2` = SESSION-3.3 Logic row L2), restated here so the old sheets need not be open. Rows that were stale at Stage 4 use their rewritten wording.
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌. Run the passes in order: 0 → 1 → 2 → 3 → 4. Pass 5 is not run.

## What automation already covers (do not repeat by hand)

Harness (engine, state, presets PR0–PR6, D55 rows UWF1 / USV3, parity, renders, param-dump), validators (auval, pluginval 10 VST3 + AU with the sidechain on and off), headless UI (bindings, value entry, lanes, MOD, toast chain TC1 / TW1, label fit en / fr / zh-Hans, preset-name fit PN1), `--cpu4` per-preset CPU verdict. **Not covered:** a real WKWebView, Finder drops, the native choosers, real hosts, anything judged by ear or eye.

---

## 0. Setup

```bash
cd ~/Dev/VST-development
./scripts/build-and-install.sh O-GranulatorInstrument      # cache clear + dual-variant sweep + install
ninja -C build OuariconGranulatorInstrument_Standalone
auval -a | grep -i granulatorinstrument                      # expect: aumu OuGI OuDv … O-GranulatorInstrument-dev
rm -rf ~/Library/O-GranulatorInstrument/Presets/Factory      # removes the factory .json files + .factory-version + .bank-hash
```

**Materials:** a sine, a voice recording, a drum loop, a left-only signal (as files and as audio tracks in Logic / Live); a 44.1 k file; a file longer than 30 s; a fake `.wav` (a text file renamed `.wav`; make two); a MIDI keyboard with sustain pedal, mod wheel and aftertouch.

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| 0-1 | Quit Logic **and** Live completely **before** installing | Neither is running during the install | | |
| 0-2 | Run `build-and-install.sh` (above) | rc 0; VST3 + AU installed as `O-GranulatorInstrument-dev` | | |
| 0-3 | Build the Standalone (above) | The `.app` is rebuilt (newer than the install) | | |
| 0-4 | `auval -a \| grep -i granulatorinstrument` | One line: `aumu OuGI OuDv … O-GranulatorInstrument-dev` | | |
| 0-5 | Delete `~/Library/O-GranulatorInstrument/Presets/Factory` (incl. `.factory-version` and `.bank-hash`) **before the first launch** | The folder is gone | | |
| 0-6 | Launch the Standalone once; `ls ~/Library/O-GranulatorInstrument/Presets/Factory \| wc -l` and `ls -a` | 64 `.json` files plus `.factory-version` and `.bank-hash` | | |

If the plugin looks stale in a host: quit the host, rerun the cache-clear lines from the root `CLAUDE.md`, reopen. Redo 0-5 / 0-6 after **any** preset or source change (a fix wave at an unchanged 1.0.0).

---

## 1. Standalone pass (engine + UI, no host)

Defaults to know: lanes I–IV are slots 1–4 and stream N reads lane N; only stream I is enabled at default. At default FX are off, Feedback 0 %, Freeze off, Reverb Mix 0 %, the matrix is empty, Voice Mode Poly, Glide 0, MPE off. **Live · Init** resets every parameter and the scale and keeps your lanes — use it to return to defaults between rows.

**Rows that need a take** (D55-1, the take part of 3.2-18, 3.2-19, 3.2-21, 3.2-23): if the Standalone's Record-in shows "No sidechain", run them in Pass 3 right after S3.3-L2 and mark them here.

### 1a. Engine (2.1–2.5b)

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| 2.1-1 | Drop a WAV / AIFF / FLAC on each of lanes I–IV, or use each lane's browse button ("Drop a file") *(rewritten at Stage 4)* | Each lane shows the file's name and waveform; a > 30 s file shows the trimmed badge | | |
| 2.1-2 | Pitched sample on lane I, set Root to its pitch, hold a 4-note chord | Four distinct pitched clouds | | |
| 2.1-3 | Play the root key; then Key Track 0 and play several keys | Root = the file's pitch; with Key Track 0 every key sounds the same pitch | | |
| 2.1-4 | Polyphony 4, play 6+ distinct notes legato | Notes beyond 4 steal the oldest; no clicks | | |
| 2.1-5 | Sweep Attack / Decay / Sustain / Release, short and long; vary velocity | Audible, click-free; Velocity→Amp scales level | | |
| 2.1-6 | Scan Rate 100 % then 0 %; Retrigger / Free-run / Ping-pong | 100 % moves in real time; 0 % holds; Retrigger restarts per note, Free-run continues, Ping-pong reflects | | |
| 2.1-7 | Drag the Loop Start handle past Loop End; make the region tiny | Plays the sorted region; never silent or stuck (10 ms minimum) | | |
| 2.1-8 | Hold notes, drop a new file on lane I | Switches with a short fade; no click or dropout | | |
| 2.1-9 | Load the > 30 s file | Trimmed badge; plays the first 30 s | | |
| 2.1-10 | Hold a note, change the device rate in Audio Settings (48 → 96 k) | Pitch holds after the switch (short crossfade); no crash | | |
| 2.1-11 | Dense chord, Grain Limit 256 → 32 | Density thins quickly; no clicks or stuck grains | | |
| 2.2-1 | Four files on lanes I–IV, enable streams II–IV, hold a chord | Each stream plays its own lane; all four audible | | |
| 2.2-2 | Toggle each stream's Enable / Mute / Solo during a chord | Click-free; Solo is additive; Unmute is instant | | |
| 2.2-3 | Per stream: sweep Jitter, Spray, Stereo, Reverse, Gain Random | Each audible on its own stream | | |
| 2.2-4 | Sweep Window 0 → 1 and Window Skew −100 → +100 on a held note | Grain shape changes audibly; click-free | | |
| 2.2-5 | Pitch Random 12 st; Scale Major, a pentatonic, 19-EDO, then Off | Pitches snap to the scale; Off = continuous random pitch | | |
| 2.2-7 | 4 streams × dense chord, Grain Limit 256 → 32 | Thins quickly; no clicks or stuck grains | | |
| 2.2-8 | Polyphony 4, 6+ notes legato, 4 streams on | Steals are inaudible | | |
| 2.2-9 | Release 500 ms+, 4 streams, release a chord | Every stream keeps spawning through the release and fades out | | |
| 2.3-1 | Held note: Filter Type LP, BP, HP; sweep Cutoff and Res on each | Each type distinct; Res 100 % rings without blowing up | | |
| 2.3-2 | Filter Random 100 %, then 0 % | Colour varies per grain at 100 %; static at 0 % | | |
| 2.3-3 | Drive 0 → 100 % | Progressive saturation; Drive 0 = FX off | | |
| 2.3-4 | Crush 0 → 100 % with Crush Chance 100 / 50 / 0 % | S&H + bit reduction; at 50 % only some grains crush. **Note** where quiet material disappears (R22): ______ | | |
| 2.3-5 | Ring Freq 20 → 5000 Hz at several Ring Mix; then Ring Random 100 % | Follows freq and mix; at Random 100 % each grain has its own carrier | | |
| 2.3-6 | Res 100 + Drive 100 + Crush + Ring on all 4 streams, chord, Grain Limit 256 | No dropouts, no stuck / silent output. **Note** LP / HP resonance vs wet safety (R19): ______ | | |
| 2.3-7 | Every FX off on every stream | Sounds as with no FX | | |
| 2.4-1 | Chord; Feedback 0 → 100 % at Saturation 0, 20, 100 % | Copy evolves; each Saturation has its own character; no runaway. **Note** character: ______ | | |
| 2.4-2 | Feedback 80 %: sweep FB Low Cut, FB High Cut, then FB Diffusion 0 → 100 % | Written layer thins / darkens with the cuts; diffusion smears it | | |
| 2.4-3 | Feedback 80 %, toggle Freeze while playing | Heads stop with no click, copy holds; unfreeze resumes smoothly | | |
| 2.4-4 | Feedback 100 %, release all, wait 2 s, play again | No splice or tick where the head stopped | | |
| 2.4-5 | Press a lane's Reset button, then Reset All, while playing and while frozen *(rewritten at Stage 4)* | Pristine sound back through a smooth 20 ms crossfade; frozen stays frozen | | |
| 2.4-6 | Feedback 80 %: one note, then a wide chord | Chords thin in the copy. **Listening call:** is the `1/sqrt(Σa)` chord fallback needed? ______ | | |
| 2.4-7 | Sweep Reverb Mix / Size / Damp; pull Mix to 0 while a tail rings; raise again after a pause | Smooth; tail fades with no step at 0; cold re-enable is clean | | |
| 2.5a-1 | LFO 1 → S1 Position, then Level, then Pitch, depth ±50; all 6 shapes at 0.5–8 Hz | Each shape as named; clean S&H steps, Smooth Random glides; no zipper. **Note** LFO feel: ______ | | |
| 2.5a-2 | LFO 1 Per-voice → S1 Pan on a held chord; switch Global ↔ Per-voice while holding | Each chord note moves on its own; mode switch has no click | | |
| 2.5a-3 | Velocity → S1 Size, Key → S1 Filter Cutoff (LP), Note Random → S1 Pan; chords at mixed velocities | Each note gets its own size / brightness / position | | |
| 2.5a-4 | Mod Env 1 → S1 Size, Mod Env 2 → S1 Filter Cutoff at +60; sweep A / D / S / R | Shapes follow the knobs; a mid-segment edit lands at the next stage | | |
| 2.5a-5 | Mod wheel, aftertouch, CC74, pitch bend each → S1 Level; then send CC121 | Each controller moves its route; CC121 returns them to rest | | |
| 2.5a-7 | Velocity → Reverb Mix +60; legato phrases at varied velocity, then release all | Reverb Mix follows the newest note smoothly and holds after release | | |
| 2.5a-8 | While a route sounds: change source, destination, depth, sign; set source / dest to None | No clicks; every change crossfades over 20 ms | | |
| 2.5a-11 | LFO 1 → Reverb Mix ±50 with Reverb Mix 0 | Tank restarts cold each time the mix hits 0 (R54). **Listening call:** acceptable? ______ | | |
| 2.5b-1 | Mono, Attack 200 ms, Sustain 50 %; overlapping phrases at mixed velocity | Re-attacks from the current level; no click, no dip | | |
| 2.5b-2 | Legato, Glide 150 ms; overlapping phrases | Only pitch moves; envelope, scan, LFOs keep running | | |
| 2.5b-3 | Mono and Legato: hold C, press E, release E | Glides back to C without a re-attack | | |
| 2.5b-4 | Glide 0 / 50 / 300 / 2000 ms in Mono, small and large intervals; then Poly + Glide 300 | Constant time for any interval; Glide 0 jumps cleanly; Poly starts at the last note's pitch | | |
| 2.5b-5 | Hold a chord in Poly, switch to Mono and back while holding | Newest voice keeps sounding, rest fade in 5 ms; no click, no stuck note | | |
| 2.5b-10 (CK5b, CK5c arms) | Sine source: Legato glide (CK5b), fallback glide (CK5c) | No clicks or zipper (listening check) | | |

### 1b. UI (3.1–3.3)

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| 3.1-1 | Open the editor | 1200 × 800, not resizable, not blank; STREAMS, FX · FEEDBACK, MOD, VOICE render with no clipped / overlapping text | | |
| 3.1-2 | Each page: drag a knob, a choice knob, a fader, a segment row, a `<select>`, a toggle; on STREAMS a loop handle and a root field | Readout and sound follow; double-click resets to default | | |
| 3.1-4 | Change knobs, a scale, a degree mask, load a file into lane I; save a preset; change everything / load another preset; load the saved one | Parameters, scale, masks and lane I's source come back (file reloads from its path) | | |
| 3.1-5 | OUTPUT −12 dB, Arm On, load any preset | OUTPUT stays −12 dB; Arm unchanged; no recording starts | | |
| 3.1-7 | Lane: load with the folder button; Relink to another file; Reset; Reset All; toggle Embed on a file lane | Name, badges, waveform after each action; Reset returns the pristine tape; Embed on a factory or empty lane is refused | | |
| 3.1-8 | Click a knob's NAME; type, Enter; again, Esc; again, click elsewhere | Name lights on hover; Enter applies, Esc cancels, click-away applies; out-of-range clamps; units optional | | |
| 3.1-9 | MOD, Sync = Tempo: click an LFO Rate name, type a division as the readout writes it | That division is selected; non-division text changes nothing | | |
| 3.1-10 | VOICE in Français: click a Scan name, type `Redéclench` | Mode selected; English names work too | | |
| 3.1-11 | With a field open, switch language in the gear menu | Field closes without writing | | |
| 3.1-12 | Look at the gear beside the source dock; open it | Reads well as placed; popover opens down-left, covers nothing needed | | |
| 3.1-13 | Français, then 简体中文, on all four pages | No clipped or overlapping label | | |
| 3.1-14 | macOS Reduce Motion on, reopen the editor | No animated transitions; still usable | | |
| 3.1-15 | Web Inspector: console + network, reload | No error, no 404 | | |
| 3.1-16 | STREAMS canvas text (window figures, lane text) | EB Garamond, not a fallback face | | |
| 3.2-1 | Drop a file from Finder on each of the four lanes | Each loads; lane name = the file's name (not a hash) | | |
| 3.2-2 | Open `~/Music/Ouaricon/O-GranulatorInstrument/Imports/`; drop the same file on another lane | One `<16 hex>.<ext>` file per distinct drop; the repeat adds none | | |
| 3.2-3 (Standalone arm) | After a drop, save the state (Options → Save, or quit and relaunch), reopen | Lane comes back from Imports with the same name and waveform | | |
| 3.2-4 (Standalone arm) | Delete that file from Imports, reopen | Lane reads missing with the relink badge; Relink to the original restores it | | |
| 3.2-5 | Drop the fake `.wav` on a loaded lane | Lane keeps what it had; toast: file could not be loaded | | |
| 3.2-6 | Français, then 简体中文: repeat 3.2-5 | Toast reads correctly and fits | | |
| 3.2-7 | Hold several notes on loaded lanes | One playhead per held voice per enabled lane, inside the loop; none on an empty lane | | |
| 3.2-8 | Holding notes: Stereo 0 → 100 %, then Reverse 0 → 100 % | Dots on the centre line at 0 %, fan out as it rises; reversed look as Reverse rises; size follows Size; dots fade with the window | | |
| 3.2-9 | Enable streams II–IV with tapes | Dots only on their own lane | | |
| 3.2-10 | Raise a lane's Feedback, hold a note | Ochre head moves; waveform redraws; dirty ticks only where it changed; Reset highlighted | | |
| 3.2-11 | During 3.2-10, Freeze on | Head stops and draws dashed; with Feedback 0 there is no head | | |
| 3.2-12 | Reset the evolved lane | Pristine waveform; ticks clear; highlight goes | | |
| 3.2-16 | Record-in with no sidechain bus | "No sidechain"; Arm does nothing harmful | | |
| 3.2-18 | Clear on a loaded lane; then Clear on the target lane during a take | Lane empties; during a take: refused, the take completes | | |
| 3.2-19 | On a `rec:` lane press Save, name it, confirm (notes playing) | 24-bit stereo WAV; opened elsewhere it is the lane as it sounds now, feedback writes included | | |
| 3.2-21 | A hot take (Record Gain up, clipping): Save | File clips at 0 dBFS (24-bit integer, accepted); note what you hear | | |
| 3.2-22 | A `file:` lane | No Save button | | |
| 3.2-23 | Save chooser: save over the take's own file in Recordings | Refused (toast); take untouched | | |
| 3.2-24 | Lanes I and II loaded, feedback evolving lane I; load a parameter-only preset (any **Live ·** preset); then a preset with a source for lane III only (save one first as a user preset) | Lanes I, II and the evolved tape stay; lane III loads (D45) | | |
| 3.2-25 | VOICE: hold notes, watch the amp figure | Level dots on the amp figure — acceptable? ______ | | |
| 3.2-27 | Web Inspector console + network during 3.2-1, a take, 3.2-19 | No error, no 404 | | |
| 3.3-1 | Drag each of the 17 MOD chips onto a knob | Matrix row with that source and destination at +40 %; knob gets a ring | | |
| 3.3-2 | Drag a few chips onto a loop start / end handle | Route on the loop destination; handle shows a tick | | |
| 3.3-3 | Global LFO → a knob and a loop end; no notes | Ring dot and loop-edge tick move; loop shading follows | | |
| 3.3-4 | Env 1 → stream I Level; hold, release | Ring dot follows the envelope, rests at the knob value after the note | | |
| 3.3-5 | Env 2 → Output; LFO 2 Per-voice → Reverb mix | Both show the mono badge; rings hold the last value after the last note (known limit) | | |
| 3.3-6 | Alt-drag a matrix row's depth | Smooth; ring travel follows | | |
| 3.3-7 | Polyphony 8, 10-note chord | 8 cells lit; each steal shows a stolen cell ~0.3 s, new note in its own cell | | |
| 3.3-8 | Polyphony 16, play past 16 | Steals read as a note change in the same cell (accepted limit) | | |
| 3.3-9 | Release notes; then sustain pedal and release keys | Release state until the tail ends; with pedal the cells stay held | | |
| 3.3-10 | Play several notes in turn | Key view and newest marker follow the last note | | |
| 3.3-11 | Hold a note, slow Env 1 / Env 2 / amp | Scope dots ride their curves stage by stage; read-outs show `newest`; idle with no note | | |
| 3.3-12 | LFO 1 Per-voice, Phase 90°, three notes at different times | One dot per voice on the curve (newest in ochre), on the line | | |
| 3.3-13 | A note > 48 st above the stream root | `+48` marker on that cell; that stream's dots in warn colour; Key Track 0 stream does not | | |
| 3.3-14 | Stream I Pan full left, Stereo 0, Reverb 0, hold a note | Output L moves, R rests | | |
| 3.3-16 | Close the editor while audio plays, reopen | First frame = max since last read, then normal ballistics (known limit) | | |
| 3.3-17 | Grain Limit 128, then 256 | Caption `n / 128`, `n / 256`; heavy hint off at 128, on at 256 | | |
| 3.3-18 | Français, then 简体中文: walk MOD and VOICE incl. a clamp cell and a stolen cell | Text correct and fits; zh-Hans clamp marker fits | | |

### 1c. New at Stage 4 (D55, D59)

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| D55-1 | `chmod a-w ~/Music/Ouaricon/O-GranulatorInstrument/Recordings` (create it first if absent); record a take; afterwards `chmod u+w` it back | Toast "Slot n: the take plays, but its WAV file could not be written"; the take still plays | | |
| D55-2 | Drop a fake `.wav` on lane I and the second one on lane II in one go (back to back, within a second) | Two toasts, one after the other in lane order (I, then II), not one | | |
| D55-3 | Fresh instance / Live · Init: read OUTPUT; move it and double-click to reset; read S1–S4 Level and Record Gain | OUTPUT reads exactly **0.0 dB** at default and after the reset; S1–S4 Level and Record Gain read 0.0 dB | | |
| 3.2-26 | VOICE in Français, hold a note, read the voice cell | "tenue" beside the voice number, no overlap. **Decide:** wording OK? ______ | | |

---

## 2. Preset audition pass (Standalone)

Runs before the DAW passes so they can use real presets. For each preset: load it from the preset bar, play a short phrase (a single note, a 3-note chord held ~3 s, a few legato notes across two octaves, release and hear the tail), mark ✅ / ❌. A ❌ is: silence, a click, a level jump far from the bank, a lane that does not load, or a sound that does not match the name.

- **Live ·** presets keep your lanes and set up Record-in: **load something into lanes I–IV first** (e.g. sine, voice, drums, a factory lane). Check the lanes stay and Record-in's trigger / target / length change as described; a take is optional.
- **Keys ·** presets: also check **no pitch wobble (realism)**.
- **Expressive ·** MPE presets (EX1–EX4) **need an MPE controller** → without one, check it loads and plays with normal MIDI.
- **Rhythmic ·** presets lock to the host tempo; if the Standalone's grid sounds off, re-hear one in Logic (Pass 3) before marking ❌.

### Drones

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| DR1 | Drones · Iron Sky | Load, phrase | | |
| DR2 | Drones · Deep Well | Load, phrase | | |
| DR3 | Drones · Organ Loft | Load, phrase | | |
| DR4 | Drones · Frozen Field | Load, phrase | | |
| DR5 | Drones · Hollow Wind | Load, phrase | | |
| DR6 | Drones · Low Orbit | Load, phrase | | |
| DR7 | Drones · Tape Halo | Load, phrase | | |
| DR8 | Drones · Night Engine | Load, phrase | | |

### Expressive

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| EX1 | Expressive · MPE Breath | MPE controller; else loads and plays with normal MIDI | | |
| EX2 | Expressive · Slide Glass | MPE controller; else loads and plays with normal MIDI | | |
| EX3 | Expressive · Press Vowel | MPE controller; else loads and plays with normal MIDI | | |
| EX4 | Expressive · Bend Choir | MPE controller; else loads and plays with normal MIDI | | |
| EX5 | Expressive · Mono Lead | Phrase; monophonic with a soft glide; mod wheel brightens | | |
| EX6 | Expressive · Glide Bass | Overlapping notes glide without re-attack | | |
| EX7 | Expressive · Wheel Morph | Phrase; move the mod wheel; vary velocity | | |
| EX8 | Expressive · Velo Shards | Phrase at mixed velocities | | |

### Keys

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| KY1 | Keys · Grain Piano | Phrase; no pitch wobble (realism) | | |
| KY2 | Keys · Celesta | Phrase; no pitch wobble (realism) | | |
| KY3 | Keys · Bell | Phrase; no pitch wobble (realism) | | |
| KY4 | Keys · Glass Mallet | Phrase; no pitch wobble (realism) | | |
| KY5 | Keys · Soft Rhodes | Phrase; no pitch wobble (realism) | | |
| KY6 | Keys · Kalimba | Phrase; no pitch wobble (realism) | | |
| KY7 | Keys · Harp Cascade | Phrase; no pitch wobble (realism) | | |
| KY8 | Keys · Toy Box | Phrase; no pitch wobble (realism) | | |

### Live (load lanes first; lanes must stay)

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| LI1 | Live · Init | Every parameter and the scale to default; lanes and takes stay | | |
| LI2 | Live · Manual Loop | Lanes stay; Record-in set up (Manual, 4 s, slot 1) | | |
| LI3 | Live · Threshold Catch | Lanes stay; Record-in set up (Threshold −36 dB, 2 s, slot 1) | | |
| LI4 | Live · Next Note Grab | Lanes stay; Record-in set up (Next Note, 3 s, slot 2) | | |
| LI5 | Live · Tempo Bars | Lanes stay; Record-in set up (Tempo, 2 bars, slot 1) | | |
| LI6 | Live · Feedback Loop | Lanes stay; Record-in to slot 2, feedback on | | |
| LI7 | Live · Freeze Cloud | Lanes stay; Record-in to slot 3 at +6 dB, Freeze | | |
| LI8 | Live · Four Lanes | Lanes stay; Record-in to slot 4 at −6 dB, all four streams on | | |

### Motion

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| MO1 | Motion · Orbit Lines | Load, phrase | | |
| MO2 | Motion · Reverse River | Load, phrase | | |
| MO3 | Motion · Loop Weaver | Load, phrase | | |
| MO4 | Motion · Ring Spinner | Load, phrase | | |
| MO5 | Motion · Crush Tides | Load, phrase | | |
| MO6 | Motion · Swarm | Load, phrase | | |
| MO7 | Motion · Phase Garden | Load, phrase | | |
| MO8 | Motion · Clockwork | Load, phrase | | |

### Pads

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| PA1 | Pads · Warm Choir | Load, phrase | | |
| PA2 | Pads · Fifth Bloom | Load, phrase | | |
| PA3 | Pads · Pentatonic Mist | Load, phrase | | |
| PA4 | Pads · Just Glow | Load, phrase | | |
| PA5 | Pads · Nineteen | Load, phrase | | |
| PA6 | Pads · Seventeen Glass | Load, phrase | | |
| PA7 | Pads · Slow Tide | Load, phrase | | |
| PA8 | Pads · Velvet Strings | Load, phrase | | |

### Rhythmic

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| RH1 | Rhythmic · Pulse Grid | Load, phrase; on the grid | | |
| RH2 | Rhythmic · Pluck Arp | Load, phrase; on the grid | | |
| RH3 | Rhythmic · Stutter Kit | Load, phrase; on the grid | | |
| RH4 | Rhythmic · Dotted Echo | Load, phrase; on the grid | | |
| RH5 | Rhythmic · Triplet Rain | Load, phrase; on the grid | | |
| RH6 | Rhythmic · Gated Pad | Load, phrase; on the grid | | |
| RH7 | Rhythmic · Broken Beat | Load, phrase; on the grid | | |
| RH8 | Rhythmic · Ratchet | Load, phrase; on the grid | | |

### Textures

| ID | Preset | Check | ✅ / ❌ | Notes |
|---|---|---|---|---|
| TX1 | Textures · Rain Glass | Load, phrase | | |
| TX2 | Textures · Static Choir | Load, phrase | | |
| TX3 | Textures · Sand Drift | Load, phrase | | |
| TX4 | Textures · Broken Radio | Load, phrase | | |
| TX5 | Textures · Ember Field | Load, phrase | | |
| TX6 | Textures · Wind Harp | Load, phrase | | |
| TX7 | Textures · Glitter Haze | Load, phrase | | |
| TX8 | Textures · Rust Cloud | Load, phrase | | |

### Preset proof rows

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| PP1 | Dirty state: twist many knobs, change the scale and a mask, add routes, drop a file on a lane; then load a factory preset | Sounds like a fresh load of that preset (compare with loading it again from Live · Init) | | |
| PP2 | Set OUTPUT to −7 dB; load presets from several categories | OUTPUT is never touched by a preset load | | |
| PP3 | Hold a chord and switch presets, including onto **Keys** presets | No click (the automated click gate covers sustaining targets only) | | |
| PP4 | Load presets with factory lanes (e.g. Pads · Fifth Bloom: Harmonic, Harmonic, Drone) | Lanes resolve (factory badge, waveform) and show their names; no missing / relink | | |
| PP5 | Edit a factory preset, save as a user preset, load another, reload it | Identical (parameters, lanes, sound) | | |
| PP6 | Save dialog: navigate into `…/Presets/Factory` and save there | Refused (save-failed toast); no file written or overwritten in Factory | | |
| PP7 | Realism-titled Keys: sustain a note on each | No pitch wobble (no vibrato, no random pitch) | | |

---

## 3. Logic AU pass

Software Instrument track with `O-GranulatorInstrument-dev` (AU `aumu OuGI OuDv`); audio tracks playing the sine, voice, drums and the left-only signal. Project at 48 kHz / 128 buffer unless a row says otherwise. Run the parked "needs a take" rows from Pass 1 right after S3.3-L2.

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| S3.3-L1 | Plugin header → Side Chain → an audio track | Record-in meter moves (SC L / R); "No signal" clears | | |
| S3.3-L2 | Record-in Manual: Arm into slot I, then Threshold into slot II, then Next Note into slot III | Each take lands in its slot (waveform at commit); statuses as 3.2-13 / 3.2-14 | | |
| S3.3-L3 | Side Chain → None, Arm | No signal / take discarded | | |
| S3.3-L4 | Save the project, quit Logic, reopen | All three takes come back with names and waveforms | | |
| S3.3-L5 | Stop the transport so the instrument idles, audio track muted | SC meter reads 0 (expected: Logic does not feed an idle instrument's side-chain) | | |
| S3.3-L6 | Automate one knob and play with an LFO routed (MOD) | Automation and rings move together; no clicks; CPU reasonable at Grain Limit 256 | | |
| 2.1-12 | **UAF:** click a lane's browse button to open the chooser, close the plugin window, then pick a file | No crash, no load | | |
| 2.1-13 | Save the project with lanes loaded, close, reopen *(rewritten at Stage 4)* | Lanes come back loaded (slots restore from the session) | | |
| 2.2-6 | **Voice gain (D16):** one note at defaults vs O-Granulator at **mix 100 %**, same file; then a 6-note chord | Single note matches in loudness; chord has no harsh knee or pumping. Trim: ______ dB (0.0 = confirmed) | | |
| 2.2-10 | Different `sN_scale` per stream, save, close, reopen *(rewritten at Stage 4)* | Scale choices come back; lanes come back too (slots restore from the session) | | |
| 2.3-8 | Worst case: 16 notes × 4 streams, BP Res 100 + Drive 100 + Crush 100 + Ring Mix 100 | No overloads (informational; verdict is automated) | | |
| 2.3-9 | Different FX per stream, save, close, reopen | Every FX setting comes back | | |
| 2.4-8 | Feedback 100 + Reverb Mix 100 + Res 100 all streams + 16 voices, OUTPUT 0 dB | Never above +6 dBFS on Logic's meter. **Note** knee character: ______ | | |
| 2.4-9 | Feedback on, note held, switch the project 44.1 ↔ 96 kHz | Pitch correct; heads resume after the rebuild (short pause expected) | | |
| 2.4-10 | Set Feedback, FB controls, Freeze, Reverb; save, close, reopen | Every parameter back; working copy reloads pristine (by design) | | |
| 2.5a-6 | Macro 1–4 → four destinations; sweep by hand and by automation | Smooth, no stepping on fast moves | | |
| 2.5a-9 | Sync Tempo; S1 Density Sync / Size Sync at 1/16, 1/8 T, 1 bar in a loop at 3 tempi; change tempo and jump the loop | Grains locked to the beat; note-on grain immediate; heavy chords thin but stay on grid. **Note** burst feel: ______ | | |
| 2.5a-10 | Sync Tempo, LFO 1 Rate Sync 1 bar → S1 Level; Global, then Per-voice | Global locked to the bar across loop jumps; Per-voice restarts per note at tempo rate | | |
| 2.5b-6 | Pedal down, play / release, re-strike a held note, pedal up; then stop the transport with the pedal down | Held notes ring to pedal-up; re-strike releases the old one with its tail; transport stop silences everything | | |
| 2.6-1 | Side-chain a vocal / drum track; each trigger (Manual, Threshold −30 dB, Next Note) into slots I–IV, 4 s; play a chord on the slot | Each take lands and plays; one WAV per take in Recordings | | |
| 2.6-2 | Side Chain None, Arm Manual | Nothing captured ("No sidechain signal"), lane unchanged, nothing of the sidechain audible | | |
| 2.6-4 | Threshold −40 and −20 dB on drums and voice | Take starts just before the hit / word (10 ms pre-roll). **Note** feel: ______ | | |
| 2.6-5 | Next Note: play a note to start; repeat in Legato with overlapping notes | Starts with the played note; a Legato pitch change does not start it | | |
| 2.6-6 | Sync Tempo, Length Sync 2 bars, transport playing; then stopped | Playing: exactly 2 bars; stopped: Record Length seconds | | |
| 2.6-7 | Load **Pads · Fifth Bloom** (lane III = factory Drone); then lane I = a file, lane II = a take, lane IV = a take whose WAV you then delete; save, close, reopen *(rewritten at Stage 4)* | All lanes come back and sound; lane III from the factory source; lane IV from the embedded copy; loops and custom scale back | | |
| 2.6-8 | Move / rename a recording WAV inside Recordings, reopen; then delete a user file a lane uses, reopen | Moved recording found again; deleted file → lane "missing", stable, reference kept on save | | |
| 2.6-9 | Record Gain +12 dB on a loud source; reload with the WAV deleted | Clipping only on the blob-only reload (PD3, by design) | | |
| 2.6-10 | Bypass the plugin while recording, then unbypass | A take ≥ 0.5 s kept up to the bypass; a gap if Logic stops rendering (PD10) | | |
| 2.6-11 | 4 lanes of ~11 MB embedded takes (30 s stereo at 48 k); save a few times | Saves usable, no hang. Time: ______ | | |
| 2.6-13 | Hold a chord; commit a take into the sounding slot; separately reopen a state that changes its file | No click; brief level dip is the swap rule — noticeable? ______ | | |
| 3.1-3 | Automation lanes for a slider (Stream I Size), a choice (Grain Limit), a toggle (Stream II enable); play back | UI control follows the lane; touching writes automation (one gesture per drag) | | |
| 3.1-6 | Load a preset, save the project, reopen; change one degree mask, save, reopen | Both reopens restore exactly what was saved | | |
| 3.2-3 (Logic arm) | After a drop, save the project, quit, reopen | Lane comes back from Imports, same name and waveform | | |
| 3.2-4 (Logic arm) | Delete that file from Imports, reopen | Lane missing with relink badge; Relink restores | | |
| 3.2-13 | Sidechain routed with signal; Manual: Arm | Armed → Recording (record head sweeps, tint on) → Committing → Done; waveform at commit; "Done" falls back after ~4 s | | |
| 3.2-14 | Threshold, then Next Note | Waiting for level / waiting for a note, then as 3.2-13 | | |
| 3.2-15 | Sidechain routed but silent; Manual: Arm, let it run | Discarded ~4 s, then "no signal" | | |
| 3.2-17 | Tempo length: set bars so the take exceeds the 30 s cap, not recording | Capped note shows; faster tempo / fewer bars → it goes | | |
| 3.2-20 | Track idle (not selected / transport stopped): Save on a `rec:` lane | Toast: could not be saved; with the track live the retry works | | |
| 3.3-15 | Side-chain the left-only signal | SC L moves, R rests; no side-chain → slashed / No signal | | |
| CPU-1 | 48 kHz / 128 buffer: load each of the 3 heaviest factory presets, hold a dense chord 20 s | Logic's CPU meter: no overloads, no "System Overload". Peaks: ______ | | |

Heaviest 3 factory presets for CPU-1 (`--cpu4` T, harness): **Drones · Hollow Wind** (8.64 %), **Motion · Reverse River** (8.10 %), **Pads · Fifth Bloom** (7.93 %). Logic’s meter at 48 kHz / 128 should sit well under the 12 % typical budget per instance.

---

## 4. Live VST3 pass

Ableton Live: a MIDI track with `O-GranulatorInstrument-dev` (VST3; rescan plugins if missing); audio tracks with the drums and voice. **That Live offers a sidechain selector for a VST3 instrument's aux bus is unverified** — if it does not, LV1 is ❌-by-host: record it, do not fix it.

| ID | Do | Expect | ✅ / ❌ | Notes |
|---|---|---|---|---|
| 2.6-3 (Live arm) | Route audio into the VST3 sidechain (aux); capture once. Then bypass the plugin with audio still routed | The take lands. Bypassed: no sidechain audio bleeds into the output | | |
| LV1 | Aux into Record-in, all three triggers (Manual, Threshold, Next Note) | Each take lands in its slot | | |
| LV2 | No sidechain routed, Arm | "No signal"; nothing captured (QUAL-02) | | |
| LV3 | Save the Set, close, reopen | Lanes, takes and preset come back | | |
| LV4 | Automation + Sync Tempo (Density / Size Sync) in a looped clip (2.5a-9 in Live) | Automation plays back; grains stay locked across loop jumps | | |
| LV5 | Factory presets: browse and load from the plugin's preset bar | List shows the 64 presets by category; a load sounds right | | |

---

## 5. Deferred — DEFERRED, not run (owner: 1.0.x / hardware)

| ID | What | Why deferred |
|---|---|---|
| 2.5b-7 | MPE controller: per-note bend, pressure, slide (Pressure → S1 Level, Slide → S1 Filter Cutoff) | No MPE controller (COMPAT-02 partial, D57) |
| 2.5b-8 | MPE bend range from the controller's RPN 0 | No MPE controller |
| 2.5b-9 | Dorico + VST3 note expression: quarter-tones in preview / playback; Legato glide lands on the tuned pitch | Dorico pass in 1.0.x (D57) |
| 2.5b-10 (CK5f arm) | MPE member bend sweep on a sine: no click / zipper | No MPE controller |
| 2.6-3 (Reaper arm) | VST3 sidechain capture + bypass-while-routed in Reaper | Reaper pass in 1.0.x |
| W1 | Windows: lane drop and browse in the WebView2 UI | Windows UI is CI-only at 1.0.0 (D54) |
| W2 | Windows: DAW load with the WebView rendering | Windows UI is CI-only at 1.0.0 (D54) |

## N/A — "old patches" rows

| ID | Was | Status |
|---|---|---|
| 2.4-11 | Load a 2.1–2.3 patch: unchanged (knee adoption) | N/A — no pre-1.0 sessions exist; harness-covered (TB7 / state rows) |
| 2.5a-12 | Load 2.1–2.4 patches: unchanged | N/A — no pre-1.0 sessions exist; harness-covered (TB7 / state rows) |
| 2.5b-11 | Load 2.1–2.5a patches in Poly, MPE off: unchanged | N/A — no pre-1.0 sessions exist; harness-covered (TB7 / state rows) |
| 2.6-12 | Load 2.1–2.5b patches with Record-in idle: unchanged | N/A — no pre-1.0 sessions exist; harness-covered (TB7 / state rows) |

---

## Requirements each pass closes

| Pass | Requirements |
|---|---|
| 1 Standalone | FUNC-01, FUNC-03, FUNC-04, FUNC-05, FUNC-09, DSP-02, DSP-03, UI-02; the macOS arm of FUNC-02 and UI-01; QUAL-01 (with the 2.6-13 Logic row) |
| 2 Preset | FUNC-08, plus the human arm of PERF-02 (the automated verdict is `--cpu4`) |
| 3 Logic | FUNC-06, FUNC-07; QUAL-02 (AU unrouted: 2.6-2, S3.3-L3, S3.3-L5); DSP-07 (with 2.1-10); PERF-03 (2.1-8 / 2.1-12, 2.6-11) |
| 4 Live | QUAL-02 (VST3 arm), FUNC-07 (VST3) |

Stays partial: UI-01 (Windows arm, D54); COMPAT-02 (MPE and Dorico, D57).

## ❌ triage

Route every ❌ as one of:

- **(a) 1.0.0 blocker** → Stage 4 gap wave `PLAN-4.F` via `/plugin-execute O-GranulatorInstrument 4-polish` (path-scoped commits).
- **(b) Known issue** → CHANGELOG `[1.0.0]` "Known issues".
- **(c) Stale expectation** → rewrite the row (here, and in the source sheet if it carries the same wording).

Verify returns PARTIAL with the list. **VERSION stays 1.0.0** (nothing is published; amend only the CHANGELOG body and date). After a fix wave, retest the ❌ rows plus the adjacent rows in the same pass; the executor reruns the full automated bar (harness, parity, drift, param-dump, renders, auval, pluginval SC on / off, UI gates, `--cpu26`). If the fix touched presets or sources, redo 0-5 / 0-6 first.

| ID | Symptom | Route (a / b / c) | Follow-up |
|---|---|---|---|
| | | | |

## Results

| Pass | Rows | ✅ | ❌ | Notes / routing |
|---|---|---|---|---|
| 0 Setup | 6 | | | |
| 1 Standalone | 103 | | | |
| 2 Preset audition | 71 | | | |
| 3 Logic AU | 41 | | | |
| 4 Live VST3 | 6 | | | |
| 5 Deferred | 7 | — | — | DEFERRED, not run |
| N/A (old patches) | 4 | — | — | harness-covered |

Result: ______ (all run rows ✅ → Stage 4 hands-on PASS; any ❌ → triage above)
