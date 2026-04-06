# Stage 3: GUI Phase 3.2 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Phase Scope

Visual polish and advanced UI elements for the WebView UI established in Phase 3.1.

### Features

| Feature | Priority | Description |
|---------|----------|-------------|
| Formant peaks overlay | nice (UI-02) | F1-F5 dot markers on XY pad showing current formant positions |
| Cursor glow | nice | Radial glow effect behind XY pad cursor |
| ADSR curve display | nice | Visual envelope curve in the Envelope section |

## Requirements Targeted

| Requirement | Priority | Status |
|-------------|----------|--------|
| UI-02: Real-time formant peaks overlay (F1-F5 frequency bars) | nice | Deferred from 3.1 -> implementing as dot markers |

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Formant data source | JS-side computation | Formant freqs are deterministic from APVTS params (vowelX, vowelY, vowelFocus, formantShift, formantSpread) already available via relays. No atomics or DSP-to-UI channel needed. Works even when audio isn't running. |
| Formant display format | Dot markers on XY pad | User preference. Simpler than vertical bars or heat map. Shows F1-F5 as small labeled dots. |
| Heat map | Deferred to v1.1 | User interested but too complex for this phase (requires pre-computed texture). Dot markers first, iterate later. |
| MPE formant offsets | Not reflected in overlay | Per-voice MPE offsets (mpeVowelYOffset) modify formant positions in audio thread. JS computation uses base param values only. Acceptable for v1. |
| Cursor glow | Radial gradient on canvas | Draw radial gradient behind cursor dot in drawXYPad(). Simple, no performance concern. |
| ADSR display | Canvas or inline SVG | Small visualization in the Envelope bottom-group showing the 4-stage curve. Driven purely by the 4 APVTS relay values. |

## Implementation Details

### 1. Formant Peaks Overlay (JS-side computation)

Port VowelMorpher logic to JavaScript:
- Use same VowelData vowel positions and formant tables (hardcode in JS, matches VowelData.h)
- Shepard IDW interpolation with current vowelFocus value
- Apply formantShift (semitone-based pitch shift) and formantSpread (distance from center-of-mass)
- Draw 5 small dots on the XY pad at positions mapped from F1-F5 frequencies
- Label each dot (F1-F5) with small text
- Update on vowelX/vowelY/vowelFocus/formantShift/formantSpread relay value changes

**Frequency-to-position mapping:** F1-F5 ranges roughly 200-5000 Hz. Map to XY pad coordinates:
- X axis: log-frequency mapping (200 Hz = left, 5000 Hz = right)
- Y axis: formant amplitude/gain (from VowelData gain values)
- Or: simpler approach — just show F1-F5 as frequency labels along the bottom edge of the pad

**Preferred approach:** Overlay dots within the XY pad area. F1-F5 positions on a horizontal frequency axis near the bottom of the pad, with height indicating relative gain. Small circles (3-4px radius) with F1-F5 labels, colored in moss green (#8BA870) at reduced opacity.

### 2. Cursor Glow

In `drawXYPad()`, before drawing the cursor dot:
- Create radial gradient centered at cursor position
- Inner color: rgba(139, 168, 112, 0.3) (moss green, semi-transparent)
- Outer color: transparent
- Radius: ~25-30px
- Draws behind existing dot + crosshair

### 3. ADSR Curve Display

Add a small canvas element (or repurpose existing space) in the Envelope bottom-group:
- Approximately 120x50px alongside the 4 knobs
- Draw classic ADSR curve: attack ramp -> decay curve -> sustain level -> release curve
- Use exponential curves matching juce::ADSR behavior
- Update when any of the 4 relay values change
- Style: moss green (#8BA870) stroke on paper background, 1.5px line

**Layout adjustment:** The bottom-group Envelope section currently has 4 knobs in a row. Add a small ADSR canvas to the right of the knobs, or above them if space is tight. The bottom row has flex: 1 on each group, so there should be room.

## Files to Modify

| File | Changes |
|------|---------|
| `Source/ui/public/js/main.js` | Add VowelData tables, formant computation, glow drawing, ADSR canvas logic |
| `Source/ui/public/index.html` | Add ADSR canvas element in envelope group |

**No C++ changes required.** All features are purely UI-side using existing relay data.

## Constraints

- No new relays or attachments needed (all data comes from existing 21 relays)
- No audio thread changes
- Canvas DPR scaling must be maintained for formant dots and ADSR canvas
- Performance: formant computation is lightweight (~50 FLOPs per update), triggered by relay value changes only
- Formant overlay dots must not obscure vowel labels or cursor

## Open Questions

None — scope is clear.

## Dependencies

- **Requires:** Phase 3.1 complete (WebView UI with all 21 relays bound) -- DONE
- **VowelData source:** `Source/dsp/VowelData.h` (5 vowels, F1-F5 freq/bw/gain)
- **VowelMorpher reference:** `Source/dsp/VowelMorpher.h` (Shepard IDW, log-freq blend)

## Next Phase

Ready for: research phase (JUCE WebView canvas patterns, ADSR visualization approaches)
