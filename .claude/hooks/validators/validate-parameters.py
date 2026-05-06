#!/usr/bin/env python3
"""
Parameter Validator (Stage 2)
Validates that all parameters from parameter-spec.md exist in PluginProcessor.cpp
Exit 0: PASS, Exit 1: FAIL
"""

import os
import re
import sys
from pathlib import Path
from typing import Dict, Set, List, Tuple

def find_plugin_directory() -> Path:
    """Find the active plugin directory (has parameter-spec.md)"""
    plugins_dir = Path("plugins")
    if not plugins_dir.exists():
        return None

    # Check ACTIVE_PLUGIN env var first
    active = os.environ.get("ACTIVE_PLUGIN", "")
    if active:
        plugin_dir = plugins_dir / active
        if plugin_dir.is_dir() and (plugin_dir / ".planning" / "parameter-spec.md").exists():
            return plugin_dir

    for plugin_dir in plugins_dir.iterdir():
        if plugin_dir.is_dir():
            spec_file = plugin_dir / ".planning" / "parameter-spec.md"
            if spec_file.exists():
                return plugin_dir

    return None

def parse_parameter_spec(spec_path: Path) -> Dict[str, Dict]:
    """Parse parameter-spec.md to extract parameter IDs, types, and ranges"""
    if not spec_path.exists():
        print(f"ERROR: parameter-spec.md not found at {spec_path}", file=sys.stderr)
        return {}

    content = spec_path.read_text()
    parameters = {}

    # Format 1: Markdown table format
    # Anchored to start-of-line (multiline). ID is UPPER_SNAKE_CASE so Name-column
    # words ("Brightness", "Rosin") never get picked up. Accepts any intermediate
    # columns (Name, Range, Default, Unit, Description) between ID and Type.
    table_pattern = re.compile(
        r'^\|\s*([A-Z][A-Z0-9_]*)\s*\|.*?\|\s*(Float|Bool|Choice|Int)\s*\|',
        re.MULTILINE,
    )
    for match in table_pattern.finditer(content):
        param_id = match.group(1)
        param_type = match.group(2)
        parameters[param_id] = {"type": param_type}

    # Format 2: Section-based format (e.g., "### paramID" followed by "- **Type:** Float")
    # This format is used when parameters have detailed descriptions
    section_pattern = r'^###\s+(\w+)\s*$'
    type_pattern = r'^\s*-\s*\*\*Type:\*\*\s*(Float|Bool|Choice|Int)'

    lines = content.split('\n')
    current_param = None

    for i, line in enumerate(lines):
        # Check for parameter section heading
        section_match = re.match(section_pattern, line)
        if section_match:
            current_param = section_match.group(1)
            # Look ahead for type in next few lines
            for j in range(i + 1, min(i + 10, len(lines))):
                type_match = re.match(type_pattern, lines[j])
                if type_match:
                    parameters[current_param] = {"type": type_match.group(1)}
                    break

    return parameters

def parse_plugin_processor(processor_path: Path) -> Dict[str, Dict]:
    """Parse PluginProcessor.cpp to extract APVTS parameter declarations"""
    if not processor_path.exists():
        print(f"ERROR: PluginProcessor.cpp not found at {processor_path}", file=sys.stderr)
        return {}

    content = processor_path.read_text()
    parameters = {}

    # Build alias map from `using APF = juce::AudioParameterFloat;` style declarations.
    alias_pattern = re.compile(
        r'using\s+(\w+)\s*=\s*(?:juce::)?AudioParameter(Float|Bool|Choice|Int)\s*;'
    )
    alias_map = {m.group(1): m.group(2) for m in alias_pattern.finditer(content)}

    # Match parameter declarations: full type or alias inside make_unique<...>
    # Combined with ParameterID{"NAME", n}.
    type_options = ["AudioParameter(?:Float|Bool|Choice|Int)"] + list(alias_map.keys())
    type_alt = "|".join(f"(?:{t})" for t in type_options)
    param_pattern = re.compile(
        rf'make_unique\s*<\s*(?:juce::)?({type_alt})\s*>\s*\([\s\S]*?'
        rf'ParameterID\s*\{{\s*"(\w+)"'
    )

    for match in re.finditer(param_pattern, content):
        type_tok = match.group(1)
        param_id = match.group(2)
        if type_tok in alias_map:
            param_type = alias_map[type_tok]
        else:
            # Strip "AudioParameter" prefix to get bare type name.
            param_type = type_tok.replace("AudioParameter", "")
        parameters[param_id] = {"type": param_type}

    # Fallback: legacy style (direct AudioParameterFloat constructor without make_unique).
    if not parameters:
        legacy_pattern = r'AudioParameter(Float|Bool|Choice|Int)[\s\S]*?ParameterID\s*\{\s*"(\w+)"'
        for match in re.finditer(legacy_pattern, content):
            parameters[match.group(2)] = {"type": match.group(1)}

    return parameters

def validate_parameters(spec_params: Dict[str, Dict], code_params: Dict[str, Dict]) -> Tuple[bool, List[str]]:
    """Validate that spec parameters match code parameters"""
    errors = []

    spec_ids = set(spec_params.keys())
    code_ids = set(code_params.keys())

    # Check for missing parameters (in spec but not in code)
    missing = spec_ids - code_ids
    if missing:
        errors.append(f"Missing parameters in code: {', '.join(sorted(missing))}")

    # Check for extra parameters (in code but not in spec) - WARNING only
    extra = code_ids - spec_ids
    if extra:
        print(f"WARNING: Extra parameters in code (not in spec): {', '.join(sorted(extra))}", file=sys.stderr)

    # Check type mapping for common parameters
    for param_id in spec_ids & code_ids:
        spec_type = spec_params[param_id]["type"]
        code_type = code_params[param_id]["type"]

        # Validate type mapping
        if spec_type != code_type:
            errors.append(f"Type mismatch for '{param_id}': spec has {spec_type}, code has {code_type}")

    return len(errors) == 0, errors

def main():
    """Main validation entry point"""
    # Find active plugin
    plugin_dir = find_plugin_directory()
    if not plugin_dir:
        print("No active plugin with parameter-spec.md found, skipping validation", file=sys.stderr)
        sys.exit(0)  # Graceful skip

    print(f"Validating parameters for: {plugin_dir.name}")

    # Parse spec and code
    spec_path = plugin_dir / ".planning" / "parameter-spec.md"
    processor_path = plugin_dir / "Source" / "PluginProcessor.cpp"

    spec_params = parse_parameter_spec(spec_path)
    code_params = parse_plugin_processor(processor_path)

    if not spec_params:
        print("WARNING: No parameters found in parameter-spec.md", file=sys.stderr)
        sys.exit(0)  # Skip if no parameters defined yet

    if not code_params:
        print("ERROR: No APVTS parameters found in PluginProcessor.cpp", file=sys.stderr)
        sys.exit(1)

    # Validate
    success, errors = validate_parameters(spec_params, code_params)

    if success:
        print(f"✓ All {len(spec_params)} parameters from spec present in code")
        sys.exit(0)
    else:
        print("✗ Parameter validation FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
