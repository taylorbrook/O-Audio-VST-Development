# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-04-06
**Participants:** User, Claude

## Starting State

- C++ editor fully wired: 28 WebSliderRelays + 6 WebComboBoxRelays + 1 WebToggleButtonRelay + WebView + all 35 attachments
- index.html is placeholder shell -- no actual UI
- Window size: 900x600
- 35 APVTS parameters total (33 active in DSP, instrumentPreset deferred to GUI morph)
- All relays and attachments already declared in PluginEditor.h -- no C++ changes needed for binding

## Requirements Confirmed

- **Aesthetic:** Ouaricon Naturalist (ouaricon-naturalist-001) -- aged paper, seed cross-section knobs, serif typography (Garamond), botanical green accents, brown borders/structure, botanical illustration overlay
- **Window size:** 900x600 (no change)
- **Layout:** Three-panel navigation (tabs or sidebar)
  - **Panel 1 - Instrument:** All instrument/performance controls in scrollable view with collapsible sections
  - **Panel 2 - Tuning:** Tuning system, reference pitch, and related controls
  - **Panel 3 - Effects:** Reserved for future (empty placeholder or hidden)
- **Instrument panel:** Collapsible sections for each parameter group, vertically scrollable
- **Bore visualization:** Real-time SVG/canvas showing bore taper shape responding to parameters
- **Instrument morph:** XY pad for morphing between instrument presets in 2D space
- **All 35 parameters** must have UI controls with two-way binding
- **Skip UI mockup** -- go straight to implementation

## Constraints Identified

- 900x600 with tabs/panels means each panel gets full height minus header/tab bar (~550px usable)
- 35 params on one scrollable panel with collapsible sections is viable -- most sections collapsed by default
- XY pad for instrument morph needs meaningful 2D axes (e.g., bore character X, double reed amount Y)
- Bore visualization must update in real-time without excessive CPU from JS rendering
- Botanical image needs careful placement to not conflict with scrollable content
- WebView scrolling must feel native -- no custom scroll implementations

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Aesthetic | Ouaricon Naturalist | Brand consistency across all Ouaricon plugins |
| Layout | Tab-based panels (Instrument / Tuning / Effects) | Clean separation of concerns, full panel space per view |
| Instrument panel | Scrollable with collapsible sections | 30+ params organized by function, progressive disclosure |
| Bore viz | Real-time SVG/canvas | Interactive visual feedback of bore profile shape |
| Instrument morph | XY pad | 2D morphing between instrument parameter sets |
| Window size | 900x600 | Standard, fits most screens |
| Effects panel | Placeholder for future | Reserved tab, not implemented in Stage 3 |
| Mockup | Skipped | Direct to implementation |

## Panel Layout Specification

### Header Bar
```
+----------------------------------------------------------+
|  O-REED                        [Instrument] [Tuning] [FX] |
+----------------------------------------------------------+
```
- Plugin title left-aligned, Garamond, uppercase, letter-spacing
- Tab buttons right-aligned in header bar

### Panel 1: Instrument (Default View)

Scrollable content with collapsible sections:

```
+----------------------------------------------------------+
| [Instrument Morph XY Pad]                                 |  ~180px tall
|   X: Bore Character    Y: Double Reed (Psi)              |
|   Current: [preset label]                                |
+----------------------------------------------------------+
| v PRIMARY CONTROLS                                        |
|   Breath Pressure  Embouchure  Reed Hardness  Output Gain |
+----------------------------------------------------------+
| > BORE & RESONANCE                                        |
|   Bore Character  Bore Diameter  Bell Size  Bore Length   |
|   Bore Profile (dropdown)                                 |
+----------------------------------------------------------+
| > BORE VISUALIZATION                                      |
|   [SVG/Canvas: bore taper profile]                        |
+----------------------------------------------------------+
| > REED                                                    |
|   Reed Opening  Reed Mass  Reed Damping  Double Reed      |
|   Mouthpiece Volume                                       |
+----------------------------------------------------------+
| > EXPRESSION                                              |
|   Vibrato Depth  Vibrato Rate  Vibrato Source (dropdown)  |
|   Growl Amount  Flutter Tongue  Subtone  Attack Chiff     |
|   Air Noise                                               |
+----------------------------------------------------------+
| > SOUND DESIGN (Impossible Physics)                       |
|   Infinite Sustain  Reverse Bore  Feedback Path           |
|   Dual Bore (toggle)  Drone Pitch                         |
+----------------------------------------------------------+
| > VOICE                                                   |
|   Poly Mode (dropdown)  Max Voices  Oversampling          |
+----------------------------------------------------------+
```

- **Primary Controls** expanded by default, all others collapsed
- Each section header is clickable to expand/collapse
- Smooth CSS transitions on expand/collapse

### Panel 2: Tuning

```
+----------------------------------------------------------+
| TUNING                                                    |
|                                                           |
|   Reference Pitch  [knob, 220-880 Hz, default 440]       |
|                                                           |
|   Tuning System    [dropdown: 12TET / Scala / MTS-ESP]   |
|                                                           |
|   [Future: Scala file browser, scale visualization]       |
+----------------------------------------------------------+
```

- Only 2 parameters -- spacious layout with generous whitespace
- Room for future Scala file browser and scale visualization

### Panel 3: Effects (Future)

```
+----------------------------------------------------------+
| EFFECTS                                                   |
|                                                           |
|   Coming soon                                             |
+----------------------------------------------------------+
```

- Placeholder -- reserved for future effect chain additions

## Parameter-to-Section Mapping

### Instrument Panel

| Section | Parameters | Control Types |
|---------|-----------|---------------|
| XY Pad | instrumentPreset (macro) | XY pad (custom) |
| Primary Controls | breathPressure, embouchure, reedHardness, outputGain | 4 knobs |
| Bore & Resonance | boreCharacter, boreDiameter, bellSize, boreLength, boreProfile | 4 knobs + 1 dropdown |
| Bore Visualization | (responds to bore params) | SVG/canvas |
| Reed | reedOpening, reedMass, reedDamping, doubleReed, mouthpieceVol | 5 knobs |
| Expression | vibratoDepth, vibratoRate, vibratoSource, growlAmount, flutterTongue, subtone, attackChiff, airNoise | 6 knobs + 1 dropdown + 1 knob |
| Sound Design | infiniteSustain, reverseBore, feedbackPath, dualBore, dronePitch | 3 knobs + 1 toggle + 1 knob |
| Voice | polyMode, maxVoices, oversampling | 1 knob + 2 dropdowns |

### Tuning Panel

| Section | Parameters | Control Types |
|---------|-----------|---------------|
| Tuning | referencePitch, tuningSystem | 1 knob + 1 dropdown |

**Total: 35 parameters across 2 panels (28 knobs + 6 dropdowns + 1 toggle)**

## Bore Visualization Spec

- **Type:** SVG or Canvas element, ~300x120px within collapsible section
- **Content:** Cross-section profile of bore shape (side view)
  - Left = mouthpiece end, Right = bell end
  - Upper/lower curves show bore radius along length
  - Responds to: boreCharacter (taper), boreDiameter (width), bellSize (flare), boreLength (extent), reverseBore (inverts taper), boreProfile (simple vs multi-segment)
- **Dual bore:** Second bore outline appears when dualBore toggled on, offset by dronePitch
- **Colors:** Brown (#8B7355) outline on aged paper background, green (#6B8E4E) fill for active playing region
- **Update rate:** On parameter change (not every frame)

## XY Pad Spec

- **Size:** ~300x180px at top of Instrument panel
- **Axes:** X = bore character (cylindrical to conical), Y = double reed amount (single to double)
- **Preset markers:** Named positions for each instrument preset (Clarinet, Saxophone, Oboe, Duduk, etc.)
- **Current position:** Draggable crosshair/dot showing current parameter values
- **Interaction:** Drag to morph -- updates boreCharacter and doubleReed params simultaneously
- **Visual:** Aged paper background, brown grid lines, green preset markers, green crosshair for current position
- **Labels:** Instrument names near their preset positions in small Garamond

## Botanical Illustration

- **Plugin type:** Synthesizer (reed wind instrument)
- **Character:** Warm, organic, airy -- reed instruments are made from actual reeds/cane
- **Best fit:** Flora category -- botanical illustration of grass/reed/cane plant (direct thematic connection: reed instruments use Arundo donax cane)
- **Alternative:** Birds (wind/breath connection) or insects (buzzing reed vibration)
- **Placement:** Right side, ~70% height, opacity 0.3, pointer-events: none
- **Note:** Image TBD -- select from available library or source appropriate reed/cane botanical illustration

## Phase Breakdown

- **Phase 4.1:** HTML/CSS layout, tab navigation, collapsible sections, XY pad, seed knobs for all params, bore viz placeholder
- **Phase 4.2:** Full two-way parameter binding (all 35 params via relays), host automation sync, XY pad <-> parameter interaction
- **Phase 4.3:** Bore visualization (real-time SVG), preset browser, Scala file browser, advanced polish

## Open Questions

- Which specific botanical illustration PNG for reed/wind theme? (Check available library at implementation time)
- XY pad preset positions -- should they be hardcoded or derived from instrumentPreset parameter state? (Recommend: hardcoded positions based on BRIEF.md instrument specs)
- Scala/TUN file browser -- native file dialog via C++ callback or WebView file picker? (Defer to Phase 4.3)

## Next Phase

Ready for: research phase
