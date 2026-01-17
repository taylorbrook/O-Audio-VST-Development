# Changelog

All notable changes to Ouaricon Polystutter will be documented in this file.

## [1.1.6] - 2026-01-16

### Fixed

- **Distortion/artifacts when ENV (envelope follower) trigger mode is enabled**
  - Root cause 1: No crossfade on retrigger during active playback
    - When retriggering mid-playback, `playbackPosition` reset to 0 without any envelope
    - The v1.1.1 fade-in logic was skipped when `isTriggered` was already true
    - Result: Abrupt audio discontinuity at the trigger point = clicks/distortion
    - Fix: Save old playback state and crossfade blend old→new audio over 5ms
  - Root cause 2: No minimum time between envelope triggers
    - Rapid transients could fire triggers every few milliseconds
    - This caused constant retriggering = glitchy/distorted sound
    - Fix: Added 50ms cooldown timer between envelope triggers
  - Testing: Enable ENV mode with transient-heavy material (drums, percussion)

## [1.1.5] - 2026-01-16

### Fixed

- **Audio artifacts when ENV (envelope follower) triggers on transients**
  - Root cause: Capture timing mismatch in envelope-triggered mode
  - When `trigger()` was called before `processBlock()`, `captureStartPosition` was calculated
    based on the OLD capture buffer contents (from previous blocks)
  - The current block containing the transient hadn't been written yet
  - Result: Stutter played back OLD audio that didn't include the triggering transient
  - Fix: Implemented deferred capture using `pendingCapture` flag
    - `trigger()` now sets `pendingCapture = true` instead of calculating position immediately
    - `processBlock()` calculates `captureStartPosition` AFTER writing current block to buffer
    - This ensures the capture window includes the transient that triggered the effect
  - Testing: Play audio with sharp transients (drums, plucks) with ENV mode enabled

## [1.1.4] - 2026-01-16

### Changed

- **Bug image fragmented into 12 scattered pieces across UI**
  - Original single botanical overlay replaced with 12 randomly-shaped fragments
  - Each fragment uses CSS clip-path with irregular polygon shapes
  - Fragments positioned throughout the interface for artistic scattered effect
  - Slightly reduced opacity (0.25 vs 0.32) for subtler background presence

- **DRY/WET mix knobs repositioned to prevent cut-off**
  - Footer section moved up 15px (765px → 750px) and expanded (50px → 80px height)
  - Mix knobs now fully visible within plugin frame
  - Footer toggles vertically centered in taller footer section

### Added

- **Sequence grid greyed out when SEQ mode inactive**
  - Pattern sequencer section now dims (40% opacity, 30% grayscale) when SEQ toggle is off
  - Visual feedback matches lane disable behavior for consistency
  - Helps users understand when pattern sequencing is bypassed

## [1.1.3] - 2026-01-16

### Changed

- **SUBDIV dial text doubled in size** (8px → 16px) for better readability of subdivision values
- **Knob readout values increased by 2pt** (8px → 10px) across all lane controls
- **Wet/Dry knobs enlarged to match lane knobs** (32px → 42px) for visual consistency

### Removed

- **Parameter monitor debug panel** - Development diagnostic removed from production UI

## [1.1.2] - 2026-01-16

### Fixed

- **Lane control labels no longer overlap with dial value displays**
  - Root cause: Row spacing was 60px but each knob container required ~70px (label + knob + value)
  - This caused knob value text (e.g., "90%", "100%") to overlap with the next row's label
  - Fix: Increased row spacing from 60px to 75px (rows at top: 55px, 130px, 205px, 280px)
  - Expanded horizontal column spread to match progress bar width (columns at left: 20px, 95px, 170px)
  - Increased lanes-section height from 320px to 400px (25% larger)
  - Adjusted downstream sections and expanded plugin frame from 750px to 830px
  - All 4 lanes updated with consistent improved spacing

## [1.1.1] - 2026-01-16

### Fixed

- **Audio clicks at stutter start/end transitions eliminated**
  - Root cause: Missing global envelope for stutter effect boundaries
  - The existing crossfade logic (5ms) only smoothed transitions *within* repeat segments
  - No fade-in was applied when stutter first triggered (abrupt onset)
  - No fade-out was applied when all repeats finished (`buffer.clear()` caused abrupt cut)
  - Fix: Added global attack-release envelope to RepeatLane class
    - `globalEnvelopeGain` tracks envelope level (0.0 to 1.0)
    - `fadeInSamplesRemaining` handles fade-in over `crossfadeSamples` (5ms)
    - `fadeOutActive` / `fadeOutSamplesRemaining` handle smooth fade-out when repeats finish
  - Envelope resets on trigger, cancels fade-out if retriggered during release
  - Testing: Listen for click-free transitions on any audio material

## [1.1.0] - 2026-01-16

### Added

- **Wet/Dry mix controls** - Separate faders for dry signal (0-100%) and wet effect (0-100%)
- **Dedicated capture buffer** - Non-destructive repeat playback architecture

### Fixed

- **Pitch shifting and audio corruption** - Switched from popSample() delay line to dedicated capture buffer

## [1.0.2] - 2026-01-16

### Fixed

- **Lane buttons can now be re-enabled after being disabled**
  - Root cause: CSS `pointer-events: none` on `.lane-container.lane-disabled` blocked the lane header button
  - Fix: Added `pointer-events: auto` to `.lane-header` within disabled lanes to keep header clickable
  - This also fixes subdivision combo box being non-interactable when lane is disabled

### Added

- **Sequencer enable toggle (SEQ button in footer)**
  - New parameter: `sequencer_enabled` (default: ON)
  - When OFF, pattern sequencer is bypassed (all steps treated as enabled)
  - Allows users to easily toggle between sequenced and continuous stutter modes

## [1.0.1] - 2026-01-16

### Fixed

- **UI elements now respond to user interaction** (critical bug fix)
  - Root cause: JavaScript parameter-bindings.js was using incorrect JUCE WebView API
  - Sliders used `state.normalisedValue` (nonexistent property) instead of `state.getNormalisedValue()` / `state.setNormalisedValue()`
  - Toggles used direct `state.value = x` assignment instead of `state.setValue(x)` method call
  - ComboBoxes used `state.selectedId` (nonexistent property) instead of `state.getChoiceIndex()` / `state.setChoiceIndex()`
  - Direct property assignment doesn't emit events to JUCE backend; method calls are required

## [1.0.0] - 2026-01-15

### Added

- Initial release
- 4 independent repeat lanes with per-lane parameters
- 16-step pattern sequencer per lane (64 steps total)
- 5 trigger modes: beat sync, envelope follower, sidechain, manual, MIDI
- Tape degradation section (saturation, wow, flutter, hiss, rolloff, dropout)
- WebView UI with paper texture and botanical aesthetic
- 130 total automatable parameters
