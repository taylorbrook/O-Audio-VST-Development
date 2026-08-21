---
phase: quick-260821-aym
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/hooks/SessionStart.py
  - CLAUDE.md
  - .claude/skills/context-resume/SKILL.md
  - .claude/commands/continue.md
  - .claude/worktrees/
autonomous: true
requirements: [SCOPE-1, SCOPE-2, SCOPE-3, SCOPE-4]

estimate:
  tokens: 52000
  raw_tokens: 26000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "A fresh session (after /clear) sees current branch, worktree count, dirty/staged counts, unpushed count, and in-flight plugins on its first screen without asking (SCOPE-2)."
    - "A non-main branch or a second worktree is surfaced as a WARN on stdout, not silently reported or routed to stderr (SCOPE-2)."
    - "SessionStart.py still prints every pre-existing dependency-validation line and still exits 0 even when run outside a git repo (SCOPE-2)."
    - "CLAUDE.md states all plugin work happens on main in the single checkout, and gives the concurrent-session commit discipline (SCOPE-1)."
    - "A resume via /continue or context-resume halts and surfaces the branch when HEAD is not main, instead of proceeding (SCOPE-3)."
    - ".claude/worktrees/ no longer exists on disk (SCOPE-4)."
  artifacts:
    - ".claude/hooks/SessionStart.py — with a Git Context section printed before sys.exit(0)"
    - "CLAUDE.md — rewritten ## Parallel Plugin Development section"
    - ".claude/skills/context-resume/SKILL.md — Step 0 location check"
    - ".claude/commands/continue.md — location check as Process step 1"
  key_links:
    - "Git Context output MUST go to stdout. SessionStart stdout is what enters the fresh session's context; stderr does not. Routing the WARN lines to stderr the way the existing validation code does would print them for the human and still leave the agent blind — which is the exact failure being fixed."
    - "The STATUS.md frontmatter scan must cap its read. O-Octagon's frontmatter runs 1628 lines and O-Contrabass's 866; all four keys sit within the first 10 lines of every file, so a 40-line cap is the link between 'scan 37 files' and 'stay inside the 5s hook timeout'."
    - "The hook is registered at .claude/settings.json with timeout: 5000. The added git work must be deadline-bounded, not just per-call-timeout-bounded, or four 1s timeouts stack toward the ceiling."
---

<objective>
Give every fresh session its missing 4th coordinate — *location* — and move the written policy to trunk-based development.

Purpose: The Plugin Freedom System resolves plugin work from fixed paths (`plugins/<Name>/.planning/STATUS.md`) but has no reader for branch or worktree. After `/clear`, a session on `main` reads a stale STATUS.md that looks fresh and silently redoes or clobbers work that lives on a feature branch. Rather than build branch-aware machinery, the user-approved decision is to put all plugin work on `main` and make any deviation loudly visible at session start.

Output: A Git Context section in the SessionStart hook, a rewritten CLAUDE.md policy section, location checks in both resume protocols, and the empty `.claude/worktrees/` directory removed.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
</execution_context>

<context>
@.planning/quick/260821-aym-fix-multi-plugin-branch-visibility-after/260821-aym-DIAGNOSIS.md
@.claude/hooks/SessionStart.py
@CLAUDE.md
</context>

<ground_truth>
Measured this session — do not re-derive, and do not assume any of it:

- Hook runtime today: 0.09–0.25s wall. stdout 16 lines, stderr 0 lines on a healthy machine. Exits 0.
- All four git probes together: **0.078s** total (`branch --show-current`, `worktree list`, `status --porcelain`, `rev-list --count origin/main..HEAD`).
- Current values: branch `main`, 1 worktree, 5 porcelain lines, 24 commits ahead of `origin/main`.
- `plugins/*/.planning/STATUS.md`: **37 files**, of which **8 have no frontmatter at all** (O-AnalogEQ, O-Bass, O-FreqPulse, O-GrainScatter, O-IntonationPad, O-MultiBandCompressor, O-Polystutter, O-SimpleReverb) — these must be skipped silently, not crash the scan.
- Frontmatter closing `---` is at line 1628 for O-Octagon and 866 for O-Contrabass. But `plugin:`/`stage:`/`status:`/`phase:` are all within the **first 10 lines** of every file, including those two.
- Two values carry a trailing `#` comment on the same line (O-Octagon's `status:` and `phase:`) and must be de-commented before display.
- Status values are free-form snake_case, not an enum. 20 files are exactly `complete`. Under the exclusion set `{complete, plugin_complete, installed}` exactly **7 plugins** are in-flight today: O-Bassoon, O-Contrabass, O-MicrotonalSampler, O-Octagon, O-Orbit, O-simpleFM, O-simpleGrain.
- `.claude/worktrees/` is empty **and untracked** (`git ls-files` returns nothing). Removing it produces no diff and nothing to stage — see Task 3.
- `backups/<Plugin>/vX.Y.Z/` is a real on-disk convention (e.g. `backups/O-AnalogEQ/v1.1.0/`).
- Release tags are named **`vX.Y.Z-<PluginName>`** (version first, plugin suffix — e.g. `v3.1.1-O-Bells`), 344 of them.
- **`git revert` does not accept a pathspec** — `git revert -n HEAD -- PLUGINS.md` fails with `fatal: bad revision 'PLUGINS.md'`. The working path-scoped undo is `git restore --source=<rev> -- <path>` (verified). Do not write a path-scoped revert into CLAUDE.md.
- `.gitattributes` exists at repo root with `PLUGINS.md merge=union` plus a 5-line explanatory comment. It is **out of scope** — leave it byte-identical.
</ground_truth>

<tasks>

<!-- planner-discipline-allow: Git Context -->
<!-- Rationale: "Git Context" is the exact section header Task 1 must emit, so it
     cannot be paraphrased out of the action prose. The one negative grep using it
     (`grep -c 'Git Context' /tmp/aym.p.err`) targets the HOOK'S STDERR STREAM, not
     any plan or source file, so prose in this document cannot contaminate it. The
     usual self-invalidation failure mode does not apply here. -->

<task type="tracer" tdd="false">
  <name>Task 1: Git Context section in SessionStart.py — branch, worktrees, dirty, unpushed, in-flight plugins</name>
  <files>.claude/hooks/SessionStart.py</files>
  <read_first>
    Read `.claude/hooks/SessionStart.py` in full (361 lines) before editing. Note two existing conventions you are extending, not replacing: `get_version_output()` swallows `TimeoutExpired`/`FileNotFoundError`/`OSError` and returns `None`; and `main()` ends with a `=== Validation Summary ===` block followed by `print("===========================")`, a blank `print()`, and `sys.exit(0)` with the comment about never blocking session start.
  </read_first>
  <action>
    Add two module-level helpers and one call site. Do not modify any existing validation logic, any existing print, or the exit contract (SCOPE-2).

    Helper 1 — `run_git(args, cwd, timeout=1)`: runs `["git"] + args` via `subprocess.run` with `capture_output=True, text=True`, list form only, never `shell=True`. Returns `result.stdout.strip()` when `returncode == 0`, otherwise `None`. Catches `subprocess.TimeoutExpired`, `FileNotFoundError`, and `OSError` and returns `None` — mirroring the existing `get_version_output` contract so a machine without git degrades instead of raising.

    Helper 2 — `print_git_context()`: emits the whole section. Resolve the repo root from `os.environ.get("CLAUDE_PROJECT_DIR")` falling back to `Path.cwd()`. Confirm it is a repository with `git rev-parse --show-toplevel`; when that returns `None`, print a single `[INFO] Git context unavailable (not a git repository)` line and return — this is the fail-soft path a fresh clone or a hook invoked from elsewhere takes.

    Deadline bound: capture `time.monotonic()` on entry and skip any remaining git probe once 1.5s of wall time has elapsed, printing one `[INFO] Git context truncated (slow repository)` line instead. Combined with the 1s per-call timeout this caps the section near 2.5s worst case against the `timeout: 5000` registration in `.claude/settings.json`; measured cost is 0.078s. Add `import time` to the existing import block.

    **Every line this function emits goes to stdout — no `file=sys.stderr` anywhere in it.** SessionStart stdout is what lands in the fresh session's context; a warning on stderr reaches the human and leaves the agent exactly as blind as it is today, which is the defect under repair.

    Print a `=== Git Context ===` header, then:

    - Branch, from `branch --show-current`. Exactly `main` prints `[OK] Branch: main`. Any other value prints `[WARN] Branch: <name> — expected main; all plugin work belongs on main`. Empty output means detached HEAD and prints a `[WARN] Branch: (detached HEAD)` line.
    - Worktrees, from `worktree list --porcelain`, counting lines that begin with `worktree `. A count of 1 prints `[OK] Worktrees: 1`. Anything greater prints `[WARN] Worktrees: <n> — extra checkouts exist; run git worktree list`.
    - Working tree, from `status --porcelain`. Report total changed paths as the line count, and staged paths as the count of lines whose index status character at position 0 is neither a space nor `?`. Print `[INFO] Working tree: <n> changed, <s> staged`; print `[OK] Working tree: clean` when the output is empty.
    - Unpushed, from `rev-list --count origin/main..HEAD`. Print `[INFO] Unpushed: <n> commit(s) ahead of origin/main` when it parses as an integer greater than zero, `[OK] Unpushed: 0` when zero, and skip the line entirely when the call returns `None` (no remote configured).
    - In-flight plugins, from the STATUS.md scan below.

    STATUS.md scan: iterate `sorted(repo_root.glob("plugins/*/.planning/STATUS.md"))`. For each file, read **at most the first 40 lines** — the keys sit within the first 10 lines even in the two files whose frontmatter runs past line 800, so a full read would scan thousands of lines for nothing. Require the first line to be exactly `---`; if it is not, skip the file silently (8 of 37 files have no frontmatter). Stop at the next line that is exactly `---`. Within that window, regex-match `^(plugin|stage|status|phase):\s*(.*)$` and keep the first occurrence of each key.

    Sanitize every captured value before it is printed, in this order: split on ` #` and keep the left side (O-Octagon's `status:` and `phase:` both carry a trailing comment), strip whitespace, drop any character with `ord(ch) < 32`, then truncate to 60 characters. This bounds what plugin-authored file content can inject into the session context.

    Filter: skip the plugin when its sanitized `status` is one of the three literals `complete`, `plugin_complete`, or `installed`. Everything else counts as in-flight — the values are free-form so an allowlist is the only stable direction. Fall back to the plugin directory name when the `plugin:` key is absent.

    Print `In-flight plugins (plugins/*/.planning/STATUS.md):` followed by one indented `  - <plugin> — stage <stage> — <status>` line per match, capped at 12 rows with a `  ... and <n> more` overflow line. Print `[OK] No in-flight plugins` when the list is empty. Today this yields 7 rows.

    Call site: invoke `print_git_context()` inside `main()` after the existing `print("===========================")` and its trailing blank `print()`, and before `sys.exit(0)`. Wrap the call in `try: ... except Exception: pass` so no defect in this section can ever block session start. Leave the `sys.exit(0)` line and its comment untouched.
  </action>
  <verify>
    <!-- Every gate regenerates its own hook output. Do NOT let one gate depend on a
         temp file another gate wrote - each must pass run individually and in any
         order. The hook costs 0.09s, so re-running it per gate is free. -->

    <!-- NEW-BEHAVIOR gates. Each of these fails before this task and passes after.
         Confirmed RED against the unmodified hook at plan time. -->
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; python3 .claude/hooks/SessionStart.py &gt;/tmp/aym.out 2&gt;/tmp/aym.err; test $? -eq 0 &amp;&amp; grep -q '=== Git Context ===' /tmp/aym.out &amp;&amp; grep -qE '^\[(OK|WARN)\] Branch: ' /tmp/aym.out &amp;&amp; grep -qE '^\[(OK|WARN)\] Worktrees: ' /tmp/aym.out &amp;&amp; grep -qE '^\[(OK|INFO)\] Working tree: ' /tmp/aym.out &amp;&amp; grep -q 'In-flight plugins' /tmp/aym.out &amp;&amp; echo TASK1-STDOUT-PASS</automated>
    <automated>cd /tmp &amp;&amp; env -u CLAUDE_PROJECT_DIR python3 /Users/taylorbrook/Dev/VST-development/.claude/hooks/SessionStart.py &gt;/tmp/aym.nogit 2&gt;&amp;1; test $? -eq 0 &amp;&amp; grep -q 'Git context unavailable' /tmp/aym.nogit &amp;&amp; echo TASK1-FAILSOFT-PASS</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; python3 .claude/hooks/SessionStart.py &gt;/tmp/aym.scan 2&gt;/dev/null; N=$(grep -c '^  - O-' /tmp/aym.scan) &amp;&amp; test "$N" -eq 7 &amp;&amp; grep -q '^  - O-Bassoon ' /tmp/aym.scan &amp;&amp; grep -q '^  - O-simpleGrain ' /tmp/aym.scan &amp;&amp; ! grep -q '^  - O-Bells ' /tmp/aym.scan &amp;&amp; echo "TASK1-SCAN-PASS ${N} in-flight"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; python3 .claude/hooks/SessionStart.py &gt;/tmp/aym.san 2&gt;/dev/null; grep -q '^  - O-Octagon — stage 4 — stage_4_complete$' /tmp/aym.san &amp;&amp; ! grep -q 'VERIFICATION.md REWRITTEN' /tmp/aym.san &amp;&amp; echo TASK1-SANITIZE-PASS</automated>

    <!-- PRESERVATION gates. These pass BEFORE this task too, BY DESIGN - they are
         negative controls against destroying what already works, not evidence the
         new section exists. A green here proves only that nothing broke. The
         NEW-BEHAVIOR gates above are what prove the feature landed; do not read a
         green preservation gate as task completion. -->
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; python3 .claude/hooks/SessionStart.py &gt;/tmp/aym.p.out 2&gt;/tmp/aym.p.err; test $? -eq 0 &amp;&amp; grep -q '=== Validation Summary ===' /tmp/aym.p.out &amp;&amp; grep -q 'Environment Validation' /tmp/aym.p.out &amp;&amp; test "$(grep -c '^\[OK\]' /tmp/aym.p.out)" -ge 9 &amp;&amp; grep -c 'Git Context' /tmp/aym.p.err | grep -qx 0 &amp;&amp; echo TASK1-PRESERVE-PASS</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; S=$(python3 -c "import time,subprocess;t=time.monotonic();subprocess.run(['python3','.claude/hooks/SessionStart.py'],capture_output=True);print(f'{time.monotonic()-t:.2f}')") &amp;&amp; python3 -c "import sys;sys.exit(0 if float('$S') &lt; 2.0 else 1)" &amp;&amp; echo "TASK1-TIMING-PASS ${S}s"</automated>
  </verify>
  <done>
    `python3 .claude/hooks/SessionStart.py` exits 0 and prints a `=== Git Context ===` section on **stdout** carrying branch, worktree count, working-tree counts, unpushed count, and 7 in-flight plugin rows. Zero Git Context output reaches stderr. All pre-existing validation output survives verbatim. Run from `/tmp` with no `CLAUDE_PROJECT_DIR` it still exits 0 via the unavailable-repository path. Total runtime stays under 2.0s. O-Octagon's row shows `stage_4_complete` with its trailing `#` comment stripped, and O-Bells (status `complete`) is absent from the list.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Rewrite CLAUDE.md "Parallel Plugin Development" for trunk-based development</name>
  <files>CLAUDE.md</files>
  <read_first>
    Read `CLAUDE.md` lines 104–157. The section to replace begins at the `## Parallel Plugin Development` heading on line 104 and ends at line 155, immediately before `## Project Structure` on line 157. It currently holds five subsections and two fenced bash blocks.
  </read_first>
  <action>
    Replace the body of `## Parallel Plugin Development` with a trunk-based policy (SCOPE-1). Use `Edit` scoped to that section — do not `Write` the whole file, and do not touch `## Project Structure` or anything above line 104.

    Delete the four subsections covering per-plugin branching, per-plugin worktrees, worktree directory naming, and worktree placement relative to the repo, along with the `git switch -c` block and the worktree-commands block. The conventions they describe no longer apply.

    Keep the section heading. Open with a short lead: multiple plugins in flight at once is still normal and expected, but isolation now comes from path discipline inside the single checkout at `~/Dev/VST-development`, not from branches. All plugin work happens on `main`.

    Then write these subsections:

    `### Commit discipline for concurrent sessions` — the sharp edge that survives the migration. Two sessions in one checkout share `.git/index` and HEAD, so another session's staging can join your commit between the moment you check and the moment you commit. Two rules: path-scope every commit by naming the paths explicitly (`git commit -- plugins/<Name> PLUGINS.md`), never `git add -A` and never `git commit -a`; and re-run `git branch --show-current` together with `git status --short` immediately before every commit. State plainly that a session-start snapshot is not good enough — it can be minutes stale, and the hook's Git Context is a starting picture, not a commit-time guarantee.

    `### Rollback without branches` — what replaces per-plugin branches for undo. Three mechanisms, all real and all path-scoped: the `backups/<Plugin>/vX.Y.Z/` snapshots that `/improve` writes; the release tags, whose actual format is `vX.Y.Z-<PluginName>` with the version first and the plugin name as the suffix (for example `v3.1.1-O-Bells`); and `git restore --source=<tag-or-sha> -- plugins/<Name>` to pull one plugin's tree back to a known state. Include the one-line warning that `git revert` takes no pathspec — `git revert -n <sha> -- plugins/<Name>` fails with `fatal: bad revision`, so `restore` is the path-scoped tool, not `revert`.

    `### Branches are for exceptional repo-wide work only` — a branch is justified only when a change is repo-wide and risky enough that `main` should not carry it mid-flight, such as a framework version bump or a cross-plugin removal sweep. Never for a single plugin's feature or fix. It must be cut from `main`, kept short-lived, and merged and deleted in the same working period. Note that the SessionStart hook prints the current branch and worktree count at every session start, so a stray branch or leftover worktree surfaces after every `/clear` rather than silently stranding work. Worktrees are no longer part of the routine workflow; if the hook reports more than one, remove it with `git worktree remove` once its branch is merged.

    `### PLUGINS.md union merge (exceptional branches only)` — condense the two existing caveats to the one case that can still occur. The root `.gitattributes` maps `PLUGINS.md` to git's `union` driver. Under trunk-based development there are no routine merges, so this only fires when an exceptional branch merges back. Union trades a conflict for a possible duplicate row, and it fires even when each side edited only its own row because adjacent rows share a diff hunk. Preserve the duplicate-check command verbatim: `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` (empty output means clean). Drop the second caveat about which working tree the merge attribute is read from — it described a branch-to-branch scenario that trunk-based development removes.

    Leave `.gitattributes` itself untouched.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; grep -q '^## Parallel Plugin Development$' CLAUDE.md &amp;&amp; grep -q '^## Project Structure$' CLAUDE.md &amp;&amp; grep -q 'git commit -- plugins/' CLAUDE.md &amp;&amp; grep -q 'git restore --source=' CLAUDE.md &amp;&amp; grep -q 'uniq -d' CLAUDE.md &amp;&amp; echo TASK2-CONTENT-PASS</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; sed -n '/^## Parallel Plugin Development$/,/^## Project Structure$/p' CLAUDE.md &gt; /tmp/aym.sec &amp;&amp; grep -v '^#' /tmp/aym.sec | grep -c 'git switch -c' | grep -qx 0 &amp;&amp; grep -v '^#' /tmp/aym.sec | grep -c 'git worktree add' | grep -qx 0 &amp;&amp; echo TASK2-REMOVED-PASS</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; git diff --numstat -- CLAUDE.md | awk '{print $NF}' | grep -qx CLAUDE.md &amp;&amp; test "$(git diff --name-only -- .gitattributes | wc -l)" -eq 0 &amp;&amp; echo TASK2-SCOPE-PASS</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; python3 -c "
import re,sys
s=open('CLAUDE.md').read()
sec=s.split('## Parallel Plugin Development')[1].split('## Project Structure')[0]
subs=re.findall(r'^### (.+)$',sec,re.M)
assert len(subs)==4, f'expected 4 subsections, got {len(subs)}: {subs}'
assert sec.count('\`\`\`')%2==0, 'unbalanced fenced block'
print('TASK2-STRUCTURE-PASS', subs)
"</automated>
  </verify>
  <done>
    `## Parallel Plugin Development` holds exactly four subsections covering concurrent-session commit discipline, branchless rollback, exceptional-branch use, and the condensed union-merge note. The per-plugin branch and worktree conventions and both of their command blocks are gone. `git restore --source=` appears (and no path-scoped `git revert` does). The duplicate-row check command survives verbatim. Only `CLAUDE.md` is modified; `.gitattributes` and `## Project Structure` are untouched.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Location check in both resume protocols, and remove .claude/worktrees/</name>
  <files>.claude/skills/context-resume/SKILL.md, .claude/commands/continue.md, .claude/worktrees/</files>
  <read_first>
    Read `.claude/skills/context-resume/SKILL.md` — the Resume Workflow progress checklist is at lines 74–80 and `### Step 1: Locate Handoff File` at line 82. Read `.claude/commands/continue.md` — the `## Process` numbered list runs lines 36–62, with "Load checkpoint skill" as item 1 and "Load active plugin state" as item 2.
  </read_first>
  <action>
    Add a location check to both resume protocols so a resume on the wrong branch halts instead of proceeding on stale state (SCOPE-3), then remove the dead directory (SCOPE-4).

    In `.claude/skills/context-resume/SKILL.md`: insert a `### Step 0: Verify Location` ahead of `### Step 1: Locate Handoff File`, and add a matching `- [ ] Step 0: Verify location (git branch --show-current == main)` as the first line of the Progress Tracking checklist. The step runs `git branch --show-current` and compares against `main`. On a match it proceeds silently to Step 1. On any other value — including empty output for a detached HEAD — it MUST stop, print the branch it found, and tell the user that STATUS.md is branch-versioned so the file it is about to read may not describe the work on this branch. Offer switching to `main` or explicitly confirming a resume here; do not auto-proceed and do not auto-switch. Add a matching line to the `<requirements>` MUST list, and add the corresponding entry to `<anti_patterns>` describing the failure of resuming without first confirming which branch is checked out.

    In `.claude/commands/continue.md`: insert the same check as a new numbered item **1** at the head of `## Process`, renumbering the existing six items to 2–7. Keep it to three lines — run `git branch --show-current`; if the result is not `main`, stop and surface the branch rather than loading state; otherwise continue. Add a `**Wrong branch:**` block to `## Output Format` showing what the halt looks like, matching the existing fenced-block style used by the other output examples.

    Both edits are additive to the surrounding documents — do not restructure or reword the existing steps beyond the renumbering that inserting item 1 forces.

    Then remove `.claude/worktrees/`. It is empty and **untracked** — `git ls-files` returns nothing for it — so `rmdir .claude/worktrees` is the whole operation. Expect no diff and nothing to stage from this removal; it will not appear in `git status` and contributes no content to the commit. Do not use `git rm`, which would fail on an untracked path. The only two mentions of the word "worktrees" outside `.git/` are prose lines in the CLAUDE.md section Task 2 rewrites, so nothing references this path.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; grep -q 'Step 0: Verify Location' .claude/skills/context-resume/SKILL.md &amp;&amp; grep -c 'branch --show-current' .claude/skills/context-resume/SKILL.md | grep -qvx 0 &amp;&amp; grep -q 'Step 0: Verify location' .claude/skills/context-resume/SKILL.md &amp;&amp; echo TASK3-SKILL-PASS</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; grep -c 'branch --show-current' .claude/commands/continue.md | grep -qvx 0 &amp;&amp; grep -q 'Wrong branch' .claude/commands/continue.md &amp;&amp; python3 -c "
import re
s=open('.claude/commands/continue.md').read()
proc=s.split('## Process')[1].split('## Validation on Resume')[0]
nums=[int(m) for m in re.findall(r'^(\d+)\. ',proc,re.M)]
assert nums==list(range(1,8)), f'process items not 1..7: {nums}'
print('TASK3-CONTINUE-PASS',nums)
"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; test ! -d .claude/worktrees &amp;&amp; test -d .claude/skills &amp;&amp; test -d .claude/hooks &amp;&amp; echo TASK3-CLEANUP-PASS</automated>
    <!-- PRESERVATION gate: passes before this task too, by design. It guards the two
         files against being corrupted by the edits above (unbalanced fence, lost
         frontmatter); it is not evidence the location checks were added. -->
    <automated>cd /Users/taylorbrook/Dev/VST-development &amp;&amp; python3 -c "
for f in ['.claude/skills/context-resume/SKILL.md','.claude/commands/continue.md']:
    s=open(f).read()
    assert s.count('\`\`\`')%2==0, f'unbalanced fenced block in {f}'
    assert s.startswith('---'), f'frontmatter lost in {f}'
print('TASK3-INTEGRITY-PASS')
"</automated>
  </verify>
  <done>
    Both resume protocols check `git branch --show-current` before loading any state and halt on a non-main branch instead of proceeding. `continue.md`'s Process list numbers cleanly 1–7 with the location check first, and carries a wrong-branch output example. `SKILL.md` has a Step 0, a matching checklist line, a `<requirements>` entry, and an `<anti_patterns>` entry. `.claude/worktrees/` is gone while the rest of `.claude/` is intact. Both files keep balanced fenced blocks and intact frontmatter.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| repo files → session context | The hook reads `plugins/*/.planning/STATUS.md` and prints the values into the fresh session's context on stdout. File content becomes agent-visible text. |
| git subprocess → hook | Branch names, worktree paths, and status output are attacker-influenceable strings if the repo is ever cloned from an untrusted remote. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-aym-01 | Elevation of Privilege | `run_git()` in SessionStart.py | medium | mitigate | List-form `subprocess.run` only, never `shell=True`, so a crafted branch or path name cannot reach a shell. Fixed argument vectors — no user string is ever concatenated into a command. |
| T-aym-02 | Tampering | STATUS.md frontmatter → session context | medium | mitigate | Sanitize every captured value before printing: strip inline `#` comments, drop control characters (`ord < 32`), truncate to 60 chars, cap the list at 12 rows. Bounds how much plugin-authored text can shape the session's opening context. |
| T-aym-03 | Denial of Service | SessionStart hook vs `timeout: 5000` | high | mitigate | 1s per-git-call timeout plus a 1.5s cumulative deadline in `print_git_context()`; 40-line read cap per STATUS.md; whole section in `try/except Exception: pass`. A hang degrades the section, never the session. Verified by the under-2.0s timing gate. |
| T-aym-04 | Information Disclosure | Git Context on stdout | low | accept | Branch names, plugin names, and counts are already visible to anyone with repo access; the hook runs locally and adds no new exposure. |
</threat_model>

<verification>
Run after all three tasks, from the repo root:

1. `python3 .claude/hooks/SessionStart.py` exits 0, prints `=== Git Context ===` on stdout, and 7 in-flight plugin rows.
2. Negative control for the branch WARN — the branch line must change with the branch, not merely exist. On a scratch branch the hook prints the `[WARN] Branch:` form; back on `main` it prints the `[OK] Branch: main` form:
   `git switch -c tmp/aym-probe && python3 .claude/hooks/SessionStart.py | grep -E '^\[WARN\] Branch: tmp/aym-probe' && git switch main && git branch -d tmp/aym-probe && python3 .claude/hooks/SessionStart.py | grep -E '^\[OK\] Branch: main'`
   A gate that passes on both branches would be decoration.
3. `git status --short` shows modifications confined to `.claude/hooks/SessionStart.py`, `CLAUDE.md`, `.claude/skills/context-resume/SKILL.md`, and `.claude/commands/continue.md` — plus whatever was already dirty before this task started. `.claude/worktrees/` will not appear (it was untracked).
4. `.gitattributes`, `PLUGINS.md`, and every `plugins/**` path are unmodified.
</verification>

<success_criteria>
- A fresh session after `/clear` sees branch, worktree count, working-tree counts, unpushed count, and the in-flight plugin list in its opening context, with any non-main branch or extra worktree marked `[WARN]`.
- The hook never blocks session start: exit 0 on a healthy repo, exit 0 outside a repo, exit 0 with git unavailable, and all pre-existing dependency validation output preserved.
- CLAUDE.md directs all plugin work to `main`, gives path-scoped concurrent-session commit discipline, documents branchless rollback with commands that actually run, and confines branches to exceptional repo-wide work.
- `/continue` and `context-resume` both refuse to load state on a non-main branch.
- `.claude/worktrees/` is gone.
- Nothing outside the four in-scope files changed. No schema edits, no PLUGINS.md column changes, no per-command branch preconditions, no plugin-workflow orchestration changes.
</success_criteria>

<output>
Commit as a single atomic commit scoped to the four files:
`fix(workflow): trunk-based development + session-start git context`

Then create `.planning/quick/260821-aym-fix-multi-plugin-branch-visibility-after/260821-aym-SUMMARY.md`.
</output>
