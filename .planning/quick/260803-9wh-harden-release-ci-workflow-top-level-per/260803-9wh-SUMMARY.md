---
phase: quick-260803-9wh
plan: 01
subsystem: ci
status: complete
tags: [ci, github-actions, supply-chain, security-hardening, yaml]
requires: []
provides:
  - "release workflow with least-privilege default GITHUB_TOKEN scope"
  - "8 SHA-pinned third-party action references"
  - "standing anti-fork-trigger rule in the workflow header"
affects:
  - .github/workflows/build-and-release.yml
  - PUBLIC-RELEASE-READINESS.md
tech-stack:
  added: []
  patterns:
    - "SHA-pin third-party actions; trailing `# vN (vN.N.N)` comment records both the major tag and the precise release"
    - "top-level read-only permissions default, widened only by job-level override"
key-files:
  created: []
  modified:
    - .github/workflows/build-and-release.yml
    - PUBLIC-RELEASE-READINESS.md
decisions:
  - "Kept the section 3.1 table indented inside its ordered-list item rather than de-indenting it to satisfy a broken gate regex — the plan forbade restructuring, and de-indenting would terminate the list."
  - "Did not re-resolve the four action SHAs; used the orchestrator-verified values verbatim as the plan required."
metrics:
  duration: ~9 min
  completed: 2026-08-03
  tasks: 2
  commits: 2
  files_changed: 2
---

# Quick Task 260803-9wh: Harden Release CI Workflow Summary

Pinned all 8 third-party GitHub Actions to commit SHAs, added a top-level `contents: read` token default, and recorded a standing rule in the workflow header forbidding fork-reachable triggers while it holds eight Apple signing secrets — then reconciled PUBLIC-RELEASE-READINESS.md section 3.1, whose action-reference counts were wrong about the file they described.

## What Was Built

**Task 1 — `.github/workflows/build-and-release.yml`** (commit `72d656cd`)

Three edits, one atomic commit:

- **Standing trigger rule (readiness step 10).** A set-off block appended to the file's existing header comment, so it is the first thing an editor reads. Names `pull_request_target` and the secrets-bearing `pull_request` case as permanently forbidden, lists all eight Apple signing secrets by name, and states the stake — a leak means signed, notarised malware under the Ouaricon identity, which revoking a token cannot undo. Cites PUBLIC-RELEASE-READINESS.md section 3.1 as the source and explains the SHA-pinning rationale in the same breath.
- **Top-level `permissions: contents: read` (step 8).** Inserted at column 0 between `on:` and `env:`, with a comment pointing at `create-release` as the intended override path.
- **8 SHA pins (step 9).** Every reference replaced with a 40-hex commit SHA plus a `# vN (vN.N.N)` trailing comment:

| Action | SHA | Comment | Refs |
|---|---|---|---|
| `actions/checkout` | `11d5960a326750d5838078e36cf38b85af677262` | `# v4 (v4.4.0)` | 3 |
| `actions/upload-artifact` | `ea165f8d65b6e75b540449e92b4886f43607fa02` | `# v4 (v4.6.2)` | 3 |
| `actions/download-artifact` | `d3f86a106a0bac45b974a628896c90dbdf5c8093` | `# v4 (v4.3.0)` | 1 |
| `softprops/action-gh-release` | `3bb12739c298aeb8a4eeaf626c5b8d85266b0e65` | `# v2 (v2.6.2)` | 1 |

SHAs were used verbatim from the orchestrator's verified list; no re-resolution was attempted, per the plan.

**Task 2 — `PUBLIC-RELEASE-READINESS.md`** (commit `43cff844`)

- Section 3.1 table corrected from `×2 / ×3 / — / —` to `×3 / ×3 / ×1 / ×1`, with a dated note that all eight are now SHA-pinned and that the comments carry both tag and precise release.
- Checklist items 8, 9, 10 flipped to done in the document's existing convention (struck-through bold title, `✅ **Done 2026-08-03**`, one-line record, section-and-severity reference preserved). Items 4, 5, 6, 7, 11, 12, 13, 14 left untouched and open.

## Verification Results

All gates were run for real. **YAML parser used: PyYAML 6.0.3** (present as the plan stated; the `ruby -ryaml` fallback was not needed).

**Task 1 Gate D structural output — pasted verbatim as the plan's `<output>` requires:**

```
structure OK — 4 jobs
triggers: ['push', 'workflow_dispatch']
dispatch inputs: ['plugin_name', 'version', 'validate_only']
top-level permissions: {'contents': 'read'}
create-release permissions: {'contents': 'write'}
env: {'JUCE_VERSION': '8.0.14', 'BUILD_TYPE': 'Release'}
```

| Gate | Result |
|---|---|
| T1-A zero mutable-tag references | PASS — grep exit 1, no hits |
| T1-B exactly 8 SHA pins with comment | PASS — returned `8` |
| T1-C header names `pull_request_target` | PASS — line 13 |
| T1-C header names `pull_request` (word) | PASS — line 16 |
| T1-C header states signing rationale | PASS — lines 19, 23, 24 |
| T1-D YAML parse + structure | PASS — see block above |
| T1-E diff scope | PASS — 43 insertions, 8 deletions; all 8 deletions are `uses:` lines; `≤ 8` holds |
| T2-A table counts 3/3/1/1 | PASS **after gate fix** — see Deviations |
| T2-B three items ticked | PASS — returned `3` |
| T2-C eight items still open | PASS — returned `8` |
| T2-D additive edit | PASS — literal gate returned `3`; true deletion count is `6`; both `≤ 8` |

**Final end-to-end verification (plan `<verification>` 1–6):**

1. Zero mutable-tag references — PASS (grep exit 1).
2. SHA-pin count — PASS, returned `8`.
3. YAML parse + structural assertions — PASS via PyYAML 6.0.3.
4. `git log --oneline -2` — PASS, exactly two commits, workflow first:
   ```
   43cff844 docs: record CI hardening — readiness steps 8-10 done, section 3.1 counts corrected
   72d656cd ci: harden release workflow — read-only default token scope, SHA-pinned actions, standing trigger rule
   ```
5. `git status --porcelain` — clean apart from the untracked `.planning/quick/260803-9wh-.../` directory (the materialized plan; the orchestrator owns the docs commit). Note: the plan anticipated an untracked `build-release/`, which is not present in this worktree because it was never tracked.
6. `git diff --name-only HEAD~2` — PASS, exactly:
   ```
   .github/workflows/build-and-release.yml
   PUBLIC-RELEASE-READINESS.md
   ```

No plugin build, ninja, cmake, auval, or cache-clearing was run — correctly, as this was a YAML + Markdown task.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] Task 2 Gate A regex could never match the table it validates**

- **Found during:** Task 2, before committing.
- **Issue:** The plan's Gate A used `re.search(r'^\|\s*`…`\s*\|…', t, re.M)`, anchoring the row at column 0. The section 3.1 table is indented **3 spaces** because it sits inside ordered-list item 2. Run as written, the gate aborts with `AssertionError: row missing: actions/checkout@v4` regardless of the cell contents — it was a broken gate, not a wrong document.
- **Evidence:** Ran the plan's regex verbatim first; it failed as described. `cat -et` confirmed the 3-space leading indent on all four rows.
- **Fix:** Corrected the gate regex to `^[ \t]*\|` (allow leading whitespace) and re-ran. Deliberately did **not** de-indent the table — the plan forbids restructuring it, and de-indenting would terminate the enclosing ordered list and change the rendered document.
- **Files modified:** None in the repo; the fix was to the verification script only.
- **Commit:** n/a (gate-only).

**2. [Rule 1 — Observation, no code change] Task 2 Gate D undercounts markdown list deletions**

- **Issue:** `grep -cE '^-[^-]'` returned `3`, but the real deletion count is `6`. Markdown list-item deletions appear in a diff as `-- [ ] **8. …` — diff marker `-` followed by the list's own `-` — so `[^-]` excludes them.
- **Impact:** None on the outcome; both the literal reading (3) and the true count (6) satisfy `≤ 8`. Recorded so the number is not mistaken for evidence that only three lines changed. The 6 deletions are exactly the 3 rewritten table rows and the 3 rewritten checklist items — verified line by line, nothing unrelated was dropped.
- **Commit:** n/a.

No Rule 2, 3, or 4 deviations. No architectural decisions were required. No authentication gates were hit. No package installs were attempted.

## Threat Mitigations Applied

| Threat ID | Disposition | Status |
|---|---|---|
| T-9wh-01 Tampering — mutable-tag action refs | mitigate | **Done.** 8/8 pinned; Gate A confirms zero mutable-tag refs remain. |
| T-9wh-02 EoP — default `GITHUB_TOKEN` scope | mitigate | **Done.** Top-level `contents: read`; `create-release` write override verified intact by Gate D. |
| T-9wh-03 Info disclosure — secrets vs. fork-PR trigger | mitigate | **Done, with the plan's residual risk standing.** The control is documentary only — a future editor can still add the trigger. No technical enforcement (ruleset / required review) was added; that remains out of scope and unmitigated. |
| T-9wh-04 Tampering — the pin SHAs themselves | accept | Accepted per plan; SHAs used verbatim, not re-resolved. |

## Threat Flags

None. No new network endpoint, auth path, file-access pattern, or schema change was introduced — the change strictly narrows token scope and freezes third-party code at reviewed commits.

## Known Stubs

None. No stub, placeholder, skipped test, or unrun verification remains. Every gate in the plan was executed and its real output is recorded above.

## Follow-ups

- **Not done by design:** readiness items 4, 5, 6, 7, 11, 12, 13, 14 remain open and untouched.
- **Residual from T-9wh-03:** the anti-fork-trigger rule has no technical enforcement. If that matters, a branch ruleset requiring review on `.github/workflows/**` would close it.
- **Pin maintenance:** the four SHAs are frozen at their 2026-08-03 values. They will need periodic refresh; the `# vN (vN.N.N)` comments exist so a future maintainer can see exactly what each pin corresponded to.
- Note: **8 Apple signing secrets are still not present on the public repo** — `/publish` remains blocked there. Unchanged by this task.

## Self-Check: PASSED

- `.github/workflows/build-and-release.yml` — FOUND (29,390 bytes)
- `PUBLIC-RELEASE-READINESS.md` — FOUND (37,832 bytes)
- Commit `72d656cd` — FOUND in `git log --all`
- Commit `43cff844` — FOUND in `git log --all`
- `git diff --name-only HEAD~2` lists exactly the two expected files — CONFIRMED
