---
name: plugin-packaging-windows
description: Create Windows EXE installers for VST3 plugin distribution using Inno Setup. Use when user requests to build a Windows installer, create setup.exe, or mentions distributing a Windows plugin. Invoked by /build-installer command or natural language like 'create Windows installer for O-Chorus'.
---

# plugin-packaging-windows Skill

**Purpose:** Create professional EXE installers for sharing VST3 plugins on Windows using Inno Setup.

## Overview

Generates Windows EXE installers with branded UI and automated VST3 plugin installation. Installers place the VST3 bundle into the standard Windows location (`C:\Program Files\Common Files\VST3\`) and register with Add/Remove Programs for clean uninstall.

## Workflow

<critical_sequence enforcement="strict" blocking="true">

**Track your progress:**

```
Plugin Windows Installer Progress:
- [ ] 1. Prerequisites verified (plugin built, Inno Setup available)
- [ ] 2. Metadata extracted (PRODUCT_NAME, VERSION from CMakeLists.txt)
- [ ] 3. Inno Setup script generated (template populated)
- [ ] 4. Installer compiled (iscc invoked)
- [ ] 5. Distribution package output (dist/ created, readme generated)
```

---

### 1. Verify Prerequisites

Check plugin is ready for installer creation:
- Read PLUGINS.md, verify status is 📦 Installed or ✅ Working
- Locate VST3 build artifact: check `build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[ProductName].vst3`
  - If not found, attempt building first: `powershell -File scripts/build-and-install.ps1 [PluginName] -NoInstall`
  - If still not found after build attempt, abort and guide user
- Find Inno Setup compiler:
  1. Check PATH: `where iscc 2>nul`
  2. Check default location: `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`
  3. If not found: abort with install instructions: `winget install JRSoftware.InnoSetup` or https://jrsoftware.org/isdl.php

**Blocking:** If prerequisites fail, guide user with specific fix instructions.

**Preconditions verified in this step:**
- Plugin status is 📦 Installed or ✅ Working
- VST3 build artifact exists in build output directory
- Inno Setup compiler (iscc) is available

### 2. Extract Plugin Metadata

**Read multiple files in parallel** using multiple Read tool calls:
- PLUGINS.md entry for plugin (version, description, status)
- `plugins/[PluginName]/CMakeLists.txt` (for PRODUCT_NAME and VERSION extraction)
- `.claude/branding.json` (for company name, website, copyright)

Extract PRODUCT_NAME from CMakeLists.txt:
```powershell
$content = Get-Content "plugins/$PluginName/CMakeLists.txt" -Raw
if ($content -match 'PRODUCT_NAME\s+"([^"]+)"') {
    $productName = $Matches[1]
    # Strip dev suffix for production installer branding
    $productName = $productName -replace '\$\{OUARICON_DEV_SUFFIX\}', ''
}
```

Extract VERSION from CMakeLists.txt:
```powershell
if ($content -match 'VERSION\s+(\d+\.\d+\.\d+)') {
    $version = $Matches[1]
}
```

Generate deterministic APP_GUID from plugin name (same plugin always gets same GUID for upgrade support):
```powershell
$guidBytes = [System.Text.Encoding]::UTF8.GetBytes("OuariconAudio-$PluginName")
$hash = [System.Security.Cryptography.SHA256]::Create().ComputeHash($guidBytes)
$guid = [guid]::new($hash[0..15])
```

Read `.claude/branding.json` via `ConvertFrom-Json` for company name and website.

See Section 2 in references/inno-setup-creation.md for complete extraction commands.

**Template variables to extract:**
- {{PLUGIN_NAME}}, {{PRODUCT_NAME}}, {{VERSION}}
- {{APP_GUID}}, {{VST3_SOURCE_PATH}}, {{OUTPUT_DIR}}
- {{DESCRIPTION}}, {{FEATURES}}, {{PARAMETERS}}

### 3. Generate Inno Setup Script

Read template from `assets/inno-template.iss` and replace all `{{PLACEHOLDER}}` values with extracted metadata:

- `{{PLUGIN_NAME}}` -> Plugin directory name (e.g., O-Chorus)
- `{{PRODUCT_NAME}}` -> PRODUCT_NAME from CMakeLists.txt with dev suffix stripped (e.g., O-Chorus)
- `{{VERSION}}` -> Version from CMakeLists.txt (e.g., 1.2.0)
- `{{APP_GUID}}` -> Deterministic GUID generated from plugin name
- `{{VST3_SOURCE_PATH}}` -> Full absolute path to the build artifact directory (e.g., `H:\dev\VST-development\build\plugins\O-Chorus\OuariconChorus_artefacts\Release\VST3\O-Chorus.vst3`)
- `{{OUTPUT_DIR}}` -> Absolute path to dist/ output (e.g., `H:\dev\VST-development\plugins\O-Chorus\dist`)

Write populated script to: `plugins/[PluginName]/dist/installer.iss`

See Section 3 in references/inno-setup-creation.md for complete template replacement process.

### 4. Compile Installer

Invoke Inno Setup compiler:
```powershell
# Find iscc path
$iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source
if (-not $iscc) { $iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" }

# Compile the installer
& $iscc "plugins/$PluginName/dist/installer.iss"
```

Verify EXE was created at `plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe`.

Report file size:
```powershell
$exe = Get-Item "plugins/$PluginName/dist/$PluginName-OuariconAudio-Setup.exe"
$sizeMB = [math]::Round($exe.Length / 1MB, 1)
```

See Section 4 in references/inno-setup-creation.md for complete compilation process.

**Validation:** ONLY proceed to step 5 when EXE exists.

### 5. Output Distribution Package

Finalize and present to user:

**5a. Ensure dist directory exists:**
```powershell
$distDir = "plugins/$PluginName/dist"
if (-not (Test-Path $distDir)) { New-Item -ItemType Directory -Path $distDir -Force }
```

**5b. Generate install-readme-windows.txt** from `assets/win-readme-template.txt` with populated placeholders.

**5c. Keep installer.iss in dist/** for reproducibility (user can manually recompile or customize).

**5d. Git commit:**
```bash
git add plugins/[PluginName]/dist/
git commit -m "feat([PluginName]): create v[VERSION] Windows installer"
```

**5e. Update PLUGINS.md** with Windows packaging metadata:
```markdown
**Last Packaged (Windows):** YYYY-MM-DD
**Windows Installer:** plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe (X.X MB)
```

**5f. Display summary:**
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

</critical_sequence>

---

## Integration Points

**Invoked by:**
- `/build-installer [PluginName]` command
- Natural language: "Create Windows installer for O-Chorus", "Build setup.exe for GainKnob"

**Invokes:**
- None (terminal skill - does not invoke other skills)

**Reads:**
- `PLUGINS.md` -> Plugin metadata
- `plugins/[PluginName]/CMakeLists.txt` -> PRODUCT_NAME and VERSION extraction
- `.claude/branding.json` -> Company name, website, copyright
- `build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[ProductName].vst3` -> Source build artifact
- `assets/inno-template.iss` -> Inno Setup script template
- `assets/win-readme-template.txt` -> Windows readme template

**Creates:**
- `plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe` -> Windows installer
- `plugins/[PluginName]/dist/install-readme-windows.txt` -> Installation guide
- `plugins/[PluginName]/dist/installer.iss` -> Populated Inno Setup script (kept for reproducibility)

**Updates:**
- `PLUGINS.md` -> Add **Last Packaged (Windows):** timestamp and **Windows Installer:** path/size
- Git repository -> Commit dist/ folder with distribution package

---

## Decision Menu

<decision_gate type="checkpoint" enforcement="strict">

After successful installer creation, present this menu and WAIT for user response:

```
[PluginName] Windows installer created successfully

Created: plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe (X.X MB)

What's next?
1. Test installer (recommended) - Run the setup.exe and verify in DAW
2. Build installer for another plugin - /build-installer [OtherPlugin]
3. View installation guide - Show install-readme-windows.txt
4. Other

Choose (1-4): _
```

**Option handlers:**
1. **Test installer** -> Provide testing checklist (see references/inno-setup-creation.md Section 5)
2. **Build another** -> Prompt for plugin name, re-invoke skill
3. **View guide** -> Display install-readme-windows.txt contents
4. **Other** -> Open-ended response

</decision_gate>

---

## Error Handling

### Build artifact not found
**Symptom:** VST3 build artifact missing from build output
**Fix:** Guide user to build first: `powershell -File scripts/build-and-install.ps1 [PluginName] -NoInstall`

### Inno Setup not installed
**Symptom:** iscc not found on PATH or default location
**Fix:** `winget install JRSoftware.InnoSetup` or download from https://jrsoftware.org/isdl.php

### iscc compilation fails
**Symptom:** Inno Setup compiler returns error during compilation
**Fix:** Display error output. Common causes: invalid paths in .iss file, missing source files. Check the generated installer.iss in dist/ for issues.

### Permission denied on dist/
**Symptom:** Cannot write to plugins/[PluginName]/dist/
**Fix:** Check if another process has files locked. Try running as Administrator if needed.

### Antivirus blocking
**Symptom:** Generated EXE flagged or quarantined by antivirus
**Fix:** This is common with newly generated EXE installers. Add an exception in your antivirus for the dist/ directory, or temporarily disable real-time scanning during compilation.

For detailed error scenarios, see references/inno-setup-creation.md Section 6.

---

## Success Criteria

Installer creation succeeds when:
- EXE installer created with Inno Setup branded wizard
- Installer places VST3 into `C:\Program Files\Common Files\VST3\`
- Installer registers in Add/Remove Programs for clean uninstall
- Installation guide generated for Windows
- All files placed in `plugins/[PluginName]/dist/`
- File size reported
- User knows what files to share

**NOT required for success:**
- Actually testing the installer (recommended but not blocking)
- Code signing (future enhancement - Inno Setup supports Authenticode signing)
- Notarization (Windows does not have an equivalent requirement)

---

## Notes for Claude

**When executing this skill:**

1. Extract PRODUCT_NAME carefully -- strip `${OUARICON_DEV_SUFFIX}` for installer name (production branding, not development)
2. GUID must be deterministic per plugin (same plugin always gets same GUID for upgrade support)
3. VST3 on Windows is a directory (.vst3 folder), not a single file -- use `recursesubdirs` in Inno Setup
4. The installer.iss file is kept in dist/ so the user can manually recompile or customize
5. Branding consistency: always use "Ouaricon Audio" (production branding, not "Ouaricon Audio Development")
6. The build artifact path uses the CMake target name (e.g., OuariconChorus) not the plugin directory name (e.g., O-Chorus)
7. Report file size to user (helpful for sharing over email/Dropbox)
8. Use absolute paths in the .iss file for VST3_SOURCE_PATH and OUTPUT_DIR

**Branding consistency:**
- Always use "Ouaricon Audio" in installer title and publisher
- Format: "[PluginName] VST3 by Ouaricon Audio"
- Website: https://ouaricon.audio

**Path conventions:**
- Build artifact: `build/plugins/[PluginName]/[CMakeTarget]_artefacts/Release/VST3/[ProductName].vst3`
- Install target: `C:\Program Files\Common Files\VST3\[ProductName].vst3`
- Output: `plugins/[PluginName]/dist/[PluginName]-OuariconAudio-Setup.exe`
