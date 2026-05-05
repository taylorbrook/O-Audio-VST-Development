---
name: dorico
description: Dorico integration helper — microtonal playback, expression maps, playback templates, keyswitch routing, CC/PC technique triggers
argument-hint: <PluginName> [question-or-task]
---

# /dorico

Spawn the `dorico-agent` subagent to diagnose and (when authorized) fix Dorico-side integration issues for an Ouaricon VST instrument.

<preconditions enforcement="blocking">
  <check target="cwd" condition="project_root" required="true">
    Must be invoked from the VST-development project root.
  </check>
  <check target="plugins/<PluginName>" condition="exists" required="true">
    `<PluginName>` MUST resolve to a directory under `plugins/`.
    If missing: report the error and the list of available plugins (use `ls plugins/`); do not spawn the agent.
  </check>
</preconditions>

<routing>
  <invoke agent="dorico-agent" with="$ARGUMENTS" required="true">
    Parse `$ARGUMENTS` into `<plugin>` (first whitespace-delimited token) and `<task>` (remainder).
    Spawn dorico-agent via the Task tool with `subagent_type: "dorico-agent"` and a prompt of the shape:

    ```
    Plugin: <plugin>
    Plugin Dorico bundle: plugins/<plugin>/Resources/dorico/  (use Glob to enumerate)
    Task: <task>

    Run your <entry_protocol> reads first, then proceed through the graduated workflow.
    Return your report in the <output_contract> shape.
    ```

    The agent reads its own memory file and the relevant Dorico reference artifacts on entry.
    Do NOT pre-load Dorico XML or memory contents into the spawn prompt — the agent does that itself.
  </invoke>
</routing>

<background_info>
## What dorico-agent covers

Full Dorico integration stack for the Ouaricon plugin suite:

- Microtonal / VST3-NoteExpression playback (top-level `<pitchBendRange>` + `<microtonalPlaybackMethod>`)
- Playback Templates and `.doricolib` distribution authoring
- EndpointConfigs (`Resources/dorico/EndpointConfigs/<Plugin>/endpointconfig.xml`)
- Expression maps — keyswitch / CC / Program Change technique triggers
- The 3-layer keyswitch routing stack (schema + plugin defaults + fresh-instance reset)
- Dynamics audit (CC1 / CC11 / Velocity routing)
- Plugin C++ side (Layer 2: `createParameterLayout()` defaults that gate KS/CC/PC)
- SMOKE-TEST.md TC-1..TC-5 walkthroughs

**Canonical reference:** `plugins/O-MicrotonalSampler/Resources/dorico/` (v1.16.x state) — 4-family expression-map bundle, EndpointConfigs, PlaybackTemplateSpec.

## Typical use cases

- Microtonal regression in Dorico (TC-4 fails but TC-1..TC-3 pass)
- Keyswitches not firing for a technique transition
- Authoring a Dorico distribution bundle for a new plugin
- Pre-release SMOKE-TEST.md walkthrough on a microtonal-cohort plugin
- Debugging instrument-family routing in a multi-family Playback Template
</background_info>

<examples>
## Examples

```bash
/dorico O-MicrotonalSampler microtonal pitch wrong in Dorico TC-4
/dorico O-Lyrica add Dorico distribution bundle (Playback Template + EndpointConfig)
/dorico O-Reed keyswitches not firing for Staccato → Legato transition
```
</examples>
</content>
</invoke>