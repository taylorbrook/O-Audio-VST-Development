# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-07
**Participants:** User, Claude

## Requirements Confirmed

- **Window size:** 700 x 250 (wide, thin strip layout)
- **7 parameters** to bind: rate, depth, voices, width, tone, mix, drive
- **Visual style:** Ouaricon Naturalist template (paper texture, botanical overlays, Garamond serif, brown/green/wheat palette)
- **Knob style:** Segment knobs with conic-gradient (wheat/tan alternating), matching O-DigiDelay pattern
- **Voices control:** Stepped rotary knob snapping to 1-8 integer values
- **LFO indicator:** Animated ring/orbit showing modulation movement in real-time
- **LFO animation method:** JS-driven using Rate parameter value (no C++ timer needed)

## Constraints Identified

- WebView infrastructure already complete (relays, attachments, resource provider in PluginEditor)
- 7 relays + 7 attachments already wired in Stage 1 editor code
- Only `index.html` needs replacement (plus any new CSS/JS/image resources)
- Resource provider serves from BinaryData — all assets must be added to CMakeLists.txt
- Must follow JUCE 8 WebView patterns (type="module", getBackendResourceAddress(), etc.)
- Cross-platform URL scheme: use `getResourceProviderRoot()` / `getBackendResourceAddress()`

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Layout | 700x250 horizontal strip | User preference — wide and thin |
| Visual style | Ouaricon Naturalist | Match existing plugin portfolio aesthetic |
| Knob type | 10-segment conic gradient | Consistent with O-DigiDelay, O-AnalogSaturation |
| Voices display | Stepped knob | Consistent with other rotary controls |
| LFO indicator | JS-animated ring | Simpler than C++ timer, visually sufficient |
| LFO method | Derive from Rate param in JS | No audio thread changes needed |
| Parameter grouping | Modulation (rate, depth, voices) + Character (width, tone, mix, drive) | Logical grouping from parameter-spec.md |
| Background | Paper texture + botanical overlay | Portfolio consistency |
| Typography | Garamond serif, dark brown #3C2F2F | Portfolio standard |

## Design Specification

### Layout (700 x 250)

```
┌──────────────────────────────────────────────────────────────────────┐
│  O-CHORUS                                          [< Preset Name >]│  ← Header bar (22px)
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│   RATE    DEPTH   VOICES    [LFO]    WIDTH    TONE    MIX    DRIVE  │  ← Knobs row
│    ◎       ◎       ◎        ○○○      ◎        ◎       ◎       ◎    │
│   1.0Hz   50%      4                 70%      0%     50%     30%    │  ← Values
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

- Header: Plugin title left, preset bar right
- Center: LFO animated ring between Modulation and Character groups
- 7 knobs evenly spaced with labels above and values below

### Color Palette (from Ouaricon Naturalist)

- Background: Paper texture (#F5E6D3 base)
- Knob segments: Wheat #F5DEB3 / Tan #E8D5B7 alternating
- Knob border: Brown #8B7355
- Labels: Dark brown #3C2F2F, uppercase, Garamond
- Active accents: Green #6B8E4E
- LFO ring: Sage green #5a7a6a stroke
- Plugin border: 2px solid #8B7355

### Knob Interaction

- Vertical drag to change value (relative delta, not absolute)
- Double-click to reset to default
- Shift+drag for fine control
- Voices knob: same interaction but snaps to integer steps

### LFO Ring Animation

- SVG circle with `stroke-dasharray` animation
- Rotation speed derived from Rate parameter value
- Scale/pulse effect derived from Depth parameter
- Color: Sage green #5a7a6a (matching vine-arc style from O-Comp)

## Assets Needed

1. Paper texture image (paper1.jpg — reuse from existing plugins)
2. Botanical overlay image (optional — could use butterfly or floral)
3. Main CSS (inline or separate file)
4. Main JS (app.js — knob logic, JUCE interop, LFO animation)
5. JUCE interop scripts (already exist: index.js, check_native_interop.js)

## Open Questions

None — ready for research/plan phase.

## Next Phase

Ready for: research phase (or skip to plan, since GUI patterns are well-established)
