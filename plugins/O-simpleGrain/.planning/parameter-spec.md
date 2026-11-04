# O-simpleGrain — Parameter Specification

---
version: 2.0.0
plugin: O-simpleGrain
created: 2026-06-24
finalized: 2026-06-24
source: BRIEF.md parameter table + ARCHITECTURE.md Parameter Mapping (research-locked)
status: FINAL — promoted from parameter-spec-draft.md to satisfy the Pre-Stage-1 gate.
gate_note: |
  The plan's Pre-Stage-1 gate normally requires a UI mockup to finalize this spec
  (mockup = source of truth). Per user decision (2026-06-24) the mockup is DEFERRED to
  Stage 3 (GUI). This spec is therefore frozen from the research-locked ARCHITECTURE.md
  parameter mapping (the planning contract). Stage 1 (Foundation) builds the APVTS
  skeleton against THIS 18-parameter set. If the Stage 3 mockup surfaces naming/range
  deltas, reconcile then (accepted risk: minor APVTS / state-persistence rework).
---

> **FINAL (mockup deferred).** This is the authoritative 18-parameter APVTS contract for
> Stage 1 Foundation. It reconciles the draft's 16 named params with the two
> research-adopted params (`panSpray`, `velToDensity`) and the ranges/defaults/skews
> locked in `research/ARCHITECTURE.md` § Parameter Mapping.

## Parameter Group: Source

| Param | ID | Type | Range / Choices | Default | Unit | Notes |
|-------|----|------|-----------------|---------|------|-------|
| Source Sample | `sourceSample` | choice | fire / voice / water / piano | **fire** | — | Which built-in short sound is granulated. Built-ins embedded via `juce_add_binary_data`. The `(loaded)` state is reflected in custom state, not as a 5th choice. |
| Load… (user file) | *(action — NOT APVTS)* | native fn + custom state | drag-drop / file picker | — | — | Load-your-own short source. macOS WebView content-streaming drag-drop (`juce::Base64::convertFromBase64`, **NOT** `MemoryBlock::fromBase64Encoding`) + `FileChooser` picker fallback. Loaded-source identity persisted as custom (non-APVTS) ValueTree state. |

## Parameter Group: Grain

| Param | ID | Type | Range | Default | Skew | Unit | Notes |
|-------|----|------|-------|---------|------|------|-------|
| Grain Size | `grainSize` | float | 2 – 200 | 30 | ~0.4 (fine low-end) | ms | Length of each grain — the buzz↔fragments control (DSP-01). |
| Density | `density` | float | 1 – 200 | 40 | log | grains/s | Grains fired per second; with grain size sets overlap depth (DSP-02). Derived live **overlap readout** = `grainSizeSec × density` (display only). |
| Position | `position` | float | 0 – 100 | 50 | linear | % | Read-head resting point in the source (DSP-06). |
| Scan / Time-Stretch | `scan` | float | −200 – +200 | 0 | linear, bipolar | % | Read-head velocity. 0% = held; <100% = stretched; negative = reverse (DSP-06). Pairs with Freeze. |
| Freeze | `freeze` | bool | off / on | **off** | `getToggleState`; smoothed pin | — | Pins the read head on the current instant and sustains indefinitely (FUNC-03). Zipper-free crossfade on toggle (QUAL-01). |

## Parameter Group: Window Shape

| Param | ID | Type | Choices | Default | Notes |
|-------|----|------|---------|---------|-------|
| Window Shape | `windowShape` | choice | rect / tri / Welch / Gauss / Hann | **Hann** | Per-grain amplitude envelope (DSP-03). Five precomputed 2048-pt LUTs. **Rectangular intentionally clicks** (teaching artifact, not a bug). |

## Parameter Group: Spray & Scatter

| Param | ID | Type | Range | Default | Skew | Unit | Notes |
|-------|----|------|-------|---------|------|------|-------|
| Pitch Spray | `pitchSpray` | float | 0 – 12 | 0 | linear | st | Random per-grain transposition — makes a frozen texture shimmer (DSP-04). |
| Position Spray | `positionSpray` | float | 0 – 100 | 0 | linear | % | Random per-grain read position — scatters reads across the source (DSP-04). |
| Scatter | `scatter` | float | 0 – 100 | 0 | linear | % | Randomizes the grain period. 0% = synchronous (discrete sidebands); high = asynchronous (noisy cloud). The sync↔async axis (DSP-05). |
| Grain Pitch | `grainPitch` | float | −24 – +24 | 0 | linear | st | Global transposition of grains; combines multiplicatively with MIDI key-track + `pitchSpray` (decision #2). |
| Pan Spray | `panSpray` | float | 0 – 100 | 0 | linear | % | Per-grain stereo pan spread, equal-power (decision #7). At 0 every grain is centered. |
| Vel → Density | `velToDensity` | float | 0 – 100 | 0 | linear (opt-in) | % | Velocity scales effective density (decision #6). `velToAmp` is always-on (implicit in the amp envelope), so only `velToDensity` is a param. |

## Parameter Group: Amplitude Envelope (per-voice ADSR)

| Param | ID | Type | Range | Default | Skew | Unit |
|-------|----|------|-------|---------|------|------|
| Amp Attack | `ampAttack` | float | 0 – 5 | 0.01 | ~0.35 | s |
| Amp Decay | `ampDecay` | float | 0 – 5 | 0.3 | ~0.35 | s |
| Amp Sustain | `ampSustain` | float | 0 – 100 (0–1) | 0.8 | linear | % |
| Amp Release | `ampRelease` | float | 0 – 5 | 0.4 | ~0.35 | s |

> Velocity scales the note-on level (`velToAmp` implicit/always-on). `setSampleRate` in `prepareToPlay` before first `setParameters`. Mirrors O-simpleFM/O-simpleAdditive exactly.

## Parameter Group: Output

| Param | ID | Type | Range | Default | Notes |
|-------|----|------|-------|---------|-------|
| Output Level | `outputLevel` | float | −inf – 0 | 0 | dB → linear, 20 ms smoothed. Master trim with overlap-aware headroom normalization upstream. |

## Parameter Group: Pulsar (v2.0.0)

Appended with `ParameterID { …, 2 }`; no earlier ID, range, default or order changed. Every default is the 1.9.0 sound: Mode = Cloud, and Cloud never reads the six pulsar controls.

| Param | ID | Type | Range / Choices | Default | Skew | Unit | Notes |
|-------|----|------|-----------------|---------|------|------|-------|
| Grain Mode | `grainMode` | choice | Cloud / Pulsar | **Cloud** | — | — | Cloud = the granular engine, unchanged (bit-exact vs 1.9.0). Pulsar = the key sets the pulse rate f_p; each pulse fires one pulsaret. In Pulsar, Grain Size, Density, Scatter, Vel → Density and Grain Tilt are ignored (dimmed in the UI). |
| Formant | `formant` | float | 1 – 32 | 4 | centre √32 | × | f_formant / f_p = 1 / duty. Per-sample linear ramp. The formant's top rate (× Glisson end × Pitch Spray) is held under **0.10 · fs** by clamping the duty, and f_p itself when a top key alone would pass it. |
| Pulsaret | `pulsaretSource` | choice | Sine / Sample | **Sine** | — | — | One sine cycle (trimmed 2⁻⁴, −24.1 dB), or one root-key (MIDI 60) period of the loaded source squeezed into the pulsaret. Latched per pulsaret. Sine never reads the source. |
| Pulse Mask | `pulseMask` | choice | Off / 2 … 8 | **Off** | — | — | Mute 1 of N: pulse k is silent when k mod N = N − 1, a stable f_p / N subharmonic. |
| FM Depth | `fmDepth` | float | 0 – 100 | 0 | linear | % | Sine rate FM of the pulse clock, linear deviation 0 … 0.95; the mean rate stays f_p. Per-sample linear ramp. |
| FM Ratio | `fmRatio` | float | 0.25 – 8 | **0.5** | centre 1 | × | Modulator = ratio × f_p. Whole-number ratios are inert by construction (onsets stay 1/f_p apart); 0.5 / 1.5 put a line at f_p / 2. |
| Glisson | `glisson` | float | −24 – +24 | 0 | linear (interval 0) | st | Each pulsaret's content glides start → start + glisson over its own length. Latched per pulsaret. A positive glide lowers the formant ceiling (top keys stop rising sooner). |

Engine notes (not parameters): pulsarets under 1.25 ms crossfade to an internal Blackman-Harris window (fully BH under 0.75 ms) with per-window power gains; onsets are fractional (sub-sample); the pulse clock and FM phase are double, ticked per sample; Skip, Pan Spray, Pitch Spray, Position Spray, Position, Scan and Freeze act per pulse; Window Shape / Taper set the user window.

---

## Summary

> **v2.0.0: 30 APVTS parameters.** The 18 below, plus `windowTaper`, `grainSkip` and `grainTilt` (Spray & Scatter / Window, 1.x) and `adsrEnabled` (Amp Envelope, 1.x), which this spec did not track, plus the 7 Pulsar parameters above (version hint 2). See CHANGELOG.md for the 1.x additions.

**18 APVTS parameters** (v1.0) (the count used for complexity scoring):
`sourceSample`, `grainSize`, `density`, `position`, `scan`, `freeze`, `windowShape`,
`pitchSpray`, `positionSpray`, `scatter`, `grainPitch`, `panSpray`, `velToDensity`,
`ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`, `outputLevel`.

**NOT APVTS params:**
- `Load…` — a native-function action + custom (non-APVTS) ValueTree state holding the
  loaded-source identity (`"embedded:fire"` or a user file path).

**Engine config (NOT user params):** polyphony = 8; `MaxGrainsPerVoice` = 24 / global cap 192
(steal-oldest); `ROOT_NOTE` = 60 (C3); source-length cap = 10 s; window LUT size = 2048;
overlap readout (derived/displayed); grain-count/CPU readout (derived/displayed).

## State Persistence (Stage 1 scope)

- All params (30 at v2.0.0) serialized via APVTS `getStateInformation`/`setStateInformation` (suite standard).
- **Custom non-APVTS state:** loaded user-source identity (path or `"embedded:fire"`) as a
  ValueTree child, so a session restores the same source. Embedded built-ins restore by name;
  a user-loaded file restores by re-reading its path (if missing → fall back to default built-in
  with a notice).

## Out of Scope (v1.0)

Spectral STFT (→ O-simpleSpectral), phase-vocoder/tempo-locked stretch, live-input recording,
effects, deep mod matrix / LFO networks, multi-sample / multi-layer sources, auto-scan LFO,
per-grain reverse, second source layer. See REQUIREMENTS.md "Out of Scope".

---
*Finalized 2026-06-24 from parameter-spec-draft.md + research-locked ARCHITECTURE.md to satisfy the Pre-Stage-1 gate; UI mockup deferred to Stage 3 per user decision.*
