---
phase: 25-package-docs
plan: 02
type: execute
wave: 2
depends_on: [25-01]
files_modified:
  - .claude/skills/plugin-packaging/references/pkg-creation.md
  - .claude/skills/plugin-packaging/assets/inno-template.iss
  - .claude/skills/plugin-packaging/references/inno-setup-creation.md
autonomous: false
requirements: [INST-03, INST-04]
tags: [installer, pkg, inno-setup, dorico, doricolib, atomic-sweep, path-b]

must_haves:
  truths:
    - "PKG postinstall (shared template) writes the canonical .doricolib + README to ~/Library/Application Support/Ouaricon/Microtonal Suite/ on every plugin install."
    - "Inno Setup template writes the canonical .doricolib + README to %APPDATA%\\Ouaricon\\Microtonal Suite\\ on every plugin install."
    - "All 8 plugins' freshly-built installers, when installed, produce a working Path B import flow on macOS and Windows."
    - "Cross-platform validation gate (D-08) passes: 1 representative install on macOS + 1 on Windows confirms Library Manager Import + quarter-sharp ~269 Hz."
  artifacts:
    - path: ".claude/skills/plugin-packaging/references/pkg-creation.md"
      provides: "Shared PKG postinstall reference; new section copies .doricolib + README to Ouaricon shared path"
      contains: "Ouaricon/Microtonal Suite"
    - path: ".claude/skills/plugin-packaging/assets/inno-template.iss"
      provides: "Shared Inno Setup template; new [Files] entries for .doricolib + README"
      contains: "Ouaricon-VST3-NoteExpression.doricolib"
    - path: ".claude/skills/plugin-packaging/references/inno-setup-creation.md"
      provides: "Shared Inno Setup reference; new template variables for the two suite paths"
    - path: ".planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md"
      provides: "Cross-platform install + Library Manager Import + quarter-sharp results"
  key_links:
    - from: "PKG postinstall script"
      to: "~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib"
      via: "cp + chown ${ACTUAL_USER}:staff"
      pattern: "Ouaricon/Microtonal Suite"
    - from: "Inno Setup [Files] section"
      to: "%APPDATA%\\Ouaricon\\Microtonal Suite\\Ouaricon-VST3-NoteExpression.doricolib"
      via: "{userappdata}\\Ouaricon\\Microtonal Suite — Flags: ignoreversion"
      pattern: "userappdata.*Ouaricon"
    - from: "any of 8 freshly-built installers"
      to: "Library Manager Import + quarter-sharp playback"
      via: "PKG/EXE install → user runs Library Manager Import once → assigns map per-channel"
      pattern: "xmap.ouaricon.vst3_note_expression"
---

<objective>
Extend the shared PKG postinstall reference and Inno Setup template so every PKG/EXE installer built across the 8-plugin cohort bundles the canonical `.doricolib` + user-facing README, lands them at the Ouaricon shared path on the target platform, and works with a one-time Library Manager Import in Dorico. Then build fresh installers for all 8 plugins on whichever platforms are accessible and run the cross-platform validation gate (D-08): a representative install on macOS + a representative install on Windows.

Purpose: Plan 25-01 ships the canonical asset and module-level install rule, but those only fire when running `cmake --install` directly. End users install plugins via PKG (macOS) or EXE (Windows) — those installer pipelines need to bundle and write the suite asset themselves. This plan modifies the shared installer templates so all 8 plugins' next installer rebuild picks up the new bundling; it does NOT redesign the per-plugin packaging workflow (the existing `plugin-packaging` and `build-installer` skills remain authoritative).

Output: Two shared template files modified (the mechanical sweep), 8 plugin installers rebuilt + validated, and a cross-platform validation matrix recording PASS/FAIL per plugin per platform per gate point. v1.5 ships when this plan's matrix shows the required PASSes (D-08).

Per D-09: Plan 25-02 v3 is dramatically smaller than v2 — no Pascal `[Code]` Dorico-version detection, no `Default Library Additions` directory creation, no spaces-vs-no-spaces variance handling. One destination per platform.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-01-author-and-install-collapse-PLAN.md
@.planning/phases/24-propagate/24-08-final-sweep-PLAN.md
@.claude/skills/plugin-packaging/SKILL.md
@.claude/skills/plugin-packaging/SKILL-windows.md
@.claude/skills/plugin-packaging/references/pkg-creation.md
@.claude/skills/plugin-packaging/references/inno-setup-creation.md
@.claude/skills/plugin-packaging/assets/inno-template.iss
@.claude/skills/build-installer/SKILL.md
@CLAUDE.md

<interfaces>
<!-- Canonical asset paths (created by Plan 25-01) -->
- modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib  (6,431 B Dorico-valid)
- modules/tuning/note-expression/resources/README-microtonal-suite.txt                     (Path B user-facing fallback)

<!-- Build-tree paths (after `ninja <Plugin>_VST3 <Plugin>_AU`) -->
- build/plugins/<Plugin>/<Plugin>_artefacts/Release/VST3/<Plugin>{,-dev}.vst3
- build/plugins/<Plugin>/<Plugin>_artefacts/Release/AU/<Plugin>{,-dev}.component   (macOS only)

<!-- Install destinations (Path B, Plan 25-01 D-07) -->
- macOS PKG postinstall:  ~/Library/Application Support/Ouaricon/Microtonal Suite/
- Windows Inno Setup:     %APPDATA%\Ouaricon\Microtonal Suite\

<!-- 8-plugin cohort -->
OLyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant

<!-- Existing shared template structure (unchanged surfaces — extend, do not redesign) -->
PKG (pkg-creation.md):
- Section 4a "Copy Binaries to Payload" copies VST3+AU into $TEMP_DIR/payload/${PLUGIN_NAME}/
- Section 4b "Create Postinstall Script" emits a /bin/bash heredoc with ACTUAL_USER detection,
  mkdir -p Plug-Ins dirs, cp from /tmp/PLUGIN_NAME_PLACEHOLDER/, chown -R ACTUAL_USER:staff,
  rm -rf /tmp/PLUGIN_NAME_PLACEHOLDER, exit 0.
- Placeholders replaced via sed -i ''  s/PLUGIN_NAME_PLACEHOLDER and PRODUCT_NAME_PLACEHOLDER

Inno Setup (inno-template.iss):
- [Files] section contains: Source: "{{VST3_SOURCE_PATH}}\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"
- [Code] section has CurStepChanged(ssPostInstall) that logs Ableton dir
- Template variables in {{double-curlies}} replaced by build-installer skill PowerShell

Plan 25-02 EXTENDS:
- pkg-creation.md Section 4a: copy 2 additional files into $TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite/
- pkg-creation.md Section 4b heredoc: append a new "Microtonal Suite" block AFTER existing chown lines and BEFORE rm -rf /tmp/...
- inno-template.iss [Files]: append 2 lines for .doricolib + README
- inno-setup-creation.md: document 2 new template variables (paths to canonical asset + README)
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Extend shared PKG postinstall (single source of truth across 8 plugins) for Path B suite copy</name>
  <files>.claude/skills/plugin-packaging/references/pkg-creation.md</files>
  <read_first>
    - .claude/skills/plugin-packaging/references/pkg-creation.md (full file; understand Section 4a/4b structure before editing)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-06 single bundled asset; D-07 single-write to Ouaricon shared path)
    - .planning/phases/25-package-docs/25-01-author-and-install-collapse-PLAN.md (Task 4 README content; install destination strings)
    - CLAUDE.md (postinstall must run as root but write user-owned files via chown -R ACTUAL_USER:staff)
  </read_first>
  <action>
    Edit `.claude/skills/plugin-packaging/references/pkg-creation.md` to extend the shared PKG postinstall with Microtonal Suite copy logic.

    **Edit 1 — Extend Section 4a "Copy Binaries to Payload":**
    Append a new sub-block AFTER the existing two `cp -R` lines (the ones that copy VST3 + AU). Add:

    ```bash
    # Microtonal Suite asset (Phase 25 v3 Path B) — bundled in every PKG built
    # against a plugin that consumes the note-expression module.
    mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite"
    cp "<PROJECT_ROOT>/modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib" \
       "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite/"
    cp "<PROJECT_ROOT>/modules/tuning/note-expression/resources/README-microtonal-suite.txt" \
       "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite/"
    ```

    Use literal `<PROJECT_ROOT>` as a documentation placeholder; the per-plugin packaging script that consumes this reference resolves it (existing convention — pkg-creation.md uses similar shell-variable conventions throughout). Add a one-line clarifying comment that the path resolves to the repo root at packaging time.

    **Edit 2 — Extend Section 4b postinstall heredoc:**
    Insert a NEW block of bash AFTER the existing `chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Audio/Plug-Ins/Components/PRODUCT_NAME_PLACEHOLDER.component"` line (currently around line 202) and BEFORE the `# Clean up temp files` / `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"` lines (currently around line 204-205).

    Insert this block:

    ```bash

    # Microtonal Suite (Phase 25 v3 Path B): copy canonical .doricolib + README
    # to the Ouaricon shared path. User performs a one-time Library Manager
    # Import in Dorico per machine. No Dorico auto-discovery write.
    SUITE_DIR="$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite"
    mkdir -p "$SUITE_DIR"
    cp "/tmp/PLUGIN_NAME_PLACEHOLDER/microtonal-suite/Ouaricon-VST3-NoteExpression.doricolib" "$SUITE_DIR/"
    cp "/tmp/PLUGIN_NAME_PLACEHOLDER/microtonal-suite/README-microtonal-suite.txt" "$SUITE_DIR/"
    chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Application Support/Ouaricon"
    echo "[Ouaricon] Microtonal Suite installed at: $SUITE_DIR"
    echo "[Ouaricon] To activate in Dorico: Library -> Library Manager -> Import... -> select Ouaricon-VST3-NoteExpression.doricolib"
    ```

    Place the new block exactly between the existing chown lines and the cleanup. Sed-placeholder substitution (`PLUGIN_NAME_PLACEHOLDER` → `${PLUGIN_NAME}` etc.) already occurs after the heredoc; the new block uses `PLUGIN_NAME_PLACEHOLDER` consistently with the rest of the script and gets substituted automatically.

    **Edit 3 — Add a section header above the new Section 4a sub-block:**
    Add a small inline subsection header `#### Microtonal Suite asset (Phase 25 v3 Path B)` right above the new block, with a one-paragraph explanation: "Plugins that consume the `note-expression` module ship the canonical Dorico expression-map library bundle alongside the VST3+AU artefacts. The asset is single-source-of-truth at `modules/tuning/note-expression/resources/library/`. The postinstall script lands it at `~/Library/Application Support/Ouaricon/Microtonal Suite/` on the user system; one-time Library Manager Import in Dorico activates it (D-01)."

    Stage as a single atomic commit `docs(25-02): extend PKG postinstall shared template for Path B suite (INST-03)`.
  </action>
  <verify>
    <automated>
      grep -q 'microtonal-suite' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'Ouaricon-VST3-NoteExpression.doricolib' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'README-microtonal-suite.txt' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'Library/Application Support/Ouaricon/Microtonal Suite' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'Library Manager.*Import' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      ! grep -q 'PlaybackTemplateSpecs\|Default Library Additions\|dorico_pt' .claude/skills/plugin-packaging/references/pkg-creation.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `pkg-creation.md` Section 4a contains a sub-block referencing `microtonal-suite` directory under payload
    - `pkg-creation.md` Section 4a copies BOTH `Ouaricon-VST3-NoteExpression.doricolib` AND `README-microtonal-suite.txt` from the module's `resources/` tree into the payload
    - `pkg-creation.md` Section 4b postinstall heredoc contains a `SUITE_DIR` variable resolving to `$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite`
    - `pkg-creation.md` Section 4b postinstall heredoc contains exactly 2 `cp` lines copying from `/tmp/PLUGIN_NAME_PLACEHOLDER/microtonal-suite/` into `$SUITE_DIR/`
    - `pkg-creation.md` Section 4b postinstall heredoc contains a `chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Application Support/Ouaricon"` line (ownership fix per CLAUDE.md root-vs-user pattern)
    - `pkg-creation.md` Section 4b heredoc echoes the canonical activation hint mentioning `Library Manager` and `Import`
    - The new postinstall block is placed BEFORE the `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"` cleanup line (verified by line ordering — `grep -n` shows SUITE_DIR appears before `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"`)
    - `pkg-creation.md` does NOT contain Path A strings: `PlaybackTemplateSpecs`, `Default Library Additions`, `dorico_pt`, `EndpointConfigs`
    - Atomic commit `docs(25-02): extend PKG postinstall shared template for Path B suite (INST-03)` modifies only this file
  </acceptance_criteria>
  <done>
    Shared PKG postinstall reference describes how to bundle and install the Microtonal Suite asset on macOS. All 8 plugins' next PKG rebuild picks up the change automatically.
  </done>
</task>

<task type="auto">
  <name>Task 2: Extend shared Inno Setup template + reference (single source of truth across 8 plugins) for Path B suite copy</name>
  <files>
    .claude/skills/plugin-packaging/assets/inno-template.iss
    .claude/skills/plugin-packaging/references/inno-setup-creation.md
  </files>
  <read_first>
    - .claude/skills/plugin-packaging/assets/inno-template.iss (full file; understand current [Files] + [Code] structure)
    - .claude/skills/plugin-packaging/references/inno-setup-creation.md (full file; understand template-variable substitution pattern)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-06 single bundled asset; D-07 single-write to %APPDATA%\Ouaricon\Microtonal Suite\)
    - .planning/phases/25-package-docs/25-01-author-and-install-collapse-PLAN.md (Task 1 + Task 4 — canonical asset path; README path)
  </read_first>
  <action>
    Two coordinated edits across the Inno Setup shared template files.

    **Edit 1 — `.claude/skills/plugin-packaging/assets/inno-template.iss`:**

    In the existing `[Files]` block, AFTER the existing `Source: "{{VST3_SOURCE_PATH}}\*"; ...` line (currently line 49), append two new lines:

    ```iss
    ; Microtonal Suite (Phase 25 v3 Path B) — bundled with every plugin install
    ; Lands at %APPDATA%\Ouaricon\Microtonal Suite\ for one-time Dorico Library Manager Import.
    Source: "{{MICROTONAL_SUITE_DORICOLIB_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
    Source: "{{MICROTONAL_SUITE_README_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
    ```

    The existing `[Code]` block's `CurStepChanged(ssPostInstall)` procedure can be extended to log the suite install location for the user. Update the existing procedure body (currently logs Ableton plugin rescan) to ALSO log the Microtonal Suite hint:

    ```iss
    [Code]
    procedure CurStepChanged(CurStep: TSetupStep);
    var
      AbletonDir: String;
    begin
      if CurStep = ssPostInstall then
      begin
        // Clear Ableton plugin cache (optional, non-fatal)
        AbletonDir := ExpandConstant('{userappdata}\Ableton');
        if DirExists(AbletonDir) then
        begin
          Log('Ableton preferences directory found - plugin rescan will occur on next launch');
        end;
        // Microtonal Suite (Phase 25 v3 Path B) installed via [Files]; user activates via:
        //   Dorico -> Library -> Library Manager -> Import... -> select Ouaricon-VST3-NoteExpression.doricolib
        Log('[Ouaricon] Microtonal Suite installed at: ' + ExpandConstant('{userappdata}\Ouaricon\Microtonal Suite'));
        Log('[Ouaricon] Activate in Dorico via Library -> Library Manager -> Import...');
      end;
    end;
    ```

    Do NOT add: a `function ExtractZipTo(...)` (no zip extraction needed under Path B), a `for V := 6 downto 4 do` loop (no Dorico-version probe), any `ForceDirectories(DoricoDir + ...)` calls (no auto-discovery write).

    **Edit 2 — `.claude/skills/plugin-packaging/references/inno-setup-creation.md`:**

    Find Section 3 (the section that documents the template-variable substitution PowerShell). Add a new subsection 3.x titled "Microtonal Suite template variables (Phase 25 v3 Path B)" documenting two new template variables:

    - `{{MICROTONAL_SUITE_DORICOLIB_PATH}}` — absolute path to `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` at packaging time. Resolve from project root.
    - `{{MICROTONAL_SUITE_README_PATH}}` — absolute path to `modules/tuning/note-expression/resources/README-microtonal-suite.txt` at packaging time.

    Provide a 4-5-line PowerShell snippet showing how the per-plugin packaging script resolves these to absolute paths and substitutes them in the template (mirroring the existing `{{VST3_SOURCE_PATH}}` substitution pattern). Example:

    ```powershell
    $repoRoot = Resolve-Path "${PSScriptRoot}\..\..\..\.."  # adjust depth per script location
    $suiteDoricolib = "$repoRoot\modules\tuning\note-expression\resources\library\Ouaricon-VST3-NoteExpression.doricolib"
    $suiteReadme = "$repoRoot\modules\tuning\note-expression\resources\README-microtonal-suite.txt"
    $issContent = $issContent -replace '\{\{MICROTONAL_SUITE_DORICOLIB_PATH\}\}', $suiteDoricolib
    $issContent = $issContent -replace '\{\{MICROTONAL_SUITE_README_PATH\}\}', $suiteReadme
    ```

    Also add a one-paragraph note: "Plugins that consume the `note-expression` module bundle the canonical Dorico expression-map library bundle. The asset lands at `%APPDATA%\Ouaricon\Microtonal Suite\` on user install. The user performs a one-time `Library → Library Manager → Import…` per machine to activate the map. No Dorico auto-discovery directory is written; the install destination is Dorico-version-agnostic (D-07)."

    Stage as a single atomic commit `docs(25-02): extend Inno Setup shared template + reference for Path B suite (INST-03)`.
  </action>
  <verify>
    <automated>
      grep -c 'MICROTONAL_SUITE_DORICOLIB_PATH' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -c 'MICROTONAL_SUITE_README_PATH' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -q 'userappdata.*Ouaricon.*Microtonal Suite' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -q 'ignoreversion' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      ! grep -q 'function ExtractZipTo\|for V := 6 downto\|ForceDirectories\|DefaultLibraryAdditions\|dorico_pt' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -q 'MICROTONAL_SUITE_DORICOLIB_PATH' .claude/skills/plugin-packaging/references/inno-setup-creation.md && \
      grep -q 'MICROTONAL_SUITE_README_PATH' .claude/skills/plugin-packaging/references/inno-setup-creation.md && \
      grep -q 'Library Manager' .claude/skills/plugin-packaging/references/inno-setup-creation.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `inno-template.iss` `[Files]` section contains exactly one line matching `MICROTONAL_SUITE_DORICOLIB_PATH` with `DestDir: "{userappdata}\Ouaricon\Microtonal Suite"` and `Flags: ignoreversion`
    - `inno-template.iss` `[Files]` section contains exactly one line matching `MICROTONAL_SUITE_README_PATH` with same `DestDir` + `ignoreversion`
    - `inno-template.iss` `[Code]` `CurStepChanged` procedure contains 2 new `Log(...)` calls referencing the Microtonal Suite path and the Library Manager Import activation hint
    - `inno-template.iss` does NOT contain Path A strings: `function ExtractZipTo`, `for V := 6 downto`, `ForceDirectories`, `DefaultLibraryAdditions`, `dorico_pt`, `PlaybackTemplateSpecs`
    - `inno-setup-creation.md` documents both template variables with descriptions and a PowerShell substitution example
    - `inno-setup-creation.md` contains a paragraph explaining the Path B import flow (`Library Manager` + `Import` mentioned)
    - Atomic commit `docs(25-02): extend Inno Setup shared template + reference for Path B suite (INST-03)` modifies exactly these 2 files
  </acceptance_criteria>
  <done>
    Shared Inno Setup template + reference describe how to bundle and install the Microtonal Suite asset on Windows. All 8 plugins' next EXE rebuild picks up the change automatically.
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 3: Cross-platform validation gate (D-08) — 1 representative install per platform + quarter-sharp smoke</name>
  <files>.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md</files>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-08 cross-platform validation gate; canary on O-Lyrica per Phase 23 precedent)
    - CLAUDE.md (build + install commands; cache-clear protocol — mandatory)
    - .claude/skills/plugin-packaging/SKILL.md (macOS PKG build flow; the `/package` command)
    - .claude/skills/plugin-packaging/SKILL-windows.md (Windows EXE build flow)
    - .claude/skills/build-installer/SKILL.md (Windows installer build CLI)
    - .planning/phases/24-propagate/24-08-final-sweep-PLAN.md (atomic-sweep shape; per-plugin task table reference)
    - modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (canonical asset Plan 25-01 created)
    - .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md (Plan 25-01 canary log; this validation extends that pattern)
  </read_first>
  <action>Execute the human-verified cross-platform validation gate described in <how-to-verify>. The task pauses for the user to rebuild installers across the 8-plugin cohort, install the canary representative on each accessible platform, run Dorico Library Manager Import, and run the 3-point smoke gate. Results are recorded in the validation matrix file. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    Tasks 1+2 modified two shared template files. Those templates feed every per-plugin installer build via the existing `plugin-packaging`/`build-installer` skills. The shipped feature only works if (a) the templates are correct, (b) per-plugin installers are rebuilt, and (c) running an installer on a clean target system produces a working end-to-end import flow. This checkpoint exercises (b) and (c).
  </what-built>
  <how-to-verify>
    Two phases. Run on the dev machine; coordinate with the user for Windows access if not present.

    Append all results to `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` (NEW file). Use a single matrix table per platform.

    ### Phase A — macOS sweep + canary

    **A1. Rebuild macOS PKG installer for O-Lyrica (representative; Phase 23 precedent):**
    ```bash
    # Pre-reqs: clean build of OLyrica VST3+AU per CLAUDE.md
    cd /Users/taylorbrook/Dev/VST-development
    ninja -C build OLyrica_VST3 OLyrica_AU
    ```
    Then trigger PKG packaging for O-Lyrica via the `/package` skill or its Bash equivalent (whichever the team uses for individual plugin packaging — the user-facing slash command is `/package O-Lyrica` per `.claude/skills/package/SKILL.md`). The build script must consume the updated `pkg-creation.md` reference; if the per-plugin script has its own copy of the postinstall heredoc, that is a process bug to flag (escalate per D-11). Capture: PKG output path (e.g., `plugins/O-Lyrica/dist/O-Lyrica-OuariconAudio.pkg`).

    **A2. Clean target environment:**
    ```bash
    # Cache clear per CLAUDE.md
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/OLyrica*.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/OLyrica*.component
    # Suite slate
    rm -rf "$HOME/Library/Application Support/Ouaricon/Microtonal Suite"
    ```

    **A3. Install the freshly-built PKG:**
    ```bash
    sudo installer -pkg plugins/O-Lyrica/dist/O-Lyrica-OuariconAudio.pkg -target /
    ```
    Expected console: pkg-creation log lines including the new `[Ouaricon] Microtonal Suite installed at: ...` echo.

    **A4. Verify suite asset landed at Ouaricon shared path:**
    ```bash
    ls -la "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/"
    stat -f%z "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib"
    # Must show 6431 bytes
    ```
    Verify ownership: file owner is `$USER`, not `root`.

    **A5. Dorico Library Manager Import + quarter-sharp gate:**
    Repeat the procedure from Plan 25-01 Task 5 (Library Manager Import → assign expression map → quarter-sharp ~269 Hz on a chord with quarter-sharp C4). All 3 gate points (per Phase 24 D-07): pitch ~269 Hz, no attack zipper, polyphonic isolation (other notes play 12-TET).

    Record in matrix: PKG built, install succeeded, suite landed (size + owner), Library Manager Import PASS, quarter-sharp gate PASS/FAIL.

    **A6. Bulk PKG sweep (remaining 7 plugins):**
    Per Phase 24 atomic-sweep shape, rebuild PKG for each remaining plugin sequentially:
    ```bash
    for p in O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
        ninja -C build ${p}_VST3 ${p}_AU
        # Run /package $p (or its non-interactive equivalent)
    done
    ```
    For each: capture PKG output path. Spot-check (NOT full Dorico import per plugin — D-08 only requires 1 representative per platform): install one PKG (any of the 7), verify the suite copy landed (running A4 step). Stop-on-first-failure per D-11 — escalate any structural failure to `25-02-NN-fix-PLAN.md`.

    Record per-plugin: PKG built (Y/N), spot-check install (Y/N for the one selected — for the others, Y means "PKG built without packaging error", relying on the shared template guarantee).

    ### Phase B — Windows sweep + canary

    **B1. Determine Windows access:**
    Check whether a Windows dev environment is currently available. If NOT (e.g., user only has macOS access today), record `Windows: ACCESS-BLOCKED` in the matrix. Per D-08: if Windows access is blocked, this plan halts hard — don't silently degrade to macOS-only. The user MUST decide whether to (a) defer Plan 25-02 closeout until Windows access is available, or (b) accept a documented "macOS-only validated" risk for v1.5 ship and explicitly punt Windows validation to v1.6.

    If Windows IS available:

    **B2. Rebuild Windows EXE installer for O-Lyrica:**
    ```powershell
    # On the Windows host:
    cd C:\path\to\VST-development
    cmake --build build --config Release --target OLyrica_VST3 --parallel
    # Trigger build-installer for O-Lyrica (per .claude/skills/build-installer/SKILL.md)
    ```
    The build-installer script must consume the updated `inno-template.iss` and `inno-setup-creation.md`. Capture: EXE output path.

    **B3. Clean target environment:**
    ```powershell
    Remove-Item -Recurse -Force "$env:COMMONPROGRAMFILES\VST3\OLyrica*.vst3" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$env:APPDATA\Ouaricon\Microtonal Suite" -ErrorAction SilentlyContinue
    Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" -Force -ErrorAction SilentlyContinue
    ```

    **B4. Install the freshly-built EXE silently or with UI:**
    ```powershell
    .\plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe /SILENT
    # or interactive: .\plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe
    ```

    **B5. Verify suite asset landed:**
    ```powershell
    Get-ChildItem "$env:APPDATA\Ouaricon\Microtonal Suite\"
    (Get-Item "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib").Length
    # Must show 6431
    ```

    **B6. Dorico Library Manager Import + quarter-sharp gate (Windows):**
    Repeat A5 in Dorico Windows. Same 3-point gate. Record PASS/FAIL.

    **B7. Bulk EXE sweep (remaining 7 plugins):**
    Mirror A6 on Windows. Spot-check one of the 7 by installing + suite-asset check (B5 step).

    ### Matrix output

    Create `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` with this structure:

    ```markdown
    # Plan 25-02 Cross-Platform Validation Matrix

    Date: <YYYY-MM-DD>
    Dorico version: <version>
    macOS host: <version>
    Windows host: <version-or-ACCESS-BLOCKED>

    ## macOS PKG Sweep

    | Plugin | PKG built | Install OK | Suite landed (6431 B) | Library Mgr Import | 3-point gate |
    |--------|-----------|------------|----------------------|--------------------|--------------|
    | O-Lyrica (canary) | Y/N | Y/N | Y/N | PASS/FAIL | PASS/FAIL |
    | O-Bells | Y/N | (skipped) | (skipped) | (skipped) | (skipped) |
    | ... |

    ## Windows EXE Sweep

    (same shape; mark ACCESS-BLOCKED row if applicable)

    ## Verdict
    macOS: PASS / FAIL / PARTIAL
    Windows: PASS / FAIL / ACCESS-BLOCKED
    Overall: PASS / FAIL / DEFER-WINDOWS-TO-v1.6 (per user decision if Windows blocked)
    ```
  </how-to-verify>
  <acceptance_criteria>
    - `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` exists with the documented structure
    - macOS row "O-Lyrica (canary)" has PKG built, Install OK, Suite landed, Library Mgr Import = PASS, 3-point gate = PASS
    - macOS rows for the 7 other plugins each have "PKG built" = Y (or DEFERRED if PKG packaging is rate-limited or batch-deferred per user)
    - Suite landed verification shows file size 6,431 bytes on macOS (`stat -f%z`)
    - Suite file owner on macOS is `$USER` (NOT `root`) — verifies the postinstall chown step worked
    - If Windows accessible: Windows row "O-Lyrica (canary)" has EXE built, Install OK, Suite landed (size 6,431), Library Mgr Import = PASS, 3-point gate = PASS
    - If Windows accessible: Suite landed verification shows file size 6,431 bytes on Windows (`(Get-Item ...).Length`)
    - If Windows blocked: matrix records `ACCESS-BLOCKED` and the Verdict line records the user's explicit decision (`DEFER-WINDOWS-TO-v1.6` is the only valid alternative to a Windows PASS — silent degradation is rejected per D-08)
    - The matrix's Verdict line is one of: `PASS` (both platforms PASS), `FAIL` (any required gate FAIL), or `DEFER-WINDOWS-TO-v1.6` (macOS PASS + user decision)
    - On any structural FAIL: a `25-02-NN-fix-PLAN.md` is created per D-11 stop-on-first-failure protocol; main plan halts pending fix-plan completion
  </acceptance_criteria>
  <resume-signal>Type "matrix-pass" if both platforms PASS; "matrix-defer-windows" with user-decision rationale if Windows is blocked and v1.6-deferral is accepted; or "matrix-fail [details]" to escalate to a fix-plan per D-11.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Build host → user system (PKG/EXE) | Installer payload is signed (PKG productsign on macOS; Inno Setup signature optional on Windows). Plan 25-02 does not change signature posture; the existing `plugin-packaging` skill is authoritative for codesigning. |
| Postinstall script → user filesystem | macOS postinstall runs as root with elevated privileges (`sudo installer`). Writes to `$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite/`. |
| Inno Setup [Files] copy → user filesystem | Runs in installer process context (typically admin per existing `PrivilegesRequired=admin`). Writes to `%APPDATA%\Ouaricon\Microtonal Suite\`. |
| Shared template files (in-repo) | Single source of truth for all 8 plugins' installers. Tampering or accidental edit affects all 8 next rebuilds. Mitigation: existing PR review + atomic commit per task. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-02-01 | Tampering | Shared `pkg-creation.md` and `inno-template.iss` are single-source-of-truth across all 8 plugins; an unintended edit cascades | mitigate | Each task is one shared-template edit, atomic commit, with grep-verifiable acceptance criteria pinning the expected strings (Microtonal Suite path, ignoreversion flag, Library Manager hint). Reviewer can diff to confirm scope. |
| T-25-02-02 | Elevation of Privilege | macOS postinstall runs as root and writes user-owned files; if `$USER_HOME` were attacker-controlled, files could land outside intended dir | mitigate | `$USER_HOME` is derived from `eval echo ~$ACTUAL_USER` where `ACTUAL_USER` comes from `stat -f '%Su' /dev/console` (existing pattern). Both are resolved in-script from system state, not user input. The `chown -R "$ACTUAL_USER:staff"` step ensures correct ownership transfer. ASVS L1 path-traversal mitigation: hard-coded subdir name `Ouaricon/Microtonal Suite` (no user-controlled segment). |
| T-25-02-03 | Tampering | `.doricolib` could be tampered between authoring (Plan 25-01) and packaging (this plan) | mitigate | Plan 25-01 committed the canonical bytes to the repo. Packaging reads from `modules/tuning/note-expression/resources/library/`, the same in-repo path. PKG payload includes the file as-is from repo; PKG signing covers the bundle. Inno Setup [Files] reads the same path; EXE is signed (existing process). |
| T-25-02-04 | Information Disclosure | Postinstall script logs (echo lines) printed to installer log — could leak user paths | accept | Logged paths are well-known canonical locations (`$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite`, `%APPDATA%\Ouaricon\Microtonal Suite`). No PII. Standard installer behavior. |
| T-25-02-05 | Denial of Service | Per-plugin installer rebuild could fail across the cohort if a plugin has a broken `note-expression` consumption (regression from earlier phases) | mitigate | Plan 25-01 Task 5 already validates O-Lyrica works; Phase 24 closed with all 8 plugins passing. Per-plugin rebuild that fails is detected at PKG/EXE build time, not at install time. Stop-on-first-failure (D-11) escalates structural failures to fix plans. |
| T-25-02-06 | Repudiation | The cross-platform validation gate is human-driven (canary install + Library Manager Import + audible smoke) | accept | This is the same architecture as Phase 24's per-plugin Dorico smoke gate. Audit-trail is the matrix file; the gate is the milestone-defining shipped behavior, validated by the human user (visual + auditory). Automated alternatives are deferred (CONTEXT.md "Automated Dorico smoke harness" deferred). |
| T-25-02-07 | Tampering | Inno Setup `Flags: ignoreversion` causes overwrite without version check — if an attacker placed a malicious file at `%APPDATA%\Ouaricon\Microtonal Suite\` before install, our install would NOT detect it | accept | Pre-install state of `%APPDATA%\Ouaricon\Microtonal Suite\` is user-controlled; if compromised, the entire user environment is compromised. ASVS L1 scope is the installer payload itself, which is signed. `ignoreversion` is the standard Inno Setup pattern for non-versioned static assets. |

**Severity:** All HIGH severity threats (Tampering of shared templates, Tampering of `.doricolib` in transit, EoP via path manipulation) mitigated. MEDIUM/LOW threats accepted.

</threat_model>

<verification>
- `.claude/skills/plugin-packaging/references/pkg-creation.md` extended; postinstall block writes 2 files to `~/Library/Application Support/Ouaricon/Microtonal Suite/` with correct chown
- `.claude/skills/plugin-packaging/assets/inno-template.iss` extended; 2 new `[Files]` entries write to `%APPDATA%\Ouaricon\Microtonal Suite\` with `ignoreversion`
- `.claude/skills/plugin-packaging/references/inno-setup-creation.md` documents 2 new template variables with PowerShell substitution example
- macOS validation: O-Lyrica PKG built, install lands the asset (6,431 B, user-owned), Library Manager Import PASS, quarter-sharp ~269 Hz PASS, no attack zipper, polyphonic isolation works
- Windows validation: same end-to-end (or explicit `DEFER-WINDOWS-TO-v1.6` user decision)
- Validation matrix file recorded in `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md`
</verification>

<success_criteria>
1. Both shared templates (`pkg-creation.md` postinstall + `inno-template.iss` [Files]/[Code]) extended with Path B suite copy logic.
2. `inno-setup-creation.md` documents the 2 new template variables for the per-plugin packaging script substitution.
3. All 8 plugins have freshly-built PKG (and EXE if Windows accessible) installers that bundle the suite asset, verified by spot-check installs.
4. Cross-platform validation gate (D-08) passes on macOS + (Windows or explicit deferral): canary plugin install lands the asset, Library Manager Import PASS, quarter-sharp ~269 Hz, no attack zipper, polyphonic isolation works.
5. Validation matrix file documents the per-platform per-plugin per-gate-point result.
6. No Path A residue in shared templates: zero matches for `dorico_pt`, `PlaybackTemplateSpecs`, `Default Library Additions`, `DefaultLibraryAdditions`, `EndpointConfigs` in modified files.
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-02-SUMMARY.md` documenting:
- The two shared-template edits (single-source-of-truth across 8 plugins)
- Per-plugin installer rebuild status (built/skipped/failed)
- Cross-platform validation matrix verdict
- Any in-flight escalations to fix plans (per D-11)
- Final ship-readiness gate: this plan + Plan 25-03 close v1.5 Phase 25.
</output>
