# Non-12-Note Scale Linear Mapping Improvement

**Version:** 1.0
**Created:** 2026-01-23
**Target Version:** v1.13.0 (Minor Release)
**Status:** ✅ RESOLVED (v1.13.2)

---

## Problem Statement

When loading scales with a number of degrees other than 12 (e.g., 19-EDO, 31-EDO, Bohlen-Pierce), the current TuningEngine has bugs that prevent correct playback:

1. **Incorrect scale degree calculation:** The code uses MIDI note 0 as the reference point instead of middle C (MIDI 60), causing scale degrees to be miscalculated
2. **Premature octave wrapping:** For a 19-note scale, the code jumps to the next "scale octave" at MIDI note 76 instead of continuing linearly
3. **Hidden tonic selector:** The UI hides the tonic selector for non-12 scales, preventing transposition
4. **Broken interval rotation:** The `rotateIntervalsForTonic()` function assumes 12-note scales

### Example Bug (19-note scale)

| MIDI Note | Current Behavior | Expected Behavior |
|-----------|------------------|-------------------|
| 60 | Degree 3 (wrong octave calc) | Degree 0 |
| 75 | Degree 18 | Degree 15 |
| 76 | **Jumps to degree 0, next octave** | Degree 16 |
| 78 | Degree 2 | Degree 18 |
| 79 | Degree 3 | Degree 0 (next scale octave) |

---

## Requirements

| Requirement | Specification |
|-------------|---------------|
| **Linear mapping anchor** | MIDI 60 + tonic offset (tonic=D → MIDI 62 = degree 0) |
| **Scale degree progression** | Each MIDI key = next scale degree, wrapping at scale size |
| **Tonic selector UI** | Show 12 notes (C-B) for all scale sizes |
| **Tonic behavior** | Semitone transposition (anchor shifts by 12-TET semitones) |
| **Period handling** | Use scale's period from SCL file (linear mapping for all scales) |
| **KBM priority** | KBM mapping takes priority when loaded; otherwise use linear mapping |
| **Backwards compatibility** | 12-note scale behavior unchanged |

---

## Implementation Plan

### Phase 1: Core DSP Fix

#### 1.1 Update `calculateCustomFrequency()` - Non-KBM Path

**File:** `Source/DSP/TuningEngine.cpp`
**Lines:** ~954-1011

**Current code (buggy):**
```cpp
// Calculate scale degree relative to tonic
int noteRelativeToTonic = midiNote - tonic;

octaveNumber = noteRelativeToTonic >= 0
    ? noteRelativeToTonic / scaleSize
    : (noteRelativeToTonic - scaleSize + 1) / scaleSize;

scaleDegree = noteRelativeToTonic - (octaveNumber * scaleSize);
```

**Fixed code:**
```cpp
// Linear mapping anchor point: MIDI 60 (middle C) + tonic offset
// When tonic = 0 (C), MIDI 60 = degree 0
// When tonic = 2 (D), MIDI 62 = degree 0
const int anchorNote = 60 + tonic;

// Calculate position relative to anchor
int noteRelativeToAnchor = midiNote - anchorNote;

// Calculate scale octave and degree using linear mapping
// This works for any scale size (12, 19, 31, etc.)
if (noteRelativeToAnchor >= 0)
{
    scaleOctave = noteRelativeToAnchor / scaleSize;
    scaleDegree = noteRelativeToAnchor % scaleSize;
}
else
{
    // Handle negative positions (notes below anchor)
    scaleOctave = (noteRelativeToAnchor - scaleSize + 1) / scaleSize;
    scaleDegree = noteRelativeToAnchor - (scaleOctave * scaleSize);
}

// Get interval for this scale degree from ORIGINAL intervals
// (not rotated - tonic transposition is handled by anchor shift)
double intervalCents = scaleIntervals[static_cast<size_t>(scaleDegree)];

// Get the scale's period (typically 1200¢ for octave, 1902¢ for tritave)
double period = scaleIntervals.back();

// Calculate frequency:
// 1. Start with 12-TET frequency of anchor note
// 2. Add scale octaves (each octave = period cents)
// 3. Add scale degree interval
double anchorFreq = calculate12TETFrequency(anchorNote);
double totalCents = (scaleOctave * period) + intervalCents;

// Apply octave stretch if enabled
double stretchedCents = totalCents * static_cast<double>(octaveStretch);

return anchorFreq * std::pow(2.0, stretchedCents / 1200.0);
```

#### 1.2 Simplify Tonic Handling for Non-12 Scales

**Current behavior:** `rotateIntervalsForTonic()` rotates the interval array, which only makes sense for 12-note scales where tonic selection means modal rotation.

**New behavior:** For the linear mapping approach, tonic simply shifts the anchor point. We don't need to rotate intervals at all - the same interval pattern applies, just starting from a different MIDI note.

**Change:** Skip interval rotation when using linear mapping (non-KBM mode). The anchor shift handles transposition naturally.

```cpp
void TuningEngine::setTonicNote(int tonicIndex)
{
    int newTonic = juce::jlimit(0, 11, tonicIndex);
    int oldTonic = tonicOffset.load(std::memory_order_relaxed);

    if (oldTonic != newTonic)
    {
        tonicOffset.store(newTonic, std::memory_order_relaxed);

        // Note: For linear mapping (non-KBM), tonic shifts the anchor point
        // Interval rotation is only needed for KBM-based modal rotation
        // The frequency calculation handles this via anchorNote = 60 + tonic

        rebuildFrequencyTable();
    }
}
```

#### 1.3 Update `resetKeyboardMapping()` for Variable Scale Sizes

**File:** `Source/DSP/TuningEngine.cpp`

Currently hardcodes 12-note mapping. Update to use current scale size:

```cpp
void TuningEngine::resetKeyboardMapping()
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    // Use current scale size, default to 12 if not set
    int mapSize = (scaleDegrees > 0) ? scaleDegrees : 12;

    kbmMapSize = mapSize;
    kbmFirstNote = 0;
    kbmLastNote = 127;
    kbmMiddleNote = 60;
    kbmReferenceNote = 69;
    kbmOctaveDegree = mapSize;

    // Linear mapping for current scale size
    kbmMapping.clear();
    kbmMapping.reserve(mapSize);
    for (int i = 0; i < mapSize; ++i)
        kbmMapping.push_back(i);

    kbmLoaded = false;
}
```

---

### Phase 2: UI Updates

#### 2.1 Show Tonic Selector for All Scale Sizes

**File:** `Source/WebView/index.html`

**Current behavior:** Tonic selector is hidden when `scaleSize !== 12`

**Find the code that hides tonic selector** (likely in JavaScript):
```javascript
// Current (approximate)
if (scaleSize === 12) {
    tonicSelector.style.display = 'block';
} else {
    tonicSelector.style.display = 'none';
}
```

**Change to:**
```javascript
// Always show tonic selector - it provides 12-TET semitone transposition
// for all scale sizes
tonicSelector.style.display = 'block';
```

#### 2.2 Update Tonic Selector Label/Tooltip

Add clarifying text for non-12 scales:

```javascript
function updateTonicSelectorLabel(scaleSize) {
    const label = document.getElementById('tonic-label');
    if (scaleSize === 12) {
        label.textContent = 'Tonic:';
        label.title = 'Select the tonic note (modal rotation)';
    } else {
        label.textContent = 'Tonic:';
        label.title = 'Transpose by semitones (anchor note for scale)';
    }
}
```

---

### Phase 3: Testing & Validation

#### 3.1 Test Cases

**Test 1: 19-note scale linear mapping**
- Load `19-31.scl`
- Tonic = C (default)
- Play MIDI 60-78: Should hear all 19 scale degrees in sequence
- Play MIDI 79: Should hear degree 0, one octave higher (2× frequency of MIDI 60)
- Play MIDI 60 and 79 together: Should sound like a perfect octave

**Test 2: 19-note scale with tonic transposition**
- Load `19-31.scl`
- Set tonic = D
- Play MIDI 62: Should be degree 0 at D4 frequency (293.66 Hz)
- Play MIDI 60: Should be degree 17 of previous octave (below D4)
- Play MIDI 62-80: Should hear all 19 degrees, then wrap

**Test 3: 7-note scale (fewer than 12)**
- Load a 7-note scale (e.g., major scale as SCL)
- Tonic = C
- Play MIDI 60-66: Should hear 7 scale degrees
- Play MIDI 67: Should be degree 0, next octave
- All 7 degrees playable, no skipped keys

**Test 4: Bohlen-Pierce (non-octave scale)**
- Load Bohlen-Pierce SCL (13 notes, 1902¢ tritave)
- Play MIDI 60-72: Should hear 13 scale degrees
- Play MIDI 73: Should be degree 0, one "tritave" higher (3× frequency)

**Test 5: KBM priority**
- Load a 19-note SCL
- Verify linear mapping works
- Load a KBM file
- Verify KBM mapping takes priority
- Reset KBM (or don't load one)
- Verify linear mapping resumes

**Test 6: 12-note scale backwards compatibility**
- Load Werckmeister III (12-note)
- Verify tonic selection still works as before
- C = degree 0 at C4 frequency
- D = degree 0 at D4 frequency

#### 3.2 True Keys Verification

For each test, verify True Keys display shows correct cent intervals:
- Degree 0 → Degree 1 should show first interval from SCL
- Intervals should match SCL file values exactly

---

## Files Modified

| File | Changes |
|------|---------|
| `Source/DSP/TuningEngine.cpp` | Fix `calculateCustomFrequency()` non-KBM path, update `setTonicNote()`, update `resetKeyboardMapping()` |
| `Source/DSP/TuningEngine.h` | Update comments/documentation |
| `Source/WebView/index.html` | Show tonic selector for all scale sizes, update tooltip |

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Breaking 12-note scale behavior | Test thoroughly with existing temperaments before release |
| KBM interaction bugs | Ensure KBM path is untouched; only modify non-KBM path |
| Edge cases (MIDI 0, MIDI 127) | Test boundary conditions explicitly |
| Negative scale degree math | Use careful modulo arithmetic with negative number handling |

---

## Success Criteria

1. ✅ All scale degrees playable for any scale size (7, 12, 19, 31, etc.)
2. ✅ Linear mapping: each MIDI key = next scale degree
3. ✅ Tonic selector visible and functional for all scales
4. ✅ Tonic selection transposes by 12-TET semitones
5. ✅ Scale period respected (octave, tritave, or any interval)
6. ✅ KBM files still work when loaded (take priority)
7. ✅ 12-note scales behave exactly as before
8. ✅ True Keys displays correct frequencies

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-23 | Initial plan |
| 1.1 | 2026-01-23 | Marked as RESOLVED - implemented in v1.13.2 |
