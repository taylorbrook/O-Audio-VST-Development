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

### Build Targets
When building plugins, always build both formats:
```bash
ninja [PluginName]_VST3 [PluginName]_AU
```

## Testing Requirements
- Always test in DAW (Logic Pro, Ableton, etc.) after installation
- Verify AU appears with `auval -a | grep -i [pluginname]`
- If plugin shows stale behavior, close DAW completely and restart

## Project Structure
- Plugins are in `plugins/[PluginName]/`
- Build output is in `build/plugins/[PluginName]/[PluginName]_artefacts/Release/`
- Working directory for builds: `/Users/taylorbrook/Dev/VST-development/build`
