---
plugin: O-simpleReverb
stage: improve
stage_name: complete
status: family_retrofit_complete
version: 3.0.1
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

**v3.0.0 complete** (milestone **family-retrofit**, 2026-10-15). Automated verify PASS (`improvements/family-retrofit/VERIFICATION.md`) and the DAW hands-on in Logic + Ableton passed. Installed locally as `-dev`; not tagged (`/publish` is a separate decision).

Open, carried forward (low): editor scale overwritten by the display cap (family-wide, same in O-simpleAmbisonics), unlocked `currentPresetName` read (pre-existing), second "Load file…" leaves the first promise pending.

## Milestones

| Milestone | Version | Status |
|---|---|---|
| reverb-engine-rewrite | 2.0.0 | ✓ shipped 2026-10-01 |
| family-retrofit | 3.0.0 | ✓ complete 2026-10-15 |

## Build Artifacts

```bash
./scripts/build-and-install.sh O-simpleReverb
```

- VST3: `~/Library/Audio/Plug-Ins/VST3/O-simpleReverb-dev.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-simpleReverb-dev.component`
- render-check: `build/plugins/O-simpleReverb/O-simpleReverb-render-check_artefacts/Release/O-simpleReverb-render-check` (`--mutants`, `--write-frames`)

---
*Last updated: 2026-10-15*
