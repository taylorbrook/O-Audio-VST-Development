# Aesthetic Library

This directory stores reusable visual design systems (aesthetics) for plugin UIs.

## Structure

Each aesthetic is stored in its own directory:

```
[aesthetic-id]/
├── aesthetic.yaml      # Visual system specification
├── preview.html        # Visual reference (original mockup)
└── metadata.json       # Name, description, usage tracking
```

## Usage

- **Save aesthetic:** Use ui-mockup skill → Choose "Save as aesthetic template" in decision menu
- **Apply aesthetic:** Use ui-mockup skill → Choose "Start from aesthetic template" at beginning
- **List aesthetics:** Invoke ui-template-library skill with "list" operation
- **Delete aesthetic:** Invoke ui-template-library skill with "delete" operation

## What is an Aesthetic?

An aesthetic captures the **visual system** (colors, typography, control styling, spacing, effects) but NOT the **layout** (parameter count, grid structure).

This allows applying the same visual design to plugins with different parameter counts.

**Example:**
- Aesthetic: "Vintage Hardware" (brass knobs, warm colors, textured background)
- 3-param plugin: Horizontal layout, 3 brass knobs
- 8-param plugin: Grid layout 2x4, 8 brass knobs, same visual styling

## Workflow

### Save Aesthetic (After Creating Mockup)

1. Create plugin mockup with ui-mockup skill
2. In Phase 4.5 decision menu, choose option 3: "Save as aesthetic template"
3. Provide name and description
4. Aesthetic saved to `.claude/aesthetics/[aesthetic-id]/`

### Apply Aesthetic (When Starting New Plugin)

1. Start ui-mockup skill for new plugin
2. In Phase 0, choose option 1: "Start from aesthetic template"
3. Select aesthetic from list
4. Mockup generated with appropriate layout for parameter count
5. Visual styling matches aesthetic

### List and Browse Aesthetics

1. Invoke ui-template-library skill
2. View table of all aesthetics with metadata
3. Open preview HTML files to see visual samples

### Delete Aesthetic

1. Invoke ui-template-library skill with delete operation
2. Confirm deletion (won't affect existing plugins)
3. Aesthetic removed from library

## Benefits

- **Faster iteration** - Reuse visual systems across plugins
- **Consistent brand** - Build visually unified plugin family
- **No rigid templates** - Aesthetics adapt to parameter count
- **Easy discovery** - Offered through decision menus
- **Non-intrusive** - Always option to start from scratch

## Technical Details

**Aesthetic YAML contains:**
- Color palette (background, primary, accent, text colors)
- Typography (font families, sizes, weights)
- Control styling (knobs, sliders, buttons - appearance and dimensions)
- Spacing philosophy (gaps, padding, margins)
- Visual effects (shadows, borders, textures, glows)
- Suggested window dimensions (not enforced)

**Layout is NOT saved:**
- Parameter count determines layout
- Layout generated fresh when applying aesthetic
- Same aesthetic works for 3-param or 12-param plugins

**See:** `.claude/skills/ui-template-library/SKILL.md` for complete documentation
