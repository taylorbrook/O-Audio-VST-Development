# Success Menu Definitions

menus:
  stage_2_foundation:
    completion: "Foundation verified"
    options:
      - label: "Continue to Stage 3 (DSP)"
        recommended: true
      - label: "Review generated code"
      - label: "Pause workflow"

  stage_3_dsp:
    completion: "DSP implementation complete"
    options:
      - label: "Continue to Stage 3 (GUI)"
        recommended: true
      - label: "Test in DAW now"
      - label: "Review DSP code"
      - label: "Pause workflow"

  stage_3_gui:
    completion: "GUI implementation complete - Plugin validated and ready"
    options:
      - label: "Test in DAW now"
        recommended: true
      - label: "Review GUI code"
      - label: "Package for distribution"
      - label: "Start new plugin"

  plugin_improve:
    completion: "Improvements applied successfully"
    options:
      - label: "Test in DAW now"
        recommended: true
      - label: "Review changes"
      - label: "Apply more improvements"
      - label: "Finalize version"

## Usage

Load appropriate menu based on invoking context:
1. Extract stage number from invocation parameters (0, 2, 3, 4, 5)
2. Map to menu key: stage_2_foundation, stage_3_dsp, stage_4_gui, stage_5_validation, plugin_improve
3. Display completion message and options
4. Mark recommended option if specified
