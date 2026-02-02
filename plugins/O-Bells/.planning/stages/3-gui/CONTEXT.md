# Stage 3: GUI Implementation - Context

## Discussion Summary

**Date:** 2026-02-01
**Participants:** User, Claude
**Previous Stage:** Stage 2 (DSP) verified - 18/18 parameters connected (100%)

---

## Requirements Confirmed

### UI Technology
- **Approach:** WebView (JUCE 8 HTML/CSS/JS interface)
- **Rationale:** Allows rich Ouaricon Botanical aesthetic, custom styling, consistent with O-Lyrica

### Window Dimensions
- **Size:** 800x600 pixels
- **Rationale:** Standard size, fits DAW arrangements, sufficient room for controls and snail image

### Layout Architecture
- **Panel Organization:** Tab switching (Instrument / Tuning tabs)
- **Tab 1 (Instrument):** All 18 parameters - synthesis, ensemble, and advanced controls
- **Tab 2 (Tuning):** Blank placeholder for future tuning module implementation
- **Rationale:** Single instrument tab keeps all sound-shaping together; tuning module planned for later

### Visual Design
- **Aesthetic:** Ouaricon Naturalist brand system
- **Hero Image:** Snail (Architectonica perspectiva) as **background watermark**
- **Image Source:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png`
- **Positioning:** Subtle behind controls (low opacity), maximizes control space
- **Tab Transition Animation:** Background PNG shifts horizontally when switching tabs
  - Instrument tab: Default position (e.g., right: -20px)
  - Tuning tab: Shifted further right (e.g., right: -80px) with reduced opacity
  - Smooth CSS transition (0.3-0.4s ease-out)
- **Color Palette:** Warm earth tones per Ouaricon aesthetic
  - Background: Aged paper (#F5E6D3, #EBD9C7)
  - Accents: Moss green (#8BA870), walnut brown (#8B7355)
  - Text: Dark brown (#3C2F2F)

### Control Style
- **Primary Controls:** Horizontal sliders (like O-Lyrica)
- **Style Reference:** O-Lyrica slider implementation with aged paper track, cream thumb
- **Rationale:** User preference for consistency across Ouaricon plugins

### Main Panel Layout
- **Structure:** Two sections
  1. Main synthesis controls (6 parameters)
  2. Ensemble section grouped below (5 parameters)
- **Ensemble Presentation:** Clear visual separation with "ENSEMBLE" label

### Visual Feedback
- **Metering:** Simple output level meter
- **Rationale:** Useful reference, minimal CPU impact

---

## Constraints Identified

### JUCE 8 WebView Requirements
- NEEDS_WEB_BROWSER TRUE in CMakeLists.txt (already set)
- ES6 module loading (type="module")
- WebSliderRelay + WebSliderParameterAttachment for parameter binding
- Percentage-based sizing (no viewport units like vh/vw)
- See `juce8-critical-patterns.md` for critical patterns

### Ouaricon Aesthetic Requirements
- Garamond serif typography
- Seed cross-section knob style (even though using sliders, maintain aesthetic)
- One botanical illustration per plugin
- Wide letter-spacing on uppercase labels
- Warm earth-tone palette

### Performance Constraints
- WebView rendering must not impact DSP performance
- Botanical overlay image optimized (PNG compression)
- Limited pseudo-elements/complex shadows

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| UI Technology | WebView | Rich styling, brand consistency with O-Lyrica |
| Window Size | 800x600 | Standard, accommodates 18 parameters comfortably |
| Panel Layout | Two tabs (Instrument / Tuning) | All params on Instrument, Tuning placeholder for future |
| Hero Image Placement | Background watermark | Maximizes control space, subtle brand presence |
| Hero Image Animation | Shift on tab change | Visual feedback when switching tabs |
| Control Type | Sliders | User preference, consistent with O-Lyrica |
| Ensemble Section | Grouped section on Instrument tab | Clear visual hierarchy |
| Visual Feedback | Level meter | Useful without complexity |
| Typography | Garamond | Brand standard |
| Color System | Ouaricon Naturalist | Brand consistency |

---

## Parameter Layout Plan

### Tab 1: Instrument (All 18 Parameters)

**Section 1: Synthesis (6 sliders)**
| Parameter | ID | Type | Display |
|-----------|-----|------|---------|
| Strike Position | strikePosition | Float 0-100% | Horizontal slider |
| Mallet Hardness | malletHardness | Float 0-100% | Horizontal slider |
| Damping | damping | Float 0-100% | Horizontal slider |
| Brightness | brightness | Float 0-100% | Horizontal slider |
| Material | material | Float 0-100% | Horizontal slider |
| Inharmonicity | inharmonicity | Float 0-100% | Horizontal slider |

**Section 2: Ensemble (5 controls)**
| Parameter | ID | Type | Display |
|-----------|-----|------|---------|
| Unison Count | unisonCount | Int 1-4 | Selector or slider |
| Unison Detune | unisonDetune | Float 0-50 cents | Horizontal slider |
| Octave Blend Sub | octaveBlendSub | Float 0-100% | Horizontal slider |
| Octave Blend Oct | octaveBlendOct | Float 0-100% | Horizontal slider |
| Stereo Spread | stereoSpread | Float 0-100% | Horizontal slider |

**Section 3: Character (3 choice controls)**
| Parameter | ID | Type | Display |
|-----------|-----|------|---------|
| Strike Noise | strikeNoiseChar | Choice (0-2) | Dropdown or buttons |
| Velocity Curve | velocityCurve | Choice (0-2) | Dropdown or buttons |
| Decay Shape | decayShape | Choice (0-2) | Dropdown or buttons |

**Section 4: Advanced (4 sliders)**
| Parameter | ID | Type | Display |
|-----------|-----|------|---------|
| Partial Tuning | partialTuning | Float -100 to +100 | Horizontal slider |
| Pitch Envelope | pitchEnvelope | Float 0-100% | Horizontal slider |
| Pitch Env Time | pitchEnvTime | Float 0.01-2.0s | Horizontal slider |
| Nonlinear Effects | nonlinearEffects | Float 0-100% | Horizontal slider |

**Section 5: Output (1 slider + meter)**
| Parameter | ID | Type | Display |
|-----------|-----|------|---------|
| Output Gain | outputGain | Float -24 to +12 dB | Horizontal slider |
| Output Level | - | Visual only | Level meter |

### Tab 2: Tuning (Placeholder)

**Status:** Empty placeholder for future implementation

**Planned Content:**
- Tuning module integration (to be implemented later)
- Placeholder text: "Tuning module coming soon" or similar

**Visual Treatment:**
- Background snail shifts further right (visual feedback for tab change)
- Reduced opacity on botanical overlay
- Clean, minimal placeholder state

---

## File Structure Plan

```
plugins/O-Bells/
├── Resources/
│   └── ui/
│       ├── index.html          # Main WebView HTML
│       └── img/
│           └── snail.png       # Botanical overlay image
├── Source/
│   ├── PluginProcessor.h/cpp   # Already exists
│   ├── PluginEditor.h/cpp      # Update for WebView
│   └── ui/                     # (alternative resource location)
└── CMakeLists.txt              # Already has NEEDS_WEB_BROWSER TRUE
```

---

## Open Questions

None - all requirements clarified during discussion.

---

## Next Phase

**Ready for:** Research phase

**Research Topics:**
1. JUCE 8 WebView parameter binding patterns (WebSliderRelay, WebSliderParameterAttachment)
2. Level meter implementation in WebView
3. O-Lyrica slider CSS implementation details
4. Botanical overlay positioning with watermark effect
5. Tab-based botanical overlay animation (O-Lyrica has `.botanical-overlay.techniques-position`, `.botanical-overlay.tuning-position` classes for this pattern)

---

## Reference Files

- **BRIEF:** `plugins/O-Bells/.planning/BRIEF.md`
- **ROADMAP:** `plugins/O-Bells/.planning/ROADMAP.md`
- **Stage 2 Verification:** `plugins/O-Bells/.planning/stages/2-dsp/VERIFICATION.md`
- **Ouaricon Aesthetic:** `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`
- **O-Lyrica UI:** `plugins/O-Lyrica/Resources/ui/index.html`
- **JUCE 8 Patterns:** `troubleshooting/patterns/juce8-critical-patterns.md`
- **Parameter Spec:** `plugins/O-Bells/.planning/parameter-spec.md`

---

*Discussion completed: 2026-02-01*
