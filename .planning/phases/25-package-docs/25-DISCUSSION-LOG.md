# Phase 25: Package & Internal Technical Notes - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-26
**Phase:** 25-package-docs
**Areas discussed:** Expression map authoring, Installer bundling pattern, End-user install path + README, Plan structure & notes file layout

---

## Area 1: Expression Map Authoring

### Author source

| Option | Description | Selected |
|--------|-------------|----------|
| Author from scratch | Hand-write the XML/JSON .doricoexpmap referencing the spike-findings VST3 Note Expression patterns. Most control, no provenance baggage. Slower but produces a clean, documented artifact. | ✓ |
| Adapt existing Dorico project | Use the working Dorico project from Phase 23 spike testing as the seed and clean it up. Fastest — file already battle-tested. | |
| Adapt vendor template | Start from a third-party VST3 plugin's published .doricoexpmap that already uses 'VST3 Note Expression' microtonality. Risk: license/attribution; benefit: more complete articulation coverage. | |

**User's choice:** Author from scratch
**Notes:** Owning provenance and field-set composition matters more than saving authoring time; file is small enough to hand-author cleanly.

### Map scope

| Option | Description | Selected |
|--------|-------------|----------|
| Microtonality-only (minimal) | Microtonality field set to 'VST3 Note Expression' — nothing else. Smallest, most focused file. Articulation behavior left to Dorico defaults. Matches v1.5 milestone scope. | ✓ |
| Microtonality + basic articulations | Adds standard articulation switches (natural, staccato, legato, accent). More useful out of the box but expands scope and maintenance. | |
| Per-plugin variants | One canonical map for the suite, but with per-plugin overrides (e.g., MPE plugins). Splits the 'single source of truth' invariant. | |

**User's choice:** Microtonality-only (minimal)
**Notes:** v1.5 boundary is microtonality. Articulations are a separate concern; conflating them balloons authoring + maintenance.

---

## Area 2: Installer Bundling Pattern

### Bundling source

| Option | Description | Selected |
|--------|-------------|----------|
| CMake install() rule in module | Add an install() rule in modules/tuning/note-expression/module.cmake that any consuming plugin inherits via ouaricon_add_module(). Module owns its asset, propagates automatically. | ✓ |
| Build-time copy per plugin | Each plugin's CMakeLists.txt copies the .doricoexpmap from the module into its build output. More explicit but introduces 8 copy points to maintain. | |
| Symlink in each plugin source | Each plugin gets a symlink to the canonical file. Risky on Windows (symlink support varies). | |

**User's choice:** CMake install() rule in module
**Notes:** Aligns with Phase 23's "module owns everything" invariant. Naturally extends per-format routing to per-platform install routing.

### Artifact layout

| Option | Description | Selected |
|--------|-------------|----------|
| Inside .vst3/.component bundle Resources/ | Embed the file inside each plugin's bundle. Travels with the binary; downside is invisibility to manual import. | |
| Sibling file at install location | Install .doricoexpmap as a sibling to the .vst3. Visible, easy to import manually. Less tidy — 8 copies scattered. | |
| Single shared location (one file total) | Installer drops ONE .doricoexpmap at a shared Ouaricon resources path regardless of which plugin installer ran. All 8 installers write the same file. De-duplicated, requires idempotent write. | ✓ |

**User's choice:** "Whatever you suggest that is most efficient and customizable" → Claude recommended **Single shared location** (`~/Library/Application Support/Ouaricon/Expression Maps/` on macOS; `%APPDATA%\Ouaricon\Expression Maps\` on Windows). Idempotent overwrite, user-editable, no duplication, no bundle-internal hiding.
**Notes:** User delegated; Claude locked the recommendation in CONTEXT.md as D-05.

---

## Area 3: End-User Install Path + README

### Auto-scan placement

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, dual-write to both locations | Installer writes to both Ouaricon shared resources path AND Dorico's user expression-maps scan path. Best UX — auto-discovered in Dorico's picker. Requires Dorico version path handling. | ✓ |
| No, neutral location + README only | Installer drops .doricoexpmap at the shared Ouaricon path with a README explaining manual import via Dorico's Library menu. | |
| README only, no shared resources file | Installer just emits a README pointing to where to extract the file. Minimal footprint; worst UX. | |

**User's choice:** Yes, dual-write to both locations
**Notes:** Map needs to be both editable (shared path) and auto-discovered (Dorico scan path). Claude added README emission as a fallback (D-08) for Dorico-version-mismatch cases.

### Validation gate platforms

| Option | Description | Selected |
|--------|-------------|----------|
| macOS only (defer Windows to FUT-01) | Run the dry-run install + Dorico smoke test on macOS only. Inherits Phase 23/24's deferral. | |
| Both macOS and Windows | Validate on both platforms. Expands scope; requires Windows access + Dorico installation. | ✓ |
| macOS for Dorico smoke + both for installer build | Build PKG and EXE on both platforms but only run actual Dorico playback test on macOS. | |

**User's choice:** Both macOS and Windows
**Notes:** Reverses Phase 23/24's FUT-01 deferral for the .doricoexpmap install pipeline specifically. Plugin behavior outside the install pipeline remains FUT-01-deferred. Planner must surface Windows access blocker as hard halt if it materializes.

---

## Area 4: Plan Structure & Notes Layout

### Plan shape

| Option | Description | Selected |
|--------|-------------|----------|
| Grouped sweep (3 plans) | 25-01 author + plumbing; 25-02 per-plugin installer-bundling sweep + cross-platform validation matrix; 25-03 internal notes. Mirrors Phase 24's 24-08 sweep model. | ✓ |
| Per-plugin (8 + 2 = 10 plans) | Plan per plugin installer plus authoring + notes plans. Maximum atomicity but per-plugin work is mostly identical here. | |
| Two plans (artifact + sweep) | One plan for everything package-related; one plan for notes. Most compact but lower atomicity. | |

**User's choice:** Grouped sweep (3 plans)
**Notes:** The per-plugin installer-config edit is mechanical enough that the sweep model earns its weight. Atomicity preserved at the plan level; rollback granularity is per-major-step, not per-plugin.

### Notes layout

| Option | Description | Selected |
|--------|-------------|----------|
| Per-topic sub-files (4 files) | Each DOCS topic in its own file. Cleaner translation to website chapters. Easier to update one without touching others. | |
| Single combined file (1 file) | research/microtonal-dorico-integration.md with 4 H2 sections. One file, easier to skim. | ✓ |
| Single file + separate setup-procedure quickstart draft | Combines architecture + host-quirks + troubleshooting; setup procedure is a separate website-ready quickstart draft. | |

**User's choice:** Single combined file
**Notes:** Single file matches the file-naming hint in the roadmap success criteria text and keeps the developer-reference scope tight.

---

## Claude's Discretion

- Exact ordering of plugins inside the bundling sweep (recommended canary: O-Lyrica, then other 7 in any order).
- CMake mechanism for per-platform dual-write (recommended: module-side install() rule).
- Whether to add an explicit `ouaricon_install_dorico_expression_map()` CMake function vs implicit auto-install on `ouaricon_add_module()` (recommended: implicit).
- Exact Dorico version targeted by the installer (recommended: latest installed on user's machine; documented in DOCS-02).
- Whether to bundle the README inside the .vst3 too (recommended: NO — single shared location only).
- PLAN.md naming convention (recommended: `25-NN-<slug>-PLAN.md`).
- Artifact layout (single shared location at `~/Library/Application Support/Ouaricon/Expression Maps/` on macOS; `%APPDATA%\Ouaricon\Expression Maps\` on Windows) — explicitly delegated to Claude by user.

## Deferred Ideas

- Articulation switches in the canonical .doricoexpmap (staccato, legato, dynamics).
- Per-plugin .doricoexpmap variants (especially MPE plugins).
- Automated Dorico smoke harness (still manual; inherited deferral from Phase 24).
- End-user-facing manual / quickstart on the sales website (FUT-06).
- Multi-Dorico-version installer logic (auto-detect 5/6/N, write to all present).
- Other Steinberg DAW expression-map paths (Cubase, Nuendo).
- Module-side `.doricoexpmap` validation/linting in CI.
- Per-plugin CHANGELOG entries for the bundling change (no binary change → optional).
