# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-11
**Participants:** User, Claude

## Arriving From Stage 2

Stage 2 (DSP) is fully verified:
- 31/31 requirements complete, 0 failed, 4 deferred to this stage
- All 4 motion paths, VBAP (stereo through 24ch), distance model, auto-downmix, L+R split
- 3 targets build with zero O-Orbit warnings, standalone launches
- Clean architecture: background VBAP compute thread, lock-free audio thread

### Deferred Requirements Landing Here

| Requirement | Description |
|-------------|-------------|
| FR-3.2 | Custom speaker layout editor (add, remove, reposition speakers) |
| FR-3.4 | Save/load custom speaker layouts as user presets |
| FR-3.5 | Import/export speaker layout files for sharing |
| FR-6.4 | Visual downmix warning badge |

## Requirements Confirmed

### Layout & Structure
- **Window size:** 800x600
- **Layout style:** Central visualizer — large orbital visualizer dominates center, parameter controls arranged around it (grouped sections below)
- **Speaker editor access:** Toggle panel — button switches between "Motion View" (orbital visualizer) and "Speaker Editor" in the same central space

### Visual Aesthetic: Ouaricon Botanical / Naturalist
- **Template:** Ouaricon Audio Naturalist (`.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`)
- **Background:** Aged paper tone (#F5E6D3) with subtle paper grain texture
- **Text:** Warm dark brown (#3C2F2F primary, #5C4033 secondary), Garamond serif font family
- **Knobs:** Botanical seed cross-section design (10-segment conic gradient, cream/brown tones)
- **Buttons/toggles:** Green botanical theme (muted green #8BA870, deeper #6B8E4E for active)
- **Borders:** Warm brown (#8B7355, #5C4033), 2-3px solid
- **Shadows:** Soft, organic (2px 2px 6px rgba(0,0,0,0.25))
- **Labels:** Garamond, uppercase, 0.5-2px letter spacing
- **Decorative:** Fleurons (low opacity) where appropriate

### Botanical Illustration
- **Image:** Shell (ocean category) — spiral shell echoing orbital/spiral motion, nautilus geometry
- **Placement:** Right side, slightly bleeding off edge, centered vertically
- **Height:** 60-75% of plugin height
- **Opacity:** 0.3-0.4 (visible but not competing with controls)
- **Z-index:** Above background, below controls

### Orbital Visualizer
- **Style:** Integrated botanical — uses the earth-tone palette throughout
- **Path trails:** Warm brown/amber fade lines
- **Source dot:** Muted green (#8BA870) glowing dot
- **L+R split:** Two dots (green for L, warm brown/amber for R)
- **Speaker icons:** Cream/brown icons with channel labels around perimeter
- **Background:** Slightly darker area (like a naturalist's diagram plate) within aged paper
- **Animation:** Canvas-based, 60fps via requestAnimationFrame
- **View:** Top-down 2D by default

### Speaker Layout Editor (Toggle View)
- **Same space as orbital visualizer** — toggled via button
- **Top-down circle view** showing speaker positions
- **Drag-to-reposition** speakers (updates azimuth, elevation)
- **Click to add** speaker, right-click to remove
- **Text inputs** for precise az/el/dist values
- **Preset buttons** along top of editor (Stereo, Quad, 5.1, 7.1, etc.)
- **Save/Load** custom layout buttons
- **Import/Export** layout files (.json)
- **Styled in botanical aesthetic** (earth tones, Garamond labels)

### Parameter Organization
Three grouped sections below the visualizer:

**Motion Section (8 params):**
- Path (dropdown), Speed (knob), Width (knob), Depth (knob)
- Tilt (knob), Phase (knob), Elevation Enable (toggle), Tempo Sync (dropdown)

**Spatial Section (5 params):**
- Speaker Layout (dropdown), Distance (knob), Air Absorption (knob)
- Attenuation Curve (dropdown), Center Diverge (knob)

**Source Section (3 params):**
- Source Mode (toggle: Mono / L+R Split), L/R Offset (knob), Mix (knob)

### Status Indicators
- **Downmix warning:** Subtle badge near speaker layout selector showing "Layout: 7.1 -> DAW: Stereo" when active
- **Earth-tone styling** — not alarming, unobtrusive
- Only visible when downmix is actually active

## Constraints Identified

- **17 parameters in 800x600** — dense layout requires careful spacing; knobs at 55px (compact tier per aesthetic spec)
- **Canvas animation + WebView** — must maintain 60fps; use requestAnimationFrame, avoid heavy DOM manipulation during animation
- **Motion state transfer** — need atomic snapshot from audio thread to UI; `std::atomic<MotionSnapshot>` or native function relay
- **Speaker editor drag interaction** — canvas mouse events need coordinate mapping to azimuth/elevation
- **File I/O for speaker layouts** — needs JUCE FileChooser integration via native function relay (WebView can't access filesystem directly)
- **Cross-platform WebView** — use `getResourceProviderRoot()` in C++ and `getBackendResourceAddress()` in JS; Windows needs `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (already in CMakeLists.txt)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Window size | 800x600 | Standard plugin size, fits most DAW layouts |
| Layout | Central visualizer + grouped controls below | Clear hierarchy, visualizer as focal point |
| Aesthetic | Ouaricon Botanical/Naturalist | Brand consistency across all plugins |
| Botanical image | Shell (ocean) | Spiral form echoes orbital motion, nautilus geometry |
| Visualizer style | Earth-tone integrated | Maintains aesthetic consistency, no neon/contrast |
| Speaker editor | Toggle panel (same space as visualizer) | Space-efficient, clean transition |
| Parameter groups | Motion / Spatial / Source sections | Logical grouping by function |
| Knob size | 55px (compact for 17 params) | Fits density while maintaining usability |
| Downmix indicator | Subtle badge near layout selector | Informative without being alarming |
| Animation approach | HTML5 Canvas + requestAnimationFrame | Standard, performant, well-supported |
| Motion state relay | Native function relay (C++ -> JS) | Thread-safe, low latency |
| File I/O | Native function relay for FileChooser | WebView can't access filesystem |

## Phased Implementation Plan (from ROADMAP.md)

### Phase 3.1: Basic WebView UI + Parameter Controls
- WebView setup with resource provider
- HTML/CSS layout with all 17 parameter controls
- JS parameter binding (WebSliderRelay, WebSliderParameterAttachment)
- Botanical aesthetic applied (knobs, colors, typography, shell overlay)
- No visualizer yet — placeholder area

### Phase 3.2: Orbital Visualizer (Animated)
- Canvas-based animated visualizer in the central area
- Motion state snapshot relay (C++ -> JS)
- Source dot, path trails, speaker position icons
- 60fps animation loop
- L+R split dual-dot display

### Phase 3.3: Speaker Layout Editor + File I/O
- Toggle between visualizer and speaker editor
- Drag-to-reposition, click-to-add, right-click-to-remove
- Preset layout buttons
- Save/load/import/export speaker layouts via native relay
- Downmix warning badge

## Open Questions

- Elevation range display in visualizer (show 3D perspective when elevation enabled, or keep 2D always?) — default to 2D, revisit in Phase 3.2
- Custom path drawing (deferred to v1.1 per ROADMAP) — not in scope for Stage 3

## Next Phase

Ready for: **research** phase
