---
phase: 25-package-docs
plan: 03
type: execute
wave: 2
depends_on: [25-01]
files_modified:
  - research/microtonal-dorico-integration.md
autonomous: true
requirements: [DOCS-01, DOCS-02, DOCS-03, DOCS-04, DOCS-05]
tags: [dorico, vst3-note-expression, microtuning, internal-docs, research]

must_haves:
  truths:
    - "research/microtonal-dorico-integration.md exists as a single combined developer-reference doc with 4 H2 sections (DOCS-01..04)."
    - "DOCS-01 'Module Architecture' covers NEC advertisement flow, raw-event queue semantics, voice-routing, and TuningEngine composition; references the shipping module source files."
    - "DOCS-02 'Canonical Dorico Setup Procedure' documents Path B's Library Manager Import + per-channel assignment flow with exact menu paths (no Playback Template, no auto-discovery)."
    - "DOCS-03 'Host-Side Behavior Quirks' covers Dorico neighbor-semitone + NE-delta representation, NEC handshake, sample-offset timing, AND the new kScoreLibrary 48-container schema requirement, AND rationale for explicit-import + skipped <pluginNames> (D-01/D-02 carry-forward)."
    - "DOCS-04 'Troubleshooting Signatures' covers the expression-map-skipped UX trap AND new signatures for 'Library Manager Import: invalid file format' + 'Expression map appears in dropdown but quarter-sharp plays at semitone'."
    - "DOCS-05 honored: doc is in research/ (not docs/, not website-facing); audience tagged 'internal-dev-only' in front-matter."
  artifacts:
    - path: "research/microtonal-dorico-integration.md"
      provides: "Single combined developer-facing technical reference for v1.5 microtonal Dorico integration (Path B)"
      contains: "## Module Architecture"
      min_lines: 200
  key_links:
    - from: "DOCS-01 (Module Architecture)"
      to: "modules/tuning/note-expression/cpp/NoteExpression.h, cpp/vst3/NoteExpression_VST3.cpp"
      via: "code-reference markdown links"
      pattern: "modules/tuning/note-expression/cpp"
    - from: "DOCS-02 (Setup Procedure)"
      to: "Library → Library Manager → Import… → Ouaricon-VST3-NoteExpression.doricolib"
      via: "named exact menu paths and file paths"
      pattern: "Library Manager"
    - from: "DOCS-04 (Troubleshooting)"
      to: "DOCS-03 (Host-Side Quirks)"
      via: "symptom-cause table linking failure signatures to underlying mechanism"
      pattern: "kScoreLibrary"
---

<objective>
Author `research/microtonal-dorico-integration.md` as the single combined developer-facing technical reference for the v1.5 microtonal cohort's Dorico integration. Captures module architecture, the canonical Path B setup procedure, host-side behavior quirks (including the v2 schema-defect lesson), and troubleshooting signatures.

Purpose: Internal-only source-of-truth notes for future implementers (us, v1.6) and as raw material for later end-user manual authoring on the sales website (FUT-06; out of scope this milestone). Tone: technical reference, terse, code-grounded. No quickstart copy, no marketing prose.

Output: One Markdown file with YAML front-matter, four H2 sections (DOCS-01..04), and the failure-mode documentation (kScoreLibrary 48-container schema, explicit-import rationale, skipped `<pluginNames>` rationale) that v3 planning surfaced as critical knowledge.

Per D-12 / D-13: single combined doc; 4 H2 sections; developer-facing only.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-FINDING-path-b-validation.md
@.planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md
@.planning/phases/25-package-docs/25-RESEARCH.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@research/microtonality-implementation-juce.md

<interfaces>
<!-- Existing research/ file family — match style/tone -->
research/microtonality-implementation-juce.md  (existing reference; front-matter shape, TOC structure, code excerpt style)
research/microtonality-theory-formats.md
research/microtonality-comprehensive-database.md
research/microtonality-commercial-performance.md

<!-- Front-matter convention (extracted from microtonality-implementation-juce.md) -->
```yaml
title: "..."
created: YYYY-MM-DD
last_verified: YYYY-MM-DD
juce_version: "8.0.4"
summary: "..."
domain: dsp
type: guide  # or `reference`
keywords: [...]
stages: [...]
agents: [...]
```

<!-- Module surface to reference (DOCS-01) -->
- modules/tuning/note-expression/cpp/NoteExpression.h           (public API; SharedCode-bound types)
- modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (VST3-only TU; Controller body, dispatch)
- Public namespace: Ouaricon::NoteExpression
- Two TU split (Phase 23 D-23-04-A): SharedCode-bound TU + VST3-only TU
- Two dispatch slots (g_neUpdate, g_neQuery) connect SharedCode to VST3 layer

<!-- Validated patterns (from spike-findings skill — already auto-loaded) -->
Spike findings 5 patterns + 5 landmines documented at:
.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
- Pattern 1: noteId correlation
- Pattern 2: dev/prod CID variance (informational under Path B; not consumed by v3 plumbing)
- Pattern 3: full-scale 240-semitone tuning table
- Pattern 4: pre-DSP-trigger tuning application (zipper prevention)
- Pattern 5: NEC handshake (advertise via VST3ClientExtensions)
- Landmine 3 (load-bearing for DOCS-03): microtonalPlaybackMethod=kVST3NoteExpression — never kAuto/kPitchBend
</interfaces>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Author research/microtonal-dorico-integration.md (4 H2 sections)</name>
  <files>research/microtonal-dorico-integration.md</files>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-12 doc layout; D-13 audience constraint)
    - .planning/phases/25-package-docs/25-FINDING-path-b-validation.md (failure mode + Path B user flow; source for DOCS-04 troubleshooting signatures)
    - .planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md (background context: why .doricoexpmap failed; v1→v2 history)
    - modules/tuning/note-expression/cpp/NoteExpression.h (public API; for DOCS-01 architecture)
    - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (VST3-only impl; for DOCS-01)
    - modules/tuning/note-expression/README.md (consumer integration patterns)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (5 patterns + 5 landmines; source for DOCS-03 quirks)
    - research/microtonality-implementation-juce.md (front-matter shape + tone reference)
  </read_first>
  <action>
    Create `research/microtonal-dorico-integration.md` as a single Markdown file. Structure: YAML front-matter, then a TOC, then 4 H2 sections (DOCS-01 → DOCS-04). Audience: internal-dev-only. Tone: technical reference, terse.

    **Front-matter (per existing research/ family convention):**

    ```yaml
    ---
    title: "Microtonal Dorico Integration: Module Architecture, Setup, Quirks, Troubleshooting"
    created: 2026-04-27
    last_verified: 2026-04-27
    juce_version: "8.0.4"
    dorico_version: "6.x"
    summary: "Internal developer reference for the v1.5 microtonal cohort's Dorico integration via VST3 Note Expression. Covers module architecture, canonical Path B setup procedure (Library Manager Import), host-side behavior quirks, and troubleshooting signatures. Source-of-truth notes; not user-facing manual copy."
    domain: dsp
    type: reference
    keywords:
      - dorico
      - vst3-note-expression
      - microtuning
      - doricolib
      - library-manager
      - integration-notes
    stages: [3, 4]
    audience: internal-dev-only
    agents: [dsp, integration]
    related:
      - .planning/phases/23-extract/
      - .planning/phases/24-propagate/
      - .planning/phases/25-package-docs/
      - .claude/skills/spike-findings-VST-development/
    ---
    ```

    **TOC (immediately after front-matter):**
    ```markdown
    ## Table of Contents

    1. [Module Architecture](#module-architecture)            — DOCS-01
    2. [Canonical Dorico Setup Procedure](#canonical-dorico-setup-procedure)  — DOCS-02 (Path B)
    3. [Host-Side Behavior Quirks](#host-side-behavior-quirks)  — DOCS-03
    4. [Troubleshooting Signatures](#troubleshooting-signatures)  — DOCS-04
    ```

    **H2 Section 1 — `## Module Architecture` (DOCS-01)**

    Cover (200-400 words + targeted code excerpts):
    - **Public surface.** Namespace `Ouaricon::NoteExpression`; entry points are header-only consumer helpers. Reference `modules/tuning/note-expression/cpp/NoteExpression.h` (link with file path).
    - **NEC advertisement flow (Pattern 5).** `TuningNoteExpressionController` advertises `kTuningTypeID` via `VST3ClientExtensions::queryIEditController`. Dorico discovers the type during plugin scan, uses it to route per-note pitch deltas as VST3 NE events.
    - **Raw-event queue semantics.** Block-rate ingest in `processBlock`; events stored in `PendingTuningTable` keyed by `noteId` (Pattern 1 — noteId correlation between NoteOn and NE delta within the same block).
    - **Voice-routing logic.** Header-only helper `applyPendingTuning(pendingSource, midiNote, currentFrequency)` resolves the queue entry for a given noteId at voice-trigger time and returns the tuned frequency. Composition order is load-bearing (Pattern 4 — apply BEFORE DSP `trigger(...)` to prevent attack zipper).
    - **TuningEngine composition.** Module composes with each plugin's existing `TuningEngine` multiplicatively — NE deltas stack on top of alternate tunings without bypassing tuning math. Reference: O-Lyrica integration as Phase 23 canary; the composition order in `HarpSynthVoice` is the exemplar pattern.
    - **Two-TU split (Phase 23 D-23-04-A architectural fix).** SharedCode-bound TU at `cpp/NoteExpression.cpp` (no Steinberg symbols) hosts ctor/dtor/drainAndUpdate body; VST3-only TU at `cpp/vst3/NoteExpression_VST3.cpp` hosts Controller body + `vst3QueryIEditController` + dispatch registrar. Two function-pointer dispatch slots (`g_neUpdate`, `g_neQuery`) bridge the layers without leaking VST3 symbols into AU/Standalone link lines.
    - **Module ownership of the Dorico expression-map asset (Phase 25 D-04).** `resources/library/Ouaricon-VST3-NoteExpression.doricolib` is the canonical asset; `module.cmake` adds a per-consumer `install(SCRIPT)` rule.

    Include 2-3 small code excerpts (5-10 lines each): the public helper signature from `NoteExpression.h`; a snippet of the consumer integration from O-Lyrica `HarpSynthVoice::startNote` showing the composition order; the dispatch-slot pattern from `NoteExpression_VST3.cpp`.

    **H2 Section 2 — `## Canonical Dorico Setup Procedure` (DOCS-02 — Path B)**

    Cover (150-250 words). EXACT menu paths, no abstractions. Reframed for Path B's manual-import + manual-assign flow.

    Per-machine one-time setup:
    1. Install any Ouaricon plugin (PKG on macOS, EXE on Windows). Installer lands `Ouaricon-VST3-NoteExpression.doricolib` at:
       - macOS: `~/Library/Application Support/Ouaricon/Microtonal Suite/`
       - Windows: `%APPDATA%\Ouaricon\Microtonal Suite\`
    2. Open Dorico (any project).
    3. `Library → Library Manager → Import…`
    4. Navigate to the install path above and select `Ouaricon-VST3-NoteExpression.doricolib`.
    5. Click Import. Dorico adds "Ouaricon VST3 Note Expression" to the project library; verify under `Library → Expression Maps`.

    Per-project per-channel assignment:
    1. Load any Ouaricon plugin in your project: `Play → Endpoints → Add Plug-in` (or via existing routing).
    2. With the plugin's channel selected, locate the `Expression Map` dropdown in the Endpoints panel.
    3. Pick "Ouaricon VST3 Note Expression" from the dropdown.
    4. Microtonal accidentals on that channel now route as VST3 Note Expression to the plugin.

    Verification: place a note with a quarter-sharp accidental at C4. Press play. Pitch sounds at ~269 Hz (between C4 = 261.63 Hz and C♯ = 277.18 Hz). If pitch is at 261.63 Hz (semitone), the expression map is not assigned correctly — see Troubleshooting.

    No Playback Template, no auto-discovery, no `Default Library Additions/` — those are deferred or rejected (see Quirks).

    **H2 Section 3 — `## Host-Side Behavior Quirks` (DOCS-03)**

    Cover (300-500 words). Each quirk is a small subsection (`### `).

    1. **Dorico's neighbor-semitone + NE-delta representation.** Quarter-sharp C4 is wired as `noteOn(C#4)` + `NoteExpression(kTuningTypeID, value=-50¢)` in the same processBlock. The plugin must correlate the NE delta to the noteId at trigger time, not bias the noteOn. (Pattern 1.)
    2. **NEC handshake is ignored by Dorico.** Dorico does not consume the `VST3ClientExtensions::queryIEditController` advertisement — it routes NE events regardless of whether the plugin advertises the type. We keep the advertisement for other host parity (Bitwig, etc.). (Pattern 5.)
    3. **Sample-offset timing requirements.** NE events carry sample offsets within the block. The voice-trigger logic must resolve the queue at trigger time (which is after block-rate ingest), not on `noteOn` event arrival.
    4. **kScoreLibrary 48-container schema requirement.** A Dorico-valid `.doricolib` requires the full 48 top-level `<kScoreLibrary>` containers as siblings (`<temperaments>`, `<accidentalSystems>`, …, `<lineStyleCollectionDefinition>`), even when most are empty. Dorico's Library Manager rejects partial-skeleton files with "Error opening file: invalid file format". The v2 implementation (commit 819b2b4) inherited this defect by trusting the recovered cd2c2c6 XML body, which was an `<ExpressionMapDefinition>` *fragment*, not a complete library bundle. v3 protocol (D-03): bootstrap from the HALion Sonic factory `expressionMapsDefinitions.xml` skeleton (`/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml`), empty all containers except `<expressionMapDefinitions>`, inject the recovered `<ExpressionMapDefinition>`. Reference asset at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` (6,431 B).
    5. **Explicit-import vs auto-discovery (D-01 rationale).** v1.5 ships explicit Library Manager Import to one Ouaricon-controlled path. Auto-discovery via Dorico's `Expression Maps/User/` directory was tested informationally during Plan 25-01 Wave 0 (see `25-01-WAVE-0-VERIFICATION.md`). v1.5 ships explicit-import regardless of probe outcome — no Dorico-side scan-behavior dependency, deterministic across Dorico versions and user environments. Auto-discovery is a v1.6 polish candidate.
    6. **`<pluginNames>` skipped this milestone (D-02 rationale).** The shipped `.doricolib` does not populate the `<pluginNames>` element on the `<ExpressionMapDefinition>`. User picks "Ouaricon VST3 Note Expression" manually from the Endpoints dropdown after the one-time import. Schema verification cost (verifying the exact element name and entry format Dorico expects) was not paid this milestone. v1.6 revival candidate; if revisited, ship both prod + dev names (16 entries) so dev installs auto-suggest too.
    7. **Microtonality method invariant (Landmine 3).** `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` is the load-bearing setting. `kAuto` falls back to pitch-bend or VST2 detune (neither reaches a JUCE VST3); `kPitchBend` does not propagate per-note in the way Dorico needs. The shipped `.doricolib` pins `kVST3NoteExpression`.

    **H2 Section 4 — `## Troubleshooting Signatures` (DOCS-04)**

    Cover (250-400 words). Symptom-cause table format.

    | Symptom | Cause | Fix |
    |---------|-------|-----|
    | Quarter-sharp plays at exactly 277.18 Hz (next semitone) instead of ~269 Hz | User did not assign "Ouaricon VST3 Note Expression" to the plugin channel; map exists in library but isn't bound | `Play → Endpoints → <plugin channel> → Expression Map` dropdown → select "Ouaricon VST3 Note Expression" |
    | "Ouaricon VST3 Note Expression" not in Endpoints dropdown | User skipped the one-time Library Manager Import | `Library → Library Manager → Import…` from `~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib` (or `%APPDATA%\Ouaricon\Microtonal Suite\` on Windows) |
    | Library Manager Import fails with "Error opening file: invalid file format" | The `.doricolib` is a kScoreLibrary fragment, not a full 48-container library bundle (the v2 defect; should not occur with the v3 shipped asset) | Verify `xmllint --xpath 'count(/kScoreLibrary/*)' file` returns 48; re-author from the HALion Sonic factory skeleton per D-03 |
    | Expression map appears in dropdown but quarter-sharp still plays at semitone | (a) Plugin advertises wrong CID under Dorico's name+CID match; OR (b) plugin's NEC handshake broken; OR (c) `microtonalPlaybackMethod` regressed to `kAuto`/`kPitchBend` | Verify `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` in the `.doricolib` (grep); rebuild plugin and verify `moduleinfo.json` Audio Module Class CID matches Dorico's expectation; for dev/prod CID variance, verify `OUARICON_DEV_SUFFIX` is consistent between build and install |
    | All 8 plugins missing the suite asset on a fresh install | Per-plugin packaging script did not consume the updated shared `pkg-creation.md` / `inno-template.iss`; the plugin was packaged with an old template | Rebuild PKG/EXE for the affected plugin; verify shared template files contain the Microtonal Suite block (per Plan 25-02 Tasks 1+2 acceptance criteria) |
    | Suite asset landed but file is owned by `root` on macOS | Postinstall script's `chown -R "$ACTUAL_USER:staff"` step missing or failed | Verify `pkg-creation.md` Section 4b heredoc contains the chown for `$USER_HOME/Library/Application Support/Ouaricon`; rerun installer; check Console.app for postinstall errors |
    | Auto-discovery probe (Wave 0) was PASS but v1.5 still ships explicit-import | This is by design (D-01); the probe is informational only; v1.5 ships explicit-import to keep behavior deterministic across Dorico versions. Auto-discovery is a v1.6 candidate | No fix; expected behavior. Log the probe result in `25-01-WAVE-0-VERIFICATION.md` for v1.6 reference |

    Tail with a brief paragraph: "When in doubt, the canonical reference asset for verifying schema correctness is the v3 shipped file at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`. `xmllint --noout` and `xmllint --xpath 'count(/kScoreLibrary/*)'` are the two-line schema sanity check."

    Stage as a single atomic commit `docs(25-03): author internal microtonal-dorico-integration reference (DOCS-01..05)`.
  </action>
  <verify>
    <automated>
      test -f research/microtonal-dorico-integration.md && \
      head -25 research/microtonal-dorico-integration.md | grep -q '^title:' && \
      head -25 research/microtonal-dorico-integration.md | grep -q '^audience: internal-dev-only' && \
      grep -q '^## Module Architecture' research/microtonal-dorico-integration.md && \
      grep -q '^## Canonical Dorico Setup Procedure' research/microtonal-dorico-integration.md && \
      grep -q '^## Host-Side Behavior Quirks' research/microtonal-dorico-integration.md && \
      grep -q '^## Troubleshooting Signatures' research/microtonal-dorico-integration.md && \
      grep -q 'kVST3NoteExpression' research/microtonal-dorico-integration.md && \
      grep -q 'kScoreLibrary' research/microtonal-dorico-integration.md && \
      grep -q 'Library Manager' research/microtonal-dorico-integration.md && \
      grep -q 'noteId' research/microtonal-dorico-integration.md && \
      grep -q 'Ouaricon::NoteExpression' research/microtonal-dorico-integration.md && \
      grep -q 'invalid file format' research/microtonal-dorico-integration.md && \
      grep -q '269' research/microtonal-dorico-integration.md && \
      ! grep -q 'PlaybackTemplateSpecs\|dorico_pt\|Default Library Additions\|DefaultLibraryAdditions' research/microtonal-dorico-integration.md && \
      test "$(wc -l < research/microtonal-dorico-integration.md)" -ge 200
    </automated>
  </verify>
  <acceptance_criteria>
    - File `research/microtonal-dorico-integration.md` exists
    - Front-matter contains: `title:`, `created:`, `last_verified:`, `domain: dsp`, `type: reference`, `audience: internal-dev-only`, and `dorico_version`
    - Front-matter `audience` field is exactly `internal-dev-only` (DOCS-05 boundary enforced)
    - File contains all 4 H2 headings, exactly: `## Module Architecture`, `## Canonical Dorico Setup Procedure`, `## Host-Side Behavior Quirks`, `## Troubleshooting Signatures`
    - DOCS-01 section references both `modules/tuning/note-expression/cpp/NoteExpression.h` and `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` paths
    - DOCS-01 section mentions namespace `Ouaricon::NoteExpression`, `noteId`, `applyPendingTuning`, AND `g_neUpdate` (or `g_neQuery`) — the dispatch-slot pattern
    - DOCS-02 section contains the exact menu path `Library → Library Manager → Import…` (or `Import...`)
    - DOCS-02 section contains both install paths: `~/Library/Application Support/Ouaricon/Microtonal Suite/` AND `%APPDATA%\Ouaricon\Microtonal Suite\`
    - DOCS-02 section mentions `Play → Endpoints` and `Expression Map` dropdown for per-channel assignment
    - DOCS-02 section mentions ~269 Hz (verification frequency)
    - DOCS-03 section contains a subsection or paragraph mentioning the kScoreLibrary 48-container schema requirement AND the v2 defect (cd2c2c6 fragment)
    - DOCS-03 section contains a subsection on rationale for explicit-import (D-01)
    - DOCS-03 section contains a subsection on rationale for skipped `<pluginNames>` (D-02)
    - DOCS-03 section contains the `kVST3NoteExpression` invariant (Landmine 3)
    - DOCS-04 section contains a symptom-cause table with at least 5 rows
    - DOCS-04 contains the symptom string `invalid file format` (kScoreLibrary defect signature)
    - DOCS-04 contains the symptom of "expression map appears in dropdown but quarter-sharp plays at semitone"
    - File does NOT contain Path A residue: `PlaybackTemplateSpecs`, `dorico_pt`, `Default Library Additions`, `DefaultLibraryAdditions`
    - File total line count >= 200 (sanity: substantive content vs stub)
    - Atomic commit `docs(25-03): author internal microtonal-dorico-integration reference (DOCS-01..05)` modifies only this file
  </acceptance_criteria>
  <done>
    Single combined developer-reference doc exists at `research/microtonal-dorico-integration.md` with all 4 H2 sections covering DOCS-01..04, DOCS-05 honored via `audience: internal-dev-only` front-matter, no Path A residue, atomic commit landed.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Repo → developer (internal docs) | The doc is in-repo; no external distribution boundary. Audience is internal developers (us, v1.6+ implementers). |
| Internal docs → future end-user manual | DOCS-05 boundary: this doc is NOT shipped to end users this milestone. Sales-website manual authoring (FUT-06) consumes this doc as raw material. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-03-01 | Information Disclosure | Internal doc could accidentally reveal plugin GUIDs, build-host paths, or other sensitive build-pipeline details | mitigate | Doc references only repo-relative paths and well-known canonical install destinations. No CIDs, no secrets, no `/Users/<name>/` paths. The CID-free Path B asset (D-02) means this doc has no plugin-GUID exposure surface. |
| T-25-03-02 | Tampering | Doc could be edited to misdirect future implementers (e.g., wrong invariants documented) | mitigate | Acceptance criteria pin every load-bearing string (`kVST3NoteExpression`, `kScoreLibrary`, exact menu paths, exact install paths, ~269 Hz). Grep verification at task-completion time catches drift. Future PRs can re-verify the same gates. |
| T-25-03-03 | Repudiation | Doc author claims could later be disputed | accept | Standard git history covers authorship. The doc cites verifiable artifacts (file paths, commit hashes, the `/tmp/` reference asset) rather than personal recollection. |
| T-25-03-04 | Spoofing | Doc could be moved or renamed accidentally | accept | DOCS-05 specifies path `research/microtonal-dorico-integration.md`. Build/CI does not load this file at runtime; misplacement does not affect shipped behavior, only future implementer discoverability. |

**Severity:** All threats are LOW severity (internal documentation surface). All MEDIUM-or-higher threats mitigated by acceptance-criteria string-pinning. No HIGH severity threats.

</threat_model>

<verification>
- File exists at `research/microtonal-dorico-integration.md`
- Front-matter audience = `internal-dev-only` (DOCS-05 honored)
- All 4 H2 sections present (DOCS-01..04)
- DOCS-01 grounds in actual module source paths (NoteExpression.h, NoteExpression_VST3.cpp) and names load-bearing types
- DOCS-02 lists exact menu paths and exact install paths for Path B
- DOCS-03 covers the kScoreLibrary schema requirement, explicit-import rationale, skipped `<pluginNames>` rationale, and the kVST3NoteExpression invariant
- DOCS-04 includes the v3-relevant symptom-cause table with at least 5 entries (including invalid-file-format and assignment-not-bound)
- No Path A residue (`PlaybackTemplateSpecs`, `dorico_pt`, `Default Library Additions`, `DefaultLibraryAdditions`)
- File >= 200 lines (substantive content)
</verification>

<success_criteria>
1. `research/microtonal-dorico-integration.md` is the single combined doc covering DOCS-01..04 in 4 H2 sections.
2. Audience is `internal-dev-only` per DOCS-05; no end-user manual prose, no quickstart copy.
3. All v3-relevant content is documented: Path B import flow (DOCS-02), kScoreLibrary 48-container requirement (DOCS-03), explicit-import rationale (DOCS-03), skipped `<pluginNames>` rationale (DOCS-03), invalid-file-format troubleshooting signature (DOCS-04).
4. Module-architecture section grounds in actual code paths and names load-bearing types from `NoteExpression.h` / `NoteExpression_VST3.cpp`.
5. Atomic commit shipped.
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-03-SUMMARY.md` documenting:
- The 4 H2 sections authored (DOCS-01..04) with brief content inventory
- DOCS-05 boundary honored (file location, audience tag, no end-user manual prose)
- Single atomic commit reference
- Cross-references to 25-CONTEXT.md decisions captured (D-01, D-02, D-03 rationale all surfaced in DOCS-03)
</output>
