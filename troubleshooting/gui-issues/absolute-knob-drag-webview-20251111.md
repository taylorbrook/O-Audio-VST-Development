---
plugin: JUCE
date: 2025-11-11
problem_type: ui_layout
component: webview
symptoms:
  - Rotary knobs jump to mouse cursor position instead of dragging relative to start point
  - Knob behavior feels unnatural and imprecise
  - User must move mouse to exact target position instead of dragging incrementally
root_cause: logic_error
juce_version: 8.0.x
resolution_type: code_fix
severity: moderate
tags: [webview, knob, drag, interaction, ux, ui-controls]
---

# Troubleshooting: Absolute vs Relative Knob Drag Behavior

## Problem
WebView UI knobs implemented with absolute positioning logic instead of relative drag, causing unnatural interaction where knob value jumps to match mouse Y-position rather than incrementing based on drag distance.

## Environment
- Plugin: Generic WebView UI pattern
- JUCE Version: 8.0.x
- Affected: WebView knob controls, all UI mockups
- Date: 2025-11-11

## Symptoms
- Clicking and dragging knob causes value to jump to cursor position
- Cannot make small precise adjustments
- Interaction feels broken and unpredictable
- User must move mouse cursor to exact position where they want knob, not drag relative to starting point
- Common complaint: "sometimes plugins do this and I hate this kind of knob behavior"

## What Didn't Work

**Attempted Solution 1:** Initial implementation used absolute positioning
```javascript
const deltaY = startY - e.clientY;  // Total distance from start
const newRotation = startRotation + deltaY;
```
- **Why it failed:** This calculates total distance from initial click point, causing knob to track absolute mouse position. The `deltaY` value grows continuously as mouse moves away from start, making rotation directly proportional to cursor Y-coordinate rather than drag distance since last frame.

## Solution

Replace absolute positioning with **relative drag** (delta drag / virtual slider pattern).

**Code changes:**
```javascript
// Before (broken - absolute positioning):
knobs.forEach(knob => {
    let isDragging = false;
    let startY = 0;
    let startRotation = 0;

    const indicator = knob.querySelector('.knob-indicator');

    knob.addEventListener('mousedown', (e) => {
        isDragging = true;
        startY = e.clientY;
        const transform = window.getComputedStyle(indicator).transform;
        const matrix = new DOMMatrix(transform);
        startRotation = Math.atan2(matrix.m21, matrix.m11) * (180 / Math.PI);
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;

        const deltaY = startY - e.clientY;  // ❌ Absolute - total distance from start
        const newRotation = Math.max(-135, Math.min(135, startRotation + deltaY));
        indicator.style.transform = `translateX(-50%) rotate(${newRotation}deg)`;
    });

    document.addEventListener('mouseup', () => {
        isDragging = false;
    });
});

// After (fixed - relative drag):
knobs.forEach(knob => {
    let isDragging = false;
    let lastY = 0;  // ✅ Track last frame position, not start position
    let rotation = 0;  // ✅ Maintain current rotation state

    const indicator = knob.querySelector('.knob-indicator');

    // Extract initial rotation from inline style
    const initialTransform = indicator.style.transform;
    const match = initialTransform.match(/rotate\(([^)]+)deg\)/);
    if (match) {
        rotation = parseFloat(match[1]);
    }

    knob.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastY = e.clientY;  // ✅ Store current Y, not start Y
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;

        // ✅ Relative drag: increment based on distance since LAST FRAME
        const deltaY = lastY - e.clientY;
        rotation += deltaY * 0.5;  // Sensitivity factor
        rotation = Math.max(-135, Math.min(135, rotation));

        indicator.style.transform = `translateX(-50%) rotate(${rotation}deg)`;
        lastY = e.clientY;  // ✅ Update for next frame
    });

    document.addEventListener('mouseup', () => {
        isDragging = false;
    });
});
```

## Why This Works

**Root cause:** Absolute positioning tracks total distance from initial click point, making knob rotation directly proportional to mouse Y-coordinate. This creates "jump to cursor" behavior.

**Solution mechanism:**
1. **Frame-to-frame delta:** `deltaY = lastY - e.clientY` calculates movement since LAST frame, not since start
2. **Incremental updates:** `rotation += deltaY * sensitivity` adds to current rotation rather than replacing it
3. **State persistence:** `lastY = e.clientY` updates tracking variable each frame
4. **Current state maintenance:** `rotation` variable stores cumulative rotation across all frames

**Why this feels natural:**
- Drag up = value increases (deltaY positive when mouse moves up)
- Drag down = value decreases (deltaY negative when mouse moves down)
- Distance dragged = amount of change (proportional relationship)
- Absolute cursor position irrelevant (can drag infinitely in same direction)

**Industry standard:** Pro Tools, Logic Pro, Ableton Live, and virtually all professional VST/AU plugins use relative drag for rotary controls.

## Prevention

**When implementing WebView knob controls:**

1. **Always use relative drag pattern:**
   - Track `lastY` (previous frame position), not `startY` (initial click position)
   - Calculate `deltaY = lastY - e.clientY` each frame
   - Increment rotation: `rotation += deltaY * sensitivity`
   - Update tracking: `lastY = e.clientY`

2. **Never use absolute positioning:**
   - Don't calculate total distance from start point
   - Don't make rotation proportional to mouse Y-coordinate
   - Don't use `startRotation + totalDistance` pattern

3. **Test interaction feel:**
   - Can you make small precise adjustments?
   - Does dragging up/down consistently increase/decrease value?
   - Can you drag beyond knob visual bounds and keep changing value?
   - Does it feel like professional plugin controls?

4. **Sensitivity tuning:**
   - Start with `deltaY * 0.5` (half pixel = half degree)
   - Adjust based on knob size and expected range
   - Larger knobs = lower sensitivity for precision
   - Smaller ranges = higher sensitivity for coverage

5. **Document in aesthetic guidelines:**
   - Include relative drag pattern in all WebView UI aesthetic templates
   - Add code example to "Interaction Feel" section
   - Reference this issue in implementation checklist

## Related Issues

No related issues documented yet.

## References

- **Interaction pattern:** Relative drag / delta drag / virtual slider
- **Also called:** Frame-delta input, incremental drag
- **Opposite (bad) pattern:** Absolute positioning, jump-to-cursor
- **Industry examples:** Pro Tools, Logic Pro, Ableton Live, FabFilter, Waves, Soundtoys
