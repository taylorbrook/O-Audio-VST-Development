# Changelog

All notable changes to O-Tremolo will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.8.0] - 2026-08-30

Hover-help, in both languages — and the thing that paints it. Eight tooltips (six parameters plus the gear and the language selector), authored in English and French, bound in `TIP_BINDINGS`, and a renderer, because v1.7.0 had nothing on this page that could show one.

**The copy alone would have been invisible.** `applyI18n()` writes `data-tip-title` and `data-tip` onto the bound anchors and stops there; the code that reads those attributes and paints a surface is per-plugin, and this plugin had none. That was measured rather than assumed: with `setupTooltips()` disabled and everything else identical, `check-i18n --plugin O-Tremolo` and `check-ui-labels --plugin O-Tremolo` both still report **ALL CHECKS PASS**, and `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. Eight unpaintable strings would have shipped past three green gates. Only the new `tests/ui_tip_render_check.js` sees it, and it fails 150 assertions.

### Added

- **Eight tooltip entries in `Source/ui/public/js/i18n.js`**, each `{t, b}` in English and French: Speed, Depth, Waveform, Smoothing, Pan Sync, Tempo Sync, the settings gear and the language selector. Every body says what the control does, when to reach for it, and ends with its range and unit.
- **`setupTooltips()` and a `#tooltip` surface** in `Source/ui/public/index.html`, ported from O-simpleFM's delegated cursor-following renderer and dressed in this page's own vocabulary — the `.preset-dropdown` cream plate, a 2 px `#5C4033` border, and the title line in the `#2C3E10` olive the toggle buttons already use. Delegated on `document` (no anchor carries `data-tip` until `applyI18n()` has run, so a setup-time `querySelectorAll` would bind nothing and fail silently); `pointerover`/`pointerout`/`focusin`/`focusout` because those bubble; `createElement` + `textContent`, never `innerHTML`; clamped on all four edges at 8 px **after** the flip; `pointer-events: none`; `position: fixed` + `visibility: hidden` at rest. Called AFTER `initI18n()` inside the same `try/catch` at the foot of `DOMContentLoaded`.
- **A keyboard latch on the focus arm.** A mouse click on a `<button>` focuses it, so an unconditional `focusin` rule parks a tip on screen after every click — measured on two sibling plugins at 146 x 35 px and 161 x 29 px over the popover the click had just opened. `lastInputWasPointer`, set by any `pointerdown` and cleared by any `keydown`, gates it. `:focus-visible` was rejected as the discriminator: Chromium reports it false for a programmatic `.focus()` after a click, so a gate driving focus directly would measure "no tip" and record that as correct.
- **`tests/ui_tip_render_check.js`** — 186 assertions, the only gate in this repo that can see a rendered tooltip. Drives the real page at the shipping 600 x 400 read out of `PluginEditor.cpp`, hovers all eight anchors in `en`, `fr` and `en` again, asserts the rendered title and body are **byte-equal** to the table (not "contains" — a stale `.tip-title` passes a contains check), and asserts the tip rectangle is inside the frame on all four edges. Both halves of the latch are asserted separately, so the feature cannot silently decay into "focus never shows a tip".
- **`.planning/params.tsv`** — the runtime parameter inventory, and the `OUARICON_BUILD_TESTS` / `ouaricon_add_param_dump()` wiring in `CMakeLists.txt` that produces it. A regex over `createParameterLayout()` is not authoritative; only a walk of `getParameters()` on a constructed processor is.

### Changed

- **`PluginProcessor.cpp` includes `PluginEditor.h` behind `#if JUCE_WEB_BROWSER`**, directly above `createEditor()`, with a `GenericAudioProcessorEditor` fallback. The param-dump console target compiles this TU with `JUCE_WEB_BROWSER=0` and no editor sources, so a top-of-file include breaks the link. Under a normal build `JUCE_WEB_BROWSER=1` and behaviour is byte-identical.

### Notes

- **Seven parameters, six tips.** `SYNC_DIVISION_PARAM` has no control of its own on this page — the Speed knob BECOMES its stepper while Tempo Sync is engaged — so there is no element to bind a seventh tip to. An authored body with no binding is an orphan that `check-i18n` assertion 2 fails, so the division is described inside the Speed and Tempo Sync tips where the user actually meets it. No control was added to make the count come out; that would be a feature change with a geometry cost.
- **Four of the eight bindings use a wrapper.** The two knobs bind `.knob-container` and the select and slider bind `.waveform-section` / `.slider-container`, so the hover target is the whole caption/control/readout cell rather than a 60 px circle with its caption outside the hover area. No `tabindex` was added to `.knob-container`: these knobs are mouse-drag only and a tab stop there would add stops for controls the keyboard still could not move, and would pop a tip open mid click-drag.
- **No unit had to be recovered from a formatter.** `params.tsv` carries `Hz` on Speed and `%` on Depth and Smoothing; Speed and Depth agree independently with the page's own `setupKnob()` suffixes, and Smoothing has no readout node at all. The three parameters with an empty `label` are the choice and the two bools, whose range is their option words.
- **The six waveform names stay English inside the French tooltip body**, because they are byte-identical `WAVEFORM_PARAM` `AudioParameterChoice` options and a French user hunting for "Pulse" in the selector must be told "Pulse". The sentence around them is French. The same holds for the musical divisions.
- **Zero geometry movement.** `check-ui-labels --plugin O-Tremolo` output is **byte-identical** to the v1.7.0 baseline in all three driven states — `moved=0` before and after, the same 10 `[data-i18n]` elements, the same 4 decoration elements in the `[8b]` count. No pin was added and none was needed: the surface is `position: fixed` + `visibility: hidden` + `opacity: 0` at rest, which that gate's visibility predicate rejects, so an unshown tip is neither measured nor swept for text. **No pin means no negative control is owed.**
- **The gear tip describes only what the popover holds** — the language selector and nothing else. O-Tapestop's wording promises a hover-help on/off toggle this plugin does not have, and a tip that lies is worse than no tip.
- **The preset bar gets no tips.** Its five controls took accessible names from their deleted `title=` attributes at v1.7.0 and are self-describing.
- **All eight French tooltip bodies are machine-drafted and flagged `reviewed: false`.** No native speaker has read one word of them; the worklist for this plugin is now 29 entries (8 tooltip, 21 label).
- **Non-breaking.** No parameter is added, removed or renamed, and no DSP changed.

## [1.7.0] - 2026-08-29

The page speaks French. Every visible caption, every accessible name and the two image `alt` strings now resolve through a label table, and a gear popover in the bottom-right corner switches the interface between English and French. The choice is remembered with the session.

**This release does NOT add hover-help.** O-Tremolo has never had tooltip copy — only five stray native `title=` attributes — and none is invented here. Authoring hover-help is a separate piece of work. `TIP_BINDINGS` is empty and `I18N` carries no bodies, which is this plugin's correct state rather than a gap.

### Added

- **`Source/ui/public/js/i18n.js`** — the label table, English + French, `{en:{t}, fr:{t, reviewed}}`. 21 entries: 12 captions, 2 image `alt` strings and 7 accessible names. Embedded in `juce_add_binary_data` SOURCES **and** served from a `getResource()` branch in the same commit — a file embedded but not served is a 404 that presents as a page stuck in English and nothing else.
- **The canon v2 i18n runtime** in the inline `<script type="module">`: `trLabel`, `applyLabel`, `applyI18nAttributes`, `setLabel`, `applyI18n`, `initI18n`. Byte-identical to `scripts/i18n-canon.js` after comment stripping; `check-i18n.js` assertion 6 compares it. Placed at the TOP of the module rather than the bottom, so a future top-level label read cannot hit the temporal dead zone and take the whole inline-module UI with it.
- **A settings popover**, opened by a gear button 6 px from the bottom-right corner. Positioned absolutely in the 144 px of unused flex space to the right of `.right-panel`, so **no existing element moves to make room** — measured against the v1.6.0 rect sweep, not eyeballed. The language row is STACKED (caption above select) rather than side by side: side by side needs 133.55 px of row, which forces a panel wide enough to overlap `#waveformCanvas`.
- **`getUiLanguage` / `setUiLanguage` native functions** (`Source/PluginEditor.cpp`) and a **`uiLanguage` atomic** on the processor (`Source/PluginProcessor.h`). The page PULLS once at init; nothing is pushed from the editor constructor, which would race the WebView's load.
- **Language persistence.** `uiLanguage` rides the APVTS state tree as a non-parameter property, written as the string `"en"` / `"fr"` so a hand-inspected session says what it means. Deliberately NOT an `AudioParameterChoice`: it must not appear in a DAW automation lane, and a preset must not be able to change which language somebody reads their plugin in.
- **`tests/i18n-states.json`** — the two states the label gate cannot reach by loading the page: the settings popover and the preset dropdown.

### Changed

- **The five native `title=` attributes on the preset bar are DELETED, not localized.** Their existing English moved verbatim into `data-i18n-aria`; no new prose was invented, and this plugin's own wording ("Load preset from file", "Save current settings") was kept rather than harmonised with a sibling's shorter form. A native `title` is a second, untranslated OS tooltip.
- **The preset dropdown's three JS-written strings go through `setLabel()`** instead of raw `textContent`: the Factory and User section headers and "No presets available". The element becomes a `[data-i18n]` element the moment it is written, so the language sweep owns it and it cannot be stranded in the previous language when the selector fires. Preset NAMES stay a plain `textContent` write — a preset name is the JSON filename on disk.
- **Three width pins, each measured against this page's own English boxes and each negative-controlled.** `#loadPreset` 48 px and `#savePreset` 45 px (their English border boxes rounded up), `.knob-container` 60 px, `.settings-popover` 116 px. Together the two preset-bar pins add **0.39 px** to a header with 114.06 px of slack; **no other English element moves at all** between v1.6.0 and v1.7.0.

### Notes

- **All French is machine-drafted and flagged `reviewed: false`.** No native speaker has read it. `node scripts/check-i18n.js` prints the worklist.
- **Six strings stay English on purpose**, each with a reason in `I18N_EXEMPT`: the six waveform names are `WAVEFORM_PARAM` `AudioParameterChoice` options **byte-identical** to the C++ `StringArray`, so translating the caption while the DAW's automation lane keeps the English would put two names on one control. The product name, the company name, the preset name and the two language endonyms are exempt for their own reasons.
- **The two readout nodes are untouched.** `#speedValue` and `#depthValue` show a number with a unit, or a musical division name such as `1/8T` — which is itself a `SYNC_DIVISION_PARAM` choice string verbatim. A readout is never a localized element.
- **Non-breaking.** No parameter is added, removed or renamed. A pre-1.7.0 session has no `uiLanguage` property and opens in English.

## [1.6.0] - 2026-07-08

Synced Speed is now backed by a dedicated discrete division parameter instead of the continuous Speed knob. This removes the two v1.5.0 known limitations and makes the synced rate exact at every tempo. Non-breaking: adds one parameter at the end of the layout, so existing sessions and older presets load fine (the new parameter falls back to its default).

### Added

- **`SYNC_DIVISION_PARAM` — a discrete "Sync Division" choice parameter** (16 choices mirroring the musical-division table: `1/1 … 1/32`, triplets, quintuplets). When Tempo Sync is ON, this is now the source of truth for the modulation rate. Default `1/8` (index 3), reproducing the 4.5 Hz Speed default at 120 BPM. Automatable and preset-saved like any other parameter.

### Changed

- **Tempo-synced rate is computed directly from the selected division — no 20 Hz cap, no nearest-Hz search.** When synced, the DSP now sets `speedHz = beatsPerSecond / division.beatMultiplier` straight from `SYNC_DIVISION_PARAM` (falling back to 120 BPM when the host reports no tempo). Previously the synced rate was derived by nearest-searching the 16 divisions against the continuous `SPEED_PARAM` (0.1–20 Hz), which the v1.5.0 knob then stepped through. In free (non-synced) mode the Speed knob is unchanged — continuous 0.1–20 Hz. (`Source/PluginProcessor.cpp`)
  - **Resolves v1.5.0 Known Limitation (1): fast divisions unreachable at high tempos.** Divisions whose synced rate exceeds 20 Hz (e.g. `1/32`, `1/32Q`, `1/32T` above ~135 BPM) are now selectable — the 20 Hz `SPEED_PARAM` ceiling no longer bounds the synced rate.
  - **Resolves v1.5.0 Known Limitation (2): adjacent divisions colliding under the 0.1 Hz snap.** Divisions are now selected by discrete index, so triplet/quintuplet neighbours that map to rates <0.1 Hz apart at very low tempos no longer collapse onto the same `SPEED_PARAM` step.
- **Speed knob is dual-bound in the UI.** Synced → the knob drives/reads the discrete `syncDivision` choice state, stepping through all 16 divisions in slow→fast order (one detent per division, ~14 px each) with the indicator snapping and the readout showing the division name — no Hz round-trip math. Free → continuous Hz exactly as before. (`Source/ui/public/index.html`, `Source/PluginEditor.cpp/.h` — added a `WebComboBoxRelay` + `WebComboBoxParameterAttachment`.)
- **Factory presets carry an explicit `SYNC_DIVISION_PARAM`.** Each preset's division is the one nearest its stored Speed at 120 BPM, so toggling sync reproduces the pre-v1.6.0 behavior. The one already-synced factory preset, **Synced Sidechain, is set to `1/16`** — its current synced rate (8.0 Hz @ 120 BPM). (`Source/PluginProcessor.cpp`)

### Fixed

- Supersedes the v1.5.0 "fast divisions can be unreachable at high tempos" and low-tempo division-collision limitations (see Changed above). The synced division shown on the knob and the rate the DSP plays are now the same discrete selection by construction — they can no longer disagree.

### Migration Notes

- **Non-breaking.** `SYNC_DIVISION_PARAM` is appended to the end of the parameter layout; sessions and presets saved before v1.6.0 simply omit it and load with the default (`1/8`). An old session that was tempo-synced will now play the default division rather than whatever the continuous Speed value happened to nearest-snap to — set the Sync Division knob to taste if it differs.
- The v1.5.1 host-BPM readout plumbing (`getHostBpm`) is retained on the C++ side but is no longer consumed by the UI (the synced readout now shows the tempo-independent division name).

## [1.5.1] - 2026-07-08

Two small synced-Speed consistency fixes. No new parameters; existing sessions and presets load and sound identical.

### Fixed

- **(A) Synced rate now matches the readout in Standalone / no host BPM.** When Tempo Sync was engaged but the host reported no BPM (e.g. the Standalone build, or a host that doesn't publish tempo), the WebView readout snapped division names using a 120 BPM fallback (`rebuildSyncSteps`), but the DSP guard `if (tempoSyncEnabled && bpm > 0.0)` fell through to the raw continuous `SPEED_PARAM` Hz — so the *displayed* division and the *actual* modulation rate disagreed. The DSP now mirrors the UI's fallback: when synced with no host BPM it uses an effective 120 BPM for the nearest-division snap, so the readout and the audio agree. (`Source/PluginProcessor.cpp`)
- **(B) Indicator no longer holds a stale angle in the no-reachable-division edge case.** In the stepped Speed knob's `updateVisual()`, the branch taken when no musical division fits the knob's Hz range at the current tempo (`syncSteps.length === 0`) updated only the text readout, leaving the indicator pointer at whatever angle it last painted. That branch now also rotates the indicator from the normalized value, matching the continuous-mode behavior. (`Source/ui/public/index.html`)

## [1.5.0] - 2026-07-08

### Changed

- **Synced Speed knob now steps through discrete divisions.** When Tempo Sync is engaged, the Speed knob no longer sweeps continuously (0.1–20 Hz) with only its text readout snapping to a division — the **indicator itself now snaps** to one discrete detent per reachable musical division, and dragging advances division-by-division (≈14 px per step). This makes a synced tremolo rate read clearly at a glance instead of appearing continuous. In free (non-synced) mode the knob behaves exactly as before (continuous Hz).
  - **Root cause:** the division quantization lived only in the DSP (`processBlock` nearest-`kMusicalDivisions` search) and in the WebView *text* readout. The knob indicator and drag handler stayed bound to the raw continuous `SPEED_PARAM`, so the pointer glided smoothly while the readout jumped — the knob looked continuous even though the audio was stepped.
  - **Implementation (UI-only, `Source/ui/public/index.html`):** added `rebuildSyncSteps()` (builds the slow→fast list of divisions whose synced rate falls inside the knob's real Hz range — exactly the set the DSP can select, so knob and audio stay in lockstep), `nearestSyncIndex()` (mirrors the C++ nearest search to highlight the playing detent), and `speedHzToNorm()` (range/skew-aware Hz→normalized round-trip). The Speed knob's `updateVisual()` snaps the indicator to `idx/(N-1)` of the arc when synced; the drag handler steps the division index instead of accumulating continuous normalized value. The sync toggle and the 1 s host-tempo interval now re-snap the knob through a single hoisted `updateSpeedVisual()`, replacing the previous duplicated text-only updates.
  - **No DSP / parameter / state changes:** `SPEED_PARAM` and the tempo-sync DSP path are untouched, so existing sessions and presets load and sound identical. Removed the now-dead `getMusicalDivisionName()` helper (its callers were consolidated into the single updater).

### Known Limitations

- **Fast divisions can be unreachable at high tempos.** Because the synced rate is still derived from the continuous `SPEED_PARAM` (capped at 20 Hz), divisions whose rate would exceed 20 Hz at faster tempos are not selectable — the stepper simply omits them (unchanged from prior behavior; the DSP could never reach them either). Lifting this would require a dedicated sync-division parameter and is deferred.

## [1.4.8] - 2026-07-07

Resolves all findings from a deep GSD code review (see `CODE_REVIEW.md`): 1 critical, 2 warnings, 5 info.

### Fixed

- **CR-01 (crash): FileChooser use-after-free on editor teardown.** The `savePresetWithDialog` and `loadPresetFromFile` native functions launched `juce::FileChooser::launchAsync` with completions that captured a raw `this` and called the WebView-owned `complete()` callback. Closing the plugin window (or removing the track) while the native Save/Load dialog was open fired the completion against a destroyed editor → use-after-free. **Root cause:** the codebase's `SafePointer` teardown pattern was not applied here. Both completions now capture a `juce::Component::SafePointer` and bail with a bare `return` if the editor is gone — notably *without* calling `complete()`, which is itself owned by the dead WebView.
- **WR-01 (zipper noise): Depth parameter is now smoothed.** `depth` scales the per-sample output gain, so an un-smoothed step from automation, preset recall, or a UI drag produced an audible click/zipper. Depth is now ramped with a 20 ms `juce::SmoothedValue` consumed per-sample in both the mono and pan-sync paths. (LFO rate remains block-updated — rate steps are inaudible and lower-risk to leave unchanged.)
- **WR-02 (display drift): Knob readouts use the JUCE scaled value.** Speed/Depth readouts remapped normalized→real with hardcoded JS literals (`0.1–20`, `0–100`), which would silently drift from the C++ `NormalisableRange`/skew. They now read `SliderState.getScaledValue()` (authoritative from C++) and refresh on `propertiesChangedEvent`. The duplicated remap in the tempo-sync toggle handler was removed too.

### Changed

- **IN-02:** Preset dropdown now distinguishes **Factory** vs **User** presets (via the `isFactoryPreset` native fn) instead of labeling every entry "Factory".
- **IN-03:** Visualizer smoothing state (`smoothedY`) is reset per redraw instead of persisting across draws; removed a dead `phase` local in the noise case.
- **IN-04:** Tempo-sync readout now shows the division for the **real host tempo**. Added a `getHostBpm` native function (host BPM cached on the audio thread) that the UI polls while sync is active, replacing the hardcoded 120 BPM assumption.
- **IN-05:** Waveform selector index divisor is now derived from the waveform-list length (single `WAVEFORM_NAMES` source of truth) instead of a hardcoded `×5`, so adding/removing a waveform can't silently break the mapping. (Kept the working `WebSliderRelay` binding rather than rewiring to a ComboBox relay, to avoid regression risk.)

### Notes

- **IN-01 (intentionally not fixed):** the async `promptDelete` change in the shared `preset-manager.js` module is inert in O-Tremolo (no delete affordance is wired in the UI). Adding a delete control is new functionality, out of scope for this review-fix pass; the module sync is committed as-is.
- Native-function bridge verified balanced (12 C++ ↔ 12 JS, all names match).

## [1.4.7] - 2026-02-12

### Added

- **Version branding footer** - Added "Ouaricon Audio v{version}" display at the bottom of the plugin UI, dynamically fetched from C++ via `getPluginVersion` native function

## [1.4.5] - 2026-02-12

### Added

- **Version number in UI footer** - Added "Ouaricon Audio v1.4.5" branding footer at the bottom of the plugin UI. Version is fetched dynamically from C++ via `getPluginVersion` native function so it always stays in sync with the build

## [1.4.3] - 2026-02-12

### Fixed

- **Fixed license overlay blink on macOS** - Auth screen no longer flashes momentarily when opening a licensed plugin. Changed overlay from `addAndMakeVisible` to `addChildComponent` to prevent forced-visible intermediate state during editor construction
- **Fixed missing macOS activation in portal** - License refresh cycle was omitting `product_id` and `app_version` from the re-activation request body, causing the backend to not properly associate macOS activations with the product. Portal now correctly shows activations for all platforms

## [1.4.2] - 2026-02-12

### Fixed

- **Fixed Windows build pipeline** - Resolved CI/CD issues preventing successful Windows builds

## [1.4.1] - 2026-02-08

### Fixed

- **Fixed release title metadata** - GitHub release now displays clean "O-Tremolo" name instead of showing CMake variable reference
- **Rebuilt cross-platform binaries** - Fresh macOS (Universal) and Windows builds with latest CI pipeline

## [1.4.0] - 2026-02-06

### Added

- **5 new factory presets** expanding the preset library from 5 to 10:
  - **Helicopter** - Fast triangle wave at ~12 Hz, 90% depth, minimal smoothing for stuttery blade effect
  - **Vintage Amp** - Medium sine at ~5.5 Hz, 55% depth, heavy smoothing for classic amp tremolo
  - **Synced Sidechain** - Tempo-synced pulse wave at full depth, mimics sidechain compression pumping
  - **Wide Stereo** - Slow triangle at ~2 Hz with pan sync for gentle stereo widening
  - **Glitch** - Fast noise at ~18 Hz, full depth, zero smoothing for experimental textures

- **Windows WebView2 cross-platform support** - Plugin now works correctly on Windows:
  - Added `NEEDS_WEBVIEW2 TRUE` for static linking of WebView2Loader
  - Added `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile definition
  - Explicitly requests WebView2 backend (prevents silent IE fallback)
  - Sets user data folder to temp directory (prevents access denied in DAW hosts)
  - Guards resource provider with `JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE`

### Technical Notes

- Factory presets use normalized parameter values (0-1 range matching APVTS)
- WebView2 configuration follows O-AnalogEQ reference pattern
- No breaking changes - existing macOS sessions and presets remain compatible

## [1.3.1] - 2026-01-24

### Changed

- **Renamed plugin from "Ouaricon Tremolo" to "O-Tremolo"** - Affects all user-facing identifiers:
  - DAW display name now shows "O-Tremolo"
  - Binary files renamed: `O-Tremolo.vst3`, `O-Tremolo.component`
  - Preset folder moved to `~/Library/Application Support/O-Tremolo/Presets/`
  - Source folder renamed from `plugins/OuariconTremolo/` to `plugins/O-Tremolo/`
  - Plugin window title updated

### Technical Notes

- CMake PRODUCT_NAME changed from "Ouaricon Tremolo" to "O-Tremolo"
- Internal CMake target remains `OuariconTremolo` for build system compatibility
- Plugin codes unchanged (OuTr) - maintains DAW session compatibility
- No breaking changes - existing sessions will continue to work

## [1.3.0] - 2026-01-12

### Added

- **Preset management system** - Full preset save/load functionality using the Ouaricon Module System
  - **Factory presets**: 5 built-in presets (Default, Slow Pulse, Fast Chop, Auto-Pan, Subtle)
  - **User presets**: Save custom settings to `~/Library/Application Support/Ouaricon Tremolo/Presets/User/`
  - **Preset bar UI**: Header now displays current preset name with navigation controls
    - Previous/Next buttons for cycling through presets
    - Save button for creating user presets
    - Click preset name to see all available presets
  - **DAW session integration**: Preset state automatically saved/restored with DAW projects

### Changed

- **Header layout updated** - Title moved to left side with preset bar on right (matching Ouaricon Marimba style)
- **State management refactored** - Now uses `OuariconPresetManager` for unified state handling

### Technical Notes

- Integrated `preset-manager` module v1.0.0 from Ouaricon Module System (`modules/persistence/preset-manager/`)
- CMake integration via `ouaricon_add_module(OuariconTremolo preset-manager)`
- Added 8 native functions for WebView↔C++ preset communication:
  - `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`
  - `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`
- Factory presets use normalized parameter values (0-1 range)
- Presets stored as JSON with APVTS parameter serialization
- No breaking changes - existing DAW sessions remain compatible

## [1.2.1] - 2026-01-06

### Fixed

- **Improved dial value readability** - Speed and Depth knob value displays (e.g., "4.5 Hz", "75%") now use bold font weight for better legibility
  - Applied `font-weight: bold` to `.knob-value` CSS class
  - Dial labels remain unchanged (already bold)
  - Enhances readability of parameter values against vintage botanical background

### Technical Notes

- CSS-only change (added `font-weight: bold` to `.knob-value` class in index.html:308)
- No breaking changes - purely cosmetic improvement

## [1.2.0] - 2026-01-06

### Changed

- **Redesigned parameter dials with vintage botanical seed aesthetic** - Speed and Depth knobs now feature hand-drawn seed cross-section illustrations inspired by 18th/19th century botanical engravings
  - **Visual design**: 10-segment radial seed chamber pattern using CSS conic-gradient (similar to citrus cross-section from botanical manuscripts)
  - **Color palette**: Vintage warm tones replacing modern green - cream (#F5DEB3), sepia (#E8D5B7), tan (#8B7355), cornsilk center (#FFF8DC)
  - **Engraving effects**: Cross-hatching texture overlay with fine radial lines and concentric circles for authentic botanical illustration feel
  - **Indicator integration**: Value indicator redesigned as triangular seed segment (golden brown #8B6914) that rotates within the seed pattern
  - **Borders and depth**: Aged paper outer ring with engraved inset shadows for dimensional vintage aesthetic
  - **Botanical motif**: Subtle flower symbol (✿) in center matching the plugin's flora theme

### Technical Notes

- CSS-only implementation (no JavaScript changes required)
- Knob background uses layered gradients: outer ring (radial), seed segments (conic), inner core (radial)
- Engraving texture created with `repeating-conic-gradient` (radial lines) and `repeating-radial-gradient` (concentric circles)
- Indicator uses CSS triangle technique (transparent borders with colored top border)
- Mix-blend-mode multiply for authentic engraving overlay effect
- All existing knob functionality preserved (rotation range, interactivity, JUCE parameter binding)
- No breaking changes - purely visual enhancement

## [1.1.4] - 2026-01-05

### Fixed

- **Fixed mono->stereo centering** - Mono input now properly centered between L/R channels
  - **Root cause**: When a mono track was processed, the DAW provided a stereo buffer but only populated channel 0, leaving channel 1 silent
  - **Solution**: Detect mono input → stereo output configuration and duplicate channel 0 to channel 1 before processing tremolo
  - **Result**: Mono sources now output centered stereo signal instead of hard-panned left
  - Pan Sync behavior preserved: when OFF = centered tremolo, when ON = stereo phase offset

- **Fixed GUI blank screen on reopen** - Plugin GUI now loads correctly every time it's reopened
  - **Root cause**: `hasNavigated` flag was declared `static` in `parentHierarchyChanged()`, persisting across editor instances
  - **Impact**: First GUI open worked, but closing and reopening resulted in blank white window because flag was still `true`
  - **Solution**: Converted `hasNavigated` to member variable so each editor instance tracks navigation independently
  - **Result**: GUI reliably loads on every open/reopen within the same session

### Technical Notes

- Added mono input detection using `getTotalNumInputChannels()` and `getTotalNumOutputChannels()`
- Mono→stereo duplication happens before tremolo processing to ensure consistent behavior with Pan Sync
- GUI fix maintains JUCE 8 safety requirements (navigation only after window context exists)
- Both fixes preserve real-time safety (no allocations in audio thread)

## [1.1.3] - 2026-01-05

### Fixed

- **Eliminated remaining clicks from phasor and noise waveforms** - Extended s-curve smoothing to cover all waveform types
  - **Phasor**: Added polynomial transitions at both wrap boundaries (end and start of cycle)
    - Last 2% of cycle smoothly prepares for wrap discontinuity
    - First 2% of cycle smoothly recovers from wrap
    - Eliminates 2.0-unit discontinuity at cycle boundary
  - **Noise**: Added smooth interpolation between all quarter-boundary transitions
    - Transitions at 0°, 90°, 180°, 270° now use cubic polynomial s-curves
    - Each transition spans 2% of quarter duration (0.5% of full cycle)
    - Previous held value tracked for smooth interpolation
  - All waveforms now click-free across full speed range (0.1-20 Hz)
  - Consistent 2% transition width across square, pulse, phasor, and noise

### Technical Notes

- Added `noisePrevHeldValue` member variable to track previous random value
- Phasor now uses bidirectional smoothing (end-of-cycle and start-of-cycle zones)
- Noise transitions use `quarterPhase` calculation for precise boundary detection
- All smoothing uses `smoothTransition()` cubic polynomial (3t² - 2t³)
- Real-time safety preserved (no allocations in audio thread)

## [1.1.2] - 2026-01-05

### Fixed

- **Eliminated audio clicks from sharp waveform transitions** - Phasor, square, and pulse waveforms now use polynomial transition zones (cubic smoothstep) instead of hard edges
  - Transition width: 2% of cycle for smooth S-curve interpolation
  - High-fidelity approach appropriate for LFO frequencies (0.1-20 Hz)
  - Zero audible artifacts while preserving waveform character

- **Fixed noise waveform behavior** - Changed from continuous noise burst to smooth sample-and-hold
  - Samples new random value 4 times per LFO cycle (at 0°, 90°, 180°, 270°)
  - Creates organic random amplitude modulation instead of harsh noise
  - **Critical fix**: Resolved Pan Sync phase offset issue where stereo channels triggered new random values every sample instead of holding for 1/4 cycle
  - Solution: Modified `generateWaveform()` to accept `mainLfoPhase` parameter, ensuring noise uses consistent phase tracking regardless of Pan Sync stereo offset
  - Matches professional tremolo implementations

### Changed

- **UI color consistency** - Waveform visualizer and dropdown menu now use pale green background matching button theme
  - Changed from cream `rgba(255, 248, 231, 0.8)` to pale green `rgba(139, 168, 112, 0.3)`
  - Improved visual coherence across interface elements

### Technical Notes

- Added `smoothTransition()` helper function using cubic polynomial (3t² - 2t³)
- Sample-and-hold state variables: `noiseHeldValue`, `noiseLastQuarter`
- Modified `generateWaveform()` signature to accept `mainLfoPhase` parameter for Pan Sync compatibility
- Noise waveform now ignores stereo phase offset, uses main LFO phase for consistent quarter tracking
- JavaScript visualizer updated to match C++ waveform behavior
- All changes preserve real-time safety (no allocations in audio thread)

## [1.1.1] - 2026-01-05

### Fixed

- **UI polish improvements** - Enhanced visual layout and interactivity
  - Centered text in waveform dropdown menu for better visual balance
  - Tightened spacing between knobs and their labels/values (gap reduced from 8px to 4px)
  - Adjusted depth dial position upward by 5px for improved vertical alignment
  - Waveform visualizer now responds to depth parameter (amplitude scales with depth 0-100%)

### Technical Notes

- CSS modifications only (no C++ changes required)
- Waveform amplitude calculation: `baseAmplitude * depthNormalized` where depthNormalized = 0.0 to 1.0
- Added `.depth-knob-container` class for specific positioning control
- All changes are visual-only, no parameter behavior changes

## [1.1.0] - 2026-01-05

### Added

- **Musical division display when tempo sync is enabled** - Speed dial now displays musical rhythmic values (e.g., "1/8T", "1/4Q") instead of Hz when tempo sync is ON
  - Expanded from 6 to 16 musical divisions including:
    - Straight divisions: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32
    - Triplet divisions: 1/2T, 1/4T, 1/8T, 1/16T, 1/32T (3 notes in space of 2)
    - Quintuplet divisions: 1/2Q, 1/4Q, 1/8Q, 1/16Q, 1/32Q (5 notes in space of 4)
  - Preserves musical relationship when DAW tempo changes (Option A parameter storage)
  - Hz display retained when tempo sync is OFF for continuous frequency control

### Changed

- **Tempo sync behavior** - Speed parameter now snaps to musical divisions when synced, ensuring tremolo stays locked to musical timing regardless of tempo changes
- **UI responsiveness** - Speed display automatically switches between Hz and musical notation when toggling tempo sync button

### Technical Notes

- Updated `MusicalDivision` table in PluginProcessor.cpp with 16 divisions using precise beat multipliers
- Modified WebView UI JavaScript to detect tempo sync state and format speed display accordingly
- Beat multiplier calculations:
  - Triplets: `(base_beats * 2/3)` - e.g., 1/8T = 0.5 beats * 2/3 = 0.333 beats
  - Quintuplets: `(base_beats * 4/5)` - e.g., 1/8Q = 0.5 beats * 4/5 = 0.4 beats
- No breaking changes - existing presets and sessions remain compatible

## [1.0.1] - 2026-01-05

### Fixed

- **Critical crash on plugin load** - Fixed initialization order bug that caused segmentation fault during editor construction
  - **Root Cause #1**: `setSize(600, 400)` was called in constructor BEFORE `webView` was created. When JUCE tried to resize child components, it dereferenced a nullptr causing crash at `Component::setBounds()`.
  - **Root Cause #2**: `WebBrowserComponent::goToURL()` was called in constructor before component was attached to a native window. In JUCE 8, WebKit requires valid window context for navigation.
  - **Solution #1**: Moved `setSize()` to END of constructor, after all components are created and added
  - **Solution #2**: Moved WebView navigation from constructor to `parentHierarchyChanged()` callback
  - **Impact**: Plugin now loads successfully in all DAWs (Logic Pro, Ableton, Reaper) and passes AU/VST3 validation

### Technical Notes

- Fixed component initialization order: relays → webView → attachments → addAndMakeVisible → setSize
- Added `parentHierarchyChanged()` override for safe WebView navigation
- Implemented one-time navigation guard using static flag
- Added safety checks (`isShowing()` and `webView != nullptr`) before navigation
- Exception type: `EXC_BAD_ACCESS (SIGSEGV)` at `KERN_INVALID_ADDRESS 0x0000000000000040`

## [1.0.0] - 2026-01-04

### Added

- Initial release of OuariconTremolo
- Tremolo effect with 6 waveform types (Sine, Triangle, Phasor, Noise, Square, Pulse)
- Speed control (0.1-20.0 Hz)
- Depth control (0-100%)
- Smoothing filter (0-100%)
- Pan Sync mode (stereo modulation with 180° L/R phase offset)
- Tempo Sync (locks to DAW BPM with note division quantization)
- Botanical vintage WebView UI with real-time waveform visualizer
- VST3 and AU formats
- macOS support

### Known Issues

- Plugin crashes on load (fixed in v1.0.1)
