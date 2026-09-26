# O-AnalogEQ Changelog

## [1.5.3] - 2026-09-26

Clears the Info tier of the v1.5.0 thorough code review — **IN-06 through IN-09,
all four**, verified LIVE against current source before any edit. PATCH: no
parameter ID, range, type or state format changed, and the rendered audio is
byte-identical.

Three of the four were comment and dead-number work whose whole risk was the
compiler's. Two touch what runs, and both are gated.

### Fixed

- **IN-08 — a second preset dialog destroyed the first `FileChooser` mid-flight.**
  *Root cause:* `savePresetWithDialog` and `loadPresetFromFile` assign to the same
  `std::unique_ptr<juce::FileChooser>` editor member, so a second launch while the
  first dialog was still presented destroyed that chooser underneath its own open
  dialog. The pending completion then either never fires — a hung JS promise with
  the editor still **alive**, which is precisely the case WR-04's bail path is not
  — or fires against a destroyed `Pimpl`.

  *Fix:* a `bool presetDialogInFlight` member on the editor, set before each
  launch and released inside the completion above every early return.
  **Deliberately not the review's prescribed fix.** §IN-08 says to guard on
  `fileChooser != nullptr` and "clear `fileChooser` at the end of each completion
  body" — which would destroy the `FileChooser`, and the `std::function` then
  executing, from inside that very callback. The nullptr test also cannot be
  adopted without that clear: guard without it and the first dialog latches the
  guard shut, so no preset dialog ever opens again for the editor's lifetime.
  Clearing a bool touches no owner, so the chooser is replaced only on the *next*
  launch, when the previous dialog has certainly finished — which is what the code
  did before, and was safe. This is the second time a §Fix block in this review
  has been unsafe as literally written (WR-07's was the first, closed in v1.5.2).

  The refusal is reported as `{success: false, name: ""}`, matching the shape the
  cancel path already returns, so `preset-manager.js` needs no change.

### Changed

- **IN-07 — sixteen `getRawParameterValue(StringRef)` lookups per block are now
  sixteen pointers resolved once in the constructor.** Not an RT-safety fix, and
  the distinction is recorded in `PluginProcessor.h` so a later reviewer does not
  re-raise it as a CR-01-class blocker: APVTS keys `adapterTable` on `StringRef`,
  not `juce::String`, so a `find` with a literal constructs no `juce::String` and
  allocates nothing. What it cost was ~16 × log₂16 ≈ 64 string comparisons per
  block forever — ~48 000/second at 64 samples and 48 kHz — to re-derive pointers
  fixed for the processor's lifetime. Resolved in the constructor rather than
  `prepareToPlay`: the adapter table is built with `parameters`, so a sample-rate
  change has nothing to re-resolve. The twelve `prepareToPlay` seeds read through
  the cache too.

  Output is identical by construction, which is exactly why it needed a gate
  rather than a build: the one defect this change can introduce is a silent
  mapping typo (`pHmfFreq` taking `"hmf_gain"`), and render-harness G1–G4 all
  still pass with two same-band reads exchanged. See Testing.

- **IN-06 — the settings-popover contract comments described a one-row panel two
  minor versions after it grew a second row.** Both blocks rewritten
  (`index.html:1079` and the popover's JS section heading). The `:1079` block had
  read as an argument *against* adding the hover-help toggle — "adding one means a
  second control here, a preference persisted through C++ and a data-tip-always
  bypass" — all three of which shipped in v1.4.0, so an executor adding a third
  row would have concluded the work was unbuilt and re-derived a bypass that
  already exists.

  **The count in that block was wrong in both terms and is deleted rather than
  refreshed.** It claimed "Two plugins in the suite have that toggle and forty-one
  do not"; measured at v1.5.3, **25 of 44** carry `#tips-toggle` and 19 do not
  (and 2 + 41 = 43, never the suite size). Its premise — that the toggle is rare
  and uniformity is a separate pass — has inverted since it was written. Writing
  `25/19` in its place would go stale the same way: a suite-wide fact has no gate
  inside one plugin's markup, so the sentence is gone. The popover heading's
  `tip.settings` claim is deleted for the same reason — it asserted what a copy
  string promises, the string was revised in v1.5.0, and the comment lost that
  race immediately.

  The surviving geometry half was **re-measured, not assumed**: `bottom: 8px` on
  `.settings-cluster`, `height: 220px` on `.container`, `overflow: hidden` — all
  three confirmed, so the "opens upwards" reasoning is kept and now cites the
  rules it depends on.

- **IN-09 — the page's init log announced `v1.3.1`** while `CMakeLists` declared
  1.5.2: four versions stale, not the two the review recorded. The literal is
  dropped rather than corrected (`'OuariconAnalogEQ UI initialized'`). A
  hand-maintained second copy of the version has no gate and will go stale again,
  so the fix is that there is no second copy. Gated by check [10] below.

### Testing

Two new gates, because three of these four findings are the kind a comment cannot
protect. **Both were seen to FAIL before being accepted** — a check that has never
failed is not a guard.

- **`tests/check-param-cache.js`** (new, 11 assertions) gates IN-07's mapping by
  NAME rather than by audio, since audio cannot see the defect. Each cache member
  must be `p` + CamelCase of the ID it holds; the cache must be complete against
  `createParameterLayout()`'s own `ParameterID` list in both directions; and no
  `getRawParameterValue` call may survive outside the constructor, so the per-block
  lookup cannot creep back one parameter at a time. Negative controls, all three
  restored afterwards:
  - `pHmfFreq`/`pHmfGain` swapped → **[3] FAILS**, naming both sides and the
    expected member. This is the exact typo the file exists for.
  - one per-block lookup reintroduced in `processBlock` → **[5] FAILS**.
  - one cache assignment dropped (a parameter added without an entry) →
    **[2] and [4] FAIL**, naming `lmf_q` and the member it expected.
  11/11 with the real cache.

- **`tests/ui_tip_render_check.js` check [10]** gates IN-09: no `console.*` call in
  `index.html` may hard-code a `vX.Y.Z` literal. Scoped to console call lines on
  purpose — the file is full of legitimate `// v1.2.0:` history comments and a
  blanket ban would fail on every one. Negative control: restoring the v1.3.1
  string turns [10] red and names the line.

- Render harness **19/19** unchanged, which is the honest statement of what it can
  say here: G1–G4 confirm IN-07 broke none of CR-02, WR-05, WR-06 or the WR-07
  follow-up guard, and by design cannot confirm the mapping — check-param-cache
  does that.

- `check-i18n` 0 FAIL repo-wide over 44 plugins; `auval` SUCCEEDED; build clean.

### Notes

The review's open list is now **empty**: CR-02 and WR-05..10 closed in v1.5.1, the
two residual defects those fixes introduced closed in v1.5.2, and IN-06..09 here.
`CODE_REVIEW.md` is retained as the record rather than deleted.

Deferred, unchanged from v1.5.2 and both larger than this plugin:
`savePresetToFile()` is still unguarded inside `modules/persistence/` with nine
other dialog callers unaudited, and `check-i18n`'s prose rules still do not scan
served shared modules (reported per plugin as a `NOTE:` rather than passing in
silence).

## [1.5.2] - 2026-09-26

Closes two residual defects found while re-verifying the v1.5.1 review fixes.
Both were introduced or left open BY that release rather than by the v1.5.0
code under review: WR-07's fix dropped a guard, and WR-10's gate widening
covered O-AnalogEQ but not the repo it claimed. PATCH — no parameter ID, range,
type or state format changed.

### Fixed

- **WR-07 follow-up — the fix that closed the discarded-directory bug silently
  removed factory-preset overwrite protection.** *Root cause:* preset-manager's
  two save APIs are not interchangeable. `savePreset(name)` early-returns
  `false` on `isFactoryPreset()`; `savePresetToFile(file)` has **no guard at
  all** and writes wherever it is pointed. v1.5.1 swapped the first for the
  second — correctly, since the dialog's chosen directory was being discarded —
  and lost the check with it. Two failures, both reachable from a dialog that
  opens in `getUserPresetsDirectory()`, and both silent:
  1. Typing a factory name (e.g. `Surgical Cut`) writes
     `Presets/User/Surgical Cut.json` and returns `true`, so the UI reports
     success — but `loadPreset()` searches `Presets/Factory/` **first**
     (`OuariconPresetManager.h:450-453`), so the save is written and
     permanently unreachable. Pre-swap, `savePreset()` returned `false` and the
     UI correctly said so.
  2. Navigating up into `Presets/Factory/` overwrites the factory JSON
     outright. `initializeFactoryPresets()` early-returns while the
     `.factory-version` sentinel matches `JucePlugin_VersionString`, so nothing
     restores it until the next version bump.

  *Fix:* `oaeq::presetSaveRefusal` in the new `Source/PresetSaveGuard.h`, called
  at the dialog callback before the write. The test is **location-aware rather
  than name-only** on purpose: a bare `isFactoryPreset(basename)` reject would
  re-break the arbitrary-path export WR-07 exists to enable, because saving a
  tweaked "Surgical Cut" to `~/Desktop` shadows nothing — `loadPreset()` only
  ever searches the two library directories. Only a write into those can
  collide, so only those are refused. The refusal is reported through
  `NativeMessageBox` as well as `complete`, because the page's SAVE button is
  fire-and-forget (`preset-manager.js` wires `() => this.saveWithDialog()` and
  that method discards everything but `{success, name}`), so a reason carried
  only on the bridge would be invisible — the same silent-failure class WR-07
  itself was. Reporting it in C++ keeps the fix plugin-local; threading a reason
  through the shared module is a 19-consumer rollout.

- **WR-10 follow-up — the widened `check-i18n` scan resolved
  `${CMAKE_SOURCE_DIR}` entries against the wrong root and dropped them in
  silence.** *Root cause:* the served-set scan added in v1.5.1 derives shipped
  page JS from `juce_add_binary_data`'s SOURCES, but its path regex
  `[A-Za-z0-9._/-]+\.js` excludes `$`, `{` and `}`. A SOURCES line reading
  `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js`
  therefore captured the **leading-slash** remainder, which
  `path.join(pluginRoot, rel)` resolved to `plugins/<Name>/modules/…` — a path
  that does not exist — and the branch then `continue`d with no `missing[]`
  report, unlike its sibling scan directly above it. So WR-10 closed the hole
  for O-AnalogEQ (whose SOURCES are all plugin-relative) while leaving it open
  for every plugin that references the shared module by its canonical path.
  Four were affected, embedding modules that nothing scanned for the contract
  §4 native-`title=` rule: **O-ReverseDelay** and **O-Contrabass**
  (`preset-manager.js`), **O-Marimba** (`analog-eq-unit.js`,
  `compressor-unit.js`), **O-MicrotonalSampler** (`webview-drop-streaming.js`).
  O-ReverseDelay's gate printed `[12] … 1 module(s): js/app.js` with no `NOTE:`
  line and passed `[11] zero native title= remain` — the exact "statement about
  one file printed as a statement about the page" shape WR-10 was opened over.

  *Fix* (`scripts/check-i18n.js`), three parts, and the first alone would not
  have been safe:
  1. `${CMAKE_SOURCE_DIR}/` entries resolve against the **repo** root and
     plugin-relative ones against the plugin root; deduplication keys on the
     resolved path, since the two forms can name one file.
  2. An unresolvable entry is **reported**, never skipped — a path the gate
     cannot open is a file the build embeds and the page runs while nothing
     scans it.
  3. The extension is anchored with `(?![\w.])` and trailing `#` comments are
     stripped. Both were latent while unresolvable paths were dropped in
     silence and become **false failures** the moment they are reported: a
     SOURCES entry `strings.json` otherwise yields a phantom `strings.js`, and
     `foo.js  # superseded by bar.js` otherwise harvests `bar.js`.

### Testing

- `tests/render-harness/` gains **G4**, seven assertions over
  `oaeq::presetSaveRefusal`. The predicate was lifted into its own header
  specifically so this target can reach it: the editor TU is not compiled in
  (`JUCE_WEB_BROWSER=0` — `pattern_render_harness_breaks_on_webview_editor`),
  so a lambda in the dialog callback would have been ungateable, and G4 asserts
  the value the callback actually branches on rather than a downstream proxy.
  **Verified non-vacuous in both directions:**
  - Guard neutered to `return {}` → the four refusal arms FAIL, the two "allow"
    arms still pass.
  - Guard replaced with the obvious name-only `isFactoryPreset(basename)`
    reject → the export arm FAILS alone, which is the arm that exists to stop
    WR-07 being re-broken by its own fix.
  - The first assertion gates the factory bank itself being non-empty, without
    which every arm below it would pass over nothing.
  - 19/19 pass with the real guard in place (12 prior + 7 new).
- `check-i18n` verified non-vacuous for the widened scan: re-adding
  `title="Save preset"` to the canonical `createPresetBar` template now turns
  **O-ReverseDelay and O-Contrabass red**, where before the fix both stayed
  green while embedding that exact file. Repo-wide the gate remains 0 FAIL over
  44 plugins, and the four previously-blind plugins now report their served
  modules in `[12]`.

### Known limitations

- The served modules are scanned for the §4 native-`title=` rule but still not
  for unkeyed prose; the gate reports this per plugin as a `NOTE:` rather than
  passing in silence. Keying them is a shared-module rollout across 19
  consumers, unchanged from v1.5.1.
- `savePresetToFile()` itself remains unguarded in `modules/persistence/`. The
  guard here is at O-AnalogEQ's call site. Nine other plugins call that API from
  a dialog and have not been audited for the same gap.

## [1.5.1] - 2026-09-25

Resolves seven findings from the v1.5.0 thorough code review (`CODE_REVIEW.md`):
one critical, six warning. PATCH — no parameter ID, range, type or state format
changed, and every fix is a defect repair rather than a feature.

**A version bump is load-bearing here, not cosmetic.** CR-02 changes what bus
layouts the AU advertises, and Logic caches an AU's I/O configuration per plugin
VERSION — clearing the cache is not enough, the version must move for Logic to
re-negotiate.

### Fixed

- **CR-02 — `isBusesLayoutSupported` was never overridden, so a stereo-in /
  mono-out negotiation null-dereferenced every filter on the audio thread.**
  *Root cause:* the base `juce::AudioProcessor::isBusesLayoutSupported` returns
  `true` unconditionally, so the wrappers advertised every layout they enumerate
  — including asymmetric ones — although `BusesProperties` declares stereo in /
  stereo out. On a `(2,1)` negotiation `prepareToPlay` sized each
  `ProcessorDuplicator` from `getTotalNumOutputChannels()` = 1, allocating ONE
  mono filter, while JUCE sized the audio buffer at `max(totalIn, totalOut)` = 2.
  `ProcessorDuplicator::process` then loops `jmin(inCh, outCh)` = 2 and indexes
  `processors[1]`, which is out of range on an `OwnedArray` and returns
  `nullptr`; its two `jassert`s are debug-only, so a Release build dereferenced
  it at the first block. *Fix:* the override accepts mono or stereo and requires
  input to equal output, refusing asymmetric negotiation outright. `processBlock`
  additionally clamps the chain to the prepared channel count as defence in
  depth, since a buffer's width is a max over both directions and says nothing
  on its own about how many output channels exist.

- **WR-05 — `output_gain` stepped instead of ramping; `dsp::Gain`'s default ramp
  is zero seconds.** *Root cause:* the prior review's WR-02 excluded
  `output_gain` from the v1.1.9 smoothing work on the stated grounds that
  `juce::dsp::Gain` "smooths internally". It does not by default:
  `rampDurationSeconds` is declared `0`, `reset()` calls
  `gain.reset(sampleRate, 0)`, and a `SmoothedValue` with zero steps jumps on
  `setTargetValue`. `setRampDurationSeconds` was never called anywhere in the
  plugin. Loading "Surgical Cut" moved `output_gain` 0.5 → 0.542, i.e.
  0 dB → +1.008 dB, in a single sample while every band around it ramped over
  30 ms. *Fix:* `setRampDurationSeconds(kSmoothingSeconds)` before
  `outputGain.prepare(spec)` — that order matters, because `reset()` no-ops
  while `sampleRate` is still 0, so `prepare()` is what applies the duration.

- **WR-06 — band on/off was a hard bypass over stale filter state: a click on
  toggle, a transient burst on re-enable.** *Root cause:* the four `*_on`
  booleans were read once per block and used as a raw `if` branch around each
  filter. Two defects followed, neither covered by WR-02's frequency/gain
  smoothing. (1) The falling edge removed an entire shelf between one block and
  the next — an instantaneous spectral change, automatable from a DAW lane.
  (2) Worse, a bypassed band's `ProcessorDuplicator` was neither fed nor reset,
  so its biquad `z⁻¹`/`z⁻²` retained the last samples seen before the bypass —
  possibly minutes earlier and at an unrelated level — and re-enabling convolved
  them into the current signal as an impulse-like burst. *Fix:* each band now
  carries a wet/dry `SmoothedValue` driven from its boolean over the same 30 ms
  ramp, and the filter runs **unconditionally**. A band that is always fed cannot
  hold stale state, so (2) disappears as a consequence of fixing (1). A settled
  fully-on band still filters in place, so the common path costs exactly what the
  old branch did; only a fading or fully-off band uses the scratch buffer, which
  is sized in `prepareToPlay` so the crossfade allocates nothing on the audio
  thread.

- **WR-07 — "Save Preset" silently ignored the directory the user chose.**
  *Root cause:* the dialog handler took the chosen `juce::File`, discarded
  everything but its base name, and called `savePreset(name)` — which always
  writes to `getUserPresetsDirectory()`. Saving to `~/Desktop` reported success
  and updated the name display while writing nothing there; saving to
  `~/Desktop/Default.json` silently overwrote the user preset `Default` in the
  library directory. `savePresetToFile(const juce::File&)` has existed since
  preset-manager v1.0.8 and handles the extension and parent directory itself;
  `loadPresetFromFile` already used its counterpart correctly, so load and save
  were simply asymmetric. *Fix:* call `savePresetToFile(file)`, and report the
  name that actually landed on disk rather than re-deriving it.

- **WR-08 — the v1.4.0 hover-help switch shipped with zero behavioural
  assertions.** *Root cause:* `#tips-toggle` appeared in `ui_tip_render_check.js`
  exactly once, in the `anchorsOpen` array — which hovers the switch to read its
  own tooltip and never presses it. Both `page.click` calls were `#gear-btn`.
  Deleting the show gate itself left the file reporting 415 PASS, `check-i18n` 0,
  and `check-ui-labels` 0, while a user who turned hover-help off got it anyway.
  *Fix:* a tips-off arm with thirteen assertions, each failing on a distinct
  single-line deletion — the show gate, the `data-tip-always` bypass (the one
  that distinguishes "gate works" from "renderer died"), the `hideTip` coupling,
  and the `localStorage` round-trip read back off the rendered switch. The
  always-set and the contrast anchor are derived from the DOM rather than
  spelled. *Verified:* deleting the show gate now fails the suite (exit 1);
  428 assertions pass with it in place.

- **WR-09 — the switch's Off label was never geometry-measured, in any
  language.** *Root cause:* `check-ui-labels` drives the states in
  `tests/i18n-states.json`, which held two entries, both of which leave the
  switch in its `ui.on` arm. Its "11 of 11 `[data-i18n]` elements were VISIBLE"
  line was true and was exactly the blind spot — a two-state control counted
  once. `.settings-toggle` is `flex: 0 0 auto` with `min-width: 42px`, a floor
  rather than a cap, so the box grows with its content. *Fix:* a third state
  reaching the Off arm, plus a schema extension to `scripts/check-ui-labels.js`
  so a state may carry an ARRAY of selectors (the switch lives inside the
  popover, so it needs `#gear-btn` then `#tips-toggle`). A string still works
  everywhere it did before. *Verified:* planting a long French `ui.off` string
  now fails assertion [5] with `ui.off 0.0px -> 52.2px`.

- **WR-10 — `check-i18n`'s JS scan saw one module; the page ships three.**
  *Root cause:* assertion [12] enumerated the inline `<script type="module">`
  plus `Source/ui/public/js/*.js` plus `${CMAKE_SOURCE_DIR}/modules/**.js`
  named from CMakeLists. None of those reach a plugin-local vendored copy in a
  sibling directory, and this plugin serves `/modules/preset-manager.js` from
  `Source/ui/public/modules/`. Assertion [11] then certified "zero native
  `title=` remain" over the half it could see. *Fix, two parts — and the first
  alone would not have closed it:* (1) the served set is now derived from
  `juce_add_binary_data`'s SOURCES, which is the build's own authority on what
  the page is served, excluding `js/juce/` as upstream; and (2) a markup-in-JS
  title detector, because the four offending attributes sit inside an
  `innerHTML` template literal and are invisible to both the markup scan and to
  the existing `.title =` / `setAttribute` regex. The prose rules ([12]/[13]/
  [15]) stay deliberately scoped to avoid a 19-consumer keying rollout, and now
  SAY so in their own output rather than reporting the smaller set in silence.
  *Verified:* re-adding one `title=` to a served module fails the gate; the
  repo-wide run is 0 FAIL, exit 0.

### Shared modules

- **preset-manager 1.0.8 → 1.0.9.** `createPresetBar`'s markup template carried
  four hardcoded English `title=` attributes, replaced with `aria-label`. A
  native title draws a second, OS-rendered tooltip in a language unrelated to
  the page's, competing with the measure-then-pin renderer — contract §4.
  **Markup-only, and only inside the `createPresetBar` factory:** every current
  consumer constructs `new PresetManager({...})` directly, so no rendered UI
  changes anywhere. Propagated to all 18 other consumer copies, which fall into
  three groups: **7 are git-ignored artifacts** regenerated from canonical by
  `ouaricon_add_module`'s `configure_file(COPYONLY)` at configure time, so they
  pick the fix up automatically and carry no commit; **5 tracked copies** at
  canonical 1.0.8 took a clean overwrite; and **6 tracked copies had diverged
  24–184 lines** and took a surgical four-attribute edit instead, preserving
  that divergence. The surgical arm is not merely conservative — none of those
  six calls `ouaricon_add_module` for this module, so their committed copy is
  authoritative and a blind overwrite would have silently discarded real local
  changes rather than being corrected on the next configure.

- **O-Lyrica** carried the same §4 defect in its own inline module —
  `title="${interval.toFixed(1)}¢"` on interval-matrix cells, written into a
  template literal and therefore invisible to the old scan. Converted to
  `aria-label`, following the precedent that plugin's own v2.4.0 sweep set for
  four other native titles. The precise cent value stays available to assistive
  tech; the visible text was already rounded.

### Testing

- **New:** `tests/render-harness/` — a console app gating the three DSP fixes on
  rendered audio rather than on flags. G1 drives `(2,1)` and `(1,2)` and requires
  refusal (driving `(2,2)` alone would pass on the broken build). G2 measures the
  gain step on a DC bed with every band off, so the gain stage is the only thing
  that can produce a first difference, and derives its threshold from the ramp
  length — it also asserts the gate separates ramped from stepped by more than
  10×, so a pass cannot come from measuring something too small to tell them
  apart. G3 calibrates its click threshold against the signal's own slew measured
  on the same render, and drives a loud→quiet→re-enable sequence for the stale
  state. Off by default; `-DOUARICON_BUILD_TESTS=ON`.
- `ui_tip_render_check.js` 415 → 428 assertions.
- `tests/i18n-states.json` 2 → 3 states.

### Known limitations

- The `check-i18n` prose rules ([12]/[13]/[15]) still do not scan served shared
  modules. That is a deliberate scope boundary, not an oversight: firing them
  unrestricted turns seven plugins red on unkeyed English captions
  (O-Bassoon/O-Bowed/O-Reed/O-Wind 39 each, O-Contrabass 46, O-ReverseDelay 6,
  O-Marimba 5), which is a shared-module keying rollout rather than part of this
  fix. The gate now names the modules it is not prose-scanning on every run.
- The other 18 `preset-manager.js` consumers received the markup fix but have
  **not** been re-versioned or rebuilt; they pick the change up on their next
  build. Six of them remain diverged from canonical and should go through
  `/module-upgrade` on their own schedule.

## [1.5.0] - 2026-09-05

> **This release ships versions 1.2.0 through 1.5.0.** The last tagged release
> was v1.1.11 (2026-08-02); the six versions below were authored since and are
> all contained in these binaries. In order:
>
> - **1.2.0** — the page speaks French. A settings popover, seventeen i18n keys,
>   `getUiLanguage` / `setUiLanguage` with session persistence.
> - **1.3.0** — hover-help, in both languages. Thirteen tooltip entries covering
>   fifteen of the sixteen parameters, plus a tooltip renderer and its CSS.
> - **1.3.1** — French copy revised against the suite glossary; its lint went
>   from 20 findings to 0. The preset button reads **ENREG.**, not SAUVER.
> - **1.4.0** — a hover-help switch in the settings popover, persisted under
>   `oaeq.tipsEnabled`. Default ON, so an existing user's plugin is unchanged.
> - **1.4.1** — the French hover-help caption becomes **`Infobulles`**, with
>   every dependent sentence re-agreed to feminine plural.
> - **1.5.0** — Simplified Chinese, described below.
>
> Full detail for each is in `CHANGELOG.md`.

**Simplified Chinese.** Every caption, hover-help body and accessible name now
renders in `zh-Hans` alongside English and French — 35 entries, 49 emitter rows.
MINOR: a language is added and two English hover-help bodies are corrected; no
parameter, range, type or state format changed, and no DSP was touched.

### Added

- **`zh-Hans` on all 35 entries.** `LANGUAGES` reads three, the selector carries
  a third `<option>` as numeric character references, and the
  `languageCode` / `languageIndex` codec is three-way and pure ASCII. WIDE, MED
  and TIGHT stay English inside the Chinese Q bodies: they are
  `AudioParameterChoice` strings and they are what the button on the page says.

  **DISCLOSED QUALITY LEVEL: `reviewed: 'bt'`, not `'native'`.** This project
  has no native Chinese reader, so the ship bar for the Chinese rollout is a
  BACK-TRANSLATION: an independent pass that has never seen the English source
  renders the shipped Chinese back into English, and the drift is read in a
  language this project can read. **Every one of the 49 rows was carried through
  that pass and read**, with `--verbose` rather than the twelve the default view
  prints.

      forward   claude-opus-5 forward draft, quick task 260905-acr, 2026-09-05
      reverse   claude-sonnet-4-5, fresh non-interactive session, no tools,
                cwd outside the repo, 2026-09-05

  `reviewed: 'native'` stays OPEN and is printed by the lint's R1 rule on every
  run. Nothing here has been read by a native speaker.

  Every drift accepted with a written reason. The recurring shape is
  ABBREVIATION EXPANSION — LMF and HMF come back as "Low-mid" and "High-mid",
  because the Chinese spells out what the English initialises — and no drift
  could send a reader to a different control.

### Fixed

- **Forty font carriers, in THREE different forms, all of them the
  bare-generic shape — and a sweep of the obvious form finds fifteen.**
  - **15 CSS declarations** reading `font-family: Garamond, serif`.
  - **24 SVG `font-family=` presentation attributes**, one per frequency tick on
    the four dial faces. Not a declaration and not in a stylesheet, so the
    property-and-colon grep never sees them; `getComputedStyle` does reach SVG
    `<text>`, which is the argument for measuring rather than grepping. All 24
    hold nothing but ASCII figures, so they took the face-naming repair and no
    CJK tail — there is no Han on them to fall back for.
  - **1 `<button>` with no declaration at all.** `.gear-btn` declares no
    `font-family`, and a form control does not inherit one — the UA stylesheet
    gives it Arial. Invisible to a sweep of every declaration AND every
    attribute in the file; only computed style finds it.

  Garamond is absent on this machine (0 family matches for Garamond, EB Garamond
  and Adobe Garamond Pro, against 4 installed families for Times New Roman), so
  all 39 Garamond carriers reached their trailing generic for **every codepoint
  they rendered, Latin included** — and a bare generic resolves against the
  DOCUMENT language. The ASCII on the whole page, every dial tick included,
  changed face and metrics the moment the page language changed. **That half of
  this fix holds no Chinese at all and was invisible to every gate in this repo
  until a third language arrived.** Arial is kept first on the gear so the
  English and French Latin metrics do not move.

- **Two hover-help bodies asserted things that were no longer true**, in English
  and French both: the gear body claimed the panel holds one control, and the
  language body ended with a fixed pair. Both deleted rather than extended.

- **Four elements grew 3 to 4 px on the Chinese arm** from `line-height: normal`
  resolving against a taller Chinese face. Pinned to their own measured English
  boxes, unitless and scoped, no-ops in English and French.

- **`tests/ui_tip_render_check.js` carried THREE language defects where two were
  expected.** It iterated a three-element array literal naming its languages, in
  the off-census spelling. Its assertion `[5]` required the non-English pass to
  be strictly TALLER, which the Chinese arm refutes — measured, **0 grew and 5
  SHRANK**. And a third, found only by running it: `if (drivenStates.length <=
  2)` guarded the height recording, where the literal `2` meant "the number of
  languages, not counting the return pass" — a third place the file spelled its
  own language count, in a form no census greps for. With three languages the
  Chinese pass recorded no heights at all and assertion 5 failed reporting that
  nothing had moved, on a page where five things had. All three are now derived
  from the table's own export, behind a derive-or-abort guard. The gate runs 415
  checks where it ran 260.

### Verification

`check-i18n` exit 0; `check-ui-labels` exit 0, 0 FAIL on all three arms;
`i18n-zh-lint` 0 findings, `BELOW SHIP BAR 0`; `i18n-fr-lint` exit 0; the tip
gate 415 PASS. Zero Han under `Source/**` with the positive control fired.
`auval -v aufx OuAE OuDv` AU VALIDATION SUCCEEDED.

## [1.4.1] - 2026-09-03

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


## [1.4.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `oaeq.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 133.2..188, 170 x 54.8 px — byte-identical in English and French** — inside a 920 x 220 frame, clearing the frame's top by 133 px. The 170 px width pin stands unchanged. The switch face measures **42.00 x 14.80 px in both languages**: *On* and *Marche* both fit inside the 42 px floor.
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


## [1.3.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **14 of 43 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint, which went from **20 findings to
  0** (`--strict` exit 0). Terminology: the preset button now reads **ENREG.**
  rather than SAUVER — *sauver* is a calque, and the glossary's abbreviation
  measures **35.50 px against SAUVER's 39.25**, so it is 3.75 px NARROWER inside
  the same 62 px pin; the save button's accessible name follows it to
  *Enregistrer un préréglage dans un fichier*, and the preset dropdown's takes
  the glossary's settled *Cliquer pour parcourir les préréglages*. Typography:
  no-break spaces before every `:` and `;` and between every number and its unit
  (*500 Hz*, *−12,0 dB*), ten tooltip bodies. Grammar and idiom: a misplaced
  relative clause in the Analog tip, a missing verb in both Q tips, *une seule
  commande* for *un seul contrôle*, ranges as *de X à Y*, and the Q values
  separated by semicolons because the French decimal comma made *0,5, 1,0*
  ambiguous.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Unchanged, deliberately

- **`reviewed: false` on all 30 French entries.** That flag means a native
  speaker read the string; this pass is a second machine reading against a
  glossary and a lint, and it is recorded in the `i18n.js` header instead.
- **The four band tip titles and the Analog tip title keep their caption's
  abbreviation** (*LF Plat.*, *HF Plat.*, *Analog.*) rather than expanding in
  the roomier tooltip: on this page the caption IS the switch being pointed at.
- **Off / On stay English in the bodies** — they are the words the host's
  automation lane shows for the five boolean parameters.
- **LMF and HMF stay `sameAsEn`** — they are the band abbreviations silk-screened
  on the consoles sold in France.

### Known

- **WCAG 2.5.3 label-in-name now holds by STEM on the save button.** *ENREG.* is
  not a substring of *Enregistrer un préréglage dans un fichier* because the
  period ends it; containment holds on *Enreg*. The full word is 66.95 px
  against a 48 px content box, so no caption was invented to close it.
- **The `TIGHT` Q option clips its own right edge in ENGLISH** — Stage M
  decision item 10, re-measured here at **1.88 px past the toggle's content box,
  IDENTICALLY in both languages, on both `#lmf_q` and `#hmf_q`**.
  `.three-way-option` is `flex: 1` without `min-width: 0`, so the three options
  do not fit the 108 px toggle and `overflow: hidden` takes the difference off
  the last one. Pre-existing since v1.2.0, unreachable by French
  (`WIDE / MED / TIGHT` are `I18N_EXEMPT` verbatim parameter option strings, so
  the French pass cannot reach those nodes at all) and fixable only in CSS,
  which Stage N does not touch.

## [1.3.0] - 2026-08-30

### Added — hover-help, in both languages (Stage M batch M2)

- **Thirteen tooltip entries in `Source/ui/public/js/i18n.js`**, each with an
  English and a French `{t, b}`, covering **fifteen of the plugin's sixteen
  parameters** plus the gear button and the language selector. Every French
  string is a machine draft flagged `reviewed: false`; no native speaker has
  read one.
- **A tooltip RENDERER and its CSS in `index.html`.** This is not a content-only
  change and that was measured before a line was written: this page had no
  `#tooltip` node, no `.tooltip` rule and no hover handler, and `applyI18n()`
  only ever writes `data-tip-title` / `data-tip` ATTRIBUTES. Authoring thirteen
  bodies and binding them, with nothing else, ships thirteen INVISIBLE strings
  past three green gates. Negative-controlled: with `setupTooltips()` disabled,
  `check-i18n` prints ALL CHECKS PASS and `check-ui-labels` is **byte-identical
  to its pre-change baseline**, while the new render gate fails **204**
  assertions.
- **`tests/ui_tip_render_check.js`** — 308 assertions, the seat where a tip that
  never appeared FAILS instead of passing quietly. It drives every anchor in
  `en`, `fr` and `en` again, asserts the rendered title and body are BYTE-EQUAL
  to the table (not "contains"), and asserts the tip rect is inside 920 x 220 on
  all four edges at every anchor in both languages.
- **`.planning/params.tsv`** and the `OUARICON_BUILD_TESTS` param-dump wiring in
  `CMakeLists.txt` / `PluginProcessor.cpp`. The processor's
  `#include "PluginEditor.h"` moved behind `#if JUCE_WEB_BROWSER` above
  `createEditor()`, with a `GenericAudioProcessorEditor` fallback, because the
  param-dump console target compiles this TU with `JUCE_WEB_BROWSER=0` and no
  editor sources. Under a normal build `JUCE_WEB_BROWSER=1` and behaviour is
  byte-identical to v1.2.0.

### Findings — thirteen tips for sixteen parameters, and neither gap is an omission

- **The four dual knobs carry TWO parameters each on ONE hover target, and the
  page forces it.** `.knob-outer` (frequency) and `.knob-inner` (gain) are BOTH
  `pointer-events: none`, so neither is hoverable and neither can carry a tip;
  the only node that receives a pointer event is their parent
  `.dual-knob-container`, which resolves outer-vs-inner from the cursor's
  DISTANCE FROM THE CENTRE rather than from a child boundary. So each band gets
  one tip naming both rings. A second `TIP_BINDINGS` row on the same node would
  SILENTLY OVERWRITE the first while `check-i18n` reported two bound tips — the
  gate now asserts all thirteen bindings land on distinct nodes.
- **`output_gain` has no control on the page at all.** Deliberate and already
  documented in the source: `PluginProcessor.cpp:88` records it as removed in
  the v1.0.5 UI simplification (review item IN-01), kept because it is
  host-automatable and because factory presets set it. Host-reachable,
  page-unreachable. **No control was added to satisfy the count** — that is a
  feature change with a geometry cost on a 220 px frame — and no body was
  authored, because a body with nothing to bind is an ORPHAN.
- **"Bind to the ids the UI already uses" is wrong here twice.** `#lf_freq_knob`
  and `#lf_gain_knob` exist and are the naive reading; both are
  `pointer-events: none`, so a tip bound to either could never open. And the
  addressable container is 65 x 65 inside an 85 x 85 `.dual-knob-wrapper` that
  also carries the frequency scale, so the wrapper walk widens the hover cell by
  71% (4225 -> 7225 px2) without moving a pixel of paint.
- **The chrome binds BARE.** `#gear-btn` and `#lang-select` share
  `.settings-cluster`, whose own rect is the gear's 18 x 18 box, so a wrapper
  walk on either would make hovering the language selector open the GEAR's tip.

### Findings — the clamp, and a line in the renderer family that cannot fire

- **This page is NOT its batch's O-Chorus.** 220 px is short, but a 204 px well
  against tips that run 51.1 to 78.1 px leaves 126 px of headroom, and the
  anchors sit at y 62..184 rather than filling the frame. Measured over 26
  placements in both languages: **8 of 26** overflow the naive cursor offset
  vertically and are placed by the flip; **0 of 26** are outside on both sides of
  it. O-Chorus at 700 x 125 reports 20/20 and 17/20.
- So the re-clamp ships UNEXERCISED by the shipped copy, and `[4c]` drives it
  deliberately rather than relying on it: a searched 800-character plant on
  `#lf_on` renders 145.6 px tall, which is outside BELOW the cursor (bottom
  224.1 past 212) **and** outside ABOVE it (top −95.1 past 8). Deleting the
  `Math.max(M, ny)` floor alone puts that tip at −95.1, **103 px off the top of
  the page**, and `[4c]` reports it.
- **The `ny = innerHeight - M - r.height` line in this renderer family is
  unreachable BY CONSTRUCTION, on every frame.** After a flip
  `ny = y - h - 12`, so `ny + r.height` collapses to `y - 12` — it no longer
  mentions the tip's size at all — and that can exceed `innerHeight - M` only
  for a cursor outside the viewport. The same collapse holds on the x axis
  (`nx + r.width` becomes `x - 14`). What actually re-clamps a flipped result is
  the `Math.max(M, …)` floor beneath it. The lines are KEPT so this port stays
  byte-shaped with the other ten copies, and DOCUMENTED because O-Chorus's copy
  credits them with a placement the floor is making.
- **No `drag.active` flag is needed here**, and that is a property of this page
  rather than an omission: `setPointerCapture(e.pointerId)` retargets every
  pointer event for the rest of a knob drag, so a drag crossing into a
  neighbouring cell fires no boundary event there. Driven and asserted.

### Geometry

**Zero elements moved, and no pin was added, so none is owed a negative
control.** `check-ui-labels --plugin O-AnalogEQ` output after this change is
**byte-identical** to its v1.2.0 baseline — moved-before 0, moved-after 0 —
which is the expected result of adding a `position: fixed`, `visibility:
hidden` surface: it is out of flow, and a fixed box at 0,0 has the same
rectangle in both languages.

### Copy

- The body ranges come from `.planning/params.tsv`, a runtime walk of
  `getParameters()`. **Eleven of the sixteen parameters carry a real `label`**
  (`Hz` on every `*_freq`, `dB` on every `*_gain`), each agreeing with the
  page's own formatter — the brief's expectation that `label` is usually empty
  is false on this plugin.
- **One range had to be recovered from the formatter anyway:** `hf_freq` dumps
  `textAtMax 20000.0`, but `index.html:970` renders
  `hz >= 10000 ? (hz / 1000).toFixed(1) + 'k' : Math.round(hz)`, so the top of
  that knob READS `20.0k Hz` and never `20000 Hz`. The body quotes what the user
  sees.
- **The numbers are spelled differently in the two languages, on purpose.** A
  tooltip body is prose and takes French convention — decimal COMMA, U+2212 for
  the minus — while the READOUT keeps its point, because D-03 exempts the
  readout NODE. So `−12.0 to +12.0 dB` here is `−12,0 à +12,0 dB` there while
  `#lf_db` still renders `-12.0 dB` in both languages.
- **`WIDE` / `MED` / `TIGHT` and `Off` / `On` stay English inside the French
  sentences.** They are `AudioParameterChoice` options, exempt on the page under
  D-01 arm 1 so the face and the host automation lane agree; inside a body they
  are being NAMED rather than displayed, so the sentence around them is French
  and the words are not.
- **`#gear-btn`'s body describes only what that popover actually holds** — the
  language selector and nothing else. This plugin has no hover-help on/off
  toggle, and the wording two other plugins in the suite use would promise one.

### Not changed

No parameter IDs, ranges, types, defaults or DSP behaviour. No layout change, no
new CSS pin, no new file in `juce_add_binary_data` SOURCES — the renderer and its
styles went into `index.html` and the copy into the already-embedded `js/i18n.js`.

## [1.2.0] - 2026-08-28

### Added — the PAGE speaks French (Stage K batch K2, canon v2)

- **`Source/ui/public/js/i18n.js`** — seventeen keys: ten visible captions and
  seven accessible names. Embedded in `juce_add_binary_data` SOURCES **and**
  served from a `getResource()` branch, in this same commit (check-i18n
  assertion 8 exists because a file embedded but not served is a 404 that
  presents as a page stuck in English and nothing else).
- **A settings popover** in the empty bottom-left margin carrying the language
  selector. ONE row: this plugin has no hover-help to switch on or off.
- **`getUiLanguage` / `setUiLanguage`** native functions and session
  persistence. The page PULLS once at init; nothing is pushed from the editor
  constructor, which would race the WebView's load. `uiLanguage` is a
  non-parameter property on the APVTS state tree read back through an
  `isVoid()` guard, because the XML round-trip rebuilds every property as a
  `var` over the attribute STRING.
- `plugins/O-AnalogEQ/tests/i18n-states.json` — drives the popover and the
  preset dropdown so `check-ui-labels` measures 9 of 9 keyed elements.

**No hover-help was authored.** `TIP_BINDINGS` and `I18N` are both empty, which
is this plugin's correct state and which assertion 2 reports as "0 tip(s)
bound". Authoring that copy is Stage M's job.

### Changed

- **The five native `title=` attributes on the preset bar are DELETED**
  (contract §4) and their existing English moved verbatim into
  `data-i18n-aria`. No new prose was invented. Both button names contain their
  own visible caption as a prefix, so label-in-name holds in both languages
  (WCAG 2.5.3). `#presetName` is the one place that rule cannot hold — its
  visible text is a runtime preset name, exempt under D-02.
- **`#savePreset` / `#loadPreset` pinned to 62 px.** Unpinned, SAUVER and
  CHARGER widen the right-anchored flex bar and shove `#presetBar`,
  `#prevPreset`, `#presetName` and `#nextPreset` left by 34.4 px; removed
  alone, `check-ui-labels` assertion 7 reports exactly those four. 62 px is
  O-Chorus's and O-DigiDelay's number, kept so the suite's preset bar is one
  shape. Its cost is that the ENGLISH cluster now sits 44.33 px further left,
  in a header whose middle is empty paper.
- **`.band-label { white-space: nowrap }`** — NOT a geometry pin and not
  claimed as one; its negative control passes on the shipped strings. It ships
  because it converts this page's third failure mechanism, which BOTH gate
  assertions are structurally blind to, into one that assertion 4 catches. See
  the comment in `index.html` and the note below.

### Geometry, measured at 920 x 220 over every rendered box, at 180 ms and 1.7 s

The page HOLDS STILL: zero of 155 elements differ between the two settle times
at a 0.01 px tolerance, in either language, in either popover state. Nothing
free-runs off the wall clock in the harness, so there is no assertion-7
animation NOTE on this plugin and none was expected.

- **English v1.1.11 → v1.2.0:** 6 of 147 elements moved, 8 added (the settings
  cluster), 0 vanished, document scroll extent 920 x 220 unchanged. All six
  moved are the preset bar and all six are the one pin. ZERO band labels, ZERO
  knob or notch elements, ZERO VU-meter elements, zero title movement.
- **French vs English at v1.2.0: ZERO elements moved**, in the default state
  and with the preset dropdown open — not one of 155 boxes, including the keyed
  captions themselves, because every one of them has a pinned width. With the
  settings popover open, ONE element moves and it is the keyed
  `.settings-label` shrinking 14.44 px (LANGUAGE → LANGUE); zero non-label
  elements move, which is the `.settings-popover { width: 170px }` pin working.
  Removed alone, assertion 7 reports the panel, its row and `#lang-select`.

### The three cliffs, and the one that is invisible

| | Control | Budget | Mechanism | Caught by | Blind |
|---|---|---|---|---|---|
| A | `#analog` | 57.00 px | SPILL | `[4][fr]` | `[7]` |
| B | `#savePreset` / `#loadPreset` | — | PUSH | `[7]` | `[4]` |
| C | `.band-label` | 67.00 px | WRAP | **nothing** | both |

All three were PLANTED and watched, because "no failures" is otherwise
indistinguishable from a sweep that cannot see them:

- **A.** French `ANALOGIQUE` fails `[4][fr] "ANALOGIQUE" 66.7>57.0` and moves
  nothing at all.
- **B.** Removing the 62 px pin fails `[7]` with four elements at dx=-34.4
  while every `[4]` check stays green.
- **C.** French `PLATEAU BF` on `#lf_on`, with `nowrap` removed, passes
  **EVERY ASSERTION** while the caption grows 21 → 34 px and reaches y=86 into
  the knob ring that begins at y=75. `.band-label` is `position: absolute` with
  an inline `width: 85px` and no fixed height, so a wrapped caption exceeds
  neither its content width nor its own grown content height, and being both
  the keyed element and absolutely positioned it pushes no sibling. With
  `nowrap` present the same string fails `[4][fr] "PLATEAU BF" 81.8>67.0`.

### French, all machine drafts (`reviewed: false`)

Rendered widths against each control's measured content box:

```
LF SHELF  63.03 -> LF PLAT.  57.44   9.56 spare   (67.00 box)
LMF       28.41 -> LMF       28.41   sameAsEn
HMF       29.63 -> HMF       29.63   sameAsEn
HF SHELF  64.25 -> HF PLAT.  58.66   8.34 spare
ANALOG    42.33 -> ANALOG.   45.30  11.70 spare   (57.00 box)
Level     34.16 -> Niveau    41.33  66.67 spare   (108.00 box)
SAVE      24.52 -> SAUVER    39.25   8.75 spare   (48.00 box, pinned)
LOAD      27.16 -> CHARGER   46.80   1.20 spare   TIGHTEST SHIPPED
Language  55.31 -> Langue    41.08   SHRANK
```

TWO of the ten SHRINK and TWO do not change at all — a clip-only check would
have certified this page.

`LF` / `LMF` / `HMF` / `HF` are kept verbatim: they are the band abbreviations
silk-screened on the French market's own consoles, so translating them would
make the plugin less legible, not more. Only SHELF — the filter TYPE — is a
word, and it becomes PLAT. Every fuller form is past the 67.00 px wrap cliff
(PLATEAU BF 81.77, PLATEAU HF 82.98, BAS MEDIUM 85.63, HAUT MEDIUM 97.14) and
`MED.HAUT` (70.77) stays on one line only to spill 3.77 px. ANALOGIQUE (66.70)
would need the button widened from 75 to 85 px — a layout change caused by
French, which this page did not need. OUVRIR (37.75) is the reviewer's lever if
CHARGER's 1.20 px is judged too thin on Windows metrics.

### Not changed, and why

- **`WIDE` / `MED` / `TIGHT` do not translate.** They are the three
  `lmf_q` / `hmf_q` `AudioParameterChoice` options declared verbatim as
  `juce::StringArray { "WIDE", "MED", "TIGHT" }` (`PluginProcessor.cpp:56,
  :69`) — D-01 arm 1. A French face reading MOYEN against a DAW automation lane
  offering MED is the divergence that arm exists to prevent. The consequence
  for this release is that `.three-way-option`'s `white-space: nowrap` is never
  reached by a French string.
- **Both three-way Q toggles clip their own TIGHT option in ENGLISH**, and this
  release does not fix it. `.three-way-option` is `flex: 1` without
  `min-width: 0`, so the three items sit at their min-content widths — 35.95 +
  32.83 + 39.09 plus two 1 px gaps = 109.87 px inside a 108 px content box —
  and `.three-way-toggle { overflow: hidden }` clips 1.87 px off TIGHT's right
  edge. Pre-existing since the control was authored, present on `#lmf_q` and
  `#hmf_q` alike, invisible to both gates because these nodes are exempt and
  never keyed. Fixing it means changing English geometry on an exempt control
  for a non-localization reason, which is a layout decision.
- **The vendored `modules/preset-manager.js`** still carries four native
  `title=` attributes inside a `createPresetBar()` factory this page never
  calls. Dead markup in a SHARED module, so a local edit is reverted by
  `/module-upgrade`.

No parameter IDs, ranges, types, defaults or DSP behaviour changed.

### Verification

`check-i18n --strict-v2` exits 0, 39 assertions, canon v2.
`check-ui-labels` exits 0, 75 assertions across three states, 9 of 9 keyed
elements visible, no page error, every resource served.
`boot-all-uis` 41/43 clean, verdict identical to HEAD's (O-Bowed and O-Reed
fail on an unrelated pre-existing `Unexpected token export`); O-AnalogEQ reads
text 24 → 27, title 5 → 0, aria 0 → 8, i18n 0 → 9, and the repo title total
falls 453 → 448.
`auval -v aufx OuAE OuDv`: AU VALIDATION SUCCEEDED. Both installed bundles
report 1.2.0 and the binary carries the table, both native function names and
`/js/i18n.js`.

## [1.1.11] - 2026-08-02

### Changed
- Added AGPL-3.0 license notice headers to all Ouaricon-authored source files. No functional changes.

## [1.1.10] - 2026-06-30

### Fixed (code-review info items)
- **IN-01 (hidden `output_gain`):** Documented that `output_gain` is intentionally not
  surfaced in the UI (the output knob was deliberately removed in the v1.0.5 simplification).
  It stays a host-automatable parameter (default 0 dB) set by some factory presets. Added a
  code comment so it isn't mistaken for a missing binding. No behavior change. (Kept hidden.)
- **IN-02 (double-click reset):** Reset now restores each frequency knob's true APVTS default
  (100 / 500 / 2000 / 8000 Hz) via the skew inverse, instead of normalised 0.5 (which was
  ~77 Hz for LF, not the real default). Gains already reset correctly (0 dB = 0.5).
- **IN-03 (dead code):** Removed the unused `currentParamName` variable and its assignments
  from `setupDualKnob` (`currentState` already carries the reference).
- **IN-04 (unbounded poll):** `preset-manager` module — `_waitForNative()` now bounds its
  poll (100 × 50 ms = 5 s) and logs an error instead of hanging preset init forever if the
  JUCE backend never appears.
- **IN-05 (fragile `confirm()`):** `preset-manager` module — `promptDelete()` now prefers an
  optional `onConfirmDelete` hook (reliable native/in-DOM dialog) and guards the
  `window.confirm()` fallback, which is a silent no-op/throw in some JUCE WebView backends;
  aborts fail-safe (no accidental delete) and logs when no confirmation mechanism exists.

### Notes
- Closes all remaining items from the 2026-06-30 code review (`CODE_REVIEW.md`):
  CR-01 + WR-01 (1.1.8), WR-02/03/04 (1.1.9), IN-01..IN-05 (this release).
- IN-04/IN-05 fixes were made in the **shared `preset-manager` module** (bumped to 1.0.1);
  O-AnalogEQ's copy is synced. The other 10 dependent plugins can adopt it via `/module-upgrade`.

## [1.1.9] - 2026-06-30

### Fixed
- **WR-02 (zipper noise on automation):** Frequency and gain were read once per block
  and coefficients jumped straight to the new value, producing audible zipper/clicks when
  automating or dragging — worst on the skewed frequency ranges where a small knob move is a
  large Hz jump. Each band's frequency/gain is now a `juce::SmoothedValue` (30 ms linear ramp,
  seeded to the current parameter in `prepareToPlay` so nothing swoops on load). While a band
  is ramping, its coefficients are rebuilt every 32 samples so the response glides to the
  target. Root cause: unsmoothed per-block coefficient steps.
- **WR-02 / CR-01 (RT-safety preserved):** The per-chunk rebuild uses
  `juce::dsp::IIR::ArrayCoefficients::make*` (returns a stack `std::array`) assigned into the
  existing filter state, instead of the allocating `Coefficients::make*` factories. The math is
  identical (the factories just wrap `ArrayCoefficients` in a heap allocation), so the sound is
  unchanged, and the audio thread never allocates — even mid-automation. When no band is moving,
  the block still runs in a single pass with no coefficient recompute (CR-01 steady-state path).
- **WR-03 (Nyquist clamp):** Every band's cutoff is now clamped to `0.99 × Nyquist` before
  building coefficients. Previously `hf_freq` (up to 20 kHz) and `hmf_freq` (up to 8 kHz) were
  passed straight through, producing degenerate/NaN coefficients at host sample rates below
  ~40 kHz. Verified via `auval` render tests at 22050 Hz and 11025 Hz. Root cause: unbounded
  cutoff vs. sample rate.
- **WR-04 (FileChooser use-after-free):** The async save/load `FileChooser` completion lambdas
  captured `this` and dereferenced the processor. If the editor window closed while the OS
  dialog was still open, the callback fired against a destroyed editor. Both callbacks now
  capture a `juce::Component::SafePointer` and bail early if the editor was deleted. Root cause:
  raw `this` capture across an async native dialog.

### Notes
- Closes the remaining WARNING items from the 2026-06-30 code review (`CODE_REVIEW.md`).
  CR-01 and WR-01 were fixed in 1.1.8. Only the IN-* info items remain (all benign/documented).

## [1.1.8] - 2026-06-30

### Fixed
- **CR-01 (RT-safety, critical):** `processBlock` rebuilt all four bands' IIR coefficients
  every block via the allocating `IIRCoefficients::make*` factories, heap-allocating on the
  audio thread even when no parameter changed. Now each band's coefficients are recomputed
  only when its frequency/gain/Q inputs actually change (guarded against cached last-seen
  values), making the steady-state playback path allocation-free. Cached sentinels are reset
  in `prepareToPlay` so coefficients still rebuild on the first block after prepare and on a
  sample-rate change. DSP output is unchanged (same `make*` formulas on change).
  Root cause: unconditional per-block coefficient rebuild.
- **WR-01 (display correctness):** Frequency tooltips showed wrong Hz because the JS formatters
  mapped the normalised knob value linearly, ignoring the C++ `NormalisableRange` 0.3 skew
  (e.g. the 100 Hz LF default displayed as ~301 Hz). Formatters now invert the skew
  (`hz = min + (max - min) * pow(v, 1/0.3)`) so displayed Hz matches the actual filter
  frequency across all four bands. Gain readouts were already correct (linear skew) and are
  unchanged. Root cause: JS display math did not mirror the parameter's frequency skew.

### Notes
- Both issues from the 2026-06-30 code review (`CODE_REVIEW.md`). Remaining review
  items (WR-02 coefficient smoothing, WR-03 Nyquist clamp, WR-04 FileChooser lifetime, and the
  IN-* info items) are not addressed in this patch.

## [1.1.7] - 2026-02-09

### Added
- Preset system with save/load functionality

### Changed
- UI improvements: moved title left, presets right
- EQ algorithm upgrades

## [1.1.4] - 2026-02-05

### Changed
- Added licensing module integration (compile-flag gated, OFF for local dev)
- Added branding variables for company name, manufacturer code, and dev suffix
- Added WebView2 backend support for Windows compatibility

### Fixed
- Windows CI build failure - added preprocessor guards for `withResourceProvider` when WebView2 SDK is missing
- Added `JUCE_USE_WIN_WEBVIEW2=1` compile definition

## [1.1.3] - 2026-02-05

### Fixed
- **Windows CI build failure** - `withResourceProvider` is not available when WebView2 SDK is missing
  - Added `#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE` preprocessor guards around `withResourceProvider` and `getResourceProviderRoot()` calls in PluginEditor.cpp
  - Added WebView2 NuGet package installation step to Windows CI build workflow
  - No functional change on macOS/Linux where resource provider is always available

## [1.1.2] - 2026-02-05

### Changed
- Published release with code signing and Apple notarization via GitHub Actions CI/CD

## [1.1.1] - 2026-02-04

### Changed
- **Eliminated duplicated filter coefficient logic** - Removed `updateFilterCoefficients()` and 8 `previous*` change-detection members; coefficients now set directly in `processBlock()` via a shared `dBtoGain` lambda and a `constexpr qValues[]` lookup table
- **Replaced `ProcessSpec` member with `double currentSampleRate`** - Only the sample rate was needed between `prepareToPlay` and `processBlock`; `ProcessSpec` is now a local in `prepareToPlay`
- **Defaulted empty destructor and `releaseResources()`** - Removed boilerplate empty bodies
- **Added `StereoFilter` type alias** - Shortened repeated `ProcessorDuplicator<IIR::Filter<float>, IIR::Coefficients<float>>` declarations to a single alias
- **Removed unused `needsUpdate` variable** from `processBlock()`
- **Consolidated parameter layout** - Reduced verbose per-parameter comments to band-level comments; one `layout.add` call per line
- **Merged `setupBandLabelToggle` and `setupToggle`** in index.html into a single `setupToggle(element, state, className, activeWhen)` function
- **Removed `getQValueFromChoice()` helper** - Replaced with `constexpr qValues[]` array indexed directly

### Code metrics
- PluginProcessor.cpp: 399 → 195 lines (51% reduction)
- PluginProcessor.h: 82 → 55 lines (33% reduction)
- index.html JS: removed 15 lines of duplicate toggle logic

## [1.1.0] - 2026-01-24

### Changed
- **Renamed plugin** - Changed from "OuariconAnalogEQ" to "O-AnalogEQ"
  - Directory: `plugins/OuariconAnalogEQ/` → `plugins/O-AnalogEQ/`
  - DAW display name: "Ouaricon Analog EQ" → "O-AnalogEQ"
  - Binary names: Now `O-AnalogEQ.vst3` and `O-AnalogEQ.component`
  - Consistent with O-Tremolo and O-DigiDelay naming convention
- Internal CMake target remains `OuariconAnalogEQ` for preset/session compatibility

## [1.0.10] - 2026-01-11

### Changed
- **VU meter moved left** - Shifted 40px left (now at left: 758px)
- **Analog saturation retuned** - Changed from `tanh(x * 1.5) * 1.1` to `tanh(x * 0.5) * 2.0`
  - Now gain-neutral (no volume boost)
  - Adds subtle harmonic warmth/coloration without level change
  - Lower drive preserves dynamics while adding character

## [1.0.9] - 2026-01-11

### Changed
- **VU meter reduced to 80%** - Now 112x112px (was 140x140px) for better proportions
- **VU meter repositioned** - Adjusted position to fit new size
- **Analog button moved right** - Shifted 20px right (left: 620px) for better spacing

## [1.0.8] - 2026-01-11

### Changed
- **VU meter doubled in size** - Now 140x140px (was 70x70px) for better visibility
- **VU meter shifted right** - Positioned at far right edge of window
- **Analog button repositioned** - Now centered between HF shelf dial and VU meter
- **VU meter scale updated** - Larger text and arc for readability at new size

## [1.0.7] - 2026-01-11

### Changed
- **Title on single line** - Widened title container to prevent line break
- **Removed SHELF sublabels** - Cleaned up redundant labels below LF and HF dials
- **Q toggles moved down** - WIDE/MED/TIGHT buttons lowered by 10px for better spacing
- **Flower centered vertically** - Botanical overlay now vertically centered in window
- **Renamed band labels** - LF → "LF SHELF", HF → "HF SHELF" for clarity

## [1.0.6] - 2026-01-11

### Changed
- **Widened Q toggles** - WIDE/MED/TIGHT buttons increased to 110px (fully visible text)
- **Centered band labels** - LF/LMF/HMF/HF toggle buttons now centered above their dials
- **Resized botanical overlay** - Flower reduced to 75% and repositioned to end at far right
- **Updated title** - Changed from "OUARICON ANALOG EQ" to "OUARICON ANALOG EQUALIZER"

## [1.0.5] - 2026-01-11

### Changed
- **Removed output gain dial** - Simplified UI by removing the output gain control
- **Analog button moved under VU meter** - Better visual grouping of output section
- **Band labels are now toggles** - LF/LMF/HMF/HF labels function as on/off buttons
  - Green = band active, brown = band bypassed
  - Removed separate on/off buttons below each dial
- **Improved layout spacing** - Four EQ bands now evenly distributed with VU meter on right

## [1.0.4] - 2026-01-11

### Changed
- **Centered knob layout** - All controls now properly centered in the UI
- **Widened Q toggle buttons** - WIDE/MED/TIGHT labels no longer truncated (95px width)
- **Added OUTPUT/GAIN labels** - Output gain knob now has proper labeling
- **Vertical default position** - All knobs initialize at 12 o'clock (center) position
- **Double-click reset** - Double-clicking any knob returns it to default position
- **Green gradient on outer rings** - Outer frequency rings now have botanical green gradient
- **Added frequency notches** - SVG tick marks around dual-layer knobs show frequency position

## [1.0.3] - 2026-01-11

### Fixed
- **Dual-layer knobs now functional** - Implemented distance-based hit detection
  - Outer ring (>60% from center) controls frequency
  - Inner dial (<60% from center) controls gain
  - Added outer ring indicator for visual feedback
  - Added value tooltips showing both freq and gain values
- **VU meter now responds to actual audio levels** - Marimba-style implementation
  - C++ PluginProcessor calculates peak output level in processBlock
  - PluginEditor uses Timer to emit `outputLevel` events to WebView at 30Hz
  - JavaScript animates needle with ballistic motion (fast attack, slow decay)
  - Needle color interpolates from green (quiet) to red (loud)

## [1.0.2] - 2026-01-11

### Fixed
- **Missing check_native_interop.js** - Added the required JUCE WebView JavaScript bridge file
  - Root cause: JavaScript module `index.js` imports `check_native_interop.js` which sets up `window.__JUCE__.backend`
  - Without this file, no C++ ↔ JavaScript communication was possible
  - Added file to CMakeLists.txt resources and PluginEditor.cpp resource provider

## [1.0.1] - 2026-01-11

### Fixed
- **GUI controls now interactable** - Fixed type mismatch between C++ WebView relays and JavaScript state accessors
  - Root cause: Q parameters (`lmf_q`, `hmf_q`) were using `WebSliderRelay` but JavaScript expected `WebComboBoxRelay`
  - Changed `lmfQRelay` and `hmfQRelay` from `WebSliderRelay` to `WebComboBoxRelay`
  - Changed `lmfQAttachment` and `hmfQAttachment` from `WebSliderParameterAttachment` to `WebComboBoxParameterAttachment`

## [1.0.0] - 2026-01-11

### Added
- Initial release
- 4-band analog-style EQ (LF shelf, LMF bell, HMF bell, HF shelf)
- Per-band frequency, gain, and Q controls (Q on bell bands only)
- Per-band bypass toggles
- Global output gain control
- Analog warmth/saturation toggle
- VU meter display
- WebView UI with botanical paper aesthetic
