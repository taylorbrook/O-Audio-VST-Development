# Stage O — the defects Stage N found by reading the French against the code

Stage N (2026-08-31) changed French only. Its executors read every French entry against
the English AND the processor source, and found the defects below in the ENGLISH, the DSP,
the CSS and the test data. Each is plugin-local. This brief is the spec for fixing them:
**one executor per plugin, one patch bump per plugin, 5 executors concurrent, builds on
the Stage K mutex** (`/tmp/claude-501/stagek-build.lock`, `mkdir` to take, `rmdir` after
`auval`).

Every executor reads first: this file; `260826-ieq-STAGE-N-BRIEF.md` sections "The eleven
steps" (steps 5–11 — geometry, render, gates, header, version, build, commit) and
corrections 7, 8, 14, 21, 26, 31, 35, 44, 45; and `CLAUDE.md`. Rules carried from N:
`git commit -- plugins/<Name>` only, `<sha>^` not `HEAD~1`, `touch CMakeLists.txt` before
the build and read the INSTALLED `Info.plist`, scratchpad under `scratchpad/<Name>/`,
never `scripts/` / `modules/` / `PLUGINS.md` (report the row), no tag, no push, no
`--amend`, no `git checkout --` to restore a plant.

**French rule:** any English copy you change gets its French changed in the same commit,
and `node scripts/i18n-fr-lint.js --plugin <Name>` (a gate now — any finding exits 2) and
`node scripts/check-i18n.js --plugin <Name>` must pass. New French entries are
`reviewed: false` (the developer reads French and will flip them).

**Gates, every plugin:** the plugin's own `tests/` (render gate or clamp gate),
`check-ui-labels --plugin`, `check-i18n --plugin`, `i18n-fr-lint --plugin`, `boot-all-uis`
(43/43, 0 DEAD, this plugin's late count unchanged), build + `auval`, installed plist.
A DSP change additionally runs the plugin's render harness if one exists and states the
measured before/after.

## Per plugin

### O-Freeze — item 48 (DSP) — the one that changes sound
`PluginProcessor.cpp:582` applies DETUNE as `playbackRate = 1 + r·(detune/1200)`; cents are
`2^(c/1200)`. Knob 50 → ±70.67 ct actual (measured ~1.44×). Fix the math to
`std::pow(2.0f, r*detune/1200.0f)`. This changes the sound of existing sessions at the same
knob value — say so in the CHANGELOG under Changed, name the ratio. Verify with a pitch
measurement before/after (knob 5/25/50, expected ±5/±25/±50 ct); the render harness if
present. Minor bump, not patch.

### O-simpleFM — item 30 (host-visible labels)
`FMVoice.h:210` computes `fm = carrierHz * ratio` — the knob is MODULATOR:CARRIER. The
caption `Ratio C:M`, the tip title `Ratio (C : M)`, the automation-lane name
(`PluginProcessor.cpp:63`) and the `FactoryPresets.cpp` comments are backwards; the tip
body and the Clarinet lesson are right. Fix the labels to M:C (keep the parameter ID —
sessions must load). French follows.

### O-simplePhysicalModelSynth — items 53, 54
53: `stringModel` is never `load()`ed (`StringResonator.h:33` defers the waveguide); its
tooltip promises "v1.1" on a 1.2.1 plugin. Recommended: keep the parameter (session
compatibility), hide its dropdown, make the tooltip say it is reserved — or implement the
waveguide if the DSP is ready, which is a bigger job: report which you did and why.
54: `aria.helpToggle` "Toggle tooltips" vs tip title "Hover help" — one name.

### O-Polystutter — item 34
`midi` tooltip says C1–B1 trigger lanes 1–4 and any other note triggers all;
`TriggerRouter.cpp:76-85` routes notes 60–63 and only 67 triggers all. Fix the body (both
languages) and the wrong source comment at `:69-74`.

### O-Tremolo — item 35
`tip.panSync` says a stereo *signal* is needed; `PluginProcessor.cpp:345` duplicates mono
into channel 1 on a 1→2 bus and `:352` gates on `numChannels == 2` — the condition is the
BUS. Reword both languages.

### O-Bassoon — item 36
`tip.breath` says CC2 "takes over"; `BassoonVoice.cpp:167` is `ui_breath × cc2` — with the
knob at 0 a breath controller does nothing. Reword to match `aria.breathMeter`, which is right.

### O-Gain — item 37
`tip.info-confidence` states only the 5 s / 15 s thresholds; `kConfidenceMinBlocks`
(`PluginProcessor.cpp:133`, `:1027`) also forces LOW under 50 gating blocks. Add the
condition (both languages); watch the tip height — 350×500 frame, read heights before/after.

### O-Formant — item 63 (+ item 22 if clean)
63: `tip.formantSpread` says "around the first one"; `FormantFilterBank.h:100-107` and
`CascadeFormantBank.h:98-116` scale around the centre of mass of all five. Reword.
22 (optional, larger): A4 tuning lost on session reopen (M3 finding 7) — C++ state
restore; take it only if the fix is clean and measured with a save/reopen probe.

### O-Bells — item 66
`tip.partialTuning` says "the upper partials"; `BellVoice.cpp:757-760` moves the tierce
(partial 2) only. `tip.damping` overstates scope (live path: partials 0–1 in the hum stage,
`:1034-1038`, plus release `:404`). Reword both. Remove the dead `ModalPartial::decayRate`
write (`BellVoice.h:119`, `BellVoice.cpp:920`) — its inverted mapping is a trap; DSP-neutral
by construction, confirm with the render harness golden.

### O-Bowed — items 41, 42
41: 15 canvas `fillText` strings in `index.html:1468-1866` are English literals in both
languages (*Bridge*, *Nut*, *Speed: 0.20 m/s*, *Bow Pressure (N)*, *Helmholtz*…). Key them
in `I18N` with empty bodies (the O-Comp shape) and paint through `tr()`; verify with a
`fillText`-recording probe en→fr→en and `measureText` widths on the canvas.
42: Sympathetic Decay stays visible at Count 0 where Sympathetic Amount is hidden by
`updateSympVisibility()` (`index.html:1395`) — hide both, or say in the tip that it is
conditional; match whatever Amount does.

### O-Lyrica — items 50, 51, 58
50: `.footer` (`position: absolute; z-index: 10`, y 392–447) paints over
`#sympatheticAmount` / `#sympatheticQ` (y 386–410) — the sliders lose their drag surface
and hover-help. Fix the layout (footer height/position or slider group), prove with
`elementFromPoint` at the slider centres before/after, `check-ui-labels` 0 moved.
51: `technique` body says "Harmonics"; the Choice face is "Harmonic".
58: no focus latch — port the Stage M `lastInputWasPointer` latch (see O-Comp v1.7.0
`setupTooltips`): a pointer click opens no tip, keyboard focus still does.

### O-Tapestop — items 38, 39
38: three division selects carry `aria-label` "Stop Time" / "Start Time" / "Env Length"
(`index.html:228, :253, :294`) against tip titles *Spin-Down Time* / *Spin-Up Time* /
*Pass Length* and caption *Division* — one name per control, in both languages.
39: `PRESET_THEMES` (`app.js:597-605`) writes English headings via `textContent` on the
French page — key them (`setLabel`), French entries added.

### O-simpleGrain — items 54, 57, 58
54: `aria.helpToggle` "Toggle tooltips" vs "Hover help". 57: `tests/i18n-states.json`
names `captionAsyncCloud` as the longest caption; `captionPitchedBuzz` (838.58 px) already
wraps in French and moves the rack 13.19 px in a state no gate drives — add that state and
fix the caption (shorter, same claim — correction 43). 58: focus latch.

### O-simpleSampler — items 55, 58
55: `tip.pitchMode` and `tip.lessonRepitchStretch` both open "The headline A/B." — give one
of them its own opening; `#pitchModeReadout` wraps to two lines in French in the Stretch
state (drive that state in `tests/i18n-states.json`, then fix width or copy). 58: focus latch.

### O-simpleAdditive — item 58: focus latch.   ### O-Orbit — items 44, 58
44: `.toggle-label { font-size: 9px }` loses on specificity and is dead — decide (raise
specificity = visible size change on a shipped control, or delete the rule and update the
width comments). 58: no `focusin` handler at all — add the latched one.

### O-Contrabass — item 61
`note-expression-toggle` tip title "Note expression" vs caption / VST3 feature "Note
Expression" — capitalise the title. (Two stale AU registrations `OCb5` / `OCbP` on this
machine are not repo work.)

### O-FreqPulse, O-SpectralShaper — item 40
`.settings-toggle { min-width: 40px }` no longer covers *Marche* (36.97 px in a 22 px
content box) / *Désactivée* (61.88): the button resizes between its own French faces. Pin
~55–62 px (measure both faces), `check-ui-labels` 0 moved in both languages, popover width
unchanged. Update the CSS comment.

### O-SimpleReverb — item 47;   O-Wind — items 62, 65
47: footer wordmark and console banner hard-code `v1.5.5` (`index.html:917`, `:1851`) inside
a text-matched `I18N_EXEMPT` entry — replace with a runtime `#versionLabel` span filled from
the version (the O-DigiDelay / O-Tremolo shape), update the exemption.
62: `tests/i18n-states.json` opens the settings popover first; it covers the Effects tab
button (popover 698,39 190×40 over the 300 px tab at 600,40) so the tab click times out and
25 of 65 captions have never been measured — reorder the states (tab first, popover last).
65: `[Effects] Panel initialized (v1.14.0)` console banner is stale — make it read the
version.

## Batches (5 concurrent; the mutex serializes builds)
- **O1:** O-Freeze, O-simpleFM, O-Polystutter, O-Tremolo, O-Bassoon
- **O2:** O-Gain, O-Formant, O-Bells, O-Bowed, O-Lyrica
- **O3:** O-Tapestop, O-simpleGrain, O-simpleSampler, O-simpleAdditive, O-Orbit
- **O4:** O-Contrabass, O-FreqPulse, O-SpectralShaper, O-SimpleReverb, O-Wind, O-simplePhysicalModelSynth

## Not in scope (bigger than "small")
Item 31 (committed render gates for 17 plugins), item 59 (guillemet spacing rule +
43 edits), item 19 (O-Prism's 64 mod-matrix rows), item 8 (O-Reed's dead `instrumentPreset`
/ oversampling latency), item 2 (hover-help toggle uniformity), item 11 (keyboard reach).

## Report back, per plugin
Version + sha + files; each item: fixed / not fixed and why, with the measurement or probe
that proves it; gates; installed plist; the PLUGINS.md row (not written).

---

# CARRIED FROM O1 (O-Freeze `89f82a6a`, O-simpleFM `aa76ef71`, O-Polystutter `c5f42cf7`, O-Tremolo `00b98b5e`, O-Bassoon `97db3b04`)

Where this section disagrees with anything above, THIS section wins.

1. **The brief's French is a suggestion; the glossary and your Stage K header are the spec.**
   O-simpleFM's target `Ratio M:C` ignored the settled *Rapport* and *P* (porteuse). Read
   `scripts/i18n-fr-glossary.js` and the header before you write a French label.
2. **Verify every "X is right" claim in your section against the source** before leaving it.
   The Clarinet lesson the brief called right opened with the backwards ratio.
3. **A measurement that cannot show the expected number is telling you about a second bug.**
   O-Freeze's knob-5 grains sat on a 3.38 ct grid: a `float` fractional read position at a
   1000 ms grain. If your plugin is granular (O-simpleGrain), READ the fractional-position
   type and REPORT it (item 69) — do not fix it in Stage O unless your item is DSP.
4. **Tip bodies grow by a line; read the height in both languages before/after** and state the
   clearance. O-Bassoon's breath tip went 136.5 → 219.9 px inside a 260 px cap; on O-Gain's
   350×500 frame that margin does not exist — split a sentence before you overflow.
5. **Dev AU triple is `<subtype> OuDv`** (`auval -v aumu/aufx <Sub> OuDv`); a wrong guess inside
   the lock wastes the mutex. Take the lock for build + auval only (O1 held it ≤ 2 min each).
6. **A brief's note-name convention may contradict itself** — the plugin's own docs and
   `KeyboardComponentBase::octaveNumForMiddleC` settle it; carry the note numbers too.
7. **Set `reviewed: false` on every entry whose French MEANING you change**, not only on new
   entries. O1 flipped 7 (O-simpleFM), 1–2 elsewhere; the developer re-reads them.
