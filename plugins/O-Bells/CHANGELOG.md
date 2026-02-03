# O-Bells Changelog

All notable changes to O-Bells will be documented in this file.

## [1.5.2] - 2026-02-03

### Added
- **Attack Amount slider in UI** - Now visible in Character section, underneath the Noise (attack type) selector
  - Controls the level of the strike transient (noise, thud, ping)
  - 0% = minimal transient, pure tone
  - 50% = natural transient level (default)
  - 100% = exaggerated transient, percussive

### Changed
- **Parameter label renamed** - "Attack" → "Attack Amount" for clarity

### Technical Notes
- Domain: UI only (parameter already existed since v1.3.0, just wasn't visible in UI)
- Total UI controls: 30 sliders + 2 choice params
- Preset compatibility: Fully compatible (parameter ID unchanged)

## [1.5.1] - 2026-02-03

### Changed
- **Bloom Speed readouts now display milliseconds** - All 4 bloom speed controls show actual time values instead of percentages
  - Main Bloom Speed: 25-400 ms (mid-partial range)
  - Bloom Speed Low: 15-250 ms
  - Bloom Speed Mid: 25-400 ms
  - Bloom Speed High: 50-800 ms

### Technical Notes
- Domain: UI (parameter display only, no DSP changes)
- Preset compatibility: Fully compatible (internal values unchanged)

## [1.5.0] - 2026-02-03

### Added
- **Bloom Fine Controls** - Per-band (Low/Mid/High) independent control of bloom speed and amount
  - 6 new parameters: `bloomSpeedLow`, `bloomSpeedMid`, `bloomSpeedHigh`, `bloomAmountLow`, `bloomAmountMid`, `bloomAmountHigh`
  - Toggle parameter `bloomFineEnabled` enables Override Mode
  - When enabled, main Bloom Speed/Amount sliders are greyed out and per-band controls take full effect
  - Expandable UI section with clear visual distinction between basic and advanced controls
  - Default values: All at 50% speed, 0% amount (matches behavior when disabled)

### Technical Notes
- Domain: DSP + UI
- Total parameters: 30 (was 23)
- Band definitions: Low (partials 0-1), Mid (partials 2-4), High (partials 5-7)
- Preset compatibility: New params default to values that match current behavior

## [1.4.1] - 2026-02-03

### Changed
- **Bloom Speed range expanded** - Much wider duration ranges for more dramatic bloom effects
  - Low partials: 15-250ms (was 5-15ms)
  - Mid partials: 25-400ms (was 30-120ms)
  - High partials: 50-800ms (was 50-200ms)
- **Low partial bloom amount increased** - More audible bloom on foundation tones
  - Low partials: 0-40% reduction (was 0-5%)

## [1.4.0] - 2026-02-03

### Added
- **Bloom Speed parameter** - Independent control over bloom duration (0-100%)
  - Low partials: 5-15ms bloom time
  - Mid partials: 30-120ms bloom time
  - High partials: 50-200ms bloom time
- **Bloom Amount parameter** - Independent control over bloom intensity (0-100%)
  - Controls how much partials swell from initial to peak amplitude
  - Low partials: subtle effect (0-5% reduction)
  - Mid partials: moderate effect (0-60% reduction)
  - High partials: dramatic effect (0-90% reduction)

### Changed
- **Bloom parameter split** - The original single "Bloom" parameter is now two separate controls:
  - "Bloom Speed" controls how fast the swell occurs
  - "Bloom Amount" controls the intensity of the spectral swelling effect
  - Provides finer control over the organic "breath" effect on bell attacks

### Breaking Changes
- **Bloom parameter replaced** - Old presets with `bloom` parameter will not load correctly
  - Users should resave presets after loading in v1.4.0
  - Default values: Speed=50%, Amount=0% (bloom off by default)

### Technical Notes
- Domain: DSP + UI
- Total parameters: 23 (was 22)
- Pluginval: Passes Level 5 (VST3 and AU)

## [1.3.0] - 2026-02-03

### Added
- **Attack parameter** - New slider (0-100%) controls strike transient volume
  - 0% = minimal transient, pure tone
  - 50% = natural transient level (default)
  - 100% = exaggerated transient, percussive

### Changed
- **Shimmer quality improvement** - Wider LFO range (0.1-8 Hz) with better desynchronization
  - Replaced prime ratios with more spread values for organic metallic shimmer
  - No more audible LFO synchronization patterns at any setting
- **Material differentiation** - Exaggerated material properties for audible distinction
  - Bronze: Baseline warm church bell (1.0x decay)
  - Brass: Bright, short, jazzy (0.7x decay, +0.20 brightness)
  - Steel: Very bright, long sustain (2.0x decay, +0.25 brightness)
  - Aluminum: Very bright, short, thin (0.5x decay, +0.30 brightness)
  - Cast Iron: Dark, long, gamelan-like (1.5x decay, -0.25 brightness)
- **Attack noise overhaul** - Replaced filtered noise with impulse-driven resonant filter bank
  - 4 resonant filters tuned to first 4 partials
  - Q values based on strike character (Click=10, Thud=2, Ping=5)
  - Strike transients now sound like physical mallet impact

### Fixed
- **Bloom bug** - Bloom effect now produces audible amplitude swell
  - Fixed decay masking bloom by delaying decay until bloom completes
  - Added spectral bloom with staggered partial timing (low partials instant, high partials fade in)
- **Per-note variation** - Repeated strikes now sound subtly different
  - Added Gaussian-distributed pitch variation (±10 cents)
  - Added amplitude variation (±25%, clamped 50%-150%)

### Breaking Changes
- **Material parameter type changed** from continuous slider to discrete dropdown
  - Old presets will round material value to nearest discrete option
  - Users should resave presets after loading in v1.3.0

### Technical Notes
- Domain: DSP
- Milestone: acoustic-realism-v2
- Total parameters: 22 (was 21)
- Pluginval: Passes Level 5 (VST3 and AU)

## [1.2.0] - 2026-02-02

### Added
- **Bloom parameter** - Spectral swelling effect where partials swell from initial to peak amplitude before decay
  - Range: 0-100%, Default: 0% (off)
  - Creates organic "breath" to bell attacks
- **Shimmer parameter** - Frequency modulation that increases during decay for metallic shimmering effect
  - Range: 0-100%, Default: 20%
  - Prime-ratio LFOs prevent phase locking between partials
  - Intensity increases as bell decays for authentic bell shimmer
- **Mallet temporal spreading** - Soft mallets now have gradual 50ms attack, hard mallets remain instant
- **Stereo enhancement infrastructure** - Per-partial panning, slow pan LFOs, and Haas delay functions implemented

### Changed
- **Material system overhaul** - Replaced 4-material system with research-based 5-material acoustic model
  - Bronze (1.0x decay, neutral), Brass (0.9x, +0.05 brightness), Steel (1.4x, +0.10 brightness)
  - Aluminum (0.7x, +0.15 brightness), Cast Iron (1.2x, -0.10 brightness, gamelan-like)
  - Each material now affects decay, brightness, AND inharmonicity
- **Inharmonicity label** - UI now displays full word "Inharmonicity" instead of "Inharm"

### Removed
- **Decay Shape parameter** - Multi-stage decay envelope is now always active
  - Simplified UI with more predictable sound
  - Note: Presets with old `decayShape` entries will ignore them

### Technical Notes
- Domain: DSP
- Milestone: realism-overhaul
- CPU overhead: ~4.3% increase for new features
- Memory: ~23KB increase (8 voices with bloom/shimmer/stereo state)
- Preset compatibility: Existing presets load with bloom=0%, shimmer=20% defaults

## [1.1.1] - 2026-02-02

### Fixed
- **Output clipping at default settings** - Added proper gain staging normalization in DSP
  - Root cause: 8 partials summing to ~2.7x, plus unison voices and octave layers, caused signal to exceed 0 dBFS
  - Fix: Normalize signal for partial count (0.4x), unison voices (sqrt), and octave layer blend before output gain stage
  - Output gain at 0 dB now produces unity gain as expected

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
