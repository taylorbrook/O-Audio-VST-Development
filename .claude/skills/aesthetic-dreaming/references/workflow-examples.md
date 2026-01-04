# Workflow Examples: Adaptive Questioning in Action

This document shows how adaptive questioning works across different user input scenarios. These examples demonstrate gap analysis and question generation strategy.

**Purpose**: Learning and troubleshooting reference - not loaded during normal execution.

**See also**:
- `references/aesthetic-questions.md` - Question banks organized by tier
- `references/question-examples.md` - AskUserQuestion tool formatting

---

## Example 1: Detailed Input

```
User: "I want a vintage analog aesthetic with brass knobs, warm earth tones, and a subtle paper texture. Should feel like 1960s hardware."

Extracted:
- Vibe: Vintage analog, 1960s hardware ✓
- Color philosophy: Warm earth tones ✓
- Control style: Brass knobs ✓
- Surface: Paper texture ✓
- Typography: Not mentioned ✗
- Specific colors: Earth tones (generic) ✗
- Spacing: Not mentioned ✗
- Best for: Not mentioned ✗

Gaps identified (4 needed):
- Specific earth tone colors? (Tier 2)
- Typography approach? (Tier 2)
- Spacing density? (Tier 2)
- Best suited for what plugins? (Tier 3)

Question Batch 1:
1. "What specific earth tone palette?" → [Orange/brown/cream, Brown/tan/beige, Green/olive/khaki, Other]
2. "Typography approach?" → [Technical sans-serif, Classic serif, Vintage typewriter, Other]
3. "Spacing density?" → [Tight vintage, Comfortable balanced, Generous spacious, Other]
4. "Best suited for what plugins?" → [Tape/analog effects, Vintage compressors, All warm effects, Other]
```

---

## Example 2: Vague Input

```
User: "A clean modern aesthetic"

Extracted:
- Vibe: Clean, modern ✓
- Everything else: Not mentioned ✗

Gaps identified (4 needed):
- Color philosophy? (Tier 1)
- Control style? (Tier 1)
- Specific colors? (Tier 2)
- Typography? (Tier 2)

Question Batch 1:
1. "Color philosophy?" → [Dark with bright accents, Light with dark accents, Monochromatic, Other]
2. "Control styling?" → [Flat 2D, Subtle 3D, Skeuomorphic, Other]
3. "Primary color?" → [Blue professional, Gray neutral, Green organic, Other]
4. "Typography approach?" → [Technical/readable, Decorative/bold, Minimal/light, Other]

[Then decision gate]

If user chooses "Ask me 4 more questions":
- User answered: "Dark with bright accents", "Flat 2D", "Blue professional", "Technical/readable"

Updated context:
- Vibe: Clean, modern ✓
- Color: Dark with blue accents ✓
- Controls: Flat 2D ✓
- Typography: Technical/readable ✓

New gaps for Batch 2:
- Specific blue shade? (Tier 2)
- Spacing preference? (Tier 2)
- Surface treatment? (Tier 2)
- Best suited for? (Tier 3)
```

---

## Key Patterns

**Extraction principle**: Parse user input to identify stated concepts vs. gaps
**Gap prioritization**: Tier 1 (Critical) → Tier 2 (Visual Core) → Tier 3 (Context)
**Redundancy avoidance**: Never ask about concepts already stated by user
**Cumulative context**: Each iteration builds on previous answers
