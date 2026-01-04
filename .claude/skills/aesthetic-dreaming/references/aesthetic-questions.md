# Aesthetic Questions Reference

This document provides question banks and gap analysis strategies for the aesthetic-dreaming skill's adaptive questioning system.

---

## Question Priority Tiers

### Tier 1 (Critical) - Must Ask If Missing

**Overall Vibe/Mood:**
- What's the core design philosophy?
- What feelings or associations should it evoke?
- Modern/vintage/minimal/maximal/technical/organic?

**Color Philosophy:**
- Warm vs cool temperature?
- High contrast vs subtle/monochromatic?
- Dark vs light base?
- Saturated vs desaturated?

**Control Style:**
- Flat 2D vs 3D vs skeuomorphic?
- Minimal vs detailed?
- Hardware-inspired vs software-native?

### Tier 2 (Visual Core) - Ask After Tier 1

**Specific Colors:**
- Primary background colors?
- Accent colors for highlights/interactions?
- Text colors and hierarchy?
- Control colors (knobs, sliders, buttons)?

**Typography Approach:**
- Sans-serif vs serif vs monospace?
- Font sizing philosophy (compact vs spacious)?
- Font weights (light vs bold vs varied)?
- Letter spacing (tight vs loose)?
- Text transform (uppercase vs normal)?

**Spacing Density:**
- Tight/compact (8-16px gaps)?
- Comfortable/moderate (20-32px gaps)?
- Generous/spacious (40+px gaps)?
- How to adapt spacing with parameter count?

**Surface Textures:**
- Completely flat vs textured?
- Subtle noise/grain vs prominent texture?
- Shadow strategy (none/subtle/prominent)?
- Border presence and style?

**Control Details:**
- Knob size and indicator style?
- Slider orientation and thumb design?
- Button shape and states?
- Active/hover feedback approach?

### Tier 3 (Context) - Nice to Have

**Best Suited For:**
- What plugin types does this work well with?
- What design contexts (prototypes/client work/personal)?
- What should it NOT be used for?

**Special Features:**
- Any animations or transitions?
- Glow effects or highlights?
- Decorative elements?
- Focus indicators?

**Technical Patterns:**
- Border radius preferences?
- Transition speed?
- Layout technique preferences (flexbox/grid)?

**Inspirations:**
- Any reference designs or hardware?
- Existing plugins to emulate?
- Visual style references?

---

## Gap Analysis Strategies

### Strategy 1: Keyword Detection

**Scan user input for presence of tier-related keywords:**

**Tier 1 indicators:**
- Vibe: modern, vintage, minimal, maximal, clean, dirty, technical, organic, playful, serious
- Color philosophy: warm, cool, bright, dark, monochrome, colorful, saturated, muted
- Control style: flat, 3D, skeuomorphic, realistic, abstract, hardware, software

**Tier 2 indicators:**
- Colors: hex codes, color names, palette descriptions
- Typography: font names, size mentions, spacing descriptions
- Spacing: pixel values, density descriptions (tight/loose/comfortable)
- Textures: noise, grain, shadow, texture mentions
- Controls: knob, slider, button descriptions, sizes

**Tier 3 indicators:**
- Best for: plugin type mentions, use case descriptions
- Features: animation, glow, transition mentions
- Technical: border-radius, CSS properties
- Inspirations: references to existing designs/hardware

**If keyword detected in tier → mark that topic as covered.**

### Strategy 2: Completeness Scoring

**Score each tier section 0-100% based on coverage:**

**Tier 1 sections:**
- Vibe (0-100%): Has user described overall feeling/philosophy?
- Color philosophy (0-100%): Has user indicated warm/cool, light/dark, contrast level?
- Control style (0-100%): Has user indicated flat/3D/skeuomorphic approach?

**Overall Tier 1 score = average of 3 sections**

**Tier 2 sections:**
- Specific colors (0-100%): Are concrete colors mentioned?
- Typography (0-100%): Are font choices/sizing indicated?
- Spacing (0-100%): Is density preference indicated?
- Textures (0-100%): Are surface treatments described?
- Control details (0-100%): Are specific control styles described?

**Overall Tier 2 score = average of 5 sections**

**Tier 3 sections:**
- Best for (0-100%): Are ideal use cases mentioned?
- Features (0-100%): Are special features described?
- Technical (0-100%): Are technical patterns mentioned?
- Inspirations (0-100%): Are references provided?

**Overall Tier 3 score = average of 4 sections**

**Question prioritization:**
1. If Tier 1 < 80% → Ask Tier 1 questions first
2. If Tier 1 >= 80% and Tier 2 < 60% → Ask Tier 2 questions
3. If Tier 1 >= 80% and Tier 2 >= 60% → Ask Tier 3 questions
4. If all tiers well-covered → Present finalization

### Strategy 3: Dependency Mapping

**Some concepts depend on others being established first:**

**Dependencies:**
- Specific color choices depend on color philosophy (warm/cool)
- Control details depend on control style (flat/3D)
- Spacing adaptation depends on spacing density baseline
- Typography details depend on typography approach

**Question ordering:**
1. Ask foundational questions first (philosophy, approach)
2. Then ask specific implementation questions (colors, sizes)
3. Finally ask contextual questions (best for, inspirations)

---

## Question Bank by Category

### Visual Identity Questions

**Overall Vibe:**
```
Question: "What's the core design philosophy?"
Options:
  - "Modern and minimal" (Clean lines, lots of white space)
  - "Vintage and analog" (Retro styling, warm colors)
  - "Technical and precise" (Grid-based, high info density)
  - "Organic and playful" (Curves, varied colors, friendly)
```

**Mood/Feel:**
```
Question: "What feeling should this aesthetic evoke?"
Options:
  - "Professional and serious" (Studio-grade, no-nonsense)
  - "Creative and experimental" (Playful, encourages exploration)
  - "Nostalgic and warm" (Comfortable, familiar)
  - "Futuristic and sleek" (Cutting-edge, high-tech)
```

### Color System Questions

**Temperature:**
```
Question: "Warm or cool color palette?"
Options:
  - "Warm (orange/red/yellow tones)" (Analog, cozy, inviting)
  - "Cool (blue/cyan/purple tones)" (Digital, professional, calm)
  - "Neutral (gray/brown/beige)" (Balanced, subtle)
  - "Mixed (warm + cool)" (Dynamic, varied)
```

**Contrast Level:**
```
Question: "Contrast approach?"
Options:
  - "High contrast (very dark + very bright)" (Dramatic, readable)
  - "Medium contrast (balanced tones)" (Comfortable, versatile)
  - "Low contrast (subtle differences)" (Elegant, understated)
  - "Variable (depends on context)" (Adaptive)
```

**Base Tone:**
```
Question: "Light or dark color scheme?"
Options:
  - "Dark mode (dark backgrounds)" (Modern, easy on eyes)
  - "Light mode (light backgrounds)" (Clean, spacious)
  - "Medium tone (mid-range grays)" (Balanced)
  - "Context-dependent" (Adapts to environment)
```

**Specific Colors:**
```
Question: "Primary accent color?"
Options:
  - "Blue (professional)" (#3b82f6 or similar)
  - "Orange (energetic)" (#ff6b35 or similar)
  - "Green (organic)" (#10b981 or similar)
  - "Purple (creative)" (#8b5cf6 or similar)
  - "Neutral (gray/white/black)" (No color accent)
```

### Typography Questions

**Font Family:**
```
Question: "Typography approach?"
Options:
  - "Technical sans-serif" (Helvetica, Arial, system fonts)
  - "Modern geometric sans" (Futura, Avenir, clean shapes)
  - "Classic serif" (Georgia, Times, traditional)
  - "Monospace/code" (Courier, Monaco, technical)
```

**Sizing Philosophy:**
```
Question: "Font sizing approach?"
Options:
  - "Compact (small, efficient)" (Fit more information)
  - "Comfortable (medium, balanced)" (Standard readability)
  - "Spacious (large, breathing room)" (Bold presence)
  - "Hierarchical (varied sizes)" (Clear visual layers)
```

**Styling:**
```
Question: "Font styling preferences?"
Options:
  - "Light weights (200-300)" (Elegant, minimal)
  - "Medium weights (400-500)" (Balanced, readable)
  - "Bold weights (600-700)" (Strong, impactful)
  - "Mixed weights (varied)" (Dynamic hierarchy)
```

**Letter Spacing:**
```
Question: "Letter spacing approach?"
Options:
  - "Tight (minimal gaps)" (Compact, efficient)
  - "Normal (default spacing)" (Standard readability)
  - "Wide (generous gaps)" (Technical, engineered feel)
  - "Very wide (dramatic spacing)" (Vintage hardware style)
```

**Text Transform:**
```
Question: "Text capitalization preference?"
Options:
  - "All uppercase" (Hardware labeling, technical)
  - "Title case" (Balanced, readable)
  - "Lowercase" (Minimal, modern)
  - "Mixed (depends on context)" (Varied)
```

### Control Style Questions

**Overall Approach:**
```
Question: "Control styling approach?"
Options:
  - "Flat 2D (minimal depth)" (Modern, clean)
  - "Subtle 3D (slight shadows)" (Depth without heaviness)
  - "Prominent 3D (clear depth)" (Tactile, engaging)
  - "Skeuomorphic (realistic hardware)" (Analog emulation)
```

**Knob Style:**
```
Question: "Knob visual design?"
Options:
  - "Simple circles with line indicator" (Minimal, modern)
  - "Circles with tick marks around edge" (Technical, precise)
  - "Brass/metal realistic knobs" (Skeuomorphic, vintage)
  - "Abstract shapes or custom design" (Creative, unique)
```

**Knob Size:**
```
Question: "Knob size preference?"
Options:
  - "Small (50px diameter)" (Compact, space-efficient)
  - "Medium (70px diameter)" (Balanced, standard)
  - "Large (90px+ diameter)" (Prominent, easy to grab)
  - "Variable (depends on importance)" (Hierarchical)
```

**Slider Orientation:**
```
Question: "Slider orientation preference?"
Options:
  - "Horizontal (side to side)" (Traditional, wide layouts)
  - "Vertical (up and down)" (Compact, stacked layouts)
  - "Both work equally" (Flexible, adaptive)
  - "Avoid sliders (use knobs)" (Consistent control type)
```

**Button Style:**
```
Question: "Button visual design?"
Options:
  - "Flat rectangles" (Minimal, modern)
  - "Rounded rectangles" (Friendly, approachable)
  - "Pills (fully rounded)" (Smooth, flowing)
  - "Toggle switches" (Hardware-inspired)
```

### Spacing Questions

**Overall Density:**
```
Question: "Spacing density preference?"
Options:
  - "Tight (8-16px gaps)" (Information-dense, compact)
  - "Comfortable (20-32px gaps)" (Balanced, standard)
  - "Generous (40+px gaps)" (Spacious, minimal)
  - "Variable (depends on complexity)" (Adaptive)
```

**Adaptation Strategy:**
```
Question: "How should spacing adapt with more parameters?"
Options:
  - "Fixed gaps (always same spacing)" (Consistent feel)
  - "Scale proportionally (maintain ratio)" (Balanced scaling)
  - "Compress as needed (fit more)" (Practical)
  - "Scrollable (maintain spacing)" (Never compromise)
```

### Surface Treatment Questions

**Texture Presence:**
```
Question: "Background texture approach?"
Options:
  - "Completely flat (no texture)" (Pure, clean)
  - "Subtle noise/grain" (Slight depth, not distracting)
  - "Noticeable texture (paper, metal, etc.)" (Character, tactile)
  - "Prominent pattern" (Bold, decorative)
```

**Shadow Strategy:**
```
Question: "Shadow and depth approach?"
Options:
  - "No shadows (completely flat)" (Minimal, modern)
  - "Subtle shadows (slight elevation)" (Gentle depth)
  - "Medium shadows (clear layers)" (Defined hierarchy)
  - "Dramatic shadows (strong depth)" (Bold, 3D)
```

**Border Presence:**
```
Question: "Border styling approach?"
Options:
  - "No borders (color only)" (Clean, minimal)
  - "Subtle borders (1px, low contrast)" (Defined without weight)
  - "Prominent borders (2-3px, clear)" (Strong definition)
  - "3D bevels (inset/outset)" (Hardware-inspired depth)
```

### Context Questions

**Best Suited For:**
```
Question: "What plugin types is this aesthetic best for?"
Options:
  - "Compressors and dynamics" (Utility processing)
  - "EQs and filters" (Precise technical tools)
  - "Creative effects (delay, reverb, etc.)" (Experimental processing)
  - "Synths and instruments" (Musical, expressive)
  - "All types (versatile)" (General-purpose)
```

**Not Recommended For:**
```
Question: "What types of plugins might this NOT work well for?"
Options:
  - "Simple utility plugins" (Too elaborate for basic tools)
  - "Complex multi-page plugins" (Not enough visual hierarchy)
  - "Vintage/analog emulations" (Style mismatch)
  - "Modern digital plugins" (Style mismatch)
  - "None (works for everything)" (Fully versatile)
```

**Design Context:**
```
Question: "When would you choose this aesthetic?"
Options:
  - "Quick prototypes" (Fast iteration, don't need polish)
  - "Client work" (Professional, presentable)
  - "Personal projects" (Expressive, experimental)
  - "Commercial products" (Market-ready, polished)
  - "All contexts" (General-purpose)
```

### Special Features Questions

**Animations:**
```
Question: "Animation and transition approach?"
Options:
  - "No animations (instant changes)" (Minimal, fast)
  - "Subtle transitions (150ms)" (Smooth without delay)
  - "Noticeable animations (300-500ms)" (Engaging, polished)
  - "Elaborate animations (complex sequences)" (Playful, showy)
```

**Interaction Feedback:**
```
Question: "How should controls react to interaction?"
Options:
  - "Color change only" (Minimal, simple)
  - "Subtle scale/glow" (Gentle feedback)
  - "Prominent highlight/shadow" (Clear feedback)
  - "Complex animation" (Elaborate, satisfying)
```

**Focus Indicators:**
```
Question: "Keyboard focus indicator style?"
Options:
  - "Browser default (outline)" (Standard, accessible)
  - "Subtle glow/shadow" (Polished, integrated)
  - "Color change" (Minimal, clean)
  - "Prominent border/highlight" (Clear, accessible)
```

### Inspiration Questions

**Visual References:**
```
Question: "Any design inspirations or references?"
Options:
  - "Existing plugins (FabFilter, iZotope, etc.)" (Industry standard)
  - "Hardware gear (analog equipment)" (Tactile, vintage)
  - "UI/UX design systems (Material, Fluent, etc.)" (Modern, consistent)
  - "Custom/original (no specific reference)" (Unique)
```

**Era/Style:**
```
Question: "What design era or style?"
Options:
  - "1960s-70s analog hardware" (Vintage, warm)
  - "1980s digital gear" (Retro, colorful)
  - "2000s software UI" (Early digital aesthetic)
  - "Contemporary/modern" (Current design trends)
  - "Futuristic" (Forward-looking, experimental)
```

---

## Question Generation Strategies

### Strategy 1: Contrast Pairs

When foundational aspect is established, ask about contrasting details:

**Examples:**
- User says "dark aesthetic" → Ask "What accent color to contrast the darkness?"
- User says "minimal" → Ask "How to ensure controls stand out in minimal design?"
- User says "vintage" → Ask "Authentic skeuomorphic or modern interpretation?"

### Strategy 2: Specification Ladder

When general concept is mentioned, climb the specificity ladder:

**Example progression:**
1. "Blue aesthetic" (vague)
2. Ask: "Bright electric blue or professional medium blue?" (more specific)
3. User: "Professional medium"
4. Ask: "Exact shade preference?" → Options with hex codes

### Strategy 3: Implication Questions

When one choice is made, ask about implied consequences:

**Examples:**
- User chooses "Skeuomorphic controls" → Ask "What material should controls emulate? (brass, aluminum, wood, etc.)"
- User chooses "Tight spacing" → Ask "How to maintain readability in dense layout?"
- User chooses "Dramatic shadows" → Ask "Shadow color approach? (black, colored, multi-tone)"

### Strategy 4: Tradeoff Questions

When design involves tradeoffs, present them clearly:

**Examples:**
```
Question: "Balance between aesthetic consistency and parameter count flexibility?"
Options:
  - "Strict consistency (may not fit 12+ params)" (Pure aesthetic)
  - "Flexible adaptation (spacing compresses if needed)" (Practical)
  - "Scrollable (never compromise aesthetic)" (Uncompromising)
```

```
Question: "Performance vs visual richness?"
Options:
  - "Lightweight (fast, minimal effects)" (Performance priority)
  - "Balanced (some effects, good performance)" (Compromise)
  - "Rich (animations, textures, shadows)" (Visual priority)
```

---

## Avoiding Redundancy

### Rule 1: Never Ask What's Already Stated

**Bad:**
```
User: "I want a dark modern aesthetic with blue accents"
System: "What's the overall vibe?" ❌ (Already stated: modern)
System: "Light or dark?" ❌ (Already stated: dark)
System: "What accent color?" ❌ (Already stated: blue)
```

**Good:**
```
User: "I want a dark modern aesthetic with blue accents"
System: "What shade of blue?" ✅ (Refining stated concept)
System: "Control styling approach?" ✅ (Not yet mentioned)
System: "Spacing density?" ✅ (Not yet mentioned)
```

### Rule 2: Don't Repeat Context

If user has answered a question, don't ask it again in different words:

**Bad:**
```
Batch 1: "Spacing density?" → User: "Comfortable (24px)"
Batch 2: "How much space between controls?" ❌ (Already answered)
```

**Good:**
```
Batch 1: "Spacing density?" → User: "Comfortable (24px)"
Batch 2: "How should spacing adapt with 12+ params?" ✅ (Different aspect)
```

### Rule 3: Extract Maximum Information

Parse user responses thoroughly before asking more:

**Example:**
```
User: "I want something like vintage hardware - brass knobs, warm browns, subtle wear texture"

Extracted:
- Vibe: Vintage hardware ✓
- Control style: Brass (skeuomorphic) ✓
- Colors: Warm browns ✓
- Texture: Subtle wear ✓

Don't ask about any of these - they're covered!
```

---

## Decision Gate Phrasing

**Standard decision gate (after each batch):**
```
Ready to finalize the aesthetic concept?

1. Yes, finalize it - Create aesthetic.md and optional previews
2. Ask me 4 more questions - Continue refining the design
3. Let me add more context first - Provide additional details
```

**When near-complete (80%+ coverage):**
```
The aesthetic concept is well-defined. Ready to finalize?

1. Yes, finalize it - Create aesthetic.md and previews
2. Ask me 4 more questions - Polish remaining details
3. Let me review what we have - Show summary before finalizing
```

**When minimal info provided (still in early exploration):**
```
We're still exploring the aesthetic. What's next?

1. Ask me 4 more questions - Help me refine the vision
2. Let me provide more details - I'll describe more freely
3. Start over - Different direction entirely
```

---

## Summary

This reference provides:
- **Tier-based question prioritization** - Critical → Visual → Context
- **Question banks by category** - Ready-made questions for all visual aspects
- **Gap analysis strategies** - How to identify what's missing
- **Question generation strategies** - How to create contextual questions
- **Redundancy avoidance rules** - Never ask what's already known

Use these resources to implement adaptive questioning that feels conversational, not interrogative.
