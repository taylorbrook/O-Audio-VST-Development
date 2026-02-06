# Phase 11: Context Injection Pipeline - Research

**Researched:** 2026-02-05
**Domain:** Skill orchestrator prompt construction, Python content extraction, token budget management
**Confidence:** HIGH

## Summary

Phase 11 connects Phase 10's discovery engine output to agent input. The discovery infrastructure (manifest, scoring, tiering) is complete and working. This phase creates a shared Python utility that any skill orchestrator calls to get a formatted context block, then modifies the skill files that spawn stage agents to include that block in their Task() prompts. The core engineering problems are: (1) building a content extraction function that reads research documents and excerpts relevant portions within a 4,000-token budget, (2) handling stage-specific pattern files as always-on auto-injected resources, and (3) updating 3+ skill orchestrator files with consistent injection patterns.

The existing codebase has clear injection points. The `plugin-workflow` SKILL.md already uses `invoke_task(subagent_type=agent, prompt=...)` pseudocode patterns where resource content would be appended. The stage-specific reference files (`stage-2-dsp.md`, `stage-3-gui.md`) show exact prompt templates with `Required Reading: troubleshooting/patterns/stage-N-patterns.md` lines that currently tell agents to read files themselves. After Phase 11, agents will receive the content inline instead of needing to fetch it.

**Primary recommendation:** Create a single Python script `inject-context.py` in `.claude/scripts/` that imports `discover-resources.py`'s `discover()` function, reads top-ranked document files, extracts frontmatter summary + key sections within the token budget, and outputs a formatted text block. Skills call this script via subprocess and embed the output in their Task() prompts.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Skills only** -- orchestrator-level injection, not hooks. Aligns with Phase 10 decision to avoid hook timeout constraints.
- **All stage-spawning skills** get injection -- any skill that calls Task() with a stage agent (dsp-agent, gui-agent, foundation-shell-agent, research-planning-agent, etc.), not just the big three.
- **Shared utility function** -- one callable function/script that any skill invokes with (plugin_name, stage, agent_type) and gets back a formatted context block. DRY, consistent across all skills.
- **Excerpted content** -- the utility reads top-ranked resource files and extracts key sections (frontmatter summary + relevant portions). Agents receive actual content inline, not just paths.
- Fits within the 4,000 token budget cap per agent invocation.
- Agents don't need to Read files themselves -- knowledge is delivered ready to consume.
- **Primary tier (score >= 0.75) = MUST-READ** -- flagged for Phase 12 accountability.
- **Stage-specific pattern files auto-inject** -- files like stage-2-patterns.md are always injected for matching agents, bypassing score threshold.
- **Script failure = continue without injection** -- log warning, skip injection, agent proceeds normally.
- **Missing files on disk = skip with warning** -- if a manifest reference points to a deleted/moved file, skip it and log warning.

### Claude's Discretion
- Prompt delimiter style (XML tags vs markdown sections) for injected content
- Whether to show relevance scores to agents or just order by relevance
- Resource cap per agent (3 vs 5 vs budget-limited)
- Token budget split between primary and secondary tiers
- Logging implementation (console, file, or both)
- Zero-result behavior (silent skip vs brief note)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Python | 3.14.2 | Runtime for injection script | Project standard, already installed |
| PyYAML | 6.0.3 | Parse frontmatter from research docs for summary extraction | Already installed, used by Phase 10 scripts |
| discover-resources.py | Phase 10 | Discovery engine -- returns ranked, tiered results | Already built and verified, import its `discover()` function |
| json (stdlib) | builtin | Serialize/deserialize discovery results | Zero dependency |
| pathlib (stdlib) | builtin | File path operations | Matches existing script conventions |
| re (stdlib) | builtin | Frontmatter extraction, section parsing | Matches Phase 10 patterns |
| sys (stdlib) | builtin | stderr logging, exit codes | Standard error handling |
| argparse (stdlib) | builtin | CLI interface for testing | Matches discover-resources.py pattern |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| textwrap (stdlib) | builtin | Clean excerpt formatting | If content needs truncation with ellipsis |
| importlib.util (stdlib) | builtin | Import discover-resources.py as module | If library import preferred over subprocess |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Python script via subprocess | Shell script calling discover-resources.py | Python is consistent with Phase 10 tooling; shell would add another language |
| Direct library import | subprocess call to discover-resources.py CLI | Import is cleaner, avoids JSON serialization overhead; both work |
| tiktoken for token counting | `len(text) / 4` character heuristic | tiktoken is unnecessary dependency for budget-enforcement; 4 chars/token heuristic is sufficient for a hard cap with safety margin |
| Full document content | Summary + key sections | Full docs are 3,000-8,000+ words each; 2-3 full docs would blow the 4,000-token budget immediately |

**Installation:**
```bash
# No new dependencies needed -- all libraries already installed
```

## Architecture Patterns

### Recommended Project Structure
```
.claude/
├── scripts/
│   ├── discover-resources.py      # Phase 10 (existing) - discovery engine
│   ├── generate-resource-index.py # Phase 10 (existing) - manifest generator
│   └── inject-context.py          # Phase 11 (new) - context injection utility
├── resource-index.json            # Phase 10 (existing) - manifest
└── skills/
    ├── plugin-workflow/SKILL.md   # Modified: adds injection to execute phase
    ├── plugin-planning/SKILL.md   # Modified: adds injection to Stage 0 agent
    ├── improve-milestone/SKILL.md # Modified: adds injection to execute phase
    └── plugin-improve/SKILL.md    # Modified: adds injection if spawning agents
```

### Pattern 1: Injection Utility Interface

**What:** A Python script that combines discovery + content extraction into a single formatted output.
**When to use:** Called by any skill orchestrator before constructing a Task() prompt.

```python
# Source: Designed from Phase 10 discover() API + CONTEXT.md decisions
# inject-context.py

def inject_context(
    plugin_name: str,        # Plugin being worked on (for future use)
    stage: int,              # Pipeline stage (0-4)
    agent_type: str,         # Agent name or role (e.g., "dsp-agent", "dsp")
    keywords: list[str] = None,  # Optional task keywords
    token_budget: int = 4000,    # Hard cap
) -> str:
    """
    Returns a formatted text block ready to embed in a Task() prompt.

    1. Calls discover() to get ranked resources
    2. Auto-injects stage-specific pattern file if it exists
    3. Reads top resources, extracts summary + key sections
    4. Enforces token budget
    5. Returns formatted string with XML delimiters

    On any error: returns empty string (agent continues without injection).
    """
```

**CLI interface:**
```bash
python3 .claude/scripts/inject-context.py --stage 2 --agent dsp-agent --plugin O-EQ
# Outputs formatted context block to stdout
```

**Return format (recommendation: XML tags):**
```xml
<research_context>
## Relevant Research Resources

### MUST-READ (Primary Tier)
**FFT Processing Best Practices in JUCE** (relevance: 0.85)
Comprehensive guide to implementing high-quality FFT-based audio processing...
Key sections: STFT Architecture, Buffer Management, Window Functions
[Full summary + excerpted key sections here]

### Supplementary
**DSP Click Prevention and Debugging Guide** (relevance: 0.40)
Reference for understanding, preventing, and debugging audio clicks...
[Summary only]

### Stage Patterns (auto-injected)
[Full content of troubleshooting/patterns/stage-2-patterns.md]
</research_context>
```

### Pattern 2: Content Extraction Strategy

**What:** How to extract meaningful content from research documents within the token budget.
**When to use:** Inside inject-context.py for each discovered resource.

**Strategy for primary tier resources (score >= 0.75):**
1. Include frontmatter `summary` field (30-50 words per doc)
2. Extract the first content section after frontmatter (Executive Summary / Overview)
3. Extract Table of Contents if present (gives agent a map of what's available)
4. Include the document's file path so agent can Read more if needed

**Strategy for supplementary tier resources:**
1. Include frontmatter `summary` field only (one-line mention)
2. Include file path for optional deeper reading

**Strategy for stage-specific pattern files:**
1. Include full content -- these files are curated to be relevant and are 6-12 KB each
2. Stage pattern files bypass scoring entirely -- always injected when stage matches

### Pattern 3: Token Budget Allocation

**What:** How to divide the 4,000-token budget across resource types.
**When to use:** Budget enforcement logic in inject-context.py.

**Recommended allocation:**
| Resource Type | Budget Allocation | Rationale |
|---------------|-------------------|-----------|
| Stage pattern file | Up to 2,000 tokens | These are curated, always-relevant content (~1,500-2,800 words = ~2,000-3,700 tokens). They get first priority |
| Primary tier resources | Remaining budget - 200 | Summaries + key sections for highest-scoring discovered resources |
| Supplementary mentions | 200 tokens (reserve) | Brief one-line mentions with paths only |

**Token estimation function (no external deps):**
```python
def estimate_tokens(text: str) -> int:
    """Estimate token count using character-based heuristic.

    ~4 characters per token is the standard approximation.
    We use 3.5 for safety margin (slightly overestimates).
    """
    return int(len(text) / 3.5)
```

**Safety margin:** Using 3.5 chars/token instead of 4 chars/token means we slightly overestimate token count, ensuring we never exceed the 4,000-token budget in practice.

### Pattern 4: Stage Pattern File Auto-Injection

**What:** Stage-specific troubleshooting pattern files are always injected for matching agents.
**When to use:** Whenever an agent is invoked for stages 1-3.

**Existing files:**
| Stage | File | Size | Est. Tokens |
|-------|------|------|-------------|
| 1 | `troubleshooting/patterns/stage-1-patterns.md` | 7,017 bytes | ~2,005 |
| 2 | `troubleshooting/patterns/stage-2-patterns.md` | 5,928 bytes | ~1,694 |
| 3 | `troubleshooting/patterns/stage-3-patterns.md` | 11,609 bytes | ~3,317 |

**Important:** Stage 3 pattern file at ~3,317 tokens consumes most of the 4,000-token budget alone. The injection logic must handle this:
- If stage pattern file > 2,500 tokens, truncate to key sections or cap at 2,500 tokens
- This leaves 1,500 tokens for discovered research resources
- If stage pattern file <= 2,000 tokens, full content injected with more room for research

**Auto-injection mapping:**
```python
STAGE_PATTERN_MAP = {
    1: "troubleshooting/patterns/stage-1-patterns.md",
    2: "troubleshooting/patterns/stage-2-patterns.md",
    3: "troubleshooting/patterns/stage-3-patterns.md",
    # Stage 0 and 4: no dedicated pattern file (by design)
}
```

### Pattern 5: Skill Modification Pattern

**What:** How to modify existing skill SKILL.md files to include injection.
**When to use:** When updating plugin-workflow, plugin-planning, improve-milestone.

**Before (current):**
```python
result = invoke_task(
    subagent_type=agent,
    prompt=f"""
    Implement {stage} for {plugin_name}.

    PLAN.md tasks:
    {plan}

    Required Reading: Load stage-{stage[0]}-patterns.md
    """
)
```

**After (with injection):**
```python
# Get research context block
research_context = get_research_context(plugin_name, stage_number, agent_type)

result = invoke_task(
    subagent_type=agent,
    prompt=f"""
    Implement {stage} for {plugin_name}.

    PLAN.md tasks:
    {plan}

    {research_context}
    """
)
```

**Key change:** The `Required Reading: Load stage-{stage[0]}-patterns.md` instruction is replaced by inline content. The agent no longer needs to Read the file itself -- it's already in the prompt.

### Pattern 6: Graceful Failure Pattern

**What:** How injection failures are handled.
**When to use:** In both the inject-context.py script and the skill orchestrators.

```python
# In inject-context.py
def inject_context(...) -> str:
    try:
        # ... discovery, extraction, formatting ...
        return formatted_block
    except Exception as e:
        print(f"Warning: Context injection failed: {e}", file=sys.stderr)
        return ""  # Empty string = no injection

# In skill orchestrator (pseudocode)
research_context = run_inject_context(plugin_name, stage, agent)
# research_context is either formatted block or empty string
# Either way, just embed it in the prompt -- empty string is harmless
```

### Anti-Patterns to Avoid

- **Injecting full document content:** Research docs are 3,000-8,000+ words each. Even one full document could exceed the 4,000-token budget. Always excerpt.
- **Using hooks for injection:** The CONTEXT.md explicitly locked this decision -- skills only, not hooks. Hook timeouts (2-10s) are too tight for file reading + content extraction.
- **Relying on agents to Read injected paths:** The whole point of Phase 11 is that agents receive content inline. Don't inject paths and expect agents to fetch them -- that's the pre-Phase 11 pattern.
- **Hardcoding resource lists:** Use the discovery engine's dynamic scoring. Don't manually list which resources go to which agent -- that's what Phase 10 built.
- **Skipping token budget enforcement:** Without budget enforcement, a few large primary-tier resources could exhaust the agent's context window. Always count tokens before including content.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Resource discovery | Custom file scanner | `discover-resources.py` `discover()` function | Phase 10 already built weighted scoring with tiering |
| Agent-to-role mapping | New mapping table | `discover-resources.py` `resolve_role()` function | Phase 10 maintains the canonical 11-agent mapping |
| Frontmatter parsing | Custom YAML parser | Phase 10's `parse_frontmatter()` pattern (PyYAML + regex) | Proven pattern, handles all edge cases |
| Token counting | tiktoken dependency | `len(text) / 3.5` character heuristic | No new dependency; heuristic is sufficient for hard budget caps with safety margin |
| Manifest loading | File reading + validation | `discover()` handles this internally | Manifest validation already built into discovery |

**Key insight:** Phase 10 built the discovery engine as a library-importable module. Phase 11 should import and extend it, not duplicate it. The `discover()` function already returns paths, titles, summaries, scores, and tiers -- Phase 11 adds content extraction on top.

## Common Pitfalls

### Pitfall 1: Stage 3 Pattern File Blows Token Budget
**What goes wrong:** `stage-3-patterns.md` is 11,609 bytes (~3,317 tokens), consuming 83% of the 4,000-token budget. No room left for discovered resources.
**Why it happens:** Stage 3 (GUI) has the most complex patterns (WebView, relay ordering, CSS, etc.).
**How to avoid:** Implement a budget allocation strategy: if stage pattern file exceeds 2,500 tokens, truncate to the most critical sections. Alternatively, set a maximum of 2,500 tokens for the pattern file and distribute the remaining 1,500 to discovered resources.
**Warning signs:** Agents for Stage 3 receive zero discovered research resources because the pattern file consumed the entire budget.

### Pitfall 2: Requirement Mismatch Between ROADMAP and CONTEXT.md
**What goes wrong:** The ROADMAP success criteria mention "SubagentStart hook" for INJT-01 and INJT-03, but the CONTEXT.md (which supersedes) explicitly says "Skills only, not hooks."
**Why it happens:** The ROADMAP was written before the discuss phase refined the approach.
**How to avoid:** Follow CONTEXT.md decisions (skills-only). The ROADMAP requirements will need their wording updated to reflect skills-level injection instead of hook-level. The intent (agents receive resources) is the same; the mechanism changes.
**Warning signs:** Planner creates tasks to implement SubagentStart hook injection, which contradicts CONTEXT.md.

### Pitfall 3: Import Path Issues Between Scripts
**What goes wrong:** `inject-context.py` cannot import from `discover-resources.py` because of the hyphenated filename.
**Why it happens:** Python modules cannot be imported with `import discover-resources` -- hyphens are not valid Python identifiers.
**How to avoid:** Use `importlib.util` to load the module from its file path, matching the pattern already established in the codebase. This was noted in Phase 10's research as well.
**Warning signs:** `ImportError` or `ModuleNotFoundError` when inject-context.py tries to use discover().

```python
# Correct: use importlib.util for hyphenated filenames
import importlib.util
spec = importlib.util.spec_from_file_location("discover_resources", SCRIPT_DIR / "discover-resources.py")
discover_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(discover_module)
discover = discover_module.discover
resolve_role = discover_module.resolve_role
```

### Pitfall 4: Non-Deterministic Injection Content
**What goes wrong:** The same agent invocation produces different injected content on different runs, making debugging difficult.
**Why it happens:** If token budget causes different truncation points based on file read timing or order.
**How to avoid:** Discovery results are already deterministically sorted (by relevance descending, then path). Content extraction should follow the same deterministic order. Use the same sort order from `discover()` without randomization.
**Warning signs:** Same stage/agent combination produces different context blocks on consecutive runs.

### Pitfall 5: Skills That Spawn Multiple Agent Types
**What goes wrong:** A skill like `plugin-workflow` spawns different agents per stage (foundation-shell-agent for Stage 1, dsp-agent for Stage 2, etc.) but injection is only wired for one.
**Why it happens:** The injection point is per-stage in the dispatch logic, not per-skill. Each dispatch point needs injection for the correct agent type.
**How to avoid:** Audit ALL Task() invocation points across ALL skills. Each invocation point needs the injection call with the correct (stage, agent_type) parameters.
**Warning signs:** Some agents receive research context but others (same skill, different stage) do not.

### Pitfall 6: Stale Pattern File References After File Moves
**What goes wrong:** Stage pattern files move or are renamed, but the STAGE_PATTERN_MAP still points to old paths.
**Why it happens:** Pattern files are at `troubleshooting/patterns/stage-N-patterns.md` and could be relocated.
**How to avoid:** The injection script should verify pattern file existence before reading. On missing file, log a warning and skip pattern injection (per CONTEXT.md: "missing files on disk = skip with warning").
**Warning signs:** Stage pattern content missing from injected context; warning in stderr.

## Code Examples

### Content Extraction from Research Document
```python
# Source: Derived from Phase 10's parse_frontmatter pattern
import re
import yaml
from pathlib import Path

def extract_content(filepath: str, max_tokens: int = 800) -> str:
    """Extract summary + key sections from a research document.

    Returns formatted content string within token budget.
    """
    path = Path(filepath)
    if not path.exists():
        return ""

    content = path.read_text(encoding="utf-8")

    # Parse frontmatter for summary
    fm_match = re.match(r'^---\n(.*?)\n---\n?(.*)', content, re.DOTALL)
    if not fm_match:
        return ""

    frontmatter = yaml.safe_load(fm_match.group(1))
    body = fm_match.group(2).strip()

    # Start with summary
    result = f"**{frontmatter.get('title', 'Untitled')}**\n"
    result += f"{frontmatter.get('summary', '')}\n"

    current_tokens = estimate_tokens(result)

    # Add first major section (usually Executive Summary)
    sections = re.split(r'\n## ', body)
    if len(sections) > 1:
        first_section = "## " + sections[1]
        section_tokens = estimate_tokens(first_section)
        if current_tokens + section_tokens <= max_tokens:
            result += f"\n{first_section}\n"
            current_tokens += section_tokens

    # Add path for optional deeper reading
    result += f"\n_Full document: {filepath}_\n"

    return result


def estimate_tokens(text: str) -> int:
    """Estimate token count. ~3.5 chars per token (conservative)."""
    return int(len(text) / 3.5)
```

### Formatted Output Block
```python
# Source: CONTEXT.md decision on XML tags + tier structure
def format_context_block(
    primary_resources: list[dict],
    supplementary_resources: list[dict],
    stage_pattern_content: str | None,
    extracted_content: dict[str, str],  # path -> extracted text
) -> str:
    """Format the complete injection block."""
    parts = ["<research_context>"]
    parts.append("## Relevant Research Resources\n")
    parts.append("The following research has been automatically discovered as relevant")
    parts.append("to your current task. Primary resources are MUST-READ.\n")

    if primary_resources:
        parts.append("### MUST-READ (Primary Tier)")
        for r in primary_resources:
            content = extracted_content.get(r["path"], r["summary"])
            parts.append(f"\n{content}")

    if supplementary_resources:
        parts.append("\n### Supplementary")
        for r in supplementary_resources:
            parts.append(f"- **{r['title']}**: {r['summary']}")
            parts.append(f"  _Path: {r['path']}_")

    if stage_pattern_content:
        parts.append("\n### Stage Patterns (auto-injected)")
        parts.append(stage_pattern_content)

    parts.append("</research_context>")
    return "\n".join(parts)
```

### Skill Orchestrator Integration (plugin-workflow example)
```python
# Source: Current plugin-workflow/SKILL.md execute phase pattern, modified for injection

def run_execute_phase(plugin_name, stage):
    stage_agents = {
        "1-foundation": "foundation-shell-agent",
        "2-dsp": "dsp-agent",
        "3-gui": "gui-agent",
        "4-polish": "polish-agent"
    }
    agent = stage_agents[stage]
    stage_number = int(stage[0])

    # NEW: Get research context via injection utility
    # Runs: python3 .claude/scripts/inject-context.py --stage N --agent AGENT --plugin NAME
    # Returns formatted block or empty string on failure
    research_context = get_research_context(plugin_name, stage_number, agent)

    plan = read_file(f"plugins/{plugin_name}/.planning/stages/{stage}/PLAN.md")
    contracts = load_contracts(plugin_name)

    result = invoke_task(
        subagent_type=agent,
        prompt=f"""
        Implement {stage} for {plugin_name}.

        PLAN.md tasks:
        {plan}

        Contracts:
        - BRIEF.md: {contracts.brief_summary}
        - ARCHITECTURE.md: {contracts.arch_summary}

        {research_context}
        """
        # NOTE: "Required Reading: Load stage-N-patterns.md" line REMOVED
        # Stage patterns are now embedded in research_context block
    )
```

### Stage Pattern Auto-Injection
```python
# Source: Codebase audit of troubleshooting/patterns/ directory

STAGE_PATTERN_MAP = {
    1: "troubleshooting/patterns/stage-1-patterns.md",
    2: "troubleshooting/patterns/stage-2-patterns.md",
    3: "troubleshooting/patterns/stage-3-patterns.md",
}

def get_stage_pattern_content(stage: int, max_tokens: int = 2500) -> str | None:
    """Read stage-specific pattern file. Returns None if not found."""
    pattern_path = STAGE_PATTERN_MAP.get(stage)
    if not pattern_path:
        return None

    full_path = PROJECT_ROOT / pattern_path
    if not full_path.exists():
        print(f"Warning: Stage pattern file missing: {pattern_path}", file=sys.stderr)
        return None

    content = full_path.read_text(encoding="utf-8")
    tokens = estimate_tokens(content)

    if tokens <= max_tokens:
        return content

    # Truncate: keep sections up to budget
    lines = content.split('\n')
    truncated = []
    running_tokens = 0
    for line in lines:
        line_tokens = estimate_tokens(line + '\n')
        if running_tokens + line_tokens > max_tokens:
            truncated.append("\n_[Truncated for token budget. Full file: " + pattern_path + "]_")
            break
        truncated.append(line)
        running_tokens += line_tokens

    return '\n'.join(truncated)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Agents told "Read stage-N-patterns.md" | Pattern content injected inline into prompt | Phase 11 (this phase) | Agents don't waste tool calls reading files; content guaranteed delivered |
| No research resource discovery for agents | Discovery engine returns ranked resources | Phase 10 (complete) | Agents could theoretically access resources but had to be told which ones |
| Manual resource lists in agent prompts | Auto-injected content based on discovery scoring | Phase 11 (this phase) | Zero manual prompt maintenance; resources automatically flow to relevant agents |

**Not applicable:**
- Vector databases (overkill for 26-doc corpus)
- RAG pipelines (agents receive content directly, no retrieval step during execution)
- LLM-based summarization (frontmatter summaries already exist and are human-curated)

## Decisions and Recommendations (Claude's Discretion Items)

### Prompt Delimiter Style: XML Tags
**Recommendation:** Use `<research_context>` XML tags.
**Rationale:** XML tags are already used extensively in this project's agent files (e.g., `<gate_preconditions>`, `<critical_sequence>`, `<delegation_rule>` in plugin-improve/SKILL.md). XML tags clearly delimit injected content from the rest of the prompt and are trivially parseable. Markdown sections would blend into surrounding prompt markdown, making it harder for agents and Phase 12 accountability to distinguish injected content.

### Show Relevance Scores: Yes, Ordered by Relevance
**Recommendation:** Show scores and order by relevance.
**Rationale:** Scores help Phase 12 accountability tracking and give agents a signal about which resources are most relevant. Agents can prioritize reading order based on score.

### Resource Cap Per Agent: Budget-Limited (Not Fixed Count)
**Recommendation:** Use token budget as the cap, not a fixed count.
**Rationale:** A fixed cap of 3 or 5 is arbitrary. Budget-limited ensures maximum useful content within the 4,000-token constraint. In practice, with a 2,500-token pattern file and content extraction, this will typically result in 1-3 primary resources with excerpts and 2-5 supplementary mentions.

### Token Budget Split
**Recommendation:**
- Stage pattern file: up to 2,500 tokens (first priority)
- Primary tier content: up to remaining budget minus 200
- Supplementary mentions: 200 tokens reserved
- If no stage pattern file applies (stages 0, 4): full 4,000 tokens available for research resources

### Logging Implementation: stderr Only
**Recommendation:** Print warnings to stderr.
**Rationale:** The script outputs the formatted context block to stdout. Warnings and errors go to stderr. This follows Unix conventions and matches the existing scripts (discover-resources.py prints errors to stderr). No file-based logging needed -- this is a stateless utility.

### Zero-Result Behavior: Brief Note
**Recommendation:** When discovery returns zero results, include a brief note rather than silently omitting the block.
**Rationale:** A brief note like "No relevant research resources discovered for this context." confirms the injection system ran but found nothing. This aids debugging and is cheap (one line). Silent omission makes it impossible to distinguish "injection didn't run" from "injection found nothing."

## Codebase Audit: Injection Points

### Skills That Spawn Stage Agents (MUST be modified)

| Skill | File | Agent Types Spawned | Task() Location |
|-------|------|---------------------|-----------------|
| plugin-workflow | `.claude/skills/plugin-workflow/SKILL.md` | foundation-shell-agent, dsp-agent, gui-agent, polish-agent | `run_execute_phase()` pseudocode |
| plugin-workflow | `.claude/skills/plugin-workflow/references/stage-1-foundation-shell.md` | foundation-shell-agent | Direct Task() call |
| plugin-workflow | `.claude/skills/plugin-workflow/references/stage-2-dsp.md` | dsp-agent | Direct Task() calls (single + phased) |
| plugin-workflow | `.claude/skills/plugin-workflow/references/stage-3-gui.md` | gui-agent | Direct Task() calls (single + phased) |
| plugin-planning | `.claude/skills/plugin-planning/SKILL.md` | research-planning-agent | Stage 0 dispatch |
| improve-milestone | `.claude/skills/improve-milestone/SKILL.md` | dsp-agent, gui-agent, polish-agent | Phase 4 execute |
| improve-milestone | `.claude/skills/improve-milestone/references/phase-agents.md` | dsp-agent, gui-agent, polish-agent | Domain-specific invocation |

### Skills That Spawn Non-Stage Agents (injection NOT needed)

| Skill | Agent Types | Why Skip |
|-------|-------------|----------|
| plugin-workflow | gsd-phase-researcher, gsd-planner, gsd-verifier | These are workflow infrastructure agents, not stage implementers |
| plugin-workflow | validation-agent | Validates output, doesn't implement |
| plugin-workflow | plugin-discuss-agent | Gathers user input, doesn't implement |
| plugin-improve | deep-research | Investigates issues, doesn't implement |

### Stage-to-Agent-to-Pattern Mapping

| Stage | Agent | Stage Pattern File | Research Domain |
|-------|-------|--------------------|-----------------|
| 0 | research-planning-agent | None | research |
| 1 | foundation-shell-agent | stage-1-patterns.md | build |
| 2 | dsp-agent | stage-2-patterns.md | dsp |
| 3 | gui-agent | stage-3-patterns.md | ui |
| 4 | polish-agent | None | dsp |

## Open Questions

1. **Should the injection script be invoked via subprocess or direct import in skill pseudocode?**
   - What we know: Skills are markdown files with pseudocode. The orchestrator (Claude) interprets the pseudocode and runs actual commands.
   - What's unclear: Whether the skill should document running `python3 .claude/scripts/inject-context.py --stage 2 --agent dsp-agent` via Bash tool, or describe calling the function conceptually.
   - Recommendation: Document it as a Bash subprocess call in the skill pseudocode, matching how `pre-stage-scan.py` is called. This is consistent with existing patterns. The CLI output (stdout) becomes the context block to embed.

2. **How should injection work for improve-milestone's execute phase where stage number is not always clear?**
   - What we know: improve-milestone detects domain (dsp/gui/polish) from content analysis, not from a stage number. It may invoke dsp-agent outside the normal stage 2 context.
   - What's unclear: What stage number to pass to the discovery engine for improvement scenarios.
   - Recommendation: Default to the agent's primary stage (dsp-agent -> stage 2, gui-agent -> stage 3) or allow "any stage" mode where only agent role matching is used. The discovery engine already handles role-only queries (score of 0.35 for role match exceeds the 0.3 threshold).

3. **Should the "Required Reading" instructions be fully removed from skill files or kept as fallback?**
   - What we know: Currently, agents are told "Required Reading: Load stage-N-patterns.md" and read the file themselves.
   - What's unclear: Should we remove this instruction (since content is now inline) or keep it as a fallback in case injection fails?
   - Recommendation: Remove the instruction and replace with the injected content block. If injection fails (empty string), the agent proceeds without patterns -- which aligns with the "continue without injection" fallback decision. Keeping both creates confusion (agent reads the same content twice).

## Sources

### Primary (HIGH confidence)
- Direct codebase audit of `.claude/scripts/discover-resources.py` (259 lines) -- verified discover() API, resolve_role(), score_document(), tiering logic
- Direct codebase audit of `.claude/resource-index.json` (26 documents) -- verified manifest structure and content
- Direct codebase audit of `troubleshooting/patterns/stage-{1,2,3}-patterns.md` -- verified file sizes (7KB, 6KB, 12KB)
- Direct codebase audit of all skill SKILL.md files -- verified Task() invocation patterns and injection points
- Direct codebase audit of `.claude/agents/` (11 agent files) -- verified agent types
- Phase 10 RESEARCH.md and 10-04-SUMMARY.md -- verified discovery engine design and decisions
- `wc -w` on all 26 research documents -- verified word counts (3,000-8,000+ words each)

### Secondary (MEDIUM confidence)
- Token estimation heuristic (4 chars/token for English text) -- widely used industry standard, validated by multiple sources

### Tertiary (LOW confidence)
- None -- all findings verified against codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries already installed, no new dependencies. Existing Phase 10 scripts serve as the foundation.
- Architecture: HIGH - Injection points identified from direct skill file audit. Pattern is straightforward: call script, embed output.
- Pitfalls: HIGH - Stage 3 budget issue verified by exact byte counts. Import issue verified by examining hyphenated filenames. Requirement mismatch verified by reading both ROADMAP and CONTEXT.md.

**Research date:** 2026-02-05
**Valid until:** 2026-03-07 (30 days -- stable domain, no fast-moving dependencies)
