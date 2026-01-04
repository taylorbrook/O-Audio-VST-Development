# Common Pitfalls: ui-mockup Skill

## Premature Scaffolding Generation (Severity: HIGH)

### The Anti-Pattern

❌ NEVER generate Phase B files (3-7) without user approval from Phase 5.5 menu

❌ NEVER skip the Phase 5.5 decision menu after Phase A completes

❌ NEVER assume design is ready for implementation without explicit "Finalize" choice

❌ NEVER proceed to Phases 6-10 if finalization marker is missing from YAML

### The Correct Pattern

✓ ALWAYS present Phase 5.5 decision menu after Phase A (files 1-2 generated)

✓ ALWAYS wait for explicit option 2 (Finalize) choice before Phase B

✓ ALWAYS verify gate criteria (WebView constraints + creative brief validation)

✓ ALWAYS check finalization marker in YAML before generating any Phase B file

### Why This Matters

The entire two-phase design exists to avoid generating C++ boilerplate (files 3-7) when design is still changing.

**Broken workflow** (generates all 7 files at once):
1. User tests design in browser
2. User wants to change layout/colors/controls
3. Phase A files (1-2) regenerated ✓
4. Phase B files (3-7) now outdated and must be regenerated ✗
5. Wasted time on boilerplate that became obsolete

**Correct workflow** (respects gate):
1. Generate Phase A (YAML + test HTML) → commit → present menu
2. User iterates (option 1) OR finalizes (option 2)
3. If iterate: stay in Phase A, increment version, repeat
4. If finalize: validate, mark as finalized, proceed to Phase B
5. Generate Phase B files (3-7) ONLY for finalized design
6. Implementation files match locked design (no drift)

### Technical Enforcement

The gate is enforced via:
1. Finalization marker in YAML (`finalized: true`)
2. Phase B guard checks before Phases 6-10
3. Phase 5.5 decision menu blocking auto-progression
4. State tracking in `.continue-here.md`

See `references/phase-b-enforcement.md` for implementation details.

## Other Common Pitfalls

### Missing Context Extraction (Severity: MEDIUM)

❌ Don't ask users to repeat information from creative-brief.md

✓ Always read creative-brief.md if it exists (Phase 1)

✓ Extract UI concept, parameters, plugin type, visual references

✓ Build on what user already provided

### Viewport Units in CSS (Severity: HIGH)

❌ Never use `100vh`, `100vw`, `100dvh`, `100svh` in CSS

✓ Always use `html, body { height: 100%; }` for full-screen layouts

**Why**: Viewport units cause blank screens in JUCE WebView. This is a JUCE 8 limitation.

See `references/ui-design-rules.md` for complete constraints.

### Skipping WebView Validation (Severity: HIGH)

❌ Don't proceed to Phase 5.5 without running Phase 5.3 validation

✓ Always validate WebView constraints before presenting decision menu

✓ Catch forbidden patterns early (viewport units, missing user-select: none, etc.)

### Parameter ID Naming Issues (Severity: MEDIUM)

❌ Don't use uppercase, spaces, hyphens, or special chars in parameter IDs

✓ Always use lowercase, snake_case, alphanumeric + underscore only

✓ Ensure IDs are valid C++ identifiers (no reserved keywords)

**Why**: Invalid IDs cause compile-time errors in PluginProcessor.cpp
