# O-Bitrot - Parameter Specification

## Overview

Broken-media degradation box — a clocked stochastic state machine over a shared circular buffer emulating failing playback hardware. Six degradation families (Tape / CD Skip / Vinyl / Packet Loss / Codec / Crush), each with an explicit per-module control section, plus a global clock/seed/mix strip. Fully seeded randomness for reproducible renders.

**Status:** BINDING contract for Stage 1+ (promoted 2026-08-15 from `parameter-spec-draft.md` with Stage 0 deltas from `research/ARCHITECTURE.md` Parameter Mapping folded in, per Stage 1 discuss decision). The UI mockup phase may refine layout/UI labels but NOT parameter IDs, types, ranges, or defaults.

## Parameters

### Global

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CLOCK_MODE | Clock Mode | Choice | Sync / Free | Sync | - | Tempo-synced divisions vs free-running rate. |
| CLOCK_SYNC_DIV | Clock Division | Choice | 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 bar (7 choices) | 1/8 | - | Re-roll interval in Sync mode (musical divisions). |
| CLOCK_FREE_RATE | Clock Rate | Float | 0.1 - 20.0 (exponential skew) | 2.0 | Hz | Re-roll rate in Free mode ("the deck is dying" feel). |
| SEED | Seed | Int | 0 - 9999 | 0 | - | Deterministic randomness; same seed + input + params = identical bounce. Persisted in state. Reseed dice button rolls a new value (UI trigger writing this param, not a separate param). |
| HARD_EDGES | Hard Edges | Bool | Off / On | Off | - | Bypass 1–5 ms jump crossfades for deliberate clicks. |
| MIX | Mix | Float | 0.0 - 100.0 | 100.0 | % | Global dry/wet. |

### Tape

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| TAPE_ENABLE | Tape Enable | Bool | Off / On | On | - | Section enable. |
| TAPE_PROB | Tape Probability | Float | 0.0 - 100.0 | 25.0 | % | Probability of a tape event per clock tick. |
| TAPE_STOP_PROB | Tape-Stop Share | Float | 0.0 - 100.0 | 10.0 | % | Share of tape events that are full stops. |
| TAPE_RAMP | Tape Ramp | Float | 20.0 - 500.0 | 150.0 | ms | Speed-transition ramp time — the ramp IS the glide sound and the click-safety. |
| TAPE_DROP | Tape Dropout Share | Float | 0.0 - 100.0 | 0.0 | % | *(v1.4.0)* Share of NON-stop tape events that are oxide-shed dropouts — a level dip to a random 10–70% floor over 5–150 ms with a concurrent HF cutoff dip. Installs no rate event (gain/filter domain only). At 0 the roll is skipped entirely, so the tape stream's draw pattern is unchanged. |
| TAPE_WOW | Tape Wow/Flutter | Float | 0.0 - 100.0 | 0.0 | % | *(v1.4.0)* Continuous speed-modulation bed — wow (0.73 / 2.31 Hz) plus flutter (7–55 Hz), 2.0% peak read-rate deviation at full knob, on its own RNG stream. Implemented as a non-negative READ OFFSET rather than a rate multiplier (see Design Notes). Exactly transparent at 0. |
| TAPE_HISS | Tape Hiss | Float | 0.0 - 100.0 | 0.0 | % | *(v1.5.0)* Continuous tape-hiss bed — decorrelated stereo white behind a −9 dB shelf above 3.5 kHz and a 60 Hz highpass, −48 dBFS RMS at full knob. Rides `TapeStopGain`'s speed gain, so a tape stop mutes the hiss with the programme. Own RNG stream (2 draws/sample). Exactly transparent at 0. |

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
| VINYL_RPM | Vinyl RPM | Choice | 33 1/3 / 45 | 33 1/3 | RPM | Revolution quantum for jumps/locked grooves (1.8 s @ 33 1/3, 1.333 s @ 45). Pitch never changes. |
| VINYL_POP | Vinyl Pop | Float | 0.0 - 100.0 | 50.0 | % | Synthesized pop level. |
| VINYL_WEAR | Vinyl Wear | Float | 0.0 - 100.0 | 0.0 | % | *(v1.5.0)* Continuous surface bed — Poisson micro-ticks (up to 140/s at full knob, cubic power-law amplitude through a 2–6 kHz bandpass) over pinked bearing rumble (−42 dBFS RMS at full, LP'd below 55 Hz, amplitude-modulated once per platter revolution at the VINYL_RPM rate). One knob scales both tick density and bed level. Mono — one bearing, one stylus. Own RNG stream. Exactly transparent at 0. |

### Packet Loss

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| PACKET_ENABLE | Packet Enable | Bool | Off / On | Off | - | Section enable. |
| PACKET_LOSS | Packet Loss | Float | 0.0 - 100.0 | 20.0 | % | Overall loss intensity (drives Gilbert–Elliott Markov state probabilities over 20 ms packets). |
| PACKET_BURST | Packet Burstiness | Float | 0.0 - 100.0 | 30.0 | % | Burstiness — P(Good→Bad) / burst length (E[burst] 1–8 packets). |
| PACKET_CONCEAL | Concealment | Choice | Silence / Repeat / Decay / Substitute | Decay | - | Concealment mode: silence / robotic repeat / decaying repeat / waveform substitution. |
| PACKET_COMFORT | Comfort Noise | Float | 0.0 - 100.0 | 0.0 | % | *(v1.5.0)* G.711 App II / GSM SID-style comfort noise under extended concealment. After 3 lost packets (~60 ms — exactly where Decay's ramp reaches its silence floor) a spectrally-matched noise bed ramps in UNDER the conceal output, and back out over one packet when the burst ends. Level is squared before scaling: 100% = −30 dB relative to the tracked programme RMS, 50% = −42 dB (the G.711 figure), 25% = −54 dB. Own RNG stream. Exactly transparent at 0. |

### Codec

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CODEC_ENABLE | Codec Enable | Bool | Off / On | Off | - | Section enable. |
| CODEC_MODE | Codec Mode | Choice | Mu-law / GSM | Mu-law | - | Landline (mono sum → 300–3400 Hz BP → 8 kHz → μ-law round trip) vs cellphone (GSM 06.10 via vendored libgsm). |
| CODEC_MIX | Codec Mix | Float | 0.0 - 100.0 | 100.0 | % | Codec chain blend. |
| CODEC_NOISE | Line Noise | Float | 0.0 - 100.0 | 0.0 | % | *(v1.5.0)* Phone-line bed — deterministic mains hum (fundamental + 2nd + 3rd, −40 dBFS RMS at full knob, phase from the sample count so it takes NO RNG draws) plus Poisson crackle bursts (up to 6/s, 3–25 ms, band-limited 300–3000 Hz). Injected AFTER `CodecStage` — 50 Hz in front of a 300–3400 Hz passband is annihilated — and scaled by CODEC_MIX, since the hum IS the line. Mono. Exactly transparent at 0. |
| CODEC_MAINS | Mains Frequency | Choice | 50 Hz / 60 Hz | 50 Hz | - | *(v1.5.0)* Mains frequency for the CODEC_NOISE hum. Inert while CODEC_NOISE is 0. |
| CODEC_AGC | Codec AGC | Float | 0.0 - 100.0 | 100.0 | % | *(v1.8.0)* Depth of the feed-forward AGC on the codec sub-path (after the mode blend, before the post-LPF): 1 ms attack, 50 ms release, −20 dBFS threshold, 4:1, fixed +10 dB makeup, unity near −6.7 dBFS. RNG-free. Gain is assembled in dB and scaled there, so 0 is `exp(0)` = exactly 1.0f — bit-transparent, restoring the v1.7.0 gain structure. Inert while CODEC_ENABLE is off. |

### Crush

| ID | Name | Type | Range | Default | Unit | Description |
|----|------|------|-------|---------|------|-------------|
| CRUSH_ENABLE | Crush Enable | Bool | Off / On | Off | - | Section enable. |
| CRUSH_BITS | Crush Bits | Float | 1.0 - 16.0 | 16.0 | bits | Fractional-bit quantization depth (mid-tread/riser), continuously sweepable, zipper-free. |
| CRUSH_RATE | Crush Rate | Float | 500.0 - 20000.0 (exponential skew) | 20000.0 | Hz | Sample-rate reduction — fractional-crossing interpolated hold (DeRez-style). Range is fs-independent for automation portability; runtime clamp to fs/2; at max the latch runs every sample = transparent. |
| CRUSH_JITTER | Crush Jitter | Float | 0.0 - 100.0 | 0.0 | % | S&H clock jitter (Decimort-style hash). |
| CRUSH_ENV_AMT | Crush Env Amount | Float | -100.0 - 100.0 | 0.0 | % | Envelope-driven dynamic bit depth; − = duck (tails crush), + = pump (transients splatter). Per-sample follower. |
| CRUSH_DITHER | Crush Dither | Float | 0.0 - 2.0 | 0.0 | LSB | TPDF dither: grit ↔ hiss morph. |

## Parameter Count Summary

- Global: 6
- Tape: 7 (4 at Stage 1; TAPE_DROP + TAPE_WOW added v1.4.0; TAPE_HISS added v1.5.0)
- CD Skip: 4
- Vinyl: 5 (VINYL_WEAR added v1.5.0)
- Packet Loss: 5 (PACKET_COMFORT added v1.5.0)
- Codec: 6 (CODEC_NOISE + CODEC_MAINS added v1.5.0; CODEC_AGC added v1.8.0)
- Crush: 6
- **Total: 40** (+ reseed UI trigger) — 31 at Stage 1, 33 at v1.4.0, 38 at v1.5.0, 39 at v1.7.0

## Post-Stage-1 Amendments

This document is the BINDING Stage-1 contract; improvement releases amend
it here rather than rewriting history above.

- **v1.3.0 (2026-08-17):** `kRingSeconds` 2.5 → 10.0. The Design Note below
  still names 2.5 as the Stage-1 value; the constraint it states (span the
  max vinyl revolution + max tape ramp + margin, as a `static_assert`) was
  rewritten at the same time to constrain the constant rather than merely
  be satisfied by it — the old form was true of 2.5 s and 10 s alike.
  `CaptureRing::readFrac` 2-point lerp → 4-point Catmull-Rom.
- **v1.4.0 (2026-08-17):** `TAPE_DROP` and `TAPE_WOW` added, both default 0
  and exactly transparent there. They are **appended to the end of the
  APVTS layout**, not inserted into the Tape block, because layout order is
  the automation-slot order a host presents — inserting would shift all 23
  later parameters by two slots and repoint saved automation lanes. The
  table above groups them with Tape for readability; the layout does not.
- **v1.5.0 (2026-08-17):** `TAPE_HISS`, `VINYL_WEAR`, `CODEC_NOISE`,
  `CODEC_MAINS` and `PACKET_COMFORT` added — the per-family media-noise beds
  (brief item 4) and comfort noise under extended concealment (item 19). All
  five default to their transparent value and are **appended to the end of
  the APVTS layout** for the same automation-slot reason as the v1.4.0 pair;
  the tables above group them with their families for readability only.
  `CODEC_MAINS` is a NEW choice parameter rather than an appended choice on
  an existing one, so no saved normalised value is repointed and no preset
  migration gate is required. Four RNG streams (`tapeBed`, `vinylBed`,
  `codecBed`, `comfort`) appended to `RngBank` — stream *k* is seeded from a
  function of *k* alone, so every pre-existing stream, and every render made
  with an old `SEED`, is bit-identical.

- **v1.8.0 (2026-08-17):** `CODEC_AGC` added (brief item 16), **appended to
  the end of the APVTS layout** for the fourth release running, same
  automation-slot reason. Unlike the v1.4.0, v1.5.0 and v1.7.0 appends it
  does **not** default to its transparent value: it defaults to 100, on with
  the section, because the research chain has a fast AGC after the codec and
  the pumping is a large part of why a phone sounds like a phone. A v1.7.0
  session with `CODEC_ENABLE` **off** is still bit-identical; one with it
  **on** renders differently — from the AGC, and independently from item
  29's segmented G.711 mu-law, which has no opt-out at all, so defaulting
  this to 0 would have preserved nothing. It is a NEW float parameter, so no
  saved normalised value is repointed and no preset migration gate is
  required.

## Design Notes

- **ASCII-safe host-facing labels** (Stage 1 discuss decision): all C++ parameter/choice strings use plain ASCII — `Mu-law`, `33 1/3` — because `juce::String(const char*)` is ASCII-only and non-ASCII literals mangle in DAW automation lanes (repo pattern). The WebView UI is free to render the μ-law / 33⅓ glyphs.
- **Skews:** CLOCK_FREE_RATE and CRUSH_RATE use exponential skew (wide ranges); all other floats linear.
- **Per-module control sections, no macro dice knobs** — each family has explicit enable + parameters (FUNC-05).
- **Clock split into three params** (mode + sync division + free rate) so both modes have stable automatable values.
- **Seeded RNG:** one stream per stochastic subsystem (shared audio-thread RNG breaks block-size invariance — repo pattern). Seed persisted in APVTS state; reseed is a UI-side trigger writing SEED.
- **Anti-zipper rules:** smooth the rate/step *target* per-sample, never the output; fractional-crossing interpolation; never reset phase accumulators on param change; all position jumps crossfaded 1–5 ms unless HARD_EDGES.
- **Defaults:** physical-media families (Tape/CD/Vinyl) on, digital families (Packet/Codec/Crush) off — instant broken-media character without instant lo-fi mangling.
- **Latency:** constant `ceil(0.020·fs)` samples reported once in `prepareToPlay()` via `setLatencySamples()` (scheme lands in Stage 2 Phase 2.1; Stage 1 passthrough reports 0).
- **Buffer sizing:** circular buffer `kRingSeconds = 2.5` must span the max vinyl revolution (1.8 s @ 33 1/3) + max tape ramp + margin — static_assert.

## Source

Draft extracted from `BRIEF.md` 2026-08-15; promoted to binding spec at Stage 1 discuss (2026-08-15) with ARCHITECTURE.md Parameter Mapping deltas: CLOCK_SYNC_DIV enumerated to 7 divisions, CRUSH_RATE fixed at 500 Hz–20 kHz (fs-independent), exponential skews recorded, ASCII-safe label policy.
