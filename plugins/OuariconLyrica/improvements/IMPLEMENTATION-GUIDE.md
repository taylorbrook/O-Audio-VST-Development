# OuariconLyrica Tuning System - Implementation Guide

**Purpose:** Standalone reference for implementing each phase with fresh context.
**Full Research:** See `tuning-system-overhaul.md` in this directory.
**Current Version:** v1.9.0 → Target: v2.0.0

---

## Progress Tracker

| Phase | Status | Version | Completed |
|-------|--------|---------|-----------|
| Phase 1: DSP Foundation | ✅ COMPLETE | v1.9.0 | 2026-01-21 |
| Phase 2: UI Visualization | ✅ COMPLETE | v1.10.0 | 2026-01-21 |
| Phase 3: Advanced Features | ⏳ Pending | v1.11.0 | - |
| Phase 4: Professional Polish | ⏳ Pending | v2.0.0 | - |

---

## Quick Reference

### Key Files
| File | Purpose |
|------|---------|
| `Source/DSP/TuningEngine.h` | Tuning API definitions |
| `Source/DSP/TuningEngine.cpp` | Tuning implementation |
| `Source/PluginProcessor.h/.cpp` | Parameter definitions, native functions |
| `Source/PluginEditor.cpp` | WebView setup, timer callback for MIDI |
| `Resources/ui/index.html` | All UI (HTML/CSS/JS in single file) |

### Current TuningEngine Capabilities (v1.9.0)
- Modes: `TwelveTET`, `Scala`, `MTSESP` (placeholder)
- Scala .scl/.kbm loading: ✅ Complete
- Master tune (A4): 400-480 Hz
- Tonic transposition: 0-11
- Per-note pitch bend: ✅ Complete
- Frequency table: Pre-computed, lock-free audio access
- **Octave Stretch:** 0.95-1.25, applied to both 12-TET and custom tunings (v1.9.0)
- **Built-in Presets:** 10 temperaments + Custom option (v1.9.0)

---

## Phase 1: DSP Foundation ✅ COMPLETE

### Goal
Add stretch tuning parameter and built-in temperament presets (additive to existing Scala loading).

> **Status:** Implemented in v1.9.0 (2026-01-21). See CHANGELOG.md for details.

### Tasks

#### 1.1 Stretch Tuning Parameter

**TuningEngine.h** - Add after line ~70:
```cpp
// Stretch tuning for physical modeling
void setOctaveStretch(float stretch);  // 0.95 - 1.25, default 1.0
float getOctaveStretch() const;
```

**TuningEngine.h** - Add to private section (~line 246):
```cpp
float octaveStretch = 1.0f;
```

**TuningEngine.cpp** - Implement:
```cpp
void TuningEngine::setOctaveStretch(float stretch)
{
    float newStretch = juce::jlimit(0.95f, 1.25f, stretch);
    if (std::abs(newStretch - octaveStretch) > 0.001f)
    {
        octaveStretch = newStretch;
        rebuildFrequencyTable();
    }
}

float TuningEngine::getOctaveStretch() const { return octaveStretch; }
```

**TuningEngine.cpp** - Modify `calculate12TETFrequency()` (~line 540):
```cpp
double TuningEngine::calculate12TETFrequency(int midiNote) const
{
    // Apply stretch: wider octaves in upper register, narrower in lower
    const double semitonesFromA4 = static_cast<double>(midiNote - 69);
    const double stretchedSemitones = semitonesFromA4 * octaveStretch;
    return a4Frequency * std::pow(2.0, stretchedSemitones / 12.0);
}
```

**TuningEngine.cpp** - Also apply stretch in `calculateCustomFrequency()` final calculation.

**PluginProcessor** - Add APVTS parameter:
```cpp
// In createParameterLayout()
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "octaveStretch", "Octave Stretch",
    juce::NormalisableRange<float>(0.95f, 1.25f, 0.01f), 1.0f));
```

**index.html** - Add slider in Advanced section (around line 1280):
```html
<div class="param-row">
    <label>Octave Stretch</label>
    <input type="range" id="octave-stretch" min="0.95" max="1.25" step="0.01" value="1.0">
    <span id="octave-stretch-value">1.00</span>
</div>
```

---

#### 1.2 Built-in Temperament Presets

**TuningEngine.h** - Add enum after Mode enum (~line 47):
```cpp
enum class BuiltInPreset
{
    Equal12TET = 0,
    Pythagorean,
    Zarlino,
    MeantoneQuarter,
    WerckmeisterIII,
    KirnbergerIII,
    Vallotti,
    WellTempered,
    JustIntonation,
    BohlenPierce,
    Custom  // Set when user loads .scl file
};

void setBuiltInPreset(BuiltInPreset preset);
BuiltInPreset getBuiltInPreset() const;
juce::String getPresetName() const;
```

**TuningEngine.h** - Add to private:
```cpp
BuiltInPreset currentPreset = BuiltInPreset::Equal12TET;
```

**TuningEngine.cpp** - Add preset data (new static arrays):
```cpp
// Cents from C for each temperament (12 notes)
static const std::array<double, 12> PRESET_EQUAL = {
    0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0
};

static const std::array<double, 12> PRESET_WERCKMEISTER_III = {
    0.0, 90.225, 192.18, 294.135, 390.225, 498.045, 588.27, 696.09, 792.18, 888.27, 996.09, 1092.18
};

static const std::array<double, 12> PRESET_PYTHAGOREAN = {
    0.0, 113.685, 203.91, 294.135, 407.82, 498.045, 611.73, 701.955, 815.64, 905.865, 996.09, 1109.775
};

static const std::array<double, 12> PRESET_MEANTONE_QUARTER = {
    0.0, 76.05, 193.16, 310.26, 386.31, 503.42, 579.47, 696.58, 772.63, 889.74, 1006.84, 1082.89
};

static const std::array<double, 12> PRESET_KIRNBERGER_III = {
    0.0, 90.18, 193.16, 294.13, 386.31, 498.04, 590.22, 696.58, 792.18, 889.74, 996.09, 1088.27
};

static const std::array<double, 12> PRESET_VALLOTTI = {
    0.0, 94.13, 196.09, 298.04, 392.18, 501.96, 592.18, 698.04, 796.09, 894.13, 1000.0, 1090.22
};

static const std::array<double, 12> PRESET_ZARLINO = {
    0.0, 111.73, 203.91, 315.64, 386.31, 498.04, 582.51, 701.96, 813.69, 884.36, 1017.60, 1088.27
};

static const std::array<double, 12> PRESET_JUST_INTONATION = {
    0.0, 111.73, 203.91, 315.64, 386.31, 498.04, 582.51, 701.96, 813.69, 884.36, 996.09, 1088.27
};

// Bohlen-Pierce is 13-note, handle separately
```

**TuningEngine.cpp** - Implement setBuiltInPreset():
```cpp
void TuningEngine::setBuiltInPreset(BuiltInPreset preset)
{
    currentPreset = preset;

    if (preset == BuiltInPreset::Custom)
        return;  // Don't change intervals, custom file already loaded

    std::vector<double> intervals;
    juce::String name;

    switch (preset)
    {
        case BuiltInPreset::Equal12TET:
            intervals = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100};
            name = "Equal 12-TET";
            setMode(Mode::TwelveTET);
            break;
        case BuiltInPreset::WerckmeisterIII:
            intervals.assign(PRESET_WERCKMEISTER_III.begin(), PRESET_WERCKMEISTER_III.end());
            name = "Werckmeister III";
            setMode(Mode::Scala);
            break;
        // ... other cases
        default:
            return;
    }

    setCustomIntervals(intervals, name);
}
```

**TuningEngine.cpp** - Modify loadScalaFile() to set preset to Custom:
```cpp
bool TuningEngine::loadScalaFile(const juce::File& sclFile)
{
    // ... existing implementation ...

    // After successful load, add:
    currentPreset = BuiltInPreset::Custom;

    return true;
}
```

**PluginProcessor** - Add parameter and native function:
```cpp
// Parameter
params.push_back(std::make_unique<juce::AudioParameterChoice>(
    "temperamentPreset", "Temperament Preset",
    juce::StringArray{"Equal 12-TET", "Pythagorean", "Zarlino", "Meantone 1/4",
                      "Werckmeister III", "Kirnberger III", "Vallotti",
                      "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom"},
    0));

// Native function
webView->bind("setTemperamentPreset", [this](int index) {
    tuningEngine.setBuiltInPreset(static_cast<TuningEngine::BuiltInPreset>(index));
});
```

**index.html** - Add preset dropdown (before tonic selector, ~line 1213):
```html
<div class="tuning-preset-row">
    <label>Preset:</label>
    <select id="temperament-preset" onchange="handlePresetChange(this.value)">
        <option value="0">Equal 12-TET</option>
        <option value="1">Pythagorean</option>
        <option value="2">Zarlino</option>
        <option value="3">Meantone (1/4 comma)</option>
        <option value="4">Werckmeister III</option>
        <option value="5">Kirnberger III</option>
        <option value="6">Vallotti</option>
        <option value="7">Well Tempered</option>
        <option value="8">Just Intonation</option>
        <option value="9">Bohlen-Pierce</option>
        <option value="10" id="custom-preset-option" style="display:none">Custom</option>
    </select>
</div>
```

```javascript
async function handlePresetChange(value) {
    const index = parseInt(value);
    try {
        await Juce.getNativeFunction('setTemperamentPreset')(index);
        // Refresh intervals display
        await refreshTuningState();
    } catch (e) {
        console.error('Failed to set preset:', e);
    }
}

// When .scl file loaded, show Custom option and select it
function onScalaFileLoaded(filename) {
    const customOption = document.getElementById('custom-preset-option');
    customOption.style.display = 'block';
    customOption.textContent = `Custom (${filename})`;
    document.getElementById('temperament-preset').value = '10';
}
```

---

### Phase 1 Verification Checklist (✅ COMPLETE - 2026-01-21)
- [x] Octave stretch slider appears in tuning controls panel
- [x] Stretch values 0.95-1.25 affect frequency calculation
- [x] Upper octaves sound progressively wider with stretch > 1.0
- [x] Preset dropdown appears above mode buttons
- [x] Selecting preset instantly changes intervals
- [x] Werckmeister III sounds noticeably different from Equal
- [x] Load .scl still works, shows "Custom (filename)" in dropdown
- [x] Selecting preset after loading .scl overrides custom tuning
- [x] Settings persist when plugin reloaded

**Implementation Notes:**
- Octave stretch slider placed below scale name display (not Advanced section)
- Temperament dropdown placed at top of tuning controls panel
- 10 built-in presets + Custom option (shown when .scl loaded)
- Native functions: `setTemperamentPreset`, `getTemperamentPreset`, `setOctaveStretch`, `getOctaveStretch`
- Files modified: TuningEngine.h/cpp, PluginProcessor.cpp, PluginEditor.h/cpp, index.html

---

## Phase 2: UI Visualization

### Goal
Add 5 visualization modes: Circle (existing), Polar, Matrix, True Keys, Modal Rotation.

### Tasks

#### 2.1 View Mode Toggle

**index.html** - Add mode buttons above visualization (~line 1220):
```html
<div class="viz-mode-toggle">
    <button class="viz-btn active" data-mode="circle" onclick="setVizMode('circle')">Circle</button>
    <button class="viz-btn" data-mode="polar" onclick="setVizMode('polar')">Polar</button>
    <button class="viz-btn" data-mode="matrix" onclick="setVizMode('matrix')">Matrix</button>
    <button class="viz-btn" data-mode="truekeys" onclick="setVizMode('truekeys')">True Keys</button>
    <button class="viz-btn" data-mode="rotation" onclick="setVizMode('rotation')">Rotation</button>
</div>
```

```javascript
let vizMode = 'circle';

function setVizMode(mode) {
    vizMode = mode;
    document.querySelectorAll('.viz-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.mode === mode);
    });
    updateVisualization();
}

function updateVisualization() {
    switch (vizMode) {
        case 'circle': drawPitchCircle(); break;
        case 'polar': drawPolarWheel(); break;
        case 'matrix': drawIntervalMatrix(); break;
        case 'truekeys': updateTrueKeysDisplay(); break;
        case 'rotation': drawModalRotationMatrix(); break;
    }
}
```

---

#### 2.2 Enhanced Interval List (Deviation Display)

**index.html** - Modify updateIntervalListUI() (~line 1940):
```javascript
function updateIntervalListUI() {
    const total = currentIntervals.length;
    let html = `<div class="interval-list-header">${currentScaleName} (${total} notes)</div>`;

    // Tonic selector (existing code)
    if (total === 12) {
        html += generateTonicSelector();
    }

    // Enhanced interval rows with deviation
    for (let i = 0; i < total; i++) {
        const noteName = getDegreeLabel(i, total);
        const cents = currentIntervals[i];
        const equalCents = (i * 1200 / total);
        const deviation = cents - equalCents;
        const devClass = Math.abs(deviation) < 1 ? 'pure' : (deviation > 0 ? 'sharp' : 'flat');

        html += `
            <div class="interval-row">
                <span class="note-name">${noteName}</span>
                <span class="cents-value">${cents.toFixed(1)}¢</span>
                <span class="deviation ${devClass}">${deviation >= 0 ? '+' : ''}${deviation.toFixed(1)}</span>
            </div>
        `;
    }

    intervalList.innerHTML = html;
}
```

**CSS:**
```css
.deviation { font-size: 10px; opacity: 0.7; margin-left: 4px; }
.deviation.pure { color: #4CAF50; }
.deviation.sharp { color: #FF9800; }
.deviation.flat { color: #2196F3; }
```

---

#### 2.3 Polar/Radial Tone Wheel

**index.html** - Add canvas and drawing function:
```javascript
function drawPolarWheel() {
    const canvas = document.getElementById('tuning-canvas');
    const ctx = canvas.getContext('2d');
    const cx = canvas.width / 2;
    const cy = canvas.height / 2;
    const maxR = Math.min(cx, cy) - 20;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Reference circles (200, 400, 600... cents)
    ctx.strokeStyle = 'rgba(255,255,255,0.15)';
    for (let c = 200; c <= 1200; c += 200) {
        ctx.beginPath();
        ctx.arc(cx, cy, (c / 1200) * maxR, 0, Math.PI * 2);
        ctx.stroke();
    }

    // Scale degree nodes
    const total = currentIntervals.length;
    for (let i = 0; i < total; i++) {
        const angle = (i / total) * Math.PI * 2 - Math.PI / 2;
        const r = (currentIntervals[i] / 1200) * maxR;
        const x = cx + Math.cos(angle) * r;
        const y = cy + Math.sin(angle) * r;

        // Highlight active notes
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

---

#### 2.4 True Keys Mode

**PluginEditor.cpp** - Send held notes in timerCallback():
```cpp
void OuariconLyricaAudioProcessorEditor::timerCallback()
{
    // Existing code...

    // Collect held notes for True Keys display
    juce::String notesJson = "[";
    juce::String freqsJson = "[";
    bool first = true;

    for (int i = 0; i < processorRef.getSynthesiser().getNumVoices(); ++i)
    {
        auto* voice = processorRef.getSynthesiser().getVoice(i);
        if (voice && voice->isVoiceActive())
        {
            int note = voice->getCurrentlyPlayingNote();
            double freq = processorRef.getTuningEngine().getFrequency(note);

            if (!first) { notesJson += ","; freqsJson += ","; }
            notesJson += juce::String(note);
            freqsJson += juce::String(freq, 4);
            first = false;
        }
    }
    notesJson += "]";
    freqsJson += "]";

    webView->evaluateJavascript("window.updateHeldNotes && window.updateHeldNotes("
        + notesJson + "," + freqsJson + ");");
}
```

**index.html:**
```javascript
let heldNotes = [];
let heldFreqs = [];

window.updateHeldNotes = function(notes, freqs) {
    heldNotes = notes;
    heldFreqs = freqs;
    if (vizMode === 'truekeys') updateTrueKeysDisplay();
};

function updateTrueKeysDisplay() {
    const container = document.getElementById('viz-container');

    if (heldNotes.length < 2) {
        container.innerHTML = '<div class="tk-hint">Hold 2+ notes to see intervals</div>';
        return;
    }

    let html = '<div class="tk-grid">';
    for (let i = 0; i < heldNotes.length; i++) {
        for (let j = i + 1; j < heldNotes.length; j++) {
            const cents = 1200 * Math.log2(heldFreqs[j] / heldFreqs[i]);
            html += `
                <div class="tk-interval">
                    <span>${midiToName(heldNotes[i])} → ${midiToName(heldNotes[j])}</span>
                    <span class="tk-cents">${cents.toFixed(1)}¢</span>
                </div>
            `;
        }
    }
    html += '</div>';
    container.innerHTML = html;
}

function midiToName(midi) {
    const names = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
    return names[midi % 12] + Math.floor(midi / 12 - 1);
}
```

---

#### 2.5 Modal Rotation Matrix

**index.html:**
```javascript
function drawModalRotationMatrix() {
    const container = document.getElementById('viz-container');
    const total = currentIntervals.length;

    let html = '<table class="rotation-table"><tr><th>Mode</th>';
    for (let i = 1; i <= total; i++) html += `<th>${i}</th>`;
    html += '</tr>';

    for (let start = 0; start < total; start++) {
        html += `<tr><td class="mode-name">${getDegreeLabel(start, total)}</td>`;

        let prev = 0;
        for (let step = 0; step < total; step++) {
            const deg = (start + step) % total;
            let cents = currentIntervals[deg];
            if (deg < start) cents += 1200;

            const interval = step === 0 ? 0 : cents - prev;
            prev = cents;

            const cls = interval < 100 ? 'small' : interval < 250 ? 'medium' : 'large';
            html += `<td class="int-cell ${cls}" title="${interval.toFixed(1)}¢">${Math.round(interval)}</td>`;
        }
        html += '</tr>';
    }

    html += '</table>';
    container.innerHTML = html;
}
```

**CSS:**
```css
.rotation-table { border-collapse: collapse; font-size: 10px; }
.rotation-table th, .rotation-table td { padding: 3px 5px; border: 1px solid rgba(255,255,255,0.2); }
.mode-name { background: rgba(139,115,85,0.3); font-weight: bold; }
.int-cell.small { background: rgba(33,150,243,0.4); }
.int-cell.medium { background: rgba(76,175,80,0.4); }
.int-cell.large { background: rgba(255,152,0,0.4); }
```

---

### Phase 2 Verification Checklist (✅ COMPLETE - 2026-01-21)
- [x] View mode buttons appear and switch correctly
- [x] Circle mode shows existing pitch circle
- [x] Polar mode shows radial tone wheel with cents as radius
- [x] Matrix mode shows interval values between notes
- [x] True Keys shows intervals between held MIDI notes in real-time
- [x] Rotation mode shows modal rotation matrix with color-coded intervals
- [x] Interval list shows ±deviation from equal temperament
- [x] Active MIDI notes highlight in Polar view

**Implementation Notes:**
- Viz mode toggle added above visualization container with 5 buttons
- Each visualization mode has its own view div with `.viz-view` class
- True Keys receives held notes data from C++ via `updateHeldNotes()` callback
- Deviation display color-coded: green=pure, orange=sharp, blue=flat
- All existing `updatePitchCircle()` calls replaced with `updateVisualization()`
- Files modified: index.html, PluginProcessor.h/cpp, PluginEditor.cpp

---

## Phase 3: Advanced Features

### Goal
MTS-ESP client, scale generators, factory library, export tools.

### Tasks

#### 3.1 MTS-ESP Client

**Dependencies:** Add libMTSClient to project
```cmake
# CMakeLists.txt
add_subdirectory(libs/libMTSClient EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PRIVATE libMTSClient)
```

**TuningEngine.h:**
```cpp
#include "libMTSClient.h"

// In public:
bool connectMTSClient();
void disconnectMTSClient();
bool isMTSConnected() const;

// In private:
MTSClient* mtsClient = nullptr;
```

**TuningEngine.cpp:**
```cpp
bool TuningEngine::connectMTSClient()
{
    if (mtsClient) return true;

    mtsClient = MTS_RegisterClient();
    if (mtsClient) {
        currentMode.store(Mode::MTSESP, std::memory_order_relaxed);
        return true;
    }
    return false;
}

void TuningEngine::disconnectMTSClient()
{
    if (mtsClient) {
        MTS_DeregisterClient(mtsClient);
        mtsClient = nullptr;
    }
}

// In getFrequency(), add MTS-ESP case:
if (currentMode.load() == Mode::MTSESP && mtsClient) {
    return MTS_NoteToFrequency(mtsClient, midiNote, midiChannel);
}
```

---

#### 3.2 Scale Generators

**New file: ScaleGenerator.h/.cpp**
```cpp
class ScaleGenerator {
public:
    static std::vector<double> generateEDO(int divisions);
    static std::vector<double> generateHarmonicSeries(int start, int end);
    static std::vector<double> generateRank2(double generator, double period, int count);
};

// EDO implementation
std::vector<double> ScaleGenerator::generateEDO(int divisions)
{
    std::vector<double> intervals = {0.0};
    double step = 1200.0 / divisions;
    for (int i = 1; i < divisions; i++)
        intervals.push_back(i * step);
    return intervals;
}
```

**UI:** Add generator panel with inputs for EDO divisions, harmonic range, etc.

---

#### 3.3 Factory Library

**New file: EmbeddedTunings.h/.cpp**
- Embed 20+ tunings as static data
- Categories: Historical, Just, EDO, Non-Octave, World
- Provide browser UI with category filter

---

#### 3.4 Export Functions

**New file: TuningExporter.h/.cpp**
```cpp
class TuningExporter {
public:
    static juce::String toSCL(const TuningEngine& engine);
    static juce::String toKBM(const TuningEngine& engine);
    static juce::String toHTML(const TuningEngine& engine);
};
```

---

### Phase 3 Verification Checklist
- [ ] MTS-ESP connects to Oddsound MTS-ESP Mini
- [ ] Tuning follows MTS-ESP master changes
- [ ] EDO generator creates correct intervals for 19, 31, 53 divisions
- [ ] Harmonic series generator creates expected ratios
- [ ] Factory library browser shows categorized tunings
- [ ] Export SCL produces valid Scala file
- [ ] Export HTML produces readable documentation

---

## Phase 4: Professional Polish

### Goal
Unison detuning, apply tuning options, interactive interval matrix.

### Tasks

#### 4.1 Unison Detuning

Major DSP change to WaveguideString:
- Support 1-3 strings per note
- Add unisonWidth (0-2 cents) and unisonBalance (-1 to +1) parameters
- Process multiple delay lines and sum output

**CPU Impact:** 2-3x per voice when enabled

---

#### 4.2 Apply Tuning Options

Add `TuningApplicationPoint` enum:
- `AtMIDIInput` - modulation in 12-TET (default)
- `AfterModulation` - modulation follows scale

Affects pitch bend and portamento behavior.

---

#### 4.3 Interactive Interval Matrix

Click-and-drag to retune intervals in matrix view.
- Shift+drag for fine adjustment (0.1 cent per pixel)
- Send modified intervals to C++ in real-time

---

### Phase 4 Verification Checklist
- [ ] Unison with width=1.5 produces audible beating
- [ ] Unison balance shifts detuning distribution
- [ ] Apply tuning option changes pitch bend behavior
- [ ] Dragging in matrix retunes intervals in real-time
- [ ] CPU remains acceptable with unison enabled

---

## Version Milestones

| Phase | Version | Notes |
|-------|---------|-------|
| Phase 1 | v1.9.0 | Minor - new features, no breaking changes |
| Phase 2 | v1.10.0 | Minor - UI additions |
| Phase 3 | v1.11.0 | Minor - new dependencies (MTS-ESP) |
| Phase 4 | v2.0.0 | Major - DSP changes (unison) |

---

## Context Reset Instructions

When starting a new phase with fresh context:

1. **Read this file** for phase-specific tasks
2. **Read current TuningEngine.h/.cpp** to understand existing implementation
3. **Read index.html tuning section** (search for "TUNING TAB")
4. **Check CHANGELOG.md** for recent changes
5. **Run build** to verify starting state: `cmake --build build --target OuariconLyrica`

**Key search patterns for index.html:**
- Tuning tab: `id="tuning-tab"`
- Interval list: `id="interval-list"`
- Pitch circle: `updatePitchCircle`
- Tonic handlers: `handleTonicDown`
