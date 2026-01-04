---
name: package
description: Create branded PKG installer for plugin distribution
argument-hint: <PluginName>
---

# /package

<preconditions enforcement="blocking">
  <check target="PLUGINS.md" condition="status_equals" required="true">
    Plugin status MUST be 📦 Installed (binaries in system folders)
  </check>
  <check target="system-folders" condition="binaries_exist" required="true">
    VST3 and AU MUST exist in ~/Library/Audio/Plug-Ins/
  </check>
  <on_failure action="block">
    IF plugin not installed:
      Display: "Cannot package {{PLUGIN_NAME}} - plugin not installed"
      Guide: "Run: /install-plugin {{PLUGIN_NAME}}"

    IF binaries missing:
      Display: "VST3 or AU binaries not found in system folders"
      Guide: "Verify installation and try again"
  </on_failure>
</preconditions>

<routing>
  <invoke skill="plugin-packaging" with="$ARGUMENTS" required="true">
    Pass plugin name to plugin-packaging skill for PKG creation
  </invoke>
</routing>

<state_contracts>
  <reads target="PLUGINS.md">
    Plugin metadata (version, description, parameters, use cases)
  </reads>
  <reads target="plugins/{{PLUGIN_NAME}}/CMakeLists.txt">
    PRODUCT_NAME extraction
  </reads>
  <reads target="~/Library/Audio/Plug-Ins/VST3/">
    Source VST3 binary for packaging
  </reads>
  <reads target="~/Library/Audio/Plug-Ins/Components/">
    Source AU binary for packaging
  </reads>
  <writes target="plugins/{{PLUGIN_NAME}}/dist/">
    {{PLUGIN_NAME}}-by-TACHES.pkg and install-readme.txt
  </writes>
</state_contracts>

<success_criteria>
  Packaging succeeds when:
  - PKG file created with branded installer screens
  - Installation guide generated
  - Both files copied to plugins/{{PLUGIN_NAME}}/dist/
  - User presented with decision menu
</success_criteria>

<invocation_examples>
  User input: "/package TapeAge"
  → Creates: plugins/TapeAge/dist/TapeAge-by-TACHES.pkg

  User input: "/package GainKnob"
  → Creates: plugins/GainKnob/dist/GainKnob-by-TACHES.pkg

  User input: "Create installer for DriveVerb"
  → Natural language routing to plugin-packaging skill
</invocation_examples>
