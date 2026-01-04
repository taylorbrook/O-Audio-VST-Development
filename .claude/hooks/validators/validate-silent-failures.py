#!/usr/bin/env python3
"""
Silent Failure Pattern Detector
Scans code for known patterns that compile but fail at runtime
Exit 0: PASS, Exit 1: FAIL (blocking), Exit 2: WARNING (non-blocking)
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple

# Pattern definitions from juce8-critical-patterns.md
SILENT_FAILURE_PATTERNS = {
    "webslider_2param": {
        "severity": "critical",
        "pattern": r'WebSliderParameterAttachment\s*\([^,)]+,[^,)]+\)',
        "description": "WebSliderParameterAttachment with 2 parameters (needs 3 in JUCE 8)",
        "fix": "Add nullptr as third parameter: WebSliderParameterAttachment(param, relay, nullptr)",
        "reference": "Pattern #12 in juce8-critical-patterns.md",
        "files": ["*.cpp", "*.h"]
    },
    "webtoggle_2param": {
        "severity": "critical",
        "pattern": r'WebToggleButtonParameterAttachment\s*\([^,)]+,[^,)]+\)',
        "description": "WebToggleButtonParameterAttachment with 2 parameters (needs 3 in JUCE 8)",
        "fix": "Add nullptr as third parameter",
        "reference": "Pattern #12 variant",
        "files": ["*.cpp", "*.h"]
    },
    "missing_virtual_destructor": {
        "severity": "warning",
        "pattern": r'class\s+\w+\s*:\s*public\s+juce::(?:Component|AudioProcessor)\s*\{(?:(?!virtual\s+~).)*?\}',
        "description": "Component/AudioProcessor subclass without virtual destructor",
        "fix": "Add: virtual ~ClassName() override = default;",
        "reference": "Common threading issue",
        "files": ["*.h"]
    },
    "processblock_ui_call": {
        "severity": "critical",
        "pattern": r'void\s+processBlock.*?\{(?:[^}]|(?:\{[^}]*\}))*repaint\s*\(',
        "description": "repaint() called in processBlock (thread violation)",
        "fix": "Use MessageManager::callAsync() or parameter listeners",
        "reference": "Pattern #5 in juce8-critical-patterns.md",
        "files": ["*.cpp"]
    },
    "missing_juce_generate_header": {
        "severity": "critical",
        "pattern": r'target_link_libraries.*?juce::.*?(?!.*juce_generate_juce_header)',
        "description": "CMakeLists.txt missing juce_generate_juce_header() after target_link_libraries()",
        "fix": "Add: juce_generate_juce_header(TargetName) after target_link_libraries()",
        "reference": "Pattern #1 in juce8-critical-patterns.md",
        "files": ["CMakeLists.txt"]
    },
    "missing_needs_web_browser": {
        "severity": "critical",
        "pattern": r'juce_add_plugin\s*\([^)]*FORMATS[^)]*VST3(?:(?!NEEDS_WEB_BROWSER).)*?\)',
        "description": "VST3 format without NEEDS_WEB_BROWSER flag (WebView won't load)",
        "fix": "Add: NEEDS_WEB_BROWSER TRUE to juce_add_plugin()",
        "reference": "Pattern #9 in juce8-critical-patterns.md",
        "files": ["CMakeLists.txt"]
    },
    "webview_event_callback_param": {
        "severity": "critical",
        "pattern": r'valueChangedEvent\.addListener\s*\(\s*\(\s*\w+\s*\)\s*=>',
        "description": "valueChangedEvent callback with parameter (JUCE doesn't pass values)",
        "fix": "Use callback without params: addListener(() => { const value = state.getNormalisedValue(); })",
        "reference": "Pattern #15 in juce8-critical-patterns.md",
        "files": ["*.js", "*.html"]
    },
    "absolute_knob_drag": {
        "severity": "warning",
        "pattern": r'startY\s*-\s*e\.clientY',
        "description": "Absolute knob positioning (knob jumps to cursor)",
        "fix": "Use relative drag: lastY - e.clientY with lastY = e.clientY each frame",
        "reference": "Pattern #16 in juce8-critical-patterns.md",
        "files": ["*.js", "*.html"]
    },
    "old_reverb_api": {
        "severity": "critical",
        "pattern": r'juce::dsp::Reverb.*?setSampleRate\s*\(',
        "description": "juce::dsp::Reverb using old API (setSampleRate doesn't exist)",
        "fix": "Use prepare(ProcessSpec) instead of setSampleRate()",
        "reference": "Pattern #17 in juce8-critical-patterns.md",
        "files": ["*.cpp"]
    },
    "missing_check_native_interop": {
        "severity": "critical",
        "pattern": r'juce_add_binary_data.*?(?:(?!check_native_interop\.js).)*?(?=\n\s*(?:SOURCE|\)))',
        "description": "WebView UI resources missing check_native_interop.js",
        "fix": "Add: Source/ui/public/js/juce/check_native_interop.js to juce_add_binary_data()",
        "reference": "Pattern #13 in juce8-critical-patterns.md",
        "files": ["CMakeLists.txt"]
    },
    "es6_module_without_type": {
        "severity": "critical",
        "pattern": r'<script\s+src=["\'].*?juce.*?index\.js["\'](?!\s+type=["\']module["\'])',
        "description": "JUCE index.js loaded without type='module' (getSliderState undefined)",
        "fix": "Add type='module' attribute: <script type='module' src='js/juce/index.js'>",
        "reference": "Pattern #21 in juce8-critical-patterns.md",
        "files": ["*.html"]
    },
    "getsliderstate_for_bool": {
        "severity": "warning",
        "pattern": r'getSliderState\s*\(["\'](?:.*?(?:MODE|BYPASS|ENABLE|SWITCH|TOGGLE).*?)["\']',
        "description": "Using getSliderState() for boolean parameter (should use getToggleState())",
        "fix": "Use getToggleState() for boolean parameters",
        "reference": "Pattern #19 in juce8-critical-patterns.md",
        "files": ["*.js", "*.html"]
    }
}

def find_plugin_directory() -> Path:
    """Find the active plugin directory"""
    plugins_dir = Path("plugins")
    if not plugins_dir.exists():
        return None

    # Prefer plugin with Source/ directory (most recent work)
    for plugin_dir in plugins_dir.iterdir():
        if plugin_dir.is_dir():
            source_dir = plugin_dir / "Source"
            if source_dir.exists():
                return plugin_dir

    return None

def scan_file_for_patterns(file_path: Path, patterns: Dict) -> List[Tuple[str, Dict, List[str]]]:
    """Scan a single file for all relevant patterns"""
    try:
        content = file_path.read_text()
    except (UnicodeDecodeError, PermissionError):
        return []

    findings = []
    for pattern_id, pattern_def in patterns.items():
        # Check if pattern applies to this file type
        file_matches = False
        for file_glob in pattern_def["files"]:
            if file_path.match(file_glob):
                file_matches = True
                break

        if not file_matches:
            continue

        # Search for pattern
        matches = list(re.finditer(pattern_def["pattern"], content, re.MULTILINE | re.DOTALL))
        if matches:
            # Extract line numbers
            line_numbers = []
            for match in matches:
                line_num = content[:match.start()].count('\n') + 1
                line_numbers.append(line_num)

            findings.append((pattern_id, pattern_def, line_numbers))

    return findings

def scan_plugin(plugin_dir: Path) -> Tuple[List, List]:
    """Scan entire plugin for silent failure patterns"""
    critical_findings = []
    warning_findings = []

    # Scan Source/
    source_dir = plugin_dir / "Source"
    if source_dir.exists():
        for file_path in source_dir.rglob("*"):
            if file_path.is_file():
                findings = scan_file_for_patterns(file_path, SILENT_FAILURE_PATTERNS)
                for pattern_id, pattern_def, line_numbers in findings:
                    rel_path = file_path.relative_to(plugin_dir)
                    finding = {
                        "file": str(rel_path),
                        "pattern": pattern_id,
                        "severity": pattern_def["severity"],
                        "description": pattern_def["description"],
                        "fix": pattern_def["fix"],
                        "reference": pattern_def["reference"],
                        "lines": line_numbers
                    }

                    if pattern_def["severity"] == "critical":
                        critical_findings.append(finding)
                    else:
                        warning_findings.append(finding)

    # Scan CMakeLists.txt
    cmake_file = plugin_dir / "CMakeLists.txt"
    if cmake_file.exists():
        findings = scan_file_for_patterns(cmake_file, SILENT_FAILURE_PATTERNS)
        for pattern_id, pattern_def, line_numbers in findings:
            finding = {
                "file": "CMakeLists.txt",
                "pattern": pattern_id,
                "severity": pattern_def["severity"],
                "description": pattern_def["description"],
                "fix": pattern_def["fix"],
                "reference": pattern_def["reference"],
                "lines": line_numbers
            }

            if pattern_def["severity"] == "critical":
                critical_findings.append(finding)
            else:
                warning_findings.append(finding)

    # Scan ui/public/ (WebView resources)
    ui_dir = plugin_dir / "ui" / "public"
    if ui_dir.exists():
        for file_path in ui_dir.rglob("*"):
            if file_path.is_file():
                findings = scan_file_for_patterns(file_path, SILENT_FAILURE_PATTERNS)
                for pattern_id, pattern_def, line_numbers in findings:
                    rel_path = file_path.relative_to(plugin_dir)
                    finding = {
                        "file": str(rel_path),
                        "pattern": pattern_id,
                        "severity": pattern_def["severity"],
                        "description": pattern_def["description"],
                        "fix": pattern_def["fix"],
                        "reference": pattern_def["reference"],
                        "lines": line_numbers
                    }

                    if pattern_def["severity"] == "critical":
                        critical_findings.append(finding)
                    else:
                        warning_findings.append(finding)

    return critical_findings, warning_findings

def main():
    """Main validation entry point"""
    # Find active plugin
    plugin_dir = find_plugin_directory()
    if not plugin_dir:
        print("No active plugin found, skipping silent failure detection", file=sys.stderr)
        sys.exit(0)  # Graceful skip

    print(f"Scanning {plugin_dir.name} for silent failure patterns...")

    critical_findings, warning_findings = scan_plugin(plugin_dir)

    # Report findings
    if critical_findings:
        print("\n❌ CRITICAL: Silent failure patterns detected (will compile but fail at runtime):\n", file=sys.stderr)
        for i, finding in enumerate(critical_findings, 1):
            print(f"{i}. {finding['file']} (lines {', '.join(map(str, finding['lines']))})", file=sys.stderr)
            print(f"   Issue: {finding['description']}", file=sys.stderr)
            print(f"   Fix: {finding['fix']}", file=sys.stderr)
            print(f"   Reference: {finding['reference']}", file=sys.stderr)
            print("", file=sys.stderr)

        print(f"✗ {len(critical_findings)} critical pattern(s) found", file=sys.stderr)
        print("These patterns WILL cause runtime failures even though code compiles", file=sys.stderr)
        print("Fix immediately before testing plugin", file=sys.stderr)
        sys.exit(1)  # Block workflow

    if warning_findings:
        print("\n⚠️  WARNING: Potential issues detected:\n", file=sys.stderr)
        for i, finding in enumerate(warning_findings, 1):
            print(f"{i}. {finding['file']} (lines {', '.join(map(str, finding['lines']))})", file=sys.stderr)
            print(f"   Issue: {finding['description']}", file=sys.stderr)
            print(f"   Fix: {finding['fix']}", file=sys.stderr)
            print("", file=sys.stderr)

        print(f"⚠️  {len(warning_findings)} warning(s) - not blocking but should be addressed", file=sys.stderr)
        sys.exit(2)  # Warning (non-blocking)

    print("✓ No silent failure patterns detected")
    sys.exit(0)

if __name__ == "__main__":
    main()
