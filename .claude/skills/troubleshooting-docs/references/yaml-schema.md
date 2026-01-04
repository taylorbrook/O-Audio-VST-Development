# YAML Frontmatter Schema

**See `.claude/skills/troubleshooting-docs/schema.yaml` for the complete schema specification.**

## Required Fields

- **plugin** (string): Plugin name (e.g., "GainKnob") or "JUCE" for generic issues
- **date** (string): ISO 8601 date (YYYY-MM-DD)
- **problem_type** (enum): One of [build_error, runtime_error, api_misuse, validation_failure, ui_layout, dsp_issue, state_management, performance, thread_violation]
- **component** (enum): One of [cmake, juce_processor, juce_editor, juce_dsp, juce_gui_basics, juce_audio_utils, apvts, pluginval, xcode, system, webview]
- **symptoms** (array): 1-5 specific observable symptoms
- **root_cause** (enum): One of [missing_framework, missing_module, wrong_api, thread_violation, missing_constraint, state_sync, memory_issue, config_error, version_incompatibility, logic_error, event_timing, url_protocol]
- **resolution_type** (enum): One of [code_fix, config_change, environment_setup, api_migration]
- **severity** (enum): One of [critical, moderate, minor]

## Optional Fields

- **juce_version** (string): JUCE version in X.Y.Z format
- **tags** (array): Searchable keywords (lowercase, hyphen-separated)

## Validation Rules

1. All required fields must be present
2. Enum fields must match allowed values exactly (case-sensitive)
3. symptoms must be YAML array with 1-5 items
4. date must match YYYY-MM-DD format
5. juce_version (if provided) must match X.Y.Z format
6. tags should be lowercase, hyphen-separated

## Example

```yaml
---
plugin: GainKnob
date: 2025-11-10
problem_type: build_error
component: cmake
symptoms:
  - "Missing juce_dsp module causing link errors"
  - "Undefined symbols for juce::dsp::Compressor"
root_cause: missing_module
juce_version: 8.0.9
resolution_type: config_change
severity: critical
tags: [build, cmake, linker, juce-dsp]
---
```

## Category Mapping

Based on `problem_type`, documentation is filed in:

- **build_error** → `troubleshooting/build-failures/`
- **runtime_error** → `troubleshooting/runtime-issues/`
- **ui_layout** → `troubleshooting/gui-issues/`
- **api_misuse** → `troubleshooting/api-usage/`
- **dsp_issue** → `troubleshooting/dsp-issues/`
- **state_management** → `troubleshooting/parameter-issues/`
- **performance** → `troubleshooting/runtime-issues/`
- **thread_violation** → `troubleshooting/runtime-issues/`
- **validation_failure** → `troubleshooting/validation-problems/`
