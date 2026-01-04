---
name: reset-to-ideation
description: Roll back plugin to ideation stage - keep idea/mockups, remove all implementation
argument-hint: "[PluginName]"
---

# /reset-to-ideation - Surgical Rollback

**Purpose:** Direct shortcut to reset plugin to ideation stage. Invokes plugin-lifecycle skill with mode: 'reset'.

**Use case:** Implementation went wrong, but the concept and UI design are solid. Start fresh from Stage 0.

## Behavior

<preconditions enforcement="blocking">
  <check target="PLUGINS.md" condition="plugin_exists">
    Plugin entry MUST exist in PLUGINS.md

    <on_failure action="BLOCK">
      Display error: "[PluginName] not found in PLUGINS.md.

      Run /dream to create a new plugin idea first."
    </on_failure>
  </check>

  <check target="status" condition="has_implementation">
    Status MUST NOT be 💡 Ideated (nothing to reset)

    <valid_states>
      ✅ Working, 📦 Installed, 🚧 Stage N (N ≥ 2)
    </valid_states>

    <on_failure action="BLOCK">
      Display error: "[PluginName] is already at ideation stage (💡 Ideated).

      Nothing to reset - run /implement [PluginName] to start implementation."
    </on_failure>
  </check>

  <check target="ideation_artifacts" condition="preservable_content_exists" severity="warning">
    At least one ideation artifact MUST exist to preserve

    <required_files>
      - .ideas/creative-brief.md OR
      - .ideas/mockups/*.html OR
      - .ideas/parameter-spec.md
    </required_files>

    <on_failure action="WARN">
      Display warning: "⚠️  No ideation artifacts found to preserve.

      This will delete the plugin entirely. Consider /destroy instead."
    </on_failure>
  </check>
</preconditions>

<routing_logic>
  <trigger>User invokes /reset-to-ideation [PluginName]</trigger>

  <sequence>
    <step order="1">Verify preconditions (block if failed)</step>
    <step order="2" required="true">
      Invoke plugin-lifecycle skill via Skill tool:

      Parameters:
      - plugin_name: $ARGUMENTS
      - mode: 'reset'
    </step>
  </sequence>

  <skill_reference>
    Implementation: .claude/skills/plugin-lifecycle/references/mode-3-reset.md
  </skill_reference>
</routing_logic>

## What Gets Preserved/Removed

<preservation_contract>
  <preserved>
    - Creative brief (.ideas/creative-brief.md)
    - UI mockups (.ideas/mockups/)
    - Parameter specifications (.ideas/parameter-spec.md)
  </preserved>

  <removed>
    - Source code (Source/ directory)
    - Build configuration (CMakeLists.txt)
    - Implementation docs (.ideas/architecture.md, .ideas/plan.md)
    - Build artifacts and installed binaries
  </removed>

  <state_change>
    Status: [Any] → 💡 Ideated
  </state_change>
</preservation_contract>

## Implementation

See `.claude/skills/plugin-lifecycle/references/mode-3-reset.md` for complete rollback workflow.

## User Confirmation

The plugin-lifecycle skill will present interactive confirmation showing:
- Files to be removed (Source/, CMakeLists.txt, implementation docs, binaries)
- Files to be preserved (creative-brief, mockups, parameter-spec)
- Backup location (backups/rollbacks/)
- Status transition ([Current] → 💡 Ideated)

See confirmation example in `.claude/skills/plugin-lifecycle/assets/reset-confirmation-example.txt`

## Success Output

On successful reset, plugin-lifecycle skill displays:
- Confirmation of removed files and directories
- Confirmation of preserved ideation artifacts
- Backup file path for recovery
- Status transition confirmation
- Suggested next steps

See success example in `.claude/skills/plugin-lifecycle/assets/reset-success-example.txt`

## Routes To

`plugin-lifecycle` skill (mode: 'reset')

## Related Commands

<related_commands>
  <decision_matrix>
    <scenario condition="want_to_start_over_but_keep_idea">
      Use: /reset-to-ideation (THIS COMMAND)
      Effect: Keep creative brief/mockups, delete implementation
    </scenario>

    <scenario condition="want_to_remove_binaries_only">
      Use: /uninstall
      Effect: Remove installed VST3/AU, keep source code
    </scenario>

    <scenario condition="want_to_delete_everything">
      Use: /destroy
      Effect: Remove plugin entirely (with backup)
    </scenario>

    <scenario condition="unsure_which_cleanup">
      Use: /clean
      Effect: Interactive menu to choose operation
    </scenario>
  </decision_matrix>
</related_commands>
