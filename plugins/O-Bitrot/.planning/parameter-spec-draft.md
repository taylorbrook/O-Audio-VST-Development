# O-Bitrot - Parameter Specification (Draft)

## Overview

Broken-media degradation box — a clocked stochastic state machine over a shared circular buffer emulating failing playback hardware. Six degradation families (Tape / CD Skip / Vinyl / Packet Loss / Codec / Crush), each with an explicit per-module control section, plus a global clock/seed/mix strip. Fully seeded randomness for reproducible renders.

Source: extracted from `BRIEF.md` (2026-08-14). Refine into full `parameter-spec.md` after UI mockup.

## Parameters

### Global

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CLOCK_MODE | Clock Mode | Choice | Sync / Free | Sync | - | Tempo-synced divisions vs free-running rate. |
| CLOCK_SYNC_DIV | Clock Division | Choice | 1/16 – 1 bar | 1/8 | - | Re-roll interval in Sync mode (musical divisions). |
| CLOCK_FREE_RATE | Clock Rate | Float | 0.1 - 20.0 | 2.0 | Hz | Re-roll rate in Free mode ("the deck is dying" feel). |
| SEED | Seed | Int | 0 - 9999 | 0 | - | Deterministic randomness; same seed + input + params = identical bounce. Persisted in state. Reseed dice button rolls a new value (UI trigger, not an automatable param). |
| HARD_EDGES | Hard Edges | Bool | Off / On | Off | - | Bypass 1–5 ms jump crossfades for deliberate clicks. |
| MIX | Mix | Float | 0.0 - 100.0 | 100.0 | % | Global dry/wet. |

### Tape

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| TAPE_ENABLE | Tape Enable | Bool | Off / On | On | - | Section enable. |
| TAPE_PROB | Tape Probability | Float | 0.0 - 100.0 | 25.0 | % | Probability of a tape event per clock tick. |
| TAPE_STOP_PROB | Tape-Stop Share | Float | 0.0 - 100.0 | 10.0 | % | Share of tape events that are full stops. |
| TAPE_RAMP | Tape Ramp | Float | 20.0 - 500.0 | 150.0 | ms | Speed-transition ramp time — the ramp IS the glide sound and the click-safety. |

### CD Skip

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CD_ENABLE | CD Enable | Bool | Off / On | On | - | Section enable. |
| CD_PROB | CD Probability | Float | 0.0 - 100.0 | 25.0 | % | Probability of a skip event per tick. |
| CD_SEVERITY | CD Severity | Float | 0.0 - 1.0 | 0.5 | - | Continuous position on the CIRC failure ladder: interpolation/LPF concealment → ms mutes + tick → machine-gun buffer loop with restart chirp. |
| CD_SEGMENT | CD Segment | Float | 10.0 - 400.0 | 100.0 | ms | Repeated-segment length in loop mode. |

### Vinyl

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| VINYL_ENABLE | Vinyl Enable | Bool | Off / On | On | - | Section enable. |
| VINYL_PROB | Vinyl Probability | Float | 0.0 - 100.0 | 25.0 | % | Probability of a groove jump per tick. |
| VINYL_RPM | Vinyl RPM | Choice | 33⅓ / 45 | 33⅓ | - | Revolution quantum for jumps/locked grooves (1.8 s @ 33⅓, 1.33 s @ 45). Pitch never changes. |
| VINYL_POP | Vinyl Pop | Float | 0.0 - 100.0 | 50.0 | % | Synthesized pop level. |

### Packet Loss

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| PACKET_ENABLE | Packet Enable | Bool | Off / On | Off | - | Section enable. |
| PACKET_LOSS | Packet Loss | Float | 0.0 - 100.0 | 20.0 | % | Overall loss intensity (drives Gilbert–Elliott Markov state probabilities over 20 ms packets). |
| PACKET_BURST | Packet Burstiness | Float | 0.0 - 100.0 | 30.0 | % | Burstiness — P(Good→Bad) / burst length. |
| PACKET_CONCEAL | Concealment | Choice | Silence / Repeat / Decay / Substitute | Decay | - | Concealment mode: silence / robotic repeat / decaying repeat / waveform substitution. |

### Codec

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CODEC_ENABLE | Codec Enable | Bool | Off / On | Off | - | Section enable. |
| CODEC_MODE | Codec Mode | Choice | μ-law / GSM | μ-law | - | Landline (mono sum → 300–3400 Hz BP → 8 kHz → μ-law round trip) vs cellphone (GSM 06.10 via vendored libgsm; reports framing latency). |
| CODEC_MIX | Codec Mix | Float | 0.0 - 100.0 | 100.0 | % | Codec chain blend. |

### Crush

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CRUSH_ENABLE | Crush Enable | Bool | Off / On | Off | - | Section enable. |
| CRUSH_BITS | Crush Bits | Float | 1.0 - 16.0 | 16.0 | bits | Fractional-bit quantization depth (mid-tread/riser), continuously sweepable, zipper-free. |
| CRUSH_RATE | Crush Rate | Float | 500.0 - fs | fs | Hz | Sample-rate reduction — fractional-crossing interpolated hold (DeRez-style). Max clamps to host fs. |
| CRUSH_JITTER | Crush Jitter | Float | 0.0 - 100.0 | 0.0 | % | S&H clock jitter (Decimort-style hash). |
| CRUSH_ENV_AMT | Crush Env Amount | Float | -100.0 - 100.0 | 0.0 | % | Envelope-driven dynamic bit depth; − = duck (tails crush), + = pump (transients splatter). Per-sample follower. |
| CRUSH_DITHER | Crush Dither | Float | 0.0 - 2.0 | 0.0 | LSB | TPDF dither: grit ↔ hiss morph. |

## Parameter Count Summary

- Global: 6
- Tape: 4
- CD Skip: 4
- Vinyl: 4
- Packet Loss: 4
- Codec: 3
- Crush: 6
- **Total: 31** (+ reseed UI trigger)

## Design Notes

- **Per-module control sections, no macro dice knobs** — each family has explicit enable + parameters (FUNC-05).
- **Clock split into three params** (mode + sync division + free rate) so both modes have stable automatable values; BRIEF listed a single `clockRate` — Stage 0 finalizes this split.
- **Seeded RNG:** one stream per stochastic subsystem (shared audio-thread RNG breaks block-size invariance — repo pattern). Seed persisted in APVTS state; reseed is a UI-side trigger writing SEED.
- **Anti-zipper rules:** smooth the rate/step *target* per-sample, never the output; fractional-crossing interpolation; never reset phase accumulators on param change; all position jumps crossfaded 1–5 ms unless HARD_EDGES.
- **Defaults:** physical-media families (Tape/CD/Vinyl) on, digital families (Packet/Codec/Crush) off — instant broken-media character without instant lo-fi mangling.
- **Latency:** GSM mode's 8 kHz / 160-sample framing latency reported via `setLatencySamples()` in `prepareToPlay()`.
- **Buffer sizing:** circular buffer must span the max vinyl revolution (1.8 s @ 33⅓) + ramp headroom — static_assert.

## Source

Extracted from `BRIEF.md` on 2026-08-15 to unblock Stage 0 planning. Will be superseded by full `parameter-spec.md` after UI mockup phase.
