# O-simpleBeatmaker - Creative Brief

## Overview

**Type:** Synth (Pedagogical Step-Sequencer Drum Machine)
**Core Concept:** A deliberately simple TR-808/909-style drum machine with a built-in 16-step sequencer — designed to make the craft of beatmaking tangible: program a beat on the grid, then watch and hear **velocity, swing, quantize, and humanization** reshape it in real time.
**Status:** 💡 Ideated
**Created:** 2026-06-25

## Vision

O-simpleBeatmaker is the rhythm sibling to **O-simpleFM**, **O-simpleAdditive**, **O-simpleGrain**, and **O-simpleSubtractive** — same pedagogical DNA, different subject. Where the others each isolate one *synthesis* technique, O-simpleBeatmaker isolates the thing that turns sounds into *music with a groove*: **MIDI sequencing and timing feel.** It is a teaching instrument first and a drum machine second.

It is built to run alongside the MUSC319 wk09 MIDI & beatmaking session. That class teaches that **MIDI is control data, not audio** — a stream of note-on/note-off messages carrying pitch and velocity that drives whatever instrument is loaded — and that a beat is programmed on a grid (the step sequencer / piano roll), where **velocity, quantize strength, swing, and humanization** separate a stiff pattern from a living one. The class's headline interactive demo is a 16-step kick/snare/hi-hat grid where you click a cell again to accent it, push tempo and swing, and raise humanize to loosen machine-tight timing. O-simpleBeatmaker turns that demo into a real DAW instrument so students can **reproduce the moves in their own session for A3** — and, crucially, *see why each move works.*

The pedagogical payload is the simple-family north star: the tight loop between **gesture and visible consequence.** Light a step and watch the playhead trigger it. Set a hard hit on the backbeat and a quiet ghost note between — see the cells change height and hear the pattern gain dynamics instead of marching. Push swing and watch the off-beat 16ths slide *visibly* later in a dedicated timing lane while the groove starts to shuffle. Raise humanize and watch every hit scatter slightly off the grid lines; then turn quantize strength up and watch them get pulled back toward the grid — the exact tradeoff the class names ("quantize enough that the part is solid without over-quantizing the life out of it"), made visual. A live MIDI readout prints the note-on messages (note number + velocity) as steps fire, so the abstract claim "the sequencer emits MIDI" becomes something you watch happen.

Every control carries a short, plain-language tooltip (what velocity is, why a flat pattern sounds mechanical, what swing delays, what quantize strength trades away). The design north star, like its siblings: a curious student reaches a genuine "oh, *that's* what swing/quantize/velocity actually do" moment within five minutes, with no manual — and leaves able to program the A3 groove and feel it sit rather than march.

## Architecture

**A 16-step grid of synthesized drum voices, sequenced by an internal host-synced clock and playable live over MIDI.** The drum voices are deliberately transparent (808/909-lineage synthesis, no samples, no black box); the *sequencer and the timing-feel engine are the pedagogical heart.*

```
  MIDI in (DAW piano roll / controller) ─────────────┐
                                                      ▼
  Internal 16-step sequencer ──► step triggers ──► DRUM VOICES ──► mixer ──► out
   (synced to host transport)        │             (synthesized:
        ▲         ▲        ▲         │              kick, snare, clap,
        │         │        │         │              closed/open hat, tom…)
      swing   humanize  quantize     │
        └─────────┴────────┴──► per-hit TIMING OFFSET (Δt) + VELOCITY
                                      │
                       ┌──────────────┴───────────────┐
                       ▼                               ▼
              timing/groove lane              live MIDI message readout
            (each hit's Δt vs grid)         (note-on: note#, velocity)
```

- **Step grid:** rows = drum voices, columns = 16 sixteenth-note steps (one bar). Toggle a hit per cell; each lit step carries an editable **velocity** (with quick accent / ghost states, mirroring the demo's click-again-to-accent).
- **Internal sequencer:** runs in sync with the host transport (follows DAW tempo + play position); the on-screen playhead sweeps the grid in time. The same voices also respond to **incoming MIDI notes** (GM drum mapping) so a student can program the identical pattern from the DAW piano roll and hear the same instrument — making concrete that *the step grid and the piano roll are two views of one MIDI stream.*
- **Timing-feel engine (the lesson):** three global controls reshape *when* each hit actually fires, computed as sub-step sample-accurate offsets:
  - **Swing** delays the off-beat 16ths into long-short pairs (the shuffle of funk/hip-hop/house).
  - **Humanize** adds small random timing + velocity offsets so repeats aren't identical (the "loosely played" stand-in).
  - **Quantize strength** pulls the humanized deviation back toward the grid — 100% = dead tight, lower = keeps human looseness. (Swing is treated as intentional groove and is *not* removed by quantize — matching how DAWs layer groove over quantize.)
- **Synthesized drum voices:** an 808/909-lineage set, each with a few transparent shaping knobs (tune, decay, tone/snap, level) — enough to be musical and "see-inside," few enough that they never compete with the sequencing lesson.
- **Visualization layer:** the timing/groove lane and the live MIDI readout are first-class teaching features, not decoration.

## Parameters

*Core set defined here; Stage 0 research should confirm the exact voice roster, swing/humanize/quantize formulations, ranges, and tapers. Ranges below are starting proposals.*

### Sequencer / timing-feel (the pedagogical core)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Step On/Off (per voice × 16) | on/off | off | The grid. Toggle a hit at each step — the fastest way to build a drum pattern. |
| Step Velocity (per lit step) | 0–127 (with ghost / normal / accent quick-states) | normal (~100) | Per-hit dynamics. Accents on the backbeat + ghost notes between give a pattern its human read. Cell height/brightness shows it. THE velocity lesson. |
| Swing | 0–75% | 0% | Delays the off-beat 16ths into long-short pairs. The shuffle at the heart of groove. |
| Humanize | 0–100% | 0% | Random per-hit timing + velocity offset so repeats aren't identical copies (the "loosely played" stand-in). |
| Quantize Strength | 0–100% | 100% | How far the humanized timing is pulled back toward the grid. 100% = dead tight; lower = keeps human looseness. THE quantize lesson. |
| Pattern Length | 8 / 16 / 32 steps | 16 | Steps per bar/loop (16 = sixteen sixteenths, the classic grid). Confirm range in research. |
| Tempo | follows host transport | host | Sequencer runs in sync with the DAW. (Confirm whether to expose an internal free-run tempo for standalone auditioning.) |

### Per drum voice (kept intentionally minimal — transparent, not a black box)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Voice Tune | -12 to +12 semitones (or per-voice musical range) | 0 | Pitch of the synthesized voice. |
| Voice Decay | short–long | per-voice | Length of the voice's tail (e.g. kick boom, open-hat sustain). |
| Voice Tone / Snap | 0–100% | per-voice | One timbral knob per voice (kick click/punch, snare tone vs snap, hat brightness). Exact mapping per voice in research. |
| Voice Level | -inf–0 dB | 0 dB | Per-voice mixer level. |
| Voice Mute / Solo | on/off | off | Standard mixer mutes/solos for isolating parts while teaching. |

### Master

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Output Level | -inf–0 dB | 0 dB | Master output gain. |

**Proposed drum-voice roster (confirm in research):** Kick, Snare, Clap, Closed Hat, Open Hat, Tom (≈6 voices). Possible additions: Rimshot, Cowbell. The demo uses 3 (kick/snare/hat); ~6 gives enough range for real beats while staying projector-readable. Final count set in Stage 0.

**Likely additions / confirmations from research (Stage 0):** the 808-vs-909 synthesis flavor per voice, sub-step timing-offset scheduling (sample-accurate triggering within `processBlock`), the exact swing curve (e.g. % delay of even 16ths) and whether 8th-note vs 16th-note swing, the humanize distribution + how it splits timing vs velocity, how quantize-strength composes with swing, GM drum note mapping for MIDI-play, voice/polyphony handling (per-voice retrigger + tail), per-voice pan (nice-to-have), and whether pattern presets ship as factory data.

## UI Concept

*Captured from user-volunteered direction + the class demo; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable. Top-to-bottom: the **16-step grid** (voice rows × 16 columns, with the playhead) as the dominant element | the **timing/groove lane** directly beneath the grid (each hit's offset from its grid line) | global **Swing / Humanize / Quantize Strength** controls | a compact **per-voice strip** (tune / decay / tone / level / mute-solo) | the **live MIDI readout** | master output. Pattern-preset selector for the concept tour.

**Visual Style:** Clean, instructional, uncluttered — consistent with the simple family. Readable at a glance, high-contrast for a projector.

**Key Elements (pedagogical layer — first-class functional features, not decoration):**
- **16-step grid with live playhead** — the main control surface and the headline visual; rows are drum voices, columns are sixteenth-note steps, the playhead sweeps in sync with the host.
- **Per-step velocity made visible** — cell height/brightness encodes velocity; accent and ghost states are obvious at a glance (the demo's click-again-to-accent, generalized to real velocity).
- **Timing / groove lane** — the standout teaching visual: shows each hit's actual time offset from the grid as swing and humanize push hits off the line and quantize strength pulls them back. Makes "feel" something you can *see*.
- **Live MIDI message readout** — prints note-on messages (note number + velocity) as steps fire, so "the sequencer emits MIDI control data" is watched, not just asserted.
- **Concept-isolating pattern presets** — a tour where each preset isolates one idea (straight/no-feel, backbeat + accents, ghost notes, swing, humanized, plus a couple genre grooves) so students reverse-engineer the moves.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (what velocity maps to, why a flat pattern marches, what swing delays, what quantize strength trades).

## Use Cases

- **Classroom demonstration** — instructor projects the plugin, programs a kick/snare/hat beat, sets accents and ghost notes, then pushes swing, humanize, and quantize strength live — the grid, the timing lane, and the MIDI readout all respond immediately so the whole class sees *and* hears each concept.
- **Self-directed student learning** — a student works the pattern-preset tour, reads tooltips, and builds a groove that "sits," then reproduces the move in their DAW for A3.
- **Step-grid ↔ piano-roll bridge** — because the voices are MIDI-playable, a student can program the same pattern in the DAW piano roll and hear the identical instrument, internalizing that the two editors are views of one MIDI stream.
- **Lightweight creative drum machine** — playable and musical enough to double as a simple 808/909-style beat instrument in real projects.

## Inspirations

- **O-simpleFM / O-simpleAdditive / O-simpleGrain / O-simpleSubtractive** — the direct siblings and pedagogical template (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets, single projector-readable page).
- **Roland TR-808 (1980) / TR-909 (1983)** — the canonical step-button grid the class cites as the ancestor of the on-screen sequencer, and the synthesized-drum-voice lineage.
- **Akai MPC / FL Studio Channel Rack / Ableton Drum Rack + step grid** — the modern step-sequencer workflows the class maps; O-simpleBeatmaker is the deliberately minimal teaching counterpart.
- **MUSC319 wk09 step-sequencer demo** — the embedded 16-step kick/snare/hat sequencer with accent, tempo, swing, and humanize; this plugin lets students reproduce that move in a real DAW instrument and *see* the timing feel.

## Technical Notes

- **DSP (drum voices):** synthesized 808/909-lineage voices — kick (pitched sine/triangle with pitch + amp envelope and a click transient), snare (tonal body + noise), hats (band-passed/high-passed noise, closed vs open decay), clap (multi-burst noise + reverb-ish tail), tom (pitched body + decay). Decide the 808-vs-909 flavor per voice in research. Voices are transparent by design (no samples).
- **Sequencer clock:** derive step timing from the host transport (tempo + ppq position) so the grid aligns to the DAW bar; the on-screen playhead reflects it. Confirm fallback internal clock for standalone/no-transport auditioning. Real-time safe.
- **Timing-feel engine:** swing, humanize, and quantize strength resolve to a per-hit **sub-step time offset (Δt)** so triggers are scheduled sample-accurately *within* the processing block (not snapped to block boundaries). Humanize randomness must be real-time safe (pre-seeded RNG, no allocation in `processBlock`). Define the exact swing curve, humanize distribution (timing vs velocity split), and how quantize-strength composes with swing in research — the relationships must be both audible and visible in the timing lane.
- **Velocity:** per-step velocity (0–127) drives voice loudness and a little timbre (harder = brighter/snappier), so accents and ghost notes read dynamically. Incoming MIDI note-on velocity drives the same path when played live.
- **MIDI:** GM drum note mapping so each voice has a stable note number; incoming MIDI notes trigger voices directly (MIDI-playable from the DAW piano roll / a pad controller). The live MIDI readout surfaces note-on (note#, velocity) events from both the internal sequencer and incoming MIDI.
- **Visualizations:** playhead position, per-step trigger/velocity, per-hit timing offsets, and MIDI events cross to the UI via a lock-free FIFO / atomics; keep audio-thread work allocation-free. The timing lane must accurately reflect the Δt actually applied to audio.
- **Polyphony / voices:** each drum voice is monophonic-retrigger with its own tail; the instrument mixes ~6 voices. Confirm exact handling in research.
- **Platform:** WebView UI (JUCE 8) for the rich live grid, timing lane, MIDI readout, and tooltips — consistent with the simple family. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards. Two `juce_add_binary_data` targets (if any embedded data + WebView resources) must use distinct `NAMESPACE` values to avoid the BinaryData collision seen in O-simpleGrain Stage 3.1.

## Out of Scope (v1.0)

- **Bass / melodic lanes** — decided drums-only to keep the groove/velocity/swing/quantize focus tight; the A3 bassline is programmed with a separate instrument in the DAW.
- **Sample loading / sampled kits** — voices are synthesized and transparent; a sample engine is O-MicrotonalSampler / DrumRoulette territory.
- **CC / orchestration-realism teaching** (velocity layers, CC1/CC11 swells, keyswitches) — the class covers it, but it belongs to a deep sample library, not this minimal beat instrument. Deliberately out.
- **Song mode / pattern chaining (A/B/…)** — v1 is a single pattern; chaining is a future expansion.
- **Per-step probability, ratchets/rolls, parameter locks (Elektron-style)** — out of the pedagogical core; future.
- **Per-row step lengths / polymeter** — future.
- **Effects (reverb/delay/comp) beyond a master output** — keep the signal path transparent for teaching.

## Next Steps

- [ ] Create UI mockup (`/start O-simpleBeatmaker` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleBeatmaker`)
