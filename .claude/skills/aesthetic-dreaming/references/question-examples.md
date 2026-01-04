# Question Batch Examples

## Full AskUserQuestion Example

### Example 1: Control Styling Question

```javascript
AskUserQuestion({
  questions: [
    {
      question: "What control styling approach fits your vision?",
      header: "Controls",
      multiSelect: false,
      options: [
        {
          label: "Flat 2D",
          description: "Minimal depth, clean lines, modern aesthetic"
        },
        {
          label: "Skeuomorphic",
          description: "Physical hardware realism with shadows and depth"
        },
        {
          label: "Soft/Neumorphic",
          description: "Subtle shadows and depth, soft appearance"
        },
        {
          label: "Glassy/Translucent",
          description: "Transparency effects, light reflections"
        }
      ]
    }
  ]
})
```

### Example 2: Color Palette Question

```javascript
AskUserQuestion({
  questions: [
    {
      question: "What color palette approach?",
      header: "Colors",
      multiSelect: false,
      options: [
        {
          label: "Monochromatic",
          description: "Single hue with varying brightness"
        },
        {
          label: "Complementary",
          description: "Opposing colors for high contrast"
        },
        {
          label: "Analogous",
          description: "Adjacent colors for harmony"
        },
        {
          label: "Triadic",
          description: "Three evenly spaced colors"
        }
      ]
    }
  ]
})
```

### Example 3: Typography Question

```javascript
AskUserQuestion({
  questions: [
    {
      question: "What typography style fits your aesthetic?",
      header: "Typography",
      multiSelect: false,
      options: [
        {
          label: "Sans-serif modern",
          description: "Clean, contemporary, highly readable"
        },
        {
          label: "Monospace technical",
          description: "Fixed-width, developer-style"
        },
        {
          label: "Serif classic",
          description: "Traditional, sophisticated"
        },
        {
          label: "Display/Custom",
          description: "Unique, stylized, brand-specific"
        }
      ]
    }
  ]
})
```

### Example 4: Spacing Philosophy Question

```javascript
AskUserQuestion({
  questions: [
    {
      question: "What's your spacing philosophy?",
      header: "Spacing",
      multiSelect: false,
      options: [
        {
          label: "Compact/Dense",
          description: "More controls visible, efficient use of space"
        },
        {
          label: "Balanced",
          description: "Moderate spacing, neither cramped nor sparse"
        },
        {
          label: "Generous/Spacious",
          description: "Breathing room, minimalist feel"
        },
        {
          label: "Adaptive",
          description: "Varies by plugin complexity"
        }
      ]
    }
  ]
})
```

## Question Generation Strategies

### Strategy 1: Tier-Based Prioritization

Always prioritize gaps by tier:
1. **Tier 1 (Critical)**: Overall vibe/mood, color philosophy, control style
2. **Tier 2 (Visual Core)**: Specific colors, typography, spacing, textures, control details
3. **Tier 3 (Context)**: Best suited for, special features, inspirations

### Strategy 2: Context-Aware Phrasing

Tailor questions to user's accumulated context:
- If user mentioned "vintage", ask about specific era references
- If user mentioned "technical", ask about precision vs. approachability
- If user mentioned specific plugin type, ask about typical use cases

### Strategy 3: Option Design

For each question:
- Provide 3-4 distinct, non-overlapping options
- Include descriptions that explain the choice
- Avoid jargon unless user demonstrated technical fluency
- "Other" option is automatically included by tool

### Strategy 4: Redundancy Prevention

Before generating questions:
1. List all concepts already mentioned
2. Cross-reference against tier checklist
3. Only ask about uncovered gaps
4. Never rephrase a previous question

## Full Question Banks

See references/aesthetic-questions.md for comprehensive question banks organized by category (controls, colors, typography, spacing, textures, special features).
