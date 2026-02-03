# Bloom Fine Controls - Advanced Per-Band Control

## Summary
Add an expandable "Bloom Fine Controls" section that allows independent control of bloom speed and amount for each partial band (Low, Mid, High).

## Current State (v1.4.1)
- **Bloom Speed** (0-100%): Controls duration for all bands uniformly
  - Low: 15-250ms, Mid: 25-400ms, High: 50-800ms
- **Bloom Amount** (0-100%): Controls intensity for all bands uniformly
  - Low: 0-40%, Mid: 0-60%, High: 0-90%

## Proposed Enhancement

### New Parameters (6 total)
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `bloomSpeedLow` | 0-100% | 50% | Speed for partials 0-1 |
| `bloomSpeedMid` | 0-100% | 50% | Speed for partials 2-4 |
| `bloomSpeedHigh` | 0-100% | 50% | Speed for partials 5-7 |
| `bloomAmountLow` | 0-100% | 0% | Amount for partials 0-1 |
| `bloomAmountMid` | 0-100% | 0% | Amount for partials 2-4 |
| `bloomAmountHigh` | 0-100% | 0% | Amount for partials 5-7 |

### UI Design
- Add expandable "Bloom Fine Controls" section below current Bloom Speed/Amount sliders
- When collapsed: Main Bloom Speed/Amount control all bands
- When expanded: Per-band sliders override main controls (or main acts as master scaling all bands)
- Visual: Collapsible panel with subtle border, similar to Multi-Stage Envelope section styling

### Interaction Options
**Option A - Override Mode:**
- Main sliders disabled when fine controls expanded
- Per-band sliders take full control

**Option B - Master + Offset Mode:**
- Main sliders act as "master" baseline
- Per-band sliders add/subtract from master value
- More intuitive for quick adjustments

### Files to Modify
- `BellVoice.h` - Add 6 new currentBloomSpeed/Amount per-band variables
- `BellVoice.cpp` - Modify `initializeBloom()` to use per-band values when active
- `PluginProcessor.h` - Add 6 new parameter pointers
- `PluginProcessor.cpp` - Add 6 new parameters to layout, update voice calls
- `PluginEditor.h` - Add 6 new relays/attachments
- `PluginEditor.cpp` - Create bindings for all new parameters
- `Resources/ui/index.html` - Add expandable section with 6 sliders, collapse/expand logic

### Version
This would be v1.5.0 (MINOR - new feature with additional parameters)

## Notes
- Consider preset compatibility: new params should default to values that match current behavior
- UI state (expanded/collapsed) could be saved with plugin state or reset on load
