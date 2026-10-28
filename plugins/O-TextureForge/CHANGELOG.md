# O-TextureForge Changelog

## [1.7.0] — 2026-10-28

Pitched-up grains no longer alias. In Pitch-Mapped mode a note above C4 plays its grain faster than recorded, and the linear read folded everything above the new Nyquist back into the audible band: a 20 kHz partial played a fifth up came back at 18 kHz, almost as loud as the original. Those grains now read a band-limited copy of the corpus, built when it loads. MINOR because notes above C4 sound different: the folded-back tones are gone. There is no parameter, range, state, UI or i18n change. Every grain at or below C4, and every grain in Trigger+Modulate and Drone mode, renders bit-identically to 1.6.2.

### Changed

- **Band-limited reads for up-transposed grains (grain-engine 1.1.0 BandlimitedReader).** Each corpus is stored with six half-band octave levels (`SharedCorpus::mip`, a `MipRing<6>`). A grain with rate > 1 latches its octave level from its rate when it spawns, plus the 16-tap sinc set for the residual rate on that level. The rate is fixed for the grain's life, so the render loop does one 16-tap read per sample and nothing else. Grains at rate ≤ 1 keep the linear read, since they cannot alias.
- **The mipmap is built by the corpus loader, before the corpus is published.** The audio thread never sees a corpus without its mipmap, or with a half-built one, and the corpus lock and retired-corpus reaping are unchanged. The levels are padded with silence on both sides, so a grain that runs past the end of the corpus reads silence, as the linear read does. A load cancelled mid-build is dropped like any other cancelled load.
- The mipmap is kept as the ring itself, read by delay from its head, rather than copied into a `MipSnapshot`. A snapshot would hold a second full copy while it is built, and corpora have no length cap.

### Notes

- Top-octave trade-off of the 16-tap kernels: up to 9 kHz every rate is within 1.3 dB. At 16 kHz, a note one semitone above an octave is −0.9 dB, and a note one semitone below the next octave (the narrowest set) is −9.8 dB, where 1.6.2 was −0.9 dB with its aliases left in.
- Memory: the mipmap holds about 4× the corpus (two channels, because the module's ring is stereo and the engine reads mono). A 6-minute corpus at 48 kHz takes about 280 MB more. Building it adds about 0.5 s to that load, on the loader thread.
- Six levels cover rate 128. MIDI reaches +67 st (rate 48, level 5).

### Testing

- New `tests/alias-gate/` (`build.sh`, `alias.cpp`, `gate.py`): the scheduler offline, built against this tree and against the v1.6.2 backup. A 10 s sine at 48 kHz, one isolated 60 ms Hann grain per note. Thirteen alias cases cover levels 0 to 5, with each octave edge tested from both sides (S1 one semitone above, S6 one semitone below). Each folds to an audible frequency, and the audible energy must be ≤ −60 dB against the same sine at rate 1. Six in-band cases at about 9 kHz must stay within ±1.5 dB.
  - v1.7.0: all 19 pass. Alias −64.7 to −112.3 dB, in-band −1.23 to +0.01 dB.
  - v1.6.2 (positive control): all 13 alias cases fail, at −3.0 to 0.0 dB.
  - Far grain: a band-limited grain 95 s and 6.4 min into a periodic corpus is bit-identical to the same grain near the start, at notes 67, 89 and 127.
- New `tests/scheduler-harness/gate_v170.py` (`TF_BLR=1` builds the mipmap in the harness corpus, `TF_NOTE_MAX` clamps notes):
  - Block-size null with the mipmap, 32 / 512 / variable, all three modes, notes 40 to 99: 10/10 bit-exact.
  - Bit-identical to v1.6.2 in Drone and Trigger+Modulate, and in Pitch-Mapped with notes clamped to 60: 8/8.
  - Controls: mipmap on vs off differ by −9.9 dBFS in Pitch-Mapped, and with the mipmap off the new build is bit-identical to v1.6.2.
- `tests/scheduler-harness/reaper/` (processor level, real corpus loads through the loader, so the mipmap is built): `prepare_stress` A/B/C pass (62,742 saved states, 0 freed-corpus reads); `reaper_stress new` 0 poisoned blocks over 6,000 publishes.
- pluginval strictness 10 SUCCESS, auval SUCCEEDED.
- The harness builds now use `-std=c++20` and add the grain-engine headers with `-idirafter`, so the module's `GrainScheduler.h` cannot shadow the plugin's.

## [1.6.2] — 2026-10-27

### Fixed

- **Host threads could read a corpus while it was being replaced or freed.**
  The host calls prepareToPlay and getStateInformation on its own threads.
  Both read the current corpus without a lock while a load could be swapping
  it out. In a stress test, v1.6.1 saved the path of an already-freed corpus
  into the session state, and prepareToPlay started reloads from freed corpora.
  The corpus is now guarded by a lock that the audio thread never takes. Every
  other reader works from its own reference to the corpus, which keeps it alive.
- **A sample-rate change could fire a reload from the host thread.** That
  reload ran the corpus loader from the host thread while the message thread
  could be running its own loads, and it could hold up prepareToPlay for up to
  5 s while an earlier load shut down. The rate check and reload now run on the
  message thread, and repeated prepareToPlay calls at the same rate queue only
  one reload.
- **A file still loading when the rate changed played at the wrong pitch.** If
  the host changed sample rate while a corpus was still being analysed, that
  corpus arrived at the old rate and stayed there. It now plays as soon as it
  arrives and is reloaded at the new rate.

Audio rendering is unchanged from v1.6.1.

## [1.6.1] — 2026-10-27

### Fixed

- **Pitched grains got grittier deeper into a long corpus.** Grain read
  positions were single-precision floats. The interpolation fraction was already
  rounded to 1/16 of a sample 12 s into a 44.1 kHz file, was gone entirely past
  about 3.2 min, and past about 6.3 min grains skipped samples even at unity
  pitch. Within a grain, high notes also drifted: at note 89 a 180 ms grain
  ended 3 samples from where it should. Read positions are now double. A grain
  6.4 min into a file now renders bit-identically to the same audio at the start
  of the file, and to an exact reference. Before, the error was −53 dB at 12 s
  and −17 dB past 6.3 min.
- **Loading two corpora in quick succession could crash.** The processor kept
  only one retired corpus alive for the audio thread. If two loads finished back
  to back, the second freed a corpus that an audio block might still be reading.
  This could happen with a load already queued when the next began, or with the
  sample-rate reload in prepareToPlay. Retired corpora are now kept in a list and
  freed only once two audio blocks have completed since retirement, or once no
  block is running. The second rule frees them even when the host has stopped
  calling the audio callback.

Drone and Trigger + Modulate renders, which play at unity pitch, are
bit-identical to v1.6.0 on corpora under 2^24 samples (~6.3 min at 44.1 kHz).
Pitch-Mapped renders change by the size of the old error.

## [1.6.0] — 2026-10-26

### Changed

- **Grains start on their exact sample.** In Generative Drone mode every grain
  due in a block started together at the block's first sample, so the spacing
  between grains, and how they stacked, depended on the host's buffer size.
  Drone grains now start exactly one spawn interval apart. MIDI notes, note-offs
  and CCs also take effect on their own sample within the block; before, they
  were all applied at the block's start. The first drone grain now sounds as
  soon as playback starts. Before, it waited one spawn interval, up to 1.28 s
  at low Grain Density.
- **Renders are reproducible.** Variation's random grain picks came from a
  generator seeded from the clock, so every bounce sounded slightly different.
  Each pick is now a fixed function of a seed and the grain's number. The seed
  is chosen at random for each new instance and saved with the session, so a
  reopened project picks the same grains again. A session saved before 1.6.0
  has no seed, so it gets a new random one when it opens.

Renders are now the same at any buffer size: 32, 512 and varying blocks, each
on a fresh instance, null exactly. MIDI renders whose events fall on block
starts, and drone renders where the block equals the spawn interval, are
bit-identical to v1.5.4 at Variation 0.

## [1.5.4] — 2026-10-26

### Fixed

- **Dense drones clipped.** In Generative Drone mode the stacked grains were
  summed with no level compensation, so high Grain Density with long grains
  drove the output into the ±1 safety clip (density 64 × 500 ms stacks about 25
  grains). Drone mode now scales the cloud by 1/√(overlap), weighted by the
  window's power so Crossfade doesn't change the level, and ramps the change
  over 100 ms. Dense drones now hold the level of a single continuous grain
  instead of climbing up to 18 dB above it.

Pitch-Mapped and Trigger+Modulate modes are unchanged, and so are sparse drones
(less than one grain overlapping): those render bit-identically to v1.5.3.

## [1.5.3] — 2026-10-26

### Fixed

- **Voice steals clicked.** With all 64 grains sounding, the oldest grain was
  cut off mid-window to make room. The stolen grain now moves to one of 8
  spare slots and fades out over 5 ms while the new grain starts in the slot
  it always took.
- **Note-off clicked.** Releasing a key (or a new note in mono mode) cut that
  note's grains off instantly, mid-window. They now fade out over 5 ms.

Renders with no steals and no note-offs are bit-identical to v1.5.2.

## [1.5.2] — 2026-10-19

### Fixed

- **Clicking another dial now applies a typed value.** With a value-entry field
  open, pressing on a different dial left the field open and its readout
  frozen. The press now applies the typed value and closes the field before
  the new drag starts.

## [1.5.1] — 2026-10-19

### Changed

- **Output level now starts at −6 dB** on new instances (Gain knob,
  `OUTPUT_GAIN`; ID and −60..+12 dB range unchanged) and stays the last gain
  stage — only the safety clip follows it. Saved sessions recall their stored
  level unchanged.
- **Output level changes are now smoothed** (~20 ms linear ramp) instead of
  stepping once per block, so Gain moves and automation no longer zipper.
- **Double-click a dial to type a value** — Enter or click away applies, Esc
  cancels. **Alt/Option-click resets** a control to its default (this was
  double-click, and it now lands as one host gesture). Grain Size, Density and
  Gain show no readout, so their field opens over the dial itself.

### Fixed

- Resetting **Grain Size** to its default now lands on 50 ms. The reset used
  the linear knob position (0.08) on a skewed range and landed near 13 ms.
- **Build:** the plugin compiles again against the current `knncolle` (pulled
  in by umappp at `master`), whose headers now use `std::filesystem`, which
  libc++ marks unavailable below the suite's macOS 10.13 deployment target.
  The availability check is disabled for this plugin's code only.

## [1.5.0] - 2026-09-25

The UI pass from the 260924-nho design review: AA text colours (R4), the 9px
text floor, and the bundled EB Garamond face (R5). MINOR because the change is
visible. No parameter, range, type or state format changed. No DSP or processor
file was touched, and **the webpack bundle was not rebuilt**: `js/app.bundle.js`
and `src/app.js` are byte-identical to v1.4.0.

### Changed

- **Walnut text is `#6A5641`, down from `#8B7355`.** The body is a three-stop
  gradient (`#F5E6D3 → #EBD9C7 → #E8D0B8`). On its deepest stop, where the
  Gain and Crossfade captions sit, the template's `#715D45` reads only 4.22.
  `#6A5641` reads 4.68 / 5.06 / 5.68 across the three stops. `#8B7355` stays as
  `--knob-border` and on the two decorative fleurons.
- **Pressed or expanded sage fills are `#4E6839` (`--sage-deep`).** The expanded
  gear and the ON hover-help toggle read 3.06 and 3.99 under their paper text;
  they now read 5.10. `--active-green` and `--active-green-dark` still draw the
  borders, focus outlines and progress fill, which carry no text.
- **The large-file dialog's "Load Anyway" fill is `#7A5C10`,** up from 4.15 to
  5.09. The colour lives in an inline style that `src/app.js` builds, so the
  repair is a stylesheet `!important` rather than a bundle rebuild.
- **The drop zone's drag-over caption stays walnut.** Green text there read
  about 2.7:1. The green border and tint still carry the drag cue.
- **9px text floor.** The section labels, bottom-strip knob captions and
  readouts, the MIDI-mode caption and the UMAP progress caption move from 8px to
  9px. The unitless line-height pins were kept, so they scale with the size.
- **EB Garamond, bundled** from `modules/ui/eb-garamond` 1.0.0 by direct embed:
  4 `SOURCES` lines, 4 `getResource()` branches, and `css/eb-garamond.css`
  linked before the page stylesheet. `--font-primary` now starts
  `'EB Garamond', 'Georgia', …`. The bare `'Garamond'` entry is dropped so that
  an Office-installed Windows Garamond cannot outrank the bundled face.
- **`.midi-mode-label` min-width is re-pinned 55.17 → 58.66 px**, the new widest
  (en 58.66, fr 58.66, zh 55.16). The old pin fell below English at 9px in the
  new face, and check-ui-labels [7] moved `#midi-mode` 3.5 px on the Chinese arm.
- **Hover-help init renamed onto the canon spelling:** `initializeTipsToggle` →
  `initTipsToggle`, and `hideTip` → `hideTooltip`. Behaviour is unchanged and
  the `otf.tipsEnabled` key is kept. check-ui-canon moves `hover-help-init`
  from a variant to `shape`: identical code, only the storage key differs.
  `tests/ui_tip_render_check.js` [0] follows the rename.
- **The decorative fleurons carry `aria-hidden="true"`.**

### Not changed

- **Knob interaction.** The knob code (`setupKnob` in `src/app.js`) was not
  touched, so the O-ReverseDelay keyboard/ARIA port (R7) is deferred to a
  change that rebuilds the bundle.

### Measured (before v1.4.0 → after)

| Gate | Before | After |
|---|---|---|
| `measure-ui.js --contrast`, per language | 24 / 35 below AA (68.6%), 10 under 9px, median 4.49 | 2 / 35 (5.7%, the two `aria-hidden` fleurons), 0 under 9px, median 6.95 |
| `check-ui-labels.js` (en / fr / zh-Hans, 3 states) | ALL PASSED, 130 PASS | ALL PASSED, 130 PASS |
| CDP face probe (all 3 languages) | exit 2: no EB Garamond FontFace | PASS: every Latin run on the bundled face, Han on PingFang SC, 0 unserved |
| `check-ui-canon.js` | tooltip v, hover-help v, knob-binding v, knob-visual — | tooltip v, **hover-help shape**, knob-binding v, knob-visual — |
| `tests/ui_tip_render_check.js` | ALL PASSED | ALL PASSED |
| check-i18n / i18n-fr-lint / i18n-zh-lint | pass | pass |

## [1.4.0] - 2026-09-05

Simplified Chinese joins English and French (task 260905-rwh, Stage 4 wave 4c).
MINOR: a third language, one font-token edit, twelve geometry pins and a repaired
render gate. No parameter, range, type or state format changed, no audio path was
touched, and **the webpack bundle was not rebuilt**.

### Added

- **`zh-Hans` on every one of the 61 emitter rows** — 15 `I18N` entries (title
  and body) and 31 `LABELS` entries. Authored at `reviewed: 'mt'`, committed
  there, and promoted to **`reviewed: 'bt'`** only after two independent blind
  reverse reads. `'native'` stays open on every row: this project has no native
  Chinese reader, and `i18n-zh-lint` rule R1 prints that quality level on every
  run rather than leaving it to a reader's memory.
- **The endonym `简体中文`**, as numeric character references.
- **A three-way `languageCode` / `languageIndex` codec.** Anything that is
  neither `fr` nor `zh-Hans` degrades to English. **No Chinese character appears
  anywhere under `Source/`.**
- **A second UI state**, driving the file-size overlay. See below — the first
  state file drove one state, and the overlay was invisible to every measurement
  because of it.

### Changed

- **The file-size dialog's two buttons now name a face.** `src/app.js` builds
  that overlay at runtime, and its buttons carry only background, colour, border,
  padding, radius, cursor and font-size — a form control does *not* inherit
  `font-family`, so both took the UA stylesheet's bare Arial, a face with no Han
  glyphs, under captions that come from the label table and *do* translate.
  **The repair is a CSS rule, not a source edit**: a stylesheet reaches a
  runtime-created node just as well as an inline style would, and it keeps the
  committed bundle and its source in sync for free.
- **One token edit covers six declarations.** Every `font-family` on this page
  resolves through `--font-primary`, so the CJK tail lands once — *before* the
  trailing generic, because Chromium resolves a bare `serif` against the
  document's `lang` and a tail written after it is never consulted. No face had
  to be named: Garamond is not installed on the build machine (measured, 0 family
  matches) but Georgia and Times New Roman both are.
- **Twelve geometry pins.** Nine are `line-height`, each derived from that leaf's
  own measured English box, unitless, never global. `.knob-label` is pinned
  **twice** — it renders at 9 px in `.knob-row` and 8 px in `.bottom-knob-group`,
  and 10/9 and 9/8 are different ratios, so one unitless number cannot serve both.
  The other three exist because **the Chinese is smaller**, which is the half a
  clip check is blind to: the MIDI-mode caption is 55.17 px in English and French
  and 44.94 in Chinese, so the selector beside it slid 10.23 px left; and the
  file-size dialog's message is two lines in English and French and one in
  Chinese *and* the widest child of a shrink-to-fit dialog, so the dialog came out
  14.5 px narrower and shifted sideways. All three are `min-*`, never fixed: a
  floor must not cap a language that needs a third line or a wider sentence.
- **Two hover-help bodies lost an enumeration, in English and French alike.** The
  gear body counted the popover's controls and named the only one it found, which
  stopped being true when v1.3.0 added the hover-help switch — `check-i18n`
  assertion [16] *requires* that switch, which is what made the clause false. The
  language body ended by naming the selector's options in full. The endonyms still
  live in `I18N_EXEMPT`, because the `<option>` texts are never translated: the
  selector is now the only place the list appears, and the one place it cannot go
  stale.
- **`tests/ui_tip_render_check.js` derives its language list at BOTH of its two
  language sites.** The joined-pair assertion was the loud half; the two
  hand-written sweep calls were the silent one, and no repo-wide census can see
  them because a call site spells no list to grep for. Guarded so a list that goes
  empty makes the gate exit non-zero rather than sweep nothing and report green.
- **Three strings were re-authored after the blind reverse read.** The tagline
  named the wrong synthesis technique — 串联 is *series* in the electrical sense
  where the technique is concatenative, 拼接. The dialog's confirm button was in
  the wrong grammatical mood, reading as a progress report rather than an
  intention. And the brightness body described a resonator this plugin does not
  model. None of the three was findable by any gate in this repo.

### Verified

- `check-i18n` exit 0; `check-ui-labels` exit 0 with **0 FAIL on the English,
  French and Chinese arms**; `i18n-zh-lint` 0 findings and `BELOW SHIP BAR 0`;
  `i18n-fr-lint` exit 0 — French untouched.
- `measure-ui --mode box --report all`: `undeclared-font` **2 → 0**, `wrap-count`
  **1 → 0**, `svg-font-attr` 0, beside 38 visible Han-bearing nodes of which none
  resolves to a non-CJK face. `line-height-normal` reads 2, both evidenced
  non-movers.
- **`git diff --stat` on `js/app.bundle.js` is EMPTY.** The i18n pass is entirely
  outside the webpack bundle, which the render gate has asserted since v1.1.0 and
  which was confirmed independently: the bundle contains zero occurrences of
  `applyI18n`, `data-i18n` or `data-tip`.
- The render gate passes with a real Chinese arm swept, and its derive-or-abort
  guard was **fired**: a planted empty language export makes it exit non-zero, and
  the reverting edit leaves the table byte-identical by sha256.
- Zero Han under `Source/**/*.{h,cpp}`, positive control fired on the same run.
- `auval -v` AU VALIDATION SUCCEEDED, triple read off `auval -a`.

## [1.3.1] - 2026-09-03

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


## [1.3.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `otf.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 35.5..94.5, 190 x 59 px — byte-identical in English and French** — inside a 900 x 600 frame. The switch face grows 42.00 -> 44.25 px for *Marche*, leftward into the panel's own slack; `check-ui-labels` [7] reports 0 non-label elements displaced.
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

### Also driven

- `tests/ui_tip_render_check.js` [0] asserted the init line byte-for-byte and
  now asserts the ORDER plus `initializeTipsToggle()`'s presence inside the same
  guarded block — see the note in O-Texture v0.4.0 for the reasoning.


## [1.2.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **17 of 41 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint, which went from 19 findings to 0
  under `--strict`. Ten carry a terminology or idiom fix, thirteen a typography
  fix, seven a grammar or agreement fix and two a meaning fix. The most visible:
  the GRAIN SIZE caption is now **Taille de grain** (the glossary root, not the
  v1.2.0 abbreviation), CROSSFADE is **Fondu enchaîné** rather than *Fondu* —
  *fondu* alone is a fade, not a crossfade — a *bed* of grains is a **nappe**
  rather than a literal *lit*, and no-break spaces now sit before `%`, `;` and
  `:` and between every number and its unit, as French typography requires.
- **Two French sentences said something the English does not.** The TEXTURE tip
  listed two of the English's three qualities and has its third restored
  (*la rugosité*, for "grit"); the DENSITY tip said the high *values* overlap
  when it is the *grains* that overlap. Neither language's meaning was invented
  — the English was checked against `GrainScheduler.cpp` first.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.
- Four tip titles that are the same word in both languages — Texture, Variation,
  Position, Gain — now carry the explicit `sameAsEn: true` declaration, so an
  untranslated string cannot hide among them as a coincidence.

### Notes
- `reviewed: false` is unchanged on all 41 entries. That flag means *a native
  speaker read this*, and none has; this pass is a second machine reading
  against a glossary and a lint, recorded in the `i18n.js` header.
- Scatter X / Y keep **Disp. X** / **Disp. Y**. The glossary root *Dispersion X*
  measures 65.14 px and wraps to two lines inside the hard 72 px caption box,
  taking the label from 10 px tall to 20.

## [1.2.0] - 2026-08-30

### Added
- Hover-help, in both languages. 14 tooltips — one for each of the 12
  parameters, plus the settings gear and the language selector — each with a
  title and a two-or-three sentence body ending in the control's range and unit.
  Every French body is machine-drafted and flagged `reviewed: false`; no native
  speaker has read them.
- A tooltip RENDERER, because authoring the copy was not the whole job. Canon
  v2's `applyI18n()` writes `data-tip-title` and `data-tip` attributes onto the
  anchors and stops there; the code that reads them and paints a surface is
  per-plugin, and this page had none — `id="tooltip"` 0, `.tooltip {` 0,
  `closest("[data-tip]")` 0. Binding 14 entries without it would have shipped 14
  invisible strings past three green gates. Ported behaviourally from
  O-simpleFM: one delegated, cursor-following surface, clamped on all four edges
  with an 8 px margin, `pointer-events: none`, hidden until hovered.
- The renderer and its styling live in `Source/ui/public/js/i18n_init.js` and
  the page's own stylesheet, NOT in `Source/ui/src/app.js`. This is the suite's
  only webpack-bundled page, and putting them in the bundle input would mean a
  webpack rebuild inside every copy change.
- A last-input-device latch on the focus arm. A mouse click on a `<button>`
  focuses it, so an unconditional `focusin` rule parks a tip on screen after
  every click — measured here as **4648 px²** of the gear's own tip lying across
  the settings popover the click had just opened. `:focus-visible` is not the
  discriminator; Chromium reports it false for a programmatic `.focus()` after a
  click. Any keydown clears the latch, so the keyboard half of hover-help still
  works.
- A drag guard, which the reference renderer does not have and this page needs.
  Every knob starts its drag on `mousedown` and tracks `document.mousemove`, so
  a vertical drag straying into a neighbouring `.knob-row` would otherwise open
  that row's tip on top of the control being turned.
- `tests/ui_tip_render_check.js` — 287 assertions at the shipping 900 × 600.
  No existing gate can see a rendered tooltip: `check-i18n` reads the table
  statically, `check-ui-labels` has no tooltip awareness at all, and
  `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. With
  `setupTooltips()` commented out, this file reports 36 failures while the other
  two stay green — `check-ui-labels` byte-identically so.
- `.planning/params.tsv`, the runtime parameter inventory, and the
  `OUARICON_BUILD_TESTS` / `ouaricon_add_param_dump()` block that produces it.

### Changed
- `PluginProcessor.cpp` puts `#include "PluginEditor.h"` and all five uses of
  `TextureForgeEditor` behind `#if JUCE_WEB_BROWSER` — `createEditor()` plus the
  four `dynamic_cast<TextureForgeEditor*>(getActiveEditor())` sites in
  `setStateInformation()` and `loadCorpusFile()`. The param-dump console target
  compiles this TU with `JUCE_WEB_BROWSER=0` and no editor sources. Under a
  normal build `JUCE_WEB_BROWSER=1`, every guarded arm is the original code
  verbatim, and behaviour is byte-identical to v1.1.0.
- Two `I18N_EXEMPT` reasons corrected. Both said `50ms` and `0 dB` were "written
  by src/app.js"; they are not written at all. See below.

### Found and NOT fixed
- **Three `.knob-value` readouts render BLANK, and have since v1.0.0.**
  `setupKnob` is passed `(n) => ''` as the formatter for `grainSize`,
  `grainDensity` and `outputGain` (`Source/ui/src/app.js:705`, `:706`, `:708`),
  so the first `updateDisplay()` erases the authored markup fallbacks `50ms`,
  `8` and `0 dB` and nothing replaces them. Measured in the headless harness,
  not reasoned. Repairing it means editing the webpack INPUT and rebuilding
  `app.bundle.js`, which is out of scope for a hover-help commit and a 220 KB
  unreviewable diff. The three ranges in the new tooltip bodies therefore come
  from the parameter dump rather than from the page's formatter, and say so.
- **A fresh CMake configure of this repo cannot build this plugin.** The root
  `CMakeLists.txt` caches `CMAKE_OSX_DEPLOYMENT_TARGET` at 10.13, and
  `_deps/knncolle-src` needs `std::filesystem` (macOS 10.15+). The committed
  `build/` works only because its cache says 11.0. Root-CMakeLists-owned, one
  plugin, latent fresh-clone / CI hazard.
- `Source/ui/package.json` still declares `"version": "1.0.0"`. It is private,
  not host-visible, and was already stale at v1.1.0.

### Not changed
- No parameter IDs, ranges, types, defaults or DSP behaviour.
- No geometry. `check-ui-labels` output is byte-identical to the v1.1.0
  baseline — 0 non-label elements moved in either direction, and the tooltip
  node does not enter the label sweep. **No pin was added, so none is owed a
  negative control.**
- `js/app.bundle.js` is untouched — verified by checksum.
- No hover-help on/off toggle. Two plugins in the suite have one and forty-one
  do not; making this the forty-second is a uniformity decision, not a side
  effect of a copy commit.
- Readouts stay English in both languages, per D-03.

## [1.1.0] - 2026-08-28

### Added
- The PAGE speaks French. 21 HTML captions and 8 JavaScript-written messages are
  localized through a new `Source/ui/public/js/i18n.js` label table (en + fr),
  plus 3 keyed accessible names. Every French string is machine-drafted and
  flagged `reviewed: false` — no native speaker has read them.
- A settings popover beside the plugin name, carrying the language selector,
  styled in this plugin's own naturalist vocabulary.
- `getUiLanguage` / `setUiLanguage` native functions and session persistence.
  The language rides the saved state as a `UILANG` XML child beside `CORPUS` —
  this processor's own idiom for non-parameter state — and is deliberately NOT
  an `AudioParameterChoice`: it must not appear in a DAW automation lane, and a
  preset must not decide which language somebody reads their plugin in.
- `Source/ui/public/js/i18n_init.js`, a served ES module carrying the canonical
  i18n runtime. This is the first webpack-BUNDLED page in the suite: the canon
  block cannot live in `src/app.js`, because webpack would resolve its
  `import './i18n.js'` at build time and inline the label table into
  `app.bundle.js`, leaving the embedded, served `js/i18n.js` read by nobody.
- `tests/i18n-states.json`, so the label gate can open the popover and measure
  the captions inside it.

### Changed
- `.bottom-knobs` is a five-column grid rather than `flex` + `space-around`.
  Under space-around the five shrink-wrapped knob columns took their positions
  from every caption's rendered width, in both directions, so a French caption
  moved the whole row. A 1fr grid makes each column 173.6 px whatever it holds.
  The English knobs move by at most 3.12 px and the row is now evenly spaced.
- `.scatter-placeholder` has a fixed 320 px width. It is absolutely positioned
  and centred with `translate(-50%)`, so it shrink-wrapped its message and moved
  its own box as the text changed. Its visible centre is unchanged to 0.00 px.
- The scatter placeholder's text moved into its own span. Through 1.0.2 the
  placeholder was rewritten with `innerHTML` in three places, re-authoring the
  fleuron glyph each time; the glyph is now authored once and only the text is
  written, so no localized string passes through a markup path.
- No native `title=` attribute was removed, because 1.0.2 carried none.

### Not changed
- No parameter IDs, ranges, types, defaults or DSP behaviour.
- The three MIDI-mode option captions stay English in French: they are the
  `MIDI_MODE` `AudioParameterChoice` option strings byte for byte, and the page
  and the host automation lane must agree about the same setting.
- Readouts stay English in both languages — `50 ms`, `0 dB`, `50%`.

## [1.0.1] - 2026-02-15

### Fixed
- File loading was non-functional: WebView intercepted all drag-and-drop events before they reached the JUCE FileDragAndDropTarget
- Added `browseForFile` native function using juce::FileChooser for reliable file selection
- Wired click handlers on drop zone and scatter placeholder to open file browser dialog
- Large file warning (>100MB) now also works through file browser path

## [1.0.0] - 2026-02-15

### Added
- Initial release: concatenative texture synthesis engine
- 19D MFCC descriptor extraction with PCA and UMAP scatter visualization
- Real-time granular playback with KD-tree nearest-neighbor search
- Timbral macro controls (Energy, Brightness, Texture)
- Scatter position controls with variation radius
- Three MIDI modes: Pitch-Mapped, Trigger + Modulate, Generative Drone
