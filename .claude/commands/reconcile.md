---
name: reconcile
description: Reconcile workflow state files to ensure checkpoints are updated
argument-hint: "[PluginName?] (optional)"
---

# /reconcile

When user runs `/reconcile [PluginName?]`, invoke the workflow-reconciliation skill.

## Routing Logic

<routing_decision>
  <check condition="plugin_name_provided">
    IF $ARGUMENTS contains plugin name:
      Invoke workflow-reconciliation skill with plugin_name parameter
    ELSE:
      Invoke workflow-reconciliation skill in context-detection mode
      (skill will analyze current workflow context)
  </check>
</routing_decision>

## What This Does

Reconciliation ensures workflow state files are synchronized:
- `.continue-here.md` - Resume point with stage/phase/status
- `PLUGINS.md` - Plugin registry with status emoji
- Git status - Staged/unstaged/uncommitted changes
- Contract files - Required artifacts per workflow stage

The workflow-reconciliation skill:
1. Detects current workflow context
2. Applies workflow-specific reconciliation rules
3. Performs gap analysis (filesystem vs expected state)
4. Presents reconciliation report with proposed actions
5. Executes user's chosen reconciliation strategy

## Preconditions

None - reconciliation can be invoked at any point to check/fix state.
