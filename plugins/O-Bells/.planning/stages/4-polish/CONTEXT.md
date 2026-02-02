# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-02-02
**Participants:** User, Claude

---

## Requirements Confirmed

### Testing Scope
- **Full validation** - pluginval + manual DAW testing + stress testing
- DAWs: Logic Pro, Ableton (user's standard workflow)
- Stress test: 8 voices, full ensemble (4 unison × 3 octave layers)
- CPU target: <60% single core worst case

### Preset System
- **Preset bar module** - Match O-Lyrica style with enhancements
- **Subgrouping** - Folder-based categories (not flat list)
- **Comprehensive pack** - 20+ presets covering all BRIEF categories
- Position: Top header bar (title left, preset controls right)

### Release Target
- **Personal use** - Local installation, no public distribution
- No branded installer needed at this time
- Skip signing/notarization requirements

### Known Issues
- **None found** - Stage 3 WebView and DSP verified working

---

## Constraints Identified

1. **Preset module enhancement required** - Current preset-manager module uses flat list; needs folder-based category support
2. **Module compatibility** - Must work with existing OuariconPresetManager.h interface
3. **UI integration** - Preset bar must fit O-Bells header (800x600 window, Ouaricon Naturalist theme)
4. **No breaking changes** - O-Lyrica and other plugins using preset-manager should continue working

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset categories | Folder-based in Factory/User dirs | Standard pattern, filesystem organization, no JSON schema changes |
| Preset bar style | O-Lyrica style with category headers | Consistent Ouaricon aesthetic, proven implementation |
| Header layout | Title (left) - Preset bar (right) | Matches user's specification, clean layout |
| Testing depth | Full validation (pluginval + DAW + stress) | Ensures quality before personal use |
| Release scope | Personal use only | Skip packaging/distribution complexity |

---

## Preset Categories (from BRIEF)

Based on BRIEF.md preset categories:

| Category | Description | Target Count |
|----------|-------------|--------------|
| Orchestral | Tubular bells, chimes, glockenspiels | 4-5 presets |
| Sacred | Church bells, carillons, meditation bowls | 4-5 presets |
| World | Gamelan, Tibetan bowls, ethnic percussion | 4-5 presets |
| Ambient | Evolving pads, frozen shimmer, drones | 4-5 presets |
| Cinematic | Tension risers, horror stingers, epic swells | 4-5 presets |

**Total:** 20-25 factory presets across 5 categories

---

## Stage 4 Deliverables

### 1. Preset System Integration

**Files to Create/Modify:**
- `Source/OuariconPresetManager.h` - Copy from modules, add category support
- `Source/PluginProcessor.h/cpp` - Add presetManager member, factory preset initialization
- `Source/PluginEditor.h/cpp` - Add preset native functions, update WebView options
- `Resources/ui/index.html` - Add preset bar to header section

**C++ Changes:**
- Add `OuariconPresetManager presetManager` to processor
- Initialize factory presets with category subfolders
- Register native functions: `getPresetList`, `loadPreset`, `savePreset`, `selectNext/Previous`, etc.
- Add `getPresetCategories()` for grouped listing

**UI Changes:**
- Header layout: `[Title: O-Bells] ... [◀] [Preset Name ▼] [▶] [Save] [Load]`
- Dropdown with category headers (Orchestral, Sacred, World, Ambient, Cinematic)
- Ouaricon Naturalist styling (Garamond, earth tones, aged paper background)

### 2. Factory Presets (20-25 presets)

**Category Structure:**
```
~/Library/O-Bells/Presets/
├── Factory/
│   ├── Orchestral/
│   │   ├── Tubular Bells.json
│   │   ├── Concert Chimes.json
│   │   ├── Glockenspiel.json
│   │   └── Celesta Mallet.json
│   ├── Sacred/
│   │   ├── Church Bell.json
│   │   ├── Cathedral Carillon.json
│   │   ├── Meditation Bowl.json
│   │   └── Temple Gong.json
│   ├── World/
│   │   ├── Gamelan Saron.json
│   │   ├── Gamelan Bonang.json
│   │   ├── Tibetan Bowl.json
│   │   └── Steel Pan.json
│   ├── Ambient/
│   │   ├── Frozen Shimmer.json
│   │   ├── Bell Pad.json
│   │   ├── Crystal Drone.json
│   │   └── Ethereal Chime.json
│   └── Cinematic/
│       ├── Epic Bell.json
│       ├── Tension Chime.json
│       ├── Horror Stinger.json
│       └── Dramatic Swell.json
└── User/
```

### 3. Validation Testing

**pluginval:**
- Run with `--strictness-level 5` (standard)
- Ensure no failures on state save/restore
- Verify parameter automation

**DAW Testing (Logic Pro):**
- [ ] Plugin loads without blank WebView
- [ ] All 18 parameters respond to UI controls
- [ ] DAW automation updates UI in real-time
- [ ] Preset save/load works
- [ ] Preset navigation (prev/next) works
- [ ] State saves correctly with project
- [ ] No crashes during normal operation

**Stress Test:**
- Play 8-note chord with full ensemble (4 unison, 100% octave blend)
- Monitor CPU usage (<60% target)
- Rapid preset switching (no crashes)
- Parameter automation stress test

### 4. Installation

**Local installation only:**
```bash
# Clear AU cache and install
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bells.component
cp -R build/plugins/O-Bells/O-Bells_artefacts/Release/VST3/O-Bells.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Bells/O-Bells_artefacts/Release/AU/O-Bells.component ~/Library/Audio/Plug-Ins/Components/
```

---

## Open Questions

None - all requirements clarified during discussion.

---

## Next Phase

Ready for: **research** phase

Research will investigate:
1. Folder-based preset category implementation in OuariconPresetManager
2. O-Lyrica preset bar HTML/CSS/JS patterns for adaptation
3. Factory preset parameter values for each bell archetype

---

*Discussion completed: 2026-02-02*
