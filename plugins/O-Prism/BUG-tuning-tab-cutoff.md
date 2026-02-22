# BUG: Tuning Tab Content Cut Off at Top

## Symptom
The Tuning tab in O-Prism shows only the viz-mode toggle buttons (Circle, Polar, Matrix, True Keys, Rotation) barely visible near the top of the tab, with the rest of the content area completely blank. The interval list, pitch circle visualization, and right-side controls panel are all invisible/cut off.

## Screenshot
User-provided screenshot shows: header, tab bar (SYNTH / **TUNING** / EFFECTS), then viz-mode buttons partially visible at very top of content area, then a large empty cream-colored space, then footer knobs.

## What Was Tried (Did NOT Fix It)
- Removed `position: relative` from `#tuning-tab` CSS rule (line ~339 of index.html)
- Rationale was that `position: relative` overrode `position: absolute` from `.tab-content` base class, collapsing the tab height since all children are absolutely positioned
- The fix looked correct in browser preview (localhost served via `python3 -m http.server`) but the problem **persists in the actual plugin WebView**

## Key Layout Architecture

### Parent chain:
```
body (flex column)
  .header-bar (flex-shrink: 0, h: 34px)
  .tab-bar (flex-shrink: 0)
  .tab-content-area (flex: 1, position: relative, overflow: hidden)
    #tuning-tab.tab-content (position: absolute, inset: 0)  <-- THIS IS THE PROBLEM AREA
  .footer-bar (flex-shrink: 0, h: 68px)
```

### Tuning tab children (ALL absolutely positioned):
1. `.tuning-viz-container` — left: 14px, top: 10px, bottom: 10px (interval list)
2. `.viz-mode-toggle` — left: calc(50% - 40px), top: 8px (Circle/Polar/Matrix buttons)
3. `.viz-container` — left: calc(50% - 40px), bottom: 40px, w: 220px, h: 210px (pitch circle/canvas)
4. `.tuning-controls-panel` — right: 14px, top: 10px, w: 200px (library, A4 ref, file ops)

### CSS rules involved:
- `.tab-content` (line ~90): `display: none; position: absolute; inset: 0; overflow-y: auto; padding: 10px 14px 6px;`
- `.tab-content.active` (line ~98): `display: block;`
- `#tuning-tab` (line ~339): Originally had `position: relative;` (was changed to comment but fix didn't work in plugin)

## Likely Root Cause Candidates
1. **The `#tuning-tab` position override** may need a different approach — perhaps the tab needs explicit dimensions or the children need non-absolute layout
2. **WebView rendering differences** — the fix worked in Chrome but not in the JUCE WebView (WebKit on macOS). Could be a WebKit-specific issue with `inset: 0` or flex layout
3. **The `.tab-content-area` may not have computed height** — if the flex layout isn't giving `.tab-content-area` proper height, then `inset: 0` on an absolutely-positioned child yields 0 height
4. **Padding on `.tab-content`** combined with absolute children — the `padding: 10px 14px 6px` may interact poorly with the absolute positioning

## File Location
- `plugins/O-Prism/Source/ui/public/index.html`
- Tuning tab CSS starts at ~line 338
- Tuning tab HTML starts at ~line 814

## Suggested Investigation
1. Add a visible debug border to `#tuning-tab` and `.tab-content-area` to see actual computed dimensions in WebView
2. Consider converting tuning tab layout from all-absolute to CSS Grid or flexbox
3. Test if other tabs (Synth, Effects) have the same parent height — they use normal flow so they work fine
4. Check if WebKit handles `inset: 0` differently than Chromium on `position: absolute` elements inside flex children

## Plugin Info
- **Plugin:** O-Prism v0.10.0
- **Status:** Installed
- **Date:** 2026-02-20
