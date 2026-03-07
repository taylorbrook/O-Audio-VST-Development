#!/usr/bin/env python3
"""SessionStart hook - Validate development environment with proactive dependency checks."""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


def get_version_output(cmd, args=None):
    """Run a command and return first line of output, or None on failure."""
    try:
        full_cmd = [cmd] + (args or ["--version"])
        result = subprocess.run(
            full_cmd, capture_output=True, text=True, timeout=10
        )
        output = result.stdout.strip() or result.stderr.strip()
        return output.splitlines()[0] if output else None
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return None


def main():
    errors = 0
    warnings = 0

    print("=== Plugin Freedom System - Environment Validation ===")
    print()

    # ---- CRITICAL: Python 3 (required for validators) ----
    python_cmd = "python3"
    if sys.platform == "win32":
        # On Windows, python3 may not exist; try python first
        python_cmd = shutil.which("python3") or shutil.which("python")
    else:
        python_cmd = shutil.which("python3")

    if not python_cmd:
        print("[ERROR] CRITICAL: python3 not found", file=sys.stderr)
        print("   Validators won't work - all plugin validation will fail", file=sys.stderr)
        if sys.platform == "darwin":
            print("   FIX: brew install python3", file=sys.stderr)
        elif sys.platform == "win32":
            print("   FIX: winget install Python.Python.3", file=sys.stderr)
        else:
            print("   FIX: sudo apt install python3", file=sys.stderr)
        errors += 1
    else:
        version = get_version_output(python_cmd)
        if version:
            print(f"[OK] {version}")
        else:
            print(f"[OK] python3 found at {python_cmd}")

    # ---- jq (required for bash hooks, NOT required for Python hooks) ----
    jq_path = shutil.which("jq")
    if sys.platform == "win32":
        if jq_path:
            version = get_version_output(jq_path)
            print(f"[OK] {version or 'jq available'}")
        else:
            print("[INFO] jq not found (not required - Python hooks handle JSON natively)")
    else:
        if not jq_path:
            print("[ERROR] CRITICAL: jq not found", file=sys.stderr)
            print("   Bash hooks will fail - state management broken", file=sys.stderr)
            if sys.platform == "darwin":
                print("   FIX: brew install jq", file=sys.stderr)
            else:
                print("   FIX: sudo apt install jq", file=sys.stderr)
            errors += 1
        else:
            version = get_version_output(jq_path)
            print(f"[OK] {version or 'jq available'}")

    # ---- CRITICAL: CMake (required for builds) ----
    cmake_path = shutil.which("cmake")
    if cmake_path:
        version_str = get_version_output(cmake_path)
        if version_str:
            print(f"[OK] {version_str}")
            # Check minimum version (3.15+)
            ver_match = re.search(r"(\d+)\.(\d+)", version_str)
            if ver_match:
                major = int(ver_match.group(1))
                minor = int(ver_match.group(2))
                if major < 3 or (major == 3 and minor < 15):
                    print(
                        f"[WARN] WARNING: CMake {major}.{minor} detected (JUCE 8 requires 3.15+)",
                        file=sys.stderr,
                    )
                    if sys.platform == "darwin":
                        print("   FIX: brew upgrade cmake", file=sys.stderr)
                    elif sys.platform == "win32":
                        print("   FIX: winget upgrade Kitware.CMake", file=sys.stderr)
                    else:
                        print("   FIX: sudo apt upgrade cmake", file=sys.stderr)
                    warnings += 1
        else:
            print("[OK] cmake found")
    else:
        print("[ERROR] CRITICAL: cmake not found", file=sys.stderr)
        print("   All plugin builds will fail", file=sys.stderr)
        if sys.platform == "darwin":
            print("   FIX: brew install cmake", file=sys.stderr)
        elif sys.platform == "win32":
            print("   FIX: winget install Kitware.CMake", file=sys.stderr)
        else:
            print("   FIX: sudo apt install cmake", file=sys.stderr)
        errors += 1

    # ---- CRITICAL: Build tools (platform-specific) ----
    if sys.platform == "darwin":
        # macOS: Xcode required
        xcodebuild_path = shutil.which("xcodebuild")
        if xcodebuild_path:
            version = get_version_output(xcodebuild_path, ["-version"])
            print(f"[OK] {version or 'Xcode available'}")
        else:
            print("[ERROR] CRITICAL: Xcode not found", file=sys.stderr)
            print("   macOS builds will fail", file=sys.stderr)
            print("   FIX: xcode-select --install", file=sys.stderr)
            errors += 1

        # Check code signing tools
        codesign_path = shutil.which("codesign")
        if not codesign_path:
            print("[ERROR] CRITICAL: codesign not found", file=sys.stderr)
            print(
                "   Plugins won't load in DAWs (signature validation required)",
                file=sys.stderr,
            )
            print("   FIX: Install Xcode Command Line Tools", file=sys.stderr)
            errors += 1
        else:
            print("[OK] codesign available")

    elif sys.platform == "win32":
        # Windows: Visual Studio required
        # Look for vswhere.exe
        program_files_x86 = os.environ.get(
            "ProgramFiles(x86)", r"C:\Program Files (x86)"
        )
        vswhere = (
            Path(program_files_x86)
            / "Microsoft Visual Studio"
            / "Installer"
            / "vswhere.exe"
        )
        if vswhere.is_file():
            try:
                result = subprocess.run(
                    [
                        str(vswhere),
                        "-latest",
                        "-property",
                        "installationVersion",
                    ],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                vs_version = result.stdout.strip()
                if vs_version:
                    print(f"[OK] Visual Studio {vs_version}")
                else:
                    print("[OK] Visual Studio found via vswhere")
            except (subprocess.TimeoutExpired, OSError):
                print("[OK] Visual Studio installer found")
        else:
            print("[ERROR] CRITICAL: Visual Studio not found", file=sys.stderr)
            print("   Windows builds will fail", file=sys.stderr)
            print(
                "   FIX: Install Visual Studio with C++ Desktop workload",
                file=sys.stderr,
            )
            errors += 1

    else:
        # Linux: gcc/g++ or clang required
        gpp_path = shutil.which("g++")
        clangpp_path = shutil.which("clang++")
        if gpp_path:
            version = get_version_output(gpp_path)
            print(f"[OK] {version or 'g++ available'}")
        elif clangpp_path:
            version = get_version_output(clangpp_path)
            print(f"[OK] {version or 'clang++ available'}")
        else:
            print(
                "[ERROR] CRITICAL: No C++ compiler found (g++ or clang++)",
                file=sys.stderr,
            )
            print("   Builds will fail", file=sys.stderr)
            print("   FIX: sudo apt install build-essential", file=sys.stderr)
            errors += 1

    # ---- CRITICAL: JUCE (required for plugin builds) ----
    juce_found = False
    home = Path.home()

    if sys.platform == "darwin":
        juce_paths = [
            Path("/Applications") / "JUCE",
            home / "JUCE",
            home / "Developer" / "JUCE",
        ]
    elif sys.platform == "win32":
        juce_paths = [Path("C:\\") / "JUCE"]
        juce_env = os.environ.get("JUCE_DIR")
        if juce_env:
            juce_paths.insert(0, Path(juce_env))
        # Also check home directory
        juce_paths.append(home / "JUCE")
    else:
        juce_paths = [home / "JUCE"]

    for juce_path in juce_paths:
        if juce_path.is_dir():
            print(f"[OK] JUCE found at {juce_path}")
            juce_found = True
            break

    if not juce_found:
        print("[ERROR] CRITICAL: JUCE not found at standard locations", file=sys.stderr)
        print("   All plugin builds will fail", file=sys.stderr)
        print(
            "   FIX: git clone https://github.com/juce-framework/JUCE.git ~/JUCE",
            file=sys.stderr,
        )
        print("        OR download from https://juce.com/", file=sys.stderr)
        errors += 1

    # ---- HIGH PRIORITY: Git (required for version control) ----
    git_path = shutil.which("git")
    if git_path:
        version = get_version_output(git_path)
        print(f"[OK] {version or 'git available'}")
    else:
        print("[WARN] WARNING: git not found", file=sys.stderr)
        print(
            "   Version control disabled - workflow state won't persist",
            file=sys.stderr,
        )
        if sys.platform == "darwin":
            print("   FIX: brew install git", file=sys.stderr)
        elif sys.platform == "win32":
            print("   FIX: winget install Git.Git", file=sys.stderr)
        else:
            print("   FIX: sudo apt install git", file=sys.stderr)
        warnings += 1

    # ---- MEDIUM PRIORITY: Ninja (optional but recommended) ----
    ninja_path = shutil.which("ninja")
    if ninja_path:
        version = get_version_output(ninja_path)
        print(f"[OK] ninja {version or ''} (fast builds enabled)")
    else:
        print("[INFO] ninja not found (builds will use default generator)", file=sys.stderr)
        if sys.platform == "darwin":
            print("   FIX (optional): brew install ninja", file=sys.stderr)
        elif sys.platform == "win32":
            print("   FIX (optional): winget install Ninja-build.Ninja", file=sys.stderr)
        else:
            print("   FIX (optional): sudo apt install ninja-build", file=sys.stderr)

    # ---- LOW PRIORITY: pluginval (optional for validation testing) ----
    pluginval_found = False
    pluginval_path = shutil.which("pluginval")
    if pluginval_path:
        print("[OK] pluginval available (validation testing enabled)")
        pluginval_found = True

    if not pluginval_found:
        if sys.platform == "darwin":
            mac_pluginval = (
                Path("/Applications")
                / "pluginval.app"
                / "Contents"
                / "MacOS"
                / "pluginval"
            )
            if mac_pluginval.is_file():
                print("[OK] pluginval found in /Applications")
                pluginval_found = True
            elif (home / "pluginval").is_file():
                print(f"[OK] pluginval found at {home / 'pluginval'}")
                pluginval_found = True
        elif sys.platform == "win32":
            # Check common Windows install paths
            win_paths = [
                home / "pluginval.exe",
                Path(os.environ.get("ProgramFiles", r"C:\Program Files"))
                / "pluginval"
                / "pluginval.exe",
                Path(
                    os.environ.get("LOCALAPPDATA", home / "AppData" / "Local")
                )
                / "pluginval"
                / "pluginval.exe",
            ]
            for p in win_paths:
                if p.is_file():
                    print(f"[OK] pluginval found at {p}")
                    pluginval_found = True
                    break
        else:
            if (home / "pluginval").is_file():
                print(f"[OK] pluginval found at {home / 'pluginval'}")
                pluginval_found = True

    if not pluginval_found:
        print("[INFO] pluginval not found (validation testing unavailable)", file=sys.stderr)
        print(
            "   FIX (optional): Download from https://github.com/Tracktion/pluginval/releases",
            file=sys.stderr,
        )

    # ---- Summary ----
    print()
    print("=== Validation Summary ===")

    if errors > 0:
        print(f"[ERROR] {errors} critical error(s) found")
        print(
            "   Workflow will fail - fix critical issues before running /plan or /implement"
        )
        print()
        print("Quick fix command:")
        if sys.platform == "darwin":
            print("  brew install python3 jq cmake && xcode-select --install")
        elif sys.platform == "win32":
            print("  winget install Python.Python.3 Kitware.CMake Ninja-build.Ninja")
            print("  NOTE: jq is no longer required (Python hooks handle JSON natively)")
        else:
            print("  sudo apt install python3 jq cmake build-essential")
        print()
        print(
            "Session will continue, but workflows will fail until dependencies are installed"
        )

    if warnings > 0:
        print(
            f"[WARN] {warnings} warning(s) - system usable but degraded functionality"
        )

    if errors == 0 and warnings == 0:
        print("[OK] All dependencies validated - system ready")

    print("===========================")
    print()

    # Never block session start (allow user to see errors and fix)
    sys.exit(0)


if __name__ == "__main__":
    main()
