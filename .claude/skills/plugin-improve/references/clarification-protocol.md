# Clarification Protocol (Phase 0.3)

**Purpose**: Ask 4 targeted questions using inline menus when request is specific enough but needs details.

**When to use**: Request passed Phase 0 specificity detection (has feature name, action, acceptance criteria) but needs 1-2 clarifications.

---

## Question 1: What to Change

```
What needs to change?

1. Fix a bug - Something is broken or behaving incorrectly
2. Add a feature - New capability or enhancement
3. Improve existing behavior - Refine how something works
4. Other

Choose (1-4): _
```

**Purpose**: Understand change type for version bump logic.

---

## Question 2: Scope

```
How extensive is this change?

1. Single function/method - Localized change in one place
2. Multiple related components - Changes across a few files
3. System-wide change - Affects architecture or many components
4. Other

Choose (1-4): _
```

**Purpose**: Determine investigation tier (Tier 1/2/3) automatically.

---

## Question 3: Version Bump Priority

```
Version bump priority?

1. PATCH (bug fix) - Backward compatible fix, increment 0.0.X
2. MINOR (feature) - New feature, backward compatible, increment 0.X.0
3. MAJOR (breaking) - Breaking change, incompatible API, increment X.0.0
4. Other

Choose (1-4): _
```

**Purpose**: Pre-determine version bump (Phase 1 will confirm).

---

## Question 4: Testing Approach

```
Should I run regression tests?

1. Yes, full regression suite - Compare new behavior vs baseline backup
2. Yes, but manual review if failures - Show me what changed and let me decide
3. No, skip regression tests - Just build and verify compilation
4. Other

Choose (1-4): _
```

**Purpose**: Configure Phase 5.5 regression testing behavior.

---

## After Collection

**Collect all responses before proceeding to Phase 0.4 (Decision Gate).**

Phase 0.4 will summarize all answers and ask for confirmation before continuing to Phase 0.45 (Research Detection).
