# O-simpleSampler — Parameter Specification

---
version: 1.1.0
plugin: O-simpleSampler
created: 2026-06-25
updated: 2026-08-01
source: research/ARCHITECTURE.md Parameter Mapping (authoritative; supersedes parameter-spec-draft.md)
status: final (live APVTS contract)
param_count: 20
---

> Finalized from the research-locked **ARCHITECTURE.md → Parameter Mapping** table
> (2026-06-25). The plan-before-mockup path was taken (no mockup YAML yet); every
> range/default/skew below is resolved and is the authoritative contract the APVTS is
> built against. The UI must bind exactly this set — additions/removals require a
> contract bump.
>
> **v1.1.0 contract change:** the Source Sample choice parameter was removed, leaving
> **20 parameters**. Source selection is no longer parameterised — see "Built-in source
> set" below.

## Signal path (left → right)

`SOURCE → REGION (start/end · loop · reverse) → PITCH (root · Repitch/Stretch · tune/fine) → VINTAGE → FILTER (resonant LP) → AMP (ADSR → VCA · vel) → OUTPUT`

## The 20 APVTS Parameters

### Source (0)

No source parameter. As of v1.1.0 the active source is **not** an APVTS parameter — the
plugin starts on its one embedded built-in, and `Load…` / drag-drop is a native-fn action
carrying custom `ValueTree` state (FUNC-02/03). See "Built-in source set" below.

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

**Total: 20 APVTS parameters.**

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

## Built-in source set (v1.1.0 — single embedded source)

**There is exactly one embedded source: `piano`**, at recorded root **48** (probed f0
≈131.6 Hz via YIN → nearest MIDI note). Its identity is **`embedded:piano`**, which is
also the default identity for a fresh instance. The recorded root is seeded into the live
`rootKey` on a fresh instance so the source plays in tune with no user action (the APVTS
`rootKey` *default* stays 60, frozen).

**Source selection is no longer parameterised.** The v1.0 choice parameter was removed in
v1.1.0 — with one built-in remaining, a single-entry `AudioParameterChoice` is invalid in
JUCE and produces a degenerate normalisable range. `Load…` and drag-and-drop are the only
ways to change the source, and they carry custom `ValueTree` state rather than a parameter
value. The engine keeps `kNumBuiltIns` / `kBuiltInNames` / `kBuiltInRoot` as arrays so a
future cleared built-in is a one-line addition.

Provenance for the embedded asset is recorded in `Source/samples/LICENSE.md`.

## v1.1-deferred (NOT in v1.0)

`velToFilter` (velocity→filter); phase-vocoder "HQ Stretch"; slicing/chop-to-pads
(→ O-simpleSlicer); multi-zone / velocity-layer multisampling; effects
(reverb/delay/chorus); tempo-sync / warp-to-grid; internal sequencer / pad grid.

---
*Finalized 2026-06-25 from research/ARCHITECTURE.md. Supersedes parameter-spec-draft.md.*
