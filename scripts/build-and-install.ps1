#Requires -Version 5.1
<#
.SYNOPSIS
    JUCE Plugin Build & Installation Script (Windows)

.DESCRIPTION
    7-Phase Pipeline:
      1. Pre-flight Validation
      2. Build (VST3 only on Windows - no AU)
      3. Extract PRODUCT_NAME
      4. Remove Old Versions
      5. Install New Versions
      6. Clear DAW Caches
      7. Verification

.PARAMETER PluginName
    Name of plugin directory in plugins/

.PARAMETER DryRun
    Show commands without executing

.PARAMETER NoInstall
    Build only, skip installation

.PARAMETER Verbose
    Show detailed output

.PARAMETER Reconfigure
    Force CMake reconfiguration (delete build/ first)

.EXAMPLE
    .\build-and-install.ps1 O-Chorus
    .\build-and-install.ps1 O-Chorus -DryRun
    .\build-and-install.ps1 O-Chorus -NoInstall
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$PluginName,

    [switch]$DryRun,
    [switch]$NoInstall,
    [switch]$Reconfigure
)

$ErrorActionPreference = 'Stop'

# ============================================================================
# Color output functions
# ============================================================================
function Write-Success { param([string]$Message) Write-Host "[OK] $Message" -ForegroundColor Green }
function Write-Warning2 { param([string]$Message) Write-Host "[WARN] $Message" -ForegroundColor Yellow }
function Write-Error2 { param([string]$Message) Write-Host "[FAIL] $Message" -ForegroundColor Red }
function Write-Info { param([string]$Message) Write-Host "-> $Message" -ForegroundColor Cyan }

# ============================================================================
# Global variables
# ============================================================================
$script:ProductName = ''
$script:LogFile = ''
$script:StartTime = Get-Date

# ============================================================================
# Setup logging
# ============================================================================
function Initialize-Logging {
    $logDir = Join-Path 'logs' $PluginName
    if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir -Force | Out-Null }
    $timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
    $script:LogFile = Join-Path $logDir "build_$timestamp.log"

    Write-Info "Log file: $script:LogFile"
    "Build started at $(Get-Date)" | Out-File $script:LogFile
    "Plugin: $PluginName" | Out-File $script:LogFile -Append
    "Flags: DryRun=$DryRun NoInstall=$NoInstall Verbose=$($VerbosePreference -eq 'Continue') Reconfigure=$Reconfigure" | Out-File $script:LogFile -Append
    '---' | Out-File $script:LogFile -Append
}

# ============================================================================
# Execute command (respects dry-run flag)
# ============================================================================
function Invoke-BuildCommand {
    param(
        [string]$Command,
        [string[]]$Arguments
    )
    $cmdLine = "$Command $($Arguments -join ' ')"

    if ($VerbosePreference -eq 'Continue') { Write-Info "Execute: $cmdLine" }
    "`$ $cmdLine" | Out-File $script:LogFile -Append

    if ($DryRun) {
        Write-Host "[DRY-RUN] $cmdLine"
        return 0
    }

    if ($VerbosePreference -eq 'Continue') {
        & $Command @Arguments 2>&1 | Tee-Object -FilePath $script:LogFile -Append
    } else {
        & $Command @Arguments 2>&1 | Out-File $script:LogFile -Append
    }
    return $LASTEXITCODE
}

# ============================================================================
# Phase 1: Pre-flight Validation
# ============================================================================
function Invoke-Phase1 {
    Write-Host ''
    Write-Info 'Phase 1: Pre-flight Validation'
    'Phase 1: Pre-flight Validation' | Out-File $script:LogFile -Append

    # Check plugin directory exists
    Write-Info '  - Checking plugin directory...'
    $pluginDir = Join-Path 'plugins' $PluginName
    if (-not (Test-Path $pluginDir -PathType Container)) {
        Write-Error2 "Plugin directory not found: $pluginDir"
        'ERROR: Plugin directory not found' | Out-File $script:LogFile -Append
        exit 1
    }

    # Check CMakeLists.txt exists
    Write-Info '  - Checking CMakeLists.txt...'
    $cmakeFile = Join-Path $pluginDir 'CMakeLists.txt'
    if (-not (Test-Path $cmakeFile)) {
        Write-Error2 "CMakeLists.txt not found in $pluginDir/"
        'ERROR: CMakeLists.txt not found' | Out-File $script:LogFile -Append
        exit 1
    }

    # Check PRODUCT_NAME in CMakeLists.txt
    Write-Info '  - Checking PRODUCT_NAME...'
    if (-not (Select-String -Path $cmakeFile -Pattern 'PRODUCT_NAME' -Quiet)) {
        Write-Warning2 'PRODUCT_NAME not found in CMakeLists.txt (will use directory name as fallback)'
        'WARNING: PRODUCT_NAME not found' | Out-File $script:LogFile -Append
    }

    # Check CMake available
    Write-Info '  - Checking CMake...'
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Error2 'CMake not found. Install from https://cmake.org/download/ or: winget install Kitware.CMake'
        'ERROR: CMake not found' | Out-File $script:LogFile -Append
        exit 1
    }

    # Check Ninja available
    Write-Info '  - Checking Ninja...'
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        Write-Error2 'Ninja not found. Install with: winget install Ninja-build.Ninja'
        'ERROR: Ninja not found' | Out-File $script:LogFile -Append
        exit 1
    }

    # Check Visual Studio 2022
    Write-Info '  - Checking Visual Studio...'
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -property installationPath 2>$null
        if ($vsPath) {
            Write-Info "  - Visual Studio found at: $vsPath"
            "Visual Studio: $vsPath" | Out-File $script:LogFile -Append
        } else {
            Write-Warning2 'Visual Studio installation found but no valid version detected'
        }
    } else {
        Write-Warning2 'vswhere not found - Visual Studio may not be installed'
        Write-Warning2 'Install Visual Studio 2022 with C++ desktop workload'
    }

    # Check JUCE
    if ($env:JUCE_DIR -and (Test-Path $env:JUCE_DIR)) {
        Write-Info "  - Using JUCE_DIR: $env:JUCE_DIR"
        "JUCE_DIR: $env:JUCE_DIR" | Out-File $script:LogFile -Append
    } elseif (Test-Path 'C:\JUCE') {
        Write-Info '  - JUCE found at C:\JUCE'
    } else {
        Write-Warning2 'JUCE not found at C:\JUCE or $env:JUCE_DIR'
    }

    Write-Success 'Pre-flight validation passed'
    'Pre-flight validation: PASS' | Out-File $script:LogFile -Append
}

# ============================================================================
# Phase 2: Build (VST3 only on Windows)
# ============================================================================
function Invoke-Phase2 {
    Write-Host ''
    Write-Info 'Phase 2: Build'
    'Phase 2: Build' | Out-File $script:LogFile -Append

    $buildDir = 'build'

    # Handle --reconfigure flag
    if ($Reconfigure -and (Test-Path $buildDir)) {
        Write-Info '  - Removing build directory for reconfiguration...'
        if ($DryRun) {
            Write-Host "[DRY-RUN] Remove-Item -Recurse -Force `"$buildDir`""
        } else {
            Remove-Item -Recurse -Force $buildDir
            'Removed build directory for reconfiguration' | Out-File $script:LogFile -Append
        }
    }

    # One-time configure at root (only if build/ doesn't exist)
    if (-not (Test-Path $buildDir)) {
        Write-Info '  - Configuring CMake at root with Ninja generator...'
        $exitCode = Invoke-BuildCommand 'cmake' @('-B', $buildDir, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release')
        if ($exitCode -ne 0) {
            Write-Error2 'CMake configuration failed'
            'ERROR: CMake configuration failed' | Out-File $script:LogFile -Append
            exit 1
        }
    } else {
        Write-Info '  - Build directory exists, skipping configure (use -Reconfigure to force)'
    }

    # Build specific plugin (VST3 only on Windows - no AU support)
    Write-Info "  - Building ${PluginName} (VST3 only)..."
    $exitCode = Invoke-BuildCommand 'cmake' @('--build', $buildDir, '--config', 'Release', '--target', "${PluginName}_VST3", '--parallel')
    if ($exitCode -ne 0) {
        Write-Error2 'Build failed'
        'ERROR: Build failed' | Out-File $script:LogFile -Append
        exit 1
    }

    Write-Success 'Build complete'
    'Build: SUCCESS' | Out-File $script:LogFile -Append
}

# ============================================================================
# Phase 3: Extract PRODUCT_NAME
# ============================================================================
function Invoke-Phase3 {
    Write-Host ''
    Write-Info 'Phase 3: Extract PRODUCT_NAME'
    'Phase 3: Extract PRODUCT_NAME' | Out-File $script:LogFile -Append

    Write-Info '  - Parsing CMakeLists.txt...'
    $cmakeFile = Join-Path 'plugins' $PluginName 'CMakeLists.txt'
    $content = Get-Content $cmakeFile -Raw
    if ($content -match 'PRODUCT_NAME\s+"([^"]+)"') {
        $script:ProductName = $Matches[1]
        Write-Info "  - Product name: $($script:ProductName)"
        "PRODUCT_NAME: $($script:ProductName)" | Out-File $script:LogFile -Append
    } else {
        Write-Warning2 'Could not extract PRODUCT_NAME, using directory name as fallback'
        $script:ProductName = $PluginName
        "WARNING: Using fallback PRODUCT_NAME=$($script:ProductName)" | Out-File $script:LogFile -Append
    }

    Write-Success "Product name extracted: $($script:ProductName)"
}

# ============================================================================
# Phase 4: Remove Old Versions
# ============================================================================
function Invoke-Phase4 {
    Write-Host ''
    Write-Info 'Phase 4: Remove Old Versions'
    'Phase 4: Remove Old Versions' | Out-File $script:LogFile -Append

    $vst3Dir = Join-Path $env:COMMONPROGRAMFILES 'VST3'
    $vst3Path = Join-Path $vst3Dir "$($script:ProductName).vst3"

    # Search for existing VST3
    Write-Info '  - Searching for old VST3...'
    if (Test-Path $vst3Path) {
        if ($DryRun) {
            Write-Host "[DRY-RUN] Would remove: $vst3Path"
        } else {
            Write-Info "  - Removing old VST3: $vst3Path"
            Remove-Item -Recurse -Force $vst3Path
            "Removed old VST3: $vst3Path" | Out-File $script:LogFile -Append
        }
    } else {
        Write-Info '  - No old VST3 found'
        'No old VST3 found' | Out-File $script:LogFile -Append
    }

    Write-Success 'Old versions removed'
}

# ============================================================================
# Phase 5: Install New Versions
# ============================================================================
function Invoke-Phase5 {
    Write-Host ''
    Write-Info 'Phase 5: Install New Versions'
    'Phase 5: Install New Versions' | Out-File $script:LogFile -Append

    $vst3Dir = Join-Path $env:COMMONPROGRAMFILES 'VST3'
    $vst3Build = Join-Path 'build' 'plugins' $PluginName "${PluginName}_artefacts" 'Release' 'VST3' "$($script:ProductName).vst3"

    # Ensure target directory exists
    if (-not (Test-Path $vst3Dir)) {
        New-Item -ItemType Directory -Path $vst3Dir -Force | Out-Null
    }

    # Verify VST3 artifact exists (skip in dry-run)
    Write-Info '  - Locating VST3 build artifact...'
    if (-not $DryRun -and -not (Test-Path $vst3Build)) {
        Write-Error2 "VST3 build artifact not found: $vst3Build"
        'ERROR: VST3 artifact not found' | Out-File $script:LogFile -Append
        exit 1
    }

    # Install VST3
    $destPath = Join-Path $vst3Dir "$($script:ProductName).vst3"
    Write-Info "  - Installing VST3 to $vst3Dir/"
    if ($DryRun) {
        Write-Host "[DRY-RUN] Copy-Item -Recurse `"$vst3Build`" `"$destPath`""
    } else {
        try {
            Copy-Item -Recurse -Force $vst3Build $destPath
            "Installed VST3: $destPath" | Out-File $script:LogFile -Append
        } catch {
            if ($_.Exception.Message -match 'Access.*denied|UnauthorizedAccess') {
                Write-Error2 "Permission denied writing to $vst3Dir"
                Write-Warning2 'Try running this script as Administrator, or install to a user-local path.'
                Write-Warning2 "Alternative: Copy manually from $vst3Build"
                'ERROR: Permission denied for VST3 installation' | Out-File $script:LogFile -Append
                exit 1
            }
            throw
        }
    }

    Write-Success 'New versions installed (VST3)'
}

# ============================================================================
# Phase 6: Clear DAW Caches
# ============================================================================
function Invoke-Phase6 {
    Write-Host ''
    Write-Info 'Phase 6: Clear DAW Caches'
    'Phase 6: Clear DAW Caches' | Out-File $script:LogFile -Append

    # Clear Ableton plugin database (Windows location)
    $abletonPrefs = Join-Path $env:APPDATA 'Ableton'
    Write-Info '  - Clearing Ableton plugin database...'
    if ($DryRun) {
        Write-Host "[DRY-RUN] Remove-Item `"$abletonPrefs\*\PluginScanDb.txt`""
    } else {
        if (Test-Path $abletonPrefs) {
            Get-ChildItem -Path $abletonPrefs -Recurse -Filter 'PluginScanDb.txt' -ErrorAction SilentlyContinue |
                Remove-Item -Force -ErrorAction SilentlyContinue
            'Cleared Ableton plugin database' | Out-File $script:LogFile -Append
        } else {
            Write-Info '  - Ableton preferences directory not found (skipping)'
            'Ableton preferences not found' | Out-File $script:LogFile -Append
        }
    }

    # Clear FL Studio plugin database (if present)
    $flStudioPrefs = Join-Path $env:APPDATA 'Image-Line'
    if (Test-Path $flStudioPrefs) {
        Write-Info '  - Clearing FL Studio plugin database...'
        if ($DryRun) {
            Write-Host "[DRY-RUN] Would clear FL Studio plugin cache"
        } else {
            Get-ChildItem -Path $flStudioPrefs -Recurse -Filter 'plugin database*' -ErrorAction SilentlyContinue |
                Remove-Item -Force -ErrorAction SilentlyContinue
            'Cleared FL Studio plugin database' | Out-File $script:LogFile -Append
        }
    }

    # Note: No AU cache or AudioComponentRegistrar on Windows
    Write-Info '  - Audio Unit cache: N/A (Windows)'

    Write-Success 'DAW caches cleared'
}

# ============================================================================
# Phase 7: Verification
# ============================================================================
function Invoke-Phase7 {
    Write-Host ''
    Write-Info 'Phase 7: Verification'
    'Phase 7: Verification' | Out-File $script:LogFile -Append

    $vst3Dir = Join-Path $env:COMMONPROGRAMFILES 'VST3'
    $vst3Path = Join-Path $vst3Dir "$($script:ProductName).vst3"

    # Check VST3 exists (skip in dry-run)
    Write-Info '  - Checking VST3 exists...'
    if (-not $DryRun -and -not (Test-Path $vst3Path)) {
        Write-Error2 "VST3 not found at: $vst3Path"
        'ERROR: VST3 verification failed' | Out-File $script:LogFile -Append
        exit 1
    }

    if ($DryRun) {
        Write-Info '  - Would check timestamps...'
        Write-Info '  - Would check file sizes...'
        Write-Host ''
        Write-Success 'Verification complete (dry-run)'
        Write-Host ''
        Write-Info 'Would verify:'
        Write-Host "  VST3: $vst3Path"
    } else {
        # Check timestamps (modified within last 60 seconds)
        Write-Info '  - Checking timestamps...'
        $now = Get-Date
        $vst3Item = Get-Item $vst3Path
        $vst3Age = [int]($now - $vst3Item.LastWriteTime).TotalSeconds

        if ($vst3Age -gt 60) {
            Write-Warning2 "VST3 timestamp is older than 60 seconds (${vst3Age}s) - may not be fresh build"
            "WARNING: VST3 timestamp: ${vst3Age}s old" | Out-File $script:LogFile -Append
        }

        # Get file sizes
        Write-Info '  - Checking file sizes...'
        $vst3Size = (Get-ChildItem $vst3Path -Recurse -File | Measure-Object -Property Length -Sum).Sum
        $vst3SizeMB = [math]::Round($vst3Size / 1MB, 2)

        Write-Host ''
        Write-Success 'Verification complete'
        Write-Host ''
        Write-Info 'Installed plugins:'
        Write-Host "  VST3: $vst3Path"
        Write-Host "        Size: ${vst3SizeMB} MB, Age: ${vst3Age}s"

        '' | Out-File $script:LogFile -Append
        'Verification: PASS' | Out-File $script:LogFile -Append
        "VST3: $vst3Path (${vst3SizeMB} MB, ${vst3Age}s old)" | Out-File $script:LogFile -Append
    }
}

# ============================================================================
# Main execution
# ============================================================================
Initialize-Logging

Write-Host '============================================================================'
Write-Host "JUCE Plugin Build & Installation: $PluginName"
Write-Host '============================================================================'

# Phase 1: Pre-flight Validation
Invoke-Phase1

# Phase 2: Build
Invoke-Phase2

# If -NoInstall, exit early
if ($NoInstall) {
    Write-Host ''
    Write-Success 'Build complete (-NoInstall flag set, skipping installation)'

    $duration = [int]((Get-Date) - $script:StartTime).TotalSeconds
    Write-Info "Build time: ${duration}s"
    Write-Info "Log: $script:LogFile"

    '' | Out-File $script:LogFile -Append
    "Build completed at $(Get-Date)" | Out-File $script:LogFile -Append
    "Duration: ${duration}s" | Out-File $script:LogFile -Append
    exit 0
}

# Phase 3: Extract PRODUCT_NAME
Invoke-Phase3

# Phase 4: Remove Old Versions
Invoke-Phase4

# Phase 5: Install New Versions
Invoke-Phase5

# Phase 6: Clear DAW Caches
Invoke-Phase6

# Phase 7: Verification
Invoke-Phase7

# Final success message
Write-Host ''
Write-Host '============================================================================'
Write-Success 'Build and installation complete!'
Write-Host '============================================================================'

$duration = [int]((Get-Date) - $script:StartTime).TotalSeconds
Write-Info "Total time: ${duration}s"
Write-Info "Log: $script:LogFile"

'' | Out-File $script:LogFile -Append
"Build completed at $(Get-Date)" | Out-File $script:LogFile -Append
"Duration: ${duration}s" | Out-File $script:LogFile -Append
