# Design Rationale: Why aesthetic-dreaming Works This Way

This document explains the reasoning behind key design decisions in the aesthetic-dreaming skill.

**Purpose**: Meta-documentation for understanding system architecture - not needed during execution.

**Audience**: System designers, troubleshooters analyzing workflow patterns, future refactoring efforts.

---

## Why This Approach Works

**1. Adaptive questioning matches plugin-ideation pattern:**
- Free-form collection first (capture everything)
- Gap analysis identifies missing pieces
- Questions generated contextually, not from rigid script
- Decision gate allows iteration or finalization

**2. Visual design tiers make sense:**
- Tier 1 (Critical): Vibe, color philosophy, control style
- Tier 2 (Visual Core): Specific colors, typography, spacing, textures
- Tier 3 (Context): Best suited for, inspirations, special features

**3. Test previews solve the visualization problem:**
- User can see aesthetic applied to real layouts
- Multiple parameter counts demonstrate flexibility
- Optional (user can skip if they just want prose description)

**4. Integration with ui-template-library:**
- aesthetic-dreaming creates the aesthetic.md
- ui-template-library interprets and applies it
- Separation of concerns: creation vs application

---

## Format Consistency

**aesthetic.md structure:**
- Exactly 12 sections in fixed order
- Prose descriptions with concrete examples
- Interpretable by both humans and Claude
- Human-editable, machine-parseable

**Test previews:**
- Stored in test-previews/ subdirectory
- Named by plugin type: simple-compressor.html
- Standalone HTML files (open in browser)
- Demonstrate aesthetic across different layouts

---

## Quality Control

**Before saving aesthetic:**
- ✅ Verify all template sections are present
- ✅ Check for placeholder text not replaced
- ✅ Validate color codes are valid hex/rgb
- ✅ Ensure prose is descriptive (not just values)
- ✅ Confirm aesthetic ID is unique

**Before generating test previews:**
- ✅ Verify test plugin specs exist
- ✅ Check ui-template-library skill is available
- ✅ Ensure aesthetic.md is complete
- ✅ Validate output directory exists
