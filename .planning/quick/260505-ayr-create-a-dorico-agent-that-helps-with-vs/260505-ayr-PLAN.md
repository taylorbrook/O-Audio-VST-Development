---
quick_id: 260505-ayr
description: Create a Dorico agent (subagent + slash command + seed memory) for VST instrument integration in Steinberg Dorico
mode: quick
created: 2026-05-05
plan_type: scaffolding
---

# Quick Task 260505-ayr: Dorico Agent — Plan

## Goal

Create three files that together deliver a write-capable Dorico-integration subagent:

1. `.claude/agents/dorico-agent.md` — main subagent definition (frontmatter + body)
2. `.claude/commands/dorico.md` — `/dorico` slash command that spawns the subagent
3. `.claude/agent-memory/dorico-agent.md` — seed persistent-memory file with day-one entries

All decisions are LOCKED per `260505-ayr-CONTEXT.md`. All conventions and content are sourced from `260505-ayr-RESEARCH.md` (no re-research).

---

## Tasks

### Task 1 — Create the Dorico subagent definition

**File:** `.claude/agents/dorico-agent.md`

**Action:** Author a new file using the convention verified in research §1. Frontmatter MUST be the canonical 5-key shape (no `model:` key — Claude Code defaults apply):

```yaml
---
name: dorico-agent
description: Dorico integration specialist for VST instruments. Diagnoses and edits Dorico Playback Templates, EndpointConfigs, expression maps (.doricoexpmap / .doricolib), keyswitch routing, CC/PC technique triggers, and the plugin C++ code that backs them. Use when integrating Ouaricon plugins into Dorico, debugging microtonal playback, fixing keyswitch failures, or authoring the .doricolib distribution bundle.
tools: Read, Edit, Write, Bash, Grep, Glob, WebSearch, WebFetch, mcp__context7__resolve-library-id, mcp__context7__get-library-docs
color: orange
---
```

**Body sections** (in order, mirroring the structure in `troubleshoot-agent.md` and `gui-agent.md`):

1. **`# Dorico Agent`** — title + 2-3 sentence purpose paragraph: who I am, what I do, when I should be used.
2. **`<scope>`** — one-paragraph statement of full Dorico-stack coverage from CONTEXT.md decisions (microtonal/note-expression, Playback Templates, EndpointConfigs, expression maps, keyswitch routing, CC/PC technique triggers, dynamics audit, .doricolib distribution).
3. **`<entry_protocol>`** — explicit reading list the agent runs on every invocation:
   - `.claude/agent-memory/dorico-agent.md` (own memory)
   - `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_distribution_mechanism.md`
   - `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_keyswitch_routing.md`
   - `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md`
   - The relevant plugin's `Resources/dorico/` tree if it exists (use `Glob` to discover)
4. **`## Dorico Reference Map`** — knowledge map from research §4 (~25 lines). Lists canonical reference files, microtonal/note-expression references, repo memory locations, and external doc lookups. **DO NOT inline Dorico XML schema.**
5. **`<known_landmines>`** — the 12-item numbered checklist from research §6, verbatim. This is the agent's day-one triage primer. Each entry: short rule + source citation.
6. **`<workflow>`** — graduated-investigation pattern (mirrors troubleshoot-agent's "Levels"):
   - **Level 1** (5 min): Read entry-protocol files. Match symptom against the 12 landmines. If matched → return diagnosis + targeted fix.
   - **Level 2** (15 min): Inspect the relevant plugin's `Resources/dorico/` artifacts. Cross-check the 3-layer KS stack (schema / plugin defaults / fresh instance reminder).
   - **Level 3** (30 min): Diff against canonical O-MicrotonalSampler v1.16.x reference. Run `git log --oneline -- plugins/<Plugin>/Resources/dorico/` to find recent changes.
   - **Level 4** (escalate): If still unresolved, recommend `troubleshoot-agent` for build/auval issues OR `deep-research` for novel Steinberg-side concerns. Do NOT auto-spawn.
7. **`<capabilities>`** — explicit statement that the agent MAY edit:
   - Dorico XML artifacts: `.doricolib`, `.doricoexpmap`, `endpointconfig.xml`, `playbacktemplatespec.xml`
   - Plugin C++ source: `plugins/<Plugin>/Source/PluginProcessor.{h,cpp}`, `PluginEditor.{h,cpp}`, parameter-layout code (Layer 2 of KS stack)
   - Plugin `CMakeLists.txt` for Dorico-resource bundling rules
   - Build/install/cache-reset shell commands per `CLAUDE.md` (kill AudioComponentRegistrar, clear AU cache, install fresh)
8. **`<output_contract>`** — what the agent returns to its caller:
   - Plain markdown summary: ## DIAGNOSIS, ## ROOT CAUSE, ## FIX APPLIED (or ## RECOMMENDED FIX if no edit was authorized), ## VERIFICATION STEPS (cite TC-1..TC-5 from `SMOKE-TEST.md` when applicable), ## REMAINING RISKS.
   - File-paths-and-line-numbers format for any edits made.
9. **`<persistent_memory>`** — append-on-completion protocol (mirrors troubleshoot-agent lines 793-807):
   - Path: `.claude/agent-memory/dorico-agent.md`
   - Format: `- [PluginName or "General"]: <one-line description>`
   - Cap: 80 lines, drop oldest 20 from "Learned Patterns" when exceeded
   - Update "Last Updated" line every write.

**Files modified:** `.claude/agents/dorico-agent.md` (created)

**Verify:**
- File exists and is non-empty: `test -s .claude/agents/dorico-agent.md`
- Frontmatter parses cleanly: `head -10 .claude/agents/dorico-agent.md` shows valid 5-key YAML between `---` markers
- All 12 landmines from research §6 are present: `grep -c '^| [0-9]\+ |' .claude/agents/dorico-agent.md` ≥ 12 (or equivalent enumeration if rendered as a numbered list)
- All 5 entry-protocol references resolve (each path exists)

**Done when:** file is committed and the smoke check above passes.

---

### Task 2 — Create the `/dorico` slash command

**File:** `.claude/commands/dorico.md`

**Action:** Author a new thin command-wrapper file mirroring `install-plugin.md` (lines 1-32 of that file are the template). Frontmatter:

```yaml
---
name: dorico
description: Dorico integration helper — microtonal playback, expression maps, playback templates, keyswitch routing, CC/PC technique triggers
argument-hint: <PluginName> [question-or-task]
---
```

**Body:**

1. **`# /dorico`** — title + 1 sentence purpose.
2. **`<preconditions>`** — must run from project root; `<PluginName>` must resolve to a directory under `plugins/`.
3. **`<routing>`** — instruction block: parse `$ARGUMENTS` into `<plugin>` and `<task>`, spawn `dorico-agent` via `Task(subagent_type="dorico-agent", ...)` with a prompt that includes the plugin name, the user's task description, and a pointer to the plugin's `Resources/dorico/` directory. Mirror the routing-block style from `install-plugin.md`.
4. **`<background_info>`** — 8-12 lines summarizing what the agent covers (full Dorico stack), typical use cases (microtonal regression, KS failure, distribution bundle authoring, SMOKE-TEST.md walkthrough), and one-line pointer to canonical reference (O-MicrotonalSampler v1.16.x).
5. **`<examples>`** — 3 example invocations:
   - `/dorico O-MicrotonalSampler microtonal pitch wrong in Dorico TC-4`
   - `/dorico O-Lyrica add Dorico distribution bundle (Playback Template + EndpointConfig)`
   - `/dorico O-Reed keyswitches not firing for Staccato → Legato transition`

**Files modified:** `.claude/commands/dorico.md` (created)

**Verify:**
- File exists: `test -s .claude/commands/dorico.md`
- Frontmatter has all three keys: `grep -E '^(name|description|argument-hint):' .claude/commands/dorico.md | wc -l` = 3
- `<routing>` block references `dorico-agent` (not a non-existent skill): `grep -q 'dorico-agent' .claude/commands/dorico.md`

**Done when:** file is committed and smoke checks pass.

---

### Task 3 — Create the seed agent-memory file

**File:** `.claude/agent-memory/dorico-agent.md`

**Action:** Author the day-one memory file using the exact 4 seed entries from research §5. Format mirrors all 5 existing `.claude/agent-memory/*.md` files (verified):

```markdown
# Dorico Agent Memory

## Learned Patterns
- General (RECURRING REGRESSION): <full text from research §5 entry 1>
- General: <full text from research §5 entry 2 — standalone .doricoexpmap drops>
- General: <full text from research §5 entry 3 — 3-layer KS routing>
- O-Lyrica: <full text from research §5 entry 4 — auval DEF-24-01 benign>

## Common Issues
- <full text from research §5 issue 1 — TC-4 reveals top-level fields regression>
- <full text from research §5 issue 2 — invalid file format>
- <full text from research §5 issue 3 — Library Manager has no Import Expression Map>

## Last Updated
2026-05-05 (seeded from critical_dorico_*.md and v1.16.6 incident)
```

**Files modified:** `.claude/agent-memory/dorico-agent.md` (created)

**Verify:**
- File exists: `test -s .claude/agent-memory/dorico-agent.md`
- Has 4 seed pattern entries: `grep -c '^- ' .claude/agent-memory/dorico-agent.md` ≥ 7 (4 patterns + 3 issues)
- Has all three section headers: `grep -E '^## (Learned Patterns|Common Issues|Last Updated)' .claude/agent-memory/dorico-agent.md | wc -l` = 3

**Done when:** file is committed.

---

## Out of scope (follow-up tasks)

Per research §7, six existing skills/agents should learn to delegate to `dorico-agent`. **These are NOT part of this quick task.** Note them as separate follow-up work:

1. `.claude/agents/troubleshoot-agent.md` — recommend `dorico-agent` on Dorico symptoms
2. `.claude/skills/plugin-improve/SKILL.md` — Dorico-tagged bug routing
3. `.claude/skills/plugin-publishing/SKILL.md` — pre-release SMOKE-TEST.md validation
4. `.claude/commands/generalize-microtones.md` — Phase B per-plugin delegation
5. `.claude/agents/gui-agent.md` — Stage 3 KS-routing parameter cross-reference
6. `.claude/agents/dsp-agent.md` — note-expression tuning/trigger order callout

The user can spin these up with separate `/gsd-quick` calls once the Dorico agent ships.

---

## Done criteria

- [ ] `.claude/agents/dorico-agent.md` exists with valid 5-key frontmatter, all 12 landmines, knowledge map, workflow levels, output contract, and persistent_memory protocol
- [ ] `.claude/commands/dorico.md` exists with valid frontmatter (`name`, `description`, `argument-hint`), `<routing>` block spawning `dorico-agent`, and 3 example invocations
- [ ] `.claude/agent-memory/dorico-agent.md` exists with 4 seed pattern entries + 3 common-issue entries + Last Updated stamp
- [ ] All file references inside the agent body resolve (no broken paths)
- [ ] Three files committed atomically with message `feat(dorico-agent): create Dorico integration subagent + /dorico slash command`
