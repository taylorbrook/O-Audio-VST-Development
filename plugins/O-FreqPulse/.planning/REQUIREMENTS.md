# O-FreqPulse - Requirements

## Functional Requirements

### FR-1: Spectral Band Processing
- FR-1.1: Process audio through FFT analysis (2048 samples default)
- FR-1.2: Divide spectrum into 4 configurable frequency bands
- FR-1.3: Apply independent gain modulation per band
- FR-1.4: Reconstruct audio via overlap-add synthesis
- FR-1.5: Report latency to host DAW for compensation

### FR-2: Step Sequencer
- FR-2.1: Support 4, 8, 16, and 32 step sequence lengths
- FR-2.2: Tempo-sync to host (1/1 through 1/32, triplets, dotted)
- FR-2.3: Per-step on/off state for each band
- FR-2.4: Per-step gain level (0-100%) for each band
- FR-2.5: Visual playhead showing current step position
- FR-2.6: Swing control (0-100%) for groove

### FR-3: Euclidean Rhythm Generation
- FR-3.1: Generate Euclidean patterns per band
- FR-3.2: Configurable steps (1-32), pulses (1-32), offset (0-31)
- FR-3.3: Real-time pattern update when parameters change
- FR-3.4: Toggle between Manual and Euclidean mode per band

### FR-4: Band Configuration
- FR-4.1: Adjustable frequency crossover points per band
- FR-4.2: Band enable/bypass toggle
- FR-4.3: Depth control (how much gain reduction on "off" steps)
- FR-4.4: Default band presets (Sub/Low/Mid/High)

### FR-5: Global Controls
- FR-5.1: Mix (dry/wet) control 0-100%
- FR-5.2: Smoothing control for attack/release (0-100ms)
- FR-5.3: Master enable/bypass

### FR-6: Visual Grid Interface
- FR-6.1: Display frequency × time grid
- FR-6.2: Show active steps per band with color coding
- FR-6.3: Visual playhead synchronized to host transport
- FR-6.4: Click-to-toggle step editing
- FR-6.5: Logarithmic frequency scale display

## Non-Functional Requirements

### NFR-1: Performance
- NFR-1.1: CPU usage <5% on Apple Silicon at 44.1kHz stereo
- NFR-1.2: Memory footprint <50MB
- NFR-1.3: SIMD optimization for FFT bin processing

### NFR-2: Latency
- NFR-2.1: Report accurate latency to host
- NFR-2.2: Default latency ~46ms (2048 FFT at 44.1kHz)
- NFR-2.3: (Future) Low-latency mode option

### NFR-3: Audio Quality
- NFR-3.1: No audible artifacts from FFT processing
- NFR-3.2: Smooth gain transitions (no clicks/pops)
- NFR-3.3: Minimal spectral leakage at band boundaries

### NFR-4: Compatibility
- NFR-4.1: VST3 and AU format support
- NFR-4.2: macOS 10.15+ (Intel and Apple Silicon)
- NFR-4.3: Sample rates: 44.1kHz, 48kHz, 88.2kHz, 96kHz

### NFR-5: Usability
- NFR-5.1: Intuitive grid interaction (click to toggle)
- NFR-5.2: Visual feedback within 16ms of interaction
- NFR-5.3: Clear mode indication (Manual vs Euclidean)

## Parameter Summary

### Global (5 parameters)
| ID | Name | Range | Default |
|----|------|-------|---------|
| mix | Mix | 0-100% | 100% |
| steps | Steps | 4/8/16/32 | 16 |
| rate | Rate | 1/1 to 1/32 | 1/16 |
| swing | Swing | 0-100% | 0% |
| smoothing | Smoothing | 0-100ms | 5ms |

### Per-Band (8 parameters × 4 bands = 32 parameters)
| ID Pattern | Name | Range | Default |
|------------|------|-------|---------|
| band{N}_enable | Band N Enable | On/Off | On |
| band{N}_low | Band N Low Freq | 20-20kHz | varies |
| band{N}_high | Band N High Freq | 20-20kHz | varies |
| band{N}_depth | Band N Depth | 0-100% | 100% |
| band{N}_euclidean_on | Band N Euclidean | On/Off | Off |
| band{N}_euclidean_steps | Band N Euc Steps | 1-32 | 16 |
| band{N}_euclidean_pulses | Band N Euc Pulses | 1-32 | 8 |
| band{N}_euclidean_offset | Band N Euc Offset | 0-31 | 0 |

### Step Grid (1 parameter per step per band)
- step_b{N}_s{M}: Band N, Step M on/off state
- For 4 bands × 32 max steps = 128 step parameters

**Total: ~165 parameters** (5 global + 32 band + 128 step)

## Default Band Frequencies

| Band | Name | Low Freq | High Freq |
|------|------|----------|-----------|
| 1 | Sub | 20 Hz | 120 Hz |
| 2 | Low | 120 Hz | 500 Hz |
| 3 | Mid | 500 Hz | 4000 Hz |
| 4 | High | 4000 Hz | 20000 Hz |

## Acceptance Criteria

1. **AC-1:** Loading plugin on a drum loop, enabling all bands with default Euclidean (8 pulses in 16 steps), produces rhythmic chopping effect
2. **AC-2:** Setting Band 1 (Sub) to 4 pulses and Band 4 (High) to 12 pulses creates audibly different rhythmic patterns per frequency range
3. **AC-3:** Adjusting smoothing from 0ms to 50ms eliminates any clicking artifacts
4. **AC-4:** Playhead visually tracks host transport accurately
5. **AC-5:** Mix at 50% produces balanced blend of processed and dry signal

---

*Extracted from BRIEF.md: 2026-02-03*
