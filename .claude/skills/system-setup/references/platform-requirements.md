# Platform Requirements

Platform-specific requirements and installation guides for the Plugin Freedom System.

---

## macOS

### Minimum Requirements

- **OS Version:** macOS 10.13 High Sierra or later (11.0 Big Sur or later recommended)
- **Architecture:** Intel (x86_64) or Apple Silicon (arm64)
- **RAM:** 8 GB minimum, 16 GB recommended
- **Disk Space:** 10 GB free space for tools and builds

### Required Dependencies

#### 1. Python 3.8+

**Check if installed:**
```bash
python3 --version
```

**Installation via Homebrew (recommended):**
```bash
# Install Homebrew if not present
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Python 3
brew install python3

# Verify
python3 --version
```

**Direct download:**
1. Visit https://www.python.org/downloads/
2. Download latest Python 3 for macOS
3. Run the installer package
4. Verify installation: `python3 --version`

#### 2. Xcode Command Line Tools

**Check if installed:**
```bash
xcode-select -p
```

**Installation:**
```bash
xcode-select --install
```

This opens a GUI dialog. Click "Install" and follow the prompts.

**Verify:**
```bash
xcode-select -p
# Should output: /Library/Developer/CommandLineTools
```

**Troubleshooting:**
- If `xcode-select --install` fails, install full Xcode from App Store first
- After Xcode installation, run: `sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer`

#### 3. CMake 3.15+

**Check if installed:**
```bash
cmake --version
```

**Installation via Homebrew (recommended):**
```bash
brew install cmake

# Verify
cmake --version
```

**Direct download:**
1. Visit https://cmake.org/download/
2. Download latest CMake for macOS (.dmg)
3. Open DMG and drag CMake to Applications
4. Add to PATH:
   ```bash
   echo 'export PATH="/Applications/CMake.app/Contents/bin:$PATH"' >> ~/.zshrc
   source ~/.zshrc
   ```
5. Verify: `cmake --version`

#### 4. JUCE 8.0.0+

See `juce-setup-guide.md` for detailed installation instructions.

**Quick install:**
```bash
# Download to home directory
cd ~
curl -L -o JUCE.zip "https://github.com/juce-framework/JUCE/releases/download/8.0.3/juce-8.0.3-osx.zip"

# Extract
unzip JUCE.zip
mv juce-8.0.3-osx JUCE

# Cleanup
rm JUCE.zip

# Verify
ls ~/JUCE/modules/juce_core/juce_core.h
```

#### 5. pluginval

**Check if installed:**
```bash
pluginval --version
```

**Installation:**
```bash
# Download latest release
cd /tmp
curl -L -o pluginval.zip "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip"

# Extract
unzip pluginval.zip

# Install (requires sudo)
sudo mv pluginval.app/Contents/MacOS/pluginval /usr/local/bin/
sudo chmod +x /usr/local/bin/pluginval

# Cleanup
rm -rf pluginval.zip pluginval.app

# Verify
pluginval --version
```

**Alternative install location (no sudo):**
```bash
# Install to ~/bin instead
mkdir -p ~/bin
mv pluginval.app/Contents/MacOS/pluginval ~/bin/
chmod +x ~/bin/pluginval

# Add to PATH
echo 'export PATH="$HOME/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Homebrew Installation (All-in-One)

If you prefer to install all dependencies via Homebrew:

```bash
# Install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build dependencies
brew install python3 cmake

# Install Xcode CLI Tools separately (requires GUI)
xcode-select --install
```

JUCE and pluginval must be installed manually (see above).

---

## Linux (Ubuntu/Debian)

### Minimum Requirements

- **Distribution:** Ubuntu 20.04 LTS or later, Debian 11 or later
- **Architecture:** x86_64 or ARM64
- **RAM:** 8 GB minimum, 16 GB recommended
- **Disk Space:** 10 GB free space

### Required Dependencies

#### 1. Python 3.8+

Most modern Linux distributions include Python 3.

**Check:**
```bash
python3 --version
```

**Install if missing:**
```bash
sudo apt update
sudo apt install python3 python3-pip
```

#### 2. Build Tools

**Install GCC/G++ and build essentials:**
```bash
sudo apt update
sudo apt install build-essential
```

**Verify:**
```bash
gcc --version
g++ --version
```

#### 3. CMake 3.15+

**Check:**
```bash
cmake --version
```

**Install:**
```bash
sudo apt update
sudo apt install cmake
```

**If version too old, use Kitware's repository:**
```bash
# Add Kitware repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null

sudo apt update
sudo apt install cmake
```

#### 4. Additional Libraries for JUCE

JUCE requires several system libraries for audio, graphics, and windowing:

```bash
sudo apt install \
  libasound2-dev \
  libcurl4-openssl-dev \
  libfreetype6-dev \
  libgl1-mesa-dev \
  libjack-jackd2-dev \
  libx11-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxext-dev \
  libxinerama-dev \
  libxrandr-dev \
  libxrender-dev \
  webkit2gtk-4.0
```

#### 5. JUCE 8.0.0+

See `juce-setup-guide.md` for detailed instructions.

**Quick install:**
```bash
cd ~
curl -L -o JUCE.zip "https://github.com/juce-framework/JUCE/releases/download/8.0.3/juce-8.0.3-linux.zip"
unzip JUCE.zip
mv juce-8.0.3-linux JUCE
rm JUCE.zip
```

#### 6. pluginval

```bash
cd /tmp
curl -L -o pluginval.zip "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Linux.zip"
unzip pluginval.zip
sudo mv pluginval /usr/local/bin/
sudo chmod +x /usr/local/bin/pluginval
rm pluginval.zip
```

---

## Windows

### Minimum Requirements

- **OS Version:** Windows 10 version 1809 or later (Windows 11 recommended)
- **Architecture:** x64 (64-bit)
- **RAM:** 8 GB minimum, 16 GB recommended
- **Disk Space:** 20 GB free space (Visual Studio is large)

### Required Dependencies

#### 1. Python 3.8+

**Check if installed:**
```powershell
python --version
```

**Installation:**
1. Download from https://www.python.org/downloads/windows/
2. Run installer
3. **IMPORTANT:** Check "Add Python to PATH" during installation
4. Verify: Open new terminal and run `python --version`

**Via Microsoft Store (alternative):**
1. Open Microsoft Store
2. Search for "Python 3.11"
3. Click "Get" to install

#### 2. Visual Studio 2019 or later

**Required for C++ compilation.**

**Installation:**
1. Download Visual Studio Community (free) from https://visualstudio.microsoft.com/downloads/
2. Run installer
3. Select "Desktop development with C++" workload
4. Install (requires ~10 GB disk space)

**Minimum components needed:**
- MSVC v142 or later C++ build tools
- Windows 10 SDK (latest)
- C++ CMake tools for Windows

**Verify:**
Open "Developer Command Prompt for VS 2019" and run:
```cmd
cl
```
Should show Microsoft C/C++ compiler version.

#### 3. CMake 3.15+

**Check if installed:**
```powershell
cmake --version
```

**Installation:**
1. Download from https://cmake.org/download/
2. Download Windows installer (.msi)
3. Run installer
4. **IMPORTANT:** Select "Add CMake to system PATH" during installation
5. Verify: Open new terminal and run `cmake --version`

**Alternative (via Visual Studio):**
CMake is included with Visual Studio if you selected "C++ CMake tools" during installation.

#### 4. JUCE 8.0.0+

See `juce-setup-guide.md` for detailed instructions.

**Quick install:**
1. Download from https://github.com/juce-framework/JUCE/releases/download/8.0.3/juce-8.0.3-windows.zip
2. Extract to `C:\JUCE` (or your preferred location)
3. Verify: Check that `C:\JUCE\modules\juce_core\juce_core.h` exists

#### 5. pluginval

**Installation:**
1. Download from https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip
2. Extract to a permanent location (e.g., `C:\Program Files\pluginval\`)
3. Add to PATH:
   - Right-click "This PC" → Properties → Advanced system settings
   - Click "Environment Variables"
   - Under "System variables", select "Path" and click "Edit"
   - Click "New" and add `C:\Program Files\pluginval\`
   - Click OK to save
4. Verify: Open new terminal and run `pluginval --version`

### PowerShell Script (Automated)

**Note:** Requires administrator privileges.

```powershell
# Install Chocolatey (package manager for Windows)
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Install dependencies
choco install python3 cmake -y

# Visual Studio must be installed manually (too large for automated install)
Write-Host "Please install Visual Studio 2019+ manually from https://visualstudio.microsoft.com/downloads/"
```

---

## Common Issues

### macOS: "xcode-select: error: tool 'xcodebuild' requires Xcode"

**Solution:** Install full Xcode from App Store, then run:
```bash
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
```

### macOS: "Operation not permitted" when installing to /usr/local/bin

**Solution:** Grant Full Disk Access to Terminal:
1. System Preferences → Security & Privacy → Privacy
2. Select "Full Disk Access"
3. Add Terminal (or your terminal app)
4. Restart terminal and retry

**Alternative:** Install to `~/bin` instead (see pluginval installation above).

### Linux: "webkit2gtk-4.0 not found"

**Solution:** Install WebKit library:
```bash
sudo apt install webkit2gtk-4.0
```

### Windows: "cmake not recognized"

**Solution:** CMake not in PATH. Either:
1. Reinstall CMake and select "Add to PATH" option
2. Manually add CMake bin directory to PATH:
   - Default location: `C:\Program Files\CMake\bin`

### All platforms: "JUCE version too old"

**Solution:** Update JUCE to 8.0.0 or later:
```bash
# Backup old JUCE (if customized)
mv ~/JUCE ~/JUCE-backup

# Download latest JUCE 8.x
# See platform-specific instructions above
```

### All platforms: Build fails with "could not find JUCE"

**Solution:** Update system-config.json with correct JUCE path:
```bash
# Re-run setup to reconfigure
/setup
```

---

## Version Requirements Summary

| Dependency       | Minimum Version | Recommended Version | Platform      |
|------------------|----------------|---------------------|---------------|
| Python           | 3.8            | 3.11+               | All           |
| CMake            | 3.15           | 3.27+               | All           |
| JUCE             | 8.0.0          | 8.0.3 (latest)      | All           |
| Xcode CLI Tools  | 13.0           | 15.0+               | macOS only    |
| Visual Studio    | 2019 (16.0)    | 2022 (17.0+)        | Windows only  |
| GCC/Clang        | 9.0            | 11.0+               | Linux only    |
| pluginval        | 1.0.0          | Latest              | All           |

---

## Next Steps

After all dependencies are installed:

1. Run `/setup` to validate and configure the system
2. Review system report to confirm all dependencies are ready
3. Create your first plugin with `/dream`
