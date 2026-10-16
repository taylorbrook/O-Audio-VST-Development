---
plugin: O-simpleReverb
stage: improve
stage_name: improve-milestone
status: family_retrofit_execute_complete_verify_next
activeMilestone: family-retrofit
current_phase: execute
next_phase: verify
next_action: improve-milestone
version: 2.0.0
target_version: 3.0.0
last_updated: 2026-10-15
complexity_score: 4.2
parameter_count: 8
plugin_code: OuSr
workflow_mode: manual
mockup_finalized: true
mockup_signed_off: 2026-10-15
---

# O-simpleReverb Status

## Current Position

Milestone **family-retrofit** (v2.0.0 → v3.0.0) — **execute ✓ 2026-10-15** (T0–T10) · next: verify (`/improve-milestone O-simpleReverb`).

Execute evidence (`improvements/family-retrofit/SUMMARY.md`): installed `-dev` VST3 + AU from `build-and-install.sh` (0 warnings, old-case bundles swept); auval SUCCEEDED; pluginval strictness 10 SUCCESS on VST3 (with the editor tests) and AU; render-check 137 PASS / 0 FAIL, 32 / 32 mutants; verify_3_null BIT-EXACT on the installed VST3 (290 buffers vs v2.0.0); verify_3_state S0–S5 ALL PASS on the installed VST3; check-i18n, check-ui-labels (38 states), fr / zh lint, boot-all-uis (0 dead tips) green; frame-probe 36/36, header-fit, lesson-row-budget, tooltip-clamp ALL PASS at 0.6 / 1.0 / 1.5 × en / fr / zh-Hans; negative controls freeze / slope / chipbody FAIL as designed.

Open for verify: DAW hands-on (Logic + Ableton: rescan shows "O-simpleReverb", a v2.0.0 session reconnects, wells move live, resize persists).

Version stays **2.0.0** in CMake until the post-verify bump; no tag (tags only via `/publish`).

## Milestones

| Milestone | Version | Status |
|---|---|---|
| reverb-engine-rewrite | 2.0.0 | ✓ shipped 2026-10-01 |
| family-retrofit | 3.0.0 | execute ✓ · verify next |

## Build Artifacts

```bash
./scripts/build-and-install.sh O-simpleReverb
```

- VST3: `~/Library/Audio/Plug-Ins/VST3/O-simpleReverb-dev.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-simpleReverb-dev.component`
- render-check: `build/plugins/O-simpleReverb/O-simpleReverb-render-check_artefacts/Release/O-simpleReverb-render-check` (`--mutants`, `--write-frames`)

---
*Last updated: 2026-10-15*
