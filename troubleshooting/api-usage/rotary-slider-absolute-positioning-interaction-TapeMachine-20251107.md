---
plugin: TapeMachine
date: 2025-11-07
problem_type: api_misuse
component: juce_gui_basics
symptoms:
  - Knobs jump to absolute mouse position instead of drag-to-adjust
  - Clicking at top of knob instantly sets value to maximum
  - Both VST3 and AU formats exhibit same broken behavior
  - No visual errors, but interaction feels wrong
root_cause: wrong_api
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [slider, rotary, interaction, drag, mouse-tracking, ui]
---

# Troubleshooting: Rotary Slider Using Absolute Positioning Instead of Drag Interaction

## Problem
Rotary knobs in plugin UI respond to absolute mouse position rather than click-and-drag gestures. Clicking anywhere on the knob jumps the value to that position instead of requiring the user to drag up/down to adjust.

## Environment
- JUCE Version: 8.0.9
- Affected: PluginEditor.cpp setupKnob() method, all rotary sliders
- Plugin: TapeMachine v1.1.0
- Date: 2025-11-07

## Symptoms
- Clicking at the top of a knob instantly sets value to 100%
- Clicking at the bottom instantly sets value to 0%
- No drag gesture required - mouse position directly maps to value
- Behavior is consistent across VST3 and AU formats
- Other plugins (like Shake) work correctly with drag-to-adjust

## What Didn't Work

**Attempted Solution 1:** Added `setRotaryParameters()` to configure rotation angles
- **Why it failed:** `setRotaryParameters()` only controls the visual arc angles (start/end rotation), not the interaction mode. It has no effect on whether the slider uses absolute or relative positioning.

**Investigation:** Compared with working plugin (Shake)
- Found Shake uses `Slider::RotaryHorizontalVerticalDrag`
- TapeMachine was using `Slider::Rotary`

## Solution

Change the slider style from `Slider::Rotary` to `Slider::RotaryHorizontalVerticalDrag`.

**Code changes:**
```cpp
// plugins/TapeMachine/Source/PluginEditor.cpp

// Before (broken):
void TapeMachineAudioProcessorEditor::setupKnob(juce::Slider& knob, ...)
{
    knob.setSliderStyle(juce::Slider::Rotary);  // ❌ Absolute positioning
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters(...);  // Irrelevant to interaction mode
}

// After (fixed):
void TapeMachineAudioProcessorEditor::setupKnob(juce::Slider& knob, ...)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);  // ✓ Drag-to-adjust
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    // setRotaryParameters() removed - not needed for basic rotary behavior
}
```

After changing the code:
```bash
./scripts/build-and-install.sh TapeMachine
# Quit DAW, rescan plugins, reload
```

## Why This Works

JUCE Slider has multiple style enums that control interaction behavior:

**`Slider::Rotary`:**
- Mouse position maps directly to slider value (absolute positioning)
- Clicking at any position jumps the value to that position
- Used for "snap-to" or "click-anywhere" interfaces
- No drag gesture required

**`Slider::RotaryHorizontalVerticalDrag`:**
- Click-and-drag interaction (relative positioning)
- Dragging up increases value, dragging down decreases
- Initial click position doesn't affect value
- Standard expected behavior for audio plugin knobs

The confusion arose because:
1. `Slider::Rotary` sounds like the correct choice for rotary knobs
2. The visual appearance is identical (both show circular knobs)
3. The difference is only in interaction behavior, not visual style

**Key insight:** `setRotaryParameters()` controls the visual arc (rotation angles), while `setSliderStyle()` controls interaction behavior. They're independent settings.

## Prevention

**When creating rotary knobs in JUCE plugins:**

1. **Always use `Slider::RotaryHorizontalVerticalDrag` for audio plugin knobs**
   - This is the standard interaction users expect
   - Provides precise control without accidental value jumps

2. **Reserve `Slider::Rotary` for special cases:**
   - Click-to-set interfaces (e.g., visual equalizer band selection)
   - Interfaces where absolute positioning makes sense
   - Non-real-time controls where jumping is acceptable

3. **Test interaction in your DAW before finalizing:**
   - Try clicking at different positions on knobs
   - Verify drag gesture works as expected
   - Compare with professional plugins for reference

4. **Reference working plugins:**
   - When unsure about slider styles, check existing plugins in your codebase
   - Copy the pattern from plugins with correct behavior

5. **Common slider styles for audio plugins:**
   - Rotary knobs: `RotaryHorizontalVerticalDrag`
   - Faders: `LinearVertical` or `LinearHorizontal`
   - Both provide relative positioning (drag-to-adjust)

## Related Issues

- See also: invisible-rotary-sliders.md - Related to rotary slider rendering issues
