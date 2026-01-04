# Design Questions Reference

**Ask only about gaps not covered in Phase 1.5 free-form vision capture.**

## Window Size

**If not mentioned, ask:**

```
What window dimensions? (e.g., 600x400, 800x600)
```

**Common sizes:**

- **Small utility**: 400x300 (simple processors, one or two controls)
- **Standard effect**: 600x400 (typical compressor/EQ)
- **Large effect**: 800x600 (complex multi-band, visual feedback)
- **Synth**: 1000x700+ (oscillators, envelopes, modulation matrix)

**Default if user unsure:** 600x400 (safe for most effects)

## Layout Organization

**If not described, ask:**

```
How should controls be organized? (grid, vertical sections, custom)
```

**Common layouts:**

- **Grid**: Equal-sized controls in rows/columns (clean, organized)
- **Vertical sections**: Logical groupings (input → processing → output)
- **Horizontal sections**: Signal flow left-to-right (compressor stages)
- **Custom**: Unique arrangements (circular modulation matrix)

**Example clarification:**

> User: "Not sure, what works best for a compressor?"
> You: "Compressors often use horizontal flow: input controls (threshold/ratio) on left, timing (attack/release) in center, output (makeup gain) on right. Want that?"

## Control Style

**If not specified, ask:**

```
What control style? (rotary knobs, linear sliders, buttons)
```

**Control types:**

- **Rotary knobs**: Classic feel, save space, good for precise values
- **Linear sliders**: Fast adjustments, good for sweeps
- **Buttons**: On/off, mode switching
- **Combo boxes**: Multiple options (filter types, ratios)

**Match to plugin type:**

- Compressors/EQs → Rotary knobs (analog feel)
- Gates/expanders → Linear sliders (threshold visualization)
- Utilities → Buttons/combos (functional, not expressive)

## Color Scheme

**If not provided, ask:**

```
Color scheme? (dark, light, custom colors)
```

**Presets:**

- **Dark** (default): `background: #2b2b2b`, `primary: #4a9eff`, `text: #ffffff`
- **Light**: `background: #f5f5f5`, `primary: #007aff`, `text: #333333`
- **Custom**: Ask for specific hex codes

**Brand consideration:**

If this is part of a plugin suite, ask: "Should this match other plugins you've built?"

## Special Elements

**If mentioned but vague, clarify:**

```
You mentioned [VU meter / analyzer / waveform]. What should it display exactly?
```

**Common special elements:**

- **VU meter**: Input/output level, gain reduction
- **Frequency analyzer**: Real-time FFT display
- **Waveform display**: Input signal visualization
- **Envelope follower**: ADSR visual feedback
- **Modulation indicators**: LFO/envelope routing

**Implementation note:** Some special elements may need C++ rendering (OpenGL). For Phase 1, design them in HTML/CSS with the understanding they may need Stage 3 (GUI) implementation.

## Example Question Batches

### Example 1: Three-knob reverb with minimal brief

**Context gaps identified:**
- Layout structure (not specified)
- Knob visual style (not specified)
- Background treatment (color mentioned but not layout)

**Generated question batch:**

```yaml
Question 1:
  question: "Layout structure for the three knobs?"
  header: "Layout"
  options:
    - label: "Horizontal row of knobs"
      description: "Classic single-row layout"
    - label: "Vertical stack"
      description: "Narrow, tall layout"
    - label: "Triangle arrangement"
      description: "Two bottom, one top"
    - label: "Other"
      description: "Custom arrangement"

Question 2:
  question: "Knob visual style preference?"
  header: "Knob Style"
  options:
    - label: "Realistic 3D knobs"
      description: "Skeuomorphic hardware look"
    - label: "Flat minimal knobs"
      description: "Modern, clean circles"
    - label: "Vintage radio knobs"
      description: "Textured, aged appearance"
    - label: "Other"
      description: "Custom design"

Question 3:
  question: "Background visual treatment?"
  header: "Background"
  options:
    - label: "Solid color with subtle gradient"
      description: "Clean, professional"
    - label: "Textured surface (metal, wood)"
      description: "Hardware-inspired"
    - label: "Geometric patterns"
      description: "Modern, abstract"
    - label: "Other"
      description: "Custom design"

Question 4:
  question: "Additional UI elements needed?"
  header: "Extras"
  options:
    - label: "Plugin name/logo only"
      description: "Minimal chrome"
    - label: "Preset browser"
      description: "Dropdown or menu"
    - label: "Visual feedback (meters, waveforms)"
      description: "Dynamic display"
    - label: "Other"
      description: "Custom elements"
```

**Tier classification:**
- Question 1 (Layout): Tier 1 - Critical gap
- Question 2 (Knob style): Tier 2 - Visual gap
- Question 3 (Background): Tier 2 - Visual gap
- Question 4 (Extras): Tier 3 - Polish gap

### Example 2: Complex multi-effect with detailed brief

**Context gaps identified:**
- Tab navigation style (brief mentions "tabbed interface" but no visual style)
- Module layout within tabs (not specified)

**Generated question batch:**

```yaml
Question 1:
  question: "Tab navigation visual style?"
  header: "Tab Style"
  options:
    - label: "Top horizontal tabs"
      description: "Classic web-style tabs"
    - label: "Side vertical tabs"
      description: "Narrow vertical strip"
    - label: "Icon-based tab bar"
      description: "Minimal, icon-only"
    - label: "Other"
      description: "Custom navigation"

Question 2:
  question: "Module layout within each tab?"
  header: "Module Layout"
  options:
    - label: "Grid arrangement"
      description: "Organized rows/columns"
    - label: "Free-form positioning"
      description: "Custom placement"
    - label: "Single-column flow"
      description: "Vertical stacking"
    - label: "Other"
      description: "Custom layout"
```

**Note:** Fewer questions needed when brief is detailed. Focus on genuinely ambiguous aspects.
