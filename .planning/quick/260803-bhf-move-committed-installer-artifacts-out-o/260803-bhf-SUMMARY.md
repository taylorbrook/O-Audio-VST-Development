---
phase: quick-260803-bhf
plan: 01
subsystem: repo-hygiene
tags: [public-release-readiness, gitignore, untracking, installer-artifacts, git]
status: complete
requires: []
provides:
  - "Zero tracked installer artifacts repo-wide"
  - "Ignore rules (*.pkg, *.dmg, *.msi) that make installer output non-re-addable"
  - "PUBLIC-RELEASE-READINESS.md step 12 ticked, section 4.3 resolved"
affects:
  - .gitignore
  - PUBLIC-RELEASE-READINESS.md
  - plugins/O-Polystutter/dist/
tech-stack:
  added: []
  patterns:
    - "Untrack AND ignore, ignore-rule first — the ordering that prevents the step-6 residual"
    - "Preservation gate proven by MD5 comparison before any git rm, not asserted"
key-files:
  created:
    - "~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/O-Polystutter-OuariconAudio.pkg"
    - "~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/PolyStutter.zip"
  modified:
    - .gitignore
    - PUBLIC-RELEASE-READINESS.md
  untracked:
    - plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg
    - plugins/O-Polystutter/dist/PolyStutter.zip
decisions:
  - "Untrack-only: both blobs kept on disk (git rm --cached), not deleted"
  - "Preservation satisfied by an external backup copy, not by git history"
  - "History deliberately left intact — the ~9.5 MB reclaim is step 14's decision"
  - "Directory-wide plugin-dist ignore rule considered and rejected"
metrics:
  duration: ~12min
  tasks: 3
  files_modified: 2
  files_untracked: 2
  completed: 2026-08-03
---

# Quick Task 260803-bhf: Move Committed Installer Artifacts Out of Git — Summary

Untracked the two O-Polystutter v1.8.0 installer blobs (9,498,890 B combined) after MD5-verified external preservation, added `*.pkg`/`*.dmg`/`*.msi` ignore rules **before** the removal so they cannot return, and closed readiness checklist step 12.

## What Was Done

Three atomic commits, in the plan's deliberate order — ignore rules land before untracking, so the blobs are never simultaneously untracked-and-unignored (the exact window that reopened step 6).

| # | Commit | Task |
|---|--------|------|
| 1 | `2cefd026` | Append installer/distribution ignore section to `.gitignore` (`*.pkg`, `*.dmg`, `*.msi`) |
| 2 | `ef817350` | Preserve both blobs externally, MD5-verify, then `git rm --cached` |
| 3 | `d8f4f19b` | Tick readiness step 12, resolve section 4.3, repair two stale `.gitignore` line citations |

## Working-Tree Disposition — Untrack-Only, Files Kept on Disk

`git rm --cached` was used on both blobs; neither was deleted from the working tree. Three reasons, as scoped:

1. **Precedent.** The three prior untracking steps (5, 6, 7) all untracked without deleting — `.claude/system-config.json`, `build-release/`, and the 31 build logs are all still on disk.
2. **Non-destructive.** Now that the files are also *ignored*, they are inert: `git check-ignore` resolves both, `git add -An` on the directory stages zero paths, and `git status --porcelain` on it is silent. Leaving them costs nothing and keeps the change fully reversible.
3. **Out of scope.** On-disk tidying of stale renders and artifacts is separately scoped to checklist **step 13**, which this task must not touch.

## Preservation Gate — Satisfied by the Backup, Not by History

Neither blob matches any published GitHub Release asset. The only published O-Polystutter release is `O-Polystutter-v1.12.4` (assets at 10,151,465 B and 4,822,035 B) — a different version at different sizes — and the local release backup held only `O-Polystutter-v1.12.2/` and `O-Polystutter-v1.12.4/`. So the external copy, not git history, is what makes the removal safe.

Both files were copied to `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/` and MD5-compared against the on-disk originals **before** `git rm` ran:

| File | Bytes | MD5 (original == backup) |
|------|-------|--------------------------|
| `O-Polystutter-OuariconAudio.pkg` | 4,756,409 | `c778e3de586f536d0972f54336de2463` |
| `PolyStutter.zip` | 4,742,481 | `705eae9b8dcf00cc6cf3d0c77e5ca857` |

Both matched. The on-disk MD5s were also re-asserted against the plan's verified-facts block at the start of Task 2 — a changed file would have halted the task before the destructive step.

## History Left Intact — By Design

This was a **checkout-only** change. No `git filter-repo`, no `git filter-branch`, no rewrite, no push, no force-push — `git reflog | grep -ciE 'filter-repo|filter-branch|push'` returns **0**.

The ~9.5 MB of blobs remains in `.git`, and both files stay recoverable by addressing the blob at its pre-removal commit. Per section 4.6's pivotal mechanic, removing files in a new commit shrinks the checkout but not `.git` — **the 912 MB did not move**. Reclaiming that space belongs to **step 14**, whose section 4.6 `filter-repo` proposal already lists `plugins/O-Polystutter/dist/`.

## The Rejected Directory-Wide Ignore Rule

A wildcard rule over every plugin's `dist` folder was considered and **deliberately rejected**. Those folders also hold hand-authored source:

- `plugins/O-Tremolo/dist/installer.iss` — the Inno Setup script
- `plugins/O-Tremolo/dist/install-readme-windows.txt`
- `plugins/O-Polystutter/dist/install-readme.txt`

A directory-wide rule would silently stop new installer scripts from being tracked at all. Matching on the built-artifact **extension** is the correct tool. This rationale is recorded in three places so it is not re-derived and reversed later: the `.gitignore` comment banner itself, section 4.3's resolution note, and here.

All three source files remain tracked and stageable — verified after the removal.

## Figure Correction in Section 4.3

The section's own headline number was wrong in a way that mattered, and was corrected in place while keeping the original inventory as the historical record:

- Only **two** of the three listed blobs were ever tracked. `O-Polystutter-by-TACHES.pkg` was already history-only at the assessment date — nothing to untrack for it.
- The tracked total was **9,498,890 bytes (~9.5 MB)**, not 13.5 MB.
- The two **overlap** rather than being independent: `PolyStutter.zip` contains a copy of the `.pkg` at exactly 4,756,409 bytes plus an install-readme. The pair is one v1.8.0 distribution package stored twice.

## Verification

All three per-task gates and all seven plan-level verification points passed.

| Check | Result |
|-------|--------|
| Tracked installer blobs repo-wide (`.pkg/.exe/.dmg/.msi/.zip/.tar.gz/.deb`) | **0** |
| Tracked-but-ignored census | **9**, all vendored `preset-manager.js` (was 10) |
| `git add -An` on both dist directories | stages **zero** paths |
| `git check-ignore -v` on removed paths | `.gitignore:252:*.pkg` and `.gitignore:202:*.zip` |
| Both blobs present on disk | yes |
| Hand-authored source still tracked | 3/3 |
| `.gitignore` diff deletions | **0** (pure append; `*.log` still 197, `*.zip` still 202) |
| `PUBLIC-RELEASE-READINESS.md` diff | 18 insertions, 5 deletions (budget ≤12) |
| Checklist state | **13 ticked / 4 open** (4, 11, 13, 14 untouched) |
| SAF gitlink | `b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af` — unchanged |
| Reflog rewrite/push evidence | **0** |
| Working tree | clean |

The census moving 10 → 9 with zero non-`preset-manager.js` entries is the single number proving both that the blob left tracking and that the three new rules shadowed nothing else.

## Deviations from Plan

**1. [Rule 3 — Blocking] Task 3's verification gate could not run as written on macOS bash 3.2**

- **Found during:** Task 3 verification.
- **Issue:** The gate's `[ "$(grep -cE "gitignore.{0,7}196" $F)" = 0 ]` construct nests double quotes inside `$( )` inside double quotes. bash 3.2 (the macOS system bash) mis-parses this and **brace-expands** `{0,7}`, splitting one grep into two — `gitignore.0196` and `gitignore.7196`. The substitution then yields `0 0`, and `[` aborts with `too many arguments`. A `bash -x` trace confirmed it.
- **Fix:** Re-authored the gate with single-quoted patterns and pre-computed variables. **No assertion was weakened or dropped** — every check in the plan's gate ran, plus per-check failure messages. The corrected counts (`196` → 0, `197` → 2) were also confirmed independently before the rewrite.
- **Files modified:** none in the repository — scratchpad harness only.
- **Scope:** harness quoting artifact, not a content failure. Worth knowing because the same construct appears in plan-authored gates generally and will fail the same way on any macOS default shell whenever a brace pattern is involved.

**2. Collateral citation repair (planned, noted for the record)**

The `*.log` line citations in section 3.4 and checklist step 7 both said line 196; the rule actually sits at line 197. This drift was **pre-existing** — introduced when step 6 inserted `build-release/` at line 67 — not caused by this task. Repaired as a citation-only change; neither item's status, wording, or tick state was altered. This is precisely why Task 1 appended to the end of `.gitignore` rather than inserting mid-file.

## Observation — Not Acted On (Out of Scope)

Checklist **step 11** still reads *"Must resolve **before** step 12, because it changes what a rewrite would need to strip."* Step 12 is now ticked while step 11 remains open, so that ordering note is stale. It was left verbatim because the scope boundary forbids modifying step 11. The stated dependency is about what a *rewrite* would strip, and no rewrite occurred here, so the sequencing concern did not actually bind — but the sentence will read oddly until step 11 is resolved. Worth a one-line touch-up whenever step 11 is next opened.

## Known Stubs

None.

## Threat Flags

None. No new network endpoints, auth paths, file-access patterns, or schema changes. The change reduces public surface by removing two multi-megabyte binaries from a public repository's tracked tree.

## Self-Check: PASSED

- `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/O-Polystutter-OuariconAudio.pkg` — FOUND, MD5 verified
- `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/PolyStutter.zip` — FOUND, MD5 verified
- `.gitignore` — FOUND, modified
- `PUBLIC-RELEASE-READINESS.md` — FOUND, modified
- Commit `2cefd026` — FOUND
- Commit `ef817350` — FOUND
- Commit `d8f4f19b` — FOUND
