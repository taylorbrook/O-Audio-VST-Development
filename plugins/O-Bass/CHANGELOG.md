# O-Bass Changelog

## [1.0.2] - 2026-01-27

### Changed
- **Renamed plugin from OBass to O-Bass** - Matches Ouaricon naming convention (O-Tremolo, O-Comp, O-Marimba, etc.)
  - Updated CMake target, product name, and plugin code
  - Updated all source file headers and getName() return value
  - Renamed plugin folder from `plugins/OBass/` to `plugins/O-Bass/`
  - Updated PLUGINS.md registry entry

### Notes
- This is a breaking change for existing DAW sessions (plugin ID changed)
- Users will need to re-add the plugin in existing projects

## [1.0.1] - 2026-01-27

### Fixed
- **Logic Pro crash with "Sample Rate XXXXX" error** - Root cause: Memory corruption from buffer size mismatches in DSP processing chain
  - CleanModeProcessor: Removed audio-thread allocation (`setSize()` in process loop), added buffer size validation
  - HarmonicGenerator: Added guard against processing buffers larger than prepared size (oversampler internal buffers)
  - PitchTracker: Now stores and validates `maxBlockSize` from prepare()
- Re-enabled CleanModeProcessor which was temporarily disabled due to crash

### Technical Notes
- The "Sample Rate 15,595" error was not a sample rate issue - it was Logic Pro interpreting corrupted memory as a sample rate value
- Buffer size validation now rejects oversized buffers gracefully (pass-through) rather than causing memory corruption
- Colored mode crossfade remains disabled pending similar fixes to ColoredModeProcessor

## [1.0.0] - 2026-01-27

### Added
- Initial release
- Crossover filtering with low/high band separation
- Clean mode for transparent bass enhancement
- Colored mode with harmonic generation
- Mono summing for bass frequencies with stereo expansion
- Envelope follower for dynamic response
- Pitch tracking for intelligent processing
- WebView UI with botanical/paper aesthetic
- Preset management system
- Low Latency and High Fidelity crossover modes
- Limit indicator for visual feedback
