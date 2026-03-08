# O-Gain: Intelligent Gain Staging Utility

## Plugin Concept

O-Gain is a precision gain staging plugin with an intelligent "Learn" mode. It occupies a currently empty niche: the only plugin combining a one-button learn-and-set workflow with multi-standard metering and essential channel utilities in a compact, zero-latency package designed to live on every track in a session.

**Tagline:** "Measure. Learn. Set. Done."

## The Problem

Gain staging is the most fundamental step in mixing — setting every track to a consistent level before processing begins. Yet the workflow today requires engineers to:
1. Insert a VU meter on every track
2. Play each track's loudest section
3. Manually read the meter and adjust a trim knob
4. Repeat 40-100 times per session

No existing plugin automates this with a clean, purpose-built workflow. The closest competitors each cover only a subset of what's needed:
- **Klanghelm VUMT Deluxe** (22 EUR): Best metering + trim + utilities, but no auto-learn
- **HoRNet VU Meter MK4** (5.99 EUR): Auto-gain, but VU-only targeting, no utilities
- **HoRNet TheNormalizer** (9.99 EUR): Most flexible targets, but limited metering display, no utilities
- **LetiMix GainMatch** ($19): Good auto-matching, but designed for A/B comparison not gain staging

## Core Workflow

1. User inserts O-Gain as the first plugin on a track
2. Sets their target level (default: -18 dBFS / 0 VU — the industry standard for analog-modeled plugin sweet spots)
3. Presses **Learn** and plays audio (ideally the loudest/most representative section, 10-30 seconds)
4. O-Gain measures the incoming audio using K-weighted LUFS with EBU R128 gating
5. When Learn is toggled off, the plugin calculates: `gain_dB = target - measured_level`
6. The gain offset is applied as a static value — it does NOT continuously adjust like a compressor or loudness rider
7. The gain value is saved as an automatable parameter that persists with the DAW session
8. User can fine-tune with a manual trim control if desired

## Feature Set

### Primary: Auto-Learn Gain Staging
- **Learn toggle button** — start/stop measurement with visual feedback
- **Target level selector** with presets:
  - -18 dBFS (0 VU) — Standard gain staging reference (default)
  - -16 dBFS — Hotter reference for EDM/pop
  - -20 dBFS — Conservative reference for film/post
  - -14 LUFS — Spotify/YouTube streaming target
  - -16 LUFS — Apple Music streaming target
  - -23 LUFS — EBU R128 broadcast target
  - Custom value (user-defined)
- **Measurement mode**: LUFS (primary), with RMS as secondary option
- **Confidence indicator** during learn (Low / Medium / High based on analysis duration and signal variance)
- **True peak ceiling check** — warns if calculated gain would push peaks above -1 dBTP
- **Gain range limiting** — default ±24 dB, prevents destructive over-amplification

### Secondary: Multi-Mode Metering Display
- **Switchable meter modes**: Peak, RMS, VU, LUFS
- **Input and output metering** — see levels before and after gain
- **During Learn mode**: Show momentary LUFS, short-term LUFS, running integrated LUFS, elapsed time, and block count
- VU meter with authentic 300ms ballistics (per ANSI standard)

### Tertiary: Channel Utilities
- **Manual gain knob** — wide range (±40 dB), fine resolution (0.1 dB steps)
- **Trim control** — ±6 dB fine adjustment post-learn
- **Per-channel phase inversion** (L and R independently)
- **Channel swap** (L↔R)
- **Mono sum** mode
- **M/S encode/decode** mode

## Technical Architecture

### Loudness Measurement
- **Algorithm**: ITU-R BS.1770-5 compliant K-weighted loudness measurement
  - Stage 1: Pre-filter (high-shelf ~+4 dB above 2 kHz, modeling head diffraction)
  - Stage 2: RLB high-pass filter (rolloff below ~100 Hz)
  - 400ms gating blocks with 75% overlap (new block every 100ms)
- **Gating**: EBU R128 dual-gate system
  - Absolute gate at -70 LUFS (removes silence)
  - Relative gate at -10 LU below absolute-gated mean (removes unrepresentative quiet passages)
- **Implementation**: Either libebur128 (MIT, C library) or custom implementation using JUCE DSP IIR filters with published ITU coefficients
- **True peak**: 4x oversampled peak detection per BS.1770-5

### Why LUFS Over RMS
From the research, LUFS is the correct choice because:
1. **Perceptual accuracy** — K-weighting accounts for frequency-dependent loudness (equal-loudness contours), unlike flat RMS
2. **Gating** — handles silence and quiet passages that would skew RMS measurements
3. **Industry standard** — all streaming platforms and broadcast standards use LUFS
4. **1:1 dB relationship** — `target_LUFS - measured_LUFS = gain_in_dB` (no conversion needed)

The textbooks confirm: "Our ears perceive loudness in relation to the average level of sounds, not their peak level" (Izhaki). K-weighting is a practical approximation of equal-loudness perception validated across content types.

### Gain Calculation
```
gain_dB = target_LUFS - measured_LUFS
gain_dB = clamp(gain_dB, -maxGain, +maxGain)

// True peak safety check
max_safe_gain = ceiling_dBTP - measured_true_peak_dBTP
gain_dB = min(gain_dB, max_safe_gain)
```

### Thread Safety
- Audio thread: reads learn flag (atomic), accumulates samples, writes metering values (atomic floats)
- UI thread: reads metering values, toggles learn flag, reads finalized results after learn completes
- Learn accumulator is single-producer (audio thread) / single-consumer (UI thread after learn stops)
- Gain offset stored as APVTS parameter (saved with session, automatable)
- Learn mode is transient UI state (NOT saved — reloading a session should NOT restart learning)

### Parameters (APVTS)
| Parameter | Range | Default | Saved | Purpose |
|-----------|-------|---------|-------|---------|
| `gain_offset` | -40 to +40 dB | 0 | Yes | Calculated by learn mode (also manually adjustable) |
| `trim` | -6 to +6 dB | 0 | Yes | Manual fine adjustment |
| `target_level` | -36 to 0 | -18 | Yes | Target level for learn mode |
| `measurement_mode` | LUFS / RMS | LUFS | Yes | Which metric to use |
| `meter_mode` | Peak / RMS / VU / LUFS | VU | Yes | Display meter type |
| `phase_invert_l` | On/Off | Off | Yes | Left channel polarity |
| `phase_invert_r` | On/Off | Off | Yes | Right channel polarity |
| `channel_swap` | On/Off | Off | Yes | L↔R swap |
| `mono_sum` | On/Off | Off | Yes | Mono summing |
| `ms_mode` | Off/Encode/Decode | Off | Yes | Mid-Side processing |

### Signal Flow
```
Input → [Phase Invert] → [Channel Swap] → [M/S Encode] → [Mono Sum]
     → [Learn Measurement (pre-gain, K-weighted)]
     → [Apply Gain (gain_offset + trim)]
     → [Output Metering]
     → Output
```

Note: Learn measurement happens on the INPUT signal (before gain) so the measurement reflects the true incoming level, not a feedback loop of the plugin's own gain.

### Processing
- **Zero latency** — no lookahead or FFT required
- **Minimal CPU** — the DSP is trivially simple (gain multiplication). The measurement is the complex part, and it only runs during Learn mode
- **64-bit internal processing** for gain calculations

## Gain Staging Context from Professional Literature

The textbooks unanimously support these principles:

1. **-18 dBFS / 0 VU is THE standard** — "Calibrate 0 VU to -18 dBpeak as a starting point" (Stavrou). This is where analog-modeled plugins operate at their sweet spot.

2. **Average level matters more than peak** — "Our ears perceive loudness in relation to the average level of sounds, not their peak level" (Izhaki). This is why LUFS/RMS-based targeting is correct over peak-based.

3. **Headroom is essential** — "Headroom means that our average level might be -10 dB or less on the meter, leaving plenty of room for transients above that" (Owsinski). O-Gain's true peak ceiling check enforces this.

4. **Gain staging is every engineer's first step** — "Set the signal to optimum level as early as possible in the signal chain, and keep it there" (Izhaki). O-Gain is purpose-built for this moment.

5. **Loudness bias is real** — "The louder version will almost always be preferred regardless of whether it actually sounds better" (Senior). O-Gain helps establish consistent reference levels that eliminate this bias.

## UI Vision

Compact, professional interface suitable for placement on every track:
- Central gain knob (large, clear readout in dB)
- Meter display (switchable mode) flanking or below the knob
- Learn button with visual state indicator (idle/learning/complete)
- Target level display with preset selector
- Utility buttons row (phase L, phase R, swap, mono, M/S)
- Confidence indicator visible during/after learn
- Input/output level readouts

The UI should be narrow enough to not crowd the mixer view when instantiated on 40+ tracks.

## Minimum Viable Product (MVP) vs Full Feature Set

### MVP (v1.0)
- Manual gain knob (±40 dB)
- Learn mode with LUFS measurement and auto-gain calculation
- Target level selector with common presets
- Basic level metering (peak + RMS)
- Phase inversion (L/R)
- True peak ceiling check

### v1.1+
- VU meter with authentic ballistics
- LUFS metering display (momentary/short-term/integrated)
- Channel swap, mono sum, M/S mode
- Confidence indicator with detailed feedback
- Measurement mode selector (LUFS vs RMS)
- Gain range limiting UI

## Research Sources

Full research documents available at:
- `research/gain-staging-plugin-market-research.md` — Competitive landscape analysis
- `research/gain-staging-metering-loudness-textbook-research.md` — Professional literature synthesis
- (Loudness algorithm research and DSP architecture research captured in agent transcripts)
