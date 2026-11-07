# Stage 4 — Polish · Hands-on checklist (Standalone, real WebView)

**Plugin:** O-Granulator 1.0.0 · **Run at:** Stage 4 **verify**, by Taylor (PLAN-4 P161). Not run in execute.
**Why a human:** the harness proves the preset values, renders and state (PR0–PR5, ST1–ST3), and PF1 proves the 16 names fit the name box in EN / FR / zh-Hans in the served page. Only the real WebView shows the menu grouping, the ellipsis behaviour and a language switch with the menu open.

**Host:** the Standalone (`O-Granulator-dev.app`). Install first: `./scripts/build-and-install.sh O-Granulator`.
**Pre-step (R143):** `ls ~/Library/O-Granulator/Presets/Factory` → the 16 current files, `.factory-version` → `1.0.0` (see DAW-4.md for the fix if not).

Mark each item PASS / FAIL / NOTE. A FAIL gets a one-line note.

| # | Item | Expect | FAIL consequence | Verdict |
|---|---|---|---|---|
| H1 | Open the preset menu | 16 factory presets, grouped by prefix in the sorted list (Ambient ×4, Cascade ×4, Harmoniser ×4, Rhythmic ×4), the `·` separator renders (no tofu / `?`) | FUNC-09 (naming) | PASS |
| H2 | Select the widest name, `Harmoniser · 19-EDO Haze` | The name shows in full in the preset bar (PF1: 150 / 172 px); no ellipsis, no overlap with ◀ ▶ or Save | UI-04 (shorten the name, never widen the box) | PASS |
| H3 | Save a user preset with a long name (30+ characters) | The bar shows it with a clean ellipsis; the menu row is readable | UI-04 | PASS |
| H4 | Switch the language EN → FR → zh-Hans with the preset menu **open** | The menu stays usable (no layout jump, no clipped rows); the preset names stay as they are (not localised); the Factory / User chip translates | UI-04 | PASS |
| H5 | ◀ ▶ through all 16 | Each step loads the next preset; the bar name and the knobs update together | FUNC-09 | PASS |
| H6 | Load a factory preset, change Output Gain to −7 dB, load another | Output Gain stays at −7 dB (presets never set it) | FUNC-09 / feedback_presets_never_set_output_gain | PASS |

## Verdict

HANDS-ON-4 PASS + DAW-4 PASS → QUAL-04 complete at Stage 4 verify.

**Result (2026-11-07, Taylor): H1–H6 all PASS** — HANDS-ON-4 PASS.
