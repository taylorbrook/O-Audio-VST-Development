# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-02-03
**Participants:** User, Claude

## Requirements Confirmed

From ARCHITECTURE.md and user discussion:

### FFT Infrastructure
- **FFT Size:** 2048 samples (fixed for v1.0)
- **Overlap:** 4× (75% overlap, hop size 512)
- **Window:** Hann (satisfies COLA constraint)
- **Latency:** Report 2048 samples to host via `setLatencySamples()`

### Band Processing
- **Bands:** 4 configurable (Sub, Low, Mid, High)
- **Crossover:** Hard cutoff (bins belong to exactly one band)
- **Gain Application:** Scale magnitude, preserve phase
- **Bin Mapping:** Pre-calculate lookup table on frequency change

### Step Sequencer Engine
- **Tempo Sync:** Via `AudioPlayHead` (BPM, PPQ position)
- **Rate Options:** 1/1 through 1/32, plus triplets and dotted
- **Swing:** Apply to odd-numbered steps (delay by swing amount)
- **Transport Stopped:** Freeze on current step position

### Euclidean Generator
- **Algorithm:** Bresenham/bucket-fill (efficient, no recursion)
- **Parameters:** Steps (1-32), Pulses (1-32), Offset (0-31)
- **Update Timing:** Immediate (no bar quantization)
- **Mode Toggle:** Per-band Manual ↔ Euclidean switch

### Smoothing & Mixing
- **Smoothing Strategy:** Per-band `juce::SmoothedValue` (4 smoothers total)
- **Smoothing Range:** 0-100ms (user parameter, default 5ms)
- **Dry/Wet:** Use `juce::dsp::DryWetMixer`

## Constraints Identified

1. **Latency:** ~46ms at 44.1kHz is acceptable (mixing use case, DAW PDC handles it)
2. **CPU Target:** <5% on Apple Silicon at 44.1kHz stereo
3. **Real-time Safety:** No allocations in `processBlock()`, pre-allocate all buffers
4. **Phase Preservation:** Critical for sound quality - multiply both real/imag by same gain

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Smoothing | Per-band SmoothedValue | Simple, efficient, 4 smoothers vs 1025 (per-bin) |
| Transport stopped | Freeze current step | Predictable, preserves pattern state |
| CPU target | <5% | Achievable with standard optimizations |
| Edge case handling | Standard only | Keep scope focused for v1.0 |
| Audio quality | Balanced | COLA + Hann + phase preservation |
| Euclidean updates | Immediate | Responsive UI, simpler implementation |

## Implementation Approach

### Processing Flow
```
Input → Push to DryWetMixer → STFT Buffer → FFT → Band Gains → IFFT → Overlap-Add → Mix → Output
```

### Key Components to Implement

1. **STFT Processor Class** (or inline in processBlock)
   - Input ring buffer (accumulate samples)
   - FFT frame buffer (2048 floats)
   - Output overlap-add buffer
   - Hann window table

2. **Bin-to-Band Mapper**
   - Pre-calculated array: `int bandForBin[1025]`
   - Recalculate when band frequencies change
   - Handle gaps (frequencies not in any band → pass through at 100%)

3. **Step Sequencer State**
   - Current step index (per band if polymetric, global for v1.0)
   - Last PPQ position (for step calculation)
   - Swing offset calculation

4. **Euclidean Pattern Cache**
   - `std::array<std::array<bool, 32>, 4>` for current patterns
   - Regenerate when euc_steps/pulses/offset changes
   - Listener pattern or check-on-read

5. **Gain Smoothers**
   - `std::array<juce::SmoothedValue<float>, 4>` for 4 bands
   - Reset in `prepareToPlay()` with sample rate and smoothing time

## Open Questions

None - all key decisions resolved.

## Stage 2 Deliverables

1. **FFT processing produces audible spectral gating**
2. **Tempo-synced step sequencing works** (verified with playhead)
3. **Euclidean patterns generate correctly**
4. **No clicks/pops during gate transitions**
5. **Latency reported to DAW** (2048 samples)

## Testing Strategy

| Test | Method |
|------|--------|
| FFT quality | Sine sweep (check for phase issues) |
| Transient handling | Drum loops (verify attack preservation) |
| Tempo sync | Compare visual playhead to DAW grid |
| Euclidean correctness | Known patterns (3,8) = [X..X..X.] |
| Smoothing | Fast toggle, listen for clicks |
| CPU | Profile at 96kHz (worst case) |

## Next Phase

Ready for: **research** phase (gather JUCE API specifics) or **plan** phase (if architecture is sufficient)

---

*Generated: 2026-02-03 via /plugin-discuss*
