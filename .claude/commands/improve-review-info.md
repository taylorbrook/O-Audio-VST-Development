---
name: improve-review-info
description: Sweep the Info tier (IN-*) of a plugin's CODE_REVIEW.md — the opt-out findings /improve-review skips (high-volume, low-risk cleanup)
argument-hint: "[PluginName] [findings? e.g. IN-01..03]"
---

# /improve-review-info

The Info-tier companion to `/improve-review`. Sweeps the **IN-\*** findings in
`plugins/[PluginName]/CODE_REVIEW.md` — the ones `/improve-review` deliberately leaves
behind, because its mandatory step 4 makes Info findings opt-in and its recommended
default scope is CR-\* + WR-\* only.

Those findings accumulate. They are individually cheap and collectively the reason a
header's stated contract, a CSS budget comment and a `NOTES.md` entry can each describe
a different release. This command exists so clearing them is one gate rather than six
arguments to `/improve-review`.

**What Info findings actually are**, and why the batching below is by *kind* and not by
severity — every one of these is real, drawn from shipped reviews:

- Dead state (`age` written and never read; `envLastCurve` assigned and never read)
- A class-contract comment three releases stale (window size, native-fn count, relay counts)
- Two contradictory budget comments in one file, with `NOTES.md` mirroring the wrong one
- A documented behaviour that does not exist (a DPR redraw whose binding is dead)
- A check-then-write sentinel that does not close the race it documents
- Two functions disagreeing about whether a pointer can be null

Only the last three can change behaviour. The rest are comment and dead-code work whose
whole risk is the compiler's.

## Preconditions

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <check condition="plugin_exists">
      Plugin entry MUST exist in PLUGINS.md.
    </check>
    <check condition="status_in(✅ Working, 📦 Installed)">
      Status MUST be ✅ Working OR 📦 Installed (same gate as `/improve-review`).
      <on_violation status="🚧 In Development">
        REJECT: "[PluginName] is still in development. Finish it with /continue
        [PluginName] before sweeping review findings."
      </on_violation>
      <on_violation status="[any other status]">
        REJECT: "[PluginName] has status '[Status]'. Only ✅ Working or 📦 Installed
        plugins can be review-resolved."
      </on_violation>
    </check>
  </status_verification>

  <review_present target="plugins/[PluginName]/CODE_REVIEW.md" required="true">
    <on_missing>
      REJECT: "No CODE_REVIEW.md found for [PluginName]. Generate one first
      (/gsd-code-review), then re-run /improve-review-info."
    </on_missing>
  </review_present>

  <higher_tier_progression required="recommended">
    Check whether any CR-* or WR-* findings in the review are still unresolved (verify
    against the source, not against the CHANGELOG — see Step 1).

    <on_unresolved_critical>
      WARN (do not block):
      "[N] Critical and [M] Warning findings look unresolved. Info findings are cosmetic;
      shipping them first spends a version number without touching a defect. Run
      /improve-review [PluginName] first? (y/n)"
    </on_unresolved_critical>
  </higher_tier_progression>

  <version_check required="true">
    4-way version check BEFORE any edit — CMakeLists `VERSION`, CHANGELOG top entry,
    PLUGINS.md row, and `git tag -l "*[PluginName]*" | tail -1`. All four must agree.

    <on_mismatch>
      STOP and report the divergence. An uncommitted improve-version has been lost this
      way before (`pattern_uncommitted_improve_versions_lost`); do not bump on top of a
      gap.
    </on_mismatch>
  </version_check>
</preconditions>

## Routing

**If no plugin name provided:**
1. `ls plugins/*/CODE_REVIEW.md` → candidate list.
2. For each, read the front-matter `findings:` block and show only the Info count:
   `1. O-Foo (6 info) — 2026-07-07`. Skip plugins with zero IN-* findings.
3. Wait for selection.

**If plugin name only:** parse all IN-* findings and proceed to Step 1.

**If plugin name + findings filter** (`IN-01..03`, `IN-02, IN-05`, `all`): parse to an
explicit ID set, then Step 1. Reject any non-IN-\* ID with: "That's a [CR/WR] finding —
use /improve-review [PluginName] [ID] for the defect tiers."

## Workflow

### Step 1 — Verify each finding against current source

**Do not trust the report.** For every selected IN-\*, read the CURRENT source at the
cited `file:line` and confirm the issue still exists. Info findings are the most likely
tier to be stale: their line numbers drift with every release, and some are resolved as
side-effects of the CR/WR pass that preceded them.

Classify each as:

- **LIVE** — reproduced at (possibly shifted) coordinates. Record the *current* line.
- **ALREADY FIXED** — report it and drop it. Do not re-apply.
- **STALE PREMISE** — the cited code changed shape enough that the finding's reasoning no
  longer holds. Report what it says versus what is there, and ask before acting.

A finding whose fix is "correct this number" needs its **correct value derived, not
copied from the other comment** — see the Constraints.

### Step 2 — Batch by kind

- **Batch A — comment / documentation only.** Stale contracts, contradictory budgets,
  wrong counts. Zero runtime risk. Default-bulk-approve.
- **Batch B — dead code removal.** Unread fields and their writes, unreachable helpers.
  The compiler proves the removal; risk is that the state was not actually dead.
  Default-bulk-approve, but grep the whole repo (including `tests/`) for each symbol
  first, not just `Source/`.
- **Batch C — behaviour-touching.** Anything that changes what runs: wiring up a
  documented-but-absent behaviour, reordering a sentinel write, changing a null-guard
  posture. **Per-item gate, no bulk approve.**

Omit empty batches. Display with IDs, one-line summaries and current file:line.

### Step 3 — Surface the genuine forks BEFORE applying

Info findings disproportionately offer two legitimate resolutions, and the review usually
states both without choosing. Wherever that is true, stop and use **AskUserQuestion**,
leading with a recommendation. Do not silently pick. The recurring shapes:

| Fork | Options |
|------|---------|
| A documented behaviour that does not exist | Wire it up (MINOR-ish, new runtime behaviour) **vs** delete the dead binding and the half of the comment that promises it (PATCH) |
| Two contradictory comments | Keep the measured one and delete the other **vs** replace both with an assertion in a check script |
| A guard posture disagreement | Add the missing guards **vs** assert once at construction and drop the guards, so both sites read the same way |
| A race a sentinel does not close | Stamp before the work (accepts a non-retried interrupted pass) **vs** `InterProcessLock` **vs** document and leave |

### Step 4 — Approval gate

```
Info sweep for [PluginName] (v[X.Y.Z] → v[X.Y.Z+1], PATCH):

Batch A — comments/docs (N):     IN-02, IN-03  → apply all? (y / pick / skip)
Batch B — dead code (N):         IN-01, IN-04  → apply all? (y / pick / skip)
Batch C — behaviour (N, gated):  IN-05 — [summary] — apply? (y/n/skip)
                                 IN-06 — [summary] — apply? (y/n/skip)

Already fixed (dropped):         [IDs + why]
Stale premise (needs a call):    [IDs + what changed]
```

Wait for confirmation before any write.

### Step 5 — Hand off to plugin-improve

Invoke the `plugin-improve` skill with a self-contained description listing each approved
finding: ID, **current** file:line, the change shape, the chosen fork resolution, and any
LOAD-BEARING constraint to preserve. Skip the vagueness check — the review plus the
batch approval IS the specification.

The handoff MUST state:

- **Bump PATCH.** Info findings never justify MINOR. If a chosen fork adds real runtime
  behaviour (the "wire it up" arm), flag it and re-confirm the bump.
- **Verification** — build, `auval -v <type> <subtype> <mfr>`, plus the plugin's own
  gates: the render harness (if `tests/render-harness/` exists) and every
  `tests/*.js` check. A comment-only sweep must still leave all of them green.
- **Atomic commit:** `docs([Plugin]): v[X.Y.Z] — Info-tier review sweep (N findings)`,
  or `refactor(...)` if Batch B or C is non-empty.
- **Tag** `O-<Plugin>-v<X.Y.Z>` — never a bare `vX.Y.Z` (bare tags collide across plugins).

### Step 6 — Post-sweep

1. Annotate `CODE_REVIEW.md`: append `**Resolved in v[X.Y.Z]**` to each applied finding's
   heading, and add a `## Resolved` roll-up at the foot with the commit SHA. Leave skipped
   findings verbatim.
2. Update the CHANGELOG's `### Notes` deferred-findings list from the previous release —
   the entries just cleared must stop being listed as open.
3. If every IN-\* is now resolved and no CR/WR remain, offer to delete `CODE_REVIEW.md`
   or leave it as the record (default: leave).
4. Present: test in DAW · re-review the plugin · next plugin · stop.

## Constraints

- **A number in a comment gets measured, never copied.** Where two comments disagree, the
  fix is not "keep the one the review preferred" — it is to derive the value from the
  rules as written (or render the page and measure the box) and then write *that*. The
  contradiction exists because someone previously copied instead of measuring.
- **Prefer an assertion to a corrected comment.** If the finding's own text says "better:
  add the measurement to a check", do that — a comment cannot fail the build, and a
  budget written down twice will diverge a third time
  (`pattern_test_fixture_mirrors_drift_silently`).
- **Never write an unmeasured number into a comment or a test threshold.** If a claim
  needs a figure, produce the figure first. Estimates written into comments as fact have
  shipped wrong before.
- **Any check added by this sweep must be seen to FAIL.** Temporarily revert the fix,
  confirm the new assertion catches it, restore. A check that has never failed is not a
  guard — a crest-factor probe once passed on the very code it was written to reject.
- **Dead-code removal greps the whole repo**, including `tests/`, the render harness and
  any `.js` check. A field read only by a test is not dead.
- **Never touch CR-\* or WR-\* findings here.** Those go through `/improve-review`, which
  has the defect-tier verification depth.
- **Idempotent.** Never re-apply a finding already marked `**Resolved in v…**`.
- **Behaviour-touching Info findings still get the full gate.** IN-\* is a severity label,
  not a permission slip: a sentinel reorder makes an interrupted migration permanent, and
  that deserves the same build + auval + harness pass as a Warning.

## Notes

- Runs cleanly with zero LIVE findings: report "No remaining Info findings" and exit
  without bumping a version.
- The batching is by *kind* rather than severity because Info is already a single severity
  — the only useful axis left is whether the compiler can prove the change safe.
- Headless plugins follow the same flow, minus the `tests/*.js` checks.

## Integration Points

**Invoked by:** the user, or the completion menu of an `/improve-review` run that deferred
the Info tier.

**Invokes:** `plugin-improve` (backup / version / build / changelog machinery).

**Updates:** `CODE_REVIEW.md` (resolution annotations), plus everything `plugin-improve`
touches — CHANGELOG, `CMakeLists` VERSION, PLUGINS.md row, NOTES.md, source, backups, tag.

## Relationship to /improve-review

| Command | Tier | Volume | Gate |
|---------|------|--------|------|
| `/improve-review` | CR-\* + WR-\* | 1–10 | Per-finding, defect-depth verification |
| `/improve-review-info` | IN-\* | 3–15 | Batched by kind; per-item only for behaviour-touching |
