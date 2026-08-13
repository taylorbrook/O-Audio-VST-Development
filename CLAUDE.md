# VST Development Project Guidelines

## Implementation Knowledge (auto-loaded skills)

- **Spike findings for VST-development** (implementation patterns, constraints, gotchas) → `Skill("spike-findings-VST-development")`

## Build Requirements

### CRITICAL: Plugin Cache Clearing
**Every time you build a VST3 or AU plugin, you MUST:**
1. Clear the macOS AU cache BEFORE installing
2. Remove old plugin binaries from system folders
3. Install fresh binaries to system plugin folders

```bash
# Always run this sequence after any ninja build of plugins:
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Remove old AND alternate-variant bundles before install (-dev ↔ unsuffixed)
# Why: dev branding produces "<Name>-dev.{vst3,component}" while release branding
# produces "<Name>.{vst3,component}" — same AU triple (type/subtype/manufacturer).
# Leaving the alternate variant on disk pins Logic's registry slot to whichever
# was installed first. See O-Prism v1.17.4 CHANGELOG for the regression.
rm -rf ~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3
rm -rf ~/Library/Audio/Plug-Ins/VST3/[PluginName]-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/[PluginName].component
rm -rf ~/Library/Audio/Plug-Ins/Components/[PluginName]-dev.component

# Install fresh — substitute the suffix actually produced by your build:
#   - Dev branding (default local):   [PluginName]-dev
#   - Release branding (CI only):     [PluginName]
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[PluginName]*.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/AU/[PluginName]*.component ~/Library/Audio/Plug-Ins/Components/
```

**Preferred:** use `./scripts/build-and-install.sh [PluginName]` — its Phase 4 already does the dual-variant sweep automatically and emits a `⚠ Sweeping ALTERNATE-variant` warning when an orphan is found.

### Windows Plugin Management
**On Windows, AU is not available. Only VST3 is built and installed.**

```powershell
# Build and install on Windows:
.\scripts\build-and-install.ps1 [PluginName]

# Remove old and install fresh (manual)
Remove-Item -Recurse -Force "$env:COMMONPROGRAMFILES\VST3\[PluginName].vst3"
Copy-Item -Recurse "build\plugins\[PluginName]\[PluginName]_artefacts\Release\VST3\[PluginName].vst3" "$env:COMMONPROGRAMFILES\VST3\"

# Clear Ableton cache on Windows
Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" -Force -ErrorAction SilentlyContinue
```

### Build Targets

**macOS** (VST3 + AU):
```bash
ninja [PluginName]_VST3 [PluginName]_AU
```

**Windows** (VST3 only):
```powershell
cmake --build build --config Release --target [PluginName]_VST3 --parallel
```

## Testing Requirements
- Always test in DAW after installation
- **macOS:** Verify AU appears with `auval -a | grep -i [pluginname]`
- **Windows:** Verify VST3 appears in DAW plugin scanner (Ableton, FL Studio, Reaper, etc.)
- If plugin shows stale behavior, close DAW completely and restart

## CRITICAL: Phase/Stage Completion Handoffs

**After completing ANY plugin workflow phase or stage, you MUST present a two-step handoff message and STOP.**

The handoff format is:
1. Show what was completed (phase name, artifacts created)
2. Present "Next Up" with **Step 1:** `/clear` and **Step 2:** the next command with full plugin name
3. List alternative options
4. **STOP** — do NOT auto-invoke the next phase/command

This applies to ALL of these commands: `/plugin-discuss`, `/plugin-research`, `/plugin-plan`, `/plugin-execute`, `/plugin-verify`, `/plugin-handoff`, `/implement`.

### MANDATORY: Use Specific Next Phase Commands (Manual Mode)

**When using individual phase commands, Step 2 MUST be the exact next phase command — NOT `/implement`.**

| After | Step 2 (Next Command) |
|-------|----------------------|
| `/plugin-discuss [Name]` | `/plugin-research [Name]` |
| `/plugin-research [Name]` | `/plugin-plan [Name]` |
| `/plugin-plan [Name]` | `/plugin-execute [Name]` |
| `/plugin-execute [Name]` | `/plugin-verify [Name]` |
| `/plugin-verify [Name]` (mid-stage) | `/plugin-discuss [Name]` (next stage) |
| `/plugin-verify [Name]` (Stage 4) | `/install-plugin [Name]` |

**No exceptions.** Every phase completion = copy-paste-ready slash command for the next phase.

See `.claude/references/handoff-protocol.md` for the full format specification.

**If you forget: the user MUST see `/clear` as Step 1 and the specific next phase command as Step 2 before you stop.**

## Parallel Plugin Development

Multiple plugins are routinely worked on at the same time. These rules keep that work isolated.

### One branch per plugin

Every plugin gets its own branch, and every branch is cut from `main` — never from another plugin's feature branch. An improve branch cut off a second plugin's feature branch strands that plugin's release behind unrelated work.

```bash
git switch main
git switch -c improve/o-someplugin-v1.2
```

### One worktree per concurrently-developed plugin

Two plugins in flight at once means two worktrees, not one checkout being switched back and forth. A worktree keeps each plugin's build directory, installed bundles, and dirty files separate.

### Worktree naming: `VST-development-<slug>`

Worktree directories are named `VST-development-<slug>` where `<slug>` identifies the plugin or feature (for example `VST-development-octagon` for `feat/o-octagon`). The worktree `VST-contrabass-v1.1` broke this convention — it dropped the `VST-development-` prefix — and was removed.

### Worktrees live OUTSIDE the repo

Create worktrees as **siblings** of the repo inside `~/Dev`, never inside the repo tree. A worktree nested under the repo shows up as an untracked path in `git status`, pollutes every `git add -A`, and can be swept by cleanup tooling.

```
~/Dev/VST-development/            <- the repo
~/Dev/VST-development-octagon/    <- sibling worktree, correct
```

### PLUGINS.md is a shared registry — only edit YOUR plugin's row

`PLUGINS.md` is a single global registry table with one row per plugin, and every plugin branch edits it. The repo root `.gitattributes` maps it to git's built-in `union` merge driver so parallel-branch row edits merge without a conflict.

Two caveats, stated honestly:

- **Union merge trades a conflict for a possible DUPLICATE row.** When two branches touch the same hunk, union keeps both sides' lines rather than flagging them. Editing another plugin's row is exactly what turns a clean merge into a duplicated or contradictory row — so only ever edit your own plugin's row.
- **The merge attribute is read from the working tree of the branch being merged INTO.** `main` must carry `.gitattributes` for the driver to apply at all; a branch that has it while `main` does not gets a plain conflict.

### Worktree commands

```bash
# Create a worktree + branch for a new plugin effort (cut from main)
git worktree add ../VST-development-<slug> -b <branch> main

# See what is currently checked out where
git worktree list

# Tear down when the branch is merged
git worktree remove ../VST-development-<slug>
git branch -d <branch>          # -d refuses if unmerged; never use -D
```

## Project Structure
- Plugins are in `plugins/[PluginName]/`
- Build output is in `build/plugins/[PluginName]/[PluginName]_artefacts/Release/`
- Working directory for builds: `build/` (relative to project root)
- **Research documents** go in `research/` (NOT `docs/`) — includes algorithm references, market research, technical deep-dives
- **Licensing integration** for a plugin: `/add-licensing {PluginFolder} {product-id}`
