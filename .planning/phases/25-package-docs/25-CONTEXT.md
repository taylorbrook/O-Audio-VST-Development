# Phase 25: Package & Internal Technical Notes — Context (v2 — Playback Template pivot)

**Gathered:** 2026-04-26 (v2 replan after Plan 25-01 v1 revert)
**Status:** Ready for replanning
**Supersedes:** v1 CONTEXT (committed at ed0fc90) — distribution mechanism was wrong; XML asset content stays.

<domain>
## Phase Boundary

Author one canonical Dorico Playback Template (`Ouaricon-Microtonal-Suite.dorico_pt`, a zip archive containing `playbacktemplatespec.xml` + `endpointconfig.xml` + `playbacktemplatedeps.doricolib`) and one canonical expression-map library bundle (`Ouaricon-VST3-NoteExpression.doricolib`). Bundle BOTH files in all 8 affected plugins' installers (PKG on macOS, EXE on Windows) so Dorico-aware microtonal playback wires up automatically when the user installs any Ouaricon plugin. Capture developer-facing internal technical notes under `research/` covering module architecture, Dorico Playback Template setup procedure, host-side quirks, and troubleshooting signatures. Both shipped assets are the single source of truth at `modules/tuning/note-expression/resources/`; consumed via the existing module system; written to user systems via auto-discovery directories that Dorico scans at startup.

**Distribution mechanism (NEW vs v1):** Drop into Dorico's auto-scan directories. The `.dorico_pt` extracts into `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/`; the `.doricolib` lands in `Default Library Additions/`. Dorico's `loadDefaultLibraryAdditions` and `PlaybackTemplateSpecs` scan symbols are confirmed in the v6 binary; `.doricoexpmap` is NOT a recognized extension (verified empirically by Plan 25-01 v1 + binary `strings`).

**In scope:**
- Authoring `modules/tuning/note-expression/resources/playback-template/` (template source tree zipped at build time into `Ouaricon-Microtonal-Suite.dorico_pt`).
- Authoring `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` (standalone expression-map library; recovered from commit `cd2c2c6` — the XML body is structurally valid as `.doricolib`).
- CID extraction helper (`ouaricon_extract_vst3_cids`) that reads `Contents/Resources/moduleinfo.json` from each built `.vst3` bundle and substitutes plugin GUIDs into `endpointconfig.xml` via `configure_file @ONLY`. Dev installers ship dev CIDs; prod installers ship prod CIDs.
- Module-level `install()` rule + propagation logic so each consuming plugin inherits both resources via `ouaricon_add_module()`.
- Per-platform dual-write installer logic: PKG (macOS) and Inno Setup EXE (Windows) each write to (a) Ouaricon shared resources path and (b) Dorico `PlaybackTemplateSpecs/` + `Default Library Additions/` auto-scan paths.
- All 8 affected plugins' installer configs updated to bundle and write both files.
- Cross-platform validation: dry-run install + Dorico apply-template + quarter-sharp smoke test on **both** macOS and Windows.
- Internal notes under `research/microtonal-dorico-integration.md` (single combined file with 4 H2 sections covering DOCS-01..04). DOCS-02 / DOCS-03 / DOCS-04 are reframed for the Playback Template apply flow (not the standalone expression-map import flow).

**Out of scope (deferred to other phases / future work):**
- End-user-facing manuals or quickstart guides on the sales website (FUT-06; DOCS-01..05 explicitly remain internal this milestone).
- Articulation switches in the expression map (staccato, legato, dynamics) — microtonality-only this milestone.
- `slot<N>.pluginstate` files (curated knob positions per plugin) — see **D-12** below; v1.5 ships state-less.
- Per-plugin `.dorico_pt` variants — single omnibus across all 8 plugins (D-08 below).
- MTS-ESP, MPE, pitch-bend fallback (FUT-02..04).
- Per-note custom NE types beyond `kTuningTypeID` (FUT-02).
- Automated Dorico smoke harness (still manual; deferred from Phase 24).

**Carrying forward from v1 CONTEXT (still valid after pivot):**
- **Module owns the asset** (v1 D-04 / now D-05). Extends from one file to two (.dorico_pt + .doricolib). The module's `install()` rule fires per-consumer at install/package time, mirroring Phase 23's per-consumer JUCE-NE-PATCH marker check.
- **Module v1.0.0 → v1.1.0 minor bump.** Additive resource surface, not breaking. `modules/registry.yaml` updated.
- **3-plan structure** (v1 D-11 / now D-15). 25-01 / 25-02 / 25-03 keep their roles; content of each is updated to reflect the new architecture. `--replan` will regenerate plan files; old PLAN.md files are stale and supersede-tagged.
- **Stop-on-first-failure with in-plan triage** (v1 D-12 / now D-16). Structural failures promote to `25-NN-fix-PLAN.md`.
- **Cross-platform validation gate** (v1 D-09 / now D-13). Both macOS and Windows must pass real Dorico smoke for the installer/template pipeline. The plugins' non-installer behavior remains FUT-01 (macOS-only).
- **Internal-developer-only notes** (v1 D-14 / now D-19). DOCS-05 boundary preserved. Tone is technical reference, not user marketing.
- **Single canonical file location at `modules/tuning/note-expression/resources/`** (v1 D-03). Tree expands from one file to a small subdirectory (`playback-template/` source tree + `library/` standalone .doricolib + `README-microtonal-suite.txt`).

**Carrying forward from Phase 23 (locked):**
- Module path `modules/tuning/note-expression`, public API surface `Ouaricon::NoteExpression::*`, header-only consumption (D-04..D-09, D-23). Phase 25 v2 does NOT modify the module's source surface.
- Per-format module-source convention (`cpp/<format>/`) — not directly relevant since these are configuration/data resources, but the module-owned-asset principle extends naturally.
- One-liner consumer integration via `ouaricon_add_module()` — v2 extends this to propagate dual-resource installation (`.dorico_pt` + `.doricolib`).

**Carrying forward from Phase 24 (locked):**
- 8 affected plugins are the v1.5 cohort: O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant. All 8 are confirmed `note-expression` consumers (Phase 24 final sweep, commit 0ec32e9).
- Each plugin's installer workflow already exists: `plugin-packaging` / `package` skill (PKG, macOS); `build-installer` skill (EXE, Windows). Phase 25 v2 extends configs, does not redesign workflows.
- D-12 (Phase 24): stop-on-first-failure, triage in same plan — preserved.

**Carrying forward from `CLAUDE.md`:**
- Build targets: `ninja <Plugin>_VST3 <Plugin>_AU <Plugin>_Standalone` (macOS); `cmake --build build --config Release --target <Plugin>_VST3` (Windows).
- AU cache clear + remove old bundles + fresh install protocol must be honored before any cross-platform Dorico smoke test.

</domain>

<decisions>
## Implementation Decisions (v2)

> **Numbering note:** v2 starts D-01 from scratch and explicitly cites which v1 decisions it replaces or extends. Several v1 decisions remain valid in spirit (module ownership, semver bump, plan structure, validation gate) and are noted in the Phase Boundary's "Carrying forward from v1" section above.

### Distribution Architecture (replaces v1 D-01..D-08)

- **D-01: Two distributable assets, single canonical source.** Ship a `.dorico_pt` (Playback Template archive) alongside a `.doricolib` (expression-map library bundle). The `.dorico_pt` carries the full routing config (plugin slots + per-channel expression-map binding); the `.doricolib` makes the expression map independently available in `Library → Expression Maps` even when the user has not yet applied the template. Both files are produced from a single canonical XML body — the `<kScoreLibrary><expressionMapDefinitions><ExpressionMapDefinition>` recovered from commit `cd2c2c6` (D-04). The `.dorico_pt`'s embedded `playbacktemplatedeps.doricolib` and the standalone `.doricolib` share the same `<entityID>xmap.ouaricon.vst3_note_expression</entityID>` byte-exactly.
  - Why both: self-containment of the template (drag-drop install does not need pre-existing library content) AND independent reusability of the expression map (advanced users may apply a custom endpoint config and still want the map available).
  - `.dorico_pt` is a standard zip archive; verified by extracting the Ample China public sample and matching the binary's recognized `dorico_pt` / `dorico_pt.zip` strings.

- **D-02: Microtonality-only minimum scope.** The expression-map XML body sets `microtonalPlaybackMethod=kVST3NoteExpression` (the spike-validated trap from Patterns 1–3 — Dorico's `Auto` selection picks pitch-bend for non-Steinberg VST3s and silently breaks microtonal playback). No articulation switches (staccato/legato/dynamics) or per-plugin variants this milestone. (Carries forward from v1 D-02 unchanged.)

- **D-03: Recover, do not re-author the expression-map XML.** The Plan 25-01 v1 `.doricoexpmap` body is structurally valid as a `.doricolib` — its `<kScoreLibrary>` root matches the .doricolib root verbatim. Recover from `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`. The plumbing was wrong; the asset content was right.

### Template Granularity

- **D-04: ONE omnibus `Ouaricon-Microtonal-Suite.dorico_pt`** with 8 plugin slots (one per cohort plugin). Schema natively supports multi-plugin via `<slots>` in `endpointconfig.xml` — Ample China sample proves 11-slot multi-plugin templates load correctly. Dorico warns + skips missing-plugin slots rather than failing template apply. Single discoverable entry in `Play → Playback Template`. Lower user friction, lower maintenance surface, single source of truth. *(User confirmed 2026-04-26.)*

### Plugin State

- **D-05: Ship state-less `.dorico_pt`** for v1.5. `endpointconfig.xml` declares 8 slots WITHOUT corresponding `slot<N>.pluginstate` files. Dorico loads each plugin with its own factory defaults; the load-bearing invariant (microtonal routing via expression-map binding) is fully encoded in `endpointconfig.xml`'s `<expressionMapID>` and survives the omission. Factory `Silence` template demonstrates the same pattern (`<pluginStateFile/>` empty self-closing). Curated states are deferred — they cannot be hand-authored (proprietary Steinberg binary), they require a one-time human Dorico session, and they create maintenance debt as plugin defaults evolve. **Verification: A2 (manual ~10 min, runs in Plan 25-01 v2 Wave 0).** *(User confirmed 2026-04-26.)*

### Plugin GUID Acquisition

- **D-06: Read each plugin's `Contents/Resources/moduleinfo.json` from the built `.vst3` bundle.** The `Audio Module Class` entry's `CID` field is the canonical 32-hex value. `configure_file(... @ONLY)` substitutes per-plugin tokens (e.g., `@OLYRICA_PLUGINID@`) into `endpointconfig.xml.in` at packaging time. JUCE 8 emits this JSON deterministically from `(manufacturerCode, pluginCode)`; reading the actually-built artifact avoids any drift from algorithm reimplementation. **Dev installers carry dev CIDs (`OuDv` middle bytes `4F754476`); prod installers carry prod CIDs (`OuAu` middle bytes `4F754175`).** All 8 dev CIDs are tabulated in `25-RESEARCH.md` Pattern 2.

- **D-07: Helper lives in `modules/cmake/OuariconModules.cmake`.** New function `ouaricon_extract_vst3_cids(OUTPUT_VAR ... PLUGINS ...)` parses each `moduleinfo.json` (Python helper to handle JUCE's trailing-comma JSON quirk) and sets per-plugin `<NAME>_PLUGINID` variables in parent scope. Invoked from `module.cmake` AFTER all 8 `_VST3` targets build (custom command dependency).

### Module-Side Asset Ownership (extends v1 D-04)

- **D-08: The `note-expression` module owns BOTH resources via CMake `install()` rules in `modules/tuning/note-expression/module.cmake`.** Any consumer of `ouaricon_add_module(<Plugin> note-expression)` automatically inherits both `.dorico_pt` (built from the `playback-template/` source tree at build time via `cmake -E tar cf ... --format=zip`) and `.doricolib` (copied from `library/` directly). Mirrors v1 ownership philosophy — extends from one file to two.

- **D-09: Resource layout under `modules/tuning/note-expression/resources/`:**
  ```
  resources/
  ├── playback-template/                                          # Source for .dorico_pt (zipped at build time)
  │   ├── PlaybackTemplateSpecs/Ouaricon Microtonal Suite/
  │   │   └── playbacktemplatespec.xml.in                        # configure_file template
  │   └── EndpointConfigs/Ouaricon Microtonal Suite/
  │       ├── endpointconfig.xml.in                              # @<NAME>_PLUGINID@ tokens
  │       └── playbacktemplatedeps.doricolib.in                  # Embedded copy of the expression-map XML body
  ├── library/
  │   └── Ouaricon-VST3-NoteExpression.doricolib                 # Standalone, recovered from cd2c2c6
  └── README-microtonal-suite.txt                                # User-facing fallback (INST-04)
  ```

### Cross-Platform Installer Logic (replaces v1 D-05..D-08)

- **D-10: Per-plugin installer bundles BOTH files.** Each of the 8 plugins' PKG (macOS) and EXE (Windows) installers ships `Ouaricon-Microtonal-Suite.dorico_pt` + `Ouaricon-VST3-NoteExpression.doricolib`. Idempotent overwrite — all 8 installers write the same canonical content for a given build flavor. *(User confirmed 2026-04-26.)*

- **D-11: Dual-write per platform** to (a) Ouaricon shared resources (canonical, editable) AND (b) Dorico's auto-scan directories (auto-discovered without user import action):
  - **macOS shared:** `~/Library/Application Support/Ouaricon/Microtonal Suite/`
  - **macOS Dorico template:** Extract `.dorico_pt` zip into `~/Library/Application Support/Steinberg/Dorico [N]/` (zip's internal layout already targets `PlaybackTemplateSpecs/` + `EndpointConfigs/` subdirectories — verified via Ample China sample)
  - **macOS Dorico library:** Copy `.doricolib` into `~/Library/Application Support/Steinberg/Dorico [N]/Default Library Additions/` (note: spaces in dir name on macOS)
  - **Windows shared:** `%APPDATA%\Ouaricon\Microtonal Suite\`
  - **Windows Dorico template:** Extract into `%APPDATA%\Steinberg\Dorico [N]\` matching internal layout
  - **Windows Dorico library:** `%APPDATA%\Steinberg\Dorico [N]\DefaultLibraryAdditions\` (note: NO spaces on Windows)
  - Installer creates `Default Library Additions` / `DefaultLibraryAdditions` dir if missing — Dorico does not auto-create it.

- **D-12: Dorico version targeting — install to the latest detected Dorico version directory.** Probe descending order (Dorico 6, 5, 4). If multiple Dorico versions are installed, install to the latest (consistent with Plan 25-01 v1 detection logic — that part was correct; only the destination subdirectory changes). Future Dorico releases require an installer config update — note as known maintenance touchpoint in DOCS-03. (Carries forward from v1 D-07.)

- **D-13: README emission as fallback (INST-04).** Each installer writes `README-microtonal-suite.txt` at the Ouaricon shared resources path describing (a) both files' purposes, (b) how to manually apply the Playback Template via `Play → Playback Template → Import` if auto-discovery missed the directory, (c) how to manually import the expression map via `Library → Import Library` if needed, and (d) the canonical files' source-of-truth location in the module repo. README is plain technical text (honors DOCS-05 boundary). (Carries forward from v1 D-08, content updated.)

### Verification Gates (NEW)

- **D-14: A2 + A4 verifications run inside Plan 25-01 v2 Wave 0.** *(User confirmed 2026-04-26.)*
  - **A2 (state-less .dorico_pt accepted by Dorico):** Pack a stripped `.dorico_pt` with slots only, no `slot<N>.pluginstate` files. Import into Dorico 6. Confirm template appears in `Play → Playback Template`, applies cleanly, and slots load (with whatever default plugin state). ~10 min.
  - **A4 (drag-drop extraction is faithful):** Drag the Ample China sample onto Dorico 6. Confirm both `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ample China/` AND `EndpointConfigs/Ample China/` exist after install. ~5 min.
  - **Stop-on-first-failure (D-16) applies.** If A2 fails, escalate to D-05 reconsideration (curated state authoring becomes mandatory). If A4 fails, escalate to D-11 reconsideration (drag-drop install is rejected; explicit `Play → Playback Template → Import` becomes the user-facing flow and the installer simply lands the file in `~/Library/Application Support/Ouaricon/Microtonal Suite/`).

### Validation Gate (extends v1 D-09 / D-10)

- **D-15: Cross-platform Dorico apply-template + quarter-sharp smoke on macOS AND Windows.** Reverses Phase 23/24's macOS-only convention for the installer pipeline. Each platform: install one representative plugin → open Dorico → confirm `Ouaricon Microtonal Suite` appears in template picker → apply template → load test project → quarter-sharp C4 = +50¢, no attack zipper, NE correlated by `noteId` (3-point gate from Phase 24 D-07). Aggregate results in Plan 25-02's SUMMARY.md.

- **D-16: Per-platform validation matrix.** macOS reference consumer: O-Lyrica (Phase 23 precedent). Windows reference consumer: planner picks based on Windows machine availability (recommend O-Lyrica for parity, fall back to whatever is fastest to install). If Windows access is blocked, plan must surface as hard halt (don't silently degrade to macOS-only).

### Plan Structure (carries v1 D-11 with content updates)

- **D-17: 3 plans, contents reframed for v2 architecture.** Existing v1 PLAN.md files are stale and will be regenerated by `/gsd-plan-phase 25`.
  - **`25-01-author-and-plumbing-PLAN.md` (v2)** — A2/A4 verifications first; recover XML body from cd2c2c6; author `playback-template/` source tree (`playbacktemplatespec.xml.in`, `endpointconfig.xml.in`, `playbacktemplatedeps.doricolib.in`); author standalone `library/Ouaricon-VST3-NoteExpression.doricolib`; add `ouaricon_extract_vst3_cids` helper to `OuariconModules.cmake`; extend `module.cmake` with `.dorico_pt` packing custom command + dual `install()` rules + `install-microtonal-suite.cmake.in` template; bump module 1.0.0 → 1.1.0; update `modules/registry.yaml`; canary install on O-Lyrica proves end-to-end pipeline (file is at canonical user paths AND template appears in Dorico picker).
  - **`25-02-installer-bundling-sweep-PLAN.md` (v2)** — atomic sweep across all 8 plugins' installer configs (PKG + EXE) bundling `.dorico_pt` + `.doricolib`. Cross-platform installer build + dry-run install + Dorico apply-template + quarter-sharp smoke matrix (D-15/D-16). Mirrors Phase 24 final-sweep shape.
  - **`25-03-internal-notes-PLAN.md` (v2)** — write `research/microtonal-dorico-integration.md` (4 H2 sections, DOCS-01..04). DOCS-02 reframed for Playback Template apply flow (not standalone .doricoexpmap import). DOCS-03 covers Playback Template + Default Library Additions auto-discovery quirks. DOCS-04 includes new troubleshooting signatures: missing-plugin warnings on apply, `.doricolib` directory-name spaces variance (macOS vs Windows), CID-mismatch silent failure (dev vs prod build).

- **D-18: Stop-on-first-failure, in-plan triage** (carries v1 D-12). Structural failures promote to `25-NN-fix-PLAN.md`.

### Internal Notes Layout (carries v1 D-13 with content updates)

- **D-19: Single combined file at `research/microtonal-dorico-integration.md`** with 4 H2 sections:
  - `## Module Architecture` (DOCS-01) — unchanged from v1: NEC advertisement flow, raw-event queue semantics, voice-routing logic, composition with `TuningEngine` analogs. References `modules/tuning/note-expression/cpp/NoteExpression.h` + `cpp/vst3/NoteExpression_VST3.cpp`.
  - `## Canonical Dorico Setup Procedure` (DOCS-02) — **REFRAMED**: step-by-step Playback Template apply flow (not standalone expression-map import). Covers: install Ouaricon plugin → restart Dorico → `Play → Playback Template → Ouaricon Microtonal Suite → Apply and Close`. Names exact menu paths. Includes the manual-import fallback (`Play → Playback Template → Import` for `.dorico_pt`; `Library → Import Library` for `.doricolib`) for users whose auto-scan didn't catch the directory.
  - `## Host-Side Behavior Quirks` (DOCS-03) — **EXTENDED**: Dorico's neighbor-semitone + NE-delta representation; NEC handshake ignored by Dorico but kept for other hosts; sample-offset timing; multiple-Dorico-version installer caveat; **NEW: `Default Library Additions` directory does not exist by default — installer creates it**; **NEW: directory name spaces variance (macOS spaces, Windows no spaces)**; **NEW: dev vs prod CID variance and the `configure_file @ONLY` mitigation**.
  - `## Troubleshooting Signatures` (DOCS-04) — **EXTENDED**: symptoms-vs-cause table for the expression-map-skipped UX trap; missing-plugin warnings on template apply (graceful, not a failure); silent template non-appearance after install (caused by writing to wrong Dorico version dir or wrong subdirectory name).
- **D-20: Notes are developer-facing only (DOCS-05).** No end-user manual or quickstart copy this milestone. Tone: technical reference. (Carries forward from v1 D-14.)

### Stale Artifacts to Clean Up

- **D-21: `build/plugins/<Plugin>/install-doricoexpmap-<Plugin>.cmake` files (9 stale).** Generated by reverted Plan 25-01 v1's `configure_file`. Gitignored; regenerated on next clean build but currently named after the dead path. Plan 25-01 v2 deletes the v1 `.cmake.in` template and renames its replacement `install-microtonal-suite.cmake.in`. Stale build outputs vanish on next clean build.

### Claude's Discretion

- **Exact ordering of plugins in the bundling sweep** (D-15). Recommended canary: O-Lyrica first (reference consumer); other 7 in any order — installer-config edit is mechanical.
- **Whether `ouaricon_add_module()` implicitly auto-installs the resources** vs requiring an explicit `ouaricon_install_microtonal_suite()` call. Recommend implicit (matches one-liner integration philosophy from Phase 23 D-26/D-27/D-29).
- **`fileVersion` in the canonical XML files** (`1.1416` from Ample China sample for Dorico 6; A3 verifies cross-version compatibility). Recommend pinning to `1.1416` initially; revisit if A3 verification (drag-drop into Dorico 5) fails.
- **Whether to bundle README inside `.vst3` too** (belt-and-braces). Recommend NO — single shared location only.
- **PLAN.md naming convention.** Recommend `25-NN-<slug>-PLAN.md` matching v1 / Phase 23/24 style.

</decisions>

<specifics>
## Specific Ideas

- **"The asset content is fine; the wrapper format is wrong."** Plan 25-01 v1's `.doricoexpmap` XML body is structurally valid as a `.doricolib` (both have `<kScoreLibrary>` root). Recover from cd2c2c6, do not re-author. The pivot is purely a wrapper + distribution change.
- **"One omnibus suite is the schema-native answer."** Dorico's `endpointconfig.xml` natively supports multi-plugin via `<slots>`. The Ample China sample proves 11-slot templates work. Eight per-plugin templates would replicate v1's "many small files" pattern unnecessarily.
- **"State-less is the right v1.5 default."** The load-bearing invariant is microtonal routing (expression-map binding); knob positions are not. Skipping `.pluginstate` authoring eliminates a maintenance vector and a Dorico-session lock-in. Curated states are a candidate for v1.6+ if user feedback warrants.
- **"Dual-write means the user never has to import anything."** `.dorico_pt` extracts into `PlaybackTemplateSpecs/` (Dorico scans at startup); `.doricolib` lands in `Default Library Additions/` (Dorico merges via `loadDefaultLibraryAdditions`). Both auto-discover. Users open Dorico after installing any Ouaricon plugin and the suite is wired up.
- **"Dev/prod CID divergence is real and `configure_file` solves it."** JUCE 8 emits `Contents/Resources/moduleinfo.json` per-build; dev manufacturer code `OuDv` and prod `OuAu` produce different CIDs. Reading the just-built artifact at packaging time means dev installers ship dev CIDs and prod installers ship prod CIDs automatically. No hard-coded CIDs in source.
- **"A2 + A4 verifications are cheap; do them first."** ~15 min combined on the dev machine. Eliminates the only two MEDIUM-confidence assumptions in research before the bulk implementation work begins. Plan 25-01 v2 Wave 0 = these two checks.
- **"Per-plugin installer bundle, not a separate Suite Installer."** All 8 plugins' installers ship the same canonical pair. Idempotent overwrite. Users get the suite-wide Dorico routing whether they install one Ouaricon plugin or all eight. Mirrors v1 architecture exactly.
- **"Internal notes are source material for FUT-06."** DOCS-01..04 reframed for the Playback Template flow; tone stays technical-reference; the website team translates later.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents (researcher, planner, executor) MUST read these before planning or implementing.**

### Phase Scoping
- `.planning/ROADMAP.md` §Phase 25 — goal, dependencies (Phase 24), 5 success criteria. Note: criterion #1 mentions `.doricoexpmap` by name; success is measured by *behavioral* equivalence (canonical asset exists at module path; installers bundle it; Dorico picks it up; quarter-sharp smoke passes). The `.dorico_pt` + `.doricolib` pair satisfies the same intent via a different file format. ROADMAP success-criteria text may be tightened in Plan 25-01 v2's docs touch.
- `.planning/REQUIREMENTS.md` §INST-01..04, §DOCS-01..05 — binding requirements. Same observation as above re: filename in INST-01 — the canonical asset is now a pair of files; intent is preserved.
- `.planning/REQUIREMENTS.md` §FUT-06 — end-user manual/quickstart deferred; DOCS-05 boundary.
- `.planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md` — **MANDATORY READ.** Records what was tried, why it failed, and the pivot trigger. Reverted commits: cd2c2c6, 496d4c4, 029b12b. Combined revert: d2c86c5.
- `.planning/phases/25-package-docs/25-RESEARCH.md` — **MANDATORY READ.** Pivot research with HIGH-confidence answers to Q1, Q2, Q6, Q7 + recommendations for Q3, Q4, Q5 (all locked in this CONTEXT.md). Includes verified `.dorico_pt` zip layout, all 8 cohort dev CIDs, recovered XML body schema, A2/A3/A4/A5/A6 assumptions log.
- `.planning/phases/24-propagate/24-CONTEXT.md` — Phase 24 design decisions, especially D-12 (stop-on-first-failure playbook) and the 5 propagation patterns catalogued in plan 24-08.
- `.planning/phases/23-extract/23-CONTEXT.md` — Phase 23 module ownership philosophy, per-format routing convention. Phase 25 v2's "module owns the asset" decision (D-08) extends these principles to two resources instead of one.
- `.planning/seeds/microtonal-shared-module.md` — original seed; rationale for the microtonal milestone.

### Recovery Source
- `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` — recovered XML body for both `.doricolib` and embedded `playbacktemplatedeps.doricolib`. Use as-is; do not re-author. The reverted README content (`README-doricoexpmap.txt`) is similarly recoverable from `git show cd2c2c6:modules/tuning/note-expression/resources/README-doricoexpmap.txt` — content needs revision for the new flow but tone/structure carry forward.

### Reference Sample (extracted, kept for ongoing reference)
- `/tmp/ample_china/` and `/tmp/ample_china_extracted/` — public third-party `.dorico_pt` extracted during research. Schema reference for `playbacktemplatespec.xml`, `endpointconfig.xml`, `playbacktemplatedeps.doricolib`, `slot<N>.pluginstate`. Plan 25-01 v2 A4 verification uses this directly.

### Implementation Bible (auto-loaded skill)
- `.claude/skills/spike-findings-VST-development/SKILL.md` — findings index.
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — validated patterns 1–5, landmines 1–5, constraints. **The `kVST3NoteExpression` Microtonality invariant (Landmine 3) is what the recovered XML encodes.**
- `.claude/skills/spike-findings-VST-development/sources/` — spike-era reference code; useful for DOCS-01 architecture cross-references.

### Background Research (carry-forward from v1 — still applicable)
- `.planning/notes/dorico-microtonal-vst-research.md` — Dorico's 3 wire mechanisms; source material for DOCS-02 (canonical setup procedure, REFRAMED for Playback Template flow) and DOCS-03 (host-side quirks).
- `research/microtonality-implementation-juce.md`
- `research/microtonality-theory-formats.md`
- `research/microtonality-comprehensive-database.md`
- `research/microtonality-commercial-performance.md`

### Module Surface (consume; module install() rules will be ADDED in this phase)
- `modules/tuning/note-expression/cpp/NoteExpression.h` — public API. Source for DOCS-01 architecture notes.
- `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` — VST3-only TU. Source for DOCS-01.
- `modules/tuning/note-expression/README.md` — consumer integration steps; updated to mention canonical `.dorico_pt` + `.doricolib` and auto-install behavior.
- `modules/tuning/note-expression/module.yaml` — bumped 1.0.0 → 1.1.0 in plan 25-01 v2.
- `modules/tuning/note-expression/module.cmake` — currently fires JUCE-NE-PATCH marker check; plan 25-01 v2 ADDS the `.dorico_pt` packing custom command + dual `install()` rules.
- `modules/registry.yaml` — `note-expression` entry; version bumped to 1.1.0; changelog entry.

### Module System Plumbing
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` macro. Plan 25-01 v2 ADDS `ouaricon_extract_vst3_cids()` helper here.

### Installer Workflows (the bundling targets — extend, do not redesign)
- `.claude/skills/plugin-packaging/SKILL.md` — macOS PKG installer workflow. Plan 25-02 v2 extends per-plugin packaging configs.
- `.claude/skills/package/SKILL.md` — `/package` command. Same workflow.
- `.claude/skills/build-installer/SKILL.md` — Windows EXE via Inno Setup. Plan 25-02 v2 extends per-plugin Inno Setup configs (`[Files]` section + Pascal `[Code]` for Dorico version detection).

### Build & Install Discipline
- `CLAUDE.md` — Plugin Cache Clearing protocol on macOS; Windows installer + cache-clear steps. **Mandatory before each platform's Dorico smoke test in plan 25-02.**

### Reference Plugins (the 8 affected; installer configs touched in plan 25-02 v2)
- `plugins/O-Lyrica/CMakeLists.txt`, `plugins/O-Bells/CMakeLists.txt`, `plugins/O-IntonationPad/CMakeLists.txt`, `plugins/O-Prism/CMakeLists.txt`, `plugins/O-Wind/CMakeLists.txt`, `plugins/O-Reed/CMakeLists.txt`, `plugins/O-Bowed/CMakeLists.txt`, `plugins/O-Formant/CMakeLists.txt`. Each consumes `note-expression`. Installer configs under each plugin's packaging entry point.

### Phase 24 Closeout (immediate predecessor — read for context, not for re-decision)
- `.planning/phases/24-propagate/24-VERIFICATION.md` — confirms all 8 plugins ship with NE module, 8/8 PASS Dorico smoke.
- `.planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md` — aggregate matrix.

### Dorico Documentation (external — agents fetch live)
- Steinberg Dorico Playback Template + `.dorico_pt` import flow (current Dorico 6 docs). Researcher fetches at planning time.
- [archive.steinberg.help/dorico/v3/en/dorico/topics/play_mode/play_mode_playback_templates_importing_t.html](https://archive.steinberg.help/dorico/v3/en/dorico/topics/play_mode/play_mode_playback_templates_importing_t.html) — drag-and-drop import documented.
- [steinberg.help/dorico_pro/v5/en/dorico/topics/play_mode/play_mode_playback_template_custom_creating_t.html](https://steinberg.help/dorico_pro/v5/en/dorico/topics/play_mode/play_mode_playback_template_custom_creating_t.html) — custom playback template creation.

</canonical_refs>

<code_context>
## Existing Code Insights (v2)

### Reusable Assets
- **`modules/tuning/note-expression/`** — fully extracted module with stable API. Phase 25 v2 ADDS one new directory tree (`resources/playback-template/` + `resources/library/`) and one new CMake build/install pipeline in `module.cmake`. Module source surface is otherwise untouched.
- **`modules/cmake/OuariconModules.cmake`** — `ouaricon_add_module()` macro. Plan 25-01 v2 APPENDS `ouaricon_extract_vst3_cids()` helper here.
- **`.claude/skills/plugin-packaging/SKILL.md` + `.claude/skills/package/SKILL.md`** — macOS PKG installer workflow (signed, branded). Plan 25-02 v2 extends per-plugin configs with two payload entries instead of one.
- **`.claude/skills/build-installer/SKILL.md`** — Windows Inno Setup EXE workflow. Plan 25-02 v2 extends each plugin's `.iss` template (or equivalent) with two `[Files]` entries + Pascal `[Code]` for Dorico version detection + `Default Library Additions`-creation.
- **`scripts/verify-au-link.sh`** — AU verify gate (Phase 23/24). Optional inheritance for plan 25-02 per-plugin smoke (recommend skip — already validated in Phase 24).
- **`research/` directory** — already contains 4 microtonality research files. Plan 25-03 v2's `research/microtonal-dorico-integration.md` is a NEW file in this same family.
- **Plan 25-01 v1 CMake plumbing patterns** — the `configure_file` + `install(SCRIPT)` pattern (commit 496d4c4) is reusable; only source/destination paths and per-platform branches need adjustment. Don't re-architect what worked.

### Established Patterns
- **Module owns the asset** — Phase 23 established for source code, JUCE patch, README; Phase 25 v2 extends to two distributable resources (.dorico_pt + .doricolib).
- **One-liner consumer integration** — `ouaricon_add_module(<Plugin> note-expression)` already wires source compilation + patch-marker check. Phase 25 v2 makes this also wire dual-resource installation.
- **Atomic plan = atomic commit** — preserved.
- **Stop-on-first-failure with in-plan triage; promote structural failures to fix-plans** — preserved (D-18).
- **Per-platform dual-path install** — extends from one file × two paths (v1) to two files × multiple paths (v2): `.dorico_pt` to Ouaricon shared + Dorico `PlaybackTemplateSpecs/`; `.doricolib` to Ouaricon shared + Dorico `Default Library Additions/`.
- **Cross-platform validation gate** — extends from "installer-build only" to "installer-build + apply-template + Dorico smoke" on both platforms.

### NEW Patterns this phase
- **Configured-XML resource generation** — `playbacktemplatespec.xml.in` / `endpointconfig.xml.in` / `playbacktemplatedeps.doricolib.in` carry `@TOKEN@` placeholders substituted at packaging time by `configure_file @ONLY` from per-plugin `moduleinfo.json` reads. First time the project authors XML resources from a templated source.
- **JUCE-bundle `moduleinfo.json` introspection** — `ouaricon_extract_vst3_cids()` reads `Contents/Resources/moduleinfo.json` from each built `.vst3` to extract the canonical 32-hex CID. Avoids re-implementing JUCE's `jucePluginId` algorithm; tracks the actually-built artifact.
- **Build-time zip packing** — `cmake -E tar cf <archive>.dorico_pt --format=zip <files>` produces the distributable archive. Cross-platform; built into CMake.

### Integration Points (per-platform install pipeline, v2)

**Module side (one-time, plan 25-01 v2):**
1. `modules/tuning/note-expression/resources/playback-template/` — NEW source tree (3 templated XML files).
2. `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — NEW (recovered from cd2c2c6).
3. `modules/tuning/note-expression/resources/README-microtonal-suite.txt` — NEW.
4. `modules/cmake/OuariconModules.cmake` — APPEND `ouaricon_extract_vst3_cids()` function.
5. `modules/tuning/note-expression/module.cmake` — ADD packing custom command + dual `install()` rules + `install-microtonal-suite.cmake.in` template.
6. `modules/tuning/note-expression/module.yaml` — bump 1.0.0 → 1.1.0; add changelog entry.
7. `modules/tuning/note-expression/README.md` — append section describing the dual-resource shipping behavior.
8. `modules/registry.yaml` — bump version + changelog.

**Plugin side (per-plugin, plan 25-02 v2 — atomic sweep):**
- Each plugin's PKG payload + Inno Setup `[Files]` section updated to consume the module's two staged resources (`.dorico_pt` + `.doricolib`).
- Per-platform install destinations follow D-11 (Ouaricon shared + Dorico auto-scan).
- Inno Setup `[Code]` Pascal section adds Dorico version detection + `DefaultLibraryAdditions` directory creation.

**Notes side (one-time, plan 25-03 v2):**
- `research/microtonal-dorico-integration.md` — NEW file, single combined doc with 4 H2 sections (DOCS-02/03/04 reframed for Playback Template flow).

### Variation Points
- **macOS PKG payload structure** — postinstall script unzips `.dorico_pt` directly into `~/Library/Application Support/Steinberg/Dorico [N]/` (zip's internal `PlaybackTemplateSpecs/` and `EndpointConfigs/` paths land naturally) and copies `.doricolib` into `Default Library Additions/`.
- **Windows Inno Setup `[Files]` section** — two source entries (`.dorico_pt` + `.doricolib`) with multiple destinations each; Inno's `external: yes` flag may help if `.dorico_pt` is unzipped at install time vs shipped as a zip.
- **Dorico version detection** — both platforms probe descending order (6 → 5 → 4); skip + log if no Dorico version directory exists; user falls back to README manual-import path.

### Phase 25 v2 Touch Points (the package surface)

**Module-side (additive — does not modify existing module API):**
- `modules/tuning/note-expression/resources/` — NEW subtree.
- `modules/tuning/note-expression/module.cmake` — packing + install rules added.
- `modules/tuning/note-expression/module.yaml` — version bump.
- `modules/tuning/note-expression/README.md` — append.
- `modules/cmake/OuariconModules.cmake` — `ouaricon_extract_vst3_cids()` appended.
- `modules/registry.yaml` — version bump entry.

**Plugin-side (per-plugin, plan 25-02 v2):**
- 8 plugins' packaging configs — extended (specifics depend on packaging skill's config layout).

**Skill-side (potentially):**
- `.claude/skills/plugin-packaging/SKILL.md` and `.claude/skills/build-installer/SKILL.md` — may need template-level updates to read module-staged resources. Planner determines.

**Notes-side (plan 25-03 v2):**
- `research/microtonal-dorico-integration.md` — NEW.

**Stale build outputs (auto-cleanup):**
- `build/plugins/<Plugin>/install-doricoexpmap-<Plugin>.cmake` — 9 files, gitignored, vanish on next clean build (D-21).

</code_context>

<deferred>
## Deferred Ideas

- **Curated `slot<N>.pluginstate` per plugin** — D-05 ships state-less for v1.5. Revisit in v1.6+ if user feedback shows curated knob positions add meaningful value. Authoring path: one Dorico session, `Play → Save Endpoint Configuration`, copy state files into module resources, re-pack `.dorico_pt`.
- **Articulation switches in the canonical .doricolib** — staccato, legato, dynamics. Out of scope for v1.5 (microtonality-only). Revisit when there's user demand or website-manual content needs articulation coverage.
- **Per-plugin .dorico_pt variants** — e.g., MPE plugins (O-Reed, O-Bowed) potentially needing different routing. Defer until a real user-facing playback issue surfaces; the omnibus suite is correct for the milestone.
- **Automated Dorico smoke harness** — still manual per platform.
- **Multi-Dorico-version installer logic** — installer auto-detects Dorico 5/6/N and writes to all present versions. Plan 25-01 v2 may implement if straightforward; otherwise pin to latest detected (D-12) and document upgrade path in DOCS-03.
- **Cubase / Nuendo expression-map paths** — Cubase reads expression maps too. Out of scope for v1.5 (Dorico-only). Future generalization candidate.
- **Module-side `.doricolib` / `.dorico_pt` schema linting in CI** — programmatic XML schema validation before packaging. Manual smoke covers it for v1.5.
- **Updating each plugin's CHANGELOG with "now ships Ouaricon Microtonal Suite Dorico template"** — optional. Not required by INST/DOCS. If included, recommend uniform one-line entry without binary-version bump.
- **Cross-version `fileVersion` handling** — A3 not blocking; pinning to Dorico 6's `1.1416` and verifying Dorico 5 import works in research-followup. If Dorico 5 rejects, plan 25-01 v2 may need version-conditional `fileVersion` tokens.
- **End-user-facing manual / quickstart on the sales website** — FUT-06.

</deferred>

---

*Phase: 25-package-docs*
*Context gathered: 2026-04-26 (v2 — Playback Template pivot)*
*v1 superseded — see `25-FINDING-playback-template-pivot.md` and revert commit `d2c86c5`.*
