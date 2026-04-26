---
phase: 25-package-docs
plan: 03
type: execute
wave: 3
depends_on: [25-01, 25-02]
files_modified:
  - research/microtonal-dorico-integration.md
autonomous: true
requirements: [DOCS-01, DOCS-02, DOCS-03, DOCS-04, DOCS-05]
must_haves:
  truths:
    - "research/microtonal-dorico-integration.md exists as a new single combined file under research/ per CLAUDE.md convention (DOCS-05)"
    - "The file has exactly 4 H2 sections covering all 4 DOCS topics, in this order: '## Module Architecture' (DOCS-01), '## Canonical Dorico Setup Procedure' (DOCS-02), '## Host-Side Behavior Quirks' (DOCS-03), '## Troubleshooting Signatures' (DOCS-04)"
    - "Tone is developer-facing technical reference, NOT end-user marketing copy — DOCS-05 boundary explicitly honored"
    - "Notes describe what was actually shipped in Plans 25-01 and 25-02 (the canonical .doricoexpmap, the module-side install rules, the per-platform PKG/EXE dual-write logic, the install-time Dorico version probe) — they are written AFTER plans 25-01/02 land so they describe shipped behavior accurately, not anticipated behavior"
    - "DOCS-02 is structured for direct translation into future website manual/quickstart copy (FUT-06) — uses Dorico's exact menu paths verbatim, screenshots/numbered steps, host-version pinned"
    - "DOCS-04 is symptoms-vs-cause table format covering the expression-map-skipped UX trap (Spike 002 reference) plus regression signatures from Phases 23/24 (AU-link Steinberg-symbol leak, per-plugin propagation patterns)"
  artifacts:
    - path: "research/microtonal-dorico-integration.md"
      provides: "Single combined developer-reference document with 4 H2 sections covering DOCS-01..04. Source material for FUT-06 website authoring."
      min_lines: 250
      contains: "## Module Architecture"
  key_links:
    - from: "research/microtonal-dorico-integration.md ## Module Architecture"
      to: "modules/tuning/note-expression/cpp/NoteExpression.h + cpp/vst3/NoteExpression_VST3.cpp"
      via: "code-reference cross-links to the actual module API surface"
      pattern: "modules/tuning/note-expression"
    - from: "research/microtonal-dorico-integration.md ## Canonical Dorico Setup Procedure"
      to: "modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap"
      via: "documents the canonical asset's install paths and Dorico's import workflow"
      pattern: "Ouaricon-VST3-NoteExpression.doricoexpmap"
    - from: "research/microtonal-dorico-integration.md ## Host-Side Behavior Quirks"
      to: "spike-findings Patterns 1-5 and Landmines 1-5"
      via: "summary references with cross-links into spike-findings skill"
      pattern: "noteId"
    - from: "research/microtonal-dorico-integration.md ## Troubleshooting Signatures"
      to: "spike-findings Landmine 3 + Phase 23 D-23-04-A AU-link defect"
      via: "symptoms-vs-cause table"
      pattern: "Symptom"
---

<objective>
Author `research/microtonal-dorico-integration.md` as a single combined developer-reference document with 4 H2 sections covering DOCS-01..04. Tone is technical reference (not end-user marketing — DOCS-05 boundary). The document captures the shipped state of the v1.5 microtonal cohort: the module architecture (DOCS-01), the canonical Dorico setup procedure derived from what Plans 25-01/02 made automatic (DOCS-02), the Dorico/VST3 host-side quirks the implementation had to navigate (DOCS-03), and the symptoms-vs-cause troubleshooting table for the expression-map-skipped UX trap and known regression signatures (DOCS-04).

Purpose: Close DOCS-01..05 with a single source-material document that future website authoring (FUT-06) translates into end-user-facing manual/quickstart copy. The notes describe shipped behavior accurately because Plan 25-03 runs AFTER Plans 25-01 and 25-02 land.

Output: One new file at `research/microtonal-dorico-integration.md`, ~250-400 lines, 4 H2 sections, fully cross-referenced to source files and spike findings.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@.planning/ROADMAP.md
@.planning/REQUIREMENTS.md
@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-01-author-and-plumbing-PLAN.md
@.planning/phases/25-package-docs/25-02-installer-bundling-sweep-PLAN.md
@.planning/phases/24-propagate/24-CONTEXT.md
@.planning/phases/24-propagate/24-08-final-sweep-PLAN.md
@.planning/phases/23-extract/23-CONTEXT.md
@.planning/notes/dorico-microtonal-vst-research.md
@.claude/skills/spike-findings-VST-development/SKILL.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp
@modules/tuning/note-expression/cpp/NoteExpression.cpp
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/module.cmake
@modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap
@research/microtonality-implementation-juce.md
@research/microtonality-theory-formats.md
@research/microtonality-comprehensive-database.md
</context>

<interfaces>
<!-- Key contracts the executor needs. Use these directly. -->

The 4 H2 section titles are FIXED by D-13 (CONTEXT.md) — use these verbatim, in this order:

1. `## Module Architecture` (DOCS-01)
2. `## Canonical Dorico Setup Procedure` (DOCS-02)
3. `## Host-Side Behavior Quirks` (DOCS-03)
4. `## Troubleshooting Signatures` (DOCS-04)

The 5 propagation patterns catalogued in Phase 24 (referenced in DOCS-01 per D-13):
1. classic-Synthesiser-multi-osc (O-Prism)
2. classic-Synthesiser-physical-period (O-Bells, O-Wind)
3. classic-Synthesiser-multi-sub-voice (O-IntonationPad — multiplicative neRatio)
4. MPE-helper-based (O-Reed 3 sites, O-Bowed 2 sites)
5. MPE-per-call-site (O-Formant)

Spike-findings invariants the document MUST cite (from `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md`):
- Pattern 1: JUCE patch uses Approach 2 (side-channel queue), `JUCE-NE-PATCH` marker
- Pattern 2: Plugin advertises NEC via `VST3ClientExtensions::queryIEditController` (kept for Cubase/Nuendo even though Dorico ignores it — Landmine 1)
- Pattern 3: Processor drains queue at top of `processBlock` before `renderNextBlock`
- Pattern 4: Voice consumes tuning with `exchange(0.0)` BEFORE DSP trigger (Landmine 4)
- Pattern 5: Measurement math: `plain_semitones = 240.0 * (normalized - 0.5)`
- Landmine 1: Dorico does NOT use the NEC handshake — sends NE based solely on expression-map Microtonality setting
- Landmine 2: Dorico represents quarter-sharp C4 as `pitch=C#4, NE=-50¢` (NEVER pitch=60, NE=+50¢) — correlate by noteId, never pitch
- Landmine 3: Default expression maps route microtones to VST2 detune / pitch bend; users MUST select Microtonality = "VST3 Note Expression" — THE invariant the canonical .doricoexpmap encodes
- Landmine 4: Voice-side drain MUST happen BEFORE DSP trigger (attack zipper signature)
- Landmine 5: Spike diagnostic logging stripped before module extraction

Phase 23 architectural defect to reference in DOCS-04 (D-23-04-A from Phase 23 CONTEXT):
- `JucePlugin_Build_VST3` guards in headers leak Steinberg symbols into SharedCode → AU/Standalone link fails with `Undefined symbols: Steinberg::Vst::INoteExpressionController::iid, …`
- Resolution: per-format module-source convention (`cpp/<format>/`) auto-routes Steinberg-touching code into `${TARGET}_VST3` only

Module API surface (from `modules/tuning/note-expression/cpp/NoteExpression.h` — referenced by DOCS-01):
- `Ouaricon::NoteExpression::PendingTuningTable` = `std::array<std::atomic<double>, 128>`
- `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq) → double`
- `Ouaricon::NoteExpression::VST3Extensions` (subclass of `juce::VST3ClientExtensions`)
- `Ouaricon::NoteExpression::Controller` (NEC for `kTuningTypeID`)
- `Ouaricon::NoteExpression::updatePendingFromEvents(events, table)` (free function)

Dorico setup procedure (DOCS-02 — describes what's automatic post-Plan-25-02 vs what user still does manually):
- Auto-installed by plugin installer: .doricoexpmap lands at `~/Library/Application Support/Ouaricon/Expression Maps/` AND `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` (macOS) / `%APPDATA%\Ouaricon\Expression Maps\` AND `%APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\` (Windows)
- Auto-discovered: appears in Dorico's `Library → Expression Maps…` picker without manual import (when Dorico-scan write succeeded)
- Manual step still required: assign the expression map to the plugin's channel via `Play → Endpoint Setup → expression-map dropdown`
- Dorico version targeted: latest detected at install time (probe order: 6 → 5 → 4)
</interfaces>

<tasks>

<task type="auto">
  <name>Task 1: Author research/microtonal-dorico-integration.md with all 4 H2 sections (DOCS-01..04 in single combined file per D-13)</name>
  <files>research/microtonal-dorico-integration.md</files>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-13, D-14 — single combined file, exact H2 section titles, developer-facing tone, source material for FUT-06)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (entire file — Patterns 1-5, Landmines 1-5 source material for DOCS-01 + DOCS-03)
    - .planning/notes/dorico-microtonal-vst-research.md (background — Dorico's 3 wire mechanisms, source for DOCS-02 setup procedure + DOCS-03 quirks)
    - modules/tuning/note-expression/cpp/NoteExpression.h (concrete API for DOCS-01 architecture references)
    - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (concrete VST3-only TU for DOCS-01 — Controller body, queryIEditController dispatch)
    - modules/tuning/note-expression/cpp/NoteExpression.cpp (SharedCode TU for DOCS-01 — drainAndUpdate, dispatch slot)
    - modules/tuning/note-expression/README.md (existing consumer docs — DOCS-02 cross-references the user-side surface)
    - modules/tuning/note-expression/module.cmake (Plan 25-01's install rules — DOCS-02 documents what's automatic)
    - modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap (the asset DOCS-02 describes)
    - .planning/phases/24-propagate/24-08-final-sweep-PLAN.md (cross-references the 5 propagation patterns for DOCS-01)
    - .planning/phases/23-extract/23-CONTEXT.md sections D-20..D-32 (AU-link Steinberg-symbol leak fix — DOCS-04 troubleshooting reference)
    - research/microtonality-implementation-juce.md (cross-reference for DOCS-01 architecture)
    - research/microtonality-theory-formats.md (cross-reference for DOCS-03 — positions VST3 NE among alternatives)
  </read_first>
  <action>
    Create `research/microtonal-dorico-integration.md` (NEW file). Single combined Markdown file with exactly 4 H2 sections per D-13.

    Use this exact structure (the H2 titles are non-negotiable per D-13; the content under each is the executor's responsibility, but the depth and topic coverage MUST match the per-section spec below).

    Document header:
    ```markdown
    # Microtonal Dorico Integration — Internal Technical Notes

    **Status:** Internal developer reference (DOCS-05 boundary — NOT end-user-facing this milestone; source material for future website manual/quickstart authoring per FUT-06).
    **Audience:** Ouaricon developers, future contributors maintaining the `note-expression` module or Dorico integration.
    **Scope:** Covers the v1.5 milestone deliverable — the shared `note-expression` module, its consumer integration shape across all 8 v1.5 cohort plugins (O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant), the canonical pre-configured Dorico expression map shipped in their installers, and the host-side behavior the implementation navigates.
    **Cohort plugins:** 8 (see [Module Architecture](#module-architecture) for per-plugin composition pattern).
    **Last updated:** 2026-04-26 (Phase 25 closeout).
    **Cross-references:**
      - Spike findings: `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md`
      - Module README (consumer integration): `modules/tuning/note-expression/README.md`
      - Background research: `.planning/notes/dorico-microtonal-vst-research.md`
      - Phase decisions: Phase 23 CONTEXT (extraction), Phase 24 CONTEXT (propagation), Phase 25 CONTEXT (packaging + docs).

    ---
    ```

    ── Section 1: `## Module Architecture` (DOCS-01) ──

    Coverage requirements:
    - NEC advertisement flow: how `Ouaricon::NoteExpression::Controller` (in the VST3-only TU `cpp/vst3/NoteExpression_VST3.cpp`) advertises `kTuningTypeID` via `queryIEditController` to hosts; how the dispatch slot pattern (function-pointer registered at static-init) keeps the SharedCode TU Steinberg-free
    - Raw-event queue semantics: how the local JUCE patch (`JUCE-NE-PATCH` marker) routes raw `kNoteExpressionValueEvent` and noteId-tagged NoteOn/NoteOff events to `VST3ClientExtensions::onVst3RawEvent` BEFORE `MidiEventList::toMidiBuffer` drops them; the 128-slot `PendingTuningTable` (`std::array<std::atomic<double>, 128>`) ownership on `VST3Extensions`
    - Voice-routing logic: the `applyPendingTuning(table, midi, freq) → double` voice-side helper, `exchange(0.0)` consume semantic, multiplicative composition with any base-frequency source
    - Composition with each plugin's TuningEngine analog: NE delta is always 12-TET semitones; multiplicative compose is mathematically correct for any base tuning; references the 5 propagation patterns Phase 24 catalogued
    - Per-format module-source convention: `cpp/<format>/` (e.g., `cpp/vst3/`) auto-routes by `OuariconModules.cmake` to `${TARGET}_<FORMAT>`; SharedCode TU stays Steinberg-free; resolution to D-23-04-A
    - Module asset ownership extension (Phase 25 D-04): module owns canonical `.doricoexpmap` at `resources/`; consumer plugins inherit install via `module.cmake` install() rules

    Subsections (use H3):
    - `### Wire Diagram` — short ASCII diagram showing data flow: Dorico → JUCE patched wrapper → VST3Extensions raw-event queue → drainAndUpdate() → updatePendingFromEvents() → PendingTuningTable[pitch] → voice startNote → applyPendingTuning() → DSP trigger
    - `### Two-TU Split (Phase 23 D-20..D-22)` — the SharedCode-bound TU vs VST3-only TU split, custom function-pointer deleter pimpl, dispatch slot for NE correlation
    - `### Per-Format Source Convention (Phase 23 D-24..D-29)` — how `cpp/vst3/` content auto-routes; why this is structural-not-conditional (preprocessor guards leak symbols into SharedCode); the macro extension in `OuariconModules.cmake`
    - `### Per-Plugin Composition Patterns (Phase 24 catalog)` — name and one-paragraph each: classic-Synthesiser-multi-osc (O-Prism), classic-Synthesiser-physical-period (O-Bells/O-Wind), classic-Synthesiser-multi-sub-voice (O-IntonationPad with multiplicative neRatio), MPE-helper-based (O-Reed/O-Bowed), MPE-per-call-site (O-Formant). One-line code snippet per pattern showing the integration shape.
    - `### Module Asset Ownership (Phase 25 D-04)` — the canonical .doricoexpmap lives at `modules/tuning/note-expression/resources/`; module.cmake install() rules fire per-consumer; PKG/EXE installers replicate the dual-write logic at distribution time

    ── Section 2: `## Canonical Dorico Setup Procedure` (DOCS-02) ──

    Coverage requirements:
    - Step-by-step procedure suitable for direct translation into website manual/quickstart copy — pinned to specific Dorico major version (use whichever is current on the user's machine; pin in writing here)
    - Names exact Dorico menu paths verbatim (`Dorico → Library → Expression Maps…`, `Dorico → Play → Endpoint Setup`, etc.)
    - Splits "automatic" steps (what the plugin installer does for the user) from "manual" steps (what the user still must do)
    - Documents the install paths the plugin installer writes to (per Plan 25-02's bundled behavior)
    - Specifies the verification step (quarter-sharp C4 → +50¢ ≈ 269.29 Hz) that confirms the setup worked

    Subsections (use H3):
    - `### What the Plugin Installer Does (Automatic)`
      Bulleted list of the dual-write paths per platform; the install-time Dorico version probe; the auto-discovery via Dorico's user expression-maps scan path
    - `### What the User Must Still Do (Manual — One-Time)`
      Numbered procedure (Steinberg's exact menu paths). Concretely:
      1. Open Dorico → `Library → Expression Maps…`
      2. Confirm `Ouaricon VST3 Note Expression` appears in the list (auto-discovered if Dorico-scan write succeeded)
      3. If not present: click the import (folder) icon, navigate to `~/Library/Application Support/Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap` (macOS) or `%APPDATA%\Ouaricon\Expression Maps\Ouaricon-VST3-NoteExpression.doricoexpmap` (Windows), import
      4. Open `Play → Endpoint Setup`
      5. For each Ouaricon v1.5 plugin's channel, set the Expression Map dropdown to `Ouaricon VST3 Note Expression`
      6. Save the project
    - `### Verification`
      Write a quarter-sharp accidental on C4. Playback should land at C4 + 50¢ ≈ 269.29 Hz (vs standard C4 = 261.63 Hz). If the note plays at standard 261.63 Hz, either (a) the expression map is not assigned to that channel or (b) Microtonality in the assigned map is set to "Auto" instead of "VST3 Note Expression" — see [Troubleshooting Signatures](#troubleshooting-signatures).
    - `### Dorico Version Targeting`
      Phase 25 installer probes for Dorico 6, 5, 4 in that order; writes to the first detected. Future Dorico versions (7+) require an installer update — see Host-Side Behavior Quirks below for the maintenance touchpoint. Also: simultaneously installed Dorico 5 + 6 — installer writes to 6 only; user can manually copy to other versions if needed.

    ── Section 3: `## Host-Side Behavior Quirks` (DOCS-03) ──

    Coverage requirements:
    - Dorico's neighbor-semitone + NE-delta representation (Landmine 2 from spike-findings): quarter-sharp C4 arrives as `pitch=C#4, NE=-50¢` (NEVER `pitch=60, NE=+50¢`). Frequency is correct either way, but: never derive direction from NE sign; never correlate NE to NoteOn by pitch; ALWAYS correlate by `noteId`.
    - NEC handshake ignored by Dorico (Landmine 1): Dorico never queries `INoteExpressionController::iid`; sends NE solely based on expression-map Microtonality setting. Cubase/Nuendo do use the handshake. The Controller class is dead code for Dorico but kept for other VST3 hosts.
    - Sample-offset timing: NE events arrive in the same processing block as their corresponding NoteOn (block-locality assumption); spike validated this on Dorico 6; other hosts may emit mid-note NE requiring a persistent `noteId → voice` map (FUT-04).
    - VST3 `kTuningTypeID` measurement math (Pattern 5): `plain_semitones = 240.0 * (normalized - 0.5)`; full-scale span is 240 semitones (±10 octaves); table mapping for common Dorico accidentals.
    - Multi-Dorico-version installer caveat (D-07 from Phase 25 CONTEXT): installer probes 6/5/4 descending; first hit wins; future Dorico versions require installer config update; user can post-hoc copy to other version directories manually.
    - JUCE 8 patch dependency: every JUCE upgrade requires re-applying `scripts/juce-patches/note-expression-juce-8.0.4.patch`; CMake-time `JUCE-NE-PATCH` marker check fatal-errors at configure time if missing.
    - AU/VST3 platform asymmetry: Note Expression is VST3-only; AU has no equivalent (Dorico falls back to pitch bend for AU per upstream research); the per-format module-source convention auto-handles this — AU/Standalone targets get NO Steinberg references and link cleanly without microtonal capability.

    Subsections (use H3):
    - `### Dorico's Pitch Representation`
    - `### NEC Handshake Asymmetry`
    - `### Sample-Offset Timing Assumption`
    - `### `kTuningTypeID` Measurement Math`
      | Dorico accidental | NE normalized | Semitones | Cents | Hz at C4 |
      |---|---|---|---|---|
      | quarter-sharp | 0.497917 | -0.5 | -50 | (display Pattern 3 240-semi math; Hz = 269.29) |
      | natural | 0.5 | 0 | 0 | 261.63 |
      | quarter-flat (likely) | 0.502083 | +0.5 | +50 | (note: spike validated quarter-sharp only; quarter-flat unverified per FUT-05) |
    - `### Multi-Dorico-Version Maintenance`
    - `### JUCE Patch Dependency`
    - `### VST3-Only Capability`

    ── Section 4: `## Troubleshooting Signatures` (DOCS-04) ──

    Coverage requirements:
    - Symptoms-vs-cause table for the expression-map-skipped UX trap (Spike 002 reference) — by far the most common failure mode users will hit
    - Known regression signatures from Phase 23 (AU-link Steinberg-symbol leak — D-23-04-A) and Phase 24 (per-plugin propagation failures, e.g., O-Bowed missing `isBusesLayoutSupported`, O-Formant missing `OuariconModules.cmake` include)
    - Diagnostic checklist for "Dorico shows microtonal accidentals but plugin plays 12-TET"

    Subsections (use H3):
    - `### Primary Symptoms-vs-Cause Table` — Markdown table format:
      | Observed Symptom | Likely Cause | Diagnostic Step | Fix |
      |---|---|---|---|
      | Quarter-sharp C4 plays at 261.63 Hz (12-TET) | Expression map's Microtonality not set to "VST3 Note Expression" | `Library → Expression Maps…` → check assigned map's Microtonality dropdown | Set to `VST3 Note Expression` and save |
      | "Ouaricon VST3 Note Expression" doesn't appear in Dorico's picker | Installer's Dorico-scan-path write skipped (Dorico version not detected at install time, or installed AFTER plugin installer ran) | Check `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` | Manually import from `~/Library/Application Support/Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap` |
      | Microtonal accidentals work in Cubase but not Dorico | Cubase uses NEC handshake; Dorico ignores NEC and routes via expression map (Landmine 1) | Verify expression map assignment in `Play → Endpoint Setup` | Assign Ouaricon VST3 Note Expression map to the plugin's channel |
      | Quarter-sharp note has audible attack glide/sweep | Voice's `applyPendingTuning` running AFTER DSP trigger (Landmine 4 / Pattern 4 violated) | Read voice's `startNote`; confirm `applyPendingTuning` runs BEFORE `<dsp_model>.trigger(...)` | Reorder: applyPendingTuning then trigger |
      | Polyphonic chord — wrong note detuned | NE correlated to NoteOn by pitch instead of `noteId` (Landmine 2) | Read `updatePendingFromEvents` — confirm `noteId → pitch` map built from NoteOns first, then NE events looked up by `noteId` | Use the module's `updatePendingFromEvents` helper as-is; do not roll plugin-local correlation |
      | macOS AU build link fails: `Undefined symbols: Steinberg::Vst::INoteExpressionController::iid` | Steinberg-touching code leaked into SharedCode TU (D-23-04-A regression class) | `nm` the AU link line; check for any `cpp/*.cpp` (top-level) including `<pluginterfaces/...>` | Move Steinberg-touching code to `cpp/vst3/` per per-format convention; let `OuariconModules.cmake` route it correctly |
      | `cmake configure` fails: `[note-expression] JUCE patch marker 'JUCE-NE-PATCH' not found` | Local JUCE fork was upgraded without re-applying the NE patch | Run `./scripts/apply-juce-patches.sh` or regenerate the patch for the new JUCE version | After re-apply, `grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/` should return ≥4 hits |
    - `### "Dorico shows microtones but plugin plays 12-TET" Diagnostic Tree`
      Step-by-step decision tree starting from the most common cause (expression map's Microtonality setting) and descending to rarer causes (NEC dispatch failure, voice-side composition order, JUCE patch missing).
    - `### Known Regression Signatures`
      Brief one-paragraph each:
      - D-23-04-A AU-link Steinberg-symbol leak (Phase 23 fix via two-TU split + per-format convention)
      - O-Bowed `isBusesLayoutSupported` missing override (Phase 24 inline fix)
      - O-Formant `OuariconModules.cmake` include missing (Phase 24 inline fix)
      - O-IntonationPad missing `juce::juce_audio_utils` + `juce::juce_audio_devices` link (Phase 24 inline fix; blocked Standalone build)
      - O-Lyrica auval parameter-meta-flag (DEF-24-01 [DOWNGRADED] — benign tool-static-check artifact, NOT a defect)
    - `### Per-Plugin Smoke-Test Quick Reference`
      One-line per cohort plugin: `<Plugin>: Dorico 3-point gate ran <date>; result <PASS/FAIL>; CHANGELOG entry: "adds VST3 Note Expression microtonal support for Dorico" at v<X.Y.Z>`. Source: Phase 24 verification artifacts.

    ── Document footer ──

    ```markdown
    ---

    *Document produced as Phase 25 deliverable closing DOCS-01..05. The internal-only constraint (DOCS-05) is honored: this is a developer-reference document, not end-user-facing copy. Future website authoring (FUT-06) translates each section to chapter-style prose suitable for the Ouaricon Audio sales site.*

    *Source plans: `.planning/phases/25-package-docs/25-01-author-and-plumbing-PLAN.md`, `25-02-installer-bundling-sweep-PLAN.md`, `25-03-internal-notes-PLAN.md`.*
    ```

    Style guide:
    - Tone is technical reference: declarative, present-tense, concrete file/line references over generic descriptions.
    - When referencing a code construct, give its full namespaced name AND the file path: `Ouaricon::NoteExpression::applyPendingTuning(...)` in `modules/tuning/note-expression/cpp/NoteExpression.h`.
    - Cross-link section headings using GitHub-flavored anchor links (`[Troubleshooting Signatures](#troubleshooting-signatures)`).
    - Code snippets use ` ```cpp ` / ` ```bash ` / ` ```cmake ` fencing as appropriate.
    - Tables use Markdown pipe syntax with header separator row.
    - No emojis (per CLAUDE.md).
    - Target ~250-400 lines total (each H2 section ~60-100 lines).
  </action>
  <verify>
    <automated>
      test -f research/microtonal-dorico-integration.md &&
      [ "$(wc -l < research/microtonal-dorico-integration.md)" -ge 250 ] &&
      grep -q '^## Module Architecture' research/microtonal-dorico-integration.md &&
      grep -q '^## Canonical Dorico Setup Procedure' research/microtonal-dorico-integration.md &&
      grep -q '^## Host-Side Behavior Quirks' research/microtonal-dorico-integration.md &&
      grep -q '^## Troubleshooting Signatures' research/microtonal-dorico-integration.md &&
      [ "$(grep -c '^## ' research/microtonal-dorico-integration.md)" -eq 4 ] &&
      grep -q 'DOCS-05 boundary' research/microtonal-dorico-integration.md &&
      grep -q 'kTuningTypeID' research/microtonal-dorico-integration.md &&
      grep -q 'noteId' research/microtonal-dorico-integration.md &&
      grep -q 'applyPendingTuning' research/microtonal-dorico-integration.md &&
      grep -q 'JUCE-NE-PATCH' research/microtonal-dorico-integration.md &&
      grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' research/microtonal-dorico-integration.md &&
      grep -q 'cpp/vst3/' research/microtonal-dorico-integration.md &&
      grep -q 'O-Lyrica' research/microtonal-dorico-integration.md &&
      grep -q 'O-Bells' research/microtonal-dorico-integration.md &&
      grep -q 'O-Prism' research/microtonal-dorico-integration.md &&
      grep -q 'O-Wind' research/microtonal-dorico-integration.md &&
      grep -q 'O-Reed' research/microtonal-dorico-integration.md &&
      grep -q 'O-Bowed' research/microtonal-dorico-integration.md &&
      grep -q 'O-Formant' research/microtonal-dorico-integration.md &&
      grep -q 'O-IntonationPad' research/microtonal-dorico-integration.md &&
      grep -q 'Symptom' research/microtonal-dorico-integration.md &&
      grep -q '269.29' research/microtonal-dorico-integration.md &&
      grep -q 'D-23-04-A' research/microtonal-dorico-integration.md &&
      grep -q 'Library → Expression Maps' research/microtonal-dorico-integration.md &&
      grep -q 'Play → Endpoint Setup' research/microtonal-dorico-integration.md &&
      ! grep -iE '\b(amazing|powerful|you.?ll love|industry.?leading|revolutionary|stunning)\b' research/microtonal-dorico-integration.md
    </automated>
  </verify>
  <acceptance_criteria>
    - File `research/microtonal-dorico-integration.md` exists.
    - File length ≥250 lines (substantive content per H2 section, not stubs).
    - Exactly 4 H2 sections, in this order: `## Module Architecture`, `## Canonical Dorico Setup Procedure`, `## Host-Side Behavior Quirks`, `## Troubleshooting Signatures`. Verify with `grep -c '^## ' research/microtonal-dorico-integration.md` returns exactly 4.
    - Document declares the DOCS-05 boundary in its header (grep `DOCS-05 boundary` returns ≥1 match — proves internal-only intent is explicit).
    - All 8 cohort plugins are named at least once each (grep each plugin name, expect ≥1 match each — proves the cohort coverage is complete).
    - Load-bearing technical terms are referenced: `kTuningTypeID`, `noteId`, `applyPendingTuning`, `JUCE-NE-PATCH`, `Ouaricon-VST3-NoteExpression.doricoexpmap`, `cpp/vst3/` (the per-format convention directory). Each grep returns ≥1 match.
    - DOCS-02 references the exact Dorico menu paths verbatim: `Library → Expression Maps`, `Play → Endpoint Setup` (each grep returns ≥1 match).
    - DOCS-03 contains the `kTuningTypeID` math reference (`240.0 * (normalized - 0.5)` or equivalent — grep `240` returns ≥1 match).
    - DOCS-04 contains a Symptoms-vs-Cause table (grep `Symptom` returns ≥1 match for the table header) AND references the verification frequency 269.29 Hz (grep returns ≥1 match) AND the Phase 23 D-23-04-A regression class (grep returns ≥1 match).
    - No end-user-marketing tone (CONCRETE NEGATIVE GREP per WARNING #9 from checker review): the document MUST NOT contain marketing-tone phrases. Automated gate: `! grep -iE '\b(amazing|powerful|you.?ll love|industry.?leading|revolutionary|stunning)\b' research/microtonal-dorico-integration.md` returns exit 0 (i.e., no match). This is grep-enforced, not just author commitment.
  </acceptance_criteria>
  <done>
    `research/microtonal-dorico-integration.md` exists as a single combined developer-reference document with exactly 4 H2 sections covering DOCS-01..04. Tone is technical reference (DOCS-05 honored). Document describes shipped behavior from Plans 25-01/02 (the canonical .doricoexpmap, the module-side install rules, the per-platform PKG/EXE dual-write, the install-time Dorico version probe). All 8 cohort plugins are referenced; spike-findings landmines are cited; Phase 23 D-23-04-A regression class is documented in DOCS-04 troubleshooting.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| repo→public visibility | `research/microtonal-dorico-integration.md` is committed to the repo. If the repo is public (sales site uses GitHub-hosted source material), the document's content becomes externally visible. Per DOCS-05 boundary the tone is internal-developer; no secrets or licensing-sensitive code embedded. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-03-01 | I (Information disclosure) | Document contains system path references like `~/Library/Application Support/...` and `%APPDATA%\...` | accept | These are platform-standard user-scope paths, not personal information. The example path on macOS is from the user's CLAUDE.md memory (`/Users/taylorbrook/JUCE`) — but that path is already public knowledge in the existing module README and CMake error messages. No new disclosure. |
| T-25-03-02 | T (Tampering) | Document is the future website source material | accept | Document is in `research/` (per CLAUDE.md convention); commits go through standard git review. Future website team translates with explicit mapping (DOCS → website chapter); document edits don't deploy to website automatically. |
</threat_model>

<verification>
1. `research/microtonal-dorico-integration.md` exists with the exact 4 H2 sections in the prescribed order.
2. Document is ≥250 lines of substantive content.
3. All grep gates from Task 1 acceptance criteria pass.
4. Self-review for tone: no end-user-marketing copy (the document reads as technical reference suitable for FUT-06 translation, NOT for direct publication).
</verification>

<success_criteria>
Plan 25-03 succeeds when:
- DOCS-01 satisfied: Module Architecture section covers NEC advertisement flow, raw-event queue semantics, voice-routing logic, composition with each plugin's TuningEngine analog, per-format source convention, the 5 propagation patterns from Phase 24.
- DOCS-02 satisfied: Canonical Dorico Setup Procedure section names exact Dorico menu paths, splits automatic vs manual steps, names install paths, includes verification step (quarter-sharp C4 → 269.29 Hz), notes Dorico version targeting per D-07.
- DOCS-03 satisfied: Host-Side Behavior Quirks section covers Dorico's neighbor-semitone representation (Landmine 2), NEC handshake asymmetry (Landmine 1), sample-offset timing (block-locality), kTuningTypeID measurement math (Pattern 5), multi-Dorico-version maintenance, JUCE patch dependency, VST3-only capability.
- DOCS-04 satisfied: Troubleshooting Signatures section has symptoms-vs-cause table covering the expression-map-skipped UX trap (Landmine 3) plus regression signatures (D-23-04-A AU-link, Phase 24 per-plugin Rule-3 fixes, DEF-24-01 downgraded item).
- DOCS-05 satisfied: Document declares internal-only intent in the header; tone is developer-reference; structured for FUT-06 translation but not consumed as end-user copy this milestone.
- File at canonical path `research/microtonal-dorico-integration.md` per CLAUDE.md research/ convention.
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-03-internal-notes-SUMMARY.md` with:
- File created (path, line count, H2 section list)
- Cross-reference inventory (which spike-findings, which Phase 23/24 decisions, which research/ files were cited)
- DOCS-01..05 satisfaction summary (one-line each linking section → coverage)
- Hand-off note: this completes Phase 25 and milestone v1.5. The document is now the source-of-truth for FUT-06 website manual/quickstart authoring in the next milestone.
</output>
