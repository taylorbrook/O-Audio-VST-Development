# O-Strata - Parameter Specification (Draft v2 — live terrain oscillator)

> Draft extracted from BRIEF.md (Parameters section, re-planned 2026-09-08) and O-Prism v1.24.0's parameter registry (`plugins/O-Prism/.planning/params.tsv`, 173 parameters). Supersedes `superseded-baked-v1/parameter-spec.md` (v1, 219 params, baked design). The full specification is generated at UI mockup v2 finalisation. IDs follow O-Prism's `osc{A,B}` prefix convention (`StrataParamIds::oscIds`); exact suffix strings are confirmed in the Stage 1 re-parameterise pass.

**Totals:** 171 inherited + 34 new (17 × A/B) = **205** APVTS parameters. Mod-matrix destinations: 26 inherited + 20 new (10 × A/B) = **46** entries in each `modSlot{0..15}Dst`.

## Inherited from O-Prism v1.24.0 (unchanged unless noted)

All O-Prism parameters carry over with identical IDs, ranges, defaults and flags **except** `oscATable` and `oscBTable`, which are removed. Inherited count: **171**.

| Section | IDs (from params.tsv) | Count |
|---------|-----------------------|-------|
| Osc A / Osc B (minus Table) | `osc{A,B}{Pos,Level,Pan,Coarse,Fine,Phase,Unison,Detune,Width,WarpType,WarpAmt}` | 22 |
| Sub / Noise | `subShape subOctave subLevel subRouting noiseType noiseLevel` | 6 |
| Envelopes | `amp{Attack,Decay,Sustain,Release}`, `filt{Attack,Decay,Sustain,Release}`, `filt{A,B}EnvDepth` | 10 |
| Filters | `filt{A,B}{Type,Cutoff,Res,Drive,KeyTrack}`, `filtRouting` | 11 |
| Tuning / Global pitch | `tuningPreset tonic masterTune octaveStretch pitchBendRange glideMode glideTime` | 7 |
| Effects | reverb (7), delay (7), chorus (4), distortion (4), EQ (5) | 27 |
| LFOs | `lfo{1..4}{Rate,Shape,Sync,Division,FreeRun}` | 20 |
| Mod matrix | `modSlot{0..15}{Src,Dst,Amt,On}` | 64 |
| Master | `masterVol stereoWidth oscMix velocityCurve` | 4 |

**Changed in place (same ID, same range):**
- `osc{A,B}Pos` — label **Orbit Size**; default **0.5** (was 0.0); maps to orbit radius 0.05–1.0 (0.0 would collapse the orbit to a point → DC → silence). Remains a mod destination ("Osc A Position" entry renamed "Osc A Orbit Size" in the destination list — same index).
- `osc{A,B}Unison` — maximum **4** for the live oscillator (was 8), per BRIEF must-have 6. Range change on an inherited ID; no O-Strata preset has shipped.
- `modSlot{0..15}Dst` — choice list grows from 26 to 46 entries (append-only; see below).

## New: per-oscillator Terrain / Orbit / Quality (A and B)

Every parameter below exists twice (`oscA…`, `oscB…`). **(mod)** = also a mod-matrix destination, smoothed per sample. Float parameters are linear unless noted.

### Terrain

| Parameter | ID (draft) | Type | Range | Default | Unit | mod | Description |
|-----------|------------|------|-------|---------|------|-----|-------------|
| Terrain | osc?Terrain | Choice | Sine Product / Radial Rings / Saddle / Ridged Cosines / Mitsuhashi / Cosine Wells / Imported… | Sine Product | - | | Analytic terrain or the imported PNG; `Imported…` always last |
| Terrain Freq | osc?TerFreq | Float | 0.25 – 8.0 (log skew) | 1.0 | × | ✓ | Spatial-frequency multiplier; pitch-tracked by Pitch Track |
| Terrain Mod X | osc?TerModX | Float | 0.0 – 1.0 | 0.5 | - | ✓ | Terrain-specific shape input (documented per terrain) |
| Terrain Mod Y | osc?TerModY | Float | 0.0 – 1.0 | 0.5 | - | ✓ | Second terrain-specific shape input |
| Pitch Track | osc?TerTrack | Float | 0.0 – 1.0 | 1.0 | - | | 1 = harmonic count constant across the keyboard; 0 = fixed spatial frequency |
| Saturation | osc?TerSat | Float | 0.0 – 1.0 | 0.0 | - | ✓ | tanh drive on the scanned value; identity at 0 |
| Image Blur | osc?TerBlur | Float | 0.0 – 1.0 | 0.2 | - | | Gaussian pre-blur, PNG only (inert for analytic) |
| Edge Mode | osc?TerEdge | Choice | Mirror / Window | Mirror | - | | PNG edge handling (inert for analytic) |

### Orbit

| Parameter | ID (draft) | Type | Range | Default | Unit | mod | Description |
|-----------|------------|------|-------|---------|------|-----|-------------|
| Orbit | osc?Orbit | Choice | Ellipse / Superellipse / Limaçon / Epitrochoid 3 / Epitrochoid 5 / Epitrochoid 7 / Hypocycloid 3 / Hypocycloid 5 / Hypocycloid 7 / Butterfly / Squarcle | Ellipse | - | | Closed curve traced once per cycle (θ = phase accumulator) |
| Orbit Size | osc?Pos (inherited) | Float | 0.0 – 1.0 | 0.5 | - | ✓ | Radius 0.05–1.0; see "Changed in place" |
| Orbit Aspect | osc?OrbAspect | Float | 0.1 – 1.0 | 0.7 | - | ✓ | Minor/major ratio; default < 1 for the symmetry rule |
| Orbit Rotation | osc?OrbRot | Float | 0.0 – 360.0 | 0.0 | ° | ✓ | X/Y phase relationship |
| Orbit Centre X | osc?OrbCX | Float | -1.0 – 1.0 | 0.13 | - | ✓ | Off-centre default (symmetry rule); view-drag target |
| Orbit Centre Y | osc?OrbCY | Float | -1.0 – 1.0 | 0.21 | - | ✓ | Off-centre default; view-drag target |
| Orbit Mod | osc?OrbMod | Float | 0.0 – 1.0 | 0.5 | - | ✓ | Per-orbit shape input (superellipse n, limaçon loop, epitrochoid inner ratio, squarcle k); inert for Ellipse |
| Feedback | osc?OrbFeedback | Float | 0.0 – 1.0 | 0.0 | - | ✓ | Trajectory feedback amount |
| Feedback Damp | osc?OrbFbDamp | Float | 0.0 – 1.0 | 0.5 | - | | One-pole smoothing of the feedback displacement |

### Quality

| Parameter | ID (draft) | Type | Range | Default | Unit | mod | Description |
|-----------|------------|------|-------|---------|------|-----|-------------|
| Quality | osc?Quality | Choice | Bandlimited / 2× / 4× | 2× | - | | Chebyshev 1× / per-osc oversampling. Default is a Stage 0 decision (2× universal vs Bandlimited showcase) |

### Mod-matrix destination additions (append after O-Prism's 26, in this order)

Per oscillator, A then B: `Orbit Aspect`, `Orbit Rotation`, `Orbit Centre X`, `Orbit Centre Y`, `Orbit Mod`, `Terrain Freq`, `Terrain Mod X`, `Terrain Mod Y`, `Feedback`, `Saturation` — 10 × 2 = 20 new entries; `Osc A/B Position` (existing entries) are relabelled `Orbit Size`. All are per-sample smoothed (`juce::SmoothedValue`, ~5 ms) at the oscillator; the mod matrix itself is unchanged.

## Non-parameter state

| State | Format | Notes |
|-------|--------|-------|
| `terrainImports/osc{A,B}Import` | `kind="png" name sha256 bytes embedded="1" data="<base64>"` or `embedded="0" path sha256` | Raw PNG bytes (already deflated; no gzip), ≤ 2 MB per oscillator; above the cap path + SHA. Missing file → library fallback (Sine Product) + localised notice. Properties round-trip as strings. |
| Chebyshev coefficient sets | runtime only | Regenerated from parameters off the audio thread; never persisted |
| `uiLanguage`, `tuningEngine` | inherited | unchanged |

## Draft reconciliation notes

- The baked design's 48 geometry parameters (`osc?Geo*`, `osc?Mesh*`, `osc?Vol*`, the old `osc?Ter*` sweep set) are removed in the Stage 1 re-parameterise pass; `osc?TerBlur` and `osc?TerEdge` keep their IDs and meaning. Anything else that collides by name (`osc?Terrain`) keeps the ID but its choice list changes — no shipped preset depends on the old list.
- Choice lists are append-only after v1.0.0 ships; `Imported…` stays the last Terrain index.
- Every Choice has ≥ 2 entries (memory `critical_choice_param_needs_two_choices`).
- None of the new suffixes collides with a `juce::` free function (memory `critical_paramid_shadows_juce_free_function`) — `Orbit`, `Quality`, `TerFreq` etc. checked by eye; re-check at Stage 1.
