# O-Granulator Changelog

## [1.0.0] - 2026-11-07

First release. O-Granulator is a live-input granular effect: four grain streams read one continuously recording buffer (up to 30 s), each with its own position, pitch, density, size, window, direction, spray and per-grain filter, drive, crush and ring modulation. A feedback network (tone filters, saturation, diffuser and a loop AGC) returns the grains to the buffer, Freeze holds it, and a 16-slot modulation matrix routes 4 LFOs, an envelope follower, pitch and onset followers and 4 macros to 112 destinations. VST3, AU and Standalone; English, French and Simplified Chinese UI.

### Added

- **Four grain streams** with level, pan, mute and solo, Free (Hz / ms) or Tempo (note divisions) timing, window shape and skew, reverse probability, stereo spread and gain / pitch / size / filter / ring randomisation.
- **Scales and pitch follow:** 30 built-in pitch collections (12-TET modes, EDOs, just, Bohlen-Pierce, gamelan, maqam) plus one custom scale (cents, ratios or `edo N`), per-degree masks, Scale Root, and per-stream Pitch Follow that snaps the tracked input note to the scale.
- **Onset triggers:** grain bursts fired by transients in the input.
- **Feedback network:** low / high cut, saturation, diffusion and a loop AGC, with a loop-gain readout.
- **Freeze** with a click-free release, and an output reverb.
- **Modulation matrix:** 16 routes from LFO 1–4, Env, Pitch, Onset and Macro 1–4.
- **16 factory presets**, four per use case. The name prefix groups them in the preset menu. No preset sets Output Gain.
  - **Ambient · Cloud Drift** — Long, sprayed Hann grains with an octave-up second stream; a slow LFO walks the read position through the buffer.
  - **Ambient · Frozen Halo** — A three-voice root / fifth / octave halo of very long grains into a large diffused room.
  - **Ambient · Slow Shimmer** — Octave and two-octave grains recirculate through a bright, diffused loop; smooth random panning keeps it moving.
  - **Ambient · Dust Bloom** — Dense, jittered grains through a randomised low-pass with a reversed octave-down bed.
  - **Harmoniser · Major Choir** — Three followed voices a third, a fifth and an octave above the tracked note, snapped to C major.
  - **Harmoniser · Fifths** — Power-chord harmoniser: fifth and octave voices snapped to Fifths + Octaves in D.
  - **Harmoniser · Just Halo** — Pure-interval harmony: thirds and fifths snapped to 5-limit just intonation in A, with a soft halo.
  - **Harmoniser · 19-EDO Haze** — Followed voices snapped to a masked 19-EDO custom scale in E, with random pitch scatter on the same degrees.
  - **Rhythmic · Stutter 16** — Sixteenth-note grains locked to the host tempo; a square LFO at 1/8 gates the level.
  - **Rhythmic · Triplet Dust** — Eighth-note triplet bursts with random reverse and crushed accents, panned by a stepped LFO.
  - **Rhythmic · Gate Pulse** — Quarter-note pulses of two streams an octave apart; a saw LFO at one bar sweeps the band-pass.
  - **Rhythmic · Onset Spray** — Grain bursts fired by the input's transients, with tempo-synced sixteenth-note tails.
  - **Cascade · Infinite** — Near-unity feedback through a gentle diffuser: an endless, slowly evolving tail.
  - **Cascade · Rust Spiral** — Hot, saturated feedback with driven grains; the AGC holds the spiral at the saturation ceiling.
  - **Cascade · Pitch Ladder** — Every generation climbs a fifth: pitched grains in a long feedback loop build a rising ladder.
  - **Cascade · Dark Bloom** — Octave-down grains in a dark, heavily diffused loop; a slow LFO breathes the feedback amount.

### Changed

- **Feedback AGC now tracks the saturation setting.** The loop AGC follows the saturator's normalised output, so it engages at the same loop excess at every Saturation value. Before, it never engaged above about 6 % Saturation (the default is 20 %). Saturation 0 % is unchanged, bit for bit.
