---
plugin: O-MicrotonalSampler
stage: improve
phase: v1.16.2 PATCH SHIPPED — TC-5 Dorico KS-firing fix landed
status: dorico_distribution_complete_tc2_tc6_open_follow_up
last_updated: 2026-05-04
version: 1.16.2
previous_versions: 1.0.0, 1.0.1, 1.0.2, 1.0.4, 1.1.0, 1.2.0, 1.2.1, 1.2.2, 1.2.3, 1.2.4, 1.3.0, 1.4.0, 1.5.0, 1.5.1, 1.6.0, 1.7.0, 1.7.1, 1.8.0, 1.9.0, 1.9.1, 1.10.0, 1.11.0, 1.12.0, 1.12.1, 1.12.2, 1.12.3, 1.12.4, 1.13.0, 1.14.0, 1.15.0, 1.16.0, 1.16.1, 1.16.2
---

# Resume Point

## v1.16.2 Patch Shipped (2026-05-04)

**Status:** primary Dorico-distribution gap (TC-5 keyswitch-from-notation)
closed. v1.16.0 → v1.16.2 series is now functionally complete for the
notation→playback path it was designed for — Playback Template loads,
expression map binds, microtonal pitch routes via VST3 Note Expression,
and playing-technique markings fire the plugin's keyswitch.

### What landed

- **FIX (plugin source):** `ks_enabled` default flipped `false → true`.
  Fresh Dorico endpoints now boot with KS scanning armed, so exp-map
  keyswitches are honored without manual UI toggling. KS range stays
  0..9 (well below any real instrument) — no risk of accidental fire
  from normal MIDI.
- **FIX (plugin source):** `technique_count` default raised `1 → 8`.
  The KS handler clamps incoming KS notes to `[0, techCount-1]`; with
  count=1 every KS resolved to slot 0 regardless of the requested
  technique. Default=8 unlocks all plugin slots for exp-map use.
- **FIX (distribution artifact):** per-`<playingTechniqueCombination>`
  `<exclusionGroup>1</exclusionGroup>` added to the doricolib (all 10
  combos). Matches HSO factory shape; required for Dorico's mutual-
  exclusion logic to fire `<switchOnAction>` cleanly on technique
  transitions out of `Ord.`. `<version>` 4 → 7 to defeat Dorico's
  cache.

### Validation

- Manual Dorico 6 smoke (delete+re-add endpoint required for fresh
  defaults): TC-1, TC-3, TC-4, TC-5 all PASS.
- pluginval-5 PASS. auval pre-existing benign DEF-24-01 unchanged.

### Open follow-ups (carried, lower priority)

- **TC-2 (auto-load Playback Template):** still won't fire the
  endpoint without manual instantiation. Needs `<generatorSpec>`
  fallback in `playbacktemplatespec.xml`. Bonus item from the v1.16.x
  brief, deferred from v1.16.2 to keep scope tight.
- **TC-6 (CC11 dynamics through Dorico):** never tested end-to-end.
  Wire is in the exp-map; plugin handler validated against non-Dorico
  DAW. Worth a quick smoke test next session.
- **8-slot cap vs 10-technique exp-map:** plugin caps at 8 slots
  internally (`OMtsTrigger::kMaxTech = 8`); exp-map ships 10 KS notes
  (techniques 0–9). KS 8 (tremolo) and 9 (flautando) clamp to slot 7
  (martele). Either trim exp-map to 8 or raise `kMaxTech`. None of
  the user's primary techniques fall in 8–9.

## v1.16.1 Patch Shipped (2026-05-04)

**Status:** distribution-artifacts patch on top of v1.16.0. Two FIX-class
findings from a Dorico smoke test, plus a documented brief for the
remaining open follow-up (`improvements/dorico-keyswitch-fix.md`).

### What landed

- **FIX:** Dorico launch crash ("Error opening file: invalid file format").
  Three XML/`.doricolib` files in v1.16.0 had leading XML comments before
  the root element; Dorico's strict user-config parsers rejected them.
  Stripped comments from `endpointconfig.xml`,
  `playbacktemplatespec.xml`, and `playbacktemplatedeps.doricolib`.
  (Comment AFTER the root opening tag is fine; comment BEFORE root
  crashes launch.)
- **FIX:** Expression map not appearing in Track Inspector dropdown in
  v1.16.0. Discovered via spike: `playbacktemplatedeps.doricolib` inside
  `EndpointConfigs/<Name>/` is endpoint-scoped — its exp-map only
  registers when that endpoint is active. Without an active endpoint
  (the auto-load template fails — TC-2), the exp-map was invisible.
  v1.16.1 documents the `DefaultLibraryAdditions/` distribution path
  (Dorico auto-merges every `.doricolib` placed there into every
  project's library on launch) as the canonical mechanism for global
  exp-map registration.
- **DOC:** `Resources/dorico/INSTALL-DORICO.md` rewritten — three-folder
  layout (now four with `DefaultLibraryAdditions/`), macOS + Windows
  install, dev-vs-release CID caveat preserved, known-issue scope, and
  a troubleshooting section.
- **NEW:** `improvements/dorico-keyswitch-fix.md` — comprehensive
  diagnostic brief for the v1.16.x KS-firing patch. Documents what's
  been tried (and reverted), HSO factory reference, prioritized
  investigation paths, file map, test loop, and risk envelope.

### Smoke test outcomes (v1.16.0 → v1.16.1)

| TC | Title | Status (after v1.16.1) | Notes |
|---|---|---|---|
| TC-1 | Playback Template appears in dropdown | ✓ PASS | |
| TC-2 | Apply template auto-loads plugin slot | ✗ FAIL | `playbacktemplatespec.xml` `<entries>` need fallback `<generatorSpec>` (modelled on Ample China). Open follow-up. |
| TC-3 | Expression map appears in Track Inspector | ✓ PASS | Via `DefaultLibraryAdditions/` (NOT via auto-template). |
| TC-4 | Microtonal pitch (kVST3NoteExpression) | ✓ PASS | LOAD-BEARING. Quarter-sharp accidentals play at +50¢ correctly. |
| TC-5 | Technique-marking text fires keyswitch | ✗ FAIL | The whole subject of `improvements/dorico-keyswitch-fix.md`. |
| TC-6 | CC11 dynamics swell | not tested | Skipped to wrap session. |

### Open follow-up

- **v1.16.x patch — Dorico KS-from-notation routing.** See
  `improvements/dorico-keyswitch-fix.md` for full context. Slash command
  to spawn investigation in a fresh session:

  ```
  /improve O-MicrotonalSampler v1.16.x — Fix Dorico keyswitch-from-notation routing (TC-5 FAIL from v1.16.0 smoke test). Read plugins/O-MicrotonalSampler/improvements/dorico-keyswitch-fix.md FIRST for full diagnostic context, what's already been tried (and reverted), the working factory reference (HSO), and suggested investigation paths in priority order. Goal: typing "sul pont." text in Dorico fires the keyswitch and the plugin's WebView technique-tab strip switches accordingly. Microtonal pitch (kVST3NoteExpression) and exp-map registration via DefaultLibraryAdditions/ both work and MUST stay working. The plugin's C++ KS handler is validated from non-Dorico DAW testing — issue is most likely the exp-map XML or Dorico's MIDI routing, not plugin source.
  ```

- **TC-2 fix** can be folded into the same v1.16.x patch (a
  fallback `<generatorSpec>` entry in `playbacktemplatespec.xml`) — see
  brief, Path D.

### Build / validation gate

- No source-code changes. Build outputs identical to v1.16.0 except for
  the `<bundleVersion>` field bumped via CMake `VERSION` 1.16.0 → 1.16.1.
- Manual Dorico smoke test (TC-1, TC-3, TC-4) re-verified after redeploy.

### Resume command

Multi-Version Improvement Plan complete. Next improvement cycle starts
fresh — `/improve O-MicrotonalSampler [new description]`. Open follow-up
brief at `improvements/dorico-keyswitch-fix.md`.

---

## v1.16.0 Implementation Complete (2026-05-03)

**Status:** distribution-artifacts-only release. No source-code changes;
binary unchanged from v1.15.0 baseline (VST3 + AU + Standalone macOS).
Multi-Version Improvement Plan (v1.14.0 + v1.15.0 + v1.16.0) complete.

### What landed

- **`Resources/dorico/`** — three-folder distribution tree authored
  against the Dorico 6 user-library layout:
  - `EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml`
  - `EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`
    (10 keyswitch combinations — `pt.natural`, `pt.sulPonticello`,
    `pt.sulTasto`, `pt.nonVibrato`, `pt.muted`, `pt.pizzicato`,
    `pt.naturalHarmonic1`, `pt.martele`, `pt.tremolo`, `pt.flautando`
    on MIDI notes 0..9 vel 127; `microtonalPlaybackMethod
    = kVST3NoteExpression`; `volumeType = kCC` param1=11)
  - `PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml`
- **`Resources/dorico/INSTALL-DORICO.md`** — end-user install guide
  (macOS + Windows paths, dev-vs-release CID caveat).
- **`Resources/dorico/SMOKE-TEST.md`** — six-step manual verification
  procedure.
- **`CMakeLists.txt`** — `VERSION` 1.15.0 → 1.16.0.
- **`CHANGELOG.md`** — `[1.16.0] - 2026-05-03` entry.

### Spike findings (2026-05-03)

The earlier plan ("ship `O-MicrotonalSampler.doricolib` containing
exp-map + Playback Template + Endpoint Configuration") was structurally
incorrect. A spike against the user's installed Dorico 6 user library
revealed that EndpointConfigs and PlaybackTemplateSpecs are **separate
top-level folder hierarchies** (not entities within `.doricolib`):

```
~/Library/Application Support/Steinberg/Dorico 6/
├── EndpointConfigs/
│   └── <Name>/
│       ├── endpointconfig.xml
│       └── playbacktemplatedeps.doricolib
├── PlaybackTemplateSpecs/
│   └── <Name>/
│       └── playbacktemplatespec.xml
└── ...
```

`.doricolib` Library Manager imports register expression-map definitions
but NOT EndpointConfig or PlaybackTemplate — those entities live in the
folder structures above. v1.16.0 ships the validated 3-folder layout
that Dorico itself uses for user-saved templates (modelled on the
"Test State-less" and "Ample China" references in the user's library).

This unblocks the previously reverted Phase 25 Plan 01 distribution
mechanism (commit `d2c86c5` rollback in the parent `note-expression`
module). Per-plugin shipping pattern is now established and reusable
for the v1.5 microtonal cohort (O-Lyrica, O-Bells, O-IntonationPad,
O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) — each gets its own
`Resources/dorico/EndpointConfigs/<Name>/` + `PlaybackTemplateSpecs/<Name>/`
tree with its CID and exp-map.

### Schema validation

XML schemas confirmed against:
- `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Symphonic Orchestra/expressionMapsDefinitions.xml`
  — `<switchOnAction><type>kKeySwitch</type><param1>NOTE</param1><param2>127</param2></switchOnAction>`
- `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/Iconica Sketch/expressionMapsDefinitions.xml`
  — `<volumeType><type>kCC</type><param1>11</param1></volumeType>`
  (CC# is in `param1`, type literal is `kCC` not `kCC11`)
- `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Test State-less/`
  — slot-less EndpointConfig structure + bundled `playbacktemplatedeps.doricolib`

### Open follow-up (deferred to v1.16.x)

- **Release-branding artifact tree.** Current bundle hardcodes the
  dev-build VST3 CID (`ABCDEF019182FAEB4F7544764F4D7453`, manufacturer
  `OuDv`). Release builds (manufacturer `OuAu`, no `-dev` suffix)
  produce a different CID. Track as a v1.16.x patch series — author
  release-flavor `endpointconfig.xml` from a release-branded
  `moduleinfo.json` and ship under `Resources/dorico/release/` (or via
  CMake `configure_file` substitution at build time).
- **Generalize to v1.5 microtonal cohort.** This same per-plugin
  template pattern is now ready to ship for O-Lyrica, O-Bells,
  O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant.
  A future cross-plugin module (`module-create dorico-distribution`?)
  could extract the playbacktemplatedeps.doricolib body into a shared
  authoring template that each plugin parameterises with its CID +
  technique vocabulary.

### Build / validation gate

- No source-code changes. Build outputs identical to v1.15.0 baseline
  except for the `<bundleVersion>` field bumped via CMake `VERSION`.
- Smoke procedure (`Resources/dorico/SMOKE-TEST.md`) requires manual
  Dorico session — not gated automatically.

### Resume command

Multi-Version Improvement Plan complete. Next improvement cycle starts
fresh — `/improve O-MicrotonalSampler [new description]`.

---

## v1.15.0 Implementation Complete (2026-05-03)

**Status:** code complete, build green (VST3 + AU + Standalone macOS),
pluginval-5 SUCCESS, auval AU PASS (Component Version: 1.15.0), 2 new
test executables pass all assertions (50 + 56 = 106 cases).

### What landed

- **3 new APVTS params:** `cc_select_enabled` (bool, default false),
  `cc_number` (0..119, default 32 — General Purpose 1, free of
  CC1/CC11/bank-select reservations), `pc_enabled` (bool, default
  false). Round-trip through project state and host automation.
- **`Source/TriggerMapping.h`** — new pure-data header containing
  `CcSlot` / `PcSlot` structs, `defaultCcMapping(count)` /
  `defaultPcMapping()` builders, and three resolver free functions
  (`resolveCcTechnique`, `resolvePcTechnique`,
  `resolveTriggerPrecedence`). Audio thread, message thread, and
  unit tests all share identical resolution code.
- **`PluginProcessor` private members:**
  `std::shared_ptr<OMtsTrigger::CcMapping> currentCcMapping` and
  `std::shared_ptr<OMtsTrigger::PcMapping> currentPcMapping` — COW
  shared_ptr slots (matches the `currentSampleMap` pattern).
  `triggerStateChangedCallback` + `triggerStateDirty` for editor
  notifications. Constructor seeds both tables to plan-defined
  defaults.
- **`processBlock` extended** from a single-trigger (KS) walk to a
  three-trigger candidate harvest with KS > CC > PC > history
  precedence. Single-pass scan over `midiMessages` collects up to
  three candidates (each starts at -1) and resolves precedence once
  at the end via `OMtsTrigger::resolveTriggerPrecedence`. RT-safety
  preserved: COW shared_ptr reads are lock-free, the KS filter
  buffer remains pre-allocated, and a no-trigger block leaves
  `pendingTechniqueIndex` untouched (eliminates spurious
  AsyncUpdater triggers).
- **State persistence: `<CcMapping>` + `<PcMapping>`** ValueTree
  children with sparse 8-slot child lists. Stripped on capture
  (defensive). v1.14.0 sessions decode cleanly back to seeded
  defaults — the plan's back-compat contract holds.
- **7 new WebView native fns** in `PluginEditor.cpp`:
  `getTriggerState` (bulk snapshot — gates + mappings),
  `setCcEnabled`, `setCcNumber`, `setCcMapping(slot, lo, hi, tech)`,
  `setPcEnabled`, `setPcMapping(slot, pc, tech)`,
  `resetTriggerMappings`. The editor wires
  `setTriggerStateChangedCallback` to emit `triggerStateUpdated`
  events to JS.
- **HTML + CSS + JS trigger panel.** Collapsible `<details>`
  disclosure under the technique-bar with two sub-panels (CC + PC),
  each showing an 8-row editable table. CSS mirrors the
  technique-bar style language (paper-cream backgrounds, monospace
  numerics). JS state mirrors the C++ COW pattern: a local
  `triggerState` cache populated by `pullTriggerState`,
  `subscribeTriggerStateUpdates` for live sync,
  `renderTriggerPanel` driven by both trigger-state and
  technique-state changes (count drives slot dimming + visibility).
  Hidden when `technique_count == 1` (matches the technique-bar
  back-compat visual contract).
- **`docs/dynamics-mapping.md`** — new ~150-line doc explaining
  velocity-vs-CC11 dynamics path, recommended Dorico exp-map
  setting (`<volumeType><type>kCC11</type></volumeType>`), and the
  Dorico 3+ secondary-volume-control forward-compat note. Includes
  a quick-DAW-test procedure.
- **2 new EXCLUDE_FROM_ALL test execs** registered in
  CMakeLists.txt: `O-MicrotonalSampler_CcPcTriggerCheck` (50
  assertions: defaultCcMapping bucketing, defaultPcMapping pairs,
  resolveCcTechnique band routing + edge cases + overlap
  precedence, resolvePcTechnique sparse table, precedence
  resolver, end-to-end three-block scenario) and
  `O-MicrotonalSampler_DynamicsLayerCheck` (56 assertions:
  velocity-to-layer bucket boundaries for N=1/2/4/8, edge-case
  clamping, findCell-respects-layer determinism). All 106
  assertions pass.

### Files touched (10)

1. `Source/TriggerMapping.h` (NEW)
2. `Source/PluginProcessor.h`
3. `Source/PluginProcessor.cpp`
4. `Source/PluginEditor.cpp`
5. `Resources/ui/index.html`
6. `Resources/ui/css/sampler-shell.css`
7. `Resources/ui/js/sampler-app.js`
8. `Source/tests/cc_pc_trigger_check.cpp` (NEW)
9. `Source/tests/dynamics_layer_check.cpp` (NEW)
10. `docs/dynamics-mapping.md` (NEW)
11. `CMakeLists.txt`
12. `CHANGELOG.md`

### Backups

- `backups/O-MicrotonalSampler/v1.14.0/` — full plugin tree, 3.3 MB,
  created before any source edits.

### Build / validation gate

- `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone`: SUCCESS
- `ninja O-MicrotonalSampler_CcPcTriggerCheck`: SUCCESS — 50/50 assertions PASS
- `ninja O-MicrotonalSampler_DynamicsLayerCheck`: SUCCESS — 56/56 assertions PASS
- `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests`: SUCCESS
- `auval -v aumu OMtS OuDv`: AU VALIDATION SUCCEEDED (Component Version: 1.15.0)

### Resume command

Next slice in the multi-version plan: `/improve O-MicrotonalSampler`
and the skill should pick up the v1.16.0 block (Dorico distribution
— `.doricolib` + Playback Template) directly from "Sequenced Plan"
below.

---

## v1.14.0 Implementation Complete (2026-05-03)

**Status:** code complete, build green (VST3 + AU + Standalone macOS), pluginval-5 SUCCESS, auval AU PASS, 3 new test executables pass all assertions (24 + 11 + 10 = 45 cases).

### What landed

- `SampleCell` gained `int technique = 0`; `SampleMap::findCell` triplet overload with `tech=0` fallback; back-compat 2-arg overload preserved.
- `applyMergeRrCell` triplet-keyed (cells with same `(midi, vel)` but different techniques no longer collide).
- `FilenameParser::parseAsTechnique` recognises 10 default tokens (`ord`, `sp`, `st`, `sv`, `cs`, `pizz`, `harm`, `mart`, `trem`, `flaut`) — exact match, case-insensitive, never substring; `ParsedName.techniqueIndex` field added.
- `SampleLoader` 3D grouping (key = `midi*32 + layer*8 + tech`); `LoadOptions::{targetTechnique, overrideTechnique}` honoured by per-file processing.
- `LoadOp` carries `targetTechnique` + `overrideTechnique` through both user-triggered and replay-queue load paths.
- `MicrotonalSamplerVoice` reads `pendingTechniqueIndex` atomic with `memory_order_acquire` at startNote; RR counter array expanded to 4096 entries (`midi*4*8 + layer*8 + tech`); crossfade pair shares technique.
- 5 new APVTS params: `technique_count` (1..8), `technique_select` (0..7), `ks_enabled` (bool), `ks_low_note` (0..127, default 0), `ks_high_note` (0..127, default 9).
- `processBlock` keyswitch filter: pre-allocated `juce::MidiBuffer` strips note-ons/note-offs in `[ks_low..ks_high]` before forwarding to `Synthesiser::renderNextBlock`; absorbed note-on offset stores into `pendingTechniqueIndex` atom.
- `<TechniqueNames>` state-tree child (sparse, only renamed slots); curated default vocab seeded by ctor.
- 8 new WebView native fns (`getTechniqueState`, `setActiveTechnique`, `setTechniqueName`, `resetTechniqueNames`, `addTechniqueSlot`, `removeTechniqueSlot`, `setKeyswitchEnabled`, `setKeyswitchRange`). Existing 4 (loadSingleSampleDialog / overrideLoopPoints / resetLoopToAutoDetect / getWaveformPeaks) gained optional trailing `technique` arg defaulting to active cursor.
- HTML/CSS/JS technique tab strip + KS picker + rename modal; bar hidden until count > 1 OR ks_enabled (back-compat visual contract).
- 3 new EXCLUDE_FROM_ALL test execs registered in CMakeLists.txt.

### Files touched (10)

1. `Source/SampleMap.h`
2. `Source/FilenameParser.{h,cpp}`
3. `Source/SampleLoader.{h,cpp}`
4. `Source/PluginProcessor.{h,cpp}`
5. `Source/MicrotonalSamplerVoice.{h,cpp}`
6. `Source/PluginEditor.cpp`
7. `Resources/ui/index.html`
8. `Resources/ui/css/sampler-shell.css`
9. `Resources/ui/js/sampler-app.js`
10. `CMakeLists.txt`
11. `Source/tests/technique_parse_check.cpp`, `find_cell_triplet_check.cpp`, `state_migration_check.cpp` (NEW)
12. `CHANGELOG.md`

### Backups

- `backups/O-MicrotonalSampler/v1.13.0/` — full plugin tree, 3.1 MB, created before any source edits.

### Resume command

Next slice in the multi-version plan: `/improve O-MicrotonalSampler` and the skill should pick up the v1.15.0 block (CC + PC triggers + Dynamics audit) directly from "Sequenced Plan" below.

---

## Active Multi-Version Improvement Plan: Playing Techniques + Dorico (2026-05-03)

**Mode:** `/improve --research --discuss` — research and plan complete. v1.14.0 EXECUTED 2026-05-03. v1.15.0 + v1.16.0 still pending.

**Goal (verbatim from user):** Add the capacity for different playing techniques (e.g. *senza vibrato*, *sul pont*, *ordinario*) triggered via keyswitch / CC / program change. Compatible with Dorico playback (via expression mapping) and accessible through DAWs. Also ensure dynamics are correctly mapped from Dorico notation to playback.

### Decisions (user-confirmed)

| # | Decision | Choice |
|---|---|---|
| Q1 | Trigger mechanism | Keyswitches **+** MIDI CC **+** Program Change (all three; NE skipped — Dorico does not switch articulations via NE) |
| Q2 | Sample loading | Filename-token auto-detect **+** option in folder-load modal to assign-folder-to-technique manually |
| Q3 | Technique vocabulary | Hybrid: ship ~10 curated defaults (ord, sul pont, sul tasto, senza vib, con sord, pizz, harm, mart, trem, flautando) — user can rename / extend |
| Q4 | Dorico shipping | Expression Map (`.doricolib`) **+** Playback Template (auto-loads plugin) **+** Velocity-vs-CC dynamics audit + docs |
| Q5 | Workflow | Multiple `/improve` iterations (one per minor version) — research/plan now, execute next |
| Q6 | Versioning | Backward-compatible MINOR — old presets load as single-technique "ord" libraries, KS off by default until 2nd technique added |

### Research Findings (referenced by versions below)

**RF-1 — Dorico expression-map XML schema** (confirmed via `mhcoffin/go-doricolib` Go schema + existing `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`)

- Root: `<kScoreLibrary>` with ~40 entity-list sections; the relevant ones are `<playingTechniques>`, `<expressionMapDefinitions>`, `<playingTechniqueAppearanceCollectionDefinition>`.
- An `ExpressionMapDefinition` contains `<playingTechniqueCombinations>`. Each combination has: `<baseSwitchID>`, `<techniqueIDs>` (e.g. `pt.sulPonticello`), `<switchOnActions>`, `<switchOffActions>`, `<volumeType>`, `<attackType>`, plus modulators (velocity range, pitch range, transpose, etc.).
- Action types (the `<type>` field inside `<switchOnAction>` / `<switchOffAction>`):
  - `kKeySwitch` — params: MIDI note + velocity
  - `kControlChange` — params: CC# + value
  - `kProgramChange` — params: PC#
  - `kAbsoluteChannelChange`, `kRelativeChannelChange`
- Switch types (3): **Base** (mutually exclusive, e.g. arco↔pizz), **Add-on** (combines with base, e.g. legato), **Init** (sent at playback start).
- The microtonality bridge already shipped uses `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` — we will inherit/parent from `xmap.ouaricon.vst3_note_expression` so the new map ALSO gets microtonal pitch routing.
- Dorico's built-in technique vocabulary maps cleanly to our curated defaults:

| Plugin default | Dorico ID | Notation glyph in Dorico |
|---|---|---|
| ord | `pt.natural` | "Ord." text |
| sul pont | `pt.sulPonticello` | "Sul pont." text |
| sul tasto | `pt.sulTasto` | "Sul tasto" text |
| senza vib | `pt.nonVibrato` | "Senza vib." text |
| con sord | `pt.muted` | mute glyph |
| pizz | `pt.pizzicato` | "Pizz." text |
| harm | `pt.naturalHarmonic1` (or `pt.harpHarmonic`) | small circle |
| mart | `pt.martele` / `pt.martellato` | wedge accent |
| trem | `pt.tremolo` | tremolo strokes |
| flautando | `pt.flautando` | "Flaut." text |

Custom user-defined techniques use the `pt.user.<name>` prefix.

**RF-2 — Dorico Playback Template + Endpoint Configuration**

- **User library on disk** (macOS): `~/Library/Application Support/Steinberg/Dorico 6/`
  - `EndpointConfigs/` — endpoint configurations (VST + exp-map + routing)
  - `DefaultLibraryAdditions/` — `.doricolib` files auto-merged into every project's library
  - `PlaybackTemplates/` (or merged into the user library) — saved templates
- **`.doricolib`** is the only single-file format that Dorico's `Library → Library Manager → Import…` accepts for cross-document distribution. Standalone `.doricoexpmap` is **not** auto-ingested (per existing memory `critical_dorico_distribution_mechanism.md`).
- **Distribution path for end-users:** ship `O-MicrotonalSampler.doricolib` containing the expression map(s) + Playback Template + Endpoint Configuration. User imports once via Library Manager, the template appears in `Play → Playback Template` dropdown for all subsequent projects.
- Endpoint Configuration ties together: **VST instance = O-MicrotonalSampler** + **Expression Map = our new map** + **MIDI channel routing**. Saved via `Play → Playback Template → Endpoint Setup → Save Endpoint Configuration`.

**RF-3 — Dorico dynamics path**

- Dorico's expression map has a `<volumeType>` field (the "primary volume control") — its `<type>` can be `kNoteVelocity`, `kCC1`, `kCC11`, `kCC7`, `kAftertouch`, etc.
- **`kNoteVelocity` only sets the layer at note-on; it does NOT continuously modulate during sustained notes.** This is a critical mismatch for sustained microtonal long-tones (the plugin's primary use case).
- **`kCC11` (Expression)** continuously modulates volume during sustain — this is what most orchestral exp-maps use as primary.
- Dorico 3+ adds **secondary volume control** (currently mirrors primary; future-Dorico will allow independent curves). Expected pattern for a layered sampler: **primary = velocity (selects the right layer), secondary = CC11 (smooths volume within the layer)** — but only one curve today.
- **Plugin already maps CC11 → "Expression" parameter** (verified in audit, `PluginProcessor.cpp` lines 309–394). Ship two exp-map variants: "Velocity Dynamics" (matches our 4 layers, no continuous swell during sustain) and "CC11 Expression" (smooth swell, no layer-crossfade across dynamic shape) — recommend the latter as default in docs.

**RF-4 — Source-code audit (technique-axis impact)**

Full audit in this STATUS.md commit history. Summary of impacted files:

| File | Change scope |
|---|---|
| `Source/SampleMap.h` | Add `int technique` to `SampleCell`; extend `findCell(midi, vel, tech)`; extend `applyMergeRrCell` triplet match; pre-size RR counter array to worst-case 4096 (128×4×8) |
| `Source/FilenameParser.{h,cpp}` | Add `int techniqueIndex = -1` to `ParsedName`; new `parseAsTechnique()` recognizing `_ord`, `_sp`, `_st`, `_sv`, `_cs`, `_pizz`, `_harm`, `_mart`, `_trem`, `_flaut` (case-insensitive, delimited) |
| `Source/SampleLoader.{h,cpp}` | Group by `(midi, vel, tech)` triplet; extend `LoadOptions` with `targetTechnique` + `overrideTechnique`; `AmbiguousDuplicate` gains `int technique` |
| `Source/PluginProcessor.{h,cpp}` | New APVTS: `technique_count` (1–8), `technique_select` (current technique), `ks_enabled`, `ks_low_note`, `ks_high_note`, `cc_select_enabled`, `cc_number`, `pc_enabled`. Vocabulary names persisted via state ValueTree (not APVTS — strings). MIDI scan in `processBlock` for KS/CC/PC → atomic `pendingTechniqueIndex`. Extend `LoadOp` with technique fields. State migration: missing fields → defaults |
| `Source/MicrotonalSamplerVoice.{h,cpp}` | At `startNote`, read `pendingTechniqueIndex.load()` before resolving cell; `findCell(midi, vel, tech)` with fallback to `tech=0` if not present; RR counter index becomes `midi * 4 * 8 + vel * 8 + tech`; crossfade-adjacent cells must match technique (no cross-technique morph) |
| `Source/PluginEditor.cpp` | Native fns gain optional `technique` arg (default 0): `loadSingleSampleDialog`, `getCellInfo`, `overrideLoopPoints`, `resetLoopToAutoDetect`, `getWaveformPeaks`. New native fns: `setTechniqueName`, `addTechniqueSlot`, `removeTechniqueSlot`, `selectTechniqueForFolderLoad` |
| `Resources/ui/index.html` | Technique tab strip in main view; technique-rename modal; KS-range picker; CC# picker; PC enable toggle; folder-load modal grows technique dropdown |
| `Resources/ui/js/sampler-app.js` | Technique state in `editorState`; folder-load modal extension; per-cell click → respects current technique; new modal for "drop folder onto technique" |
| `Source/tests/` | New: `technique_parse_check.cpp`, `find_cell_triplet_check.cpp`, `technique_state_migration_check.cpp` |
| `Resources/dorico/` (NEW) | `O-MicrotonalSampler.doricolib` containing exp-map + Playback Template + Endpoint Configuration |
| `INSTALL-DORICO.md` (NEW) | End-user instructions for importing the .doricolib via Library Manager |
| `docs/dynamics-mapping.md` (NEW) | Dynamics path documentation (velocity vs CC11; recommended exp-map setting) |

---

## Sequenced Plan

### v1.14.0 — Engine + Keyswitches + UI core (MINOR)

**Goal:** Plugin can hold multiple techniques per cell and switch between them via keyswitches. Standalone-testable. No CC/PC yet, no Dorico shipping yet.

**Files touched (10):**

1. `Source/SampleMap.h` — technique axis on `SampleCell`; `findCell(midi, vel, tech)` with `tech=0` fallback; `applyMergeRrCell` triplet match; counter-array sizing constants.
2. `Source/FilenameParser.{h,cpp}` — `techniqueIndex` field; `parseAsTechnique()` helper; default vocab token table.
3. `Source/SampleLoader.{h,cpp}` — 3D grouping; `LoadOptions::{targetTechnique, overrideTechnique}`; `AmbiguousDuplicate::technique`.
4. `Source/PluginProcessor.{h,cpp}` — new APVTS: `technique_count`, `technique_select`, `ks_enabled`, `ks_low_note`, `ks_high_note`. Vocabulary names persisted via state ValueTree (`<techniqueNames>` child, JSON-encoded array of 8 strings). KS scan in `processBlock` (note-on in [`ks_low_note`..`ks_high_note`] sets `pendingTechniqueIndex` atomic, blocks the note from reaching synth). Extend `LoadOp`. State migration: absent fields → defaults (technique_count=1, ks_enabled=false).
5. `Source/MicrotonalSamplerVoice.{h,cpp}` — read `pendingTechniqueIndex` at startNote; technique-aware findCell + RR counter (resize to 4096); crossfade pair must share technique.
6. `Source/PluginEditor.cpp` — native fns extended with technique arg; new natives: `setTechniqueName(idx, name)`, `addTechniqueSlot()`, `removeTechniqueSlot(idx)`.
7. `Resources/ui/index.html` — technique tab strip above sample grid; rename modal; KS-range picker (two number inputs + on/off toggle).
8. `Resources/ui/js/sampler-app.js` — `editorState.activeTechnique`, `editorState.techniqueNames`, `editorState.ksEnabled/Low/High`. Folder-load modal grows technique dropdown + override checkbox. Per-cell click respects current technique tab.
9. `Source/tests/technique_parse_check.cpp`, `find_cell_triplet_check.cpp`, `state_migration_check.cpp` (new).
10. `CMakeLists.txt` — bump `PLUGIN_VERSION` 1.13.0 → 1.14.0; add new test targets.

**RT-safety contract:**

- No allocations in `processBlock` KS scan (pure atomic store on note-on in KS range).
- KS notes are absorbed (don't reach the synth) — they only update `pendingTechniqueIndex.store(newTech, std::memory_order_release)`.
- Voice in flight when technique flips: holds its captured `cellLow/cellHigh` to release; only NEW note-ons see the new technique.
- RR counter is pre-sized to 4096 (worst case `128 × 4 × 8`); old voices reading old indices remain safe.
- SampleMap atomic-swap unchanged.

**Test surface:**

1. `parseAsTechnique` recognizes all 10 default tokens, case-insensitive, delimited; rejects false positives like `chord_suspended.wav` matching `sv` substring.
2. `findCell(midi, vel, tech)` — exact match, fallback to `tech=0`, returns null if neither.
3. State migration: load v1.13.0 preset → all cells become `technique=0`, `technique_count=1`, `ks_enabled=false`. Audio bit-identical to v1.13.0 reference.
4. KS detection: note-on in KS range increments `pendingTechniqueIndex`, doesn't reach synth; note-on outside range plays normally and reads current technique.
5. Round-robin counter indexing: `midi=60, vel=0, tech=0` and `midi=60, vel=0, tech=1` advance independently.
6. Render-harness identity (single-technique library): bit-identical output vs v1.13.0.

**Acceptance:**

- Build green (VST3 + AU + Standalone, macOS); pluginval-5 SUCCESS; auval AU PASS.
- Drop a folder of `C4_v1_ord.wav, C4_v1_sp.wav` files; both load into the same cell coordinate, different technique slots.
- KS C-1 → switches active technique to slot 0; C#-1 → slot 1; etc. Audible.
- Old presets (v1.13.0) load and play unchanged.

**Resume command (next session):** `/improve O-MicrotonalSampler` — start from this STATUS.md, version block "v1.14.0 — Engine + Keyswitches + UI core".

---

### v1.15.0 — CC + PC triggers + Dynamics audit (MINOR)

**Goal:** Plugin reachable from any DAW via three trigger mechanisms. Velocity-vs-CC11 dynamics path documented and verified.

**Files touched (~7):**

1. `Source/PluginProcessor.{h,cpp}` — APVTS: `cc_select_enabled`, `cc_number` (0–119, default 32), `pc_enabled`. Add CC#-mapping table (state ValueTree: 8 entries, value-range → technique-index, default = equal split 0–127 / N). Add PC-mapping table (8 entries, PC# → technique-index, default 0..7). MIDI scan in `processBlock`: CC value → atomic `pendingTechniqueIndex` if `cc_select_enabled`; PC → atomic if `pc_enabled`. Trigger precedence at note-on: KS > CC > PC > history > 0.
2. `Resources/ui/index.html` — CC-trigger panel (CC# selector + value-range editor for 8 slots); PC-trigger panel (8 PC# inputs); precedence indicator.
3. `Resources/ui/js/sampler-app.js` — bindings for the new panels.
4. `Source/tests/cc_pc_trigger_check.cpp` (new) — verify CC value range routing + PC routing; verify precedence.
5. `Source/tests/dynamics_layer_check.cpp` (new) — render-harness test verifying that varying note-on velocity hits the correct layer (sanity check existing behavior).
6. `docs/dynamics-mapping.md` (new) — explains: (a) velocity sets layer at note-on only — does NOT continuously modulate during sustain; (b) CC11 ("Expression" param) continuously modulates volume during sustain; (c) recommended Dorico exp-map setting: **"CC11 Expression" as primary volume**, accept layer crossfade is fixed at note-on; (d) alternative for short notes: "Velocity" as primary; (e) the secondary-volume-control option (Dorico 3+) and what it does today vs future.
7. `CMakeLists.txt` — bump 1.14.0 → 1.15.0.

**Test surface:** CC value-range table → correct technique; PC# → correct technique; precedence (KS > CC > PC) holds when multiple are active simultaneously; render harness shows correct velocity-layer selection for v0..v127 input.

**Acceptance:** In Logic / Reaper / Cubase, all three triggers verified with a test MIDI clip. `docs/dynamics-mapping.md` reviewed by user.

**Resume command:** `/improve O-MicrotonalSampler` — start from STATUS.md, version block "v1.15.0 — CC + PC triggers + Dynamics audit".

---

### v1.16.0 — Dorico distribution (`.doricolib` + Playback Template) (MINOR)

**Goal:** Dorico user installs one `.doricolib` via Library Manager and gets: (a) plugin auto-loads on new projects, (b) Sul pont./Senza vib./Ord. notation triggers technique switching, (c) dynamics route correctly to playback.

**Files touched (~5):**

1. `Resources/dorico/O-MicrotonalSampler.doricolib` (new) — extends parent `xmap.ouaricon.vst3_note_expression` (microtonal NE preserved). Contains:
   - One `ExpressionMapDefinition` (entityID `xmap.ouaricon.o_microtonalsampler`) with 10 `playingTechniqueCombinations` — one per default technique. Each combination has `<switchOnActions>` containing a single `<switchOnAction>` of `<type>kKeySwitch</type>` with `<param1>` = MIDI note (C-1 + index, i.e. 0..9) and `<param2>` = 127 (velocity).
   - `<volumeType><type>kCC11</type></volumeType>` (continuous expression), with a documentation note inside the description that users with "Velocity dynamics" preference can switch to `kNoteVelocity` in Dorico's UI.
   - `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` (inherited).
   - One `EndpointConfiguration` referencing `O-MicrotonalSampler` VST3 + the new exp-map + MIDI channel 1.
   - One `PlaybackTemplate` referencing the EndpointConfiguration (auto-applies on `New from Template`).
2. `Resources/dorico/INSTALL-DORICO.md` (new) — end-user import instructions: open Dorico → Library → Library Manager → Import → select `O-MicrotonalSampler.doricolib`. Verify in `Play → Playback Template`. Smoke-test procedure.
3. `Resources/dorico/SMOKE-TEST.md` (new) — manual smoke procedure: new project from O-MicrotonalSampler template, type pizz./sul pont./senza vib./ord. text on staves, verify each switches the technique audibly (visual: playback-technique panel in Play mode shows technique switching). Reuse procedure from existing `.planning/stages/4-polish/RESEARCH.md` RQ4-4.
4. `CMakeLists.txt` — bump 1.15.0 → 1.16.0; ensure `Resources/dorico/` ships in installer.
5. `INSTALLER-NOTES.md` patch — call out the Dorico bundle in the installer post-install message.

**Acceptance:** In Dorico 5/6: user imports `.doricolib`, creates new project from template, types `pizz.` on a staff, hears pizzicato variant. Microtonal pitch (24-EDO) plays correctly (parent-map inheritance verified). Dynamics route via CC11; sustained note swells.

**Resume command:** `/improve O-MicrotonalSampler` — start from STATUS.md, version block "v1.16.0 — Dorico distribution".

---

## Hand-off / Resume Instructions

For the next session: clear context, run `/improve O-MicrotonalSampler`. The skill's Phase 0.45 will detect this completed research block in STATUS.md and skip re-investigation; Phase 0.6 will pick up the v1.14.0 plan above and ask for the version-bump confirmation gate.

Each version is a fresh `/improve` cycle — research has been done up-front so subsequent cycles run lighter (no `--research` flag needed).

**Open questions to revisit before v1.16.0 build:**
- Confirm the exact path/filename Dorico expects under `~/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/` for an auto-discovered `.doricolib`. Spike a manual import first; harden after.
- Decide whether the bundled `.doricolib` should also re-export the parent NE map (single file = one-step user import) or chain via `parentEntityID` (assumes user has already imported the NE module's library — risky). **Lean toward bundling parent inline.**

---



## v1.9.0 Implementation Complete (2026-05-01)

**Status:** code complete, awaiting build verification and user DAW smoke test.

### What landed

- **`Source/PluginProcessor.h`** — `LoadMode::MergeRR = 3` added with docstring. `loadSingleSample` gains optional `mergeAsRr` parameter (default false).
- **`Source/PluginProcessor.cpp`** — `applyFolderLoad` per-cell collision branches on mode: `MergeRR` calls the new `applyMergeRrCell` helper; other modes keep replace semantics. `loadSingleSample` completion callback honours `mergeAsRr` — appends to existing cell's variants vector instead of replacing. Variant cap (64) enforced via `lastSkippedFiles` on overflow. RR counter reset on every touched cell. `loadModeToString`/`loadModeFromString` add `"merge_rr"` (round-trip safe in saved presets).
- **`Source/SampleMap.h`** — new `applyMergeRrCell` pure helper (header-only): inserts new cell or appends variants on collision, enforces cap, records skipped overflow filenames. Used by both folder-load `MergeRR` and the per-cell merge path; isolates merge contract for unit tests.
- **`Source/PluginEditor.cpp`** — `loadSingleSampleDialog` native fn accepts optional 3rd arg `mergeAsRr`, forwards to `processorRef.loadSingleSample(midi, vel, file, mergeAsRr)`.
- **`Resources/ui/index.html`** — folder-load modal grows a 4th radio (`merge_rr` / "Layer as round-robin"). New `#per-cell-merge-dialog` markup with three buttons (Cancel, Replace cell, Add as round-robin).
- **`Resources/ui/js/sampler-app.js`** — `showFolderLoadOptionsModal` handles new mode + explanation copy. New `showPerCellMergeDialog(existingCount, midi, layer)` returning `'merge'` / `'replace'` / `null`. `replaceCellSample` surfaces the prompt for non-empty cells before calling `loadSingleSampleDialog` with the chosen flag. Cap-reached state disables the merge button.
- **`Source/tests/merge_rr_check.cpp`** — six unit tests over `applyMergeRrCell` (no-collision insert, collision merge order, variant cap, cap-already-reached, layer-aware key, multi-call shape).
- **`CMakeLists.txt`** — `PLUGIN_VERSION` 1.8.0 → 1.9.0. New `O-MicrotonalSampler_MergeRrCheck` test target (EXCLUDE_FROM_ALL).
- **`CHANGELOG.md`** — `[1.9.0] - 2026-05-01` entry with Added/Changed/Implementation notes/Test surface sections.

### Backups

- `backups/O-MicrotonalSampler/v1.8.0/` (pre-improvement; created from working tree since v1.8.0 was uncommitted)
- Older history: v1.0.0 through v1.7.1 (already on disk)

### Outstanding

1. **Build verification** — triple build (VST3 + AU + Standalone) + `O-MicrotonalSampler_MergeRrCheck` exit code 0.
2. **User-side DAW smoke test** — exercise the new "Layer as round-robin" radio with two folders that share notes; exercise per-cell merge prompt via dblclick on a loaded cell.
3. **Atomic commit** of v1.8.0 + v1.9.0 work (currently both sets are uncommitted in the working tree).

### Resume command

`/improve O-MicrotonalSampler --discuss` to plan v1.9.x (drag-drop merge prompt, cross-cell RR group tagging, per-variant velocity sub-layering).

---

## Previous: v1.8.0 — Round-Robin Samples (IMPLEMENTED 2026-05-01)

**Status:** built, installed, pluginval-5 SUCCESS, auval AU VALIDATION SUCCEEDED.
Awaiting user commit + DAW smoke test.

### v1.8.0 What landed

- **`Source/SampleMap.h`** — introduced `SampleVariant` (audio + per-take loop fields) and renamed `SampleSlot` → `SampleCell` (the addressing wrapper holding a `std::vector<SampleVariant>`). `findSlot` → `findCell`. `slots` → `cells`.
- **`Source/FilenameParser.{h,cpp}`** — added `rrIndex` to `ParsedName`. New `parseAsRrIndex` recognises `rr[N]` / `take[N]` / `tk[N]` (1-based, capped at 64). New unit-test cases cover all three forms in pre- and post-note positions plus rejection of `round[N]` / `var[N]`.
- **`Source/SampleLoader.{h,cpp}`** — `loadSingleSlot` → `loadSingleVariant`. Folder-mode `run()` groups parsed files by `(midi, layer)`; explicit RR tokens flow silently into multi-variant cells, bare duplicates surface as `AmbiguousDuplicate` entries in the new 3-arg `CompletionCallback`. Stable-sort puts explicit-RR entries first by `rrIndex` then load-order.
- **`Source/PluginProcessor.{h,cpp}`** — new `rr_mode` `AudioParameterChoice` (Cycle / Random No-Repeat / Random; default RandomNoRepeat). 512-entry `std::array<std::atomic<uint8_t>, 128*4>` per-cell counter array (deviation from 352 spec — 0.16 KB more for index-bound safety). `confirmRoundRobinLoad(bool)` applies or discards the staged ambiguous-duplicate map; chains correctly through state-restore replay queue. Counters reset on `ReplaceAll`, per-layer wipe on `ReplaceLayer`, per-cell on collision. `loadSingleSample` now replaces a cell with a single-variant cell. `overrideLoopPoints` / `resetLoopToAutoDetect` / `snapshotWaveformPeaks` gain a `variantIndex` parameter (default = primary).
- **`Source/MicrotonalSamplerVoice.{h,cpp}`** — `slotLow/slotHigh` → `cellLow/cellHigh` + `variantLow/variantHigh`. New `selectVariantIndex(cell, mode)` called twice in `startNote` (primary + crossfade-adjacent). Pure atomic ops + integer math + per-voice xorshift32 PRNG (seeded from `this` ptr ⊕ sample rate). Render path reads from the selected variant's audio buffer + loop fields.
- **`Source/PluginEditor.cpp`** — `ambiguousDuplicateCallback` wired in ctor, emits `ambiguousDuplicates` event with JSON payload to JS. New `confirmRoundRobinLoad` native fn forwards the user's decision. Existing `overrideLoopPoints` / `resetLoopToAutoDetect` / `getWaveformPeaks` natives forward optional `variantIndex` arg.
- **`Resources/ui/js/sampler-app.js`** — multi-variant cell tooltip lists every variant filename + dot glyph for visual scan. Loop editor `editorState` gains `variantIndex` / `variantCount`. New `renderVariantTabStrip()` renders pill tabs when `variantCount > 1`. New `subscribeAmbiguousDuplicatesEvent` + `showAmbiguousDuplicatesDialog` modal flow forwards user decision via `confirmRoundRobinLoad` native.
- **`Resources/ui/index.html`** — `<div id="le-variant-tabs">` between loop-editor header and canvas; full `<div id="rr-confirm-dialog">` modal markup.
- **`Resources/ui/css/sampler-shell.css`** *(spec named `sampler.css` — no such file exists; appended to `sampler-shell.css`)* — `.cell-multivariant::after` antique-gold dot glyph; `.le-variant-tabs` strip styling; `.rr-confirm-dialog-content` + `.rr-confirm-list` modal styling.
- **`CMakeLists.txt`** — `PLUGIN_VERSION` 1.7.1 → 1.8.0.
- **`CHANGELOG.md`** — `[1.8.0] - 2026-05-01` entry.

### Build / validation

- Triple build (VST3 + AU + Standalone): GREEN
- AU cache cleared + reinstalled per CLAUDE.md
- `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests --timeout-ms 120000`: **SUCCESS**
- `auval -v aumu OMtS OuDv`: **AU VALIDATION SUCCEEDED**

### Backups

- `backups/O-MicrotonalSampler/v1.7.1/` (pre-improvement; verify-backup.sh PASS)
- Older history: v1.0.0 through v1.7.0 (already on disk)

### Outstanding

1. **User-side DAW smoke test** — exercise round-robin selection in Logic / Standalone with a real RR sample folder (rr/take/tk filenames) plus the bare-duplicate modal flow.
2. **Atomic commit** of all v1.8.0 files (10 sources + status + changelog) per gsd convention.

### Resume command

`/improve O-MicrotonalSampler --discuss` to plan v1.9 follow-ups (per-variant velocity sub-layering, cross-cell RR group tagging, per-cell algorithm override).

---

## Previous Active Improvement Plan: v1.8.0 — Round-Robin Samples (APPROVED 2026-05-01)

**Mode**: `/improve --discuss` — plan approved, awaiting implementation kickoff.

### Design decisions (user-confirmed)

- **Q1 — Detection**: Recognize explicit `rr[N]` / `take[N]` / `tk[N]` filename tokens AND fall back to duplicate detection. When duplicates are detected *without* explicit RR tokens, fire a WebView modal to confirm intent before treating as variants. Explicit tokens load silently.
- **Q2 — Selection algorithm**: User-selectable via dropdown — Cycle / Random No-Repeat / Random. Default = **Random No-Repeat** (industry standard).
- **Q3 — UI exposure**: No surface badge on cells. Tooltip (`title` attr) lists all variant filenames on hover. Loop editor side panel gains a variant-tab strip when `variants.length > 1` so loop points can be set per variant.
- **Q4 — Version bump**: MINOR → v1.8.0.

### Files touched (10)

- `Source/SampleMap.h` — `SampleSlot` → `SampleCell` with `std::vector<SampleVariant>`; loop fields move to variant.
- `Source/FilenameParser.{h,cpp}` — add `rrIndex` to `ParsedName`; recognize `rr[N]`/`take[N]`/`tk[N]` tokens.
- `Source/SampleLoader.{h,cpp}` — group parsed files by `(midi, layer)`; explicit RR → silent variants; ambiguous duplicates → modal payload.
- `Source/PluginProcessor.{h,cpp}` — new APVTS `rr_mode` choice param; processor-level `std::array<std::atomic<uint8_t>, 352>` per-cell RR state (cycle counter + last-variant); new native fn `confirmRoundRobinLoad`; preset-load migration for old slot format.
- `Source/MicrotonalSamplerVoice.{h,cpp}` — `slotLow/slotHigh` → `variantLow/variantHigh`; new `selectVariant()` helper called twice in `startNote` (primary + crossfade-adjacent).
- `Source/PluginEditor.cpp` — variant-list JSON pushed to JS; modal-trigger event on ambiguous duplicates.
- `Resources/ui/js/sampler-app.js` — multi-line variant tooltips; loop editor variant-tab strip; modal dialog component.
- `Resources/ui/css/sampler.css` — variant-tab + modal styling.
- `CMakeLists.txt` — bump VERSION 1.7.1 → 1.8.0.
- `CHANGELOG.md` — `[1.8.0]` entry.

### RT-safety contract

- No allocations in `startNote` or `renderNextBlock` — variant selection is pure index math + atomic ops.
- SampleMap atomic-swap semantics preserved — voices in flight finish on captured snapshot.
- Per-cell RR counter survives map swaps (lives in processor, not map). Reset only on `LoadMode::ReplaceAll`.

### Test surface

- **Render-harness identity**: single-variant library produces bit-identical output vs v1.7.1 (proves zero overhead in common case).
- **New unit tests**: filename parser RR tokens; variant selection (cycle / no-repeat / random) edge cases at N=1/2/N; counter persistence across map swaps.
- **Manual DAW pass**: explicit RR tokens (silent load), pure duplicates (modal fires), no duplicates (regression-free).

### Risks called out

1. Preset compatibility — old format migration on load (additive, non-breaking).
2. Loop editor UX — must default to variant 0 with clear "Variant 1 of N" indicator.
3. Render-harness identity test must pass before merge.

### Out of scope (deferred to v1.9)

- Per-variant velocity sub-layering
- Cross-cell RR group tagging
- Per-cell RR algorithm override

### Resume command

`/improve O-MicrotonalSampler` (without `--discuss`) — picks up from approved plan, runs Phase 0.9 (backup) → Phase 1 (version) → Phase 3 (implement) onwards.

---

## Previous State: STAGE 4 VERIFIED — plugin v1.0 complete

`/plugin-verify O-MicrotonalSampler 4-polish` walked the goal-backward
analysis across Phases 4.1–4.4, re-confirmed all invariant greps and
gate-log SUCCESS lines, and appended the **Goal-Backward Roll-up**
section to `.planning/stages/4-polish/VERIFICATION.md`. All 22
requirements `complete`; final verdict ✅ VERIFIED. Three v1.1
follow-ups (V11-LOOP-FALLBACK, V11-PERF-METER, V11-MIXED-SR-EXPLICIT)
logged — none block v1.0.

Next: `/install-plugin O-MicrotonalSampler` (drop the `-dev` suffix
and install for general internal use), or `/show-standalone` for a
visual inspection pass.

## Previous State: STAGE 4 COMPLETE (execute) — v1.0 ready for internal use

`/plugin-execute O-MicrotonalSampler 4-polish` Phase 4.4 produced
`.planning/stages/4-polish/PHASE-4.4-SUMMARY.md`, extended
`VERIFICATION.md` with the Stage Gate Evidence section, overwrote
`gate-report.json` (phase 4.4 payload), and persisted three run logs
under `logs/`. All 22 requirements complete. Three v1.1 follow-ups
logged — none block v1.0.

### Phase 4.4 final gate — all green

| Check | Outcome |
|---|---|
| Cache-clear + reinstall (per CLAUDE.md) | ✓ |
| Triple build current from Phase 4.1 (b47434d) | ✓ ninja: no work to do (4.2 + 4.3 docs-only) |
| pluginval `--strictness-level 10 --skip-gui-tests` (seed `0xC0FFEE` / 120 s) | **SUCCESS** — gate-of-record for PERF-02 |
| pluginval `--strictness-level 10` (with-GUI) | **SUCCESS** |
| auval `-v aumu OMtS OuDv` | **AU VALIDATION SUCCEEDED** |
| Logic AU smoke (USER) | PASS — Path B abbreviated spot check |
| Dorico microtonal smoke (USER) | PASS — Path B carry-forward from Phase 4.3 item 4 (user-confirmed: Dorico with VST3 Note Expression expression map, not Auto) |
| Latency-zero grep (PERF-04) | single comment-only hit at PluginProcessor.cpp:133 ✓ |
| WebView2 flags grep (3-of-3) | NEEDS_WEBVIEW2 + STATIC_LINKING=1 + withUserDataFolder ✓ |
| `v0.1.0` literal grep | zero hits ✓ |
| modules.json no-new-deps | vacuous PASS (file does not exist; juce::* only) ✓ |

### PERF-02 conditional flip → unconditional

Phase 4.2 flipped `PERF-02` partial → complete conditionally on
Phase 4.4 strictness-10 timing PASS. **Strictness-10 PASSED.** The
flip is now unconditional. No rollback. No Stage 2 reopen.

### REQUIREMENTS.md final state

All 22 rows = `complete`. Two flips during Stage 4:

- `PERF-02` partial → complete (Phase 4.2; objective gate-of-record = strictness-10)
- `QUAL-01` partial → complete (Phase 4.3; 6/7 unambiguous PASS)

### v1.1 follow-ups (none block v1.0)

- **V11-LOOP-FALLBACK** — default loop fallback should loop entire
  sample when LoopDetector variance/headroom gates fail but length
  gate passes (Stage 2.5 owner)
- **V11-PERF-METER** — capture Logic Performance Meter delta on a
  future Logic release / alt DAW (Stage 4 / metrology owner)
- **V11-MIXED-SR-EXPLICIT** — explicit mixed-SR fixture listening
  pass (Stage 4 / verify owner)

### v1.0 scope (per CONTEXT.md D4-3)

Internal use only. macOS VST3 + AU + Standalone. No code-signing, no
installer, no public release. Public distribution is a post-v1.0
milestone.

## Stage 4 Sub-stage Status (FINAL)

| Phase | Goal | Gate | Commit | Status |
|---|---|---|---|---|
| 4.1 Version pill | runtime version-pill via `getPluginVersion` | triple build + pluginval-5 + auval | b47434d | ✅ PASS |
| 4.2 PERF-02 | 16-voice CPU budget within spec | methodology-deviation; gate-of-record = 4.4 | 3ca88e4 | ✅ PASS (now unconditional) |
| 4.3 QUAL-01 | listening pass (no clicks / zipper / aliasing) | 7-item subjective checklist | 7c57c30 | ✅ PASS (Path A) |
| 4.4 Final gate | pluginval-10 + auval + Logic + Dorico smoke + invariants | strictness-10 SUCCESS, all greens | this commit | ✅ PASS (Path B) |

## Next phase

`/plugin-verify O-MicrotonalSampler 4-polish` — final goal-backward
verification across all four sub-phases against `BRIEF.md` + the 22
requirements. Should produce a green VERIFIED verdict given the
Phase 4.4 gate evidence captured here. After verify, optional
follow-ups: `/install-plugin O-MicrotonalSampler` (drop the `-dev`
suffix and install for general internal use), or `/show-standalone`.

## Previous State: Phase 4.3 QUAL-01 listening pass closed (Path A)

`/plugin-execute O-MicrotonalSampler 4-polish` Phase 4.3 produced
`.planning/stages/4-polish/PHASE-4.3-SUMMARY.md` and extended
`VERIFICATION.md` + `gate-report.json` (phase 4.3 payload). User-driven
listening pass on the Phase 4.1 gate-time bundle (`-dev`).

### 7-item checklist outcomes

| # | Test | Verdict |
|---|---|---|
| 1 | Sustained sine | PASS (audio quality); behavioral observation → v1.1 V11-LOOP-FALLBACK |
| 2 | Cello vibrato / organic legato | PASS |
| 3 | Transient / plucked | PASS |
| 4 | ±50 c retune sweep | PASS |
| 5 | Voice-steal stress (24 notes / 16 cap) | PASS |
| 6 | Mixed-SR fixture | SKIPPED → v1.1 V11-MIXED-SR-EXPLICIT |
| 7 | Short-region loop | PASS |

### Path A taken (per pre-execute discuss)

Item 1 surfaced a behavioral / spec gap rather than a QUAL-01
audio-quality defect: sustained sine plays cleanly (no clicks /
zipper / DC) but doesn't loop until note-off because LoopDetector's
variance gate rejects sines (constant RMS, no quiet window) and falls
through to OneShot. User expectation is "loop entire sample by
default until note-off."

QUAL-01 criterion as written ("no clicks / zipper / aliasing") is met.
The loop-point editor (DSP-06, complete) provides a per-slot
workaround in v1.0. v1.0 ships as internal-use-only (no public
distribution per Stage 4 D4-3), so per-slot manual override is an
acceptable workaround. Heuristic tuning deferred to v1.1 if real-use
feedback confirms it's a frequent problem.

### v1.1 follow-ups logged in VERIFICATION.md

- **V11-LOOP-FALLBACK** — default loop fallback should loop entire
  sample when LoopDetector variance/headroom gates fail but length
  gate passes (Stage 2.5 owner; v1.1 trigger).
- **V11-PERF-METER** — capture Logic Performance Meter
  `delta_CPU_pct` on a future Logic release / alternative DAW
  (Stage 4 / metrology owner; v1.1 trigger). Deferred from Phase 4.2.
- **V11-MIXED-SR-EXPLICIT** — explicit mixed-SR fixture listening
  pass (Stage 4 / verify owner; v1.1 trigger).

## Stage 4 Sub-stage Status

| Phase | Goal | Gate | Commit | Status |
|---|---|---|---|---|
| 4.1 Version pill | runtime version-pill via `getPluginVersion` | triple build + pluginval-5 + auval | b47434d | ✅ PASS |
| 4.2 PERF-02 | 16-voice CPU budget within spec | methodology-deviation; objective gate-of-record = 4.4 | 3ca88e4 | ✅ PASS (conditional) |
| 4.3 QUAL-01 | listening pass (no clicks / zipper / aliasing) | 7-item subjective checklist | this commit | ✅ PASS (Path A) |
| 4.4 Final gate | pluginval-10 + auval + Logic + Dorico smoke + invariants | strictness-10 SUCCESS, all greens | pending | ⏳ next |

## Previous State: Phase 4.2 PERF-02 closed (methodology deviation)

`/plugin-execute O-MicrotonalSampler 4-polish` Phase 4.2 produced
`.planning/stages/4-polish/{VERIFICATION,PHASE-4.2-SUMMARY,gate-report}.{md,json}`
and flipped `REQUIREMENTS.md` row `PERF-02` from `partial → complete`
(verified at `stage-4`, with the methodology-deviation caveat carried
in the row's `verified at` field for surface-level visibility).

### Phase 4.2 deviation summary

The spec metric (Logic Pro Performance Meter `delta_CPU_pct ≤ 5 %`)
was unmeasurable: Logic 11.x's Performance Meter is not surfaceable
in this user's environment (window restructured / removed; LCD
mini-meter not visible). Path B taken (per pre-execute discuss):

- **Activity Monitor used as supporting headline only.** One run on
  M4 Max laptop on power, 16 voices / 48 kHz / 256: ~16 % of one
  core ≈ ~1 % of total system CPU on the 16-core part. Well below
  the 5 % spec budget at the system level.
- **Objective per-block timing budget = `pluginval --strictness-level
  10` in Phase 4.4** — gate-of-record. Strictness-10 stress includes
  timing constraints + fuzzed parameter sequences, an objective and
  reproducible substitute for the Logic-side metric.
- **Conditional flip.** PERF-02 → `complete` on the basis of (1)
  Activity Monitor headline (2) PERF-01 RT-safety precondition
  (already verified stage-2) and (3) deferred objective gate to 4.4.
  If 4.4 strictness-10 surfaces a timing regression, the flip rolls
  back and Stage 2 sub-phase 2.4 / 2.5 reopens per `PLAN.md
  §Failure Routing`.
- **v1.1 follow-up logged.** Capture Logic-side metric on a future
  Logic release (or alternative DAW with stable per-track meter, e.g.
  Reaper) once one is available.

### Phase 4.2 commit also backfills Stage 4 planning prerequisites

`CONTEXT.md`, `RESEARCH.md`, `PLAN.md` were on-disk-but-untracked
from the discuss/research/plan phases (never landed in their own
commits). VERIFICATION.md and PHASE-4.2-SUMMARY.md reference these
documents (RESEARCH §RQ4-3 in particular), so they're brought into
tree alongside the 4.2 deliverables to keep cross-references live.

## Stage 4 Sub-stage Status

| Phase | Goal | Gate | Commit | Status |
|---|---|---|---|---|
| 4.1 Version pill | runtime version-pill via `getPluginVersion` | triple build + pluginval-5 + auval | b47434d | ✅ PASS |
| 4.2 PERF-02 | 16-voice CPU budget within spec | methodology-deviation; objective gate-of-record = 4.4 | this commit | ✅ PASS (conditional) |
| 4.3 QUAL-01 | listening pass (no clicks / zipper / aliasing) | 7-item subjective checklist | pending | ⏳ next |
| 4.4 Final gate | pluginval-10 + auval + Logic + Dorico smoke + invariants | strictness-10 SUCCESS, all greens | pending | ⏳ |

## Previous State: Phase 2.5 reopen RESOLVED — full chromatic playback verified

Case A (the "only D#3/E3 audible" symptom) was a **two-bug interaction**
surfaced by audit, not the load-pipeline corruption hypothesized in the
charter. Both bugs fixed in this commit; engineering bar green; user
perceptual verification PASS in Standalone against both test folders.

### Bug 1 — `cubicInterp` always-wrap defect (primary cause of silence)

`Source/MicrotonalSamplerVoice.cpp` — the cubic-Hermite interpolator
unconditionally folded read indices into the loop region whenever a slot
had `loopMode = Auto`. Effect: at note-on with `pos = 0`, the four taps
came from `buf[loopEnd-1, loopStart, loopStart+1, loopStart+2]` instead
of `buf[0..3]`. The entire attack `[0, loopStart)` was never played —
the sampler started **inside** the loop region from sample 0.

Combined with `LoopDetector::detectLoop`'s search for the **quietest**
1024-sample window (which is exactly what a loop detector should look
for), every slot whose variance gate passed rendered near-silent through
the ADSR fade-in. Slots whose variance gate rejected (→ `OneShot`) played
correctly because the no-wrap path was already correct.

User-confirmed: every silent cell in the test had `loopMode: Auto`,
every audible cell had `loopMode: OneShot`.

**Fix:** `cubicInterp` is now pure clamp (loopStart/loopEnd parameters
removed). Looping is the protocol of `readSlotWithLoop` (boundary
crossfade) plus `wrapLoopPosition` (cursor reset). Since
`wrapLoopPosition` only fires when `pos >= loopEnd`, the first pass
naturally plays `[0, loopEnd)` (attack + body) and subsequent passes
oscillate in `[loopStart, loopEnd)`.

The crossfade `inSample` math was also corrected — it previously read
the END of the loop region (degenerate, identical to `outSample`); now
reads the loop HEAD at `pos - lpLen + 8 ∈ [lpStart, lpStart + 8)` as
intended.

### Bug 2 — `FilenameParser` pre-note dynamics false-match

`Source/FilenameParser.cpp` — the velocity scan walked all tokens
left-to-right and accepted the first match. Filenames like
`vln_long_mp-D#3-V127-T6N6.aif` matched `mp` (a dynamics token) at
token index 2 — BEFORE the note token at index 3 — and assigned
`velLayer=1` to every slot in the library. Combined with
`numVelocityLayers = jlimit(1, 4, maxLayer + 1) = 2` and
`layerWidth = 64`, this silenced the entire library at MIDI velocities
< 65 (the layer-1 threshold for a 2-layer map).

**Fix:** velocity scan is now two-tier:
- **Tier 1 (post-note):** any velocity form, including dynamics. Handles
  the conventional `[note]_[dyn]` and `[note]_[v_N]` patterns.
- **Tier 2 (pre-note):** explicit forms only (`v[1-4]` / `vel[1-4]` /
  `layer[N]` / `L[N]` / `lyr[N]`). Dynamics letters (`p`/`mp`/`mf`/`f`)
  are skipped here because they collide with instrument-name fragments.

`vln_long_mp-D#3-…` now resolves to `velLayer=0`. Existing pre-note
explicit conventions (`Lyr3_C4`, `L4_C4`, `vel2_C4`) still resolve
correctly. New regression test cases added under `OMTS_UNIT_TESTS`.

### Engineering bar (post-fix, against installed `~/Library/Audio/Plug-Ins/`)

- triple build (VST3 + AU + Standalone): GREEN
- cache-clear + reinstall per CLAUDE.md
- `pluginval --strictness-level 10 --validate-in-process --skip-gui-tests --timeout-ms 120000`: SUCCESS
- `pluginval --strictness-level 10 --validate-in-process --timeout-ms 120000` (with-GUI): SUCCESS
- `auval -v aumu OMtS OuDv`: AU VALIDATION SUCCEEDED

### User perceptual verification (Standalone, fixed-velocity on-screen keyboard)

- `~/Documents/samples/vlnsolo_flaut/` (42 .aif, "Auto Sampled Instrument-…"
  naming, parser-clean velLayer=0): full chromatic G2..C6 audible. ✓
- `~/Documents/samples/vln_long_mp/` (42 .aif, "vln_long_mp-…" naming,
  previously snagged by Bug 2): full chromatic at any velocity now audible. ✓

## Stage-4 resume condition

Both reopens closed. Resume from **Stage 4 Phase 4.2** (PERF-02 Logic Pro
CPU meter measurement) per `.planning/stages/4-polish/PLAN.md`. The
`CASE-A-AUDIT-CHARTER.md` can be archived — no fresh-context deep audit
needed.

## Previous State: Phase 2.1 reopened + rectified mid-Stage-4

While prepping for Phase 4.2 (Logic Pro CPU meter measurement), the
user loaded a real sparse sample folder (`vln_long_mp-A#2-V127-T6N6.aif`
× 43 files) into the fixture-OFF binary and surfaced two Stage 2
defects that the in-memory test fixture had been masking:

- **DEF-2.1-R1 (FUNC-04):** `SampleMap::findSlot` was exact-MIDI-match
  only — the spec's "or nearest if N is unsampled" clause
  (`REQUIREMENTS.md:80`) was never implemented. Sparse-folder keys
  silenced.
- **DEF-2.1-R2 (FUNC-03):** the `polyphony` APVTS parameter was wired
  through APVTS + WebSlider but never read by the audio engine. The
  cap had no effect.

Per the PLAN failure-routing table, Phase 2.1 was reopened. Both
fixes shipped in one atomic commit. Engineering bar green:

- pluginval `--strictness-level 10 --skip-gui-tests`: SUCCESS (21 tests, 0 fail)
- pluginval `--strictness-level 10` with-GUI: SUCCESS (25 tests, 0 fail)
- `auval -v aumu OMtS OuDv`: AU VALIDATION SUCCEEDED

See `.planning/stages/2-dsp/PHASE-2.1-REOPEN-SUMMARY.md` for the full
defect + fix narrative. REQUIREMENTS rows FUNC-03 and FUNC-04 annotated
with `(rectified stage-4 phase-2.1 reopen ...)`.

**Next up — user-side perceptual verification** (closes the reopen
and unblocks Stage 4 Phase 4.2):

- [ ] Single-note coverage across the full range (sparse folder)
- [ ] 16-note chord rings 16 voices; cap of 4 rings 4 with smooth steals
- [ ] Voice-steal ramp inaudible at moderate velocity (D2-3 regression)

If all three pass: resume at Stage 4 Phase 4.2 (PERF-02 Logic Pro
CPU meter run). If any fail: file the defect against the responsible
sub-phase per PLAN failure-routing table.

## Previous State: Stage 4 PLAN complete — ready for execute

`/plugin-plan O-MicrotonalSampler 4-polish` produced
`.planning/stages/4-polish/PLAN.md` with **20 numbered tasks** organised
across the four sub-stages locked in CONTEXT (4.1 version-pill →
4.2 PERF-02 → 4.3 QUAL-01 → 4.4 final gate). Each sub-stage carries an
atomic commit + `gate-report.json` + `PHASE-4.N-SUMMARY.md`, matching
Stage 2/3 cadence.

**Sub-stage breakdown:**

- **4.1 (Tasks 1–4):** insert `getPluginVersion` native function in
  `PluginEditor.cpp` between `getOctaveStretch` (line 127) and
  `getEmbeddedTuningList` (line 137); HTML strips hard-coded `v0.1.0`
  to empty `<div id="about-version">`; JS adds `refreshAboutVersion`
  modeled on `refreshTuningReadout`, called once at JUCE-init alongside
  the existing tuning-readout call. Phase 4.1 gate = triple build +
  cache-clear+install + visual confirmation `v1.0.0` in About pill +
  no-literal grep + pluginval-5 + auval.
- **4.2 (Tasks 5–8):** Logic Pro CPU-meter measurement per RQ4-3
  protocol — Apple Silicon on power, 48 kHz / 256 buffer, 16-voice
  held chord, 3 runs, delta-from-baseline-with-transport, plus
  microtonal-stretch confirmatory read. Acceptance:
  `delta_CPU_pct ≤ 5 %` flips PERF-02 to `complete`.
- **4.3 (Tasks 9–11):** QUAL-01 listening checklist (sustained sine,
  cello vibrato, transient, ±50 c retune sweep, voice-steal stress,
  mixed-SR fixture, short-region loop edge case). All-pass flips
  QUAL-01 to `complete`; any fail reopens Stage 2 sub-phase.
- **4.4 (Tasks 12–20):** final stage gate — clean triple build,
  `pluginval --strictness-level 10 --validate-in-process
  --skip-gui-tests --random-seed 0xC0FFEE --timeout-ms 120000` then
  same with-GUI; auval; Logic AU smoke; Dorico microtonal smoke
  (C4 / ¼♯C4 / C4 / ¼♭C4 with Microtonality="VST3 Note Expression");
  invariant greps (latency-zero, WebView2 flags, no `v0.1.0`); final
  VERIFICATION.md + STATUS.md update; atomic commit.

**Strict order: 4.1 → 4.2 → 4.3 → 4.4.** A failure in 4.2 / 4.3
**reopens** the relevant Stage 2 sub-phase rather than absorbing into
Stage 4. Only 4.4 closes the stage.

**Dependency graph + failure routing** documented in PLAN.md (per-task
defect routes back to Stage 2/3 sub-phase ownership).

**Files modified at execute time:**
`Source/PluginEditor.cpp`, `Resources/ui/index.html`,
`Resources/ui/js/sampler-app.js`, `.planning/REQUIREMENTS.md`,
`.planning/STATUS.md`, `.planning/stages/4-polish/{VERIFICATION,
PHASE-4.{1,2,3,4}-SUMMARY,gate-report}.{md,json}`. **Untouched:**
all Stage 2 audio-thread paths, CMakeLists.txt, modules.json.

## Previous State: Stage 4 RESEARCH complete — ready for plan

`/plugin-research O-MicrotonalSampler 4-polish` produced
`.planning/stages/4-polish/RESEARCH.md`, resolving all four open
questions from CONTEXT (RQ4-1 .. RQ4-4):

- **RQ4-1** — `JucePlugin_VersionString` is a compile-time string-literal
  macro, available via `<JuceHeader.h>`. Sibling Ouaricon plugins
  (O-FreqPulse:215, O-DigiDelay:125, O-Tremolo:122) all use the same
  `withNativeFunction("getPluginVersion", ...)` shape. Pattern to
  mirror verbatim. Build's actual value confirmed `"1.0.0"` from
  `Defs.txt` / `CMakeLists.txt:14`.
- **RQ4-2** — Correct flag is `--strictness-level`, NOT `--strictness`.
  Prior Stage 3 runs were silent strictness-5 fallback (flag
  malformed). Strictness-10 unlocks `FuzzParametersTest` + heavier
  `ParameterThreadSafetyTest` / `BackgroundThreadStateTest`. No prior
  strictness-10 evidence in this codebase — Stage 4 is first run.
  Plan to pin `--random-seed` + `--timeout-ms 120000` for
  reproducibility.
- **RQ4-3** — Logic Pro per-track CPU is not isolated; PERF-02 measures
  as **delta from baseline-with-transport** in the aggregate
  Performance Meter. Apple Silicon must be on power. Specified
  reproducible 8-step protocol with VERIFICATION fields.
- **RQ4-4** — Dorico smoke procedure: 11-step manual UI configuration
  (no `.doricoexpmap` distribution needed for smoke). Critical step:
  duplicate Default expression map and set Microtonality to **"VST3
  Note Expression"** — Dorico ignores `INoteExpressionController` and
  Auto-mode silently routes to pitch-bend. Test passage:
  C4 / ¼♯C4 / C4 / ¼♭C4 quarter-tone alternation.

**No new module dependencies.** Eight invariants/pitfalls carried
forward into PLAN.

## Previous State: Stage 4 DISCUSS complete — ready for research

`/plugin-discuss O-MicrotonalSampler 4-polish` produced
`.planning/stages/4-polish/CONTEXT.md` with **7 locked decisions
(D4-1..D4-7)** and a **provisional 4-sub-stage plan** (4.1 version
plumbing → 4.2 PERF-02 benchmark → 4.3 QUAL-01 listening → 4.4 final
gate).

**Stage 4 scope (intentionally narrow):**
- Close PERF-02 (16-voice ≤ 5 % CPU) and QUAL-01 (no artifacts) — both
  carry `partial` from Stage 2.
- Plumb dynamic version pill via `getPluginVersion()` native function
  (replaces hard-coded `v0.1.0` in About tab).
- Final gate: pluginval `--strictness 10` + auval + Logic + Dorico
  smoke. macOS-only (VST3 + AU + Standalone). Internal use; no signing,
  no installer, no public release.

**Out of v1.0:** preset system, installers, Windows build, per-slot
xfade, octave grouping, render-harness target.

**4 open questions (RQ4-1..RQ4-4)** pending research:
- JucePlugin_VersionString macro source / runtime accessor
- pluginval `--strictness 10` delta vs strictness-5 for WebView editors
- Logic CPU-meter measurement protocol (per-track vs delta)
- Dorico smoke procedure (Playback Template / endpoint mapping)

## Previous State: Stage 3 VERIFIED

`/plugin-verify O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/VERIFICATION.md`. All five Stage 3 requirements
(FUNC-05, FUNC-06, DSP-06, UI-01, UI-02) marked **complete** in
`REQUIREMENTS.md`. All five sub-stage gates green; all 11 Phase 3.5 gate
criteria green; Stage 2 audio invariant intact end-to-end.

Stage 3 (GUI) is closed.

## Previous State: Phase 3.5 GATE PASS — Stage 3 EXECUTE COMPLETE

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.5 produced
`.planning/stages/3-gui/PHASE-3.5-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.5). Tasks 29–34 implemented. **All five Stage 3
sub-stage gates green.**

**Phase 3.5 deliverables:**
- Bottom control strip rebuilt as 7 SVG arc-knobs (44 px, 270 deg sweep,
  antique-gold vine, rosewood track) — lifted from O-Bells `#effects-tab .knob`
  ruleset. Each knob wraps a hidden `<input type="range">` so the existing
  WebSliderRelay binding (Phase 3.1) is preserved verbatim — relay attaches by
  element id, not DOM hierarchy. Order left→right: Attack · Decay · Sustain ·
  Release · Polyphony · Vel-XF · Out Gain.
- `KNOB_FORMATS` table maps each relay to per-parameter display range + unit
  suffix + formatter (Attack/Decay/Release in seconds with adaptive precision,
  Sustain/Vel-XF unitless 0..1, Polyphony integer-rounded, Out Gain in dB
  with sign).
- Pointer drag = relative-vertical (200 px = full sweep, sliderDragStarted/Ended
  bracketing); wheel = 2 % per tick; dblclick = snap to mid (Stage 4 will plumb
  parameter defaults explicitly).
- Tuning-state readout in chrome (`<span id="tuning-readout">`) already present
  from Phase 3.1; verified poll cadence honours RP3-3 (editor open +
  Tuning-tab activation only, no background interval).
- About tab populated (RP3-4): `.about-card` with plugin name (Garamond
  serif heading), version pill `v0.1.0` (hard-coded; Stage 4 will plumb
  dynamically from CMakeLists.txt PLUGIN_VERSION), tagline "Microtonal
  sample engine for Dorico microtonal playback", short blurb, Ouaricon
  license link (`https://ouaricon.com`).
- Aesthetic polish: 8/16/24 px spacing scale enforced via `--gap-sm/md/lg`
  CSS vars; hover states on cells, buttons, knobs, tabs, links; warm-card
  shadows + `--border-warm` border treatments matching O-Bells convention;
  Garamond serif for headings + system sans for numeric readouts.
- Narrow-window guard: `checkNarrowWindowGuard` auto-closes the loop editor
  + toasts "Resize wider to use the loop editor." when window width crosses
  below 900 px with the panel open. ResizeObserver-driven; one-shot per
  bucket transition (no spam).

**Gate (Tasks 33 + 34):**
- Triple build (VST3 + AU + Standalone) GREEN.
- Cache-clear + reinstall per CLAUDE.md.
- `pluginval --strictness 5 --validate-in-process --skip-gui-tests`: **SUCCESS**.
- `pluginval --strictness 5 --validate-in-process` (with GUI tests): **SUCCESS**.
- `auval -v aumu OMtS OuDv`: **AU VALIDATION SUCCEEDED**.
- Latency invariant: `grep -rn setLatencySamples plugins/O-MicrotonalSampler/Source/`
  returns one comment-only hit (PluginProcessor.cpp:133) — no actual calls.
  Stage 2 latency-zero contract preserved end-to-end across Stage 3.

## Previous State: Phase 3.4 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.4 produced
`.planning/stages/3-gui/PHASE-3.4-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.4). Tasks 23–28 implemented.

**Phase 3.4 deliverables:**
- `OMicrotonalSamplerAudioProcessor::overrideLoopPoints` full impl
  (atomic deep-copy via `std::make_shared<SampleMap>(*current)` + slot
  mutation + version bump + callback). Manual override sets
  `LoopMode::Manual`; `resetToAutoDetect=true` re-runs
  `LoopDetector::detectLoop` and writes `Auto` (valid) or `OneShot`
  (invalid).
- New `OMicrotonalSamplerAudioProcessor::resetLoopToAutoDetect(midi, vel)`
  convenience wrapper.
- `OMicrotonalSamplerAudioProcessor::snapshotWaveformPeaks` full impl —
  per-bin min/max scan over slot audio, `framesPerBin = numFrames/bins`,
  sum-of-channels mixdown; emits `juce::DynamicObject` with peaks +
  meta per RESEARCH §RQ3-5 schema (midiNote, velocityLayer,
  lengthSamples, sourceSampleRate, loopStart, loopEnd, loopMode,
  filename, peaks). Single-pass O(N), ≈1 ms / 5 s sample at 48 kHz.
- Three native function skeletons replaced with full impls in
  `PluginEditor.cpp`: `getWaveformPeaks(midi, vel, bins=512)`,
  `overrideLoopPoints(midi, vel, start, end, xfade=8)`,
  `resetLoopToAutoDetect(midi, vel)`.
- `Resources/ui/index.html` — `#loop-editor-panel` populated (header
  with filename · MIDI · L<vel> + close X, canvas wrap, meta row,
  Reset/Cancel/Apply actions).
- `Resources/ui/css/sampler-shell.css` — full panel slide-in (350 ms
  ease, `body.le-open` grid reflow with `padding-right: calc(...
  + 360px)`); canvas sized via `width: calc(100% - 0px); height: 200px`
  (memory pitfall #6 — never `position: absolute` with `left+right`).
- `Resources/ui/js/sampler-app.js` — loop-editor module:
  `openLoopEditor(midi, vel)` async fetch + render; `redrawLoopEditor`
  with DPR-aware backing store + warm-brown stroke + antique-gold fill
  envelope + draggable markers (gold start, rust-red end, 8-px
  hit-tolerance, 16-sample min gap); pointer-event drag with
  `setPointerCapture`; Apply emits toast `"New loop points apply to
  next note-on."` (EC3-6); Reset disabled + tooltip when one-shot
  (EC3-7); Esc/X/Cancel close; `ResizeObserver` re-renders on canvas
  size change.
- `handleCellSingleClick` for cell-loaded now calls `openLoopEditor`
  (replaces Phase 3.2 placeholder); context menu open-loop-editor
  routes to same.
- `handleSampleMapSnapshot` syncs editor state when open + not
  mid-drag so loop-mode label stays consistent after Apply.

**Gate:** triple build green, cache-clear+install per CLAUDE.md, pluginval --strictness 5 SUCCESS, auval AU VALIDATION SUCCEEDED.

## Previous State: Phase 3.3 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.3 produced
`.planning/stages/3-gui/PHASE-3.3-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.3). Tasks 19–22 implemented.

**Phase 3.3 deliverables:**
- `OMicrotonalSamplerAudioProcessorEditor::filesDropped` full hit-test + routing
  (cell + audio file → `loadSingleSample`; folder-zone + folder → `loadSampleFolder`;
  invalid combinations → toast hints; out-of-bounds → silent reject; EC3-3 folder-on-cell disallowed)
- `fileDragEnter/Move/Exit` emit `hostFileDragMove({x,y})` / `hostFileDragExit({})`
  events for JS hover visuals
- `loadSampleFolderDialog` native function full impl (FileChooser canSelectDirectories
  → `processorRef.loadSampleFolder`)
- JS `bindHostDragEvents` toggles `.drag-over` class on `#folder-drop-zone` based on (x,y) ∈ rect
- Folder-button enabled (was disabled placeholder in 3.1) — click → `loadSampleFolderDialog`
- 3-second single-element toast queue (`showToast` + backend `toast` event subscription)
- Skipped-files disclosure: `<ul id="issues-list">` rendered from `snapshot.skippedFiles`
  on every `sampleMapUpdated`; transition-tracked toast `"N files skipped"` on set change
- CSS `.drag-over` glow strengthened with inset box-shadow

**Gate:** triple build green, cache-clear+install per CLAUDE.md, pluginval --strictness 5 SUCCESS, auval AU VALIDATION SUCCEEDED.

## Previous State: Phase 3.2 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.2 produced
`.planning/stages/3-gui/PHASE-3.2-SUMMARY.md` and overwrote
`gate-report.json` (phase 3.2). Tasks 12–18 implemented.

**Phase 3.2 deliverables:**
- `SampleLoader::loadSingleSlot` worker (SR-convert + loop-detect + async completion)
- `OMicrotonalSamplerAudioProcessor::loadSingleSample` full impl (atomic deep-copy + version bump + callback)
- `loadSingleSampleDialog` native function (FileChooser launch)
- `renderGrid` JS — 88×4 CSS grid, cell-loaded/empty/loading classes
- Cell interactions (RP3-1): single-click empty → FileChooser; single-click loaded → loop-editor placeholder; double-click loaded → replace; right-click → context menu
- 250 ms double-click discrimination
- `publishCellLayout` (ResizeObserver + rAF-throttled) → `reportCellLayout` native function

**Gate:** triple build green, cache-clear+install per CLAUDE.md, pluginval --strictness 5 SUCCESS, auval AU VALIDATION SUCCEEDED.

## Previous State: Phase 3.1 GATE PASS

`/plugin-execute O-MicrotonalSampler 3-gui` Phase 3.1 produced
`.planning/stages/3-gui/PHASE-3.1-SUMMARY.md` and `gate-report.json`.

**Phase 3.1 Foundation delivers:**

- WebView shell replaces the Phase 2.2 placeholder editor wholesale.
- 7 APVTS sliders (`attack`, `decay`, `sustain`, `release`, `polyphony`,
  `velocity_crossfade`, `output_gain`) bound via `WebSliderRelay` +
  `WebSliderParameterAttachment` in correct destruction order.
- Tabbed UI (Sample Map / Tuning / About) with read-only TuningPanel
  (verbatim O-Bells carry + readonly CSS overlay + interval-input → span
  swap shim per RESEARCH §RQ3-1).
- 8 fully-implemented native functions (`getSampleMap`, `getTuningName`,
  `getTuningIntervals`, `getTonicNote`, `getOctaveStretch`,
  `getEmbeddedTuningList`, `getEmbeddedTuningCategories`, `reportCellLayout`,
  `getSkippedFiles`) + 6 skeletons returning sane defaults
  (`loadSampleFolderDialog`, `loadSingleSampleDialog`, `overrideLoopPoints`,
  `resetLoopToAutoDetect`, `getWaveformPeaks` for 3.2/3.3/3.4).
- `sampleMapUpdated` event scaffold: processor's
  `setSampleMapChangedCallback` lambda emits the JSON snapshot whenever the
  sample map atomic-stores; editor wires up the lambda on construction.
- Cross-platform WebView2 compliance: `NEEDS_WEBVIEW2 TRUE` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder` +
  resource provider URL=path equality.
- Stage 2 invariant addition (per RESEARCH §RQ3-3): `SampleSlot::audio` →
  `std::shared_ptr<juce::AudioBuffer<float>>`; `SampleSlot::filename`;
  `LoopMode` enum + `SampleSlot::loopMode`; `SampleMap::version` monotonic
  counter.

**Stage 2 regression gate (Task 4):** `pluginval --strictness 5
--validate-in-process --skip-gui-tests` SUCCESS + `auval -v aumu OMtS OuDv`
AU VALIDATION SUCCEEDED on the post-shared_ptr-swap build. No render-harness
existed; coverage substituted by pluginval+auval per gate-report advisory.

**Phase 3.1 gate (Task 11):** Triple build green. Cache-clear + install per
CLAUDE.md. pluginval SUCCESS. auval SUCCEEDED. Atomic commit recipe
documented in PHASE-3.1-SUMMARY.md.

## Stage 3 Sub-stage Status

| Phase | Goal | Gate | Commit | Status |
|---|---|---|---|---|
| 3.1 Foundation | WebView shell + Stage 2 invariant + relays + JSON broadcast | infra | d1a0d7a | ✅ PASS |
| 3.2 Grid | FUNC-06, UI-01 | grid in <100 ms; per-cell replace | 4083582 | ✅ PASS |
| 3.3 Folder Drop | FUNC-05 | drop = button parity; skipped files surface | aa99790 | ✅ PASS |
| 3.4 Loop Editor | DSP-06, UI-02 | edit → audible diff on next note-on | d7cfd29 | ✅ PASS |
| 3.5 Polish | (visual) | aesthetic + final pluginval gate | pending atomic commit | ✅ Code + automated gate green |

## Previous State: Stage 3 (GUI) PLAN complete

`/plugin-plan O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/PLAN.md` with **34 numbered tasks** organized
across 5 sub-stages (3.1 Foundation → 3.2 Grid → 3.3 Folder Drop →
3.4 Loop Editor → 3.5 Polish), each with its own atomic-commit gate.

**Plan-phase resolutions (open questions RP3-1..RP3-5):**

- **RP3-1** Cell interactions: single-click loaded cell → loop editor;
  double-click → replace via FileChooser; right-click → context menu;
  single-click empty cell → FileChooser.
- **RP3-2** Crossfade-length stays global (Phase 2.5 constant) for
  v1.0; per-slot xfade is a v1.1 candidate.
- **RP3-3** Tuning-state readout polls on Tuning-tab activation +
  editor open only (no background interval).
- **RP3-4** About tab: empty in 3.1; minimal version + license link
  in 3.5.
- **RP3-5** Narrow-window grid: horizontal scroll when min cell width
  (8 px) is hit; no octave grouping in v1.0.

**Critical sequencing note:** Phase 3.1 includes a Stage 2 invariant
addition (`SampleSlot::audio` → `std::shared_ptr<juce::AudioBuffer<float>>`).
Task 4 blocks on a full Stage 2 verification gate (pluginval, auval,
render-harness identity test) before proceeding to editor work — any
regression reopens Stage 2 rather than being absorbed into 3.1.

## Previous State: Stage 3 (GUI) RESEARCH complete

`/plugin-research O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/RESEARCH.md` resolving all 8 research questions
(RQ3-1..RQ3-8). Key resolutions:

- **TuningPanel readonly mode** (RQ3-1): carry verbatim suite copy + CSS
  overlay + register only read-side native functions.
- **SampleMap JSON schema** (RQ3-2): version-stamped snapshot with per-slot
  filename/length/SR/loopStart/loopEnd/loopMode + skippedFiles array.
- **Per-cell loader** (RQ3-3): new `loadSingleSample(midi, vel, file)` —
  requires Stage 2 invariant addition `SampleSlot::audio` →
  `std::shared_ptr<juce::AudioBuffer<float>>` to keep map deep-copy cheap
  on per-cell replace. Land in 3.1.
- **Loop-override** (RQ3-4): `overrideLoopPoints(midi, vel, start, end,
  xfade)` on message thread, atomic shared_ptr replace, snapshot
  rebroadcast. Voices keep their own snapshot for active notes.
- **Waveform render** (RQ3-5): pre-render 512-bin peak summary on message
  thread, broadcast via `emitEventIfBrowserIsVisible("waveformPeaks", ...)`,
  JS draws on DPR-aware canvas.
- **Cell DnD** (RQ3-6): `juce::FileDragAndDropTarget` on host editor +
  C++-side cell-layout shadow published by JS via `reportCellLayout`
  native function. No reliance on HTML5 `dataTransfer.files` paths.
- **Aesthetic** (RQ3-7): pull palette/typography from O-Bells inline
  styles. Garamond serif, cream parchment + warm-brown + antique-gold +
  rust-red active. Botanical motif deferred to 3.5 polish.
- **Resource bundling** (RQ3-8): `juce_add_binary_data` baked, served via
  resource provider — matches O-Bells.

Stage 3 verifies 5 requirements: FUNC-05, FUNC-06, DSP-06, UI-01, UI-02.

Native function inventory: ~13 (`getSampleMap`, `loadSingleSampleDialog`,
`overrideLoopPoints`, `getWaveformPeaks`, `reportCellLayout`,
`getTuning*` reads, etc.).

Open RP3-1..RP3-5 for plan phase to resolve (single-click cell behavior,
crossfade-len global vs per-slot, tuning-readout polling cadence,
About-tab content, narrow-window cell clamp).

## Previous State: Stage 3 (GUI) DISCUSS complete

`/plugin-discuss O-MicrotonalSampler 3-gui` produced
`.planning/stages/3-gui/CONTEXT.md` with 15 locked decisions (D3-1..D3-15)
and 5 sub-stages (3.1 shell+tabs+TuningPanel → 3.2 sample-map grid →
3.3 folder-drop + skipped-files → 3.4 loop-point editor → 3.5 control
strip + aesthetic polish). 8 research questions resolved in RESEARCH.md.

**Key decisions:** WebView UI (D3-1), Ouaricon house aesthetic (D3-2),
no separate `/ui-mockup` pass (D3-3 — design specified in prose),
tabbed layout with TuningPanel as its own tab (D3-4 + D3-7 — copy-paste
the suite tuning-panel.{js,css} per O-Bells pattern), horizontal piano
strip × 4 vel-layer rows (D3-5), loop editor as side panel inside the
Sample Map tab (D3-6), 7 APVTS relays + custom `sampleMap` JSON relay
(D3-11). Cross-platform WebView2 flags from memory are mandatory.

Stage 3 verifies 5 requirements: FUNC-05, FUNC-06, DSP-06, UI-01, UI-02.

## Previous State: Stage 2 (DSP) VERIFIED

`/plugin-verify O-MicrotonalSampler 2-dsp` ran goal-backward analysis against
CONTEXT.md / PLAN.md / 5×PHASE-N-SUMMARY.md, walked all 15 in-scope requirements,
and re-ran the automated bar (triple build green; cache-clear + fresh install;
`pluginval --strictness 5 --validate-in-process --skip-gui-tests` SUCCESS;
`auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED).

**Verdict:** ✅ VERIFIED — 13 requirements complete (FUNC-01..04, FUNC-07,
DSP-01..05, DSP-07, DSP-08, PERF-01, PERF-03, PERF-04, COMPAT-02), 2 marked
partial pending the user's subjective DAW pass (PERF-02 CPU benchmark; QUAL-01
listening test). All engineering mitigations for the partials are in place;
they remain open only because they require a human listener / metering step.

See `.planning/stages/2-dsp/VERIFICATION.md` for the full evidence table and
the deferred Human Verification checklist.

**Phase 2.5 commit still pending.** The Phase 2.5 source changes
(`LoopDetector.{h,cpp}`, modified `MicrotonalSamplerVoice.{h,cpp}`,
`SampleLoader.cpp`, `CMakeLists.txt`) plus the new verify artefacts (this
file, `REQUIREMENTS.md` updates, `VERIFICATION.md`) ride in a single atomic
commit per the recipe in `VERIFICATION.md` Outstanding Actions §1.

## Stage 2 Sub-stage Status

| Phase | Gate | Commit | Status |
|---|---|---|---|
| 2.1 Voice DSP | 1 | `bb0e7f7` | ✅ PASS |
| 2.2 Loader | 2 | `cacffda` | ✅ PASS |
| 2.3 Vel xfade | 3 | `11bd39c` | ✅ PASS |
| 2.4 Voice-steal | 4 | `1aceb4c` | ✅ PASS |
| 2.5 Loop detect | 5 | pending atomic commit | ✅ Code + automated gate green |

## Completed So Far

**Ideation:** ✓ Complete
**Stage 1 (Foundation):** ✓ Verified — silent shell builds + AU/VST3/Standalone validate
**Stage 2 Discuss:** ✓ Complete (CONTEXT.md, 2026-04-27)
**Stage 2 Research:** ✓ Complete (RESEARCH.md, 2026-04-27)
**Stage 2 Plan:** ✓ Complete (PLAN.md, 2026-04-27)
**Stage 2 Execute:** ✓ All 5 sub-stages code-complete; 4 committed, 5th pending atomic commit
**Stage 2 Verify:** ✓ VERIFIED (VERIFICATION.md, 2026-04-27)

## Stage 2 Locked Decisions (D2-1..D2-12)

- **D2-1 Interpolator:** Cubic-Hermite (4-pt). Conditional 1st-order tilt LPF NOT added (Phase 2.1 sine-sweep null test landed below threshold).
- **D2-2 Voice-steal:** JUCE default `findVoiceToSteal` already implements oldest-released → oldest-keyup → oldest-non-protected (R1; no override).
- **D2-3 Steal ramp:** 5 ms linear (`ceil(0.005·SR)+16` samples).
- **D2-4 Loop auto-detect:** RMS scan + zc snap + 8-sample equal-power xfade; one-shot fallback on variance / length / headroom failures.
- **D2-5 ADSR:** `juce::ADSR` (linear segments).
- **D2-6 Sub-stage order:** 2.1 → 2.2 → 2.3 → 2.4 → 2.5 (all complete).
- **D2-7 Filename parser:** Tolerant; case-insensitive; multi-convention.
- **D2-8 Out-of-range notes:** Silence.
- **D2-9 SR conversion:** `juce::LagrangeInterpolator` per channel at load time.
- **D2-10 Mono → stereo:** Duplicate L/R at unity gain.
- **D2-11 Smoothing:** `output_gain` smoothed via `juce::SmoothedValue` + `applyGainRamp`. `velocity_crossfade` consumed once per startNote (no SmoothedValue needed).
- **D2-12 NE granularity:** Once at `startNote()`.

## Files Created/Modified (Stage 2)

`Source/MicrotonalSamplerVoice.{h,cpp}`,
`Source/SampleMap.h` (`findSlot` linear scan),
`Source/SampleLoader.{h,cpp}` (full implementation),
`Source/FilenameParser.{h,cpp}` (new, Phase 2.2),
`Source/LoopDetector.{h,cpp}` (new, Phase 2.5),
`Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`,
`Source/tests/aliasing_check.cpp` (RQ-1 driver, EXCLUDE_FROM_ALL),
`plugins/O-MicrotonalSampler/CMakeLists.txt`,
`.planning/stages/2-dsp/CONTEXT.md`, `RESEARCH.md`, `PLAN.md`,
`PHASE-2.{1,2,3,4,5}-SUMMARY.md`, `VERIFICATION.md`,
`.planning/STATUS.md`, `.planning/REQUIREMENTS.md`.

## Outstanding Actions (post-verify)

1. **User commits Phase 2.5 + verify artefacts** — atomic commit per recipe in
   `VERIFICATION.md` Outstanding Actions §1.
2. **Subjective DAW pass** (Human Verification checklist in
   `VERIFICATION.md`) — sustained sine, vibrato cello, transient fallback,
   short-region edge case, regression suite re-run, +50 c retune listening
   test, mixed-SR fixture.
3. **CPU benchmark (PERF-02)** — 16 sustained voices, 48 kHz / 256 buffer,
   Apple Silicon, looping samples. Logic CPU meter or `pluginval
   --benchmark`. Confirm ≤ 5 %.

If any subjective check fails, file a defect and reopen the relevant
sub-phase rather than advancing to Stage 3.

## Next Steps

1. **Atomic commit** of Phase 2.5 + Stage 2 verify (recipe in
   `VERIFICATION.md`) — still outstanding.
2. **Stage 3 plan** — `/plugin-plan O-MicrotonalSampler 3-gui` to break
   3.1–3.5 into ordered tasks with gate-reports.
