# AngelGrain Changelog

All notable changes to AngelGrain will be documented in this file.

## [1.1.0] - 2025-11-19

### Changed
- **Stereo input support**: Plugin now accepts stereo input and preserves the stereo field throughout processing
  - BusesProperties changed from mono-in/stereo-out to stereo-in/stereo-out
  - Grain buffer now stores and processes both L/R channels
  - Feedback loop operates in stereo
  - Pan randomization crossfades between stereo channels rather than panning a mono signal

### Fixed
- Stereo source material no longer collapses to mono in the wet signal
- Dry/wet mix at lower values no longer causes phase issues between stereo dry and mono wet

### Technical Details
- Root cause: Original design used `AudioChannelSet::mono()` for input, forcing DAW to sum stereo to mono
- Solution: Dual-channel DelayLine with stereo feedback path
- Reference: Pattern from Scatter plugin implementation

## [1.0.0] - 2025-11-19

### Added
- Initial release
- Granular delay with 32 polyphonic grain voices
- Parameters: Delay Time, Grain Size, Feedback, Chaos, Character, Mix, Tempo Sync
- Pitch quantization to octaves and fifths (harmonically coherent)
- Character morphing from glitchy (sparse, short crossfades) to smooth (dense, long crossfades)
- Chaos master randomization (position, pitch, pan, timing)
- Tempo sync to host BPM
- WebView UI with soft neomorphic design
- Linear dry/wet mixing for intuitive behavior at 50%
