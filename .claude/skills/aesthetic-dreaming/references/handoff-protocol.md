# Handoff Protocol

This document defines how aesthetic-dreaming integrates with other skills in the Plugin Freedom System.

---

## Invoked By

- /dream command (option: "Create aesthetic template")
- Natural language: "Create aesthetic template", "Design visual system"
- Direct skill invocation: Skill("aesthetic-dreaming")

---

## Invokes: ui-template-library

<skill name="ui-template-library">
  <operation>apply</operation>
  <context>Phase 5, Step 5: Test preview generation</context>

  <preconditions>
    MUST verify before invocation:
    - aesthetic.md exists at .claude/aesthetics/[id]/aesthetic.md
    - Test plugin spec exists in assets/test-plugin-specs.json
    - Aesthetic ID is valid and unique
    - Output directory .claude/aesthetics/[id]/test-previews/ exists
  </preconditions>

  <invocation_parameters>
    - aesthetic_id: string (e.g., "modern-professional-001")
    - target_plugin_name: string (e.g., "TestCompressor")
    - parameter_spec: object (from test-plugin-specs.json)
    - output_dir: string (.claude/aesthetics/[id]/test-previews/)
    - filename: string (e.g., "simple-compressor.html")
  </invocation_parameters>

  <postconditions>
    Verify after invocation:
    - HTML file created at specified path
    - File is valid standalone HTML (can be opened in browser)
    - Success status returned from skill
  </postconditions>

  <error_handling>
    IF skill invocation fails:
    - Log error message
    - Continue with remaining previews (don't abort entire Phase 5)
    - Report failure in Phase 5 confirmation message
  </error_handling>
</skill>

---

## Invokes: ui-mockup

<skill name="ui-mockup">
  <operation>create_mockup_with_aesthetic</operation>
  <context>Phase 6, Option 4: Apply to existing plugin</context>

  <preconditions>
    MUST verify before invocation:
    - Plugin exists in PLUGINS.md
    - Plugin has parameter-spec.md
    - Aesthetic exists in manifest.json
  </preconditions>

  <handoff_data>
    Pass to ui-mockup:
    - aesthetic_id: string (pre-select in Phase 0)
    - plugin_name: string (user selected from PLUGINS.md)
  </handoff_data>

  <postconditions>
    ui-mockup skill takes over:
    - Loads aesthetic.md for interpretation
    - Generates mockup with aesthetic applied
    - aesthetic-dreaming skill exits after handoff
  </postconditions>
</skill>

---

## Integration Notes

When aesthetic-dreaming creates an aesthetic, it becomes immediately available in ui-mockup Phase 0 aesthetic selection menu.
