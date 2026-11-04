# O-Granulator Parameter Specification (Draft)

> Extracted from BRIEF.md (2026-11-03). Full spec will be finalized during the UI mockup phase — required before Stage 1.
> IDs are draft; per-stream params are instantiated ×4 with prefix `s1_` … `s4_`.

## Global / Buffer

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| input_gain | Input Gain | -24.0 to +12.0 dB | 0.0 | Float (dB) | Level into the buffer |
| buffer_length | Buffer Length | 1.0–30.0 s | 8.0 | Float | Active ring-buffer span that position scans across |
| freeze | Freeze | Off/On | Off | Bool | Stop writing; buffer contents held |
| sync_mode | Sync Mode | Free / Tempo | Free | Choice | Enables tempo-synced size/density/LFO rate |
| mix | Mix | 0–100 % | 50 | Float | Dry/wet |
| output_gain | Output Gain | -24.0 to +12.0 dB | 0.0 | Float (dB) | Final level |

## Per Stream (×4) — prefix `sN_`

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| sN_enable | Enable | Off/On | s1 On, s2–s4 Off | Bool | Stream active |
| sN_mute | Mute | Off/On | Off | Bool | Mixer mute |
| sN_solo | Solo | Off/On | Off | Bool | Mixer solo |
| sN_level | Level | -inf to +6.0 dB | 0.0 | Float (dB) | Stream output level |
| sN_pan | Pan | -100 (L) – +100 (R) | 0 | Float | Stream pan |
| sN_position | Position | 0–100 % | 0 | Float | Read point behind write head (0 = most recent) |
| sN_spray | Spray | 0–100 % | 10 | Float | Random position jitter per grain |
| sN_size | Size | 5–2000 ms | 100 | Float (skewed) | Grain length (Free mode) |
| sN_size_sync | Size (Sync) | 1/64 – 2 bars (incl. dotted/triplet) | 1/16 | Choice | Grain length (Tempo mode) |
| sN_size_rand | Size Random | 0–100 % | 0 | Float | Grain-length randomisation |
| sN_density | Density | 0.5–200 grains/s | 10 | Float (skewed) | Grain rate (Free mode) |
| sN_density_sync | Density (Sync) | 1/64 – 1 bar | 1/16 | Choice | Grain rate (Tempo mode) |
| sN_jitter | Jitter | 0–100 % | 20 | Float | Onset timing randomisation (sync ↔ async) |
| sN_pitch | Pitch | -24 to +24 st | 0 | Float | Grain transposition |
| sN_pitch_rand | Pitch Random | 0–24 st | 0 | Float | Random pitch spread |
| sN_scale | Scale | Off / Chromatic / Major / Minor / Pentatonic / Whole Tone / Octaves / Fifths | Off | Choice | Quantise pitch spread |
| sN_reverse_prob | Reverse | 0–100 % | 0 | Float | Probability a grain plays backwards |
| sN_window | Window | 0–1 continuous (Hann → Tukey → Trapezoid → Exponential) | 0 (Hann) | Float | Window shape morph |
| sN_window_skew | Window Skew | -100 – +100 % | 0 | Float | Attack/decay skew |
| sN_stereo_spread | Stereo Spread | 0–100 % | 30 | Float | Per-grain pan randomisation |
| sN_gain_rand | Gain Random | 0–100 % | 0 | Float | Per-grain level jitter |

## Per-Grain FX (×4) — prefix `sN_`

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| sN_filter_type | Filter Type | Off / LP / BP / HP | Off | Choice | Per-grain SVF mode |
| sN_filter_cutoff | Cutoff | 20 Hz – 20 kHz | 20000 | Float (log) | Base cutoff |
| sN_filter_rand | Cutoff Random | 0–100 % (± octaves) | 0 | Float | Per-grain cutoff randomisation |
| sN_filter_res | Resonance | 0–100 % | 20 | Float | Resonance |
| sN_drive | Drive | 0–100 % | 0 | Float | Per-grain saturation |
| sN_crush | Crush | 0–100 % | 0 | Float | Bit/sample-rate reduction |
| sN_crush_prob | Crush Probability | 0–100 % | 100 | Float | Probability a grain is crushed |
| sN_ring_freq | Ring Freq | 20 Hz – 5 kHz | 200 | Float (log) | Ring-mod carrier base frequency |
| sN_ring_rand | Ring Random | 0–100 % | 0 | Float | Per-grain carrier randomisation |
| sN_ring_mix | Ring Mix | 0–100 % | 0 | Float | Ring-mod amount |

## Feedback Network

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| feedback | Feedback | 0–100 % (safety-limited, loop gain < 1) | 0 | Float | Grain output written back into buffer |
| fb_low_cut | FB Low Cut | 20–2000 Hz | 80 | Float (log) | HP in loop |
| fb_high_cut | FB High Cut | 1–20 kHz | 12000 | Float (log) | LP in loop |
| fb_saturation | FB Saturation | 0–100 % | 20 | Float | Soft-clip in loop (also runaway guard) |
| fb_diffusion | FB Diffusion | 0–100 % | 0 | Float | Allpass diffuser in loop |

## Output FX

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| reverb_mix | Reverb Mix | 0–100 % | 0 | Float | Post-mixer reverb/diffuser amount |
| reverb_size | Reverb Size | 0–100 % | 50 | Float | Decay/size |
| reverb_damp | Reverb Damp | 0–100 % | 50 | Float | HF damping |

## Modulation Sources

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| lfoN_shape (N=1–4) | LFO Shape | Sine / Tri / Saw / Square / S&H / Smooth Random | Sine | Choice | Waveform |
| lfoN_rate | LFO Rate | 0.01–20 Hz | 0.5 | Float (skewed) | Rate (Free) |
| lfoN_rate_sync | LFO Rate (Sync) | 1/64 – 8 bars | 1 bar | Choice | Rate (Tempo) |
| lfoN_phase | LFO Phase | 0–360° | 0 | Float | Start phase (retrigger on transport) |
| env_attack | Env Attack | 1–500 ms | 10 | Float | Envelope follower attack |
| env_release | Env Release | 10–2000 ms | 200 | Float | Envelope follower release |
| env_gain | Env Gain | -24 to +24 dB | 0 | Float (dB) | Follower sensitivity |
| pitch_follow_sN (N=1–4) | Pitch Follow | Off/On | Off | Bool | Stream pitch tracks detected input pitch (pitch follower is also a matrix source) |
| onset_threshold | Onset Threshold | -60 to 0 dB | -30 | Float (dB) | Transient detection → mod pulse / grain-burst trigger |
| macroN (N=1–4) | Macro | 0–100 % | 0 | Float | Performance knobs, multi-destination |

## Modulation Matrix (16 slots, M=1–16)

| ID | Parameter | Range | Default | Type | Description |
|----|-----------|-------|---------|------|-------------|
| modM_source | Source | None / LFO1–4 / Env / Pitch / Onset / Macro1–4 | None | Choice | Mod source |
| modM_dest | Destination | None / any continuous param above | None | Choice | Mod target (continuous params only) |
| modM_depth | Depth | -100 – +100 % | 0 | Float | Bipolar depth |

## Count

- Global 6 + per-stream (21 + 10) × 4 = 124 + feedback 5 + output FX 3 + mod sources (16 + 3 + 4 + 1 + 4 = 28) + matrix 48 ≈ **214 parameters**
- Open for planning: Freeze × feedback interaction (ignore write vs overdub), pitch-follower algorithm/latency, CPU budget + grain cap, matrix-destination enumeration strategy.
