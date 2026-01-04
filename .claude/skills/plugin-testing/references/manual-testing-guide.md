# Manual DAW Testing Guide

**Purpose:** Final validation before release. Automated tests can't catch everything (sonic quality, workflow, edge cases).

## When to Use

- Final validation before release
- Testing sonic quality and character
- Workflow validation (automation, presets, project save/load)
- Cross-DAW compatibility checks
- User experience assessment

## Testing Methodology

Manual testing uses a comprehensive checklist (see `assets/manual-testing-checklist.md`) that covers:

1. **Setup** - Load plugin in DAW, verify UI
2. **Parameter Testing** - Test each parameter with real audio
3. **Automation** - Record and playback parameter automation
4. **Preset Recall** - Save and load presets
5. **Project Save/Load** - Verify state persistence across DAW sessions
6. **Sample Rate Changes** - Test at different sample rates (44.1k, 48k, 96k)
7. **Buffer Size Changes** - Test at different buffer sizes (64, 256, 1024 samples)
8. **Channel Configuration** - Test mono, stereo, multichannel
9. **Edge Cases** - Silence, DC offset, clipped input, rapid parameter changes
10. **Sonic Quality** - Verify no artifacts, proper gain staging, expected behavior
11. **Stress Testing** - Long-term stability, rapid load/unload, extreme settings
12. **Cross-DAW Testing** - Test in multiple DAWs (Logic, Ableton, Reaper, etc.)

## Generating Customized Checklists

The checklist template should be customized for each plugin based on its `parameter-spec.md`:

**Steps:**

1. Read `assets/manual-testing-checklist.md` template
2. Load `plugins/[PluginName]/.ideas/parameter-spec.md`
3. Extract parameter list
4. Generate parameter-specific test items
5. Insert into checklist template

**Example parameter-specific test:**

```markdown
### Specific parameters to test:

- [ ] **threshold** (-60.0 to 0.0 dB): Test at extremes, verify compression behavior
- [ ] **ratio** (1.0 to 20.0): Test 1:1 (no compression) vs 20:1 (limiting)
- [ ] **attack** (0.1 to 100.0 ms): Fast attack should catch transients, slow should let them through
- [ ] **release** (10.0 to 1000.0 ms): Fast release should pump, slow should be smooth
- [ ] **gain** (0.0 to 24.0 dB): Verify makeup gain compensates for gain reduction
- [ ] **bypass** (on/off): Toggle while audio playing, verify no clicks or pops
```

## After Testing

User marks checklist items as they test. When complete, offer next steps:

```
✓ Manual DAW testing complete

What's next?
1. Install plugin (/install-plugin) → Ready for production use
2. Mark plugin as release-ready (update PLUGINS.md status)
3. Report issues found (if any)
4. Run additional automated tests
5. Other

Choose (1-5): _
```

## Notes Section

The checklist includes a free-form notes section for users to document:

- Issues discovered
- Unexpected behavior
- Sonic observations
- Performance concerns
- Ideas for improvements
