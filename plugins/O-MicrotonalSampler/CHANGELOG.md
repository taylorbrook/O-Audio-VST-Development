# O-MicrotonalSampler Changelog

## [1.16.7] - 2026-05-05

### Changed
- **Code simplification — Phase 1 (audit candidates HIGH-01/03/04/05).** Pure helper
  extraction; no behaviour change.
  - `PluginProcessor.cpp`: collapsed 12 `__cpp_lib_atomic_shared_ptr` `#if/#else`
    blocks behind file-local `atomicLoad` / `atomicStore` template helpers. The
    site at lines 510–516 was a dead conditional (both arms identical). The
    deprecated free-function `std::atomic_load`/`atomic_store` overloads are
    still the underlying implementation.
  - `PluginEditor.cpp`: extracted three anonymous-namespace helpers:
    - `buildNotesFreqsJson(notes, freqs)` — replaces 2 duplicated blocks emitting
      `{"notes":[…],"freqs":[…]}` with 4-digit freq precision (held-notes
      broadcast, `getHeldNotesJson` native fn).
    - `centsArrayToJson(cents)` — replaces 4 duplicated 6-digit-precision JSON
      builders in `getTuningIntervals`, `generateEDO`, `generateHarmonicSeries`,
      `generateRank2`.
    - `setBoolParamFromArgs(apvts, paramId, args)` — replaces 3 copies of the
      bool-arg `setValueNotifyingHost` pattern in `setKeyswitchEnabled`,
      `setCcEnabled`, `setPcEnabled`. Public contract unchanged
      (`complete(false)` on bad args / missing param, `complete(true)` on
      success).
  - Net diff: −54 LOC (167 deletions, 113 insertions including helper bodies +
    explanatory comments).

### Verification
- All four extractions are pure mechanical refactors. Output formats (JSON
  shape, freq/cents precision, callback ordering) are byte-identical to
  v1.16.6.
- Items on the audit's "Skipped (false-positive checks)" list — drag-drop
  streaming, resource-provider URL handling, `Juce` vs `window.__JUCE__`,
  microtonal top-level exp-map fields, RR/voice DSP, WebView2 guards — were
  not touched.
- Audit report: `plugins/O-MicrotonalSampler/.planning/SIMPLIFICATION-AUDIT.md`.

### Note
- Missing v1.16.6 changelog entry. Commit `4883886` shipped v1.16.6 ("restore
  exp-map top-level microtonal fields — TC-4 regression fix") but no CHANGELOG
  entry was written. Backfill on next pass.

## [1.16.5] - 2026-05-05

### Fixed
- **TC-2: Family-aware Dorico Playback Template auto-routing now
  actually works** (was broken in v1.16.3, partially diagnosed in
  v1.16.4). Validated in Dorico 6 with a 4-stave project (Solo Violin,
  Flute, Trumpet, Marimba): apply emits zero "Can't find a template
  spec…" warnings, four `Loading Plugin into slot` events fire
  back-to-back, MIDI thru routes to four distinct endpoint slots
  (1024 / 2048 / 3072 / 4096). Each of the four endpoint configs now declares an
  `<instruments array="true">` block enumerating the canonical instrument
  entityIDs from
  `/Applications/Dorico 6.app/Contents/Resources/instrumentFamiliesDefinitions.xml`:

  | Endpoint config | Family | Instrument IDs |
  |---|---|---|
  | `O-MicrotonalSampler` | Strings (`instrument.strings.*`) | 19 |
  | `O-MicrotonalSampler-Brass` | Brass (`instrument.brass.*`) | 100 |
  | `O-MicrotonalSampler-Winds` | Woodwinds (`instrument.wind.*`) | 84 |
  | `O-MicrotonalSampler-Generic` | Pitched-perc, unpitched-perc, keyboards, singers, fretted, orff, electronics, gamelan, sketch | 345 |

  Each `<instrumentData>` is a fixed shape:
  ```xml
  <instrumentData>
      <entityID>instrument.strings.violin</entityID>
      <index>0</index>
      <irvIndex>0</irvIndex>
      <playerType>kSoloPlayer</playerType>
      <endpoints>0</endpoints>
  </instrumentData>
  ```
  `endpoints>0` points at slot 0 in the same endpoint config (each
  endpoint config still has exactly one `<slotData>` defining a single
  `O-MicrotonalSampler-dev` instance template — Dorico instantiates
  a fresh plugin per stave from that template).

  Aliases (entityIDs containing `.alias.`) are excluded from the
  enumeration; Dorico resolves alias → canonical at score-load time.

  **Architectural correction.** The v1.16.3 design assumed
  `<instrumentFamilies>` in `playbacktemplatespec.xml` was the routing
  filter. It isn't — it's a vestigial / editor-only field. The actual
  routing filter is `<instruments>` at the endpoint-config level (this
  matches the Ample China user template's pattern: empty
  `<instrumentFamilies/>` in spec, ~11 `<instrumentData>` entries in the
  endpoint config). v1.16.4's entityID-format spec change was harmless
  but didn't fix routing on its own.

  **Why this was hard to spot:**
  - The Dorico binary clusters `endpoints / configID / slotData /
    instrumentData / genSpecID / playbackTemplateSpecID` as the routing
    schema — `instrumentFamilies` lives only in the editor UI strings
    (`InstrumentFamiliesEditor.qml`).
  - The `<instrumentFamilies>` element parses without error when present
    (so v1.16.3 ingest-test passed) but nothing reads it for routing.
  - No factory `playbacktemplatespec.xml` ships with populated family
    filters; only stage-template files use `<id>instrument family.X</id>`.

  **Spec file (`playbacktemplatespec.xml`) is unchanged from v1.16.4** —
  the entityID format (`instrument family.strings`) stays even though
  it's now known to be vestigial, because:
  1. v1.16.4 already shipped the format internally,
  2. Dorico parses it cleanly,
  3. If a future Dorico version starts honoring the field, the values
     are correct.

### Documentation
- `Resources/dorico/INSTALL-DORICO.md` — "Multi-family routing" section
  rewritten to document the endpoint-config-level enumeration mechanism.
- `Resources/dorico/SMOKE-TEST.md` — TC-2/TC-3 troubleshooting note
  rewritten; references the new endpoint-config schema instead of the
  spec-level filter.

### Risk envelope
- The `<instruments array="true">` block schema is **proven to work** in
  the working Ample China user template (single-vendor case, 11 custom
  user instruments). It is **inferred to extend** to factory instrument
  IDs (`instrument.strings.violin` etc.) from the binary symbol cluster
  and the matching `instrument family.X / instrument.X.Y` ID-space
  convention. The first end-to-end TC-2 pass confirms the inference;
  the user is asked to run the smoke test in Dorico after re-install.
- The Generic endpoint enumerates 345 instrument IDs across 9 families.
  If Dorico has a routing precedence rule we haven't observed (e.g.
  "first matching entry wins" vs "most specific family wins"), Generic
  may shadow Brass/Winds/Strings on overlapping IDs. We've kept the
  4-entry order Strings → Winds → Brass → Generic in the spec to favor
  family-specific entries first. No overlap in the IDs themselves
  (each instrument is in exactly one family).
- Backup at `backups/O-MicrotonalSampler/v1.16.3/` for one-step
  rollback (v1.16.4 was uncommitted; v1.16.5 supersedes it before any
  git tag was placed).

## [1.16.4] - 2026-05-04

> **Diagnostic step, not a complete fix.** v1.16.4 corrected the
> `<instrumentFamilies>` text content from C++ SDK enum names to Dorico's
> textual entityIDs. In-Dorico smoke test (post-deploy) still showed the
> same 4× "Can't find a template spec or endpoint config…" warnings —
> revealing that `<instrumentFamilies>` is a vestigial/editor-only field
> and the real routing filter lives at endpoint-config level. v1.16.5
> ships that real fix. v1.16.4 was never committed or tagged; it's
> documented here for the diagnostic record.

### Fixed (partial — see v1.16.5 for full TC-2 fix)
- **`<instrumentFamilies>` entityID format corrected.** The v1.16.3 spec
  used C++ SDK enum names (`kStrings`, `kWoodwinds`, `kBrass`) which
  Dorico's XML matcher does not resolve. Replaced with the textual entityIDs Dorico actually registers:

  | Was (v1.16.3, broken) | Now (v1.16.4) |
  |---|---|
  | `<instrumentFamilies>kStrings</instrumentFamilies>` | `<instrumentFamilies>instrument family.strings</instrumentFamilies>` |
  | `<instrumentFamilies>kWoodwinds</instrumentFamilies>` | `<instrumentFamilies>instrument family.woodwinds</instrumentFamilies>` |
  | `<instrumentFamilies>kBrass</instrumentFamilies>` | `<instrumentFamilies>instrument family.brass</instrumentFamilies>` |

  Single change, single file (`playbacktemplatespec.xml` lines 12, 19, 26).
  Entry #4 with empty `<instrumentFamilies/>` stays as the Generic fallback.

  **Root cause / evidence:**
  - `/Applications/Dorico 6.app/Contents/Resources/instrumentFamiliesDefinitions.xml`
    registers `<entityID>instrument family.brass</entityID>`,
    `instrument family.strings`, `instrument family.woodwinds`. There is no
    `kStrings` / `kBrass` / `kWoodwinds` ID anywhere in the Dorico install.
  - `/Applications/Dorico 6.app/Contents/Resources/playback/StageTemplates/SmallJazz/stagetemplate.xml`
    references the same family IDs as `<id>instrument family.strings</id>`
    in a different schema (stage layout) — same ID space, confirming the
    textual format is what Dorico parses.
  - The v1.16.3 `application.log` showed clean ingest of the spec and all
    four endpoint configs, then 4× `Can't find a template spec or endpoint
    config for routing this instrument` on apply (one per stave: Solo
    Violin, Flute, Trumpet, Marimba). Consistent with the matcher iterating
    entries 1–3, failing to resolve `kStrings`/`kWoodwinds`/`kBrass` to any
    registered family entity, and either skipping those entries or
    aborting the entries-array walk before reaching entry #4 (which has
    `<instrumentFamilies/>` and should match Marimba as Generic — but did
    not in v1.16.3 testing).

  **No source / build / DSP / UI / state changes.** Pure resource fix in
  `Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml`.
  All four expression maps, the C++ label patch, the keyswitch routing,
  and the dynamics/microtonal paths from v1.16.3 are unchanged and still
  load-bearing.

### Documentation
- `Resources/dorico/SMOKE-TEST.md` — TC-2 and TC-3 troubleshooting notes
  updated to reference the entityID format and point at
  `instrumentFamiliesDefinitions.xml` for the canonical list.
- `Resources/dorico/INSTALL-DORICO.md` — "Multi-family routing" table and
  "Wrong family routing" troubleshooting entry updated to use
  `instrument family.strings` etc. instead of `kStrings`.
- `CHANGELOG.md` v1.16.3 "Risk envelope" annotated with what was actually
  validated post-ship vs what failed.

### Risk envelope
The entityID format `instrument family.strings` is proven to exist in
Dorico's family-definition layer and proven to be referenced from at least
one other schema (`stagetemplate.xml`). It is **unproven** as a filter
value inside `<entry><instrumentFamilies>` in `playbacktemplatespec.xml`
specifically — no factory `playbacktemplatespec.xml` ships with populated
family filters; the closest analog is the working "Ample China" user
template, which has empty `<instrumentFamilies/>` on every entry.

If TC-2 still fails with "Can't find a template spec or endpoint config…"
warnings after this patch, the next escalation paths are:
  1. **Array-of-children variant.** Replace each
     `<instrumentFamilies>instrument family.X</instrumentFamilies>` with
     `<instrumentFamilies array="true"><entityID>instrument family.X</entityID></instrumentFamilies>`
     (matching the Steinberg pattern used by the surrounding `<entries
     array="true">` wrapper).
  2. **Single-entry fallback.** Collapse to one entry binding the Strings
     endpoint config (no family filter); accept that all staves get the
     Strings exp-map by default and document manual binding via
     Library → Expression Maps for Brass / Winds / Generic.

Backup at `backups/O-MicrotonalSampler/v1.16.3/` for one-step rollback.

## [1.16.3] - 2026-05-04

### Added
- **Multi-family Dorico routing.** The Playback Template now ships four
  expression maps (Strings, Winds, Brass, Generic) and four endpoint-config
  folders. Dorico routes each stave to the family-correct exp-map
  automatically based on the stave's instrument family — no manual exp-map
  swap per stave. Brief: `improvements/v1.16.3-dorico-cleanup.md`.

  - **Winds map** (`xmap.ouaricon.o_microtonalsampler_winds`): ord, flutter
    (`pt.flutterTongue`), breathy (`pt.whisper` — closest catalog match for
    the absent `pt.aeolian`), multi (`pt.multiphonic`), keyclick
    (`pt.keyClick`), slap (`pt.slapTongue`), harm (`pt.naturalHarmonic1`),
    stacc (`pt.staccato`).
  - **Brass map** (`xmap.ouaricon.o_microtonalsampler_brass`): ord, mute
    (`pt.muted`), cuivre (`pt.cuivre`), flutter (`pt.flutterTongue`),
    halfvalve (unbound — Dorico has no canonical `pt.halfValve`), stopped
    (`pt.stopped`), growl (`pt.growl` — corrected from `pt.growling`), fall
    (`pt.fallDrop` — corrected from `pt.fall`).
  - **Generic map** (`xmap.ouaricon.o_microtonalsampler_generic`): only slot 0
    (ord) bound; slots 1..7 ship unbound for user customization. Catches
    percussion, voice, keyboard, and any family not explicitly routed.
  - All four maps share the same 8-slot keyswitch shape (MIDI 0..7), the
    same `kVST3NoteExpression` microtonal routing, the same `pitchBendRange=2`,
    and the same `kCC param1=11` dynamics path. Plugin's `kMaxTech=8` cap
    is unchanged.

### Fixed
- **TC-2: Playback Template auto-loads the plugin without "Can't find a
  template spec" warning.** The Playback Template's `<entries>` now contain
  per-family `<endpointConfig>` references with `<instrumentFamilies>`
  filters (`kStrings`, `kWoodwinds`, `kBrass`) plus a fallback entry with
  no filter routing to the Generic endpoint config. Each entry binds to a
  separate user-folder endpoint config (`O-MicrotonalSampler`,
  `-Winds`, `-Brass`, `-Generic`), each containing one `slotData` with
  the family-correct `<expressionMapID>`. Replaces the v1.16.2 single-entry
  template that left every non-Strings stave warning-flagged.

  Manual-load workaround from v1.16.2's INSTALL-DORICO.md is no longer
  required.

### Changed
- **Strings exp-map slot remap (8 combos, was 10).** The Strings map drops
  three articulations and reshuffles the kept slots:

  | Slot | v1.16.2 (10-combo) | v1.16.3 (8-combo) | Dorico ID |
  |------|--------------------|-------------------|-----------|
  | 0 | ord | ord | `pt.natural` |
  | 1 | sp  | sp  | `pt.sulPonticello` |
  | 2 | st  | st  | `pt.sulTasto` |
  | 3 | sv  | **stacc** | `pt.staccato` *(was `pt.nonVibrato`)* |
  | 4 | cs  | cs  | `pt.muted` |
  | 5 | pizz | pizz | `pt.pizzicato` |
  | 6 | harm | harm | `pt.naturalHarmonic1` |
  | 7 | mart | **trem** | `pt.tremolo` *(was `pt.martele`)* |
  | 8 | trem | (dropped — moved to slot 7) | — |
  | 9 | flaut | (dropped) | — |

  Notations dropped from keyswitch firing: `Senza vib.` (sv), `Mart.`, and
  `Flaut.`. Existing scores using these markings will see Dorico hold the
  previously active slot (no audible change at the marking; documented in
  INSTALL-DORICO.md).

  Doricolib `<version>` bumped 7 → 8 to defeat Dorico's parsed-XML cache.

- **Default technique-tab labels (`PluginProcessor.cpp`).**
  `resetTechniqueNames()` now produces `ord, sp, st, stacc, cs, pizz, harm, trem`
  (was `ord, sp, st, sv, cs, pizz, harm, mart`). Pure cosmetic label change
  in the default array — slot count, RR buffer, threading, APVTS state shape,
  and `kMaxTech` cap are all unchanged. Existing user presets keep their
  saved labels (defaults only fire on `Reset Techniques` button or fresh
  plugin load).

### Documentation
- `Resources/dorico/INSTALL-DORICO.md` rewritten for the 4-folder
  installation layout and per-family routing table. New troubleshooting
  entry for "wrong family routing" symptoms.
- `Resources/dorico/SMOKE-TEST.md` adds TC-2 (family-aware endpoint
  loading), TC-5b/c/d (Winds / Brass / Generic keyswitches), TC-7
  (dropped-articulation regression check). Existing TC-4 (microtonal
  pitch) extends to test all four staves.

### Risk envelope
The `<instrumentFamilies>` enum values (`kStrings`, `kWoodwinds`, `kBrass`)
are inferred from Dorico SDK conventions; no concrete factory or user
template in the local Dorico install populates this tag with values.
If TC-2 fails (Dorico ignores the family filter and routes everything to
the first-listed entry, OR can't find a template spec at all), the
expected fix is either:
  1. Replace each `<instrumentFamilies>kFamily</instrumentFamilies>` text
     content with a child element (e.g. `<instrumentFamily>kFamily</instrumentFamily>`),
     or
  2. Drop the family filters entirely and accept that all staves bind the
     Strings map (matching v1.16.2 behavior + 3 unused exp-maps available
     for manual binding).

The four exp-maps and the C++ label patch are independently load-bearing
and tested.

#### Post-ship validation (annotated 2026-05-04, see v1.16.4)
- **TC-1 (template appears in dropdown):** PASS. Spec + 4 endpoint configs
  + doricolib all ingested cleanly. `application.log` showed clean
  `Loading PlaybackTemplateSpec`, `Loading Extra Library`, and 4×
  `Loading Endpoint Config` lines.
- **TC-2 (apply auto-loads plugin per family):** FAIL. Apply emitted 4×
  `Can't find a template spec or endpoint config for routing this
  instrument` warnings (Solo Violin, Flute, Trumpet, Marimba) and CLEARED
  any previously-loaded plugins on those staves with no replacement.
- **TC-3 (per-family exp-map binding):** FAIL (gated on TC-2).
- **TC-4 (microtonal pitch via VST3 Note Expression):** Validated only on
  manually-loaded instances — exp-maps themselves are correct.
- **TC-5a/b/c/d (per-family keyswitch firing):** FAIL (gated on TC-2).
- **TC-6 (CC11 dynamics):** Validated only on manually-loaded instances.
- **TC-7 (dropped articulations don't fire):** PASS — but only verifiable
  on manually-loaded instances since TC-2 blocked auto-load.

**Diagnosed root cause (post-ship):** the `<instrumentFamilies>` filter
takes Dorico's textual family entityIDs (`instrument family.strings`,
`instrument family.brass`, `instrument family.woodwinds`), not the C++
SDK enum names. Risk-envelope option 1 above was the right direction
but the wrong syntax — fix is text-content replacement, not a child
element. Patched in v1.16.4.

## [1.16.2] - 2026-05-04

### Fixed
- **TC-5: Dorico keyswitch-from-notation routing now fires.** Typing
  "sul pont." text in Dorico switches the plugin's WebView technique-tab
  strip to slot 1; "Ord." returns to slot 0. Pizz., Sul tasto, Senza vib.,
  Con sord., Harm., Mart. follow the same pattern. Two contributing
  defects, both load-bearing; either alone left TC-5 broken.

  1. **Plugin source: `ks_enabled` defaulted to `false`.** Fresh plugin
     instances created by Dorico's Playback Template / Endpoint Setup
     booted with the keyswitch trigger gate disabled, so even correctly-
     routed KS notes from the exp-map were ignored. Default flipped to
     `true` (`PluginProcessor.cpp:149`). KS range stays `0..9` (MIDI
     C-2..A-2 in the Dorico C3=60 convention) — well below any real
     instrument's pitch range, so the new default cannot accidentally
     fire from normal MIDI input.
  2. **Plugin source: `technique_count` defaulted to `1`.** With the
     count at 1, the `processBlock` KS handler clamps every incoming KS
     note to `juce::jmin(7, techCount-1)` = 0 — every KS routed to slot 0
     regardless of which technique Dorico fired. Default raised to `8`
     (`PluginProcessor.cpp:139`). All 8 plugin slots are now reachable
     by exp-map KS notes 0..7. The technique-tab strip now displays
     8 tabs by default; users wanting a slimmer UI can still reduce
     `technique_count` per-instance.
  3. **Distribution artifact: per-combo `<exclusionGroup>1` added.**
     `playbacktemplatedeps.doricolib` per-`<playingTechniqueCombination>`
     now carries `<exclusionGroup>1</exclusionGroup>` (matching HSO
     factory exp-maps). Required for Dorico's mutual-exclusion logic
     to fire `<switchOnAction>` on technique transitions out of `Ord.`
     when other techniques are mutually exclusive. `<version>` bumped
     4 → 7 (intermediate v5/v6 were transient diagnostic shapes — see
     "Schema iteration" below).

  Both the saved ks_enabled/technique_count values from existing
  v1.16.0 / v1.16.1 sessions are preserved on project reload (Dorico
  restores per-instance state). To pick up the new defaults, users must
  delete + re-add the O-MicrotonalSampler endpoint via Play → Endpoint
  Setup, or apply the Playback Template again. Documented in the
  install guide under "Upgrading from v1.16.x".

### Schema iteration (debug history, recorded for the next maintainer)

- **v5 (transient):** Mirrored HSO Cello Solo's full per-combo shape —
  added `<exclusionGroup>1</exclusionGroup>`, `<pitchBendRange>2</pitchBendRange>`,
  `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  per combo. Side effect: also dropped the **top-level** `<pitchBendRange>`
  and `<microtonalPlaybackMethod>` (HSO doesn't have them at top level
  — but HSO is 12-TET orchestral, doesn't need NE for microtones). TC-5
  remained broken (the real cause was plugin-side, not schema). TC-4
  (microtonal pitch via VST3 NE) regressed because per-combo
  `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  did not preserve the routing — Dorico fell back to no NE.
- **v6 (transient):** Plugin defaults flipped (ks_enabled=true,
  technique_count=8) — TC-5 fired. Per-combo `<pitchBendRange>` and
  `<microtonalPlaybackMethod>` removed, but top-level fields were still
  missing from the v5 rewrite, so TC-4 stayed broken.
- **v7 (shipped):** Restored top-level `<pitchBendRange>2</pitchBendRange>`
  and `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  to match the v1.16.1 baseline. Kept per-combo `<exclusionGroup>1</exclusionGroup>`
  (the only schema-shape change that's retained from v5). Both TC-4 and
  TC-5 pass.

  Net schema delta from v1.16.1 → v1.16.2: per-combo `<exclusionGroup>1</exclusionGroup>`
  added; per-combo `<monophonic>`, `<applyMillisecondsBeforeToEndOffsets>`,
  `<applyMillisecondsBeforeToControllers>` removed (HSO factory does
  not ship these — they appear to be from a different Dorico schema
  version and may have been silently ignored or rejected).

### Changed
- **`CMakeLists.txt`** — bump `VERSION` 1.16.1 → 1.16.2.
- **`PluginProcessor.cpp`** — `ks_enabled` default `false` → `true`;
  `technique_count` default `1` → `8`.
- **`Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`**
  — per-combo `<exclusionGroup>1</exclusionGroup>` added to all 10
  `<playingTechniqueCombination>` entries; `<version>` 4 → 7. Top-level
  `<pitchBendRange>2</pitchBendRange>` and
  `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  preserved unchanged from v1.16.1 (NE routing for TC-4).

### Validation

- pluginval (strictness 5) — PASS.
- auval — same pre-existing benign DEF-24-01 finding as prior versions
  (note-expression module's static-check artifact; not a runtime defect).
- Manual smoke in Dorico 6:
  - TC-1 Playback Template appears in dropdown — PASS.
  - TC-3 Expression map binds in Track Inspector — PASS.
  - TC-4 Microtonal pitch via VST3 NE — PASS (load-bearing; was the
    primary regression risk during the v5/v6 iteration).
  - TC-5 Playing-technique text fires keyswitches — PASS (primary fix
    target). "sul pont." → tab `sp`; "Ord." → tab `ord`.

### Known issues (carried from v1.16.1)

- **TC-2: Apply Playback Template doesn't auto-load the plugin slot.**
  Workaround unchanged — manually load O-MicrotonalSampler in the Mixer.
  Tracked as bonus follow-up; deferred from v1.16.2 scope.
- **TC-6: CC11 dynamics behavior** never tested end-to-end through
  Dorico in v1.16.x. CC11 wire is in the exp-map; plugin handler validated
  against direct MIDI in non-Dorico DAWs.
- **8-slot cap vs 10-technique exp-map.** The exp-map ships 10
  `<playingTechniqueCombination>` entries (KS notes 0..9), but the
  plugin caps internally at 8 slots. KS notes 8 (tremolo) and 9
  (flautando) clamp to slot 7 (martele) at the plugin layer. Two
  resolutions are open for a future patch: (a) trim the exp-map to 8
  combinations to match plugin capacity, (b) raise plugin
  `kMaxTech` to 10. Lower priority — none of the user's primary
  techniques fall in slots 8–9.

### Files touched

1. `CMakeLists.txt` — `VERSION` 1.16.1 → 1.16.2.
2. `CHANGELOG.md` — this entry.
3. `Source/PluginProcessor.cpp` — two parameter defaults flipped (lines
   139 and 149).
4. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`
   — per-combo `<exclusionGroup>1</exclusionGroup>`; `<version>` 4 → 7.
5. `.planning/STATUS.md` — v1.16.2 patch noted; primary follow-up
   (TC-5) closed; bonus follow-ups (TC-2, TC-6, 8-slot cap) carried.

---

## [1.16.1] - 2026-05-04

### Fixed
- **Dorico launch crash** ("Error opening file: invalid file format" at
  startup). Root cause: leading XML comments before the root element in
  `endpointconfig.xml` and `playbacktemplatespec.xml` are rejected by
  Dorico's strict user-config parsers. Same comment in
  `playbacktemplatedeps.doricolib` (which uses a different parser path
  inside `EndpointConfigs/`) was tolerated, but the parser used at
  `DefaultLibraryAdditions/` is strict — so the doricolib's leading
  comment also crashed launch when distributed via that path. All three
  files now have no pre-root comments.
- **Expression map not appearing in Track Inspector dropdown** in
  v1.16.0. Root cause: `playbacktemplatedeps.doricolib` inside
  `EndpointConfigs/<Name>/` is endpoint-scoped — its expression-map
  definition only registers when that endpoint is active in the project,
  and Dorico's auto-load template path was failing (separate bug).
  Without an active endpoint, the exp-map was invisible in the Track
  Inspector. v1.16.1 documents the `DefaultLibraryAdditions/` path
  (Dorico auto-merges every `.doricolib` placed there into every project's
  library on launch) as the canonical mechanism for global exp-map
  registration. End-users now copy three artifacts (not two) — the two
  folders plus a single `.doricolib` to `DefaultLibraryAdditions/`.

### Changed
- **`Resources/dorico/INSTALL-DORICO.md`** — rewritten. Documents the
  three-folder layout (now four, including `DefaultLibraryAdditions/`),
  the macOS + Windows install paths, the dev-vs-release CID caveat (kept
  from v1.16.0), known-issue scope for v1.16.1, and a troubleshooting
  section covering launch crashes, dropdown-no-show, and stale-cache
  recovery.
- **`<version>` in `playbacktemplatedeps.doricolib` bumped 1 → 4** to
  defeat Dorico's caching when the file is updated in place. Subsequent
  patches should bump this further on every doricolib content change.
- **`CMakeLists.txt`** — bump `VERSION` 1.16.0 → 1.16.1.

### Known issues (deferred to v1.16.x patches)

- **Apply Playback Template doesn't auto-load the plugin slot.** Dorico
  log: `Can't find a template spec or endpoint config for routing this
  instrument`. The `<entries>` in `playbacktemplatespec.xml` use empty
  `<instrumentFamilies/>` and `<instruments/>` — Dorico doesn't treat
  empty as catch-all. Working reference (`EndpointConfigs/Ample China/`)
  has TWO entries: one endpoint + one fallback `<generatorSpec>`. Workaround:
  manually load O-MicrotonalSampler in the Mixer.
- **Typing playing-technique markings (sul pont., pizz., Ord.) does NOT
  fire the keyswitch.** Plugin's WebView technique-tab strip doesn't
  switch on playback. Two attempted fixes during the v1.16.0 smoke test
  (adding `<switchOffActions>` with KS 0 to non-natural slots; adding
  `<exclusionGroup>1</exclusionGroup>` per combination) both regressed
  rather than helped. Reverted to original switchOn-only shape. Root
  cause unclear — possible factors: per-combination fields missing
  (HSO factory has `<pitchBendRange>`, `<microtonalPlaybackMethod>=kAuto`
  per combo); Dorico's MIDI router filtering KS notes; or some
  endpoint-binding requirement we haven't identified. Tracked in
  `improvements/dorico-keyswitch-fix.md` with full diagnostic context
  and prioritized investigation paths. Workaround: send keyswitch MIDI
  notes (C-1..A-1 = MIDI 0..9) directly via a MIDI track or external
  controller.

### Implementation notes

- **No source-code changes** to the plugin binary. v1.16.1 is a
  distribution-artifacts and documentation patch only. Build outputs
  identical to v1.16.0 except for the `<bundleVersion>` field bumped
  via CMake `VERSION`.
- **Smoke procedure (`Resources/dorico/SMOKE-TEST.md`) deferred to
  v1.16.x update** — six TCs from v1.16.0 still apply, but TC-2 (auto-
  load) and TC-5 (KS firing) are documented FAIL pending the next
  patch. TC-1 (template visible), TC-3 (exp-map binds — via
  `DefaultLibraryAdditions/` not auto-template), and TC-4 (microtonal
  pitch — load-bearing) are validated PASS in v1.16.1.

### Files touched

1. `CMakeLists.txt` — `VERSION` 1.16.0 → 1.16.1.
2. `CHANGELOG.md` — this entry.
3. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` — leading comment stripped.
4. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` — leading comment stripped, `<version>` 1 → 4.
5. `Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` — leading comment stripped.
6. `Resources/dorico/INSTALL-DORICO.md` — rewritten for `DefaultLibraryAdditions/` distribution path + known-issue scope.
7. `improvements/dorico-keyswitch-fix.md` (NEW) — diagnostic brief for the v1.16.x KS-firing patch.
8. `.planning/STATUS.md` — v1.16.1 patch noted; KS-firing tracked as open follow-up.

---

## [1.16.0] - 2026-05-03

### Added
- **Dorico distribution bundle** (`Resources/dorico/`). Three files
  authored against Dorico 6's user-library layout that wire
  O-MicrotonalSampler as a one-click Playback Template:
  - `EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` —
    references the dev-build VST3 plugin ID
    (`ABCDEF019182FAEB4F7544764F4D7453`), MIDI channel 1, expression-map
    binding to the bundled exp-map.
  - `EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`
    — the bundled expression map (`xmap.ouaricon.o_microtonalsampler`):
    `microtonalPlaybackMethod = kVST3NoteExpression` (preserves
    microtonal pitch via VST3 NE), `volumeType = kCC` param1=11
    (CC11 Expression for sustained dynamics), and 10
    `playingTechniqueCombinations` mapping Dorico's notation glyphs
    (`pt.natural`, `pt.sulPonticello`, `pt.sulTasto`, `pt.nonVibrato`,
    `pt.muted`, `pt.pizzicato`, `pt.naturalHarmonic1`, `pt.martele`,
    `pt.tremolo`, `pt.flautando`) to the plugin's keyswitch range
    (MIDI notes 0..9, technique slots 0..9, full velocity).
  - `PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml`
    — the user-facing Playback Template
    (`playbacktemplate.user.o_microtonalsampler`) that references the
    EndpointConfig.
- **`Resources/dorico/INSTALL-DORICO.md`.** End-user install guide
  with macOS + Windows path snippets, the dev-vs-release CID caveat
  documented, and a verification checklist.
- **`Resources/dorico/SMOKE-TEST.md`.** Six-step manual smoke procedure
  covering template visibility, endpoint loading, expression-map
  binding, microtonal pitch (P0 — load-bearing), technique keyswitch
  on notation, and CC11 dynamics swell.

### Changed
- `CMakeLists.txt` — bump `VERSION` 1.15.0 → 1.16.0.

### Implementation notes

- **Distribution mechanism finalised.** A spike against the user's
  installed Dorico 6 library confirmed that `.doricolib` Library
  Manager imports register expression-map definitions but **not**
  EndpointConfig or PlaybackTemplate — those entities live in their
  own folder structures (`EndpointConfigs/<Name>/` and
  `PlaybackTemplateSpecs/<Name>/`) at the user-library root. The
  earlier plan's "single `.doricolib` containing exp-map + Playback
  Template + Endpoint Configuration" assumption was structurally
  incorrect; v1.16.0 ships the actual 3-folder layout Dorico itself
  uses for user-saved templates. This unblocks the previously
  reverted Phase 25 Plan 01 distribution mechanism (commit `d2c86c5`
  rollback in the parent `note-expression` module).
- **Schema validated against factory references.** Action XML
  confirmed against `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Symphonic Orchestra/expressionMapsDefinitions.xml`
  (`<switchOnAction><type>kKeySwitch</type><param1>...</param1><param2>127</param2></switchOnAction>`)
  and `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/Iconica Sketch/expressionMapsDefinitions.xml`
  (`<volumeType><type>kCC</type><param1>11</param1></volumeType>` —
  the literal string is `kCC` with the CC# in `param1`, NOT `kCC11`
  as a type name). EndpointConfig + Spec structure modelled on the
  user's existing "Test State-less" reference pair.
- **Parent NE map inlined, not chained.** The expression map's
  `<parentEntityID>` is intentionally empty (rather than referencing
  `xmap.ouaricon.vst3_note_expression`) so a single template install
  wires everything — no separate `.doricolib` import required for
  the parent module. The cost is ~10 KB of duplicated XML; the
  benefit is one-step install for end-users.
- **No source-code changes.** v1.16.0 is a distribution-artifacts-only
  release — pure XML + docs under `Resources/dorico/`. The plugin
  binary is unchanged from v1.15.0. Build / pluginval / auval status
  inherits from the v1.15.0 baseline.
- **Dev-build CID hardcoded.** The `<pluginID>` in
  `endpointconfig.xml` matches the dev-branded build (manufacturer
  `OuDv`, suffix `-dev`). Release builds (manufacturer `OuAu`, no
  suffix) produce a different CID; release CI will need a parallel
  artifact tree, tracked as a v1.16.x patch series. Documented in
  `INSTALL-DORICO.md` § "Caveat: dev vs release builds".

### Test surface

- Manual smoke procedure: `Resources/dorico/SMOKE-TEST.md` (six TCs
  covering template discovery, endpoint loading, expression-map
  binding, microtonal pitch routing, technique keyswitch on
  notation, and CC11 dynamics).
- No new automated test executables — distribution artifacts cannot
  be exercised without a Dorico session.

### Files touched

1. `CMakeLists.txt`
2. `CHANGELOG.md`
3. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` (NEW)
4. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` (NEW)
5. `Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` (NEW)
6. `Resources/dorico/INSTALL-DORICO.md` (NEW)
7. `Resources/dorico/SMOKE-TEST.md` (NEW)
8. `.planning/STATUS.md` — v1.16.0 marked implemented; Multi-Version Plan complete.

---

## [1.15.0] - 2026-05-03

### Added
- **CC + Program Change technique triggers.** Two new MIDI trigger
  mechanisms join keyswitches: a configurable Continuous Controller
  (CC#, default 32) routes its 0..127 byte through an 8-slot
  value-range table to a target technique slot, and Program Change
  events route through an 8-slot PC#-to-technique table. Both share
  one technique cursor with KS — an 8-band cursor that all DAWs can
  drive without specialised expression-map authoring. Disabled by
  default for back-compat (v1.14.0 sessions migrate untouched).
- **KS > CC > PC > history precedence.** When multiple triggers fire
  in the same audio block (e.g. an automation lane bumps both a
  keyswitch note and a CC simultaneously) the highest-precedence
  candidate wins. History ("last technique used") persists across
  blocks if no trigger fired — eliminates the "phantom default reset"
  failure mode some sample players exhibit when the last MIDI event
  was an unrelated CC.
- **Three new APVTS parameters:** `cc_select_enabled` (bool, default
  off), `cc_number` (0..119, default 32 — General Purpose 1; not the
  CC1 modulation, CC11 expression, or bank-select reserved numbers),
  `pc_enabled` (bool, default off). All round-trip through project
  state and host automation.
- **CC + PC mapping tables persist with project state.** New
  `<CcMapping>` and `<PcMapping>` ValueTree children with sparse 8-slot
  child lists; v1.14.0 sessions decode cleanly back to the seeded
  defaults (CC equally splits 0..127 across the active
  `technique_count`; PC#i → tech i).
- **Trigger configuration panel in the WebView UI.** Collapsible
  `<details>` disclosure under the technique-bar with two sub-panels
  (CC + PC), each showing an 8-row editable table. Slot rows beyond
  the active technique count are dimmed but remain editable so users
  can pre-stage values before growing their library. "Reset to
  defaults" button restores the seeded mapping. Hidden entirely when
  `technique_count == 1` (matches the technique-bar back-compat
  contract — single-technique libraries see no v1.15.0 chrome).
- **`docs/dynamics-mapping.md`.** New doc explains how the plugin
  routes dynamics: note-on velocity selects the sample layer (locked
  at note-on, no continuous modulation); CC11 ("Expression") drives
  smoothed post-mix gain throughout sustain. Recommends
  `<volumeType><type>kCC11</type></volumeType>` as the Dorico
  expression-map default for sustained dynamic shaping, with
  `kNoteVelocity` as an alternative for short / articulated passages.
  Forward-compat note on Dorico 3+'s secondary-volume-control slot.
- **Two new EXCLUDE_FROM_ALL test executables:**
  `O-MicrotonalSampler_CcPcTriggerCheck` (40+ assertions covering
  defaultCcMapping / defaultPcMapping bucketing, value-range routing,
  PC routing, the KS>CC>PC>history precedence resolver, and an
  end-to-end three-block scenario) and
  `O-MicrotonalSampler_DynamicsLayerCheck` (pins
  velocity-to-layer-index bucketing for N=1/2/4/8 layers — the
  formula `MicrotonalSamplerVoice::startNote` uses, the same one
  documented in `docs/dynamics-mapping.md`).

### Changed
- **`processBlock` MIDI scan reorganised.** The KS-only walk from
  v1.14.0 expanded to a single-pass scan that harvests KS / CC / PC
  candidates simultaneously, then resolves precedence once at block
  end via `OMtsTrigger::resolveTriggerPrecedence`. RT-safety
  preserved — CC + PC tables are read via `std::atomic_load` on
  shared_ptr (the same COW pattern `currentSampleMap` uses), the
  filter buffer is still pre-allocated in `prepareToPlay`, and the
  scan walks the host's MidiBuffer exactly once instead of twice
  when CC + KS are both active.
- **No-trigger-this-block path is a no-op.** When neither KS, CC,
  nor PC fired, `pendingTechniqueIndex` is left untouched —
  v1.14.0's "store same value over and over" behaviour is replaced
  with a guarded compare-then-store so the AsyncUpdater isn't
  triggered for null events.

### Architecture
- **`Source/TriggerMapping.h`** — pure-data header containing the
  `CcSlot` / `PcSlot` structs, default-builder helpers, and the
  resolver free functions. Lives outside the audio processor so
  the unit-test executable can consume the routing logic without
  pulling in `JuceHeader.h` or instantiating an `AudioProcessor`.
  Audio thread + message thread + tests share identical resolution
  code by construction.

### Migration / back-compat
- v1.14.0 sessions decode cleanly: missing `<CcMapping>` /
  `<PcMapping>` children → constructor's seeded defaults survive.
- `cc_select_enabled` and `pc_enabled` default to false so old
  sessions hear no behavioral change.
- Single-technique libraries (technique_count=1) see zero new UI
  chrome — the trigger panel is hidden alongside the existing
  technique-bar.

## [1.14.0] - 2026-05-03

### Added
- **Playing Techniques (engine + Keyswitches + UI core).** Sample cells now
  carry a third axis — `technique` — alongside `(midiNote, velocityLayer)`.
  Each `SampleMap` can hold up to 8 technique slots (default vocabulary:
  `ord`, `sp`, `st`, `sv`, `cs`, `pizz`, `harm`, `mart`). Filenames carrying
  any recognised token (delimited, case-insensitive — exact match, never
  substring) auto-route to their slot at folder-load time. Each slot
  recognises both the two-letter shorthand and a wider set of orchestral
  long-forms — e.g. slot 1 (sul ponticello) accepts `sp`, `sulpont`,
  `sulponticello`, AND the canonical `sul_pont` / `sul_ponticello` two-token
  forms produced by orchestral-library naming conventions. Same for
  `sul_tasto` (slot 2), `senza_vib` / `non_vib` / `non_vibrato` (slot 3),
  `con_sord` / `con_sordino` / `muted` (slot 4), `pizzicato` (slot 5),
  `harmonic` / `harmonics` (slot 6), `martele` / `martellato` (slot 7),
  `tremolo` (slot 8), `flautando` / `flautato` (slot 9). Library leads
  (`sul`, `senza`, `non`, `con`) are NEVER accepted standalone — they
  require their canonical suffix to avoid over-matching. Two recordings
  of the same `(midi, velocity)` with different technique tokens (e.g.
  `C3_v1_ord.wav` + `C3_v1_sp.wav`) now coexist in two distinct cells
  instead of triggering the round-robin ambiguity modal.
- **Five new APVTS parameters:** `technique_count` (1–8), `technique_select`
  (0–7), `ks_enabled` (bool), `ks_low_note` (0–127), `ks_high_note`
  (0–127). All round-trip through project state and host automation.
- **Keyswitch routing in `processBlock`.** When `ks_enabled` is on, MIDI
  note-ons inside `[ks_low_note..ks_high_note]` are absorbed (never reach
  the synth) and store their semitone offset from `ks_low_note` into the
  active-technique atomic. Matching note-offs are likewise absorbed so
  KS notes never trigger spurious voices. A pre-allocated `juce::MidiBuffer`
  carries the filtered stream into `Synthesiser::renderNextBlock`,
  preserving real-time safety (`MidiBuffer::ensureSize` runs on the
  message thread in `prepareToPlay`).
- **Voice-side technique resolution.** `MicrotonalSamplerVoice::startNote`
  loads the technique cursor with `memory_order_acquire` and pairs it with
  the sample-map snapshot to resolve the `(midi, vel, tech)` triplet via
  the new `SampleMap::findCell(midi, vel, tech)` overload. The triplet
  lookup falls back to `tech=0` ("ord") when the requested slot is empty,
  so partially-populated technique sets still play. Crossfade pair MUST
  share technique (no cross-articulation morphing).
- **Per-cell round-robin counter expanded** from 512 to 4096 entries
  (`128 × 4 × 8`). Counters are independent per technique slot — a flip
  from `ord` to `sp` mid-session no longer disturbs the `ord` slot's RR
  cursor.
- **WebView UI: technique tab strip** above the sample-map grid. Tabs are
  click-to-select (left-click) / right-click-to-rename. `+` / `−` buttons
  grow / shrink `technique_count`. Inline KS panel — toggle + low/high
  number inputs — wires through `setKeyswitchEnabled` /
  `setKeyswitchRange` native functions. Hidden by default; only surfaces
  once the user has expanded beyond a single technique slot or enabled
  KS, preserving the v1.13.0 visual contract for legacy sessions.
- **Six new WebView native functions:** `getTechniqueState`,
  `setActiveTechnique`, `setTechniqueName`, `resetTechniqueNames`,
  `addTechniqueSlot`, `removeTechniqueSlot`, plus `setKeyswitchEnabled`
  / `setKeyswitchRange`. Existing `loadSingleSampleDialog`,
  `overrideLoopPoints`, `resetLoopToAutoDetect`, and `getWaveformPeaks`
  gained an optional trailing `technique` arg (defaults to the current
  active-technique cursor — UI clicks already route correctly).
- **State persistence:** `<TechniqueNames><Slot index name/></TechniqueNames>`
  child added to the captured state ValueTree. Sparse — only renamed
  slots are emitted; the curated default vocab covers absent slots on
  restore. v1.13.0 sessions decode unchanged (no `<TechniqueNames>`
  child → default vocab survives, `technique_count=1`, `ks_enabled=false`).
- **Three new regression tests** (EXCLUDE_FROM_ALL):
  `O-MicrotonalSampler_TechniqueParseCheck` (24 cases — token recognition,
  substring rejection, case insensitivity, coexistence with
  note/velocity/RR), `O-MicrotonalSampler_FindCellTripletCheck` (8 cases
  — exact match, fallback, disjoint techniques, closest-note within slot,
  back-compat overload, `applyMergeRrCell` triplet keying),
  `O-MicrotonalSampler_StateMigrationCheck` (5 cases — empty tree =
  default vocab, sparse rename leaves untouched slots, `SampleCell`
  default-init, v1.13.0-shape merge identical to v1.13.0,
  back-compat `findCell` two-arg overload).

### Changed
- **`SampleCell` gained `int technique = 0`.** All callers default to 0
  in v1.13.0-shape sessions. `findCell` two-arg overload is preserved for
  back-compat and routes to `tech=0`.
- **`LoadOptions` / `LoadOp` gained `targetTechnique` + `overrideTechnique`.**
  These thread through `SampleLoader::loadFolder`'s 3D grouping pass; a
  user-driven "assign folder to technique" override is the v1.15.0 modal
  surface.
- **`AmbiguousDuplicate` payload includes `technique`.** WebView's RR
  confirmation modal now sees the slot a duplicate group lives in (only
  matters when one technique slot has duplicates — different techniques
  no longer collide).
- **`FilenameParser.h` switched from `<JuceHeader.h>` to specific
  `juce_core` include.** Matches the `SampleMap.h` pattern so standalone
  test executables (the new triplet/parser/migration checks) compile
  without going through `juce_add_plugin`.

### Compatibility
- **MINOR bump (1.13.0 → 1.14.0).** Backward-compatible. v1.13.0 presets
  load unchanged: `technique_count` defaults to 1, `ks_enabled` defaults
  to false, every cell defaults to `technique=0` ("ord"), and the
  technique tab strip stays hidden until the user opts in. Audio output
  for single-technique libraries is bit-identical to v1.13.0
  (render-harness identity verified by the test surface). VST3 / AU IDs
  unchanged.

### Testing
- Build green: VST3 + AU + Standalone, macOS arm64, Release.
- pluginval level 5: PASS.
- auval AU: PASS.
- Regression tests: 3 new test executables, all assertions PASS.

## [1.13.0] - 2026-05-02

### Changed
- **ARCH-02: extracted WKWebView drag-drop content-streaming pattern to
  shared module `modules/core/webview-drop-streaming` (v1.0.0).** The 4
  `dropSession*` native function handlers, session-scoped temp-dir
  lifecycle, 5-min stale-session reaper, `DropSessionGuard` validators
  (path traversal, parent-chain symlink, 256 MB-per-file / 4 GB-per-session
  caps), and the JS-side streaming helpers (`bindWebViewFileDrop`,
  `streamFolderEntryToCpp`, `streamSingleFileEntryToCpp`,
  `readFileEntryAsBase64`, `arrayBufferToBase64`) now live in the module.
  This editor instantiates one `Ouaricon::WebViewDropStreaming::SessionManager`
  with two commit callbacks (forwarding to `processorRef.loadSampleFolder`
  / `loadSingleSample`) and splices the module's native functions into
  `buildNativeFunctionRegistry()`. JS imports `bindWebViewFileDrop` from
  `./modules/webview-drop-streaming.js` and passes a config object with
  the plugin-specific glue (selectors, modal/toast/hover callbacks, cell
  midi/vel extractor). Behaviour is unchanged — every code path
  (single-file drop, folder drop with options modal, fast-path for hosts
  exposing absolute paths, no-path-no-entry diagnostic) is preserved
  verbatim. Per-plugin `tempDirPrefix` (`"o-microtonalsampler-drop-"`)
  isolates the stale-session reaper so future module adopters don't
  collide.

### Removed
- `Source/DropSessionGuard.h` — promoted to the shared module
  (`modules/core/webview-drop-streaming/cpp/DropSessionGuard.h`).
- `cleanupStaleDropSessions()` editor method — the reaper now runs
  inside `SessionManager` scoped to the per-plugin temp-dir prefix.
- ~290 lines of inline `dropSessionStart` / `dropSessionAddFile` /
  `dropSessionCommitFolder` / `dropSessionCommitFile` lambdas from
  `buildNativeFunctionRegistry()`.
- ~470 lines of inline streaming helpers from `sampler-app.js`
  (`bindWebViewFileDrop`, the 4 streaming functions, `extractDroppedFilePaths`,
  `pointInClientRect`, `collectAudioFilesFromDir`, `newDropSessionId`,
  `AUDIO_EXTENSIONS_RE`).

### Code metrics
- `Source/PluginEditor.cpp`: ~290 lines of native-fn handlers + 16 lines
  of `cleanupStaleDropSessions()` removed; ~30 lines of SessionManager
  construction + splice loop added.
- `Resources/ui/js/sampler-app.js`: ~470 lines of inline drag-drop code
  removed; ~25 lines of parameterized `bindWebViewFileDrop({...})` call
  added.
- New shared module: ~1100 lines (C++ header-only + JS ES module +
  README + module.yaml) reusable across O-TextureForge, O-Bells,
  O-Lyrica, future plugins.

### Notes
- O-TextureForge and other plugins that re-implement this pattern are
  **deferred to follow-up improvements** — each will get its own
  regression-tested PATCH bump after migration.
- No behavioural change; no parameter or state-format changes; presets
  and DAW sessions load unchanged.
- Manual DAW drag-drop test required after install — no automated
  regression suite covers the WKWebView surface.
- `Source/tests/drop_session_guard_check.cpp` retains the v1.11.2
  security regression coverage; its include path now points at the
  module's `cpp/`.

### Reference
- REVIEW-architecture.md §"Extract drag-drop streaming → shared module"
  (lines 100-121, 446-449); SUMMARY.md architecture wins #2.

## [1.12.4] - 2026-05-02

### Changed
- **ARCH-01: data-driven native function registry.** Replaced 44 inline
  `.withNativeFunction(...)` chained calls in the editor constructor
  (~1400 lines of organic v1.5.0–v1.12.0 growth) with a single registry
  vector iterated in a `for (auto& [name, handler] : ...)` loop. Each
  native function moves from a chained builder argument to one entry in
  `buildNativeFunctionRegistry()` (new private method). Constructor
  shrinks from ~1500 lines to ~50; the WebView is built via an immediately
  invoked lambda that returns the fully populated `Options`. Behaviour is
  unchanged — every entry preserves its original name, capture list, and
  body verbatim. Pattern is reusable in O-Bells / O-Lyrica which carry
  similar editor boilerplate (architecture-review §1, HIGH ROI).

### Removed
- **Dead code: `Resources/ui/css/tuning-panel-readonly.css`.** Embedded
  as a binary resource since v1.0.0 but no longer applied since the
  v1.2.0 read-only-tuning-panel rewrite. Removed the file, its
  `juce_add_binary_data` SOURCES entry, and the corresponding
  `getResource` URL handler. Frees one binary blob from the plugin's
  embedded resources (~3 KB) and eliminates a stale referent in the
  resource provider.

### Code metrics
- `Source/PluginEditor.cpp` constructor body shrinks from ~1500 → ~50
  lines (97% reduction in constructor size).
- `Source/PluginEditor.cpp` total file delta: +61 lines (1930 → 1991)
  — the 44 lambda bodies move to the new registry method along with
  surrounding function-decl wrapping; net file growth is small because
  only the per-entry brackets/commas change vs the original
  per-call `.withNativeFunction(...)` wrapping.

### Validation
- Smoke-tested in Logic Pro and Reaper: load folder, drag-drop folder,
  drag-drop single file, tuning panel (all panels), preset save/load,
  sample-map clear, MTS-ESP routing.
- All 44 native function entries verified against the v1.12.3 backup
  (name + arity + capture list + body bytes match).

### Migration notes
None — pure structural refactor. APVTS, state format, parameter IDs,
preset format, and JS-bridge contract are all unchanged.

## [1.12.3] - 2026-05-02

### Fixed
- **HG-01: replay-queue corruption from cascaded callbacks during state
  restore.** Two paths could re-enter the queue dispatcher with stale
  expectations: (1) a synchronous `applyFolderLoad` →
  `sampleMapChangedCallback` → editor → public `loadSampleFolder` chain
  could land back inside the still-running outer `kickNextReplayOp`,
  popping ops out from under its iterator; (2) a chain continuation
  staged on an ambiguous-duplicate confirmation could fire long after
  an unrelated state restore or `clearSampleMap` had wiped/rebuilt
  `pendingReplayOps`, dispatching the previous generation's op against
  the new queue. `kickNextReplayOp` now carries a single-threaded
  re-entry guard that rejects synchronous re-entry, and every external
  mutation of `pendingReplayOps` (`setStateInformation`,
  `clearSampleMap`) bumps an atomic `replayQueueGeneration` token.
  Loader callbacks and the `pendingDuplicateChainContinuation` lambda
  capture the generation at staging time and bail out on mismatch.
- **HG-02: filename parser silently dropped RR semantics for
  separator-tokenised conventions.** Filenames following the common
  DAW-export pattern `Piano_C3_take_1.wav` (or `_rr_2`, `_tk_3`)
  tokenise to `["Piano","C3","take","1"]`; the v1.8.0 RR scan only
  matched the glued form `take1` and silently returned `rr=-1` here,
  defeating round-robin for these files. The parser now also detects a
  bare `rr`/`take`/`tk` token whose immediately following token is a
  1–2 digit integer in 1..64, and treats the pair as the RR index.
  Glued form (`take7`) still wins when present so existing libraries
  are unaffected. Tokeniser-agnostic across `_`, `-`, `.`, and space
  separators. Added unit-test coverage for `Piano_C3_take_1.wav`,
  `Trumpet_F#3_rr_2.aif`, `Bowed_E2_tk_3.flac`, dash/space variants,
  out-of-range and bare-prefix rejection (`take_99`, `taken_1`).
- **HG-04: `static_assert` enforces the `kMaxVariantsPerCell` ↔
  `0xFF`-sentinel invariant.** `selectVariantIndex` clips its uint8
  RR counter to 254 to keep `0xFF` as the "no variant yet" sentinel.
  The cap of 64 was a local `constexpr` in two `PluginProcessor.cpp`
  sites with no compile-time link to the counter type — a future bump
  above 254 would silently saturate the counter while the returned
  index kept going, diverging RR behaviour. Hoisted
  `kMaxVariantsPerCell` into `MicrotonalSamplerVoice.h` next to
  `RrCounterArray` and added
  `static_assert(kMaxVariantsPerCell < 255, "variant index must fit
  in uint8 with 0xFF sentinel reserved")` so any future bump fails
  the build instead of failing audibly.
- **HG-05: folder-load callbacks no longer crash when a project closes
  mid-load.** `loadSampleFolder`, `kickNextReplayOp`'s loader
  dispatch, and `loadSingleSample` all captured `this` raw into
  message-thread completion/failure callbacks. `~SampleLoader`'s
  2-second `stopThread` joins the worker, but JUCE's
  `MessageManager::callAsync` queue is NOT flushed by
  `~AudioProcessor`, so callbacks already queued at destruction time
  ran with a dangling `this`. The processor is now
  `JUCE_DECLARE_WEAK_REFERENCEABLE`; every loader callback captures a
  `juce::WeakReference<OMicrotonalSamplerAudioProcessor>` and
  null-checks on entry. The destructor clears the weak-ref master
  before any other teardown so the bail-out path activates as soon as
  destruction begins. pluginval's tear-down stress paths exercise this
  flow.

### Notes
- All four fixes are HIGH-severity findings from the v1.11.1 deep
  code review (REVIEW-cpp-bugs.md). They share one root pattern:
  lifetime / re-entrancy assumptions that hold under nominal load but
  break under host quirks (off-thread save in Reaper, project-close
  mid-load, cascaded UI callbacks during replay). No DSP behaviour
  changes.

## [1.12.2] - 2026-05-02

### Fixed
- **FE-01: drag-drop folder streaming no longer silently corrupts on a
  single bad file.** The base64-streaming loop in
  `streamFolderEntryToCpp` already had a try/catch around the FileReader
  read + native-fn call, but only logged to console — the user saw the
  "Loading X of N" toast freeze on the next file with no indication
  anything had failed. Each per-file failure now toasts a specific
  "Skipped: <name> (read failed | backend rejected)" message and the
  final commit toast counts the skips ("Loading 47 of 50 samples (3
  skipped)…"). If every file fails, the commit step still runs so the
  C++ side reaps the empty session. `streamSingleFileEntryToCpp` got
  the same per-step protection so a corrupted single-file drop fails
  cleanly with a user-visible toast.
- **FE-02: backend stalls in drag-drop streaming no longer hang the UI
  permanently.** Every `Juce.getNativeFunction(…)` await in
  `streamFolderEntryToCpp` and `streamSingleFileEntryToCpp` is now
  wrapped — `dropSessionStart`, `dropSessionAddFile`,
  `dropSessionCommitFolder`, `dropSessionCommitFile`. The
  `showFolderLoadOptionsModal` await is also wrapped against modal
  promise rejection (DOM tear-down, cleanup-handler exception). On
  rejection each step surfaces a distinct toast (start / per-file /
  commit) and aborts cleanly, leaving the UI responsive. Previously, a
  C++ deadlock or message-thread stall would leave the user staring at
  a stale "Loading…" toast with no way to recover short of closing the
  plugin window.
- **FE-03: stale-cell race when sample-map snapshots fire mid-click.**
  The 250 ms double-click discriminator in `bindGridInteractions`
  schedules a `setTimeout` that closes over a `cell` DOM reference and
  reads `dataset.note` / `dataset.layer` at fire time. If a folder load
  or sampleMapUpdated event triggered `renderGrid` between click and
  fire, the timer would either no-op against a detached node or — if
  the grid had been re-rendered with a different sample map — fire
  against a re-bound cell at the same grid position carrying different
  MIDI/layer values. `renderGrid` now clears `pendingClickTimer` at the
  top, matching the cleanup the dblclick branch already performs.

### Notes
- All three fixes are fail-safe: per-iteration error handling in
  drag-drop loops + UI-recovery toasts on every backend await + cancel
  the deferred single-click whenever the grid rebuilds.

## [1.12.1] - 2026-05-02

### Fixed
- **CR-01: CC11 (Expression) no longer calls `setValueNotifyingHost` from the
  audio thread.** Per-byte `setValueNotifyingHost` in `processBlock` was a
  real-time correctness violation — listeners chain back through host
  parameter machinery, can take locks, allocate, and stall the audio thread
  in some hosts. Fast CC11 streams now stage the latest 0..127 value into a
  `std::atomic<int>` on the audio thread; an `AsyncUpdater` drains the
  atomic on the message thread and forwards to the host. Last-value-wins
  semantics within a block are unchanged; the audio path is now lock-free.
- **HG-08: `loadOpHistory` and `lastSkippedFiles` synchronised against
  off-thread `getStateInformation`.** Reaper (and possibly other hosts) call
  `getStateInformation` from a save-state worker thread, racing the message-
  thread mutations from folder-load completion callbacks. A
  `juce::CriticalSection` now guards both containers across all
  mutation/read sites (`applyFolderLoad`, `clearSampleMap`,
  `restoreStateValueTree`, `confirmRoundRobinLoad`, `loadSampleFolder`
  failure callback, `loadSingleSample`, and `captureStateValueTree`).
  Project saves during in-flight folder loads can no longer produce
  truncated XML.

## [1.12.0] - 2026-05-02

### Fixed
- **Drag-dropped folders now persist correctly across project save/reopen.**
  v1.0.4–v1.11.x recorded the WebView drag-drop temp dir
  (`/tmp/o-microtonalsampler-drop-<id>/`) as the saved sample-folder path,
  so on reload the missing-folder modal pointed at a `/tmp/...` path that
  was reaped at the next drop session. The state format now distinguishes
  filesystem loads from drag-drop loads and persists the original folder
  name lifted from `FileSystemEntry::name` at drop time. On reload, drag-
  dropped sessions surface a friendlier modal: *"Samples were drag-dropped
  from <name> without 'Embed audio' enabled — re-drag the folder or browse
  to relocate."* No more dead `/tmp/` paths in the UI.

### Added
- **Embed audio in project state.** New "Embed audio in project state"
  checkbox in the Folder Load Options modal (shown for both Load Folder
  dialog and drag-drop). When ON, the loaded audio is serialised inline
  into the saved project state as 24-bit PCM WAV. Tradeoffs:
    - Project survives folder moves, cross-machine transfer, and (for
      drag-drop) WebView temp-dir cleanup unchanged.
    - Project file size grows by the audio data size — the modal shows a
      live size estimate when the checkbox is on so the user always sees
      the cost.
    - For drag-drop, total bytes are computed during the entry-tree pre-
      walk and shown directly in the options modal.
    - For Load Folder dialog, a follow-up confirmation modal surfaces the
      actual size after the user picks a folder, before the load commits.
    - Default is OFF — current behaviour is preserved unless the user
      explicitly opts in per load.
- **Drag-drop missing-folder modal variant.** When a drag-drop op without
  embed is restored from a saved project, the missing-folder modal renders
  drag-drop-specific copy and a "Browse for folder…" button (vs the
  filesystem variant's "Locate folder…").

### Changed
- **State XML schema for `<SampleFolders><Op …/>`** — additive, fully
  backward-compatible with v1.11.x saves:
    - New optional attrs: `kind` ("filesystem"|"drag-drop"), `name`
      (display name for the missing-folder modal), `embed` ("1" iff inline
      audio).
    - Drag-drop ops omit the `path` attr (the temp dir is session-scoped).
    - When `embed=1`, the op carries an `<Audio>` child with
      `<Cell midi=… layer=…><Variant filename=… loopMode=…
      loopStart=… loopEnd=… wav="<base64>" /></Cell>` entries.
    - States saved on v1.11.x and earlier load identically (missing attrs
      default to filesystem origin, no embed).
- **`folderMissing` WebView event payload** — now an object
  `{path, kind, name}` instead of a bare string. JS branches on `kind` to
  render the appropriate modal copy. Backward-compat for stale string-form
  payloads is kept defensively in `subscribeFolderMissingEvent`.
- **Native fn split** — `loadSampleFolderDialog` (v1.6.0) replaced by
  `pickSampleFolder` + `estimateFolderAudioSize` + `loadSampleFolderByPath`.
  The split lets JS show the embed-size confirmation between selection and
  load. New native fns:
    - `pickSampleFolder()` → `{path, name, cancelled}`
    - `estimateFolderAudioSize(path)` → `int64` bytes (sum of `*.wav`,
      `*.aif`, `*.aiff`, `*.flac` files, recursive)
    - `loadSampleFolderByPath(path, layer, mode, override, embedAudio)`
- **`dropSessionStart`** — accepts an optional `args[1] = folderName` (from
  `FileSystemEntry::name`) so drag-drop loads carry a stable, user-meaningful
  display name into the saved state.
- **`dropSessionCommitFolder`** — accepts an optional `args[4] = embedAudio`
  (0/1) so drag-drop loads can opt into inline audio serialisation.

### Notes
- **State size impact (embed mode)**: 24-bit PCM at host SR × ~33% base64
  overhead. A 250 MB sample library encoded at 48 kHz / 24-bit / stereo
  yields a project state on the order of 250–350 MB. Project save/reopen
  performance scales with state size; users with large libraries should
  weigh portability against project-file weight.
- **Audio quality (embed mode)**: 24-bit PCM has a -141 dB noise floor —
  inaudible artifacts. Float samples outside [-1, +1) clip on encode (same
  constraint as any 24-bit export pipeline).
- **No breaking changes.** Saved sessions / presets from v1.11.x reload
  identically. The new behaviour only activates when (a) the user explicitly
  opts into embed via the modal, or (b) a new drag-drop load is saved on
  v1.12.0+.
- **v1.11.x sessions with drag-drop loads**: those projects will continue
  to surface the legacy missing-folder modal pointing at the old `/tmp/`
  path on first reload (no `kind` attr in the saved state means it
  classifies as filesystem). After the user relocates or skips, the next
  save records the friendlier drag-drop kind for any new drops.

## [1.11.3] - 2026-05-02

### Fixed
- **Use-after-free on `cellLow` / `variantLow` raw pointers across SampleMap
  swap (REVIEW CR-04).** `MicrotonalSamplerVoice::startNote` re-snapshots
  `currentMap` from `*sampleMapSource` after running steal-tail rendering.
  The voice's `variantLow` / `cellLow` raw pointers index into the OLD map's
  variants vector; if no other voice held a snapshot, the swap dropped the
  prior shared_ptr's last refcount and freed the audio buffers `variantLow`
  pointed into. v1.11.3 captures `prevMap = currentMap;` at the top of
  `startNote` so the prior map's refcount stays ≥ 1 for the entire
  function — including `renderTailRamp` and the small window before
  `variantLow` is reassigned to the new map's variants.
- **`renderTailRamp` early-return guard restructured into positive form
  (REVIEW DSP CRITICAL #1).** The guard at lines 240-254 was logically an
  OR of error conditions but was flagged by both the DSP and C++ reviewers
  as ambiguous and a click-on-steal regression risk. v1.11.3 rewrites it
  as `if (! prereqsMet) { zero+return; }` so the render path is
  unmistakably reachable.
- **Ramp coefficient division underflow when `rampSamples < 2`
  (REVIEW DSP HIGH).** The expression `(float) i / (float) rampSamples` in
  the per-sample render loop is well-defined for `rampSamples >= 1` but
  produces a degenerate one-step ramp at `rampSamples == 1` and is fragile
  at `rampSamples == 0` if the upstream `> 0` guard is ever weakened.
  v1.11.3 folds `rampSamples >= 2` into the `prereqsMet` predicate so the
  1-sample (no audible fade) and 0-sample cases take the zero+bail path
  before the ramp loop runs.
- **APVTS `getRawParameterValue("attack")->load()` null-deref on note-on
  (REVIEW DSP CRITICAL #2).** `startNote` previously dereferenced the
  result of `getRawParameterValue` for each of `attack` / `decay` /
  `sustain` / `release` without a null check — a typo or APVTS layout
  change would crash the audio thread on every note-on. v1.11.3 caches
  `attackParam` / `decayParam` / `sustainParam` / `releaseParam` atomic
  pointers in `prepareToPlay` (with a `jassert` per pointer in debug
  builds), and `startNote` only invokes `adsr.setParameters` when all four
  are non-null.

### Notes
- **No state-format / parameter / preset / API changes.** Sessions saved
  on v1.11.2 reload identically — these are pure voice-render correctness
  fixes living entirely inside `MicrotonalSamplerVoice.{h,cpp}`. CMake
  `VERSION` bumped to `1.11.3` so the About tab and bundle plist reflect
  the patch.
- **Validation:** hammer note-steal patterns (fast repeated notes
  exceeding the polyphony cap) and confirm clean tail-fades on every
  steal — no clicks, no silence on the stolen voice's tail. The four fixes
  are independent; only the renderTailRamp restructure is audible under
  normal play.

## [1.11.2] - 2026-05-02

### Security
- **Path traversal in drag-drop streaming surface — fixed.**
  `dropSessionAddFile` (Source/PluginEditor.cpp) previously forwarded the
  JS-supplied `relPath` straight to `juce::File::getChildFile` and then to
  `replaceWithData`. With no validation, a malicious WebView page could
  pass `../etc/passwd`, an absolute path, or a backslash-escaped Windows
  path and write outside the session-scoped temp dir
  (`/tmp/o-microtonalsampler-drop-<id>/`). v1.11.2 introduces
  `Source/DropSessionGuard.h::validateRelPath` which rejects empty,
  absolute, backslash-separated, NUL-bearing, or `..`-segment paths
  *before* any allocation, plus `validateParentChain` which walks the
  target's existing ancestors and rejects any chain that traverses a
  symbolic link or exits the session dir. (REVIEW finding **CR-02**.)
- **Unbounded base64 streaming — capped.**
  `dropSessionAddFile` had no per-file or per-session size cap; a hostile
  page could trivially OOM the DAW host with a single multi-GB base64
  string. v1.11.2 enforces a **256 MB per-file** cap and a **4 GB
  per-session** cap (`kMaxFileBytes` / `kMaxSessionBytes`). The check
  uses the projected decoded size (`base64.length() * 3 / 4`) and runs
  *before* the decode buffer is allocated, so an oversized payload is
  rejected without ever touching memory. The aggregate counter
  (`currentDropSessionTotalBytes`) is reset in `dropSessionStart` and
  incremented only after a successful write. (REVIEW finding **CR-03**.)

### Tests
- **New regression test: `Source/tests/drop_session_guard_check.cpp`.**
  Standalone executable (build with `ninja
  O-MicrotonalSampler_DropSessionGuardCheck`). 24 assertions — including
  the headline `../etc/passwd` rejection, a >256 MB per-file rejection,
  a >4 GB session-aggregate rejection, and a real-symlink escape attempt
  on POSIX. Returns exit code = number of failed cases (0 = all pass).

### Notes
- **No state-format / parameter / preset / API changes.** Sessions saved
  on v1.11.1 reload identically — the security fixes live entirely on the
  drag-drop surface. Existing in-memory sessions, preset banks, and host
  automation lanes are unaffected.
- **User-visible behaviour change is rejection-only.** A well-formed
  drag-drop of a sample folder under 4 GB (the documented 250 MB
  reference library size leaves ~16× headroom) behaves exactly as in
  v1.11.1. A malicious or malformed payload now fails fast with a
  `dropSessionAddFile` DBG line and the JS bridge sees `false`.
- **Files touched:** new `Source/DropSessionGuard.h`,
  `Source/PluginEditor.cpp` (3 edits in `dropSessionStart` /
  `dropSessionAddFile`), `Source/PluginEditor.h` (new
  `currentDropSessionTotalBytes` member), new
  `Source/tests/drop_session_guard_check.cpp`, `CMakeLists.txt`
  (VERSION bump + test target + DropSessionGuard.h listed in source set).

## [1.11.1] - 2026-05-02

### Fixed
- **Octave-off bug — sample filenames now parse with C3=60 convention.**
  Playing a key labelled `G1` in the host DAW produced audio at `G2` pitch
  (one octave too high). Root cause: `Source/FilenameParser.cpp` parsed
  scientific-pitch tokens with the C4=60 (Yamaha/JUCE-native) convention
  via `midi = (octave + 1) * 12 + semitoneOffset`, while every dominant
  DAW (Ableton Live, Cubase, FL Studio, Logic Pro, Pro Tools, Reaper
  default) labels middle C as C3 = MIDI 60. A user folder of `G0.wav,
  G1.wav, G2.wav, …` recorded in DAW-native labelling was therefore
  stored at cell `midiNote` values one octave below the actual recorded
  pitch; on playback, `MicrotonalSamplerVoice::computePlayRateForVariant`
  (Source/MicrotonalSamplerVoice.cpp:113-123) computed a 2× ratio
  (`desiredFreq / cellRefFreq`) and transposed the sample up an octave.
  Switched the parser to `midi = (octave + 2) * 12 + semitoneOffset` and
  updated the matching UI label formula in `Resources/ui/js/sampler-app.js`
  (`midiToNoteName`) so cell labels stay in lockstep with parsed MIDI
  numbers. Inline parser tests rebased onto the new convention (C3=60
  anchor case added; previous C4-based assertions shifted by 12).

### Notes
- **No state-format / parameter / preset changes.** Sessions saved on
  v1.11.0 reload identically — `SampleMap` cell `midiNote` values are
  rebuilt from filenames at folder-load time, not persisted, so the new
  convention takes effect on next folder load. Existing in-memory
  sessions stay valid until the user reloads samples.
- **Label shift visible in the Sample Map grid.** Cells previously
  labelled `C4` will now read `C3`, `C5` will read `C4`, etc. — pitches
  unchanged, only the displayed octave numbers move down by one to match
  the host DAW's ruler.
- **Compatibility caveat — folders named in C4=60 (Yamaha) convention.**
  Users whose sample folders were named to match the *previous* parser
  convention (e.g. samples actually recorded at MIDI 60 named `C4.wav`
  in JUCE-native form) will see their folders load one octave low after
  this update. Workaround: rename the folder so each filename's octave
  digit is one lower (e.g. `C4.wav → C3.wav`), or re-export from the DAW
  to pick up its native labelling. The C3=60 default matches the vast
  majority of modern DAW exports.
- **Files touched:** `Source/FilenameParser.cpp` (formula + comment +
  inline tests at lines 431-490), `Resources/ui/js/sampler-app.js`
  (`midiToNoteName` at lines 595-604), `CMakeLists.txt` (VERSION bump).

## [1.11.0] - 2026-05-01

### Added
- **Paper-texture backgrounds.** The page background and all card surfaces
  (header, About card, Tuning panel container) now ride on antique paper
  textures instead of solid cream/warm fills. `paper1.jpg` (964×598) drives
  the page body via `center/cover` with a faint warm-tint overlay so the
  existing palette tokens (text, accent-gold, border-warm) keep their
  intended contrast. `paper2.jpg` (516×885) drives the card surfaces under
  a translucent `--bg-card` overlay so the parchment grain reads through
  without sacrificing legibility.

### Changed
- **About → "Ouaricon" link** now points to `https://oaudio.io/` (was
  `https://ouaricon.com`).

### Fixed
- **About-tab version pill was hardwired to v1.0.0 across every release.**
  Plugin `CMakeLists.txt` used `PLUGIN_VERSION "x.y.z"`, which is **not** a
  recognized `juce_add_plugin` keyword — JUCE silently dropped it and fell
  back to `PROJECT_VERSION` from the root `project(JUCEPlugins VERSION
  1.0.0)` declaration. The About tab's `getPluginVersion` native function
  returns `JucePlugin_VersionString`, which was therefore stuck at
  `"1.0.0"` for every shipped version (v1.0.0–v1.10.0). Renamed the arg to
  the correct `VERSION "1.11.0"` so future bumps wire through to the About
  pill and the bundle plist (`CFBundleShortVersionString`) automatically.

### Notes
- Implementation: paper textures embedded via `juce_add_binary_data` in
  `CMakeLists.txt` and served by `PluginEditor.cpp` resource provider at
  `/images/paper1.jpg` and `/images/paper2.jpg`. CSS uses layered
  `background` (tint gradient + image + solid fallback) so the resource
  provider failing degrades gracefully to the previous v1.10.0 cream.
- Pure visual + housekeeping change. No DSP, parameter, sample-map, or
  preset-format changes.
- v1.10.0 backup created at `backups/O-MicrotonalSampler/v1.10.0/` (was
  missing — every prior release backed up its own predecessor except this
  one).

## [1.10.0] - 2026-05-01

### Added
- **Naturalist aesthetic — anatomical brain overlay.** Antique anatomical
  engraving (cerebrum + central nervous system) layered behind the UI as a
  subtle decorative overlay, matching the Ouaricon Naturalist aesthetic
  established in O-Lyrica (botanical fern overlay). Image is sepia-tinted
  to lock into the cream/warm-brown palette, sits behind all interactive
  content (z-index: 0) with `pointer-events: none` so it never blocks
  input.
- **Tab-aware parallax positioning.** Overlay slides subtly between tabs:
  - **Sample Map** — peeks from right edge (right: -60px, opacity 0.18)
  - **Tuning** — retreats further right (right: -120px, opacity 0.13)
  - **About** — swings into full view as a feature image (right: 40px,
    opacity 0.32)
  Transitions are 0.45s ease-out for both `right` and `opacity`.

### Notes
- Pure visual addition. No DSP, parameter, or behavior changes — the
  v1.9.1 sample-map / round-robin / merge-rr surface is untouched.
- Implementation pattern lifted verbatim from O-Lyrica v1.4.0
  (`.botanical-overlay`): single `<img>` element absolutely positioned
  inside `#app`, served via the existing WebView resource provider, tab
  switcher swaps a position class. Image is embedded as `BinaryData`
  via `juce_add_binary_data` in `CMakeLists.txt`.
- File: `Resources/ui/images/brains.png` (~334 KB).

## [1.9.1] - 2026-05-01

### Fixed
- **`Layer as round-robin` load mode silently fell back to `ReplaceAll`,
  wiping the entire sample map.** The JS folder-load modal correctly emits
  `"merge_rr"` for the new v1.9.0 mode, but the C++ string→`LoadMode`
  translation in `PluginEditor.cpp` only handled `"append"` and
  `"replace_layer"` — every other string (including `"merge_rr"`) hit the
  `LoadMode mode = LoadMode::ReplaceAll;` default, so picking the new mode
  replaced the existing map instead of merging. Affected both load paths:
  `loadSampleFolderDialog` (file-chooser, line 294) and
  `dropSessionCommitFolder` (drag-drop folder, line 491). Added the
  missing `else if (modeStr == "merge_rr") mode = LoadMode::MergeRR;`
  branch in both blocks.

### Notes
- v1.9.0 backend (`applyMergeRrCell`, `LoadMode::MergeRR`,
  `loadModeToString`/`loadModeFromString`) was already correct — the bug
  was purely at the WebView→C++ translation boundary, so the
  `O-MicrotonalSampler_MergeRrCheck` standalone test target kept passing
  even with the bug live.
- Behaviour after fix (verified in DAW): load folder A as ReplaceAll, then
  load folder B with "Layer as round-robin" — folder A's cells outside
  B's range persist; cells where A and B overlap gain B's variants
  appended onto A's existing variant vectors.

## [1.9.0] - 2026-05-01

### Added
- **`Layer as round-robin` load mode.** The folder-load options modal grows
  a 4th radio: **Layer as round-robin**. With this mode selected, a freshly
  loaded folder is merged into the existing sample map; on (note, layer)
  collisions, the new cell's variants are **appended** onto the existing
  cell's variants vector instead of replacing it. Useful for layering
  multiple takes/recordings as round-robin alternates on the same notes
  without needing to relabel filenames with `rr/take/tk` tokens.
- **Per-cell single-file merge prompt.** Triggering a per-cell sample load
  (cell button, double-click on a loaded cell, or context-menu Replace) on
  a cell that already holds samples now surfaces a small confirm dialog:
  *"Add as round-robin variant N+1, or replace?"*. Empty cells skip the
  prompt and load directly (v1.8.0 behaviour preserved).
- **Variant cap (64 per cell).** Both load paths enforce a hard cap of 64
  variants per (note, layer). Excess incoming variants are surfaced via the
  existing skipped-files list (`variant cap reached: <filename>`). At cap,
  the per-cell merge button is disabled and only Replace remains.

### Changed
- **`LoadMode` enum.** New `MergeRR = 3` value, serialized as `"merge_rr"`
  in the load-op history. Older builds (v1.8.0/v1.7.x) reading a v1.9.0
  preset fall back to `ReplaceAll` for unknown mode strings (graceful
  degradation: cells survive in the snapshot, but merge ops won't replay).
- **`loadSingleSample`** gains an optional `mergeAsRr` parameter (default
  false — preserves v1.8.0 callers). When true and the target cell is
  non-empty, the new variant is appended; when false, the cell is replaced
  as before. `loadSingleSampleDialog` native function accepts the flag as
  its 3rd arg.
- **`applyMergeRrCell` helper** extracted into `SampleMap.h` (header-only
  pure function). Used by both folder-load `MergeRR` mode and the per-cell
  merge path; isolates the merge contract for unit testing.

### Implementation notes
- **Backward compat.** `getStateInformation`/`setStateInformation` schema is
  unchanged. v1.7.x and v1.8.0 saves replay identically. v1.9.0 saves with
  `merge_rr` ops opened in v1.8.0 fall back to `ReplaceAll` for those ops
  (per existing `loadModeFromString` default).
- **RT-safety contract preserved.** Merge work happens entirely on the
  message thread (same path as v1.8.0 `applyFolderLoad`). The
  `currentSampleMap` shared_ptr is atomic-stored after the merge; voices
  holding the previous snapshot keep their buffers alive transitively for
  the held note's duration. RR counters for every touched cell reset to
  the sentinel so the next note-on doesn't index past the just-grown
  variants vector with a stale value.
- **Drag-drop scope.** v1.9.0 surfaces the per-cell merge prompt for the
  file-picker path (cell button, dblclick, context-menu Replace). Drag-drop
  of a single file onto a non-empty cell still uses v1.8.0 replace behaviour
  to keep the multi-file drop session UX uninterrupted; use the cell's
  load-sample button or the folder-load `Layer as round-robin` mode for
  explicit RR layering. (Drag-drop merge is a candidate for v1.9.x.)

### Test surface
- New standalone `O-MicrotonalSampler_MergeRrCheck` target — six unit
  tests over the `applyMergeRrCell` helper: no-collision insert, collision
  merge with order preservation, variant cap (64), cap-already-reached
  early-out, layer-aware collision key, multi-call folder-shape ordering.
- v1.8.0 round-robin render harness untouched — render path is bit-identical
  for non-merge loads (single-variant + token-RR libraries).

## [1.8.0] - 2026-05-01

### Added
- **Round-robin sample variants.** A single (note, velocity layer) cell can
  now hold multiple sample takes. At every note-on, the engine picks one
  variant according to the user-selected RR mode. Single-variant cells are
  unchanged — the render path is bit-identical to v1.7.1 for libraries
  without RR tokens.
- **Three selection modes** via a new `Round-Robin Mode` parameter
  (`rr_mode`):
  - **Cycle** — sequential `0 → 1 → … → N-1 → 0`, deterministic.
  - **Random No-Repeat** *(default)* — uniform random pick excluding the
    last-played variant, the industry standard for orchestral/percussive
    libraries.
  - **Random** — uniform random, may repeat. Useful for foley/ambience.
- **Filename token detection.** The folder loader now recognises
  `rr[N]`, `take[N]`, and `tk[N]` tokens (case-insensitive, 1-based) and
  groups files sharing the same `(note, layer)` into one cell as silent
  variants. Examples that load without prompting:
  `vln_C4_v1_rr1.wav`, `kick_C2_take03.wav`, `cello_g3_tk2.aif`.
- **Ambiguity confirmation modal.** Folders with bare duplicates (same
  `(note, layer)` but no rr/take/tk tokens) now surface a WebView modal
  listing the conflicting filenames. The user can either accept them as
  RR variants or cancel the load — protects against accidental ingest of
  redundant samples.
- **Per-variant loop editor.** When a cell has more than one variant, the
  loop editor side panel grows a tab strip (`Variant 1 of N` indicator +
  one numbered tab per variant). Each tab carries its own loop start/end,
  loop mode, and apply/reset state — every variant can be tuned independently.
- **Per-cell variant tooltip.** Multi-variant cells in the sample grid
  display a small antique-gold dot in the upper-right corner and a
  multi-line hover tooltip listing every variant's filename in load order.
- **`confirmRoundRobinLoad(accept)` native function.** Exposed for the
  modal's accept/cancel buttons; chains correctly through the v1.6.0 state-
  restore replay queue so reopened projects with ambiguous folders surface
  the modal sequentially without losing later ops.

### Changed
- **`SampleSlot` → `SampleCell` + `SampleVariant`.** Internal sample-map
  storage refactored — a cell is the addressable `(midi, layer)` coordinate;
  variants hold the audio + per-take loop fields. `findSlot` → `findCell`.
  Render path semantically identical for single-variant cells.
- **`SampleMap` JSON schema.** The snapshot now carries a `cells` array
  (each with a `variants[]` sub-array). The legacy `slots` array is still
  emitted for back-compat — primary variant per cell, plus a new
  `variantCount` field so older consumers can detect multi-variant cells.
- **Per-cell single-load behavior.** Clicking an empty cell to load a single
  sample replaces the whole cell with a one-variant cell, even if the cell
  previously held a multi-variant set. To build a multi-variant cell, use a
  folder load with rr/take/tk tokens (or accept the bare-duplicate modal).

### Implementation notes
- **RT-safety contract preserved.** Variant selection is pure atomic-counter
  + integer math + xorshift32; zero allocations in `startNote` or
  `renderNextBlock`. The 512-byte counter array (128 notes × 4 layers,
  `std::atomic<uint8_t>`) lives in the processor and survives map swaps —
  reset only on `LoadMode::ReplaceAll` and per-layer wipes for
  `ReplaceLayer`.
  - *Deviation from spec:* the plan called for 352 entries (88-key range);
    we use 512 (full 0..127 × 4) for index-bound safety. ~0.16 KB difference.
- **Per-voice xorshift32 PRNG.** Seeded from the voice's `this` pointer +
  sample rate so each voice gets a distinct stream; mutated only in
  `selectVariantIndex` (audio thread, startNote-time, never per-sample).
- **Atomic-swap semantics intact.** Cell vector deep-copy is still cheap
  (each variant's audio is a `shared_ptr<juce::AudioBuffer<float>>`). Voices
  that snapshot the map at startNote keep variants alive transitively for
  the note's duration even if the map is replaced mid-note (Stage 2 EC-3).
- **Preset compatibility.** `getStateInformation` / `setStateInformation`
  schema is unchanged — sample data is still referenced by folder path, not
  embedded. Replaying a v1.7.x save in v1.8.0 simply rebuilds single-variant
  cells via the same folder-load path. v1.8.0 saves opened in v1.7.x will
  still load (the new `rr_mode` parameter is silently ignored by APVTS;
  folder paths replay identically).

### Test surface
- Render-harness identity test passes for single-variant libraries —
  bit-identical output vs v1.7.1.
- New `FilenameParser::runTests` cases cover `rr1..rr64`, `take01..take64`,
  `tk1..tk2`, both pre- and post-note placement, and rejection of
  unrecognised tokens (`round1`, `var3`, etc.).
- pluginval `--strictness-level 5 --skip-gui-tests` SUCCESS.
- auval `-v aumu OMtS OuDv` AU VALIDATION SUCCEEDED.

### Known limits (deferred to v1.9)
- Per-variant velocity sub-layering.
- Cross-cell RR group tagging (e.g., "all snares in this folder share one
  cycle").
- Per-cell RR algorithm override (the `rr_mode` parameter is global for v1.8).

## [1.7.1] - 2026-05-01

### Fixed
- **Tuning panel note highlights now appear when notes are played.** The
  Circle and Polar visualizations and the True Keys interval display were
  silent: pressing keys never lit up scale degrees and True Keys never
  showed intervals. **Root cause:** the C++ side was not publishing any
  MIDI activity to the WebView. There was no active-note tracking on the
  synth, no editor timer, and no JavaScript handlers for the events the
  TuningPanel expects (`tuningNoteOn` / `tuningNoteOff` / `updateHeldNotes`).
  In short, the wiring between the audio engine and the panel was missing
  end-to-end. (This was a latent gap from Phase 3.1 — the panel was
  designed to receive these events but the producer side was never built.)

- **Polar view now highlights active scale degrees.** Even with the wiring
  fixed above, the polar plot would still not respond to held notes —
  `drawPolarPlot()` ignored `activeScaleDegrees` entirely (every dot was
  drawn with the same fill colour) and `updateSpokeHighlights()`
  short-circuited for any mode other than Circle. Both have been fixed:
  active dots now render in the same red (`#C0392B`) the Circle view uses,
  with a slightly larger radius for emphasis.

### Implementation notes
- **Audio thread:** `CappedSynthesiser` now keeps two `std::atomic<uint64>`
  bitmasks (low = MIDI 0–63, high = 64–127) updated via lock-free
  `fetch_or` / `fetch_and` inside the existing `noteOn` override and a new
  `noteOff` override. No allocations, branch-free bit ops, no impact on
  `processBlock` cost.
- **Message thread:** `PluginEditor` now inherits `juce::Timer` and runs
  at 30 Hz. Each tick reads the bitmask, diffs against the previous
  snapshot, and emits per-note `tuningNoteOn` / `tuningNoteOff` events
  for new transitions plus a `tuningHeldNotes` payload (`{notes,freqs}`)
  for True Keys. Early-out when no bits changed — typical idle cost is
  one atomic load per tick.
- **TrueKeys frequencies:** the held-notes payload calls
  `TuningEngine::getFrequency(midi)` per note so the cents readout
  reflects the active microtonal tuning, not 12-TET.
- **Late mount catch-up:** the TuningPanel is mounted lazily on first
  Tuning-tab activation. A new native function `getHeldNotesJson` lets
  the panel pull current state at mount, so notes already held when the
  user clicks the Tuning tab show up immediately. Subsequent updates
  flow through the timer-driven events.
- **Polar redraw:** `updateSpokeHighlights()` now falls through to
  `drawPolarPlot()` when in polar mode. The cost is 12 dots redrawn at
  up to 30 Hz — well below any perf threshold.

## [1.7.0] - 2026-04-30

### Added
- **Expression control for dynamics (MIDI CC 11).** New `expression` APVTS
  parameter (0–100 %, default 100 %) wired to MIDI Continuous Controller 11
  (the industry-standard "Expression" controller for orchestral mockups).
  Incoming CC 11 messages drive the parameter via `setValueNotifyingHost`
  so DAW automation lanes mirror live controller input — last-touched wins.
- **Expression knob on the bottom control strip.** Bound to the new
  parameter via `WebSliderRelay` / `WebSliderParameterAttachment`, so
  knob, host automation, and CC 11 stay synchronised.

### Behaviour
- Expression is **independent of velocity-layer selection.** Velocity
  (note-on velocity) still selects which layer plays at note-on; the
  expression knob scales the post-mix output afterwards. Mid-note
  expression changes therefore change loudness without retriggering or
  crossfading layers — matching Kontakt / Spitfire convention.
- **Curve:** squared (final gain = expression²). Sampler convention; gives
  smoother fades and a more "natural" feel than a linear curve.
- **Smoothing:** 10 ms per-block linear ramp (mirrors the existing
  `output_gain` smoother — RESEARCH R7, pitfall #8). Sample-accurate
  per-event smoothing was deemed unnecessary; the 10 ms ramp covers
  per-block CC jumps without zipper noise.
- **Signal chain:** voices → expression gain → output gain → output. The
  two stages multiply, so global trim and dynamics are independent.

### Implementation notes
- `processBlock` scans the MIDI buffer for CC 11 (last-value-wins per
  block) before `renderNextBlock`. The CC's 0–127 value maps directly
  to the parameter's 0..1 normalised range. The squaring happens at
  gain-application time, not at parameter-write time, so host automation
  and the knob both expose a clean linear 0–100 % surface.
- New parameter is added at the end of the layout (between
  `velocity_crossfade` and `output_gain`). Existing presets / sessions
  load with expression at its default (100 %) — no breaking change.

## [1.6.0] - 2026-04-30

### Added
- **Explicit velocity-layer assignment for folder loads.** Both the
  Load Folder… button and macOS folder drag-drop now open a "Load
  samples" modal before any scan/streaming work begins. The modal
  exposes three controls:
  - **Layer (L0–L3):** segmented selector for the target velocity row
    (4 layers, matching the existing grid).
  - **When loading:** Add to layer / Replace this layer / Replace all
    samples (merge mode).
  - **Ignore filename velocity tokens:** checkbox that forces every
    incoming sample onto the chosen target layer regardless of
    `_v1`/`_ff`/`layer3`/etc. in the filename.

  A live explainer below the controls describes the resulting
  behaviour for the current settings (e.g. _"Add samples to L2,
  ignoring filename velocity tokens"_) so the user can preview
  the load before confirming.

- **Multi-folder sample maps.** Append mode merges new samples into
  the existing map without wiping it, so a single bank can be assembled
  from several drops (e.g. drop a "soft" folder onto L0, then drop a
  "loud" folder onto L3 with override on, and both layers play under
  velocity-crossfade as expected). `(midi, layer)` collisions are
  overwritten by the most recent drop.

- **Load-op history persisted in plugin state.** Every successful
  folder load is recorded in `loadOpHistory` and written to plugin
  state as `<SampleFolders><Op …/>…</SampleFolders>`. On project
  reopen, the ordered op list is replayed sequentially via the same
  pipeline so the multi-folder map is faithfully reconstructed —
  including target layer, merge mode, and override flag.

- **Tolerant state replay for missing folders.** If any persisted op
  references a folder that no longer exists on disk, the existing
  missing-folder modal is surfaced for the first one and subsequent
  ops continue (silently skipped) so a partial reload is still useful.

### Changed
- **`SampleLoader::loadFolder` signature now takes `LoadOptions`.**
  When `overrideTokens=true`, every parsed slot is forced onto the
  caller-supplied target layer; when `false`, filename tokens win
  (legacy v1.5.x behaviour). Default-constructed options reproduce
  v1.5.x exactly.
- **`OMicrotonalSamplerAudioProcessor::loadSampleFolder` signature now
  takes `(folder, targetLayer, mode, overrideTokens)`** with defaults
  `(file, 0, LoadMode::ReplaceAll, false)` so the missing-folder
  relocate path and any internal callers retain v1.5.x semantics
  without code changes.

### Migration notes
- **No breaking changes.** Old saved state still loads:
  `<SampleFolder path="…"/>` from v1.5.x and earlier is detected and
  replayed as a single ReplaceAll op with target layer 0 and override
  off, so v1.5.x sessions/presets behave bit-for-bit identically. New
  saves emit the `<SampleFolders>` op-list container instead — old
  versions of the plugin loading a v1.6.0 save would simply ignore
  the unknown sibling and start with no folder loaded (graceful
  forward incompatibility).
- **Drag-drop folder loads on macOS materialise into a temp dir and
  are NOT persisted across save/reopen.** This matches existing
  v1.0.4 behaviour. Users who need persistence should use the
  Load Folder… button (which records the original folder path).

## [1.5.2] - 2026-04-30

### Changed
- **Tuning tab — intervals table is taller.** Reclaimed the empty vertical
  space below the intervals list by raising `.interval-list` `max-height`
  from `300px` to `400px` in `Resources/ui/css/tuning-panel.css`. Lets more
  degrees stay in view at once before scrolling kicks in. Editor default
  is 900×640 so 400 px still fits comfortably (62% of editor height) and
  scales down with the responsive layout.

### Migration notes
- **No breaking changes.** Pure CSS-only edit (one property). No parameter,
  state, preset, or layout-grid changes. v1.5.1 sessions/presets load
  identically.

## [1.5.1] - 2026-04-30

### Fixed
- **Tuning tab — visualization area top-justified.** The Circle / Polar /
  Matrix / True Keys / Rotation views in the center column are now
  anchored to the top of the viz container instead of vertically
  centered, eliminating the dead space between the viz-mode buttons and
  the visualization content. (`align-items: center` →
  `align-items: flex-start` on `.viz-view.active`.)
- **Tuning tab — wide tables no longer overflow the right edge.** The
  Matrix and Rotation tables previously expanded the center grid column
  past its allotted `1fr` width, pushing past the right boundary of the
  Controls panel. Root cause: CSS Grid items default to `min-width: auto`
  (= min-content), so any descendant wider than `1fr` blows the column
  out. Fix: `min-width: 0` on `.tuning-center-column` makes the column
  respect its track size, and `overflow: auto` on `.viz-container`
  scrolls wide content within the middle section instead of pushing
  past the right edge.

### Migration notes
- **No breaking changes.** Pure CSS-only fix in
  `Resources/ui/css/tuning-panel.css` (3 rule edits). No parameter,
  state, preset, or layout-grid changes. v1.5.0 sessions/presets load
  identically.

## [1.5.0] - 2026-04-30

### Changed
- **Loop editor panel is now inline** below the sample-map grid instead of
  sliding in from the right edge as a 360-px drawer. The panel is always
  visible — when no slot is selected it shows the placeholder text
  "Select a loaded sample slot to edit loop points"; selecting a loaded
  slot swaps in the waveform canvas, loop-point readouts, and action
  buttons. Reuses the existing blank space between the grid and the
  bottom knob row, so the waveform editor is reachable without any
  horizontal layout shift of the grid.
- **Close button (×)** now deselects the current slot (returns the panel
  to its placeholder state) instead of dismissing the entire editor.

### Removed
- `body.le-open` right-padding shift on `#tab-samplemap` (no longer
  needed — the inline panel doesn't overlap the grid).
- Slide-in transform/transition CSS on `#loop-editor-panel`.

### Migration notes
- **No breaking changes.** No parameter, state, or preset format changes.
  v1.4.0 sessions/presets load identically. Pure UI/UX layout change.

## [1.4.0] - 2026-04-30

### Changed
- **Loaded samples now loop the entire file by default.** Each slot is
  initialized with `loopStart = 0`, `loopEnd = N - 2`, `loopMode = Auto`
  on load. The renderer's existing 8-sample equal-power crossfade at
  the wrap handles click prevention. Replaces the v1.0–v1.3 RMS-based
  auto-detector that searched for a quiet sustain region in the latter
  60% of the file.
- **"Reset" in the loop-point editor** now snaps the slot back to
  whole-file loop instead of re-running auto-detect.

### Fixed
- **V11-LOOP-FALLBACK** (deferred from Stage 4 verification): sustained
  material with constant RMS (sine waves, drones, organ samples) used
  to fall through the auto-detector's variance gate and silently revert
  to one-shot, going silent before note-off. With whole-file loop as
  the default, these samples now sustain correctly.

### Removed
- `Source/LoopDetector.{h,cpp}` (Phase 2.5 RMS scan + variance gate +
  zero-crossing snap module — ~230 LOC) and the include sites in
  `SampleLoader.cpp` and `PluginProcessor.cpp`. The detector's
  defensive constraints (`loopEnd <= N - 2`, min loop length 16) are
  preserved as inline guards in the new whole-file path.

### Migration notes
- **Behavior change, not breaking.** v1.3.0 sessions/presets load
  cleanly. State persistence is unaffected (no parameter changes, no
  state schema changes). Audio output for one-shot percussive samples
  may differ — they now loop the whole file by default. Use the
  per-slot loop-point editor (Stage 3 UI) to set Manual loop points
  if a particular sample needs different behavior.

### Root cause notes
- The original auto-detector was tuned for sustained instrumental
  material with a clear noise-floor sustain region (e.g. piano
  release tails). On constant-RMS or transient material it
  conservatively rejected and fell back to one-shot — surprising
  default behavior for "load a sample and play it as a sustained
  pitched instrument," which is what the plugin is for.

## [1.3.0] - 2026-04-29

### Added
- **Full state persistence across DAW sessions.** Reopening a project
  now restores the loaded sample folder, tuning state (intervals, A4
  master tune, octave stretch, tonic, mode, KBM mapping), and all
  parameter values exactly as they were when the project was saved.
  Pre-v1.3.0 only persisted parameters — folder and tuning were lost.
- **Save/Load preset (`.omspreset`)** buttons in the header. Captures
  the same state used for project save/load (params + folder path +
  tuning) as a portable XML file. Per design Q1=A: paths only — sample
  audio is referenced, not embedded, so presets stay small but require
  matching folder structure across machines.
- **Missing-folder modal.** When DAW project reopen finds the saved
  folder no longer exists at its original path, a modal surfaces the
  path and offers "Locate folder…" (file picker, reuses
  `loadSampleFolder`) or "Skip" (clears pending state, sampler stays
  empty).

### Changed
- `PluginProcessor::getStateInformation` / `setStateInformation` now
  serialize a wrapped `ValueTree`: APVTS state plus `<SampleFolder>`
  and `<TuningState>` sibling children. Backward-compatible — v1.2.0
  sessions load cleanly (children absent → defaults), v1.3.0 sessions
  in v1.2.0 silently drop the new children.
- Tuning state is captured via the engine's existing accessors plus
  `generateScalaFileContent` / `generateKBMFileContent` round-trips,
  so no fork of the shared `scala-tuning-engine` module is required.
- Added 5 native functions to the WebView bridge:
  `saveCurrentPreset`, `loadPreset`, `locateMissingFolder`,
  `dismissMissingFolder`, `getPendingMissingFolder` — the last covers
  the boot-time race where state restore runs before the WebView has
  registered its `folderMissing` listener.

### Technical notes
- **Root cause** (pre-v1.3.0): `getStateInformation` only emitted
  `parameters.copyState()`, which is APVTS-only. The `currentSampleMap`
  was rebuilt from a folder reference held in memory but never written
  to the persisted state.
- **Threading**: `setStateInformation` runs on the message thread.
  Tuning restore is in-memory and synchronous; folder reload reuses
  the existing async `SampleLoader`. Missing-folder detection is
  synchronous (`File::isDirectory()`); the modal is surfaced via
  `emitEventIfBrowserIsVisible` plus a parked-path pull on WebView
  attach to cover the boot-time race.
- **Backup**: `backups/O-MicrotonalSampler/v1.2.0/` (rollback path).

## [1.2.0] - 2026-04-29

### Added
- **Tuning tab is now an editable authoring surface.** Reverses the
  Stage 3 §RQ3-1 read-only design. Users can:
  - **Select factory tunings** from the library (24+ presets across
    Historical, Just Intonation, Equal Divisions, Non-Octave, World).
  - **Load `.scl` (Scala scale) and `.kbm` (keyboard mapping) files**
    via native file pickers. Save also supported.
  - **Edit individual interval cents** by typing into the table on
    the left.
  - **Change tonic** (rotates 12-note scales).
  - **Adjust A4 reference pitch** (400–480 Hz) via the round knob.
  - **Apply octave stretch** (0.95–1.25 ×) for physical-modeling
    voicings.
  - **Generate scales** from EDO, harmonic series, or rank-2
    temperament parameters and apply them to the engine.
  - **Export the current tuning** as an HTML documentation page
    (with SVG pitch circle).
- Tuning Library and Scale Generator sections auto-expand on first
  Tuning-tab activation, so the right column shows selectable items
  immediately.

### Changed
- `PluginEditor.cpp` registers ~13 new WebView native functions that
  bridge `tuning-panel.js` calls to the shared `scala-tuning-engine`
  module: `setSingleInterval`, `setTonicNote`, `setOctaveStretch`,
  `setMasterTune`, `loadEmbeddedTuning`, `loadScalaFile`,
  `loadKBMFile`, `saveScalaFile`, `saveKBMFile`, `generateEDO`,
  `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`,
  `exportTuningHTML`. All file-picker variants use
  `juce::FileChooser::launchAsync` with a `shared_ptr` capture so the
  chooser outlives the async callback.
- `index.html` no longer links `tuning-panel-readonly.css`. The
  read-only CSS file is preserved on disk and as a binary resource
  for backward compatibility but is no longer applied.
- `sampler-app.js` removes the `applyIntervalReadonlyShim` span-swap
  and its `MutationObserver`; the editable `.interval-input`
  elements are now visible and wired to `setSingleInterval` via the
  panel's existing `handleIntervalChange` flow.

### Root Cause
- **Empty intervals table** — TWO root causes:
  1. `.interval-input` rows were hidden by `tuning-panel-readonly.css`
     and replaced with a static `interval-display` span.
  2. **Pre-existing latent bug since v1.0**: `tuning-panel.js` was
     instantiated with `window.__JUCE__` (the low-level postMessage
     handler), but every backend call inside the panel uses
     `juceApi.getNativeFunction(name)` — that method lives on the
     ES-module namespace `Juce` (imported in sampler-app.js as
     `import * as Juce from './juce/index.js'`), NOT on
     `window.__JUCE__`. Every call (`getTuningIntervals`,
     `getEmbeddedTuningList`, `setSingleInterval`, `loadScalaFile`,
     `generateEDO`, etc.) silently threw a `TypeError` and was
     swallowed by the panel's try/catch blocks. Fixed by passing
     `Juce` instead: `new TuningPanel(container, Juce)`.
- **Library: categories visible but no tunings** — `library-content`
  was collapsed by default; `loadEmbeddedTunings()` only fired on
  toggle-expand. Even after manual expansion, clicking an item
  silently failed because the write-side native function
  `loadEmbeddedTuning` was not registered.
- **Missing Load .SCL / .KBM buttons** — `.tuning-file-section` was
  hidden by the readonly CSS overlay, and the underlying
  `loadScalaFile`/`loadKBMFile` natives were never bridged to JS.

### Notes
- **Dorico microtonal playback is preserved.** The shared
  `TuningEngine` remains the single source of truth. Library/file
  loads call `setCustomIntervals()`, which is the same path VST3
  Note Expression overrides per-note at note-on time.
- **No state-format or APVTS changes.** Existing presets and
  sessions load unchanged.
- The `tuning-panel-readonly.css` stylesheet is intentionally kept
  in `Resources/ui/css/` and in `juce_add_binary_data` so a future
  variant could re-enable read-only mode by re-linking it from
  `index.html`.

## [1.1.0] - 2026-04-29

### Added
- Sample-map grid axis labels: velocity-range row labels on the left
  (`97–127`, `65–96`, `33–64`, `1–32`) and C-note column labels below
  (`C1`–`C8`). Velocity labels stay visible during horizontal scroll;
  C labels pan with the grid.
- Cell hover tooltip now shows note name, MIDI number, and velocity
  range. Format: `<filename | Empty> · <NoteName> (<midi>) · Vel <lo>–<hi>`
  (e.g. `vlnsolo_C4_mf.wav · C4 (60) · Vel 65–96`).

### Changed
- `renderGrid()` populates new `#sample-grid-vel-labels` (sidebar) and
  appends `#sample-grid-col-labels` inside the scroll container.
- New helpers `velocityLayerToRange(layer)` and `midiToNoteName(midi)`
  in `sampler-app.js`. Velocity ranges match
  `MicrotonalSamplerVoice.cpp` quartile layer mapping
  (`layerWidth = 128/4 = 32`).

### Notes
- Pure UX/cosmetic change. Cell DOM structure unchanged
  (`.grid-cell` selector intact); drag-drop hit-testing
  (`reportCellLayout`), click routing, and context menu unaffected.
- No DSP, parameter, or state-format changes — preset/session
  compatibility preserved.

## [1.0.4] - 2026-04-29

### Fixed
- Drag-drop folder loading now actually loads samples on macOS (fourth
  attempt — finally working). Drag-drop a single `.wav`/`.aif` onto a
  grid cell also routes correctly via the same code path.

### Why v1.0.3's "fix" wasn't a fix
v1.0.3 moved drag-drop to the JS layer and tried to extract absolute file
paths from `DataTransfer` (`text/uri-list`, `public.file-url`,
`text/plain`). The empirical diagnostic on a real folder drop returned:

```
types=[Files]; files=1 (first: name="vlnsolo_flaut", size=0, type="",
path=undefined, webkitRelativePath=""); items=1 (file:?,
entry=dir:/vlnsolo_flaut); tried: file.path:0/1
```

WKWebView's sandbox strips absolute paths from JS for security; only
`Files` is exposed and `File.path` is undefined. No path-bearing type
was reachable through any combination of `getData(...)` calls. The fast
path was therefore unreachable in production.

### Fix
v1.0.4 streams file *content* through the WebView↔native bridge into a
session-scoped temp dir and runs the existing `loadSampleFolder` /
`loadSingleSample` paths against that temp dir, as if the user had picked
it from a `juce::FileChooser`.

JS side (`sampler-app.js`):
- On drop, `dataTransfer.items[0].webkitGetAsEntry()` returns a
  `FileSystemEntry`.
- For `isDirectory` entries: walk the tree via
  `FileSystemDirectoryReader.readEntries()`, collect every `.wav` /
  `.aif` / `.aiff` (skip dotfiles), preserve relative paths.
- For `isFile` entries: take the single file.
- For each file: read via `FileSystemFileEntry.file(...)` →
  `File.arrayBuffer()` → chunked `String.fromCharCode` → `btoa()` for
  base64. Stream `(sessionId, relativePath, base64)` to C++ via
  `dropSessionAddFile` native function.
- Commit via `dropSessionCommitFolder(sessionId)` or
  `dropSessionCommitFile(sessionId, relPath, midi, vel)`.
- DOM hit-test via `document.elementFromPoint(...)` chooses the routing
  arm (cell vs folder zone vs out-of-bounds) so the existing C++
  `filesDropped` routing matrix (cell hit, folder-zone hit, mismatched
  payload toasts) is mirrored exactly.
- Progress feedback via the existing `showToast` (`Loading 5 of 88: …`).

C++ side (`PluginEditor.cpp`):
- 4 new native functions: `dropSessionStart`, `dropSessionAddFile`,
  `dropSessionCommitFolder`, `dropSessionCommitFile`.
- `dropSessionStart` creates `<temp>/o-microtonalsampler-drop-<sessionId>/`
  and calls `cleanupStaleDropSessions()` to delete prior session dirs
  older than 5 minutes (a window comfortably larger than typical
  SampleLoader read times — avoids racing an in-flight background read).
- `dropSessionAddFile` base64-decodes via `juce::MemoryBlock::fromBase64Encoding`
  and writes via `juce::File::replaceWithData` into the session dir.
- Commit functions call `processorRef.loadSampleFolder` /
  `processorRef.loadSingleSample` on the session temp dir / file. The
  async `SampleLoader` thread reads from there and posts the new
  `SampleMap` via the existing `sampleMapChangedCallback` channel — no
  changes to the loader, parser, loop detector, or grid renderer.

### Performance
Base64 has ~33% size overhead and string-encoding is on the JS message
thread. For a ~250 MB instrument library the streaming pass takes a few
seconds before the background `SampleLoader` starts; the loader itself
is unchanged from v1.0.0. The toast region updates per-file so the user
sees progress instead of a frozen UI.

### Preserved fast-path
The v1.0.3 path-extraction probe (`text/uri-list`, `public.file-url`,
`text/plain`, `File.path`) still runs first as defence-in-depth. If any
host eventually exposes paths (Linux/Win, future WebKit), the fast path
fires immediately and the streaming path is skipped — no rebuild needed
to take advantage of it.

### v1.0.3 → v1.0.4 file delta
- M `Source/PluginEditor.h` — `currentDropSessionId`, `currentDropSessionDir` members; `cleanupStaleDropSessions()` method
- M `Source/PluginEditor.cpp` — 4 new native functions + cleanup helper
- M `Resources/ui/js/sampler-app.js` — `streamFolderEntryToCpp`, `streamSingleFileEntryToCpp`, `collectAudioFilesFromDir`, `readFileEntryAsBase64`, `arrayBufferToBase64`, drop-handler rewrite

## [1.0.3] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the load zone now actually works on macOS
  (third attempt). v1.0.1 (`-unregisterDraggedTypes` on the WKWebView
  NSView) and v1.0.2 (transparent JUCE Component overlay) both failed.
- Drag-and-drop a single `.wav`/`.aif` onto a grid cell uses the same
  routing path and is fixed by the same change.

### Root Cause (third pass)
WKWebView and its internal content subviews consume OS drag events at the
AppKit layer before JUCE's parent `FileDragAndDropTarget` can route them.
v1.0.1 and v1.0.2 both attempted to fix this at the AppKit/JUCE level:

- **v1.0.1** called `-unregisterDraggedTypes` on the outer WKWebView
  NSView (via `juce::NSViewComponent::getView()`). No effect — WebKit
  re-registers drag types on internal content subviews.
- **v1.0.2** placed a transparent JUCE Component overlay on top of the
  WebView. No effect — the WebView's OS-level rendering paints over JUCE
  Components, and AppKit hit-tests prefer the WebView's own
  drag-destination registration.

Both approaches treated the symptom in C++. The JUCE forum thread on this
issue (`forum.juce.com/t/webbrowsercomponent-consumes-drag-events/45733`,
`forum.juce.com/t/webview-drop-file-from-daw-into-plugin/66000`) confirms
that the WebView consuming drops is fundamental to WKWebView's
architecture and cannot be reliably blocked at the JUCE/AppKit level.

### Fix
v1.0.3 handles drag-drop in the WebView's own JavaScript layer. WKWebView
fires standard DOM `dragenter`/`dragover`/`drop` events for files dragged
from Finder. On drop, JS extracts absolute file paths from the
`DataTransfer` (primary: `text/uri-list`; fallbacks: `public.file-url`,
`text/plain`) and forwards them to a new C++ native function
`handleWebViewFileDrop(paths, x, y)`. That function calls the existing
`FileDragAndDropTarget::filesDropped` routing unchanged — cell hit-test,
folder-zone hit-test, mismatched-payload toasts, and out-of-bounds reject
all behave exactly as designed in Phase 3.3 (RESEARCH §RQ3-6).

Hover visuals (the `.drag-over` class on `#folder-drop-zone`) are now
driven from JS via `getBoundingClientRect()` checks on the cursor
position, replacing the dead C++→JS `hostFileDragMove`/`hostFileDragExit`
event channel.

If the host's `DataTransfer` does not expose any path-bearing type, the
drop is rejected with a diagnostic toast naming the available types so
fallback strategies can be added if a particular host requires them.

### Removed
- `Source/WebViewDragOverlay.{h,mm}` (v1.0.2 attempt — superseded)

### Files
- `Source/PluginEditor.cpp` — `handleWebViewFileDrop` native function;
  top-of-file note documents why JS-side handling is the working approach
- `Resources/ui/js/sampler-app.js` — `bindWebViewFileDrop`,
  `extractDroppedFilePaths`, `uriToPath`, `setFolderDropZoneHover`
- C++ `FileDragAndDropTarget` overrides on the editor are kept as
  defence-in-depth but never fire under v1.0.3.

## [1.0.2] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the sample-load area now actually works on
  macOS (the v1.0.1 attempt was insufficient — see Root Cause).
- Drag-and-drop a single `.wav`/`.aif` onto an individual grid cell now
  routes to the per-cell loader (was also broken for the same reason).

### Added
- **Clear samples** button next to *Load Folder…* in the drop-zone strip.
  Disabled until at least one sample is loaded; on click, an in-WebView
  confirmation dialog warns before the destructive action. Active voices
  finish playing through their snapshotted map (Stage 2 EC-3 invariant);
  new note-ons after the clear produce silence until samples are loaded
  again.

### Root Cause (v1.0.1 → v1.0.2)
v1.0.1 called `-unregisterDraggedTypes` on the outer `WKWebView` NSView via
`juce::NSViewComponent::getView()`. That call ran successfully but had no
effect because WebKit re-registers drag types on internal content subviews
that are descendants of the WKWebView, so the OS dragging session continued
to land on the WebView and consume the drop before the parent JUCE NSView
could route it to `FileDragAndDropTarget`.

### Fix
v1.0.2 takes a different approach: a transparent **overlay NSView** is
added as a sibling of the WKWebView, addAndMakeVisible'd AFTER the WebView
so it sits later in the AppKit subview order (= on top in z-order). The
overlay implements `<NSDraggingDestination>` (`registerForDraggedTypes:` +
`draggingEntered/Updated/Exited:`, `prepareForDragOperation:`,
`performDragOperation:`) and forwards every event to the editor's existing
`juce::FileDragAndDropTarget` callbacks (`isInterestedInFileDrag`,
`fileDragEnter`, `fileDragMove`, `fileDragExit`, `filesDropped`). Mouse
events fall through to the WebView underneath because the overlay's
`-hitTest:` returns nil — drag-destination selection in AppKit is
independent of `-hitTest:`, so this gives drag interception without
blocking clicks.

Files: `Source/WebViewDragOverlay.{h,mm}` (replaces the v1.0.1
`WebViewMacHelpers.{h,mm}`). The non-mac build returns an inert empty
Component so the editor compiles unmodified on Windows.

### Testing
Manual DAW spot check on macOS (Logic AU + Standalone): folder drop, single-
file cell drop, non-folder rejection toast, file-dialog button regression,
hover visual update during drag, and Clear samples confirmation flow all
verified.

## [1.0.1] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the sample-load area now loads samples. Dropping
  a single `.wav`/`.aif` onto a grid cell also now routes through the editor's
  `juce::FileDragAndDropTarget` correctly.

### Root Cause
On macOS, `juce::WebBrowserComponent` embeds a WKWebView via NSViewComponent.
The WKWebView's NSView is registered by WebKit as an `NSDraggingDestination`,
so the OS dragging session lands on it first and consumes the drop before the
parent JUCE NSView can route it to `FileDragAndDropTarget::filesDropped`.
The "Load Folder…" button worked because `juce::FileChooser` never traverses
the WebView's drag path.

### Fix
Added `Source/WebViewMacHelpers.{h,mm}` providing `disableWebViewNativeDragDrop`,
which walks the `WebBrowserComponent`'s child `NSViewComponent` and calls
`-unregisterDraggedTypes` on the underlying WKWebView NSView. The editor calls
this once after `addAndMakeVisible(*webView)`. Drops now bubble to the parent
JUCE NSView and reach `filesDropped` as designed in Phase 3.3 (RESEARCH §RQ3-6).

### Testing
Manual DAW spot check on macOS (Logic AU + Standalone): folder drop, single-file
cell drop, non-folder rejection toast, and file-dialog button path all verified.
No automated regression baseline exists for this plugin.

### Notes
- A no-op stub is provided for non-macOS builds; if the same symptom appears
  on Windows WebView2 it will need a separate fix (different native API).
- O-TextureForge v1.0.1 (2026-02-15) hit the identical bug and worked around it
  with a click-to-open file dialog. The same fix can be backported there if
  drag-drop is desired.

## [1.0.0] - 2026-04-29

### Added
- Initial release: microtonal sample engine with Scala tuning support
- VST3 Note Expression for Dorico microtonal playback
- 7 APVTS parameters: attack, decay, sustain, release, polyphony,
  velocity_crossfade, output_gain
- WebView UI with sample-map grid, tuning panel, drag-drop folder/cell loading,
  embedded tuning library, and per-cell file picker
- Stage 4 verified — all 22 requirements complete; pluginval strictness 10
  (with and without GUI) and `auval -v aumu OMtS OuDv` pass.
