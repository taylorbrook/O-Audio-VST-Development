# Stage 3: GUI — Plan

**Date:** 2026-08-21
**Inputs:** CONTEXT.md (design decisions), RESEARCH.md (verified sources + resolved open questions), `parameter-spec.md` (BINDING), ROADMAP phases 3.1/3.2
**Primary copy source:** O-Bitrot (segmented choice binding, knob interaction, CMake, clean paper texture). O-Prism supplies typed value entry.

---

## Goal

Ship the O-Emulator WebView GUI: Ouaricon Naturalist aesthetic at fixed 620×430, five-segment console selector (accent-tinted per console), static per-console info readout, four 60px macro knobs with full house interaction (relative drag + wheel + double-click typed entry + Alt-click reset), dinosaur-skeleton specimen overlay — all 5 parameters bound two-way via relays (UI-01), preset/automation-fresh, with the render harness untouched and still passing.

**Binding constraint:** `parameter-spec.md` is frozen — 1 choice `console` (5 entries) + 4 floats `crush` / `age` / `reverb` / `mix` (linear 0–100%). No ID/type/range/default changes.

---

## Tasks

### Phase 3.1 — Layout and Basic Controls

1. [ ] **Asset preparation**
   - Crop dino PNG to content bbox (42,85)→(971,560) + ~10px margin, convert → WebP q90 as `Source/ui/public/img/specimen.webp` (no hyphens in filename)
   - Copy `plugins/O-Bitrot/Source/ui/public/img/paper.jpg` → `Source/ui/public/img/paper.jpg`; **verify md5 `40c5f97e25bd2492a6c8fe2ef0882541`** (clean O-Tremolo texture, NOT the watermarked Adobe Stock one)
   - Copy `plugins/O-Bitrot/Source/ui/public/js/juce/` (index.js + check_native_interop.js) → `Source/ui/public/js/juce/`
   - Write `Source/ui/public/img/PROVENANCE.md`: Marsh, *The Dinocerata* (USGS, 1886, public domain) + processing steps; paper.jpg lineage
   - Files: `Source/ui/public/img/*`, `Source/ui/public/js/juce/*`
   - Depends on: none

2. [ ] **HTML/CSS scaffold** (`Source/ui/public/index.html`)
   - Naturalist frame per aesthetic template: paper background, ink borders, Garamond stack, header row with title `O-EMULATOR` + ❦ ornament + reserved preset-bar slot (~28px, empty in v1.0)
   - Layout per CONTEXT sketch: header → full-width 5-segment selector → info readout line → single row of 4 knobs (60px, labels CRUSH / AGE / REVERB / MIX, readout divs)
   - Specimen overlay: `img/specimen.webp`, height ~60% (~260px), `right: -140px`, `top: 55%`, opacity 0.35, `pointer-events: none`, z above paper / below controls
   - Segmented control: O-Bitrot `.seg` CSS extended to 5 buttons (`data-value="0..4"`, labels SNES / PS1 / NES / GB / GENESIS)
   - Accent system: CSS custom properties (`--accent`, `--accent-dark`) swapped via `body[data-console]` attribute selectors; five hexes from RESEARCH §2: SNES `#7B6480`, PS1 `#5F7386`, NES `#9B5A48`, GB `#6F7D3F`, Genesis `#4E7A70`
   - Files: `Source/ui/public/index.html`
   - Depends on: Task 1

3. [ ] **CMake binary-data target**
   - Single `juce_add_binary_data(OEmulator_UIResources ...)` at the marked spot (~line 64): index.html, js/juce/index.js, js/juce/check_native_interop.js, img/paper.jpg, img/specimen.webp
   - `target_link_libraries(OEmulator PRIVATE OEmulator_UIResources)`; add `Source/PluginEditor.cpp` to plugin sources
   - Identity config (NEEDS_WEB_BROWSER etc.) already done in Stage 1 — touch none of it
   - Files: `plugins/O-Emulator/CMakeLists.txt`
   - Depends on: Tasks 1, 2

4. [ ] **PluginEditor + relays + resource provider**
   - New `Source/PluginEditor.{h,cpp}` copying O-Bitrot's shape: members as `std::unique_ptr` (#11) — 1 `WebComboBoxRelay("console")` + 4 `WebSliderRelay`; 1 `WebComboBoxParameterAttachment` + 4 `WebSliderParameterAttachment`, all 3-arg with nullptr UndoManager (#12); `WebBrowserComponent` declared last
   - Options chain: `.withNativeIntegrationEnabled()` + `.withResourceProvider(...)` + 5 × `.withOptionsFrom(*relay)`; `goToURL(getResourceProviderRoot())`
   - `getResource()`: explicit URL map (#8) — `index.html` (+ `/`), both js/juce files (#13), both images
   - Wire `createEditor()` behind the pre-planted `#if JUCE_WEB_BROWSER` guard in PluginProcessor.cpp (~line 41); `GenericAudioProcessorEditor` fallback in `#else`. **Do not touch `tests/render-harness/CMakeLists.txt`**
   - `setSize(620, 430)`, non-resizable
   - Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/PluginProcessor.cpp`
   - Depends on: Task 3

5. [ ] **Selector + knob JS bindings (drag baseline)**
   - Segmented selector: O-Bitrot's generic `.seg[data-param]` block via `Juce.getComboBoxState("console")` — render toggles `.sel` by index (classList only, never button text), click → `setChoiceIndex`
   - Same `valueChangedEvent` listener also: sets `body[data-console]` accent attribute + writes info readout from static `CONSOLE_SPECS` table (RESEARCH §8 — names/rates/codecs locked to ARCHITECTURE.md)
   - Knobs: O-Bitrot `setupKnob` — pointer capture, relative drag 0.005/px (shift 0.001 fine), wheel ±0.02, `getScaledValue()` readouts, `propertiesChangedEvent` re-render; skew-aware helper retained
   - ES-module `Juce` namespace import throughout; `window.__JUCE__` appears nowhere
   - Files: `Source/ui/public/index.html`
   - Depends on: Tasks 2, 4

6. [ ] **Build + 3.1 gate**
   - `ninja OEmulator_VST3 OEmulator_AU`; install via `./scripts/build-and-install.sh O-Emulator` (dual-variant sweep)
   - Render harness re-run: ALL PASS, **digests unchanged** (GUI must not touch DSP)
   - Verify: WebView opens at 620×430, layout matches sketch, all 5 controls respond, console switch audibly rides the 30 ms crossfade, no console errors
   - Depends on: Tasks 1–5

### Phase 3.2 — Binding Completeness + Polish

7. [ ] **Typed value entry + reset relocation**
   - `dblclick` on knob → O-Prism `attachValueEntry` pattern (overlay input on readout span, transparent span text keeps box size, ink color captured before hide, untouched-text guard, Escape cancels / blur commits, re-render from live state)
   - Reset-to-default moves to **Alt/Option-click** on capture phase (preempts drag)
   - Parse: float, clamp 0–100, `setNormalisedValue(v/100)` via the skew-aware helper
   - Files: `Source/ui/public/index.html`
   - Depends on: Task 6

8. [ ] **Accent theming polish + open design calls**
   - Active segment: accent fill at ~0.45–0.55 alpha over paper, border/text = accent darkened ~15%; hover keeps template light-green tint
   - Decide divider color (keep green `rgba(60,92,26,0.5)` vs. neutral brown `rgba(92,64,51,0.4)`) — pick whichever reads cleanly against all 5 accents
   - Knob indicator stems tinted by active accent; info-readout text tinted; `pointer-events: none` on stems verified
   - Window height final call (±10px around 430) after visual check with real metrics
   - Files: `Source/ui/public/index.html`, possibly `Source/PluginEditor.cpp` (setSize)
   - Depends on: Task 6

9. [ ] **Preset/automation freshness + bridge audit**
   - Verify host automation of all 5 params updates UI live; preset/state load updates segments + accent + readout + all knobs (relay events fire on load — no revision counter expected; add only if a one-shot push sneaks in)
   - grep-diff bridge audit: expected native-fn count **0**; `window.__JUCE__` zero hits; getNativeFunction zero hits
   - Files: none expected (audit + fixes if found)
   - Depends on: Tasks 7, 8

10. [ ] **Final validation + install**
    - Render harness: ALL PASS, digests identical to Stage-2 baseline
    - pluginval strictness 10, VST3 + AU; `auval -a | grep -i emulator`
    - `./scripts/build-and-install.sh O-Emulator`; manual DAW pass: all 5 consoles switch cleanly, knobs automate, preset load refreshes everything, UI at correct size
    - Depends on: Tasks 7–9

---

## Files Summary

| File | Action |
|------|--------|
| `Source/ui/public/index.html` | create (tasks 2, 5, 7, 8) |
| `Source/ui/public/img/specimen.webp` | create (crop+convert from Ouaricon Audio Images) |
| `Source/ui/public/img/paper.jpg` | copy from O-Bitrot, md5-verified |
| `Source/ui/public/img/PROVENANCE.md` | create |
| `Source/ui/public/js/juce/*` | copy from O-Bitrot |
| `Source/PluginEditor.h/.cpp` | create |
| `Source/PluginProcessor.cpp` | edit (guarded createEditor only) |
| `plugins/O-Emulator/CMakeLists.txt` | edit (binary-data target + editor source) |
| `tests/render-harness/CMakeLists.txt` | **do not touch** |

---

## Success Criteria

- [ ] WebView editor opens at 620×430 (±10 final), layout matches CONTEXT sketch, Naturalist aesthetic with dino specimen overlay
- [ ] All 5 parameters bound two-way: UI → DSP, host automation → UI, preset load refreshes segments/accent/readout/knobs (UI-01)
- [ ] Console selector one-click switches modes; accent + info readout follow; audio rides the existing 30 ms crossfade
- [ ] Knobs: relative drag + shift-fine + wheel + double-click typed entry + Alt-click reset; readouts via `getScaledValue()`
- [ ] Render harness ALL PASS with digests **identical** to Stage-2 baseline
- [ ] pluginval strictness 10 clean (VST3 + AU); auval clean
- [ ] Bridge audit clean: 0 native functions, 0 `window.__JUCE__` references
- [ ] paper.jpg md5 = `40c5f97e25bd2492a6c8fe2ef0882541`; PROVENANCE.md present; no hyphens in resource filenames; exactly one binary-data target
