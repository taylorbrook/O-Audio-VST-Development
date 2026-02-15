# Stage 3: GUI - Execution Plan

**Created:** 2026-02-14
**Plugin:** O-Texture
**Goal:** Replace the placeholder Stage 1 WebView with the full Ouaricon Naturalist GUI -- XY pad with orbital trails, naturalist sliders/knobs, source icon selector, and all 10 parameters bound via JUCE 8 WebView relay/attachment system.

## Prerequisites

- Stage 2 (DSP) verified complete -- all 10 APVTS parameters exist in PluginProcessor
- JUCE 8.0.4 WebBrowserComponent relay/attachment API available
- JUCE frontend JS files at `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/`
- O-TextureForge Naturalist CSS at `plugins/O-TextureForge/Source/ui/public/css/ouaricon-naturalist.css`
- O-TextureForge fern.png at `plugins/O-TextureForge/Source/ui/public/images/fern.png`

## Tasks

### Task 1: Copy JUCE frontend JS and botanical assets into project
- **Files to create:**
  - `Source/ui/public/js/juce/index.js` (copy verbatim from JUCE source)
  - `Source/ui/public/js/juce/check_native_interop.js` (copy verbatim from JUCE source)
  - `Source/ui/public/img/fern.png` (copy from O-TextureForge -- use fern as lichen placeholder)
- **Depends on:** none
- **Notes:** JUCE JS files must NOT be modified. Fern.png reused as botanical overlay; user can swap for lichen-specific image later.

### Task 2: Create Ouaricon Naturalist CSS adapted for O-Texture layout
- **Files to create:**
  - `Source/ui/public/css/ouaricon-naturalist.css`
- **Depends on:** none
- **Notes:** Adapt from O-TextureForge's CSS but restructure for O-Texture's unique layout: XY pad dominant left, 3 vertical sliders right, source icon row below, bottom strip with knobs + freeze toggle. Key elements:
  - Root variables: aged paper tones, botanical green (#6B8E4E), amber (#8B6914)
  - Garamond serif typography
  - Header bar with plugin name + mode toggle
  - XY pad area: dark inset parchment (#C8B8A0) with inset shadow
  - Vertical slider styles: aged paper track, seed-disc thumb
  - Source icon buttons: brown ink line art, highlighted active state
  - Seed cross-section rotary knobs for Brightness/Mix
  - Freeze toggle with botanical green active state
  - Fern botanical overlay (body::after, low opacity, bottom-right)
  - Ice crystal overlay class for Freeze state on XY pad

### Task 3: Create main.js -- parameter binding, XY pad, animations, controls
- **Files to create:**
  - `Source/ui/public/js/main.js`
- **Depends on:** Task 1 (JUCE JS files must exist for import)
- **Notes:** Single entry point JS module. Contains:
  - Import from `./juce/index.js` (getSliderState, getToggleState, getComboBoxState)
  - 7 WebSliderRelay bindings (X, Y, CHARACTER_A, CHARACTER_B, EVOLVE, BRIGHTNESS, MIX)
  - 2 WebComboBoxRelay bindings (SOURCE, MODE)
  - 1 WebToggleButtonRelay binding (FREEZE)
  - XY pad: Canvas pointer events driving X + Y relays simultaneously, sliderDragStarted/Ended pairs
  - Orbital trail animation: 30fps throttled requestAnimationFrame, trail array (60 points), fading botanical green circles
  - Freeze visual: toggle ice crystal overlay on/off, freeze trail updates
  - 3 vertical sliders: pointer drag interaction with gesture management
  - 2 rotary knobs: vertical drag interaction, seed cross-section visual rotation
  - Source icon button group: 6 buttons, click calls setChoiceIndex()
  - Mode toggle: 2-state toggle, click calls setChoiceIndex()
  - All controls listen to valueChangedEvent for backend-driven updates (automation, presets)
  - Double-click reset on knobs and sliders
  - ControlParameterIndexUpdater for DAW parameter hover highlighting

### Task 4: Create index.html -- Naturalist layout structure
- **Files to modify:**
  - `Source/ui/public/index.html` (replace Stage 1 placeholder)
- **Depends on:** Tasks 2, 3 (CSS and JS must be designed)
- **Notes:** Complete HTML structure:
  - Links to `css/ouaricon-naturalist.css`
  - Script module `js/main.js`
  - Header: plugin name "O-TEXTURE", mode toggle (Generate | Transform)
  - Main area: XY pad canvas (left), 3 vertical sliders (right: CharA, CharB, Evolve)
  - Source selector: 6 icon buttons with inline SVG line art (raindrop, gear, swirl, group, wave, leaf)
  - Bottom strip: Brightness knob, Mix knob, Freeze toggle
  - Fern botanical overlay via CSS (no extra HTML needed)
  - Data attributes for parameter index (DAW hover integration)

### Task 5: Update CMakeLists.txt UIResources for new files
- **Files to modify:**
  - `CMakeLists.txt`
- **Depends on:** Tasks 1, 2, 3, 4 (all source files must exist)
- **Notes:** Update `juce_add_binary_data(${PROJECT_NAME}_UIResources ...)` to include:
  - `Source/ui/public/index.html`
  - `Source/ui/public/css/ouaricon-naturalist.css`
  - `Source/ui/public/js/juce/index.js`
  - `Source/ui/public/js/juce/check_native_interop.js`
  - `Source/ui/public/js/main.js`
  - `Source/ui/public/img/fern.png`

### Task 6: Update PluginEditor.h -- Add relay and attachment members
- **Files to modify:**
  - `Source/PluginEditor.h`
- **Depends on:** none (can be done in parallel with Tasks 1-4)
- **Notes:** Add to header in CRITICAL order (relays -> webView -> attachments):
  - 7 `std::unique_ptr<juce::WebSliderRelay>` (x, y, characterA, characterB, evolve, brightness, mix)
  - 2 `std::unique_ptr<juce::WebComboBoxRelay>` (source, mode)
  - 1 `std::unique_ptr<juce::WebToggleButtonRelay>` (freeze)
  - Keep existing `std::unique_ptr<juce::WebBrowserComponent> webView` in position
  - 7 `std::unique_ptr<juce::WebSliderParameterAttachment>`
  - 2 `std::unique_ptr<juce::WebComboBoxParameterAttachment>`
  - 1 `std::unique_ptr<juce::WebToggleButtonParameterAttachment>`

### Task 7: Update PluginEditor.cpp -- Relay creation, Options chain, attachments, resource provider, destructor
- **Files to modify:**
  - `Source/PluginEditor.cpp`
- **Depends on:** Tasks 5, 6 (CMake must know about files for BinaryData identifiers; header must have members)
- **Notes:** Constructor changes:
  - Create 10 relays with exact names matching JS: "xSlider", "ySlider", "characterASlider", "characterBSlider", "evolveSlider", "brightnessSlider", "mixSlider", "sourceCombo", "modeCombo", "freezeToggle"
  - Chain all 10 relays into WebBrowserComponent Options via `.withOptionsFrom()`
  - Keep existing `.withBackend()`, `.withWinWebView2Options()`, `.withNativeIntegrationEnabled()`, `.withResourceProvider()`
  - Add `.withKeepPageLoadedWhenBrowserIsHidden()`
  - Create 10 attachments binding relays to APVTS parameters
  - Navigate to resource provider root

  Destructor changes:
  - Explicit `.reset()` in reverse order: all attachments, then webView, then all relays

  Resource provider changes:
  - Add URL routes for all new files: `/css/ouaricon-naturalist.css`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`, `/js/main.js`, `/img/fern.png`
  - Map to correct BinaryData identifiers
  - Set correct MIME types (text/css, application/javascript, image/png)

### Task 8: Build, test, fix
- **Files to modify:** Any from above
- **Depends on:** Tasks 1-7 (all implementation complete)
- **Notes:**
  - CMake configure (verify BinaryData identifiers generated)
  - Build: `ninja OuariconTexture_VST3 OuariconTexture_AU`
  - Run Standalone to verify:
    - WebView loads without blank page
    - All 10 parameters sync bidirectionally (move in DAW automation -> UI updates, move in UI -> parameter changes)
    - XY pad responds to click/drag, updates both X and Y simultaneously
    - Orbital trail animation visible and smooth
    - Freeze toggle shows ice crystal overlay and stops trail
    - Source icon buttons switch and highlight
    - Mode toggle works
    - Vertical sliders respond to drag
    - Rotary knobs respond to vertical drag
    - Double-click resets knobs/sliders
  - Fix any BinaryData identifier mismatches, resource 404s, relay connection issues

## File Change Summary

| File | Action | Task |
|------|--------|------|
| `Source/ui/public/js/juce/index.js` | Create (copy from JUCE) | 1 |
| `Source/ui/public/js/juce/check_native_interop.js` | Create (copy from JUCE) | 1 |
| `Source/ui/public/img/fern.png` | Create (copy from O-TextureForge) | 1 |
| `Source/ui/public/css/ouaricon-naturalist.css` | Create | 2 |
| `Source/ui/public/js/main.js` | Create | 3 |
| `Source/ui/public/index.html` | Replace | 4 |
| `CMakeLists.txt` | Modify (UIResources section) | 5 |
| `Source/PluginEditor.h` | Modify (add relay/attachment members) | 6 |
| `Source/PluginEditor.cpp` | Modify (constructor, destructor, resource provider) | 7 |

## Success Criteria

- [ ] Plugin builds without errors (VST3 + AU)
- [ ] WebView displays Ouaricon Naturalist aesthetic (aged paper, earth tones, Garamond type)
- [ ] XY pad renders with dark inset parchment surface
- [ ] Click/drag on XY pad updates both X and Y parameters
- [ ] Orbital trail animation shows recent cursor path in botanical green at ~30fps
- [ ] Freeze toggle activates ice crystal overlay and stops trail updates
- [ ] 3 vertical sliders (CharA, CharB, Evolve) respond to drag
- [ ] 2 rotary knobs (Brightness, Mix) respond to vertical drag
- [ ] 6 source icon buttons highlight on selection and switch SOURCE parameter
- [ ] Mode toggle switches between Generate/Transform
- [ ] All 10 parameters sync bidirectionally (UI <-> APVTS <-> DAW automation)
- [ ] Fern botanical overlay visible at low opacity (bottom-right)
- [ ] No blank WebView on any platform (resource provider serves all files)
- [ ] pluginval passes at strictness 5 (VST3 + AU)

## Execution Order

Tasks 1, 2, 6 can run in parallel (no dependencies).
Task 3 depends on Task 1.
Task 4 depends on Tasks 2, 3.
Task 5 depends on Tasks 1, 2, 3, 4.
Task 7 depends on Tasks 5, 6.
Task 8 depends on all above.

```
[Task 1: Copy assets] ──┬──> [Task 3: main.js] ──┬──> [Task 4: index.html] ──> [Task 5: CMake] ──┐
[Task 2: CSS]        ───┘                         ┘                                               ├──> [Task 7: Editor.cpp] ──> [Task 8: Build+Test]
[Task 6: Editor.h]  ─────────────────────────────────────────────────────────────────────────────┘
```
