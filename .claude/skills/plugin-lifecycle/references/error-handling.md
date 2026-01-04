# Error Handling

**Context:** This file is part of the plugin-lifecycle skill.
**Invoked by:** Installation/uninstallation processes when errors occur
**Purpose:** Comprehensive error scenarios and troubleshooting guidance

---

## Build Files Not Found

```
Release build not found.

Expected locations:
- VST3: build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[Product].vst3
- AU: build/plugins/[PluginName]/[PluginName]_artefacts/Release/AU/[Product].component

This usually means:
1. Plugin hasn't been built yet
2. Build was in Debug mode (need Release)
3. Build failed (check previous errors)

To build manually:
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel

Then rerun: /install-plugin [PluginName]

Or use: /test [PluginName] build (builds + validates in one step)
```

## Permission Denied

```
Permission denied writing to system folders.

Error: cp: cannot create directory '~/Library/Audio/Plug-Ins/VST3/[Product].vst3': Permission denied

This usually means:
1. System folder doesn't exist (create it first)
2. System folder not writable (check permissions)
3. Disk is read-only (rare on macOS)

Fixes to try:

1. Create directories:
   mkdir -p ~/Library/Audio/Plug-Ins/VST3
   mkdir -p ~/Library/Audio/Plug-Ins/Components

2. Check permissions:
   ls -ld ~/Library/Audio/Plug-Ins/VST3
   # Should show: drwxr-xr-x (755)

3. Fix permissions:
   chmod 755 ~/Library/Audio/Plug-Ins/VST3
   chmod 755 ~/Library/Audio/Plug-Ins/Components

4. Check disk space:
   df -h ~
   # Ensure "Available" > 100 MB

After fixing, rerun: /install-plugin [PluginName]
```

## Cache Clear Failed

```
⚠️  Warning: Cache clear failed (not critical)

Ableton Live: Permission denied deleting PluginCache.db
Logic Pro: AudioComponentRegistrar not found

Plugin installed successfully, but DAW caches may be stale.

Manual fixes:

Ableton Live:
1. Quit Ableton Live
2. Manually delete: ~/Library/Preferences/Ableton/Live */PluginCache.db
3. Relaunch Ableton Live

Logic Pro:
1. Quit Logic Pro
2. Open Terminal, run: killall AudioComponentRegistrar
3. Delete cache: rm -rf ~/Library/Caches/AudioUnitCache/*
4. Relaunch Logic Pro

Or simply restart your DAW - it should rescan automatically.
```

## Plugin Doesn't Appear in DAW

```
Plugin installed but not appearing in DAW?

Troubleshooting checklist:

1. Restart DAW:
   Many DAWs only scan plugins at launch. Quit and relaunch.

2. Check DAW scan paths:
   Verify DAW is scanning ~/Library/Audio/Plug-Ins/VST3 and /Components

   Logic Pro: Should scan automatically (Audio Units)
   Ableton Live: Preferences → Plug-Ins → VST3 Custom Folder
   Reaper: Preferences → Plug-ins → VST → Re-scan

3. Check plugin format:
   Logic Pro: Uses AU only (ignore VST3)
   Ableton Live: Prefers VST3 over AU
   Reaper: Uses both VST3 and AU

4. Check for load errors:
   Open Console.app (macOS utility)
   Search for "[ProductName]" or "AudioComponentRegistrar"
   Look for error messages

5. Verify installation:
   ls -la ~/Library/Audio/Plug-Ins/VST3/[Product].vst3
   ls -la ~/Library/Audio/Plug-Ins/Components/[Product].component

   Check permissions: Should be drwxr-xr-x (755)

6. Try manual rescan:
   Most DAWs: Preferences → Plugins → Rescan or "Clear cache and rescan"

7. Check for Gatekeeper block:
   System Preferences → Security & Privacy → General
   Look for: "[Product] was blocked from use because it is not from an identified developer"
   Click: "Open Anyway"

If still not working, share Console.app errors for debugging.
```

---

**Return to:** Main plugin-lifecycle orchestration in `SKILL.md`
