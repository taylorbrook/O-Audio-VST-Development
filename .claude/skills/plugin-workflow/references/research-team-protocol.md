# Research Team Protocol

**Context:** This file is part of the plugin-workflow skill.
**Referenced by:** SKILL.md Research Team Integration section
**Purpose:** Detailed protocol for spawning and coordinating parallel research teams via the research-lead agent

---

## When to Use Parallel Research

The workflow orchestrator selects between sequential and parallel research automatically based on creative brief analysis:

**Sequential research (gsd-phase-researcher):**
- Plugin complexity 1-3
- Fewer than 5 parameters
- No custom DSP algorithms (uses existing modules only)
- Standard JUCE patterns (gain, filter, delay with known implementations)

**Parallel research (research-lead agent):**
- Plugin complexity 4+
- Custom DSP algorithms required (novel synthesis, custom reverb, spectral processing)
- 10+ parameters
- Novel techniques not covered by existing modules
- Multiple interacting DSP subsystems

## Domain Assignment

Domains are determined **dynamically at runtime** from the creative brief content. There is NO fixed set of domains. Each research session derives domains specific to the plugin being researched.

**Domain selection process:**
1. Research-lead reads the creative brief (BRIEF.md)
2. Identifies primary technical unknowns
3. Maps unknowns to research domains (2-3 domains)
4. Assigns one researcher per domain

**Example domain assignments:**

| Plugin Type | Domain 1 | Domain 2 | Domain 3 |
|-------------|----------|----------|----------|
| Shimmer Reverb | DSP algorithm approaches for shimmer reverb with pitch shifting | JUCE API mapping for reverb and pitch processing modules | -- |
| Granular Synth | Granular synthesis algorithms and grain scheduling | JUCE audio buffer management and MIDI voice allocation | UI patterns for real-time grain visualization |
| Multiband Comp | Crossover filter design and band splitting | Dynamics processing and envelope following | Metering and gain reduction visualization |

## Debate Format

When researchers produce findings, synthesis follows a structured debate protocol:

### Round 1: Independent Findings
- Each researcher presents their findings independently
- No cross-reading at this stage
- Structured format: approach, evidence, trade-offs, recommendation

### Round 2: Cross-Read and Contradiction Flagging
- Researchers read each other's findings
- Flag contradictions or incompatible recommendations
- Respond to other researchers' arguments with evidence
- Conflict detection script runs: `.claude/hooks/detect-research-conflicts.py`

### Round 3: Synthesis Attempt
- Researchers attempt to find compatible middle ground
- If compatible: produce merged recommendation
- If incompatible: document both positions clearly

**Maximum 3 debate rounds.** This limit prevents infinite deliberation loops.

## Conflict Resolution

After 3 rounds without consensus on incompatible approaches:

1. **BLOCK planning** -- do NOT proceed to plan phase
2. Document both positions with:
   - What the conflict is
   - Researcher A's position and evidence
   - Researcher B's position and evidence
   - Why they are incompatible
   - Research-lead's recommended resolution (if any)
3. Present to user for decision
4. User selects approach or provides alternative direction
5. Research proceeds with selected approach

**Conflicts that block:** Incompatible architectural approaches (e.g., FIR vs IIR for a specific filter requirement where both cannot coexist), mutually exclusive algorithm choices, contradictory JUCE API usage patterns.

**Conflicts that do NOT block:** Performance trade-offs (can be parameterized), minor implementation detail differences (choose either), optional feature scope (defer to planning).

## Output Format

The research team produces a merged research document with:

```markdown
# Research Synthesis: [Plugin Name]

## Team
- Researcher 1: [Domain 1]
- Researcher 2: [Domain 2]
- Researcher 3: [Domain 3] (if applicable)

## Synthesis
[Merged recommendations from all researchers]

## Individual Findings
### [Domain 1]
[Researcher 1's findings]

### [Domain 2]
[Researcher 2's findings]

### [Domain 3] (if applicable)
[Researcher 3's findings]

## Resolved Conflicts
[Any conflicts that were resolved during debate, with rationale]

## Open Questions
[Questions that need user input or further investigation]
```

## Graceful Degradation

If Agent Teams experimental feature fails (spawning error, timeout, feature disabled):

1. Log the failure
2. Fall back to sequential subagent research
3. Each researcher runs as a separate subagent (Task tool)
4. Debate happens via shared files instead of real-time communication
5. Research-lead reads all findings and performs synthesis manually

The sequential fallback produces the same output format. Quality may be slightly lower due to lack of real-time debate, but research coverage is maintained.

## Team Size

| Plugin Complexity | Parameters | Researchers | Rationale |
|-------------------|-----------|-------------|-----------|
| Simple (1-3) | 1-4 | N/A (use gsd-phase-researcher) | Sequential is sufficient |
| Moderate (4-6) | 5-9 | 2 | Two domains cover DSP + API |
| Complex (7+) | 10+ | 3 | Third researcher for UI/integration |

## Important Rules

1. **Dynamic domains only** -- never use a fixed set of researcher domains
2. **3-round debate cap** -- escalate after 3 rounds without consensus
3. **Conflicts block planning** -- incompatible approaches must be resolved before planning
4. **Delegate mode** -- research-lead coordinates but does not write research files directly
5. **Short sessions** -- keep research team lifetimes short, clean up after completion
