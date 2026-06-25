# O-simpleSampler — Parameter Specification (DRAFT)

---
version: 0.1.0-draft
plugin: O-simpleSampler
created: 2026-06-25
source: BRIEF.md parameter table
status: draft (full parameter-spec.md required before Stage 1 — produced by mockup finalization)
---

> **DRAFT** — extracted from BRIEF.md for Stage 0 complexity/architecture planning.
> Ranges are starting proposals to be validated by research. Items marked *(research)*
> are likely additions Stage 0 should confirm and fold into the final spec.

## Source

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Source Sample | `sourceSample` | choice + load | curated built-ins + Load… | (first built-in) | — | Which recording is played; **Load…** drags/picks a user file. THE raw material. Built-ins embedded as binary data; load-your-own buffered off the audio thread (macOS WebView content-streaming drag-drop + file-picker fallback). |

## Region (Start/End · Loop · Reverse)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Start | `start` | float | 0–100 | 0 | % of source | Start of the played region — isolates the useful part of the recording. |
| End | `end` | float | 0–100 | 100 | % of source | End of the played region. Start/End together are the "isolate the useful region" lesson. |
| Loop Mode | `loopMode` | choice | off / forward / ping-pong | off | — | Whether and how the region loops so a short sound sustains as a held note. |
| Loop Start | `loopStart` | float | 0–100 | 0 | % of region | Start of the loop within the region. |
| Loop End | `loopEnd` | float | 0–100 | 100 | % of region | End of the loop within the region. |
| Loop Crossfade | `loopCrossfade` | float | 0–500 | 10 | ms | Equal-power crossfade across the loop seam to remove the per-repeat click. |
| Reverse | `reverse` | bool | on / off | off | — | Plays the region backwards (swells, reverse-cymbal transitions). |

## Pitch Engine (Root Key · Repitch/Stretch · Tune)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Root Key | `rootKey` | int (MIDI) | C-1–G9 (0–127) | C3 (60) | MIDI note | The key at which the sample plays at original pitch; makes the recording track the keyboard in tune. |
| Pitch Mode | `pitchMode` | choice | Repitch / Stretch | Repitch | — | Repitch = varispeed (pitch+time coupled, classic-sampler behaviour); Stretch = pitch/time independent. THE pitch/time-independence lesson, as an A/B toggle. |
| Tune | `tune` | int | -24–+24 | 0 | semitones | Coarse transposition of the sample independent of the keyboard. |
| Fine | `fine` | float | -100–+100 | 0 | cents | Fine detune. |

## Vintage

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Vintage | `vintage` | float | 0–100 | 0 | % | One macro: sample-rate decimation (sample-and-hold downsampling) + bit-depth reduction for the SP-1200 lo-fi grit. 0 = bit-for-bit clean. |

## Filter (resonant low-pass)

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Filter Cutoff | `filterCutoff` | float | 20–20000 | 20000 | Hz (log) | Low-pass cutoff — gives the raw recording a tonal contour by removing highs. Open at default. |
| Filter Resonance | `filterResonance` | float | 0–100 | 0 | % | Emphasis at the cutoff. |

## Amplitude Envelope (ADSR → VCA)

| Param | ID | Type | Range | Default | Unit |
|-------|----|------|-------|---------|------|
| Amp Attack | `ampAttack` | float | 0–5 | 0.005 | s |
| Amp Decay | `ampDecay` | float | 0–5 | 0.3 | s |
| Amp Sustain | `ampSustain` | float | 0–100 | 100 | % |
| Amp Release | `ampRelease` | float | 0–5 | 0.2 | s |

## Voice / Output

| Param | ID | Type | Range | Default | Unit | Notes |
|-------|----|------|-------|---------|------|-------|
| Velocity → Amp | `velToAmp` | float | 0–100 | 50 | % | How much note velocity scales loudness (the responsiveness the class describes). |
| Output Level | `outputLevel` | float | -inf–0 | 0 | dB | Master output gain. |

*(Polyphony: proposing 16 voices, sibling-consistent — confirm in research. Not an APVTS parameter.)*

## Open Research Items (Stage 0 to confirm / fold into final spec)

- **Stretch algorithm** *(research, DSP-01)* — reuse the O-simpleGrain granular scheduler vs a phase vocoder. Must read as "same length, different pitch" cleanly enough to teach; characterise the artifact profile. Headline lesson — must be audibly AND visibly distinct from Repitch.
- **Interpolation / anti-aliasing on upward repitch** *(research, DSP-02)* — fractional read with interpolation + band-limiting so high keys stay clean (a teaching tool must not buzz/alias).
- **Loop-crossfade formulation + zero-crossing snap** *(research, DSP-03)* — equal-power crossfade; whether Start/End and loop markers snap to zero-crossings to avoid clicks.
- **Vintage formulation** *(research, DSP-04)* — exact sample-rate decimation + bit-depth quantization curve (mirrors O-simpleAdditive bit-depth lesson); clean at zero, no NaNs/denormals across the range.
- **Key-tracking behaviour in Stretch mode** *(research)* — how pitch maps to key when duration is held.
- **Velocity → filter routing** *(research, optional)* — possible `velToFilter` param.
- **Tune/Fine consolidation** *(research)* — whether to fold coarse Tune + Fine into a single transpose control.
- **Built-in sample set + per-sample default root key** *(research)* — curated found-sounds list, per-sample default root, source-length cap for built-ins and loaded files.
- **Polyphony confirmation + voice-stealing** *(research, DSP-07)* — proposing 16 voices with graceful stealing.

## Deferred to v1.1+ (per BRIEF "Out of Scope")

- Slicing a break into hits / chop-to-pads (future "O-simpleSlicer" sibling)
- Internal step sequencer / pad grid / beat-making
- Multi-zone / velocity-layered multisampling
- Effects (reverb/delay/chorus)
- Tempo-sync / warp-to-grid

---
*Draft generated from BRIEF.md on 2026-06-25. Replace with full parameter-spec.md at mockup finalization before Stage 1.*
