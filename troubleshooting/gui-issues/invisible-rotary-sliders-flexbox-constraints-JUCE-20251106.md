---
plugin: JUCE
date: 2025-11-06
problem_type: ui_layout
component: juce_gui_basics
symptoms:
  - Rotary sliders not visible in plugin UI
  - Labels visible but no knob graphics rendered
  - UI appeared empty with just text
root_cause: missing_constraint
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [flexbox, rotary-slider, layout, sizing, dimensions, ui]
---

# Troubleshooting: Invisible Rotary Sliders in JUCE

## Problem

Rotary sliders configured with `juce::Slider::RotaryHorizontalVerticalDrag` were not visible in the plugin UI. Labels appeared correctly, but the actual knob graphics were missing.

**Symptoms:**
- Labels visible and positioned correctly
- Text boxes below knobs visible
- No visual knob/dial graphics rendered
- UI appeared mostly empty with just text

## Root Cause

**FlexBox with `.withFlex()` doesn't provide explicit dimensions to child components.** Rotary sliders require concrete width/height values to render their circular graphics properly.

## What Didn't Work

### Attempt 1: Adding Colors
```cpp
slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a9eff));
slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
// ... more colors
```

**Result:** Still invisible. Colors only matter if the slider has dimensions to render into.

**Why it failed:** The sliders had 0x0 or minimal bounds, so there was no area to draw the rotary graphics regardless of color.

## Solution

Use `.withWidth()` and `.withHeight()` instead of `.withFlex()` for rotary sliders in FlexBox:

### Before (Broken):
```cpp
juce::FlexBox fb;
fb.flexDirection = juce::FlexBox::Direction::row;
fb.items.add(juce::FlexItem(roomSizeSlider).withFlex(1).withMargin(10));
fb.items.add(juce::FlexItem(dampingSlider).withFlex(1).withMargin(10));
// ...
fb.performLayout(area);
```

### After (Working):
```cpp
const int knobSize = 80;
const int margin = 10;

juce::FlexBox fb;
fb.flexDirection = juce::FlexBox::Direction::row;
fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
fb.alignItems = juce::FlexBox::AlignItems::center;

fb.items.add(juce::FlexItem(roomSizeSlider).withWidth(knobSize).withHeight(knobSize + 40).withMargin(margin));
fb.items.add(juce::FlexItem(dampingSlider).withWidth(knobSize).withHeight(knobSize + 40).withMargin(margin));
// ...
fb.performLayout(area);
```

**Key changes:**
- `.withWidth(knobSize)` - Explicit width for the knob
- `.withHeight(knobSize + 40)` - Height includes knob (80px) + text box below (40px)
- Removed `.withFlex(1)` - No longer using flex grow

## Why This Works

Rotary sliders need to know their rendering bounds to:
1. Calculate the circular arc diameter
2. Position the thumb indicator
3. Draw the fill and outline within a defined area

FlexBox's `.withFlex()` only controls growth/shrink ratios, not absolute dimensions. For components that need concrete sizes (like rotary sliders), use `.withWidth()` and `.withHeight()`.

## When to Use Each

### Use `.withFlex()` for:
- Linear sliders that can stretch
- Labels that can resize
- Containers that should grow/shrink proportionally

### Use `.withWidth()` / `.withHeight()` for:
- Rotary sliders (need circular bounds)
- Buttons with specific sizes
- Components with aspect ratio requirements
- Any component that doesn't look good when stretched

## Recommended Sizes

For rotary sliders with text box below:
- **Knob diameter:** 60-100px (80px is standard)
- **Text box height:** 20-40px (for value display)
- **Total height:** knobSize + textBoxHeight
- **Spacing:** 10-15px margin between knobs

## Files Changed

- `plugins/ReverbPlugin/Source/PluginEditor.cpp` - Fixed FlexBox layout with explicit dimensions
- Added color configuration (though this wasn't the core issue)

## Prevention

When using FlexBox with rotary sliders:
1. Always use `.withWidth()` and `.withHeight()`
2. Test UI immediately after layout changes
3. Use Standalone builds for quick visual feedback

## Related Issues

- Linear sliders work fine with `.withFlex()` because they can stretch in one dimension
- Grid layout would also work, but FlexBox is more flexible for responsive designs
- Using `setBounds()` directly would work but violates the "no setBounds" UI rule
