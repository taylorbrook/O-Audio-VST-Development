# Stage 4: Polish - Execution Plan

**Date:** 2026-02-18
**Plugin:** O-Prism (Microtonal Wavetable Synthesizer)
**Goal:** Fix 4 bugs, flatten effects UI, version bump to v0.9.0, verify build + pluginval

---

## Tasks

### 1. [ ] Fix noise generator clipping (Brown/Vinyl/Wind)
- **File:** `Source/dsp/NoiseGenerator.cpp`
- **Changes:**
  - Line 61: `return brownState * 3.5;` → `return std::tanh(brownState * 3.5);`
  - Line 91: `return vinyl;` → `return std::tanh(vinyl);`
  - Line 109: `return windLPState * 5.0;` → `return std::tanh(windLPState * 5.0);`
- **Add:** `#include <cmath>` if not present (check NoiseGenerator.h)
- **Depends on:** none
- **Risk:** Low — per-sample tanh is negligible CPU

### 2. [ ] Fix wavetable canvas blank display
- **File:** `Source/ui/public/index.html` (JS section, lines ~953-1062)
- **Changes:**
  - Replace silent catch at line 981: `catch(e) { /* ignore */ }` → `catch(e) { console.error('WT fetch:', e); }`
  - Add null guard in `fetchAndDraw()`: check `typeof this.getFrame === 'function'` before calling
  - Replace `setTimeout(refresh, 200)` with retry logic: try up to 5 times at 300ms intervals until data returns
  - Add `console.log` in refresh to verify `tableIdx` and `pos` values on first draw
- **Depends on:** none
- **Risk:** Low-Medium — JS only, no C++ changes

### 3. [ ] Fix filter routing dropdown alignment
- **File:** `Source/ui/public/index.html` (HTML, lines 499-509)
- **Changes:**
  - Remove the standalone `<div class="section">` wrapper around Filter Routing
  - Integrate routing dropdown into the `.inline-sections` container between Filter A and Filter B as a narrow center column:
    ```html
    <div class="inline-sections">
        <div class="inline-section"> <!-- Filter A --> </div>
        <div style="display:flex; align-items:center; padding-top:20px;">
            <div class="dropdown-group">
                <span class="dropdown-label">Filter Routing</span>
                <select id="select-filtRouting" class="param-select">
                    <option>Serial</option><option>Parallel</option>
                </select>
            </div>
        </div>
        <div class="inline-section"> <!-- Filter B --> </div>
    </div>
    ```
- **Depends on:** none
- **Risk:** Low — HTML layout only

### 4. [ ] Remove effects sub-tabs, show all effects at once
- **File:** `Source/ui/public/index.html`
- **CSS changes (lines 310-339):**
  - Remove `.effect-tab-bar`, `.effect-tab`, `.effect-tab:last-child`, `.effect-tab:hover`, `.effect-tab.active` rules
  - Remove `.effect-panel { display: none; }` and `.effect-panel.active { display: block; }`
  - Add: `.effect-section { margin-bottom: 12px; }` for vertical spacing between effect sections
- **HTML changes (lines 552-628):**
  - Remove the entire `.effect-tab-bar` div (lines 554-560)
  - Remove `class="effect-panel active"` from reverb div, replace with `class="effect-section"`
  - Remove `class="effect-panel"` from delay/chorus/distortion/eq divs, replace with `class="effect-section"`
- **JS changes (lines 668-673):**
  - Remove the `switchEffectTab()` function entirely
- **Depends on:** none
- **Risk:** Low — CSS/HTML/JS only, no parameter bindings change

### 5. [ ] Version bump to v0.9.0
- **File:** `CMakeLists.txt` line 12
- **Change:** `VERSION 0.1.0` → `VERSION 0.9.0`
- **Depends on:** Tasks 1-4 complete

### 6. [ ] Build, install, and validate
- **Commands:**
  ```bash
  cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && ninja O-Prism_VST3 O-Prism_AU
  ```
- **Install:** Clear AU cache, remove old plugins, install fresh (per CLAUDE.md)
- **Validate:** `pluginval --validate-in-process --strictness-level 10`
- **Depends on:** Task 5

### 7. [ ] Create CHANGELOG.md
- **File:** `plugins/O-Prism/CHANGELOG.md` (new)
- **Content:** Document v0.9.0 beta features:
  - Dual wavetable oscillators with 4 waveforms + unison
  - Sub oscillator + 6 noise types
  - Dual SVF filters (7 types each) with serial/parallel routing
  - ADSR envelopes (amp + filter)
  - 3 LFOs with multiple destinations
  - 5 effects (reverb, delay, chorus, distortion, 3-band EQ)
  - Full scala-tuning-engine integration (25 tunings + custom Scala import)
  - Glide (legato/always modes)
  - 16-voice polyphony
  - WebView UI with Naturalist aesthetic
- **Depends on:** Task 6 (after build passes)

---

## Success Criteria

- [ ] Brown/Wind/Vinyl noise types produce clean output without clipping
- [ ] Wavetable canvas displays waveforms for both Osc A and Osc B on load
- [ ] Filter Routing dropdown appears visually between Filter A and Filter B sections
- [ ] Effects tab shows all 5 effects in a single scrollable view (no sub-tab bar)
- [ ] CMakeLists.txt shows VERSION 0.9.0
- [ ] pluginval passes at strictness 10
- [ ] VST3 + AU build cleanly and install to system folders
- [ ] CHANGELOG.md documents v0.9.0 beta

---

## Files Modified

| File | Changes |
|------|---------|
| `Source/dsp/NoiseGenerator.cpp` | tanh() soft clip on 3 noise types |
| `Source/ui/public/index.html` | Canvas retry logic, filter routing layout, effects flat view, remove switchEffectTab |
| `CMakeLists.txt` | VERSION 0.1.0 → 0.9.0 |

## Files Created

| File | Purpose |
|------|---------|
| `CHANGELOG.md` | v0.9.0 beta release notes |

---

## Execution Order

Tasks 1-4 are independent (can be parallelized). Task 5 follows. Task 6 validates everything. Task 7 is documentation after passing.

```
[1: Noise fix]  ──┐
[2: Canvas fix] ──┤
[3: Filter UI]  ──┼──→ [5: Version bump] → [6: Build+Validate] → [7: CHANGELOG]
[4: Effects UI] ──┘
```
