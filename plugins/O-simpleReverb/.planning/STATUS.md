---
plugin: O-simpleReverb
stage: improve
stage_name: improve-milestone
status: family_retrofit_verify_automated_pass_daw_gate_pending
activeMilestone: family-retrofit
current_phase: verify
next_phase: post-milestone
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

Milestone **family-retrofit** (v2.0.0 → v3.0.0): **verify, automated PASS** (`improvements/family-retrofit/VERIFICATION.md`). Every gate was re-run on a fresh build: render-check 139/0, mutants 32/32, null BIT-EXACT (290 buffers, installed VST3 vs a rebuilt v2.0.0), state S0–S5, pluginval 10 for VST3 (with GUI) and AU, auval, the i18n/UI gates and the 4 probes.

Fixed in verify: a v2 user preset named like a lesson ("Init") is renamed to "Init (user)" instead of being hidden.

Open: the DAW hands-on (Logic + Ableton). After that comes the post-milestone bump to 3.0.0 (no tag).

## Milestones

| Milestone | Version | Status |
|---|---|---|
| reverb-engine-rewrite | 2.0.0 | ✓ shipped 2026-10-01 |
| family-retrofit | 3.0.0 | verify automated ✓ · DAW gate pending |

## Build Artifacts

```bash
./scripts/build-and-install.sh O-simpleReverb
```

- VST3: `~/Library/Audio/Plug-Ins/VST3/O-simpleReverb-dev.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-simpleReverb-dev.component`
- render-check: `build/plugins/O-simpleReverb/O-simpleReverb-render-check_artefacts/Release/O-simpleReverb-render-check` (`--mutants`, `--write-frames`)

---
*Last updated: 2026-10-15*
