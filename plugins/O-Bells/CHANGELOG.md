# O-Bells Changelog

All notable changes to O-Bells will be documented in this file.

## [1.1.0] - 2026-02-02

### Added
- **Reverb control** - New "Reverb" slider in the Output section for adding spaciousness to bell sounds
  - Range: 0-100% wet/dry mix
  - Default: 30%
  - Uses JUCE's high-quality reverb with optimized settings for metallic/bell tones
  - Positioned to the left of the Gain slider as requested

### Technical Details
- Added `reverbMix` parameter to APVTS (ID: "reverbMix", version 1)
- Implemented `juce::dsp::Reverb` with bell-optimized settings (roomSize: 0.7, damping: 0.4, width: 1.0)
- WebView UI binding via WebSliderRelay/WebSliderParameterAttachment
- Total parameters: 19 (was 18)

## [1.0.0] - 2026-02-02

### Added
- Initial release of O-Bells physical modeling bell synthesizer
- 18 parameters across 5 sections: Synthesis, Ensemble, Character, Advanced, Output
- WebView-based UI with botanical aesthetic
- 25 factory presets across 5 categories: Orchestral, Sacred, World, Ambient, Cinematic
- Full preset management with save/load functionality
- Real-time output metering
