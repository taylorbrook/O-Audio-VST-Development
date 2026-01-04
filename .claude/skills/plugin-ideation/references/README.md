# Reference Documentation

This directory contains detailed workflow documentation loaded on-demand to optimize context window usage.

## Mode-Based Workflow References

The plugin-ideation skill uses progressive disclosure by loading only the relevant workflow after mode detection:

### Core Workflows

- **[new-plugin-workflow.md](new-plugin-workflow.md)** - 8-phase workflow for new plugin ideation
  - Free-form collection → Gap analysis → Adaptive questioning → Creative brief creation
  - Includes parallel workflow path (Phase 8.1) for faster iteration
  - Loaded when plugin does not exist in PLUGINS.md

- **[improvement-workflow.md](improvement-workflow.md)** - 7-phase workflow for improvement brainstorming
  - Vagueness detection → Free-form collection → Gap analysis → Improvement proposal creation
  - Loaded when plugin exists in PLUGINS.md

### Supporting Documentation

- **[adaptive-questioning-examples.md](adaptive-questioning-examples.md)** - Detailed question batch examples for new plugins
- **[improvement-mode-examples.md](improvement-mode-examples.md)** - Question batch examples for improvements
- **[adaptive-strategy.md](adaptive-strategy.md)** - Principles for adaptive questioning
- **[question-generation-patterns.md](question-generation-patterns.md)** - Patterns for generating meaningful questions
- **[parallel-workflow-test-scenario.md](parallel-workflow-test-scenario.md)** - Test cases for parallel workflow path

## Context Efficiency

This structure reduces initial skill load from ~8k tokens (889 lines) to ~2k tokens (98 lines) by:
1. Detecting mode first (New Plugin vs Improvement)
2. Loading only the relevant workflow reference
3. Keeping supporting examples in separate reference files

**Token savings:** ~4k per invocation (50% reduction in context load)

## Related Assets

Check `../assets/` for document templates:
- `creative-brief.md` - Template for new plugin briefs
- `improvement-proposal.md` - Template for improvement proposals
- `parameter-spec-draft-template.md` - Template for quick parameter capture
