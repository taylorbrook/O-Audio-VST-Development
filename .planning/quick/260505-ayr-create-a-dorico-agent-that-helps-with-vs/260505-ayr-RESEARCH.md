# Quick Task 260505-ayr: Dorico Agent — Research

**Researched:** 2026-05-05
**Domain:** Claude Code subagent authoring (`.claude/agents/*.md`) for Dorico VST integration
**Confidence:** HIGH (everything is repo-grounded; no external lookups needed)

## Summary

Build `.claude/agents/dorico-agent.md` as a single subagent file mirroring `troubleshoot-agent.md`'s frontmatter+body shape but with the **write-capable tool surface from `gui-agent.md`** (Read, Edit, Write, Bash) plus search/web tools. Pair it with a thin `.claude/commands/dorico.md` slash command (precedent: `install-plugin.md`, `research.md`). Ship `.claude/agent-memory/dorico-agent.md` from day one seeded with the three Dorico landmines already captured in user-memory (`critical_dorico_distribution_mechanism.md`, `critical_dorico_keyswitch_routing.md`, the v1.16.6 microtonal regression). The agent's prompt embeds **only a knowledge map** (file types + repo locations + reference plugin), not Dorico XML schemas — the canonical artifacts at `plugins/O-MicrotonalSampler/Resources/dorico/` are the agent's source-of-truth.

**Primary recommendation:** Tool surface `Read, Edit, Write, Bash, Grep, Glob, WebSearch, WebFetch, mcp__context7__resolve-library-id, mcp__context7__get-library-docs`. Slash command at `.claude/commands/dorico.md` with `argument-hint: <PluginName> [question-or-task]`. Memory file pre-seeded with 3 entries.

---

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Form factor:** Single `.claude/agents/dorico-agent.md` mirroring `troubleshoot-agent` pattern; spawnable via `Task(subagent_type="dorico-agent", ...)`.
- **Tool surface:** Read, Edit, Write, Bash, Grep, Glob, WebSearch, WebFetch, mcp__context7__* (write-capable diagnose+advise+edit).
- **Scope coverage:** Microtonal/note-expression playback, Playback Templates, EndpointConfigs, Expression Maps, keyswitch routing, CC/PC technique triggers, Dynamics audit, distribution mechanism. Full Dorico stack.
- **Trigger:** `/dorico [PluginName] [task]` slash command + Task-tool spawnable. No filesystem hooks / auto-trigger.
- **Capabilities:** Diagnose + advise + **edit** (Dorico artifacts AND plugin C++). May spawn deep-research/troubleshoot-agent only if explicitly delegated.
- **Knowledge embedding:** Pull from existing memory + reference files. Agent should *know where to look*, not *embed everything*.

### Claude's Discretion
- Exact YAML frontmatter shape — match `troubleshoot-agent.md` / `gui-agent.md` conventions.
- Slash command location — `.claude/commands/dorico.md` vs `.claude/skills/dorico-helper/SKILL.md`.
- Whether to ship `.claude/agent-memory/dorico-agent.md` from day one or defer.

### Deferred Ideas
- (none)

---

## 1. Agent Scaffold Convention

### YAML frontmatter shape (canonical)

All agents share a tight 5-key frontmatter block. No `model:` key is used in this repo (Claude Code defaults apply). Verified:

| Agent | Lines | Tools | color |
|---|---|---|---|
| `troubleshoot-agent.md` | 807 | `Read, Write, Grep, Glob, Bash, WebSearch, WebFetch, mcp__context7__search_juce_docs` | purple |
| `gui-agent.md` | 1444 | `Read, Edit, Write, Bash, mcp__context7__resolve-library-id, mcp__context7__get-library-docs` | green |
| `dsp-agent.md` | 1239 | `Read, Edit, Write, mcp__context7__resolve-library-id, mcp__context7__get-library-docs` | yellow |
| `validation-agent.md` | — | `Read, Write, Grep, Bash` | blue |

**Frontmatter template** (lines 1-6 of every agent file):
```yaml
---
name: <agent-name>          # kebab-case, matches Task subagent_type
description: <one-or-two sentence trigger description>
tools: <comma-separated tool list>
color: <single color word>
---
```

`description` is the **routing string** — Claude Code uses it to auto-suggest the agent. troubleshoot-agent uses past-tense problem-domain phrasing ("Use when encountering build errors..."); gui-agent uses workflow-stage phrasing ("MUST be invoked by plugin-workflow skill for Stage 3").

### Recommended frontmatter for `dorico-agent`

```yaml
---
name: dorico-agent
description: Dorico integration specialist for VST instruments. Diagnoses and edits Dorico Playback Templates, EndpointConfigs, expression maps (.doricoexpmap / .doricolib), keyswitch routing, CC/PC technique triggers, and the plugin C++ code that backs them. Use when integrating Ouaricon plugins into Dorico, debugging microtonal playback, fixing keyswitch failures, or authoring the .doricolib distribution bundle.
tools: Read, Edit, Write, Bash, Grep, Glob, WebSearch, WebFetch, mcp__context7__resolve-library-id, mcp__context7__get-library-docs
color: orange
---
```

**Rationale:**
- **Tool surface = troubleshoot-agent (Read/Write/Grep/Glob/Bash/Web*) ∪ dsp-agent (Edit + mcp__context7__resolve+get-library-docs).** This matches the locked decision (write-capable).
- `mcp__context7__search_juce_docs` (used by troubleshoot-agent) is JUCE-only — drop in favor of the generic `resolve-library-id` + `get-library-docs` pair, which is what gui-agent and dsp-agent use, since the Dorico agent will need to look up VST3 SDK / Steinberg docs as well as JUCE.
- `color: orange` is unused by other agents (purple/green/yellow/blue/red taken). Visual distinguishability matters in agent-list UIs.
- Description leads with the plugin-suite-specific role and ends with concrete trigger phrases ("debugging microtonal playback", "fixing keyswitch failures", ".doricolib distribution") so Claude Code's auto-router picks it up reliably.

---

## 2. Slash Command Convention

### Repo precedent

- `.claude/commands/` contains 44 `.md` files; `.claude/skills/` contains 28 directories each holding a `SKILL.md`. **Commands and skills are distinct primitives.**
- **Thin command wrappers** (frontmatter + `<routing>` block delegating to a skill or subagent) are well-precedented:
  - `.claude/commands/install-plugin.md` (43 lines): frontmatter (`name`, `description`, `argument-hint`) + `<preconditions>` + `<routing>` block invoking the `plugin-lifecycle` skill.
  - `.claude/commands/research.md` (163 lines): frontmatter + `<routing>` invoking the `deep-research` skill.
  - `.claude/commands/generalize-microtones.md` (66 lines): frontmatter (`description` only) + free-form prose body — no `<routing>` block, just narrative routing instructions.
- **No precedent for a command that wraps a single subagent directly** — every existing command-wrapper points to a skill, and skills sometimes invoke subagents. But there's also no rule against it; subagents are spawned via the Task tool from any context.

### Recommendation: `.claude/commands/dorico.md`

Place the slash command at `.claude/commands/dorico.md` (NOT a SKILL.md wrapper). Reason: this task is one-shot delegation to a single subagent — no multi-step skill orchestration logic required. A SKILL.md would add a directory + indirection for no benefit.

**Recommended shape** (mirrors `install-plugin.md` lines 1-32):
```markdown
---
name: dorico
description: Dorico integration helper — microtonal playback, expression maps, playback templates, keyswitch routing
argument-hint: <PluginName> [question-or-task]
---

# /dorico

<routing>
  <invoke agent="dorico-agent" with="$ARGUMENTS" required="true">
    Spawn dorico-agent via the Task tool with the user's plugin name and task description.
    The agent reads its own memory file and the relevant Dorico reference artifacts on entry.
  </invoke>
</routing>

<background_info>
[brief: 5-10 lines on what the agent covers]
</background_info>
```

---

## 3. Dorico Domain Knowledge Inventory

The canonical reference is **O-MicrotonalSampler v1.16.x** at `plugins/O-MicrotonalSampler/Resources/dorico/`. Full file tree (verified live):

```
plugins/O-MicrotonalSampler/Resources/dorico/
├── INSTALL-DORICO.md                  # End-user install steps + log signals
├── SMOKE-TEST.md                      # 5-test Dorico battery (TC-1..TC-5)
├── EndpointConfigs/
│   ├── O-MicrotonalSampler/
│   │   ├── endpointconfig.xml         # 171 lines — slot+plugin+exp-map binding + instrument family list
│   │   └── playbacktemplatedeps.doricolib  # 1187 lines — 4 expression maps (Strings/Winds/Brass/Generic)
│   ├── O-MicrotonalSampler-Winds/endpointconfig.xml
│   ├── O-MicrotonalSampler-Brass/endpointconfig.xml
│   └── O-MicrotonalSampler-Generic/endpointconfig.xml
└── PlaybackTemplateSpecs/
    └── O-MicrotonalSampler/
        └── playbacktemplatespec.xml   # 40 lines — instrument-family → endpointConfig routing
```

### File-type schema map (what the agent must know to navigate, not embed)

| File type | Top-level XML root | Purpose | Canonical example |
|---|---|---|---|
| `.doricolib` | `<kScoreLibrary>` → `<expressionMapDefinitions>` → `<entities>` → `<ExpressionMapDefinition>` | Bundle of expression-map definitions. **The actual primitive Dorico ingests when dropped in `DefaultLibraryAdditions/`.** | `playbacktemplatedeps.doricolib` |
| `.doricoexpmap` | (legacy/standalone) — same `<ExpressionMapDefinition>` shape | Single expression map. **DOES NOT auto-ingest** (see §6). |
| `endpointconfig.xml` | `<endpointConfig>` → `<slots>` + `<instruments>` | Binds plugin (by VST3 GUID hex) → channel slot → exp-map → instrument-family list. One file per family. | `EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` |
| `playbacktemplatespec.xml` | `<playbackTemplateSpec>` → `<entries>` | User-facing Playback Template; routes instrument families → endpointConfigs. | `PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` |

### Key load-bearing fields (cite into agent prompt)

From `playbacktemplatedeps.doricolib` lines 4-29 (header comment block — the agent MUST treat this comment as gospel):

- **Top-level** in each `<ExpressionMapDefinition>`, between `<applyStageTemplateSettings>` and `<initSwitchData>` (lines 44-46):
  - `<pitchBendRange>2</pitchBendRange>`
  - `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  - **These two fields are load-bearing for microtonal pitch routing.** Per-combination duplicates do NOT substitute. Removing them = silent regression to nearest-12-TET. (See `critical_dorico_keyswitch_routing.md` Layer 1 + agent-memory/troubleshoot-agent.md line 4.)
- **Per `<playingTechniqueCombination>`** (lines 52-83 illustrate one combo):
  - `<exclusionGroup>1</exclusionGroup>` — required for KS firing on technique transitions out of `Ord.` (matches HSO factory shape)
  - `<switchOnActions>` → `<switchOnAction>` → `<type>kKeySwitch</type>` + `<param1>N</param1>` (slot index 0..N) + `<param2>127</param2>` (velocity)
  - `<volumeType>` → `<type>kCC</type>` + `<param1>11</param1>` (CC11/Expression dynamics; or `kNoteVelocity` for velocity-only)
  - `<attackType>` → `<type>kNoteVelocity</type>`

### The 3-layer KS routing stack (from `critical_dorico_keyswitch_routing.md`)

When KS doesn't fire, instrument all three in PARALLEL — each layer is silent on failure:

1. **Distribution artifact (`.doricolib`):** missing per-combo `<exclusionGroup>1</exclusionGroup>` is the most common bug.
2. **Plugin parameter defaults:** `ks_enabled=false`, `technique_count=1`, etc. silently absorb correctly-routed KS. Fix in `createParameterLayout()`.
3. **Dorico project state:** saved per-instance values shadow new binary defaults — must use a FRESH plugin instance (Play mode → Endpoint Setup → trash → re-add) to verify.

NotePerformer is NOT a KS reference (uses `kControlChange`). HSO Symphonic Orchestra IS the KS reference.

### CMake bundling

`plugins/O-MicrotonalSampler/CMakeLists.txt` line 58-59:
```
# VST3 Note Expression microtonal support (Dorico) — D-5: ouaricon_add_module ONLY
ouaricon_add_module(O-MicrotonalSampler note-expression)
```

The Dorico XML/doricolib resources are **NOT currently bundled by CMake** — they ship as user-runnable copy steps in `INSTALL-DORICO.md` (verified by `grep -i dorico CMakeLists.txt`). Future installer work would need to add `install(DIRECTORY Resources/dorico/...)` rules. The agent should know this gap exists.

---

## 4. Embedded vs Referenced Knowledge — Recommended Knowledge Map

The agent prompt should NOT contain Dorico XML schema bodies inline. Instead, embed a **knowledge map** (~20-30 lines) that points to:

```markdown
## Dorico Reference Map

### Canonical reference (read these on every invocation)
- `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` — 4-family expression-map bundle (lines 4-29 contain the load-bearing-fields comment; treat as gospel)
- `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/<family>/endpointconfig.xml` — slot + GUID + exp-map binding (4 files: Strings/Winds/Brass/Generic)
- `plugins/O-MicrotonalSampler/Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` — instrument-family routing
- `plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md` — install paths + log signals
- `plugins/O-MicrotonalSampler/Resources/dorico/SMOKE-TEST.md` — 5-test validation battery (TC-1..TC-5)

### Microtonal / Note-Expression reference
- `plugins/O-Lyrica/` — original validated spike. **PASS state** (auval DEF-24-01 is benign). Do NOT classify auval parameter-meta findings as defects on O-Lyrica.
- `Skill("spike-findings-VST-development")` — auto-loaded skill with VST3 NoteExpression patterns/landmines. Pattern 6 + Landmine 6 cover exp-map structural requirements.
- Shared module: `modules/dsp/note-expression/` (or wherever `/module-list` reports it after extraction)

### Repo memory (auto-load on entry)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_distribution_mechanism.md` — why standalone .doricoexpmap drop fails
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_keyswitch_routing.md` — 3-layer stack (schema + plugin defaults + fresh instance)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md` — O-Lyrica is PASS
- `.claude/agent-memory/troubleshoot-agent.md` line 4 — recurring microtonal regression pattern (top-level fields)
- `.claude/agent-memory/dorico-agent.md` — own memory (seed entries below)

### External docs (when needed)
- Steinberg Dorico SDK / Playback Template authoring guide (web search at runtime)
- VST3 NoteExpression API (Context7 query "VST3 INoteExpressionController")
```

This pattern matches what gui-agent and dsp-agent already do — they reference contracts and stage-N-patterns.md rather than embedding the patterns themselves.

---

## 5. Memory File Pattern

### Convention

`.claude/agent-memory/<agent-name>.md` is a flat markdown file with three sections (verified across all 5 existing files):

```markdown
# <Agent Name> Memory

## Learned Patterns
- [Plugin or "General"]: [one-line description]
- ...

## Common Issues
- [problem]: [diagnostic shortcut]
- ...

## Last Updated
YYYY-MM-DD (note)
```

The agent appends entries at end-of-task per its own `<persistent_memory>` block. troubleshoot-agent.md lines 793-807 specify the convention:
- Format: `- [PluginName]: [one-line description]`
- Cap: 80 lines, drop oldest 20 from "Learned Patterns" when exceeded
- Path: `.claude/agent-memory/<name>.md`

### Recommendation: ship memory from day one

**Yes, ship `.claude/agent-memory/dorico-agent.md` with seed entries.** Reasoning: the three Dorico landmines are already documented as critical-class memories — making the agent re-derive them on first run wastes a session and risks losing them if the regression recurs before the agent learns. Seed with:

```markdown
# Dorico Agent Memory

## Learned Patterns
- General (RECURRING REGRESSION): Microtonal pitch falling back to nearest 12-TET = top-level <pitchBendRange>2</pitchBendRange> + <microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod> missing from each <ExpressionMapDefinition> in playbacktemplatedeps.doricolib. Per-combination duplicates DO NOT substitute. Fix: restore top-level fields, bump <version>, redeploy to ~/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/, full Cmd-Q + relaunch. (Validated O-MicrotonalSampler v1.16.6 on 2026-05-05.)
- General: Standalone .doricoexpmap drops into User/Expression Maps/ are SILENTLY skipped by Dorico. Distribute via Playback Template + .doricolib in DefaultLibraryAdditions/. (Phase 25 Plan 01 reverted at d2c86c5; finding doc at .planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md.)
- General: Dorico keyswitches not firing = check 3 layers in PARALLEL: (1) per-combo <exclusionGroup>1</exclusionGroup> in .doricolib; (2) plugin trigger gates not defaulting false (ks_enabled, technique_count, etc.); (3) FRESH plugin instance (saved project state shadows new defaults). HSO factory map is the KS reference, NOT NotePerformer (which uses kControlChange).
- O-Lyrica: validated spike/reference. auval DEF-24-01 (parameter-meta-flag annotation gap) is benign — NOT a runtime defect. Do not propose fixes unless explicitly requested.

## Common Issues
- Microtonal pitch wrong in Dorico but plugin tests fine: TC-4 of SMOKE-TEST.md (quarter-sharp at C4 with 24-EDO) is the only test that reveals top-level-fields regression. TC-1..TC-3 will all still pass.
- Dorico log shows "Error opening file: invalid file format": .doricolib XML is structurally invalid. Diff against last-known-good in git.
- Dorico Library Manager has no "Import Expression Map" command — only "Import Library" (.doricolib) and "Import Cubase Expression Map". This is by design.

## Last Updated
2026-05-05 (seeded from critical_dorico_*.md and v1.16.6 incident)
```

---

## 6. Pitfalls / Gotchas to Bake Into the Agent Prompt

The agent prompt's `<constraints>` or `<known_landmines>` block must lead with these — they are the failure modes the agent will encounter on day-one. All sourced from repo memory + recent commit messages on `O-MicrotonalSampler`:

| # | Landmine | Source | Why critical |
|---|---|---|---|
| 1 | **Standalone `.doricoexpmap` drops are silently skipped.** Always use Playback Template + `.doricolib` in `DefaultLibraryAdditions/`. | `critical_dorico_distribution_mechanism.md`; reverted commit `d2c86c5`/`cd2c2c6` | Wasted a full plan iteration in Phase 25 |
| 2 | **Top-level `<pitchBendRange>2</pitchBendRange>` + `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` are load-bearing.** Per-combo duplicates DO NOT substitute. | `agent-memory/troubleshoot-agent.md` line 4; `playbacktemplatedeps.doricolib` lines 4-29 (in-file comment) | Recurring regression — broken multiple times in v1.16.x refactors |
| 3 | **Per-combo `<exclusionGroup>1</exclusionGroup>` required for KS firing.** Match HSO factory shape, NOT NotePerformer. | `critical_dorico_keyswitch_routing.md` Layer 1; commit `e8b6a2c` (v1.16.2 — keyswitch-from-notation) | Silent KS-routing failure |
| 4 | **Plugin trigger gates that default `false` silently absorb KS.** Always check `createParameterLayout()` — `ks_enabled`, `technique_count`, `cc_select_enabled`, `pc_enabled`. | `critical_dorico_keyswitch_routing.md` Layer 2; commit `e8b6a2c` (v1.16.2) | TC-5 failed across two patch iterations because layer 2 was diagnosed in isolation |
| 5 | **Saved Dorico project state shadows new binary defaults.** Always test fresh plugin instance: Play mode → Endpoint Setup → trash → re-add. | `critical_dorico_keyswitch_routing.md` Layer 3 | False-negative test results |
| 6 | **DefaultLibraryAdditions loads on app startup, not project open.** Full Cmd-Q + relaunch required after any `.doricolib` redeploy. | `agent-memory/troubleshoot-agent.md` line 4; `INSTALL-DORICO.md` step 3 | Common false-negative |
| 7 | **Dorico C3=60 convention.** KS slots 0..7 → MIDI C-2..G-2 (not C-1..G-1 as in some DAW conventions). | `playbacktemplatedeps.doricolib` line 39 (description) | Off-by-octave KS routing |
| 8 | **Dynamics dual-routing:** `volumeType=kCC param1=11` (CC11/Expression — current default) OR `kNoteVelocity` (velocity-only). User-changeable in Library → Expression Maps. | commit `69208e2` (v1.15.0 — Dynamics audit); `playbacktemplatedeps.doricolib` line 39 | Affects every plugin with dynamics |
| 9 | **Plugin GUID hex** in `endpointconfig.xml` `<pluginID>` is the 16-byte VST3 ClassID in hex. Mismatch = endpointconfig binds to nothing. | `endpointconfig.xml` line 12: `ABCDEF019182FAEB4F7544764F4D7453` | Endpoint config silent fail |
| 10 | **Family-aware Playback Template** routes by `<instrumentFamilies>` strings (e.g. `instrument family.strings`, `instrument family.woodwinds`, `instrument family.brass`) — case-sensitive, dotted, lowercase. Empty `<instrumentFamilies/>` = generic fallback. | commit `9bd0909` (v1.16.5); `playbacktemplatespec.xml` lines 11-37 | Family routing bug |
| 11 | **Launch crash from missing DefaultLibraryAdditions path.** v1.16.1 fixed a startup crash when DefaultLibraryAdditions/ didn't exist on the host. Installer must `mkdir -p` it. | commit `5c823bc` (v1.16.1) | App-level crash, not plugin-level |
| 12 | **O-Lyrica auval DEF-24-01 is BENIGN.** Don't classify as a defect. | `project_o_lyrica_spike_reference.md` | Wasted-effort prevention |

These 12 land in the agent prompt as a numbered checklist the agent can self-reference when triaging Dorico issues.

---

## 7. Orchestration Hooks (integration points to update later — DO NOT modify in this task)

These are existing skills/agents that should learn to delegate to `dorico-agent`. The current task scope is just creating the agent + slash command — these are follow-up tasks the planner should note:

| Skill / Agent | File | Why it should know about dorico-agent |
|---|---|---|
| `troubleshoot-agent` | `.claude/agents/troubleshoot-agent.md` | When the user reports a Dorico-specific symptom (microtonal regression, KS failure), troubleshoot-agent should `Return to invoker recommending dorico-agent` rather than doing graduated web research itself. |
| `plugin-improve` | `.claude/skills/plugin-improve/SKILL.md` | Bug-classification step — Dorico-tagged user reports → route to dorico-agent. |
| `plugin-publishing` | `.claude/skills/plugin-publishing/SKILL.md` | Pre-release validation — must invoke dorico-agent for the SMOKE-TEST.md TC-1..TC-5 battery on every microtonal-cohort plugin. |
| `generalize-microtones` | `.claude/commands/generalize-microtones.md` | Phase B (propagate to 7 plugins) — each plugin's Dorico bring-up should delegate to dorico-agent rather than re-deriving the pattern inline. |
| `gui-agent` | `.claude/agents/gui-agent.md` | When Stage 3 needs to wire a technique-tab strip, the parameter defaults that gate KS routing (Layer 2 in the 3-layer stack) must match exp-map expectations. Cross-reference. |
| `dsp-agent` | `.claude/agents/dsp-agent.md` | When microtonal NoteExpression is in scope, applying tuning to `currentFrequency` BEFORE `trigger(...)` (per spike landmine) is a DSP-side concern that benefits from dorico-agent context. |

**Recommendation:** the planner should add a `## Follow-up tasks` section to PLAN.md listing these 6 integration points as **separate tasks** (not part of agent-creation). They're independent edits and can be sequenced after agent ships.

---

## Sources

### Primary (HIGH confidence — repo-grounded, verified live)
- `.claude/agents/troubleshoot-agent.md` (lines 1-6 frontmatter; 793-807 persistent_memory pattern)
- `.claude/agents/gui-agent.md` (lines 1-6; tool surface)
- `.claude/agents/dsp-agent.md` (lines 1-6; tool surface)
- `.claude/agents/validation-agent.md` (lines 1-6)
- `.claude/agent-memory/troubleshoot-agent.md` (line 4 — recurring regression entry; format convention)
- `.claude/agent-memory/gui-agent.md` (format convention)
- `.claude/commands/install-plugin.md` (thin slash-command wrapper precedent, lines 1-32)
- `.claude/commands/research.md` (skill-delegating slash command, lines 1-27)
- `.claude/commands/generalize-microtones.md` (free-form prose command precedent)
- `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` (lines 1-120; in-file gospel comment lines 4-29)
- `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` (full file — 171 lines)
- `plugins/O-MicrotonalSampler/Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` (full file — 40 lines)
- `plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md` (head section; install paths + log signals)
- `plugins/O-MicrotonalSampler/CMakeLists.txt` line 58-59 (note-expression module wire-up; **no Dorico Resources install rules**)
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_distribution_mechanism.md`
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_keyswitch_routing.md`
- `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/project_o_lyrica_spike_reference.md`
- Recent commits on main: `9bd0909` (v1.16.5), `e8b6a2c` (v1.16.2), `5c823bc` (v1.16.1), `7e56e16` (v1.16.0), `69208e2` (v1.15.0)

### Secondary / Tertiary
None needed — task is pure scaffolding off existing repo conventions.

---

## Metadata

**Confidence breakdown:**
- Frontmatter shape: HIGH — verified across 5 agent files
- Slash command shape: HIGH — verified `install-plugin.md`, `research.md`, `generalize-microtones.md`
- Dorico domain knowledge: HIGH — all artifacts present in repo, recent commits map cleanly to v1.16.x state
- Memory pattern: HIGH — 5 existing agent-memory files all follow same format
- Integration hooks: MEDIUM — identified the 6 hooks but did not read every skill body to confirm best insertion point. The planner / executor should read each before editing.

**Research date:** 2026-05-05
**Valid until:** 2026-06-05 (30 days — stable convention, low-volatility codebase)
