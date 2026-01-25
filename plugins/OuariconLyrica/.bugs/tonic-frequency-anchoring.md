# Bug: Tonic Selection Does Not Affect Sounding Pitches

**Status:** ✅ RESOLVED
**Created:** 2026-01-22
**Resolved:** 2026-01-23
**Fixed in:** v1.12.3
**Priority:** High
**Reference:** See `improvements/tonic-rotation-reference.md` for reusable implementation guide

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

## Root Cause

The `calculateCustomFrequency()` function was calculating all frequencies relative to A4=440Hz, even when the tonic changed. The rotated intervals were being used, but the reference point stayed at A4, so the tonic note was not at its 12-TET pitch.

The non-KBM code path was trying to:
1. Calculate the note's scale degree and cents offset
2. Calculate a reference point (A4) and its cents offset
3. Find the difference and apply it to A4=440Hz

This approach was fundamentally wrong because when the tonic rotates, the intervals change their relationship to A4, but the formula still expected A4 to be the stable reference.

## Solution

Rewrote the non-KBM code path in `calculateCustomFrequency()` with two key changes:

1. **Anchor tonic at 12-TET frequency** - The tonic note is always at its equal temperament pitch
2. **Use original intervals, not rotated** - The interval pattern stays the same, just maps to different notes

```cpp
// v1.12.3 FIX: Tonic Frequency Anchoring
// The tonic note is ALWAYS at its 12-TET frequency.
// The interval pattern stays the same, just starting from the tonic.
//
// Example with Werckmeister III (intervals: 0, 90, 192, 294, 390, ...):
//   Tonic=C: C=0¢, C#=90¢, D=192¢, D#=294¢, E=390¢...
//   Tonic=D: D=0¢, D#=90¢, E=192¢, F=294¢, F#=390¢...

// Calculate scale degree relative to tonic
int noteRelativeToTonic = midiNote - tonic;

// Calculate which octave of the scale we're in and the scale degree
int octaveNumber = noteRelativeToTonic >= 0
    ? noteRelativeToTonic / scaleSize
    : (noteRelativeToTonic - scaleSize + 1) / scaleSize;
int scaleDegree = noteRelativeToTonic - (octaveNumber * scaleSize);

// Get the interval for this scale degree from the ORIGINAL intervals
// (not rotated - the same interval pattern applies regardless of tonic)
double intervalCents = scaleIntervals[scaleDegree];

// Calculate the MIDI note number of the tonic in this scale octave
int tonicMidiInThisOctave = tonic + (octaveNumber * scaleSize);

// Get the 12-TET frequency of the tonic in this octave - THIS IS THE ANCHOR
double tonicFreq = calculate12TETFrequency(tonicMidiInThisOctave);

// Calculate final frequency: tonic's 12-TET freq + interval offset in cents
return tonicFreq * std::pow(2.0, intervalCents / 1200.0);
```

## Test Case

1. Load OuariconLyrica
2. Go to Tuning tab
3. Select "Werckmeister III" preset
4. Set tonic to C, play D4 → should be ~292.3 Hz (192 cents above 12-TET C4)
5. Set tonic to D, play D4 → should be ~293.66 Hz (12-TET D4 - the tonic is pure)
6. With tonic D, play E4 → should be 192 cents above D4 (rotated interval)
7. Use True Keys visualization to verify intervals match the displayed pattern

## Files Modified

- `Source/DSP/TuningEngine.cpp` - rewrote non-KBM code path in `calculateCustomFrequency()`

## What Was Tried (Historical)

### Attempt 1: Transposition in calculate12TETFrequency (WRONG)
- Added `int transposedNote = midiNote - tonic` to transpose all notes
- Result: This was transposition, not modal rotation
- Reverted: commit e717fc0

### Attempt 2: Reference-based calculation (WRONG)
- Tried to calculate relative to A4=440Hz with adjusted reference cents
- Result: Formula was fundamentally flawed - couldn't anchor tonic correctly
- Root cause identified: the A4 reference approach doesn't work for tonic rotation

### Attempt 3: Tonic-anchored + rotated intervals (PARTIAL - v1.12.3 first try)
- Anchor each scale octave to the 12-TET frequency of the tonic
- Still used `rotatedIntervals` which mathematically rotated the interval VALUES
- Result: Tonic was correctly at 12-TET, but intervals were wrong (e.g., D#=102¢ instead of 90¢)

### Attempt 4: Tonic-anchored + original intervals (CORRECT - v1.12.3 final)
- Anchor each scale octave to the 12-TET frequency of the tonic
- Use original `scaleIntervals` directly (not rotated)
- The scale degree calculation `(midiNote - tonic) mod scaleSize` handles the "rotation"
- Formula: `12-TET(tonic_in_this_octave) * 2^(scaleIntervals[scale_degree] / 1200)`
- Result: Works correctly - tonic at 12-TET, interval pattern preserved and mapped to correct notes
