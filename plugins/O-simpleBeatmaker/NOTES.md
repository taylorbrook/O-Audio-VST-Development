# O-simpleBeatmaker Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.2
- **Type:** Synth (Pedagogical Step-Sequencer Drum Machine)

## Lifecycle Timeline

- **2026-06-25 (v1.0.0):** First release — 6-voice synthesized 808/909-lineage
  step sequencer for the MUSC319 wk09 MIDI & beatmaking session. Built in four
  staged passes, each gated by the offline render-harness + pluginval/auval.
- **2026-07-15:** Deep code review (`CODE_REVIEW.md`) — 2 critical, 3 warning,
  8 info findings.
- **2026-07-15 (v1.0.1):** Resolved all Critical + Warning findings (CR-01
  unwrapped phase accumulators, CR-02 mono +6 dB double-add, WR-01 partial
  preset apply, WR-02 tail underreport, WR-03 audio-thread MIDI realloc).
- **2026-07-15 (v1.0.2):** Resolved the 8 remaining Info findings (IN-01..08):
  muted-voice viz gating, silence-threshold alignment, free-run playhead wrap,
  live sample-rate in the frame event, dblclick-to-default knobs, block-spans-
  pattern enumeration bound, pointer capture/cancel on knob drags, grid-poll
  local-edit holdoff. Render-harness 12/12 probes pass; auval PASS. Installed.
- **2026-07-15 (v1.0.2 rebuild):** Initial 1.0.2 build shipped self-reporting
  1.0.1 (missed CMakeLists VERSION bump — now fixed) and folded in IN-09: viz
  loop constructed a MidiMessage for SysEx on the audio thread; WR-03 raw-byte
  gate applied. Render-harness 12/12 probes pass. Reinstalled.

## Known Issues

None.

## Additional Notes

- 42-param APVTS + custom atomic 6×32 step grid persisted in a `PATTERN`
  ValueTree child; sequencer emits GM-mapped note-ons into the same MidiBuffer
  as host MIDI (grid and piano roll are two views of one stream).
- Offline render-harness at `tests/render-harness/` is the DSP correctness gate
  (12 probes incl. mono-parity, viz-truth, block-boundary, quantize-preserves-
  swing). Editor is WebView; harness compiles the processor with
  `JUCE_WEB_BROWSER=0`.
- Review record kept at `CODE_REVIEW.md` (all 14 findings resolved as of v1.0.2).
