# Parameter Specification (DRAFT): O-ParametricEQ

**Status: DRAFT, for Stage 0 planning only.** Transcribed on 2026-11-12 from the parameter tables in `BRIEF.md`. IDs, skews and the 24-band automation layout are **provisional** — the brief assigns them to Stage 0 ("Exact IDs, ranges, skews and the automation layout for 24 bands are settled in Stage 0"). The full `parameter-spec.md` comes from mockup finalization (`/start O-ParametricEQ` → option 3) and is required before Stage 1.

**Suite conventions (binding for Stage 1):** `ParameterID {id, 1}`; no lambda `NormalisableRange` (invisible to the WebView slider frontend); `setSkewForCentre` for log-like rows; interval 0 on ranges where an exact 0 default matters (a stepped range with a negative start snaps 0 to −4e-06); no parameter writes another parameter from a listener.

## Total Parameter Count (provisional)

| Group | Count | Notes |
|-------|-------|-------|
| Per band (8 × 24) | 192 | exposed in v1 |
| Global | 5 | exposed in v1 |
| **Total v1** | **197** | |
| Reserved per band for dynamic EQ (3 × 24) | 72 | not exposed in v1 — see Open Questions |

## Parameter Definitions

### Per band — template `bN_` (N = 1 … 24; 8 × 24 = 192)

| ID | Type | Range / choices | Default | Skew | Description |
|----|------|-----------------|---------|------|-------------|
| `bN_enabled` | Bool | Off / On | Off | — | Whether the band exists. Bands are created by clicking the curve. |
| `bN_bypass` | Bool | Off / On | Off | — | Mutes the band's effect without removing it. |
| `bN_type` | Choice | 8: Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt Shelf | 0 (Bell) | — | Filter shape. |
| `bN_freq` | Float | 10 … 30000 Hz | 1000 | log-like (centre ≈ 548 Hz, the geometric mean; or centre 1 kHz) | Centre / corner frequency. Must be clamped below Nyquist in the DSP at low sample rates. |
| `bN_gain` | Float (interval 0) | −30 … +30 dB | 0 | linear | Bell, shelf and tilt gain. Unused by cut, notch and band pass. |
| `bN_q` | Float | 0.025 … 40 | 1.0 | log-like (centre 1.0, the geometric mean) | Bandwidth / resonance. |
| `bN_slope` | Choice | 8: 6, 12, 18, 24, 36, 48, 72, 96 dB/oct | 1 (12 dB/oct) | — | Steepness, on every type that supports it. |
| `bN_placement` | Choice | 5: Stereo, Left, Right, Mid, Side | 0 (Stereo) | — | Which channel component the band processes. Inert on a mono layout. |

### Reserved per band for dynamic EQ (not exposed in v1)

| ID (provisional) | Type | Range | Default | Description |
|------------------|------|-------|---------|-------------|
| `bN_dyn_range` | Float (interval 0) | TBD (± dB) | 0 | Dynamic gain range; 0 = static band. |
| `bN_dyn_threshold` | Float | TBD (dB) | TBD | Detector threshold. |
| `bN_dyn_auto_threshold` | Bool | Off / On | On | Automatic threshold. |

### Global (5)

| ID | Type | Range / choices | Default | Skew | Description |
|----|------|-----------------|---------|------|-------------|
| `output_gain` | Float (interval 0) | −36 … +36 dB | 0 | linear | Post-EQ level trim. |
| `auto_gain` | Bool | Off / On | Off | — | Compensates the loudness change the curve introduces. |
| `gain_scale` | Float | 0 … 200 % | 100 | linear | Scales the gain of all bands together. |
| `analyzer` | Choice | 4: Off, Pre, Post, Pre+Post | 2 (Post) | — | Real-time spectrum display source. |
| `bypass` | Bool | Off / On | Off | — | Click-free global bypass. |

## Open Questions for Stage 0

1. **Dynamic-EQ reservation.** Register the 72 reserved parameters now (hidden / non-automatable, defaults = static) so the host parameter list never changes, or add them in v1.x under new IDs with a state-version migration? The choice must keep v1 sessions and automation intact (DSP-07).
2. **Parameter order.** Band-major (`b1_*`, `b2_*` …) or parameter-major, and whether the global block comes first — this fixes the automation-lane order hosts show for 197+ parameters.
3. **`bN_slope` on types that ignore it.** Which of Bell / Notch / Band Pass / Tilt honour slope ("every type that supports it"), and what the parameter does on the rest.
4. **`analyzer` and `bN_enabled` as host parameters.** Whether the analyzer source is an automatable parameter or UI state, and whether `bN_enabled` should be automatable (click-free either way, per DSP-04).
5. **Global `bypass`.** Wire to the host bypass parameter (`getBypassParameter()`) so host bypass is the same click-free path.
6. **Frequency skew.** Exact log mapping realisable without a lambda range.
