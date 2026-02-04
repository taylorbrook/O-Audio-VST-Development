# O-Bells Changelog

All notable changes to O-Bells will be documented in this file.

## [2.4.0] - 2026-02-04

### Added
- **Humanize parameter** (0-100%) - Per-note random variation for organic bell realism
  - Varies strike position (±5%) per note - simulates hitting different spots on bell
  - Varies mallet hardness (±10%) per note - simulates varying strike force
  - Varies decay time (±15%) per note - each ring is slightly different
  - Varies attack time (±20%) per note - soft mallet bounces vary
  - Varies inharmonicity (±3%) per note - bell shape micro-variations
  - Uses Gaussian distribution (Central Limit Theorem approximation) for natural variation
  - Default 30% humanization for subtle organic character
  - Set to 0% for deterministic/mechanical behavior (backwards compatible)

### Technical Details
- New APVTS parameter: `humanize` (0.0-1.0)
- Per-note variation state stored in BellVoice (calculated once per note-on)
- Variations applied multiplicatively to base parameter values
- UI slider added to Advanced section in WebView interface
- No impact on CPU when humanize = 0% (variations all equal 1.0)

### Research Sources
- [AAS Chromaphone](https://www.applied-acoustics.com/chromaphone-3/) physical modeling techniques
- [Noise Engineering humanization](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/humanization-and-variation/) principles
- Gaussian distribution for natural variation (CLT approximation already existed in codebase)

## [2.3.0] - 2026-02-03

### Added
- **16-voice polyphony** - Increased from 8 to 16 simultaneous voices
  - Allows more complex chord voicings and sustained passages
  - Voice stealing now triggers at 17th note instead of 9th

## [2.2.1] - 2026-02-03

### Changed
- **Complete factory preset redesign** - 25 new presets with descriptive, evocative names
  - Replaced culturally-specific names with descriptive acoustic character names
  - Full utilization of v2.2.0 parameters: airAbsorption, airAbsorptionTime, acousticBrightness
  - All 5 material types exercised across preset library
  - Research-informed parameter values based on modal synthesis bell research

### New Preset Categories & Names

**Large Bells (5):** Deep, long-sustaining tones
- Deep Bronze Tower, Massive Iron Bell, Cavernous Brass, Grand Cathedral Bell, Slow Tolling Bell

**Bright Bells (5):** Clear, articulate tones
- Bright Clear Crotale, Crystalline Steel Chime, Sparkling Aluminum, Brilliant Bronze Plate, Crisp Steel Bar

**Warm Bells (5):** Mellow, soft attack
- Soft Mallet Bronze, Mellow Brass Bowl, Warm Aluminum Bars, Gentle Hand Bell, Velvet Bronze Tone

**Metallic (5):** Complex spectra with inharmonicity
- Dense Bronze Gamelan, Clanging Steel Plate, Beating Bronze Gong, Shimmering Bell Tree, Dark Iron Resonance

**Ambient (5):** Atmospheric, evolving textures
- Distant Cathedral, Underwater Bell, Evolving Bronze Wash, Frozen Steel Shimmer, Ethereal Chime Pad

### Research Sources
Based on `research/modal-synthesis-bells-academic-research.md`:
- Church bell partial ratios (Hum:Prime:Tierce:Quint:Nominal)
- Frequency-dependent damping: R_k = b_1 + b_3 * f_k^2
- Multi-stage envelope structures (strike → body → hum tail)
- Fletcher & Rossing bell physics data
- Risset bell inharmonicity ratios

### Technical Notes
- Domain: Preset data only (no DSP changes)
- Previous presets must be deleted manually: `rm -rf ~/Library/O-Bells/Presets/Factory/`
- New presets created on next plugin load after clearing old presets
- Preset compatibility: BREAKING - old preset files incompatible with new parameter format

## [2.2.0] - 2026-02-03

### Added
- **GUI Keyboard in footer** - 2-octave interactive keyboard (C3-B4) for auditioning sounds
  - Click or touch keys to play notes
  - QWERTY keyboard support (Z-M for C3-B3, Q-P for C4-B4)
  - Visual feedback on key press
  - Sends MIDI to synth engine via `sendMidiNote` native function

### Changed
- **Footer panel expanded** - New layout: Gain Fader | GUI Keyboard | Branding
  - Gain slider moved from Output section to sticky footer for quick access
  - Footer height increased from 40px to 55px
  - Output section now contains only Reverb slider and stereo meters

### Technical Notes
- Domain: UI + C++ (index.html, PluginEditor.cpp, PluginProcessor.cpp/.h)
- New C++ methods: `triggerNoteOn(int, float)`, `triggerNoteOff(int)`
- New native function: `sendMidiNote(note, velocity, isNoteOn)`
- Keyboard uses `juce::Synthesiser::noteOn()`/`noteOff()` (thread-safe)
- Tab content height adjusted: `calc(100% - 145px)`
- **Integration approach:** Inline CSS/JS additions rather than using standalone module files (see NOTES.md for details)

### Compatibility
- Preset compatibility: Fully compatible (no parameter changes)
- DAW session compatibility: Fully compatible

## [2.1.0] - 2026-02-03

### Added
- **Air Absorption parameter** - Time-varying lowpass filter simulating realistic acoustic propagation
  - Simulates progressive high-frequency loss as bell sound decays (air absorption, distance effect)
  - 0%: No filtering (transparent, preserves existing sound)
  - 50%: Subtle darkening over decay
  - 100%: Progressive HF rolloff from 18kHz → 2kHz as bell decays
  - Default: 0% (non-breaking, preserves all existing presets)
  - Located in Synthesis section, next to Inharmonicity slider

### Technical Notes
- Domain: DSP + UI (BellVoice.cpp, PluginProcessor.cpp, PluginEditor.cpp, index.html)
- Total parameters: 32 (was 31)
- Implementation: One-pole lowpass filter per voice, cutoff modulated by decay progress
- Filter coefficient: `coeff = 1 - exp(-2π * cutoff / sampleRate)`
- Cutoff formula: `cutoff = 18000 - (18000 - minCutoff) * decayProgress` where `minCutoff = 18000 - 16000 * airAbsorption`
- CPU overhead: Negligible (~2 multiplies + 1 exp per sample, only when parameter > 0)
- Research basis: [Modal synthesis frequency-dependent damping](https://nathan.ho.name/posts/exploring-modal-synthesis/), [bell damping studies](https://www.acoustics.asn.au/conference_proceedings/ICSVS-1997/pdf/scan/sv970230.pdf)

### Compatibility
- Preset compatibility: Fully compatible (new param defaults to 0%, no audible change to existing presets)
- DAW session compatibility: Sessions saved with older versions will load with Air=0%

## [2.0.0] - 2026-02-03

### Breaking Changes
- **Parameter ID renamed:** `brightness` → `overtoneBrightness`
  - Existing presets will NOT load brightness values correctly
  - DAW automation lanes referencing "brightness" will break
  - Users must re-save presets after loading in v2.0.0

### Added
- **Acoustic Brightness parameter** - Controls frequency-dependent decay rate (new)
  - Simulates air absorption and natural bell physics where higher frequencies fade first
  - 0%: Higher partials decay 4× faster (very warm, dark sustain)
  - 50%: Moderate HF decay (natural bell character)
  - 100%: Normal decay rates (bright, synthetic)
  - Default: 70% (slightly natural)
  - Research: Based on [Stanford CCRMA frequency-dependent damping](https://ccrma.stanford.edu/~jos/pasp/Frequency_Dependent_Damping.html) and [bell damping studies](https://www.acoustics.asn.au/conference_proceedings/ICSVS-1997/pdf/scan/sv970230.pdf)

### Changed
- **Brightness parameter split into two controls:**
  - **Overtone Brightness** (renamed from "Brightness"): Controls initial partial amplitudes
    - Expanded range: [0.1×, 2.0×] for highest partial (was [1.0×, 2.0×])
    - 0%: Dark attack (upper partials attenuated)
    - 50%: Neutral
    - 100%: Bright attack (upper partials boosted)
  - **Acoustic Brightness** (new): Controls how fast upper partials decay over time

### Technical Notes
- Domain: DSP + UI (BellVoice.cpp, PluginProcessor.cpp, index.html)
- Total parameters: 31 (was 30)
- Acoustic brightness formula: `acousticDecayMult = 1.0 - (1.0 - acousticBrightness) * partialRatio * 0.75`
- Applied to both standard decay and multi-stage body phase decay
- All factory presets updated with acousticBrightness=70% default

### Migration Notes
To migrate existing presets:
1. Load preset in v2.0.0 (brightness value will be lost)
2. Manually set "Overtone" slider to desired initial brightness
3. Adjust "Acoustic" slider for decay character
4. Re-save preset

## [1.6.0] - 2026-02-03

### Changed
- **Complete factory preset redesign** - All 25 presets remade with research-informed parameter values
  - New preset names reflecting real instruments and sonic characteristics
  - Full utilization of bloom, shimmer, multi-stage decay, and ensemble features
  - Acoustically accurate parameters based on bell physics research

### Research Sources
Presets designed using acoustic research on real bells:
- Church bell partial ratios: Hum(0.25):Prime(0.5):Tierce(0.6):Quint(0.75):Nominal(1.0)
- Gamelan inharmonicity derived from bronze metallophone spectra (sléndro tuning)
- Singing bowl beating frequencies (~2-3Hz monaural beats from asymmetric modes)
- Tubular bell strike pitch phenomenon (4th/5th/6th partials in 2:3:4 ratio)
- Steel pan harmonic generation through nonlinear vibration
- Vibraphone modal frequency ratios (1:2.76:5.4 for bar instruments)

### New Preset List

**Orchestral (5):**
- Westminster Chimes - Steel tubular bells with characteristic twangy brightness
- Crystal Glockenspiel - Pure, high steel bars with suppressed overtones
- Jazz Vibes - Warm aluminum vibraphone with motor-like shimmer
- Antique Crotales - Sustaining bronze discs with harmonic purity
- Nutcracker Celesta - Gentle hammered steel with wooden warmth

**Sacred (5):**
- Flemish Carillon - True-harmonic bronze with minor third partial
- Russian Zvon - Cast iron with intentionally beating partials
- Himalayan Bowl - Bronze singing bowl with monaural beat texture
- Temple Tam-Tam - Large gong with complex inharmonic bloom
- Sanctus Handbell - Clear brass with prominent octave partial

**World (5):**
- Javanese Saron - Extreme inharmonicity bronze bar (sléndro-informed)
- Balinese Bonang - Knobbed gong essential to gamelan tuning
- Trinidad Tenor Pan - Steel pan with nonlinear harmonic generation
- West African Balafon - Warm woody tone with subtle buzzing texture
- Temple Woodblock - Dry percussive with emphasis on attack

**Ambient (5):**
- Spectral Bloom - Slowly evolving texture with maximum bloom
- Ice Crystals - High shimmering harmonics with crystalline character
- Subterranean Drone - Deep, dark texture with extreme sub presence
- Wind Chimes - Delicate, spacious aluminum chimes
- Sunken Cathedral - Filtered, underwater-like bell atmosphere

**Cinematic (5):**
- Trailer Impact - Massive hit bell for epic moments
- Dread Toll - Dissonant, tense bell for suspense/horror
- Ascension - Ethereal, angelic bell for emotional peaks
- Harbinger - Dark, ominous bell with deep sub weight
- Victory Peal - Bright, triumphant celebratory bell

### Technical Notes
- Domain: Preset data only (no DSP changes)
- Previous presets must be deleted manually: `~/Library/O-Bells/Presets/Factory/`
- New presets will be created on next plugin load after clearing old presets
- All parameters now properly utilized (bloom, shimmer, multi-stage decay, reverb, attack)

## [1.5.4] - 2026-02-03

### Changed
- **UI reorganization** - Improved parameter grouping for better workflow
  - Renamed "Character" section to "Onsets" - better describes the parameters in this group
  - Moved "Strike" and "Mallet" sliders from Synthesis section to Onsets section
  - Onsets section now contains all onset-related parameters: Strike, Mallet, Attack Amount, Noise, Velocity

### Technical Notes
- Domain: UI only (no DSP changes, no parameter ID changes)
- Preset compatibility: Fully compatible (parameter IDs unchanged)

## [1.5.3] - 2026-02-03

### Fixed
- **No sound when Strike at 0% or 100%** - Strike position now produces sound across the full range
  - Root cause: Comb filter formula created zero-nodes at position extremes (`sin(0)=0`, `sin(nπ)=0`)
  - Fix: Replaced comb filter with spectral tilt model for more musical and audible results

### Changed
- **Strike parameter redesigned** - Now models physical strike position with clear tonal difference
  - 0% (center): Warm, fundamental-heavy tone - like striking the bell's center
  - 100% (edge): Bright, partial-rich tone - like striking near the rim
  - Smooth spectral transition across the full range

### Technical Notes
- Domain: DSP (BellVoice.cpp:calculateStrikePositionGain)
- Preset compatibility: Existing presets may sound slightly different (spectral balance changed)

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
