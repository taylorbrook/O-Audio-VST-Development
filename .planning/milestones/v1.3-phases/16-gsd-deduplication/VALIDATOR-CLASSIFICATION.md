# Validator Classification: Structural vs Domain

**Date:** 2026-02-09
**Purpose:** Authoritative classification of all PFS validators as structural (replaced by gsd-tools verify suite) or domain (preserved). Satisfies decision P34: "Classify all custom code as duplicate/extension/workaround BEFORE removing." Informs GSDD-02 (frontmatter boundary) and GSDD-03 (structural verification replacement).

---

## Section 1: Structural Validators (Replaced by GSD)

These structural validation concerns were never separate Python scripts -- they existed as inline instructions in agent and workflow markdown files (e.g., gsd-plan-checker.md, verify-phase.md, execute-phase.md). The gsd-tools 1.18.0 verify suite provides deterministic CLI commands that replace all inline structural checks.

| Structural Check | gsd-tools Command | Status | Prior Location |
|-----------------|-------------------|--------|----------------|
| Plan structure (frontmatter, tasks, waves) | `verify plan-structure` | Replaced -- used by gsd-plan-checker Step 2 | Inline grep/parsing in agent markdown |
| Phase completeness (plans vs summaries) | `verify phase-completeness` | Replaced -- used by verify-phase | Inline ls + comparison in workflows |
| Reference resolution (@-refs, paths) | `verify references` | Replaced -- used by verify-phase | Manual file existence checks in agents |
| Commit verification (hash validation) | `verify commits` | Replaced -- used by verify-work | Manual `git cat-file` / `git log` loops |
| Artifact existence + content checks | `verify artifacts` | Replaced -- used by verify-phase | Manual file existence + line count checks |
| Key link wiring verification | `verify key-links` | Replaced -- used by verify-phase | Manual source-target pattern matching |

**Migration scope:** Replace callsites in agent/workflow instruction files from inline grep/parse to gsd-tools verify commands. No Python scripts to delete -- the structural validation was always inline in markdown prompts.

## Section 2: Domain Validators (Preserved -- No GSD Equivalent)

These validators perform domain-specific semantic checks that require knowledge of VST plugin architecture, JUCE framework, contract systems, and research document schemas. They have no GSD equivalent and serve an orthogonal purpose to the structural verify suite.

### Core Domain Validators (6)

| Validator | File | Domain | Why Preserved |
|-----------|------|--------|---------------|
| DSP Components | `validate-dsp-components.py` | Audio/DSP | Verifies architecture.md DSP components exist in PluginProcessor -- requires audio domain knowledge of processing chains and component wiring |
| Parameter Matching | `validate-parameters.py` | Audio/APVTS | Verifies parameter-spec.md IDs match APVTS code -- plugin-specific schema that maps UI controls to audio parameters |
| GUI Bindings | `validate-gui-bindings.py` | WebView | Verifies HTML parameter IDs match C++ WebView relay -- cross-language binding validation between web frontend and native backend |
| Checksums | `validate-checksums.py` | Contract integrity | SHA256 checksums of contract files vs STATUS.md records -- PFS-specific contract system integrity verification |
| Cross-Contract | `validate-cross-contract.py` | Contract consistency | Cross-references parameters/components across multiple PFS contract documents -- ensures spec coherence |
| Resource Accountability | `validate-resource-accountability.py` | Agent behavior | Verifies agents reported consulting injected MUST-READ resources -- PFS-specific agent accountability system |

### Additional Domain Validators (3)

| Validator | File | Domain | Why Preserved |
|-----------|------|--------|---------------|
| Silent Failures | `validate-silent-failures.py` | DSP safety | JUCE 8 runtime failure pattern detection -- identifies audio-specific anti-patterns that cause silent DSP failures |
| Foundation | `validate-foundation.py` | Build safety | CMakeLists.txt and source file existence validation -- PFS-specific build structure requirements |
| Research Frontmatter | `validate-research-frontmatter.py` | Research quality | 10-field YAML schema with enum values, date patterns, JUCE version format -- semantic validation far beyond presence-only checking (see Section 3) |

**Total domain validators: 9 (all preserved, no modifications)**

## Section 3: Frontmatter Boundary (GSDD-02)

The boundary between gsd-tools frontmatter validation and PFS research frontmatter validation is clear and documented here.

### gsd-tools Frontmatter Validation (Structural -- Presence Only)

| Schema | Fields Checked | Validation Type |
|--------|---------------|-----------------|
| `frontmatter validate --schema plan` | phase, plan, type, wave, depends_on, files_modified, autonomous, must_haves (8 fields) | Presence only -- checks field EXISTS in YAML |
| `frontmatter validate --schema summary` | phase, plan, subsystem, tags, key-files, completed (6 fields) | Presence only -- checks field EXISTS in YAML |
| `frontmatter validate --schema verification` | phase, status, score, timestamp (4 fields) | Presence only -- checks field EXISTS in YAML |

### PFS Research Frontmatter Validation (Domain -- Semantic)

| Field | Validation Type | Example Check |
|-------|----------------|---------------|
| `title` | Non-empty string | Must be present and non-blank |
| `summary` | Non-empty string | Must be present and non-blank |
| `domain` | Enum | Must be one of: dsp, gui, architecture, workflow, testing, deployment |
| `type` | Enum | Must be one of: algorithm, best-practice, comparison, reference, troubleshooting |
| `keywords` | List content | Must be non-empty list of strings |
| `stages` | List content | Must be non-empty list of valid stage names |
| `agents` | List content | Must be non-empty list of valid agent names |
| `created` | Date format | Must match YYYY-MM-DD pattern |
| `last_verified` | Date format | Must match YYYY-MM-DD pattern |
| `juce_version` | Version pattern | Must match JUCE version format (e.g., "8.0.4") |

### Boundary Conclusion

- **gsd-tools** handles structural frontmatter validation: "Does the field exist?" (presence-only, schema-aware)
- **PFS validate-research-frontmatter.py** handles semantic frontmatter validation: "Is the field value correct?" (enum matching, pattern validation, date parsing, list content checking)
- **No overlap** -- both are needed. gsd-tools cannot replace the PFS validator because it does not perform semantic value checking. The PFS validator does not duplicate gsd-tools because it validates a different schema (research documents, not plans/summaries).

## Section 4: SubagentStop.sh Dispatch (DO NOT TOUCH)

`SubagentStop.sh` is the hook that dispatches domain validators based on subagent type:

- `foundation-shell-agent` -> validate-foundation.py, validate-parameters.py
- `dsp-agent` -> validate-dsp-components.py
- `gui-agent` -> validate-gui-bindings.py
- All implementation agents -> validate-checksums.py, validate-cross-contract.py
- All agents -> validate-resource-accountability.py

**This hook and its validator invocations are explicitly OUT OF SCOPE for Phase 16.** No changes to SubagentStop.sh or any of its called Python validators. The hook is part of the domain validation infrastructure, not structural verification.

## Section 5: contract_validator.py (Shared Library -- Preserved)

`contract_validator.py` is a shared Python library used by:
- `validate-checksums.py` -- imports contract parsing and checksum computation functions
- `validate-cross-contract.py` -- imports contract reading and cross-reference utilities

It is part of the domain validation infrastructure and is preserved alongside all domain validators. No GSD equivalent exists or is needed.

## Section 6: Other Supporting Files (Not Validators)

| File | Role | Status |
|------|------|--------|
| `pre-stage-scan.py` | Pre-execution scanner | Preserved -- stage preparation, not validation dedup target |
| `validation-cache.py` | Cache layer for validators | Preserved -- supports domain validator performance |

---

**Classification complete.** 6 structural validation concerns replaced by gsd-tools verify suite. 9 domain validators preserved untouched. Frontmatter boundary documented. SubagentStop.sh dispatch out of scope. P34 compliance achieved.
