# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-07
**Participants:** User, Claude

## Requirements Confirmed

### Layout & Structure
- **Window size:** 900x700 pixels
- **Layout approach:** Grouped sections — all 18 parameters visible at once, no tabs
- **Visualization placement:** Top section (~40% height) for grain scatter + Euclidean circle; controls fill bottom ~60%
- **Parameter groups:** 4 distinct visual groups

### Visualizations
- **Grain scatter display:** 2D scatter plot — horizontal axis = time position in delay buffer, vertical axis = pitch. Each active grain is a dot/circle that fades as envelope decays. Shows all 64 voices in real-time.
- **Euclidean circle visualizer:** Circular display showing step distribution with current step highlighted. Positioned in top-right area alongside the grain scatter.
- **Update rate:** 30-60 FPS via timerCallback (already present in editor)

### Aesthetic
- **Base aesthetic:** Ouaricon Naturalist — aged paper background (#F5E6D3), botanical seed-cross-section rotary knobs, warm earth tones, Garamond typography
- **Reference:** `/Users/taylorbrook/Dev/VST-development/.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`
- **Consistency:** Matches O-FreqPulse, O-Bells, O-SpectralShaper brand family

### Controls
- **Continuous parameters:** Botanical seed-cross-section rotary knobs (grain size, density, spread, pitch random, pan random, reverse, feedback, dry/wet, probability, repeats, euclidean pulses, euclidean steps)
- **Choice parameters:** Dropdown menus (scale, root note, sync mode, pitch mode)
- **Toggle parameters:** Toggle buttons with state glow (freeze, stutter gate)
- **Freeze behavior:** Latching toggle with glow/pulse when active; grain visualization changes appearance to indicate frozen state

### Parameter Grouping (4 Groups)

**Group 1: Core Engine**
- Grain Size (knob, 10-500ms)
- Density (knob, 0-100%)
- Spread (knob, 0-100%)
- Reverse (knob, 0-100%)
- Feedback (knob, 0-100%)
- Dry/Wet (knob, 0-100%)

**Group 2: Pitch & Scale**
- Pitch Random (knob, 0-100%)
- Scale (dropdown: Chromatic/Major/Minor/Penta/WholeTone)
- Root Note (dropdown: C-B)
- Pitch Mode (dropdown: Random/Up/Down/Pendulum)
- Pan Random (knob, 0-100%)

**Group 3: Beat Sync**
- Sync Mode (dropdown: Free/1-4/1-8/1-16/1-32/1-8T/1-16T)
- Probability (knob, 0-100%)
- Repeats (knob, 1-16)
- Stutter Gate (toggle)

**Group 4: Euclidean**
- Pulses (knob, 1-16)
- Steps (knob, 2-16)

**Special Controls**
- Freeze (toggle with glow) — positioned prominently, likely near grain visualization

## Constraints Identified

- Must use JUCE WebView resource provider (no external URLs)
- Must use `getBackendResourceAddress()` in JS for cross-platform URL compatibility
- All parameter bindings via WebSliderRelay/WebComboBoxRelay/WebToggleButtonRelay (already declared in PluginEditor.h)
- No viewport units in CSS (WebView constraint) — use percentage-based sizing
- Timer callback already exists for real-time visualization updates
- 18 relays + 18 attachments already declared in PluginEditor.h — need to instantiate them in .cpp

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Layout | Grouped sections, no tabs | 18 params manageable in single view; tabs add click overhead |
| Window size | 900x700 | Enough room for visualizations + 4 control groups |
| Grain viz | 2D scatter plot (time x pitch) | Most informative; shows where grains read from and their pitch |
| Euclidean viz | Circular step display | Intuitive for rhythm patterns; matches musical expectation |
| Aesthetic | Ouaricon Naturalist | Brand consistency across plugin lineup |
| Controls | Rotary knobs + dropdowns | Naturalist knob design, dropdowns for discrete choices |
| Freeze UX | Latching toggle + glow | Clear state indication, no mouse-hold needed |
| Viz placement | Top 40% | Visualizations are primary feedback, controls below |

## Reference Plugins for Implementation

1. **O-FreqPulse** — Naturalist aesthetic, grid visualization, Euclidean controls
2. **O-SpectralShaper** — WebGL real-time visualization patterns
3. **O-IntonationPad** — SVG circular visualization (pitch-circle.js)
4. **O-Bells** — Complex multi-parameter layout, Naturalist styling

## Technical Notes

### Existing Infrastructure (from Stage 1)
- PluginEditor.h already declares all 18 relays and 18 attachments
- WebBrowserComponent pointer declared
- Timer interface implemented
- getResource() method for serving UI files
- JUCE interop JS files present in Source/ui/public/js/juce/

### Implementation Scope
- Replace placeholder index.html with full UI
- Add CSS styles (naturalist aesthetic)
- Add JS modules: parameter binding, grain visualization, Euclidean circle
- Instantiate relays and attachments in PluginEditor.cpp constructor
- Wire timerCallback for visualization data push

### Grain Visualization Data Flow
- Processor exposes grain state (position, pitch, pan, envelope phase, active) via atomic or lock-free mechanism
- Editor reads grain state in timerCallback at 30-60 Hz
- JS receives grain data via `__juce__.backend.emitEvent()` or custom native function
- Canvas/SVG renders grain dots with fade based on envelope phase

### Euclidean Visualization Data Flow
- Processor exposes current Euclidean pattern (16 bools) and current step index
- Editor pushes pattern + step to JS on timerCallback
- SVG circle renders steps with active pulse highlighting

## Open Questions

- None — all key decisions made. Ready for research phase.

## Next Phase

Ready for: **research** phase — investigate implementation patterns for grain visualization, Euclidean circle rendering, and Naturalist WebView integration.
