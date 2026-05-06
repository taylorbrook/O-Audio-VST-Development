# Phase 25: Package & Internal Technical Notes — Context (v3 — Path B locked)

**Gathered:** 2026-04-27 (v3 replan after Path B validation)
**Status:** Ready for replanning
**Supersedes:** v2 CONTEXT (committed at 3dc28e4 — `.dorico_pt` Playback Template approach) and v1 CONTEXT (committed at ed0fc90 — `.doricoexpmap` distribution mechanism). Architectural pivot recorded in `25-FINDING-path-b-validation.md`.

<domain>
## Phase Boundary

Author one canonical Dorico expression-map library bundle (`Ouaricon-VST3-NoteExpression.doricolib`) — a Dorico-valid `.doricolib` with the full ~48-container `<kScoreLibrary>` skeleton, populated only with the recovered `<ExpressionMapDefinition>` for VST3 Note Expression microtonal routing. Bundle this single file in all 8 affected plugins' installers (PKG on macOS, EXE on Windows) so any Ouaricon plugin install lands the asset at a canonical Ouaricon shared path. User performs a one-time Library Manager → Import to load the expression map into Dorico, then assigns it to plugin channels via the normal Play → Endpoints flow. Capture developer-facing internal technical notes under `research/` covering module architecture, Path B setup procedure, host-side quirks, and troubleshooting signatures. Single shipped asset is the source of truth at `modules/tuning/note-expression/resources/library/`; consumed via the existing module system.

**Distribution mechanism (Path B, v3):** One `.doricolib` lands at one platform-specific Ouaricon shared path. No auto-discovery. No Playback Template. No CID extraction. No dual-write.

- **macOS:** `~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib`
- **Windows:** `%APPDATA%\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib`

**User flow:** install any Ouaricon plugin → open Dorico → `Library → Library Manager → Import…` → pick the canonical path. After one-time import, the expression map appears in `Play → Endpoints → Expression Map` dropdown for any channel. User assigns it to their already-loaded Ouaricon plugin's channel manually.

**Verified end-to-end on macOS Dorico 6 with O-Lyrica-dev** (2026-04-27): quarter-sharp C4 plays at ~269 Hz between standard C4 (261.63 Hz) and C♯ (277.18 Hz) — VST3 Note Expression microtonal routing confirmed. See `25-FINDING-path-b-validation.md` and `25-01-WAVE-0-VERIFICATION.md` § A2 Result. Reference asset built during validation: `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B).

**In scope:**
- Authoring `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — full `<kScoreLibrary>` skeleton sourced from `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml`, with the recovered `<ExpressionMapDefinition>` from `git show cd2c2c6` injected into `<expressionMapDefinitions>/<entities>`. Replace v2's invalid fragment.
- Module-level `install()` rule that propagates the single resource to every consumer of `ouaricon_add_module(<Plugin> note-expression)`.
- Per-platform single-write installer logic: PKG (macOS) and Inno Setup EXE (Windows) write the `.doricolib` + a fallback README to the platform-specific Ouaricon shared path.
- All 8 affected plugins' installer configs updated to bundle and write the file (mechanical sweep).
- Cross-platform validation: install + Dorico Library Manager Import + quarter-sharp smoke on **both** macOS and Windows.
- Internal notes at `research/microtonal-dorico-integration.md` — single combined file, four H2 sections (DOCS-01..04) reframed for Path B's manual-assign flow.
- **Surgical amend-forward cleanup of Path A artifacts in commit `819b2b4`** (D-10): delete `.dorico_pt` packing from `module.cmake`, the `ouaricon_extract_vst3_cids` helper in `OuariconModules.cmake`, the three `.xml.in` templates under `resources/playback-template/`, and `playbacktemplatedeps.doricolib.in`. Keep the version bump, registry entry, README structure, version-probe install script (collapse to single-write), and recovered XML body (re-wrapped).

**Out of scope (deferred):**
- Auto-discovery via `Expression Maps/User/` or `Default Library Additions/` — explicit-import is the user-facing flow this milestone (D-01). Auto-discovery candidate for v1.6 if user feedback warrants.
- `<pluginNames>` array populating `<ExpressionMapDefinition>` for auto-suggestion in the Endpoints dropdown (D-02) — skipped this milestone. Schema verification cost not paid; manual dropdown selection is acceptable. Revisit for v1.6 with a clear user-feedback signal.
- End-user-facing manuals or quickstart guides on the sales website (FUT-06; DOCS-05 boundary).
- Articulation switches in the expression map (staccato, legato, dynamics) — microtonality-only.
- Per-plugin `.doricolib` variants — single canonical map applies to all 8 plugins.
- MTS-ESP, MPE, pitch-bend fallback (FUT-02..04).
- Per-note custom NE types beyond `kTuningTypeID` (FUT-02).
- Automated Dorico smoke harness — still manual per platform.
- Curated `.pluginstate` snapshots — Playback Template architecture is gone; not relevant under Path B.
- Dev/prod CID handling in shipped asset — `.doricolib` carries no plugin CIDs at all under Path B (D-02 makes the asset CID-free).

**Carrying forward from v2 CONTEXT (still valid after Path B pivot):**
- **Module owns the asset** (v2 D-08; now D-03). Reduces from two resources to one. Module's `install()` rule fires per-consumer.
- **Module v1.0.0 → v1.1.0 minor bump** — already landed in commit `02fdcc2` (part of `819b2b4`). Keep; re-version is unnecessary churn.
- **3-plan structure** (v2 D-17; now D-09). 25-01 v3 / 25-02 v3 / 25-03 v3 — much smaller scope per plan than v2 since dual-write collapses to single-write and CID plumbing disappears.
- **Stop-on-first-failure with in-plan triage** (v2 D-18; preserved). Structural failures promote to `25-NN-fix-PLAN.md`.
- **Cross-platform validation gate** (v2 D-15/D-16; now D-08). Both macOS and Windows must pass installer build + Dorico Library Manager Import + quarter-sharp smoke. Per-platform reference consumer: O-Lyrica on macOS (Phase 23 precedent), planner picks Windows reference based on availability.
- **Per-plugin installer bundles the asset** (v2 D-10; preserved). All 8 plugins' installers ship the same canonical file. Idempotent overwrite.
- **Internal-developer-only docs** (v2 D-20; preserved). DOCS-05 boundary. Tone is technical reference.

**Carrying forward from Phase 23 (locked):**
- Module path `modules/tuning/note-expression`, public API surface `Ouaricon::NoteExpression::*`, header-only consumption (D-04..D-09, D-23). Phase 25 v3 does NOT modify the module's source surface.
- One-liner consumer integration via `ouaricon_add_module()` — v3 propagates single-resource installation (collapsed from v2's dual-resource).

**Carrying forward from Phase 24 (locked):**
- 8 affected plugins are the v1.5 cohort: O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant. All 8 confirmed `note-expression` consumers (commit `0ec32e9`).
- Each plugin's installer workflow already exists: `plugin-packaging` / `package` skill (PKG, macOS); `build-installer` skill (EXE, Windows). v3 extends configs, does not redesign workflows.

**Carrying forward from `CLAUDE.md`:**
- Build targets: `ninja <Plugin>_VST3 <Plugin>_AU <Plugin>_Standalone` (macOS); `cmake --build build --config Release --target <Plugin>_VST3` (Windows).
- AU cache clear + remove-old-bundles + fresh install protocol must be honored before each platform's Dorico smoke test.

</domain>

<decisions>
## Implementation Decisions (v3)

> **Numbering note:** v3 starts D-01 from scratch and explicitly cites which v2 decisions it replaces, extends, or carries forward. v3 is much shorter than v2 (single asset, single path, no Playback Template plumbing).

### Distribution Architecture (replaces v2 D-01..D-13)

- **D-01: Single canonical asset, single canonical install path, explicit one-time import.** Ship one file: `Ouaricon-VST3-NoteExpression.doricolib` (Dorico-valid `.doricolib` with full `<kScoreLibrary>` skeleton + the recovered `<ExpressionMapDefinition>`). Installer writes it (and the fallback README) to one platform-specific Ouaricon shared path:
  - macOS: `~/Library/Application Support/Ouaricon/Microtonal Suite/`
  - Windows: `%APPDATA%\Ouaricon\Microtonal Suite\`
  - **No auto-discovery.** Do NOT also write to `Expression Maps/User/`, `Default Library Additions/`, or `PlaybackTemplateSpecs/`. The README directs the user to perform a one-time `Library → Library Manager → Import…` from the canonical Ouaricon path. Most deterministic, simplest plumbing, no Dorico-side scan-behavior dependency. *(User confirmed 2026-04-27.)*
  - **Cost:** one-time manual import per machine. Documented in README + DOCS-02. Revisit auto-discovery in v1.6 if user feedback shows the manual step is friction.

- **D-02: Skip `<pluginNames>` auto-suggest array.** The shipped `.doricolib` does NOT populate the `<pluginNames>` element on its `<ExpressionMapDefinition>`. User picks "Ouaricon VST3 Note Expression" manually from the Endpoints dropdown after the one-time import. Schema verification cost not paid this milestone. *(User confirmed 2026-04-27.)*
  - **Carry-forward note:** if/when v1.6 revisits, ship both prod and dev names (16 entries) so dev installs auto-suggest too. This was the user's preference if `<pluginNames>` were shipped — captured here so future-Claude doesn't re-ask. (User reasoning: dev users matter; we are them.)
  - **Asset is CID-free.** Without `<pluginNames>` and without the Playback Template's `<endpointconfig>`, the shipped XML carries no plugin GUID references. Dev/prod build flavors ship byte-identical `.doricolib` content.

### Asset Authoring (replaces v2 D-03)

- **D-03: Bootstrap from the HALion Sonic factory skeleton; inject the recovered `<ExpressionMapDefinition>`.** The Plan 25-01 v2 strategy of "recover, do not re-author" inherited a defect: the cd2c2c6 XML body is an expression-map *fragment* (only `<expressionMapDefinitions>` under `<kScoreLibrary>`), not a complete library bundle. Dorico requires all ~48 top-level `<kScoreLibrary>` containers as siblings (`<temperaments>`, `<accidentalSystems>`, `<accidentalDefinitions>`, … `<lineStyleCollectionDefinition>`), even when empty (`<entities array="true"/>`). v3 protocol:
  1. Read `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml` — the canonical 48-container factory skeleton.
  2. Empty every container EXCEPT `<expressionMapDefinitions>` (set `<entities array="true"/>` on each emptied one).
  3. Inject the recovered `<ExpressionMapDefinition>` from `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` into `<expressionMapDefinitions>/<entities>`. The element is structurally correct and load-bearing — `microtonalPlaybackMethod=kVST3NoteExpression`, `entityID=xmap.ouaricon.vst3_note_expression`, technique combinations preserved.
  4. Write to `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`.
  - **Reference implementation:** `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B, Dorico-valid, Library Manager → Import: PASS, quarter-sharp smoke: PASS). v3 plan task should follow this build pattern.

### Module-Side Asset Ownership (replaces v2 D-08)

- **D-04: The `note-expression` module owns the single resource via a CMake `install()` rule in `modules/tuning/note-expression/module.cmake`.** Any consumer of `ouaricon_add_module(<Plugin> note-expression)` automatically inherits the `.doricolib` (copied from `resources/library/`) at install time. Mirrors v2's ownership philosophy, simplified from two resources to one.

- **D-05: Resource layout under `modules/tuning/note-expression/resources/`:**
  ```
  resources/
  ├── library/
  │   └── Ouaricon-VST3-NoteExpression.doricolib   # Authored per D-03; the canonical asset
  └── README-microtonal-suite.txt                  # User-facing fallback (INST-04, Path B-flavored)
  ```
  - **Surgical deletion (D-10):** the `playback-template/` subtree from commit `819b2b4` (3 `.xml.in` templates + `playbacktemplatedeps.doricolib.in`) is removed under Plan 25-01 v3.

### Cross-Platform Installer Logic (replaces v2 D-10..D-13)

- **D-06: Per-plugin installer bundles the single asset.** Each of the 8 plugins' PKG (macOS) and EXE (Windows) installers ships `Ouaricon-VST3-NoteExpression.doricolib` + `README-microtonal-suite.txt`. Idempotent overwrite — all 8 installers write the same canonical content.

- **D-07: Single-write per platform** to the Ouaricon shared resources path only:
  - **macOS:** `~/Library/Application Support/Ouaricon/Microtonal Suite/`
  - **Windows:** `%APPDATA%\Ouaricon\Microtonal Suite\`
  - Installer creates the directory if missing.
  - **No Dorico-version probe required for the install destination** (the install target is an Ouaricon-controlled path, not a Dorico-version-specific one). The version-probe pattern from v2's `install-microtonal-suite.cmake.in` is preserved as carry-forward but logically unused — keep the helper for future revival if v1.6 adds auto-discovery; it's a no-op under v3.

### Verification Gates (replaces v2 D-14)

- **D-08: Plan 25-01 v3 Wave 0 retest.** Single ~5 min check on the dev machine before bulk implementation:
  - **Auto-discovery sanity (informational, not blocking):** drop the `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` reference asset into `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/`, restart Dorico, check whether "Ouaricon VST3 Note Expression" appears in the Endpoints dropdown without explicit import. **PASS or FAIL is informational only** — D-01 ships explicit-import regardless. A PASS result gets logged into v1.6 deferred-ideas with concrete evidence.
  - **Cross-platform validation gate (D-08; replaces v2 D-15/D-16):** install reference plugin → Dorico Library Manager Import from canonical path → quarter-sharp C4 → confirm ~269 Hz on both macOS and Windows. macOS reference: O-Lyrica (Phase 23 precedent). Windows reference: planner picks based on availability, recommend O-Lyrica for parity. If Windows access blocked, plan halts hard (don't silently degrade to macOS-only).

### Plan Structure (replaces v2 D-17)

- **D-09: 3 plans, contents reframed for Path B.** Existing v2 PLAN.md files are stale and superseded; `/gsd-plan-phase 25` regenerates them.
  - **`25-01-author-and-plumbing-PLAN.md` (v3)** — Wave 0 auto-discovery sanity check (informational); author the canonical `.doricolib` per D-03 (skeleton + injected expression-map body); collapse `module.cmake`'s install logic from dual-write to single-write of one asset; surgical deletion sweep per D-10; rewrite README content for Path B's import flow (structure preserved); canary install on O-Lyrica proves single-asset single-path pipeline.
  - **`25-02-installer-bundling-sweep-PLAN.md` (v3)** — atomic sweep across all 8 plugins' installer configs (PKG + EXE) bundling the single `.doricolib` + README. Cross-platform installer build + Dorico Library Manager Import + quarter-sharp smoke matrix per D-08. Much smaller scope than v2 (one file, one destination, no Inno Setup Pascal `[Code]` for Dorico-version detection or directory-name variance).
  - **`25-03-internal-notes-PLAN.md` (v3)** — write `research/microtonal-dorico-integration.md` (4 H2 sections, DOCS-01..04). DOCS-02 reframed for Path B's manual-import + manual-assign flow. DOCS-03 covers explicit-import rationale, Dorico Library Manager behavior, the kScoreLibrary 48-container schema requirement (the bug v2 inherited), and rationale for skipping `<pluginNames>` this milestone. DOCS-04 includes new troubleshooting signature: "Library Manager Import: Error opening file: invalid file format" → kScoreLibrary skeleton incomplete.

### v2 Cleanup Strategy (NEW)

- **D-10: Amend-forward with surgical deletions of Path A artifacts.** v3 plans do NOT revert commit `819b2b4` wholesale. Instead, Plan 25-01 v3 surgically deletes only the Path A-specific files and code regions, preserving the carry-forward bits as in-place modifications:
  - **Delete from working tree:**
    - `modules/tuning/note-expression/resources/playback-template/` — entire subtree (3 `.xml.in` templates, `playbacktemplatedeps.doricolib.in`).
    - `ouaricon_extract_vst3_cids` function in `modules/cmake/OuariconModules.cmake` — only consumer was endpointconfig CID substitution; Path B's CID-free asset (D-02) makes this dead code.
    - `.dorico_pt` packing custom command in `modules/tuning/note-expression/module.cmake` (the `cmake -E tar cf … --format=zip` block).
    - The dual-write logic in `install-microtonal-suite.cmake.in` — collapse to single-write of `.doricolib` to Ouaricon shared path. Keep the Dorico-version-probe pattern as commented-out / unused-but-preserved carry-forward; mark as "v1.6 revival candidate".
    - The invalid `library/Ouaricon-VST3-NoteExpression.doricolib` (current content is the truncated-fragment defect) — **rewrite per D-03**, not a delete-then-recreate; v3 task reauthors the file.
  - **Keep from `819b2b4` (no v3 work needed):**
    - `module.yaml` v1.0.0 → v1.1.0 bump (commit `02fdcc2`).
    - `modules/registry.yaml` updated entry.
    - `modules/tuning/note-expression/README.md` "Dorico End-User Setup" structure (rewrite *content* for Path B import flow; structure preserved).
    - `install-microtonal-suite.cmake.in` Dorico-version probe logic (preserved unused under v3 — see above).
  - **Audit-trail consequence:** each surgical deletion is its own task in Plan 25-01 v3, atomically committed. `git log --oneline` will show: v2 author/plumbing commits → v3 surgical deletes → v3 reauthors → v3 single-write collapse → v3 README rewrite → v3 canary. Cleaner than rerunning a full revert + reland sequence; keeps the version-bump and registry commits intact. *(User confirmed 2026-04-27.)*

- **D-11: Stop-on-first-failure, in-plan triage** (carries v2 D-18). Structural failures promote to `25-NN-fix-PLAN.md`.

### Internal Notes Layout (replaces v2 D-19)

- **D-12: Single combined file at `research/microtonal-dorico-integration.md`** with 4 H2 sections:
  - `## Module Architecture` (DOCS-01) — unchanged from v2: NEC advertisement flow, raw-event queue semantics, voice-routing logic, composition with `TuningEngine` analogs. References `modules/tuning/note-expression/cpp/NoteExpression.h` + `cpp/vst3/NoteExpression_VST3.cpp`.
  - `## Canonical Dorico Setup Procedure` (DOCS-02) — **REFRAMED for Path B**: install Ouaricon plugin → open Dorico → `Library → Library Manager → Import…` → select `<canonical Ouaricon path>/Ouaricon-VST3-NoteExpression.doricolib` → confirm import. Then load Ouaricon plugin in score's playback template flow, assign "Ouaricon VST3 Note Expression" via `Play → Endpoints → Expression Map` dropdown. Names exact menu paths.
  - `## Host-Side Behavior Quirks` (DOCS-03) — **REWRITTEN for Path B**: Dorico's neighbor-semitone + NE-delta representation; NEC handshake ignored by Dorico but kept for other hosts; sample-offset timing; **NEW: kScoreLibrary 48-container schema requirement (the v2 inherited defect — Dorico's parser rejects partial-skeleton `.doricolib` files with "invalid file format")**; **NEW: rationale for explicit-import (deterministic, no Dorico-side scan dependency, accepted UX cost of one manual step per user/machine)**; **NEW: rationale for skipping `<pluginNames>` this milestone (schema verification cost not paid; revisit candidate for v1.6)**.
  - `## Troubleshooting Signatures` (DOCS-04) — **REWRITTEN for Path B**: symptoms-vs-cause table for the expression-map-skipped UX trap; new entry: "Library Manager Import: Error opening file: invalid file format" → asset author injected expression-map fragment into incomplete `<kScoreLibrary>` skeleton (recovery: bootstrap from HALion Sonic factory skeleton per D-03); new entry: "Expression map appears in dropdown but quarter-sharp plays at semitone" → user assigned wrong map to channel OR plugin advertises wrong CID (Dorico's expression map binding is name+CID matched).

- **D-13: Notes are developer-facing only (DOCS-05).** No end-user manual or quickstart copy this milestone. Tone: technical reference. (Carries v2 D-20.)

### Stale Artifacts to Clean Up

- **D-14: `build/plugins/<Plugin>/install-doricoexpmap-<Plugin>.cmake` and `install-microtonal-suite-<Plugin>.cmake` files (gitignored).** Generated by reverted v1 and current v2 `configure_file` calls. Vanish on next clean build under v3's collapsed install logic.

### Claude's Discretion

- **Exact ordering of plugins in the bundling sweep** (Plan 25-02 v3). Recommended canary: O-Lyrica first (reference consumer); other 7 in any order — installer-config edit is mechanical.
- **Whether `ouaricon_add_module()` implicitly auto-installs the resource** vs requiring an explicit `ouaricon_install_microtonal_suite()` call. Recommend implicit (matches one-liner integration philosophy from Phase 23 D-26/D-27/D-29). v2 already did implicit; preserve.
- **`fileVersion` in the canonical XML.** Recommend pinning to whatever HALion Sonic factory uses on Dorico 6 (matches the bootstrap skeleton's source). A3 cross-version verification (Dorico 5 import) is non-blocking; defer to research-followup if it surfaces an issue.
- **README.md content rewrite tone.** Recommend keeping the v2 structure (3 sections: Quick Start, Manual Import Steps, Source-of-Truth Note) and updating each for Path B's import-then-assign flow.
- **PLAN.md naming convention.** Recommend `25-NN-<slug>-PLAN.md` matching v1 / v2 / Phase 23/24 style.
- **Whether D-08's auto-discovery sanity result feeds into v1.6 deferred-ideas.** Recommend yes — log result with concrete date and evidence so v1.6 has a starting point.

</decisions>

<specifics>
## Specific Ideas

- **"Bootstrap from the factory skeleton, never re-author the kScoreLibrary."** The HALion Sonic factory `expressionMapsDefinitions.xml` shipped inside Dorico 6's app bundle is the canonical reference for the 48-container `<kScoreLibrary>` structure. v2 inherited the truncated-fragment defect by trusting the recovered cd2c2c6 XML body without verifying Dorico's parser would accept it. v3 task: load factory file, empty all containers except `<expressionMapDefinitions>`, inject recovered `<ExpressionMapDefinition>`, write. The reference asset at `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` is the proof-of-concept.
- **"Explicit-import is a feature, not a regression."** No Dorico-side scan-behavior dependency means our distribution is deterministic across Dorico versions, plugin installation orders, and user environments. The one-time manual step is documented and easily understood. Auto-discovery is a v1.6 polish candidate, not a v1.5 blocker.
- **"`<pluginNames>` skipped means the asset is CID-free."** No `<pluginNames>` element + no `<endpointconfig>` (Path A artifact) = no plugin GUID references in the shipped XML. Dev and prod build flavors ship byte-identical `.doricolib` content. The dev/prod CID divergence problem that v2 solved with `ouaricon_extract_vst3_cids` is moot under Path B — that helper is dead code and gets deleted under D-10.
- **"Amend-forward, not full-revert."** Commit `819b2b4` carried real value: version bump (`02fdcc2`), registry entry, README skeleton, version-probe pattern, recovered XML body. Surgically deleting only the Path A-specific files (.dorico_pt packing, CID helper, .xml.in templates, embedded .doricolib.in) preserves those wins and avoids re-doing work. Each deletion is its own atomic commit for clean audit trail.
- **"Plan 25-02 v3 is dramatically smaller than v2."** No Inno Setup Pascal `[Code]` for Dorico-version detection. No `Default Library Additions` directory creation. No spaces-vs-no-spaces directory-name handling. One `[Files]` entry per platform per plugin. Sweep is purely mechanical.
- **"v1.5 ships first; v1.6 evaluates the friction."** Real user feedback after v1.5 ships will tell us whether explicit-import friction or skipped-auto-suggest friction warrants the schema-verification + asset-population work. Empirical signal beats premature optimization.
- **"Internal docs document the failure mode, not just the success path."** DOCS-03 and DOCS-04 explicitly capture the kScoreLibrary 48-container schema requirement (the v2 inherited defect) and the symptom-cause mapping for "Library Manager Import: invalid file format". Future implementers (us, v1.6) get the failure mode in the troubleshooting bible.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents (researcher, planner, executor) MUST read these before planning or implementing.**

### Phase Scoping
- `.planning/ROADMAP.md` § Phase 25 — goal, dependencies (Phase 24), 5 success criteria. Note: criterion #1 mentions `.doricoexpmap` by name; success is measured by *behavioral* equivalence (canonical asset exists at module path; installers bundle it; user can route quarter-sharp via VST3 NE through Dorico). The single Path B `.doricolib` satisfies the same intent.
- `.planning/REQUIREMENTS.md` § INST-01..04, § DOCS-01..05 — binding requirements. Same observation as above re: filename in INST-01 — canonical asset is one `.doricolib`; intent is preserved.
- `.planning/REQUIREMENTS.md` § FUT-06 — end-user manual/quickstart deferred; DOCS-05 boundary.

### Mandatory Reads (architectural-pivot trail — newest first)
- `.planning/phases/25-package-docs/25-FINDING-path-b-validation.md` — **MANDATORY READ.** Records v2 → v3 pivot. Documents two concrete bugs in commit `819b2b4` (invalid `.doricolib` format; dev/prod plugin-name divergence) and the architectural mismatch (Playback Template over-engineered for the use case). Path B end-to-end PASS verified 2026-04-27.
- `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` § A2 Result — verification log with full test trace, hypothesis chain, and final PASS at ~269 Hz quarter-sharp C4 on macOS Dorico 6 / O-Lyrica-dev.
- `.planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md` — earlier v1 → v2 pivot. Background context on why `.doricoexpmap` failed.
- `.planning/phases/25-package-docs/25-RESEARCH.md` — v2 research; superseded by Path B finding for distribution mechanism, but still authoritative on dev/prod CID layouts (informational, not consumed by v3 plumbing).
- `.planning/phases/24-propagate/24-CONTEXT.md` — Phase 24 design decisions, especially D-12 (stop-on-first-failure playbook).
- `.planning/phases/23-extract/23-CONTEXT.md` — Phase 23 module ownership philosophy. Phase 25 v3's "module owns the asset" decision (D-04) extends Phase 23's principles to the data resource layer.
- `.planning/seeds/microtonal-shared-module.md` — original seed; rationale for the microtonal milestone.

### Recovery / Reference Sources
- `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` — recovered `<ExpressionMapDefinition>` element. Use the element body as-is per D-03; do NOT reuse the surrounding `<kScoreLibrary>` skeleton (it's the defect).
- `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml` — **the kScoreLibrary skeleton authority.** v3 task bootstraps from this file per D-03.
- `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B) — **reference implementation for the v3 authoring task.** Dorico 6 Library Manager → Import: PASS. Quarter-sharp C4: PASS. v3 plan's authoring task should produce a byte-identical or functionally-equivalent file.

### Implementation Bible (auto-loaded skill)
- `.claude/skills/spike-findings-VST-development/SKILL.md` — findings index.
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — validated patterns 1–5, landmines 1–5, constraints. **The `kVST3NoteExpression` Microtonality invariant (Landmine 3) is what the recovered `<ExpressionMapDefinition>` encodes.**
- `.claude/skills/spike-findings-VST-development/sources/` — spike-era reference code; useful for DOCS-01 architecture cross-references.

### Background Research (carry-forward — still applicable)
- `.planning/notes/dorico-microtonal-vst-research.md` — Dorico's wire mechanisms; source material for DOCS-02/DOCS-03 (REFRAMED for Path B).
- `research/microtonality-implementation-juce.md`
- `research/microtonality-theory-formats.md`
- `research/microtonality-comprehensive-database.md`
- `research/microtonality-commercial-performance.md`

### Module Surface (consume; module install() rules will be ADDED/MODIFIED in this phase)
- `modules/tuning/note-expression/cpp/NoteExpression.h` — public API. Source for DOCS-01.
- `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` — VST3-only TU. Source for DOCS-01.
- `modules/tuning/note-expression/README.md` — already updated by `819b2b4`; v3 rewrites *content* of "Dorico End-User Setup" section for Path B import flow; structure preserved.
- `modules/tuning/note-expression/module.yaml` — already at 1.1.0 per `02fdcc2`; no v3 changes.
- `modules/tuning/note-expression/module.cmake` — currently has v2's `.dorico_pt` packing + dual-write install rules; v3 surgically deletes the packing block and collapses install rules to single-write of `.doricolib`.
- `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — currently invalid (truncated fragment); v3 reauthors per D-03.
- `modules/tuning/note-expression/resources/playback-template/` — DELETED entire subtree under D-10.
- `modules/tuning/note-expression/resources/README-microtonal-suite.txt` — content rewritten for Path B's manual-import flow.
- `modules/registry.yaml` — already updated by `819b2b4`; no v3 changes.

### Module System Plumbing
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` macro intact; v3 surgically DELETES the `ouaricon_extract_vst3_cids()` helper added in `ef64ec4` (dead code under Path B).

### Installer Workflows (the bundling targets — extend, do not redesign)
- `.claude/skills/plugin-packaging/SKILL.md` — macOS PKG installer workflow. Plan 25-02 v3 extends per-plugin packaging configs.
- `.claude/skills/package/SKILL.md` — `/package` command. Same workflow.
- `.claude/skills/build-installer/SKILL.md` — Windows EXE via Inno Setup. Plan 25-02 v3 extends per-plugin Inno Setup configs (single `[Files]` entry, no Pascal `[Code]` Dorico-version detection).

### Build & Install Discipline
- `CLAUDE.md` — Plugin Cache Clearing protocol on macOS; Windows installer + cache-clear steps. **Mandatory before each platform's Dorico smoke test in Plan 25-02 v3.**

### Reference Plugins (the 8 affected; installer configs touched in Plan 25-02 v3)
- `plugins/O-Lyrica/CMakeLists.txt`, `plugins/O-Bells/CMakeLists.txt`, `plugins/O-IntonationPad/CMakeLists.txt`, `plugins/O-Prism/CMakeLists.txt`, `plugins/O-Wind/CMakeLists.txt`, `plugins/O-Reed/CMakeLists.txt`, `plugins/O-Bowed/CMakeLists.txt`, `plugins/O-Formant/CMakeLists.txt`. Each consumes `note-expression`. Installer configs under each plugin's packaging entry point.

### Phase 24 Closeout (predecessor — read for context)
- `.planning/phases/24-propagate/24-VERIFICATION.md` — confirms all 8 plugins ship with NE module, 8/8 PASS Dorico smoke.
- `.planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md` — aggregate matrix.

### Dorico Documentation (external)
- Steinberg Dorico Library Manager Import flow (current Dorico 6 docs). Researcher fetches at planning time for DOCS-02 menu-path accuracy.

</canonical_refs>

<code_context>
## Existing Code Insights (v3)

### Reusable Assets
- **`modules/tuning/note-expression/`** — fully extracted module with stable API. v3 modifies only the `resources/` subtree and `module.cmake`'s install logic. Module source surface untouched.
- **v2 carry-forward in `819b2b4`:** module.yaml v1.1.0 bump (`02fdcc2`), registry.yaml entry, README.md "Dorico End-User Setup" section structure, `install-microtonal-suite.cmake.in` Dorico-version probe pattern, recovered `<ExpressionMapDefinition>` body. All preserved under D-10.
- **`/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib`** — Dorico-valid reference asset. v3 authoring task should produce equivalent output.
- **`.claude/skills/plugin-packaging/SKILL.md` + `.claude/skills/package/SKILL.md`** — macOS PKG installer workflow. v3 extends per-plugin configs with one payload entry.
- **`.claude/skills/build-installer/SKILL.md`** — Windows Inno Setup EXE workflow. v3 extends each plugin's `.iss` template with one `[Files]` entry.
- **`research/` directory** — already contains 4 microtonality research files. Plan 25-03 v3's `research/microtonal-dorico-integration.md` is a NEW file in this same family.

### Established Patterns
- **Module owns the asset** — Phase 23 established for source code, JUCE patch, README; v3 extends to one data resource (`.doricolib`).
- **One-liner consumer integration** — `ouaricon_add_module(<Plugin> note-expression)` already wires source compilation + patch-marker check + (post-v2) resource install. v3 simplifies the resource install from dual-write to single-write.
- **Atomic plan = atomic commit** — preserved.
- **Stop-on-first-failure with in-plan triage** — preserved (D-11).
- **Per-platform install** — collapses from v2's two-files-multiple-paths to v3's one-file-one-path.
- **Cross-platform validation gate** — preserved; verification protocol simplifies to "Library Manager Import" instead of "Playback Template apply".

### NEW Patterns this phase
- **Factory-skeleton bootstrap for Dorico XML resources** (D-03) — load HALion Sonic factory `expressionMapsDefinitions.xml`, empty all containers except the relevant one, inject the project-specific element. Establishes a precedent for any future Dorico XML asset authoring. The recovered cd2c2c6 fragment is reusable as the *injected element*, NOT as a complete library bundle.
- **Surgical amend-forward** (D-10) — instead of reverting a merged feature commit, delete only the dead-end files and code regions in their own atomic commits, preserving the carry-forward bits in place. Cleaner audit trail than full revert + reland; precedent for future architectural pivots in mid-implementation phases.

### Integration Points (per-platform install pipeline, v3)

**Module side (one-time, Plan 25-01 v3):**
1. `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — REAUTHOR per D-03 (replace truncated-fragment with factory-skeleton-bootstrapped valid asset).
2. `modules/tuning/note-expression/resources/playback-template/` — DELETE entire subtree (D-10).
3. `modules/tuning/note-expression/resources/README-microtonal-suite.txt` — REWRITE content for Path B import flow.
4. `modules/cmake/OuariconModules.cmake` — DELETE `ouaricon_extract_vst3_cids()` function added in `ef64ec4` (D-10).
5. `modules/tuning/note-expression/module.cmake` — DELETE `.dorico_pt` packing custom command; COLLAPSE dual `install()` rules to single-write of `.doricolib` to Ouaricon shared path.
6. `modules/tuning/note-expression/install-microtonal-suite.cmake.in` (or wherever the staged install script lives) — collapse dual-write logic to single-write; preserve Dorico-version probe as commented unused-but-preserved (v1.6 revival candidate).
7. `modules/tuning/note-expression/README.md` — REWRITE "Dorico End-User Setup" section content for Path B import flow; preserve overall README structure.

**Plugin side (per-plugin, Plan 25-02 v3 — atomic sweep):**
- Each plugin's PKG payload + Inno Setup `[Files]` section updated to consume the module's single staged resource (`.doricolib` + README).
- Per-platform install destinations follow D-07 (Ouaricon shared only).
- Inno Setup `[Code]` Pascal section: NO Dorico-version detection needed; NO `Default Library Additions` directory creation needed.

**Notes side (one-time, Plan 25-03 v3):**
- `research/microtonal-dorico-integration.md` — NEW file, single combined doc with 4 H2 sections (DOCS-01..04) reframed for Path B's manual-import + manual-assign flow.

### Variation Points
- **macOS PKG payload structure** — postinstall script copies `.doricolib` + README to `~/Library/Application Support/Ouaricon/Microtonal Suite/`. Single source-dest pair per file. Idempotent.
- **Windows Inno Setup `[Files]` section** — two source entries (`.doricolib` + README), each with one destination. Inno's `external: yes` flag NOT needed.
- **Dorico-version detection** — NOT REQUIRED for v3's install destinations. The Ouaricon shared path is Dorico-version-agnostic. (User performs Library Manager → Import once; Dorico itself handles version compatibility on import.)

### Phase 25 v3 Touch Points (the package surface)

**Module-side (modify-and-delete, Plan 25-01 v3):**
- `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — reauthored.
- `modules/tuning/note-expression/resources/playback-template/` — entire subtree deleted.
- `modules/tuning/note-expression/resources/README-microtonal-suite.txt` — rewritten.
- `modules/tuning/note-expression/module.cmake` — surgical deletions + install collapse.
- `modules/tuning/note-expression/README.md` — content rewrite, structure preserved.
- `modules/cmake/OuariconModules.cmake` — `ouaricon_extract_vst3_cids()` deleted.
- `modules/tuning/note-expression/install-microtonal-suite.cmake.in` — single-write collapse.

**Module-side (no-op, preserved from `819b2b4`):**
- `modules/tuning/note-expression/module.yaml` — already at 1.1.0.
- `modules/registry.yaml` — already updated.

**Plugin-side (per-plugin, Plan 25-02 v3):**
- 8 plugins' packaging configs — extended (single bundle entry per platform).

**Skill-side (potentially):**
- `.claude/skills/plugin-packaging/SKILL.md` and `.claude/skills/build-installer/SKILL.md` — minimal/no template-level updates expected. Planner determines.

**Notes-side (Plan 25-03 v3):**
- `research/microtonal-dorico-integration.md` — NEW.

**Stale build outputs (auto-cleanup):**
- `build/plugins/<Plugin>/install-doricoexpmap-<Plugin>.cmake` (v1) and `install-microtonal-suite-<Plugin>.cmake` (v2) — gitignored, vanish on next clean build (D-14).

</code_context>

<deferred>
## Deferred Ideas

- **Auto-discovery via `Expression Maps/User/`** — D-08's auto-discovery sanity check is informational; if PASS, log result + concrete date in v1.6 deferred-ideas as a no-friction-cost enhancement. Plan 25-01 v3 ships explicit-import regardless.
- **`<pluginNames>` array for auto-suggestion** — schema verification + 8 (or 16) entries. User preference if revisited: ship both prod + dev names (16 entries). Big UX win for ~30 min of work; deferred to v1.6 with clear user-feedback signal.
- **`.dorico_pt` Playback Template architecture** — the v2 approach is rejected for the v1.5 routing-only use case but may resurface in a later milestone if curated `slot<N>.pluginstate` snapshots become a feature (e.g., "demo project setup" workflow). Architecture documented in `25-FINDING-path-b-validation.md`.
- **Curated `slot<N>.pluginstate` per plugin** — depends on Playback Template revival. Not relevant under v3.
- **Articulation switches in the canonical .doricolib** — staccato, legato, dynamics. Out of scope for v1.5 (microtonality-only). Revisit when there's user demand or website-manual content needs articulation coverage.
- **Per-plugin `.doricolib` variants** — e.g., MPE plugins (O-Reed, O-Bowed) potentially needing different routing. Defer until a real user-facing playback issue surfaces; the single canonical map is correct for the milestone.
- **Automated Dorico smoke harness** — still manual per platform.
- **Multi-Dorico-version installer logic** — not needed under v3 (install destination is Dorico-version-agnostic). The `install-microtonal-suite.cmake.in` Dorico-version probe is preserved as commented unused-but-preserved code, ready for v1.6 revival if auto-discovery work needs it.
- **Cubase / Nuendo expression-map paths** — Cubase reads expression maps too. Out of scope for v1.5 (Dorico-only). Future generalization candidate.
- **Module-side `.doricolib` schema linting in CI** — programmatic XML schema validation before packaging, specifically the kScoreLibrary 48-container check. Manual smoke + the factory-skeleton bootstrap pattern (D-03) covers it for v1.5.
- **Updating each plugin's CHANGELOG with "now ships Ouaricon Microtonal Suite expression map"** — optional. Not required by INST/DOCS. If included, recommend uniform one-line entry without binary-version bump.
- **Cross-version `fileVersion` handling** — A3 not blocking; pin to whatever HALion Sonic factory uses on Dorico 6. Verify Dorico 5 import in research-followup if user reports an issue.
- **End-user-facing manual / quickstart on the sales website** — FUT-06.

</deferred>

---

*Phase: 25-package-docs*
*Context gathered: 2026-04-27 (v3 — Path B validation locked)*
*v2 superseded — see `25-FINDING-path-b-validation.md` and `25-01-WAVE-0-VERIFICATION.md` § A2 Result.*
*v1 superseded — see `25-FINDING-playback-template-pivot.md` and revert commit `d2c86c5`.*
