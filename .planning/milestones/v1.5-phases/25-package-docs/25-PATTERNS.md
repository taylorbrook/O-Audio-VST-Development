# Phase 25: Package & Internal Technical Notes (v2 Playback Template Pivot) — Pattern Map

**Mapped:** 2026-04-26
**Files analyzed:** 14 NEW + 11 MODIFY + 1 DOC = 26 file touch points across 3 plans
**Analogs found:** 24 / 26 (2 NEW XML resources have no in-repo analog — extracted from `/tmp/ample_china_extracted/` external sample)

> **Reference shape:** This phase is heavily *additive* on top of three existing patterns:
> 1. **Phase 23 module-owned-asset shape** (single module owns one resource; consumer auto-inherits via `ouaricon_add_module`) — see `modules/tuning/note-expression/module.cmake:21-41`
> 2. **Phase 24 atomic-sweep shape** (one plan touches all 8 cohort plugins atomically; per-plugin task tables) — see `.planning/phases/24-propagate/24-08-final-sweep-PLAN.md:104-196`
> 3. **Existing macOS pkgbuild + Windows Inno Setup workflows** (per-plugin packaging configs that already exist for all 8 cohort plugins) — see `.claude/skills/plugin-packaging/references/pkg-creation.md` and `.claude/skills/plugin-packaging/assets/inno-template.iss`
>
> Phase 25 v2 EXTENDS each pattern; it does NOT redesign any of them. Every excerpt below names the verbatim file + line range so the planner can write `read_first` lists pointing to the exact analog source.

---

## File Classification

### Plan 25-01 (Author + plumbing — module side)

| File | Op | Role | Data Flow | Closest Analog | Match Quality |
|------|----|------|-----------|----------------|---------------|
| `modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in` | **NEW** | XML resource template (`configure_file @ONLY`) | static-config | `/tmp/ample_china_extracted/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml` | external sample (no in-repo analog; **structural verbatim copy + 1 token swap**) |
| `modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in` | **NEW** | XML resource template (`@TOKEN@` substitution for 8 plugin CIDs) | static-config | `/tmp/ample_china_extracted/EndpointConfigs/Ample China/endpointconfig.xml` | external sample (8-slot subset; **structural verbatim with `@<NAME>_PLUGINID@` substitution**) |
| `modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in` | **NEW** | XML resource (embedded `kScoreLibrary`) | static-config | `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` | recovered XML (use **byte-exact** as-is — `<kScoreLibrary>` root is structurally valid for both `.doricolib` and the embedded deps file) |
| `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` | **NEW** | XML resource (standalone library bundle) | static-config | `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` | recovered XML (use **byte-exact** — same XML body, file extension is what changes) |
| `modules/tuning/note-expression/resources/README-microtonal-suite.txt` | **NEW** | User-facing fallback documentation | static-text | `git show cd2c2c6:modules/tuning/note-expression/resources/README-doricoexpmap.txt` | recovered text (**revise content** for new dual-asset Playback Template flow; preserve tone + structure) |
| `modules/tuning/note-expression/install-microtonal-suite.cmake.in` | **NEW** | CMake install-script template (`configure_file @ONLY`) | static-config | RESEARCH.md "Code Examples" §Build the canonical .dorico_pt §`install-microtonal-suite.cmake.in` template (lines 478-530); **structural carryover from reverted Plan 25-01 v1's `install-doricoexpmap.cmake.in`** | research-driven (no v2 in-repo analog yet — v1 file deleted at d2c86c5) |
| `modules/tuning/note-expression/module.cmake` | **MODIFY (append)** | CMake module hook (currently JUCE-NE-PATCH marker check) | build-time-action | `modules/tuning/note-expression/module.cmake:1-42` (existing patch-marker block — **preserve verbatim**, append packing + install rules below) | exact (extending the same file) |
| `modules/tuning/note-expression/module.yaml` | **MODIFY** | Module metadata (version 1.0.0 → 1.1.0; changelog entry) | metadata | `modules/tuning/note-expression/module.yaml:7` (version line) + `modules/tuning/note-expression/module.yaml:58-71` (changelog entries) | exact |
| `modules/tuning/note-expression/README.md` | **MODIFY (append)** | Module consumer docs | static-text | `modules/tuning/note-expression/README.md:155-173` (existing "Dorico End-User Setup" section — REPLACE with new Playback Template flow) | exact (existing 4-step manual flow becomes the auto-discovery flow + manual-import fallback) |
| `modules/cmake/OuariconModules.cmake` | **MODIFY (append)** | CMake module-system helper | build-time-helper | `modules/cmake/OuariconModules.cmake:30-145` (`ouaricon_add_module` function shape — **mirror the function shape**, append `ouaricon_extract_vst3_cids` after) | exact (same file, new helper function) |
| `modules/registry.yaml` | **MODIFY** | Registry version bump + changelog entry | metadata | `modules/registry.yaml:189-216` (existing `note-expression` entry: bump `version: 1.0.0` → `1.1.0`; ensure `used_by:` list still 8 entries from Phase 24) | exact |
| `plugins/O-Lyrica/CMakeLists.txt` | **TOUCH (verify)** | Canary plugin CMake | build-time-action | `plugins/O-Lyrica/CMakeLists.txt:80` (existing `ouaricon_add_module(OLyrica note-expression)` line) | exact (no edit needed — module-side propagation is implicit via existing call; canary install verifies pipeline works) |

### Plan 25-02 (Installer bundling — atomic sweep across 8 plugins)

| File | Op | Role | Data Flow | Closest Analog | Match Quality |
|------|----|------|-----------|----------------|---------------|
| **For each of 8 plugins** (`plugins/O-{Lyrica,Bells,IntonationPad,Prism,Wind,Reed,Bowed,Formant}/`): | | | | | |
| `plugins/<Plugin>/dist/installer.iss` (Windows, generated per-build) | **MODIFY (template)** | Inno Setup script for VST3 install | installer-config | `.claude/skills/plugin-packaging/assets/inno-template.iss` (47-58 `[Files]` block; 60-77 `[Code]` Pascal block) | exact (template-edit, propagated to 8 plugins via the build-installer skill) |
| `plugins/<Plugin>/dist/<Plugin>-OuariconAudio.pkg` (macOS, generated) | **REBUILD** | macOS PKG installer | installer-config | `.claude/skills/plugin-packaging/references/pkg-creation.md:170-238` (Section 4: Build Base Package — `payload/`, `scripts/postinstall`, `pkgbuild`) | exact (script-edit propagated to 8 plugins via plugin-packaging skill) |
| `.claude/skills/plugin-packaging/assets/inno-template.iss` | **MODIFY** | Shared Inno template (single source of truth across 8 installers) | installer-config | `.claude/skills/plugin-packaging/assets/inno-template.iss:47-77` (existing `[Files]` + `[Code]` sections — append two new `[Files]` + Pascal extraction logic) | exact |
| `.claude/skills/plugin-packaging/references/pkg-creation.md` | **MODIFY** | Shared PKG postinstall reference (single source of truth across 8 installers) | installer-config | `.claude/skills/plugin-packaging/references/pkg-creation.md:177-216` (existing `postinstall` script block — append unzip + Default Library Additions copy) | exact |

### Plan 25-03 (Internal docs — single combined research file)

| File | Op | Role | Data Flow | Closest Analog | Match Quality |
|------|----|------|-----------|----------------|---------------|
| `research/microtonal-dorico-integration.md` | **NEW** | Internal-developer-only technical reference (4 H2 sections: DOCS-01..04) | static-text | `research/microtonality-implementation-juce.md:1-40` (front-matter + TOC structure) | role-match (existing `research/microtonality-*.md` family — same style/tone, NEW topical scope) |

---

## Pattern Assignments

### Pattern A — Module owns an installable asset (extends Phase 23)

**Analog:** `modules/tuning/note-expression/module.cmake:1-42` (existing JUCE-NE-PATCH marker check is the **per-consumer hook fired at configure time** — Phase 25 v2 appends the .dorico_pt packing + dual install() rules **inside the same file** so the same per-consumer firing extends to install-time resource propagation).

**Existing module.cmake header (lines 1-12)** — preserve verbatim:
```cmake
# ==============================================================================
# note-expression module CMake hook
# Verifies the local JUCE fork has the JUCE-NE-PATCH markers applied.
# Fails loud + fails fast at configure time (D-15).
#
# Cross-platform note: CMake-native file/READ + string/FIND is chosen over
# execute_process(grep) because Windows hosts lack `grep`. This check is
# scoped to plugins that consume the note-expression module (only fires
# when ouaricon_add_module(<plugin> note-expression) is called).
# ==============================================================================
```

**Existing per-consumer fire pattern (lines 25-39)** — Phase 25 v2 adds the packing + install logic AFTER this block (additive only, no edit to lines 1-42):
```cmake
foreach(_ne_f ${_NE_FILE1} ${_NE_FILE2})
    if(NOT EXISTS "${_ne_f}")
        message(FATAL_ERROR
            "[note-expression] Expected JUCE source not found: ${_ne_f}\n"
            "Ensure JUCE 8.0.4 is installed and run scripts/apply-juce-patches.sh")
    endif()
    file(READ "${_ne_f}" _ne_contents)
    string(FIND "${_ne_contents}" "${_NE_MARKER}" _ne_idx)
    if(_ne_idx EQUAL -1)
        message(FATAL_ERROR
            "[note-expression] JUCE patch marker '${_NE_MARKER}' not found in:\n"
            "  ${_ne_f}\n"
            "Run: ./scripts/apply-juce-patches.sh")
    endif()
endforeach()

message(STATUS "[note-expression] JUCE-NE-PATCH markers verified in ${_NE_JUCE_ROOT}")
```

**v2 append target — RESEARCH.md "Code Examples" lines 374-426** provides the exact CMake to append:
```cmake
# Stage canonical Playback Template files into a build-tree directory tree matching .dorico_pt layout.
set(DORICO_PT_STAGE "${CMAKE_BINARY_DIR}/_microtonal-suite/Ouaricon-Microtonal-Suite")
file(MAKE_DIRECTORY "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite")
file(MAKE_DIRECTORY "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite")

ouaricon_extract_vst3_cids(
    OUTPUT_VAR PLUGIN_CIDS
    PLUGINS OLyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in"
    "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in"
    "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"
    "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib"
    @ONLY
)

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt"
    COMMAND ${CMAKE_COMMAND} -E tar cf
            "${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt"
            --format=zip
            "PlaybackTemplateSpecs"
            "EndpointConfigs"
    WORKING_DIRECTORY "${DORICO_PT_STAGE}"
    DEPENDS
        "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml"
        "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
        "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib"
    COMMENT "Packing Ouaricon-Microtonal-Suite.dorico_pt"
)
add_custom_target(ouaricon_microtonal_suite_pt ALL
    DEPENDS "${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install-microtonal-suite.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
    @ONLY
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake")
```

**Apply to:** `modules/tuning/note-expression/module.cmake` (Plan 25-01 v2)

---

### Pattern B — CMake helper function shape (mirror `ouaricon_add_module`)

**Analog:** `modules/cmake/OuariconModules.cmake:30-145` — `ouaricon_add_module()` is the existing helper; the new `ouaricon_extract_vst3_cids()` mirrors its structure.

**Existing function signature pattern (lines 30-32)**:
```cmake
function(ouaricon_add_module TARGET_NAME MODULE_NAME)
    cmake_parse_arguments(ARG "" "CONFIG" "" ${ARGN})
    ...
```

**Existing PARENT_SCOPE return pattern (line 143)**:
```cmake
set(OUARICON_MODULE_${MODULE_NAME}_INCLUDED TRUE PARENT_SCOPE)
```

**Existing FATAL_ERROR pattern (lines 45-48)** — the new helper should fail-fast identically when a plugin's `.vst3` is not yet built:
```cmake
if("${MODULE_DIR}" STREQUAL "")
    message(FATAL_ERROR "Module '${MODULE_NAME}' not found in any category. "
                       "Searched: ${MODULE_CATEGORIES}")
endif()
```

**Existing STATUS log pattern (line 50)** — the new helper should announce extracted CIDs identically:
```cmake
message(STATUS "[Ouaricon] Adding module '${MODULE_NAME}' from ${MODULE_DIR}")
```

**v2 helper to APPEND — RESEARCH.md lines 432-474**:
```cmake
function(ouaricon_extract_vst3_cids)
    set(options)
    set(oneValueArgs OUTPUT_VAR)
    set(multiValueArgs PLUGINS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(plugin_target IN LISTS ARG_PLUGINS)
        set(moduleinfo "${CMAKE_BINARY_DIR}/plugins/${plugin_target}/${plugin_target}_artefacts/Release/VST3/${plugin_target}${OUARICON_DEV_SUFFIX}.vst3/Contents/Resources/moduleinfo.json")
        if(NOT EXISTS "${moduleinfo}")
            message(FATAL_ERROR "[Ouaricon] CID extraction: ${plugin_target} VST3 not built yet — build all _VST3 targets before packaging the Microtonal Suite. Expected: ${moduleinfo}")
        endif()
        execute_process(
            COMMAND python3 -c "
import json, sys, re
with open(sys.argv[1]) as f:
    raw = f.read()
raw = re.sub(r',(\\s*[}\\]])', r'\\1', raw)
data = json.loads(raw)
for cls in data['Classes']:
    if cls['Category'] == 'Audio Module Class':
        print(cls['CID'])
        sys.exit(0)
sys.exit(1)
" "${moduleinfo}"
            OUTPUT_VARIABLE cid
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        string(TOUPPER "${plugin_target}" var_name)
        string(REPLACE "-" "" var_name "${var_name}")
        set("${var_name}_PLUGINID" "${cid}" PARENT_SCOPE)
        message(STATUS "[Ouaricon] ${plugin_target} pluginID = ${cid}")
    endforeach()
endfunction()
```

**Apply to:** `modules/cmake/OuariconModules.cmake` (append after existing `ouaricon_check_module_updates` ending at line 188).

---

### Pattern C — XML resource generation (`configure_file @ONLY` with `@TOKEN@` substitution)

**Analog (in-repo, JS resource):** `modules/cmake/OuariconModules.cmake:113-115` — the existing `configure_file(... COPYONLY)` pattern for JS files. The new pattern uses `@ONLY` instead of `COPYONLY` to enable `@TOKEN@` substitution — same primitive, different flag.

```cmake
foreach(JS_FILE ${MODULE_JS_FILES})
    get_filename_component(JS_FILENAME ${JS_FILE} NAME)
    configure_file(${JS_FILE} "${UI_MODULES_DIR}/${JS_FILENAME}" COPYONLY)
    message(STATUS "[Ouaricon]   Copied ${JS_FILENAME} to ui/public/modules/")
endforeach()
```

**Analog (external sample, XML body):** `/tmp/ample_china_extracted/EndpointConfigs/Ample China/endpointconfig.xml` lines 7-34 — one slot's `<slotData>` block. Phase 25 v2 needs **8 slots** (one per cohort plugin), each substituting `@OLYRICA_PLUGINID@`, `@OBELLS_PLUGINID@`, etc., for the `<pluginID>` field. Verbatim slot template:

```xml
<slotData>
    <numAudioOutputs>2</numAudioOutputs>
    <instanceData>
        <slotID>1</slotID>
        <pluginID>@OLYRICA_PLUGINID@</pluginID>
        <pluginName>O-Lyrica</pluginName>
        <pluginPresetLibraryID/>
        <pluginPresetLibraryIDs/>
        <enabled>true</enabled>
        <flags>0</flags>
        <endpointConfigID>endpointconfig.user.ouaricon_microtonal_suite</endpointConfigID>
        <endpointConfigSlotIndex>0</endpointConfigSlotIndex>
        <programContents>
            <entries array="true">
                <entry>
                    <portIndex>0</portIndex>
                    <channelNumberRel0>0</channelNumberRel0>
                    <programName/>
                    <collectionName/>
                    <expressionMapID>xmap.ouaricon.vst3_note_expression</expressionMapID>
                    <drumkitNoteMapID/>
                    <flags>0</flags>
                </entry>
            </entries>
        </programContents>
    </instanceData>
</slotData>
```

**Analog (external sample, top-level template):** `/tmp/ample_china_extracted/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml` lines 1-26 (verbatim structural copy; only `<name>`, `<playbackTemplateSpecID>`, `<creator>`, and `<description>` change for the Ouaricon suite — see RESEARCH.md "Pattern 3" lines 198-227 for the exact target XML).

**Cohort name → CMake variable suffix mapping** (load-bearing for the `@TOKEN@` substitution to work):

| CMake target name | Suffix produced by `string(TOUPPER ...) + REPLACE("-" "")` | XML token to write |
|-------------------|-------------------------------------------------------------|---------------------|
| `OLyrica`         | `OLYRICA`                                                   | `@OLYRICA_PLUGINID@` |
| `O-Bells`         | `OBELLS`                                                    | `@OBELLS_PLUGINID@` |
| `O-IntonationPad` | `OINTONATIONPAD`                                            | `@OINTONATIONPAD_PLUGINID@` |
| `O-Prism`         | `OPRISM`                                                    | `@OPRISM_PLUGINID@` |
| `O-Wind`          | `OWIND`                                                     | `@OWIND_PLUGINID@` |
| `O-Reed`          | `OREED`                                                     | `@OREED_PLUGINID@` |
| `O-Bowed`         | `OBOWED`                                                    | `@OBOWED_PLUGINID@` |
| `O-Formant`       | `OFORMANT`                                                  | `@OFORMANT_PLUGINID@` |

**Apply to:** `endpointconfig.xml.in` (Plan 25-01 v2).

---

### Pattern D — Recovered XML body (use as-is, do NOT re-author)

**Analog:** `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`

**Recovered XML body** (verified read from git; ~57 lines; the `<kScoreLibrary>` root is structurally valid for both `.doricolib` and embedded `playbacktemplatedeps.doricolib`):

```xml
<?xml version="1.0" encoding="utf-8"?>
<!-- Ouaricon Audio canonical Dorico expression map. ... -->
<kScoreLibrary>
    <expressionMapDefinitions>
        <entities array="true">
            <ExpressionMapDefinition>
                <name>Ouaricon VST3 Note Expression</name>
                <entityID>xmap.ouaricon.vst3_note_expression</entityID>
                <parentEntityID/>
                <inheritanceMask>0</inheritanceMask>
                <creator>Ouaricon Audio</creator>
                <description>Routes Dorico per-note microtonal pitch deltas as VST3 Note Expression (kTuningTypeID) events to Ouaricon plugins...</description>
                <version>1</version>
                <pluginNames/>
                <autoMutualExclusion>true</autoMutualExclusion>
                <allowMultipleNotesAtSamePitch>false</allowMultipleNotesAtSamePitch>
                <applyStageTemplateSettings>true</applyStageTemplateSettings>
                <pitchBendRange>2</pitchBendRange>
                <microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>
                <initSwitchData>...</initSwitchData>
                <playingTechniqueCombinations array="true">
                    <playingTechniqueCombination>
                        <baseSwitchID>0</baseSwitchID>
                        <techniqueIDs>pt.natural</techniqueIDs>
                        <enabled>true</enabled>
                        ...
                    </playingTechniqueCombination>
                </playingTechniqueCombinations>
                <techniqueAddOns array="true"/>
                <mutualExclusionGroups array="true"/>
                <playbackOptionsOverrides array="true"/>
            </ExpressionMapDefinition>
        </entities>
    </expressionMapDefinitions>
</kScoreLibrary>
```

**Load-bearing invariants** (D-02, Landmine 3):
- `<entityID>xmap.ouaricon.vst3_note_expression</entityID>` MUST match `<expressionMapID>` in `endpointconfig.xml.in` byte-exactly (cross-file ID coupling).
- `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` is the spike-validated value (Pattern 1–3); must never regress to `kAuto` or `kPitchBend`.

**Apply to:** Both `library/Ouaricon-VST3-NoteExpression.doricolib` AND `playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in` (Plan 25-01 v2).

**Recovery command:**
```bash
git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap > modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
```

---

### Pattern E — Recovered README (revise content, preserve structure/tone)

**Analog:** `git show cd2c2c6:modules/tuning/note-expression/resources/README-doricoexpmap.txt` — 6 H1-style sections (PURPOSE, INSTALL LOCATIONS, MANUAL IMPORT FALLBACK, SOURCE OF TRUTH, SUPPORTED PLUGINS, signature line). Section structure carries forward; section content needs revision for the new dual-asset Playback Template flow.

**Existing structure (sections to preserve verbatim):**
- Title with `===` underline (line 1-2)
- `PURPOSE` H1 (lines 4-12)
- `INSTALL LOCATIONS` H1 (lines 13-23) — **REVISE**: now lists 2 files × 2 destinations × 2 platforms = 8 paths (D-11)
- `MANUAL IMPORT FALLBACK` H1 (lines 24-34) — **REVISE**: replace `Library → Expression Maps → Import` flow with `Play → Playback Template → Import` flow (D-13)
- `SOURCE OF TRUTH` H1 (lines 35-39)
- `SUPPORTED PLUGINS (v1.5 cohort)` H1 (lines 41-44) — preserve verbatim 8-plugin list

**Apply to:** `modules/tuning/note-expression/resources/README-microtonal-suite.txt` (Plan 25-01 v2).

---

### Pattern F — `install-microtonal-suite.cmake.in` template (per-platform dual-write)

**Analog:** RESEARCH.md "Code Examples" §`install-microtonal-suite.cmake.in` template (lines 478-530) — fully-formed v2 template provided by research; **structurally mirrors** the reverted Plan 25-01 v1's `install-doricoexpmap.cmake.in` (only source file + destination subdirectory change).

**Per-platform branch shape** (from RESEARCH.md):
```cmake
# Substitute @-tokens at configure time; run by `cmake --install` at install time.
set(SUITE_PT "@CMAKE_BINARY_DIR@/Ouaricon-Microtonal-Suite.dorico_pt")
set(SUITE_LIB "@CMAKE_CURRENT_LIST_DIR@/resources/library/Ouaricon-VST3-NoteExpression.doricolib")

if(APPLE)
    set(SHARED_DIR "$ENV{HOME}/Library/Application Support/Ouaricon/Microtonal Suite")
    file(MAKE_DIRECTORY "${SHARED_DIR}")
    file(COPY "${SUITE_PT}" DESTINATION "${SHARED_DIR}")
    file(COPY "${SUITE_LIB}" DESTINATION "${SHARED_DIR}")

    foreach(_v 6 5 4)
        set(DORICO_DIR "$ENV{HOME}/Library/Application Support/Steinberg/Dorico ${_v}")
        if(IS_DIRECTORY "${DORICO_DIR}")
            file(MAKE_DIRECTORY "${DORICO_DIR}/PlaybackTemplateSpecs")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xf "${SUITE_PT}"
                WORKING_DIRECTORY "${DORICO_DIR}"
            )
            file(MAKE_DIRECTORY "${DORICO_DIR}/Default Library Additions")
            file(COPY "${SUITE_LIB}" DESTINATION "${DORICO_DIR}/Default Library Additions")
            message(STATUS "[Ouaricon] Microtonal Suite installed for Dorico ${_v}: ${DORICO_DIR}")
            break()
        endif()
    endforeach()
elseif(WIN32)
    set(SHARED_DIR "$ENV{APPDATA}/Ouaricon/Microtonal Suite")
    file(MAKE_DIRECTORY "${SHARED_DIR}")
    file(COPY "${SUITE_PT}" DESTINATION "${SHARED_DIR}")
    file(COPY "${SUITE_LIB}" DESTINATION "${SHARED_DIR}")

    foreach(_v 6 5 4)
        set(DORICO_DIR "$ENV{APPDATA}/Steinberg/Dorico ${_v}")
        if(IS_DIRECTORY "${DORICO_DIR}")
            file(MAKE_DIRECTORY "${DORICO_DIR}/PlaybackTemplateSpecs")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xf "${SUITE_PT}"
                WORKING_DIRECTORY "${DORICO_DIR}"
            )
            file(MAKE_DIRECTORY "${DORICO_DIR}/DefaultLibraryAdditions")  # Note: NO spaces on Windows
            file(COPY "${SUITE_LIB}" DESTINATION "${DORICO_DIR}/DefaultLibraryAdditions")
            message(STATUS "[Ouaricon] Microtonal Suite installed for Dorico ${_v}: ${DORICO_DIR}")
            break()
        endif()
    endforeach()
endif()
```

**Critical invariants** (D-11, D-12):
- macOS dir name: `Default Library Additions` (with spaces)
- Windows dir name: `DefaultLibraryAdditions` (NO spaces)
- Probe Dorico versions in descending order: 6 → 5 → 4 (D-12)
- `cmake -E tar xf` extracts the .dorico_pt zip's internal `PlaybackTemplateSpecs/` and `EndpointConfigs/` subdirs directly into `${DORICO_DIR}/` (verified faithful by A4)
- Idempotent overwrite — `file(COPY)` overwrites in place; safe for all-8-installers writing identical content

**Apply to:** `modules/tuning/note-expression/install-microtonal-suite.cmake.in` (Plan 25-01 v2).

---

### Pattern G — macOS PKG payload (extend pkgbuild postinstall script)

**Analog:** `.claude/skills/plugin-packaging/references/pkg-creation.md:170-216` — existing `payload/` + `postinstall` script structure for macOS PKG.

**Existing payload structure (lines 87-90)**:
```bash
TEMP_DIR="/tmp/${PLUGIN_NAME}-installer"
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR/resources"
mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}"
mkdir -p "$TEMP_DIR/scripts"
```

**Existing payload copy block (lines 173-174)** — per-plugin VST3+AU, Phase 25 v2 ADDS two payload entries (`.dorico_pt` + `.doricolib` from the module's staged build output `${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt` + `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`):
```bash
cp -R "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
cp -R "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
```

**Existing postinstall script body (lines 180-209)** — Phase 25 v2 EXTENDS with a Dorico version probe + unzip + DefaultLibraryAdditions copy block AFTER the existing chown lines:
```bash
cat > "$TEMP_DIR/scripts/postinstall" << 'EOF'
#!/bin/bash
# ...existing user detection + plugin install logic...
ACTUAL_USER=$(stat -f '%Su' /dev/console)
USER_HOME=$(eval echo ~$ACTUAL_USER)
# ...existing VST3+AU copies + chown...

# NEW (Phase 25 v2): Microtonal Suite Dorico template + library bundle
SHARED_DIR="$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite"
mkdir -p "$SHARED_DIR"
cp "/tmp/PLUGIN_NAME_PLACEHOLDER/Ouaricon-Microtonal-Suite.dorico_pt" "$SHARED_DIR/"
cp "/tmp/PLUGIN_NAME_PLACEHOLDER/Ouaricon-VST3-NoteExpression.doricolib" "$SHARED_DIR/"
chown -R "$ACTUAL_USER:staff" "$SHARED_DIR"

for _v in 6 5 4; do
    DORICO_DIR="$USER_HOME/Library/Application Support/Steinberg/Dorico ${_v}"
    if [ -d "$DORICO_DIR" ]; then
        mkdir -p "$DORICO_DIR/PlaybackTemplateSpecs"
        ditto -x -k "$SHARED_DIR/Ouaricon-Microtonal-Suite.dorico_pt" "$DORICO_DIR"
        mkdir -p "$DORICO_DIR/Default Library Additions"
        cp "$SHARED_DIR/Ouaricon-VST3-NoteExpression.doricolib" "$DORICO_DIR/Default Library Additions/"
        chown -R "$ACTUAL_USER:staff" "$DORICO_DIR/PlaybackTemplateSpecs/Ouaricon Microtonal Suite"
        chown -R "$ACTUAL_USER:staff" "$DORICO_DIR/EndpointConfigs/Ouaricon Microtonal Suite"
        chown -R "$ACTUAL_USER:staff" "$DORICO_DIR/Default Library Additions"
        echo "[Ouaricon] Microtonal Suite installed for Dorico ${_v}"
        break
    fi
done
EOF
```

**Apply to:** `.claude/skills/plugin-packaging/references/pkg-creation.md` Section 4b (Plan 25-02 v2 — single source of truth, propagates to all 8 plugins' next pkgbuild run).

---

### Pattern H — Windows Inno Setup `[Files]` + `[Code]` Pascal extraction

**Analog:** `.claude/skills/plugin-packaging/assets/inno-template.iss:47-77` — existing template's `[Files]` (single VST3 entry) + `[Code]` (post-install Ableton cache hint) sections.

**Existing `[Files]` block (lines 47-49)**:
```iss
[Files]
; Install the VST3 bundle (entire directory tree)
Source: "{{VST3_SOURCE_PATH}}\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
```

**Existing `[Code]` block (lines 60-77)** — the existing `CurStepChanged(ssPostInstall)` hook is exactly where the Dorico version probe + extraction logic goes:
```iss
[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var
  AbletonDir: String;
begin
  if CurStep = ssPostInstall then
  begin
    AbletonDir := ExpandConstant('{userappdata}\Ableton');
    if DirExists(AbletonDir) then
    begin
      Log('Ableton preferences directory found - plugin rescan will occur on next launch');
    end;
  end;
end;
```

**v2 additions to `[Files]` block** (Phase 25 v2 appends two entries; both files are produced by the module's CMake at build time, so they live in `${CMAKE_BINARY_DIR}/`):
```iss
[Files]
; Existing VST3 entry — preserve verbatim
Source: "{{VST3_SOURCE_PATH}}\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; NEW (Phase 25 v2): Microtonal Suite Dorico template + library bundle
; Stage to %APPDATA%\Ouaricon\Microtonal Suite\ (canonical, editable)
Source: "{{MICROTONAL_SUITE_PT_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
Source: "{{MICROTONAL_SUITE_DORICOLIB_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
```

**v2 additions to `[Code]` block** — extend `CurStepChanged(ssPostInstall)` to probe Dorico versions and extract:
```iss
[Code]
function ExtractZipTo(ZipPath, DestDir: String): Boolean;
var
  Shell, ZipObj, Folder: Variant;
begin
  // Use Windows Shell.Application COM to extract zip; Inno Setup has no native unzip
  Result := False;
  try
    Shell := CreateOleObject('Shell.Application');
    ZipObj := Shell.NameSpace(ZipPath);
    Folder := Shell.NameSpace(DestDir);
    Folder.CopyHere(ZipObj.Items, 16);  // 16 = no UI prompts, overwrite
    Result := True;
  except
    Log('Zip extraction failed: ' + GetExceptionMessage);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  AbletonDir, DoricoBase, DoricoDir, SharedDir, PtPath, LibPath: String;
  V: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    AbletonDir := ExpandConstant('{userappdata}\Ableton');
    if DirExists(AbletonDir) then
      Log('Ableton preferences directory found - plugin rescan will occur on next launch');

    // NEW (Phase 25 v2): probe Dorico 6 → 5 → 4 and dual-write
    SharedDir := ExpandConstant('{userappdata}\Ouaricon\Microtonal Suite');
    PtPath := SharedDir + '\Ouaricon-Microtonal-Suite.dorico_pt';
    LibPath := SharedDir + '\Ouaricon-VST3-NoteExpression.doricolib';
    DoricoBase := ExpandConstant('{userappdata}\Steinberg');

    for V := 6 downto 4 do
    begin
      DoricoDir := DoricoBase + '\Dorico ' + IntToStr(V);
      if DirExists(DoricoDir) then
      begin
        ForceDirectories(DoricoDir + '\PlaybackTemplateSpecs');
        ExtractZipTo(PtPath, DoricoDir);
        ForceDirectories(DoricoDir + '\DefaultLibraryAdditions');
        FileCopy(LibPath, DoricoDir + '\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib', False);
        Log('[Ouaricon] Microtonal Suite installed for Dorico ' + IntToStr(V));
        Break;
      end;
    end;
  end;
end;
```

**Critical invariants:**
- macOS Dorico dir: `Default Library Additions` (spaces) — Windows: `DefaultLibraryAdditions` (NO spaces). Pattern N below catalogs this trap.
- Inno's `Shell.Application.NameSpace(...).CopyHere` is the standard Inno-Setup-without-extra-DLL way to unzip; alternative is bundling 7zip but adds installer weight.

**Apply to:** `.claude/skills/plugin-packaging/assets/inno-template.iss` (Plan 25-02 v2 — single source of truth, propagates to all 8 plugins' next iscc compile).

**Required new template variables (in `inno-setup-creation.md` Section 3.3):** `{{MICROTONAL_SUITE_PT_PATH}}`, `{{MICROTONAL_SUITE_DORICOLIB_PATH}}`. PowerShell extraction in Section 3.2 must be extended to find these in the build tree.

---

### Pattern I — Atomic-sweep plan (extends Phase 24 final-sweep shape)

**Analog:** `.planning/phases/24-propagate/24-08-final-sweep-PLAN.md:104-196` — Phase 24 final-sweep is the canonical "one plan touches all 8 plugins atomically" shape for Phase 25's Plan 25-02 v2.

**Per-plugin task table pattern (lines 21-37)** — Plan 25-02 v2's task should mirror this table shape:
```yaml
artifacts:
    - path: "~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3"
      provides: "Freshly rebuilt+installed (regression sweep)"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3"
      provides: "Freshly rebuilt+installed"
    # ... 6 more plugins
```

**Per-plugin sweep task body (lines 144-196)** — Phase 24 Task 2's `for each plugin in <8>: build + cache-clear + install` loop is the EXACT skeleton for Plan 25-02 v2's per-plugin installer-build sweep:
```bash
for p in OLyrica O-Bells O-Prism O-Wind O-IntonationPad O-Reed O-Bowed O-Formant; do
    test -d ~/Library/Audio/Plug-Ins/VST3/$p.vst3 || { echo "MISSING VST3: $p"; exit 1; }
done
```

**Stop-on-first-failure escalation pattern (Phase 24 D-12)** — preserved as Phase 25 v2 D-18. Structural failures inside Plan 25-02 sweep promote to `25-02-NN-fix-PLAN.md`.

**Apply to:** Plan 25-02 v2 task structure (overall plan shape and per-plugin task body).

---

### Pattern J — Module metadata bump (semver minor)

**Analog:** `modules/tuning/note-expression/module.yaml:7` (version line) + `modules/tuning/note-expression/module.yaml:58-71` (changelog block).

**Existing version line (line 7)**:
```yaml
version: 1.0.0
```

**Existing changelog block (lines 58-71)** — append a v1.1.0 entry mirroring the v1.0.0 shape:
```yaml
changelog:
  - version: 1.0.0
    date: 2026-04-24
    changes:
      - "Initial extraction from O-Lyrica spike 001/002/003"
      - "Header-only public API under namespace Ouaricon::NoteExpression"
      ...
```

**v2 append target**:
```yaml
  - version: 1.1.0
    date: 2026-04-26
    changes:
      - "Added Ouaricon Microtonal Suite distributable resources (.dorico_pt + .doricolib)"
      - "Added per-platform install() rules (Ouaricon shared + Dorico auto-scan)"
      - "Added ouaricon_extract_vst3_cids helper to OuariconModules.cmake (additive)"
      - "Module source surface unchanged; additive resource surface only"
```

**Registry mirror:** `modules/registry.yaml:191` (the `version: 1.0.0` line in the `note-expression` entry) — bump to `1.1.0`. Existing `used_by:` list (lines 211-227) carries 8 entries from Phase 24; do NOT modify.

**Apply to:** `modules/tuning/note-expression/module.yaml` and `modules/registry.yaml` (Plan 25-01 v2).

---

### Pattern K — Internal research doc (4 H2 sections)

**Analog:** `research/microtonality-implementation-juce.md:1-40` — existing front-matter + TOC structure for the `research/microtonality-*.md` family.

**Existing front-matter shape (lines 1-17)**:
```markdown
---
title: "Microtonality Implementation in JUCE VST Plugins"
created: 2026-01-09
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Complete implementation guide for ..."
domain: dsp
type: guide
keywords:
  - microtonality
  - juce-dsp
  - mts-esp
  ...
stages: [1, 2]
agents: [dsp]
---
```

**v2 target front-matter (Plan 25-03 v2)**:
```markdown
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

**Existing 4-section pattern (lines 31-40)** — TOC structure scales naturally to Plan 25-03's 4 H2 sections (DOCS-01..04):
```markdown
## Table of Contents

1. [Module Architecture](#module-architecture)             [DOCS-01]
2. [Canonical Dorico Setup Procedure](#canonical-setup)    [DOCS-02 — REFRAMED for Playback Template flow]
3. [Host-Side Behavior Quirks](#host-side-quirks)          [DOCS-03 — EXTENDED with new quirks]
4. [Troubleshooting Signatures](#troubleshooting)          [DOCS-04 — EXTENDED with new signatures]
```

**Apply to:** `research/microtonal-dorico-integration.md` (Plan 25-03 v2 NEW file).

---

## Shared Patterns (cross-cutting concerns)

### S-1: Cache clear + fresh install protocol (CLAUDE.md mandate)

**Source:** `CLAUDE.md:9-26` — "CRITICAL: Plugin Cache Clearing" section.

**Apply to:** All Dorico smoke validations in Plan 25-02 v2 (D-15 / D-16). Before each platform's smoke test, run:

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

rm -rf ~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/[PluginName].component
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[PluginName].vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/AU/[PluginName].component ~/Library/Audio/Plug-Ins/Components/
```

```powershell
# Windows equivalent (CLAUDE.md:31-41)
Remove-Item -Recurse -Force "$env:COMMONPROGRAMFILES\VST3\[PluginName].vst3"
Copy-Item -Recurse "build\plugins\[PluginName]\[PluginName]_artefacts\Release\VST3\[PluginName].vst3" "$env:COMMONPROGRAMFILES\VST3\"
Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" -Force -ErrorAction SilentlyContinue
```

### S-2: Per-consumer module hook firing

**Source:** `modules/cmake/OuariconModules.cmake:121-124` — `module.cmake` is `include()`'d once per `ouaricon_add_module()` call; everything inside `module.cmake` fires per-consumer.

```cmake
# Module-supplied CMake hook (optional, backward-compatible)
if(EXISTS "${MODULE_DIR}/module.cmake")
    message(STATUS "[Ouaricon]   Including ${MODULE_NAME}/module.cmake")
    include("${MODULE_DIR}/module.cmake")
endif()
```

**Apply to:** Phase 25 v2 D-08 — the `note-expression` module's `module.cmake` extensions (Pattern A) inherit per-consumer firing automatically. No new dispatch logic needed; existing `OuariconModules.cmake` already routes correctly.

### S-3: `OUARICON_DEV_SUFFIX` build-flavor variable (dev vs prod)

**Source:** `plugins/O-Lyrica/CMakeLists.txt:11` (`PRODUCT_NAME "O-Lyrica${OUARICON_DEV_SUFFIX}"`) + `RESEARCH.md` Pattern 2 (`OuDv` middle bytes for dev, `OuAu` for prod).

**Apply to:** `ouaricon_extract_vst3_cids` helper (Pattern B) MUST honor `${OUARICON_DEV_SUFFIX}` when building the `moduleinfo.json` path:
```cmake
set(moduleinfo "${CMAKE_BINARY_DIR}/plugins/${plugin_target}/${plugin_target}_artefacts/Release/VST3/${plugin_target}${OUARICON_DEV_SUFFIX}.vst3/Contents/Resources/moduleinfo.json")
```

This is the **load-bearing mitigation for Pitfall 2** (RESEARCH.md lines 336-340: dev CID baked into prod installer would silently fail to route). Dev installer reads dev `moduleinfo.json` → ships dev CIDs; prod installer reads prod `moduleinfo.json` → ships prod CIDs. `configure_file @ONLY` substitutes per-flavor at install time.

### S-4: Cross-file ID coupling (`xmap.ouaricon.vst3_note_expression`)

**Source:** Pattern D (recovered XML) `<entityID>` AND Pattern C (endpointconfig.xml.in) `<expressionMapID>`.

**Invariant:** The string `xmap.ouaricon.vst3_note_expression` MUST appear **byte-exact** in three locations:
1. `library/Ouaricon-VST3-NoteExpression.doricolib` `<entityID>` (line ~7 of recovered XML)
2. `playback-template/EndpointConfigs/.../playbacktemplatedeps.doricolib.in` `<entityID>` (same line, embedded copy)
3. `playback-template/EndpointConfigs/.../endpointconfig.xml.in` `<expressionMapID>` (×8, one per slot)

**Recommendation (CONTEXT.md D-17 carries this forward):** Pin via a shared CMake variable used by all three configure_file invocations, OR XML-lint the cross-file ID match in a CMake post-stage check.

### S-5: `xmllint` + `unzip -t` per-task wellformedness gate

**Source:** RESEARCH.md "Sampling Rate" lines 757-759.

**Apply to:** Every Plan 25-01 v2 task that writes/modifies an XML resource — append a wellformedness check to the task `verify` block:
```bash
xmllint --noout modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
xmllint --noout build/_microtonal-suite/Ouaricon-Microtonal-Suite/PlaybackTemplateSpecs/Ouaricon\ Microtonal\ Suite/playbacktemplatespec.xml
xmllint --noout build/_microtonal-suite/Ouaricon-Microtonal-Suite/EndpointConfigs/Ouaricon\ Microtonal\ Suite/endpointconfig.xml
unzip -t build/Ouaricon-Microtonal-Suite.dorico_pt
```

---

## Files With No In-Repo Analog

| File | Op | Reason | Substitute Source |
|------|----|--------|-------------------|
| `playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in` | NEW | First Dorico Playback Template authored in this repo | `/tmp/ample_china_extracted/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml` (verified read; 26 lines) |
| `playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in` | NEW | First Dorico Endpoint Config authored in this repo | `/tmp/ample_china_extracted/EndpointConfigs/Ample China/endpointconfig.xml` (verified read; 484 lines, 11 slots) |

For both files, the planner should reference RESEARCH.md "Pattern 3" (lines 198-227) and "Pattern 4" (lines 236-273) for the canonical Ouaricon-suite-specific XML, NOT the raw Ample China sample. The Ample China sample is the structural reference; Ouaricon's content is purpose-built for the cohort.

---

## Naming Variance Catalog (load-bearing, planner cite)

| Concept | macOS | Windows | Notes |
|---------|-------|---------|-------|
| Default Library Additions dir | `Default Library Additions` (spaces) | `DefaultLibraryAdditions` (NO spaces) | RESEARCH.md Pitfall 3, lines 342-349; Pattern F + Pattern H both encode this asymmetry |
| App Support root | `~/Library/Application Support/` | `%APPDATA%\` | Standard cross-platform variance |
| Dorico version subdir | `Steinberg/Dorico [N]` (spaces, brackets are literal-as-spaces) | `Steinberg\Dorico [N]` (same) | Both platforms use literal "Dorico 6" / "Dorico 5" / "Dorico 4" |
| VST3 dev suffix variable | `${OUARICON_DEV_SUFFIX}` (CMake) → `-dev` (dev) or empty (prod) | Same | Drives bundle name AND `moduleinfo.json` path; load-bearing for CID extraction (S-3) |
| .vst3 install location | `~/Library/Audio/Plug-Ins/VST3/` | `$env:COMMONPROGRAMFILES\VST3\` | CLAUDE.md:22-37 |

---

## Plan ↔ Pattern ↔ File Index (planner crib)

### Plan 25-01 (author + plumbing)

| Task scope | Patterns to apply | Files touched |
|------------|-------------------|---------------|
| Wave 0: A2 + A4 manual verifications (D-14) | — (manual smoke; no code) | None — produces written verification log |
| Recover XML from cd2c2c6 | D | `library/Ouaricon-VST3-NoteExpression.doricolib` (NEW), `playback-template/.../playbacktemplatedeps.doricolib.in` (NEW) |
| Author `playbacktemplatespec.xml.in` | C, S-4 | `playback-template/PlaybackTemplateSpecs/.../playbacktemplatespec.xml.in` (NEW) |
| Author `endpointconfig.xml.in` (8 slots) | C, S-3, S-4 | `playback-template/EndpointConfigs/.../endpointconfig.xml.in` (NEW) |
| Author README | E | `resources/README-microtonal-suite.txt` (NEW) |
| Add `ouaricon_extract_vst3_cids` helper | B, S-3 | `modules/cmake/OuariconModules.cmake` (APPEND) |
| Extend `module.cmake` (packing + install rules) | A, S-2 | `modules/tuning/note-expression/module.cmake` (APPEND) |
| Author `install-microtonal-suite.cmake.in` | F | `modules/tuning/note-expression/install-microtonal-suite.cmake.in` (NEW) |
| Bump module 1.0.0 → 1.1.0 | J | `module.yaml`, `registry.yaml` (MODIFY) |
| Update README.md | E | `modules/tuning/note-expression/README.md` (MODIFY — replace "Dorico End-User Setup" section) |
| O-Lyrica canary install + Dorico smoke | S-1, S-5 | None edited; verify-only |

### Plan 25-02 (installer bundling sweep — atomic)

| Task scope | Patterns to apply | Files touched |
|------------|-------------------|---------------|
| Pre-flight: 25-01 v2 closeout verified | I | None |
| Extend macOS pkgbuild postinstall (single source) | G, S-1 | `.claude/skills/plugin-packaging/references/pkg-creation.md` (MODIFY Section 4b) |
| Extend Windows Inno template (single source) | H, N (variance) | `.claude/skills/plugin-packaging/assets/inno-template.iss` (MODIFY) + `.claude/skills/plugin-packaging/references/inno-setup-creation.md` (MODIFY Section 3.3 — add 2 template vars) |
| Per-plugin sweep: rebuild PKG (×8) | G, I, S-1 | `plugins/<8 plugins>/dist/<Plugin>-OuariconAudio.pkg` (REBUILD; binaries not committed) |
| Per-plugin sweep: rebuild EXE (×8) | H, I, S-1 | `plugins/<8 plugins>/dist/<Plugin>-OuariconAudio-Setup.exe` (REBUILD) |
| Cross-platform validation matrix (D-15/D-16) | S-1, S-5 | `25-02-installer-bundling-sweep-SUMMARY.md` (NEW; results table per platform per plugin) |

### Plan 25-03 (internal docs)

| Task scope | Patterns to apply | Files touched |
|------------|-------------------|---------------|
| Author 4-section research doc | K | `research/microtonal-dorico-integration.md` (NEW) |

---

## Metadata

**Analog search scope:**
- `modules/tuning/note-expression/` (existing module surface — full read)
- `modules/cmake/OuariconModules.cmake` (full read, 188 lines)
- `modules/registry.yaml` (note-expression entry, lines 189-227)
- `plugins/O-Lyrica/CMakeLists.txt` (full read, 119 lines — canary plugin)
- 8 cohort plugins' CMakeLists.txt (grep verified all 8 consume `ouaricon_add_module(<Plugin> note-expression)`)
- `.claude/skills/plugin-packaging/{SKILL.md, SKILL-windows.md, references/pkg-creation.md, references/inno-setup-creation.md, assets/inno-template.iss}` (relevant sections)
- `.planning/phases/24-propagate/{24-08-final-sweep-PLAN.md, 24-PATTERNS.md}` (atomic-sweep shape reference)
- `research/microtonality-implementation-juce.md` (front-matter + TOC structure for the `research/microtonality-*.md` family)
- `git show cd2c2c6:modules/tuning/note-expression/resources/{Ouaricon-VST3-NoteExpression.doricoexpmap, README-doricoexpmap.txt}` (recovered content)
- `/tmp/ample_china_extracted/{PlaybackTemplateSpecs, EndpointConfigs}/` (verified-extracted external sample — schema reference)
- `RESEARCH.md` Code Examples sections (lines 374-530 — full-formed CMake template + helper to copy verbatim)

**Files scanned:** ~30
**Files extracted with code excerpts:** 11
**Pattern extraction date:** 2026-04-26 (v2 replan after Plan 25-01 v1 revert at d2c86c5)
**Supersedes:** Phase 25 v1 patterns (no v1 PATTERNS.md was written; this is the first PATTERNS.md for the phase, written after the pivot).
