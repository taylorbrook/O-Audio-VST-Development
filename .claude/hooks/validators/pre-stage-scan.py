#!/usr/bin/env python3
"""
Pre-Stage Anti-Pattern Scanner
Runs BEFORE each implementation stage to catch issues proactively.

Usage: python3 pre-stage-scan.py <plugin_name> <stage_number>
Exit 0: PASS (no issues), Exit 1: FAIL (blocking), Exit 2: WARNING (non-blocking)
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple

# Stage-specific pattern definitions
# Each pattern has a 'stages' list indicating when it should be checked
STAGE_PATTERNS = {
    # === Stage 1 Patterns (Foundation/Build) ===
    "missing_juce_generate_header": {
        "stages": [1],
        "severity": "critical",
        "pattern": r'target_link_libraries\s*\([^)]*juce::[^)]*\)(?!\s*\n\s*juce_generate_juce_header)',
        "description": "CMakeLists.txt: target_link_libraries() without juce_generate_juce_header() following",
        "fix": "Add juce_generate_juce_header(TargetName) immediately after target_link_libraries()",
        "reference": "stage-1-patterns.md #1",
        "files": ["CMakeLists.txt"]
    },
    "missing_needs_web_browser": {
        "stages": [1, 3],
        "severity": "critical",
        "pattern": r'juce_add_plugin\s*\([^)]*FORMATS[^)]*VST3(?:(?!NEEDS_WEB_BROWSER).)*\)',
        "description": "VST3 format without NEEDS_WEB_BROWSER flag (WebView UI won't load)",
        "fix": "Add NEEDS_WEB_BROWSER TRUE to juce_add_plugin()",
        "reference": "stage-1-patterns.md #4",
        "files": ["CMakeLists.txt"]
    },
    "synth_missing_is_synth": {
        "stages": [1],
        "severity": "critical",
        "pattern": r'\.withOutput\s*\([^)]*\)(?!\s*\.\s*withInput).*?(?<!IS_SYNTH\s+TRUE)',
        "description": "Instrument with output-only bus but missing IS_SYNTH TRUE flag",
        "fix": "Add IS_SYNTH TRUE and NEEDS_MIDI_INPUT TRUE to juce_add_plugin()",
        "reference": "stage-1-patterns.md #5",
        "files": ["*.cpp"]
    },

    # === Stage 2 Patterns (DSP) ===
    "allocation_in_processblock": {
        "stages": [2],
        "severity": "critical",
        "pattern": r'void\s+\w*[Pp]rocessBlock.*?\{(?:[^}]|(?:\{[^}]*\}))*(?:new\s+\w+|std::vector\s*<|std::string\s+\w+\s*=)',
        "description": "Memory allocation detected in processBlock (not real-time safe)",
        "fix": "Pre-allocate buffers in prepareToPlay(), use only pre-allocated memory in processBlock()",
        "reference": "stage-2-patterns.md #2",
        "files": ["*.cpp"]
    },
    "missing_scoped_no_denormals": {
        "stages": [2],
        "severity": "warning",
        "pattern": r'void\s+\w*[Pp]rocessBlock\s*\([^)]*\)\s*(?:override\s*)?\{(?:(?!ScopedNoDenormals)[^}])*\}',
        "description": "processBlock() missing ScopedNoDenormals (may cause CPU spikes)",
        "fix": "Add juce::ScopedNoDenormals noDenormals; at start of processBlock()",
        "reference": "stage-2-patterns.md #2",
        "files": ["*.cpp"]
    },
    "old_reverb_api": {
        "stages": [2],
        "severity": "critical",
        "pattern": r'juce::dsp::Reverb.*?(?:setSampleRate|processMono|processStereo)\s*\(',
        "description": "juce::dsp::Reverb using old API (wrong methods for modern DSP)",
        "fix": "Use prepare(ProcessSpec) and process(ProcessContext) for juce::dsp components",
        "reference": "stage-2-patterns.md #3",
        "files": ["*.cpp"]
    },

    # === Stage 3 Patterns (GUI/WebView) ===
    "webslider_2param": {
        "stages": [3],
        "severity": "critical",
        "pattern": r'WebSliderParameterAttachment\s*\(\s*[^,]+,\s*[^,)]+\s*\)',
        "description": "WebSliderParameterAttachment with 2 parameters (JUCE 8 needs 3)",
        "fix": "Add nullptr as third parameter: WebSliderParameterAttachment(param, relay, nullptr)",
        "reference": "stage-3-patterns.md #8",
        "files": ["*.cpp", "*.h"]
    },
    "wrong_member_order": {
        "stages": [3],
        "severity": "critical",
        "pattern": r'WebBrowserComponent.*?WebSliderRelay',
        "description": "WebView declared before Relays (wrong initialization order)",
        "fix": "Declaration order must be: Relays → WebView → Attachments",
        "reference": "stage-3-patterns.md #7",
        "files": ["*.h"]
    },
    "webview_event_callback_param": {
        "stages": [3],
        "severity": "critical",
        "pattern": r'valueChangedEvent\.addListener\s*\(\s*\(\s*\w+\s*\)\s*=>',
        "description": "valueChangedEvent callback with parameter (JUCE doesn't pass values)",
        "fix": "Use: addListener(() => { const val = state.getNormalisedValue(); })",
        "reference": "stage-3-patterns.md #10",
        "files": ["*.js", "*.html"]
    },
    "es6_module_without_type": {
        "stages": [3],
        "severity": "critical",
        "pattern": r'<script\s+src=["\'][^"\']*juce[^"\']*index\.js["\'](?![^>]*type=["\']module["\'])',
        "description": "JUCE index.js loaded without type='module' (getSliderState undefined)",
        "fix": "Add type='module' attribute: <script type='module' src='...'>",
        "reference": "stage-3-patterns.md #14",
        "files": ["*.html"]
    },
    "absolute_knob_drag": {
        "stages": [3],
        "severity": "warning",
        "pattern": r'startY\s*-\s*e\.clientY',
        "description": "Absolute knob positioning (knob jumps to cursor instead of smooth drag)",
        "fix": "Use relative drag: lastY - e.clientY with lastY = e.clientY each frame",
        "reference": "stage-3-patterns.md #11",
        "files": ["*.js", "*.html"]
    },
    "getsliderstate_for_bool": {
        "stages": [3],
        "severity": "warning",
        "pattern": r'getSliderState\s*\(["\'](?:.*?(?:MODE|BYPASS|ENABLE|SWITCH|TOGGLE|MUTE|SOLO).*?)["\']',
        "description": "Using getSliderState() for boolean parameter",
        "fix": "Use getToggleState() for boolean parameters",
        "reference": "stage-3-patterns.md #12",
        "files": ["*.js", "*.html"]
    },
    "missing_check_native_interop": {
        "stages": [3],
        "severity": "critical",
        "pattern": r'juce_add_binary_data\s*\([^)]*index\.js(?:(?!check_native_interop).)*?\)',
        "description": "WebView resources include index.js but missing check_native_interop.js",
        "fix": "Add check_native_interop.js to juce_add_binary_data()",
        "reference": "stage-3-patterns.md #9",
        "files": ["CMakeLists.txt"]
    }
}


def get_patterns_for_stage(stage: int) -> Dict:
    """Filter patterns relevant to a specific stage"""
    return {
        k: v for k, v in STAGE_PATTERNS.items()
        if stage in v["stages"]
    }


def scan_file(file_path: Path, patterns: Dict) -> List[Dict]:
    """Scan a single file for anti-patterns"""
    try:
        content = file_path.read_text()
    except (UnicodeDecodeError, PermissionError):
        return []

    findings = []
    for pattern_id, pattern_def in patterns.items():
        # Check if pattern applies to this file type
        file_matches = any(file_path.match(g) for g in pattern_def["files"])
        if not file_matches:
            continue

        matches = list(re.finditer(pattern_def["pattern"], content, re.MULTILINE | re.DOTALL))
        if matches:
            line_numbers = [content[:m.start()].count('\n') + 1 for m in matches]
            findings.append({
                "file": str(file_path),
                "pattern": pattern_id,
                "severity": pattern_def["severity"],
                "description": pattern_def["description"],
                "fix": pattern_def["fix"],
                "reference": pattern_def["reference"],
                "lines": line_numbers
            })

    return findings


def scan_plugin(plugin_dir: Path, stage: int) -> Tuple[List[Dict], List[Dict]]:
    """Scan plugin for stage-specific anti-patterns"""
    patterns = get_patterns_for_stage(stage)
    critical = []
    warnings = []

    # Scan directories based on stage
    scan_paths = []

    # Stage 1: Focus on CMakeLists.txt and any existing Source/
    if stage == 1:
        cmake = plugin_dir / "CMakeLists.txt"
        if cmake.exists():
            scan_paths.append(cmake)
        source = plugin_dir / "Source"
        if source.exists():
            scan_paths.extend(source.rglob("*"))

    # Stage 2: Focus on Source/*.cpp for DSP patterns
    elif stage == 2:
        source = plugin_dir / "Source"
        if source.exists():
            scan_paths.extend(source.rglob("*.cpp"))
            scan_paths.extend(source.rglob("*.h"))

    # Stage 3: Focus on Source/ and ui/public/
    elif stage == 3:
        source = plugin_dir / "Source"
        if source.exists():
            scan_paths.extend(source.rglob("*"))
        ui_dir = plugin_dir / "Source" / "ui" / "public"
        if ui_dir.exists():
            scan_paths.extend(ui_dir.rglob("*"))
        cmake = plugin_dir / "CMakeLists.txt"
        if cmake.exists():
            scan_paths.append(cmake)

    # Scan files
    for path in scan_paths:
        if path.is_file():
            findings = scan_file(path, patterns)
            for f in findings:
                f["file"] = str(path.relative_to(plugin_dir))
                if f["severity"] == "critical":
                    critical.append(f)
                else:
                    warnings.append(f)

    return critical, warnings


def main():
    if len(sys.argv) < 3:
        print("Usage: pre-stage-scan.py <plugin_name> <stage_number>", file=sys.stderr)
        print("Example: pre-stage-scan.py OuariconTremolo 2", file=sys.stderr)
        sys.exit(1)

    plugin_name = sys.argv[1]
    try:
        stage = int(sys.argv[2])
    except ValueError:
        print(f"Invalid stage number: {sys.argv[2]}", file=sys.stderr)
        sys.exit(1)

    if stage not in [1, 2, 3]:
        print(f"Stage must be 1, 2, or 3 (got {stage})", file=sys.stderr)
        sys.exit(1)

    plugin_dir = Path("plugins") / plugin_name
    if not plugin_dir.exists():
        print(f"Plugin directory not found: {plugin_dir}", file=sys.stderr)
        sys.exit(1)

    stage_names = {1: "Foundation", 2: "DSP", 3: "GUI"}
    print(f"━━━ Pre-Stage Scan: {plugin_name} → Stage {stage} ({stage_names[stage]}) ━━━")

    patterns = get_patterns_for_stage(stage)
    print(f"Checking {len(patterns)} anti-patterns relevant to Stage {stage}...")

    critical, warnings = scan_plugin(plugin_dir, stage)

    if critical:
        print(f"\n❌ BLOCKING: {len(critical)} critical issue(s) found:\n", file=sys.stderr)
        for i, f in enumerate(critical, 1):
            print(f"  {i}. {f['file']}:{f['lines'][0]}", file=sys.stderr)
            print(f"     {f['description']}", file=sys.stderr)
            print(f"     Fix: {f['fix']}", file=sys.stderr)
            print(f"     Ref: {f['reference']}", file=sys.stderr)
            print("", file=sys.stderr)
        print("Fix these issues before Stage implementation.", file=sys.stderr)
        sys.exit(1)

    if warnings:
        print(f"\n⚠️  {len(warnings)} warning(s) (non-blocking):", file=sys.stderr)
        for i, f in enumerate(warnings, 1):
            print(f"  {i}. {f['file']}:{f['lines'][0]} - {f['description']}", file=sys.stderr)
        print("", file=sys.stderr)
        sys.exit(2)

    print(f"✓ No Stage {stage} anti-patterns detected - ready to proceed")
    sys.exit(0)


if __name__ == "__main__":
    main()
