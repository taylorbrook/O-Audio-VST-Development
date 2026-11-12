# O-GranulatorInstrument — HANDS-ON-3.1 (human gate)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 3.1: the v1 page in the WebView editor at a fixed 1200 × 800, 261 parameters bound, 34 native functions, presets, slot actions, value entry.
**Run in:** the Standalone and one DAW (Logic AU `aumu OuGI OuDv`, or a VST3 host). Rebuild the Standalone first: `ninja -C build OuariconGranulatorInstrument_Standalone`.
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌. 3.1 rows only; HANDS-ON-2.1–2.6 and the Logic DAW pass run once at 3.3 verify (D39).

## What automation already covers (do not repeat by hand)

Headless: 261 / 261 bindings both directions under a C++-seeded stub, value entry 14 / 14 rows, label fit in en / fr / zh-Hans over 19 states. Validators: auval, pluginval 10 VST3 + AU with the sidechain on and off. **Not covered by automation:** the real JUCE relay ↔ attachment plumbing in a host, pointer drags, anything in a real WKWebView. That is what this sheet is for.

## Rows

| # | Where | Do | Expect | ✅ / ❌ |
|---|---|---|---|---|
| 1 | Standalone + DAW | Open the editor | 1200 × 800, not resizable, not blank; all four pages (STREAMS, FX · FEEDBACK, MOD, VOICE) render with no clipped or overlapping text | |
| 2 | Each page | Drag one knob, one choice knob, one fader, one segment row, one `<select>`, one toggle; on STREAMS also a loop handle and a root field | Readout follows; sound follows; double-click resets to the default | |
| 3 | DAW | Write one automation lane for a slider (e.g. Stream I Size), a choice (e.g. Grain Limit) and a toggle (e.g. Stream II enable); play it back | The UI control moves with the lane; touching the control writes automation (one gesture per drag) | |
| 4 | Preset bar | Change several knobs, a scale, a degree mask and load a file into slot I. Save a preset. Change everything, clear by loading another preset, then load the saved one | Parameters, scale, masks and the slot I source come back (the file reloads from its path; the preset stores no audio) | |
| 5 | Preset bar | Set OUTPUT to −12 dB, turn Arm On, load any preset | OUTPUT stays −12 dB; Arm does not change and no recording starts | |
| 6 | DAW | Load a preset, save the project, reopen; change one degree mask, save, reopen | Both reopens restore exactly what was saved | |
| 7 | STREAMS lanes | Load a file with the folder button; Relink it to another file; Reset the slot; Reset All; toggle Embed on a file slot | The lane shows the name, badges and the waveform after each action; Reset returns the pristine tape; Embed on a factory or empty slot is refused | |
| 8 | Any knob | Click the knob's NAME. Type a value, Enter. Again, Esc. Again, click elsewhere | The name lights on hover; the readout becomes a field; Enter applies, Esc cancels, click-away applies; out-of-range clamps; units optional | |
| 9 | MOD, Sync = Tempo | Click an LFO Rate name, type another division the way the readout writes it | That division is selected; text that is not a division changes nothing | |
| 10 | VOICE, language = Français | Click a Scan name, type the French face (e.g. `Redéclench`) | The mode is selected; English names work too | |
| 11 | Any | With a field open, switch language in the gear menu | The field closes without writing | |
| 12 | Header | Look at the gear at (1124, 62) beside the source dock; open it | Reads well as placed (D40); the popover opens down-left and does not cover anything needed | |
| 13 | Gear menu | Switch to Français, then 简体中文, on all four pages | No clipped or overlapping label | |
| 14 | macOS Settings | Turn Reduce Motion on, reopen the editor | No animated transitions; everything still usable | |
| 15 | Standalone (Web Inspector) | Open the console and the network tab, reload | No error, no 404 | |
| 16 | STREAMS | Look at the window figures and lane text | EB Garamond on the canvas text (not a fallback face) | |

## Expected 3.1 placeholders (not defects)

- VOICE strip reads "idle"; voice cells are blank.
- MOD: LFO curves draw, but no moving dots, no mod rings, no meters.
- Record-in reads "no signal" with a sidechain bus and "No sidechain" without one; Arm from the UI still records (engine), but there is no record head, progress or run state on the page.
- Lanes: no playheads, no grain dots, no live overview while recording (the waveform is the pristine tape).
- Clear (×) and Save recording refuse silently.
- OUTPUT / Record-in meters do not move.

## Known, for your decision at verify

- **Dropping a file on a lane works but is not durable yet.** The dropped file loads from a temporary folder that is deleted after about 5 minutes, so a project saved afterwards reopens with that slot missing unless Embed was on. Drop is 3.2 scope; use the folder button for anything you want to keep.
- **Scan-mode `aria-valuetext` is not re-localized on a language switch** (the visible readout is). Pre-existing in v1; a one-line fix is available if you want it under D40.
