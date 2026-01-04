# Layout Validation Reference

**Purpose:** Mathematical constraints for UI mockup layout validation. These rules prevent broken UIs without prescribing aesthetic choices.

**Audience:**
- ui-mockup orchestrator (uses calculation helpers before asking window size)
- ui-design-agent (validates generated layouts before committing)

**Philosophy:** Enforce physics (math), not style (aesthetics).

---

## Hard Rules (Always Enforce)

These are **mathematical constraints**, not style preferences. Violations should block mockup generation.

### 1. Bounds Containment

**Rule:** All controls and labels must fit within window bounds.

**Formulas:**
```
element.x >= 0
element.y >= 0
element.x + element.width <= window_width
element.y + element.height <= window_height
```

**Why:** Controls extending beyond window boundaries are invisible or non-interactive. This prevents users from being unable to access plugin features.

**Exception:** Intentional visual effects that don't interfere with interaction:
- Drop shadows extending 5-10px beyond control bounds
- Glow effects on active controls
- Decorative overlay graphics (background textures, borders)

**Validation check:**
```python
def validate_bounds(element, window_width, window_height):
    if element.x < 0:
        return f"Element extends past left edge (x={element.x})"
    if element.y < 0:
        return f"Element extends past top edge (y={element.y})"
    if element.x + element.width > window_width:
        overflow = (element.x + element.width) - window_width
        return f"Element overflows right edge by {overflow}px"
    if element.y + element.height > window_height:
        overflow = (element.y + element.height) - window_height
        return f"Element overflows bottom edge by {overflow}px"
    return None  # Valid
```

### 2. Overlap Prevention

**Rule:** Controls must not occupy the same space (bounding box collision).

**Formula:** Two elements overlap if ALL four conditions are true:
```
elem1.x < elem2.x + elem2.width  AND
elem1.x + elem1.width > elem2.x  AND
elem1.y < elem2.y + elem2.height AND
elem1.y + elem1.height > elem2.y
```

**Why:** Overlapping controls create ambiguous click targets. Users cannot reliably interact with overlapped elements, leading to frustration and unusable plugins.

**Exception:** Intentional layering with explicit parent-child relationships:
- Background images/textures (marked as layer="background")
- Decorative overlays (marked as layer="decorative")
- Grouped controls within explicit container elements
- Labels positioned above/below their associated controls (semantic relationship)

**Validation check:**
```python
def validate_no_overlaps(elements):
    errors = []
    for i, elem1 in enumerate(elements):
        for elem2 in elements[i+1:]:
            # Skip if either is explicitly background/overlay
            if elem1.get("layer") in ["background", "decorative"]:
                continue
            if elem2.get("layer") in ["background", "decorative"]:
                continue

            # Skip if one is a label for the other
            if elem1.get("label_for") == elem2.get("id"):
                continue
            if elem2.get("label_for") == elem1.get("id"):
                continue

            # Bounding box collision detection
            if (elem1.x < elem2.x + elem2.width and
                elem1.x + elem1.width > elem2.x and
                elem1.y < elem2.y + elem2.height and
                elem1.y + elem1.height > elem2.y):

                overlap_width = min(elem1.x + elem1.width, elem2.x + elem2.width) - max(elem1.x, elem2.x)
                overlap_height = min(elem1.y + elem1.height, elem2.y + elem2.height) - max(elem1.y, elem2.y)
                errors.append(
                    f"{elem1.id} overlaps {elem2.id} "
                    f"({overlap_width}px × {overlap_height}px overlap region)"
                )
    return errors
```

### 3. Minimum Usable Sizes

**Rule:** Controls must meet minimum dimensions for usable interaction.

**Minimum sizes:**

| Control Type | Minimum Width | Minimum Height | Rationale |
|-------------|---------------|----------------|-----------|
| Slider (horizontal) | 100px | 30px | Touch target + visual feedback space |
| Slider (vertical) | 30px | 100px | Touch target + visual feedback space |
| Knob/rotary | 40px | 40px | Standard touch target size (44px iOS guideline) |
| Button | 60px | 24px | Minimum readable text + padding |
| Toggle/checkbox | 20px | 20px | Minimum discernible on/off states |
| Dropdown/combo box | 80px | 24px | Minimum text visibility + arrow indicator |
| Label (text) | auto | 16px | Minimum readable text height |

**Why these numbers:**
- **Touch target size:** iOS Human Interface Guidelines recommend 44px minimum, Android recommends 48dp
- **Mouse precision:** 20px is comfortable minimum for mouse clicks at typical plugin scale
- **Visual feedback:** Sliders need space for thumb + track visibility
- **Text readability:** 24px height accommodates 12-14px font + padding

**Validation check:**
```python
MINIMUM_SIZES = {
    "slider": {"horizontal": {"width": 100, "height": 30}, "vertical": {"width": 30, "height": 100}},
    "knob": {"width": 40, "height": 40},
    "button": {"width": 60, "height": 24},
    "toggle": {"width": 20, "height": 20},
    "combo": {"width": 80, "height": 24},
    "label": {"width": None, "height": 16}  # Width auto-calculated
}

def validate_minimum_sizes(element):
    control_type = element.type
    if control_type not in MINIMUM_SIZES:
        return None  # Unknown type, skip validation

    min_size = MINIMUM_SIZES[control_type]

    # Handle orientation-specific minimums (sliders)
    if control_type == "slider":
        orientation = element.get("orientation", "horizontal")
        min_size = min_size[orientation]

    errors = []

    # Check width (if specified)
    if min_size.get("width") is not None:
        if element.width < min_size["width"]:
            errors.append(
                f"{element.id} width too small: {element.width}px "
                f"(minimum {min_size['width']}px for {control_type})"
            )

    # Check height (if specified)
    if min_size.get("height") is not None:
        if element.height < min_size["height"]:
            errors.append(
                f"{element.id} height too small: {element.height}px "
                f"(minimum {min_size['height']}px for {control_type})"
            )

    return errors
```

### 4. Text Legibility

**Rule:** Labels must be large enough to contain their text content without truncation.

**Formula (rough estimate):**
```
label_width >= text.length × font_size × 0.6
```

**Why 0.6 multiplier:**
- Average character width is approximately 60% of font size for typical sans-serif fonts
- Accounts for kerning and spacing variations
- Conservative estimate (prefers slightly too wide over too narrow)

**Minimum font size:** 10px (anything smaller is unreadable on standard displays)

**Handling text overflow:**
- **Option 1:** Auto-resize container to fit text (preferred for labels)
- **Option 2:** Truncate with ellipsis (...) and show full text on hover
- **Option 3:** Use multi-line text with word wrap (for longer descriptions)

**Validation check:**
```python
def validate_text_legibility(element):
    if element.type != "label":
        return None

    text = element.get("text", "")
    font_size = element.get("font_size", 12)

    errors = []

    # Check minimum font size
    if font_size < 10:
        errors.append(
            f"{element.id} font too small: {font_size}px (minimum 10px for readability)"
        )

    # Estimate required width
    estimated_width = len(text) * font_size * 0.6

    # Check if container is large enough
    if element.width < estimated_width:
        errors.append(
            f"{element.id} text may truncate: container {element.width}px wide, "
            f"estimated {int(estimated_width)}px needed for '{text}'"
        )

    return errors
```

### 5. Minimum Spacing

**Rule:** Maintain minimum spacing between controls and window edges.

**Minimum spacing values:**

| Spacing Type | Minimum | Rationale |
|-------------|---------|-----------|
| Between adjacent controls | 10px | Prevents accidental clicks on wrong control |
| From window edges to first control | 15px | Visual breathing room, prevents cramped feel |
| Label to associated control | 5px | Visual grouping while maintaining separation |

**Why these values:**
- **10px between controls:** Comfortable separation for mouse precision, prevents fat-finger errors
- **15px edge margin:** Industry standard for UI padding, prevents controls from visually bleeding into window frame
- **5px label spacing:** Close enough to show association, far enough to be distinct elements

**Note:** These are minimums. Larger spacing is acceptable and often desirable for aesthetic reasons.

**Validation check:**
```python
def validate_minimum_spacing(elements, window_width, window_height):
    warnings = []  # Spacing violations are warnings, not errors

    # Check edge spacing
    for element in elements:
        if element.get("layer") in ["background", "decorative"]:
            continue

        # Left edge
        if element.x < 15:
            warnings.append(f"{element.id} too close to left edge ({element.x}px, recommend 15px minimum)")

        # Top edge
        if element.y < 15:
            warnings.append(f"{element.id} too close to top edge ({element.y}px, recommend 15px minimum)")

        # Right edge
        right_distance = window_width - (element.x + element.width)
        if right_distance < 15:
            warnings.append(f"{element.id} too close to right edge ({right_distance}px, recommend 15px minimum)")

        # Bottom edge
        bottom_distance = window_height - (element.y + element.height)
        if bottom_distance < 15:
            warnings.append(f"{element.id} too close to bottom edge ({bottom_distance}px, recommend 15px minimum)")

    # Check spacing between controls
    for i, elem1 in enumerate(elements):
        if elem1.get("layer") in ["background", "decorative"]:
            continue

        for elem2 in elements[i+1:]:
            if elem2.get("layer") in ["background", "decorative"]:
                continue

            # Skip if one is a label for the other
            if elem1.get("label_for") == elem2.get("id"):
                continue
            if elem2.get("label_for") == elem1.get("id"):
                continue

            # Calculate minimum distance between elements
            horizontal_gap = max(elem2.x - (elem1.x + elem1.width), elem1.x - (elem2.x + elem2.width))
            vertical_gap = max(elem2.y - (elem1.y + elem1.height), elem1.y - (elem2.y + elem2.height))

            # Elements are adjacent if gaps are small
            if horizontal_gap >= 0 and horizontal_gap < 10:
                warnings.append(
                    f"{elem1.id} and {elem2.id} only have {horizontal_gap}px horizontal gap "
                    f"(recommend 10px minimum)"
                )

            if vertical_gap >= 0 and vertical_gap < 10:
                warnings.append(
                    f"{elem1.id} and {elem2.id} only have {vertical_gap}px vertical gap "
                    f"(recommend 10px minimum)"
                )

    return warnings
```

### What's NOT Included in Hard Rules

These are aesthetic choices that should NOT be enforced:

- **Balanced margins:** Vintage styles use thick borders, minimal styles use edge-to-edge layouts
- **Equal spacing between controls:** Grid layouts want this, organic layouts don't
- **Grid alignment to 5px/10px multiples:** Style preference, not a functional requirement
- **Centered elements:** Asymmetric layouts are valid design choices
- **Color contrast ratios:** Important for accessibility but not layout math
- **Visual balance:** Subjective aesthetic judgment
- **Consistent control sizes:** Hierarchical designs intentionally vary sizes

---

## Calculation Helpers

Mathematical formulas to estimate space requirements BEFORE asking user for window size.

### Control Size Estimation

**Typical sizes for each control type:**

| Control Type | Small | Medium | Large | Notes |
|-------------|-------|--------|-------|-------|
| Knob (rotary) | 60×60px | 80×80px | 100×100px | Standard circular control |
| Horizontal slider | 150×30px | 200×30px | 250×40px | Length varies by range precision |
| Vertical slider | 30×150px | 30×200px | 40×250px | Height varies by range precision |
| Button | 80×30px | 100×35px | 120×40px | Width depends on label text |
| Toggle/checkbox | 40×20px | 50×25px | 60×30px | Switch-style toggle |
| Dropdown/combo | 120×30px | 150×35px | 180×40px | Width depends on longest option |
| Label (above control) | control_width×20px | control_width×25px | control_width×30px | Matches control width |
| Label (beside control) | auto×control_height | auto×control_height | auto×control_height | Auto-sized to text |

**Selection criteria:**

- **Small:** Utility plugins, dense control layouts, vintage compact designs
- **Medium:** Default choice for most plugins (good balance of usability and space efficiency)
- **Large:** Accessibility focus, touch-screen interfaces, simple plugins with few controls

### Layout-Specific Formulas

#### Horizontal Layout

**Characteristics:** Controls arranged in a single row.

**Formula:**
```
min_width = sum(control_widths) + (control_count + 1) × spacing + left_margin + right_margin
min_height = max(control_heights) + top_margin + bottom_margin + label_height
```

**Example calculation:**
```
Controls: 3 knobs (80px each), 2 toggles (50px each)
Spacing: 20px between controls
Margins: 30px all sides
Labels: 25px height above each control

control_widths = [80, 80, 80, 50, 50] = 340px total
control_count = 5
spacing_gaps = (5 + 1) = 6 gaps × 20px = 120px

min_width = 340 + 120 + 30 + 30 = 520px
min_height = 80 (tallest knob) + 30 + 30 + 25 = 165px
```

#### Vertical Layout

**Characteristics:** Controls arranged in a single column.

**Formula:**
```
min_width = max(control_widths) + left_margin + right_margin
min_height = sum(control_heights) + (control_count + 1) × spacing + top_margin + bottom_margin + total_label_height
```

**Example calculation:**
```
Controls: 4 sliders (200×30px each)
Spacing: 15px between controls
Margins: 25px all sides
Labels: 20px height above each slider

control_heights = [30, 30, 30, 30] = 120px total
label_heights = [20, 20, 20, 20] = 80px total
control_count = 4
spacing_gaps = (4 + 1) = 5 gaps × 15px = 75px

min_width = 200 + 25 + 25 = 250px
min_height = 120 + 80 + 75 + 25 + 25 = 325px
```

#### Grid Layout

**Characteristics:** Controls arranged in rows and columns.

**Formula:**
```
cols = ceil(sqrt(control_count))  # Approximate square grid
rows = ceil(control_count / cols)

min_width = cols × max(control_widths) + (cols + 1) × spacing + left_margin + right_margin
min_height = rows × max(control_heights) + (rows + 1) × spacing + top_margin + bottom_margin + label_height
```

**Example calculation:**
```
Controls: 8 knobs (80×80px each)
Spacing: 20px between controls
Margins: 30px all sides
Labels: 20px height above each knob

cols = ceil(sqrt(8)) = 3
rows = ceil(8 / 3) = 3

min_width = 3 × 80 + (3 + 1) × 20 + 30 + 30 = 240 + 80 + 60 = 380px
min_height = 3 × 80 + (3 + 1) × 20 + 30 + 30 + 20 = 240 + 80 + 60 + 20 = 400px
```

**Grid optimization:**
- For rectangular layouts, adjust cols to match aspect ratio: `cols = round(sqrt(control_count × window_aspect_ratio))`
- For control groups, calculate sub-grids independently then combine

#### Custom Layout

**Characteristics:** Irregular positioning, multiple sections, complex arrangements.

**Formula (bounding box calculation):**
```
# After manually positioning all elements:
min_width = max(element.x + element.width for all elements) + right_margin
min_height = max(element.y + element.height for all elements) + bottom_margin
```

**Example calculation:**
```
Elements positioned at:
- Knob at (30, 50), size 80×80
- Slider at (130, 60), size 200×30
- Button at (30, 150), size 100×35

max_right = max(30+80, 130+200, 30+100) = 330px
max_bottom = max(50+80, 60+30, 150+35) = 185px

min_width = 330 + 30 (right margin) = 360px
min_height = 185 + 30 (bottom margin) = 215px
```

### Recommended Dimensions Algorithm

**Purpose:** Calculate comfortable window size with breathing room.

```python
def calculate_recommended_dimensions(layout_type, controls, spacing, margins):
    # Step 1: Calculate absolute minimum (tight fit)
    absolute_min = calculate_from_formula(layout_type, controls, spacing, margins)

    # Step 2: Add 20% breathing room for comfortable layout
    recommended_width = absolute_min.width * 1.2
    recommended_height = absolute_min.height * 1.2

    # Step 3: Round up to nearest 50px (clean dimensions)
    recommended_width = ceil(recommended_width / 50) * 50
    recommended_height = ceil(recommended_height / 50) * 50

    # Step 4: Enforce minimum usable size (too small is unusable)
    recommended_width = max(recommended_width, 400)  # Minimum plugin width
    recommended_height = max(recommended_height, 300)  # Minimum plugin height

    # Step 5: Enforce maximum reasonable size (too large wastes screen space)
    recommended_width = min(recommended_width, 1200)
    recommended_height = min(recommended_height, 800)

    return {
        "width": recommended_width,
        "height": recommended_height,
        "absolute_minimum": {
            "width": absolute_min.width,
            "height": absolute_min.height
        }
    }
```

**Why 20% breathing room:**
- Prevents cramped layouts
- Accounts for visual weight of controls (larger controls feel more comfortable with space)
- Provides flexibility for minor adjustments without recalculation

**Why round to 50px:**
- Clean numbers are easier to remember and communicate
- Aligns with common screen resolutions
- Prevents odd dimensions like 427×318

**Why enforce 400×300 minimum:**
- Below this size, plugins feel cramped and unprofessional
- Too small to comfortably manipulate controls
- Industry standard minimum for plugin UIs

**Why enforce 1200×800 maximum:**
- Larger than most DAW mixer channels allow
- Wastes screen space on typical displays
- Consider resizable plugin for larger needs

### Window Size Calculation Workflow

**For orchestrator to use BEFORE asking user for window size:**

```
Step 1: Count controls from user requirements
    - Parse control types from design requirements
    - Count parameters from creative-brief.md (if exists)

Step 2: Estimate control sizes based on layout style
    - Vintage/skeuomorphic → Large (100px knobs)
    - Modern/minimal → Medium (80px knobs)
    - Utility/compact → Small (60px knobs)

Step 3: Calculate minimum dimensions using formulas
    - Select appropriate layout formula
    - Use estimated control sizes
    - Apply standard spacing (20px) and margins (30px)

Step 4: Calculate recommended dimensions (min × 1.2, round to 50px)
    - Add breathing room
    - Round to clean numbers
    - Enforce min/max constraints

Step 5: PRESENT calculation to user with explanation
    Example:
    "Based on 5 controls in horizontal layout, I estimate:
     - Minimum size: 520×165px (tight fit)
     - Recommended: 650×200px (comfortable spacing)

    Would you like to use the recommended size, or specify custom dimensions?"

Step 6: Ask for window size with recommended as default
    - Offer recommended as option 1
    - Offer custom as option 2
    - Offer recalculate with different assumptions as option 3

Step 7: Warn if user chooses size smaller than minimum
    Example:
    "⚠️ Warning: 400×150px is smaller than calculated minimum (520×165px).
     Controls may overlap or extend beyond window bounds.
     Proceed anyway? (y/n)"
```

**Key principle:** Show your work. Users should understand WHY you're recommending specific dimensions, not just receive a number.

---

## Validation Scripts

Pseudocode algorithms for validating generated layouts.

### Bounds Check Algorithm

```python
def validate_bounds(elements, window_width, window_height):
    """
    Validate that all elements fit within window boundaries.

    Args:
        elements: List of UI elements with x, y, width, height properties
        window_width: Plugin window width in pixels
        window_height: Plugin window height in pixels

    Returns:
        List of error messages (empty if valid)
    """
    errors = []

    for element in elements:
        # Skip background/decorative layers (can extend beyond)
        if element.get("layer") in ["background", "decorative"]:
            continue

        # Check left boundary
        if element.x < 0:
            errors.append(
                f"❌ {element.id} extends past left edge (x={element.x})"
            )

        # Check top boundary
        if element.y < 0:
            errors.append(
                f"❌ {element.id} extends past top edge (y={element.y})"
            )

        # Check right boundary
        element_right = element.x + element.width
        if element_right > window_width:
            overflow = element_right - window_width
            errors.append(
                f"❌ {element.id} overflows right edge\n"
                f"   Element: x={element.x}, width={element.width}\n"
                f"   Right edge: {element_right}px\n"
                f"   Window width: {window_width}px\n"
                f"   Overflow: {overflow}px past boundary"
            )

        # Check bottom boundary
        element_bottom = element.y + element.height
        if element_bottom > window_height:
            overflow = element_bottom - window_height
            errors.append(
                f"❌ {element.id} overflows bottom edge\n"
                f"   Element: y={element.y}, height={element.height}\n"
                f"   Bottom edge: {element_bottom}px\n"
                f"   Window height: {window_height}px\n"
                f"   Overflow: {overflow}px past boundary"
            )

    return errors
```

### Overlap Detection Algorithm

```python
def validate_no_overlaps(elements):
    """
    Detect overlapping controls using bounding box collision.

    Args:
        elements: List of UI elements with x, y, width, height properties

    Returns:
        List of error messages (empty if valid)
    """
    errors = []

    # Compare each element against all others
    for i, elem1 in enumerate(elements):
        for elem2 in elements[i+1:]:
            # Skip if either is background/decorative layer
            if elem1.get("layer") in ["background", "decorative"]:
                continue
            if elem2.get("layer") in ["background", "decorative"]:
                continue

            # Skip if one is a label for the other (semantic relationship)
            if elem1.get("label_for") == elem2.get("id"):
                continue
            if elem2.get("label_for") == elem1.get("id"):
                continue

            # Bounding box collision detection (AABB)
            # Two rectangles overlap if ALL four conditions are true
            overlaps_horizontally = (
                elem1.x < elem2.x + elem2.width and
                elem1.x + elem1.width > elem2.x
            )

            overlaps_vertically = (
                elem1.y < elem2.y + elem2.height and
                elem1.y + elem1.height > elem2.y
            )

            if overlaps_horizontally and overlaps_vertically:
                # Calculate overlap region dimensions
                overlap_left = max(elem1.x, elem2.x)
                overlap_right = min(elem1.x + elem1.width, elem2.x + elem2.width)
                overlap_top = max(elem1.y, elem2.y)
                overlap_bottom = min(elem1.y + elem1.height, elem2.y + elem2.height)

                overlap_width = overlap_right - overlap_left
                overlap_height = overlap_bottom - overlap_top

                errors.append(
                    f"❌ {elem1.id} overlaps {elem2.id}\n"
                    f"   {elem1.id}: x={elem1.x} to {elem1.x + elem1.width}, "
                    f"y={elem1.y} to {elem1.y + elem1.height}\n"
                    f"   {elem2.id}: x={elem2.x} to {elem2.x + elem2.width}, "
                    f"y={elem2.y} to {elem2.y + elem2.height}\n"
                    f"   Overlap region: {overlap_width}px × {overlap_height}px"
                )

    return errors
```

### Minimum Size Validation

```python
MINIMUM_SIZES = {
    "slider": {
        "horizontal": {"width": 100, "height": 30},
        "vertical": {"width": 30, "height": 100}
    },
    "knob": {"width": 40, "height": 40},
    "button": {"width": 60, "height": 24},
    "toggle": {"width": 20, "height": 20},
    "combo": {"width": 80, "height": 24}
}

def validate_minimum_sizes(elements):
    """
    Validate that controls meet minimum usable sizes.

    Args:
        elements: List of UI elements with type, width, height properties

    Returns:
        List of error messages (empty if valid)
    """
    errors = []

    for element in elements:
        # Skip non-control elements
        if element.type not in MINIMUM_SIZES:
            continue

        # Get minimum size for this control type
        min_size = MINIMUM_SIZES[element.type]

        # Handle orientation-specific minimums (sliders)
        if element.type == "slider":
            orientation = element.get("orientation", "horizontal")
            min_size = min_size[orientation]

        # Check width
        if "width" in min_size:
            if element.width < min_size["width"]:
                errors.append(
                    f"❌ {element.id} width too small\n"
                    f"   Current: {element.width}px\n"
                    f"   Minimum: {min_size['width']}px for {element.type}\n"
                    f"   Reason: Controls smaller than this are difficult to interact with"
                )

        # Check height
        if "height" in min_size:
            if element.height < min_size["height"]:
                errors.append(
                    f"❌ {element.id} height too small\n"
                    f"   Current: {element.height}px\n"
                    f"   Minimum: {min_size['height']}px for {element.type}\n"
                    f"   Reason: Controls smaller than this are difficult to interact with"
                )

        # Special check: knobs should be square (or close to it)
        if element.type == "knob":
            aspect_ratio = element.width / element.height
            if aspect_ratio < 0.8 or aspect_ratio > 1.2:
                errors.append(
                    f"⚠️ {element.id} aspect ratio unusual for knob\n"
                    f"   Current: {element.width}×{element.height}px "
                    f"(ratio {aspect_ratio:.2f}:1)\n"
                    f"   Recommendation: Knobs should be approximately square"
                )

    return errors
```

### Complete Validation Workflow

```python
def validate_layout(yaml_spec):
    """
    Run all validation checks and return combined results.

    Args:
        yaml_spec: Parsed YAML specification with window and controls sections

    Returns:
        tuple: (passed: bool, errors: list, warnings: list)
    """
    errors = []
    warnings = []

    # Extract data from YAML
    window_width = yaml_spec["window"]["width"]
    window_height = yaml_spec["window"]["height"]

    # Parse all elements (controls + labels + decorative elements)
    elements = []
    for control in yaml_spec.get("controls", []):
        elements.append({
            "id": control["id"],
            "type": control["type"],
            "x": control["position"]["x"],
            "y": control["position"]["y"],
            "width": control["size"]["width"],
            "height": control["size"]["height"],
            "layer": control.get("layer", "control")
        })

    # Add labels if present
    for label in yaml_spec.get("labels", []):
        elements.append({
            "id": label["id"],
            "type": "label",
            "x": label["position"]["x"],
            "y": label["position"]["y"],
            "width": label["size"]["width"],
            "height": label["size"]["height"],
            "layer": "label",
            "label_for": label.get("for")  # Which control this labels
        })

    # Run hard rule validations (errors block generation)
    errors.extend(validate_bounds(elements, window_width, window_height))
    errors.extend(validate_no_overlaps(elements))
    errors.extend(validate_minimum_sizes(elements))

    # Run spacing validation (warnings don't block)
    spacing_issues = validate_minimum_spacing(elements, window_width, window_height)
    if spacing_issues:
        warnings.extend(spacing_issues)

    # Check for common layout issues (warnings)
    layout_warnings = validate_layout_quality(elements, window_width, window_height)
    if layout_warnings:
        warnings.extend(layout_warnings)

    # Determine pass/fail
    passed = len(errors) == 0

    # Format results
    if passed and len(warnings) == 0:
        print("✅ PASSED - No errors or warnings")
    elif passed and len(warnings) > 0:
        print(f"✅ PASSED - {len(warnings)} warning(s)")
        for warning in warnings:
            print(f"⚠️  {warning}")
    else:
        print(f"❌ FAILED - {len(errors)} error(s)")
        for error in errors:
            print(error)
        if warnings:
            print(f"\n⚠️  Also found {len(warnings)} warning(s)")

    return passed, errors, warnings


def validate_minimum_spacing(elements, window_width, window_height):
    """
    Validate minimum spacing between controls and edges.
    Returns warnings (not errors).
    """
    warnings = []

    for element in elements:
        if element.get("layer") in ["background", "decorative"]:
            continue

        # Edge spacing checks
        if element.x < 15:
            warnings.append(
                f"{element.id} only {element.x}px from left edge "
                f"(recommend 15px minimum for breathing room)"
            )

        right_distance = window_width - (element.x + element.width)
        if right_distance < 15:
            warnings.append(
                f"{element.id} only {right_distance}px from right edge "
                f"(recommend 15px minimum for breathing room)"
            )

    # Check spacing between adjacent controls
    for i, elem1 in enumerate(elements):
        if elem1.get("layer") in ["background", "decorative"]:
            continue

        for elem2 in elements[i+1:]:
            if elem2.get("layer") in ["background", "decorative"]:
                continue

            # Skip label relationships
            if elem1.get("label_for") == elem2.get("id"):
                continue

            # Calculate gaps
            horizontal_gap = max(
                elem2.x - (elem1.x + elem1.width),
                elem1.x - (elem2.x + elem2.width)
            )
            vertical_gap = max(
                elem2.y - (elem1.y + elem1.height),
                elem1.y - (elem2.y + elem2.height)
            )

            # Warn if adjacent and too close
            if 0 <= horizontal_gap < 10:
                warnings.append(
                    f"{elem1.id} and {elem2.id} only {horizontal_gap}px apart horizontally "
                    f"(recommend 10px minimum to prevent misclicks)"
                )

            if 0 <= vertical_gap < 10:
                warnings.append(
                    f"{elem1.id} and {elem2.id} only {vertical_gap}px apart vertically "
                    f"(recommend 10px minimum to prevent misclicks)"
                )

    return warnings


def validate_layout_quality(elements, window_width, window_height):
    """
    Check for common layout quality issues (warnings).
    These don't block generation but suggest improvements.
    """
    warnings = []

    # Check for excessive whitespace
    total_control_area = sum(e["width"] * e["height"] for e in elements
                            if e.get("layer") == "control")
    window_area = window_width * window_height

    if total_control_area / window_area < 0.15:
        warnings.append(
            f"Layout uses only {(total_control_area/window_area*100):.1f}% of window area. "
            f"Consider reducing window size or adding visual elements."
        )

    # Check for unusual aspect ratios
    aspect_ratio = window_width / window_height
    if aspect_ratio > 3.0:
        warnings.append(
            f"Window is very wide ({window_width}×{window_height}, {aspect_ratio:.1f}:1 ratio). "
            f"Consider more vertical layout."
        )
    elif aspect_ratio < 0.5:
        warnings.append(
            f"Window is very tall ({window_width}×{window_height}, {aspect_ratio:.1f}:1 ratio). "
            f"Consider more horizontal layout."
        )

    return warnings
```

---

## Examples

### Example 1: Horizontal Layout Calculation

**Input:**
- Layout: horizontal
- Controls: 3 knobs (80px each), 2 toggles (50px each)
- Spacing: 20px between controls
- Margins: 30px all sides
- Labels: 25px height above each control

**Calculation:**
```
# Control widths
control_widths = [80, 80, 80, 50, 50] = 340px total

# Spacing calculation
control_count = 5
spacing_gaps = (5 + 1) = 6 gaps × 20px = 120px

# Minimum width
min_width = 340 (controls) + 120 (spacing) + 30 (left margin) + 30 (right margin)
min_width = 520px

# Minimum height
max_control_height = 80px (knobs are tallest)
min_height = 80 (controls) + 30 (top margin) + 30 (bottom margin) + 25 (labels)
min_height = 165px

# Recommended dimensions (add 20% breathing room)
recommended_width = 520 × 1.2 = 624px → round to 650px
recommended_height = 165 × 1.2 = 198px → round to 200px
```

**Recommendation:**
```
Based on 5 controls in horizontal layout:
- Minimum: 520×165px (tight fit)
- Recommended: 650×200px (comfortable spacing)

Use recommended size? (y/n)
```

### Example 2: Validation Failure - Bounds Overflow

**YAML:**
```yaml
window:
  width: 400
  height: 300

controls:
  - id: gain
    type: knob
    position: { x: 350, y: 50 }
    size: { width: 80, height: 80 }
```

**Validation Results:**
```
❌ FAILED - 1 error found:

1. Bounds violation: gain overflows right edge
   Element: x=350, width=80
   Right edge: 350 + 80 = 430px
   Window width: 400px
   Overflow: 30px past boundary

   Solution: Either reduce x position to 320 or reduce window width to 430px
```

### Example 3: Validation Failure - Overlap

**YAML:**
```yaml
window:
  width: 400
  height: 300

controls:
  - id: gain
    type: knob
    position: { x: 100, y: 50 }
    size: { width: 80, height: 80 }

  - id: tone
    type: knob
    position: { x: 150, y: 50 }
    size: { width: 80, height: 80 }
```

**Validation Results:**
```
❌ FAILED - 1 error found:

1. Overlap detected: gain overlaps tone
   gain: x=100 to 180, y=50 to 130
   tone: x=150 to 230, y=50 to 130
   Overlap region: 30px × 80px

   Solution: Increase spacing between controls (recommend minimum 10px gap)
   Suggested tone position: x=190 (gives 10px gap)
```

### Example 4: Validation Failure - Minimum Size

**YAML:**
```yaml
window:
  width: 400
  height: 300

controls:
  - id: gain
    type: slider
    orientation: horizontal
    position: { x: 50, y: 100 }
    size: { width: 80, height: 30 }  # Too narrow
```

**Validation Results:**
```
❌ FAILED - 1 error found:

1. Minimum size violation: gain width too small
   Current: 80px
   Minimum: 100px for slider (horizontal)
   Reason: Controls smaller than this are difficult to interact with

   Solution: Increase width to at least 100px
```

### Example 5: Validation Success with Warnings

**YAML:**
```yaml
window:
  width: 600
  height: 400

controls:
  - id: threshold
    type: knob
    position: { x: 50, y: 100 }
    size: { width: 100, height: 100 }

  - id: ratio
    type: knob
    position: { x: 155, y: 100 }  # Only 5px gap
    size: { width: 100, height: 100 }
```

**Validation Results:**
```
✅ PASSED - 1 warning

⚠️  threshold and ratio only 5px apart horizontally (recommend 10px minimum to prevent misclicks)

Layout is valid but could be improved. Consider adjusting spacing for better usability.
```

### Example 6: Complex Layout with Multiple Issues

**YAML:**
```yaml
window:
  width: 500
  height: 400

controls:
  - id: input_gain
    type: knob
    position: { x: 450, y: 50 }  # Overflow
    size: { width: 80, height: 80 }

  - id: output_gain
    type: knob
    position: { x: 440, y: 50 }  # Overlap with input_gain
    size: { width: 80, height: 80 }

  - id: filter_freq
    type: slider
    orientation: horizontal
    position: { x: 50, y: 200 }
    size: { width: 60, height: 30 }  # Too small

  - id: bypass
    type: toggle
    position: { x: 5, y: 350 }  # Too close to edge
    size: { width: 50, height: 25 }
```

**Validation Results:**
```
❌ FAILED - 3 errors, 2 warnings

Errors:
1. input_gain overflows right edge by 30px
   - Element: x=450, width=80 → right edge at 530px
   - Window width: 500px
   - Solution: Move to x=420 or smaller

2. input_gain overlaps output_gain
   - input_gain: x=450 to 530, y=50 to 130
   - output_gain: x=440 to 520, y=50 to 130
   - Overlap: 70px × 80px
   - Solution: Separate by at least 10px (e.g., output_gain at x=330)

3. filter_freq width too small (60px < 100px minimum for horizontal slider)
   - Reason: Sliders need 100px minimum for usable interaction
   - Solution: Increase width to 100px

Warnings:
⚠️  bypass only 5px from left edge (recommend 15px for breathing room)
⚠️  bypass only 25px from bottom edge (recommend 15px for breathing room)

Fix all errors before proceeding. Warnings are optional improvements.
```

---

## Usage Guidelines

### For ui-mockup Orchestrator

**When to use calculation helpers:**
- After gathering control count and layout type from user
- Before asking user for window dimensions
- When user says "I don't know what size to use"

**Workflow:**
1. Parse controls from user requirements
2. Select control size estimates based on style (vintage=large, modern=medium, utility=small)
3. Run appropriate layout formula
4. Calculate recommended dimensions
5. Present calculation to user with explanation
6. Ask for final window size with recommended as default

**Example prompt:**
```
Based on your requirements (5 controls, horizontal layout, modern style):

Calculation:
- Controls: 3×80px knobs + 2×50px toggles = 340px
- Spacing: 6 gaps × 20px = 120px
- Margins: 30px left + 30px right = 60px
- Total minimum width: 520px
- With 20% breathing room: 650px

Recommended window size: 650×200px

Options:
1. Use recommended (650×200px)
2. Specify custom dimensions
3. Recalculate with different spacing/margins

Choose (1-3): _
```

### For ui-design-agent

**When to run validation:**
- After generating YAML specification
- Before committing files to git
- Before returning success JSON report

**Workflow:**
1. Parse generated YAML
2. Extract window dimensions and all elements
3. Run validate_layout() function
4. If errors found: Fix and re-validate (up to 3 attempts)
5. If still errors: Return failure JSON with specific errors
6. If passed: Proceed to commit
7. Include warnings in JSON report (informational, not blocking)

**Example validation output:**
```bash
# After generating v1-ui.yaml

echo "Validating layout constraints..."
python validate_layout.py plugins/PluginName/.ideas/mockups/v1-ui.yaml

# Output:
✅ PASSED - No errors or warnings
Proceeding to commit...
```

**On failure:**
```bash
echo "Validation failed - attempting fixes..."

# Auto-fix attempt 1: Adjust positions to prevent overlaps
# Auto-fix attempt 2: Increase window size to fit overflowing elements
# Auto-fix attempt 3: Reduce control sizes to meet minimums

# If all attempts fail:
return JSON failure report with errors
```

---

## Related Documentation

- **WebView constraints:** `ui-design-rules.md` - CSS/JavaScript constraints for WebView
- **Design questions:** `design-questions.md` - Targeted question templates
- **Context extraction:** `context-extraction.md` - Loading plugin context from contracts

