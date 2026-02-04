# SpectralToolbox Plugin Synopses

*Generated from deep research: 2026-02-02*

---

## 1. O-SpectralShaper (Spectral Transient Shaper)

**One-liner:** Per-frequency transient control with real-time visual feedback - boost the kick's low-end punch without adding click, or add snare crack only where it matters.

### Concept

Traditional transient shapers treat the whole signal (or a few fixed bands) identically. O-SpectralShaper uses FFT analysis to detect and shape transients at specific frequencies independently. See exactly where transients occur on a spectrogram, then surgically boost or cut attack/sustain per frequency region.

### Target Users
- Mix engineers wanting surgical drum control
- Producers shaping instrument attacks (guitar pick, piano hammer)
- Sound designers creating punchy or soft textures

### Key Differentiators
1. **Spectrogram + transient overlay** - Color-coded visualization shows where transients are detected in real-time
2. **Drawable attack/sustain curves** - Draw boost/cut amounts across the frequency spectrum (like an EQ, but for transients)
3. **Spectral masks** - Presets for common sources (kick attack, snare crack, hi-hat bite, guitar pick)
4. **32 logarithmic bands** - More resolution than competitors' 5-8 bands

### Core Parameters
- **Attack:** -24 to +24 dB (transient boost/cut)
- **Sustain:** -24 to +24 dB (body boost/cut)
- **Sensitivity:** How easily transients are detected
- **Speed:** Envelope timing (0.1-50ms)
- **Mix:** Wet/dry blend

### Technical Approach
- 1024-point FFT with 75% overlap (~23ms latency)
- Spectral flux detection per band
- Dual envelope followers (fast ~1ms, slow ~15ms)
- Optional lookahead for precise alignment

### Competitive Landscape
- oeksound Spiff ($149) - Good but limited visual feedback
- Eventide SplitEQ ($179) - Only 2 paths, not per-frequency
- Sonible entropy:EQ+ ($129) - EQ paradigm, only 8 bands

**Estimated price point:** $49-79

---

## 2. O-FreqPulse (Spectral Sequencer)

**One-liner:** A rhythmic gate that knows about frequency - pulse your highs at 1/16, keep your lows solid, and paint spectral patterns on a 2D grid.

### Concept

Traditional trance gates affect the whole signal uniformly. O-FreqPulse combines FFT spectral processing with step sequencing, allowing different rhythmic patterns for different frequency regions. Paint patterns on a frequency × time grid, generate Euclidean rhythms per band, or create slowly evolving spectral animations.

### Target Users
- EDM producers wanting frequency-aware rhythmic effects
- Ambient artists creating evolving pad textures
- Sound designers seeking experimental spectral manipulation

### Key Differentiators
1. **2D visual grid** - Paint patterns on a frequency (Y) × time (X) canvas
2. **Euclidean per-band** - Mathematical rhythm generation for each frequency region
3. **Multiple polyrhythmic lanes** - Independent rates per frequency range (like O-Polystutter, but spectral)
4. **Spectral freeze steps** - Certain steps can hold/freeze the spectrum
5. **Drawable masks** - Per-step frequency curves that morph over time

### Core Parameters
- **Steps:** 1-32 steps per pattern
- **Rate:** 1/1 to 1/32 (tempo-synced)
- **Bands:** 8-32 frequency regions
- **Attack/Release:** Per-step envelope (0-100ms)
- **Depth:** Gate intensity (0-100%)
- **Mix:** Wet/dry blend

### Technical Approach
- 1024-point FFT with 75% overlap
- Tempo sync via AudioPlayHead (proven in O-Polystutter)
- Per-bin gain application with smoothing
- Crossfade between steps to prevent clicks

### Competitive Landscape
- Sinevibes Array v4 ($29) - Only 8 octave bands, no visual grid
- Unfiltered Audio SpecOps (premium) - Complex, not rhythm-focused
- Traditional trance gates - No frequency selectivity

**Estimated price point:** $29-49

---

## Shared Foundation

Both plugins share a core STFT (Short-Time Fourier Transform) engine:

```
Input → Window → FFT → [Spectral Processing] → IFFT → Window → Overlap-Add → Output
```

Building one creates reusable infrastructure for the other. Recommended approach: build O-FreqPulse first (faster MVP, clearer gap), then leverage the STFT engine for O-SpectralShaper.

---

## Quick Comparison

| Aspect | O-SpectralShaper | O-FreqPulse |
|--------|------------------|-------------|
| Problem solved | Surgical transient control | Rhythmic spectral animation |
| Complexity | Higher | Medium |
| MVP timeline | 4-5 weeks | 2-3 weeks |
| Market gap | Moderate (competitors exist) | Strong (no real competitor) |
| Price potential | Higher ($49-79) | Lower ($29-49) |
| Wow factor | Technical/professional | Creative/visual |

---

*Full research documents available at:*
- `research/spectral-transient-shaper-research.md`
- `research/spectral-sequencer-research.md`
