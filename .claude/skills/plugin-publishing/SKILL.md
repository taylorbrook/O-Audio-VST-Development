---
name: plugin-publishing
description: Cross-platform plugin distribution via GitHub Actions CI/CD. Use when user requests to publish a release, distribute cross-platform, create GitHub releases, or mentions releasing/versioning plugins for public download. Invoked by /publish command or natural language like 'publish OuariconTremolo' or 'release GainKnob 1.5.0'.
---

# plugin-publishing Skill

**Purpose:** Create cross-platform releases via GitHub Actions CI/CD, enabling users to distribute plugins (macOS, Windows, Linux) through GitHub Releases.

## Overview

Automates the release workflow: version bump, changelog update, git tagging, and triggering cloud builds. Artifacts are built by GitHub Actions runners and published to GitHub Releases automatically.

## Workflow

<critical_sequence enforcement="strict" blocking="true">

**Track your progress:**

```
Plugin Publishing Progress:
- [ ] 1. Prerequisites verified (status OK, git clean, remote exists)
- [ ] 2. Version determined (current → next)
- [ ] 3. Changelog entry created (Added/Changed/Fixed)
- [ ] 4. Files updated (CMakeLists.txt, CHANGELOG.md, PLUGINS.md)
- [ ] 5. Changes committed (release commit)
- [ ] 6. Tag created and pushed ({PluginName}-v{VERSION})
- [ ] 7. User notified (Actions URL, release tracking)
```

---

### 1. Verify Prerequisites

**Run these checks in sequence:**

```bash
# Check git status (must be clean)
git status --porcelain

# Check remote exists
git remote get-url origin
```

**Read PLUGINS.md** and verify:
- Plugin entry exists
- Status is 📦 Installed or ✅ Working

**Blocking conditions:**
- If git has uncommitted changes → "Commit or stash changes first"
- If no remote → "Add remote: git remote add origin <url>"
- If plugin not ready → Guide to /implement or /continue

**Also verify GitHub Actions workflow exists:**
- Check `.github/workflows/build-and-release.yml` exists
- If missing, offer to create it from template (see Step 1a)

### 1a. Create GitHub Actions Workflow (First-Time Setup)

If `.github/workflows/build-and-release.yml` doesn't exist:

```
This is your first release! I'll create the GitHub Actions workflow for cross-platform builds.

This workflow will:
- Build for macOS (Universal Binary - Intel + Apple Silicon)
- Build for Windows (VST3)
- Build for Linux (VST3)
- Create GitHub Release with all artifacts

Proceed with workflow setup? (Y/n)
```

If yes: Copy template from `assets/workflow-templates/build-and-release.yml` to `.github/workflows/`, commit with message "ci: add cross-platform build workflow".

---

### 2. Determine Version

**Extract current version:**
```bash
grep "VERSION" plugins/[PluginName]/CMakeLists.txt | head -1
```

Parse format: `VERSION X.Y.Z`

**If --version flag provided:**
- `--version patch` → X.Y.(Z+1)
- `--version minor` → X.(Y+1).0
- `--version major` → (X+1).0.0

**If no flag, prompt user:**
```
Current version: 1.3.0

What type of release is this?
1. Patch (1.3.1) - Bug fixes, small tweaks
2. Minor (1.4.0) - New features, enhancements
3. Major (2.0.0) - Breaking changes, major overhaul

Choose (1-3):
```

**Store:** `CURRENT_VERSION`, `NEW_VERSION`

---

### 3. Create Changelog Entry

**Prompt for changes:**
```
Describe the changes in this release:
(What was added, changed, or fixed?)

>
```

**After user provides description, categorize:**

Read `assets/changelog-entry-template.md` for categorization rules:
- **Added** - New features, capabilities, presets
- **Changed** - Enhancements, UI updates, behavior changes
- **Fixed** - Bug fixes, crash fixes, corrections
- **Breaking Changes** - API changes, preset incompatibility

**Present draft for approval:**
```
## [1.4.0] - 2026-01-24

### Fixed
- Fixed preset loading crash on Windows
- Resolved audio dropout at high CPU load

### Changed
- Improved reverb algorithm for smoother tails

Does this look correct? (Y/edit/cancel)
```

**If edit:** Let user modify the draft
**If cancel:** Abort publish workflow

---

### 4. Update Files

**4a. Update CMakeLists.txt VERSION:**
```bash
# Replace VERSION line
sed -i '' "s/VERSION [0-9]*\.[0-9]*\.[0-9]*/VERSION $NEW_VERSION/" plugins/[PluginName]/CMakeLists.txt
```

**4b. Prepend to CHANGELOG.md:**
- Read existing content
- Insert new entry after header (after "adheres to Semantic Versioning" line)
- Write back

**4c. Update PLUGINS.md:**
- Find plugin row in registry table
- Update Version column to new version
- Update Last Updated column to today's date

**Validation:** Verify all three files modified before proceeding.

---

### 4b. Microtonal Cohort SMOKE-TEST (Advisory)

If the plugin being published is in the **microtonal cohort**, run a Dorico smoke-test sweep via `dorico-agent` and surface results as a warning block in the publish summary. **This is advisory — does NOT block release.**

**Cohort:** O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-MicrotonalSampler.

**Trigger:** `[PluginName]` matches one of the cohort entries above.

**Handoff:**
```
Task(subagent_type="dorico-agent",
     description="[PluginName] pre-release SMOKE-TEST",
     prompt="Run TC-1..TC-5 from plugins/[PluginName]/Resources/dorico/SMOKE-TEST.md against the v[NEW_VERSION] build. Report PASS/FAIL per TC. Do not edit anything; this is a validation pass only.")
```

**Result handling:**
- **All TC pass** → continue to Step 5, no warning shown.
- **Any TC fails** → append an "⚠ Dorico SMOKE-TEST advisory" block to the publish summary listing the failing TCs and the agent's diagnosis. **Continue to Step 5 anyway** — release proceeds. User decides follow-up.

**Reference docs:** `.claude/agents/dorico-agent.md` (Output Contract section); per-plugin `Resources/dorico/SMOKE-TEST.md`.

---

### 5. Commit Changes

**Create release commit:**
```bash
git add plugins/[PluginName]/CMakeLists.txt
git add plugins/[PluginName]/CHANGELOG.md
git add PLUGINS.md

git commit -m "$(cat <<'EOF'
release([PluginName]): v[NEW_VERSION]

- [Summary of changes from changelog]

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
EOF
)"
```

---

### 6. Create and Push Tag

**Tag format:** `{PluginName}-v{VERSION}` (e.g., `OuariconTremolo-v1.4.0`)

```bash
# Create annotated tag
git tag -a "[PluginName]-v[VERSION]" -m "Release [PluginName] v[VERSION]"

# Push commit and tag
git push origin HEAD
git push origin "[PluginName]-v[VERSION]"
```

**This triggers the GitHub Actions workflow** which will:
1. Parse tag to extract plugin name and version
2. Build on macOS, Windows, and Linux runners
3. Create GitHub Release with artifacts

---

### 7. Notify User

**Display completion summary:**
```
✓ [PluginName] v[VERSION] published!

Release workflow triggered:
→ https://github.com/[owner]/[repo]/actions

When builds complete (~10-15 min), artifacts available at:
→ https://github.com/[owner]/[repo]/releases/tag/[PluginName]-v[VERSION]

Expected artifacts:
- [PluginName]-[VERSION]-macos.tar.gz (VST3 + AU Universal)
- [PluginName]-[VERSION]-windows.zip (VST3)
- [PluginName]-[VERSION]-linux.tar.gz (VST3)
```

</critical_sequence>

---

## Decision Menu

<decision_gate type="checkpoint" enforcement="strict">

After successful publish, present menu and WAIT for user response:

```
✓ [PluginName] v[VERSION] published!

What's next?
1. Monitor build status → Open GitHub Actions in browser
2. Publish another plugin → /publish [OtherPlugin]
3. View release page → Open GitHub Releases in browser
4. Continue development → Return to coding
5. Other

Choose (1-5):
```

**Option handlers:**
1. **Monitor builds** → Provide direct link to Actions run
2. **Publish another** → Prompt for plugin name, re-invoke skill
3. **View release** → Provide release page URL
4. **Continue** → End workflow, ready for next task
5. **Other** → Open-ended response

</decision_gate>

---

## Integration Points

**Invoked by:**
- `/publish [PluginName]` command
- `/publish [PluginName] --version [patch|minor|major]`
- Natural language: "Publish OuariconTremolo", "Release GainKnob 1.5"

**Complements:**
- `plugin-packaging` - Local PKG installer (offline distribution)
- `plugin-publishing` - Cloud CI/CD releases (online distribution)
- `plugin-improve` - Shares versioning logic (backup-based vs release-based)

**Reads:**
- `PLUGINS.md` → Plugin status, current version
- `plugins/[PluginName]/CMakeLists.txt` → VERSION extraction
- `plugins/[PluginName]/CHANGELOG.md` → Existing entries
- `.claude/branding.json` → Company metadata
- `.github/workflows/build-and-release.yml` → Workflow existence check

**Writes:**
- `plugins/[PluginName]/CMakeLists.txt` → Updated VERSION
- `plugins/[PluginName]/CHANGELOG.md` → New release entry
- `PLUGINS.md` → Updated version and date
- `.github/workflows/build-and-release.yml` → Created on first publish
- Git → Release commit and tag

---

## Error Handling

**Build failures:**
If GitHub Actions fails, user can:
1. Check Actions logs for error details
2. Fix locally and re-publish with `--version patch` (or same version if tag deleted)
3. Delete failed tag: `git push origin :refs/tags/[PluginName]-v[VERSION]`

**Tag already exists:**
```
Error: Tag [PluginName]-v[VERSION] already exists

Options:
1. Delete and recreate tag (if release failed)
2. Bump to next version
3. Cancel

Choose (1-3):
```

**Network/push failures:**
- Verify remote access: `git remote -v`
- Check authentication: `gh auth status`
- Retry push

---

## Success Criteria

Publishing succeeds when:
- ✅ Version bumped in CMakeLists.txt
- ✅ Changelog entry added with categorized changes
- ✅ PLUGINS.md updated with new version and date
- ✅ Release commit created
- ✅ Tag pushed to remote
- ✅ GitHub Actions workflow triggered
- ✅ User has tracking URLs

**NOT required for success:**
- GitHub Actions completing (async process)
- User downloading artifacts
- Code signing/notarization (Phase 2)

---

## Notes for Claude

**When executing this skill:**

1. **Always verify git state first** - Clean working directory is critical
2. **Confirm version bump type** - Don't assume, ask if not specified
3. **Categorize changes properly** - Use keywords from template
4. **Use exact tag format** - `{PluginName}-v{VERSION}` (no spaces)
5. **Provide actionable URLs** - User needs to monitor progress
6. **Handle first-time setup** - Create workflow file if missing

**Version logic:**
- PATCH = bug fixes, no new features
- MINOR = new features, backward compatible
- MAJOR = breaking changes, API changes

**Tag naming rationale:**
- Includes plugin name because repo has multiple plugins
- Allows selective builds via tag pattern matching
- Format: `OuariconTremolo-v1.4.0`, `GainKnob-v2.0.0`
