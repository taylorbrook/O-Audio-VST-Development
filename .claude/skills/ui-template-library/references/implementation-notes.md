# Implementation Notes: ui-template-library

Technical details about the prose-based aesthetic system implementation.

---

## Why This Approach Works

### 1. No Browser APIs Required

The system extracts visual patterns without browser-based DOM manipulation:

- **Pattern matching**: grep/sed extracts colors, fonts, dimension values
- **String analysis**: Detects control types (knobs, sliders, buttons) by class names
- **Prose generation**: Transforms extracted patterns into interpretable descriptions
- **Tool constraints**: Only requires Read, Write, Bash tools

**Example extraction**:
```bash
# Extract color palette
grep -oE 'background.*#[0-9a-fA-F]{6}' mockup.html | cut -d'#' -f2 | sort -u

# Detect control types
grep -oE 'class="[^"]*knob[^"]*"' mockup.html | wc -l
```

### 2. Aesthetics Are Conceptual

Prose descriptions capture **intent and feel** rather than rigid specifications:

- ❌ Rigid: "knob diameter: 70px, border: 2px solid #c8a882"
- ✅ Prose: "Brass rotary knobs with tick marks, medium size, warm metallic finish"

**Benefits**:
- Transferable across different parameter counts (3-knob vs 12-knob plugins)
- Claude interprets descriptions and makes context-appropriate decisions
- Human-editable without breaking machine parseability
- Adapts naturally to different layout requirements

### 3. Format Is Idempotent

Template structure ensures consistency:

- Same sections in same order (defined by aesthetic-template.md)
- Sections delimited by markdown headers (predictable parsing)
- Always produces same structure for same mockup
- Human-editable while remaining machine-parseable

**Parsing reliability**:
```bash
# Section detection always works the same way
sed -n '/^### Knob Style$/,/^### /p' aesthetic.md | head -n -1
```

### 4. Flexible Adaptation

Apply workflow interprets prose contextually:

- **3-parameter plugin**: Single row layout
- **8-parameter plugin**: 3x3 grid layout
- **12-parameter plugin**: 4x3 grid layout

**Constant across layouts**:
- Visual language (colors, control styles, typography)
- Spacing philosophy ("generous" → 32px, "tight" → 12px)
- Design intent and feel

---

## Constraints Satisfied

The skill design satisfies all technical constraints:

✅ **NO browser-based operations** (no getComputedStyle, DOM parsing)
✅ **NO complex JavaScript execution** requirements
✅ **ONLY uses Read, Write, Bash tools** (no external dependencies)
✅ **Produces consistent, parseable aesthetic.md** format
✅ **Maintains compatibility** with ui-mockup skill invocation patterns

---

## Parsing Strategy

### Extracting from aesthetic.md

**1. Section detection**:
```bash
# Extract specific section
sed -n '/^### Knob Style$/,/^### /p' aesthetic.md | head -n -1
```

**2. Value extraction**:
```bash
# Extract color codes from Example Color Codes section
sed -n '/^## Example Color Codes$/,/^## /p' aesthetic.md | \
    grep -oE '#[0-9a-fA-F]{6}'
```

**3. Prose interpretation**:

Claude reads section text and interprets descriptive language:

- "generous spacing" → `gap: 32px`
- "warm palette" → orange/brown hue range
- "subtle texture" → low-opacity background-image
- "bold typography" → font-weight: 600+

**Interpretation examples**:
```
Prose: "Brass rotary knobs with tick marks, medium-large size"
↓
CSS:
  width: 80px; height: 80px;
  border: 3px solid #c8a882;
  background: radial-gradient(circle, #d4af8a, #b89968);
  /* Add tick marks via SVG or border styling */
```

---

## Quality Control

### Before Saving Aesthetic

**Validation checklist**:
- [ ] All template sections present (verify against aesthetic-template.md)
- [ ] No placeholder text (no `[...]`, `TODO`, `FIXME`)
- [ ] Color codes valid (match `/^#[0-9a-fA-F]{6}$/`)
- [ ] Prose is descriptive (not just naked values: "32px" → "generous spacing (32px)")
- [ ] Example color codes section has at least 5 colors

**Validation implementation**:
```bash
# Check for placeholders
grep -E '\[.*\]|TODO|FIXME' aesthetic.md && echo "Placeholders found!"

# Validate color codes
grep -oE '#[0-9a-fA-F]{6}' aesthetic.md | \
  while read color; do
    [[ $color =~ ^#[0-9a-fA-F]{6}$ ]] || echo "Invalid color: $color"
  done
```

### Before Applying Aesthetic

**Validation checklist**:
- [ ] aesthetic.md exists and is readable
- [ ] Required sections present (Visual Identity, Color Palette, Control Styles)
- [ ] Parameter spec available for target plugin
- [ ] Generated HTML will follow WebView constraints (no viewport units, etc.)

**File validation**:
```bash
AESTHETIC_FILE=".claude/aesthetics/$AESTHETIC_ID/aesthetic.md"
test -f "$AESTHETIC_FILE" || { echo "Aesthetic not found: $AESTHETIC_ID"; exit 1; }
test -r "$AESTHETIC_FILE" || { echo "Aesthetic not readable: $AESTHETIC_ID"; exit 1; }
```

**Section validation**:
```bash
# Verify required sections
for section in "Visual Identity" "Color Palette" "Knob Style" "Slider Style"; do
  grep -q "### $section" "$AESTHETIC_FILE" || echo "Missing section: $section"
done
```

---

## Progressive Disclosure Benefits

The reference file system provides token efficiency:

**Typical invocations**:
- **save_aesthetic**: Loads save-operation.md (~270 tokens)
- **apply_aesthetic**: Loads apply-operation.md + aesthetic-interpretation.md (~800 tokens)
- **list_aesthetics**: Loads list-operation.md (~360 tokens)

**Rare invocations**:
- **delete_aesthetic**: Loads delete-operation.md (~360 tokens)
- **update_aesthetic**: Loads update-operation.md (~480 tokens)

**Implementation notes**: Only loaded when debugging or investigating issues (~450 tokens)

**Result**: 80% of invocations never load this file, saving ~450 tokens per operation.
