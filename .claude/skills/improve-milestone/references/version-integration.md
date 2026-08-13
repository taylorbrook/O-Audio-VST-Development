# Version Integration

This document defines the backup, CHANGELOG, and git procedures for improve-milestone.

---

## Version Determination

### During Plan Phase

Version bump type is determined by analyzing the improvement scope:

| Bump Type | Criteria | Examples |
|-----------|----------|----------|
| PATCH (0.0.x) | Bug fix, no new features | Fix click on parameter change, correct filter calculation |
| MINOR (0.x.0) | New feature, backward compatible | Add chorus effect, new parameter, UI enhancement |
| MAJOR (x.0.0) | Breaking changes | Changed parameter IDs, removed features, preset incompatibility |

### Version Calculation

```javascript
function calculateVersion(current, bumpType) {
  const [major, minor, patch] = current.split('.').map(Number);

  switch (bumpType) {
    case 'major':
      return `${major + 1}.0.0`;
    case 'minor':
      return `${major}.${minor + 1}.0`;
    case 'patch':
      return `${major}.${minor}.${patch + 1}`;
  }
}
```

### Breaking Change Detection

Auto-detect breaking changes by checking:

- [ ] Parameter ID renamed
- [ ] Parameter ID removed
- [ ] Parameter range changed
- [ ] Parameter type changed (float → choice)
- [ ] State format changed
- [ ] Feature removed
- [ ] Public API signature changed

If any detected → Force MAJOR bump with user confirmation.

---

## Backup Creation

### Timing

Backup created at **start of execute phase**, before any implementation.

### Location

```
backups/[PluginName]/v[baseVersion]/
├── Source/
│   └── [all source files]
├── CMakeLists.txt
├── CHANGELOG.md
└── .planning/
    └── [planning files]
```

### Creation Script

```bash
#!/bin/bash
# Create backup before execute phase

PLUGIN_NAME="$1"
VERSION="$2"
BACKUP_DIR="backups/${PLUGIN_NAME}/v${VERSION}"

# Create backup directory
mkdir -p "$BACKUP_DIR"

# Copy plugin source (exclude build artifacts)
rsync -av --exclude='build/' --exclude='*.log' \
  "plugins/${PLUGIN_NAME}/" "$BACKUP_DIR/"

# Verify backup
if [ -f "$BACKUP_DIR/CMakeLists.txt" ]; then
  echo "✓ Backup created: $BACKUP_DIR"
  exit 0
else
  echo "✗ Backup verification failed"
  exit 1
fi
```

### Verification

Before proceeding with execute phase:

1. Check backup directory exists
2. Verify CMakeLists.txt present
3. Verify Source/ directory present
4. Compare file count (sanity check)

**If verification fails:** HALT execute phase, do not proceed.

---

## CHANGELOG Update

### Timing

CHANGELOG updated **after verify phase** succeeds, before git operations.

### Location

`plugins/[PluginName]/CHANGELOG.md`

### Entry Format

```markdown
## [X.Y.Z] - YYYY-MM-DD

### Added
- [New features from CONTEXT.md requirements]

### Changed
- [Modifications from SUMMARY.md]

### Fixed
- [Bug fixes if applicable]

### Technical Notes
- Domain: [DSP/GUI/Polish/Mixed]
- Milestone: [slug]
- Investigation: [1-2 sentence summary from RESEARCH.md]
```

### Entry Generation

Read from milestone documents:

```javascript
function generateChangelogEntry(milestone) {
  const context = readFile(`${milestone.path}/CONTEXT.md`);
  const summary = readFile(`${milestone.path}/SUMMARY.md`);
  const research = readFile(`${milestone.path}/RESEARCH.md`);

  // Extract sections
  const requirements = extractSection(context, '## Requirements Gathered');
  const changes = extractSection(summary, '## Implementation Summary');
  const approach = extractSection(research, '## Recommended Approach');

  // Categorize changes
  const added = requirements.filter(r => r.type === 'new_feature');
  const changed = requirements.filter(r => r.type === 'modification');
  const fixed = requirements.filter(r => r.type === 'bug_fix');

  return formatEntry(milestone.targetVersion, added, changed, fixed, {
    domain: milestone.domain,
    slug: milestone.slug,
    investigation: approach.summary
  });
}
```

### Insertion

Insert new entry at top of CHANGELOG.md, after header but before previous entries.

---

## Git Operations

### Timing

Git operations performed **after CHANGELOG update**, as final step.

### Commit

```bash
# Stage all milestone-related changes
# plugin-local STATUS.md and the improvements/ dir are under plugins/[PluginName]/
git add plugins/[PluginName]/

# Create commit with conventional format
git commit -m "improve: [PluginName] v[version] - [milestone-slug]

- [Key change 1]
- [Key change 2]
- [Key change 3]

Milestone: [slug]
Domain: [domain]
"
```

### Tag

```bash
# Create annotated tag
git tag -a "v[version]" -m "[PluginName] v[version]: [milestone description]

Changes:
- [Key change 1]
- [Key change 2]

Milestone: [slug]
Verified: [verify phase result]
"
```

### Push (User-Initiated)

Do NOT auto-push. Display instructions:

```
Git operations complete:
- Commit: [hash] improve: [PluginName] v[version] - [slug]
- Tag: v[version]

To push changes:
  git push origin main
  git push origin v[version]
```

---

## Rollback Procedure

If issues found during verify phase:

### Option 1: Fix and Re-verify

1. Return to execute phase
2. Make corrections
3. Re-run verify phase

### Option 2: Rollback to Backup

```bash
#!/bin/bash
# Rollback to backup

PLUGIN_NAME="$1"
VERSION="$2"
BACKUP_DIR="backups/${PLUGIN_NAME}/v${VERSION}"

# Verify backup exists
if [ ! -d "$BACKUP_DIR" ]; then
  echo "✗ Backup not found: $BACKUP_DIR"
  exit 1
fi

# Remove current implementation
rm -rf "plugins/${PLUGIN_NAME}/Source"
rm -f "plugins/${PLUGIN_NAME}/CMakeLists.txt"

# Restore from backup
cp -R "$BACKUP_DIR/Source" "plugins/${PLUGIN_NAME}/"
cp "$BACKUP_DIR/CMakeLists.txt" "plugins/${PLUGIN_NAME}/"

# Rebuild
cd build && ninja "${PLUGIN_NAME}_VST3" "${PLUGIN_NAME}_AU"

echo "✓ Rolled back to v${VERSION}"
```

### Cleanup After Rollback

1. Update STATUS.yaml with rollback info
2. Keep improvement directory for reference
3. Remove activeMilestone from registry
4. Do NOT create version bump or tag

---

## Version Files to Update

### CMakeLists.txt

```cmake
project([PluginName] VERSION X.Y.Z)
```

Update the VERSION field.

### PluginProcessor.cpp (if version string present)

Some plugins display version in UI:

```cpp
// Search for patterns like:
static const juce::String version = "X.Y.Z";
// or
#define PLUGIN_VERSION "X.Y.Z"
```

Update if found.

### PLUGINS.md

Update table row:

```markdown
| [PluginName] | vX.Y.Z | ✅ Working | YYYY-MM-DD |
```
