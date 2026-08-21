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

Multiple plugins in flight at once is normal and expected. **This project is trunk-based: all plugin work happens on `main`, in the single checkout at `~/Dev/VST-development`.** Isolation comes from path discipline — each plugin owns `plugins/<Name>/` — not from branches or worktrees.

The reason is that the workflow tooling resolves plugin state from fixed paths (`plugins/<Name>/.planning/STATUS.md`) and has no reader for branch or worktree. A feature branch makes STATUS.md branch-versioned, so a fresh session on `main` reads a stale file that still looks current and silently redoes or clobbers finished work.

### Commit discipline for concurrent sessions

Two sessions in one checkout share `.git/index` **and** HEAD. Another session's staging can join your commit in the gap between the moment you check and the moment you commit.

Two rules, both mandatory:

1. **Path-scope every commit.** Name the paths explicitly. Never `git add -A`, never `git commit -a`.
   ```bash
   git commit -- plugins/<Name> PLUGINS.md
   ```
2. **Re-check location and staging immediately before every commit**, not once at session start:
   ```bash
   git branch --show-current
   git status --short
   ```

A session-start snapshot is not good enough — it can be minutes stale. The SessionStart hook's Git Context is a starting picture, not a commit-time guarantee.

### Rollback without branches

Per-plugin undo does not need a per-plugin branch. Three mechanisms, all path-scoped:

- **Backup snapshots** — `/improve` writes `backups/<Plugin>/vX.Y.Z/` before it changes anything.
- **Release tags** — named `vX.Y.Z-<PluginName>`, version first with the plugin name as the suffix (for example `v3.1.1-O-Bells`).
- **Path-scoped restore** — pull one plugin's tree back to a known state without touching anything else:
  ```bash
  git restore --source=v3.1.1-O-Bells -- plugins/O-Bells
  ```

**`git revert` takes no pathspec.** `git revert -n <sha> -- plugins/<Name>` fails with `fatal: bad revision`. `restore` is the path-scoped tool; `revert` is not.

### Branches are for exceptional repo-wide work only

A branch is justified only when a change is repo-wide and risky enough that `main` should not carry it mid-flight — a framework version bump, a cross-plugin removal sweep. **Never for a single plugin's feature or fix.**

When one is genuinely warranted: cut it from `main`, keep it short-lived, and merge and delete it within the same working period.

The SessionStart hook prints the current branch and worktree count at every session start, so a stray branch or a leftover worktree surfaces after every `/clear` instead of silently stranding work. Worktrees are no longer part of the routine workflow — if the hook reports more than one, remove it with `git worktree remove` once its branch is merged.

### PLUGINS.md union merge (exceptional branches only)

The root `.gitattributes` maps `PLUGINS.md` to git's built-in `union` merge driver. Under trunk-based development there are no routine merges, so this only fires when an exceptional branch merges back.

**Union merge trades a conflict for a possible DUPLICATE row.** When two sides touch the same hunk, union keeps both sides' lines rather than flagging them. This fires even when each side edited ONLY its own row — adjacent rows fall in the same diff hunk (proven on the first live merge, 2026-08-13: two single-row branches produced four rows). After any merge that touches PLUGINS.md, run the duplicate check and keep the newest row:

```bash
grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d
```

Empty output means clean.

## Project Structure
- Plugins are in `plugins/[PluginName]/`
- Build output is in `build/plugins/[PluginName]/[PluginName]_artefacts/Release/`
- Working directory for builds: `build/` (relative to project root)
- **Research documents** go in `research/` (NOT `docs/`) — includes algorithm references, market research, technical deep-dives
- **Licensing integration** for a plugin: `/add-licensing {PluginFolder} {product-id}`
