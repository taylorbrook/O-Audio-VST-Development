# Bug: Tonic Selection Does Not Affect Sounding Pitches

**Status:** In Progress
**Created:** 2026-01-22
**Priority:** High

## Problem Description

When changing the tonic in the Tuning tab, the UI correctly updates (interval table and circular visualization show rotated pattern), but the actual sounding frequencies do not change. Pitches always stay "in relation to C" regardless of tonic selection.

## Expected Behavior

**Key principle: "Tonic note is always the equal temperament note."**

With Werckmeister III:
- **Tonic = C:** C is at 12-TET C (261.63 Hz). D is 192 cents above C = ~292.3 Hz
- **Tonic = D:** D is at 12-TET D (293.66 Hz). E is 192 cents above D (rotated interval)

So D with tonic=C (~292.3 Hz) should sound DIFFERENT from D with tonic=D (293.66 Hz).

The interval PATTERN rotates with the tonic:
- Tonic C: C→C#=90¢, C#→D=102¢, D→D#=102¢...
- Tonic D: D→D#=90¢ (first interval now starts from D), D#→E=102¢...

## What Was Tried

### Attempt 1: Transposition in calculate12TETFrequency (WRONG)
- Added `int transposedNote = midiNote - tonic` to transpose all notes
- Result: This was transposition, not modal rotation
- Reverted: commit e717fc0

### Attempt 2: Tonic-anchored calculation in calculateCustomFrequency
- Modified the non-KBM path to:
  1. Calculate 12-TET frequency of the tonic: `tonic12TET = a4Frequency * 2^((60+tonic-69)/12)`
  2. Calculate note's position relative to tonic in octave 5
  3. Get cents from rotated intervals
  4. Return `tonic12TET * 2^(cents/1200)`
- Result: Did not work (user reported no change)
- Code is still in place (not reverted)

## Technical Details

### Key Files
- `Source/DSP/TuningEngine.cpp` - frequency calculation
- `Source/DSP/TuningEngine.h` - class definition
- `Resources/ui/index.html` - UI (correctly shows rotated intervals)

### Key Functions
- `calculateCustomFrequency()` - calculates frequency for custom tunings (Scala mode)
- `calculate12TETFrequency()` - calculates frequency for 12-TET mode
- `rotateIntervalsForTonic()` - rotates interval pattern when tonic changes
- `setTonicNote()` - called when user changes tonic, triggers rotation and rebuild
- `rebuildFrequencyTable()` - rebuilds the 128-note frequency lookup table

### Current Code Flow
1. User changes tonic via UI → calls `setTonicNote()`
2. `setTonicNote()` stores tonic, calls `rotateIntervalsForTonic()`, then `rebuildFrequencyTable()`
3. `rebuildFrequencyTable()` checks mode:
   - Mode::TwelveTET → calls `calculate12TETFrequency()` (ignores tonic - 12-TET has equal intervals)
   - Mode::Scala → calls `calculateCustomFrequency()` (should use rotated intervals)

### Suspected Issues to Investigate
1. Is the mode correctly set to Scala when using Werckmeister III preset?
2. Are the rotated intervals being used in the calculation?
3. Is `rebuildFrequencyTable()` actually being called when tonic changes?
4. Is the frequency table being read correctly by the audio engine?
5. Are voices using stale frequency values?

### Debug Steps for Next Session
1. Add DBG() output in `setTonicNote()` to verify it's called
2. Add DBG() output in `rebuildFrequencyTable()` to verify mode and sample frequencies
3. Add DBG() output in `calculateCustomFrequency()` to see actual calculation values
4. Verify that `currentMode` is Scala (1) not TwelveTET (0) when preset is Werckmeister III
5. Check if voices are caching frequencies and not updating

## Test Case

1. Load OuariconLyrica
2. Go to Tuning tab
3. Select "Werckmeister III" preset (or any non-12TET preset)
4. Set tonic to C, play D4 → note the pitch
5. Set tonic to D, play D4 → pitch should be noticeably higher (~1.4 Hz difference)
6. With tonic D, play E4 → should be 192 cents above D (not 390 cents above C)

## Related Files
- `plugins/OuariconLyrica/improvements/tuning-system-overhaul.md` - full tuning system plan
- `plugins/OuariconLyrica/improvements/IMPLEMENTATION-GUIDE.md` - implementation reference

## Version Info
- Current version: 1.12.2 (PLUGINS.md not updated since fix didn't work)
- Attempted fix would be: 1.12.4
