# Architecture Research: Resource Discovery & Context Injection

**Domain:** Agent intelligence - resource discovery and context injection for multi-agent JUCE plugin development system
**Researched:** 2026-02-04
**Confidence:** HIGH (based on thorough analysis of existing codebase, not external sources)

---

## Executive Summary

The existing system has 23 research documents, 11 agent definitions, 25 skills, and rich per-plugin planning artifacts -- but no mechanism connects research knowledge to agent execution. Research docs are only referenced when a human or agent happens to know they exist. The dsp-agent implementing a bell plugin never sees `modal-synthesis-bells-academic-research.md` unless someone manually adds it to the prompt.

The architecture must solve three problems:
1. **Discovery:** Given a task context, which resources are relevant?
2. **Injection:** How do relevant resources reach the agent before execution?
3. **Accountability:** Did the agent actually use the resources, and can we verify it?

The recommended architecture follows the existing system's patterns: a static index file (like plugin-registry.json), a discovery script (like template-lookup.py), prompt augmentation in the skill orchestrator (like how contracts are already injected), and validation in SubagentStop (like existing contract validation). No new architectural primitives needed -- just new components following established patterns.

**Critical design constraint:** Claude Code agents communicate through markdown files and the Task tool prompt string. There is no runtime API, no shared memory, no message bus. Context injection must happen through prompt construction or file reading instructions.

---

## System Overview

```
                        SKILL ORCHESTRATOR
                     (plugin-workflow, plugin-improve, etc.)
                              |
                   1. Load task context
                   (plugin name, stage, agent type)
                              |
                   2. Invoke DISCOVERY SCRIPT
                   (python3 .claude/scripts/resource-discovery.py)
                              |
                              v
              +-------------------------------+
              |    RESOURCE INDEX             |
              |    (.claude/resource-index.json)|
              |                               |
              |  { "research/modal-...md":    |
              |    { "tags": ["bells",        |
              |      "modal-synthesis",       |
              |      "physical-modeling"],     |
              |    "agents": ["dsp-agent",    |
              |      "research-planning-agent"],|
              |    "plugins": ["O-Bells"],    |
              |    "dsp_topics": ["synthesis", |
              |      "envelopes", "damping"]  |
              |    }                          |
              |  }                            |
              +-------------------------------+
                              |
                   3. Score & rank matches
                              |
                              v
              +-------------------------------+
              |    CONTEXT INJECTION          |
              |    (in skill orchestrator)     |
              |                               |
              |  Append to agent prompt:       |
              |  "Relevant resources to READ:  |
              |   - research/modal-synthesis..."|
              |   - research/fft-processing..." |
              |  Priority: MUST-READ / SHOULD  |
              +-------------------------------+
                              |
                   4. Agent executes with
                      resource awareness
                              |
                              v
              +-------------------------------+
              |    AGENT EXECUTION            |
              |    (dsp-agent, gui-agent, etc.)|
              |                               |
              |  - Reads injected resources   |
              |  - Uses content in work       |
              |  - Reports resources_consulted|
              |    in JSON output             |
              +-------------------------------+
                              |
                   5. SubagentStop validates
                      resource usage
                              |
                              v
              +-------------------------------+
              |    USAGE VALIDATION           |
              |    (SubagentStop.sh +          |
              |     validate-resource-usage.py)|
              |                               |
              |  - Parse agent JSON report    |
              |  - Check resources_consulted  |
              |    against injected resources |
              |  - Warn if MUST-READ skipped  |
              +-------------------------------+
```

### Component Responsibilities

| Component | Responsibility | Existing Pattern It Follows | New/Modified |
|-----------|----------------|----------------------------|-------------|
| Resource Index | Static catalog of all resources with tags | plugin-registry.json | NEW file |
| Discovery Script | Match task context to relevant resources | template-lookup.py | NEW script |
| Skill Orchestrator | Inject discovered resources into agent prompt | Contract injection in plugin-workflow | MODIFIED (3 skills) |
| Agent Definitions | Declare `resources_consulted` in output report | JSON report format already exists | MODIFIED (output section) |
| SubagentStop Hook | Validate resource usage after agent completion | Existing SubagentStop.sh pattern | MODIFIED (add case) |
| Usage Validator | Compare injected vs consulted resources | validate-dsp-components.py pattern | NEW validator |

---

## Question 1: Where Does Resource Discovery Happen?

**Answer: In the skill orchestrator, before agent spawning.**

### Why the Skill Orchestrator (Not a Hook, Not the Agent)

**Option A: In a hook (UserPromptSubmit or custom PreAgentSpawn)**
- Hooks have tight timeouts (2-10s)
- Hooks output to stderr/stdout, which is injected as system context
- Hooks cannot modify the Task() prompt that the skill constructs
- Hooks run AFTER the skill has already built the prompt
- **Verdict: Wrong layer.** Hooks validate/inject at the conversation level, not at the agent spawning level.

**Option B: In the agent itself (agent reads index, discovers own resources)**
- Agents run in fresh contexts with limited initial knowledge
- Would require every agent to have Bash/Read tools to run discovery
- Wastes agent execution time on discovery instead of domain work
- Discovery logic duplicated across 11 agent definitions
- **Verdict: Wrong responsibility.** Agents should execute domain work, not meta-work about resource discovery.

**Option C: In the skill orchestrator, before Task() invocation** (RECOMMENDED)
- Orchestrators already load contracts, state, and context before spawning agents
- Orchestrators already construct the prompt string for Task()
- Adding resource discovery is a natural extension of prompt construction
- Centralized logic, not duplicated across agents
- Matches existing pattern: `load_contracts(plugin_name)` then `invoke_task(prompt=...)`
- **Verdict: Correct layer.** This is where context assembly already happens.

### Integration Point: plugin-workflow

Current code (from `run_execute_phase`):
```python
# Current: contracts loaded and injected into prompt
contracts = load_contracts(plugin_name)
plan = read_file(f"plugins/{plugin_name}/.planning/stages/{stage}/PLAN.md")

result = invoke_task(
    subagent_type=agent,
    prompt=f"""
    Implement {stage} for {plugin_name}.

    Contracts:
    - BRIEF.md: {contracts.brief_summary}
    - ARCHITECTURE.md: {contracts.arch_summary}
    ...
    """
)
```

Proposed change:
```python
# New: discover resources THEN inject into prompt
contracts = load_contracts(plugin_name)
plan = read_file(f"plugins/{plugin_name}/.planning/stages/{stage}/PLAN.md")

# NEW: Resource discovery
resources = discover_resources(plugin_name, stage, agent)

result = invoke_task(
    subagent_type=agent,
    prompt=f"""
    Implement {stage} for {plugin_name}.

    Contracts:
    - BRIEF.md: {contracts.brief_summary}
    - ARCHITECTURE.md: {contracts.arch_summary}
    ...

    ## Relevant Resources (READ BEFORE IMPLEMENTING)

    **MUST-READ:** These contain directly relevant algorithms and patterns:
    {format_must_read(resources)}

    **SHOULD-READ:** These may contain useful context:
    {format_should_read(resources)}

    Read these files using the Read tool before starting implementation.
    Report which resources you consulted in your JSON output.
    """
)
```

### Integration Point: plugin-improve

The plugin-improve skill has a different flow (Phase 0.5 investigation, not staged execution). Resource discovery should happen at Phase 0.5 (Investigation) where the skill already searches for relevant context:

```python
# Phase 0.5: Investigation
# NEW: Before investigating, discover relevant research
resources = discover_resources(plugin_name, improvement_context, "plugin-improve")

# Present to user or pass to Tier 2/3 investigation
```

### Integration Point: plugin-planning (research-planning-agent)

The research-planning-agent already does external research via WebSearch and Context7. Resource discovery would surface existing research docs to avoid duplicate investigation:

```python
# Before spawning research-planning-agent
resources = discover_resources(plugin_name, "stage-0-research", "research-planning-agent")

# Inject into prompt: "These research docs already exist for related topics..."
```

---

## Question 2: How Does Context Flow from Discovery to Injection?

**Answer: Appended to the agent prompt as file paths with read instructions.**

### Why Prompt Injection (Not Context Files, Not Hook Injection)

**Option A: Write a context file, agent reads it**
- Adds a write-then-read step (slower)
- Creates file clutter in `.planning/`
- Agent might not read it (no enforcement)
- **Verdict: Unnecessary indirection.** The prompt IS the context delivery mechanism.

**Option B: Hook injects via stderr/stdout**
- Hooks inject at conversation level, not agent level
- Cannot target specific Task() invocations
- Timeout constraints limit processing
- **Verdict: Wrong mechanism.** Hooks are for conversation-level context, not agent-level.

**Option C: Append to Task() prompt string** (RECOMMENDED)
- Direct delivery to agent context
- Agent sees resources as part of its instructions
- No intermediate files
- Orchestrator controls exactly what's injected
- Matches existing pattern (contracts are already prompt-injected)
- **Verdict: Correct mechanism.** This is how the system already delivers context.

### Context Format

The injected context should be:

1. **File paths** (not file contents) -- agent uses Read tool to load
2. **Priority classification** (MUST-READ vs SHOULD-READ)
3. **Relevance summary** (why this resource matches)
4. **Read instruction** (explicit directive to use Read tool)

```markdown
## Relevant Resources

**MUST-READ** (directly relevant to your task):
1. `research/modal-synthesis-bells-academic-research.md`
   - Contains: Frequency-dependent damping formulas, multi-stage decay envelopes
   - Relevance: Core algorithm reference for bell modal synthesis

2. `research/multi-stage-decay-envelopes-comparison.md`
   - Contains: Comparison of decay envelope approaches
   - Relevance: Implementation options for bell decay behavior

**SHOULD-READ** (potentially useful context):
3. `research/custom-fft-implementations.md`
   - Contains: FFT library comparison, integration patterns
   - Relevance: May be useful if spectral analysis needed

**Instructions:** Read MUST-READ resources using Read tool before starting implementation.
Include `resources_consulted` array in your JSON output report listing files you read.
```

### Why File Paths Not Contents

- Research docs are 10-70KB each (modal-synthesis is 12KB, microtonality-implementation is 72KB)
- Injecting full content would consume significant context window budget
- Agent can selectively read sections (Read tool with offset/limit)
- Agent decides what depth of reading is needed
- Prevents context window overflow on multiple large docs

---

## Question 3: What New Components Are Needed?

### Component 1: Resource Index (NEW)

**File:** `.claude/resource-index.json`
**Purpose:** Static catalog of all resources with searchable metadata
**Follows pattern of:** `.claude/plugin-registry.json`

```json
{
  "$schema": "./schemas/resource-index.schema.json",
  "version": "1.0.0",
  "generated": "2026-02-04",
  "resources": {
    "research/modal-synthesis-bells-academic-research.md": {
      "title": "Modal Synthesis for Bells and Metallic Percussion",
      "type": "algorithm-reference",
      "tags": ["modal-synthesis", "bells", "metallic", "percussion", "physical-modeling", "damping", "envelopes"],
      "dsp_topics": ["synthesis", "envelopes", "frequency-dependent-damping", "modal-analysis"],
      "relevant_agents": ["dsp-agent", "research-planning-agent"],
      "relevant_plugins": ["O-Bells"],
      "keywords": ["CCRMA", "decay", "partials", "resonance", "modes", "inharmonicity"],
      "summary": "Academic compilation from CCRMA Stanford and IRCAM on modal synthesis. Includes mathematical formulas for frequency-dependent damping, multi-stage decay envelopes, and specific parameter values.",
      "size_kb": 12
    },
    "research/custom-fft-implementations.md": {
      "title": "Custom FFT Implementations for Audio Plugins",
      "type": "technology-comparison",
      "tags": ["fft", "spectral", "performance", "libraries"],
      "dsp_topics": ["fft", "spectral-processing", "convolution", "frequency-domain"],
      "relevant_agents": ["dsp-agent", "research-planning-agent"],
      "relevant_plugins": ["O-SpectralShaper", "O-FreqPulse"],
      "keywords": ["vDSP", "Accelerate", "FFTW", "PFFFT", "KissFFT", "AudioFFT"],
      "summary": "Comparison of FFT libraries (Apple Accelerate, Intel IPP, FFTW3, PFFFT, KissFFT) with benchmarks, licensing, and JUCE integration patterns.",
      "size_kb": 21
    }
  }
}
```

**Index structure rationale:**
- `tags`: Broad category labels for fuzzy matching
- `dsp_topics`: DSP-specific classification for dsp-agent matching
- `relevant_agents`: Which agents would benefit from this resource
- `relevant_plugins`: Direct plugin association (from past usage)
- `keywords`: Fine-grained terms for keyword matching against task context
- `summary`: One-paragraph description for the agent to assess relevance
- `size_kb`: Helps agent decide whether to read full file or skim

**Maintenance:** Static file, manually updated when new research docs are added. Could be regenerated by a maintenance script, but not auto-generated on every invocation (too slow for hook timeouts).

### Component 2: Discovery Script (NEW)

**File:** `.claude/scripts/resource-discovery.py`
**Purpose:** Given task context, return ranked list of relevant resources
**Follows pattern of:** `.claude/scripts/template-lookup.py`

```python
#!/usr/bin/env python3
"""Resource discovery for agent context injection.

Usage:
  python3 .claude/scripts/resource-discovery.py \
    --plugin O-Bells \
    --agent dsp-agent \
    --stage 2-dsp \
    --context "modal synthesis, bell sounds, decay envelopes" \
    [--limit 5]

Output: JSON array of matched resources with priority and relevance.
"""

import json
import sys
import argparse
from pathlib import Path

def load_index():
    index_path = Path(".claude/resource-index.json")
    if not index_path.exists():
        return {"resources": {}}
    with open(index_path) as f:
        return json.load(f)

def score_resource(resource_meta, plugin, agent, stage, context_terms):
    """Score a resource's relevance to the current task.

    Scoring weights:
    - Direct plugin match: +10
    - Agent relevance: +5
    - Tag match: +3 per matching tag
    - DSP topic match: +3 per matching topic
    - Keyword match: +1 per matching keyword
    - Context term match: +2 per matching term
    """
    score = 0

    # Direct plugin match (highest signal)
    if plugin and plugin in resource_meta.get("relevant_plugins", []):
        score += 10

    # Agent relevance
    if agent and agent in resource_meta.get("relevant_agents", []):
        score += 5

    # Tag matching
    tags = set(resource_meta.get("tags", []))
    for term in context_terms:
        term_lower = term.lower().strip()
        if term_lower in tags:
            score += 3

    # DSP topic matching
    dsp_topics = set(resource_meta.get("dsp_topics", []))
    for term in context_terms:
        term_lower = term.lower().strip()
        if term_lower in dsp_topics:
            score += 3

    # Keyword matching (broader)
    keywords = set(resource_meta.get("keywords", []))
    for term in context_terms:
        term_lower = term.lower().strip()
        for keyword in keywords:
            if term_lower in keyword.lower() or keyword.lower() in term_lower:
                score += 1

    return score

def discover(plugin, agent, stage, context, limit=5):
    index = load_index()
    results = []

    context_terms = [t.strip() for t in context.split(",") if t.strip()]

    # Also extract terms from stage name
    if stage:
        stage_terms = stage.replace("-", " ").split()
        context_terms.extend(stage_terms)

    for path, meta in index.get("resources", {}).items():
        score = score_resource(meta, plugin, agent, stage, context_terms)
        if score > 0:
            priority = "MUST-READ" if score >= 8 else "SHOULD-READ"
            results.append({
                "path": path,
                "title": meta.get("title", path),
                "summary": meta.get("summary", ""),
                "score": score,
                "priority": priority,
                "size_kb": meta.get("size_kb", 0)
            })

    # Sort by score descending
    results.sort(key=lambda r: r["score"], reverse=True)

    return results[:limit]

def main():
    parser = argparse.ArgumentParser(description="Discover relevant resources")
    parser.add_argument("--plugin", default="")
    parser.add_argument("--agent", default="")
    parser.add_argument("--stage", default="")
    parser.add_argument("--context", default="")
    parser.add_argument("--limit", type=int, default=5)
    parser.add_argument("--format", choices=["json", "markdown"], default="json")

    args = parser.parse_args()
    results = discover(args.plugin, args.agent, args.stage, args.context, args.limit)

    if args.format == "json":
        print(json.dumps(results, indent=2))
    else:
        # Markdown format for direct prompt injection
        must_read = [r for r in results if r["priority"] == "MUST-READ"]
        should_read = [r for r in results if r["priority"] == "SHOULD-READ"]

        if must_read:
            print("**MUST-READ** (directly relevant to your task):")
            for i, r in enumerate(must_read, 1):
                print(f"{i}. `{r['path']}`")
                print(f"   - {r['summary']}")
                print()

        if should_read:
            print("**SHOULD-READ** (potentially useful context):")
            for i, r in enumerate(should_read, len(must_read) + 1):
                print(f"{i}. `{r['path']}`")
                print(f"   - {r['summary']}")
                print()

    return 0

if __name__ == "__main__":
    sys.exit(main())
```

**Performance constraint:** This script must complete in under 2 seconds. With a static JSON index and simple scoring, it will run in ~50ms for 23 resources. No external calls, no file scanning, no LLM inference.

### Component 3: Context Extraction Helper (NEW, optional)

**File:** `.claude/scripts/extract-context-terms.py`
**Purpose:** Extract context terms from plugin planning files for discovery input
**When needed:** When the skill orchestrator needs to build the `--context` argument

```python
#!/usr/bin/env python3
"""Extract searchable context terms from plugin planning artifacts.

Usage:
  python3 .claude/scripts/extract-context-terms.py plugins/O-Bells/.planning/

Output: Comma-separated context terms suitable for resource-discovery.py --context
"""

import sys
import re
from pathlib import Path

def extract_from_brief(brief_path):
    """Extract plugin type, sonic goals, key features from BRIEF.md"""
    terms = []
    if brief_path.exists():
        content = brief_path.read_text()
        # Extract plugin type
        for line in content.split("\n"):
            if "type:" in line.lower():
                terms.extend(line.split(":")[-1].strip().split())
            # Extract key features mentioned
            if any(kw in line.lower() for kw in ["feature", "algorithm", "technique", "synthesis", "effect"]):
                words = re.findall(r'\b[a-z]{3,}\b', line.lower())
                terms.extend(words)
    return terms

def extract_from_architecture(arch_path):
    """Extract DSP components, algorithms from ARCHITECTURE.md"""
    terms = []
    if arch_path.exists():
        content = arch_path.read_text()
        # Extract section headers as topics
        headers = re.findall(r'^#{1,3}\s+(.+)$', content, re.MULTILINE)
        for h in headers:
            terms.extend(h.lower().split())
        # Extract JUCE class names
        juce_classes = re.findall(r'juce::dsp::(\w+)', content)
        terms.extend([c.lower() for c in juce_classes])
    return terms

def main():
    planning_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(".")

    terms = set()
    terms.update(extract_from_brief(planning_dir / "BRIEF.md"))
    terms.update(extract_from_architecture(planning_dir / "research" / "ARCHITECTURE.md"))

    # Deduplicate and filter noise words
    noise = {"the", "and", "for", "with", "from", "this", "that", "are", "was", "will", "has", "been"}
    filtered = [t for t in terms if t not in noise and len(t) > 2]

    print(",".join(sorted(set(filtered))))

if __name__ == "__main__":
    main()
```

### Component 4: Usage Validator (NEW)

**File:** `.claude/hooks/validators/validate-resource-usage.py`
**Purpose:** Parse agent JSON report for `resources_consulted` field, compare against injected resources
**Follows pattern of:** `validate-dsp-components.py`, `validate-parameters.py`

```python
#!/usr/bin/env python3
"""Validate that agent consulted injected resources.

Reads agent JSON report and compares resources_consulted against
the MUST-READ resources that were injected.

Exit codes:
  0 = All MUST-READ resources consulted
  1 = MUST-READ resources skipped (blocking)
  2 = SHOULD-READ resources skipped (warning only)
"""

import json
import sys

def main():
    # Read agent report from stdin or argument
    report = json.loads(sys.stdin.read())

    consulted = set(report.get("resources_consulted", []))
    injected_must = set(report.get("_injected_must_read", []))
    injected_should = set(report.get("_injected_should_read", []))

    # Check MUST-READ compliance
    must_missed = injected_must - consulted
    should_missed = injected_should - consulted

    if must_missed:
        print(f"WARNING: Agent skipped {len(must_missed)} MUST-READ resources:", file=sys.stderr)
        for path in must_missed:
            print(f"  - {path}", file=sys.stderr)
        # WARNING not BLOCKING -- agent may have valid reason
        # (e.g., resource turned out to be irrelevant after reading summary)
        return 2

    if should_missed:
        print(f"INFO: Agent skipped {len(should_missed)} SHOULD-READ resources (acceptable)", file=sys.stderr)
        return 0

    print("Resource usage validated: all MUST-READ resources consulted", file=sys.stderr)
    return 0

if __name__ == "__main__":
    sys.exit(main())
```

**Design decision: Warning, not blocking.** The validator issues warnings for skipped MUST-READ resources but does not block the workflow (exit code 2, not 1). Rationale:
- The agent may determine after reading the summary that a resource is not relevant
- Blocking on resource usage would create false positives
- The traceability value comes from the report, not from enforcement
- Blocking should be reserved for contract violations, not resource suggestions

### Component 5: Resource Index Schema (NEW)

**File:** `.claude/schemas/resource-index.schema.json`
**Purpose:** Validate resource-index.json structure

---

## Question 4: How Do Post-Agent Hooks Validate Resource Usage?

**Answer: Parse the agent's JSON report for a `resources_consulted` field.**

### Modified SubagentStop.sh

Add a new case to the existing SubagentStop.sh:

```bash
# After existing agent validation cases...

# Resource usage validation (all agents)
if [ -n "$PLUGIN_NAME" ]; then
  echo "Validating resource usage..." >&2
  python3 .claude/hooks/validators/validate-resource-usage.py
  RESOURCE_RESULT=$?
  if [ $RESOURCE_RESULT -eq 2 ]; then
    echo "WARNING: Some MUST-READ resources were not consulted" >&2
    # Continue but warn -- not blocking
  fi
fi
```

**Why SubagentStop and not a separate hook:**
- SubagentStop already runs after every agent completion
- Already has access to agent output and plugin context
- Adding a case follows the existing pattern
- No need for a new hook type

### What Gets Validated

| Check | Severity | Exit Code | Blocks Workflow |
|-------|----------|-----------|-----------------|
| All MUST-READ consulted | Info | 0 | No |
| Some MUST-READ skipped | Warning | 2 | No |
| `resources_consulted` field missing entirely | Warning | 2 | No |
| SHOULD-READ skipped | Info | 0 | No |

**Rationale for non-blocking:** Resource usage is advisory, not contractual. The primary value is traceability -- knowing which resources were available and which were actually used. Blocking would be appropriate only if we're certain the resources are relevant, but discovery scoring is heuristic.

---

## Question 5: What Changes to Existing Agent Contracts?

### New Output Field: `resources_consulted`

Add to the unified subagent report schema (`.claude/schemas/subagent-report.json`):

```json
{
  "resources_consulted": {
    "type": "array",
    "items": { "type": "string" },
    "description": "File paths of research/reference resources the agent read during execution. Include only resources actually loaded via Read tool.",
    "default": []
  }
}
```

**This field is optional, not required.** Agents that receive no resource injection (because discovery found nothing relevant) will omit it or return an empty array.

### New Input Context: Resource Section in Prompt

This is not a schema change -- it is a prompt augmentation. The orchestrator appends a `## Relevant Resources` section to the agent prompt. No schema change needed for this because the prompt is a free-form string.

### Modified Agent Definitions

Each agent definition (`.claude/agents/*.md`) should document the `resources_consulted` field in its JSON report section. Example addition to dsp-agent.md:

```markdown
**Extended success report (with resource tracking):**

```json
{
  "agent": "dsp-agent",
  "status": "success",
  "outputs": {
    "plugin_name": "[PluginName]",
    "dsp_components": [...],
    "processing_chain": "Input -> Filter -> Gain -> Output"
  },
  "resources_consulted": [
    "research/modal-synthesis-bells-academic-research.md",
    "research/multi-stage-decay-envelopes-comparison.md"
  ],
  "issues": [],
  "ready_for_next_stage": true
}
```

### Agents Affected

| Agent | Receives Resources | Reports Usage | Priority |
|-------|-------------------|---------------|----------|
| dsp-agent | Yes (DSP research) | Yes | HIGH |
| research-planning-agent | Yes (existing research to avoid re-research) | Yes | HIGH |
| gui-agent | Yes (UI patterns, WebGL research) | Yes | MEDIUM |
| foundation-shell-agent | Rarely (mostly template-driven) | Yes | LOW |
| polish-agent | Occasionally (optimization patterns) | Yes | LOW |
| music-theory-agent | Yes (microtonality, theory docs) | Yes | MEDIUM |
| troubleshoot-agent | Yes (debugging patterns) | Yes | MEDIUM |

---

## Question 6: How Does the research/ Folder Get Indexed?

**Answer: Static manifest with manual maintenance, plus a regeneration script for bulk updates.**

### Why Static Manifest (Not Dynamic Scanning)

**Option A: Dynamic scanning at discovery time**
- Must scan 23 files, parse frontmatter, extract metadata on every invocation
- Hook timeout constraints (2-10s) make this risky with larger research folders
- File parsing is fragile (research docs have no standardized frontmatter)
- Would need NLP or keyword extraction to generate tags
- **Verdict: Too slow, too fragile.**

**Option B: Dynamic scanning at session start (SessionStart hook)**
- SessionStart already validates environment
- Could scan research/ and rebuild index
- But SessionStart has 5s timeout -- marginal for 23 files, problematic at 50+
- Would run on every session, even when research/ hasn't changed
- **Verdict: Workable but wasteful.**

**Option C: Static manifest with regeneration script** (RECOMMENDED)
- Index is a JSON file that gets committed to git
- Updated when research docs are added/modified
- Regeneration script available for bulk rebuilds
- Discovery reads a single JSON file (fast, predictable)
- **Verdict: Simple, fast, reliable.**

### Index Generation Script

**File:** `.claude/scripts/generate-resource-index.py`
**Purpose:** Scan research/ folder and generate/update resource-index.json
**When to run:** Manually after adding new research docs, or as part of commit process

The script:
1. Scans `research/` recursively for `.md` files
2. For each file, extracts: title (first H1), size, headings
3. Generates initial tags from headings and content keywords
4. Merges with existing index to preserve manual tag refinements
5. Outputs updated `.claude/resource-index.json`

**Human curation expected:** Auto-generated tags are a starting point. The `relevant_agents`, `relevant_plugins`, `summary`, and refined `tags` should be human-curated for best discovery quality.

### Per-Plugin Research

Some research lives in `plugins/[Name]/.planning/research/` (per-plugin ARCHITECTURE.md). These should also be indexed but are already loaded via contract injection. The resource index focuses on the shared `research/` folder that contains cross-cutting knowledge.

---

## Recommended Project Structure (New/Modified Files)

```
.claude/
  resource-index.json          # NEW: Static resource catalog
  schemas/
    resource-index.schema.json # NEW: Schema for resource index
  scripts/
    resource-discovery.py      # NEW: Discovery script
    extract-context-terms.py   # NEW: Context extraction helper
    generate-resource-index.py # NEW: Index regeneration
    template-lookup.py         # EXISTING (unchanged)
    plugin-registry.py         # EXISTING (unchanged)
  hooks/
    hooks.json                 # MODIFIED: Add resource validation
    SubagentStop.sh            # MODIFIED: Add resource usage check
    validators/
      validate-resource-usage.py # NEW: Resource usage validator
  agents/
    dsp-agent.md               # MODIFIED: Add resources_consulted to report
    gui-agent.md               # MODIFIED: Add resources_consulted to report
    research-planning-agent.md # MODIFIED: Add resources_consulted to report
    (other agents similarly)   # MODIFIED: Add resources_consulted to report
  skills/
    plugin-workflow/
      SKILL.md                 # MODIFIED: Add discovery call before Task()
    plugin-improve/
      SKILL.md                 # MODIFIED: Add discovery in Phase 0.5
    plugin-planning/
      SKILL.md                 # MODIFIED: Add discovery before research-planning-agent
```

---

## Data Flow: End-to-End Example

**Scenario:** User runs `/implement O-Bells` and system reaches Stage 2 (DSP execute phase).

```
1. plugin-workflow reads O-Bells state
   - Plugin: O-Bells
   - Stage: 2-dsp
   - Agent: dsp-agent
   - ARCHITECTURE.md mentions: "modal synthesis", "frequency-dependent damping"

2. Skill extracts context terms
   $ python3 .claude/scripts/extract-context-terms.py plugins/O-Bells/.planning/
   > "modal,synthesis,bells,damping,envelopes,partials,decay,metallic"

3. Skill runs resource discovery
   $ python3 .claude/scripts/resource-discovery.py \
       --plugin O-Bells \
       --agent dsp-agent \
       --stage 2-dsp \
       --context "modal,synthesis,bells,damping,envelopes,partials,decay,metallic" \
       --format markdown

   > **MUST-READ:**
   > 1. `research/modal-synthesis-bells-academic-research.md`
   >    - Contains frequency-dependent damping formulas, modal analysis
   > 2. `research/multi-stage-decay-envelopes-comparison.md`
   >    - Contains decay envelope implementation comparison

   > **SHOULD-READ:**
   > 3. `research/generative-audio-algorithms-reference.md`
   >    - Contains algorithmic patterns for generative audio

4. Skill constructs agent prompt with resources appended
   Task(
     subagent_type="dsp-agent",
     prompt="""
       Read these contracts:
       - plugins/O-Bells/.planning/BRIEF.md
       - plugins/O-Bells/.planning/research/ARCHITECTURE.md
       - plugins/O-Bells/.planning/parameter-spec.md

       ## Relevant Resources (READ BEFORE IMPLEMENTING)

       **MUST-READ:**
       1. `research/modal-synthesis-bells-academic-research.md`
          - Frequency-dependent damping formulas and modal analysis parameters
       2. `research/multi-stage-decay-envelopes-comparison.md`
          - Decay envelope approaches for bell synthesis

       **SHOULD-READ:**
       3. `research/generative-audio-algorithms-reference.md`
          - Algorithmic patterns for generative audio

       Read MUST-READ resources before implementing.
       Report resources_consulted in your JSON output.

       Implement Stage 2 DSP for O-Bells...
     """
   )

5. dsp-agent executes
   - Reads ARCHITECTURE.md (contract)
   - Reads modal-synthesis-bells-academic-research.md (MUST-READ)
   - Reads multi-stage-decay-envelopes-comparison.md (MUST-READ)
   - Implements modal synthesis using formulas from research
   - Returns JSON report with resources_consulted

6. SubagentStop validates
   - Contract validation passes
   - Resource usage: 2/2 MUST-READ consulted (PASS)
   - DSP component validation passes
```

---

## Architectural Patterns

### Pattern 1: Static Index with Heuristic Scoring

**What:** Pre-computed resource metadata with runtime keyword matching instead of semantic search.

**Why this over semantic/embedding search:**
- No external API calls (works offline, no latency)
- Deterministic results (same query = same results)
- Human-curated tags are more reliable than auto-embeddings for a 23-doc corpus
- Script runs in <100ms
- No dependencies beyond Python stdlib

**Trade-offs:**
- Requires manual index maintenance when docs change
- Keyword matching misses semantic similarity ("reverb" won't match "room acoustics" unless tagged)
- Quality depends on tag curation

**When to upgrade:** If the research folder grows beyond ~100 docs, consider adding embedding-based search. At 23 docs, keyword matching with curated tags is optimal.

### Pattern 2: Priority-Classified Injection

**What:** Resources classified as MUST-READ or SHOULD-READ, not just a flat list.

**Why:**
- Agents have limited context window budget
- MUST-READ signals "this directly affects your work quality"
- SHOULD-READ signals "skim if you have capacity"
- Prevents information overload while ensuring critical resources aren't missed

**Classification threshold:** Score >= 8 = MUST-READ, lower = SHOULD-READ.
The threshold is tunable -- start here and adjust based on observed agent behavior.

### Pattern 3: Report-Based Accountability

**What:** Agents self-report which resources they consulted via JSON output field.

**Why not file access logs:**
- Claude Code agents use the Read tool, but tracking which Read calls were for "resource consultation" vs "contract reading" vs "code inspection" requires parsing all Read call paths
- Self-reporting is simpler and more reliable
- Aligns with existing JSON report pattern
- Agent has incentive to report accurately (it's in the instructions)

**Trade-off:** Agent could lie (report reading something it didn't). This is acceptable because:
- The value is traceability, not enforcement
- If agent reports reading a doc but doesn't use its content, that's an agent quality issue, not an architecture issue
- The SubagentStop validator catches the case where agent doesn't report at all

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: Full Content Injection

**What people do:** Inject entire research doc contents into agent prompt.
**Why it's wrong:** Research docs are 10-72KB. Injecting 3 docs at 30KB each uses 90KB of context window -- nearly the entire budget for a Sonnet agent.
**Do this instead:** Inject file paths with summaries. Agent reads what it needs via Read tool.

### Anti-Pattern 2: Real-Time Index Generation

**What people do:** Scan and parse all research files during discovery.
**Why it's wrong:** Hook timeout constraints (2-10s), fragile parsing of unstructured markdown, slow for growing corpus.
**Do this instead:** Static index file, generated offline, read at discovery time.

### Anti-Pattern 3: Mandatory Enforcement

**What people do:** Block workflow if agent doesn't read all suggested resources.
**Why it's wrong:** Discovery is heuristic. False positives in resource matching would block valid agent work. Resources may become irrelevant once agent reads the summary.
**Do this instead:** Warning-level validation. Log what was suggested vs consulted. Let humans audit the gap.

### Anti-Pattern 4: Discovery in Every Agent

**What people do:** Put discovery logic in each agent definition so agents self-discover resources.
**Why it's wrong:** Duplicates logic across 11 agents. Agents waste execution time on discovery instead of domain work. Discovery logic changes require updating all agents.
**Do this instead:** Centralize discovery in the orchestrator. Agents receive pre-discovered resources.

### Anti-Pattern 5: Over-Indexing

**What people do:** Add 50 metadata fields per resource to catch every possible match.
**Why it's wrong:** Maintenance burden grows linearly with field count. Most fields add marginal discovery value. Index becomes stale quickly.
**Do this instead:** 7 fields (title, type, tags, dsp_topics, relevant_agents, relevant_plugins, keywords, summary). Expand only if discovery quality proves insufficient.

---

## Build Order (Suggested Implementation Phases)

### Phase 1: Foundation (Resource Index + Discovery Script)

**Dependencies:** None (greenfield)
**Creates:**
- `.claude/resource-index.json` -- Manually create with all 23 research docs
- `.claude/scripts/resource-discovery.py` -- Discovery script
- `.claude/schemas/resource-index.schema.json` -- Validation schema

**Testable independently:** Run discovery script from CLI and verify results make sense for known plugin/research combinations.

**Estimated effort:** 1 phase, moderate complexity

### Phase 2: Injection (Modify Skill Orchestrators)

**Dependencies:** Phase 1 (index and script must exist)
**Modifies:**
- `.claude/skills/plugin-workflow/SKILL.md` -- Add discovery + injection in execute phase
- `.claude/skills/plugin-planning/SKILL.md` -- Add discovery before research-planning-agent
- `.claude/skills/plugin-improve/SKILL.md` -- Add discovery in investigation phase

**Testable:** Run a plugin workflow and verify agent prompt includes resource section.

**Estimated effort:** 1 phase, moderate complexity (careful SKILL.md editing)

### Phase 3: Accountability (Agent Contracts + Validation)

**Dependencies:** Phase 2 (agents must receive resources to report on them)
**Modifies:**
- `.claude/agents/dsp-agent.md` -- Add resources_consulted to report format
- `.claude/agents/gui-agent.md` -- Add resources_consulted to report format
- `.claude/agents/research-planning-agent.md` -- Add resources_consulted to report format
- (5 more agent definitions)
- `.claude/hooks/SubagentStop.sh` -- Add resource usage validation case
- `.claude/schemas/subagent-report.json` -- Add optional resources_consulted field

**Creates:**
- `.claude/hooks/validators/validate-resource-usage.py`

**Testable:** Run a full plugin workflow and verify SubagentStop reports resource usage.

**Estimated effort:** 1 phase, low-moderate complexity

### Phase 4: Tooling (Index Maintenance)

**Dependencies:** Phase 1 (index format must be stable)
**Creates:**
- `.claude/scripts/generate-resource-index.py` -- Index regeneration from research/ scan
- `.claude/scripts/extract-context-terms.py` -- Context extraction helper

**Testable:** Run generation script, verify output matches manual index, verify context extraction produces reasonable terms.

**Estimated effort:** 1 phase, low complexity

### Phase Ordering Rationale

```
Phase 1 (Index + Script)
    |
    +--- Must exist before orchestrators can discover resources
    |
Phase 2 (Injection)
    |
    +--- Must exist before agents receive resources
    |
Phase 3 (Accountability)
    |
    +--- Must exist after agents receive resources (otherwise nothing to validate)
    |
Phase 4 (Tooling)
    |
    +--- Maintenance tooling, not blocking for core functionality
```

---

## Integration Points Summary

| Existing Component | Integration Type | Change Description |
|-------------------|-----------------|-------------------|
| `plugin-workflow/SKILL.md` | MODIFY | Add discovery call + prompt section in `run_execute_phase` and `run_research_phase` |
| `plugin-improve/SKILL.md` | MODIFY | Add discovery in Phase 0.5 investigation |
| `plugin-planning/SKILL.md` | MODIFY | Add discovery before research-planning-agent spawn |
| `SubagentStop.sh` | MODIFY | Add resource-usage validation case after existing validation |
| `hooks.json` | NO CHANGE | SubagentStop already runs for all agents |
| `dsp-agent.md` | MODIFY | Add `resources_consulted` to JSON report docs |
| `gui-agent.md` | MODIFY | Add `resources_consulted` to JSON report docs |
| `research-planning-agent.md` | MODIFY | Add `resources_consulted` to JSON report docs |
| `foundation-shell-agent.md` | MODIFY | Add `resources_consulted` to JSON report docs |
| `polish-agent.md` | MODIFY | Add `resources_consulted` to JSON report docs |
| `plugin-registry.json` | NO CHANGE | Resource index is separate file |
| `SessionStart.sh` | NO CHANGE | Index is static, no session-start scanning needed |
| `PostToolUse.sh` | NO CHANGE | Resource validation happens at agent stop, not tool use |
| `UserPromptSubmit.sh` | NO CHANGE | Resource injection is at agent level, not user prompt level |

---

## Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| Discovery location (orchestrator) | HIGH | Clear from codebase analysis -- orchestrators already construct prompts with contracts |
| Injection mechanism (prompt append) | HIGH | Matches existing contract injection pattern exactly |
| Index format (static JSON) | HIGH | Follows plugin-registry.json pattern, proven in this system |
| Scoring algorithm | MEDIUM | Heuristic -- will need tuning based on real usage |
| Accountability approach | MEDIUM | Self-reporting has inherent trust assumptions |
| Build order | HIGH | Dependencies are clear and linear |

---

## Open Questions

1. **Score threshold for MUST-READ vs SHOULD-READ:** Starting at score >= 8. May need adjustment after observing real discovery results across multiple plugins.

2. **Maximum resources injected:** Currently limited to 5 via `--limit`. May need per-agent tuning (dsp-agent might benefit from more, foundation-shell-agent from fewer).

3. **Cross-plugin research:** Should discovery consider research from other plugin's `.planning/research/` folders? Currently scoped to shared `research/` only. Could expand later.

4. **Research doc frontmatter standardization:** If research docs adopted YAML frontmatter with tags, the index generation script could auto-extract metadata. Currently, metadata is human-curated in the index. Worth considering for future research docs.

5. **Discovery for improvement workflows:** plugin-improve has no explicit stage context. How to extract context terms from a freeform improvement request? The `--context` flag accepts freeform text, but quality depends on term extraction from the user's request.

---

*Architecture research for: Resource Discovery & Context Injection*
*Researched: 2026-02-04*
*Researcher: gsd-project-researcher agent*
