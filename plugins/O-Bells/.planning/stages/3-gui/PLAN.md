# Stage 3: GUI Implementation - Plan

**Plugin:** O-Bells
**Stage:** 3 (GUI Implementation)
**Phase:** Plan
**Date:** 2026-02-01

---

## Goal

Implement WebView-based UI for O-Bells with 18 parameter bindings, Ouaricon Naturalist aesthetic, tab-based layout (Instrument / Tuning placeholder), snail botanical watermark, and simple output level meter.

---

## Tasks

### Task 1: Create Resources Directory Structure
- **Files to create:**
  - `plugins/O-Bells/Resources/ui/` (directory)
  - `plugins/O-Bells/Resources/ui/js/juce/` (directory)
  - `plugins/O-Bells/Resources/ui/img/` (directory)
- **Depends on:** None
- **Complexity:** Trivial

### Task 2: Copy JUCE ES6 Bridge Files
- **Files to create:**
  - `plugins/O-Bells/Resources/ui/js/juce/index.js` (from O-Lyrica)
  - `plugins/O-Bells/Resources/ui/js/juce/check_native_interop.js` (from O-Lyrica)
- **Source:** `plugins/O-Lyrica/Resources/ui/js/juce/`
- **Depends on:** Task 1
- **Complexity:** Trivial

### Task 3: Copy and Optimize Botanical Image
- **Files to create:**
  - `plugins/O-Bells/Resources/ui/img/snail.png`
- **Source:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png`
- **Action:** Copy file to Resources location
- **Depends on:** Task 1
- **Complexity:** Trivial

### Task 4: Create WebView HTML/CSS/JS (index.html)
- **Files to create:**
  - `plugins/O-Bells/Resources/ui/index.html`
- **Content:**
  - HTML structure with header, tab bar, two tab contents, footer
  - CSS styling (Ouaricon Naturalist: Garamond, aged paper palette, seed-like sliders)
  - JavaScript parameter bindings for all 18 parameters
  - Tab switching with botanical overlay animation
  - Simple CSS-based output meter placeholder
- **Parameter Bindings (18 total):**
  - **Synthesis (6 sliders):** strikePosition, malletHardness, damping, brightness, material, inharmonicity
  - **Ensemble (5 controls):** unisonCount (slider), unisonDetune, octaveBlendSub, octaveBlendOct, stereoSpread
  - **Character (3 choice buttons):** strikeNoiseChar, velocityCurve, decayShape
  - **Advanced (4 sliders):** partialTuning, pitchEnvelope, pitchEnvTime, nonlinearEffects
  - **Output (1 slider):** outputGain
- **Depends on:** Tasks 2, 3
- **Complexity:** High (core implementation)

### Task 5: Update CMakeLists.txt for Binary Data
- **Files to modify:**
  - `plugins/O-Bells/CMakeLists.txt`
- **Changes:**
  - Add `juce_add_binary_data(O-Bells_UIResources ...)` target
  - Add files: index.html, js/juce/index.js, js/juce/check_native_interop.js, img/snail.png
  - Link UIResources to O-Bells target
- **Depends on:** Tasks 2, 3, 4
- **Complexity:** Low

### Task 6: Update PluginEditor.h with WebView Members
- **Files to modify:**
  - `plugins/O-Bells/Source/PluginEditor.h`
- **Changes:**
  - Add `#include <JuceHeader.h>` if missing
  - Add 15 WebSliderRelay unique_ptrs (float/int params)
  - Add 3 WebComboBoxRelay unique_ptrs (choice params)
  - Add WebBrowserComponent unique_ptr
  - Add 15 WebSliderParameterAttachment unique_ptrs
  - Add 3 WebComboBoxParameterAttachment unique_ptrs
  - Add getResource() helper method declaration
  - Maintain CRITICAL ORDER: Relays → WebView → Attachments
- **Depends on:** None
- **Complexity:** Medium

### Task 7: Implement PluginEditor.cpp with WebView
- **Files to modify:**
  - `plugins/O-Bells/Source/PluginEditor.cpp`
- **Changes:**
  - Constructor: Create relays, webView with options, attachments (in order)
  - Implement getResource() with explicit URL mappings (Pattern #8)
  - Set fixed size 800x600
  - Use three-parameter WebSliderParameterAttachment (Pattern #12)
  - Use nullptr for undoManager in attachments
- **Depends on:** Tasks 5, 6
- **Complexity:** High

### Task 8: Build and Verify WebView Renders
- **Action:** Build VST3 + AU, verify WebView loads in DAW
- **Verification:**
  - Plugin builds without errors
  - WebView renders (no blank screen)
  - All resources load (no 404s in console)
  - Botanical overlay visible
- **Depends on:** Task 7
- **Complexity:** Low

### Task 9: Verify Parameter Bindings
- **Action:** Test all 18 parameters respond to UI interaction and automation
- **Verification:**
  - Moving slider updates parameter value (check via DAW automation)
  - Changing parameter via automation updates slider visual
  - Choice buttons update correctly
  - No crashes on parameter changes
- **Depends on:** Task 8
- **Complexity:** Medium

### Task 10: Verify Tab Switching and Polish
- **Action:** Test tab switching, botanical animation, visual polish
- **Verification:**
  - Tabs switch cleanly between Instrument and Tuning
  - Botanical overlay shifts position on tab change
  - Placeholder text shows on Tuning tab
  - No visual glitches or layout issues
- **Depends on:** Task 9
- **Complexity:** Low

---

## File Summary

### Files to Create (6)
| File | Type | Purpose |
|------|------|---------|
| `Resources/ui/index.html` | HTML | Main WebView interface |
| `Resources/ui/js/juce/index.js` | JS | JUCE ES6 bridge |
| `Resources/ui/js/juce/check_native_interop.js` | JS | Native interop check |
| `Resources/ui/img/snail.png` | PNG | Botanical overlay |

### Files to Modify (3)
| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Add binary data target |
| `Source/PluginEditor.h` | Add WebView members |
| `Source/PluginEditor.cpp` | Implement WebView UI |

---

## Parameter Binding Map

| Parameter ID | Type | Relay Type | UI Control |
|--------------|------|------------|------------|
| strikePosition | Float | WebSliderRelay | Horizontal slider |
| malletHardness | Float | WebSliderRelay | Horizontal slider |
| damping | Float | WebSliderRelay | Horizontal slider |
| brightness | Float | WebSliderRelay | Horizontal slider |
| material | Float | WebSliderRelay | Horizontal slider |
| inharmonicity | Float | WebSliderRelay | Horizontal slider |
| unisonCount | Int | WebSliderRelay | Horizontal slider (discrete 1-4) |
| unisonDetune | Float | WebSliderRelay | Horizontal slider |
| octaveBlendSub | Float | WebSliderRelay | Horizontal slider |
| octaveBlendOct | Float | WebSliderRelay | Horizontal slider |
| stereoSpread | Float | WebSliderRelay | Horizontal slider |
| partialTuning | Float | WebSliderRelay | Horizontal slider |
| pitchEnvelope | Float | WebSliderRelay | Horizontal slider |
| pitchEnvTime | Float | WebSliderRelay | Horizontal slider |
| nonlinearEffects | Float | WebSliderRelay | Horizontal slider |
| outputGain | Float | WebSliderRelay | Horizontal slider |
| strikeNoiseChar | Choice | WebComboBoxRelay | 3-button selector |
| velocityCurve | Choice | WebComboBoxRelay | 3-button selector |
| decayShape | Choice | WebComboBoxRelay | 3-button selector |

**Total:** 16 sliders + 3 choice selectors = 18 parameters

---

## Critical Patterns to Follow

From `juce8-critical-patterns.md`:

| Pattern # | Description | Application |
|-----------|-------------|-------------|
| #8 | Explicit URL mapping in getResource() | No generic loops |
| #11 | unique_ptr member order: Relays → WebView → Attachments | Destruction safety |
| #12 | Three-parameter attachment constructor | nullptr for undoManager |
| #15 | valueChangedEvent callbacks take no parameters | Use closure capture |
| #21 | ES6 module type="module" | Required for JUCE bridge |

---

## Success Criteria

- [ ] Plugin builds (VST3 + AU) without errors
- [ ] WebView renders with Ouaricon Naturalist aesthetic
- [ ] All 18 parameters bound and responsive to UI interaction
- [ ] All 18 parameters respond to DAW automation
- [ ] Tab switching works (Instrument ↔ Tuning)
- [ ] Botanical overlay animates on tab switch
- [ ] Tuning tab shows placeholder content
- [ ] No visual glitches or layout issues
- [ ] State save/load preserves parameter values
- [ ] No crashes during normal operation

---

## Estimated Task Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ Task 1: Create directories                                      │
└──────────────────────┬──────────────────────────────────────────┘
                       │
          ┌────────────┼────────────┐
          ▼            ▼            ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│ Task 2      │ │ Task 3      │ │ Task 6      │
│ Copy JS     │ │ Copy image  │ │ Editor.h    │
└──────┬──────┘ └──────┬──────┘ └──────┬──────┘
       │               │               │
       └───────┬───────┘               │
               ▼                       │
       ┌─────────────┐                 │
       │ Task 4      │                 │
       │ index.html  │                 │
       └──────┬──────┘                 │
              │                        │
              ▼                        │
       ┌─────────────┐                 │
       │ Task 5      │                 │
       │ CMakeLists  │                 │
       └──────┬──────┘                 │
              │                        │
              └────────────┬───────────┘
                           ▼
                   ┌─────────────┐
                   │ Task 7      │
                   │ Editor.cpp  │
                   └──────┬──────┘
                          │
                          ▼
                   ┌─────────────┐
                   │ Task 8      │
                   │ Build+Verify│
                   └──────┬──────┘
                          │
                          ▼
                   ┌─────────────┐
                   │ Task 9      │
                   │ Param test  │
                   └──────┬──────┘
                          │
                          ▼
                   ┌─────────────┐
                   │ Task 10     │
                   │ Polish      │
                   └─────────────┘
```

---

## Reference Files

- **O-Lyrica Editor:** `plugins/O-Lyrica/Source/PluginEditor.h`, `.cpp`
- **O-Lyrica UI:** `plugins/O-Lyrica/Resources/ui/index.html`
- **Critical Patterns:** `troubleshooting/patterns/juce8-critical-patterns.md`
- **Aesthetic:** `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`
- **Context:** `plugins/O-Bells/.planning/stages/3-gui/CONTEXT.md`
- **Research:** `plugins/O-Bells/.planning/stages/3-gui/RESEARCH.md`

---

*Plan created: 2026-02-01*
*Ready for: Execute phase*
