---
description: Add current problem to Required Reading (juce8-critical-patterns.md)
argument-hint: "[optional: pattern name]"
---

<state_files>
  <file path="troubleshooting/patterns/juce8-critical-patterns.md" mode="read_write">
    Read: Count existing patterns via grep (determine next number)
    Write: Append new pattern before "## Usage Instructions" section
    Contract: Each pattern numbered sequentially (## N. Title)
  </file>
</state_files>

# Add Critical Pattern Command

Directly promote current problem to Required Reading (juce8-critical-patterns.md) without full documentation workflow.

**Use when:**
- System made this mistake multiple times
- Critical pattern affects all future plugins
- Want fast path (skip documentation ceremony)

<preconditions enforcement="blocking">
  <check target="conversation_context" condition="has_problem_solution">
    Current conversation MUST contain a solved problem with:
    - Clear description of what was wrong
    - Working solution with code examples
    - Explanation of why the fix works
  </check>

  <check target="file_exists" condition="target_file">
    Target file MUST exist: troubleshooting/patterns/juce8-critical-patterns.md
  </check>
</preconditions>

## Task

Extract pattern from current conversation and add to `troubleshooting/patterns/juce8-critical-patterns.md`.

## Steps

<critical_sequence>
  <step order="1" required="true">
    Review conversation and extract:
    - ❌ WRONG example (with code)
    - ✅ CORRECT example (with code)
    - Technical explanation (why required)
    - Context (when this applies)
  </step>

  <step order="2" required="true" tool="Bash">
    Determine pattern number:
    ```bash
    # Count existing patterns
    grep -c "^## [0-9]" troubleshooting/patterns/juce8-critical-patterns.md
    # Next number = count + 1
    ```
  </step>

  <step order="3" required="true">
    Format pattern using template:
    ```markdown
    ## N. [Pattern Name] (ALWAYS REQUIRED)

    ### ❌ WRONG ([Will cause X error])
    ```[language]
    [code showing wrong approach]
    ```

    ### ✅ CORRECT
    ```[language]
    [code showing correct approach]
    ```

    **Why:** [Technical explanation]

    **Placement/Context:** [When this applies]

    **Documented in:** [If there's a troubleshooting doc, link it]

    ---
    ```
  </step>

  <step order="4" required="true" tool="Edit">
    Add pattern to file:
    - Append before "## Usage Instructions" section
    - Update numbering if needed
  </step>

  <step order="5" required="true" blocking="true">
    Confirm completion:
    ```
    ✓ Added to Required Reading

    Pattern #N: [Pattern Name]
    Location: troubleshooting/patterns/juce8-critical-patterns.md

    All subagents (Stages 1-4) will see this pattern before code generation.
    ```
  </step>
</critical_sequence>

## Integration

**Invoked by:**
- User phrases: "add to required reading", "make this critical", "add critical pattern"
- Explicit: `/add-critical-pattern [name]`
- Option 2 from `/doc-fix` decision menu

**Related:** `/doc-fix` provides full documentation workflow with promotion option

**Notes:** Pattern name optional (inferred from context). If unsure whether critical, use `/doc-fix` instead.
