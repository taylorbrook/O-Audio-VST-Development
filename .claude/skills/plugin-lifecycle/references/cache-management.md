# DAW Cache Management

**Context:** This file is part of the plugin-lifecycle skill.
**Invoked by:** Installation and uninstallation processes
**Purpose:** Clear DAW caches so they recognize newly installed/removed plugins

---

## Overview

DAWs cache plugin metadata (name, parameters, version) for fast scanning. After installation or removal, caches must be cleared.

## Ableton Live

**Cache location:**

```bash
~/Library/Preferences/Ableton/Live */PluginCache.db
```

**Clear cache:**

```bash
rm -f ~/Library/Preferences/Ableton/Live\ */PluginCache.db
```

**Explanation:**

- `Live */` matches all Live versions (Live 10, Live 11, Live 12)
- `PluginCache.db` is an SQLite database of scanned plugins
- After deletion, Live rescans on next launch

**Verification:**

```bash
ls ~/Library/Preferences/Ableton/Live\ */PluginCache.db 2>/dev/null
# Should return nothing (file deleted)
```

## Logic Pro

**Cache location:**

```bash
~/Library/Caches/AudioUnitCache/
```

**Clear cache + restart registration:**

```bash
# Delete cache files
rm -rf ~/Library/Caches/AudioUnitCache/*

# Kill AudioComponentRegistrar (macOS service that indexes Audio Units)
killall AudioComponentRegistrar 2>/dev/null || true
```

**Explanation:**

- `AudioUnitCache/` contains indexed AU metadata
- `AudioComponentRegistrar` is a macOS background service that maintains AU database
- Killing it forces macOS to rebuild AU database on next access
- `2>/dev/null` suppresses error if process not running
- `|| true` prevents script failure if killall returns non-zero

**Verification:**

```bash
# Check if cache cleared
ls ~/Library/Caches/AudioUnitCache/
# Should be empty or show only new cache files

# Check if AudioComponentRegistrar restarted
ps aux | grep AudioComponentRegistrar
# Should show new process (different PID)
```

## Other DAWs

**Reaper:**
- No persistent cache (scans on launch)
- No cache clearing needed

**Pro Tools:**
- Cache location: `~/Library/Application Support/Avid/Audio/Plug-Ins/`
- AAX plugins (future support)

**Cubase/Nuendo:**
- Preferences → Plug-in Manager → Reset
- Manual rescan (no command-line access)

## Cache Clearing Report

**After clearing all caches:**

```
✓ Cache clearing complete

Ableton Live: PluginCache.db deleted (all versions)
Logic Pro: AudioUnitCache cleared, AudioComponentRegistrar restarted

Next steps:
1. Launch your DAW
2. Rescan plugins (or restart DAW)
3. Plugin should appear as "[Product Name]"
```

**If cache clearing fails:**

```
⚠️  Warning: Cache clearing failed (not critical)

Ableton Live: Permission denied
Logic Pro: AudioComponentRegistrar not running

Plugin installed successfully, but DAW caches may be stale.
Manual fix: Restart your DAW to pick up new plugin.
```

---

**Return to:** Installation or uninstallation process in `SKILL.md`
