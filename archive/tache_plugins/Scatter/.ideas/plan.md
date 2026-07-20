# Scatter - Implementation Plan

**Date:** 2025-11-13
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 9 parameters (9/5 = 1.8 points, capped at 2.0) = **1.8**
- **Algorithms:** 10 DSP components = **10**
  - Granular Delay Buffer (juce::dsp::DelayLine)
  - Grain Voice Engine (custom polyphonic, 64 voices)
  - Grain Scheduler (custom sample-based timer)
  - Window Function Generator (juce::dsp::WindowingFunction - Hann)
  - Pitch Shifter (playback rate adjustment)
  - Scale Quantization System (lookup tables)
  - Random Pan Generator (juce::Random)
  - Reverse Playback System (direction flag)
  - Feedback Loop (manual mixing)
  - Dry/Wet Mixer (juce::dsp::DryWetMixer)
- **Features:** 3 points
  - Feedback loops (+1)
  - Real-time visualization (+1) - Particle field grain position streaming
  - Polyphonic voice management (+1) - 64 grain voices
- **Total:** 1.8 + 10 + 3 = **14.8** → Capped at **5.0**

---

## Stages

- Stage 0: Research ✓
- Stage 2: Foundation + Shell ← Next
- Stage 3: DSP (phased implementation - 3 phases)
- Stage 4: GUI (phased implementation - 2 phases)
- Stage 5: Validation

---

## Complex Implementation (Score 5.0)

### Stage 3: DSP Phases

#### Phase 3.1: Core Granular Engine

**Goal:** Implement polyphonic grain voice management with basic playback

**Components:**
- Granular delay buffer setup (`juce::dsp::DelayLine` with Lagrange3rd interpolation)
- Grain voice structure array (64 pre-allocated voices)
- Grain scheduler (sample counter, spawn interval calculation)
- Window function application (Hann window via `juce::dsp::WindowingFunction`)
- Voice allocation logic (find inactive voice, spawn new grain)
- Basic grain playback loop (read from buffer, apply window, sum voices)

**Signal flow:**
```
Input → Delay Buffer Write
         ↓
Grain Scheduler (trigger new grains based on density)
         ↓
For each active grain:
  - Read from buffer (no pitch shift yet, just base playback)
  - Apply Hann window envelope
  - Sum grain outputs
         ↓
Output (mono for testing)
```

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through delay buffer
- [ ] Single grain spawns and plays with windowed envelope (no clicks)
- [ ] Multiple grains spawn based on density parameter (0% = sparse, 100% = dense)
- [ ] Grain size parameter controls grain duration (5ms to 500ms)
- [ ] No artifacts or discontinuities at grain boundaries
- [ ] CPU usage profiled (should be <50% at 64 voices, 48kHz)

**Duration:** 6-8 hours

---

#### Phase 3.2: Pitch Shifting + Scale Quantization

**Goal:** Add randomized, quantized pitch shifting to grains via playback rate

**Components:**
- Playback rate calculation from semitones (`rate = 2^(semitones/12)`)
- Scale lookup tables (Chromatic, Major, Minor, Pentatonic, Blues)
- Quantization algorithm (nearest-note to scale)
- Root note transposition
- Integrate pitch-shifted playback into grain voice loop

**Pitch flow:**
```
Grain spawn triggered
         ↓
Generate random pitch: -7 to +7 semitones (scaled by pitch_random parameter)
         ↓
Quantize to selected scale (lookup table)
         ↓
Transpose by root_note (0-11 semitones)
         ↓
Calculate playback rate: rate = 2^(quantizedPitch / 12)
         ↓
Grain reads from buffer at playbackRate (interpolated by DelayLine)
```

**Test Criteria:**
- [ ] Grains play at randomized pitches within ±7 semitone range
- [ ] pitch_random parameter scales randomization amount (0% = no pitch shift, 100% = full ±7 range)
- [ ] Scale quantization works correctly:
  - [ ] Chromatic: All pitches (no quantization)
  - [ ] Major: Pitches snap to major scale intervals
  - [ ] Minor: Pitches snap to minor scale intervals
  - [ ] Pentatonic: Pitches snap to pentatonic scale
  - [ ] Blues: Pitches snap to blues scale
- [ ] root_note parameter transposes scale (C through B)
- [ ] Pitch-shifted grains sound smooth (no interpolation artifacts)
- [ ] Extreme pitches (±7 semitones) don't cause buffer overruns or crashes

**Duration:** 3-4 hours

---

#### Phase 3.3: Spatial, Reverse, Feedback

**Goal:** Add pan randomization, reverse playback, feedback loop, and dry/wet mixing

**Components:**
- Random pan positioning per grain (`juce::Random::nextFloat()` for 0.0-1.0)
- Reverse playback direction (50/50 probability per grain)
- Feedback loop (grain output → feedback gain → mixed with delay buffer input)
- Dry/wet mixer (`juce::dsp::DryWetMixer<float>`)

**Complete signal flow:**
```
Input (Stereo)
  ↓
Dry/Wet Mixer (capture dry) ← mix parameter
  ↓
Mix with feedback signal ← feedback parameter
  ↓
Write to Delay Buffer
  ↓
Grain Scheduler (spawn with randomized parameters)
  ↓
For each active grain (64 voices per channel):
  - Randomize pan position (0.0-1.0, scaled by pan_random)
  - Randomize reverse/forward (50/50)
  - Read from buffer (pitch-shifted, forward or reverse)
  - Apply Hann window
  - Apply stereo pan (leftGain, rightGain)
  - Sum to stereo output
  ↓
Apply feedback gain
  ↓
Route feedback to delay buffer input
  ↓
Dry/Wet Mixer (blend) ← mix parameter
  ↓
Output (Stereo)
```

**Test Criteria:**
- [ ] Pan randomization works (0% = all grains centered, 100% = full stereo spread)
- [ ] Reverse playback randomly selected per grain (audible reversed grains)
- [ ] Feedback loop creates evolving textures (low feedback = sparse, high feedback = dense self-reinforcing cloud)
- [ ] Dry/wet mix blends processed and unprocessed signal (0% = dry only, 100% = wet only)
- [ ] Stereo imaging is correct (panned grains appear in correct stereo field)
- [ ] No runaway feedback at 100% feedback (soft clipping or limiting prevents instability)
- [ ] All 9 parameters functional and interactive

**Duration:** 3-4 hours

---

### Stage 4: GUI Phases

#### Phase 4.1: Layout and Basic Controls

**Goal:** Integrate HTML mockup and bind all parameters via WebView

**Components:**
- Copy `v1-ui.html` (or latest mockup) to `Source/ui/public/index.html`
- Update `PluginEditor.h/cpp` with WebView setup
- Configure `CMakeLists.txt` for WebView resources (BinaryData, `NEEDS_WEB_BROWSER TRUE`)
- Create relay objects for 7 knobs + 2 combo boxes
- Bind parameters via `WebSliderParameterAttachment` and `WebComboBoxParameterAttachment`
- Implement resource provider for HTML/CSS/JS files

**Bindings:**
- **Knobs (7):** delay_time, grain_size, density, pitch_random, pan_random, feedback, mix
- **Combo boxes (2):** scale, root_note

**Test Criteria:**
- [ ] WebView window opens with correct size (550×600px)
- [ ] All 7 knobs visible and styled correctly (90px diameter, 3D shadows/gradients)
- [ ] 2 combo boxes visible in header (scale, root_note)
- [ ] Background and styling match mockup (cream/spacey/textured skeuomorphic)
- [ ] Knob rotations change DSP parameters (two-way binding works)
- [ ] Host automation updates UI knobs in real-time
- [ ] Preset changes update all UI elements correctly
- [ ] No lag or visual glitches during parameter changes

**Duration:** 3 hours

---

#### Phase 4.2: Particle Field Visualization

**Goal:** Implement real-time grain position visualization (200×200px particle field)

**Components:**
- C++ grain data collection (copy active grain positions in timer callback)
- Timer-based data streaming to JavaScript (30Hz throttled updates)
- JavaScript particle rendering (Canvas or SVG)
- Particle animation with interpolation (smooth 60fps)
- Particle styling (glow effects, color based on pan position)

**Visualization data per grain:**
- **X-axis:** Time position in delay buffer (0-1 normalized)
- **Y-axis:** Pitch shift amount (-1 to +1, representing -7 to +7 semitones)
- **Color/Size:** Pan position (left grains vs right grains, warm gold glow)

**Implementation approach:**
```cpp
// C++ (PluginEditor timer callback, 30Hz)
std::vector<GrainData> collectActiveGrains() {
    std::vector<GrainData> data;
    for (const auto& grain : processor.getActiveGrains()) {
        if (grain.active) {
            data.push_back({
                grain.readPos / bufferSize,    // x: 0-1
                grain.pitchSemitones / 7.0f,   // y: -1 to +1
                grain.pan                       // pan: 0-1
            });
        }
    }
    return data;
}

// Send to JavaScript via native function call
```

```javascript
// JavaScript (index.html or separate js file)
function updateParticleField(grainData) {
    // Clear canvas
    ctx.clearRect(0, 0, 200, 200);

    // Draw each grain as particle
    grainData.forEach(grain => {
        const x = grain.x * 200;  // Map 0-1 to 0-200px
        const y = (1 - (grain.y + 1) / 2) * 200;  // Map -1..+1 to 200..0px (inverted Y)

        // Glow effect based on pan (left = gold, right = brighter gold)
        const glowIntensity = grain.pan;
        const gradient = ctx.createRadialGradient(x, y, 0, x, y, 8);
        gradient.addColorStop(0, `rgba(255, 179, 71, ${glowIntensity})`);  // Bright center
        gradient.addColorStop(1, 'rgba(255, 216, 155, 0)');  // Fade to transparent

        ctx.fillStyle = gradient;
        ctx.beginPath();
        ctx.arc(x, y, 8, 0, Math.PI * 2);
        ctx.fill();
    });
}

// Smooth animation loop (60fps interpolation)
function animate() {
    updateParticleField(currentGrainData);
    requestAnimationFrame(animate);
}
```

**Test Criteria:**
- [ ] Particle field displays in UI (200×200px area, centered)
- [ ] Particles appear when grains spawn (real-time correlation)
- [ ] Particle X-position represents time in delay buffer
- [ ] Particle Y-position represents pitch shift amount
- [ ] Particle color/intensity represents pan position
- [ ] Smooth 60fps animation (no jitter or lag)
- [ ] Performance acceptable (no audio dropouts from visualization overhead)
- [ ] Particle glow effects render correctly (warm gold gradient)

**Duration:** 4-5 hours

---

### Estimated Duration

**Total: ~20-25 hours**

- Stage 0: Research & Planning ✓ (30 min)
- Stage 2: Foundation + Shell: 10 min
  - Foundation: 5 min (project structure)
  - Shell: 5 min (APVTS parameters)
- Stage 3: DSP Implementation: 12-16 hours (phased)
  - Phase 3.1: Core Granular Engine: 6-8 hours
  - Phase 3.2: Pitch + Quantization: 3-4 hours
  - Phase 3.3: Spatial/Reverse/Feedback: 3-4 hours
- Stage 4: GUI Implementation: 7-8 hours (phased)
  - Phase 4.1: Layout + Basic Controls: 3 hours
  - Phase 4.2: Particle Field Visualization: 4-5 hours
- Stage 5: Validation: 20 min (presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()` (APVTS)
- Grain voice array pre-allocated in `prepareToPlay()` (no allocations in `processBlock()`)
- Grain data for visualization copied to temporary buffer (audio thread writes, message thread reads)
- No locks in audio thread (visualization updates are non-blocking)

### Performance
- **Estimated CPU usage:**
  - Granular engine (64 voices): ~40-50% single core
  - Pitch shifting (playback rate): ~5-10%
  - Window application: ~5%
  - **Total: ~50-65% single core** at 48kHz, 512 sample buffer
- **Optimization strategies:**
  - Pre-calculate window lookup tables (avoid per-sample `cos()` calls)
  - Use `juce::FloatVectorOperations::add()` for grain summing (SIMD)
  - Early-exit inactive voices (skip processing if `grain.active == false`)
  - Profile in Phase 3.1, reduce voice count if CPU exceeds 60%
- **Voice count fallbacks:**
  - Primary: 64 voices
  - Fallback 1: 32 voices (if CPU > 60%)
  - Fallback 2: 16 voices (if CPU > 40% after optimization)

### Latency
- **Total processing latency:**
  - Delay buffer: 100-2000ms (user-controlled via delay_time parameter)
  - Grain size: 5-500ms (additional latency from grain windowing)
  - No FFT latency (time-domain pitch shifting)
- **Host compensation:**
  - Report `delay_time` as latency via `getLatencySamples()` if delay is fixed
  - If delay_time is modulated during playback, latency reporting may be approximate (acceptable for creative effect)

### Denormal Protection
- Use `juce::ScopedNoDenormals` in `processBlock()`
- Grain window fade-out may produce denormals near completion (JUCE handles internally)
- DelayLine and DryWetMixer have built-in denormal protection

### Known Challenges

**Challenge 1: Voice allocation efficiency**
- Linear search for inactive voice may become bottleneck at high grain spawn rates
- **Solution:** Profile in Phase 3.1, consider voice stealing (reuse oldest grain near completion) or voice pool with free list

**Challenge 2: Buffer boundary handling for reverse playback**
- Reverse grains may read beyond buffer start (negative indices)
- **Solution:** Wrap read position using modulo: `readPos = (readPos + bufferSize) % bufferSize`

**Challenge 3: Visualization performance overhead**
- Streaming 64 grain positions at 30Hz may impact audio thread
- **Solution:** Copy grain data to separate buffer in audio thread (lock-free), read in timer callback on message thread

**Challenge 4: Extreme pitch shifts causing buffer overruns**
- Playback rate of 2.0 (±7 semitones) may cause grain to read past buffer end during single grain playback
- **Solution:** Clamp read position to buffer bounds, wrap using modulo

**Challenge 5: Feedback loop stability at high feedback values**
- Feedback gain of 1.0 (100%) causes infinite feedback (runaway)
- **Solution:** Cap feedback gain at 0.95, apply soft clipping or tanh to prevent runaway

---

## References

**Contract files:**
- Creative brief: `plugins/Scatter/.ideas/creative-brief.md`
- Parameter spec: `plugins/Scatter/.ideas/parameter-spec.md`
- DSP architecture: `plugins/Scatter/.ideas/architecture.md`
- UI mockup: `plugins/Scatter/.ideas/mockups/v1-ui.yaml` (when created)

**Similar plugins for reference:**
- **FlutterVerb** - Pitch modulation delay line (Lagrange3rd interpolation pattern)
- **LushVerb** - Dry/wet mixing, reverb processing chain
- **TapeAge** - LFO-based modulation, window function usage
- **GainKnob** - WebView parameter binding reference

**Professional plugins researched:**
- **Output Portal** - Granular delay architecture, scale quantization, XY visualization
- **Red Panda Particle** - Delay-line-based granular, reverse playback probability
- **GrainScanner** - Polyphonic grain voice management, window shapes
- **Audio Damage Quanta 2** - Playback rate pitch shifting for granular synthesis
