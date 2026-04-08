# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Phase Restructuring

ROADMAP originally specified 3 phases (5.1 Layout, 5.2 Binding, 5.3 Polish). User merged 5.1+5.2 into a single phase:

| Phase | Goal |
|-------|------|
| 3.1 | Layout + Controls + Parameter Binding (all 14 params wired, preset module, 3 tabs, tone hole toggle) |
| 3.2 | Breath/jet visualization, register indicator, visual polish |

No mockup iteration -- straight to code based on BRIEF + this context.

## Requirements Targeted

| Requirement | Priority | Phase |
|-------------|----------|-------|
| UI-01: Signal-flow parameter layout with grouped controls | must | 3.1 |
| UI-02: 3-tab interface (Sound, Tuning, Effects) | must | 3.1 |
| UI-03: Instrument preset selector (preset module) | must | 3.1 |
| UI-04: Tone hole system toggle (visible) | must | 3.1 |
| UI-05: Tuning panel integration (shared module) | must | 3.1 |
| UI-06: Breath/jet real-time visualization | nice | 3.2 |
| UI-07: Register indicator (active harmonic mode) | nice | 3.2 |
| UI-08: Visual polish and animation refinement | nice | 3.2 |

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Visual aesthetic | Ouaricon Naturalist | Brand consistency across all plugins |
| Window size | 900x600 | Enough room for 14 knobs + preset bar + tabs with naturalist spacing |
| Tab structure | 3 tabs: Sound, Tuning, Effects | Signal-flow controls on Sound tab; shared tuning module on Tuning tab; Effects tab blank (future) |
| Preset system | Shared preset module (OuariconPresetManager + preset-manager.js) | Same as O-Bells, O-Lyrica, etc. |
| Botanical illustration | Fern (flora/fern_naturalistsmisc1Geor_0089.png) | Delicate feathery fronds evoke wind and air movement |
| Tone hole toggle | Visible UI toggle on Sound tab | Tier 2 Keefe scattering defaults off; user controls it |
| Mockup step | Skipped | Signal-flow layout is well-defined; straight to implementation |
| Phase merge | 5.1+5.2 combined | No value in non-functional UI step; build layout and wire params together |

## Tab Structure

### Tab 1: SOUND (Main)

Signal-flow layout left-to-right mirroring jet -> bore -> output chain:

```
[Preset Browser Bar]
[Tab Bar: SOUND | TUNING | EFFECTS]

+-- Excitation --+  +-- Resonator ------+  +-- Expression --+  +-- Output --+
| Breath Pressure|  | Tone Color        |  | Vibrato Rate   |  | Width      |
| Embouchure     |  | Air Column        |  | Vibrato Depth  |  | Output Lvl |
| Breath Noise   |  | Jet Reflection    |  +----------------+  +------------+
+----------------+  | End Reflection    |
                    +-------------------+

+-- Impossible Physics ----+  [Tone Hole Toggle]
| Infinite Sustain         |
| Reversed Jet             |
| Sub-Harmonics            |
+---------------------------+
```

**Group layout:** 4 groups across the top row (Excitation, Resonator, Expression, Output), Impossible Physics as a bottom-left section, Tone Hole toggle near Resonator or bottom area.

### Tab 2: TUNING

Shared tuning-panel.js module (already imported in PluginEditor.cpp resource provider). Contains:
- A4 reference frequency control
- Tuning system selector (12-TET, Scala, MTS-ESP)
- Interval visualization
- Scale generator
- Scala file I/O

### Tab 3: EFFECTS

Blank placeholder for future expansion. Display "Coming Soon" or minimal branding.

## Preset Module Integration

Uses shared `modules/persistence/preset-manager/`:
- **C++ side:** Add `OuariconPresetManager` to PluginProcessor, register native functions in PluginEditor (`getPresetList`, `getCurrentPreset`, `loadPreset`, `savePreset`, `selectNextPreset`, `selectPreviousPreset`, etc.)
- **JS side:** Copy `preset-manager.js` to Resources, preset browser HTML bar with prev/next arrows + dropdown + save/load
- **Factory presets:** 8 instrument presets as factory JSON files (Concert Flute, Shakuhachi, Bansuri, Native American Flute, Recorder, Pan Flute, Piccolo, Ocarina)
- **Preset directory:** `~/Library/Application Support/O-Wind/Presets/`

### Preset Browser HTML Pattern (from O-Bells)

```html
<div class="preset-browser">
    <div class="preset-arrow" id="preset-prev">&#9664;</div>
    <div class="preset-name-wrapper">
        <span id="preset-name-display">Default</span>
        <div id="preset-dropdown" class="preset-dropdown">
            <!-- Categories populated dynamically -->
        </div>
    </div>
    <div class="preset-arrow" id="preset-next">&#9654;</div>
    <button id="preset-save">Save</button>
    <button id="preset-load">Load</button>
</div>
```

## Instrument Preset <-> Factory Preset Relationship

The 8 InstrumentPresets in DSP (InstrumentPresets.h) set internal DSP coefficients. The preset module saves/restores ALL APVTS parameter values (user-facing knobs). These are separate systems:

- **Instrument presets** = DSP coefficient sets (bore characteristics, jet gain, noise spectrum per instrument type)
- **Factory presets** = Saved parameter snapshots (breath=50%, embouchure=60%, etc. + instrument=Shakuhachi)

Factory presets will include instrument selection + curated parameter values for each instrument.

## Parameters to Bind (16 total via APVTS)

### Sliders via WebSliderRelay (14 -- already wired in PluginEditor)

| Parameter ID | Group | Relay Name |
|-------------|-------|------------|
| breathPressure | Excitation | breathPressure |
| embouchure | Excitation | embouchure |
| breathNoise | Excitation | breathNoise |
| toneColor | Resonator | toneColor |
| airColumn | Resonator | airColumn |
| jetReflection | Resonator | jetReflection |
| endReflection | Resonator | endReflection |
| vibratoRate | Expression | vibratoRate |
| vibratoDepth | Expression | vibratoDepth |
| width | Output | width |
| outputLevel | Output | outputLevel |
| infiniteSustain | Impossible Physics | infiniteSustain |
| reversedJet | Impossible Physics | reversedJet |
| subHarmonics | Impossible Physics | subHarmonics |

### Tuning Parameters (2 -- handled by tuning-panel.js)

| Parameter ID | Control |
|-------------|---------|
| referencePitch | Tuning panel A4 knob |
| tuningSystem | Tuning panel selector |

### Additional UI Controls (not APVTS, native functions)

| Control | Type | Communication |
|---------|------|---------------|
| Instrument Preset | Dropdown/selector | Native function -> `currentPresetIndex` atomic |
| Tone Hole Enable | Toggle button | Native function -> needs new mechanism (WebToggleButtonRelay or native function) |

## Tone Hole Toggle Implementation

Options:
1. **Add APVTS bool parameter** for tone hole enable -- cleanest, survives preset save/load
2. **Native function** toggle -- simpler but doesn't persist with presets

Recommended: Add `toneHoleEnabled` as AudioParameterBool to APVTS (requires PluginProcessor change). Wire via WebToggleButtonRelay + WebToggleButtonParameterAttachment in PluginEditor.

## Botanical Image

**Source:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/flora/fern_naturalistsmisc1Geor_0089.png`
**Destination:** `plugins/O-Wind/Resources/ui/img/fern.png`
**Positioning:** Right side, ~70% height, 0.35 opacity, pointer-events: none

## Visual Style: Ouaricon Naturalist

Reference: `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`

- **Background:** Aged paper #F5E6D3 with subtle grain
- **Text:** Garamond serif, dark warm brown #3C2F2F
- **Knobs:** 10-segment botanical seed cross-section (55-60px)
- **Accents:** Moss green #8BA870 for active states
- **Borders:** 2px solid walnut brown #8B7355
- **Tabs:** O-Bells-style tab bar below preset browser (uppercase, letter-spacing 1.5px)
- **Preset bar:** Top of window, above tab bar

## Layout Dimensions (900x600)

```
+-- 900px -------------------------------------------+
| [Preset Browser Bar]                     ~40px high |
| [Tab Bar: SOUND | TUNING | EFFECTS]     ~35px high |
|                                                      |
| [Tab Content Area]                      ~525px high  |
|   Sound tab: signal-flow parameter groups            |
|   Tuning tab: tuning-panel.js module                 |
|   Effects tab: placeholder                           |
|                                                      |
+------------------------------------------------------+
         600px
```

## Existing Editor State

PluginEditor.h/cpp already has:
- 14 WebSliderRelay + WebSliderParameterAttachment pairs (all wired)
- WebBrowserComponent with resource provider
- Tuning panel JS/CSS served via resource provider
- WinWebView2 user data folder configured
- 900x600 window size

**Changes needed in Phase 3.1:**
- Add preset module (C++ OuariconPresetManager + native functions + JS)
- Add tone hole toggle parameter + relay + attachment
- Replace shell index.html with full UI
- Add botanical fern image to resources
- Update CMakeLists.txt binary resources (new image, new JS)
- Possibly add instrument preset selector native functions (separate from preset module)

## Constraints

- No viewport units in CSS (WebView limitation)
- Resource provider receives bare paths (not full URLs)
- Must set WinWebView2 user data folder to temp directory (already done)
- NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 (already set)
- Effects tab is blank placeholder -- no DSP effects exist yet

## Open Questions

- Exact positioning of Impossible Physics group vs Tone Hole toggle (bottom-left vs bottom-right?)
- Whether instrument preset selector should be integrated into preset browser bar or as separate control on Sound tab
- Tone hole toggle: add as APVTS parameter or native function? (Recommended: APVTS for persistence)

## Dependencies

- **Requires:** Stage 2 verification complete (confirmed)
- **References:** Ouaricon Naturalist aesthetic, O-Bells preset module pattern, O-Lyrica/O-Bells tab pattern
- **Shared modules:** `modules/persistence/preset-manager/`, tuning-panel.js/css

## Reference Plugins

- **O-Bells** -- Preset module + 2-tab (Instrument/Tuning) + naturalist aesthetic
- **O-Lyrica** -- 4-tab (Sound/Techniques/Tuning/Effects) + preset module
- **O-Formant** -- Naturalist aesthetic with grouped parameter layout

## Next Phase

Ready for: **research** phase (WebView relay patterns, preset module integration, tab implementation details)
