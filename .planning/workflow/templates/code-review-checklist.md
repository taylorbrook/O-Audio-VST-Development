# Code Review Checklist

**Plugin:** [PLUGIN]
**Stage:** [STAGE]
**Reviewer:** ________________
**Date:** [DATE]

---

## Mandatory Simplification Pass

*Review these items regardless of stage. Every review should leave the code simpler.*

- [ ] Dead code removed (commented-out code, unused functions)
- [ ] Duplicate logic consolidated (DRY principle applied)
- [ ] Magic numbers extracted to named constants
- [ ] Complex conditionals simplified (early returns, guard clauses)
- [ ] Unused includes/imports removed

---

## Stage 1: Foundation Review

*Focus: Project structure, build system, parameter setup*

- [ ] CMakeLists.txt follows project patterns (JUCE setup, targets)
- [ ] APVTS parameter setup correct (types, ranges, defaults)
- [ ] Plugin metadata accurate (name, manufacturer, format codes)
- [ ] Basic project structure in place (Source/, .planning/)
- [ ] Parameter IDs match ARCHITECTURE.md specification

---

## Stage 2: DSP Review

*Focus: Audio processing correctness and real-time safety*

- [ ] No memory allocations in processBlock (no new, malloc, vector resize)
- [ ] ScopedNoDenormals present at start of processBlock
- [ ] All parameters connected to DSP (no orphan parameters)
- [ ] Buffer handling safe (zero-length check, sample loop bounds)
- [ ] Signal flow matches ARCHITECTURE.md
- [ ] Smoothing applied to parameter changes (avoid zipper noise)
- [ ] Stereo/mono handling correct (respects channel count)

---

## Stage 3: GUI Review

*Focus: Thread safety, layout, visual consistency*

- [ ] Member declaration order correct (APVTS before components that use it)
- [ ] APVTS attachment patterns used correctly (attachments outlive components)
- [ ] No audio thread access from GUI (no direct processor state access)
- [ ] Responsive layout implemented (resizeable or fixed appropriate for design)
- [ ] Visual consistency with mockup (colors, spacing, typography)
- [ ] All controls functional and labeled
- [ ] Keyboard accessibility where appropriate

---

## Stage 4: Polish Review

*Focus: Production readiness, validation, documentation*

- [ ] pluginval strictness 10 passes (no crashes, no warnings)
- [ ] Presets functional (save/load state works correctly)
- [ ] Documentation complete (README with usage, parameter reference)
- [ ] Build artifacts verified (VST3 + AU both load in DAW)
- [ ] Final UX polish applied (tooltips, sensible defaults, visual refinement)
- [ ] Performance acceptable (CPU usage reasonable for feature set)
- [ ] No console warnings or errors in release build

---

## Reviewer Notes

*Free-form comments, suggestions, observations:*

```
[Notes go here]
```

---

## Verdict

Select one:

- [ ] **APPROVED** - Proceed to next stage. Code meets quality bar.
- [ ] **CHANGES_REQUESTED** - Address feedback before proceeding. Minor issues found.
- [ ] **BLOCKED** - Major issues require redesign or significant rework.

**Verdict rationale (required if not APPROVED):**

```
[Explain the issues and required changes]
```

---

*Review saved to: plugins/[PLUGIN]/.planning/stages/[STAGE]/review-notes.md*
