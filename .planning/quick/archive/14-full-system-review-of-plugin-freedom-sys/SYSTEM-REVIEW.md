# Plugin Freedom System -- Full System Review

## Date: 2026-03-05

## Executive Summary

- **Total PFS size: 3.6 MB across 443 files (79,924 lines), estimating ~632K tokens.** The skills subsystem alone accounts for 54% of all tokens (~344K). If research docs are included (under `research/`), the full ecosystem is ~1.1M tokens.
- **10 dead `.sh` hook files (816 lines) remain alongside their `.py` replacements** -- a Python migration was completed but the old shell scripts were never deleted. Additionally, 3 hook event types defined in `hooks.json` are not registered in `settings.json` and never fire.
- **3 agent definitions are never referenced anywhere** (aesthetics-agent, dynamic-researcher, music-theory-agent) -- a combined 473 lines / ~3,600 tokens of dead agent definitions. Additionally, 4 of 5 agent memory files are empty placeholders.
- **The skills subsystem (27 skills, 229 files, 44,792 lines) is the dominant context cost** but uses a lazy-loading architecture (SKILL.md indexes + on-demand rules/) that mitigates this. The actual per-invocation cost is the SKILL.md index (~130-680 lines per skill) rather than the full skill tree.
- **`hooks.json` is entirely vestigial** -- `settings.json` is the authoritative hook configuration for Claude Code. The two files have diverged significantly, with `hooks.json` defining hooks (Stop, SubagentStop, UserPromptSubmit) that are not active, and `settings.json` defining hooks (SubagentStart, TaskCompleted) that are not in `hooks.json`.

---

## System Inventory

| Subsystem | Files | Lines | Disk Size | Est. Tokens | % of Total |
|-----------|-------|-------|-----------|-------------|------------|
| Skills | 229 | 44,792 | 1.8 MB | ~344K | 54.4% |
| Agents + Agent Memory | 19 | 9,990 | 352 KB | ~79K | 12.5% |
| Hooks + Validators | 47 | 5,659 | 388 KB | ~48K | 7.6% |
| Commands | 44 | 6,569 | 260 KB | ~47K | 7.4% |
| Aesthetics | 19 | 4,906 | N/A | ~26K | 4.1% |
| Templates | 23 | 2,998 | 132 KB | ~22K | 3.5% |
| Scripts | 10 | 2,187 | N/A | ~18K | 2.8% |
| Schemas | 28 | 2,154 | 124 KB | ~17K | 2.7% |
| Top-level configs | 13 | ~1,200 | N/A | ~14K | 2.2% |
| Critics | 4 | 1,338 | N/A | ~12K | 1.9% |
| Utils | 3 | 691 | N/A | ~5K | 0.8% |
| References | 1 | 137 | 4 KB | ~1K | 0.2% |
| **Total .claude/** | **443** | **79,924** | **3.6 MB** | **~632K** | **100%** |
| Research docs (separate) | 54 | 52,992 | 1.9 MB | ~480K | (external) |

---

## Subsystem Analysis

### 1. Hooks and Validators

#### Current State

Two competing configuration files exist:
- **`settings.json`** (76 lines) -- the **authoritative** Claude Code hook configuration. This is what Claude Code actually reads and executes.
- **`hooks.json`** (104 lines) -- a PFS-internal config file that is **not loaded by Claude Code**. It appears to be documentation or a legacy config from before the migration to the official format.

**Active hooks** (from `settings.json`):

| Event | Hook | Timeout | Purpose | Value |
|-------|------|---------|---------|-------|
| SessionStart | SessionStart.py | 5s | Env validation (python3, cmake, JUCE, etc.) | HIGH -- catches missing deps early |
| SessionStart (compact) | PostCompact-SessionStart.py | 5s | Restore context after compaction | MEDIUM -- restores active plugin state |
| PostToolUse (Write/Edit) | PostToolUse.py | 2s | RT safety checks, contract immutability, silent failure detection | HIGH -- prevents audio bugs at write time |
| PreCompact | PreCompact.py | 10s | Write domain-aware snapshot before compaction | MEDIUM -- preserves active context |
| SubagentStart (7 agents) | inject-agent-memory.py | 3s | Inject persistent memory for agents | LOW -- only 1 of 5 memory files has content |
| TaskCompleted | task-validator-dispatch.py | 15s | Route to domain validators based on task content | MEDIUM -- automated quality gate |

**Dead hooks** (in `hooks.json` but NOT in `settings.json`):

| Event | Hook | Status |
|-------|------|--------|
| Stop | Stop.py | DEAD -- never fires; stage commit verification |
| SubagentStop | SubagentStop.py | DEAD -- never fires; contract validation after subagent |
| UserPromptSubmit | UserPromptSubmit.py | DEAD -- never fires; /continue context injection |

The SubagentStop hook (209 lines) contains the most sophisticated validation logic in the system (contract checksums, cross-contract validation, stage-specific dispatch) but it **never executes** because it is not registered in `settings.json`.

#### Findings

1. **10 duplicate `.sh` files are dead code.** Every `.py` hook has a corresponding `.sh` file from before the Python migration. `settings.json` only references `.py` files. Total dead code: 816 lines across 10 files.

2. **`hooks.json` is entirely vestigial.** It has diverged from `settings.json` and serves no runtime purpose. Its PostToolUse section has a wildcard matcher (`.*`) plus two additional validators (research-frontmatter, regenerate-manifest) that are not in `settings.json` -- meaning these validators never fire either.

3. **The `validate-research-frontmatter.py` and `regenerate-manifest.py` hooks only exist in `hooks.json`** and therefore never execute. Research frontmatter is not being validated on write. The resource-index.json is not being auto-regenerated.

4. **3 `__pycache__` directories (156 KB, 15 files) are in the repo** and not gitignored. These are Python bytecode artifacts that should not be tracked.

5. **The validation cache (`cache/validation-results.json`) is empty** -- a 3-byte file containing `{}`. The caching system described in `utils/validation-cache.md` (231 lines) and `utils/validation-cache.sh` (172 lines) appears unused.

6. **PostToolUse.py fires on Write/Edit to plugin source files** and runs real-time safety checks (heap allocation, mutex, I/O in processBlock) plus silent failure pattern detection via a subprocess call to `validate-silent-failures.py`. This is valuable but adds ~2s latency per write.

7. **The TaskCompleted hook (task-validator-dispatch.py)** intelligently routes validation based on task content keywords but requires the `ACTIVE_PLUGIN` environment variable or an O-PluginName pattern in the task description to function. If neither is available, it silently skips all validation.

#### Issues

| # | Issue | Severity | Files |
|---|-------|----------|-------|
| H-1 | 10 dead `.sh` hook files (816 lines) | Low | `.claude/hooks/*.sh` |
| H-2 | `hooks.json` vestigial, diverged from `settings.json` | Medium | `.claude/hooks/hooks.json` |
| H-3 | SubagentStop.py never fires (not in settings.json) | High | `.claude/hooks/SubagentStop.py` |
| H-4 | Stop.py never fires (not in settings.json) | Low | `.claude/hooks/Stop.py` |
| H-5 | UserPromptSubmit.py never fires | Medium | `.claude/hooks/UserPromptSubmit.py` |
| H-6 | Research frontmatter validation not active | Medium | `.claude/hooks/validators/validate-research-frontmatter.py` |
| H-7 | Resource index auto-regeneration not active | Low | `.claude/hooks/regenerate-manifest.py` |
| H-8 | `__pycache__` dirs in repo, not gitignored | Low | 3 directories, 15 files |
| H-9 | Validation cache always empty | Low | `.claude/cache/validation-results.json` |

---

### 2. Agents and Agent Memory

#### Current State

14 agent definitions totaling 9,931 lines (~79K tokens). The largest agents are:

| Agent | Lines | Referenced In | Status |
|-------|-------|---------------|--------|
| gui-agent | 1,444 | 24 files | Active -- core implementation agent |
| ui-design-agent | 1,291 | 5 files | Active -- UI mockup generation |
| ui-finalization-agent | 1,244 | 4 files | Active -- scaffolding generation |
| dsp-agent | 1,239 | 22 files | Active -- core DSP implementation |
| foundation-shell-agent | 1,025 | 16 files | Active -- project scaffolding |
| research-planning-agent | 990 | 8 files | Active -- research orchestration |
| validation-agent | 950 | 14 files | Active -- quality checking |
| troubleshoot-agent | 807 | 2 files | Active -- build debugging |
| music-theory-agent | 193 | 0 files | **DEAD -- never referenced** |
| critic-orchestrator | 173 | 2 files | Low use |
| polish-agent | 166 | 10 files | Active -- Stage 4 |
| aesthetics-agent | 145 | 0 files | **DEAD -- never referenced** |
| dynamic-researcher | 135 | 0 files | **DEAD -- never referenced** |
| research-lead | 129 | 2 files | Low use |

**3 agents are never referenced:** aesthetics-agent (145 lines), dynamic-researcher (135 lines), music-theory-agent (193 lines). Combined: 473 lines / ~3,600 tokens.

#### Findings

1. **Three UI-related agents exist with overlapping domains:**
   - `gui-agent` (1,444 lines) -- WebView integration, parameter binding, UI implementation
   - `ui-design-agent` (1,291 lines) -- Visual mockup generation, layout design
   - `ui-finalization-agent` (1,244 lines) -- Scaffolding and finalization

   These three agents total 3,979 lines (~30K tokens). While they have distinct roles in the stage workflow, there is likely significant shared boilerplate (JUCE conventions, WebView patterns, CSS approaches).

2. **Agent memory is nearly unused.** Of 5 memory files:
   - `research-planning-agent.md`: 19 lines with 2 actual learned patterns (valuable)
   - `dsp-agent.md`, `gui-agent.md`, `troubleshoot-agent.md`, `validation-agent.md`: All 10 lines each, containing empty "No patterns recorded yet" placeholders

   The `inject-agent-memory.py` SubagentStart hook fires for 7 agents but only 1 has meaningful content. The hook adds 3s timeout overhead for 4 empty file reads per session.

3. **`agent-profiles.json` is explicitly documentation-only.** Its own comment states: "This is a convention/documentation file, not runtime config. Claude Code does not support per-agent effort in frontmatter." It has no runtime effect.

4. **The top 4 agents (gui-agent, ui-design-agent, dsp-agent, foundation-shell-agent) consume 5,000+ lines.** Each agent definition includes extensive inline domain knowledge (JUCE API patterns, WebView communication protocols, DSP algorithms). This is necessary for quality output but represents a massive context cost when spawned.

#### Issues

| # | Issue | Severity | Files |
|---|-------|----------|-------|
| A-1 | 3 dead agents never referenced (473 lines) | Low | `aesthetics-agent.md`, `dynamic-researcher.md`, `music-theory-agent.md` |
| A-2 | 4 of 5 agent memory files are empty placeholders | Low | `agent-memory/*.md` |
| A-3 | `agent-profiles.json` has no runtime effect | Low | `.claude/agent-profiles.json` |
| A-4 | 3 UI agents with likely shared boilerplate | Medium | `gui-agent.md`, `ui-design-agent.md`, `ui-finalization-agent.md` |

---

### 3. Skills

#### Current State

27 skills containing 229 files (44,792 lines, ~344K tokens). This is the **largest subsystem by far**, accounting for 54% of all PFS tokens.

The skill architecture uses a **lazy-loading pattern**:
- Each skill has a `SKILL.md` index (~130-680 lines) that is loaded on demand
- Additional `rules/*.md` files are loaded only when needed during implementation
- No skill has an `AGENTS.md` file (the heavy context pattern was avoided)

**Top 10 skills by total content:**

| Skill | SKILL.md | Rules Files | Total Lines | Est. Tokens |
|-------|----------|-------------|-------------|-------------|
| plugin-workflow | 683 | 23 | 5,944 | ~45K |
| ui-template-library | 267 | 12 | 4,338 | ~33K |
| ui-mockup | 360 | 19 | 5,252 | ~40K |
| plugin-planning | 148 | 16 | 3,640 | ~28K |
| improve-milestone | 668 | 11 | 2,905 | ~22K |
| plugin-improve | 566 | 14 | 2,790 | ~21K |
| plugin-ideation | 124 | 12 | 2,241 | ~17K |
| system-setup | 450 | 6 | 2,132 | ~16K |
| plugin-lifecycle | 269 | 11 | 2,091 | ~16K |
| plugin-testing | 400 | 9 | 1,955 | ~15K |

#### Findings

1. **Lazy loading is working well.** Commands reference skills by name (e.g., `skills/state-recovery`), and only the SKILL.md index is loaded initially. Rules files are loaded on demand during execution. This is the correct architecture for this scale.

2. **10 of 27 skills are `plugin-*` prefixed**, creating a confusing namespace:
   - `plugin-context` -- plugin directory layout knowledge
   - `plugin-ideation` -- new plugin brainstorming
   - `plugin-improve` -- improving existing plugins
   - `plugin-lifecycle` -- install/uninstall/destroy
   - `plugin-packaging` -- building release packages
   - `plugin-phases` -- GSD phase commands
   - `plugin-planning` -- creative brief and spec generation
   - `plugin-publishing` -- publishing to marketplace
   - `plugin-testing` -- automated and manual testing
   - `plugin-workflow` -- stage 1-4 orchestration

   `plugin-phases` (477 lines, 0 rules) and `plugin-workflow` (683 lines, 23 rules, 5,944 total) have heavily overlapping descriptions. `plugin-phases` describes "GSD-style phase commands" while `plugin-workflow` describes "stage 1-4 with GSD phase cycles." This naming ambiguity could cause incorrect skill selection.

3. **6 skills have no rules/ files at all** -- their entire content is in SKILL.md:
   - `build-installer` (77 lines) -- very minimal
   - `contract-validation` (242 lines)
   - `module-system` (532 lines)
   - `plugin-context` (223 lines)
   - `plugin-phases` (477 lines)
   - `session-checkpoint` (363 lines)
   - `state-recovery` (299 lines)
   - `state-validation` (233 lines)
   - `workflow-reconciliation` (285 lines)

   These 9 self-contained skills could potentially be simplified or merged.

4. **Only 9 of 44 commands explicitly reference skills** (via `skills/` paths in their text). The remaining 35 commands either inline their knowledge or rely on implicit skill activation. This means the skill system's value is concentrated in a few key workflows.

5. **The `aesthetic-dreaming` skill (1,922 total lines) is the most context-expensive specialty skill.** It covers theme generation and aesthetic specification -- a creative domain that may not need this depth of guidance.

#### Issues

| # | Issue | Severity | Files |
|---|-------|----------|-------|
| S-1 | `plugin-phases` and `plugin-workflow` overlap significantly | Medium | Both skills |
| S-2 | 35 of 44 commands don't reference skills at all | Low | N/A |
| S-3 | 10 `plugin-*` prefixed skills create naming confusion | Low | N/A |
| S-4 | `aesthetic-dreaming` may be over-engineered (1,922 lines for theme generation) | Low | `skills/aesthetic-dreaming/` |

---

### 4. Commands

#### Current State

44 commands totaling 6,569 lines (~47K tokens). Commands range from 43 lines (`install-plugin`) to 377 lines (`plugin-handoff`).

**Command families:**

| Family | Commands | Total Lines | Purpose |
|--------|----------|-------------|---------|
| plugin-* | 13 | 2,317 | Plugin development lifecycle |
| module-* | 7 | 1,512 | Module system management |
| Standalone | 24 | 2,740 | Build, test, research, etc. |

**Top 10 commands by size (potential context cost):**

| Command | Lines | Referenced Skills |
|---------|-------|-------------------|
| plugin-handoff | 377 | None |
| module-upgrade | 365 | None |
| module-upgrade-all | 302 | None |
| plugin-critique | 289 | None |
| plugin-focus | 286 | state-validation |
| continue | 262 | session-checkpoint, state-recovery, state-validation |
| plugin-verify | 252 | None |
| improve-milestone | 243 | improve-milestone |
| module-add | 224 | None |
| plugin-execute | 224 | None |

#### Findings

1. **The `/plugin-*` family has 13 commands:**
   `discuss`, `research`, `plan`, `execute`, `verify`, `handoff`, `critique`, `resume`, `pause`, `focus`, `status`, `list`, `improve`

   This is a complete lifecycle but some commands are minimal:
   - `plugin-list`: 46 lines (just reads PLUGINS.md)
   - `plugin-pause`: 62 lines (updates STATUS.md)
   - `plugin-resume`: 65 lines (reads STATUS.md)
   - `plugin-status`: 70 lines (reads STATUS.md)

   These 4 commands (243 lines combined) could be consolidated into a single `plugin-info` command with subcommands.

2. **The module-* family has 7 commands** for a module system:
   `module-add`, `module-create`, `module-info`, `module-list`, `module-remove`, `module-upgrade`, `module-upgrade-all`, `modules`

   Total: 1,512 lines. The `module-upgrade` (365 lines) and `module-upgrade-all` (302 lines) are the two largest. The module system is a v1.2 feature -- these commands should be evaluated for actual usage frequency.

3. **Most commands inline their logic rather than delegating to skills.** Only 9 commands reference skills. Large commands like `plugin-handoff` (377 lines) and `plugin-critique` (289 lines) contain all their orchestration logic inline, which means updating behavior requires editing command files rather than shared skill rules.

4. **`/pfs` (136 lines) is a meta-command** that describes the system itself. It references skills directory for listing but otherwise is documentation-as-command.

#### Issues

| # | Issue | Severity | Files |
|---|-------|----------|-------|
| C-1 | 4 minimal plugin-* commands could be consolidated | Low | `plugin-list`, `plugin-pause`, `plugin-resume`, `plugin-status` |
| C-2 | Module family (7 commands, 1,512 lines) -- evaluate usage frequency | Low | `module-*.md` |
| C-3 | 35 commands don't reference skills (inline logic) | Medium | Various |
| C-4 | `plugin-handoff` (377 lines) is largest command with no skill delegation | Low | `plugin-handoff.md` |

---

### 5. Research Documents

#### Current State

54 research documents in `research/` totaling 52,992 lines (~480K tokens, 1.9 MB). These are NOT part of `.claude/` but are referenced by the PFS resource system.

**Resource index (`resource-index.json`):** Contains 27 entries for 54 actual files -- **50% coverage gap**. The auto-regeneration hook (`regenerate-manifest.py`) is defined in `hooks.json` but not in `settings.json`, so the index is not being kept current.

**Document types in index:**
- algorithm: 6
- guide: 7
- pattern: 1
- reference: 13

#### Findings

1. **Frontmatter consistency is mixed.** Sampling the first 5 documents:
   - `2d-scatter-plot-concatenative-synthesis.md`: Full YAML frontmatter (title, created, domain, type, keywords)
   - `ambisonics-binaural-decoding-deep-dive.md`: No YAML frontmatter (uses inline headers instead)
   - `ambisonics-encoding-deep-dive.md`: No YAML frontmatter
   - `circuit-modeling-fundamentals.md`: Full YAML frontmatter
   - `concatenative-synthesis-comprehensive.md`: No frontmatter at all

   At least 2 of 5 sampled docs lack YAML frontmatter entirely. The frontmatter validator (`validate-research-frontmatter.py`, 272 lines) exists but is not active (only in `hooks.json`).

2. **`frontmatter-issues.txt` logs only 1 known issue** (`plugin-development-without-juce.md`) but the actual problem is much larger based on sampling.

3. **Research docs are the second largest token cost** (~480K tokens) after skills (~344K). They are loaded on demand via agent research phases, not at session start, so the per-session impact is limited.

4. **The `resource-index.json` (740 lines, ~5,700 tokens)** is loaded by `discover-resources.py` and `inject-context.py` scripts. With 27 of 54 files indexed, it provides incomplete coverage.

#### Issues

| # | Issue | Severity | Files |
|---|-------|----------|-------|
| R-1 | Resource index covers only 27/54 docs (50% gap) | Medium | `resource-index.json` |
| R-2 | Research frontmatter validation not active | Medium | See H-6 |
| R-3 | Mixed frontmatter formats across research docs | Medium | `research/*.md` |
| R-4 | Resource index auto-regeneration not working | Medium | See H-7 |

---

### 6. Templates, Schemas, and Supporting Infrastructure

#### Current State

**Templates (23 files, 2,998 lines, ~22K tokens):**
- Code snippets: 9 YAML files covering cmake, dsp, parameter-binding, webview patterns
- Prose patterns: 7 YAML files covering architecture, dsp, ui documentation patterns
- Plugin planning: 3 MD templates (BRIEF, REQUIREMENTS, STATUS)
- Meta: README.md, registry.yaml

**Schemas (28 files, 2,154 lines, ~17K tokens):**
- Agent contracts: 11 input/output schema pairs for 6 agent types
- Plugin registry schema
- Subagent report schema
- Validator report schema
- Phase constants

**Critics (4 files, 1,338 lines, ~12K tokens):**
- `critic-architecture.md`: 253 lines
- `critic-dsp.md`: 423 lines
- `critic-foundation.md`: 287 lines
- `critic-ui.md`: 375 lines

**Aesthetics (19 files, 4,906 lines, ~26K tokens):**
- 5 theme definitions: ouaricon-naturalist, studio-hardware, swiss-minimal, vintage-bakelite, vintage-hardware
- Each theme has an `aesthetic.md` description and `metadata.json`
- 3 themes have test-preview HTML files

**Scripts (10 files, 2,187 lines, ~18K tokens):**
- `inject-context.py`: 429 lines -- context injection for agent spawning
- `plugin-registry.py`: 332 lines -- registry management
- `verify-freshness.py`: 270 lines -- research document freshness checking
- `discover-resources.py`: 264 lines -- resource discovery
- `generate-resource-index.py`: 208 lines -- index generation
- `template-lookup.py`: 221 lines -- template resolution
- Plus canary test scripts (267 lines combined) and create-digest.sh (196 lines)

**Utils (3 files, 691 lines, ~5K tokens):**
- `sync-brief-from-mockup.sh`: 288 lines
- `validation-cache.md`: 231 lines (documentation)
- `validation-cache.sh`: 172 lines (implementation)

#### Findings

1. **Schemas are referenced but not programmatically enforced.** The README states "All JSON reports must validate against these schemas" but grep reveals no actual `jsonschema` validation calls. They serve as documentation contracts -- valuable for consistency but not automated.

2. **Critics are used by 3 commands** (`plugin-critique`, `plugin-execute`, `research`) and by `merge-critic-reports.py` and `pre-stage-scan.py`. They provide domain-specific code review checklists. Usage is infrequent but impactful when invoked.

3. **Aesthetics themes (26K tokens) are loaded by the aesthetic-dreaming skill and ui-mockup skill.** The 5 test-preview HTML files (3 themes have them) are development artifacts that could be excluded from the repo or moved to a dev-only location.

4. **The template system has a registry.yaml and lookup script** but is referenced by only 1 command (`/templates`). Templates are more commonly accessed directly by skills that know the paths.

5. **`preferences-README.md` (453 lines, ~2,600 tokens)** explains the preferences system in detail but is loaded as a top-level `.claude/` file. It is purely documentation and should not be in `.claude/` root.

6. **`BRANDING.md` (383 lines, ~2,900 tokens)** is comprehensive brand guidelines. It is referenced by aesthetics and UI skills. Reasonable content but high token cost for information that changes rarely.

7. **`plugin-registry.json` in `.claude/` is self-marked as deprecated** ("Moved to .planning/workflow/registry.json - this file kept for backward compatibility"). It contains stale data (O-Chorus focused, last activity 2026-02-07).

8. **The canary test system** (`scripts/canary-test.py` + `canary-test.sh`, 267 lines) is a build smoke test framework. The `.sh` version may be dead code similar to the hooks migration pattern.

#### Issues

| # | Issue | Severity | Files |
|---|-------|----------|-------|
| I-1 | Schemas not programmatically enforced | Low | `.claude/schemas/` |
| I-2 | `plugin-registry.json` deprecated but still present | Low | `.claude/plugin-registry.json` |
| I-3 | `preferences-README.md` is 453 lines of docs in root | Low | `.claude/preferences-README.md` |
| I-4 | Aesthetic test-preview HTML files are dev artifacts in repo | Low | `.claude/aesthetics/*/test-previews/` |
| I-5 | `canary-test.sh` may be dead alongside `.py` version | Low | `.claude/scripts/canary-test.sh` |

---

## Context Window Analysis

### Per-Session Baseline Load

Every Claude Code session loads:

| Item | Tokens | Load Method |
|------|--------|-------------|
| `CLAUDE.md` | ~800 | Automatic (Claude Code built-in) |
| `MEMORY.md` | ~1,100 | Automatic (Claude Code built-in) |
| SessionStart.py output | ~200 | Hook stdout printed |
| **Total baseline** | **~2,100** | |

This is efficient. No bloat at session start.

### Per-Command Invocation

When a user runs a slash command, the command `.md` file is loaded:

| Command Category | Avg Lines | Avg Tokens | Notes |
|------------------|-----------|------------|-------|
| Small commands (list, pause, status) | ~60 | ~450 | Minimal |
| Medium commands (research, plan) | ~150 | ~1,100 | Reasonable |
| Large commands (handoff, upgrade, critique) | ~320 | ~2,400 | Heavy |

Plus, if the command references skills, each SKILL.md adds ~130-680 lines (~1K-5K tokens).

### Per-Agent Spawn

When an agent is spawned as a subagent, the full agent `.md` definition is loaded:

| Agent Size | Examples | Tokens |
|------------|----------|--------|
| Small (< 200 lines) | polish-agent, research-lead | ~1K-1.5K |
| Medium (800-1000 lines) | troubleshoot-agent, validation-agent | ~6K-8K |
| Large (1000-1500 lines) | gui-agent, dsp-agent, ui-design-agent | ~8K-12K |

The `inject-agent-memory.py` hook adds the agent's memory file (10-19 lines, negligible tokens) on SubagentStart.

### Per-Write/Edit Operation

Every Write/Edit to plugin source files triggers:

| Validator | Timeout | Estimated Runtime | Token Impact |
|-----------|---------|-------------------|-------------|
| PostToolUse.py (main) | 2s | ~100ms (regex checks) | 0 (no context injection) |
| validate-silent-failures.py (subprocess) | 30s | ~200ms | 0 (no context injection) |

Hooks that produce stderr/stdout consume tokens only for their output. The PostToolUse chain produces ~50-200 chars of output per invocation (warnings/errors only).

### Top 5 Context Hogs (if fully loaded)

| # | Subsystem | Full Token Cost | Typical Per-Session Load |
|---|-----------|-----------------|--------------------------|
| 1 | Research docs | ~480K | ~5-20K (1-2 docs on demand) |
| 2 | Skills | ~344K | ~3-10K (1-3 SKILL.md indexes) |
| 3 | Agents | ~79K | ~8-25K (2-4 agents spawned) |
| 4 | Commands | ~47K | ~1-5K (1-3 commands run) |
| 5 | Hooks | ~48K | ~0.2K (output only, code never in context) |

**Key insight:** The lazy-loading architecture means actual per-session context usage is ~15-60K tokens from PFS infrastructure, well within acceptable limits. The total 632K token footprint is a storage cost, not a runtime cost.

---

## Redundancy Map

| Component A | Component B | Overlap | Recommendation |
|-------------|-------------|---------|----------------|
| `hooks.json` | `settings.json` | Competing hook configs | **Remove** `hooks.json` -- `settings.json` is authoritative |
| 10 `.sh` hooks | 10 `.py` hooks | Identical functionality | **Remove** all `.sh` files |
| `plugin-phases` skill | `plugin-workflow` skill | Overlapping GSD phase descriptions | **Merge** into `plugin-workflow` |
| `plugin-list` + `plugin-status` + `plugin-pause` + `plugin-resume` | (consolidation target) | Minimal status commands | **Consider** merging into `plugin-info` |
| `aesthetics-agent` | `aesthetic-dreaming` skill | Agent unreferenced, skill covers domain | **Remove** dead agent |
| `dynamic-researcher` | `research-lead` agent | Agent unreferenced | **Remove** dead agent |
| `music-theory-agent` | (standalone) | Agent unreferenced anywhere | **Remove** dead agent |
| `plugin-registry.json` (.claude/) | `.planning/workflow/registry.json` | Self-deprecated duplicate | **Remove** deprecated file |
| `canary-test.sh` | `canary-test.py` | Likely duplicate after migration | **Verify** and remove `.sh` if dead |
| `preferences-README.md` | (documentation) | 453-line README in root | **Move** to a docs location or remove |
| `agent-profiles.json` | (documentation) | No runtime effect | **Move** to docs or remove |

---

## Prioritized Recommendations

### High Impact

| # | Recommendation | Impact | Effort | Category |
|---|---------------|--------|--------|----------|
| 1 | **Activate SubagentStop hook in settings.json.** This is 209 lines of sophisticated contract validation (checksums, cross-contract, stage-specific dispatch) that currently never executes. Adding it to settings.json immediately enables automated quality gates after every subagent completion. | High | Low (5 min) | Quality |
| 2 | **Activate research frontmatter validation and resource index regeneration in settings.json.** Two PostToolUse hooks exist but are only in hooks.json. Activating them fixes the 50% resource index coverage gap over time and enforces frontmatter consistency. | High | Low (10 min) | Quality |
| 3 | **Delete 10 dead `.sh` hook files.** 816 lines of dead code from the Python migration. No runtime risk. Clear win. | Medium | Low (5 min) | Cleanup |
| 4 | **Delete `hooks.json`.** It is vestigial, diverged from `settings.json`, and creates confusion about which config is authoritative. Document the canonical hooks in `settings.json` comments or a separate doc. | Medium | Low (5 min) | Cleanup |
| 5 | **Add `__pycache__/` and `*.pyc` to `.gitignore`.** Prevents 156 KB of bytecode from being tracked. Then remove existing `__pycache__` dirs from tracking. | Medium | Low (5 min) | Cleanup |

### Medium Impact

| # | Recommendation | Impact | Effort | Category |
|---|---------------|--------|--------|----------|
| 6 | **Merge `plugin-phases` into `plugin-workflow` skill.** These two skills overlap in domain and description. Consolidating eliminates a 477-line skill and clarifies the skill namespace. | Medium | Medium (30 min) | Efficiency |
| 7 | **Remove 3 dead agent definitions** (aesthetics-agent, dynamic-researcher, music-theory-agent). 473 lines that are never loaded, but their presence creates maintenance burden and potential confusion. | Low | Low (5 min) | Cleanup |
| 8 | **Run resource index regeneration once** to update `resource-index.json` from 27 to 54 entries. This is a one-time fix; recommendation #2 prevents future drift. | Medium | Low (5 min) | Quality |
| 9 | **Standardize research doc frontmatter.** Run a script to add YAML frontmatter to the ~50% of research docs that lack it. The validator exists (`validate-research-frontmatter.py`, 272 lines) but needs data to validate. | Medium | Medium (1 hour) | Quality |
| 10 | **Remove deprecated `plugin-registry.json` from `.claude/`.** It is self-marked deprecated, contains stale data (focused on O-Chorus from Feb 7), and a new registry exists at `.planning/workflow/registry.json`. | Low | Low (5 min) | Cleanup |

### Lower Impact

| # | Recommendation | Impact | Effort | Category |
|---|---------------|--------|--------|----------|
| 11 | **Evaluate module-* command usage.** 7 commands totaling 1,512 lines -- are they actively used? If the module system sees low adoption, these could be simplified. | Low | Medium (1 hour eval) | Efficiency |
| 12 | **Move `preferences-README.md` and `agent-profiles.json` out of `.claude/` root.** These are documentation files with no runtime effect, adding ~3K tokens to any scan of the `.claude/` directory. | Low | Low (10 min) | Cleanup |
| 13 | **Populate or remove empty agent memory files.** 4 of 5 files are empty placeholders. Either establish a pattern for recording learnings or remove the empty files to avoid the false impression that memory injection is working broadly. | Low | Low (10 min) | Quality |
| 14 | **Remove or relocate aesthetic test-preview HTML files.** 5 HTML preview files are development artifacts that shouldn't be in the production `.claude/` tree. | Low | Low (10 min) | Cleanup |
| 15 | **Evaluate `validation-cache` system.** The cache file is empty, the documentation (231 lines) and implementation (172 lines) exist but appear unused. Either activate caching or remove the dead infrastructure. | Low | Medium (30 min) | Cleanup |

---

## Quick Wins (< 30 min each)

1. **Delete `.sh` hook files** (5 min): `rm .claude/hooks/*.sh` -- 816 lines removed, zero risk.
2. **Delete `hooks.json`** (5 min): Remove the vestigial config that creates confusion.
3. **Add `__pycache__` to `.gitignore`** (5 min): Add `**/__pycache__/` and `*.pyc` patterns, then `git rm -r --cached .claude/hooks/__pycache__ .claude/hooks/validators/__pycache__ .claude/scripts/__pycache__`.
4. **Activate SubagentStop in `settings.json`** (5 min): Add SubagentStop event with SubagentStop.py to settings.json.
5. **Activate research hooks in `settings.json`** (10 min): Add validate-research-frontmatter.py and regenerate-manifest.py as PostToolUse hooks.
6. **Delete 3 dead agents** (5 min): Remove aesthetics-agent.md, dynamic-researcher.md, music-theory-agent.md.
7. **Remove deprecated `plugin-registry.json`** (5 min): Delete `.claude/plugin-registry.json`.
8. **Regenerate resource-index.json** (5 min): Run `python3 .claude/scripts/generate-resource-index.py` to update from 27 to 54 entries.
9. **Clean up `frontmatter-issues.txt`** (5 min): This single-line file tracking one known issue is better handled by the frontmatter validator output.

All 9 quick wins combined: ~50 minutes, removes ~1,500 lines of dead code, activates dormant quality checks, and fixes the resource index gap.

---

## Architecture Observations

### What's Working Well

1. **Lazy-loading skill architecture.** The SKILL.md + rules/ pattern keeps per-session context costs manageable despite 344K total tokens in the skill tree. This is the right design.

2. **Hook-based validation pipeline.** Real-time safety checks (PostToolUse), contract immutability enforcement, and automated validation dispatch are sophisticated and valuable. The architecture is sound even though some hooks aren't active.

3. **Agent specialization.** The separation of foundation-shell, dsp, gui, and validation agents into distinct definitions with deep domain knowledge produces higher quality output than generic prompting would. The 79K token investment in agent definitions pays off in implementation quality.

4. **Pre-compaction snapshot.** The PreCompact hook writing a domain-aware snapshot before context compaction is a clever solution to Claude Code's context window limitations. The PostCompact-SessionStart hook restoring it completes the cycle cleanly.

5. **Research corpus.** 54 documents totaling 480K tokens of domain-specific knowledge (DSP algorithms, JUCE patterns, spatial audio, synthesis techniques) is a genuine competitive advantage. This knowledge base is what enables one-shot implementation of complex audio plugins.

### What's Fighting Against Claude Code's Design

1. **hooks.json as a shadow config.** Claude Code reads `settings.json`. Maintaining a separate `hooks.json` creates a split-brain configuration problem. The divergence between them has caused 3 hooks to silently stop working.

2. **Schemas without enforcement.** JSON schemas exist (17K tokens) but no code validates against them. Claude Code doesn't have a built-in schema validation hook. This infrastructure adds tokens without delivering automated value.

3. **agent-profiles.json as aspirational documentation.** Claude Code doesn't support per-agent effort levels in any runtime capacity. This file documents intent that cannot be implemented.

### What Should Change Structurally

1. **Single source of truth for hooks.** Delete `hooks.json`. Audit `settings.json` to ensure all valuable hooks from `hooks.json` are migrated. Add comments to `settings.json` explaining each hook's purpose.

2. **Research doc governance.** The research corpus is valuable but ungoverned -- mixed frontmatter, incomplete indexing, no freshness tracking. Activating the dormant hooks (frontmatter validation, index regeneration) and running a one-time frontmatter normalization pass would bring this under control.

3. **Consider skill consolidation roadmap.** The 10 `plugin-*` skills could be reorganized into 3-4 clearer domains: `plugin-create` (ideation + planning), `plugin-build` (workflow + phases + lifecycle), `plugin-ship` (testing + packaging + publishing), `plugin-maintain` (improve + context). This is a v2 consideration, not a quick fix.

4. **Agent memory needs a recording mechanism.** The inject-agent-memory.py hook works, but agents don't have a counterpart hook to WRITE learnings back to memory files. Only manual edits populate agent memory, which is why 4 of 5 files are empty. Consider adding a post-agent hook that extracts key learnings.
