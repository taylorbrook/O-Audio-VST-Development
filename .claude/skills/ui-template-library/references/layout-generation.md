# Layout Generation Logic

This document describes how to generate appropriate layouts when applying an aesthetic to a plugin with a specific parameter count.

## Core Principle

**Aesthetics define visual styling, layouts adapt to parameter count.**

When applying an aesthetic to a new plugin, the layout must be generated based on:
1. Number of parameters
2. Parameter types (rotary, slider, button, toggle, combo)
3. Parameter importance (prominent controls get special treatment)
4. Aesthetic spacing philosophy (tight/comfortable/generous)

## Layout Selection Algorithm

### 1-3 Parameters: Horizontal Layout

**Structure:**
- Single horizontal row
- Large controls with generous spacing
- Simple, focused interface

**Layout CSS:**
```css
.controls {
    display: flex;
    flex-direction: row;
    justify-content: center;
    align-items: center;
    gap: var(--control-gap);  /* From aesthetic spacing */
    padding: var(--section-padding);
}
```

**Window sizing:**
- Width: `(controlCount * controlSize) + (controlCount + 1) * gap + padding`
- Height: `controlSize + labelHeight + padding`
- Typical: ~400-500px wide, ~300-350px tall

**Example (3 knobs):**
```
┌─────────────────────────────────────┐
│         Plugin Name                  │
│                                      │
│   [Knob]    [Knob]    [Knob]        │
│   Param1    Param2    Param3        │
└─────────────────────────────────────┘
```

### 4-6 Parameters: Grid 2x3

**Structure:**
- 2 rows, up to 3 columns
- Balanced grid layout
- Room for control labels

**Layout CSS:**
```css
.controls {
    display: grid;
    grid-template-columns: repeat(3, auto);
    grid-template-rows: repeat(2, auto);
    gap: var(--control-gap);
    justify-content: center;
    align-items: center;
    padding: var(--section-padding);
}
```

**Window sizing:**
- Width: `(3 * controlSize) + (4 * gap) + padding`
- Height: `(2 * controlSize) + (3 * gap) + headerHeight + padding`
- Typical: ~500-600px wide, ~400-450px tall

**Example (6 knobs):**
```
┌─────────────────────────────────────┐
│         Plugin Name                  │
│                                      │
│   [Knob]  [Knob]  [Knob]            │
│   Param1  Param2  Param3            │
│                                      │
│   [Knob]  [Knob]  [Knob]            │
│   Param4  Param5  Param6            │
└─────────────────────────────────────┘
```

### 7-10 Parameters: Grid 3x3 or Sectioned

**Structure (Option A: 3x3 Grid):**
- 3 rows, 3 columns
- Uniform grid
- Some cells may be empty if < 9 params

**Layout CSS:**
```css
.controls {
    display: grid;
    grid-template-columns: repeat(3, auto);
    grid-template-rows: repeat(3, auto);
    gap: var(--control-gap);
    justify-content: center;
    padding: var(--section-padding);
}
```

**Structure (Option B: Sectioned with Prominent Sliders):**
- Main section: 2x3 or 3x2 grid (6 rotary knobs)
- Side section: 2-4 vertical sliders (Mix, Output, etc.)
- Visual separation between sections

**Layout CSS:**
```css
.container {
    display: flex;
    gap: var(--section-gap);
    padding: var(--section-padding);
}

.main-controls {
    display: grid;
    grid-template-columns: repeat(3, auto);
    grid-template-rows: repeat(2, auto);
    gap: var(--control-gap);
}

.output-controls {
    display: flex;
    flex-direction: row;
    gap: var(--control-gap);
    align-items: flex-start;
}
```

**Window sizing:**
- Width: ~600-700px
- Height: ~450-550px

**Example (8 params: 6 knobs + 2 sliders):**
```
┌────────────────────────────────────────────┐
│         Plugin Name                         │
│                                             │
│   [Knob]  [Knob]  [Knob]    [Slider] [Slider]│
│   Param1  Param2  Param3    Mix      Output │
│                                             │
│   [Knob]  [Knob]  [Knob]                   │
│   Param4  Param5  Param6                   │
└────────────────────────────────────────────┘
```

### 11+ Parameters: Multi-Section Layout

**Structure:**
- Multiple logical sections with headers
- Grouped related parameters
- May include scrolling if > 16 params
- Mix of control types

**Layout CSS:**
```css
.container {
    display: flex;
    flex-direction: column;
    gap: var(--section-gap);
    padding: var(--section-padding);
    max-height: 600px;
    overflow-y: auto;  /* If needed */
}

.section {
    border: 1px solid var(--border-color);
    padding: var(--section-padding);
    border-radius: var(--border-radius);
}

.section-controls {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(80px, auto));
    gap: var(--control-gap);
}
```

**Window sizing:**
- Width: ~700-800px
- Height: ~600-700px
- May include scrollbar

**Example (14 params: 3 sections):**
```
┌──────────────────────────────────────────┐
│         Plugin Name                       │
│                                           │
│ ┌─── Timing ────────────────────────┐   │
│ │ [Knob]  [Knob]  [Knob]  [Knob]    │   │
│ │ Time1   Time2   Time3   Time4     │   │
│ └───────────────────────────────────┘   │
│                                           │
│ ┌─── Modulation ────────────────────┐   │
│ │ [Knob]  [Knob]  [Knob]  [Knob]    │   │
│ │ Rate    Depth   Shape   Phase     │   │
│ └───────────────────────────────────┘   │
│                                           │
│ ┌─── Output ─────────────────────────┐  │
│ │ [Slider] [Slider] [Button] [Button] │  │
│ │ Mix      Level    Bypass   Phase  │  │
│ └───────────────────────────────────┘  │
└──────────────────────────────────────────┘
```

## Prominence Handling

**Prominent parameters** (Mix, Output, Master, Dry/Wet) get special treatment:
- Vertical sliders instead of knobs
- Larger size
- Positioned on right side or in dedicated section

**Detection:**
```javascript
function isProminentParameter(param) {
    const prominentNames = ['mix', 'output', 'master', 'dry', 'wet', 'gain', 'volume'];
    return prominentNames.some(name => param.id.toLowerCase().includes(name));
}
```

## Layout Generation Function

```javascript
function generateLayout(parameters, aesthetic) {
    const paramCount = parameters.length;
    const prominentParams = parameters.filter(isProminentParameter);
    const regularParams = parameters.filter(p => !isProminentParameter(p));

    if (paramCount <= 3) {
        return generateHorizontalLayout(parameters, aesthetic);
    } else if (paramCount <= 6) {
        return generateGridLayout(regularParams, prominentParams, 3, 2, aesthetic);
    } else if (paramCount <= 10) {
        if (prominentParams.length >= 2) {
            return generateSectionedLayout(regularParams, prominentParams, aesthetic);
        } else {
            return generateGridLayout(parameters, [], 3, 3, aesthetic);
        }
    } else {
        return generateMultiSectionLayout(parameters, aesthetic);
    }
}
```

## Control Sizing

Control sizes should adapt to layout density:

**Horizontal (1-3 params):**
- Large knobs: 80-100px diameter
- Large sliders: 200px height
- Generous spacing

**Grid 2x3 (4-6 params):**
- Medium knobs: 70-80px diameter
- Medium sliders: 180px height
- Comfortable spacing

**Grid 3x3 (7-10 params):**
- Standard knobs: 60-70px diameter
- Standard sliders: 150px height
- Standard spacing

**Multi-section (11+ params):**
- Compact knobs: 50-60px diameter
- Compact sliders: 120px height
- Tighter spacing

**Sizing logic:**
```javascript
function getControlSize(paramCount, baseSize) {
    if (paramCount <= 3) return baseSize * 1.3;  // 130%
    if (paramCount <= 6) return baseSize * 1.1;  // 110%
    if (paramCount <= 10) return baseSize;       // 100%
    return baseSize * 0.85;                      // 85%
}
```

## Spacing Adaptation

Aesthetic spacing philosophy affects final layout:

**Generous spacing (philosophy: "generous"):**
- Multiply gaps by 1.4
- More breathing room
- Feels spacious

**Comfortable spacing (philosophy: "comfortable"):**
- Use gaps as-is
- Balanced

**Tight spacing (philosophy: "tight"):**
- Multiply gaps by 0.7
- Compact
- Efficient use of space

```javascript
function applySpacingPhilosophy(gap, philosophy) {
    const multipliers = {
        'tight': 0.7,
        'comfortable': 1.0,
        'generous': 1.4,
    };
    return gap * (multipliers[philosophy] || 1.0);
}
```

## Window Size Calculation

```javascript
function calculateWindowSize(layout, controlSizes, aesthetic) {
    let width, height;

    switch (layout.type) {
        case 'horizontal':
            width = layout.columns * controlSizes.width +
                    (layout.columns + 1) * aesthetic.spacing.controlGap +
                    2 * aesthetic.spacing.edge_margin;
            height = controlSizes.height +
                     aesthetic.spacing.control_label_gap +
                     labelHeight +
                     2 * aesthetic.spacing.edge_margin +
                     headerHeight;
            break;

        case 'grid':
            width = layout.columns * controlSizes.width +
                    (layout.columns + 1) * aesthetic.spacing.controlGap +
                    2 * aesthetic.spacing.edge_margin;
            height = layout.rows * (controlSizes.height + labelHeight) +
                     (layout.rows + 1) * aesthetic.spacing.controlGap +
                     2 * aesthetic.spacing.edge_margin +
                     headerHeight;
            break;

        // ... more cases
    }

    // Apply suggested aspect ratio preference if needed
    if (aesthetic.suggested_window.aspect_ratio_preference === 'wider') {
        width = Math.max(width, height * 1.5);
    }

    return { width: Math.round(width), height: Math.round(height) };
}
```

## Summary

**Key principles:**
1. Parameter count determines layout structure
2. Control types affect layout choices (knobs vs sliders)
3. Prominence affects control sizing and positioning
4. Aesthetic spacing philosophy scales gaps
5. Control sizes adapt to layout density
6. Window size calculated from layout + spacing
7. Layout is generated fresh each time (not saved in aesthetic)
