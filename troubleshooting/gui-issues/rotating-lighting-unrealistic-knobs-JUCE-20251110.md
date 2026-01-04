---
plugin: JUCE
date: 2025-11-10
problem_type: ui_layout
component: webview
symptoms:
  - Knob lighting/shadows rotate with the control
  - Highlights move as knob turns
  - Unrealistic lighting behavior
  - Visual confusion during interaction
root_cause: logic_error
resolution_type: code_fix
severity: minor
tags: [ui, webview, css, lighting, knobs, visual-design, layers]
---

# Rotating Lighting on Knobs (Unrealistic Light Behavior)

## Problem

When implementing rotary knobs in WebView UIs, lighting effects (highlights, shadows, gradients) often rotate with the knob as the user turns it. This creates an unrealistic appearance where the light source appears to move with the control, rather than staying fixed in the environment.

**Visual symptoms:**
- Highlights on knob surface rotate as knob turns
- Shadows move with knob rotation
- Gradients/shine effects follow the indicator
- Breaks physical realism - real knobs have fixed lighting from room light source

## Environment

- **Component:** WebView UI with rotary knobs (HTML/CSS)
- **Manifestation:** Any plugin with CSS-based rotary controls
- **Impact:** Visual polish, user experience, professional appearance

## Why This Happens

**Root cause:** All visual effects (gradients, shadows, highlights) placed on the same DOM element that rotates.

**Common implementation pattern that causes this:**

```html
<div class="knob">
  <div class="knob-rotatable">
    <!-- Gradients, shadows, AND indicator all rotate together -->
  </div>
</div>
```

When `knob-rotatable` has `transform: rotate()`, everything inside rotates - including lighting effects that should stay fixed.

## Failed Approaches

### 1. Counter-rotating the lighting
Attempting to rotate lighting effects in the opposite direction to "cancel out" rotation. Complex, imprecise, creates jittery appearance.

### 2. JavaScript-based lighting updates
Recalculating gradient positions on every frame. Performance-intensive, unnecessary complexity.

### 3. Single-layer design
Trying to keep everything on one element and selectively rotate parts. CSS limitations make this impossible.

## Solution

**Separate static and rotating layers:**

```html
<div class="knob">
  <div class="knob-body">
    <!-- STATIC: Gradients, shadows, highlights stay here -->
  </div>
  <div class="knob-rotatable">
    <!-- ROTATES: Only the indicator line -->
  </div>
</div>
```

**CSS implementation:**

```css
.knob-body {
  /* Fixed lighting from top-left (room light) */
  background: radial-gradient(circle at 35% 35%, #lighter 0%, #darker 100%);
  box-shadow:
    0 8px 16px rgba(0, 0, 0, 0.8),  /* Drop shadow (fixed) */
    inset 0 4px 8px rgba(0, 0, 0, 0.6),  /* Top shadow (fixed) */
    inset 0 -2px 4px rgba(255, 255, 255, 0.15);  /* Bottom highlight (fixed) */
  position: absolute;
  /* DOES NOT ROTATE */
}

.knob-body::before {
  /* Center brushed metal texture (fixed) */
  background: radial-gradient(circle at 40% 40%, ...);
  /* Stays in place as knob turns */
}

.knob-rotatable {
  transform-origin: center center;
  transform: rotate(0deg);  /* Only this rotates */
  /* Contains ONLY the indicator line */
}

.knob-rotatable::after {
  /* Gold indicator line */
  background: linear-gradient(180deg, #gold 0%, #bronze 100%);
  /* Rotates with knob position */
}
```

**Key principle:** If it represents light hitting the knob from the room, it stays fixed. If it represents the knob's current position, it rotates.

## Technical Explanation

**Physical realism:**
- Real knobs in studios have fixed overhead lighting
- Light source doesn't move when you turn the knob
- Only the position indicator changes
- Shadows cast on the panel stay in the same direction

**Implementation strategy:**
1. **knob-body** = The physical knob surface with fixed material properties
   - Gradients showing roundness
   - Shadows showing depth
   - Highlights showing light reflection
2. **knob-rotatable** = Only the position indicator
   - Single line or marker
   - Rotates to show current value
   - No lighting effects

**Z-ordering:**
```
knob-body (z: 1) - Fixed lighting
  └─ knob-body::before - Fixed center texture
knob-rotatable (z: 2) - Rotating indicator
  └─ knob-rotatable::after - Indicator line
```

## Prevention

**Design checklist for rotary knobs:**

1. ✓ Separate static and rotating layers in HTML structure
2. ✓ Place ALL lighting effects (gradients, shadows, highlights) on static layer
3. ✓ Place ONLY position indicator on rotating layer
4. ✓ Test by rotating knob - lighting should stay fixed
5. ✓ Imagine a real hardware unit with overhead lights - replicate that behavior

**Common mistake patterns to avoid:**
- Putting pseudo-elements with gradients on rotating layer
- Using `transform: rotate()` on elements with `box-shadow`
- Placing center texture on rotating layer
- Not testing visual behavior during rotation

## Code Template

```html
<div class="knob" data-param="drive">
  <!-- Layer 1: Fixed lighting -->
  <div class="knob-body"></div>

  <!-- Layer 2: Rotating indicator only -->
  <div class="knob-rotatable"></div>
</div>
```

```css
/* STATIC LAYER - Lighting stays fixed */
.knob-body {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  position: absolute;

  /* Main surface gradient (light from top-left) */
  background: radial-gradient(circle at 35% 35%, #lighter, #darker);

  /* Shadows (fixed) */
  box-shadow:
    0 8px 16px rgba(0, 0, 0, 0.8),  /* Drop */
    inset 0 4px 8px rgba(0, 0, 0, 0.6),  /* Recess */
    inset 0 -2px 4px rgba(255, 255, 255, 0.15);  /* Highlight */
}

.knob-body::before {
  content: '';
  /* Center circle with material texture (fixed) */
  background: radial-gradient(circle at 40% 40%, ...);
}

/* ROTATING LAYER - Position indicator only */
.knob-rotatable {
  width: 100%;
  height: 100%;
  position: absolute;
  transform-origin: center center;
  transform: rotate(0deg);  /* JavaScript updates this */
  transition: transform 0.05s ease-out;
}

.knob-rotatable::after {
  content: '';
  /* Indicator line (rotates) */
  position: absolute;
  top: 8%;
  left: 50%;
  transform: translateX(-50%);
  width: 4px;
  height: 35px;
  background: linear-gradient(180deg, #gold, #bronze);
  border-radius: 2px;
}
```

```javascript
function updateKnobVisual(knob, value) {
  const rotatable = knob.querySelector('.knob-rotatable');
  const angle = -135 + (value * 270);  // -135° to +135°
  rotatable.style.transform = `rotate(${angle}deg)`;
}
```

## Related Issues

- Rotating button textures (same principle applies)
- Moving fader highlights (should be fixed to track, not fader thumb)
- Meter needle shadows (shadow should stay fixed, only needle rotates)
- Any control where material/lighting appears to move with interaction

## Success Criteria

Knob lighting is correct when:
- ✓ Turn knob left/right - highlights stay in same position
- ✓ Shadows remain consistent with room lighting angle
- ✓ Only the indicator line/mark changes position
- ✓ Looks like physical hardware with overhead studio lights
- ✓ No visual jarring or confusion during interaction

## References

- TapeAge v3 mockup implementation (correct example)
- TapeAge v2 mockup (incorrect - entire surface rotated)
- Real hardware photos: Studio knobs under fixed lighting
