# Stage 3: GUI - Plan

**Date:** 2026-08-15
**Inputs:** CONTEXT.md, RESEARCH.md (this stage), parameter-spec.md (BINDING), ROADMAP.md
Stage-3 phases, Stage-2 VERIFICATION.md (carried listening items)

---

## Goal

Build the O-Bitrot WebView UI: six family panels (Tape / CD Skip / Vinyl / Packet / Codec /
Crush) + global strip in the Ouaricon Naturalist aesthetic with a decomposing-specimen plate,
all 31 params two-way bound, reseed dice + seed readout, CLOCK_MODE control swap,
enable-dimming, and per-panel event LEDs driven by an RT-safe atomic-mask bridge — with the
Stage-2 render harness still passing 44/44 bit-identical.

**Fixed window ≈900×620 (finalized in mockup). parameter-spec.md is BINDING — layout/UI-label
refinement only; no param ID/type/range/default changes.**

Phase mapping: ROADMAP 3.1/3.2 as written; LED bridge folded into Phase 3.3 (research §8).

---

## Tasks

### Phase 3.1 — Layout and Basic Controls

1. [ ] **Source assets (texture + specimen plate)**
   - Copy clean paper texture from `plugins/O-Tremolo/Source/ui/public/img/paper.jpg`
     → verify md5 == `40c5f97e25bd2492a6c8fe2ef0882541`; assert md5 !=
     `b7c865c45f2fb95a7a8651071da186e6` (watermarked O-Lyrica/O-Gain texture — NEVER ship)
   - Source public-domain 19th-c. mycological/botanical decay plate (BHL scans), process to
     transparent-background WebP; record provenance (source, work, plate) in a
     `Source/ui/public/img/PROVENANCE.md` at import time — no "provenance unknown" assets
   - Files: `plugins/O-Bitrot/Source/ui/public/img/` (new), PROVENANCE.md
   - Depends on: none

2. [ ] **UI mockup workflow (design + finalize)**
   - Invoke `ui-mockup` skill; iterate to finalized design, then finalization agent
   - Must include: 6 compact panels (~56–64 px knobs, small-caps specimen headers) + global
     strip, chain-ordered 3×2 grid (top: Tape/CD/Vinyl; bottom: Packet/Codec/Crush), event-LED
     marks in panel headers (spore-print/wax-seal, moss `#8BA870` active / ~30% brown idle),
     dice + seed readout, CLOCK_MODE swap affordance, dimmed-state design, fixed size finalized
   - Naturalist template `ouaricon-naturalist-001`; parameter-spec BINDING
   - Files: `plugins/O-Bitrot/.planning/mockups/` (7 outputs)
   - Depends on: Task 1 (assets available to the mockup)

3. [ ] **CMake WebView configuration**
   - `NEEDS_WEB_BROWSER TRUE` + `NEEDS_WEBVIEW2 TRUE` on `juce_add_plugin`;
     `JUCE_WEB_BROWSER=1` compile definition; Windows WebView2 static-linking define
   - `juce_add_binary_data` for `Source/ui/public/` (no hyphenated asset filenames — symbol
     names strip hyphens)
   - Confirm render-harness target still compiles with `JUCE_WEB_BROWSER=0` and no editor
     sources (guard exists at PluginProcessor.cpp:627–636)
   - Files: `plugins/O-Bitrot/CMakeLists.txt`
   - Depends on: none (parallel with 1–2)

4. [ ] **WebView editor scaffold + production HTML**
   - Mockup production HTML → `Source/ui/public/index.html`; `check_native_interop.js` copied
   - PluginEditor: relays declared BEFORE `unique_ptr<WebBrowserComponent>`, attachments after;
     explicit resource-provider mapping with BARE paths (no scheme); Windows
     `withUserDataFolder()`; fixed `setSize` per mockup
   - `type="module"` + `import ... from "juce/index.js"` (Juce ES-module namespace)
   - Files: `Source/PluginEditor.h/.cpp` (rewrite), `Source/ui/public/index.html`
   - Depends on: Tasks 2, 3

**Gate 3.1:** build + install; WebView opens at correct fixed size; six panels + strip render
per mockup (UI-01); ES-module loading confirmed (no console errors). Commit.

### Phase 3.2 — Parameter Binding

5. [ ] **C++ relays/attachments — all 31 params**
   - 19 × `WebSliderRelay`+attachment (18 floats + SEED Int), 7 × `WebToggleButtonRelay`
     (6 enables + HARD_EDGES), 5 × `WebComboBoxRelay` (CLOCK_MODE, CLOCK_SYNC_DIV, VINYL_RPM,
     PACKET_CONCEAL, CODEC_MODE) — 3-arg JUCE 8.0.14 form, copy O-GrainScatter's relay block
     pattern
   - Files: `Source/PluginEditor.h/.cpp`
   - Depends on: Task 4

6. [ ] **JS binding + readouts**
   - `getSliderState`/`getToggleState`/`getComboBoxState` via the `Juce` namespace; knob
     drag (relative), two-way sync
   - Readouts via `SliderState.getScaledValue()` ONLY (CLOCK_FREE_RATE, CRUSH_RATE are
     skewed — no JS min/max mapping)
   - UI glyphs (μ-law, 33⅓) live in HTML only — spec labels stay ASCII
   - Files: `Source/ui/public/index.html` (or split JS)
   - Depends on: Task 5

7. [ ] **Enable-dimming + continuous-family steady LEDs (JS-only)**
   - Toggle-state listeners add `.disabled` per panel → `opacity ~0.45` +
     `pointer-events: none` on control area (enable toggle stays clickable)
   - Same listeners drive Codec/Crush steady LED marks (lit while enabled) — no bridge traffic
   - Files: `Source/ui/public/index.html`
   - Depends on: Task 6

**Gate 3.2:** every control moves DSP; host automation + preset load update UI; grep-diff
`getNativeFunction` (JS) vs `withNativeFunction` (C++) — expect zero entries both sides.
Commit.

### Phase 3.3 — Dice, Clock Toggle, LED Bridge, Polish

8. [ ] **Reseed dice + seed readout (UI-02, FUNC-04)**
   - Pure JS: `seedState.setNormalisedValue(Math.random())` bracketed by
     `sliderDragStarted()`/`sliderDragEnded()` (one undoable host gesture); readout =
     `getScaledValue()` as integer
   - Dice visual/roll animation from O-IntonationPad `index.html:672–698`, simplified to
     single-action
   - Files: `Source/ui/public/index.html`
   - Depends on: Task 6

9. [ ] **CLOCK_MODE control swap**
   - Both SYNC_DIV and FREE_RATE stay bound at all times; CLOCK_MODE comboBoxState listener
     toggles CSS visibility between them (no dead params; hidden-param automation still works)
   - Files: `Source/ui/public/index.html`
   - Depends on: Task 6

10. [ ] **DSP activity accessors (1-line const, zero behavior change)**
    - `VinylTransport::isAudible()` = locked || popLevel above threshold (VinylTransport.h:76
      area); `PacketLossStage::isConcealing()` exposing lost/`stateBad`
      (PacketLossStage.h:369 area)
    - Existing: `TapeTransport::isIdle()` (h:71), `CDSkip::isActive()` (h:85) — use as-is
    - `const noexcept` getters only — no RNG draws, no state mutation, no new DSP members
    - Files: `Source/DSP/VinylTransport.h`, `Source/DSP/PacketLossStage.h`
    - Depends on: none (parallel)

11. [ ] **Processor activity mask (RT-safe)**
    - `std::atomic<uint32_t> uiActivityMask` on the processor; inside the existing per-sample
      loop (PluginProcessor.cpp:~549–613) OR-accumulate a LOCAL uint32_t from the 4 event-family
      accessors (bits 0–3: Tape `!isIdle()`, CD `isActive()`, Vinyl `isAudible()`, Packet
      `isConcealing()`); one `store(relaxed)` at block end
    - Per-sample OR is mandatory — block-end-only sampling misses ms-scale CD mutes at large
      block sizes
    - Files: `Source/PluginProcessor.h/.cpp`
    - Depends on: Task 10

12. [ ] **Editor timer → LED events + JS render**
    - `startTimerHz(30)`; `timerCallback` loads mask →
      `webView->emitEventIfBrowserIsVisible("ledUpdate", json)` (fire-and-forget; NO
      native-function polling, no completion-dependent state machine)
    - JS: `window.__JUCE__.backend.addEventListener("ledUpdate", ...)` (the one legitimate
      `__JUCE__` use); stateless render + per-family `lastActiveMs` pulse-stretch ≥ ~120 ms
      on the message/JS side
    - Files: `Source/PluginEditor.h/.cpp`, `Source/ui/public/index.html`
    - Depends on: Tasks 4, 11

13. [ ] **Harness regression re-run**
    - Rebuild render harness (compiles PluginProcessor.cpp with `JUCE_WEB_BROWSER=0`) after
      Tasks 10–11 processor edits; require 44/44 probes green, bit-identical — the telemetry
      is read-only so any diff is a defect
    - Files: none (verification)
    - Depends on: Tasks 10, 11

14. [ ] **Final polish + install + validation**
    - Visual polish pass vs mockup; `./scripts/build-and-install.sh O-Bitrot`; pluginval
      strictness-10 both formats; auval
    - DAW session: LED check per family soloed (flash/held semantics per research §2 table);
      dice/save-restore seed persistence (FUNC-04); sync/free swap; dimming
    - Carried Stage-2 listening items (non-blocking): Logic smoke across families,
      MIX 50%/0% + HARD_EDGES on, ENV_AMT ±100% voicing, Standalone SEED persistence
    - Depends on: Tasks 8, 9, 12, 13

**Gate 3.3:** all test criteria below green; commit `phase: O-Bitrot 3-gui/execute complete`.

---

## Dependency Graph

```
1 ──► 2 ──► 4 ──► 5 ──► 6 ──► 7
      3 ──►┘                ├─► 8 ──►┐
                            └─► 9 ──►┤
10 ──► 11 ──► 12 ───────────────────►├─► 14
       └────► 13 ───────────────────►┘
```

Task 3 parallel with 1–2; Task 10 can start any time; 13 gates 14 alongside the UI tasks.

---

## Success Criteria

- [ ] WebView opens at fixed mockup size; six chain-ordered panels + global strip render (UI-01)
- [ ] All 31 params two-way bound: every control moves DSP; host automation + preset load
      update the UI; readouts correct on skewed params (getScaledValue)
- [ ] Dice writes random 0–9999 to SEED as one host gesture; seed readout visible; seed
      persists through save/restore (UI-02, FUNC-04)
- [ ] CLOCK_MODE swaps visible SYNC_DIV ↔ FREE_RATE with no dead params
- [ ] Disabled family panels dim; enable toggle remains clickable
- [ ] Event LEDs: Tape/CD held during events, Vinyl flashes per pop + held while locked,
      Packet held during loss bursts (each family verified soloed); Codec/Crush steady-lit
      from toggle state
- [ ] LED bridge: no native functions added (grep-diff empty); survives hidden view
      (fire-and-forget events only)
- [ ] Render harness 44/44 green, bit-identical, after processor edits
- [ ] Paper texture md5 == clean O-Tremolo hash; != watermarked hash; specimen plate
      provenance recorded
- [ ] pluginval strictness-10 both formats + auval pass; plugin installed

## Pitfalls (from research §9 — check at execute)

Relay member order before webView / attachments after · Juce namespace for binding,
`__JUCE__.backend` only for ledUpdate · getScaledValue readouts · no completion-dependent
LED state · per-sample OR-accumulate · const-accessor-only telemetry + harness re-run ·
texture md5 gate · provenance at import · empty native-fn grep-diff · carried listening items.
