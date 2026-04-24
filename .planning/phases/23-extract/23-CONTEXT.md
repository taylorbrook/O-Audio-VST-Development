# Phase 23: Extract — Context

**Gathered:** 2026-04-24
**Status:** Ready for planning

<domain>
## Phase Boundary

A new shared Ouaricon microtonal module (`modules/tuning/note-expression`) is extracted from the cleaned spike code (O-Lyrica spikes 001–003), registered in the module system, and proven end-to-end on O-Lyrica as the reference consumer. O-Lyrica consumes the module via `/module-add`, composes NE tuning with its existing `TuningEngine`, and passes the Dorico quarter-sharp smoke test. The local JUCE patch is promoted from spike-findings markdown hunks into a real `.patch` file with an idempotent apply script and a CMake-time verification check.

**In scope:** module creation, O-Lyrica refactor, JUCE patch tooling, O-Lyrica version bump + CHANGELOG.
**Out of scope** (owned by later phases): 7 other pitched plugins (Phase 24), `.doricoexpmap` bundling in installers (Phase 25), website-ready DOCS-01..05 (Phase 25), Windows verification (FUT-01), MTS-ESP (FUT-03).

</domain>

<decisions>
## Implementation Decisions

### Module Identity & Placement

- **D-01:** Module path = `modules/tuning/note-expression`. Lives in existing `tuning/` category alongside `scala-tuning-engine` — semantically accurate (NE is a per-note tuning mechanism) and avoids adding an unused `dsp/` category. Overrides the REQUIREMENTS.md candidate `dsp/note-expression` (which was provisional).
- **D-02:** Registry name = `note-expression`. Scoped by category path, matches existing module name conventions (`preset-manager`, `vu-meter`).
- **D-03:** Starting semver = `1.0.0`. Spike-validated + shipping on O-Lyrica this phase + adopted by 7 plugins in Phase 24 = stable public API from day one. Matches registry convention.

### Module API Surface

- **D-04:** Public namespace = `Ouaricon::NoteExpression`. Modern nested-namespace convention; leaves room for sibling `Ouaricon::*` modules.
- **D-05:** Public classes:
  - `Ouaricon::NoteExpression::Controller` — was `TuningNoteExpressionController` in the spike; advertises `kTuningTypeID` NE.
  - `Ouaricon::NoteExpression::VST3Extensions` — was `LyricaVST3Extensions` in the spike; subclass of `juce::VST3ClientExtensions`. **Owns** the 128-slot pending tuning table.
- **D-06:** Public type: `Ouaricon::NoteExpression::PendingTuningTable = std::array<std::atomic<double>, 128>`.
- **D-07:** Voice helper signature (header-only, MOD-04):
  ```cpp
  double Ouaricon::NoteExpression::applyPendingTuning (
      PendingTuningTable& table, int midiNoteNumber, double currentFrequency);
  ```
  Returns new frequency. `pow(2, semis/12)` is encapsulated inside the helper — voice code never calls `pow` directly. Consumes the pending slot via `exchange(0.0)` so retriggered notes don't inherit stale offsets.
- **D-08:** Drain + correlate helper:
  ```cpp
  void Ouaricon::NoteExpression::updatePendingFromEvents (
      const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
      PendingTuningTable& table);
  ```
  Implements the two-pass logic: (1) build `std::map<noteId, midi pitch>` from NoteOns; (2) for each `kTuningTypeID` NE, look up pitch by `noteId`, compute `semitones = 240.0 * (value - 0.5)`, store into `table[pitch]`. Moves this logic out of the plugin's `processBlock` — plugin just calls it after `drainBlockEvents`.
- **D-09:** Pending-table ownership lives on `VST3Extensions` inside the module, not on the plugin's `PluginProcessor`. Processor calls `m_extensions.drainBlockEvents(events)` then `NoteExpression::updatePendingFromEvents(events, m_extensions.getPendingTable())`. Voices get the pointer via `&m_extensions.getPendingTable()`. Phase 24 plugins don't need to re-declare the 128-slot table in their own processors.

### TuningEngine Composition (O-Lyrica-specific, pattern generalizes)

- **D-10:** Composition order: voice computes base frequency via `TuningEngine.getFrequency(midi)` **first**, then passes that frequency through `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)`. NE delta is always in 12-TET semitones (Dorico computes the offset against the EDO12 neighbor) — multiplicative compose is mathematically correct for any base tuning. Satisfies LYR-02 "no raw pow bypass": pow is inside the module helper, not in voice code.
- **D-11:** `note-expression` module has **no dependency** on `scala-tuning-engine`. Composition happens at the call site (in the voice), not inside either module. Preserves portability to plugins that don't use `scala-tuning-engine`.

### JUCE Patch Management

- **D-12:** Patch format = unified `.patch` file (generated once via `git diff` against a temp-init'd JUCE tree), compatible with `git apply` / `patch -p1`. Upgrades the spike's markdown-hunks approach.
- **D-13:** Patch location = `scripts/juce-patches/note-expression-juce-8.0.4.patch`. Matches MOD-07 spec ("named patch file in `scripts/`"). Filename encodes target JUCE version for disambiguation across future upgrades.
- **D-14:** Apply script = `scripts/apply-juce-patches.sh`. Idempotent: greps target JUCE tree for the `JUCE-NE-PATCH` marker first, skips application if already present. Fails with actionable error if JUCE path (`/Users/taylorbrook/JUCE` per CLAUDE.md memory) is missing.
- **D-15:** Verification = CMake-time marker check. On every `cmake configure`, a CMake snippet greps the JUCE tree for `JUCE-NE-PATCH` markers; fatal error if missing, with message pointing to `scripts/apply-juce-patches.sh`. Fails loud + fails fast — prevents the silent regression where a JUCE upgrade leaves plugins building but microtones quietly broken. Enforcement lives at the module level (in the module's CMake include) so it only gates plugins that actually consume `note-expression`.

### O-Lyrica Refactor Shape

- **D-16:** `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` is **deleted entirely**. No plugin-local shim. O-Lyrica sets the clean reference pattern for Phase 24.
- **D-17:** O-Lyrica consumes the module via `/module-add note-expression`. `PluginProcessor` instantiates `Ouaricon::NoteExpression::VST3Extensions m_extensions`; `getVST3ClientExtensions()` returns `&m_extensions`. Voices `#include` the module header and call `applyPendingTuning` after their `TuningEngine` frequency query.
- **D-18:** Diagnostic spike code stripped per MOD-06: zero `OLyrica::detail::neTrace(...)` call sites remain in O-Lyrica sources; `detail::neTrace` / `detail::iidToHex` helpers deleted; `#include <fstream>` removed from any O-Lyrica or module source. Grep-verify as acceptance.
- **D-19:** O-Lyrica version bump = **2.2.2 → 2.3.0** (minor). New user-visible feature (Dorico microtonal playback via VST3 Note Expression). CHANGELOG entry documents "adds VST3 Note Expression microtonal support for Dorico" + shared-module adoption.

### Claude's Discretion

- Module README structure (format, TOC depth) — follow `scala-tuning-engine`'s README pattern.
- Exact apply-script error message wording — Claude chooses so long as it names the script path and explains the recovery action.
- CMake marker-check implementation (shell `execute_process` vs CMake `file(READ)` + `string(FIND)`) — Claude picks the cleaner option.
- Test ordering within the O-Lyrica refactor — Claude sequences the safe-rebuild checkpoints.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Spike Findings (implementation bible)
- `.claude/skills/spike-findings-VST-development/SKILL.md` — auto-loaded findings index.
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — validated patterns 1–5, landmines 1–5, constraints. Primary reference.
- `.claude/skills/spike-findings-VST-development/sources/shared-code/juce-patch.md` — exact hunks for both JUCE files (source of truth for the generated `.patch`).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h` — pre-strip spike code (Controller + Extensions).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp` — drain + two-pass NE→pitch correlation logic (moves into `updatePendingFromEvents`).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp` — voice-side `exchange(0.0)` pattern (moves into `applyPendingTuning`).

### Milestone Scoping
- `.planning/REQUIREMENTS.md` §MODULE, §LYRICA — MOD-01..08 + LYR-01..04 binding requirements for this phase.
- `.planning/ROADMAP.md` §Phase 23 — goal statement + 5 success criteria.
- `.planning/seeds/microtonal-shared-module.md` — original seed; extraction rationale and candidate extractables table.
- `.planning/notes/dorico-microtonal-vst-research.md` — upstream research on Dorico's NE behavior.

### Current O-Lyrica Spike Sites (source of truth for refactor targets)
- `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` — deletion target.
- `plugins/O-Lyrica/Source/PluginProcessor.h` lines 22, 48, 127, 210 — `pendingTuningSemis` array + `queryPendingTuningSource()` accessor; moves to module.
- `plugins/O-Lyrica/Source/PluginProcessor.cpp` lines 506, 716–759 — drain + correlation site; becomes `updatePendingFromEvents` call.
- `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` lines 13, 87, 142–158 — voice-side `exchange` + neTrace call sites; voice code post-refactor is one `applyPendingTuning` call.
- `plugins/O-Lyrica/Source/HarpSynthVoice.h` line 128 — `pendingTuningSource` pointer; switches to pointing at module's table.

### Module System
- `modules/registry.yaml` — target registration entry (tuning category).
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` integration pattern.
- `modules/tuning/scala-tuning-engine/module.yaml` — reference shape for module metadata.
- `modules/tuning/scala-tuning-engine/README.md` — reference shape for consumer integration docs.

### JUCE Patch Targets (local fork)
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` line 64 — `Vst3RawEvent` struct + `onVst3RawEvent` virtual. Patch hunk 1/2.
- `/Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` line 3699 — raw event forwarding before `MidiEventList::toMidiBuffer`. Patch hunk 2/2.

### Plugin Conventions
- `CLAUDE.md` — Plugin Cache Clearing protocol (AU cache + re-install sequence) applies to post-refactor O-Lyrica build.
- `plugins/O-Lyrica/CHANGELOG.md` — style reference for the 2.3.0 entry.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`modules/cmake/OuariconModules.cmake`** — `ouaricon_add_module()` handles CMake sources, module.yaml discovery by category path, and JS copy. The new module plugs into this without extending the function.
- **`modules/tuning/scala-tuning-engine/`** — canonical shape for a tuning-category module (module.yaml, README.md, cpp/, snippets/, docs/). New `note-expression` mirrors this layout minus the JS (pure C++ / no UI).
- **Existing JUCE patch already applied** at `/Users/taylorbrook/JUCE/` — markers confirmed at 2 locations via `grep -rn "JUCE-NE-PATCH"`. Patch file is generated from these already-applied edits, not re-applied during this phase.
- **O-Lyrica `TuningEngine`** (`Source/DSP/TuningEngine.cpp`) — exposes per-note frequency lookup; composition point for D-10.

### Established Patterns
- **Module registry categories** live in `registry.yaml`; `tuning` category already present — no category addition needed (path decision D-01).
- **Semver + used_by tracking** — every registered module carries version and a `used_by:` list; Phase 23 adds O-Lyrica, Phase 24 appends 7 more.
- **Markdown hunks for JUCE patches** — current `spike-findings` convention. This phase upgrades to real `.patch` files (D-12) — sets a new convention going forward.
- **`CLAUDE.md` build-and-install protocol** — every rebuild clears AU cache + reinstalls fresh. LYR-03 smoke test depends on this discipline.

### Integration Points
- **`modules/registry.yaml` `modules:` list** — append new entry at end of `tuning` section.
- **O-Lyrica `CMakeLists.txt`** — `ouaricon_add_module(OLyrica note-expression)` line near the existing `include(OuariconModules.cmake)`.
- **O-Lyrica `PluginProcessor.h`** — swap 128-slot table declaration for `Ouaricon::NoteExpression::VST3Extensions m_extensions;`.
- **CMake marker check** — lives inside the module's CMake glue (`modules/tuning/note-expression/` or registered via `ouaricon_add_module` hook), so it only activates for plugins that consume the module.

</code_context>

<specifics>
## Specific Ideas

- **"Ouaricon::NoteExpression:: feels right."** The spike's `OLyrica::` prefix was plugin-specific; generic namespace is the natural extraction.
- **Reference-consumer pattern:** O-Lyrica is the template Phase 24 plugins copy. Keeping O-Lyrica free of any plugin-local NE code (no shim) means Phase 24 follow-up plugins have an unambiguous target shape.
- **Dorico quarter-sharp smoke test is the canonical acceptance gate:** pitch = +50¢ above C4, no attack zipper, NE events correlated by `noteId`. Every Phase 24 plugin will repeat this test.
- **JUCE-NE-PATCH marker is the load-bearing convention** — both hunks already carry it (spike 001), the `.patch` file preserves it, the CMake check greps for it. Do not rename this marker.

</specifics>

<deferred>
## Deferred Ideas

- **MOD-05 README vs Phase 25 DOCS overlap** — Phase 23 ships a functional module README (consumer integration + JUCE patch re-apply + basic end-user Dorico expression-map setup). Phase 25 produces the comprehensive, website-ready `research/microtonal-dorico-integration.md` (DOCS-01..05). Brief overlap is acceptable; the Phase 25 version supersedes for end-user authoring.
- **Automated regression test for NE** — currently manual Dorico smoke test only. Preserving a spike-era test harness is deferred; noted as a future-phase candidate if Phase 24 reveals recurring regression risk.
- **Windows VST3 verification** — FUT-01. Patch is in cross-platform wrapper code; should work unchanged. Out of scope this phase.
- **MTS-ESP as orthogonal microtonal path** — FUT-03. Different protocol, not suitable for Dorico's per-note deltas.
- **Cross-block `noteId → voice` map** — FUT-04. Dorico emits NE in same block; sufficient for this phase.
- **Custom NE types beyond `kTuningTypeID`** (per-note timbre, vibrato depth) — FUT-02. Reserved ID range `[100000, 200000]` noted in spike findings.

</deferred>

---

*Phase: 23-extract*
*Context gathered: 2026-04-24*
