---
phase: quick-260803-c4n
plan: 01
subsystem: repo-hygiene
status: complete
tags: [public-release-readiness, repo-root, gitignore, preservation-gate, provenance-correction]
requires: []
provides:
  - readiness-step-13-closed
  - readiness-section-4.4-resolved
  - o-bowed-golden-relocated-and-ignored
affects:
  - PUBLIC-RELEASE-READINESS.md
  - .gitignore
tech-stack:
  added: []
  patterns:
    - "ignore-rule-before-move (step 6 window designed out, second application after 260803-bhf)"
    - "archive-then-MD5-verify-then-delete (preservation gate proven, not assumed)"
key-files:
  created:
    - ~/VST-development-root-scratch-20260803/README.txt
    - plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav
  modified:
    - .gitignore
    - PUBLIC-RELEASE-READINESS.md
decisions:
  - "Root e1-max-sustain pair ARCHIVED not deleted — a tracked RESEARCH.md cites it, even though that citation is stale"
  - "RESEARCH.md deliberately NOT edited — historical stage document; correction recorded in the readiness doc instead"
  - "Ignore rule scoped to one exact path, not a directory wildcard — O-Contrabass tracks 19 goldens in the sibling directory"
  - ".DS_Store is the only outright deletion in the plan; every other disposition is reversible"
  - "backups/ and logs/ considered and ruled OUT OF SCOPE, recorded explicitly in §4.4"
metrics:
  duration: ~18min
  tasks: 3
  files_modified: 2
  files_relocated: 1
  files_archived: 77
  files_deleted_outright: 1
  completed: 2026-08-03
---

# Quick Task 260803-c4n: Tidy the Repo Root (Readiness Step 13) Summary

Closed readiness checklist step 13 by clearing ~21 MB of untracked scratch from the repository root — relocating the one load-bearing file in the pile, MD5-verifying every other file into a dated external archive before deleting anything, and landing the ignore rule for the relocated golden **before** the move so it was never simultaneously present and unignored.

## What Happened

Both HALT gates in Task 1 cleared in the safe direction, so no re-planning was needed:

- **Golden sha256 gate:** root `o-bowed-pre-extraction-canonical.wav` = `93124fb8dd8223caafac5948c988a226230363d79a17323d386e9a1db34c8891`, matching the tracked `.sha256` sibling exactly. Relocation premise holds.
- **e1 divergence gate:** root wav md5 `ba8d5b37ba5fc613b00dbe313c4f00e5` vs plugin-local `d585110e4f7738a15aaca397c17c0fe1`. Divergent — a distinct superseded run, not a redundant copy. Archive-not-delete premise holds.

### Dispositions executed

| Item | Size | Disposition | Destination |
|---|---|---|---|
| `o-bowed-pre-extraction-canonical.wav` | 1,323,104 B | **RELOCATED in-repo** | `plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav` |
| `e1-max-sustain.wav` + `.json` | 17,199,104 B + 687 B | ARCHIVED | `contrabass-e1-superseded-run/` |
| `o-bowed-pre-extraction-canonical.json` | 472 B | ARCHIVED | `o-bowed-harness-json/` |
| `mbc-v150.png`, `o-reversedelay-484.png`, `tooltip-knob.png` | ~856 KB | ARCHIVED | `screenshots/` |
| `after-header.png`, `after-tips-off.png`, `before-full.png` | ~523 KB | ARCHIVED | `screenshots/` |
| `scratch-pv/` (6 text logs) | 56 KB | ARCHIVED | `scratch-pv/` |
| `.playwright-mcp/` (63 files) | 2.2 MB | ARCHIVED | `playwright-mcp/` |
| `.DS_Store` | 6,148 B | **DELETED outright** | — (only item with no preservation copy) |
| `backups/` | 1.3 GB | OUT OF SCOPE — untouched | — |
| `logs/` | 9.8 MB | OUT OF SCOPE — untouched | — |

Archive root: `~/VST-development-root-scratch-20260803/` (20 MB, 5 subdirectories + `README.txt`).

**Preservation gate: 77 of 77 files MD5-matched** against their originals before a single original was removed.

### Ordering — step 6's window designed out

The `.gitignore` rule landed and was **committed** (`54bc7a35`) before the wav moved. This mattered specifically because the destination is a directory where renders *are* legitimately tracked — O-Contrabass keeps 19 of its own — so a stray wav there looks native and would have been swept up by one `git add -A` into a now-public repo. Post-move sha256 re-verified byte-exact.

The rule at `.gitignore:68` names one exact path, never a directory wildcard, with the reasoning written into the comment (mirrors the installer block's rejected-wildcard precedent from 260803-bhf).

### Record correction landed in §4.4

`plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md:1390` cites "`e1-max-sustain.json` (project root)" as its numerical evidence, quoting `rmsMid_s5_s6 = 0.0353`. **That parenthetical is wrong.** Measured:

| Field | ROOT (archived) | PLUGIN-LOCAL (tracked) |
|---|---|---|
| `peak` | 0.099262863397598 | 0.068542242050171 |
| `rmsMid_s5_s6` | 0.027670025825500 | 0.035618029534817 |
| `rmsFinal_lastSecond` | 0.006334509700537 | 0.012674955651164 |
| `rmsRatio_final_over_mid` | 0.228930383920670 | 0.355857849121094 |

`VERIFICATION.md:68` reproduces "peak 0.0683, rmsMid 0.0353, rmsFinal 0.0127, ratio 0.36" — the plugin-local pair matches on `rmsFinal` and `rmsRatio` to the digit and on `peak`/`rmsMid` to within re-run drift; the root pair matches on **no field at all**. The real evidence artifact is the tracked plugin-local pair, which is unaffected. `RESEARCH.md` was deliberately **not** edited — historical stage document; the correction belongs in the readiness doc.

## Verification

All ten of the plan's verification points pass:

| # | Check | Result |
|---|---|---|
| 1 | Root free of scratch renders/screenshots/`.DS_Store`/`scratch-pv`/`.playwright-mcp` | clean |
| 2 | Golden sha256 == tracked sha256 sibling | `93124fb8…c8891` both |
| 3 | `git check-ignore -v` on destination | `.gitignore:68` |
| 4 | `git add -An .` stages zero paths (scoped, see deviation) | 0 outside this task's docs dir |
| 5 | tracked-but-ignored census / tracked wav total | **9** / **31** |
| 6 | O-Contrabass golden tracked files | **70** (19 wav) |
| 7 | Exactly two new commits, one file each | `54bc7a35` `.gitignore`, `ad4321ef` readiness doc |
| 8 | reflog: no push, filter-repo, filter-branch, force | clean |
| 9 | SAF submodule gitlink | `b6fe1882…` unchanged (v1.3.4) |
| 10 | Archive populated | README.txt + 5 subdirectories |

Task gates: `GATE-PASS` (Task 1), `ROOT-CLEAN` (Task 2), `DOC-OK` (Task 3).

Checklist now **14 ticked / 3 open** (4, 11, 14).

## Deviations from Plan

**1. [Rule 3 — measurement] `git add -An .` gate scoped to exclude this task's own docs artifact**

- **Found during:** Task 1 baseline capture
- **Issue:** The plan's gate requires `git add -An . | wc -l` == 0, citing finding #1's pre-planning measurement. It measured **1**: `.planning/quick/260803-c4n-tidy-the-repo-root-per-public-release-re/260803-c4n-PLAN.md` — this quick task's own PLAN.md, created by the orchestrator *after* finding #1 was taken.
- **Fix:** Used the scoped form `git add -An . | grep -v '\.planning/quick/260803-c4n' | wc -l`, which measured **0 both before and after** the tidy. This preserves the gate's stated intent (per the plan's own `key_links`: "the tidy added no new committable surface") while excluding an out-of-scope artifact that predates the tidy and is reserved for the orchestrator's docs commit.
- **Files modified:** none
- **Commit:** n/a (verification method only)

**2. [No action needed] No checklist tally exists in the document to update**

- **Found during:** Task 3
- **Issue:** The plan instructs "update the checklist tally wherever the document states it… grep for the count rather than assuming where it appears." Grepping found **zero** numeric tally statements in `PUBLIC-RELEASE-READINESS.md`. The 13-ticked/4-open figure lives only in `STATE.md`.
- **Fix:** Nothing to change in the doc. Box counts verified directly instead: 14 ticked / 3 open, matching the plan's gate exactly. `STATE.md` is the orchestrator's to update.
- **Commit:** n/a

**3. [Precondition, noted not blocking] Working tree was not strictly empty at Task 1**

- `git status --porcelain` showed one entry: `?? .planning/quick/260803-c4n-…/`, this task's own plan directory. Not uncommitted work at risk of being archived over, so execution proceeded. Same root cause as deviation 1.

No Rule 1, 2, or 4 deviations. No architectural decisions required. No package installs.

## Out-of-Scope Observation (report only, per plan `non_scope_observation`)

**§4.2 states "32 `.wav` files are tracked in total"; the measured count is 31.** That figure belongs to step 14's section and went stale independently of this work — it was **not** edited here. One line for whoever opens step 14.

Note also that §4.2's own bullet already flags `plugins/O-Contrabass/e1-max-sustain.wav` (16.4 MB) as "a stray scratch render that was committed into a plugin folder." This task's §4.4 correction establishes that **that** file is in fact the cited evidence artifact for `RESEARCH.md:1390` / `VERIFICATION.md:68` — worth reading together before step 14 includes it in a `filter-repo` path list. It is currently listed in §4.6's rewrite proposal.

## Known Stubs

None. No code was written; both modified files are configuration/documentation and are complete as committed.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern, or schema change was introduced. The plan's own threat register was addressed in full:

| Threat ID | Mitigation delivered |
|---|---|
| T-c4n-01 | Ignore rule committed before the move; `git check-ignore -q` passes; scoped `git add -An` stages zero |
| T-c4n-02 | sha256 verified before AND after relocation; file moved, never deleted |
| T-c4n-03 | 77/77 MD5-verified into dated archive with provenance README before any removal |
| T-c4n-04 | Rule scoped to one path; O-Contrabass 70/19 and census 9 all held constant |
| T-c4n-05 | Every figure in §4.4 came from Task 1/Task 2 recorded output; gate greps the measured values |

## Self-Check: PASSED

- `plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav` — FOUND
- `~/VST-development-root-scratch-20260803/README.txt` — FOUND
- `~/VST-development-root-scratch-20260803/contrabass-e1-superseded-run/` (wav + json) — FOUND
- `~/VST-development-root-scratch-20260803/screenshots/` (6 PNGs) — FOUND
- `~/VST-development-root-scratch-20260803/scratch-pv/` (6 logs) — FOUND
- `~/VST-development-root-scratch-20260803/o-bowed-harness-json/` — FOUND
- `~/VST-development-root-scratch-20260803/playwright-mcp/` — FOUND
- `.gitignore` — MODIFIED, committed
- `PUBLIC-RELEASE-READINESS.md` — MODIFIED, committed
- Commit `54bc7a35` — FOUND
- Commit `ad4321ef` — FOUND
