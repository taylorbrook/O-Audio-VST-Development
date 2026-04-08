# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Starting State

- C++ editor fully wired: 20 WebSliderRelays + 1 WebComboBoxRelay + WebView + all attachments
- Missing relays for 2 params added in DSP phases: `frictionTier` (choice) and `bowNoise` (float)
- index.html is a placeholder shell -- no actual UI
- Window size: 900x600 (keeping)
- 23 APVTS parameters total (22 original + frictionTier + bowNoise)

## Requirements Confirmed

- **Aesthetic:** Ouaricon Naturalist (ouaricon-naturalist-001) -- aged paper, seed cross-section knobs, serif typography (Garamond), botanical green accents, brown borders/structure, botanical illustration overlay
- **Layout:** Center visualization with controls flanking left/right (Bow left, Body/Strings right, Impossible Physics below center viz, Output bottom bar)
- **Visualizations:** All three -- bow-string animation, body resonance spectrum, Schelleng diagram
- **Window size:** 900x600 (no change)
- **All 23 parameters** must have UI controls with two-way binding
- **frictionTier** needs new WebComboBoxRelay + attachment added to PluginEditor.h/cpp
- **bowNoise** needs new WebSliderRelay + attachment added to PluginEditor.h/cpp

## Constraints Identified

- 900x600 is tight for 23 params + 3 visualizations -- layout must be efficient
- Botanical illustration must not compete with center visualization panel
- Seed cross-section knobs need ~55px minimum -- constrains how many fit per column
- 3 visualizations (bow-string, body spectrum, Schelleng) must share center panel space -- tabbed or stacked
- Per-string tuning (4 params) only relevant when stringCount > 1 -- consider conditional visibility
- Sympathetic controls only relevant when sympatheticCount > 0

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Aesthetic | Ouaricon Naturalist | Brand consistency across all Ouaricon plugins |
| Layout | Center viz, flanking controls | Visual focus on bow-string interaction, controls accessible on sides |
| Visualizations | All 3 (bow-string, body spectrum, Schelleng) | User wants full visual feedback for this complex synth |
| Viz switching | Tabbed center panel | Can't fit all 3 at once in 900x600 |
| Friction tier | 3-way toggle (Core/Enhanced/Quality) | Matches existing choice parameter, prominent placement |
| Tuning system | Dropdown selector | Already a WebComboBoxRelay, standard pattern |
| String tuning 1-4 | Conditionally visible based on stringCount | Reduces visual clutter when using single string |
| Window size | 900x600 | Already set, sufficient with efficient layout |
| Botanical overlay | Right side, low opacity (~0.3) | Standard Ouaricon placement, won't conflict with right-side controls if positioned carefully |

## Layout Specification

```
+--------------------------------------------------+
|  O-BOWED           [Preset]  [Tuning: dropdown]  |  Header bar
+----------+---------------------+-----------------+
|   BOW    |   CENTER VIZ PANEL  |   BODY          |
| Speed    |  [Bow] [Body] [Sch] |  Material       |  Tab selector
| Pressure |  +-----------------+|  Size           |
| Position |  |                 ||  Brightness     |
| Rosin    |  |  Active viz     ||                 |
| Noise    |  |  (animated)     ||  STRINGS        |
|          |  |                 ||  Count          |
| [Tier    |  +-----------------+|  Tune 1-4       |
|  toggle] |                     |  Symp Amount    |
|          | IMPOSSIBLE PHYSICS  |  Symp Count     |
|          | Inf.  Rev.  Sub.    |                 |
+----------+---------------------+-----------------+
|                Width      Level                   |  Output bar
+--------------------------------------------------+
```

## Visualization Specs

### Bow-String Animation (Default Tab)
- Horizontal string line with bow contact point
- Bow position (beta) shown as contact location on string
- Bow pressure shown as bow angle/penetration
- String vibration amplitude shown as wave displacement
- Animates in response to MIDI input and parameter changes

### Body Resonance Spectrum
- Frequency response curve of current body preset (8 peaks)
- Updates when Material or Size changes
- X-axis: 20Hz-20kHz (log), Y-axis: dB
- Color-coded by material type

### Schelleng Diagram
- 2D plot: bow pressure (Y) vs bow position (X)
- Shows playable region (Helmholtz motion) as colored zone
- Current playing point shown as dot/crosshair
- Helps users understand why certain parameter combos produce better sound

## C++ Changes Needed

1. Add `frictionTierRelay` (WebComboBoxRelay) to PluginEditor.h
2. Add `frictionTierAttachment` (WebComboBoxParameterAttachment) to PluginEditor.h
3. Add `bowNoiseRelay` (WebSliderRelay) to PluginEditor.h
4. Add `bowNoiseAttachment` (WebSliderParameterAttachment) to PluginEditor.h
5. Register both new relays with `.withOptionsFrom()` in WebView construction
6. Create attachments in constructor after WebView
7. Add resource provider routes for any new CSS/JS files

## Open Questions

- Which botanical illustration for a bowed string synth? (suggest: botanical drawing of horsehair/rosin plant, or cross-section of resonant wood grain)
- Preset browser UI pattern -- inline dropdown or full panel? (defer to Phase 4.3)
- Scala/TUN file browser -- native file dialog via C++ callback or WebView file picker? (defer to Phase 4.3)

## Phase Breakdown (from ROADMAP)

- **Phase 4.1:** Layout + basic knob controls (HTML/CSS structure, seed knobs, section grouping)
- **Phase 4.2:** Full two-way parameter binding (all 23 params, host automation sync)
- **Phase 4.3:** Visualizations (bow-string, body spectrum, Schelleng), preset browser, tuning file browser

## Next Phase

Ready for: research phase
