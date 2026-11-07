# O-Granulator — Phase 3.3 DAW pass (consolidated Stage 2 + 3 Logic Pro AU session)

**Stage:** 3-gui / 3.3 · **Run at:** 3.3 **verify**, by Taylor (CONTEXT-3.3 D37, PLAN-3.3 P140), in the same session as HANDS-ON-3.3.md. Not run in execute. · **Written:** execute T8, 2026-11-07
**Build:** the installed `O-Granulator-dev` AU from the 3.3 execute commit, **with the WebView UI**. Install: `./scripts/build-and-install.sh O-Granulator` (it sweeps the AU cache). Quit and relaunch Logic after the install.

This is the one Stage 3 DAW session D37 asked for. It runs the carried Stage 2 DAW items **verbatim from their own documents** (each item's recipe, listen-for and PASS rule are in the referenced file; they are not copied here so there is one source of truth), then the four 3.3 Logic items below.

**General setup:** Logic Pro (AU), 48 kHz, buffer 256, monitors at normal listening level. O-Granulator on a stereo audio track, editor open (the WebView), from defaults unless the item says otherwise. Record PASS / FAIL and one line of what you saw / heard per item.

---

## Part 1 — carried Stage 2 items (run from their own files)

| Ref | Items | File | FAIL consequence |
|---|---|---|---|
| DAW-2.5a | 1–8 (tempo sync, re-timing, ramp, LFO zipper, synced LFO across a loop, 3/4 (watch item 6), reverb modulation, count-in / pre-roll) | `../2-dsp/DAW-2.5a.md` | FUNC-04 / QUAL-03 reopen (tempo / zipper) — fix under Stage 4 or a 2.x gap phase, per the item |
| DAW-2.5b | 1–5 (onset bursts Free / Tempo, pitch follow at 0 and 2 s, frozen-chord harmonisation, feedback-heavy follow, tempo burst level) | `../2-dsp/DAW-2.5b.md` | FUNC-08 / DSP-08 reopen (onset / follow) |
| DAW-2.6 | 1–3 (∞ tail bounce / stop / CPU, drums over a sustained pad (R85, recorded), 192 kHz CPU) | `../2-dsp/DAW-2.6.md` | Item 1: **D21 fallback (b): `getTailLengthSeconds()` = 600 s** (pre-signed). Item 2 is recorded behaviour (R85). Item 3: PERF-02 |

Run Logic only for this pass (D37). Where an item names Live or Reaper too, record "Logic only" and leave the others for a VST3 pass if COMPAT-03 asks for one (Stage 4 carried).

## Part 2 — 3.3 Logic items

| # | Item | Session recipe | Look for | PASS if |
|---|---|---|---|---|
| 1 | Pills on a stopped transport (R131) | A drum loop region, 120 bpm, track **not** record-armed, `s1_onset_trigger` On. Play 4 bars, then stop the transport mid-hit. | Logic idles the audio thread when stopped on an unarmed track. | The onset pills go dark within 0.2 s of the stop and stay dark. **FAIL → the reader clock is not in use (stuck-lit pills); reopen R131 / D40** |
| 2 | Reopen after a loud passage (R130) | Play a loud passage with the editor **closed** for 10 s, stop, then open the editor. Repeat with the transport running. | The first frame after the reopen. | No phantom peak on the meters and no onset tick on the first frame; the meters show the current level. **FAIL → the 250 ms gap rule is not applied; reopen R130** |
| 3 | Tempo LFO dot on start / locate | `sync_mode` Tempo, LFO 1 `lfo1_rate_sync` 1 bar, square. Start from bar 1, then locate to bar 3 beat 3 while stopped, start again; cycle bars 2–3. | The LFO 1 dot. | The dot jumps to the bar-locked position on start / locate / cycle wrap (expected; it follows the bar-locked phase, R132) and then runs smoothly. A dot that drifts against the bar or freezes while playing is a FAIL (FUNC-04 / UI-03) |
| 4 | Meters in mono and stereo inserts | (a) The plugin as a **mono → stereo** insert on a mono track; (b) **stereo → stereo** on a stereo track. A −6 dBFS sine region, `mix` 0 %. | The out meter and the input lane. | (a) and (b): the out meter reads about −6 dBFS on both channels; the input lane moves with Input Gain. **FAIL → a channel-mapping defect in the out-meter scan (P130); fix under D28** |

---

## Notes for the verifier

- Part 1 items keep their own PASS rules and watch items (DAW-2.5a item 6, DAW-2.6 item 2 R85). This file only collects them into one Logic session.
- A Logic mono-out insert configuration (mono → mono) also exists; if you try it, the out meter's R lane mirrors L (mono out → R = L, P130). Record it as a note.
- The AU registry pin: if Logic shows stale behaviour after the install, quit Logic, run the build-and-install script again (it sweeps the `-dev` / unsuffixed alternate bundles) and relaunch.
- **Verdict:** Stage 3 closes at 3.3 verify only when this pass **and HANDS-ON-3.3.md** both PASS (PLAN-3.3 P142). A Part 1 FAIL reopens its own requirement; it does not block the 3.3 UI items unless it is the D21 tail item (then apply fallback (b) first).
