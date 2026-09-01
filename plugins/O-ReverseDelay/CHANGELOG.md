# Changelog — O-ReverseDelay

All notable changes to the O-ReverseDelay granular reverse delay.
Format loosely follows [Keep a Changelog]. **v1.0.0 is the first shipped product
version** — there is no earlier release track.

## [1.10.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

**No parameter, preset, state, DSP or layout change.** Nothing outside `js/i18n.js`
and this file moved. `check-ui-labels` reports the same **zero** non-label elements
displaced between English and French, and `tests/ui_tooltip_clamp_check.js` reports
byte-identical geometry for all 62 anchor measurements (31 anchors × 2 languages):
8 clamps and 2 below-flips per language, unchanged from Stage D.

### Changed

- **24 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and lint (`scripts/i18n-fr-lint.js`), taking the
  plugin from 29 findings to 0 at `--strict`. The visible ones:
  - **PENTE → INCLINAISON** for Tilt. *Pente* is the suite's French for a
    **rolloff**; the glossary settles Tilt as *Inclinaison*, and the tooltip on
    that same knob had said *Inclinaison* since v1.9.0 — one control, two French
    names. Measured, the root fits: 71.13 px in the WINDOW group's 66 px cell,
    clearing the Taper caption by 24.58 px and the panel's left edge by 19.44 px.
  - **AMPLEUR → PROFONDEUR** for Drift Depth, which also puts the caption, the
    tooltip title (*Profondeur de dérive*) and Drift Rate's body on one word.
  - **Adoucissement → Biseau** in the Taper tooltip title, so that control also
    has one French name; the body now reads *biseautée* where the English reads
    "tapered", and the Envelope display's body names the three real captions.
  - **Mixage → Mix**, **Direction → Sens**, **Atténuation dynamique → Ducking**
    and **Mode de synchronisation → Mode de synchro** in tooltip titles — all
    four are the settled suite terms, and *Ducking* now matches the caption the
    page has always shown.
  - The Sync-mode and Division bodies name **Libre** and **Synchro**, the page's
    own segment captions, where the drafts had left the English *Free* / *Sync*.
  - **French typography throughout**: no-break spaces before `:`, `;` and `?` and
    between a number and its unit (*7 dB*), on 17 strings.
- **`<html lang>` now follows the language selector** (canon change, all plugins),
  so assistive technology reads the page in the language it is displayed in.

### Not changed

`reviewed: false` stays `false` on all 69 entries — it records a **native
speaker's** reading, and this release is a second machine reading against a
glossary and a lint. No English copy, no key, no binding, no selector and no CSS
rule was touched.

## [1.10.0] — 2026-08-27 — the PAGE speaks French, not only the hover help

**No parameter, preset, state or DSP change.** Every knob, every range, every
default and every factory preset is bitwise what v1.9.0 shipped. The layout DID
change — see the measured table below — and every one of those changes applies
identically in both languages.

Fourth plugin on canon v2, after O-Tapestop v1.6.0, O-MultiBandCompressor
v1.11.0 and O-Bitrot v1.15.0. **D13 stands and was extended, not weakened:** the
settings popover still carries the language selector alone, there is still no
hover-help toggle and no `setTooltipsEnabled` anywhere.

### Added

- **35 label keys** in a new `LABELS` table in `js/i18n.js`, plus a reasoned
  `I18N_EXEMPT` for the two halves of the product name and the loaded preset
  name.
- **`data-i18n` on 50 elements and `data-i18n-aria` on 11.** The element owns its
  caption; the authored English stays in the markup as the fallback that renders
  if `applyI18n()` never runs.
- **`tests/i18n-states.json`** driving the FREE↔SYNC time-slot swap and the
  settings popover, which is what takes the label gate from 44 of 50 elements
  measured to **50 of 50**.

### Changed

- **Canon v1 → canon v2** in `js/app.js`, verbatim from `scripts/i18n-canon.js`.
  The block did not have to move: every binding on this page runs inside `init()`
  at the foot of the module, so nothing reaches the i18n bindings at
  module-evaluation time.
- **SIXTEEN label keys are REUSED from the tooltip table**, more than any other
  plugin in the suite, because this page's tooltip titles were authored as the
  captions they sit under — `knob-delayTime` (Delay / Délai), `knob-density`,
  `knob-lowCut`, `knob-highCut`, `knob-width`, `knob-mix`, `knob-jitter`,
  `knob-delayScatter`, `combo-grainShape`, `knob-direction`, `knob-regenMakeup`,
  `knob-diffusion`, `knob-drive`, `knob-feedback`, `sourceSegments` and
  `freezeSegments`. The reuse rule requires the string to be identical in BOTH
  languages, and it is in each of these.
- **The delete button's two faces became keys.** `data-label` / `data-confirm`
  were the right answer while the page was English-only — the copy stayed out of
  the JS, which is what `pattern_js_state_updater_overwrites_html_labels` asks
  for — and the wrong answer with two languages, because an attribute holds ONE
  string and a switch while the button was armed would have restored the ENGLISH
  armed face.
- **Two false sentences fixed, in both languages.** `lang-select`'s tip said "the
  labels on the page itself do not change"; the selector's own `aria-label` said
  "Hover help language" while the control now sets the language of the page.
- **`tests/ui_frontend_check.js`, two rewrites and one extension.** The delete
  button's copy assertion moved from "carries data-label and data-confirm" to
  three stronger checks: the markup declares the unarmed KEY, both faces are
  swapped through `setLabel()` with PLAIN STRING literals, and nothing writes
  that button's `textContent` directly. Separately, the v1.9.0 anchor-coverage
  assertion — the one that immediately found the COLOUR panel had shipped
  without `knob-diffusion` and `knob-drive` — was extended to LABELS: every
  `data-i18n` / `data-i18n-aria` key in the markup must resolve in `LABELS` or
  `I18N` (the same fallback `trLabel()` uses) and carry a title in BOTH
  languages. **A third assertion states D13 in its label form:** no `ui.on` /
  `ui.off` key may exist, so the toggle cannot be reintroduced through the label
  table. All three confirmed by negative control.

### Fixed — layout, from the measured English↔French geometry diff

`node scripts/check-ui-labels.js --plugin O-ReverseDelay` reported **7 non-label
elements moved** at the shipping 940 × 768 before any fix, and **zero** after.
D-04 forbids an auto-shrink font and a short-variant fallback.

| Moved | By | Cause | Fix |
|---|---|---|---|
| both preset-bar fleurons, both nav arrows and the name readout | dx ±10.4 | SAVE 56.8 → ENREG. 69.8, LOAD 59.8 → OUVRIR 72.3, DELETE 71.8 → SUPPR. 67.1, in a centred band | the three buttons pinned to 74 px |
| both footer fleurons | dx ±69.6 | the footer caption runs 403.5 → 542.7 in a `justify-content: center` row | `.footer-text { width: 560px }` |
| — (a clip, not a shift) | — | SYNCHRO is 54.8 px inside a 58 px segment's 54 px content box | `#syncSegments .segment { width: 66px }` — 132 px inside the group's pinned 190 |

**Three French captions were sized rather than the layout changed**, because the
three specimen rows share ONE pinned 190 \| 190 \| 276 \| 190 width contract that a
prior plan locked and French may not move:

- `Tilt` → **Pente** and `Taper` → **Biseau**, not the tooltip titles
  "Inclinaison" (71.1 px) and "Adoucissement" (91.9 px), because the WINDOW
  group's cells are 66 px where every other knob cell on the page is 72.
- `Depth` → **Ampleur**, not "Profondeur" (72.4 px in a 72 px cell — 0.4 px over,
  which is a clip rather than a near miss).

### Known, and left alone

- **The gate found the WINDOW captions as an OVERLAP, not as a clip**, and only
  when BOTH were over-long. `.knob-label` is a shrink-to-fit flex item, so its
  box is always exactly its text and the text-spill check can never fire on it;
  `offsetParent` is the frame rather than the cell, so the spill check cannot
  either. A SINGLE over-long caption in a two-cell row is therefore not caught
  by any assertion here — confirmed by negative control, which is why it is
  written down rather than assumed away.
- Twelve `.group-label` legends are `position: absolute` and deliberately
  overhang their panel by 9 px, and the decorative bird overlay runs 28 px past
  the frame. Both are identical in both languages; the gate reports them and
  does not assert on them.
- All 66 French entries (31 tooltip, 35 label) are machine drafts flagged
  `reviewed: false`. `Enreg.` / `Ouvrir` / `Suppr.`, `Pente`, `Biseau`,
  `Ampleur`, `Recouvr.` and `Ducking` were picked with width as a constraint and
  should be the first a native speaker challenges.

## [1.9.0] — 2026-08-26 — English/French hover help; the clamp gate now sweeps both languages

**No parameter, preset, state, DSP or layout change.** Every knob, every range,
every default and every factory preset is bitwise what v1.8.1 shipped. What
changed is where the hover-help copy LIVES, and the addition of a language it
can be read in.

### Hover-help copy moved out of the markup

All 29 tooltips left `index.html` and now live in a new `js/i18n.js` as a key
table, `{ key: { en: {t, b}, fr: {t, b, reviewed} } }`. **The English was moved,
not rewritten** — every `en` entry is byte-for-byte the string the markup
carried, verified by extracting both and comparing.

The renderer is UNCHANGED. `showTooltip()` still reads `data-tip-title` /
`data-tip` off the anchor; those two attributes are simply written at runtime by
`applyI18n()` instead of being authored in the HTML. There is no
`data-tip-key` attribute and the renderer never sees a key.

### A settings popover carrying a language selector

A gear button in the header opens a panel with one row: English / Français. The
choice re-renders every tip immediately, with no reload, and persists with the
session as a non-parameter property on the APVTS state tree — so a preset load
cannot change it, and a DAW automation lane never sees it.

**ONE ROW, and there must not be a second.** D13 scoped this plugin's hover help
to display-only: no on/off toggle, no persisted enabled flag, no
`setTooltipsEnabled`. Nine other plugins in the suite have that toggle;
O-ReverseDelay deliberately does not. The bridge goes 13 → 15 — the language
pair and nothing else.

The gear is absolutely positioned inside `.header` rather than added as a flex
child. `.header` is a plain `text-align: center` block, and making it a flex row
to seat a button would have re-laid out the centred title that every tooltip
clamp measurement in this plugin's history was taken against.

### All French is machine-drafted and UNREVIEWED

All 31 entries carry `reviewed: false`. No native speaker has read them.
`node scripts/check-i18n.js` prints the worklist. The terms most likely to want
a native speaker's judgement: `knob-duck` ("Atténuation dynamique"),
`knob-regenMakeup` ("Regain"), `knob-tukeyTaper` ("Adoucissement") and
`knob-delayScatter` ("Dispersion").

### Three gates rewritten, each with a negative control

`ui_tooltip_clamp_check.js` — **parameterised by language, not duplicated.** The
whole anchor sweep runs once for `en` and once for `fr`, in one process against
one page load, driven through `window.__setLanguage()`. Assertion 3 (vertical)
is the language-sensitive one: French wraps to more lines inside the unchanged
230 px cap, so tips get TALLER and the flip-to-below has to catch what no longer
fits above. `max-width` is now PARSED from this plugin's own CSS rather than
hard-coded, because the cap differs per plugin; the literal survives as the drift
guard. A third sweep pass opens the settings popover so `#lang-select` — a real
anchor in the top-right corner, where the clamp and the flip both bite hardest —
is measured rather than skipped. Two vacuity guards, both per-language: the
clamp must engage at least once, and every anchor's copy must actually differ
between `en` and `fr`.

`ui_frontend_check.js` section 14 — the anchor-coverage assertion **failed by
construction** once the copy left the markup, so it was rewritten into a stronger
form: every anchor is bound in `TIP_BINDINGS`, resolves to a key that exists, and
carries BOTH an `en` and an `fr` entry. The old form could only say "some string
is present"; this one fails on a missing translation, which the old one was
structurally incapable of noticing. It immediately found that the inventory had
drifted — v1.8.0's COLOUR panel shipped without adding `knob-diffusion` and
`knob-drive` to it, the second such drift in this file. Backfilled.

`ui_frontend_check.js` section 14, D13 — **tightened, not weakened.** The
assertion was a bare substring search for `setTooltipsEnabled` anywhere in
`app.js` or `PluginEditor.cpp`, which cannot tell REGISTERING the function from
writing a comment saying the function is deliberately absent. v1.9.0's comments
explaining the absence contain the literal name, so the old form failed on source
that honours D13 exactly. It now matches the two forms that would be a real
violation — a C++ `withNativeFunction` registration and a JS `getNativeFunction`
fetch — plus a new assertion that the processor holds no `tooltipsEnabled` state
at all. Confirmed by negative control: injecting a real registration fires it;
the comments do not.

### French geometry, MEASURED at the shipping 940 × 768

Every anchor hovered and measured in both languages:

| | anchors | clamped | flipped below | widest | tallest |
|---|---|---|---|---|---|
| `en` | 31 | 8 | 2 | 230.0 px | 133.9 px |
| `fr` | 31 | 8 | 2 | 230.0 px | 148.8 px |

**French costs zero extra flips and zero extra clamps on this frame.** It is
14.8 px taller at its tallest, and every tip in both languages sits fully inside
the viewport with an 8 px margin. `.tooltip { max-width }` was NOT touched.

### Version sentinels

The bump re-triggers both, and both are inert. `initializeFactoryPresets`
re-seeds on `.factory-version` with an untouched preset table, so the eight
factory presets are rewritten to identical contents. `migrateUserPresets`
re-stamps `.user-migration-version`; its rescale gates are `< 1.0.1` (delayTime)
and `< 1.5.0` (grainSize), and a preset written by v1.5.0–v1.8.1 packs above
both, so nothing is rescaled.

### Not verified

* **Nothing has been checked in a real DAW.** Everything above is headless
  Chromium against the repo's own ui-stub, `auval`, and the offline C++ harness.
* **The C++ persistence path was never executed.** Nothing wrote `"fr"` into a
  session and read it back. The claim "close the DAW, reopen, still French" is
  reasoned from `getStateAsXml()` taking its own `copyState()` and
  `setStateFromXml()` doing `replaceState`, not measured.
* **The native bridge was exercised only against the ui-stub.** The real
  `Juce.getNativeFunction('getUiLanguage')` round trip through WKWebView has not
  run.
* **No cross-platform check.** Windows/WebView2 has not been exercised.
* **The Standalone `.app` is stale** — `build-and-install.sh` builds VST3 + AU.

## [1.8.1] — 2026-08-10 — AGPL-3.0 notice headers; first published release

Patch release. **No parameter, preset, state, DSP or layout change** — every
source edit in it is a comment block. A v1.8.0 session, preset or factory patch
opens and renders identically.

**This is the first O-ReverseDelay release to actually reach GitHub Releases.**
The `v1.8.0` tag exists on both remotes but no release was ever built from it: it
was cut on 2026-07-26, before the repository moved to the public
`O-Audio-VST-Development` origin, and the 2026-08-03 batch that proved the
signing pipeline end-to-end covered O-Bells, O-Polystutter, O-IntonationPad,
O-Tremolo and O-Detune without picking this plugin up. So v1.8.1 is a patch in
version terms and a first publication in distribution terms, and the two should
not be conflated — nothing about the audio path changed between them.

The bump is not inert, because two sentinels key off `JucePlugin_VersionString`.
Both were checked rather than assumed:

* `initializeFactoryPresets` re-seeds on the `.factory-version` sentinel. The
  engineering-unit preset table is untouched, so the re-seed rewrites the eight
  factory presets to **identical** contents.
* `migrateUserPresets` re-stamps `.user-migration-version` and walks the user
  directory once. Its two rescale gates are `< 1.0.1` (delayTime) and `< 1.5.0`
  (grainSize); a preset written by v1.5.0–v1.8.0 packs to 10500 or higher and
  trips neither, so `changed` stays false and **no user preset file is
  rewritten**. Only pre-v1.5.0 files still on disk migrate, which is the
  behaviour they have been owed since v1.5.0 shipped.

### Changed

**AGPL-3.0 per-file notice headers (repo-wide sweep `cff295d9`).** 15 files under
`plugins/O-ReverseDelay/` — 6 `.h`, 3 `.cpp`, 4 `.js`, 1 `.css`, 1 `.html` — each
gain the FSF notice, the file's product subject, an `SPDX-License-Identifier` and
the warranty disclaimer. **285 insertions, 0 deletions**; the additions-only
count is asserted from `git show --numstat`, not inferred, because the sweep's
first pass silently rewrote a CRLF file whole via `Path.read_text` and a
zero-deletion assertion is what catches that class of damage.

Local `OuariconReverseDelay_VST3` build re-verified clean at 1.8.1 before
tagging.

## [1.8.0] — 2026-07-26 — COLOUR: diffusion and loop drive (B4 #7, #8)

Minor release. The last two items of section B4 of the v1.0.0 review, and with
them **the review's B section is complete**. `diffusion` (0–100 %, default 0) and
`drive` (0–100 %, default 0) fill the COLOUR panel v1.7.0 shipped framed and
empty. Both no-ops are the range minimum and both are exact, so a v1.0–v1.7
session, preset or factory patch renders **bit-identically** — asserted by probe
AZ rather than claimed.

**The reserve paid out, and it cost markup.** v1.7.0 framed this panel empty
during a resize it was making anyway, betting that filling it later would cost
markup instead of a second resize. It did: two `.knob-cell`s at 72 px with the
shared 14 px gap are 158 px inside the 190 px panel already declared — the shape
GRAIN and OUTPUT have had since Stage 3 — so **no width change, no height change
and not one CSS rule was added**. The editor stays 940 × 768 and the tooltip
clamp geometry under test is the one already verified.

Row 3 is now full and **nothing on the page is reserved**. `.group-reserved` is
deleted for the second time, on the same principle v1.6.0 deleted it: a rule kept
"in case" is dead CSS that still reads like a live layout decision.

Harness 138 → **151 probes**; `ui_frontend_check.js` 145 → **155 checks**;
`ui_tooltip_clamp_check.js` re-measured at 940 × 768 (**29/29 anchors**, clamp
firing for 6). auval SUCCEEDED; pluginval strictness-10 ×3 on both formats.

### Added

**`diffusion` — four Schroeder allpasses in the feedback return (B4 #7).**
Sections at 4.7 / 8.3 / 13.9 / 21.7 ms, per channel (a shared chain would fold L
and R through one state and collapse the tail to mono the moment it came up).
Placed **after** the damping filters and **before** the saturator: after damping
because the diffuser should blur what survives the loop rather than what is on
its way out of it, and before the saturator because the limiter must stay the
last thing in the loop — the invariant v1.6.0's makeup was placed around.

The knob is a **wet/dry mix over the chain, not a scaling of the allpass
coefficient**, and that distinction is what makes 0 a true no-op. Scaling the
coefficient toward zero leaves each section a pure N-sample *delay*, so "off"
would still splice ~48 ms of latency into the loop and the knob's first movement
would click. Mixing leaves the dry path at exactly `(1 − 0)·x`.

It also makes the stability argument exact rather than empirical. An allpass has
unit magnitude at every frequency, so the block's response is
`|(1−m) + m·e^{jφ}| ≤ (1−m) + m = 1` for every `m` and every `φ`. Diffusion is
**non-expansive by construction** — it cannot raise the loop gain at any
frequency, and so cannot open a self-oscillation path `feedback` alone would not.
That is why it carries no measured cap where `kRegenMakeupMaxDb` needed one.

**`drive` — the loop saturator, now level-compensated (B4 #8).** `tanh(x)`
becomes **`tanh(d·x)/d`**, `d` = 1 … 8 mapped exponentially from the percentage.

This is the one item the release was argued about before it was written, because
the obvious reading of "loop drive" is a second pre-gain into the tanh — and
`regenMakeup` already is one. Its own measured ladder says that control stops
doing anything past ~6 dB (+0.03 dB/s and 0.86 peak at 12 dB against +0.08 and
0.834 at 6), so a naive Drive would have shipped with an inert top half.

Dividing by `d` is what separates them. `d/dx tanh(d·x)` at `x=0` is exactly `d`,
so the small-signal gain is **1 at every setting**. Three consequences, and they
are the argument for the knob:

- **The decay rate does not move.** Probe BB measures 4.4515 / 4.4509 / 4.4464
  dB/s at drive 0 / 50 / 100 — a spread of 0.005 dB/s. `regenMakeup` fails that
  line by construction at any non-zero setting, which is precisely why it needed
  a cap and this does not.
- **The loop is bounded to ±1/d**, strictly tighter than the ±1 guaranteed since
  v1.0. Drive improves the safety property rather than spending it.
- **It does not plateau.** Level being compensated, more drive keeps adding
  harmonic content instead of asymptoting into a limiter: loud repeats compress
  and dull while quiet ones stay linear, so the tail *blooms* as it decays. That
  is the character `regenMakeup` cannot produce at any setting.

So `regenMakeup` answers *how long* and `drive` answers *what colour*. The knob
reads in **percent, not dB**, deliberately: a level-compensated control has no
gain to report, and two adjacent dB knobs doing unrelated things is a misread
waiting to happen.

### Fixed

**The non-finite guard now resets the allpass chain.** The chain is the one
v1.8.0 state that *recirculates* — each section feeds its own output back through
`g` — so a single non-finite sample written into its buffer would survive every
later block and reproduce itself for the life of the instance, exactly as an
envelope follower's state does (`pattern_envelope_follower_state_sticky_nan`).
Clearing the feedback source without clearing the allpasses would have left the
guard looking like it worked while the chain kept re-poisoning the loop.

### Notes

**The capture-ring `static_assert` is deliberately unchanged**, and the reflex is
to touch it. Diffusion adds ~48 ms of group delay to the feedback return, but
that bound is on the worst-case latched *read* span — how far back a grain
reaches from the write head — and the allpasses delay what is **written**, not
where anything reads from. A grain's read offset is `(gD + n)`, computed at spawn
from the parameters; no allpass appears in it.

**Diffusion makes the tail decay slower in dB/second, and that is not added
energy.** Two versions of probe BA failed on this before the cause was found, so
it is worth recording. The chain adds up to 48.6 ms of group delay per pass, so
at a 400 ms delay each generation takes ~12 % longer — fewer generations per
second is a slower decay per second with the loss *per generation* unchanged. The
measured ratio confirms it rather than merely fitting: 4.8053/5.2989 = **0.9068**
against the pure period prediction 400/448.6 = **0.8917**, the residual being
group delay falling below the full sum at the top of the band. Self-oscillation
depends on gain per trip around the loop, not per second, so the probe now
asserts **dB per generation** with the diffused loop credited the maximum group
delay the chain can physically produce: −2.1556 against −2.1196 undiffused.

**The eight factory presets are pinned at 0 for both controls.** "Reverse Bloom"
at diffusion 40 would be a better patch; it would also be a *different* patch
than the one shipped since v1.0.0, and re-voicing a sound people have built work
on is what this table has now refused six times
(`pattern_activating_dead_param_default_timbre`). A v1.8.0 preset that wants
diffusion is a new entry, not an edit to an existing one.

## [1.7.3] — 2026-07-25 — Info-tier review sweep (IN-01 … IN-06)

Patch release. Clears the six **Info** findings the v1.7.1 review left open and
v1.7.2 deferred. **No parameter, preset, state, DSP or layout change** — a v1.7.2
session, preset or factory patch opens and renders identically, and the render
harness's 145 probes are byte-for-byte unchanged.

Four of the six are comment and dead-code work. Two touch behaviour and were
gated accordingly: the user-preset migration sentinel now stamps before its walk,
and the cached parameter atomics are asserted once at construction instead of
guarded inconsistently at two call sites.

Harness **145 probes**, unchanged and all passing; `ui_frontend_check.js`
unchanged; `ui_tooltip_clamp_check.js` **27/27 anchors + 3 new WINDOW-panel
assertions**. auval SUCCEEDED, AU version verified as 1.7.3 (0x10703).

### Fixed

**IN-03 — the WINDOW panel's height budget was written down four times, and the
three copies that agreed with each other were the wrong ones.** `styles.css` held
two contradictory budgets 120 lines apart: one itemising an 80 px knob-cell for
214 px of content into a 213 px body ("already 1 px over"), the other a 78 px
knob-cell for 212 of 213 ("one px spare"). The 214 version had also been copied
into `styles.css`'s top-of-file block, into `PluginEditor.cpp:518` and into
`NOTES.md`.

Rendered and measured at the shipping viewport, the correct figures are
**44 + 78 + 72 + two 9 px gaps = 212 into a 213 px box — 1 px spare**. The 80 px
knob-cell was never real; it was *derived from the CSS rules* rather than
observed, which is the same failure mode that put a second budget in the file to
begin with. The code review that raised this finding recommended keeping the 80
version for the same reason, and was also wrong.

The budget now exists in exactly one comment and, more importantly, in three
assertions in `ui_tooltip_clamp_check.js` that measure the rendered rows. All
three were verified to FAIL by temporarily restoring the pre-v1.4.0 56 px knob
(content 222 into 213, −9 px spare), then restored bit-identically.

Worth recording: the first version of that assertion compared the content sum
against the body's own `clientHeight` and was **structurally incapable of
failing** — `.group-body` is `flex: 1 1 0%` with `min-height: auto`, so it grows
with its content instead of clipping, and `clientHeight` grew right along with
it (213 → 222, still "0 px spare"). That is
`pattern_flex1_container_slack_invisible_to_row_sum` rebuilt by accident while
fixing its twin. The shipped assertion measures against the *panel's* fixed
content box (245 less border and padding = 213), which is the only bound that
does not move with the thing it is bounding.

**IN-05 — the preset migration sentinel was check-then-write, so the race it
documented survived.** `migrateUserPresets()` read the sentinel, walked and
rewrote the user preset directory, and only then stamped the sentinel — from the
processor **constructor**. A host instantiating several instances in parallel on
the first launch after an upgrade had every one of them pass the check and
rewrite the same files. Benign in outcome (the transform is per-file
version-gated, so all writers produce identical content, and `replaceWithText`
goes via a temp file) but N× the intended message-thread file IO during
construction, which is where AU validation is timing-sensitive. The stamp now
happens before the walk.

Trade-off, accepted deliberately and documented in `NOTES.md`: an interrupted
migration is no longer retried, because the claim is already on disk. Recovery is
to delete the sentinel.

**IN-06 — `reset()` and `prepareToPlay()` disagreed about whether the cached
parameter atomics could be null.** `reset()` guarded every pointer; `prepareToPlay`
dereferenced the same ones bare. The disagreement picked the wrong direction: an
id typo in `createParameterLayout()` would make `reset()` a silent no-op and
`prepareToPlay` a release crash. All 25 cached pointers are now `jassert`-ed once
in the constructor and both functions dereference identically.

### Removed

**IN-01 — `ReverseGrain::age` was write-only dead state.** Zeroed at spawn,
incremented per grain per pass on the audio thread, read by nothing in `Source/`
or `tests/`. A leftover from the steal-oldest pool policy v1.1.0 replaced with
find-inactive round-robin. Field and both writes deleted.

**IN-04 — `envLastCurve` was assigned and never read, and the behaviour its
comment promised did not exist.** `envResize()`'s comment claimed it handled a
window dragged between a retina and a non-retina display; `envResize()` is only
reachable from `drawEnvelope()` ← `fetchEnvelope()`, which runs on a parameter
change or at init. The dead binding and the misleading half of the comment are
gone, and what remains states the real limitation. Wiring up a `matchMedia`
listener was considered and declined — it is new runtime behaviour, not a
comment fix, and does not belong in a patch sweep.

### Changed

**IN-02 — `PluginEditor.h`'s class contract was three releases stale and
contradicted the `.cpp`.** Every figure re-measured from source: the window is
940 × 768 (not 484), the native-function surface is **thirteen** (not eleven),
the relays are **20 sliders + 4 combos + 1 toggle = 25** (not "17 + 3"), the
parameter count is 25 (not 10), and decision D10's "no C++→JS polling bridge" was
reversed at v1.3.0 by the 15 Hz `getGrainMeter` poll. The header now says so.

### Notes

**One finding is only half-resolved, deliberately.** IN-05 names *both* preset
sentinels, but `.factory-version` is written by
`OuariconPresetManager::initializeFactoryPresets()`, which lives in the **shared
`preset-manager` module (v1.0.5)** rather than in this plugin. Reordering it
would change behaviour for every plugin in the suite and needs its own module
version bump plus a rollout to all dependents. Out of scope for a single-plugin
Info sweep; the plugin-local half (`migrateUserPresets`) is done and the module
half is **still open**.

## [1.7.2] — 2026-07-25 — code-review resolution (CR-01/02, WR-01–05)

Patch release. Resolves the seven defect findings of the v1.7.1 deep code review:
two Critical, five Warning. No parameter, preset, state or UI-layout change — a
v1.7.1 session, preset or factory patch opens and renders identically unless it
uses **delay drift** or **Freeze**, both of which are called out below.

Harness **138 → 145 probes**; `ui_frontend_check.js` **147 → 155 checks**;
`ui_tooltip_clamp_check.js` unchanged at 27/27 anchors with the clamp firing for
5. auval SUCCEEDED, AU version verified as 1.7.2 (0x10702).

Both Critical fixes were verified by reverting them against the new probes rather
than by inspection: CR-01's probe reports `max|512−4096| = 0.084275` on the old
code against `0.000000000000` on the new, and CR-02's reports a wet RMS of
`0.000000` against `0.061454`.

### Fixed

**CR-01 — delay drift with Scatter off read unwritten capture, and broke the
512-vs-4096 bit-identity.** The engine's sub-block pass bound was widened to `D`
whenever `delayScatter == 0`, on the v1.0.1 reasoning that "the only thing that
can pull a latched delay below `D` is scatter". v1.7.0's `driftMul()` broke that
premise: it multiplies `D` by as little as `1 − kDriftMaxFraction` = 0.75. At a
host block of 4096 with `driftDepth` 100 %, `delayScatter` 0 and `delayTime`
95 ms, `passLen` was 4096 while `gD` reached 3420, so a grain spawning at pass
offset 3800 latched `readAbs` **380 samples ahead of the capture write head** —
which step 6 does not write until after step 4 has rendered — and read one full
14 s ring lap of stale material. With `direction > 0` the whole grain is
corrupted rather than just its head, because a forward grain reads `t − gD` at
every sample of its life. Reachable at 48 kHz/4096 for `delayTime` ≈ 51–114 ms.
The pass bound now tests every parameter that can *shorten* a latched delay
(`scatterSamples > 0 || driftDepthNorm > 0`), which restores `passOffset < gD` at
every block size. `driftMul()` early-returns exactly `1.0f` at depth 0, so this
is bitwise inert at the shipped default, every factory preset and every
pre-v1.7.0 session — the four existing invariance probes still read
`0.000000000`.

Why 138 probes missed it: every drift probe ran with `delayScatter` at 80 ms or
at its maximum, and `scatterSamples > 0` is precisely the branch that selects the
*safe* bound. The one scatter-free drift probe ran at `block = 512`, where
`passLen` is two orders of magnitude below the smallest reachable `gD`. New probe
`drift-noscatter-blocksize-invariance` closes it.

**CR-02 — a session saved with Freeze engaged reopened with a permanently silent
wet path.** `prepareToPlay` jumped `freezeSmoothed` to the restored parameter
value so the session would "reopen frozen", but the capture ring is not part of
saved state and `capture.prepare()` had just cleared it. The first `processBlock`
then latched `jlimit(1, …, totalWritten == 0)` = **1**, and `pushLooped(1)` copies
the sample one slot back — zero — so every later sample copied the zero it had
just written. `freezeEngaged` was set unconditionally from the parameter on that
same block, so the latch never re-armed: the wet output was exactly zero for the
life of the session, with the dry path passing through, the FREEZE segment lit
and the grain meter showing grains active. It presented as "the plugin stopped
working", and the only recovery was to toggle Freeze off, wait for capture and
toggle it back on.

Three changes: `prepareToPlay` and `reset()` both start **un-frozen** (leaving
the edge detector armed); the latch refuses to arm until at least one grain
(`G` samples) has been captured, which also closes the sibling failure where
engaging Freeze in the first blocks after load produced a **240 Hz tone** rather
than a wash; and `freezeEngaged` now tracks whether the latch *succeeded* rather
than what the parameter says, so an early Freeze is deferred instead of lost. The
smoother's target follows `freezeEngaged` for the same reason — otherwise
`pushCrossfaded` would ramp `holdWeight` to 1.0 against the un-latched
`freezeLoopSamples == 1` and write the very zeros the guard refused to arm. A
session saved frozen still reopens frozen; it captures one grain first, which is
the only reading under which the control does anything at all.

New probes `freeze-restored-from-session-is-not-silent` and
`freeze-early-engage-is-not-a-tone`. The second is measured by **lagged
autocorrelation**, not crest factor: a short loop is a periodic *waveform* whose
crest is if anything lower than a cloud's, and the first draft of that probe read
crest 3.62 against a live 3.20 on the very code it was meant to reject and
passed. Autocorrelation separates them cleanly — 0.9936 broken against 0.5531
fixed.

**WR-01 — the render harness's `JucePlugin_VersionString` had drifted to 1.5.0
again.** Pinned at `"1.5.0"` while the plugin shipped 1.7.1, across three
releases. Both preset sentinels compare against this string, so the harness and
the installed plugin ping-pong `.factory-version` / `.user-migration-version`,
making every processor construction re-seed eight factory files and re-walk the
user preset library on the message thread — and any user preset the harness
rewrites is stamped with the stale version. This machine's
`.user-migration-version` was found reading `1.5.0`, confirming the mechanism.
The same file had already drifted once (1.2.0 across v1.3.0/v1.4.0) and carried a
comment saying the value was load-bearing, which is the point: "keep in sync
with" is a promise a comment cannot keep. Both values are now **derived** from
the plugin target's `JUCE_VERSION` property, with the hex code computed by JUCE's
own `(major << 16) + (minor << 8) + patch` and a `FATAL_ERROR` if the property
cannot be read. Three new `ui_frontend_check.js` assertions fail the build if a
literal ever comes back.

**WR-02 — `kMaxSpawnsPerBlock`'s derivation was unsound, and probe AB reproduced
the same error.** The bound assumed `passLen <= kDelayTimeMinMs·fs`, which only
holds when `delayScatter > 0`; with scatter at 0 — the shipped default and every
factory preset — the governing quantity is the **host block size**, which appeared
nowhere in the derivation. Probe AB reproduced the derivation verbatim and then
pinned `delayTime` to `kDelayTimeMinMs`, the single configuration in which the
false premise happens to be true. The real worst case is `delayTime` at its
*maximum* with `grainSize` at its minimum and ceiling 16: at 44.1 kHz the interval
is 137 samples, giving 30 nominal spawns at a 4096-sample block and **119 at
16384** against a cap of 128 — a claimed 8× margin that was really 1.07×, with
`droppedSpawns` asserted `== 0` resting on it. The cap is now the **hard** bound
rather than a probabilistic one, since jitter's floor is itself hard
(`nextInterval` returns `jmax(1, (int)(interval · mul))` with `mul >= 0.1`):
`16384 / 13 = 1261`, so **128 → 2048** (8 KB, one preallocated array). The
sensitivity has *inverted* — pressure comes from low sample rates and large
blocks, not high ones — so the two assumptions (`fs >= 44100`,
`hostBlock <= 16384`) are now written down and the second is `jassert`ed in
`prepareToPlay`. Probe AB gains a `longdelay` case at 512/4096/**16384**; all five
report `dropped=0`.

**WR-03 — cutoff smoothing was quantised to the host block.** `skip(numSamples)`
advanced the smoother by a whole block and one coefficient set was held for it, so
the documented 20 ms contract sampled itself at the host's block rate: a 20 ms
ramp is 960 samples at 48 kHz, so a 4096-sample block skipped the entire ramp in
**one step**. The cutoffs now advance on a fixed 32-sample grid driven by a
countdown that persists across blocks *and passes* — chunking relative to each
pass offset would have re-broken the invariant it fixes, since `grainDelayFloor`
is 2205 samples at 44.1 kHz and a second pass starting there shifts the grid at
4096 while 512 stays aligned. Bitwise inert for static cutoffs (once a
`SmoothedValue` is at target, 30 × `skip(32)` and 1 × `skip(960)` return the
identical float), which is every existing probe.

Measured honestly, and **less consequential than the finding implied**: rendered
both ways, the 512-vs-4096 divergence under a fast `highCut` sweep moves only
from 7.2 % to 6.7 % of wet RMS, and the click detector cannot separate the two
implementations at all (0.98× either way). The divergence is dominated by
`processBlock` reading each parameter **once per block** — all a host without
sample-accurate automation delivers — so a 512-sample block samples the
automation curve eight times more finely than a 4096-sample one and the two
follow genuinely different target sequences. Bit-identity under automation is
therefore not reachable in the DSP at all. This is a fidelity-to-contract
correction, and the overstated claim needed fixing regardless: **NOTES.md now
scopes the invariant to "bit-identical for static parameters"**, which is the
property the offline-bounce guarantee actually rests on. New probe
`cutoff-sweep-bounded-and-clickfree` records both numbers rather than leaving
them unmeasured.

**WR-04 — `getTailLengthSeconds()` was still 10 s.** Set at v1.0.0 when
`kDelayTimeMaxMs` was 2000 and `kGrainSizeMaxMs` 500, i.e. ~4× the longest
single-generation span. Since then `delayTime` went to 4000 ms, `grainSize` to
4000 ms, drift added +25 % and `regenMakeup` can push the loop into sustain — and
the `static_assert` on `kCaptureSeconds` puts one generation's read span at
**13.5 s**, already longer than the declared tail before any feedback. Hosts
honour this when deciding how far past the last input event to keep rendering an
offline bounce, so a long wash ended mid-decay while the same settings monitored
live decayed properly. Now derived as four generations from `kCaptureSeconds`
(~54 s) so a future range move carries it along, which is the actual failure being
fixed. Reported metadata only, no DSP change; an over-long tail costs offline
render time, an under-long one truncates audio.

**WR-05 — a lost `pointerup` stuck a knob to the cursor and left a host
automation gesture open.** Knob drags added `pointermove`/`pointerup` listeners to
`window` with no `setPointerCapture`, no `pointercancel` handler and no
`lostpointercapture` handler, and `onUp` was the only thing that cleared
`dragging` and called `sliderDragEnded()`. Any path that does not deliver that
`pointerup` — releasing over the DAW after dragging out of the plugin window, the
host taking a modal grab, the WebView losing focus mid-drag, the OS synthesising a
`pointercancel` — left both listeners attached, so every later mouse move over the
page kept writing `setNormalisedValue()` with **no button held**, and left
`sliderDragStarted()` unmatched, which latches automation write on that parameter
in Logic and Live. Both silent; nothing reaches the console. The drag now captures
the pointer on the knob itself and terminates on up, cancel and lost-capture, with
`try/catch` for older backends and an idempotent `onUp` that releases the capture.
Five new `ui_frontend_check.js` assertions, including that `pointermove` is no
longer bound to `window` — asserted structurally because both UI scripts dispatch
synthetic events that always deliver their `pointerup` and so cannot reach the
state.

### Notes

The six **Info** findings of the review (IN-01 … IN-06) were out of scope for this
release: `ReverseGrain::age` is write-only dead state; `PluginEditor.h`'s class
contract is three releases stale (940 × 484, "eleven" native functions, "17
sliders + 3 combos"); `styles.css` states two contradictory WINDOW height budgets
120 lines apart, with `NOTES.md` mirroring one of them; `envLastCurve` is assigned
and never read, so the DPR redraw its comment promises does not exist; the preset
sentinels are check-then-write, so the race they document is not closed; and
`prepareToPlay` dereferences the cached parameter atomics without the null guards
`reset()` applies.

**All six were swept in v1.7.3** — see that entry. One resolved differently from
the review's recommendation: IN-03's contradictory budgets were settled by
*measuring* the rendered panel, which showed the review had picked the wrong
comment.

## [1.7.1] — 2026-07-25 — 940 × 768

Patch release. **Editor 940 × 972 → 940 × 768.** No parameter, preset, state or
DSP change: the 138-probe render harness is untouched and passes, and a v1.7.0
session or preset opens bit-identical. Every one of the 204 px removed was empty.

### Fixed

**The window did not fit a 1080p screen.** At 972 px, plus a DAW's plugin header
strip and the menu bar above it, the frame ran to roughly 1035–1065 of the 1080
available — technically on-screen with the dock hidden, and off the bottom
otherwise. It is now 768, which clears the same chrome with ~247 px to spare.

The space came from three places, and the arithmetic is in `styles.css` rather
than summarised here:

| | v1.7.0 | v1.7.1 | content it holds |
|---|---|---|---|
| Row 1 — TIME / GRAIN / FEEDBACK / OUTPUT | 215 | **145** | 100 (the `.time-slot`) |
| Row 2 — RANDOM / WINDOW / COUNT / MOTION | 245 | **245** | 214 (WINDOW) — unchanged |
| Row 3 — SOURCE / DUCK / DRIFT / COLOUR | 215 | **145** | 94 (a knob-cell) |
| `.groups` slack around the rows | 93.5 | **29.5** | — |

Row 2 is deliberately untouched. Its WINDOW panel already lays 214 px of content
into a 213 px body — the one panel on the page with no slack — so trimming row 2
means shrinking the v1.4.0 envelope display. That is a feature change wearing a
layout change's clothes, and it was declined; the ~28 px it would have bought is
not worth the graph.

The 29.5 px left inside `.groups` is not leftover. The `.group-label` cartouches
sit at `top: -9px`, straddling each panel's top border, so row 1 needs ≥ 9 px of
clearance under the preset band's rule or the two collide. 14.75 px top and
bottom is that margin and nothing more, and `ui_frontend_check.js` now asserts
the band stays inside [18, 40] rather than trusting the comment.

### The finding

**`.groups` had 93.5 px of centred nothing, and five releases of comments proved
it did not.** `.groups` is `flex: 1`, so its height is the frame's content box
minus header, preset band and footer — never the row sum — and `justify-content:
center` centres the rows in whatever that leaves. At 972 that was 796.5 px
holding 703 px of rows.

Every comment from v1.1.0 to v1.7.0 asserted the opposite, in the same words:
*"743 + 229 = 972 and `.groups` still has EXACTLY zero slack."* Both sums are
sums **of rows**. Neither ever subtracted the row total from the frame height,
which is the only subtraction that could have found the slack — so the
arithmetic that was supposed to be the guard was structurally incapable of
firing, and each release re-derived it and passed it on. `ui_frontend_check.js`
mirrored the same sum, so the fixture agreed with the comment for the same
reason (`pattern_test_fixture_mirrors_drift_silently`).

Rows 1 and 3 are the same class of error at panel scale. Row 1 has been 215 px
since Stage 3, when its panels were the tallest thing on a 484 px page and the
height was reasonable; row 3 inherited 215 from row 1 at v1.7.0 on the correct
principle of not inventing new geometry. Neither number was ever measured
against what the panels actually hold, and by v1.7.0 that was 100 px and 94 px.

None of this is visible to `ninja`, `auval`, `pluginval` or a static check — an
over-tall frame renders perfectly. It was found by serving the real page through
`tests/ui-stub/` and measuring the boxes, which is now how the height budget in
`styles.css` is written: rendered values, not a paper sum.

### Verification

- `ui_frontend_check.js` — all pass, including three new/changed assertions: the
  chassis is 940 × 768 in the editor and both CSS spots, the row sum is
  145 + 14 + 245 + 14 + 145 = 563, and the frame is **≤ 900 px so it fits 1080p
  with host chrome** — the property this release exists for, asserted rather
  than left to a comment.
- `ui_tooltip_clamp_check.js` re-run at the new viewport — **27/27 anchors pass
  across both TIME modes**, and the horizontal edge clamp still engages for 5
  controls, so the verification is still live rather than passing by having room.
  This is the first resize in the plugin's history that made the page *shorter*,
  which inverts the vertical risk: not a tip flipping off the bottom, but row 1
  losing the clearance to place a tip above its knob. Both directions measured.
- 138-probe render harness — **all pass**, unchanged. `auval` — pass.
- Rendered in the real WKWebView (Standalone) as well as the stub browser.

### Changed

- `Source/PluginEditor.cpp` — `setSize (940, 972)` → `setSize (940, 768)`.
- `Source/ui/public/css/styles.css` — `html, body` and `.frame` heights;
  `.group` 215 → 145. The superseded "zero slack" note is kept in the history
  block with the correction under it, because the shape of the error is the
  reusable part.
- `tests/ui_frontend_check.js`, `tests/ui_tooltip_clamp_check.js` — geometry
  fixtures re-pinned to 768.

## [1.7.0] — 2026-07-25 — Source, Duck, Drift

Minor release. The last three parameters from section B4 of the v1.0.0 review —
ducking (#4), stereo source (#5) and delay drift (#6) — and the row-3 layout
decision that section D of the same review said they would force. All four new
parameters default to the engine's exact no-op, so **a v1.0–v1.6 session, preset
or factory patch is bit-identical under v1.7.0**, and the 138-probe render
harness asserts that rather than claiming it.

Three findings below, and none of them is the feature working as described. Two
are corrections to what this release's own probes first asserted, which is the
useful kind: the probe was wrong in a way that looked exactly like the engine
being wrong.

### Added

**Duck (`duck`, 0–100 %, default 0).** Attenuates the wet by the dry input's
envelope, so the wash blooms in the gaps rather than competing with the source —
the brief's "vocal ambience behind a lead" case. Applied to the **output path
only**, never to the feedback tap: a duck inside the loop would let the input's
envelope modulate the decay rate, so the knob would control how long the tail
lasts rather than when it is heard. That is the same output/loop split
`gainRandom` has used since v1.1.0, `loopTrim` since v1.2.0 and `forwardTrim`
since v1.6.0, applied to a fourth control.

Gain is `1 − depth · env/(env + 0.1)` — a smooth compressive map with no knee
parameter to explain, no discontinuity and no division by zero. Half depth
arrives at −20 dBFS, roughly where a mixed vocal or guitar sits, and full depth
is approached asymptotically so a loud source never quite mutes the wet.
Measured against an un-ducked render of the same excitation: **−10.10 dB** while
the dry is loud, **−1.08 dB** 700 ms into the gap.

**Source (`sourceMode`, Mono Sum / Stereo, default Mono Sum).** Through v1.6.0
the grain engine always read `CaptureBuffer::monoSum`, so the source's stereo
image was discarded before a grain ever saw it and `width` could only pan mono
copies of it — `CaptureBuffer::readAbs(ch, …)` existed and was called by nothing.
In Stereo mode a grain latches a channel at spawn, following **its own pan
side**: a grain that will be placed right reads the right channel. Hard-panned
source, width 100, measured L/R wet asymmetry: **0.637 in Stereo against 0.001
in Mono Sum.**

It follows `panSign` rather than the resolved pan *position*, and that is the
width-0 case rather than a detail. At width 0 every grain's pan is exactly 0.5,
so a position test would send every grain to the same channel and the mode would
silently become "mono, but only the left input".

**Drift (`driftRate` 0.02–5 Hz default 0.30, `driftDepth` 0–100 % default 0).**
A slow sine on the delay, sampled **at spawn** and latched with everything else,
so it is click-free by construction rather than by smoothing: a grain never
changes the delay it is reading from, the cloud's delay wanders. Multiplicative
rather than additive, so ±25 % at full depth reads the same at a 100 ms delay as
at a 4 s one — which is what tape does. Rate is inert while Depth is 0 and its
cell dims to say so, exactly as Taper's does off Tukey.

### Changed

**Capture ring 13.0 → 14.0 s.** Drift multiplies the latched delay, so
`gD_max` became `4000·1.25 + 500 = 5500 ms` and the requirement
`5.5 + 2·4.0 = 13.5 s`. 14.0 keeps the same 0.5 s margin every earlier release
held. Cost ~5.4 MB stereo at 48 kHz (~21.5 MB at 192 kHz), allocated once in
`prepareToPlay`. The `static_assert` added at v1.5.0 was extended rather than
re-reasoned about — and its parenthesisation is load-bearing, because drift
**multiplies** where scatter **adds**, so the two compose as `(D·drift + scatter)`
and not as a third additive term.

**Editor 940 × 743 → 940 × 972.** The reserve v1.1.0 framed held for five
releases and eleven controls without the window moving once; it is spent, so
this release pays the resize the review budgeted. Row 3 takes the **same**
190 | 190 | 276 | 190 contract, so the columns keep aligning, and one panel —
COLOUR — ships framed and empty for the review's remaining loop-character items
(#7 diffusion, #8 loop drive).

The **width did not change**, and that is the load-bearing half. The tooltip
edge-clamp gate is horizontal and only fires at the real shipping width, so a
height change leaves the clamp geometry under test intact where a width change
would have invalidated it outright
(`pattern_tooltip_clamp_gate_viewport_sensitive`). Re-measured anyway at
940 × 972: 27/27 anchors, clamp engaging for 5 of them, right-most tip ending at
932 of 940.

### The duck follower runs per sample, and the brief asked for per block

The requested design was a block-rate envelope: block RMS, one-pole with
coefficient `exp(−N/(τ·fs))`, gain ramped across the block, with a guard against
dividing by `numSamples <= 0`. It is cheaper and it is what most ambient delays
do. It is wrong **here**, for a reason specific to this plugin rather than to
ducking.

Probes O, W2 and AQ assert that a 512- and a 4096-sample render of the same
input are bit-identical, and three earlier releases spent real effort earning
that — the sub-block pass bound at v1.0.1, the split RNG streams at v1.1.0, the
draw-before-`obtain()` ordering at v1.6.0. A block-rate envelope breaks it by
construction, and not subtly: at 4096 samples the attack resolves to 85 ms
against 10.7 ms at 512, so an **offline bounce would duck audibly later than the
same session monitored**. That is the class of defect
`pattern_offline_render_asyncupdater_dynamics_gap` describes, reached from a
different direction.

What ships is a one-pole on `|dry|` advanced per sample, with its coefficients
computed once per block from `exp(−1/(τ·fs))`. It costs two mul-adds and one
divide per sample, runs inside the existing mix loop (the last place the dry
input is still readable, so there is no second pass and no copy), and removes
the question. There is no block RMS and therefore no division by `numSamples` to
guard — the guard the block-rate design needed is not weakened here, it is
absent because the quantity is. Probe AX asserts the invariant with ducking
engaged: `max|512 − 4096| = 0.000000000000`.

Probe M's click detector holds `duck` to the **smooth** tier rather than the
latched-content tier the other three new parameters get, which is the assertion:
a per-block gain step would fail that line and pass every other probe in the
suite.

### The follower is the first state here that could not heal, and it did not

The per-sample design has a consequence the block-rate one would have shared and
that neither the brief nor the first review of this release caught: `duckEnv` is
**persistent audio state with no recovery path**. The update is
`rect + c·(duckEnv − rect)`, which reproduces a NaN for any finite `rect` — so a
single bad input sample poisons the duck gain for the life of the instance. An
infinity reaches the same place by a second route, since `inf/(inf + knee)` is
NaN rather than 1.

This stood out precisely because nothing else in the engine behaves that way.
The capture ring ages a bad sample out after one lap; the feedback loop's
`isfinite` guard resets the damping filters within the pass. Measured before the
guard existed — a 10 ms NaN/inf burst at 1 s into a 30 s render, tail window
[20 s, 30 s] after the 14 s ring has fully lapped:

    duck  0 (the v1.0–v1.6 path)   finite, rms 0.0638   — fully recovered
    duck 80                        NaN                  — permanently

auval passed, pluginval strictness-10 passed three times on both formats, and
all 137 other probes passed, in that state. It took a probe that feeds
non-finite input deliberately.

Fixed with an `isfinite` reset to 0 — un-ducked, which is the safe direction,
since garbage in the envelope must not attenuate the wet. It costs one bit test
per sample in a loop that already evaluates `cos` and `sin`, and it cannot fire
for finite input, so the duck-0 bitwise identity and probe AX's block-size
invariance are both untouched. Probe AY is the standing guard, and it measures
duck 80 **against duck 0** rather than against an absolute: the shipped path's
recovery is the standard the new one has to meet. A probe asserting only "duck 80
recovers" would be satisfied by an engine that never went bad, and one asserting
"the output is finite throughout" would fail on the ring lap, which is correct
behaviour.

`processBlock`'s entry guard also moved from `numSamples == 0` to
`numSamples <= 0` (and the same for the channel count). It is defensive rather
than load-bearing — `getNumSamples()` should never be negative — but every loop
and division below keys off it, and the check is free.

### A correlated input does not imply a correlated capture ring

Probe AT asserts that with `L == R` the two source modes render **bit-identically**
— `0.5·(L+R)` is exactly `L` when the two agree, so the read laws coincide
sample-for-sample. Written with feedback at 40 and width at 60, it failed by
0.0154.

The engine was right. The identity is about the **capture ring**, not about the
input, and the ring is written with `input + feedback return` — where the
feedback return is the **width-panned wet**. So the moment anything recirculates
at width > 0, a perfectly correlated input stops implying a correlated ring and
the two read laws legitimately diverge. The probe now runs at feedback 0 (width
stays at 60, so the pan path is still exercised) and measures exactly 0.

### The mono fold's 0.7071 is unchanged, and the first probe of it was 3 dB wrong

The review asked whether the equal-power fold in `processBlock`'s mono branch
still holds once grains can read one channel instead of the sum. It does, and
the reason is sharper than "it still sounds right": **mono output is only
reachable with mono input** — the bus layout rejects stereo→mono outright — and
with a mono input the capture ring holds `L == R`, so the two source modes read
identical material by construction. The constant cannot depend on the mode
because the mode cannot reach it. Probe AU asserts that as bit-equality.

The fold's *value* was harder to test than to derive. The first version of the
probe compared mono-out RMS against the stereo render's **L channel** and
reported +3.010 dB — which reads exactly like a broken constant and is not. At
width 0 each channel carries `1/√2` of the grain sum, so one channel is always
3 dB under the pair. The reference is the stereo render's **total power**,
`√(rmsL² + rmsR²)`, which is the right one precisely because it is
fold-*independent*: it is the same number whatever constant the mono branch uses,
so the comparison tests the constant instead of restating it. Measured
**−0.000 dB**. A bare sum would read +3.01, a 0.5 average −3.01.

### Width 0 has never been bitwise dual-mono

Found by the same probe, and it belongs to v1.0.0 rather than to this release.
At width 0 the pan is exactly 0.5, so the gains are `cos(π/4)` and `sin(π/4)` —
mathematically equal, and **one ulp apart** as the library computes them. The
shipped width-0 wet has therefore always been dual-mono to within a pan-gain ulp
rather than bitwise, which is why probe K has always measured it with a
tolerance. Measured here at 1.1e-8 against a 0.107 peak, i.e. about −136 dB.
Probe AT's collapse assertion was rewritten to the meaningful form: Stereo at
width 0 must be no more asymmetric than Mono Sum is at width 0. Both measure
0.000000000.

### Drift's LFO phase comes from the spawn position, not an accumulator

The phase is `absPos · rate / fs`, where `absPos` is the capture ring's
monotonic write position at the moment the grain spawns — a quantity the engine
already computes. That makes drift block-size invariant *exactly*: the same
spawn lands at the same absolute sample at 512 and at 4096, so it sees the same
phase, where a per-block accumulator advanced eight times as often would land a
few ulps apart and probe AX demands bit equality.

The cost is that moving the **rate** knob re-derives the phase rather than
continuing it. On a latched-at-spawn parameter that is at worst one grain
landing at a different delay — inaudible against Scatter, which does the same
thing deliberately — and it buys an invariant the whole harness depends on. Both
drift parameters take the loose click tier in probe M for exactly that reason.

The ring clamp is a **guard, not a shaper**: the `static_assert` already
guarantees no reachable setting can ask for more than the ring holds, so it never
fires. It exists because the failure it prevents is silent — an over-reaching
read does not fault, does not produce a NaN and does not click; it wraps onto
overwritten material and the only symptom is that the long settings sound wrong.
Probe AV measures the observable instead of the engine's own belief: D and G both
at 4000 ms, scatter at max, drift at full depth, and the pre-arrival window at
**1e-7 of the arrival window**.

### Verification

- Render harness **122 → 138 probes**, all passing. Twelve new assertions —
  `duck-gap-bloom`, `duck-zero-is-noop`, `duck-depth-monotone`,
  `stereo-source-image` / `-correlated-identity` / `-width0-collapse`,
  `mono-fold-source-invariant`, `drift-ring-clamp`, `drift-zero-is-noop`,
  `drift-is-live`, `v170-blocksize-invariance`, `nonfinite-input-does-not-stick`
  — plus four new lines in probe M's sweep and four new columns in probe N's
  factory audit. `nonfinite-input-does-not-stick` is the first probe in this
  suite to feed deliberately pathological INPUT rather than to check that the
  engine's own output stays finite.
- `ui_frontend_check.js` **129 → 145 checks**, including a new **choice closure**
  (APVTS choice params ↔ `kComboIds` ↔ `getComboBoxState` in app.js, both
  directions). That gap is what v1.6.0's bool closure left open: through v1.6.0
  the choice ids were excluded from the knob closure by a hand-written list and
  then checked against nothing, so a choice parameter that never reached
  `kComboIds` would have been absent from every assertion. `sourceMode` is
  precisely that case.
- `ui_tooltip_clamp_check.js` re-measured at **940 × 972** — 27/27 anchors, clamp
  engaging for 5.
- Two more tooltip-inventory gaps backfilled: v1.6.0's `freezeSegments`,
  `knob-direction` and `knob-regenMakeup` were never added to the anchor list,
  which is the hand-maintained-fixture drift
  `pattern_test_fixture_mirrors_drift_silently` describes.
- auval SUCCEEDED; pluginval strictness-10 ×3 on VST3 and on AU, all SUCCESS.

## [1.6.0] — 2026-07-25 — Motion: Freeze, Direction, Regen

Minor release. The three high-payoff / low-effort parameters from section B4 of
the v1.0.0 review, filling the last reserved UI panel. Every one defaults to the
engine's exact no-op, so **a v1.0–v1.5 session, preset or factory patch is
bit-identical under v1.6.0** — the 122-probe render harness asserts that rather
than claiming it.

Two of the three turned out to have consequences the review did not anticipate.
Both are written up below, because in each case the obvious one-line
implementation is the wrong one and passes a casual listen.

### Added

**Freeze (`freeze`, bool, default off).** Stops writing new material into the
capture ring while the grains go on reading it, so the wash holds indefinitely.
Dry passes through untouched; the buffer resumes capturing where it left off on
release. Both transitions are a ~20 ms crossfade of the ring's *content*, which
also makes the loop's own seam a crossfade rather than a splice.

**Direction (`direction`, 0–100 %, default 0).** The probability that a grain is
latched to read *forward* instead of backward. At 0 every grain is reversed —
the shipped engine. At 100 the wet path becomes a clean delay tap. Between them
the cloud genuinely contains both read laws at once. Level is matched across the
whole range (see below).

**Regen Makeup (`regenMakeup`, 0–6 dB, default 0 dB).** The D11 feedback-tap
constant that was *declined* at v1.0.0 as a hidden number, shipped instead as a
user control. The topology loses ≈7.3 dB per generation at width 0 (−4.3 dB of
Hann-squared duty, −3.0 dB of the pan-to-mono-sum round trip), so "Near-Infinite"
at feedback 100 cannot in fact self-sustain — it decays, slowly. This makes true
sustain reachable without moving anybody's default.

### The three findings

**Freezing the write head produces a buzz, and freezing the writes produces
silence.** Grains latch `readAbs = totalWritten + offset − gD` at spawn, so with
the head stopped every grain spawned during the hold latches the *same* readAbs
and replays the same G samples — strictly periodic at the spawn interval, a
~28 Hz buzz at the shipped 200 ms grain. Advancing the head without writing fixes
that and then fails differently: the read sweeps over the whole 13 s ring
including however much of it was never written, so a freeze in the first 13 s of
a session falls silent mid-hold. Probe AP measured exactly that — rms at 10 s,
30 s and 60 s all `0.000000`. What ships instead keeps writing, but writes a copy
of the ring `freezeLoopSamples` back, latched on the rising edge to how much has
*actually* been captured. Unity-gain copy, so a hold cannot grow, decay or drift
however long it runs: 60 s frozen renders at 0.04 dB of drift.

**Forward grains sum coherently — the read law alone gives Direction a +7.3 dB
level jump.** At output time `t` a grain reads `2s − gD − t` reversed but
`t − gD` forward, i.e. *independent of its spawn sample*. Every forward grain in
flight therefore reads the same source sample, bit for bit, so the forward set
adds in amplitude (`N·m`) where the reverse set adds in power (`√(N·q)`). This is
also why the plugin is a *reverse* delay: the s-dependence that decorrelates the
grains is created by the read head moving opposite to the write head, and a
unit-rate forward read is a plain delay tap by construction. `WindowLut::
getForwardNorm` cancels the jump on the **output** path with a derived
`√(q_eff/N)/m_eff`; the feedback tap takes no direction trim, because
`getLoopNorm` already models the loop as a coherent sum. Measured spread across
the whole knob is **0.95 dB**, against 7.3 dB uncorrected. The residual is a
~0.9 dB sag at mid-travel and it is derived rather than overlooked — mixing a
coherent set with an incoherent one gives `q·[p² + p(1−p)·q/(N·m²) + (1−p)]`,
which is `q` at both ends and `0.82·q` at `p = 0.5`.

**Collision safety needed no new clamp, which is the opposite of the intuition.**
A forward read head moves *toward* the write head rather than away from it, so
the review flagged it as possibly needing its own headroom clamp. It does not: at
pass-relative index `k` a forward grain reads `passStartAbs − gD + k`, and the
v1.0.1 A2 pass bound already guarantees `k < passLen ≤ grainDelayFloor ≤ gD`. The
read is strictly behind the write head at every `k`, at every host block size,
and the ring span a forward grain needs is `gD + G` — *smaller* than the reverse
case's `gD + 2·G` that `kCaptureSeconds` is sized for. Probe AM proves it in
audio: correlation between the wet output and the input delayed by exactly D is
**1.0000** at direction 100 and 0.0038 at direction 0.

### The regen ceiling is a measured number

Probe AO renders a 1 dB ladder at feedback 100, width 0 (the loop's worst case):

| makeup | peak | decay dB/s |
|--------|------|-----------|
| 0 dB | 0.262 | −2.09 |
| 1 dB | 0.275 | −0.65 |
| 2 dB | 0.445 | **+0.53** |
| 4 dB | 0.760 | +0.26 |
| 6 dB | 0.834 | +0.08 |
| 12 dB | 0.860 | +0.03 |

6 dB is where two things meet: sustain arrives at 2 dB here and needs ~3 dB more
at width 100, so 6 dB clears every configuration with margin; and past ~6 dB the
control stops doing anything, because the tanh is already limiting and 12 dB buys
0.026 dB/s over 6 dB. A knob whose top half is inert is a worse control than a
shorter one.

**What the cap does not guarantee, stated plainly.** The tanh bounds the *loop*
to ±1 at every setting. It does not bound the wet *output*, which sums `overlap`
grains reading self-similar limited content and approaches
`√overlap · mean · windowNorm` — 1.41 for Hann at overlap 8, 1.55 for Tukey at
overlap 16. That is the same shortfall behind v1.3.0's 1.28 peak, which
`loopCountTrim` fixed by *preventing* self-oscillation rather than by bounding
the sum. A cap that held peak < 1.0 everywhere would be ~1 dB, which reaches
sustain nowhere. So the peak < 1.0 invariant the rest of the suite asserts
belongs to the non-self-oscillating engine — regen 0 dB, i.e. the default, every
factory preset and every pre-v1.6.0 session. Above it the plugin does what a
self-oscillating delay does, and probe AO still requires finite, convergent, and
under a hard 1.8 runaway tripwire.

Direction also costs up to **1.09 dB/s** of decay rate at feedback 100, and that
is topology rather than a missing trim: at direction 100 the loop *is* a plain
feedback delay. Every rate stays negative. The bound is the 1.20 dB/s the suite
already accepts for window shape.

### Changed

- **UI:** the reserved SPACE panel becomes **MOTION**, holding all three
  controls. Markup, one width alias and two scoped rules — no panel width,
  position or height moved, so the 940 × 743 frame and the tooltip clamp geometry
  are untouched. The chassis framed at v1.1.0 is now full: the next new control
  is the row-3 / MORE-page decision from the review's section D, which genuinely
  arrives rather than being deferred a sixth time.
- **Editor:** first `WebToggleButtonRelay` in the plugin. `freeze` is the only
  `AudioParameterBool`, and a bool bound through a slider relay attaches without
  complaint and produces a switch that never updates — `ui_frontend_check.js` now
  closes APVTS bools against `kToggleIds` in both directions.
- **Factory presets:** all eight carry the three new keys pinned at 0.
  "Near-Infinite" is deliberately *not* re-authored to self-sustain, even though
  that is the review's stated motivation for the control — it is a shipped sound
  people have used since v1.0.0, and true sustain is now one knob away
  (`pattern_activating_dead_param_default_timbre`).

### Fixed (test fixtures)

- `ui_tooltip_clamp_check.js` asserted a hardcoded `boundReadouts === 15`. It had
  failed for the same non-reason three releases running — a knob was added and
  the fixture still described the release before it. The count is now parsed from
  `KNOB_IDS` (`pattern_test_fixture_mirrors_drift_silently`).
- `ui_frontend_check.js`'s new bool-binding check flagged app.js's own header
  comment, which explains the trap in prose containing the literal
  `getSliderState("freeze")`. Comments are now stripped before the binding
  regexes run — the same fix the `.group-motion` selector check already carries
  for CSS.

### Verification

- Render harness: **122 probes, 0 failures** (up from 108; AM, AN, AO, AP, AQ
  added). Every pre-v1.6.0 probe reports the number it reported at v1.5.0.
- `ui_frontend_check.js`: 129 checks, all pass.
- `ui_tooltip_clamp_check.js` re-run at the shipping 940 px viewport: 23 anchors,
  clamp engaged for 5 controls. `#knob-regenMakeup` is the new right-most control
  and clamps to exactly x=932 at its full 230 px width — the shrink-to-fit edge
  case (`pattern_fixed_tooltip_shrink_to_fit_edge`), verified rather than assumed.
- Page rendered against the JUCE-bridge stub: MOTION reads `Reverse` / `0.0 dB`,
  OFF is lit, all 17 knob readouts populated, no console errors.
- `auval -v aufx ORvD OuDv`: PASS. pluginval strictness 10: SUCCESS ×3.

## [1.5.0] — 2026-07-25 — Grain Size to 4000 ms

Minor release. One requested change — `grainSize`'s ceiling raised from **500 ms
to 4000 ms** — plus the two things that change forces and one latent test-harness
defect it exposed.

### `grainSize` — 50 to 4000 ms, default 200, skew centred on 316 ms

The knob now spans the **same 50–4000 ms as `delayTime`, on the same taper**.
Sharing `delayTime`'s 316 ms skew centre is deliberate: the two long-throw time
knobs sit next to each other on the panel, so a given knob angle now reads as
roughly the same duration on both.

The default stays **200 ms** and every shipped sound is unchanged — a new
instance renders identically to v1.4.0.

Measured off the rendered UI, not derived:

| knob | grain size |
|------|-----------|
| 0 % | 50 ms |
| 25 % | 68 ms |
| 50 % | 316 ms |
| 75 % | 1339 ms |
| 100 % | 4000 ms |

The taper is steep at the bottom — the old 50–500 ms working range now lives in
roughly the lower 55 % of the throw, and the top quarter buys 1339 → 4000 ms.
That is the cost of reaching 4 s on one knob, and it is exactly the cost
`delayTime` has always had.

### The capture ring had to grow — 6.0 s to 13.0 s

This is the load-bearing half of the change, and it is not optional.

A grain spawned at output sample `s` reads source `(s − gD − n)` at its own
sample `n`, so its **last** read lands at `(s − gD − G)` while the write head has
itself advanced to `(s + G)`. The ring must therefore span `gD_max + 2·G_max`,
not `gD_max + G_max`:

```
gD_max = kDelayTimeMaxMs + kDelayScatterMaxMs = 4.0 + 0.5 =  4.5 s
G_max  = kGrainSizeMaxMs                                  =  4.0 s   (was 0.5)
      -> 4.5 + 2·4.0                                      = 12.5 s required
```

Shipping the wider range against the old 6.0 s ring would **not** have faulted.
Long grains would simply have wrapped onto material the writer had already
overwritten — no NaN, no discontinuity (the ring is contiguous), every existing
probe still green, and the only symptom "the long settings sound a bit crunchy".

Cost: **~5.0 MB** stereo at 48 kHz (was ~2.3 MB), ~10 MB at 96 kHz, ~20 MB at
192 kHz. Allocated once in `prepareToPlay()`, never on the audio thread.

**The invariant is now a `static_assert`, not a comment.** Every prose statement
of this requirement was already correct at v1.4.0 and none of them stopped this
release from silently invalidating it — comments do not fail the build. Any
future move of `kDelayTimeMaxMs`, `kDelayScatterMaxMs` or `kGrainSizeMaxMs` that
outgrows the ring now stops the compiler.

### User presets are migrated; sessions need nothing

Two storage formats, opposite treatment — the same split v1.0.1 documented:

- **Sessions** (APVTS) store *denormalised* milliseconds. A session saved at
  350 ms recalls 350 ms under the new range with no migration, and rescaling one
  would actively corrupt it. Untouched.
- **Preset JSON** stores *normalised fractions*, which shift meaning when the
  range moves. A v1.4.0 preset at the old default 200 ms holds ~0.573 — which
  under the new curve would read back as **~1450 ms**.

`migrateUserPresets()` gains a `grainSize` arm. The two arms carry **different
version gates**, which is the part that was easy to get wrong:

| parameter | moved at | gate |
|-----------|----------|------|
| `delayTime` | v1.0.1 | `version < 1.0.1` |
| `grainSize` | v1.5.0 | `version < 1.5.0` |

Reusing the existing `!= "1.0.0"` gate would have migrated v1.0.0 presets and
silently left every v1.1–v1.4 preset — the bulk of any real library — holding a
fraction against the old curve. The failure is quiet: the preset still loads, it
just recalls the wrong grain size.

`grainSize` is also the harder rescale of the two. `delayTime` kept its skew
centre and moved only its max; `grainSize` moved **both** (max 500 → 4000, centre
158 → 316), so the curves differ in shape as well as extent and no scale factor
does the job — only reconstructing the old range and round-tripping through
milliseconds. The gate is per-file, so migration is idempotent even if a previous
pass was interrupted before the sentinel was written.

Factory presets need no edits: they are authored in engineering units and
re-converted through the new range on the `.factory-version` bump. All eight
re-seed and recall at `worst=0.0000` tolerance.

### Fixed — render-harness version string had drifted two releases

`tests/render-harness/CMakeLists.txt` pinned `JucePlugin_VersionString="1.2.0"`
while the plugin shipped 1.3.0 and 1.4.0. The file's own comment warns that this
value is load-bearing rather than cosmetic — both the factory-preset and
user-preset sentinels key off it — so probes N and R spent two releases auditing
v1.2.0's stale on-disk presets. Now 1.5.0, and the re-seed is visible in the run.

### Verified

All **108** render-harness probes pass, including:

- **`ring-cover-maxgrain`** (new) — at `D = G = 4000 ms` the reversed burst is
  absent before 4 s and present from 5–9 s, ratio `8.6e-8`. This is the assertion
  an undersized ring fails: a 6 s ring would wrap `(s − 8 s)` forward onto recent
  material and leak the burst into the early window.
- **`grainsize-preset-migration`** (new) — worst recall error `0.000061 ms`
  against a 0.01 ms parameter step, and the un-migrated drift is at least 9.0 ms
  (70 ms would recall 61.0 ms), so the probe has teeth rather than passing on a
  migration that did nothing.
- `blocksize-invariance` and `scatter-blocksize-invariance` — still bit-identical
  (`max|512−4096| = 0.000000000`), with W1 now driving `grainSize` at the new
  4000 ms maximum.
- `count-default-identity`, `window-default-identity`, `window-loopnorm-identity`
  — all still exactly `0.000000000`.
- `decay-count-fb100` / `ceiling16-loop-bounded` — decay still negative and
  monotone; the raised range does not disturb the v1.3.0 loop trim.
- Full-range parameter sweep now covers 50–4000 ms with no click or NaN.

### Not affected

Recorded because each looks adjacent and is not:

- `GrainScheduler::kMaxSpawnsPerBlock` — its bound is
  `overlapMax · kDelayTimeMinMs / kGrainSizeMinMs`, which keys off grainSize's
  **minimum**. Unmoved, so the cap's 8× margin is intact.
- `GrainPool`'s 32 slots — a grain lives `G` samples and the spawn interval is
  `G/overlap`, so concurrent grains ≈ overlap regardless of `G`.
- `loopCountTrim` — a function of overlap only.
- `ReverseGrain` — latches an `int G`; it owns no buffer to resize.
- `sizeRandom` — already clamps to `kGrainSizeMaxMs`, so it follows the new
  ceiling automatically.
- The WebView readout — reads `SliderState.getScaledValue()`, so it tracks the
  C++ range with no JS change.

## [1.4.0] — 2026-07-25 — Continuous Tukey taper + window-shape display

Minor release. Two requested changes: unfreeze Tukey's shape parameter, and show
the windowing function on screen.

### The range, corrected

The request was for a range of 0.01–9.9. Tukey's shape parameter is its **taper
fraction α**, hard-coded at `0.5` in `WindowLut.h` since v1.2.0, and it is
mathematically bounded to **[0, 1]** — 0 is the rectangular window, 1 is exactly
Hann, and there is nothing above 1 to reach. A 9.9 maximum would have clamped
from 1.0 upward, leaving roughly 90 % of the knob's travel rendering an identical
window. Confirmed with the user and shipped as **[0.01, 1.00]**.

### `tukeyTaper` — 0.01 to 1.00, step 0.01, default 0.50

- **0.01** — very nearly rectangular. Fast grain edges, an open/gated character.
- **0.50** — the shipped window. The default, so nothing existing changes.
- **1.00** — exactly Hann, reached rather than approached.

Rendered with **no new table and no new transcendental in the grain loop**,
because Tukey's taper is literally a Hann half. With `taperEnd = α/2`:

```
Tukey_taper(φ) = 0.5(1 − cos(πφ / taperEnd))
Hann(x)        = 0.5(1 − cos(2πx))
            ->   Tukey_taper(φ) = Hann(φ / (2·taperEnd))
```

so the whole window is one phase remap into the existing Hann table, saturating
at the flat top (`Hann(0.5)` is exactly 1.0f, so a grain's plateau is a true
unity plateau):

```
u = min(φ, 1 − φ)                 distance to the nearest edge
r = min(u / taperEnd, 1) · 0.5    [0, 0.5], flat top at exactly 0.5
w = Hann_table(r)
```

Continuous in α, zero extra memory, and one per-grain flag rather than a
quantised bank of tables.

**The step is load-bearing.** α changes the window's duty cycles, so the level
and feedback normalisations must track it — and `WindowLut`'s rule is that those
constants are integrated from the real window, never hand-derived. Integrating
2048 points per block on the audio thread is not available, so the stats are
precomputed per α in the constructor; a 0.01 step over [0.01, 1.00] makes that a
100-entry grid on which **every reachable α lands exactly**, so the stats are
exact rather than interpolated and the α = 0.5 entry reproduces v1.3.0's
constants bitwise. `taper-default-grid-exact` asserts that.

### ⚠ Not bitwise for Tukey — measured, and confined

The remap deviates from v1.3.0's stored Tukey table by up to **2.4e-6
(−112.5 dB)**. That is the Hann table's linear-interpolation error, not a change
of shape, and it cannot be avoided: reading a 2048-point table at an arbitrary
phase is not the same operation as evaluating `cos` there.

Stated properly rather than waved at — 2.4e-6 is roughly **20× a 24-bit LSB**, so
it is not "below the noise floor" as an absolute envelope error. What makes it
inaudible is *where* it occurs: the worst case is at φ ≈ 0.999, at the very end of
the taper where the window value is itself almost zero, so the error multiplies a
sample being faded out. Against a typical source level it lands near −124 dB.

The blast radius is Tukey only, and the cross-version diff confirms it rather
than asserting it: of 93 shared probes, **92 are byte-for-byte identical** and
the one that moved is `window-live-Tukey` (0.075537 → 0.075538). Hann, Gaussian,
Triangular and Expo-Decay never take this path, all eight factory presets are on
Hann, and a default session is bitwise unchanged. At α = 1.0 the remap lands on
the table's own points and the deviation drops to 4.2e-7.

### α needed two normalisation corrections, not one

Third release running where the output and feedback paths required *different*
constants for the same control — after shape (v1.2.0) and overlap (v1.3.0). The
split the engine has carried since v1.1.0 earns its keep again:

| | duty at α=0.01 | at α=1.0 | swing |
|---|---|---|---|
| Power (output path) | 0.994 | 0.375 | 4.2 dB |
| Amplitude (loop path) | 0.995 | 0.500 | 6.0 dB |

An α-aware output norm alone would have left ~1.8 dB of per-generation error in
the loop — "taper" heard as "tail length". Results:

| Measurement | Result |
|---|---|
| Wet level across α (probe AJ) | **0.010 dB** spread |
| Decay at fb 60, α ≥ 0.1 (AK) | **0.030 dB/s** vs default |
| Decay at fb 100, α ≥ 0.1 (AK) | **0.056 dB/s** vs default |
| Tilt power-invariance, all 100 α × 5 tilts | **exactly 1.0f** |

**α = 0.01 is a documented exception**, not a regression: 0.240 dB/s at fb 60 and
0.791 at fb 100. A near-rectangular window has crest factor ~1.0 against Hann's
1.63 and overlaps to something close to a constant; neither is removable by a
linear duty constant, which is the same statement `WindowLut.h` already makes
about Expo-Decay. It is bounded and printed separately rather than excused.

On the click risk the low end implies — the engine removed a 2 % Gaussian
pedestal at v1.2.0 for exactly this reason — the answer is measured and reassuring:
at the 50 ms grain minimum the grain-edge first difference is **0.0112 at α = 0.01
against 0.0058 at α = 0.5**, i.e. twice as fast and nowhere near the 0.25 click
threshold. A fast edge is the point of the low end, not a defect.

### The window display, inside the WINDOW panel

It draws the live envelope with shape, tilt and taper composed, a dashed midpoint
guide so tilt is legible, and a filled area so a near-rectangular taper reads as
"more window" at a glance.

It sits **inside the WINDOW panel**, beneath the three controls that shape it, so
the panel reads top-to-bottom as Shape → Tilt/Taper → the resulting window — cause
then effect, the same ordering the panel already used for Shape over Tilt. (It
was first built as a panel of its own in the reserved SPACE slot; putting it with
its controls reads better and leaves that slot reserved, so the row-3 / MORE-page
decision is still one release away rather than due now.)

Fitting it cost ~73 px the panel did not have spare, bought back by shrinking that
panel's own controls: knobs 56→46 px, select padding 7→4 px, gaps 12→9 and 7→5.
**Every one of those rules is scoped to `.group-window`** — `.knob`, `.knob-cell`,
`.select-cell` and `.division-select` are shared by all eight panels, so an
unscoped edit would have resized the whole interface while looking correct in the
one screenshot anyone checks. The frontend check now asserts the scoping.

The height budget is measured, not estimated, and lands with 1 px spare in a
158 × 213 body — the same zero-slack discipline the row geometry uses:

```
select-cell   28 + 6 + 10           =  44
knob-cell     46 + 5 + 10 + 5 + 12  =  78
env-cell      6 + 58 + 6 + 2 border =  72
2 row gaps at 9                     =  18
                              total =  212 of 213
```

The knob-stem is scaled with the knob (24→20 px). It is the one part of the knob
that is a fixed pixel height rather than a percentage gradient, so leaving it
would have given the smaller dial a pointer that overshot its own edge.

The curve is **fetched from C++** (`getWindowCurve`, 128 points) rather than
recomputed in JavaScript, and that is the design rather than an implementation
detail: a JS copy of the window would be a second definition free to drift from
the first, and a graph has no units to reveal it when it does — unlike a knob
readout, which is why the same rule already keeps readouts on `getScaledValue()`.
It is pulled on change (coalesced at 40 ms), not polled.

Verified by canvas hashing rather than by eye, which produced the nicest result
in this release: rendering Hann, Tukey α=0.50, α=0.01, α=1.00 and Expo-Decay
gives **4 distinct canvases from 5 renders** — and the single collision is
Tukey α=1.00 against Hann, byte-identical, because α=1 *is* Hann. The display
proves the mathematical identity visually.

`tukeyTaper` is **inert unless Tukey is selected**: the cell dims and sets
`aria-disabled`, but stays relay-bound and adjustable, so a value set beforehand
is honoured. Hiding it would make the panel jump as Shape changes.

### Row 2 still has a reserved slot

RANDOM | WINDOW | COUNT | SPACE — unchanged from v1.3.0. Because the display went
inside WINDOW rather than taking SPACE, the chassis v1.1.0 sized for v1.2–v1.6
keeps one free panel, and the row-3 / MORE-page decision (v1.0.0 review, section
D) is still ahead rather than forced now. No panel width, position or height
changed, so the tooltip edge-clamp geometry is the geometry v1.3.0 verified.

### Verification

| Check | Result |
|-------|--------|
| v1.3.0's shared probe result lines vs v1.4.0's | **92 of 93 byte-for-byte identical** (the one delta is the documented Tukey remap) |
| Offline render harness | **106/106 probes PASS, exit 0** (93 + 13 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 20/20 anchors, clamp fired on 4, 15/15 knobs bound |
| WINDOW panel height budget | **212 of 213 px**, `scrollHeight == clientHeight` (no overflow) |
| Envelope display, canvas-hashed | **4 distinct renders / 5**, collision = Tukey α=1 ≡ Hann |
| `pluginval --strictness-level 10` VST3 | **exit 0 ×3** |
| `pluginval --strictness-level 10` AU | **exit 0 ×3** |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **66560** (= 1.4.0) |

New probes: `taper-remap-is-tukey`, `taper-alpha1-is-hann`,
`taper-duty-closed-form`, `taper-default-grid-exact`,
`taper-tilt-power-invariant`, `level-flat-taper`, `decay-taper-fb60/fb100`,
`taper-live-1/25/100`, `taper-inert-off-tukey`, `taper-edge-report`.

Two harness fixes found on the way: probes Z4 and AF compare each measurement
against the reference configuration's *as they go*, which works only because
their reference happens to be first in their sweep. The new taper probe's
reference (α = 0.5) sits third, and a single-pass version reported a whole decay
rate as the delta — a 9.7 dB/s "failure" that was the probe. It now collects
first and compares after. The `boundReadouts` assertion tightened at v1.3.0 also
caught the knob count again (15, not 14), which is the argument for keeping it
exact rather than `>=`.

### Not done

Human DAW sign-off. The taper is verified offline and by both validators, but
whether α near 0.01 is musically useful or merely edgy is a listening call.

## [1.3.0] — 2026-07-25 — Grain count / overlap ceiling

Minor release implementing **section B2** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`): grain count was never directly
controllable, only inferable from density and grain size. Fills the second
reserved panel of the chassis v1.1.0 framed — markup plus one CSS block, no
resize.

### What changed

- **`grainCount`** — an explicit overlap ceiling, 2–16, step 1, **default 8**.
  The density map becomes `overlap = 2 + density·(ceiling − 2)`, replacing the
  hard-coded `2 + d·6`. Density 0 still gives overlap 2 at any ceiling, so
  nothing previously reachable became unreachable.
- **Spawn cap 32 → 128**, and no longer silent: dropped requests and pool
  refusals are counted separately and exposed on the processor.
- **COUNT panel** — the Count knob plus a live **Active / Overlap** readout.

### Why the ceiling is its own parameter and not a wider density knob

Density is stored **denormalised** in session state: a session saved at 60 %
recalls 60 %. Widening the density knob's own span to reach overlap 16 would
therefore have made every existing session ~2.3× denser at the same knob
position, and there is no migration available — APVTS sessions and preset JSON
need opposite treatment, and rescaling a session tree corrupts the ones already
correct (`critical_apvts_denormalised_vs_preset_normalised`).

A separate parameter sidesteps it entirely. Absent from any v1.0–v1.2 session or
preset, `grainCount` resolves to its default, and `(8 − 2)` is exactly `6.0f` —
so the new expression is the same three float operations on the same values that
v1.0.1 shipped. Bitwise, not "equivalent": written as
`min + d·(ceiling − min)` and deliberately not as the algebraically identical
`min·(1−d) + ceiling·d`, which lands an ulp away. Third release running where the
no-op default is a specific number rather than zero (v1.1's four → 0, v1.2's
`grainTilt` → 0.5, this → 8).

### The gain correction landed on the opposite path from the one predicted

The review expected `grainGain`'s `1/sqrt(overlap)` to under-correct as overlap
rose — overlapping grains read the same reversed material, so summing should be
partially coherent, the real rise should sit between √N and N, and the error
should grow with the ceiling. Sound reasoning, wrong path.

**Output path: no correction needed, measured.** The grains read the same
material but not at the same *time*. At a fixed output sample, grain *k* reads
source `2kH − D − t` for spawn interval *H*, so every pair in the sum is
separated by a multiple of `2H` — decorrelated for broadband input. Probe AA
sweeps overlap 2 → 16 and holds the wet level inside **0.07 dB** with no
correction term, against the ±1 dB budget probes D and Z2 use.

> The first version of that probe measured a **2.5 dB non-monotonic** spread,
> which is exactly what a coherence error looks like. It was the harness. The
> shared excitation generator has ±0.077 autocorrelation at lags 600–2400
> samples, which is precisely where the spawn interval sits, so each overlap
> setting summed a different amount of correlation *in the test signal* — the
> +1.45 dB outlier at overlap 10 sat on the generator's correlation peak at lag
> 960, that setting's own interval. A murmur3 finaliser (`whiteNoiseAt`,
> max|acf| 0.0024) drops the spread to 0.07 dB. Added alongside the old
> generator, not replacing it, so pre-v1.3.0 probe numbers stay diffable.

**Feedback path: a clipping defect.** What recirculates *is* self-similar, and
there `1/sqrt(N)` leaves √N of excess loop gain per generation. Harmless while N
stopped at 8; doubling the ceiling spent the whole margin:

| ceiling | decay @ fb 100, before the fix |
|---------|-------------------------------|
| 8       | −0.29 dB/s (shipped)          |
| 10      | **+0.87 dB/s** (growing)      |
| 12      | +0.87 dB/s                    |
| 14      | +0.67 dB/s                    |
| 16      | +0.46 dB/s → 90 s peak **1.28, clipped** |

Positive dB/s is self-oscillation. The `tanh` bounds the loop to ±1 per sample,
but the wet output is a near-coherent sum of 16 grains each reading loop content
at the limiter's ceiling, and `1/sqrt(16)` does not bound that.

Fixed by `loopCountTrim` = `(N/8)^−0.5` — the fully-coherent amplitude law,
used as derived rather than tuned — anchored at the legacy ceiling and exactly
`1.0f` at or below it, so the shipped decay stays bitwise the shipped decay. It
rides on the output/loop gain split v1.2.0 built for `gainRandom`. After:
decay spread across all ceilings **0.020 dB/s**, worst-case peak **0.28**.

### The spawn cap

The old 32 matched `GrainPool::kMaxGrains` on the reasoning that "excess spawns
would only steal grains anyway" — which died at v1.1.0, when the pool started
*refusing* instead of stealing. A drop and a refusal became different events, and
only the refusal is a design choice. The bound is now derived and sample-rate
independent: `overlapMax · kDelayTimeMinMs / kGrainSizeMinMs` = 16 · 50/50 = **16
nominal against a cap of 128**. Note it is grain size's *minimum*, not the
ceiling, that this cap is most sensitive to — below ~6 ms it would reach 128 at
ceiling 16, and probe AB is what will notice.

### The UI, and a reversed decision

The readout **reverses Stage 3's decision D10** ("no visualization, no Timer, no
C++→JS polling bridge"), deliberately: `GrainPool::countActive()` shipped in
Stage 2 and was called by nothing until v1.1's probe Y, which is how density
stayed an abstract percentage. It is a *pull* — JS polls a native function at
15 Hz — so there is no `juce::Timer` and no event-listener plumbing, which keeps
the whole bridge inside the surface the ui-stub already models. The native-fn
count goes 11 → 12.

The panel that was labelled MOTION is now **COUNT**: 276 px, unchanged in width,
position and height, so the frame geometry every tooltip clamp was verified
against is the same geometry. The label had to move regardless — "Motion"
describes delay-time drift (review B4 #6), which is not this, and now waits in
SPACE.

### Verification

| Check | Result |
|-------|--------|
| v1.2.0's 80 shared probe result lines vs v1.3.0's | **byte-for-byte identical** |
| Offline render harness | **93/93 probes PASS, exit 0** (81 + 12 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 18/18 anchors, clamp fired on 4 |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, exit 0 |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, exit 0 |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **66304** (= 1.3.0) |
| Grain-steal refusal at ceiling 16 | **holds** — `ceiling16-pool-clickfree`, maxStep 0.0088 vs 0.177 threshold, peak 24/32 grains |

The AU pluginval run emits one pre-existing `!!! WARNING: Current program is
−1` from the JUCE AU wrapper. Not a failure; exit code 0.

New probes: `level-flat-count`, `spawncap-headroom-512/4096`,
`count-default-identity`, `ceiling16-pool-clickfree`, `count-live-2/12/16`,
`count-meter-live`, `decay-count-fb60/fb100`, `ceiling16-loop-bounded`.

### Not done

Human DAW sign-off. The ceiling raise is verified offline and by both
validators, but "does overlap 16 actually sound like a smoother wash" is a
listening judgement no probe makes.

## [1.2.0] — 2026-07-24 — Grain window shape + tilt

Minor release implementing **section B1** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`), which called window tilt "the
highest-value single change in this document". Fills the WINDOW panel that
v1.1.0 framed and reserved — markup only, no resize, exactly as promised.

This is a *reverse* delay, and through v1.1.0 every grain played its source
backwards under a **symmetric** Hann: each one swelled in and out identically,
so the effect smeared but never bloomed. A window whose peak sits late produces
the backwards-swell-into-a-transient shape the effect is bought for.

### The compatibility guarantee, and how it was verified

Both new parameters default to the **shipped window**, and the no-op is not zero
for both: `grainShape` defaults to index 0 (Hann) and `grainTilt` to **0.5**
(symmetric). 0 is a hard peak-early tilt, so "new parameter, default it to 0"
would have re-voiced every existing session and all eight factory presets.

The defaults are the engine's *exact* no-op rather than approximately it, and
that is by construction (see the tilt design below). Measured the same way
v1.1.0 measured its own: the v1.1.0 harness was rebuilt from commit `8fa3646`
and run head-to-head.

| Check | Result |
|-------|--------|
| v1.1.0's 63 probe result lines vs v1.2.0's | **byte-for-byte identical** |
| Offline render harness | **81/81 probes PASS, exit 0** (63 + 18 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** (sections 1–15) |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 16/16 anchors, clamp fired on 4 |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **66048** (= 1.2.0) |

### Added

- **`grainShape` (Hann / Tukey / Gaussian / Triangular / Expo-Decay, default
  Hann)** — `WindowLut.h` was hard-coded Hann and had noted since Stage 2 that
  it was trimmed from O-simpleGrain's 5-shape `WindowLuts.h`. All five tables are
  built in the constructor, never on the audio thread, and indexed at spawn.
  Hann's table is bit-for-bit the expression v1.0.0 shipped.
  - The **Gaussian deliberately deviates** from O-simpleGrain's copy: at σ = 0.18
    the raw curve reads 0.021 at both ends rather than 0. In a one-shot granular
    texture that 2 % step is inaudible; here it would be a step at every grain
    boundary, overlapping 2–8 deep, inside a loop that re-reverses it every
    generation. The pedestal is subtracted and the result renormalised.
- **`grainTilt` (0–1, default 0.5)** — moves the window's peak within the grain.
  0 = peak early (a plucked, decaying reverse grain), 0.5 = the symmetric Hann
  as shipped, 1 = peak late (slow swell into a fast cut). Displayed as a signed
  percentage centred on "Centre"; the parameter keeps its 0–1 range because 0.5
  is the value whose warp is exactly neutral.
- **WINDOW panel**, with tooltip copy, dblclick reset and keyboard/wheel
  adjustment on the same footing as every other control.

### The tilt is a two-segment linear phase warp

`q = min(p, t)·(0.5/t) + max(p − t, 0)·(0.5/(1 − t))`, mapping `[0, t] → [0, ½]`
and `[t, 1] → [½, 1]`. Chosen over the review's suggested `read(pow(phase, k))`
(which puts a transcendental back in the grain loop) and over a family of
pre-tilted LUTs (~2.6 MB for a quantised approximation of a continuous control).
Two properties are exact rather than approximate:

- **At t = 0.5 it is the bitwise identity.** Both coefficients are exactly
  `1.0f`, so `q = min(p, 0.5) + max(p − 0.5, 0)`; for p ≥ 0.5 Sterbenz's lemma
  makes `p − 0.5` exact and `0.5 + (p − 0.5)` rounds to exactly `p`. Asserted
  over a 4097-point phase sweep, not assumed.
- **It is power-invariant for symmetric windows.** The segments' Jacobians are
  2t and 2(1−t), so the warped mean square is `t·mLo + (1−t)·mHi` — independent
  of t whenever the halves match. Tilt therefore cannot move the level or the
  loop's duty cycle for four of the five shapes, by construction rather than by
  a compensating constant.

### Two normalisations, because the two paths sum differently

The review warned that `grainGain = 1/sqrt(overlap)` assumes Hann's power duty
and that a shape change would read as a volume *and* a feedback change. Both
halves were real, and they needed **different** constants — which the first
implementation of this release got wrong in an instructive way.

- **Output path — power.** A pass over broadband input has each grain reading a
  different stretch of the ring, so contributions are decorrelated and sum in
  power. `shapeNorm = √(m_hann / m_shape)` folded into `grainGain` holds all five
  shapes inside **0.147 dB** (probe Z2). Tukey's mean square is 0.687 against
  Hann's 0.375, so uncompensated it would have landed +2.6 dB.
- **Feedback tap — amplitude.** What recirculates is the wash the engine just
  made: self-similar material read by overlapping grains at nearby offsets, so it
  sums closer to *coherently*, and a coherent sum follows the window's **mean**,
  not its mean square. Power-only normalisation left the decay rate spanning
  **4.40 dB/s** at feedback 100, ranked exactly by amplitude duty — "window
  shape" audible as "how long the tail lasts". `getLoopNorm()` multiplies the
  loop tap gains only.

The engine could express this because it has carried separate output and
feedback-tap gains since v1.1.0, where the split was built so `gainRandom` could
sit *after* the feedback tap. The same split, used in the other direction,
carries this. Neither constant may cross over: `loopTrim` on the output would
undo the power normalisation; `gainRandom` in the loop would make the decay rate
stochastic. Measured, worst deviation from Hann in dB/s:

| | feedback 60 | feedback 100 |
|---|---|---|
| power-only, all five shapes | 6.216 | 4.400 |
| power-only, excluding Expo-Decay | 1.318 | 1.330 |
| **+ loop trim, all five** | **1.848** | **0.175** |
| **+ loop trim, excluding Expo-Decay** | **0.042** | **0.042** |

Expo-Decay's residual is **not** a normalisation error and no linear constant
removes it: its crest factor is 3.10 against Hann's 1.63, so at equal loop energy
its peaks hit the loop's `tanh` harder and it genuinely loses more per
generation. It is largest at feedback 60 — mid-knee, where a limiter's
incremental gain is most level-dependent — rather than at 100, where everything
is deep enough into limiting for the differences to wash out. Probe Z4 bounds the
four low-crest shapes at 0.35 dB/s and Expo-Decay separately, so a regression to
power-only normalisation still fails even though it would sit inside any single
bound wide enough for Expo-Decay.

### Changed

- **`.shape-select` is 120 px, not `.division-select`'s 82 px.** "Expo-Decay"
  measures 94 px with padding and the arrow and rendered clipped. Visible only in
  a browser render — build, `auval`, `pluginval` and the static checks all pass a
  clipped select. Still inside the 190 px panel's content box, so nothing moves.
- **`bindDivisionCombo` generalised to `bindSelectCombo(juce, paramId)`** and
  called twice. The grainShape select needs identical behaviour — options built
  from live `properties.choices`, rebuilt if they arrive late, index refreshed on
  both events — and a second copy would be a second place for that to rot.
- **Factory presets** carry both new keys explicitly at the shipped window. The
  CMake `VERSION` bump is what makes those edits reach disk; at a frozen version
  the preset table is a silent no-op.

### Testing

Harness **63 → 81 probes**. New: `window-warp-identity`, `window-norm-identity`,
`window-default-identity`, `window-duty-report`, `window-loopnorm-identity`,
`level-flat-shape`, `level-flat-tilt-{Hann,Expo-Decay}`,
`decay-shape-fb{60,100}`, `window-live-{Tukey,Gaussian,Triangular,Expo-Decay}`,
`window-live-tilt{0,100}`, plus `sweep-grainTilt` / `sweep-grainShape` in the
all-parameter sweep and two new columns in the factory-preset audit.

`level-flat-tilt` runs for **Hann and Expo-Decay** specifically: for a symmetric
window the warp is power-preserving by construction and `getTiltNorm()` returns
exactly `1.0f`, so Hann tests the *warp*; Expo-Decay is the only asymmetric shape
and therefore the only one where the tilt normalisation arithmetic actually runs.
Testing Hann alone would leave it entirely unexercised.

`window-live-*` is the mirror of probe T: every other new probe asserts a *must
not change*, and a control wired to nothing satisfies all of them perfectly.

Two harness corrections fell out of the first run, both worth recording because
each looked like a DSP regression and neither was:

- **`setBaseline()`/`setDefaults()` now reset the window parameters.** Probe M
  sweeps them and leaves them where its triangle ended; probes P, Q and V run
  afterwards and inherited a tilted, non-Hann window. Probe Q's
  constant-overlap-add flatness read 0.3718 instead of 1.0000. Resetting at the
  source makes the leak impossible rather than making it every future probe's job
  to remember.
- **`decay-shape-fb60` needed its own measurement windows.** At ~9.8 dB/s, probes
  S and X's `[5–10 s]` vs `[20–25 s]` pair spans ~133 dB and the later window is
  reading the denormal floor. Four of five shapes still agreed to 0.04 dB/s while
  Expo-Decay read 1.5 dB/s off — which looks exactly like a normalisation failure
  and is not one.

### Notes

- No new RNG draws. The two xorshift streams' consumption per spawn is unchanged
  from v1.1.0, so probes T (zero-determinism) and W2 (block-size invariance) stay
  valid without re-tuning.
- Both parameters are latched per grain at spawn. Smoothing a window *shape* is
  not merely unnecessary but meaningless — two windows disagree at every phase,
  so any crossfade still steps a live grain's envelope.
- A symmetric window's two halves are canonicalised at construction when their
  power agrees to within 1e-6 relative. `std::cos`/`std::exp` evaluated at
  mirrored arguments disagree in the last ulp, which would turn "exactly 1.0f at
  every tilt" into "1.0f ± 5e-8" — inaudible, but it would cost the ability to
  assert power invariance as an *exact* property, and an invariant checkable only
  to a tolerance can rot by a real amount unnoticed. Mirroring the tables instead
  was not available: Hann's must stay bit-identical to v1.0.0's.

---

## [1.1.0] — 2026-07-24 — Grain randomisation + UI chassis

Minor release implementing **section B3** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`): the four grain randomisations that
close the gap between "many reverse delays" and "a granular cloud". Also
expands the editor chassis **once**, sized for the controls planned through
v1.6, so later releases drop into space that already exists.

Builds on v1.0.1's grown capture ring, as the review required.

### The compatibility guarantee, and how it was verified

All four new parameters default to **0**, which is the exact no-op in the
engine — not a small value, a genuine no-op. Every randomisation is gated on
`amount > 0` and draws **nothing** from the RNG when off, so the pan sequence
is untouched and existing work renders identically.

This is measured, not asserted. The v1.0.1 harness was rebuilt from commit
`78af47b` and run head-to-head with v1.1.0:

| Check | Result |
|-------|--------|
| v1.0.1's 49 probe result lines vs v1.1.0's | **byte-for-byte identical** |
| Offline render harness | **63/63 probes PASS, exit 0** (49 + 14 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** (sections 1–15) |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 14/14 anchors |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **65792** (= 1.1.0) |

Because nothing is renamed, removed, re-ranged or re-typed, and no existing
session or preset changes value or sound, this is MINOR rather than MAJOR.

### Added

- **`jitter` (0–100 %, default 0)** — randomises the grain **spawn interval**,
  `interval · (1 ± 0.9·jitter·u)`. Through v1.0.1 the scheduler was a strictly
  periodic countdown, and a fixed interval against a fixed grain length is a
  comb — the reason sustained material read as metallic rather than as a cloud.
  The deviation is symmetric, so the *mean* interval, and with it the average
  overlap and the feedback loop's duty cycle, are exactly unchanged. Capped at
  ±90 % rather than ±100 % so the low tail cannot approach a zero-length
  interval (i.e. a spawn every sample).
- **`delayScatter` (0–500 ms, default 0)** — randomises each grain's latched
  delay by ±this. Thickens the smear without moving the rhythmic anchor,
  because the mean delay is unchanged. This is the parameter that required
  v1.0.1 first: it can push a grain's latched delay 500 ms *past* the delayTime
  maximum, so the worst-case read span became 4.5 + 2·0.5 = **5.5 s** — which
  v1.0.1's 5.5 s ring met by a single sample. The ring is now **6.0 s**
  (+192 KB stereo at 48 kHz) for a real margin.
- **`sizeRandom` (0–100 %, default 0)** — randomises each grain's latched
  length, clamped back into `grainSize`'s own range. Jitter alone leaves a
  residual periodicity because every grain still shares one envelope length;
  this removes it. Clamping to the parameter's own endpoints means a randomised
  grain is never longer than one the user could dial in by hand, which is what
  keeps the ring bound above true.
- **`gainRandom` (0–100 %, default 0)** — randomises per-grain gain for depth
  and shimmer, applied **after** the feedback tap (see below). Power-normalised
  by `1/sqrt(1 + dev²/3)`, so it changes spread and not level.
- **RANDOM panel** holding the four new knobs, with tooltip copy, dblclick
  reset and keyboard/wheel adjustment on the same footing as every other knob.
- **`getActiveGrainCount()`** — exposes the live concurrent-grain count.
  `GrainPool::countActive()` had existed since Stage 2 and was called by
  nothing; the harness now reports peak concurrency as a measured number.

### Changed

- **`GrainPool::obtain()` refuses the spawn when no slot is free, instead of
  stealing the oldest grain.** v1.0 overwrote the oldest slot in place, which
  cut a live Hann envelope from mid-window to zero in one sample — a click, not
  a crossfade. It was unreachable in v1.0 steady state (max overlap 8 against
  32 slots), but all four randomisations raise the transient concurrent-grain
  peak, so it had to be safe *before* they landed. Refusing costs one
  contributor out of a wash of 8–32 and is inaudible.
- **The wet path is now accumulated twice** — once with per-grain random gain
  (the output) and once without (the feedback tap). This is what keeps
  `gainRandom` downstream of the loop: a randomised gain inside a recirculating
  path compounds every generation, so the knob would control *how long the tail
  lasts* rather than how it shimmers, and at feedback = 100 would make the
  decay rate itself stochastic. Costs two extra mul-adds per grain-sample.
- **`rngState` seeds from a per-instance hash** rather than the shared literal
  `0x12345678`. v1.0 gave every instance the same seed, so two instances on two
  tracks produced identical pan sequences — and would have produced identical
  grain randomisation too, correlating exactly where a wide cloud is wanted.
  The seed is fixed for the *lifetime of the instance*, not re-rolled per
  `prepareToPlay`, so one instance still reproduces across prepare/reset cycles.
  Under `OUARICON_RENDER_HARNESS=1` it collapses back to v1.0's literal.
- **Editor 940 × 484 → 940 × 743.** A second panel row (RANDOM | WINDOW |
  MOTION | SPACE) sharing row 1's pinned width contract (190 | 190 | 276 |
  190), so the two rows align column-for-column. `215 + 14 + 245 = 474`
  consumes the height increase exactly — row 1 and the footer do not move.
  Capacity is ~27 knob-cell slots against the ~26 controls planned through
  v1.6. WINDOW / MOTION / SPACE are framed and labelled but empty, carrying a
  dimmed fleuron; filling one in a later release is an HTML change with no
  resize and no re-verification.
- `.botanical-overlay` height pinned at 340 px instead of `70%`. Under a
  percentage the resize scaled the plate to ~520 px and it began reading as
  clutter behind two rows of translucent panels.
- `kDelayTimeMinMs` named, replacing the `50.0f` literals in the parameter
  range and the tempo-sync clamp — the same single-definition discipline A1
  established for the maximum.

### Fixed

- **The ui-stub's `delayTime` range was stale at 50–2000 ms**, missed when
  v1.0.1 widened it to 50–4000. Any browser render of the page — which is the
  gate for the failure classes C++ builds cannot see — was showing a readout
  that disagreed with the plugin.

### Verification added

Fourteen new render-harness probes (T–Y) and two new frontend sections:

- **T `random-live` / `random-zero-determinism`** — each randomisation
  measurably changes the render (no dead controls), and two independent
  all-zero renders are bit-identical.
- **U `level-flat`** — wet RMS within ±1 dB across {0, 50, 100 %} for all four,
  the same budget probe D holds density to. Catches a character knob that is
  really a loudness knob.
- **V `jitter-breaks-grid`** — at density 0 a regular spawn grid overlap-adds
  perfectly flat (probe Q), so flatness reads grid regularity directly:
  1.0000 at jitter 0, 0.0339 at jitter 100. Asserts *both* ends, so a dead
  jitter fails rather than passing quietly.
- **W `scatter-ring-worst-case` / `scatter-blocksize-invariance`** — the 5.5 s
  read span against the 6.0 s ring, and 512-vs-4096 bit equality with all four
  randomisations on.
- **X `gainrandom-loop-neutral`** — loop decay at feedback 100 with gainRandom
  0 vs 100: −2.493 vs −2.527 dB/s, delta 0.034. The single assertion behind
  "applied after the feedback tap".
- **Y `pool-pressure-clickfree`** — grainSize swept under maximum
  randomisation; peak concurrency reported (14/32), click-freedom asserted.
- **`ui_frontend_check.js` §15** — four-way knob closure across
  `createParameterLayout` / `kSliderIds` / `KNOB_IDS` / the `knob-*` and
  `val-*` elements, plus a FORMAT entry and a ui-stub range for each, plus the
  four defaults pinned at 0. A knob wired in three of the four places is a
  silently dead control.
- **`ui_tooltip_clamp_check.js`** (new file) — drives the real page in a
  browser at the real 940 × 743 and measures every tooltip rectangle. The
  static check can prove the clamp *code* is correct but not that it *fires*,
  because that depends entirely on viewport width
  (`pattern_tooltip_clamp_gate_viewport_sensitive`). It asserts both edges, not
  just width, and fails if the clamp never engages at all.

### Two block-size bugs caught during this work

Both were found by the new probes, and both would have shipped silently:

1. **Jitter draws batched per pass.** The scheduler consumes its RNG inside a
   per-sample countdown while the spawn handler consumes after the whole pass
   is scheduled. Sharing one stream interleaved them differently at 512 than at
   4096 samples, so an offline bounce would not match what was monitored. Fixed
   by splitting into two streams, each consumed a fixed number of times per
   spawn — making consumption a function of spawn *index*, which is block-size
   invariant.
2. **The scatter clamp was derived from `passLen`.** A2 bounds each engine pass
   to `D`; negative scatter can put a grain's latched delay below that, and the
   obvious repair — clamp the latched delay up to `passLen` — makes the latched
   value itself depend on the host block size. Fixed by keying both the pass
   bound and the clamp off `grainDelayFloor`, a function of the parameters
   alone.

Per-grain randomisation values are also now drawn *before* the pool slot is
requested, so a refused spawn consumes exactly what a granted one does and RNG
consumption cannot depend on pool occupancy.

---

## [1.0.1] — 2026-07-24 — DSP correctness

Patch release fixing the three defects found in the v1.0.0 read-only review
(`improvements/2026-07-24-v1.1-review.md`, sections A and C). **No new
parameters, no UI change, no window resize.** All three were invisible to auval,
pluginval-10 and the shipped 41-probe harness, which is why they survived Stage 4
— the harness gained 8 probes that each fail on v1.0.0 and pass here.

Validated at release:

| Gate | Result |
|------|--------|
| Offline render harness | **49/49 probes PASS, exit 0** (41 shipped + 8 new) |
| Same harness vs v1.0.0 DSP | **12 FAIL** — the new probes are not vacuous |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| `ui_frontend_check.js` | **ALL CHECKS PASSED, exit 0** |
| AU component version | **65537** (= 1.0.1) |

### Fixed

- **A1 — tempo sync silently clamped across this plugin's own tempo range.**
  `delayTime` maxed at 2000 ms and sync-derived times were clamped into it, so
  `1/1` collapsed below 120 BPM, `1/2D` below 90 and `1/2` below 60. In the
  70–100 BPM band the brief targets, the UI named a division the engine was not
  playing and two divisions landed on the same delay with no indication.
  **Root cause:** a literal `2000.0` in the sync clamp
  (`PluginProcessor.cpp:323`) duplicating the parameter's max instead of
  referencing it. `delayTime` max is now **4000 ms** (covers `1/1` at 60 BPM),
  the clamp reads the same `kDelayTimeMaxMs` constant, and the capture ring grows
  **3.5 s → 5.5 s** to cover `Dmax + 2·Gmax` = 4.0 + 1.0 s (≈ 2.1 MB stereo at
  48 kHz). The skew centre deliberately stays at **316 ms**, so short delay times
  keep their knob resolution.
- **A2 — grains read unwritten capture at large block sizes.** A grain spawned at
  block offset `i` latched `readAbs = blockStart + i − D` and rendered *before*
  the block's capture write, so reads were already-written only while `i < D`.
  With `D` bottoming out at 2205 samples (50 ms at 44.1 kHz), every 2048- or
  4096-sample buffer — routine in offline bounce and high-latency live rigs — had
  its late grains reading a full ring lap of stale audio, or silence early on.
  **Fix:** each engine pass is now bounded to `D` samples, so `i < D` holds by
  construction at any host block size. Chosen over clamping `D ≥ numSamples`
  (which would silently lengthen the delay at large buffers) and over
  write-input-first (which still drops the block's own feedback regeneration).
  The engine is now block-size **invariant**: a 4096-sample render is
  bit-identical to a 512-sample one (`max|Δ| = 0.000000000`, probe O). At the
  shipped 512-sample block with `D ≥ 2400`, the code path is a single pass and
  bit-identical to v1.0.0 — the fix costs nothing where it was already correct.
- **A3 — the bottom ~14 % of Density was a full-depth tremolo.** `overlap` mapped
  to `1 + density·7`, so at low density the hop equalled the grain length and
  Hann grains **abutted** — the wet output amplitude-modulated to true silence at
  every boundary (a 5 Hz, 100 %-depth gate at `grainSize = 200 ms`). Hann reaches
  constant-overlap-add at hop `G/2`, i.e. `overlap ≥ 2`. Remapped to
  **`overlap = 2 + density·6`**: same maximum (8), whole travel now a genuine
  smooth→dense sweep. Measured envelope min/max at `density = 0` goes
  **0.0000 → 1.0000** (probe Q).
- **C — `processBlock`'s oversized-block bail left the extra output channel
  unwritten.** The v1.0.0 bare `return` did already pass channel 0 dry through
  (the review's "bails to total silence" reading is wrong), but in a mono→stereo
  layout channel 1 is never written by this plugin and carried stale host memory.
  Dry is now explicitly duplicated to any unfilled output channel before bailing.
- **C — no `AudioProcessor::reset()` override.** Hosts calling `reset()` left the
  capture ring, grain pool, scheduler countdown and filter states populated, so a
  stale reverse tail survived a host-level reset. Now cleared, alloc-free, with
  the RNG re-seeded to the same fixed value `prepareToPlay` uses.

### Changed

- **Factory presets re-authored and re-seeded.** The A3 remap changes what a
  given `density` value means, so every preset's density is rewritten to
  `(7·d_old − 100)/6` — the value that reproduces its **shipped** overlap exactly
  (60→53.3, 55→47.5, 70→65, 30→18.3, 90→88.3, 65→59.2, 80→76.7). All eight
  presets therefore render as they did at v1.0.0; only the knob's scale moved.
  The `VERSION 1.0.0 → 1.0.1` bump is what invalidates the `.factory-version`
  sentinel and lets these edits actually reach
  `~/Library/O-ReverseDelay/Presets/Factory` — at a static version they would
  have been a silent no-op.
- **Feedback decay re-measured at `feedback = 100`** (probe S), since overlap
  sets both the spawn hop and `grainGain = 1/√overlap`, i.e. the loop's duty
  cycle. At the overlap-matched density (5.20) the decay is **−2.955 dB/s**
  against v1.0.0's **−2.958 dB/s** at the same overlap — unchanged to 0.003 dB/s.
  At the *same knob position* (density 60, overlap now 5.6) it is **−2.493 dB/s**,
  i.e. ~0.46 dB/s more sustain. No shipped preset moves, because all eight are
  overlap-matched.
- **Dead code removed:** `GrainScheduler::sampleRate` was stored in `prepare()`
  and never read. (`CaptureBuffer::readAbs()` is still uncalled and deliberately
  kept — it is the entry point for the planned stereo-source mode.)

### Migration Notes

The two persistence formats needed **opposite** treatment, and the review's
premise that both recall by normalised fraction is only half right:

- **Sessions need no migration.** APVTS stores each `PARAM`'s *denormalised*
  value — literal milliseconds — and JUCE restores it through
  `setDenormalisedValue()`, which re-normalises against whatever range is
  current. A v1.0.0 session saved at 1400 ms recalls 1400 ms under the 4000 ms
  range. Rescaling it would have **corrupted** it. Probe P asserts the round trip
  directly, and it passes against both the v1.0.0 and v1.0.1 DSP.
- **User presets do need migration.** `OuariconPresetManager::createPresetJson`
  stores `RangedAudioParameter::getValue()` — the normalised 0–1 fraction — so a
  v1.0.0 preset saved at 1400 ms would have read back as **2450.5 ms** under the
  wider range. `migrateUserPresets()` rewrites the `delayTime` fraction of every
  `"version": "1.0.0"` file in `Presets/User/` through the reconstructed v1.0.0
  range, then re-stamps the file. One-shot, guarded by a
  `.user-migration-version` sentinel mirroring the factory one (without it, every
  processor construction would re-read every preset on the message thread and
  concurrent constructions would race). **Known limit:** a v1.0.0 preset restored
  from a backup *after* the sentinel is stamped will not be migrated.

Automation, parameter IDs, ranges of the other nine parameters, and the state
format are all unchanged — this is not a breaking release.

## [1.0.0] — 2026-07-24 — first release

First shipped version: granular reverse-delay engine, Ouaricon Naturalist WebView
editor, 8 factory presets, preset bar and hover help.

**DSP is frozen as verified in Stage 2.** Stage 4 (Polish) shipped **zero** audio
changes — the D11 feedback-tap makeup constant was auditioned in Standalone and
**explicitly declined**: the wash decays as intended at `feedback = 100`, so the
topology's inherent ≈ −7.3 dB/generation pre-damping loss (−4.3 dB Hann² duty
+ −3.0 dB pan→mono-sum round trip) stands as the shipped character.

Validated at release:

| Gate | Result |
|------|--------|
| Offline render harness | **41/41 probes PASS, exit 0** (33 Stage-2 + 8 factory-preset audits) |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| `ui_frontend_check.js` | **76/76 PASS, exit 0** |
| AU component version | **65536** (= 1.0.0) |

pluginval strictness 10 covers Editor, Open editor whilst processing, Automation,
Editor Automation, Plugin state, Plugin state restoration, Parameter thread
safety and Fuzz parameters.

Windows is **deferred to CI** — the CMake already carries `NEEDS_WEBVIEW2` and
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, and the two `FileChooser`
completions hoist their `SafePointer` to a local rather than init-capturing it in
a nested lambda, which is what MSVC rejects.

### Added — Stage 4: Polish

- **Factory presets:** 8 presets seeded to
  `~/Library/O-ReverseDelay/Presets/Factory/` on first run — Reverse Bloom,
  Guitar Swell, Vocal Halo, Slow Wash, Tight Smear, Dark Cavern, Near-Infinite,
  Rhythmic Reverse. Authored in **engineering units** (ms / % / Hz / choice index)
  and converted skew-safe through each parameter's own `NormalisableRange` via
  `convertTo0to1`; `delayTime`, `grainSize`, `lowCut` and `highCut` are skewed, and
  a hand-written normalised fraction on any of them would recall 10–30× wrong.
  Harness probe N audits all eight through the **shipping** `loadPreset()` — the
  measured round-trip error is **0.0000 on every parameter of every preset**.
  *Near-Infinite* runs `feedback = 100` and renders 30 s in the harness as a
  preset-driven stability statement. *Rhythmic Reverse* is the one tempo-synced
  preset (1/8 dotted), audited against a 120 BPM playhead.
- **Preset bar:** window grows 940 × 440 → **940 × 484** for a 44 px band under
  the header carrying `◀ ▶ [ name ] Save Load Delete`. The band and the height
  increase are the same 44 px, so panel heights and the footer are untouched.
  Styling reuses the page's own `.segment` / `.division-select` vocabulary rather
  than importing a dark chrome strip that would give the page a second title bar.
  Delete uses a **two-click inline confirm**, never the browser confirm dialog,
  which is a silent no-op or a throw in some JUCE WebView backends.
- **OuariconPresetManager v1.0.5** integrated via CMake include (header-only, no
  vendored copy). Session state now routes through it, so the current preset name
  survives a save/reload; pre-Stage-4 APVTS sessions still load unchanged.
- **Tooltips** on all 10 controls, authored as `data-tip-title` / `data-tip` in
  `index.html`. Hover only — no toggle, no persisted state. The tip measures its
  width at `left: 0` and **pins it before placing**, so the right-most control
  (`mix`) gets a full 230 px tip instead of a shrink-wrapped ribbon.

### Added — Stage 3: GUI

- Ouaricon Naturalist WebView editor: four framed group panels in signal-flow
  order (TIME | GRAIN | FEEDBACK | OUTPUT), all 10 parameters bound two-way
  through `Web*Relay` / `Web*ParameterAttachment`.
- Sync/Free control swap on a shared fixed-size slot — both controls stay
  relay-bound at all times, so neither is ever a dead control.
- Readouts and knob angles come exclusively from `SliderState.getScaledValue()` /
  `getNormalisedValue()`; the C++ `NormalisableRange` is the only source of range
  and skew. Double-click resets to the engineering default fetched from C++.

### Added — Stage 2: DSP

- Reverse grain engine over a 3.5 s stereo capture ring (reverse read offset
  D + 2n), Hann-windowed grains from a 32-slot preallocated pool with per-grain
  parameter latching for click-free changes.
- Feedback loop through the shared capture buffer: wet → gain → high-pass →
  low-pass → `tanh` → non-finite guard. 2nd-order Butterworth damping filters
  updated in place with `ArrayCoefficients` (never `Coefficients::makeXXX` on the
  audio thread), cutoffs clamped to 0.49·fs.
- Tempo sync across a 13-entry note-division table with a no-BPM fallback; width
  spread via an RT-safe xorshift32 with alternating pan sign; custom equal-power
  dry/wet mix (zero latency).

### Added — Stage 1: Foundation

- JUCE 8 plugin shell, VST3 + AU + Standalone, `PLUGIN_CODE ORvD`.
- Bus layouts mono→mono, mono→stereo, stereo→stereo.
- APVTS with the 10-parameter contract: `delayTime`, `syncMode`, `noteDivision`,
  `grainSize`, `density`, `feedback`, `lowCut`, `highCut`, `width`, `mix`.

### Notes

- Preset library location is `~/Library/O-ReverseDelay/Presets/{Factory,User}/`
  (**not** `~/Library/Application Support/`). The name is hardcoded without the
  dev suffix, so dev and release builds share one library.
- Factory presets only re-seed when `JucePlugin_VersionString` changes. While the
  version is frozen at 1.0.0, editing the factory table is a silent no-op until
  `~/Library/O-ReverseDelay/Presets/Factory` is removed. (v1.0.1 bumps the
  version, which re-seeds them.)
