# Phase 24: Propagate — Context

**Gathered:** 2026-04-25
**Status:** Ready for planning

<domain>
## Phase Boundary

The shared `note-expression` module (delivered Phase 23) is consumed by all 7 remaining pitched plugins — **O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant** — via the standard `/improve [PluginName]` workflow. Each rollout produces a minor version bump in `CMakeLists.txt`, a CHANGELOG entry naming "adds VST3 Note Expression microtonal support for Dorico", a `.planning/STATUS.md` update, a clean VST3+AU+Standalone build, a fresh system install per `CLAUDE.md`, an `auval` AU load check via `scripts/verify-au-link.sh`, and a Dorico quarter-sharp smoke test that confirms +50¢ above C4 with no attack zipper. After all 7 land, a phase-end sweep rebuilds + reinstalls all 8 affected plugins (the 7 above plus O-Lyrica from Phase 23) and audits the module registry's `used_by` list.

**In scope:** consumer-side adoption in 7 plugins, per-plugin version/CHANGELOG/STATUS bookkeeping, per-plugin Dorico smoke + AU verify gates, final all-8 rebuild+install sweep, module registry `used_by` updates.

**Out of scope (owned by later phases):**
- Canonical `.doricoexpmap` authoring + installer bundling — Phase 25 (DOCS/INST).
- Internal developer-reference notes under `research/` (DOCS-01..05) — Phase 25.
- Module API changes — locked at Phase 23 (D-04..D-09, D-23). No version bump on `note-expression` module unless a defect surfaces during propagation.
- Windows VST3 verification (FUT-01) — patch is cross-platform but not validated this milestone.
- MTS-ESP, MPE, pitch-bend fallback (FUT-02..04).

**Carrying forward from Phase 23 (locked, not re-discussed):**
- Module path `modules/tuning/note-expression`, public API surface `Ouaricon::NoteExpression::*`, `applyPendingTuning(table, midi, freq)` voice-side helper signature (D-04..D-09, D-23).
- Composition order: voice computes base frequency first, then pipes through `applyPendingTuning` — `pow` lives inside the helper (D-10). Generalizes from O-Lyrica's `TuningEngine` to any base-frequency source (raw 12-TET, wavetable osc, scale lookup).
- Per-format module-source convention (`cpp/<format>/`) auto-routes Steinberg-touching code into `${TARGET}_VST3` only — AU/Standalone link clean by construction, no `#if JucePlugin_Build_VST3` guards needed in consumer code (D-22..D-29).
- One-liner consumer integration: `ouaricon_add_module(<Plugin> note-expression)` — no plugin-side CMake plumbing (D-26/D-27/D-29).
- `JUCE-NE-PATCH` marker check in `module.cmake` fails configure if patch is missing — protects against silent JUCE-upgrade regression (D-15).
- AU verify gate: `scripts/verify-au-link.sh <Plugin>` runs `auval -v <type> <subtype> <manuf>` with codes parsed from the plugin's `CMakeLists.txt` (D-30/D-31).
- Per-plugin version bump = **minor** (matches O-Lyrica 2.2.2 → 2.3.0 in Phase 23, D-19).
- CHANGELOG line phrasing = "adds VST3 Note Expression microtonal support for Dorico" (TRACK-03 exact phrase).

**Carrying forward from `CLAUDE.md`:**
- AU cache clear + remove old bundles + fresh install of `.vst3` and `.component` after every plugin rebuild — mandatory before any Dorico test.
- Build targets: `ninja <Plugin>_VST3 <Plugin>_AU <Plugin>_Standalone`.
- After install: `auval -a | grep -i <pluginname>` confirms AU registration before Dorico opens.

</domain>

<decisions>
## Implementation Decisions

### Plan Structure & Granularity

- **D-01: 7 per-plugin plans + 1 final sweep plan = 8 plans total.**
  - `24-01-O-<Plugin1>-PLAN.md` through `24-07-O-<Plugin7>-PLAN.md` — one plan per plugin, atomic commit, full `/improve` cycle (sources + version bump + CHANGELOG + STATUS + build + Dorico smoke + `verify-au-link.sh`).
  - `24-08-final-sweep-PLAN.md` — closing plan: rebuild + freshly install all 8 affected plugins (7 above + O-Lyrica), confirm `verify-au-link.sh` PASS for all 8, audit `modules/registry.yaml` `used_by` list shows all 8, regenerate `/module-info note-expression`, aggregate Dorico-test results table.
  - Maps directly to TRACK-01..05 (per-plugin tracking) and Success Criterion #4 (all 8 freshly installed).
- **D-02: No phase-start prep plan.** Module is already extracted, registry exists, `OuariconModules.cmake` is finalized, `verify-au-link.sh` exists. First per-plugin plan picks up clean.
- **D-03: Each per-plugin plan's top-level execution task is `/improve [Plugin]`.** PLAN.md documents pre-conditions, the integration-point spec (file:line), expected post-conditions (version, CHANGELOG line, STATUS update, smoke result), and references `24-INTEGRATION-MATRIX.md`. The `/improve` skill does the work — backup + Phase 0.9 verification + version bump + CHANGELOG automation + build + regression. Honors TRACK-01 literally ("every Phase B plugin rollout executed via the `/improve [PluginName]` workflow"). Plan invokes `/improve` likely on its express/auto path because the spec is locked at phase level (planner finalizes the exact `/improve` invocation flags).

### Per-Plugin Spec Depth

- **D-04: Each per-plugin PLAN.md is file:line specific.** Names the exact voice file (e.g., `plugins/O-Bells/Source/BellVoice.cpp:NN`), the exact line where the base-frequency assignment occurs and where `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)` inserts, the `PluginProcessor.{h,cpp}` swap (declare `Ouaricon::NoteExpression::VST3Extensions m_extensions;`, return `&m_extensions` from `getVST3ClientExtensions()`, call `m_extensions.drainAndUpdate()` in `processBlock`), and the `CMakeLists.txt` location of the new `ouaricon_add_module(<Plugin> note-expression)` line. Mirrors Phase 23's HarpSynthVoice.cpp:142–158 specificity. Discovery work happens at planning time (one inspection pass), not surfaced as a /improve discovery step.
- **D-05: Phase artifact `.planning/phases/24-propagate/24-INTEGRATION-MATRIX.md`** is written during plan-phase. One row per plugin with columns: `Plugin | Voice file | Base-freq source class | Composition note | Voice-trigger entry point | PluginProcessor swap site | CHANGELOG file | STATUS file | Notes`. Each per-plugin plan references its row by anchor link. Single source of truth across the 7 plans; surfaces structural variations (TuningEngine vs raw frequency vs wavetable osc) before any `/improve` runs.
- **D-06: Planner inspects all 7 plugins** to confirm composition pattern. Confirmed already from quick scout: O-Bells (`Source/TuningEngine.h` + `BellVoice.{h,cpp}`) and O-IntonationPad (`Source/DSP/TuningEngine.h` + `DSP/WavetableVoice.{h,cpp}`) compose with TuningEngine like O-Lyrica. Need explicit inspection of O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant during planning to confirm whether they (a) carry a TuningEngine, (b) compute base frequency raw via 12-TET, or (c) use a different mechanism (e.g., O-Bowed has `BoreWaveguide.h` — physical-modeling base-freq derivation may differ).

### Smoke-Test Rigor

- **D-07: Per-plugin Dorico smoke-test acceptance gate (3-point):**
  1. Play C4 quarter-sharp from Dorico — pitch lands at **+50¢ above C4** (verifiable via tuner plugin in DAW or audible reference).
  2. **No attack zipper** — pitch is correct from the very first sample of the note (the `applyPendingTuning` `exchange` consumes the offset before voice trigger; this verifies the composition order is right per-plugin).
  3. **NE correlated by `noteId`** — start a polyphonic chord with mixed accidentals (e.g., quarter-sharp C4 + natural E4) and confirm only the C4 voice is detuned. Catches the landmine of correlating by pitch instead of `noteId` (Dorico represents quarter-sharp C as `pitch=C#4, NE=-50¢`).
  - One Dorico session per plugin; ~2 minutes of testing each. Faster than the full LYR-03 5-test battery (which is overkill once the pattern is proven on O-Lyrica) while still hitting all three Phase-23 landmines (Patterns 1–3 in `vst3-note-expression-dorico.md`).
- **D-08: Per-plugin build-side gate (before Dorico opens):**
  1. `ninja <Plugin>_VST3 <Plugin>_AU <Plugin>_Standalone` — all three link cleanly with no `Undefined symbols for architecture arm64` errors mentioning `Steinberg::*`. (Per-format convention should make this automatic, but the gate enforces it as a regression check.)
  2. Fresh install per CLAUDE.md (AU cache cleared; old `.vst3` + `.component` removed; new artefacts copied to system folders).
  3. `scripts/verify-au-link.sh <Plugin>` — auval validates AU loads. (Inherited from D-30/D-31, generalized to all 7 plugins.)
- **D-09: Regression testing leans on `/improve`'s standard pipeline.** No new test infrastructure for Phase 24. `/improve` Phase 5.5 runs whatever regression baseline each plugin already has. NE wires in additively — when no NE events arrive, `applyPendingTuning` returns the original frequency unchanged (`exchange(0.0)` → `pow(2, 0)` = 1.0), so existing 12-TET behavior in non-Dorico hosts is preserved by construction. No need for an explicit "no-NE regression" check.
- **D-10: Per-plugin Dorico smoke-test results recorded in each plan's SUMMARY.md.** Format matches Phase 23's `23-04-version-readme-dorico-smoketest-SUMMARY.md` — PASS/FAIL line + observed values. Then `24-08-final-sweep-SUMMARY.md` tabulates all 7 plugin results in one closeout row for the phase verify gate.

### Sequencing & Failure Handling

- **D-11: Easy-first ordering.** Plugins structurally closest to O-Lyrica's pattern go first (TuningEngine + simple voice composition: likely O-Bells then O-IntonationPad). Physical-model and formant plugins (O-Wind, O-Reed, O-Bowed, O-Formant) come last; surprises hit only when the pattern is fully proven and the team has rhythm. Final exact ordering is finalized during plan-phase using the `24-INTEGRATION-MATRIX.md` (D-05) — planner can promote a particularly clean candidate to slot 1 as the propagation canary.
- **D-12: Stop-on-first-failure, triage in same plan.** If plugin N's `/improve` cycle fails at any stage (build, install, auval, Dorico smoke), the plan halts and diagnoses inline. If the failure is plugin-local (e.g., a voice-file site picked the wrong line), fix in-place and resume that plan — atomic commit unchanged. If the failure is structural (e.g., a module API bug surfaces, a per-format-routing edge case), promote to a `24-NN-fix-PLAN.md` that closes before resuming the rollout — same playbook Phase 23 used when Plan 23-04 surfaced the AU-link defect and Plan 23-05 was created. Preserves Phase 23's plan-checker discipline; preserves atomic-plan-commit invariant.
- **D-13: `/improve` cycles are strictly serial.** Plan executor runs 24-01 through 24-08 in order, never in parallel. Reasons: (a) AU cache (`~/Library/Caches/AudioUnitCache/`) is OS-shared — parallel installs would race on cache invalidation; (b) `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/` are shared install paths; (c) Dorico smoke testing is inherently serial (one DAW instance, one ear); (d) `/improve` itself runs build+install+test as one transaction per plugin.
- **D-14: Module registry `used_by` updates per-plugin in `/improve` cycle.** Each `/improve` invokes `/module-add note-expression` for its plugin, which appends to `modules/registry.yaml`'s `used_by:` list and writes the registry change as part of the same atomic plugin commit. By plan 24-07 close, the list contains all 8 (O-Lyrica + 7 propagation targets). Plan 24-08 audits the list — does not write it.

### Claude's Discretion

- **Exact ordering of plugins 1–7 within the easy-first principle (D-11).** Planner chooses based on the integration matrix. Suggested heuristic: first the two confirmed TuningEngine-composing synths (O-Bells, O-IntonationPad in either order); then any other 12-TET voice-based synths; then the physical-models (O-Wind, O-Reed, O-Bowed); O-Formant likely last given its potentially distinct pitched/formant flow.
- **Exact `/improve` invocation flags per plan (D-03).** Planner picks express/auto path vs interactive. Likely express (`--auto` or equivalent) since the spec is fully locked at the phase level; interactive only if a plugin shows non-obvious integration during planning.
- **PLAN.md task breakdown shape.** Whether to model the `/improve` invocation as one task that delegates to the skill, or as the `/improve` internal phases broken into discrete plan tasks. Recommended: one task `Run /improve <Plugin>` with sub-bullets for the expected sub-phases — keeps plan executor in sync with how `/improve` reports completion.
- **Format of `24-INTEGRATION-MATRIX.md` (D-05).** Markdown table is the recommended baseline; YAML matrix acceptable if it cross-references nicely with other tooling.
- **Whether the canary plugin (slot 1, D-11) gets a heavier smoke battery than the minimum (D-07).** Default: same minimum 3-point gate as the other 6. Planner may upgrade slot 1 to the full LYR-03 5-test battery if the propagation pattern looks risky in the matrix (e.g., a plugin in slot 1 has a structurally distinct voice flow). Document the choice in the plan if upgraded.
- **Whether `24-08-final-sweep` re-runs Dorico smoke on all 8 plugins or only validates build/install state.** Recommended: re-run a quick C4 quarter-sharp on each of the 8 (since fresh installs sometimes regress), but defer to planner if calendar pressure. Default to running.
- **PLAN.md naming convention.** Recommend `24-NN-O-<Name>-PLAN.md` (matches `23-NN-<slug>-PLAN.md` style); planner can pick a tighter slug.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents (researcher, planner, executor) MUST read these before planning or implementing.**

### Phase Scoping
- `.planning/ROADMAP.md` §Phase 24 — goal, dependencies (Phase 23), 5 success criteria, requirements list.
- `.planning/REQUIREMENTS.md` §PROP-01..07, §TRACK-01..05 — binding requirements for this phase.
- `.planning/phases/23-extract/23-CONTEXT.md` — Phase 23 design decisions D-01..D-34. The full module API and per-format-source convention live here. Do not redesign; consume verbatim.
- `.planning/seeds/microtonal-shared-module.md` — original seed; rationale for propagation.

### Implementation Bible (auto-loaded skill)
- `.claude/skills/spike-findings-VST-development/SKILL.md` — findings index.
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — validated patterns 1–5, landmines 1–5, constraints. Patterns 1 (correlate by noteId), 2 (apply tuning before DSP trigger), 3 (240 semitones full-scale conversion) are the load-bearing rules per-plugin.
- `.claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp` — reference for the drain + correlate hand-off (now lives in module's `updatePendingFromEvents`; voice-side composition still mirrors this shape per plugin).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp` — reference for voice-side `exchange(0.0)` pattern (now wrapped in `applyPendingTuning`; consumer call shape preserved).

### Background Research
- `.planning/notes/dorico-microtonal-vst-research.md` — Dorico's 3 wire mechanisms, why VST3 Note Expression is the right target, expression-map setup quirk (Microtonality must be set to "VST3 Note Expression" — Auto picks pitch-bend for non-Steinberg VST3s).

### Module Surface (already extracted, consume only — do not modify)
- `modules/tuning/note-expression/cpp/NoteExpression.h` — public API: `PendingTuningTable`, `applyPendingTuning(table, midi, freq)` (voice-side, header-only), `VST3Extensions` class, free helper `updatePendingFromEvents`, forward-declared `Controller`.
- `modules/tuning/note-expression/cpp/NoteExpression.cpp` — SharedCode-bound TU. Steinberg-free. Ctor/dtor of `VST3Extensions`, `drainAndUpdate` body, dispatch slot.
- `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` — VST3-only TU. All `<pluginterfaces/...>` includes. `Controller` class body, `queryIEditController`, `updatePendingFromEvents`, real deleter, dispatch registration.
- `modules/tuning/note-expression/README.md` — consumer integration steps. The 7 plugins follow this verbatim.
- `modules/tuning/note-expression/module.yaml` — module metadata.
- `modules/tuning/note-expression/module.cmake` — JUCE-NE-PATCH marker check hook; fires per-consumer at configure time.

### Module System Plumbing
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` macro with the per-format routing loop (D-22..D-29 from Phase 23). Per-format `cpp/<format>/` sources auto-route into `${TARGET}_<FORMAT>`; `cpp/*.cpp` go to SharedCode. Consumer plugins call this; do not modify it.
- `modules/registry.yaml` — `note-expression` entry under `tuning` category. Each `/improve` cycle's `/module-add` writes into this file's `used_by:` list.

### JUCE Patch Discipline
- `scripts/juce-patches/note-expression-juce-8.0.4.patch` — committed patch file for the 2 JUCE files (`juce_VST3ClientExtensions.h:64`, `juce_audio_plugin_client_VST3.cpp:3699`). Should already be applied at `/Users/taylorbrook/JUCE/`.
- `scripts/apply-juce-patches.sh` — idempotent apply script. Greps for `JUCE-NE-PATCH` marker; skips if already present. Safe to run as a phase-start sanity check.
- The JUCE `module.cmake` marker check fails configure with an actionable error if the patch isn't present — automatic guard.

### AU Verify Gate (inherited from Phase 23)
- `scripts/verify-au-link.sh` — small shell helper. Takes plugin name, parses `PLUGIN_CODE` / `PLUGIN_MANUFACTURER_CODE` / `PLUGIN_AU_MAIN_TYPE` from `plugins/<Plugin>/CMakeLists.txt`, invokes `auval -v <type> <subtype> <manuf>`. **Each Phase 24 plan inherits this verbatim.**

### Build & Install Discipline
- `CLAUDE.md` — Plugin Cache Clearing protocol (kill `AudioComponentRegistrar`, clear `AudioUnitCache`, remove `~/Library/Audio/Plug-Ins/VST3/<Plugin>.vst3` + `~/Library/Audio/Plug-Ins/Components/<Plugin>.component`, copy fresh artefacts). **Mandatory after every plugin rebuild before any Dorico test.**

### Reference Consumer (the Phase 23 template)
- `plugins/O-Lyrica/CMakeLists.txt` — line `ouaricon_add_module(OLyrica note-expression)`. The 7 Phase 24 plugins copy this exact one-liner shape.
- `plugins/O-Lyrica/Source/PluginProcessor.h` — `Ouaricon::NoteExpression::VST3Extensions m_extensions;` member declaration.
- `plugins/O-Lyrica/Source/PluginProcessor.cpp` — `getVST3ClientExtensions()` returns `&m_extensions`; `m_extensions.drainAndUpdate(...)` call from `processBlock`.
- `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` — voice-side composition: `freq = TuningEngine.getFrequency(midi); freq = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midi, freq);` before DSP trigger. **The single most important reference site** for Phase 24 voice-side edits.
- `plugins/O-Lyrica/Source/HarpSynthVoice.h` — `pendingTuningSource` pointer member shape.
- `plugins/O-Lyrica/CHANGELOG.md` — entry style for the 2.3.0 release. **The 7 Phase 24 plugins replicate the same first-line phrasing** ("adds VST3 Note Expression microtonal support for Dorico"; TRACK-03).
- `plugins/O-Lyrica/.planning/STATUS.md` — STATUS.md update style for the 7 plugins.

### Per-Plugin Source Sites (planning-time inspection targets — populate `24-INTEGRATION-MATRIX.md` from these)
- `plugins/O-Bells/Source/PluginProcessor.{h,cpp}`, `Source/BellVoice.{h,cpp}`, `Source/TuningEngine.{h,cpp}`, `CMakeLists.txt`, `CHANGELOG.md`, `.planning/STATUS.md`.
- `plugins/O-IntonationPad/Source/PluginProcessor.{h,cpp}`, `Source/DSP/WavetableVoice.{h,cpp}`, `Source/DSP/TuningEngine.{h,cpp}`, `CMakeLists.txt`, `CHANGELOG.md`, `.planning/STATUS.md`.
- `plugins/O-Prism/Source/*` — planner identifies voice file + base-frequency source.
- `plugins/O-Wind/Source/*` — physical-model; planner identifies base-frequency source (likely a tube/bore parameter).
- `plugins/O-Reed/Source/ReedWindVoice.{h,cpp}`, `Source/DSP/BoreWaveguide.h`, etc. — physical-model; planner identifies base-frequency source.
- `plugins/O-Bowed/Source/*` (likely `BoreWaveguide.h` analog) — physical-model; planner identifies base-frequency source.
- `plugins/O-Formant/Source/*`, `Source/dsp/ConsonantEngine.h` — formant-pitched; planner confirms whether it's MIDI-driven pitched (it's listed as pitched in the roadmap).

### `/improve` Workflow (the execution mechanism)
- `.claude/skills/plugin-improve/SKILL.md` — **TRACK-01 mandates Phase 24 rollouts go through this skill.** Phase 0 specificity detection, Phase 0.6 implementation planning, Phase 0.9 backup verification gate, Phase 4 CHANGELOG automation, Phase 5 build/install (delegates to `build-automation`), Phase 5.5 regression testing.

### Phase 23 Closeout (the immediate predecessor — read for context, not for re-decision)
- `.planning/phases/23-extract/23-VERIFICATION.md` — Phase 23 verification gate.
- `.planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md` — LYR-03 5-test results format, AU-link defect surfacing, exit criteria pattern.
- `.planning/phases/23-extract/23-05-fix-au-link-steinberg-symbols-PLAN.md` — Plan 23-05 establishing the per-format convention and `verify-au-link.sh` gate.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- **`modules/tuning/note-expression/`** — fully extracted module with stable 1.0.0 API. The 7 Phase 24 plugins consume it via one-liner; no module-side edits anticipated.
- **`scripts/verify-au-link.sh`** — auval gate, parameterized by plugin name. Inherited verbatim from Phase 23 to every Phase 24 plan.
- **`scripts/apply-juce-patches.sh`** — idempotent JUCE-patch ensure script. Phase-start sanity check (or trust the CMake-time marker check fails configure if missing).
- **`/improve [PluginName]` skill** — full plugin-improvement workflow with backup, version bump, CHANGELOG automation, build/install, regression. Each Phase 24 per-plugin plan invokes this as its top-level execution task.
- **`/module-add note-expression` (within /improve)** — appends consumer to `modules/registry.yaml` `used_by:` list automatically.
- **O-Lyrica reference shape** (`PluginProcessor.{h,cpp}`, `HarpSynthVoice.{h,cpp}`) — the textbook for what each of the 7 plugins looks like post-`/improve`.

### Established Patterns

- **Per-format module-source routing** — `cpp/vst3/` lands in `${TARGET}_VST3` only; `cpp/*.cpp` lands in SharedCode. Auto-applied via `OuariconModules.cmake`. Phase 24 inherits this with zero consumer-side awareness.
- **Atomic plan = atomic commit** — Phase 23's plan-checker discipline. Each Phase 24 per-plugin plan ships as one commit; rollback granularity is per-plugin.
- **Minor version bump for new user-visible feature** — O-Lyrica 2.2.2 → 2.3.0 is the precedent. Each of the 7 plugins gets a minor bump.
- **Exact CHANGELOG phrasing** — "adds VST3 Note Expression microtonal support for Dorico" appears in O-Lyrica 2.3.0; TRACK-03 mandates it for all 7.
- **AU cache + reinstall after every build** — CLAUDE.md protocol. Non-negotiable per plugin.
- **Composition order** (D-10 generalized) — base-frequency lookup first, then `applyPendingTuning`, then DSP trigger. Same shape regardless of base-frequency source (TuningEngine, raw 12-TET, wavetable osc).

### Integration Points (per-plugin pattern)

For each of the 7 plugins, the per-`/improve` edit shape is:

1. **`plugins/<Plugin>/CMakeLists.txt`** — add `ouaricon_add_module(<Plugin> note-expression)` near the existing `OuariconModules.cmake` include + bump `PLUGIN_VERSION` (minor).
2. **`plugins/<Plugin>/Source/PluginProcessor.h`** — declare `Ouaricon::NoteExpression::VST3Extensions m_extensions;` member.
3. **`plugins/<Plugin>/Source/PluginProcessor.cpp`** — `getVST3ClientExtensions()` returns `&m_extensions`; in `processBlock`, after `m_extensions.drainBlockEvents(events)` call `m_extensions.drainAndUpdate()` (which internally dispatches to `updatePendingFromEvents` in the VST3 TU).
4. **`plugins/<Plugin>/Source/<Voice>.{h,cpp}`** — voice gets a `Ouaricon::NoteExpression::PendingTuningTable*` member pointing to `m_extensions.getPendingTable()`; voice-side composition: `double freq = <baseFreqLookup>(midi); freq = Ouaricon::NoteExpression::applyPendingTuning(*table, midi, freq);` before DSP trigger.
5. **`plugins/<Plugin>/CHANGELOG.md`** — new entry: version + "adds VST3 Note Expression microtonal support for Dorico" + composition note (e.g., "composes with TuningEngine" or "applied to oscillator base frequency").
6. **`plugins/<Plugin>/.planning/STATUS.md`** — entry recording the version bump and microtonal integration.
7. **`modules/registry.yaml`** — `/module-add` appends `<Plugin>` to `note-expression` `used_by:` list.

### Variation Points (planner identifies via `24-INTEGRATION-MATRIX.md`)

- **TuningEngine-composing plugins** — base-frequency lookup goes through a `TuningEngine.getFrequency(midi)` (or analog). Composition order matches O-Lyrica exactly.
- **Raw 12-TET plugins** — base-frequency = `440 * pow(2, (midi - 69) / 12)` directly (or equivalent). `applyPendingTuning` slots in immediately after the 12-TET formula.
- **Wavetable / oscillator-driven plugins** — base-frequency may be passed to an oscillator's `setFrequency()`; `applyPendingTuning` modifies the value before that call.
- **Physical-model plugins (O-Wind, O-Reed, O-Bowed)** — base-frequency drives a delay-line or waveguide period; `applyPendingTuning` modifies the value before the period calc. Care: if the physical model has a slow tuning slew, attack-zipper protection (Pattern 2) may need additional thought; mitigation is the same `exchange` consume.
- **Formant plugin (O-Formant)** — `Source/dsp/ConsonantEngine.h` is the formant generator; the pitched fundamental drives it. Planner confirms the fundamental-frequency assignment site.

### Phase 24 Touch Points (the propagation surface)

**Module-side (zero changes):**
- `modules/tuning/note-expression/*` — read-only consumption only. Module 1.0.0 stays at 1.0.0 unless a defect surfaces.

**Plugin-side (per-plugin, executed via `/improve`):**
- `plugins/O-Bells`, `plugins/O-IntonationPad`, `plugins/O-Prism`, `plugins/O-Wind`, `plugins/O-Reed`, `plugins/O-Bowed`, `plugins/O-Formant` — each receives the 7-touch edit shape above.

**Build/install-side (per-plugin, executed via `/improve` → `build-automation`):**
- `~/Library/Caches/AudioUnitCache/` — cleared per plugin.
- `~/Library/Audio/Plug-Ins/VST3/<Plugin>.vst3` — replaced fresh.
- `~/Library/Audio/Plug-Ins/Components/<Plugin>.component` — replaced fresh.

**Phase-end sweep (24-08):**
- All 8 affected plugins (7 + O-Lyrica) freshly rebuilt + reinstalled.
- `modules/registry.yaml` `note-expression.used_by:` audited (should list all 8).
- `/module-info note-expression` regenerated.
- Aggregate Dorico-test results table written to `24-08-final-sweep-SUMMARY.md`.

</code_context>

<specifics>
## Specific Ideas

- **"O-Lyrica is the textbook; the 7 plugins are translations."** Phase 23 deliberately left O-Lyrica as the cleanest possible reference (no plugin-local NE shim, all NE plumbing in the module). Each Phase 24 plan should be readable as "translate O-Lyrica's pattern to this plugin's voice file." Cross-references to O-Lyrica file:line in each plan reduce planning load.
- **"`/improve` is the execution discipline, not just bookkeeping."** TRACK-01 isn't just a paperwork requirement — `/improve` enforces backup verification (Phase 0.9) and atomic CHANGELOG automation. Bypassing `/improve` for "just one plugin" would re-introduce the kind of ad-hoc divergence v1.4 explicitly worked to eliminate. Per-plan execution must invoke `/improve` literally.
- **"The integration matrix is the planner's deliverable."** Each per-plugin plan can be terse (5–10 tasks) precisely because `24-INTEGRATION-MATRIX.md` carries the structural information. The matrix is what makes Phase 24 fast — it converts 7 discovery problems into 7 lookup problems.
- **"Easy-first builds momentum and surfaces structural issues incrementally."** The first one or two TuningEngine-composing plugins should be near-mechanical. If they aren't, that's a signal to pause and re-examine the matrix before propagating to the harder cases.
- **"Quarter-sharp at C4 + chord polyphony test catches all three landmines."** The 3-point smoke gate isn't arbitrary — each point validates one of Patterns 1, 2, 3 from the spike findings: pitch landing (Pattern 3 = 240 semitones full-scale), no zipper (Pattern 2 = apply-before-trigger), polyphonic correlation (Pattern 1 = noteId not pitch). Lighter than LYR-03 5-test but covers the same failure modes.
- **"AU has to load, not just link."** Per Phase 23 specifics — `verify-au-link.sh` is a load test, not just a link test. Inherited verbatim.
- **"Stop on failure, fix in same plan, preserve atomic commits."** Phase 23 demonstrated this with Plan 23-05. Phase 24 inherits the same playbook — if the wheels come off, the first-failure plugin's plan absorbs the fix.

</specifics>

<deferred>
## Deferred Ideas

- **Automated Dorico smoke harness** — currently manual per plugin (open Dorico, play C4 quarter-sharp, observe). A scripted harness that drives Dorico via expression-map fixtures and meters output pitch would let the whole 7-plugin sweep run unattended. Out of scope for Phase 24; revisit if Phase 24 reveals recurring smoke-test churn.
- **Preset-format compatibility audit** — the per-plugin presets don't carry NE state (NE is wire-level, transient), so propagation should be preset-neutral. Still worth a one-line check per plugin, but no audit deliverable.
- **Module README fork-or-extend per plugin** — each plugin's docs may want to say "this plugin supports Dorico microtonal playback when bundled with the canonical .doricoexpmap." That's Phase 25 (DOCS-01..05) territory; Phase 24 just lands the technical capability.
- **Windows VST3 verification of the propagated 7** — FUT-01. Same as Phase 23 — patch is cross-platform, but no Windows DAW host smoke this milestone.
- **Per-plugin custom NE types beyond `kTuningTypeID`** (per-note timbre, vibrato depth) — FUT-02. Not in propagation scope.
- **Performance audit of the 128-slot atomic table per plugin** — negligible (atomic-double load + multiply per voice trigger), but worth noting if any plugin shows a measurable change. Defer unless a regression surfaces during smoke.
- **Cross-plugin coordination of /module-info schema** — `/module-info note-expression` after Phase 24 should show 8 consumers; if the schema doesn't natively support this scale, Phase 25 may extend it.

</deferred>

---

*Phase: 24-propagate*
*Context gathered: 2026-04-25*
