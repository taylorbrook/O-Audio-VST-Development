# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-01
**Participants:** User, Claude

## Requirements Confirmed

### Visual Style: Ouaricon Botanical

- **Background:** Aged paper texture (`paper1.jpg`) - warm sepia tones
- **Illustration:** Anatomical muscle illustration (`muscles_histoirephysiqu911875gran_0161.png`) as subtle background overlay
- **Overall Aesthetic:** Scientific journal / botanical illustration style

### Assets

| Asset | Path | Usage |
|-------|------|-------|
| Paper texture | `/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg` | Full background |
| Anatomical illustration | `/Users/taylorbrook/Dev/Ouaricon Audio Images/anatomy/muscles_histoirephysiqu911875gran_0161.png` | Faded overlay behind controls |

### Layout

- **Structure:** Central freeze button as focal point, knobs arranged around it
- **Dimensions:** 450×450 pixels (square)
- **Header:** Top bar with "Ouaricon Granular Freeze" branding

### Controls (5 total)

| Parameter | UI Element | Style | Notes |
|-----------|------------|-------|-------|
| FREEZE | Organic-shaped button | Animated when engaged | Central, large, prominent |
| MODE | Toggle (Manual/Threshold) | Botanical-styled | Near freeze button |
| THRESHOLD | Rotary knob | Botanical with vine indicators | Disabled (subtle dim) in Manual mode |
| DRIFT | Rotary knob | Botanical with vine indicators | Always active |
| MIX | Rotary knob | Botanical with vine indicators | Always active |

### Freeze Button Specification

- **Shape:** Organic/irregular (not geometric) - matches anatomical theme
- **State Indication:** Animation when frozen (subtle pulse, shimmer, or movement)
- **Color Behavior:** May complement with color shift alongside animation

### Knob Style Specification

- **Type:** Custom botanical design
- **Indicator:** Vine or tendril-inspired arc that grows/wraps as value increases
- **Labels:** Serif typography (scientific journal aesthetic)

### Disabled State

- **Behavior:** Subtle dim (reduced opacity ~50-60%)
- **Affected Controls:**
  - THRESHOLD knob: Dimmed when MODE = Manual
  - FREEZE button: Dimmed when MODE = Threshold

### Activity Visualization

- **Type:** Subtle grain activity indicator
- **Trigger:** Visible only when freeze is engaged
- **Style:** Small particles, dots, or organic movement indicating grain activity
- **Location:** Near or around freeze button (TBD in mockup)

### Typography

- **Font Family:** Serif (e.g., Georgia, Playfair Display, or similar scientific font)
- **Header:** "Ouaricon Granular Freeze" - prominent in top bar
- **Labels:** Parameter names beneath each control
- **Values:** Current parameter values displayed

## Constraints Identified

1. **Mode-dependent UI:** Controls must visually respond to MODE parameter changes
2. **Animation Performance:** Freeze button animation must be smooth, not impact DSP
3. **Asset Integration:** Paper and illustration must blend seamlessly (opacity/blending)
4. **Readability:** Knob values must remain legible against textured background
5. **WebView Implementation:** All visuals via HTML/CSS/JS (JUCE WebViewEditor)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Layout | Central button | FREEZE is primary action; should dominate visually |
| Illustration placement | Background overlay | Adds atmosphere without competing with controls |
| Knob style | Custom botanical | Cohesive with overall theme; unique identity |
| Disabled state | Subtle dim | Maintains layout consistency; user understands control exists |
| Dimensions | 450×450 square | Accommodates central layout; distinctive aspect ratio |
| Typography | Serif | Matches scientific/botanical aesthetic |
| Animation | On freeze button | Clear state feedback; focal point for activity |

## Open Questions

- Exact organic button shape to be determined during mockup iteration
- Specific animation style (pulse vs shimmer vs other) to explore in mockup
- Precise placement of grain activity indicator
- Font selection from available serif options

## Asset Paths for Implementation

```
Background texture: /Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg
Anatomical overlay: /Users/taylorbrook/Dev/Ouaricon Audio Images/anatomy/muscles_histoirephysiqu911875gran_0161.png
```

## Parameter-UI Mapping (from parameter-spec.md)

| Parameter ID | Type | Range | UI Element | Relay Type |
|-------------|------|-------|------------|------------|
| FREEZE | Bool | On/Off | Organic animated button | Custom button relay |
| THRESHOLD | Float | -60 to 0 dB | Botanical rotary knob | WebSliderRelay |
| MODE | Choice | Manual/Threshold | Toggle button | WebToggleButtonRelay |
| DRIFT | Float | 0-100% | Botanical rotary knob | WebSliderRelay |
| MIX | Float | 0-100% | Botanical rotary knob | WebSliderRelay |

## Next Phase

Ready for: **Plan** or **UI Mockup** phase

The context is sufficient to proceed with either:
1. `/ui-mockup O-Freeze` - Create visual HTML mockup for iteration
2. `/plugin-plan O-Freeze 3-gui` - Create execution plan for WebView implementation
