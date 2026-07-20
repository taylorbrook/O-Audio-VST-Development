# Changelog

All notable changes to FlutterVerb will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.3] - 2025-11-12

### Fixed

- **UI Hover Stability:** Eliminated subtle shaking when hovering over knobs
  - **Root Cause:** `transform: scale(1.005)` in `.knob:hover` caused sub-pixel rendering recalculation in WebView, creating imperceptible but continuous layout shifts
  - **Solution:** Replaced scale transform with border color change (matching toggle switch behavior)
  - **Implementation:** Added `border: 2px solid rgba(212, 165, 116, 0.3)` to `.knob-body`, hover changes to `#c49564` (gold accent)
  - **User Impact:** Knobs now have stable, clean hover effect without visual artifacts
  - **Technical Details:** WebView/WebBrowserComponent struggles with micro-scale transforms (<1%), causing render loop instability
  - **Testing:** Verified across all 7 knobs—no shake on hover, depression effect on click preserved
  - **Backward Compatibility:** Zero impact—visual-only change, no parameter or state modifications

## [1.0.2] - 2025-11-11

### Fixed
- **VU meter animation**: VU meter now displays with smooth ballistic needle movement
  - Root cause: Missing requestAnimationFrame loop and target/current angle tracking
  - Impact: VU meter needle didn't animate despite receiving events
  - Solution: Added animateVUMeter() loop with attack/decay ballistics (TapeAge pattern)

- **MOD_MODE toggle UI**: Toggle switch now clickable and visually updates
  - Root cause: JavaScript used `getToggleButtonState()` instead of `getToggleState()`
  - Impact: Switch not clickable, no visual feedback
  - Solution: Changed to `Juce.getToggleState()` (correct JUCE WebView API)

- **MOD_MODE functionality**: TONE and DRIVE now correctly route based on mode
  - Root cause: TONE/DRIVE applied after dry/wet split, only affected wet signal in both modes
  - Impact: Mode 1 (WET+DRY) didn't affect dry signal as expected
  - Solution: Refactored DSP to apply effects before split in Mode 1, after reverb in Mode 0

- **SIZE/DECAY independence**: Decay now works at all SIZE settings including 0%
  - Root cause: JUCE Reverb roomSize=0 collapses delay lines, making decay ineffective
  - Impact: Decay knob had no effect when SIZE=0%
  - Solution: Map SIZE to minimum base roomSize (0.2-0.6), multiply by decay factor (0.5-2.0x)

### Technical
- DSP changes:
  - SIZE mapping: 0-100% → roomSize 0.2-0.6 (base) × 0.5-2.0 (decay multiplier)
  - TONE/DRIVE extracted into lambdas for conditional routing
  - Signal flow Mode 0: Input → Reverb → Mod → Drive → Tone → Mix
  - Signal flow Mode 1: Input → Mod → Drive → Tone → [Split/Reverb/Mix]
- UI changes:
  - MOD_MODE: Changed to `getValue()`/`setValue()` with boolean type
  - VU meter: Added currentNeedleAngle/targetNeedleAngle tracking
  - VU meter: requestAnimationFrame loop with 0.4 attack / 0.15 decay speeds

### Testing
- VU meter: Verified smooth needle movement with ballistic response
- MOD_MODE toggle: Click response confirmed, visual state syncs with parameter
- MOD_MODE routing: Audio tests confirm TONE affects both paths in Mode 1
- SIZE/DECAY: Verified decay works at SIZE=0%, independent parameter control

## [1.0.1] - 2025-11-11

### Fixed
- **Decay independence**: Decay parameter now works independently of SIZE parameter
  - Root cause: JUCE Reverb's damping was coupled with room size
  - Impact: Small room sizes can now have long decay times (was artificially limited)
  - Solution: Adjusted damping range mapping for better parameter independence

- **Drive latency compensation**: Fixed ~50ms timing misalignment between dry and wet signals
  - Root cause: Modulation delay buffer (50ms base) without latency compensation
  - Impact: Audible phasing/delay when drive active with mixed dry/wet signal
  - Solution: Added `DryWetMixer::setWetLatency()` compensation in prepareToPlay()

- **AGE parameter scaling**: AGE knob now has exponential curve for better usability
  - Root cause: Linear 0-100% scaling made parameter too extreme above 15%
  - Impact: Most useful range compressed into narrow knob range (0-15%)
  - Solution: Applied exponential scaling (value²) - smooth control 0-50%, extremes at 100%

- **MOD_MODE switch non-functional**: Toggle switch now clickable and functional
  - Root cause: JavaScript used `getToggleButtonState()` which doesn't exist in JUCE WebView API
  - Impact: Switch stuck on "WET ONLY", couldn't toggle to "WET+DRY" mode
  - Solution: Changed to `getSliderState()` (boolean parameters use 0/1 slider state)

- **VU meter not responding**: VU meter now displays real-time output levels
  - Root cause: Atomic variable storing linear gain but not using correct pattern
  - Impact: VU meter needle didn't move (no visual feedback)
  - Solution: Store level in dB format directly (TapeAge pattern), pass to JavaScript as dB

### Technical
- DSP changes:
  - Damping range adjusted: 0.95-0.05 (was 0.9-0.1) for better decay independence
  - DryWetMixer latency compensation: 50ms (2400 samples @ 48kHz)
  - AGE modulation depth: exponential scaling instead of linear
- Parameter changes:
  - No parameter IDs changed (backward compatible)
  - MOD_MODE: Boolean parameter now properly bound to UI toggle
- UI changes:
  - MOD_MODE toggle: Changed from `getToggleButtonState` to `getSliderState`
  - VU meter: JavaScript now receives dB values instead of normalized 0-1

### Testing
- All 5 fixes tested in isolation
- No regressions detected
- Presets remain compatible

## [1.0.0] - 2025-11-11

### Added
- Initial release of FlutterVerb
- Plate reverb with SIZE and DECAY controls
- Wow and flutter modulation system (AGE parameter)
- Tape saturation with DRIVE control
- DJ-style filter with TONE control (-100% LP to +100% HP)
- MOD_MODE toggle (wet-only vs wet+dry modulation routing)
- Real-time VU meter with peak level display
- WebView UI with 6 knobs + toggle switch
- 7 factory presets showcasing different use cases

### Technical Details
- VST3 and AU formats
- Stereo processing
- JUCE 8 framework
- Real-time safe DSP implementation
- Thread-safe parameter communication
- Zero-latency processing (no look-ahead required)

### Features
- Dual LFO modulation (wow: 1Hz, flutter: 6Hz)
- Lagrange 3rd-order interpolation for smooth pitch modulation
- Exponential filter cutoff mapping for musical response
- VU meter ballistics (fast attack, slow release)
- Bypass zone for center filter position (transparent)
- State reset on filter type transitions (no burst artifacts)

### Known Limitations
- None at this time

[1.0.0]: https://github.com/yourusername/plugin-freedom-system/releases/tag/flutterverb-v1.0.0
