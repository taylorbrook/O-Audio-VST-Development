---
phase: quick-260730-vwx
plan: 01
subsystem: repo-governance
tags: [security-audit, licensing, repo-size, public-release, documentation]
requires: [260730-vwx-SCOUT.md]
provides: [PUBLIC-RELEASE-READINESS.md]
affects: []
tech-stack:
  added: []
  patterns: [scout-cited-assessment, gated-irreversible-commands]
key-files:
  created:
    - PUBLIC-RELEASE-READINESS.md
  modified: []
decisions:
  - "Numeric traceability outranks plan prose: the plan's 'six tag-pinned action references' was not a SCOUT-measured figure, so the document lists the references without asserting a total."
  - "Deferred the JUCE dual-license question to the user as an explicit decision with two branches rather than recommending a root license."
  - "Framed the history rewrite as optional throughout, because SCOUT S1 removes any security justification for it."
metrics:
  duration: ~4 minutes
  completed: 2026-07-31
  tasks: 2
  files: 1
status: complete
---

# Quick Task 260730-vwx: Public-Release Readiness Assessment Summary

Produced `PUBLIC-RELEASE-READINESS.md` — a 323-line, fully SCOUT-cited assessment separating the two genuine legal blockers to a public release from the hardening, size, and disclosure work that is optional or discretionary.

## What Was Built

A single decision-ready document at the repository root, in six sections:

| Section | Content |
|---|---|
| 1. Verdict | No secret ever committed (verified over full history) [S1]; the real blockers are the missing root LICENSE [L1] and undocumented samples shipped inside binaries [L4] |
| 2. Blockers | Root LICENSE [L1], sample provenance [L4], tracked `.claude/system-config.json` [S3], tracked `build-release/` with a compiled executable [E3] |
| 3. Security hardening | CI posture already fork-safe, three hardening actions [S4]; 354-file local-path disclosure [S2]; the 852-file `.claude/`+`.planning/` keep-or-strip decision [S5]; 31 tracked build logs [S6] |
| 4. Efficiency and size | 912 MB `.git` baseline, blob table, installers, root scratch, clone cost [E1] [E2] [E4] [E5] |
| 5. Legal and licensing | LICENSE gating [L1], JUCE dual-license branches [L2], attribution inventory [L3], provenance split [L4] |
| 6. Ordered checklist | 15 numbered items, each naming the section and SCOUT ID it discharges |

All 15 SCOUT IDs (S1–S6, L1–L4, E1–E5) are cited. Every measured figure traces to SCOUT — verified by a mechanical set-difference of all numeric tokens against the SCOUT document; the only residue was structural section numbers, checklist ordinals, and the `Apache-2.0` license identifier.

## Task Commits

| Task | Name | Commit | Files |
|---|---|---|---|
| 1 | Author PUBLIC-RELEASE-READINESS.md end-to-end (tracer) | `42e4348` | `PUBLIC-RELEASE-READINESS.md` |
| 2 | Traceability and safety audit, then commit | `42e4348` | (corrections folded into the same atomic commit) |

Task 1 wrote the document; Task 2's correction pass was applied before staging, so the plan's requirement that the commit touch exactly one file is satisfied by a single commit rather than two.

## Verification

Task 1 gate — PASS:
- File exists at repo root; 323 lines (≥200 required)
- All 15 bracketed SCOUT citations present
- Gate line `> **IRREVERSIBLE — decide before running.**` appears exactly twice (≥2 required)
- Six `##` top-level sections

Task 2 gate — PASS:
- `git log -1` subject begins `docs(quick-260730-vwx)`
- `git show --name-only HEAD` lists exactly one file: `PUBLIC-RELEASE-READINESS.md`
- Post-commit deletion check: no deletions
- `git status --porcelain` after commit is byte-identical to the pre-task state apart from the removed untracked artifact entry

Manual audit checks:
- Both gate lines sit directly above their fenced blocks (lines 226→228 for `git filter-repo`, 310→312 for `gh repo edit`). The only other `filter-repo` mention is prose in checklist item 14, which needs no gate line.
- No emoji (Unicode scan clean). No first-person plural.
- No text implies the repo is already public or that any file has been removed.
- Checklist ordering holds: rewrite is item 14 (last local change), visibility flip is item 15 (final).

## Hard Prohibitions Honoured

Nothing was executed. Confirmed not run: repo visibility change, `gh repo edit`, `git filter-repo`/BFG, `git rebase`, `git reset --hard`, `git rm`/`git rm --cached`, `git mv`. No existing file was created, modified, moved, or deleted — `.gitignore`, the CI workflow, `README.md`, and everything under `.claude/` and `.planning/` are untouched. No `LICENSE` or `THIRD-PARTY-NOTICES.md` was created. Every command in the document is text for the user to read and later decide on.

The four pre-existing dirty working-tree entries (`.claude/agent-memory/dorico-agent.md`, `.claude/frontmatter-issues.txt`, `.claude/resource-index.json`, `.claude/commands/improve-review-info.md`, `scratch-pv/`) were not staged and remain exactly as found.

## Deviations from Plan

**1. [Rule 1 — Correctness] Dropped the plan's "six tag-pinned action references" count**
- **Found during:** Task 2 numeric traceability pass
- **Issue:** The plan's section 3 instruction says to "SHA-pin the six tag-pinned action references." SCOUT records four distinct action references with usage multipliers (`checkout@v4` ×2, `upload-artifact@v4` ×3, `download-artifact@v4`, `action-gh-release@v2`) but never states a total of six. Under the plan's own source-of-truth rule, any number not traceable to SCOUT must be corrected or deleted.
- **Fix:** The document lists the references in a table with SCOUT's own multipliers and asserts no total, satisfying "listing them" without introducing an unmeasured figure.
- **Commit:** `42e4348`

**2. [Rule 1 — Correctness] Removed an inferred `×1` usage count**
- **Found during:** Task 2 audit
- **Issue:** The first draft rendered `download-artifact@v4` and `action-gh-release@v2` as `×1`. SCOUT annotates neither with a multiplier, so `×1` was an inference rather than a measurement.
- **Fix:** Replaced both with `—` and retitled the column "Uses recorded."
- **Commit:** `42e4348`

**3. [Rule 2 — Accuracy] Softened an unverified claim about JUCE commercial terms**
- **Found during:** Task 2 "no invented findings" check
- **Issue:** Branch A of the JUCE licensing discussion asserted that commercial JUCE terms "typically restrict redistributing JUCE source itself." That is a claim about a document neither SCOUT nor this executor has seen.
- **Fix:** Reworded to direct the user to read their own agreement's redistribution clause, with an explicit statement that this document has not seen the agreement and does not assert what it permits.
- **Commit:** `42e4348`

## Known Stubs

None. The document is complete as specified; its open items are decisions deliberately left to the user (JUCE license branch, `.claude/`/`.planning/` disclosure, whether to rewrite history at all), not unfinished work.

## Threat Flags

None. This plan created one Markdown document and introduced no network endpoint, auth path, file-access pattern, or schema change.

## Self-Check: PASSED

- `PUBLIC-RELEASE-READINESS.md` — FOUND (323 lines)
- Commit `42e4348` — FOUND in `git log --all`
- Commit touches exactly one file — CONFIRMED
