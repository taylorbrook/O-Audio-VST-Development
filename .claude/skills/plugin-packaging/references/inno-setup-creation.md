# Inno Setup EXE Creation Reference

Complete step-by-step implementation for creating Windows EXE installers using Inno Setup.

---

## Section 1: Prerequisites Verification

### 1.1 Check Plugin Status

```powershell
# Verify plugin exists in PLUGINS.md with valid status
$pluginsContent = Get-Content "PLUGINS.md" -Raw
if ($pluginsContent -match "$PluginName\s*\|.*?(Installed|Working)") {
    Write-Host "Plugin status verified"
} else {
    Write-Error "Cannot create installer for $PluginName - plugin not found or not built"
    Write-Host "Build first: powershell -File scripts/build-and-install.ps1 $PluginName -NoInstall"
    exit 1
}
```

### 1.2 Check Inno Setup Availability

```powershell
# Check PATH first
$iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source

# Fall back to default installation path
if (-not $iscc) {
    $defaultPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    if (Test-Path $defaultPath) {
        $iscc = $defaultPath
    }
}

# Abort if not found
if (-not $iscc) {
    Write-Error "Inno Setup compiler (iscc) not found"
    Write-Host "Install with: winget install JRSoftware.InnoSetup"
    Write-Host "Or download from: https://jrsoftware.org/isdl.php"
    exit 1
}

Write-Host "Inno Setup found: $iscc"
```

### 1.3 Verify Build Artifact Exists

```powershell
# Extract PRODUCT_NAME first (see Section 2)
$vst3Path = "build\plugins\$PluginName\${CMakeTarget}_artefacts\Release\VST3\$ProductName.vst3"

if (-not (Test-Path $vst3Path)) {
    Write-Error "VST3 build artifact not found: $vst3Path"
    Write-Host "Build first: powershell -File scripts/build-and-install.ps1 $PluginName -NoInstall"
    exit 1
}

Write-Host "VST3 artifact found: $vst3Path"
```

---

## Section 2: Metadata Extraction

### 2.1 Extract PRODUCT_NAME from CMakeLists.txt

```powershell
$cmakeContent = Get-Content "plugins/$PluginName/CMakeLists.txt" -Raw

# Extract PRODUCT_NAME
if ($cmakeContent -match 'PRODUCT_NAME\s+"([^"]+)"') {
    $rawProductName = $Matches[1]
    # Strip dev suffix for production installer branding
    $productName = $rawProductName -replace '\$\{OUARICON_DEV_SUFFIX\}', ''
    Write-Host "PRODUCT_NAME: $productName (raw: $rawProductName)"
} else {
    Write-Warning "Could not extract PRODUCT_NAME, using directory name as fallback"
    $productName = $PluginName
}
```

**Important:** PRODUCT_NAME in CMakeLists.txt may contain `${OUARICON_DEV_SUFFIX}` which appends "-dev" during development builds. For the installer, always strip this to use the production name.

### 2.2 Extract VERSION

```powershell
if ($cmakeContent -match 'VERSION\s+(\d+\.\d+\.\d+)') {
    $version = $Matches[1]
    Write-Host "VERSION: $version"
} else {
    Write-Warning "Could not extract VERSION, using 1.0.0 as fallback"
    $version = "1.0.0"
}
```

### 2.3 Extract CMake Target Name

The CMake target name is used in the build artifact path. It is the first argument to `juce_add_plugin`:

```powershell
if ($cmakeContent -match 'juce_add_plugin\((\w+)') {
    $cmakeTarget = $Matches[1]
    Write-Host "CMake target: $cmakeTarget"
} else {
    Write-Warning "Could not extract CMake target name"
    $cmakeTarget = $PluginName
}
```

### 2.4 Read Branding

```powershell
$branding = Get-Content ".claude/branding.json" -Raw | ConvertFrom-Json
$companyName = $branding.company.production.full_name    # "Ouaricon Audio"
$website = $branding.company.website                      # "https://ouaricon.audio"
$developer = $branding.developer.name                     # "Taylor Brook"
$copyrightStart = $branding.company.copyright_year_start  # 2025
```

### 2.5 Generate Deterministic GUID

Each plugin needs a unique but deterministic GUID so that Inno Setup can properly handle upgrades (installing over a previous version):

```powershell
$guidBytes = [System.Text.Encoding]::UTF8.GetBytes("OuariconAudio-$PluginName")
$hash = [System.Security.Cryptography.SHA256]::Create().ComputeHash($guidBytes)
$guid = [guid]::new($hash[0..15])
Write-Host "APP_GUID: $guid"
```

This ensures the same plugin always produces the same GUID, enabling clean upgrade behavior.

---

## Section 3: Inno Setup Script Generation

### 3.1 Load Template

```powershell
$template = Get-Content ".claude/skills/plugin-packaging/assets/inno-template.iss" -Raw
```

### 3.2 Prepare Paths

```powershell
$projectRoot = (Get-Location).Path
$vst3SourcePath = Join-Path $projectRoot "build\plugins\$PluginName\${cmakeTarget}_artefacts\Release\VST3\$productName.vst3"
$outputDir = Join-Path $projectRoot "plugins\$PluginName\dist"

# Ensure dist directory exists
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}
```

### 3.3 Replace Placeholders

```powershell
$issContent = $template
$issContent = $issContent -replace '\{\{PLUGIN_NAME\}\}', $PluginName
$issContent = $issContent -replace '\{\{PRODUCT_NAME\}\}', $productName
$issContent = $issContent -replace '\{\{VERSION\}\}', $version
$issContent = $issContent -replace '\{\{APP_GUID\}\}', $guid.ToString()
$issContent = $issContent -replace '\{\{VST3_SOURCE_PATH\}\}', $vst3SourcePath
$issContent = $issContent -replace '\{\{OUTPUT_DIR\}\}', $outputDir

# Write populated script to dist/
$issPath = Join-Path $outputDir "installer.iss"
$issContent | Out-File -FilePath $issPath -Encoding UTF8
Write-Host "Generated: $issPath"
```

### 3.4 Microtonal Suite template variables (Phase 25 v3 Path B)

Plugins that consume the `note-expression` module bundle the canonical Dorico expression-map library bundle. The asset lands at `%APPDATA%\Ouaricon\Microtonal Suite\` on user install. The user performs a one-time `Library → Library Manager → Import…` per machine to activate the map. No Dorico auto-discovery directory is written; the install destination is Dorico-version-agnostic (D-07).

Two new template variables in `inno-template.iss` `[Files]` section:

- **`{{MICROTONAL_SUITE_DORICOLIB_PATH}}`** — absolute path to `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` at packaging time. Resolve from project root.
- **`{{MICROTONAL_SUITE_README_PATH}}`** — absolute path to `modules/tuning/note-expression/resources/README-microtonal-suite.txt` at packaging time. Resolve from project root.

Both lines write to `{userappdata}\Ouaricon\Microtonal Suite` with `Flags: ignoreversion` (single canonical asset; idempotent overwrite across the 8-plugin cohort).

PowerShell substitution (mirroring the `{{VST3_SOURCE_PATH}}` pattern in 3.3):

```powershell
# Adjust depth per script location; from a packaging script in plugins/<P>/scripts/, the repo root is 4 levels up.
$repoRoot = Resolve-Path "${PSScriptRoot}\..\..\..\.."
$suiteDoricolib = "$repoRoot\modules\tuning\note-expression\resources\library\Ouaricon-VST3-NoteExpression.doricolib"
$suiteReadme = "$repoRoot\modules\tuning\note-expression\resources\README-microtonal-suite.txt"
$issContent = $issContent -replace '\{\{MICROTONAL_SUITE_DORICOLIB_PATH\}\}', $suiteDoricolib
$issContent = $issContent -replace '\{\{MICROTONAL_SUITE_README_PATH\}\}', $suiteReadme
```

If the orchestrating PowerShell already runs from the project root (typical for the `build-installer` skill), substitute `$projectRoot = (Get-Location).Path` from Section 3.2 directly:

```powershell
$suiteDoricolib = Join-Path $projectRoot "modules\tuning\note-expression\resources\library\Ouaricon-VST3-NoteExpression.doricolib"
$suiteReadme = Join-Path $projectRoot "modules\tuning\note-expression\resources\README-microtonal-suite.txt"
$issContent = $issContent -replace '\{\{MICROTONAL_SUITE_DORICOLIB_PATH\}\}', $suiteDoricolib
$issContent = $issContent -replace '\{\{MICROTONAL_SUITE_README_PATH\}\}', $suiteReadme
```

The activation hint is logged by the template's `[Code]` `CurStepChanged(ssPostInstall)` block (no PowerShell substitution required for that side).

### 3.5 Key Inno Setup Directives Explained

- **`{commonpf}`** = `C:\Program Files` on 64-bit Windows (handles 64-bit correctly with `ArchitecturesInstallIn64BitMode`)
- **`ArchitecturesInstallIn64BitMode=x64compatible`** = ensures 64-bit installation mode for 64-bit-only VST3 plugins
- **`DisableDirPage=yes`** = hides the directory selection page since VST3 install location is standardized
- **`Compression=lzma2`** = best compression ratio for the installer
- **`SolidCompression=yes`** = compresses all files as a single block for better ratio
- **`recursesubdirs createallsubdirs`** = VST3 on Windows is a directory tree (.vst3 folder), not a single file
- **`PrivilegesRequired=admin`** = required because `C:\Program Files\Common Files\VST3\` requires elevation
- **`AppId`** = deterministic GUID per plugin, enables upgrade detection

---

## Section 4: Compilation

### 4.1 Invoke iscc

```powershell
# Find iscc
$iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source
if (-not $iscc) { $iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" }

# Compile
$issPath = "plugins\$PluginName\dist\installer.iss"
Write-Host "Compiling installer..."
& $iscc $issPath
```

### 4.2 Expected Output

```
Inno Setup 6 Compiler
Copyright (C) 1997-2024 Jordan Russell. All Rights Reserved.

Compiling "plugins\O-Chorus\dist\installer.iss"
Compression: lzma2, SolidCompression=yes
Output directory: H:\dev\VST-development\plugins\O-Chorus\dist
Output filename: O-Chorus-OuariconAudio-Setup.exe
Successful compile (X.XX sec). Resulting Setup program filename is:
H:\dev\VST-development\plugins\O-Chorus\dist\O-Chorus-OuariconAudio-Setup.exe
```

### 4.3 Verify Output

```powershell
$exePath = "plugins\$PluginName\dist\$PluginName-OuariconAudio-Setup.exe"
if (-not (Test-Path $exePath)) {
    Write-Error "Installer EXE not created: $exePath"
    Write-Host "Check the .iss file for errors: plugins\$PluginName\dist\installer.iss"
    exit 1
}

$exe = Get-Item $exePath
$sizeMB = [math]::Round($exe.Length / 1MB, 1)
Write-Host "Installer created: $exePath ($sizeMB MB)"
```

---

## Section 5: Verification and Testing

### 5.1 Testing Checklist

After creating the installer, verify with this checklist:

1. **Run the .exe installer** - Double-click the setup file, verify the wizard appears with Ouaricon Audio branding
2. **Check UAC prompt** - Installer should request administrator privileges
3. **Verify VST3 appears** - Check `C:\Program Files\Common Files\VST3\[ProductName].vst3` exists
4. **Open DAW, rescan plugins** - Plugin should appear under "Ouaricon Audio" or by name
5. **Verify plugin loads and processes audio** - Insert on a track, play audio through it
6. **Test uninstall via Add/Remove Programs** - Search for "[ProductName] VST3 by Ouaricon Audio" in Settings > Apps
7. **Verify VST3 removed after uninstall** - Check that the .vst3 folder was deleted from Common Files\VST3

### 5.2 Quick Smoke Test

```powershell
# After running installer, verify VST3 was placed correctly
$installedPath = "C:\Program Files\Common Files\VST3\$productName.vst3"
if (Test-Path $installedPath) {
    $size = (Get-ChildItem $installedPath -Recurse -File | Measure-Object -Property Length -Sum).Sum
    $sizeMB = [math]::Round($size / 1MB, 2)
    Write-Host "VST3 installed: $installedPath ($sizeMB MB)"
} else {
    Write-Error "VST3 not found at expected location: $installedPath"
}
```

---

## Section 6: Error Scenarios

### iscc Not Found

**Symptom:** `iscc: The term 'iscc' is not recognized` or default path does not exist
**Cause:** Inno Setup not installed
**Fix:**
```powershell
# Install via winget
winget install JRSoftware.InnoSetup

# Or download manually from:
# https://jrsoftware.org/isdl.php

# After installation, iscc is typically at:
# C:\Program Files (x86)\Inno Setup 6\ISCC.exe
```

### Build Artifact Missing

**Symptom:** VST3 build artifact not found at expected path
**Cause:** Plugin has not been built, or build directory is stale
**Fix:**
```powershell
# Build the plugin (VST3 only, no install)
powershell -File scripts/build-and-install.ps1 $PluginName -NoInstall

# Verify artifact exists after build
$vst3Path = "build\plugins\$PluginName\${CMakeTarget}_artefacts\Release\VST3\$ProductName.vst3"
Test-Path $vst3Path
```

### Permission Denied

**Symptom:** Cannot write to dist/ directory or other file access errors
**Cause:** File locked by another process, or insufficient permissions
**Fix:**
- Close any file explorer windows or editors pointing to dist/
- Try running the terminal as Administrator
- Check if antivirus has quarantined files in dist/

### Invalid .iss Syntax

**Symptom:** iscc reports compilation errors
**Cause:** Incorrect placeholder replacement, bad paths, or template corruption
**Fix:**
- Open `plugins/[PluginName]/dist/installer.iss` in a text editor
- Look for unreplaced `{{PLACEHOLDER}}` values
- Verify all paths use backslashes and exist on disk
- Check that GUID format is correct: `{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}`

### Antivirus Blocking

**Symptom:** Generated EXE is flagged, quarantined, or deleted by antivirus software
**Cause:** Newly compiled EXE installers are commonly flagged by heuristic scanning (false positive)
**Fix:**
- Add the project's dist/ directory to your antivirus exclusion list
- Temporarily disable real-time scanning during compilation
- This is a known issue with Inno Setup generated executables and does not indicate malware
- If distributing to others, consider code signing the EXE (Authenticode certificate) to reduce false positives

---

## Future Enhancements

- **Authenticode code signing:** Sign the EXE with a code signing certificate for trusted publisher status
- **Custom installer icon:** Add branded .ico file to SetupIconFile directive
- **Custom wizard images:** Add branded WizardImageFile and WizardSmallImageFile
- **Silent install support:** `/VERYSILENT /SUPPRESSMSGBOXES` flags already work with Inno Setup
- **Batch installer creation:** Build installers for multiple plugins in one command
- **Multi-format packages:** ZIP archive alongside EXE installer
