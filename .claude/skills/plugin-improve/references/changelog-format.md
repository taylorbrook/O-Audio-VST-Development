# Enhanced CHANGELOG Format

## Template Structure

Add new version entry at top with technical details:

```markdown
# Changelog

All notable changes to this plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this plugin adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [VERSION] - YYYY-MM-DD

### Added
- **Feature name:** Brief description
  - Technical detail 1
  - Technical detail 2
  - Impact: What this enables or improves

### Changed
- **Component name:** What changed
  - Before: Previous behavior
  - After: New behavior
  - Reason: Why this change was made

### Fixed
- **Issue:** What was broken
  - Root cause: Technical explanation
  - Solution: How it was fixed
  - Affects: Which parameters/features

### Breaking Changes (MAJOR versions only)
- **API change:** What's incompatible
  - Migration: How to update existing uses
  - Deadline: When old behavior will be removed

### Migration Notes (MAJOR versions only)
**From vX.Y.Z to vA.B.C:**
1. Update parameter mappings
2. Rebuild host applications
3. Test automation compatibility

### Technical Details
- Files modified: [list]
- DSP changes: [algorithm updates]
- Parameter changes: [APVTS modifications]
- Testing: [regression test results]

### Dependencies
- JUCE version: X.Y.Z
- Minimum macOS: X.Y
- Minimum Windows: X.Y
```

## Section Usage Guide

### For PATCH Versions (0.0.X)
**Use:** "Fixed" section primarily
**Include:**
- Root cause analysis
- Technical solution
- Affected components

**Example:**
```markdown
## [1.2.3] - 2025-11-12

### Fixed
- **Reverb tail cutoff:** Decay was stopping abruptly at 80% threshold
  - Root cause: Incorrect envelope calculation in DampingProcessor
  - Solution: Fixed exponential decay curve math
  - Affects: Decay Time and Damping parameters
```

### For MINOR Versions (0.X.0)
**Use:** "Added" and "Changed" sections
**Include:**
- Feature description
- Technical implementation notes
- Parameter additions/modifications

**Example:**
```markdown
## [1.3.0] - 2025-11-12

### Added
- **Pre-delay control:** Adjustable delay before reverb onset (0-200ms)
  - New parameter: Pre-Delay (predelay)
  - Implementation: Ring buffer with linear interpolation
  - Impact: Enables early reflection simulation

### Changed
- **Room size range:** Extended from 0-100% to 0-200%
  - Before: Small to medium rooms only
  - After: Small rooms to cathedral spaces
  - Reason: User request for larger spaces
```

### For MAJOR Versions (X.0.0)
**Use:** All sections including "Breaking Changes" and "Migration Notes"
**Include:**
- Complete changelog
- Incompatible changes documented
- Step-by-step migration guide

**Example:**
```markdown
## [2.0.0] - 2025-11-12

### Breaking Changes
- **Parameter IDs renamed:** Standardized to camelCase format
  - Migration: Update automation mappings in DAW
  - Old → New: room_size → roomSize, decay_time → decayTime
  - Deadline: v1.x support ends 2026-01-01

### Migration Notes
**From v1.x to v2.0:**
1. Export automation data from DAW
2. Replace old parameter IDs with new (see mapping above)
3. Re-import automation
4. Test all presets

### Added
- **Freeze function:** Capture current reverb tail and hold indefinitely
  - New parameter: Freeze (freeze)
  - Implementation: Buffer capture with crossfade
  - Impact: Creative sound design capability
```

## Best Practices

1. **Always include dates:** Use ISO format (YYYY-MM-DD)
2. **Link issues:** Reference GitHub issues if applicable
3. **Technical depth:** Include enough detail for developers to understand changes
4. **User impact:** Explain what users will notice
5. **Testing notes:** Document regression test results
6. **Version links:** Add comparison links at bottom of file

## Version Comparison Links

At the bottom of CHANGELOG.md, add:

```markdown
[Unreleased]: https://github.com/user/repo/compare/v1.2.3...HEAD
[1.2.3]: https://github.com/user/repo/compare/v1.2.2...v1.2.3
[1.2.2]: https://github.com/user/repo/compare/v1.2.1...v1.2.2
```
