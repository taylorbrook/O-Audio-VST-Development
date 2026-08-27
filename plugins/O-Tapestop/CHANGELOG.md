# Changelog — O-Tapestop

All notable changes to O-Tapestop are documented here.

## [1.5.0] — 2026-08-26 — English/French hover help; the clamp gate now sweeps both languages

**No parameter, preset, state, DSP or layout change.** Every knob, every range,
every default, every factory preset and the 860 × 580 frame are exactly what
v1.4.0 shipped. What changed is where the hover-help copy LIVES, and the
addition of a language it can be read in.

### Hover-help copy moved out of the markup

All 33 tooltips left `index.html` and now live in a new `js/i18n.js` as a key
table, `{ key: { en: {t, b}, fr: {t, b, reviewed} } }`. **The English was moved,
not rewritten** — every `en` entry was extracted mechanically rather than
re-typed, and a comparison against the original markup confirms all 33 are
byte-identical, with HTML entities decoded because `setAttribute` +
`textContent` do not decode them.

The renderer is UNCHANGED. `showTooltip()` still reads `data-tip-title` /
`data-tip` off the anchor; those attributes are simply written at runtime by
`applyI18n()`.

**Keys are the anchor's own id, or its first id'd descendant.** 17 of the 33
anchors are `.knob-cell`, `.select-cell`, `.ratio-cell` and `.env-plate`
wrappers with no id, so the canonical `[selector, key, wrapper]` triple
addresses them. The id'd child rather than the wrapper's class, because the
sync/free swap slots put a `.select-cell` and a `.knob-cell` back to back under
the SAME tip title — "Spin-Down Time" appears twice, once as a note division and
once in milliseconds — so a class-based key would have collided.

### A settings popover, and the hover-help toggle moved into it

The gear takes over the absolute box the v1.4.0 `?` held — same `top`/`right`,
same 22 px circle, same palette — so the 860 × 580 layout, a PLAN Locked
Decision, is untouched: no sibling's box moves and the centred title does not
shift. Asserted by measurement, not assumed.

The panel carries two rows: Language (English / Français) and Hover help. **The
toggle MOVED; it is not duplicated.** It reads On/Off now rather than showing a
static `?`, so its caption is written from script for the first time — the copy
comes from `data-on` / `data-off` **authored in the markup**, never from a
literal in `app.js`.

The language persists with the session as a non-parameter property on the APVTS
state tree beside `tooltipsEnabled`, guarded on restore by `isVoid()` and read
with `toString()` — the same trap, handled the same way
(`critical_valuetree_xml_roundtrip_loses_type`). Bridge 13 → 15.

### All French is machine-drafted and UNREVIEWED

All 35 entries carry `reviewed: false`. No native speaker has read them.
`node scripts/check-i18n.js` prints the worklist. The terms most likely to want
a native speaker's judgement: `knob-TONE_TRACK` ("Suivi de timbre"),
`seg-char-wobble` ("Pleurage"), `engage-btn` ("Enclencher") and
`knob-CONT_DEPTH` ("Profondeur du mouvement").

### The clamp gate is parameterised by language, not duplicated

All six MODE × SYNC passes now run once for `en` and once for `fr`, in one
process against one page load, driven through `window.__setLanguage()`.
Assertion 3 (vertical) is the language-sensitive one: French wraps to more lines
inside the **unchanged** 230 px cap, so tips get TALLER and the above/below flip
has to catch what no longer fits above.

`max-width` is now PARSED from the plugin's own CSS rather than hard-coded,
because the cap differs per plugin; the literal survives as the drift guard. The
anchor count is derived from `TIP_BINDINGS` rather than by counting `data-tip=`
literals in the markup — that count is zero now, so the old assertion would have
passed vacuously against nothing. A seventh pass opens the settings popover so
`#lang-select` and the moved toggle are measured rather than skipped. Two
vacuity guards, both per-language: the clamp must engage at least once, and
every anchor's copy must actually differ between `en` and `fr`.

The `?`-glyph assertion was rewritten, not dropped: it compared the rendered
caption against a pinned literal, which cannot survive a deliberate change of
caption. It now compares the rendered caption against the `data-on` / `data-off`
attributes authored on the element — the rule it was always protecting.

### French geometry, MEASURED at the shipping 860 × 580

| | anchors | clamped | flipped below | widest | tallest |
|---|---|---|---|---|---|
| `en` | 35 | 12 | 4 | 230.0 px | 104.2 px |
| `fr` | 35 | 12 | 4 | 230.0 px | 119.1 px |

**French costs zero extra flips and zero extra clamps on the smallest frame in
the suite.** It is 14.8 px taller at its tallest, and every tip in both
languages sits fully inside the viewport with an 8 px margin.
`.tooltip { max-width }` was NOT touched.

### Not verified

* **Nothing has been checked in a real DAW.** Everything above is headless
  Chromium against the repo's own ui-stub, `auval`, and the offline C++ harness.
* **The C++ persistence path was never executed.** Nothing wrote `"fr"` into a
  session and read it back. The property sits beside `tooltipsEnabled`, which
  has shipped and worked since v1.4.0, but the language itself was not
  round-tripped.
* **The native bridge was exercised only against the ui-stub.**
* **No cross-platform check.** Windows/WebView2 has not been exercised.

## [1.4.0] — 2026-08-18

### Added
- **A "?" toggle in the header and hover help on all 33 controls.** The button
  sits `position: absolute` at the top-right of `.frame`, deliberately outside
  the header's centred flow — the 860 × 580 frame is a PLAN Locked Decision and
  a button placed in the header's normal flow would shift the title. Measured
  rather than assumed: the UI gate asserts the frame still renders 860 × 580,
  the title's centre is still the frame's centre (430 vs 430), and the page
  does not scroll.

  It ships **unlit**. The preference defaults to `false` in the processor and
  the page PULLS it at init, so a fresh instance is silent until asked.

  The toggle's own tip carries `data-tip-always` and bypasses the on/off gate.
  Without that exemption the one control able to turn help back on would be the
  one control unable to explain itself.

  Tip copy is per-segment for MODE, TIMING and CHARACTER rather than one tip per
  group: the three modes and the three characters are the plugin's central
  concepts and describing them three-at-a-time in one string was the worse read.

- **The preference persists with the session.** Not a parameter — an
  `AudioParameterBool` would appear in every host's automation lane and in every
  preset, neither of which is wanted for a help layer. It rides the state tree
  as a plain property beside `scratchEnvelopeJson`, and round-trips through two
  new native functions. The native-fn parity gate moves **13 ↔ 13 → 15 ↔ 15**
  (`PluginEditor.h`, `PluginEditor.cpp`, `js/app.js`, and the stub's whitelist
  all carry the count; the grep-diff is clean).

  `getTooltipsEnabled` is **pulled** by the page, never pushed from C++. A push
  from the editor constructor or the first 30 Hz tick fires before `js/app.js`
  has evaluated, so the preference would silently never arrive and the toggle
  would read OFF on every reopen — the O-FreqPulse WR-01 bug, avoided by
  construction here.

- **`tests/ui-stub/` and `tests/ui_tooltip_clamp_check.js`** — the browser
  render gate this plugin did not previously have. The stub mirrors all 19
  parameters, the 7 choice lists, the backend event bus and all 15 native fns;
  the check drives the real page at 860 × 580 and measures every tip.

### Fixed
- **The restore guard would have silently never restored anything.**
  `getStateInformation` writes `tooltipsEnabled` as a **bool** var, so
  `if (tips.isBool() || tips.isInt())` reads as obviously correct. It is wrong:
  the value goes out through `ValueTree::createXml` and comes back through
  `NamedValueSet::setFromXmlAttributes`, which rebuilds every property as
  `var (value)` over the attribute **string**. What returns is a var holding
  `"1"`, so a bool-or-int type test is false for every saved session and the
  preference restores as OFF forever.

  Caught by writing the round-trip probe rather than by reading the code — and
  the probe was written because the code looked right. `! tips.isVoid()` is the
  only correct test; `var`'s bool conversion handles all three forms once the
  property is known to exist.

### Verification
- **The tooltip clamp check did not discriminate at first, and saying so is the
  point.** Reverting `showTooltip()` to the naive measure-at-previous-offset
  form left all 33 anchor assertions still passing. The reason is arithmetic:
  every tip's copy is long enough to hit the 230 px `max-width`, and the
  horizontal clamp's fixed point at this frame width is
  `left = 860 − 230 − 8 = 622`, which leaves exactly 238 px to the right —
  enough for the next 230 px tip. The collapse can never start.

  That safety is a property of the **copy**, not of the code: one short tip is
  all it takes, because a narrow tip is placed further right and the next long
  tip measured at that stale offset re-wraps into a ribbon that never recovers.
  So a stress stage now manufactures the condition — short copy on the
  right-most control, then long copy back. With the fix the tip returns to
  230 px; with the naive version it renders **110.4 px**. Both directions were
  run (pattern_probe_must_target_the_branch_the_fix_changed).

  Same treatment for the restore guard: reverted to `isBool() || isInt()`, the
  round-trip probe fails in both directions; restored, it passes.

- Gates, all at the shipping geometry: 33/33 anchors measured across all six
  MODE × SYNC_MODE combinations (a single-pane sweep would leave a third of them
  unverified forever — the centre panel is three mode-switched panes and every
  duration control is a Sync/Free time-slot); the edge clamp **actually engaged**
  for 10 controls, so the run is not passing vacuously; the right-most tip (the
  "?" itself) ends at 852.0 of 860, exactly the 8 px limit.
- Render harness 69/69 probes, 0 failures — the 67 pre-existing DSP probes plus
  the two new state-round-trip ones. No DSP was touched.
- `auval` PASS, `pluginval` strictness-10 SUCCESS, zero console errors on load.

## [1.3.6] — 2026-08-17

### Changed
- **The factory-preset table is a base plus 28 override maps, not 28 full
  transcriptions.** Audit queue item 7, the last item in the queue and the one
  with the largest churn and the least behaviour. `PluginProcessor.cpp:236-523`
  spelled out 28 presets × 19 parameter IDs = 532 entries. Only **142** of
  those differed from a common baseline; the other **390** existed solely to
  satisfy the "every preset lists all 19 IDs" defence-in-depth rule. That rule
  is about what lands **on disk**, not about what a human types, so the table
  now declares a 19-entry `basePreset` and a `presetSpecs` list where each
  preset names only what it changes; a merge loop re-emits all 19 IDs.
  288 source lines → 159.

  Three details that make this a true no-op rather than a near-one:

  - **The base is the parameter defaults, not the statistical mode.** Taking
    the most common literal per ID would have cost 2 fewer override entries
    (140 vs 142) but would have set the base `MODE` to `2.0` (Continuous) and
    `TONE_TRACK` to `55.0` — values that match no default and would read as a
    lie to anyone treating `basePreset` as "what a preset starts from". The
    base is now verbatim the 19 defaults in `createParameterLayout()`, which
    makes it checkable against a single other place in the file. As a
    by-product "Classic Half-Bar Stop" now has an **empty** override map — it
    always was the defaults, which the old table's 19 spelled-out lines hid.

  - **The merge walks the BASE, not the overrides.** `params` starts as a copy
    of `basePreset` and each of its 19 entries looks *itself* up in the
    override map. It is therefore structurally impossible for a mistyped
    override ID to smuggle in a 20th key or drop a real one — the map's size
    never changes. (The old hand-written table had no such protection: a typo
    there would silently have produced 20 keys, one of them wrong.) A
    `jassert` names the typo in Debug; Release cannot be malformed either way.

  - **`envelope` defaults to `kDefaultWobbleEnv`.** 21 of the 28 presets ride
    it, so it is a defaulted struct member and only the 7 presets with their
    own scratch curve name one. Every preset still carries an explicit blob in
    the emitted JSON — the default is applied at the C++ struct, not skipped.

  Gated on byte-identity of the **generated** JSON, not on reading the diff:
  the render harness constructs `TapestopProcessor`, which writes all 28
  factory `.json` files; the directory was captured before the refactor and
  again after, and `diff -r` over the 28 files plus the version sentinel is
  empty (manifest SHA-256 `f05f9a6e…` on both sides). That covers the CR-02
  `convertTo0to1` round-trip on the three skew-0.35 `freeMsRange()` parameters
  — e.g. `STOP_FREE_MS` 4000 ms still stores `0.784240245819092`, the skewed
  value, not the linear `0.49937` a raw-fraction authoring would have given
  (`pattern_factory_preset_normalized_ignores_skew`).

  The gate was proven to be a discriminator rather than decoration
  (`pattern_probe_must_target_the_branch_the_fix_changed`): with one override
  entry deleted (`Half-Mix Stop`'s `MIX`) and one skew-0.35 value nudged by
  1 ms (`Slow-Tape Drag`'s `STOP_FREE_MS` 4000 → 3999), `diff -r` fired on
  exactly those two presets and no others — the 1 ms nudge moving the stored
  normalized value by 6.9e-5, which only the live skew conversion produces.

### Testing
- Render harness: **67/67 probe checks, 0 failures** — unchanged from v1.3.5,
  as expected for a table that emits identical bytes.
- Factory-preset JSON: 28/28 files still emit exactly 19 parameter IDs.

## [1.3.5] — 2026-08-17

### Changed
- **The double-ended debt clamp now has exactly one definition.** Audit queue
  item 6. It was transcribed verbatim at four sites — `TapestopTransport`'s
  carrier advance and fading-voice advance in `tick()`, `spliceCarrierTo()`,
  and `VarispeedVoice::read()` — each independently spelling out
  `maxDebt = ringSpan − kInterpGuard` and clamping to
  `[live − maxDebt, live]`. Four transcriptions of a safety invariant are four
  places for it to drift, so they all now call one
  `VarispeedVoice::clampToRing(pos, ring)`.

  Two details that make the extraction a true no-op rather than a near-one:

  - **Clamp order.** Three sites clamped `jmin`-then-`jmax`; `read()` clamped
    `jmax`-then-`jmin`. The helper uses `jmin`-then-`jmax`. The two orders
    agree bit-for-bit whenever the rails are ordered `lo <= hi`, which holds
    for every ring the plugin prepares, and they agree on NaN input as well
    (JUCE's `jmin`/`jmax` are `b < a ? b : a` / `a < b ? b : a`, so a NaN
    falls through both).
  - **No `jlimit()`.** The obvious one-liner carries a `jassert(lower <=
    upper)` that would fire on an unprepared ring, where `bufferSize == 0`
    puts the rails out of order. The explicit `jmin`/`jmax` pair keeps the
    existing assert-free contract — neither rail is an error path.

  The *single*-ended clamp at `release()` (SpinUp entry, CONTEXT decision 1)
  is deliberately left alone: it clamps only the lower rail and measures from
  `getTotalWritten()`, not `getTotalWritten() − 1`, so it is a different
  invariant, not a fifth copy of this one.

### Removed
- **Unreachable `case State::ScratchPass:` in `TapestopTransport::release()`.**
  The early `if (state == State::ScratchPass)` at the top of the function
  returns before the switch is reached, so the label was dead. The `default:`
  in the same group already covers the enumerator, so `-Wswitch` stays quiet;
  a comment now records that its absence is deliberate.
- **`chans` in `processBlock()`.** It existed only to feed
  `chR = chans > 1 ? 1 : 0` on the very next line, and
  `jmin(numChannels, 2) > 1` is just `numChannels > 1`. Inlined.

### Verified
- Render harness **67/67, byte-identical** to the v1.3.4 baseline
  (`f024b20980e57f4e…`), which is the acceptance criterion the audit set for
  this item. The one-line `[PresetManager] Factory presets initialized: 28`
  that appears on the first post-bump run is the version-sentinel rewrite
  (`OuariconPresetManager.h` stamps `JucePlugin_VersionString`), not a
  behaviour change — it is absent from every subsequent run.

## [1.3.4] — 2026-08-17

### Removed
- **Dead state deleted from three DSP headers.** Audit queue item 5. Pure
  deletion — no DSP, parameter, preset or enum change. Every symbol was
  re-confirmed unread by grep across `Source/` and `tests/` before removal:

  | Symbol | Was | Now |
  |---|---|---|
  | `VarispeedVoice::gain` | never read *or* written | gone |
  | `VarispeedVoice::active` | write-only — **9** assignments in `TapestopTransport`, zero reads | gone, with all 9 writes |
  | `ContinuousMotion::eventTargetR` | assigned in `reset()` only | gone |
  | `ContinuousMotion::eventForcedSnap` | write-only (4 sites) | gone |
  | `ContinuousMotion::slotCount` | member read only inside `recomputeSlots()` | now a local `const int` |

  Deleting `active` left `startXfade()`'s `VarispeedVoice* v` parameter
  unused — it was the only line that touched it — so the parameter went too,
  and its three call sites lost the argument. The force-complete policy
  comment that sat on the removed line is preserved on `fadingIdx = idx;`,
  which is where the drop now happens implicitly.

### Fixed
- **`ContinuousMotion::reset()` now covers the full state set.** It previously
  skipped `samplesSinceJump`, `lastXfLen` and `snapEmitted` (contrary to the
  audit note, `shuffleEmitted` was already covered). Latent only — `reset()`
  is reachable solely through `prepare()`, so today it always runs on
  freshly-constructed state where the member initialisers already hold these
  values. It was a trap for the next edit, and a re-prepare at a new sample
  rate would otherwise carry a stale fade length into the next engage.

### Testing
- Render harness **67/67 green, output byte-identical** to the v1.3.3 build —
  `sha256 f024b209…` on both sides, `diff` empty. This is the item's stated
  acceptance criterion: any waveform change would have meant something was
  load-bearing after all. The check is meaningful rather than cosmetic because
  the harness prints per-probe numeric diagnostics (`maxDiff`, bounds,
  deviations), not just PASS labels.
- The byte-identical result is structural, not lucky: every one of the 31
  harness call sites constructs a fresh `TapestopProcessor` immediately before
  `prepareAndSettle`, so the completed `reset()` only ever runs over values
  that already matched.

## [1.3.3] — 2026-08-17

### Documentation
- **Two comment blocks described the exact opposite of the splice law that is
  actually implemented.** Audit queue item 3 (B2a). No DSP, parameter, preset
  or enum-name change — both DSP headers are byte-identical from `#pragma once`
  onward, verified by checksum against the v1.3.2 backup.

  `SpliceLaw::EqualPower` is an equal-**gain** law, not equal-power:

  ```
  fadeOut = hann(0.5 + phi/2) = cos^2(pi*phi/2)
  fadeIn  = hann(phi/2)       = sin^2(pi*phi/2)
  fadeOut + fadeIn            = 1        <- AMPLITUDE sum, not power sum
  ```

  The identity `sin^2 + cos^2 = 1` does hold, but it constrains the sum of the
  *gains*, which is the amplitude sum. That inverts both claims that were on
  the page:

  | material | implemented (equal-gain) | true equal-power (sqrt of these gains) |
  |---|---|---|
  | correlated | `(fadeOut+fadeIn)^2 = 1` → **0 dB, flat** | **+3.01 dB** over-sum at midpoint |
  | decorrelated | `fadeOut^2+fadeIn^2 = (1+cos^2(pi*phi))/2` → 0.5 → **−3.01 dB floor** | 1.0 → **0 dB, flat** |

  - `Source/dsp/WindowLut.h` claimed "Equal-power by construction
    (sin^2 + cos^2 = 1)". It is equal-gain by construction.
  - `Source/dsp/TapestopTransport.h` claimed the law "over-sums CORRELATED
    material by up to +3 dB". That describes the true equal-power law, which
    is *not* implemented; an amplitude-sum-of-1 law sums correlated material to
    exactly 0 dB. Its real failure mode is the opposite one — a ~3 dB dip on
    decorrelated material.

  Because resync crossfades the live-head rider against a fading voice seconds
  behind it, the decorrelated (dipping) case is the one that actually applies
  at the splice. The `Linear` law is likewise amplitude-sum-1 and shares the
  same 3.01 dB decorrelated midpoint dip (`phi^2 + (1-phi)^2 = 0.5`); that is
  now stated too.

  The misleading enum name is **deliberately retained** — renaming it, or
  adding a real `sqrt`-based equal-power option, is audit item 4 (B2b), which
  changes how the plugin sounds.

- **Recorded the splice A/B measurement in the comments and NOTES.md rather
  than the analytic figure.** Running the harness to verify this doc change
  produced item 4's step-1 numbers, so they are captured here:

  | law | bump | dip |
  |---|---|---|
  | `AB-splice-equal-power` (raised cosine, shipped) | −0.48 dB | **−6.21 dB** |
  | `AB-splice-linear` | −0.58 dB | **−6.99 dB** |

  The near-zero **bump** on both laws is the empirical confirmation of the
  correction above: neither law over-sums correlated material. The **dip is
  ~6 dB, not 3 dB** — the 3.01 dB figure is the law's analytic floor for two
  *equal-power* decorrelated sources, while the real fading voice is a
  varispeed read of different material at a different level. The comments now
  say explicitly not to quote 3 dB as the plugin's resync dip.

### Testing
- Render harness: **67 probe checks, 0 failures.**
- Documentation-only change, verified rather than asserted: `WindowLut.h` and
  `TapestopTransport.h` are byte-identical to the v1.3.2 backup from
  `#pragma once` onward (SHA-256 of the code body matches), so no DSP,
  parameter, preset or enum change is possible in this commit.
- Built VST3 + AU; both bundles report `CFBundleShortVersionString` 1.3.3.
- Known gap left in place (item 4 step 3): `AB-splice-*` asserts only
  `|bump| < 4.0`, so the dip is reported but not gated — a dip regression
  would still pass.

## [1.3.2] — 2026-08-17

### Fixed
- **The engaged-trim blend never actually landed on 0 at the resync→Bypassed
  handoff.** OUTPUT_GAIN must never touch the Bypassed path (that is what makes
  the post-resync null bitwise), so the transport ramps `trimAmount` 1 → 0
  across exactly the ResyncXfade window and the processor applies gain as
  `1 + (g−1)·trimAmount`. Two independent defects meant it stopped short:

  1. **Target read on the wrong side of the state flip.** `TapestopTransport::
     tick()` flips the state to `Bypassed` *inside* the crossfade-completion
     block, and `trimTarget` was computed *below* it. On the final fade sample
     the target therefore read back as 1 and trim ticked **up**, ending at
     `2/xfLen` (8.3e-4 @ 48 kHz) instead of 0. `trimTarget` is now latched
     between the state machine and the completion block — the only point that
     sees `ResyncXfade` on every one of the fade's `xfLenSamples` ticks,
     including both the first (where `enterResync()` enters the state) and the
     last (where the flip to `Bypassed` happens). Latching on entry to `tick()`
     instead — the obvious reading — loses the first tick and lands on
     `1/xfLen`, so it is specifically the mid-point latch that is correct.

  2. **The ramp was a float accumulator, so the rails were rate-dependent.**
     `trim ± trimStep` clamped at the rails only lands *on* a rail if the
     accumulated rounding happens to overshoot it. Measured: exact at 44.1 /
     48 / 192 kHz, but leaving 4.07e-5 / 1.88e-5 / 6.73e-5 at 88.2 / 96 /
     176.4 kHz. The ramp is now an integer position over `[0, xfLenSamples]`
     with both rails snapped, so `0.0f` and `1.0f` are exact at every rate by
     construction and the intermediate values are the ideal linear ramp
     instead of a drifting one.

  The file header's claim that the applied gain "lands on EXACTLY 1.0 as the
  fade ends" and `reset()`'s documented Bypassed baseline of 0 are both true
  now; neither held before. Audible impact of the original defect was nil —
  `PluginProcessor.cpp` re-checks `isBypassed()` after `tick()`, so the final
  fade sample renders dry and never consumed the wrong value; the residue only
  offset the *next* engage's ramp (~0.02 dB on one sample at +12 dB).

### Changed
- Rendered output is **byte-identical at the shipped OUTPUT_GAIN default of
  0 dB** (verified: FNV-1a hash of a full engage→release→resync render matches
  v1.3.1 exactly, `77815f94d22f1c13`). At non-default OUTPUT_GAIN the trim ramp
  now follows the ideal linear shape rather than the drifting accumulator —
  worst case **0.0025 dB** difference, during the 50 ms resync fade only, at the
  +12 dB maximum.

### Testing
- New harness probe `B3-trim-exact-rails-6-rates`, driven at the transport
  level across 44.1 / 48 / 88.2 / 96 / 176.4 / 192 kHz. Transport-level and
  rate-swept on purpose: the final fade tick renders dry, so **no rendered
  waveform can discriminate the value**, and a 48 kHz-only probe would have
  passed straight over defect 2.
- Confirmed a real discriminator against both defects independently — pre-fix
  v1.3.1 fails at all 6 rates (8.9e-4 … 1.1e-4); the correct latch with the
  accumulator restored fails at exactly 88.2 / 96 / 176.4 kHz with the three
  predicted residues.
- Harness 67/67.

## [1.3.1] — 2026-08-17

### Fixed
- **Editing the scratch curve during a live pass could tear the envelope
  mid-pass.** In Scratch mode the transport latches the baked 2048-point
  envelope LUT at the ENGAGE edge and reads it for the whole pass — up to 8 s
  at ENV_FREE_MS max. `ScratchEnvelope` double-buffers, baking into "whichever
  of its two buffers is not currently published", which survives exactly ONE
  outstanding reader generation. The editor commits 50 ms after every
  pointer-up, so the SECOND curve edit inside one pass alternated back onto the
  buffer the audio thread was still reading: the playback rate stepped to the
  new curve's value mid-pass (an audible speed jerk, since a scratch `r` can
  swing across the full ±2 range) on top of a formal data race between the
  message and audio threads.

  Root cause: an ownership gap, not a missing lock — the audio thread held a
  raw `const float*` into memory the message thread was free to rewrite.
  `TapestopTransport` now owns the bytes it reads. `engageScratch()` memcpys
  the published LUT into its own `std::array<float, 2048>` at the engage edge
  (8 KB, no allocation, no lock — RT-safe) and reads only that copy, making the
  latch contract structural instead of probabilistic. Mid-pass edits now
  provably affect the next pass only, which is what the thread-contract comment
  in `ScratchEnvelope.h` always claimed; that comment has been corrected to
  state where the guarantee actually comes from.

  No parameter, preset, or state-format change — sessions and presets are
  unaffected.

### Testing
- New harness probe `scratch-envelope-edit-midpass-inert`: renders a 2 s scratch
  pass twice, once clean and once with two `commitScratchEnvelopeJson()` calls
  landing at 0.5 s and 1.0 s into the pass, and asserts the two renders are
  bit-identical, with a liveness check that the pass actually bends the pitch
  (a dry-vs-dry comparison would pass vacuously). The commits move `r` from 0.5
  to 1.5 to 2.0, so a torn read is a gross divergence rather than a rounding
  difference.
- Confirmed as a real discriminator, not a decorative assertion: built and run
  against the **pre-fix** source first, where it failed at sample 52096 — the
  exact sample of the second commit (engage 4096 + 48000). Post-fix it passes.
- Required a new `renderWithActions()` harness helper: `Event` carries only a
  float, so message-thread calls that are not APVTS writes could not previously
  be scheduled mid-render.
- Full render harness: 66 probe checks, 0 failures (65 pre-existing probes
  unchanged and still green, including every bit-identity and block-size
  invariance probe — the copy adds no waveform change).

## [1.3.0] — 2026-08-16

### Added
- **14 new factory presets — the bank doubles to 28.** New Tape Stops
  (Power Cut, Cassette Eject, Two-Bar Dive, Snap Back, Half-Mix Stop),
  Scratch shapes with four new envelope blobs (Transformer, Tape Rewind,
  Orbit, Crab Roll), Wobble & Warp (Tape Flutter, Pitch Tide, Loose
  Capstan), and Glitch & Chaos (Sputter, Data Rot). All authored in
  engineering units through the CR-02 conversion, every preset carries all
  19 param IDs + an envelope blob, ENGAGE stays 0 everywhere. The
  sentinel-gated factory writer re-runs on the version-string change, so
  the new files land on first scan.
- **Themed preset dropdown.** Clicking the preset name opens a grouped
  panel — Tape Stops / Scratch / Wobble & Warp / Glitch & Chaos, plus a
  dynamic User group for anything not in the factory map. The prev/next
  carousel is unchanged. Grouping is display-side only (`PRESET_THEMES` in
  js/app.js): `getPresetList()` stays a flat alphabetical sort, the preset
  JSON format is untouched, and the shared preset-manager module (C++ and
  JS) is unmodified. Panel is rebuilt on every open, closes on outside
  click / Escape / selection, and stays inert until the manager's
  initialize() resolves (same honest-disable contract as the buttons).

### Testing
- UI exercised in a Playwright browser harness with the JUCE bridge
  stubbed at `js/juce/index.js` (route interception; the stub feeds the
  real alphabetized 28-name factory list + 2 fake user presets): all 5
  groups render in order, 30 items, current-preset marker tracks
  selection, click-to-load closes the panel, Escape and outside-click
  dismiss, panel bottom lands at y=508 in the 580 px window and scrolls
  internally. Honest-disable held: with the stub absent the band and
  dropdown stay inert.
- auval PASS (aufx OTsp OuDv); pluginval strictness-10 SUCCESS on the
  installed VST3.

## [1.2.2] — 2026-08-16

### Fixed
- **Continuous pane overflowed the center panel — CHARACTER label crossed
  the left border, CHAOS knob crossed the right.** Root cause: the pane's
  34 px column gap was budgeted against the 58 px segment column, but the
  CHARACTER caption widens its column to ~71 px, so the centered flex row
  (71 + 3×88 + 3×34 = 437 px) overflowed the 398 px content box ~20 px on
  BOTH sides — invisible to the column-sum arithmetic, caught only by
  measuring rendered boxes. Gap reduced 34 → 16 px; measured clearance is
  now 7.3 px per side.

## [1.2.1] — 2026-08-16

### Changed
- **Glitch pushed further off the grid — more erratic at high CHAOS.**
  v1.2.0's barrage was dense but still grid-locked: every event started
  exactly on a slot boundary, which reads as rhythmic rather than erratic.
  All four changes are gated on `g = 2·max(0, chaos − 0.5)` (no RNG draw at
  g = 0), so CHAOS ≤ 0.5 stays bit-identical to v1.2.0:
  - **Slot-start jitter** — each slot's event attempt is deferred by a
    random 0–35 % of a slot (scaled by g), so high-chaos events fall
    off the tempo grid instead of quantizing to it.
  - **Density** — slots per cell now unlock 1→5 above CHAOS 0.4 (was
    1→4), max-chaos cadence up 25 %.
  - **⅛-cell micro-bursts** — a fourth event-length tier below the ¼
    fraction (weight 1.4·g·chaos²), and the ¼ weight itself rises with g
    (2.2 → 3.8 at max): more blink-length freezes/slams/stutter blips.
  - **Tame fade deepened** — dip/half-speed weights fade 65 % at max
    chaos (was 50 %), so the top of the CHAOS range is dominated by the
    extreme family.
  - Debt safety unchanged (3 s soft / 6 s hard budgets); measured 40 s
    worst-case debt 1.77 s (bound 8 s). Render harness: 65/65 pass with
    no bound recalibration — C-P6 glitch continuity 0.0576 vs 0.0904.

## [1.2.0] — 2026-08-16

### Changed
- **Glitch character overhauled — sudden, extreme, dense.** Root cause of
  the tame v1.1 sound: one event per grid cell, always full-cell length,
  with a mostly-smooth palette (shaped dips, flat half-speed holds).
  - **Sub-cell scheduling** — 1→4 event slots per cell unlock as CHAOS
    rises past 0.4, and event lengths draw from {1, ½, ¼}×cell with short
    bursts favored at high chaos: max-chaos Glitch is now a rapid-fire
    barrage instead of one texture per cell.
  - **New events** (unlock above CHAOS 0.5): dead-stop **freeze** (r = 0,
    instant resume), **slam** (holds the +2× engine rail), square-wave
    **chatter** (1 ± depth at 10–30 Hz), buffer-**shuffle** (spliced
    back-jump of 1–4 cells, replaying recent audio), and stutter
    **halving-roll** / **±2-semitone pitch-ramp** variants with per-event
    random slice depth (event/2..event/8 — micro-buzz at the bottom).
  - **Reverse-flick boosted** — chaos pushes the flick toward the −2 rail
    (was capped at −depth).
  - **Chaos remap** — tame-family weights fade 50 % as extremes ramp, so
    the top quarter of CHAOS reads as mayhem, not decorated wobble.
    CHAOS ≤ 0.4 keeps the v1.1 cadence; existing Glitch presets at high
    chaos ("Glitch", "Total Meltdown") now sound substantially wilder.
  - Debt safety unchanged: new events carry Δdebt signs in the exp bias
    (shuffle capped at 2 s + hard-budget headroom); 3 s soft / 6 s
    hard-snap budgets as before. Measured 40 s worst-case debt: 2.12 s.
- Render harness 62 → 65 checks: new C-P0x max-chaos probes (two-instance
  determinism, 512-vs-4096 bit-identity, post-release bitwise null with
  the full v1.2 palette reachable); C-P6 glitch continuity bound
  recalibrated from the r = 1.85 precedent to the ±2 rail (slam + ramped
  stutter splices legitimately exceed the old bound: measured 0.0576 vs
  old 0.0553 / new 0.0904 — click-detection power intact, a real click
  sits ~10× above the new bound).

## [1.1.0] — 2026-08-16

### Added
- **Continuous mode** (MODE gains a third choice) — tape-speed motion runs
  continuously while ENGAGE is latched, instead of as a single gesture.
  Three characters via the new **CHARACTER** parameter:
  - **Wobble** — deterministic wow sine + 3-harmonic flutter stack
    (ChowTapeModel motor-model ratios 0.56/0.20/0.24); CHAOS morphs toward
    per-cycle rate/amp jitter, slow Ornstein-Uhlenbeck drift and a filtered
    noise band.
  - **Random** — Ornstein-Uhlenbeck octave stack (RATE sets correlation
    time; CHAOS adds octaves and widens excursion), 20 Hz post-LP, plus a
    weak position-debt servo (±0.2 %, ~5 s time constant) so a long hold
    never walks into the capture-ring rail.
  - **Glitch** — tempo-grid event scheduler (p = chaos² per cell):
    tapestop-dip, half-speed drag, speed-jump, reverse-flick, stutter-repeat
    (2-voice splices at slice/4, 3–50 ms), resync-snap. Event selection is
    debt-biased with a 3 s soft budget and 6 s hard resync-snap, so debt is
    self-centering by construction.
- New parameters: `CHARACTER`, `CONT_RATE_SYNC_DIV` / `CONT_RATE_HZ`
  (Sync/Free twin, 0.05–20 Hz), `CONT_DEPTH` (log-perceptual 0.1 %→12 %
  peak speed deviation ≈ ±2 cents→±2 semitones), `CONT_CHAOS`. DEPTH/RATE
  are live (16-sample absolute grid); CHARACTER + RNG seeds latch at the
  engage edge (deterministic, repeatable bounces).
- Release from Continuous rides the existing SpinUp → Catchup → ResyncXfade
  path — START time/curve shape the return, post-resync output stays
  bitwise dry.
- 6 new factory presets: Subtle Wobble, Warped Record, Drunk Tape, Seasick,
  Glitch, Total Meltdown (14 total).
- UI: MODE segment is 3-way (Stop / Scratch / Motion); new Continuous pane
  (Character stack, Rate slot, Depth + Chaos knobs). The three mode buttons
  stack vertically at full ENGAGE width (140px) — the first cut squeezed
  them side-by-side into thirds of the 148px TRIGGER content box, which
  clipped SCRATCH/MOTION even at 8px type.
- Render harness extended 47 → 62 checks: per-character determinism /
  block-size invariance / post-release null, 40 s debt-bound probes,
  continuity (first-difference) scans, depth liveness + zipper probe,
  preset-migration probe.

### Changed
- Preset-manager module v1.0.6: optional `setMigrationCallback()` hook,
  invoked with each preset's parameters + saved version before apply.
- Presets saved by v1.0.0 are migrated on load: MODE was stored normalized
  over 2 choices — 1.0 (Scratch) would decode as Continuous over 3. The
  migration remaps pre-1.1.0 MODE fractions (round(n)·0.5).

### Known caveats
- VST3 automation lanes of MODE recorded before 1.1.0 store the normalized
  value host-side and cannot be migrated: a lane sitting at 1.0 (formerly
  Scratch) now selects Continuous. MODE is a setup control (ENGAGE is the
  performance param), so exposure is expected to be rare.
- At MIX < 100 %, Continuous mode blends wet near-unity-speed audio against
  dry — tape-flanging combing is audible by design.

## [1.0.0] — 2026-08-15

Initial release.

### Added
- **Stop mode** — tape-stop / tape-start varispeed transport: tempo-synced
  (1/16 – 4 bars) or free-time (10 ms – 8 s) spin-down and spin-up with
  independent curve shaping, Signalsmith-style resync (1.25× catch-up +
  crossfade skip, bitwise-dry post-resync null).
- **Scratch mode** — drawable bipolar speed-vs-time envelope (2–64 points,
  per-segment curvature), r ∈ [−2, +2], tempo-synced or free pass length.
- **Tone Track** — speed-tracking low-pass that darkens the wet path as the
  tape slows, wet-path/engaged-only.
- **Mix / Output Gain** on the engaged chain only; the Bypassed state is a
  true bitwise pass-through.
- **WebView UI** (860 × 580, Ouaricon Naturalist): large latching ENGAGE,
  mode-switched center panel, drawable envelope editor with live playhead,
  live playback-ratio readout.
- **Preset system** — shared preset-manager module v1.0.5: save / save-as /
  load / load-from-file / prev / next / two-click delete, factory + user
  banks under `~/Library/Ouaricon Tapestop/Presets/`. The scratch envelope
  rides inside preset JSON as an opaque `customState` blob.
- **Factory bank** — 8 presets: Classic Half-Bar Stop, Classic 1-Bar Stop,
  DJ Spinup, Baby Scratch, Chirp Flare, Tempo-Synced Short Stop,
  Slow-Tape Drag, Stutter-Scratch.
