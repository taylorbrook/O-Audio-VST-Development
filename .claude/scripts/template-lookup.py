#!/usr/bin/env python3
"""
Template Lookup Utility
Finds and returns relevant templates based on stage, tags, or features.

Usage:
  template-lookup.py stage <1|2|3>           # Templates for a stage
  template-lookup.py tags <tag1,tag2,...>    # Templates matching any tag
  template-lookup.py features <brief.md>     # Analyze brief for relevant templates
  template-lookup.py show <template-name>    # Show full template content
  template-lookup.py list                    # List all templates
"""

import os
import sys
import yaml
from pathlib import Path
from typing import List, Dict, Optional

TEMPLATES_DIR = Path(".claude/templates")


def load_registry() -> Dict:
    """Load the template registry"""
    registry_path = TEMPLATES_DIR / "registry.yaml"
    if not registry_path.exists():
        return {"templates": {"code-snippets": [], "prose-patterns": []}}

    with open(registry_path) as f:
        return yaml.safe_load(f)


def find_template_files() -> List[Path]:
    """Find all template YAML files"""
    templates = []
    for subdir in ["code-snippets", "prose-patterns"]:
        dir_path = TEMPLATES_DIR / subdir
        if dir_path.exists():
            templates.extend(dir_path.rglob("*.yaml"))
    return templates


def load_template(path: Path) -> Optional[Dict]:
    """Load a single template file"""
    try:
        with open(path) as f:
            return yaml.safe_load(f)
    except Exception as e:
        print(f"Error loading {path}: {e}", file=sys.stderr)
        return None


def get_templates_for_stage(stage: int) -> List[Dict]:
    """Get all templates relevant to a specific stage"""
    results = []
    for template_path in find_template_files():
        template = load_template(template_path)
        if template:
            template_stage = template.get("stage")
            if template_stage == stage or template_stage == "all":
                template["_path"] = str(template_path.relative_to(TEMPLATES_DIR))
                results.append(template)
    return results


def get_templates_by_tags(tags: List[str]) -> List[Dict]:
    """Get templates matching any of the given tags"""
    results = []
    tags_lower = [t.lower() for t in tags]

    for template_path in find_template_files():
        template = load_template(template_path)
        if template:
            template_tags = [t.lower() for t in template.get("tags", [])]
            if any(t in template_tags for t in tags_lower):
                template["_path"] = str(template_path.relative_to(TEMPLATES_DIR))
                results.append(template)
    return results


def find_template_by_name(name: str) -> Optional[Dict]:
    """Find a template by name (case-insensitive partial match)"""
    name_lower = name.lower()

    for template_path in find_template_files():
        template = load_template(template_path)
        if template:
            template_name = template.get("name", "").lower()
            if name_lower in template_name or template_name in name_lower:
                template["_path"] = str(template_path.relative_to(TEMPLATES_DIR))
                return template
    return None


def analyze_brief_for_templates(brief_path: str) -> List[Dict]:
    """Analyze a creative brief and suggest relevant templates"""
    try:
        with open(brief_path) as f:
            brief_content = f.read().lower()
    except FileNotFoundError:
        print(f"Brief not found: {brief_path}", file=sys.stderr)
        return []

    # Keywords to tags mapping
    keyword_tags = {
        "eq": ["eq", "filter", "iir"],
        "equalizer": ["eq", "filter", "iir"],
        "compressor": ["dynamics", "compressor", "envelope"],
        "compression": ["dynamics", "compressor", "envelope"],
        "limiter": ["dynamics", "limiter", "envelope"],
        "tremolo": ["lfo", "modulation", "tremolo"],
        "vibrato": ["lfo", "modulation", "vibrato"],
        "chorus": ["lfo", "modulation", "chorus"],
        "phaser": ["lfo", "modulation", "phaser"],
        "flanger": ["lfo", "modulation", "flanger"],
        "saturation": ["saturation", "distortion", "warmth"],
        "distortion": ["saturation", "distortion"],
        "warmth": ["saturation", "warmth", "analog"],
        "analog": ["saturation", "warmth", "analog"],
        "synth": ["synth", "instrument", "midi"],
        "synthesizer": ["synth", "instrument", "midi"],
        "instrument": ["synth", "instrument", "midi"],
        "vu meter": ["vu", "meter", "metering"],
        "meter": ["vu", "meter", "metering"],
        "webview": ["webview", "relay"],
    }

    # Find matching tags
    matched_tags = set()
    for keyword, tags in keyword_tags.items():
        if keyword in brief_content:
            matched_tags.update(tags)

    if matched_tags:
        return get_templates_by_tags(list(matched_tags))
    return []


def format_template_summary(template: Dict) -> str:
    """Format a template for display"""
    name = template.get("name", "Unknown")
    stage = template.get("stage", "?")
    complexity = template.get("complexity", "")
    path = template.get("_path", "")
    description = template.get("description", "")[:80]

    complexity_str = f" (complexity: {complexity})" if complexity else ""

    return f"""
  {name}
    Stage: {stage}{complexity_str}
    Path: {path}
    {description}..."""


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    command = sys.argv[1]

    if command == "stage":
        if len(sys.argv) < 3:
            print("Usage: template-lookup.py stage <1|2|3>")
            sys.exit(1)
        stage = int(sys.argv[2])
        templates = get_templates_for_stage(stage)
        print(f"━━━ Templates for Stage {stage} ━━━")
        for t in templates:
            print(format_template_summary(t))

    elif command == "tags":
        if len(sys.argv) < 3:
            print("Usage: template-lookup.py tags <tag1,tag2,...>")
            sys.exit(1)
        tags = sys.argv[2].split(",")
        templates = get_templates_by_tags(tags)
        print(f"━━━ Templates matching: {', '.join(tags)} ━━━")
        for t in templates:
            print(format_template_summary(t))

    elif command == "features":
        if len(sys.argv) < 3:
            print("Usage: template-lookup.py features <creative-brief.md>")
            sys.exit(1)
        brief_path = sys.argv[2]
        templates = analyze_brief_for_templates(brief_path)
        print(f"━━━ Suggested Templates for Brief ━━━")
        for t in templates:
            print(format_template_summary(t))

    elif command == "show":
        if len(sys.argv) < 3:
            print("Usage: template-lookup.py show <template-name>")
            sys.exit(1)
        name = " ".join(sys.argv[2:])
        template = find_template_by_name(name)
        if template:
            # Print as YAML
            print(yaml.dump(template, default_flow_style=False, sort_keys=False))
        else:
            print(f"Template not found: {name}")
            sys.exit(1)

    elif command == "list":
        print("━━━ All Templates ━━━")
        for template_path in sorted(find_template_files()):
            template = load_template(template_path)
            if template:
                template["_path"] = str(template_path.relative_to(TEMPLATES_DIR))
                print(format_template_summary(template))

    else:
        print(f"Unknown command: {command}")
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
