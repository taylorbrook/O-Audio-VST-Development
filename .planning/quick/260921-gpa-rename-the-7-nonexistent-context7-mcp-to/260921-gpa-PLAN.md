---
phase: quick-260921-gpa
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/agents/research-planning-agent.md
  - .claude/agents/dorico-agent.md
  - .claude/agents/dsp-agent.md
  - .claude/agents/gui-agent.md
  - .claude/agents/foundation-shell-agent.md
  - .claude/agents/troubleshoot-agent.md
autonomous: true
requirements: [QT-gpa-01, QT-gpa-02, QT-gpa-03]

estimate:
  tokens: 26000
  raw_tokens: 13000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "Every context7 tool name declared anywhere under .claude/agents/ names a tool the live context7 server actually exposes (resolve-library-id or query-docs)."
    - "The troubleshoot agent can resolve a library name to a library ID before fetching docs — its declared context7 toolset is self-sufficient, not a dangling fetch verb (QT-gpa-02, SCOPE ADDITION SA-01)."
    - "All six edited agent files still open with a YAML frontmatter block whose `tools:` key is a single line at line 4."
    - "The unrelated pre-existing working-tree modification to .claude/agent-memory/research-planning-agent.md is still uncommitted and unmodified after the run (QT-gpa-03)."
  artifacts:
    - .claude/agents/research-planning-agent.md
    - .claude/agents/dorico-agent.md
    - .claude/agents/dsp-agent.md
    - .claude/agents/gui-agent.md
    - .claude/agents/foundation-shell-agent.md
    - .claude/agents/troubleshoot-agent.md
  key_links:
    - "Each file's line-4 `tools:` allowlist entry -> the live context7 MCP server tool registry (exactly two tools: mcp__context7__resolve-library-id, mcp__context7__query-docs)."
    - "research-planning-agent.md:765 body prose (the documented call shape) -> that same file's line-4 allowlist; both must name the same fetch verb or the agent's own instructions contradict its permissions."
    - "troubleshoot-agent.md line-4 allowlist -> the query-docs precondition that a libraryId must be resolved first."
---

<objective>
Rename all 7 occurrences of nonexistent context7 MCP tool names under `.claude/agents/` to the tool the live server actually exposes, `mcp__context7__query-docs`.

The live context7 server exposes exactly two tools: `mcp__context7__resolve-library-id` and `mcp__context7__query-docs`. Six agent definitions declare a third name in their frontmatter allowlist that no longer exists, so those agents silently lose documentation access at dispatch time.

Requirements addressed:
- **QT-gpa-01** — all 7 occurrences of the two dead names renamed to `mcp__context7__query-docs` (6 frontmatter allowlist entries + 1 body-prose reference at research-planning-agent.md:765).
- **QT-gpa-02** — **SCOPE ADDITION SA-01**: troubleshoot-agent.md also gains `mcp__context7__resolve-library-id`. See the flag below.
- **QT-gpa-03** — frontmatter integrity preserved and the commit is path-scoped to exactly the 6 agent files.

Purpose: restore working documentation lookup for the dsp, foundation-shell, dorico, research-planning, gui, and troubleshoot agents.
Output: 6 edited agent markdown files, one path-scoped commit on `main`.

## SCOPE ADDITION SA-01 (beyond the literal 7 renames) — flagged for developer awareness

`troubleshoot-agent.md:4` currently declares `mcp__context7__search_juce_docs` and **no** `mcp__context7__resolve-library-id`. The replacement tool `query-docs` requires a resolved `libraryId` (`/org/project`) as input — its own tool contract says resolve-library-id must be called first unless the caller already holds a literal library ID. The troubleshoot-agent body (Level 2, lines 141-178) describes only a generic "Query Context7 with JUCE library search" and never names or supplies a literal library ID.

So a literal-only rename on that file produces a declared-but-unusable tool — the same class of defect this task exists to remove. This plan therefore adds `mcp__context7__resolve-library-id` to that one tools line.

Cost of the addition: one token in one allowlist, granting a read-only library-name-to-ID lookup against an MCP server the agent is already permitted to call. The agent already holds `WebSearch` and `WebFetch`, which are strictly broader network reach. Reversible in one edit. If the developer wants the literal-7-only version instead, drop the second half of Task 2's troubleshoot edit; the other 6 occurrences are unaffected.

## Planner-contribution checkpoints (plan:pre hooks) — all non-firing, stated for the record

- **api-coverage**: does not fire. No external API integration is introduced; the only API surface touched is a tool-name string in an agent allowlist.
- **assumption-delta**: does not fire. No singular-to-plural or cardinality transition — this is a 1:1 name substitution with a fixed, live-verified count of 7.
- **schema-gate**: does not fire. No ORM, migration, or schema files are in `files_modified`; all six targets are markdown.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md

Relevant CLAUDE.md rules binding this plan:
- Trunk-based: stay on `main`. No branch, no worktree.
- Concurrent sessions share `.git/index` and HEAD. Path-scope every commit; never `git add -A`, never `git commit -a`. Re-check `git branch --show-current` and `git status --short` immediately before the commit, not once at plan start.

Live observation made at planning time (authoritative scope — 7 hits, 6 files, re-confirmed by the planner, not inherited from STATE.md):

    .claude/agents/dorico-agent.md:4            (frontmatter allowlist)
    .claude/agents/foundation-shell-agent.md:4  (frontmatter allowlist)
    .claude/agents/dsp-agent.md:4               (frontmatter allowlist)
    .claude/agents/gui-agent.md:4               (frontmatter allowlist)
    .claude/agents/research-planning-agent.md:4 (frontmatter allowlist)
    .claude/agents/research-planning-agent.md:765 (body prose, tools_guidance section)
    .claude/agents/troubleshoot-agent.md:4      (frontmatter allowlist, the odd one out)

Body-prose survey already performed by the planner — do NOT re-audit, and do NOT rewrite working prose:
- The only body-prose occurrence of a dead tool name in the whole directory is research-planning-agent.md:765.
- No file anywhere under `.claude/agents/` documents the old fetch tool's parameter names (no `topic:`, no `tokens:`). The surviving descriptor on line 765 — "with resolved library ID" — remains accurate for query-docs, which takes a `libraryId`. Leave that phrasing alone; rename only the tool name inside the backticks.
- All other "Context7" mentions (dorico-agent.md:66; troubleshoot-agent.md:141,147,172,247,398,567,587,600,717; research-planning-agent.md:226,228,231,236,760,762,821,959) are generic product-name prose that is correct as written. **Do not touch them.**
- `.claude/agents/research-lead.md` carries its `tools:` key at line 5, not line 4, and declares no context7 tools. It is out of scope — never include it in an edit or in a line-4 frontmatter assertion.

Baseline occurrence counts under `.claude/agents/` before any edit (measured at planning time):
- dead names (`get-library-docs` + `search_juce_docs`): **7**
- `mcp__context7__query-docs`: **0**
- `mcp__context7__resolve-library-id`: **6**
</context>

<tasks>

<task type="tracer">
  <name>Task 1: End-to-end rename on the one file that has both layers — research-planning-agent.md</name>
  <precondition>HEAD is on `main`, and `git status --short` shows exactly one modified path: ` M .claude/agent-memory/research-planning-agent.md`. If any of the six target agent files is already modified, another session is editing them — halt and report rather than editing over it.</precondition>
  <files>.claude/agents/research-planning-agent.md</files>
  <action>
This is the thin end-to-end slice: research-planning-agent.md is the only file carrying BOTH an occurrence layer — a frontmatter allowlist entry (line 4) and a body-prose reference (line 765). Proving the rename on it proves both layers before the other five files are touched.

<!-- planner-discipline-allow: get-library-docs -->
Use the Edit tool for both edits, one Edit call each. Do NOT use `sed -i`, `python`, or any bulk rewriter: BSD sed on macOS needs `-i ''` and the project has a recorded trap where Python text I/O silently rewrites line endings. Two targeted Edit calls are deterministic and carry no rewrite risk.

Edit 1, line 4: in the `tools:` frontmatter value, replace the trailing entry `mcp__context7__get-library-docs` with `mcp__context7__query-docs`. Leave `mcp__context7__resolve-library-id` and every other entry, the comma-space separators, and the `tools: ` key prefix exactly as they are. Line 4 must remain ONE line — introducing a newline breaks YAML frontmatter parsing and silently strips the agent's entire toolset.

Edit 2, line 765, inside the `### Context7-MCP` block of `<tools_guidance>`: the line reads `- Fetch docs: ` followed by the dead tool name in backticks, then ` with resolved library ID`. Replace only the backticked tool name with `mcp__context7__query-docs`. Keep the `- Fetch docs: ` prefix and the trailing ` with resolved library ID` verbatim — that descriptor is still correct for query-docs, which takes a `libraryId`. Do not add, reflow, or reword anything else in that block; line 764 (the resolve-library-id line) is already correct and must not change.

Touch nothing else in this 900+ line file. In particular, leave lines 226, 228, 231, 236, 760, 762, 821, and 959 alone — those are generic "Context7-MCP" product-name prose, not tool names.
  </action>
  <verify>
    <automated>test "$(grep -o -e 'get-library-docs' -e 'search_juce_docs' .claude/agents/research-planning-agent.md | wc -l | tr -d ' ')" = "0" && test "$(grep -o 'mcp__context7__query-docs' .claude/agents/research-planning-agent.md | wc -l | tr -d ' ')" = "2" && test "$(grep -o 'mcp__context7__resolve-library-id' .claude/agents/research-planning-agent.md | wc -l | tr -d ' ')" = "2" && grep -n '^tools: ' .claude/agents/research-planning-agent.md | grep -q '^4:' && test "$(grep -c '^tools: ' .claude/agents/research-planning-agent.md | tr -d ' ')" = "1" && test "$(grep -o -e 'get-library-docs' -e 'search_juce_docs' -r .claude/agents/ | wc -l | tr -d ' ')" = "5"</automated>
  </verify>
  <done>research-planning-agent.md contains zero dead tool names, exactly 2 occurrences of `mcp__context7__query-docs` (line 4 and line 765) and still exactly 2 of `mcp__context7__resolve-library-id` (line 4 and line 764). Its `tools:` key is still a single line at line 4. The directory-wide dead-name count has dropped from the baseline 7 to exactly 5, confirming this file surrendered both of its occurrences and no other file changed.</done>
</task>

<task type="auto">
  <name>Task 2: Expand the proven rename to the remaining five files, plus scope addition SA-01</name>
  <files>.claude/agents/dorico-agent.md, .claude/agents/dsp-agent.md, .claude/agents/gui-agent.md, .claude/agents/foundation-shell-agent.md, .claude/agents/troubleshoot-agent.md</files>
  <action>
Apply the same single-Edit-per-file frontmatter rename proven in Task 1. Again: Edit tool only, no bulk rewriter.

<!-- planner-discipline-allow: get-library-docs -->
<!-- planner-discipline-allow: search_juce_docs -->
Four straightforward files — dorico-agent.md, dsp-agent.md, gui-agent.md, foundation-shell-agent.md. Each has line 4 ending in `mcp__context7__resolve-library-id, mcp__context7__get-library-docs`. Replace only the final entry with `mcp__context7__query-docs`. Every other tool in each list differs between these files (dorico carries Bash/Grep/Glob/WebSearch/WebFetch, dsp and foundation-shell carry only Read/Edit-or-Write, gui carries Bash) — preserve each list exactly as found and change nothing but the final entry. Line 4 stays one line in every file.

Fifth file, troubleshoot-agent.md line 4 — the odd one out, and the SCOPE ADDITION. Its current final entry is `mcp__context7__search_juce_docs` and it has NO resolve entry at all. Replace that final entry with the two-entry sequence `mcp__context7__resolve-library-id, mcp__context7__query-docs`. The second of those is the literal rename (QT-gpa-01); the first is scope addition SA-01 (QT-gpa-02), without which query-docs has no way to obtain the `libraryId` it requires. Preserve the preceding `Read, Write, Grep, Glob, Bash, WebSearch, WebFetch, ` prefix and the `tools: ` key verbatim, and keep the whole thing on line 4.

Do NOT edit the troubleshoot-agent body. Its Level 2 section (lines 141-178) says "Query Context7 with JUCE library search" and lists class names, namespaces, and modules to search for — that prose is tool-name-agnostic and reads correctly against the resolve-then-query pair. It is working prose; leave it.

Do NOT touch `.claude/agents/research-lead.md`, `critic-orchestrator.md`, `polish-agent.md`, `ui-design-agent.md`, `ui-finalization-agent.md`, or `validation-agent.md` — none declares a context7 tool.
  </action>
  <verify>
    <automated>test "$(grep -o -e 'get-library-docs' -e 'search_juce_docs' -r .claude/agents/ | wc -l | tr -d ' ')" = "0" && test "$(grep -o 'mcp__context7__query-docs' -r .claude/agents/ | wc -l | tr -d ' ')" = "7" && test "$(grep -o 'mcp__context7__resolve-library-id' -r .claude/agents/ | wc -l | tr -d ' ')" = "7" && test "$(grep -c 'mcp__context7__resolve-library-id' .claude/agents/troubleshoot-agent.md | tr -d ' ')" = "1" && test "$(grep -n '^tools: ' .claude/agents/dorico-agent.md .claude/agents/dsp-agent.md .claude/agents/gui-agent.md .claude/agents/foundation-shell-agent.md .claude/agents/troubleshoot-agent.md .claude/agents/research-planning-agent.md | grep -c ':4:' | tr -d ' ')" = "6"</automated>
  </verify>
  <done>Zero dead tool names remain anywhere under `.claude/agents/`. `mcp__context7__query-docs` occurs exactly 7 times (6 frontmatter + 1 prose), matching the 7 live-observed hits one-for-one. `mcp__context7__resolve-library-id` has risen from the baseline 6 to 7, the single increment being SA-01 on troubleshoot-agent.md. All six edited files still carry their `tools:` key as a single line at line 4.</done>
  <reversibility rating="reversible">SA-01 adds one entry to one allowlist for a read-only docs-lookup tool; removing it is a one-token Edit with no downstream state.</reversibility>
</task>

<task type="auto">
  <name>Task 3: Final sweep and path-scoped commit that excludes the unrelated agent-memory modification</name>
  <files>(no file edits — verification and commit only)</files>
  <precondition>`git branch --show-current` still reports `main` and `git status --short` still shows ` M .claude/agent-memory/research-planning-agent.md` as a modification that this plan did not make. Re-check both immediately before the commit — a session-start snapshot is minutes stale and another session sharing this index can stage into the gap.</precondition>
  <action>
Re-run the branch and staging checks named in the precondition immediately before committing — not earlier in the task, and not once at plan start. This checkout is shared with concurrent sessions; `git status --short` must be read at commit time.

Commit with an explicit pathspec naming only the six agent files. Never `git add -A`, never `git commit -a`, never a bare `git commit` that could sweep in another session's staging or the unrelated `.claude/agent-memory/research-planning-agent.md` modification. Note the near-collision: the excluded file and one edited file share the basename `research-planning-agent.md` and differ only in directory (`agent-memory/` vs `agents/`) — spell the six paths out in full rather than relying on a basename glob.

Commit message: `fix(claude): rename dead context7 tool names in agent allowlists`, with a body recording (a) 7 occurrences across 6 files renamed to mcp__context7__query-docs, and (b) SA-01, the resolve-library-id entry added to troubleshoot-agent.md so the renamed fetch verb has a way to obtain a library ID.

After committing, confirm the unrelated agent-memory file is still present in the working tree as an uncommitted modification — this plan must leave it exactly as it found it.
  </action>
  <verify>
    <automated>test "$(grep -o -e 'get-library-docs' -e 'search_juce_docs' -r .claude/agents/ | wc -l | tr -d ' ')" = "0" && test "$(git show --pretty=format: --name-only HEAD | grep -c . | tr -d ' ')" = "6" && test "$(git show --pretty=format: --name-only HEAD | grep -c '^\.claude/agents/' | tr -d ' ')" = "6" && ! git show --pretty=format: --name-only HEAD | grep -q 'agent-memory' && test "$(git status --porcelain -- .claude/agents/ | grep -c . | tr -d ' ')" = "0" && git status --porcelain -- .claude/agent-memory/research-planning-agent.md | grep -q '^ M' && test "$(git branch --show-current)" = "main"</automated>
  </verify>
  <done>HEAD's commit touches exactly 6 files, every one of them under `.claude/agents/`, and none of them the agent-memory file. No agent file is left dirty. `.claude/agent-memory/research-planning-agent.md` is still an uncommitted working-tree modification. The branch is still `main`.</done>
</task>

</tasks>

<threat_model>
Scope note: proportionate to the change. This plan edits agent tool allowlists in markdown; it installs no packages, adds no dependency, opens no network listener, and touches no runtime or plugin code. ASVS level 1; blocking threshold `high`. No threat below reaches `high`, so nothing here blocks execution.

## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| agent frontmatter allowlist -> MCP tool dispatch | The `tools:` line is the authorization surface deciding which tools a dispatched subagent may call. An edit here changes granted capability. |
| agent -> context7 MCP server | Query text leaves the machine and reaches a third-party documentation API. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-gpa-01 | Elevation of Privilege | `tools:` line of the 6 edited agent files | low | accept | The renamed entry grants `mcp__context7__query-docs`, the same read-only documentation-fetch capability the dead name was written to grant. No new capability class, no write or execute verb. Tasks 1 and 2 forbid touching any non-context7 entry, and Task 2's verify pins the total query-docs count at exactly 7 — one per live-observed hit — so no extra grant can slip in unnoticed. |
| T-gpa-02 | Elevation of Privilege | `tools:` line of troubleshoot-agent.md (SA-01) | low | accept | SA-01 adds `mcp__context7__resolve-library-id`: a read-only library-name-to-ID lookup against a server the agent is already authorized to call. The agent already holds `WebSearch` and `WebFetch`, strictly broader network reach, so this widens no real boundary. Disclosed above as a flagged scope addition and marked reversible; Task 2's verify pins the repo-wide resolve-library-id count at exactly 7 (baseline 6 + this one). |
| T-gpa-03 | Information Disclosure | agent -> context7 API query text | low | accept | Pre-existing exposure, unchanged by this plan — five of the six agents already reach this server. The context7 tool contract already forbids sensitive content in queries, and no task here introduces a credential-bearing or private-source query. |
| T-gpa-04 | Tampering | shared `.git/index` and HEAD | medium | mitigate | Concurrent sessions share this checkout, so a foreign `git add` can join this plan's commit and an unrelated agent-memory modification is already dirty in the tree. Mitigated concretely: Task 3 carries a precondition requiring `git branch --show-current` and `git status --short` to be re-read at commit time (not at plan start), mandates an explicit six-path pathspec with `git add -A` / `git commit -a` prohibited, warns about the `research-planning-agent.md` basename collision between `agents/` and `agent-memory/`, and verifies post-commit that HEAD touches exactly 6 files, all under `.claude/agents/`, none matching `agent-memory`. |
| T-gpa-05 | Denial of Service | YAML frontmatter of the 6 edited agent files | medium | mitigate | A newline accidentally introduced into line 4 silently strips an agent's entire toolset — the agent still loads but can do nothing, a failure that no functional test in this repo would catch. Mitigated: Tasks 1 and 2 mandate the Edit tool over `sed -i`/Python bulk rewriters (recorded CRLF-rewrite trap) and state the single-line constraint explicitly; both verifies assert that `^tools: ` still resolves to line 4 in every edited file, and Task 1 additionally asserts the key occurs exactly once. |

No supply-chain threat (`T-gpa-SC`) is registered: this plan runs no npm, pip, or cargo install, so the package-legitimacy gate does not apply.
</threat_model>

<verification>
Run from the repository root after Task 3 completes.

1. The original live-observation grep returns nothing:
   `grep -rn "get-library-docs\|search_juce_docs" .claude/agents/` -> no output, exit 1.
2. Occurrence counts land exactly on the live-observed scope:
   `grep -o 'mcp__context7__query-docs' -r .claude/agents/ | wc -l` -> `7`
   `grep -o 'mcp__context7__resolve-library-id' -r .claude/agents/ | wc -l` -> `7` (baseline 6 + SA-01)
3. Frontmatter integrity across the six edited files — each reports its `tools:` key at line 4, exactly once:
   `grep -n '^tools: ' .claude/agents/{dorico,dsp,gui,foundation-shell,troubleshoot,research-planning}-agent.md`
   -> six lines, every one containing `:4:`. (`research-lead.md` legitimately reports line 5 and is out of scope; do not include it.)
4. Commit hygiene:
   `git show --stat HEAD` -> exactly 6 files, all under `.claude/agents/`.
   `git status --short` -> still shows ` M .claude/agent-memory/research-planning-agent.md` and nothing else.
   `git branch --show-current` -> `main`.
</verification>

<success_criteria>
- All 7 live-observed occurrences of the two nonexistent tool names are gone from `.claude/agents/` (QT-gpa-01).
- `mcp__context7__query-docs` appears exactly 7 times under `.claude/agents/` — 6 frontmatter allowlist entries plus research-planning-agent.md:765 — a one-for-one substitution with no over- or under-reach.
- troubleshoot-agent.md declares both `mcp__context7__resolve-library-id` and `mcp__context7__query-docs`, making its context7 toolset self-sufficient (QT-gpa-02 / SA-01).
- Every edited file's YAML frontmatter still parses: a single `tools:` line at line 4 in all six.
- Exactly one path-scoped commit on `main` touching exactly the 6 agent files; `.claude/agent-memory/research-planning-agent.md` is untouched and still uncommitted (QT-gpa-03).
- No file outside `.claude/agents/` was modified, and no generic "Context7" product-name prose was rewritten.
</success_criteria>

<output>
Create `.planning/quick/260921-gpa-rename-the-7-nonexistent-context7-mcp-to/260921-gpa-SUMMARY.md` when done.

Record in the summary: the before/after occurrence counts (dead names 7 -> 0; query-docs 0 -> 7; resolve-library-id 6 -> 7), the commit SHA, explicit confirmation that SA-01 was applied (or, if the developer declined it, that troubleshoot-agent.md carries query-docs without a resolve entry and why), and confirmation that the agent-memory modification stayed out of the commit.
</output>
