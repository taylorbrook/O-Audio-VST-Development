# O-simpleSampler — Parameter Specification

---
version: 1.0.0
plugin: O-simpleSampler
created: 2026-06-25
source: research/ARCHITECTURE.md Parameter Mapping (authoritative; supersedes parameter-spec-draft.md)
status: final (Stage-1 APVTS contract)
param_count: 21
---

> Finalized from the research-locked **ARCHITECTURE.md → Parameter Mapping** table
> (2026-06-25). The plan-before-mockup path was taken (no mockup YAML yet); the
> 21-parameter set and every range/default/skew below are resolved and are the
> authoritative contract the Stage-1 APVTS is built against. The Stage-3 mockup
> must bind exactly these 21 parameters — additions/removals require a contract bump.

## Signal path (left → right)

`SOURCE → REGION (start/end · loop · reverse) → PITCH (root · Repitch/Stretch · tune/fine) → VINTAGE → FILTER (resonant LP) → AMP (ADSR → VCA · vel) → OUTPUT`

## The 21 APVTS Parameters

### Source (1)

| Param | ID | Type | Range / Choices | Default | Skew / Notes |
|-------|----|------|-----------------|---------|--------------|
| Source Sample | `sourceSample` | Choice | curated built-ins | first built-in | seeds `rootKey` to per-sample default (Stage 2). `Load…` is a **separate native-fn action + custom ValueTree state**, NOT a 5th choice (FUNC-02/03). |

### Region — Start/End · Loop · Reverse (7)

| Param | ID | Type | Range | Default | Skew / Notes |
|-------|----|------|-------|---------|--------------|
| Start | `start` | Float | 0 – 100% | 0 | linear; zero-cross snapped (Stage 2) — FUNC-04 |
| End | `end` | Float | 0 – 100% | 100 | linear; zero-cross snapped — FUNC-04 |
| Loop Mode | `loopMode` | Choice | Off / Forward / Ping-Pong | Off | — FUNC-05 |
| Loop Start | `loopStart` | Float | 0 – 100% (of region) | 0 | linear; zero-cross snapped — FUNC-05 |
| Loop End | `loopEnd` | Float | 0 – 100% (of region) | 100 | linear; zero-cross snapped — FUNC-05 |
| Loop Crossfade | `loopCrossfade` | Float | 0 – 500 ms | 10 | skew ≈0.4; equal-power loop-seam crossfade — DSP-03 |
| Reverse | `reverse` | Bool | off / on | off | `getToggleState` — FUNC-06 |

### Pitch — Root · Repitch/Stretch · Tune (4)

| Param | ID | Type | Range | Default | Skew / Notes |
|-------|----|------|-------|---------|--------------|
| Root Key | `rootKey` | Int | 0 – 127 (C-1–G9) | 60 (C3) | key at which the sample plays at original pitch — FUNC-01 |
| Pitch Mode | `pitchMode` | Choice | Repitch / Stretch | Repitch | the HEADLINE A/B: pitch+time coupled vs independent — DSP-01 |
| Tune | `tune` | Int | −24 – +24 st | 0 | coarse transpose (KEPT separate from `fine`) — FUNC-08 |
| Fine | `fine` | Float | −100 – +100 cents | 0 | linear — FUNC-08 |

### Vintage (1)

| Param | ID | Type | Range | Default | Skew / Notes |
|-------|----|------|-------|---------|--------------|
| Vintage | `vintage` | Float | 0 – 100% | 0 | linear; **full bypass at 0** — S&H decimation + bit-crush, SP-1200 grit — DSP-04 |

### Filter — resonant low-pass (2)

| Param | ID | Type | Range | Default | Skew / Notes |
|-------|----|------|-------|---------|--------------|
| Filter Cutoff | `filterCutoff` | Float | 20 – 20000 Hz | 20000 | **log** (skew-for-centre ≈1 kHz) — DSP-05 |
| Filter Resonance | `filterResonance` | Float | 0 – 100% | 0 | linear → Q — DSP-05 |

### Amplitude Envelope — ADSR → VCA (4)

| Param | ID | Type | Range | Default | Skew / Notes |
|-------|----|------|-------|---------|--------------|
| Amp Attack | `ampAttack` | Float | 0 – 5 s | 0.005 | skew ≈0.35 — DSP-06 |
| Amp Decay | `ampDecay` | Float | 0 – 5 s | 0.3 | skew ≈0.35 |
| Amp Sustain | `ampSustain` | Float | 0 – 100% (stored 0–1) | 100% (1.0) | linear; UI scales ×100 (O-simpleGrain convention) |
| Amp Release | `ampRelease` | Float | 0 – 5 s | 0.2 | skew ≈0.35 |

### Voice / Output (2)

| Param | ID | Type | Range | Default | Skew / Notes |
|-------|----|------|-------|---------|--------------|
| Velocity → Amp | `velToAmp` | Float | 0 – 100% | 50 | linear; how much note velocity scales loudness — DSP-06 |
| Output Level | `outputLevel` | Float | −60 – 0 dB | 0 | dB→lin, 20 ms smooth (−60 dB ≡ −inf) |

**Total: 21 APVTS parameters.**

## Storage conventions (Stage-1 APVTS ↔ Stage-3 UI)

- Percent params (`start`, `end`, `loopStart`, `loopEnd`, `vintage`, `filterResonance`, `velToAmp`) are stored as **0–100** (raw percent) — O-simpleGrain convention.
- `ampSustain` is stored **0–1** (UI multiplies ×100) so it feeds `juce::ADSR` directly — O-simpleGrain convention.
- `rootKey` / `tune` are integer params (`juce::AudioParameterInt`).
- All `ParameterID`s are versioned (`{ "id", 1 }`).

## Non-APVTS state

- **Loaded user-source identity** — `embedded:<name>` (built-in) or a file path (load-your-own). Persisted as a custom `ValueTree` child (`SOURCE/identity`) alongside the APVTS tree so a session restores the same source. Default `embedded:<first-built-in>`. (O-simpleGrain pattern.)

## Engine config (NOT user parameters)

- Polyphony: **16 voices**
- `rootKey` default: **60 (C3)** — key-track reference
- Source-length cap: **30 s**
- Stretch internal grain (fixed/hidden): `size ≈ 60 ms`, Hann window, 2× overlap, `MaxGrainsPerVoice = 4`
- Vintage floor: `FS_MIN ≈ 3000 Hz`, `BITS_MIN ≈ 8`
- Latency: **0** (no oversampling / lookahead)

## Built-in source set (FINALIZED — Stage 4, curated assets delivered)

The curated 4-choice set is embedded (`Source/samples/`) and wired through the
`sourceSample` `AudioParameterChoice`. Each source's recorded-pitch root (`kBuiltInRoot`,
MIDI) was probed via YIN f0 → nearest MIDI note and is seeded into the live `rootKey`
on selection (the APVTS `rootKey` *default* stays 60, frozen). `hit` is percussive
(unvoiced) → neutral root 60 so pressing the reference key plays it at recorded speed.

| Index | Choice  | Recorded root (MIDI) | Probed f0 |
|-------|---------|----------------------|-----------|
| 0 | `piano` | 48 | ≈131.6 Hz |
| 1 | `cello` | 69 | ≈441 Hz   |
| 2 | `pizz`  | 69 | ≈445 Hz   |
| 3 | `hit`   | 60 | percussive (unvoiced) |

Default `sourceSample` = index 0 (`piano`); default identity = `embedded:piano`.

## v1.1-deferred (NOT in v1.0)

`velToFilter` (velocity→filter); phase-vocoder "HQ Stretch"; slicing/chop-to-pads
(→ O-simpleSlicer); multi-zone / velocity-layer multisampling; effects
(reverb/delay/chorus); tempo-sync / warp-to-grid; internal sequencer / pad grid.

---
*Finalized 2026-06-25 from research/ARCHITECTURE.md. Supersedes parameter-spec-draft.md.*
