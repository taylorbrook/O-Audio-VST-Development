# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Requirements Confirmed

- **Ouaricon Naturalist aesthetic** applied throughout (aged paper, earth tones, serif typography, botanical motifs)
- **XY pad dominant layout** matching BRIEF ASCII mockup: XY pad ~50% left, vertical sliders right (CharA, CharB, Evolve), source selector below, Brightness/Mix/Freeze at bottom
- **Orbital trail visualization** in XY pad: cursor leaves fading trail showing recent latent path, Evolve creates orbital motion around current position
- **Dark inset parchment** XY pad surface: darker paper tone (#D4C4B0 or #C8B8A0) with inset shadow, trails rendered in botanical green (#6B8E4E)
- **Ice crystal overlay** for Freeze state: subtle crystalline pattern overlays pad surface when Freeze is active, trails freeze in place
- **Naturalist line art icons** for 6 source categories: brown ink line drawings (raindrop, anvil/gear, swirl, group, wave, leaf)
- **Icon buttons** for source selector: 6 square buttons with line art icons, highlighted when selected
- **Lichen/fungi botanical illustration** as decorative overlay (right side, 0.3-0.4 opacity)
- **800x600 window** (keep current size)
- **10 parameters** all bound via WebView relays + APVTS attachments:
  - SOURCE (choice, 6 options) -- icon button selector
  - MODE (choice, 2 options) -- Generate/Transform toggle at top
  - X (float 0-1) -- XY pad horizontal
  - Y (float 0-1) -- XY pad vertical
  - CHARACTER_A (float 0-1) -- vertical slider right of pad
  - CHARACTER_B (float 0-1) -- vertical slider right of pad
  - EVOLVE (float 0-1) -- vertical slider right of pad
  - FREEZE (bool) -- toggle button in bottom row
  - BRIGHTNESS (float -1 to +1) -- knob in bottom row
  - MIX (float 0-1) -- knob in bottom row

## Constraints Identified

- **800x600 is compact** for XY pad + side sliders + source selector + bottom controls -- layout must be space-efficient while maintaining naturalist aesthetic spacing principles
- **Orbital trail animation** requires requestAnimationFrame loop in WebView -- must be efficient to avoid CPU overhead in a plugin context
- **Ice crystal overlay** needs careful opacity tuning to not obscure XY pad usability when Freeze is active
- **6 source icons** as naturalist line art SVGs need to be created (inline SVG or embedded)
- **Lichen/fungi botanical image** needs to be sourced/created as transparent PNG
- **XY pad is 2D input** -- requires custom pointer event handling for both X and Y parameter updates simultaneously
- **WebView relay/attachment pattern** must follow JUCE 8 WebBrowserComponent native integration (WebSliderRelay, WebToggleButtonRelay, etc.)
- **Resource provider** must serve additional assets (CSS, JS, SVG icons, botanical image) beyond just index.html

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Aesthetic | Ouaricon Naturalist | Brand consistency, user preference |
| Layout | BRIEF ASCII as-is | XY pad left (dominant), vertical sliders right, source below, controls bottom |
| XY Pad visualization | Orbital trails | Cursor leaves fading trail, Evolve creates orbital motion -- shows latent history |
| XY Pad surface | Dark inset parchment | Darker paper (#D4C4B0) with inset shadow, botanical green trails |
| Freeze visual | Ice crystal overlay | Crystalline pattern overlay on pad, trails freeze in place |
| Source selector | Icon buttons (6) | Naturalist line art SVGs in brown ink, highlighted when active |
| Botanical image | Lichen/fungi | Thematic match: lichen IS natural surface texture |
| Icon style | Naturalist line art | Brown ink line drawings matching aged manuscript aesthetic |
| Window size | 800x600 | Keep current, compact but workable |
| Mode toggle | Top header area | Generate/Transform toggle prominent at top, naturalist button style |
| Bottom controls | Brightness knob + Mix knob + Freeze toggle | Seed cross-section knobs for Brightness/Mix, green botanical toggle for Freeze |
| Vertical sliders | Naturalist slider style | Aged paper inset track, simplified seed disc thumb |

## UI Layout Plan

```
800 x 600
┌──────────────────────────────────────────────────────────────┐
│  O-TEXTURE              [Generate | Transform]      800w     │  <- Header (36px)
│                                                              │
│  ┌────────────────────────────┐  ╭──────╮  ╭──────╮        │
│  │                            │  │ Char │  │ Char │        │
│  │    XY PAD (dark inset)     │  │  A   │  │  B   │        │
│  │    orbital trails          │  │      │  │      │        │  <- Main area (~400px)
│  │    ice crystal on freeze   │  ╰──────╯  ╰──────╯        │
│  │                            │                              │
│  │         ● (cursor)         │  ╭──────╮                   │
│  │                            │  │Evolve│                   │
│  └────────────────────────────┘  ╰──────╯                   │
│                                                              │
│  [🌧Rain] [⚙Metal] [🌀Wind] [👥Crowd] [〰Synth] [🍃Organic] │  <- Source selector (~50px)
│                                                              │
│  ⟨Brightness ●──⟩   ⟨Mix ●──⟩   [❄ FREEZE]                │  <- Bottom strip (~80px)
└──────────────────────────────────────────────────────────────┘
```

## Parameter-to-Control Mapping

| Parameter | Control Type | WebView Element | Relay Type |
|-----------|-------------|-----------------|------------|
| SOURCE | Icon button group | 6 SVG buttons | WebComboBoxRelay |
| MODE | Toggle button pair | 2-state toggle | WebComboBoxRelay |
| X | XY pad horizontal | Canvas pointer events | WebSliderRelay |
| Y | XY pad vertical | Canvas pointer events | WebSliderRelay |
| CHARACTER_A | Vertical slider | Custom slider | WebSliderRelay |
| CHARACTER_B | Vertical slider | Custom slider | WebSliderRelay |
| EVOLVE | Vertical slider | Custom slider | WebSliderRelay |
| FREEZE | Toggle button | Bool toggle | WebToggleButtonRelay |
| BRIGHTNESS | Rotary knob | Seed cross-section knob | WebSliderRelay |
| MIX | Rotary knob | Seed cross-section knob | WebSliderRelay |

## File Structure Plan

```
plugins/O-Texture/Source/ui/public/
├── index.html          (main page, inline or linked CSS/JS)
├── css/
│   └── ouaricon-naturalist.css   (shared aesthetic CSS)
├── js/
│   └── main.js         (parameter binding, XY pad, animations)
└── img/
    └── lichen.png      (botanical overlay, transparent PNG)
```

## Animation & Interaction Details

### XY Pad - Orbital Trails
- Canvas-based rendering (HTML5 Canvas 2D)
- Trail = array of recent positions (last 30-60 frames)
- Each point drawn as circle with decreasing opacity (newest = opaque, oldest = transparent)
- Trail color: botanical green (#6B8E4E) with alpha gradient
- When Evolve > 0, cursor drifts with orbital motion (parameter updates from processor reflected in pad position)
- requestAnimationFrame loop, throttled to 30fps to save CPU
- Pointer down + move on canvas updates X and Y parameters simultaneously

### XY Pad - Freeze State
- Ice crystal overlay: CSS pseudo-element or SVG pattern over canvas
- Subtle crystalline geometric pattern at low opacity (0.15-0.2)
- Trail stops updating (frozen in place)
- Cursor position still shown but marked as "pinned" (subtle border or glow)
- Smooth transition in/out (0.3s fade)

### Knob Interaction
- Vertical drag to adjust (standard DAW knob behavior)
- Seed cross-section visual rotates with value
- Double-click to reset to default

### Vertical Sliders
- Vertical drag, thumb moves up/down
- Naturalist aesthetic track and thumb
- Value display below slider

## Open Questions

- Exact lichen/fungi PNG to source -- may need to create or find suitable transparent botanical illustration
- JUCE 8 WebSliderRelay/WebToggleButtonRelay exact API for XY pad (two relays controlled by one pointer interaction)
- Whether to use HTML Canvas or SVG for XY pad orbital trail rendering (Canvas likely better for animation performance)
- Exact naturalist line art SVG designs for the 6 source category icons

## Next Phase

Ready for: **research** phase
- Investigate JUCE 8 WebBrowserComponent relay/attachment API
- Research WebView Canvas animation performance in plugin context
- Research SVG icon creation approach for naturalist line art
- Look at O-TextureForge GUI implementation as reference (same aesthetic, similar XY pad concept)
