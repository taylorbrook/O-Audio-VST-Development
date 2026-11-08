# O-Granulator Changelog

## [1.1.0] - 2026-11-07

The factory bank grows from 16 to 64 presets, eight in each of eight categories, and every factory preset now runs fully wet.

### Changed

- **Every factory preset is at Mix 100 %.** The 16 presets from 1.0.0 now play only the processed signal. Stream levels were re-voiced so the bank stays within 9 dB of itself (the 64 presets span about 7 dB on the reference stimulus). No preset sets Output Gain.
- **Harmoniser presets keep the played note.** At 100 % wet the dry note no longer comes through, so each Harmoniser preset adds an un-followed stream at the original pitch under its harmony voices.
- **The preset menu opens at the current preset.** With 64 entries the list scrolls, so it now opens with the loaded preset in view.

### Added

- **48 new factory presets** in four new categories (Texture, Pitch FX, Glitch, Drone, eight each) and four more in each of the original categories. The name prefix groups them in the preset menu. The Drone presets leave Freeze off: a preset that turned it on would freeze an empty buffer when the plugin is first inserted. Let the drone build, then press Freeze.
  - **Texture · Granite** — Short, dense, fully sprayed grains with random gain: a coarse, stony grain cloud.
  - **Texture · Swarm** — Three streams of small, slightly scattered grains: a busy, buzzing swarm around the source.
  - **Texture · Rain Glass** — Tiny, high-passed pentatonic droplets scattered an octave up across the stereo field.
  - **Texture · Sand Drift** — Half-reversed grains through a randomised band-pass, drifting slowly through the buffer.
  - **Texture · Velvet** — Overlapping, low-passed Hann grains: a soft, smeared copy of the input.
  - **Texture · Bubbles** — Small, resonant band-passed grains with random cutoff and pitch: bubbling, liquid blips.
  - **Texture · Fur** — Dense, lightly driven short grains, low-passed and spread wide: a warm, fuzzy surface.
  - **Texture · Static Bloom** — Very short, dense grains with occasional crushed hits, blooming into a diffused room.
  - **Pitch FX · Octave Up** — A clean octave-up shift: short, tightly overlapped grains with almost no spray.
  - **Pitch FX · Octave Down** — A clean octave-down shift with longer grains for a smooth low end.
  - **Pitch FX · Sub Octaver** — The source at pitch with one and two octaves below, the lowest low-passed: a heavy sub-octave thickener.
  - **Pitch FX · Fifth Up** — A fifth above the source, with the source itself underneath: an open, power-chord shift.
  - **Pitch FX · Detune Wide** — Three copies a few cents apart, panned left, centre and right: a wide, chorus-like doubler.
  - **Pitch FX · Shimmer Verb** — Octave and two-octave grains into a big, bright reverb with a little feedback: a shimmer reverb.
  - **Pitch FX · Random Steps** — Each grain jumps to a random chromatic step within an octave either way.
  - **Pitch FX · Whole Tone** — Grains scattered across a whole-tone scale on C: dreamy, unresolved pitch clouds.
  - **Glitch · Bitcrush Spray** — Sprayed grains, most of them bit-crushed: a gritty, broken scatter.
  - **Glitch · Ring Tones** — Ring-modulated grains with a randomised carrier: metallic, bell-like glitches.
  - **Glitch · Reverse Scatter** — Mostly reversed grains thrown across the whole buffer at random levels.
  - **Glitch · Stutter Crush** — Tempo-locked 1/32 grains on a sixteenth grid, crushed, with a stepped filter jumping every sixteenth.
  - **Glitch · Broken Radio** — Narrow, resonant band-passed grains, driven, crushed and ring-modulated: a detuned transistor radio.
  - **Glitch · Pitch Chaos** — Unquantised random pitch across two octaves either way, with some grains reversed.
  - **Glitch · Micro Cuts** — Very short, percussive slices with a decaying window and random gain: clicks and ticks cut from the input.
  - **Glitch · Drive Grit** — Hard-driven grains with a touch of crush, low-passed to keep the fizz in check.
  - **Drone · Endless Root** — Two-second grains sprayed far back into the buffer, held up by a diffused loop and a large hall.
  - **Drone · Low Fifths** — An octave-down and a fifth-below-root drone of long grains, dark and steady.
  - **Drone · Organ Pipes** — Sub-octave, root, octave and twelfth streams of long, flat-topped grains: a pipe-organ drone.
  - **Drone · Dark Matter** — Two octaves down, heavily low-passed, in a damped hall: a sub-heavy rumble.
  - **Drone · Bowed Air** — Long band-passed grains with a slowly sweeping centre: a breathy, bowed drone.
  - **Drone · Overtone Hum** — Long grains scattered over the harmonic series 8-16 on C: a hovering overtone chord.
  - **Drone · Buffer Ghost** — Thirty seconds of buffer, read from far back: the drone keeps returning what you played long ago.
  - **Drone · Choir Pad** — Root, fifth and octave streams with a slow cents-wide drift, through a soft vocal band-pass.
  - **Ambient · Glass Garden** — Bright octave grains through a high-pass, with a quiet octave-and-fifth sparkle above, into a wide room.
  - **Ambient · Tidal Wash** — Long grains that swell and recede: a slow sine LFO moves the level and the read position together.
  - **Ambient · Night Field** — Dark, low-passed grains and an octave-down bed in a damped hall; quiet and distant.
  - **Ambient · Wide Mist** — Two slightly detuned streams panned hard apart: a wide, chorused haze around a sprayed centre.
  - **Harmoniser · Minor Triad** — Minor third and fifth above the tracked note, with an octave below, snapped to A minor.
  - **Harmoniser · Dorian Wash** — Stacked fourths in D Dorian, washed through a diffused feedback loop and a long room.
  - **Harmoniser · Octave Pair** — Followed octaves above and below the tracked note, snapped to Octaves; a clean, organ-like doubling.
  - **Harmoniser · Rast Voices** — Neutral-third and fifth voices snapped to Maqam Rast on D, with gentle pitch scatter on the same degrees.
  - **Rhythmic · Dotted Echo** — Sixteenth-note grains on a dotted-eighth grid, fed back into the buffer for a rhythmic echo.
  - **Rhythmic · Octave Hops** — Sixteenth-note grains that leap between octaves at random, quantised to Octaves.
  - **Rhythmic · Half-Time** — Heavy quarter-note grains with a low-passed half-note octave-down stream underneath.
  - **Rhythmic · Reverse 8ths** — Eighth-note reversed grains with a decaying window: every slice swells in backwards.
  - **Cascade · Down Spiral** — Every generation falls a fourth: grains a fourth down in a long, warm feedback loop.
  - **Cascade · Shimmer Loop** — Octave-up grains in a bright, diffused loop: each pass climbs another octave and fades into air.
  - **Cascade · Glass Tower** — Fifth and octave streams feed back together into a bright, ringing stack.
  - **Cascade · Tape Ghost** — Half-reversed grains in a saturated, dark loop with a wandering read position, like a worn tape echo.

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
