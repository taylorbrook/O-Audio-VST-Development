# Stage 3 — GUI · Phase 3.1 — Hands-on checklist (real WebView)

**Plugin:** O-Granulator · **Phase:** 3.1 (Layout + controls + bindings)
**Run at:** 3.1 **verify**, by Taylor (PLAN-3.1 P112; RESEARCH-3.1 R113 layer 3). Not run in execute.
**Why a human:** Playwright cannot attach to WKWebView, and the AX / CGWindow route steals focus. The automated layers prove the page binds all 220 ids both ways under a C++-seeded stub (`tests/ui_binding_probe.js`) and that the four ID tables agree by ID and type (`tests/ui_frontend_check.js`). Only this pass proves JUCE's relay ↔ attachment plumbing inside the real WebView.

**Hosts:** the Standalone (`O-Granulator-dev.app`) and **one** DAW (Logic or Reaper). Install first: `./scripts/build-and-install.sh O-Granulator`.

Mark each item PASS / FAIL / NOTE. A FAIL gets a one-line note (what you did, what you saw).

---

## A. Load and look (checklist §4, §7)

| # | Item | Expect | Verdict |
|---|---|---|---|
| A1 | Open the editor (Standalone and the DAW) | 1200 × 800, not resizable, page drawn (not blank, not white) | |
| A2 | Right-click → Inspect Element (Standalone): Console | No JavaScript errors; `window.__JUCE__` exists | |
| A3 | Inspector → Network, reload the page | No 404; `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/preset-manager.js`, `css/eb-garamond.css`, three `.woff2` all 200 | |
| A4 | Every knob shows a value after load | No `—` left anywhere | |
| A5 | Canvas text (figures, buffer strip captions) | EB Garamond, not the fallback serif | |
| A6 | Default readouts (checklist §6) | Size `100 ms`, Density `10 gr/s`, Cutoff `20.0 kHz`, Ring `200 Hz`, FB Low Cut `80 Hz`, FB High Cut `12.0 kHz`, LFO Rate `0.50 Hz`, Length `8.0 s`, Env Attack `10.0 ms` | |

## B. One control of each kind, on each page (UI → host)

Watch the DAW's own parameter display (or the generic editor in Reaper) while you move each control.

| # | Page | Control (kind) | Expect | Verdict |
|---|---|---|---|---|
| B1 | STREAMS | S1 Position (knob): drag, arrow keys, wheel, double-click | Host value follows; double-click returns to default | |
| B2 | STREAMS | Sync = Tempo, then S2 Size division (stepped choice knob): arrow keys | Steps one division per key; host shows the division string | |
| B3 | STREAMS | S3 Scale (grouped trigger): pick 19-EDO from the list | Host `S3 · Scale` = 19-EDO; chips redraw | |
| B4 | STREAMS | S1 Mute (toggle) | Host toggles; button pressed state follows | |
| B5 | FX | S1 Filter type (segment row): BP | Host = BP | |
| B6 | FX | Reverb mix (knob) | Host follows | |
| B7 | MOD | Mod 1 source / destination (selects) + depth (fader) | Host follows all three; the row lights | |
| B8 | MOD | LFO 2 shape (segment), LFO 2 rate (knob); Tempo → LFO 2 rate division | Host follows | |
| B9 | header | Freeze (toggle), Grain limit (select), Scale root (select) | Host follows | |
| B10 | any | Drag a source chip onto a knob | First empty slot gets source / destination / +40 %; ring + row flash | |

## C. Host → UI (automation, one lane per relay type)

| # | Lane | Expect | Verdict |
|---|---|---|---|
| C1 | `S1 · Position` (WebSliderRelay) — draw a ramp, play | Knob and readout follow the lane | |
| C2 | `S1 · Filter Type` (WebComboBoxRelay) — step lane | Segment row follows | |
| C3 | `Freeze` (WebToggleButtonRelay) — on / off lane | Button + strip outline (ochre) + "FROZEN" head follow | |
| C4 | `Mod 1 · Dest` (combo, select) | Select follows | |

## D. Presets (preset bar + scale state + seed)

| # | Item | Expect | Verdict |
|---|---|---|---|
| D1 | Set a custom scale (name "Test 7", text `120 270 540 670 785 950 1200`), mask two chips on S1 (Custom), change a few knobs; Save preset "HO-1" | Saved; appears in the list | |
| D2 | Change everything (other scale, other knobs); load "HO-1" | Knobs, custom scale NAME on the trigger, the editor text, and the chips all come back (scale-revision re-pull) | |
| D3 | Seed: with "HO-1" loaded, bounce / listen 5 s; load another preset, load "HO-1" again, same input | Same grain texture (the seed travels in the preset) | |
| D4 | Set Output to −12 dB, load "HO-1" (saved at 0 dB) | Output stays at −12 dB (excluded from presets) | |
| D5 | Delete "HO-1"; prev / next arrows | Bar behaves; no errors | |

## E. Session persistence — TWO reopens (critical_preset_manager_stale_customstate_child)

| # | Item | Expect | Verdict |
|---|---|---|---|
| E1 | Load a preset; save the DAW project; close; reopen | Everything as saved (params, custom scale, masks) | |
| E2 | Change one mask chip; save; close; reopen again | The CHANGED state comes back (not the preset's) | |

## F. Robustness

| # | Item | Expect | Verdict |
|---|---|---|---|
| F1 | Close and reopen the editor ×10 (DAW) | No crash, page drawn every time | |
| F2 | Remove / re-insert the plugin ×5 | No crash | |
| F3 | Hide the plugin window, keep playing 10 s, show it | Page still live (no blank, telemetry resumes) | |

## G. Hands-on (checklist §9)

| # | Item | Expect | Verdict |
|---|---|---|---|
| G1 | Tab through the page | Every knob reachable; arrows move it; division knobs step one division | |
| G2 | Drag a knob out of the window, release outside | Gesture ends; the knob stops following | |
| G3 | Double-click resets (incl. division knobs and matrix depth bars → 0 %) | | |
| G4 | Hover-help switch: off, close editor, reopen | Still off (persists) | |
| G5 | Language fr, then zh-Hans | Relabels, no clipping — all three tabs, the scale list, the custom-scale editor | |
| G6 | macOS Reduce Motion on | No animation (knob hover scale, drop flash, matrix row flash, toggles, tooltip fade) | |
| G7 | Scales: list arrows / Home / End / Enter / Escape; outside press closes; Custom opens the editor; chips toggle + paint-drag; All / None / Invert; 53-EDO = 2 rows; Load .scl fills a draft; Apply commits; Save .scl writes a valid file | | |
| G8 | Freeze: strip outline ochre, head "FROZEN", "writing stopped · N s held" | | |
| G9 | **R108 watch (sign-off 3):** S1 scale = 53-EDO, paint-drag across many chips quickly | No visible chip flicker | |

## H. Expected 3.1 stub artefacts — NOT defects (RESEARCH-3.1 §3)

These are filled in 3.2 / 3.3. Do not fail 3.1 verify on them:

- Per stream: **"0 sounding"**; header **"0 / 128"** (limit falls back to the Grain limit choice).
- Loop gain reads **"Open"**.
- The LFO dots sit at the **left edge** (phase 0); the LFO value (dot height) and the dock chip tints DO move.
- Onset history empty; onset pill never flashes; **level lane flat**.
- Buffer strip: **no waveform, no grains**; "writing stopped · N s held" shows the Buffer length knob, not the held span.
- Output meter flat.

Live in 3.1 (they should move): mod-source chip tints and LFO dot heights, env history, pitch note / cents / clarity bar (feed a pitched input), mod-ring live dots on routed knobs, the window figure (S1–S4 window / skew), the scale chips and custom-scale name.

---

**Result:** ___ / ___ PASS · Notes:
