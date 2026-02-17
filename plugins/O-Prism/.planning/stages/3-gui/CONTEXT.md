# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-17
**Participants:** User, Claude

## Requirements Confirmed

### Layout Architecture
- **3-tab layout:** SYNTH | TUNING | EFFECTS
- **Persistent elements:** Header bar (O-PRISM branding) + bottom master strip (Master Volume, Osc Mix, Polyphony) always visible regardless of active tab
- **Window size:** 1200 x 800 pixels (as per BRIEF)

### Tab Structure

**Tab 1: SYNTH** (default/home tab)
- Oscillator A: wavetable display, position, coarse, fine, phase, unison, detune, width, level, pan (10 params)
- Oscillator B: identical layout to Osc A (10 params)
- Sub Oscillator: shape, octave, level (3 params)
- Noise Generator: type, level (2 params)
- Osc Mix slider (1 param — also in persistent footer)
- Filter A: type, cutoff, resonance, drive, key tracking (5 params)
- Filter B: type, cutoff, resonance, drive, key tracking (5 params)
- Filter Routing: serial/parallel toggle (1 param)
- Amp Envelope: ADSR (4 params)
- Filter Envelope: ADSR + depth (5 params)
- **Total: ~46 parameters on this tab**

**Tab 2: TUNING**
- Tuning preset selector (dropdown of 24+ factory tunings)
- Tonic selector (C through B)
- Master tune (Hz)
- Octave stretch
- Pitch bend range
- Glide mode (Off/Legato/Always) + glide time
- Load .scl button (Scala file import)
- Load .kbm button (keyboard mapping import)
- Embedded tuning library browser (categories + tunings list)
- Scale generators (EDO, harmonic series, rank-2)
- Export tuning documentation button
- **Total: ~7 slider params + native function UI**

**Tab 3: EFFECTS** (with sub-tabs)
- Sub-tab bar: [Reverb] [Delay] [Chorus] [Distortion] [EQ]
- One effect visible at a time
- Reverb: size, damping, pre-delay, mix (4 params)
- Delay: time, feedback, sync toggle, mode, mix (5 params including toggle)
- Chorus: rate, depth, mix (3 params)
- Distortion: type, drive, mix (3 params)
- EQ: low gain, mid gain, mid freq, high gain (4 params)
- **Total: ~19 parameters across 5 sub-tabs**

### Persistent Footer Strip
- Master Volume knob
- Osc Mix slider (A <-> B)
- Polyphony control
- Always visible across all tabs

### Visual Design

**Aesthetic:** Ouaricon Naturalist (ouaricon-naturalist-001)
- Earth tones: warm browns, aged paper backgrounds, muted greens
- Typography: Garamond for section headers (uppercase, wide letter-spacing)
- Seed cross-section knob design
- Brown/amber accent colors

**Knob Size:** 50px (medium) — fits ~12-14 knobs per row with labels

**Botanical Image:** Snow bunting specimen illustration (provided by user)
- Two birds on rocky perch — muted browns, whites, warm earth tones
- Used as background watermark at low opacity (~0.15-0.20)
- **Shifts position/crop when switching tabs** (matching O-Bells, O-Lyrica, O-Marimba pattern)
- Each tab sees a slightly different portion/position of the image

**Wavetable Display:**
- 2D waveform rendered on HTML Canvas
- Updates on position parameter change only (not real-time during playback)
- Styled as specimen illustration (aged paper background, brown border)
- One display per oscillator (Osc A and Osc B)

### Parameter Binding
- 73 WebSliderRelay instances (fix from numSliderParams=67 bug)
- 1 WebToggleButtonRelay (delaySync)
- WebSliderParameterAttachment for all params (3-arg constructor)
- 23 native tuning functions already registered in PluginEditor.cpp

## Constraints Identified

- **74 total APVTS parameters** — highest in Ouaricon catalog
- Must fix numSliderParams from 67 to 73 (Stage 2 carry-over bug)
- Must add "BP24" to filtAType/filtBType choices (7 types total, fix Stage 1 bug)
- WebView performance target: 60fps during interaction
- No real-time audio-thread-to-UI data streaming needed (waveform is static)
- Cross-platform: resource provider URLs must use `getResourceProviderRoot()` / `getBackendResourceAddress()`
- Windows: WebView2 static linking already configured, user data folder already set

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Layout | 3-tab (SYNTH / TUNING / EFFECTS) | Reduces density vs single-page; clean grouping |
| Persistent UI | Header + master footer | Master volume always accessible; branding always visible |
| Effects sub-UI | Sub-tabs (one effect at a time) | 5 effects with ~4 params each — tabs keep it clean |
| Waveform display | 2D Canvas, position-change only | Low CPU cost, no C++->JS streaming needed |
| Knob size | 50px medium | Balance of density and usability |
| Botanical | Snow bunting, shifting watermark | Matches existing Ouaricon plugin pattern (O-Bells etc.) |
| Design flow | Straight to implementation | Use BRIEF wireframe + Naturalist template, no mockup phase |

## Fixes to Apply in Stage 3

1. **numSliderParams bug:** Change from 67 to 73 in PluginEditor.h
2. **Filter type parameter:** Add "BP24" choice to filtAType/filtBType APVTS definitions (PluginProcessor.cpp), making 7 choices: LP12, LP24, HP12, HP24, BP12, BP24, Notch

## Implementation Phases (from ROADMAP.md, adapted)

### Phase 3.1: Layout + Basic Controls + Bug Fixes
- HTML/CSS tabbed layout (SYNTH | TUNING | EFFECTS)
- Ouaricon Naturalist styling (CSS)
- Knob components for all continuous parameters
- Dropdown menus for choice parameters
- Fix numSliderParams and filter type bugs
- Resource provider URL mapping for all assets
- Snow bunting botanical watermark with tab-shift

### Phase 3.2: Full Parameter Binding + Tuning Panel
- All 73 slider params bound with WebSliderParameterAttachment
- delaySync toggle bound
- Value displays for each parameter
- Tuning panel integration (tuning-panel.js from scala-tuning-engine module)
- Host automation <-> UI sync verified
- Wavetable selector UI (dropdown per oscillator)

### Phase 3.3: Wavetable Visualization + Polish
- 2D Canvas waveform display per oscillator
- Position indicator on waveform
- Effect sub-tab switching
- Button states, hover effects, visual feedback
- Final aesthetic polish and alignment
- Cross-browser testing (WebView2 + WebKit)

## Open Questions

- Exact wavetable selector presentation (dropdown list vs grid of thumbnails) — resolve during Phase 3.2
- Whether polyphony control belongs in footer or SYNTH tab — resolve during Phase 3.1 layout

## Existing Assets

- PluginEditor.h/cpp: WebView shell with 23 native functions, relay infrastructure
- index.html: Placeholder (to be replaced)
- JUCE index.js + check_native_interop.js: Already in place
- Ouaricon Naturalist CSS: Available from other plugins (O-Bells, O-Lyrica, O-Marimba)
- Tuning panel JS: Available from scala-tuning-engine module
- Snow bunting specimen image: Provided by user (to be embedded as BinaryData)

## Next Phase

Ready for: **research** phase (investigate existing Ouaricon UI patterns, CSS reuse, Canvas waveform rendering)
