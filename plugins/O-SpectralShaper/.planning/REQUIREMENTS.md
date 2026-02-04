# O-SpectralShaper Requirements

*Auto-extracted from BRIEF.md — 2026-02-03*

## Functional Requirements

### FR-1: Spectral Analysis
- FR-1.1: Perform real-time FFT analysis with ~512-sample window size
- FR-1.2: Divide spectrum into 32 logarithmically-spaced frequency bands
- FR-1.3: Maintain continuous spectrogram display (time × frequency)
- FR-1.4: Support sample rates from 44.1kHz to 192kHz

### FR-2: Transient Detection
- FR-2.1: Detect transients independently per frequency band
- FR-2.2: Adjustable sensitivity threshold (0-100%)
- FR-2.3: Visual feedback showing detected transients on spectrogram (heat overlay)
- FR-2.4: Lookahead buffer for clean attack capture (0-10ms configurable)

### FR-3: Transient Shaping
- FR-3.1: Independent attack shaping curve across 32 bands
- FR-3.2: Independent sustain shaping curve across 32 bands
- FR-3.3: Global attack time multiplier (0.1-50ms)
- FR-3.4: Global sustain release time (10-500ms)
- FR-3.5: Per-band envelope application based on curve values

### FR-4: Curve Editing
- FR-4.1: Freehand drawing mode with automatic smoothing
- FR-4.2: Node-based editing mode with bezier handles
- FR-4.3: Toggle between freehand and node modes
- FR-4.4: Visual distinction between attack and sustain curves
- FR-4.5: Curves persist across sessions (state save/restore)

### FR-5: Global Controls
- FR-5.1: Mix control (0-100% wet/dry)
- FR-5.2: Output gain (-12dB to +12dB)
- FR-5.3: All parameters automatable via DAW

### FR-6: Visualization
- FR-6.1: Scrolling spectrogram with logarithmic frequency axis
- FR-6.2: Transient heat overlay (color-coded intensity)
- FR-6.3: Real-time update at display refresh rate
- FR-6.4: Clear visual distinction between analysis and curve editing areas

## Non-Functional Requirements

### NFR-1: Performance
- NFR-1.1: Total system latency ≤10ms (target 5ms)
- NFR-1.2: CPU usage <50% single core at 44.1kHz stereo
- NFR-1.3: Smooth UI at 60fps without audio dropouts

### NFR-2: Audio Quality
- NFR-2.1: No audible artifacts from FFT processing
- NFR-2.2: Phase-coherent output (minimize spectral smearing)
- NFR-2.3: Clean attack transients without pre-ringing

### NFR-3: Compatibility
- NFR-3.1: VST3 and AU formats
- NFR-3.2: macOS 10.15+ (primary), Windows 10+ (future)
- NFR-3.3: 64-bit only

### NFR-4: User Experience
- NFR-4.1: Ouaricon dark theme consistency
- NFR-4.2: Immediate audible feedback when drawing curves
- NFR-4.3: Responsive curve editing (<16ms input-to-visual latency)

## Parameter Specification

| ID | Parameter | Type | Range | Default | Automation |
|----|-----------|------|-------|---------|------------|
| P1 | Mix | Float | 0.0-1.0 | 1.0 | Yes |
| P2 | Attack Time | Float | 0.1-50ms | 10ms | Yes |
| P3 | Sustain Time | Float | 10-500ms | 100ms | Yes |
| P4 | Sensitivity | Float | 0.0-1.0 | 0.5 | Yes |
| P5 | Lookahead | Float | 0-10ms | 2ms | Yes |
| P6 | Output Gain | Float | -12 to +12dB | 0dB | Yes |
| P7-P38 | Attack Curve [32] | Float[] | -1.0 to +1.0 | 0.0 | No* |
| P39-P70 | Sustain Curve [32] | Float[] | -1.0 to +1.0 | 0.0 | No* |

*Curve data stored as plugin state, not individual automatable parameters

## Constraints

- **C1:** FFT overlap-add must maintain time-domain continuity
- **C2:** Per-band detection must not introduce inter-band artifacts
- **C3:** Lookahead cannot exceed FFT window size
- **C4:** WebView UI must synchronize curve state with audio thread safely

## Dependencies

- JUCE 8.x framework
- WebView for UI (consistent with O-* plugin family)
- FFT implementation (JUCE built-in or FFTW if needed)

---

*Validation: Requirements trace back to BRIEF.md sections*
