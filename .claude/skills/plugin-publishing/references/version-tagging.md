# Version Tagging Reference

Conventions and best practices for plugin version management.

---

## Tag Format

```
{PluginName}-v{MAJOR}.{MINOR}.{PATCH}
```

### Examples
- `OuariconTremolo-v1.4.0`
- `GainKnob-v2.0.0`
- `Ouaricon Digital Delay-v1.2.1`

### Rationale
- **Includes plugin name** - Enables per-plugin releases in multi-plugin repo
- **Hyphen separator** - Distinguishes name from version
- **Lowercase 'v' prefix** - Common convention, easy to parse
- **Semantic versioning** - Industry standard for version numbers

---

## Semantic Versioning

Based on [SemVer 2.0.0](https://semver.org/)

### MAJOR (X.0.0)
Increment when making **breaking changes**:
- Parameter ID changes (breaks automation)
- Preset format changes (old presets won't load)
- Removed features
- API changes

**User impact:** May need to redo automation, reload presets, or adjust workflow.

### MINOR (x.Y.0)
Increment when adding **new features** (backward compatible):
- New parameters or controls
- New presets
- New waveforms or modes
- UI enhancements
- Performance improvements

**User impact:** Existing projects work, new capabilities available.

### PATCH (x.y.Z)
Increment for **bug fixes** (backward compatible):
- Crash fixes
- Audio glitch corrections
- UI rendering fixes
- Documentation updates

**User impact:** Existing projects work better, no learning curve.

---

## Version Bump Decision Tree

```
Is this a breaking change?
├── YES → MAJOR (X.0.0)
└── NO → Does it add new features?
         ├── YES → MINOR (x.Y.0)
         └── NO → PATCH (x.y.Z)
```

### Quick Examples

| Change | Bump | Result |
|--------|------|--------|
| Fixed crash on preset load | PATCH | 1.3.0 → 1.3.1 |
| Added 5 new factory presets | MINOR | 1.3.1 → 1.4.0 |
| Improved reverb algorithm | MINOR | 1.4.0 → 1.5.0 |
| Changed parameter IDs | MAJOR | 1.5.0 → 2.0.0 |
| Fixed typo in UI | PATCH | 2.0.0 → 2.0.1 |
| Added MIDI learn | MINOR | 2.0.1 → 2.1.0 |

---

## Version Storage Locations

### CMakeLists.txt
Primary source of truth:
```cmake
juce_add_plugin(OuariconTremolo
    VERSION 1.4.0
    ...
)
```

### PLUGINS.md
Registry for quick reference:
```markdown
| OuariconTremolo | 📦 Installed | 1.4.0 | Audio Effect (Tremolo) | 2026-01-24 |
```

### CHANGELOG.md
History with details:
```markdown
## [1.4.0] - 2026-01-24

### Added
- Preset management system
```

---

## Git Tagging Commands

### Create Annotated Tag
```bash
git tag -a "OuariconTremolo-v1.4.0" -m "Release OuariconTremolo v1.4.0"
```

### Push Tag
```bash
git push origin "OuariconTremolo-v1.4.0"
```

### List Tags for Plugin
```bash
git tag -l "OuariconTremolo-v*"
```

### Delete Tag (If Release Failed)
```bash
# Local
git tag -d "OuariconTremolo-v1.4.0"

# Remote
git push origin :refs/tags/OuariconTremolo-v1.4.0
```

### View Tag Details
```bash
git show "OuariconTremolo-v1.4.0"
```

---

## Version Parsing (Bash)

### Extract from Tag
```bash
TAG="OuariconTremolo-v1.4.0"
PLUGIN_NAME="${TAG%-v*}"       # OuariconTremolo
VERSION="${TAG##*-v}"          # 1.4.0
```

### Extract from CMakeLists.txt
```bash
VERSION=$(grep "VERSION" plugins/OuariconTremolo/CMakeLists.txt | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
```

### Bump Version (Bash Functions)
```bash
bump_patch() {
  local version="$1"
  local major=$(echo $version | cut -d. -f1)
  local minor=$(echo $version | cut -d. -f2)
  local patch=$(echo $version | cut -d. -f3)
  echo "${major}.${minor}.$((patch + 1))"
}

bump_minor() {
  local version="$1"
  local major=$(echo $version | cut -d. -f1)
  local minor=$(echo $version | cut -d. -f2)
  echo "${major}.$((minor + 1)).0"
}

bump_major() {
  local version="$1"
  local major=$(echo $version | cut -d. -f1)
  echo "$((major + 1)).0.0"
}
```

---

## Pre-Release Tags (Optional)

For beta/RC releases:
```
OuariconTremolo-v2.0.0-beta.1
OuariconTremolo-v2.0.0-rc.1
```

Sorted correctly by semantic version tools.

---

## Common Mistakes

### 1. Forgetting 'v' prefix
- Wrong: `OuariconTremolo-1.4.0`
- Right: `OuariconTremolo-v1.4.0`

### 2. Using spaces in plugin name tags
- Plugin name: `Ouaricon Digital Delay`
- Tag format: Keep as-is (`Ouaricon Digital Delay-v1.2.0`)
- Note: Requires quoting in shell commands

### 3. Version mismatch across files
Always update together:
1. CMakeLists.txt VERSION
2. PLUGINS.md version column
3. CHANGELOG.md entry header

### 4. Pushing before committing
Ensure release commit is pushed before tag:
```bash
git push origin HEAD
git push origin "PluginName-v1.0.0"
```

---

## Integration with GitHub Actions

The workflow file matches tags with pattern:
```yaml
on:
  push:
    tags:
      - '*-v*'
```

This triggers on any tag containing `-v`, extracting plugin name and version dynamically.
