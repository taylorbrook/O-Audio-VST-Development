# O-Emulator - Creative Brief

## Overview

**Type:** Effect
**Core Concept:** Authentic retro console audio emulation — run any audio through the real codec, resampler, and output-stage behavior of classic game systems (SNES, PS1, NES, Game Boy, Genesis).
**Status:** 💡 Ideated
**Created:** 2026-08-20

## Vision

O-Emulator makes any signal sound like it is being played back by classic game hardware — not by approximating with generic bitcrush and filtering, but by **actually running audio through faithful implementations of each console's audio pipeline**: the codec encode→decode round-trip, the console's true sample rate and interpolation, and its DAC/output-stage coloration.

The user picks a console, then shapes the character with a small set of macro knobs. The result should be instantly recognizable to anyone who grew up with these systems: the dark, watery warmth of SNES BRR compression, the crunchy ADPCM grit and murky reverb of PS1-era games, the brutal quantization of NES DPCM and Game Boy wave channels, and the crusty low-rate sample playback of the Genesis YM2612 DAC.

Strict authenticity where it matters (codecs, rates, filters), creative freedom where it's fun (the console reverb is available in every mode, not just PS1).

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Console | SNES / PS1 / NES / Game Boy / Genesis | SNES | Selects the emulated system: codec, fixed sample rate, interpolation, and output-stage model |
| Crush | 0–100% | 50% | Codec intensity — how hard the console's compression/quantization artifacts hit, from subtle color to fully degraded |
| Age | 0–100% | 20% | Hardware condition — noise floor, electrical hum, filter dulling, and drift increase with age |
| Reverb | 0–100% | 0% | PS1/N64-style SPU reverb send — the murky short game reverb, available in every console mode |
| Mix | 0–100% | 100% | Parallel dry/wet blend of processed vs. clean signal |

**Sample rate handling:** Authentic fixed rates per console (e.g. SNES 32 kHz, PS1 SPU-domain rates, Genesis ~26 kHz DAC playback). No sample-rate knob — the console choice IS the rate.

## UI Concept

**Layout:** Console selector as the focal element, flanked by the four macro character knobs (Crush, Age, Reverb, Mix).
**Visual Style:** TBD in mockup phase.
**Key Elements:** Console selector, 4 macro knobs.

## Use Cases

- **Lo-fi / hip-hop producers** — texture and vibe on melodic loops, drums, or whole mixes
- **Chiptune / VGM artists** — authentic-sounding retro game music from modern DAWs
- **Game audio designers** — previewing/rendering assets as they'd sound on retro hardware
- **Sound designers / electronic musicians** — creative mangling: crunchy textures, transitions, ear candy

## Inspirations

- **SNES (S-DSP):** BRR 4-bit ADPCM compression, 32 kHz, 4-tap Gaussian interpolation (the characteristic dark rolloff)
- **PS1 (SPU):** SPU-ADPCM 4-bit compression, era-typical low source rates, the famous SPU reverb algorithm
- **NES (2A03) / Game Boy:** DPCM delta quantization / 4-bit wave-channel grit
- **Genesis (YM2612):** 8-bit DAC sample playback at low rates, DAC ladder-effect crossover distortion
- Sonic reference: 16/32-bit era game soundtracks — Secret of Mana, Final Fantasy VII, Castlevania: Symphony of the Night

## Technical Notes

- **Authentic codec emulation is the core differentiator:** audio is genuinely encoded and decoded through each console's codec (BRR encode→decode for SNES, SPU-ADPCM round-trip for PS1, DPCM for NES, 4-bit wave quantization for Game Boy, 8-bit low-rate playback for Genesis), not perceptually approximated.
- **Per-console fixed internal rates** with proper resampling in/out of the console domain; each console's true interpolation mode (e.g. SNES Gaussian) is part of the model.
- **Console output stage modeled per system:** output lowpass/DAC filter, DAC nonlinearity, and headroom clipping.
- **SPU-style reverb** implemented once, routable in all modes (creative choice over strict authenticity).
- **Age knob** drives a hardware-condition model: noise floor, hum, filter dulling, drift.
- **Crush knob** scales codec artifact intensity — likely a blend/drive into the codec domain so 0% ≈ transparent-ish and 100% = fully committed to the console pipeline.

## Next Steps

- [ ] Create UI mockup (`/start O-Emulator` → option 3)
- [ ] Start implementation (`/implement O-Emulator`)
