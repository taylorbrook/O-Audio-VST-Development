# Phase 25: Package & Internal Technical Notes — Context

**Gathered:** 2026-04-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Author one canonical Dorico expression map (`Ouaricon-VST3-NoteExpression.doricoexpmap`), bundle it in all 8 affected plugins' installers (PKG on macOS, EXE on Windows) so it ships with every plugin in the v1.5 microtonal cohort, and capture developer-facing internal technical notes under `research/` covering module architecture, Dorico setup procedure, host-side quirks, and troubleshooting signatures. The expression map is the single source of truth at `modules/tuning/note-expression/resources/`; consumed via the existing module system; written to two locations on user systems (Ouaricon shared resources path + Dorico's auto-scan path) so the map is both editable and auto-discovered.

**In scope:**
- Authoring `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` (microtonality-only minimum, hand-authored from scratch).
- Module-level `install()` rule + propagation logic so each consuming plugin inherits the resource via `ouaricon_add_module()`.
- Per-platform dual-write installer logic: PKG (macOS) and Inno Setup EXE (Windows) each write to (a) Ouaricon shared resources path and (b) Dorico user expression-maps scan path.
- All 8 affected plugins' installer configs updated to bundle and dual-write the file.
- Cross-platform validation: dry-run install + Dorico quarter-sharp smoke test on **both** macOS and Windows.
- Internal notes under `research/microtonal-dorico-integration.md` (single combined file with 4 H2 sections covering DOCS-01..04).

**Out of scope (deferred to other phases / future work):**
- End-user-facing manuals or quickstart guides on the sales website (FUT-06; DOCS-01..05 explicitly remain internal this milestone).
- Articulation switches in the expression map (staccato, legato, dynamics) — microtonality-only this milestone.
- Per-plugin .doricoexpmap variants — single canonical file across all 8 plugins.
- MTS-ESP, MPE, pitch-bend fallback (FUT-02..04).
- Per-note custom NE types beyond `kTuningTypeID` (FUT-02).
- Automated Dorico smoke harness (still manual; deferred from Phase 24).

**Carrying forward from Phase 23 (locked, not re-discussed):**
- Module path `modules/tuning/note-expression`, public API surface `Ouaricon::NoteExpression::*`, header-only consumption (D-04..D-09, D-23).
- Per-format module-source convention (`cpp/<format>/`) — not directly relevant to Phase 25 since resources are configuration data not source code, but the module-owned-asset principle extends naturally to resources.
- One-liner consumer integration via `ouaricon_add_module()` — Phase 25 extends this to propagate resource installation, not just source compilation.
- Atomic plan = atomic commit (Phase 23/24 plan-checker discipline).

**Carrying forward from Phase 24 (locked):**
- 8 affected plugins are the v1.5 cohort: O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant. All 8 are confirmed `note-expression` consumers in `modules/registry.yaml` `used_by:` (per Phase 24 final sweep, commit 0ec32e9).
- Each plugin's installer workflow already exists: `plugin-packaging` / `package` skill produces signed PKG (macOS); `build-installer` skill produces Inno Setup EXE (Windows). Phase 25 extends configs, does not redesign workflows.

**Carrying forward from `CLAUDE.md`:**
- Build targets: `ninja <Plugin>_VST3 <Plugin>_AU <Plugin>_Standalone` (macOS); `cmake --build build --config Release --target <Plugin>_VST3` (Windows).
- AU cache clear + remove old bundles + fresh install protocol must be honored before any cross-platform Dorico test.

</domain>

<decisions>
## Implementation Decisions

### Expression Map Authoring (INST-01, INST-02)

- **D-01: Author the canonical `.doricoexpmap` from scratch.** Hand-write the XML referencing the spike-validated patterns (Patterns 1–3 in `vst3-note-expression-dorico.md`). Most control over field set, no provenance or licensing baggage from third-party templates. The file is the artifact the v1.5 milestone is built around — owning its origin matters.
- **D-02: Microtonality-only minimum scope.** Microtonality field set explicitly to `"VST3 Note Expression"` (the spike-validated trap from Patterns 1–3 — Dorico's `Auto` selection picks pitch-bend for non-Steinberg VST3s and silently breaks microtonal playback). No articulation switches (staccato/legato/dynamics) or per-plugin variants this milestone. Each plugin's natural Dorico articulation behavior remains the responsibility of Dorico's defaults. Smallest possible file; broadest applicability across 8 distinct plugins.
- **D-03: Single canonical file at `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`.** No per-plugin variants. The 8 plugins all consume the identical asset.

### Module-Side Asset Ownership (INST-02)

- **D-04: The `note-expression` module owns the resource via a CMake `install()` rule in `modules/tuning/note-expression/module.cmake`.** Any consumer of `ouaricon_add_module(<Plugin> note-expression)` automatically inherits the resource staging. Each plugin's installer config reads from the staged location — no per-plugin manual copy of the file. Mirrors Phase 23's "module owns everything" invariant: just as the JUCE-NE-PATCH marker check fires per-consumer at configure time (D-15), the `install()` rule fires per-consumer at install/package time. Module v1.0.0 → v1.1.0 minor bump (additive resource surface, not a breaking API change; recommended per semver). `modules/registry.yaml` updated accordingly.
- **D-05: Single shared install location for the file (one canonical user-facing copy per host machine, regardless of how many of the 8 plugins are installed).**
  - macOS: `~/Library/Application Support/Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap`
  - Windows: `%APPDATA%\Ouaricon\Expression Maps\Ouaricon-VST3-NoteExpression.doricoexpmap`
  - Each of the 8 installers writes the same file to the same location — idempotent overwrite. User-editable, discoverable, no duplication. No bundle-internal hiding (file is NOT placed inside `.vst3/Contents/Resources/`).

### Cross-Platform Installer Logic (INST-03, INST-04)

- **D-06: Dual-write to both Ouaricon shared resources AND Dorico's user expression-maps scan path.** Best UX — the user opens Dorico after installation and the map is already in the picker, no manual import step required. Locations:
  - macOS shared: `~/Library/Application Support/Ouaricon/Expression Maps/`
  - macOS Dorico scan: `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/`
  - Windows shared: `%APPDATA%\Ouaricon\Expression Maps\`
  - Windows Dorico scan: `%APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\`
- **D-07: Dorico version targeting — install to the latest Dorico version directory present on the host machine** (likely Dorico 5 / Dorico 6 — confirm at planning time which is current). Document the version in DOCS-02. If multiple Dorico versions are installed, install to the latest detected. Future Dorico releases require an installer config update — note as a known maintenance touchpoint in DOCS-03.
- **D-08: README emission as fallback (INST-04).** Each installer also writes a short README at the Ouaricon shared resources path explaining (a) the file's purpose, (b) how to manually import via Dorico's Library menu if the auto-scan path didn't catch it (e.g., Dorico version mismatch), and (c) the canonical file's source-of-truth location in the module repo. README is plain text — internal-developer-style, not end-user marketing copy (honors DOCS-05 boundary).

### Validation Gate (INST-03 Success Criterion #2)

- **D-09: Validate on BOTH macOS and Windows.** Reverses Phase 23/24's implicit FUT-01 deferral for this phase only — installer bundling and dual-write logic must be platform-symmetric, so each platform must pass a real Dorico quarter-sharp smoke test. Planner must budget for Windows machine access + Dorico installation. If Windows access is blocked, the plan must surface this as a hard halt (don't silently fall back to macOS-only).
- **D-10: Per-platform validation matrix.** One representative plugin per platform (e.g., O-Lyrica on macOS as the reference consumer; planner picks the Windows representative based on availability — likely O-Bells or O-Lyrica). Quarter-sharp C4 smoke = pitch lands at +50¢ above C4, no attack zipper, NE correlated by `noteId`. Same 3-point gate as Phase 24's D-07. Aggregate results recorded in the bundling sweep plan's SUMMARY.md.

### Plan Structure

- **D-11: 3 plans (grouped sweep model).**
  - **`25-01-author-and-plumbing-PLAN.md`** — author the canonical `.doricoexpmap` from scratch + add module `install()` rule in `modules/tuning/note-expression/module.cmake` + bump module to v1.1.0 + update `modules/registry.yaml` + write per-platform dual-write logic (CMake helpers or installer-config snippets). Deliverable: file exists at canonical path; module advertises it; one consumer plugin proves the install pipeline end-to-end (planner picks the canary, recommend O-Lyrica as the reference consumer per Phase 23 precedent).
  - **`25-02-installer-bundling-sweep-PLAN.md`** — atomic sweep across all 8 plugins' installer configs. Each plugin's PKG and EXE installer config (under `plugin-packaging` / `build-installer` workflow inputs) is updated to consume the module's staged resource. Cross-platform installer build + dry-run install + Dorico quarter-sharp smoke validation matrix (macOS + Windows). One commit covering the 8-plugin installer config sweep + validation results. Mirrors Phase 24's `24-08-final-sweep-PLAN.md` shape.
  - **`25-03-internal-notes-PLAN.md`** — write `research/microtonal-dorico-integration.md` covering all 4 DOCS topics (single combined file, 4 H2 sections). Atomic commit.
- **D-12: Stop-on-first-failure, triage in same plan.** Inherits Phase 23/24's playbook (D-12 from Phase 24 CONTEXT.md). If the bundling sweep fails on plugin N (e.g., a per-platform installer-config edge case), halt and diagnose inline. Structural failures (e.g., the module `install()` rule has a defect) get promoted to a `25-NN-fix-PLAN.md`.

### Internal Notes Layout (DOCS-01..05)

- **D-13: Single combined file at `research/microtonal-dorico-integration.md`** with 4 H2 sections, one per DOCS topic:
  - `## Module Architecture` (DOCS-01) — NEC advertisement flow, raw-event queue semantics, voice-routing logic, composition with each plugin's `TuningEngine` analog. References `modules/tuning/note-expression/cpp/NoteExpression.h` + `cpp/vst3/NoteExpression_VST3.cpp` for concrete API. Cross-references the 5 propagation patterns catalogued in Phase 24 plan 24-08 (classic-Synthesiser-multi-osc, classic-Synthesiser-physical-period, classic-Synthesiser-multi-sub-voice, MPE-helper-based, MPE-per-call-site).
  - `## Canonical Dorico Setup Procedure` (DOCS-02) — step-by-step, host-version noted (Dorico 5 / Dorico 6), suitable for direct translation into future website manual/quickstart copy. Names the exact Dorico menu paths for importing the expression map and confirming Microtonality is set to "VST3 Note Expression".
  - `## Host-Side Behavior Quirks` (DOCS-03) — Dorico's neighbor-semitone + NE-delta representation (e.g., `quarter-sharp C4 = pitch C#4 + NE -50¢`); NEC handshake is ignored by Dorico but kept for other hosts; sample-offset timing requirements; multiple-Dorico-version installer caveat from D-07.
  - `## Troubleshooting Signatures` (DOCS-04) — symptoms-vs-cause table for the expression-map-skipped UX trap (matches Spike 002 observations); known regression signatures from Phase 23 (AU-link Steinberg-symbol leak) and Phase 24 (per-plugin propagation failures).
- **D-14: Notes are developer-facing only (DOCS-05).** No end-user manual or quickstart copy published this milestone — DOCS-01..04 are structured to translate cleanly into future website authoring (FUT-06). Tone: technical reference, not marketing.

### Claude's Discretion

- **Exact ordering of plugins inside the bundling sweep (D-11).** Planner picks based on installer-config simplicity. Recommended canary order: O-Lyrica first (already validated as reference consumer in Phase 23), then the other 7 in any order since the installer-config edit is mechanical.
- **CMake mechanism for per-platform dual-write (D-04, D-06).** Planner chooses between (a) a single shared CMake helper invoked by each plugin's installer config, (b) module-side `install(...)` rule with `CONFIGURATIONS Release` + per-platform `if(APPLE)/if(WIN32)` branches, or (c) injection at `plugin-packaging` / `build-installer` skill level. Recommend the module-side `install()` rule as cleanest.
- **Whether to add a `modules/tuning/note-expression/module.cmake`-level CMake function `ouaricon_install_dorico_expression_map()`** that consuming plugins explicitly invoke, vs auto-installing as a side effect of `ouaricon_add_module()`. Recommend implicit (auto-install) — matches the one-liner integration philosophy from Phase 23 (D-26/D-27/D-29).
- **Exact Dorico version tested (D-07).** Planner uses whatever's currently installed on the user's machine — likely Dorico 5 or 6. Documented in DOCS-02. Multi-version logic in installers is desirable but optional.
- **Whether to bundle the README inside the .vst3 too (belt-and-braces).** Recommend NO — single shared location only, avoid duplication.
- **PLAN.md naming convention.** Recommend `25-NN-<slug>-PLAN.md` matching Phase 23/24 style.

</decisions>

<specifics>
## Specific Ideas

- **"One canonical .doricoexpmap, hand-written from scratch."** The v1.5 milestone is built around this single file — owning its provenance and field-set composition matters more than saving authoring time by copying a vendor template. The file is small enough that hand-authoring is the right level of investment.
- **"The module owns the asset, not the plugins."** Phase 23 established the principle that the `note-expression` module owns everything related to NE — patch, source, README. Phase 25 extends this to resources: the module owns the canonical `.doricoexpmap`, and the install pipeline propagates it to consumers. No per-plugin copy of the file in the source tree.
- **"Dual-write so users get auto-discovery without losing editability."** Writing to Ouaricon shared resources path makes the file editable, discoverable, and version-portable. Writing to Dorico's auto-scan path makes it available in Dorico's picker without the user touching the Library menu. Both writes target the same file content; the canonical edit point is the shared resources copy.
- **"Microtonality-only is enough for this milestone."** Articulations are a separate concern; conflating them into the canonical map balloons authoring effort and locks downstream maintenance to articulation conventions that may shift. The phase boundary is microtonality.
- **"Validate on both platforms because dual-write only matters if both platforms work."** macOS-only validation would leave Windows installer logic unverified — and Phase 23/24 already deferred Windows verification to FUT-01. Phase 25 must close that gap for the .doricoexpmap pipeline specifically (not for the broader plugin behavior — that remains FUT-01).
- **"Internal notes are source material for future website authoring."** DOCS-05 mandates internal-only this milestone, but DOCS-01..04 are structured so they translate cleanly into website manual chapters. Tone is technical reference, not user marketing — the website team translates later (FUT-06).
- **"Module v1.0.0 → v1.1.0 minor bump."** Adding a resource to the module's surface is additive (semver minor). Not a defect; not a breaking change. Updates `modules/registry.yaml` and the module's own changelog. First module version bump since extraction in Phase 23.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents (researcher, planner, executor) MUST read these before planning or implementing.**

### Phase Scoping
- `.planning/ROADMAP.md` §Phase 25 — goal, dependencies (Phase 24), 5 success criteria, requirements list.
- `.planning/REQUIREMENTS.md` §INST-01..04, §DOCS-01..05 — binding requirements for this phase.
- `.planning/REQUIREMENTS.md` §FUT-06 — end-user manual/quickstart deferred to website authoring; DOCS-05 boundary.
- `.planning/phases/24-propagate/24-CONTEXT.md` — Phase 24 design decisions, especially D-12 (stop-on-first-failure playbook) and the 5 propagation patterns catalogued in plan 24-08.
- `.planning/phases/23-extract/23-CONTEXT.md` — Phase 23 module ownership philosophy, per-format routing convention, JUCE patch discipline. Phase 25's "module owns the asset" decision (D-04) is a direct extension of these principles.
- `.planning/seeds/microtonal-shared-module.md` — original seed; rationale for the microtonal milestone.

### Implementation Bible (auto-loaded skill)
- `.claude/skills/spike-findings-VST-development/SKILL.md` — findings index.
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — validated patterns 1–5, landmines 1–5, constraints. **Pattern: Microtonality must be set to "VST3 Note Expression" (Auto picks pitch-bend, silently breaks microtonal playback) — this is THE invariant the .doricoexpmap encodes.**
- `.claude/skills/spike-findings-VST-development/sources/` — spike-era reference code; useful for DOCS-01 (architecture notes) cross-references.

### Background Research
- `.planning/notes/dorico-microtonal-vst-research.md` — Dorico's 3 wire mechanisms, why VST3 Note Expression is the right target, expression-map setup quirk. **Source material for DOCS-02 (canonical setup procedure) and DOCS-03 (host-side quirks).**
- `research/microtonality-implementation-juce.md` — JUCE-side microtonality patterns; useful cross-reference for DOCS-01 architecture notes.
- `research/microtonality-theory-formats.md` — broader microtonality format landscape; useful for DOCS-03 (positions VST3 Note Expression among alternatives).
- `research/microtonality-comprehensive-database.md` — vendor/host support landscape; useful for DOCS-03 and DOCS-04 (which hosts behave differently).
- `research/microtonality-commercial-performance.md` — commercial benchmarks; useful for the website-translation phase (FUT-06), not strictly needed for DOCS-01..04.

### Module Surface (consume; module install() rule will be ADDED in this phase)
- `modules/tuning/note-expression/cpp/NoteExpression.h` — public API: `PendingTuningTable`, `applyPendingTuning`, `VST3Extensions`. Source for DOCS-01 architecture notes.
- `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` — VST3-only TU, NEC body, dispatch registration. Source for DOCS-01 architecture notes.
- `modules/tuning/note-expression/README.md` — consumer integration steps; will be updated to mention the canonical .doricoexpmap and the auto-install behavior.
- `modules/tuning/note-expression/module.yaml` — module metadata; bumped 1.0.0 → 1.1.0 in plan 25-01.
- `modules/tuning/note-expression/module.cmake` — currently fires the JUCE-NE-PATCH marker check; plan 25-01 ADDS the `install()` rule for the .doricoexpmap here.
- `modules/registry.yaml` — `note-expression` entry; version field bumped to 1.1.0; changelog entry added.

### Module System Plumbing
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` macro. May be extended in plan 25-01 if implicit auto-install of resources requires plumbing here.

### Installer Workflows (the bundling targets — extend, do not redesign)
- `.claude/skills/plugin-packaging/SKILL.md` — macOS PKG installer workflow; signed, branded. Plan 25-02 extends per-plugin packaging configs.
- `.claude/skills/package/SKILL.md` — `/package` command (macOS PKG). Same workflow as plugin-packaging.
- `.claude/skills/build-installer/SKILL.md` — Windows EXE installer via Inno Setup. Plan 25-02 extends per-plugin installer scripts.

### `/improve` Workflow (relevant for changelog/version bump on module)
- `.claude/skills/plugin-improve/SKILL.md` — does NOT apply to module-level version bumps (that's manual). Phase 25 plan 25-01 manually bumps `modules/tuning/note-expression/module.yaml` and updates `modules/registry.yaml` directly. `/improve` only applies if a plugin's CMakeLists.txt needs a version bump as a side effect of the bundling sweep — not anticipated.

### Build & Install Discipline
- `CLAUDE.md` — Plugin Cache Clearing protocol on macOS; Windows installer + cache-clear steps. **Mandatory before each platform's Dorico smoke test in plan 25-02.**

### Reference Plugins (the 8 affected; installer configs touched in plan 25-02)
- `plugins/O-Lyrica/CMakeLists.txt`, `plugins/O-Bells/CMakeLists.txt`, `plugins/O-IntonationPad/CMakeLists.txt`, `plugins/O-Prism/CMakeLists.txt`, `plugins/O-Wind/CMakeLists.txt`, `plugins/O-Reed/CMakeLists.txt`, `plugins/O-Bowed/CMakeLists.txt`, `plugins/O-Formant/CMakeLists.txt`. Each consumes `note-expression` (verified by Phase 24 final sweep). Installer configs under each plugin's packaging entry point.

### Phase 24 Closeout (the immediate predecessor — read for context, not for re-decision)
- `.planning/phases/24-propagate/24-VERIFICATION.md` — confirms all 8 plugins ship with NE module, all PASS Dorico smoke. Phase 25 builds on this verified foundation.
- `.planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md` — aggregate Dorico-test matrix (24/24 individual gate-points PASS). Phase 25's bundling sweep validates the .doricoexpmap path WITHOUT regressing the playback validated here.

### Dorico Documentation (external — agents fetch live)
- Steinberg Dorico Expression Map XML schema reference (current Dorico 5/6 release docs). Researcher fetches at planning time; pin a version snapshot in DOCS-02.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- **`modules/tuning/note-expression/`** — fully extracted module with stable API. Phase 25 ADDS one new directory (`resources/`) and one new CMake `install()` rule in `module.cmake`. Module source is otherwise untouched.
- **`modules/cmake/OuariconModules.cmake`** — `ouaricon_add_module()` macro with per-format routing loop (D-22..D-29 from Phase 23). Likely the right place to wire implicit auto-install of module resources.
- **`.claude/skills/plugin-packaging/SKILL.md` + `.claude/skills/package/SKILL.md`** — macOS PKG installer workflow (signed, branded). Each plugin's packaging config gets a one-line addition for the .doricoexpmap.
- **`.claude/skills/build-installer/SKILL.md`** — Windows Inno Setup EXE workflow. Each plugin's `.iss` template (or equivalent) gets the .doricoexpmap source + install destination entries for both Ouaricon shared and Dorico scan paths.
- **`scripts/verify-au-link.sh`** — AU verify gate (Phase 23/24). Optional inheritance for plan 25-02's per-plugin smoke if desired (recommended: skip — already validated in Phase 24, no expression-map effect on AU link).
- **`research/` directory** — already contains 4 microtonality research files (`microtonality-{commercial-performance,comprehensive-database,implementation-juce,theory-formats}.md`). Plan 25-03's `research/microtonal-dorico-integration.md` is a NEW file in this same family.

### Established Patterns

- **Module owns the asset** — Phase 23 established this for source code, JUCE patch, and consumer README; Phase 25 extends to resources. No per-plugin copy of the .doricoexpmap in source.
- **One-liner consumer integration** — `ouaricon_add_module(<Plugin> note-expression)` already wires source compilation and patch-marker check. Phase 25 should make this also wire resource installation (no consumer-side awareness needed).
- **Atomic plan = atomic commit** — preserved.
- **Stop-on-first-failure with in-plan triage; promote structural failures to fix-plans** — preserved (D-12 from Phase 24 CONTEXT).
- **Per-platform dual-path install** — NEW pattern this phase. macOS shared + macOS Dorico scan; Windows shared + Windows Dorico scan. The pattern can later generalize to other Steinberg products (Cubase, Nuendo) if needed (FUT).
- **Cross-platform validation gate** — Phase 25 reverses Phase 23/24's macOS-only convention for the installer pipeline specifically. The plugins themselves remain validated only on macOS for non-installer behavior (FUT-01); only the .doricoexpmap install pipeline is gated on both platforms.

### Integration Points (per-platform install pipeline)

**Module side (one-time, plan 25-01):**
1. `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` — NEW file, hand-authored.
2. `modules/tuning/note-expression/module.cmake` — ADD `install()` rule for the resource (per-platform branches for shared + Dorico scan paths).
3. `modules/tuning/note-expression/module.yaml` — bump `version: 1.0.0` → `version: 1.1.0`; add changelog entry.
4. `modules/tuning/note-expression/README.md` — append section describing the .doricoexpmap, install locations, and consumer auto-inheritance behavior.
5. `modules/registry.yaml` — bump `note-expression.version` to 1.1.0; add changelog entry.

**Plugin side (per-plugin, plan 25-02 — atomic sweep across 8 plugins):**
- Each plugin's packaging config (under `plugin-packaging` / `build-installer` workflow input) updated to consume the module's staged resource. Likely a single template-level change in the packaging skills + per-plugin config additions.
- Optional: each plugin's STATUS.md gets a one-line note that the .doricoexpmap is bundled (not strictly required since the module owns the asset, but useful traceability).

**Notes side (one-time, plan 25-03):**
- `research/microtonal-dorico-integration.md` — NEW file, single combined doc with 4 H2 sections.

### Variation Points (per-platform installer specifics)

- **macOS PKG payload structure** — the .doricoexpmap is a payload file at the Ouaricon shared path AND at Dorico's scan path. Postinstall script may need to handle the Dorico scan path detection (Dorico version detection at install time).
- **Windows Inno Setup `[Files]` section** — the .doricoexpmap file source + multiple destinations (`{commonappdata}\Ouaricon\Expression Maps\` + `{userappdata}\Steinberg\Dorico [N]\Expression Maps\User\`). Dorico version detection via Inno Setup `Code` section or explicit per-version `[Files]` entries.
- **Dorico version detection** — both platforms need a way to handle the case where the targeted Dorico version directory doesn't exist (skip + log; rely on the README fallback for manual import via Library menu).

### Phase 25 Touch Points (the package surface)

**Module-side (additive — does not modify existing module API):**
- `modules/tuning/note-expression/resources/` — NEW directory.
- `modules/tuning/note-expression/module.cmake` — install() rule added.
- `modules/tuning/note-expression/module.yaml` — version bump.
- `modules/tuning/note-expression/README.md` — append.
- `modules/registry.yaml` — version bump entry.

**Plugin-side (per-plugin, plan 25-02):**
- 8 plugins' packaging configs — extended (specifics depend on the packaging skill's config layout).

**Skill-side (potentially):**
- `.claude/skills/plugin-packaging/SKILL.md` and `.claude/skills/build-installer/SKILL.md` — may need template-level updates to read module-staged resources. Planner determines whether template updates suffice or if per-plugin hand-edits are required.

**Notes-side (plan 25-03):**
- `research/microtonal-dorico-integration.md` — NEW.

</code_context>

<deferred>
## Deferred Ideas

- **Articulation switches in the canonical .doricoexpmap** — staccato, legato, dynamics. Out of scope for v1.5 (microtonality-only). Revisit when there's user demand or website-manual content needs articulation coverage.
- **Per-plugin .doricoexpmap variants** — e.g., MPE plugins (O-Reed, O-Bowed) potentially needing different pitch-bend handling. Defer until a real user-facing playback issue surfaces; the unified canonical map is correct for the milestone.
- **Automated Dorico smoke harness** — still manual per platform. Phase 24 deferred; Phase 25 inherits the deferral. Revisit if cross-platform smoke testing becomes a recurring chokepoint.
- **End-user-facing manual / quickstart on the sales website** — FUT-06. DOCS-05 explicitly forbids this milestone. Notes from plan 25-03 are the source material; website authoring happens in a future milestone.
- **Multi-Dorico-version installer logic** — installer auto-detects Dorico 5/6/N and writes to all present versions. Plan 25-01/25-02 may implement this if straightforward; otherwise pin to latest detected and document upgrade path in DOCS-03.
- **Other Steinberg DAW expression-map paths (Cubase, Nuendo)** — Cubase also reads expression maps. Out of scope for v1.5 (Dorico-only). Future generalization candidate.
- **Module-side `.doricoexpmap` validation/linting in CI** — programmatically validating the XML schema before packaging. Not in scope; manual smoke covers it.
- **Updating each plugin's CHANGELOG with "now ships canonical Dorico expression map"** — optional. Not required by INST/DOCS requirements; defer to planner discretion. If included, recommend a uniform one-line entry without a version bump (the bundling is a packaging change, not a binary change).

</deferred>

---

*Phase: 25-package-docs*
*Context gathered: 2026-04-26*
