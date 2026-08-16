# O-Bitrot - Creative Brief

## Overview

**Type:** Effect
**Core Concept:** Broken-media degradation box — a clocked stochastic state machine over a shared circular buffer that emulates failing playback hardware: tape bends, CD skips, vinyl jumps, packet loss, telecom codecs, and bitcrush.
**Status:** 💡 Ideated
**Created:** 2026-08-14

## Vision

O-Bitrot makes audio sound like it is being played back by dying media — not an LFO-modulated "glitch effect," but a machine that *fails* on a clock. A single continuously-written circular buffer feeds per-channel variable-rate read heads; on each clock tick the state machine probabilistically re-rolls what kind of failure is happening (the RSBrokenMedia pattern). Six degradation families, each with its own explicit control section, cover the full physical-to-digital spectrum of broken playback:

1. **Tape** — speed bends from a musical-interval table (`{1.0, 0.67, 1.5, 0.5, 2.0}`), tape-stops, reverse; all transitions through ~150 ms ramps (the ramp IS the glide sound and the click-safety).
2. **CD skip** — the CIRC failure ladder: interpolation dullness (brief LPF dip) → ms mutes with residual tick → the machine-gun buffer loop with a synthesized chirp at each segment restart, then a jump forward on recovery.
3. **Vinyl** — revolution-quantized read-pointer jumps (1.8 s @ 33⅓, 1.33 s @ 45) with synthesized pops; locked-groove loops. Pitch never changes — that's what makes it vinyl.
4. **Packet loss** — Gilbert–Elliott 2-state Markov bursts over 20 ms packets, four concealment modes (silence / robotic repeat / decaying repeat / waveform substitution).
5. **Codec** — the real telephone/cellphone chain: mono sum → 300–3400 Hz bandpass → 8 kHz downsample → μ-law round trip (landline) or GSM 06.10 via vendored libgsm (cellphone).
6. **Crush** — DeRez-style sweepable sample-rate reduction (fractional-crossing interpolated hold, jitter) + fractional-bit quantization (mid-tread/riser, TPDF dither) with envelope-driven dynamic bit depth (Digitalis-style duck/pump polarities).

Chaos stays *rhythmic* because glitch decisions are quantized to a clock — tempo-synced by default, free-running for authentic "the deck is dying" feel. Randomness is fully seeded: the same seed renders the same bounce every time, and a reseed dice button rolls new variations.

## Parameters

Per-module control sections (explicit enable + parameters per family), not macro dice knobs. Representative set — Stage 0 planning will finalize:

### Global

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| clockMode | Sync / Free | Sync | Tempo-synced divisions vs free-running rate |
| clockRate | 1/16 – 1 bar (Sync); 0.1–20 Hz (Free) | 1/8 | How often the failure state re-rolls |
| seed | 0–9999 + reseed trigger | saved in state | Deterministic randomness; same seed = same bounce |
| hardEdges | Off / On | Off | Bypass jump crossfades for deliberate clicks |
| mix | 0–100% | 100% | Dry/wet |

### Tape

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| tapeEnable | Off/On | On | Section enable |
| tapeProb | 0–100% | 25% | Probability of a tape event per clock tick |
| tapeStopProb | 0–100% | 10% | Share of tape events that are full stops |
| tapeRamp | 20–500 ms | 150 | Speed-transition ramp time (the glide sound) |

### CD Skip

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| cdEnable | Off/On | On | Section enable |
| cdProb | 0–100% | 25% | Probability of a skip event per tick |
| cdSeverity | Conceal → Mute → Loop | continuous | Position on the CIRC failure ladder |
| cdSegment | 10–400 ms | 100 | Repeated-segment length |

### Vinyl

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| vinylEnable | Off/On | On | Section enable |
| vinylProb | 0–100% | 25% | Probability of a groove jump per tick |
| vinylRPM | 33⅓ / 45 | 33⅓ | Revolution quantum for jumps/locked grooves |
| vinylPop | 0–100% | 50% | Synthesized pop level |

### Packet Loss

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| packetEnable | Off/On | Off | Section enable |
| packetLoss | 0–100% | 20% | Overall loss intensity (drives Markov state probabilities) |
| packetBurst | 0–100% | 30% | Burstiness (P(Good→Bad) / burst length) |
| packetConceal | Silence / Repeat / Decay / Substitute | Decay | Concealment mode |

### Codec

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| codecEnable | Off/On | Off | Section enable |
| codecMode | μ-law / GSM | μ-law | Landline vs cellphone |
| codecMix | 0–100% | 100% | Codec chain blend |

### Crush

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| crushEnable | Off/On | Off | Section enable |
| crushBits | 1–16 (fractional) | 16 | Quantization depth, continuously sweepable |
| crushRate | 500 Hz – fs (fractional) | fs | Sample-rate reduction, interpolated hold |
| crushJitter | 0–100% | 0% | S&H clock jitter (Decimort-style hash) |
| crushEnvAmt | −100…+100% | 0% | Envelope-driven dynamic bit depth; − = duck (tails crush), + = pump (transients splatter) |
| crushDither | 0–2 LSB | 0 | TPDF dither: grit ↔ hiss morph |

## UI Concept

**Layout:** Finalized in mockup v1 (2026-08-15): fixed 900×620; 3×2 chain-ordered specimen-plate grid (Tab. I–VI: Tape / CD Skip / Vinyl top, Packet / Codec / Crush bottom) + Tab. VII global strip (clock, seed ledger + bone-die reseed, Hard Edges, Mix).
**Visual Style:** Ouaricon Naturalist (`ouaricon-naturalist-001`) — aged paper (clean O-Tremolo texture), walnut/moss palette, small-caps plate captions, ghosted Sowerby *Coprinus comatus* plate (t. 189, public domain, provenance in `Source/ui/public/img/PROVENANCE.md`) behind the right column.
**Key Elements:** Bone-die reseed button + 4-digit seed ledger; Sync/Free segmented toggle swapping division dropdown ↔ free-rate mini knob in a fixed slot; spore-print event LEDs per family panel (moss `#8BA870` active / ~30% brown idle); disabled panels dim to 0.45 with the enable tag still live; μ-law and 33⅓ glyphs rendered in HTML only (host strings stay ASCII).

## Use Cases

- Sound design: turning clean sources into decayed, haunted-media textures (hauntology, vaporwave, William Basinski-style disintegration)
- Beat production: rhythmic CD-skip stutters and tape bends locked to host tempo
- Mix character: codec/crush sections as a lo-fi channel strip (phone vocals, 8-bit drums)
- Reproducible chaos: seeded renders mean a producer can commit a take knowing the bounce matches playback

## Inspirations

- **RSBrokenMedia** (reillypascal, GPL-3.0) — the clocked stochastic architecture, CD-skip machinery, real GSM chain (patterns reference; license check before any code reuse)
- **Airwindows DeRez/DeRez2** (MIT) — interpolated fractional hold, per-sample smoothed targets, μ-law wrap
- **D16 Decimort 2** — full AD/DA-path sim architecture (pre-filter, S&H + jitter, mid-tread/riser, images post-filter)
- **Aberrant DSP Digitalis** — envelope-driven dynamic bitcrush framing
- Sonic references: dying Walkmans, scratched CDs, locked grooves, GSM dropouts, answering machines

## Technical Notes

- Architecture per `research/glitch-effects/degradation-dsp-deep-dive.md` §6: **CrushStage / QuantStage / CodecStage / MediaPlayer** module decomposition over one shared circular buffer + clocked stochastic read heads.
- Anti-zipper rules (§2.5): smooth the rate/step *target* per-sample, never the output; fractional-crossing interpolation; never reset phase accumulators on parameter change; all position jumps crossfaded 1–5 ms unless hardEdges.
- Seeded RNG: one stream per stochastic subsystem (repo pattern: shared audio-thread RNG breaks block-size invariance); seed persisted in APVTS state.
- Envelope follower for dynamic crush runs per-sample (block-rate followers break offline-bounce invariance — repo pattern).
- GSM 06.10 via vendored libgsm (tiny, MIT-style); 8 kHz / 160-sample frames introduce latency → `setLatencySamples()` in `prepareToPlay()`.
- Circular buffer sizing must cover the largest vinyl revolution quantum (1.8 s @ 33⅓) plus ramp headroom — static_assert the span.
- GPL caution: RSBrokenMedia is read-for-patterns only; verify AGPL-3.0 ↔ GPL-3.0 compatibility before incorporating any code.

## Next Steps

- [ ] Create UI mockup (`/start O-Bitrot` → option 3)
- [ ] Start implementation (`/implement O-Bitrot`)
