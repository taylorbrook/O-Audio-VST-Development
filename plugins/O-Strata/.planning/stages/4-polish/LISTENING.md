# O-Strata — Listening pass (QUAL-04)

**Plugin:** O-Strata · **Stage:** 4 (Polish / Validation), Round B · **Requirement:** QUAL-04
**Material prepared:** 2026-09-13 · **Code sha at preparation:** `26599292` (Phase 4.2a) · **Verifier:** Taylor
**Hosts:** Standalone `build/plugins/O-Strata/O-Strata_artefacts/Release/Standalone/O-Strata-dev.app` (the install script does not install the Standalone — memory `pattern_build_install_skips_standalone_stale_ui`) · Logic Pro (AU, installed `O-Strata-dev.component`) · Ableton Live 12 (VST3, installed `O-Strata-dev.vst3`)
**Device:** ______________________  ·  **Monitoring:** ______________________  ·  **Date of sitting:** ____________

> Every verdict cell below is **blank on purpose**. QUAL-04 is Taylor's sign-off; the executor prepares the material and never fills a verdict.

## Vocabulary

| Verdict | Meaning | Where it goes |
|---|---|---|
| **PASS** | sounds as intended, nothing to do | nowhere — the row is done |
| **NOTE** | a real characteristic worth documenting, but not worth changing | CHANGELOG → *Known limits* (never a code change) |
| **CHANGE** | a constant should move (a default, a range, a law) | one CHANGE per commit, after the sitting — see *CHANGE protocol* below |
| **FAIL** | a bug | a bug fix with the full Round B gate list (PLAN Task 9, B1–B20) re-run |

## Sitting order (one session)

1. **Standalone** — Table C row 2 (hands-on), then Table C row 1 (the ⌥-click perf burst; tick a MIDI input under Options → Audio/MIDI Settings first).
2. **Logic Pro (AU)** — Table A Logic column, plus Table C rows 3 (⌘Z) and 15 (PERF-03 Logic half).
3. **Ableton Live 12 (VST3)** — Table A Live column.
4. **The WAV grid** — Table B, by `afplay` loop or on a Logic audio track.

---

## Table A — the 18 factory presets, in two hosts

Render references (post-FX, note 60 velocity 1.0, 2 s held + 1 s release) are in `plugins/O-Strata/tests/exports/presets/` — the same numbering. The three **Bandlimited** presets carry an extra cell: hold a chord above C6 and listen for the muting behaviour (`silent above <note>` in the readout is correct, a hash or a whistle is not).

| # | Preset | Category | Terrain × Orbit | What to listen for | Logic | Live 12 | Chord above C6 | Note |
|---|---|---|---|---|---|---|---|---|
| 01 | Init | Init | Sine Product × Ellipse | all defaults — the reference timbre; nothing should sound wrong here |  |  | — |  |
| 02 | Breathing Pad | Pads | Sine Product × Ellipse | two slow LFOs breathe Orbit Size and Centre X; listen for a smooth swell with no stepping on the size sweep |  |  | — |  |
| 03 | Chatter Lead | Lead | Ridged Cosines × Epitrochoid 5 | Feedback 0.4 / Damp 0.3 and ModWheel → Terrain Freq; push the wheel and listen for growl without a squeal or a runaway |  |  | — |  |
| 04 | Pierce Bell | Keys | Cosine Wells × Hypocycloid 3, F 1.5 | **Bandlimited** — a clean bell with no aliasing hash on the decay; wants the Tuning tab at Bohlen-Pierce |  |  |  |  |
| 05 | Your Terrain Here | FX | Mitsuhashi × Limaçon | the drop-a-PNG demo: it must sound like a terrain, then drop a greyscale PNG on the view and hear it change |  |  | — |  |
| 06 | PD Organ | Keys | Saddle × Squarcle | the filter envelope sweeps Rotation 90° — listen for a phase-distortion-like opening, not a filter sweep |  |  | — |  |
| 07 | Radial Keys | Keys | Radial Rings × Superellipse, F 1.5 | saturation 0.2 with key-tracked LP24; listen for bite that stays even across the keyboard |  |  | — |  |
| 08 | Folded Bass | Bass | Ridged Cosines × Hypocycloid 5, F 0.7 | Coarse −12 with Feedback 0.25 / Damp 0.6; listen for fold without losing the fundamental |  |  | — |  |
| 09 | Wells Drone | Drone | Cosine Wells × Epitrochoid 7, F 2.0 | **Bandlimited** — a very slow Mod Y drift under a long reverb; listen for the drift to be continuous, not stepped |  |  |  |  |
| 10 | Saddle Pluck | Pluck | Saddle × Hypocycloid 7, F 3.0 | short envelope, velocity → cutoff; listen for a consistent attack across velocities |  |  | — |  |
| 11 | Butterfly Choir | Pads | Mitsuhashi × Butterfly | unison 3 / detune 0.3 / width 0.8 — the widest patch in the bank; listen for a stable centre image in mono fold-down |  |  | — |  |
| 12 | Limacon Lead | Lead | Sine Product × Limaçon, F 2.0 | Orbit Mod 0.7 + saturation 0.4 into soft distortion; listen for the lead to cut without fizz |  |  | — |  |
| 13 | Squarcle Storm | FX | Radial Rings × Squarcle | centre (0.23, 0.21), Feedback 0.6 / Damp 0.2, S&H LFO on Centre Y; listen for chaos that never goes silent or blows up |  |  | — |  |
| 14 | Glass Rings | Keys | Radial Rings × Hypocycloid 3, F 2.5 | **Bandlimited** — long bright decay; listen for no aliasing sparkle high on the keyboard |  |  |  |  |
| 15 | Mitsuhashi Organ | Keys | Mitsuhashi × Ellipse | the deliberate centred case (odd terrain → odd harmonics, h1 present); listen for an organ, not an octave-up |  |  | — |  |
| 16 | Superellipse Bass | Bass | Cosine Wells × Superellipse | Coarse −12, LP24 at 900 Hz with envelope; listen for a solid low end with no thump from the DC blocker |  |  | — |  |
| 17 | Epi Keys | Keys | Sine Product × Epitrochoid 3, F 1.5 | Orbit Mod 0.6 + velocity → Orbit Mod; listen for the timbre to open with velocity, evenly |  |  | — |  |
| 18 | Saddle Sweep | Sequence | Saddle × Epitrochoid 7 | LFO1 synced 1/2 → Terrain Freq; listen for the sweep to lock to the host tempo and loop seamlessly |  |  | — |  |

**Verdict cells:** 18 × 2 host cells = 36, plus 3 chord cells (Pierce Bell, Wells Drone, Glass Rings) = **39 blank verdicts**.

---

## Table B — the terrain × orbit grid

The 66 pairs at C4 (2 s, defaults, Quality 2×) plus the flagged C2 / C6 pairs. Files are `plugins/O-Strata/tests/exports/<Terrain>-<Orbit>-<Note>.wav`, written by `--gate export` and pinned by `tests/render-harness/golden/round-a-grid.sha256` (198 checksums, LF; `shasum -a 256 -c` passes with no `tr`).

### B-1 · the 66 C4 rows

| # | Terrain | Orbit | File (`tests/exports/`) | Verdict | Note |
|---|---|---|---|---|---|
| 1 | Sine Product | Ellipse | `SineProduct-Ellipse-C4.wav` |  |  |
| 2 | Sine Product | Superellipse | `SineProduct-Superellipse-C4.wav` |  |  |
| 3 | Sine Product | Limaçon | `SineProduct-Limacon-C4.wav` |  |  |
| 4 | Sine Product | Epitrochoid 3 | `SineProduct-Epitrochoid3-C4.wav` |  |  |
| 5 | Sine Product | Epitrochoid 5 | `SineProduct-Epitrochoid5-C4.wav` |  |  |
| 6 | Sine Product | Epitrochoid 7 | `SineProduct-Epitrochoid7-C4.wav` |  |  |
| 7 | Sine Product | Hypocycloid 3 | `SineProduct-Hypocycloid3-C4.wav` |  |  |
| 8 | Sine Product | Hypocycloid 5 | `SineProduct-Hypocycloid5-C4.wav` |  |  |
| 9 | Sine Product | Hypocycloid 7 | `SineProduct-Hypocycloid7-C4.wav` |  |  |
| 10 | Sine Product | Butterfly | `SineProduct-Butterfly-C4.wav` |  |  |
| 11 | Sine Product | Squarcle | `SineProduct-Squarcle-C4.wav` |  |  |
| 12 | Radial Rings | Ellipse | `RadialRings-Ellipse-C4.wav` |  |  |
| 13 | Radial Rings | Superellipse | `RadialRings-Superellipse-C4.wav` |  |  |
| 14 | Radial Rings | Limaçon | `RadialRings-Limacon-C4.wav` |  |  |
| 15 | Radial Rings | Epitrochoid 3 | `RadialRings-Epitrochoid3-C4.wav` |  |  |
| 16 | Radial Rings | Epitrochoid 5 | `RadialRings-Epitrochoid5-C4.wav` |  |  |
| 17 | Radial Rings | Epitrochoid 7 | `RadialRings-Epitrochoid7-C4.wav` |  |  |
| 18 | Radial Rings | Hypocycloid 3 | `RadialRings-Hypocycloid3-C4.wav` |  |  |
| 19 | Radial Rings | Hypocycloid 5 | `RadialRings-Hypocycloid5-C4.wav` |  |  |
| 20 | Radial Rings | Hypocycloid 7 | `RadialRings-Hypocycloid7-C4.wav` |  |  |
| 21 | Radial Rings | Butterfly | `RadialRings-Butterfly-C4.wav` |  |  |
| 22 | Radial Rings | Squarcle | `RadialRings-Squarcle-C4.wav` |  |  |
| 23 | Saddle | Ellipse | `Saddle-Ellipse-C4.wav` |  |  |
| 24 | Saddle | Superellipse | `Saddle-Superellipse-C4.wav` |  |  |
| 25 | Saddle | Limaçon | `Saddle-Limacon-C4.wav` |  |  |
| 26 | Saddle | Epitrochoid 3 | `Saddle-Epitrochoid3-C4.wav` |  |  |
| 27 | Saddle | Epitrochoid 5 | `Saddle-Epitrochoid5-C4.wav` |  |  |
| 28 | Saddle | Epitrochoid 7 | `Saddle-Epitrochoid7-C4.wav` |  |  |
| 29 | Saddle | Hypocycloid 3 | `Saddle-Hypocycloid3-C4.wav` |  |  |
| 30 | Saddle | Hypocycloid 5 | `Saddle-Hypocycloid5-C4.wav` |  |  |
| 31 | Saddle | Hypocycloid 7 | `Saddle-Hypocycloid7-C4.wav` |  |  |
| 32 | Saddle | Butterfly | `Saddle-Butterfly-C4.wav` |  |  |
| 33 | Saddle | Squarcle | `Saddle-Squarcle-C4.wav` |  |  |
| 34 | Ridged Cosines | Ellipse | `RidgedCosines-Ellipse-C4.wav` |  |  |
| 35 | Ridged Cosines | Superellipse | `RidgedCosines-Superellipse-C4.wav` |  |  |
| 36 | Ridged Cosines | Limaçon | `RidgedCosines-Limacon-C4.wav` |  |  |
| 37 | Ridged Cosines | Epitrochoid 3 | `RidgedCosines-Epitrochoid3-C4.wav` |  |  |
| 38 | Ridged Cosines | Epitrochoid 5 | `RidgedCosines-Epitrochoid5-C4.wav` |  |  |
| 39 | Ridged Cosines | Epitrochoid 7 | `RidgedCosines-Epitrochoid7-C4.wav` |  |  |
| 40 | Ridged Cosines | Hypocycloid 3 | `RidgedCosines-Hypocycloid3-C4.wav` |  |  |
| 41 | Ridged Cosines | Hypocycloid 5 | `RidgedCosines-Hypocycloid5-C4.wav` |  |  |
| 42 | Ridged Cosines | Hypocycloid 7 | `RidgedCosines-Hypocycloid7-C4.wav` |  |  |
| 43 | Ridged Cosines | Butterfly | `RidgedCosines-Butterfly-C4.wav` |  |  |
| 44 | Ridged Cosines | Squarcle | `RidgedCosines-Squarcle-C4.wav` |  |  |
| 45 | Mitsuhashi | Ellipse | `Mitsuhashi-Ellipse-C4.wav` |  |  |
| 46 | Mitsuhashi | Superellipse | `Mitsuhashi-Superellipse-C4.wav` |  |  |
| 47 | Mitsuhashi | Limaçon | `Mitsuhashi-Limacon-C4.wav` |  |  |
| 48 | Mitsuhashi | Epitrochoid 3 | `Mitsuhashi-Epitrochoid3-C4.wav` |  |  |
| 49 | Mitsuhashi | Epitrochoid 5 | `Mitsuhashi-Epitrochoid5-C4.wav` |  |  |
| 50 | Mitsuhashi | Epitrochoid 7 | `Mitsuhashi-Epitrochoid7-C4.wav` |  |  |
| 51 | Mitsuhashi | Hypocycloid 3 | `Mitsuhashi-Hypocycloid3-C4.wav` |  |  |
| 52 | Mitsuhashi | Hypocycloid 5 | `Mitsuhashi-Hypocycloid5-C4.wav` |  |  |
| 53 | Mitsuhashi | Hypocycloid 7 | `Mitsuhashi-Hypocycloid7-C4.wav` |  |  |
| 54 | Mitsuhashi | Butterfly | `Mitsuhashi-Butterfly-C4.wav` |  |  |
| 55 | Mitsuhashi | Squarcle | `Mitsuhashi-Squarcle-C4.wav` |  |  |
| 56 | Cosine Wells | Ellipse | `CosineWells-Ellipse-C4.wav` |  |  |
| 57 | Cosine Wells | Superellipse | `CosineWells-Superellipse-C4.wav` |  |  |
| 58 | Cosine Wells | Limaçon | `CosineWells-Limacon-C4.wav` |  |  |
| 59 | Cosine Wells | Epitrochoid 3 | `CosineWells-Epitrochoid3-C4.wav` |  |  |
| 60 | Cosine Wells | Epitrochoid 5 | `CosineWells-Epitrochoid5-C4.wav` |  |  |
| 61 | Cosine Wells | Epitrochoid 7 | `CosineWells-Epitrochoid7-C4.wav` |  |  |
| 62 | Cosine Wells | Hypocycloid 3 | `CosineWells-Hypocycloid3-C4.wav` |  |  |
| 63 | Cosine Wells | Hypocycloid 5 | `CosineWells-Hypocycloid5-C4.wav` |  |  |
| 64 | Cosine Wells | Hypocycloid 7 | `CosineWells-Hypocycloid7-C4.wav` |  |  |
| 65 | Cosine Wells | Butterfly | `CosineWells-Butterfly-C4.wav` |  |  |
| 66 | Cosine Wells | Squarcle | `CosineWells-Squarcle-C4.wav` |  |  |

### B-2 · flagged pairs (C2 / C6)

Three groups, listed once each. The 17 A6-muted pairs are the Bandlimited truncation set — in the 2× WAVs below they still sound; the muting is what the *Bandlimited* Quality does at A6 and is the thing to compare against.

| Group | Terrain × Orbit | Why flagged | File | Verdict | Note |
|---|---|---|---|---|---|
| H6 2× worst | Mitsuhashi × Epitrochoid 7 | worst 2× aliasing row in the whole grid, −70.7 dB at A2 | `Mitsuhashi-Epitrochoid7-C2.wav` |  |  |
| H2 worst | Radial Rings × Ellipse | weakest fundamental in the grid, h1-max −2.3 dB | `RadialRings-Ellipse-C4.wav` |  |  |
| Bandlimited muted at A6 | Sine Product × Epitrochoid 5 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `SineProduct-Epitrochoid5-C6.wav` |  |  |
| Bandlimited muted at A6 | Sine Product × Epitrochoid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `SineProduct-Epitrochoid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Sine Product × Hypocycloid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `SineProduct-Hypocycloid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Radial Rings × Epitrochoid 3 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RadialRings-Epitrochoid3-C6.wav` |  |  |
| Bandlimited muted at A6 | Radial Rings × Epitrochoid 5 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RadialRings-Epitrochoid5-C6.wav` |  |  |
| Bandlimited muted at A6 | Radial Rings × Epitrochoid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RadialRings-Epitrochoid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Radial Rings × Hypocycloid 5 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RadialRings-Hypocycloid5-C6.wav` |  |  |
| Bandlimited muted at A6 | Radial Rings × Hypocycloid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RadialRings-Hypocycloid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Saddle × Epitrochoid 5 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `Saddle-Epitrochoid5-C6.wav` |  |  |
| Bandlimited muted at A6 | Saddle × Epitrochoid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `Saddle-Epitrochoid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Saddle × Hypocycloid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `Saddle-Hypocycloid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Ridged Cosines × Epitrochoid 5 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RidgedCosines-Epitrochoid5-C6.wav` |  |  |
| Bandlimited muted at A6 | Ridged Cosines × Epitrochoid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RidgedCosines-Epitrochoid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Ridged Cosines × Hypocycloid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `RidgedCosines-Hypocycloid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Cosine Wells × Epitrochoid 5 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `CosineWells-Epitrochoid5-C6.wav` |  |  |
| Bandlimited muted at A6 | Cosine Wells × Epitrochoid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `CosineWells-Epitrochoid7-C6.wav` |  |  |
| Bandlimited muted at A6 | Cosine Wells × Hypocycloid 7 | strongest harmonic below −60 dBFS at A6 in Bandlimited | `CosineWells-Hypocycloid7-C6.wav` |  |  |

**Row counts:** 66 C4 rows + 19 flagged rows = 85 blank verdicts.

---

## Table C — the open human rows, folded in

Twelve rows carried from the two earlier verifications, plus three new to Round B. None of them blocks the automated stage verdict.

| # | Row | Source | Verdict / result | Note |
|---|---|---|---|---|
| 1 | **PERF-03 in WKWebView (Standalone, then Logic AU), DPR 2:** Terrain tab, ⌥-click the small `θ · r · c` HUD bottom-left three times, then read `~/Library/Logs/O-Strata/view-perf.log`. Pass: `mean` ≤ 2 ms at `mode: "webgl2"`. | `stages/3-gui/VERIFICATION.md` §Human row 1 |  |  |
| 2 | **Standalone hands-on** (tick a MIDI input first): wireframe follows Terrain / Terrain Freq / Mod X / Mod Y; orbit curve matches the ≋ trace; Feedback > 0 → scan point on the displaced path + amber trail; `Init` apply and a session restore re-push map + playhead; drag / wheel / ⌥-drag / ⌥-click edit the active oscillator; Osc B selection repoints them; `Import…` → Imported…, Blur / Edge live; drop a PNG on the big view and on the Osc B mini; a 10 MB PNG → the amber over-2-MB notice; a renamed JPEG → "could not read". | `stages/3-gui/VERIFICATION.md` §Human row 2 |  |  |
| 3 | **Logic (AU):** one ⌘Z undoes a whole drag / wheel burst / ⌥-drag (UI-03); the row-2 hands-on list in the AU. | `stages/3-gui/VERIFICATION.md` §Human row 3 |  |  |
| 4 | **`WEBGL_lose_context` in a Debug Standalone** (`window.__strataScene.debugLoseContext()` / `debugRestoreContext()`) recovers. | `stages/3-gui/VERIFICATION.md` §Human row 4 |  |  |
| 5 | **French read of the 2 Stage 3 rows** (`label.importTooLarge`, `label.importFailed`) and the **zh flips**. | `stages/3-gui/VERIFICATION.md` §Human row 5 |  |  |
| 6 | **Stage 3 Round A rows 3–10:** ≋ with a held note + re-election, host automation on 6 of the 34 parameters, preset apply refresh, Osc A / B repoint, held-notes bar, Logic pass of the readout forms, D3 feel, the two-line readout wrap. | `stages/3-gui/VERIFICATION.md` §Human row 6 |  |  |
| 7 | **Locate… by hand (QUAL-03):** Import… a 3–8 MiB PNG, save a User preset, move the file, reload → "Source missing" + **Locate…**, the oscillator sounds Sine Product; Locate… the moved file → restored; Locate… a *different* PNG → "Different file — hash does not match the preset" for 4 s, sticky notice returns. | `stages/4-polish/round-a/VERIFICATION.md` §Human row 1 |  |  |
| 8 | **Logic, bytes form (FUNC-08):** a User preset with an embedded ≤ 2 MiB PNG survives save / close / reopen of the project; the map re-pushes within one interval. | `stages/4-polish/round-a/VERIFICATION.md` §Human row 2 |  |  |
| 9 | **Live restore (UI-04):** save a Standalone session with an imported PNG, quit, relaunch → the map shows the picture within one interval; load `Init` → back to Sine Product, notice hidden. | `stages/4-polish/round-a/VERIFICATION.md` §Human row 3 |  |  |
| 10 | **Dropdown:** `Init` first, then Pads … FX; ◀ / ▶ walk all 18 alphabetically. | `stages/4-polish/round-a/VERIFICATION.md` §Human row 4 |  |  |
| 11 | **First listen** to the 18 presets for gross faults only — superseded by Table A, tick it there. | `stages/4-polish/round-a/VERIFICATION.md` §Human row 5 |  |  |
| 12 | **French read of the five Round A rows** (`label.locate`, `label.locateMismatch`, `label.importTooLargeFile`, `tip.terImport`, `tip.terLocate`, all `reviewed: false`) and the **zh flips** (53 at `'mt'`). | `stages/4-polish/round-a/VERIFICATION.md` §Human row 6 |  |  |
| 13 | **Open the CI run URL once** and confirm both jobs green (`probes-macos` + `windows-vst3`, dispatched with `plugin=O-Strata`). URL in `SUMMARY.md` / CHANGELOG *Technical notes*. | Round B (Decision 44 a) |  |  |
| 14 | **Acknowledge the Windows named deferral:** "Named deferral — owner none, blocked on hardware. No human sees the Windows UI this milestone." CI proves MSVC compiles, the VST3 loads and pluginval 10 opens the editor — not that the UI is correct. | Round B (Decision 44 b) |  |  |
| 15 | **PERF-03, Logic half:** the same ⌥-click burst with the AU open in Logic at DPR 2, recorded next to the Standalone figure. | Round B (Decision 44 c) |  |  |

**Row count:** 12 carried + 3 Round B = **15 rows**.

> **Round B note on row 1:** the executor tried the scripted burst four times on 2026-09-13 (System Events process-scoped click; CGEvent `postToPid`; the window pinned to (40, 60) via the Standalone `.settings`) and got no `view-perf.log`. Brave was frontmost throughout and the clicks never reached the WKWebView without activating the app, which would have taken the screen from Taylor. The page itself rendered (static DOM + knob arcs) but the rAF-driven canvases were blank — the documented occluded-WKWebView behaviour, not a defect. PERF-03 therefore stays **pending** and the `gl.finish()` flag was **not** added (PLAN Decision 36's unreachable branch).

---

## How to run the material

```bash
cd /Users/taylorbrook/Dev/VST-development

# The preset renders (Table A), in bank order
for f in plugins/O-Strata/tests/exports/presets/*.wav; do echo "$f"; afplay "$f"; done

# One grid file on a loop (Table B) — ^C to move on
while :; do afplay plugins/O-Strata/tests/exports/Mitsuhashi-Epitrochoid7-C2.wav; done

# Re-render the material if anything changed
H=./build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test
$H --gate exportPresets      # 18 preset WAVs -> tests/exports/presets/
$H --gate export             # 198 grid WAVs  -> tests/exports/ + re-records the golden
(cd plugins/O-Strata/tests/exports && shasum -a 256 -c ../render-harness/golden/round-a-grid.sha256 | grep -vc ': OK')   # expect 0

# The Standalone for the hands-on rows (the install script does not install it)
open build/plugins/O-Strata/O-Strata_artefacts/Release/Standalone/O-Strata-dev.app
```

## CHANGE protocol (PLAN Decision 38 — after the sitting, never during it)

One CHANGE per commit. Name the constant by file + token, edit it, then:

```bash
H=/Users/taylorbrook/Dev/VST-development/build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test
ninja -C /Users/taylorbrook/Dev/VST-development/build O-Strata-render-test O-Strata_Standalone O-Strata_VST3 O-Strata_AU
$H --gate H2 --gate H3 --gate H6 --gate H7        # + --gate scheduler for Terrains.h, --gate saturation for the saturation law
$H --gate orbits --out /tmp/orbits.json && node plugins/O-Strata/tests/orbit-golden.mjs /tmp/orbits.json   # Orbits.h only, after porting the JS mirror in terrain-view.js
$H --gate export                                   # re-records tests/exports + the golden (not in `all`)
git diff --stat -- plugins/O-Strata/tests/render-harness/golden/round-a-grid.sha256   # changed terrain / orbit rows only
(cd plugins/O-Strata/tests/exports && shasum -a 256 -c ../render-harness/golden/round-a-grid.sha256 | grep -vc ': OK')   # expect 0
$H --gate all
git commit -- plugins/O-Strata                     # code + golden together
```

A **NOTE** never changes code — it goes to CHANGELOG *Known limits*. The factory bank is not re-authored for a NOTE. A CHANGE that moves a preset's H2 figure re-runs `--gate H2` and updates the `FactoryPresets.cpp` comment; the on-disk bank regenerates itself from the content stamp.
