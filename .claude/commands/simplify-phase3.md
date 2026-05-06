---
name: simplify-phase3
description: Sweep MEDIUM and LOW tier candidates from a plugin's SIMPLIFICATION-AUDIT.md (Phase 3 of /simplify workflow — high-volume, low-risk cleanup)
argument-hint: "[PluginName]"
---

# /simplify-phase3

Phase 3 of the code-simplification workflow. Sweeps the **MEDIUM-severity** and **LOW-severity** candidates from `plugins/[PluginName]/.planning/SIMPLIFICATION-AUDIT.md` after Phases 1 and 2 have shipped.

Phase 1 (LOW-risk HIGH items via `/improve`) and Phase 2 (MEDIUM-risk HIGH items via `/simplify-phase2`) carry one or two heavyweight candidates with significant verification overhead. Phase 3 handles the long tail — typically ~10–20 small wins. The gate shifts from "prove behaviour preservation" to "build + auval + spot-check the changed sites" with a per-item carve-out only for any MEDIUM-risk stragglers.

## Preconditions

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <check condition="plugin_exists">
      Plugin entry MUST exist in PLUGINS.md.
    </check>

    <check condition="status_in(✅ Working, 📦 Installed)">
      Status MUST be ✅ Working OR 📦 Installed (same gate as `/improve` and `/simplify-phase2`).
    </check>
  </status_verification>

  <audit_artifact required="true">
    File `plugins/[PluginName]/.planning/SIMPLIFICATION-AUDIT.md` MUST exist.

    <on_violation>
      REJECT with message:
      "No simplification audit found for [PluginName].
      Run /improve [PluginName] code simplification audit first to generate one."
    </on_violation>
  </audit_artifact>

  <phase_progression required="recommended">
    Check git log for recent commits matching:
    - `refactor(\\[PluginName\\]).*simplification.*Phase 1` (or equivalent)
    - `refactor(\\[PluginName\\]).*Phase 2` (or equivalent)

    <on_phase1_missing>
      WARN (do not block):
      "No Phase 1 commit detected. Phase 3 candidates assume the LOW-risk
      HIGH items have already shipped. Continue anyway? (y/n)"
    </on_phase1_missing>

    <on_phase2_missing>
      WARN (do not block):
      "No Phase 2 commit detected. Some MEDIUM-risk HIGH items may still be
      pending. Phase 3 will skip them (they're HIGH severity, out of scope).
      Continue anyway? (y/n)"
    </on_phase2_missing>
  </phase_progression>

  <staleness_check required="recommended">
    Compare audit file mtime vs. most recent commit on the plugin's source tree
    (`plugins/[PluginName]/Source/`, `Resources/`).

    <on_stale>
      WARN (do not block):
      "Audit is older than the most recent source commit. Some candidates
      may reference shifted line numbers or already-resolved code. Re-audit
      before proceeding? (y/n)"

      If yes → invoke `code-simplifier` agent in propose-only mode to
      regenerate the audit, then re-enter Step 1.
    </on_stale>
  </staleness_check>
</preconditions>

## Workflow

### Step 1 — Parse the audit

Read `plugins/[PluginName]/.planning/SIMPLIFICATION-AUDIT.md` and extract every candidate where:

- Severity tag is `[MEDIUM-XX]` OR `[LOW-XX]`
- The candidate is NOT already listed under a `## Phase 1 Applied`, `## Phase 2 Applied`, or `## Phase 3 Applied` section

Group the matched candidates into three batches by **risk** (not severity):

- **Batch A — LOW-severity items.** Always cosmetic or nit-tier. Default-bulk-approve.
- **Batch B — MEDIUM-severity, LOW-risk items.** Small structural dedups. Default-bulk-approve.
- **Batch C — MEDIUM-severity, MEDIUM- or HIGH-risk items.** Per-item gate (same as Phase 2's pattern). Examples: dialog-modal lifecycle helpers, native-fn invocation wrappers that touch many call sites.

If any batch is empty, omit it from the menu.

### Step 2 — Tiered approval

Display the parsed batches with counts, IDs, and one-line descriptions:

```
Phase 3 sweep for [PluginName]:

Batch A — LOW severity (N items, ~M LOC saved):
  • LOW-01: [one-line summary]
  • LOW-02: [...]
  → Apply all? (y / pick / skip)

Batch B — MEDIUM severity, LOW risk (N items, ~M LOC saved):
  • MEDIUM-01: [...]
  • MEDIUM-02: [...]
  → Apply all? (y / pick / skip)

Batch C — MEDIUM severity, MEDIUM-risk (N items — per-candidate gate):
  • MEDIUM-04: [...] — Apply? (y/n/skip)
  • [...]
```

**Bulk responses (Batches A and B):**

- `y` → apply every item in the batch
- `pick` → fall through to a per-item y/n/skip menu for that batch (same shape as Batch C)
- `skip` → skip the entire batch (items remain in audit verbatim)

**Per-item responses (Batch C, or Batch A/B in `pick` mode):**

- `y` → apply
- `n` / `skip` → leave in audit untouched
- `defer` → tag for a future Phase 3 run; included in audit's `## Phase 3 Deferred` section

### Step 3 — Verification protocol inference

For each approved candidate, infer the verification check set from its `Type` and `Test impact` lines:

| Type / Risk | Required checks |
|------------|-----------------|
| `Type: dead-code` | build + auval (compiler proves equivalence) |
| `Type: stale-comment` / `verbose-pattern` (LOW risk) | build + auval + grep that target string is gone |
| `Type: duplication` in DSP / RT-safe code | build + auval + render-harness identity test |
| `Type: duplication` in HTML/CSS | build + auval + visual smoke + CSS selector dependency check |
| `Type: duplication` with format-string risk | build + auval + format-stability grep (both surfaces) |
| Dialog/modal lifecycle (MEDIUM risk) | build + auval + manual smoke of every affected dialog |
| Native-fn wrapper across N call sites | build + auval + spot-check 3 representative call sites in standalone |

Merge the inferred checks across all approved candidates into one consolidated list. De-dupe.

### Step 4 — Approval gate

Show the consolidated plan:

```
Phase 3 plan for [PluginName]:
- Apply: [list of candidate IDs the user approved, grouped by batch]
- Skip:  [list]
- Defer: [list]
- Version bump: PATCH (v[X.Y.Z] → v[X.Y.Z+1])
- Verification: [merged list of required checks]

Estimated LOC delta: -[N] (sum of audit "LOC saved" estimates for approved items)

Proceed? (y/n)
```

Wait for confirmation before any write.

### Step 5 — Hand off to plugin-improve

Invoke the `plugin-improve` skill via the Skill tool with:

- `pluginName`: [name from arg]
- `description`: A self-contained, fully-specified description that:
  1. **Lists each approved candidate** with ID, file:line (re-verified for staleness — line numbers may have shifted since audit), the proposed change shape, and any LOAD-BEARING constraints to preserve.
  2. **Includes a verification protocol block** ordered: (a) build, (b) auval, (c) internal CTest binaries, (d) per-candidate-specific spot checks (greps, format-stability, visual smoke, manual modal test).
  3. **References the audit's "Skipped (false-positive checks)" list** — these stay untouched. Phase 3 must not touch any item on that list.
  4. **Specifies the CHANGELOG entry shape** — a single `### Changed` section summarising the sweep ("Phase 3 sweep — N MEDIUM/LOW candidates applied"), with a sub-bullet per applied candidate (ID + one-line summary), a `### Verification` section, and a final note pointing back to the audit.

Skip the vagueness check — the audit + tiered approval IS the specification.

The handoff prompt MUST also state:

- **Bump PATCH** (Phase 3 is the last step of a refactor sweep — never breaks anything user-visible).
- **Atomic commit** with message: `refactor([Plugin]): v[X.Y.Z] — Phase 3 sweep (N candidates from MEDIUM/LOW tier)`.

### Step 6 — Post-handoff

After `plugin-improve` returns:

1. Move applied candidates' entries from the Candidates section to a new `## Phase 3 Applied (v[X.Y.Z])` section, with the implementing commit SHA, grouped by batch (A/B/C). Use the same sub-section format as `/simplify-phase2`'s "Phase 2 Applied" output.
2. Move deferred candidates to a new `## Phase 3 Deferred` section (or extend if it exists).
3. Re-stage the audit + commit it as a follow-up: `docs([Plugin]): mark Phase 3 candidates as applied in SIMPLIFICATION-AUDIT.md`.
4. Present the standard improvement-completion menu:
   - Test in DAW
   - Re-audit the plugin (some candidates may have been resolved as side-effects)
   - Review deferred items (if any)
   - Move on to next plugin
   - Stop

## Constraints

- **Never apply a candidate the user said `skip` to.** Skipped items stay in the audit verbatim for a future pass.
- **Never apply HIGH-severity items in Phase 3.** Those go through `/improve` (LOW risk) or `/simplify-phase2` (MEDIUM risk) — different gates with different verification depth.
- **Never apply items already listed in any `## Phase X Applied` section.** Idempotency.
- **Bulk-approve is the default for low-risk batches**, but per-candidate confirmation is REQUIRED for any item with `**Risk:** MEDIUM` or higher. Auto-approving MEDIUM-risk items defeats the gate.
- **Never edit the "Skipped (false-positive checks)" list.** That list documents intentional non-targets.
- **Stale line numbers are NOT a blocker.** Re-verify line numbers as the first step inside the plugin-improve handoff (audit was generated at an earlier version; line drift across Phase 1/2 is expected). The handoff prompt must include the *current* line numbers, not the audit's.

## Notes

- The 3-batch grouping is the key UX difference from Phase 2. Phase 2's per-item gate works for 1–2 candidates; for 10+ items it produces survey fatigue and the user clicks `y` mechanically. Bulk-approve with `pick` as an escape hatch keeps the gate honest.
- If a future audit splits MEDIUM severity further (e.g., MEDIUM-LOW vs MEDIUM-HIGH), the Type/Risk grouping in Step 1 still works — Batch B widens to absorb LOW-risk MEDIUMs.
- Headless plugins follow the same flow.
- If the audit has zero remaining MEDIUM/LOW items (everything already applied or skipped), report "No remaining Phase 3 candidates" and exit cleanly.

## Integration Points

**Invoked by:**
- User typing `/simplify-phase3 [PluginName]`
- Follow-up suggestion from a completed `/simplify-phase2` run
- The completion menu of a previous `/simplify-phase3` run (for deferred candidates or post-re-audit)

**Invokes:**
- `plugin-improve` skill (Phase 4: implementation onwards) — receives the Step 5 handoff prompt
- Optionally invokes `code-simplifier` agent in propose-only mode for staleness re-audit before applying

**Updates:**
- `plugins/[Plugin]/.planning/SIMPLIFICATION-AUDIT.md` (moves applied items to "Phase 3 Applied", deferred items to "Phase 3 Deferred")
- Everything `plugin-improve` updates (CHANGELOG, CMakeLists VERSION, PLUGINS.md row, source files, backups, git tag)

## Relationship to Phases 1 and 2

| Phase | Severity | Risk | Volume | UX |
|-------|----------|------|--------|----|
| Phase 1 (`/improve` on audit) | HIGH | LOW | 1–6 items | Inline propose-and-apply |
| Phase 2 (`/simplify-phase2`) | HIGH | MEDIUM/HIGH | 1–4 items | Per-candidate gate |
| Phase 3 (`/simplify-phase3`) | MEDIUM + LOW | mostly LOW | 5–20 items | Tiered batch approval |

After Phase 3 completes, the only audit items remaining should be:
- Items the user explicitly skipped or deferred
- The "Skipped (false-positive checks)" list
- Any candidates that became stale and were reclassified during a re-audit

Phase 3 is intentionally the final pass. If a fourth pass is needed, it's a sign the audit needs regeneration, not extension.
