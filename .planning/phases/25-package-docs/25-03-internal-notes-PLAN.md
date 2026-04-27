---
phase: 25-package-docs
plan: 03
type: execute
wave: 2
depends_on: [25-01-author-and-plumbing-PLAN.md]
files_modified:
  - research/microtonal-dorico-integration.md
autonomous: true
requirements: [DOCS-01, DOCS-02, DOCS-03, DOCS-04, DOCS-05]
must_haves:
  truths:
    - "Single combined research file exists at research/microtonal-dorico-integration.md (D-19)"
    - "File contains exactly 4 H2 sections: Module Architecture (DOCS-01), Canonical Dorico Setup Procedure (DOCS-02 reframed for Playback Template apply flow), Host-Side Behavior Quirks (DOCS-03 extended), Troubleshooting Signatures (DOCS-04 extended)"
    - "File honors DOCS-05 boundary: developer-facing only, no end-user marketing copy (D-20)"
    - "DOCS-02 names exact menu paths for the Playback Template apply flow AND the manual-import fallback (D-19 Pattern E)"
    - "DOCS-03 extends with NEW quirks: Default Library Additions does not exist by default; macOS-vs-Windows directory-name spaces variance; dev-vs-prod CID variance + configure_file @ONLY mitigation (D-19, S-3)"
    - "DOCS-04 extends with NEW signatures: missing-plugin warnings on template apply (graceful, NOT failure); silent template non-appearance after install (wrong Dorico version dir or wrong subdirectory name)"
    - "All references to module source code (DOCS-01) point to current paths: modules/tuning/note-expression/cpp/NoteExpression.h + modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp"
  artifacts:
    - path: "research/microtonal-dorico-integration.md"
      provides: "Internal-developer technical reference (4 H2 sections covering DOCS-01..04)"
      contains: "## Module Architecture"
      min_lines: 200
  key_links:
    - from: "research/microtonal-dorico-integration.md DOCS-01 section"
      to: "modules/tuning/note-expression/cpp/NoteExpression.h + cpp/vst3/NoteExpression_VST3.cpp"
      via: "Direct file path references; module's public API surface (Ouaricon::NoteExpression::*)"
      pattern: "modules/tuning/note-expression/cpp"
    - from: "research/microtonal-dorico-integration.md DOCS-02 section"
      to: "Plan 25-01's modules/tuning/note-expression/resources/ tree + Plan 25-02's installer dual-write logic"
      via: "Step-by-step Playback Template apply flow with exact menu paths"
      pattern: "Play -> Playback Template"
    - from: "research/microtonal-dorico-integration.md DOCS-03 section"
      to: "PATTERNS.md S-3 (dev/prod CID variance) + Naming Variance Catalog (lines 795-803)"
      via: "Concrete examples of CID divergence + dir-name asymmetry across platforms"
      pattern: "DefaultLibraryAdditions"
    - from: "research/microtonal-dorico-integration.md DOCS-04 section"
      to: "RESEARCH.md Pitfall 1-6 + Plan 25-01 v1 finding (FINDING-playback-template-pivot.md)"
      via: "Symptoms-vs-cause table; lessons learned from the v1 wrong-distribution-mechanism attempt"
      pattern: "Symptoms"
---

<objective>
Author a single combined internal-developer technical reference (`research/microtonal-dorico-integration.md`) covering all four DOCS-01..04 topics, REFRAMED for the v2 Playback Template architecture. The doc serves as source material for the future website manual/quickstart authoring pass (FUT-06), but is not user-facing in v1.5 (DOCS-05 boundary).

Purpose: Capture v1.5's accumulated knowledge — module architecture, canonical Dorico setup, host-side quirks, troubleshooting — in a form a future developer (or Claude in a future improve cycle) can consult to debug, extend, or document the suite.

Output: 1 NEW file (`research/microtonal-dorico-integration.md`) with 4 H2 sections.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/REQUIREMENTS.md

@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-RESEARCH.md
@.planning/phases/25-package-docs/25-PATTERNS.md
@.planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md
@.planning/phases/25-package-docs/25-01-SUMMARY.md

@.planning/phases/23-extract/23-CONTEXT.md
@.planning/phases/24-propagate/24-CONTEXT.md

@modules/tuning/note-expression/cpp/NoteExpression.h
@modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp
@modules/tuning/note-expression/README.md

@research/microtonality-implementation-juce.md
@research/microtonality-theory-formats.md
@.planning/notes/dorico-microtonal-vst-research.md

@.claude/skills/spike-findings-VST-development/SKILL.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md

<interfaces>
PATTERNS.md Pattern K (lines 647-705) provides the front-matter shape and TOC structure.

RESEARCH.md provides full content for DOCS-03 (host-side quirks — Patterns 1-6 + Pitfalls 1-6) and DOCS-04 (troubleshooting signatures from the same Pitfalls + the Plan 25-01 v1 finding).

The 4 H2 sections must each address its specific REQ-ID:
- DOCS-01: module architecture (NEC advertisement flow, raw-event queue semantics, voice-routing logic, composition with TuningEngine analogs)
- DOCS-02: canonical Dorico Playback Template apply flow + manual-import fallback (REFRAMED — v1 was standalone .doricoexpmap import; v2 is .dorico_pt + .doricolib auto-discovery)
- DOCS-03: host-side behavior quirks (EXTENDED — adds Default Library Additions caveat, macOS-vs-Windows asymmetry, dev/prod CID variance)
- DOCS-04: troubleshooting signatures (EXTENDED — adds missing-plugin warnings, silent template non-appearance)
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Author research/microtonal-dorico-integration.md (single combined doc, 4 H2 sections)</name>
  <read_first>
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern K (lines 647-705) — front-matter shape + TOC structure
    - .planning/phases/25-package-docs/25-RESEARCH.md sections: Pattern 1-6 (lines 146-300), Pitfall 1-6 (lines 330-365), "Code Examples" lines 374-530, Q1-Q7 answers (lines 558-714)
    - .planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md (full file — what was tried, why it failed)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-19 + D-20 — section structure + DOCS-05 boundary)
    - modules/tuning/note-expression/cpp/NoteExpression.h (Ouaricon::NoteExpression public API surface)
    - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (Steinberg-touching code split)
    - .planning/notes/dorico-microtonal-vst-research.md (background — Dorico's 3 wire mechanisms, source for DOCS-02 + DOCS-03)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (validated patterns 1-5, landmines 1-5; Landmine 3 is the kVST3NoteExpression invariant)
    - research/microtonality-implementation-juce.md (lines 1-40 — front-matter + TOC structure for the research/microtonality-*.md family)
  </read_first>
  <action>
    Author NEW file `research/microtonal-dorico-integration.md`. Use Pattern K's v2 target front-matter (PATTERNS.md lines 671-691) verbatim.

    File structure (in order):

    **1. YAML front-matter** (per Pattern K v2 target):
    ```yaml
    ---
    title: "Microtonal Dorico Integration: Module Architecture, Setup, Quirks, Troubleshooting"
    created: 2026-04-26
    last_verified: 2026-04-26
    juce_version: "8.0.4"
    dorico_version: "6.1.0"
    summary: "Internal-developer reference for the v1.5 microtonal cohort's Dorico integration: VST3 Note Expression module architecture, Playback Template apply procedure, host-side behavioral quirks, and troubleshooting signatures."
    domain: dsp
    type: reference
    keywords:
      - dorico
      - vst3-note-expression
      - playback-template
      - microtuning
      - integration-notes
    stages: [3, 4]
    audience: internal-dev-only
    agents: [dsp, integration]
    ---
    ```

    **2. Title (H1)** + 1-paragraph intro stating the doc is internal developer reference (DOCS-05) and is source material for future end-user manual authoring (FUT-06).

    **3. Table of Contents** with the 4 H2 sections (per Pattern K lines 696-702):
    ```markdown
    1. [Module Architecture](#module-architecture) — DOCS-01
    2. [Canonical Dorico Setup Procedure](#canonical-dorico-setup-procedure) — DOCS-02 (REFRAMED for Playback Template flow)
    3. [Host-Side Behavior Quirks](#host-side-behavior-quirks) — DOCS-03 (EXTENDED)
    4. [Troubleshooting Signatures](#troubleshooting-signatures) — DOCS-04 (EXTENDED)
    ```

    **4. ## Module Architecture** (DOCS-01) — required content:
    - **NEC advertisement flow**: how `Ouaricon::NoteExpression::Controller` advertises `kTuningTypeID` to the host via VST3's IInfoListener, the queryIEditController dispatch from VST3Extensions to Controller (Phase 23 D-23 two-TU split: cpp/NoteExpression.cpp Steinberg-free body + cpp/vst3/NoteExpression_VST3.cpp VST3-only TU). Cite spike-findings Pattern 1 + Phase 23 Plan 05 SUMMARY.md decision.
    - **Raw-event queue semantics**: the local JUCE patch (scripts/juce-patches/note-expression-juce-8.0.4.patch) adds `VST3ClientExtensions::onVst3RawEvent` so kNoteExpressionValueEvent and noteId-tagged NoteOn/NoteOff reach the plugin BEFORE MidiEventList::toMidiBuffer drops them. The VST3Extensions subclass owns the raw-event queue + 128-slot std::array<std::atomic<double>> PendingTuningTable.
    - **Voice-routing logic**: `applyPendingTuning(pendingSource, midiNote, currentFrequency)` — header-only voice helper applied BEFORE the DSP model's trigger() to prevent attack zipper (Landmine 4). Each consumer plugin's `startNote` calls this in one line.
    - **Composition with TuningEngine analogs**: each cohort plugin's existing TuningEngine (e.g., O-Lyrica's TuningEngine, O-Bells' applyPendingTuning at root with multiplicative ratio derivation per Phase 24's catalogued 5 propagation patterns). NE delta composes multiplicatively with base-frequency lookup — NO raw pow() bypass (LYR-02 invariant).
    - **5 propagation patterns** discovered in Phase 24 (briefly enumerated): classic-Synthesiser-multi-osc (Prism), classic-Synthesiser-physical-period (Bells/Wind), classic-Synthesiser-multi-sub-voice (IntonationPad), MPE-helper-based (Reed/Bowed), MPE-per-call-site (Formant). Reference Phase 24 STATE.md for narrative depth.
    - **NEW (Phase 25 v2): module owns 2 distributable resources**: .dorico_pt + .doricolib. Owned by `module.cmake` per-consumer hook firing (Pattern A + S-2). `ouaricon_extract_vst3_cids` helper extracts CIDs from each consumer's built moduleinfo.json (Pattern B + S-3 — dev/prod variance mitigation).

    **5. ## Canonical Dorico Setup Procedure** (DOCS-02 — REFRAMED for Playback Template flow):
    - **Auto-discovery flow** (the v2 default — installer writes to Dorico auto-scan dirs):
      1. Install any Ouaricon cohort plugin (PKG on macOS, EXE on Windows). Installer writes both Microtonal Suite resources to `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/` and `.../Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib` (macOS) or `%APPDATA%\Steinberg\Dorico [N]\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\` and `...\DefaultLibraryAdditions\` (Windows).
      2. Restart Dorico. At startup, Dorico scans `PlaybackTemplateSpecs/*/` (binary symbol "Loading PlaybackTemplateSpec:" — see RESEARCH.md Q6) AND calls `loadDefaultLibraryAdditions` to merge any `.doricolib` in `Default Library Additions/` into the in-memory default library.
      3. Verify: `Library -> Expression Maps` shows "Ouaricon VST3 Note Expression"; `Play -> Playback Template` shows "Ouaricon Microtonal Suite".
      4. Apply: `Play -> Playback Template -> Ouaricon Microtonal Suite -> Apply and Close`.
      5. Quarter-sharp C4 verifies routing: write a quarter-sharp accidental on C4 -> playback at approximately 269.29 Hz (vs 12-TET 261.63 Hz). +50¢ confirmed.
    - **Manual-import fallback** (if auto-discovery missed the directory — D-13):
      1. Playback Template: `Play -> Playback Template -> Import...` -> select `Ouaricon-Microtonal-Suite.dorico_pt` from `~/Library/Application Support/Ouaricon/Microtonal Suite/` (macOS) or `%APPDATA%\Ouaricon\Microtonal Suite\` (Windows).
      2. Expression Map library: `Library -> Import Library...` -> select `Ouaricon-VST3-NoteExpression.doricolib` from the same shared directory.
      3. Apply the template per step 4 above.
    - **Stage gate (3-point smoke from Phase 24 D-07)**: quarter-sharp C4 = +50¢; no attack zipper on first sample of tuned note; polyphonic chord with quarter-sharp C4 + natural E4 + natural G4 shows ONLY C4 detuned (NE-correlation by noteId).
    - **NOTE on legacy v1 flow**: the standalone `.doricoexpmap` -> `Expression Maps/User/` drop flow (Plan 25-01 v1) does NOT work. Dorico does not auto-ingest `.doricoexpmap` files (verified empirically + by Dorico 6 binary `strings` — see FINDING-playback-template-pivot.md). The v2 Playback Template flow is the canonical mechanism.

    **6. ## Host-Side Behavior Quirks** (DOCS-03 — EXTENDED):
    - **Dorico's neighbor-semitone + NE-delta representation** (carry forward from v1): quarter-sharp C4 is sent as MIDI C#4 + NE delta of -50¢. The plugin must compose the delta with the MIDI-note base frequency. Cite Landmine 1.
    - **NEC handshake ignored by Dorico but kept for other hosts** (carry forward): Dorico does not call queryIEditController; the NEC advertises kTuningTypeID for hosts that DO call it. Don't remove just because Dorico ignores it.
    - **Sample-offset timing** (carry forward): NE events arrive in the same processBlock as the corresponding NoteOn; same-block correlation by noteId is sufficient (FUT-04 cross-block tracking deferred).
    - **Multiple-Dorico-version installer caveat** (D-12): installer probes 6 -> 5 -> 4 and writes to first found. Future Dorico 7 release requires installer config update (foreach extends to 7 5 4). Document as known maintenance touchpoint.
    - **NEW: `Default Library Additions` directory does not exist by default on user systems**. Dorico does not auto-create it. Installer must `ForceDirectories` (Inno) or `mkdir -p` (PKG postinstall). If the dir is missing, .doricolib auto-discovery silently fails. Cite Pitfall 3.
    - **NEW: Directory name asymmetry across platforms (Pitfall 3)**:
      - macOS: `Default Library Additions` (with SPACES)
      - Windows: `DefaultLibraryAdditions` (NO spaces)
      Verified: macOS binary string `Default Library Additions`; Windows binary string `DefaultLibraryAdditions`. Conflating the two breaks the install on the wrong platform.
    - **NEW: Dev vs prod CID variance** (S-3 + Pitfall 2):
      - Dev manufacturer code `OuDv` -> CID middle bytes `4F754476` (e.g. O-Lyrica dev: `ABCDEF019182FAEB4F7544764F4C7972`)
      - Prod manufacturer code `OuAu` -> CID middle bytes `4F754175` (e.g. O-Lyrica prod: `ABCDEF019182FAEB4F7541754F4C7972`)
      Hard-coding dev CIDs into a prod-shipped template breaks routing. Mitigation: `ouaricon_extract_vst3_cids` reads each plugin's actually-built `Contents/Resources/moduleinfo.json` at packaging time + `configure_file @ONLY` substitutes per-flavor tokens.
    - **NEW: Dorico's `<entries>` model handles missing-plugin gracefully** (D-04 / Q4): a Playback Template with 8 slots applied on a system with only 1 Ouaricon plugin installed produces 7 missing-plugin warnings + 1 successfully-loaded slot. Template apply does NOT block on missing plugins.

    **7. ## Troubleshooting Signatures** (DOCS-04 — EXTENDED):
    Authored as a symptoms-vs-cause table where applicable. Required entries:

    | Symptom | Likely cause | Fix |
    |---------|--------------|-----|
    | Plugin plays 12-TET despite Dorico sending microtones (Landmine 3 + the v1.5 spike's UX trap) | Expression map's microtonalPlaybackMethod is `kAuto` or `kPitchBend` instead of `kVST3NoteExpression` | Confirm `Ouaricon-VST3-NoteExpression.doricolib` was installed AND the applied Playback Template references its entityID `xmap.ouaricon.vst3_note_expression`. Inspect installed `endpointconfig.xml` for `<expressionMapID>xmap.ouaricon.vst3_note_expression</expressionMapID>`. |
    | Template does NOT appear in `Play -> Playback Template` after install | Wrong Dorico version directory (installer wrote to Dorico 5 but user runs Dorico 6, or vice versa) | Inspect installer log for `[Ouaricon] Microtonal Suite installed for Dorico N`. Compare N to the actual Dorico version the user is running. Manual fix: `Play -> Playback Template -> Import...` from `~/Library/Application Support/Ouaricon/Microtonal Suite/`. |
    | Expression map does NOT appear in `Library -> Expression Maps` after install | `.doricolib` written to wrong subdirectory (e.g. macOS-style `Default Library Additions` on Windows where the dir name has NO spaces) | Verify exact path on disk: macOS `~/Library/Application Support/Steinberg/Dorico N/Default Library Additions/`; Windows `%APPDATA%\Steinberg\Dorico N\DefaultLibraryAdditions\`. Pitfall 3. |
    | Template applies but no plugins load | Dev CID baked into prod installer (Pitfall 2) | Inspect installed `endpointconfig.xml` for CIDs containing the bytes `4F7544 76` (`OuDv` dev) when production plugins use `4F754175` (`OuAu`). Rebuild installer against prod artifacts. |
    | Template applies but only some plugins load (others produce warnings) | Expected (D-04) — Dorico graceful missing-plugin behavior | NOT a failure. Install missing plugins to populate other slots, OR apply a smaller template via custom edit. |
    | Plugin loads but quarter-sharp C4 plays at wrong pitch | NE delta not composed with TuningEngine base frequency, OR raw pow() bypass (regression of LYR-02) | Audit each consumer plugin's `startNote` for the `applyPendingTuning(...)` call; confirm composition is multiplicative with the TuningEngine base. |
    | Plugin attack zipper (audible pitch sweep at start of tuned note) | NE delta applied AFTER trigger() instead of BEFORE | Audit each consumer plugin's voice helper: `applyPendingTuning` MUST be called BEFORE the DSP model's `trigger(...)` (Landmine 4). |
    | First-time install: nothing happens after restart, no errors | `.doricoexpmap` legacy attempt re-introduced (Plan 25-01 v1 distribution mechanism — does not work) | Verify the installed file is `.dorico_pt` + `.doricolib` (current asset names), NOT `.doricoexpmap` (deprecated). Dorico binary strings do not recognize `.doricoexpmap` as an extension. See FINDING-playback-template-pivot.md. |
    | Installer reports success but Dorico did not pick up the template | `Default Library Additions` directory was not created (Dorico does not auto-create it) | Verify the installer log line `[Ouaricon] Microtonal Suite installed for Dorico N` appeared. If the dir was missing pre-install, installer creates it (Pattern G + Pattern H both call `mkdir -p` / `ForceDirectories`). If still missing, inspect installer log for any silent failure. |
    | `.dorico_pt` zipped with parent dir wrapping (Pitfall 5) | Build invocation used wrong WORKING_DIRECTORY for `cmake -E tar cf` | Verify `unzip -l Ouaricon-Microtonal-Suite.dorico_pt | head` — first non-zero entries should be `PlaybackTemplateSpecs/Ouaricon Microtonal Suite/...` and `EndpointConfigs/Ouaricon Microtonal Suite/...` (no parent dir prefix). If wrong, `module.cmake`'s `add_custom_command` WORKING_DIRECTORY is wrong. |

    **8. Closing footer**: signature line "Internal developer reference. Source material for FUT-06 (end-user manual on the sales website). Last verified against Dorico 6.1.0 + JUCE 8.0.4."

    **Tone**: technical, concise, factual. NO marketing copy. NO end-user-style "Quick Start" or "Get Started" phrasing. Honor DOCS-05 (D-20) — this is for developers debugging the integration, not for users learning Dorico.

    **Length target**: 200-400 lines. Heavy use of tables + bullet lists for scannability. Code references as inline backticks; multi-line code in fenced blocks.
  </action>
  <verify>
    <automated>F=research/microtonal-dorico-integration.md && test -f "$F" && grep -q "^## Module Architecture" "$F" && grep -q "^## Canonical Dorico Setup Procedure" "$F" && grep -q "^## Host-Side Behavior Quirks" "$F" && grep -q "^## Troubleshooting Signatures" "$F" && grep -q "audience: internal-dev-only" "$F" && grep -q "modules/tuning/note-expression/cpp/NoteExpression.h" "$F" && grep -q "modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp" "$F" && grep -q "Default Library Additions" "$F" && grep -q "DefaultLibraryAdditions" "$F" && grep -q "kVST3NoteExpression" "$F" && grep -q "Play -> Playback Template" "$F" && grep -q "Library -> Import Library\|Library -> Expression Maps" "$F" && grep -q "OuDv\|4F754476" "$F" && grep -q "OuAu\|4F754175" "$F" && grep -q "moduleinfo.json" "$F" && grep -q "configure_file" "$F" && grep -q "missing-plugin\|missing plugin" "$F" && grep -q "doricoexpmap\|FINDING-playback-template-pivot" "$F" && [ "$(wc -l < "$F")" -ge 200 ]</automated>
  </verify>
  <acceptance_criteria>
    - File exists at canonical path
    - All 4 H2 section headers present with correct names
    - Front-matter contains `audience: internal-dev-only` (DOCS-05 honored)
    - DOCS-01 references both module source files (Steinberg-free + VST3-only TUs)
    - DOCS-03 documents BOTH `Default Library Additions` (macOS, spaces) AND `DefaultLibraryAdditions` (Windows, NO spaces)
    - File contains `kVST3NoteExpression` (Landmine 3 invariant explicitly named)
    - DOCS-02 names exact menu paths `Play -> Playback Template` AND a Library import path
    - DOCS-03 documents dev vs prod CID variance (`OuDv`/`4F754476` AND `OuAu`/`4F754175` referenced)
    - DOCS-03 references `moduleinfo.json` + `configure_file` mitigation
    - DOCS-04 covers missing-plugin warning scenario AND legacy `.doricoexpmap` failure mode (with reference to FINDING)
    - File length >= 200 lines
  </acceptance_criteria>
  <done>4-section internal reference doc authored at research/microtonal-dorico-integration.md, satisfies DOCS-01..05 with REFRAMED + EXTENDED content per D-19.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Documentation file -> developer reading it | Doc is internal-only (DOCS-05); not deployed; not user-facing. No runtime trust boundary. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-03-01 | Information disclosure | Doc reveals dev CIDs (`4F754476`) and module internals | accept | Repo is private; CIDs are public-after-build anyway (every shipped VST3 bundle exposes them in `moduleinfo.json`). Module internals are open implementation detail. No secrets disclosed. |
| T-25-03-02 | Repudiation / staleness | Doc becomes stale after future Dorico release or JUCE upgrade | mitigate | Front-matter `last_verified: 2026-04-26` + `dorico_version: "6.1.0"` + `juce_version: "8.0.4"` makes staleness detectable. DOCS-03 explicitly names "future Dorico 7 release requires installer config update" as a known maintenance touchpoint. |
| T-25-03-03 | Tampering | Doc instructs developer toward wrong distribution mechanism (regression to v1) | mitigate | DOCS-04 explicitly enumerates the legacy `.doricoexpmap` failure mode + references FINDING-playback-template-pivot.md. Future developer reading the doc cannot accidentally regress. |
| T-25-03-04 | Path traversal | N/A — doc, not code | not applicable | |
| T-25-03-05 | Privilege escalation | N/A — doc, not code | not applicable | |
| T-25-03-06 | Idempotency | N/A — doc, not deployment payload | not applicable | |
| T-25-03-07 | Stale-asset | Doc references file paths that may move in future refactors | accept | Path references (`modules/tuning/note-expression/cpp/...`) are in repo and move-tracked by git. If a future phase moves the module, the doc gets updated in that phase's plan. |
</threat_model>

<verification>
- Single-task plan; all gates are inside the one task's `<acceptance_criteria>` block.
- DOCS-05 boundary verified by `audience: internal-dev-only` front-matter check.
- DOCS-01..04 each gated by section-header presence + content-keyword presence.
- Pitfall 3 (directory-name asymmetry) verified by both spelling variants present.
- Landmine 3 (kVST3NoteExpression invariant) explicitly named.
- Plan 25-01 v1 finding (legacy `.doricoexpmap` failure) explicitly referenced — prevents future regression.
</verification>

<success_criteria>
- research/microtonal-dorico-integration.md exists
- All 4 H2 sections present and named per D-19
- DOCS-01..04 content satisfies their respective requirements (REFRAMED for Playback Template flow per D-17)
- DOCS-05 boundary honored (audience: internal-dev-only; no marketing copy)
- File length >= 200 lines (substantive content)
- All NEW quirks/signatures from D-19 EXTENDED list are present
- File integrates with existing `research/microtonality-*.md` family conventions (Pattern K)
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-03-SUMMARY.md` documenting:
- File path and line count of the authored doc
- Mapping of DOCS-01..05 -> sections in the file
- Cross-references to RESEARCH.md sources used (which Patterns + Pitfalls + Q&A informed each H2 section)
- Any deferred content (e.g., articulation switches, MTS-ESP path) explicitly NOT covered with rationale
- "Phase 25 complete" closing line (this is the last plan in Wave 2; Plans 25-02 and 25-03 close concurrently)

Update `.planning/STATE.md` with Plan 25-03 completion timestamp.
</output>
</content>
