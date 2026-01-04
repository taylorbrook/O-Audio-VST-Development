# Vintage Bakelite

> **Vibe:** Warm, tactile vintage hardware with molded plastic character
>
> **Source:** Created from AutoClip mockup v5
>
> **Best For:** Aggressive processing tools (clippers, compressors, saturators), drum processors, character effects where physical control feel enhances workflow

---

## Visual Identity

This aesthetic evokes 1960s-70s Bakelite hardware—molded brown plastic with warm gradients, subtle surface texture, and physical depth. The design emphasizes tactile realism through rotating textures on knobs (mimicking actual surface rotation) and stitched border details that suggest molded construction. It balances vintage warmth with clean readability, avoiding excessive skeu morphism while maintaining nostalgic hardware character.

---

## Color System

### Primary Palette

**Background Colors:**
- Main background: Dark brown radial gradient from warm center to cooler edges (e.g., `radial-gradient(circle at 50% 0%, #2a1808, #1a0800)`)
- Surface/panel background: Same as main (unified surface)
- Elevated surfaces: Slightly lighter browns for controls (#3a2010 to #6a4a2a range)

**Accent Colors:**
- Primary accent: Cream/tan indicator color for pointers and highlights (#d4b494)
- Secondary accent: Muted tan for subdued elements (#a88870)
- Hover/active state: Golden brown highlight (#c49564) on borders

**Text Colors:**
- Primary text: Muted tan/brown (#a88870) for titles
- Secondary/muted text: Warmer muted brown (#986a4a) for labels
- Labels: Same as secondary (#986a4a)

### Control Colors

**Knobs/Rotary Controls:**
- Base color: Radial gradient from lighter brown at highlight point to darker at edges (#6a4a2a → #5a3a1a → #3a2010 → #2a1008)
- Pointer/indicator: Cream-to-tan gradient vertical bar (#d4b494 → #a88870)
- Active state: Border changes to golden brown (#c49564) on hover

**Sliders:**
- Track color: Dark brown gradient matching inactive toggle (#3a2010 → #2a1008)
- Thumb color: Medium brown radial gradient (#987860 → #786040)
- Fill color: Lighter brown gradient for active sections (#5a4030 → #4a3020)

**Buttons/Toggles:**
- Default state: Dark brown gradient (#3a2010 → #2a1008) with subtle inset shadows
- Active/on state: Medium brown gradient (#5a4030 → #4a3020) with same shadow structure
- Hover state: Golden brown border (#c49564)

### Philosophy

Warm, monochromatic brown palette with subtle variations in tone and saturation. High contrast is achieved through lightness differences (dark backgrounds, lighter controls) rather than hue shifts. The aesthetic maintains a cohesive "molded from single material" feel while using gradients and shadows to create depth and tactility.

---

## Typography

**Font Families:**
- Headings: Bold sans-serif (system-ui fallback stack)
- Body text: Same sans-serif family (unified type system)
- Values/numbers: Same sans-serif (no monospace, maintains vintage simplicity)

**Font Sizing:**
- Plugin title: Large (e.g., 20px), prominent but not overwhelming
- Section labels: Not used in this minimal aesthetic
- Parameter labels: Small (e.g., 11px), subtle presence
- Parameter values: Not displayed in mockup (pure visual interface)

**Font Styling:**
- Title weight: Bold (700) for strong presence against dark background
- Label weight: Normal (400) for subtlety
- Letter spacing: Wide (0.25em for titles, 0.15em for labels) creating vintage typographic feel
- Text transform: Uppercase throughout (title and labels) for industrial character

**Philosophy:**
Typography is minimal and utilitarian, echoing mid-century industrial design. Wide letter-spacing and all-caps treatment create a technical, professional feel without ornament. The unified sans-serif system keeps focus on physical controls rather than typographic hierarchy.

---

## Controls

### Knob Style

**Visual Design:**
- Shape: Perfect circle
- Size: Large (190px diameter) for dominant parameter control
- Indicator style: Vertical bar pointer (10×60px) positioned at top
- Border treatment: Prominent (3px solid) with color-changing hover state

**Surface Treatment:**
- Base appearance: Radial gradient mimicking lit plastic surface (highlight at upper-left, shadow at lower-right)
- Depth: Moderate 3D with inset shadows (top highlight, bottom shadow) suggesting molded depth
- Tick marks: None (clean surface)
- Center indicator: No (pointer bar serves as indicator)

**Interaction Feel:**
Physical rotation model where the entire knob body (including surface texture) rotates around center axis. Texture noise pattern rotates with the surface, while shadow container remains fixed. This creates realistic tactile feedback matching actual hardware knob interaction. Rotation range ±135° (270° total travel), cursor changes to ns-resize for vertical drag interaction.

### Slider Style

**Layout:**
- Orientation preference: Horizontal (based on toggle implementation)
- Dimensions: Medium thickness (38px height for toggle track suggests similar for sliders)
- Track design: Filled pill shape with gradient background

**Thumb Design:**
- Shape: Circle
- Size: Medium (28px diameter as seen in toggle)
- Style: Radial gradient with subtle shadow, appears slightly raised

**Visual Treatment:**
- Track appearance: Dark brown gradient with subtle Perlin noise texture, inset shadows create recessed feel
- Fill behavior: Background color change from inactive to active gradient
- Scale marks: None (maintains clean vintage look)

### Button Style

**Shape & Size:**
- Shape: Pill/rounded rectangle (border-radius 50% of height)
- Padding: Comfortable (based on 80×38px toggle with 28px thumb, ~5px padding)
- Aspect ratio: Wide (approximately 2:1 ratio)

**Visual Design:**
- Default state: Dark brown gradient with inset shadows (appears recessed)
- Active state: Lighter brown gradient (appears slightly less recessed or lit)
- Border: Subtle (2px solid matching dark palette), changes to golden on hover

**Typography:**
- Label style: Normal weight, all-caps, wide letter-spacing (0.15em)
- Icon treatment: None (text-only labels below controls)

---

## Spacing & Layout Philosophy

**Overall Density:**
Generous and focused. The design uses ample negative space to emphasize the few controls present, creating a zen-like simplicity. This spaciousness reinforces the "one primary control" approach suitable for specialized processing tools.

**Control Spacing:**
- Between controls: Generous (60px gap between knob and toggle) allows breathing room
- Vertical rhythm: Clear vertical flow (title → large gap → knob → medium gap → toggle)
- Grouping strategy: No formal grouping needed due to minimal parameter count; visual weight creates implicit hierarchy

**Padding & Margins:**
- Edge margins: Moderate (30px top/bottom, 20px left/right from 300×500px container)
- Section padding: Not applicable (single section design)
- Label-to-control gap: Close (14px gap between toggle and label maintains association)

**Layout Flexibility:**
Spacing scales with parameter count. For 1-3 parameters, maintain generous spacing. For 4-6, reduce vertical gaps proportionally while keeping edge margins consistent. For 7+ parameters, consider grid layout with tighter gaps but preserve edge breathing room.

---

## Surface Treatment

### Textures

**Background:**
- Texture type: Subtle Perlin noise (SVG feTurbulence filter)
- Intensity: Subtle (opacity 0.17, barely visible but adds organic feel)
- Implementation: SVG filter with baseFrequency 2.3, 3 octaves, layered over gradient

**Control Surfaces:**
- Control texture: Same Perlin noise technique applied to knob and toggle surfaces
- Consistency: All controls use identical noise parameters, creating unified material feel

### Depth & Dimensionality

**Shadow Strategy:**
- Shadow presence: Medium (noticeable but not dramatic)
- Shadow color: Black with varying opacity (0.6-0.8 alpha)
- Shadow blur: Soft (10-20px blur for drop shadows)
- Typical shadow values: Knob drop `0 10px 20px rgba(0,0,0,0.7)`, inset highlights `inset 0 4px 4px rgba(90,58,26,0.3)`, inset shadows `inset 0 -4px 4px rgba(0,0,0,0.8)`

**Elevation System:**
- Layers: Subtle elevation (controls appear slightly raised from background via drop shadows)
- How elevation is shown: Combination of drop shadows (outset) and inset highlights/shadows (surface depth)

**Borders:**
- Border presence: Selective (controls have borders, background uses dashed stitching detail)
- Border style: Solid lines (2-3px) for controls, dashed (1px) for decorative stitching
- Border color approach: Dark brown matching shadow areas (#2a1408), golden hover state (#c49564)

---

## Details & Embellishments

**Special Features:**
Stitched inner border (8px inset from container edge, 1px dashed cream line at low opacity) creates molded Bakelite panel aesthetic. Perlin noise texture applied consistently across all surfaces adds organic, manufactured feel. Physical knob rotation model (texture rotates with surface) is signature interactive detail.

**Active State Feedback:**
Border color change to golden brown (#c49564) on hover/active. Toggle thumb slides horizontally with smooth 0.2s ease transition. Knob transforms rotation smoothly with 0.05s ease-out. Background gradient shifts on toggle (inactive → active state uses warmer brown).

**Hover States:**
Border color changes from dark brown to golden highlight (#c49564) with 0.15s ease transition. Cursor changes to ns-resize over knob (indicates vertical drag interaction). Toggle shows pointer cursor.

**Focus Indicators:**
Not explicitly defined in mockup (WebView context may not need keyboard navigation), but would follow border color pattern (golden outline).

**Decorative Elements:**
Stitched border (dashed line 8px inset) suggests molded panel seams. Text shadow on title (0 2px 4px black) adds subtle depth. No other ornamental details—aesthetic relies on material realism rather than decoration.

---

## Technical Patterns

**CSS Patterns:**
- Border radius: Subtle to prominent depending on element (4px for indicator, 19px for pill toggle, 50% for circles)
- Transition speed: Fast (0.05s for knob rotation, 0.15s for border color, 0.2s for toggle thumb slide)
- Easing: Ease-out for knob (snappy feedback), ease for color transitions (smooth), ease for toggle (smooth mechanical feel)

**Layout Techniques:**
- Preferred layout: Flexbox for vertical centering and flow
- Responsive strategy: Fixed sizes (300×500px container, 190px knob) maintains vintage hardware feel
- Alignment: Center-aligned vertical flow

**Performance Considerations:**
SVG filter noise texture is performance-intensive (rasterized per-frame during knob rotation). Consider pre-rendered texture image for production. Multiple box-shadow layers (3 on knob) may impact performance on older hardware. Smooth rotation requires `will-change: transform` hint for GPU acceleration.

---

## Interaction Feel

**Responsiveness:**
Immediate visual feedback with short transitions (50-200ms). Knob rotation tracks mouse precisely with minimal perceived lag. Toggle responds instantly to click with animated thumb slide providing mechanical satisfaction.

**Feedback:**
Strong visual feedback through border color changes (hover), rotation transforms (knob), and sliding animations (toggle). No explicit value displays—user learns parameter position through visual indicator position (knob angle, toggle state).

**Tactility:**
Feels like physical hardware. Knob rotation model mimics touching actual Bakelite surface (texture moves with your hand). Inset shadows create recessed, pressable feel for toggle. Drop shadows lift controls off background, suggesting physical depth.

**Overall UX Personality:**
Nostalgic, focused, tactile. Professional vintage studio gear aesthetic. Encourages mindful parameter adjustment through large, deliberate controls. Suited for aggressive, characterful processing where the interface aesthetic matches the sonic character (warm, analog, saturated).

---

## Best Suited For

**Plugin Types:**
- Hard clippers and soft clippers
- Drum processors (transient shapers, saturators)
- Vintage-style compressors
- Character effects (distortion, tape emulation, tube saturation)
- Simple utility effects with 1-4 parameters

**Design Contexts:**
- Personal projects with vintage/analog aesthetic goals
- Specialized processing tools where simplicity is a feature
- Drum bus processors where tactile control feel enhances workflow
- Lo-fi/vintage production toolkits

**Not Recommended For:**
- Surgical/transparent processors (use modern/clinical aesthetic instead)
- Complex multi-band or modular plugins (10+ parameters would feel cramped)
- Bright, colorful, playful effects (aesthetic is warm and serious)
- Plugins targeting EDM/modern electronic genres (may feel too retro)

---

## Application Guidelines

### When Applying to New Plugin

**Parameter Count Adaptation:**
- **1-3 parameters:** Use large knobs (150-190px), generous vertical spacing (60px gaps), portrait or square layout
- **4-6 parameters:** Use medium knobs (100-120px), moderate spacing (30-40px gaps), consider 2-column grid or horizontal row
- **7-9 parameters:** Use smaller knobs (70-90px), tighter spacing (20px gaps), 3×3 grid or sectioned layout
- **10+ parameters:** Not recommended for this aesthetic; consider tabs/pages or different aesthetic with denser feel

**Control Type Mapping:**
- Float parameters → Large rotary knobs for primary controls, smaller knobs or sliders for secondary
- Boolean parameters → Pill-shaped toggles with sliding thumb (horizontal orientation)
- Choice parameters → Consider vertical pill toggle stack or rotary selector styled as knob with discrete positions

**Prominent Parameter Handling:**
Size and position convey importance. Primary parameter (like threshold) gets largest knob (180-200px), centered placement, top third of layout. Output/mix parameters go bottom-right as medium knobs (100-120px). Dry/wet toggles use same pill toggle style as other booleans.

### Customization Points

**Easy to Adjust:**
- Color temperature (shift brown hue toward orange/red for warmer, toward gray for cooler)
- Knob size (scale proportionally while maintaining indicator ratio)
- Border hover color (golden works well, but could use cream or orange)
- Spacing gaps (compress or expand while maintaining rhythm)
- Title/label text (plug in different plugin names, adjust letter-spacing)

**Core Identity Elements:**
- Warm brown monochromatic palette (defines the "Bakelite" material)
- Perlin noise texture (creates manufactured plastic feel)
- Physical knob rotation model (texture rotates with body)
- Stitched border detail (signature molded panel aesthetic)
- Inset shadow structure (top highlight, bottom shadow pattern)
- Wide-spaced uppercase typography (vintage industrial character)

### Integration Notes

SVG filter noise requires WebView to support inline SVG and feTurbulence (works in modern browsers, JUCE WebView). For older WebView implementations, replace with pre-rendered PNG noise texture. Rotation transform on knob requires GPU acceleration for smooth 60fps—use `will-change: transform`. Fixed 300×500px size works for small UIs; scale proportionally for larger parameter counts (e.g., 400×600px for 6-8 parameters).

---

## Example Color Codes

```css
/* Backgrounds */
--bg-main: radial-gradient(circle at 50% 0%, #2a1808, #1a0800);
--bg-surface: #1a0800;

/* Accents */
--accent-primary: #d4b494;
--accent-hover: #c49564;

/* Text */
--text-primary: #a88870;
--text-secondary: #986a4a;

/* Controls */
--control-base: radial-gradient(circle at 30% 25%, #6a4a2a, #5a3a1a 40%, #3a2010 70%, #2a1008 100%);
--control-active: linear-gradient(to bottom, #5a4030, #4a3020);

/* Borders */
--border-default: #2a1408;
--border-outer: #4a3020;
--border-hover: #c49564;

/* Shadows */
--shadow-drop: 0 10px 20px rgba(0, 0, 0, 0.7);
--shadow-inset-top: inset 0 4px 4px rgba(90, 58, 26, 0.3);
--shadow-inset-bottom: inset 0 -4px 4px rgba(0, 0, 0, 0.8);

/* Stitching */
--stitch-color: rgba(212, 180, 148, 0.15);
```

---

## Implementation Checklist

When applying this aesthetic to a new plugin:

- [ ] Extract core color palette and define CSS variables
- [ ] Apply typography hierarchy (fonts, sizes, weights)
- [ ] Style each control type according to specifications
- [ ] Implement spacing system (gaps, padding, margins)
- [ ] Add surface treatments (textures, shadows, borders)
- [ ] Apply interaction states (hover, active, focus)
- [ ] Test with different parameter counts
- [ ] Verify visual consistency with source aesthetic
- [ ] Validate WebView constraints (no viewport units, etc.)
- [ ] Test in both Debug and Release builds
