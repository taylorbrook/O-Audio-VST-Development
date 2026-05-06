---
name: simplify-phase2
description: Apply MEDIUM-risk HIGH-severity candidates from a plugin's SIMPLIFICATION-AUDIT.md (Phase 2 of /simplify workflow)
argument-hint: "[PluginName]"
---

# /simplify-phase2

Phase 2 of the code-simplification workflow. Applies the **MEDIUM-risk HIGH-severity** candidates from `plugins/[PluginName]/.planning/SIMPLIFICATION-AUDIT.md` — the items that didn't fit Phase 1 because they require behaviour-stability proofs (format round-trips, visual smoke tests, CSS dependency checks) on top of compile + auval.

Phase 1 (LOW-risk HIGH items) is run via the regular `/improve` flow on a propose-only audit. Phase 2 carries more verification overhead, so it gets its own gate.

## Preconditions

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <check condition="plugin_exists">
      Plugin entry MUST exist in PLUGINS.md.
    </check>

    <check condition="status_in(✅ Working, 📦 Installed)">
      Status MUST be ✅ Working OR 📦 Installed (same gate as `/improve`).
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

  <phase1_completion required="recommended">
    Check git log for a recent commit matching `refactor(\\[PluginName\\]).*simplification.*Phase 1`.

    <on_missing>
      WARN (do not block):
      "No Phase 1 commit detected. Phase 2 candidates assume the LOW-risk
      HIGH items have already shipped. Continue anyway? (y/n)"
    </on_missing>
  </phase1_completion>
</preconditions>

## Workflow

### Step 1 — Parse the audit

Read `plugins/[PluginName]/.planning/SIMPLIFICATION-AUDIT.md` and extract every candidate where:

- Severity tag is `[HIGH-XX]`
- The candidate's `**Risk:**` line says `MEDIUM` or `HIGH` (not `LOW`)

These are the Phase 2 set. List them with their identifier, file:line, type, and risk in a numbered menu.

### Step 2 — Per-candidate confirmation

For each Phase 2 candidate:

1. Show the candidate body (Current / Proposed / Rationale / Test impact) to the user.
2. Ask: "Apply this candidate? (y/n/skip)"
3. Collect verification requirements — for each candidate, infer the verification checks from its Type and Test impact lines:
   - **Type: duplication** with a string-format risk → require a state save/load round-trip + a UI payload byte-compare
   - **Type: duplication** in HTML/CSS → require a visual smoke screenshot diff + selector-dependency check
   - **Type: verbose-pattern** in DSP / RT-safe code → require render-harness identity test
   - **Default** → render-harness + auval

Skip means do nothing for that candidate. The user can re-run `/simplify-phase2` later.

### Step 3 — Approval gate

Show the consolidated plan:

```
Phase 2 plan for [PluginName]:
- Apply: [list of candidate IDs the user approved]
- Skip:  [list of candidate IDs the user skipped]
- Version bump: PATCH (v[X.Y.Z] → v[X.Y.Z+1])
- Verification: [merged list of required checks]

Proceed? (y/n)
```

Wait for confirmation before any write.

### Step 4 — Hand off to plugin-improve

Invoke the `plugin-improve` skill via the Skill tool with:

- `pluginName`: [name from arg]
- `description`: A self-contained, fully-specified description that lists each approved candidate's ID, the exact files/lines, the proposed change, the LOAD-BEARING constraints to preserve (extracted from the audit), and the verification checks merged in Step 2. Use the same prose style as the description block that drove Phase 1 (see commit `a6e0a3b` for the template).
- Skip vagueness check — the audit + per-candidate confirmation IS the specification.

The handoff prompt MUST include:

1. **Per-candidate spec block** with file:line, current code shape, proposed code shape, and any "DO NOT change" constraints (e.g., "preserve hyphenated `one-shot` in JSON, underscore `one_shot` in XML — both are load-bearing").
2. **Verification protocol block** listing the checks merged from Step 2, ordered: (a) build, (b) auval, (c) internal CTest binaries, (d) format-stability check (if any), (e) visual smoke (if any).
3. **Reference to the audit's "Skipped (false-positive checks)" list** — these stay untouched.
4. **CHANGELOG entry template** matching the v1.16.7 entry shape: a `### Changed` section with one bullet per applied candidate, a `### Verification` section, and a final note pointing back to the audit file.

### Step 5 — Post-handoff

After the `plugin-improve` skill returns:

1. Move applied candidates' entries in the audit file from "Candidates" to a new `## Phase 2 Applied (v[X.Y.Z])` section, with the commit SHA.
2. Re-stage the audit + commit it as a follow-up: `docs([Plugin]): mark Phase 2 candidates as applied in SIMPLIFICATION-AUDIT.md`.
3. Present the standard improvement-completion menu (test in DAW, run /simplify-phase2 again with deferred items, move to MEDIUMs, stop).

## Constraints

- **Never apply a candidate the user said `skip` to.** Skipped items stay in the audit verbatim for a future pass.
- **Never apply LOW-severity audit items in Phase 2.** Those go through `/improve` directly when the user picks them.
- **Never bypass the per-candidate confirmation step.** Phase 2 candidates are MEDIUM-risk by definition — auto-applying them defeats the gate.
- **Never edit the "Skipped (false-positive checks)" list.** That list documents intentional non-targets.

## Notes

- The audit's `**Risk:**` field is the gating signal for Phase 2 inclusion. If a future audit splits HIGH severity into LOW/MED/HIGH risk further, this command still works — it picks any HIGH-severity item with non-LOW risk.
- If the audit has a `## Phase 2 Applied` section already, the command should subtract those from the candidate list before showing the menu.
- Headless plugins follow the same flow; there's no GUI-creation fork (that's `/improve`'s job).

## Integration Points

**Invoked by:**
- User typing `/simplify-phase2 [PluginName]`
- Follow-up suggestion from a completed Phase 1 `/improve` run
- The completion menu of a previous `/simplify-phase2` run (deferred candidates)

**Invokes:**
- `plugin-improve` skill (Phase 4: implementation onwards) — receives the Step 4 handoff prompt
- Optionally invokes `code-simplifier` agent in propose-only mode to re-audit before applying, if the audit is older than the most recent commit on the plugin's source tree (staleness check)

**Updates:**
- `plugins/[Plugin]/.planning/SIMPLIFICATION-AUDIT.md` (moves applied items to "Phase 2 Applied")
- Everything `plugin-improve` updates (CHANGELOG, CMakeLists VERSION, PLUGINS.md row, source files, backups, git tag)
