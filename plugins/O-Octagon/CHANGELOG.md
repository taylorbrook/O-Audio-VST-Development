# O-Octagon Changelog

## v1.0.0 (unreleased — Stage 4 phase 4.1 complete, 2026-08-12)

Eight-channel DBAP spatializer for irregular concert arrays. First release.

### Added

- **DBAP panning across eight speakers**, solved from measured venue geometry rather than an assumed
  ring. DBAP and not VBAP because VBAP discards distance, which an irregular hang cannot afford.
- **Measured venue model** — 42 values (eight speaker positions with labels, plus the rake and
  bounding-box scalars), editable in the UI and saved to `.venue` files independently of presets.
- **Source position, width and blur** — a puck over the room plan, sub-point source widening, and a
  blur radius that scales with the rig rather than with metres, so a patch means the same thing in a
  different hall.
- **Outside-hull processing** — a dB/metre trim with a −24 dB floor, and an air filter whose cutoff
  falls with distance beyond the speaker hull.
- **Six named scenes plus four user slots** (FUNC-06), resolved against the measured geometry rather
  than fixed speaker indices, so they follow a re-hung rig.
- **Verify ping** — a per-speaker identification tone for checking the map at the desk.
- **Eight output meters** that follow the channel map, not the buffer order.
- **Six factory presets** — Dry Point, Concert Default, Chamber, Wide Hall, Distant Field,
  Enveloping. Room character only: a preset never moves the source or the scene.
- **SAFE mode** — instantiating on a mono or stereo output gives a defined, non-destructive fold with
  a banner in the UI rather than a refusal to load.
- **Per-commit CI** (`.github/workflows/ci-tests.yml`) building and running both C++ test targets on
  macOS, and building the VST3 under MSVC with pluginval strictness 10 on Windows.

### Technical Notes

- **95 offline probes** across two console targets — 45 geometry/unit, 50 render-harness. No unit-test
  framework: `juce_add_console_app` plus exit codes, matching the twelve existing harnesses in this
  repo.
- **The channel map is derived, never hardcoded.** `AudioChannelSet` is a bitset, so buffer order is
  enum-bit order and a hardcoded 0..7 map would silently scramble the speaker assignment while
  passing auval and pluginval. Layer 2 of the gate compares a golden generated from *parsed JUCE
  source* by a compile-time `static_assert`, so a JUCE release that reorders `ChannelType` fails the
  build rather than shipping a wrong map.
- **Three 8-channel containers are accepted** — 7.1, 7.1-SDDS and 5.1.2 — because those are the only
  8-channel formats Logic exposes. Anything else, including `octagonal()`, folds to SAFE mode and
  raises the banner. The predicate is written as the complement of the three real rigs
  (`Source/Data/RigPolicy.h`), which is what makes an unknown fourth container fold *safely* rather
  than silently pass as a rig nobody mapped.
- **Preset loads preserve the source and the scene.** The shared preset manager resets every
  parameter to its default before applying a preset (by design), so a room-character preset omitting
  the position keys would not leave them alone — it would re-centre the source and clear the scene.
  `oo::presets::loadPreserving` snapshots and restores those eleven parameters around the load, at
  O-Octagon's call site and never in the shared module.
- **Factory presets are initialized from the editor, not the processor**, which keeps all preset file
  I/O off the headless `auval`/pluginval scan path. Verified: six pluginval runs and an `auval` pass
  created no user preset directory at all.
- Factory preset values are authored in engineering units and converted through the live
  `NormalisableRange`, never as normalised literals.

### Validation

- pluginval strictness 10 — VST3 ×3 and AU ×3, all six exit 0.
- `auval -v aufx OuOc OuDv` — **AU VALIDATION SUCCEEDED**, with all six `AUChannelInfo` configs
  reported: `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]`.
- 95/95 probes, 0 failures, from a forced full recompile with zero compiler warnings.
- 69 JS UI-gate sections green.

### Not Yet Validated

- **No host testing.** Stage 4 phase 4.2 covers Logic on an 8-channel interface — bounce channel
  order, LFE gain handling, and the verify ping through eight physical outputs.
- **No hall.** Every Stage 4 criterion closes at the desk; nothing has been heard on a real array.
- **Windows UI correctness.** CI proves the code compiles under MSVC and that pluginval 10 opens the
  editor without a timeout. No human has seen the UI on Windows.
- **RT-safety beyond allocation.** Allocation is measured by replacing the global `operator new`
  family; locks and file I/O in `processBlock` remain grep plus inspection.
  `-fsanitize=realtime` is unsupported by Apple clang 17.0.0.
