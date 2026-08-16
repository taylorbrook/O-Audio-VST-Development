# Stage 3: GUI - Research

**Date:** 2026-08-15
**Inputs:** CONTEXT.md (this stage), parameter-spec.md (BINDING), ROADMAP.md Stage-3 phases,
O-Bitrot Stage-1/2 source, repo prior art (O-GrainScatter, O-IntonationPad, O-Comp, O-Tremolo)

---

## 1. LED Bridge Mechanism — DECIDED: timer + `emitEventIfBrowserIsVisible`

**Decision: 30 Hz editor timer pushing `emitEventIfBrowserIsVisible("ledUpdate", json)`.
Reject native-function polling.**

Rationale:
- `emitEventIfBrowserIsVisible` is fire-and-forget: when the view is hidden the event is
  simply not delivered, and no promise/completion is left dangling. This is exactly the
  CONTEXT constraint (native-fn completions are DROPPED while hidden — a polling state
  machine wedges; see memory `critical_webview_completion_gated_on_isvisible`).
- JS side is a stateless render of the latest payload — missed frames cost nothing.
- Established in 10+ repo plugins. Closest exemplar (naturalist + activity viz):
  `plugins/O-GrainScatter/Source/PluginEditor.cpp:261` (`grainUpdate` event, snapshot →
  JSON → emit) with `window.__JUCE__.backend.addEventListener("grainUpdate", ...)`
  — note the JS event listener uses `window.__JUCE__.backend.addEventListener` (this is
  the one legitimate `__JUCE__` use; param binding still goes through the `Juce` ES-module
  namespace).
- O-Comp's `evaluateJavascript` meter (PluginEditor.cpp:248) also works but is the weaker
  pattern (string-built JS, no hidden-view gating). Don't use it.

### RT-safe publication path

Audio thread → single `std::atomic<uint32_t> uiActivityMask` member on the processor:

1. Inside the existing per-sample loop in `processBlock` (PluginProcessor.cpp:~549–613),
   OR-accumulate a **local** `uint32_t` from the four event-family activity accessors
   (4 bool reads per sample — negligible; guarantees no missed short events at ANY block
   size, so no 512-vs-4096 visibility difference and no extra event counters needed).
2. One `store(..., std::memory_order_relaxed)` at block end.
3. Editor timer (`startTimerHz(30)`, O-Comp precedent PluginEditor.cpp:207) loads the mask,
   applies pulse-stretch, emits JSON.

**Determinism guard (critical):** the telemetry is read-only over existing DSP state —
const accessors only, no RNG draws, no state mutation, no new members inside the DSP
classes' processing paths. Stage-2's 44/44 harness bit-identity must be unaffected; the
render harness never compiles the editor (`#if JUCE_WEB_BROWSER` guard already in place,
PluginProcessor.cpp:627–636).

**Pulse-stretch on the message thread**, not the audio thread: editor keeps a
per-family `lastActiveMs` and holds the LED lit ≥ ~120 ms after last-seen-active. Keeps
all cosmetic timing out of the DSP.

## 2. Per-Family LED Semantics

| Family | Activity source | Accessor status | LED behavior |
|--------|----------------|-----------------|--------------|
| Tape | `TapeTransport` state ≠ Idle (Bend/Stop/Releasing) | `isIdle()` exists (TapeTransport.h:71) — use `!isIdle()` | Held while event runs (ramps 20–500 ms) |
| CD Skip | `CDSkip` state ≠ Idle (Conceal/Mute/Loop) | `isActive()` exists (CDSkip.h:85) | Held; short ms-mutes caught by per-sample OR + 120 ms stretch |
| Vinyl | locked groove OR pop ringing | `isActive()` exists but = `locked` only (VinylTransport.h:76). **Add `isAudible()` = locked \|\| popLevel above threshold** — covers momentary jumps via the pop that every jump fires | Flash per jump (pop decay), held while locked |
| Packet | current packet lost/concealing | **No public accessor — add `isConcealing()`** exposing the lost/`stateBad` condition (PacketLossStage.h:369 area) | Held during loss bursts (packets are 20 ms, ≥ poll period w/ stretch) |
| Codec | continuous process | none needed | **JS-only:** steady-lit while CODEC_ENABLE toggle is on — no bridge traffic |
| Crush | continuous process | none needed | **JS-only:** steady-lit while CRUSH_ENABLE on |

New accessors are 1-line `const noexcept` getters — zero DSP behavior change. Only the
four event families ride the bridge (bits 0–3); continuous families derive from toggle
state the JS already has.

## 3. Parameter Binding Surface (31 params, spec BINDING)

| Relay type | Count | Params |
|-----------|-------|--------|
| `WebSliderRelay` + `WebSliderParameterAttachment` | 19 | 18 floats + SEED (Int 0–9999 binds fine as a slider relay) |
| `WebToggleButtonRelay` + attachment | 7 | 6 family enables + HARD_EDGES |
| `WebComboBoxRelay` + attachment | 5 | CLOCK_MODE, CLOCK_SYNC_DIV, VINYL_RPM, PACKET_CONCEAL, CODEC_MODE |

- JUCE 8.0.14 3-arg relay/attachment form — copy the exact pattern from
  `plugins/O-GrainScatter/Source/PluginEditor.cpp` (its relay block is current and verified).
- **Member declaration order:** all relays declared BEFORE the `unique_ptr<WebBrowserComponent>`
  (construction order), attachments AFTER (repo pattern).
- JS: `type="module"`, `import ... from "juce/index.js"`, pass the **`Juce` ES-module
  namespace** — never `window.__JUCE__` for param binding (memory
  `critical_juce_webview_namespace_vs_postmessage`).
- Readouts via `SliderState.getScaledValue()` — CLOCK_FREE_RATE and CRUSH_RATE have
  exponential skew; a JS min/max map would drift (memory pattern).
- Bools via `getToggleState`, Choices via `getComboBoxState`.
- Bridge-gap check at execute: grep-diff `getNativeFunction` (JS) vs `withNativeFunction`
  (C++) — must match 1:1 (memory pattern). Note: the LED bridge adds NO native functions
  (events only), so ideally this list stays empty or near-empty.
- UI glyph freedom: spec labels are ASCII (`Mu-law`, `33 1/3`); the WebView may render
  μ-law / 33⅓ — safe because glyphs live in HTML, not `juce::String` literals.

### Reseed dice (Phase 3.3, UI-02/FUNC-04)

Pure JS — no native function needed: on click,
`seedState.setNormalisedValue(Math.random())` (0–9999 int param; JUCE quantizes),
bracketed by `sliderDragStarted()`/`sliderDragEnded()` so hosts see one undoable gesture.
Seed readout = `getScaledValue()` rendered as integer. Dice button CSS/roll animation
prior art: `plugins/O-IntonationPad/Source/ui/public/index.html:672–698` (`.dice-btn`,
`dice-roll` keyframes) — reuse the visual, simplify to single-action (no menu).

### CLOCK_MODE control swap

Both SYNC_DIV and FREE_RATE stay bound at all times (no dead relays); the CLOCK_MODE
comboBoxState listener toggles CSS visibility between the two controls. No dead params,
satisfies the CONTEXT requirement, and automation of the hidden param still works.

### Enable-dimming

Toggle-state listener adds `.disabled` to the panel → CSS `opacity: ~0.45` +
`pointer-events: none` on the control area (enable toggle itself stays clickable).
Same listener drives Codec/Crush steady LEDs.

## 4. Naturalist Template Fit (`ouaricon-naturalist-001`)

Template: `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` (720 lines).
Palette anchors: aged paper `#F5E6D3`/`#EBD9C7`, walnut `#8B7355`, dark brown `#5C4033`,
moss/sage active green `#8BA870`/`#6B8E4E`, seed-cross-section knobs.

- The template's prose is panel-count-agnostic; ~12 naturalist plugins shipped with it.
  Densest prior layouts are 900×700 with multiple panels (O-GrainScatter research §11).
  Six panels + strip at ≈900×620 needs a **compact-panel variant**: ~56–64 px knobs,
  4 controls per panel, panel headers as small-caps specimen labels — a mockup-phase
  refinement, not a template blocker.
- **LED styling opportunity:** render event LEDs as thematic marks (spore print dot / wax
  seal that glows moss-green `#8BA870` when active, ~30% opacity brown when idle) rather
  than literal hardware LEDs — consistent with "decorative but never frivolous".
- Layout teaches the chain (CONTEXT decision): top row Tape/CD/Vinyl, bottom row
  Packet/Codec/Crush, global strip bottom.

## 5. Assets — texture and specimen plate

- **Paper texture: use O-Tremolo's clean texture** `plugins/O-Tremolo/Source/ui/public/img/paper.jpg`
  (md5 `40c5f97e25bd2492a6c8fe2ef0882541`, verified watermark-free at 3.2× contrast).
  **Do NOT copy O-Lyrica/O-Gain `paper1.jpg`** (md5 `b7c865c45f2fb95a7a8651071da186e6`) —
  it is the tiled "Adobe Stock" WATERMARKED texture (repo is public AGPL; this already
  shipped by accident twice). Verification step for execute: md5-compare + brighten/look.
- **Decomposing-specimen illustration:** source a genuine public-domain 19th-century
  mycological/botanical plate (rotting fruit, fungal bloom). Precedent with recorded
  provenance: O-SpectralShaper's `slug-overlay.webp` (Trinchese lithograph, *Atti della
  R. Università di Genova*). Good hunting grounds: BHL (biodiversitylibrary.org) scans of
  mycology atlases. Requirements: transparent-background WebP, provenance recorded in the
  plugin (CHANGELOG or a PROVENANCE note) at import time — never ship "provenance unknown".

## 6. Build / CMake Deltas

Current `plugins/O-Bitrot/CMakeLists.txt` has `juce_gui_extra` but **no WebView config yet**.
Stage 3 adds:

- `NEEDS_WEB_BROWSER TRUE` (+ `NEEDS_WEBVIEW2 TRUE` for Windows) on `juce_add_plugin`,
  `JUCE_WEB_BROWSER=1` compile definition.
- Windows: WebView2 **static linking** define + `withUserDataFolder()` (memory
  `critical_webview2_static_linking`, `critical_webview2_runtime_gotchas_windows`).
- `juce_add_binary_data` for `Source/ui/public/` — caveats: hyphens stripped from
  generated symbol names (`check_native_interop.js` → `check_native_interop_js` is fine,
  avoid hyphenated asset names); if a second binary-data target ever appears, give it a
  distinct `NAMESPACE`.
- Resource provider: explicit mapping with **bare paths** (provider receives no scheme);
  don't hard-code `juce://`.
- **Render harness must keep building:** harness target compiles PluginProcessor.cpp with
  `JUCE_WEB_BROWSER=0` and no editor sources — guard already in place
  (PluginProcessor.cpp:627–636). Re-run the harness after Stage-3 processor edits
  (uiActivityMask member + block-end store) to confirm 44/44 still green — the memory
  pattern `render_harness_breaks_on_webview_editor` is the exact failure mode to avoid.

## 7. Mockup Workflow Entry

- Entry: invoke the `ui-mockup` skill ("Design UI for O-Bitrot") — orchestrator runs
  ui-design-agent iterations, then ui-finalization-agent on "Finalize".
- Outputs land in `plugins/O-Bitrot/.planning/mockups/` (7 files: YAML, test HTML,
  production HTML, C++ boilerplate, CMake snippet, integration checklist, parameter-spec).
- parameter-spec.md is BINDING — mockup refines layout/UI labels only.
- Mockup must include: 6 compact panels + strip, event-LED marks in each panel header,
  dice + seed readout, CLOCK_MODE swap affordance, dimmed-state design, fixed size
  (≈900×620 finalized there).
- Sequencing for plan phase: mockup (design+finalize) is the entry task of Phase 3.1
  (ROADMAP: "mockup HTML → `Source/ui/public/index.html`").

## 8. ROADMAP Delta

ROADMAP's three GUI phases (3.1 layout / 3.2 binding / 3.3 dice+clock+polish) predate the
discuss-phase LED scope. Recommendation for plan: fold the LED bridge into **Phase 3.3**
(processor mask + accessors + timer emit + JS render), keeping 3.1/3.2 as written. Its test
criteria: LEDs flash on audible events per family with each family soloed; no harness
regression (44/44); no new native functions.

## 9. Pitfalls Checklist (execute-phase)

1. Relay member order before `webView` unique_ptr; attachments after.
2. `Juce` ES-module namespace for binding; `window.__JUCE__.backend.addEventListener`
   ONLY for the `ledUpdate` event.
3. `getScaledValue()` for readouts (two skewed params).
4. No completion-dependent LED state machine (hidden-view drops).
5. Per-sample OR-accumulate the mask (block-end-only sampling can miss ms-scale CD mutes
   at large block sizes).
6. Const-accessor-only telemetry; RNG untouched; harness re-run after processor edits.
7. md5-check the paper texture against the watermarked hash before commit.
8. Illustration provenance recorded at import.
9. Grep-diff native-fn bridge (expect zero entries).
10. Non-blocking Stage-2 listening items ride along in Stage-3 DAW sessions: Logic smoke
    across families, MIX 50%/0% + HARD_EDGES, ENV_AMT ±100% voicing, Standalone SEED
    persistence.
