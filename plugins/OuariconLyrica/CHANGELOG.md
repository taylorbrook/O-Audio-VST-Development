# OuariconLyrica Changelog

All notable changes to OuariconLyrica are documented in this file.

## [1.7.1] - 2026-01-19

### Changed

- **Keyboard visualization now compact and centered** (matching Ouaricon Marimba layout)
  - Fixed width: 280px × 70px (was full-width)
  - Centered at bottom of tuning tab
  - Black key positions now use CSS ID selectors for precise pixel placement

- **SCL/KBM file buttons now conditional**
  - Load .SCL, Load .KBM, Save .SCL, Save .KBM buttons hidden by default
  - Only visible when Custom tuning mode is selected
  - Matches Marimba behavior where file operations are context-dependent

### Technical Notes

- Files modified: Resources/ui/index.html
- Keyboard CSS refactored to use absolute positioning like Marimba
- Added `updateFileButtonsVisibility()` function to toggle scala-buttons div
- Black key left positions: 29px, 69px, 149px, 189px, 229px (40px per white key)

## [1.7.0] - 2026-01-18

### Changed

- **Tuning tab UI refactored to match Ouaricon Marimba layout**
  - Two-column + bottom layout replacing three-column design
  - Left side: Interval list with embedded tonic selector (◀ TONIC: C ▶)
  - Center: Large pitch circle visualization with radial interval lines
  - Right side: Mode buttons (12-TET, CUSTOM, MTS-ESP), A4 REF knob, scale name display
  - Bottom: Full-width keyboard visualization with "Click to play" label and note labels

- **Interval list redesigned**
  - Dynamic header showing note count: "Intervals (12 notes)"
  - Editable cent values in Custom mode (disabled in 12-TET mode)
  - Tonic selector embedded in list header for 12-tone scales
  - Compact layout with monospace font for values

- **A4 Reference Pitch changed from slider to rotary knob**
  - Draggable knob with 270-degree sweep
  - Visual indicator showing current position
  - Double-click to reset to 440.0 Hz (center)
  - Hz value displayed below knob

- **Pitch circle visualization upgraded**
  - Radial lines from center to each scale degree (Marimba style)
  - Note labels positioned around circumference
  - Adapts to any number of scale degrees (5-24+)
  - Green lines (#6B8E4E) matching Ouaricon aesthetic

- **MTS-ESP mode button added** (placeholder for future implementation)

### Removed

- Preset Scales dropdown (scales now loaded via .SCL files or 12-TET default)
- Tonic button grid (replaced with inline tonic selector in interval list)
- Pitch Bend Range from tuning tab (parameter remains functional)
- Three-column layout in favor of cleaner two-column + bottom design

### Technical Notes

- Files modified: Resources/ui/index.html (CSS + HTML + JavaScript restructure)
- masterTune parameter now controlled via custom knob code instead of slider binding
- Keyboard visualization now uses green scale degree bars on white keys
- CSS uses absolute positioning for tuning panel components (matching Marimba pattern)

## [1.6.0] - 2026-01-18

### Added

- **Full Tuning Module** with 12-TET and Custom (Scala) tuning support
  - New TUNING tab replaces placeholder with complete microtonal tuning interface
  - Two tuning modes: 12-TET (standard equal temperament) and Custom (Scala scales)
  - Interval list display showing all scale degrees in cents with ratio approximations
  - Pitch circle visualization showing note positions around the octave
  - Tonic/root selector (C through B) for scale transposition
  - Reference pitch (A4 frequency) slider moved from Techniques tab
  - Pitch bend range slider moved from Techniques tab

- **Scala File Support (.scl)**
  - Load any standard Scala scale file
  - Supports both cents and ratio notation (n/d, decimal, integer)
  - Scale name extracted from file
  - Save current scale as .scl file

- **Keyboard Mapping File Support (.kbm)**
  - Load keyboard mapping files to set reference frequency
  - Save current mapping as .kbm file

- **Preset Scales** (built-in)
  - 12-TET (Equal Temperament)
  - Just Intonation
  - Pythagorean
  - Quarter-comma Meantone
  - Arabic 17-TET
  - Slendro (5-TET)

- **Interactive Keyboard Visualization**
  - Visual piano keyboard in tuning tab
  - Click-to-highlight feedback (visual only)

- **Native Functions for WebView**
  - `getTuningIntervals()` - Get current scale intervals in cents
  - `setTuningIntervals(intervals, name)` - Set custom scale
  - `getTuningName()` - Get active tuning name
  - `setTonicNote(index)` / `getTonicNote()` - Tonic transposition
  - `loadScalaFile()` / `saveScalaFile()` - .scl file I/O
  - `loadKBMFile()` / `saveKBMFile()` - .kbm file I/O

### Changed

- **TuningEngine upgraded** with Mode enum, Scala parsing, interval storage
  - Thread-safe frequency table using atomic operations
  - Lock-free audio thread access to frequencies
  - Mutex-protected interval updates on message thread
- **Techniques tab simplified** - Tuning controls moved to dedicated Tuning tab
- **tuningMode parameter added** (0=12-TET, 1=Custom)

### Technical Notes

- Files modified: TuningEngine.h/.cpp, PluginProcessor.h/.cpp, PluginEditor.h/.cpp, index.html
- Frequency calculation: Custom intervals use cents-based formula: `f = c0 * 2^(cents/1200)`
- Scala parser handles cents (decimal), ratios (n/d), and integer ratios
- Tonic transposition shifts entire scale by specified semitones
- Preset scales defined in JavaScript with accurate historical intervals

## [1.5.4] - 2026-01-18

### Fixed

- **Preset UI buttons now fully functional** (prev/next arrows, dropdown, Save/Load)
  - Root cause: ComboBox (dropdown) binding code was throwing an uncaught error that crashed the entire JavaScript module before preset event listeners could be attached
  - The error occurred in `comboState.valueChangedEvent.addListener()` for the stringMaterial dropdown
  - Fix: Added try-catch around comboBox binding to prevent script crash, allowing preset system to initialize
  - Also changed preset native function calls to use inline get-and-call pattern for robustness
  - Files modified: Resources/ui/index.html (JavaScript)
  - See `.bugs/preset-ui-not-connected.md` for full investigation history

### Technical Notes

- ComboBox binding now wrapped in try-catch to prevent cascade failures
- Preset functions use inline pattern: `await Juce.getNativeFunction('name')()`
- This matches the working `getVoiceCount` pattern used elsewhere in the code

## [1.5.3] - 2026-01-18

### Changed

- **Refactored preset JavaScript to match Marimba pattern** (fix attempt #3)
  - Removed `await` from all `getNativeFunction()` calls
  - Changed `let` to `const` for function references
  - Simplified event listener attachment
  - Added `window.onPresetLoaded` callback for C++ integration

### Known Issues (Resolved in v1.5.4)

- **Preset UI buttons still not functional** - arrows, dropdown, Save/Load don't respond
  - See `.bugs/preset-ui-not-connected.md` for full investigation notes
  - Three fix attempts made, none successful
  - Next step: Add debug output to verify if native functions are being obtained

## [1.5.2] - 2026-01-18

### Fixed

- **CSS dropdown arrow now displays correctly** (was showing "u25BC" as text)
  - Root cause: CSS unicode escape was using JavaScript syntax `\u25BC` instead of CSS syntax `\25BC`
  - Fix: Changed `content: ' \u25BC'` to `content: ' \25BC'` in preset-name::after

### Changed

- **Refactored preset system JavaScript to avoid top-level await**
  - JUCE's WebKit WebView doesn't support top-level await in ES modules
  - Moved all `getNativeFunction()` calls inside async `initializePresetSystem()` function
  - Event listeners now check if native functions are ready before calling

## [1.5.1] - 2026-01-18

### Fixed

- **Preset module now fully functional**
  - Root cause: Preset bar was missing Save/Load buttons and corresponding native function handlers. The C++ side only registered basic preset functions (loadPreset, getPresetList, getCurrentPreset, selectNext/Previous) but lacked file dialog support for saving/loading preset files.
  - Fix: Added Save and Load buttons to preset bar HTML with matching CSS styling. Registered `savePresetWithDialog` and `loadPresetFromFile` native functions in PluginEditor.cpp using async FileChooser dialogs. Added JavaScript handlers for button click events.
  - Result: Users can now save custom presets to `~/Library/OuariconLyrica/Presets/User/` and load presets from any location using native file dialogs.

### Added

- **Save button** - Opens file dialog to save current settings as a user preset
- **Load button** - Opens file dialog to load any .json preset file
- `savePresetWithDialog` native function with async FileChooser
- `loadPresetFromFile` native function with async FileChooser

### Technical Notes

- Files modified: index.html (CSS + HTML + JavaScript), PluginEditor.cpp, PluginEditor.h
- FileChooser uses async launchAsync() pattern for non-blocking dialogs
- User presets stored in: `~/Library/OuariconLyrica/Presets/User/`
- Preset bar styling matches Ouaricon Naturalist aesthetic

## [1.5.0] - 2026-01-17

### Added

- **Preset Management System**
  - Integrated `OuariconPresetManager` module for save/load/navigate presets
  - Preset bar in header with prev/next navigation and dropdown browser
  - Botanical/Naturalist aesthetic styling matching UI design
  - Category-organized dropdown menu with string material groupings

- **48 Factory Presets** organized by string material (6 presets each):
  - **Gut**: Ancient Lyre, Fireside Tales, Medieval Court, Warm Classical, Bardic Song, Nostalgic Whisper
  - **Nylon**: Celtic Dawn, Folk Ballad, Gentle Stream, Morning Dew, Pastoral Scene, Harmonic Dreams
  - **Wire**: Bright Cascade, Articulate Pluck, Concert Grand, Modern Classic, Silver Strings, Pedal Technique
  - **Carbon**: Crystal Clear, Precision Touch, Extended Range, Studio Session, Clean Articulation, Harmonic Purity
  - **Metal Alloy**: Brilliant Sustain, Bell Tones, Orchestral Ring, Shimmering Heights, Warm Metallic, Ethereal Chime
  - **Glass**: Crystalline Voice, Fragile Beauty, Ice Palace, Winter Bells, Delicate Touch, Harmonic Prism
  - **Crystal**: Pure Resonance, Mystical Glow, Sacred Space, Singing Bowls, Meditation, Angelic Choir
  - **Energy**: Quantum Strings, Plasma Resonance, Electric Dreams, Cosmic Harp, Neon Glow, Future Primitive

- **Native Functions for WebView Integration**
  - `savePreset(name)`, `loadPreset(name)`, `getPresetList()`
  - `getCurrentPreset()`, `selectNextPreset()`, `selectPreviousPreset()`
  - `isFactoryPreset(name)` for identifying read-only factory presets

### Changed

- **State serialization** now uses preset manager for consistent preset name preservation
- **Header layout** updated to accommodate preset bar between title and voice counter
- **Preset storage location**: `~/Library/OuariconLyrica/Presets/Factory/` and `.../User/`

### Technical Notes

- Factory presets auto-initialize on first plugin load
- Presets stored as JSON with full parameter state
- Preset bar persists across all tabs (header-level component)
- Dropdown menu organized by string material categories
- CMakeLists.txt updated to include modules/persistence/preset-manager/cpp

## [1.4.0] - 2026-01-17

### Added

- **Ouaricon Naturalist aesthetic UI redesign**
  - New WebView interface with warm earth-tone paper texture background
  - Coral sea-fan botanical overlay (fern_naturalistsmisc1Geor_0089.png) that shifts right as tabs change
  - Garamond serif typography with wide letter-spacing for classical elegance
  - Custom slider styling with cream gradient thumbs and inset paper tracks
  - Styled dropdown menus for choice parameters

- **4-tab interface structure**
  - **SOUND tab**: All 20 sound parameters organized into 5 logical sections (Main, String, Body, Excitation, Sympathetic)
  - **TECHNIQUES tab**: Master Tune, Pitch Bend Range, Glissando Mode/Scale
  - **TUNING tab**: Placeholder for future microtonal tuning system
  - **EFFECTS tab**: Placeholder for future EQ and compressor

- **Connected 4 missing WebView relays** from v1.3.0 parameters
  - attackNoise, sympatheticQ, bodyModeSpread, bridgeBrightness now controllable via UI

### Changed

- **Window size reduced** from 800×600 to 700×450 for more compact layout
- **UI organization** with grouped sections and clear section headers
- **All parameters accessible** via sliders (float params) and dropdowns (choice params)
- **Master Volume** moved to footer with "Ouaricon Audio" branding

### Technical Notes

- Inline CSS and JavaScript for simpler asset management
- Tab-aware botanical overlay animation (shifts right -30px to -180px per tab)
- Double-click to reset sliders to default values
- Voice count polling at 100ms intervals
- Paper texture (paper1.jpg) and botanical image embedded as BinaryData

## [1.3.2] - 2026-01-17

### Changed (Code Quality)

- **Removed redundant null checks in HarpSynthVoice.cpp**
  - Root cause: Every APVTS parameter access checked for null, but APVTS guarantees non-null pointers for registered parameters
  - Fix: Removed per-parameter null checks while keeping the top-level `parameters != nullptr` guard
  - Result: Cleaner code, reduced verbosity (~40 lines removed from startNote/updateParametersFromAPVTS)
  - Files modified: HarpSynthVoice.cpp

- **Replaced dynamic_cast with static_cast in PluginProcessor.cpp**
  - Root cause: Voice loop used dynamic_cast when type is known at compile time (we control voice creation)
  - Fix: Use static_cast in prepareToPlay() since all voices are HarpSynthVoice
  - Result: Slightly more efficient, expresses intent better
  - Files modified: PluginProcessor.cpp

- **Added named constants in SympatheticResonance.cpp**
  - Root cause: Hardcoded magic numbers (0.05f, 0.1f, 0.995f, etc.) scattered throughout DSP code
  - Fix: Added constexpr constants in anonymous namespace:
    - `ENERGY_DECAY_BASE` (0.995f), `ENERGY_DECAY_MODIFIER` (0.0048f)
    - `COUPLING_SCALE_FACTOR` (0.05f), `INTENSITY_CHANGE_THRESHOLD` (0.01f), `Q_CHANGE_THRESHOLD` (0.05f)
    - `SOFT_CLIP_THRESHOLD` (0.1f), `SOFT_CLIP_HEADROOM` (0.05f)
    - `UNISON_COUPLING` (0.9f), `OCTAVE_COUPLING` (0.7f), `FIFTH_COUPLING` (0.5f), `THIRD_COUPLING` (0.3f)
  - Result: Self-documenting code, easier to tune DSP behavior
  - Files modified: SympatheticResonance.cpp

- **Fixed audio thread allocation in GlissandoController**
  - Root cause: `setScale()` copied std::vector during startNote(), causing allocation on audio thread
  - Fix: Replaced `std::vector<double> scale` with `std::array<double, MAX_SCALE_SIZE>` (48 elements)
  - Result: No dynamic allocation on audio thread in scale-locked glissando mode
  - Files modified: GlissandoController.h, GlissandoController.cpp

### Technical Notes

- Pure code quality release - no functional changes
- All changes are refactoring/cleanup identified by code review
- Build validates clean with Release configuration

## [1.3.1] - 2026-01-17

### Changed (Code Simplification)

- **Extracted enum conversion helpers** - Replaced 3 duplicate switch statements with inline helper functions
  - `woodTypeFromIndex()` in BodyResonance.h
  - `techniqueFromIndex()` in PluckExciter.h
  - `glissandoModeFromIndex()` in GlissandoController.h
  - Reduces HarpSynthVoice.cpp by ~40 lines while improving maintainability

- **Centralized filter cutoff calculations** in WaveguideString.cpp
  - New `FilterCutoffs` struct and `calculateFilterCutoffs()` method
  - Eliminates duplicate calculation in `updateFilters()` and `calculateFilterGroupDelay()`
  - Ensures filter cutoffs are always computed consistently

- **Added named constants** in BodyResonance.cpp
  - `MAX_DRY_REDUCTION` (0.6f) - Maximum dry signal reduction at full body resonance
  - `WET_GAIN_MULTIPLIER` (0.7f) - Wet signal gain (v1.1.5 value)
  - Replaces magic numbers with self-documenting constants

### Removed

- **HarpSynthSound.cpp** - Empty file (implementation was header-only)
- **BridgeFilter.h/.cpp** - Unused (WaveguideString uses juce::dsp::IIR::Filter)
- **DelayLine.h/.cpp** - Unused (WaveguideString uses juce::dsp::DelayLine)
- **StringVoice.h/.cpp** - Legacy Phase 2.1 implementation replaced by WaveguideString in Phase 2.2

### Technical Notes

- Pure refactoring release - no functional changes
- Removed 7 unused files, reducing codebase size
- CMakeLists.txt updated to reflect file removals
- Build validates clean with Release configuration

## [1.3.0] - 2026-01-17

### Added

- **Attack Noise Amount** - Independent control of pluck transient noise (0-100%)
  - Overrides material default noise content for user control
  - 0% = clean attack, 100% = scratchy/noisy attack
  - Files modified: WaveguideString.h/.cpp (setAttackNoise passthrough), HarpSynthVoice.cpp, PluginProcessor.cpp

- **Sympathetic Sharpness (Q)** - Controls resonator filter Q for sympathetic resonance
  - Range: 0.1 (broad/diffuse) to 20.0 (sharp/ringing)
  - Default: 5.0 (moderate sharpness)
  - Higher Q = more defined resonant peaks, more "shimmer"
  - Lower Q = broader resonance, more diffuse coupling
  - Files modified: SympatheticResonance.h/.cpp (setResonatorQ), PluginProcessor.cpp

- **Body Mode Spread** - Controls detuning/spread of body resonance modal frequencies
  - Range: -100% to +100% (centered at 0%)
  - 0% = original uniform scaling (modes at harmonic ratios)
  - Positive = modes spread apart (wider harmonic series)
  - Negative = modes compress together (tighter harmonic series)
  - Mode 2 (600Hz base) is the pivot point; modes 0,1 shift down and 3,4 shift up
  - Files modified: BodyResonance.h/.cpp (setModeSpread, scaleFrequency), HarpSynthVoice.cpp, PluginProcessor.cpp

- **Bridge Brightness** - Direct control of bridge filter cutoff for waveguide reflection
  - Range: 0-100% (0% = very dark/damped, 50% = neutral, 100% = very bright)
  - Provides more direct waveguide control than the general brightness parameter
  - Affects the first-order lowpass filter at the bridge reflection point
  - Pitch-compensated via calculateFilterGroupDelay()
  - Files modified: WaveguideString.h/.cpp (setBridgeBrightness), HarpSynthVoice.cpp, PluginProcessor.cpp

### Technical Notes

- All 4 new parameters support real-time modulation via updateParametersFromAPVTS()
- Attack Noise uses existing PluckExciter.setNoiseAmount() with passthrough from WaveguideString
- Sympathetic Q updates all existing resonator filters when changed (setResonatorQ)
- Body Mode Spread formula: `spreadMultiplier = 1.0 + (spread * modeOffset * 0.15)` where modeOffset is -2 to +2
- Bridge Brightness formula: `modifier = 0.3 + bridgeBrightness * 1.7` (0.3x to 2.0x range)
- UI additions: 4 new sliders in appropriate sections (Pluck, Sympathetic, Body, Advanced)

## [1.2.0] - 2026-01-17

### Added

- **Advanced string parameters now affect sound (tension, gauge, length)**
  - Root cause: Parameters were defined in PluginProcessor but never connected to DSP. The UI sliders existed but did nothing - only `stringStiffness` was wired up.
  - Fix: Added setter methods to WaveguideString (`setTension`, `setGauge`, `setLength`) and connected them in HarpSynthVoice.

### Changed

- **String Tension** (0-100%)
  - Controls string brightness and resonance via bridge/nut filter cutoff frequencies
  - Low tension (0%): Dark, muted, loose sound - filter cutoffs reduced by 50%
  - High tension (100%): Bright, resonant, tight sound - filter cutoffs increased by 2x
  - Pitch remains stable (filter group delay compensation updated)

- **String Gauge** (0-100%)
  - Controls damping characteristics simulating string mass/thickness
  - Low gauge (0%): Thin string, bright, quick attack and decay
  - High gauge (100%): Thick string, dark, heavier tone with more damping
  - Affects loop damping filter (200Hz - 14kHz range, expanded from 500Hz - 10.5kHz)

- **String Length** (0-100%)
  - Controls decay envelope character without changing pitch
  - Short (0%): Punchy, quick decay (70% of base decay time)
  - Long (100%): Sustained, diffuse decay (160% of base decay time)
  - Affects feedback coefficient calculation

### Technical Notes

- Files modified: WaveguideString.h, WaveguideString.cpp, HarpSynthVoice.cpp
- Tension modifier formula: `0.5 + tension * 1.5` (0.5x to 2.0x brightness)
- Gauge modifier formula: `0.5 + gauge * 1.5` (0.5x to 2.0x damping)
- Length modifier formula: `0.7 + length * 0.9` (0.7x to 1.6x decay time)
- `calculateFilterGroupDelay()` updated to include tension/gauge for pitch stability
- Real-time modulation supported via `updateParametersFromAPVTS()`

## [1.1.5] - 2026-01-17

### Changed

- **Increased body resonance intensity for more pronounced effect**
  - Wet mix increased from 0.3 to 0.7 (more than doubled)
  - Gain values significantly increased for each wood type:
    - Spruce: 1.8x → 3.5x (~11 dB boost)
    - Maple: 1.5x → 2.8x (~9 dB boost)
    - Exotic: 2.2x → 4.5x (~13 dB boost)
    - Synthetic: 2.5x → 5.5x (~15 dB boost)
  - Dry signal now preserved at 40% even at max resonance for blend
  - Files modified: BodyResonance.cpp

## [1.1.4] - 2026-01-17

### Fixed

- **Body parameters (size, resonance, wood type) now have audible effect**
  - Root cause: `BodyResonance::updateFilterCoefficients()` created peak filters with unity gain (1.0), which meant the resonant frequencies were not actually boosted. The body size affected filter frequencies and wood type affected Q values, but without gain boosting these differences were inaudible.
  - Fix: Added `getGainForWoodType()` method that returns appropriate linear gain values based on wood type. Peak filters now boost resonant frequencies by 3.5-8 dB depending on wood type.
  - Wood type gain values:
    - Spruce: 1.8x (~5.1 dB) - Traditional, balanced resonance
    - Maple: 1.5x (~3.5 dB) - Warmer, more subtle body
    - Exotic: 2.2x (~6.8 dB) - Pronounced, rich resonance
    - Synthetic: 2.5x (~8.0 dB) - Sharp, defined peaks
  - Result: Body size now audibly shifts resonant frequencies (small=bright, large=deep), wood type creates distinct tonal characters, and body resonance controls wet/dry blend.
  - Files modified: BodyResonance.h, BodyResonance.cpp

### Technical Notes

- JUCE's `makePeakFilter()` gain parameter is linear (not dB): 1.0 = unity, 2.0 = +6dB
- Body resonance uses 5-mode modal synthesis at 300, 400, 600, 900, 1200 Hz (scaled by body size)
- Q factor still varies by wood type (2.5-5.0) controlling resonance sharpness
- Mode amplitudes also vary by wood type for additional timbral shaping

## [1.1.3] - 2026-01-17

### Fixed

- **Pitch now stable across all string materials**
  - Root cause: `calculateFilterGroupDelay()` used a hardcoded `stiffnessDelay = 0.5f` constant, but the stiffness filter is a 4-stage allpass cascade whose group delay varies dramatically with material stiffness. Materials range from 0.05 (Gut/Wire) to 0.70 (Crystal), resulting in ~14x different phase delays. This caused pitch to drift differently per material even after the v1.1.2 fix.
  - Fix: Replaced fixed constant with dynamic calculation that replicates StiffnessFilter's coefficient computation (frequency scaling + per-stage progressive scaling) and sums the group delay from all 4 allpass stages using the formula `(1 - a) / (1 + a)` samples per stage.
  - Result: All 8 material types now produce identical fundamental frequency. Stiffness affects only inharmonicity (harmonic stretch), not fundamental pitch.
  - Files modified: WaveguideString.cpp

### Technical Notes

- Allpass group delay at DC: `τ = (1 - a) / (1 + a)` samples where `a` is the coefficient
- StiffnessFilter uses 4 stages with coefficients: `stiffness * freqScaling * stageScaling * 0.8`
- Crystal (0.70) now correctly compensates ~3.2 samples vs Gut (0.05) ~0.2 samples
- This completes the filter group delay compensation system: brightness (v1.1.1), material cutoffs (v1.1.2), and stiffness allpass (v1.1.3)

## [1.1.2] - 2026-01-17

### Fixed

- **String materials no longer affect fundamental pitch**
  - Root cause: `setMaterial()` was missing delay line compensation that was added in v1.1.1 for brightness. Different materials have vastly different `brightnessCutoff` values (Gut=2000Hz, Crystal=16000Hz) and `dampingCoeff` values, which feed into `calculateFilterGroupDelay()`. Changing materials altered the filter group delay by up to ~3 samples without compensating the delay line length, causing pitch drift.
  - Fix: Added delay line recalculation to `setMaterial()` using the same pattern as `setBrightness()`. Now when material changes, the delay line length is recomputed to maintain correct pitch.
  - Result: All 8 material types now produce the same fundamental frequency while retaining their distinct timbral characteristics (damping, brightness, stiffness, noise content).
  - Files modified: WaveguideString.cpp

### Technical Notes

- This completes the filter group delay compensation system started in v1.1.1
- Both `setBrightness()` and `setMaterial()` now recalculate delay lines when their parameters change
- Materials affect timbre via: brightnessCutoff (filter color), dampingCoeff (decay rate), stiffnessAmount (inharmonicity), noiseContent (attack character)

## [1.1.1] - 2026-01-17

### Fixed

- **Brightness slider no longer affects pitch**
  - Root cause: `calculateRailDelay()` used a fixed group delay compensation constant (6.0f samples), but the actual filter group delay varies dynamically with brightness settings. Lower brightness = lower filter cutoffs = higher group delay = lower pitch (up to 1 semitone flat at brightness=0).
  - Fix: Added `calculateFilterGroupDelay()` method that computes the actual group delay from all filters (bridgeFilter, nutFilter, loopDamping) based on their current cutoff frequencies. The delay line length is now recalculated whenever brightness changes.
  - Files modified: WaveguideString.h, WaveguideString.cpp

### Technical Notes

- First-order lowpass group delay at DC: `delay_samples = sampleRate / (2 * pi * cutoffHz)`
- Bridge/nut/damping filters now contribute dynamic compensation instead of fixed 6.0f
- `setBrightness()` now updates delay line lengths in addition to filter coefficients

## [1.1.0] - 2026-01-17

### Added

- **New "Decay Time" parameter for true sustain duration control**
  - Range: 0.1s to 20s with skewed control for finer adjustment at lower values
  - Implementation: Feedback coefficient multiplier applied per waveguide cycle
  - Formula: `coefficient = 10^(-3 / (decayTime * frequency))` for -60dB decay
  - This provides uniform energy loss independent of frequency content

### Changed

- **Renamed "Sustain" parameter to "Timbre"**
  - Root cause: The original "Sustain" parameter actually controlled tonal damping (lowpass filter cutoff in the feedback loop), not decay duration. Users perceived it as affecting attack brightness rather than sustain length.
  - The parameter now more accurately reflects its function: controlling the brightness/warmth of the string tone
  - Timbre=0.0 produces darker, warmer tones; Timbre=1.0 produces brighter tones
  - Internal behavior unchanged (controls `loopDamping` filter cutoff 500Hz-10.5kHz)

### Technical Notes

- Files modified: PluginProcessor.cpp, WaveguideString.h/.cpp, HarpSynthVoice.cpp, index.html, app.js
- Feedback coefficient recalculated on note trigger and frequency change (pitch bend)
- Breaking change: "sustain" parameter ID renamed to "timbre" - existing presets/automation will need adjustment

## [1.0.4] - 2026-01-17

### Fixed

- **Master Volume fader now controls output level**
  - Root cause: `masterVolume` parameter was connected to UI but never applied in `processBlock()`
  - Fix: Added gain stage after synthesizer rendering that converts dB parameter to linear gain

- **Sustain slider now affects decay time**
  - Root cause: `setMaterial()` unconditionally overwrote `dampingAmount` with the material's `dampingCoeff`, discarding the user's sustain slider value (same bug pattern as v1.0.3 stiffness fix)
  - Fix: Added `materialDamping` and `userDampingModifier` member variables with `calculateFinalDamping()` function that combines them using a 0.5x-1.5x modifier range
  - Result: Sustain=1.0 gives 0.5x material damping (longer decay), sustain=0.0 gives 1.5x material damping (shorter decay), while preserving material-specific characteristics

## [1.0.3] - 2026-01-16

### Fixed

- **String materials now produce audibly different timbres**
  - Root cause: `WaveguideString::setStiffness()` was completely overwriting the material's stiffness value with the user's slider value, making all materials sound identical in terms of inharmonicity
  - Each material defines a unique stiffness (Gut=0.10, Crystal=0.50) that creates its characteristic harmonic structure, but this was being discarded
  - Fix: User's stiffness slider now acts as a modifier (0.5x to 1.5x) rather than an overwrite, preserving material-specific inharmonicity while still allowing user adjustment
  - Result: Gut strings now sound warm/mellow, Crystal strings sound bright/bell-like, with clear audible distinction between all 8 material types

## [1.0.2] - 2026-01-16

### Fixed

- **Tuning: Pitches were ~1 semitone flat**
  - Root cause: `WaveguideString::calculateRailDelay()` did not compensate for the group delay introduced by feedback filters (bridgeFilter, nutFilter, loopDamping, stiffnessFilter)
  - The combined filter group delay (~6 samples) effectively lengthened the delay line, lowering pitch by approximately one semitone
  - Fix: Added 6-sample group delay compensation to the delay calculation

## [1.0.1] - 2026-01-16

### Fixed

- Enable real-time parameter modulation during note playback

## [1.0.0] - 2026-01-16

### Added

- Initial release
- Physical modeling harp synthesizer with bidirectional waveguide string model
- String materials: Nylon, Gut, Wire, Carbon
- Playing techniques: Normal, Harmonic, Muted, Pres de la Table
- Body resonance with wood type selection (Spruce, Maple, Exotic, Synthetic)
- Sympathetic resonance engine
- Tuning engine with master tune and pitch bend support
- Glissando controller with free and scale-locked modes
- WebView-based GUI
