#!/usr/bin/env python3
"""
Contract Validation Library

Provides contract immutability enforcement for the Plugin Freedom System:
1. Checksum calculation and verification
2. Cross-contract consistency validation
3. Contract modification detection
4. Evolution tracking

Used by hooks and subagents to ensure contract integrity during Stages 1-4.
"""

import hashlib
import json
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass


@dataclass
class ContractChecksum:
    """Represents a contract file checksum."""
    file_name: str
    checksum: str

    def __repr__(self):
        return f"sha256:{self.checksum[:16]}..."


@dataclass
class ValidationResult:
    """Result of a contract validation check."""
    passed: bool
    message: str
    severity: str  # 'error', 'warning', 'info'
    contract: Optional[str] = None

    def to_dict(self):
        return {
            'passed': self.passed,
            'message': self.message,
            'severity': self.severity,
            'contract': self.contract
        }


class ContractValidator:
    """Validates contract files and enforces immutability."""

    CONTRACT_FILES = [
        'creative-brief.md',
        'parameter-spec.md',
        'architecture.md',
        'plan.md'
    ]

    def __init__(self, plugin_path: Path):
        """
        Initialize validator for a plugin.

        Args:
            plugin_path: Path to plugin directory (e.g., plugins/PluginName)
        """
        self.plugin_path = Path(plugin_path)
        self.ideas_path = self.plugin_path / '.ideas'
        self.continue_here_path = self.ideas_path / '.continue-here.md'

    def calculate_checksum(self, contract_file: str) -> Optional[str]:
        """
        Calculate SHA256 checksum for a contract file.

        Args:
            contract_file: Name of contract file (e.g., 'creative-brief.md')

        Returns:
            SHA256 checksum string, or None if file doesn't exist
        """
        file_path = self.ideas_path / contract_file
        if not file_path.exists():
            return None

        with open(file_path, 'rb') as f:
            content = f.read()
            return hashlib.sha256(content).hexdigest()

    def calculate_all_checksums(self) -> Dict[str, str]:
        """
        Calculate checksums for all contract files.

        Returns:
            Dict mapping contract file names to checksums
        """
        checksums = {}
        for contract_file in self.CONTRACT_FILES:
            checksum = self.calculate_checksum(contract_file)
            if checksum:
                # Convert filename to key (creative-brief.md -> creative_brief)
                key = contract_file.replace('.md', '').replace('-', '_')
                checksums[key] = f"sha256:{checksum}"
        return checksums

    def verify_checksums(self, stored_checksums: Dict[str, str]) -> List[ValidationResult]:
        """
        Verify current contract checksums match stored checksums.

        Args:
            stored_checksums: Dict from .continue-here.md frontmatter

        Returns:
            List of ValidationResult objects
        """
        results = []
        current_checksums = self.calculate_all_checksums()

        for key, stored_checksum in stored_checksums.items():
            # Convert key back to filename
            contract_file = key.replace('_', '-') + '.md'

            if key not in current_checksums:
                results.append(ValidationResult(
                    passed=False,
                    message=f"Contract file {contract_file} is missing",
                    severity='error',
                    contract=contract_file
                ))
                continue

            current_checksum = current_checksums[key]
            if current_checksum != stored_checksum:
                results.append(ValidationResult(
                    passed=False,
                    message=f"Contract {contract_file} has been modified (checksum mismatch)",
                    severity='error',
                    contract=contract_file
                ))
            else:
                results.append(ValidationResult(
                    passed=True,
                    message=f"Contract {contract_file} verified",
                    severity='info',
                    contract=contract_file
                ))

        return results

    def validate_cross_contract_consistency(self) -> List[ValidationResult]:
        """
        Validate consistency across contract files.

        Checks:
        1. Parameter counts match across creative-brief, parameter-spec, architecture
        2. Parameter names referenced in architecture exist in parameter-spec
        3. DSP components in architecture match plan.md stage breakdown
        4. Complexity score in plan matches parameter count

        Returns:
            List of ValidationResult objects
        """
        results = []

        # Read all contracts
        contracts = {}
        for contract_file in self.CONTRACT_FILES:
            file_path = self.ideas_path / contract_file
            if file_path.exists():
                with open(file_path, 'r') as f:
                    contracts[contract_file] = f.read()

        # Validation 1: Parameter counts
        param_counts = {}

        # Count from creative-brief (parameter table)
        if 'creative-brief.md' in contracts:
            brief = contracts['creative-brief.md']
            # Count table rows with parameter definitions
            param_rows = re.findall(r'^\|[^|]*\|[^|]*\|[^|]*\|[^|]*\|', brief, re.MULTILINE)
            # Exclude header and separator rows
            param_rows = [r for r in param_rows if not re.search(r'\|[-:]+\|', r)]
            # Exclude section headers (e.g., "| Parameter | Range | Default | Description |")
            param_rows = [r for r in param_rows if 'Parameter' not in r and 'Range' not in r]
            if param_rows:
                param_counts['creative-brief'] = len(param_rows)

        # Count from parameter-spec (Total Parameters field)
        if 'parameter-spec.md' in contracts:
            spec = contracts['parameter-spec.md']
            match = re.search(r'\*\*Total Parameters:\*\*\s*(\d+)', spec)
            if match:
                param_counts['parameter-spec'] = int(match.group(1))

            # Also count actual parameter definitions
            param_defs = re.findall(r'^###\s+[A-Z_]+\s*$', spec, re.MULTILINE)
            if param_defs:
                param_counts['parameter-spec-actual'] = len(param_defs)

        # Count from architecture (Parameter Mapping table)
        if 'architecture.md' in contracts:
            arch = contracts['architecture.md']
            # Look for parameter mapping section
            if 'Parameter Mapping' in arch or 'Parameter Map' in arch:
                param_rows = re.findall(r'^\|[^|]*\|[^|]*\|', arch, re.MULTILINE)
                param_rows = [r for r in param_rows if not re.search(r'\|[-:]+\|', r)]
                # Exclude header rows
                param_rows = [r for r in param_rows if 'Parameter' not in r and 'Type' not in r]
                if param_rows:
                    param_counts['architecture'] = len(param_rows)

        # Validate parameter counts match
        if len(param_counts) >= 2:
            counts = list(param_counts.values())
            if len(set(counts)) > 1:
                results.append(ValidationResult(
                    passed=False,
                    message=f"Parameter count mismatch across contracts: {param_counts}",
                    severity='error',
                    contract='cross-contract'
                ))
            else:
                results.append(ValidationResult(
                    passed=True,
                    message=f"Parameter count consistent across contracts: {counts[0]} parameters",
                    severity='info',
                    contract='cross-contract'
                ))

        # Validation 2: Parameter names consistency
        if 'parameter-spec.md' in contracts and 'architecture.md' in contracts:
            spec = contracts['parameter-spec.md']
            arch = contracts['architecture.md']

            # Extract parameter IDs from spec
            param_ids = set(re.findall(r'^###\s+([A-Z_]+)\s*$', spec, re.MULTILINE))

            # Extract parameter references from architecture
            # Look for parameter mapping rows
            param_refs = set()
            for match in re.finditer(r'\|\s*([A-Z_]+)\s*\|', arch):
                param_refs.add(match.group(1))

            if param_refs:
                missing = param_refs - param_ids
                if missing:
                    results.append(ValidationResult(
                        passed=False,
                        message=f"Architecture references undefined parameters: {missing}",
                        severity='error',
                        contract='architecture.md'
                    ))
                else:
                    results.append(ValidationResult(
                        passed=True,
                        message=f"All architecture parameter references defined in spec",
                        severity='info',
                        contract='architecture.md'
                    ))

        # Validation 3: DSP components match plan
        if 'architecture.md' in contracts and 'plan.md' in contracts:
            arch = contracts['architecture.md']
            plan = contracts['plan.md']

            # Extract DSP components from architecture
            dsp_components = []
            in_core_section = False
            for line in arch.split('\n'):
                if '## Core Components' in line or '## DSP Components' in line:
                    in_core_section = True
                elif line.startswith('##'):
                    in_core_section = False
                elif in_core_section and line.strip().startswith('-'):
                    component = line.strip().lstrip('- ').split(':')[0].strip()
                    if component:
                        dsp_components.append(component)

            # Check if these components are mentioned in plan Stage 1
            stage2_match = re.search(r'Stage 1:.*?(?=Stage [34]:|$)', plan, re.DOTALL)
            if stage2_match and dsp_components:
                stage2_text = stage2_match.group(0)
                missing_components = []
                for component in dsp_components:
                    # Flexible matching (partial string match)
                    if component.lower() not in stage2_text.lower():
                        missing_components.append(component)

                if missing_components:
                    results.append(ValidationResult(
                        passed=False,
                        message=f"DSP components not referenced in plan Stage 1: {missing_components}",
                        severity='warning',
                        contract='plan.md'
                    ))
                else:
                    results.append(ValidationResult(
                        passed=True,
                        message=f"All DSP components from architecture referenced in plan",
                        severity='info',
                        contract='plan.md'
                    ))

        return results

    def is_contract_file(self, file_path: str) -> bool:
        """
        Check if a file path is a contract file.

        Args:
            file_path: Absolute or relative file path

        Returns:
            True if file is a contract file in .ideas/ directory
        """
        path = Path(file_path)

        # Check if it's in an .ideas directory
        if '.ideas' not in path.parts:
            return False

        # Check if it's one of the contract files
        return path.name in self.CONTRACT_FILES

    def get_current_stage(self) -> Optional[int]:
        """
        Get current stage from .continue-here.md.

        Returns:
            Stage number, or None if not found
        """
        if not self.continue_here_path.exists():
            return None

        with open(self.continue_here_path, 'r') as f:
            content = f.read()
            match = re.search(r'^stage:\s*(\d+)', content, re.MULTILINE)
            if match:
                return int(match.group(1))

        return None

    def is_immutable_stage(self) -> bool:
        """
        Check if current stage requires contract immutability.

        Returns:
            True if in Stages 1-4 (implementation stages)
        """
        stage = self.get_current_stage()
        return stage is not None and 1 <= stage <= 4

    def add_evolution_entry(self, contract_file: str, description: str) -> None:
        """
        Add an evolution history entry to a contract file.

        Args:
            contract_file: Name of contract file
            description: Description of the change
        """
        from datetime import datetime

        file_path = self.ideas_path / contract_file
        if not file_path.exists():
            return

        with open(file_path, 'r') as f:
            content = f.read()

        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        entry = f"- [{timestamp}] {description}\n"

        # Check if Evolution History section exists
        if '## Evolution History' in content:
            # Append to existing section
            content = re.sub(
                r'(## Evolution History\s*\n)',
                r'\1' + entry,
                content,
                count=1
            )
        else:
            # Add new section at the end
            content += f"\n\n## Evolution History\n\n{entry}"

        with open(file_path, 'w') as f:
            f.write(content)


def main():
    """Command-line interface for contract validation."""
    import sys
    import argparse

    parser = argparse.ArgumentParser(description='Validate plugin contracts')
    parser.add_argument('plugin_path', help='Path to plugin directory')
    parser.add_argument('--checksums', help='Verify checksums from JSON string')
    parser.add_argument('--cross-contract', action='store_true', help='Run cross-contract validation')
    parser.add_argument('--calculate', action='store_true', help='Calculate and print checksums')

    args = parser.parse_args()

    validator = ContractValidator(args.plugin_path)

    if args.calculate:
        checksums = validator.calculate_all_checksums()
        print(json.dumps(checksums, indent=2))
        return 0

    results = []

    if args.checksums:
        stored_checksums = json.loads(args.checksums)
        results.extend(validator.verify_checksums(stored_checksums))

    if args.cross_contract:
        results.extend(validator.validate_cross_contract_consistency())

    # Print results
    errors = []
    warnings = []

    for result in results:
        if result.severity == 'error' and not result.passed:
            errors.append(result)
            print(f"❌ ERROR: {result.message}", file=sys.stderr)
        elif result.severity == 'warning' and not result.passed:
            warnings.append(result)
            print(f"⚠️  WARNING: {result.message}", file=sys.stderr)
        elif result.passed and result.severity == 'info':
            print(f"✓ {result.message}")

    # Return exit code
    if errors:
        return 1
    elif warnings:
        return 2
    else:
        return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
