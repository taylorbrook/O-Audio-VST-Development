# OuariconLyrica Tuning System Overhaul

**Version:** 1.0
**Created:** 2026-01-21
**Target Version:** v2.0.0 (Major Release)
**Inspired By:** Pianoteq, Surge XT, Scale Workshop

---

## Executive Summary

This document outlines a comprehensive 4-phase improvement plan for OuariconLyrica's tuning system, drawing from industry-leading implementations in Pianoteq and Surge XT. The improvements span DSP enhancements critical for physical modeling authenticity, UI/UX visualization tools for tuning exploration, and professional features for microtuning workflows.

### Current State (v1.8.0)
- Scala SCL/KBM file loading
- Master tune (A4 reference, 400-480 Hz)
- Tonic transposition (12-note)
- Per-note pitch bend
- Basic pitch circle visualization
- MTS-ESP placeholder (not functional)

### Target State (v2.0.0)
- **All current features retained** (Scala loading, KBM, tonic, pitch bend)
- Built-in temperament preset menu (quick access, works alongside Scala loading)
- Stretch tuning for physical modeling
- Unison detuning (multiple strings)
- Full MTS-ESP client support
- Surge-style visualization modes (Polar, True Keys, Modal Rotation, Interval Matrix)
- Factory tuning library
- Scale generators and export tools

---

## Phase 1: DSP Foundation

**Priority:** Critical
**Estimated Complexity:** Medium
**Dependencies:** None (builds on existing TuningEngine)

### 1.1 Stretch Tuning

**Rationale:** Physical modeling instruments require octave stretching to compensate for string inharmonicity. Without this, upper and lower registers sound "off" to trained ears. Pianoteq considers this essential - "A unique feature of Pianoteq is that tuning does not follow a pre-computed frequency table... but takes into account the inharmonicity of the strings."

**Implementation:**

```cpp
// TuningEngine additions
class TuningEngine {
public:
    // New stretch tuning parameters
    void setOctaveStretch(float stretch);      // 0.95 - 1.25 (1.0 = normal)
    float getOctaveStretch() const;

    // Stretch affects frequency calculation
    // Formula: stretchedFreq = baseFreq * pow(2.0, (semitones * stretchFactor) / 12.0)

private:
    float octaveStretch = 1.0f;  // Default: no extra stretch

    // Inharmonicity compensation (per-register)
    float calculateStretchedFrequency(int midiNote, double baseFreq) const;
};
```

**Parameters:**
| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Octave Stretch | `octaveStretch` | 0.95 - 1.25 | 1.0 | Stretch factor for octave ratios |

**UI Location:** Tuning Tab → Advanced section

**Technical Notes:**
- Stretch affects the `rebuildFrequencyTable()` calculation
- Applied after scale/temperament but before pitch bend
- Higher values = wider octaves (more stretch in upper register)
- Typical piano tuning uses ~1.05-1.15 for upper octaves

---

### 1.2 Built-in Temperament Presets (Additional Menu)

**Rationale:** Users shouldn't need to find and load Scala files for common historical temperaments. Pianoteq and Surge both provide instant access to these presets.

**IMPORTANT: This is an ADDITION, not a replacement.** The existing Scala file loading (Load .scl / Load .kbm buttons) remains fully functional. The new temperament preset menu provides quick access to common tunings without replacing the ability to load custom Scala files.

**Workflow:**
1. User can select a built-in preset from the dropdown → instantly applies
2. User can still click "Load .scl" to load any custom Scala file → overrides preset
3. When a custom Scala file is loaded, the dropdown shows "Custom (filename.scl)"
4. User can switch back to a built-in preset at any time

**Implementation:**

```cpp
// New enum for built-in temperament presets
enum class BuiltInTemperament {
    Equal12TET = 0,      // Standard equal temperament
    Pythagorean,         // Pure fifths (3/2), wolf fifth on G#-Eb
    Zarlino,             // Just intonation (5-limit)
    MeantoneQuarter,     // Quarter-comma meantone
    WerckmeisterIII,     // Andreas Werckmeister's famous temperament
    KirnbergerIII,       // Johann Kirnberger's temperament
    Vallotti,            // Francesco Vallotti's temperament
    WellTempered,        // Generic well temperament
    JustIntonation,      // Pure ratios based on harmonic series
    BohlenPierce,        // Non-octave scale (3:1 ratio)
    Custom               // User-loaded Scala file (set automatically when .scl loaded)
};

// TuningEngine additions
void setBuiltInTemperament(BuiltInTemperament temperament);
BuiltInTemperament getBuiltInTemperament() const;
juce::String getTemperamentName() const;
```

**Embedded Temperament Data:**

```cpp
// Cents from unison for each temperament (stored in TuningEngine.cpp)
static const std::array<double, 12> WERCKMEISTER_III = {
    0.0,      // C
    90.225,   // C#
    192.18,   // D
    294.135,  // D#/Eb
    390.225,  // E
    498.045,  // F
    588.27,   // F#
    696.09,   // G
    792.18,   // G#
    888.27,   // A
    996.09,   // A#/Bb
    1092.18   // B
};

static const std::array<double, 12> PYTHAGOREAN = {
    0.0,      // C
    113.685,  // C#
    203.91,   // D
    294.135,  // Eb
    407.82,   // E
    498.045,  // F
    611.73,   // F#
    701.955,  // G
    815.64,   // G#
    905.865,  // A
    996.09,   // Bb
    1109.775  // B
};

// ... similar for other temperaments
```

**Parameters:**
| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Temperament Preset | `temperamentPreset` | Choice (11 options) | Equal 12-TET | Quick-access temperament presets |

**UI Location:** Tuning Tab → Temperament Preset dropdown (above existing controls)

**Technical Notes:**
- When switching preset, internally calls `setCustomIntervals()` with embedded data
- Mode automatically switches to `Mode::Scala` when non-12TET preset selected
- Tonic selector remains functional for transposition
- **Loading a custom Scala file automatically sets preset to "Custom (filename)"**
- **Existing "Load .scl" and "Load .kbm" buttons remain unchanged**
- Preset state saved/restored with plugin state

---

### 1.3 Phase 1 UI Updates

**Tuning Tab Layout Changes:**

```
┌─────────────────────────────────────────────────────────────────┐
│  TUNING                                                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Preset: [▼ Equal 12-TET        ]   Tonic: ◀ C ▶       [NEW]   │
│          ├─ Equal 12-TET                                        │
│          ├─ Pythagorean                                         │
│          ├─ Werckmeister III                                    │
│          ├─ Meantone (1/4 comma)                                │
│          ├─ ... (more presets)                                  │
│          └─ Custom (user-loaded.scl)  ← shown when .scl loaded  │
│                                                                  │
│  ┌─────────────────┐  ┌────────────────────────────────────┐   │
│  │ Interval List   │  │         Pitch Circle               │   │
│  │ C    0.0¢       │  │            (existing)              │   │
│  │ C#   100.0¢     │  │                                    │   │
│  │ D    200.0¢     │  │                                    │   │
│  │ ...             │  │                                    │   │
│  └─────────────────┘  └────────────────────────────────────┘   │
│                                                                  │
│  ─── Advanced ───────────────────────────────────────────────   │
│  Master Tune: [====●====] 440 Hz                                │
│  Octave Stretch: [====●====] 1.00                    [NEW]      │
│  Pitch Bend Range: [====●====] ±2 semitones                     │
│                                                                  │
│  [Load .scl] [Load .kbm] [Save Tuning]    ← UNCHANGED (still   │
│                                              available for      │
│                                              custom Scala files)│
└─────────────────────────────────────────────────────────────────┘
```

**Interaction Flow:**
1. **Select preset** → Immediately applies built-in temperament data
2. **Load .scl file** → Loads custom tuning, preset dropdown shows "Custom (filename.scl)"
3. **Select preset again** → Switches back to built-in, custom file data cleared
4. **Both methods coexist** - presets are convenience, Scala loading is power-user feature

---

## Phase 2: UI Visualization

**Priority:** High
**Estimated Complexity:** Medium-High
**Dependencies:** Phase 1 complete

### 2.1 Enhanced Interval Display

**Rationale:** Show deviation from equal temperament for each note, helping users understand the character of loaded tunings.

**Implementation:**

```javascript
// In index.html JavaScript
function updateIntervalListUI() {
    const total = currentIntervals.length;
    let html = '';

    // Header with scale name
    html += `<div class="interval-list-header">${scaleName} (${total} notes)</div>`;

    // Tonic selector (existing)
    if (total === 12) {
        html += generateTonicSelector();
    }

    // Enhanced interval rows with deviation
    for (let i = 0; i < total; i++) {
        const noteName = getDegreeLabel(i, total);
        const cents = currentIntervals[i];
        const equalCents = (i * 1200 / total);
        const deviation = cents - equalCents;
        const deviationClass = Math.abs(deviation) < 1 ? 'pure' :
                              deviation > 0 ? 'sharp' : 'flat';

        html += `
            <div class="interval-row">
                <span class="note-name">${noteName}</span>
                <span class="cents-value">${cents.toFixed(1)}¢</span>
                <span class="deviation ${deviationClass}">
                    ${deviation >= 0 ? '+' : ''}${deviation.toFixed(1)}¢
                </span>
            </div>
        `;
    }

    intervalList.innerHTML = html;
}
```

**Styling:**
```css
.deviation.pure { color: #4CAF50; }  /* Green - pure interval */
.deviation.sharp { color: #FF9800; } /* Orange - sharp */
.deviation.flat { color: #2196F3; }  /* Blue - flat */
```

---

### 2.2 Polar/Radial Tone Wheel

**Rationale:** Visual representation of pitch relationships as Surge XT provides. Shows scale structure at a glance.

**Implementation:**

```javascript
// New visualization mode toggle
let visualMode = 'circle';  // 'circle' | 'polar' | 'matrix'

function drawPolarWheel(canvas) {
    const ctx = canvas.getContext('2d');
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const maxRadius = Math.min(centerX, centerY) - 20;

    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Draw concentric reference circles (every 200 cents)
    ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    for (let cents = 200; cents <= 1200; cents += 200) {
        const r = (cents / 1200) * maxRadius;
        ctx.beginPath();
        ctx.arc(centerX, centerY, r, 0, Math.PI * 2);
        ctx.stroke();
    }

    // Draw scale degree nodes
    const total = currentIntervals.length;
    for (let i = 0; i < total; i++) {
        const angle = (i / total) * Math.PI * 2 - Math.PI / 2;  // Start at top
        const cents = currentIntervals[i];
        const r = (cents / 1200) * maxRadius;  // Radial = cents value

        const x = centerX + Math.cos(angle) * r;
        const y = centerY + Math.sin(angle) * r;

        // Node
        ctx.fillStyle = activeNotes.has(i) ? '#FFD700' : '#8B7355';
        ctx.beginPath();
        ctx.arc(x, y, 8, 0, Math.PI * 2);
        ctx.fill();

        // Label
        ctx.fillStyle = '#FFF';
        ctx.font = '11px Garamond';
        ctx.textAlign = 'center';
        ctx.fillText(getDegreeLabel(i, total), x, y - 12);
    }
}
```

**UI Toggle:** Add view mode buttons above the visualization area:
```html
<div class="viz-mode-toggle">
    <button class="viz-btn active" data-mode="circle">Circle</button>
    <button class="viz-btn" data-mode="polar">Polar</button>
    <button class="viz-btn" data-mode="matrix">Matrix</button>
</div>
```

---

### 2.3 True Keys Mode

**Rationale:** Surge XT's True Keys shows real-time cent distances between currently held notes. Essential for understanding chord voicings in microtuned contexts.

**Implementation:**

```javascript
// Track currently held MIDI notes (from C++ via native function)
let heldNotes = new Set();  // MIDI note numbers
let heldNoteFrequencies = new Map();  // MIDI note -> frequency

// Called from C++ when notes change
window.updateHeldNotes = function(notes, frequencies) {
    heldNotes = new Set(notes);
    heldNoteFrequencies = new Map();
    for (let i = 0; i < notes.length; i++) {
        heldNoteFrequencies.set(notes[i], frequencies[i]);
    }

    if (visualMode === 'truekeys') {
        updateTrueKeysDisplay();
    }
};

function updateTrueKeysDisplay() {
    const container = document.getElementById('true-keys-display');
    if (heldNotes.size < 2) {
        container.innerHTML = '<div class="tk-hint">Hold 2+ notes to see intervals</div>';
        return;
    }

    // Sort held notes
    const sortedNotes = Array.from(heldNotes).sort((a, b) => a - b);

    let html = '<div class="tk-header">Held Notes - Intervals</div>';
    html += '<div class="tk-grid">';

    // Create interval matrix for held notes
    for (let i = 0; i < sortedNotes.length; i++) {
        for (let j = i + 1; j < sortedNotes.length; j++) {
            const note1 = sortedNotes[i];
            const note2 = sortedNotes[j];
            const freq1 = heldNoteFrequencies.get(note1);
            const freq2 = heldNoteFrequencies.get(note2);

            // Calculate cents between notes
            const cents = 1200 * Math.log2(freq2 / freq1);
            const name1 = midiToNoteName(note1);
            const name2 = midiToNoteName(note2);

            html += `
                <div class="tk-interval">
                    <span class="tk-notes">${name1} → ${name2}</span>
                    <span class="tk-cents">${cents.toFixed(1)}¢</span>
                </div>
            `;
        }
    }

    html += '</div>';
    container.innerHTML = html;
}
```

**C++ Integration Required:**
```cpp
// In PluginEditor.cpp - timerCallback()
void OuariconLyricaAudioProcessorEditor::timerCallback()
{
    // Existing MIDI polling code...

    // Send held notes to WebView for True Keys display
    auto& synth = processorRef.getSynthesiser();
    juce::Array<int> heldNotes;
    juce::Array<double> frequencies;

    for (int i = 0; i < synth.getNumVoices(); ++i) {
        auto* voice = synth.getVoice(i);
        if (voice->isVoiceActive()) {
            int midiNote = voice->getCurrentlyPlayingNote();
            double freq = processorRef.getTuningEngine().getFrequency(midiNote);
            heldNotes.add(midiNote);
            frequencies.add(freq);
        }
    }

    // Call JavaScript function
    juce::String js = "window.updateHeldNotes && window.updateHeldNotes([";
    for (int i = 0; i < heldNotes.size(); ++i) {
        if (i > 0) js += ",";
        js += juce::String(heldNotes[i]);
    }
    js += "],[";
    for (int i = 0; i < frequencies.size(); ++i) {
        if (i > 0) js += ",";
        js += juce::String(frequencies[i], 4);
    }
    js += "]);";

    webView->evaluateJavascript(js);
}
```

---

### 2.4 Modal Rotation Analysis

**Rationale:** Shows how the scale's intervallic structure changes when starting from different degrees. Essential for understanding modal possibilities of a tuning.

**Implementation:**

```javascript
function drawModalRotationMatrix() {
    const container = document.getElementById('rotation-matrix');
    const total = currentIntervals.length;

    let html = '<table class="rotation-table">';

    // Header row with degree numbers
    html += '<tr><th>Mode</th>';
    for (let i = 0; i < total; i++) {
        html += `<th>${i + 1}</th>`;
    }
    html += '</tr>';

    // Each row = starting from different scale degree
    for (let startDegree = 0; startDegree < total; startDegree++) {
        const modeName = getDegreeLabel(startDegree, total);
        html += `<tr><td class="mode-name">${modeName}</td>`;

        let prevCents = 0;
        for (let step = 0; step < total; step++) {
            const actualDegree = (startDegree + step) % total;
            let cents = currentIntervals[actualDegree];

            // Adjust for wrapping
            if (actualDegree < startDegree) {
                cents += 1200;  // Add octave
            }

            // Calculate interval from previous note
            const interval = step === 0 ? 0 : cents - prevCents;
            prevCents = cents;

            // Color code by interval size
            const intervalClass = getIntervalClass(interval);

            html += `<td class="interval-cell ${intervalClass}" title="${interval.toFixed(1)}¢">
                ${interval.toFixed(0)}
            </td>`;
        }
        html += '</tr>';
    }

    html += '</table>';
    container.innerHTML = html;
}

function getIntervalClass(cents) {
    // Color code intervals
    if (cents < 50) return 'micro';           // Quartertone or less
    if (cents < 150) return 'semitone';       // ~100 cents
    if (cents < 250) return 'whole';          // ~200 cents
    if (cents < 350) return 'minor-third';    // ~300 cents
    if (cents < 450) return 'major-third';    // ~400 cents
    return 'large';                           // Larger intervals
}
```

**Styling:**
```css
.rotation-table {
    border-collapse: collapse;
    font-size: 11px;
}
.rotation-table th, .rotation-table td {
    padding: 4px 6px;
    border: 1px solid rgba(255,255,255,0.2);
    text-align: center;
}
.mode-name {
    font-weight: bold;
    background: rgba(139, 115, 85, 0.3);
}
.interval-cell.micro { background: rgba(156, 39, 176, 0.4); }
.interval-cell.semitone { background: rgba(33, 150, 243, 0.4); }
.interval-cell.whole { background: rgba(76, 175, 80, 0.4); }
.interval-cell.minor-third { background: rgba(255, 152, 0, 0.4); }
.interval-cell.major-third { background: rgba(244, 67, 54, 0.4); }
.interval-cell.large { background: rgba(121, 85, 72, 0.4); }
```

---

### 2.5 Phase 2 UI Layout

**New Tuning Tab with Visualization Modes:**

```
┌─────────────────────────────────────────────────────────────────┐
│  TUNING                                                          │
├─────────────────────────────────────────────────────────────────┤
│  Temperament: [▼ Werckmeister III    ]   Tonic: ◀ C ▶          │
├─────────────────────────────────────────────────────────────────┤
│  View: [Circle] [Polar] [Matrix] [True Keys] [Rotation]         │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌────────────────────────────────────┐   │
│  │ Intervals       │  │      Visualization Area            │   │
│  │ C   0.0¢   +0   │  │  (content changes based on mode)   │   │
│  │ C#  90.2¢  -9.8 │  │                                    │   │
│  │ D   192.2¢ -7.8 │  │                                    │   │
│  │ ...             │  │                                    │   │
│  └─────────────────┘  └────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│  ─── Advanced ───────────────────────────────────────────────   │
│  Master Tune: 440 Hz    Octave Stretch: 1.00    PB Range: ±2   │
│  [Load .scl] [Load .kbm] [Export HTML]                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## Phase 3: Advanced Features

**Priority:** Medium
**Estimated Complexity:** High
**Dependencies:** Phase 1 and 2 complete

### 3.1 MTS-ESP Client Implementation

**Rationale:** MTS-ESP is the modern standard for centralized microtuning control. Completing the placeholder allows OuariconLyrica to participate in microtuned sessions with other instruments.

**Implementation:**

```cpp
// TuningEngine.h additions
#include "libMTSClient.h"

class TuningEngine {
public:
    // MTS-ESP methods
    bool connectMTSClient();
    void disconnectMTSClient();
    bool isMTSConnected() const;

    // Query tuning options
    enum class MTSQueryMode {
        NoteOnOnly,    // Query pitch only at note-on (default)
        Continuous     // Query continuously for pitch bends
    };
    void setMTSQueryMode(MTSQueryMode mode);

private:
    MTSClient* mtsClient = nullptr;
    MTSQueryMode mtsQueryMode = MTSQueryMode::NoteOnOnly;

    double getMTSFrequency(int midiNote, int midiChannel);
};
```

```cpp
// TuningEngine.cpp - MTS-ESP implementation
bool TuningEngine::connectMTSClient()
{
    if (mtsClient != nullptr) return true;  // Already connected

    mtsClient = MTS_RegisterClient();
    if (mtsClient != nullptr) {
        mtsSynthClientConnected = true;
        currentMode.store(Mode::MTSESP, std::memory_order_relaxed);
        DBG("TuningEngine: Connected to MTS-ESP master");
        return true;
    }

    DBG("TuningEngine: No MTS-ESP master found");
    return false;
}

void TuningEngine::disconnectMTSClient()
{
    if (mtsClient != nullptr) {
        MTS_DeregisterClient(mtsClient);
        mtsClient = nullptr;
        mtsSynthClientConnected = false;

        // Fall back to previous mode
        if (scalaFileLoaded)
            currentMode.store(Mode::Scala, std::memory_order_relaxed);
        else
            currentMode.store(Mode::TwelveTET, std::memory_order_relaxed);
    }
}

double TuningEngine::getMTSFrequency(int midiNote, int midiChannel)
{
    if (mtsClient == nullptr) return calculate12TETFrequency(midiNote);

    // Check if note should be filtered (unmapped in MTS-ESP)
    if (MTS_ShouldFilterNote(mtsClient, midiNote, midiChannel))
        return -1.0;  // Signal to skip this note

    return MTS_NoteToFrequency(mtsClient, midiNote, midiChannel);
}
```

**Build Integration:**
- Add `libMTSClient` to CMakeLists.txt
- Conditional compilation for platforms without MTS-ESP support

---

### 3.2 Scale Generators

**Rationale:** Allow users to create scales mathematically without loading external files.

**Implementation:**

```cpp
// ScaleGenerator.h - New utility class
class ScaleGenerator {
public:
    // Equal divisions of the octave (EDO)
    static std::vector<double> generateEDO(int divisions);

    // Equal divisions of any interval
    static std::vector<double> generateEqualDivision(double intervalCents, int divisions);

    // Just intonation from harmonic series
    static std::vector<double> generateHarmonicSeries(int startHarmonic, int endHarmonic);

    // Subharmonic series
    static std::vector<double> generateSubharmonicSeries(int startSubharmonic, int count);

    // Rank-2 temperament (generator + period)
    static std::vector<double> generateRank2(double generator, double period, int count);

    // Combination Product Set (Euler-Fokker genus)
    static std::vector<double> generateCPS(const std::vector<int>& factors, int choose);
};
```

```cpp
// ScaleGenerator.cpp
std::vector<double> ScaleGenerator::generateEDO(int divisions)
{
    std::vector<double> intervals;
    intervals.push_back(0.0);  // Unison

    double step = 1200.0 / divisions;
    for (int i = 1; i < divisions; ++i) {
        intervals.push_back(i * step);
    }

    return intervals;
}

std::vector<double> ScaleGenerator::generateHarmonicSeries(int start, int end)
{
    std::vector<double> intervals;
    intervals.push_back(0.0);  // Unison (1/1)

    for (int h = start + 1; h <= end; ++h) {
        double ratio = static_cast<double>(h) / start;
        double cents = 1200.0 * std::log2(ratio);

        // Reduce to within octave
        while (cents >= 1200.0) cents -= 1200.0;

        intervals.push_back(cents);
    }

    // Sort by cents value
    std::sort(intervals.begin() + 1, intervals.end());

    return intervals;
}
```

**UI Integration:**
```html
<div class="scale-generator-panel">
    <h4>Generate Scale</h4>

    <div class="gen-option">
        <label>Equal Divisions (EDO):</label>
        <input type="number" id="edo-divisions" value="12" min="2" max="53">
        <button onclick="generateEDO()">Generate</button>
    </div>

    <div class="gen-option">
        <label>Harmonic Series:</label>
        <input type="number" id="harm-start" value="8" min="1" max="32">
        to
        <input type="number" id="harm-end" value="16" min="2" max="64">
        <button onclick="generateHarmonic()">Generate</button>
    </div>

    <div class="gen-option">
        <label>Rank-2 Temperament:</label>
        <input type="number" id="r2-generator" value="700" step="0.1"> ¢ generator
        <input type="number" id="r2-count" value="12" min="5" max="31"> notes
        <button onclick="generateRank2()">Generate</button>
    </div>
</div>
```

---

### 3.3 Factory Tuning Library

**Rationale:** Bundle commonly-used tunings so users can explore without finding external files.

**Implementation:**

```cpp
// EmbeddedTunings.h
namespace EmbeddedTunings {

    struct TuningDefinition {
        const char* name;
        const char* category;
        const char* description;
        std::array<double, 12> intervals;  // For 12-note scales
        // OR
        const char* sclData;  // For arbitrary scales
    };

    // Categories
    namespace Historical {
        extern const TuningDefinition WERCKMEISTER_III;
        extern const TuningDefinition KIRNBERGER_III;
        extern const TuningDefinition VALLOTTI;
        extern const TuningDefinition PYTHAGOREAN;
        extern const TuningDefinition MEANTONE_QUARTER;
        extern const TuningDefinition MEANTONE_THIRD;
        extern const TuningDefinition YOUNG_1799;
    }

    namespace JustIntonation {
        extern const TuningDefinition PTOLEMY_INTENSE;
        extern const TuningDefinition PARTCH_43;
        extern const TuningDefinition HARMONIC_12;
    }

    namespace EqualDivisions {
        extern const TuningDefinition EDO_17;
        extern const TuningDefinition EDO_19;
        extern const TuningDefinition EDO_22;
        extern const TuningDefinition EDO_31;
        extern const TuningDefinition EDO_53;
    }

    namespace NonOctave {
        extern const TuningDefinition BOHLEN_PIERCE;
        extern const TuningDefinition CARLOS_ALPHA;
        extern const TuningDefinition CARLOS_BETA;
        extern const TuningDefinition CARLOS_GAMMA;
    }

    namespace World {
        extern const TuningDefinition ARABIC_MAQAM;
        extern const TuningDefinition TURKISH_MAKAM;
        extern const TuningDefinition INDIAN_22_SHRUTI;
        extern const TuningDefinition GAMELAN_SLENDRO;
        extern const TuningDefinition GAMELAN_PELOG;
    }

    // Get all tunings for UI browser
    std::vector<TuningDefinition> getAllTunings();
    std::vector<TuningDefinition> getTuningsByCategory(const char* category);
}
```

**UI: Tuning Library Browser**
```html
<div class="tuning-library-browser">
    <h4>Tuning Library</h4>

    <select id="tuning-category">
        <option value="all">All Categories</option>
        <option value="historical">Historical Temperaments</option>
        <option value="just">Just Intonation</option>
        <option value="edo">Equal Divisions</option>
        <option value="nonoctave">Non-Octave</option>
        <option value="world">World Scales</option>
    </select>

    <div class="tuning-list" id="tuning-list">
        <!-- Populated by JavaScript -->
    </div>

    <div class="tuning-preview" id="tuning-preview">
        <!-- Shows description and interval preview on hover -->
    </div>
</div>
```

---

### 3.4 Export Functionality

**Rationale:** Allow users to export their tunings for use in other software or documentation.

**Implementation:**

```cpp
// TuningExporter.h
class TuningExporter {
public:
    // Export current tuning as Scala files
    static juce::String exportAsSCL(const TuningEngine& engine);
    static juce::String exportAsKBM(const TuningEngine& engine);

    // Export as HTML documentation (like Surge)
    static juce::String exportAsHTML(const TuningEngine& engine);

    // Export as frequency table
    static juce::String exportAsFrequencyTable(const TuningEngine& engine, int startNote = 0, int endNote = 127);
};
```

```cpp
// HTML export (inspired by Surge)
juce::String TuningExporter::exportAsHTML(const TuningEngine& engine)
{
    juce::String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>)" + engine.getActiveTuningName() + R"( - Tuning Documentation</title>
    <style>
        body { font-family: Georgia, serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        h1 { color: #8B7355; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: right; }
        th { background: #f5f5f5; }
        .note-name { text-align: left; font-weight: bold; }
    </style>
</head>
<body>
    <h1>)" + engine.getActiveTuningName() + R"(</h1>
    <p>Generated by OuariconLyrica</p>

    <h2>Scale Degrees</h2>
    <table>
        <tr><th>Degree</th><th>Cents</th><th>Ratio</th></tr>
)";

    auto intervals = engine.getIntervals();
    for (size_t i = 0; i < intervals.size(); ++i) {
        double cents = intervals[i];
        double ratio = std::pow(2.0, cents / 1200.0);
        html += "<tr><td>" + juce::String(i) + "</td>";
        html += "<td>" + juce::String(cents, 3) + "</td>";
        html += "<td>" + juce::String(ratio, 6) + "</td></tr>\n";
    }

    html += R"(
    </table>

    <h2>MIDI Note to Frequency Mapping</h2>
    <table>
        <tr><th class="note-name">Note</th><th>MIDI</th><th>Frequency (Hz)</th></tr>
)";

    for (int note = 21; note <= 108; ++note) {  // Piano range
        double freq = engine.getFrequency(note);
        juce::String noteName = getMIDINoteName(note);
        html += "<tr><td class=\"note-name\">" + noteName + "</td>";
        html += "<td>" + juce::String(note) + "</td>";
        html += "<td>" + juce::String(freq, 3) + "</td></tr>\n";
    }

    html += R"(
    </table>
</body>
</html>
)";

    return html;
}
```

---

## Phase 4: Professional Polish

**Priority:** Medium-Low
**Estimated Complexity:** High
**Dependencies:** Phases 1-3 complete

### 4.1 Unison Detuning (Multi-String)

**Rationale:** Real harps and pianos have multiple strings per note with slight detuning. This creates the rich, alive quality of acoustic instruments. Pianoteq implements this as "Unison Width" and "Unison Balance".

**Implementation:**

This is a significant DSP change requiring voice architecture modifications.

```cpp
// WaveguideString.h additions
class WaveguideString {
public:
    // Unison parameters
    void setUnisonCount(int count);       // 1-3 strings
    void setUnisonWidth(float width);     // 0.0-2.0 (cents spread)
    void setUnisonBalance(float balance); // -1.0 to +1.0

private:
    int unisonCount = 1;
    float unisonWidth = 0.0f;
    float unisonBalance = 0.0f;

    // Per-string state (up to 3)
    struct StringState {
        double frequency;
        float detuneOffset;  // in cents
        juce::dsp::DelayLine<float> upperRail;
        juce::dsp::DelayLine<float> lowerRail;
        // ... filters
    };
    std::array<StringState, 3> strings;

    void updateUnisonDetuning();
};
```

```cpp
// Unison detuning calculation
void WaveguideString::updateUnisonDetuning()
{
    if (unisonCount == 1) {
        strings[0].detuneOffset = 0.0f;
        return;
    }

    // Width in cents
    float halfWidth = unisonWidth * 0.5f;

    if (unisonCount == 2) {
        strings[0].detuneOffset = -halfWidth;
        strings[1].detuneOffset = halfWidth;
    }
    else if (unisonCount == 3) {
        // Balance controls middle string position
        // -1 = middle at low, 0 = middle centered, +1 = middle at high
        float middlePos = unisonBalance * halfWidth;

        strings[0].detuneOffset = -halfWidth;
        strings[1].detuneOffset = middlePos;
        strings[2].detuneOffset = halfWidth;
    }

    // Update delay line lengths for each string
    for (int i = 0; i < unisonCount; ++i) {
        double detuned = currentFrequency * std::pow(2.0, strings[i].detuneOffset / 1200.0);
        strings[i].frequency = detuned;
        // Recalculate delay...
    }
}
```

**Parameters:**
| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Unison Strings | `unisonCount` | 1-3 | 1 | Number of strings per note |
| Unison Width | `unisonWidth` | 0.0-2.0 cents | 0.5 | Frequency spread between strings |
| Unison Balance | `unisonBalance` | -1.0 to +1.0 | 0.0 | Middle string position |

**CPU Impact:** 2x-3x per voice when enabled

**UI Location:** String Properties section (new "Unison" subsection)

---

### 4.2 Apply Tuning Options

**Rationale:** Surge XT offers two tuning application modes: at MIDI input (modulation in 12-TET amounts) or after modulation (pitch movements follow scale intervals). This matters for pitch bends and portamento.

**Implementation:**

```cpp
// TuningEngine.h additions
enum class TuningApplicationPoint {
    AtMIDIInput,      // Traditional: tune note, then apply 12-TET modulation
    AfterModulation   // Advanced: modulation follows scale intervals
};

class TuningEngine {
public:
    void setApplicationPoint(TuningApplicationPoint point);
    TuningApplicationPoint getApplicationPoint() const;

    // Different frequency calculation based on mode
    double getFrequencyWithModulation(int midiNote, float modulation);

private:
    TuningApplicationPoint applicationPoint = TuningApplicationPoint::AtMIDIInput;
};
```

```cpp
// Frequency calculation with modulation
double TuningEngine::getFrequencyWithModulation(int midiNote, float modulation)
{
    if (applicationPoint == TuningApplicationPoint::AtMIDIInput) {
        // Traditional: get tuned base frequency, apply 12-TET modulation
        double baseFreq = getFrequency(midiNote);
        return baseFreq * std::pow(2.0, modulation / 12.0);
    }
    else {
        // After modulation: modulation follows scale intervals
        // Calculate fractional scale position
        double scaleDegrees = static_cast<double>(getScaleDegrees());
        double fractionalNote = midiNote + (modulation * scaleDegrees / 12.0);

        int lowerNote = static_cast<int>(std::floor(fractionalNote));
        int upperNote = lowerNote + 1;
        double frac = fractionalNote - lowerNote;

        double lowerFreq = getFrequency(lowerNote);
        double upperFreq = getFrequency(upperNote);

        // Interpolate in log-frequency space
        return lowerFreq * std::pow(upperFreq / lowerFreq, frac);
    }
}
```

**UI:** Checkbox in Tuning Tab → Advanced section:
```html
<label class="tuning-option">
    <input type="checkbox" id="apply-after-mod">
    Apply tuning after modulation (pitch bend follows scale)
</label>
```

---

### 4.3 Interactive Interval Matrix

**Rationale:** Surge's interval matrix allows click-and-drag to retune intervals. Adds hands-on tuning exploration.

**Implementation:**

```javascript
function drawInteractiveIntervalMatrix() {
    const container = document.getElementById('interval-matrix');
    const total = currentIntervals.length;

    let html = '<table class="interval-matrix">';

    // Header with note names
    html += '<tr><th></th>';
    for (let i = 0; i < total; i++) {
        html += `<th>${getDegreeLabel(i, total)}</th>`;
    }
    html += '</tr>';

    // Matrix cells showing interval between row and column
    for (let row = 0; row < total; row++) {
        html += `<tr><th>${getDegreeLabel(row, total)}</th>`;

        for (let col = 0; col < total; col++) {
            let interval = currentIntervals[col] - currentIntervals[row];
            if (interval < 0) interval += 1200;  // Wrap to positive

            const cellId = `cell-${row}-${col}`;
            html += `<td class="matrix-cell"
                        id="${cellId}"
                        data-row="${row}"
                        data-col="${col}"
                        data-interval="${interval}"
                        onmousedown="startIntervalDrag(event, ${row}, ${col})"
                        title="Click and drag to adjust">
                ${interval.toFixed(0)}
            </td>`;
        }
        html += '</tr>';
    }

    html += '</table>';
    container.innerHTML = html;
}

// Drag-to-retune functionality
let dragState = { active: false, row: 0, col: 0, startY: 0, startInterval: 0 };

function startIntervalDrag(event, row, col) {
    if (row === col) return;  // Can't retune unison

    dragState = {
        active: true,
        row: row,
        col: col,
        startY: event.clientY,
        startInterval: currentIntervals[col]
    };

    document.addEventListener('mousemove', handleIntervalDrag);
    document.addEventListener('mouseup', endIntervalDrag);
}

function handleIntervalDrag(event) {
    if (!dragState.active) return;

    const deltaY = dragState.startY - event.clientY;
    const centsDelta = deltaY * 0.5;  // 0.5 cents per pixel

    // Shift for fine adjustment
    const modifier = event.shiftKey ? 0.1 : 1.0;

    const newCents = dragState.startInterval + (centsDelta * modifier);
    currentIntervals[dragState.col] = Math.max(0, Math.min(1200, newCents));

    // Update display
    drawInteractiveIntervalMatrix();
    updateIntervalListUI();
    updatePitchCircle();

    // Send to C++
    sendIntervalsToNative();
}

async function sendIntervalsToNative() {
    try {
        await Juce.getNativeFunction('setCustomIntervals')(
            JSON.stringify(currentIntervals),
            'Custom (Modified)'
        );
    } catch (e) {
        console.error('Failed to send intervals:', e);
    }
}
```

---

## Implementation Timeline

### Phase 1: DSP Foundation ✅ COMPLETE (v1.9.0)
**Completed:** 2026-01-21
| Task | Status | Files Modified |
|------|--------|----------------|
| Stretch tuning parameter | ✅ | TuningEngine.h/.cpp, PluginProcessor |
| Built-in preset data (embedded) | ✅ | TuningEngine.cpp (new static data) |
| Preset dropdown UI (additive) | ✅ | index.html, PluginProcessor |
| Preset ↔ Scala file interaction | ✅ | TuningEngine, index.html (JS logic) |
| UI layout updates | ✅ | index.html (CSS/HTML) |

### Phase 2: UI Visualization ✅ COMPLETE (v1.10.0)
**Completed:** 2026-01-21
| Task | Status | Files Modified |
|------|--------|----------------|
| Enhanced interval display | ✅ | index.html (JS/CSS) |
| Polar/Radial tone wheel | ✅ | index.html (canvas drawing) |
| True Keys mode | ✅ | index.html, PluginEditor.cpp |
| Modal rotation matrix | ✅ | index.html (JS/CSS) |
| View mode switching | ✅ | index.html (UI logic) |

### Phase 3: Advanced Features
**Estimated Duration:** 3-4 weeks
| Task | Complexity | Files Modified |
|------|------------|----------------|
| MTS-ESP client | High | TuningEngine, CMakeLists, new deps |
| Scale generators | Medium | New ScaleGenerator class, UI |
| Factory tuning library | Medium | New EmbeddedTunings, UI browser |
| Export functionality | Medium | New TuningExporter class, UI |

### Phase 4: Professional Polish
**Estimated Duration:** 3-4 weeks
| Task | Complexity | Files Modified |
|------|------------|----------------|
| Unison detuning | High | WaveguideString (major changes) |
| Apply tuning options | Medium | TuningEngine, voice processing |
| Interactive interval matrix | Medium | index.html (complex JS) |

---

## Dependencies and Requirements

### External Libraries
| Library | Phase | Purpose | License |
|---------|-------|---------|---------|
| libMTSClient | 3 | MTS-ESP support | MIT |
| surge-tuning-library (optional) | 3 | Enhanced Scala parsing | MIT |

### Build System Changes
- Phase 3: Add libMTSClient submodule
- Phase 3: Conditional compilation flags for MTS-ESP

### Breaking Changes
- **None planned** - all features are additive
- Existing presets will continue to work
- Default behavior unchanged (12-TET, no stretch, single string)

---

## Testing Requirements

### Phase 1 Testing
- [ ] Stretch tuning: verify octaves sound correctly stretched
- [ ] Built-in presets: compare against reference Scala files
- [ ] Tonic transposition works with all presets
- [ ] No audio artifacts when switching presets
- [ ] Scala file loading still works (Load .scl button)
- [ ] Loading .scl file updates preset dropdown to "Custom (filename)"
- [ ] Selecting a preset after loading .scl correctly overrides custom tuning
- [ ] Preset selection persists across plugin reload

### Phase 2 Testing
- [ ] All visualization modes render correctly
- [ ] True Keys updates in real-time with MIDI input
- [ ] Modal rotation shows correct intervals
- [ ] No performance impact from visualization updates

### Phase 3 Testing
- [ ] MTS-ESP connects to common masters (Oddsound, Surge)
- [ ] Scale generators produce mathematically correct results
- [ ] Factory library loads all tunings without errors
- [ ] Export produces valid SCL/KBM/HTML files

### Phase 4 Testing
- [ ] Unison detuning produces expected beating
- [ ] CPU impact of unison within acceptable limits
- [ ] Apply tuning option affects pitch bend correctly
- [ ] Interactive matrix allows smooth retuning

---

## Success Criteria

### Phase 1 Complete When:
- Users can select from 10+ built-in temperaments without loading files
- Octave stretch parameter affects physical modeling realism
- All existing functionality preserved

### Phase 2 Complete When:
- 5 visualization modes available (Circle, Polar, Matrix, True Keys, Rotation)
- Real-time MIDI feedback in all modes
- Users report improved understanding of their tunings

### Phase 3 Complete When:
- MTS-ESP works with at least 3 common master plugins
- Users can generate EDO and harmonic series scales from UI
- 20+ factory tunings bundled
- Export to SCL/KBM/HTML functional

### Phase 4 Complete When:
- Unison detuning creates audibly richer sound
- All professional features stable and documented
- Performance remains within acceptable limits

---

## References

### Research Sources
- [Pianoteq User Manual](https://www.modartt.com/user_manual?product=pianoteq&lang=en)
- [Surge XT Tuning Guide](https://surge-synthesizer.github.io/tuning-guide/)
- [Surge XT Manual](https://surge-synthesizer.github.io/manual-xt/)
- [Surge Tuning Library (GitHub)](https://github.com/surge-synthesizer/tuning-library)
- [Scale Workshop](https://github.com/SeanArchibald/scale-workshop)
- [Scala Format Specification](https://www.huygens-fokker.org/scala/scl_format.html)

### Historical Temperament Data
- [Kyle Gann: Introduction to Historical Tunings](https://www.kylegann.com/histune.html)
- [Wikipedia: Well Temperament](https://en.wikipedia.org/wiki/Well_temperament)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-21 | Initial plan based on Pianoteq/Surge research |
