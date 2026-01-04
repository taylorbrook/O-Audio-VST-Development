# WebView Knob Hover Shake (Micro-Scale Transform)

**Date:** 2025-11-12
**Category:** GUI Issues
**Severity:** Minor (visual artifact)
**Affected Plugins:** DriveVerb, FlutterVerb, TapeAge (all WebView-based UIs)
**Status:** Resolved

---

## Problem

Subtle shaking/jittering when hovering mouse over knobs in WebView UI. Almost imperceptible vertical movement, as if the plugin window were experiencing a "floor shake." Only occurred on knobs, not on other UI elements like toggle switches.

### Symptoms

- Knobs visually shake up and down when mouse hovers over them
- Shake is continuous while hovering (not a discrete event)
- Very subtle but noticeable under scrutiny
- Does NOT happen on toggle switches or other controls
- Click/drag interaction works normally
- `:active` depression effect (click) works without issue

### User Report

> "There's some very subtle strange things that happen with the UI where if I move my mouse over it. Specifically moving my mouse over a knob. When I move my mouse over a knob, the UI very subtly shakes up and down. It's almost imperceptible, but it only happens when my mouse goes over one of the knobs. It doesn't happen on the filter switch."

---

## Root Cause

**CSS micro-scale transform causing sub-pixel rendering instability:**

```css
.knob:hover {
    transform: scale(1.005);  /* ❌ PROBLEM: 0.5% scale increase */
}
```

**Why this causes shake:**

1. **Micro-scaling (<1%):** 0.5% scale increase is barely perceptible but triggers continuous rendering
2. **Sub-pixel positioning:** WebView recalculates layout with fractional pixel coordinates
3. **Render loop instability:** Browser continuously adjusts position during hover (not a one-time change)
4. **WebBrowserComponent limitation:** JUCE's WebView wrapper struggles with micro-transforms more than native browsers
5. **Transition amplification:** Combined with `transition: transform 0.05s ease`, the animation never fully settles

**Why `:active` (click) doesn't shake:**

```css
.knob:active {
    transform: scale(0.995);  /* ✓ Works fine - discrete action */
}
```

- Click is a discrete event (mouse down → mouse up)
- Larger scale change (0.5% shrink) completes quickly
- No continuous recalculation during the action

**Why toggle switch doesn't shake:**

```css
.vertical-toggle-switch:hover {
    border-color: #c49564;  /* ✓ No transform, just color change */
}
```

- Border color changes are handled efficiently by rendering pipeline
- No layout recalculation required
- No sub-pixel positioning issues

---

## Solution

**Replace scale transform with border color change:**

### Before (Problematic)

```css
.knob {
    width: 90px;
    height: 90px;
    transition: transform 0.05s ease;
}

.knob:hover {
    transform: scale(1.005);  /* ❌ Causes shake */
}

.knob:active {
    transform: scale(0.995);  /* ✓ Keep this - works fine */
}

.knob-body {
    /* No border */
    background: radial-gradient(...);
}
```

### After (Fixed)

```css
.knob {
    width: 90px;
    height: 90px;
    transition: transform 0.05s ease;
}

.knob:hover .knob-body {
    border-color: #c49564;  /* ✓ Gold accent on hover */
}

.knob:active {
    transform: scale(0.995);  /* ✓ Keep depression effect */
}

.knob-body {
    border: 2px solid rgba(212, 165, 116, 0.3);  /* ✓ Subtle default border */
    background: radial-gradient(...);
    transition: border-color 0.1s ease;  /* ✓ Smooth color transition */
}
```

---

## Implementation

### Files Changed (per plugin)

```
plugins/[PluginName]/Source/ui/public/index.html
```

### CSS Changes

1. **Remove scale transform from hover:**
   ```css
   /* REMOVE */
   .knob:hover {
       transform: scale(1.005);
   }
   ```

2. **Add border to knob-body:**
   ```css
   .knob-body {
       border: 2px solid rgba(212, 165, 116, 0.3);  /* Subtle bronze default */
       transition: border-color 0.1s ease;
       /* ... existing styles ... */
   }
   ```

3. **Add hover effect to knob-body:**
   ```css
   .knob:hover .knob-body {
       border-color: #c49564;  /* Gold accent (matches toggle switch) */
   }
   ```

4. **Keep active depression effect:**
   ```css
   .knob:active {
       transform: scale(0.995);  /* User liked this - keep it */
   }
   ```

---

## Technical Details

### Why Micro-Transforms Fail in WebView

**Browser rendering pipeline:**
1. Layout calculation (position, size)
2. Paint (draw pixels)
3. Composite (combine layers)

**Micro-scale transform (<1%) issues:**
- Forces continuous layout recalculation during hover
- Sub-pixel coordinates (e.g., 90.45px) cause rounding instability
- WebBrowserComponent's rendering thread struggles with micro-adjustments
- Native browsers handle this better but still not ideal

**Border color changes:**
- Skip layout recalculation (no size/position change)
- Handled in paint phase only
- GPU-accelerated color interpolation
- No sub-pixel positioning issues

### Scale Transform Guidelines for WebView

**AVOID:**
- Micro-scales (<2%): `scale(1.005)`, `scale(0.99)` ❌
- Continuous hover transforms with small values ❌

**OK TO USE:**
- Larger scales (>2%): `scale(1.05)`, `scale(0.95)` ✓
- Discrete action transforms (click/active): `scale(0.995)` ✓
- Non-hover contexts: animations, loading states ✓

**PREFER:**
- Color changes: `border-color`, `background-color` ✓✓
- Opacity changes: `opacity: 0.8` ✓
- Box-shadow changes: `box-shadow: 0 0 8px ...` ✓

---

## Validation

### Testing Checklist

- [x] Hover over each knob - no shake/jitter
- [x] Click knob - depression effect still works
- [x] Border appears on hover with gold accent
- [x] Border subtle when not hovering (bronze tint)
- [x] Smooth transition between states
- [x] No performance impact (CPU/GPU usage)
- [x] Works across all plugins (DriveVerb, FlutterVerb, TapeAge)

### Affected Plugins

| Plugin | Version | Status |
|--------|---------|--------|
| DriveVerb | v1.0.2 | ✓ Fixed, tested, installed |
| FlutterVerb | v1.0.3 | ✓ Fixed, tested, installed |
| TapeAge | v1.0.3 | ✓ Fixed, tested, installed |

---

## Prevention

### Pattern for Future WebView UIs

**Hover effects - Use border/color changes:**

```css
.interactive-control {
    border: 2px solid rgba(accent-color, 0.3);
    transition: border-color 0.1s ease;
}

.interactive-control:hover {
    border-color: accent-color;  /* ✓ Stable */
}
```

**Click effects - Larger scale transforms OK:**

```css
.interactive-control:active {
    transform: scale(0.95);  /* ✓ OK - discrete action, larger scale */
}
```

**Animation effects - Non-hover transforms OK:**

```css
@keyframes pulse {
    0%, 100% { transform: scale(1); }
    50% { transform: scale(1.05); }  /* ✓ OK - not hover-triggered */
}
```

---

## Related Issues

- **Pattern applies to:** All WebView-based UIs using micro-scale transforms
- **JUCE version:** 8.x (WebBrowserComponent)
- **Platform:** macOS (likely affects all platforms)

### Similar Symptoms to Watch For

- UI elements "breathing" or "pulsing" on hover
- Subtle position shifts during mouse movement
- Jitter during drag operations with hover effects
- Text rendering instability on hover

If you see these symptoms, check for:
1. Micro-scale transforms (<2%) on `:hover`
2. Sub-pixel positioning from fractional dimensions
3. Nested transforms (parent + child both using transform)

---

## Research Notes

### Investigation Process

1. **Level 1 research (5 min):** Found exact CSS rule via grep
2. **Verification:** Checked all affected plugins had same pattern
3. **Root cause:** Identified sub-pixel rendering issue with WebView
4. **Solution:** Borrowed pattern from working toggle switch
5. **Validation:** Applied to all 3 plugins, rebuilt, tested

### Why This Was Easy to Fix

- Consistent pattern across all plugins (same UI system)
- Working alternative already in codebase (toggle switch)
- Visual-only change (no DSP/parameter impact)
- Zero backward compatibility concerns

---

## Keywords

`WebView`, `hover`, `shake`, `jitter`, `transform`, `scale`, `micro-scale`, `sub-pixel`, `rendering`, `knob`, `CSS`, `border-color`, `WebBrowserComponent`, `JUCE`, `UI stability`

---

## References

- **Fixed in commits:**
  - DriveVerb v1.0.2: `fix(DriveVerb): v1.0.2 - eliminate knob hover shake`
  - FlutterVerb v1.0.3: See multi-plugin fix commit
  - TapeAge v1.0.3: See multi-plugin fix commit

- **CHANGELOG entries:**
  - `plugins/DriveVerb/CHANGELOG.md` - [1.0.2]
  - `plugins/FlutterVerb/CHANGELOG.md` - [1.0.3]
  - `plugins/TapeAge/CHANGELOG.md` - [1.0.3]

- **PLUGINS.md updated:**
  - DriveVerb: v1.0.1 → v1.0.2
  - FlutterVerb: v1.0.2 → v1.0.3
  - TapeAge: v1.0.2 → v1.0.3
