# O-Octagon Changelog

## v1.1.0 (2026-08-20)

### Added — speaker→output assignment (the in-space rig fix)

**Root cause this addresses:** the plugin publishes a 7.1 layout and writes channel ROLES; the
host decides which physical output each role reaches. Under CoreAudio (Logic, Standalone) the
measured device order is `Emagic_Default_7_1` — `L R Lrs Rrs C Lfe Lss Rss` — so with the factory
role-order labels, speakers 3–8 land on physical outputs 5, 6, 7, 8, 3, 4 (the exact permutation
reported from in-space testing on an 8-channel interface, and the one measured at Stage 4 Gate
16). That is correct role routing, not a defect; what was missing was a first-class way to say
"speaker n is WIRED to output n."

- **Double-click a speaker glyph on the Room plan** → a popover assigns that speaker's physical
  output (1–8). Swap semantics: the previous holder of the chosen output takes the vacated one,
  so the label set stays a permutation by construction and the venue guard can never see a
  duplicate from this path.
- **Output badges on the plan**: a glyph whose label reaches a different physical output than its
  own number shows `→k` beside it (`→?` for a label outside the 7.1 set). A stock rig shows
  nothing new; a remapped rig is legible at a glance.
- **Venue rail, "Output order" group**: `Direct 1–8` writes the whole device-order label set in
  one click — the single-click fix for a rig wired 1..8 in a CoreAudio host — and `Roles`
  restores the factory surround-role labels.
- Mechanism: all three are LABEL edits through the existing validated path
  (`applyVenueEditChecked` → `buildSpeakerToBuffer` → `getChannelIndexForType`). No DSP change,
  no parameter change, no buffer index anywhere; assignments persist with the venue, `.venue`
  files, and presets, exactly as labels always have.
- New native functions `assignSpeakerOutput` and `applyOutputOrderPreset` (bridge surface
  18 → 20, closed three ways by `ui_frontend_check.js` §3); per-speaker `output` rides the
  existing `getVenueGeometry` payload. The device-order table lives in `Source/Data/OutputOrder.h`
  and in C++ only — the page renders numbers it is handed (D19).

**Caveat, stated plainly:** the output numbering assumes the measured CoreAudio 7.1 device order.
A non-CoreAudio host may map roles differently; the verify ping remains the 60-second ground
truth in any host, and the popover says so.

**Testing:** `ui_frontend_check.js` — all 42 sections pass, including the widened §3 closure.
Default behavior unchanged: with factory labels the map, the solve, and the meters are untouched.

## v1.0.0 (unreleased — Stage 4 phase 4.2 Block C complete, 2026-08-14)

### Host validation (Logic Pro 12.3, BlackHole 64ch — phase 4.2)

- **Logic's canonical interleaved 7.1 bounce order measured: `1,2,3,4,7,8,5,6`** — i.e. a bounce
  file carries `L R C Lfe Lrs Rrs Lss Rss` on channels 1–8, the canonical WAVEFORMATEXTENSIBLE
  channel-mask order (`FL FR FC LFE BL BR SL SR`). Measured at 158.3 dB minimum isolation and
  confirmed by a permuted-venue bounce returning its before-the-bounce prediction exactly. This is
  **not** the realtime device order (`Emagic_Default_7_1`: `L R Ls Rs C LFE Lc Rc`) — the bounce
  and device paths order channels differently, and both are now measured.
- **The speaker→buffer map is consulted, not decorative:** an 8-cycle label permutation loaded into
  all eight instances shifted the whole bounce order by exactly the predicted derangement.
- **LFE slot confirmed an ordinary speaker on the bounce path** — byte-identical output against a
  reference speaker fed the same 31 Hz–16 kHz multitone; no bass management, no filtering. The
  positive control (air filter engaged at a hull excursion) matches the TPT filter model to
  0.00/0.03 dB at two operating points with the cutoff derived from venue geometry, not fitted.
- **The hull-crossing audible clause is concluded** (the last open clause): sample-resolution
  null between a hull-crossing gesture and a static render on commercial program material shows
  no discontinuity; operator listen passed. Monitoring path recorded (MacBook Pro speakers).
- **Instantiation constraint documented:** in Logic, insert via the slot's **Stereo → 7.1**
  channel-config entry. Clicking the plugin *name* yields Logic's default **multi-mono** pick —
  eight independent mono instances with both banners correctly raised.
- Session recall (save → quit → reopen), all 11 automation lanes (write + read-back), `auval`,
  and `User/`-preset non-pollution all verified in-host; full gate-by-gate record in
  `.planning/stages/4-polish/evidence/session-gates-4.2.txt`.

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

- **Host testing: DONE as of 2026-08-14** (see the Block C entry above) — with two named residuals:
  the realtime-loopback LFE *delta* was not measured (reference channels lost to a monitoring
  feedback loop; the LFE device channel itself captured clean at the constructed level), and the
  audible-clause listen has not been repeated on revealing monitoring.
- **No hall.** Nothing has been heard on a real 8-speaker array; the physical-interface half of
  COMPAT-02/2 carries owner: none.
- **Windows UI correctness.** CI proves the code compiles under MSVC and that pluginval 10 opens the
  editor without a timeout. No human has seen the UI on Windows.
- **RT-safety beyond allocation.** Allocation is measured by replacing the global `operator new`
  family; locks and file I/O in `processBlock` remain grep plus inspection.
  `-fsanitize=realtime` is unsupported by Apple clang 17.0.0.
