# Phase 8: Repository Cleanup - Research

**Researched:** 2026-02-01
**Domain:** Git history rewriting, binary artifact removal, repository maintenance
**Confidence:** HIGH

## Summary

This phase addresses the repository size issue (currently 584MB in `.git`) caused by binary build artifacts committed to history. The standard approach uses `git-filter-repo` to rewrite history, removing `.o`, `.a`, `.dylib`, build outputs, and the entire `backups/` directory (209MB). A comprehensive `.gitignore` prevents re-accumulation.

The user has already decided on `git-filter-repo` (over BFG) due to its path filtering capabilities, which is the correct choice for this use case. The tool is officially recommended by the Git project as the replacement for the deprecated `git filter-branch`.

Key risks include: force-push coordination (mitigated by single developer environment), backup recovery needs (mitigated by permanent backup branch), and CI cache staleness (mitigated by no caching in current workflow).

**Primary recommendation:** Use `git-filter-repo --analyze` first, then `--path --invert-paths` for targeted removal, with a fresh clone and permanent backup branch.

## Standard Stack

The established tools for this domain:

### Core
| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| git-filter-repo | 2.47.0 | History rewriting | Official Git recommendation, replaces filter-branch |
| git | >= 2.36.0 | Version control | Required by git-filter-repo |
| Python 3 | >= 3.6 | Runtime for git-filter-repo | Dependency |

### Supporting
| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| git-sizer | latest | Repository analysis | Pre-cleanup to identify bloat |
| du / git count-objects | built-in | Size verification | Pre/post comparison |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| git-filter-repo | BFG Repo-Cleaner | BFG is faster for simple cases but lacks path filtering - user already chose git-filter-repo |
| git-filter-repo | git filter-branch | Deprecated, slower, more error-prone |

**Installation:**
```bash
brew install git-filter-repo
# Or: pip3 install git-filter-repo
```

## Architecture Patterns

### Cleanup Workflow Pattern

```
1. ANALYZE
   git-filter-repo --analyze

2. BACKUP (permanent branch)
   git checkout -b pre-cleanup-backup
   git push origin pre-cleanup-backup

3. FRESH CLONE (required by tool)
   git clone --mirror [repo] repo-cleanup
   cd repo-cleanup

4. DRY RUN (user approval)
   git-filter-repo --dry-run [options]
   # Review .git/filter-repo/fast-export.* files

5. EXECUTE
   git-filter-repo [options]

6. VERIFY
   du -sh .git
   git log --oneline | wc -l

7. FORCE PUSH
   git push --force --all
   git push --force --tags
```

### Pattern: Multi-Path Removal with git-filter-repo
**What:** Remove multiple path patterns in a single rewrite
**When to use:** Removing directories and file types together
**Example:**
```bash
# Source: git-filter-repo official documentation
git filter-repo \
  --path build/ --path backups/ \
  --path-glob '*.o' --path-glob '*.a' --path-glob '*.dylib' \
  --path .DS_Store \
  --invert-paths
```

### Pattern: Size-Based Blob Removal
**What:** Remove all files above a size threshold
**When to use:** Catching any large binaries missed by path patterns
**Example:**
```bash
# Source: git-filter-repo manpage
git filter-repo --strip-blobs-bigger-than 5M
```

### Pattern: Selective Recovery from Backup Branch
**What:** Cherry-pick specific files from backup branch
**When to use:** When a removed file is later needed
**Example:**
```bash
# Recover specific file from backup branch
git checkout pre-cleanup-backup -- path/to/needed/file

# Or view what was in backup
git show pre-cleanup-backup:path/to/file
```

### Anti-Patterns to Avoid
- **Running without fresh clone:** Tool refuses to run without `--force`, which exists to prevent accidents
- **Using `--force` habitually:** Bypasses safety checks, increases data loss risk
- **Skipping dry-run:** User requested dry-run approval as non-negotiable
- **Amending existing backup branch:** Create new backup, never modify it
- **Force-pushing without verification:** Always verify size reduction before push

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Find large files in history | bash loops over rev-list | git-filter-repo --analyze | Generates comprehensive reports automatically |
| Remove paths from history | git filter-branch | git-filter-repo | filter-branch is deprecated, slow, error-prone |
| Analyze repo bloat | Manual inspection | git-sizer | Identifies all categories of bloat systematically |
| .gitignore patterns | Guessing patterns | github/gitignore templates | Comprehensive, community-maintained templates |

**Key insight:** Git history rewriting has many edge cases (merge commits, tags, submodules, reflogs). The standard tools handle these correctly; custom scripts miss them.

## Common Pitfalls

### Pitfall 1: Remote Configuration Lost After Filter-Repo
**What goes wrong:** git-filter-repo removes `.git/config` remote settings
**Why it happens:** Intentional safety feature to prevent accidental push to wrong remote
**How to avoid:** Re-add remote after filtering: `git remote add origin [url]`
**Warning signs:** `git push` fails with "no upstream configured"

### Pitfall 2: Not Using Fresh Clone
**What goes wrong:** Tool refuses to run or `--force` causes data loss
**Why it happens:** git-filter-repo detects non-fresh clones to prevent accidents
**How to avoid:** Always work on a fresh `git clone --mirror` or `git clone`
**Warning signs:** "Refusing to run unless --force was specified"

### Pitfall 3: Forgetting Tags and All Branches
**What goes wrong:** Filtered repo still large because old branches/tags retain objects
**Why it happens:** Only current branch filtered by default
**How to avoid:** Filter operates on all refs by default; verify with `git branch -a` and `git tag`
**Warning signs:** Size not reduced as expected

### Pitfall 4: .gitignore Not Updated Before Next Commit
**What goes wrong:** Binary artifacts immediately re-committed after cleanup
**Why it happens:** Working directory still contains files that should be ignored
**How to avoid:** Update .gitignore BEFORE any other commits post-cleanup
**Warning signs:** `git status` shows build artifacts as untracked

### Pitfall 5: CI/CD Cache Contains Stale References
**What goes wrong:** CI builds fail with "object not found" or use wrong commits
**Why it happens:** GitHub Actions cache keys may reference old commit SHAs
**How to avoid:** Clear or invalidate caches after force push (this repo has no explicit caching)
**Warning signs:** CI failures mentioning missing objects or refs

### Pitfall 6: Collaborators With Old Clones
**What goes wrong:** Merge conflicts, duplicated commits, corrupted history
**Why it happens:** Local history diverges completely from rewritten remote
**How to avoid:** Single developer environment (no coordination needed); document recovery procedure
**Warning signs:** N/A for this repo (single developer)

## Code Examples

Verified patterns from official sources:

### Pre-Cleanup Analysis
```bash
# Source: git-filter-repo documentation
# Run from repository root
git filter-repo --analyze

# Reports generated in .git/filter-repo/analysis/
# - path-all-sizes.txt (files by total size across history)
# - directories-all-sizes.txt (directories by size)
# - extensions-all-sizes.txt (by file extension)
# - blob-shas-and-paths.txt (specific large blobs)
```

### Create Permanent Backup Branch
```bash
# Create backup from current state
git checkout -b pre-cleanup-backup-$(date +%Y%m%d)

# Push to remote (will survive force-push to main)
git push -u origin pre-cleanup-backup-$(date +%Y%m%d)

# Return to main
git checkout main
```

### Fresh Clone for Cleanup
```bash
# Mirror clone includes all refs
git clone --mirror git@github.com:user/repo.git repo-cleanup
cd repo-cleanup

# Or regular clone (also works)
git clone git@github.com:user/repo.git repo-cleanup
cd repo-cleanup
```

### Dry Run with Approval
```bash
# Source: git-filter-repo manpage
git filter-repo --dry-run \
  --path build/ --path backups/ \
  --path-glob '*.o' --path-glob '*.a' --path-glob '*.dylib' \
  --path-glob '*.DS_Store' \
  --invert-paths

# Review output in .git/filter-repo/
# - fast-export.original (before)
# - fast-export.filtered (after)
```

### Execute Cleanup
```bash
# Source: git-filter-repo documentation
git filter-repo \
  --path build/ --path backups/ \
  --path-glob '*.o' --path-glob '*.a' --path-glob '*.dylib' \
  --path-glob '*.DS_Store' \
  --invert-paths

# Re-add remote (removed by filter-repo)
git remote add origin git@github.com:user/repo.git
```

### Comprehensive .gitignore for CMake/C++/macOS
```gitignore
# Source: github/gitignore templates (CMake.gitignore, C++.gitignore, macOS.gitignore)

# === Build directories ===
build/
Build/
build-*/
cmake-build-*/
out/

# === CMake generated ===
CMakeLists.txt.user
CMakeCache.txt
CMakeFiles/
CMakeScripts/
cmake_install.cmake
install_manifest.txt
compile_commands.json
CTestTestfile.cmake
_deps/
CMakeUserPresets.json
Makefile

# === Compiled objects ===
*.o
*.obj
*.lo
*.slo

# === Static libraries ===
*.a
*.lib
*.la
*.lai

# === Dynamic libraries ===
*.so
*.so.*
*.dylib
*.dll

# === Precompiled headers ===
*.gch
*.pch

# === Debug/linker ===
*.ilk
*.pdb
*.dwo

# === Executables ===
*.exe
*.out
*.app

# === macOS ===
.DS_Store
__MACOSX/
.AppleDouble
.LSOverride
._*
.Spotlight-V100
.Trashes

# === Ninja ===
.ninja_deps
.ninja_log

# === IDE ===
.idea/
*.xcodeproj/
xcuserdata/
*.xcworkspace/

# === Project-specific ===
backups/
```

### Force Push After Verification
```bash
# Verify size reduction first
du -sh .git
# Should show < 100MB

# Verify history integrity
git log --oneline | head -20
git fsck --full

# Force push all branches and tags
git push --force --all origin
git push --force --tags origin
```

### Selective Recovery Script
```bash
#!/bin/bash
# recover-from-backup.sh
# Usage: ./recover-from-backup.sh path/to/file

BACKUP_BRANCH="pre-cleanup-backup-YYYYMMDD"  # Update with actual date
FILE_PATH="$1"

if [ -z "$FILE_PATH" ]; then
    echo "Usage: $0 <path/to/file>"
    echo "Available files in backup:"
    git ls-tree -r --name-only "$BACKUP_BRANCH" | head -50
    exit 1
fi

# Check if file exists in backup
if git ls-tree -r --name-only "$BACKUP_BRANCH" | grep -q "^${FILE_PATH}$"; then
    git checkout "$BACKUP_BRANCH" -- "$FILE_PATH"
    echo "Recovered: $FILE_PATH"
else
    echo "File not found in backup branch"
    echo "Similar files:"
    git ls-tree -r --name-only "$BACKUP_BRANCH" | grep -i "$(basename "$FILE_PATH")"
fi
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| git filter-branch | git-filter-repo | Git 2.22+ (2019) | filter-branch officially deprecated |
| BFG Repo-Cleaner | git-filter-repo | 2020+ | git-filter-repo more capable, maintained |
| Manual blob deletion | --strip-blobs-bigger-than | Always available | Automated large file detection |

**Deprecated/outdated:**
- `git filter-branch`: Officially deprecated, slow, many edge case bugs
- Manual reflog expiration: git-filter-repo handles automatically
- BFG for path-based filtering: Works but git-filter-repo has better path handling

## Repository-Specific Findings

### Current State
- Total .git size: 584MB
- Primary bloat: `.a` files (30-55MB each), multiple versions in history
- `backups/` folder: 209MB (entire folder to be removed)
- .gitignore: Nearly empty (only `.claude/system-config.json`)

### Largest Objects in History
Based on analysis:
1. `build/plugins/.../lib*_SharedCode.a` files (28-55MB each)
2. `plugins/OuariconAnalogEQ/build/...` artifacts
3. Multiple versions of same binaries from rebuilds

### Protected Paths (Essential Files)
These paths should NOT be filtered:
- `plugins/*/Source/` - All source code
- `plugins/*/CMakeLists.txt` - Build configuration
- `plugins/*/*.md` - Documentation
- `plugins/*/Presets/` - User presets (if any)
- `.planning/` - Planning documentation
- `.github/` - CI/CD configuration
- `CMakeLists.txt` - Root build config
- `CLAUDE.md`, `README.md`, `PLUGINS.md` - Project docs
- `research/` - Research documents

### CI/CD Impact
- Current workflow has NO explicit caching (no `actions/cache` usage)
- Cache concern is minimal - workflow downloads JUCE fresh each run
- Force push will not cause stale cache issues
- Only impact: workflow must pass on first run post-rewrite

## Open Questions

Things that couldn't be fully resolved:

1. **Exact file extensions in history**
   - What we know: .a, .o, .dylib are present
   - What's unclear: Are there other binary types (.vst3, .component) in history?
   - Recommendation: Run `git-filter-repo --analyze` to get complete picture before execution

2. **tache_plugins folder treatment**
   - What we know: Contains source files, appears to be active code
   - What's unclear: Should this be preserved or is it also backup material?
   - Recommendation: Preserve by default (contains .cpp files), user can clarify

## Sources

### Primary (HIGH confidence)
- [git-filter-repo GitHub](https://github.com/newren/git-filter-repo) - README, installation, design philosophy
- [git-filter-repo manpage (Debian)](https://manpages.debian.org/testing/git-filter-repo/git-filter-repo.1.en.html) - Complete command reference
- [GitHub gitignore templates](https://github.com/github/gitignore) - CMake.gitignore, C++.gitignore, macOS.gitignore
- Repository inspection: `.git` size, largest blobs, current .gitignore

### Secondary (MEDIUM confidence)
- [Git Tower git-filter-repo guide](https://www.git-tower.com/learn/git/faq/git-filter-repo) - Usage patterns verified with official docs
- [Marco Franssen blog](https://marcofranssen.nl/remove-files-from-git-history-using-git-filter-repo) - Practical examples
- [OneNine reduce repo size](https://onenine.com/how-to-reduce-git-repository-size-safely/) - Best practices
- [git-sizer GitHub](https://github.com/github/git-sizer) - Repository analysis tool

### Tertiary (LOW confidence)
- [DataCamp Git Push Force](https://www.datacamp.com/tutorial/git-push-force) - Team coordination patterns
- [GitHub Actions cache docs](https://github.com/actions/cache) - Cache invalidation (not directly applicable - no caching in this repo)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - git-filter-repo is officially recommended by Git project
- Architecture: HIGH - Patterns directly from official documentation
- Pitfalls: HIGH - Well-documented in multiple sources, verified with tool behavior

**Research date:** 2026-02-01
**Valid until:** 2026-03-01 (stable domain, git-filter-repo mature)
