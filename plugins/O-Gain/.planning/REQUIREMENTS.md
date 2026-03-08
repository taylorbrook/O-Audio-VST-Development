# O-Gain Requirements

## Functional Requirements

### FR-1: Manual Gain Control
- Gain knob with range ±40 dB, 0.1 dB resolution
- Trim control with range ±6 dB, 0.1 dB resolution
- Total applied gain = gain_offset + trim
- Zero-latency processing

### FR-2: Learn Mode (Auto-Gain)
- Toggle button to start/stop learning
- During learn: accumulate K-weighted LUFS measurement (ITU-R BS.1770-5)
- EBU R128 dual-gate: absolute at -70 LUFS, relative at -10 LU
- On stop: calculate gain_dB = target - measured_integrated_LUFS
- Apply calculated gain to gain_offset parameter
- Minimum 1 second of gated audio required; warn below 3 seconds
- Gain clamped to ±24 dB by default

### FR-3: Target Level Selection
- User-configurable target level (-36 to 0 dB)
- Presets: -18 dBFS (default), -16 dBFS, -20 dBFS, -14 LUFS, -16 LUFS, -23 LUFS
- Target persists across learn cycles (user preference)

### FR-4: Metering Display
- Switchable meter modes: Peak, RMS, VU, LUFS
- Input level display (pre-gain)
- Output level display (post-gain)
- VU mode: 300ms ballistics per ANSI standard

### FR-5: Learn Feedback
- Visual indicator: idle / learning / complete states
- During learn: elapsed time, running integrated level, confidence (Low/Med/High)
- After learn: display measured level and applied gain offset

### FR-6: True Peak Safety
- Track true peak during learn (4x oversampled)
- After gain calculation: check if gain + true_peak > ceiling (-1 dBTP default)
- If clipping would occur: warn user, optionally cap gain at safe maximum

### FR-7: Channel Utilities
- Per-channel phase inversion (L and R independently)
- Channel swap (L↔R)
- Mono sum mode
- M/S encode/decode mode

### FR-8: Measurement Mode
- Primary: LUFS (K-weighted, gated)
- Secondary: RMS (flat, gated)
- User-selectable

## Non-Functional Requirements

### NFR-1: Performance
- Zero added latency
- CPU usage < 1% per instance (gain is trivial; measurement only during learn)
- Must be viable on 40+ simultaneous instances

### NFR-2: Thread Safety
- Learn accumulator: single-producer (audio thread) / single-consumer (UI thread)
- All cross-thread communication via atomics
- No locks on audio thread

### NFR-3: Persistence
- gain_offset, trim, target_level, all utility states saved via APVTS
- Learn mode is transient (NOT saved with session)
- Gain value persists across session reload

### NFR-4: Compatibility
- VST3 and AU formats
- macOS (Intel + Apple Silicon) and Windows
- Sample rates: 44.1, 48, 88.2, 96, 176.4, 192 kHz
- Mono and stereo input configurations

## Parameters

| ID | Name | Range | Default | Type |
|----|------|-------|---------|------|
| `gain_offset` | Gain | -40 to +40 dB | 0 | Float |
| `trim` | Trim | -6 to +6 dB | 0 | Float |
| `target_level` | Target | -36 to 0 | -18 | Float |
| `measurement_mode` | Measure | LUFS=0, RMS=1 | 0 | Choice |
| `meter_mode` | Meter | Peak=0, RMS=1, VU=2, LUFS=3 | 2 | Choice |
| `phase_invert_l` | Phase L | Off/On | Off | Bool |
| `phase_invert_r` | Phase R | Off/On | Off | Bool |
| `channel_swap` | Swap L/R | Off/On | Off | Bool |
| `mono_sum` | Mono | Off/On | Off | Bool |
| `ms_mode` | M/S Mode | Off=0, Encode=1, Decode=2 | 0 | Choice |
