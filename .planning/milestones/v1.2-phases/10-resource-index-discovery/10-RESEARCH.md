# Phase 10: Resource Index & Discovery - Research

**Researched:** 2026-02-05
**Domain:** Static document indexing, YAML frontmatter, keyword-based relevance ranking, JSON schema validation
**Confidence:** HIGH

## Summary

Phase 10 builds a static resource discovery system for a corpus of ~27 research documents. The system has three deliverables: (1) a YAML frontmatter schema that gets added to every research document as the metadata source of truth, (2) a manifest generator that reads frontmatter and produces `resource-index.json`, and (3) a discovery script that queries the manifest given task context (stage, agent role, keywords) and returns ranked results.

The existing research documents have **zero YAML frontmatter** today. All 27 documents use ad-hoc markdown headers for metadata (title, date, confidence level). Phase 10 must retrofit frontmatter onto every existing document, then build the tooling that consumes it. The corpus is small enough that simple weighted keyword scoring with stage/role priority multipliers is sufficient -- no NLP libraries, no vector search, no TF-IDF needed.

**Primary recommendation:** Use Python stdlib + PyYAML (already installed) for frontmatter parsing and keyword matching. Install `jsonschema` (pip) for manifest schema validation. Implement a weighted scoring algorithm where stage match and agent-role match contribute high base scores, and keyword overlap provides supplementary ranking.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Python | 3.14.2 | Runtime for all scripts | Already installed, project standard for validators |
| PyYAML | 6.0.3 | Parse YAML frontmatter from markdown files | Already installed (`pip3 list` confirmed) |
| jsonschema | latest (4.x) | Validate `resource-index.json` against JSON Schema Draft 2020-12 | Python standard for JSON schema validation; project already uses Draft 2020-12 schemas for all workflow artifacts |
| json (stdlib) | builtin | Read/write manifest JSON | Zero dependency |
| pathlib (stdlib) | builtin | File path operations | Zero dependency, matches existing validator patterns |
| re (stdlib) | builtin | Frontmatter delimiter parsing, keyword extraction | Zero dependency |
| dataclasses (stdlib) | builtin | Structured result objects | Matches pattern in `contract_validator.py` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| argparse (stdlib) | builtin | CLI for manifest generator (rebuild command) | Generator script only |
| hashlib (stdlib) | builtin | Manifest content hash for cache invalidation | Optional: skip re-validation if manifest unchanged |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| jsonschema (pip) | Hand-rolled structural validator | Avoids dependency but fragile; success criteria specifically requires "validates against JSON schema" |
| jsonschema (pip) | ajv-cli via subprocess | Already in project (validate-handoff.sh uses it), but subprocess overhead on every query violates 1-second budget for cold starts |
| python-frontmatter (pip) | PyYAML + 10-line regex parser | python-frontmatter is unnecessary; YAML frontmatter is just `---\nyaml\n---` delimiters, trivially parseable with PyYAML alone |
| TF-IDF / BM25 / rank_bm25 | Weighted keyword set intersection | 23 documents do not justify information retrieval libraries; simple scoring is deterministic and debuggable |

**Installation:**
```bash
pip3 install jsonschema
```

## Architecture Patterns

### Recommended Project Structure
```
.claude/
├── resource-index.json              # Generated manifest (DISC-01)
├── resource-index.schema.json       # JSON Schema for manifest validation (DISC-01)
├── hooks/
│   └── validators/
│       └── validate-research-frontmatter.py  # Commit hook validator
└── scripts/
    ├── generate-resource-index.py    # Manifest generator (reads frontmatter, writes JSON)
    └── discover-resources.py         # Discovery script (DISC-02, DISC-03, DISC-04)

research/
├── *.md                             # All docs get YAML frontmatter added
└── stutter-effects/
    ├── *.md                         # Nested docs also get frontmatter
    └── README.md                    # Index file - NO frontmatter (skipped by generator)
```

### Pattern 1: YAML Frontmatter Schema

**What:** Every research document gets a YAML frontmatter block as its source of truth for discovery metadata.
**When to use:** Every `.md` file in `research/` that represents a research document (not README index files).

**Frontmatter fields:**
```yaml
---
title: "FFT Processing Best Practices in JUCE"
summary: "Comprehensive guide to STFT architecture, window functions, overlap-add synthesis, and artifact prevention for real-time spectral processing in JUCE plugins."
domain: dsp           # One of: dsp, ui, build, workflow
type: guide           # One of: algorithm, pattern, guide, reference
keywords:
  - fft
  - stft
  - spectral
  - windowing
  - overlap-add
  - juce-dsp
  - artifact-prevention
stages: [1, 2, 3]    # Which pipeline stages this is relevant to (0-4)
agents: [dsp]         # Which agent roles benefit: dsp, ui, build, research
---
```

**Required fields:** `title`, `summary`, `domain`, `type`, `keywords`, `stages`, `agents`
**Optional fields:** None initially -- keep the schema tight and add fields only when downstream consumers need them.

**Field definitions:**
- `title`: Human-readable document title (string, max ~100 chars)
- `summary`: 2-3 sentence description sufficient for relevance ranking without reading the document (string, max ~300 chars)
- `domain`: Primary domain tag -- exactly one of `dsp`, `ui`, `build`, `workflow` (string, enum)
- `type`: Document type tag -- exactly one of `algorithm`, `pattern`, `guide`, `reference` (string, enum)
- `keywords`: Array of lowercase keyword strings for matching (array of strings, min 3, max 15)
- `stages`: Array of integers 0-4 indicating which pipeline stages this document is relevant to (array of integers)
- `agents`: Array of agent role strings indicating which agent types benefit from this document (array of strings, enum values: `dsp`, `ui`, `build`, `research`)

### Pattern 2: Manifest Structure (resource-index.json)

**What:** Flat JSON array of document entries, generated from frontmatter.
**When to use:** Generated by `generate-resource-index.py`, consumed by `discover-resources.py`.

```json
{
  "$schema": "./resource-index.schema.json",
  "version": "1.0.0",
  "generated": "2026-02-05T12:00:00Z",
  "documents": [
    {
      "path": "research/fft-processing-best-practices.md",
      "title": "FFT Processing Best Practices in JUCE",
      "summary": "Comprehensive guide to STFT architecture...",
      "domain": "dsp",
      "type": "guide",
      "keywords": ["fft", "stft", "spectral", "windowing"],
      "stages": [1, 2, 3],
      "agents": ["dsp"]
    }
  ]
}
```

**Key design decisions:**
- **Entry key**: Use `path` field with relative file path (e.g., `research/fft-processing-best-practices.md`) -- unique, self-documenting, no slug generation needed
- **Flat array**: Documents stored in a flat `documents` array, not nested by category -- easier to filter/sort programmatically
- **No content**: Manifest does NOT include document content, only metadata -- keeps file small and fast to load
- **Generated timestamp**: Tracks when manifest was last rebuilt for staleness detection

### Pattern 3: Discovery Query Interface

**What:** Python function that accepts a structured context dict and returns ranked results.
**When to use:** Called by orchestrators (Phase 11) before invoking subagents.

```python
def discover(
    stage: int,                    # Current pipeline stage (0-4)
    agent_role: str,               # Agent role: "dsp", "ui", "build", "research"
    keywords: list[str] = None,    # Optional task-specific keywords
    plugin_name: str = None,       # Optional plugin name for future use
    max_results: int = 10,         # Safety cap (not a fixed count)
    threshold: float = 0.3         # Minimum relevance score (0.0 - 1.0)
) -> list[dict]:
    """Returns ranked list of relevant resources."""
```

**Return format:**
```python
[
    {
        "path": "research/fft-processing-best-practices.md",
        "title": "FFT Processing Best Practices in JUCE",
        "summary": "Comprehensive guide to STFT architecture...",
        "relevance": 0.85,
        "tier": "primary",         # "primary" or "supplementary"
        "match_reasons": ["stage_match", "role_match", "keyword:fft"]
    }
]
```

**Tier definitions:**
- `primary`: Document matches both stage AND agent role (high confidence relevance)
- `supplementary`: Document matches keywords but not stage/role, or matches only one context dimension

This tiering directly supports the Phase 11 4,000-token budget: primary resources get injected first, supplementary fills remaining budget.

### Pattern 4: Weighted Scoring Algorithm

**What:** Simple numeric scoring with context-priority weighting.
**When to use:** Inside the discovery script for ranking results.

```python
def score_document(doc: dict, stage: int, agent_role: str, keywords: list[str]) -> float:
    score = 0.0

    # Stage match: highest weight (0.4 max)
    if stage in doc["stages"]:
        score += 0.4

    # Agent role match: high weight (0.35 max)
    if agent_role in doc["agents"]:
        score += 0.35

    # Keyword overlap: supplementary weight (0.25 max)
    if keywords:
        doc_keywords = set(doc["keywords"])
        query_keywords = set(k.lower() for k in keywords)
        overlap = len(doc_keywords & query_keywords)
        if overlap > 0:
            # Normalize: min(overlap/3, 1.0) so 3+ matches gets full keyword score
            keyword_score = min(overlap / 3.0, 1.0) * 0.25
            score += keyword_score

    return score
```

**Weight rationale:**
- Stage (0.4): Decisions say "context trumps keywords" -- stage is the strongest signal
- Agent role (0.35): Agent affinity is the second strongest signal
- Keywords (0.25): Supplementary signal, capped so keyword-only matches stay below threshold unless very strong
- Threshold of 0.3 means: stage-only match (0.4) passes, role-only match (0.35) passes, keyword-only matches need 4+ keyword hits to pass alone

### Pattern 5: Frontmatter Parsing (No External Dependency)

**What:** Parse YAML frontmatter using only PyYAML + regex.
**When to use:** In both the manifest generator and the frontmatter validation hook.

```python
import re
import yaml

def parse_frontmatter(filepath: str) -> dict | None:
    """Extract YAML frontmatter from a markdown file.

    Returns parsed dict or None if no frontmatter found.
    """
    with open(filepath, 'r') as f:
        content = f.read()

    match = re.match(r'^---\n(.*?)\n---', content, re.DOTALL)
    if not match:
        return None

    return yaml.safe_load(match.group(1))
```

### Anti-Patterns to Avoid

- **Manual manifest editing**: The manifest MUST be generated from frontmatter, never hand-edited. Manual edits will be overwritten on next generation.
- **Content-based ranking**: Do NOT read document content for ranking. Summaries and keywords in frontmatter are sufficient. Reading 27 files on every query is slow and unnecessary.
- **Agent-name matching**: Map agent names to roles, not the reverse. The decision says "uses roles not named agents." The mapping is: `dsp-agent -> dsp`, `gui-agent -> ui`, `foundation-shell-agent -> build`, `research-planning-agent -> research`, etc.
- **Universal stage tag**: Do NOT create a "universal" or "all stages" concept. Cross-cutting docs list all applicable stages explicitly: `stages: [0, 1, 2, 3, 4]`.
- **Over-engineering the ranking**: For 23 documents, BM25/TF-IDF/cosine similarity are unnecessary. Simple weighted set intersection is deterministic, debuggable, and fast.

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| JSON Schema validation | Custom type/field checker | `jsonschema` library | Handles Draft 2020-12 features (enums, arrays, required fields, patterns), edge cases around null/empty, and provides clear error messages |
| YAML parsing | Custom YAML parser | `yaml.safe_load()` (PyYAML) | Already installed, handles all YAML edge cases (multiline strings, special characters, Unicode) |
| File discovery | Shell globbing via subprocess | `pathlib.Path.rglob('*.md')` | Cross-platform, handles nested directories, returns Path objects |
| JSON pretty-printing | Manual string formatting | `json.dumps(indent=2, ensure_ascii=False)` | Handles Unicode, consistent formatting |

**Key insight:** The heaviest dependency decision is jsonschema vs hand-rolled validation. For the manifest schema (which is simple and controlled by us), hand-rolling would work, but the success criteria explicitly states "validates the manifest against its JSON schema" -- this implies a real JSON Schema file and real validation, not a simplified approximation. Use jsonschema.

## Common Pitfalls

### Pitfall 1: Frontmatter Drift from Document Content
**What goes wrong:** Frontmatter keywords and summary become stale as document content evolves over time.
**Why it happens:** Research documents are updated by various agents, but frontmatter is treated as static metadata.
**How to avoid:** Phase 13 (Maintenance Tooling) addresses this with auto-generation. For Phase 10, ensure the commit hook validates frontmatter structure (not semantic accuracy). Accept that some drift is inevitable for a 23-doc corpus.
**Warning signs:** Discovery returns documents that aren't actually relevant when inspected.

### Pitfall 2: The Stutter Effects Subdirectory
**What goes wrong:** Generator misses nested files or incorrectly includes `README.md` index files.
**Why it happens:** `research/stutter-effects/` contains 5 files: 3 research documents, 1 findings overview, and 1 README index.
**How to avoid:** Use `pathlib.rglob('*.md')` for recursive discovery. Skip files named `README.md` (index files, not research docs). Require frontmatter presence for inclusion -- files without valid frontmatter are simply skipped.
**Warning signs:** Manifest has fewer than expected entries, or includes non-research files.

### Pitfall 3: jsonschema Import Failure in Hooks
**What goes wrong:** Hook timeout or crash because jsonschema is not installed or import fails.
**Why it happens:** Hooks have tight timeouts (2-10 seconds in hooks.json). If jsonschema is installed in a different Python environment than the hook uses, import fails.
**How to avoid:** The discovery script (not the hook) does schema validation. The frontmatter validation hook only checks structural presence of YAML fields using PyYAML (already installed). Keep jsonschema usage in the discovery script and generator only.
**Warning signs:** Hook failures with `ModuleNotFoundError: No module named 'jsonschema'`.

### Pitfall 4: Stage Number Mismatch Between Systems
**What goes wrong:** Frontmatter uses stage numbers 0-4 but the registry uses strings like `"0-ideation"`, `"2-dsp"`.
**Why it happens:** Two different stage representation systems exist in the codebase.
**How to avoid:** Frontmatter and manifest use integers (0-4). The discovery script accepts integers. Any orchestrator calling discovery must extract the numeric prefix from the registry's stage string (e.g., `"2-dsp"` -> `2`). Document this mapping explicitly.
**Warning signs:** Discovery returns empty results because stage `"2-dsp"` doesn't match stage `2`.

### Pitfall 5: Keyword Normalization
**What goes wrong:** Keywords like "FFT" in a query don't match "fft" in frontmatter, or "overlap-add" doesn't match "overlap_add".
**Why it happens:** No normalization convention established.
**How to avoid:** Define convention: all frontmatter keywords are lowercase, hyphen-separated. Discovery script lowercases query keywords before matching. No stemming or lemmatization -- exact match only (corpus is small enough).
**Warning signs:** Queries with obvious keywords return no results.

### Pitfall 6: Empty Manifest on First Run
**What goes wrong:** Discovery script runs before manifest is generated, gets an empty or missing file.
**Why it happens:** Script order dependency -- generate must run before discover.
**How to avoid:** Discovery script checks if manifest exists and is non-empty. If missing, return empty array silently (per decision: "Empty results return an empty array silently"). The generator must be run explicitly, not on-demand.
**Warning signs:** All discovery queries return empty results.

## Code Examples

### YAML Frontmatter for a DSP Research Document
```yaml
---
title: "FFT Processing Best Practices in JUCE"
summary: "Comprehensive guide to implementing high-quality FFT-based audio processing in JUCE plugins, covering STFT architecture, window functions, overlap-add synthesis, and artifact prevention."
domain: dsp
type: guide
keywords:
  - fft
  - stft
  - spectral-processing
  - windowing
  - overlap-add
  - juce-dsp
  - artifact-prevention
  - buffer-management
stages: [1, 2, 3]
agents: [dsp]
---
```

### YAML Frontmatter for a UI Research Document
```yaml
---
title: "WebGL Spectrogram Implementation Patterns"
summary: "Implementation patterns for real-time WebGL spectrograms with heat overlay blending for JUCE 8 WebView plugins, including fragment shader colormaps and circular buffer textures."
domain: ui
type: pattern
keywords:
  - webgl
  - spectrogram
  - visualization
  - webview
  - fragment-shader
  - real-time-rendering
stages: [3]
agents: [ui]
---
```

### YAML Frontmatter for a Cross-Cutting Document
```yaml
---
title: "DSP Click Prevention and Debugging Guide"
summary: "Reference for understanding, preventing, and debugging audio clicks and pops in JUCE audio plugins, covering signal discontinuities, buffer boundaries, and parameter smoothing."
domain: dsp
type: reference
keywords:
  - clicks
  - pops
  - audio-artifacts
  - parameter-smoothing
  - buffer-boundaries
  - debugging
stages: [1, 2, 3, 4]
agents: [dsp, build]
---
```

### YAML Frontmatter for a Market Research Document
```yaml
---
title: "O-Detune Market Research"
summary: "Competitive landscape analysis of detuning and pitch thickening plugins, covering Soundtoys MicroShift, Eventide MicroPitch, and identifying market gaps for analog-style wobble with unison detuning."
domain: workflow
type: reference
keywords:
  - market-research
  - detuning
  - pitch-thickening
  - competitive-analysis
  - product-planning
stages: [0]
agents: [research]
---
```

### JSON Schema for resource-index.json
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "./resource-index.schema.json",
  "title": "Resource Index",
  "description": "Manifest cataloging all research documents with discovery metadata",
  "type": "object",
  "required": ["$schema", "version", "generated", "documents"],
  "additionalProperties": false,
  "properties": {
    "$schema": { "type": "string" },
    "version": { "type": "string", "const": "1.0.0" },
    "generated": { "type": "string", "format": "date-time" },
    "documents": {
      "type": "array",
      "items": { "$ref": "#/$defs/DocumentEntry" }
    }
  },
  "$defs": {
    "DocumentEntry": {
      "type": "object",
      "required": ["path", "title", "summary", "domain", "type", "keywords", "stages", "agents"],
      "additionalProperties": false,
      "properties": {
        "path": { "type": "string", "pattern": "^research/.+\\.md$" },
        "title": { "type": "string", "minLength": 1, "maxLength": 150 },
        "summary": { "type": "string", "minLength": 10, "maxLength": 500 },
        "domain": { "type": "string", "enum": ["dsp", "ui", "build", "workflow"] },
        "type": { "type": "string", "enum": ["algorithm", "pattern", "guide", "reference"] },
        "keywords": {
          "type": "array",
          "items": { "type": "string", "pattern": "^[a-z0-9-]+$" },
          "minItems": 3,
          "maxItems": 15
        },
        "stages": {
          "type": "array",
          "items": { "type": "integer", "minimum": 0, "maximum": 4 },
          "minItems": 1
        },
        "agents": {
          "type": "array",
          "items": { "type": "string", "enum": ["dsp", "ui", "build", "research"] },
          "minItems": 1
        }
      }
    }
  }
}
```

### Agent-to-Role Mapping
```python
# Maps agent names to discovery roles
# New agents just declare their role -- no mapping change needed
AGENT_ROLE_MAP = {
    "dsp-agent": "dsp",
    "gui-agent": "ui",
    "ui-design-agent": "ui",
    "ui-finalization-agent": "ui",
    "foundation-shell-agent": "build",
    "research-planning-agent": "research",
    "polish-agent": "dsp",           # Stage 4 polish is DSP-adjacent
    "validation-agent": "build",     # Validators need build context
    "troubleshoot-agent": "dsp",     # Troubleshooting is primarily DSP
    "music-theory-agent": "dsp",     # Music theory supports DSP decisions
    "aesthetics-agent": "ui",        # UI aesthetics
}
```

### Frontmatter Validation Hook Pattern
```python
#!/usr/bin/env python3
"""Validate YAML frontmatter in research documents."""
import sys
import yaml
import re
from pathlib import Path

REQUIRED_FIELDS = {"title", "summary", "domain", "type", "keywords", "stages", "agents"}
VALID_DOMAINS = {"dsp", "ui", "build", "workflow"}
VALID_TYPES = {"algorithm", "pattern", "guide", "reference"}
VALID_AGENTS = {"dsp", "ui", "build", "research"}

def validate_frontmatter(filepath: str) -> list[str]:
    """Returns list of error messages (empty = valid)."""
    errors = []

    with open(filepath, 'r') as f:
        content = f.read()

    match = re.match(r'^---\n(.*?)\n---', content, re.DOTALL)
    if not match:
        return ["Missing YAML frontmatter (expected --- delimiters)"]

    try:
        fm = yaml.safe_load(match.group(1))
    except yaml.YAMLError as e:
        return [f"Invalid YAML in frontmatter: {e}"]

    if not isinstance(fm, dict):
        return ["Frontmatter is not a YAML mapping"]

    # Check required fields
    missing = REQUIRED_FIELDS - set(fm.keys())
    if missing:
        errors.append(f"Missing required fields: {missing}")

    # Validate enum values
    if fm.get("domain") not in VALID_DOMAINS:
        errors.append(f"Invalid domain '{fm.get('domain')}' (must be one of {VALID_DOMAINS})")
    if fm.get("type") not in VALID_TYPES:
        errors.append(f"Invalid type '{fm.get('type')}' (must be one of {VALID_TYPES})")

    # Validate arrays
    if isinstance(fm.get("keywords"), list):
        for kw in fm["keywords"]:
            if not re.match(r'^[a-z0-9-]+$', str(kw)):
                errors.append(f"Keyword '{kw}' must be lowercase alphanumeric with hyphens")

    if isinstance(fm.get("agents"), list):
        for agent in fm["agents"]:
            if agent not in VALID_AGENTS:
                errors.append(f"Invalid agent role '{agent}' (must be one of {VALID_AGENTS})")

    if isinstance(fm.get("stages"), list):
        for stage in fm["stages"]:
            if not isinstance(stage, int) or stage < 0 or stage > 4:
                errors.append(f"Invalid stage '{stage}' (must be integer 0-4)")

    return errors
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual document lists in agent prompts | Auto-discovered relevant resources via manifest | Phase 10 (this phase) | Agents receive context-appropriate research automatically |
| Vector/embedding search for doc retrieval | Keyword matching with context-priority weighting | v1.2 decision | Simpler, no ML dependencies, sufficient for 23-doc corpus |
| Hook-level discovery injection | Orchestrator-level discovery | v1.2 decision | Avoids hook timeout constraints (2-10s), allows richer processing |

**Not applicable to this phase:**
- Vector databases (Pinecone, ChromaDB) -- overkill for 23 docs
- LLM-based ranking -- adds latency and cost for marginal benefit at this scale
- Elasticsearch/Solr -- server-based search for a static 23-doc corpus

## Open Questions

1. **Where exactly does resource-index.json live?**
   - What we know: The decision says "A JSON manifest exists at `.claude/resource-index.json`"
   - What's unclear: Should the schema file be co-located (`.claude/resource-index.schema.json`) or in the existing schema directory (`.planning/workflow/schemas/`)?
   - Recommendation: Co-locate schema with manifest at `.claude/resource-index.schema.json` for self-containment. The discovery system is a `.claude` concern, not a `.planning/workflow` concern.

2. **How should the generator handle the stutter-effects README.md?**
   - What we know: `research/stutter-effects/README.md` is an index/overview file, not a research document
   - What's unclear: Should it get frontmatter or be explicitly excluded?
   - Recommendation: Skip any file named `README.md` in the generator. These are directory indexes, not research documents. The frontmatter-required convention also naturally excludes them (no frontmatter = not indexed).

3. **Should the generator run as part of a hook or be manually triggered?**
   - What we know: Manifest is derived/generated, not manually maintained. Frontmatter is source of truth.
   - What's unclear: When does regeneration happen?
   - Recommendation: Provide a standalone generator script. Run manually after adding/modifying research docs. Do NOT run in hooks (too slow, unnecessary). Phase 13 may automate this.

4. **Document count discrepancy: 23 vs 27**
   - What we know: STATE.md says "23-doc corpus" but glob finds 27 .md files in research/
   - What's unclear: Which files are "research documents" vs index files or sub-documents?
   - Recommendation: The generator's frontmatter-required approach resolves this naturally. Only files with valid frontmatter get indexed. The planner should audit all 27 files and decide which ones get frontmatter. Likely ~23-24 are genuine research documents.

5. **Discovery script location**
   - What we know: Script is "internal only -- called programmatically by hooks/orchestrators"
   - What's unclear: Should it be `.claude/scripts/discover-resources.py` or `.planning/workflow/scripts/discover-resources.py`?
   - Recommendation: Place in `.claude/scripts/` alongside the manifest. The existing `.planning/workflow/scripts/` directory contains shell scripts for stage transitions and validation. Discovery is a `.claude`-level concern (agents, not workflow).

## Existing Codebase Inventory

### Research Documents (27 files in research/)

| File | Probable Domain | Probable Stages | Probable Agents |
|------|----------------|-----------------|-----------------|
| `fft-processing-best-practices.md` | dsp | [1, 2, 3] | [dsp] |
| `fft-artifact-prevention.md` | dsp | [2, 3] | [dsp] |
| `custom-fft-implementations.md` | dsp | [2] | [dsp] |
| `reverb-comprehensive-research.md` | dsp | [0, 1, 2] | [dsp, research] |
| `delay-effects-comprehensive-guide.md` | dsp | [0, 1, 2] | [dsp, research] |
| `dsp-click-prevention-debugging.md` | dsp | [1, 2, 3, 4] | [dsp, build] |
| `generative-audio-algorithms-reference.md` | dsp | [0, 2] | [dsp, research] |
| `generative-plugins-research-synthesis.md` | dsp | [0] | [research] |
| `modal-synthesis-bells-academic-research.md` | dsp | [0, 2] | [dsp, research] |
| `multi-stage-decay-envelopes-comparison.md` | dsp | [2] | [dsp] |
| `circuit-modeling-fundamentals.md` | dsp | [0, 2] | [dsp, research] |
| `physical-modeling-commercial-analog-modeling-ml-approaches.md` | dsp | [0, 2] | [dsp, research] |
| `physical-modeling-research-agent-3-physical-modelling-optimization.md` | dsp | [2] | [dsp] |
| `spectral-sequencer-research.md` | dsp | [0, 2] | [dsp, research] |
| `spectral-transient-shaper-research.md` | dsp | [0, 2] | [dsp, research] |
| `spectral-toolbox-synopses.md` | workflow | [0] | [research] |
| `microtonality-theory-formats.md` | dsp | [0, 2] | [dsp, research] |
| `microtonality-comprehensive-database.md` | dsp | [0, 2] | [dsp, research] |
| `microtonality-implementation-juce.md` | dsp | [1, 2] | [dsp] |
| `microtonality-commercial-performance.md` | workflow | [0] | [research] |
| `O-Detune-market-research.md` | workflow | [0] | [research] |
| `webgl-spectrogram-patterns.md` | ui | [3] | [ui] |
| `stutter-effects/stutter-effects-research-findings.md` | dsp | [0, 2] | [dsp, research] |
| `stutter-effects/path-a-granular-stutter-engine.md` | dsp | [0, 2] | [dsp, research] |
| `stutter-effects/path-b-beat-repeater.md` | dsp | [0, 2] | [dsp, research] |
| `stutter-effects/path-c-playhead-modulator.md` | dsp | [0, 2] | [dsp, research] |
| `stutter-effects/README.md` | (skip) | (skip) | (skip) |

**Summary:** ~24 DSP domain docs, ~1 UI domain doc, ~3 workflow/market research docs, 1 README to skip. The corpus is heavily DSP-weighted, which is expected for a VST plugin development project.

### Existing Agent Role Mapping

From auditing all 11 agent files in `.claude/agents/`:

| Agent File | Declared Role | Discovery Role |
|------------|--------------|----------------|
| dsp-agent.md | "audio processing algorithms and DSP" | dsp |
| gui-agent.md | "integrating WebView UI mockup and binding parameters" | ui |
| ui-design-agent.md | "generating UI mockup design files" | ui |
| ui-finalization-agent.md | "generating implementation files for WebView UI" | ui |
| foundation-shell-agent.md | "creating JUCE plugin project structure" | build |
| research-planning-agent.md | "DSP architecture research and planning" | research |
| polish-agent.md | "factory presets, performance optimization" | dsp |
| validation-agent.md | "semantic review of implementation stages" | build |
| troubleshoot-agent.md | "build failures, JUCE API issues" | dsp |
| music-theory-agent.md | "tuning systems, temperament calculations" | dsp |
| aesthetics-agent.md | "visual consistency, color theory" (SPEC ONLY) | ui |

### Existing Infrastructure Patterns

**Schema convention:** All schemas use JSON Schema Draft 2020-12, stored in `.planning/workflow/schemas/`, with `$id` self-references. The manifest schema should follow this convention.

**Validator convention:** Python validators live in `.claude/hooks/validators/`, use `dataclasses` for result types, and exit with code 0 (pass), 1 (error), or 2 (warning). The frontmatter validator should follow this pattern.

**Hook convention:** Hooks are bash scripts in `.claude/hooks/` registered in `hooks.json`. Python validators are called from bash wrappers. Timeouts range from 2000ms (PostToolUse) to 10000ms (SubagentStop).

**Script convention:** Workflow scripts are bash in `.planning/workflow/scripts/`. Discovery scripts should be Python in `.claude/scripts/` since they're agent-infrastructure, not workflow-infrastructure.

## Sources

### Primary (HIGH confidence)
- `/python-jsonschema/jsonschema` via Context7 - API usage, Draft 2020-12 support, validation patterns
- `/eyeseast/python-frontmatter` via Context7 - API for loading/parsing YAML frontmatter (evaluated and rejected in favor of PyYAML + regex)
- Direct codebase audit of all 27 research documents (none have frontmatter currently)
- Direct codebase audit of all 11 agent files (role mapping extracted)
- Direct audit of `.claude/hooks/hooks.json`, existing validators, and schemas

### Secondary (MEDIUM confidence)
- PyYAML 6.0.3 availability confirmed via `pip3 list`
- Python 3.14.2 confirmed via `python3 --version`
- jsonschema NOT currently installed (confirmed via import test)

### Tertiary (LOW confidence)
- None -- all findings verified against codebase and Context7

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries verified against installed environment and Context7 docs
- Architecture: HIGH - All patterns derived from existing codebase conventions and locked decisions in CONTEXT.md
- Pitfalls: HIGH - All identified from direct codebase audit (stage number mismatch, subdirectory structure, missing dependencies)

**Research date:** 2026-02-05
**Valid until:** 2026-03-07 (30 days -- stable domain, no fast-moving dependencies)
