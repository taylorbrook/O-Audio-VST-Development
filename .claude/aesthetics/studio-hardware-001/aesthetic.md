# Studio Hardware

> **Vibe:** Professional 19" rack unit with industrial precision and LED metering
>
> **Source:** Template-only aesthetic (no source plugin)
>
> **Best For:** Mixing and mastering processors (EQ, compressor, limiter, analyzer, utility plugins)

---

## Visual Identity

Authentic 19-inch rack hardware aesthetic bringing professional studio gear to plugin interfaces. The design prioritizes industrial precision with functional LED metering, brushed metal surfaces, and minimal decoration. Heavy knobs, calibrated controls, and classic VU-style meters evoke the tactile experience of real rack processors while maintaining the clarity needed for digital workflows.

---

## Color System

### Primary Palette

**Background Colors:**
- Main background: Pure dark rack metal (#1a1a1a) creating deep panel surface
- Surface/panel background: Slightly lighter metal (#1e1e1e) for subtle panel variation
- Elevated surfaces: Same as base (flat hierarchy, hardware-authentic)

**Accent Colors:**
- Primary accent: Bright yellow-gold (#ffc864) for labels and active indicators
- Secondary accent: Deeper gold (#e6b050) for highlights and borders
- Hover/active state: No hover state (hardware-authentic behavior)

**Text Colors:**
- Primary text: Bright yellow-gold (#ffc864) matching accent system
- Secondary/muted text: Dimmed gold (#e6b050) for secondary information
- Labels: Yellow-gold (#ffc864) uppercase with tight spacing

### Control Colors

**Knobs/Rotary Controls:**
- Base color: Dark metal (#1a1a1a) with horizontal brushing texture
- Pointer/indicator: Bright yellow-gold (#ffc864) with drop shadow and glow
- Active state: No change (hardware doesn't show hover feedback)

**Sliders:**
- Track color: Dark metal (#1a1a1a) with inset shadow
- Thumb color: Brushed metal with yellow-gold accent indicator
- Fill color: LED track following traffic light system (green/yellow/red)

**Buttons/Toggles:**
- Default state: Dark metal with subtle border, power LED off
- Active/on state: Power LED illuminated (traffic light colors), same surface appearance
- Hover state: None (hardware-authentic, no visual feedback on hover)

### Philosophy

Industrial dark palette with bright yellow-gold accents creating high contrast for professional clarity. Traffic light LED system (green #44ff44, yellow #ffff44, red #ff4444) provides immediate visual feedback for metering and status. Warm gold accents against cool dark metal evoke classic studio hardware while maintaining modern readability.

---

## Typography

**Font Families:**
- Headings: Impact or Arial Black (heavy, industrial impact)
- Body text: Arial Narrow (technical precision, rack-style labels)
- Values/numbers: Seven-segment LED style (digital readout aesthetic)

**Font Sizing:**
- Plugin title: Large and bold (24-32px) for rack panel identification
- Section labels: Medium (14-16px) for logical grouping
- Parameter labels: Small to medium (11-14px) uppercase with tight tracking
- Parameter values: Medium (14-18px) seven-segment style for digital readout

**Font Styling:**
- Title weight: Extra bold (900) for industrial impact
- Label weight: Normal to medium (400-500) for clarity
- Letter spacing: Tight (-0.02em to 0em) for compact rack aesthetic
- Text transform: Uppercase for all labels (technical/professional convention)

**Philosophy:**
Industrial typography emphasizing technical precision and hardware authenticity. Heavy title font conveys professional equipment identity, while narrow condensed labels maximize information density in tight rack-style spacing. Seven-segment numeric displays reinforce the LED metering aesthetic with classic digital readout appearance.

---

## Controls

### Knob Style

**Visual Design:**
- Shape: Perfect circle (hardware-authentic rotary control)
- Size: Medium base (70px) with subtle 1.2x scaling for prominent parameters
- Indicator style: Single pointer line with drop shadow and yellow-gold glow
- Border treatment: Subtle dark border (#0a0a0a) defining edge

**Surface Treatment:**
- Base appearance: Concentric circles with conic gradient for machined texture
- Depth: Deep inset shadows (0 2px 4px rgba(0,0,0,0.8) inset) for professional depth
- Tick marks: None (clean minimal surface, values shown via digital readout)
- Center indicator: Yellow-gold pointer line radiating from center with glow effect

**Interaction Feel:**
Heavy industrial knobs with precise calibration feel. No hover feedback maintains hardware authenticity. Smooth continuous rotation with clear visual indicator showing current position. Digital readout below provides exact numeric value in seven-segment LED style.

### Slider Style

**Layout:**
- Orientation preference: Vertical faders (mixing console aesthetic)
- Dimensions: Medium width (24-32px track) for clear visibility
- Track design: Dark metal with LED fill following traffic light thresholds

**Thumb Design:**
- Shape: Rectangular fader cap (classic console style)
- Size: Medium, prominent enough for clear position reading
- Style: Brushed metal texture with yellow-gold accent line

**Visual Treatment:**
- Track appearance: Dark inset channel (#1a1a1a) with horizontal brushing
- Fill behavior: Bottom-up LED illumination (green → yellow → red thresholds)
- Scale marks: None on track, digital readout provides exact value

### Button Style

**Shape & Size:**
- Shape: Rectangular with subtle rounded corners (2-4px radius)
- Padding: Compact (8px vertical, 12px horizontal) for rack density
- Aspect ratio: Wide rectangle optimized for text labels

**Visual Design:**
- Default state: Dark metal surface with subtle border, power LED off
- Active state: Power LED illuminated (traffic light color), surface unchanged
- Border: Subtle dark border (#0a0a0a) with slight inset shadow

**Typography:**
- Label style: Uppercase Arial Narrow, normal weight
- Icon treatment: Power LED indicator positioned at corner or inline

---

## Spacing & Layout Philosophy

**Overall Density:**
Tight rack-style density (8-12px gaps) creating authentic hardware feel with information-dense layouts. Compact spacing maintains professional studio equipment aesthetic while ensuring controls remain clearly distinguishable and usable.

**Control Spacing:**
- Between controls: Tight 8-12px gaps for authentic rack density
- Vertical rhythm: Consistent 8-12px creating uniform grid alignment
- Grouping strategy: Multi-section panels with horizontal grooved dividers for logical separation

**Padding & Margins:**
- Edge margins: Minimal 12-16px matching rack ear mounting constraints
- Section padding: 12-16px vertical between logical sections, separated by grooved lines
- Label-to-control gap: Tight 4-6px keeping labels close to associated controls

**Layout Flexibility:**
Spacing maintains tight rack density regardless of parameter count. Complex plugins use multi-section panels with visual dividers (horizontal grooved lines with inset shadow) to organize controls logically. Grid layout for uniform controls (e.g., EQ bands), flexbox for meter sections and dynamic content.

---

## Surface Treatment

### Textures

**Background:**
- Texture type: Horizontal brushed metal (repeating-linear-gradient with 1-2px stripes)
- Intensity: Subtle, barely visible texture adding tactile dimension
- Implementation: `background: repeating-linear-gradient(0deg, #1a1a1a, #1e1e1e 1px, #1a1a1a 2px)`

**Control Surfaces:**
- Control texture: Knobs use conic gradients for machined appearance, faders use horizontal brushing
- Consistency: All surfaces share metal brushing theme, varied implementation by control type

### Depth & Dimensionality

**Shadow Strategy:**
- Shadow presence: Deep inset shadows for professional depth and hardware authenticity
- Shadow color: Pure black (rgba(0,0,0,0.8)) for dramatic contrast
- Shadow blur: Soft blur (2-4px) creating realistic depth
- Typical shadow values: `box-shadow: 0 2px 4px rgba(0,0,0,0.8) inset` for panels and controls

**Elevation System:**
- Layers: Subtle elevation with mounting holes creating simulated rack depth
- How elevation is shown: Primarily through inset shadows, minimal outset shadows
- Mounting holes: Radial gradients with deep shadows simulating threaded screw holes

**Borders:**
- Border presence: Selective borders defining control edges and panel sections
- Border style: Subtle 1px solid lines in near-black (#0a0a0a)
- Border color approach: Darker than background for recessed appearance

---

## Details & Embellishments

**Special Features:**
- Mounting holes positioned at corners and mid-points with radial gradient depth
- Pulsing power LED animation indicating active state
- Animated LED meters with smooth 30fps updates via requestAnimationFrame
- Seven-segment digital readouts with subtle glow effect

**Active State Feedback:**
Controls show interaction through power LED state only. When bypassed, control surface remains identical but power LED turns off. Active plugins show illuminated LED in appropriate traffic light color (green/yellow/red based on signal level or state).

**Hover States:**
None. Hardware-authentic behavior with no hover feedback maintains realistic rack equipment feel. User relies on power LEDs, digital readouts, and LED meters for state information.

**Focus Indicators:**
Subtle yellow-gold outline (1-2px) for keyboard navigation accessibility, maintaining minimal visual disruption to hardware aesthetic.

**Decorative Elements:**
Minimal decoration philosophy—function over form. Only decorative elements are mounting holes (simulating rack ears) and horizontal grooved divider lines separating logical sections. All other visual elements serve functional purpose (metering, status indication, value display).

---

## Technical Patterns

**CSS Patterns:**
- Border radius: Minimal (2-4px) for subtle softening of rectangular elements
- Transition speed: None for most elements (hardware doesn't animate), 150ms for LED pulsing
- Easing: Linear for LED updates maintaining authentic meter behavior

**Layout Techniques:**
- Preferred layout: CSS Grid for uniform control arrays (EQ bands), Flexbox for meter sections
- Responsive strategy: Fixed pixel sizes maintaining hardware proportions, no viewport units
- Alignment: Grid-based alignment with precise pixel spacing (8-12px gaps)

**Performance Considerations:**
- requestAnimationFrame for LED meter updates (30fps cap for performance)
- Conic gradients on knobs may impact performance with many controls (10+ knobs)
- Repeating gradients for brushed metal texture are lightweight

---

## Interaction Feel

**Responsiveness:**
Immediate control response with no transition delays. LED meters update at 30fps providing smooth visual feedback. Controls feel direct and calibrated, mimicking precision of professional hardware.

**Feedback:**
Visual feedback through LED metering (traffic light colors), power LED states, and seven-segment digital readouts. No hover states or animated transitions—feedback is functional and informative rather than decorative.

**Tactility:**
Heavy industrial hardware aesthetic. Controls feel like physical rack equipment with machined knobs, precise faders, and calibrated switches. Digital readouts and LED meters blend vintage VU meter aesthetic with modern precision.

**Overall UX Personality:**
Professional and technical. Serious mixing/mastering tool aesthetic conveying precision, reliability, and studio-grade quality. No playful elements—pure function and industrial design.

---

## Best Suited For

**Plugin Types:**
Mixing and mastering processors including:
- EQs (graphic, parametric, shelf)
- Dynamics processors (compressor, limiter, gate, expander)
- Metering and analysis tools
- Utility plugins (trim, phase, routing)
- Tape saturation and console emulation

**Design Contexts:**
Professional studio tools requiring precision and clarity. Best for plugins where metering and visual feedback are critical to operation. Ideal for plugins modeling hardware units or targeting professional mixing engineers.

**Not Recommended For:**
- Creative effects with experimental or playful character (delay, modulation, distortion)
- Synthesizers and sound design tools (lacks expressiveness for creative contexts)
- Simple utility plugins with 1-2 parameters (visual weight exceeds functional need)
- Plugins targeting beginner users (industrial aesthetic may feel intimidating)

---

## Application Guidelines

### When Applying to New Plugin

**Parameter Count Adaptation:**
- **1-3 parameters:** Single centered row with prominent knobs (1.2x scale), digital readouts below
- **4-6 parameters:** Single grid row or 2x3 grid, maintain tight 8-12px gaps, uniform knob size
- **7-9 parameters:** Grid layout (3x3 or 2x4+controls), consider grouping by function with subtle dividers
- **10+ parameters:** Multi-section panels with horizontal grooved dividers, logical grouping (input/processing/output), grid layout within sections

**Control Type Mapping:**
- Float parameters → Knobs for continuous control, vertical faders for level/gain parameters
- Boolean parameters → LED push buttons with power LED indicating state
- Choice parameters → LED buttons in horizontal row, or dropdown if many options (5+)

**Prominent Parameter Handling:**
Key parameters (threshold, ratio, output gain, mix) receive subtle 1.2x size scaling making them visually distinct without breaking grid alignment. Always positioned prominently (top row, rightmost column, or isolated section) with clear digital readout.

### Customization Points

**Easy to Adjust:**
- Accent color hue (yellow-gold can shift to blue, green, amber while maintaining industrial feel)
- LED traffic light thresholds (adjust green/yellow/red breakpoints for different metering)
- Spacing density (can expand to 12-16px for less dense layouts while maintaining aesthetic)
- Section count and divider placement (logical grouping varies by plugin function)

**Core Identity Elements:**
- Dark metal background with brushed texture (defines industrial aesthetic)
- Heavy knobs with conic gradient machined appearance
- Seven-segment digital readouts (critical to LED meter aesthetic)
- No hover states (hardware authenticity)
- Mounting holes and rack panel framing
- Traffic light LED system for metering

### Integration Notes

Conic gradients require modern browser support (Chrome 69+, Firefox 83+, Safari 12.1+). No viewport units used—all sizing in pixels for WebView compatibility. requestAnimationFrame for LED meters may require performance monitoring with very large meter arrays (20+ LED strips). Ensure JUCE WebView build includes JavaScript support for meter animations.

---

## Example Color Codes

```css
/* Backgrounds */
--bg-main: #1a1a1a;              /* Pure dark rack metal */
--bg-surface: #1e1e1e;           /* Slightly lighter panel */
--bg-border: #0a0a0a;            /* Near-black borders */

/* Accents */
--accent-primary: #ffc864;       /* Bright yellow-gold */
--accent-secondary: #e6b050;     /* Deeper gold */

/* LEDs - Traffic Light System */
--led-green: #44ff44;            /* Safe/normal levels */
--led-yellow: #ffff44;           /* Caution/medium levels */
--led-red: #ff4444;              /* Peak/danger levels */

/* Text */
--text-primary: #ffc864;         /* Yellow-gold labels */
--text-secondary: #e6b050;       /* Dimmed gold secondary */

/* Controls */
--control-base: #1a1a1a;         /* Dark metal base */
--control-indicator: #ffc864;    /* Yellow-gold pointer */

/* Shadows */
--shadow-inset: 0 2px 4px rgba(0,0,0,0.8) inset;
--shadow-glow: 0 0 8px rgba(255,200,100,0.6);

/* Brushed Metal Texture */
background: repeating-linear-gradient(
  0deg,
  #1a1a1a,
  #1e1e1e 1px,
  #1a1a1a 2px
);

/* Knob Conic Gradient (machined texture) */
background: conic-gradient(
  from 0deg,
  #1e1e1e 0deg,
  #1a1a1a 60deg,
  #1e1e1e 120deg,
  #1a1a1a 180deg,
  #1e1e1e 240deg,
  #1a1a1a 300deg,
  #1e1e1e 360deg
);
```

---

## Implementation Checklist

When applying this aesthetic to a new plugin:

- [ ] Extract core color palette and define CSS variables
- [ ] Apply typography hierarchy (Impact/Arial Black title, Arial Narrow labels, seven-segment values)
- [ ] Style knobs with concentric circles and conic gradient machined texture
- [ ] Implement vertical faders with LED track fill (traffic light colors)
- [ ] Style LED push buttons with power LED indicators
- [ ] Implement tight spacing system (8-12px gaps, compact rack density)
- [ ] Add brushed metal texture to backgrounds (repeating-linear-gradient)
- [ ] Apply deep inset shadows to panels and controls
- [ ] Create mounting holes with radial gradients at corners/mid-points
- [ ] Implement seven-segment digital readouts below each control
- [ ] Add LED meters with requestAnimationFrame (30fps, 8-10 LEDs, 6dB steps)
- [ ] Create horizontal grooved divider lines for multi-section layouts
- [ ] Apply 1.2x subtle scaling to prominent parameters
- [ ] Implement pulsing power LED animation (150ms linear)
- [ ] Disable hover states (hardware-authentic behavior)
- [ ] Test with different parameter counts (4-6, 7-9, 10+)
- [ ] Verify visual consistency with Studio Hardware aesthetic
- [ ] Validate WebView constraints (no viewport units, JavaScript enabled for animations)
- [ ] Test in both Debug and Release builds
- [ ] Verify LED meter performance with large parameter counts
