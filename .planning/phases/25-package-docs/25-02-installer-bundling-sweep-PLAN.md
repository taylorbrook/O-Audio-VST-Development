---
phase: 25-package-docs
plan: 02
type: execute
wave: 2
depends_on: [25-01]
files_modified:
  - .claude/skills/plugin-packaging/references/pkg-creation.md
  - .claude/skills/plugin-packaging/SKILL-windows.md
  - .claude/skills/plugin-packaging/assets/inno-template.iss
  - plugins/O-Lyrica/dist/installer.iss
  - plugins/O-Bells/dist/installer.iss
  - plugins/O-IntonationPad/dist/installer.iss
  - plugins/O-Prism/dist/installer.iss
  - plugins/O-Wind/dist/installer.iss
  - plugins/O-Reed/dist/installer.iss
  - plugins/O-Bowed/dist/installer.iss
  - plugins/O-Formant/dist/installer.iss
  - plugins/O-Lyrica/dist/
  - plugins/O-Bells/dist/
  - plugins/O-IntonationPad/dist/
  - plugins/O-Prism/dist/
  - plugins/O-Wind/dist/
  - plugins/O-Reed/dist/
  - plugins/O-Bowed/dist/
  - plugins/O-Formant/dist/
autonomous: false
requirements: [INST-03, INST-04]
must_haves:
  truths:
    - "All 8 affected plugins (O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) ship a macOS PKG installer that bundles Ouaricon-VST3-NoteExpression.doricoexpmap and writes it to BOTH ~/Library/Application Support/Ouaricon/Expression Maps/ AND ~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/ on user install"
    - "All 8 affected plugins ship a Windows EXE installer (Inno Setup) source-of-truth installer.iss that bundles Ouaricon-VST3-NoteExpression.doricoexpmap and writes it to BOTH %APPDATA%\\Ouaricon\\Expression Maps\\ AND %APPDATA%\\Steinberg\\Dorico [N]\\Expression Maps\\User\\ on user install"
    - "All 8 macOS PKG artifacts are physically materialized at plugins/<Plugin>/dist/<Plugin>-OuariconAudio.pkg via /package <Plugin>; each PKG's payload manifest contains the .doricoexpmap (per BLOCKER #1 from checker review)"
    - "README-doricoexpmap.txt is co-installed alongside the .doricoexpmap at the Ouaricon shared resources path on both platforms (INST-04 fallback)"
    - "Cross-platform Dorico smoke validation: ONE representative plugin passes the 3-point Dorico quarter-sharp gate (D-07) on macOS AND the same gate on a Windows host. Hard halt if Windows access is blocked at execute time — do NOT silently fall back to macOS-only validation."
    - "Validation matrix spot-checks at least 3 plugins on macOS via PKG installation: O-Lyrica + O-Reed + O-Formant (covers different version conventions: VERSION quoted, PLUGIN_VERSION quoted, VERSION unquoted; plus the MPE-helper-based path)"
    - "macOS PKG packaging skill (`pkg-creation.md`) and Windows Inno Setup template (`assets/inno-template.iss`) updated to consume the module-staged .doricoexpmap so per-plugin installer configs require no manual one-off edits — the change propagates by template, not by per-plugin hand-editing of XML payloads"
    - "Windows path issue resolved: Inno template uses Inno preprocessor #define RepoRoot driven from /DRepoRoot iscc command-line on the Windows host (per BLOCKER #4/#5 from checker review). No mixed forward/backslash paths leak through; Source: lines use {#RepoRoot}\\modules\\... convention resolved at iscc compile time on the Windows machine."
    - "Each plugin's `plugins/<Plugin>/dist/installer.iss` is regenerated with the new template, has a non-empty MyAppVersion matching the registry expectation (per BLOCKER #6 — generalized version regex + per-plugin sanity check), and is committed (Windows source-of-truth for reproducibility)"
  artifacts:
    - path: ".claude/skills/plugin-packaging/references/pkg-creation.md"
      provides: "macOS PKG-creation reference updated with the .doricoexpmap payload step + dual-write postinstall logic + Dorico version probe"
      contains: "Ouaricon-VST3-NoteExpression.doricoexpmap"
    - path: ".claude/skills/plugin-packaging/SKILL-windows.md"
      provides: "Windows packaging skill updated with /DRepoRoot iscc command-line argument + #define RepoRoot Inno preprocessor convention"
      contains: "DRepoRoot"
    - path: ".claude/skills/plugin-packaging/assets/inno-template.iss"
      provides: "Windows Inno Setup template updated with #define RepoRoot stub + dual-write [Files] entries using {#RepoRoot} + Pascal [Code] block for Dorico version detection"
      contains: "{#RepoRoot}"
    - path: "plugins/O-Lyrica/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Lyrica with .doricoexpmap dual-write (uses {#RepoRoot} template)"
    - path: "plugins/O-Lyrica/dist/O-Lyrica-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Lyrica via /package O-Lyrica (BLOCKER #1)"
    - path: "plugins/O-Bells/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Bells with .doricoexpmap dual-write"
    - path: "plugins/O-Bells/dist/O-Bells-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Bells via /package O-Bells (BLOCKER #1)"
    - path: "plugins/O-IntonationPad/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-IntonationPad with .doricoexpmap dual-write"
    - path: "plugins/O-IntonationPad/dist/O-IntonationPad-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-IntonationPad via /package O-IntonationPad (BLOCKER #1)"
    - path: "plugins/O-Prism/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Prism with .doricoexpmap dual-write"
    - path: "plugins/O-Prism/dist/O-Prism-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Prism via /package O-Prism (BLOCKER #1)"
    - path: "plugins/O-Wind/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Wind with .doricoexpmap dual-write"
    - path: "plugins/O-Wind/dist/O-Wind-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Wind via /package O-Wind (BLOCKER #1)"
    - path: "plugins/O-Reed/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Reed with .doricoexpmap dual-write"
    - path: "plugins/O-Reed/dist/O-Reed-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Reed via /package O-Reed (BLOCKER #1)"
    - path: "plugins/O-Bowed/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Bowed with .doricoexpmap dual-write"
    - path: "plugins/O-Bowed/dist/O-Bowed-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Bowed via /package O-Bowed (BLOCKER #1)"
    - path: "plugins/O-Formant/dist/installer.iss"
      provides: "Regenerated Inno Setup script for O-Formant with .doricoexpmap dual-write"
    - path: "plugins/O-Formant/dist/O-Formant-OuariconAudio.pkg"
      provides: "Materialized macOS PKG for O-Formant via /package O-Formant (BLOCKER #1)"
  key_links:
    - from: "macOS PKG payload step in pkg-creation.md"
      to: "modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap"
      via: "cp -R … from module-owned canonical to PKG payload directory before pkgbuild"
      pattern: "modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression\\.doricoexpmap"
    - from: "macOS PKG postinstall script"
      to: "user's ~/Library/Application Support/Ouaricon/Expression Maps/ AND ~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/"
      via: "Bash dual-write with Dorico version probe (descending 6→5→4)"
      pattern: "Steinberg/Dorico"
    - from: "Inno Setup [Files] entries using {#RepoRoot}"
      to: "%APPDATA%\\Ouaricon\\Expression Maps\\ AND %APPDATA%\\Steinberg\\Dorico [N]\\Expression Maps\\User\\"
      via: "iscc /DRepoRoot=<repo-path> at compile time on Windows host + Pascal [Code] section for runtime version probe"
      pattern: "userappdata.*Ouaricon"
---

<objective>
Sweep the canonical .doricoexpmap (authored + plumbed in Plan 25-01) into all 8 affected plugins' installer pipelines (PKG on macOS via the `plugin-packaging` skill, EXE on Windows via the `build-installer`/`plugin-packaging-windows` skill). Per D-04 and D-11, the change is template-level: extend the macOS `pkg-creation.md` reference and the Windows `inno-template.iss` so all 8 plugins inherit dual-write installer behavior without per-plugin hand-edits beyond regenerating each `installer.iss` from the updated template. Materialize all 8 macOS PKG artifacts (per BLOCKER #1 from checker review). Validate end-to-end on BOTH platforms with a representative plugin per platform (D-09: per-platform symmetric validation; hard halt if Windows access blocked).

Purpose: Close INST-03 (all 8 installers bundle the .doricoexpmap; all 8 PKGs are physically materialized; spot-checked across version conventions) and INST-04 (README fallback emitted alongside) for the entire v1.5 cohort. This is the atomic sweep mirroring Phase 24's `24-08-final-sweep-PLAN.md` shape — one commit covers all 8 plugins' installer changes plus the cross-platform validation matrix.

Output: Three skill-template updates (pkg-creation.md macOS, inno-template.iss + SKILL-windows.md Windows), eight regenerated Inno Setup `installer.iss` files committed, eight macOS PKG artifacts materialized, and a per-platform validation matrix recorded in the SUMMARY.md.
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
@.planning/phases/24-propagate/24-VERIFICATION.md
@.claude/skills/plugin-packaging/SKILL.md
@.claude/skills/plugin-packaging/SKILL-windows.md
@.claude/skills/plugin-packaging/references/pkg-creation.md
@.claude/skills/plugin-packaging/assets/inno-template.iss
@.claude/skills/build-installer/SKILL.md
@modules/tuning/note-expression/module.cmake
@modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap
@modules/tuning/note-expression/resources/README-doricoexpmap.txt
@modules/registry.yaml
@plugins/O-Tremolo/dist/installer.iss
@CLAUDE.md
</context>

<interfaces>
<!-- Key contracts the executor needs. Use these directly — no codebase exploration required. -->

From `.claude/skills/plugin-packaging/references/pkg-creation.md` Section 4a (current shape — the executor extends this):
```bash
# 4a. Copy Binaries to Payload
cp -R "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
cp -R "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
```
And Section 4b (postinstall script — the executor extends with dual-write logic before `exit 0`):
```bash
# Postinstall already handles VST3 + AU copy + chown.
# This plan adds: copy of Ouaricon-VST3-NoteExpression.doricoexpmap to two locations.
```

From `.claude/skills/plugin-packaging/assets/inno-template.iss` (current `[Files]` block — line 47-49 — the executor extends):
```inno
[Files]
; Install the VST3 bundle (entire directory tree)
Source: "{{VST3_SOURCE_PATH}}\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
```

From `plugins/O-Tremolo/dist/installer.iss` (existing reference of a regenerated installer.iss — note Windows-rooted backslash paths via H:\dev\... — only valid because it was generated on the Windows host):
```inno
[Files]
Source: "H:\dev\VST-development\build\plugins\O-Tremolo\OuariconTremolo_artefacts\Release\VST3\O-Tremolo-dev.vst3\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
```

The 8 plugins' CMake target name → directory name + version identifier conventions (verified by grep of each `plugins/<Plugin>/CMakeLists.txt`):

| Plugin (dir)       | CMake target  | Version identifier        | Expected version |
|--------------------|---------------|---------------------------|------------------|
| O-Lyrica           | OLyrica       | VERSION "2.3.0"           | 2.3.0            |
| O-Bells            | O-Bells       | PLUGIN_VERSION "4.1.0"    | 4.1.0            |
| O-IntonationPad    | O-IntonationPad | PLUGIN_VERSION "2.8.0"  | 2.8.0            |
| O-Prism            | O-Prism       | VERSION 1.17.0 (unquoted) | 1.17.0           |
| O-Wind             | O-Wind        | PLUGIN_VERSION "1.16.0"   | 1.16.0           |
| O-Reed             | O-Reed        | PLUGIN_VERSION "1.1.0"    | 1.1.0            |
| O-Bowed            | O-Bowed       | PLUGIN_VERSION "1.3.0"    | 1.3.0            |
| O-Formant          | O-Formant     | VERSION 1.25.0 (unquoted) | 1.25.0           |

**Heterogeneous version conventions (the BLOCKER #6 reality):** plugins use `VERSION` or `PLUGIN_VERSION`; some quoted, some unquoted. The version-extraction regex MUST handle BOTH identifier names AND BOTH quote conventions. Per-plugin sanity check is required (assert extracted version matches the table above; halt with the specific plugin name on mismatch).

From `modules/tuning/note-expression/module.cmake` (Plan 25-01's install rules — the macOS PKG and Windows EXE installers REPLICATE this dual-write logic at their respective install times. The CMake install() rules fire for `cmake --install`; pkgbuild and Inno Setup do not honor them, so this plan re-implements the same dual-write logic in the platform-native installer mechanisms):
- macOS shared: `~/Library/Application Support/Ouaricon/Expression Maps/`
- macOS Dorico scan: `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` (N descending probe: 6, 5, 4)
- Windows shared: `%APPDATA%\Ouaricon\Expression Maps\`
- Windows Dorico scan: `%APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\` (N descending probe: 6, 5, 4)

Inno Setup constants for Windows paths:
- `{userappdata}` → `%APPDATA%` of the running user
- `{commonpf}` → `C:\Program Files\Common Files`
- Inno's Pascal `[Code]` section supports `DirExists()` for runtime version probe, and `FileCopy()` for explicit dual-write
- `#define` preprocessor directives at the top of `.iss` are resolved at iscc compile time on the host the iscc runs on (the Windows machine)
- iscc `/D<name>=<value>` command-line argument injects a preprocessor define at compile time — the canonical way to parameterize a single .iss with host-specific paths without requiring per-host file edits

**BLOCKER #4/#5 resolution (Inno preprocessor approach):** Instead of doing macOS-side sed substitution of `{{REPO_ROOT}}` (which produces broken mixed slashes when forward-slash macOS path is concatenated to backslash template paths), the .iss source-of-truth files use `{#RepoRoot}` references resolved at iscc compile time on the Windows host. The Windows host invokes iscc with `iscc /DRepoRoot=H:\dev\VST-development plugins\O-Lyrica\dist\installer.iss`. The .iss declares `#define RepoRoot ""` as a default (empty stub) so the file is syntactically valid even without the /D argument — but the [Files] Source: paths only resolve when /DRepoRoot is supplied. This makes the .iss path-relocatable and avoids macOS↔Windows path-format conflicts.
</interfaces>

<tasks>

<task type="auto">
  <name>Task 1: Extend macOS PKG packaging reference + Windows Inno Setup template (with #define RepoRoot preprocessor approach per BLOCKER #4/#5) + SKILL-windows.md /DRepoRoot documentation</name>
  <files>
    .claude/skills/plugin-packaging/references/pkg-creation.md,
    .claude/skills/plugin-packaging/assets/inno-template.iss,
    .claude/skills/plugin-packaging/SKILL-windows.md
  </files>
  <read_first>
    - .claude/skills/plugin-packaging/SKILL.md (full macOS PKG workflow — invocation contract)
    - .claude/skills/plugin-packaging/SKILL-windows.md (full Windows EXE workflow — invocation contract; this file is now part of files_modified per BLOCKER #3)
    - .claude/skills/plugin-packaging/references/pkg-creation.md (entire file — extend Section 4a payload step + Section 4b postinstall script)
    - .claude/skills/plugin-packaging/assets/inno-template.iss (entire 76-line template — switch placeholder substitution to Inno preprocessor approach + extend [Files] block + add [Code] block)
    - plugins/O-Tremolo/dist/installer.iss (existing reference of a regenerated installer.iss)
    - modules/tuning/note-expression/module.cmake (Plan 25-01's install rules — replicate the same dual-write logic + Dorico version probe in PKG postinstall and Inno [Code] sections)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-04..D-08, D-11 — module-owned asset, dual-write paths, Dorico version targeting, README fallback)
  </read_first>
  <action>
    Three template-level extensions: macOS PKG reference, Windows Inno template, Windows skill docs. All three consume the module-staged canonical at `modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` (and the co-located `README-doricoexpmap.txt`).

    ── PART A: macOS PKG (`pkg-creation.md`) ──

    EXTEND `.claude/skills/plugin-packaging/references/pkg-creation.md` Section 4a. Insert AFTER the existing 2-line VST3+AU copy block at lines 173-174:

    ```bash
    # Copy canonical Dorico expression map into PKG payload (Phase 25 INST-03, INST-04).
    # The module owns the asset — installers consume it from modules/tuning/note-expression/resources/.
    # This step is unconditional for the v1.5 microtonal cohort (8 plugins). For non-cohort
    # plugins that do NOT consume the note-expression module, this block is a no-op (the
    # source files don't exist in non-consuming plugins; the cp will fail and skip).
    NE_RESOURCES_DIR="modules/tuning/note-expression/resources"
    if [ -f "$NE_RESOURCES_DIR/Ouaricon-VST3-NoteExpression.doricoexpmap" ]; then
        # Stage in a sub-directory of the PKG payload for the postinstall script to relocate.
        mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}/note-expression-resources"
        cp "$NE_RESOURCES_DIR/Ouaricon-VST3-NoteExpression.doricoexpmap" \
           "$TEMP_DIR/payload/${PLUGIN_NAME}/note-expression-resources/"
        cp "$NE_RESOURCES_DIR/README-doricoexpmap.txt" \
           "$TEMP_DIR/payload/${PLUGIN_NAME}/note-expression-resources/"
    fi
    ```

    Then EXTEND Section 4b (the postinstall script). Insert the following block INSIDE the heredoc, AFTER the `chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Audio/Plug-Ins/Components/PRODUCT_NAME_PLACEHOLDER.component"` line (around line 202) and BEFORE the `# Clean up temp files` comment (around line 204):

    ```bash
    # ---- Phase 25: Dorico expression map dual-write (INST-03, INST-04) ----
    # Skipped if the source files don't exist (non-microtonal plugin).
    NE_STAGED="/tmp/PLUGIN_NAME_PLACEHOLDER/note-expression-resources"
    if [ -f "$NE_STAGED/Ouaricon-VST3-NoteExpression.doricoexpmap" ]; then
        # 1) Editable canonical copy at Ouaricon shared resources path (D-05)
        OUARICON_DIR="$USER_HOME/Library/Application Support/Ouaricon/Expression Maps"
        mkdir -p "$OUARICON_DIR"
        cp "$NE_STAGED/Ouaricon-VST3-NoteExpression.doricoexpmap" "$OUARICON_DIR/"
        cp "$NE_STAGED/README-doricoexpmap.txt" "$OUARICON_DIR/"
        chown -R "$ACTUAL_USER:staff" "$OUARICON_DIR"
        echo "[note-expression] Wrote canonical .doricoexpmap to $OUARICON_DIR"

        # 2) Dorico user expression-maps scan path — auto-discovery (D-06)
        # Probe Dorico 6, 5, 4 in descending order. First hit wins.
        DORICO_USER_DIR=""
        for v in 6 5 4; do
            CANDIDATE="$USER_HOME/Library/Application Support/Steinberg/Dorico $v"
            if [ -d "$CANDIDATE" ]; then
                DORICO_USER_DIR="$CANDIDATE/Expression Maps/User"
                DORICO_VERSION="$v"
                break
            fi
        done
        if [ -n "$DORICO_USER_DIR" ]; then
            mkdir -p "$DORICO_USER_DIR"
            cp "$NE_STAGED/Ouaricon-VST3-NoteExpression.doricoexpmap" "$DORICO_USER_DIR/"
            chown -R "$ACTUAL_USER:staff" "$DORICO_USER_DIR/Ouaricon-VST3-NoteExpression.doricoexpmap"
            echo "[note-expression] Wrote .doricoexpmap to Dorico $DORICO_VERSION scan path: $DORICO_USER_DIR"
        else
            echo "[note-expression] No Dorico install detected — manual import via Library → Expression Maps… per README-doricoexpmap.txt (INST-04 fallback)"
        fi
    fi
    # ---- end Phase 25 dual-write ----
    ```

    Add a brief explanatory comment at the top of `pkg-creation.md` Section 4 (the section overview, around line 168):

    > **Phase 25 (INST-03, INST-04) addendum:** PKG payload step copies the canonical Dorico expression map from `modules/tuning/note-expression/resources/` into the payload; postinstall script dual-writes to `~/Library/Application Support/Ouaricon/Expression Maps/` (editable canonical) AND `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` (auto-discovery scan path, latest detected Dorico version). Skipped for non-microtonal plugins (source files absent). Mirrors the `module.cmake` `install()` rules from Plan 25-01 for plugins distributed via PKG instead of `cmake --install`.

    ── PART B: Windows Inno Setup template (`inno-template.iss`) — uses #define RepoRoot per BLOCKER #4/#5 ──

    The Windows template approach changes per BLOCKER #4/#5: instead of macOS-side sed substitution of `{{REPO_ROOT}}` (which mixes forward and backslashes), the template declares `#define RepoRoot ""` as a default stub and uses `{#RepoRoot}` in [Files] Source: paths. The Windows iscc invocation (Task 4 — Windows host) supplies `/DRepoRoot=H:\dev\VST-development` to override the stub at compile time. macOS has no role in the path resolution — it just regenerates the .iss source-of-truth file (Task 2).

    EDIT `.claude/skills/plugin-packaging/assets/inno-template.iss`. Multiple changes:

    1. ADD at the top of the file, AFTER line 12 (the bullet about PrivilegesRequired) but BEFORE line 13 (#define MyAppName), insert this block:

       ```inno
       ; - {#RepoRoot} resolves at iscc compile time. Default is empty stub; Windows host
       ;   MUST supply via command-line: iscc /DRepoRoot=H:\dev\VST-development installer.iss
       ;   (or equivalent). See plugin-packaging-windows SKILL.md Section 3 + 5c.
       ; - The .doricoexpmap dual-write happens in two places: [Files] for the Ouaricon
       ;   shared path (declarative), [Code]::CurStepChanged for the Dorico scan path
       ;   (runtime version probe). Both writes are skipped if the source file is
       ;   absent (non-microtonal plugin).
       #define RepoRoot ""
       ```

    2. AFTER the existing `[Files]` line (around line 49), ADD:

       ```inno
       ; ---- Phase 25 (INST-03, INST-04): Canonical Dorico expression map dual-write ----
       ; Source path is module-owned (single source of truth per D-03/D-04). Both writes
       ; target user-scope paths under {userappdata} (no admin required for this part).
       ; Files only included if the source exists (non-microtonal plugins skip silently).
       ; {#RepoRoot} resolves at iscc compile time on the Windows host (see top of file).
       Source: "{#RepoRoot}\modules\tuning\note-expression\resources\Ouaricon-VST3-NoteExpression.doricoexpmap"; DestDir: "{userappdata}\Ouaricon\Expression Maps"; Flags: ignoreversion uninsneveruninstall skipifsourcedoesntexist
       Source: "{#RepoRoot}\modules\tuning\note-expression\resources\README-doricoexpmap.txt"; DestDir: "{userappdata}\Ouaricon\Expression Maps"; Flags: ignoreversion uninsneveruninstall skipifsourcedoesntexist
       ; The Dorico-scan-path write is handled in [Code]::CurStepChanged below because it
       ; needs runtime Dorico version detection (Inno Setup [Files] is purely declarative).
       ```

       Note on `uninsneveruninstall`: the .doricoexpmap is shared across plugins and edits to it are user-owned — uninstalling one plugin must NOT remove the shared resource. Subsequent installs of any other v1.5 plugin would re-create it (overwrite is the canonical refresh path).

       Note on `skipifsourcedoesntexist`: when iscc is invoked WITHOUT /DRepoRoot, the path resolves to `\modules\tuning\note-expression\resources\...` (RepoRoot is the empty stub), which won't exist on disk. This flag prevents iscc compile failure for non-microtonal plugins or for partial smoke-builds where RepoRoot isn't supplied.

    3. EXTEND the existing `[Code]` block at the bottom of the file. REPLACE the current `procedure CurStepChanged(CurStep: TSetupStep)` with this expanded version:

       ```inno
       [Code]
       // Phase 25 (INST-03): runtime Dorico version detection + .doricoexpmap copy
       // to Dorico's user expression-maps scan path. Inno [Files] handles the
       // unconditional Ouaricon-shared write; this block handles the conditional
       // Dorico-scan write that needs version probing.
       function GetDoricoUserMapsDir(): String;
       var
         v: Integer;
         candidate: String;
       begin
         Result := '';
         for v := 6 downto 4 do
         begin
           candidate := ExpandConstant('{userappdata}\Steinberg\Dorico ') + IntToStr(v);
           if DirExists(candidate) then
           begin
             Result := candidate + '\Expression Maps\User';
             Exit;
           end;
         end;
       end;

       procedure CurStepChanged(CurStep: TSetupStep);
       var
         AbletonDir, DoricoMapsDir, MapSource, MapDest: String;
       begin
         if CurStep = ssPostInstall then
         begin
           // Phase 25 dual-write: copy .doricoexpmap to Dorico's scan path if Dorico installed
           MapSource := ExpandConstant('{userappdata}\Ouaricon\Expression Maps\Ouaricon-VST3-NoteExpression.doricoexpmap');
           if FileExists(MapSource) then
           begin
             DoricoMapsDir := GetDoricoUserMapsDir();
             if DoricoMapsDir <> '' then
             begin
               ForceDirectories(DoricoMapsDir);
               MapDest := DoricoMapsDir + '\Ouaricon-VST3-NoteExpression.doricoexpmap';
               FileCopy(MapSource, MapDest, False);
               Log('[note-expression] Wrote .doricoexpmap to Dorico scan path: ' + DoricoMapsDir);
             end
             else
             begin
               Log('[note-expression] No Dorico install detected — manual import per README-doricoexpmap.txt (INST-04 fallback)');
             end;
           end;

           // Existing Ableton cache hint (kept verbatim from pre-Phase-25 template)
           AbletonDir := ExpandConstant('{userappdata}\Ableton');
           if DirExists(AbletonDir) then
           begin
             Log('Ableton preferences directory found - plugin rescan will occur on next launch');
           end;
         end;
       end;
       ```

    ── PART C: Windows skill documentation (`SKILL-windows.md`) — per BLOCKER #3 ──

    EDIT `.claude/skills/plugin-packaging/SKILL-windows.md` to document the `/DRepoRoot` iscc command-line convention. Find Section 3 (template substitution / placeholder list) — around line 86 — and add this new sub-section AFTER the existing placeholder list:

    ```markdown
    ### Phase 25 (INST-03): Inno Preprocessor RepoRoot Convention

    The `inno-template.iss` declares `#define RepoRoot ""` at the top as a default
    empty stub. The .doricoexpmap [Files] entries reference `{#RepoRoot}\modules\...`,
    which resolves at iscc compile time. The Windows host MUST supply the actual
    repo root via the iscc command-line `/D` option:

    ```powershell
    iscc /DRepoRoot=H:\dev\VST-development plugins\O-Lyrica\dist\installer.iss
    ```

    or in a script:

    ```powershell
    $repoRoot = (Get-Location).Path
    iscc "/DRepoRoot=$repoRoot" "plugins\$PluginName\dist\installer.iss"
    ```

    Why this matters (Phase 25 BLOCKER #4/#5 history): the original Phase 25 plan
    used macOS-side sed substitution of `{{REPO_ROOT}}` to bake the path into the
    .iss file. This produced mixed forward/backslash paths
    (`/Users/.../VST-development\modules\...`) that iscc rejected. The preprocessor
    approach decouples .iss generation (cross-platform, mechanical) from path
    resolution (Windows-host-only, native conventions). It also makes the .iss
    file path-relocatable — the same source-of-truth file works on any Windows
    host that supplies the right `/DRepoRoot`.

    The `skipifsourcedoesntexist` flag on the .doricoexpmap [Files] entries
    prevents iscc compile failure when /DRepoRoot is omitted (e.g., for partial
    smoke-builds that don't need the .doricoexpmap surface).
    ```

    Confirm `SKILL-windows.md` is in this plan's `files_modified` (it now is, per BLOCKER #3 from checker review).
  </action>
  <verify>
    <automated>
      grep -q 'Phase 25 (INST-03, INST-04)' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'NE_RESOURCES_DIR=' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'note-expression-resources' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'for v in 6 5 4' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'Steinberg/Dorico' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'Wrote .doricoexpmap to Dorico' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'INST-04 fallback' .claude/skills/plugin-packaging/references/pkg-creation.md &&
      grep -q 'Phase 25 (INST-03, INST-04)' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q '#define RepoRoot' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q '{#RepoRoot}' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q 'function GetDoricoUserMapsDir' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q 'for v := 6 downto 4' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q 'uninsneveruninstall' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q 'skipifsourcedoesntexist' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      ! grep -q '{{REPO_ROOT}}' .claude/skills/plugin-packaging/assets/inno-template.iss &&
      grep -q 'DRepoRoot' .claude/skills/plugin-packaging/SKILL-windows.md &&
      grep -q 'Phase 25' .claude/skills/plugin-packaging/SKILL-windows.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `pkg-creation.md` contains the new Phase 25 INST-03/INST-04 marker (grep returns ≥1 match) AND the source-path reference `modules/tuning/note-expression/resources/` (≥1 match).
    - `pkg-creation.md` payload-staging block includes `mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}/note-expression-resources"` (grep `note-expression-resources` returns ≥1 match in pkg-creation.md).
    - `pkg-creation.md` postinstall block contains the Dorico version probe loop `for v in 6 5 4` (grep returns 1 match).
    - `pkg-creation.md` postinstall block writes to BOTH paths: `Ouaricon/Expression Maps` AND `Steinberg/Dorico` (each grep returns ≥1 match).
    - `pkg-creation.md` postinstall block emits the INST-04 fallback log line (grep `INST-04 fallback` returns 1 match).
    - `inno-template.iss` contains the Phase 25 marker (grep returns 1 match) AND the source path `Ouaricon-VST3-NoteExpression.doricoexpmap` (grep returns ≥2 matches: one [Files] entry + at least one [Code] reference).
    - `inno-template.iss` declares `#define RepoRoot` (grep returns 1 match) AND uses `{#RepoRoot}` in Source: paths (grep returns ≥2 matches — one for .doricoexpmap, one for README) per BLOCKER #4/#5 resolution.
    - `inno-template.iss` does NOT contain the obsolete `{{REPO_ROOT}}` placeholder (negative grep — confirms migration to Inno preprocessor approach).
    - `inno-template.iss` Pascal `[Code]` block contains the new function `function GetDoricoUserMapsDir(): String;` (grep returns 1 match) AND the descending probe `for v := 6 downto 4` (grep returns 1 match).
    - `inno-template.iss` `[Files]` entries for the .doricoexpmap use `uninsneveruninstall` flag (grep returns ≥1 match) AND `skipifsourcedoesntexist` flag (grep returns ≥1 match) — preserves user edits across plugin uninstalls and prevents iscc compile failure when /DRepoRoot is omitted.
    - `SKILL-windows.md` documents the `/DRepoRoot` convention (grep returns ≥1 match) AND references Phase 25 (grep returns ≥1 match).
    - `SKILL-windows.md` is correctly listed in this plan's `files_modified:` (per BLOCKER #3 — atomic-commit discipline D-11).
  </acceptance_criteria>
  <done>
    All three template/skill files are extended with the dual-write .doricoexpmap logic. macOS PKG via `pkg-creation.md` (payload step + postinstall script). Windows EXE via `inno-template.iss` ([Files] for declarative shared write using `{#RepoRoot}` preprocessor + [Code] for runtime Dorico-scan write) AND `SKILL-windows.md` (documents the iscc /DRepoRoot convention). All template-level edits are mechanical (grep-verifiable). Per-plugin regeneration happens in Task 2; per-plugin macOS PKG materialization happens in Task 3.
  </done>
</task>

<task type="auto">
  <name>Task 2: Regenerate all 8 plugins' installer.iss from updated template (with generalized version regex + per-plugin sanity check per BLOCKER #6)</name>
  <files>
    plugins/O-Lyrica/dist/installer.iss,
    plugins/O-Bells/dist/installer.iss,
    plugins/O-IntonationPad/dist/installer.iss,
    plugins/O-Prism/dist/installer.iss,
    plugins/O-Wind/dist/installer.iss,
    plugins/O-Reed/dist/installer.iss,
    plugins/O-Bowed/dist/installer.iss,
    plugins/O-Formant/dist/installer.iss
  </files>
  <read_first>
    - .claude/skills/plugin-packaging/assets/inno-template.iss (the updated template from Task 1 — source of truth for regeneration; uses `#define RepoRoot ""` stub + `{#RepoRoot}` in [Files])
    - .claude/skills/plugin-packaging/SKILL-windows.md (template substitution algorithm + placeholder list + new /DRepoRoot section)
    - plugins/O-Tremolo/dist/installer.iss (existing reference of a regenerated installer.iss — confirms substitution pattern works on a known-good output)
    - For each of the 8 plugins, read the FIRST 30 lines of `plugins/<Plugin>/CMakeLists.txt` to extract: `juce_add_plugin(<TARGET_NAME> ...)` line, `PRODUCT_NAME "..."` line, version line (heterogeneous: see version table in <interfaces> — VERSION vs PLUGIN_VERSION; quoted vs unquoted)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-11 — atomic sweep across 8 plugins)
  </read_first>
  <action>
    For each of the 8 plugins (in this order — easy-first per Phase 24 D-11 precedent: O-Lyrica first as the validated reference, then the other 7):

    1. **O-Lyrica** (CMake target `OLyrica`, `VERSION "2.3.0"` quoted)
    2. **O-Bells** (CMake target `O-Bells`, `PLUGIN_VERSION "4.1.0"` quoted)
    3. **O-IntonationPad** (CMake target `O-IntonationPad`, `PLUGIN_VERSION "2.8.0"` quoted)
    4. **O-Prism** (CMake target `O-Prism`, `VERSION 1.17.0` UNQUOTED — BLOCKER #6 case)
    5. **O-Wind** (CMake target `O-Wind`, `PLUGIN_VERSION "1.16.0"` quoted)
    6. **O-Reed** (CMake target `O-Reed`, `PLUGIN_VERSION "1.1.0"` quoted)
    7. **O-Bowed** (CMake target `O-Bowed`, `PLUGIN_VERSION "1.3.0"` quoted)
    8. **O-Formant** (CMake target `O-Formant`, `VERSION 1.25.0` UNQUOTED — BLOCKER #6 case)

    Per plugin:

    Step 1 — Define expected versions for sanity check (per BLOCKER #6):
    ```bash
    declare -A EXPECTED_VERSIONS=(
        [O-Lyrica]=2.3.0
        [O-Bells]=4.1.0
        [O-IntonationPad]=2.8.0
        [O-Prism]=1.17.0
        [O-Wind]=1.16.0
        [O-Reed]=1.1.0
        [O-Bowed]=1.3.0
        [O-Formant]=1.25.0
    )
    ```

    Step 2 — Extract metadata using GENERALIZED regex per BLOCKER #6 (handles VERSION/PLUGIN_VERSION + quoted/unquoted):
    ```bash
    PLUGIN_NAME="<Plugin>"  # e.g., O-Lyrica

    CMAKE_TARGET=$(grep -E '^juce_add_plugin\(' plugins/$PLUGIN_NAME/CMakeLists.txt | head -1 | sed -E 's/juce_add_plugin\(([A-Za-z0-9_-]+).*/\1/')

    PRODUCT_NAME=$(grep 'PRODUCT_NAME' plugins/$PLUGIN_NAME/CMakeLists.txt | head -1 | sed 's/.*PRODUCT_NAME "\([^"]*\)".*/\1/' | sed 's/\${OUARICON_DEV_SUFFIX}//')

    # GENERALIZED version regex (BLOCKER #6 fix):
    # - matches both `VERSION` and `PLUGIN_VERSION` identifier (excludes line 1's `cmake_minimum_required(VERSION 3.15)` via leading-whitespace anchor)
    # - matches both quoted "X.Y.Z" and unquoted X.Y.Z forms
    # - extracts the version triple
    VERSION=$(grep -E '^[[:space:]]+(PLUGIN_)?VERSION[[:space:]]+"?[0-9]+\.[0-9]+\.[0-9]+"?' plugins/$PLUGIN_NAME/CMakeLists.txt | head -1 | sed -E 's/.*VERSION[[:space:]]+"?([0-9]+\.[0-9]+\.[0-9]+)"?.*/\1/')

    # Sanity check (BLOCKER #6): assert non-empty AND matches expected
    EXPECTED="${EXPECTED_VERSIONS[$PLUGIN_NAME]}"
    if [ -z "$VERSION" ]; then
        echo "FATAL: VERSION extraction failed for $PLUGIN_NAME (regex did not match — check CMakeLists.txt format)"
        exit 1
    fi
    if [ "$VERSION" != "$EXPECTED" ]; then
        echo "FATAL: $PLUGIN_NAME extracted VERSION='$VERSION' but expected '$EXPECTED' (registry mismatch — halt sweep)"
        exit 1
    fi
    echo "OK: $PLUGIN_NAME version $VERSION (matches expected)"
    ```

    Step 3 — Generate deterministic APP_GUID (per SKILL-windows.md Section 2; bash equivalent of the PowerShell pattern):
    ```bash
    # Use printf + openssl to produce a SHA256-derived UUID-shaped string, deterministic per plugin.
    APP_GUID=$(printf "OuariconAudio-%s" "$PLUGIN_NAME" | openssl dgst -sha256 -binary | xxd -p -c 16 | head -1 | sed 's/\(........\)\(....\)\(....\)\(....\)\(.\{12\}\)/\1-\2-\3-\4-\5/')
    # Fallback if openssl/xxd unavailable:
    if [ -z "$APP_GUID" ]; then
        APP_GUID=$(python3 -c "import hashlib,uuid; h=hashlib.sha256(b'OuariconAudio-${PLUGIN_NAME}').digest(); print(str(uuid.UUID(bytes=h[:16])))")
    fi
    ```

    Step 4 — Compute paths. NOTE per BLOCKER #4/#5: the .iss source-of-truth file does NOT bake in the absolute REPO_ROOT — that's resolved at iscc compile time on the Windows host via /DRepoRoot. The VST3_SOURCE_PATH and OUTPUT_DIR ARE baked in (these are Windows-host paths the macOS regeneration cannot guess; the Windows host's `build-installer` skill will sed-substitute these per its own conventions, OR the Windows host re-runs this regeneration with its own paths). For the Phase 25 commit, generate Windows-style placeholder paths that signal "regenerate on Windows host" — using `<WIN_REPO_ROOT>` as a literal placeholder that iscc will tolerate via skipifsourcedoesntexist if not pre-substituted, OR more cleanly: leave these as `{#RepoRoot}` references too:

    ```bash
    # Substituted at iscc time on Windows host via /DRepoRoot:
    VST3_SOURCE_PATH='{#RepoRoot}\\build\\plugins\\'$PLUGIN_NAME'\\'$CMAKE_TARGET'_artefacts\\Release\\VST3\\'$PRODUCT_NAME'.vst3'
    OUTPUT_DIR='{#RepoRoot}\\plugins\\'$PLUGIN_NAME'\\dist'
    ```

    This makes the generated installer.iss fully path-relocatable: a single `iscc /DRepoRoot=H:\dev\VST-development installer.iss` invocation on the Windows host resolves all paths.

    Step 5 — Read template, substitute placeholders, write to `plugins/<Plugin>/dist/installer.iss`. NOTE: `{{REPO_ROOT}}` is NO LONGER a sed substitution target (per BLOCKER #4/#5 — it became `#define RepoRoot ""` resolved at iscc time):

    ```bash
    mkdir -p "plugins/${PLUGIN_NAME}/dist"
    sed \
      -e "s|{{PLUGIN_NAME}}|${PLUGIN_NAME}|g" \
      -e "s|{{PRODUCT_NAME}}|${PRODUCT_NAME}|g" \
      -e "s|{{VERSION}}|${VERSION}|g" \
      -e "s|{{APP_GUID}}|${APP_GUID}|g" \
      -e "s|{{VST3_SOURCE_PATH}}|${VST3_SOURCE_PATH}|g" \
      -e "s|{{OUTPUT_DIR}}|${OUTPUT_DIR}|g" \
      .claude/skills/plugin-packaging/assets/inno-template.iss \
      > "plugins/${PLUGIN_NAME}/dist/installer.iss"
    ```

    Step 6 — Per-plugin verify substitution worked: each generated `installer.iss` MUST have:
    - Zero remaining `{{...}}` placeholders (`grep -c '{{' plugins/<Plugin>/dist/installer.iss` returns 0)
    - `#define RepoRoot ""` declared (the empty stub — iscc on Windows replaces via /DRepoRoot)
    - `{#RepoRoot}` references in [Files] Source: paths (resolved at iscc time)
    - The substituted plugin-specific `MyPluginName` and `MyAppName` references
    - `MyAppVersion "<expected>"` (non-empty, matches the EXPECTED_VERSIONS table per BLOCKER #6)
    - The two new [Files] entries pointing at `{#RepoRoot}\modules\tuning\note-expression\resources\Ouaricon-VST3-NoteExpression.doricoexpmap` and `README-doricoexpmap.txt`
    - The Pascal [Code] block with `GetDoricoUserMapsDir` function

    Note that the actual EXE installer compilation (`iscc plugins/<Plugin>/dist/installer.iss`) is NOT executed in this task on macOS (no Inno Setup compiler on macOS). The `installer.iss` is the source-of-truth file committed to the repo for reproducibility per `SKILL-windows.md` Section 5c. Compilation + dry-run install + Dorico smoke is gated to Task 5 (Windows host).
  </action>
  <verify>
    <automated>
      declare -A EXPECTED_VERSIONS=(
        [O-Lyrica]=2.3.0
        [O-Bells]=4.1.0
        [O-IntonationPad]=2.8.0
        [O-Prism]=1.17.0
        [O-Wind]=1.16.0
        [O-Reed]=1.1.0
        [O-Bowed]=1.3.0
        [O-Formant]=1.25.0
      );
      for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
        test -f plugins/$p/dist/installer.iss || { echo "MISSING: $p"; exit 1; };
        if grep -q '{{' plugins/$p/dist/installer.iss; then echo "UNSUBSTITUTED PLACEHOLDER in $p"; exit 1; fi;
        grep -q '#define RepoRoot' plugins/$p/dist/installer.iss || { echo "NO #define RepoRoot in $p"; exit 1; };
        grep -q '{#RepoRoot}' plugins/$p/dist/installer.iss || { echo "NO {#RepoRoot} reference in $p"; exit 1; };
        grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' plugins/$p/dist/installer.iss || { echo "NO .doricoexpmap entry in $p"; exit 1; };
        grep -q 'function GetDoricoUserMapsDir' plugins/$p/dist/installer.iss || { echo "NO GetDoricoUserMapsDir in $p"; exit 1; };
        grep -q 'uninsneveruninstall' plugins/$p/dist/installer.iss || { echo "NO uninsneveruninstall in $p"; exit 1; };
        grep -q "MyPluginName \"$p\"" plugins/$p/dist/installer.iss || { echo "NO MyPluginName=$p in $p installer"; exit 1; };
        EXP="${EXPECTED_VERSIONS[$p]}";
        grep -q "MyAppVersion \"$EXP\"" plugins/$p/dist/installer.iss || { echo "WRONG VERSION in $p (expected $EXP)"; exit 1; };
        echo "OK: $p (version $EXP)";
      done
    </automated>
  </verify>
  <acceptance_criteria>
    - All 8 `plugins/<Plugin>/dist/installer.iss` files exist (per the for-loop above).
    - Zero unsubstituted `{{...}}` placeholders in any of the 8 files.
    - Each .iss declares `#define RepoRoot` (the empty stub) AND uses `{#RepoRoot}` in [Files] Source: paths (per BLOCKER #4/#5 resolution).
    - Each .iss references the canonical .doricoexpmap source path via `{#RepoRoot}\modules\tuning\note-expression\resources\Ouaricon-VST3-NoteExpression.doricoexpmap`.
    - Each .iss contains the new Pascal `[Code]` `GetDoricoUserMapsDir` function.
    - Each .iss has `uninsneveruninstall` on the .doricoexpmap [Files] entries (preserves shared resource across per-plugin uninstalls).
    - Each .iss has plugin-specific `MyPluginName "<Plugin>"` literal.
    - Each .iss has `MyAppVersion "<X.Y.Z>"` matching the EXPECTED_VERSIONS table — NON-EMPTY (per BLOCKER #6 sanity check). Specifically: O-Lyrica 2.3.0, O-Bells 4.1.0, O-IntonationPad 2.8.0, O-Prism 1.17.0, O-Wind 1.16.0, O-Reed 1.1.0, O-Bowed 1.3.0, O-Formant 1.25.0.
    - Generalized version-extraction regex was used (BLOCKER #6) — handles both `VERSION` and `PLUGIN_VERSION` identifiers AND both quoted and unquoted forms.
  </acceptance_criteria>
  <done>All 8 plugins have a regenerated, plugin-specific `installer.iss` committed under their `dist/` directories. Each script bundles the canonical .doricoexpmap (via `{#RepoRoot}` preprocessor reference) and emits the Pascal Dorico-version-probe code. Per-plugin VERSION sanity check passed for all 8 plugins. The Windows source-of-truth files are ready for compilation by the `/build-installer <Plugin>` skill on a Windows host (Task 5) using `iscc /DRepoRoot=<path> installer.iss`.</done>
</task>

<task type="auto">
  <name>Task 3: Materialize all 8 macOS PKG installers via /package + verify each PKG payload contains the .doricoexpmap (per BLOCKER #1)</name>
  <files>
    plugins/O-Lyrica/dist/,
    plugins/O-Bells/dist/,
    plugins/O-IntonationPad/dist/,
    plugins/O-Prism/dist/,
    plugins/O-Wind/dist/,
    plugins/O-Reed/dist/,
    plugins/O-Bowed/dist/,
    plugins/O-Formant/dist/
  </files>
  <read_first>
    - .claude/skills/plugin-packaging/SKILL.md (full macOS PKG workflow — invocation contract for /package)
    - .claude/skills/plugin-packaging/references/pkg-creation.md (just-extended in Task 1 — payload step + postinstall script with .doricoexpmap dual-write)
    - modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap (the asset that must be present in each PKG payload)
    - CLAUDE.md (Plugin Cache Clearing protocol — mandatory before each plugin's build)
  </read_first>
  <action>
    Per BLOCKER #1 from checker review: the original Plan 25-02 only materialized 1 of 8 macOS PKGs (O-Lyrica) and validated it. The other 7 plugins' PKG installers were never built, leaving INST-03's "all 8 installers bundle" claim unproven.

    This task closes that gap by materializing all 8 PKG installers and verifying each one's payload contains the .doricoexpmap.

    For each of the 8 plugins (in the order: O-Lyrica → O-Reed → O-Formant → O-Bells → O-IntonationPad → O-Prism → O-Wind → O-Bowed — putting the spot-check trio first):

    Step 1 — Build the plugin fresh (per CLAUDE.md):
    ```bash
    PLUGIN_NAME="<Plugin>"
    CMAKE_TARGET=$(grep -E '^juce_add_plugin\(' plugins/$PLUGIN_NAME/CMakeLists.txt | head -1 | sed -E 's/juce_add_plugin\(([A-Za-z0-9_-]+).*/\1/')

    cd build && ninja ${CMAKE_TARGET}_VST3 ${CMAKE_TARGET}_AU 2>&1 | tee /tmp/25-02-${PLUGIN_NAME}-build.log
    cd ..
    ```

    Step 2 — Pre-install per CLAUDE.md (required for the packaging skill to find the .vst3 and .component bundles):
    ```bash
    PRODUCT_NAME="${PLUGIN_NAME}"  # PRODUCT_NAME without OUARICON_DEV_SUFFIX
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/
    rm -rf ~/Library/Caches/com.apple.audiounits.cache
    rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
    rm -rf "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
    cp -R "build/plugins/${PLUGIN_NAME}/${CMAKE_TARGET}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
    cp -R "build/plugins/${PLUGIN_NAME}/${CMAKE_TARGET}_artefacts/Release/AU/${PRODUCT_NAME}.component" "$HOME/Library/Audio/Plug-Ins/Components/"
    ```

    Step 3 — Run the macOS PKG packaging skill end-to-end (`/package <Plugin>` — invokes the `plugin-packaging` skill which now consumes the updated `pkg-creation.md` from Task 1):
    ```bash
    # Invoke /package skill or run the equivalent script per .claude/skills/plugin-packaging/SKILL.md
    # Expected output: plugins/<Plugin>/dist/<Plugin>-OuariconAudio.pkg
    /package $PLUGIN_NAME 2>&1 | tee /tmp/25-02-${PLUGIN_NAME}-package.log
    ```

    (If `/package` is unavailable as a slash command in the execute-plan environment, manually walk through `pkg-creation.md` Section 4 sub-steps using bash.)

    Step 4 — Verify the PKG was produced AND its payload contains the .doricoexpmap:
    ```bash
    PKG_PATH="plugins/${PLUGIN_NAME}/dist/${PLUGIN_NAME}-OuariconAudio.pkg"
    test -f "$PKG_PATH" || { echo "PKG NOT MATERIALIZED: $PLUGIN_NAME"; exit 1; }

    # Inspect PKG payload manifest. macOS PKGs are archives; extract the payload listing:
    pkgutil --payload-files "$PKG_PATH" > "/tmp/25-02-${PLUGIN_NAME}-payload.txt" 2>&1
    grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' "/tmp/25-02-${PLUGIN_NAME}-payload.txt" || {
        echo "PAYLOAD MISSING .doricoexpmap: $PLUGIN_NAME"; exit 1;
    }
    grep -q 'README-doricoexpmap.txt' "/tmp/25-02-${PLUGIN_NAME}-payload.txt" || {
        echo "PAYLOAD MISSING README-doricoexpmap.txt: $PLUGIN_NAME"; exit 1;
    }
    echo "OK: $PLUGIN_NAME PKG materialized + .doricoexpmap + README in payload"
    ```

    Per BLOCKER #1: the spot-check trio (O-Lyrica, O-Reed, O-Formant) covers the three version conventions (VERSION quoted, PLUGIN_VERSION quoted, VERSION unquoted) AND includes one MPE-helper-based plugin (O-Reed). These three are processed first so any structural breakage surfaces early before sweeping the remaining 5.

    Manual install validation (per BLOCKER #1 spot-check requirement) happens in Task 4 (the existing macOS Dorico checkpoint) — extended to install + verify dual-write for O-Lyrica + O-Reed + O-Formant (3 plugins instead of just O-Lyrica).
  </action>
  <verify>
    <automated>
      for p in O-Lyrica O-Reed O-Formant O-Bells O-IntonationPad O-Prism O-Wind O-Bowed; do
        PKG="plugins/$p/dist/$p-OuariconAudio.pkg";
        test -f "$PKG" || { echo "MISSING PKG: $p"; exit 1; };
        pkgutil --payload-files "$PKG" 2>/dev/null | grep -q 'Ouaricon-VST3-NoteExpression.doricoexpmap' || {
          echo "MISSING .doricoexpmap in payload: $p"; exit 1;
        };
        pkgutil --payload-files "$PKG" 2>/dev/null | grep -q 'README-doricoexpmap.txt' || {
          echo "MISSING README in payload: $p"; exit 1;
        };
        echo "OK: $p PKG materialized + payload validated";
      done
    </automated>
  </verify>
  <acceptance_criteria>
    - All 8 macOS PKG files exist at `plugins/<Plugin>/dist/<Plugin>-OuariconAudio.pkg`.
    - Each PKG's payload manifest (via `pkgutil --payload-files`) contains the literal string `Ouaricon-VST3-NoteExpression.doricoexpmap`.
    - Each PKG's payload manifest contains `README-doricoexpmap.txt`.
    - The spot-check trio (O-Lyrica, O-Reed, O-Formant) was processed first and validated — covers VERSION quoted, PLUGIN_VERSION quoted, VERSION unquoted version conventions plus an MPE-helper-based plugin per BLOCKER #1.
    - The /package skill invocations were captured in per-plugin logs at `/tmp/25-02-<Plugin>-package.log` for diagnostic traceability if any plugin's PKG build fails.
  </acceptance_criteria>
  <done>
    All 8 macOS PKG installers are physically materialized at their `plugins/<Plugin>/dist/` paths. Each PKG's payload manifest is verified to contain both `.doricoexpmap` and `README-doricoexpmap.txt`. INST-03's "all 8 installers bundle" claim is now physically proven across the cohort, not just for O-Lyrica (BLOCKER #1 closed). Manual install validation for the spot-check trio happens in Task 4.
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: macOS Dorico smoke gate via PKG-installed O-Lyrica + spot-check O-Reed + O-Formant (per BLOCKER #1 — at least 2-3 plugins beyond O-Lyrica)</name>
  <what-built>
    macOS validation gate (per D-09, D-10, BLOCKER #1): materialize 8 PKGs in Task 3, then dry-run install 3 representative PKGs (O-Lyrica + O-Reed + O-Formant — covers all 3 version conventions and one MPE plugin) into a fresh user state. Verify each PKG's dual-write of the .doricoexpmap lands at both macOS paths. Then run the Dorico 3-point quarter-sharp smoke gate via the freshly PKG-installed O-Lyrica.
  </what-built>
  <how-to-verify>
    The agent will execute the macOS PKG flow for the spot-check trio:

    For each of O-Lyrica, O-Reed, O-Formant (in that order):

    1. **CRITICAL**: BEFORE installing the PKG, delete any prior installed .doricoexpmap copies (so we prove the PKG itself wrote them, not a leftover from Plan 25-01's `cmake --install`):
       ```bash
       rm -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap
       rm -f ~/Library/Application\ Support/Ouaricon/Expression\ Maps/README-doricoexpmap.txt
       rm -f ~/Library/Application\ Support/Steinberg/Dorico\ */Expression\ Maps/User/Ouaricon-VST3-NoteExpression.doricoexpmap 2>/dev/null || true
       ```

    2. Install the PKG using the system Installer.app or `sudo installer -pkg plugins/<Plugin>/dist/<Plugin>-OuariconAudio.pkg -target /`.

    3. Verify both writes happened:
       ```bash
       ls -la ~/Library/Application\ Support/Ouaricon/Expression\ Maps/
       ls -la ~/Library/Application\ Support/Steinberg/Dorico\ */Expression\ Maps/User/Ouaricon-*
       diff -q ~/Library/Application\ Support/Ouaricon/Expression\ Maps/Ouaricon-VST3-NoteExpression.doricoexpmap modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap
       ```
       All 3 of O-Lyrica + O-Reed + O-Formant must produce byte-identical dual-writes.

    4. (Only after O-Lyrica install — this is the formal Dorico gate plugin) Open Dorico, confirm "Ouaricon VST3 Note Expression" appears in the picker (Library → Expression Maps…), assign it to O-Lyrica's channel.

    5. Run the 3-point Dorico smoke gate (D-07) via O-Lyrica:
       - Gate 1: quarter-sharp C4 plays at +50¢ (~269.29 Hz)
       - Gate 2: no attack zipper on the first sample of the note
       - Gate 3: chord [q♯ C4 + ♮ E4] — only C4 detuned

    O-Reed and O-Formant are spot-check installs only (verify dual-write physically lands and PKG installs without error) — they do NOT need the full Dorico 3-point gate (that's already covered by Plan 25-01's O-Lyrica gate and this task's O-Lyrica gate via PKG).

    User must respond with the actual observed result.
  </how-to-verify>
  <resume-signal>
    Respond with one of:
      • `macos-pass` — All 3 PKGs (O-Lyrica + O-Reed + O-Formant) installed cleanly, all 3 dual-writes verified, Dorico picker auto-discovered the map, all 3 Dorico gates PASS via O-Lyrica
      • `macos-pass-manual-import` — All 3 PKGs installed + dual-write succeeded for Ouaricon path, but Dorico-scan-path write didn't take effect for at least one (had to manually import); 3 Dorico gates PASS after manual import
      • `macos-fail-pkg-build: [plugin name and error]` — A specific PKG itself failed to build (problem with Task 1's pkg-creation.md edits or Task 3's /package invocation)
      • `macos-fail-dual-write: [which plugin and which path didn't get written]` — PKG installed but file didn't land at expected location
      • `macos-fail-gate-N: [observed]` — Dorico gate N failed (specify which and what was observed)
  </resume-signal>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 5: Windows Dorico smoke gate via Inno-compiled installer (HARD HALT if Windows access blocked per D-09)</name>
  <what-built>
    Windows validation gate (per D-09, D-10 — REVERSES Phase 23/24 macOS-only convention for this milestone): build a representative plugin on a Windows host, compile the regenerated `installer.iss` with Inno Setup using the new `/DRepoRoot` convention (per BLOCKER #4/#5 resolution), run the EXE installer, verify dual-write of the .doricoexpmap landed at both Windows paths, then run the Dorico 3-point quarter-sharp smoke gate on the Windows-installed plugin.

    **Hard halt rule (CONTEXT D-09): if Windows host access is blocked at execute time, do NOT silently fall back to macOS-only validation. Surface the halt to the user and present the options below.**
  </what-built>
  <how-to-verify>
    On a Windows host with Inno Setup installed, the agent will execute:

    1. Sync the repo to the Windows host (git pull).
    2. Build a representative plugin (recommend O-Lyrica for parity with the macOS gate; if O-Lyrica's Windows build is broken, fall back to O-Bells which has been validated as the Phase 24 canary): `powershell -File scripts/build-and-install.ps1 O-Lyrica` (per CLAUDE.md Windows section).
    3. Compile the regenerated installer.iss using the new `/DRepoRoot` convention (BLOCKER #4/#5 resolution):
       ```powershell
       $repoRoot = (Get-Location).Path
       iscc "/DRepoRoot=$repoRoot" "plugins\O-Lyrica\dist\installer.iss"
       ```
       This produces `plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe`. The `{#RepoRoot}` references in the .iss resolve to `H:\dev\VST-development` (or wherever the repo is on the Windows host) at iscc compile time, producing native Windows backslash paths. No mixed-slash issue per BLOCKER #4.
    4. Pre-clean any prior .doricoexpmap copies:
       ```powershell
       Remove-Item "$env:APPDATA\Ouaricon\Expression Maps\Ouaricon-VST3-NoteExpression.doricoexpmap" -ErrorAction SilentlyContinue
       Remove-Item "$env:APPDATA\Steinberg\Dorico*\Expression Maps\User\Ouaricon-VST3-NoteExpression.doricoexpmap" -ErrorAction SilentlyContinue
       ```
    5. Run the EXE installer (interactive or silent: `O-Lyrica-OuariconAudio-Setup.exe /VERYSILENT /SUPPRESSMSGBOXES`). Requires admin (writes to Program Files for the VST3).
    6. Verify dual-write:
       ```powershell
       Test-Path "$env:APPDATA\Ouaricon\Expression Maps\Ouaricon-VST3-NoteExpression.doricoexpmap"
       Get-ChildItem "$env:APPDATA\Steinberg\Dorico*\Expression Maps\User\Ouaricon-*"
       ```
    7. Open Dorico on Windows. Confirm "Ouaricon VST3 Note Expression" in the picker. Assign to O-Lyrica's channel.
    8. Run the 3-point Dorico smoke gate (same as macOS gate above):
       - Gate 1: quarter-sharp C4 = +50¢ (~269.29 Hz)
       - Gate 2: no attack zipper
       - Gate 3: polyphonic chord — only C4 detuned

    User must respond with the actual observed Windows result.
  </how-to-verify>
  <resume-signal>
    Respond with one of:
      • `windows-pass` — EXE installed cleanly, dual-write verified at both Windows paths, Dorico picker auto-discovered the map, all 3 Dorico gates PASS
      • `windows-pass-manual-import` — EXE installed cleanly + dual-write succeeded for %APPDATA%\Ouaricon path, but Dorico-scan-path write didn't take effect (manual import required); 3 Dorico gates PASS after manual import
      • `windows-fail-iscc: [error]` — Inno Setup compilation failed (problem with Task 1's inno-template.iss edits — likely Pascal [Code] syntax or /DRepoRoot path resolution)
      • `windows-fail-install: [error]` — EXE compiled but installer failed at runtime
      • `windows-fail-dual-write: [which path didn't get written]`
      • `windows-fail-gate-N: [observed]`
      • `windows-blocked: no-host-access` — Windows host is unavailable at this time (HARD HALT per D-09; do NOT proceed to commit). User must specify how to handle: (a) defer Plan 25-02 commit until Windows access available, (b) defer Windows validation to a follow-up plan `25-04-windows-validation-PLAN.md` and commit Plan 25-02 with macOS-only validation marked PARTIAL in SUMMARY, (c) other.
      • `windows-blocked: iscc-not-installed` — Need to install Inno Setup first (winget install JRSoftware.InnoSetup); user decides whether to install now or defer.
  </resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| installer (PKG/EXE) → user filesystem | Signed PKG runs postinstall as root with user-context detection (`stat -f '%Su' /dev/console`); writes to user-HOME paths with explicit chown back to the actual user. Inno Setup EXE writes to `%APPDATA%` under user-scope (no admin needed for the .doricoexpmap part; admin only for the Program Files VST3 install). |
| repo build artifact → installer payload | The .doricoexpmap and README are read from the repo at build time (macOS PKG via `pkg-creation.md` payload step) or at iscc compile time on Windows host (via `{#RepoRoot}` preprocessor reference); no runtime fetch, no network. Source-of-truth is the module-owned canonical. |
| installed expression map → Dorico | Dorico reads the file once when the user assigns the expression map to a channel. No runtime reload, no IPC. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-02-01 | T (Tampering) | Inno Setup [Code] block runs Pascal at install time | mitigate | The Pascal code only does `DirExists` probes against known Dorico paths under user-scope `%APPDATA%`, then `FileCopy` of a file already verified by the Inno Setup [Files] block. No external execution, no shell-out, no network. Code is reviewed in this plan and committed to the .iss source-of-truth files for reproducibility. |
| T-25-02-02 | E (Elevation of privilege) | macOS PKG postinstall runs as root | accept | The existing PKG postinstall pattern (Section 4b of pkg-creation.md) already runs as root and uses `chown` to drop ownership back to the actual user. Phase 25's added dual-write block follows the same pattern verbatim. No new privilege requirement. |
| T-25-02-03 | E (Elevation of privilege) | Windows EXE installer requires admin (PrivilegesRequired=admin in template) | accept | Admin is required for the existing VST3 install to `C:\Program Files\Common Files\VST3\` (pre-Phase-25 behavior). The .doricoexpmap dual-write goes to `%APPDATA%` (user-scope) — does not need admin, but inherits admin context because the installer process is already elevated. No additional risk surface. |
| T-25-02-04 | T (Tampering) | Multiple plugin installers writing to the same canonical .doricoexpmap path | accept | All 8 installers write the same content (idempotent overwrite per D-05). Per-plugin uninstall does NOT remove the shared resource (Inno `uninsneveruninstall` flag; macOS PKG postinstall does not register an uninstall hook for the shared resource — same idempotent semantic). User-edited copies will be overwritten by the next plugin install (documented behavior in README-doricoexpmap.txt SOURCE OF TRUTH section). |
| T-25-02-05 | I (Information disclosure) | install path probes for Dorico version under user HOME / APPDATA | accept | Probes only check `DirExists` for known Dorico version paths (4, 5, 6) — no enumeration of arbitrary user files, no PII access, no network. Standard user-scope filesystem reads. |
| T-25-02-06 | D (Denial of service) | failed Dorico-scan write | mitigate | Both PKG postinstall and Inno [Code] block silently log the failure and skip — never fatal-error. INST-04 fallback (the README emitted alongside the canonical) covers manual import. User experience degrades gracefully (manual one-time import) instead of failing the install. |
| T-25-02-07 | T (Tampering) | iscc /DRepoRoot=<path> command-line argument supplied by Windows host script | accept | The /DRepoRoot value is the repo root path on the Windows host — supplied by trusted local script (build-installer skill or manual invocation). No remote injection vector. iscc itself sanitizes preprocessor defines for valid identifier substitution. |
</threat_model>

<verification>
1. Both packaging templates (`pkg-creation.md`, `inno-template.iss`) AND the Windows skill docs (`SKILL-windows.md`) contain the Phase 25 dual-write blocks (per Task 1 grep gates).
2. All 8 plugins' `installer.iss` files exist with zero unsubstituted placeholders + dual-write entries + correct non-empty MyAppVersion (per Task 2 grep gates and BLOCKER #6 sanity check).
3. All 8 macOS PKG artifacts are physically materialized at `plugins/<Plugin>/dist/<Plugin>-OuariconAudio.pkg` AND each PKG's payload manifest contains both `.doricoexpmap` and `README-doricoexpmap.txt` (per Task 3 grep gates and BLOCKER #1).
4. macOS validation: PKG installer for spot-check trio (O-Lyrica + O-Reed + O-Formant) installs cleanly, dual-write physically lands at both macOS paths for all 3, Dorico 3-point gate PASS on O-Lyrica (manual checkpoint).
5. Windows validation: EXE installer for one representative plugin (O-Lyrica or O-Bells) compiles via `iscc /DRepoRoot=<repo-path>`, installs cleanly, dual-write physically lands at both Windows paths, Dorico 3-point gate PASS (manual checkpoint). HARD HALT if Windows access is blocked — do not commit Plan 25-02 with Windows gate skipped silently.
</verification>

<success_criteria>
Plan 25-02 succeeds when:
- INST-03 satisfied: all 8 plugins' installers (PKG + EXE source-of-truth) bundle the canonical .doricoexpmap + dual-write to Ouaricon shared + Dorico scan paths on both platforms. All 8 macOS PKG artifacts materialized AND payload-validated (BLOCKER #1 closed).
- INST-04 satisfied: README-doricoexpmap.txt is co-installed at the Ouaricon shared path on both platforms; manual-import fallback is documented + verified by the case where Dorico-scan-write was not applicable.
- macOS Dorico smoke gate PASS for O-Lyrica via formal PKG install pipeline; spot-check installs PASS for O-Reed and O-Formant (3/8 plugins beyond template-level grep verification per BLOCKER #1).
- Windows Dorico smoke gate PASS for one representative plugin (3/3 sub-gates) via the formal EXE install pipeline using `iscc /DRepoRoot=<path>` (BLOCKER #4/#5 resolution validated end-to-end) — OR the user explicitly accepts a deferral to a `25-04-windows-validation-PLAN.md` follow-up with macOS-only validation noted as PARTIAL.
- All checker BLOCKERs addressed: #1 (8 PKGs materialized + spot-check), #3 (SKILL-windows.md in files_modified), #4/#5 (Inno preprocessor approach, no mixed-slash paths), #6 (generalized version regex + per-plugin sanity check).
- All edits committed atomically (one commit covers Task 1's three template/skill edits + Task 2's 8 regenerated `installer.iss` files + Task 3's 8 materialized PKGs).
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-02-installer-bundling-sweep-SUMMARY.md` with:
- Files modified (full list including SKILL-windows.md per BLOCKER #3)
- Per-plugin VERSION extraction results (8 plugins; confirms BLOCKER #6 generalized regex worked across all conventions)
- macOS PKG materialization results (8 PKGs × 2 payload-manifest checks = 16 grep verifications per BLOCKER #1)
- macOS PKG validation results: spot-check trio (O-Lyrica + O-Reed + O-Formant) install + dual-write outcome per plugin; Dorico 3-point gate result for O-Lyrica with observed pitch values
- Windows EXE validation result (Inno Setup compilation result via `iscc /DRepoRoot=<path>`, install success, dual-write paths verified, Dorico 3-point gate result) — OR explicit halt-and-defer note if Windows access was blocked
- Cross-platform validation matrix table (8 plugins × 2 platforms; for the 5 non-installed plugins on macOS, note "PKG materialized + payload validated; structural change inherited from updated template — same install logic as the spot-check trio; no per-plugin behavioral variation expected")
- Confirmation of which checker BLOCKERs / WARNINGs were addressed (link to revision summary)
- Hand-off note to Plan 25-03: the user-facing install behavior is now stable across both platforms; Plan 25-03 captures the developer-facing internal notes (DOCS-01..04) describing the architecture, setup procedure, host quirks, and troubleshooting signatures based on what shipped
</output>
