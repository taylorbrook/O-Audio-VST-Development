# Save Aesthetic Operation - Complete Workflow

## Purpose
Analyze completed mockup HTML and generate structured prose aesthetic.md that describes the visual design.

## Step-by-Step Process

### Step 1: Read Mockup HTML

**Use Read tool to load the mockup HTML file:**
```bash
# Example path
MOCKUP_PATH="plugins/LushVerb/mockups/mockup-lushverb.html"
```

**Store the entire HTML content in memory for pattern extraction in Step 2.**

### Step 2: Extract Visual Patterns

**See `references/pattern-extraction.md` for complete extraction strategies.**

Use Read tool + bash pattern matching to extract:
- Colors (hex codes, rgba values, CSS variables)
- Fonts (family names, sizes, weights)
- Control types (knobs, sliders, buttons, switches)
- Spacing values (gap, padding, margin)
- Visual effects (box-shadow, border-radius, textures)

### Step 3: Generate Prose Descriptions

**See `references/prose-generation.md` for detailed generation guidelines.**

Transform extracted patterns into interpretable prose for each aesthetic.md section:
- Color Palette → descriptive prose
- Typography → font descriptions
- Control Design → style descriptions
- Layout Patterns → spatial descriptions
- Visual Effects → appearance descriptions

### Step 4: Write aesthetic.md

**Load template using Read tool:**
- Path: `.claude/skills/ui-template-library/assets/aesthetic-template.md`
- This template defines the exact structure for aesthetic.md
- CRITICAL: Follow template section order exactly - same sections, same sequence

**Fill template sections with generated prose from Step 3:**

1. **Core Identity** - Name, essence, mood
2. **Color Palette** - Prose + example codes
3. **Typography** - Font descriptions
4. **Control Design** - Knobs, sliders, buttons
5. **Layout Patterns** - Grid, spacing, hierarchy
6. **Visual Effects** - Shadows, depth, texture
7. **Interpretation Guidelines** - Adaptation rules

**Write using Write tool:**
```bash
OUTPUT_PATH=".claude/aesthetics/$AESTHETIC_ID/aesthetic.md"
```

### Step 5: Copy Preview HTML

**Copy mockup HTML to aesthetic directory for visual reference:**
```bash
SOURCE="plugins/LushVerb/mockups/mockup-lushverb.html"
DEST=".claude/aesthetics/$AESTHETIC_ID/preview.html"
```

Use Read (source) + Write (destination) pattern.

### Step 6: Generate metadata.json

**Create metadata file with inferred tags:**
```json
{
  "id": "vintage-hardware-001",
  "name": "Vintage Hardware",
  "sourcePlugin": "LushVerb",
  "createdDate": "2025-11-12",
  "tags": ["warm", "vintage", "analog", "hardware"],
  "usedInPlugins": [],
  "version": "1.0.0"
}
```

**Tag inference logic:**
- Color palette → color tags (warm, cool, monochrome)
- Control design → style tags (minimal, detailed, skeuomorphic)
- Visual effects → appearance tags (flat, dimensional, textured)

**Write using Write tool:**
```bash
METADATA_PATH=".claude/aesthetics/$AESTHETIC_ID/metadata.json"
```

### Step 7: Update manifest.json

**CRITICAL: Use Read → modify → Write pattern to prevent corruption:**

1. **Read current manifest:**
```bash
MANIFEST_PATH=".claude/aesthetics/manifest.json"
```

2. **Parse JSON and add new entry:**
```json
{
  "aesthetics": [
    {
      "id": "vintage-hardware-001",
      "name": "Vintage Hardware",
      "path": ".claude/aesthetics/vintage-hardware-001",
      "createdDate": "2025-11-12",
      "tags": ["warm", "vintage", "analog", "hardware"]
    }
  ]
}
```

3. **Write updated manifest using Write tool**

**NEVER append directly - always read full manifest, modify in memory, write complete file.**

### Step 7.5: Commit Changes to Git

**Commit aesthetic to git using conventional format:**

```bash
# Stage all aesthetic files
git add .claude/aesthetics/$AESTHETIC_ID/
```

**Commit with conventional format:**

```bash
git commit -m "feat(aesthetics): [aesthetic-name] - saved from [source-plugin]

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

**Example:**

```bash
git add .claude/aesthetics/vintage-hardware-001/
git commit -m "feat(aesthetics): Vintage Hardware - saved from LushVerb

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

**Why this step matters:**
- Follows Plugin Freedom System checkpoint protocol (all state changes committed)
- Prevents untracked files accumulating in `.claude/aesthetics/`
- Maintains clean git history with conventional commit format
- Ensures aesthetic is versioned and recoverable

### Step 8: Present Decision Menu

(See main SKILL.md for decision gate and menu format)
