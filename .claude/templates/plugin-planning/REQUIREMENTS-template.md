# [PluginName] - Requirements

---
version: 1.0.0
plugin: [PluginName]
created: [YYYY-MM-DD]
lastUpdated: [YYYY-MM-DD]
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** [N]
**Coverage:** must: [N] | should: [N] | nice: [N]

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | [Core functionality requirement] | must | pending | stage-2 |
| FUNC-02 | [Additional functionality] | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | [Audio processing requirement] | must | pending | stage-2 |
| DSP-02 | [Sound quality requirement] | must | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | [Interface requirement] | should | pending | stage-3 |
| UI-02 | [Visual feedback requirement] | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations in processBlock) | must | pending | stage-2 |
| PERF-02 | CPU usage below [X]% at [N] voices | should | pending | stage-4 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Works in [target DAWs] | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges | must | pending | stage-2 |
| QUAL-02 | Smooth parameter automation without zipper noise | should | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: [Requirement Title]

**Description:** [Full description]

**Acceptance Criteria:**
- [ ] [Specific testable criterion 1]
- [ ] [Specific testable criterion 2]
- [ ] [Specific testable criterion 3]

**Notes:** [Any additional context]

---

### DSP-01: [Requirement Title]

**Description:** [Full description]

**Acceptance Criteria:**
- [ ] [Specific testable criterion 1]
- [ ] [Specific testable criterion 2]

**Notes:** [Any additional context]

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-*, DSP-*, PERF-01, QUAL-01, QUAL-02 |
| stage-3 | UI-* |
| stage-4 | COMPAT-02, PERF-02, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| [Deferred feature 1] | [Reason for deferral] | v1.1 |
| [Deferred feature 2] | [Reason for deferral] | v2.0 |

---
*Generated from BRIEF.md on [YYYY-MM-DD]*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
