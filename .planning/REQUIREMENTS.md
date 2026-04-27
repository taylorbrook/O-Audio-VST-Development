# Requirements: Plugin Freedom System — v1.5 Microtonal Shared Module & Suite Propagation

**Defined:** 2026-04-24
**Core Value:** Reliable collaborative workflow that produces professional-quality plugins — where agents execute quality work that doesn't require constant rework.

**Milestone goal:** Promote the validated VST3 Note Expression pattern (O-Lyrica spikes 001–003) into a shared Ouaricon module and propagate Dorico microtonal playback across all pitched plugins, with every per-plugin rollout tracked through the standard `/improve` workflow.

**Implementation bible:** auto-loaded `spike-findings-VST-development` skill (synthesized 2026-04-23 from spikes 001–003). Every pattern, code snippet, and landmine is already recorded there.

**Seed consumed:** `.planning/seeds/microtonal-shared-module.md` — trigger condition (O-Lyrica spike verified in Dorico) met.

## v1.5 Requirements

### Shared Module (MODULE)

- [x] **MOD-01
**: A new shared Ouaricon module exists for microtonal VST3 Note Expression support; name confirmed against `/module-list` before creation (candidate: `dsp/note-expression`).
- [x] **MOD-02
**: Module contains cleaned `TuningNoteExpressionController` class advertising `kTuningTypeID`.
- [x] **MOD-03
**: Module contains cleaned `VST3ClientExtensions` subclass with raw-event queue and `queryIEditController` dispatch to the NEC.
- [x] **MOD-04
**: Module provides a header-only voice helper (e.g. `applyPendingTuning(pendingSource, midiNote, currentFrequency)`) so each plugin's `startNote` can integrate NE tuning in one line, and applies the tuning to `currentFrequency` **before** the DSP model's `trigger(...)` call (zipper prevention).
- [x] **MOD-05**: Module `README.md` documents consumer integration, the required local JUCE patch, and the end-user Dorico expression-map setup procedure.
- [x] **MOD-06
**: All diagnostic spike code stripped from module source — no `OLyrica::detail::neTrace(...)` call sites, no `detail::neTrace` / `detail::iidToHex` helpers, no stray `#include <fstream>` in the header.
- [x] **MOD-07
**: Local JUCE patch committed as a named patch file in `scripts/` (or equivalent) with a re-apply procedure documented for JUCE-version bumps.
- [x] **MOD-08
**: Module registered in `OuariconModules.cmake` / the module registry with semver, discoverable via `/module-list` and `/module-info`.

### O-Lyrica Reference Integration (LYRICA)

- [x] **LYR-01**: O-Lyrica consumes the shared module via `/module-add [module-name]` — existing spike-embedded code replaced with module consumption, not duplicated.
- [x] **LYR-02**: O-Lyrica's NE tuning composes with its existing `TuningEngine` (no raw `pow()` multiplier bypass as in the spike) — alternate tunings still work alongside NE offsets.
- [x] **LYR-03**: O-Lyrica passes the Dorico quarter-sharp smoke test after refactor (pitch = 50¢ above C4 for quarter-sharp C4, no attack zipper, NE events correlated by `noteId`).
- [x] **LYR-04**: O-Lyrica version bumped with a CHANGELOG.md entry documenting shared-module adoption and microtonal NE support.

### Suite Propagation (PROP)

Each requirement corresponds to one `/improve [PluginName]` cycle — see TRACK requirements for workflow discipline.

- [x] **PROP-01**: O-Bells consumes the shared module and passes the Dorico quarter-sharp smoke test. (completed 2026-04-26 via Plan 24-01; Dorico 3-point gate 3/3 PASS)
- [ ] **PROP-02**: O-IntonationPad consumes the shared module and passes the Dorico quarter-sharp smoke test.
- [ ] **PROP-03**: O-Prism consumes the shared module and passes the Dorico quarter-sharp smoke test.
- [ ] **PROP-04**: O-Wind consumes the shared module and passes the Dorico quarter-sharp smoke test.
- [ ] **PROP-05**: O-Reed consumes the shared module and passes the Dorico quarter-sharp smoke test.
- [ ] **PROP-06**: O-Bowed consumes the shared module and passes the Dorico quarter-sharp smoke test.
- [ ] **PROP-07**: O-Formant consumes the shared module and passes the Dorico quarter-sharp smoke test.

### Workflow Tracking Discipline (TRACK)

This category exists because Phase B touches 7 production plugins — the suite-wide change must go through the same tracking rigor as any other production plugin improvement, not a hand-edit spree.

- [ ] **TRACK-01**: Every Phase B plugin rollout executed via the `/improve [PluginName]` workflow — no direct source edits that bypass plugin-level versioning, changelog, and state tracking.
- [ ] **TRACK-02**: Each improved plugin receives a version bump (patch or minor, per change impact) applied consistently in `CMakeLists.txt` / plugin metadata.
- [ ] **TRACK-03**: Each improved plugin's `CHANGELOG.md` gets an entry documenting "adds VST3 Note Expression microtonal support for Dorico" with the new version.
- [ ] **TRACK-04**: Each improved plugin's plugin-local `.planning/STATUS.md` and any relevant plugin-local state files updated to reflect the improvement.
- [ ] **TRACK-05**: Every affected plugin (all 8) rebuilt and reinstalled to system plugin folders per CLAUDE.md rules — AU cache cleared, old bundles removed, fresh VST3 + AU copied to `~/Library/Audio/Plug-Ins/`.

### Installer Packaging (INSTALL)

- [x] **INST-01**: A canonical pre-configured Dorico expression map file (`Ouaricon-VST3-NoteExpression.doricolib` — note: v3 ships `.doricolib` library bundle, not `.doricoexpmap`, since Dorico does not recognize the `.doricoexpmap` extension; full 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition with `microtonalPlaybackMethod = kVST3NoteExpression`) authored with Microtonality explicitly set to "VST3 Note Expression". (completed 2026-04-27 via Plan 25-01; byte-identical to verified Path B reference; Library Manager Import + quarter-sharp ~269 Hz canary PASS end-to-end)
- [x] **INST-02**: Canonical `.doricolib` stored at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` as the single source of truth. (completed 2026-04-27 via Plan 25-01)
- [x] **INST-03**: Each of the 8 affected plugins' installers bundle the `.doricoexpmap` file (PKG on macOS, EXE on Windows per existing `build-installer` / `package` workflows).
- [x] **INST-04**: Installed `.doricoexpmap` placed at a discoverable location, or the installer emits a README pointing users to the file's install path with a one-line import instruction for Dorico.

### Internal Technical Notes (DOCS)

These are developer-reference notes, **not** end-user manuals. They become the source-of-truth material used later during the sales-website manual/quickstart authoring pass.

- [x] **DOCS-01**: Technical note covering module architecture — NEC advertisement flow, raw-event queue semantics, voice-routing logic, composition with each plugin's `TuningEngine`.
- [x] **DOCS-02**: Technical note with the canonical Dorico expression-map setup procedure (step-by-step, host-version noted), suitable for direct translation into website manual/quickstart copy.
- [x] **DOCS-03**: Technical note on host-side behavior quirks — Dorico's neighbor-semitone + NE-delta representation (`quarter-sharp C4 = C#4 + -50¢`), NEC handshake being ignored by Dorico (kept for other hosts), sample-offset timing requirements.
- [x] **DOCS-04**: Technical note on troubleshooting signatures — what breakage looks like when end-users skip the expression-map setup (matches the UX trap observed in Spike 002), symptoms-vs-cause table.
- [x] **DOCS-05**: Internal notes stored under `research/` per CLAUDE.md convention as `research/microtonal-dorico-integration.md` (or per-topic sub-files); NOT published as end-user-facing docs this milestone.

## Future Requirements (v1.6+ / v2+)

Acknowledged and deferred — not in v1.5 roadmap.

### Platform Expansion
- **FUT-01**: Windows VST3 verification of the full microtonal pipeline (spike was macOS-only).

### Protocol Expansion
- **FUT-02**: Additional NE types beyond `kTuningTypeID` — custom IDs `[100000, 200000]` reserved for per-note timbre and vibrato expressions.
- **FUT-03**: MTS-ESP as an orthogonal microtonal path for Reaper, Bitwig, and other hosts that don't support VST3 NE.

### Host Robustness
- **FUT-04**: Cross-block `noteId → voice` map for hosts that emit mid-note NE (spike only validated same-block NE; Dorico works fine without this but other hosts may need it).

### Test Matrix
- **FUT-05**: Expanded test matrix — quarter-flat, ¾-sharp, ¾-flat, and chords with differing per-note inflections (spike only aurally/log-verified quarter-sharp).

### End-User Documentation
- **FUT-06**: End-user-facing manuals and quickstart guides authored on the sales website (Ouaricon Audio site), using DOCS-01..DOCS-05 internal notes as source material. Out of scope in this repo.

## Out of Scope

Explicitly excluded from v1.5. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| End-user-facing manuals / quickstart guides | Authored on sales website during website update, not in-repo this milestone (FUT-06) |
| Windows VST3 verification | Spike was macOS-only; Windows pipeline is a later pass (FUT-01) |
| NE types beyond `kTuningTypeID` | Per-note timbre / vibrato deferred (FUT-02) |
| MTS-ESP path | Orthogonal microtonal system; separate milestone (FUT-03) |
| Percussive / noise plugins (O-Texture, O-TextureForge, etc.) | No definite pitch — microtonal NE not meaningful |
| Expanded test matrix (quarter-flat etc.) | Quarter-sharp coverage is sufficient for extraction + propagation validation (FUT-05) |
| Cross-block `noteId → voice` tracking | Dorico emits NE in same block; other hosts out of scope this milestone (FUT-04) |
| Rewriting each plugin's existing `TuningEngine` | Module composes with existing engines; no rewrite this milestone |
| New plugins | Project charter: system improvement, not new plugin creation |

## Traceability

Populated by roadmapper during Phase A/B/C creation. Each requirement maps to exactly one phase.

| Requirement | Phase | Status |
|-------------|-------|--------|
| MOD-01 | Phase A (23) | Complete |
| MOD-02 | Phase A (23) | Complete |
| MOD-03 | Phase A (23) | Complete |
| MOD-04 | Phase A (23) | Complete |
| MOD-05 | Phase A (23) | Complete |
| MOD-06 | Phase A (23) | Complete |
| MOD-07 | Phase A (23) | Complete |
| MOD-08 | Phase A (23) | Complete |
| LYR-01 | Phase A (23) | Complete |
| LYR-02 | Phase A (23) | Complete |
| LYR-03 | Phase A (23) | Complete |
| LYR-04 | Phase A (23) | Complete |
| PROP-01 | Phase B (24) | Complete (Plan 24-01, 2026-04-26) |
| PROP-02 | Phase B (24) | Pending |
| PROP-03 | Phase B (24) | Pending |
| PROP-04 | Phase B (24) | Pending |
| PROP-05 | Phase B (24) | Pending |
| PROP-06 | Phase B (24) | Pending |
| PROP-07 | Phase B (24) | Pending |
| TRACK-01 | Phase B (24) | In progress (1/8 plugins; O-Bells via Plan 24-01) |
| TRACK-02 | Phase B (24) | In progress (1/8 plugins; O-Bells 4.0.0→4.1.0 via Plan 24-01) |
| TRACK-03 | Phase B (24) | In progress (1/8 plugins; O-Bells CHANGELOG 4.1.0 via Plan 24-01) |
| TRACK-04 | Phase B (24) | In progress (1/8 plugins; O-Bells STATUS.md via Plan 24-01) |
| TRACK-05 | Phase B (24) | In progress (1/8 plugins; O-Bells fresh install via Plan 24-01) |
| INST-01 | Phase C (25) | Complete (Plan 25-01 v3, 2026-04-27 — canonical .doricolib reauthored; canary PASS) |
| INST-02 | Phase C (25) | Complete (Plan 25-01 v3, 2026-04-27 — single source at modules/tuning/note-expression/resources/library/) |
| INST-03 | Phase C (25) | Complete (Plan 25-02 v3, 2026-04-27 — 8 cohort plugins' PKG + EXE installers bundle canonical 6,431 B .doricolib; D-08 cross-platform STRICT-PASS) |
| INST-04 | Phase C (25) | Complete (Plan 25-02 v3, 2026-04-27 — shared PKG postinstall echoes Library Manager Import hint; Inno Setup CurStepChanged Log() call mirrors; companion README shipped at Ouaricon/Microtonal Suite/) |
| DOCS-01 | Phase C (25) | Complete |
| DOCS-02 | Phase C (25) | Complete |
| DOCS-03 | Phase C (25) | Complete |
| DOCS-04 | Phase C (25) | Complete |
| DOCS-05 | Phase C (25) | Complete |

**Coverage:**
- v1.5 requirements: 33 total
- Mapped to phases: 33
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-24*
*Last updated: 2026-04-27 — INST-03, INST-04 satisfied via Plan 25-02 v3 (D-08 cross-platform STRICT-PASS for 8 cohort plugins × macOS + Windows)*
