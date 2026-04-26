# O-Contrabass - Creative Brief

## Overview

**Type:** Synth (Physical Modeling Bowed Bass)
**Core Concept:** Specialized 4-string contrabass physical model purpose-built for sustained orchestral arco and ambient drone — deep wood body resonance, bow grit, slow expressive attack. Not a general-purpose synth: every DSP decision is optimized for the bass register (E1-G3) and long-form sustained content.
**Status:** Ideated
**Created:** 2026-04-25

## Vision

O-Bowed already covers general-purpose bowed string synthesis with a Double Bass preset. O-Contrabass goes the other direction: **deep specialization**. The signal path, friction model tuning, body resonator, and string parameters are all dialed in for one job — convincing, sustained, low-register bowed bass that lives equally well in an orchestral mockup and a drone composition.

The design philosophy is **two complementary territories on one instrument:**

- **Cinematic orchestral bass** — Spitfire Albion / CSS / VSL-class polished sustains, slow swells, full-bodied wood resonance for film scores and orchestral mockups.
- **Sustained drone instrument** — Stephen O'Malley / Tony Conrad / Charlemagne Palestine territory, with infinite sustain, sub-harmonic content, slow bow modulation, and per-string detuning for just-intoned drones.

The same engine serves both. Macro controls and presets pull the instrument toward either pole.

**Key differentiators (vs O-Bowed and competitors):**
- Bass-only DSP — low-frequency waveguide stability, bass-range body modes, thick-string friction tuning
- First-class drone features (infinite sustain, sub-harmonics, slow bow LFO, per-string detuning) not bolted on as "impossible physics" but designed in
- Deep wood body resonator tuned specifically to contrabass acoustics (no morphable material — focused, not general)
- Layered expression model: intrinsic CC + dedicated vibrato section + single macro knob
- Full Ouaricon microtonal convention (VST3 Note Expression, MTS-ESP, Scala/TUN, MPE)

## Signal Flow

```
MIDI/MPE/NoteExpression
  -> [Bow Model + Vibrato + Slow LFO]
  -> [Nonlinear Friction Junction] <-> [4x String Waveguides (E-A-D-G)]
  -> [Sub-Harmonic Generator]
  -> [Bridge Filter]
  -> [Bass-Tuned Wood Body Resonator]
  -> [Bow Noise Generator]
  -> [Stereo Width]
  -> Output
```

## Sonic Targets

| Quality | Why It Matters |
|---------|----------------|
| Deep wood body resonance | Defines the "convincing bass" identity — orchestral cinematic reference |
| Audible bow noise / rosin grit | Intimate, close-mic'd realism — separates from polished sample libraries |
| Slow expressive attack | Long bow-on-string transient enables meditative orchestral and ambient contexts |

## Parameters

### Bow Controls

| Parameter | Range | Default | Description | MIDI Map |
|-----------|-------|---------|-------------|----------|
| Bow Speed | 0.02-1.5 m/s | 0.15 | Bow velocity (slower default for bass) | CC11/Expression |
| Bow Pressure | 0.05-8.0 N | 1.0 | Normal force (higher default for thick strings) | CC2/Breath/Aftertouch |
| Bow Position (beta) | 0.02-0.25 | 0.10 | Contact point — sul ponticello to sul tasto (closer to bridge for richness) | CC74/MPE Y |
| Rosin | 0-100% | 65% | Friction curve grip (higher default for bass rosin) | — |
| Bow Noise | 0-100% | 35% | Bow hair / contact noise level | — |

### Body Controls (Bass-Tuned Wood)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Body Size | 0-100% | 75% | Resonant frequency scaling within bass range (small bass to full 3/4 to 7/8) |
| Body Damping | 0-100% | 40% | Body mode decay — low for sustain, high for tighter attack |
| Brightness | 80-12000 Hz | 4500 Hz | Bridge filter cutoff |
| Body Mix | 0-100% | 80% | Wet/dry of body resonator vs raw string |

### String Configuration (4-String E-A-D-G)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| String Tension | 0-100% | 50% | Affects timbre and stiffness (thick bass strings) |
| String Stiffness | 0-100% | 30% | Inharmonicity — low for ideal string, higher for character |
| Per-String Detune (E/A/D/G) | +/- 1200 cents | 0 | Independent pitch offset per string for scordatura / drone tunings |
| Active Strings | 1-4 | 4 | Number of strings the bow can engage |

### Expression

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Vibrato Rate | 0.1-12 Hz | 5 Hz | Dedicated vibrato section |
| Vibrato Depth | 0-50 cents | 12 cents | Vibrato pitch deviation |
| Vibrato Onset | 0-3000 ms | 600 ms | Delay before vibrato fades in (orchestral realism) |
| Slow Bow LFO Rate | 0.05-2 Hz | 0.3 Hz | Drone modulation — slow swell rate |
| Slow Bow LFO Depth | 0-100% | 0% | Modulates bow speed/pressure for evolving drones |
| Expression Macro | 0-100% | 50% | Single-knob layered expression: bow speed + pressure + vibrato + body brightness |

### Drone Features

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Infinite Sustain | 0-100% | 0% | Reduces damping toward zero — endless resonance, drone-first feature |
| Sub-Harmonics | 0-100% | 0% | Nonlinear feedback / sub-octave content for sub-bass extension |

### Output

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Width | 0-200% | 100% | Stereo spread |
| Output Level | -inf to +12 dB | 0 dB | Master output gain |

### Microtonal Tuning (Ouaricon Convention)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Reference Pitch | 220-880 Hz | 440 Hz | A4 reference frequency |
| Tuning System | File / MTS-ESP / 12-TET | 12-TET | Scala/TUN import, MTS-ESP real-time retuning, or standard |
| Note Expression | enabled | on | VST3 Note Expression for Dorico microtonal playback |

## Bow Behavior

**Sustained-first hybrid model:** Bow remains engaged while MIDI note held, with release tail on note-off. The default articulation is **legato sustained bowing** — long, expressive, connected. Velocity controls initial attack character; CC11 controls dynamic shape over the held note.

**MPE / Note Expression** drives per-note pitch deviation, pressure (Z), and bow position (Y) — full Dorico-compatible expression for notation-driven workflows.

## Architecture

- **Monophonic** (authentic single-string playing — bow engages one string at a time)
- **4 active waveguide strings** at standard EADG fourths tuning (E1-G3 range), individually detunable
- **Bass-optimized friction junction** — friction model parameters tuned for thick rosined hair on bass strings
- **2x internal oversampling** at the friction junction for stability at low fundamentals
- **Zero algorithmic latency** (waveguide is causal)
- **MPE + VST3 Note Expression** support from day one
- **Bass-tuned body resonator** — fixed wood material, parametric size/damping (no morphable material)

## Performance Targets

| Metric | Target |
|--------|--------|
| CPU (typical settings) | <5% on modern laptop |
| Oversampling | 2x at friction junction |
| Latency | 0 samples (causal waveguide) |
| Polyphony | 1 voice (mono) |

## Instrument Presets

**Orchestral:**
- Cinematic Bass Sustain (default — full body, slow attack, long vibrato onset)
- Section Bass (slightly damped, moderate vibrato — for orchestral section feel)
- Solo Arco Bass (intimate close-mic, rosin grit, expressive vibrato)
- Pianissimo Bass (low pressure, slow speed, minimal vibrato — sul tasto-leaning)
- Forte Bass (high pressure, faster bow, prominent body)

**Drone / Experimental:**
- Infinite Drone (max infinite sustain, slow bow LFO, sub-harmonics on)
- Just-Intoned Drone (per-string detuning to 7-limit just intervals)
- Scordatura Bass (alternative tuning preset — e.g. dropped strings)
- Sub Drone (heavy sub-harmonic content, slow modulation)
- Dark Pad Bass (high body damping, low brightness, slow bow swell)

## Use Cases

- **Orchestral mockup** — sustained bowed bass lines in film, TV, game scoring
- **Cinematic pads & beds** — long held tones under orchestral textures
- **Ambient music** — evolving bass drones with bow modulation and sub-harmonics
- **Experimental drone** — La Monte Young / Tony Conrad style just-intoned bowed drones
- **Microtonal composition** — full Scala/TUN/MTS-ESP support; Dorico Note Expression playback
- **Live performance** — MPE controllers (Linnstrument, Seaboard) for expressive real-time bowing

## Inspirations

**Orchestral bass libraries (sonic reference for "convincing"):**
- Spitfire Albion series — polished cinematic bowed bass
- Cinematic Studio Strings — section bass legato character
- VSL Bass Section — clean orchestral reference

**Drone / experimental (sonic reference for "sustained territory"):**
- Stephen O'Malley (SunnO))) — sustained bowed bass density
- Tony Conrad — just-intoned sustained string drones
- Charlemagne Palestine — slow bow modulation, harmonic exploration
- La Monte Young / Theatre of Eternal Music — drone tuning systems

**Existing PM bass plugins (reference for current state-of-art):**
- SWAM Double Bass — what realistic PM bass currently sounds like
- AAS String Studio bass presets — bowed string as synthesis philosophy

## UI Concept

**Layout:** TBD in mockup phase. Expected groupings:
- Bow section (speed, pressure, position, rosin, noise)
- Body section (size, damping, brightness, mix)
- Strings (tension, stiffness, per-string detune, active count)
- Expression (vibrato, slow LFO, expression macro)
- Drone (infinite sustain, sub-harmonics)
- Output / Microtonal

**Visual Style:** TBD in mockup phase. Suggest: dark wood / stage-lit aesthetic to match cinematic + drone dual identity.

## Technical Notes

- Bass-range waveguide stability is non-trivial — long delay lines + nonlinear feedback at low fundamentals require careful interpolation and damping design
- Friction model can likely reuse / specialize O-Bowed's friction module if extracted as a shared module — opportunity for module extraction during implementation
- VST3 Note Expression pattern is documented in the spike-findings skill (applicable to all pitched plugins)
- Body resonator design is the second-most-important sonic element after friction — invest in convincing wood body modes
- Sub-harmonic generator should be musical (not just frequency division) — likely nonlinear feedback in the waveguide

## Differentiation from O-Bowed

| Dimension | O-Bowed | O-Contrabass |
|-----------|---------|--------------|
| Scope | General-purpose bowed string synth (violin -> bass -> erhu) | Bass-only (E1-G3), single instrument family |
| Body | Morphable material (membrane / wood / metal / glass) | Fixed wood, bass-tuned size/damping |
| Strings | Configurable 1-4 + sympathetic | Fixed 4-string EADG, no sympathetic |
| Use case priority | Realism + sound design across all bowed instruments | Sustained orchestral arco + drone |
| DSP tuning | Generalized across pitch/string ranges | Specialized for low fundamentals |
| Drone features | Listed under "Impossible Physics" (optional) | First-class design pillar |
| Friction tuning | Generic across string types | Tuned for thick rosined bass strings |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Pizzicato | Bow-only physical model — would require separate excitation engine | v1.1+ |
| Col legno / harmonics articulations | Specialized excitation modes | v1.2+ |
| Polyphony / double-stops | Mono-by-design (authentic bowing) | Possibly v2.0 |
| 5-string / B0 extension | Standard 4-string EADG only | v1.1+ |
| Sympathetic strings | Out of scope to keep focus | v1.1+ |

## Next Steps

- [ ] Stage 0: Planning — research DSP approach and create architecture (`/plan O-Contrabass`)
- [ ] Create UI mockup (`/start O-Contrabass` -> option 3)
- [ ] Start implementation (`/implement O-Contrabass`)
