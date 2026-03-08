# O-Gain Parameter Specification (Draft)

Extracted from BRIEF.md for Stage 0 planning.

## Parameters

| Parameter | ID | Type | Range | Default | Saved | Purpose |
|-----------|-----|------|-------|---------|-------|---------|
| Gain Offset | `gain_offset` | Float | -40 to +40 dB | 0 | Yes | Calculated by learn mode (also manually adjustable) |
| Trim | `trim` | Float | -6 to +6 dB | 0 | Yes | Manual fine adjustment |
| Target Level | `target_level` | Float | -36 to 0 | -18 | Yes | Target level for learn mode |
| Measurement Mode | `measurement_mode` | Choice | LUFS / RMS | LUFS | Yes | Which metric to use |
| Meter Mode | `meter_mode` | Choice | Peak / RMS / VU / LUFS | VU | Yes | Display meter type |
| Phase Invert L | `phase_invert_l` | Bool | On/Off | Off | Yes | Left channel polarity |
| Phase Invert R | `phase_invert_r` | Bool | On/Off | Off | Yes | Right channel polarity |
| Channel Swap | `channel_swap` | Bool | On/Off | Off | Yes | L↔R swap |
| Mono Sum | `mono_sum` | Bool | On/Off | Off | Yes | Mono summing |
| M/S Mode | `ms_mode` | Choice | Off/Encode/Decode | Off | Yes | Mid-Side processing |

## Non-Automatable State (UI Only)

| State | Type | Purpose |
|-------|------|---------|
| Learn Active | Bool | Toggle measurement on/off (transient, NOT saved) |
| Confidence | Enum (Low/Med/High) | Measurement quality indicator |
| Elapsed Time | Float | Learn duration display |

## Notes

- `gain_offset` is the primary output of the learn workflow
- Learn mode is transient UI state — NOT saved with session
- All parameters are APVTS-managed and automatable
- Zero latency — no lookahead parameters needed
