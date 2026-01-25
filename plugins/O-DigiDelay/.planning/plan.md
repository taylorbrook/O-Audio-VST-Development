# Ouaricon Digital Delay - Implementation Plan

**Date:** 2026-01-12
**Complexity Score:** 2.4 (Moderate)
**Strategy:** Single-pass implementation

---

## Complexity Factors

- **Parameters:** 8 parameters (8/5 = 1.6 points, capped at 2.0) = 1.6
  - TIME, SYNC, DIVISION, FEEDBACK, SPREAD, MOD, WET, DRY
  - Plus: Output meter (visual display, not a parameter)
- **Algorithms:** 4 DSP components = 1.0
  - Delay Line Engine (juce::dsp::DelayLine with Lagrange3rd)
  - Stereo Spread Processor (Haas effect)
  - Delay Time Modulation (juce::dsp::Oscillator)
  - Feedback Loop (custom signal routing)
  - Dry/Wet Mixer (juce::dsp::DryWetMixer)
- **Features:** 0 points
  - No feedback loops (≠ complexity feature: feedback is standard delay feature)
  - No FFT/frequency domain processing
  - No multiband processing
  - No external modulation systems (LFO is internal, fixed-rate)
  - No external MIDI control
- **Total:** 1.4 + 1.0 + 0.0 = 2.4 (within simple range)

**Classification:** Moderate complexity (2.1-2.9) - Single-pass implementation appropriate

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 1: Foundation ← Next
- Stage 2: Shell
- Stage 3: DSP
- Stage 3: GUI
- Stage 3: Validation

---

## Simple Implementation (Score ≤ 2.9)

### Implementation Flow

- Stage 1: Foundation - project structure
- Stage 2: Shell - APVTS parameters
- Stage 3: DSP - single pass
- Stage 3: GUI - single pass
- Stage 3: Validation - presets, pluginval, changelog

### Implementation Notes

**DSP Approach:**

Implement all DSP components in a single phase:

1. **Core delay processing:**
   - Dual mono delay lines (L/R independent)
   - Lagrange3rd interpolation for fractional delays
   - Maximum buffer: 2000ms at 192kHz

2. **Tempo sync system:**
   - Read BPM from AudioPlayHead in processBlock()
   - Calculate delay time from BPM + subdivision
   - 12 subdivision lookup table (straight, dotted, triplets, quintuplets)
   - Fallback to TIME parameter if BPM unavailable

3. **Stereo spread (Haas effect):**
   - Apply 0-15ms offset to right channel delay time
   - Left channel: baseDelay
   - Right channel: baseDelay + spreadOffset

4. **Delay time modulation:**
   - Sine wave LFO at 0.3 Hz (fixed rate)
   - Modulation depth: 0-10ms (scaled by MOD parameter)
   - Applied to output tap position (not feedback loop)

5. **Feedback loop:**
   - Scale delay output by feedback gain (0.0-0.95)
   - Mix feedback signal with input
   - Per-channel feedback (no cross-channel)

6. **Wet/dry mixing:**
   - Separate WET and DRY gain controls (juce::dsp::Gain)
   - WET: Scales delayed signal (0.0-1.0)
   - DRY: Scales clean signal (0.0-1.0)
   - Parallel sum (not crossfade) - both can be 100%
   - Spillover support (wet processing continues when bypassed)

7. **Output level meter:**
   - Calculate RMS level after final sum
   - Send L/R levels to WebView via message passing
   - Update rate: ~30Hz (batched to avoid UI overhead)

**Test criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Free mode delay works (TIME parameter 1-2000ms)
- [ ] Tempo sync mode works (BPM reading, subdivision calculation)
- [ ] Feedback creates repeating echoes (0-100% range)
- [ ] Stereo spread widens image (0-100% range, no ping-pong)
- [ ] Modulation adds chorus effect (0-100% range)
- [ ] WET parameter controls delayed signal level (0-100% range)
- [ ] DRY parameter controls clean signal level (0-100% range)
- [ ] Output meter displays signal level (L/R bars animate)
- [ ] Spillover works on bypass (tail continues)
- [ ] No clicks, pops, or artifacts
- [ ] Smooth parameter changes (no zipper noise)

**GUI Approach:**

Implement WebView UI in a single phase (from v7 mockup):

1. **Layout and controls:**
   - Use finalized v7 mockup (700×196px ultra-compact rack mount)
   - 8 parameters: TIME, SYNC, DIVISION, FEEDBACK, SPREAD, MOD, WET, DRY
   - Layout: Horizontal row (SYNC/DIVISION → TIME → FEEDBACK → SPREAD → MOD → WET → DRY → OUTPUT METER)
   - Labels above knobs, value displays below (no boxes)

2. **Parameter binding:**
   - JavaScript → C++ relay calls (control changes)
   - C++ → JavaScript parameter updates (host automation)
   - Use `getSliderState()` for continuous parameters (TIME, FEEDBACK, SPREAD, MOD, WET, DRY)
   - Use `getToggleState()` for SYNC parameter
   - Use `getComboBoxState()` for DIVISION parameter (12 choices)

3. **Value display:**
   - TIME: Display "XXXms" in free mode, subdivision name in sync mode
   - SYNC: ON/OFF indicator
   - DIVISION: Show subdivision name (1/4, 1/8D, 1/4T, etc.)
   - FEEDBACK/SPREAD/MOD/WET/DRY: Display "%"

4. **Output meter (visual display):**
   - 14-segment vertical LED meter (OuariconComp style)
   - Receives RMS level data from C++ via WebView messaging
   - Update rate: ~30Hz
   - Colors: low=#8BA870, mid=#6B8E4E, high=#C9A27B

5. **Visual assets:**
   - Paper texture background (img/paper1.jpg)
   - Butterfly overlay at natural size (img/butterfly2_Black and white.png, 30% opacity)
   - 10-segment botanical seed knobs

**Test criteria:**
- [ ] All controls visible and styled
- [ ] Knob/slider movements update DSP parameters
- [ ] Host automation updates UI controls
- [ ] Preset changes update all UI elements
- [ ] SYNC toggle switches between free/sync modes
- [ ] DIVISION dropdown shows 12 subdivisions
- [ ] Value displays show correct units and formatting
- [ ] Output meter animates with signal level
- [ ] No UI lag or visual glitches

**Key Considerations:**

1. **AudioPlayHead thread safety:**
   - Only call `getPlayHead()->getPosition()` in processBlock()
   - Check for null pointer before dereferencing
   - Fallback to TIME parameter if BPM unavailable

2. **Smooth parameter transitions:**
   - Use parameter smoothing (20ms ramp) for TIME, FEEDBACK, SPREAD, MOD, MIX
   - Prevents clicks when switching SYNC mode or changing DIVISION
   - juce::dsp::DelayLine handles fractional delay smoothly (Lagrange3rd)

3. **Mono compatibility:**
   - Haas effect (stereo spread) can cause comb filtering in mono
   - Document in user manual: "SPREAD parameter may reduce mono signal level"
   - At 0%, mono compatibility is perfect (true stereo through)

4. **Spillover implementation:**
   - Report tail size via `getTailLengthSeconds()`
   - Formula: `tailSeconds = (maxDelayMs / 1000.0) * (1.0 / (1.0 - feedbackGain))`
   - Example: 2000ms delay + 0.9 feedback = 20 seconds tail
   - DryWetMixer continues processing when bypassed (automatic)

5. **Denormal protection:**
   - Use `juce::ScopedNoDenormals` in processBlock()
   - Feedback loop hard-limited at 0.95 (prevents denormal accumulation)

**Potential Gotchas:**

1. **SYNC mode BPM fallback:**
   - Some DAWs don't provide BPM in all contexts (offline rendering, non-timeline playback)
   - Always check `positionInfo.getBpm().hasValue()` before using
   - Graceful fallback to TIME parameter

2. **DIVISION parameter display:**
   - 12 choices require clear naming (1/4, 1/4D, 1/4T, 1/4(5), etc.)
   - UI dropdown must show full subdivision names (not just "0-11")
   - JavaScript must map choice index to display string

3. **Stereo spread + modulation interaction:**
   - Both affect right channel delay time (additive)
   - spreadOffset + modulation must not exceed buffer size
   - Clamp total delay to maximum buffer size (safety check)

4. **Feedback stability:**
   - Hard limit at 0.95 prevents self-oscillation
   - Do NOT allow feedback ≥ 1.0 (runaway behavior)
   - Parameter range mapping: 0-100% → 0.0-0.95 (explicit limit)

---

## References

- Creative brief: `plugins/Ouaricon Digital Delay/.ideas/creative-brief.md`
- Parameter spec: Not yet created (use estimated parameters from brief)
- DSP architecture: `plugins/Ouaricon Digital Delay/.planning/architecture.md`
- UI mockup: Not yet created

**Similar plugins for reference:**

- **GainKnob:** Parameter smoothing pattern, WebView relay system
- **TapeAge:** Modulation implementation (LFO + parameter binding)
- **FlutterVerb:** DryWetMixer usage, spillover implementation
- **FabFilter Timeless (professional):** Tempo sync subdivisions, stereo spread
- **Waves H-Delay (professional):** Classic digital delay reference

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- AudioPlayHead only called in processBlock() (thread-safe per JUCE docs)
- DelayLine uses internal circular buffers (no locks needed)
- No shared state between channels (dual mono)

### Performance
- Delay lines (Lagrange3rd): ~8% CPU (dual mono)
- LFO (sine oscillator): ~1% CPU
- Feedback mixing: ~1% CPU
- Dry/wet mixing: ~2% CPU
- **Total estimated: ~12% single core at 48kHz**

Target: <15% single core (well within budget)

### Latency
- Delay line: 1-2000ms (intentional, user-controlled)
- DryWetMixer: Automatic latency compensation
- Report via `getLatencySamples()` if needed (typically not for delay effects)
- Spillover tail: Calculated based on delay time + feedback gain

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- DelayLine handles denormals internally (JUCE implementation)
- Feedback loop hard-limited (prevents denormal accumulation)

### Known Challenges

1. **Tempo sync subdivision calculation:**
   - Use lookup table for 12 subdivisions (straightforward)
   - Smooth transitions when changing DIVISION (parameter smoothing)
   - Test with multiple DAWs (BPM tracking reliability)

2. **Haas effect mono compatibility:**
   - Test summed L+R for comb filtering (should be acceptable at 0-15ms)
   - Document behavior in user manual
   - Spread is optional (0% = perfect mono compatibility)

3. **Spillover tail calculation:**
   - Tail length depends on delay time AND feedback gain
   - Must handle edge case: feedback = 0 (no tail, instant stop)
   - Formula: `tailSeconds = maxDelay * (1 / (1 - feedback))` (only if feedback > 0)

4. **WebView DIVISION dropdown:**
   - 12 choices require custom ComboBox implementation in JavaScript
   - Use `getComboBoxState()` for parameter binding
   - Display subdivision names (not numeric indices)

---

## Duration Estimate

Based on single-pass implementation:

- **Stage 1: Foundation** - 30 minutes
  - CMakeLists.txt setup
  - PluginProcessor.h/cpp scaffolding
  - BusesProperties (stereo input + output)

- **Stage 2: Shell** - 45 minutes
  - APVTS parameter definitions (7 parameters)
  - Parameter ranges and default values
  - DIVISION parameter (12-choice enum)

- **Stage 3: DSP** - 2 hours
  - DelayLine setup with Lagrange3rd
  - Tempo sync calculation (AudioPlayHead + subdivisions)
  - Stereo spread (Haas effect)
  - LFO modulation (juce::dsp::Oscillator)
  - Feedback loop implementation
  - DryWetMixer integration
  - Testing and debugging

- **Stage 3: GUI** - 1.5 hours
  - WebView setup (HTML/CSS/JS)
  - Parameter relay system (7 relays)
  - JavaScript parameter bindings
  - Value displays and formatting
  - DIVISION dropdown implementation
  - Testing and polish

- **Stage 3: Validation** - 30 minutes
  - Preset creation
  - Pluginval testing
  - Changelog documentation

**Total estimated: ~5 hours** (single work session or split across 2 days)

---

## Success Criteria

**Plugin succeeds when:**

1. All 7 parameters work correctly:
   - TIME: 1-2000ms in free mode
   - SYNC: Toggles between free/sync modes
   - DIVISION: 12 subdivisions calculate correct delay times
   - FEEDBACK: 0-100% creates repeating echoes
   - SPREAD: 0-100% widens stereo field without ping-pong
   - MOD: 0-100% adds chorus effect
   - WET: 0-100% controls delayed signal level
   - DRY: 0-100% controls clean signal level

2. Tempo sync works reliably:
   - BPM reading from AudioPlayHead
   - Subdivisions calculate correct delay times
   - Fallback to TIME parameter if BPM unavailable
   - Smooth transitions when changing modes/subdivisions

3. Audio quality is transparent:
   - No clicks, pops, or zipper noise
   - Lagrange3rd interpolation prevents artifacts
   - Feedback loop is stable (no self-oscillation)
   - Modulation is smooth (no pitch shift artifacts)

4. Stereo spread is subtle:
   - 0% preserves input stereo field
   - 100% widens without harsh ping-pong
   - Mono compatibility acceptable (no severe comb filtering)

5. Spillover works correctly:
   - Delay tail continues when bypassed
   - Tail length matches delay time + feedback
   - DryWetMixer handles automatically

6. UI is responsive:
   - All controls update DSP in real-time
   - Host automation updates UI
   - Preset changes update all parameters
   - Value displays show correct formatting

7. Performance is acceptable:
   - CPU usage <15% single core at 48kHz
   - No audio dropouts or glitches
   - Works with standard buffer sizes (64-512 samples)

8. Validation passes:
   - Pluginval: All tests pass
   - Multiple DAWs: Ableton, Logic, FL Studio
   - Multiple formats: VST3, AU, Standalone

---

## Next Steps

1. **Run /implement Ouaricon Digital Delay** to start Stage 1 (Foundation)
2. Review architecture.md before implementation
3. Pause after each stage for validation
4. Test spillover behavior early (critical feature)
5. Test tempo sync with multiple DAWs (BPM reliability)
