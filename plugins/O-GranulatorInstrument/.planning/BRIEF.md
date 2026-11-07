# O-GranulatorInstrument - Creative Brief

## Overview

**Type:** Synth (Sample-Based Polyphonic Granular Instrument)
**Core Concept:** The instrument sibling of O-Granulator. Its four grain streams read imported audio files instead of a live ring buffer, and MIDI notes spawn polyphonic, pitch-tracked grain clouds.
**Status:** 💡 Ideated
**Created:** 2026-11-06
**Sibling:** `plugins/O-Granulator` (Audio Effect, Stage 3). Engine, per-grain FX, feedback network, scales, mod matrix and UI language are inherited from it wherever sensible.

## Vision

O-Granulator is a granular *effect* built around one continuously recording buffer. O-GranulatorInstrument turns the same engine into a playable *instrument*. You drop audio files (field recordings, vocals, tonal samples, the bundled factory sources) onto up to four streams, and you play them from a keyboard. Each note spawns its own voice, and each voice runs its own grain cloud across the active streams. The cloud is transposed by the key relative to each stream's root note. Chords produce stacked clouds, held notes evolve, and velocity, aftertouch/MPE, the mod wheel and envelopes shape every grain.

Everything that makes O-Granulator deep carries over:
- The four streams, with their position/spray/size/density/pitch/window/direction controls.
- Scale-quantised pitch spread.
- Per-grain FX: filter, drive, crush and ring mod.
- The feedback network.
- The output reverb.
- A modulation matrix.

The live-input followers are replaced with instrument modulators: amp and mod ADSRs, velocity, key-track, aftertouch/MPE, the mod wheel and a per-note random value.

The **feedback network** works on a per-stream *working copy* of the loaded file. Grain output is written back into that copy, so held textures self-evolve and smear into shimmer cascades. A **Reset** returns the copy to the pristine file. **Freeze** stops feedback writes.

An optional **Record-in** path captures sidechain/live input into any stream's slot. This makes the instrument a hybrid: you can sample a moment live and then play it polyphonically.

Sonic goals:
- Playable, in-tune granular pads and keys.
- Dense evolving ambient beds from a single sample.
- Rhythmic, tempo-synced mangled fragments.
- No clicks, no aliasing, no runaway feedback.

## Architecture Summary

```
Files (drag/drop, ≤ N s each)      Sidechain in ──► [Record-in → target slot]
   │ load (background thread)                         │
   ▼                                                  ▼
Slot 1..4: pristine file + working copy (feedback writes here)
   │
MIDI ──► Voice allocator (poly, steal) ──► Voice k:
            amp ADSR, mod ADSR, velocity, key, MPE, note-random
            │
            ├─ Stream 1 cloud (scheduler → grains → per-grain FX → pan) reads Slot 1
            ├─ Stream 2 cloud … reads Slot 2
            ├─ Stream 3 / 4 …
            ▼
       Σ voices × amp env ──► Stream mixer (level/pan/mute/solo)
            │
            ├──► Feedback: HP/LP tone → soft-clip → diffuser → write into working copies
            ▼
       Output reverb/diffuser ──► Output gain
```

All grains of all voices are drawn from one shared grain pool. The pool has a global grain limit and fade-on-steal.

## Parameters

Parameters inherited from O-Granulator keep its IDs, ranges and semantics unless noted here. This brief lists only the **changes and additions**. See `plugins/O-Granulator/.planning/BRIEF.md` and `parameter-spec.md` for the inherited set.

### Global / Voice

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| polyphony | 1 – 16 voices | 8 | Max simultaneous notes (oldest/quietest steal with fade) |
| voiceMode | Poly / Mono / Legato | Poly | Mono/Legato use one cloud with glide |
| glide | 0 – 2000 ms | 0 ms | Portamento (Mono/Legato, or poly glide) |
| pitchBendRange | 0 – 48 st | 2 st | Pitch bend range (MPE: per-note bend) |
| grainLimit | 32 – 512 | 256 | Shared grain-pool cap across all voices/streams |
| freeze | Off/On | Off | Stops feedback writes into the working copies |
| resetBuffers | Trigger | — | Restore all working copies to the pristine files |
| syncMode | Free / Tempo | Free | As O-Granulator |
| outputGain | -24 to +12 dB | 0 dB | Final level |
| ~~inputGain~~, ~~mix~~ | — | — | **Removed.** An instrument has no dry path. Record-in has its own gain. |

### Amp Envelope (per voice)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| ampAttack | 0 – 10 s | 20 ms | Voice fade-in |
| ampDecay | 0 – 10 s | 300 ms | |
| ampSustain | 0 – 100 % | 100 % | |
| ampRelease | 0 – 20 s | 1.5 s | Voice tail. Grains keep spawning during release. |
| velToAmp | 0 – 100 % | 70 % | Velocity sensitivity |

### Per Stream (×4) — changes vs O-Granulator

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| sN_position | 0 – 100 % | 0 % | **Now:** read point within the file / loop region (was: distance behind the write head) |
| sN_scanRate | -400 – +400 % | 0 % | **New:** playhead speed through the file (0 = static, 100 = real-time) |
| sN_scanMode | Free-run / Retrigger / Ping-pong | Retrigger | **New:** does each note restart the scan at `position`? |
| sN_loopStart / sN_loopEnd | 0 – 100 % | 0 / 100 % | **New:** region the scan/spray is confined to |
| sN_rootNote | C-1 – G9 (+ fine ±100 ct) | C3 | **New:** key at which the file plays untransposed |
| sN_keyTrack | 0 – 100 % | 100 % | **New:** how much the played key transposes the grains (0 = drone/fixed pitch) |
| sN_pitch | -24 – +24 st | 0 | Offset on top of key-tracking (inherited) |
| sN_scale | (as O-Granulator) | Off | Quantises the grain pitch spread. The key itself is never quantised. |
| *all other sN_ params* | inherited | | spray, size, sizeRand, density, jitter, pitchRand, reverseProb, window, windowSkew, stereoSpread, gainRand, level, pan, enable, mute/solo |

### Sources (per slot, plugin state, not automatable)

- File path plus an embedded/relocatable reference. A missing file shows "relink". Factory sources resolve from the installed content folder.
- Formats: WAV / AIFF / FLAC (+ MP3/OGG where JUCE readers allow). Resampled to the session rate on load. Stereo is kept; mono is upmixed.
- Max length per slot: TBD in planning (e.g. 60 s at 48 kHz stereo).
- Loading happens off the audio thread with a lock-free swap. Voices on that slot fade out across the swap.

### Record-In (sidechain)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| recTarget | Slot 1 – 4 | Slot 1 | Which slot captures |
| recArm / recTrigger | Off / Armed / Recording | Off | Start/stop manually, on threshold, or on the next note |
| recLength | 0.5 – 30 s or sync bars | 4 s | Capture length |
| recGain | -24 to +12 dB | 0 dB | Sidechain input level |

### Per-Grain FX, Feedback Network, Output FX

Inherited unchanged from O-Granulator: `sN_filterType/Cutoff/Rand/Res`, `sN_drive`, `sN_crush/crushProb`, `sN_ringFreq/Rand/Mix`, `feedback`, `fbLowCut`, `fbHighCut`, `fbSaturation`, `fbDiffusion`, `reverbMix/Size/Damp`.

**Change:** feedback writes into the *working copy* of each stream's slot, at that stream's current scan position, instead of into a live ring. The voice sum feeds back, so feedback depth needs a voice-count-aware safety limit.

### Modulation — changes vs O-Granulator

| Source | Notes |
|--------|-------|
| LFO 1–4 | Inherited. Adds a per-LFO **Global / Per-voice** mode, with retrigger on note when per-voice. |
| Mod ADSR 1–2 | **New** per-voice envelopes |
| Velocity, Key-track, Note-random | **New** per-voice sources |
| Mod wheel, Aftertouch (channel + poly), MPE pressure / slide (CC74) / bend | **New** |
| Macros 1–4 | Inherited |
| ~~Env follower, Pitch follower, Onset follower~~ | **Removed** (no live input path; record-in does not drive mods in v1) |
| Matrix | 16 slots, bipolar depth, as O-Granulator. Per-voice sources modulate per-voice destinations. |

## UI Concept

Inherit O-Granulator's finalized v2 language: Ouaricon Naturalist dark, EB Garamond, ochre mod accent, moss/madder/woad/thistle stream colours, 1200×800, with the header, pages and drag-to-modulate. Expected differences, to be designed in the mockup phase:
- The buffer strip becomes **per-stream waveform lanes**. Each lane has drag-and-drop, file name/relink, loop region handles, the scan playhead, and live grain dots for each voice.
- A voice/amp-env section in the header or on the STREAMS page, plus a small keyboard/voice-activity indicator.
- The MOD page swaps the follower panels for mod ADSRs and MIDI/MPE sources.
- Record-in controls sit with the waveform lanes.

## Use Cases

- Playable granular pads/keys from any sample: chords give stacked, evolving clouds.
- Ambient/drone: long releases, a slow scan and feedback diffusion on held notes.
- Scoring/sound design: MPE pressure/slide shaping density, spray and filters per note.
- Rhythmic: tempo-synced density and per-voice LFOs on short loops and drums.
- Hybrid live sampling: record a phrase via sidechain and play it back polyphonically.

## Inspirations

- Ableton Granulator III (poly/MPE mode), Spectrasonics Omnisphere granular, Steinberg Padshop, Output Portal/Arcade, Arturia Pigments sample-granular engine, Tasty Chips GR-1
- Mutable Instruments Clouds/Beads (feedback, freeze)
- Internal: O-Granulator (direct parent), O-MicrotonalSampler (file loading/sample state patterns), O-Lyrica (MPE/note-expression reference), O-TextureForge (RT-safe buffer swap and retired-buffer reaper)

## Technical Notes

- **Code reuse strategy (decide in Stage 0):** extract the O-Granulator stream/grain/FX/feedback core into a shared module, or fork it. O-Granulator is still in Stage 3, so a fork-now/converge-later plan must be weighed against fork-drift (see the FORK-REVIEW memory).
- **CPU is the dominant risk.** A voice multiplies the cloud cost: 16 voices × 4 streams × density. The shared grain pool with a global `grainLimit` and fade-on-steal is mandatory. It needs a CPU budget plus graceful degradation, and memory notes apply: cap lowered live never converges; density norm over the cap.
- **Voice stealing** must fade (no clicks). JUCE `Synthesiser` voice steal allocates in 8.0.15, so use the `WtSynthesiser.h` port or a custom allocator.
- **File loading** is off-thread, with an atomic swap and a retired-buffer reaper. The reaper must not freeze when the host idles the audio thread (see the TextureForge and generation-reaper memories).
- **Working copy + feedback:** the read/write ordering must be explicit (the read-before-capture-write pattern). Reset and Freeze semantics also need defining, and memory is doubled per slot.
- **State:** file references (relative/absolute and factory), loop regions, scale/custom-scale JSON and degree masks (as O-Granulator). Decide whether audio can be embedded in state for portability (size limits).
- **MPE / note expression:** follow the O-Lyrica reference, including Dorico microtonal note expression if it is cheap to add.
- **Factory content:** a small curated set of bundled source sounds (licence-clean, recorded or synthesised in-house) plus presets that reference them. Installer/content-folder handling follows the existing packaging pipeline.
- Record-in needs a sidechain input bus on an instrument. AU instruments with a sidechain are host-dependent (see the AU unrouted-sidechain memory), so this must degrade cleanly.

## Out of Scope (v1, provisional)

- Multi-sample pools / crossfading between files within a stream
- Keys → position slicing mode
- Record-in driving mod sources (followers)
- Time-stretch scan that preserves formants

## Next Steps

- [ ] Stage 0 planning (`/plan O-GranulatorInstrument`)
- [ ] UI mockup (`/start O-GranulatorInstrument` → option 3)
