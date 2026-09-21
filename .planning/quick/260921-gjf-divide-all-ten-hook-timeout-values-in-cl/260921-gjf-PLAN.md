---
phase: 260921-gjf
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/settings.json
autonomous: true
requirements:
  - QUICK-260921-gjf-01
quick_task: true

estimate:
  tokens: 18000
  raw_tokens: 12000
  tasks: 2
  confidence: low

must_haves:
  truths:
    - "All ten hook timeout values in .claude/settings.json are expressed in seconds, reading [5,5,2,5,10,10,3,15,30,10] in file order."
    - ".claude/settings.json remains valid JSON with unchanged key order, unchanged 2-space indentation, and integer (not string, not float) timeout values."
    - "No content outside the ten timeout values changed — the diff is exactly 10 insertions and 10 deletions in one file."
    - "The resulting commit touches only .claude/settings.json; the pre-existing unrelated modification to .claude/agent-memory/research-planning-agent.md remains uncommitted."
  artifacts:
    - ".claude/settings.json (ten integer timeout values, seconds)"
  key_links:
    - "hooks.*[].hooks[].timeout -> Claude Code hook runner (field unit is seconds, not milliseconds)"
    - "git index -> path-scoped commit that excludes .claude/agent-memory/research-planning-agent.md"
---

<objective>
Correct the unit error in the ten hook timeout values in `.claude/settings.json`. The Claude Code settings schema documents `hooks[].hooks[].timeout` in **seconds**, but this file carries millisecond-shaped values. The worst case, `SubagentStop.py` at `30000`, currently declares a 30000-second (8.3 hour) budget rather than 30 seconds — so a hung hook would never be reaped.

Purpose: restore the intended per-hook time budgets so a stalled hook is actually killed.
Output: `.claude/settings.json` with ten integer timeout values divided by 1000, committed path-scoped on `main`.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.claude/settings.json
</context>

<tasks>

<task type="auto">
  <name>Task 1: Divide the ten hook timeout values by 1000</name>
  <files>.claude/settings.json</files>
  <precondition>
    `jq -c '[.. | objects | select(has("timeout")) | .timeout]' .claude/settings.json` prints `[5000,5000,2000,5000,10000,10000,3000,15000,30000,10000]`. This repo is a shared trunk-based checkout with concurrent sessions, so another session may already have applied this edit. If the command prints the target array instead, the work is already done — skip to Task 2's commit check and report it. If it prints anything else, halt and report: the live file no longer matches the scope this plan was authorized against.
  </precondition>
  <action>
    Apply ten scoped `Edit` calls (or ten scoped replacements) to `.claude/settings.json`, one per `"timeout"` line, replacing each value with itself divided by 1000. Each `"timeout"` line is a unique target only in combination with the `"command"` line immediately above it — several share the same numeric value — so anchor every replacement on the preceding `"command"` line to keep it unique. Target lines, identified by the hook script on the preceding `"command"` line:

    | Hook script on preceding line | New value |
    |---|---|
    | `SessionStart.py` | 5 |
    | `PostCompact-SessionStart.py` | 5 |
    | `PostToolUse.py` | 2 |
    | `validators/validate-research-frontmatter.py` | 5 |
    | `regenerate-manifest.py` | 10 |
    | `PreCompact.py` | 10 |
    | `inject-agent-memory.py` | 3 |
    | `task-validator-dispatch.py` | 15 |
    | `SubagentStop.py` | 30 |
    | `write-back-agent-memory.py` | 10 |

    Do NOT reformat the file, reorder keys, reflow the JSON, or run it through a pretty-printer — a `jq` round-trip rewrites the whole file and defeats the 10-insertion diff gate. Preserve the 2-space indentation and write bare integers (`5`, not `"5"` and not `5.0`). Change nothing else: `$schema`, all `command` strings, all `matcher` strings, and `enabledPlugins` are untouched. Only `.claude/settings.json` is in scope — not `.claude/settings.local.json`, not `~/.claude/settings.json`.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; jq -e '[.. | objects | select(has("timeout")) | .timeout] == [5,5,2,5,10,10,3,15,30,10]' .claude/settings.json &gt;/dev/null &amp;&amp; jq -e '[.. | objects | select(has("timeout")) | .timeout | select(type != "number" or . != floor)] | length == 0' .claude/settings.json &gt;/dev/null &amp;&amp; NUMSTAT="$(git diff --numstat -- .claude/settings.json)" &amp;&amp; test "$NUMSTAT" = "$(printf '10\t10\t.claude/settings.json')" &amp;&amp; echo GATE_PASS</automated>
  </verify>
  <done>
    `jq` parses the file (so it is still valid JSON), the ten timeout values equal `[5,5,2,5,10,10,3,15,30,10]` in file order, every one is an integer rather than a string or float, `git diff --numstat` reports exactly `10	10` for this single file, and the verify command prints `GATE_PASS`.
  </done>
</task>

<task type="auto">
  <name>Task 2: Commit path-scoped, excluding the unrelated dirty file</name>
  <files>.claude/settings.json</files>
  <precondition>
    `git status --short` shows a pre-existing unrelated modification to `.claude/agent-memory/research-planning-agent.md` that is NOT part of this task and must not enter the commit.
  </precondition>
  <action>
    Immediately before committing — not from a snapshot taken earlier in the session — re-check location and staging with `git branch --show-current` (expect `main`) and `git status --short`. Another session sharing this checkout can stage into `.git/index` in the gap between a check and a commit, so this re-check is the only thing that makes the commit safe.

    Then commit with an explicit pathspec and no staging step:

    Then commit with an explicit pathspec. Argument order matters — everything after `--` is read as a pathspec, so `git commit -- <path> -m "msg"` fails with `error: pathspec '-m' did not match any file(s) known to git`. The message flag must come BEFORE the separator:

    `git commit -m "fix(claude): correct hook timeout unit from milliseconds to seconds" -- .claude/settings.json`

    `.claude/settings.json` is already tracked, so no `git add` is needed — a pathspec commit picks up working-tree changes to tracked files directly. Never `git add -A`, never `git commit -a`, never a bare `git commit`. If `git status --short` shows unexpected staged content from another session, do not commit — report and halt.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; BRANCH="$(git branch --show-current)" &amp;&amp; test "$BRANCH" = main &amp;&amp; FILES="$(git show --pretty=format: --name-only HEAD)" &amp;&amp; test "$(printf '%s\n' "$FILES" | grep -c .)" = "1" &amp;&amp; printf '%s\n' "$FILES" | grep -qx '\.claude/settings\.json' &amp;&amp; STAT="$(git status --short)" &amp;&amp; printf '%s\n' "$STAT" | grep -q 'research-planning-agent\.md' &amp;&amp; RESIDUAL="$(git diff -- .claude/settings.json)" &amp;&amp; test -z "$RESIDUAL" &amp;&amp; echo GATE_PASS</automated>
  </verify>
  <done>
    HEAD is on `main`. The tip commit changes exactly one file and that file is `.claude/settings.json`. The unrelated `research-planning-agent.md` modification is still present in the working tree as uncommitted (proving it was not swept in). `.claude/settings.json` has no remaining uncommitted diff. The verify command prints `GATE_PASS`.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| `.claude/settings.json` -> Claude Code hook runner | A local, developer-authored config file that grants each hook script a wall-clock execution budget. No untrusted input crosses this boundary; the file, the hook scripts, and the runner are all local and developer-controlled. |
| working tree -> shared `.git/index` | Concurrent sessions in this single checkout share the index and HEAD, so an unscoped commit can capture another session's staged work. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-gjf-01 | Denial of Service | `hooks.SubagentStop[].timeout` (and the other nine) | medium | mitigate | This is the defect being fixed, not one being introduced. A 30000-second budget means a hung `SubagentStop.py` is never reaped and blocks subagent teardown indefinitely. Task 1 restores 30 s. |
| T-gjf-02 | Denial of Service | hook scripts newly subject to a real ceiling | low | accept | Post-fix, a hook exceeding its budget is killed mid-run (e.g. `write-back-agent-memory.py` at 10 s could truncate a memory write). Accepted: these are local Python scripts doing small file I/O and complete in well under a second in practice, and the seconds-denominated values are the schema's documented intent. If a hook is later observed being killed, raise that individual value rather than reverting the unit. |
| T-gjf-03 | Tampering | shared `.git/index` | medium | mitigate | An unscoped `git add -A` / `git commit -a` would capture the unrelated in-flight `.claude/agent-memory/research-planning-agent.md` edit and any concurrent session's staging. Task 2 mandates a pathspec commit plus an immediately-preceding `git branch --show-current` and `git status --short`, and its gate asserts the tip commit contains exactly one file. |

No package-manager installs occur in this plan, so no supply-chain (`T-gjf-SC`) threat applies. ASVS L1, blocking threshold `high`: no threat is rated high or critical, so nothing blocks.
</threat_model>

<verification>
Run from the repo root after both tasks:

```bash
jq -c '[.. | objects | select(has("timeout")) | .timeout]' .claude/settings.json
# expect: [5,5,2,5,10,10,3,15,30,10]

git show --stat HEAD -- .claude/settings.json
# expect: 1 file changed, 10 insertions(+), 10 deletions(-)

git status --short
# expect: the pre-existing ` M .claude/agent-memory/research-planning-agent.md` line still present, nothing else unexpected
```

Note: the `jq` gate in Task 1 is a whole-array equality against a fixed expected sequence rather than a grep for absent digit strings, so it cannot be invalidated by comment or prose text elsewhere in the file, and it doubles as a JSON-validity check (a malformed file makes `jq` exit non-zero).
</verification>

<success_criteria>
- Ten timeout values read `[5,5,2,5,10,10,3,15,30,10]` in file order, each a bare integer.
- `.claude/settings.json` still parses as JSON with key order, indentation, and all non-timeout content byte-identical apart from the ten changed lines.
- `git diff --numstat` for the change was exactly `10	10` before commit.
- One path-scoped commit on `main` containing only `.claude/settings.json`.
- `.claude/agent-memory/research-planning-agent.md` remains uncommitted and unmodified by this work.
</success_criteria>

<output>
Create `.planning/quick/260921-gjf-divide-all-ten-hook-timeout-values-in-cl/260921-gjf-SUMMARY.md` when done.
</output>