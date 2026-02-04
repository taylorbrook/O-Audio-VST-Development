# Stage 3: GUI Implementation - Execution Plan

**Plugin:** O-FreqPulse
**Stage:** 3 (GUI)
**Phase:** PLAN
**Date:** 2026-02-03

---

## Goal

Implement the WebView-based GUI for O-FreqPulse featuring:
- 4-band × 32-step interactive grid with brightness-based gain display
- Real-time playhead synchronization via C++ timer
- Per-band Euclidean control panels (accordion style)
- Global controls footer (Mix, Rate, Swing, Smoothing, Steps)
- Naturalist aesthetic (paper texture, earthy colors, serif fonts)

Window size: **850 × 550 pixels**

---

## Tasks

### Task 1: Create UI Resource Directory Structure
- **Files to create:**
  - `Resources/ui/index.html`
  - `Resources/ui/css/styles.css`
  - `Resources/ui/js/app.js`
  - `Resources/ui/js/juce/index.js` (copy from O-Bells)
  - `Resources/ui/js/juce/check_native_interop.js` (copy from O-Bells)
- **Depends on:** None
- **Notes:** Standard O-series WebView file structure

### Task 2: Update CMakeLists.txt for BinaryData
- **Files to modify:** `CMakeLists.txt`
- **Changes:**
  - Enable `juce_add_binary_data()` for UI resources
  - Link `O-FreqPulse_UIResources` target
- **Depends on:** Task 1

### Task 3: Create WebView Relays for Global Parameters (5)
- **Files to modify:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Parameters:**
  - `mix` → WebSliderRelay
  - `steps` → WebComboBoxRelay
  - `rate` → WebComboBoxRelay
  - `swing` → WebSliderRelay
  - `smoothing` → WebSliderRelay
- **Depends on:** None
- **Notes:** Follow Critical Pattern #11 (unique_ptr member order)

### Task 4: Create WebView Relays for Per-Band Parameters (32)
- **Files to modify:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Parameters per band (×4):**
  - `band{N}_enable` → WebToggleRelay
  - `band{N}_depth` → WebSliderRelay
  - `band{N}_mode` → WebComboBoxRelay (Manual/Euclidean)
  - `band{N}_euc_steps` → WebSliderRelay
  - `band{N}_euc_pulses` → WebSliderRelay
  - `band{N}_euc_offset` → WebSliderRelay
  - `band{N}_low` / `band{N}_high` → display only (no relay, fixed v1.0)
- **Depends on:** None
- **Notes:** 24 relays total (6 per band × 4 bands)

### Task 5: Create WebView Relays for Step Grid (128)
- **Files to modify:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Parameters:** `step_b{0-3}_s{0-31}` → WebToggleRelay (128 total)
- **Depends on:** None
- **Notes:** Batch-create in loop; monitor startup performance

### Task 6: Initialize WebBrowserComponent with Resource Provider
- **Files to modify:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Changes:**
  - Create `getResource()` method with explicit URL mapping (Critical Pattern #8)
  - Create `webView` unique_ptr with all relay options
  - Add `withNativeFunction("getCurrentStep")` for playhead sync
  - Add `withNativeFunction("getGridState")` for bulk grid read
- **Depends on:** Tasks 3, 4, 5

### Task 7: Create WebView Attachments for All Parameters
- **Files to modify:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Changes:**
  - Create `WebSliderParameterAttachment` for slider relays
  - Create `WebToggleButtonParameterAttachment` for toggle relays
  - Create `WebComboBoxParameterAttachment` for combo relays
- **Depends on:** Task 6
- **Notes:** Must be created AFTER webView (Critical Pattern #11)

### Task 8: Add Timer for Playhead Updates
- **Files to modify:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Changes:**
  - Inherit from `juce::Timer`
  - Implement `timerCallback()` to call `evaluateJavascript()` with current step
  - Start timer at 30-60Hz in constructor
- **Depends on:** Task 6
- **Notes:** Follow O-Bells meter pattern

### Task 9: Add getCurrentStep() Public Method to Processor
- **Files to modify:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Changes:**
  - Add `std::atomic<int> currentStepAtomic` member
  - Update `currentStepAtomic` in processBlock
  - Add `int getCurrentStep() const` public method
- **Depends on:** None
- **Notes:** Provides thread-safe step access for editor timer

### Task 10: Implement index.html with Naturalist Layout
- **Files to modify:** `Resources/ui/index.html`
- **Structure:**
  - Header with plugin name
  - Grid container (4 band rows × 32 steps + playhead)
  - Euclidean panel (accordion, hidden by default)
  - Footer with global controls
- **Depends on:** Task 1

### Task 11: Implement styles.css with Naturalist Aesthetic
- **Files to modify:** `Resources/ui/css/styles.css`
- **Includes:**
  - Paper texture background (#F5E6D3)
  - Dark grid area (#1a1410)
  - Step on/off colors (#5a7a6a accent green)
  - Playhead styling with glow
  - Slider/knob styling (O-Detune pattern)
  - Accordion panel transitions
  - Native feel (user-select: none)
- **Depends on:** Task 1

### Task 12: Implement app.js - JUCE Bridge Initialization
- **Files to modify:** `Resources/ui/js/app.js`
- **Features:**
  - Import JUCE bridge functions
  - Initialize SliderState for all sliders
  - Initialize ToggleState for all toggles (128 step + 4 band enable)
  - Initialize ComboBoxState for all combos
  - Wire valueChangedEvent listeners for UI updates
- **Depends on:** Tasks 1, 10

### Task 13: Implement app.js - Grid Rendering
- **Files to modify:** `Resources/ui/js/app.js`
- **Features:**
  - Generate 4 band rows × 32 step cells
  - Bind click handlers for step toggles
  - Update cell brightness based on gain value
  - Handle variable step count (4, 8, 16, 32)
- **Depends on:** Task 12

### Task 14: Implement app.js - Playhead Synchronization
- **Files to modify:** `Resources/ui/js/app.js`
- **Features:**
  - `window.updatePlayhead(step)` function (called from C++ timer)
  - CSS transform for smooth positioning
  - Handle step count changes (reposition when steps param changes)
- **Depends on:** Tasks 8, 13

### Task 15: Implement app.js - Euclidean Panel Accordion
- **Files to modify:** `Resources/ui/js/app.js`
- **Features:**
  - Click band expand button → show/hide panel
  - Only one panel open at a time
  - Bind Euclidean parameter controls (steps, pulses, offset, depth)
  - Mode toggle (Manual/Euclidean) updates grid editability
- **Depends on:** Tasks 12, 13

### Task 16: Implement app.js - Global Controls Footer
- **Files to modify:** `Resources/ui/js/app.js`
- **Features:**
  - Mix slider with drag interaction
  - Steps dropdown (4, 8, 16, 32) → grid resize
  - Rate dropdown (1/1 to 1/32T)
  - Swing slider
  - Smoothing slider
- **Depends on:** Task 12

### Task 17: Build and Validate WebView Loading
- **Files to modify:** None (test only)
- **Validation:**
  - Build VST3 + AU
  - Install to system folders
  - Open in DAW, verify WebView displays
  - Check console for JUCE bridge errors
- **Depends on:** Tasks 1-16

### Task 18: Test Parameter Binding Round-Trip
- **Files to modify:** None (test only)
- **Validation:**
  - Move slider in UI → verify APVTS updates
  - Automate parameter in DAW → verify UI updates
  - Toggle step → verify processor receives change
  - Load preset → verify grid updates
- **Depends on:** Task 17

### Task 19: Test Playhead Synchronization
- **Files to modify:** None (test only)
- **Validation:**
  - Play transport → verify playhead moves
  - Change rate → verify playhead speed changes
  - Change steps → verify grid resizes and playhead adapts
  - Stop transport → verify playhead stops
- **Depends on:** Task 17

### Task 20: Polish Visual Details
- **Files to modify:** `Resources/ui/css/styles.css`, `Resources/ui/js/app.js`
- **Refinements:**
  - Adjust cell sizes for optimal 32-step display
  - Fine-tune playhead animation
  - Add hover states for interactive elements
  - Verify responsive behavior at 850×550
  - Add band labels with frequency ranges
- **Depends on:** Tasks 17-19

---

## File Summary

### Files to Create (7)
| File | Task |
|------|------|
| `Resources/ui/index.html` | 1, 10 |
| `Resources/ui/css/styles.css` | 1, 11 |
| `Resources/ui/js/app.js` | 1, 12-16 |
| `Resources/ui/js/juce/index.js` | 1 |
| `Resources/ui/js/juce/check_native_interop.js` | 1 |

### Files to Modify (4)
| File | Tasks |
|------|-------|
| `CMakeLists.txt` | 2 |
| `Source/PluginEditor.h` | 3-8 |
| `Source/PluginEditor.cpp` | 3-8 |
| `Source/PluginProcessor.h` | 9 |
| `Source/PluginProcessor.cpp` | 9 |

---

## Dependencies Graph

```
Task 1 (Resource dirs) ─┬─► Task 2 (CMake) ─────────────────────────────┐
                        ├─► Task 10 (HTML) ─────────────────────────────┤
                        ├─► Task 11 (CSS) ──────────────────────────────┤
                        └─► Task 12 (JS init) ─┬─► Task 13 (Grid) ──────┤
                                               ├─► Task 15 (Euclidean) ─┤
                                               └─► Task 16 (Footer) ────┤
                                                                        │
Task 3 (Global relays) ──┬─► Task 6 (WebView init) ─► Task 7 (Attachments) ─┬─► Task 17 (Build test)
Task 4 (Band relays) ────┤                          ↑                       │
Task 5 (Step relays) ────┘                          │                       │
                                                    │                       │
Task 9 (Processor step) ─► Task 8 (Timer) ──────────┘                       │
                                               ↓                            │
                                         Task 14 (Playhead JS) ─────────────┘
                                                                            │
                                               ┌────────────────────────────┘
                                               ▼
                                         Task 18 (Param test)
                                         Task 19 (Playhead test)
                                               │
                                               ▼
                                         Task 20 (Polish)
```

---

## Success Criteria

- [ ] WebView loads and displays Naturalist UI in DAW
- [ ] 32-step grid renders with 4 band rows
- [ ] Step click toggles on/off state
- [ ] Step brightness reflects gain level
- [ ] Playhead moves in sync with DAW transport
- [ ] Playhead respects rate parameter
- [ ] Euclidean panel expands/collapses per band
- [ ] Mode toggle switches Manual ↔ Euclidean
- [ ] Global controls (Mix, Rate, Swing, Smoothing, Steps) functional
- [ ] Steps dropdown resizes grid (4, 8, 16, 32)
- [ ] Parameters automate from DAW
- [ ] Preset load restores grid state
- [ ] No console errors in WebView
- [ ] 60fps UI responsiveness
- [ ] pluginval Level 5 passes

---

## Risk Mitigations

| Risk | Mitigation |
|------|------------|
| 128 toggle relays slow startup | Monitor perf; fallback to native function bulk read |
| Playhead latency | C++ timer at 30Hz; visual only, not timing-critical |
| Grid rendering jank | Batch DOM updates; CSS transforms for playhead |
| WebView resource 404 | Explicit URL mapping in getResource() |

---

## Execution Notes

1. **Parallel work possible:** Tasks 1-5 and Task 9 can be done in parallel
2. **Critical path:** Task 6 (WebView init) blocks all UI work
3. **Test early:** Task 17 should be reached quickly to validate WebView loading
4. **Reference code:** O-Bells PluginEditor.cpp and index.html are proven patterns

---

**Plan Status:** ✅ COMPLETE
**Next Phase:** /plugin-execute O-FreqPulse 3-gui
