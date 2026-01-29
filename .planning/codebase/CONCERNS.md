# Codebase Concerns

**Analysis Date:** 2026-01-29

## Tech Debt

**Oversampling Disabled in Multiple Plugins Due to JUCE/Logic Incompatibility**
- Issue: Harmonic generation and saturation plugins disable JUCE oversampling to prevent "Sample Rate XXXXX" crash in Logic Pro
- Files: `plugins/O-Bass/Source/DSP/HarmonicGenerator.cpp:11-13,79-80`, `plugins/O-Bass/Source/DSP/HarmonicGenerator.h:10-13`, `plugins/O-AnalogSaturation/Source/PluginProcessor.h:56-59`
- Impact: Without oversampling, aliasing increases at higher frequencies; saturation quality limited to 2x-4x maximum; cannot implement proper alias-free processing at native sample rates
- Fix approach:
  1. Investigate exact cause of Logic crash (likely memory corruption in JUCE oversampler internals)
  2. Test with newer JUCE versions (currently constrained by Logic compatibility)
  3. Implement custom oversampling wrapper or alternative anti-aliasing (Chebyshev polynomial drive constraints)
  4. Add Logic Pro version detection to conditionally enable oversampling on compatible versions

**Latency Reporting Disabled in O-Bass**
- Issue: Logic Pro crashes when plugin reports non-zero latency ("Sample Rate XXXXX" error). Latency reporting is disabled; all latency-inducing features removed
- Files: `plugins/O-Bass/Source/PluginProcessor.cpp:159-160` (updateLatencyReport commented), `plugins/O-Bass/Source/DSP/HarmonicGenerator.h:32`
- Impact: No phase alignment correction for linear-phase filtering; automated DAW mixing may have phase issues with other plugins; FIR crossover latency not reported
- Fix approach:
  1. Create minimal test case: report latency=0, then latency=1; determine exact crash condition
  2. Try reporting latency through different mechanism (setLatencySamples vs getLatencyInSamples)
  3. Validate against multiple Logic Pro versions
  4. If unfixable, document workaround: bypass FIR mode in favor of low-latency IIR

**Pitch Tracking Disabled Due to Real-Time Performance**
- Issue: YIN pitch tracking algorithm disabled (O(n²) complexity); commented out in O-Bass CleanModeProcessor
- Files: `plugins/O-Bass/Source/DSP/PitchTracker.h:17,31`, `plugins/O-Bass/Source/DSP/CleanModeProcessor.cpp:10-14`
- Impact: Cannot adapt harmonic generation to input frequency; enhancement strength same regardless of bass frequency (suboptimal for 40Hz vs 200Hz bass)
- Fix approach:
  1. Profile actual CPU cost of YIN on 512-sample blocks at 48kHz (may be acceptable now)
  2. Implement low-frequency-optimized pitch detection (FFT-based, faster convergence on fundamentals)
  3. Run pitch tracking on separate thread with 1-2 block latency tolerance
  4. Cache pitch detection results across blocks to amortize cost

**MTS-ESP Integration Stubbed in O-Marimba and O-Lyrica**
- Issue: MTS-ESP mode falls back to frequency table; actual MIDI Tuning Standard protocol not implemented
- Files: `plugins/O-Marimba/Source/TuningEngine.cpp:30-35`, `plugins/O-Lyrica/Source/DSP/TuningEngine.h:Mode::MTSESP`
- Impact: Cannot use live dynamic retuning from DAW (e.g., Max4Live MTS-ESP provider); requires manual preset loading instead
- Fix approach:
  1. Add MTS-ESP library dependency (e.g., libtempo)
  2. Implement MIDI SysEx parsing for scale/tuning updates
  3. Add thread-safe frequency table update mechanism for real-time tuning changes
  4. Test with common MTS-ESP sources (Max4Live, Scala, ddwChant)

**Raw Pointer Allocations in Synth Voices**
- Issue: `new HarpSynthVoice()` and `new HarpSynthSound()` instead of `std::make_unique<>`
- Files: `plugins/O-Lyrica/Source/PluginProcessor.cpp:239,247`
- Impact: Inconsistent memory management pattern; exception safety weak if constructor fails
- Fix approach: Replace all raw `new` with `std::make_unique<>` for consistency and exception safety

**JavaScript Error Handling in WebView Bindings**
- Issue: No error callbacks on `evaluateJavascript()` calls; single JS error cascades and stops all subsequent bindings
- Files: `plugins/O-Bass/Source/PluginEditor.cpp`, `plugins/O-Lyrica/Source/PluginEditor.cpp:618,625,653`, `plugins/O-Polystutter/.ideas/mockups/v5-PluginEditor.cpp:501-502`
- Impact: One broken binding (e.g., ComboBox) prevents preset system, parameter bindings, and UI updates from activating (historical: Lyrica v1.5.3 had non-functional presets)
- Fix approach:
  1. Wrap all evaluateJavascript calls with error callbacks
  2. Add console error logging to UI JavaScript
  3. Implement binding health check (validate all bindings initialized before declaring UI ready)
  4. Add fallback/recovery mode if specific binding fails

**Spinlock Pattern in MIDI Event Queue**
- Issue: Custom spinlock (compare_exchange_strong loop) instead of standard lock primitives; intentionally drops events under contention
- Files: `plugins/O-Lyrica/Source/PluginProcessor.h:36-52`
- Impact: High CPU if contention occurs; design trades off reliability for lock-free performance; acceptable for visualization but fragile if requirements change
- Fix approach:
  1. Document design trade-off clearly in header comments
  2. Add metrics for dropped events (counter, logging under high MIDI density)
  3. If reliability needed, switch to lock-free queue library (boost::lockfree or moodycamel::ConcurrentQueue)
  4. Profile actual contention under 100+ note/sec MIDI density

---

## Known Bugs

**Latency Reporting Crash in O-Bass (Unresolved)**
- Symptoms: Enabling latency reporting via `setLatencySamples()` causes Logic Pro crash with "Sample Rate XXXXX" error
- Root cause: Unknown; likely memory corruption in JUCE Oversampling internals or Logic host compatibility issue
- Files: `plugins/O-Bass/Source/PluginProcessor.cpp:159-160`, `plugins/O-Bass/Source/DSP/HarmonicGenerator.h:32`
- Trigger: Call `updateLatencyReport()` from prepareToPlay when oversampling enabled
- Workaround: Disable latency reporting and latency-inducing features; use low-latency IIR filters only
- Status: Priority 4 - investigation blocked pending JUCE version upgrade or Logic compatibility investigation

**Oversampling Crash in Logic Pro (Unresolved)**
- Symptoms: JUCE Oversampling instance initialization causes "Sample Rate XXXXX" crash in Logic Pro; VST3 and Standalone work
- Root cause: Unknown; suspected memory corruption or incompatibility with Logic's audio thread context
- Files: `plugins/O-Bass/Source/DSP/HarmonicGenerator.cpp:11-13,79-80`
- Trigger: Instantiate juce::dsp::Oversampling in prepareToPlay for O-Bass or O-AnalogSaturation
- Workaround: Process at native sample rate without oversampling; use gentler waveshaping to minimize aliasing
- Status: Priority 4 - blocks high-fidelity mode implementation

**GUI Keyboard Does Not Reflect Custom Tuning (O-Lyrica)**
- Symptoms: On-screen keyboard layout remains standard 12-TET visually even when custom tuning active; audio plays correct frequencies
- Root cause: Tuning data not exposed to WebView UI; keyboard rendering hardcoded for 12-TET
- Files: `plugins/O-Lyrica/Resources/ui/index.html` (keyboard rendering), tuning data isolated in C++ layer
- Trigger: Load temperament preset, load Scala file, or apply custom tuning via parameters
- Workaround: Audio output is correct; visual feedback missing only. Use MIDI note numbers as reference.
- Status: Medium priority - affects user experience but audio is correct

---

## Security Considerations

**Scala File Parsing Without Input Validation**
- Risk: Scala file parser reads line-by-line without bounds checking on degree counts, interval values, or file size
- Files: `plugins/O-Lyrica/Source/DSP/TuningEngine.cpp:470-530` (calculateScala), `plugins/O-Marimba/Source/TuningEngine.cpp`
- Current mitigation: Max 128 MIDI notes limits practical scale size; juce::String parsing prevents buffer overflows
- Recommendations:
  1. Add explicit validation: max degree count (256), interval range (0-12000 cents), max file size (100KB)
  2. Validate scale degrees match scale name size
  3. Add error reporting when malformed Scala files loaded; don't fail silently
  4. Add fuzzing tests with malformed Scala files

**Web Resource Provider in WebBrowserComponent (Potential Path Traversal)**
- Risk: `getResource()` serves binary data and JavaScript from embedded resources without origin/path validation
- Files: `plugins/O-Bass/Source/PluginEditor.cpp`, `plugins/O-Lyrica/Source/PluginEditor.cpp:656-700`
- Current mitigation: All resources embedded (no network calls); resource URLs restricted to plugin domain
- Recommendations:
  1. Add explicit path validation before serving resources (reject `..`, absolute paths)
  2. Maintain whitelist of known safe resource paths
  3. Keep JUCE framework updated (WebKit security patches)
  4. Consider sandboxing WebView with stricter content security policy

**Atomic Variables Without Memory Order Specification**
- Risk: Some atomics use default memory order; could cause unexpected synchronization in edge cases
- Files: `plugins/O-Lyrica/Source/DSP/TuningEngine.h:324-325,338`, `plugins/O-Bass/Source/PluginProcessor.h` (limitIndicator, outputLevelDB without explicit memory_order)
- Current mitigation: Critical paths specify explicit memory_order_acquire/release; non-critical visualization uses relaxed
- Recommendations:
  1. Explicitly specify memory order on all atomic operations (acquire/release for critical paths, relaxed for UI-only metrics)
  2. Add code comments explaining why relaxed order is safe (e.g., "visualization-only, stale reads acceptable")
  3. Audit all atomics in audio thread context (potential tearing)

---

## Performance Bottlenecks

**IIR Filter Coefficient Recalculation Per-Sample During Smoothing (O-Bass)**
- Problem: `setCutoffFrequency()` recalculates filter coefficients; called per-sample when frequency smoothing active
- Files: `plugins/O-Bass/Source/DSP/CrossoverFilter.cpp:119`
- Cause: No coefficient interpolation; each frequency change recalculates IIR coefficients
- Impact: 10-20% CPU overhead during frequency sweeps (documented in CODE_REVIEW.md v1.1.1 as resolved, but worth monitoring)
- Improvement: Already optimized in v1.1.1 to update every 16 samples instead of per-sample

**High-Band Energy Calculation Full-Buffer Scan (O-Bass)**
- Problem: `calculateHighBandEnergy()` iterates entire audio buffer every block with no optimization
- Files: `plugins/O-Bass/Source/PluginProcessor.cpp:288`
- Cause: Simple implementation; no SIMD, no sampling optimization
- Impact: Scales linearly with block size; at 2048 samples, 4096 multiplications per block
- Improvement path:
  1. Use SIMD (e.g., juce::dsp::FloatVectorOperations) for vectorized calculation
  2. Sample every Nth sample for approximation if energy trend tracking is only use case
  3. Profile actual CPU impact (likely <1% with modern CPUs)

**RepeatLane Capture Buffer Circular Write With Modulo Operation (O-Polystutter)**
- Problem: Every sample writes to circular buffer (5 seconds × 192kHz = 960K samples); modulo operation per sample
- Files: `plugins/O-Polystutter/Source/DSP/RepeatLane.cpp:72-87`
- Cause: Linear allocation with modulo wraparound instead of power-of-2 optimization
- Impact: ~0.5-1% CPU overhead per capture buffer instance; negligible with modern CPUs but suboptimal
- Improvement: Use power-of-2 buffer size to replace modulo with bitwise AND (10x faster)

**Sympathetic Resonance O(MAX_VOICES²) Coupling Matrix Rebuild (O-Lyrica)**
- Problem: Coupling matrix rebuilt every voice registration change; O(16²) = 256 comparisons for each of 16 possible voices
- Files: `plugins/O-Lyrica/Source/DSP/SympatheticResonance.cpp:235-280` (buildCouplingMatrix)
- Cause: Double-buffered design requires full rebuild when topology changes; happens at block boundaries, not per-sample
- Impact: Negligible with MAX_VOICES=16; becomes problematic if scaled to 32+ voices
- Improvement path:
  1. Profile actual CPU impact at MAX_VOICES=16 (likely <0.1%)
  2. If needed, implement incremental updates instead of full rebuild
  3. Cache coupling strengths between static frequencies

**UI Polling Every 30ms in O-Lyrica**
- Problem: Timer fires 30+ times per second to pull MIDI events and update WebView visualization
- Files: `plugins/O-Lyrica/Source/PluginEditor.cpp:606-654`
- Cause: Design choice to update WebView at fixed interval rather than event-driven updates
- Impact: JavaScript execution + string concatenation overhead; noticeable at high MIDI density
- Improvement path:
  1. Reduce polling frequency if MIDI density is low (dynamic rate adjustment)
  2. Batch multiple MIDI events into single JavaScript call
  3. Use requestAnimationFrame for smooth animations instead of polling

---

## Fragile Areas

**TuningEngine Interval Mutex Pattern (O-Lyrica)**
- Files: `plugins/O-Lyrica/Source/DSP/TuningEngine.h:331`, multiple lock_guard sites
- Why fragile:
  1. 8 separate lock_guard sites - any missed lock when reading intervals causes race condition
  2. scaleIntervals vector modified (clear/push_back) under lock but read without lock during frequency table calcs
  3. If interval loading fails mid-file, vector left in inconsistent state
- Safe modification:
  1. Always wrap interval reads in lock_guard (add helper method: `getIntervals()` returns copy under lock)
  2. Use vector swap pattern for atomic interval replacement
  3. Validate complete interval set before committing
  4. Add bounds checking on all interval access

**MIDI Event Queue Circular Buffer (O-Lyrica)**
- Files: `plugins/O-Lyrica/Source/PluginProcessor.h:68-72`
- Why fragile:
  1. Drop-on-contention design acceptable for visualization; fragile if used for reliable data
  2. Fixed size (32 events) - no overflow indication; events silently dropped under high MIDI density
  3. Timer callback assumes readPos only advances from timer thread
- Safe modification:
  1. Document this is visualization-only and event loss acceptable
  2. Add metrics for dropped events (counter, logging)
  3. If reliability needed, switch to lock-free queue library (moodycamel::ConcurrentQueue)
  4. Add DBG logging when events dropped

**WebBrowserComponent JavaScript Injection (O-Bass, O-Lyrica)**
- Files: `plugins/O-Bass/Source/PluginEditor.cpp`, `plugins/O-Lyrica/Source/PluginEditor.cpp:615-653`
- Why fragile:
  1. Inline string concatenation of numbers into JavaScript (potential injection if values contain quotes)
  2. No error handling - if evaluateJavascript fails, subsequent calls may break WebView state
  3. Timer callback runs on UI thread; WebView updates must complete before next callback (no async handling)
- Safe modification:
  1. Use parameterized JavaScript instead of concatenation (pass JSON objects instead of inline strings)
  2. Implement error callback for evaluateJavascript to log failures
  3. Queue JavaScript updates; only evaluate if WebView ready
  4. Consider native message passing instead of JavaScript evaluation for frequent data updates

**Parameter Validation in Preset Loading (O-Lyrica, O-Marimba, O-Polystutter)**
- Files: `plugins/O-Lyrica/Source/OuariconPresetManager.cpp`, `plugins/O-Marimba/Source/PresetManager.cpp`, `plugins/O-Polystutter/Source/OPolystutterPresetManager.h`
- Why fragile:
  1. No validation that loaded preset parameter values are in valid ranges
  2. Malformed JSON causes silent failure or unhandled exceptions
  3. Preset files stored as-is without integrity checks
- Safe modification:
  1. Validate all loaded parameter values against AudioProcessor parameter ranges (clamp if out of bounds)
  2. Add try-catch around JSON parsing with user-friendly error messages
  3. Store version/checksum with preset to detect file corruption
  4. Implement migration logic if parameter definitions change

**PitchTracker DSP Implementation (O-Bass)**
- Files: `plugins/O-Bass/Source/DSP/PitchTracker.h`, `plugins/O-Bass/Source/DSP/PitchTracker.cpp`
- Why fragile:
  1. YIN algorithm disabled but code retained for future; no active maintenance
  2. Complex autocorrelation logic; small bugs in threshold calculations cause missed detections
  3. Decoupled from signal flow; if re-enabled, requires careful buffer synchronization with crossover filter
- Safe modification:
  1. If re-enabling, wrap in comprehensive unit tests (autocorrelation accuracy, threshold sensitivity)
  2. Profile on test signals (sweeps, sustained notes, polyphonic bass)
  3. Add debug output (detected pitch vs expected) during development
  4. Document constraints (min frequency, max polyphony detection limitations)

---

## Scaling Limits

**Sympathetic Resonance: MAX_VOICES = 16 (O-Lyrica)**
- Current capacity: 16 simultaneous voices with up to 15 couplings each
- Limit: Cannot exceed 16 voices; additional voices cause coupling data loss
- Scaling path:
  1. Profile memory/CPU at MAX_VOICES=16 (fixed arrays, ~1KB per voice for coupling matrix)
  2. If headroom exists, increase MAX_VOICES to 32 (O(n²) coupling matrix becomes 1024 entries vs 256)
  3. For larger systems, implement dynamic allocation with lock-free data structures for real-time safety

**Scala File Support: Max 128 MIDI Notes (O-Lyrica, O-Marimba)**
- Current capacity: TuningEngine supports up to 128 frequencies (one per MIDI note)
- Limit: Scala scales with > 128 degrees truncated; KBM keyboard mapping not fully utilized
- Scaling path:
  1. Support arbitrary Scala scale sizes (store up to 1000 degrees)
  2. Implement KBM mapping to intelligently distribute scales across 128 MIDI notes
  3. Add keyboard mapping editor UI to visualize degree-to-MIDI mapping

**Capture Buffer: 5 seconds at 192kHz (O-Polystutter)**
- Current capacity: RepeatLane allocates 960,000 samples per channel (5 sec @ 192k)
- Limit: Longer than 5 seconds not captured; complex samples exceed this at high sample rates
- Scaling path:
  1. Make capture duration configurable parameter (0.5 - 10 seconds)
  2. Allocate on first play instead of init to reduce memory footprint
  3. Consider memory-mapped file buffer for ultra-long captures

**Parameter Count Explosion (O-Polystutter)**
- Current capacity: 3 lanes × 14 parameters/lane = 42+ total parameters; can reach 60+ with added per-lane modulation
- Limit: More lanes or per-lane parameters quickly exhausts reasonable UI space
- Scaling path:
  1. Implement parameter tabs/pages in UI (lane 1, lane 2, lane 3 as separate pages)
  2. Add preset system for lane configurations (save/load lane chains)
  3. Limit modulation destinations to avoid parameter explosion

---

## Dependencies at Risk

**JUCE Framework WebBrowserComponent (macOS WebKit)**
- Risk: WebKit security updates required periodically; custom resource provider must stay compatible with JUCE API changes
- Impact: WebView initialization could break on JUCE version upgrade if WebKit API changes
- Files: `plugins/O-Bass/Source/PluginEditor.cpp`, `plugins/O-Lyrica/Source/PluginEditor.cpp`
- Migration plan:
  1. Use JUCE getLiveConstantValue to detect WebKit features and adapt at runtime
  2. Keep test suite for WebView rendering (validate tuning circle, presets, metering)
  3. Monitor JUCE release notes for WebBrowserComponent changes
  4. Plan fallback to non-WebView UI if WebBrowserComponent becomes unmaintainable

**Scala File Parsing (Custom Implementation)**
- Risk: Format parsing custom (not using external libscala); future Scala format changes require manual updates
- Impact: New Scala extensions (custom headers, new directives) silently ignored or cause parse failures
- Files: `plugins/O-Lyrica/Source/DSP/TuningEngine.cpp:470-530`, `plugins/O-Marimba/Source/TuningEngine.cpp`
- Migration plan:
  1. Monitor Scala format specification updates (ircam-online.github.io/scale-archive)
  2. Add unit tests for Scala parsing (compare output against libscala or Scala reference implementations)
  3. Version the parser to track supported Scala feature set
  4. Consider vendoring lightweight Scala parser library if high-quality option becomes available

**JUCE Oversampling (Version Compatibility)**
- Risk: Oversampling disabled due to Logic Pro crash; JUCE version upgrade may fix or break compatibility
- Impact: High-fidelity saturation quality limited without oversampling; aliasing increases at high frequencies
- Files: `plugins/O-Bass/Source/DSP/HarmonicGenerator.cpp`, `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp`
- Migration plan:
  1. Pin JUCE version until Logic Pro crash investigated
  2. Test with each new JUCE major version for Logic Pro compatibility
  3. Implement custom oversampling wrapper as fallback
  4. Add compile-time flags to conditionally enable oversampling based on JUCE version

---

## Missing Critical Features

**Scala KBM Full Support (O-Lyrica, O-Marimba)**
- Problem: KBM files parsed but keyboard mapping not fully applied to custom scales
- Blocks: Cannot map arbitrary scales properly to 128 MIDI keys; scales > 128 degrees truncated instead of mapped
- Implementation path:
  1. Parse KBM mapping file format (degree index to MIDI note mapping)
  2. Apply KBM mapping to distribute scale degrees across keyboard
  3. Update UI keyboard rendering to reflect KBM mapping visually
  4. Add KBM editor UI to customize keyboard layout

**Pitch Bend Per-Voice in O-Polystutter**
- Problem: Polystutter does not support per-voice pitch modulation during playback
- Blocks: Cannot create smooth glissandos or frequency modulation effects in repeat lanes
- Implementation path:
  1. Add pitch bend parameter per lane (±12 semitones)
  2. Integrate with TuningEngine for per-note frequency control
  3. Implement pitch bend envelope/LFO modulation per lane
  4. Add slide time parameter for glissando effects

**Real-time Tuning Change With Glide (O-Lyrica)**
- Problem: When tuning changes, all playing notes jump to new frequency instantly (no glide)
- Blocks: Professional real-time retuning requires smooth frequency transitions
- Implementation path:
  1. Implement frequency glide with configurable time constant
  2. Apply to all active voices; smooth target frequency changes
  3. Add glide curve option (linear, exponential, logarithmic)
  4. Test with MIDI note duration vs glide time (short notes with long glide should truncate)

**Parameter Smoothing on Preset Load**
- Problem: When presets load, all parameters jump instantly (audible clicks if automation playing)
- Blocks: Cannot use presets during live performance without clicks/pops
- Implementation path:
  1. Implement linear or exponential parameter smoothing on preset load
  2. Add configurable ramp time (50-500ms default)
  3. Apply smoothing only to parameters that changed (optimize for preset-with-mostly-same-values)
  4. Test with active automation to ensure smooth transitions

---

## Test Coverage Gaps

**Thread Safety of TuningEngine (Low Coverage)**
- What's not tested: Concurrent reads/writes to frequency table while intervals change
- Files: `plugins/O-Lyrica/Source/DSP/TuningEngine.h/cpp`, `plugins/O-Marimba/Source/TuningEngine.cpp`
- Risk: Race conditions on intervalMutex; frequency table access without lock_guard could read stale frequencies
- Priority: High - manifests only under specific timing conditions (hard to reproduce)
- Suggestion: Add thread safety tests; stress test with parameter changes while processing audio

**WebBrowserComponent Failure Modes (Not Tested)**
- What's not tested: WebView initialization failure, evaluateJavascript error handling, resource provider exceptions
- Files: `plugins/O-Bass/Source/PluginEditor.cpp`, `plugins/O-Lyrica/Source/PluginEditor.cpp:54-70`
- Risk: Plugin could silently fail UI rendering without error message
- Priority: High - affects user experience and debugging
- Suggestion: Add error callbacks to evaluateJavascript; test WebView recovery from failures

**Scala File Edge Cases (Minimal Coverage)**
- What's not tested: Malformed Scala files, missing headers, invalid cent values, scale degree mismatches
- Files: `plugins/O-Lyrica/Source/DSP/TuningEngine.cpp:470-530`, `plugins/O-Marimba/Source/TuningEngine.cpp`
- Risk: Parser may crash or produce incorrect results on unusual input
- Priority: Medium - users may provide malformed files
- Suggestion: Add unit tests for Scala parser with test file corpus (valid, malformed, edge cases)

**Preset Serialization Round-Trip (Not Tested)**
- What's not tested: Save preset, load plugin, load preset - ensure all state perfectly recovered
- Files: `plugins/O-Lyrica/Source/PluginProcessor.cpp:424-439`, preset manager implementations
- Risk: Preset corruption on load, parameter values out of range, tuning state inconsistent
- Priority: High - affects preset reliability
- Suggestion: Add integration tests: save preset, parse JSON/XML, verify all parameters in valid ranges

**MIDI Event Queue Overflow (No Coverage)**
- What's not tested: Behavior under high MIDI note density (should drop events gracefully)
- Files: `plugins/O-Lyrica/Source/PluginProcessor.h:29-72`
- Risk: Unknown if queue handles 100+ notes/second correctly
- Priority: Medium - visualization reliability under extreme MIDI
- Suggestion: Add stress test with rapid note-on/note-off events; verify visualization doesn't crash

**Buffer Size Edge Cases (Minimal Coverage)**
- What's not tested: Very small block sizes (32 samples), very large (4096+), irregular sizes, changes mid-session
- Files: All plugins with buffer allocation in prepareToPlay
- Risk: Crashes or audio glitches under edge case block sizes
- Priority: Medium - rare but breaks under certain DAW configurations
- Suggestion: Add parameterized tests for block sizes: 32, 64, 128, 256, 512, 1024, 2048, 4096

**Denormal Number Handling (Not Tested)**
- What's not tested: Audio behavior with very small amplitude signals (< 1e-38); denormal flush effectiveness
- Files: `plugins/O-Bass/Source/DSP/HarmonicGenerator.cpp`, `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp`
- Risk: CPU spikes when filters process denormal numbers
- Priority: Medium - affects performance with low-level noise inputs
- Suggestion: Add denormal input test signals; profile CPU with `-ffast-math` and without

---

*Concerns audit: 2026-01-29*
