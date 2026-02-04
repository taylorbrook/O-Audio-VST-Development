# Stage 3: GUI Implementation - Context

## Discussion Summary

**Date:** 2026-02-03
**Participants:** User, Claude

## Requirements Confirmed

### Visual Style
- **Naturalist aesthetic** (O-Detune style)
  - Paper texture background
  - Botanical/organic accents
  - Serif fonts (Georgia)
  - Muted earthy color palette with green accent
  - Vine-arc SVG knobs

### Grid Layout
- **4 rows × 32 columns** (bands as rows)
- Each band = horizontal row
- Steps flow left-to-right
- Playhead moves horizontally across grid
- **Step-based playhead** (jumps to current step, not smooth animation)

### Step Display
- **Gain as brightness** — step opacity/brightness reflects gain level
- Full brightness = 100% gain, dim = low gain, off = 0%
- Click toggles on/off, drag adjusts gain (if we add that interaction)

### Band Configuration
- **Fixed frequency bands** for v1.0:
  - Band 0 (Sub): 20-120 Hz
  - Band 1 (Low): 120-500 Hz
  - Band 2 (Mid): 500-4000 Hz
  - Band 3 (High): 4000-20000 Hz
- Display band names and frequency ranges as labels (not editable)

### Euclidean Controls
- **Expandable panel per band**
- Click band label/header to expand Euclidean settings
- Collapsed: shows pattern mode (Manual/Euclidean) indicator
- Expanded: reveals Steps, Pulses, Offset sliders/knobs
- Only one band expanded at a time (accordion style)

### Global Controls
- **Bottom bar (horizontal)** — footer area below grid
- Contains: Mix, Rate dropdown, Swing, Smoothing, Steps dropdown
- Similar layout to O-Detune output section

### Window Size
- **850 × 550 pixels** — standard comfortable size
- Room for 32-step grid at reasonable cell size
- Space for controls without scrolling

## Constraints Identified

1. **WebView rendering performance** — 128 cells + playhead updates
   - Batch DOM updates
   - Use CSS transforms for playhead position
   - requestAnimationFrame for smooth timing

2. **Parameter synchronization** — 128 step parameters
   - Efficient bulk parameter reads from JUCE
   - Consider grid state as single serialized object vs 128 individual params

3. **Euclidean pattern preview** — when Euclidean mode active, grid should show generated pattern
   - Read-only display when in Euclidean mode
   - Switch to Manual mode to enable cell editing

4. **Naturalist style on dark grid** — paper texture doesn't work well for interactive grids
   - Grid area may need darker/neutral background within naturalist frame
   - Keep naturalist chrome (header, controls, panels) around grid

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Visual style | Naturalist (O-Detune) | User preference, consistent O-series branding |
| Grid orientation | Horizontal rows per band | Most intuitive for step sequencers |
| Step display | Brightness = gain | More info without cluttering; gain visible at a glance |
| Euclidean UI | Expandable per-band | Keeps grid clean, controls accessible when needed |
| Playhead | Step-based jumps | Cleaner visual, lower CPU, aligns with step sequencer mental model |
| Global controls | Bottom bar | Familiar pattern from O-Detune, keeps header minimal |
| Band frequencies | Fixed v1.0 | Reduces UI complexity, can add in v1.1 |
| Window size | 850×550 | Comfortable for 32 steps, no scrolling needed |

## UI Layout Specification

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  ○ O-FreqPulse                                          [paper header]       │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─ Band Rows (interactive grid) ──────────────────────────────────────────┐ │
│  │                                                                         │ │
│  │  HIGH  ▼  [████░░██░░████░░██░░████░░██░░████░░██░░] E:8/5/0  [>]      │ │
│  │  MID   ▼  [██████████░░░░████████░░░░████████░░░░░░] Manual   [>]      │ │
│  │  LOW   ▼  [████████████████░░░░░░░░████████████████] E:7/3/2  [>]      │ │
│  │  SUB   ▼  [████████████████████████████████████████] Manual   [>]      │ │
│  │             ↑ playhead                                                   │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
│  ┌─ Expanded Euclidean (when [>] clicked) ─────────────────────────────────┐ │
│  │  Band: HIGH    Mode: [Euclidean ▼]                                       │ │
│  │  Steps: ○ 8    Pulses: ○ 5    Offset: ○ 0    Depth: ○ 100%              │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│  ┌─ Footer Panel ──────────────────────────────────────────────────────────┐ │
│  │   Steps: [16 ▼]    Rate: [1/16 ▼]    Swing: ○    Smooth: ○    Mix: ○   │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Grid Cell Dimensions (estimated)
- Grid width: ~700px for 32 steps = ~22px per cell
- Grid height: ~200px for 4 bands = ~50px per band row
- Cell size: ~20×40px (wider than tall, clickable)

### Color Mapping (Naturalist + Grid)
- **Paper background:** #F5E6D3 (header, footer, side areas)
- **Grid background:** #1a1410 (dark, for contrast)
- **Step off:** rgba(60, 47, 47, 0.3) (faint grid cell outline)
- **Step on (full gain):** var(--accent-green) = #5a7a6a
- **Step on (partial gain):** opacity scaled 0.3-1.0
- **Playhead:** #8BA870 (lighter green) with glow
- **Band labels:** var(--text-secondary) = #8b7355

## JavaScript Bridge Requirements

### Parameters to Expose
```javascript
// Global controls (5)
sliderStates: mix, swing, smoothing
comboStates: steps, rate

// Per-band controls (8 × 4 = 32)
sliderStates: band0_depth, band1_depth, band2_depth, band3_depth
toggleStates: band0_enable, band1_enable, band2_enable, band3_enable
comboStates: band0_mode, band1_mode, band2_mode, band3_mode
sliderStates: band0_euc_steps, band0_euc_pulses, band0_euc_offset, ... (×4)

// Step grid (128)
// Option A: Individual sliders (step_0_0 through step_3_31)
// Option B: Custom message passing for bulk grid state
```

### Playhead Communication
- Processor sends current step index periodically (every processBlock)
- WebView receives via custom message or polling
- Update playhead position in requestAnimationFrame loop

## Open Questions

1. **Grid editing interaction:**
   - Click = toggle on/off (gain 0% ↔ 100%)
   - Drag horizontal = paint multiple steps
   - Drag vertical on step = adjust gain? (v1.1 feature?)

2. **Euclidean mode grid behavior:**
   - When Euclidean mode active, is grid read-only or can user override?
   - Decision: Grid shows Euclidean pattern, editing switches to Manual mode

3. **Botanical overlay:**
   - Which botanical illustration for FreqPulse?
   - Suggestion: Something rhythmic/wave-like (fern fronds? water ripple?)

## Next Phase

Ready for: **research** phase (investigate WebView grid rendering patterns, playhead sync)

---

*Generated: 2026-02-03 via /plugin-discuss*
