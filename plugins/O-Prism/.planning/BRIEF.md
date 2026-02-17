# O-Prism Creative Brief

## Vision Statement

O-Prism is a **wavetable synthesizer with integrated microtonal capabilities** — combining Serum-class sound design power with the proven tuning engine from O-Lyrica and O-Bells. It is the first wavetable synth designed from the ground up for microtonal composition, giving producers and composers access to any tuning system imaginable while delivering the modern wavetable sound that defines contemporary electronic music.

The name "Prism" reflects how a single beam of light refracts into a full spectrum of colors — just as O-Prism refracts standard pitch into the full spectrum of microtonal intervals.

## Core Concept

A professional wavetable synthesizer with two morphable wavetable oscillators, sub oscillator, noise generator, dual multi-mode filters, and a built-in effects rack — rivaling Xfer Serum in sound design capability while integrating the complete microtonal engine (24+ factory tunings, Scala/KBM import, EDO/harmonic/rank-2 generators) that no competing wavetable synth offers.

## Target Use Cases

**Primary:**
- Microtonal electronic production — EDM, ambient, experimental with non-12-TET tunings
- Sound design — wavetable morphing, spectral manipulation, textural synthesis
- World music production — Turkish maqam (53-EDO), Arabic quarter-tone, Indian sruti, gamelan

**Secondary:**
- Standard 12-TET production — competitive with Serum/Vital for conventional use
- Film scoring — microtonal tension, unusual harmonic color
- Academic/experimental — just intonation composition, xenharmonic exploration
- Live performance — real-time wavetable morphing with microtonal scales

## Competitive Positioning

| Feature | Xfer Serum | Vital | **O-Prism** |
|---------|-----------|-------|-------------|
| Wavetable Oscillators | 2 | 3 | **2 + Sub + Noise** |
| Wavetable Morphing | Yes | Yes | **Yes** |
| Custom Wavetable Import | Audio + Image | Audio | **Audio** |
| Filters | Dual multi-mode | Dual multi-mode | **Dual multi-mode** |
| Effects | 10 rearrangeable | 6 | **5 (v1.0), 10 (v2.0)** |
| Modulation | Drag-and-drop matrix | Drag-and-drop matrix | **v2.0** |
| Microtonal Support | None (12-TET only) | Basic pitch offset | **Full engine: 24+ tunings, Scala, EDO, generators** |
| Wavetable Editor | Built-in | Built-in | **v2.0** |
| Price | $189 | Free/Paid | **TBD** |
| UI | Custom GUI | OpenGL | **WebView (1200x800)** |

**Unique selling proposition:** The only wavetable synth with a professional-grade microtonal engine. Every other competitor is locked to 12-TET or offers only basic pitch offset.

---

## Release Roadmap

### v1.0 — Core Engine (Initial Release)

The complete synthesizer core: oscillators, filters, effects, and microtonal engine. A fully usable, professional-quality instrument.

**Synthesis:**
- 2 wavetable oscillators with position morphing
- Sub oscillator (sine/triangle/saw/square)
- Noise generator with multiple noise types
- 16-voice polyphony with voice stealing

**Filtering:**
- Dual multi-mode filters
- Types: LP12, LP24, HP12, HP24, BP, Notch
- Serial and parallel routing modes
- Cutoff, resonance, drive per filter
- Key tracking

**Effects (5):**
- Reverb (hall/plate algorithms, size, damping, pre-delay, mix)
- Delay (sync/free, ping-pong, feedback, LP filter, mix)
- Chorus (rate, depth, voices, stereo, mix)
- Distortion (soft clip, hard clip, tube, fold, mix)
- EQ (3-band parametric, low/mid/high with frequency and gain)

**Microtonal Engine (ported from O-Lyrica/O-Bells):**
- 24+ factory tunings across 5 categories
  - Historical: Young 1799, Neidhardt, Kellner Bach, Lehman Bach, Vallotti
  - Just Intonation: Ptolemy, 5-limit, 7-limit, Partch 43-tone
  - Equal Divisions: 17-EDO, 19-EDO, 22-EDO, 31-EDO, 41-EDO, 53-EDO
  - Non-Octave: Bohlen-Pierce, Carlos Alpha/Beta/Gamma
  - Standard: 12-TET
- Full Scala (.scl) file import with cents and ratio support
- Keyboard mapping (.kbm) support
- Scale generators: EDO, harmonic series, rank-2 temperament
- Tonic selection with proper anchor-point shifting for non-12 scales
- Octave stretch control
- Per-note pitch bend
- Lock-free 128-note pre-computed frequency table

**Wavetable Library:**
- 100+ factory wavetables across categories:
  - Analog (classic saw, square, pulse, triangle morphs)
  - Digital (FM, additive, bitcrushed, metallic)
  - Spectral (formant, vowel, harmonic sweeps)
  - Organic (vocal, breath, string, bell-like)
  - Textural (noise-based, granular, evolving)
- Audio file import as custom wavetable (WAV/AIFF)

### v2.0 — Full Feature Set (Future Release)

- Drag-and-drop modulation matrix (LFOs, envelopes to any parameter)
- 5 additional effects (Phaser, Flanger, Compressor, Filter, Hyper/Dimension)
- Built-in wavetable editor (draw, additive partial control, spectral editing)
- Rearrangeable effects chain
- Extended wavetable library (250+)
- Preset browser with categories and search
- Additional filter types (comb, vowel, formant, analog-modeled)

---

## Technical Specifications

### Oscillator Architecture

**Wavetable Oscillator (x2):**
- Wavetable size: 2048 samples per frame
- Frames per table: Up to 256
- Position control: Continuous morph between frames (0.0–1.0)
- Phase: Free-running or reset on note-on
- Unison: 1-8 voices with detune spread and stereo width
- Coarse tune: ±24 semitones
- Fine tune: ±100 cents
- Level: -inf to +6dB
- Pan: L100 to R100
- Waveform interpolation: Linear between frames, bandlimited (anti-aliased)

**Sub Oscillator:**
- Shapes: Sine, Triangle, Saw, Square
- Octave: -2 to 0 (relative to fundamental)
- Level control
- Direct to output (bypasses filters)

**Noise Generator:**
- Types: White, Pink, Brown, Digital, Vinyl, Wind
- Level control
- Routable to filter or direct output

### Filter Architecture

**Filter A & Filter B:**
- Types: LP12, LP24, HP12, HP24, BP12, BP24, Notch
- Cutoff: 20Hz - 20kHz
- Resonance: 0-100% (self-oscillating at max for LP/HP)
- Drive: 0-100%
- Key tracking: 0-100%
- Routing: Serial (A→B), Parallel (A+B), or single filter

### Voice Architecture

- Polyphony: 16 voices
- Voice stealing: Oldest note priority
- Glide/Portamento: Off, Legato, Always (with adjustable time)
- Pitch bend range: ±1 to ±48 semitones (crucial for microtonal)

### Audio Specifications

- Sample rates: 44.1kHz, 48kHz, 88.2kHz, 96kHz
- Internal processing: 64-bit double precision
- Oversampling: 2x for anti-aliasing (wavetable interpolation)
- Latency: Zero (no lookahead processing)

---

## Parameters (v1.0)

### Oscillator Section

| Parameter | ID | Range | Default |
|-----------|-----|-------|---------|
| Osc A Wavetable | oscATable | Table index | 0 |
| Osc A Position | oscAPos | 0.0–1.0 | 0.0 |
| Osc A Level | oscALevel | 0.0–1.0 | 0.8 |
| Osc A Pan | oscAPan | -1.0–1.0 | 0.0 |
| Osc A Coarse | oscACoarse | -24–+24 | 0 |
| Osc A Fine | oscAFine | -100–+100 | 0 |
| Osc A Phase | oscAPhase | 0.0–1.0 | 0.0 |
| Osc A Unison | oscAUnison | 1–8 | 1 |
| Osc A Detune | oscADetune | 0.0–1.0 | 0.2 |
| Osc A Width | oscAWidth | 0.0–1.0 | 0.5 |
| Osc B Wavetable | oscBTable | Table index | 0 |
| Osc B Position | oscBPos | 0.0–1.0 | 0.0 |
| Osc B Level | oscBLevel | 0.0–1.0 | 0.0 |
| Osc B Pan | oscBPan | -1.0–1.0 | 0.0 |
| Osc B Coarse | oscBCoarse | -24–+24 | 0 |
| Osc B Fine | oscBFine | -100–+100 | 0 |
| Osc B Phase | oscBPhase | 0.0–1.0 | 0.0 |
| Osc B Unison | oscBUnison | 1–8 | 1 |
| Osc B Detune | oscBDetune | 0.0–1.0 | 0.2 |
| Osc B Width | oscBWidth | 0.0–1.0 | 0.5 |
| Sub Shape | subShape | Sine/Tri/Saw/Sq | Sine |
| Sub Octave | subOctave | -2–0 | -1 |
| Sub Level | subLevel | 0.0–1.0 | 0.0 |
| Noise Type | noiseType | White/Pink/Brown/etc | White |
| Noise Level | noiseLevel | 0.0–1.0 | 0.0 |

### Envelope Section

| Parameter | ID | Range | Default |
|-----------|-----|-------|---------|
| Amp Attack | ampAttack | 0.001–10.0s | 0.01 |
| Amp Decay | ampDecay | 0.001–10.0s | 0.3 |
| Amp Sustain | ampSustain | 0.0–1.0 | 0.7 |
| Amp Release | ampRelease | 0.001–20.0s | 0.5 |
| Filter Attack | filtAttack | 0.001–10.0s | 0.01 |
| Filter Decay | filtDecay | 0.001–10.0s | 0.5 |
| Filter Sustain | filtSustain | 0.0–1.0 | 0.5 |
| Filter Release | filtRelease | 0.001–20.0s | 0.5 |
| Filter Env Depth | filtEnvDepth | -1.0–1.0 | 0.0 |

### Filter Section

| Parameter | ID | Range | Default |
|-----------|-----|-------|---------|
| Filter A Type | filtAType | LP12/LP24/HP12/HP24/BP/Notch | LP24 |
| Filter A Cutoff | filtACutoff | 20–20000 Hz | 20000 |
| Filter A Resonance | filtARes | 0.0–1.0 | 0.0 |
| Filter A Drive | filtADrive | 0.0–1.0 | 0.0 |
| Filter A KeyTrack | filtAKeyTrack | 0.0–1.0 | 0.0 |
| Filter B Type | filtBType | LP12/LP24/HP12/HP24/BP/Notch | LP24 |
| Filter B Cutoff | filtBCutoff | 20–20000 Hz | 20000 |
| Filter B Resonance | filtBRes | 0.0–1.0 | 0.0 |
| Filter B Drive | filtBDrive | 0.0–1.0 | 0.0 |
| Filter B KeyTrack | filtBKeyTrack | 0.0–1.0 | 0.0 |
| Filter Routing | filtRouting | Serial/Parallel | Serial |

### Tuning Section

| Parameter | ID | Range | Default |
|-----------|-----|-------|---------|
| Tuning Preset | tuningPreset | Preset index | 0 (12-TET) |
| Tonic | tonic | C–B (0–11) | 0 (C) |
| Master Tune | masterTune | 420–460 Hz | 440 |
| Octave Stretch | octaveStretch | 0.95–1.25 | 1.0 |
| Pitch Bend Range | pitchBendRange | 1–48 semitones | 2 |
| Glide Mode | glideMode | Off/Legato/Always | Off |
| Glide Time | glideTime | 0.001–5.0s | 0.1 |

### Effects Section

| Parameter | ID | Range | Default |
|-----------|-----|-------|---------|
| Reverb Size | reverbSize | 0.0–1.0 | 0.5 |
| Reverb Damping | reverbDamp | 0.0–1.0 | 0.5 |
| Reverb Pre-delay | reverbPredelay | 0–200ms | 20 |
| Reverb Mix | reverbMix | 0.0–1.0 | 0.0 |
| Delay Time | delayTime | 0.001–2.0s | 0.375 |
| Delay Feedback | delayFeedback | 0.0–0.95 | 0.3 |
| Delay Sync | delaySync | On/Off | Off |
| Delay Mode | delayMode | Normal/PingPong | Normal |
| Delay Mix | delayMix | 0.0–1.0 | 0.0 |
| Chorus Rate | chorusRate | 0.1–10.0 Hz | 1.0 |
| Chorus Depth | chorusDepth | 0.0–1.0 | 0.5 |
| Chorus Mix | chorusMix | 0.0–1.0 | 0.0 |
| Distortion Type | distType | SoftClip/HardClip/Tube/Fold | SoftClip |
| Distortion Drive | distDrive | 0.0–1.0 | 0.0 |
| Distortion Mix | distMix | 0.0–1.0 | 0.0 |
| EQ Low Gain | eqLowGain | -12–+12 dB | 0.0 |
| EQ Mid Gain | eqMidGain | -12–+12 dB | 0.0 |
| EQ Mid Freq | eqMidFreq | 200–8000 Hz | 1000 |
| EQ High Gain | eqHighGain | -12–+12 dB | 0.0 |

### Global

| Parameter | ID | Range | Default |
|-----------|-----|-------|---------|
| Master Volume | masterVol | 0.0–1.0 | 0.8 |
| Osc Mix | oscMix | 0.0–1.0 | 0.5 |
| Polyphony | polyphony | 1–16 | 16 |

---

## UI/UX Design

### Window & Layout
- **Size:** 1200 x 800 pixels
- **Technology:** WebView (HTML/CSS/JS)
- **Aesthetic:** Ouaricon Naturalist brand (ouaricon-naturalist-001)

### Layout Architecture

The UI is organized into distinct sections to manage the large parameter count while maintaining the Ouaricon brand elegance:

```
+================================================================+
|  [Botanical]    O - P R I S M       Ouaricon Audio     [Menu]  |
|================================================================|
|                                                                 |
|  ┌──── OSC A ─────────────┐  ┌──── OSC B ─────────────┐       |
|  │ [Wavetable Display]     │  │ [Wavetable Display]     │       |
|  │ Position ──●──          │  │ Position ──●──          │       |
|  │ Coarse  Fine  Phase     │  │ Coarse  Fine  Phase     │       |
|  │ Unison  Detune  Width   │  │ Unison  Detune  Width   │       |
|  │ Level ──●── Pan ──●──   │  │ Level ──●── Pan ──●──   │       |
|  └─────────────────────────┘  └─────────────────────────┘       |
|                                                                 |
|  ┌─ SUB ──────┐  ┌─ NOISE ───┐  ┌─── OSC MIX ────────────┐   |
|  │ Shape  Oct  │  │ Type      │  │ A ──────●────── B       │   |
|  │ Level ──●── │  │ Level──●──│  └─────────────────────────┘   |
|  └─────────────┘  └───────────┘                                 |
|                                                                 |
|  ┌── FILTER A ────────────┐  ┌── FILTER B ────────────┐       |
|  │ Type [LP24 ▼]          │  │ Type [LP24 ▼]          │       |
|  │ Cutoff ──●── Res ──●── │  │ Cutoff ──●── Res ──●── │       |
|  │ Drive ──●── Key ──●──  │  │ Drive ──●── Key ──●──  │       |
|  └─────────────────────────┘  └─────────────────────────┘       |
|  Routing: [Serial] [Parallel]                                   |
|                                                                 |
|  ┌── AMP ENV ─────────┐  ┌── FILTER ENV ──────────────┐       |
|  │ A ──●── D ──●──    │  │ A ──●── D ──●── Depth──●── │       |
|  │ S ──●── R ──●──    │  │ S ──●── R ──●──            │       |
|  └─────────────────────┘  └────────────────────────────┘       |
|                                                                 |
|  ┌── EFFECTS ──────────────────────────────────────────┐       |
|  │ [Reverb] [Delay] [Chorus] [Distortion] [EQ]        │       |
|  │ (Selected effect controls shown here)               │       |
|  └─────────────────────────────────────────────────────┘       |
|                                                                 |
|  ┌── TUNING ───────────────────────────────────────────┐       |
|  │ Preset: [12-TET ▼]  Tonic: [C ▼]  Master: 440 Hz  │       |
|  │ Stretch ──●──  Glide: [Off ▼]  Time ──●──          │       |
|  │ [Load .scl]  [Load .kbm]                           │       |
|  └─────────────────────────────────────────────────────┘       |
|                                                                 |
|  Master ──●──                              [Botanical Image]   |
+================================================================+
```

### Wavetable Display

Each oscillator features a real-time wavetable visualization:
- 3D wireframe view of wavetable frames (classic Serum-style)
- Current position highlighted with accent color
- Waveform of current frame drawn in real-time
- Styled with Ouaricon earth tones (warm amber wireframe on aged paper)

### Botanical Image Selection

**Recommended:** Butterfly specimen (insects category)
- Wings suggest spectrum/prism light refraction
- Geometric wing patterns echo wavetable frame visualization
- Colorful yet elegant — fits the "Prism" identity
- Delicate, precise — reflects the precision of microtonal tuning

**Alternative:** Flora with vibrant coloring — suggests the full spectrum of tuning systems

### Aesthetic Adaptation Notes

The Ouaricon Naturalist aesthetic at 1200x800 with 60+ parameters requires adaptation:
- Smaller knob size (45-50px) for density while maintaining seed cross-section design
- Tighter spacing (15-20px) between controls within sections
- Clear section borders (aged paper panels with brown borders) to organize hierarchy
- Section headers in Garamond uppercase with wide letter-spacing
- Botanical overlay at reduced opacity (0.25) given the visual complexity
- Wavetable displays styled as "specimen illustrations" with aged-paper backgrounds and brown borders

---

## Architecture Notes

### DSP Engine

**Wavetable Engine:**
- Pre-loaded wavetable data in memory (2048 samples x 256 frames per table)
- Bandlimited interpolation between frames (cubic or higher)
- Anti-aliasing via oversampling (2x) and BLIT/polyBLEP for sub oscillator
- Unison voice spreading with per-voice detuning and stereo placement

**Microtonal Integration:**
- TuningEngine class ported directly from O-Lyrica/O-Bells
- Connected identically: `tuningEngine->getFrequency(midiNoteNumber)` replaces standard pitch calculation
- All wavetable playback speed derived from tuning engine frequency
- Pitch bend, glide, and portamento all respect active tuning system
- ScaleGenerator and EmbeddedTunings classes included for factory presets

**Signal Flow:**
```
[Osc A] ──┐
           ├──→ [Mix] ──→ [Filter A] ──→ [Filter B] ──→ [Effects] ──→ [Master]
[Osc B] ──┘                                                              ↑
[Sub]   ──────────────────────────────────────────────────────────────────┘
[Noise] ──→ [Filter routing] ──→ ...
```

**Voice Architecture:**
- Per-voice: 2 wavetable oscillators + sub + noise + 2 filters + 2 ADSR envelopes
- Global: Effects chain + master volume
- Voice stealing: Oldest-note-first with quick release fade (5ms)

### File Format Support

**Wavetable Import:**
- WAV files: Auto-split into 2048-sample frames
- Single-cycle: 2048 samples = 1 frame table
- Multi-frame: N * 2048 samples = N frame table
- Resampling: If sample count isn't multiple of 2048, resample to fit

### Code Reuse from Existing Plugins

**Direct Port (from O-Lyrica/O-Bells):**
- `TuningEngine.h/cpp` — Core frequency calculation
- `ScaleGenerator.h/cpp` — EDO, harmonic, rank-2 generators
- `EmbeddedTunings.h/cpp` — Factory tuning library
- `TuningExporter.h/cpp` — Documentation export (optional)

**Potential Module Extraction:**
- The tuning engine is currently duplicated between O-Lyrica and O-Bells
- O-Prism could be the catalyst to extract it into a shared Ouaricon module
- Would benefit all three plugins with unified updates

---

## Research Requirements

Before implementation, investigate:
- Wavetable synthesis anti-aliasing techniques (BLIT, polyBLEP, oversampling)
- Efficient wavetable interpolation (cubic, hermite, sinc)
- Bandlimited wavetable generation from arbitrary audio
- Multi-mode filter implementations (SVF, Chamberlin, ladder)
- Serum/Vital wavetable file format for potential compatibility
- Unison voice spreading algorithms and CPU optimization
- WebView performance for real-time wavetable visualization (Canvas/WebGL)

## Preset Categories (v1.0)

1. **Leads** — Aggressive, cutting, melodic lead sounds
2. **Pads** — Evolving, lush, atmospheric pad textures
3. **Bass** — Deep, powerful, sub-heavy bass sounds
4. **Keys** — Plucked, bell-like, keyboard textures
5. **FX** — Risers, falls, sweeps, one-shots
6. **Microtonal** — Showcasing non-12-TET tunings (19-EDO pads, just intonation leads, Bohlen-Pierce textures)

## Success Criteria

- [ ] Wavetable morphing sounds smooth and artifact-free
- [ ] All 24+ factory tunings produce correct pitch across full MIDI range
- [ ] Scala file import works for scales of any size (7 to 53+ degrees)
- [ ] CPU usage stays under 25% at 16 voices with unison on modern hardware
- [ ] Dual filters with serial/parallel routing produce expected timbral results
- [ ] All 5 effects are usable and musical
- [ ] Audio file import correctly splits into wavetable frames
- [ ] WebView UI renders at 60fps with wavetable visualization
- [ ] Ouaricon Naturalist aesthetic maintained at 1200x800 with 60+ parameters
- [ ] Competitive sound quality with Serum/Vital in blind A/B testing

---

## Reference Synths

- **Xfer Serum** — Industry standard wavetable synth, benchmark for sound quality and workflow
- **Vital** — Open-source wavetable synth, modern UI, strong modulation
- **Pigments (Arturia)** — Wavetable + virtual analog + granular, polished UI
- **Phase Plant (Kilohearts)** — Modular routing, clean sound, innovative interface

## Complexity Assessment

**Estimated complexity: Very High (Tier 4)**

This is the most complex plugin in the Ouaricon catalog. Key challenges:
- Wavetable engine with bandlimited interpolation (novel DSP — not in any existing plugin)
- 60+ parameters requiring careful UI organization
- Real-time wavetable visualization in WebView
- Dual filter architecture with routing options
- 5-effect chain
- 16-voice polyphony with unison (up to 128 simultaneous oscillators)
- Microtonal engine integration (proven but requires careful pitch→playback-speed mapping)

---

*Created: 2026-02-16*
*Status: Ideated*
*Inspiration: Xfer Serum + O-Lyrica/O-Bells microtonal engine*
