# O-MultiBandCompressor - Creative Brief

## Core Concept

A professional 4-band multiband compressor designed for mixing and mastering workflows. Features Linkwitz-Riley crossovers, per-band sidechain filtering, Peak/RMS blend detection, and Mid/Side processing - all wrapped in the Botanical/Ouaricon aesthetic.

## Plugin Identity

- **Name:** O-MultiBandCompressor
- **Type:** Audio Effect (Dynamics Processor)
- **Category:** Mixing/Mastering
- **Aesthetic:** Botanical/Ouaricon (organic textures, botanical illustrations, warm tones)

## Technical Specifications

### Band Architecture
- **Band Count:** 4 bands (Low / Low-Mid / High-Mid / High)
- **Architecture:** N-band scalable design (easy expansion to 5+ bands in future versions)
- **Default Crossover Points:**
  - Low/Low-Mid: 200 Hz
  - Low-Mid/High-Mid: 2 kHz
  - High-Mid/High: 8 kHz
- **Crossover Type:** Linkwitz-Riley 24dB/octave (flat summing, industry standard)

### Per-Band Compression Controls
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Threshold | -60 to 0 dB | -20 dB | Level where compression begins |
| Ratio | 1:1 to 20:1 | 4:1 | Compression ratio |
| Attack | 0.1 to 200 ms | 10 ms | Attack time |
| Release | 10 to 2000 ms | 100 ms | Release time |
| Knee | 0 to 24 dB | 6 dB | Soft knee width |
| Makeup Gain | -12 to +24 dB | 0 dB | Output gain compensation |
| Peak/RMS Blend | 0-100% | 50% | Detection mode (0=Peak, 100=RMS) |
| Solo | On/Off | Off | Solo this band |
| Bypass | On/Off | Off | Bypass this band |

### Per-Band Sidechain Filter
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| SC HPF | 20 Hz to 2 kHz | Off | High-pass filter on sidechain |
| SC LPF | 500 Hz to 20 kHz | Off | Low-pass filter on sidechain |
| SC Listen | On/Off | Off | Monitor the sidechain signal |

### Global Controls
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Input Gain | -24 to +24 dB | 0 dB | Input level trim |
| Output Gain | -24 to +24 dB | 0 dB | Master output level |
| Dry/Wet Mix | 0-100% | 100% | Parallel compression blend |
| Auto-Makeup | On/Off | Off | Automatic gain compensation |
| Mid/Side Mode | Off/Mid/Side/Both | Off | Stereo processing mode |
| Crossover 1 | 20 Hz - 500 Hz | 200 Hz | Low/Low-Mid split point |
| Crossover 2 | 200 Hz - 5 kHz | 2 kHz | Low-Mid/High-Mid split point |
| Crossover 3 | 2 kHz - 16 kHz | 8 kHz | High-Mid/High split point |

### Mid/Side Processing
When enabled, each band processes:
- **Mid Mode:** Only the center/mono content
- **Side Mode:** Only the stereo width content
- **Both Mode:** Independent compression for mid and side per band (8 compressors total)

## Visual Design

### Aesthetic Direction
- Botanical/Ouaricon style consistent with O-series plugins
- Warm, organic color palette
- Subtle paper/parchment textures
- Botanical illustrations as decorative elements
- Hand-drawn feel for knobs and UI elements

### Metering & Visualization
- **Spectrum Analyzer:** Real-time FFT display showing frequency content
- **Band Visualization:** Colored regions showing each band's frequency range
- **Gain Reduction Meters:** Per-band vertical GR meters (0 to -24 dB)
- **Input/Output Meters:** Stereo peak + RMS meters
- **Crossover Handles:** Draggable points on spectrum to adjust crossover frequencies

### Layout Concept
```
+------------------------------------------------------------------+
|  [Input Meter]     SPECTRUM ANALYZER + BAND OVERLAY    [Out Meter]|
|                   (draggable crossover points)                    |
+------------------------------------------------------------------+
|   LOW BAND    |  LOW-MID BAND  |  HIGH-MID BAND  |   HIGH BAND   |
|   [GR Meter]  |   [GR Meter]   |    [GR Meter]   |   [GR Meter]  |
|   Threshold   |   Threshold    |    Threshold    |   Threshold   |
|   Ratio       |   Ratio        |    Ratio        |   Ratio       |
|   Attack      |   Attack       |    Attack       |   Attack      |
|   Release     |   Release      |    Release      |   Release     |
|   Makeup      |   Makeup       |    Makeup       |   Makeup      |
|   Peak/RMS    |   Peak/RMS     |    Peak/RMS     |   Peak/RMS    |
|   [SC] [Solo] |   [SC] [Solo]  |    [SC] [Solo]  |   [SC] [Solo] |
+------------------------------------------------------------------+
|  [Input Gain]   [Mix]   [Auto-Makeup]   [M/S Mode]   [Output Gain]|
+------------------------------------------------------------------+
```

## DSP Implementation Notes

### Crossover Network
- Linkwitz-Riley 4th order (24dB/oct) using cascaded Butterworth filters
- Each crossover point uses complementary LP/HP pair
- Sum to unity gain across all frequencies
- Adjustable crossover frequencies with smooth parameter changes

### Compression Algorithm
- Feed-forward compressor topology
- Smooth gain computer with soft knee
- Peak/RMS blend using parallel envelope followers
- Log-domain gain calculation for musical response

### Latency Considerations
- Lookahead: Optional 0-5ms for transparent limiting
- Expected total latency: ~5-10ms (crossover + lookahead)
- Report latency to host for PDC

## Target Specifications

- **Sample Rates:** 44.1 kHz to 192 kHz
- **Bit Depth:** 32-bit float internal processing
- **Formats:** VST3, AU
- **Window Size:** ~900 x 600 px (resizable)
- **CPU Target:** < 5% on modern CPU at 44.1kHz stereo

## Future Expansion Possibilities

- 5-band or 6-band mode toggle
- Linear phase crossover option
- External sidechain input per band
- Preset morphing/interpolation
- Analyzer hold/freeze

## Success Criteria

1. Clean, transparent compression suitable for mastering
2. Intuitive workflow with visual feedback
3. Consistent Ouaricon aesthetic
4. CPU efficient for mix bus use
5. Accurate metering and gain staging

---

*Created: 2026-01-25*
*Status: Ideated*
