---
phase: 25-package-docs
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap
  - modules/tuning/note-expression/resources/README-doricoexpmap.txt
  - modules/tuning/note-expression/module.cmake
  - modules/tuning/note-expression/install-doricoexpmap.cmake.in
  - modules/tuning/note-expression/module.yaml
  - modules/tuning/note-expression/README.md
  - modules/registry.yaml
  - plugins/O-Lyrica/dist/.gitkeep
autonomous: false
requirements: [INST-01, INST-02]
must_haves:
  truths:
    - "Canonical Ouaricon-VST3-NoteExpression.doricoexpmap exists at modules/tuning/note-expression/resources/ with Microtonality field set to the literal string 'VST3 Note Expression'"
    - "The note-expression module owns the resource — module.cmake stages it via CMake install() rules with per-platform branches (APPLE → ~/Library/Application Support/Ouaricon/Expression Maps/ + ~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/; WIN32 → %APPDATA%\\Ouaricon\\Expression Maps\\ + %APPDATA%\\Steinberg\\Dorico [N]\\Expression Maps\\User\\)"
    - "Module version bumped to 1.1.0 in module.yaml and modules/registry.yaml; both files contain a 1.1.0 changelog entry naming the canonical .doricoexpmap and dual-write install behavior"
    - "Module README appended with a section documenting the canonical .doricoexpmap, the install destinations, and that consumer plugins inherit the resource via ouaricon_add_module(<Plugin> note-expression) — no per-plugin work required"
    - "End-to-end canary install proves the dual-write pipeline: cmake --install run for one consumer (O-Lyrica) lands the .doricoexpmap at BOTH the Ouaricon shared-resources path AND the latest detected Dorico user expression-maps scan path on macOS"
    - "Dorico opens, finds 'Ouaricon VST3 Note Expression' in the picker without manual import, and a quarter-sharp C4 played via O-Lyrica lands at +50¢ (~269.29 Hz), with no attack zipper, NE correlated by noteId on a polyphonic chord (D-07 3-point gate)"
  artifacts:
    - path: "modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap"
      provides: "Canonical Dorico expression map; single source of truth across all 8 plugin installers"
      contains: "Microtonality field set to 'VST3 Note Expression'"
    - path: "modules/tuning/note-expression/resources/README-doricoexpmap.txt"
      provides: "Plain-text README emitted alongside the .doricoexpmap explaining purpose, install paths, manual-import fallback (INST-04 fallback)"
    - path: "modules/tuning/note-expression/module.cmake"
      provides: "Module-side install() rules for the .doricoexpmap and README — fires once per consumer at install/package time"
      contains: "install(FILES, install(SCRIPT, configure_file, if(APPLE), elseif(WIN32)"
    - path: "modules/tuning/note-expression/install-doricoexpmap.cmake.in"
      provides: "configure_file template for the install-time Dorico version probe + scan-path copy (separates configure-time from install-time concerns; replaces fragile install(CODE \"...\") string escaping)"
      contains: "foreach(_v 6 5 4)"
    - path: "modules/tuning/note-expression/module.yaml"
      provides: "Module metadata bumped to 1.1.0 with resources entry and changelog"
      contains: "version: 1.1.0"
    - path: "modules/registry.yaml"
      provides: "Registry note-expression entry bumped to 1.1.0"
      contains: "name: note-expression"
    - path: "modules/tuning/note-expression/README.md"
      provides: "Consumer-facing docs updated to describe the .doricoexpmap install behavior"
  key_links:
    - from: "ouaricon_add_module(<Plugin> note-expression)"
      to: "modules/tuning/note-expression/module.cmake install() rules"
      via: "OuariconModules.cmake auto-inclusion of module.cmake (line 121-124 of OuariconModules.cmake)"
      pattern: "include\\(\"\\$\\{MODULE_DIR\\}/module\\.cmake\"\\)"
    - from: "module.cmake install() rule"
      to: "~/Library/Application Support/Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap (macOS)"
      via: "CMake install(FILES … DESTINATION …) at install time"
      pattern: "install\\(FILES.*Ouaricon-VST3-NoteExpression\\.doricoexpmap"
    - from: "module.cmake install(SCRIPT) generated from install-doricoexpmap.cmake.in via configure_file"
      to: "~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/Ouaricon-VST3-NoteExpression.doricoexpmap (macOS)"
      via: "Generated install script with descending Dorico version probe (6, 5, 4)"
      pattern: "Steinberg/Dorico"
---

<objective>
Author the canonical `Ouaricon-VST3-NoteExpression.doricoexpmap` from scratch (D-01, D-02, INST-01) and wire the module-side install pipeline (D-04, D-05, D-06, INST-02) so that any plugin consuming `note-expression` via `ouaricon_add_module()` automatically inherits the dual-write resource installation. Bump the module from 1.0.0 → 1.1.0 (additive resource surface) in both `module.yaml` and `modules/registry.yaml`. Append the consumer README. Prove the entire pipeline end-to-end on O-Lyrica (Phase 23 reference consumer) with a real Dorico quarter-sharp smoke test.

Purpose: Establish the canonical asset, the module-side ownership, and the dual-write install plumbing that Plan 25-02 will sweep across the other 7 plugins. This plan is the foundation — without it, 25-02 has nothing to bundle.

Output: One new resource file, one new resource README, one extended `module.cmake` with install() rules + configure_file()-based install script, version bump on `module.yaml` + `modules/registry.yaml`, README append, and a passing manual Dorico smoke gate on O-Lyrica.
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
@.planning/phases/24-propagate/24-CONTEXT.md
@.planning/phases/23-extract/23-CONTEXT.md
@.claude/skills/spike-findings-VST-development/SKILL.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@modules/tuning/note-expression/module.cmake
@modules/tuning/note-expression/module.yaml
@modules/tuning/note-expression/README.md
@modules/cmake/OuariconModules.cmake
@modules/registry.yaml
@plugins/O-Lyrica/CMakeLists.txt
@CLAUDE.md
</context>

<interfaces>
<!-- Key contracts the executor needs. Extracted from codebase. Use these directly — no codebase exploration needed. -->

From modules/cmake/OuariconModules.cmake (lines 30, 50, 120-124):
```cmake
function(ouaricon_add_module TARGET_NAME MODULE_NAME)
    # … finds MODULE_DIR by category search …
    message(STATUS "[Ouaricon] Adding module '${MODULE_NAME}' from ${MODULE_DIR}")
    # … adds C++ sources to target (top-level cpp/) …
    # … adds per-format sources (cpp/<format>/) …
    # … copies JS files …

    # Module-supplied CMake hook (optional, backward-compatible)
    if(EXISTS "${MODULE_DIR}/module.cmake")
        message(STATUS "[Ouaricon]   Including ${MODULE_NAME}/module.cmake")
        include("${MODULE_DIR}/module.cmake")
    endif()
    # …
endfunction()
```
The hook runs in the consumer plugin's CMake scope. `${MODULE_DIR}` is set to the absolute path of the module directory. `${TARGET_NAME}` is the JUCE plugin target. `install(...)` rules placed in `module.cmake` register against the install component of the consumer (each plugin gets its own install spec when packaged).

From modules/tuning/note-expression/module.cmake (current — 42 lines; extends, does not redesign):
```cmake
# Locates JUCE tree, greps for JUCE-NE-PATCH marker, FATAL_ERROR if missing.
# This file is included by ouaricon_add_module() at configure time, in the
# consumer plugin's CMakeLists scope.
```

From modules/tuning/note-expression/module.yaml (lines 1-7, 51-72):
```yaml
name: note-expression
version: 1.0.0           # → 1.1.0
# …
sources:
  cpp:
    - cpp/NoteExpression.h
# (Currently no `resources:` field — add it for 1.1.0)
used_by:
  - plugin: OLyrica
    version: 2.3.0
changelog:
  - version: 1.0.0
    date: 2026-04-24
    changes: [...]
```

From modules/registry.yaml (lines 251-286):
```yaml
- name: note-expression
  path: tuning/note-expression
  version: 1.0.0           # → 1.1.0
  # …
  used_by:
    - plugin: OLyrica
      version: 2.3.0
    - plugin: O-Bells
      version: 4.1.0
    # … 6 more (8 total)
```

Spike-finding invariant (THE thing the .doricoexpmap encodes — from `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` Landmine 3):
> "Default expression maps (NotePerformer, HSSE, HALion, etc.) route microtones to VST2 detune or pitch bend — neither reaches our VST3 plugin. … Set the duplicate's Microtonality dropdown to 'VST3 Note Expression'."

Dorico expression-map XML schema (Steinberg, Dorico 5/6): the file is XML with a top-level `<expressionMap>` element. The microtonality switch is encoded in a `<microtonalityType>` (or equivalent) element whose text value is the canonical Dorico string `VST3 Note Expression`. The exact element name and surrounding scaffolding the executor must verify against a reference Dorico expression map (e.g., one Dorico ships in `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/Default/` or by exporting from Dorico itself). The hand-authored file MUST contain the literal string `VST3 Note Expression` somewhere in the XML body — that is the load-bearing invariant Plan 25-02 will grep-verify across all 8 plugins.

CMake configure_file pattern for install scripts (replaces fragile install(CODE "...") string escaping per WARNING #8):
```cmake
# At configure time, expand template variables once:
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install-doricoexpmap.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install-doricoexpmap-${TARGET_NAME}.cmake"
    @ONLY
)
# At install time, run the pre-substituted script:
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-doricoexpmap-${TARGET_NAME}.cmake")
```
Inside the .cmake.in template, `@VAR@` is substituted at configure time, and `$ENV{HOME}` / `$ENV{APPDATA}` evaluate cleanly at install time without shell-escape ambiguity.
</interfaces>

<tasks>

<task type="auto">
  <name>Task 1: Author canonical .doricoexpmap + plain-text README at module resources/</name>
  <files>
    modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap,
    modules/tuning/note-expression/resources/README-doricoexpmap.txt
  </files>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-01, D-02, D-03, D-08 — file scope, microtonality-only minimum, INST-04 README fallback)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (Landmine 3 — THE invariant: Microtonality must be set to "VST3 Note Expression")
    - modules/tuning/note-expression/README.md (existing module README — for tone consistency on the new README-doricoexpmap.txt)
    - .planning/notes/dorico-microtonal-vst-research.md (background — Dorico's 3 wire mechanisms, source for DOCS-02/DOCS-03 in Plan 25-03; ALSO the source for the exact XML schema executor needs)
  </read_first>
  <action>
    **Hard precondition (per WARNING #7) — must be the first sub-step.** Before authoring the .doricoexpmap, the executor MUST have a reference Dorico expression map XML for schema fidelity. Try in priority order:

    ```bash
    # Probe for any factory expression map on this host:
    REFERENCE_MAP=$(ls ~/Library/Application\ Support/Steinberg/Dorico\ */Library/Expression\ Maps/Factory/*.doricoexpmap 2>/dev/null | head -1)
    if [ -z "$REFERENCE_MAP" ]; then
      REFERENCE_MAP=$(ls ~/Library/Application\ Support/Steinberg/Dorico\ */Expression\ Maps/Default/*.doricoexpmap 2>/dev/null | head -1)
    fi
    echo "Reference map: $REFERENCE_MAP"
    ```

    If `$REFERENCE_MAP` is empty AND WebFetch is unavailable (no network or tool blocked at execute time), HALT and ask the user to provide one factory expression map at execute time (paste path or upload). Do NOT attempt to invent the XML schema from scratch — Dorico's element names are non-obvious and getting them wrong causes silent picker rejection.

    If `$REFERENCE_MAP` exists, read its full XML structure to lift the schema scaffolding (root element name, namespace, version attributes, microtonality element name).

    If `$REFERENCE_MAP` is empty AND WebFetch IS available, fetch the Steinberg Dorico Expression Map XML schema reference (Dorico 6 docs) via WebFetch from https://steinberg.help/dorico/v6/en/dorico/topics/expression_maps/ or a similar URL and pin the exact element names used.

    ── End precondition ──

    Create `modules/tuning/note-expression/resources/` directory (NEW — does not exist; the `resources/` subdirectory is part of D-04's "module owns the asset" extension).

    Hand-author `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` from scratch per D-01. The file MUST be valid Dorico expression map XML with these load-bearing properties:
    1. Top-level Dorico expression map XML scaffolding (the exact root element name, namespace, and version attributes are Dorico's — match the reference factory map's structure verbatim; do NOT invent element names).
    2. `<name>` element value: `Ouaricon VST3 Note Expression` (this is what shows up in Dorico's expression-map picker; the literal string user sees).
    3. The Microtonality field MUST be set to the literal string `VST3 Note Expression`. The exact element name is Dorico-defined (likely `<microtonalityType>` or `<microtonality>` or similar — match the reference factory map's element). What is non-negotiable is that the literal string `VST3 Note Expression` appears as the value of whatever element Dorico reads to determine microtonality routing. This is THE invariant from spike-findings Landmine 3 — without this exact string, Dorico defaults to pitch-bend routing for non-Steinberg VST3s and microtonal playback breaks silently.
    4. Microtonality-only minimum scope per D-02: NO articulation switches (no staccato/legato/dynamics entries). The expression map should be the smallest valid Dorico XML that sets the microtonality routing to VST3 Note Expression. Use defaults for everything else.
    5. Add a top-of-file XML comment: `<!-- Ouaricon Audio canonical Dorico expression map. Single source of truth at modules/tuning/note-expression/resources/. Bundled by all 8 v1.5 plugins (O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant). DO NOT edit installed copies — edit the canonical file in the module repo. -->`

    Create `modules/tuning/note-expression/resources/README-doricoexpmap.txt` (plain text, INST-04 fallback per D-08). Tone: technical-developer-style, NOT marketing. Content (use exactly these section headings):

    ```
    Ouaricon VST3 Note Expression — Dorico Expression Map
    ======================================================

    PURPOSE
    -------
    This file (Ouaricon-VST3-NoteExpression.doricoexpmap) configures Dorico to
    route per-note microtonal pitch deltas as VST3 Note Expression events to
    Ouaricon plugins. Without it, Dorico defaults to pitch-bend or VST2 detune
    routing for non-Steinberg VST3s, and microtonal accidentals play as plain
    12-TET (broken playback, no error).

    INSTALL LOCATIONS
    -----------------
    The plugin installer wrote this file to two paths:
      • Editable canonical copy:
          macOS:   ~/Library/Application Support/Ouaricon/Expression Maps/
          Windows: %APPDATA%\Ouaricon\Expression Maps\
      • Dorico auto-scan path (for the picker to find it without manual import):
          macOS:   ~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/
          Windows: %APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\
    where [N] is the latest Dorico major version detected at install time.

    MANUAL IMPORT FALLBACK
    ----------------------
    If the file does not appear in Dorico's expression-map picker
    (Library → Expression Maps…), import it manually:
      1. Dorico → Library → Expression Maps…
      2. Click the Import (folder) icon at the bottom of the list
      3. Navigate to the canonical copy path above and select
         Ouaricon-VST3-NoteExpression.doricoexpmap
      4. Save
    Then assign it to your Ouaricon plugin's channel:
      • Play → Endpoint Setup → expression-map dropdown for the plugin's channel
        → select "Ouaricon VST3 Note Expression"

    SOURCE OF TRUTH
    ---------------
    This file is generated from the canonical copy in the Ouaricon module repo:
      modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap
    Edits to installed copies are overwritten by the next plugin installer run.
    Edit the canonical file in the repo if changes are needed.

    SUPPORTED PLUGINS (v1.5 cohort)
    -------------------------------
      O-Lyrica, O-Bells, O-IntonationPad, O-Prism,
      O-Wind, O-Reed, O-Bowed, O-Formant
    ```

    Per D-01 and D-02: the .doricoexpmap is hand-authored from scratch (no third-party template provenance). Microtonality-only minimum (no articulation switches).
  </action>
  <verify>
    <automated>
      test -f modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      test -f modules/tuning/note-expression/resources/README-doricoexpmap.txt &&
      grep -q 'VST3 Note Expression' modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      grep -q 'Ouaricon Audio canonical Dorico expression map' modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      grep -q 'Ouaricon VST3 Note Expression' modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      python3 -c "import xml.etree.ElementTree as ET; ET.parse('modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap')" &&
      grep -q 'PURPOSE' modules/tuning/note-expression/resources/README-doricoexpmap.txt &&
      grep -q 'MANUAL IMPORT FALLBACK' modules/tuning/note-expression/resources/README-doricoexpmap.txt &&
      grep -q 'O-Lyrica, O-Bells, O-IntonationPad, O-Prism' modules/tuning/note-expression/resources/README-doricoexpmap.txt
    </automated>
  </verify>
  <acceptance_criteria>
    - File `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` exists and is well-formed XML (parses with `python3 -c "import xml.etree.ElementTree as ET; ET.parse(…)"` exit 0).
    - The literal string `VST3 Note Expression` appears at least once in the .doricoexpmap (filtered grep on non-comment lines: `grep -v '^<!--' modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap | grep -c 'VST3 Note Expression'` returns ≥1).
    - The display name `Ouaricon VST3 Note Expression` appears at least once (grep returns ≥1 match).
    - The provenance comment `Ouaricon Audio canonical Dorico expression map` is present (grep returns 1 match).
    - File `modules/tuning/note-expression/resources/README-doricoexpmap.txt` exists with the section headings `PURPOSE`, `INSTALL LOCATIONS`, `MANUAL IMPORT FALLBACK`, `SOURCE OF TRUTH`, `SUPPORTED PLUGINS (v1.5 cohort)` (grep each).
    - README lists all 8 supported plugins by name (grep `O-Lyrica, O-Bells, O-IntonationPad, O-Prism` → 1 match; grep `O-Wind, O-Reed, O-Bowed, O-Formant` → 1 match).
    - No articulation switches (staccato/legato/dynamics) present in the .doricoexpmap (grep -i staccato/legato/dynamics → 0 matches each — microtonality-only per D-02).
    - The hard precondition was satisfied: either a reference factory map was located on disk, or WebFetch fetched Dorico schema docs, or the user provided a reference map at execute time. If none of the three was possible, the task halted before authoring (no broken XML committed).
  </acceptance_criteria>
  <done>The canonical .doricoexpmap and its plain-text README are committed at `modules/tuning/note-expression/resources/` with the load-bearing 'VST3 Note Expression' string and the file naming convention specified by D-03.</done>
</task>

<task type="auto">
  <name>Task 2a: Extend module.cmake with dual-write install() rules using configure_file()-based install script (D-04 — the meat)</name>
  <files>
    modules/tuning/note-expression/module.cmake,
    modules/tuning/note-expression/install-doricoexpmap.cmake.in
  </files>
  <read_first>
    - modules/tuning/note-expression/module.cmake (current 42-line patch-marker check — extension point per D-04)
    - modules/cmake/OuariconModules.cmake lines 120-124 (`if(EXISTS "${MODULE_DIR}/module.cmake") include(...) endif()` — confirms the module.cmake hook fires per-consumer at configure time, which is when install() rules must be registered)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-04, D-05, D-06, D-07 — module ownership, dual-write paths, Dorico version targeting)
    - CMake docs for `install(FILES ... DESTINATION ...)`, `install(SCRIPT ...)`, `configure_file(... @ONLY)`
  </read_first>
  <action>
    Per WARNING #8 from the checker review: replace the originally-planned `install(CODE "...")` block (which had fragile shell-style escaping for nested `$ENV{...}` references) with a `configure_file()` + `install(SCRIPT)` pattern. This separates configure-time substitution from install-time evaluation cleanly.

    ── PART A: Create the install-script template ──

    Create NEW file `modules/tuning/note-expression/install-doricoexpmap.cmake.in`:

    ```cmake
    # ==============================================================================
    # install-doricoexpmap.cmake.in — install-time Dorico version probe + scan-path copy
    #
    # This template is processed by configure_file() at configure time (per consumer
    # plugin), with @VAR@ placeholders substituted. The resulting concrete .cmake
    # script is run at install time via install(SCRIPT ...). $ENV{HOME} and
    # $ENV{APPDATA} evaluate at install time — they are NOT substituted by
    # configure_file() because they are not @-wrapped.
    #
    # Configure-time substitutions:
    #   @NE_DORICOEXPMAP@        — absolute path to the canonical .doricoexpmap source
    #   @NE_DORICOEXPMAP_NAME@   — basename of the .doricoexpmap (filename only)
    #   @NE_TARGET_NAME@         — consumer plugin target name (for log lines)
    # ==============================================================================

    # Resolve Dorico user expression-maps directory at install time.
    # Probe Dorico 6, 5, 4 in descending order. First hit wins.
    set(_OUA_DORICO_USER_DIR "")
    set(_OUA_DORICO_VERSION "")

    if(APPLE)
        foreach(_v 6 5 4)
            set(_dorico_root "$ENV{HOME}/Library/Application Support/Steinberg/Dorico ${_v}")
            if(IS_DIRECTORY "${_dorico_root}")
                set(_OUA_DORICO_USER_DIR "${_dorico_root}/Expression Maps/User")
                set(_OUA_DORICO_VERSION "${_v}")
                break()
            endif()
        endforeach()
    elseif(WIN32)
        foreach(_v 6 5 4)
            set(_dorico_root "$ENV{APPDATA}/Steinberg/Dorico ${_v}")
            if(IS_DIRECTORY "${_dorico_root}")
                set(_OUA_DORICO_USER_DIR "${_dorico_root}/Expression Maps/User")
                set(_OUA_DORICO_VERSION "${_v}")
                break()
            endif()
        endforeach()
    endif()

    if(_OUA_DORICO_USER_DIR)
        file(MAKE_DIRECTORY "${_OUA_DORICO_USER_DIR}")
        file(COPY "@NE_DORICOEXPMAP@" DESTINATION "${_OUA_DORICO_USER_DIR}")
        message(STATUS "[note-expression] (@NE_TARGET_NAME@) Dorico expression map installed to Dorico ${_OUA_DORICO_VERSION} scan path: ${_OUA_DORICO_USER_DIR}")
    else()
        message(STATUS "[note-expression] (@NE_TARGET_NAME@) No Dorico install detected — skipping Dorico-scan write (manual import fallback applies; see README-doricoexpmap.txt)")
    endif()
    ```

    ── PART B: APPEND to module.cmake (do not replace existing content) ──

    Keep the existing 42-line JUCE-NE-PATCH marker check verbatim. APPEND the following block AFTER the existing `message(STATUS "[note-expression] JUCE-NE-PATCH markers verified in ${_NE_JUCE_ROOT}")` line:

    ```cmake
    # ==============================================================================
    # Phase 25 (D-04, D-05, D-06): Canonical Dorico expression map install rules.
    # Fires per-consumer when ouaricon_add_module(<Plugin> note-expression) is
    # called. Each plugin's installer (PKG on macOS, EXE on Windows) inherits these
    # install() rules and dual-writes the .doricoexpmap to (a) Ouaricon shared
    # resources path and (b) Dorico's user expression-maps scan path.
    #
    # The canonical asset is owned by the module — no per-plugin copy of the file
    # exists in any plugin's source tree. Phase 25 INST-01..04.
    # ==============================================================================

    set(_NE_RESOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/resources")
    set(_NE_DORICOEXPMAP "${_NE_RESOURCES_DIR}/Ouaricon-VST3-NoteExpression.doricoexpmap")
    set(_NE_DORICOEXPMAP_README "${_NE_RESOURCES_DIR}/README-doricoexpmap.txt")
    set(_NE_DORICOEXPMAP_NAME "Ouaricon-VST3-NoteExpression.doricoexpmap")

    if(NOT EXISTS "${_NE_DORICOEXPMAP}")
        message(FATAL_ERROR
            "[note-expression] Canonical Dorico expression map not found:\n"
            "  ${_NE_DORICOEXPMAP}\n"
            "Module is at version 1.1.0 which requires the resources/ asset. "
            "Re-run from a clean checkout or restore the file from git.")
    endif()

    # Ouaricon shared resources path — always written, regardless of Dorico
    # presence. This is the editable canonical user copy (D-05).
    if(APPLE)
        install(FILES
            "${_NE_DORICOEXPMAP}"
            "${_NE_DORICOEXPMAP_README}"
            DESTINATION "$ENV{HOME}/Library/Application Support/Ouaricon/Expression Maps"
            COMPONENT note-expression-resources
        )
    elseif(WIN32)
        install(FILES
            "${_NE_DORICOEXPMAP}"
            "${_NE_DORICOEXPMAP_README}"
            DESTINATION "$ENV{APPDATA}/Ouaricon/Expression Maps"
            COMPONENT note-expression-resources
        )
    else()
        message(STATUS "[note-expression] Non-macOS, non-Windows host — Dorico expression map install skipped (Dorico is not supported on Linux)")
    endif()

    # Dorico version targeting (D-07): probe for installed Dorico major versions
    # at install time, prefer the latest detected. Configure_file() expands
    # @-wrapped placeholders at configure time (per consumer plugin), producing a
    # concrete script. install(SCRIPT) runs it at install time, where $ENV{HOME}
    # and $ENV{APPDATA} resolve cleanly without escape-string ambiguity.
    # (Per-consumer file naming prevents collisions when multiple consumers
    # configure into the same build tree.)
    if(APPLE OR WIN32)
        set(NE_DORICOEXPMAP "${_NE_DORICOEXPMAP}")
        set(NE_DORICOEXPMAP_NAME "${_NE_DORICOEXPMAP_NAME}")
        set(NE_TARGET_NAME "${TARGET_NAME}")
        set(_NE_GENERATED_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-doricoexpmap-${TARGET_NAME}.cmake")
        configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/install-doricoexpmap.cmake.in"
            "${_NE_GENERATED_SCRIPT}"
            @ONLY
        )
        install(SCRIPT "${_NE_GENERATED_SCRIPT}" COMPONENT note-expression-resources)
    endif()

    message(STATUS "[note-expression] Canonical .doricoexpmap install rules registered for ${TARGET_NAME} (module v1.1.0, INST-01..04)")
    ```

    Verification preview (not the formal gate — that's Task 3a): after writing both files, configure a single plugin (`cmake -S . -B build/test-25-01 -DOUARICON_DEV_SUFFIX=`) and confirm:
    - `build/test-25-01/install-doricoexpmap-OLyrica.cmake` exists (the configure_file() output)
    - The generated file has @NE_DORICOEXPMAP@ literally substituted to an absolute path (no remaining `@…@` placeholders)
    - The generated file still contains literal `$ENV{HOME}` and `$ENV{APPDATA}` (these resolve at install time, not configure time)
  </action>
  <verify>
    <automated>
      test -f modules/tuning/note-expression/module.cmake &&
      test -f modules/tuning/note-expression/install-doricoexpmap.cmake.in &&
      grep -v '^#' modules/tuning/note-expression/module.cmake | grep -q 'JUCE-NE-PATCH markers verified' &&
      grep -q 'Phase 25 (D-04, D-05, D-06)' modules/tuning/note-expression/module.cmake &&
      grep -q 'install(FILES' modules/tuning/note-expression/module.cmake &&
      grep -q 'configure_file' modules/tuning/note-expression/module.cmake &&
      grep -q 'install(SCRIPT' modules/tuning/note-expression/module.cmake &&
      grep -q 'if(APPLE)' modules/tuning/note-expression/module.cmake &&
      grep -q 'elseif(WIN32)' modules/tuning/note-expression/module.cmake &&
      grep -q 'Ouaricon/Expression Maps' modules/tuning/note-expression/module.cmake &&
      grep -q 'COMPONENT note-expression-resources' modules/tuning/note-expression/module.cmake &&
      grep -q 'foreach(_v 6 5 4)' modules/tuning/note-expression/install-doricoexpmap.cmake.in &&
      grep -q '@NE_DORICOEXPMAP@' modules/tuning/note-expression/install-doricoexpmap.cmake.in &&
      grep -q '@NE_TARGET_NAME@' modules/tuning/note-expression/install-doricoexpmap.cmake.in &&
      grep -q 'Steinberg/Dorico' modules/tuning/note-expression/install-doricoexpmap.cmake.in
    </automated>
  </verify>
  <acceptance_criteria>
    - `modules/tuning/note-expression/module.cmake` retains the existing 42-line JUCE-NE-PATCH marker check (filtered grep `JUCE-NE-PATCH markers verified` on non-comment lines returns 1 match — comment-line filter prevents the self-invalidating-grep-gate antipattern).
    - `module.cmake` has the new Phase 25 install block (grep `Phase 25 (D-04, D-05, D-06)` returns 1 match).
    - `module.cmake` uses the configure_file()-based pattern (grep `configure_file` returns 1 match; grep `install(SCRIPT` returns 1 match) — replaces the originally-planned fragile install(CODE "...") block per WARNING #8.
    - `module.cmake` contains `install(FILES` for the canonical Ouaricon path (grep returns ≥1 match).
    - `module.cmake` has per-platform branches: `if(APPLE)` and `elseif(WIN32)` (each grep returns 1 match).
    - `module.cmake` references the Ouaricon install destination (grep `Ouaricon/Expression Maps` returns ≥1 match).
    - `module.cmake` registers the install component name `note-expression-resources` (grep returns ≥1 match).
    - NEW file `install-doricoexpmap.cmake.in` exists with the descending Dorico version probe loop `foreach(_v 6 5 4)` (grep returns 1 match — present in macOS branch and Windows branch).
    - `install-doricoexpmap.cmake.in` contains @-wrapped placeholders for configure-time substitution: `@NE_DORICOEXPMAP@`, `@NE_TARGET_NAME@` (each grep returns ≥1 match).
    - `install-doricoexpmap.cmake.in` references the Dorico scan path (grep `Steinberg/Dorico` returns ≥1 match).
    - CMake configure of any one consumer plugin (e.g., `cmake -S . -B build/test-25-01-config -DOUARICON_DEV_SUFFIX=`) succeeds with no errors and emits the new STATUS line `[note-expression] Canonical .doricoexpmap install rules registered for OLyrica` — verify by capturing configure output to a log file and grepping.
    - The configured-out script file `build/test-25-01-config/install-doricoexpmap-OLyrica.cmake` exists AND contains zero `@…@` placeholders (grep `@[A-Z_]\+@` returns no matches — all configure-time substitutions resolved) AND still contains literal `$ENV` (resolves at install time).
  </acceptance_criteria>
  <done>
    The module owns the canonical .doricoexpmap end-to-end at the CMake level: `module.cmake` registers per-consumer `install(FILES)` rules for the Ouaricon shared path AND uses `configure_file()` + `install(SCRIPT)` for the install-time Dorico version probe. The fragile `install(CODE "...")` shell-escape pattern is replaced with the cleaner template-based approach (WARNING #8 closed).
  </done>
</task>

<task type="auto">
  <name>Task 2b: Bump module to v1.1.0 in module.yaml + modules/registry.yaml + append README documentation</name>
  <files>
    modules/tuning/note-expression/module.yaml,
    modules/registry.yaml,
    modules/tuning/note-expression/README.md
  </files>
  <read_first>
    - modules/tuning/note-expression/module.yaml (current v1.0.0 — bump target per D-04 spec)
    - modules/registry.yaml lines 251-286 (note-expression registry entry — bump target)
    - modules/tuning/note-expression/README.md (existing consumer docs — append target)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-04 — additive resource surface; semver minor bump rationale)
  </read_first>
  <action>
    This task is pure metadata + documentation update — mechanical, decoupled from the CMake work in Task 2a (per WARNING #10 split rationale). All three file edits commit atomically with Task 2a's CMake edits because they share `files_modified:` membership.

    ── module.yaml updates ──

    Edit `modules/tuning/note-expression/module.yaml`:
    1. Change line 2: `# note-expression module v1.0.0` → `# note-expression module v1.1.0`
    2. Change line 7: `version: 1.0.0` → `version: 1.1.0`
    3. After the existing `sources:` block (around line 51-54), ADD a new top-level `resources:` block:
       ```yaml
       # Resources (Phase 25 INST-01, INST-02 — module owns the canonical Dorico expression map)
       resources:
         - resources/Ouaricon-VST3-NoteExpression.doricoexpmap
         - resources/README-doricoexpmap.txt
       ```
    4. Append a new changelog entry at the END of the changelog block (after the existing 1.0.0 entry):
       ```yaml
         - version: 1.1.0
           date: 2026-04-26
           changes:
             - "Adds canonical Dorico expression map at resources/Ouaricon-VST3-NoteExpression.doricoexpmap (INST-01)"
             - "Adds plain-text README-doricoexpmap.txt as INST-04 fallback for users whose Dorico version directory was not detected at install time"
             - "Extends module.cmake with dual-write install() rules: Ouaricon shared resources path (editable canonical) + Dorico user Expression Maps/User scan path (auto-discovery; latest detected Dorico 4/5/6) per D-04..D-07"
             - "Replaces fragile install(CODE \"...\") shell-escape pattern with configure_file() + install(SCRIPT) using install-doricoexpmap.cmake.in template"
             - "Microtonality-only minimum scope per D-02 — no articulation switches; covers Dorico's expression-map skipped UX trap (spike-findings Landmine 3) for all 8 v1.5 plugins"
             - "Module-side asset ownership extends Phase 23's 'module owns everything' principle from source code/JUCE patch/README to resources (INST-02)"
       ```

    ── modules/registry.yaml updates ──

    Edit `modules/registry.yaml` for the `note-expression` entry (lines 251-286):
    1. Change line 253: `version: 1.0.0` → `version: 1.1.0`
    2. After the existing `provides:` block but BEFORE `dependencies: []`, ADD a `resources:` field:
       ```yaml
         resources:
           - file: resources/Ouaricon-VST3-NoteExpression.doricoexpmap
             description: "Canonical Dorico expression map (Microtonality = VST3 Note Expression)"
           - file: resources/README-doricoexpmap.txt
             description: "Plain-text fallback README explaining install paths and manual import"
       ```

    ── modules/tuning/note-expression/README.md append ──

    APPEND to `modules/tuning/note-expression/README.md` (after the existing `## License` section — or before it; place at the end as a new section). Use this exact text:

    ```markdown

    ## Canonical Dorico Expression Map (v1.1.0+)

    Module v1.1.0 ships a canonical pre-configured Dorico expression map at
    `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`.
    Microtonality is hard-coded to **"VST3 Note Expression"** — the load-bearing
    invariant from spike-findings Landmine 3 (Dorico's `Auto` selection picks
    pitch-bend for non-Steinberg VST3s and silently breaks microtonal playback).

    ### Auto-install on consumer plugins

    No per-plugin code is needed. The module's `module.cmake` registers
    `install()` rules that fire automatically when a plugin consumes the module
    via `ouaricon_add_module(<Plugin> note-expression)`. The plugin's installer
    (PKG on macOS, EXE on Windows) inherits the rules and writes the
    `.doricoexpmap` to two paths on the user's machine:

    | Platform | Editable canonical copy | Dorico auto-scan path |
    |----------|------------------------|------------------------|
    | macOS | `~/Library/Application Support/Ouaricon/Expression Maps/` | `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` |
    | Windows | `%APPDATA%\Ouaricon\Expression Maps\` | `%APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\` |

    `[N]` is the latest Dorico major version detected at install time
    (probed in order: 6, 5, 4). When no Dorico install is detected, the
    auto-scan write is skipped and the file's `README-doricoexpmap.txt`
    explains manual import via `Dorico → Library → Expression Maps…`.

    ### Source of truth

    Edits to installed copies are overwritten by the next plugin installer
    run. The canonical edit point is the file in the module repo:
    `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`.

    ### Supported plugins (v1.5 cohort)

    All 8 v1.5 microtonal-cohort plugins inherit the install rules:
    O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed,
    O-Formant.
    ```
  </action>
  <verify>
    <automated>
      grep -q 'note-expression module v1.1.0' modules/tuning/note-expression/module.yaml &&
      grep -q '^version: 1.1.0' modules/tuning/note-expression/module.yaml &&
      grep -q 'resources:' modules/tuning/note-expression/module.yaml &&
      grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' modules/tuning/note-expression/module.yaml &&
      grep -q '- version: 1.1.0' modules/tuning/note-expression/module.yaml &&
      grep -q 'date: 2026-04-26' modules/tuning/note-expression/module.yaml &&
      awk '/- name: note-expression/,/^  - name:/' modules/registry.yaml | grep -q 'version: 1.1.0' &&
      awk '/- name: note-expression/,/^  - name:/' modules/registry.yaml | grep -q 'resources:' &&
      grep -q '## Canonical Dorico Expression Map' modules/tuning/note-expression/README.md &&
      grep -q 'Auto-install on consumer plugins' modules/tuning/note-expression/README.md &&
      grep -q 'Source of truth' modules/tuning/note-expression/README.md &&
      grep -q 'O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed' modules/tuning/note-expression/README.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `module.yaml` has `version: 1.1.0` (grep at column 0 returns 1 match), the new `resources:` block listing the .doricoexpmap and README, and a new changelog entry with `version: 1.1.0` and `date: 2026-04-26`.
    - `modules/registry.yaml` `note-expression` entry has `version: 1.1.0` (verify with awk-extracted block, then grep) and a `resources:` field listing both files.
    - `README.md` has the new section `## Canonical Dorico Expression Map (v1.1.0+)` with subsections `Auto-install on consumer plugins`, `Source of truth`, `Supported plugins (v1.5 cohort)` (grep each returns ≥1 match).
    - `README.md` lists all 8 plugins by name in a single line (grep `O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed` returns 1 match).
    - All 3 file edits compose cleanly with Task 2a's CMake edits — no merge conflicts, no out-of-order content.
  </acceptance_criteria>
  <done>
    Module v1.1.0 is the version of record across all metadata: `module.yaml`, `modules/registry.yaml`, and the consumer-facing README all reflect the new resources surface and the auto-install behavior. Documentation matches what was implemented in Task 2a.
  </done>
</task>

<task type="auto">
  <name>Task 3a: Build O-Lyrica canary, run cmake --install, verify dual-write physically lands at both macOS paths, run AU regression check (all automated)</name>
  <files>
    plugins/O-Lyrica/dist/.gitkeep
  </files>
  <read_first>
    - CLAUDE.md (Plugin Cache Clearing protocol — mandatory before any AU regression test)
    - plugins/O-Lyrica/CMakeLists.txt (reference consumer; line 80 = `ouaricon_add_module(OLyrica note-expression)`)
    - modules/tuning/note-expression/module.cmake (the install rules just added in Task 2a)
    - modules/tuning/note-expression/install-doricoexpmap.cmake.in (the install-time script template added in Task 2a)
    - modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap (the asset that must land)
  </read_first>
  <action>
    Per D-09, D-10: O-Lyrica is the canary (first proof of the dual-write pipeline end-to-end).

    This task is purely automated — build, install, verify dual-write, AU regression. The Dorico 3-point manual gate is a separate task (Task 3b) per WARNING #2 (Task 3 was too crammed; manual checkpoint cleanly separated from automated work).

    Execute exactly this sequence on macOS (the developer machine — Windows validation is owned by Plan 25-02, not this plan):

    Step 1 — Configure + build O-Lyrica VST3 + AU + Standalone fresh:
    ```bash
    cmake -S . -B build -DOUARICON_DEV_SUFFIX= 2>&1 | tee /tmp/25-01-canary-configure.log
    grep '\[note-expression\] Canonical .doricoexpmap install rules registered for OLyrica' /tmp/25-01-canary-configure.log
    grep '\[note-expression\] JUCE-NE-PATCH markers verified' /tmp/25-01-canary-configure.log
    cd build && ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone 2>&1 | tee /tmp/25-01-canary-build.log
    cd ..
    ```
    Build MUST exit 0. The configure log MUST contain BOTH the new `Canonical .doricoexpmap install rules registered for OLyrica` STATUS line AND the existing `JUCE-NE-PATCH markers verified` line.

    Step 2 — Pre-install cache clear per CLAUDE.md (mandatory):
    ```bash
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/
    rm -rf ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Lyrica.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/O-Lyrica.component
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica.vst3 ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/O-Lyrica.component ~/Library/Audio/Plug-Ins/Components/
    ```

    Step 3 — Pre-install state snapshot (so we can verify the install actually wrote NEW files):
    ```bash
    # Capture pre-state. If the file exists, record its mtime and content hash for comparison.
    rm -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap 2>/dev/null
    rm -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/README-doricoexpmap.txt 2>/dev/null
    # For Dorico scan paths, list (don't delete — preserve user's other expression maps):
    ls -la ~/Library/Application\ Support/Steinberg/Dorico*/Expression\ Maps/User/ 2>/dev/null > /tmp/25-01-pre-dorico-state.txt || true
    # Detect which Dorico version is installed (this is what the install-time probe will pick up):
    ls -d ~/Library/Application\ Support/Steinberg/Dorico\ [4-9] 2>/dev/null | tail -1 > /tmp/25-01-detected-dorico.txt
    cat /tmp/25-01-detected-dorico.txt
    ```

    Step 4 — Run cmake --install for the note-expression-resources component (the formal install path that consumer installers will use):
    ```bash
    cmake --install build --component note-expression-resources --prefix / 2>&1 | tee /tmp/25-01-canary-install.log
    ```
    Note: `--prefix /` because the install() rules use absolute `$ENV{HOME}` paths in the configure_file()-generated install script and absolute `$ENV{HOME}/Library/...` in `install(FILES ... DESTINATION ...)` — the prefix is irrelevant when DESTINATION is absolute.

    The log MUST contain:
    - `Installing: ...Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap` (or the resolved path with $HOME expanded)
    - `Installing: ...Ouaricon/Expression Maps/README-doricoexpmap.txt`
    - Either `[note-expression] (OLyrica) Dorico expression map installed to Dorico [4-9] scan path: …` (if Dorico is installed) OR `[note-expression] (OLyrica) No Dorico install detected — skipping Dorico-scan write`

    Step 5 — Verify dual-write physically landed:
    ```bash
    test -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap && echo "OURICON-PATH: WROTE"
    test -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/README-doricoexpmap.txt && echo "OURICON-README: WROTE"
    grep -q 'VST3 Note Expression' ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap && echo "OURICON-CONTENT: VALID"
    # Dorico path verification — only if a Dorico version was detected:
    DORICO_DIR=$(cat /tmp/25-01-detected-dorico.txt)
    if [ -n "$DORICO_DIR" ]; then
      test -f "$DORICO_DIR/Expression Maps/User/Ouaricon-VST3-NoteExpression.doricoexpmap" && echo "DORICO-PATH: WROTE"
      grep -q 'VST3 Note Expression' "$DORICO_DIR/Expression Maps/User/Ouaricon-VST3-NoteExpression.doricoexpmap" && echo "DORICO-CONTENT: VALID"
    else
      echo "DORICO-PATH: SKIPPED (no Dorico install detected; manual-import fallback per INST-04)"
    fi
    ```

    Step 6 — AU regression check (Phase 23/24 convention preserved):
    ```bash
    bash scripts/verify-au-link.sh OLyrica
    ```

    Step 7 — Create `plugins/O-Lyrica/dist/.gitkeep` (empty file) so the dist/ directory exists for Plan 25-02's installer build sweep. (No real installer artifact is built in this plan — that's Plan 25-02's job. This is just a directory placeholder.)
  </action>
  <verify>
    <automated>
      test -f build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica.vst3/Contents/Info.plist &&
      test -f build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/O-Lyrica.component/Contents/Info.plist &&
      test -d ~/Library/Audio/Plug-Ins/VST3/O-Lyrica.vst3 &&
      test -d ~/Library/Audio/Plug-Ins/Components/O-Lyrica.component &&
      test -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      test -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/README-doricoexpmap.txt &&
      grep -q 'VST3 Note Expression' ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      diff -q ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap &&
      grep '\[note-expression\] Canonical .doricoexpmap install rules registered' /tmp/25-01-canary-install.log &&
      ( grep 'Dorico expression map installed to Dorico' /tmp/25-01-canary-install.log || grep 'No Dorico install detected' /tmp/25-01-canary-install.log ) &&
      bash scripts/verify-au-link.sh OLyrica &&
      test -f plugins/O-Lyrica/dist/.gitkeep
    </automated>
  </verify>
  <acceptance_criteria>
    - O-Lyrica VST3 + AU build artifacts exist after `ninja OLyrica_VST3 OLyrica_AU` (regression check against Phase 23 `verify-au-link.sh`).
    - `~/Library/Audio/Plug-Ins/VST3/O-Lyrica.vst3/` directory exists (fresh install per CLAUDE.md).
    - `~/Library/Audio/Plug-Ins/Components/O-Lyrica.component/` directory exists.
    - `~/Library/Application Support/Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap` exists AND `diff` against the canonical module file returns identical (`diff -q` exit 0 — proves no transformation/corruption during install).
    - `~/Library/Application Support/Ouaricon/Expression Maps/README-doricoexpmap.txt` exists.
    - The installed .doricoexpmap contains the literal string `VST3 Note Expression` (grep returns ≥1 match).
    - The configure log (`/tmp/25-01-canary-configure.log`) contains the new STATUS line `[note-expression] Canonical .doricoexpmap install rules registered for OLyrica` (proves the install rules registered against the OLyrica target via the per-consumer hook).
    - The install log (`/tmp/25-01-canary-install.log`) contains EITHER `Dorico expression map installed to Dorico [4-9] scan path` (Dorico detected, dual-write succeeded) OR `No Dorico install detected — skipping Dorico-scan write` (Dorico absent on this host; the file in Ouaricon shared path is the user's manual-import source per INST-04).
    - `scripts/verify-au-link.sh OLyrica` exits 0 (regression check — Phase 23/24 AU-link convention not regressed by Phase 25 changes).
    - `plugins/O-Lyrica/dist/.gitkeep` exists (placeholder for Plan 25-02's installer artifacts).
  </acceptance_criteria>
  <done>
    The dual-write install pipeline is proven end-to-end on O-Lyrica via fully-automated checks: build clean, fresh install per CLAUDE.md, `cmake --install` lands the .doricoexpmap at both Ouaricon shared resources path and (if Dorico is on this host) the latest Dorico user expression-maps scan path, file content is byte-identical to the canonical module file, AU-link regression PASS. The Dorico 3-point smoke gate is gated by Task 3b (separate manual checkpoint).
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 3b: Manual Dorico 3-point smoke gate via PKG-pristine O-Lyrica (D-07 from Phase 24, inherited)</name>
  <what-built>
    Task 3a established the full install pipeline end-to-end via `cmake --install`:
      1. Canonical .doricoexpmap landed at `~/Library/Application Support/Ouaricon/Expression Maps/`
      2. (If Dorico installed on this host) Auto-scan copy landed at `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/`
      3. AU regression gate (`verify-au-link.sh OLyrica`) PASS — no Phase 23/24 regression
      4. Module v1.0.0 → v1.1.0 with documented changelog
      5. configure_file() + install(SCRIPT) pattern replaces the originally-planned fragile install(CODE "...") block (per WARNING #8 from checker review)

    What's left: the manual gate that proves Dorico actually engages the canonical .doricoexpmap and routes microtonal accidentals as VST3 Note Expression to O-Lyrica's voices. This is the same 3-point gate inherited from Phase 24 D-07.
  </what-built>
  <how-to-verify>
    Open Dorico, create a single-staff piano flow, set the playback endpoint to O-Lyrica, and assign the "Ouaricon VST3 Note Expression" expression map.

    The map should appear automatically in Dorico's picker (Library → Expression Maps…) IF Dorico's auto-scan path was written by Task 3a's install. If it didn't appear automatically, import manually from `~/Library/Application Support/Ouaricon/Expression Maps/Ouaricon-VST3-NoteExpression.doricoexpmap`.

    Then assign the map to O-Lyrica's channel via Play → Endpoint Setup → expression-map dropdown.

    Run the 3 sub-gates:

    Gate 1 — Pitch landing (Pattern 3 = 240-semitone full-scale):
      • Write a quarter-sharp accidental on C4. Play it.
      • Expected: pitch = +50¢ above C4 ≈ 269.29 Hz (vs 12-TET C4 = 261.63 Hz).
      • Verify with a tuner plugin in the DAW chain or by ear against a 269 Hz reference tone.
      • PASS if observed pitch is within ±5¢ of 269.29 Hz; FAIL otherwise.

    Gate 2 — No attack zipper (Pattern 2 = apply-before-DSP-trigger):
      • Same quarter-sharp C4 note. Listen to the attack transient.
      • Expected: no audible glide/sweep/click on the first ~50ms — the note should sound at the tuned pitch from sample 0.
      • PASS if attack is clean; FAIL if there's a perceptible pitch sweep at the onset.

    Gate 3 — Polyphonic noteId correlation (Pattern 1 = noteId, never pitch):
      • Write a chord: quarter-sharp C4 + natural E4, struck simultaneously.
      • Expected: only the C4 voice is detuned to ~269.29 Hz; E4 plays clean 12-TET (~329.63 Hz).
      • Verify with a spectrum analyzer or a chromatic tuner targeting each voice.
      • PASS if E4 is at 12-TET while C4 is detuned; FAIL if both notes detune or neither detunes.

    Optional pass-flavor signal: confirm whether the .doricoexpmap appeared in Dorico's picker WITHOUT manual import (proves the Dorico-scan-path write took effect) OR whether you had to manually import (partial pass — Ouaricon-shared write succeeded but Dorico-scan write didn't).
  </how-to-verify>
  <resume-signal>
    Respond with one of:
      • `pass` — all 3 Dorico gates PASS and the .doricoexpmap appeared automatically in Dorico's picker
      • `pass-manual-import` — all 3 Dorico gates PASS but I had to manually import the .doricoexpmap (Dorico-scan-path write didn't take effect on this host)
      • `gate-1 fail: [observed pitch / what you saw]`
      • `gate-2 fail: [what zipper you heard]`
      • `gate-3 fail: [what voice was detuned wrongly]`
      • `pre-install-fail: [problem with the install pipeline before Dorico testing was possible]`

    On PASS: record results in the SUMMARY.md (PASS/FAIL line + observed pitch values per Gate 1).
    On FAIL: this is a halt-and-triage situation per D-12. Determine if the failure is plugin-local (e.g., O-Lyrica regressed) or structural (e.g., the .doricoexpmap doesn't actually engage VST3 Note Expression mode). Structural failure → promote to a `25-NN-fix-PLAN.md` per D-12 playbook. Plugin-local → fix in-plan and re-run the gate.
  </resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| repo→user-machine-filesystem | The plugin installer (PKG/EXE) and `cmake --install` are trusted to write to user-owned directories under `$HOME/Library/...` and `%APPDATA%`. The install() rules use absolute paths inside the user's HOME — no system-directory writes. |
| canonical .doricoexpmap → installed copies | Every plugin installer writes the SAME content (idempotent overwrite per D-05). Source of truth is module-owned; installed copies are throwaway. No tampering or per-install variation. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-01-01 | T (Tampering) | Installed .doricoexpmap files at `~/Library/Application Support/Ouaricon/Expression Maps/` | accept | The file is user-editable on purpose (D-05: "Editable canonical user copy"). No plugin runtime depends on its integrity — Dorico reads it once when the user assigns the expression map to a channel. Worst case: user breaks playback for themselves; fix is to reinstall any plugin to overwrite. |
| T-25-01-02 | I (Information disclosure) | Install paths under `$HOME/Library/...` | accept | These are user-owned directories; no PII or secrets in the .doricoexpmap or README (purely technical metadata about microtonality routing). Standard Library subdirectory permissions apply. |
| T-25-01-03 | D (Denial of service) | install-time Dorico version probe in install-doricoexpmap.cmake.in (run via install(SCRIPT)) | mitigate | Probe only checks `IS_DIRECTORY` for known Dorico version paths (4, 5, 6) under user-owned paths; no network calls, no shell execution, no privilege escalation. If probe fails or no version detected, code path skips to the message-only branch (no fatal error). Worst case: Dorico-scan write skipped; user falls back to manual import per INST-04. |
| T-25-01-04 | E (Elevation of privilege) | `install()` rules at install time | accept | `install()` runs at the privilege level the developer/installer invokes. Module install rules write to user HOME — no root/admin escalation needed. PKG installers may run postinstall scripts as root; this plan does not introduce new postinstall logic — only adds payload files. The existing macOS pkg-creation pattern handles user-detection (`stat -f '%Su' /dev/console`) for any user-HOME writes. |
| T-25-01-05 | S (Spoofing) | `Ouaricon-VST3-NoteExpression.doricoexpmap` filename collision in user's Dorico expression-maps directory | mitigate | The filename includes the `Ouaricon-VST3-` prefix to prevent collision with Dorico factory maps or other vendors' maps. README-doricoexpmap.txt explains overwrite semantics. |
</threat_model>

<verification>
1. All files in `files_modified:` exist with the expected content (per Task 1 + Task 2a + Task 2b acceptance criteria grep gates).
2. Module v1.1.0 is the version of record in BOTH `module.yaml` AND `modules/registry.yaml`.
3. The configure pass for O-Lyrica emits the new STATUS line proving the install rules registered.
4. The configure_file() output (`build/test-25-01-config/install-doricoexpmap-OLyrica.cmake`) has all `@…@` placeholders substituted.
5. The `cmake --install --component note-expression-resources` pass writes the .doricoexpmap to the expected paths AND the file contents are byte-identical to the canonical module file (`diff -q` exit 0).
6. AU regression gate `scripts/verify-au-link.sh OLyrica` PASS (no Phase 25 regression on Phase 23/24 AU-link convention).
7. Dorico 3-point smoke gate (Task 3b manual checkpoint) confirms quarter-sharp C4 = +50¢, no attack zipper, polyphonic chord correlation by noteId.
</verification>

<success_criteria>
Plan 25-01 succeeds when:
- INST-01 satisfied: canonical `Ouaricon-VST3-NoteExpression.doricoexpmap` exists at module resources/ path with Microtonality = "VST3 Note Expression"; valid XML.
- INST-02 satisfied: the module owns the asset — no per-plugin copy in any plugin's source tree (grep across `plugins/*/` for `Ouaricon-VST3-NoteExpression.doricoexpmap` returns 0 results, AND the module-owned canonical exists).
- Module bumped to v1.1.0 in `module.yaml` and `modules/registry.yaml` with documented changelog entries.
- `module.cmake` extended with per-platform dual-write install rules using configure_file() + install(SCRIPT) pattern (cleaner than install(CODE "...") per WARNING #8).
- New `install-doricoexpmap.cmake.in` template handles install-time Dorico version detection with @-wrapped configure-time placeholders.
- Module README appended with the new `## Canonical Dorico Expression Map (v1.1.0+)` section.
- O-Lyrica canary proves the entire install pipeline end-to-end: dual-write happens (Task 3a automated), file content survives intact, Dorico 3-point gate PASS (Task 3b manual checkpoint).
- Plan 25-02 has a working foundation to extend across the other 7 plugins.
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-01-author-and-plumbing-SUMMARY.md` with:
- Files modified (full list, including the new `install-doricoexpmap.cmake.in` template)
- Module version bump (1.0.0 → 1.1.0)
- O-Lyrica canary install verification (configure log STATUS line, install log paths, `diff -q` result)
- AU regression gate result (verify-au-link.sh OLyrica)
- Dorico 3-point smoke gate result (PASS/FAIL with observed values per gate)
- Whether Dorico-scan-path write succeeded (and which Dorico version was detected) or fell back to manual import (INST-04)
- Confirmation that configure_file()-based install script pattern was used (WARNING #8 closed)
- Any structural issues uncovered + how triaged (D-12 playbook)
- Hand-off note to Plan 25-02: the plumbing is ready; the 7 remaining plugins (O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) inherit the install rules automatically via their existing `ouaricon_add_module(<Plugin> note-expression)` lines — Plan 25-02 only needs to build their installers and validate the dual-write at distribution time.
</output>
