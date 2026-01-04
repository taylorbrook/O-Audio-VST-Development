---
name: uninstall
description: Remove plugin from system folders (uninstall)
---

# /uninstall

<invocation>
  <parameter name="PluginName" position="1" required="true">
    Name of plugin to uninstall (must match PLUGINS.md entry)
  </parameter>

  <routing>
    Extract PluginName from $ARGUMENTS
    Invoke plugin-lifecycle skill with mode="uninstall"
  </routing>
</invocation>

## Preconditions

<preconditions enforcement="blocking">
  <status_check target="PLUGINS.md" required="true">
    <allowed_state status="📦 Installed">
      Plugin binaries exist in system folders, safe to uninstall
    </allowed_state>

    <blocked_state status="🚧 *">
      BLOCK with message:
      "[PluginName] is currently in development.

      Complete or pause development first:
      - Continue development: /continue [PluginName]
      - Complete workflow: Finish all stages

      Cannot uninstall incomplete plugin."
    </blocked_state>

    <blocked_state status="💡 Ideated">
      BLOCK with message:
      "[PluginName] hasn't been built yet - nothing to uninstall.
      Status: 💡 Ideated (no binaries exist)"
    </blocked_state>

    <blocked_state status="✅ Working">
      BLOCK with message:
      "[PluginName] is not installed to system folders.
      Status: ✅ Working (binaries exist in build/ only)

      To install first: /install-plugin [PluginName]"
    </blocked_state>
  </status_check>
</preconditions>

## Behavior

Invoke plugin-lifecycle skill with mode="uninstall"

<workflow_sequence enforce_order="true">
  <step order="1">
    Locate plugin files (extract PRODUCT_NAME from CMakeLists.txt)
  </step>

  <step order="2" requires_user_input="true">
    Confirm removal with user (show file sizes, preserve source code)
  </step>

  <step order="3" required="true">
    Remove from system folders (VST3 + AU)
  </step>

  <step order="4" required="true">
    Clear DAW caches (Ableton + Logic)
  </step>

  <step order="5" required="true">
    <state_transition required="true">
      <validation>
        Verify current status is 📦 Installed before proceeding
      </validation>

      <update target="PLUGINS.md" field="status">
        FROM: 📦 Installed
        TO: ✅ Working
      </update>

      <on_failure action="rollback">
        If state update fails:
        1. Report error to user
        2. DO NOT remove binaries (leave system in consistent state)
        3. Present recovery options
      </on_failure>

      <verification>
        After state update:
        1. Verify PLUGINS.md contains "Status: ✅ Working"
        2. Verify "Last Updated" field reflects current date
        3. Confirm timeline entry appended
      </verification>
    </state_transition>
  </step>

  <step order="6" required="true" blocking="true">
    Present decision menu for next steps
  </step>
</workflow_sequence>

<skill_reference>
  See `.claude/skills/plugin-lifecycle/references/uninstallation-process.md` for complete workflow.
</skill_reference>

## Success Output

Display completion confirmation with:
- Plugin name and status (uninstalled)
- Removed binary paths (VST3 + AU)
- Cache clearing confirmation
- Source code preservation note
- Reinstall command hint

See `.claude/skills/plugin-lifecycle/references/uninstallation-process.md` for complete output format.

## Routes To

`plugin-lifecycle` skill (uninstallation mode)
