# Apply Aesthetic Operation - Complete Workflow

## Purpose
Generate a new mockup for a target plugin by interpreting an aesthetic's prose description and adapting it to the plugin's specific requirements.

## Step-by-Step Process

### Step 1: Load Aesthetic

**Verify aesthetic directory exists, then load files:**

```bash
AESTHETIC_DIR=".claude/aesthetics/$AESTHETIC_ID"
AESTHETIC_MD="$AESTHETIC_DIR/aesthetic.md"
METADATA_JSON="$AESTHETIC_DIR/metadata.json"
```

**Use Read tool to load:**
1. aesthetic.md - Complete prose description
2. metadata.json - Tags and usage tracking

**Store both in memory for Steps 3-5.**

**Validation:**
- BEFORE reading: Verify aesthetic directory exists
- IF NOT: Report error and halt operation

**Success criteria:**
- aesthetic.md loaded and contains all required sections
- metadata.json loaded and is valid JSON
- Both stored for subsequent interpretation

### Step 2: Load Plugin Requirements

**Read target plugin contracts to understand parameter requirements:**

**Try in order:**
1. `plugins/$PLUGIN_NAME/.ideas/parameter-spec.md` (preferred)
2. `plugins/$PLUGIN_NAME/.ideas/creative-brief.md` (fallback)

**Extract critical information:**
- Parameter count (determines layout complexity)
- Parameter types (knobs, sliders, switches, buttons)
- Plugin purpose/category (informs aesthetic interpretation)
- Any explicit UI requirements from creative brief

### Step 3: Interpret Aesthetic Prose

**See `references/aesthetic-interpretation.md` for complete interpretation strategies.**

**Transform prose descriptions into specific values:**

**Color Palette section → CSS variables:**
```css
/* Prose: "Warm vintage palette with burnt orange accent" */
--color-primary: #ff6b35;
--color-background: #2b2015;
--color-text: #f5e6d3;
```

**Typography section → CSS font properties:**
```css
/* Prose: "Industrial sans-serif headings with condensed proportions" */
--font-heading: "Roboto Condensed", sans-serif;
--font-heading-weight: 700;
```

**Control Design section → Component styles:**
```css
/* Prose: "Large knobs with engraved tick marks, worn metal texture" */
.knob {
  width: 80px;
  background: metallic-gradient;
  box-shadow: inset 0 2px 4px rgba(0,0,0,0.3);
}
```

**Key principle:** Aesthetic prose is interpretive, not prescriptive. Adapt values to fit target plugin's parameter count and layout needs.

### Step 4: Choose Layout Pattern

**See `references/layout-generation.md` for layout selection logic.**

**Select layout based on parameter count:**
- 1-6 parameters: Single-row horizontal layout
- 7-12 parameters: Two-row grid layout
- 13-24 parameters: Multi-section grouped layout
- 25+ parameters: Tabbed or scrolling layout

**Apply aesthetic's "Layout Patterns" prose to chosen structure:**
- Spacing values from aesthetic
- Hierarchy principles from aesthetic
- Grouping strategies adapted to plugin parameters

### Step 5: Generate HTML with Aesthetic

**Load base template structure from skill assets, then inject aesthetic-specific values.**

**Template locations:**
- Base HTML structure: Common mockup pattern (see ui-mockup skill)
- CSS variable insertion: Extract from aesthetic.md "Example Color Codes" section
- Control styling: Generate using `references/control-generation.md` strategies

**Complete HTML structure:**
```html
<!DOCTYPE html>
<html>
<head>
  <style>
    :root {
      /* Inject aesthetic color variables */
    }
    /* Inject aesthetic typography styles */
    /* Inject aesthetic control styles */
    /* Inject aesthetic layout patterns */
  </style>
</head>
<body>
  <!-- Generate parameter controls using aesthetic interpretation -->
</body>
</html>
```

**See `references/control-generation.md` for complete HTML/CSS generation logic.**

### Step 6: Save Generated Mockup

**Write mockup to target plugin's mockups directory:**

```bash
OUTPUT_PATH="plugins/$PLUGIN_NAME/mockups/mockup-$PLUGIN_NAME_LOWER.html"
```

**Use Write tool to save complete HTML.**

**Ensure mockups directory exists:**
```bash
# If directory doesn't exist, create it first
mkdir -p "plugins/$PLUGIN_NAME/mockups"
```

### Step 7: Update Aesthetic Metadata

**Track which plugins use this aesthetic:**

1. **Read aesthetic metadata.json** (loaded in Step 1)
2. **Add plugin to usedInPlugins array:**
```json
{
  "usedInPlugins": [
    {
      "pluginName": "Dreamscape",
      "appliedDate": "2025-11-12",
      "mockupPath": "plugins/Dreamscape/mockups/mockup-dreamscape.html"
    }
  ]
}
```
3. **Write updated metadata using Write tool**

### Step 8: Present Decision Menu

(See main SKILL.md for decision gate and menu format)
