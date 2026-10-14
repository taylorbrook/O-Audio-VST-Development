# Stage 3 — GUI (single cycle: 3.1 bindings + 3.2 visuals → user visual checkpoint → 3.3 tooltips / lessons / i18n) — PLAN

**Plugin:** O-simpleAmbisonics · **Stage:** 3 GUI · **Cycle:** single (no suffix — one RESEARCH / PLAN / SUMMARY / VERIFICATION) · **Date:** 2026-10-13
**Inputs:** `CONTEXT.md` (Approach Decisions + Constraints = locked), `RESEARCH.md` (§Q1–Q11, risks R1–R8), `.planning/parameter-spec.md` (12 frozen params), `.planning/mockups/v1-*` (finalized v1: `v1-ui.html`, `v1-i18n.js`, `v1-PluginEditor-TEMPLATE.{h,cpp}`, `v1-CMakeLists-SNIPPET.txt`, `v1-integration-checklist.md`, `v1-ui.yaml`, `v1-ui-test.html`), `REQUIREMENTS.md` UI-01…08, `ROADMAP.md` §Stage 3, `BRIEF.md` §UI Concept, `research/ARCHITECTURE.md` §Visualization Tap / §Polar plot / §Thread Boundaries, `stages/2-dsp/VERIFICATION.md` (carried items), the user's decision of this session (U1)
**Executor:** gui-agent, with Bash (build, harness, installer, Playwright, uv must run). Long commands in the background (600 s watchdog).
**Branch:** `main` @ `a10d7860` (RESEARCH committed). Trunk-based; other sessions are active in this checkout — commit only the paths listed per part.

## Goal

Replace the Generic editor with the finalized v1 WebView page: 12 parameters bound both ways through relays → WebView → attachments; one hash-gated `meterFrame` event at 30 Hz from a JUCE-free payload header shared with the harness; the B-format channel wall (UI-01), the two-canvas sphere view with drags, compass, polar beam (UI-02 / UI-03), the signal-path strip (UI-04) and the decoder view (UI-05); then, after the user's visual checkpoint, tooltips (UI-06), the live lesson tour (UI-08) and en / fr / zh-Hans — with every UI gate on the real tree. UI-07 (single page, resizable 0.6×–1.5× at a fixed aspect, `editorScale` persisted) runs through all of it.

**Nothing Stage 2 proved changes.** `processBlock`, `prepareToPlay`, `fillParams`, `createParameterLayout`, the engine, `MeterFrame` and the 12 parameters are not touched. **digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9` stay asserted verdicts**; digest-2.3 `edd1acfd21f54bcd` must reproduce (informational). The harness denominator moves **241 → 251** (the 241 Stage-2 checks untouched + 10 new Stage-3 checks, T4) — a reader must not take "251/251" as a change to the Stage-2 verdict (R7).

Covers UI-01…UI-08 (gated in T8 / T13; verify closes them). Re-gates PERF-01 (harness PERF-01 re-run, `processBlock` unchanged) and COMPAT-01 (pluginval s10 now opens the WebView editor — a first for this plugin).

## Decisions adopted (planner picks + the user's decision — each is binding on the executor)

| # | Decision | Source |
|---|---|---|
| **U1 — host-idle display = Param fallback (USER, 2026-10-13 — binding)** | **C++:** the editor computes `live = everPublished && staleTicks < 3` (constant `kStaleTicksIdle = 3`, ≈ 100 ms at 30 Hz) from `hasNewData()` polled **before** `read()`; `quantise (f, live)` zeroes `shPeak[16]` and `outPeakL/R` when `!live`; `live` is hashed and rides the payload, so the live→idle edge emits **exactly one zero-peak frame, then silence**. **Page:** while the effective payload has `live === 0` (or before any frame has arrived), draw the puck, the bars and the beam from the **parameters** with the page's own display maths — `shVec (applyR (rotMatrix (yaw, pitch, roll), dirOf (az, el)))` gated by `BAND_OF[i] <= order + 1` → `shGain`; `azEffDeg / elEffDeg` = the azimuth / elevation parameters; `azRotDeg / elRotDeg` from the rotated vector (same pole hold as frames); peaks = the payload's zeros — i.e. the mockup's mock frame engine (`v1-ui-test.html:1386-1440`) without its smoothing and synthetic programme. `#chanWell` gets class `idle` (dimmed slightly, opacity ≈ 0.8, no transition under Reduce Motion). Planner extension for one consistent idle picture: `orderActive = (order + 2)²` and `decoderMix = decoder == 0 ? 1 : 0` follow the same rule. This relaxes "the puck follows the frame" **only while idle**; with `live === 1` the puck, bars and beam follow the frame exactly as CONTEXT locks. **Gates:** harness PL3 (both idle states produced by C++: before the first block, and stale after a publish) + frame-probe A12 / A13 + negative control `idle`; Logic confirmation stays in the human checklist (R1) | user; RESEARCH §Q4.4, R1 |
| **C1 — template base + five fixes** | `v1-PluginEditor-TEMPLATE.{h,cpp}` is the editor (it already merges Wavetable v1.0.1 relays / natives / gesture close and the Additive scale pattern; effect-shaped — nothing to delete). Fixes: (1) `editorScale` is `std::atomic<float>` on the processor, persisted, display-clamped at open (C3); (2) the inline formatter (template cpp:74-105, 355-403) is replaced by `Source/MeterFramePayload.h` (C6) — the template's "one allocation" claim is false (`String::clear()` frees); (3) one drain `read()` in the constructor before `startTimerHz`; (4) peaks zeroed when stale (U1); (5) the lesson reset skips `outputLevel` too (C5) | RESEARCH §Q1, Summary 1 |
| **C2 — one processor batch, one regression run** | Every processor edit lands in T2: guarded `createEditor`, `uiLanguage`, `editorScale`, state get / set, `applyFactoryPreset` + recipes. The full regression protocol (T5) runs **once** after T1–T4. T3's tracer runs the unchanged harness once as a smoke, not the protocol | RESEARCH §Q10, Summary 2 |
| **C3 — `editorScale` persisted** | `std::atomic<float> editorScale { 0.0f }` (0 = never sized → fit the display). `getStateInformation` (on the COPY): `es > 0` → `setProperty ("editorScale", juce::String (es, 3))`, else **`removeProperty ("editorScale")`** (planner addition: a rejected value restored by `replaceState` would otherwise survive in the copy and be re-saved). `setStateInformation` after `replaceState`: `isVoid()` presence test, `getFloatValue()`, accept only finite `0.6 ≤ s ≤ 1.5`. Editor at open: `scale = stored > 0 ? jmin (stored, fitMaxForDisplay()) : fitScaleForDisplay()`, `fitMax = jlimit (0.6, 1.5, available / 780)`. `resized()` stores `getWidth() / 1180.f`. Overrides ARCHITECTURE "Custom state: none" and the siblings' session-only `editorScale` (CONTEXT later + locked) — record in SUMMARY | RESEARCH §Q7, C6 / C10 of §Q11 |
| **C4 — `uiLanguage`** | O-simpleAdditive shape (`PluginProcessor.h:169-191`, `.cpp:473-510` at HEAD): `std::atomic<int> uiLanguage { 0 }`, `static languageCode (int)` (2 → `"zh-Hans"`, 1 → `"fr"`, else `"en"`), `static languageIndex (const String&)` (unknown → 0); written as the code string on the copy; restored only when present (`isVoid()`), so a fresh instance with an old session stays English | RESEARCH §Q7 |
| **C5 — `applyFactoryPreset (lessonId)`** | Wavetable one-pass shape (`PluginProcessor.cpp:1282-1357` at HEAD), no reset-then-apply loop: unknown id → `false`, nothing touched. For each of the 12 parameters (APVTS order) **except `outputLevel` (never reset, never set)**: target = recipe value (`convertTo0to1` of engineering units; choices by index; `orbit` 1) or `getDefaultValue()`; snap through `convertTo0to1 (convertFrom0to1 (t))`; skip when `|getValue() − t| < 1e-6`; else `beginChangeGesture` / `setValueNotifyingHost` / `endChangeGesture`. Message thread, APVTS only, no `JUCE_ASSERT_MESSAGE_THREAD` (the console harness calls it). Stable ids and recipes in the Lesson table below | RESEARCH §Q10, C1 of §Q11; CONTEXT |
| **C6 — payload header** | `Source/MeterFramePayload.h`, namespace `OSimpleAmbisonics::ui`, includes only `dsp/MeterFrame.h`, `dsp/TripleBuffer.h`, `<charconv>`, `<cstdint>`, `<cmath>` — **no JUCE** (the harness includes it). API in the shared spec below. Integers via `std::to_chars` + manual `.` (locale-proof; float `to_chars` is not used). Non-finite → 0 and magnitudes clamped to ±1e6 before `lround` (pluginval fuzz must never break `JSON.parse`). Field names = `MeterFrame.h` + `live` | RESEARCH §Q4.1–4.3 |
| **C7 — the timer is the only sampler** | `timerCallback`: `current = sampleFrame (meterFrames, liveTracker)` then `emitFrame (std::exchange (forceFrameEmit, false))`. `uiReady` emits the stored `current` inside the native (`emitFrame (true)`) and completes — **no `read()` and no tracker tick** (planner refinement of §Q4.5: a handshake must never count as a stale tick). `applyFactoryPreset` sets `forceFrameEmit`. Constructor: drain `read()`, `current = quantise (MeterFrame {}, false)`, `lastFrameHash = 0`, `forceFrameEmit = true` | RESEARCH §Q1.3, §Q4.5 |
| **C8 — harness growth: 241 → 251** | Ten new `check` lines in the default run (T4): ST1–ST6 (state + lessons) and PL1–PL4 (payload + U1 + fixture determinism). The fixture assertions always run; the env var `OSAMB_UI_FIXTURES=<dir>` only adds the file write. Any extra line the executor needs (e.g. a negative control) moves the denominator and SUMMARY states the final number — never fewer than these ten | RESEARCH §Q7, §Q5, Validation table (ST6) |
| **C9 — fixture set** | `tests/ui-fixtures/frames.json` = `[{ name, params, payload }]`, all 12 params in engineering units (choices by index, `orbit` 0/1), `payload` = the exact string the editor emits. Set in the Fixture table below. **Pole pair corrected:** pitch = roll = 0 (RESEARCH's `pole_rot` with pitch 20 tilts the zenith to elRot ≈ 70°, not the pole) | RESEARCH §Q5 (corrected) |
| **C10 — `verify_3_state.py`** | New measurer on the installed VST3 (S1–S3, RESEARCH §Q7); JUCE base64 variant decode (`size.` prefix, 6-bit LSB-first over `.ABC…xyz0123456789+`). `verify_2_1/2/3.py` are Stage-2 verdict files — re-run, never edit | RESEARCH §Q7 |
| **C11 — SH icons** | `scripts/generate_sh_icons.mjs` (plugin-local, Node built-ins only) → `Source/ui/public/js/shicons.js` (`export const SH_ICONS`, hyphen-free name → `BinaryData::shicons_js`) + contact sheet. Real SN3D lobes, per-icon normalised, orthographic camera ≈ 55° down + 20° turn (constants exposed), back-face culled painter's order, polarity sage `#9ec46f` / amber `#e8b04a`, 3 Lambert steps, 18×18 viewBox, 2-decimal coords, page builds paths with `createElementNS`. The mockup's 2D-slice `shIconSvg()` stays as the fallback; **3D vs slice is the user's pick at the checkpoint** (R4) | RESEARCH §Q2 |
| **C12 — compass** | Rose rotation `−h·D2R`, `h = atan2 (sψ·cθ, cψ·cθ)` from the eased yaw ψ / pitch θ (= yaw whenever \|pitch\| < 90°, so CONTEXT's "the rose turns by yaw" holds where it is correct); glyphs counter-rotate by `h`; hold `h` when \|cos θ\| < 1.75e-4; rose alpha `max (0.35, |cos θ|)`; amber chevron = front elevation `asin (sin θ)`, ▲ / ▼, length ∝ \|sin θ\|, hidden below 0.004; roll is not drawn on the rose | RESEARCH §Q6, C2 of §Q11 |
| **C13 — azRot pole hold** | `if (Math.abs (f.elRotDeg) < 89.99) disp.azRot = f.azRotDeg;` for frames **and** the U1 param frame; the rim tick draws at 50 % alpha while held; `azEff` never holds | RESEARCH §Q6, C3; MeterFrame.h:50-53 |
| **C14 — drag + readout fixes** | Top-down dead zone = `TOP.rHead` (20 design px) instead of 3 px (`v1-ui.html:2221`); `fmtDeg`: \|v\| ≥ 179.95 → `180.0°`, no sign (Stage-2 carry "−180 displays as 180"); `orderIdx()` = `choiceOf ('order')` clamped to 0..2 everywhere the page reads the order | RESEARCH §Q3, §Q9; C5 of §Q11 |
| **C15 — band lighting from `orderActive`** | The `.band.on` / `.off` toggles and the station channel-count labels read `disp.orderActive` of the effective frame (real when live, param when idle), per CONTEXT UI-01 "rows by `orderActive`" and UI-04 "channel count label = `orderActive`" (the mockup reads the parameter, `v1-ui.html:2123-2125`); a change of `disp.orderActive` sets `readoutsDirty`. The readout beside the Order control and the beam's `beamCurve (order + 1)` stay parameter-driven (CONTEXT: readouts = parameter; UI-03 "from `order` only") | CONTEXT UI-01 / UI-04 |
| **C16 — dirty rendering** | Per-canvas dirty flags (top, side, chan, decoder, out); a canvas redraws only on a new frame, a parameter event, resize / `applyUiScale`, the `document.fonts.load` resolution (marks **every** canvas), a language change, or while its ballistics / eases / station fade have not converged (\|target − disp\| > 1e-3). One rAF loop that early-outs. Puck (radial gradient + `shadowBlur` 8) pre-rendered once per uiScale × DPR into an offscreen canvas and `drawImage`d | RESEARCH §Q8 (27 % → ≈ 1 % of a core) |
| **C17 — UI-03 beam and UI-05 decoder view land in 3.2** | The finalized page already draws both; CONTEXT's cadence puts the visual check after 3.2. ROADMAP lists them under 3.3 — record as a deviation in SUMMARY | RESEARCH C9 of §Q11 |
| **C18 — probes from C++-producible states** | `tests/ui-stub/generic-overrides.json` with the real ranges / choices / defaults is written **before** any probe (neutral 0..1 seed makes gates vacuous). `frame-probe.mjs` feeds harness-made payloads through `window.__stubEmit`, sets the stub states to the fixture's params, reads **canvas pixels**, polls until two consecutive reads match, and carries three negative controls (`fuma`, `pole`, `idle`) | RESEARCH §Q5; memory `pattern_ui_fixture_state_must_be_producible_by_cpp`, `pattern_probe_must_target_the_branch_the_fix_changed` |
| **C19 — i18n** | Glossary first: an "ambisonics" section in both shared glossaries (RESEARCH §Q9 table) with its own path-scoped commit (R6); then the en copy freeze (FuMa-letter wording: "FuMa letters, ACN order"; the ±180 automation note on azimuth; the gimbal note on pitch), then the fr / zh passes (fr `reviewed: false`, zh `reviewed: 'mt'`), then the width fixes from §Q9. The `Binaural / Stereo`, `Mono sum / Left only`, `1 / 2 / 3` strings stay English (I18N_EXEMPT) | CONTEXT i18n; RESEARCH §Q9, C4 of §Q11 |
| **C20 — tooltip clamp at two viewports** | New `tests/ui-probes/tooltip-clamp-probe.mjs` at exactly 1180×780 and 708×468 (the 0.6× floor), help layer forced on through `localStorage['osamb.tipsEnabled']` | RESEARCH pitfalls 8–9 |
| **C21 — scope fences** | No `MeterFrame` field; no `processBlock` / engine / `prepareToPlay` edit; no bank-fade indicator; no preset-manager natives (Stage 4); no Windows build (Stage 4, COMPAT-02); no new parameter; `.planning/mockups/*` are read-only inputs; `PLUGINS.md`, `REQUIREMENTS.md` untouched (verify) | CONTEXT; RESEARCH Deferred |
| **C22 — references at committed HEAD only (R8)** | Never read O-simpleFM's working tree (uncommitted WIP in another session). If any sibling file cited here shows dirty in `git status`, read it with `git show HEAD:<path>` | RESEARCH R8 |
| **C23 — commits per part** | One commit after 3.1 (T5), one after 3.2 (T9, before the STOP), one for the shared glossaries (T10), one after 3.3 (T13). Before every part commit run the three static gates CI runs on main (`check-i18n`, `i18n-fr-lint`, `i18n-zh-lint`) so main never carries a red static gate; the Playwright gates are the 3.3 bar. Commit recipe in T5 | CONTEXT; CLAUDE.md; memory `index_git_shared_checkout` |

---

## Execution parts and the STOP

```
3.1  T1 payload header ─┐
     T2 processor batch ─┼─→ T3 editor + CMake + page assets ══ TRACER GATE ══→ T4 harness + stub + verify_3_state ─→ T5 regression protocol (once) + fixtures + commit
                                                                                                                   │
3.2  T6 SH icons ─────────────────────────────────────────────────┐                                                │
     T7 page fixes (incl. U1) ─────────────────────────────────────┼─→ T8 frame-probe ─→ T9 build / install / commit ◄┘
                                                                   │                          │
     ⏸ CHECKPOINT — STOP for the user's visual check (no 3.3 work before an explicit go)  ◄───┘
                                                                                              │
3.3  T10 glossary (shared, own commit) ─→ T11 copy + widths + fr / zh ─→ T12 states + budget + tips probe + lessons ─→ T13 gates + final build + SUMMARY + commit
```

**Tracer gate (end of T3):** the first end-to-end build — page served from BinaryData → relays → APVTS, and processor → triple buffer → payload → page — must be proven before any probe is written. A blank page, a dead relay or a broken harness build is architectural: stop and report, do not continue to T4.

---

## Shared spec — `Source/MeterFramePayload.h` (T1; the editor and the harness call the same functions)

```
namespace OSimpleAmbisonics::ui
  inline constexpr int kStaleTicksIdle = 3;                        // U1: ≈ 100 ms at 30 Hz
  struct QuantisedFrame { int gain[16] {}, peak[16] {};
                          int azEff = 0, elEff = 0, azRot = 0, elRot = 0, orderActive = 4,
                              mix = 1000, outL = 0, outR = 0, latency = 64; bool live = false; };
  QuantisedFrame quantise (const dsp::MeterFrame&, bool live) noexcept;   // q3 = lround(v*1000) gains/peaks/mix/out,
                                                                          // q2d = lround(v*100) angles; non-finite → 0,
                                                                          // clamp ±1e6 first; !live → peak[] = outL = outR = 0
  std::uint64_t  hash (const QuantisedFrame&) noexcept;    // FNV-1a 64 byte-wise (seed 1469598103934665603, prime 1099511628211)
                                                           // over gain, peak, azEff, elEff, azRot, elRot, orderActive, mix,
                                                           // outL, outR, latency, live — exactly the emitted ints
  int            format (const QuantisedFrame&, char* buf, int cap) noexcept;   // returns length; 0 if cap too small
  struct LiveTracker { int staleTicks = 0; bool everPublished = false;
                       bool update (bool fresh) noexcept; };  // staleTicks = fresh ? 0 : min (staleTicks + 1, 1000);
                                                              // everPublished |= fresh; return everPublished && staleTicks < kStaleTicksIdle
  QuantisedFrame sampleFrame (dsp::TripleBuffer<dsp::MeterFrame>&, LiveTracker&) noexcept;
                                                           // fresh = hasNewData(); f = read(); live = update (fresh); quantise (f, live)
```

Emitted text (one line, field order fixed, never `-0.000`, the sign comes from the scaled int):
`{"shGain":[1.000,1.000,0.000,0.000,…16],"shPeak":[0.412,…16],"azEffDeg":90.00,"elEffDeg":0.00,"azRotDeg":90.00,"elRotDeg":0.00,"orderActive":4,"decoderMix":1.000,"outPeakL":0.301,"outPeakR":0.120,"latencySamples":64,"live":1}`

The editor holds `char payloadBuf[1024]` as a member and emits `juce::var (juce::String::fromUTF8 (payloadBuf, len))` through `emitEventIfBrowserIsVisible ("meterFrame", …)`; `len == 0` → skip (cannot happen within the clamp; PL1 proves the worst case fits).

## Fixture table (`tests/ui-fixtures/frames.json`, T4 / T5)

Each entry: fresh processor `makeProc (2, 64, preSetup)` at 48 k with the params set **before** prepare, ≈ 0.5 s of `noiseAt` in 64-sample blocks, then one `sampleFrame` → `format`. Non-default params only listed.

| name | params | purpose |
|---|---|---|
| `front_o1` | az 0 | A1, PL2 |
| `left_o1` | az 90 | A1, A4, A13 base, PL2 |
| `behind_o1` / `right_o1` | az 180 / az −90 | A2, PL2 / A6 reference |
| `overhead_o2` | el 90, order 1 | A2 (Z only), PL2 |
| `o1_az45` / `o2_az45` / `o3_az45` | az 45, order 0 / 1 / 2 | A3, A8, A11, PL2 |
| `yaw90_front` | az 0, yaw 90 | A5, A9 lead-in, PL2 |
| `pole_a` / `pole_b` | el 89.999, yaw 30 / yaw 150 (pitch = roll = 0) | A9, PL2 (\|elRot\| quantised ≥ 8999, wrap-aware \|ΔazRot\| ≥ 6000) |
| `stereo_o3` | decoder 1, order 2, az 45 | A10, PL2 (`mix` 0) |
| `silent_after_play` | az 45, order 2; 0.5 s noise, one tick, then 0.5 s zeros, one tick | A13 (live but silent: peaks 0, well **not** dimmed) |
| `idle_before_first_block` | az 90 — prepared, **no `processBlock`**, one tick | A12, PL3 (`live` 0, shGain all 0, orderActive 4) |
| `idle_stale_after_publish` | az 90 — play, tick 1 (fresh), then ticks 2–4 with no `processBlock`; the tick-4 payload | A13, PL3 (`live` 0, peaks 0, shGain = tick 1) |

## Lesson table (C5; ids from `v1-integration-checklist.md:38`, recipes from `v1-ui.yaml:371-379`)

| id | recipe (everything else → default; `outputLevel` never touched) |
|---|---|
| `pointSource` | order 0, azimuth 30, elevation 0, distance 1 |
| `orderStep` | order 2, azimuth 45 |
| `behindYou` | azimuth 180 |
| `overhead` | elevation 90, order 1 |
| `rotateRoom` | azimuth 60, yaw 120, order 1 |
| `farNear` | distance 12 |
| `slowOrbit` | orbit 1, orbitRate 0.05, order 1 |
| `speakersHeadphones` | decoder 1, order 2, azimuth 45 |

No recipe combines order 3 with distance < 1 m (Binaural headroom: order-3 bank +6.5 dB peak, 0.2 m +8.9 dB). Stage 4 refines the recipes and adds the FUNC-08 bank.

---

## Tasks

### Part 3.1 — processor batch, payload header, editor, bindings

#### T1 — `Source/MeterFramePayload.h`
**Files:** create `plugins/O-simpleAmbisonics/Source/MeterFramePayload.h`. **Depends on:** none.
- The shared spec above, verbatim API. AGPL header block; header comment: why a pre-formatted string (bytes deterministic and byte-comparable with harness fixtures; `juce::var` doubles serialise at 15 dp), why no `%f` (process-wide `LC_NUMERIC`), the U1 rule and its constant, "JUCE-free: the render harness includes this header", "integers only through `std::to_chars`".
- `-Wshorten-64-to-32` / `-Wsign-conversion` clean: `(int) std::lround (…)`, `(size_t)` subscripts.
- **Acceptance:** `grep -n "juce" Source/MeterFramePayload.h` finds comments only; compiles inside both the plugin and the harness (proved at T3 / T5).

#### T2 — Processor batch (`Source/PluginProcessor.{h,cpp}`) — every processor edit of the stage
**Files:** modify `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`. **Depends on:** none (builds with T3).
- `.h` (public, beside `meterFrames`, with a comment block each): `std::atomic<int> uiLanguage { 0 };` + `languageCode` / `languageIndex` (C4); `std::atomic<float> editorScale { 0.0f };` (C3); `bool applyFactoryPreset (const juce::String& lessonId);` (C5).
- `.cpp`:
  - Replace the NOTE comment (lines 53-57) and wrap the editor (line 352): `#if JUCE_WEB_BROWSER` / `#include "PluginEditor.h"` / `#endif` directly above `createEditor()`, whose body returns `new OSimpleAmbisonicsAudioProcessorEditor (*this)` under `#if JUCE_WEB_BROWSER` and `new juce::GenericAudioProcessorEditor (*this)` in the `#else` (memory `pattern_render_harness_breaks_on_webview_editor`).
  - `getStateInformation` (361): after `pluginVersion`, write `uiLanguage` (code string) and `editorScale` per C3 on the copy — never mutate `apvts.state` here.
  - `setStateInformation` (373): after `replaceState`, restore both per C3 / C4 (`isVoid()` + parse; never `isDouble()` / `isInt()` — memory `critical_valuetree_xml_roundtrip_loses_type`).
  - Anonymous-namespace `kLessons[8]` table (id + up to 4 `{ paramId, plain }`) and `applyFactoryPreset` per C5.
  - Header comment: add a "Stage 3" paragraph (WebView editor behind the guard; the two non-parameter properties; lessons never touch `outputLevel`).
- **Acceptance:** `git diff -- Source/PluginProcessor.cpp` has hunks only in the header comment, the NOTE / include, `createEditor`, `get/setStateInformation` and the new lesson code — none inside `prepareToPlay`, `processBlock`, `fillParams`, `createParameterLayout`; `grep -c "JUCE_WEB_BROWSER" Source/PluginProcessor.cpp` ≥ 3.

#### T3 — Editor, CMake, page assets — ends with the TRACER GATE
**Files:** create `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `Source/ui/public/js/juce/index.js`, `Source/ui/public/js/juce/check_native_interop.js`, `Source/ui/public/img/insects.png`; modify `CMakeLists.txt`; delete `Source/ui/.gitkeep`. **Depends on:** T1, T2.
- Assets, verbatim copies (no page edits in 3.1): `v1-ui.html` → `index.html`; `v1-i18n.js` → `js/i18n.js`; `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/{index.js,check_native_interop.js}` → `js/juce/` (md5 `01ff05e3…` / `3c4db27f…`); `plugins/O-simpleAdditive/Source/ui/public/img/insects.png` → `img/` (md5 `6d824230…`). EB Garamond stays a direct `${CMAKE_SOURCE_DIR}` embed.
- Editor from the template with C1 fixes 1–4 and C7: `processorRef.editorScale.load / store`; `fitMaxForDisplay()` beside `fitScaleForDisplay()`; members `ui::LiveTracker liveTracker; ui::QuantisedFrame current; std::uint64_t lastFrameHash = 0; bool forceFrameEmit = true; char payloadBuf[1024] {};` (drop `framePayload`, `q3`, `q2d`, `fnv1a`, `appendFixed`); `sampleFrame` in `timerCallback`, `emitFrame (bool force)`, `uiReady` → `emitFrame (true)` then `complete (true)`; drain `read()` before `startTimerHz (30)`. Keep: member order relays → webView → attachments, `attach()` 3-arg / `nullptr`, `.withOptionsFrom` ×12, the 5 natives, `withKeepPageLoadedWhenBrowserIsHidden`, the bare-path `getResource` ladder, `#if JUCE_WINDOWS withUserDataFolder`, gesture tracking + `closeOpenGestures()` before the attachments die, and **`setSize (1180, 780)` as the last numeric `setSize` in the file** (serve-ui `readEditorSize` takes the last match). Rewrite the template's header comments (no "TEMPLATE", no "one allocation", no "for the session" — the scale persists).
- `CMakeLists.txt`: add `Source/PluginEditor.cpp`, `Source/PluginEditor.h`, `Source/MeterFramePayload.h` to `target_sources`; the `juce_add_binary_data (O-simpleAmbisonics_UIResources …)` block from the snippet, after `target_sources` and before `target_link_libraries` (the only binary-data target this plugin has); `O-simpleAmbisonics_UIResources` as the **first** PRIVATE link entry; update the Stage-1 header comments that say "WebView UI: Stage 3" / "no PluginEditor.*". Option block and compile definitions untouched.
- **TRACER GATE (all must hold before T4):**
  1. `ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test 2>&1 | tee "$SCRATCH/osamb-build.log"`; `grep -c "warning:" "$SCRATCH/osamb-build.log"` → 0 (ninja re-runs CMake on the changed CMakeLists; if not, `cmake -S . -B build` — no generator or cache edits).
  2. Harness (unchanged probes): `[PASS]` 241, `[FAIL]` 0, both digest verdict lines PASS, digest-2.3 prints `edd1acfd21f54bcd`.
  3. Natives census: the `getNativeFunction('…')` names in `index.html` and the `withNativeFunction ("…"` names in `PluginEditor.cpp`, each `sort -u`, diff empty — exactly 5.
  4. `node scripts/boot-all-uis.js --plugin O-simpleAmbisonics` → 0 page errors, 0 404s (serve-ui maps URLs from the editor's `getResource` ladder, so this gates every branch).
  5. Launch `build/plugins/O-simpleAmbisonics/O-simpleAmbisonics_artefacts/Release/Standalone/O-simpleAmbisonics-dev.app`, make the window partly visible (an occluded WKWebView never fires rAF), `screencapture -l <windowid>` and read the PNG: EB Garamond (not Times), page not blank, bars drawn; a knob drag moves its readout; a lesson button moves its controls (memory `pattern_standalone_hands_on_via_ax_scripting_and_window_capture`).

#### T4 — Harness Stage-3 checks, stub overrides, `verify_3_state.py`
**Files:** modify `tests/render-harness/main.cpp`; create `tests/ui-stub/generic-overrides.json`, `tests/pedalboard/verify_3_state.py`. **Depends on:** T3 (tracer passed).
- `main.cpp`: `#include "MeterFramePayload.h"`; one new block **after PERF-01 and before the digest prints** (`rtcheck` already disarmed; fresh processors only; no Stage-2 render is touched; PERF-02 stays last). Header comment: "Stage 3 adds ST1–ST6 / PL1–PL4; the 241 Stage-2 checks and both digest verdicts are untouched". Exactly one `check` per item:

| Check | PASS condition |
|---|---|
| **ST1** uiLanguage round-trip | `uiLanguage = 2` → `getStateInformation` → fresh instance `setStateInformation` → 2; the XML (`Proc::getXmlFromBinary`) attribute is `"zh-Hans"` |
| **ST2** absent / unknown | fresh instance restoring XML without the attribute → 0; with `"de"` → 0 |
| **ST3** editorScale | 0.85 round-trips within 1e-3; a fresh instance (scale 0) writes **no** `editorScale` attribute and writes `uiLanguage="en"` |
| **ST4** editorScale rejected | `"abc"`, `"3.0"`, `"0.1"`, `"1.6"`, `"nan"`, `"inf"` each → `editorScale.load() == 0` **and** the re-saved XML carries no `editorScale` (C3 remove-on-copy) |
| **ST5** 12 values bit-equal | two passes (all defaults — incl. `distance`'s raw 1.00000011920929 — and an off-default set with uiLanguage 1, editorScale 1.25): every `getValue()` bit-equal and every raw atomic equal after the round-trip; both properties restored |
| **ST6** lessons (UI-08) | on a processor with every parameter off-default (az −70, el 20, d 5, order 2, yaw 30, pitch 10, roll −15, decoder 1, orbit 1, orbitRate 1.0, inputMode 1) and `outputLevel` −12 dB, with a gesture-counting `AudioProcessorParameter::Listener` on all 12: each of the 8 ids returns `true`; every parameter equals its recipe value or default (plain, within the range snap); **`outputLevel` raw == −12.0f exactly**; begins == ends == number of changed parameters; applying the same id again → 0 gestures; `"nope"` and `""` → `false`, no value changed, 0 gestures |
| **PL1** payload format | same `QuantisedFrame` twice → identical bytes and hash; one peak changed → different hash; `q3 (-4.4e-8f)` prints `0.000`; `juce::JSON::parse` of the text has exactly the 12 keys and two 16-element arrays; a frame with NaN / ±inf / 1e9 fields formats finite numbers within 1024 bytes |
| **PL2** UI-01 / UI-02 C++ facts | `front_o1` gain[0] = gain[3] = 1000, gain[1] = 0; `left_o1` gain[0] = gain[1] = 1000, gain[3] = 0 (W constant, X → Y); `behind_o1` gain[3] = −1000; `overhead_o2` gain[2] = 1000, gain[1] = gain[3] = 0; `o1/o2/o3_az45` orderActive 4 / 9 / 16 and `o1_az45` gain[4…15] = 0; `yaw90_front` azEff 0, azRot 9000 ± 1; pole pair per the Fixture table; `stereo_o3` mix 0 |
| **PL3** U1 live rule | `idle_before_first_block`: live 0, every peak 0, shGain all 0, orderActive 4; stale sequence: ticks 1–3 live, tick 4 live 0 with every peak `0.000` and shGain equal to tick 1, `hash (t4) ≠ hash (t3)` (one emission) and `hash (t5) == hash (t4)` (silence); a publish after the idle → the next tick is live again; `silent_after_play` live 1 with all peaks 0 |
| **PL4** fixture determinism | the whole `frames.json` text built twice from independent processor sets is byte-identical; under `OSAMB_UI_FIXTURES=<dir>` it is written to `<dir>/frames.json` (directory created) and an `info:` line names the path (info lines are not checks) |

- `generic-overrides.json` (C18): `sliders` for the 8 floats with `start / end / skew / interval / def / label` from `createParameterLayout()` (degree params interval 0; distance 0.2–20, interval 0.001, skew 0.2890648 = `log 0.5 / log ((2 − 0.2)/(20 − 0.2))`; orbitRate 0.02–2, interval 0.001, skew 0.2890648; outputLevel −60–0, interval 0.1); `combos` order `["1","2","3"]`, decoder `["Binaural","Stereo"]`, inputMode `["Mono sum","Left only"]`, all `def 0`; `toggles` `{ "orbit": false }`; `natives.getParameterDefaults` in engineering units; `_why` line (real ranges so readouts measure at real widths and `beamCurve` never sees order index 3).
- `verify_3_state.py` (C10): S1 a fresh instance's decoded XML carries `uiLanguage="en"` and no `editorScale`; S2 same-length patch `uiLanguage="en"` → `"fr"`, re-encode, assign `raw_state`, decode again → `"fr"`; S3 the 12 parameter values (decoded) unchanged through S2. Prints PASS / FAIL per item and `ALL PASS` / exit 1. Header: why a new file (the Stage-2 V9 / V9 / V10 round-trips compare parameters only and stay Stage-2 verdicts).

#### T5 — Regression protocol (once), fixtures, commit 3.1
**Depends on:** T1–T4. Every command in the background with a log.
1. Build the four targets → 0 warnings (T3 step 1).
2. Harness ×2: `H=build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/Release/O-simpleAmbisonics-render-test`; `[PASS]` **251**, `[FAIL]` 0; `digest-2.1 verdict … 9189d911a1fe98d5` and `digest-2.2 verdict … 7f35bc9f91e321b9` PASS; digest-2.3 `edd1acfd21f54bcd`; the two logs identical except the PERF-02 line. A digest that moves is a defect (no DSP file changed) — never an expectation to update.
3. Fixtures: from the repo root `OSAMB_UI_FIXTURES=plugins/O-simpleAmbisonics/tests/ui-fixtures "$H"`, then again into `$SCRATCH/fx2` → `cmp` identical.
4. `./scripts/build-and-install.sh O-simpleAmbisonics` (AU cache + dual-variant sweep), then `ninja -C build O-simpleAmbisonics_Standalone` (stays stale under the script).
5. `auval -v aufx OSAm OuDv` → SUCCEEDED (never `auval -a`).
6. `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3` ×2 and the same on `~/Library/Audio/Plug-Ins/Components/O-simpleAmbisonics-dev.component` ×2 → SUCCESS ×4 (its editor tests construct / destroy the WebView editor repeatedly — member order and the gesture-close path).
7. `for v in 2_1 2_2 2_3; do uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy python plugins/O-simpleAmbisonics/tests/pedalboard/verify_$v.py; done` → 25 / 35 / 42 ALL PASS; `uv run --no-project --python 3.12 --with pedalboard python plugins/O-simpleAmbisonics/tests/pedalboard/verify_3_state.py` → S1–S3 PASS.
8. Static gates (C23): `node scripts/check-i18n.js --plugin O-simpleAmbisonics`, `node scripts/i18n-fr-lint.js --plugin O-simpleAmbisonics`, `node scripts/i18n-zh-lint.js --plugin O-simpleAmbisonics` → exit 0 each.
9. `STATUS.md`: Stage 3 table `plan ✓`, `execute →` with "3.1 ✓"; Current Position line.
10. **Commit (path-scoped, temp-index):** immediately before, `git branch --show-current` (= `main`) and `git status --short`; then `export GIT_INDEX_FILE=$(mktemp -d)/index; git read-tree HEAD`; `git update-index --add -- <each path>` and `git update-index --remove -- plugins/O-simpleAmbisonics/Source/ui/.gitkeep`; `git write-tree` → `git commit-tree <tree> -p HEAD -F <msg>` → `git update-ref HEAD <sha>`; unset `GIT_INDEX_FILE`; re-check `git log -1 --stat`; resync only own paths (`git reset -q -- <paths>`). zsh: one path per argument or an array — `$PATHS` does not word-split. A plain `git commit -- <paths>` takes tracked files only and would drop the new ones (memory `pattern_git_commit_pathspec_takes_only_tracked_files`). Paths: `plugins/O-simpleAmbisonics/{CMakeLists.txt, Source/PluginProcessor.h, Source/PluginProcessor.cpp, Source/PluginEditor.h, Source/PluginEditor.cpp, Source/MeterFramePayload.h, Source/ui/public/index.html, Source/ui/public/js/i18n.js, Source/ui/public/js/juce/index.js, Source/ui/public/js/juce/check_native_interop.js, Source/ui/public/img/insects.png, tests/render-harness/main.cpp, tests/ui-stub/generic-overrides.json, tests/ui-fixtures/frames.json, tests/pedalboard/verify_3_state.py, .planning/STATUS.md}`. Message: `feat(O-simpleAmbisonics): Stage 3 part 3.1 — WebView editor + processor batch + meterFrame payload header (harness 251/251, digests unchanged)`.

### Part 3.2 — visuals

#### T6 — SH icon generator → `shicons.js` + contact sheet
**Files:** create `scripts/generate_sh_icons.mjs`; generate `Source/ui/public/js/shicons.js`, `.planning/stages/3-gui/sh-icons-contact.html`; modify `CMakeLists.txt` (+ `Source/ui/public/js/shicons.js` in the binary-data SOURCES), `Source/PluginEditor.cpp` (+ `if (url == "/js/shicons.js")` → `BinaryData::shicons_js`, `application/javascript; charset=utf-8`). **Depends on:** T5.
- `node plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs` writes both outputs; `--check` regenerates into memory and byte-compares with the committed `shicons.js` (exit 1 on drift). C11 rendering; closed forms copied from `Source/dsp/ShEncoder.h:107-136`.
- Self-checks (any failure exits non-zero without writing): (1) the closed forms match an **independent** SN3D formula `N_l^|m| · P_l^|m| (sin e) · (cos m·a | sin |m|·a)` with the associated-Legendre recursion and no Condon–Shortley phase to 1e-12 on a 13 × 7 grid; (2) the letter table equals `W Y Z X V T R S U Q O M K L N P` for ACN 0–15; (3) polarity anchors X(front) > 0, Y(left) > 0, Z(up) > 0, R(up) > 0, U(front) > 0, V(front-left 45°) > 0; (4) two runs byte-identical — the output header carries the generator's sha256 and the camera constants; (5) every icon has ≥ 1 path, no stroke under 0.6 viewBox units.
- Contact sheet: the 16 icons at 0.6×, 1×, 1.5×, 3× on the walnut well beside the mockup's 2D-slice versions (`shIconSvg`, `v1-ui.html:1610-1659`, ported into the generator for the sheet only).
- **Acceptance:** generator exit 0 with the five self-check lines; `--check` exit 0; `shicons.js` contains no prose and no `innerHTML`.

#### T7 — Page fixes in `Source/ui/public/index.html` (incl. U1)
**Files:** modify `Source/ui/public/index.html`. **Depends on:** T5 (the `live` field exists), T6 (icons).
- **U1:** `onMeterFrame` stores the parsed frame and marks dirty; `effectiveFrame()` returns it when `live === 1`, else `paramFrame()` (U1 maths; peaks / out peaks from the payload, zeros before any frame); `stepBallistics()` consumes `effectiveFrame()`; `#chanWell` toggles `idle`; every slider / combo / toggle `valueChanged` marks every canvas dirty so the idle picture follows drags and lessons. Anchor the gating line with `// [probe-anchor:idle-fallback]`.
- **C13** pole hold in `stepBallistics` (line 1729) with `// [probe-anchor:pole-hold]`; rim tick (line 1840) at 50 % alpha while held.
- **C12** compass in `drawTop` (rose 1786-1814) and `stepCompass` (1735-1739).
- **C14** dead zone `TOP.rHead` (2221), `fmtDeg` (1087), `orderIdx()` replacing `choiceOf ('order')` (1761, 1933, 2065).
- **C15** band lighting / channel counts from `disp.orderActive` (2123-2125 and the station subs).
- **C16** dirty rendering in `renderAll` / `tick` (2168-2175) + puck sprite; `document.fonts.load` (2413-2415) marks every canvas dirty.
- **C11** `import { SH_ICONS } from './js/shicons.js'`; `const SH_ICON_STYLE = '3d'` selects the generated icons or `shIconSvg` (slice), paths via `createElementNS`.
- Keep: `JSON.parse` of a string payload, the drag gesture bracket (2179-2228), `scaledToNorm` from `st.properties` (no JS range table), readouts from `getScaledValue()` (never the frame), the canon block byte-identical.
- **Acceptance:** `grep -n "'mousedown'" Source/ui/public/index.html` → 0 hits; `grep -c "probe-anchor:" …` → 2; `grep -n "innerHTML" …` → none outside the canon; the static gates exit 0.

#### T8 — `tests/ui-probes/frame-probe.mjs`
**Files:** create `tests/ui-probes/frame-probe.mjs`. **Depends on:** T7, T5 (`frames.json`, overrides).
- Modelled on `plugins/O-simpleWavetable/tests/ui-probes/wheel-gesture-probe.mjs` (generic stub, `--root`, exit code = failed arms). `createRequire` → `scripts/serve-ui.js` `buildRoot` / `serve` / `resolvePlaywright`; exit 77 if Playwright does not resolve. Viewport = `readEditorSize` = 1180×780, `deviceScaleFactor` 2; assert the served `document.title` names O-simpleAmbisonics (port-clash guard) and the stub banner reads `seed=overrides`.
- Per fixture: set every stub state to `params` (sliders through `setNormalisedValue` with the overrides' ranges, combos `setChoiceIndex`, toggle `setValue`), then `window.__stubEmit ('meterFrame', payload)`; poll a pixel read until two consecutive reads match (no blind sleeps). Bars: `getImageData` on `#chanCanvas` inside each `.bar-cell[data-acn=k]` rect, scanning the bar's centre row from the centre for sage (positive, right) or amber (negative, left); length / half-width = \|value\| ± 2 px. Puck: brass pixel centroid on `#topCanvas`.

| Arm | Req | PASS condition |
|---|---|---|
| A1 | UI-01 | `front_o1`: W ≈ +1, X ≈ +1, Y ≈ 0; `left_o1`: W unchanged within 2 px, Y ≈ +1, X ≈ 0 |
| A2 | UI-01 | `behind_o1`: X amber, length ≈ 1; `overhead_o2`: Z ≈ +1, X ≈ Y ≈ 0 |
| A3 | UI-01 | `o1/o2/o3_az45`: `.band.on` count 2 / 3 / 4, inactive rows computed opacity < 0.5; 16 icons in ACN order with the letters of T6 check (2) |
| A4 | UI-02 | `left_o1`: puck centroid left of the head centre at r(1 m) ± 3 px |
| A5 | UI-02 | `yaw90_front` (stub yaw 90): puck centroid = `front_o1`'s ± 1 px; the rim tick moved to the left; the rose ▲ at screen-left |
| A6 | UI-02 | drag on `#topWrap` from the right of the head → behind → left, with `sliders.get('azimuth')` `sliderDragStarted / Ended` monkey-patched to count: starts = ends = 1; values cross ±180 with no sample of \|az\| < 90 in the rear crossing; final +90 ± 1 |
| A7 | UI-02 | drag on `#sideWrap` upward: elevation rises (up = +), starts = ends = 1, final within 1° of the pointer angle |
| A8 | UI-03 | beam angular extent at r = rBeam(0.707) strictly decreases o1 → o2 → o3 |
| A9 | §Q6 | `yaw90_front`, then `pole_a`, then `pole_b`: the rim tick stays at +90 (± 1 px) |
| A10 | UI-05 | `o3_az45` → `stereo_o3`: `#decCanvas` pixel diff above threshold after the ease |
| A11 | UI-04 | stub yaw change → `#st-rotation.on`; the channel-count labels read 4 / 9 / 16 for `o1/o2/o3_az45` |
| A12 | U1 | stub params = `idle_before_first_block`'s (az 90), emit it: W ≈ +1, Y ≈ +1, X ≈ 0 (from the parameters), puck left at r(1 m) ± 3 px, `#chanWell.idle` with computed opacity ≤ 0.9; then `left_o1` → `idle` removed |
| A13 | U1 | `left_o1` then `idle_stale_after_publish`: peak overlay pixels decay to 0 within 1 s, bars stay Y ≈ +1, puck ± 1 px, `idle` set; while idle, stub azimuth −90 → puck moves right of the head and Y turns amber; **while live** (`left_o1` re-emitted) the same stub change does **not** move the puck; `silent_after_play` → peaks 0 and the well **not** dimmed |

- Negative controls (`--negative-control fuma|pole|idle`): `fuma` emits `left_o1` with shGain[1] / shGain[3] swapped → A1 must FAIL; `pole` serves an `index.html` copy with the `probe-anchor:pole-hold` condition removed → A9 must FAIL; `idle` serves a copy with the `probe-anchor:idle-fallback` branch forced to the frame → A12 must FAIL. A patch that matches zero or more than one anchor exits non-zero (a control that patches nothing is vacuous).
- **Acceptance:** `node plugins/O-simpleAmbisonics/tests/ui-probes/frame-probe.mjs` → exit 0 (13 / 13); each of the three controls prints its target arm "FAILS as designed".

#### T9 — Build / install, part-1 record, commit 3.2 → then STOP
**Depends on:** T6–T8.
1. Build the four targets → 0 warnings; harness ×1 → 251 / 0 with both digest verdicts (the processor is untouched since T5 — confirms).
2. `./scripts/build-and-install.sh O-simpleAmbisonics` + `ninja -C build O-simpleAmbisonics_Standalone`; `auval -v aufx OSAm OuDv`; pluginval s10 ×1 VST3 + ×1 AU (the editor TU changed).
3. frame-probe exit 0 + the three controls; `boot-all-uis --plugin O-simpleAmbisonics` 0 page errors / 0 404s; `generate_sh_icons.mjs --check` exit 0; static gates exit 0.
4. Create `.planning/stages/3-gui/SUMMARY.md` **Part 1 (3.1 + 3.2)**: files; the processor batch as implemented; harness 251 with every ST / PL detail; both digests (unchanged) + digest-2.3; pluginval / auval; verify_* results; the fixture list with the pole-pair correction; frame-probe per-arm numbers + controls; deviations (C9 pole pair, C15, C17, C7, C3 remove-on-copy); open items for the checkpoint.
5. `STATUS.md`: execute "3.1 ✓ 3.2 ✓ — visual checkpoint pending"; frontmatter `status: stage_3_execute_part1_complete`, `next_action: plugin-execute` (part 3.3 after the user's go).
6. Commit (T5 recipe). Paths: `plugins/O-simpleAmbisonics/{scripts/generate_sh_icons.mjs, Source/ui/public/js/shicons.js, Source/ui/public/index.html, Source/PluginEditor.cpp, CMakeLists.txt, tests/ui-probes/frame-probe.mjs, .planning/stages/3-gui/sh-icons-contact.html, .planning/stages/3-gui/SUMMARY.md, .planning/STATUS.md}`. Message: `feat(O-simpleAmbisonics): Stage 3 part 3.2 — live visuals (B-format wall, sphere + compass + beam, decoder view), SN3D icons, idle param fallback, frame-probe 13/13`.

### ⏸ CHECKPOINT — user visual check (STOP; no 3.3 work before an explicit go)

Present this list with the Standalone path and the contact-sheet path, then stop:
1. **Meters (UI-01):** azimuth front → left: X hands to Y, W holds at +1; behind: X turns amber; Order 1 → 2 → 3 lights the bands 4 / 9 / 16; peak overlays move with input signal (bars move without it).
2. **Sphere (UI-02):** top-down drag sets azimuth (+90 = LEFT), crossing behind shows `180.0°`, a click on the head does not throw the source; side drag sets elevation.
3. **Rotation:** yaw turns the rose, the beam and the rim tick — never the puck; pitch past ±90° flips the rose heading and fades it, the chevron shows ▲ / ▼; elevation 90 → the rim tick holds at half alpha.
4. **Beam (UI-03)** sharpens 1 → 3; **decoder view (UI-05)** crossfades KU100 head ↔ cardioid pair.
5. **Icons:** open `plugins/O-simpleAmbisonics/.planning/stages/3-gui/sh-icons-contact.html` — pick **3D** or **slice** (0.6× and 1× rows decide, R4).
6. **Resize** 0.6× ↔ 1.5×: aspect holds, whole page scales, canvas text sharp, drags track the pointer.
7. **Idle fallback (U1):** Standalone → Options → Audio/MIDI Settings → output device none: the B-format well dims slightly, peaks fall to 0, dragging the puck still moves the puck, bars and beam; restore the device → undimmed and frame-driven.
8. **Look:** spacing, colour, type against the mockup.

On "go": apply the icon pick (if **slice**: set `SH_ICON_STYLE = 'slice'`, remove the `shicons.js` import, its binary-data entry and its `getResource` branch; the generator and the sheet stay as the record) and any requested visual fix, re-run T8 + the T9 build, then start 3.3. Handoff text: Step 1 `/clear`, Step 2 `/plugin-execute O-simpleAmbisonics` (resumes at 3.3 from STATUS).

### Part 3.3 — glossary, copy, lessons, gates

#### T10 — Shared glossaries (R6 — own commit)
**Files:** modify `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js` (repo root, shared). **Depends on:** the checkpoint go.
- Before editing: `git status --short -- scripts/i18n-fr-glossary.js scripts/i18n-zh-glossary.js` must be empty — if another session has them dirty, STOP and ask (never commit someone else's hunk).
- Add an "ambisonics" section to each, following the file's existing entry shape: azimut / 方位角, élévation / 仰角, lacet / 偏航, tangage / 俯仰 (termNote vs `pitch` → hauteur / 音高), roulis / 横滚, ordre / 阶数 (阶 in 一阶 / 二阶 / 三阶), canal–canaux / 声道, format B / B 格式, encodeur / 编码器, décodeur / 解码器, binaural / 双耳, casque / 耳机, enceintes / 音箱, champ sonore / 声场, harmonique sphérique / 球谐函数, faisceau / 波束, orbite / 环绕, latence / 延迟, à l'horizontale / 水平 (termNote vs `level`); ACN, SN3D, AmbiX, FuMa, HRTF, MagLS, KU100, FOA and the channel letters unchanged (U+0020 around Latin in zh). The new renderings are [ASSUMED] (RESEARCH A5).
- **Acceptance:** suite-wide `node scripts/i18n-fr-lint.js` and `node scripts/i18n-zh-lint.js` (no `--plugin`) exit 0 — no other plugin broken.
- Commit just these two paths (T5 recipe, re-check branch + `git status --short` immediately before). Message: `i18n(glossary): ambisonics section (fr + zh) for O-simpleAmbisonics Stage 3`.

#### T11 — Copy, width fixes, fr / zh passes
**Files:** modify `Source/ui/public/js/i18n.js`, `Source/ui/public/index.html` (CSS / pins only). **Depends on:** T10.
- en copy freeze first: `chanWell` tip "FuMa letters, ACN order" (CONTEXT / yaml wording corrected); azimuth tip "+90° = LEFT; crossing behind jumps from +180 to −180 — host automation drawn between the two sweeps through the front" (R2); pitch tip gimbal note (at ±90° yaw and roll coincide — display only); every other UI-06 topic already in the TIPS table confirmed present (W/X/Y/Z, (N+1)², scene vs source rotation, ACN / SN3D / AmbiX, front / back on headphones, Stereo ignores orders 2–3, the exaggerated air model).
- Then the fr and zh passes against the glossary (two parallel sub-agents allowed only if each returns its entries as text and the executor is the single writer of `i18n.js`); fr `reviewed: false`, zh `reviewed: 'mt'`.
- Width fixes from RESEARCH §Q9: `label.subtitle` (fr), `label.decLineBin` (fr), `label.orbitOn` → `activé` or pill pinned at 54 px, `label.routeEncoder` (fr), the `label.distOpen` / `label.orbitRate` / `label.rollSub` spill and the `label.el.level` × `label.distOpen` overlap (fr + zh), **pin** the header `.title-block` width and the right-aligned `#topWrap .sph-tr` box; measure every width-pinned `[data-i18n]` (Orbit pill, station pills, chips) per language — check-ui-labels is blind to them.
- **Acceptance:** `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` (with `--plugin O-simpleAmbisonics`) exit 0.

#### T12 — States, lesson-row budget, tooltip clamp probe, lessons live
**Files:** create `tests/i18n-states.json`, `tests/ui-probes/lesson-row-budget.mjs`, `tests/ui-probes/tooltip-clamp-probe.mjs`. **Depends on:** T11.
- `i18n-states.json` (C++-producible states only): settings popover open; hover help off; Stereo (`#seg-decoder` click); order 3 (`#seg-order` click); orbit on; one state per lesson = its recipe's exact values set through `window.__stubStates` (everything else default, `outputLevel` untouched) plus the button click for the caption (the stub only logs the native).
- `lesson-row-budget.mjs`: en / fr / zh-Hans via `window.__setLanguage`; all 8 `.tour-btn` on one line and Σ widths + 7 × gap ≤ (hint.left − 8) − row.left; prints the slack; exit = failing languages.
- `tooltip-clamp-probe.mjs` (C20): viewports 1180×780 and 708×468, `localStorage['osamb.tipsEnabled'] = 'true'` before load, assert `innerWidth` equals the viewport first; hover the azimuth knob, the outputLevel knob, the gear and the last lesson button: the `#tooltip` box lies inside the window with ≥ 4 px margin and ends at the pinned or the shrink-to-fit width; exit = failed arms.
- Lessons live: in the stub each button logs `applyFactoryPreset` with its stable id (`window.__stubNativeCalls`); in the Standalone each lesson moves exactly its recipe controls and the Output knob never moves (ST6 is the C++ gate).
- **Acceptance:** budget exit 0 with fr slack ≥ 8 px; tooltip probe exit 0 at both viewports; the 8 native calls logged with the 8 ids.

#### T13 — Gates on the real tree, final build, SUMMARY, commit
**Depends on:** T10–T12.
1. UI gates (exit 77 = Playwright not resolved = nothing verified, not a pass): `node scripts/check-i18n.js --plugin O-simpleAmbisonics` → 0; `node scripts/i18n-fr-lint.js --plugin O-simpleAmbisonics` → 0; `node scripts/i18n-zh-lint.js --plugin O-simpleAmbisonics` → 0 (zh at `'mt'` counted, not failed); `node scripts/check-ui-labels.js --plugin O-simpleAmbisonics` → 0 with banner `seed=overrides` and no never-visible label; `node scripts/boot-all-uis.js --plugin O-simpleAmbisonics --strict-tips` → 0 with 0 page errors, 0 404s, 0 DEAD, 0 late.
2. frame-probe exit 0 + the three controls; lesson-row-budget and tooltip-clamp-probe exit 0.
3. Build the four targets → 0 warnings; harness ×1 → 251 / 0 + both digest verdicts; `./scripts/build-and-install.sh O-simpleAmbisonics` + Standalone rebuild; `auval -v aufx OSAm OuDv`; if `PluginEditor.cpp` or `CMakeLists.txt` changed in 3.3 (slice pick) → pluginval s10 ×1 VST3 + AU.
4. SUMMARY.md **Part 2 (3.3)**: glossary entries added, copy changes, width fixes with before / after px per language, budget slack per language, every gate's exit + table, the icon pick, the human checklist below with what is still open; deferred list (bank-fade indicator, preset-manager natives + FUNC-08 bank, QUAL-01 preset-load clause, Windows build, v1.1 params, TripleBuffer module, fr review, zh `'bt'`).
5. `STATUS.md`: execute ✓; frontmatter `status: stage_3_execute_complete`, `current_phase: execute`, `next_phase: verify`, `next_action: plugin-verify`; Files Created additions.
6. Commit (T5 recipe). Paths: `plugins/O-simpleAmbisonics/{Source/ui/public/js/i18n.js, Source/ui/public/index.html, tests/i18n-states.json, tests/ui-probes/lesson-row-budget.mjs, tests/ui-probes/tooltip-clamp-probe.mjs, .planning/stages/3-gui/SUMMARY.md, .planning/STATUS.md}` (+ `CMakeLists.txt`, `Source/PluginEditor.cpp`, `Source/ui/public/js/shicons.js` removal if slice). Message: `feat(O-simpleAmbisonics): Stage 3 part 3.3 — tooltips, live lesson tour, en/fr/zh-Hans, UI gates green`.
7. Handoff: Step 1 `/clear`, Step 2 `/plugin-verify O-simpleAmbisonics`; STOP.

## Dependency graph

```
T1 ─┐
T2 ─┴─→ T3 (tracer) ─→ T4 ─→ T5 ─┬─→ T6 ─┐
                                 └───────┴─→ T7 ─→ T8 ─→ T9 ─→ ⏸ ─→ T10 ─→ T11 ─→ T12 ─→ T13
```
T1 and T2 are independent; T3 needs both to compile. T6 can be authored in parallel with T7's non-icon fixes; T7's icon swap needs T6. 3.3 never starts before the user's go.

## Files

| Action | Path (under `plugins/O-simpleAmbisonics/` unless noted) | Task |
|---|---|---|
| create | `Source/MeterFramePayload.h` | T1 |
| modify | `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` | T2 |
| create | `Source/PluginEditor.h`, `Source/PluginEditor.cpp` (T6 adds one branch) | T3, T6 |
| create | `Source/ui/public/{index.html, js/i18n.js, js/juce/index.js, js/juce/check_native_interop.js, img/insects.png}` | T3 (index.html T7 / T11, i18n.js T11) |
| delete | `Source/ui/.gitkeep` | T3 |
| modify | `CMakeLists.txt` | T3, T6 |
| modify | `tests/render-harness/main.cpp` | T4 |
| create | `tests/ui-stub/generic-overrides.json`, `tests/pedalboard/verify_3_state.py` | T4 |
| generate | `tests/ui-fixtures/frames.json` | T5 |
| create / generate | `scripts/generate_sh_icons.mjs` → `Source/ui/public/js/shicons.js`, `.planning/stages/3-gui/sh-icons-contact.html` | T6 |
| create | `tests/ui-probes/frame-probe.mjs` | T8 |
| modify (shared, repo root) | `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js` | T10 |
| create | `tests/i18n-states.json`, `tests/ui-probes/lesson-row-budget.mjs`, `tests/ui-probes/tooltip-clamp-probe.mjs` | T12 |
| create / modify | `.planning/stages/3-gui/SUMMARY.md`, `.planning/STATUS.md` | T5, T9, T13 |

Not touched: `Source/dsp/*` (incl. `MeterFrame.h`, `TripleBuffer.h`), `tests/render-harness/{CMakeLists.txt, ReferenceModel.h, BinauralFilterGoldens.h}`, `tests/pedalboard/verify_2_*.py`, `scripts/generate_binaural_filters.py`, `.planning/mockups/*`, `parameter-spec.md`, `REQUIREMENTS.md`, `ROADMAP.md`, root `PLUGINS.md`, any other plugin, any other shared script. `Source/ui/public/modules/*.js` do not exist in v1.0 and would be gitignored if they did.

## Threat register (ASVS V5 — input validation is the only applicable domain)

| ID | Category | Component | Severity | Disposition | Mitigation |
|---|---|---|---|---|---|
| T-3-01 | Tampering | `setUiLanguage (arg)` native | low | mitigate | `languageIndex()` maps anything but `"fr"` / `"zh-Hans"` to 0; only the ASCII code is stored |
| T-3-02 | Tampering | `applyFactoryPreset (arg)` native | low | mitigate | `args[0].isString()` + fixed id table; unknown → `false`, no parameter touched (ST6) |
| T-3-03 | Tampering | state XML `uiLanguage` / `editorScale` | low | mitigate | `isVoid()` + parse, finite, [0.6, 1.5], unknown code → en, rejected scale removed from the re-saved copy (ST2 / ST4) |
| T-3-04 | Info disclosure | resource provider | low | mitigate | exact bare-path whitelist, `std::nullopt` otherwise |
| T-3-05 | Tampering | page copy / icons | low | mitigate | `textContent` / `setLabel` only, `createElementNS` for icons; check-i18n [9] forbids `innerHTML` and `<` in i18n.js |
| T-3-06 | DoS | non-finite meter values → `JSON.parse` failure | low | mitigate | `quantise` maps non-finite to 0 and clamps before `lround` (PL1) |
| T-3-SC | Tampering | package installs | — | accept | none in this stage (RESEARCH Package Legitimacy Audit: no package); the generator uses Node built-ins only |

## Gotchas checklist (executor ticks each)

- [ ] Render harness: guarded include **and** body in `createEditor`; `O-simpleAmbisonics-render-test` built explicitly at every build step (`build-and-install.sh` builds VST3 / AU only).
- [ ] `setSize (1180, 780)` is the last numeric `setSize` in `PluginEditor.cpp`; the scaled call uses `roundToInt` so the regex skips it.
- [ ] `editorScale` read before the first `setSize`; display-clamped at open; removed from the state copy when 0.
- [ ] Drain `read()` in the constructor; `uiReady` never reads; `live` hashed; `kStaleTicksIdle = 3`.
- [ ] `applyFactoryPreset` skips `outputLevel` in the reset **and** the recipe; skip-if-equal; begin / set / end per changed parameter.
- [ ] One binary-data target; hyphen-free new names (`shicons.js` → `shicons_js`); every SOURCE has a `getResource` branch (boot-all-uis 0 404s).
- [ ] Canvas: `pointerdown` dismissers only; fonts-loaded marks every canvas dirty; the Standalone window partly visible before `screencapture` (rAF).
- [ ] Probes: overrides first (banner `seed=overrides`), viewport = editor size, converged pixel reads, controls that patch exactly one anchor.
- [ ] Glossary before localizers; the shared glossaries get their own commit with a fresh `git status` check; suite-wide lints green.
- [ ] Static gates before every part commit; Playwright gates before the 3.3 commit; exit 77 is not a pass.
- [ ] Every commit temp-index + path-scoped, `git branch --show-current` + `git status --short` immediately before, `git log -1 --stat` after, own paths resynced; no tag (never tag unless `/publish`).
- [ ] No read from O-simpleFM's working tree (R8); `.planning/mockups/*` unchanged.

## Human checklist (verify closes these; the checkpoint list above is separate)

- [ ] **Logic Pro — R1 (U1 confirmation):** a stopped, non-armed, non-selected track with the editor open — does the well dim (idle) and do the puck / bars / beam follow parameter moves and automation? Press play → undimmed, frame-driven. Record whether Logic suspends `processBlock` (RESEARCH A2).
- [ ] Logic — R2: record a sphere drag that crosses behind as automation, play it back; record whether the source sweeps through the front (the azimuth tip documents it).
- [ ] Logic — R3: Activity Monitor CPU with the editor open on a static scene vs closed; record the number.
- [ ] Logic: automation moves the UI; one sphere drag = one gesture; closing the editor mid-drag ends the gesture; hide / re-show resumes the meters within a tick (`uiReady`); save / reopen keeps the window size and the language; close / reopen the editor 10× without a crash.
- [ ] Standalone: Tab reaches every knob and the arrows move it; release outside the window ends the drag; double-click resets to default; the hover-help switch persists across reopen; fr and zh-Hans relabel without clipping, the select and the Decoder segment stay English; Reduce Motion stops every transition; Overhead lesson at order 3 does not clip the output meter in either decoder.
- [ ] Optional: Taylor reviews the fr copy; after any change re-run `lesson-row-budget.mjs` and `check-ui-labels.js`.
- [ ] **R9 (planner-flagged, U1 threshold):** with `staleTicks < 3` a host whose block period exceeds ≈ 100 ms (e.g. 8192 samples at 44.1 kHz ≈ 186 ms) would flip to idle between blocks (well dims, peaks zero, puck from the parameters). Logic's buffers (≤ 1024) are far below it. If seen in any host, report it — the fix is the single constant `kStaleTicksIdle`, changed only on the user's say-so.

## Success criteria (verify checks these)

- [ ] Four targets build with 0 warnings; harness **251 / 251** (the 241 Stage-2 checks + ST1–ST6 + PL1–PL4), twice byte-identical except PERF-02; **digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9` PASS**, digest-2.3 `edd1acfd21f54bcd` reproduced; `frames.json` regenerates byte-identical.
- [ ] `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval s10 SUCCESS ×2 VST3 + ×2 AU with the WebView editor; `verify_2_1/2/3.py` 25 / 35 / 42 ALL PASS; `verify_3_state.py` S1–S3 PASS.
- [ ] UI-01: 4 / 9 / 16 rows light by `orderActive`; front → left swaps X → Y with W constant (PL2 bytes + A1 pixels); 16 SN3D icons in ACN order.
- [ ] UI-02: drags set azimuth / elevation with one gesture each and wrap cleanly across ±180 (A6 / A7); yaw turns the compass, not the puck (A5); pole hold (A9).
- [ ] UI-03 beam narrows 1 → 3 (A8); UI-04 station highlight + channel counts (A11); UI-05 decoder view follows `decoderMix` (A10).
- [ ] U1: both idle states produced by C++ (PL3); the page draws from the parameters while idle and from the frame while live (A12 / A13); the three negative controls FAIL as designed.
- [ ] UI-06: `boot-all-uis --strict-tips` 0 dead / 0 late; tooltip clamp at 1180×780 and 708×468.
- [ ] UI-07: single page, fits a 13" display at 1×, scales 0.6–1.5 at a fixed aspect; `editorScale` persists (ST3 / ST4 + Logic save / reopen).
- [ ] UI-08: the 8 lessons apply exactly their recipe, never `outputLevel` (ST6 + Standalone); the fr lesson row fits with ≥ 8 px slack.
- [ ] i18n: check-i18n, fr-lint, zh-lint, check-ui-labels all exit 0 on the real tree; suite-wide lints green after the glossary.
- [ ] SUMMARY.md (Parts 1 + 2) records every literal, the deviations (C3 remove-on-copy, C7, C9 pole pair, C15, C17 ROADMAP placement, ARCHITECTURE custom state), the icon pick, and the open human items.

## Requirement coverage

| Req | Built in | Gated by |
|---|---|---|
| UI-01 (must) | T1, T3, T6, T7 (C15) | PL2, A1–A3, checkpoint 1 |
| UI-02 (must) | T7 (U1, C12–C14) | A4–A7, A9, A12–A13, Logic gesture items |
| UI-03 (should) | page beam + T7 (C13) | A8, checkpoint 4 |
| UI-04 (should) | page stations + T7 (C15) | A11 |
| UI-05 (nice) | page decoder view | A10, checkpoint 4 |
| UI-06 (should) | T11 | boot-all-uis `--strict-tips`, tooltip-clamp-probe |
| UI-07 (must) | T2 (C3), T3 | ST3 / ST4, `verify_3_state.py`, checkpoint 6, Logic save / reopen |
| UI-08 (should) | T2 (C5), T12 | ST6, lesson-row-budget, Standalone |
| Stage-2 carries | T7 (180.0°), T11 (gimbal tip); puck readout already from the parameter | A6 readout, checkpoint 3 |
| PERF-01 / COMPAT-01 | — (no `processBlock` change) | harness PERF-01, pluginval s10 with the editor |

## Contradictions resolved (CONTEXT ↔ RESEARCH ↔ ROADMAP)

1. **Scale-pattern source:** CONTEXT names O-simpleFM; its scale pattern is uncommitted WIP in another session → the committed O-simpleAdditive copy of the same pattern is the reference (C22, R8).
2. **"Puck follows the frame" vs host idle:** resolved by the user (U1) — parameters while idle, the frame while live.
3. **"Rose turns by yaw":** heading from R·x̂, identical to yaw whenever \|pitch\| < 90° and correct beyond (C12).
4. **Lesson reset:** the checklist's "reset EVERY parameter" would set `outputLevel` → `outputLevel` skipped in the reset too (C5).
5. **Letter names:** "AmbiX letter name" → "FuMa letters, ACN order" (copy only, C19).
6. **ROADMAP 3.3 vs cadence:** UI-03 / UI-05 land in 3.2 (C17).
7. **`editorScale` persistence:** siblings keep it session-only, ARCHITECTURE says no custom state; CONTEXT (later, locked) persists it → persisted (C3).
8. **Harness "241/241":** becomes 251 / 251 with the Stage-2 subset unchanged (C8, R7).
9. **RESEARCH-internal:** the `pole_rot` fixture (pitch 20) is not at the pole → pitch = roll = 0 pair (C9); RESEARCH's task table counts "241 + 5" while its validation table adds ST6 → ST6 included, plus PL1–PL4 always on (C8).
10. **Band lighting:** the mockup lights rows from the parameter; CONTEXT says `orderActive` → effective frame's `orderActive` (C15).
