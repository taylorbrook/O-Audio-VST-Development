# VST Development Project Guidelines

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

# Remove old and install fresh
rm -rf ~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/[PluginName].component
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[PluginName].vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/AU/[PluginName].component ~/Library/Audio/Plug-Ins/Components/
```

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

## Project Structure
- Plugins are in `plugins/[PluginName]/`
- Build output is in `build/plugins/[PluginName]/[PluginName]_artefacts/Release/`
- Working directory for builds: `build/` (relative to project root)
- **Research documents** go in `research/` (NOT `docs/`) — includes algorithm references, market research, technical deep-dives
- **Licensing integration** for a plugin: `/add-licensing {PluginFolder} {product-id}`
