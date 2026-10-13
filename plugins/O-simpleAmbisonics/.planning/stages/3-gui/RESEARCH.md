# Stage 3: GUI — Research

**Researched:** 2026-10-13
**Domain:** JUCE 8.0.15 WebView editor (relays / attachments / natives / events), canvas visualisation of a 3rd-order AmbiX bus, suite i18n + UI gates
**Confidence:** HIGH for the bindings, payload, state and gates (read from the JUCE source, the template and the gate scripts this session, plus probes run against a fixture tree). MEDIUM for the icon look and the host-idle behaviour (needs the visual checkpoint and a Logic hands-on).
**Cadence:** a single RESEARCH / PLAN / SUMMARY / VERIFICATION with no suffix. Execute runs in two parts: 3.1 + 3.2, then build / install and the user's visual checkpoint, then 3.3.

---

## Summary of recommendations

1. **Use the mockup template, with five fixes. Do not lift either sibling editor wholesale.**
   - `v1-PluginEditor-TEMPLATE.{h,cpp}` already merges the right pieces from two siblings:
     - From O-simpleWavetable v1.0.1: the 12 named relays, the attach() helper, the natives, the gesture-close fix and the uiReady pattern.
     - From O-simpleAdditive: the committed scale pattern.
   - Note that O-simpleFM's scale pattern is **uncommitted** in today's working tree; HEAD is still a fixed 760×980 window. Cite O-simpleAdditive, not O-simpleFM.
   - The five fixes:
     1. `editorScale` becomes `std::atomic<float>`, persisted and clamped to the display.
     2. The payload builder moves to a JUCE-free header shared with the harness. The template's "one allocation" claim is false: `String::clear()` frees the buffer.
     3. Drain one `read()` in the constructor.
     4. Zero the peaks when the triple buffer goes stale.
     5. `applyFactoryPreset` must skip `outputLevel` in the reset too. The checklist's "reset EVERY parameter" contradicts "never `outputLevel`".
2. **Do every processor edit in one 3.1 task, then run the full regression protocol once.** The edits are: the guarded `createEditor`, `uiLanguage`, `editorScale`, the state XML, and `applyFactoryPreset` with the provisional recipes.
   - The protocol: harness 241/241 plus the new state probes, both digests, auval, pluginval s10 ×2 on VST3 + AU, and `verify_2_1/2/3.py`.
   - The Stage-2 pedalboard round-trip gates read **parameters only**, so the new attributes cannot break them. A new `tests/pedalboard/verify_3_state.py` grows the round-trip by decoding the VST3 state, which was proven feasible this session.
3. **SH icons: a Node generator with zero dependencies, rendering real SN3D lobes offline.**
   - It lives at `scripts/generate_sh_icons.mjs` and writes one data module, `Source/ui/public/js/shicons.js` (no hyphen).
   - It renders 3D orthographic, flat-shaded, polarity-coloured balloons in an 18×18 viewBox, normalised per icon.
   - The camera is "the top-down canvas tilted toward you": front recedes up, left stays left. The 2D slices in the mockup cannot tell S from T or L from M.
   - The generator self-checks against an independent SN3D formula. The ACN → letter order is verified against Wikipedia.
4. **The canvas drag is already correct in the mockup.** It uses `sliderDragStarted → setNormalisedValue(scaledToNorm(…)) → sliderDragEnded`, which matches the real JUCE 8.0.15 frontend and attachment.
   - `atan2(−dx, −dy)` gives +90° = LEFT and wraps cleanly at ±180°.
   - Change two things: widen the dead zone to the head radius, and display ±180 as `180.0°`.
   - Known limitation: host automation that ramps between +179 and −179 sweeps through the front. The parameter is frozen, so document it and check it hands-on in Logic.
5. **`meterFrame` payload: one pre-formatted, fixed-point JSON string.**
   - Build it with `std::to_chars` into a stack buffer. It is locale-proof (never `%f`), deterministic, and byte-comparable with harness fixtures.
   - Hash with FNV-1a over the quantised ints. A static scene emits nothing.
   - Add `"live"`: whether the buffer had new data within the last 3 ticks. When it goes false, zero the peaks. That fixes a held-peak freeze when the host idles the audio thread, and an all-zero W before the first block.
6. **UI-01 / UI-02 gates take their frames from the real processor.**
   - The harness gets an opt-in fixture mode that writes the exact payload strings the editor would emit: front / left / behind / overhead, orders 1–3, yaw-only, pole, and Stereo mix.
   - `tests/ui-probes/frame-probe.mjs` feeds those strings to the real page through the generic stub and asserts on **canvas pixels** and on the relay states, with a negative control.
   - A live capture from WKWebView cannot be done headless. The harness route captures the same bytes deterministically.
7. **Compass: turn the rose by the heading of the scene-front vector R·x̂, computed from the eased parameters.**
   - That heading equals `yaw` whenever |pitch| < 90°. The mockup's yaw-only rose is wrong for |pitch| > 90° (pitch spans ±180°).
   - The tilt chevron shows the front's elevation, asin(sin θ).
   - Hold rules: the heading holds when |cos θ| < 1.75e-4. The beam centre and rim tick hold `azRot` when |`elRotDeg`| ≥ 89.99, which the mockup does not do yet.
8. **Polar plot cost is negligible: the 361-sample curve per order is already cached.**
   - The real cost is the mockup's **unconditional 60 fps redraw**: about 27 % of a core headless at DPR 2, measured. Suppressing the redraw drops it to about 1 %.
   - Render on dirty flags, and pre-render the puck sprite.
9. **i18n starts from a working base.**
   - On a fixture tree built from the mockup, check-i18n, both lints and boot-all-uis `--strict-tips` already **pass**.
   - check-ui-labels **fails 10 assertions**: real fr/zh overflows plus artefacts of the neutral stub seed.
   - The French lesson row fits with **86 px** to spare (measured).
   - 3.3 work: add an ambisonics section to both shared glossaries first, then `generic-overrides.json` with the real ranges, `i18n-states.json`, and five width fixes.

**Primary recommendation:**
- Run 3.1 as one batched processor edit plus the harness additions and a guarded editor built from the template.
- 3.2 is the full mockup page, including the UI-03 beam and the UI-05 decoder view, since they are already in the page. It adds the compass / pole / dirty-render fixes, the generated icons and the frame probe. Then stop for the user's eye.
- 3.3 is glossary first, then copy, width fixes, lessons and every gate on the real tree.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

CONTEXT.md carries no separate "Claude's Discretion" or "Deferred Ideas" headings. Its **Approach Decisions** and **Constraints Identified** are the locked decisions. Its **Open Questions** are the research / discretion items, answered below. Out of scope as v1.1: spread, decoder weighting, channel solo, the distanceModel toggle, head-locked stereo, and the bank-fade indicator.

### Locked Decisions — Approach Decisions (verbatim)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Mockup sequencing | **Mockup first, then `/plugin-research`** (user) | The design must be iterated in a browser before C++ wiring; the mockup also produces `parameter-spec.md` and the v1-* scaffolding research and plan work from, as O-simpleWavetable did. This CONTEXT.md is its brief. |
| Cadence | **One GSD cycle; execute in two parts: 3.1 + 3.2 → build / install → user visual checkpoint → 3.3** (user) | The finalized mockup carries the design, so per-phase discuss adds little; the checkpoint lets the meters, puck and compass be checked by eye before tooltips, presets and i18n land on top. Single RESEARCH / PLAN / SUMMARY / VERIFICATION (no suffix). |
| i18n scope | **en / fr / zh-Hans in 3.3** (user) | Family parity; the ambisonics vocabulary needs the glossary-first pass before parallel localizers. |
| Lesson buttons | **Live with provisional recipes in 3.3** (user) | The tour is how a class demos each concept; the hands-on needs it working. Stage 4 refines and adds the FUNC-08 bank. Never `outputLevel`; Binaural recipes keep headroom. |
| Window | **Resizable at a fixed aspect with the O-simpleFM scale pattern; landscape design frame** | UI-07 is a must: fit a 13" laptop unscaled, scale up on a projector. `editorScale` persisted on the processor. |
| Polar plot placement | **Overlay in the top-down sphere canvas, centred on `azRot`** | ARCHITECTURE §Polar plot; one picture shows direction, rotation and sharpness together. |
| Sphere drag model | **Top-down angle → `azimuth`; side → `elevation`; `distance` by knob only** | One-dimensional drags are unambiguous and gesture-safe; the radius still shows distance. Mockup may overturn. |
| Meter rows | **Grouped by order band, signed `shGain` bars with `shPeak` overlay** | ARCHITECTURE Decision 9: the pattern value is program-independent and polarity-visible; the band grouping makes the order step-up the picture. |
| Signal-path highlight | **Page-side recency of parameter change** | No frame change; the highlight is a teaching cue, not a measurement. |
| Decoder-view fade indicator | **Omitted in v1.0** | Would need a frame / engine touch for a 20 ms animation; Stage-2 digests stay the verdicts. |
| Readouts | **Host parameter value, never the frame** | Carried from Stage 2 (−11.999999, −180 for +180). |
| Hands-on venue | **Standalone + Logic Pro** | Standalone covers drags, tooltips and the tour; Logic covers automation → UI, hide / re-show (`uiReady` re-send), save / reopen with `editorScale`, and clears the AU hands-on list carried from Stage 2. |

### Locked Decisions — Constraints Identified (verbatim)

- **The mockup is a hard prerequisite for research / plan / execute.** `/start O-simpleAmbisonics` → option 3 runs the ui-mockup workflow, producing `.planning/mockups/v1-ui.yaml` + `v1-ui-test.html` (design iteration) and, after approval, `v1-ui.html`, `v1-i18n.js`, `v1-PluginEditor.{h,cpp}`, `v1-CMakeLists.txt`, `v1-integration-checklist.md`, and finalises `parameter-spec.md` from ARCHITECTURE §Parameter Mapping — **12 parameters, IDs exactly as `ParamIDs`, no additions** (spread / weighting / solo are v1.1). This CONTEXT.md is the mockup's design brief; the mockup may overturn the layout assumptions here (frame size, panel proportions, the drag model) but not the DSP conventions (sign of azimuth, what the puck / compass / meters follow, the 12-parameter set).
- **Message-thread only:** the editor Timer reads the triple buffer, builds one JSON payload (32 floats + 6 scalars) and emits; no per-tick allocation beyond the payload string; the audio thread is untouched (PERF-01 stays verified).
- **Processor changes are limited to:** the `#if JUCE_WEB_BROWSER` editor hook in `createEditor()`, `applyFactoryPreset` (message thread, APVTS only, gesture begin / end per parameter), `uiLanguage` + `editorScale` in the state XML (state round-trip gates grow from 12 values accordingly), and nothing in `processBlock` / the engine. After any processor edit: harness 241/241 with both digests asserted, then `verify_2_1/2/3.py` on the installed VST3.
- **Harness build stays green:** the editor TU is guarded so `tests/render-harness` (`JUCE_WEB_BROWSER=0`) compiles without it; the UI BinaryData target is the only `juce_add_binary_data` this plugin ever has (the filter bank is a header).
- `Source/ui/public/modules/*.js` are gitignored copies written at configure time — never commit them; a `git archive` snapshot lacks them (project memory).
- Canvas drags: `pointerdown` + `preventDefault` suppresses the compatibility `mousedown`, so document-level dismissers must listen on `pointerdown`; Chromium's implicit pointer capture makes drag-outside behave (project memory). Canvas text needs `document.fonts.load()` then a repaint or it keeps the fallback face (project memory).
- Tooltips: fixed-position with shrink-to-fit at the window edge and the viewport-sensitive clamp gate (project memory); `boot-all-uis --strict-tips` must report 0 dead / 0 late.
- UI gates before the Stage-3 commit, on the real tree (not the finalization fixture): `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint`, `check-ui-labels`, `boot-all-uis --strict-tips`. Exit 77 = Playwright not resolved = nothing verified.
- Build / install via `./scripts/build-and-install.sh O-simpleAmbisonics`; rebuild the Standalone explicitly for the hands-on (it stays stale under that script — project memory). Commits path-scoped with the temp-index discipline; re-check branch + staging immediately before each commit.
- Windows build itself stays a Stage-4 item (COMPAT-02); the CMake flags are already correct.

### Claude's Discretion — CONTEXT "Open Questions (for research)" (verbatim; each answered in §Q1–Q11)

- Reference editor to adapt from: O-simpleFM (scale pattern, viz Timer) vs O-simpleWavetable (latest WebView + gate conventions, lesson buttons). Likely both: FM for the window / scale, Wavetable for the relays, natives and gates.
- How the mockup's `v1-PluginEditor.{h,cpp}` template handles an effect (no keyboard, no import) — what to delete vs adapt.
- Generating the 16 SH pattern icons offline (a script rendering `Y_acn` lobes to SVG, polarity-coloured) vs hand-drawn; consistent sizing in a meter row.
- Canvas drag → slider state: the exact gesture begin / end calls through the JUCE 8 frontend slider state for a non-knob control, and how the wrap at ±180° is handled when the pointer crosses the rear.
- `meterFrame` payload at 30 Hz: quantisation for the hash (shGain 1e-3, peaks 1e-3 of full scale, angles 0.01°) and whether `juce::var` building or a pre-formatted string is cheaper; what the rest state emits (nothing).
- UI-01 / UI-02 gates: a Playwright probe fed synthetic frames must use states the C++ can actually produce (derive from `MeterFrame` semantics — project memory); whether a headless capture of the live payload is feasible for the "X → Y swap, W constant" acceptance.
- Compass under yaw with pitch / roll non-zero: what the top-down rose shows when `elRot` leaves the horizon; the hold rule near the pole.
- `editorScale` + `uiLanguage` in the state XML: the `verify_2_*` state round-trip gates and the 12-value contract; migration-safe defaults.
- Polar-plot rendering cost per frame (Legendre at 360 angles) and whether to cache per order.
- i18n glossary draft and the width budget for the French lesson-button row (project memory: lesson rows are a French width budget).

### Deferred Ideas (OUT OF SCOPE)

- These come from CONTEXT / parameter-spec, not from a "Deferred" heading: spread, decoder weighting (basic / max-rE / in-phase), channel solo audition, `distanceModel` toggle, head-locked stereo (v1.1+, ARCHITECTURE Decision 10); the bank-fade indicator in the decoder view; the preset-manager natives and the FUNC-08 bank (Stage 4); the Windows build (Stage 4, COMPAT-02); the QUAL-01 preset-load click clause (Stage 4).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description (REQUIREMENTS.md:48-55) | Research Support |
|----|-------------|------------------|
| UI-01 (must) | Live B-format channel view: one meter per channel labelled by ACN index / name with its SH pattern icon, rows by order | §Q2 icons, §Q4 payload, §Q5 frame-probe (pixel-read bars, harness-made frames), the `live` flag (§Q4.4) |
| UI-02 (must) | Source-on-a-sphere: draggable puck top + side, distance radius, head + front marker, drag sets az / el, rotation = compass turning | §Q3 drag + gestures, §Q6 compass / pole hold, §Q5 drag + yaw-only probe arms |
| UI-03 (should) | Polar plot sharpening 1 → 3 | §Q8 (cached curve, dirty render); probe arm: beam width at −3 dB narrows 1 → 3 |
| UI-04 (should) | Signal-path diagram, active stage, channel count | Already in the mockup (`STAGE_OF`, `paintStages`); probe arm: `.on` station after a parameter touch |
| UI-05 (nice) | Decoder view | Already in the mockup (`drawDecoder`, `decoderMix` ease); probe arm: Stereo frame `decoderMix: 0` |
| UI-06 (should) | Tooltips on every control | i18n.js TIP_BINDINGS: boot-all-uis `--strict-tips` 0 dead / 0 late on the fixture today (§Q9) |
| UI-07 (must) | Single page, projector-readable, L→R signal path | §Q1 scale pattern (Additive), editorScale persistence + display clamp (§Q7) |
| UI-08 (should) | Preset tour | §Q1 / §Q10: `applyFactoryPreset`, one pass, `outputLevel` excluded; recipes from `v1-ui.yaml:371-379` |
</phase_requirements>

---

## Project Constraints (from CLAUDE.md)

- **Build / install:** after any VST3 / AU build, clear the AU cache, sweep **both** `<Name>.{vst3,component}` and `<Name>-dev.{vst3,component}`, then install. Preferred: `./scripts/build-and-install.sh O-simpleAmbisonics` (its Phase 4 sweeps both variants). Verify with `auval -v aufx OSAm OuDv`, never `auval -a`. Stage-2 practice, and also the memory note on auval SIGABRT.
- **Trunk-based development:** all work on `main`, no branches or worktrees. Commits are path-scoped (`git commit -- plugins/O-simpleAmbisonics …`), never `git add -A` or `-a`. Re-check `git branch --show-current` and `git status --short` immediately before every commit.
  - Shared files touched in 3.3 are `scripts/i18n-fr-glossary.js` and `scripts/i18n-zh-glossary.js`. They need their own path-scoped commit; other sessions edit `scripts/` too.
- **Research docs** go in `research/`, never `docs/`. This file is a stage artefact under `.planning/`, which is correct.
- **Phase handoff:** after the stage completes, present `/clear` as step 1 and `/plugin-plan O-simpleAmbisonics` as step 2, then STOP.
- **Memory directives relevant here:** never tag unless `/publish`; presets never set the output gain; run long commands in the background (600 s watchdog); the 0→1 gate needs `--force`.

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|---|---|---|---|
| Parameter state, automation, gestures | Processor (APVTS, message thread) | Page (SliderState / ComboBoxState / ToggleState) | The host owns the values; the page only proposes them through the relays |
| Meter data (`shGain`, `shPeak`, `azEff`, `azRot`, …) | Audio thread → `TripleBuffer<MeterFrame>` | Editor Timer (30 Hz) | Already verified in Stage 2; the editor only reads, quantises and emits |
| Payload quantise / hash / format | Editor (message thread) via a pure header | Harness (fixtures) | One function produces both the live bytes and the gate fixtures |
| Ballistics, compass ease, beam, icons, highlight | Page (rAF) | — | Display-only. No frame field is added (locked) |
| Readouts (numbers and words) | Page, from `getScaledValue()` | — | Locked: the parameter, never the frame |
| Lesson recipes | Processor (`applyFactoryPreset`) | Page caption (setLabel) | Message thread, APVTS only; relays sync the controls back |
| Window size / language | Processor non-parameter state | Editor (`resized`, natives) | Must survive a session save / reopen; never reaches an automation lane |
| SH pattern icons | Offline generator → static data module | Page DOM (createElementNS) | Deterministic and reviewable; no runtime maths |

---

## Standard Stack

### Core (all already in the repo; nothing to install)

| Library | Version | Purpose | Evidence |
|---|---|---|---|
| JUCE | 8.0.15 | WebBrowserComponent, Web*Relay, Web*ParameterAttachment | `[VERIFIED: /Users/taylorbrook/JUCE/modules/juce_core/system/juce_StandardHeader.h:42-44]` `JUCE_MAJOR_VERSION 8`, `JUCE_MINOR_VERSION 0`, `JUCE_BUILDNUMBER 15` |
| JUCE frontend `index.js` + `check_native_interop.js` | identical across JUCE / Wavetable / FM (md5 `01ff05e3c9b47c510a7bec98247670a0`) | Page-side relay states | `[VERIFIED: md5 this session]`. Copy it from `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/` |
| EB Garamond module | 1.0.0 (direct embed) | Family face | `[VERIFIED: modules/registry.yaml:537-560]`; direct-embed contract in the CMake snippet |
| Playwright (resolved via `serve-ui.js resolvePlaywright()`) | already resolvable | check-ui-labels, boot-all-uis, frame-probe | `[VERIFIED: ran "playwright OK" this session]` |

### Supporting

| Tool | Purpose | When |
|---|---|---|
| `scripts/serve-ui.js` + `scripts/ui-stub/generic-juce-stub.js` | Assemble the served tree; `window.__stubEmit`, `window.__stubStates`, `window.__stubNativeCalls` | frame-probe, the i18n states |
| `uv run --no-project --python 3.12 --with pedalboard --with numpy` | Installed-VST3 measurers | verify_2_* re-runs and the new verify_3_state.py |
| Node ≥ 18 built-ins only | `scripts/generate_sh_icons.mjs` | 3.2, offline |

### Alternatives considered

| Instead of | Could use | Why not |
|---|---|---|
| Pre-formatted payload string | `juce::var` tree (FM / Additive style) | JUCE serialises doubles with up to 15 decimals (`juce_String.cpp:2287` `serialiseDouble`). Bytes are not stable, so fixtures cannot be compared. Size is irrelevant either way (§Q4) |
| 3D generated icons | Mockup's 2D-slice `shIconSvg()` (`v1-ui.html:1610`) | S / T and L / M become mirror images of the same clover in different, unlabelled planes. Keep it as the fallback the checkpoint can pick |
| Harness-made fixture frames | Hand-written JS frames | Memory: `pattern_ui_fixture_state_must_be_producible_by_cpp` |

**Installation:** none. No new package enters the build or the page.

## Package Legitimacy Audit

No external package is installed by this phase. Playwright and pedalboard are already used by the repo's existing gates. The icon generator uses Node built-ins only.

| Package | Registry | Verdict | Disposition |
|---|---|---|---|
| (none) | — | — | — |

- **Packages removed due to [SLOP]:** none.
- **Packages flagged [SUS]:** none.

---

## Don't Hand-Roll / Reuse

| Problem | Don't build | Use instead (file:line) | Why |
|---|---|---|---|
| Editor skeleton, relay / attachment order, natives | A fresh editor | `.planning/mockups/v1-PluginEditor-TEMPLATE.{h,cpp}`, with the fixes in §Q1 | Already merges Wavetable v1.0.1 and the Additive scale pattern for these 12 IDs |
| Host gesture left open on editor close | — | Wavetable `PluginEditor.h:78-87`, `PluginEditor.cpp:529-555` (in the template at h:97-107, cpp:331-353) | `~WebSliderParameterAttachment` only removes the listener (`juce_ParameterAttachments.cpp:290-293`) |
| Resizable fixed-aspect window | — | O-simpleAdditive `PluginEditor.cpp:224-243` (stored scale read before setSize, constrainer, fit), `:292-297` (`resized` records scale), `:302-315` (`fitScaleForDisplay`); JS `Source/ui/public/js/app.js:477-483` | Committed and shipped. O-simpleFM's copy is uncommitted WIP in the working tree (HEAD `PluginEditor.cpp:301` is still `setSize (760, 980)`, not resizable) |
| uiLanguage codec + persistence | — | O-simpleAdditive `PluginProcessor.h:169-195` (codec), `PluginProcessor.cpp:473-510` (written as a code string, `isVoid()` restore) | Avoids the bool / int-as-string trap |
| Lesson apply | Additive's reset-then-apply loop (`PluginProcessor.cpp:383-413` resets **every** parameter, including output) | Wavetable `PluginProcessor.cpp:1282-1305` `recipeTargets` + `:1332-1357` `applyNormalisedTargets`: one pass, skips the output slot, skip-if-equal, begin / set / end per parameter | One gesture per changed parameter; the output is never touched |
| Glossary-first i18n | Free-hand French / Chinese | `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js` + the two lints; `termNote` for reasoned exceptions | Memory: `pattern_parallel_localizers_need_glossary_first` |
| Headless page tree | Copying `Source/ui/public` | `serve-ui.js buildRoot()` (places `${CMAKE_SOURCE_DIR}` embeds at their served URL) | Hand-built trees 404 the module fonts (memory R5) |
| VST3 state decode in Python | — | JUCE base64 variant: `size.` prefix plus a 6-bit LSB-first alphabet `.ABC…xyz0123456789+` (decoded this session, §Q7) | Lets pedalboard see `uiLanguage` / `editorScale` |
| SN3D Y_acn formulas | Retyping | `Source/dsp/ShEncoder.h:107-136` (lifted verbatim, R4-verified) | Shared truth with the engine; the generator cross-checks against an independent recursion |

**Key insight:** every piece this stage needs already exists in a sibling, the template or the gate scripts. The new code is limited to:
- the payload header;
- the icon generator;
- the frame-probe;
- the five page fixes: compass, pole hold, dirty render, `live`, and ±180 format.

---

## Q1. Reference-editor mapping, and what to delete or adapt in the template

### What to take from where

| Concern | Source | Lines | Already in the template? |
|---|---|---|---|
| Class shape: `AudioProcessorEditor` + `Timer` + `AudioProcessorParameter::Listener` | Wavetable `PluginEditor.h` | 64-87 | yes (h:75-107) |
| `attach()` helper (3-arg, `nullptr` UndoManager, jassert on ID drift) | Wavetable `PluginEditor.cpp` | 68-75 | yes (cpp:65-72) |
| Named relays in declaration order → webView → attachments | Wavetable `PluginEditor.cpp` | 158-178, 180-205 | yes (h:131-160, cpp:155-192) |
| `getParameterDefaults` = `convertFrom0to1(getDefaultValue())` (engineering units) | Wavetable `PluginEditor.cpp` | 225-235 | yes (cpp:203-214) |
| `getUiLanguage` / `setUiLanguage` | Wavetable `PluginEditor.cpp` | 237-253 | yes (cpp:220-237) |
| `uiReady` emits **inside** the native, then completes | Wavetable `PluginEditor.cpp` | 263-279 | yes (cpp:244-250) |
| `applyFactoryPreset` native → processor bool + force emit | Wavetable `PluginEditor.cpp` | 343-349 | yes (cpp:258-265) |
| Gesture tracking + `closeOpenGestures()` before attachments die | Wavetable `PluginEditor.cpp` | 506-507, 529-555 | yes (cpp:299-301, 331-353) |
| Resizable fixed aspect + `fitScaleForDisplay` | **O-simpleAdditive** `PluginEditor.cpp` | 224-243, 292-315 | yes (cpp:306-327, 417-441) |
| Viz Timer at 30 Hz emitting with `emitEventIfBrowserIsVisible` | O-simpleFM HEAD `PluginEditor.cpp` 311-343 / Additive 253-285 | — | yes, but rewritten as a single hash-gated event |
| JS `applyUiScale` / `makeCanvas` with uiScale folded into the DPR | Additive `app.js:477-500` | — | already in the page (`v1-ui.html:1525-1556`) |

`[VERIFIED: read this session]` Do **not** read O-simpleFM's working tree as the reference. `git diff` shows its resizable editor, `editorScale` and EB Garamond branches as uncommitted (in flight in another session). HEAD `PluginEditor.cpp:301` is `setSize (760, 980);` with no `setResizable`.

### Delete or adapt in the template (an effect: no keyboard, no import)

The template is already effect-shaped: no `uiMidi`, `importAudio`, `releaseUiHeldNotes`, `stepKnobDrag`, `FileChooser`, `closeStepKnobGestures` or preset-manager natives. **Nothing is left to delete.** Adapt five things:

1. **`editorScale` type, persistence and clamp.**
   - The template reads and writes a plain `float` member (h:55, cpp:307, cpp:422). Make it `std::atomic<float> editorScale { 0.0f }`, because `getStateInformation` may run off the message thread in some hosts `[ASSUMED]`.
   - At open: `scale = stored > 0 ? jmin (stored, fitMaxForDisplay()) : fitScaleForDisplay()`. `fitMax` = `jlimit (minScale, maxScale, available / designH)`, so a projector session (1.5×) reopened on a 13" laptop is not off-screen. Details in §Q7.
2. **Payload builder.**
   - Replace `emitMeterFrame`'s inline formatter (cpp:355-403) with a call into a new pure header, `Source/MeterFramePayload.h`, described in §Q4.
   - The template's comment "the one per-tick allocation is its growth, once" (h:121, cpp:172) is wrong. `juce::String::clear()` **releases** the buffer: `[VERIFIED: /Users/taylorbrook/JUCE/modules/juce_core/text/juce_String.cpp:265-269]`, quoted verbatim as `StringHolderUtils::release (text); text = emptyString.text;`. On top of that, `appendFixed` builds a temporary `juce::String` per field (cpp:102).
3. **Drain read in the constructor.** Call `processorRef.meterFrames.read();` once before `startTimerHz`. While no editor is open, nothing consumes `hasNewData()`, so the audio side keeps max-accumulating `shPeak` / `outPeak` indefinitely. That is the reset rule at `PluginProcessor.cpp:268-272`, quoted verbatim as `if (! meterFrames.hasNewData()) { engine.resetPeaks(); outPeakAccL = outPeakAccR = 0.0f; }`. Without the drain, the first frame after opening flashes the max since the last close.
4. **Stale / live rule:** §Q4.4.
5. **`createEditor` include guard.** The template expects `#if JUCE_WEB_BROWSER #include "PluginEditor.h"` above `createEditor()` (template cpp:32-36). The exact shape is in memory `pattern_render_harness_breaks_on_webview_editor`: guard both the include and the body.

Keep exactly as is:
- the `setSize (1180, 780)` numeric literal as the **last** numeric `setSize` in the file (cpp:313). `serve-ui.js readEditorSize()` takes the last `/setSize\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)/` match (`scripts/serve-ui.js:218-231`), and the scaled `setSize (roundToInt…)` does not match the regex;
- the getResource bare-path ladder (cpp:115-149);
- `#if JUCE_WINDOWS withUserDataFolder` (cpp:267-277);
- `withKeepPageLoadedWhenBrowserIsHidden()`.

---

## Q2. The 16 SH pattern icons

### ACN → letter order (verified)

| ACN | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| letter | W | Y | Z | X | V | T | R | S | U | Q | O | M | K | L | N | P |
| (l, m) | 0,0 | 1,−1 | 1,0 | 1,1 | 2,−2 | 2,−1 | 2,0 | 2,1 | 2,2 | 3,−3 | 3,−2 | 3,−1 | 3,0 | 3,1 | 3,2 | 3,3 |

- `[CITED: https://en.wikipedia.org/wiki/Ambisonic_data_exchange_formats, component ordering, ACN = l² + l + m]`
- This matches the page `[VERIFIED: v1-ui.html:1035]`, quoted verbatim as `const ACN_LETTER = ['W', 'Y', 'Z', 'X', 'V', 'T', 'R', 'S', 'U', 'Q', 'O', 'M', 'K', 'L', 'N', 'P'];`.
- The page's `shVec()` (`v1-ui.html:1563-1572`) is algebraically identical to `ShEncoder.h:107-136`. I expanded all 16 by hand; for example, `S58·y(3x²−y²) = kSqrt5_8·sin3A·cos³E`.
- **Terminology fix for a teaching plugin:** these are the **FuMa letter names shown in ACN (AmbiX) order**. AmbiX itself numbers the channels. CONTEXT and the yaml say "AmbiX letter name". The `chanWell` tooltip should say "FuMa letters, ACN order".

### Generator (prescribed)

- **File:** `plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs`. The plugin-local `scripts/` already holds `generate_binaural_filters.py` and `FILTERS.md`.
- **Dependencies:** Node built-ins only.
- **Run:** `node plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs`.
- **Output:** `Source/ui/public/js/shicons.js`, one ES module that only exports:
  ```js
  export const SH_ICONS = Object.freeze([ { acn: 0, letter: 'W', paths: [ { d: 'M…Z', pol: 1, shade: 2 }, … ] }, … ]);
  ```
  - The name has no hyphen. BinaryData strips hyphens, so the symbol becomes `BinaryData::shicons_js`.
  - The page builds `<path>` nodes with `createElementNS`, never `innerHTML`. It is a drop-in for `shIconSvg(acn)`, keeping the `viewBox="0 0 18 18"` box (`v1-ui.html:1610-1659`).
  - Add it to `juce_add_binary_data` SOURCES and to a `getResource` branch (`/js/shicons.js`, `application/javascript; charset=utf-8`). check-i18n [12] will scan it; a data-only module writes no prose.

**Rendering:**
- **Surface:** r(θ, φ) = |Y_acn| / max|Y_acn| on a 24 (θ) × 48 (φ) mesh. Per-icon normalisation, because under SN3D max|Y| is below 1 for non-zonal channels (V peaks at √3/2), and an icon is a shape glyph, not a level.
- **Camera:** orthographic, "the top-down canvas tilted toward you", consistent with the sphere view where front is up and left is left.
  - Look direction: from above-behind, about 55° down from the horizon, plus a 20° turn toward the right so the X and Z axes do not superimpose.
  - Screen x = −y_world·cos + …; derive it from a single rotation matrix in the script and expose the two angles as constants.
  - Tune the angles at the checkpoint with the contact sheet.
- **Draw order:** back-face culled quads, sorted back-to-front (painter's).
- **Colour:** polarity = sign of Y at the quad centre. Sage `#9ec46f` for positive, amber `#e8b04a` for negative (`v1-ui.yaml:43-44`). Three Lambert shade steps per polarity from a fixed light. Coordinates to 2 decimals.
  - Expect about 4–9 KB per icon (about 100 KB total; fine — the HRTF header is 277 KB).
- **Size:** keep the mockup box. It is an 18 px icon in the 22 px column; the yaml says "SH icon 22px" in the grid (`v1-ui.yaml:268`) and "same 18px box" (`v1-ui.yaml:277`). No stroke under 0.6 viewBox units; the fills carry the shape. Vector, so it stays sharp at 1.5× uiScale × DPR 2.

**Self-checks (exit non-zero on failure):**
1. The closed forms (copied from ShEncoder.h) agree with an **independent** SN3D formula, `N_l^|m| · P_l^|m|(sin e) · (cos m·a | sin |m|·a)` with the associated-Legendre recursion and **no** Condon–Shortley phase, to 1e-12 on a 13 × 7 grid. This mirrors RESEARCH-2.1 R4.
2. The letter table equals the Wikipedia row above.
3. Polarity anchors: X(front) > 0, Y(left) > 0, Z(up) > 0, R(up) > 0 (zonal), U(front) > 0, V(front-left 45°) > 0.
4. Determinism: two runs produce byte-identical output. The output header carries the generator's sha256 and the camera constants.
5. Also write a contact sheet, `.planning/stages/3-gui/sh-icons-contact.html`, with the 16 icons at 1×, 1.5× and 3× on the walnut well, plus the mockup's 2D-slice versions beside them. The user picks 3D or slice at the visual checkpoint, and the fallback is free.

---

## Q3. Canvas drag → slider state (JUCE 8.0.15)

### The frontend API, verified

`[VERIFIED: /Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js:135-270]`:
- `setNormalisedValue(n)` sets `scaledValue = snapToLegalValue(normalisedToScaledValue(n))` and emits `{eventType: "valueChanged", value: scaledValue}`. The **scaled** value goes to C++.
- `normalisedToScaledValue` is `pow(n, 1/skew)·(end−start)+start`.
- `snapToLegalValue` returns the value unchanged when `interval == 0`.
- `sliderDragStarted()` and `sliderDragEnded()` emit their event types.

On the C++ side `[VERIFIED: juce_ParameterAttachments.h:299-300, .cpp:77-83, 324-333]`:
- `sliderDragStarted → attachment.beginGesture()`.
- `sliderValueChanged → setValueAsPartOfGesture(value)`, which calls **only** `setValueNotifyingHost`. No begin / end of its own, so the page **must** bracket every drag.
- `sliderDragEnded → endGesture()`.

The mockup does exactly this (`v1-ui.html:2179-2228`): `sliderDragStarted()` on pointerdown, then `setNormalisedValue(scaledToNorm(st, deg))` per move, then `sliderDragEnded()` on pointerup / pointercancel / lostpointercapture. `onUp` is idempotent. `scaledToNorm` (`:1129-1135`) is `pow(proportion, skew)`, the exact inverse of the frontend map, fed from the live `st.properties`. No JS range table.

### Normalisation

- azimuth: `NormalisableRange<float> (-180, 180, 0)`, skew 1. Norm = (deg + 180) / 360. Exactly 0.5 → 0° (parameter-spec Notes). `[VERIFIED: Source/PluginProcessor.cpp:84-93]`
- elevation: (deg + 90) / 180, same helper with `halfRange 90`.
- Always go through `scaledToNorm(st, v)`, never a literal formula.

### AmbiX mapping on the top-down canvas

`azimuth = atan2(−dx, −dy)` in screen pixels (y down): pointer straight up gives 0° (front), pointer left gives +90°, below gives ±180° (`v1-ui.html:2219-2223`). It matches `azToScreen()` (`:1743`): sx = cx − sin(az)·r, sy = cy − cos(az)·r. That is AmbiX x-front, y-LEFT seen from above, consistent with ShEncoder's "+π/2 = left". Correct as written.

### The ±180 wrap

- `atan2` returns (−180, 180], so crossing the rear jumps the parameter from about +179 to −179 inside one gesture. Both are legal.
  - The engine's smoother takes the shortest path (`parameter-spec.md:24`), so the **live** audio is clean.
  - The frame reports `azEffDeg` in [−180, 180) (`MeterFrame.h:43-44`), so a +180 parameter shows the puck at −180. That is the same point.
- **Host-automation limitation (cannot be fixed under the frozen 12-parameter set):** hosts that **ramp** between recorded automation nodes (Logic draws a line between +179 and −179) replay a sweep through 0° (front) unless the two nodes are adjacent in time. Put it in the azimuth tooltip / glossary ("crossing behind you jumps from +180 to −180") and check it hands-on in Logic (Open risk R2). `[ASSUMED: Logic's interpolation behaviour]`
- Change the dead zone from 3 px (`:2221`, `dx*dx + dy*dy < 9`) to **the head radius** (`TOP.rHead` = 20). A click on the head then does not throw the source to an arbitrary angle, and angles near the centre are numerically noisy.
- Readout: format |v| ≥ 179.95 as `180.0°` with no sign. That is the carried Stage-2 item "−180 displays as 180" (`stages/2-dsp/VERIFICATION.md:83`). Today `fmtDeg` (`:1087-1090`) prints `+180.0°` and `−180.0°`.

The side drag `elevation = atan2(−dy, max(dx, 0.001))` clamped to ±90 is fine (`:2225-2228`).

---

## Q4. The `meterFrame` payload

### 4.1 Format: one fixed-point string, built without allocation, then one `juce::String`

`[VERIFIED: juce_WebBrowserComponent.cpp:414-429]`: `emitEvent` always runs `JSON::toString(object)`, two `replace()`s, `quoted('\'')` and a concatenation into an `evaluateJavascript` call. That is roughly 5–6 allocations per emit **whatever the payload type**, so neither route reaches zero allocations. `[VERIFIED: :607-611]`: `emitEventIfBrowserIsVisible` only checks `isVisible()`.

| Route | Bytes / frame | Determinism | Fixture equality |
|---|---|---|---|
| `juce::var` tree (`DynamicObject` + 2 × `Array<var>`) | ~1.2–1.6 KB (doubles at 15 dp via `serialiseDouble`, `juce_String.cpp:2287-2312`) | Unquantised floats unless they are re-quantised to doubles that print cleanly (not guaranteed) | No |
| **Pre-formatted string** (template shape) | ~550 B including JSON-string escaping | Exact | **Yes** |

At 30 Hz either costs nothing measurable. Pick the string for determinism.

**Prescribed:** `Source/MeterFramePayload.h`.
- Includes only `dsp/MeterFrame.h`, `<charconv>` and `<cstdint>`. **No JUCE**, so the harness can include it.
- API:
  ```cpp
  struct QuantisedFrame { int gain[16], peak[16], azEff, elEff, azRot, elRot, orderActive, mix, outL, outR, latency; bool live; };
  QuantisedFrame quantise (const MeterFrame&, bool live) noexcept;    // q3 = lround(v*1000), q2d = lround(v*100); !live → peaks 0
  std::uint64_t  hash (const QuantisedFrame&) noexcept;               // FNV-1a 64 over the ints (template cpp:81-91)
  int            format (const QuantisedFrame&, char* buf, int cap) noexcept;   // std::to_chars on ints + manual '.' (template appendFixed logic), returns length
  ```
- The editor holds `char buf[1024]` as a member and emits `juce::var (juce::String::fromUTF8 (buf, len))`.
- **Never `snprintf("%f")`:** `LC_NUMERIC` is process-wide, and a host that sets a comma locale turns `0.707` into `0,707`, breaking `JSON.parse`. `std::to_chars` is locale-independent. `[CITED: cppreference std::to_chars — "locale-independent"]`
- `q3(-4.4e-8)` → 0 → `"0.000"`. There is never a `-0.000`, because the sign comes from the scaled int.

**Field names:** exactly the page's reads (`v1-ui.html:1719-1733`) and `MeterFrame.h` — `shGain`, `shPeak`, `azEffDeg`, `elEffDeg`, `azRotDeg`, `elRotDeg`, `orderActive`, `decoderMix`, `outPeakL`, `outPeakR`, `latencySamples` — plus the new `live` (0/1).

`[VERIFIED: Source/dsp/MeterFrame.h:77-84]`, verbatim:
```cpp
float shGain[16] {};
float shPeak[16] {};
float azEffDeg = 0.0f, elEffDeg = 0.0f;
float azRotDeg = 0.0f, elRotDeg = 0.0f;
int   orderActive = 4;
float decoderMix  = 1.0f;
float outPeakL = 0.0f, outPeakR = 0.0f;
int   latencySamples = 64;
```

### 4.2 Hash and quantisation

- **Quantisation:** gains, peaks, mix and output peaks at 1e-3; angles at 0.01°. The template already does this (cpp:78-79). The hash covers exactly the emitted ints, so a frame either changes the hash or prints the same bytes.
- **Reset:** `lastFrameHash = 0`, `forceFrameEmit = true` at construction, so the first tick always emits.

### 4.3 Zero emission at rest

| Scene | Emits? |
|---|---|
| Transport playing | Every tick (peaks move) — expected |
| Orbit on | Every tick (azEff moves) |
| Parameters still, host still calling `processBlock` with silence | Peaks quantise to 0 after one read; shGain, angles and mix are constant → **nothing** |
| Host stopped calling `processBlock` | See 4.4 |

### 4.4 NEW — staleness / `live`

Two real defects in the mockup + template pair, both producible by C++ and both fixed editor-side without touching the frame:

- **(a) Held peaks freeze.**
  - When the host idles the audio thread, the triple buffer is never republished, so `read()` returns the last frame and its non-zero `shPeak` forever.
  - The hash never changes, so nothing is emitted.
  - The page rule `p >= disp ? p : decay` (`v1-ui.html:1723-1724`) holds `disp.peak` at p **indefinitely**.
  - Analogue in memory: `pattern_generation_reaper_freezes_when_host_idles_audio_thread`.
- **(b) Before the first block,** the read slot is a default `MeterFrame` with **`shGain` all 0**, so the W bar shows 0, not +1, and the puck sits at 0° whatever the azimuth parameter says.

**Fix (C++, editor only):**
```cpp
const bool fresh = processorRef.meterFrames.hasNewData();   // TripleBuffer.h:46-47: may be polled from either side
const auto& f    = processorRef.meterFrames.read();
staleTicks = fresh ? 0 : staleTicks + 1;
everPublished |= fresh;
const bool live = everPublished && staleTicks < 3;           // ~100 ms
```
`quantise(f, live)` zeroes `shPeak` / `outPeak` when `!live`, so exactly one zero-peak frame is emitted, then silence. `live` rides the payload.

**Page (needs user confirmation; it touches the locked "puck follows the frame"):** when `live === 0`, draw the puck, bars and beam from the **parameters**, using the page's existing display maths (`shVec`, `dirOf`, `rotMatrix`, `heardAtFromParams`; the mockup's mock frame engine in `v1-ui-test.html` is exactly this), and dim the B-format well slightly. At steady state the two sources agree; the frame only adds smoothing. Without the fallback, a teacher who opens the editor on a stopped, non-live Logic track sees empty bars. `[ASSUMED: Logic suspends processBlock for non-selected, non-armed tracks when stopped]`, to be checked at the Logic hands-on.

### 4.5 uiReady, visibilitychange, emitEventIfBrowserIsVisible

- `uiReady` sets `force` and emits **inside** the native, then completes (template cpp:244-250, Wavetable pattern). The page registers its `meterFrame` listener first, then calls `uiReady`, and calls it again on `visibilitychange → visible` (`v1-ui.html:2251-2265`).
- The language is **pulled** by the canon (`getUiLanguage`), not pushed. That satisfies CONTEXT's "+ the current language" without a second event.
- `emitEventIfBrowserIsVisible` gates on the WebBrowserComponent's **own** `visibleFlag` (`juce_WebBrowserComponent.cpp:607-611`). This editor never calls `setVisible(false)` on the view, so completions and events keep flowing while the host window is hidden. Memory `critical_webview_completion_gated_on_isvisible`, as corrected on 2026-08-25.
- The re-send exists for the page's own reload and occlusion cases. **Never chain a poll off a promise** — the page does not poll at all.
- `uiReady`'s `read()` is a second reader call on the **same** (message) thread, which `TripleBuffer.h:46-47` allows.

---

## Q5. UI-01 / UI-02 gates from states C++ can produce

### Live capture feasibility

| Route | Feasible? | Verdict |
|---|---|---|
| Capture from the Standalone's WKWebView | Not headless. Web Inspector only, by hand | Hands-on only |
| Env-var dump in the editor | Possible, but ships debug code | No |
| **Harness fixture mode, using the editor's own `MeterFramePayload.h`** | Yes: the harness already renders `processBlock` and reads `meterFrames` (e.g. `tests/render-harness/main.cpp:2108, 2394`) and has `setParam` helpers (`:399-410`) | **Recommended.** The fixture bytes are the editor's bytes for a real processor state |

### Harness fixture mode

- Triggered when the env var `OSAMB_UI_FIXTURES=<dir>` is set. The default run stays as it is: 241 checks plus the state probes (§Q10).
- Writes `tests/ui-fixtures/frames.json`: `{ name, params, payload }[]`. Each entry comes from a fresh processor at 48 k with these parameters set, about 0.5 s of noise processed in 64-sample blocks so the smoothers settle, `read()`, then `format()`:
  - `front_o1` (az 0), `left_o1` (az 90), `behind_o1` (az 180), `right_o1` (az −90)
  - `overhead_o2` (el 90, order index 1)
  - `o1` / `o2` / `o3` at az 45
  - `yaw90_front` (az 0, yaw 90)
  - `pole_rot` (el 89.999, yaw 30, pitch 20) — elRot near the pole
  - `stereo_o3` (decoder 1, order index 2)
  - `silent_after_play` (peaks 0), `notlive` (`live` 0)
- Also asserts the C++-side facts:
  - `left_o1` gives shGain[1] = 1.000 and [3] = 0.000, with [0] = 1.000 in both front and left.
  - Orders give orderActive 4 / 9 / 16.
  - The same frame twice gives an equal hash; a changed peak gives a different hash.
  - `!live` gives every peak "0.000".
- Commit `frames.json`. It is deterministic; prove it with two runs byte-identical.

### `tests/ui-probes/frame-probe.mjs`

Modelled on `plugins/O-simpleWavetable/tests/ui-probes/wheel-gesture-probe.mjs`: the generic stub, `--root`, `--negative-control`, and an exit code equal to the number of failed arms.

- **Serve:** `buildRoot('O-simpleAmbisonics')` with the plugin's `tests/ui-stub/generic-overrides.json`, which must carry the **real** ranges. Neutral 0..1 ranges make the probe vacuous (memory `pattern_generic_ui_stub_seeds_neutral_ranges_gate_state_vacuous`).
  - Viewport = `readEditorSize()` = 1180×780, `deviceScaleFactor` 2.
- **Inject:** `page.evaluate(p => window.__stubEmit('meterFrame', p), payloadString)`. The page's `onMeterFrame` parses a string (`v1-ui.html:1716-1718`). Wait **until converged**: poll a pixel read until two consecutive reads match; do not sleep blindly (memory `pattern_quiesce_before_stimulus_in_async_ui_gates`).
- **Read the render, not internal state** (memory `pattern_pointer_moved_is_not_audio_changed`): `getImageData` on `#chanCanvas`.
  - For ACN k, take the `.bar-cell[data-acn=k]` rect (design px; the probe runs at uiScale 1).
  - Scan the bar's centre row outward from the bar centre for sage (right, positive) or amber (left, negative) pixels.
  - Bar length / half-width = |value| ± 2 px.

**Arms:**

| Arm | Requirement | Assertion |
|---|---|---|
| A1 | UI-01 | `front_o1`: W ≈ +1, X ≈ +1, Y ≈ 0. `left_o1`: W ≈ +1 (unchanged within 2 px), Y ≈ +1, X ≈ 0 — the acceptance sentence |
| A2 | UI-01 | `behind_o1`: X amber (negative), length ≈ 1 |
| A3 | UI-01 | `o1` / `o2` / `o3`: `.band.on` count is 2 / 3 / 4, inactive band rows dimmed (computed opacity < 0.5); 16 icons present in ACN order with the letters from §Q2 |
| A4 | UI-02 | Puck centroid (brass pixel cluster on `#topCanvas`) for `left_o1` lies left of the head centre at radius r(distance = 1 m) ± 3 px |
| A5 | UI-02 | `yaw90_front`, with the stub's yaw state also set to 90: puck centroid **unchanged** vs `front_o1` (± 1 px); the sage rim tick moves to the left |
| A6 | UI-02 | Drag: monkey-patch `window.__stubStates.sliders.get('azimuth')`'s `sliderDragStarted` / `sliderDragEnded` to count calls. Pointer path from the right side → behind → the left side: starts = ends = 1; the values pass ±180 with **no** sample of \|az\| < 90 in the rear crossing; the final value is about +90 ± 1 |
| A7 | UI-02 | Same for elevation on `#sideCanvas` (up = +) |
| A8 | UI-03 | Beam width at −3 dB (angular extent of sage fill at r ≥ 0.707·max) strictly decreases o1 → o2 → o3 |
| A9 | §Q6 | `pole_rot` then a second pole frame whose `azRotDeg` differs by 120° and `elRotDeg` ≥ 89.99: the rim tick does not move |
| A10 | UI-05 | `stereo_o3`: the decoder canvas changes (pixel diff > threshold after easing) |
| A11 | §Q4.4 | `notlive` after `left_o1`: peak overlay pixels decay to 0 within 1 s |

**Negative controls:**
- (i) Feed `left_o1`'s payload with shGain[1] and shGain[3] swapped (FuMa-style WXYZ ordering): A1 must FAIL.
- (ii) Serve a page copy with the pole hold removed: A9 must FAIL.
- A control that passes means the arm is decoration. Memory `pattern_probe_must_target_the_branch_the_fix_changed`.

---

## Q6. Compass with pitch / roll, and the pole hold

**Rotation:** R₃ = Rz(ψ)·Ry′(θ)·Rx(φ). `[VERIFIED: Source/dsp/ShRotator.h:45-48, 164-181]`, verbatim:
- `Rz(ψ) = [[ c, −s, 0], [ s, c, 0], [ 0, 0, 1]]   +ψ: front → LEFT`
- `Ry′(θ) = [[ c, 0, −s], [ 0, 1, 0], [ s, 0, c]]   +θ: front RISES`
- `Rx(φ) = [[ 1, 0, 0], [ 0, c, −s], [ 0, s, c]]   +φ: left RISES`

The page's `rotMatrix()` (`v1-ui.html:1578-1587`) builds the identical matrices. **No sign conflict.**

**Scene front:** f = R·x̂ = (cψ·cθ, sψ·cθ, sθ). Roll does not appear.
- Heading h = atan2(sψ·cθ, cψ·cθ). That is **ψ when cos θ > 0**, **ψ + 180° when cos θ < 0**, and undefined at cos θ = 0.
- The mockup turns the rose by eased `yaw` only (`:1790` `ctx.rotate(-yaw * D2R)`). That is correct for |pitch| < 90°, but `pitch` spans ±180°. At pitch 180 / roll 180, Ry′(180)·Rx(180) = Rz(180), so the scene front is heard **behind** while the ▲ still points at `yaw`.

**Prescribed rose:**
1. Rose rotation = −h·D2R, with h computed from the eased `yaw` / `pitch`. This is identical to today whenever |pitch| < 90°, which honours "the compass turns by yaw".
2. The amber chevron (`:1806-1813`) shows the **front elevation** asin(sθ), as a ▲ / ▼ whose length is ∝ |sin θ|. Today it rotates by `−pitch·0.5`, which says nothing quantitative.
3. Roll is **not** drawn on the rose (it does not move the front). It reaches the picture through the frame's `azRot` / `elRot`: the beam, the rim ticks, and the "heard at" readout, which is already display-side from parameters (`heardAtFromParams`, `:2055`).
4. Rose alpha = max(0.35, |cos θ|). The rose fades as its heading becomes ill-defined, which is the gimbal note made visible.
5. **Hold:** when |cos θ| < 1.75e-4 (θ within 0.01° of ±90°), keep the last h.

**azRot pole hold (beam centre + top rim tick):**
- `[VERIFIED: Source/dsp/SpatialEngine.h:374-391]`: the frame computes `azRotDeg = wrapTo180 (atan2 (y, x))`, which is noise within ≈ 0.01° of the pole. The identity fast path sets `azRotDeg = az.cur` exactly.
- The page copies `disp.azRot = f.azRotDeg` unconditionally (`v1-ui.html:1729`). Add: `if (Math.abs(f.elRotDeg) < 89.99) disp.azRot = f.azRotDeg;` (the quantised value is ≥ 8999 centi-degrees at the hold).
- The beam is unaffected, since at cos(elRot) ≈ 0 the horizontal slice is isotropic (`:1820-1828`). Only the tick needs the hold; draw it at 50 % alpha while held.
- `azEffDeg` (the puck) never needs a hold: it is the smoothed **parameter**, defined at ±90° elevation.

---

## Q7. `editorScale` + `uiLanguage` in the state XML

### Today

`[VERIFIED: Source/PluginProcessor.cpp:361-379]`:
- `getStateInformation` writes `apvts.copyState()` plus `pluginVersion`.
- `setStateInformation` runs `replaceState` when `hasTagName (apvts.state.getType())`.
- The harness never round-trips state: grep finds no `getStateInformation` / `setStateInformation` in `tests/render-harness/main.cpp`.

### Prescribed

Additive shape, plus `editorScale`:
```cpp
// get: on the COPY (never mutate apvts.state here)
state.setProperty ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);
const float es = editorScale.load (std::memory_order_relaxed);
if (es > 0.0f) state.setProperty ("editorScale", juce::String (es, 3), nullptr);   // "0.850"; absent = never sized
// set: after replaceState
if (const auto v = apvts.state.getProperty ("uiLanguage"); ! v.isVoid())
    uiLanguage.store (languageIndex (v.toString()), std::memory_order_release);
if (const auto v = apvts.state.getProperty ("editorScale"); ! v.isVoid())
    if (const float s = v.toString().getFloatValue(); std::isfinite (s) && s >= 0.6f && s <= 1.5f)
        editorScale.store (s, std::memory_order_relaxed);
```

- **Type traps.** Every property comes back as a **String** var after the XML round-trip. Use `isVoid()` as the presence test and parse the string; never use `isDouble()` / `isInt()` / `isBool()`. Memory `critical_valuetree_xml_roundtrip_loses_type`.
- **Migration.** A pre-Stage-3 session has neither attribute, so it gets English and fit-to-display. An unknown code (`"de"`) gives English. Garbage `editorScale` (`"abc"` → 0) is rejected and the session fits the display.
- **Display clamp at open** (§Q1.1). A 1.5× size saved on a projector is reduced to what the current display fits.
- **Concurrency.** `editorScale` and `uiLanguage` are atomics, read in `getStateInformation` from any thread.
- **Divergence to record.** ARCHITECTURE §State Persistence says "Custom state: none in v1.0". CONTEXT (Stage 3) overrides it with exactly these two non-parameter properties. The sibling convention (Additive / FM: `editorScale` session-only, "NOT written to state") is overridden by CONTEXT's locked "in the state XML" plus the Logic "save / reopen keeps the window size" hands-on. Note it in SUMMARY.

### Effect on the existing gates

- **`verify_2_1.py` V9, `verify_2_2.py` V9, `verify_2_3.py` V10** compare **decoded parameter values** only. `[VERIFIED: tests/pedalboard/verify_2_1.py:161-169, verify_2_2.py:207-215, verify_2_3.py:357-365]` Extra XML attributes cannot change their verdicts. Re-run them unchanged; they are Stage-2 verdict files, so do not edit them.
- **Growth:** add a new file, `tests/pedalboard/verify_3_state.py`. The VST3 `raw_state` is `<VST3PluginState><IComponent>578.…` in JUCE's base64 variant: decimal size, then `.`, then 6-bit groups LSB-first over the alphabet `.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+`. `[VERIFIED: decoded the installed O-simpleAmbisonics-dev.vst3 this session: 578 bytes → "VC2!" + <Parameters pluginVersion="1.0.0">…]`
  - **S1:** a fresh instance's decoded XML carries `uiLanguage="en"` and no `editorScale` (no editor ever opened).
  - **S2:** same-length patch `uiLanguage="en"` → `"fr"`, re-encode, `q.raw_state = …`, decode `q.raw_state`, assert `uiLanguage="fr"`. Same length keeps the `VC2!` size field valid. This proves the installed binary restores and re-saves.
  - **S3:** the 12 parameter values are unchanged through S2.
- **Harness state probes** in the default run (the only place that reaches the absent and garbage cases):
  - **ST1:** uiLanguage 2 → state → fresh instance gives 2, and the XML attribute is `"zh-Hans"`.
  - **ST2:** attribute removed gives 0; `"de"` gives 0.
  - **ST3:** editorScale 0.85 round-trips (± 1e-3); 0 writes no attribute.
  - **ST4:** `"abc"`, `"3.0"` and `"0.1"` are rejected (stays 0).
  - **ST5:** all 12 parameter values bit-equal across a round-trip, including `distance`'s 1.00000011920929 raw default (seen in the decoded state this session).
  - Precedent: O-simpleWavetable `tests/state-check/main.cpp:32-34, 288-342`.
- The harness count becomes **241 + 5 (+ the fixture-mode checks only when the env var is set)**. The plan must state the new denominator; the 241 Stage-2 checks and both digests are untouched. "Update the gate's expectation, never weaken it" (checklist §2).

---

## Q8. Polar plot cost, per-order caching, and the real cost

- **Beam:** `beamCurve(N)` already caches a 361-sample `Float32Array` per order (`v1-ui.html:1599-1608`). Each draw is 181 points × 2 lobes × (cos + acos + lookup), which is microseconds. Keep it as is.
- **Measured** (headless Chromium, 1180×780, DPR 2, the mockup page, the generic stub, CDP `Performance.getMetrics` over 3 s; `[VERIFIED: this session]`):

  | Page | TaskDuration / s | ScriptDuration / s |
  |---|---|---|
  | Mockup as is, static scene | **0.266** | 0.020 |
  | Mockup, 30 Hz frames | 0.272 | 0.020 |
  | `renderAll()` suppressed | **0.012** | 0.002 |

  Paint / raster of the five canvases redrawn on **every** rAF (`tick()`, `:2175`) costs about 25 % of a core while nothing changes. Script is negligible. WKWebView numbers were not measured, but it is the same work. `[ASSUMED: same order of magnitude]`
- **Prescribed:**
  - Per-canvas dirty flags. Redraw a canvas only when one of its inputs changed (new frame, parameter event, resize, `document.fonts.load` resolution, language change) or while its ballistics or eases have not converged (|target − disp| > 1e-3), or while a station fade is running.
  - Pre-render the puck (radial gradient + `shadowBlur` 8) once per uiScale × DPR into an offscreen canvas and `drawImage` it. `shadowBlur` is the costliest op per frame.
  - Keep a single rAF loop that early-outs when nothing is dirty.

---

## Q9. i18n: glossary draft, French lesson-row budget, gate baseline

### Baseline

I built a fixture repo root in the scratchpad from the mockup:
- `index.html` ← `v1-ui.html`; `js/i18n.js` ← `v1-i18n.js`;
- the template editor and the CMake snippet;
- eb-garamond and `insects.png`;
- a copy of `scripts/`.

Results:

| Gate | Result on the fixture |
|---|---|
| `check-i18n --plugin O-simpleAmbisonics --root <fixture>` | **ALL CHECKS PASS** (canon v2; 148 / 148 fr entries unreviewed) |
| `i18n-fr-lint` (run from the fixture's own `scripts/`, which resolve `ROOT = scripts/..`) | **CLEAN**, with 3 `termNote` exemptions (tangage ×2, `à l’horizontale`) |
| `i18n-zh-lint` | **PASSED**: 148 zh at `'mt'` (counted, not failed), 3 termNotes (俯仰 ×2, 水平) |
| `boot-all-uis --strict-tips` | clean 1/1, **0 DEAD, 0 late**, stub invented `uiReady` only |
| `check-ui-labels --root <fixture>` | **exit 10**; details below |

check-ui-labels findings, split into real defects and stub artefacts:

- **Real, fr:**
  - `label.subtitle` 501.6 > 460 (clipped)
  - `label.decLineBin` 223.8 > 206
  - `label.orbitOn` "Marche" 53.7 > 49. Use the glossary's narrower `activé` (46.33 px per `i18n-fr-glossary.js:101` comment) or pin the pill at 54.
  - `label.routeEncoder` 71.6 > 57
  - spill grew: `label.distOpen`, `label.orbitRate`, `label.rollSub`
  - `label.el.level` × `label.distOpen` intersect
  - [7] moved:
    - the header `.title-block` width changed (+35.3 px fr, −164.4 px zh), which moved `.header-strip`: **pin the title-block width**
    - `#topWrap .sph-tr` grew (23.9 fr / 13.1 zh), which moved `#sphYaw` / `#sphTilt`: **pin the right-aligned well text box**
- **Real, zh:** `label.distOpen` spill grows 6.0 → 11.7 px, colliding with the elevation knob cell.
- **Artefacts of `seed=neutral-defaults`:**
  - The page error `LEGENDRE[n] is not a function`: the stub's four neutral choices give an order index of 3, so `beamCurve(4)` runs.
  - Every readout measured at 0..1 values.
  - Fix both with `tests/ui-stub/generic-overrides.json` carrying the real ranges and choices. **Also clamp** the order index to 0..2 in the page (`choiceOf('order')` → `Math.min(2, Math.max(0, …))`); it is a free robustness fix.
- **Coverage:** 11 never-visible labels: settings popover rows, `#tips-toggle`, `#sphOrbitText`, band "+N ch" tags, `#seg-orbit` faces, encoder letters. Write `tests/i18n-states.json` with states **producible by C++**:
  - settings open; tips off;
  - Stereo decoder (`#seg-decoder` click);
  - order 3 (`#seg-order` click);
  - orbit on;
  - each lesson as its recipe's exact parameter values set through `window.__stubStates`, plus the caption. Clicking a lesson button in the stub only logs the native and does **not** move parameters.

### French lesson-row budget (measured, 1180×780, EB Garamond loaded)

| Lang | Row left | Row right | Button sum | Lines | Hint left | Slack before the hint |
|---|---|---|---|---|---|---|
| en | 84.4 | 927.2 | 800.7 | 1 | 1054.1 | 126.9 |
| **fr** | 80.9 | **917.5** | 794.6 | 1 | **1003.6** | **86.1** |
| zh-Hans | 56.9 | 715.0 | 616.1 | 1 | 1046.7 | 331.7 |

The draft fr (`Source ponctuelle (FOA)` 142.6 px, `Enceintes ou casque` 117.3, …) fits with **86 px** to spare. **Budget rule for review:** any reviewed fr change must keep Σ button widths + 7 × 6 px gaps ≤ (hint.left − 8) − row.left, roughly +78 px over today. Re-measure with the probe script below after every copy change. The `Lessons` label is not width-pinned; the row shifts left in fr / zh, which is harmless because the buttons themselves are `[data-i18n]`.

### Glossary draft (add an "ambisonics" section to both shared glossaries before any localizer pass)

| en (key) | fr | zh-Hans | Status / note |
|---|---|---|---|
| azimuth | azimut | 方位角 | zh in glossary (`i18n-zh-glossary.js:480`); fr precedent O-GrainScatter `Azimut` |
| elevation | élévation | 仰角 | zh in glossary (`:477`); fr precedent ×5 suite-wide |
| distance | distance | 距离 | both glossaries (`fr:354`, `zh:472`) |
| rotation | rotation | 旋转 | both (`fr:305`, `zh:476`) |
| yaw | lacet | 偏航 | **new** |
| pitch (rotation) | tangage | 俯仰 | **termNote** vs `'pitch': ['hauteur','haut']` (`fr:273`) / `音高` (`zh:531`); already drafted |
| roll | roulis | 横滚 | **new** |
| order (ambisonic) | ordre | 阶数 (阶 inside 一阶 / 二阶 / 三阶) | **new** |
| channel | canal / canaux | 声道 | **new**; mockup zh `B 格式声道` |
| B-format | format B | B 格式 | **new** |
| encoder / decoder | encodeur / décodeur | 编码器 / 解码器 | **new** |
| binaural | binaural | 双耳 | **new**; fr precedent O-Octagon `binaural` |
| headphones / speakers | casque / enceintes | 耳机 / 音箱 | **new** |
| soundfield | champ sonore | 声场 | **new** |
| spherical harmonic | harmonique sphérique | 球谐函数 | **new** |
| beam | faisceau | 波束 | **new** |
| orbit | orbite | 环绕 | **new**; matches the mockup draft |
| latency | latence | 延迟 | **new** |
| level (ear level) | à l’horizontale | 水平 | **termNote** vs `'level'` → niveau (`fr:116`) / 电平 (`zh:243`); already drafted |
| ACN, SN3D, AmbiX, FuMa, HRTF, MagLS, KU100, W X Y Z … P, FOA | unchanged | unchanged (U+0020 around Latin) | symbols, `sameAsEn` where whole-string |
| Binaural / Stereo, Mono sum / Left only, 1 / 2 / 3 | unchanged | unchanged | I18N_EXEMPT (automation names), locked |

`[ASSUMED]`: the **new** fr / zh renderings are standard technical usage from training, not verified against a terminology base. Taylor reads French (memory `user_reads_french`); zh stays at `'mt'` per CONTEXT.

### Measurement script

The lesson-row measurement used the scratchpad `measure.cjs`: `serve-ui.serve` + `resolvePlaywright` + `window.__setLanguage(l)` + `getBoundingClientRect` of `.tour-btn` / the hint. Re-create it as `tests/ui-probes/lesson-row-budget.mjs`, so the budget is a script and not an eyeball (memory `pattern_composed_readout_needs_setlabel_tokens_and_tour_row_budget`).

---

## Q10. Processor-edit regression protocol

Batch **every** processor edit into one 3.1 task, then run the following once. Run each command in the background: the 600 s watchdog applies (memory).

```bash
# 0. Re-configure (new editor sources + the binary-data target)
cmake -S . -B build -G Ninja            # OUARICON_BUILD_TESTS is already ON in build/CMakeCache.txt:631
# 1. Build ALL four, 0 warnings (Stage-2 practice, SUMMARY-2.3:168)
ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test 2>&1 | tee /tmp/osamb-build.log; grep -c "warning:" /tmp/osamb-build.log   # want 0
# 2. Harness x2 (byte-identical except the PERF-02 wall-clock line)
build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/Release/O-simpleAmbisonics-render-test
#    want: the 241 Stage-2 checks PASS + ST1..ST5, "digest-2.1 verdict … == 9189d911a1fe98d5" PASS,
#          "digest-2.2 verdict … == 7f35bc9f91e321b9" PASS (main.cpp:2252-2253, 2495-2496); digest-2.3 edd1acfd21f54bcd (info)
# 3. Install (dual-variant sweep + AU cache) and the Standalone explicitly
./scripts/build-and-install.sh O-simpleAmbisonics && ninja -C build O-simpleAmbisonics_Standalone
# 4. Hosts
auval -v aufx OSAm OuDv
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3   # x2
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/Components/O-simpleAmbisonics-dev.component   # x2
# 5. Independent measurers on the INSTALLED VST3
for v in 2_1 2_2 2_3; do uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy python plugins/O-simpleAmbisonics/tests/pedalboard/verify_$v.py; done   # 25 / 35 / 42 ALL PASS
uv run --no-project --python 3.12 --with pedalboard python plugins/O-simpleAmbisonics/tests/pedalboard/verify_3_state.py           # new
```

pluginval s10 opens the **WebView editor** for the first time in this plugin's history. Its editor tests now construct and destroy the editor repeatedly; that exercises member order and the gesture-close path.

### Keeping the render harness (`JUCE_WEB_BROWSER=0`) compiling

- `[VERIFIED: tests/render-harness/CMakeLists.txt:12-24, 65-70]`: the harness compiles **only** `../../Source/PluginProcessor.cpp` with `JUCE_WEB_BROWSER=0`, and borrows the plugin's `INCLUDE_DIRECTORIES` (`:32-35`).
- Guard both the include and the body of `createEditor()`, per memory `pattern_render_harness_breaks_on_webview_editor`.
- `JuceHeader.h` includes `BinaryData.h` only `#if JUCE_TARGET_HAS_BINARY_DATA`. That define is an **INTERFACE** definition of the binary-data target (`JUCEUtils.cmake:529-531`), so it reaches only targets that **link** `O-simpleAmbisonics_UIResources`. The harness does not, so `PluginProcessor.cpp` stays BinaryData-free.
  - `[VERIFIED: generated JuceHeader.h for O-simpleWavetable read this session: "#if JUCE_TARGET_HAS_BINARY_DATA  #include "BinaryData.h"  #endif"]`
- The new processor members (`std::atomic<float>`, `std::atomic<int>`, `applyFactoryPreset`) are juce_core / audio_processors only, so they compile under the harness.
- `Source/MeterFramePayload.h` is JUCE-free, so the harness can include it for the fixture mode.
- Build `O-simpleAmbisonics-render-test` **explicitly** in every 3.x build step. `build-and-install.sh` builds only `_VST3` / `_AU` (`scripts/build-and-install.sh:280`), so a broken harness would stay latent.

### applyFactoryPreset (in the same batch)

- One pass in the Wavetable style.
- Target for every parameter = default or recipe value (`convertTo0to1` of engineering units; choice / bool by index), **skipping `outputLevel` in both the reset and the recipe**.
- Skip a parameter already within tolerance; otherwise begin / set / end.
- Return `false` for an unknown id.

Stable ids `[VERIFIED: v1-integration-checklist.md:38]`, verbatim: `pointSource`, `orderStep`, `behindYou`, `overhead`, `rotateRoom`, `farNear`, `slowOrbit`, `speakersHeadphones`.

Recipes `[VERIFIED: v1-ui.yaml:371-379]`:

| id | Recipe |
|---|---|
| `pointSource` | `{ order: 0, azimuth: 30, elevation: 0, distance: 1 }` |
| `orderStep` | `{ order: 2, azimuth: 45 }` |
| `behindYou` | `{ azimuth: 180 }` |
| `overhead` | `{ elevation: 90, order: 1 }` |
| `rotateRoom` | `{ azimuth: 60, yaw: 120, order: 1 }` |
| `farNear` | `{ distance: 12 }` |
| `slowOrbit` | `{ orbit: true, orbitRate: 0.05, order: 1 }` |
| `speakersHeadphones` | `{ decoder: 1, order: 2, azimuth: 45 }` |

No recipe combines order 3 with distance < 1 m, which keeps the headroom note.

---

## Q11. Conflicts between the mockup and the locked conventions

| # | Item | Mockup / checklist | Locked / correct | Disposition |
|---|---|---|---|---|
| C1 | Lesson reset | Checklist §2: "reset EVERY parameter to its default … never `outputLevel`" | Resetting every parameter **sets** `outputLevel` to 0 dB (Additive's loop does exactly that, `PluginProcessor.cpp:391-396`) | **Fix:** exclude `outputLevel` from the reset as well (Wavetable `isPresetOutputSlot`) |
| C2 | Compass for \|pitch\| > 90° | Rose by `yaw` only (`v1-ui.html:1790`) | Scene front is heard at yaw + 180 | Heading from R·x̂ (= yaw for \|pitch\| < 90) — §Q6 |
| C3 | azRot near the pole | Unconditional copy (`:1729`) | MeterFrame.h:50-53: "the Stage-3 compass holds its last value there" | Add the hold (§Q6) |
| C4 | Letter names | "AmbiX letter name" (CONTEXT, yaml) | FuMa letters in ACN / AmbiX order | Tooltip / glossary wording only |
| C5 | ±180 readout | `+180.0°` / `−180.0°` | Stage-2 carry-over: "−180 displays as 180" | Format \|v\| ≥ 179.95 as `180.0°` |
| C6 | `editorScale` persistence | Template / siblings: session-only member | CONTEXT: in the state XML | Persist (§Q7), atomic, display-clamped |
| C7 | Payload allocation claim | "one per-tick allocation" | `String::clear()` frees the buffer | Pure-header builder (§Q4) |
| C8 | Language push on uiReady | CONTEXT: "C++ answers with a forced frame + the current language" | The canon pulls `getUiLanguage` once | Pull satisfies it; no change |
| C9 | ROADMAP puts UI-03 / UI-05 in 3.3 | The finalized page already draws both | CONTEXT cadence: 3.1 + 3.2 → checkpoint → 3.3 | Land them with the page in 3.2 and check them at the visual checkpoint; record the deviation |
| C10 | ARCHITECTURE "Custom state: none" | — | CONTEXT adds `uiLanguage` + `editorScale` | CONTEXT wins (later, locked) |

- **No conflict on:**
  - azimuth sign (+90 = LEFT, verified in the drag, `azToScreen`, `shVec` and ShEncoder);
  - what the puck follows: `azEff` / `elEff` from the frame, radius from the distance **parameter**;
  - the beam centre (`azRot`);
  - the 12-parameter set: page `KNOB_IDS` + segments + select = 12. `[VERIFIED: v1-ui.html:989-993]` `['azimuth', 'elevation', 'distance', 'yaw', 'pitch', 'roll', 'orbitRate', 'outputLevel']` + `['order', 'decoder']` + `['orbit']` + `['inputMode']` ↔ `[VERIFIED: Source/PluginProcessor.h:44-55]` ParamIDs `"azimuth"` … `"outputLevel"`.
- Choice strings `[VERIFIED: Source/PluginProcessor.cpp:109, 119, 136]`, verbatim: `StringArray { "1", "2", "3" }`, `StringArray { "Binaural", "Stereo" }`, `StringArray { "Mono sum", "Left only" }`. The page rebuilds options from `properties.choices`.

---

## Architecture Patterns

### Data flow

```
 host automation / page drag ──► Web*Relay ──► Web*ParameterAttachment ──► APVTS param ──► processBlock (engine, untouched)
        ▲                                                                                     │ publish() per block
        │ valueChanged echo                                                                    ▼
 page SliderState ◄── relay ◄── attachment (message thread)                       TripleBuffer<MeterFrame>
                                                                                              │ read() 30 Hz (+ hasNewData → live)
                                                                                              ▼
                                          MeterFramePayload.h: quantise → FNV-1a → (changed or forced?) ── no ─► (silence)
                                                                                              │ yes
                                                                                              ▼
                                          emitEventIfBrowserIsVisible("meterFrame", "{…}") ──► page onMeterFrame (JSON.parse)
                                                                                              │
                                                    rAF (dirty only): ballistics/ease → chan | top (beam, rose, puck) | side | decoder | out
 uiReady (init + visibilitychange) ──► native: force=true, emit now, complete(true)
 lesson click ──► applyFactoryPreset(id) ──► processor one-pass begin/set/end (no outputLevel) ──► relays echo ──► page; force next frame
 language select ──► setUiLanguage(code) ──► processor atomic ──► state XML "uiLanguage"
 window resize ──► editor resized() ──► editorScale atomic ──► state XML "editorScale" ; page applyUiScale()
```

### Files (end of stage)

```
plugins/O-simpleAmbisonics/
├── CMakeLists.txt                       # + PluginEditor.{h,cpp}, MeterFramePayload.h, UIResources target (first PRIVATE link)
├── scripts/generate_sh_icons.mjs        # NEW (3.2), offline, deterministic
├── Source/
│   ├── PluginEditor.{h,cpp}             # from the template + §Q1 fixes
│   ├── MeterFramePayload.h              # NEW, JUCE-free, shared with the harness
│   ├── PluginProcessor.{h,cpp}          # guarded createEditor, uiLanguage, editorScale, state, applyFactoryPreset
│   └── ui/public/
│       ├── index.html                   # v1-ui.html + §Q3/§Q6/§Q8 fixes
│       ├── js/i18n.js  js/shicons.js  js/juce/{index.js,check_native_interop.js}
│       └── img/insects.png              # md5 6d824230… (Additive == Wavetable)
└── tests/
    ├── render-harness/main.cpp          # + ST1–ST5, + OSAMB_UI_FIXTURES mode
    ├── ui-fixtures/frames.json          # NEW, harness-generated
    ├── ui-probes/frame-probe.mjs        # NEW
    ├── ui-probes/lesson-row-budget.mjs  # NEW
    ├── ui-stub/generic-overrides.json   # NEW, real ranges + choices
    ├── i18n-states.json                 # NEW (3.3)
    └── pedalboard/verify_3_state.py     # NEW
```

### Anti-patterns

- **Reading the frame for a readout.** It carries −11.999999 and −180.
- **`snprintf("%f")` into JSON** (locale).
- **Chaining anything off a native's promise.**
- **A hard-coded JS range table.** Use `st.properties` only.
- **Mutating `apvts.state` in `getStateInformation`.** Write onto the copy.
- **Committing `Source/ui/public/modules/*.js`.** None exist in v1.0; eb-garamond is a direct embed.

---

## Common Pitfalls

1. **Gesture left open when the host closes the editor mid-drag.** Track `parameterGestureChanged` and end open gestures in the dtor **before** the attachments die. Memory `pattern_web_slider_attachment_dtor_leaves_gesture_open`.
2. **Render harness breaks on the WebView editor.** Guard the include and the body; build `-render-test` explicitly. Memory `pattern_render_harness_breaks_on_webview_editor`.
3. **A bool or int restored as a String.** Use the `isVoid()` + parse guard. Memory `critical_valuetree_xml_roundtrip_loses_type`.
4. **The Standalone is stale after `build-and-install.sh`.** `ninja -C build O-simpleAmbisonics_Standalone` before any visual check. Memory `pattern_build_install_skips_standalone_stale_ui`.
5. **An occluded Standalone WKWebView never fires rAF,** so every canvas captures blank. Nudge the window partly visible before `screencapture -l`. Memory `pattern_standalone_hands_on_via_ax_scripting_and_window_capture`.
6. **Canvas text keeps the fallback face.** The page already runs `document.fonts.load("11px 'EB Garamond'")` → `readoutsDirty` (`v1-ui.html:2413-2415`). With dirty rendering, that promise must also mark **every canvas** dirty. Memory `pattern_canvas_text_needs_font_load_repaint`.
7. **`pointerdown` + `preventDefault` kills document `mousedown` dismissers.** The settings popover must dismiss on `pointerdown`; grep the page for `'mousedown'`. Memory `pattern_pointerdown_preventdefault_kills_document_mousedown`.
8. **The tooltip clamp gate depends on the viewport.** Probe at exactly 1180×780, and at 708×468 (the 0.6× floor). Memory `pattern_tooltip_clamp_gate_viewport_sensitive` and `pattern_fixed_tooltip_shrink_to_fit_edge`.
9. **A tooltip probe with the help layer off is vacuous.** Check `osamb.tipsEnabled` in `localStorage`. Memory `pattern_tooltip_probe_help_layer_off_by_default`.
10. **Neutral stub seed makes the gates vacuous.** Write `generic-overrides.json` first and read `seed=` on the banner. Memory `pattern_generic_ui_stub_seeds_neutral_ranges_gate_state_vacuous`.
11. **Fixture states C++ cannot produce.** Frames come from the harness; lesson states use the exact recipe values. Memory `pattern_ui_fixture_state_must_be_producible_by_cpp`.
12. **check-ui-labels is blind to width-pinned `[data-i18n]` elements** (e.g. the Orbit `Off | On` pill, station pills, chips). Measure their border-boxes per language. Memory `pattern_ui_labels_gate_blind_to_width_pinned_data_i18n`.
13. **Lesson rows are a French width budget.** 86 px slack today (§Q9). Memory `pattern_composed_readout_needs_setlabel_tokens_and_tour_row_budget`.
14. **Glossary before the localizers.** The new ambisonics terms go into the shared glossaries first. Memory `pattern_parallel_localizers_need_glossary_first`.
15. **An unregistered native never settles** and the control is silently dead. Diff `grep -o "getNativeFunction('[^']*')" index.html | sort -u` against the `withNativeFunction` names: the expected set is exactly 5. Memory `pattern_webview_native_fn_bridge_gap`.
16. **BinaryData strips hyphens** (`eb-garamond.css` → `ebgaramond_css`); keep new JS names hyphen-free (`shicons.js`). Memory `critical_binary_data_strips_hyphens`. There is only ONE binary-data target (`critical_dual_binary_data_namespace_collision`).
17. **Held peaks and empty W when the host idles the audio thread.** Use the `live` rule (§Q4.4). Analogue: memory `pattern_generation_reaper_freezes_when_host_idles_audio_thread`.
18. **A historic peak flashes on editor open.** Drain `read()` in the constructor (§Q1.3).
19. **Automation across ±180 replays through the front** in ramping hosts. Document and check hands-on (Open risk R2).
20. **A lambda range is invisible to the WebView slider.** Already avoided: `distance` and `orbitRate` use `setSkewForCentre`. Memory `critical_lambda_normalisable_range_invisible_to_webview_slider_frontend`.

---

## Proposed task split

### 3.1 — scaffold and bindings (one processor batch)

| Task | Files | Done when |
|---|---|---|
| 3.1-A processor batch | `Source/PluginProcessor.{h,cpp}` | Guarded `createEditor`; `std::atomic<int> uiLanguage`, `languageCode` / `languageIndex`; `std::atomic<float> editorScale`; state get / set (§Q7); `applyFactoryPreset` with the 8 recipes (§Q10, C1) |
| 3.1-B payload header | `Source/MeterFramePayload.h` | `quantise` / `hash` / `format` with `live`; no JUCE |
| 3.1-C harness | `tests/render-harness/main.cpp` | ST1–ST5 in the default run; `OSAMB_UI_FIXTURES` mode writes `tests/ui-fixtures/frames.json` (2 runs byte-identical); 241 + 5 PASS, both digests |
| 3.1-D editor + CMake + assets | `Source/PluginEditor.{h,cpp}`, `CMakeLists.txt`, `Source/ui/public/{index.html, js/i18n.js, js/juce/*.js, img/insects.png}`; remove `Source/ui/.gitkeep` | Template + §Q1 fixes; natives census = 5; resource branches for every SOURCE |
| 3.1-E stub seed | `tests/ui-stub/generic-overrides.json` | 12 real ranges (start / end / skew / interval / def) + choices; banner `seed=overrides` |
| 3.1-F regression protocol | — | §Q10 all green (once, after A–D); `verify_3_state.py` S1–S3 PASS |

### 3.2 — visuals

| Task | Files | Done when |
|---|---|---|
| 3.2-A icons | `scripts/generate_sh_icons.mjs`, `Source/ui/public/js/shicons.js`, `.planning/stages/3-gui/sh-icons-contact.html`; CMake + getResource | Self-checks 1–5 pass; page swaps `shIconSvg()` for SH_ICONS |
| 3.2-B page fixes | `Source/ui/public/index.html` | Compass heading / tilt / hold (§Q6), azRot pole hold, ±180 format, drag dead zone = rHead, order clamp, dirty rendering + puck sprite (§Q8), `live` handling (zero peaks always; parameter fallback if the user confirms) |
| 3.2-C frame probe | `tests/ui-probes/frame-probe.mjs` | A1–A11 PASS; negative controls FAIL as designed |
| 3.2-D build / install | — | `build-and-install.sh` + Standalone rebuild + harness build; Release editor open / close ×10 with no crash |
| **CHECKPOINT** | — | **STOP: user visual check** — meters, puck, compass, beam, decoder view, icons contact sheet (3D vs slice), resize 0.6× / 1.5× |

### 3.3 — tooltips, lessons, i18n, gates

| Task | Files | Done when |
|---|---|---|
| 3.3-A glossary | `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js` (shared, own path-scoped commit) | The ambisonics section from §Q9 added; both lints still pass suite-wide (`node scripts/i18n-fr-lint.js`, `node scripts/i18n-zh-lint.js`, no `--plugin`) |
| 3.3-B copy + widths | `Source/ui/public/js/i18n.js`, `index.html` CSS | §Q9 real findings fixed (subtitle, decLineBin, orbitOn, routeEncoder, distOpen spill, title-block + sph-tr pins); FuMa-letter wording; ±180 / automation note in the azimuth tip; pinned boxes measured per language |
| 3.3-C states + budget | `tests/i18n-states.json`, `tests/ui-probes/lesson-row-budget.mjs` | All 105 `[data-i18n]` visible in some state; fr row slack ≥ 8 px |
| 3.3-D lessons live | page `initLessons` (already wired) | Each lesson moves exactly its recipe parameters (stub log + Standalone); `outputLevel` never moves |
| 3.3-E gates | — | check-i18n, fr-lint, zh-lint, check-ui-labels (0), boot-all-uis `--strict-tips` (0 dead / 0 late), frame-probe re-run, tooltip clamp at 1180×780 and at the 0.6× size |
| 3.3-F hands-on | — | Standalone + Logic per checklist §9, plus R1 / R2 / R3 below |

---

## Validation Architecture

### Test framework

| Property | Value |
|---|---|
| C++ | Project render harness (console app, `OUARICON_BUILD_TESTS=ON`), plus pluginval 1.0.4 and auval |
| Installed binary | pedalboard via `uv run --no-project --python 3.12` |
| Page | Playwright through `scripts/serve-ui.js` + the generic stub |
| Quick run | `node plugins/O-simpleAmbisonics/tests/ui-probes/frame-probe.mjs` (about 20 s) |
| Full suite | §Q10 protocol + the 5 UI gates + frame-probe + lesson-row-budget |

### Requirements → gates

| Req | Behaviour | Gate | Command | Exists? |
|---|---|---|---|---|
| UI-01 | 4 / 9 / 16 rows; front → left swaps X → Y, W constant | frame-probe A1–A3 (pixels) + harness fixture assertions | `node …/frame-probe.mjs` | ❌ Wave 0 |
| UI-02 | Drag sets az / el with one gesture; yaw turns the compass, not the puck | frame-probe A4–A7, A9 | same | ❌ |
| UI-02 (host) | Gesture records in Logic; editor closed mid-drag ends the gesture | Logic hands-on + pluginval s10 editor tests | manual + pluginval | manual |
| UI-03 | Beam narrows 1 → 3 | frame-probe A8 | same | ❌ |
| UI-04 | Station lights on a parameter touch; channel count | frame-probe extra arm: set the stub yaw → `#st-rotation.on` | same | ❌ |
| UI-05 | Decoder view follows `decoderMix` | frame-probe A10 | same | ❌ |
| UI-06 | Every control and visual tipped, 0 dead / 0 late, clamped | `boot-all-uis --strict-tips`; tooltip clamp probe at 2 viewports | `node scripts/boot-all-uis.js --plugin O-simpleAmbisonics --strict-tips` | ✅ script |
| UI-07 | Single page, fits 13" at 1×, scales 0.6–1.5 | `readEditorSize` 1180×780; Logic / Standalone resize hands-on; `verify_3_state` + ST3 (persistence) | manual + harness | partly |
| UI-08 | 8 lessons apply exactly their recipe, never `outputLevel` | Harness probe (add as ST6: apply each id on a processor with `outputLevel` −12 dB; assert the targets and that −12 is unchanged) + Standalone | harness | ❌ |
| i18n | Keys, canon, lints, geometry | check-i18n, fr-lint, zh-lint, check-ui-labels | `node scripts/<gate>.js --plugin O-simpleAmbisonics` | ✅ scripts |
| Regression | Stage-2 verdicts unchanged | harness 241 + digests, auval, pluginval ×2, verify_2_* | §Q10 | ✅ |

### Sampling rate

- **Per task commit:** the build of all four targets plus the harness for any C++ task; frame-probe for any page task.
- **Per wave:** the full §Q10 protocol after 3.1-A–D; the 5 UI gates after 3.3-B.
- **Phase gate:** everything green, plus the hands-on, before `/plugin-verify`.

### Wave 0 gaps

- [ ] `tests/ui-stub/generic-overrides.json`: needed before any probe
- [ ] `tests/ui-fixtures/frames.json` (harness fixture mode)
- [ ] `tests/ui-probes/frame-probe.mjs`, `lesson-row-budget.mjs`
- [ ] `tests/pedalboard/verify_3_state.py`
- [ ] Harness ST1–ST6
- [ ] `tests/i18n-states.json` (3.3)

---

## Security Domain

Applicable ASVS: **V5 Input Validation** only. There is no auth, session, crypto or network.

| Surface | Control |
|---|---|
| `setUiLanguage(arg)` | `languageIndex()` maps anything else to 0; only an ASCII code is stored |
| `applyFactoryPreset(arg)` | `args[0].isString()` plus a fixed id table; unknown → `false`, no change |
| State XML (`editorScale`, `uiLanguage`) | Parse, check finite, clamp [0.6, 1.5]; unknown code → en |
| Page copy | textContent / setLabel only; check-i18n [9] forbids `innerHTML` and `<` in i18n.js; icons via createElementNS |
| Resource provider | Exact bare-path whitelist; `std::nullopt` otherwise |

---

## Environment Availability

| Dependency | Required by | Available | Version |
|---|---|---|---|
| JUCE | Build | ✓ | 8.0.15 at /Users/taylorbrook/JUCE |
| CMake / Ninja | Build | ✓ | 4.2.1 / 1.13.2 |
| Node | Gates, generator, probes | ✓ | v24.19.0 |
| Playwright (via serve-ui) | check-ui-labels, boot-all-uis, probes | ✓ | resolves |
| uv (+ pedalboard on Python 3.12) | verify_2_*, verify_3_state | ✓ | uv 0.9.18; pedalboard resolved and ran this session |
| pluginval | COMPAT gate | ✓ | 1.0.4 (/Applications/pluginval.app) |
| auval | AU validation | ✓ | system |
| Logic Pro | Hands-on | ✓ | /Applications/Logic Pro.app |
| Installed bundles | Measurers | ✓ | O-simpleAmbisonics-dev.{vst3,component} (Stage-2 build) |

Nothing is missing.

---

## Open Risks

- **R1 — host idles `processBlock`** (Logic, stopped, non-live track). The editor-side `live` flag handles the peaks. The parameter-driven fallback for puck and bars needs **user confirmation**, because it touches the locked "puck follows the frame". Check at the Logic hands-on. `[ASSUMED: Logic suspends]`
- **R2 — automation across ±180** ramps through the front in hosts that interpolate between nodes. Only documentation is possible under the frozen parameter set. Hands-on: record a rear-crossing drag in Logic and play it back. `[ASSUMED]`
- **R3 — WKWebView paint cost.** The 25 %-of-a-core figure is headless Chromium. Measure CPU in Activity Monitor with the editor open in Logic, before and after dirty rendering.
- **R4 — icon legibility at 0.6×.** 18 px × 0.6 = 10.8 CSS px. The checkpoint picks 3D or slice from the contact sheet.
- **R5 — reviewed French may lengthen** the lesson row or the pills. The budget script and the width pins catch it; re-run check-ui-labels after review.
- **R6 — shared-file edits** (both glossaries) while other sessions are active: path-scoped commit, re-check staging immediately before it (CLAUDE.md).
- **R7 — harness denominator** changes from 241 to 246+. The plan and SUMMARY must state it so the Stage-2 verdict ("241/241") is not misread as a regression.
- **R8 — O-simpleFM's working tree is in flight in another session.** Do not copy from it; the cited references are committed.

---

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | Some hosts call `getStateInformation` off the message thread | Q1, Q7 | Low: the atomics are harmless anyway |
| A2 | Logic suspends `processBlock` for stopped, non-live tracks | Q4.4, R1 | The fallback becomes unnecessary (no harm) |
| A3 | Logic ramps automation between +179 and −179 through 0 | Q3, R2 | The tooltip note may be over-cautious |
| A4 | WKWebView canvas paint cost is similar to headless Chromium | Q8, R3 | Dirty rendering is still correct, just less urgent |
| A5 | The **new** fr / zh glossary renderings (lacet, roulis, ordre, canal, 声道, 阶数, 双耳, 声场, 波束, …) | Q9 | A reviewer edits the glossary entries; the lint then enforces them |
| A6 | `std::to_chars` is locale-independent (cppreference) | Q4.1 | None in practice; ints only |
| A7 | The 3D icon camera angles (about 55° down, 20° turn) read well at 18 px | Q2 | Tune at the checkpoint, or fall back to the slice |

---

## Sources

### Primary (HIGH; read or run this session)

- JUCE 8.0.15:
  - `juce_gui_extra/native/javascript/index.js:135-270` (SliderState)
  - `juce_gui_extra/misc/juce_WebBrowserComponent.cpp:414-429, 607-611` (emitEvent, visibility gate)
  - `juce_gui_extra/misc/juce_WebControlRelays.cpp:98-131`
  - `juce_audio_processors/utilities/juce_ParameterAttachments.{h:273-301, cpp:59-118, 279-333}`
  - `juce_core/text/juce_String.cpp:265-269, 2287-2312`
  - `extras/Build/CMake/JUCEUtils.cmake:525-531, 583-600`
- Plugin:
  - `Source/PluginProcessor.{h,cpp}`, `Source/dsp/{MeterFrame.h, TripleBuffer.h, ShEncoder.h, ShRotator.h, SpatialEngine.h:355-397}`
  - `CMakeLists.txt`
  - `tests/render-harness/{CMakeLists.txt, main.cpp}`
  - `tests/pedalboard/verify_2_{1,2,3}.py`
- Mockup: `.planning/mockups/{v1-ui.yaml, v1-ui.html, v1-i18n.js, v1-PluginEditor-TEMPLATE.{h,cpp}, v1-CMakeLists-SNIPPET.txt, v1-integration-checklist.md}`, `.planning/parameter-spec.md`
- Siblings:
  - O-simpleWavetable `Source/PluginEditor.{h,cpp}`, `Source/PluginProcessor.cpp:1282-1436`, `tests/{state-check, ui-probes, ui-stub}`
  - O-simpleAdditive `Source/PluginEditor.cpp:215-315`, `Source/PluginProcessor.{h:155-200, cpp:383-510}`, `js/app.js:477-500`
  - O-simpleFM HEAD vs working tree (`git show` / `git diff`)
- Gates: `scripts/{check-i18n.js, i18n-fr-lint.js, i18n-zh-lint.js, check-ui-labels.js, boot-all-uis.js, serve-ui.js, ui-stub/*}`, all **run** against a mockup fixture tree.
- Measurements:
  - Lesson-row widths (Playwright)
  - Render CPU (CDP metrics)
  - VST3 state decode (pedalboard)
- Project memory files cited inline.

### Secondary (MEDIUM)

- [Ambisonic data exchange formats — Wikipedia](https://en.wikipedia.org/wiki/Ambisonic_data_exchange_formats): ACN = l² + l + m; FuMa letters in ACN order.
- [Ambisonics formats explained — ambisonics.ch](https://ambisonics.ch/learn/ambisonics-formats/): FuMa vs ACN / AmbiX naming context.
- cppreference `std::to_chars` (locale independence).

### Tertiary (LOW)

- Host behaviour assumptions A1–A3; new glossary renderings A5.

## Metadata

**Confidence breakdown:**
- Bindings, payload and state: **HIGH**, read from the JUCE and plugin source and probed.
- Gates and i18n baseline: **HIGH**, run this session.
- Icons look, host-idle and automation-wrap behaviour: **MEDIUM / LOW**, needing the checkpoint and the Logic hands-on.

**Research date:** 2026-10-13
**Valid until:** 2026-11-12. The JUCE pin is stable. The sibling working trees, the O-simpleFM WIP and the shared glossaries can move sooner.
