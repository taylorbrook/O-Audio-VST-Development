# Bug: Chord Voices Always 12-TET — Temperament Not Applied

## Version
v1.5.1 (current working tree, uncommitted changes from failed fix attempts)

## Symptom
When a non-12-TET temperament (Just Intonation, Pythagorean, etc.) is selected in the tuning panel, the auto-generated chord voices still play in 12-TET. The temperament has no audible effect on chord intervals.

## How It Worked Before (v1.2.0 and earlier)

Before the scala-tuning-engine module was integrated in v1.3.0, chord voices were tuned by `TuningSystem` — a simple, self-contained class that **defaulted to Just Intonation**.

### Old flow (v1.2.0):
1. `ChordGenerator::generateChord()` produced MIDI notes with 12-TET semitone offsets (+4 for M3, +7 for P5, etc.)
2. `WavetableVoice::startNote()` called `tuningSystemPtr->getFrequencyWithOffset(midiNote, centOffset)` for each chord voice
3. `TuningSystem::getFrequencyWithOffset()` applied temperament to each note:
   ```cpp
   double baseFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);  // 12-TET base
   int relativePitch = (pitchClass - tonic + 12) % 12;
   double tuningCents = getCentOffset(relativePitch);  // JI/Pythagorean/etc. cents
   return baseFreq * centsToRatio(tuningCents + centOffset);
   ```
4. This applied the temperament interval ON TOP of the 12-TET base frequency
5. Default mode was **JustIntonation** — user heard JI-tuned chords out of the box

### Key detail about old TuningSystem math:
The old code multiplied the 12-TET frequency by `centsToRatio(absoluteCents)` where `absoluteCents` is the full interval (e.g., 386.31 for JI major third). This is NOT mathematically correct JI — correct JI would use the DEVIATION from 12-TET (e.g., -13.69 cents). But it produced a distinctive, wider-than-12-TET sound that was the intended character of the plugin. For example:
- C4 (MIDI 60): 261.63 * pow(2, 0/1200) = 261.63 Hz (correct, root is always right)
- E4 (MIDI 64): 329.63 * pow(2, 386.31/1200) = 329.63 * 1.25 = 412.04 Hz
- G4 (MIDI 67): 392.00 * pow(2, 701.96/1200) = 392.00 * 1.5 = 588.00 Hz

### Old files (still exist in codebase but unused):
- `Source/DSP/TuningSystem.h` — Simple class with JI/Pythagorean/12-TET cent tables
- `Source/DSP/TuningSystem.cpp` — `getFrequencyWithOffset()` applies cents on top of 12-TET base
- Old parameter: `"tuningSystem"` (Choice: 12-TET/JI/Pythagorean/Historical/Scala, **default: 1 = JI**)
- Old `processBlock` set mode each block: `tuningSystem.setMode(tuningSystemIndex)` and `tuningSystem.setTonicNote(keyRoot)`

## What Broke in v1.3.0 (TuningEngine Integration)

The scala-tuning-engine module (`TuningEngine`) replaced `TuningSystem` as the frequency source for chord voices. The key differences:

1. **TuningEngine defaults to 12-TET** (constructor sets `currentMode = Mode::TwelveTET`), while old TuningSystem defaulted to JI
2. **TuningEngine uses a pre-computed frequency table** — `getFrequency(midiNote)` reads from `frequencyTable[midiNote]`, computed by `rebuildFrequencyTable()`
3. **`rebuildFrequencyTable()` checks `currentMode`**: if TwelveTET, uses `calculate12TETFrequency()` and ignores stored intervals entirely
4. **`setBuiltInPreset()` has a mode-before-rebuild ordering bug**: it calls `setCustomIntervals()` (which rebuilds the table) WHILE mode is still the old value, THEN changes mode without rebuilding again

### The ordering bug in `setBuiltInPreset()` (TuningEngine.cpp ~line 178):
```cpp
setCustomIntervals(intervals, name);  // Calls rebuildFrequencyTable() — but mode is still TwelveTET!

// Mode changed AFTER rebuild — table still has 12-TET values
if (preset == BuiltInPreset::Equal12TET)
    currentMode.store(Mode::TwelveTET, ...);
else
    currentMode.store(Mode::Scala, ...);  // Too late, table already built with wrong mode
```

### Additional issue — `tuning_tuningMode` parameter conflicts:
The plugin has TWO independent mode controls that can conflict:
- `tuning_tuningMode` parameter: "12-TET" / "Custom" / "MTS-ESP" (default: 0 = 12-TET)
- `setBuiltInPreset()`: sets mode based on temperament (JI → Scala, 12-TET → TwelveTET)

If `tuning_tuningMode` is 0 (12-TET) and `parameterChanged` fires after `setBuiltInPreset`, it calls `tuningEngine.setMode(TwelveTET)` which overrides back to 12-TET. This is a race between the two mode-setting mechanisms.

## Current State of Working Tree (Uncommitted Changes)

Three files have been modified with failed fix attempts. **Recommend reverting all uncommitted changes** before attempting a fresh fix:
```bash
git checkout -- plugins/O-IntonationPad/Source/DSP/ChordGenerator.cpp
git checkout -- plugins/O-IntonationPad/Source/DSP/TuningEngine.cpp
git checkout -- plugins/O-IntonationPad/Source/PluginProcessor.cpp
```

### Changes made (all unsuccessful):
1. **ChordGenerator.cpp**: Added custom scale path (scaleType==10) using semitone offsets from `customScale[]` array. This is for the separate "custom scale builder" feature, not related to the temperament bug.
2. **TuningEngine.cpp**: Moved mode-set before `setCustomIntervals()` in `setBuiltInPreset()`. Correct in principle but did not fix the user-facing issue.
3. **PluginProcessor.cpp**: Added "Custom" to keyScale choices, custom scale state save/restore, `setCustomScaleCents()`. Removed the call to `tuningEngine.setCustomIntervals()` from `setCustomScaleCents()` to prevent overwriting temperament intervals.

## Root Cause Summary

The chord voices are always 12-TET because:
1. `TuningEngine::getFrequency()` reads from a pre-computed frequency table
2. The frequency table is built by `rebuildFrequencyTable()` which checks `currentMode`
3. `currentMode` is almost always `TwelveTET` because:
   - Constructor defaults to TwelveTET
   - `setBuiltInPreset()` builds table before changing mode (ordering bug)
   - `tuning_tuningMode` parameter defaults to 0 (12-TET) and may override preset mode
4. When mode is TwelveTET, `rebuildFrequencyTable()` uses `calculate12TETFrequency()` for all 128 notes, completely ignoring any stored JI/Pythagorean intervals

## Suggested Fix Approaches

### Approach A: Fix TuningEngine (minimal change)
1. Fix ordering in `setBuiltInPreset()` — set mode BEFORE `setCustomIntervals()`
2. Ensure `tuning_tuningMode` doesn't fight with temperament presets (maybe remove the mode parameter, or have the preset auto-update it)
3. Verify `rebuildFrequencyTable()` is called after ALL mode changes

### Approach B: Restore old TuningSystem for chord voices (preserve character)
The old `TuningSystem` math (cents applied on top of 12-TET base) produced the plugin's characteristic sound. Could restore it alongside TuningEngine:
1. Keep `TuningEngine` for the tuning panel features (Scala files, KBM, etc.)
2. Use `TuningSystem` for chord voice frequency calculation (preserves original sound)
3. Wire `TuningSystem` mode from the temperament preset selection

### Approach C: Fix TuningEngine AND match old behavior
Modify `TuningEngine::calculateCustomFrequency()` to use the same math as old `TuningSystem`:
```cpp
// Old behavior: 12-TET base * centsToRatio(absoluteInterval)
double baseFreq = calculate12TETFrequency(midiNote);
int relativePitch = (midiNote % 12 - tonic + 12) % 12;
double tuningCents = scaleIntervals[relativePitch];
return baseFreq * std::pow(2.0, tuningCents / 1200.0);
```
Note: This is NOT standard JI math. Standard JI computes frequency = root * ratio. The old code applied JI intervals multiplicatively on top of 12-TET, producing wider intervals. This may be intentional for the pad character.

## Key Files
- `Source/DSP/TuningEngine.cpp` — `setBuiltInPreset()`, `rebuildFrequencyTable()`, `calculateCustomFrequency()`
- `Source/DSP/TuningEngine.h` — Mode enum, frequency table
- `Source/DSP/TuningSystem.cpp` — Old working implementation (still in codebase, unused)
- `Source/DSP/WavetableVoice.cpp` — `startNote()` calls `tuningEnginePtr->getFrequency()`
- `Source/PluginProcessor.cpp` — `parameterChanged()` for tuning params, `processBlock()` voice setup
- `Source/PluginProcessor.h` — `tuningEngine` member

## Reference Commits
- `3b73d36` — v1.2.0 (last version with working TuningSystem, before TuningEngine)
- `9ec3f8f` — v1.3.0 (TuningEngine integration, where the regression was introduced)
- `34bf4ed` — v1.4.0 (tuning panel v2.0.0 upgrade)

## Backup
v1.4.0 backup at: `backups/O-IntonationPad/v1.4.0/`
