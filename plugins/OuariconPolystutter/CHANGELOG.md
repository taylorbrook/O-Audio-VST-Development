# Changelog

All notable changes to Ouaricon Polystutter will be documented in this file.

## [1.6.5] - 2026-01-20

### Added

- **Tape degradation bypass toggle**
  - New BYPASS button positioned left of tape knobs (next to section label)
  - When enabled, skips all tape degradation processing (saturation, wow, flutter, hiss, rolloff, dropout)
  - Tape knobs grey out when bypass is active (visual feedback)
  - Allows A/B comparison between clean and tape-processed signal
  - Parameter ID: `tape_bypass` (boolean, default: false/off)
  - Files modified: `PluginProcessor.cpp`, `PluginProcessor.h`, `PluginEditor.cpp`, `PluginEditor.h`, `index.html`, `parameter-bindings.js`

## [1.6.4] - 2026-01-19

### Fixed

- **Hiss level scaled down 10x to usable range**
  - Previous range: -60dB to -20dB (40dB range) - far too loud
  - New range: -60dB to -56dB (4dB range) - subtle tape hiss
  - Root cause: Original scaling made 10% hiss louder than desired 100%
  - File: `Source/DSP/TapeDegrader.cpp` line 165

## [1.6.3] - 2026-01-19

### Changed

- **DRY/WET knobs and MIDI/TRIG buttons moved to tape degradation row**
  - Controls relocated from footer section to right side of tape row
  - Unified bottom row: tape knobs (left) + mix/trigger controls (right)
  - Footer section removed entirely
  - Plugin window height reduced from 830px to 660px (more compact layout)

## [1.6.2] - 2026-01-19

### Changed

- **Tape degradation controls compacted to left half of UI**
  - Changed `.tape-knobs-row` from `justify-content: space-around` to `flex-start` (left-aligned)
  - Reduced gap between knobs from 15px to 8px for tighter spacing
  - Set explicit width of 500px (left half of 1000px plugin frame)
  - Changed knob containers from `flex: 1; max-width: 100px` to `flex: 0 0 auto; width: 75px`
  - "TAPE DEGRADATION" label now left-aligned (was centered)
  - All 6 knobs (SAT, WOW, FLUTTER, HISS, ROLLOFF, DROPOUT) fit comfortably in left half

## [1.6.1] - 2026-01-19

### Fixed

- **Title moved left to avoid preset bar overlap**
  - Changed title positioning from centered (`left: 50%`) to left-aligned (`left: 20px`)
  - Removed `translateX(-50%)` transform that caused horizontal centering
  - Title now sits at left edge, preset bar remains at right edge with no overlap

## [1.6.0] - 2026-01-19

### Added

- **Full preset system with save/load/navigation**
  - Preset bar in header area (top-right) with prev/next navigation
  - Dropdown menu showing all presets with factory/user badges
  - Save button opens native file dialog in User presets folder
  - Load button opens native file dialog to import presets
  - Delete button for user presets (factory presets protected)
  - Presets save ALL state: 128 parameters + sequencer patterns + lane enables
  - Presets stored as JSON in `~/Library/Ouaricon Polystutter/Presets/`

- **12 factory presets showcasing different stutter styles**
  1. Classic Stutter - Basic rhythmic 1/16 note stutter
  2. Glitch Machine - Fast chaotic stutters with pitch modulation
  3. Tape Echo - Slow repeats with warm tape character
  4. Rhythmic Bounce - Synced bounce effect
  5. Ambient Freeze - Long sustaining frozen moments
  6. Dub Delay - Classic reggae-style dub stutters
  7. Digital Chaos - Multi-lane glitchy madness
  8. Lo-Fi Dreams - Heavy tape degradation
  9. Rising Pitch - Pitch increases across lanes
  10. Polyrhythmic - Different subdivisions per lane
  11. Subtle Texture - Low probability, light effect
  12. Maximum Destruction - Everything cranked to max

### Technical

- Integrated OuariconPresetManager module from modules/persistence/preset-manager/
- Added native functions for WebView ↔ C++ preset communication:
  - savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile
  - getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset
  - deletePreset, isFactoryPreset
- Preset state integrated with DAW session save/restore (getStateInformation/setStateInformation)
- Factory presets initialized on first plugin load

## [1.5.6] - 2026-01-19

### Changed

- **SEQ button repositioned** to left of sequencer section (always visible and clickable)
- **Sequencer section shifted right** 70px to accommodate SEQ button placement
- **L1 label restored** in sequencer grid

## [1.5.5] - 2026-01-19

### Changed

- **Sequencer section moved up 100px** for tighter layout
- **SEQ button relocated** from footer to above sequencer
- Tape and footer sections adjusted accordingly

## [1.5.4] - 2026-01-19

### Changed

- **Bug images moved up 20px** for better alignment with lane headers

## [1.5.3] - 2026-01-19

### Changed

- **Bug images repositioned next to lane headers (one per lane)**
  - Each lane now has its own bug image positioned to the right of the lane button
  - Reduced size to 70% (56x70px) for better fit in header area
  - **State-aware opacity**: 100% when lane is active, 50% when lane is disabled
  - Smooth 300ms opacity transition when toggling lanes
  - Bug inherits lane state - dims along with other controls when lane is off

## [1.5.2] - 2026-01-18

### Changed

- **Bug images repositioned between lane sections** (superseded by v1.5.3)
  - Replaced 12 scattered fragments with 3 complete bug/spider images
  - Each bug now positioned as a visual divider between lanes:
    - Bug 1: Between Lane 1 and Lane 2 (x: 218px)
    - Bug 2: Between Lane 2 and Lane 3 (x: 468px)
    - Bug 3: Between Lane 3 and Lane 4 (x: 718px)
  - Vertically positioned at row 3 level (VOL/PAN/SWING controls, y: 280px)
  - Slightly increased opacity (0.32 vs 0.25) for better visibility
  - Cleaner visual separation between lane sections

## [1.5.1] - 2026-01-18

### Fixed

- **Lane progress bars now actually animate during repeat playback**
  - Root cause: CSS selector mismatch in parameter-bindings.js
  - HTML has `id="lane1_progress"` on the `.progress-fill` element itself
  - JavaScript was looking for `#lane1_progress .progress-fill` (nested), which returned null
  - Fix: Use `getElementById("lane1_progress")` directly for fill element
  - Use `.parentElement` to get the container for active/inactive class toggling
  - Progress bars now correctly show 0-100% animation during each repeat cycle

## [1.5.0] - 2026-01-18

### Added

- **Lane progress bars now animate during repeat playback**
  - Each lane's progress bar shows real-time playback position within the captured audio segment
  - Smooth ~30Hz updates from audio thread to WebView UI via custom event system
  - Progress is pitch-ratio aware (higher pitch = faster playback = shorter effective length)
  - Visual states:
    - **Active**: Green fill animates 0-100% as repeat plays
    - **Inactive**: Bar dimmed (30% opacity) when lane is not currently repeating
  - Implementation:
    - `RepeatLane::getProgress()` calculates 0.0-1.0 position accounting for pitch ratio
    - `PluginProcessor` stores progress/active state in atomic variables (thread-safe)
    - `PluginEditor` uses juce::Timer at 30Hz to emit `laneProgress` events to WebView
    - `parameter-bindings.js` listens for events and updates DOM progress bar widths

## [1.4.1] - 2026-01-18

### Fixed

- **Double-click text input now works on all knob types**
  - Root cause: Formatter for REPS, FILTER, and PAN returns a number, not a string
  - Calling `.match()` on a number fails silently, preventing text input from appearing
  - Fix: Convert formatter output to string before regex matching
  - Now works correctly on: REPS (1-16), PITCH (-12 to +12st), FILTER (-100 to +100), PAN (-100 to +100)

## [1.4.0] - 2026-01-18

### Added

- **Double-click text input for all knobs**
  - Double-click on any knob dial or its value readout to enter text input mode
  - Type raw numbers (e.g., "50" for 50%, "-12" for -12 semitones)
  - Suffix is added automatically based on parameter type (%, st, etc.)
  - Enter key or click-away confirms the value
  - Escape key cancels and reverts to original value
  - Values are clamped to valid parameter range
  - Works on all 40 knobs: lane controls (REPS, DECAY, PITCH, FILTR, PROB, VOL, PAN, SWING),
    tape section (SAT, WOW, FLUTTER, HISS, ROLLOFF, DROPOUT), and footer (DRY, WET)
  - Excludes SUBDIV which is a combo-box/dropdown, not a dial
  - CSS styled to match the vintage paper aesthetic

## [1.3.2] - 2026-01-18

### Fixed

- **AU not working in Logic Pro (blank UI, no audio processing)**
  - Root cause: Missing `VERSION` property in CMakeLists.txt caused AU to report Component Version 1.0.0
  - When v1.3.0 removed the sidechain bus, Logic's cached configuration (from v1.0.0) still expected sidechain
  - Logic tried to set up a bus configuration that no longer existed, causing initialization failure
  - Fix: Added `VERSION 1.3.2` to `juce_add_plugin()` in CMakeLists.txt
  - The version bump forces Logic to invalidate its AU cache and re-query the current bus configuration
  - AU now correctly reports stereo-only I/O without sidechain

## [1.3.1] - 2026-01-17

### Fixed

- **Critical: Plugin broken after v1.3.0 parameter removal**
  - Symptoms: Blank UI (only title visible), no audio processing
  - Root cause: PluginEditor.cpp/h still had relays and attachments for removed `envelope_enabled` and `sidechain_enabled` parameters
  - When the Editor constructor called `apvts.getParameter("envelope_enabled")`, it returned nullptr
  - This crashed the WebToggleButtonParameterAttachment initialization, preventing WebView from loading
  - Fix: Removed orphaned relay declarations, WebView option registrations, and parameter attachments from Editor files
  - Files modified: PluginEditor.h (4 lines removed), PluginEditor.cpp (8 lines removed)

## [1.3.0] - 2026-01-17

### Removed

- **ENV (Envelope Trigger) mode** - Removed due to unresolved audio artifacts
  - The envelope follower triggering caused graininess and clicks that couldn't be reliably fixed
  - Users should use beat-sync, manual, or MIDI trigger modes instead
  - Parameter `envelope_enabled` removed from APVTS

- **SC (Sidechain Trigger) mode** - Removed due to unresolved audio artifacts
  - Sidechain input triggering had similar issues to ENV mode
  - Sidechain input bus removed from plugin (simplified to stereo I/O only)
  - Parameter `sidechain_enabled` removed from APVTS

### Changed

- **TriggerRouter simplified to MIDI-only**
  - Removed envelope follower DSP components
  - Removed sidechain envelope follower DSP components
  - Cleaner, lighter codebase focused on working trigger modes

- **UI footer updated**
  - ENV and SC toggle buttons removed
  - Remaining triggers: SEQ, MIDI, TRIG (manual)

- **Total parameters reduced from 130 to 128**
  - Removed 2 boolean parameters (envelope_enabled, sidechain_enabled)

### Migration Notes

- Existing presets using ENV or SC modes will load with those parameters ignored
- The plugin is now simpler and more reliable for production use
- For transient-following behavior, consider using MIDI triggering from a gate plugin

## [1.2.2] - 2026-01-17

### Fixed

- **Sticky "reps" knobs on all 4 lanes now respond smoothly**
  - Root cause: Integer parameters (1-16 range) use discrete steps, but knob sensitivity was too low
  - Small mouse movements caused JUCE to snap back to nearest integer, creating "sticky" feel
  - User had to move in opposite direction first to break out of snap zone
  - Fix: Added delta accumulation for integer knobs in `parameter-bindings.js`
    - Detects integer parameters automatically (whole number min/max, range ≤20)
    - Accumulates mouse delta until it crosses a step boundary (1/15 normalized for 1-16 range)
    - Only sends value to JUCE when step change occurs, preventing snap-back
    - Prevents visual jitter by not updating UI for sub-step movements
  - Float knobs (decay, volume, etc.) unchanged - still use continuous behavior
  - ~13px drag now reliably moves one integer step (vs. previous sticky behavior)

## [1.2.1] - 2026-01-17

### Fixed

- **Clicks when reps=1 (single repeat mode)**
  - Root cause: Fade-out triggered immediately after `startNewRepeat()` incremented counter
  - With reps=1, `currentRepeat` became 1 after starting the repeat, triggering `currentRepeat >= maxRepeats`
  - The single repeat was cut off before it could play
  - Fix: Added check that repeat timer has expired (`samplesUntilNextRepeat <= 0`) before allowing fade-out
  - Now the full repeat plays before transitioning to fade-out

## [1.2.0] - 2026-01-17

### Changed

- **DSP engine reverted to v1.1.4 stable baseline**
  - Rationale: Complex crossfade/retrigger logic added in v1.1.5-v1.1.19 to fix ENV/Sidechain artifacts
    caused regressions in normal beat-sync stutter mode (clicks, audio artifacts)
  - This version prioritizes stable, click-free beat-sync stutter operation
  - RepeatLane.cpp reduced from 745 lines to 542 lines (simpler, more reliable)

### Known Issues

- **ENV and Sidechain trigger modes have audio artifacts**
  - Graininess and clicks when using envelope follower (ENV) triggering
  - Similar artifacts when using sidechain input triggering
  - These modes were not fully stable before the v1.1.5+ fixes, which introduced other regressions
  - Workaround: Use beat-sync, manual, or MIDI trigger modes for clean stutter
  - Future versions may address ENV/Sidechain with a different approach

### Migration Notes

- Beat-sync mode: Works cleanly (no changes needed)
- Manual trigger: Works cleanly (no changes needed)
- MIDI trigger: Works cleanly (no changes needed)
- ENV trigger: May experience artifacts (consider alternative trigger modes)
- Sidechain trigger: May experience artifacts (consider alternative trigger modes)

## [1.1.19] - 2026-01-17

### Fixed

- **Clicks at repeat boundaries eliminated**
  - Root cause: Gain discontinuity when decay is applied in startNewRepeat() after crossfade ends
  - Crossfade was at currentGain, but first sample after crossfade was at currentGain * decayAmount (~10% drop)
  - Fix: Added one-pole lowpass gain smoother
    - `smoothedGain` follows `currentGain` with gradual transitions (~500 samples to settle)
    - When startNewRepeat() applies decay, smoothedGain transitions smoothly instead of jumping
    - Uses `smoothedGain` for audio output instead of raw `currentGain`
  - Preserves v1.1.18's unconditional gain application (no audio loss)
  - Avoids v1.1.17's conditional gain application that caused audio regression

## [1.1.18] - 2026-01-17

### Fixed

- **Critical: Audio regression - only clicks heard, no stutter audio**
  - Root cause: v1.1.17 moved gain application inside conditional if/else blocks
  - This caused audio loss in edge cases where neither branch applied gain correctly
  - Fix: Revert to unconditional gain application AFTER crossfade logic (like v1.1.13)
  - Overlap-add crossfade from v1.1.14 is preserved for smooth loop boundaries
  - Gain is now ALWAYS applied to ensure audio output in all code paths

## [1.1.17] - 2026-01-17

### Fixed (REVERTED in v1.1.18)

- **Critical: Gain discontinuity causing clicks at every repeat boundary**
  - Root cause: Decay was applied in startNewRepeat() AFTER the crossfade completed
  - Crossfade output was at gain 1.0, but next sample was at gain 0.9 (10% drop = click)
  - Fix: Apply gain BEFORE blending in crossfade - old audio uses current gain, new audio uses next repeat's gain
  - Crossfade now smoothly transitions from oldGain to newGain (e.g., 1.0 → 0.9)
  - After startNewRepeat(), currentGain matches what the crossfade ended with = no discontinuity
  - **NOTE: This fix was reverted in v1.1.18 as it caused complete audio loss**

## [1.1.16] - 2026-01-17

### Fixed

- **Crossfade position wrap causing sample replay**
  - After crossfade, continue playback from safeCrossfade offset instead of 0
  - Applied to both timer-based and position-based repeat triggering

## [1.1.15] - 2026-01-17

### Fixed

- **Timer desynchronization causing audio gaps**
  - Playback position increment was inside audio-reading block, causing position to freeze at capture end
  - Fix: Move position increment outside the if block so it always advances every sample
  - Added position-based repeat triggering as fallback for pitch/swing desync

## [1.1.14] - 2026-01-17

### Changed

- **Loop boundary overlap-add crossfade**
  - Detect when approaching end of capture and blend with loop start
  - Crossfade starts BEFORE hitting boundary (not after)
  - Skips during retrigger crossfade or global fade-in to avoid conflicts

## [1.1.13] - 2026-01-17

### Changed

- **Per-lane trigger lockout for all trigger modes (ENV, Sidechain, MIDI)**
  - When a lane is triggered, it now ignores subsequent triggers until all its repetitions complete
  - Lockout is per-lane: Lane 1 being locked doesn't prevent Lane 2 from triggering
  - Triggers received during lockout are discarded (not queued)
  - Uses existing `isRepeating()` check: `currentRepeat < maxRepeats && isTriggered`
  - Lockout releases when last repeat starts playing, allowing natural fade-out
  - Manual trigger button and beat-sync modes are unaffected (only ENV/Sidechain/MIDI)
  - Provides more predictable stutter behavior - each trigger completes its full cycle

## [1.1.12] - 2026-01-17

### Fixed

- **Comprehensive crossfade system overhaul for ENV and Sidechain trigger modes**
  - After 7 previous fix attempts (v1.1.5-v1.1.11), conducted root cause analysis identifying 6 remaining issues
  - All crossfade-related audio artifacts now addressed with industry-standard techniques

- **Equal-power crossfade curve** (replaces linear)
  - Linear crossfades cause -6dB dip at 50% blend point, perceived as momentary volume drop
  - Now uses sine/cosine curves: `sin(t * π/2)` for fade-in, `cos(t * π/2)` for fade-out
  - Maintains constant perceived loudness throughout the crossfade transition

- **Increased crossfade duration from 5ms to 10ms**
  - 10ms covers 2+ complete cycles at 200Hz (the low end of bass frequencies)
  - Provides smoother transitions for low-frequency content while remaining imperceptible
  - Better masks any remaining discontinuities

- **Eliminated double-fading at trigger start**
  - Root cause: Both global envelope fade-in AND loop boundary fade-in were applying during first 10ms
  - This caused double attenuation at stutter onset, making the start sound weak/muffled
  - Fix: Skip loop boundary crossfade while global fade-in is active

- **Fixed deferred capture during retrigger**
  - Root cause: During retrigger, capture position was deferred to next processBlock()
  - But the crossfade needed the NEW capture position immediately to blend properly
  - During the deferral period, crossfade read from stale/incorrect buffer positions
  - Fix: Calculate capture position immediately for retriggers (buffer already has audio)
  - Deferred capture only used for initial triggers (where current block isn't in buffer yet)

- **Fixed old position exhaustion artifacts**
  - Root cause: When old playback position exceeded capture length during crossfade, it clamped to last sample
  - Repeating the same sample value creates a DC-like signal that sounds like graininess
  - Fix: When old position exhausts, its contribution is set to zero instead of clamped
  - The equal-power crossfade naturally handles this - old weight decreases as new weight increases
  - Result: Smooth transition to new audio without artifacts from repeating last sample

- **Fixed retrigger + loop boundary crossfade overlap** (additional fix)
  - Root cause: During retrigger, loop boundary crossfade was NOT being skipped
  - The skip check only looked at `fadeInSamplesRemaining <= 0`, but retriggers don't reset this
  - Result: Two crossfades applied simultaneously - retrigger blend + loop boundary fade-in
  - This caused the "new" audio to be double-attenuated during the first 10ms of retrigger
  - Fix: Also skip loop boundary crossfade when `retriggerCrossfadeActive` is true

## [1.1.11] - 2026-01-17

### Fixed

- **Sample rate compatibility for ENV and Sidechain trigger modes**
  - Root cause: Envelope follower channel index mismatch
  - `envelopeFollowerRight` and `sidechainEnvelopeRight` were using `processSample(1, ...)` (channel 1)
  - But these are separate mono filter instances - each should use channel 0
  - Channel 1 had different filter state that wasn't properly updated at some sample rates
  - This caused inconsistent envelope tracking, leading to clicks/graininess at 48k/96k/192kHz
  - Fix: Changed right channel filters to use `processSample(0, ...)` like left channel
  - Tested at: 44.1kHz, 48kHz, 96kHz, 192kHz - all modes now work correctly

## [1.1.10] - 2026-01-17

### Fixed

- **Graininess/distortion when ENV triggers near end of repeat cycle**
  - Root cause: Retrigger crossfade was cut short when old playback position exceeded capture length
  - The condition `if (oldFractionalPlaybackPosition < oldEffectiveLength)` skipped the blend
  - But the crossfade still had samples remaining, causing a sudden jump from ~50% blend to 100% new
  - This created micro-discontinuities (6 detected in test audio) causing audible graininess
  - Fix: Clamp old position to valid range instead of skipping the blend
  - Crossfade now always completes its full 5ms duration regardless of old position
  - Testing: Play audio with ENV mode - should be smooth without graininess

## [1.1.9] - 2026-01-16

### Fixed

- **Sidechain detection missing hysteresis and cooldown**
  - Root cause: Sidechain trigger detection had NO cooldown or hysteresis (only ENV was fixed in v1.1.8)
  - Sidechain could retrigger on every threshold crossing without any protection
  - Fix: Added same state machine with 6dB hysteresis and 50ms cooldown to sidechain detection
  - Added separate `sidechainCooldownSamplesRemaining` timer for sidechain

- **Sidechain triggering when no sidechain input is connected**
  - Root cause: Some DAWs mirror the main input to unconnected sidechain buses
  - The plugin checked if sidechain bus EXISTS, but not if it's ENABLED by the host
  - Fix: Added `getBus(true, 1)->isEnabled()` check to verify host actually routed audio to sidechain
  - Sidechain detection now only runs when user has explicitly connected a sidechain source

## [1.1.8] - 2026-01-16

### Fixed

- **ENV trigger retriggering due to threshold bounce**
  - Root cause: `wasAboveThreshold` was updated every sample, even during cooldown
  - With music that has varying dynamics, envelope fluctuates around threshold
  - When envelope briefly dipped below threshold during cooldown, `wasAboveThreshold` toggled to `false`
  - After cooldown ended, next sample above threshold caused another trigger
  - Fix 1: State is now locked during cooldown - `wasAboveThreshold` not updated until cooldown ends
  - Fix 2: Added 6dB hysteresis - envelope must drop to 50% of threshold (armThreshold) before re-arming
  - This prevents "threshold bounce" where envelope oscillates around the trigger point
  - State machine: TRIGGER at threshold → LOCK until below armThreshold → RE-ARM → wait for next trigger
  - Testing: Play sustained material with ENV mode - should only trigger on distinct transients, not continuously

## [1.1.7] - 2026-01-16

### Fixed

- **Beat-sync triggering conflict with ENV/Sidechain/MIDI modes**
  - Root cause: `updateBeatSync()` triggered lanes on every DAW subdivision boundary regardless of trigger mode settings
  - When ENV mode was enabled, both systems called `trigger()` simultaneously:
    - Envelope follower triggered on amplitude transients
    - Beat-sync triggered on grid boundaries
  - This caused double triggering, conflicting capture positions, and audio artifacts
  - Fix: Added exclusivity check in `updateBeatSync()` - when any alternative trigger mode (ENV, Sidechain, or MIDI) is active, beat-sync triggering is skipped
  - The selected trigger source now handles triggering exclusively
  - Testing: Enable ENV mode and verify stutter only triggers on transients, not on beat grid

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
