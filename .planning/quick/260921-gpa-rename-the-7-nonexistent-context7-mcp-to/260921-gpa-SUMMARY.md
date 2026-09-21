---
phase: quick-260921-gpa
plan: 01
subsystem: infra
tags: [claude-code, mcp, context7, agent-config, yaml-frontmatter]

requires: []
provides:
  - "Six agent definitions under .claude/agents/ whose context7 tool allowlists name only tools the live MCP server actually exposes"
  - "A self-sufficient context7 toolset on troubleshoot-agent.md (resolve-library-id + query-docs)"
affects: [agent-dispatch, documentation-lookup, troubleshooting, research-planning]

actuals:
  tokens: 49000
  tasks: 3
  commits: 1
  plan_head_before: 3c8cc76ec75bf7fa2316aefc4860e280fa53a089

tech-stack:
  added: []
  patterns:
    - "context7 allowlists are declared as the resolve-then-query pair; a fetch verb is never declared without a resolver"

key-files:
  created: []
  modified:
    - .claude/agents/research-planning-agent.md
    - .claude/agents/dorico-agent.md
    - .claude/agents/dsp-agent.md
    - .claude/agents/gui-agent.md
    - .claude/agents/foundation-shell-agent.md
    - .claude/agents/troubleshoot-agent.md

key-decisions:
  - "Applied scope addition SA-01: troubleshoot-agent.md gains mcp__context7__resolve-library-id, because query-docs requires a resolved libraryId and that file declared no resolver at all"
  - "Single path-scoped commit per the plan's commit structure rather than per-task commits, since Task 3's verify asserts HEAD touches exactly 6 files"
  - "Edit tool used for all 7 edits; no sed -i or Python bulk rewriter, per the recorded CRLF-rewrite trap"

patterns-established:
  - "Declared-but-unusable tool is the same defect class as a dead tool name: renaming a fetch verb requires checking its precondition verb is also declared"

requirements-completed: [QT-gpa-01, QT-gpa-02, QT-gpa-03]

coverage:
  - id: D1
    description: "All 7 occurrences of the two nonexistent context7 tool names removed from .claude/agents/ and replaced with mcp__context7__query-docs"
    requirement: "QT-gpa-01"
    verification:
      - kind: other
        ref: "grep -rn \"get-library-docs\\|search_juce_docs\" .claude/agents/ -> no output, exit 1"
        status: pass
      - kind: other
        ref: "grep -o 'mcp__context7__query-docs' -r .claude/agents/ | wc -l -> 7"
        status: pass
    human_judgment: false
  - id: D2
    description: "troubleshoot-agent.md declares a self-sufficient context7 toolset (resolve-library-id + query-docs) — scope addition SA-01"
    requirement: "QT-gpa-02"
    verification:
      - kind: other
        ref: "grep -c 'mcp__context7__resolve-library-id' .claude/agents/troubleshoot-agent.md -> 1; repo-wide resolve count 6 -> 7"
        status: pass
    human_judgment: false
  - id: D3
    description: "YAML frontmatter integrity preserved — tools: key remains a single line at line 4 in all six edited files"
    requirement: "QT-gpa-03"
    verification:
      - kind: other
        ref: "grep -n '^tools: ' <six files> -> six lines, every one at :4:"
        status: pass
    human_judgment: false
  - id: D4
    description: "One path-scoped commit touching exactly the 6 agent files; the unrelated .claude/agent-memory/research-planning-agent.md modification stayed out of it and is still uncommitted"
    requirement: "QT-gpa-03"
    verification:
      - kind: other
        ref: "git show --stat HEAD -> 6 files, all .claude/agents/; git status --short -> only ' M .claude/agent-memory/research-planning-agent.md'"
        status: pass
    human_judgment: false

duration: 2min
completed: 2026-09-21
status: complete
---

# Quick Task 260921-gpa: Dead context7 Tool Names in Agent Allowlists Summary

**Six agent definitions were silently losing documentation access at dispatch time because their frontmatter declared context7 tools the live server no longer exposes — all 7 occurrences renamed to `mcp__context7__query-docs`, and troubleshoot-agent additionally given the resolver its new fetch verb requires.**

## Performance

- **Duration:** 2 min (94 s)
- **Started:** 2026-09-21T19:06:14Z
- **Completed:** 2026-09-21T19:07:48Z
- **Tasks:** 3
- **Files modified:** 6

## Occurrence Counts (before -> after, scoped to `.claude/agents/`)

| Token | Before | After | Note |
|---|---|---|---|
| Dead names (`get-library-docs` + `search_juce_docs`) | 7 | **0** | matches the plan's live-observed baseline exactly |
| `mcp__context7__query-docs` | 0 | **7** | 6 frontmatter entries + 1 body-prose line |
| `mcp__context7__resolve-library-id` | 6 | **7** | the single increment is SA-01 on troubleshoot-agent.md |

Per-file `mcp__context7__query-docs` counts after the change:

| File | Count |
|---|---|
| `.claude/agents/dorico-agent.md` | 1 |
| `.claude/agents/dsp-agent.md` | 1 |
| `.claude/agents/gui-agent.md` | 1 |
| `.claude/agents/foundation-shell-agent.md` | 1 |
| `.claude/agents/troubleshoot-agent.md` | 1 |
| `.claude/agents/research-planning-agent.md` | 2 |

The one-for-one substitution has no over- or under-reach: 7 live-observed hits, 7 replacements.

## Accomplishments

- Restored working documentation lookup for the dsp, foundation-shell, dorico, research-planning, gui, and troubleshoot agents. Each previously declared a tool name the live context7 server does not expose, so the grant resolved to nothing at dispatch.
- Fixed the one body-prose occurrence (`research-planning-agent.md:765`) alongside its own line-4 allowlist entry, so the agent's documented call shape and its permissions no longer contradict each other.
- Closed the declared-but-unusable gap on troubleshoot-agent.md (SA-01, detailed below).
- Preserved YAML frontmatter integrity — the failure mode where a stray newline on line 4 silently strips an agent's entire toolset, which no functional test in this repo would catch.

## Task Commits

The plan specifies a single path-scoped commit in Task 3, whose verify asserts HEAD touches exactly 6 files. Tasks 1 and 2 were therefore verified independently but committed together rather than atomically — committing Task 1 separately would have made HEAD touch 1 file and failed the plan's own Task 3 verification.

1. **Tasks 1-3 (all edits + commit hygiene)** — `088c02a5` (fix)

**Plan metadata:** handled by the orchestrator (not committed here).

## Files Created/Modified

- `.claude/agents/research-planning-agent.md` — line 4 allowlist entry and line 765 body-prose fetch-verb reference both renamed (the only file carrying both occurrence layers)
- `.claude/agents/dorico-agent.md` — line 4 allowlist entry renamed
- `.claude/agents/dsp-agent.md` — line 4 allowlist entry renamed
- `.claude/agents/gui-agent.md` — line 4 allowlist entry renamed
- `.claude/agents/foundation-shell-agent.md` — line 4 allowlist entry renamed
- `.claude/agents/troubleshoot-agent.md` — line 4 dead entry replaced with the two-entry `resolve-library-id, query-docs` sequence (rename + SA-01)

## SCOPE ADDITION SA-01 — APPLIED (flagged for developer awareness)

**SA-01 was applied**, as approved. `.claude/agents/troubleshoot-agent.md:4` gained `mcp__context7__resolve-library-id` in addition to the literal rename.

Why it was necessary: that file declared `mcp__context7__search_juce_docs` and **no resolver at all**. The replacement tool `query-docs` takes a resolved `libraryId` (`/org/project`) as input — its contract requires `resolve-library-id` to be called first unless the caller already holds a literal library ID. The troubleshoot-agent body (Level 2, lines 141-178) describes only a generic "Query Context7 with JUCE library search" and never names or supplies a literal library ID. A literal-only rename on that file would therefore have produced a declared-but-unusable tool — the same defect class this task existed to remove.

Cost and reversibility: one token in one allowlist, granting a read-only library-name-to-ID lookup against an MCP server the agent is already permitted to call. The agent already holds `WebSearch` and `WebFetch`, which are strictly broader network reach. Reversible in a one-token Edit with no downstream state. The repo-wide `resolve-library-id` count moved 6 -> 7, and that single increment is the entire footprint of SA-01 — Task 2's verify pinned it, so no additional grant could slip in unnoticed.

The literal-7-only alternative remains available: delete `mcp__context7__resolve-library-id, ` from line 4 of troubleshoot-agent.md. The other six occurrences are unaffected by that choice.

## Commit Hygiene (QT-gpa-03)

- Commit `088c02a5` touches exactly 6 files, every one under `.claude/agents/`.
- `.claude/agent-memory/research-planning-agent.md` — the unrelated pre-existing working-tree modification — **stayed out of the commit and is still an uncommitted modification**, byte-unchanged by this run. It was never staged. The near-collision was handled by spelling all six paths out in full rather than relying on a basename glob: the excluded file and one edited file share the basename `research-planning-agent.md` and differ only in directory (`agent-memory/` vs `agents/`).
- Branch is still `main`. No branch and no worktree were created.
- `git branch --show-current` and `git status --short` were re-read immediately before the commit — not at plan start — per the shared-index rule. Status at that moment showed exactly the 6 intended edits plus the agent-memory file, with nothing staged by another session.
- Committed via explicit pathspec (`git commit -m ... -- <6 paths>`). No `git add -A`, no `git commit -a`.

## Decisions Made

- **SA-01 applied** (see above) — the resolver is a precondition of the renamed fetch verb, not an unrelated capability grant.
- **Single commit rather than per-task commits** — the plan's Task 3 verify asserts `git show --name-only HEAD` yields exactly 6 files, which per-task commits would contradict. Followed the plan's stated commit structure.
- **Edit tool for all 7 edits** — no `sed -i` (BSD sed on macOS needs `-i ''`) and no Python rewriter (recorded repo trap: Python text I/O silently rewrites CRLF line endings). Seven targeted Edit calls carry no rewrite risk.
- **Committed on `main` despite the generic protected-branch guard.** `.planning/config.json` does not set `git.allow_default_branch_commits`, so the stock GSD guard would treat `main` as protected. Project CLAUDE.md mandates trunk-based development on `main` and explicitly forbids a branch or worktree for single-scope work; CLAUDE.md takes precedence, and recent history confirms every quick task commits directly to `main`. Noted rather than silently bypassed.

## Deviations from Plan

None — plan executed exactly as written. SA-01 is not a deviation: the plan's Task 2 mandates it in the action body and the orchestrator approved it in advance. It is recorded above as a scope addition relative to the literal 7 renames, as required.

No generic "Context7" product-name prose was rewritten. The body-prose survey performed at planning time was trusted and not re-audited: `research-planning-agent.md:765` was the only body-prose tool-name occurrence, and the surviving descriptor on that line ("with resolved library ID") remains accurate for `query-docs`, so only the backticked tool name inside it changed. Lines 226, 228, 231, 236, 760, 762, 764, 821, 959 of that file and lines 141, 147, 172, 247, 398, 567, 587, 600, 717 of troubleshoot-agent.md were left untouched. `.claude/agents/research-lead.md` (tools key at line 5, no context7 tools) was correctly excluded from both the edits and the line-4 assertions.

## Issues Encountered

None. All three task verifications passed on first run, as did the plan's four-clause final verification block.

## Known Stubs

None. This change is a name substitution in markdown frontmatter; it introduces no placeholder values, no unwired components, and no TODO markers.

## Verification Evidence

1. `grep -rn "get-library-docs\|search_juce_docs" .claude/agents/` -> no output, exit 1. **PASS**
2. `grep -o 'mcp__context7__query-docs' -r .claude/agents/ | wc -l` -> `7`. `grep -o 'mcp__context7__resolve-library-id' -r .claude/agents/ | wc -l` -> `7`. **PASS**
3. `grep -n '^tools: '` across the six edited files -> six lines, every one reporting `:4:`; `research-planning-agent.md` additionally confirmed to carry the key exactly once. **PASS**
4. `git show --stat HEAD` -> 6 files, all under `.claude/agents/`, none matching `agent-memory`; `git status --short` -> only ` M .claude/agent-memory/research-planning-agent.md`; `git branch --show-current` -> `main`. **PASS**

## Self-Check: PASSED

- All six modified files verified present on disk and present in commit `088c02a5`.
- Commit `088c02a5` verified to exist via `git show --stat HEAD`.
- `.planning/quick/260921-gpa-rename-the-7-nonexistent-context7-mcp-to/260921-gpa-SUMMARY.md` written.

## Next Phase Readiness

- The six agents can now reach context7 documentation when dispatched. No rebuild, reinstall, or DAW restart is involved — agent frontmatter is read at dispatch time.
- No blockers. Nothing downstream depends on this change.
- Worth noting for future agent authoring: MCP tool names in allowlists are unvalidated strings. A renamed or removed server-side tool degrades silently — the agent loads and runs, it simply cannot call the tool. A periodic grep of declared `mcp__*` names against the live server's registry is the only thing that catches this class.

---
*Quick task: 260921-gpa*
*Completed: 2026-09-21*
