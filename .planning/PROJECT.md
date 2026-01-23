# O-Bass (Ouaricon Bass Enhancer)

## Current Milestone: v1.0 Initial Release

**Goal:** Ship a working psychoacoustic bass enhancer with clean/colored modes and minimal WebView UI.

**Target features:**
- Psychoacoustic enhancement algorithm (research-driven)
- Clean mode + Colored mode with switch
- 3-5 control minimal UI
- VST3/AU/Standalone builds
- Preset system integration

## What This Is

A psychoacoustically-informed bass enhancement plugin for the Ouaricon commercial VST suite. O-Bass uses research on human bass perception to create fuller, more present low end that translates across playback systems. Designed for producers and sound designers who want results without endless tweaking.

## Core Value

Make bass perceptually fuller without introducing artifacts — enhancement that sounds natural and translates well.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Psychoacoustic-based enhancement algorithm (research-driven approach to bass perception)
- [ ] Minimal interface with 3-5 main controls
- [ ] Clean mode (transparent enhancement, no coloration)
- [ ] Colored mode (analog-modeled warmth and saturation)
- [ ] Mode switch between clean and colored
- [ ] Works effectively on individual bass tracks (bass guitar, kicks, synth bass)
- [ ] Works effectively on mix bus / mastering chain
- [ ] Suitable for creative sound design (can push into extreme territory)
- [ ] WebView UI consistent with Ouaricon suite visual language
- [ ] VST3, AU, and Standalone formats
- [ ] Preset system using shared OuariconPresetManager module

### Out of Scope

- Multi-band processing with user-adjustable crossovers — adds complexity without clear benefit for minimal UI goal
- Sidechain input — may revisit in v2 if users request
- Mid/side processing — keeping v1 focused on core enhancement
- Spectrum analyzer visualization — not needed for minimal "results over tweaking" approach

## Context

**Existing Infrastructure:**
- Ouaricon plugin suite with established patterns (PluginProcessor/PluginEditor structure)
- Shared modules: preset-manager, webview-relay-manager, effects units
- WebView UI pattern with JUCE↔JavaScript bridge (WebSliderRelay, WebComboBoxRelay)
- CMake build system with auto-discovery of plugins in `plugins/` directory

**Technical Environment:**
- JUCE 8.0.9 framework
- C++17 standard
- macOS 10.13+ deployment target
- WebBrowserComponent for HTML5 UI

**Related Plugins:**
- OuariconSaturationModeling — saturation/harmonic generation (potential shared DSP concepts)
- OuariconAnalogEQ — spectral processing patterns
- OuariconComp — dynamics processing patterns

## Constraints

- **UI Complexity**: Maximum 3-5 primary controls visible — "results over tweaking" philosophy
- **Suite Consistency**: Must match Ouaricon visual language and use shared modules where appropriate
- **Performance**: Real-time safe — no allocations in audio thread, suitable for mix bus use
- **Research-Driven**: Enhancement algorithm must be grounded in psychoacoustic research, not arbitrary harmonic addition

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Psychoacoustic approach over generic harmonics | Differentiation from commodity bass enhancers; more effective enhancement | — Pending |
| Minimal UI (3-5 knobs) | Target users want results, not endless parameters | — Pending |
| Switchable clean/colored modes | Versatility for different contexts without cluttering UI | — Pending |
| WebView UI | Consistency with Ouaricon suite; enables rich visuals | — Pending |

---
*Last updated: 2026-01-22 after milestone v1.0 started*
