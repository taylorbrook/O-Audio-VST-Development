---
name: install-plugin
description: Install completed plugin to system folders for DAW use
argument-hint: <PluginName>
---

# /install-plugin

<preconditions enforcement="blocking">
  <check target="PLUGINS.md" condition="status_equals" required="true">
    Plugin status MUST be ✅ Working (Stage 3 complete)
  </check>
  <check target="build" condition="exists" required="true">
    Plugin MUST have successful Release build
  </check>
  <check target="validation" condition="passed" required="true">
    pluginval validation MUST have passed
  </check>
  <check target="testing" condition="completed" required="true">
    Plugin MUST have been tested in DAW from build folder
  </check>
  <on_failure action="block">
    Guide user to complete Stage 3 first - DO NOT proceed
  </on_failure>
</preconditions>

<routing>
  <invoke skill="plugin-lifecycle" with="$ARGUMENTS" required="true">
    Pass plugin name to plugin-lifecycle skill for installation
  </invoke>
</routing>

<state_contracts>
  <reads target="PLUGINS.md">
    Plugin entry with status ✅ Working
  </reads>
  <writes target="PLUGINS.md">
    Update status: ✅ Working → 📦 Installed
  </writes>
  <writes target="logs/[PluginName]/build_TIMESTAMP.log">
    Build and installation log output
  </writes>
</state_contracts>
