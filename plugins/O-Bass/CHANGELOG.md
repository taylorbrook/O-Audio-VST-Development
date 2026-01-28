# O-Bass Changelog

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
