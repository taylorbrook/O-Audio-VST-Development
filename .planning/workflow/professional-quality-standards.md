# Professional Quality Standards

Standards defining commercial-grade plugin quality. Used by critics, quality gates, and validation workflows.

## DSP Quality Standards

### Core Mandatory (MUST PASS)

| Metric | Requirement | Test Method |
|--------|-------------|-------------|
| DC Offset | < 0.001 at output | Process silence, measure offset |
| Digital Clipping | None at 0dBFS input | Process full-scale sine, check output |
| Null Test | Passes for bypass mode | Compare input/output in bypass |
| Silence | Produces silence for silent input | Feed zeros, measure output |

### Recommended Thresholds (TARGET)

| Metric | Professional Target | Acceptable | Test Method |
|--------|---------------------|------------|-------------|
| THD+N | < 0.005% | < 0.01% | Sine sweep analysis |
| SNR | > 100dB | > 90dB | Silence floor measurement |
| Dynamic Range | > 110dB | > 100dB | Peak to noise ratio |
| Frequency Response | +/- 0.1dB 20Hz-20kHz | +/- 0.5dB | Sweep analysis |

### Reference Comparison

- Output quality should match commercial plugins (FabFilter, Soundtoys, u-he level)
- A/B testing against reference implementations when applicable
- CPU usage competitive with similar commercial plugins

### Real-Time Performance

| Metric | Requirement |
|--------|-------------|
| CPU per instance | < 5% at 44.1kHz stereo on modern CPU |
| Latency | Zero latency for effects (unless documented) |
| Buffer underruns | Zero at reasonable buffer sizes (256+) |

## UI Quality Standards

### Visual Consistency (REQUIRED)

| Element | Requirement | Validation |
|---------|-------------|------------|
| Spacing | Consistent margins/padding throughout | Visual inspection |
| Alignment | Elements on consistent grid | Pixel measurement |
| Font Hierarchy | Clear size progression (title > label > value) | Font size audit |
| Color Palette | Systematic, limited palette (3-5 colors + neutrals) | Color extraction |

### Interaction Quality (RECOMMENDED)

| Element | Requirement | Validation |
|---------|-------------|------------|
| Animations | Smooth (60fps) or instant, never stuttery | Frame timing |
| Responsive Controls | < 16ms response time | Input latency measurement |
| Hover/Focus States | All interactive elements | Manual inspection |
| Keyboard Navigation | Tab order logical | Keyboard testing |

### DAW Compatibility (REQUIRED)

| DAW | Format | Minimum Version | Validation |
|-----|--------|-----------------|------------|
| Logic Pro | AU | 10.7+ | auval -v |
| Ableton Live | VST3/AU | 11+ | Load test |
| Pro Tools | AAX | 2022+ | Load test |
| Cubase | VST3 | 12+ | Load test |

### Validation Tools

| Tool | Purpose | Minimum |
|------|---------|---------|
| pluginval | General validation | Strictness 10 |
| auval | macOS AU validation | Pass |
| VST3 SDK validator | VST3 compliance | Pass |

## Quality Gate Integration

These standards integrate with quality gates:

- **Stage 2 (DSP):** Core mandatory metrics checked
- **Stage 3 (UI):** Visual consistency and DAW compatibility checked
- **Stage 4 (Validation):** Full metric suite including recommended thresholds

## Measurement Procedures

### DC Offset Test
1. Generate 10 seconds of silence
2. Process through plugin with all parameters at default
3. Measure mean output value
4. PASS: |mean| < 0.001

### THD+N Test
1. Generate 1kHz sine at -6dBFS
2. Process through plugin (neutral settings)
3. Measure harmonic content
4. PASS: THD+N < 0.01%

### Null Test
1. Generate test signal (pink noise, 10 seconds)
2. Process with bypass enabled
3. Subtract output from input
4. PASS: Difference < -120dB

---
*Version: 1.0*
*Created: Phase 6 Domain Specialization*
