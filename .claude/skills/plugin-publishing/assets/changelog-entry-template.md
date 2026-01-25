# Changelog Entry Template

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## Entry Structure

```markdown
## [X.Y.Z] - YYYY-MM-DD

### Added
- New feature or capability

### Changed
- Enhancement or modification to existing feature

### Fixed
- Bug fix or correction

### Breaking Changes
- Changes that affect backward compatibility

### Technical Notes
- Implementation details for developers
```

---

## Categorization Rules

### Added (New Features)
**Keywords:** new, add, introduce, implement, create, support

- New parameters or controls
- New presets or preset categories
- New waveforms, modes, or algorithms
- New UI elements or sections
- New format support (e.g., sidechain, MIDI)

**Examples:**
- "Added preset management system with factory and user presets"
- "New 'Shimmer' mode for extended reverb tails"
- "Introduced MIDI learn functionality for all parameters"

### Changed (Enhancements)
**Keywords:** improve, update, enhance, refactor, optimize, redesign, rename

- Performance improvements
- UI/UX enhancements
- Algorithm refinements
- Parameter range adjustments
- Behavior modifications

**Examples:**
- "Improved reverb algorithm for smoother tails"
- "Redesigned knob visuals with vintage aesthetic"
- "Optimized DSP for lower CPU usage"

### Fixed (Bug Fixes)
**Keywords:** fix, resolve, correct, repair, patch, eliminate

- Crash fixes
- Audio glitches or artifacts
- UI rendering issues
- Parameter synchronization bugs
- Preset loading/saving issues

**Examples:**
- "Fixed crash when loading presets on Windows"
- "Resolved audio dropout at high CPU load"
- "Corrected meter display not matching actual level"

### Breaking Changes
**Keywords:** remove, drop, incompatible, migration, restructure

- Removed features or parameters
- Changed parameter IDs (breaks automation)
- Preset format changes
- API modifications

**Examples:**
- "Removed legacy 'Vintage' mode (use 'Classic' instead)"
- "Parameter IDs restructured - existing automation may need re-recording"
- "Preset format updated - v1.x presets require manual migration"

---

## Technical Notes Section

Include when changes involve:

- DSP algorithm details
- Performance metrics
- Module integrations
- File format specifications
- API or callback changes

**Format:**
```markdown
### Technical Notes

- Implementation detail 1
- Implementation detail 2
- No breaking changes / Breaking change explanation
```

---

## Version Bump Guidelines

| Change Type | Version Bump | Example |
|-------------|--------------|---------|
| Bug fixes only | PATCH (x.y.Z) | 1.3.0 → 1.3.1 |
| New features (backward compatible) | MINOR (x.Y.0) | 1.3.1 → 1.4.0 |
| Breaking changes | MAJOR (X.0.0) | 1.4.0 → 2.0.0 |

---

## Commit Message Format

Release commits should follow:

```
release([PluginName]): vX.Y.Z

- Brief summary of main changes
- Additional notable changes

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```
