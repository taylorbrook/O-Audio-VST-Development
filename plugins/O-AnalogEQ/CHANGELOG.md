# O-AnalogEQ Changelog

## [1.1.0] - 2026-01-24

### Changed
- **Renamed plugin** - Changed from "OuariconAnalogEQ" to "O-AnalogEQ"
  - Directory: `plugins/OuariconAnalogEQ/` → `plugins/O-AnalogEQ/`
  - DAW display name: "Ouaricon Analog EQ" → "O-AnalogEQ"
  - Binary names: Now `O-AnalogEQ.vst3` and `O-AnalogEQ.component`
  - Consistent with O-Tremolo and O-DigiDelay naming convention
- Internal CMake target remains `OuariconAnalogEQ` for preset/session compatibility

## [1.0.10] - 2026-01-11

### Changed
- **VU meter moved left** - Shifted 40px left (now at left: 758px)
- **Analog saturation retuned** - Changed from `tanh(x * 1.5) * 1.1` to `tanh(x * 0.5) * 2.0`
  - Now gain-neutral (no volume boost)
  - Adds subtle harmonic warmth/coloration without level change
  - Lower drive preserves dynamics while adding character

## [1.0.9] - 2026-01-11

### Changed
- **VU meter reduced to 80%** - Now 112x112px (was 140x140px) for better proportions
- **VU meter repositioned** - Adjusted position to fit new size
- **Analog button moved right** - Shifted 20px right (left: 620px) for better spacing

## [1.0.8] - 2026-01-11

### Changed
- **VU meter doubled in size** - Now 140x140px (was 70x70px) for better visibility
- **VU meter shifted right** - Positioned at far right edge of window
- **Analog button repositioned** - Now centered between HF shelf dial and VU meter
- **VU meter scale updated** - Larger text and arc for readability at new size

## [1.0.7] - 2026-01-11

### Changed
- **Title on single line** - Widened title container to prevent line break
- **Removed SHELF sublabels** - Cleaned up redundant labels below LF and HF dials
- **Q toggles moved down** - WIDE/MED/TIGHT buttons lowered by 10px for better spacing
- **Flower centered vertically** - Botanical overlay now vertically centered in window
- **Renamed band labels** - LF → "LF SHELF", HF → "HF SHELF" for clarity

## [1.0.6] - 2026-01-11

### Changed
- **Widened Q toggles** - WIDE/MED/TIGHT buttons increased to 110px (fully visible text)
- **Centered band labels** - LF/LMF/HMF/HF toggle buttons now centered above their dials
- **Resized botanical overlay** - Flower reduced to 75% and repositioned to end at far right
- **Updated title** - Changed from "OUARICON ANALOG EQ" to "OUARICON ANALOG EQUALIZER"

## [1.0.5] - 2026-01-11

### Changed
- **Removed output gain dial** - Simplified UI by removing the output gain control
- **Analog button moved under VU meter** - Better visual grouping of output section
- **Band labels are now toggles** - LF/LMF/HMF/HF labels function as on/off buttons
  - Green = band active, brown = band bypassed
  - Removed separate on/off buttons below each dial
- **Improved layout spacing** - Four EQ bands now evenly distributed with VU meter on right

## [1.0.4] - 2026-01-11

### Changed
- **Centered knob layout** - All controls now properly centered in the UI
- **Widened Q toggle buttons** - WIDE/MED/TIGHT labels no longer truncated (95px width)
- **Added OUTPUT/GAIN labels** - Output gain knob now has proper labeling
- **Vertical default position** - All knobs initialize at 12 o'clock (center) position
- **Double-click reset** - Double-clicking any knob returns it to default position
- **Green gradient on outer rings** - Outer frequency rings now have botanical green gradient
- **Added frequency notches** - SVG tick marks around dual-layer knobs show frequency position

## [1.0.3] - 2026-01-11

### Fixed
- **Dual-layer knobs now functional** - Implemented distance-based hit detection
  - Outer ring (>60% from center) controls frequency
  - Inner dial (<60% from center) controls gain
  - Added outer ring indicator for visual feedback
  - Added value tooltips showing both freq and gain values
- **VU meter now responds to actual audio levels** - Marimba-style implementation
  - C++ PluginProcessor calculates peak output level in processBlock
  - PluginEditor uses Timer to emit `outputLevel` events to WebView at 30Hz
  - JavaScript animates needle with ballistic motion (fast attack, slow decay)
  - Needle color interpolates from green (quiet) to red (loud)

## [1.0.2] - 2026-01-11

### Fixed
- **Missing check_native_interop.js** - Added the required JUCE WebView JavaScript bridge file
  - Root cause: JavaScript module `index.js` imports `check_native_interop.js` which sets up `window.__JUCE__.backend`
  - Without this file, no C++ ↔ JavaScript communication was possible
  - Added file to CMakeLists.txt resources and PluginEditor.cpp resource provider

## [1.0.1] - 2026-01-11

### Fixed
- **GUI controls now interactable** - Fixed type mismatch between C++ WebView relays and JavaScript state accessors
  - Root cause: Q parameters (`lmf_q`, `hmf_q`) were using `WebSliderRelay` but JavaScript expected `WebComboBoxRelay`
  - Changed `lmfQRelay` and `hmfQRelay` from `WebSliderRelay` to `WebComboBoxRelay`
  - Changed `lmfQAttachment` and `hmfQAttachment` from `WebSliderParameterAttachment` to `WebComboBoxParameterAttachment`

## [1.0.0] - 2026-01-11

### Added
- Initial release
- 4-band analog-style EQ (LF shelf, LMF bell, HMF bell, HF shelf)
- Per-band frequency, gain, and Q controls (Q on bell bands only)
- Per-band bypass toggles
- Global output gain control
- Analog warmth/saturation toggle
- VU meter display
- WebView UI with botanical paper aesthetic
