---
plugin: O-Octagon
version_reviewed: 1.13.1
reviewed: 2026-10-11
head: 59404a1b
scope: four goals — bugs, code simplification, UI improvements, feature improvements
method: >
  Five parallel reviewers (DSP/audio thread; state/presets/venue/bridge; WebView front end + UI;
  simplification; features + 2025-26 field research), each read-only against HEAD. Bugs were
  confirmed by out-of-tree repros against the real GainStage / MonitorFold / VerifyPing /
  Decorrelator / VenueModel / SceneStore / APVTS objects, or by driving the shipped page under
  tests/ui-stub (Chromium and WebKit). Every High finding was re-checked by the orchestrator
  in source and, where it is visual, in the stub. Baseline gates at HEAD, run at the start:
  render 75/0, geometry 59/0, ui_frontend_check 43/43, ui_layout_check 34/34 — ALL GREEN while
  the High findings below are live, which is itself a finding (see "Gate defects").
counts:
  bugs: { high: 5, medium: 7, low: 13 }
  gate_defects: 7
  simplification: 17
  ui_improvements: 16
  features: 14
status: in_progress
resolved:
  - "v1.13.2 (2026-10-11): B1, B2, B4, B6, B7, B11, B12, B23, U1, G1, G2, G4 — Round 1"
  - "v1.14.0 (2026-10-11): B3, B5, B10, B16, U4, G5, G3 (stub preset half) — Round 2"
previous_reviews:
  - CODE_REVIEW.md (v1.8.0) — open items re-checked below
  - .planning/CODE_REVIEW-v1.3.0.md — IN-01..IN-34 re-checked below (never swept)
  - .planning/SIMPLIFICATION-AUDIT.md (v1.3.0) — skipped/deferred items re-checked below
  - .planning/FEATURE-REVIEW.md (v1.3.0) — open gaps re-ranked below
queue: .planning/IMPROVEMENT-QUEUE.md (rewritten 2026-10-11 from this review)
---

# O-Octagon v1.13.1 — four-goal review

**Propose-only. No source file was changed.** The order of work is in `IMPROVEMENT-QUEUE.md`.

## Verdict

The DSP core is in good health. It has no allocation, lock or logging on the audio thread, NaN is
contained, the channel map is safe, and offline renders match realtime. Every machine gate is green.

**The gates are green while five High defects ship.** Four of them sit where the gates cannot see:

- the **stereo / mono bus**, which has been the default headphone workflow since v1.11.0;
- the **preset rail**;
- the **venue table's commit path**;
- the **room plan's stacking order**.

The fifth, presets moving the hall's master level, is a written policy that contradicts the suite
rule. Most of the gates exercise these states with fixture combinations the processor cannot
produce, so they certify the stub, not the plugin.

The audio-path findings are all **transition** defects, measured as clicks, zipper noise and
chirps. Each was confirmed against a control render:

- the Air filter's exit and cutoff steps, which v1.13.0's listener-distance law made common;
- the decorrelator's integer taps;
- alignment-delay edits;
- the verify ping;
- render-start ramps.

None is a steady-state error, and none moves a cross-version digest at default settings.

---

## 1. Bugs

Confidence key: **R** = confirmed by repro, **T** = confirmed by code trace, **P** = plausible.

### High

#### B1 — On any stereo or mono bus the header widens the frame and clips the right-hand column (R, T)
- **Where:** `styles.css:153-160` (`.frame` is a grid with no `grid-template-columns`, so the
  implicit column takes the header's min-content width), `styles.css:184` (`.header` is a no-wrap
  flex), `styles.css:264-266` (banner widths pinned at 184/396/400 px), `app.js:648-659`
  (`renderMapBanner` shows MAP whenever `mapInvalid`), `app.js:857` (SAFE yields to binaural, MAP
  does not).
- **Why it fires:** `buildSpeakerToBuffer` returns `notEightChannels` for any set that is not 8
  channels (`ChannelMap.cpp:63`). `rebuildChannelMap` stores that as `mapInvalid`
  (`PluginProcessor.cpp:441-445`). The v1.11.0 binaural arm runs *precisely* when the map is
  unavailable. So every stereo session shows **MAP + MONITOR(binaural)**.
- **Measured** (stub, real status; reproduced by the orchestrator):
  - header `scrollWidth` 1317 px in en, 1308 in fr, 1284 in zh;
  - the controls column's right edge sits at x = 1301 inside an 1100 px frame;
  - the title wraps onto two lines.
- **What is lost:**
  - Room screen: the Source Y / Width / Blur / Air values, STORE, U3/U4 and the Elevation readouts.
  - Venue screen: Load, Roles, Ping 2–4/6–8, Stop and most of Headphones.
  - Dry stereo (binaural off) is 1102 px and still wraps the title.
  - WebKit behaves identically.
- **Since:** severe since v1.11.0, so it is in the v1.13.0 public release.
- **Why gates miss it:** `documentElement.scrollWidth` stays 1100 (layout §8). The
  `i18n-states.json:36-37` BINAURAL state sets `mapInvalid 'none'`, a combination the C++ cannot
  produce.
- **Fix:**
  - Suppress MAP when `mapInvalidReason === "notEightChannels"`, because SAFE / binaural already
    state that fact.
  - Add `.frame { grid-template-columns: minmax(0,1fr) }`, `.header { min-width: 0 }`, and
    `flex-shrink: 0; white-space: nowrap` on the title and subtitle.
  - **Gates:** replace the BINAURAL state with the real one (safeMode + `notEightChannels` +
    binaural). Assert `header.scrollWidth <= clientWidth` and that the right edges of
    `.controls-column` and `#venue-rail` are ≤ 1100 in that state.

#### B2 — A second venue-table edit within ~500 ms silently reverts the first (R, T)
- **Where:**
  - `venue.js:226-253`: `buildPayload` fills every non-pending field from the cached `committed`.
  - `commit()` clears `pending` right after sending (`:421`) and never updates `committed` on
    success.
  - `committed` refreshes only when the 2 Hz status poll sees `venueGen` move (`app.js:180, :868`).
  - C++ `setVenue` overwrites all 8 rows and the rake from the payload (`PluginEditor.cpp:651-675`).
- **Scenario:**
  - Trim 1 → −3 dB, Tab, trim 2 → −6 dB, Tab within ~200 ms: the second payload carries speaker 1
    at 0 dB, so the −3 is lost and the field repaints "0.0".
  - The same applies to an **L↔R label swap** followed quickly by any numeric edit: the routing
    reverts to a *valid* permutation, so no MAP banner fires (risk class R1).
  - Probability is about (500 − gap)/500.
- **Repro:** at a 200 ms gap the payloads were `[[-3,0],[0,-6]]`; at 700 ms (negative control)
  `[[-3,0],[-3,-6]]`.
- **Fix:** send only the pending fields. C++ already falls back to the live value per field
  (`finiteOr`, `hasProperty("label")`). An alternative is a `baseGen` stale-refusal.
- **Gates:** `ui_frontend_check` §22(3) and `ui_layout_check` §13 assert "all 50 in one payload",
  which enshrines the bug. Restate them, and add a "two commits inside one poll tick" layout
  section.

#### B3 — User presets cannot be created from the UI, and Save fails silently (T; repro with C++-shaped stub overrides)
- **Where:**
  - The rail is a `<select>` with no name field (`index.html:850`).
  - Save sends `presetSelect.value`, which is always an existing name (`venue.js:564-570`).
  - On a stock install every option is a factory preset, so the module's factory guard refuses it
    (`OuariconPresetManager.h:396-401`). The `writeFailed` result is ignored (`.then(refreshPresets)`).
- **Effect:**
  - No user preset can ever be created. Scenes-in-presets and both migration gates are reachable
    only through hand-placed JSON.
  - A hand-placed user preset is overwritten without confirmation.
  - The current name is editor-owned, so the rail opens blank / "Default" on every reopen.
  - Broken since Stage 3.2.
- **Why gates miss it:** the stub seeds a writable "Init" and has no factory guard
  (`juce-stub.js:900`).
- **Fix:** a "Save as" name field, Factory / User `<optgroup>`s, a read-only hint on factory names,
  a rendered `{ok, reason}` with a distinct `factoryName` reason, the current name persisted in
  processor state, and a factory guard added to the stub.

#### B4 — The puck can sit under a weight cell; the next grab writes that speaker's weight (R)
- **Where:** `index.html:279-300` (the puck precedes the w-cells in DOM order, and neither has a
  z-index; `styles.css:647`, `:671`); `roomplan.js:467-484` (68×30 px cells at glyph y+14).
- **Repro:** drag the puck onto speaker 1. `elementFromPoint` at the puck centre returns `ctl-w1`;
  the next grab moved Weight 1 from 1.00 to 0.07. That is audible, and recorded in Touch/Latch.
- **Fix:** `.puck { z-index: 2 }`. The output popover keeps 3.

#### B5 — Presets move the hall's master level, and every preset load swings the "preserved" source and weights through defaults (T; transient R)
- **outputGain:**
  - `kAuthored` includes `outputGain` (`PresetPolicy.h:89`), and the rows author −1.5 / −3 / −2 dB
    (`:177-186`).
  - The module's reset puts 0 dB on every preset that omits it. Example: an operator at −9 dB for
    the PA loads Concert Default, and the PA jumps **+9 dB mid-show**.
  - This contradicts the suite rule (*presets never set outputGain; load must not touch it*,
    2026-09-20/21).
- **Transient:**
  - `loadPreserving` (`PresetPolicy.h:285-291`) snapshots, loads, then restores. Inside, the module
    resets all 28 parameters and applies every key first (`OuariconPresetManager.h:360-376`).
  - Repro: srcX 0.1 → **0.5** → 0.1 and w2 0 → **1.0** → 0, recorded inside the open gesture
    brackets.
  - Any block that starts inside the window solves at centre with all weights 1, so silenced
    speakers bloom for a moment. Audibility is P; the transient is R.
- **Fix:**
  - Move `outputGain` to `kPreserved` (13 + 15 still partitions 28) and drop the column.
  - Never write the preserved twelve during a load: use a module opt-in parameter filter, or a
    local parse → migrate → authored-or-default → `customLoad` apply.
- **Gates:** CP's six-applied clause (expects −3.0 dB), DK, and the partition static_assert. Add a
  CP clause requiring zero value callbacks on the preserved set during a load.

### Medium

#### B6 — On a stereo or mono bus every venue write is refused, while the binaural fold renders that venue (R)
- **Where:** `applyVenueEditChecked` validates labels against the *negotiated* set
  (`PluginProcessor.cpp:641-651`), and that set has fewer than 8 channels.
- **Affected writes:** the table, `.venue` load, Direct/Roles, Derive and the output popover
  (`PluginEditor.cpp:678/758/796/832/937`).
- **Scenario:** v1.11.0's purpose is preparing a piece on a laptop. Loading the hall's `.venue` on
  a stereo track is refused, and the fold keeps rendering the placeholder room.
- **Fix:** when `! oo::rig::isRealRig(outSet)`, validate against `create7point1()`. The map stays
  flagged, and the existing backstop re-checks on renegotiation.
- **Gates:** a new render probe (stereo → accepted; renegotiate to 7.1 → map builds); rescope
  layout §31.

#### B7 — Venue file, preset and popover results never reach the operator (T)
- **Where:** `venue.js:514-520`, `app.js:1283-1286` and `venue.js:785` (Derive) only `.catch`.
  `venuefile::describe()` has zero callers, against the rule in `VenueFile.h:58` ("loads, but is
  SURFACED").
- **Silent outcomes:**
  - `forwardVersion` loads silently drop attributes.
  - `malformedRoot`, `unreadable`, `labelNotInSet` and failed saves do nothing visible.
  - Output-order and Ping lines print raw C++ codes (`notEightChannels`, `mapInvalid`), on the
    French page too.
  - With IN-03 (v1.3.0, still live, R), a duplicate or zero-padded `@index` loads `ok` with
    placeholder speakers.
  - With IN-23 (still live, R), `schemaVersion="abc"` loads `ok` as version 0.
- **Fix:** one refusal / result renderer with literal i18n keys (assertion 13 shape) and a
  `#vfile-state` line. Wire `describe()`. Make `@index` coverage and schemaVersion parsing strict.

#### B8 — Air filter: unsmoothed exit and per-64-sample cutoff steps; v1.13.0 made them common (R, T)
- **Where:** the cutoff is set once per control block with no ramp (`GainStage.cpp:522-525`); the
  exit switch is `fL = airActiveL ? LP(sL) : sL` (`:832`); `airAmount → 0` resets and bypasses
  (`:564-568`).
- **Why v1.13.0 matters:** the filter used to run on 5.8 % of the plane at ≥ 12 kHz. It now runs
  everywhere beyond the 0.79 m near field, down to 552 Hz.
- **Repro** (click ratio vs. a matched air-0 control):

  | Event | 1 kHz | 200 Hz |
  |---|---|---|
  | Orbit 16 m, air 1 → Motion OFF | 43× | 207× |
  | Front-left puck, air 1 → 0 | 52× | — |
  | Air 1 → 0.05 (cutoff jump, filter stays on) | 34× | — |

- **Triggers:** Motion off, loading Dry Point over Slow Orbit / Wander, an air lane touching 0, a
  stepped position, a path switch, a Sync loop wrap.
- **Fix:** a per-sub-point `airMix` smoother (`fL = sL + airMix·(LP(sL) − sL)`). When settled at 0
  this is literally `sL`, so bit-transparency survives. Interpolate G per sample between boundary
  values.
- **Gates:** CU/DC and BD are unchanged. BC (models the staircase) needs re-deriving. **Probe BB
  is stale:** its "fc near ceiling" premise died in v1.13.0, and it asserts equality rather than a
  bound.

#### B9 — Decorrelator zipper noise on every decorr move and on any width lane below 2 m (R)
- **Where:** `Decorrelator.h:287-294` computes `int(base·depth + 0.5)` with no transition, and
  depth tracks wEff below 2 m (`GainStage.cpp:599-608`). The "set-and-forget" note at
  `Decorrelator.h:273-286` is therefore false.
- **Repro:**
  - decorr 0.5 → 1.0: 28× at 200 Hz.
  - Width lane 0.5 → 2 m at decorr 1: 34× / 30×.
  - Control (decorr 0): 1.0×.
- **Fix:** crossfade the old and new taps over 32–64 samples, one change in flight at a time.
- **Gates:** none move (CV uses static depths; no digest has decorr > 0).

#### B10 — Host change-gestures leak (T; gesture pairs R)
- **Every key opens a gesture:** `app.js:323-325` opens one on *any* keydown on a focused slider,
  including Tab, Shift, Space and ⌘. On WebView2, Tab through four sliders gives four start/end
  pairs with no value write.
- **Gestures survive editor teardown:** `~OctagonEditor` (`PluginEditor.cpp:1694-1712`) closes
  none, and JUCE's `~WebSliderParameterAttachment` only removes its listener.
  - Example: ⌘W with a slider focused opens a gesture on the Meta keydown, and the window dies
    before keyup.
  - Example: a drag cut off by a track switch or project close.
  - In both, the host keeps the parameter "touched", so Touch/Latch keeps writing.
- **Fix:** gesture only on value keys (arrows, Page, Home, End). Port O-simpleWavetable's
  `closeOpenGestures()`, with a **counter** rather than a bool, because `loadPreset` and
  `applySceneWeights` nest brackets inside a live drag.

#### B11 — The Venue mini-plan is crushed to ~20×25 px, and its gate passes (R)
- **How it shrank:** `.miniplan` is the rail's `flex: 1 1 auto` residual (`styles.css:1524`). It
  went 213 px (Stage 3.2) → 119 (v1.1.0, Output order) → **25 px (v1.7.0, Monitor)**, so the eight
  r = 6 dots now form a knot.
- **Why the gate passes:** layout §11 checks only that the fitted box is inside its stage and that
  h > 0.
- **Fix:** relocate it to the 752×213 px of dead space under the venue table (U3), and assert a
  minimum rendered size.

#### B12 — Settings popover, Headphones toggle and other small interaction defects (R unless noted)
- **Settings popover survives a puck drag:** the dismisser is on `mousedown` (`app.js:1048`), but
  the puck's pointerdown `preventDefault` suppresses it (`pattern_pointerdown_preventdefault_…`).
  Fix: capture-phase `pointerdown`.
- **Headphones toggle sends the inverse of the active state, not of the preference** (P):
  `venue.js:677-688` toggles `aria-pressed`, which reflects `binauralActive`. While audio is idle,
  every click sends `true`. Fix: toggle the preference that `getStatus.stereoBinaural` already
  returns.
- **`startPing` with a bad argument returns `ok:true` after it has disarmed the monitor**
  (`PluginProcessor.cpp:672`, `VerifyPing.cpp:94`). Fix: validate in the native function.

### Low

| ID | Defect | Where | Conf. | Fix |
|---|---|---|---|---|
| B13 | Alignment-delay edits chirp. The 5 ms linear tap sweep plays backwards at up to −8× (5→50 ms: 61×). Triggered by Derive, `.venue` loads and in-hall calibration with programme playing | `GainStage.cpp:165, 679-683, 903` | R | Two-tap crossfade over 10–20 ms |
| B14 | The verify ping hard-cuts the programme to 0 at start and steps it back at stop / abort / retarget (23× / 36×). There is no `isNonRealtime()` abort (P for a VST3 realtime bounce) | `VerifyPing.cpp:193-199, 264-265, 325-333` | R | Duck programme × (1 − env); abort when non-realtime |
| B15 | Render-start / toggle transients. (a) Headphones OFF on a stereo bus is a hard fold→dry switch (24×). (b) **Every Logic offline bounce of a stereo track opens with a 240-sample swell** from the raw speaker 1/2 lanes into the fold (up to 9.5 dB). (c) Motion renders start at the anchor and converge over ~15 ms | `MonitorFold.cpp:106-107, 286-298`; `GainStage.cpp:252-260, 1030-1095` | R | Teleport the fold mix on the first engage; defer the motion teleport to the first boundary; crossfade SAFE↔binaural |
| B16 | No session version stamp. v1.13.0's blur law re-reads pre-1.13 sessions and automation (0.5 → radius ×2; old default 0.03 → 0.04 m). The CHANGELOG's "sessions untouched" is false for blur. Affects the developer's own pre-release hall sessions | `PluginProcessor.cpp:1143-1155` | T | Stamp a root `pluginVersion` now, so the next law change can migrate |
| B17 | `getTailLengthSeconds()` returns 0, but the tail is ~290 ms at decorr 1 + 50 ms delay. Hosts sizing tails or suspending on silence truncate the lanes | `PluginProcessor.cpp:1121` | R | Return ≥ 0.35 s |
| B18 | Decorrelate costs a mono source ~2.8 dB at every width (coherent → incoherent sum). A design call | `GainStage.cpp:823-824, 849-865` | R | Level-compensate the wet feeds on a mono bus, crossfaded by dMix |
| B19 | Decorrelator / delay engage flags never release while parameters stay static (CPU only; output bit-exact) | `GainStage.cpp:423-427` | R | Don't early-return while a mix or delay smoother ramps |
| B20 | IN-04 widened: the SAFE arm and bypass leave alignment lines, decorrelator rings and Air state stale. SAFE also skips `decorrMix`/`decorrDepth`, against the rule at `:840-845` | `GainStage.cpp:1056-1095` | T | Clock or clear on the SAFE↔REAL edge |
| B21 | Localization gaps since v1.9.0: raw `VERTEX`/`ON_EDGE`/`INTERIOR` in the Class column, "manual 3" / "auto 3" composed from C++ codes, raw refusal codes, French "Marche ☑ Marche" | `venue.js:303-304, 593, 633, 732`; `i18n.js:533` | R | `setLabel` with literal keys; French caption "Lancer" |
| B22 | Plan readouts are below AA over the painted field (`.w-label` 3.33–4.40:1). v1.13.1's census composited CSS colours only; production is likely worse, because the 1/k field peaks at the speakers | `styles.css` `.w-cell` | R (stub) | Backing on `.w-cell` (U2) |
| B23 | Empty user slots render struck through, against the CSS's own stated intent; a double-click selects the glyph numeral | `styles.css:1032-1043`; `#plan-geometry` | R | `.scene-btn.is-slot:disabled{text-decoration:none}`; `user-select:none` |
| B24 | Host-level presets (`.aupreset`, Logic "Save Setting As") restore another hall's venue, labels and scenes, bypassing the venue/preset split | `setStateInformation` | T | Document it, or add a "keep current venue on recall" preference |
| B25 | IN-03 (v1.8.0): Free↔Sync is a phase jump, because the PPQ branch never re-bases | `MotionClock.h:141-156` | T | Re-base `phaseBase`/`lastRate` on mode change |

### Gate defects that hid the above

| ID | Defect |
|---|---|
| G1 | The `tests/i18n-states.json` BINAURAL state sets `mapInvalid 'none'`, which the processor cannot produce on a stereo bus. It hid B1. |
| G2 | Layout §11's mini-plan guard (fitted box inside stage, h > 0) passes at 20×25 px. It hid B11. |
| G3 | Stub drift: `savePreset` accepts factory names (hid B3); `zh-Hans` clamps to `en` (`juce-stub.js:1072`); the field is brightest at centre, not at the speakers (hid B22). |
| G4 | `ui_frontend_check` §22(3) and layout §13 assert "all 50 values in one payload", the shape of B2. |
| G5 | `ui_frontend_check` §28 "endChangeGesture after the load" tests `indexOf('presetManager.loadPreset')`, which is now −1 (the call moved into `loadPreserving`), so it passes vacuously. |
| G6 | Render probe BB asserts equality with \|H − 1\| rather than a bound, and its "fc near ceiling" premise is dead since v1.13.0. |
| G7 | Still open from v1.8.0: IN-23..30 (DF `motionSolves > 0` liveness, `check(true)`, the MP1/MP7 tautologies, no no-PPQ render, unprobed CHANGELOG claims). Also, nothing observes `scenesGeneration` (NOTES). |

### Older findings, re-checked at HEAD

**v1.8.0 review (`CODE_REVIEW.md`):**
- Still live: IN-03 (B25), IN-04 (B20), IN-11 (torn live-puck triple), IN-13 (bypass freezes the
  live puck), IN-14 (`getMotionTrace` per echo, no debounce), IN-20 (Drift tail 48 polls, wiped on
  echo), IN-22 (language / tooltip prefs pulled once), IN-29 (layout §32 tautology).

**v1.3.0 review (`.planning/CODE_REVIEW-v1.3.0.md`, never swept):**
- Still live:
  - IN-01 → B8.
  - IN-03 → B7 (R).
  - IN-04 (SIDES guard `hx > 0.0f`).
  - IN-05 (`countedPow`; instrumentation only).
  - IN-08 (`fadeRatio` division order).
  - IN-09 (chunk-cadenced NaN recovery; also in MonitorFold).
  - IN-10 (SAFE duplicates R; in (2,1) the right input is dropped).
  - IN-11 (`kDenomEpsilon` cliff, by design).
  - IN-12 (meter 2 lights in (2,1)).
  - IN-13 (VerifyPing state tear).
  - IN-14 (`prepareToPlay` touches the ValueTree; the AU wrapper also calls it from
    `SetProperty(OfflineRender)`).
  - IN-15 (`meterPeak` and now `binauralLanePeak` not cleared).
  - IN-17 / IN-24 (null `committed` TypeError; 3 TypeErrors in repro).
  - IN-18 (native-fn count docs say 27 / "EXACTLY THREE"; it is 28).
  - IN-19 (FileChooser re-entry dismisses the first dialog).
  - IN-20 (the PresetPolicy doc table is two remaps stale).
  - IN-21 (SceneStore invalid-parent guard).
  - IN-22 (harmless).
  - IN-23 → B7 (R).
  - IN-26 (tip-suppression latch).
  - IN-28 (`inputmode=decimal` on label fields, now *enforced* by §23).
  - IN-29 (meter ballistics run 2× fast at 120 Hz).
  - IN-31 (elevation de-clash handles pairs only).
  - IN-32 (dead `--frame-w/-h`).
  - IN-33 (puck is `role=slider` but keyboard-inert; in WKWebView it is the only Tab stop on the
    Room screen).
  - IN-34 (meters poll 30 Hz on the Venue screen and while hidden).
- Moot:
  - IN-02 (moot for public users, but the class recurred as B16).
  - IN-16 / IN-25 (completions are never dropped; only the hidden polling of IN-34 remains).
  - IN-27 / IN-30 (completions not dropped in practice).

### Refuted / not affected (do not re-investigate)

- **Stale `<CustomState>` child:** O-Octagon is not affected. It never calls `setStateFromXml`;
  a two-reopen repro survives. The memory has been corrected.
- **A missing PARAM child keeps its live value:** refuted; JUCE 8.0.15 resets it to default.
- **Hostile `.venue` XML** (recursive entity, billion laughs, SYSTEM entity): loads in < 1 ms with
  no file read.
- **Non-ASCII venue names** round-trip.
- **FileChooser outliving the editor:** safe.
- **Bridge:** 28 registered = 28 called.
- **Latency 0 is correct:** Derive aligns to the farthest speaker, and the near ear has 0 ITD.
- **Air cutoff above Nyquist:** no; the ceiling is 0.45·fs.
- **Torn `speakerToBuffer` out of bounds:** impossible; every published map is a permutation.
- **RT safety:** clean.
- **Free-motion reproducibility across Logic offline bounces:** holds, because the AU wrapper
  re-prepares on OfflineRender.
- **Block-size invariance of the binaural arm:** holds.
- **Venue publisher:** every caller still publishes exactly once per call.
- **SmoothedValue NaN:** no NaN target reaches it.
- **Delay push/pop:** paired.
- **Biquad switch:** N/A (TPT one-poles).
- **Oversampler / DryWetMixer:** N/A.
- **Missing i18n keys / console errors:** none in en/fr/zh, in either engine.

---

## 2. Code simplification

Each item states whether it preserves behaviour, how it affects float order, and which gates it
couples to. "Golden-safe" means it does not touch the render-harness audio path.

| ID | Candidate | Payoff | Gates | Risk |
|---|---|---|---|---|
| S1 | **Comment-only pass.** 8,913 comment lines vs 9,219 code lines. ~68 % of non-licence comment lines narrate versions, review IDs or measurements. **217 KB of the 431 KB of embedded UI is comments**, compiled into the binary and served on every editor open. 25+ count statements are wrong ("RELAYS — 18", "EXACTLY THREE", "17 atomics" beside `kCount == 28`). The PresetPolicy table is stale. One incident is narrated 4×; `i18n.js:20-290` is a 270-line translation log | ~3–4k lines, 100+ KB of binary | §30 counts raw `.reset (`; §17 counts raw `setSize (`; §9 reads raw `url == "` | LOW if comment-only and in its own commit |
| S2 | **Dead code:** `VenueModel::delaysMs()`, the `fileVersion` local in `loadVenue`, unread `bindToggle`/`bindCombo` fields (LOW-03 re-introduced at v1.8.0), `scenes.js` `isArmed`/`slotCount`/`SLOT_COUNT`, `elevation.js` `axisMax`, `--frame-w/-h`. *Keep and wire* `venuefile::describe()` and `MAP_REASON_KEY`; B7 needs them | ~60 lines | none found (grepped tests + scripts) | LOW |
| S3 | **The same payload serialised twice:** `getScenes` duplicates `scenesToVar` (its extra `slot`/`generation` keys have no reader). Fold in MEDIUM-01 (`pingStateToObject`) | ~30 lines | §27 reads `scenesToVar`, unchanged | LOW, golden-safe |
| S4 | **MEDIUM-02, now 4 sites:** the venue-edit validate-and-complete tail → private `completeVenueEdit()`. Leave `loadVenue` out (§29 pins it); the new name must not match §22's `/applyVenueEdit\s*\(/` | ~25 lines | §22, §29 | LOW |
| S5 | **MEDIUM-06:** the `getResource` 11-branch ladder → a table. Rewrite §8 / §9 to read table rows; `serve-ui.js` already parses the table form (O-MultiBandCompressor precedent) | ~45 lines | §8, §9, check-i18n [8] | LOW-MED |
| S6 | **MEDIUM-04, now 3 banners:** `.safe/.map/.monitor-banner` share 7 of 8 box declarations. Keep `.map-copy` separate (§18). Do it with B1 | ~25 lines | §18, layout §34 | LOW |
| S7 | **`app.js` `init()`:** 11 copies of one try/catch → `guarded(what, fn)`. Keep a literal `initI18n();` statement (check-i18n [6]) | ~35 lines | §2 regexes, layout §1 | LOW-MED |
| S8 | **The preset migration lives in an editor lambda the harness cannot reach** (`PluginEditor.cpp:1550-1610`). Move it verbatim to `oo::presets::migrate` (PresetPolicy.h), and give `factoryDefs` a `set(id, v)` helper. Makes the `<1.3` / `<1.13` gates probe-able | ~10 net; testability | none | LOW |
| S9 | **The 1,400-line editor constructor** → ~6 `withXxxFunctions(Options)` members by family, every `withNativeFunction("name"` literal intact | readability | none (gates locate lambdas by literal) | LOW |
| S10 | **Small JS duplications:** the plan-fit prologue (`roomplan.js:297` ≈ `venue.js:331`; the untaken half of HIGH-01), `renderPuck` re-deriving `anchorMetres()`, the drop-pending-delays loop ×2, echo-subscribe loops ×3 → `onEcho(ids, fn)`, MEDIUM-05 | ~30 lines | §19 (keep `max−min` and `/` on separate lines) | LOW |
| S11 | **Binaural meter plumbing:** `binauralLanePeak` is atomic but audio-thread at both ends (the header says "Message thread"), and the max-merge is written 3× → a plain array plus `maxInto()` | ~8 lines | §30 unaffected | LOW, meters only |
| S12 | **Motion params marshalled twice** (`GainStage.cpp:358-361`, `PluginEditor.cpp:1202-1205`); a swapped float compiles silently and the trace stops matching the audio → one `paramsFrom()` | drift class removed | DC/DE unaffected | LOW |
| S13 | **`getFieldGrid`:** `readParam` is a 1-line wrapper under a stale 11-line comment, and the 8 weights still read raw atomics (the last NaN-to-JSON path) → `readFiniteParam` for all 11 | ~12 lines | none | LOW (fixes the NaN path) |
| S14 | **`captureScene` sanitises twice**; `SceneStore::capture` already does it | ~6 lines | none | LOW, bit-identical |
| S15 | **Encapsulation:** make `monitorArmed`/`monitorSuppressed`/`stereoBinaural`/`binauralActive` private (a public `monitorArmed` bypasses `setMonitorArmed`'s refusals); turn the two-ternary language codec into one table; add a `canArmMonitor()` shared with `monitorAvailable` | correctness by construction | none | LOW |
| S16 | **Canvas / field colours duplicate CSS tokens** (`roomplan.js:347/367/373`, `field.js:106-107`) → read the tokens once with `getComputedStyle` | drift class removed | layout §26 | LOW-MED |
| S17 | **LOW-08:** the dead `width`/`height` attrs on `#elev-strip`. Render harness (test code): one 6,200-line `main()` with 66 copies of the rig setup → `makeRig()` plus per-probe functions, probe order kept | 2 lines; large for tests | §21, NC1 | LOW |

**Don't:**
- MEDIUM-07 (the deadline guard ×3): §33 pins all three sites, and extracting it costs a new module
  plus a §33 rewrite.
- LOW-06 (the `const el = node` aliases): only worth doing as §6 hardening (named, pinned
  receivers).

**Rejected as look-alikes** (and why):
- **CSS:** source-order cascade traps.
- **Nyquist 0.45 ×3:** float vs double; unifying changes the cutoff bits.
- **`shape` vs `shapeAt(+0)`:** −0.0f.
- **`fire(name)` / button factories:** §3 needs the literal native names.
- **`renderMapBanner` ladder:** check-i18n [13]/[15] need literal keys.
- **Gesture-bracket helper:** §28 needs literal brackets.
- **Generated HTML rows:** §16/§18/§22/§23 count ids.
- **Splitting `renderChunk`:** §30 checks ordering.
- **The nine `kNumSpeakers`:** deliberate decoupling.
- **`PerlinNoise.h`:** a deliberate byte copy.

---

## 3. UI improvements

Fixed 1100×720, dark venue theme, Iowan Old Style + ui-monospace, and the v1.13.1 WCAG AA tokens
are kept throughout; nothing here is a reskin. Screenshots are in the session scratchpad
(`scratchpad/ui/shots/`, `.playwright-mcp/octagon-review-0*.png`).

| ID | Improvement | Change | Effort |
|---|---|---|---|
| U1 | **One banner per fact; a calm binaural banner.** Binaural is the *normal* state of every stereo session but wears the loudest alarm (filled `--alert`), which teaches users to ignore the one alarm that matters (rig outputs muted on 8-channel delivery) | `renderMonitor` sets `banner.dataset.face`. `[data-face=binaural]`: `background: rgba(194,154,74,.10)`, border `--level-lo`, copy `--ink-dim`, tag "BINAURAL" / 双耳. Keep filled orange for the armed 8-channel fold. With B1, a stereo bus shows exactly one banner | S |
| U2 | **Plan legibility.** The weight tracks are near-invisible; the duplicate numeral sits beside each weight; the hull dash runs through "8 1.00" | `.w-cell { background: rgba(26,22,19,.78); border-radius: 2px; padding: 1px 4px 2px }` (≥ 5:1 over any field); drop `.w-label` (the glyph above already numbers it) and give the slider those 12 px; raise the track to `--rule` contrast | S |
| U3 | **Mini-plan under the Venue table** (fixes B11). The 752×213 px under the rake/delay rows is empty | Move `#miniplan` into `.venue-main` after `.venue-delay` (~170×213 at the default aspect). Number the dots (r 6 → 8, 9 px mono). Light row N's dot (`--level-hi`) while any of its fields has focus; "which dot is this?" is the whole point while typing 24 coordinates | M |
| U4 | **A preset rail that saves** (fixes B3) | Name `<input>` + "Save as"; Factory / User `<optgroup>`s; Save disabled on a factory name with the hint "factory presets are read-only"; a result line; a "— unsaved —" placeholder instead of a blank select; drop the redundant current-name line under the select | M |
| U5 | **Discoverable output assignment.** It is double-click only, with no hint | Single-click a glyph opens the popover (double-click kept). Per-glyph tip "Speaker {n} → output {k} · click to reassign". `<g tabindex=0>` + Enter; focus goes into the popover and returns to the glyph. Widen `.out-pop` 168 → 196 px (the title and note wrap with a dangling arrow today) | S–M |
| U6 | **Captions hug their values.** "Ear 0.00 m Source 0.00 m" reads as "0.00 m Source"; the Chinese 源 sits 43 px from its value | `text-align: right` on pinned captions (`#readout-label-*`, `.rail-line .cell-label`, `.vcol-head > [data-i18n]`, `.elev-readouts .cell-label`). In the Elevation head, left-align values and add 12 px before the second caption. Top axis tick "7 m" | S |
| U7 | **Unit symbols are case-sensitive.** Uppercase CSS renders "X M", "TRIM DB", "DELAY MS", "MS \| M"; "M" reads as mega | `<span class="unit">` with `text-transform: none`; move "dB" out of `label.vcol.trim` (into `I18N_EXEMPT`); show "m" on the rake fields | S |
| U8 | **Field legend key.** "Field 1.3 – 10.4 dB" has no colour key or tip | A 36×6 px swatch `linear-gradient(90deg, rgba(232,200,122,0), rgba(232,200,122,.38))` with a 1 px `--rule` border, plus the tip "relative level across the audience plane; brighter = louder" | S |
| U9 | **Footer as a status bar** (~700 px empty). The footer "Source" is the anchor while the Elevation "Source" includes motion, so two readouts with the same name disagree | Right-aligned "SET Stereo · PRESET Chamber · MOTION Figure-8 0.10 Hz"; the live position while motion runs | M |
| U10 | **Source X / Y in metres.** They read "0.500" while Z and Width read metres | `normToMetres` → "6.50 m" | S |
| U11 | **An armed STORE that can't be missed, and expires.** Only the 46 px chip changes; left armed, it overwrites the next slot clicked to recall | While armed: `.is-slot { border: 1px dashed var(--alert); color: var(--ink) }`; auto-disarm after 8 s, on Escape, or on an outside press | S |
| U12 | **The Motion Run cell.** It is a native white checkbox, the only unthemed control on the page, and a 14 px hit target | A themed toggle in the STORE-chip style; `<label for="ctl-motionOn">` for the whole cell; draw the trace at 0.15 opacity while the Motion tab is open and Run is off, so a path can be designed before it is heard | S |
| U13 | **Class column in words.** `VERTEX` / `ON_EDGE` / `INTERIOR` are developer vocabulary (fixes B21) | Localized "corner / edge / inside" (or the plan's solid / dashed glyph as an icon); layout §17 compares `data-class` instead of the text | S |
| U14 | **Keyboard puck** (IN-33). It is `role=slider` but inert, and in WKWebView it is the Room screen's only Tab stop | Arrows ±0.01 (Shift ±0.1), `aria-valuetext` "6.50 m, 12.00 m", a themed focus ring | S |
| U15 | **Fold the one-control Output panel into Space** as a third column (the Motion tab already uses a 3-column grid). This frees ~70 px for a taller Elevation strip | Grid change plus layout pins | M |
| U16 | **Hover help on by default for a new instance** (a product decision); the tips are the only explanation of output assignment and STORE | Default `tooltipsEnabled = true` for sessions with no attribute | S |

---

## 4. Feature improvements

**Rules every new feature must follow:**
- New parameters need a **new version-hint generation** (`kHintV1_14 = 4`) and must be listed in
  exactly one of `kPreserved` / `kAuthored`.
- **Never append to `motionPath` / `motionSync`.** VST3 stores choices as `index/(n−1)`, so
  existing Reaper lanes would retarget. Add a new parameter instead.
- At default settings, the CU / DC digests must stay intact.
- All copy ships in en / fr / zh-Hans.

**What changed in the field since Aug 2026:**
- SPAT 26.04 shipped a cue sequencer, a timeline and LTC/MTC/Link.
- Drawn and rhythmic motion arrived (Holophonix Motion Engine preview, QLab 5.5 object audio,
  Sound Particles Beat Panner).
- ADM-OSC v1.0 is the shared control language (SPAT, Holophonix, QLab 5, Nuendo, L-ISA).
- The headphone bar rose: SOFA in SpatGRIS 4.1.5, head-tracked binaural in L-ISA (free) and
  Logic 12.
- Holophonix previewed an acoustic speaker locator.
- dearVR is discontinued.
- **No new DBAP VST/AU shipped, so the niche is still empty.**

### Quick wins (one `/improve` each, in this order)

| ID | Feature | Value | Builds on | Size | Params |
|---|---|---|---|---|---|
| F1 | **Free-rate motion locked to the timeline** | Today Free phase counts from `prepareToPlay` (`GainStage.h:483-490`), so the orbit you audition from bar 40 is not the one that bounces, and two Free instances never stay in step. Free is the default Sync setting | Sync already reads host position; keep the control grid on the absolute counter (QUAL-03), derive only the phase | S | none (documented MINOR), or Bool `motionLock` |
| F2 | **Time-of-flight in the headphone fold** (then a draggable listener seat) | The fold models distance as 1/r only. At the centroid it plays a 9.4 ms spread (inverted after Derive) where the hall has 0; at a rear corner 9.4 ms where the hall has 35.7 ms. Precedence, the main reason seats differ, is wrong on headphones | `MonitorFold::updateGeometry` already computes `radius[k]` from one listener point; fixed-allocation ~150 ms lines; the seat is a root attribute | S core / M seat | none |
| F3 | **One Monitor switch for every instance** | One instance per stem is the intended workflow; hearing a 6-stem piece binaurally on a 7.1 project needs 6 open editors today | A process-wide atomic ORed into `armed`; every non-persist / offline / bypass guard kept | S | none |
| F4 | **Scene glide** (0–30 s) | A FRONT → REAR crossfade means hand-shaping 8 lanes; a glide records as ordinary ramps in Latch / Touch | `applySceneWeights` (already gesture-bracketed); a message-thread timer; cancel on a manual edit; close gestures on teardown (B10) | S–M | none (root attribute) |
| F5 | **Rehearsal pair: feed solo + speaker out-of-service** | "What is speaker 5 carrying?" in the hall or through the fold; "speaker 6 died at sound-check", so DBAP redistributes at constant level without touching automation | Solo copies `VerifyPing`'s post-mix overwrite; out-of-service is a mask on `w[i]` published in `VenueSnapshot`; refuse an all-silent mask like `applyScene` does | S + S | none |
| F6 | **Setup checklist card** on the Venue rail: 8-ch bus negotiated, map valid, each speaker pinged this session, venue measured (not the placeholder), delays set, monitor off before printing | The first public release's top failure is Logic's multi-mono default (COMPAT-02), documented only in NOTES and one tip | `getStatus`, ping state, venue values | S–M | none |
| F7 | **Width axis for stereo stems:** Auto (today) or a fixed room angle | A stereo room recording can spread front to back as a depth pair; most of the value of independent L/R positions | `shaper::shapeAt`; Auto must run today's code verbatim (AX / AY / DC) | S | 1 (`widthAxis`, hint 4, authored) |

### Larger bets

| ID | Feature | Size | Notes |
|---|---|---|---|
| F8 | **Scene Morph lane** (A → B) | M | 3 params (hint 4, preserved). Σv² = 1 makes the lerp safe; the plan must draw *effective* weights; an empty A or B hits the all-silent path |
| F9 | **OSC / ADM-OSC** (in first, then out + head tracking) | M–L, mini-milestone | One process-wide receiver hands `/adm/obj/{n}` to the claiming instance, on the message thread only, through the same gesture-bracketed write path so Logic records it. Expect a firewall prompt |
| F10 | **Perimeter path now; Drawn paths later** | S–M / milestone | Needs a new `motionSource` choice (None / Perimeter / Drawn), not a 7th `motionPath`. `hullPts` is already in `VenueSnapshot`; "Hop" (speaker-to-speaker per beat) falls out of the same traverser |
| F11 | **Modulation lane:** LFO or input envelope → width / blur / height / rolloff | M | ~5 params; bypass the PERF-02 skip-unchanged solve while active; DC must hold with modulation off |
| F12 | **Multi-instance overview:** ghost the other stems' pucks and paths on the plan | M | Same registry as F3; whether Logic passes track names is unverified |
| F13 | **Acoustic Measure:** ping + a mic → delays and trims | L, milestone | `VerifyPing` bursts as the reference; cross-correlation off the audio thread; pick the first arrival, not the peak |
| F14 | **HRTF fold, then head tracking** | L, milestone | 16 static IR pairs re-picked in `updateGeometry`; share embedded KU100 / SADIE data with O-simpleAmbisonics (check the licence); no SAF; opt-in on the stereo-bus fold |

**Lower priority:**
- Venue CSV paste, export and recent venues, plus **undo for venue edits** (`writeToState` already
  takes an `UndoManager*`).
- A companion "deliverable fold" plugin with an explicit Print.
- 10/12-speaker carriers (Logic 5.1.4 / 7.1.2 / 7.1.4) as a milestone with its own channel-map
  test matrix.

**Do not:**
- VBAP or ambisonic modes: they contradict the DBAP thesis, and O-Orbit already covers VBAP.
- `aumf` to receive MIDI: it orphans every session.
- A sidechain key bus: it reopens the channel-map matrix, and unrouted AU sidechains are silent.
- A print switch on the 8-channel monitor fold: it defeats its safety guards.
- A cue-index parameter that writes other parameters.
- Re-recording CU / DC to make a feature pass.
- Unlimited sources / channels as a goal.
- In-plugin AirPods tracking (the host owns the motion permission).
- Runtime SOFA / SAF.
