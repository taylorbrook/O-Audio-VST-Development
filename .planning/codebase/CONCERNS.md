# Codebase Concerns

**Analysis Date:** 2026-01-22

## Tech Debt

**MTS-ESP Integration (Incomplete)**
- Issue: MTS-ESP mode is stubbed - falls back to frequency table without actual integration
- Files: `plugins/OuariconMarimba/Source/TuningEngine.cpp:30-35`, `plugins/OuariconLyrica/Source/DSP/TuningEngine.h:Mode::MTSESP`
- Impact: Plugins cannot use MIDI Tuning Standard for dynamic retuning from DAW - full custom tuning requires file loading
- Fix approach: Complete MTS-ESP protocol implementation (requires external MTS-ESP library, thread-safe buffer for MIDI tuning data)

**Raw Pointer Allocations in Audio Thread**
- Issue: `new HarpSynthVoice()` and `new HarpSynthSound()` called during plugin initialization
- Files: `plugins/OuariconLyrica/Source/PluginProcessor.cpp:239,247`
- Impact: Allocation happens at startup (not during processing), but uses raw `new` instead of `std::make_unique<>` for consistency with codebase patterns
- Fix approach: Replace with `std::make_unique<>` for memory management consistency

**JavaScript Error Handling in WebView**
- Issue: JavaScript errors in WebView bindings can cascade - one failed ComboBox binding stops all subsequent bindings
- Files: `plugins/OuariconLyrica/Source/PluginEditor.cpp:618,625,653` (evaluateJavascript calls have no error callback)
- Impact: One script error prevents remaining event listeners from attaching (e.g., v1.5.3 preset system was broken by ComboBox error)
- Fix approach: Wrap all evaluateJavascript calls with error callbacks; add console error logging to UI JavaScript

**Spinlock Pattern in MIDI Event Queue**
- Issue: Custom spinlock (compare_exchange_strong loop) used for queue synchronization instead of standard lock
- Files: `plugins/OuariconLyrica/Source/PluginProcessor.h:36-52`
- Impact: Spinlock can cause high CPU if contention occurs; try-lock pattern intentionally drops events under contention (acceptable for visualization but fragile if requirements change)
- Fix approach: Document the design trade-off clearly; consider using lock_guard with timeout if queue overflows become an issue

## Known Bugs

**GUI Keyboard Does Not Reflect Custom Tuning (OuariconLyrica)**
- Symptoms: On-screen keyboard layout remains standard 12-TET visually even when custom tuning is active; audio plays correct frequencies but visual feedback is missing
- Files: `plugins/OuariconLyrica/Resources/ui/index.html` (keyboard rendering), tuning data not exposed to UI
- Trigger: Load temperament preset, edit interval cents, or load Scala file - keyboard displays unchanged
- Workaround: Audio is correct; visual feedback is missing only. Use MIDI note numbers as reference instead of visual keyboard layout.

**Polystutter AU Blank UI in Logic (v1.3.1, Fixed in v1.3.2)**
- Symptoms: AU plugin loads in Logic but UI displays blank; VST3 and Standalone work fine
- Root cause: Missing VERSION property in CMakeLists.txt caused AU to report old version; Logic cached stale AU config from when sidechain bus existed
- Files: `plugins/OuariconPolystutter/CMakeLists.txt`
- Status: Fixed by adding `VERSION 1.3.2` to juce_add_plugin(); forces Logic to invalidate cache
- Prevention: Always update VERSION in CMakeLists.txt when bus configuration or parameter count changes

**Preset UI Not Responsive in OuariconLyrica (v1.5.3, Fixed in v1.5.4)**
- Symptoms: Preset buttons visible and styled but non-functional; clicking has no effect
- Root cause: ComboBox binding threw uncaught JavaScript error that stopped module initialization before preset event listeners attached
- Files: `plugins/OuariconLyrica/Resources/ui/index.html` (preset system lines 993-1199)
- Status: Fixed in v1.5.4 by wrapping ComboBox binding in try-catch and using inline get-and-call pattern
- Prevention: All JavaScript binding code now wrapped in try-catch; still fragile if JUCE bridge API changes

## Security Considerations

**Scala File Parsing (No Input Validation)**
- Risk: Scala file parser reads line by line but lacks bounds checking on degree counts, interval values, and file size
- Files: `plugins/OuariconLyrica/Source/DSP/TuningEngine.cpp:470-530` (calculateScala), `plugins/OuariconMarimba/Source/TuningEngine.cpp`
- Current mitigation: Max 128 MIDI notes limits practical scale size; juce::String parsing prevents buffer overflows
- Recommendations:
  1. Add explicit validation: max degree count (256), interval range (0-12000 cents), max file size (100KB)
  2. Validate that scale degrees match scale name size
  3. Add error reporting when malformed Scala files are loaded

**Web Resource Provider in WebBrowserComponent**
- Risk: `getResource()` serves binary data and JavaScript from embedded resources without origin validation
- Files: `plugins/OuariconLyrica/Source/PluginEditor.cpp:656-700`
- Current mitigation: All resources are embedded (no network calls); resource URLs restricted to plugin domain
- Recommendations:
  1. Validate URL format before serving resources
  2. Restrict resource serving to known safe paths (no directory traversal)
  3. Keep JUCE framework updated (WebKit security patches)

**Atomic Variables Without Memory Order Specification**
- Risk: Some atomics use default memory order; could cause unexpected synchronization behavior in edge cases
- Files: `plugins/OuariconLyrica/Source/DSP/TuningEngine.h:324-325,338` (tonicOffset, frequencyTable)
- Current mitigation: Most critical paths specify explicit memory order (acquire/release); non-critical visualization uses relaxed
- Recommendations: Document memory order assumptions; add comments explaining why relaxed order is safe for specific variables

## Performance Bottlenecks

**Sympathetic Resonance Engine: O(MAX_VOICES²) Coupling Calculation**
- Problem: Coupling matrix rebuilt every voice registration change; O(16²) = 256 comparisons for each of 16 possible voices
- Files: `plugins/OuariconLyrica/Source/DSP/SympatheticResonance.cpp:235-280` (buildCouplingMatrix)
- Cause: Double-buffered design requires full rebuild when topology changes; happens at block boundaries, not per-sample
- Improvement path:
  1. Profile actual CPU impact (likely negligible with MAX_VOICES=16)
  2. If needed, implement incremental updates instead of full rebuild
  3. Consider caching coupling strengths between static frequencies

**UI Polling Every 30ms (timerCallback)**
- Problem: Timer fires 30+ times per second to pull MIDI events and update visualization; JavaScript execution + string concatenation overhead
- Files: `plugins/OuariconLyrica/Source/PluginEditor.cpp:606-654`
- Cause: Design choice to update WebView at fixed interval rather than event-driven updates
- Improvement path:
  1. Profile WebView JavaScript execution time during high note density
  2. Consider reducing polling frequency if MIDI density is low (dynamic rate)
  3. Batch multiple MIDI events into single JavaScript call
  4. Use requestAnimationFrame instead of polling for smooth animations

**Frequency Table Rebuild on Parameter Changes**
- Problem: Any tuning parameter change (octave stretch, tonic, master tune) triggers complete frequency table rebuild (128 entries × log2 calculation)
- Files: `plugins/OuariconLyrica/Source/DSP/TuningEngine.cpp:82-89,103-107` (setMasterTune, setOctaveStretch)
- Cause: Simplicity over optimization; happens on UI thread when parameters change
- Improvement path:
  1. Batch multiple parameter changes before rebuild (debounce)
  2. Rebuild only affected octaves if octave stretch changes
  3. Cache frequency calculations for common frequencies

**RepeatLane Capture Buffer Circular Write**
- Problem: Every sample writes to circular buffer (5 seconds × 192kHz = 960K samples); no bounds checking on write position wraparound
- Files: `plugins/OuariconPolystutter/Source/DSP/RepeatLane.cpp:72-87`
- Cause: Linear allocation at sample rate; modulo operation happens per sample
- Improvement path:
  1. Use power-of-2 buffer size to replace modulo with bitwise AND
  2. Profile actual CPU impact (likely < 1% with modern CPUs)

## Fragile Areas

**TuningEngine Interval Mutex Pattern**
- Files: `plugins/OuariconLyrica/Source/DSP/TuningEngine.h:331`, `plugins/OuariconLyrica/Source/DSP/TuningEngine.cpp:225,248,269,297,516,559,573,670`
- Why fragile:
  1. 8 separate lock_guard sites - any missed lock_guard when reading intervals causes race condition
  2. scaleIntervals vector can be modified (clear/push_back) under lock but read without lock during frequency table calcs
  3. If interval loading fails mid-file, vector left in inconsistent state
- Safe modification:
  1. Always wrap interval reads in lock_guard
  2. Use vector swap pattern for atomic interval replacement
  3. Validate complete interval set before committing (atomicity)
  4. Add bounds checking on all interval access

**MIDI Event Queue Circular Buffer**
- Files: `plugins/OuariconLyrica/Source/PluginProcessor.h:68-72`
- Why fragile:
  1. Drop-on-contention design acceptable for visualization, fragile if used for reliable data
  2. Fixed size (32 events) - no indication of overflow; events silently dropped under high MIDI density
  3. Timer callback assumption that readPos only advances from timer thread
- Safe modification:
  1. Document this is visualization-only and event loss is acceptable
  2. Add metrics for dropped events (help debug real usage patterns)
  3. If reliability needed, switch to lock-free queue library
  4. Add DBG logging when events are dropped

**WebBrowserComponent JavaScript Injection**
- Files: `plugins/OuariconLyrica/Source/PluginEditor.cpp:615-653` (evaluateJavascript in timerCallback)
- Why fragile:
  1. Inline string concatenation of numbers into JavaScript (potential injection if values contain quotes)
  2. No error handling - if evaluateJavascript fails, subsequent calls may break WebView state
  3. Timer callback runs on UI thread; WebView updates must complete before next callback (no async handling)
- Safe modification:
  1. Use parameterized JavaScript instead of string concatenation (e.g., pass JSON objects)
  2. Implement error callback for evaluateJavascript to log failures
  3. Queue JavaScript updates; only evaluate if WebView ready
  4. Consider native message passing instead of JavaScript evaluation for data updates

**Parameter Validation in Preset Loading**
- Files: `plugins/OuariconLyrica/Source/OuariconPresetManager.cpp` (preset save/load), `plugins/OuariconMarimba/Source/PresetManager.cpp`
- Why fragile:
  1. No validation that loaded preset parameter values are in valid ranges
  2. Mal-formed JSON causes silent failure or exceptions
  3. Preset files stored as-is without integrity checks
- Safe modification:
  1. Validate all loaded parameter values against AudioProcessor parameter ranges
  2. Add try-catch around JSON parsing with user-friendly error messages
  3. Store version/checksum with preset to detect file corruption
  4. Implement migration logic if parameter definitions change

## Scaling Limits

**Sympathetic Resonance: MAX_VOICES = 16**
- Current capacity: 16 simultaneous voices with up to 15 couplings each
- Limit: Cannot exceed 16 voices; additional voices cause coupling data loss or undefined behavior
- Scaling path:
  1. Profile memory/CPU at MAX_VOICES=16 (fixed arrays, no dynamic alloc)
  2. If headroom exists, increase MAX_VOICES to 32 (O(n²) coupling matrix becomes 1024 entries vs 256)
  3. For larger systems, switch to dynamic allocation (but add audio thread safety mechanisms)

**Scala File Support: Max 128 MIDI Notes**
- Current capacity: TuningEngine supports up to 128 frequencies (one per MIDI note)
- Limit: Scala scales with > 128 degrees will be truncated
- Scaling path:
  1. Support arbitrary Scala scale sizes (store up to 1000 degrees)
  2. Map arbitrary scales to 128 MIDI notes using keyboard mapping (KBM) properly
  3. Currently KBM mapping is parsed but not fully utilized for arbitrary scales

**Capture Buffer: 5 seconds at 192kHz**
- Current capacity: RepeatLane allocates 960,000 samples per channel (5 sec @ 192k)
- Limit: Longer than 5 seconds is not captured; complex samples exceed this at high sample rates
- Scaling path:
  1. Make capture duration configurable parameter (0.5 - 10 seconds)
  2. Warn user if exceed available heap (allocate on first play, not init)
  3. Consider memory-mapped file buffer for ultra-long captures

## Dependencies at Risk

**JUCE Framework WebBrowserComponent (macOS WebKit)**
- Risk: WebKit security updates required periodically; custom resource provider must stay compatible with JUCE API changes
- Impact: WebView initialization could break on JUCE version upgrade if WebKit API changes
- Migration plan:
  1. Use JUCE getLiveConstantValue to detect WebKit features and adapt
  2. Keep test suite for WebView rendering (check tuning circle, presets, etc.)
  3. Monitor JUCE release notes for WebBrowserComponent changes
  4. Plan fallback to non-WebView UI if WebBrowserComponent becomes unmaintainable

**Scala File Parsing (Custom Implementation)**
- Risk: Format parsing is custom (not using external libscala); future Scala format changes will require manual updates
- Impact: New Scala extensions (e.g., custom headers, new directives) will be silently ignored or cause parse failures
- Migration plan:
  1. Monitor Scala format specification updates
  2. Add unit tests for Scala parsing (compare against libscala if available)
  3. Version the parser to track supported Scala feature set
  4. Consider vendoring lightweight Scala parser library if one becomes available

## Missing Critical Features

**Scala KBM Full Support**
- Problem: KBM files are parsed but keyboard mapping not fully applied to custom scales
- Blocks: Cannot map arbitrary scales properly to 128 MIDI keys; scales > 128 degrees are truncated instead of mapped
- Implementation path: Use KBM mapping to intelligently distribute scales across keyboard

**Pitch Bend Per-Voice in Polystutter**
- Problem: Polystutter does not support per-voice pitch modulation during playback
- Blocks: Cannot create smooth glissandos or frequency modulation effects in repeat lanes
- Implementation path: Add pitch bend parameter per lane; integrate with TuningEngine for per-note frequency control

**Real-time Tuning Change (No Glide)**
- Problem: When tuning changes, all playing notes jump to new frequency instantly (no glide/portamento)
- Blocks: Professional real-time retuning requires smooth frequency transitions
- Implementation path: Implement frequency glide with configurable time constant; apply to all active voices

**Parameter Smoothing on Load**
- Problem: When presets load, all parameters jump instantly (audible clicks if automation or audio is playing)
- Blocks: Cannot use presets during live performance without clicks/pops
- Implementation path: Implement linear or exponential parameter smoothing on preset load; configurable ramp time

## Test Coverage Gaps

**Thread Safety of TuningEngine (Low Coverage)**
- What's not tested: Concurrent reads/writes to frequency table while intervals change
- Files: `plugins/OuariconLyrica/Source/DSP/TuningEngine.h/cpp`
- Risk: Race conditions on intervalMutex; frequency table access without lock_guard could read stale frequencies
- Priority: High - manifests only under specific timing conditions (hard to reproduce)
- Suggestion: Add thread safety tests (stress test with parameter changes while processing)

**WebBrowserComponent Failure Modes (Not Tested)**
- What's not tested: WebView initialization failure, evaluateJavascript error handling, resource provider exceptions
- Files: `plugins/OuariconLyrica/Source/PluginEditor.cpp:54-70`
- Risk: Plugin could silently fail UI rendering without error message
- Priority: High - affects user experience and debugging
- Suggestion: Add error callbacks to evaluateJavascript; test WebView recovery from failures

**Scala File Edge Cases (Minimal Coverage)**
- What's not tested: Malformed Scala files, missing headers, invalid cent values, scale degree mismatches
- Files: `plugins/OuariconLyrica/Source/DSP/TuningEngine.cpp:470-530` (calculateScala)
- Risk: Parser may crash or produce incorrect results on unusual input
- Priority: Medium - users may provide malformed files
- Suggestion: Add unit tests for Scala parser with test file corpus

**Preset Serialization Round-Trip (Not Tested)**
- What's not tested: Save preset, load plugin, load preset - ensure all state perfectly recovered
- Files: `plugins/OuariconLyrica/Source/PluginProcessor.cpp:424-439`
- Risk: Preset corruption on load, parameter values out of range, tuning state inconsistent
- Priority: High - affects preset reliability
- Suggestion: Add integration tests: save preset, parse XML, verify all parameters in valid ranges

**MIDI Event Queue Overflow (No Coverage)**
- What's not tested: Behavior under high MIDI note density (should drop events gracefully)
- Files: `plugins/OuariconLyrica/Source/PluginProcessor.h:29-72`
- Risk: Unknown if queue handles 100+ notes/second correctly
- Priority: Medium - visualization reliability under extreme MIDI
- Suggestion: Add stress test with rapid note-on/note-off events; verify visualization doesn't crash

---

*Concerns audit: 2026-01-22*
