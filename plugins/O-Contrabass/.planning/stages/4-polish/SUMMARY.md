# Stage 4: Polish — Execution Summary

**Date:** 2026-07-15
**Plugin:** O-Contrabass · **Ships:** v1.0.0 (final stage)
**Gate:** 3-gui → 4-polish **PASSED** (build/dsp-critic/ui-critic PASSED; schema/pluginval SKIPPED — no in-tree artifacts, not defects).
**Cross-cutting invariant HELD:** DSP FROZEN — **19/19 render goldens byte-identical** after all edits.

---

## What was executed (10 tasks, 7 groups)

### Group A — Factory presets (FUNC-04) ✅
- Authored **10 `FactoryPresetDef`s** in `Source/PluginProcessor.cpp` constructor (after the
  `setCustomStateCallbacks` block): 5 Orchestral (Cinematic Bass Sustain [default/first
  alphabetically], Section, Solo Arco, Pianissimo, Forte) + 5 Drone (Infinite, Just-Intoned,
  Scordatura, Sub, Dark Pad). Engineering units → single `convertTo0to1` loop → `initializeFactoryPresets`.
- **Skew-safe verified:** seeded `Forte Bass.json` stores BOW_PRESSURE `0.629465` = computed
  `((3.2−0.05)/(8.0−0.05))^0.5`; round-trips to 3.2. BOW_SPEED/BRIGHTNESS/VIBRATO_ONSET likewise exact.
- `STRING_TENSION` omitted from every preset → stays inert at default 0.5 (v1.1). Drone presets
  carry explicit `TUNING_SYSTEM=2`(12-TET)/`NOTE_EXPRESSION=1`; per-string pitch via `DETUNE_*`.
- Just-Intoned Drone: `DETUNE_A=+204 / D=−14 / G=+182`. Scordatura: `E=−400 / A=−200 / G=+200` (C–G–D–A fifths).
- **Re-seed hazard handled** (Q1.5): version pinned 1.0.0 → only re-seeds when `Factory/` is empty.
  `rm -rf ~/Library/O-Contrabass/Presets/Factory/` before each re-tweak install. All 10 seed on first instantiation.

### Group B — Dorico `.doricolib` bundle (COMPAT-02) ✅ *(dorico-agent)*
- Created `Resources/dorico/`: `playbacktemplatedeps.doricolib`, `endpointconfig.xml`,
  `playbacktemplatespec.xml`, `INSTALL-DORICO.md`, `SMOKE-TEST.md` + CMake `install(DIRECTORY …)` rule.
- **P0 load-bearing microtonal pair present** (`.doricolib:36-37`): `<pitchBendRange>2</pitchBendRange>`
  + `<microtonalPlaybackMethod>kVST3NoteExpression`. Single `pt.natural` technique, `kNoteVelocity` dynamics.
- **ID chain byte-consistent:** `<pluginID>ABCDEF019182FAEB4F7544764F436273` (verified Audio Module CID),
  `<pluginName>O-Contrabass-dev`, configID `endpointconfig.user.o_contrabass`, entityID/expressionMapID
  `xmap.ouaricon.o_contrabass`. XML well-formed; the one comment sits **inside** `<kScoreLibrary>` (safe).
- Ships dev-branded; release-GUID swap documented in INSTALL-DORICO.md.

### Group C — Windows CI + pluginval-10 (COMPAT-01) ✅ authored *(needs push+dispatch to actually run)*
- `.github/workflows/build-and-release.yml`: added `workflow_dispatch` (inputs `plugin_name` /
  `version` / `validate_only`); `parse-tag` now handles both tag-push and dispatch and emits `validate_only`.
- On `validate_only`: `build-macos` + `create-release` skipped; Windows Configure builds **only the target
  plugin** (dynamic `SKIP_PLUGINS` = all siblings) to de-risk the never-Windows-run sibling configure.
- Added a **pluginval strictness-10 step** to `build-windows` (pinned Tracktion v1.0.3, `--timeout-ms 600000`
  for cold WebView2, log uploaded as artifact). YAML validated.
- **Not yet run** — the dispatch runs against a pushed ref and would be outward-facing; left for the user
  to trigger (see below). No public Release is published on this path.

### Group D — PERF-02 benchmark ✅ PASS
- Isolated `--perf` mode in `tests/render-harness/main.cpp` (`--sample-rate` / `--block-size`, RTF + CPU%/voice),
  returns before any golden mode + writes no WAV → cannot perturb the golden invariant.
- **Measured (Release, defaults, 1 voice, 256-block): 0.587% @44.1 kHz (170× RT), 0.652% @48 kHz (153× RT)** —
  far under the 5% budget. First house RTF/CPU% number; method is reusable.

### Group E — Subjective audition rig (FUNC-03 + DSP-10 + FUNC-04 QA) ✅ authored *(human gate)*
- `stages/4-polish/AUDITION.md` — R38 probe table: 4 core probes (orchestral A/B, slow-attack, drone A/B,
  preset-switch) + 8 preset-QA rows + skew round-trip spot-check + sign-off transcription block.

### Group F — Version / docs housekeeping ✅
- **CHANGELOG.md:** collapsed `[1.1.0-dev]` → `[1.0.0]` (2026-07-15); added a Stage-4 "Added" block.
- **parameter-spec.md:** fixed stale "Monophonic" → `kNumVoices = 4`; added a skew-factors caveat.
- **NOTES.md:** added the "v1.1 Deferrals (frozen for v1.0.0)" section (STRING_TENSION, `.tun`, DSP-07/08/09,
  FUNC-07 MTS-ESP, Dorico CC11 dynamics).
- **modules/registry.yaml:** preset-manager `1.0.2 → 1.0.4` (cosmetic R5; module.yaml authoritative).

### Group G — Automated validation battery ✅ (local portion GREEN)
| Check | Result |
|-------|--------|
| 19/19 render goldens byte-identical | ✅ PASS (frozen-DSP invariant) |
| auval `aumu OCbs OuDv` | ✅ SUCCEEDED |
| pluginval strictness-10 (macOS, warm) | ✅ SUCCESS |
| ui_frontend_check.js | ✅ 14/14 |
| bridge gate (JS vs C++ native fns) | ✅ 32 = 32 |
| PERF-02 <5% CPU/voice | ✅ 0.587% / 0.652% |
| Dorico bundle structural checks | ✅ load-bearing pair + ID chain + well-formed |

---

## Files changed
- `Source/PluginProcessor.cpp` (constructor: 10 factory presets + skew-safe seed)
- `Resources/dorico/` (5 new files) + `CMakeLists.txt` (install rule)
- `.github/workflows/build-and-release.yml` (workflow_dispatch validate-only + Windows pluginval-10)
- `tests/render-harness/main.cpp` (`--perf` mode)
- `.planning/stages/4-polish/AUDITION.md` (new)
- `CHANGELOG.md`, `.planning/parameter-spec.md`, `NOTES.md`, `modules/registry.yaml`

Build/install: `O-Contrabass-dev` VST3 + AU rebuilt & installed (14 s, verified).

---

## Remaining for VERIFY (human-in-the-loop — cannot be auto-completed)

1. **Windows pluginval-10 via CI (COMPAT-01 gate):** commit + push the workflow changes, then
   `gh workflow run build-and-release.yml -f plugin_name=O-Contrabass -f validate_only=true`. Confirm the
   Windows build + pluginval-10 pass and the WebView UI is not blank (first-ever real-Windows visual test).
   **No Release is published.** *(Left un-pushed: outward-facing + user holds release.)*
2. **Dorico TC-4 (COMPAT-02 P0):** run `Resources/dorico/SMOKE-TEST.md` — 24-EDO quarter-sharp plays at
   correct microtonal pitch (the only check that catches a dropped top-level microtonal field).
3. **Subjective sign-off (FUNC-03 / DSP-10 / FUNC-04):** run `AUDITION.md` in Logic; record CONFIRM/REVISE.
4. **5 Logic manual checks** carried from Stage 3 verify (editor open/close ×10, 31-param interaction,
   picker UAF, Logic smoke, visual QA @1000×650) + a Logic CPU-meter spot-read to corroborate PERF-02.

## Notes / flags
- Two stale differently-named AU variants remain installed (`O-Contrabass-pre-2-5-dev.component`,
  `O-Contrabass-pre-port.component`) — different AU subtypes, so **non-shadowing**; optional cleanup.
- Nothing committed/pushed. `.tun` parser, STRING_TENSION wiring, DSP-07/08/09 depth, MTS-ESP, and the
  Dorico CC11 dynamics path remain deferred to v1.1 (per Do-NOT-touch list).
