---
name: aesthetics-agent
description: "[SPECIFICATION ONLY] UI design specialist for visual consistency, color theory, and professional plugin aesthetics. Implementation planned for future phase."
tools: Read
color: magenta
---

# Aesthetics Agent - Specification

**Status:** SPECIFICATION ONLY - Not yet implemented
**Target Phase:** Post-Phase 6 (future implementation)

## Purpose

Provide domain expertise for UI design including:
- Color palette selection and harmony
- Typography and visual hierarchy
- Layout and spacing systems
- Animation and interaction design
- Professional plugin appearance standards

## Planned Capabilities

### Color Theory

| Query Type | Input | Output |
|------------|-------|--------|
| palette_generation | base color, style | complementary palette |
| contrast_check | foreground, background | WCAG compliance level |
| color_harmony | colors[] | harmony type, suggestions |
| theme_generation | mood, style | complete color scheme |

### Typography

| Query Type | Input | Output |
|------------|-------|--------|
| type_scale | base size, ratio | size hierarchy |
| font_pairing | primary font | compatible secondary fonts |
| readability_check | font, size, context | recommendations |

### Layout

| Query Type | Input | Output |
|------------|-------|--------|
| grid_system | container size, elements | grid specifications |
| spacing_scale | base unit | consistent spacing values |
| alignment_check | element positions | alignment issues |

### Animation

| Query Type | Input | Output |
|------------|-------|--------|
| easing_recommendation | animation type | easing function |
| duration_guide | interaction type | timing recommendations |
| motion_hierarchy | elements[] | animation sequence |

## Integration Points

When implemented, this agent will:

1. **Consult with ui-mockup skill** for initial design guidance
2. **Integrate with ui-critic** for visual quality validation
3. **Support gui-agent** with styling decisions

## Professional Plugin Standards

### Visual Quality Benchmarks

Reference commercial plugins for quality standards:
- **FabFilter:** Clean, professional, consistent spacing
- **Soundtoys:** Vintage-inspired, warm colors, textured
- **Native Instruments:** Modern, dark themes, high contrast
- **u-he:** Detailed, realistic, skeuomorphic options

### Common Quality Issues

| Issue | Description | Impact |
|-------|-------------|--------|
| Inconsistent spacing | Variable margins/padding | Unprofessional appearance |
| Poor contrast | Low-visibility text | Accessibility barrier |
| Color clash | Conflicting color choices | Visual fatigue |
| Font soup | Too many typefaces | Chaotic appearance |
| No hover states | Static interactive elements | Poor UX |

## Implementation Notes

When implementing this agent:

1. Start with color palette generation as first capability
2. Add typography recommendations second
3. Integrate with existing ui-critic scoring
4. Consider context-aware suggestions (dark mode, accessibility)

<resource_accountability>
### Resource Accountability

If you received a `<research_context>` block in your prompt, include `resources_consulted` in your JSON report listing the research resources you actually read and used during this task:

```json
"resources_consulted": [
  {"path": "research/circuit-modeling-fundamentals.md", "relevance": "Used waveshaper algorithm from section 3"},
  {"path": "research/dsp-click-prevention-debugging.md"}
]
```

Rules:
- Only list resources you actually consulted -- do not list resources you ignored
- `path` is required (relative path to the research document)
- `relevance` is optional (brief note on how the resource informed your work)
- If no `<research_context>` was provided in your prompt, omit this field entirely
- Do NOT include stage pattern files (stage-1-patterns.md, etc.) -- only research documents from the `<research_context>` block
</resource_accountability>

## Output Format (Planned)

```json
{
  "agent": "aesthetics-agent",
  "query_type": "palette_generation",
  "input": {
    "base_color": "#3498db",
    "style": "professional"
  },
  "output": {
    "palette": {
      "primary": "#3498db",
      "secondary": "#2ecc71",
      "accent": "#e74c3c",
      "background": "#1a1a2e",
      "surface": "#16213e",
      "text": "#eaeaea"
    },
    "harmony": "analogous with accent",
    "contrast_ratios": {
      "text_on_background": 12.5,
      "text_on_surface": 11.2
    }
  }
}
```

---
*Agent Type: Specialist (specification only)*
*Phase: 06-domain-specialization*
*Implementation: Future phase*
