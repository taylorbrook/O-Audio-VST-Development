# O-GranulatorInstrument — Stage 3 (GUI) — Phase 3.1 — RESEARCH

**Researched:** 2026-11-10
**Scope:** Phase 3.1 only (D37–D42): the v1 page in the WebView, 261 parameters bound, 34 native functions registered, DevLoader removed, parent 1.2.0 value entry ported, state sweep landed. Lanes display, drop UI, Record-in UI (3.2) and MOD / voices / meters (3.3) are out of scope except where 3.1 must leave a well-formed placeholder.
**Tree:** `main` at `734b7ef4`. Parent O-Granulator at its current head (1.2.0, `e77d29c8` resolves).
**Confidence:** HIGH on code facts (read this session, `path:line`). MEDIUM on the designs in §3–§7 (designed here, not built). Nothing was built or run in this phase; the page was not opened in a browser. Each claim is **VERIFIED** (read) or **INFERRED** (reasoning).
**Abbreviations:** M = `.planning/mockups/v1-ui.html`; T = `.planning/mockups/v1-PluginEditor.cpp`; P = `plugins/O-Granulator`; S3P = `P/.planning/stages/3-gui`.

**Inputs read:** `CONTEXT-3.1.md`; `v1-integration-checklist.md`; `v1-PluginEditor.{h,cpp}`; `v1-ui.html` (controller, natives, telemetry consumers, knob machinery); `v1-CMakeLists.txt`; `Source/{PluginProcessor.h,PluginProcessor.cpp,PluginEditor.cpp}`; `Source/dsp/{GiTelemetry.h,GiRecorder.h,SlotLoader.h,SlotLoader.cpp,Engine.h,Engine.cpp (telemetry),GiConstants.h}`; `CMakeLists.txt`; parent `Source/{PluginProcessor.*,PluginEditor.*,dsp/Engine.h}`, `S3P/{RESEARCH,PLAN,SUMMARY,VERIFICATION}-3.1.md`, `tests/{ui_frontend_check.js,ui_binding_probe.js,i18n-states.json}`, commit `e77d29c8`; `scripts/{check-i18n,i18n-fr-lint,i18n-zh-lint,check-ui-labels,boot-all-uis,serve-ui}.js`, `scripts/param-dump/ParamDump.cmake`; `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`, `modules/core/webview-drop-streaming/cpp/WebViewDropStreaming.h`, `modules/registry.yaml`.

---

## Summary (what the plan must absorb)

1. **Do not land `v1-CMakeLists.txt`.** It predates Stage 2 and would delete `dsp/SlotLoader.cpp`, `dsp/Engine.cpp`, the `Source/dsp` include dirs, the FactorySources target, `OGI_GATE_SC_DEFAULT_OFF`, and the render-harness console with `OGI_TEST_HOOKS` (`CMakeLists.txt:65-76, 176-183, 199-203, 228-235` vs template `:63-69, 158-164, 179-184`). The real file needs exactly three edits: uncomment the UIResources block and its link (`:145-164`), remove `Source/PluginEditorDevLoader.cpp` (`:67-69`), remove the `OGI_DEV_LOADER` option (`:205-212`). **VERIFIED.** [correction to checklist §3]
2. **The editor template does not compile against the real processor.** Eleven accessors it calls do not exist, slot indices differ (template 1…4, processor 0…3), `SlotInfo` has different fields, and the drop `onCommitFolder` lambda has 6 parameters where the module type has 8 (`T:762` vs `WebViewDropStreaming.h:132-139`). Full map in §2. **VERIFIED.**
3. **No page consumer throws on empties, but five telemetry values must have exact shapes** or the page shows wrong states: `rec.bus` must be 0 or 1 (never a channel count), `lanes[i].rec` and `.fb` must be `null` (not 0), `lo`/`hi` must be omitted or real, `counts` must be four numbers. §3. **VERIFIED** (code) / **INFERRED** (visual result).
4. **The template's torn-read comment is wrong.** On a failed read it returns only `{scaleRev, slotRev}` and says the page keeps its last frame (`T:599-602`); the page replaces `telemetry` wholesale (`M:3668`). With `TripleBuffer` there is no torn read anyway (the reader always gets a complete frame, `GiTelemetry.h:37-39`), so 3.1 always emits the full object. **VERIFIED.**
5. **A pristine overview is already available.** `SlotLoader::getOverview (slot)` returns (min, max) pairs per 512 tape frames (`SlotLoader.h:189`, `SlotLoader.cpp:441-453, 716-722`). Folding it to 656 peaks is about 20 lines in the editor. Without it a loaded lane shows name, badges and loop dimming but no waveform (`M:4058`). **Recommend the pristine read lands in 3.1**; the live (record-head) overlay and `dirty[]` stay 3.2. **[decision for plan]**
6. **Presets need a decision on slots.** The spec says presets store `<Slots>` including embedded blobs (`parameter-spec.md:355`). The loader already has `getSavedSlots()` / `restoreSlots()` (`SlotLoader.h:206-209`), so wiring is direct, but a preset can then carry up to 4 × 16 MB of base64 in its JSON. §5. **[decision for plan]**
7. **`rec_arm` must be excluded from presets.** Preset apply resets every parameter to default and then sets the saved value (`OuariconPresetManager.h:403-432`); a preset saved with `rec_arm` On produces an Off→On edge on load, which arms a capture (D34). Use `setExcludedParameters ({ "output_gain", "rec_arm" })`. **VERIFIED** (mechanism) / **INFERRED** (not run).
8. **The stale `<CustomState>` trap does not apply** as long as the session never calls the manager's `getStateAsXml` / `setStateFromXml` (the only writers of that child, `OuariconPresetManager.h:661, 687`). The parent uses the manager for preset files only (`P/Source/PluginProcessor.h:29, 143-146`); do the same. The two-reopen test stays as the proof. **VERIFIED.**
9. **Value entry ports almost verbatim.** The parent diff is one file, +162 lines of page code, no C++, no i18n keys, no gate (`e77d29c8`). The knob markup, `FORMAT` table and 14 formatters are byte-identical in M. New code is needed for `fCount` (`polyphony`), `fBend` (`pitch_bend_range`), the two-branch `updateChoiceKnobVisual`, and the localized Scan-mode knobs. §6. **VERIFIED.**
10. **Value entry changes what `check-ui-labels` measures.** Dropping `pointer-events: none` from `.knob-label` makes 161 labels non-inert, so they join the collision set (`scripts/check-ui-labels.js:443, 1088`). The parent never ran a state with a field open. Run the gate on the unmodified page first, then after the port, and add an open-field state (D42). **VERIFIED** / **INFERRED** (outcome).
11. **The parent's binding proof is reusable.** Three layers: a static closure check, a headless page probe (220/220 both directions under a seeded stub), and a human pass. Neither script is parameterised by plugin; both need a copy adapted to 261 and the child's binders. The probe is vacuous without a C++-generated overrides file (`--dump-ui-overrides`), which the child's harness lacks. §7. **VERIFIED.**
12. **`createEditor` must switch in the same task as the editor lands**, because the consoles depend on the plugin target (parent deviation, `S3P/SUMMARY-3.1.md:35-46`). Consoles drop any source matching `PluginEditor` and build with `JUCE_WEB_BROWSER=0` (`ParamDump.cmake:157, 200`), so `PluginProcessor.h` must stay free of WebView types; `OuariconPresetManager.h` is safe there (the parent does it, `P/Source/PluginProcessor.h:29`). **VERIFIED.**

---

## 1. Current state — VERIFIED

| Piece | Today | 3.1 |
|---|---|---|
| `Source/PluginEditor.cpp` | 12-line placeholder; no `PluginEditor.h` | the adapted template |
| `createEditor()` | DevLoader under `JUCE_WEB_BROWSER && OGI_DEV_LOADER`, else generic (`PluginProcessor.cpp:357-365`) | WebView editor under `#if JUCE_WEB_BROWSER`, generic otherwise |
| DevLoader | `PluginEditorDevLoader.{h,cpp}`, include at `PluginProcessor.cpp:27-33`, CMake `:67-69, 205-212` | all removed (D38) |
| `Source/ui/public/` | only the gitignored `modules/` copies (`.gitignore:27`) | `index.html`, `js/i18n.js`, `js/juce/{index.js,check_native_interop.js}` |
| UI binary data | commented out (`CMakeLists.txt:145-164`) | active, 10 sources |
| Preset manager | none in the processor | member + callbacks (§5) |
| `tests/` | `fork-drift-check.sh`, `golden`, `parity`, `render-harness` | + `i18n-states.json`, `ui-stub/generic-overrides.json`, two UI scripts |
| Engine access from the editor | `engine` is private; `getEngine()` exists only under `OGI_TEST_HOOKS` (`PluginProcessor.h:219-232, 292`) | new message-thread accessors on the processor (§2) |

JUCE frontend library source: `git show HEAD:plugins/O-Granulator/Source/ui/public/js/juce/index.js` and `check_native_interop.js` are tracked (verified with `git ls-files`).

## 2. Template → processor map — VERIFIED

Slot convention: the page and template use 1…4; `SlotLoader` and the processor use 0…3. Convert once, in the editor's `slotArg`.

| Template call | Exists? | 3.1 action | Audio thread touched? |
|---|---|---|---|
| `getPresetManager()` | no | add member + accessor (§5) | no |
| `getScaleStateRevision()` | no | add `std::atomic<uint32_t>`; bump in `setCustomScale`, `setDegreeMask`, `resetNonParameterState` / `applyNonParameterState` (parent: `P/Source/PluginProcessor.h:128`) | no (a relaxed counter the engine never reads) |
| `getSlotStateRevision()` | no | §4 | no |
| `getSlotInfo (1…4)` returning `scheme, factoryId, path, seconds, sourceSeconds, encodedBytes, dirty` | exists with other fields (`SlotLoader.h:86-113`) | derive in the editor (table below) | no |
| `getSlotOverview (…, 656, rev)` | no; `loader->getOverview (slot)` exists | new processor accessor that folds the pristine pairs to 656 peaks; `dirty[]` all 0 | no |
| `loadSlotFromFile (slot, file, error)` | `requestSlotLoad (slot, file)` (void) | call it; answer `{ ok: true }` (errors surface through `SlotInfo::lastError` on the next state pull) | no |
| `relinkSlot (slot, file, error)` | `requestRelink` (void) | same | no |
| `resetSlot (slot)` / `resetAllSlots()` | `requestReset (slot)`, −1 = all (`PluginProcessor.h:162-166`) | call it | no |
| `setSlotEmbed` returning an error string | `setSlotEmbed (slot, on)` (void); `SlotInfo::tooLarge`, factory / empty are silent returns (`SlotLoader.cpp:1406-1427`) | editor derives `"factory"` / `"empty"` from `SlotInfo::source` before the call; `"tooLarge"` from `tooLarge` (known only after the encode, so it reaches the page on a later state pull) | no |
| `clearSlot (slot)` | no processor entry. The loader clears through `handoff.publishClear` only inside `restoreSlots` (`SlotLoader.cpp:1255-1274`) | 3.1: refuse with `{ ok: false, error: "unavailable" }` (D37). 3.2 adds `SlotLoader::requestClear` on the same path. | no |
| `saveRecordingTo` | no | 3.1: refuse the same way; the `scheme != "rec"` guard already answers `notRecording` (`T:1055`) | no |
| `readTelemetry (Telemetry&)` | no; feeds exist separately | build the var directly in the editor from accessors (§3) | reads only |
| `getWindowCurve (stream, out, n)` | no; parent's is 10 lines over the bank (`P/Source/dsp/Engine.h:155-165`, `P/Source/PluginProcessor.cpp:790-800`) | port it. The child has the same `WindowMorphBank::stepFor / table / read` (`forked/WindowMorphBank.h:74-121`) and `getMorphBank()` (`Engine.h:584`). Real curve, not a flat stub. | no (bank is built once in the constructor) |

Because `engine` is private, add narrow message-thread accessors on the processor rather than exposing the engine: `takeOutPeak()`, `takeScPeak()`, `readUiFrame()`, `getDestTelemetry / getSourceTelemetry / getLfoPhaseTelemetry`, `getHeadPos (slot)`, `getOverviewEpoch (slot)`, `getRecordProgress()`. All wrap existing lock-free getters (`Engine.h:490-518`, `GiTelemetry.h:150-151`, `GiRecorder.h:398-403`). **[decision for plan: accessor list vs one aggregate struct]**

`SlotInfo` → page fields (the page reads only `scheme, name, missing, trimmed, embed, encodedBytes, dirty, seconds`, `M:3546-3577, 4315-4344`):

| Page field | From |
|---|---|
| `scheme` | prefix of `source`: `factory:` / `file:` / `rec:` / empty |
| `name`, `trimmed`, `missing`, `embed` | same-named fields |
| `seconds` | `frames / tapeRate` when `loaded` |
| `encodedBytes` | no source today. Send 0 unless `tooLarge`, then 16 MiB + 1, so the page's 16 MB comparison (`M:3568`) fires. An exact figure is 3.2. |
| `dirty` | `false` in 3.1 (needs the record-head epoch, 3.2) |

Other template defects to fix while adapting:
- `onCommitFolder` arity (Summary 2).
- `getParameterDefaults` iterates slider and combo ids, which is what the page needs: Float in engineering units, Choice as index (`T:796-799`; page `M:2043-2048, 2307-2310`). Toggles are not reset through it. This answers the CONTEXT question: no change needed.
- Class names already match (`GranulatorInstrumentEditor` / `GranulatorInstrumentProcessor`).

## 3. `getTelemetry` in 3.1 — VERIFIED (page) / INFERRED (visual)

The page polls every 33 ms (`M:3656`), replaces its `telemetry` object on each answer (`M:3668`), and reads these keys. `frozen` and `modRouted` are never read (the parent dropped `frozen` for the same reason).

Emit this object in 3.1:

```
{ scaleRev, slotRev,
  lanes:  [ { ovRev, fb: null, rec: null } × 4 ],     // no lo / hi keys
  voices: [], newest: -1, menv: [], grains: [],
  counts: [0,0,0,0], limit: 0, out: [0,0],
  rec:    { bus: 0|1, signal: false, run: 0, slot: 0, pos: 0, len: 0, capped: false, peak: [0,0] },
  sources: [], modNorm: {} }                          // lfoPh omitted
```

| Key | 3.1 value | Why |
|---|---|---|
| `scaleRev`, `slotRev` | real | drive the scale / slot re-pull (`M:3681, 3688`) |
| `lanes[i].ovRev` | real: a per-slot value that moves when the tape changes (`SlotInfo::generation` folded with `loaded`) | the overview is fetched only when `ovRev` moves, and an empty answer marks the revision seen (`M:3693, 3713`). If `ovRev` does not move after a load, the waveform never appears. |
| `lanes[i].fb`, `.rec` | `null` | any number draws a head / marks the lane recording (`M:4044, 4082`) |
| `lanes[i].lo`, `.hi` | omitted | the page falls back to the loop handles (`M:4054-4055`); 0/0 dims the whole lane |
| `counts` | four zeros | `[]` prints "undefined" in the caption (`M:4345`) |
| `limit` | 0 | the page then parses the `grain_limit` choice (`M:4327`) |
| `rec.bus` | `getSidechainChannelCount() > 0 ? 1 : 0` | `renderRec` tests `=== 1`, the meter tests `=== 0` (`M:3726, 4157`) |
| `rec.run` | 0 (placeholder; mapping below is 3.2) | |

Expected look (not run): lanes show name, badges, waveform (with Summary 5), loop dimming; VOICE shows "idle" and blank cells; MOD shows LFO curves without dots; Record-in reads "no signal" with a bus, "No sidechain" without.

Real peaks (`out`, `rec.peak`) and the frame are consuming / per-block feeds. Leaving them unread in 3.1 costs nothing: `pushMax` just holds the maximum. Do not half-read them.

**`rec.run` mapping (decided in 3.2; recorded here so 3.1's placeholder is compatible).** Page order of checks: `bus !== 1` → run 2 → 3 → 1 → 5 → `!signal` → 4 → 0 (`M:3722-3765`). Engine: `RecState` Idle 0 / Armed 1 / Recording 2 / Committing 3, plus `RecReason` (`GiRecorder.h:70-74`), packed `state | reason << 8 | heldArm << 16`. States 1–3 map one to one. Idle + `Committed` → 4 (done); Idle + `NoSignal` / `TooShort` → 5 (discarded); Idle + anything else → 0. `SidechainUnavailable` is already covered by `bus`. Open for 3.2: how long 4 / 5 are held (the reason persists until the next arm), and raw `scInPeak` vs post-gain `takeScPeak` for the meter; the page comment says post-gain (`T:689`).

**Failure behaviour of natives (page side).** No call has a timeout. A `getTelemetry` that never resolves retries at about 1 Hz (`M:3663`); a hung `getSlotOverview` leaves that lane's `ovInFlight` stuck forever (`M:3706-3717`). So every registered function must call `complete` on every path, including refusals. One page-side throw exists: `parseScaleText` answering `{}` throws at `M:3377`; the template always answers `{error…}` or `{cents…}`, so it is not reachable.

## 4. Slot revision — design, INFERRED

`SlotInfo` is mutated at about 15 sites under the loader lock (`SlotLoader.cpp:484-1422`), including worker completions the message thread does not see. A counter bumped at each site is one missed site away from a stale lane.

**Recommend a fingerprint:** `getSlotStateRevision()` (message thread, called from `getTelemetry` at 30 Hz) copies the four `SlotInfo`, hashes `generation, loaded, loading, missing, embed, hasBlob, tooLarge, trimmed, source, name, lastError`, and increments a member counter when the hash differs from the last one. It covers every mutation by construction. Cost: four short locked copies per poll. No audio-thread code changes. Alternative: an atomic in `SlotLoader` bumped through one `touchLocked()` helper. **[decision for plan]**

Either way `SlotLoader.*` and `PluginProcessor.*` are not forked files, so fork drift stays 0/13.

## 5. Presets — VERIFIED (module, parent) / INFERRED (child design)

Port the parent's wiring (`P/Source/PluginProcessor.cpp:185-188, 620-660`):
- Member `OuariconPresetManager presetManager { parameters, "O-GranulatorInstrument" }`, declared after `parameters`. No directory I/O in the constructor (the manager creates folders lazily; auval).
- `setValidatedApply (true)`; `setExcludedParameters ({ "output_gain", "rec_arm" })` (Summary 7). **[decision for plan: also exclude `freeze`? Not required by any rule; default keep it in presets.]**
- `presetCustomState()` = `{ seed (int64 string), custom { name, text }, masks [ { stream, collection, bits } ] }`, and `applyPresetCustomState` → the existing `applyNonParameterState` (`PluginProcessor.cpp:719-760`), which already resets to defaults first. Then bump the scale revision.
- No factory presets in 3.1 (FUNC-08, Stage 4).
- preset-manager is at 1.1.1 (`modules/registry.yaml:131`), which has the validated apply and the reset-to-defaults pass.

**Slots in presets** (`parameter-spec.md:355`): add `slots` to the custom state from `loader->getSavedSlots()` and apply with `loader->restoreSlots()`. Three options:
1. **Sources without blobs (recommended for 3.1):** emit `source, name, fileSize, contentHash, embed, trimmed`, never `embedded`. Presets stay small; a `rec:` slot resolves by path or by the Recordings search (rungs 1 and 3). A preset moved to another machine loses recordings.
2. Full spec: include blobs. Self-contained presets, up to about 64 MB of JSON.
3. Defer slots in presets to 3.2.
A preset with no `slots` key must leave the current slots alone or clear them; the spec's "reset to defaults first" reads as clear. **[decision for the user at plan time]**

## 6. Value entry port (D41) — VERIFIED

Parent `e77d29c8`: `index.html` only. Hunks and their port class:

| Hunk (parent line) | Port |
|---|---|
| CSS `.knob-label`: drop `pointer-events: none`, add `cursor: text`, colour transition (P:224) | verbatim at `M:225` |
| CSS hover / `.knob-cell.editing .knob-label` (P:226); `.knob-value-input` (P:227) | verbatim |
| `updateKnobVisual` guard `dataset.editing !== '1'` (P:1581) | verbatim at `M:2063` |
| `updateChoiceKnobVisual` guard (P:1746) | new: M has two write branches (`M:2227` localized face, `M:2230` plain); guard both |
| `numOf`, `kMul`, `PARSERS` (P:1821-1862) | verbatim for 14 formatters; add `fCount`, `fBend` |
| `parseDivision` (P:1866) | verbatim for the 12 division knobs and `rec_length_sync`; new for `sN_scan_mode` |
| `attachValueEntry`, `bindLabelEntry` (P:1882, 1941) | verbatim (same ids, `sliderState`, `comboState`, `FORMAT`, `scaledToNorm`) |
| init calls (P:3834-3835) | after `M:4815` |

Counts in M: `KNOB_IDS` 144 (`M:1377-1521`), `CHOICE_KNOB_IDS` 17 (`M:1572-1590`), 161 `.knob-value` nodes. Root fields, loop handles and depth bars are separate lists (`FIELD_IDS`, `HANDLE_IDS`, `FADER_IDS`), so the exclusions hold without extra code. No `ct` or note-name parser is needed. Record Length Free is `fSec` (handled); Record Length Tempo is a division list `1 bar … 32 bars` that `parseDivision` handles.

New instrument code:
- `fCount` (`polyphony`, `8`) and `fBend` (`pitch_bend_range`, `±2 st`): bare number / absolute value. Both ranges have interval 1 (`GiParamIds.h:223, 226`); round in the parser rather than rely on the frontend to snap.
- `sN_scan_mode`: the readout is a localized face set through `setLabel`, which puts `data-i18n` on the readout node (`M:1948-1952, 4731-4736`). The choices are English (`Free-run`, `Retrigger`, `Ping-pong`); typed French or Chinese is rejected by `parseDivision`. Match against the three localized faces of the current language plus the English choices. **[decision for plan: or exclude the 4 scan-mode knobs from entry; D41 says the 17 are covered]**
- Language switch while a field is open: `applyLabel` rewrites `textContent` of every `[data-i18n]` node (`M:4712-4751`), which would replace the input inside a scan-mode readout. Close any open field before applying a language.
- Sync twins: Free and Tempo cells share a position and toggle `hidden` (`M:2784-2789`); close an open field when its cell is hidden.
- The capture-phase `pointerdown` blur is required: choice knobs and alt-drag call `preventDefault` (`M:2299, 2424`).

Write path (unchanged from the parent): clamp to `start` / `end`, then `sliderDragStarted` → `setNormalisedValue (scaledToNorm)` → `sliderDragEnded` (one gesture); a choice writes `setChoiceIndex` only when the index differs; unchanged text writes nothing.

Parent evidence for the feature: the commit message records a "headless entry probe 21/21"; no probe file, state or gate was committed, and the parent's DAW check is still pending. The child needs its own probe rows (§7) and at least one `i18n-states.json` state with a field open in the narrowest cell (54 px header cells, `M:232`).

## 7. Verification method — VERIFIED (parent) / INFERRED (child adaptation)

Parent layers (`S3P/RESEARCH-3.1.md:168-187`, `S3P/PLAN-3.1.md:76-98`):

1. **Overrides file.** The generic stub seeds every combo with `['1','2','3','4']` and answers `null` to `getScaleState` / `getTelemetry`, so page probes are vacuous without real data. The parent's harness writes `tests/ui-stub/generic-overrides.json` with `--dump-ui-overrides` (`P/tests/render-harness/main.cpp:765, 905`), checked with `git diff --exit-code`. The child harness has no such mode; add it (choices from the layout, scale state, a 3.1-shaped telemetry object, slot state). Also needed so `check-ui-labels` and `boot-all-uis` see real choice strings.
2. **Static closure**, `P/tests/ui_frontend_check.js` (457 lines): fresh param-dump == editor `k*Ids` == page id tables by ID and type; native-function parity; member order; resource closure. Copy and adapt to 176 / 70 / 15 and 34.
3. **Binding probe**, `P/tests/ui_binding_probe.js` (323 lines): headless Chromium through `scripts/serve-ui.js`; host→UI (write stub state, fire `valueChangedEvent`, assert the DOM moved) and UI→host (real events, assert the stub value and paired gesture calls), 220/220 each way; refuses to run without the overrides banner. Copy and adapt to 261 and the child's extra binders (`bindHandle`, `bindField`, `bindFader`, the `rec_target` radio). Add value-entry rows: Enter applies, Esc cancels, outside click applies, unchanged text writes nothing, one gesture per value, clamp, `−inf`, the two new parsers, a division, a scan mode in fr.
4. **Human**, HANDS-ON-3.1: one automation lane per relay type in a DAW, preset recall, two reopens, the gear position (D40), value entry by hand.

Playwright cannot attach to WKWebView, so the Standalone is covered by a `screencapture` smoke shot plus the human pass, not by automation.

UI gates (repo root; exit 77 = Playwright missing, never a pass):

| Gate | Inputs it parses |
|---|---|
| `node scripts/check-i18n.js --plugin O-GranulatorInstrument` | `index.html`, `js/i18n.js`, comment-stripped `CMakeLists.txt`, `Source/PluginEditor.cpp`. Rule [8] needs `js/i18n.js` in an active `juce_add_binary_data` block and both `"/js/i18n.js"` and `i18n_js` in the editor (`:1170-1190`). |
| `node scripts/i18n-fr-lint.js --plugin …`, `i18n-zh-lint.js` | `Source/ui/public/js/i18n.js` |
| `node scripts/check-ui-labels.js --plugin …` | last `setSize (W, H)` in `PluginEditor.cpp`, `tests/i18n-states.json`, stub seed from `.planning/params.tsv` + overrides |
| `node scripts/boot-all-uis.js --plugin … --strict-tips` | same size reader |

`serve-ui.js` maps URLs to symbols by finding a `url == "/x"` ladder with `BinaryData::sym` within 400 characters (`:235-266`). Keep the template's explicit `if (url == …)` ladder shape; a lookup table would break the gates.

Order that worked for the parent: overrides → closure → probe → check-i18n → fr → zh → check-ui-labels → boot-all-uis. Take a baseline of the UI gates on the copied, unmodified page before the value-entry port, so any new failure is attributable.

Stage 2 regression set after the editor swap (CONTEXT constraint): harness 357/0, parity 168/0 + GV 2/0, renders-2.4 + legacy byte-identical, param-dump identical, drift 0/13, X2 0, auval, pluginval 10 VST3 + AU with sidechain on and off. New processor members (preset manager, revision counters) are compiled into the consoles too, so param-dump identity is a real check, not a formality.

## 8. Pitfalls specific to 3.1

- **CMake comments.** The commented UIResources block contains no plugin-target call, but `check-i18n` and `serve-ui` strip `#` comments before parsing, so the block must be genuinely uncommented, not duplicated.
- **Module JS.** `ouaricon_add_module` copies module JS into `Source/ui/public/modules/` at configure time (`OuariconModules.cmake:104-115`); the shipped copies come straight from `modules/` through the binary-data SOURCES. Never commit the copies; never `git add -A`.
- **Case and symbols.** Binary-data symbols strip hyphens: `presetmanager_js`, `webviewdropstreaming_js`, `ebgaramond_css`, `EBGaramondRegular_woff2` (already correct in the template).
- **Standalone rebuild.** `build-and-install.sh` installs VST3 + AU; rebuild the Standalone target explicitly before any Standalone check (carried from Stage 2).
- **Preset list at boot.** `preset-manager.js` waits up to 5 s for a backend in a plain browser (`preset-manager.js:149-166`); the probe's stub must answer the ten preset functions or boot timing shifts.
- **Editor lifetime.** Every `launchAsync` callback hoists a `SafePointer` and bare-returns when the editor is gone (template already does). Close / reopen ×10 belongs in the smoke task.
- **Language persistence** is already in the processor (`PluginProcessor.cpp:540, 617-623`); the editor only reads and stores the atomic.
- **Unverified here:** whether the JUCE frontend snaps `setNormalisedValue` to the interval for the two integer knobs; that `registry.yaml`'s uncommitted edits (foreign, dirty at session start) do not change the module versions quoted above.

## 9. Answers to the CONTEXT open questions

| Question | Answer |
|---|---|
| `getTelemetry` shape and empties | §3. No throw; five values need exact shapes. |
| `rec.run` mapping | §3, recorded for 3.2; 3.1 sends run 0 and a real `bus`. |
| New processor surface in 3.1 | §2: preset manager, scale revision, slot revision, window curve (real), pristine overview, telemetry accessors. None touches the audio thread. |
| `loadSlot` without an overview | Readable but no waveform. Recommend the pristine overview in 3.1 (Summary 5). |
| Parent 1.2.0 diff | §6. One file; new parsers for `fCount`, `fBend`, scan mode; no parent gate to inherit. |
| `getParameterDefaults` vs choice knobs | Works as templated: index for choices, engineering units for floats. |
| 261-param two-way method | §7: overrides dump + closure check + binding probe + hands-on. |
| CMake divergence | Summary 1. Edit the real file in three places; do not land the template. |

## 10. Decisions for the plan

1. Presets and slots: sources without blobs (recommended), full blobs, or defer to 3.2. **User decision.**
2. Pristine overview in 3.1 (recommended) or leave lanes without a waveform until 3.2.
3. Slot revision: fingerprint (recommended) or loader counter.
4. Scan-mode value entry: localized match (recommended) or exclude those 4 knobs.
5. Exclude `rec_arm` from presets (recommended; treat as required). `freeze` stays in.
6. Telemetry accessors: a short list of processor getters (recommended) or one aggregate struct as the template sketched.
