---
name: plan
description: Interactive research and planning for plugin (Stage 0)
---

# /plan

When user runs `/plan [PluginName?]`, invoke the plugin-planning skill to handle Stage 0 (Research & Planning - consolidated).

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <step order="1">Verify plugin entry exists in PLUGINS.md</step>
    <step order="2">Extract current status emoji</step>
    <step order="3">
      Evaluate status against allowed/blocked states:

      <allowed_state status="💡 Ideated">
        OK to proceed - start fresh planning (Stage 0)
      </allowed_state>

      <allowed_state status="🚧 Stage 0">
        OK to proceed - resume research phase
      </allowed_state>

      <blocked_state status="🚧 Stage N" condition="N >= 2" action="BLOCK_WITH_MESSAGE">
        [PluginName] is already in implementation (Stage [N]).

        Stage 0 (planning) is complete. Use:
        - /continue [PluginName] - Resume from current stage
        - /implement [PluginName] - Review implementation workflow
      </blocked_state>
    </step>
  </status_verification>

  <contract_verification blocking="true">
    <required_contract path="plugins/[PluginName]/.ideas/creative-brief.md" created_by="ideation">
      Creative brief defines plugin vision and serves as input to Stage 0 (Research)
    </required_contract>

    <on_missing action="BLOCK_AND_OFFER_SOLUTIONS">
      Display error message:
      "✗ No creative brief found for [PluginName]

      Planning requires a creative brief to define plugin vision.

      Would you like to:
      1. Create one now (/dream [PluginName])
      2. Skip planning (not recommended - leads to implementation drift)"

      WAIT for user response. Do NOT invoke plugin-planning skill until contract exists.
    </on_missing>
  </contract_verification>
</preconditions>

<behavior>
  <routing_logic>
    <condition check="argument_provided">
      IF $ARGUMENTS provided:
        1. Verify preconditions (block if failed)
        2. Invoke plugin-planning skill with plugin name
      ELSE:
        1. List plugins with status: 💡 Ideated, 🚧 Stage 0
        2. Present numbered menu of eligible plugins
        3. Offer to create new plugin via /dream
    </condition>
  </routing_logic>
</behavior>

<delegation>
  Invoke plugin-planning skill to execute Stage 0 (Research & Planning - consolidated).

  Skill produces:
  - architecture.md (DSP specification)
  - plan.md (implementation strategy with complexity score)

  Both outputs are created in a single pass by research-planning-agent.
</delegation>

<contract_enforcement>
  Stage 0 requires:
  - creative-brief.md (checked in preconditions above)
  - parameter-spec.md OR parameter-spec-draft.md (for complexity calculation)

  If missing, plugin-planning skill will BLOCK with unblock instructions.

  Note: Command verifies entry preconditions only. Skill handles stage-specific contract validation.
</contract_enforcement>

## Handoff to Implementation

After Stage 0 completes, the skill creates handoff state:
- .continue-here.md updated with "ready_for_implementation: true"
- User runs `/implement [PluginName]` to begin Stage 1 (Foundation + Shell)

## Workflow Integration

Complete plugin development flow:
1. `/dream [PluginName]` - Create creative brief + UI mockup
2. `/plan [PluginName]` - Research and planning (Stage 0 - consolidated)
3. `/implement [PluginName]` - Build plugin (Stages 1-3)

## Output

By completion of Stage 0, you have:
- ✅ architecture.md (DSP specification)
- ✅ plan.md (implementation strategy with complexity score)
- ✅ Updated PLUGINS.md status
- ✅ Git commit for consolidated stage
- ✅ Ready for implementation handoff
