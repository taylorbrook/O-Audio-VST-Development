# O-simpleSubtractive — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleSubtractive
created: 2026-06-25
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are likely additions Stage 0 should confirm and fold into the final spec.

## Oscillator / Mixer

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Osc Waveform | `oscWave` | choice | sawtooth / square / triangle / sine | sawtooth | — | The harmonically rich source. Saw = all harmonics (brightest); square = odd only (hollow); triangle = odd, fast rolloff (dark); sine = single partial (contrast). Anti-aliased (PolyBLEP/BLEP-tables). |
| Sub Osc Level | `subLevel` | float | 0–100 | 0 | % | Octave-down oscillator mixed under the main osc for bass weight. |
| Noise Level | `noiseLevel` | float | 0–100 | 0 | % | White-noise source mixed in — the filtered-noise lesson (wind/breath/percussion). |

## Filter

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Filter Type | `filterType` | choice | LP / HP / BP / Notch | LP | — | Response shape. LP removes highs (the workhorse); HP removes lows; BP keeps a band; Notch removes a band. |
| Filter Slope | `filterSlope` | choice | 6 / 12 / 24 dB/oct (1/2/4-pole) | 24 dB/oct | — | Steepness of rejection past the cutoff. 24 dB/oct = classic 4-pole. Steeper reads darker at the same cutoff. |
| Cutoff | `cutoff` | float | 20–20000 | 2000 | Hz (log) | Frequency where the filter begins to act. Lowering it darkens the tone. THE defining subtractive control. |
| Resonance (Q) | `resonance` | float | 0–100 | 10 | % | Emphasizes a band at the cutoff; high settings build the sweep whistle and approach self-oscillation. |
| Filter Env Amount | `filterEnvAmount` | float | -100–+100 | +50 | % | Bipolar depth of the filter envelope routed to cutoff. Positive opens then closes; negative inverts. |

## Filter Envelope (ADSR → cutoff via env amount)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Filter Attack | `filterAttack` | float | 0–5 | 0.005 | s |
| Filter Decay | `filterDecay` | float | 0–5 | 0.3 | s |
| Filter Sustain | `filterSustain` | float | 0–100 | 40 | % |
| Filter Release | `filterRelease` | float | 0–5 | 0.2 | s |

## Amplitude Envelope (ADSR → VCA)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Amp Attack | `ampAttack` | float | 0–5 | 0.005 | s |
| Amp Decay | `ampDecay` | float | 0–5 | 0.3 | s |
| Amp Sustain | `ampSustain` | float | 0–100 | 80 | % |
| Amp Release | `ampRelease` | float | 0–5 | 0.1 | s |

## Voice / Output

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Voice Mode | `voiceMode` | choice | Poly / Mono / Legato | Poly | — | 16-voice polyphony, or monophonic (last/legato) for classic lead/bass. |
| Glide (Portamento) | `glide` | float | 0–1 | 0 | s | Pitch-slide time between notes — the mono slide (TB-303). |
| Output Level | `outputLevel` | float | -inf–0 | 0 | dB | Master output gain. |

## Open Research Items (Stage 0 to confirm / fold into final spec)

- **Filter topology** *(research)* — Moog ladder vs zero-delay-feedback TPT/SVF. Must deliver: stable self-oscillation, all four modes (LP/HP/BP/Notch), the 6/12/24 dB slope set, *and* a magnitude curve that can be computed for the headline visual. Likely TPT/SVF for clean multi-mode + correct curves, with cascading for 24 dB; evaluate ladder for 24 dB self-osc authenticity.
- **Cutoff key-tracking** *(research, DSP-05)* — cutoff follows pitch so a self-oscillating filter plays in tune. Possible `keyTrack` 0–100% param.
- **Velocity routing** *(research)* — to amp level and/or filter env amount. Possible `velToAmp` / `velToFilterEnv` params.
- **Pulse-width / PWM** *(research)* — on the square wave; possible `pulseWidth` param. Deferred unless cheap.
- **Self-oscillation gain compensation** *(research, DSP-03)* — keep resonance sweeps from blowing up level.
- **Resonance taper / self-oscillation threshold** *(research)* — exact Q mapping and the resonance value at which self-osc begins.
- **Master tune / octave** *(research)* — possible global tune control.

## Deferred to v1.1+ (per BRIEF "Out of Scope")

- Second full oscillator + detune / supersaw
- Modulation matrix / multiple LFOs / dedicated vibrato LFO
- Effects (reverb/delay/chorus)
- Unison / arpeggiator / step sequencer

---
*Draft generated from BRIEF.md on 2026-06-25. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
