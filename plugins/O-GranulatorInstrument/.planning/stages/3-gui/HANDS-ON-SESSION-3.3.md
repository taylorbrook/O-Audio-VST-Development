# O-GranulatorInstrument — HANDS-ON-SESSION-3.3 (one human session at 3.3 verify, D47)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

Every pending hands-on file runs here, once, in this order, followed by the Logic AU pass. Execute is not gated on any of it (PD27). Mark rows in each file; copy the totals into the results table below.

## 0. Prepare

```bash
cd ~/Dev/VST-development
./scripts/build-and-install.sh O-GranulatorInstrument      # cache clear + dual-variant sweep + install
ninja -C build OuariconGranulatorInstrument_Standalone
auval -a | grep -i granulatorinstrument                      # expect: aumu OuGI OuDv … O-GranulatorInstrument-dev
```

Quit Logic completely before installing; reopen it after. If the plugin looks stale, quit Logic, rerun the cache-clear lines from the root `CLAUDE.md`, reopen.

## 1. Run order

| Step | File | Rows | Owner phase (route a ❌ to) |
|---|---|---|---|
| 1 | `stages/2-dsp/HANDS-ON-2.1.md` | 13 | 2-dsp — `/plugin-execute O-GranulatorInstrument 2-dsp` (phase 2.1 code) |
| 2 | `stages/2-dsp/HANDS-ON-2.2.md` | 10 | 2-dsp (2.2) |
| 3 | `stages/2-dsp/HANDS-ON-2.3.md` | 9 | 2-dsp (2.3) |
| 4 | `stages/2-dsp/HANDS-ON-2.4.md` | 11 | 2-dsp (2.4) |
| 5 | `stages/2-dsp/HANDS-ON-2.5a.md` | 12 | 2-dsp (2.5a) |
| 6 | `stages/2-dsp/HANDS-ON-2.5b.md` | 11 | 2-dsp (2.5b) |
| 7 | `stages/2-dsp/HANDS-ON-2.6.md` | 13 | 2-dsp (2.6) |
| 8 | `stages/3-gui/HANDS-ON-3.1.md` | 16 | 3-gui — `/plugin-execute O-GranulatorInstrument 3-gui` (3.1 code) |
| 9 | `stages/3-gui/HANDS-ON-3.2.md` | 27 (incl. row 26: fr `vstate.held` "tenue" wording) | 3-gui (3.2) |
| 10 | `stages/3-gui/HANDS-ON-3.3.md` | 18 | 3-gui (3.3) |
| 11 | Logic AU pass (below) | 6 | 3-gui (3.3) unless the row names a 2.x feature |

2.1–2.6 total 79 rows; 3.1–3.3 total 61; Logic 6.

## 2. Logic AU pass (ROADMAP 3.3)

Software Instrument track with O-GranulatorInstrument-dev; an audio track playing a known source (a sine or a voice recording).

| # | Do | Expect | ✅ / ❌ |
|---|---|---|---|
| L1 | Plugin header → Side Chain → the audio track | Record-in meter moves with the audio track (SC L / R); "No signal" clears | |
| L2 | Record-in Manual: Arm into slot I, then Threshold into slot II, then Next Note into slot III | Each take lands in its slot (waveform at commit); statuses as HANDS-ON-3.2 rows 13–14 | |
| L3 | Side Chain → None, Arm | No signal / the take is discarded | |
| L4 | Save the project, quit Logic, reopen | All three takes come back in their slots with names and waveforms | |
| L5 | Stop the transport so the instrument track idles, audio track muted | SC meter reads 0 (expected, R§10: Logic does not feed an idle instrument's side-chain) | |
| L6 | Automate one knob and play notes with an LFO routed (MOD page) | Automation and modulation rings move together; no clicks; CPU meter reasonable at Grain Limit 256 | |

## 3. Results

| File | ✅ | ❌ | Notes / routing |
|---|---|---|---|
| HANDS-ON-2.1 | | | |
| HANDS-ON-2.2 | | | |
| HANDS-ON-2.3 | | | |
| HANDS-ON-2.4 | | | |
| HANDS-ON-2.5a | | | |
| HANDS-ON-2.5b | | | |
| HANDS-ON-2.6 | | | |
| HANDS-ON-3.1 | | | |
| HANDS-ON-3.2 | | | |
| HANDS-ON-3.3 | | | |
| Logic AU pass | | | |

A ❌ in any row routes to the Stage 4 gap wave (`PLAN-4.F` via `/plugin-execute O-GranulatorInstrument 4-polish`), not to 2-dsp / 3-gui *(rewritten at Stage 4)*. UI-01 and UI-02 close at 3.3 verify only when their files are ✅.
