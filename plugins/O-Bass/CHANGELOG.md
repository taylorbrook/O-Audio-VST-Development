# O-Bass Changelog

## [1.3.1] - 2026-01-28

### Added
- **VU Meter**: Horizontal output level meter at bottom center with smooth ballistics
- Timer-based metering from C++ (30fps, same reliable pattern as O-Comp)

### Changed
- **Improved layout**: Title on top row, preset bar below, 3 knobs in single row, meter at bottom
- Window size refined to 420×320 pixels

## [1.3.0] - 2026-01-28

### Changed
- **Compact UI**: Reduced window size from 500×450 to 400×350 pixels (better fit for 3-knob layout)
- **Display title**: Changed in-plugin title from "O-Bass" to "Ouaricon Bass"

## [1.2.1] - 2026-01-28

### Changed
- **More dramatic harmonic enhancement**: Increased DSP coefficients for noticeably stronger bass effect
  - `kInputDrive`: 2.0 → 4.0 (more saturation into waveshaper)
  - `kH2Weight`: 0.5 → 0.8 (stronger 2nd harmonic warmth)
  - `kH3Weight`: 0.3 → 0.5 (more 3rd harmonic presence)
  - `kHarmonicMix`: 0.7 → 1.2 (louder harmonic content in mix)
- Root cause: Previous coefficients were too conservative, effect was barely audible at max enhance

## [1.2.0] - 2026-01-28

### Changed
- Code quality cleanup: removed unused code, extracted magic numbers to constants, added documentation

## [1.1.1] - 2026-01-27

### Performance
- **IIR coefficient updates optimized**: Filter coefficients now update every 16 samples during parameter smoothing instead of per-sample. Reduces CPU overhead during crossover frequency automation.
- **Buffer resize checks removed**: Replaced runtime buffer size checks in processBlock() with debug-only jassert assertions. Buffers are pre-allocated in prepareToPlay() making runtime checks unnecessary overhead.

## [1.1.0] - 2026-01-27

### Removed
- **Colored Mode**: Removed entirely (parameter, UI toggle, DSP processor)
  - ColoredModeProcessor.h/.cpp deleted
  - `enhanceMode` parameter removed
  - Mode toggle removed from WebView UI
  - Factory presets simplified (no more mode-specific presets)

### Fixed
- **Dead code removed**: ~80 lines of unreachable code after early `return` in processBlock()
- **Limit indicator now works**: Output gain stage with soft clipping restored
- **Buffer validation**: Added channel check in HarmonicGenerator::process()

### Changed
- Plugin now has a single, clean processing path
- Factory presets renamed ("Aggressive Colored" -> "Maximum Enhancement")

## [1.0.2] - 2026-01-27

- Renamed from OBass to O-Bass

## [1.0.1] - 2026-01-26

- Added buffer size validation in CleanModeProcessor
- Disabled Colored mode pending further testing

## [1.0.0] - 2026-01-26

- Initial release
- Crossover filtering with LR4 topology
- Dual mode: Clean (transparent) and Colored (warm) enhancement
- WebView UI with botanical aesthetic
