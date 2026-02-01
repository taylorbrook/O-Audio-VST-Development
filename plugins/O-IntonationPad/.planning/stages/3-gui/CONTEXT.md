# Stage 3: GUI - Context

**Plugin:** O-IntonationPad
**Stage:** 3 - GUI Implementation
**Discussion Date:** 2026-01-29
**Participants:** User, Claude

---

## Discussion Summary

This document captures requirements and decisions from the Stage 3 discuss phase for O-IntonationPad's WebView-based GUI implementation.

---

## Requirements Confirmed

### Overall Design

- **Aesthetic:** Ouaricon Audio Naturalist (official brand aesthetic)
  - Warm earth tones, aged paper texture
  - Botanical seed cross-section knobs
  - Garamond serif typography
  - Wide letter-spacing, classical elegance

- **Window Size:** Medium (800x500)
  - Comfortable for 15 parameters with tabbed layout
  - Room for interactive pitch circle visualization

- **Layout Structure:** Tabbed Sections
  - 4 tabs organizing parameters by function
  - Reduces visual clutter
  - Allows dedicated space for pitch circle

### Tab Organization

**Tab 1: Voice (5 parameters)**
- voiceCount (2-12) - Number of chord voices
- complexity (0-100%) - Chord extensions (triads → 13ths)
- keyRoot (C-B) - Root note of key
- keyScale (10 scales) - Scale/mode for chord generation
- inversionRandom (0-100%) - Randomize chord inversions

**Tab 2: Tuning (2 parameters + pitch circle)**
- tuningSystem (5 systems) - 12-TET, JI, Pythagorean, Historical, Scala
- keyRoot (shared with Tab 1, or display-only reference)
- **Interactive Pitch Circle** - Visual representation of current tuning intervals
  - Shows 12 pitch classes around circle
  - Highlights active pitches when notes played
  - Displays cent offsets from 12-TET
  - Click interaction to explore intervals

**Tab 3: Modulation (6 parameters)**
- wavetablePos (0-100%) - Position in wavetable
- lfoRate (0.01-20 Hz) - LFO speed
- lfoDepth (0-100%) - LFO modulation amount
- timingRandom (0-100ms) - Voice timing stagger
- detuneRandom (0-50 cents) - Micro-detuning per voice
- (Wavetable selector - Stage 4 polish, placeholder for now)

**Tab 4: Output (3 parameters)**
- attackTime (1-5000ms) - Envelope attack
- releaseTime (10-10000ms) - Envelope release
- filterCutoff (20-20000 Hz) - Low-pass filter
- masterVolume (-inf to +6 dB) - Output level

### Pitch Circle Visualization

**Interactive Features:**
- Real-time display of tuning intervals
- Visual difference between 12-TET (equal spacing) and JI/Pythagorean (unequal)
- Highlight active chord voices when notes are playing
- Click to hear individual pitches (optional - defer if complex)

**Visual Design:**
- Circular layout with 12 pitch positions
- Lines or arcs showing interval relationships
- Color coding: green for pure intervals, brown for reference
- Matches Ouaricon Naturalist aesthetic (warm tones, classical feel)

### Botanical Illustration

**Selected Image:** Ocean shell
- Path: `/Users/taylorbrook/Dev/Ouaricon Audio Images/ocean/shell_conchologiaiconi12reev_0090.png`
- **Rationale:** Shell spiral relates to harmonic series structure and natural intervals
- **Placement:** Right side of interface, 0.35 opacity, click-through

---

## Constraints Identified

### Technical Constraints

1. **WebView Environment:**
   - No viewport units (vw, vh) - use px or %
   - Limited to Chromium rendering in JUCE WebView
   - Must handle parameter updates from both UI and DAW automation

2. **Performance:**
   - Pitch circle updates should not block audio thread
   - Use requestAnimationFrame for smooth visualization
   - Avoid heavy DOM manipulation during playback

3. **Parameter Binding:**
   - 15 WebSliderRelay or WebSliderParameterAttachment instances
   - Bidirectional sync: UI → APVTS → DSP and DAW → APVTS → UI
   - Choice parameters (keyRoot, keyScale, tuningSystem) need dropdown or button group handling

### Design Constraints

1. **Tab Navigation:**
   - Must be visually clear which tab is active
   - Tabs styled to match Ouaricon Naturalist (green active state, fleuron accents)
   - Content area consistent across tabs

2. **Pitch Circle:**
   - Must be readable at 800x500 size
   - ~200-250px diameter to fit in tab content area
   - Labels must be legible (Garamond, 10-12px)

3. **Botanical Overlay:**
   - Shell image positioned to not overlap tab navigation
   - May need to reduce opacity further (0.25-0.30) if tabs conflict
   - Consider placing below tab content area if overlap is unavoidable

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Aesthetic | Ouaricon Naturalist | Official brand identity, maintains product line consistency |
| Window Size | 800x500 | Balanced - room for tabs and pitch circle without dominating screen |
| Layout | Tabbed (4 tabs) | Reduces clutter for 15 params, allows dedicated tuning visualization |
| Tab Grouping | Voice/Tuning/Mod/Output | Logical functional grouping, matches DSP architecture |
| Pitch Circle | Interactive | Key differentiator - visualizes just intonation intervals |
| Illustration | Ocean shell | Thematic fit - spiral relates to harmonic series |
| Control Style | Seed cross-section knobs | Ouaricon brand standard |
| Typography | Garamond, uppercase labels | Ouaricon brand standard |

---

## Parameter-to-Control Mapping

| Parameter | Control Type | Location | Notes |
|-----------|--------------|----------|-------|
| voiceCount | Knob (int) | Tab 1 | Range 2-12, step 1 |
| complexity | Knob (float) | Tab 1 | 0-100%, shows % |
| keyRoot | Dropdown or Button Row | Tab 1 | 12 options (C-B) |
| keyScale | Dropdown | Tab 1 | 10 scale options |
| inversionRandom | Knob (float) | Tab 1 | 0-100% |
| tuningSystem | Button Row or Dropdown | Tab 2 | 5 options |
| (keyRoot display) | Label | Tab 2 | Reference only, controlled from Tab 1 |
| wavetablePos | Knob (float) | Tab 3 | 0-100% |
| lfoRate | Knob (float) | Tab 3 | 0.01-20 Hz, log scale display |
| lfoDepth | Knob (float) | Tab 3 | 0-100% |
| timingRandom | Knob (float) | Tab 3 | 0-100ms |
| detuneRandom | Knob (float) | Tab 3 | 0-50 cents |
| attackTime | Knob (float) | Tab 4 | 1-5000ms, log scale |
| releaseTime | Knob (float) | Tab 4 | 10-10000ms, log scale |
| filterCutoff | Knob (float) | Tab 4 | 20-20000 Hz, log scale |
| masterVolume | Knob (float) | Tab 4 | -60 to +6 dB, inf at min |

---

## Open Questions

### For Research Phase

1. **Pitch Circle Implementation:**
   - Best approach for real-time pitch visualization in WebView?
   - Canvas vs SVG for performance?
   - How to receive active note data from DSP thread?

2. **Tab Component:**
   - Build custom or use lightweight library?
   - Accessibility considerations (keyboard navigation)?

3. **Choice Parameter Styling:**
   - Dropdown vs button row for 5-option tuningSystem?
   - How to style dropdown to match Ouaricon aesthetic?

### Deferred to Stage 4

- Wavetable selector (8 built-in wavetables)
- Scala file import button and file dialog
- Custom tuning manual entry UI

---

## UI Wireframe (Conceptual)

```
+------------------------------------------------------------------+
|  O-INTONATIONPAD                            [Shell illustration] |
|------------------------------------------------------------------|
|  [ Voice ]  [ Tuning ]  [ Modulation ]  [ Output ]               |
|------------------------------------------------------------------|
|                                                                  |
|  Tab Content Area (varies by tab)                                |
|                                                                  |
|  Tab 1 (Voice):                                                  |
|    [Knob]     [Knob]     [Dropdown]   [Dropdown]   [Knob]       |
|    Voices    Complexity   Key Root    Scale       Inversion     |
|                                                                  |
|  Tab 2 (Tuning):                                                 |
|    [Button Row: 12-TET | JI | Pyth | Hist | Scala]              |
|                                                                  |
|              +------------------+                                |
|              |   Pitch Circle   |     Key: C Major              |
|              |     (visual)     |     Tuning: Just Intonation   |
|              +------------------+                                |
|                                                                  |
|  Tab 3 (Modulation):                                             |
|    [Knob]     [Knob]     [Knob]     [Knob]     [Knob]           |
|    Position   LFO Rate   LFO Depth  Timing     Detune           |
|                                                                  |
|  Tab 4 (Output):                                                 |
|    [Knob]     [Knob]     [Knob]     [Knob]                      |
|    Attack    Release    Cutoff     Volume                       |
|                                                                  |
+------------------------------------------------------------------+
```

---

## Next Phase

**Ready for:** research phase

The research phase should investigate:
1. Pitch circle implementation approaches (Canvas vs SVG, real-time updates)
2. Tab component patterns in WebView (accessibility, styling)
3. Choice parameter UI patterns (dropdown vs buttons)
4. WebView ↔ APVTS bidirectional binding patterns for 15 parameters

After research, the plan phase will create detailed implementation tasks.

---

## Files to Create (Stage 3)

**HTML/CSS/JS:**
- `plugins/O-IntonationPad/Source/ui/public/index.html`
- `plugins/O-IntonationPad/Source/ui/public/css/style.css`
- `plugins/O-IntonationPad/Source/ui/public/js/main.js`
- `plugins/O-IntonationPad/Source/ui/public/js/pitch-circle.js`
- `plugins/O-IntonationPad/Source/ui/public/img/shell_conchologiaiconi12reev_0090.png`

**C++ WebView Integration:**
- `plugins/O-IntonationPad/Source/WebViewManager.h`
- `plugins/O-IntonationPad/Source/WebViewManager.cpp`
- Updates to `PluginEditor.h/cpp` for WebView hosting

---

## Context Checksum

- BRIEF.md: Valid (15 parameters defined)
- ROADMAP.md: Valid (Stage 3 goals match this context)
- Stage 2 VERIFICATION.md: Valid (DSP complete, ready for GUI)
- Aesthetic selected: ouaricon-naturalist-001
- Illustration selected: ocean/shell_conchologiaiconi12reev_0090.png
