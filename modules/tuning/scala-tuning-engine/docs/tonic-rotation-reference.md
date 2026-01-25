# Tonic Rotation Implementation Reference

**Purpose:** Reusable reference for implementing tonic selection in microtuning systems.
**Verified:** 2026-01-23 in OuariconLyrica v1.12.3

---

## Core Concept

When a user selects a tonic (root note) for a temperament, they expect:

1. **The tonic note sounds at its 12-TET frequency** (e.g., D4 = 293.66 Hz when tonic=D)
2. **The interval pattern stays the same** but maps to different notes starting from the tonic

This is NOT a mathematical rotation of interval values - it's a remapping of which notes receive which intervals.

---

## Example: Werckmeister III

**Original intervals (from C):**
```
C=0¢, C#=90¢, D=192¢, D#=294¢, E=390¢, F=498¢, F#=588¢, G=696¢, G#=792¢, A=888¢, A#=996¢, B=1092¢
```

**When tonic = D:**
```
D=0¢, D#=90¢, E=192¢, F=294¢, F#=390¢, G=498¢, G#=588¢, A=696¢, A#=792¢, B=888¢, C=996¢, C#=1092¢
```

Notice:
- The interval VALUES are identical (0, 90, 192, 294, 390...)
- They just map to different note NAMES
- Scale degree 1 is always 90¢ above the tonic, regardless of which note is the tonic

---

## Wrong Approach: Mathematical Rotation

A common mistake is to mathematically rotate the interval values:

```cpp
// WRONG - rotates interval VALUES
double tonicOffset = intervals[tonic];  // e.g., 192¢ for D
for (int i = 0; i < 12; i++) {
    int sourceIdx = (tonic + i) % 12;
    rotatedIntervals[i] = intervals[sourceIdx] - tonicOffset;
    if (rotatedIntervals[i] < 0) rotatedIntervals[i] += 1200;
}
```

This produces WRONG results for Werckmeister III with tonic=D:
```
D=0¢, D#=102¢, E=198¢, F=306¢...  // WRONG - intervals have changed!
```

The first interval (D→D#) becomes 102¢ instead of 90¢ because it's subtracting the original D position from D#.

---

## Correct Approach: Scale Degree Remapping

The correct approach keeps the original intervals and remaps which MIDI notes correspond to which scale degrees:

```cpp
// CORRECT - remaps notes to scale degrees
double calculateFrequency(int midiNote, int tonic, const std::vector<double>& scaleIntervals)
{
    int scaleSize = scaleIntervals.size() - 1;  // Exclude period (1200¢)

    // Calculate which scale degree this MIDI note is
    // When tonic=D(2), MIDI note D(2) becomes scale degree 0
    int noteRelativeToTonic = midiNote - tonic;

    // Handle octaves
    int octaveNumber = noteRelativeToTonic >= 0
        ? noteRelativeToTonic / scaleSize
        : (noteRelativeToTonic - scaleSize + 1) / scaleSize;

    int scaleDegree = noteRelativeToTonic - (octaveNumber * scaleSize);
    if (scaleDegree < 0) {
        scaleDegree += scaleSize;
        octaveNumber--;
    }

    // Get interval from ORIGINAL intervals (not rotated!)
    double intervalCents = scaleIntervals[scaleDegree];

    // Anchor: tonic is always at its 12-TET frequency
    int tonicMidiInThisOctave = tonic + (octaveNumber * scaleSize);
    double tonicFreq = calculate12TETFrequency(tonicMidiInThisOctave);

    // Final frequency
    return tonicFreq * std::pow(2.0, intervalCents / 1200.0);
}

double calculate12TETFrequency(int midiNote)
{
    const double A4_FREQ = 440.0;
    const double semitonesFromA4 = midiNote - 69;
    return A4_FREQ * std::pow(2.0, semitonesFromA4 / 12.0);
}
```

---

## Key Formula

```
scaleDegree = (midiNote - tonic) mod scaleSize
frequency = 12TET(tonic_in_octave) × 2^(intervals[scaleDegree] / 1200)
```

Where:
- `midiNote` = the MIDI note being played (0-127)
- `tonic` = the selected tonic (0=C, 1=C#, 2=D, etc.)
- `scaleSize` = number of notes in the scale (typically 12)
- `intervals[]` = the ORIGINAL temperament intervals (unchanged)
- `tonic_in_octave` = the MIDI note number of the tonic in the same octave as the played note

---

## Verification Test

With Werckmeister III:

| Test | Tonic=C | Tonic=D |
|------|---------|---------|
| Play tonic | C4 = 261.63 Hz (12-TET C) | D4 = 293.66 Hz (12-TET D) |
| Play degree 1 | C#4 = 275.6 Hz (+90¢) | D#4 = 309.4 Hz (+90¢) |
| Play degree 2 | D4 = 292.3 Hz (+192¢) | E4 = 328.4 Hz (+192¢) |

The Hz values for degree 1 and 2 should be different between tonic=C and tonic=D, but the CENT intervals from the tonic should be identical.

---

## UI Considerations

The interval list display should show intervals relative to the tonic:
- When tonic=C: "C=0¢, C#=90¢, D=192¢..."
- When tonic=D: "D=0¢, D#=90¢, E=192¢..."

This can be achieved by rotating the NOTE LABELS in the UI while keeping interval values the same, or by displaying `intervals[i]` next to `noteNames[(tonic + i) % 12]`.

---

## Common Pitfalls

1. **Rotating interval values instead of remapping notes** - The intervals themselves should never change when tonic changes

2. **Using A4=440Hz as the reference point** - When tonic changes, the reference point should be the tonic's 12-TET frequency, not A4

3. **Confusing "rotation" terminology** - "Rotating" the scale means changing which note is the starting point, NOT mathematically rotating the interval array

4. **Forgetting octave handling** - The scale degree calculation must handle negative numbers and octave wrapping correctly

---

## Module Implementation

The `OuariconTuningEngine` class in this module implements the correct algorithm in `calculateScala()`:

```cpp
inline double OuariconTuningEngine::calculateScala(int midiNote) const
{
    int tonic = tonicOffset.load();
    int tonicMidi = 60 + tonic;  // Tonic in octave 4

    // Calculate scale degree relative to tonic
    int noteOffset = midiNote - tonicMidi;
    int octave = noteOffset / scaleDegrees;
    int degree = noteOffset % scaleDegrees;

    if (degree < 0) {
        degree += scaleDegrees;
        octave--;
    }

    // Get interval from ORIGINAL intervals (not rotated!)
    double cents = scaleIntervals[degree];
    cents += octave * scaleIntervals.back();  // Add octave transposition

    // Anchor: tonic is always at its 12-TET frequency
    double tonicFreq = refPitch * std::pow(2.0, (tonicMidi - 69) / 12.0);

    return tonicFreq * std::pow(2.0, cents / 1200.0);
}
```

---

## Origin

This algorithm was developed and verified in OuariconLyrica v1.12.3 (2026-01-23).

**OuariconLyrica files for reference:**
- `Source/DSP/TuningEngine.cpp` - `calculateCustomFrequency()` implementation
- `Source/DSP/TuningEngine.h` - TuningEngine class definition
- `.bugs/tonic-frequency-anchoring.md` - Full bug investigation history
- `improvements/tonic-rotation-reference.md` - Original reference document
