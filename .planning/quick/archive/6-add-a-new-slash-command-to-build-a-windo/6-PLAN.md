---
phase: quick-6
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/commands/build-installer.md
  - .claude/skills/plugin-packaging/SKILL-windows.md
  - .claude/skills/plugin-packaging/assets/inno-template.iss
  - .claude/skills/plugin-packaging/assets/win-readme-template.txt
  - .claude/skills/plugin-packaging/references/inno-setup-creation.md
autonomous: true
must_haves:
  truths:
    - "User can run /build-installer PluginName and get a Windows EXE installer"
    - "Installer places the VST3 bundle into C:\\Program Files\\Common Files\\VST3\\"
    - "Installer uses Ouaricon Audio branding (company name, website, copyright)"
    - "Skill extracts PRODUCT_NAME and VERSION from plugin CMakeLists.txt"
    - "Skill verifies Inno Setup (iscc) is available before proceeding"
    - "Generated EXE installer is output to plugins/[PluginName]/dist/"
  artifacts:
    - path: ".claude/commands/build-installer.md"
      provides: "Slash command entry point for /build-installer"
      contains: "argument-hint"
    - path: ".claude/skills/plugin-packaging/SKILL-windows.md"
      provides: "Windows installer skill with full workflow"
      min_lines: 80
    - path: ".claude/skills/plugin-packaging/assets/inno-template.iss"
      provides: "Inno Setup script template with placeholders"
      contains: "{{PLUGIN_NAME}}"
    - path: ".claude/skills/plugin-packaging/assets/win-readme-template.txt"
      provides: "Windows installation readme template"
      contains: "{{PLUGIN_NAME}}"
    - path: ".claude/skills/plugin-packaging/references/inno-setup-creation.md"
      provides: "Step-by-step reference for Inno Setup EXE creation"
      contains: "iscc"
  key_links:
    - from: ".claude/commands/build-installer.md"
      to: ".claude/skills/plugin-packaging/SKILL-windows.md"
      via: "routing invoke"
      pattern: "invoke skill.*plugin-packaging.*windows"
    - from: ".claude/skills/plugin-packaging/SKILL-windows.md"
      to: ".claude/skills/plugin-packaging/assets/inno-template.iss"
      via: "template reference"
      pattern: "inno-template\\.iss"
    - from: ".claude/skills/plugin-packaging/SKILL-windows.md"
      to: "plugins/{{PLUGIN_NAME}}/CMakeLists.txt"
      via: "metadata extraction"
      pattern: "PRODUCT_NAME"
---

<objective>
Create a `/build-installer` slash command that builds a Windows EXE installer for a VST3 plugin using Inno Setup.

Purpose: Enable one-command Windows installer creation for any plugin in the project, mirroring the existing `/package` command (which creates macOS PKG installers) but for Windows using Inno Setup.

Output: A new slash command, a Windows-specific packaging skill document, an Inno Setup script template, a Windows readme template, and a step-by-step reference document.
</objective>

<execution_context>
@C:/Users/Taylor/.claude/get-shit-done/workflows/execute-plan.md
@C:/Users/Taylor/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@H:/dev/VST-development/.claude/commands/package.md
@H:/dev/VST-development/.claude/skills/plugin-packaging/SKILL.md
@H:/dev/VST-development/.claude/skills/plugin-packaging/assets/welcome-template.txt
@H:/dev/VST-development/.claude/skills/plugin-packaging/assets/readme-template.txt
@H:/dev/VST-development/.claude/skills/plugin-packaging/assets/conclusion-template.txt
@H:/dev/VST-development/.claude/branding.json
@H:/dev/VST-development/scripts/build-and-install.ps1
@H:/dev/VST-development/plugins/O-Chorus/CMakeLists.txt
@H:/dev/VST-development/PLUGINS.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Create the /build-installer command and Inno Setup templates</name>
  <files>
    .claude/commands/build-installer.md
    .claude/skills/plugin-packaging/assets/inno-template.iss
    .claude/skills/plugin-packaging/assets/win-readme-template.txt
  </files>
  <action>
Create three files following existing project conventions:

**1. `.claude/commands/build-installer.md`** -- Follow the exact pattern of `/package` command (`.claude/commands/package.md`):
- Frontmatter: `name: build-installer`, `description: Create Windows EXE installer for VST3 plugin distribution`, `argument-hint: <PluginName>`
- Preconditions (enforcement="blocking"):
  - Plugin status in PLUGINS.md MUST be "Installed" or "Working" (unlike /package which requires Installed, be more lenient since Windows may not have the install step done — the built VST3 artifact in the build folder is sufficient)
  - VST3 build artifact MUST exist at `build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[ProductName].vst3` (check build output, NOT system install path since Windows install requires admin)
  - Inno Setup compiler (`iscc`) MUST be on PATH or found at `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`
  - On precondition failure: guide user to build first (`/test PluginName build`), or install Inno Setup (`winget install JRSoftware.InnoSetup` or download from https://jrsoftware.org/isdl.php)
- Routing: `<invoke skill="plugin-packaging" with="$ARGUMENTS" mode="windows">` — route to the Windows skill doc (SKILL-windows.md)
- State contracts:
  - Reads: PLUGINS.md, `plugins/[PluginName]/CMakeLists.txt`, `.claude/branding.json`, build artifacts
  - Writes: `plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe`, `plugins/[PluginName]/dist/install-readme-windows.txt`
- Success criteria: EXE installer created, readme generated, files in dist/, user presented with decision menu
- Invocation examples: `/build-installer O-Chorus`, `/build-installer GainKnob`, `"Create Windows installer for O-Tremolo"`

**2. `.claude/skills/plugin-packaging/assets/inno-template.iss`** -- Inno Setup script template:
```iss
; Inno Setup Script for {{PLUGIN_NAME}}
; Generated by Plugin Freedom System

#define MyAppName "{{PRODUCT_NAME}}"
#define MyAppVersion "{{VERSION}}"
#define MyAppPublisher "Ouaricon Audio"
#define MyAppURL "https://ouaricon.audio"
#define MyPluginName "{{PLUGIN_NAME}}"

[Setup]
AppId={{{{APP_GUID}}}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={commonpf}\Common Files\VST3\{#MyAppName}.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir={{OUTPUT_DIR}}
OutputBaseFilename={{PLUGIN_NAME}}-OuariconAudio-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName={#MyAppName} VST3 by Ouaricon Audio
LicenseFile=
SetupIconFile=
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install {#MyAppName} v{#MyAppVersion} by Ouaricon Audio on your computer.%n%n{#MyAppName} is a VST3 audio plugin. After installation, open your DAW and scan for new plugins.

[Files]
; Install the VST3 bundle (entire directory tree)
Source: "{{VST3_SOURCE_PATH}}\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
; No post-install actions needed for VST3

[UninstallDelete]
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"

[Code]
// Optional: Clear DAW plugin caches after install
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
      // Ableton will rescan on next launch
      Log('Ableton preferences directory found - plugin rescan will occur on next launch');
    end;
  end;
end;
```

IMPORTANT template notes to include as comments in the file:
- `{{APP_GUID}}` must be generated uniquely per plugin (the skill will generate a deterministic GUID from the plugin name)
- `{{VST3_SOURCE_PATH}}` points to the build artifact directory
- `{{OUTPUT_DIR}}` is the dist/ output location
- The `{commonpf}\Common Files\VST3\` path is the standard Windows VST3 location (`C:\Program Files\Common Files\VST3\`)
- `ArchitecturesInstallIn64BitMode=x64compatible` ensures 64-bit installation
- `PrivilegesRequired=admin` because writing to Program Files requires elevation
- `DisableDirPage=yes` because VST3 install location is standardized

**3. `.claude/skills/plugin-packaging/assets/win-readme-template.txt`** -- Windows readme:
Follow the pattern of `readme-template.txt` but adapted for Windows:
- Replace macOS paths with Windows paths (`C:\Program Files\Common Files\VST3\`)
- Remove AU references (Windows = VST3 only)
- Include DAW-specific instructions for Windows (Ableton, FL Studio, Reaper, Bitwig)
- System requirements: Windows 10/11, 64-bit, compatible DAW
- Include uninstall instructions (Add/Remove Programs)
  </action>
  <verify>
Verify all three files exist and have correct structure:
- `Test-Path "H:/dev/VST-development/.claude/commands/build-installer.md"` returns True
- `Test-Path "H:/dev/VST-development/.claude/skills/plugin-packaging/assets/inno-template.iss"` returns True
- `Test-Path "H:/dev/VST-development/.claude/skills/plugin-packaging/assets/win-readme-template.txt"` returns True
- Command file contains `argument-hint: <PluginName>` and `preconditions` and `routing`
- ISS template contains `{{PLUGIN_NAME}}`, `{{PRODUCT_NAME}}`, `{{VERSION}}`, `[Setup]`, `[Files]`
- Readme template contains `{{PLUGIN_NAME}}` and Windows-specific paths
  </verify>
  <done>
- /build-installer command file matches project conventions (same structure as /package)
- Inno Setup template is a valid .iss file with all necessary placeholders
- Windows readme template covers VST3 installation, DAW setup, and uninstall
  </done>
</task>

<task type="auto">
  <name>Task 2: Create the Windows packaging skill and reference documentation</name>
  <files>
    .claude/skills/plugin-packaging/SKILL-windows.md
    .claude/skills/plugin-packaging/references/inno-setup-creation.md
  </files>
  <action>
Create two files that together provide the complete implementation guide for Windows EXE installer creation.

**1. `.claude/skills/plugin-packaging/SKILL-windows.md`** -- Follow the exact structure of `SKILL.md` (the macOS packaging skill):

Frontmatter:
```
---
name: plugin-packaging-windows
description: Create Windows EXE installers for VST3 plugin distribution using Inno Setup. Use when user requests to build a Windows installer, create setup.exe, or mentions distributing a Windows plugin. Invoked by /build-installer command or natural language like 'create Windows installer for O-Chorus'.
---
```

Purpose: Create professional EXE installers for sharing VST3 plugins on Windows.

Workflow (critical_sequence, enforcement="strict") with progress checklist:
```
Plugin Windows Installer Progress:
- [ ] 1. Prerequisites verified (plugin built, Inno Setup available)
- [ ] 2. Metadata extracted (PRODUCT_NAME, VERSION from CMakeLists.txt)
- [ ] 3. Inno Setup script generated (template populated)
- [ ] 4. Installer compiled (iscc invoked)
- [ ] 5. Distribution package output (dist/ created, readme generated)
```

**Step 1: Verify Prerequisites**
- Read PLUGINS.md, verify status is "Installed" or "Working"
- Locate VST3 build artifact: check `build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[ProductName].vst3`
  - If not found, try building first: `powershell -File scripts/build-and-install.ps1 [PluginName] -NoInstall`
  - If still not found after build attempt, abort and guide user
- Find Inno Setup compiler:
  1. Check PATH: `where iscc 2>nul`
  2. Check default location: `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`
  3. If not found: abort with install instructions: `winget install JRSoftware.InnoSetup` or https://jrsoftware.org/isdl.php
- Blocking: If prerequisites fail, guide user with specific fix instructions

**Step 2: Extract Plugin Metadata**
- Read `plugins/[PluginName]/CMakeLists.txt`
- Extract PRODUCT_NAME using PowerShell pattern matching (see reference doc Section 2)
  - Note: PRODUCT_NAME may include `${OUARICON_DEV_SUFFIX}` — strip this for the installer (use production name without -dev suffix)
- Extract VERSION from CMakeLists.txt
- Read `.claude/branding.json` for company name, website, copyright year
- Read PLUGINS.md entry for description and plugin type
- Generate deterministic APP_GUID from plugin name using PowerShell:
  ```powershell
  $guidBytes = [System.Text.Encoding]::UTF8.GetBytes("OuariconAudio-$PluginName")
  $hash = [System.Security.Cryptography.SHA256]::Create().ComputeHash($guidBytes)
  $guid = [guid]::new($hash[0..15])
  ```

**Step 3: Generate Inno Setup Script**
- Read template from `assets/inno-template.iss`
- Replace all `{{PLACEHOLDER}}` values with extracted metadata
- Write populated script to temp location: `plugins/[PluginName]/dist/installer.iss`
- The VST3_SOURCE_PATH should point to the full path of the build artifact directory

**Step 4: Compile Installer**
- Invoke Inno Setup compiler:
  ```powershell
  # Find iscc path
  $iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source
  if (-not $iscc) { $iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" }

  & $iscc "plugins/[PluginName]/dist/installer.iss"
  ```
- Verify EXE was created at `plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe`
- Report file size
- Validation: ONLY proceed to step 5 when EXE exists

**Step 5: Output Distribution Package**
- Ensure `plugins/[PluginName]/dist/` directory exists
- Generate `install-readme-windows.txt` from `assets/win-readme-template.txt` with populated placeholders
- Clean up temp .iss file (move to dist/ or delete — keep it in dist/ for reproducibility)
- Git commit: `git add plugins/[PluginName]/dist/ && git commit -m "feat([PluginName]): create v[VERSION] Windows installer"`
- Update PLUGINS.md with Windows packaging metadata:
  ```markdown
  **Last Packaged (Windows):** YYYY-MM-DD
  **Windows Installer:** plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe (X.X MB)
  ```
- Display summary:
  ```
  [PluginName] Windows installer created successfully

  Created: plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe (X.X MB)

  Distribution package includes:
  - [PluginName]-OuariconAudio-Setup.exe (Windows installer)
  - install-readme-windows.txt (installation guide)
  - installer.iss (Inno Setup script, for reproducibility)

  The installer will:
  - Install VST3 to C:\Program Files\Common Files\VST3\
  - Add entry to Add/Remove Programs for clean uninstall
  - Require administrator privileges (writes to Program Files)
  ```

Decision menu (same pattern as SKILL.md):
```
What's next?
1. Test installer (recommended) - Run the setup.exe and verify in DAW
2. Build installer for another plugin - /build-installer [OtherPlugin]
3. View installation guide - Show install-readme-windows.txt
4. Other

Choose (1-4): _
```

Integration Points section:
- Invoked by: `/build-installer [PluginName]` command
- Reads: PLUGINS.md, CMakeLists.txt, branding.json, build artifacts
- Creates: dist/[PluginName]-OuariconAudio-Setup.exe, dist/install-readme-windows.txt, dist/installer.iss
- Updates: PLUGINS.md (packaging metadata), git (commit dist/)

Error Handling section:
- If build artifact not found: guide to build first
- If Inno Setup not installed: provide install instructions
- If iscc compilation fails: display error output, suggest checking the .iss file
- If permission denied on dist/: suggest running as admin or checking file locks

Notes for Claude section:
- Extract PRODUCT_NAME carefully — strip `${OUARICON_DEV_SUFFIX}` for installer name (production branding)
- GUID must be deterministic per plugin (same plugin always gets same GUID for upgrade support)
- VST3 on Windows is a directory (.vst3 folder), not a single file — use recursesubdirs in Inno Setup
- The installer.iss file is kept in dist/ so the user can manually recompile or customize
- Branding consistency: "Ouaricon Audio" (not development branding) in installer

**2. `.claude/skills/plugin-packaging/references/inno-setup-creation.md`** -- Detailed reference:

Structure it like `pkg-creation.md` with numbered sections:

**Section 1: Prerequisites Verification**
- PowerShell commands to check Inno Setup availability
- Commands to verify build artifact exists
- PRODUCT_NAME extraction via PowerShell (handle the `${OUARICON_DEV_SUFFIX}` stripping)

**Section 2: Metadata Extraction**
- PowerShell to parse CMakeLists.txt for PRODUCT_NAME, VERSION
- Stripping dev suffix: `$productName = $raw -replace '\$\{OUARICON_DEV_SUFFIX\}', ''`
- Reading branding.json via `ConvertFrom-Json`
- Generating deterministic GUID from plugin name

**Section 3: Inno Setup Script Generation**
- Template loading and placeholder replacement
- Key Inno Setup directives explained:
  - `{commonpf}` = `C:\Program Files` (handles 64-bit correctly)
  - `ArchitecturesInstallIn64BitMode` for 64-bit-only plugins
  - `DisableDirPage=yes` since VST3 location is standardized
  - `Compression=lzma2` for best compression
  - Recurse subdirectories for .vst3 bundle

**Section 4: Compilation**
- Full iscc command with path resolution
- Expected output and error handling
- How to verify the EXE was created

**Section 5: Verification and Testing**
- Testing checklist:
  1. Run the .exe installer
  2. Verify VST3 appears in `C:\Program Files\Common Files\VST3\`
  3. Open DAW, rescan plugins
  4. Verify plugin loads and processes audio
  5. Test uninstall via Add/Remove Programs
  6. Verify VST3 removed after uninstall

**Section 6: Error Scenarios**
- iscc not found
- Build artifact missing
- Permission denied
- Invalid .iss syntax
- Antivirus blocking (common with generated EXEs — note this for user)
  </action>
  <verify>
Verify both files exist and have correct structure:
- `Test-Path "H:/dev/VST-development/.claude/skills/plugin-packaging/SKILL-windows.md"` returns True
- `Test-Path "H:/dev/VST-development/.claude/skills/plugin-packaging/references/inno-setup-creation.md"` returns True
- SKILL-windows.md contains: frontmatter with `name: plugin-packaging-windows`, `critical_sequence`, all 5 workflow steps, Integration Points, Decision Menu, Error Handling, Notes for Claude
- Reference doc contains: Section 1 through Section 6, PowerShell code blocks, iscc commands
- Cross-references are correct: SKILL-windows.md references `assets/inno-template.iss` and `references/inno-setup-creation.md`
  </verify>
  <done>
- SKILL-windows.md provides complete Windows packaging workflow matching SKILL.md conventions
- Reference doc provides copy-paste-ready PowerShell commands for every step
- All cross-references between command, skill, template, and reference are consistent
- Inno Setup workflow covers: prerequisite check, metadata extraction, script generation, compilation, output
  </done>
</task>

</tasks>

<verification>
After both tasks complete, verify end-to-end consistency:

1. Command routes correctly:
   - `.claude/commands/build-installer.md` references plugin-packaging skill with windows mode
   - The skill file SKILL-windows.md exists where the command expects it

2. Template completeness:
   - `inno-template.iss` has all placeholders that SKILL-windows.md populates
   - `win-readme-template.txt` has all placeholders that SKILL-windows.md populates

3. Reference accuracy:
   - `inno-setup-creation.md` covers every step referenced in SKILL-windows.md
   - PowerShell commands use correct paths matching `build-and-install.ps1` build output structure

4. Convention compliance:
   - Command frontmatter matches `/package` pattern
   - Skill structure matches `SKILL.md` pattern (critical_sequence, decision_gate, integration points)
   - Branding uses "Ouaricon Audio" (production) not "Ouaricon Audio Development"
   - File naming follows project conventions
</verification>

<success_criteria>
- 5 files created: command, skill, ISS template, readme template, reference doc
- /build-installer command follows exact conventions of /package command
- SKILL-windows.md follows exact conventions of SKILL.md (macOS counterpart)
- Inno Setup template produces a valid .iss file when placeholders are replaced
- Workflow covers: prerequisite check, metadata extraction, Inno Setup script generation, iscc compilation, output with decision menu
- All Windows-specific details correct: VST3-only (no AU), Program Files paths, admin privileges, Add/Remove Programs uninstall
- Branding consistent with `.claude/branding.json` (Ouaricon Audio, ouaricon.audio, Taylor Brook)
</success_criteria>

<output>
After completion, create `.planning/quick/6-add-a-new-slash-command-to-build-a-windo/6-SUMMARY.md`
</output>
