#!/usr/bin/env python3
"""Canary test script for Plugin Freedom System.

Tests O-SimpleReverb (standard plugin) and O-AnalogEQ (WebView plugin).
Run after any system-level changes to verify no breakage.

Platform-aware: builds VST3+AU on macOS, VST3 only on Windows.
Skips auval and AudioComponentRegistrar on Windows.
"""

import subprocess
import sys
from pathlib import Path

PROJ_DIR = Path(__file__).resolve().parent.parent.parent
BUILD_DIR = PROJ_DIR / "build"

IS_MACOS = sys.platform == "darwin"
IS_WINDOWS = sys.platform == "win32"


def run(cmd, **kwargs):
    """Run a command and return (success, output)."""
    result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    return result.returncode == 0, result.stdout + result.stderr


def clear_caches_macos():
    """Clear AU cache and kill AudioComponentRegistrar on macOS."""
    subprocess.run(["killall", "-9", "AudioComponentRegistrar"],
                   capture_output=True)
    home = Path.home()
    au_cache = home / "Library" / "Caches" / "AudioUnitCache"
    au_cache2 = home / "Library" / "Caches" / "com.apple.audiounits.cache"
    if au_cache.exists():
        subprocess.run(["rm", "-rf", str(au_cache)])
    if au_cache2.exists():
        subprocess.run(["rm", "-rf", str(au_cache2)])


def install_macos(plugin_cmake_name, product_name):
    """Install VST3 + AU on macOS."""
    home = Path.home()
    vst3_dir = home / "Library" / "Audio" / "Plug-Ins" / "VST3"
    au_dir = home / "Library" / "Audio" / "Plug-Ins" / "Components"
    artefacts = BUILD_DIR / "plugins" / plugin_cmake_name / f"{plugin_cmake_name}_artefacts" / "Release"
    vst3_src = artefacts / "VST3" / f"{product_name}.vst3"
    au_src = artefacts / "AU" / f"{product_name}.component"

    # Remove old
    vst3_dest = vst3_dir / f"{product_name}.vst3"
    au_dest = au_dir / f"{product_name}.component"
    if vst3_dest.exists():
        subprocess.run(["rm", "-rf", str(vst3_dest)])
    if au_dest.exists():
        subprocess.run(["rm", "-rf", str(au_dest)])

    # Copy new
    subprocess.run(["cp", "-R", str(vst3_src), str(vst3_dir) + "/"])
    subprocess.run(["cp", "-R", str(au_src), str(au_dir) + "/"])


def install_windows(plugin_cmake_name, product_name):
    """Install VST3 on Windows."""
    import shutil
    import os
    vst3_dir = Path(os.environ.get("COMMONPROGRAMFILES", r"C:\Program Files\Common Files")) / "VST3"
    artefacts = BUILD_DIR / "plugins" / plugin_cmake_name / f"{plugin_cmake_name}_artefacts" / "Release"
    vst3_src = artefacts / "VST3" / f"{product_name}.vst3"

    vst3_dest = vst3_dir / f"{product_name}.vst3"
    if vst3_dest.exists():
        shutil.rmtree(vst3_dest)
    shutil.copytree(vst3_src, vst3_dest)


def run_canary(name, cmake_name, product_name, auval_type=None, auval_code=None, auval_mfr=None):
    """Run a single canary test. Returns True on pass."""
    print(f"--- Canary: {name} ---")

    # Build targets
    if IS_MACOS:
        targets = [f"{cmake_name}_VST3", f"{cmake_name}_AU"]
    else:
        targets = [f"{cmake_name}_VST3"]

    print(f"Building {' + '.join(targets)}...")
    build_cmd = ["cmake", "--build", str(BUILD_DIR), "--config", "Release", "--parallel"]
    for t in targets:
        build_cmd.extend(["--target", t])

    success, output = run(build_cmd)
    if not success:
        print(f"Build: FAILED")
        print(output[-500:] if len(output) > 500 else output)
        return False
    print("Build: OK")

    # Clear caches + install
    if IS_MACOS:
        print("Clearing AU cache...")
        clear_caches_macos()
        print("Installing...")
        install_macos(cmake_name, product_name)
    elif IS_WINDOWS:
        print("Installing VST3...")
        try:
            install_windows(cmake_name, product_name)
        except PermissionError:
            print("WARN: Permission denied installing to CommonProgramFiles\\VST3")
            print("      Run as Administrator or install manually.")
            return False

    # AU validation (macOS only)
    if IS_MACOS and auval_type and auval_code and auval_mfr:
        print(f"Validating AU ({auval_type} {auval_code} {auval_mfr})...")
        success, output = run(["auval", "-v", auval_type, auval_code, auval_mfr])
        if "* * * PASS" in output:
            print(f"PASS: {name} AU validation passed")
            return True
        else:
            print(f"WARN: auval did not explicitly PASS -- check manually")
            return False
    else:
        if IS_WINDOWS:
            print("AU validation: skipped (Windows)")
        print(f"PASS: {name} build and install succeeded")
        return True


def main():
    print("=== PFS Canary Test ===")
    print(f"Platform: {'macOS' if IS_MACOS else 'Windows' if IS_WINDOWS else sys.platform}")
    print()

    # Ensure build directory exists and has been configured
    build_ninja = BUILD_DIR / "build.ninja"
    cmake_cache = BUILD_DIR / "CMakeCache.txt"
    if not build_ninja.exists() and not cmake_cache.exists():
        print("ERROR: Build not configured. Run cmake first.")
        sys.exit(1)

    pass_count = 0
    fail_count = 0

    # Canary 1: O-SimpleReverb (standard plugin)
    if run_canary("O-SimpleReverb", "O-SimpleReverb", "O-SimpleReverb",
                  auval_type="aufx", auval_code="OuSr", auval_mfr="OuDv"):
        pass_count += 1
    else:
        fail_count += 1

    print()

    # Canary 2: O-AnalogEQ (WebView plugin)
    if run_canary("O-AnalogEQ", "OuariconAnalogEQ", "O-AnalogEQ",
                  auval_type="aufx", auval_code="OuAE", auval_mfr="OuDv"):
        pass_count += 1
    else:
        fail_count += 1

    print()
    print("=== Canary Test Complete ===")
    print(f"Passed: {pass_count} / 2")
    if fail_count > 0:
        print(f"Failures: {fail_count}")
        sys.exit(1)

    print("All canary tests passed.")


if __name__ == "__main__":
    main()
