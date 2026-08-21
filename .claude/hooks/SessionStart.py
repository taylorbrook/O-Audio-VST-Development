#!/usr/bin/env python3
"""SessionStart hook - Validate development environment with proactive dependency checks."""

import os
import re
import shutil
import subprocess
import sys
import time
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


def run_git(args, cwd, timeout=1):
    """Run a git command (list form only, never shell) and return stripped stdout.

    Returns None on non-zero exit, timeout, or a missing/unusable git binary -
    mirroring get_version_output()'s degrade-instead-of-raise contract.
    """
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return None
    if result.returncode != 0:
        return None
    return result.stdout.strip()


def _sanitize_frontmatter_value(value):
    """Bound what plugin-authored STATUS.md text can inject into session context."""
    value = value.split(" #")[0]
    value = value.strip()
    value = "".join(ch for ch in value if ord(ch) >= 32)
    return value[:60]


def _scan_inflight_plugins(repo_root):
    """Read plugins/*/.planning/STATUS.md frontmatter; return non-complete plugins."""
    done = {"complete", "plugin_complete", "installed"}
    key_re = re.compile(r"^(plugin|stage|status|phase):\s*(.*)$")
    inflight = []

    for status_path in sorted(repo_root.glob("plugins/*/.planning/STATUS.md")):
        try:
            with status_path.open("r", encoding="utf-8", errors="replace") as fh:
                lines = []
                for i, line in enumerate(fh):
                    if i >= 40:
                        break
                    lines.append(line.rstrip("\n"))
        except OSError:
            continue

        if not lines or lines[0].strip() != "---":
            continue  # no frontmatter - 8 of 37 files look like this

        fields = {}
        for line in lines[1:]:
            if line.strip() == "---":
                break
            match = key_re.match(line)
            if match and match.group(1) not in fields:
                fields[match.group(1)] = _sanitize_frontmatter_value(match.group(2))

        status = fields.get("status", "")
        if status in done:
            continue

        name = fields.get("plugin") or status_path.parent.parent.name
        inflight.append((name, fields.get("stage", "?"), status or "?"))

    return inflight


def print_git_context():
    """Print the session's location coordinates. Everything here goes to STDOUT -
    stdout is what enters the fresh session's context; stderr does not."""
    started = time.monotonic()
    deadline = 1.5  # cumulative wall-clock budget vs the hook's timeout: 5000

    def out_of_time():
        return (time.monotonic() - started) > deadline

    repo_root = Path(os.environ.get("CLAUDE_PROJECT_DIR") or Path.cwd())

    print("=== Git Context ===")

    toplevel = run_git(["rev-parse", "--show-toplevel"], repo_root)
    if not toplevel:
        print("[INFO] Git context unavailable (not a git repository)")
        print()
        return
    repo_root = Path(toplevel)

    truncated = False

    # ---- Branch ----
    if out_of_time():
        truncated = True
    else:
        branch = run_git(["branch", "--show-current"], repo_root)
        if branch is None:
            truncated = True
        elif branch == "main":
            print("[OK] Branch: main")
        elif branch == "":
            print("[WARN] Branch: (detached HEAD)")
        else:
            print(
                f"[WARN] Branch: {branch} — expected main; "
                "all plugin work belongs on main"
            )

    # ---- Worktrees ----
    if out_of_time():
        truncated = True
    else:
        wt_out = run_git(["worktree", "list", "--porcelain"], repo_root)
        if wt_out is None:
            truncated = True
        else:
            count = sum(
                1 for line in wt_out.splitlines() if line.startswith("worktree ")
            )
            if count > 1:
                print(
                    f"[WARN] Worktrees: {count} — extra checkouts exist; "
                    "run git worktree list"
                )
            else:
                print(f"[OK] Worktrees: {count}")

    # ---- Working tree ----
    if out_of_time():
        truncated = True
    else:
        status_out = run_git(["status", "--porcelain"], repo_root)
        if status_out is None:
            truncated = True
        elif status_out == "":
            print("[OK] Working tree: clean")
        else:
            lines = status_out.splitlines()
            staged = sum(1 for line in lines if line[:1] not in (" ", "?", ""))
            print(f"[INFO] Working tree: {len(lines)} changed, {staged} staged")

    # ---- Unpushed ----
    if out_of_time():
        truncated = True
    else:
        ahead = run_git(["rev-list", "--count", "origin/main..HEAD"], repo_root)
        if ahead is not None:  # None = no remote configured; skip the line entirely
            try:
                n = int(ahead)
            except ValueError:
                n = None
            if n is not None:
                if n > 0:
                    print(f"[INFO] Unpushed: {n} commit(s) ahead of origin/main")
                else:
                    print("[OK] Unpushed: 0")

    # ---- In-flight plugins ----
    if out_of_time():
        truncated = True
    else:
        inflight = _scan_inflight_plugins(repo_root)
        if inflight:
            print("In-flight plugins (plugins/*/.planning/STATUS.md):")
            for name, stage, status in inflight[:12]:
                print(f"  - {name} — stage {stage} — {status}")
            if len(inflight) > 12:
                print(f"  ... and {len(inflight) - 12} more")
        else:
            print("[OK] No in-flight plugins")

    if truncated:
        print("[INFO] Git context truncated (slow repository)")

    print()


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

    # ---- Git Context (location: branch / worktrees / dirty / unpushed / in-flight) ----
    # Wrapped so no defect in this section can ever block session start.
    try:
        print_git_context()
    except Exception:
        pass

    # Never block session start (allow user to see errors and fix)
    sys.exit(0)


if __name__ == "__main__":
    main()
