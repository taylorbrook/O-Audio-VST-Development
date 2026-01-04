---
name: show-standalone
description: Open plugin UI in Standalone mode for visual inspection
allowed-tools: [bash]
---

# /show-standalone

When user runs `/show-standalone [PluginName]`, execute directly (no skill routing).

## Preconditions

<preconditions enforcement="validate_before_execute">
  <check target="plugin_source" condition="directory_exists" path="plugins/$ARGUMENTS">
    Plugin source directory must exist at plugins/$ARGUMENTS
  </check>
  <check target="source_code" condition="has_source" scope="any_stage">
    Plugin must have source code (works at any development stage)
  </check>
</preconditions>

Note: This command works at any stage of development, even before completion.

## Behavior

<execution_sequence>
  <step order="1" required="true">
    Extract plugin name from $ARGUMENTS or context
  </step>

  <step order="2" conditional="true">
    Check if Standalone build exists:
    `build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Debug/Standalone/${PLUGIN_NAME}.app`
  </step>

  <step order="3" condition="!build_exists" tool="bash">
    Build Standalone target:
    `cmake --build build --target ${PLUGIN_NAME}_Standalone`
  </step>

  <step order="4" required="true" tool="bash">
    Launch application:
    `open "build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Debug/Standalone/${PLUGIN_NAME}.app"`
  </step>

  <step order="5" required="true">
    Report success with testing guidance
  </step>
</execution_sequence>

## Success Output

<success_response>
  <output format="compact">
    ✓ $PLUGIN_NAME UI opened successfully

    Testing checklist:
    - Controls layout and behavior
    - Parameter ranges and defaults
    - Graphics and component rendering
  </output>
</success_response>

## Troubleshooting

<error_handling>
  <error condition="build_fails">
    1. Verify CMakeLists.txt includes Standalone in FORMATS
    2. Check compilation errors
    3. Clean rebuild: `cmake --build build --target $PLUGIN_NAME_Standalone --clean-first`
  </error>

  <error condition="app_wont_launch">
    1. Check Console.app for crash logs
    2. Verify .app bundle exists at expected path
    3. Run from terminal for error output
  </error>

  <error condition="ui_blank">
    1. Check paint() method in PluginEditor.cpp
    2. Verify component initialization in constructor
    3. Look for assertion failures
  </error>
</error_handling>

## Use Cases

- Rapid UI iteration during Stage 3 (GUI)
- Visual design verification
- Quick parameter testing without DAW
- Layout debugging
- Component rendering checks

## Notes

- Uses Debug build (faster compilation during development)
- Provides built-in audio I/O for basic testing
- Supports MIDI input for testing MIDI-controlled parameters
- Not a replacement for full DAW testing
- Can keep open and rebuild to see changes
