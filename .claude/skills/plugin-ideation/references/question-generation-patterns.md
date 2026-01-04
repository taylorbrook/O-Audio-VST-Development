# Question Generation Patterns

## Question Tiers

### Tier 1: Critical (always ask if missing)
**Plugin type:**
- "What type of plugin? (Effect / Instrument / MIDI / Utility / Analyzer)"
- "Who is this for? (Beginners / Producers / Mix Engineers / Sound Designers / Live Performers)"

**Core concept:**
- "What's the main sonic goal?"
- "What problem does this solve?"

**Unique value:**
- "What makes this different from existing plugins?"
- "What's the 'signature sound' or feature?"

### Tier 2: Functional (ask early to shape design)
**Parameters:**
- "What controls should the user have?"
- "Any specific ranges or behaviors?"

**Audio routing:**
- "Mono, stereo, or sidechain input?"
- "Wet/dry mix control?"

**Presets:**
- "Should this come with factory presets?"
- "Any specific starting points users should have?"

**DSP approach:**
- "Any specific algorithms or processing techniques?"
- "Reference plugins for similar sound?"

### Tier 3: Optional (nice-to-have context)
**Workflow:**
- "Typical use case in a project?"
- "Serial or parallel processing?"

**Visual style:**
- "Any aesthetic inspiration?"
- "Minimal vs detailed UI?"

**Advanced features:**
- "Modulation sources?"
- "Automation features?"

### Tier 4: Never Ask (UI implementation details)
**Do NOT ask about:**
- Exact pixel dimensions ("How wide should the knob be?")
- Font choices ("What font for labels?")
- Color codes ("What hex color for the background?")
- Layout specifics ("Should buttons be left or right?")

**Why:** These are determined during UI mockup phase, not ideation.

## Question Batch Generation Rules

### Generate exactly 4 questions per batch:

1. **Prioritize gaps in Tier 1-2**
   - If any Tier 1 missing → Include it
   - Fill remaining slots with Tier 2 gaps
   - Only ask Tier 3 if Tiers 1-2 complete

2. **Never ask what's already known**
   - Parse previous responses
   - Skip questions where answer already provided
   - Refine questions based on context

3. **Use AskUserQuestion tool format:**
   ```
   questions:
     - question: "What type of plugin?"
       header: "Type"
       multiSelect: false
       options:
         - label: "Effect"
           description: "Process audio signals"
         - label: "Instrument"
           description: "Generate sounds"
   ```

4. **Balance question types:**
   - Mix closed (options) and open (text input via "Other")
   - Avoid all yes/no questions
   - Prefer specific over general

## Extraction Example: From User Input to Questions

**User says:**
"I want a reverb plugin."

**Extract what's known:**
- Type: Effect ✓
- Core concept: Reverb (vague - need refinement) ⚠
- Unique value: Unknown ✗
- Parameters: Unknown ✗
- Target user: Unknown ✗

**Generate 4 questions:**
1. **Tier 1 gap:** "What makes this reverb unique?" (unique value)
2. **Tier 1 gap:** "Who is this for?" (target user)
3. **Tier 2 gap:** "What reverb character?" (refine core concept)
4. **Tier 2 gap:** "What controls should users have?" (parameters)

**Avoid asking:**
- "What type of plugin?" (already know: Effect)
- "What color should the UI be?" (Tier 4 - UI details)
