# Dorico Agent Memory

## Learned Patterns
- General (RECURRING REGRESSION): Microtonal pitch falling back to nearest 12-TET = top-level <pitchBendRange>2</pitchBendRange> + <microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod> missing from each <ExpressionMapDefinition> in playbacktemplatedeps.doricolib. Per-combination duplicates DO NOT substitute. Fix: restore top-level fields, bump <version>, redeploy to ~/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/, full Cmd-Q + relaunch. (Validated O-MicrotonalSampler v1.16.6 on 2026-05-05.)
- General: Standalone .doricoexpmap drops into User/Expression Maps/ are SILENTLY skipped by Dorico. Distribute via Playback Template + .doricolib in DefaultLibraryAdditions/. (Phase 25 Plan 01 reverted at d2c86c5; finding doc at .planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md.)
- General: Dorico keyswitches not firing = check 3 layers in PARALLEL: (1) per-combo <exclusionGroup>1</exclusionGroup> in .doricolib; (2) plugin trigger gates not defaulting false (ks_enabled, technique_count, etc.); (3) FRESH plugin instance (saved project state shadows new defaults). HSO factory map is the KS reference, NOT NotePerformer (which uses kControlChange).
- O-Lyrica: validated spike/reference. auval DEF-24-01 (parameter-meta-flag annotation gap) is benign — NOT a runtime defect. Do not propose fixes unless explicitly requested.

## Common Issues
- Microtonal pitch wrong in Dorico but plugin tests fine: TC-4 of SMOKE-TEST.md (quarter-sharp at C4 with 24-EDO) is the only test that reveals top-level-fields regression. TC-1..TC-3 will all still pass.
- Dorico log shows "Error opening file: invalid file format": .doricolib XML is structurally invalid. Diff against last-known-good in git.
- Dorico Library Manager has no "Import Expression Map" command — only "Import Library" (.doricolib) and "Import Cubase Expression Map". This is by design.

## Last Updated
2026-05-05 (seeded from critical_dorico_*.md and v1.16.6 incident)
</content>
</invoke>
