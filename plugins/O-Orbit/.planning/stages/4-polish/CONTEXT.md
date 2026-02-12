# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-02-11
**Participants:** User, Claude

## Arriving From Stage 3

Stage 3 (GUI) is fully verified:
- All 17 parameters bound via WebView with Ouaricon Botanical/Naturalist aesthetic
- Canvas orbital visualizer: 60fps, source dots, path trails, speaker icons
- Speaker layout editor: drag/add/remove, presets, export/import JSON
- Downmix badge, custom layout persistence
- All 3 targets build with zero O-Orbit source warnings

## Stage 4 Scope

**Polish Level:** Standard
- pluginval validation (strictness level 10)
- AU validation (auval)
- Plugin state persistence verification
- Build cleanup (warnings, edge cases)

**Presets:** Full preset pack (8-12 factory presets)

## Requirements for Stage 4

### Validation
1. **pluginval:** Pass strictness level 10 for VST3
2. **auval:** Pass AU validation (`auval -v aufx OuOr Ouar`)
3. **State persistence:** Save/restore plugin state correctly (all 17 parameters + custom speaker layout)
4. **No audio glitches:** Clean audio under all parameter changes

### Factory Presets
Create 8-12 presets covering different use cases:

**Stereo presets (work in any DAW):**
- Slow Orbit (gentle circular pan)
- Fast Spiral (energetic orbital motion)
- Pendulum Swing (side-to-side rhythmic motion)
- Drift (ambient random movement)
- Tempo-synced Quarter Note

**Surround presets (for multi-channel setups):**
- 5.1 Orbit (wide circular motion through 5.1)
- 7.1.4 Height Sweep (uses elevation for 3D)
- Quad Drift (random motion through 4 speakers)

**Creative presets:**
- L+R Split Wide (stereo input, opposite orbits)
- Deep Space (max distance, high air absorption)
- Tight Focus (narrow width, close distance)
- Rhythmic Bounce (tempo-synced pendulum)

### Build Cleanup
- Verify zero warnings from O-Orbit source code
- Ensure all file paths are correct in CMakeLists.txt
- Verify cross-platform definitions (WebView2 static linking, user data folder)

## Approach

1. Run pluginval first to catch any issues
2. Fix any pluginval failures
3. Run auval for AU validation
4. Create factory presets via C++ (getNumPrograms/getProgramName/setCurrentProgram)
5. Test state persistence (save/load in DAW)
6. Final build verification

## Constraints

- No audio thread changes needed (DSP is complete)
- Presets are parameter snapshots only (no custom speaker layouts in presets)
- pluginval may flag multi-channel bus negotiation — handle gracefully
- Factory presets implemented via AudioProcessor programs API

## Next Phase

Ready for: **research** phase
