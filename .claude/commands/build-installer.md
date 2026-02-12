---
name: build-installer
description: Create Windows EXE installer for VST3 plugin distribution using Inno Setup
argument-hint: <PluginName>
---

# /build-installer

<preconditions enforcement="blocking">
  <check target="PLUGINS.md" condition="status_equals" required="true">
    Plugin status MUST be 📦 Installed or ✅ Working (built VST3 artifact is sufficient for Windows)
  </check>
  <check target="build-artifacts" condition="vst3_build_exists" required="true">
    VST3 build artifact MUST exist at build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[ProductName].vst3
  </check>
  <check target="inno-setup" condition="iscc_available" required="true">
    Inno Setup compiler (iscc) MUST be on PATH or at C:\Program Files (x86)\Inno Setup 6\ISCC.exe
  </check>
  <on_failure action="block">
    IF plugin not built:
      Display: "Cannot create installer for {{PLUGIN_NAME}} - VST3 build artifact not found"
      Guide: "Run: /test {{PLUGIN_NAME}} build (or: powershell -File scripts/build-and-install.ps1 {{PLUGIN_NAME}} -NoInstall)"

    IF Inno Setup not installed:
      Display: "Inno Setup compiler (iscc) not found"
      Guide: "Install with: winget install JRSoftware.InnoSetup"
      Guide: "Or download from: https://jrsoftware.org/isdl.php"

    IF plugin not in PLUGINS.md:
      Display: "{{PLUGIN_NAME}} not found in PLUGINS.md"
      Guide: "Verify the plugin name matches a registry entry"
  </on_failure>
</preconditions>

<routing>
  <invoke skill="plugin-packaging" with="$ARGUMENTS" mode="windows" required="true">
    Pass plugin name to plugin-packaging Windows skill (SKILL-windows.md) for EXE creation
  </invoke>
</routing>

<state_contracts>
  <reads target="PLUGINS.md">
    Plugin metadata (version, description, status)
  </reads>
  <reads target="plugins/{{PLUGIN_NAME}}/CMakeLists.txt">
    PRODUCT_NAME and VERSION extraction
  </reads>
  <reads target=".claude/branding.json">
    Company name, website, copyright for installer branding
  </reads>
  <reads target="build/plugins/{{PLUGIN_NAME}}/{{PLUGIN_NAME}}_artefacts/Release/VST3/">
    Source VST3 build artifact for packaging
  </reads>
  <writes target="plugins/{{PLUGIN_NAME}}/dist/">
    {{PLUGIN_NAME}}-OuariconAudio-Setup.exe, install-readme-windows.txt, installer.iss
  </writes>
</state_contracts>

<success_criteria>
  Installer creation succeeds when:
  - EXE installer created with branded installer screens
  - Installation guide generated for Windows
  - Both files placed in plugins/{{PLUGIN_NAME}}/dist/
  - User presented with decision menu
</success_criteria>

<invocation_examples>
  User input: "/build-installer O-Chorus"
  -> Creates: plugins/O-Chorus/dist/O-Chorus-OuariconAudio-Setup.exe

  User input: "/build-installer GainKnob"
  -> Creates: plugins/GainKnob/dist/GainKnob-OuariconAudio-Setup.exe

  User input: "Create Windows installer for O-Tremolo"
  -> Natural language routing to plugin-packaging Windows skill
</invocation_examples>
