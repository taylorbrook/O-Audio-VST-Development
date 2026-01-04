# How to Add Critical Patterns - User Guide

**TL;DR:** YOU decide what's critical, not Claude. Three ways to promote patterns.

---

## What Makes a Pattern "Critical"?

Critical patterns are mistakes the system keeps making across different plugins. They get injected into subagent prompts so the pattern is seen BEFORE code generation.

**Signs a pattern is critical:**
- You've fixed this same issue 2+ times on different plugins
- The solution is non-obvious (not a simple typo)
- It's a JUCE 8 requirement that differs from JUCE 7
- Foundational (affects CMake, build system, threading, APVTS)
- Must be followed every time (not optional)

**Examples of critical patterns:**
- ✅ `juce_generate_juce_header()` required in CMakeLists.txt
- ✅ WebView needs `juce::juce_gui_extra` + `JUCE_WEB_BROWSER=1`
- ✅ Effects need input+output bus, instruments need output-only
- ❌ One-off bug in specific DSP code (not critical)
- ❌ Typo in variable name (not critical)

---

## Three Ways to Promote Patterns

### Method 1: Through Documentation Menu (Recommended)

**When:** After solving a problem and you want to document it.

**Steps:**
1. Problem is solved, plugin works
2. Say "that worked" or run `/doc-fix`
3. Claude documents the solution in `troubleshooting/[category]/`
4. Decision menu appears:
   ```
   What's next?
   1. Continue workflow
   2. Add to Required Reading ← CHOOSE THIS
   3. Link related issues
   4. Update common patterns
   5. View documentation
   6. Other
   ```
5. Choose **Option 2**
6. Claude extracts the pattern and adds to `juce8-critical-patterns.md`
7. Done - next plugin will see this pattern

**Advantages:**
- Problem is fully documented first
- Can review documentation before promoting
- Creates paper trail (doc → critical pattern)

---

### Method 2: Direct Promotion (Fast Path)

**When:** You've already solved it and know it's critical. Skip the ceremony.

**Steps:**
1. Problem is solved, plugin works
2. Say any of:
   - "add that to required reading"
   - "make this critical"
   - "add critical pattern"
   - Or explicitly: `/add-critical-pattern [name]`
3. Claude extracts pattern from conversation
4. Adds to `juce8-critical-patterns.md`
5. Done - no documentation file created

**Advantages:**
- Fastest path (one command)
- No documentation overhead
- Ideal when you've seen this mistake 3+ times

---

### Method 3: System Suggestion (Optional Hint)

**When:** Claude thinks it might be critical and suggests it.

**How it works:**
1. Run `/doc-fix` after solving problem
2. Claude classifies: `severity: critical` in YAML
3. Decision menu includes hint:
   ```
   💡 This might be worth adding to Required Reading (Option 2)

   What's next?
   1. Continue workflow
   2. Add to Required Reading ← SUGGESTED
   3. Link related issues
   ...
   ```
4. You decide: Choose Option 2 or ignore suggestion

**This is just a hint.** Claude suggests, YOU decide.

**Claude suggests when:**
- YAML severity is `critical`
- Affects foundational stages (Stage 2 or 3)
- Solution is non-obvious

---

## What Happens After Promotion?

1. Pattern is added to `troubleshooting/patterns/juce8-critical-patterns.md`
2. Format: ❌ WRONG vs ✅ CORRECT with code examples
3. Next time ANY subagent runs (foundation, shell, dsp, gui):
   - Subagent prompt includes "CRITICAL: Required Reading"
   - Claude reads `juce8-critical-patterns.md` BEFORE generating code
   - Sees your pattern, follows correct approach
   - Mistake is prevented

**Impact:**
- Prevents repeat mistakes across ALL future plugins
- No token cost bloat (single curated file vs all docs)
- Self-improving system (gets smarter as you use it)

---

## Example Scenarios

### Scenario 1: Third Time Seeing JuceHeader Error

```
You: [Fix JuceHeader issue by adding juce_generate_juce_header()]
You: "That's the third plugin with this error. Add to required reading."

Claude: [Extracts pattern]
Claude: ✓ Added to Required Reading

Pattern #1: CMake Header Generation
Location: troubleshooting/patterns/juce8-critical-patterns.md

All subagents will see this pattern before code generation.

Next plugin: foundation-agent reads pattern → includes juce_generate_juce_header() automatically
```

---

### Scenario 2: Documented Problem, Unsure If Critical

```
You: [Fix WebView parameter binding issue]
You: "That worked"

Claude: ✓ Solution documented
        File: troubleshooting/api-usage/webview-parameter-binding-...md

        What's next?
        1. Continue workflow
        2. Add to Required Reading
        ...

You: [Choose 1 - Continue workflow]

[Later, after seeing same issue again]

You: "/add-critical-pattern WebView Event Format"

Claude: [Extracts from previous doc + current context]
Claude: ✓ Added to Required Reading
```

---

### Scenario 3: System Suggests, You Decline

```
You: [Fix obscure AudioBuffer sizing issue]
You: "/doc-fix"

Claude: ✓ Solution documented

        💡 This might be worth adding to Required Reading (Option 2)

        What's next?
        1. Continue workflow
        2. Add to Required Reading ← SUGGESTED
        ...

You: [Choose 1 - Continue, it's too specific to be critical]

Claude: [Returns to workflow, pattern stays in docs but not Required Reading]
```

---

## When to Promote vs When to Keep as Doc

### Promote to Required Reading when:
- ✅ Affects multiple plugins (not one-off)
- ✅ Non-obvious solution (not a simple typo)
- ✅ Foundational (build system, JUCE API, threading)
- ✅ Must be followed every time
- ✅ You've seen it 2+ times

### Keep as documentation only when:
- ❌ One-off bug in specific DSP implementation
- ❌ Plugin-specific edge case
- ❌ Obvious mistake (typo, syntax error)
- ❌ Solution is situation-dependent
- ❌ First time seeing this issue

**Rule of thumb:** If you think "I don't want to fix this again," promote it.

---

## Managing the Critical Patterns File

**Location:** `troubleshooting/patterns/juce8-critical-patterns.md`

**When to review:**
- Monthly: Are patterns still relevant?
- After JUCE version upgrade: Do patterns need updates?
- When promoting new pattern: Does it overlap with existing?

**When to remove patterns:**
- JUCE API changed (pattern obsolete)
- Issue was fixed upstream in JUCE
- Pattern merged with broader pattern

**Editing manually:** You can directly edit the file. It's markdown. Just maintain the format:
```markdown
## N. Pattern Name (ALWAYS REQUIRED)

### ❌ WRONG
[code]

### ✅ CORRECT
[code]

**Why:** [explanation]
```

---

## Summary

**YOU are in control.** Claude never auto-promotes. Three paths:

1. **Through menu:** `/doc-fix` → Choose Option 2
2. **Direct:** "add to required reading"
3. **Suggested:** Claude hints 💡, you decide

Pattern becomes part of subagent context → Prevents future mistakes → System learns from experience.
