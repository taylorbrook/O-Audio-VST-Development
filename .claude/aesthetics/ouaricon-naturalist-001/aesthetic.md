# Ouaricon Audio Naturalist

> **Vibe:** Classical naturalist aesthetic with vintage botanical illustrations, warm earth tones, and elegant typography reminiscent of 18th-19th century scientific illustration
>
> **Source:** Created from OuariconTremolo design as official Ouaricon Audio brand identity
>
> **Best For:** All Ouaricon Audio plugins - this is the signature brand aesthetic

---

## Brand Philosophy

The Ouaricon Audio Naturalist aesthetic evokes the golden age of natural history illustration - the careful observation, elegant documentation, and tactile beauty of vintage scientific manuscripts. Each plugin becomes a page from a naturalist's field guide, combining technical precision with organic warmth. The design language celebrates the intersection of art and science, creating interfaces that feel both sophisticated and approachable.

**Core Principles:**
- Classical elegance over modern minimalism
- Organic forms integrated with technical controls
- Warm, tactile materiality (aged paper, botanical specimens)
- One unique botanical illustration per plugin
- Sophisticated restraint - decorative but never frivolous

---

## Visual Identity

This aesthetic transforms each plugin into a specimen page from a vintage naturalist's collection. Warm paper-textured backgrounds suggest aged parchment, while carefully selected botanical illustrations (with transparent backgrounds) provide unique character to each plugin. Controls are designed as naturalist elements - knobs styled as cross-sectioned seeds or botanical diagrams, creating cohesive thematic unity.

The overall feeling is:
- **Warm & inviting** - earth tones, paper textures, soft shadows
- **Classically sophisticated** - serif typography, wide letter-spacing, elegant proportions
- **Scientifically curious** - detailed illustrations, careful labeling, orderly layout
- **Tactilely real** - subtle textures, dimensional shadows, physical presence

---

## Color System

### Primary Palette

**Background Colors:**

- Primary background: Aged paper tone (#F5E6D3 or #EBD9C7) - warm cream suggesting vintage parchment
- Alternative background: Lighter cream (#FAF0E6) for subtler contrast
- Accent surfaces: Slightly darker tan (#D4C4B0) for subtle paneling or sections
- Border/frame colors: Medium brown (#8B7355) for definition and structure

**Earth Tone Accents:**

- Muted green: #8BA870 (moss/sage) - for active states, botanical references
- Deeper green: #6B8E4E - for hover states, emphasis
- Warm brown: #8B7355 (walnut/oak) - for borders, text, structural elements
- Dark brown: #5C4033 - for deep shadows, strong borders, emphasis

**Text Colors:**

- Primary text: Dark brown (#3C2F2F) - warm, readable, elegant
- Secondary text: Medium brown (#5C4033) - labels, less prominent information
- Subtle text: Lighter brown with reduced opacity - tertiary information

**Special Accents:**

- Botanical green: #6B8E4E to #8BA870 - for toggles, active states, botanical elements
- Fleuron decorations: Use sparingly in dark brown (#5C4033) at low opacity

### Control Colors

**Knobs (Botanical Seed Cross-Section):**

- Outer ring: Aged paper tone (#C9A27B)
- Ring detail: Walnut brown (#8B7355)
- Seed segments: Alternating cream tones (#F5DEB3, #E8D5B7)
- Segment dividers: Dark brown lines (#8B7355) - 1deg wide creating radial pattern
- Inner core: Lightest cream (#FFF8DC) - 20% radius circle at center
- Border: 2px solid walnut (#8B7355)

**Sliders (if needed):**

- Track: Inset aged paper with subtle border
- Thumb: Circular, matching knob aesthetic (seed cross-section or simplified disc)
- No fill color - position indicates value

**Buttons/Toggles:**

- Default: Light green background (rgba(139, 168, 112, 0.3)) with green border (#3C5C1A)
- Active: Deeper green (rgba(107, 142, 35, 0.6)) with darker border (#2C3E10)
- Hover: Increased opacity, subtle lift (translateY(-1px))
- Decorative fleuron (❦) in corner at low opacity

### Philosophy

Warm, low-saturation earth palette evokes vintage paper and botanical specimens. All colors suggest natural materials - parchment, wood, moss, aged ink. High enough contrast for readability while maintaining soft, organic warmth. Green accents reference botanical subject matter without overwhelming. Overall palette creates cohesive naturalist aesthetic - like opening a treasured field guide.

---

## Typography

**Font Families:**

- Primary: 'Garamond', 'Times New Roman', serif
  - Classical elegance, historical authenticity
  - Excellent readability with refined character
  - Suggests scientific manuscripts and vintage publications
- Fallback chain ensures graceful degradation on all systems

**Font Sizing:**

- Plugin title: Large (22-26px), wide letter-spacing (2-3px) for elegant presence
- Section labels: Medium (12-14px), uppercase, moderate spacing (1px)
- Parameter labels: Small (9-11px), uppercase, wide letter-spacing (0.5-1px)
- Value displays: Small to medium (10-12px), regular case for numbers

**Font Styling:**

- Title weight: Normal (400) or Light (300) - elegant without aggression
- Label weight: Normal (400) or Medium (500) - clear but not bold
- Letter spacing: Wide for uppercase labels (0.5-2px) - creates refined technical feel
- Text transform: UPPERCASE for labels, preserving classical labeling tradition
- Text shadow: Subtle light shadow (1px 1px 2px rgba(255,255,255,0.5)) creates embossed effect on dark text

**Philosophy:**

Classical serif typography honors the tradition of scientific illustration and vintage publications. Wide letter-spacing on labels creates sophisticated, deliberate aesthetic. Light font weights prevent heaviness. Mix of uppercase labels and regular case values maintains hierarchy and readability. Overall typographic approach is refined, scholarly, and timeless.

---

## Botanical Imagery Integration

### Image Selection Guidelines

Each plugin receives ONE unique botanical illustration positioned as decorative overlay. Image selection should consider:

**By Plugin Type:**

- **Effects (reverb, delay, modulation)**: Flora, insects, birds - organic, flowing, ethereal
- **Dynamics (compression, limiting)**: Anatomy, skeletons - structure, control, force
- **Saturation/distortion**: Fauna, predators - raw energy, transformation
- **Filters**: Ocean specimens, shells - shaping, filtering natural processes
- **Utility (gain, pan)**: Fruit, seeds - fundamental, nourishing, essential
- **Synthesizers**: Your choice - insects for buzzing synths, birds for airy pads, etc.

**By Plugin Character:**

- **Aggressive/intense**: Skeletons, predatory fauna, striking anatomy
- **Gentle/musical**: Flowers, butterflies, birds
- **Warm/vintage**: Fruit, seeds, warm-toned botanical illustrations
- **Precise/technical**: Geometric flowers, anatomical diagrams, insects with clear structure
- **Experimental**: Unusual specimens (skeleton praying, human anatomy)

**Available Categories:**

- flora (3 images): flowers, plants - elegant, botanical
- fauna (3 images): grouse, snakes - animals, dynamic
- ocean (1 image): shell - structural, spiral forms
- insects (3 images): butterflies, bugs - delicate, geometric
- birds (1 image): birds - light, airy, free
- fruit (1 image): pomegranate - seeds, abundance, warmth
- anatomy (1 image): human - structure, precision
- skeletons (1 image): skeleton praying - contemplative, structural

### Image Placement

**Technical Requirements:**

- Source folder: `Ouaricon Audio Images/[category]/[filename].png`
- File format: PNG with transparent background
- Copy to plugin: `plugins/[PluginName]/Source/ui/public/img/[filename].png`
- Reference in HTML: `<img src="img/[filename].png" class="botanical-overlay">`

**Visual Placement:**

- Position: Absolute positioning, typically right side of interface
- Vertical alignment: Centered (top: 50%, transform: translateY(-50%))
- Horizontal offset: Right: -20px to -40px (allows image to bleed off edge)
- Size: Height 60-75% of plugin height (adjust per image to avoid overwhelming controls)
- Opacity: 0.3 to 0.4 - visible but subtle, never competing with controls
- Z-index: Above background, below controls
- Pointer events: none (click-through to controls beneath)

**CSS Pattern:**

```css
.botanical-overlay {
    position: absolute;
    right: -20px;  /* Adjust per image */
    top: 50%;
    transform: translateY(-50%);
    height: 71.25%;  /* Adjust per image - typically 60-75% */
    opacity: 0.35;  /* Adjust per image - typically 0.3-0.4 */
    pointer-events: none;
}
```

**Design Balance:**

- Image should enhance, not dominate
- Leave left 60-70% of interface clear for controls
- Subtle opacity ensures controls remain primary focus
- Right-side placement creates asymmetric visual interest
- Slight bleed off edge suggests larger specimen extending beyond frame

---

## Controls

### Knob Style (Botanical Seed Cross-Section)

**Visual Design:**

- Shape: Perfect circle (border-radius: 50%)
- Size: Medium (55-65px diameter) - prominent but not overwhelming
- Design motif: Stylized seed cross-section with radial segments
- Border: 2px solid walnut brown (#8B7355) for definition

**Surface Pattern:**

10-segment radial pattern created with conic-gradient:
- Each segment 36deg wide (360deg ÷ 10)
- Alternating cream tones: #F5DEB3 and #E8D5B7
- Dark brown dividers (#8B7355) at 1deg width between segments
- Creates botanical diagram aesthetic

**Layered Construction:**

```css
background:
    /* Outer ring - aged paper */
    radial-gradient(circle, transparent 88%, #C9A27B 88%,
                    #C9A27B 92%, #8B7355 92%, #8B7355 94%, transparent 94%),
    /* 10-segment radial pattern */
    conic-gradient(from 0deg,
        #F5DEB3 0deg, #F5DEB3 18deg,
        #8B7355 18deg, #8B7355 19deg,  /* 1deg divider */
        #E8D5B7 19deg, #E8D5B7 36deg,
        /* Pattern repeats 10 times... */
    ),
    /* Inner center core */
    radial-gradient(circle, #FFF8DC 0%, #FFF8DC 20%, transparent 20%);
```

**Depth & Texture:**

- Box shadow: Inset shadows create engraved depth
  - `inset 1px 1px 3px rgba(0,0,0,0.3)` - top-left shadow
  - `inset -1px -1px 2px rgba(255,248,220,0.5)` - bottom-right highlight
  - `2px 2px 6px rgba(0,0,0,0.25)` - outer drop shadow
- Optional overlay: Cross-hatching pattern via pseudo-element for engraved texture

**Indicator System:**

- Style: Subtle botanical stem/line extending from center to edge
- Color: Darker brown or contrasting green
- Width: 2-3px
- Length: Extends to ~90% radius
- Rotation: Transforms with knob value
- Can use simple div or SVG path

**Interaction Feel:**

- Hover: Subtle scale increase (1.05x) suggesting touchability
- Active: Slight scale decrease (0.98x) for press feedback
- Transition: Quick (0.1s) for immediate response
- Rotation: Smooth continuous rotation with value changes

### Slider Style

Not prominently used in naturalist aesthetic, but when needed:

**Layout:**

- Orientation: Vertical preferred for visual harmony
- Track: Aged paper inset (#E8D5B7) with brown border (#8B7355)
- Dimensions: Thin track (6-8px), medium thumb (18-24px diameter)

**Thumb Design:**

- Shape: Circle, simplified seed cross-section, or smooth disc
- Color: Matching knob palette (cream gradients, brown border)
- Shadow: Subtle drop shadow for elevation

**Visual Treatment:**

- Track: Inset appearance with subtle inner shadow
- No fill color - clean minimalist approach
- Optional tick marks in brown for value reference

### Button/Toggle Style

**Shape & Dimensions:**

- Shape: Rounded rectangle (border-radius: 4-6px) for subtle softness
- Size: Compact (60-80px width, 30-40px height) integrating into layout
- Padding: 8-12px for comfortable hit target

**Visual Design:**

- Default state: Light green tint (rgba(139, 168, 112, 0.3))
- Border: 2px solid darker green (#3C5C1A)
- Active state: Deeper green fill (rgba(107, 142, 35, 0.6))
- Border active: Darker (#2C3E10)

**Typography:**

- Font: Garamond, small size (9-11px)
- Transform: UPPERCASE for labels
- Letter-spacing: 0.5-1px for refinement
- Color: Dark green (#2C3E10)

**Decorative Elements:**

- Optional fleuron (❦) in corner at low opacity (0.3)
- Subtle dotted pattern border (can be created with repeating-linear-gradient)
- Shadow: `2px 2px 5px rgba(0,0,0,0.2)` for slight elevation

**Interaction:**

- Hover: Increased background opacity, subtle lift (translateY(-1px))
- Shadow on hover: `2px 3px 6px rgba(0,0,0,0.3)` - enhanced depth
- Transition: 0.2s for smooth state changes

---

## Spacing & Layout Philosophy

**Overall Density:**

Generous and breathable. Classical manuscripts avoided crowding - each element has space to be appreciated. Create sense of unhurried elegance and premium quality. Controls should feel deliberately placed, not crammed.

**Control Spacing:**

- Between controls: Moderate to generous (25-40px gaps) - allows breathing room
- Vertical rhythm: Clear visual sections with consistent spacing
- Grouping: Related controls closer together (15-20px), groups separated by wider gaps (40-60px)

**Padding & Margins:**

- Edge margins: Comfortable (20-30px from container edges)
- Content padding: 20-30px internal padding for sections
- Label-to-control gap: 8-12px - clearly associated but not touching
- Section separation: 15-25px vertical gaps between major sections

**Layout Patterns:**

**For 1-4 parameters:**
- Single horizontal row
- Large controls (65px knobs)
- Generous spacing (40px gaps)
- Botanical image on right, controls on left

**For 5-8 parameters:**
- Two sections or single row with grouping
- Medium controls (60px knobs)
- Moderate spacing (30px gaps)
- May use left panel + right panel layout

**For 9+ parameters:**
- Grid layout (2 rows or 2 columns)
- Compact controls (55px knobs)
- Tighter spacing (25px gaps) while maintaining elegance
- Consider grouping by function

**Layout Flexibility:**

System scales gracefully with parameter count while maintaining naturalist aesthetic. For more parameters, prioritize clear grouping and visual hierarchy over cramming. If plugin has many parameters, consider multi-section layout preserving generous feel within each section.

---

## Surface Treatment

### Textures

**Background:**

- Primary texture: Aged paper grain (subtle noise or paper texture image)
- Alternative: Vertical or horizontal paper fibers (very subtle, ~0.02 opacity)
- Implementation: Can use CSS filter, SVG noise, or subtle background-image
- Intensity: Barely visible - adds tactility without distraction

**Control Surfaces:**

- Knob texture: Engraved cross-hatching via pseudo-element (optional)
  - Diagonal lines at 45deg and -45deg
  - Very low opacity (0.1-0.15)
  - Creates subtle etched quality
- Button texture: Smooth with optional dotted border pattern

### Depth & Dimensionality

**Shadow Strategy:**

- Shadow presence: Moderate - creates gentle depth without drama
- Shadow color: Black with low opacity (0.15-0.3) for soft shadows
- Shadow blur: Soft (4-8px blur) for natural, organic feel
- Typical values: `2px 2px 6px rgba(0,0,0,0.25)` for controls

**Elevation System:**

- Background layer: Paper texture, botanical overlay
- Surface layer: Main container with border
- Controls layer: Knobs, sliders, buttons with subtle shadows
- Decorative layer: Optional fleurons, ornamental details

**How Elevation is Shown:**

- Primarily through subtle drop shadows
- Inset shadows for recessed elements (slider tracks)
- Slight scale changes on interaction suggest physical movement

**Borders:**

- Border presence: Selective and purposeful
- Main container: Medium border (2-4px) in brown (#5C4033 or #8B7355)
- Controls: Thin borders (2px) for definition
- Border style: Simple solid borders, occasionally decorative (dotted patterns)
- Border colors: Always warm browns, never harsh black

### Special Background Treatment

**Container Border (Optional):**

Can add decorative border to main plugin container:
```css
border: 3px solid #5C4033;
box-shadow: 0 10px 40px rgba(0,0,0,0.5);
```

Creates sense of framed specimen, like matted botanical print.

---

## Details & Embellishments

**Decorative Elements:**

- Fleurons (❦, ✤, ✦): Use sparingly in corners, section dividers
- Opacity: Low (0.2-0.4) - subtle ornamentation
- Color: Dark brown (#5C4033 or #3C2F2F)
- Placement: Top corners, button corners, section dividers

**Botanical Overlay:**

- Primary decorative feature - one per plugin
- Transparent PNG with vintage illustration aesthetic
- Positioned right side, subtle opacity (0.3-0.4)
- Never competes with controls - purely enhancing

**Active State Feedback:**

- Subtle transformations: scale, translateY, shadow intensity
- No color shifts - maintains consistent palette
- Quick transitions (0.1-0.2s) feel immediate and responsive

**Hover States:**

- Knobs: Scale increase (1.05x)
- Buttons: Background opacity increase, slight lift (translateY(-1px))
- Shadow enhancement: Slightly larger or darker shadows
- No dramatic effects - refined and subtle

**Focus Indicators:**

- Accessibility consideration: Subtle brown outline or enhanced shadow
- Could use dotted border in green (#6B8E4E)
- Maintains aesthetic while ensuring keyboard navigation clarity

**Typography Embellishments:**

- Text shadow on dark text over light background: `1px 1px 2px rgba(255,255,255,0.5)` creates subtle emboss
- Wide letter-spacing on labels (0.5-2px) - refined, deliberate
- Optional small caps for section headers

---

## Technical Patterns

**CSS Patterns:**

- Border radius: Moderate for circles (50%), subtle for rectangles (4-6px)
- Transition speed: Quick for direct manipulation (0.1s), moderate for states (0.2s)
- Easing: ease or ease-out - natural, not robotic

**Layout Techniques:**

- Flexbox for main layout - excellent for responsive parameter arrangements
- Absolute positioning for botanical overlay and decorative elements
- Grid optional for complex parameter layouts (9+ params)
- Percentage-based sizing where appropriate, pixel-based for precision

**Responsive Strategy:**

- Fixed plugin dimensions (typical 600x400, adjustable based on parameter count)
- Botanical overlay scales proportionally with container height
- Controls maintain minimum hit targets (44x44px) for usability

**Performance Considerations:**

- Conic gradients for knobs are moderately expensive but acceptable for plugin UI
- Keep botanical overlay images optimized (PNG compression)
- Limit pseudo-elements and complex shadows for better performance
- CSS transforms preferred over position changes for animations

---

## Interaction Feel

**Responsiveness:**

Quick and immediate. Controls respond fluidly to input with minimal latency. Transitions fast enough to feel direct (100-200ms) while providing clear visual feedback.

**Feedback:**

Visual feedback is elegant and subtle:
- Scale changes on interaction (hover grow, active shrink)
- Shadow intensity variations
- Smooth rotations and state transitions
- No flashy or jarring effects

**Tactility:**

Sophisticated physical feel without heavy skeuomorphism. Subtle shadows and textures suggest real materials (paper, wood, botanical specimens) while maintaining clean, usable interface. Interactions feel like turning pages in a treasured field guide.

**Overall UX Personality:**

Classically refined, scholarly, and inviting. Sophisticated without pretension. Warm and approachable despite technical function. Evokes sense of discovery, observation, and careful documentation. Users feel like naturalists exploring sound - combining artistry with scientific precision.

---

## Best Suited For

**This is the signature Ouaricon Audio aesthetic - ALL plugins should use this system.**

**Particularly Excellent For:**

- Vintage/warm character effects
- Organic modulation (tremolo, chorus, phaser)
- Reverbs and delays with natural character
- Synthesizers with acoustic or experimental character
- Utility plugins that benefit from approachable elegance

**Design Contexts:**

- Building cohesive Ouaricon Audio brand identity
- Products emphasizing musical warmth over clinical precision
- Plugins targeting creative/artistic users
- Projects celebrating intersection of art, science, and sound

---

## Application Guidelines

### When Applying to New Plugin

**1. Select Botanical Illustration**

Choose image from `Ouaricon Audio Images/[category]/` based on:
- Plugin type and character (see Botanical Imagery Integration)
- Visual balance with parameter count (fewer params = larger image possible)
- Variety across product line (avoid repeating same image too frequently)

**2. Copy Image to Plugin**

```bash
cp "Ouaricon Audio Images/[category]/[filename].png" \
   "plugins/[PluginName]/Source/ui/public/img/[filename].png"
```

**3. Parameter Count Adaptation**

- **1-3 parameters:** Large knobs (65px), generous spacing (40px), single row
- **4-6 parameters:** Medium knobs (60px), moderate spacing (30px), single row or two sections
- **7-9 parameters:** Compact knobs (55px), organized spacing (25px), grid or multi-section
- **10+ parameters:** Consider grouping into sections, maintain elegance with tighter but organized layout

**4. Control Type Mapping**

- Float parameters → Botanical seed cross-section knobs (primary control)
- Boolean parameters → Green toggle buttons with fleuron
- Choice parameters (2-3 options) → Toggle buttons in row
- Choice parameters (4+ options) → Could use dropdown styled to match aesthetic

**5. Color Customization Points**

Easy to adjust while maintaining identity:
- Background paper tone (adjust warmth/lightness)
- Green accent saturation (more or less vibrant)
- Brown border darkness
- Botanical overlay opacity

Core identity elements (preserve these):
- Warm earth-tone palette
- Serif typography (Garamond)
- Botanical seed cross-section knob design
- Wide letter-spacing on labels
- One unique botanical illustration per plugin

---

## Example Color Codes

```css
/* Backgrounds */
--bg-paper-light: #FAF0E6;        /* Lightest paper tone */
--bg-paper: #F5E6D3;              /* Primary aged paper */
--bg-paper-mid: #EBD9C7;          /* Medium paper tone */
--bg-accent: #D4C4B0;             /* Darker tan for panels */

/* Browns */
--brown-border: #8B7355;          /* Walnut - borders, text */
--brown-frame: #5C4033;           /* Oak - strong borders */
--brown-text: #3C2F2F;            /* Primary text */

/* Greens (Botanical Accents) */
--green-light: #8BA870;           /* Moss/sage - active states */
--green-mid: #6B8E4E;             /* Deeper green - hover, emphasis */
--green-dark: #3C5C1A;            /* Dark green - borders */
--green-darkest: #2C3E10;         /* Darkest green - text, active borders */

/* Knob Colors (Seed Cross-Section) */
--knob-ring: #C9A27B;             /* Outer ring */
--knob-segment-1: #F5DEB3;        /* Light cream segments */
--knob-segment-2: #E8D5B7;        /* Medium cream segments */
--knob-divider: #8B7355;          /* Brown segment dividers */
--knob-core: #FFF8DC;             /* Center core */
--knob-border: #8B7355;           /* Border */

/* Buttons */
--btn-default: rgba(139, 168, 112, 0.3);  /* Light green tint */
--btn-active: rgba(107, 142, 35, 0.6);    /* Deeper green active */
--btn-border: #3C5C1A;            /* Default border */
--btn-border-active: #2C3E10;     /* Active border */

/* Effects */
--shadow-light: rgba(0, 0, 0, 0.15);      /* Light shadows */
--shadow-medium: rgba(0, 0, 0, 0.25);     /* Medium shadows */
--shadow-heavy: rgba(0, 0, 0, 0.5);       /* Heavy shadows (container) */
--text-emboss: rgba(255, 255, 255, 0.5);  /* Text emboss highlight */
--fleuron-color: rgba(60, 47, 47, 0.3);   /* Decorative symbols */
```

---

## Implementation Checklist

When applying Ouaricon Naturalist aesthetic to a new plugin:

- [ ] Select appropriate botanical illustration from image library
- [ ] Copy image to plugin ui/public/img/ folder
- [ ] Set up color palette using CSS variables
- [ ] Apply Garamond typography with proper hierarchy
- [ ] Style knobs with 10-segment seed cross-section pattern
- [ ] Style buttons/toggles with green botanical theme
- [ ] Position botanical overlay (right side, 0.35 opacity)
- [ ] Add aged paper background texture
- [ ] Apply spacing system (generous, breathable)
- [ ] Implement shadows and depth (subtle, organic)
- [ ] Add interaction states (hover, active)
- [ ] Include decorative fleurons where appropriate
- [ ] Test visual balance with botanical overlay
- [ ] Verify all controls readable and accessible
- [ ] Validate WebView constraints (no viewport units, etc.)
- [ ] Test in Debug and Release builds

---

## Integration with Plugin Freedom System

**Automatic Application:**

This aesthetic can be set as default for all Ouaricon Audio plugins:
1. ui-mockup skill checks for `.claude/aesthetics/ouaricon-naturalist-001/`
2. Automatically applies when creating new Ouaricon Audio plugins
3. Offers as "official brand aesthetic" in decision menus

**Manual Selection:**

Always available in aesthetic template library:
- Run ui-mockup skill
- Choose "Start from aesthetic template"
- Select "Ouaricon Audio Naturalist"
- System adapts to parameter count and generates appropriate layout

**Customization:**

While maintaining brand consistency, individual plugins can:
- Adjust opacity of botanical overlay (0.25-0.45)
- Fine-tune spacing for specific parameter counts
- Modify knob size within reasonable range (55-70px)
- Adjust background paper tone warmth
- Select most thematically appropriate botanical image

---

## Brand Consistency Notes

**This aesthetic IS the Ouaricon Audio brand identity.**

All plugins under Ouaricon Audio label should use this system to:
- Build recognizable visual brand
- Create cohesive product family
- Establish market differentiation
- Provide consistent user experience

Key to brand consistency:
- Always use serif typography (Garamond family)
- Always include one botanical illustration
- Always use warm earth-tone palette
- Always maintain seed cross-section knob design
- Always preserve elegant spacing and classical layout

Allowed variations:
- Different botanical images per plugin
- Spacing adjustments for parameter count
- Minor color saturation tweaks
- Layout adaptations while preserving core aesthetic

---

**Version:** 1.0
**Created:** 2026-01-08
**Based on:** OuariconTremolo v1.2.1
**Status:** Official Ouaricon Audio Brand Aesthetic
