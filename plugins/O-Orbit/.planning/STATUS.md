---
plugin: O-Orbit
stage: improve
phase: v1.4.2 PATCH — paper halo under canvas text over the shell
status: v1_4_2_canvas_halo_built_installed_daw_check_pending
last_updated: 2026-09-26
version: 1.4.2
next_brief: (none — known follow-ups: R7 keyboard/ARIA knobs, Elevation pill thumb covers first glyph, English-only canvas captions)
previous_versions: 1.0.0, 1.0.1, 1.1.0, 1.1.1, 1.2.0, 1.2.1, 1.2.2, 1.3.0, 1.4.0, 1.4.1
---

# Resume Point

## Current State: v1.4.2 built, installed — DAW visual check pending

`fillTextHalo()` in `js/app.js`: a 3px `#F5E6D3` `strokeText` under FRONT/REAR, ELEV, the hint line, L/R letters and elevation badges.
Glyph-pixel probe (2×, text removed, p5): hint 3.17→5.66, R on a shell stripe 3.41→5.88, the rest 4.47–5.50 → 5.64–6.71.
Backup: `backups/O-Orbit/v1.4.1/`.

## Next Steps

1. Open O-Orbit-dev in a DAW: hint line and L/R letters readable over the shell; the halo should not read as an outline on the plain plate. Plus the v1.4.1 checks below.

## Previous: v1.4.1

### v1.4.1 built, installed, auval PASS — DAW visual check pending

`#botanical-overlay` z-index 1000 → -1: the shell now sits under all text, veiled by panel washes.
Pixel probe (2×, text hidden, p5): Phase 3.81→9.83, Export 3.45→8.88, Import 3.50→8.80, Del 3.44→8.56, Save 3.89→8.56; 0/68 rows worse.
Gates: check-ui-labels en/fr/zh-Hans PASS, check-i18n PASS, measure-ui --contrast 0/67. Backup: `backups/O-Orbit/v1.4.0/`.

## Next Steps

1. Open O-Orbit-dev in a DAW: shell visible under the controls, toolbar and Phase readable; plus the v1.4.0 checks below.

## Previous: v1.4.0

### v1.4.0 built, installed, auval PASS — DAW visual check pending

UI-only pass per `.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md`
Phase C. No DSP / parameter / state change. Backup: `backups/O-Orbit/v1.3.0/`.

- `measure-ui.js --contrast`: 6/67 → 0/67 below AA per language, <9px 1 → 0
- `check-ui-labels.js` en/fr/zh-Hans: ALL CHECKS PASSED; `check-i18n.js`: ALL PASS
- Resize 600×450 / 800×600 / 1600×1200: no horizontal overflow (toolbar scroll at 600 is D4 by design)
- Resolved face (CDP): EB Garamond on all Latin nodes, PingFang SC on Han

## Next Steps

1. Open O-Orbit-dev in a DAW: check the font, focus rings (Tab through header/popover/toolbar), lit Elevation pill, preset menu.
2. Decide the botanical-overlay layering (it sits above text at z-index 1000).
3. R7 knob keyboard/ARIA as its own pass if wanted.

Stage and v1.1 brief history: `.planning/stages/*/`, `.planning/improvements/v1.1-review-findings.md`, `CHANGELOG.md`.
