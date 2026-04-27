---
phase: 25-package-docs
plan: 02
type: execute
wave: 2
depends_on: [25-01-author-and-plumbing-PLAN.md]
files_modified:
  - .claude/skills/plugin-packaging/references/pkg-creation.md
  - .claude/skills/plugin-packaging/assets/inno-template.iss
  - .claude/skills/plugin-packaging/references/inno-setup-creation.md
  - .planning/phases/25-package-docs/25-02-SUMMARY.md
autonomous: false
requirements: [INST-03, INST-04]
must_haves:
  truths:
    - "macOS PKG postinstall script (single shared source) extracts .dorico_pt zip into ~/Library/Application Support/Steinberg/Dorico [N]/ via ditto -x -k AND copies .doricolib into Default Library Additions/ (with spaces) (D-10, D-11, D-12)"
    - "Windows Inno Setup template (single shared source) carries new [Files] entries for .dorico_pt + .doricolib AND new [Code] Pascal logic that probes Dorico 6 -> 5 -> 4, extracts via Shell.Application.NameSpace.CopyHere, and copies into DefaultLibraryAdditions (NO spaces) (D-10, D-11, D-12)"
    - "All 8 cohort plugins' next packaging run (PKG on macOS + EXE on Windows) automatically inherits the dual-resource bundling — no per-plugin CMakeLists.txt or per-plugin packaging-config edits needed (PATTERNS.md observation 3+4)"
    - "Cross-platform validation matrix passes: 1 representative macOS install + 1 representative Windows install both surface 'Ouaricon Microtonal Suite' in Dorico's template picker and quarter-sharp C4 = +50¢ (D-15, D-16)"
    - "If Windows access is blocked, plan surfaces as hard halt rather than silently degrading to macOS-only (D-16)"
    - "Phase 24's 3-point gate (quarter-sharp C4 = +50¢, no attack zipper, NE correlated by noteId) re-passes per platform per representative plugin"
  artifacts:
    - path: ".claude/skills/plugin-packaging/references/pkg-creation.md"
      provides: "Single source of truth for macOS PKG postinstall — Section 4b extended with Microtonal Suite block (Pattern G)"
      contains: "Ouaricon-Microtonal-Suite.dorico_pt"
    - path: ".claude/skills/plugin-packaging/assets/inno-template.iss"
      provides: "Single source of truth for Windows Inno Setup — [Files] + [Code] extended (Pattern H)"
      contains: "ExtractZipTo"
    - path: ".claude/skills/plugin-packaging/references/inno-setup-creation.md"
      provides: "Documents 2 new template variables: MICROTONAL_SUITE_PT_PATH and MICROTONAL_SUITE_DORICOLIB_PATH"
      contains: "MICROTONAL_SUITE_PT_PATH"
    - path: ".planning/phases/25-package-docs/25-02-SUMMARY.md"
      provides: "Cross-platform validation matrix per D-15"
  key_links:
    - from: ".claude/skills/plugin-packaging/references/pkg-creation.md (Section 4b postinstall)"
      to: "Per-plugin PKG payload includes Ouaricon-Microtonal-Suite.dorico_pt and Ouaricon-VST3-NoteExpression.doricolib (sourced from build/ produced by Plan 25-01)"
      via: "cp + ditto -x -k from /tmp staging into ~/Library/Application Support/Steinberg/Dorico [N]/"
      pattern: "ditto -x -k"
    - from: ".claude/skills/plugin-packaging/assets/inno-template.iss [Code] section"
      to: "%APPDATA%\\Steinberg\\Dorico [N]\\PlaybackTemplateSpecs\\Ouaricon Microtonal Suite\\ AND %APPDATA%\\Steinberg\\Dorico [N]\\DefaultLibraryAdditions\\"
      via: "Shell.Application.NameSpace.CopyHere for zip extraction; FileCopy for .doricolib"
      pattern: "Shell.Application"
    - from: "Each of 8 cohort plugins' next /package or build-installer invocation"
      to: "Plan 25-01's two staged resources (build/Ouaricon-Microtonal-Suite.dorico_pt and modules/.../resources/library/Ouaricon-VST3-NoteExpression.doricolib)"
      via: "Both shared skill templates emit identical payload entries — propagation is implicit (PATTERNS.md observation 3)"
      pattern: "Ouaricon-VST3-NoteExpression.doricolib"
---

<objective>
Atomically extend the SHARED macOS PKG postinstall script and SHARED Windows Inno Setup template so all 8 cohort plugins' next packaging run bundles and dual-writes the Ouaricon Microtonal Suite resources. Validate the result with a cross-platform Dorico apply-template + quarter-sharp smoke matrix on macOS AND Windows.

Purpose: Realize Phase 25's distribution goal — every Ouaricon plugin's installer ships the canonical Microtonal Suite assets. PATTERNS.md observation 4: edit the shared `pkg-creation.md` + `inno-template.iss` ONCE -> propagates to all 8 plugins' next packaging run. No per-plugin CMakeLists.txt or per-plugin packaging-config edits needed (observation 3 — all 8 already consume the module).

Output: 3 MODIFIED skill files (single source of truth for both platforms) + 1 NEW SUMMARY.md with cross-platform validation matrix.
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
@.planning/phases/25-package-docs/25-01-SUMMARY.md

@.planning/phases/24-propagate/24-CONTEXT.md
@.planning/phases/24-propagate/24-08-final-sweep-PLAN.md

@.claude/skills/plugin-packaging/SKILL.md
@.claude/skills/plugin-packaging/SKILL-windows.md
@.claude/skills/plugin-packaging/references/pkg-creation.md
@.claude/skills/plugin-packaging/references/inno-setup-creation.md
@.claude/skills/plugin-packaging/assets/inno-template.iss

@CLAUDE.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md

<interfaces>
Plan 25-01 produces these two artifacts that Plan 25-02 consumes:
  1. build/Ouaricon-Microtonal-Suite.dorico_pt (zip; per-build-flavor — dev or prod CIDs)
  2. modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (static, recovered from cd2c2c6)

Both files MUST be findable by the per-platform packaging templates:
  - PKG side: copy from build/ into the packaging /tmp staging dir alongside the existing VST3+AU payload
  - Inno side: 2 new TEMPLATE_VARS resolve to absolute paths

Phase 24's 3-point smoke gate (D-07 from Phase 24):
  1. Quarter-sharp C4 = +50 cents at approximately 269.29 Hz (vs 12-TET 261.63 Hz)
  2. No attack zipper on the first sample of the tuned note
  3. NE correlated by noteId — polyphonic chord shows ONLY the quarter-sharp note detuned (others play 12-TET)

D-15 cross-platform matrix: macOS reference = O-Lyrica; Windows reference = O-Lyrica (recommended for parity, fall back to fastest-to-install if O-Lyrica build is blocked on Windows).

D-16: if Windows access is blocked, HARD HALT, do not silently ship macOS-only.
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Pre-flight — verify Plan 25-01 closeout and rebuild canonical .dorico_pt</name>
  <read_first>
    - .planning/phases/25-package-docs/25-01-SUMMARY.md (must exist; canary PASS recorded)
    - .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md (A2 + A4 + canary PASS)
    - modules/tuning/note-expression/module.yaml (must show version: 1.1.0)
    - modules/tuning/note-expression/install-microtonal-suite.cmake.in
    - modules/tuning/note-expression/module.cmake (must show v2 append after line 42)
  </read_first>
  <action>
    Pre-flight gate before touching any installer config.

    1. Verify Plan 25-01 closed cleanly:
       - 25-01-SUMMARY.md exists
       - Module version is 1.1.0 in both yaml files
       - Canary PASS recorded for O-Lyrica
       - Build artifact build/Ouaricon-Microtonal-Suite.dorico_pt exists and unzip -t is clean

    2. Rebuild the canonical .dorico_pt fresh from current source (idempotent — confirms reproducibility):

       cd build
       ninja OLyrica_VST3 O-Bells_VST3 O-IntonationPad_VST3 O-Prism_VST3 O-Wind_VST3 O-Reed_VST3 O-Bowed_VST3 O-Formant_VST3
       ninja ouaricon_microtonal_suite_pt
       unzip -t Ouaricon-Microtonal-Suite.dorico_pt
       unzip -l Ouaricon-Microtonal-Suite.dorico_pt

    3. Capture the absolute build paths the installers will need:

       PT_PATH=$(realpath build/Ouaricon-Microtonal-Suite.dorico_pt)
       LIB_PATH=$(realpath modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib)
       echo "PT_PATH=$PT_PATH"
       echo "LIB_PATH=$LIB_PATH"

    Record both paths in this task's notes — they're load-bearing for Tasks 2 and 3.

    Stop-on-first-failure (D-18): If pre-flight fails (Plan 25-01 incomplete, build broken, missing artifacts), HALT and escalate. Do NOT touch installer configs against an unfinished module-side pipeline.
  </action>
  <verify>
    <automated>test -f .planning/phases/25-package-docs/25-01-SUMMARY.md && grep -q "version: 1.1.0" modules/tuning/note-expression/module.yaml && test -f build/Ouaricon-Microtonal-Suite.dorico_pt && unzip -t build/Ouaricon-Microtonal-Suite.dorico_pt > /dev/null && unzip -l build/Ouaricon-Microtonal-Suite.dorico_pt | grep "PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml" > /dev/null && unzip -l build/Ouaricon-Microtonal-Suite.dorico_pt | grep "EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" > /dev/null && test -f modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib</automated>
  </verify>
  <acceptance_criteria>
    - 25-01-SUMMARY.md exists
    - module.yaml reports version 1.1.0
    - build/Ouaricon-Microtonal-Suite.dorico_pt exists and unzip -t is clean
    - unzip -l shows BOTH PlaybackTemplateSpecs and EndpointConfigs entries (no parent-dir wrapping)
    - library/Ouaricon-VST3-NoteExpression.doricolib exists at canonical module path
  </acceptance_criteria>
  <done>Plan 25-01 closeout verified, canonical .dorico_pt is fresh and well-formed, both source paths captured for installer template substitution.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 2: Extend macOS PKG postinstall — single source of truth (pkg-creation.md Section 4)</name>
  <read_first>
    - .claude/skills/plugin-packaging/references/pkg-creation.md (Section 4 — existing payload + postinstall blocks)
    - .claude/skills/plugin-packaging/SKILL.md (workflow context)
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern G (lines 422-475) and S-1 (cache clear)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-10, D-11, D-12)
  </read_first>
  <action>
    Extend `.claude/skills/plugin-packaging/references/pkg-creation.md` Section 4. Per Pattern G (PATTERNS.md lines 422-475), make TWO additive edits:

    EDIT (a): Section 4a — Payload Copy. After the existing two cp lines that copy VST3 + AU bundles into TEMP_DIR/payload/PLUGIN_NAME/, INSERT this block:

    ```bash
    # NEW (Phase 25 v2): Microtonal Suite Dorico template + library bundle
    # Sourced from the module's build-time output and canonical resources path.
    # PROJECT_ROOT is the repo root; the calling /package skill must export it.
    if [ -f "${PROJECT_ROOT}/build/Ouaricon-Microtonal-Suite.dorico_pt" ]; then
        cp "${PROJECT_ROOT}/build/Ouaricon-Microtonal-Suite.dorico_pt" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
    else
        echo "ERROR: Ouaricon-Microtonal-Suite.dorico_pt not found in build/ — run: ninja ouaricon_microtonal_suite_pt"
        exit 1
    fi

    if [ -f "${PROJECT_ROOT}/modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib" ]; then
        cp "${PROJECT_ROOT}/modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
    else
        echo "ERROR: Ouaricon-VST3-NoteExpression.doricolib not found in module resources"
        exit 1
    fi
    ```

    EDIT (b): Section 4b — Postinstall Script. Inside the existing `cat > "$TEMP_DIR/scripts/postinstall" << 'EOF' ... EOF` heredoc, INSERT the following block AFTER the existing `chown` lines (the `chown -R ... .component` line) and BEFORE the `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"` cleanup:

    ```bash
    # NEW (Phase 25 v2): Ouaricon Microtonal Suite Dorico template + library bundle
    # Dual-write per D-11: shared canonical (Ouaricon dir) + Dorico auto-scan (Steinberg dir)
    SHARED_DIR="$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite"
    mkdir -p "$SHARED_DIR"

    if [ -f "/tmp/PLUGIN_NAME_PLACEHOLDER/Ouaricon-Microtonal-Suite.dorico_pt" ]; then
        cp "/tmp/PLUGIN_NAME_PLACEHOLDER/Ouaricon-Microtonal-Suite.dorico_pt" "$SHARED_DIR/"
    fi
    if [ -f "/tmp/PLUGIN_NAME_PLACEHOLDER/Ouaricon-VST3-NoteExpression.doricolib" ]; then
        cp "/tmp/PLUGIN_NAME_PLACEHOLDER/Ouaricon-VST3-NoteExpression.doricolib" "$SHARED_DIR/"
    fi
    chown -R "$ACTUAL_USER:staff" "$SHARED_DIR"

    # Probe Dorico 6 -> 5 -> 4; install to first detected (D-12)
    for _v in 6 5 4; do
        DORICO_DIR="$USER_HOME/Library/Application Support/Steinberg/Dorico ${_v}"
        if [ -d "$DORICO_DIR" ]; then
            mkdir -p "$DORICO_DIR/PlaybackTemplateSpecs"
            # ditto -x -k unzips a .dorico_pt into the destination, preserving the
            # archive's internal PlaybackTemplateSpecs/ + EndpointConfigs/ subdirs.
            if [ -f "$SHARED_DIR/Ouaricon-Microtonal-Suite.dorico_pt" ]; then
                ditto -x -k "$SHARED_DIR/Ouaricon-Microtonal-Suite.dorico_pt" "$DORICO_DIR"
            fi
            # macOS dir name has SPACES (Pitfall 3). Installer creates if missing — Dorico does not auto-create.
            mkdir -p "$DORICO_DIR/Default Library Additions"
            if [ -f "$SHARED_DIR/Ouaricon-VST3-NoteExpression.doricolib" ]; then
                cp "$SHARED_DIR/Ouaricon-VST3-NoteExpression.doricolib" "$DORICO_DIR/Default Library Additions/"
            fi
            chown -R "$ACTUAL_USER:staff" "$DORICO_DIR/PlaybackTemplateSpecs/Ouaricon Microtonal Suite" 2>/dev/null || true
            chown -R "$ACTUAL_USER:staff" "$DORICO_DIR/EndpointConfigs/Ouaricon Microtonal Suite" 2>/dev/null || true
            chown -R "$ACTUAL_USER:staff" "$DORICO_DIR/Default Library Additions" 2>/dev/null || true
            echo "[Ouaricon] Microtonal Suite installed for Dorico ${_v}"
            break
        fi
    done
    ```

    Why `ditto -x -k` and not `unzip`: macOS PKG postinstalls run as root with a known-minimal PATH; `ditto` is in `/usr/bin` always. Pattern G chose `ditto -x -k` as the macOS-native idiom that handles extended attributes correctly.

    Add an explanatory sub-header in the markdown noting that Section 4 is now extended for Phase 25 v2 (e.g. "### Section 4b (extended for Phase 25 v2 — Microtonal Suite)" before the new block).

    Do NOT modify Sections 1, 2, 3, 5, 6 of pkg-creation.md or the pkgbuild invocation in 4c.
  </action>
  <verify>
    <automated>F=.claude/skills/plugin-packaging/references/pkg-creation.md && grep -q "Ouaricon-Microtonal-Suite.dorico_pt" "$F" && grep -q "Ouaricon-VST3-NoteExpression.doricolib" "$F" && grep -q "ditto -x -k" "$F" && grep -q "Default Library Additions" "$F" && grep -q "for _v in 6 5 4" "$F" && grep -q "Microtonal Suite" "$F" && grep -q "PROJECT_ROOT" "$F"</automated>
  </verify>
  <acceptance_criteria>
    - File contains the asset filename `Ouaricon-Microtonal-Suite.dorico_pt`
    - File contains the asset filename `Ouaricon-VST3-NoteExpression.doricolib`
    - File contains `ditto -x -k` (macOS-native extraction)
    - File contains the literal `Default Library Additions` (with SPACES — Pitfall 3 macOS branch)
    - File contains `for _v in 6 5 4` (descending Dorico version probe — D-12)
    - File contains `Microtonal Suite` literal (sanity)
    - File contains `PROJECT_ROOT` reference (payload sourcing precondition)
  </acceptance_criteria>
  <done>macOS PKG single source of truth extended; all 8 plugins' next /package run will inherit dual-resource bundling.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 3: Extend Windows Inno Setup template + creation reference (single source)</name>
  <read_first>
    - .claude/skills/plugin-packaging/assets/inno-template.iss (full file — existing [Files] line 47-49 + [Code] block lines 60-77)
    - .claude/skills/plugin-packaging/references/inno-setup-creation.md (template variable documentation conventions)
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern H (lines 478-580) — full Pascal extraction code provided
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-10, D-11, D-12; Pitfall 3 — DefaultLibraryAdditions has NO spaces on Windows)
    - .planning/phases/25-package-docs/25-RESEARCH.md "Pattern 2" (CID variance) and Pitfall 3
  </read_first>
  <action>
    Two coordinated edits to the SHARED Inno Setup configuration:

    EDIT (a): `.claude/skills/plugin-packaging/assets/inno-template.iss` — extend [Files] and [Code] sections.

    Append two NEW lines to the existing [Files] section (after the existing VST3 line at ~line 49):

    ```iss
    ; NEW (Phase 25 v2): Microtonal Suite Dorico template + library bundle
    ; Stage to %APPDATA%\Ouaricon\Microtonal Suite\ (canonical, editable shared dir)
    Source: "{{MICROTONAL_SUITE_PT_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
    Source: "{{MICROTONAL_SUITE_DORICOLIB_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
    ```

    REPLACE the existing [Code] block (lines 60-77) with the extended version per Pattern H lines 521-571. The new block adds an `ExtractZipTo` helper function (uses Windows Shell.Application COM — Inno Setup has no native unzip) and extends `CurStepChanged(ssPostInstall)` to probe Dorico 6 -> 5 -> 4 and dual-write:

    ```iss
    [Code]
    function ExtractZipTo(ZipPath, DestDir: String): Boolean;
    var
      Shell, ZipObj, Folder: Variant;
    begin
      // Use Windows Shell.Application COM to extract zip; Inno Setup has no native unzip.
      // Flag 16 = no UI prompts, overwrite existing files.
      Result := False;
      try
        Shell := CreateOleObject('Shell.Application');
        ZipObj := Shell.NameSpace(ZipPath);
        Folder := Shell.NameSpace(DestDir);
        Folder.CopyHere(ZipObj.Items, 16);
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
        // Existing: Ableton cache hint
        AbletonDir := ExpandConstant('{userappdata}\Ableton');
        if DirExists(AbletonDir) then
          Log('Ableton preferences directory found - plugin rescan will occur on next launch');

        // NEW (Phase 25 v2): probe Dorico 6 -> 5 -> 4 and dual-write Microtonal Suite
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
            // Windows: dir name has NO spaces (Pitfall 3) — vs macOS spaces.
            ForceDirectories(DoricoDir + '\DefaultLibraryAdditions');
            FileCopy(LibPath, DoricoDir + '\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib', False);
            Log('[Ouaricon] Microtonal Suite installed for Dorico ' + IntToStr(V));
            Break;
          end;
        end;
      end;
    end;
    ```

    Add a top-of-file template-comment line documenting the two new template variables (matching the existing `; - {{APP_GUID}} must be generated...` style):

    ```
    ; - {{MICROTONAL_SUITE_PT_PATH}} resolves to absolute path of build/Ouaricon-Microtonal-Suite.dorico_pt (built by Plan 25-01's ouaricon_microtonal_suite_pt CMake target)
    ; - {{MICROTONAL_SUITE_DORICOLIB_PATH}} resolves to absolute path of modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
    ```

    EDIT (b): `.claude/skills/plugin-packaging/references/inno-setup-creation.md`

    Add a subsection (or extend the existing Section 3.2 PowerShell template-variable substitution if present) documenting the 2 new template variables. PowerShell side must locate both files in the build tree and substitute their absolute paths into the .iss before `iscc` compile. Recommended snippet:

    ```powershell
    $MicrotonalSuitePtPath = (Resolve-Path "$ProjectRoot\build\Ouaricon-Microtonal-Suite.dorico_pt").Path
    $MicrotonalSuiteDoricolibPath = (Resolve-Path "$ProjectRoot\modules\tuning\note-expression\resources\library\Ouaricon-VST3-NoteExpression.doricolib").Path
    if (-not (Test-Path $MicrotonalSuitePtPath)) {
        Write-Error "Ouaricon-Microtonal-Suite.dorico_pt not found — run: cmake --build build --target ouaricon_microtonal_suite_pt"
        exit 1
    }
    $issContent = $issContent -replace '\{\{MICROTONAL_SUITE_PT_PATH\}\}', $MicrotonalSuitePtPath
    $issContent = $issContent -replace '\{\{MICROTONAL_SUITE_DORICOLIB_PATH\}\}', $MicrotonalSuiteDoricolibPath
    ```

    Do NOT modify any other sections of inno-setup-creation.md.
  </action>
  <verify>
    <automated>I=.claude/skills/plugin-packaging/assets/inno-template.iss && grep -q "MICROTONAL_SUITE_PT_PATH" "$I" && grep -q "MICROTONAL_SUITE_DORICOLIB_PATH" "$I" && grep -q "function ExtractZipTo" "$I" && grep -q "Shell.Application" "$I" && grep -q "for V := 6 downto 4" "$I" && grep -q "DefaultLibraryAdditions" "$I" && ! grep -q "Default Library Additions" "$I" && D=.claude/skills/plugin-packaging/references/inno-setup-creation.md && grep -q "MICROTONAL_SUITE_PT_PATH" "$D" && grep -q "MICROTONAL_SUITE_DORICOLIB_PATH" "$D"</automated>
  </verify>
  <acceptance_criteria>
    - inno-template.iss contains literal `MICROTONAL_SUITE_PT_PATH`
    - inno-template.iss contains literal `MICROTONAL_SUITE_DORICOLIB_PATH`
    - inno-template.iss contains `function ExtractZipTo` definition
    - inno-template.iss contains `Shell.Application` COM invocation
    - inno-template.iss contains `for V := 6 downto 4` (descending Dorico version probe)
    - inno-template.iss contains `DefaultLibraryAdditions` (NO spaces — Windows branch)
    - inno-template.iss does NOT contain `Default Library Additions` (with spaces — that is macOS-only; Pitfall 3 separation)
    - inno-setup-creation.md documents both template vars
  </acceptance_criteria>
  <done>Windows Inno single source of truth extended; PowerShell substitution path documented; directory-name asymmetry preserved.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: macOS validation — build PKG, install, run Dorico smoke (D-15 macOS half)</name>
  <read_first>
    - CLAUDE.md (lines 9-26 — cache-clear protocol)
    - .planning/phases/25-package-docs/25-PATTERNS.md S-1 + S-5
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-15, D-16; Phase 24 D-07 3-point gate)
    - .planning/phases/24-propagate/24-08-final-sweep-PLAN.md (3-point gate canonical formulation)
  </read_first>
  <what-built>
    A complete fresh PKG installer for the macOS reference plugin (O-Lyrica) built using the extended pkg-creation.md flow. Installer is run; Dorico is restarted; the Microtonal Suite template is auto-discovered and applied; quarter-sharp C4 is verified.
  </what-built>
  <how-to-verify>
    From repo root, with Plan 25-01 module-side already built:

    Step 1 — Build O-Lyrica fresh + ensure dorico_pt is built:
    ```
    cd build
    ninja OLyrica_VST3 OLyrica_AU
    ninja ouaricon_microtonal_suite_pt
    ```

    Step 2 — Cache-clear protocol per CLAUDE.md (S-1):
    ```
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/
    rm -rf ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Lyrica.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/O-Lyrica.component
    ```

    Step 3 — Install fresh O-Lyrica binaries (so /package preconditions are met):
    ```
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica*.vst3 ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/O-Lyrica*.component ~/Library/Audio/Plug-Ins/Components/
    ```

    Step 4 — Run /package O-Lyrica (uses the EXTENDED pkg-creation.md). The skill produces a signed branded PKG under plugins/O-Lyrica/dist/.

    Step 5 — Pre-state cleanup (so we observe a true install, not a re-run):
    ```
    rm -rf "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite"
    rm -rf "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite"
    rm -f "$HOME/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib"
    rm -rf "$HOME/Library/Application Support/Ouaricon/Microtonal Suite"
    ```

    Step 6 — Install the PKG (double-click in Finder, or `installer -pkg <path> -target /` if testing as admin). Capture install log under /var/log/install.log.

    Step 7 — Post-install verification:
    ```
    test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-Microtonal-Suite.dorico_pt" && echo "OK shared dorico_pt"
    test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib" && echo "OK shared doricolib"
    test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml" && echo "OK dorico spec"
    test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "OK dorico endpoint"
    test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib" && echo "OK dorico lib"
    ```

    Step 8 — Open Dorico 6. In `Play -> Playback Template`, confirm "Ouaricon Microtonal Suite" appears. Apply it to a fresh project. Quarter-sharp C4 smoke: write a quarter-sharp accidental on C4. Confirm playback at +50 cents (about 269.29 Hz, vs 12-TET 261.63 Hz).

    Step 9 — Polyphonic NE-correlation gate: write a chord with quarter-sharp C4 + natural E4 + natural G4. Confirm only C4 is detuned (E4 = 329.63 Hz, G4 = 392.00 Hz play 12-TET).

    Step 10 — Attack-zipper gate: visually verify the first ~10 ms of the C4 quarter-sharp envelope is smooth (no detune sweep from 261.63 -> 269.29 Hz).

    Append the full bash output + 3-point gate observations to `.planning/phases/25-package-docs/25-02-SUMMARY.md` under "## macOS Validation Result".

    Stop-on-first-failure (D-18): If any step fails, HALT and escalate to `25-02-macOS-FAIL-fix-PLAN.md`.
  </how-to-verify>
  <acceptance_criteria>
    - All 5 `test -f` "OK" markers echoed
    - Dorico 6 picker shows "Ouaricon Microtonal Suite"
    - Quarter-sharp C4 plays at +50 cents (matches Phase 23 LYR-03 + Phase 24 D-07 gate value)
    - Polyphonic chord shows only C4 detuned (NE-correlation by noteId proven)
    - No attack zipper observed
    - 25-02-SUMMARY.md "## macOS Validation Result" section recorded with timestamps + observed behavior
  </acceptance_criteria>
  <resume-signal>Type "macOS PASS — proceed to Windows" or "macOS FAIL — escalate to fix-plan"</resume-signal>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
  <action>
    (checkpoint task — see <what-built> and <how-to-verify> below for the verification protocol)
    
    WHAT BUILT:
    A complete fresh PKG installer for the macOS reference plugin (O-Lyrica) built using the extended pkg-creation.md flow. Installer is run; Dorico is restarted; the Microtonal Suite template is auto-discovered and applied; quarter-sharp C4 is verified.
    
    HOW TO VERIFY:
    From repo root, with Plan 25-01 module-side already built:
    
        Step 1 — Build O-Lyrica fresh + ensure dorico_pt is built:
        ```
        cd build
        ninja OLyrica_VST3 OLyrica_AU
        ninja ouaricon_microtonal_suite_pt
        ```
    
        Step 2 — Cache-clear protocol per CLAUDE.md (S-1):
        ```
        killall -9 AudioComponentRegistrar 2>/dev/null || true
        rm -rf ~/Library/Caches/AudioUnitCache/
        rm -rf ~/Library/Caches/com.apple.audiounits.cache
        rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Lyrica.vst3
        rm -rf ~/Library/Audio/Plug-Ins/Components/O-Lyrica.component
        ```
    
        Step 3 — Install fresh O-Lyrica binaries (so /package preconditions are met):
        ```
        cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica*.vst3 ~/Library/Audio/Plug-Ins/VST3/
        cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/O-Lyrica*.component ~/Library/Audio/Plug-Ins/Components/
        ```
    
        Step 4 — Run /package O-Lyrica (uses the EXTENDED pkg-creation.md). The skill produces a signed branded PKG under plugins/O-Lyrica/dist/.
    
        Step 5 — Pre-state cleanup (so we observe a true install, not a re-run):
        ```
        rm -rf "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite"
        rm -rf "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite"
        rm -f "$HOME/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib"
        rm -rf "$HOME/Library/Application Support/Ouaricon/Microtonal Suite"
        ```
    
        Step 6 — Install the PKG (double-click in Finder, or `installer -pkg <path> -target /` if testing as admin). Capture install log under /var/log/install.log.
    
        Step 7 — Post-install verification:
        ```
        test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-Microtonal-Suite.dorico_pt" && echo "OK shared dorico_pt"
        test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib" && echo "OK shared doricolib"
        test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml" && echo "OK dorico spec"
        test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "OK dorico endpoint"
        test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib" && echo "OK dorico lib"
        ```
    
        Step 8 — Open Dorico 6. In `Play -> Playback Template`, confirm "Ouaricon Microtonal Suite" appears. Apply it to a fresh project. Quarter-sharp C4 smoke: write a quarter-sharp accidental on C4. Confirm playback at +50 cents (about 269.29 Hz, vs 12-TET 261.63 Hz).
    
        Step 9 — Polyphonic NE-correlation gate: write a chord with quarter-sharp C4 + natural E4 + natural G4. Confirm only C4 is detuned (E4 = 329.63 Hz, G4 = 392.00 Hz play 12-TET).
    
        Step 10 — Attack-zipper gate: visually verify the first ~10 ms of the C4 quarter-sharp envelope is smooth (no detune sweep from 261.63 -> 269.29 Hz).
    
        Append the full bash output + 3-point gate observations to `.planning/phases/25-package-docs/25-02-SUMMARY.md` under "## macOS Validation Result".
    
        Stop-on-first-failure (D-18): If any step fails, HALT and escalate to `25-02-macOS-FAIL-fix-PLAN.md`.
  </action>
  <verify><automated>see acceptance_criteria above (human-verified checkpoint; automated gate is on the recorded VERIFICATION.md file)</automated></verify>
  <done>All <acceptance_criteria> conditions above are satisfied.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 5: Windows validation — build EXE, install, run Dorico smoke (D-15 Windows half)</name>
  <read_first>
    - CLAUDE.md (lines 31-41 — Windows install + cache-clear protocol)
    - .planning/phases/25-package-docs/25-PATTERNS.md S-1 (Windows half) + Pattern H
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-15, D-16 — D-16 hard halt if Windows blocked)
    - .claude/skills/plugin-packaging/SKILL-windows.md
  </read_first>
  <what-built>
    A fresh EXE installer for the Windows reference plugin (recommend O-Lyrica for cross-platform parity) built using the extended inno-template.iss + inno-setup-creation.md PowerShell substitution. Installer is run on Windows; Dorico is restarted; the Microtonal Suite template is auto-discovered and applied; quarter-sharp C4 is verified.
  </what-built>
  <how-to-verify>
    Pre-flight gate: confirm Windows machine with Dorico 6 (or 5 or 4) is accessible. If NO Windows access, HALT this task per D-16 — surface as hard halt, do NOT silently degrade to macOS-only.

    From repo root on Windows (PowerShell):

    Step 1 — Build O-Lyrica VST3:
    ```powershell
    cmake --build build --config Release --target OLyrica_VST3 ouaricon_microtonal_suite_pt --parallel
    ```

    Step 2 — Cache-clear protocol per CLAUDE.md Windows section:
    ```powershell
    Remove-Item -Recurse -Force "$env:COMMONPROGRAMFILES\VST3\O-Lyrica.vst3" -ErrorAction SilentlyContinue
    Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" -Force -ErrorAction SilentlyContinue
    ```

    Step 3 — Run the build-installer skill for O-Lyrica. The skill invokes the EXTENDED inno-template.iss + the EXTENDED PowerShell substitution from inno-setup-creation.md. Output: plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe.

    Step 4 — Pre-state cleanup:
    ```powershell
    Remove-Item -Recurse -Force "$env:APPDATA\Steinberg\Dorico 6\PlaybackTemplateSpecs\Ouaricon Microtonal Suite" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$env:APPDATA\Steinberg\Dorico 6\EndpointConfigs\Ouaricon Microtonal Suite" -ErrorAction SilentlyContinue
    Remove-Item -Force "$env:APPDATA\Steinberg\Dorico 6\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$env:APPDATA\Ouaricon\Microtonal Suite" -ErrorAction SilentlyContinue
    ```

    Step 5 — Run the EXE installer (elevated). Inno's [Code] section logs to %TEMP%\Setup Log YYYY-MM-DD.txt; capture the lines starting with "[Ouaricon]".

    Step 6 — Post-install verification:
    ```powershell
    Test-Path "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-Microtonal-Suite.dorico_pt"
    Test-Path "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib"
    Test-Path "$env:APPDATA\Steinberg\Dorico 6\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\playbacktemplatespec.xml"
    Test-Path "$env:APPDATA\Steinberg\Dorico 6\EndpointConfigs\Ouaricon Microtonal Suite\endpointconfig.xml"
    Test-Path "$env:APPDATA\Steinberg\Dorico 6\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib"
    ```
    All 5 must return True.

    Step 7 — Open Dorico 6 on Windows. Same 3-point gate as Task 4 (apply template, quarter-sharp C4 = +50 cents, polyphonic NE-correlation, no attack zipper).

    Append "## Windows Validation Result" section to 25-02-SUMMARY.md.

    Stop-on-first-failure (D-18 + D-16): If extraction failed (Shell.Application COM error), or directory creation failed (admin/permission issue), or Dorico picker did not show the template, HALT and escalate. The fix-plan must address the specific Windows-side failure mode.
  </how-to-verify>
  <acceptance_criteria>
    - Windows access confirmed (D-16 hard halt would have fired otherwise)
    - All 5 Test-Path calls return True
    - Inno Setup log contains `[Ouaricon] Microtonal Suite installed for Dorico` line
    - Dorico picker on Windows shows "Ouaricon Microtonal Suite"
    - Quarter-sharp C4 plays at +50 cents on Windows
    - Polyphonic chord NE-correlation observed on Windows
    - No attack zipper on Windows
    - 25-02-SUMMARY.md "## Windows Validation Result" recorded
  </acceptance_criteria>
  <resume-signal>Type "Windows PASS — finalize SUMMARY" or "Windows FAIL — escalate" or "Windows BLOCKED — D-16 hard halt"</resume-signal>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
  <action>
    (checkpoint task — see <what-built> and <how-to-verify> below for the verification protocol)
    
    WHAT BUILT:
    A fresh EXE installer for the Windows reference plugin (recommend O-Lyrica for cross-platform parity) built using the extended inno-template.iss + inno-setup-creation.md PowerShell substitution. Installer is run on Windows; Dorico is restarted; the Microtonal Suite template is auto-discovered and applied; quarter-sharp C4 is verified.
    
    HOW TO VERIFY:
    Pre-flight gate: confirm Windows machine with Dorico 6 (or 5 or 4) is accessible. If NO Windows access, HALT this task per D-16 — surface as hard halt, do NOT silently degrade to macOS-only.
    
        From repo root on Windows (PowerShell):
    
        Step 1 — Build O-Lyrica VST3:
        ```powershell
        cmake --build build --config Release --target OLyrica_VST3 ouaricon_microtonal_suite_pt --parallel
        ```
    
        Step 2 — Cache-clear protocol per CLAUDE.md Windows section:
        ```powershell
        Remove-Item -Recurse -Force "$env:COMMONPROGRAMFILES\VST3\O-Lyrica.vst3" -ErrorAction SilentlyContinue
        Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" -Force -ErrorAction SilentlyContinue
        ```
    
        Step 3 — Run the build-installer skill for O-Lyrica. The skill invokes the EXTENDED inno-template.iss + the EXTENDED PowerShell substitution from inno-setup-creation.md. Output: plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe.
    
        Step 4 — Pre-state cleanup:
        ```powershell
        Remove-Item -Recurse -Force "$env:APPDATA\Steinberg\Dorico 6\PlaybackTemplateSpecs\Ouaricon Microtonal Suite" -ErrorAction SilentlyContinue
        Remove-Item -Recurse -Force "$env:APPDATA\Steinberg\Dorico 6\EndpointConfigs\Ouaricon Microtonal Suite" -ErrorAction SilentlyContinue
        Remove-Item -Force "$env:APPDATA\Steinberg\Dorico 6\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib" -ErrorAction SilentlyContinue
        Remove-Item -Recurse -Force "$env:APPDATA\Ouaricon\Microtonal Suite" -ErrorAction SilentlyContinue
        ```
    
        Step 5 — Run the EXE installer (elevated). Inno's [Code] section logs to %TEMP%\Setup Log YYYY-MM-DD.txt; capture the lines starting with "[Ouaricon]".
    
        Step 6 — Post-install verification:
        ```powershell
        Test-Path "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-Microtonal-Suite.dorico_pt"
        Test-Path "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib"
        Test-Path "$env:APPDATA\Steinberg\Dorico 6\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\playbacktemplatespec.xml"
        Test-Path "$env:APPDATA\Steinberg\Dorico 6\EndpointConfigs\Ouaricon Microtonal Suite\endpointconfig.xml"
        Test-Path "$env:APPDATA\Steinberg\Dorico 6\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib"
        ```
        All 5 must return True.
    
        Step 7 — Open Dorico 6 on Windows. Same 3-point gate as Task 4 (apply template, quarter-sharp C4 = +50 cents, polyphonic NE-correlation, no attack zipper).
    
        Append "## Windows Validation Result" section to 25-02-SUMMARY.md.
    
        Stop-on-first-failure (D-18 + D-16): If extraction failed (Shell.Application COM error), or directory creation failed (admin/permission issue), or Dorico picker did not show the template, HALT and escalate. The fix-plan must address the specific Windows-side failure mode.
  </action>
  <verify><automated>see acceptance_criteria above (human-verified checkpoint; automated gate is on the recorded VERIFICATION.md file)</automated></verify>
  <done>All <acceptance_criteria> conditions above are satisfied.</done>
</task>

<task type="auto">
  <name>Task 6: Author 25-02-SUMMARY.md cross-platform validation matrix</name>
  <read_first>
    - .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md (matrix shape reference)
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern I (atomic-sweep + per-plugin table)
    - Tasks 4 + 5 outputs already appended in 25-02-SUMMARY.md
  </read_first>
  <action>
    Finalize `.planning/phases/25-package-docs/25-02-SUMMARY.md`. The earlier verification tasks have been appending sections to it. This task structures the file into a final, agent-readable matrix.

    Required sections (in order):
    1. Front-matter / header (phase, plan, status, completion timestamp)
    2. Summary (1-paragraph: "All 8 cohort plugins inherit Microtonal Suite bundling via shared skill templates; macOS PKG postinstall + Windows Inno [Code] logic dual-write to user systems; cross-platform validation gate passed.")
    3. ## Files Modified — list of 3 files: pkg-creation.md, inno-template.iss, inno-setup-creation.md (with brief diff intent per file)
    4. ## Cross-Platform Validation Matrix (D-15) — table with columns: Platform | Reference plugin | PKG/EXE built | Auto-discovery | Template in picker | Quarter-sharp C4 = +50¢ | Polyphonic NE | Attack zipper | Result. Rows: macOS / O-Lyrica, Windows / O-Lyrica.
    5. ## Per-File Truth Table — list each of the 5 destination paths (3 macOS + 3 Windows = 6 actually) and the test-f / Test-Path result
    6. ## Affected Plugins (8) — table with column "Will inherit on next /package run" — list all 8 cohort plugins with Y for each
    7. ## Outstanding work — note that the 7 non-O-Lyrica cohort plugins inherit the bundling automatically, but their actual /package or build-installer runs happen organically (next time their CHANGELOGs are bumped), NOT in this plan; INST-03 is satisfied structurally
    8. ## Decisions Honored — table mapping D-XX decision to evidence in this plan
    9. ## Stop-on-first-failure log — list any escalations (Tasks 4/5 fix-plans triggered, if any)

    Use the `gsd-sdk query` SUMMARY template if available; otherwise mirror Phase 24 final-sweep SUMMARY.md style.
  </action>
  <verify>
    <automated>S=.planning/phases/25-package-docs/25-02-SUMMARY.md && grep -q "Cross-Platform Validation Matrix" "$S" && grep -q "macOS" "$S" && grep -q "Windows" "$S" && grep -q "Quarter-sharp" "$S" && grep -q "O-Lyrica" "$S" && for plug in O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do grep -q "$plug" "$S" || { echo "MISSING PLUGIN IN AFFECTED TABLE: $plug"; exit 1; }; done && echo "ALL 8 PLUGINS RECORDED IN SUMMARY"</automated>
  </verify>
  <acceptance_criteria>
    - File contains "Cross-Platform Validation Matrix" header
    - File names both macOS and Windows
    - File contains "Quarter-sharp" gate descriptor
    - All 8 cohort plugin names appear in the file
    - Final command echoes "ALL 8 PLUGINS RECORDED IN SUMMARY"
  </acceptance_criteria>
  <done>SUMMARY.md authored as cross-platform matrix; INST-03 + INST-04 satisfaction documented; downstream consumers (Plan 25-03 doc references, future improve-cycle SUMMARYs) can read it.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| macOS PKG postinstall (root) -> user filesystem under ~/Library/Application Support/ | Postinstall runs as root; writes to user-owned paths under $USER_HOME (derived from /dev/console). |
| Windows EXE installer (admin elevated) -> %APPDATA% under user's profile | Inno installer runs elevated; %APPDATA% expands to the elevating user's roaming profile. |
| ditto -x -k extracting .dorico_pt zip into Steinberg dir | Zip contents are repo-controlled (built by Plan 25-01 from source-controlled XML). No external zip enters the pipeline. |
| Shell.Application COM in Windows Inno [Code] | Same — zip is repo-controlled. Shell.Application has no path-traversal protection but the destination DoricoDir is controlled. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-02-01 | Tampering / Path traversal | macOS postinstall ditto extraction destination | mitigate | Destination DORICO_DIR is hardcoded probe of `~/Library/Application Support/Steinberg/Dorico 6/` (etc.). Zip contents (PlaybackTemplateSpecs/, EndpointConfigs/) are repo-controlled, validated by Wave 0 A4 in Plan 25-01 + by `unzip -l` gates in Task 1. |
| T-25-02-02 | Tampering / Path traversal | Windows Inno Shell.Application.NameSpace.CopyHere extraction | mitigate | Same — DoricoDir is hardcoded ExpandConstant('{userappdata}\Steinberg\Dorico N'); zip entries are repo-controlled. ExtractZipTo helper logs failures via try/except. |
| T-25-02-03 | Elevation of privilege | macOS postinstall runs as root; Windows installer elevated | accept | Both installers run with the privileges the platform's PKG/EXE convention demands. Postinstall confines writes to USER_HOME-derived paths and chowns back to ACTUAL_USER. Inno writes to {userappdata} (per-user). No system-wide writes outside per-user profile. |
| T-25-02-04 | Idempotency / partial-state | Mid-install crash after .dorico_pt extracted but before .doricolib copied | mitigate | Each cp / FileCopy / ditto invocation is independent; Dorico tolerates partial install (template appears but library missing -> warns but the .dorico_pt's embedded playbacktemplatedeps.doricolib still carries the expression-map definition, so apply still works). Re-running the installer is safe (idempotent overwrite per D-10). |
| T-25-02-05 | Stale-asset risk | Older Microtonal Suite version remains after upgrade | mitigate | `file(COPY)` and `cp` overwrite in place. `ditto -x -k` likewise overwrites. The Dorico-side extraction always replaces files of the same name. Version tracking is via the installer version (PKG identifier + EXE version), not per-resource. |
| T-25-02-06 | Information disclosure | Dev CIDs from a misbuilt installer leak to prod machines | mitigate | Plan 25-01's `ouaricon_extract_vst3_cids` honors OUARICON_DEV_SUFFIX (S-3); dev installers ship dev CIDs, prod installers ship prod CIDs. The /package skill must be invoked against a prod build before any public release; this plan tests against dev builds for the validation gate (correct, since Dorico tests against installed dev plugins). |
| T-25-02-07 | Denial of service | Inno Shell.Application COM failure on locked-down Windows hosts | accept | ExtractZipTo's try/except catches and logs the failure. Worst case: extraction fails, user sees install completed but template absent. Manual import fallback (README-microtonal-suite.txt from Plan 25-01) works as backstop. Documented in DOCS-04 (Plan 25-03). |
| T-25-02-08 | Spoofing | If user has malicious zip at SharedDir before install (race) | accept | Installer overwrites SharedDir contents with its own bundled assets; pre-existing files are replaced. Window for race is sub-second. Acceptable risk. |
</threat_model>

<verification>
- Task 1 pre-flight: gates the entire plan; failure HALTS Wave 2 entirely.
- Tasks 2 + 3 (template edits): grep gates on shared skill files. The directory-name asymmetry (Default Library Additions on macOS, DefaultLibraryAdditions on Windows) is verified by separate grep checks per file.
- Task 4 (macOS validation): full Phase 24 D-07 3-point gate re-run — quarter-sharp + polyphonic NE-correlation + no attack zipper. Plus 5 dual-write paths verified.
- Task 5 (Windows validation): same 3-point gate on Windows. D-16 hard halt if Windows blocked.
- Task 6 (SUMMARY): structural gate — all 8 plugins enumerated even though only 1 was actually installed (the other 7 inherit on their next /package run).
</verification>

<success_criteria>
- macOS PKG postinstall (single shared source) extended with Microtonal Suite block (Pattern G)
- Windows Inno template (single shared source) extended with [Files] entries + ExtractZipTo helper + Dorico-version probe in [Code] (Pattern H)
- inno-setup-creation.md documents 2 new template variables
- macOS validation (Task 4): O-Lyrica PKG installs cleanly, dual-write succeeds, Dorico 6 picker shows the template, quarter-sharp C4 at +50 cents
- Windows validation (Task 5): O-Lyrica EXE installs cleanly, dual-write succeeds, Dorico 6 picker shows the template, quarter-sharp C4 at +50 cents — OR D-16 hard halt surfaced if Windows blocked
- 25-02-SUMMARY.md cross-platform matrix recorded
- Pitfall 3 (directory-name asymmetry) honored in BOTH platforms' configs
- Pitfall 5 (Pitfall 5 — wrapping parent dir in zip) verified at Task 1 by `unzip -l`
- INST-03 satisfied structurally for all 8 plugins (skill-template propagation), INST-04 satisfied via Plan 25-01's README + this plan's macOS/Windows fallback in the README content
</success_criteria>

<output>
After completion, ensure `.planning/phases/25-package-docs/25-02-SUMMARY.md` is finalized per Task 6 structure. Update `.planning/STATE.md` with Plan 25-02 completion + cross-platform matrix result + any escalation breadcrumbs.
</output>
</content>
