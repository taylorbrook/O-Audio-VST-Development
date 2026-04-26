---
phase: 23-extract
plan: 02
type: execute
wave: 1
depends_on: []
files_modified:
  - scripts/juce-patches/note-expression-juce-8.0.4.patch
  - scripts/apply-juce-patches.sh
  - modules/tuning/note-expression/module.cmake
  - modules/cmake/OuariconModules.cmake
autonomous: true
requirements:
  - MOD-07
tags: [juce, patch, tooling, cmake-marker-check, idempotent-script]

must_haves:
  truths:
    - "A named patch file exists at scripts/juce-patches/note-expression-juce-8.0.4.patch and preserves the exact hunks documented in the spike (JUCE-NE-PATCH marker present in both target files' diffs verbatim)."
    - "scripts/apply-juce-patches.sh exists, is executable, fails loudly if /Users/taylorbrook/JUCE is missing, and is idempotent — a second invocation after a successful apply detects the marker and exits 0 without re-patching."
    - "A CMake-time marker-check runs on `cmake configure` for plugins that consume the note-expression module (and ONLY those plugins), fatal-erroring with a message that names `scripts/apply-juce-patches.sh` if the JUCE-NE-PATCH marker is missing from either patched JUCE file."
    - "Re-apply procedure after a JUCE upgrade is documented in the patch file header (top of .patch) so MOD-07's 're-apply procedure' is discoverable from the patch itself."
  artifacts:
    - path: "scripts/juce-patches/note-expression-juce-8.0.4.patch"
      provides: "Unified diff of both JUCE hunks with JUCE-NE-PATCH marker preserved verbatim in both hunks."
    - path: "scripts/apply-juce-patches.sh"
      provides: "Idempotent bash apply script with preflight (JUCE_DIR exists) + marker detection + loud error messages."
    - path: "modules/tuning/note-expression/module.cmake"
      provides: "CMake marker-check hook. Auto-included by ouaricon_add_module() when present."
    - path: "modules/cmake/OuariconModules.cmake"
      provides: "One additive insertion enabling the optional module.cmake hook pattern (backward-compatible)."
  key_links:
    - from: "modules/tuning/note-expression/module.cmake"
      to: "/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h"
      via: "file(READ) + string(FIND) for marker"
      pattern: "JUCE-NE-PATCH"
    - from: "modules/cmake/OuariconModules.cmake"
      to: "modules/tuning/note-expression/module.cmake"
      via: "include() if EXISTS"
      pattern: 'if\\(EXISTS "\\${MODULE_DIR}/module.cmake"\\)'
    - from: "scripts/apply-juce-patches.sh"
      to: "scripts/juce-patches/note-expression-juce-8.0.4.patch"
      via: "patch -p1 < $PATCH_FILE"
      pattern: "patch -p1"
---

<objective>
Ship the JUCE patch as a named, committable artifact plus idempotent tooling to apply and verify it. Before this plan, the JUCE fork at `/Users/taylorbrook/JUCE/` already carries the `JUCE-NE-PATCH` markers (applied during spike 001). This plan extracts those edits into a `.patch` file, wraps re-application in a safe bash script, and wires a CMake-time marker check that only fires for plugins consuming the `note-expression` module.

Purpose: MOD-07 (local JUCE patch committed as named file with re-apply procedure) and D-12/D-13/D-14/D-15 (format, location, idempotency, verification).

Output:
- `scripts/juce-patches/note-expression-juce-8.0.4.patch`
- `scripts/apply-juce-patches.sh` (executable)
- `modules/tuning/note-expression/module.cmake`
- Additive edit to `modules/cmake/OuariconModules.cmake`
</objective>

<execution_context>
@.claude/get-shit-done/workflows/execute-plan.md
@.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/phases/23-extract/23-CONTEXT.md
@.planning/phases/23-extract/23-PATTERNS.md
@.claude/skills/spike-findings-VST-development/sources/shared-code/juce-patch.md
@scripts/verify-backup.sh
@modules/cmake/OuariconModules.cmake

<interfaces>
<!-- Inputs the tooling consumes (JUCE tree paths) and outputs the CMake hook produces. -->

Post-patch JUCE files (already present on this developer machine; marker = verbatim string):
- /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h (line ~60 onward)
- /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp (line ~3696)
- Marker string (load-bearing — do not rename): `JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)`

OuariconModules.cmake public API (unchanged by this plan — the insertion is backward-compatible):
```cmake
function(ouaricon_add_module TARGET_NAME MODULE_NAME)
    # ... resolves MODULE_DIR by category search ...
    # ... adds cpp/*.{h,cpp} sources, copies js/*.js ...
    # <<< THIS PLAN INSERTS: optional include of ${MODULE_DIR}/module.cmake >>>
    # ... existing ARG_CONFIG handling ...
endfunction()
```
</interfaces>
</context>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Developer machine → JUCE fork | The apply script writes into `/Users/taylorbrook/JUCE/` (developer-local fork). This is a trust boundary only in the sense that a wrong script could corrupt the local JUCE tree. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-23-04 | T (tampering — accidental partial write) | `apply-juce-patches.sh` | mitigate | Preflight: fail with `exit 1` if `JUCE_DIR` does not exist. Idempotency: skip application entirely if the marker is already present (prevents double-application corrupting the tree). |
| T-23-05 | D (denial — silent regression after JUCE upgrade) | `module.cmake` marker-check | mitigate | CMake `FATAL_ERROR` with message pointing to `scripts/apply-juce-patches.sh` so a JUCE upgrade that reverts the patch fails `cmake configure` loudly instead of producing a plugin that silently drops NE events. |
| T-23-06 | E (elevation — untrusted JUCE path) | `module.cmake` | accept | `JUCE_DIR` is read from `$ENV{JUCE_DIR}` or defaults to known paths (`/Users/taylorbrook/JUCE`, `C:/JUCE`). No user-supplied data flows in; `file(READ)` operates on a developer-controlled tree. |
</threat_model>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Generate the committable .patch file</name>
  <files>scripts/juce-patches/note-expression-juce-8.0.4.patch</files>
  <read_first>
    - .claude/skills/spike-findings-VST-development/sources/shared-code/juce-patch.md (entire file — source of truth for both hunks; preserves exact marker comments)
    - .planning/phases/23-extract/23-CONTEXT.md (D-12 unified-diff format, D-13 filename convention, D-15 marker convention)
    - .planning/phases/23-extract/23-PATTERNS.md §`scripts/juce-patches/note-expression-juce-8.0.4.patch (CREATE — new convention)` (generation procedure)
  </read_first>
  <action>
    The JUCE fork at `/Users/taylorbrook/JUCE/` already has the patch applied (spike 001 state — confirmed by `grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/ /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/` returning 4 hits). Generate a unified diff against pristine JUCE 8.0.4 and commit it as the canonical patch.

    **Step 1 — Preflight check:** Run `grep -rln "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/ /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/`. If this returns fewer than 2 file paths, STOP and report: "JUCE fork does not have the JUCE-NE-PATCH applied. Cannot generate patch file." (Expected: 2 files, one each in those directories.)

    **Step 2 — Fetch pristine JUCE 8.0.4 for diff baseline:** Clone into a temp directory:
    ```bash
    mkdir -p /tmp/juce-ne-patchgen
    if [ ! -d /tmp/juce-ne-patchgen/JUCE ]; then
      git clone --depth=1 --branch=8.0.4 https://github.com/juce-framework/JUCE /tmp/juce-ne-patchgen/JUCE
    fi
    ```

    **Step 3 — Create the patches subdir:**
    ```bash
    mkdir -p scripts/juce-patches
    ```

    **Step 4 — Generate the unified diff with unified `diff -u` (patch -p1 compatible):**
    ```bash
    PATCH_OUT=scripts/juce-patches/note-expression-juce-8.0.4.patch
    # Header comment block (written first)
    cat > "$PATCH_OUT" << 'HDR'
    # note-expression module — JUCE local-fork patch (Ouaricon)
    #
    # Target: JUCE 8.0.4
    # Marker: JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)
    # Purpose: surface kNoteExpressionValueEvent and noteId-tagged NoteOn/NoteOff
    #          to the plugin via VST3ClientExtensions::onVst3RawEvent, before
    #          MidiEventList::toMidiBuffer drops them.
    #
    # Apply from your JUCE root:
    #     cd $JUCE_DIR && patch -p1 < /path/to/this/file
    # Or use the idempotent wrapper:
    #     ./scripts/apply-juce-patches.sh
    #
    # Re-apply procedure after a JUCE upgrade:
    #   1. Upgrade JUCE to the new version.
    #   2. Regenerate this patch by diffing pristine-new vs your-forked-new; rename
    #      the file to note-expression-juce-<NEW-VERSION>.patch.
    #   3. Commit the new patch file and delete the old one.
    #   4. Update modules/tuning/note-expression/module.yaml requirements.juce_patch.file
    #      and requirements.juce_patch.juce_version.
    #   5. Run ./scripts/apply-juce-patches.sh to verify idempotent apply.
    HDR

    # Append the two unified diffs.
    diff -u /tmp/juce-ne-patchgen/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h \
            /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h \
        | sed \
            -e 's|^--- /tmp/juce-ne-patchgen/JUCE/|--- a/|' \
            -e 's|^+++ /Users/taylorbrook/JUCE/|+++ b/|' \
        >> "$PATCH_OUT" || true

    diff -u /tmp/juce-ne-patchgen/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp \
            /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp \
        | sed \
            -e 's|^--- /tmp/juce-ne-patchgen/JUCE/|--- a/|' \
            -e 's|^+++ /Users/taylorbrook/JUCE/|+++ b/|' \
        >> "$PATCH_OUT" || true
    ```
    (Note: `diff -u` exits 1 when files differ — treat non-zero exit as success; hence `|| true`. Both `sed` invocations rewrite absolute paths to `a/` and `b/` prefixes so `git apply` / `patch -p1` work portably against any JUCE checkout root.)

    **Step 5 — Verify the generated patch contains the marker:** `grep -c "JUCE-NE-PATCH" scripts/juce-patches/note-expression-juce-8.0.4.patch` must be at least `2` (one in each file's inserted block).

    **Step 6 — Sanity-check hunk structure:**
    - `grep -c "^--- a/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns `1`.
    - `grep -c "^--- a/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns `1`.
    - `grep -c "^+++ b/" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns `2`.
    - `grep -c "onVst3RawEvent" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns at least `2` (one in header struct declaration, one in cpp dispatch).

    Do NOT hand-write the patch body. The diff utility must be the source of truth so the file is guaranteed re-appliable.

    **Fallback path (only if the pristine clone is unavailable / offline):** write the patch body manually by transcribing the exact hunks from `.claude/skills/spike-findings-VST-development/sources/shared-code/juce-patch.md` into unified-diff form. This is a strict fallback; prefer the `diff -u` path. If the fallback is used, the patch header comment must note "Hand-transcribed fallback — verify re-apply on next JUCE upgrade."
  </action>
  <verify>
    <automated>test -f scripts/juce-patches/note-expression-juce-8.0.4.patch &amp;&amp; [ "$(grep -c 'JUCE-NE-PATCH' scripts/juce-patches/note-expression-juce-8.0.4.patch)" -ge "2" ] &amp;&amp; [ "$(grep -c '^--- a/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h' scripts/juce-patches/note-expression-juce-8.0.4.patch)" = "1" ] &amp;&amp; [ "$(grep -c '^--- a/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp' scripts/juce-patches/note-expression-juce-8.0.4.patch)" = "1" ] &amp;&amp; grep -q 'onVst3RawEvent' scripts/juce-patches/note-expression-juce-8.0.4.patch</automated>
  </verify>
  <acceptance_criteria>
    - `test -f scripts/juce-patches/note-expression-juce-8.0.4.patch` exits 0
    - `grep -c "JUCE-NE-PATCH" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns at least `2` (marker preserved verbatim — D-15, landmine 5)
    - `grep -c "^--- a/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns `1`
    - `grep -c "^--- a/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns `1`
    - `grep -c "^+++ b/" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns exactly `2`
    - `grep -c "onVst3RawEvent" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns at least `2`
    - `grep -c "struct Vst3RawEvent" scripts/juce-patches/note-expression-juce-8.0.4.patch` returns at least `1`
    - `head -20 scripts/juce-patches/note-expression-juce-8.0.4.patch | grep -c "Re-apply procedure"` returns `1` (re-apply procedure documented in header per MOD-07)
    - Dry-run test: `cd /tmp/juce-ne-patchgen/JUCE && patch -p1 --dry-run < $OLDPWD/scripts/juce-patches/note-expression-juce-8.0.4.patch` exits 0 (patch is syntactically valid and applies cleanly to pristine)
  </acceptance_criteria>
  <done>
    Patch file committed with valid unified-diff format, both hunks present, marker preserved verbatim, re-apply procedure in header, passes `patch --dry-run` against pristine JUCE 8.0.4.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Create apply-juce-patches.sh (idempotent wrapper)</name>
  <files>scripts/apply-juce-patches.sh</files>
  <read_first>
    - scripts/verify-backup.sh (bash style: set -e, color codes, early exit — lines 1–20)
    - .planning/phases/23-extract/23-CONTEXT.md (D-14 idempotency requirement)
    - .planning/phases/23-extract/23-PATTERNS.md §`scripts/apply-juce-patches.sh (CREATE)` (full script template ready to adapt)
    - CLAUDE.md (confirms `/Users/taylorbrook/JUCE` as the expected developer-local JUCE root)
  </read_first>
  <action>
    Write `scripts/apply-juce-patches.sh` as a POSIX bash script matching the style of `scripts/verify-backup.sh` (set -e, color variables, early exit). The script must be safe to run N times: applied state on first run, no-op on subsequent runs.

    **Exact script body** (paste verbatim, adjust nothing except wording of echo lines if you prefer — keep behavior identical):

    ```bash
    #!/bin/bash
    set -e

    # ==============================================================================
    # apply-juce-patches.sh
    # Idempotent applier for Ouaricon JUCE local-fork patches.
    #
    # Behavior:
    #   1. Fail loudly if JUCE_DIR (default /Users/taylorbrook/JUCE) is missing.
    #   2. Skip application if the JUCE-NE-PATCH marker is already present.
    #   3. Apply scripts/juce-patches/note-expression-juce-8.0.4.patch otherwise.
    # ==============================================================================

    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    RED='\033[0;31m'
    NC='\033[0m'

    JUCE_DIR="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    PATCH_DIR="$SCRIPT_DIR/juce-patches"
    PATCH_FILE="$PATCH_DIR/note-expression-juce-8.0.4.patch"
    MARKER="JUCE-NE-PATCH"

    # Step 1: preflight — JUCE tree must exist
    if [[ ! -d "$JUCE_DIR" ]]; then
      echo -e "${RED}[apply-juce-patches] JUCE tree not found at ${JUCE_DIR}${NC}"
      echo -e "Set JUCE_DIR env var or install JUCE at /Users/taylorbrook/JUCE (see CLAUDE.md)."
      exit 1
    fi

    # Step 2: patch file must exist
    if [[ ! -f "$PATCH_FILE" ]]; then
      echo -e "${RED}[apply-juce-patches] Patch file not found: ${PATCH_FILE}${NC}"
      exit 1
    fi

    # Step 3: idempotency — skip if marker already present in the two target files
    HEADER_FILE="$JUCE_DIR/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h"
    CPP_FILE="$JUCE_DIR/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
    FOUND=0
    for f in "$HEADER_FILE" "$CPP_FILE"; do
      if [[ -f "$f" ]] && grep -q "$MARKER" "$f"; then
        FOUND=$((FOUND + 1))
      fi
    done

    if [[ "$FOUND" -ge 2 ]]; then
      echo -e "${GREEN}[apply-juce-patches] Marker ${MARKER} already present in JUCE tree — skipping.${NC}"
      exit 0
    fi

    # Step 4: apply
    echo -e "${YELLOW}[apply-juce-patches] Applying ${PATCH_FILE}...${NC}"
    ( cd "$JUCE_DIR" && patch -p1 < "$PATCH_FILE" )
    echo -e "${GREEN}[apply-juce-patches] Patch applied. Verify with:${NC}"
    echo -e "  grep -rn \"$MARKER\" $JUCE_DIR/modules/juce_audio_processors/utilities/ $JUCE_DIR/modules/juce_audio_plugin_client/"
    ```

    After writing, `chmod +x scripts/apply-juce-patches.sh`.

    **Run the script once to validate idempotency** (on this developer machine the JUCE fork already carries the marker, so the expected outcome is the idempotency branch — "skipping"):
    ```bash
    ./scripts/apply-juce-patches.sh
    # Expected: green line "Marker JUCE-NE-PATCH already present ... skipping." and exit 0.
    ```
    Then run it a second time — output must be identical.
  </action>
  <verify>
    <automated>test -x scripts/apply-juce-patches.sh &amp;&amp; bash -n scripts/apply-juce-patches.sh &amp;&amp; ./scripts/apply-juce-patches.sh | grep -q "already present\|Patch applied"</automated>
  </verify>
  <acceptance_criteria>
    - `test -f scripts/apply-juce-patches.sh` exits 0
    - `test -x scripts/apply-juce-patches.sh` exits 0 (executable bit set)
    - `bash -n scripts/apply-juce-patches.sh` exits 0 (syntactically valid)
    - `grep -c "JUCE_DIR:-/Users/taylorbrook/JUCE" scripts/apply-juce-patches.sh` returns `1` (default path per CLAUDE.md)
    - `grep -c 'MARKER="JUCE-NE-PATCH"' scripts/apply-juce-patches.sh` returns `1` (marker referenced verbatim)
    - `grep -c "patch -p1" scripts/apply-juce-patches.sh` returns `1`
    - Running `./scripts/apply-juce-patches.sh` on the developer machine (where the patch is already applied) prints a "skipping" line and exits 0
    - Running it a SECOND time produces identical output and also exits 0 (idempotency proven)
    - Running `JUCE_DIR=/tmp/nonexistent-juce-dir ./scripts/apply-juce-patches.sh; echo "exit=$?"` prints a red error line and `exit=1` (preflight works)
  </acceptance_criteria>
  <done>
    Script is executable, idempotent (no-op on applied tree), fails loudly when JUCE_DIR missing, prints clear status messages.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Add CMake-time marker check (module.cmake + OuariconModules.cmake hook)</name>
  <files>
    modules/tuning/note-expression/module.cmake,
    modules/cmake/OuariconModules.cmake
  </files>
  <read_first>
    - modules/cmake/OuariconModules.cmake (entire file — especially lines 30–101 ouaricon_add_module body)
    - .planning/phases/23-extract/23-CONTEXT.md (D-15 verification = CMake-time marker check; module-level enforcement so only consumers are gated; Claude's discretion covers `file(READ)` vs `execute_process` choice)
    - .planning/phases/23-extract/23-PATTERNS.md §`CMake-time marker check (D-15 — new convention)` and §`modules/cmake/OuariconModules.cmake (MODIFY — one optional insertion)` (full template)
  </read_first>
  <action>
    Wire a CMake-time marker check that fails `cmake configure` (fatal error) whenever the JUCE fork is missing the `JUCE-NE-PATCH` marker — BUT only for plugins that consume the `note-expression` module.

    **File 1 — Create `modules/tuning/note-expression/module.cmake`:**

    ```cmake
    # ==============================================================================
    # note-expression module CMake hook
    # Verifies the local JUCE fork has the JUCE-NE-PATCH markers applied.
    # Fails loud + fails fast at configure time (D-15).
    # ==============================================================================

    # Locate JUCE tree the same way the root CMakeLists.txt does.
    if(DEFINED ENV{JUCE_DIR})
        set(_NE_JUCE_ROOT "$ENV{JUCE_DIR}")
    elseif(WIN32)
        set(_NE_JUCE_ROOT "C:/JUCE")
    else()
        set(_NE_JUCE_ROOT "/Users/taylorbrook/JUCE")
    endif()

    set(_NE_MARKER "JUCE-NE-PATCH")
    set(_NE_FILE1 "${_NE_JUCE_ROOT}/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h")
    set(_NE_FILE2 "${_NE_JUCE_ROOT}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp")

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

    Rationale (keep in comments): the `file(READ)` + `string(FIND)` path is chosen over `execute_process(grep)` because Windows hosts lack `grep`. The check is scoped to this module (via the hook in OuariconModules.cmake), so only plugins that `ouaricon_add_module(X note-expression)` get gated.

    **File 2 — Edit `modules/cmake/OuariconModules.cmake`:** Insert a small additive block that auto-includes `${MODULE_DIR}/module.cmake` when present. Place it AFTER the JS-copy block that ends at line 80 and BEFORE the `# Process configuration if provided` block at line 82.

    Current code (lines 67–82 of `modules/cmake/OuariconModules.cmake`) — context window around the insertion point:
    ```cmake
        # Copy JS files to ui/public/modules/
        if(EXISTS "${MODULE_DIR}/js")
            file(GLOB MODULE_JS_FILES "${MODULE_DIR}/js/*.js")

            if(MODULE_JS_FILES)
                set(UI_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/modules")
                file(MAKE_DIRECTORY "${UI_MODULES_DIR}")

                foreach(JS_FILE ${MODULE_JS_FILES})
                    get_filename_component(JS_FILENAME ${JS_FILE} NAME)
                    configure_file(${JS_FILE} "${UI_MODULES_DIR}/${JS_FILENAME}" COPYONLY)
                    message(STATUS "[Ouaricon]   Copied ${JS_FILENAME} to ui/public/modules/")
                endforeach()
            endif()
        endif()

        # Process configuration if provided
        if(ARG_CONFIG)
    ```

    Insert the following block between the `endif()` that closes the JS block and the `# Process configuration if provided` comment:

    ```cmake

        # Module-supplied CMake hook (optional, backward-compatible)
        if(EXISTS "${MODULE_DIR}/module.cmake")
            message(STATUS "[Ouaricon]   Including ${MODULE_NAME}/module.cmake")
            include("${MODULE_DIR}/module.cmake")
        endif()

    ```

    Do not change anything else in `OuariconModules.cmake`. No existing module ships a `module.cmake`, so this change is behavior-preserving for all other plugins.

    After this task, the integration chain is:
    - Plan 03 writes `ouaricon_add_module(OLyrica note-expression)` into `plugins/O-Lyrica/CMakeLists.txt`
    - CMake finds `modules/tuning/note-expression/module.cmake`
    - The hook fires, reads both JUCE files, verifies the marker
    - If missing, `FATAL_ERROR` with the apply-script path

    **Validation build (smoke check ONLY — full build is gated by Plan 03):** the CMake configure can be run without building. On the developer machine (where the JUCE fork IS patched), the configure succeeds. Any existing plugin that does NOT consume note-expression is unaffected.
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/module.cmake &amp;&amp; grep -q 'JUCE-NE-PATCH' modules/tuning/note-expression/module.cmake &amp;&amp; grep -q 'FATAL_ERROR' modules/tuning/note-expression/module.cmake &amp;&amp; grep -q 'scripts/apply-juce-patches.sh' modules/tuning/note-expression/module.cmake &amp;&amp; grep -c 'if(EXISTS "\${MODULE_DIR}/module.cmake")' modules/cmake/OuariconModules.cmake | grep -q '^1$'</automated>
  </verify>
  <acceptance_criteria>
    - `test -f modules/tuning/note-expression/module.cmake` exits 0
    - `grep -c "JUCE-NE-PATCH" modules/tuning/note-expression/module.cmake` returns at least `2` (marker variable + usage)
    - `grep -c "FATAL_ERROR" modules/tuning/note-expression/module.cmake` returns `2` (one for missing file, one for missing marker)
    - `grep -c "scripts/apply-juce-patches.sh" modules/tuning/note-expression/module.cmake` returns at least `1` (error messages name the recovery action per D-15)
    - `grep -c "file(READ" modules/tuning/note-expression/module.cmake` returns `1` (cross-platform — chosen over `execute_process(grep)`)
    - `grep -c "string(FIND" modules/tuning/note-expression/module.cmake` returns `1`
    - `grep -c 'if(EXISTS "${MODULE_DIR}/module.cmake")' modules/cmake/OuariconModules.cmake` returns `1`
    - `grep -c "include(\"\${MODULE_DIR}/module.cmake\")" modules/cmake/OuariconModules.cmake` returns `1`
    - The edit to `OuariconModules.cmake` is additive: `git diff modules/cmake/OuariconModules.cmake` shows only inserted lines (no removals) — `git diff --stat modules/cmake/OuariconModules.cmake | grep -E "deletions?" | grep -v "^ 0 " | wc -l` should be `0`
    - **Negative test (manual):** on a machine where the JUCE patch is NOT applied, `cmake configure` of O-Lyrica (after Plan 03) must fatal-error with a message naming `scripts/apply-juce-patches.sh`. (Not run by this plan; documented as operational expectation.)
    - **Positive test (now, on the developer machine):** `cd build && cmake ..` (or equivalent) for any existing plugin that does NOT consume note-expression succeeds without triggering the new hook — confirms backward compatibility. If a build directory is not available, this check is deferred to Plan 03.
  </acceptance_criteria>
  <done>
    module.cmake exists with marker check; OuariconModules.cmake has a single additive insertion that auto-includes the hook; error messages name the recovery script; no existing plugin behavior changes.
  </done>
</task>

</tasks>

<verification>
1. Patch file committable and valid:
   - `test -f scripts/juce-patches/note-expression-juce-8.0.4.patch && grep -c "JUCE-NE-PATCH" scripts/juce-patches/note-expression-juce-8.0.4.patch | grep -qE '^[2-9]|^[0-9]{2,}'`
2. Apply script runs idempotently on the current machine:
   - `./scripts/apply-juce-patches.sh && ./scripts/apply-juce-patches.sh` — both invocations exit 0 with "already present" messaging on the second.
3. CMake hook is scoped to consumers only:
   - `grep -c "if(EXISTS \"\${MODULE_DIR}/module.cmake\")" modules/cmake/OuariconModules.cmake` returns `1` (one-line additive insert)
   - No existing plugin's CMake configure fails as a result of this plan.
4. Recovery path is discoverable from the failure message:
   - `grep -c "apply-juce-patches.sh" modules/tuning/note-expression/module.cmake` returns at least `1`
</verification>

<success_criteria>
- MOD-07: Local JUCE patch committed as a named file in `scripts/juce-patches/note-expression-juce-8.0.4.patch`; re-apply procedure documented in the patch's header comment AND in `scripts/apply-juce-patches.sh`'s idempotent behavior.
- D-15: CMake-time marker-check fatal-errors with a message pointing at `scripts/apply-juce-patches.sh` if the marker is absent from either JUCE target file. Hook is scoped to plugins that consume `note-expression` (and ONLY those plugins).
- Marker string preserved verbatim in the `.patch` file (one of the Phase 23 quality gates).
</success_criteria>

<output>
After completion, create `.planning/phases/23-extract/23-02-SUMMARY.md` describing:
- Patch file location, hunk count, and header comment content.
- Apply-script idempotency behavior (tested: apply-then-apply on a patched tree, and preflight on a missing JUCE_DIR).
- CMake hook scope (only consumers trigger the check).
- Known limitation: re-apply after a JUCE upgrade requires re-generating the patch (procedure in patch header).
- What Plan 03 consumes from this plan: the `ouaricon_add_module(OLyrica note-expression)` call in O-Lyrica's CMakeLists.txt will automatically pull in `modules/tuning/note-expression/module.cmake` and the marker check.
</output>
