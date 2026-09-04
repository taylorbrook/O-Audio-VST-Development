# O-MicrotonalSampler Changelog

## [1.26.1] - 2026-09-03

The French rendering of the hover-help surface changes suite-wide (task
260903-ukp; O-Gain 1.3.3 was the tracer). PATCH: French strings and source
comments only — no parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (feminine plural). The superseded
  rendering named the ACTION — help on hover; *infobulle* is the noun French
  DAW and OS interfaces use for the surface itself. The glossary root moved
  with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now reads
  `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural**, not
  substituted: `cette …` → `ces infobulles`, `l’…` → `les infobulles`,
  `de l’…` → `des infobulles`, `toute l’…` → `toutes les infobulles`,
  `Une fois désactivée` → `Une fois désactivées`; the distributive `chaque …`
  → `chaque infobulle` is the one place the new term stays singular.
  Bare back-references that carried no occurrence of the old phrase — clauses
  reading *le réglage de l’aide*, *l’état de l’aide*, *son affichage ou non*,
  and the pronouns in *Lorsqu’elle est désactivée … la réactiver* — were
  rewritten too. A regex pass would have left every one of them pointing at an
  antecedent that no longer exists.
- Every changed body was read by the developer at a blocking checkpoint
  *before* it was written, so each ships `reviewed: true` legitimately and the
  repo-wide unreviewed-French TOTAL stays at 0.


## [1.26.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `oms.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help
  layer, so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol
  / Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies **y 51.5..128.5, 208 x 77 px — byte-identical in English and French** — inside a 900 x 640 frame. The switch face grows 42.00 -> 50.66 px for *Marche*, leftward into the panel's own slack; `check-ui-labels` [7] reports 0 non-label elements displaced.
- **The switch's face is a `min-width: 42px` floor, not a pinned width**, so a
  longer French face grows LEFTWARD into slack the popover already has. The row
  is `space-between` and the button is a `[data-i18n]` node, so nothing the
  geometry gate measures moves. `check-ui-labels` [7] reports **0 non-label
  elements displaced** between English and French, and the visible element set
  identical in both.
- Every declaration in `.settings-toggle` above the four switch-specific ones
  is **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Decided

- **Default is ON.** The previous version showed hover help unconditionally, so
  ON is the setting that leaves an existing user's plugin behaving exactly as
  it did. Default OFF would additionally have made `boot-all-uis --strict-tips`
  measure an empty tip surface and call it correct.

### Not reused

- **This page has its own `<input type="checkbox">` controls and the switch is
  NOT one of them.** It is a `<button class="settings-toggle">` with
  `aria-pressed`, matching the twenty-two sibling plugins rather than this
  page's local checkbox vocabulary — a checkbox and a pressed-state button are
  different things to a screen reader, and the switch is the latter.


## [1.25.2] - 2026-08-31

The tuning panel's A4 REF knob now reads the engine. Sweep of the tuning-panel
family for the O-Formant item 22 gap (quick task 260831-wq3, SUMMARY item 71).

### Fixed

- **The A4 REF readout lied about the pitch the plugin was sounding.** The
  **state** half here was already correct — `captureTuningValueTree`
  (`PluginProcessor.cpp:2844`) saves the engine's `masterTune` and
  `restoreTuningFromValueTree` (`:2876`) puts it back — so a session saved with
  A4 at 442 Hz genuinely reopened with the engine at 442. What was missing was
  the **panel** half: `Resources/ui/js/tuning-panel.js` had no read path at all.
  `loadInitialState()` fetched intervals, scale name, tonic and octave stretch
  and never asked for A4, so the knob kept the `440.0 Hz` its own markup ships
  with while the engine sounded 442. `Source/PluginEditor.cpp` gains a
  `getMasterTune` native fn beside `getOctaveStretch` (the editor registers a
  table of `{ "name", lambda }` pairs, not a `.withNativeFunction` chain), and
  `loadInitialState()` now reads it in its **own** try/catch — guarded to a
  finite number, because the ui-stub invents a value for any native fn it does
  not know, and clamped to the knob's own [400, 480] domain. The separate try
  matters: a throwing A4 read must not skip `updateIntervalList()` /
  `updateVisualization()` and blank the whole tuning tab.
- **Every A4 drag restarted from 440, so a second drag threw the first away.**
  `setupRefPitchKnob()` seeded `startValue` from a literal `440` and never
  updated it. It now tracks `currentHz`, written by `updateKnob`, and
  `mousedown` starts each drag from the value the knob currently shows.
  `updateKnob` is published as `this.updateRefPitchKnob` so the load-time read
  can drive it; `attachEventListeners()` runs before `loadInitialState()` in
  `init()`, so the hook is always present by then.
- **Probe** (page-level, mounts the vendored panel module in Chromium against a
  hand-written `juce` stub, so no tracked file is touched):
  with `getMasterTune` → 442, `#ref-pitch-value` read **`440.0 Hz` before** and
  **`442.0 Hz` after**; two identical drags ended **450.0 / 450.0 Hz before**
  (equal — the second discarded the first) and **452.0 / 462.0 Hz after**. The
  control arm, `getMasterTune` → 440 expecting `440.0 Hz`, passed both ways, so
  the failing assertion is discriminating rather than always-red.

### Notes

- No parameter, preset-format or state-tree change; no user-visible English or
  French copy change, so no i18n edit. `getMasterTune` is a read-only native fn.
  The existing `setMasterTune` (which writes the engine under the persistence
  lock) is untouched. `modules/` and `scripts/` untouched — this plugin's
  `tuning-panel.js` is plugin-owned and vendored, and CMake embeds this copy.

## [1.25.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **73 French values revised** of 290, across all 270 entries, against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint: 28 terminology, 38 typography, 2 casing,
  5 restored meaning. Lint findings 74 → 1. The visible ones: the Decay knob now reads
  **Déclin** rather than *Chute*, which is also what its own tooltip already said; the
  Release tooltip title is **Relâchement** rather than *Extinction*, matching the
  **Relâch.** on the knob; every *Vider* is **Effacer**; the Tuning tab is **Accord**;
  the tuning panel takes the settled shared forms — **Intervalles de la gamme**,
  **Bibliothèque de gammes**, **RÉF. A4**, **Rotation**, **Non octaviantes**,
  **Tenir 2 notes ou plus…** — the same strings O-Bells, O-IntonationPad, O-Lyrica and
  O-Prism carry over the same panel; and four occurrences of *plugiciel* are now
  **plugin**, the settled masculine.
- **One French name per control.** Dynamic Range had three — *Ét. dyn.* on the knob,
  *Plage dynamique* in its tooltip, *Amplitude dynamique* in its accessible name. It is
  **Plage dynamique** everywhere now, with **Pl. dyn.** on the 58 px knob caption.
- **French typography throughout**: a no-break space before `% : ; ! ?` and between a
  number and its unit (*0 à 10 s*, *−24 à +24 dB*, *0 à 100 %*), so a value and its unit
  can no longer be split across a line.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so
  assistive technology reads the page in the language it is displayed in.

## [1.25.0] - 2026-08-30

### Added

- **Hover-help, in both languages.** Twenty tooltips — eighteen parameters plus the settings
  gear and the language selector — with a title and a two-or-three-sentence body that says
  what the control does, when to reach for it, and its range and unit. All French is
  machine-drafted and flagged `reviewed: false`; no native speaker has read it.
  `node scripts/check-i18n.js` prints the worklist.
- **A tooltip renderer, because there was none.** Canon v2's `applyI18n()` writes
  `data-tip-title` and `data-tip` ATTRIBUTES onto the bound anchors and stops there; the code
  that reads them and paints a surface is per-plugin, and at v1.24.0 this page had no
  `#tooltip` element, no `#tooltip` rule and no hover handler. Authoring the copy without it
  would have shipped twenty invisible strings past three green gates —
  **measured: with `setupTooltips()` disabled, `check-i18n --plugin O-MicrotonalSampler`
  still prints ALL CHECKS PASS while the new render gate fails 50 assertions.**
  `setupTooltips()` in `js/sampler-app.js` is ported from O-simpleFM: delegated on
  `document`, cursor-following, flipped then clamped on all four edges at an 8px margin,
  built with `createElement` + `textContent` so localized copy never reaches a markup path.
- **A last-input-device focus latch**, which the O-simpleFM reference does not have. A mouse
  click on a `<button>` focuses it, so an unconditional `focusin` rule parks a tip on screen
  after every click. `:focus-visible` is not the discriminator — Chromium reports it false
  for a programmatic `.focus()` after a click, so a gate driving focus directly would measure
  "no tip" and record that as correct.
- **`tests/ui_tip_render_check.js`** — 349 assertions at the shipping 900x640 frame. No gate
  in this repo can see a rendered tooltip: `check-i18n` reads the table statically,
  `check-ui-labels` has no tooltip awareness at all, and `boot-all-uis` counts `aria-label`
  and `title` and never `data-tip`. It hovers every anchor in English and again in French,
  byte-compares the rendered title and body against the table, and measures the rectangle
  against all four edges. Three negative controls: an over-long planted body sized against
  THIS frame (4200 chars, measured 260 x 1357.5px, 725.5px past the bottom); the focus latch
  in both halves; and the `pointerout` child-boundary rule.
- **`.planning/params.tsv`** — the runtime parameter inventory, from a walk of
  `AudioProcessor::getParameters()` on a constructed processor rather than a regex over
  `createParameterLayout()`. Nineteen parameters. Wired behind a new
  `OUARICON_BUILD_TESTS` cache option in `CMakeLists.txt`.

### Changed

- `Source/PluginProcessor.cpp` — `#include "PluginEditor.h"` moved from the top of the TU to
  behind `#if JUCE_WEB_BROWSER` directly above `createEditor()`, with a
  `GenericAudioProcessorEditor` fallback, so the param-dump console target (which builds with
  `JUCE_WEB_BROWSER=0` and no editor sources) links. Under a normal build
  `JUCE_WEB_BROWSER=1` and behaviour is byte-identical to v1.24.0.

### Not changed, deliberately

- **The 51 body-less `I18N` entries.** They are toasts, dialog copy and composed accessible
  names reached through `trLabel()`, and the Stage K4 decision put them in `I18N` with
  `b: ''` precisely so they would not demand a `TIP_BINDINGS` row. Verified: zero non-empty
  `b` outside the new `tip.*` keys, asserted in the render gate so it cannot drift.
  The consequence is worth recording — the first authored body flips `check-i18n`
  assertion 2 from "0 tips bound is a state, not a gap" to "every bodied entry must be
  bound", so the orphan check now runs against a table where 51 entries are legitimately
  body-less.
- **`rr_mode` has no tooltip, because it has no control on this page.** Round-Robin Mode
  (Cycle / Random No-Repeat / Random) is automatable and host-reachable and page-unreachable
  in every version. A body for it could not be bound and would fail assertion 2 as an orphan.
  A control was NOT added to satisfy the count: that is a feature change with a geometry cost
  on a control strip whose knobs are already `flex: 1 1 0` at `min-width: 56px`.
- **The preset bar, the tab strip, the drop zone and the six dialogs get no tips.** They took
  accessible names from their deleted native `title=` attributes at v1.24.0 and are
  self-describing; tips there are polish, not this stage's scope.

### Geometry

- **Zero movement.** `check-ui-labels --plugin O-MicrotonalSampler` is byte-identical before
  and after, across all 22 driven states, with `moved=0` throughout — controlled by re-running
  the gate against the `HEAD` sources in the same session, because the first baseline run had
  caught the `#anatomyOverlay` parallax transition mid-flight and reported a 0.6px
  self-animation NOTE that the post-change runs did not. The `HEAD` re-run matched the
  post-change output exactly, so that NOTE is a run-to-run transient of the page's own
  animation and not a consequence of this release.
- **No pin was added, so none is owed a negative control.** A hidden `position: fixed`
  surface moves nothing; that is why it is `fixed` rather than merely off-screen.

## [1.24.0] - 2026-08-30

### Added

- **The page speaks French.** A language selector in a new settings gear switches every
  caption, section heading, tab name, button face, dialog, panel title and accessible name
  between English and French. All entries are machine-drafted and flagged `reviewed: false`;
  no native speaker has read them. `node scripts/check-i18n.js` prints the worklist.
- **`Resources/ui/js/i18n.js`**, the string table, added to the EXISTING
  `O-MicrotonalSampler_UIResources` target — a second `juce_add_binary_data` call would have
  collided on the default `BinaryData` namespace. Four places, one commit: the file on disk,
  the `SOURCES` list, a `getResource()` branch, and the `import` in `js/sampler-app.js`.
- **`getUiLanguage` / `setUiLanguage`** native functions, with the choice persisted on the
  APVTS state tree. A non-parameter value round-trips through XML as a **string** `var`, so
  the guard is `isVoid()`, not `isBool()`.
- **The Tuning tab is localized too.** `Resources/ui/js/tuning-panel.js` is a plugin-owned
  copy — its header reads "part of O-MicrotonalSampler" and it is 317 lines diverged from
  `modules/tuning/scala-tuning-engine` — so its ~34 strings are in scope for this plugin.
  `scripts/i18n-extract.js` skips the filename unconditionally, so they appear in no
  extractor count and had to be enumerated by hand. This deliberately widens an already
  large divergence from the module.

### Fixed

- **Eight JS-written native `title=` attributes deleted** (contract §4). They were written
  from grid-render paths in `sampler-app.js`, so a handful of source lines produced a much
  larger number of rendered tooltips — untranslated OS tooltips competing with the page's own
  help. `boot-all-uis` now reports `title= 0`. These were invisible to `check-i18n` until
  assertion 11 was widened to read the JS as well as the markup.

### Changed

- **Inline English pluralization removed, not ported.** `${n} file${n === 1 ? '' : 's'}` and
  its siblings are gone: French pluralizes 0 as singular where English does not, so a
  mechanical port is wrong at n=0 before it is wrong anywhere else. The copy is authored
  around the inflection per contract §6 — the count sits after a colon beside an invariant
  plural noun phrase that reads correctly at 0, 1 and n in both languages.
- **Geometry: 116 non-label elements moved between English and French; now 0**, across all
  22 driven states, measured at both 180 ms and 1.7 s. Every pin is sized to a measured
  width in these elements and was reverted alone and confirmed to re-break the gate.
  Nine French strings were shortened rather than pinned where a pin would have opened a
  visible gap in prose or grown an English box.

### Notes

- `tests/i18n-states.json` (22 states) and `tests/ui-stub/generic-overrides.json` added. The
  stub's sample map now reports `slots`, without which `#clear-samples-btn` and
  `#batch-loop-btn` stay disabled and six dialog states are unreachable — the harness died on
  a hidden button rather than reporting them.

### Fixed
- **The settings popover was painted over, so the language selector could not be clicked.**
  Present since the settings popover was added in Stage K. `#tab-samplemap` painted on top of `#settings-popover`, whose own
  `z-index: 21` is scoped inside `#header`'s stacking context — `#header`, `#tab-bodies` and `#control-strip` were all `z-index: 1`, a tie broken by document order, and `#tab-bodies` comes later.
  Measured with `elementFromPoint` at `#lang-select`'s centre, which returned
  `#tab-samplemap` at every probe point: the language selector, the only control the
  whole i18n feature adds, was unreachable.

  **No gate saw it.** `check-ui-labels` compares rectangles, and a rect is unchanged by
  paint order. It surfaced only from a repo-wide hit-test written after O-Bassoon's
  executor hit the same shape on its own page.

  Fix is paint order only — `#header` gets `z-index: 2` — and the two layers do not overlap in layout.
  Negative control: reverting it alone returns the probe to FULLY BLOCKED.

## [1.23.10] - 2026-08-08

### Fixed

- Windows build compatibility: hoisted `SafePointer` init-captures out of
  nested lambdas (MSVC C2059 workaround)

### Changed

- Added AGPL-3.0 license notice headers to all Ouaricon-authored sources

## [1.23.9] - 2026-07-21

Patch release fixing a **regression introduced by v1.23.8**: offline export
hairpins stuttered (rapid sawtooth of the dynamic level) instead of the old
sudden jumps. Real-time playback was unaffected. **No parameter IDs, ranges,
or state format change.**

### Fixed

- **Offline export: dynamics sawtoothed between fresh CC 11 and the stale
  `expression` param.** v1.23.8's reconciliation branch adopted the
  `expression` APVTS value on CC-quiet blocks whenever it differed from the
  last forwarded CC (epsilon-guarded) — intended to catch genuine UI-knob /
  automation moves. But the guard only recognises the forward's echo AFTER
  it lands; during offline export the message-thread forward lags the render
  thread by whole hairpins, so on every block between Dorico's discrete CC
  steps the param still held a value from much earlier in the hairpin and
  the branch snapped `liveExpression` back to it. Fresh CC → forward jump,
  quiet block → stale snap-back: a per-block sawtooth ≈ the reported
  "constant stuttering". In real time the param tracks within the epsilon,
  so the branch never fired and playback stayed clean — which is why the
  regression was export-only.

  **Fix:** the adoption branch is now additionally gated on
  `pendingCC11Value == -1` (no CC forward in flight): while a forward is
  pending, the param is known-stale and must not be trusted. To make that
  gate sound, `handleAsyncUpdate` now clears the pending slot only AFTER
  `setValueNotifyingHost` has written the param (load + CAS-clear instead of
  exchange-then-write) — clearing first left a window where the gate was
  open but the param was still stale. The CAS also preserves a newer CC
  value staged mid-dispatch (the audio thread's `triggerAsyncUpdate`
  re-queues the updater, which forwards it next pass). Genuine knob /
  automation moves of `expression` are still adopted on any CC-quiet,
  nothing-pending block, and CC still wins while a stream is active —
  unchanged policy. RT-safe: relaxed atomic load added to processBlock;
  CAS runs on the message thread only.

## [1.23.8] - 2026-07-21

Patch release fixing **sudden dynamic-level jumps in Dorico offline audio
export** (real-time playback was always correct). **No parameter IDs, ranges,
or state format change.**

### Fixed

- **Offline export: CC 11 dynamics jumped instead of ramping.** The audio
  path's only dynamics input was the `"expression"` APVTS atom, which is
  updated exclusively by `handleAsyncUpdate` on the **message thread** (the
  v1.12.1 CR-01 RT-safety staging: `processBlock` → `pendingCC11Value` →
  `triggerAsyncUpdate` → `setValueNotifyingHost`). In real time that
  round-trip is a few ms — far under the voice's 20 ms smoother — so playback
  sounded right. During offline export the render thread outruns the
  wall-clock-paced, **coalescing** `AsyncUpdater`: a whole hairpin of CC 11
  steps collapsed into one late value, and the voice stepped its equal-power
  layer crossfade plus the dB-linear `dynamic_range` gain (default 20 dB,
  max 40 dB) across a single 20 ms ramp → audible jumps only in the exported
  audio. Note starts were worse: `startNote` seeded the smoother from the
  same stale param **unsmoothed** (`setCurrentAndTargetValue`), so notes
  began at the wrong dynamic and snapped mid-note when the async update
  finally landed.

  **Fix:** `processBlock` now stores each block's last CC 11 value into a new
  audio-thread atom `liveExpression` **before** staging the async forward;
  the voices (`startNote` seed + per-block smoother target, via
  `currentExpression()`) and the Velocity-mode post-mix `expressionSmoother`
  read that atom instead of the APVTS atom. `handleAsyncUpdate` is unchanged
  and remains solely the UI-knob / host-automation / DAW-visibility mirror.
  A reconciliation branch (0.5/127 epsilon) still adopts genuine UI-knob or
  host-automation moves of `expression` on CC-quiet blocks while ignoring
  the quantised echo of our own `setValueNotifyingHost` forward;
  `prepareToPlay` seeds `liveExpression` from the restored param so session
  restores land before the first CC arrives. RT-safe: relaxed atomic
  load/store only, no allocation, no locks. Smoothing constants unchanged;
  real-time playback is audibly identical (the value now arrives ≤1 block
  earlier — sub-ms, far under the 20 ms ramp). Fixes both dynamics modes
  (CC Crossfade default path and Velocity mode's milder variant of the same
  defect).

  Root cause found via dorico-agent investigation; Dorico's side (exp map
  `volumeType kCC param1=11`, discrete CC steps for hairpins) is correct and
  untouched.

## [1.23.7] - 2026-07-03

Patch release closing out the **WebView frontend** findings — the five warnings
and four of the infos from the 2026-06-30 ui-frontend review
(`.planning/review/2026-06-30/REVIEW-ui-frontend.md`), the last item of the
v1.23.x review-followups batch. **No parameter IDs, ranges, or state format
change** — changes are confined to `Resources/ui/js/sampler-app.js` plus one
new read-only native fn (`getParameterDefaults`) in `PluginEditor.cpp`.

### Fixed

- **WR-01 — ADSR knob readouts displayed the wrong value (up to 2× low).**
  `KNOB_FORMATS` mapped the normalised knob position linearly against a
  hard-coded 0.001–5.0 s range, but the real attack/decay/release
  `NormalisableRange` is 0–10 s with skew 0.5 — at full knob the label read
  "5.00 s" while the host received 10.0 s. (Polyphony had the same class of
  drift: JS claimed 1–32 vs the real 1–16 — missed by the review, caught during
  investigation.) **Fix:** readouts are now computed from
  `SliderState.getScaledValue()` — JUCE pushes the real range (start/end/skew)
  via `propertiesChanged`, so JS no longer hard-codes ANY parameter range and
  this class of drift is structurally gone. A `propertiesChangedEvent` listener
  refreshes each knob when the real range lands at boot. The audio was always
  correct; only the label lied.
- **WR-02 — latent boot-time TypeError in two event subscribers.**
  `subscribeTechniqueStateUpdates` / `subscribeTriggerStateUpdates` guarded
  only `window.__JUCE__` before dereferencing `.backend`, unlike every other
  subscriber in the file. A boot-ordering gap would throw mid-`DOMContentLoaded`
  and abort the rest of the init sequence. Both now use the full guard.
- **WR-03 — CC/PC trigger tables clobbered in-progress edits.**
  `renderTriggerPanel` tore down and rebuilt all rows (`innerHTML = ''`) on
  every technique/trigger state echo, discarding whatever the user was typing
  and dropping focus. Now mirrors the v1.23.0 trim-panel reconciliation: rows
  are built once (`ensureTriggerRows`) and values are written in place through
  `setInputValueUnlessFocused`, which skips the input the user is focused in.
- **WR-04 — knob double-click "reset" jumped to mid-range, not the default.**
  It hard-coded normalised 0.5, which for the skewed ADSR ranges denormalises
  to ~2.5 s (attack default is 0.005 s). New `getParameterDefaults` native fn
  returns each control-strip param's `getDefaultValue()` (normalised); JS pulls
  it once at boot and dblclick now snaps to the real APVTS default (0.5 only
  as a fallback if the pull failed).
- **WR-05 — wheel edits weren't recorded as host automation.** The wheel
  handler wrote values without a `sliderDragStarted`/`sliderDragEnded` gesture
  (drag and dblclick both had one), so hosts in automation-write mode could
  coalesce or drop wheel tweaks. A gesture now opens on the first tick and
  closes after 250 ms idle.
- **IN-01 — KS low/high fields also overwritten mid-edit** — same
  activeElement guard as WR-03 (one-line, same pattern; folded in).
- **IN-04 — modal Esc bubbled into the loop editor.** `bindModal`'s
  capture-phase key handler now calls `stopPropagation()` once it claims a
  key, so dismissing a modal no longer also closes the loop editor behind it.
- **IN-05 — dead `window.confirm` fallbacks removed.** WKWebView does not wire
  `window.confirm` through the UIDelegate (returns `undefined` silently), so
  the missing-modal-DOM fallbacks in `showEmbedSizeConfirmModal` and
  `showAmbiguousDuplicatesDialog` could never actually ask — the RR path would
  send a "cancel" the user never saw. Both now fail safe explicitly:
  `console.error` + toast + cancel.

### Changed

- **IN-02 — stale octave-label comments corrected.** Under the intentional
  C3=60 convention the grid labels read `C0…C7` (low key `A-1`), not the
  `C1…C8` the comments claimed. Comments now match the rendering (which was
  always correct-by-design, matching FilenameParser).
- **IN-03 — loop markers no longer drawn for one-shot cells.** The `drawMarker`
  call site's comment claimed one-shot suppression the code didn't do; the code
  now honors it (markers previously rendered stacked at x=0). Cosmetic.

### Deferred (out of scope per the followups brief)

- IN-06 (`openLoopEditor` param `vel` → `layer` rename) and IN-07
  (`renderControlStrip` tooltip interpolation → `createElement`) — naming/
  defensive-pattern infos, no functional impact.

### Testing

- New `Source/tests/ui_frontend_check.js` (node script, 19/19): syntax gate
  (`node --check` — a load-time SyntaxError silently kills the whole UI),
  JS↔C++ native-fn bridge closure (all 42 names registered, incl. the new
  `getParameterDefaults`), and static pins for WR-01..05 / IN-01 / IN-05.
- All 15 existing C++ regression checks re-run green (352 assertions total);
  auval PASS.

## [1.23.6] - 2026-07-01

Patch release hardening the **sample loader + filename parser** — the four
warnings and the loader/parser infos from the 2026-06-30 loading-parsing review
(`.planning/review/2026-06-30/REVIEW-loading-parsing.md`). **No parameter IDs,
ranges, or state format change** — existing sessions and `.omspreset` files load
byte-for-byte unchanged. Changes are confined to `SampleLoader.cpp`,
`FilenameParser.cpp`, and a new pure-helper header `LoaderSupport.h`.

The review's **CR-01** (worker force-kill via `pthread_cancel`) was already fixed
in v1.23.1 (block reads + `shouldExit()` checkpoints + 5000 ms signal-first
`stopThread`) and is **not** re-addressed here. **IN-04** (stale
`lowestNote`/`highestNote` after a single-variant replace) lives in
`PluginProcessor.cpp`, outside these two files, and is deferred.

### Fixed

- **WR-01 — a corrupt/huge file silently aborted the ENTIRE folder load with no
  callback.** `processOneFile` allocated `AudioBuffer(srcChannels, srcSamples)`
  and `make_shared<AudioBuffer>(2, outNumSamples)` straight from the file header
  with no upper bound and no per-file `try`/`catch`. A bad length → `std::bad_alloc`,
  which JUCE's Release `threadEntryPoint` `catch(...)` (Threads.cpp:108-114)
  swallows silently: the worker exits mid-batch, neither `completionCallback` nor
  `failureCallback` fires (UI spinner hangs), and every already-decoded sample is
  discarded. **Fix:** wrap the per-file body in `try`/`catch` so a throw becomes a
  recorded skip (`outSkipReason`) + continue in folder mode / a reported failure in
  single-variant mode, and reject `srcSamples` above `kMaxSamplesPerFile`
  (~268M samples/ch) **before** allocating.

- **WR-02 — `int64 → int` truncation of `lengthInSamples` mis-sized very long /
  corrupt files.** `const int srcSamples = (int) reader->lengthInSamples;` wrapped a
  >2^31-sample length to a small positive (passing the `<= 0` guard → silent
  under-read) or negative (false "invalid header" reject of a real file). **Fix:**
  keep the count as `juce::int64` and validate it against the same ceiling
  (`oms::isAcceptableSampleLength`) **before** narrowing.

- **WR-03 / IN-05 — ~1-sample heap over-read on SR-mismatched loads.**
  `outNumSamples = ceil(srcSamples / srcRatio)` made `LagrangeInterpolator::process`
  consume up to ~1 input sample past `sourceBuf`'s valid region on any odd-length
  down/up-convert (e.g. 44.1↔48 kHz) — a real out-of-bounds heap read on the
  majority of real-world loads. **Fix:** pad the source buffer with one **cleared
  guard sample** (`srcSamples + 1`, `clear()`d) so the look-ahead read is in-bounds
  and silent; output length is unchanged (still `ceil`). The now-provably-dead
  `if (outNumSamples < 1) outNumSamples = 1;` clamp (IN-05) is removed — `srcSamples >= 1`
  and `srcRatio > 0` guarantee `ceil >= 1`. *(Chose guard-padding over the review's
  `floor` alternative so no output tail is dropped and tiny files never produce a
  zero-length buffer.)*

- **WR-04 — RR split-form fabricated a round-robin index from the note token.**
  The RR scan ran over ALL tokens without excluding the one already claimed as the
  note, so `take_60.wav` → `["take","60"]` used `"60"` as BOTH the MIDI note AND
  `parseAsRrIndex("take60")` → rr 59, planting a spurious explicit RR that
  suppressed the ambiguous-duplicate modal and skewed variant ordering. **Fix:**
  skip `i == noteTokenIndex`, and in the split form require the digit token index
  `!= noteTokenIndex`. A genuinely distinct note + take (`C3_take_60`) keeps its RR.

### Changed

- **IN-01 — nested subfolders are now reported.** Folder enumeration stays flat
  (recursing would silently ingest unrelated audio in sibling/bounce/render
  subfolders), but a `"N subfolder(s) ignored (flat load …)"` line is now added to
  the skipped-files payload so a user who dropped a per-articulation folder tree
  sees why the map is partial instead of getting silence.

- **IN-02 — more duplicate groups now ask for confirmation.** Previously a group
  was flagged ambiguous ONLY when NO file carried an explicit RR token, so a mix
  like `{C3_v1_rr1.wav, C3_v1.wav}` (a likely accidental duplicate next to an RR
  set) and two files resolving to the SAME RR index merged silently. The new
  `oms::isAmbiguousRrGroup` also flags mixed explicit/no-token groups and duplicate
  explicit indices. Distinct explicit sets (`rr1, rr2, rr3…`) still merge silently.
  The built map is unchanged either way — this only governs the confirmation modal.

- **IN-03 — bare single-letter dynamics no longer mis-map pre-note.** With no
  post-note velocity token, the pre-note tier accepted a bare `p`/`f` anywhere
  before the note, so `F-C3.wav` (Flute) read as forte→layer 3 and `P_C3.wav`
  (Piano) as piano→layer 0 — silent mis-maps. **Fix:** the pre-note tier now
  requires a two-char dynamic (`mp`/`mf`) or an explicit `v`/`vel`/`L`/`layer`/`lyr`
  form; bare `p`/`f` remain valid POST-note. *(Tradeoff: a library that uses a bare
  `f`/`p` PRE-note as its intended dynamic now lands on the default layer — the
  dominant `_mf_`/`_mp_` convention and all post-note forms are unaffected.)*

### Testing

- New `loader_robustness_check` regression (46/46) pins the extracted pure helpers
  (`isAcceptableSampleLength`, `resampleOutLength`, `isAmbiguousRrGroup`) and the
  parser fixes (WR-04 split-form exclusion, IN-03 pre-note dynamics gate).
- `technique_parse_check`, `merge_rr_check`, `find_cell_triplet_check`,
  `dynamics_layer_check`, `state_migration_check`, `cc_pc_trigger_check` all re-run
  green (no parse/grouping/state regressions).
- auval (`aumu OMtS OuDv`) **PASS**. The WR-01 `try`/`catch` and CR-01 cooperative
  cancellation are integration-level (real/corrupt reader) — covered by build +
  auval, not the console harness.

## [1.23.5] - 2026-07-01

Patch release hardening the **WebView C++/JS bridge** — the two warnings and
three infos from the 2026-06-30 editor-bridge review
(`.planning/review/2026-06-30/REVIEW-editor-bridge.md`). **No parameter IDs,
ranges, or state format change** — existing sessions and `.omspreset` files load
byte-for-byte unchanged. The changes affect only editor lifetime-safety, save
error reporting, drop routing, and dead-code removal in `PluginEditor.cpp`.

### Fixed

- **W12 — async file-dialog completions could use-after-free after editor
  teardown.** Each of the 9 `this`-capturing `chooser->launchAsync(...)`
  completions (`loadSingleSampleDialog`, `loadScalaFile`, `loadKBMFile`,
  `saveScalaFile`, `saveKBMFile`, `saveCurrentPreset`, `loadPreset`,
  `locateMissingFolder`, `exportTuningHTML`) kept the `FileChooser` alive via a
  shared_ptr but captured a raw `this`. If the host tore the editor window down
  while a native dialog was open, the completion later dereferenced a dangling
  `this` (`processorRef.…`) **and** called the WebView's `complete` callback
  after the `WebBrowserComponent` was destroyed — both use-after-free.
  **Fix:** capture a `juce::Component::SafePointer<…Editor>` and bail on null.
  **Note (deviation from the review snippet):** the bail does **not** call
  `complete(false)`. Verified in JUCE 8.0.9 that the native-fn completion is
  `[this = WebBrowserComponent::Impl*, resultId](result){ … }` owned by the
  editor's WebView — so calling `complete` on the dead-editor path would itself
  UAF the freed `Impl`. The editor (and its JS) are gone during teardown, so the
  unresolved promise is moot; we bail without touching the dead bridge.
  (`pickSampleFolder` / `estimateFolderAudioSize` were already safe — `[]`
  captures, no `this`.)

- **W13 — silent save failures reported to JS as success.** `saveScalaFile`,
  `saveKBMFile`, and `exportTuningHTML` called `file.replaceWithText(…)` and
  unconditionally resolved `true`, discarding the write result. A failed write
  (read-only location, permission denied, disk full) showed a success state in
  the UI while nothing was written — silent data loss. **Fix:** propagate the
  real `bool` (`complete(var(ok))`), matching `saveCurrentPreset` which already
  did this.

- **IN-02 — `reportCellLayout` never reset `folderZoneRect` on an omitted
  payload.** The folder drop-zone rectangle was only overwritten when the JSON
  contained a `folderZone` object; a later report that omitted it (zone
  scrolled out / removed from the DOM) or failed to parse left a stale rectangle
  that `filesDropped` could still route a folder drop to. **Fix:** reset
  `cellLayout` + `folderZoneRect` up-front each report, before conditionally
  repopulating — so a stale rect can no longer mis-route.

- **IN-03 — cell-hit drop path loaded without an existence check.**
  `handleWebViewFileDrop` forwards JS-supplied path strings straight into
  `filesDropped`; a filename-only / relative string (a host that doesn't expose
  absolute paths) yielded a non-existent `juce::File` and a phantom
  `loadSingleSample` dispatch (also a debug jassert). **Fix:** gate the cell-hit
  load on `file.existsAsFile()` and toast otherwise — matching the dialog path.

### Changed

- **IN-01 — removed 3 dead native functions from the bridge registry.** A
  full-tree scan of `Resources/ui/js` (every `getNativeFunction` / `invokeNative`
  call site) confirmed `getEmbeddedTuningCategories`, `getSkippedFiles`, and
  `resetTechniqueNames` have no JS caller (`getSkippedFiles` is superseded by JS
  reading `snap.skippedFiles` directly; `resetTechniqueNames` by
  `applyTechniqueNames`/per-slot renames). Removed the registry entries; the
  processor's `getLastSkippedFiles()` accessor and `resetTechniqueNames()` method
  are retained (still used internally / as API surface).

- **Extracted drop hit-test geometry to `DropRouting.h` (`oms::hitTestDrop`).**
  The XY→zone routing (cell vs folder-zone priority, empty-zone guard) that lived
  inline in `filesDropped` is now a pure, unit-testable free function.
  Behaviour-preserving — cell wins over the zone, half-open intervals unchanged.

### Testing

- New `drop_routing_check` regression (**13/13 PASS**) pins `oms::hitTestDrop`:
  cell hit / cell-priority-over-zone / folder-zone hit / empty-zone stale-rect
  guard (IN-02) / out-of-bounds / half-open edges / first-match-wins.
- Build (VST3 + AU, Release) clean; **auval PASS**; **pluginval strictness-10
  PASS** including the Editor / "Open editor whilst processing" / Editor
  Automation open-close cycles that exercise the W12 lifetime path.

## [1.23.4] - 2026-07-01

Patch release hardening the **state-serialization path** — three findings from
the 2026-06-30 processor/state review
(`.planning/review/2026-06-30/REVIEW-processor-state.md`). **No parameter IDs,
ranges, or state format change** — existing sessions and `.omspreset` files load
byte-for-byte unchanged; same-version project reopens are identical. The changes
affect only (a) thread-safety of off-message-thread saves, (b) determinism when
loading *partial/older* presets, and (c) save latency for embedded libraries.

### Fixed

- **WR-01 — off-message-thread `getStateInformation` could tear-read technique
  names / tuning → crash.** Reaper (and other hosts) call `getStateInformation`
  off the message thread — the documented HG-08 reason the `persistenceLock`
  exists — but `captureStateValueTree` read two *other* pieces of mutable state
  without it: `techniqueNames` (a `juce::StringArray`, which reallocates its
  backing store on `set`/`add`) and the live `TuningEngine` (via
  `captureTuningValueTree`). A concurrent rename or tuning edit on the message
  thread during an off-thread save was a torn read / use-after-free. **Fix:**
  snapshot `techniqueNames` under `persistenceLock` in the capture, guard
  `setTechniqueName` / `resetTechniqueNames` (and the restore-path mutation)
  with the same lock, hold the lock around the tuning capture, and take the lock
  in the editor's 8 tuning-*write* native functions (`setSingleInterval`,
  `setTonicNote`, `setOctaveStretch`, `setMasterTune`, `loadEmbeddedTuning`,
  `loadScalaFile`, `loadKBMFile`, `applyGeneratedScale`). Read-only tuning
  queries stay lock-free — two readers can't tear. New public
  `getPersistenceLock()` accessor; the lock is recursive and never taken on the
  audio thread.

- **WR-02 — loading a partial/older preset bled the previous session's technique
  names and CC/PC trigger tables through.** `restoreStateValueTree` reset the
  *trims* table to unity before applying saved entries, but only *overrode
  present slots* for technique names and only rebuilt the CC/PC tables when the
  child was present. Loading an `.omspreset` that lacked `<TechniqueNames>` /
  `<CcMapping>` / `<PcMapping>` over a customized session left the prior
  session's renamed techniques and custom trigger tables active — a silently
  mixed state the user never authored. **Root cause:** inconsistent
  "state the preset doesn't carry" handling across the restore sections. **Fix:**
  mirror the trims pattern — reset technique names to
  `defaultTechniqueVocabulary()`, CC to `defaultCcMapping(count)`, and PC to
  `defaultPcMapping()` **before** applying whatever the loaded tree carries.
  Restore is now deterministic: the final state depends only on the loaded tree,
  never on prior state. Back-compat preserved — v1.13.0/v1.14.0 sessions that
  omit these children reset to the exact ctor defaults they had before.

- **WR-04 — a 250 MB embedded library was re-encoded to base64 WAV on *every*
  save.** `buildEmbeddedAudioTree` called `encodeVariantAsBase64Wav` for every
  variant on each `getStateInformation`, re-encoding the whole embedded library
  (a latency hazard on hosts that save from an audio-adjacent thread). **Fix:**
  memoise the encoded blob on `SampleVariant` (`cachedBase64Wav`) — encode once,
  reuse on every subsequent save. Embedded snapshots are immutable after load and
  loop points serialise as separate XML attrs, so the cache never goes stale.
  DAW autosave no longer re-encodes.

### Testing

- New `preset_determinism_check` regression (15/15) pins the WR-02 no-bleed
  contract using the real `defaultTechniqueVocabulary` / `defaultCcMapping` /
  `defaultPcMapping` factories.
- Re-ran `state_migration_check`, `cc_pc_trigger_check`, `ks_default_check` —
  all green. auval PASS; pluginval clean.

## [1.23.3] - 2026-06-30

Patch release fixing the fresh-instance keyswitch default that silently ate low
notes, plus two related technique-axis clean-ups, from the 2026-06-30
processor/state review (`.planning/review/2026-06-30/REVIEW-processor-state.md`).
**No parameter IDs, ranges, or state format change** — existing sessions and
presets keep their stored keyswitch settings and load unchanged. Only the
*fresh-instance defaults* move, so this is a behaviour change for **new**
instances only.

### Fixed

- **WR-03 — a fresh instance silently absorbed every note-on in MIDI 0–9.**
  The shipped defaults were `ks_enabled=true` with the keyswitch range set to
  MIDI 0–9, so `processBlock` absorbed every note-on in that range as a
  keyswitch and never forwarded it to the synth. **Root cause:** any library
  mapped into the low register (microtonal or full-range) lost those notes with
  no user-visible cause, and the adjacent block comment claimed the *opposite*
  (`technique_count=1, ks_enabled=false` "reproduces v1.13.0") — inviting a
  future maintainer to "fix" the defaults straight back into the bug. **Fix:**
  keyswitches are now **opt-in** — `ks_enabled` defaults to `false`. A new
  instance forwards all notes; with no KS/CC/PC trigger active the technique
  cursor stays at slot 0, reproducing v1.13.0 playback exactly. The misleading
  comment is rewritten to describe the real defaults. Users who want
  keyswitching flip one toggle in the UI.

- **IN-04 — the default keyswitch range advertised one more slot than exists.**
  With `ks_high_note=9` and eight techniques, notes 8 and 9 both clamped onto
  the last technique (slot 7). **Fix:** the default range is now MIDI 0–7 —
  exactly `kMaxTech` slots wide (`ks_low + kMaxTech - 1`) — so if a user enables
  keyswitching, one semitone maps to one technique with no collapse; notes 8/9
  are forwarded to the synth instead of saturating.

- **IN-03 — the default technique vocabulary disagreed across three sites.**
  `resetTechniqueNames()` seeded the Dorico-aligned
  `{ord, sp, st, stacc, cs, pizz, harm, trem}` (the canonical set since v1.16.3,
  matching the shipped Strings expression map), but two `PluginProcessor.h`
  docstrings and the `state_migration_check` fixture still advertised the stale
  `sv`/`mart` names for slots 3 and 7. **Fix:** all three now reference the one
  canonical vocabulary.

### Changed

- **New `Source/TechniqueDefaults.h` — single source of truth for the
  technique-axis defaults.** The fresh-instance keyswitch defaults
  (`ks_enabled`, range, `technique_count`), the canonical vocabulary, and the
  keyswitch→technique mapping now live in one header consumed by
  `createParameterLayout`, `resetTechniqueNames`, and `processBlock`. This
  removes the drift class that produced IN-03 in the first place. `processBlock`
  now computes the keyswitch candidate via the shared
  `OMtsTechnique::keyswitchTechnique` helper (behaviour-identical to the prior
  inline math), so the absorption contract is unit-testable.

### Testing

- **New `Source/tests/ks_default_check.cpp` (27 assertions, all pass).** Guards
  the shipped opt-in defaults (fails loudly if a future edit re-enables the
  note-eating default), the keyswitch→technique mapping across the default range
  (notes 8/9 forwarded, no collapse), and the canonical vocabulary.
- `state_migration_check` (10/10) and `cc_pc_trigger_check` (51/51) re-run
  green; the migration fixture now pulls its default vocabulary from the shared
  header instead of a hand-copied literal.

## [1.23.2] - 2026-06-30

Patch release hardening the audio-thread voice render path against the three
remaining **WARNING**-level findings (plus three **INFO** clean-ups) from the
2026-06-30 DSP/voice review (`.planning/review/2026-06-30/REVIEW-dsp-voice.md`).
No parameters, state format, or normal-path audio behaviour change — sessions
and presets remain **fully compatible with v1.23.1**. The one audible change is
the removal of a per-cycle click at loop boundaries (W9), which only ever made
the sustained loop *worse*.

### Fixed

- **W11 (WR-03) — `+Inf` play-rate lockup on a zero host sample rate.**
  `computePlayRateForVariant` divided by `hostSR = getSampleRate()` with no
  guard. **Root cause:** if the voice is ever asked for a rate before a playback
  sample rate is set (`hostSR == 0.0`), `slotSR / hostSR` is `+Inf`; `pos += Inf`
  becomes `Inf`, and `wrapLoopPosition`'s `while (pos >= lpEnd) pos -= lpLen`
  never terminates → a hard audio-thread hang (and `cubicInterp`'s
  `(int) floor(Inf)` is UB). **Fix:** clamp the divisor to 44100 when `hostSR <= 0`,
  and make `wrapLoopPosition` finite-safe — a non-finite `pos` snaps back to the
  loop start instead of spinning. JUCE normally sets the rate before `startNote`,
  so this is defence-in-depth against a hard lockup.

- **W10 (WR-02) — a reload-boundary voice-steal could `free()` a `SampleMap`
  (and its audio buffers) on the audio thread.** `startNote` snapshots the prior
  map into a local `prevMap` (keeps the steal-tail source alive, C1/CR-04) then
  re-points `currentMap` at the freshly loaded map. **Root cause:** right after a
  background ReplaceAll the processor has already swapped its slot, so only
  in-flight voices hold the old map; when such a voice re-snapshots, `prevMap`'s
  destructor — running on the **audio thread** — could drop the last reference and
  free the entire old `SampleMap` (cell vector + every `SampleVariant` + every
  `shared_ptr<AudioBuffer>`, potentially hundreds of MB) inside the render
  callback. **Fix:** a new `RetiredMapReaper` (single-producer/single-consumer
  ring drained by an 8 Hz message-thread `juce::Timer`). The voice hands a
  retired `prevMap` to the reaper only when it differs from the new snapshot (a
  genuine reload boundary), so the big `free()` runs on the message thread;
  steady-state playback generates zero traffic. Same reload boundary as the
  shipped C1 use-after-free, different failure mode.

- **W9 (WR-01) — per-cycle click at every loop wrap on sustained samples.**
  The 8-sample equal-power loop crossfade indexed an 8-entry LUT built at
  `x = i/8` for `i ∈ 0..7`, so its largest weight was `equalPowerWeights(7/8)` →
  outgoing ≈ 0.195, incoming ≈ 0.981. **Root cause:** the incoming weight never
  reached 1.0, so at the wrap the still-~0.195×tail term vanished instantly — an
  audible `≈ 0.195 × tail` step every loop cycle (the sampler's primary use
  case), and the fade was quantized to 8 steps. **Fix:** drive the crossfade with
  a continuous phase `x = (pos - fadeStart) / 8` that reaches 1.0 at the wrap, so
  hand-off to the post-wrap signal is continuous and un-quantized. The `cos`/`sin`
  fire only for the final 8 samples of each loop cycle → negligible RT cost.

### Changed (INFO clean-ups)

- **IN-02 — round-robin counter no longer advances on a skipped degenerate
  layer.** In the CC-crossfade gather loop, `selectVariantIndex` advanced a
  cell's persistent RR counter *before* the empty-buffer skip, so a failed/empty
  variant still consumed an RR step and could skew the per-cell progression.
  The gather now snapshots the counter and restores it when a degenerate variant
  is skipped (cosmetic — RR ordering only; not audible correctness).
- **IN-01 / IN-03 — comments corrected.** Documented the deliberate equal-*power*
  (not equal-gain) crossfade choice and its known ~+3 dB bump for highly
  correlated content (accepted: layers/CC-morph neighbours are essentially never
  phase-coherent, and equal-gain would instead dip the common uncorrelated case).
  Fixed the stale "squared CC gain" single-layer comment to describe the shipped
  v1.22 dB-linear `dynGain = decibelsToGain(rangeDb·(d−1))` ramp.

### Refactored

- Extracted the pure varispeed-read leaf helpers (`referenceFrequencyForNote`,
  `equalPowerWeights`, `cubicInterp`, `readVariantWithLoop`, `wrapLoopPosition`,
  `computePlayRateForVariant`) from `MicrotonalSamplerVoice.cpp`'s anonymous
  namespace into `Source/VoiceDsp.h` (`inline`, byte-for-byte identical codegen)
  so the new regression test drives the **real** shipped implementations rather
  than a mirror copy.

### Testing

- New `loop_crossfade_check` standalone test (`ninja
  O-MicrotonalSampler_LoopCrossfadeCheck`, exit code = failed assertions, 21/21
  pass): exercises the real `VoiceDsp.h` helpers — the crossfade reaches full
  incoming weight at the wrap and the wrap click is `< 0.02` (was ≈ 0.214
  pre-fix); `wrapLoopPosition` snaps `+Inf`/`-Inf`/NaN to the loop start and
  still wraps finite positions correctly; `computePlayRateForVariant` stays
  finite with `hostSR == 0` and matches the explicit-44100 result.
- Regression sweep of the existing DSP/RR/gather tests all pass (AliasingCheck,
  DynamicsLayerCheck, MergeRrCheck, FindCellTripletCheck, TrimGainCheck,
  EmbeddedTechniqueCheck) — the extraction and IN-02 change are behaviour-neutral.
- VST3 + AU build clean; `auval` validates the AU.

## [1.23.1] - 2026-06-30

Patch release fixing the **3 CRITICAL findings** from the 2026-06-30 full-instrument
code review (`.planning/review/2026-06-30/SUMMARY.md`, C1/C2/C3). No parameters,
state format, or audio behaviour change on the normal path — this hardens three
edge cases (a voice use-after-free, silent embedded-state data loss, and a loader
force-kill), so sessions/presets remain **fully compatible with v1.23.0**.

### Fixed

- **C1 — Audio-thread use-after-free / stuck phantom voice after a failed note-start**
  (`MicrotonalSamplerVoice.cpp`, `startNote`). The two `startNote` failure early-returns
  (no cells / no cell for this note) cleared the velocity-path pointers but left
  `ccDynamicsActive` / `dynLayerCount` set. Because `renderNextBlock` dispatches on
  `ccDynamicsActive` *before* the `variantLow`/`adsr` guard, a failed note-start right
  after a CC-crossfade note (e.g. a ReplaceAll whose new map has no cell for the note)
  left the voice rendering `dynLayers[]` pointers into a just-released `SampleMap` —
  a use-after-free when this voice held the last reference, or a stuck phantom voice
  when the old buffers survived. **Root cause:** the CC-mode failure path did not mirror
  `stopNote`'s hard-off clear. **Fix:** both failure blocks now set
  `ccDynamicsActive = false; dynLayerCount = 0;`, so `renderCcCrossfade`'s
  `dynLayerCount <= 0` guard fires and the voice falls through to the shared clear.

- **C2 — Embedded-audio state round-trip silently dropped `SampleCell.technique`**
  (`PluginProcessor.cpp`, `buildEmbeddedAudioTree` / `decodeEmbeddedAudioTree`).
  The embedded-library serializer persisted only `midi` + `layer`. Since `technique`
  is part of the cell key `(midi, layer, technique)`, any embedded folder on a non-zero
  technique slot (a `pizz` library on slot 5, or any folder loaded with
  `overrideTechnique`) reloaded with **every cell collapsed onto technique 0 ("ord")** —
  silent sample-map corruption and possible collisions with real `ord` cells on project
  reopen. **Fix:** write a `tech` attribute and restore it (jlimited to
  `0..kMaxTechniques-1`). Older embedded saves lacking `tech` default to 0, so the
  round-trip is back-compatible.

- **C3 — Loader worker force-killed (`pthread_cancel`) mid-decode → leaked reader + UI hang**
  (`SampleLoader.cpp`). Every load entry point restarted the worker with `stopThread(500)`;
  `processOneFile` had no cancellation checkpoint inside its read, so on the large libraries
  this plugin targets (3–5 s for 250 MB) a second load or project-close mid-decode escalated
  to `pthread_cancel` — leaking the `AudioFormatReader`, half-mutating `skippedFiles`/`loaded`,
  and **never firing the completion/failure callback**, so the load spinner hung forever.
  **Fix:** `processOneFile` now reads in 64 K-sample blocks with a `threadShouldExit()`
  checkpoint between blocks (and one before the resample); block reads are position-addressed
  so the decoded buffer is **byte-identical** to the previous single-read path. The load
  entry points and the destructor now `signalThreadShouldExit()` then join with a generous
  non-killing timeout (`stopThread(5000)`, `jassertfalse` telemetry on the should-never-happen
  force-kill fallback). Single-variant loads cancelled by a superseding load return silently
  instead of firing a spurious "cancelled" failure callback.

### Testing

- New `embedded_technique_check` standalone test (`ninja
  O-MicrotonalSampler_EmbeddedTechniqueCheck`, exit code = failed assertions):
  builds a `SampleMap` with the same `(midi, layer)` populated on two different
  technique slots ("ord" on 0, "pizz" on 5), round-trips it through the embedded
  cell-key property contract, and drives the **real** `SampleMap::findCell` to
  assert both cells stay distinct and every `0..kMaxTechniques-1` slot survives.
  Case 3 reproduces the pre-fix corruption (drop `tech` → both cells collapse to
  technique 0 and the tech-5 lookup falls back to "ord"), documenting exactly what
  C2 repairs. (The codec functions are file-local in `PluginProcessor.cpp`, so the
  test mirrors their property lines — kept in sync per the `state_migration_check`
  convention.)
- Backup of the v1.23.0 baseline taken at `backups/O-MicrotonalSampler/v1.23.0/`
  (verified) before any edit; one-command rollback available.

### Not addressed (deferred)

- The 18 WARNING and 22 INFO findings from the same review (state-race in
  `captureStateValueTree`, preset-load bleed-through, KS-absorbs-0..9 default, loop-crossfade
  7/8 LUT click, resampler 1-sample over-read, WebView `SafePointer` gaps, etc.) are **not**
  in this patch — they are tracked in `.planning/review/2026-06-30/SUMMARY.md` for a follow-up.

## [1.23.0] - 2026-06-29

Add **per-technique and per-dynamic-layer loudness trims** so a library can be
balanced without re-recording or re-normalising samples — e.g. "make just the
staccato quieter" or "pull down only the mf layer of ord."

### Added

- **Two-level trim model** (range **−12…+12 dB**, default **0 dB**):
  - A **per-technique master** trim that scales every layer of one technique.
  - A **per-(technique, layer)** trim that scales a single dynamic layer of one
    technique.

  The two combine additively in dB (multiplicatively in linear gain), so the
  final cell gain is `dbToGain(techniqueTrim[t] + layerTrim[t][layer])`.
- **"Trims (loudness)" panel** in the WebView (a disclosure below the technique
  tabs). It targets the **active** technique — a master slider plus four layer
  sliders (`p / mp / mf / f`), each with a live dB readout. Switching technique
  tabs retargets the panel; layers with no samples in the active technique are
  dimmed. Double-click a slider to reset it; "Reset all trims" zeroes the table.
  The panel reveals itself once a library is loaded.
- Native-fn bridge: `setTechniqueTrim`, `setLayerTrim`, `resetTrims`, and a
  `trims` block on `getTechniqueState`.
- `TrimGainCheck` standalone test pinning the trim gain math (additive combine,
  unity defaults, out-of-range guard).

### Changed

- The voice folds the trim into its **layer weights** (Velocity mode — covers
  the render path *and* the voice-steal tail) and into each **DynLayer** gain
  (CC Crossfade mode — covers `renderCcCrossfade` *and* `renderTailRampCc`).
  All lookups happen **once at note-on** (RT-safe atomic loads); there is no
  per-sample cost beyond a multiply.

### State / compatibility

- Trims are **library-balancing metadata, not automation** — they live OUTSIDE
  the APVTS in a processor-owned `TrimTable` and round-trip through the state
  ValueTree as a sparse `<CellTrims>` child (only non-zero entries are written),
  exactly like the technique names. **No new host-automation parameters.**
- **No breaking changes.** Sessions/presets older than v1.23.0 have no
  `<CellTrims>` child, so every trim restores to 0 dB → playback is
  **bit-identical** to v1.22.0 until a trim is touched.

## [1.22.0] - 2026-06-25

Add a tunable **Dynamic Range** knob for **CC Crossfade** mode. Fixes the
report that in Dorico "forte dynamics are too soft and piano dynamics are too
loud."

### Root cause

CC Crossfade (the default since v1.21.0, and what Dorico drives via CC 11) was
a **pure timbre morph with a flat volume envelope**. In `renderCcCrossfade` a
multi-layer note used `dynGain = 1.0` — so the only loudness difference between
`pp` and `ff` was the *recorded* amplitude gap between velocity layers. Many
libraries are peak-normalised (little or no inter-layer level difference), so
the dynamic range collapsed: `pp` played its lowest layer at full level (too
loud) and `ff` its highest layer at the same level (too soft). The old squared
post-mix Expression gain that used to add a wide sweep is correctly **bypassed**
in CC Crossfade mode (to fix the v1.20 Dorico `pp` double-attenuation), which is
*why* the range was missing — not a leftover compensation cutting `ff`.

### Added

- **`dynamic_range` parameter** (float, `0–40 dB`, default `20 dB`). The dB
  span between `pp` (CC 11 = 0) and `ff` (CC 11 = 127). Automatable; exposed as
  a **Dyn Rng** knob in the WebView control strip (between Expression and Out
  Gain) and in the host generic editor.
- The voice now layers a **dB-linear loudness ramp** on top of the equal-power
  layer crossfade: `dynGain = decibelsToGain(rangeDb · (d − 1))`, where
  `d ∈ [0,1]` is the live smoothed CC 11 position. CC 11 now shapes **both**
  timbre (layer morph) **and** loudness — like a real sustain patch. Applied in
  both `renderCcCrossfade` and the voice-steal tail `renderTailRampCc` so steals
  match. Read once per block from a cached atom (RT-safe).

### Changed

- **Single-layer CC Crossfade** now uses the same dB-linear ramp instead of the
  old `d²` curve, so the new knob governs its dynamics consistently. At the
  default 20 dB a single-dynamic library still responds musically to CC 11; set
  the knob to **0 dB** for the exact v1.21.0 flat behaviour (timbre morph only,
  loudness only from recorded layers).

### Notes

- **CC Crossfade only.** Velocity mode is untouched — its post-mix squared
  Expression gain path is unchanged.
- No Dorico expression-map change needed; the maps already send dynamics on
  CC 11.
- No new automated test; verified via build + offline render and DAW/Dorico
  check. Rollback baseline: `backups/O-MicrotonalSampler/v1.21.0/`.

## [1.21.0] - 2026-06-22

Add the **CC Crossfade dynamics engine** — the structural fix for the Dorico
`pp` double-attenuation problem deferred in v1.20.2. A new **Dynamics Mode**
chooses how MIDI CC 11 / Expression shapes dynamics.

### Added
- **`dynamics_mode` parameter** (choice: `Velocity` / `CC Crossfade`).
  Automatable; exposed in the WebView control strip as a segmented toggle next
  to the Expression knob, and in the host generic editor.
- **CC Crossfade engine** (`MicrotonalSamplerVoice`). At note-on the voice
  resolves **every populated velocity layer** for the note's resolved technique
  into a layer stack (new `SampleMap::gatherLayerCells`, single-technique — no
  articulation mixing across the loudness axis). All layers advance in lockstep
  each sample (time-synced → click-free bracket entry); only the two layers
  bracketing the live, smoothed CC 11 position `d∈[0,1]` are summed with
  equal-power weights (`p = d·(N−1)`). The result is a true timbre + loudness
  morph on hairpins, like professional sustain patches — not just a volume
  trim. 20 ms per-voice smoothing keeps the morph zipper-free; voice-stealing
  has a matching CC-aware tail ramp.

### Changed
- **Default Dynamics Mode is `CC Crossfade`.** New instances — and existing
  projects/presets that predate this parameter — adopt CC Crossfade on load.
  To restore exact v1.20.x playback, switch Dynamics Mode to **Velocity**.
- **Post-mix Expression gain is bypassed in CC Crossfade mode.** CC 11 is the
  dynamics axis in that mode; the voice already crossfades layers by it, so the
  old squared post-mix gain would double-attenuate quiet passages (the original
  Dorico `pp` problem). Velocity mode is unchanged — the post-mix gain still
  applies and the render path is bit-identical to v1.20.2.

### Notes
- **Why this is the right fix (per v1.20.2 research):** the industry model
  separates *dynamics* (a continuous CC that crossfades recorded layers,
  changing timbre AND loudness) from *expression* (a secondary volume trim).
  The reverted v1.20.1 gain-floor guessed at sample loudness; crossfading real
  layers does not.
- **Single-dynamic libraries:** with only one populated layer there is nothing
  to crossfade, so CC 11 drives a squared gain on that layer instead — CC still
  shapes dynamics rather than going flat.
- **No Dorico expression-map change needed** — the maps already send dynamics
  on CC 11 (`volumeType` `kCC` `param1=11`).
- **Testing:** built Release (VST3 + AU), `auval` pass, installed. Velocity mode
  verified bit-identical to v1.20.2 baseline; CC Crossfade verified on a
  multi-layer library (hairpin morph) and a single-layer library (squared-gain
  fallback).

## [1.20.2] - 2026-06-22

Revert: **roll back the v1.20.1 layer-adaptive Expression floor.** It
over-corrected — `pp`→`ff` dynamic contrast became too small on many libraries.

### Reverted
- **v1.20.1's `30/N` Expression-gain floor is removed.** The CC11 Expression
  curve is restored to the v1.20.0 baseline (`gain = expression²`).

  **Why it was wrong:** v1.20.1 assumed that with `N` velocity layers the
  recorded samples already encode ~`(N-1)/N` of the `pp`→`ff` loudness drop, so
  it raised the CC11 gain floor (to ~−7.5 dB at N=4) to avoid double-attenuation.
  Research into how Kontakt / orchestral libraries / NotePerformer handle
  dynamics showed that assumption is unreliable: baked-in dynamic range varies
  enormously and is usually *small* or normalized (e.g. Cinebrass `pp`→`ff`
  ≈ 0–6 dB; "most libraries have only ~25% of the dynamic range they should").
  When a library's layers carry little loudness difference, raising the CC11
  floor collapses the usable dynamic range → flat playback.

### Notes
- **Back to a known-good baseline.** With the floor removed, behaviour matches
  v1.20.0 exactly. The original "Dorico `pp` is too quiet" issue is real but is
  best solved structurally (see below), not by guessing at sample loudness.
- **Planned proper fix — continuous CC dynamic crossfade.** The industry model
  separates *dynamics* (a continuous CC that crossfades between recorded layers,
  changing timbre AND loudness) from *expression* (CC11 as a secondary volume
  trim). The follow-up adds a `Dynamics` mode where CC11 drives a real-time
  equal-power crossfade across all velocity layers mid-note (defaulting to that
  mode for this plugin's Dorico-centric use), so hairpins morph timbre and
  loudness like pro sustain patches. Tracked separately so it can be A/B'd
  against this restored baseline.

## [1.20.1] - 2026-06-21

Fix: **quiet dynamics in Dorico are no longer attenuated twice.** On a
multi-layer library, a `pp` marking played far too quiet relative to `ff`.

### Fixed
- **Layer-adaptive Expression (CC11) gain depth.** A quiet dynamic in Dorico
  sends two signals at once: a low note-on velocity (which selects the
  inherently-quieter `pp` velocity-layer *sample*) **and** a low CC11 (which
  scales post-mix gain). The pre-1.20.1 curve was a bare `expression²` that
  floored to **zero** at CC11 = 0, so the pp→ff loudness drop was counted
  **twice** (quiet sample × heavy gain cut) → `pp` was ~−37 dB below `ff`.

  **Root cause** (`PluginProcessor.cpp::processBlock`): the expression gain
  target was `v*v` regardless of how many velocity layers the library carried.
  Velocity-layer selection already encodes the recorded loudness drop, so the
  full-range gain cut double-counted it.

  **Fix:** the depth of the CC11 cut now scales with the velocity-layer count.
  With `N` layers the samples carry ~`(N-1)/N` of the dynamic range, so CC11
  supplies only the remaining `1/N`:

  ```
  depthDb(N) = 30 / N                         # 30, 15, 10, 7.5 dB for N = 1..4
  gain       = dbToGain(-depthDb(N)) * ...    # remapped squared curve
             = floor(N) + (1 - floor(N)) * expression²
  ```

  - **N = 1 (no dynamic layers available):** floor ≈ −30 dB → CC11 keeps a full
    expressive range, because it is the *only* dynamics source. Directly handles
    the "sometimes dynamic layers will not be available" case.
  - **N = 4 (standard multi-layer):** floor ≈ −7.5 dB → a `pp` note lands at
    roughly −19 dB below `ff` (sample −12 dB + gain −7.5 dB) instead of −37 dB.

  The layer count is read on the audio thread via one `atomic_load` of the
  published `SampleMap` per block (the same snapshot pattern voices use in
  `startNote`).

### Notes
- **Not a breaking change.** No parameter IDs, ranges, or state format changed.
  At `expression = 1` the gain is **exactly 1.0 for every layer count**, so any
  project left at the default (`expression = 1.0`) is bit-identical to v1.20.0.
  Only the *bottom* of the CC11 range is raised, and only for multi-layer
  libraries.
- **Regression coverage:** `tests/dynamics_layer_check.cpp` gains a section
  pinning the unity-at-full-expression invariant, the monotonic floor-vs-layers
  relationship, and the pp-louder-than-old-curve scenario.
- **Docs:** `docs/dynamics-mapping.md` updated with the layer-adaptive depth
  formula and rationale.

## [1.20.0] - 2026-06-21

Feature: **technique naming presets**. A new "Technique preset" dropdown above
the technique strip renames all eight technique slots to match a Dorico
instrument family in one action — so non-string instruments no longer require
renaming each slot by hand. Slot order follows the keyswitch (baseSwitchID)
order of the matching O-MicrotonalSampler expression map, so sampler slot *N*
lines up with keyswitch *N* in Dorico.

### Added
- **`Technique preset` selector** (always visible, above the technique bar).
  Four families mirror the four expression maps in
  `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`:
  - **Strings** — `ord, sp, st, stacc, cs, pizz, harm, trem` (= existing default)
  - **Winds** — `ord, flz, whis, mult, key, slap, harm, stacc`
    (natural, flutter-tongue, whisper, multiphonic, key click, slap-tongue,
    nat. harmonic, staccato)
  - **Brass** — `ord, mute, cuiv, flz, spare, stop, growl, fall`
    (natural, muted, cuivré, flutter-tongue, spare/open, stopped, growl,
    fall/drop — slot 4 is intentionally a spare, matching the unbound keyswitch
    in the Dorico Brass map, so slots 5–7 stay aligned)
  - **Generic** — `ord, t2…t8` (ord + 7 open slots; percussion / unknown
    families — a customization seed)
- **`applyTechniqueNames(name0…)` native function** (`PluginEditor.cpp`):
  bulk-renames slots and grows `technique_count` to the supplied count (1–8) in
  one synchronous pass, firing a single `techniqueStateUpdated`.

### Notes
- **Non-destructive.** Applying a preset only renames slots and (if needed)
  grows the technique count to 8. Loaded samples are keyed by slot *index* and
  are not moved or deleted; the preset dropdown snaps back to its placeholder
  after applying, since slots remain freely renameable.
- **Always visible.** The selector sits above the technique bar (which stays
  hidden at `technique_count == 1`) so a fresh, single-technique instance can
  be switched to a full family layout in one click — wind/brass sample
  filenames are not auto-detected by the string-oriented filename router, so
  the count would not otherwise grow on its own.
- **Filename routing unchanged.** Slot names are cosmetic labels; the
  filename-token → slot router (`FilenameParser`) remains fixed to the string
  layout and is unaffected by renames.

## [1.19.0] - 2026-06-21

Enhancement: the **Output Gain** parameter now boosts up to **+24 dB** (was
+12 dB). The minimum (−24 dB) and default (0 dB) are unchanged. Useful for
quiet sample libraries that need more makeup gain than the previous ceiling
allowed.

### Changed
- **Output Gain range raised from −24…+12 dB to −24…+24 dB.**
  - C++: `output_gain` `NormalisableRange` max `12.0f → 24.0f`
    (`PluginProcessor.cpp`).
  - UI: `output_gain` knob display max `12.0 → 24.0`
    (`Resources/ui/js/sampler-app.js`) so the readout and drag travel cover the
    full new range.
- **Compatibility:** non-breaking for stored values — every saved/preset value
  in the old −24…+12 range is still valid and restores to the same dB
  (APVTS persists the denormalised value). Parameter ID, type, default, and
  state format are unchanged. **Caveat:** host *automation lanes* are stored
  normalised, so an existing lane written at the old top (1.0 = +12 dB) now
  resolves to +24 dB; re-scale any output-gain automation written before this
  version. No MAJOR bump taken because the range was *expanded*, not shifted or
  shrunk (no value clamps, no preset breakage).



Bug fix: a selected playing technique (e.g. **pizz**) now always plays that
technique instead of silently reverting to **ord** when a note's velocity falls
on a layer the articulation doesn't cover. Surfaced during Dorico keyswitch
playback. No parameter, state, or preset changes — fully backward-compatible.

### Fixed
- **Velocity-layer/technique sample-lookup fallback (`SampleMap::findCellNearestLayer`).**
  When an articulation was populated on fewer velocity layers than ord — e.g. a
  single-dynamic `pizz` (1 layer) against a 4-layer `norm`/ord set — any note
  whose velocity bucketed to a layer the articulation didn't cover played the
  **ord** sample, even though the technique cursor (and the UI technique-tab
  highlight) stayed on the selected articulation. Symptom in Dorico: a pizz
  passage "played pizz at first then reverted to ord," highlight unchanged.
  - **Root cause:** `findCellNearestLayer` called `findCell()`, whose per-layer
    technique→0 ("ord") fallback fired at the *requested* velocity layer
    **before** the cross-layer expansion ran. So the lookup returned the ord
    cell sitting on the requested layer instead of expanding outward to find the
    requested technique on its actual (populated) layer.
  - **Fix:** search the requested technique across **all** velocity layers first
    (via `findCellExact`, no ord substitution), expanding outward by layer
    distance; only fall back to technique 0 — again layer-tolerant — once the
    requested slot is proven empty on every layer. Technique fidelity now wins
    over velocity-layer fidelity: pizz is always pizz, relayered/repitched as
    needed.
  - **Regression safety:** fully-populated multi-layer libraries still hit the
    exact (layer, technique) cell first (unchanged); single-technique (ord-only)
    libraries take Phase 1 only (equivalent to prior behaviour); a genuinely
    empty technique slot (e.g. con sord with no samples) still falls back to ord
    via Phase 2 — the "empty slot plays ord" contract is preserved. The
    crossfade-partner path is unaffected (its `cellAdj->technique == xfadeTech`
    guard already rejected fallback ord cells).

### Known issues (not addressed in this patch)
- Filename auto-detect maps `trem`→slot 8 and `flaut`→slot 9, outside the 8-slot
  keyswitch range, so an auto-detected tremolo folder never reaches the Dorico
  tremolo keyswitch slot (7). Use the load modal's **Force all samples onto this
  technique** targeting the correct slot as a workaround.
- The load modal's **target technique** dropdown is a no-op unless **Force** is
  also checked: when override is off and a filename carries no technique token,
  the loader defaults to slot 0 rather than the chosen target slot.

## [1.18.2] - 2026-06-20

Cosmetic UI fix: the **Batch loop…** button in the Sample Map drop-zone now
matches the other buttons in that row (Load Folder…, Clear samples).

### Fixed
- **Batch loop button styling.** `#batch-loop-btn` had no CSS rule, so it
  rendered with the default browser button chrome (grey bevel, system font)
  while its row neighbors used the warm cream/`border-warm` plugin style. Root
  cause: the button was added in v1.18.0 without a matching style rule. Fix:
  grouped `#batch-loop-btn` into the existing `#load-folder-btn` selectors
  (base, `:hover:not([disabled])`, and `[disabled]`) so the two share the
  neutral primary-action style and the disabled/hover states stay in sync.
  No audio, parameter, or state changes.

## [1.18.1] - 2026-06-20

Cosmetic UI relabel: the Sample Map's velocity layers now read as **dynamic
markings** (`p` / `mp` / `mf` / `f`) instead of numeric MIDI-velocity ranges or
`L0`–`L3` indices. Matches the `p/mp/mf/f` tokens the filename parser already
recognizes. No audio, parameter, or state changes.

### Changed
- **Velocity-row labels (sidebar).** The four `.vel-label` rows now show
  `f` / `mf` / `mp` / `p` (loudest-on-top) instead of the MIDI-velocity range
  (`97–127`, `65–96`, …). Mapping: L0→`p` (softest), L1→`mp`, L2→`mf`,
  L3→`f` (loudest) — driven by a new `VELOCITY_MARKS = ['p','mp','mf','f']`
  table feeding a `mark` field on `velocityLayerToRange()`.
- **Cell tooltips** now read `Vel f (97–127)` — the dynamic mark first, with the
  numeric MIDI range retained in parentheses for reference.
- **Loop-editor header** shows the mark (`f`) instead of `L3`; the static `L`
  prefix was removed from `index.html` (`<span id="le-vel">`).
- **Confirm dialogs / toasts** for delete-cell, clear-layer, and the
  round-robin merge prompt now refer to layers by mark (e.g. "Clear velocity
  layer `mf`?") instead of `L2`.
- **Round-robin duplicate-confirm header** shows the mark instead of
  `Layer N`.

### Implementation notes
- Single source of truth: `velocityLayerToRange(layer).mark` is reused at every
  call site, so the soft→loud order and labels can never drift between the grid,
  tooltips, sidebar, and dialogs.
- The numeric `label` (MIDI range) is unchanged and still available — only its
  visible placement moved into the tooltip parentheses.
- Pure presentation change in `Resources/ui/js/sampler-app.js` +
  `Resources/ui/index.html`; no C++, DSP, parameter, or saved-state changes, so
  existing sessions and presets load identically.

## [1.18.0] - 2026-06-20

Two sample-management features: a **batch loop-point** control that sets one
loop region across every loaded sample at once, and **granular deletion** —
remove a single cell or clear a whole velocity layer instead of only being able
to wipe the entire map.

### Added
- **Batch loop points (`applyLoopPointsToAll`).** A new "Batch loop…" button
  (next to "Clear samples") opens a modal that applies one loop region to
  **every variant of every cell** in a single action. Two unit modes:
  - **Proportional (%)** — loop start/end as a percentage of *each* sample's
    own length (e.g. 0–100 %), scaled per-sample. Best for a mixed-length
    library where one absolute position can't fit all samples.
  - **Milliseconds** — loop start/end in ms from the file start, converted
    per-variant via its own `sourceSampleRate` (falls back to 48 kHz when the
    rate is unknown).
  Each variant is clamped exactly like the per-cell loop editor
  (`overrideLoopPoints`), set to `LoopMode::Manual`, and one-shot buffers
  (< 18 samples) are skipped untouched. A toast reports how many samples were
  updated. Applies on the next note-on, like the per-cell editor.
- **Delete a single sample (`deleteSampleCell` → `removeCell`).** The
  right-click cell menu's previously-disabled "Clear" entry is now an enabled
  **"Delete sample"** action. It removes just that (note, velocity layer) cell
  on the **active technique**, leaving the rest of the map intact. Disabled on
  empty cells; behind the standard confirm dialog.
- **Clear a whole velocity layer (`clearVelocityLayer`).** **Right-click a
  velocity-row label** to remove every sample in that layer across **all
  techniques** (mirrors the existing `LoadMode::ReplaceLayer` semantics). The
  row-label tooltip advertises the gesture; behind the confirm dialog; reports
  the count cleared.

### Implementation notes
- All three operations follow the established deep-copy → mutate → version-bump
  → `atomicStore` → `sampleMapChangedCallback` pattern used by
  `overrideLoopPoints` / `clearSampleMap`, so active voices keep their prior
  map snapshot and in-flight notes finish naturally (Stage 2 EC-3).
- `removeCell` / `clearVelocityLayer` recompute note bounds + velocity-layer
  count via a shared `recomputeMapBounds` helper (mirrors `applyFolderLoad`)
  and reset the affected round-robin counters.

### Known limitation (reload persistence)
- The sample map is persisted as a **replay of load-ops**, not as a serialized
  map. Like the existing per-cell loop editor, these new operations mutate the
  live map only — they are **not recorded as load-ops**, so **deletes,
  layer-clears, and batch loop overrides apply for the session but are undone
  when a project reloads and replays its op history** (re-loading the source
  folder restores deleted cells). This matches current loop-editor behaviour
  and introduces no regression. Making these survive reload would require
  recording delete/override ops (or a freeze-to-embedded-snapshot) — deferred
  to a follow-up to avoid touching the working save/load path.

## [1.17.2] - 2026-06-19

Fixes pre-note velocity-layer parsing so dynamics that appear **before** the
pitch token (the dominant orchestral naming convention) are honoured, and adds
a playback safety net so this can never silence a library.

### Fixed
- **Pre-note dynamics (`p`/`mp`/`mf`/`f`) are now parsed as velocity layers
  (PARSE-DYN).** Filenames like `vln_norm_mf-A#2-V127-JXRO.aif` place the
  dynamic immediately before the pitch. The velocity scan previously **skipped**
  dynamics tokens in the pre-note region (only explicit `v1–4`/`vel1–4`/`L[N]`/
  `layer[N]`/`lyr[N]` forms were accepted there), so `mf` was ignored and the
  sample defaulted to **velocity layer 0** instead of **layer 2**.
  - **Root cause:** the pre-note tier used `parseAsExplicitVelocity`, which
    explicitly rejected `p`/`mp`/`mf`/`f`. That suppression was added in v1.14.0
    to stop a single-dynamic library (e.g. all `mp`) from landing every slot on
    a non-zero layer — which silenced the bottom of the velocity range because
    `MicrotonalSamplerVoice::startNote` bailed to silence when the resolved
    layer's cell was empty. Pre-note `mf` (a real dynamic) and a pre-note
    name-fragment are structurally identical, so position alone can't tell them
    apart — the suppression was throwing out valid dynamics.
  - **Fix:** the pre-note tier now accepts any velocity form, including
    dynamics. Pre/post-note dynamics are symmetric again.

### Changed
- **Velocity layer lookup falls back to the nearest populated layer
  (`SampleMap::findCellNearestLayer`).** The voice's PRIMARY cell lookup now
  expands outward by layer distance when the velocity-bucketed layer is empty,
  instead of going silent. This is the safety net that makes honouring pre-note
  dynamics safe: a single-dynamic library (every slot on one non-zero layer)
  now plays across the entire velocity range. Equidistant layers prefer the
  lower (quieter) layer. The crossfade-partner lookup stays **exact** (an empty
  adjacent layer still means "no crossfade"), and fully-populated multi-layer
  libraries never reach the expansion loop, so they are bit-identical.
- **`norm` / `normale` / `normal` recognised as technique slot 0 (`ord`).**
  Previously these only landed on slot 0 by the missing-token default; they are
  now explicit aliases for the plain articulation.

### Testing
- `FilenameParser` inline tests updated: pre-note `mp`/`mf` cases now assert
  their dynamic layer (flipped from 0); added the user's exact filename
  (`vln_norm_mf-A#2-V127-JXRO` → midi 58, layer 2) and `norm`/`normale` cases.
- `technique_parse_check.cpp`: added `norm`/`normale`/`normal` → slot 0 cases.
- `dynamics_layer_check.cpp`: added `findCellNearestLayer` cases — single-layer
  fallback, equidistant lower-layer tie-break, full-velocity-sweep
  no-silence, and a fully-populated-library no-op (nearest == exact).

## [1.17.1] - 2026-06-01

Internal efficiency + simplification pass from a full code audit. **No
user-facing behaviour change** — no parameters, state format, or audio-render
output altered. All changes are behaviour-preserving cleanups.

### Changed (efficiency)
- **Audio thread: cached APVTS parameter pointers (EF-1).** `processBlock` was
  resolving 10 parameters by string ID every block (`polyphony`, `ks_enabled`,
  `ks_low_note`, `ks_high_note`, `technique_count`, `cc_select_enabled`,
  `cc_number`, `pc_enabled`, `expression`, `output_gain`) — each call hashing
  the ID and walking the APVTS map. These `std::atomic<float>*` pointers are now
  resolved ONCE in the constructor (`cacheAudioParamPointers`) and read directly
  on the audio thread. The pointers are stable for the processor's lifetime.
- **SampleLoader: removed per-file triple-buffering (LOAD-E3).** `processOneFile`
  no longer allocates an intermediate `workBuf`; resampling writes straight into
  the 2-channel output buffer, and source channels beyond the first two (which
  were discarded anyway) are no longer resampled. Decoded output is bit-identical.
- **SampleLoader: O(n²) → O(n log n) cell grouping (LOAD-E4).** Folder grouping
  replaced the per-unique-key full scan with a single sort of an index array by
  `(encodeKey, rr-sort-token, load-order)` + one linear pass. The comparator
  reproduces the previous cell order AND within-cell variant order exactly, so
  the built `SampleMap` is identical — only large-library load time improves.
- **FilenameParser: lower-case each token once (PARSE-S1).** `parse()` now
  pre-lower-cases tokens into `lcTokens` and passes them to the case-insensitive
  helpers (MIDI form / velocity / RR / technique), which previously each
  re-allocated a lower-cased copy per call (4–6× per token). `parseAsScientificPitch`
  deliberately keeps the ORIGINAL token — it is case-SENSITIVE (`'b'`=flat vs
  `'B'`=note letter); lower-casing it would change pitch parsing.
- **WebView drag-drop: hoisted native-fn binding out of the streaming loop
  (DROP-E1).** `streamFolderEntryToCpp` resolved `getNativeFunction('dropSessionAddFile')`
  on every file (250× for a 250-file folder); it is now bound once before the
  loop. Applied in the shared `webview-drop-streaming` module (O-MicrotonalSampler
  is its only current consumer).

### Removed (dead code / stale comments)
- Dead CSS rule `.grid-cell.cell-active` (note-on highlight that was never wired;
  the class is never applied in JS). (CSS-M5)
- Dead gate-metric instrumentation in `sampler-app.js`: the `performance.now()`
  `t0`/`t1` capture, the per-load `console.log` timing line, and the write-only
  `lastReplaceTimestamp`. (APP-S2)
- `tuning-panel.js`: write-only `heldNotes` Set (TUN-L6); no-op
  `toFixed(isOctave ? 1 : 1)` → `toFixed(1)` (TUN-L5); the static
  `interval-list-header` in `render()` that `updateIntervalList()` always
  overwrote on first load (TUN-H2).
- Refreshed stale "Stage 1 / silent shell / SKELETONS" banners and member
  comments in `PluginProcessor.{h,cpp}` and `PluginEditor.cpp` to describe the
  shipping plugin rather than scaffolding state.

### Notes
- **Shared-module note:** TUN-L5/L6/H2 were applied to this plugin's local copy
  of `Resources/ui/js/tuning-panel.js`, which has **forked** from the
  `scala-tuning-engine` module (currently v2.1.0). These fixes are
  O-MicrotonalSampler-specific and are **deliberately NOT propagated upstream**:
  - **L6** (remove `heldNotes`) — `heldNotes` is dead here only because this
    plugin's v3.1.0 `drawTrueKeys()` rewrite uses `heldNotesMidi`/`heldNotesFreqs`
    instead. The module's `drawTrueKeys()` still *reads* `heldNotes`, so removing
    it upstream would break the module and O-Formant.
  - **L5** (`toFixed(1)`) — a local display choice (1-decimal cents). The module
    uses `toFixed(2)`; upstreaming would change precision for O-Bells / O-Formant
    / O-IntonationPad.
  - **H2** (drop redundant static `interval-list-header`) — the only genuinely
    shared cleanup, left un-upstreamed (cosmetic dead markup only).
  Because this panel is a fork, a naive `module-upgrade` would already overwrite
  far more than these three edits (the v3.1.0 `drawTrueKeys`, the 1-decimal
  display), so the module-upgrade customization warning — not upstreaming — is
  the right guard.
- **Verified-safe (left intact):** the back-compat `slots` JSON array and
  `SampleCell::primary()` (both still consumed by the JS grid / snapshot path),
  the OFF-by-default Phase-2.1 sine-burst test fixture, and the
  intentionally-disabled `clear` context-menu button.
- **Testing:** TechniqueParseCheck (62 cases) + MergeRr / FindCellTriplet /
  DynamicsLayer / CcPcTrigger / StateMigration / DropSessionGuard standalone
  suites all pass; `auval` AU validation succeeds. Manual DAW verification of a
  real folder load (LOAD-E3/E4 path) is recommended as the loader grouping/resample
  paths have no standalone unit test.

## [1.17.0] - 2026-05-06

### Added
- Folder-load options modal now exposes **technique targeting** alongside
  the existing layer targeting. Users can:
  - Pick a **target technique** from a dropdown populated with the user's
    current technique slot names (renameable via the technique strip:
    `ord, sp, st, sv, cs, pizz, harm, mart` by default).
  - Toggle **"Force all samples onto this technique"** to override
    filename-token routing (`_pizz`, `_harm`, etc.) and force every file
    in the folder onto the chosen technique slot.
  - The technique dropdown defaults to the **currently-active technique
    tab**, so workflows like "select pizz tab → drop pizz folder" land
    where the user is already looking.
- Both targeting axes (layer / technique) have **independent force toggles**
  — you can force technique while letting filenames decide layer, or
  vice versa. Mirrors how `SampleLoader.cpp` already treats
  `overrideTokens` (layer) and `overrideTechnique` (technique) as
  independent flags.

### Changed
- Folder-load options modal — UX clarity rework for layer targeting:
  - Replaced the segmented `L0 | L1 | L2 | L3` button group with a
    `<select>` dropdown. Same payload (`targetLayer 0..3`), clearer
    affordance for users who prefer a labelled menu over compact pills.
  - Renamed the `Ignore filename velocity tokens (e.g. _v1, _ff)` checkbox
    to `Force all samples onto this layer` with a sub-label that explains
    the alternative (filename tokens decide layer when off). The original
    label described the *negative* behaviour (what gets ignored); the new
    label describes the *positive* outcome (every file lands on the chosen
    layer), which is what the user actually selects this option for.

### Why
- The technique-override path has existed in the loader since v1.14.0
  (`LoadOp::targetTechnique` + `LoadOp::overrideTechnique`,
  `SampleLoader.cpp:296-298`) but was unreachable from the modal — the
  flags were only set via direct `LoadOp` construction during state
  rehydrate. This release surfaces that capability through the user-facing
  load flow.
- Symmetrically, the "force load on a specific layer" capability has
  existed since v1.7.0 (`LoadOptions::overrideTokens` —
  `SampleLoader.cpp:286-288`) but was gated behind an opaque label that
  described the mechanism, not the intent. The rename + dropdown
  restructure puts both targeting axes on equal footing in the UI.

### Implementation
- `Resources/ui/index.html` — segmented Layer buttons → `<select>` dropdown;
  new Technique row with `<select id="flo-technique-select">` (options
  injected at modal-open from `getTechniqueState().names`); new
  `Force all samples onto this technique` checkbox below the layer one.
- `Resources/ui/js/sampler-app.js` — `showFolderLoadOptionsModal` now
  `async`; fetches technique names + active index via existing
  `getTechniqueState` native fn; rebuilds the technique dropdown each open
  so renames are picked up; defaults dropdown to active tab; payload
  extended to `{ layer, mode, override, technique, overrideTechnique,
  embedAudio }`. Old `.flo-segmented` / `.flo-seg` handlers replaced with
  `change`-event handlers on both `<select>` elements.
- `Resources/ui/css/sampler-shell.css` — old `.flo-segmented` / `.flo-seg`
  rules removed; new `.flo-select` (cream bg, warm border, gold focus
  ring, inline SVG chevron) and `.flo-override-text/-title/-sub` for the
  two-line override labels.
- `modules/core/webview-drop-streaming/js/webview-drop-streaming.js` —
  `dropSessionCommitFolder` call site extended with two trailing args
  (`technique`, `overrideTechnique`). Old call sites that don't supply
  them get default 0/false.
- `modules/core/webview-drop-streaming/cpp/WebViewDropStreaming.h` —
  `OnCommitFolder` callback signature extended with `int targetTechnique`
  + `bool overrideTechnique` trailing params. `dropSessionCommitFolder`
  handler reads `args[5]` / `args[6]` (defaults preserved). Module is
  consumed only by O-MicrotonalSampler today; no other plugins broken.
- `Source/PluginEditor.cpp` — drop `onCommitFolder` lambda receives the
  two new params and forwards to `processorRef.loadSampleFolder`;
  `loadSampleFolderByPath` native fn handler reads `args[5]` / `args[6]`
  with the same fallback pattern.
- `Source/PluginProcessor.h/.cpp` — 7-arg `loadSampleFolder` overload
  extended to 9 args with defaulted `targetTechnique = 0` /
  `overrideTechnique = false`. Pre-v1.17.0 callers continue to compile
  unchanged; the new fields land on `LoadOp::targetTechnique` /
  `LoadOp::overrideTechnique`, which `SampleLoader.cpp:296-298` already
  wires to the per-file `effectiveTechnique` decision.

### Persistence (saved-state round-trip)
- LoadOp serialisation extended with `technique` + `overrideTechnique`
  properties. Emitted only when non-default to keep clean diffs for the
  common "filename-tokens decide" case. Pre-v1.17.0 saved states load
  identically: missing properties default to `0` / `false`, matching
  pre-v1.17.0 implicit behaviour.

### Verification
- Visual inspection in Standalone (file-dialog flow + drag-drop flow).
- Confirmed technique dropdown populates from `getTechniqueState()` and
  defaults to active tab.
- Confirmed force-technique toggle independently controls
  `overrideTechnique`.
- Confirmed C++ build is clean (no warnings, no errors).

### Backward compatibility
- Preset / saved-state format: forward-compatible. Old states load with
  technique=0/override=false (pre-v1.17.0 implicit behaviour). New states
  written by v1.17.0+ that omit the technique/overrideTechnique
  properties are equivalent to the default — old plugin builds will read
  them correctly.
- Native function `loadSampleFolderByPath`: backward-compatible. New
  optional args[5]/args[6] default to 0/false when JS callers don't
  supply them.
- Shared module `WebViewDropStreaming` callback signature: BREAKING for
  consumers of the module. Verified only O-MicrotonalSampler uses it
  today; updated in lockstep.

## [1.16.11] - 2026-05-05

### Changed
- Phase 3 sweep — MEDIUM-04 (Batch C, the final remaining audit candidate) applied.
  - **[MEDIUM-04]** Added a unified `bindModal(dialog, buttons, opts)` helper to
    `sampler-app.js`. Centralises the `addEventListener` / `cleanup` /
    `removeEventListener` / Esc–Enter dance that previously recurred across 7
    dialogs (folder-load-options, embed-size-confirm, per-cell-merge, generic
    confirm, diagnostic, missing-folder, ambiguous-RR confirm). Each dialog
    converted to a single `bindModal(...)` call site; double-fire and
    forgotten-listener bug classes are now structurally impossible. Helper
    supports asymmetric cases via:
    - Function-form key targets (`opts.keys.Enter: () => capHit ? replBtn : mergeBtn`)
    - Optional key bindings (diagnostic-dialog has Esc only — no Enter)
    - `opts.onClose` for non-dismissing extra listeners (folder-load-options
      live-preview handlers, diagnostic-dialog copy button)
    - Async button handlers awaited before the modal resolves
  - The `technique-rename-dialog` is intentionally left untouched — it uses a
    text-input lifecycle (commit-on-Enter / cancel-on-Esc with input focus),
    which is not the same shape as the 7 button-driven dialogs the audit
    targeted.

### Verification
- Build (macOS VST3 + AU): PASS — clean compile.
- auval: PASS.
- Listener-leak greps:
  - `const cleanup = ` in modal lifecycle code → zero hits (helper owns it).
  - `removeEventListener(.*onKey` outside the helper → zero hits.
  - `bindModal` call sites → 7 (folder-load-options, embed-size-confirm,
    per-cell-merge, confirm-dialog, diagnostic, missing-folder, rr-confirm).
- Standalone manual smoke (deferred to user-side UAT — MEDIUM-risk gate):
  - Folder-load options: layer/mode/override/embed live-preview, confirm/cancel/Esc/Enter
  - Embed-size confirm: appears for large folders, all paths
  - Per-cell merge: 3-button (merge/replace/cancel); cap-hit disables merge,
    Enter falls back to replace
  - Generic confirm (clear samples): destructive style + onConfirm callback
  - Diagnostic dialog: copy button updates label without dismissing; close + Esc dismiss
  - Missing-folder: skip = `dismissMissingFolder`, locate = native picker;
    dialog dismisses synchronously before the OS picker opens
  - Ambiguous-RR confirm: `sendRrConfirmation(true|false)` based on choice

This commit closes out the SIMPLIFICATION-AUDIT.md backlog — all HIGH and
MEDIUM candidates from the original audit have now been applied (Phase 1 +
Phase 2 + Phase 3 Batches A/B/C).

See [SIMPLIFICATION-AUDIT.md](.planning/SIMPLIFICATION-AUDIT.md)
`## Phase 3 Applied (v1.16.11)` for the per-candidate record.

## [1.16.10] - 2026-05-05

### Changed
- Phase 3 sweep — 7 MEDIUM-severity, LOW-risk simplification candidates applied
  (Batch B from `.planning/SIMPLIFICATION-AUDIT.md`). Net source LOC roughly
  flat (helpers add depth in one place but each amortises across many call
  sites — e.g. MEDIUM-06 alone collapses 14 five-line invoke blocks into
  one-liners). No behaviour change.
  - **[MEDIUM-02]** Hoisted RR counter index packing into
    `MicrotonalSamplerVoice::packRrCounterIndex(midi, layer, tech)` constexpr
    helper. Four call sites (`PluginProcessor.cpp` ReplaceLayer wipe / folder-load
    apply / single-sample load + `MicrotonalSamplerVoice::selectVariantIndex`)
    now route through the helper. Layout coupling to `kRrCounterSize` is now
    expressed in one place; the literal `* 4 * 8 + … * 8 + …` no longer recurs.
    `kRrCounterSize` itself rewritten as `128 * 4 * kMaxTechniques` (was
    `128 * 4 * 8`). Local `constexpr int kMaxTech = 8` in voice.cpp removed.
  - **[MEDIUM-03]** Added `joinJsonArray` template helper in the
    `PluginEditor.cpp` anonymous namespace. Three native-fn registry sites
    (`getEmbeddedTuningList`, `getEmbeddedTuningCategories`, `getSkippedFiles`)
    converted from indexed `for (size_t i = 0; ...)` JSON loops to range-for +
    first-flag form via the helper. The CSV intervals loop in
    `captureTuningValueTree` was deliberately left as-is — it emits to a
    comma-separated string with no `[…]` wrapping (different shape).
  - **[MEDIUM-05]** Added `num(v, fallback)` JS helper. Replaced 18
    `Number.isFinite(x) ? x : default` ternaries across snapshot-deserialise
    paths (loop editor, technique state, trigger state, sample-map snapshot).
    Sites that use `Number.isFinite` as a control-flow guard
    (`if (!Number.isFinite(x)) return`) preserved — different intent.
  - **[MEDIUM-06]** Added `invokeNative(name, ...args)` async wrapper.
    Replaced 14 `if (!window.__JUCE__) return; try { Juce.getNativeFunction(...)... }
    catch ...` blocks with single-line `await invokeNative(...)` calls
    (KS/CC/PC enables, CC/PC mapping setters, technique slot ops, dialog
    confirms, etc.). Standardised `[sampler-app] {name} failed` error tag.
    Preserved sites: backend `addEventListener` subscriptions, sites with
    user-visible toast on catch (saveCurrentPreset / loadPreset / locateMissingFolder),
    sites with semantic-dependent missing-host fallback (setActiveTechnique).
  - **[MEDIUM-07]** Extracted generic `mutateMappingSlot` template into the
    `PluginProcessor.cpp` anonymous namespace. `setCcMappingSlot` and
    `setPcMappingSlot` collapsed by sharing the COW boilerplate (atomic load →
    make_shared from current-or-default → mutate slot → atomic store). The
    `triggerStateDirty.store + triggerAsyncUpdate()` notification is intentionally
    outside the helper — it fires from each call site so the helper stays
    purely structural.
  - **[MEDIUM-08]** Extracted
    `OMicrotonalSamplerAudioProcessor::publishMissingFolderIfNew(kind, path,
    displayName)` member helper. Two near-identical "first-missing publish"
    blocks in `kickNextReplayOp` (drag-drop case + filesystem case) collapsed
    to one-line helper calls. "First-missing-only" semantics now expressed in
    one place.
  - **[MEDIUM-09]** Removed redundant explicit empty default constructor
    `MicrotonalSamplerSound() {}` in favour of the implicit one. Class is now
    pure interface overrides.

### Verification
- Build (macOS VST3 + AU): PASS — clean compile, no new warnings.
- auval: PASS.
- Format-stability greps:
  - `* 4 * 8 +` literal in `PluginProcessor.cpp` / `MicrotonalSamplerVoice.cpp`:
    zero non-helper hits (only inside the `packRrCounterIndex` definition and
    the `kRrCounterSize` declaration).
  - `Number.isFinite` in `sampler-app.js`: drops from 33 → 18 (helper +
    preserved guard sites; comment lines included).
  - `window.__JUCE__` in `sampler-app.js`: drops from 49 → 37 (helper +
    backend event-listener subscriptions + preserved toast-on-catch sites +
    comments).
  - `MicrotonalSamplerSound() {}`: zero hits.
- Standalone smoke (deferred to user-side UAT):
  - KS / CC / PC enable toggles, CC mapping slot edit + trigger callback,
    missing-folder toast (fires once on multi-op replay), loop editor
    deserialise with default fallbacks, scale-generator JSON.
- RR drift smoke (deferred to user-side UAT): 8× repeat-note cycle,
  ReplaceLayer wipe reset, single-cell load reset.

Batch C (MEDIUM-04, dialog modal `bindModal` helper) deferred — separate run
required because of the MEDIUM-risk gate and 7-modal manual smoke surface.

See [SIMPLIFICATION-AUDIT.md](.planning/SIMPLIFICATION-AUDIT.md)
`## Phase 3 Applied (v1.16.10)` for the full per-candidate record.

## [1.16.9] - 2026-05-05

### Changed
- Phase 3 sweep — 4 LOW-severity simplification candidates applied (see
  `.planning/SIMPLIFICATION-AUDIT.md` for the full audit).
  - **[LOW-01]** Extracted `OMicrotonalSamplerAudioProcessor::resetAllRrCounters()`
    helper. Three identical `for (auto& c : rrCounters) c.store ((uint8_t) 0xFFu,
    std::memory_order_relaxed);` loops in `PluginProcessor.cpp` (constructor,
    `applyFolderLoad` ReplaceAll branch, `clearSampleMap`) collapsed into
    single-line calls. Memory order, sentinel value, and `noexcept` contract
    preserved.
  - **[LOW-03]** Replaced `lastWidthBucket` string ('wide'/'narrow') with
    `lastIsNarrow` boolean in `sampler-app.js` narrow-window guard. The string
    bucket served no purpose — boolean is a more direct match for the predicate
    `w < NARROW_BREAKPOINT_PX`. No behaviour change (initial `null` preserved
    so first invocation still proceeds).
  - **[LOW-05]** Moved the `makeVector` lambda from the file-anonymous namespace
    at the top of `PluginEditor.cpp` into the body of `getResource()` (its only
    call site, ~12 invocations). Lambda signature/body byte-identical; only
    scope changes.
  - **[LOW-07]** Replaced `startTechnique = 0; if (...) startTechnique = ...;`
    with a single ternary in `MicrotonalSamplerVoice::startNote` for symmetry
    with surrounding code. `std::memory_order_acquire` and `juce::jlimit (0,
    kMaxTechniques - 1, ...)` clamp preserved (RT-safety contract from v1.14.0).

### Verification
- Build (macOS VST3 + AU): PASS
- auval: PASS
- Greps:
  - `0xFFu` literal in `PluginProcessor.cpp` confined to the `resetAllRrCounters`
    definition + three remaining single-cell stores in CC/PC counter-index
    paths (NOT reset-all loops).
  - `lastWidthBucket`: zero hits in `sampler-app.js`. `lastIsNarrow`: 3 hits
    (declaration + comparison + assignment).
  - `makeVector`: all 12 hits inside `getResource()`. Anonymous-namespace
    definition removed.
  - `startTechnique = 0` line: zero hits in `MicrotonalSamplerVoice.cpp` (the
    ternary subsumes both branches).
- Visual smoke: narrow-window auto-close + toast still triggers on bucket
  transitions across the 900px boundary.
- DSP sanity: round-robin selection rotates as expected; technique-freeze on
  note-start unchanged.

### Note
Two LOW items from the audit (LOW-04 enum doc order, LOW-06 duplicate
double-click comment header) re-verified as false positives in current code
and were excluded from this sweep. The `LoadMode` enum doc already matches
the enum value order, and the "250 ms double-click" canonical comment lives
once at `sampler-app.js:854` (the cross-reference at line 659 is load-bearing
context, not duplication). Both moved to a `## Phase 3 Skipped (re-verified
false-positive)` section in the audit.

## [1.16.8] - 2026-05-05

### Changed
- **Code simplification — Phase 2 (audit candidates HIGH-02 / HIGH-06).**
  Pure structural deduplication; no behaviour change.
  - **[HIGH-02]** `PluginProcessor.cpp`: deduplicated `loopModeToString`. Three
    copies (two in-function lambdas + one namespace-scope mapper, with
    inconsistent hyphen-vs-underscore spelling) collapsed into two canonical
    helpers in the top anonymous namespace:
    - `loopModeToJsonString()` → hyphenated form (`"one-shot"`, `"auto"`,
      `"manual"`) for JSON UI payloads — what the WebView loop-editor parses
      from `sampleMapUpdated` events / `snapshotSampleMapJson()` /
      `snapshotWaveformPeaks()` results.
    - `loopModeToXmlString()` → underscored form (`"one_shot"`, `"auto"`,
      `"manual"`) for the embedded-audio XML state ValueTree, paired with the
      existing `loopModeFromString()` deserialiser.
    Both spellings are LOAD-BEARING and are preserved exactly — JS UI relies
    on hyphens, saved projects from v1.0+ rely on underscores.
  - **[HIGH-06]** `Resources/ui/index.html` + `Resources/ui/js/sampler-app.js`:
    consolidated 8 nearly-identical `<div class="ouaricon-knob">` blocks
    (~99 lines of HTML) into a single JS render driven by `SLIDER_BINDINGS`
    (now extended with `label` and optional `tooltip` fields). New
    `renderControlStrip()` helper runs at boot before `bindOneKnob` so
    `getElementById` lookups still resolve. `<footer id="control-strip">`
    survives as the JS render target. Future knob additions become a
    one-line `SLIDER_BINDINGS` entry.

### Verification
- Build: `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU` succeeds with
  no new warnings.
- AU validation: `auval -v aumu OMtS Ouar` passes.
- Format stability:
  - JSON path: `"one-shot"` literal appears only inside
    `loopModeToJsonString()`; 3 callers reference the helper.
  - XML path: `"one_shot"` literal appears in `loopModeToXmlString()` writer
    and the `loopModeFromString()` parser; XML state round-trip works
    unchanged.
- Visual smoke (HIGH-06): JS-rendered DOM is structurally equivalent to the
  prior static HTML — same `<div class="ouaricon-knob">` wrapper, same SVG
  geometry, same input ids/min/max/step. CSS selector dependency check ran
  clean. Per-knob standalone verification (drag, value readouts, hover
  states, expression tooltip) recommended before public release; the
  refactor is mechanical and the binding contract (`bindOneKnob` → unchanged)
  is preserved.
- CSS selector dependency check: zero positional selectors
  (`:nth-child` / `:first-child` / `:last-child` / `>` direct-child) target
  `.ouaricon-knob` — JS-rendered DOM is a drop-in match for the static HTML.

See `plugins/O-MicrotonalSampler/.planning/SIMPLIFICATION-AUDIT.md` (Phase 2
Applied section) for the full candidate spec.

## [1.16.7] - 2026-05-05

### Changed
- **Code simplification — Phase 1 (audit candidates HIGH-01/03/04/05).** Pure helper
  extraction; no behaviour change.
  - `PluginProcessor.cpp`: collapsed 12 `__cpp_lib_atomic_shared_ptr` `#if/#else`
    blocks behind file-local `atomicLoad` / `atomicStore` template helpers. The
    site at lines 510–516 was a dead conditional (both arms identical). The
    deprecated free-function `std::atomic_load`/`atomic_store` overloads are
    still the underlying implementation.
  - `PluginEditor.cpp`: extracted three anonymous-namespace helpers:
    - `buildNotesFreqsJson(notes, freqs)` — replaces 2 duplicated blocks emitting
      `{"notes":[…],"freqs":[…]}` with 4-digit freq precision (held-notes
      broadcast, `getHeldNotesJson` native fn).
    - `centsArrayToJson(cents)` — replaces 4 duplicated 6-digit-precision JSON
      builders in `getTuningIntervals`, `generateEDO`, `generateHarmonicSeries`,
      `generateRank2`.
    - `setBoolParamFromArgs(apvts, paramId, args)` — replaces 3 copies of the
      bool-arg `setValueNotifyingHost` pattern in `setKeyswitchEnabled`,
      `setCcEnabled`, `setPcEnabled`. Public contract unchanged
      (`complete(false)` on bad args / missing param, `complete(true)` on
      success).
  - Net diff: −54 LOC (167 deletions, 113 insertions including helper bodies +
    explanatory comments).

### Verification
- All four extractions are pure mechanical refactors. Output formats (JSON
  shape, freq/cents precision, callback ordering) are byte-identical to
  v1.16.6.
- Items on the audit's "Skipped (false-positive checks)" list — drag-drop
  streaming, resource-provider URL handling, `Juce` vs `window.__JUCE__`,
  microtonal top-level exp-map fields, RR/voice DSP, WebView2 guards — were
  not touched.
- Audit report: `plugins/O-MicrotonalSampler/.planning/SIMPLIFICATION-AUDIT.md`.

### Note
- Missing v1.16.6 changelog entry. Commit `4883886` shipped v1.16.6 ("restore
  exp-map top-level microtonal fields — TC-4 regression fix") but no CHANGELOG
  entry was written. Backfill on next pass.

## [1.16.5] - 2026-05-05

### Fixed
- **TC-2: Family-aware Dorico Playback Template auto-routing now
  actually works** (was broken in v1.16.3, partially diagnosed in
  v1.16.4). Validated in Dorico 6 with a 4-stave project (Solo Violin,
  Flute, Trumpet, Marimba): apply emits zero "Can't find a template
  spec…" warnings, four `Loading Plugin into slot` events fire
  back-to-back, MIDI thru routes to four distinct endpoint slots
  (1024 / 2048 / 3072 / 4096). Each of the four endpoint configs now declares an
  `<instruments array="true">` block enumerating the canonical instrument
  entityIDs from
  `/Applications/Dorico 6.app/Contents/Resources/instrumentFamiliesDefinitions.xml`:

  | Endpoint config | Family | Instrument IDs |
  |---|---|---|
  | `O-MicrotonalSampler` | Strings (`instrument.strings.*`) | 19 |
  | `O-MicrotonalSampler-Brass` | Brass (`instrument.brass.*`) | 100 |
  | `O-MicrotonalSampler-Winds` | Woodwinds (`instrument.wind.*`) | 84 |
  | `O-MicrotonalSampler-Generic` | Pitched-perc, unpitched-perc, keyboards, singers, fretted, orff, electronics, gamelan, sketch | 345 |

  Each `<instrumentData>` is a fixed shape:
  ```xml
  <instrumentData>
      <entityID>instrument.strings.violin</entityID>
      <index>0</index>
      <irvIndex>0</irvIndex>
      <playerType>kSoloPlayer</playerType>
      <endpoints>0</endpoints>
  </instrumentData>
  ```
  `endpoints>0` points at slot 0 in the same endpoint config (each
  endpoint config still has exactly one `<slotData>` defining a single
  `O-MicrotonalSampler-dev` instance template — Dorico instantiates
  a fresh plugin per stave from that template).

  Aliases (entityIDs containing `.alias.`) are excluded from the
  enumeration; Dorico resolves alias → canonical at score-load time.

  **Architectural correction.** The v1.16.3 design assumed
  `<instrumentFamilies>` in `playbacktemplatespec.xml` was the routing
  filter. It isn't — it's a vestigial / editor-only field. The actual
  routing filter is `<instruments>` at the endpoint-config level (this
  matches the Ample China user template's pattern: empty
  `<instrumentFamilies/>` in spec, ~11 `<instrumentData>` entries in the
  endpoint config). v1.16.4's entityID-format spec change was harmless
  but didn't fix routing on its own.

  **Why this was hard to spot:**
  - The Dorico binary clusters `endpoints / configID / slotData /
    instrumentData / genSpecID / playbackTemplateSpecID` as the routing
    schema — `instrumentFamilies` lives only in the editor UI strings
    (`InstrumentFamiliesEditor.qml`).
  - The `<instrumentFamilies>` element parses without error when present
    (so v1.16.3 ingest-test passed) but nothing reads it for routing.
  - No factory `playbacktemplatespec.xml` ships with populated family
    filters; only stage-template files use `<id>instrument family.X</id>`.

  **Spec file (`playbacktemplatespec.xml`) is unchanged from v1.16.4** —
  the entityID format (`instrument family.strings`) stays even though
  it's now known to be vestigial, because:
  1. v1.16.4 already shipped the format internally,
  2. Dorico parses it cleanly,
  3. If a future Dorico version starts honoring the field, the values
     are correct.

### Documentation
- `Resources/dorico/INSTALL-DORICO.md` — "Multi-family routing" section
  rewritten to document the endpoint-config-level enumeration mechanism.
- `Resources/dorico/SMOKE-TEST.md` — TC-2/TC-3 troubleshooting note
  rewritten; references the new endpoint-config schema instead of the
  spec-level filter.

### Risk envelope
- The `<instruments array="true">` block schema is **proven to work** in
  the working Ample China user template (single-vendor case, 11 custom
  user instruments). It is **inferred to extend** to factory instrument
  IDs (`instrument.strings.violin` etc.) from the binary symbol cluster
  and the matching `instrument family.X / instrument.X.Y` ID-space
  convention. The first end-to-end TC-2 pass confirms the inference;
  the user is asked to run the smoke test in Dorico after re-install.
- The Generic endpoint enumerates 345 instrument IDs across 9 families.
  If Dorico has a routing precedence rule we haven't observed (e.g.
  "first matching entry wins" vs "most specific family wins"), Generic
  may shadow Brass/Winds/Strings on overlapping IDs. We've kept the
  4-entry order Strings → Winds → Brass → Generic in the spec to favor
  family-specific entries first. No overlap in the IDs themselves
  (each instrument is in exactly one family).
- Backup at `backups/O-MicrotonalSampler/v1.16.3/` for one-step
  rollback (v1.16.4 was uncommitted; v1.16.5 supersedes it before any
  git tag was placed).

## [1.16.4] - 2026-05-04

> **Diagnostic step, not a complete fix.** v1.16.4 corrected the
> `<instrumentFamilies>` text content from C++ SDK enum names to Dorico's
> textual entityIDs. In-Dorico smoke test (post-deploy) still showed the
> same 4× "Can't find a template spec or endpoint config…" warnings —
> revealing that `<instrumentFamilies>` is a vestigial/editor-only field
> and the real routing filter lives at endpoint-config level. v1.16.5
> ships that real fix. v1.16.4 was never committed or tagged; it's
> documented here for the diagnostic record.

### Fixed (partial — see v1.16.5 for full TC-2 fix)
- **`<instrumentFamilies>` entityID format corrected.** The v1.16.3 spec
  used C++ SDK enum names (`kStrings`, `kWoodwinds`, `kBrass`) which
  Dorico's XML matcher does not resolve. Replaced with the textual entityIDs Dorico actually registers:

  | Was (v1.16.3, broken) | Now (v1.16.4) |
  |---|---|
  | `<instrumentFamilies>kStrings</instrumentFamilies>` | `<instrumentFamilies>instrument family.strings</instrumentFamilies>` |
  | `<instrumentFamilies>kWoodwinds</instrumentFamilies>` | `<instrumentFamilies>instrument family.woodwinds</instrumentFamilies>` |
  | `<instrumentFamilies>kBrass</instrumentFamilies>` | `<instrumentFamilies>instrument family.brass</instrumentFamilies>` |

  Single change, single file (`playbacktemplatespec.xml` lines 12, 19, 26).
  Entry #4 with empty `<instrumentFamilies/>` stays as the Generic fallback.

  **Root cause / evidence:**
  - `/Applications/Dorico 6.app/Contents/Resources/instrumentFamiliesDefinitions.xml`
    registers `<entityID>instrument family.brass</entityID>`,
    `instrument family.strings`, `instrument family.woodwinds`. There is no
    `kStrings` / `kBrass` / `kWoodwinds` ID anywhere in the Dorico install.
  - `/Applications/Dorico 6.app/Contents/Resources/playback/StageTemplates/SmallJazz/stagetemplate.xml`
    references the same family IDs as `<id>instrument family.strings</id>`
    in a different schema (stage layout) — same ID space, confirming the
    textual format is what Dorico parses.
  - The v1.16.3 `application.log` showed clean ingest of the spec and all
    four endpoint configs, then 4× `Can't find a template spec or endpoint
    config for routing this instrument` on apply (one per stave: Solo
    Violin, Flute, Trumpet, Marimba). Consistent with the matcher iterating
    entries 1–3, failing to resolve `kStrings`/`kWoodwinds`/`kBrass` to any
    registered family entity, and either skipping those entries or
    aborting the entries-array walk before reaching entry #4 (which has
    `<instrumentFamilies/>` and should match Marimba as Generic — but did
    not in v1.16.3 testing).

  **No source / build / DSP / UI / state changes.** Pure resource fix in
  `Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml`.
  All four expression maps, the C++ label patch, the keyswitch routing,
  and the dynamics/microtonal paths from v1.16.3 are unchanged and still
  load-bearing.

### Documentation
- `Resources/dorico/SMOKE-TEST.md` — TC-2 and TC-3 troubleshooting notes
  updated to reference the entityID format and point at
  `instrumentFamiliesDefinitions.xml` for the canonical list.
- `Resources/dorico/INSTALL-DORICO.md` — "Multi-family routing" table and
  "Wrong family routing" troubleshooting entry updated to use
  `instrument family.strings` etc. instead of `kStrings`.
- `CHANGELOG.md` v1.16.3 "Risk envelope" annotated with what was actually
  validated post-ship vs what failed.

### Risk envelope
The entityID format `instrument family.strings` is proven to exist in
Dorico's family-definition layer and proven to be referenced from at least
one other schema (`stagetemplate.xml`). It is **unproven** as a filter
value inside `<entry><instrumentFamilies>` in `playbacktemplatespec.xml`
specifically — no factory `playbacktemplatespec.xml` ships with populated
family filters; the closest analog is the working "Ample China" user
template, which has empty `<instrumentFamilies/>` on every entry.

If TC-2 still fails with "Can't find a template spec or endpoint config…"
warnings after this patch, the next escalation paths are:
  1. **Array-of-children variant.** Replace each
     `<instrumentFamilies>instrument family.X</instrumentFamilies>` with
     `<instrumentFamilies array="true"><entityID>instrument family.X</entityID></instrumentFamilies>`
     (matching the Steinberg pattern used by the surrounding `<entries
     array="true">` wrapper).
  2. **Single-entry fallback.** Collapse to one entry binding the Strings
     endpoint config (no family filter); accept that all staves get the
     Strings exp-map by default and document manual binding via
     Library → Expression Maps for Brass / Winds / Generic.

Backup at `backups/O-MicrotonalSampler/v1.16.3/` for one-step rollback.

## [1.16.3] - 2026-05-04

### Added
- **Multi-family Dorico routing.** The Playback Template now ships four
  expression maps (Strings, Winds, Brass, Generic) and four endpoint-config
  folders. Dorico routes each stave to the family-correct exp-map
  automatically based on the stave's instrument family — no manual exp-map
  swap per stave. Brief: `improvements/v1.16.3-dorico-cleanup.md`.

  - **Winds map** (`xmap.ouaricon.o_microtonalsampler_winds`): ord, flutter
    (`pt.flutterTongue`), breathy (`pt.whisper` — closest catalog match for
    the absent `pt.aeolian`), multi (`pt.multiphonic`), keyclick
    (`pt.keyClick`), slap (`pt.slapTongue`), harm (`pt.naturalHarmonic1`),
    stacc (`pt.staccato`).
  - **Brass map** (`xmap.ouaricon.o_microtonalsampler_brass`): ord, mute
    (`pt.muted`), cuivre (`pt.cuivre`), flutter (`pt.flutterTongue`),
    halfvalve (unbound — Dorico has no canonical `pt.halfValve`), stopped
    (`pt.stopped`), growl (`pt.growl` — corrected from `pt.growling`), fall
    (`pt.fallDrop` — corrected from `pt.fall`).
  - **Generic map** (`xmap.ouaricon.o_microtonalsampler_generic`): only slot 0
    (ord) bound; slots 1..7 ship unbound for user customization. Catches
    percussion, voice, keyboard, and any family not explicitly routed.
  - All four maps share the same 8-slot keyswitch shape (MIDI 0..7), the
    same `kVST3NoteExpression` microtonal routing, the same `pitchBendRange=2`,
    and the same `kCC param1=11` dynamics path. Plugin's `kMaxTech=8` cap
    is unchanged.

### Fixed
- **TC-2: Playback Template auto-loads the plugin without "Can't find a
  template spec" warning.** The Playback Template's `<entries>` now contain
  per-family `<endpointConfig>` references with `<instrumentFamilies>`
  filters (`kStrings`, `kWoodwinds`, `kBrass`) plus a fallback entry with
  no filter routing to the Generic endpoint config. Each entry binds to a
  separate user-folder endpoint config (`O-MicrotonalSampler`,
  `-Winds`, `-Brass`, `-Generic`), each containing one `slotData` with
  the family-correct `<expressionMapID>`. Replaces the v1.16.2 single-entry
  template that left every non-Strings stave warning-flagged.

  Manual-load workaround from v1.16.2's INSTALL-DORICO.md is no longer
  required.

### Changed
- **Strings exp-map slot remap (8 combos, was 10).** The Strings map drops
  three articulations and reshuffles the kept slots:

  | Slot | v1.16.2 (10-combo) | v1.16.3 (8-combo) | Dorico ID |
  |------|--------------------|-------------------|-----------|
  | 0 | ord | ord | `pt.natural` |
  | 1 | sp  | sp  | `pt.sulPonticello` |
  | 2 | st  | st  | `pt.sulTasto` |
  | 3 | sv  | **stacc** | `pt.staccato` *(was `pt.nonVibrato`)* |
  | 4 | cs  | cs  | `pt.muted` |
  | 5 | pizz | pizz | `pt.pizzicato` |
  | 6 | harm | harm | `pt.naturalHarmonic1` |
  | 7 | mart | **trem** | `pt.tremolo` *(was `pt.martele`)* |
  | 8 | trem | (dropped — moved to slot 7) | — |
  | 9 | flaut | (dropped) | — |

  Notations dropped from keyswitch firing: `Senza vib.` (sv), `Mart.`, and
  `Flaut.`. Existing scores using these markings will see Dorico hold the
  previously active slot (no audible change at the marking; documented in
  INSTALL-DORICO.md).

  Doricolib `<version>` bumped 7 → 8 to defeat Dorico's parsed-XML cache.

- **Default technique-tab labels (`PluginProcessor.cpp`).**
  `resetTechniqueNames()` now produces `ord, sp, st, stacc, cs, pizz, harm, trem`
  (was `ord, sp, st, sv, cs, pizz, harm, mart`). Pure cosmetic label change
  in the default array — slot count, RR buffer, threading, APVTS state shape,
  and `kMaxTech` cap are all unchanged. Existing user presets keep their
  saved labels (defaults only fire on `Reset Techniques` button or fresh
  plugin load).

### Documentation
- `Resources/dorico/INSTALL-DORICO.md` rewritten for the 4-folder
  installation layout and per-family routing table. New troubleshooting
  entry for "wrong family routing" symptoms.
- `Resources/dorico/SMOKE-TEST.md` adds TC-2 (family-aware endpoint
  loading), TC-5b/c/d (Winds / Brass / Generic keyswitches), TC-7
  (dropped-articulation regression check). Existing TC-4 (microtonal
  pitch) extends to test all four staves.

### Risk envelope
The `<instrumentFamilies>` enum values (`kStrings`, `kWoodwinds`, `kBrass`)
are inferred from Dorico SDK conventions; no concrete factory or user
template in the local Dorico install populates this tag with values.
If TC-2 fails (Dorico ignores the family filter and routes everything to
the first-listed entry, OR can't find a template spec at all), the
expected fix is either:
  1. Replace each `<instrumentFamilies>kFamily</instrumentFamilies>` text
     content with a child element (e.g. `<instrumentFamily>kFamily</instrumentFamily>`),
     or
  2. Drop the family filters entirely and accept that all staves bind the
     Strings map (matching v1.16.2 behavior + 3 unused exp-maps available
     for manual binding).

The four exp-maps and the C++ label patch are independently load-bearing
and tested.

#### Post-ship validation (annotated 2026-05-04, see v1.16.4)
- **TC-1 (template appears in dropdown):** PASS. Spec + 4 endpoint configs
  + doricolib all ingested cleanly. `application.log` showed clean
  `Loading PlaybackTemplateSpec`, `Loading Extra Library`, and 4×
  `Loading Endpoint Config` lines.
- **TC-2 (apply auto-loads plugin per family):** FAIL. Apply emitted 4×
  `Can't find a template spec or endpoint config for routing this
  instrument` warnings (Solo Violin, Flute, Trumpet, Marimba) and CLEARED
  any previously-loaded plugins on those staves with no replacement.
- **TC-3 (per-family exp-map binding):** FAIL (gated on TC-2).
- **TC-4 (microtonal pitch via VST3 Note Expression):** Validated only on
  manually-loaded instances — exp-maps themselves are correct.
- **TC-5a/b/c/d (per-family keyswitch firing):** FAIL (gated on TC-2).
- **TC-6 (CC11 dynamics):** Validated only on manually-loaded instances.
- **TC-7 (dropped articulations don't fire):** PASS — but only verifiable
  on manually-loaded instances since TC-2 blocked auto-load.

**Diagnosed root cause (post-ship):** the `<instrumentFamilies>` filter
takes Dorico's textual family entityIDs (`instrument family.strings`,
`instrument family.brass`, `instrument family.woodwinds`), not the C++
SDK enum names. Risk-envelope option 1 above was the right direction
but the wrong syntax — fix is text-content replacement, not a child
element. Patched in v1.16.4.

## [1.16.2] - 2026-05-04

### Fixed
- **TC-5: Dorico keyswitch-from-notation routing now fires.** Typing
  "sul pont." text in Dorico switches the plugin's WebView technique-tab
  strip to slot 1; "Ord." returns to slot 0. Pizz., Sul tasto, Senza vib.,
  Con sord., Harm., Mart. follow the same pattern. Two contributing
  defects, both load-bearing; either alone left TC-5 broken.

  1. **Plugin source: `ks_enabled` defaulted to `false`.** Fresh plugin
     instances created by Dorico's Playback Template / Endpoint Setup
     booted with the keyswitch trigger gate disabled, so even correctly-
     routed KS notes from the exp-map were ignored. Default flipped to
     `true` (`PluginProcessor.cpp:149`). KS range stays `0..9` (MIDI
     C-2..A-2 in the Dorico C3=60 convention) — well below any real
     instrument's pitch range, so the new default cannot accidentally
     fire from normal MIDI input.
  2. **Plugin source: `technique_count` defaulted to `1`.** With the
     count at 1, the `processBlock` KS handler clamps every incoming KS
     note to `juce::jmin(7, techCount-1)` = 0 — every KS routed to slot 0
     regardless of which technique Dorico fired. Default raised to `8`
     (`PluginProcessor.cpp:139`). All 8 plugin slots are now reachable
     by exp-map KS notes 0..7. The technique-tab strip now displays
     8 tabs by default; users wanting a slimmer UI can still reduce
     `technique_count` per-instance.
  3. **Distribution artifact: per-combo `<exclusionGroup>1` added.**
     `playbacktemplatedeps.doricolib` per-`<playingTechniqueCombination>`
     now carries `<exclusionGroup>1</exclusionGroup>` (matching HSO
     factory exp-maps). Required for Dorico's mutual-exclusion logic
     to fire `<switchOnAction>` on technique transitions out of `Ord.`
     when other techniques are mutually exclusive. `<version>` bumped
     4 → 7 (intermediate v5/v6 were transient diagnostic shapes — see
     "Schema iteration" below).

  Both the saved ks_enabled/technique_count values from existing
  v1.16.0 / v1.16.1 sessions are preserved on project reload (Dorico
  restores per-instance state). To pick up the new defaults, users must
  delete + re-add the O-MicrotonalSampler endpoint via Play → Endpoint
  Setup, or apply the Playback Template again. Documented in the
  install guide under "Upgrading from v1.16.x".

### Schema iteration (debug history, recorded for the next maintainer)

- **v5 (transient):** Mirrored HSO Cello Solo's full per-combo shape —
  added `<exclusionGroup>1</exclusionGroup>`, `<pitchBendRange>2</pitchBendRange>`,
  `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  per combo. Side effect: also dropped the **top-level** `<pitchBendRange>`
  and `<microtonalPlaybackMethod>` (HSO doesn't have them at top level
  — but HSO is 12-TET orchestral, doesn't need NE for microtones). TC-5
  remained broken (the real cause was plugin-side, not schema). TC-4
  (microtonal pitch via VST3 NE) regressed because per-combo
  `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  did not preserve the routing — Dorico fell back to no NE.
- **v6 (transient):** Plugin defaults flipped (ks_enabled=true,
  technique_count=8) — TC-5 fired. Per-combo `<pitchBendRange>` and
  `<microtonalPlaybackMethod>` removed, but top-level fields were still
  missing from the v5 rewrite, so TC-4 stayed broken.
- **v7 (shipped):** Restored top-level `<pitchBendRange>2</pitchBendRange>`
  and `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  to match the v1.16.1 baseline. Kept per-combo `<exclusionGroup>1</exclusionGroup>`
  (the only schema-shape change that's retained from v5). Both TC-4 and
  TC-5 pass.

  Net schema delta from v1.16.1 → v1.16.2: per-combo `<exclusionGroup>1</exclusionGroup>`
  added; per-combo `<monophonic>`, `<applyMillisecondsBeforeToEndOffsets>`,
  `<applyMillisecondsBeforeToControllers>` removed (HSO factory does
  not ship these — they appear to be from a different Dorico schema
  version and may have been silently ignored or rejected).

### Changed
- **`CMakeLists.txt`** — bump `VERSION` 1.16.1 → 1.16.2.
- **`PluginProcessor.cpp`** — `ks_enabled` default `false` → `true`;
  `technique_count` default `1` → `8`.
- **`Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`**
  — per-combo `<exclusionGroup>1</exclusionGroup>` added to all 10
  `<playingTechniqueCombination>` entries; `<version>` 4 → 7. Top-level
  `<pitchBendRange>2</pitchBendRange>` and
  `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
  preserved unchanged from v1.16.1 (NE routing for TC-4).

### Validation

- pluginval (strictness 5) — PASS.
- auval — same pre-existing benign DEF-24-01 finding as prior versions
  (note-expression module's static-check artifact; not a runtime defect).
- Manual smoke in Dorico 6:
  - TC-1 Playback Template appears in dropdown — PASS.
  - TC-3 Expression map binds in Track Inspector — PASS.
  - TC-4 Microtonal pitch via VST3 NE — PASS (load-bearing; was the
    primary regression risk during the v5/v6 iteration).
  - TC-5 Playing-technique text fires keyswitches — PASS (primary fix
    target). "sul pont." → tab `sp`; "Ord." → tab `ord`.

### Known issues (carried from v1.16.1)

- **TC-2: Apply Playback Template doesn't auto-load the plugin slot.**
  Workaround unchanged — manually load O-MicrotonalSampler in the Mixer.
  Tracked as bonus follow-up; deferred from v1.16.2 scope.
- **TC-6: CC11 dynamics behavior** never tested end-to-end through
  Dorico in v1.16.x. CC11 wire is in the exp-map; plugin handler validated
  against direct MIDI in non-Dorico DAWs.
- **8-slot cap vs 10-technique exp-map.** The exp-map ships 10
  `<playingTechniqueCombination>` entries (KS notes 0..9), but the
  plugin caps internally at 8 slots. KS notes 8 (tremolo) and 9
  (flautando) clamp to slot 7 (martele) at the plugin layer. Two
  resolutions are open for a future patch: (a) trim the exp-map to 8
  combinations to match plugin capacity, (b) raise plugin
  `kMaxTech` to 10. Lower priority — none of the user's primary
  techniques fall in slots 8–9.

### Files touched

1. `CMakeLists.txt` — `VERSION` 1.16.1 → 1.16.2.
2. `CHANGELOG.md` — this entry.
3. `Source/PluginProcessor.cpp` — two parameter defaults flipped (lines
   139 and 149).
4. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`
   — per-combo `<exclusionGroup>1</exclusionGroup>`; `<version>` 4 → 7.
5. `.planning/STATUS.md` — v1.16.2 patch noted; primary follow-up
   (TC-5) closed; bonus follow-ups (TC-2, TC-6, 8-slot cap) carried.

---

## [1.16.1] - 2026-05-04

### Fixed
- **Dorico launch crash** ("Error opening file: invalid file format" at
  startup). Root cause: leading XML comments before the root element in
  `endpointconfig.xml` and `playbacktemplatespec.xml` are rejected by
  Dorico's strict user-config parsers. Same comment in
  `playbacktemplatedeps.doricolib` (which uses a different parser path
  inside `EndpointConfigs/`) was tolerated, but the parser used at
  `DefaultLibraryAdditions/` is strict — so the doricolib's leading
  comment also crashed launch when distributed via that path. All three
  files now have no pre-root comments.
- **Expression map not appearing in Track Inspector dropdown** in
  v1.16.0. Root cause: `playbacktemplatedeps.doricolib` inside
  `EndpointConfigs/<Name>/` is endpoint-scoped — its expression-map
  definition only registers when that endpoint is active in the project,
  and Dorico's auto-load template path was failing (separate bug).
  Without an active endpoint, the exp-map was invisible in the Track
  Inspector. v1.16.1 documents the `DefaultLibraryAdditions/` path
  (Dorico auto-merges every `.doricolib` placed there into every project's
  library on launch) as the canonical mechanism for global exp-map
  registration. End-users now copy three artifacts (not two) — the two
  folders plus a single `.doricolib` to `DefaultLibraryAdditions/`.

### Changed
- **`Resources/dorico/INSTALL-DORICO.md`** — rewritten. Documents the
  three-folder layout (now four, including `DefaultLibraryAdditions/`),
  the macOS + Windows install paths, the dev-vs-release CID caveat (kept
  from v1.16.0), known-issue scope for v1.16.1, and a troubleshooting
  section covering launch crashes, dropdown-no-show, and stale-cache
  recovery.
- **`<version>` in `playbacktemplatedeps.doricolib` bumped 1 → 4** to
  defeat Dorico's caching when the file is updated in place. Subsequent
  patches should bump this further on every doricolib content change.
- **`CMakeLists.txt`** — bump `VERSION` 1.16.0 → 1.16.1.

### Known issues (deferred to v1.16.x patches)

- **Apply Playback Template doesn't auto-load the plugin slot.** Dorico
  log: `Can't find a template spec or endpoint config for routing this
  instrument`. The `<entries>` in `playbacktemplatespec.xml` use empty
  `<instrumentFamilies/>` and `<instruments/>` — Dorico doesn't treat
  empty as catch-all. Working reference (`EndpointConfigs/Ample China/`)
  has TWO entries: one endpoint + one fallback `<generatorSpec>`. Workaround:
  manually load O-MicrotonalSampler in the Mixer.
- **Typing playing-technique markings (sul pont., pizz., Ord.) does NOT
  fire the keyswitch.** Plugin's WebView technique-tab strip doesn't
  switch on playback. Two attempted fixes during the v1.16.0 smoke test
  (adding `<switchOffActions>` with KS 0 to non-natural slots; adding
  `<exclusionGroup>1</exclusionGroup>` per combination) both regressed
  rather than helped. Reverted to original switchOn-only shape. Root
  cause unclear — possible factors: per-combination fields missing
  (HSO factory has `<pitchBendRange>`, `<microtonalPlaybackMethod>=kAuto`
  per combo); Dorico's MIDI router filtering KS notes; or some
  endpoint-binding requirement we haven't identified. Tracked in
  `improvements/dorico-keyswitch-fix.md` with full diagnostic context
  and prioritized investigation paths. Workaround: send keyswitch MIDI
  notes (C-1..A-1 = MIDI 0..9) directly via a MIDI track or external
  controller.

### Implementation notes

- **No source-code changes** to the plugin binary. v1.16.1 is a
  distribution-artifacts and documentation patch only. Build outputs
  identical to v1.16.0 except for the `<bundleVersion>` field bumped
  via CMake `VERSION`.
- **Smoke procedure (`Resources/dorico/SMOKE-TEST.md`) deferred to
  v1.16.x update** — six TCs from v1.16.0 still apply, but TC-2 (auto-
  load) and TC-5 (KS firing) are documented FAIL pending the next
  patch. TC-1 (template visible), TC-3 (exp-map binds — via
  `DefaultLibraryAdditions/` not auto-template), and TC-4 (microtonal
  pitch — load-bearing) are validated PASS in v1.16.1.

### Files touched

1. `CMakeLists.txt` — `VERSION` 1.16.0 → 1.16.1.
2. `CHANGELOG.md` — this entry.
3. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` — leading comment stripped.
4. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` — leading comment stripped, `<version>` 1 → 4.
5. `Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` — leading comment stripped.
6. `Resources/dorico/INSTALL-DORICO.md` — rewritten for `DefaultLibraryAdditions/` distribution path + known-issue scope.
7. `improvements/dorico-keyswitch-fix.md` (NEW) — diagnostic brief for the v1.16.x KS-firing patch.
8. `.planning/STATUS.md` — v1.16.1 patch noted; KS-firing tracked as open follow-up.

---

## [1.16.0] - 2026-05-03

### Added
- **Dorico distribution bundle** (`Resources/dorico/`). Three files
  authored against Dorico 6's user-library layout that wire
  O-MicrotonalSampler as a one-click Playback Template:
  - `EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` —
    references the dev-build VST3 plugin ID
    (`ABCDEF019182FAEB4F7544764F4D7453`), MIDI channel 1, expression-map
    binding to the bundled exp-map.
  - `EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`
    — the bundled expression map (`xmap.ouaricon.o_microtonalsampler`):
    `microtonalPlaybackMethod = kVST3NoteExpression` (preserves
    microtonal pitch via VST3 NE), `volumeType = kCC` param1=11
    (CC11 Expression for sustained dynamics), and 10
    `playingTechniqueCombinations` mapping Dorico's notation glyphs
    (`pt.natural`, `pt.sulPonticello`, `pt.sulTasto`, `pt.nonVibrato`,
    `pt.muted`, `pt.pizzicato`, `pt.naturalHarmonic1`, `pt.martele`,
    `pt.tremolo`, `pt.flautando`) to the plugin's keyswitch range
    (MIDI notes 0..9, technique slots 0..9, full velocity).
  - `PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml`
    — the user-facing Playback Template
    (`playbacktemplate.user.o_microtonalsampler`) that references the
    EndpointConfig.
- **`Resources/dorico/INSTALL-DORICO.md`.** End-user install guide
  with macOS + Windows path snippets, the dev-vs-release CID caveat
  documented, and a verification checklist.
- **`Resources/dorico/SMOKE-TEST.md`.** Six-step manual smoke procedure
  covering template visibility, endpoint loading, expression-map
  binding, microtonal pitch (P0 — load-bearing), technique keyswitch
  on notation, and CC11 dynamics swell.

### Changed
- `CMakeLists.txt` — bump `VERSION` 1.15.0 → 1.16.0.

### Implementation notes

- **Distribution mechanism finalised.** A spike against the user's
  installed Dorico 6 library confirmed that `.doricolib` Library
  Manager imports register expression-map definitions but **not**
  EndpointConfig or PlaybackTemplate — those entities live in their
  own folder structures (`EndpointConfigs/<Name>/` and
  `PlaybackTemplateSpecs/<Name>/`) at the user-library root. The
  earlier plan's "single `.doricolib` containing exp-map + Playback
  Template + Endpoint Configuration" assumption was structurally
  incorrect; v1.16.0 ships the actual 3-folder layout Dorico itself
  uses for user-saved templates. This unblocks the previously
  reverted Phase 25 Plan 01 distribution mechanism (commit `d2c86c5`
  rollback in the parent `note-expression` module).
- **Schema validated against factory references.** Action XML
  confirmed against `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Symphonic Orchestra/expressionMapsDefinitions.xml`
  (`<switchOnAction><type>kKeySwitch</type><param1>...</param1><param2>127</param2></switchOnAction>`)
  and `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/Iconica Sketch/expressionMapsDefinitions.xml`
  (`<volumeType><type>kCC</type><param1>11</param1></volumeType>` —
  the literal string is `kCC` with the CC# in `param1`, NOT `kCC11`
  as a type name). EndpointConfig + Spec structure modelled on the
  user's existing "Test State-less" reference pair.
- **Parent NE map inlined, not chained.** The expression map's
  `<parentEntityID>` is intentionally empty (rather than referencing
  `xmap.ouaricon.vst3_note_expression`) so a single template install
  wires everything — no separate `.doricolib` import required for
  the parent module. The cost is ~10 KB of duplicated XML; the
  benefit is one-step install for end-users.
- **No source-code changes.** v1.16.0 is a distribution-artifacts-only
  release — pure XML + docs under `Resources/dorico/`. The plugin
  binary is unchanged from v1.15.0. Build / pluginval / auval status
  inherits from the v1.15.0 baseline.
- **Dev-build CID hardcoded.** The `<pluginID>` in
  `endpointconfig.xml` matches the dev-branded build (manufacturer
  `OuDv`, suffix `-dev`). Release builds (manufacturer `OuAu`, no
  suffix) produce a different CID; release CI will need a parallel
  artifact tree, tracked as a v1.16.x patch series. Documented in
  `INSTALL-DORICO.md` § "Caveat: dev vs release builds".

### Test surface

- Manual smoke procedure: `Resources/dorico/SMOKE-TEST.md` (six TCs
  covering template discovery, endpoint loading, expression-map
  binding, microtonal pitch routing, technique keyswitch on
  notation, and CC11 dynamics).
- No new automated test executables — distribution artifacts cannot
  be exercised without a Dorico session.

### Files touched

1. `CMakeLists.txt`
2. `CHANGELOG.md`
3. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` (NEW)
4. `Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` (NEW)
5. `Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` (NEW)
6. `Resources/dorico/INSTALL-DORICO.md` (NEW)
7. `Resources/dorico/SMOKE-TEST.md` (NEW)
8. `.planning/STATUS.md` — v1.16.0 marked implemented; Multi-Version Plan complete.

---

## [1.15.0] - 2026-05-03

### Added
- **CC + Program Change technique triggers.** Two new MIDI trigger
  mechanisms join keyswitches: a configurable Continuous Controller
  (CC#, default 32) routes its 0..127 byte through an 8-slot
  value-range table to a target technique slot, and Program Change
  events route through an 8-slot PC#-to-technique table. Both share
  one technique cursor with KS — an 8-band cursor that all DAWs can
  drive without specialised expression-map authoring. Disabled by
  default for back-compat (v1.14.0 sessions migrate untouched).
- **KS > CC > PC > history precedence.** When multiple triggers fire
  in the same audio block (e.g. an automation lane bumps both a
  keyswitch note and a CC simultaneously) the highest-precedence
  candidate wins. History ("last technique used") persists across
  blocks if no trigger fired — eliminates the "phantom default reset"
  failure mode some sample players exhibit when the last MIDI event
  was an unrelated CC.
- **Three new APVTS parameters:** `cc_select_enabled` (bool, default
  off), `cc_number` (0..119, default 32 — General Purpose 1; not the
  CC1 modulation, CC11 expression, or bank-select reserved numbers),
  `pc_enabled` (bool, default off). All round-trip through project
  state and host automation.
- **CC + PC mapping tables persist with project state.** New
  `<CcMapping>` and `<PcMapping>` ValueTree children with sparse 8-slot
  child lists; v1.14.0 sessions decode cleanly back to the seeded
  defaults (CC equally splits 0..127 across the active
  `technique_count`; PC#i → tech i).
- **Trigger configuration panel in the WebView UI.** Collapsible
  `<details>` disclosure under the technique-bar with two sub-panels
  (CC + PC), each showing an 8-row editable table. Slot rows beyond
  the active technique count are dimmed but remain editable so users
  can pre-stage values before growing their library. "Reset to
  defaults" button restores the seeded mapping. Hidden entirely when
  `technique_count == 1` (matches the technique-bar back-compat
  contract — single-technique libraries see no v1.15.0 chrome).
- **`docs/dynamics-mapping.md`.** New doc explains how the plugin
  routes dynamics: note-on velocity selects the sample layer (locked
  at note-on, no continuous modulation); CC11 ("Expression") drives
  smoothed post-mix gain throughout sustain. Recommends
  `<volumeType><type>kCC11</type></volumeType>` as the Dorico
  expression-map default for sustained dynamic shaping, with
  `kNoteVelocity` as an alternative for short / articulated passages.
  Forward-compat note on Dorico 3+'s secondary-volume-control slot.
- **Two new EXCLUDE_FROM_ALL test executables:**
  `O-MicrotonalSampler_CcPcTriggerCheck` (40+ assertions covering
  defaultCcMapping / defaultPcMapping bucketing, value-range routing,
  PC routing, the KS>CC>PC>history precedence resolver, and an
  end-to-end three-block scenario) and
  `O-MicrotonalSampler_DynamicsLayerCheck` (pins
  velocity-to-layer-index bucketing for N=1/2/4/8 layers — the
  formula `MicrotonalSamplerVoice::startNote` uses, the same one
  documented in `docs/dynamics-mapping.md`).

### Changed
- **`processBlock` MIDI scan reorganised.** The KS-only walk from
  v1.14.0 expanded to a single-pass scan that harvests KS / CC / PC
  candidates simultaneously, then resolves precedence once at block
  end via `OMtsTrigger::resolveTriggerPrecedence`. RT-safety
  preserved — CC + PC tables are read via `std::atomic_load` on
  shared_ptr (the same COW pattern `currentSampleMap` uses), the
  filter buffer is still pre-allocated in `prepareToPlay`, and the
  scan walks the host's MidiBuffer exactly once instead of twice
  when CC + KS are both active.
- **No-trigger-this-block path is a no-op.** When neither KS, CC,
  nor PC fired, `pendingTechniqueIndex` is left untouched —
  v1.14.0's "store same value over and over" behaviour is replaced
  with a guarded compare-then-store so the AsyncUpdater isn't
  triggered for null events.

### Architecture
- **`Source/TriggerMapping.h`** — pure-data header containing the
  `CcSlot` / `PcSlot` structs, default-builder helpers, and the
  resolver free functions. Lives outside the audio processor so
  the unit-test executable can consume the routing logic without
  pulling in `JuceHeader.h` or instantiating an `AudioProcessor`.
  Audio thread + message thread + tests share identical resolution
  code by construction.

### Migration / back-compat
- v1.14.0 sessions decode cleanly: missing `<CcMapping>` /
  `<PcMapping>` children → constructor's seeded defaults survive.
- `cc_select_enabled` and `pc_enabled` default to false so old
  sessions hear no behavioral change.
- Single-technique libraries (technique_count=1) see zero new UI
  chrome — the trigger panel is hidden alongside the existing
  technique-bar.

## [1.14.0] - 2026-05-03

### Added
- **Playing Techniques (engine + Keyswitches + UI core).** Sample cells now
  carry a third axis — `technique` — alongside `(midiNote, velocityLayer)`.
  Each `SampleMap` can hold up to 8 technique slots (default vocabulary:
  `ord`, `sp`, `st`, `sv`, `cs`, `pizz`, `harm`, `mart`). Filenames carrying
  any recognised token (delimited, case-insensitive — exact match, never
  substring) auto-route to their slot at folder-load time. Each slot
  recognises both the two-letter shorthand and a wider set of orchestral
  long-forms — e.g. slot 1 (sul ponticello) accepts `sp`, `sulpont`,
  `sulponticello`, AND the canonical `sul_pont` / `sul_ponticello` two-token
  forms produced by orchestral-library naming conventions. Same for
  `sul_tasto` (slot 2), `senza_vib` / `non_vib` / `non_vibrato` (slot 3),
  `con_sord` / `con_sordino` / `muted` (slot 4), `pizzicato` (slot 5),
  `harmonic` / `harmonics` (slot 6), `martele` / `martellato` (slot 7),
  `tremolo` (slot 8), `flautando` / `flautato` (slot 9). Library leads
  (`sul`, `senza`, `non`, `con`) are NEVER accepted standalone — they
  require their canonical suffix to avoid over-matching. Two recordings
  of the same `(midi, velocity)` with different technique tokens (e.g.
  `C3_v1_ord.wav` + `C3_v1_sp.wav`) now coexist in two distinct cells
  instead of triggering the round-robin ambiguity modal.
- **Five new APVTS parameters:** `technique_count` (1–8), `technique_select`
  (0–7), `ks_enabled` (bool), `ks_low_note` (0–127), `ks_high_note`
  (0–127). All round-trip through project state and host automation.
- **Keyswitch routing in `processBlock`.** When `ks_enabled` is on, MIDI
  note-ons inside `[ks_low_note..ks_high_note]` are absorbed (never reach
  the synth) and store their semitone offset from `ks_low_note` into the
  active-technique atomic. Matching note-offs are likewise absorbed so
  KS notes never trigger spurious voices. A pre-allocated `juce::MidiBuffer`
  carries the filtered stream into `Synthesiser::renderNextBlock`,
  preserving real-time safety (`MidiBuffer::ensureSize` runs on the
  message thread in `prepareToPlay`).
- **Voice-side technique resolution.** `MicrotonalSamplerVoice::startNote`
  loads the technique cursor with `memory_order_acquire` and pairs it with
  the sample-map snapshot to resolve the `(midi, vel, tech)` triplet via
  the new `SampleMap::findCell(midi, vel, tech)` overload. The triplet
  lookup falls back to `tech=0` ("ord") when the requested slot is empty,
  so partially-populated technique sets still play. Crossfade pair MUST
  share technique (no cross-articulation morphing).
- **Per-cell round-robin counter expanded** from 512 to 4096 entries
  (`128 × 4 × 8`). Counters are independent per technique slot — a flip
  from `ord` to `sp` mid-session no longer disturbs the `ord` slot's RR
  cursor.
- **WebView UI: technique tab strip** above the sample-map grid. Tabs are
  click-to-select (left-click) / right-click-to-rename. `+` / `−` buttons
  grow / shrink `technique_count`. Inline KS panel — toggle + low/high
  number inputs — wires through `setKeyswitchEnabled` /
  `setKeyswitchRange` native functions. Hidden by default; only surfaces
  once the user has expanded beyond a single technique slot or enabled
  KS, preserving the v1.13.0 visual contract for legacy sessions.
- **Six new WebView native functions:** `getTechniqueState`,
  `setActiveTechnique`, `setTechniqueName`, `resetTechniqueNames`,
  `addTechniqueSlot`, `removeTechniqueSlot`, plus `setKeyswitchEnabled`
  / `setKeyswitchRange`. Existing `loadSingleSampleDialog`,
  `overrideLoopPoints`, `resetLoopToAutoDetect`, and `getWaveformPeaks`
  gained an optional trailing `technique` arg (defaults to the current
  active-technique cursor — UI clicks already route correctly).
- **State persistence:** `<TechniqueNames><Slot index name/></TechniqueNames>`
  child added to the captured state ValueTree. Sparse — only renamed
  slots are emitted; the curated default vocab covers absent slots on
  restore. v1.13.0 sessions decode unchanged (no `<TechniqueNames>`
  child → default vocab survives, `technique_count=1`, `ks_enabled=false`).
- **Three new regression tests** (EXCLUDE_FROM_ALL):
  `O-MicrotonalSampler_TechniqueParseCheck` (24 cases — token recognition,
  substring rejection, case insensitivity, coexistence with
  note/velocity/RR), `O-MicrotonalSampler_FindCellTripletCheck` (8 cases
  — exact match, fallback, disjoint techniques, closest-note within slot,
  back-compat overload, `applyMergeRrCell` triplet keying),
  `O-MicrotonalSampler_StateMigrationCheck` (5 cases — empty tree =
  default vocab, sparse rename leaves untouched slots, `SampleCell`
  default-init, v1.13.0-shape merge identical to v1.13.0,
  back-compat `findCell` two-arg overload).

### Changed
- **`SampleCell` gained `int technique = 0`.** All callers default to 0
  in v1.13.0-shape sessions. `findCell` two-arg overload is preserved for
  back-compat and routes to `tech=0`.
- **`LoadOptions` / `LoadOp` gained `targetTechnique` + `overrideTechnique`.**
  These thread through `SampleLoader::loadFolder`'s 3D grouping pass; a
  user-driven "assign folder to technique" override is the v1.15.0 modal
  surface.
- **`AmbiguousDuplicate` payload includes `technique`.** WebView's RR
  confirmation modal now sees the slot a duplicate group lives in (only
  matters when one technique slot has duplicates — different techniques
  no longer collide).
- **`FilenameParser.h` switched from `<JuceHeader.h>` to specific
  `juce_core` include.** Matches the `SampleMap.h` pattern so standalone
  test executables (the new triplet/parser/migration checks) compile
  without going through `juce_add_plugin`.

### Compatibility
- **MINOR bump (1.13.0 → 1.14.0).** Backward-compatible. v1.13.0 presets
  load unchanged: `technique_count` defaults to 1, `ks_enabled` defaults
  to false, every cell defaults to `technique=0` ("ord"), and the
  technique tab strip stays hidden until the user opts in. Audio output
  for single-technique libraries is bit-identical to v1.13.0
  (render-harness identity verified by the test surface). VST3 / AU IDs
  unchanged.

### Testing
- Build green: VST3 + AU + Standalone, macOS arm64, Release.
- pluginval level 5: PASS.
- auval AU: PASS.
- Regression tests: 3 new test executables, all assertions PASS.

## [1.13.0] - 2026-05-02

### Changed
- **ARCH-02: extracted WKWebView drag-drop content-streaming pattern to
  shared module `modules/core/webview-drop-streaming` (v1.0.0).** The 4
  `dropSession*` native function handlers, session-scoped temp-dir
  lifecycle, 5-min stale-session reaper, `DropSessionGuard` validators
  (path traversal, parent-chain symlink, 256 MB-per-file / 4 GB-per-session
  caps), and the JS-side streaming helpers (`bindWebViewFileDrop`,
  `streamFolderEntryToCpp`, `streamSingleFileEntryToCpp`,
  `readFileEntryAsBase64`, `arrayBufferToBase64`) now live in the module.
  This editor instantiates one `Ouaricon::WebViewDropStreaming::SessionManager`
  with two commit callbacks (forwarding to `processorRef.loadSampleFolder`
  / `loadSingleSample`) and splices the module's native functions into
  `buildNativeFunctionRegistry()`. JS imports `bindWebViewFileDrop` from
  `./modules/webview-drop-streaming.js` and passes a config object with
  the plugin-specific glue (selectors, modal/toast/hover callbacks, cell
  midi/vel extractor). Behaviour is unchanged — every code path
  (single-file drop, folder drop with options modal, fast-path for hosts
  exposing absolute paths, no-path-no-entry diagnostic) is preserved
  verbatim. Per-plugin `tempDirPrefix` (`"o-microtonalsampler-drop-"`)
  isolates the stale-session reaper so future module adopters don't
  collide.

### Removed
- `Source/DropSessionGuard.h` — promoted to the shared module
  (`modules/core/webview-drop-streaming/cpp/DropSessionGuard.h`).
- `cleanupStaleDropSessions()` editor method — the reaper now runs
  inside `SessionManager` scoped to the per-plugin temp-dir prefix.
- ~290 lines of inline `dropSessionStart` / `dropSessionAddFile` /
  `dropSessionCommitFolder` / `dropSessionCommitFile` lambdas from
  `buildNativeFunctionRegistry()`.
- ~470 lines of inline streaming helpers from `sampler-app.js`
  (`bindWebViewFileDrop`, the 4 streaming functions, `extractDroppedFilePaths`,
  `pointInClientRect`, `collectAudioFilesFromDir`, `newDropSessionId`,
  `AUDIO_EXTENSIONS_RE`).

### Code metrics
- `Source/PluginEditor.cpp`: ~290 lines of native-fn handlers + 16 lines
  of `cleanupStaleDropSessions()` removed; ~30 lines of SessionManager
  construction + splice loop added.
- `Resources/ui/js/sampler-app.js`: ~470 lines of inline drag-drop code
  removed; ~25 lines of parameterized `bindWebViewFileDrop({...})` call
  added.
- New shared module: ~1100 lines (C++ header-only + JS ES module +
  README + module.yaml) reusable across O-TextureForge, O-Bells,
  O-Lyrica, future plugins.

### Notes
- O-TextureForge and other plugins that re-implement this pattern are
  **deferred to follow-up improvements** — each will get its own
  regression-tested PATCH bump after migration.
- No behavioural change; no parameter or state-format changes; presets
  and DAW sessions load unchanged.
- Manual DAW drag-drop test required after install — no automated
  regression suite covers the WKWebView surface.
- `Source/tests/drop_session_guard_check.cpp` retains the v1.11.2
  security regression coverage; its include path now points at the
  module's `cpp/`.

### Reference
- REVIEW-architecture.md §"Extract drag-drop streaming → shared module"
  (lines 100-121, 446-449); SUMMARY.md architecture wins #2.

## [1.12.4] - 2026-05-02

### Changed
- **ARCH-01: data-driven native function registry.** Replaced 44 inline
  `.withNativeFunction(...)` chained calls in the editor constructor
  (~1400 lines of organic v1.5.0–v1.12.0 growth) with a single registry
  vector iterated in a `for (auto& [name, handler] : ...)` loop. Each
  native function moves from a chained builder argument to one entry in
  `buildNativeFunctionRegistry()` (new private method). Constructor
  shrinks from ~1500 lines to ~50; the WebView is built via an immediately
  invoked lambda that returns the fully populated `Options`. Behaviour is
  unchanged — every entry preserves its original name, capture list, and
  body verbatim. Pattern is reusable in O-Bells / O-Lyrica which carry
  similar editor boilerplate (architecture-review §1, HIGH ROI).

### Removed
- **Dead code: `Resources/ui/css/tuning-panel-readonly.css`.** Embedded
  as a binary resource since v1.0.0 but no longer applied since the
  v1.2.0 read-only-tuning-panel rewrite. Removed the file, its
  `juce_add_binary_data` SOURCES entry, and the corresponding
  `getResource` URL handler. Frees one binary blob from the plugin's
  embedded resources (~3 KB) and eliminates a stale referent in the
  resource provider.

### Code metrics
- `Source/PluginEditor.cpp` constructor body shrinks from ~1500 → ~50
  lines (97% reduction in constructor size).
- `Source/PluginEditor.cpp` total file delta: +61 lines (1930 → 1991)
  — the 44 lambda bodies move to the new registry method along with
  surrounding function-decl wrapping; net file growth is small because
  only the per-entry brackets/commas change vs the original
  per-call `.withNativeFunction(...)` wrapping.

### Validation
- Smoke-tested in Logic Pro and Reaper: load folder, drag-drop folder,
  drag-drop single file, tuning panel (all panels), preset save/load,
  sample-map clear, MTS-ESP routing.
- All 44 native function entries verified against the v1.12.3 backup
  (name + arity + capture list + body bytes match).

### Migration notes
None — pure structural refactor. APVTS, state format, parameter IDs,
preset format, and JS-bridge contract are all unchanged.

## [1.12.3] - 2026-05-02

### Fixed
- **HG-01: replay-queue corruption from cascaded callbacks during state
  restore.** Two paths could re-enter the queue dispatcher with stale
  expectations: (1) a synchronous `applyFolderLoad` →
  `sampleMapChangedCallback` → editor → public `loadSampleFolder` chain
  could land back inside the still-running outer `kickNextReplayOp`,
  popping ops out from under its iterator; (2) a chain continuation
  staged on an ambiguous-duplicate confirmation could fire long after
  an unrelated state restore or `clearSampleMap` had wiped/rebuilt
  `pendingReplayOps`, dispatching the previous generation's op against
  the new queue. `kickNextReplayOp` now carries a single-threaded
  re-entry guard that rejects synchronous re-entry, and every external
  mutation of `pendingReplayOps` (`setStateInformation`,
  `clearSampleMap`) bumps an atomic `replayQueueGeneration` token.
  Loader callbacks and the `pendingDuplicateChainContinuation` lambda
  capture the generation at staging time and bail out on mismatch.
- **HG-02: filename parser silently dropped RR semantics for
  separator-tokenised conventions.** Filenames following the common
  DAW-export pattern `Piano_C3_take_1.wav` (or `_rr_2`, `_tk_3`)
  tokenise to `["Piano","C3","take","1"]`; the v1.8.0 RR scan only
  matched the glued form `take1` and silently returned `rr=-1` here,
  defeating round-robin for these files. The parser now also detects a
  bare `rr`/`take`/`tk` token whose immediately following token is a
  1–2 digit integer in 1..64, and treats the pair as the RR index.
  Glued form (`take7`) still wins when present so existing libraries
  are unaffected. Tokeniser-agnostic across `_`, `-`, `.`, and space
  separators. Added unit-test coverage for `Piano_C3_take_1.wav`,
  `Trumpet_F#3_rr_2.aif`, `Bowed_E2_tk_3.flac`, dash/space variants,
  out-of-range and bare-prefix rejection (`take_99`, `taken_1`).
- **HG-04: `static_assert` enforces the `kMaxVariantsPerCell` ↔
  `0xFF`-sentinel invariant.** `selectVariantIndex` clips its uint8
  RR counter to 254 to keep `0xFF` as the "no variant yet" sentinel.
  The cap of 64 was a local `constexpr` in two `PluginProcessor.cpp`
  sites with no compile-time link to the counter type — a future bump
  above 254 would silently saturate the counter while the returned
  index kept going, diverging RR behaviour. Hoisted
  `kMaxVariantsPerCell` into `MicrotonalSamplerVoice.h` next to
  `RrCounterArray` and added
  `static_assert(kMaxVariantsPerCell < 255, "variant index must fit
  in uint8 with 0xFF sentinel reserved")` so any future bump fails
  the build instead of failing audibly.
- **HG-05: folder-load callbacks no longer crash when a project closes
  mid-load.** `loadSampleFolder`, `kickNextReplayOp`'s loader
  dispatch, and `loadSingleSample` all captured `this` raw into
  message-thread completion/failure callbacks. `~SampleLoader`'s
  2-second `stopThread` joins the worker, but JUCE's
  `MessageManager::callAsync` queue is NOT flushed by
  `~AudioProcessor`, so callbacks already queued at destruction time
  ran with a dangling `this`. The processor is now
  `JUCE_DECLARE_WEAK_REFERENCEABLE`; every loader callback captures a
  `juce::WeakReference<OMicrotonalSamplerAudioProcessor>` and
  null-checks on entry. The destructor clears the weak-ref master
  before any other teardown so the bail-out path activates as soon as
  destruction begins. pluginval's tear-down stress paths exercise this
  flow.

### Notes
- All four fixes are HIGH-severity findings from the v1.11.1 deep
  code review (REVIEW-cpp-bugs.md). They share one root pattern:
  lifetime / re-entrancy assumptions that hold under nominal load but
  break under host quirks (off-thread save in Reaper, project-close
  mid-load, cascaded UI callbacks during replay). No DSP behaviour
  changes.

## [1.12.2] - 2026-05-02

### Fixed
- **FE-01: drag-drop folder streaming no longer silently corrupts on a
  single bad file.** The base64-streaming loop in
  `streamFolderEntryToCpp` already had a try/catch around the FileReader
  read + native-fn call, but only logged to console — the user saw the
  "Loading X of N" toast freeze on the next file with no indication
  anything had failed. Each per-file failure now toasts a specific
  "Skipped: <name> (read failed | backend rejected)" message and the
  final commit toast counts the skips ("Loading 47 of 50 samples (3
  skipped)…"). If every file fails, the commit step still runs so the
  C++ side reaps the empty session. `streamSingleFileEntryToCpp` got
  the same per-step protection so a corrupted single-file drop fails
  cleanly with a user-visible toast.
- **FE-02: backend stalls in drag-drop streaming no longer hang the UI
  permanently.** Every `Juce.getNativeFunction(…)` await in
  `streamFolderEntryToCpp` and `streamSingleFileEntryToCpp` is now
  wrapped — `dropSessionStart`, `dropSessionAddFile`,
  `dropSessionCommitFolder`, `dropSessionCommitFile`. The
  `showFolderLoadOptionsModal` await is also wrapped against modal
  promise rejection (DOM tear-down, cleanup-handler exception). On
  rejection each step surfaces a distinct toast (start / per-file /
  commit) and aborts cleanly, leaving the UI responsive. Previously, a
  C++ deadlock or message-thread stall would leave the user staring at
  a stale "Loading…" toast with no way to recover short of closing the
  plugin window.
- **FE-03: stale-cell race when sample-map snapshots fire mid-click.**
  The 250 ms double-click discriminator in `bindGridInteractions`
  schedules a `setTimeout` that closes over a `cell` DOM reference and
  reads `dataset.note` / `dataset.layer` at fire time. If a folder load
  or sampleMapUpdated event triggered `renderGrid` between click and
  fire, the timer would either no-op against a detached node or — if
  the grid had been re-rendered with a different sample map — fire
  against a re-bound cell at the same grid position carrying different
  MIDI/layer values. `renderGrid` now clears `pendingClickTimer` at the
  top, matching the cleanup the dblclick branch already performs.

### Notes
- All three fixes are fail-safe: per-iteration error handling in
  drag-drop loops + UI-recovery toasts on every backend await + cancel
  the deferred single-click whenever the grid rebuilds.

## [1.12.1] - 2026-05-02

### Fixed
- **CR-01: CC11 (Expression) no longer calls `setValueNotifyingHost` from the
  audio thread.** Per-byte `setValueNotifyingHost` in `processBlock` was a
  real-time correctness violation — listeners chain back through host
  parameter machinery, can take locks, allocate, and stall the audio thread
  in some hosts. Fast CC11 streams now stage the latest 0..127 value into a
  `std::atomic<int>` on the audio thread; an `AsyncUpdater` drains the
  atomic on the message thread and forwards to the host. Last-value-wins
  semantics within a block are unchanged; the audio path is now lock-free.
- **HG-08: `loadOpHistory` and `lastSkippedFiles` synchronised against
  off-thread `getStateInformation`.** Reaper (and possibly other hosts) call
  `getStateInformation` from a save-state worker thread, racing the message-
  thread mutations from folder-load completion callbacks. A
  `juce::CriticalSection` now guards both containers across all
  mutation/read sites (`applyFolderLoad`, `clearSampleMap`,
  `restoreStateValueTree`, `confirmRoundRobinLoad`, `loadSampleFolder`
  failure callback, `loadSingleSample`, and `captureStateValueTree`).
  Project saves during in-flight folder loads can no longer produce
  truncated XML.

## [1.12.0] - 2026-05-02

### Fixed
- **Drag-dropped folders now persist correctly across project save/reopen.**
  v1.0.4–v1.11.x recorded the WebView drag-drop temp dir
  (`/tmp/o-microtonalsampler-drop-<id>/`) as the saved sample-folder path,
  so on reload the missing-folder modal pointed at a `/tmp/...` path that
  was reaped at the next drop session. The state format now distinguishes
  filesystem loads from drag-drop loads and persists the original folder
  name lifted from `FileSystemEntry::name` at drop time. On reload, drag-
  dropped sessions surface a friendlier modal: *"Samples were drag-dropped
  from <name> without 'Embed audio' enabled — re-drag the folder or browse
  to relocate."* No more dead `/tmp/` paths in the UI.

### Added
- **Embed audio in project state.** New "Embed audio in project state"
  checkbox in the Folder Load Options modal (shown for both Load Folder
  dialog and drag-drop). When ON, the loaded audio is serialised inline
  into the saved project state as 24-bit PCM WAV. Tradeoffs:
    - Project survives folder moves, cross-machine transfer, and (for
      drag-drop) WebView temp-dir cleanup unchanged.
    - Project file size grows by the audio data size — the modal shows a
      live size estimate when the checkbox is on so the user always sees
      the cost.
    - For drag-drop, total bytes are computed during the entry-tree pre-
      walk and shown directly in the options modal.
    - For Load Folder dialog, a follow-up confirmation modal surfaces the
      actual size after the user picks a folder, before the load commits.
    - Default is OFF — current behaviour is preserved unless the user
      explicitly opts in per load.
- **Drag-drop missing-folder modal variant.** When a drag-drop op without
  embed is restored from a saved project, the missing-folder modal renders
  drag-drop-specific copy and a "Browse for folder…" button (vs the
  filesystem variant's "Locate folder…").

### Changed
- **State XML schema for `<SampleFolders><Op …/>`** — additive, fully
  backward-compatible with v1.11.x saves:
    - New optional attrs: `kind` ("filesystem"|"drag-drop"), `name`
      (display name for the missing-folder modal), `embed` ("1" iff inline
      audio).
    - Drag-drop ops omit the `path` attr (the temp dir is session-scoped).
    - When `embed=1`, the op carries an `<Audio>` child with
      `<Cell midi=… layer=…><Variant filename=… loopMode=…
      loopStart=… loopEnd=… wav="<base64>" /></Cell>` entries.
    - States saved on v1.11.x and earlier load identically (missing attrs
      default to filesystem origin, no embed).
- **`folderMissing` WebView event payload** — now an object
  `{path, kind, name}` instead of a bare string. JS branches on `kind` to
  render the appropriate modal copy. Backward-compat for stale string-form
  payloads is kept defensively in `subscribeFolderMissingEvent`.
- **Native fn split** — `loadSampleFolderDialog` (v1.6.0) replaced by
  `pickSampleFolder` + `estimateFolderAudioSize` + `loadSampleFolderByPath`.
  The split lets JS show the embed-size confirmation between selection and
  load. New native fns:
    - `pickSampleFolder()` → `{path, name, cancelled}`
    - `estimateFolderAudioSize(path)` → `int64` bytes (sum of `*.wav`,
      `*.aif`, `*.aiff`, `*.flac` files, recursive)
    - `loadSampleFolderByPath(path, layer, mode, override, embedAudio)`
- **`dropSessionStart`** — accepts an optional `args[1] = folderName` (from
  `FileSystemEntry::name`) so drag-drop loads carry a stable, user-meaningful
  display name into the saved state.
- **`dropSessionCommitFolder`** — accepts an optional `args[4] = embedAudio`
  (0/1) so drag-drop loads can opt into inline audio serialisation.

### Notes
- **State size impact (embed mode)**: 24-bit PCM at host SR × ~33% base64
  overhead. A 250 MB sample library encoded at 48 kHz / 24-bit / stereo
  yields a project state on the order of 250–350 MB. Project save/reopen
  performance scales with state size; users with large libraries should
  weigh portability against project-file weight.
- **Audio quality (embed mode)**: 24-bit PCM has a -141 dB noise floor —
  inaudible artifacts. Float samples outside [-1, +1) clip on encode (same
  constraint as any 24-bit export pipeline).
- **No breaking changes.** Saved sessions / presets from v1.11.x reload
  identically. The new behaviour only activates when (a) the user explicitly
  opts into embed via the modal, or (b) a new drag-drop load is saved on
  v1.12.0+.
- **v1.11.x sessions with drag-drop loads**: those projects will continue
  to surface the legacy missing-folder modal pointing at the old `/tmp/`
  path on first reload (no `kind` attr in the saved state means it
  classifies as filesystem). After the user relocates or skips, the next
  save records the friendlier drag-drop kind for any new drops.

## [1.11.3] - 2026-05-02

### Fixed
- **Use-after-free on `cellLow` / `variantLow` raw pointers across SampleMap
  swap (REVIEW CR-04).** `MicrotonalSamplerVoice::startNote` re-snapshots
  `currentMap` from `*sampleMapSource` after running steal-tail rendering.
  The voice's `variantLow` / `cellLow` raw pointers index into the OLD map's
  variants vector; if no other voice held a snapshot, the swap dropped the
  prior shared_ptr's last refcount and freed the audio buffers `variantLow`
  pointed into. v1.11.3 captures `prevMap = currentMap;` at the top of
  `startNote` so the prior map's refcount stays ≥ 1 for the entire
  function — including `renderTailRamp` and the small window before
  `variantLow` is reassigned to the new map's variants.
- **`renderTailRamp` early-return guard restructured into positive form
  (REVIEW DSP CRITICAL #1).** The guard at lines 240-254 was logically an
  OR of error conditions but was flagged by both the DSP and C++ reviewers
  as ambiguous and a click-on-steal regression risk. v1.11.3 rewrites it
  as `if (! prereqsMet) { zero+return; }` so the render path is
  unmistakably reachable.
- **Ramp coefficient division underflow when `rampSamples < 2`
  (REVIEW DSP HIGH).** The expression `(float) i / (float) rampSamples` in
  the per-sample render loop is well-defined for `rampSamples >= 1` but
  produces a degenerate one-step ramp at `rampSamples == 1` and is fragile
  at `rampSamples == 0` if the upstream `> 0` guard is ever weakened.
  v1.11.3 folds `rampSamples >= 2` into the `prereqsMet` predicate so the
  1-sample (no audible fade) and 0-sample cases take the zero+bail path
  before the ramp loop runs.
- **APVTS `getRawParameterValue("attack")->load()` null-deref on note-on
  (REVIEW DSP CRITICAL #2).** `startNote` previously dereferenced the
  result of `getRawParameterValue` for each of `attack` / `decay` /
  `sustain` / `release` without a null check — a typo or APVTS layout
  change would crash the audio thread on every note-on. v1.11.3 caches
  `attackParam` / `decayParam` / `sustainParam` / `releaseParam` atomic
  pointers in `prepareToPlay` (with a `jassert` per pointer in debug
  builds), and `startNote` only invokes `adsr.setParameters` when all four
  are non-null.

### Notes
- **No state-format / parameter / preset / API changes.** Sessions saved
  on v1.11.2 reload identically — these are pure voice-render correctness
  fixes living entirely inside `MicrotonalSamplerVoice.{h,cpp}`. CMake
  `VERSION` bumped to `1.11.3` so the About tab and bundle plist reflect
  the patch.
- **Validation:** hammer note-steal patterns (fast repeated notes
  exceeding the polyphony cap) and confirm clean tail-fades on every
  steal — no clicks, no silence on the stolen voice's tail. The four fixes
  are independent; only the renderTailRamp restructure is audible under
  normal play.

## [1.11.2] - 2026-05-02

### Security
- **Path traversal in drag-drop streaming surface — fixed.**
  `dropSessionAddFile` (Source/PluginEditor.cpp) previously forwarded the
  JS-supplied `relPath` straight to `juce::File::getChildFile` and then to
  `replaceWithData`. With no validation, a malicious WebView page could
  pass `../etc/passwd`, an absolute path, or a backslash-escaped Windows
  path and write outside the session-scoped temp dir
  (`/tmp/o-microtonalsampler-drop-<id>/`). v1.11.2 introduces
  `Source/DropSessionGuard.h::validateRelPath` which rejects empty,
  absolute, backslash-separated, NUL-bearing, or `..`-segment paths
  *before* any allocation, plus `validateParentChain` which walks the
  target's existing ancestors and rejects any chain that traverses a
  symbolic link or exits the session dir. (REVIEW finding **CR-02**.)
- **Unbounded base64 streaming — capped.**
  `dropSessionAddFile` had no per-file or per-session size cap; a hostile
  page could trivially OOM the DAW host with a single multi-GB base64
  string. v1.11.2 enforces a **256 MB per-file** cap and a **4 GB
  per-session** cap (`kMaxFileBytes` / `kMaxSessionBytes`). The check
  uses the projected decoded size (`base64.length() * 3 / 4`) and runs
  *before* the decode buffer is allocated, so an oversized payload is
  rejected without ever touching memory. The aggregate counter
  (`currentDropSessionTotalBytes`) is reset in `dropSessionStart` and
  incremented only after a successful write. (REVIEW finding **CR-03**.)

### Tests
- **New regression test: `Source/tests/drop_session_guard_check.cpp`.**
  Standalone executable (build with `ninja
  O-MicrotonalSampler_DropSessionGuardCheck`). 24 assertions — including
  the headline `../etc/passwd` rejection, a >256 MB per-file rejection,
  a >4 GB session-aggregate rejection, and a real-symlink escape attempt
  on POSIX. Returns exit code = number of failed cases (0 = all pass).

### Notes
- **No state-format / parameter / preset / API changes.** Sessions saved
  on v1.11.1 reload identically — the security fixes live entirely on the
  drag-drop surface. Existing in-memory sessions, preset banks, and host
  automation lanes are unaffected.
- **User-visible behaviour change is rejection-only.** A well-formed
  drag-drop of a sample folder under 4 GB (the documented 250 MB
  reference library size leaves ~16× headroom) behaves exactly as in
  v1.11.1. A malicious or malformed payload now fails fast with a
  `dropSessionAddFile` DBG line and the JS bridge sees `false`.
- **Files touched:** new `Source/DropSessionGuard.h`,
  `Source/PluginEditor.cpp` (3 edits in `dropSessionStart` /
  `dropSessionAddFile`), `Source/PluginEditor.h` (new
  `currentDropSessionTotalBytes` member), new
  `Source/tests/drop_session_guard_check.cpp`, `CMakeLists.txt`
  (VERSION bump + test target + DropSessionGuard.h listed in source set).

## [1.11.1] - 2026-05-02

### Fixed
- **Octave-off bug — sample filenames now parse with C3=60 convention.**
  Playing a key labelled `G1` in the host DAW produced audio at `G2` pitch
  (one octave too high). Root cause: `Source/FilenameParser.cpp` parsed
  scientific-pitch tokens with the C4=60 (Yamaha/JUCE-native) convention
  via `midi = (octave + 1) * 12 + semitoneOffset`, while every dominant
  DAW (Ableton Live, Cubase, FL Studio, Logic Pro, Pro Tools, Reaper
  default) labels middle C as C3 = MIDI 60. A user folder of `G0.wav,
  G1.wav, G2.wav, …` recorded in DAW-native labelling was therefore
  stored at cell `midiNote` values one octave below the actual recorded
  pitch; on playback, `MicrotonalSamplerVoice::computePlayRateForVariant`
  (Source/MicrotonalSamplerVoice.cpp:113-123) computed a 2× ratio
  (`desiredFreq / cellRefFreq`) and transposed the sample up an octave.
  Switched the parser to `midi = (octave + 2) * 12 + semitoneOffset` and
  updated the matching UI label formula in `Resources/ui/js/sampler-app.js`
  (`midiToNoteName`) so cell labels stay in lockstep with parsed MIDI
  numbers. Inline parser tests rebased onto the new convention (C3=60
  anchor case added; previous C4-based assertions shifted by 12).

### Notes
- **No state-format / parameter / preset changes.** Sessions saved on
  v1.11.0 reload identically — `SampleMap` cell `midiNote` values are
  rebuilt from filenames at folder-load time, not persisted, so the new
  convention takes effect on next folder load. Existing in-memory
  sessions stay valid until the user reloads samples.
- **Label shift visible in the Sample Map grid.** Cells previously
  labelled `C4` will now read `C3`, `C5` will read `C4`, etc. — pitches
  unchanged, only the displayed octave numbers move down by one to match
  the host DAW's ruler.
- **Compatibility caveat — folders named in C4=60 (Yamaha) convention.**
  Users whose sample folders were named to match the *previous* parser
  convention (e.g. samples actually recorded at MIDI 60 named `C4.wav`
  in JUCE-native form) will see their folders load one octave low after
  this update. Workaround: rename the folder so each filename's octave
  digit is one lower (e.g. `C4.wav → C3.wav`), or re-export from the DAW
  to pick up its native labelling. The C3=60 default matches the vast
  majority of modern DAW exports.
- **Files touched:** `Source/FilenameParser.cpp` (formula + comment +
  inline tests at lines 431-490), `Resources/ui/js/sampler-app.js`
  (`midiToNoteName` at lines 595-604), `CMakeLists.txt` (VERSION bump).

## [1.11.0] - 2026-05-01

### Added
- **Paper-texture backgrounds.** The page background and all card surfaces
  (header, About card, Tuning panel container) now ride on antique paper
  textures instead of solid cream/warm fills. `paper1.jpg` (964×598) drives
  the page body via `center/cover` with a faint warm-tint overlay so the
  existing palette tokens (text, accent-gold, border-warm) keep their
  intended contrast. `paper2.jpg` (516×885) drives the card surfaces under
  a translucent `--bg-card` overlay so the parchment grain reads through
  without sacrificing legibility.

### Changed
- **About → "Ouaricon" link** now points to `https://oaudio.io/` (was
  `https://ouaricon.com`).

### Fixed
- **About-tab version pill was hardwired to v1.0.0 across every release.**
  Plugin `CMakeLists.txt` used `PLUGIN_VERSION "x.y.z"`, which is **not** a
  recognized `juce_add_plugin` keyword — JUCE silently dropped it and fell
  back to `PROJECT_VERSION` from the root `project(JUCEPlugins VERSION
  1.0.0)` declaration. The About tab's `getPluginVersion` native function
  returns `JucePlugin_VersionString`, which was therefore stuck at
  `"1.0.0"` for every shipped version (v1.0.0–v1.10.0). Renamed the arg to
  the correct `VERSION "1.11.0"` so future bumps wire through to the About
  pill and the bundle plist (`CFBundleShortVersionString`) automatically.

### Notes
- Implementation: paper textures embedded via `juce_add_binary_data` in
  `CMakeLists.txt` and served by `PluginEditor.cpp` resource provider at
  `/images/paper1.jpg` and `/images/paper2.jpg`. CSS uses layered
  `background` (tint gradient + image + solid fallback) so the resource
  provider failing degrades gracefully to the previous v1.10.0 cream.
- Pure visual + housekeeping change. No DSP, parameter, sample-map, or
  preset-format changes.
- v1.10.0 backup created at `backups/O-MicrotonalSampler/v1.10.0/` (was
  missing — every prior release backed up its own predecessor except this
  one).

## [1.10.0] - 2026-05-01

### Added
- **Naturalist aesthetic — anatomical brain overlay.** Antique anatomical
  engraving (cerebrum + central nervous system) layered behind the UI as a
  subtle decorative overlay, matching the Ouaricon Naturalist aesthetic
  established in O-Lyrica (botanical fern overlay). Image is sepia-tinted
  to lock into the cream/warm-brown palette, sits behind all interactive
  content (z-index: 0) with `pointer-events: none` so it never blocks
  input.
- **Tab-aware parallax positioning.** Overlay slides subtly between tabs:
  - **Sample Map** — peeks from right edge (right: -60px, opacity 0.18)
  - **Tuning** — retreats further right (right: -120px, opacity 0.13)
  - **About** — swings into full view as a feature image (right: 40px,
    opacity 0.32)
  Transitions are 0.45s ease-out for both `right` and `opacity`.

### Notes
- Pure visual addition. No DSP, parameter, or behavior changes — the
  v1.9.1 sample-map / round-robin / merge-rr surface is untouched.
- Implementation pattern lifted verbatim from O-Lyrica v1.4.0
  (`.botanical-overlay`): single `<img>` element absolutely positioned
  inside `#app`, served via the existing WebView resource provider, tab
  switcher swaps a position class. Image is embedded as `BinaryData`
  via `juce_add_binary_data` in `CMakeLists.txt`.
- File: `Resources/ui/images/brains.png` (~334 KB).

## [1.9.1] - 2026-05-01

### Fixed
- **`Layer as round-robin` load mode silently fell back to `ReplaceAll`,
  wiping the entire sample map.** The JS folder-load modal correctly emits
  `"merge_rr"` for the new v1.9.0 mode, but the C++ string→`LoadMode`
  translation in `PluginEditor.cpp` only handled `"append"` and
  `"replace_layer"` — every other string (including `"merge_rr"`) hit the
  `LoadMode mode = LoadMode::ReplaceAll;` default, so picking the new mode
  replaced the existing map instead of merging. Affected both load paths:
  `loadSampleFolderDialog` (file-chooser, line 294) and
  `dropSessionCommitFolder` (drag-drop folder, line 491). Added the
  missing `else if (modeStr == "merge_rr") mode = LoadMode::MergeRR;`
  branch in both blocks.

### Notes
- v1.9.0 backend (`applyMergeRrCell`, `LoadMode::MergeRR`,
  `loadModeToString`/`loadModeFromString`) was already correct — the bug
  was purely at the WebView→C++ translation boundary, so the
  `O-MicrotonalSampler_MergeRrCheck` standalone test target kept passing
  even with the bug live.
- Behaviour after fix (verified in DAW): load folder A as ReplaceAll, then
  load folder B with "Layer as round-robin" — folder A's cells outside
  B's range persist; cells where A and B overlap gain B's variants
  appended onto A's existing variant vectors.

## [1.9.0] - 2026-05-01

### Added
- **`Layer as round-robin` load mode.** The folder-load options modal grows
  a 4th radio: **Layer as round-robin**. With this mode selected, a freshly
  loaded folder is merged into the existing sample map; on (note, layer)
  collisions, the new cell's variants are **appended** onto the existing
  cell's variants vector instead of replacing it. Useful for layering
  multiple takes/recordings as round-robin alternates on the same notes
  without needing to relabel filenames with `rr/take/tk` tokens.
- **Per-cell single-file merge prompt.** Triggering a per-cell sample load
  (cell button, double-click on a loaded cell, or context-menu Replace) on
  a cell that already holds samples now surfaces a small confirm dialog:
  *"Add as round-robin variant N+1, or replace?"*. Empty cells skip the
  prompt and load directly (v1.8.0 behaviour preserved).
- **Variant cap (64 per cell).** Both load paths enforce a hard cap of 64
  variants per (note, layer). Excess incoming variants are surfaced via the
  existing skipped-files list (`variant cap reached: <filename>`). At cap,
  the per-cell merge button is disabled and only Replace remains.

### Changed
- **`LoadMode` enum.** New `MergeRR = 3` value, serialized as `"merge_rr"`
  in the load-op history. Older builds (v1.8.0/v1.7.x) reading a v1.9.0
  preset fall back to `ReplaceAll` for unknown mode strings (graceful
  degradation: cells survive in the snapshot, but merge ops won't replay).
- **`loadSingleSample`** gains an optional `mergeAsRr` parameter (default
  false — preserves v1.8.0 callers). When true and the target cell is
  non-empty, the new variant is appended; when false, the cell is replaced
  as before. `loadSingleSampleDialog` native function accepts the flag as
  its 3rd arg.
- **`applyMergeRrCell` helper** extracted into `SampleMap.h` (header-only
  pure function). Used by both folder-load `MergeRR` mode and the per-cell
  merge path; isolates the merge contract for unit testing.

### Implementation notes
- **Backward compat.** `getStateInformation`/`setStateInformation` schema is
  unchanged. v1.7.x and v1.8.0 saves replay identically. v1.9.0 saves with
  `merge_rr` ops opened in v1.8.0 fall back to `ReplaceAll` for those ops
  (per existing `loadModeFromString` default).
- **RT-safety contract preserved.** Merge work happens entirely on the
  message thread (same path as v1.8.0 `applyFolderLoad`). The
  `currentSampleMap` shared_ptr is atomic-stored after the merge; voices
  holding the previous snapshot keep their buffers alive transitively for
  the held note's duration. RR counters for every touched cell reset to
  the sentinel so the next note-on doesn't index past the just-grown
  variants vector with a stale value.
- **Drag-drop scope.** v1.9.0 surfaces the per-cell merge prompt for the
  file-picker path (cell button, dblclick, context-menu Replace). Drag-drop
  of a single file onto a non-empty cell still uses v1.8.0 replace behaviour
  to keep the multi-file drop session UX uninterrupted; use the cell's
  load-sample button or the folder-load `Layer as round-robin` mode for
  explicit RR layering. (Drag-drop merge is a candidate for v1.9.x.)

### Test surface
- New standalone `O-MicrotonalSampler_MergeRrCheck` target — six unit
  tests over the `applyMergeRrCell` helper: no-collision insert, collision
  merge with order preservation, variant cap (64), cap-already-reached
  early-out, layer-aware collision key, multi-call folder-shape ordering.
- v1.8.0 round-robin render harness untouched — render path is bit-identical
  for non-merge loads (single-variant + token-RR libraries).

## [1.8.0] - 2026-05-01

### Added
- **Round-robin sample variants.** A single (note, velocity layer) cell can
  now hold multiple sample takes. At every note-on, the engine picks one
  variant according to the user-selected RR mode. Single-variant cells are
  unchanged — the render path is bit-identical to v1.7.1 for libraries
  without RR tokens.
- **Three selection modes** via a new `Round-Robin Mode` parameter
  (`rr_mode`):
  - **Cycle** — sequential `0 → 1 → … → N-1 → 0`, deterministic.
  - **Random No-Repeat** *(default)* — uniform random pick excluding the
    last-played variant, the industry standard for orchestral/percussive
    libraries.
  - **Random** — uniform random, may repeat. Useful for foley/ambience.
- **Filename token detection.** The folder loader now recognises
  `rr[N]`, `take[N]`, and `tk[N]` tokens (case-insensitive, 1-based) and
  groups files sharing the same `(note, layer)` into one cell as silent
  variants. Examples that load without prompting:
  `vln_C4_v1_rr1.wav`, `kick_C2_take03.wav`, `cello_g3_tk2.aif`.
- **Ambiguity confirmation modal.** Folders with bare duplicates (same
  `(note, layer)` but no rr/take/tk tokens) now surface a WebView modal
  listing the conflicting filenames. The user can either accept them as
  RR variants or cancel the load — protects against accidental ingest of
  redundant samples.
- **Per-variant loop editor.** When a cell has more than one variant, the
  loop editor side panel grows a tab strip (`Variant 1 of N` indicator +
  one numbered tab per variant). Each tab carries its own loop start/end,
  loop mode, and apply/reset state — every variant can be tuned independently.
- **Per-cell variant tooltip.** Multi-variant cells in the sample grid
  display a small antique-gold dot in the upper-right corner and a
  multi-line hover tooltip listing every variant's filename in load order.
- **`confirmRoundRobinLoad(accept)` native function.** Exposed for the
  modal's accept/cancel buttons; chains correctly through the v1.6.0 state-
  restore replay queue so reopened projects with ambiguous folders surface
  the modal sequentially without losing later ops.

### Changed
- **`SampleSlot` → `SampleCell` + `SampleVariant`.** Internal sample-map
  storage refactored — a cell is the addressable `(midi, layer)` coordinate;
  variants hold the audio + per-take loop fields. `findSlot` → `findCell`.
  Render path semantically identical for single-variant cells.
- **`SampleMap` JSON schema.** The snapshot now carries a `cells` array
  (each with a `variants[]` sub-array). The legacy `slots` array is still
  emitted for back-compat — primary variant per cell, plus a new
  `variantCount` field so older consumers can detect multi-variant cells.
- **Per-cell single-load behavior.** Clicking an empty cell to load a single
  sample replaces the whole cell with a one-variant cell, even if the cell
  previously held a multi-variant set. To build a multi-variant cell, use a
  folder load with rr/take/tk tokens (or accept the bare-duplicate modal).

### Implementation notes
- **RT-safety contract preserved.** Variant selection is pure atomic-counter
  + integer math + xorshift32; zero allocations in `startNote` or
  `renderNextBlock`. The 512-byte counter array (128 notes × 4 layers,
  `std::atomic<uint8_t>`) lives in the processor and survives map swaps —
  reset only on `LoadMode::ReplaceAll` and per-layer wipes for
  `ReplaceLayer`.
  - *Deviation from spec:* the plan called for 352 entries (88-key range);
    we use 512 (full 0..127 × 4) for index-bound safety. ~0.16 KB difference.
- **Per-voice xorshift32 PRNG.** Seeded from the voice's `this` pointer +
  sample rate so each voice gets a distinct stream; mutated only in
  `selectVariantIndex` (audio thread, startNote-time, never per-sample).
- **Atomic-swap semantics intact.** Cell vector deep-copy is still cheap
  (each variant's audio is a `shared_ptr<juce::AudioBuffer<float>>`). Voices
  that snapshot the map at startNote keep variants alive transitively for
  the note's duration even if the map is replaced mid-note (Stage 2 EC-3).
- **Preset compatibility.** `getStateInformation` / `setStateInformation`
  schema is unchanged — sample data is still referenced by folder path, not
  embedded. Replaying a v1.7.x save in v1.8.0 simply rebuilds single-variant
  cells via the same folder-load path. v1.8.0 saves opened in v1.7.x will
  still load (the new `rr_mode` parameter is silently ignored by APVTS;
  folder paths replay identically).

### Test surface
- Render-harness identity test passes for single-variant libraries —
  bit-identical output vs v1.7.1.
- New `FilenameParser::runTests` cases cover `rr1..rr64`, `take01..take64`,
  `tk1..tk2`, both pre- and post-note placement, and rejection of
  unrecognised tokens (`round1`, `var3`, etc.).
- pluginval `--strictness-level 5 --skip-gui-tests` SUCCESS.
- auval `-v aumu OMtS OuDv` AU VALIDATION SUCCEEDED.

### Known limits (deferred to v1.9)
- Per-variant velocity sub-layering.
- Cross-cell RR group tagging (e.g., "all snares in this folder share one
  cycle").
- Per-cell RR algorithm override (the `rr_mode` parameter is global for v1.8).

## [1.7.1] - 2026-05-01

### Fixed
- **Tuning panel note highlights now appear when notes are played.** The
  Circle and Polar visualizations and the True Keys interval display were
  silent: pressing keys never lit up scale degrees and True Keys never
  showed intervals. **Root cause:** the C++ side was not publishing any
  MIDI activity to the WebView. There was no active-note tracking on the
  synth, no editor timer, and no JavaScript handlers for the events the
  TuningPanel expects (`tuningNoteOn` / `tuningNoteOff` / `updateHeldNotes`).
  In short, the wiring between the audio engine and the panel was missing
  end-to-end. (This was a latent gap from Phase 3.1 — the panel was
  designed to receive these events but the producer side was never built.)

- **Polar view now highlights active scale degrees.** Even with the wiring
  fixed above, the polar plot would still not respond to held notes —
  `drawPolarPlot()` ignored `activeScaleDegrees` entirely (every dot was
  drawn with the same fill colour) and `updateSpokeHighlights()`
  short-circuited for any mode other than Circle. Both have been fixed:
  active dots now render in the same red (`#C0392B`) the Circle view uses,
  with a slightly larger radius for emphasis.

### Implementation notes
- **Audio thread:** `CappedSynthesiser` now keeps two `std::atomic<uint64>`
  bitmasks (low = MIDI 0–63, high = 64–127) updated via lock-free
  `fetch_or` / `fetch_and` inside the existing `noteOn` override and a new
  `noteOff` override. No allocations, branch-free bit ops, no impact on
  `processBlock` cost.
- **Message thread:** `PluginEditor` now inherits `juce::Timer` and runs
  at 30 Hz. Each tick reads the bitmask, diffs against the previous
  snapshot, and emits per-note `tuningNoteOn` / `tuningNoteOff` events
  for new transitions plus a `tuningHeldNotes` payload (`{notes,freqs}`)
  for True Keys. Early-out when no bits changed — typical idle cost is
  one atomic load per tick.
- **TrueKeys frequencies:** the held-notes payload calls
  `TuningEngine::getFrequency(midi)` per note so the cents readout
  reflects the active microtonal tuning, not 12-TET.
- **Late mount catch-up:** the TuningPanel is mounted lazily on first
  Tuning-tab activation. A new native function `getHeldNotesJson` lets
  the panel pull current state at mount, so notes already held when the
  user clicks the Tuning tab show up immediately. Subsequent updates
  flow through the timer-driven events.
- **Polar redraw:** `updateSpokeHighlights()` now falls through to
  `drawPolarPlot()` when in polar mode. The cost is 12 dots redrawn at
  up to 30 Hz — well below any perf threshold.

## [1.7.0] - 2026-04-30

### Added
- **Expression control for dynamics (MIDI CC 11).** New `expression` APVTS
  parameter (0–100 %, default 100 %) wired to MIDI Continuous Controller 11
  (the industry-standard "Expression" controller for orchestral mockups).
  Incoming CC 11 messages drive the parameter via `setValueNotifyingHost`
  so DAW automation lanes mirror live controller input — last-touched wins.
- **Expression knob on the bottom control strip.** Bound to the new
  parameter via `WebSliderRelay` / `WebSliderParameterAttachment`, so
  knob, host automation, and CC 11 stay synchronised.

### Behaviour
- Expression is **independent of velocity-layer selection.** Velocity
  (note-on velocity) still selects which layer plays at note-on; the
  expression knob scales the post-mix output afterwards. Mid-note
  expression changes therefore change loudness without retriggering or
  crossfading layers — matching Kontakt / Spitfire convention.
- **Curve:** squared (final gain = expression²). Sampler convention; gives
  smoother fades and a more "natural" feel than a linear curve.
- **Smoothing:** 10 ms per-block linear ramp (mirrors the existing
  `output_gain` smoother — RESEARCH R7, pitfall #8). Sample-accurate
  per-event smoothing was deemed unnecessary; the 10 ms ramp covers
  per-block CC jumps without zipper noise.
- **Signal chain:** voices → expression gain → output gain → output. The
  two stages multiply, so global trim and dynamics are independent.

### Implementation notes
- `processBlock` scans the MIDI buffer for CC 11 (last-value-wins per
  block) before `renderNextBlock`. The CC's 0–127 value maps directly
  to the parameter's 0..1 normalised range. The squaring happens at
  gain-application time, not at parameter-write time, so host automation
  and the knob both expose a clean linear 0–100 % surface.
- New parameter is added at the end of the layout (between
  `velocity_crossfade` and `output_gain`). Existing presets / sessions
  load with expression at its default (100 %) — no breaking change.

## [1.6.0] - 2026-04-30

### Added
- **Explicit velocity-layer assignment for folder loads.** Both the
  Load Folder… button and macOS folder drag-drop now open a "Load
  samples" modal before any scan/streaming work begins. The modal
  exposes three controls:
  - **Layer (L0–L3):** segmented selector for the target velocity row
    (4 layers, matching the existing grid).
  - **When loading:** Add to layer / Replace this layer / Replace all
    samples (merge mode).
  - **Ignore filename velocity tokens:** checkbox that forces every
    incoming sample onto the chosen target layer regardless of
    `_v1`/`_ff`/`layer3`/etc. in the filename.

  A live explainer below the controls describes the resulting
  behaviour for the current settings (e.g. _"Add samples to L2,
  ignoring filename velocity tokens"_) so the user can preview
  the load before confirming.

- **Multi-folder sample maps.** Append mode merges new samples into
  the existing map without wiping it, so a single bank can be assembled
  from several drops (e.g. drop a "soft" folder onto L0, then drop a
  "loud" folder onto L3 with override on, and both layers play under
  velocity-crossfade as expected). `(midi, layer)` collisions are
  overwritten by the most recent drop.

- **Load-op history persisted in plugin state.** Every successful
  folder load is recorded in `loadOpHistory` and written to plugin
  state as `<SampleFolders><Op …/>…</SampleFolders>`. On project
  reopen, the ordered op list is replayed sequentially via the same
  pipeline so the multi-folder map is faithfully reconstructed —
  including target layer, merge mode, and override flag.

- **Tolerant state replay for missing folders.** If any persisted op
  references a folder that no longer exists on disk, the existing
  missing-folder modal is surfaced for the first one and subsequent
  ops continue (silently skipped) so a partial reload is still useful.

### Changed
- **`SampleLoader::loadFolder` signature now takes `LoadOptions`.**
  When `overrideTokens=true`, every parsed slot is forced onto the
  caller-supplied target layer; when `false`, filename tokens win
  (legacy v1.5.x behaviour). Default-constructed options reproduce
  v1.5.x exactly.
- **`OMicrotonalSamplerAudioProcessor::loadSampleFolder` signature now
  takes `(folder, targetLayer, mode, overrideTokens)`** with defaults
  `(file, 0, LoadMode::ReplaceAll, false)` so the missing-folder
  relocate path and any internal callers retain v1.5.x semantics
  without code changes.

### Migration notes
- **No breaking changes.** Old saved state still loads:
  `<SampleFolder path="…"/>` from v1.5.x and earlier is detected and
  replayed as a single ReplaceAll op with target layer 0 and override
  off, so v1.5.x sessions/presets behave bit-for-bit identically. New
  saves emit the `<SampleFolders>` op-list container instead — old
  versions of the plugin loading a v1.6.0 save would simply ignore
  the unknown sibling and start with no folder loaded (graceful
  forward incompatibility).
- **Drag-drop folder loads on macOS materialise into a temp dir and
  are NOT persisted across save/reopen.** This matches existing
  v1.0.4 behaviour. Users who need persistence should use the
  Load Folder… button (which records the original folder path).

## [1.5.2] - 2026-04-30

### Changed
- **Tuning tab — intervals table is taller.** Reclaimed the empty vertical
  space below the intervals list by raising `.interval-list` `max-height`
  from `300px` to `400px` in `Resources/ui/css/tuning-panel.css`. Lets more
  degrees stay in view at once before scrolling kicks in. Editor default
  is 900×640 so 400 px still fits comfortably (62% of editor height) and
  scales down with the responsive layout.

### Migration notes
- **No breaking changes.** Pure CSS-only edit (one property). No parameter,
  state, preset, or layout-grid changes. v1.5.1 sessions/presets load
  identically.

## [1.5.1] - 2026-04-30

### Fixed
- **Tuning tab — visualization area top-justified.** The Circle / Polar /
  Matrix / True Keys / Rotation views in the center column are now
  anchored to the top of the viz container instead of vertically
  centered, eliminating the dead space between the viz-mode buttons and
  the visualization content. (`align-items: center` →
  `align-items: flex-start` on `.viz-view.active`.)
- **Tuning tab — wide tables no longer overflow the right edge.** The
  Matrix and Rotation tables previously expanded the center grid column
  past its allotted `1fr` width, pushing past the right boundary of the
  Controls panel. Root cause: CSS Grid items default to `min-width: auto`
  (= min-content), so any descendant wider than `1fr` blows the column
  out. Fix: `min-width: 0` on `.tuning-center-column` makes the column
  respect its track size, and `overflow: auto` on `.viz-container`
  scrolls wide content within the middle section instead of pushing
  past the right edge.

### Migration notes
- **No breaking changes.** Pure CSS-only fix in
  `Resources/ui/css/tuning-panel.css` (3 rule edits). No parameter,
  state, preset, or layout-grid changes. v1.5.0 sessions/presets load
  identically.

## [1.5.0] - 2026-04-30

### Changed
- **Loop editor panel is now inline** below the sample-map grid instead of
  sliding in from the right edge as a 360-px drawer. The panel is always
  visible — when no slot is selected it shows the placeholder text
  "Select a loaded sample slot to edit loop points"; selecting a loaded
  slot swaps in the waveform canvas, loop-point readouts, and action
  buttons. Reuses the existing blank space between the grid and the
  bottom knob row, so the waveform editor is reachable without any
  horizontal layout shift of the grid.
- **Close button (×)** now deselects the current slot (returns the panel
  to its placeholder state) instead of dismissing the entire editor.

### Removed
- `body.le-open` right-padding shift on `#tab-samplemap` (no longer
  needed — the inline panel doesn't overlap the grid).
- Slide-in transform/transition CSS on `#loop-editor-panel`.

### Migration notes
- **No breaking changes.** No parameter, state, or preset format changes.
  v1.4.0 sessions/presets load identically. Pure UI/UX layout change.

## [1.4.0] - 2026-04-30

### Changed
- **Loaded samples now loop the entire file by default.** Each slot is
  initialized with `loopStart = 0`, `loopEnd = N - 2`, `loopMode = Auto`
  on load. The renderer's existing 8-sample equal-power crossfade at
  the wrap handles click prevention. Replaces the v1.0–v1.3 RMS-based
  auto-detector that searched for a quiet sustain region in the latter
  60% of the file.
- **"Reset" in the loop-point editor** now snaps the slot back to
  whole-file loop instead of re-running auto-detect.

### Fixed
- **V11-LOOP-FALLBACK** (deferred from Stage 4 verification): sustained
  material with constant RMS (sine waves, drones, organ samples) used
  to fall through the auto-detector's variance gate and silently revert
  to one-shot, going silent before note-off. With whole-file loop as
  the default, these samples now sustain correctly.

### Removed
- `Source/LoopDetector.{h,cpp}` (Phase 2.5 RMS scan + variance gate +
  zero-crossing snap module — ~230 LOC) and the include sites in
  `SampleLoader.cpp` and `PluginProcessor.cpp`. The detector's
  defensive constraints (`loopEnd <= N - 2`, min loop length 16) are
  preserved as inline guards in the new whole-file path.

### Migration notes
- **Behavior change, not breaking.** v1.3.0 sessions/presets load
  cleanly. State persistence is unaffected (no parameter changes, no
  state schema changes). Audio output for one-shot percussive samples
  may differ — they now loop the whole file by default. Use the
  per-slot loop-point editor (Stage 3 UI) to set Manual loop points
  if a particular sample needs different behavior.

### Root cause notes
- The original auto-detector was tuned for sustained instrumental
  material with a clear noise-floor sustain region (e.g. piano
  release tails). On constant-RMS or transient material it
  conservatively rejected and fell back to one-shot — surprising
  default behavior for "load a sample and play it as a sustained
  pitched instrument," which is what the plugin is for.

## [1.3.0] - 2026-04-29

### Added
- **Full state persistence across DAW sessions.** Reopening a project
  now restores the loaded sample folder, tuning state (intervals, A4
  master tune, octave stretch, tonic, mode, KBM mapping), and all
  parameter values exactly as they were when the project was saved.
  Pre-v1.3.0 only persisted parameters — folder and tuning were lost.
- **Save/Load preset (`.omspreset`)** buttons in the header. Captures
  the same state used for project save/load (params + folder path +
  tuning) as a portable XML file. Per design Q1=A: paths only — sample
  audio is referenced, not embedded, so presets stay small but require
  matching folder structure across machines.
- **Missing-folder modal.** When DAW project reopen finds the saved
  folder no longer exists at its original path, a modal surfaces the
  path and offers "Locate folder…" (file picker, reuses
  `loadSampleFolder`) or "Skip" (clears pending state, sampler stays
  empty).

### Changed
- `PluginProcessor::getStateInformation` / `setStateInformation` now
  serialize a wrapped `ValueTree`: APVTS state plus `<SampleFolder>`
  and `<TuningState>` sibling children. Backward-compatible — v1.2.0
  sessions load cleanly (children absent → defaults), v1.3.0 sessions
  in v1.2.0 silently drop the new children.
- Tuning state is captured via the engine's existing accessors plus
  `generateScalaFileContent` / `generateKBMFileContent` round-trips,
  so no fork of the shared `scala-tuning-engine` module is required.
- Added 5 native functions to the WebView bridge:
  `saveCurrentPreset`, `loadPreset`, `locateMissingFolder`,
  `dismissMissingFolder`, `getPendingMissingFolder` — the last covers
  the boot-time race where state restore runs before the WebView has
  registered its `folderMissing` listener.

### Technical notes
- **Root cause** (pre-v1.3.0): `getStateInformation` only emitted
  `parameters.copyState()`, which is APVTS-only. The `currentSampleMap`
  was rebuilt from a folder reference held in memory but never written
  to the persisted state.
- **Threading**: `setStateInformation` runs on the message thread.
  Tuning restore is in-memory and synchronous; folder reload reuses
  the existing async `SampleLoader`. Missing-folder detection is
  synchronous (`File::isDirectory()`); the modal is surfaced via
  `emitEventIfBrowserIsVisible` plus a parked-path pull on WebView
  attach to cover the boot-time race.
- **Backup**: `backups/O-MicrotonalSampler/v1.2.0/` (rollback path).

## [1.2.0] - 2026-04-29

### Added
- **Tuning tab is now an editable authoring surface.** Reverses the
  Stage 3 §RQ3-1 read-only design. Users can:
  - **Select factory tunings** from the library (24+ presets across
    Historical, Just Intonation, Equal Divisions, Non-Octave, World).
  - **Load `.scl` (Scala scale) and `.kbm` (keyboard mapping) files**
    via native file pickers. Save also supported.
  - **Edit individual interval cents** by typing into the table on
    the left.
  - **Change tonic** (rotates 12-note scales).
  - **Adjust A4 reference pitch** (400–480 Hz) via the round knob.
  - **Apply octave stretch** (0.95–1.25 ×) for physical-modeling
    voicings.
  - **Generate scales** from EDO, harmonic series, or rank-2
    temperament parameters and apply them to the engine.
  - **Export the current tuning** as an HTML documentation page
    (with SVG pitch circle).
- Tuning Library and Scale Generator sections auto-expand on first
  Tuning-tab activation, so the right column shows selectable items
  immediately.

### Changed
- `PluginEditor.cpp` registers ~13 new WebView native functions that
  bridge `tuning-panel.js` calls to the shared `scala-tuning-engine`
  module: `setSingleInterval`, `setTonicNote`, `setOctaveStretch`,
  `setMasterTune`, `loadEmbeddedTuning`, `loadScalaFile`,
  `loadKBMFile`, `saveScalaFile`, `saveKBMFile`, `generateEDO`,
  `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`,
  `exportTuningHTML`. All file-picker variants use
  `juce::FileChooser::launchAsync` with a `shared_ptr` capture so the
  chooser outlives the async callback.
- `index.html` no longer links `tuning-panel-readonly.css`. The
  read-only CSS file is preserved on disk and as a binary resource
  for backward compatibility but is no longer applied.
- `sampler-app.js` removes the `applyIntervalReadonlyShim` span-swap
  and its `MutationObserver`; the editable `.interval-input`
  elements are now visible and wired to `setSingleInterval` via the
  panel's existing `handleIntervalChange` flow.

### Root Cause
- **Empty intervals table** — TWO root causes:
  1. `.interval-input` rows were hidden by `tuning-panel-readonly.css`
     and replaced with a static `interval-display` span.
  2. **Pre-existing latent bug since v1.0**: `tuning-panel.js` was
     instantiated with `window.__JUCE__` (the low-level postMessage
     handler), but every backend call inside the panel uses
     `juceApi.getNativeFunction(name)` — that method lives on the
     ES-module namespace `Juce` (imported in sampler-app.js as
     `import * as Juce from './juce/index.js'`), NOT on
     `window.__JUCE__`. Every call (`getTuningIntervals`,
     `getEmbeddedTuningList`, `setSingleInterval`, `loadScalaFile`,
     `generateEDO`, etc.) silently threw a `TypeError` and was
     swallowed by the panel's try/catch blocks. Fixed by passing
     `Juce` instead: `new TuningPanel(container, Juce)`.
- **Library: categories visible but no tunings** — `library-content`
  was collapsed by default; `loadEmbeddedTunings()` only fired on
  toggle-expand. Even after manual expansion, clicking an item
  silently failed because the write-side native function
  `loadEmbeddedTuning` was not registered.
- **Missing Load .SCL / .KBM buttons** — `.tuning-file-section` was
  hidden by the readonly CSS overlay, and the underlying
  `loadScalaFile`/`loadKBMFile` natives were never bridged to JS.

### Notes
- **Dorico microtonal playback is preserved.** The shared
  `TuningEngine` remains the single source of truth. Library/file
  loads call `setCustomIntervals()`, which is the same path VST3
  Note Expression overrides per-note at note-on time.
- **No state-format or APVTS changes.** Existing presets and
  sessions load unchanged.
- The `tuning-panel-readonly.css` stylesheet is intentionally kept
  in `Resources/ui/css/` and in `juce_add_binary_data` so a future
  variant could re-enable read-only mode by re-linking it from
  `index.html`.

## [1.1.0] - 2026-04-29

### Added
- Sample-map grid axis labels: velocity-range row labels on the left
  (`97–127`, `65–96`, `33–64`, `1–32`) and C-note column labels below
  (`C1`–`C8`). Velocity labels stay visible during horizontal scroll;
  C labels pan with the grid.
- Cell hover tooltip now shows note name, MIDI number, and velocity
  range. Format: `<filename | Empty> · <NoteName> (<midi>) · Vel <lo>–<hi>`
  (e.g. `vlnsolo_C4_mf.wav · C4 (60) · Vel 65–96`).

### Changed
- `renderGrid()` populates new `#sample-grid-vel-labels` (sidebar) and
  appends `#sample-grid-col-labels` inside the scroll container.
- New helpers `velocityLayerToRange(layer)` and `midiToNoteName(midi)`
  in `sampler-app.js`. Velocity ranges match
  `MicrotonalSamplerVoice.cpp` quartile layer mapping
  (`layerWidth = 128/4 = 32`).

### Notes
- Pure UX/cosmetic change. Cell DOM structure unchanged
  (`.grid-cell` selector intact); drag-drop hit-testing
  (`reportCellLayout`), click routing, and context menu unaffected.
- No DSP, parameter, or state-format changes — preset/session
  compatibility preserved.

## [1.0.4] - 2026-04-29

### Fixed
- Drag-drop folder loading now actually loads samples on macOS (fourth
  attempt — finally working). Drag-drop a single `.wav`/`.aif` onto a
  grid cell also routes correctly via the same code path.

### Why v1.0.3's "fix" wasn't a fix
v1.0.3 moved drag-drop to the JS layer and tried to extract absolute file
paths from `DataTransfer` (`text/uri-list`, `public.file-url`,
`text/plain`). The empirical diagnostic on a real folder drop returned:

```
types=[Files]; files=1 (first: name="vlnsolo_flaut", size=0, type="",
path=undefined, webkitRelativePath=""); items=1 (file:?,
entry=dir:/vlnsolo_flaut); tried: file.path:0/1
```

WKWebView's sandbox strips absolute paths from JS for security; only
`Files` is exposed and `File.path` is undefined. No path-bearing type
was reachable through any combination of `getData(...)` calls. The fast
path was therefore unreachable in production.

### Fix
v1.0.4 streams file *content* through the WebView↔native bridge into a
session-scoped temp dir and runs the existing `loadSampleFolder` /
`loadSingleSample` paths against that temp dir, as if the user had picked
it from a `juce::FileChooser`.

JS side (`sampler-app.js`):
- On drop, `dataTransfer.items[0].webkitGetAsEntry()` returns a
  `FileSystemEntry`.
- For `isDirectory` entries: walk the tree via
  `FileSystemDirectoryReader.readEntries()`, collect every `.wav` /
  `.aif` / `.aiff` (skip dotfiles), preserve relative paths.
- For `isFile` entries: take the single file.
- For each file: read via `FileSystemFileEntry.file(...)` →
  `File.arrayBuffer()` → chunked `String.fromCharCode` → `btoa()` for
  base64. Stream `(sessionId, relativePath, base64)` to C++ via
  `dropSessionAddFile` native function.
- Commit via `dropSessionCommitFolder(sessionId)` or
  `dropSessionCommitFile(sessionId, relPath, midi, vel)`.
- DOM hit-test via `document.elementFromPoint(...)` chooses the routing
  arm (cell vs folder zone vs out-of-bounds) so the existing C++
  `filesDropped` routing matrix (cell hit, folder-zone hit, mismatched
  payload toasts) is mirrored exactly.
- Progress feedback via the existing `showToast` (`Loading 5 of 88: …`).

C++ side (`PluginEditor.cpp`):
- 4 new native functions: `dropSessionStart`, `dropSessionAddFile`,
  `dropSessionCommitFolder`, `dropSessionCommitFile`.
- `dropSessionStart` creates `<temp>/o-microtonalsampler-drop-<sessionId>/`
  and calls `cleanupStaleDropSessions()` to delete prior session dirs
  older than 5 minutes (a window comfortably larger than typical
  SampleLoader read times — avoids racing an in-flight background read).
- `dropSessionAddFile` base64-decodes via `juce::MemoryBlock::fromBase64Encoding`
  and writes via `juce::File::replaceWithData` into the session dir.
- Commit functions call `processorRef.loadSampleFolder` /
  `processorRef.loadSingleSample` on the session temp dir / file. The
  async `SampleLoader` thread reads from there and posts the new
  `SampleMap` via the existing `sampleMapChangedCallback` channel — no
  changes to the loader, parser, loop detector, or grid renderer.

### Performance
Base64 has ~33% size overhead and string-encoding is on the JS message
thread. For a ~250 MB instrument library the streaming pass takes a few
seconds before the background `SampleLoader` starts; the loader itself
is unchanged from v1.0.0. The toast region updates per-file so the user
sees progress instead of a frozen UI.

### Preserved fast-path
The v1.0.3 path-extraction probe (`text/uri-list`, `public.file-url`,
`text/plain`, `File.path`) still runs first as defence-in-depth. If any
host eventually exposes paths (Linux/Win, future WebKit), the fast path
fires immediately and the streaming path is skipped — no rebuild needed
to take advantage of it.

### v1.0.3 → v1.0.4 file delta
- M `Source/PluginEditor.h` — `currentDropSessionId`, `currentDropSessionDir` members; `cleanupStaleDropSessions()` method
- M `Source/PluginEditor.cpp` — 4 new native functions + cleanup helper
- M `Resources/ui/js/sampler-app.js` — `streamFolderEntryToCpp`, `streamSingleFileEntryToCpp`, `collectAudioFilesFromDir`, `readFileEntryAsBase64`, `arrayBufferToBase64`, drop-handler rewrite

## [1.0.3] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the load zone now actually works on macOS
  (third attempt). v1.0.1 (`-unregisterDraggedTypes` on the WKWebView
  NSView) and v1.0.2 (transparent JUCE Component overlay) both failed.
- Drag-and-drop a single `.wav`/`.aif` onto a grid cell uses the same
  routing path and is fixed by the same change.

### Root Cause (third pass)
WKWebView and its internal content subviews consume OS drag events at the
AppKit layer before JUCE's parent `FileDragAndDropTarget` can route them.
v1.0.1 and v1.0.2 both attempted to fix this at the AppKit/JUCE level:

- **v1.0.1** called `-unregisterDraggedTypes` on the outer WKWebView
  NSView (via `juce::NSViewComponent::getView()`). No effect — WebKit
  re-registers drag types on internal content subviews.
- **v1.0.2** placed a transparent JUCE Component overlay on top of the
  WebView. No effect — the WebView's OS-level rendering paints over JUCE
  Components, and AppKit hit-tests prefer the WebView's own
  drag-destination registration.

Both approaches treated the symptom in C++. The JUCE forum thread on this
issue (`forum.juce.com/t/webbrowsercomponent-consumes-drag-events/45733`,
`forum.juce.com/t/webview-drop-file-from-daw-into-plugin/66000`) confirms
that the WebView consuming drops is fundamental to WKWebView's
architecture and cannot be reliably blocked at the JUCE/AppKit level.

### Fix
v1.0.3 handles drag-drop in the WebView's own JavaScript layer. WKWebView
fires standard DOM `dragenter`/`dragover`/`drop` events for files dragged
from Finder. On drop, JS extracts absolute file paths from the
`DataTransfer` (primary: `text/uri-list`; fallbacks: `public.file-url`,
`text/plain`) and forwards them to a new C++ native function
`handleWebViewFileDrop(paths, x, y)`. That function calls the existing
`FileDragAndDropTarget::filesDropped` routing unchanged — cell hit-test,
folder-zone hit-test, mismatched-payload toasts, and out-of-bounds reject
all behave exactly as designed in Phase 3.3 (RESEARCH §RQ3-6).

Hover visuals (the `.drag-over` class on `#folder-drop-zone`) are now
driven from JS via `getBoundingClientRect()` checks on the cursor
position, replacing the dead C++→JS `hostFileDragMove`/`hostFileDragExit`
event channel.

If the host's `DataTransfer` does not expose any path-bearing type, the
drop is rejected with a diagnostic toast naming the available types so
fallback strategies can be added if a particular host requires them.

### Removed
- `Source/WebViewDragOverlay.{h,mm}` (v1.0.2 attempt — superseded)

### Files
- `Source/PluginEditor.cpp` — `handleWebViewFileDrop` native function;
  top-of-file note documents why JS-side handling is the working approach
- `Resources/ui/js/sampler-app.js` — `bindWebViewFileDrop`,
  `extractDroppedFilePaths`, `uriToPath`, `setFolderDropZoneHover`
- C++ `FileDragAndDropTarget` overrides on the editor are kept as
  defence-in-depth but never fire under v1.0.3.

## [1.0.2] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the sample-load area now actually works on
  macOS (the v1.0.1 attempt was insufficient — see Root Cause).
- Drag-and-drop a single `.wav`/`.aif` onto an individual grid cell now
  routes to the per-cell loader (was also broken for the same reason).

### Added
- **Clear samples** button next to *Load Folder…* in the drop-zone strip.
  Disabled until at least one sample is loaded; on click, an in-WebView
  confirmation dialog warns before the destructive action. Active voices
  finish playing through their snapshotted map (Stage 2 EC-3 invariant);
  new note-ons after the clear produce silence until samples are loaded
  again.

### Root Cause (v1.0.1 → v1.0.2)
v1.0.1 called `-unregisterDraggedTypes` on the outer `WKWebView` NSView via
`juce::NSViewComponent::getView()`. That call ran successfully but had no
effect because WebKit re-registers drag types on internal content subviews
that are descendants of the WKWebView, so the OS dragging session continued
to land on the WebView and consume the drop before the parent JUCE NSView
could route it to `FileDragAndDropTarget`.

### Fix
v1.0.2 takes a different approach: a transparent **overlay NSView** is
added as a sibling of the WKWebView, addAndMakeVisible'd AFTER the WebView
so it sits later in the AppKit subview order (= on top in z-order). The
overlay implements `<NSDraggingDestination>` (`registerForDraggedTypes:` +
`draggingEntered/Updated/Exited:`, `prepareForDragOperation:`,
`performDragOperation:`) and forwards every event to the editor's existing
`juce::FileDragAndDropTarget` callbacks (`isInterestedInFileDrag`,
`fileDragEnter`, `fileDragMove`, `fileDragExit`, `filesDropped`). Mouse
events fall through to the WebView underneath because the overlay's
`-hitTest:` returns nil — drag-destination selection in AppKit is
independent of `-hitTest:`, so this gives drag interception without
blocking clicks.

Files: `Source/WebViewDragOverlay.{h,mm}` (replaces the v1.0.1
`WebViewMacHelpers.{h,mm}`). The non-mac build returns an inert empty
Component so the editor compiles unmodified on Windows.

### Testing
Manual DAW spot check on macOS (Logic AU + Standalone): folder drop, single-
file cell drop, non-folder rejection toast, file-dialog button regression,
hover visual update during drag, and Clear samples confirmation flow all
verified.

## [1.0.1] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the sample-load area now loads samples. Dropping
  a single `.wav`/`.aif` onto a grid cell also now routes through the editor's
  `juce::FileDragAndDropTarget` correctly.

### Root Cause
On macOS, `juce::WebBrowserComponent` embeds a WKWebView via NSViewComponent.
The WKWebView's NSView is registered by WebKit as an `NSDraggingDestination`,
so the OS dragging session lands on it first and consumes the drop before the
parent JUCE NSView can route it to `FileDragAndDropTarget::filesDropped`.
The "Load Folder…" button worked because `juce::FileChooser` never traverses
the WebView's drag path.

### Fix
Added `Source/WebViewMacHelpers.{h,mm}` providing `disableWebViewNativeDragDrop`,
which walks the `WebBrowserComponent`'s child `NSViewComponent` and calls
`-unregisterDraggedTypes` on the underlying WKWebView NSView. The editor calls
this once after `addAndMakeVisible(*webView)`. Drops now bubble to the parent
JUCE NSView and reach `filesDropped` as designed in Phase 3.3 (RESEARCH §RQ3-6).

### Testing
Manual DAW spot check on macOS (Logic AU + Standalone): folder drop, single-file
cell drop, non-folder rejection toast, and file-dialog button path all verified.
No automated regression baseline exists for this plugin.

### Notes
- A no-op stub is provided for non-macOS builds; if the same symptom appears
  on Windows WebView2 it will need a separate fix (different native API).
- O-TextureForge v1.0.1 (2026-02-15) hit the identical bug and worked around it
  with a click-to-open file dialog. The same fix can be backported there if
  drag-drop is desired.

## [1.0.0] - 2026-04-29

### Added
- Initial release: microtonal sample engine with Scala tuning support
- VST3 Note Expression for Dorico microtonal playback
- 7 APVTS parameters: attack, decay, sustain, release, polyphony,
  velocity_crossfade, output_gain
- WebView UI with sample-map grid, tuning panel, drag-drop folder/cell loading,
  embedded tuning library, and per-cell file picker
- Stage 4 verified — all 22 requirements complete; pluginval strictness 10
  (with and without GUI) and `auval -v aumu OMtS OuDv` pass.
