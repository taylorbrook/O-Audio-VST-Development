# O-Prism Changelog

## v0.9.2 (2026-02-18)

### Fixed
- **Stereo filter distortion**: Mono filter + stereo balance reconstruction caused full-wave rectification on left channel and 3x amplification on right channel during negative signal excursions. All waveforms were severely distorted (sine sounded like square). Replaced with true stereo filter processing using independent L/R filter instances.
- **Wavetable selection mapping**: oscATable/oscBTable parameter range was [0, 15] but only 4 factory tables exist. UI dropdown normalized values mapped incorrectly — selecting Square or Triangle both loaded the Sine table. Fixed parameter range to [0, 3] matching the 4 factory waveforms.

## v0.9.1 (2026-02-18)

- Initial release with tuning panel v2.0.0
