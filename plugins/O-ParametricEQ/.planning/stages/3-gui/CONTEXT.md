# O-ParametricEQ — Stage 3 (GUI) — CONTEXT

**Date:** 2026-11-15
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:** `mockups/v1-ui.yaml`, `mockups/v1-ui.html`, `mockups/v1-ui-test.html`, `mockups/v1-integration-checklist.md`, `parameter-spec.md` (196 parameters), `BRIEF.md`, `REQUIREMENTS.md` (UI-01 – UI-04, UI-06, UI-07, FUNC-08), `research/ARCHITECTURE.md`, `ROADMAP.md` § Stage 3, `stages/2-dsp/VERIFICATION-2.6.md` (Issues 1, 2, 5; Human Verification)

## Goal

The finalized v1 mockup integrated as the editor: 196 parameters bound, the response display as the instrument (true curve from `eqSections`, G9 (b)), analyzer, output meter, per-band fills and listen. Three phases as ROADMAP: 3.1 layout / bindings / scale check, 3.2 interactive response display, 3.3 analyzer / meter / fills / listen.

## Decisions

### U1 — Summed curve follows the selected band's placement (user; mockup open question 1)
One ink line: the diagonal term of the 2x2 composition chosen by the selected band's placement. Stereo and Left → LL, Right → RR, Mid → MM, Side → SS; nothing selected → LL. All-Stereo sessions: all four coincide. One function owns it (`sumView()` in the page).

### U2 — Listen: drag and latch (user; SUMMARY-2.6 open item)
- Listen engages on the dragged band for the duration of a node drag and releases on pointer up / cancel / lost capture.
- The strip's Listen button latches listen on the selected band; pressing it again, or selecting another band's Listen, moves / ends it.
- A drag on another band while latched listens to the dragged band, then returns to the latched band on release.
- Disabling or deleting the listened band ends listen. Closing the editor ends listen (`listenBand` → -1 in the editor destructor; transient, never saved).
- Engine timing is fixed by Stage 2 (30 ms input ramp + 20 ms crossfade, after auto gain).

### U3 — Band creation as in the mockup (user; open question 4)
- Plot click: next free band, Bell, Stereo, not bypassed, default Q, frequency and gain from the click; slope left as stored.
- Empty-chip click: enables that band index at its stored values (a deleted band comes back as it was).

### U4 — Dragonfly plate: the mockup's inline SVG ships (user; open question 5)
Carry the procedural emperor-dragonfly engraving from `mockups/v1-ui-test.html` (`buildPlate()`, line ~879; `#plate` geometry at line 167: right 1.5 %, top 3 %, height 94 %, max-width 62 %, opacity 0.3, rotated -21 deg, behind the grid) into `index.html`. Finalization had dropped it as a sketch; the user wants the mockup look. Colours stay on the page's sepia token, both themes. A raster engraving is not required for v1.

### U5 — 25th band click shows a keyed transient tip (user)
With 24 bands enabled, a plot click shows a brief tip near the cursor, text from a new i18n key ("All 24 bands are in use"), English in Stage 3, French / zh-Hans drafted with the Stage 4 localisation (UI-05). *Amended at plan 3.2 (RESEARCH-3.2 Summary 8): `check-i18n [1]` requires all three languages, so machine drafts ship in 3.2 (`fr` `reviewed: false`, `zh-Hans` `'mt'`); Stage 4 reviews them with the rest.* It must pass `check-i18n` (copy only from the table) and the tooltip edge-clamp rules.

### U6 — 196 relays measured first, 500 ms gate kept (user)
3.1 opens with the integration checklist § 0: all 196 relays + attachments, open time and first-paint time recorded, preset load settles in one or two frames, 10 close / reopen cycles in Release. Fallback (relays for the selected band only, re-pointed on selection) only if open > 500 ms or a preset load does not settle. Decided before 3.2 starts.

### U7 — Output meter on `vu-meter` 1.0.0, fallback ready (user)
First production consumer of `vu-meter` 1.0.0, mounted in `#meter-mount`: vertical, -60 to +6 dB, two bars, peak hold, ruler inside the bordered box, approved colours (`drawMeter` in `v1-ui-test.html`). If it fails UI-06 (-6 dBFS sine within 0.5 dB) or any gate, fall back to O-AnalogEQ's `outputLevel` atomic + 30 Hz event and a page-drawn meter.
*Amended at plan 3.3 (RESEARCH-3.3 C1, Taylor 2026-11-16): the fallback is taken. 1.0.0's JS cannot draw the approved meter (stretched-gradient zones, no ruler, per-frame decay) and its bridge overwrites per block. The meter is page-drawn (ported from `drawMeter`, dB-per-second ballistics), fed by an `outputLevel` event from the editor's 60 Hz timer (`readAndResetOutputPeak`, held over the last H reads). `vu-meter` is removed from the plugin and its `used_by` row.*

### U8 — Stage 2 carry-forwards folded in (user)
- **3.1:** port the eight (count corrected at plan 3.1; RESEARCH-3.1 Q6) probe-only rows from `stages/2-dsp/verify-2.6/` into the render harness (LS10, LS11, LS12, LS15 / PX26, PX27, PX38, PX39) plus DM2 (layout change at the same rate; the editor relies on it when a host renegotiates mono / stereo), AN1 (no analyzer frame straddles a drop) and PX22 (mono placement edit while listening starts nothing).
- **3.3:** fix VERIFICATION-2.6 Issue 1 (late NaN burst inside listen's input ramp with no band running): `Listen::process` reports a non-finite voice output, treated like a non-finite lane at step 8. Probe N1e becomes a render row.
- **Not taken:** symmetric G9 floors (XG-exact / XR-g9) stay a hardening item for a later `/improve`.

### Kept from the mockup without discussion (finalization defaults, not re-opened)
- Header padding 6 px in the 40 px header (open question 2).
- Typed entry opens after the 240 ms double-click window so double-click resets (open question 3).
- Stereo glyph `LR`, so `S` means Side (open question 6).
- Value fields (not knobs) for band frequency / gain / Q; Family A knobs for `output_gain`, `gain_scale`.
- Reflowing free-aspect frame 800 x 500 – 1600 x 1000, default 960 x 600; literal `setSize (960, 600);` kept for the gates.
- UI state in the `UI` child of the state tree: `analyzerMode` (default 2), `displayRange` (default 12), `selectedBand`, editor size, language, hover help. Not in APVTS, not in presets.
- Gain field disabled for Low Cut / High Cut / Notch / Band Pass; Slope disabled for Bell / Notch / Band Pass (D6); Q disabled at 6 dB/oct on slope-honouring types.

## Contract amendment made in this discuss

`ROADMAP.md`:
- Phase 2.6 analyzer criterion: "ring overflow keeps the newest samples" reworded to the decided behaviour (a push never overwrites unread data; keeps its newest samples that fit; processor offers each block's last 32 768). Closes VERIFICATION-2.6 Issue 5.
- Phase 3.1 components: probe-row port (U8).
- Phase 3.3 components and criteria: listen fix (U8), listen latch and band-disable release (U2), N1e row.

Checksum refreshed in `STATUS.md`. `research/ARCHITECTURE.md` not edited.

## Constraints

- Every native function the page or the preset module calls must be registered (`getNativeFunction(` grep vs `withNativeFunction`); an unregistered one hangs its promise silently.
- WebView2: `withBackend (webview2)` + `withUserDataFolder()`; static linking.
- Resource provider gets bare paths; one `UIBinaryData` target incl. `i18n.js`, module JS, EB Garamond css + woff2.
- Canvas text needs `document.fonts.load()` + repaint; tooltips shrink-to-fit at edges; popover / entry blur on capture-phase `pointerdown`.
- Node x from `getScaledValue()` on a true log axis; freq / Q writes use live relay `start / end / skew`.
- `eqSections` carries `sampleRate` (addition to the ARCHITECTURE packet).
- Editor timer drives `SpectrumAnalyzer::process (measured dt)` and `DisplayModel::poll()`; full push after preset load and on `requestEqSections()`.
- Audio thread untouched by the editor except via the existing atomics; 60 fps drag redraw with the analyzer on must not raise audio-thread time.
- UI gates (check-i18n, fr / zh lint, check-ui-labels, boot-all-uis `--strict-tips`) all pass before each Stage 3 commit; add `tests/ui-stub/generic-overrides.json` and a band-selected `tests/i18n-states.json` state.
- No existing processor behaviour changes outside U8's listen fix; listen-off output stays bit-identical.

## Open Questions (for research)

1. Measured cost of 196 relays in WKWebView / WebView2: open time, first paint, preset-load settle (U6).
2. `vu-meter` 1.0.0 API fit for two bars + peak hold with ruler inside the box; how it is fed (event vs native pull) (U7).
3. Listen-state ownership when drag and latch overlap (U2): page-side state machine vs a single `setListenBand` with the page as the only writer; behaviour on preset load that disables the listened band.
4. Plate SVG cost in the canvas layer stack at 1600 x 1000 (static layer, redraw only on resize?) (U4).
5. JS closed-form evaluator vs C++ packets for G9 (b) (0.01 dB) — harness dump format and where it runs.
6. Gesture recording in Logic for the two-parameter node drag (one begin / end per parameter).

## Next Phase

Ready for: research phase (Stage 3, starting with 3.1).
