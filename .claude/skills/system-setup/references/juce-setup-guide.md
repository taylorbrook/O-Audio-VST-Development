# JUCE Setup Guide

Detailed guide for installing and configuring JUCE 8.0+ for plugin development.

---

## What is JUCE?

JUCE (Jules' Utility Class Extensions) is a cross-platform C++ framework for building audio applications and plugins. It provides:

- Audio processing classes (DSP, synthesis, effects)
- Plugin hosting and creation (VST3, AU, AAX)
- GUI components (native and OpenGL)
- File I/O, networking, threading
- Build system integration (CMake, Projucer)

**Version requirement:** Plugin Freedom System requires JUCE 8.0.0 or later.

---

## Installation Methods

### Method 1: Direct Download (Recommended)

**Advantages:**
- Official releases, tested and stable
- No dependencies on package managers
- Easy to manage multiple versions

**Steps:**

1. **Download JUCE from GitHub releases:**

   Visit: https://github.com/juce-framework/JUCE/releases

   Download the appropriate package for your platform:
   - macOS: `juce-8.0.3-osx.zip`
   - Linux: `juce-8.0.3-linux.zip`
   - Windows: `juce-8.0.3-windows.zip`

2. **Extract to standard location:**

   **macOS/Linux:**
   ```bash
   cd ~
   unzip ~/Downloads/juce-8.0.3-*.zip
   mv juce-8.0.3-* JUCE
   ```

   **Windows:**
   - Extract zip to `C:\JUCE`
   - Or use your preferred location (e.g., `C:\Development\JUCE`)

3. **Verify installation:**

   Check that the modules directory exists:

   **macOS/Linux:**
   ```bash
   ls ~/JUCE/modules/juce_core/juce_core.h
   ```

   **Windows:**
   ```powershell
   dir C:\JUCE\modules\juce_core\juce_core.h
   ```

   If this file exists, JUCE is installed correctly.

### Method 2: Git Clone (For Development)

**Advantages:**
- Easy to update (`git pull`)
- Access to latest features and fixes
- Can switch between versions (`git checkout`)

**Disadvantages:**
- Requires Git
- Must manage updates manually
- May include unreleased/unstable code

**Steps:**

```bash
# Clone JUCE repository
cd ~
git clone --branch 8.0.3 https://github.com/juce-framework/JUCE.git

# Or clone latest develop branch (not recommended for production)
git clone https://github.com/juce-framework/JUCE.git
```

**Verify:**
```bash
ls ~/JUCE/modules/juce_core/juce_core.h
```

### Method 3: Package Manager (Linux Only)

**Ubuntu/Debian:**
```bash
# Warning: May not be latest version
sudo apt install juce-tools libjuce-dev
```

**Arch Linux:**
```bash
# AUR package (usually up-to-date)
yay -S juce
```

**Note:** Package manager versions may lag behind official releases. For JUCE 8.0+, direct download is recommended.

---

## Standard Installation Locations

The system-setup skill checks these locations in order:

1. **`~/JUCE`** (user home directory)
   - Recommended for single-user systems
   - No sudo required for updates
   - Most common location

2. **`/Applications/JUCE`** (macOS only)
   - System-wide installation
   - Requires sudo for updates
   - Alternative for shared systems

3. **`/usr/local/JUCE`** (macOS/Linux)
   - System-wide installation
   - Requires sudo for updates
   - Alternative for shared systems

4. **`C:\JUCE`** (Windows)
   - Recommended Windows location
   - Easy to remember and access
   - No special permissions needed

**Custom locations are supported** - the setup skill will ask for the path if JUCE isn't found in standard locations.

---

## Version Validation

JUCE version is determined by reading version defines from the header files:

**JUCE 8.x:** `modules/juce_core/system/juce_StandardHeader.h`
**JUCE 7.x:** `modules/juce_core/juce_core.h`

```cpp
#define JUCE_MAJOR_VERSION  8
#define JUCE_MINOR_VERSION  0
#define JUCE_BUILDNUMBER    9
```

The system-setup skill parses these files to ensure version ≥ 8.0.0.

**Manual version check:**

**macOS/Linux:**
```bash
grep "JUCE_MAJOR_VERSION" ~/JUCE/modules/juce_core/system/juce_StandardHeader.h
grep "JUCE_MINOR_VERSION" ~/JUCE/modules/juce_core/system/juce_StandardHeader.h
grep "JUCE_BUILDNUMBER" ~/JUCE/modules/juce_core/system/juce_StandardHeader.h
```

**Windows:**
```powershell
Select-String -Path "C:\JUCE\modules\juce_core\system\juce_StandardHeader.h" -Pattern "JUCE_MAJOR_VERSION"
```

**Expected output:**
```
#define JUCE_MAJOR_VERSION  8
```

If `JUCE_MAJOR_VERSION` is less than 8, you need to update JUCE.

---

## Directory Structure

A valid JUCE installation has this structure:

```
JUCE/
├── modules/              # JUCE framework modules
│   ├── juce_audio_basics/
│   ├── juce_audio_devices/
│   ├── juce_audio_formats/
│   ├── juce_audio_plugin_client/
│   ├── juce_audio_processors/
│   ├── juce_audio_utils/
│   ├── juce_core/
│   ├── juce_data_structures/
│   ├── juce_dsp/
│   ├── juce_events/
│   ├── juce_graphics/
│   ├── juce_gui_basics/
│   ├── juce_gui_extra/
│   └── ... (more modules)
├── extras/               # Example projects and tools
│   ├── AudioPluginHost/
│   ├── Projucer/
│   └── ...
├── examples/             # Example code
├── docs/                 # Documentation
└── CMakeLists.txt        # CMake integration
```

**Key files to verify:**
- `modules/juce_core/system/juce_StandardHeader.h` - Version defines (JUCE 8+)
- `modules/juce_core/juce_core.h` - Core module header
- `modules/juce_audio_processors/juce_audio_processors.h` - Plugin API
- `CMakeLists.txt` - CMake integration file

If any of these are missing, the installation is incomplete.

---

## CMake Integration

JUCE 8.x uses CMake as its primary build system. The Plugin Freedom System relies on this integration.

**Verify CMake can find JUCE:**

```bash
# Create test CMakeLists.txt
cat > /tmp/test-juce.cmake <<'EOF'
cmake_minimum_required(VERSION 3.15)
project(JUCETest)

# Point to JUCE modules
add_subdirectory(/path/to/JUCE/modules modules)

# Test that JUCE was found
if(TARGET juce::juce_core)
    message(STATUS "JUCE found successfully")
else()
    message(FATAL_ERROR "JUCE not found")
endif()
EOF

# Run CMake (replace /path/to/JUCE with actual path)
cmake -S /tmp -B /tmp/build-test -DJUCE_PATH=/Users/lex/JUCE
```

**Expected output:**
```
-- JUCE found successfully
```

If you see "JUCE not found", check:
1. Path is correct
2. `CMakeLists.txt` exists in JUCE root
3. `modules/` directory exists

---

## Updating JUCE

### From Direct Download

1. **Backup current installation (if customized):**
   ```bash
   mv ~/JUCE ~/JUCE-backup-$(date +%Y%m%d)
   ```

2. **Download new version** (see Method 1 above)

3. **Extract to JUCE location**

4. **Verify version:**
   ```bash
   grep "JUCE_MAJOR_VERSION" ~/JUCE/modules/juce_core/system/juce_StandardHeader.h
   ```

5. **Update system-config.json:**
   ```bash
   # Re-run setup to update config
   /setup
   ```

### From Git Clone

```bash
cd ~/JUCE

# Update to latest release
git fetch --tags
git checkout 8.0.3

# Or update to latest develop (not recommended for production)
git checkout develop
git pull
```

**After updating, rebuild all plugins** to ensure compatibility.

---

## Multiple JUCE Versions

You can maintain multiple JUCE versions for different projects:

```bash
# Install multiple versions
~/JUCE-8.0.3/
~/JUCE-8.0.2/
~/JUCE-7.0.5/  # For legacy projects

# Symlink to active version
ln -sf ~/JUCE-8.0.3 ~/JUCE
```

**Update system-config.json** to point to the active version:
```json
{
  "juce_path": "/Users/lex/JUCE-8.0.3",
  "juce_version": "8.0.3"
}
```

**To switch versions:**
```bash
rm ~/JUCE
ln -sf ~/JUCE-8.0.2 ~/JUCE
/setup  # Re-validate
```

---

## Projucer (Optional)

Projucer is JUCE's IDE for creating and managing projects. The Plugin Freedom System doesn't require it (we use CMake directly), but it's useful for:

- Browsing JUCE examples
- Experimenting with JUCE features
- Converting Projucer projects to CMake

**Build Projucer:**

**macOS/Linux:**
```bash
cd ~/JUCE/extras/Projucer/Builds/MacOSX  # or Linux/Makefile
xcodebuild -configuration Release  # macOS
# or: make CONFIG=Release  # Linux
```

**Windows:**
Open `JUCE/extras/Projucer/Builds/VisualStudio2022/Projucer.sln` in Visual Studio and build.

**Run Projucer:**
```bash
# macOS
open ~/JUCE/extras/Projucer/Builds/MacOSX/build/Release/Projucer.app

# Linux
~/JUCE/extras/Projucer/Builds/Linux/build/Projucer

# Windows
JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Release\Projucer.exe
```

---

## Troubleshooting

### "modules/juce_core/system/juce_StandardHeader.h not found"

**Cause:** Incomplete installation, wrong path, or very old JUCE version.

**Solution:**
1. Verify JUCE was extracted completely
2. Check that `modules/` directory exists
3. Re-download and extract JUCE if necessary

### "JUCE version 7.x detected, need 8.0+"

**Cause:** Old JUCE version installed.

**Solution:**
1. Download JUCE 8.0.3 or later
2. Replace old installation
3. Re-run `/setup` to validate

### "CMake can't find JUCE"

**Cause:** Path not configured correctly.

**Solution:**
1. Check `system-config.json` has correct `juce_path`
2. Verify path points to JUCE root (not modules/)
3. Re-run `/setup` to reconfigure

### "Permission denied" when updating JUCE

**Cause:** JUCE installed in system directory (e.g., `/usr/local/JUCE`).

**Solution:**
```bash
# Use sudo for system installations
sudo rm -rf /usr/local/JUCE
sudo unzip juce-8.0.3-*.zip -d /usr/local/
sudo mv /usr/local/juce-8.0.3-* /usr/local/JUCE

# Or reinstall to user directory (no sudo needed)
unzip juce-8.0.3-*.zip -d ~/
mv ~/juce-8.0.3-* ~/JUCE
/setup  # Update config
```

### "Build fails with JUCE API errors"

**Cause:** Plugin built against old JUCE version, now using new version.

**Solution:**
1. Clean build directory: `rm -rf build/`
2. Rebuild from scratch
3. Update code if JUCE API changed between versions

### macOS: "JUCE modules fail to compile with Apple Silicon"

**Cause:** Using x86_64 JUCE build on ARM Mac.

**Solution:**
1. Download universal JUCE build (or build from source)
2. Or use Rosetta: `arch -x86_64 /path/to/build/script`

---

## Platform-Specific Notes

### macOS

- **Xcode CLI Tools required** - JUCE uses Apple's compiler toolchain
- **Recommended location:** `~/JUCE`
- **Audio plugins require code signing** for distribution (not development)
- **VST3 plugins install to:** `~/Library/Audio/Plug-Ins/VST3/`
- **AU plugins install to:** `~/Library/Audio/Plug-Ins/Components/`

### Linux

- **Additional libraries required** - See `platform-requirements.md`
- **Recommended location:** `~/JUCE`
- **VST3 plugins install to:** `~/.vst3/`
- **LV2 plugins install to:** `~/.lv2/` (if JUCE compiled with LV2 support)

### Windows

- **Visual Studio required** - JUCE uses MSVC compiler
- **Recommended location:** `C:\JUCE`
- **VST3 plugins install to:** `C:\Program Files\Common Files\VST3\`
- **Build requires "Desktop development with C++" workload in Visual Studio

---

## Next Steps

After JUCE is installed and validated:

1. Run `/setup` to configure the system
2. System will detect JUCE and save path to `system-config.json`
3. Create your first plugin with `/dream`
4. Build system will automatically use configured JUCE path

---

## Additional Resources

- **JUCE Documentation:** https://docs.juce.com/
- **JUCE Forum:** https://forum.juce.com/
- **JUCE Tutorials:** https://juce.com/learn/tutorials
- **JUCE GitHub:** https://github.com/juce-framework/JUCE
- **JUCE Discord:** https://discord.com/invite/juce
