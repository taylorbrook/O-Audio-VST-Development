---
title: "O-GranulatorInstrument Stage 3 (GUI) — Phase 3.3 Execution Plan"
created: 2026-11-10
juce_version: "8.0.15"
phase: "3.3"
summary: "Five waves: (W0) pre-flight, Stage 2 + 3.2 baselines, UI gates on the unmodified tree; (W1) engine / recorder / loader / processor surface, each block-path step checked audio-neutral: voice frame extension (state, serial, mod envs, stages, per-voice LFO phases, wantSt) + newestSerial + steal ring + output peaks L / R, sidechain peaks L / R + the CM6 take-name fix, the 16-row display map + 300 ms steal hold + the 3.3 telemetry (newest, menv, sources, modNorm, lfoPh, lo / hi, out, limit) in the one builder, harness rows ui_gates_33 + amended carried rows + 3.3 overrides, commit C1; (W2) the LFO-dot page defect + editor notes, build / install / validators, commit C2; (W3) UI proof: closure [12] / [15] re-pointed + [16], probe rows (voice strip, MOD, drag census, meters, state-hold replay), i18n states (vacuous three removed / rewritten, voice + write states), five gates, commit C3; (W4) regression incl. --cpu26, HANDS-ON-3.3 + the one-session checklist (D47), SUMMARY / STATUS, commit C4. Version 0.1.0, no tag, no new params, no forked-file edit, no module edit."
stages: [3]
agents: [gui-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 3 / Phase 3.3 Execution Plan (MOD page, voice section, meters)

## Goal

UI-02 becomes closable at verify: the 17 instrument sources on the MOD page, drag-to-modulate from every source chip onto every knob and loop handle that has a `data-dest`, mono badges on per-voice → global routes, mod rings and loop-edge ticks driven by the newest voice, the voice / amp section, a voice strip that shows notes, releases and steals, the output and sidechain meters (max since read, L / R), and the `grain_limit` heavy hint. Every value rides the existing ~30 Hz `getTelemetry` pull through the one processor-side `buildUiTelemetry`. The three 3.2 carry-overs are fixed (D49). **Stage 2 audio does not move.**

**Out of scope (D50):** fr `vstate.held` "tenue" (decided at HANDS-ON-3.2 row 26); `WavFailed` silent after a take, one toast for several failing lanes, the save worker joined without timeout on a hung disk, the `output_gain` default of 7.15e-07 dB, the Windows drop check (all Stage 4). No new controls, no visual polish beyond defects (D48). Running the hands-on files is verify's job (D47), not execute's.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/3-gui/CONTEXT-3.3.md` | D47–D50, constraints. **Binding, not reopened.** |
| `stages/3-gui/RESEARCH-3.3.md` | Summary 1–9 (cited **RSx**), §1–§12 (**R§x**). Line numbers there are from `ce413b23`; see "Research corrections" below before relying on R§9's row names and R§12.10. |
| `stages/3-gui/PLAN-3.2.md`, `SUMMARY-3.2.md`, `VERIFICATION-3.2.md` | PD1 (grain frame), PD4 (stale frame), PD9 (`GiRecRunHold` pattern), PD10 (SC peak), PD12 / PD13 (one builder, 3.2 shape), PD23–PD29 (harness / overrides / closure / probe / states / gate order) carry; SUMMARY deviations 1 (CM6), 3 (vacuous states), 4 (UPM6b note), 9 (`GiRecRunHold` lives in `PluginProcessor.h`), 11 (fixture part real, part hand-built). |
| This plugin | `Source/{PluginProcessor.h,PluginProcessor.cpp,PluginEditor.cpp,GiUiJson.h}`, `Source/dsp/{GiTelemetry.h,Engine.h,Engine.cpp,VoiceAllocator.h,AmpEnv.h,GiVoiceMod.h,GiModMatrix.h,GiRecorder.h,SlotLoader.h,SlotLoader.cpp,GiConstants.h}`, `Source/dsp/forked/ModSources.h` (read only: `Lfo::position()`, `getPhaseDeg()`), `Source/ui/public/{index.html,js/i18n.js}`, `.planning/mockups/{v1-ui.html,v1-PluginEditor.cpp}` (T: the telemetry contract, `buildTelemetry`), `tests/render-harness/{main.cpp,ui_gates_31.h,ui_gates_32.h,engine_gates_26a.h,engine_gates_26b.h}`, `tests/{ui_frontend_check.js,ui_binding_probe.js,i18n-states.json}`, `tests/ui-stub/generic-overrides.json`. |
| Parent (read, never edit) | `plugins/O-Granulator/Source/PluginProcessor.cpp:680-745` (routed-only `modNorm`, `lfoPhase`, `outPkL / outPkR`), its MOD / ring / meter closure and probe rows. |
| Repo | `scripts/{check-i18n,i18n-fr-lint,i18n-zh-lint,check-ui-labels,boot-all-uis,serve-ui}.js` (run, never edit), the French glossary. |

Plan-time facts (2026-11-10, HEAD `febe5049`): last code commit under `Source/ tests/ CMakeLists.txt` = **`399fd122`**.
- `PluginProcessor.h` 692 (`GiRecRunHold:94`, `UiTelemetry:378`, `kVoices:380`, `telemetryLock:642`) / `.cpp` 1862 (`setDestRange:235`, `buildUiTelemetry:1172`, voice rows `:1221-1236`, `takeScPeak:1321`).
- `PluginEditor.cpp` 1226 (`getTelemetry:729-736`). `GiUiJson.h` 438 (3.1 placeholder `telemetryToVar (std::uint32_t …):201`, its processor wrapper `:252`, 3.2 encoder `:283-369`).
- `GiTelemetry.h` 169 (`GiUiVoice:73`, `GiUiFrame:80`, `GiTelemetry:145`, `outPeak:147`, `takeOutPeak:163`). `Engine.h` 972 (test accessors `:578-633`, telemetry getters `:528-530`, `newestSer:916`, `destTel / srcTel / lfoPhaseTel:920-922`) / `Engine.cpp` 4105 (`publishTelemetry:754`, voice loop `:801-821`, `lfoPerVoiceActive:1508`, `updateNewest:1705`, `writeTelemetry:1770`, `srcPerVoiceNow / destVoiceNow:1891-1899`, `allocateVoice:2423`, `forceReleaseVoice:2618`, `forceReleaseVoiceMs:2628`, `stealVoice:3019`).
- `VoiceAllocator.h` 216 (`Voice::State {free, active, stolen}:55`, `isReleased:95`, `findFree:205`). `AmpEnv.h:58` / `GiVoiceMod.h:59` stage enums. `GiConstants.h:33-36` (20 slots = 16 counted + 4 spares), `:48` `kStealMs` 5.
- `GiRecorder.h` 618 (`runPiece:306`, peak `:315-316`, `scPeakPush:375 / :571`, `takeScPeak:427`). `SlotLoader.cpp` 1828 (`runCommit:1084`, take naming `:1100-1122`).
- `index.html` 5096 (== `mockups/v1-ui.html`, `cmp` clean): `BIPOLAR_SOURCES:1852`, `PER_VOICE_SOURCES:1855`, `isGlobalDest:3081`, `addRoute:3124`, `attachSourceDrag:3155` (targets `.knob-cell[data-dest], .loop-h[data-dest]` `:3170`), `ingestTelemetry:3887`, `drawAmpFigure:4189`, `drawMenvFigure:4208`, `modulatedNorm:4240`, `drawLane:4245` (`lo / hi:4263-4264`, grain warn bit `:4322`, edge tick `:4327-4332`), `meterBars:4341`, `drawLfos:4386` (dot `:4409`), `drawModSources:4427`, `drawVoices:4461` (`v[7]` wantSt `:4507`), `updateReadouts:4532` (`limit:4536`), `updateLiveModulation:4578`.
- `i18n.js` 3225 (== `v1-i18n.js`). Harness `main.cpp` 1651 (fixture self-check `voices.size() == 20` `:1454`), `ui_gates_31.h` 1002 (UPM6b note `:538`, UTL31 `:881-976`), `ui_gates_32.h` 2313 (UCN1 `limit 0` `:531/539`, UST1 `== 20` `:1885`, UTL32 `:1966-2136`). `ui_frontend_check.js` 740 ([12] `:562-601`, [15] `:638-737`), `ui_binding_probe.js` 1378, `i18n-states.json` 38 states. Harness **402 / 0**.
- **Line numbers drift — re-grep.**

---

## Research corrections (checked against the source at plan time)

1. **Row-name collisions.** `USR1` / `USR1n` already exist (3.1 slot-revision rows, `ui_gates_31.h:34-40`); the sources row is **USRC1**. `UAL1` is a carried 3.2 row; the 3.3 allocation proof is a new row **UAL2**, UAL1 is not amended.
2. **Carried rows the 3.3 shape breaks were not listed.** UTL32 pins `voices` 20, `newest` −1, `menv` / `sources` `[]`, `limit` 0, `out` zeros, `modNorm` `{}`, `lfoPh` absent, `peak[0] == peak[1]` (`ui_gates_32.h:2005-2077`); UST1 pins `voices.size() == 20` (`:1885`); UCN1 asserts `limit 0` on every frame (`:531`, title `:539`); the overrides self-check pins 20 rows (`main.cpp:1454`). PD20 amends them.
3. **"Update the placeholder so UTL31 still pins it" (R§9, R§12.10) does not hold.** UTL31 asserts exactly 13 keys, `lfoPh` absent and no `lo / hi` (`ui_gates_31.h:885-947`), so an updated placeholder fails it. And closure [12] compares the **placeholder's** key set with the fixture (`ui_frontend_check.js:565-600`), so a live `lfoPh` alone breaks [12]. PD14 freezes the placeholder (no production caller: only UTL31 calls it, `ui_gates_31.h:966`) and re-points [12] / [15] at the live encoder.
4. **`wantSt` on the wire is one number per row**, not four: the page reads `v[7]` only (I:4507); T defines it as "the semitones the first enabled stream WANTS before the ±48 clamp". The frame keeps four (for the grain bit); PD11 defines the reduction.
5. **A stolen voice's note is not in the frame** (`note = −1` unless counted, `Engine.cpp:805`); the held stolen row takes its note from the row map's last observation (PD1).
6. **`newest` can name a stolen voice.** `updateNewest` moves `newestSer` only while a counted voice exists (`Engine.cpp:1720`); after a polyphony shrink or All Sound Off it still names the victim, whose row may be in its steal hold. PD4: `newest` = −1 unless the row is counted.
7. **`rec-recording`'s clicks turn streams 2–4 ON** (they default Off, `parameter-spec.md:89`); moving them changes the page every later state sees. PD18 keeps them at the same sequence position.
8. **Drag targets:** the static markup has 137 `data-dest` attributes over 125 distinct destinations (duplicates 6, 8, 34, 36, 62, 64, 90, 92, 122–125); R§5's "133 nodes" is not confirmed. The census counts the page's own target selector at runtime (PD24).
9. Line cite `GiRecorder.h:296` for the SC peak is `noteMinted`; the peak is `runPiece:315-316 / :375`.

Everything else R§1–R§11 states was confirmed: 20 slots vs 16 page rows with first-free identity allocation (`VoiceAllocator.h:205`); steal = 5 ms forced release; `stealVoice` is the only steal site (`Engine.cpp:2430`); `srcTel = srcG`; `destTel` = `v01` through the parameter's own range (`GiModMatrix.h:342-375`, `PP:235`); `takeOutPeak` = `max (|L|, |R|)` with no caller; `takeScPeak` has one caller (`PP:1321`); the CM6 sibling call sits inside the success branch (`SlotLoader.cpp:1111-1117`); the LFO dot draws `shape (pp)` against a curve at `shape (p + ph0)` (I:4401 vs :4409); `frozen` has no page reader; the page's mono rule equals the engine's predicates.

---

## Sign-off (open — Taylor, one line, before T13)

| Item | Plan default |
|---|---|
| R§12.11 / R§6 how "meters match the known sine levels" is read | **SC meter:** exact known-level rows (fs/4 sine, the peak sample IS the amplitude, 0.5 and 0.1 → −6.02 / −20 dBFS, ±0.01 dB). **Output meter:** bit-exact against the rendered output buffer (any level, incl. a known one) + L / R separation; a synth-path known-level output row is not used because grain overlap-add sets the level. **Both:** pixel rows for the bar mapping. Execute proceeds on this default; a different reading from Taylor adds a row, it does not reopen the design. |

## Plan-phase decisions (resolve R§12)

- **PD1 [R§12.1] 16 display rows, keyed by serial.** A pure struct `GiVoiceRowMap` in `PluginProcessor.h` beside `GiRecRunHold` (3.2 dev. 9 precedent), owned by the processor, used only inside `buildUiTelemetry` under `telemetryLock`, clock injected. Rows `[16] = { serial, note, shownState, holdUntilMs }`. Per poll, in this order:
  1. new steals from the ring (PD2): a row holding that serial → `shownState = 3`, `holdUntil = now + kStealHoldMs`;
  2. a row whose serial is not in the frame as a non-free voice and is not in a hold → empty;
  3. rows whose serial is in the frame → state / note / amp / scans / wantSt updated (state 3 in the frame also starts a hold);
  4. frame voices not yet in a row, by ascending serial → the lowest empty row, else the lowest row whose hold expired, else the stolen-held row with the oldest hold (**a counted voice always wins a row**; counted ≤ `polyphony` ≤ 16).
  - A voice keeps its row for its life. A held stolen row shows the note from the row's last observation (the frame sends `note = −1` for stolen voices), the frame's `amp` while the slot still holds that serial (else 0), scans −1, `menv` stage 4, `lfoPh` −1. A stale frame (3.2 PD4) clears the map.
  - Emit **16** rows (`UiTelemetry::kVoices` = `ogi::kMaxCountedVoices`; T:344, the page draws 16). The grain `voice` field goes through the same map (−1 when unmapped; T: "voice 0-15").
  - **Accepted limit (HANDS-ON-3.3):** at polyphony 16 every row is counted, so the new voice takes the victim's row at once and the steal reads as a note change.
- **PD2 [R§12.2] Steal ring + 300 ms hold.** `GiTelemetry` gains `std::array<std::atomic<std::uint64_t>, 16> stolen` and `std::atomic<std::uint32_t> stolenCount`, written **only** in `Engine::stealVoice` (entry, before `forceReleaseVoice`), outside `OGI_TEST_HOOKS`: the serial with a relaxed store at `count % 16`, then `stolenCount` with a release store. Polyphony shrink, All Sound Off and mono handovers do not write it (they are not steals). The reader keeps `stolenSeen`; it takes at most the 16 newest entries per poll (older ones only shorten a hold) and re-reads `stolenCount` after copying, discarding entries the writer may have overwritten meanwhile. `kStealHoldMs` = **300** (≈ 9 polls), a constructor argument of `GiVoiceRowMap` so a harness control can set 0. The existing `hooks.stolenSerials` stays for the 2.x rows.
- **PD3 [R§12.3] Frame extension.** `GiUiVoice` gains: `state` (u8: 0 free · 1 gated (incl. CC64-held) · 2 released = counted && ! gated · 3 stolen), `serial` (u64), `menv[2]` (float, `vm[k].env[e].getLevel()`), `menvStage[2]` and `ampStage` (u8; mapping below), `lfoPh[4]` (float), `wantSt[4]` (float). `GiUiFrame` gains `newestSerial` (u64, from `newestSer`). `note` keeps its 3.2 meaning (−1 unless counted).
  - Stages for the page (1 A · 2 D · 3 S · 4 R · 0 off): `AmpEnv::Stage {idle, attack, decay, sustain, release, forced}` → 0, 1, 2, 3, 4, 4; `GiModEnv::Stage` the same without `forced`.
  - `lfoPh[n]` = `vm[k].lfo[n].position() − getPhaseDeg() / 360` wrapped to [0, 1) (the global path's own expression, `Engine.cpp:1781-1784`) when the voice is not free, `lfoPerVoiceActive (n)` and `vm[k].lfoOn[n]`; else −1. `lfoPerVoiceActive` is evaluated once per n, outside the voice loop.
  - `wantSt[s]` = `voiceSemis (v, pairSp (k, s), absPos)` for a counted voice and a stream whose current tape exists, else 0. Excludes the grain-level `pitch_rand` / scale pick (R§2, accepted: the per-grain draw is not stored).
  - All writes in `publishTelemetry`'s voice loop; nothing is read back by the DSP. `static_assert (trivially copyable)` stays. Two test accessors in the `OGI_TEST_HOOKS` block: `srcPerVoiceNowForTest (int)`, `destVoiceNowForTest (int)` (UMB1).
- **PD4 `newest`.** = the row whose serial == `frame.newestSerial` **and** whose shown state is 1 or 2; else −1 (correction 6). Never a reader-side guess. A note-on in a block's last cell may show one block late; not "fixed" reader-side (R§10).
- **PD5 [R§12.4] Meters split L / R (CONTEXT open question 1).**
  - Engine: `GiTelemetry::outPeakL / outPeakR` replace `outPeak` (`pushMax` each, per block, post `output_gain`); `takeOutPeaks (float& l, float& r)` replaces `takeOutPeak` (no caller today). `scInPeak` / `takeScInPeak` unchanged (TL1).
  - Recorder: `runPiece` accumulates `pkL = max |xL|`, `pkR = max |xR|` beside the unchanged `a` (threshold, capture, `peak` untouched); `scPeakL / scPeakR` replace `scPeak`; `takeScPeaks (float& l, float& r)` replaces `takeScPeak` (one caller, the builder). Mono sidechain: `xR` is the L sample, the bars move together (correct).
  - Builder: `out = [L, R]`, `rec.peak = [L, R]`; `rec.signal` uses `max (L, R)` (identical to today by construction). `UiTelemetry::Rec::peak` stays (= max; URC2 reads it) beside new `peakL / peakR`; `UiTelemetry` gains `outL / outR`.
  - Window = the page's pull; the editor-closed period accumulates and the first open shows its max (CONTEXT; no 250 ms zeroing, unlike the parent's R130). Page ballistics untouched (D48).
- **PD6 [R§12.5] `sources`.** 17 numbers `getSourceTelemetry (1…17)` = `srcG` (newest-voice ramped, held when silent; global sources live). Rejected: the newest voice's raw value (would disagree with the ring dots during a handover).
- **PD7 `menv`.** 16 rows `[e1Level, e1Stage, e2Level, e2Stage, ampStage]` through the PD1 map; an empty row is five zeros; a stolen-held row has `ampStage` 4 and the mod envs from the frame while the slot holds the serial (else 0).
- **PD8 `lfoPh`.** Always emitted: 4 rows of 17, `[global, row0 … row15]`; global = `getLfoPhaseTelemetry (n)`; per row from the frame through the map, −1 for an empty row, a stolen row or a Global-mode LFO.
- **PD9 `modNorm`.** Routed-only port of the parent (P:688-701): routed[d] = some `modM_source` raw > 0 with `modM_dest` raw == d, M = 1…16, depth ignored (the page's `routesFor` rule); `modNorm[String (d)] = getDestTelemetry (d)` for routed d only. Unrouted keys absent (the page falls back to the knob).
- **PD10 [R§12.6] `lanes[].lo / .hi`.** Emitted for lane s only while dest 27 + 28·s or 28 + 28·s is routed: `lo = min`, `hi = max` of the two destinations' `getDestTelemetry` (the unrouted one is the engine's base value). Absent otherwise (the page uses the handles, I:4263-4264). v1 draws it (D48).
- **PD11 [R§12.7] `wantSt` column, grain clamp bit, grain `voice`.** Row column 8 = the frame's `wantSt[s]` of the **first stream that is enabled (`sN_enable`) and has a tape**; 0 when none (correction 4). Grain `flags` bit 1 (value 2, the page's warn colour) = `|wantSt[voiceSlot][stream]| > 48`, read from the frame by the grain's **engine** slot before mapping; bit 0 (reversed) as 3.2; the engine's killing bit is still never sent.
- **PD12 `limit`, heavy hint, `frozen`.** `limit = 32 << grain_limit` from the APVTS raw choice (the engine's cap is the same number; onset thinning lowers quotas, not the cap). Heavy hint stays parameter-driven (`comboIndex ('grain_limit') > 2`, S1); no change. `frozen` and `modRouted` are not emitted (no page reader).
- **PD13 `getTelemetry` 3.3 shape.** `{ scaleRev, slotRev, lanes: [ { ovRev, fb, rec[, lo, hi] } × 4 ], voices: [ [state, note, amp, scan1..4, wantSt] × 16 ], newest, menv: [ [e1, st1, e2, st2, ampSt] × 16 ], lfoPh: [ [g, r0..r15] × 4 ], grains, counts, limit, out: [L, R], rec: { …, peak: [L, R] }, sources: [17], modNorm: { "d": v } }`. 14 top-level keys (13 + `lfoPh`), lane keys 3 static + `lo / hi` when routed, 8 rec keys. Absent stays absent / null, never 0. Floats 4 decimals.
- **PD14 [R§12.10, departs] The 3.1 placeholder stays frozen.** `telemetryToVar (std::uint32_t …)` and its processor wrapper keep the 3.1 shape; UTL31 is **not** touched. Closure [12] reads the key set from the **live** encoder (`telemetryToVar (const …UiTelemetry& t)`) and compares it with the fixture; [15] asserts the live set explicitly (14 / 5 / 8 with `lo / hi`) and "placeholder ⊂ live, difference exactly `lfoPh` (top) and `lo, hi` (lane)". Reason: correction 3.
- **PD15 One reader per consuming feed.** `buildUiTelemetry` is the only caller of `frames.read()`, `takeOutPeaks`, `takeScPeaks` and the steal-ring cursor. Closure [12]'s "other readers" pattern extends to `takeOutPeaks|takeScPeaks|stolenCount|takeOutPeak\b|takeScPeak\b`; the harness reads only through its own processor instance on a scripted clock.
- **PD16 Page defect (D48): the LFO dot.** In `drawLfos` the dot's Y becomes `lfoShapeAt (shape, (pp + ph0) % 1)`; X stays `pp / 2`. Lands in `index.html` **and** `mockups/v1-ui.html` (`cmp` clean). No other page edit except a defect a gate finds (recorded, mirrored, with the failing gate as its reason). No i18n key is added or changed (every 3.3 key exists in en / fr / zh-Hans).
- **PD17 [R§12.8] CM6 = take naming (D49b).** In `SlotLoader::runCommit` the take file is `dir.getChildFile (base).getNonexistentSibling (false)` **before** the write branch, so a failed take's intended path is unique too (on a missing directory the base name is already unique). The stamp format stays `%Y%m%d-%H%M%S` (no ms; nothing parses it, and a working name stays as shipped). Test hook `legacyTakeName` (`OGI_TEST_HOOKS`, `SlotLoader` hooks) re-enacts the sibling-inside-branch order for the control. CM6 itself is unchanged and stops depending on CM5's wall second.
- **PD18 [R§12.9] i18n states (D49a + new).**
  - Delete `rec-wait-level` and `rec-no-sidechain` (their keys are measured live by `rec-wait-level-live`, `rec-no-sidechain-live`, `rec-capped-live`). `rec-wait-level`'s drag-over cleanup moves into the next surviving state's setup; `#heavy.on` needs no move (default index 3) — the next state asserts it is on.
  - Replace `rec-recording` **in place** by `streams-2-4-on`: the same three enable clicks at the same sequence position (every later state sees the same page as before, minus the vacuous label), with an inline check that the clicks landed (`#fxcount-s2` no longer carries `label.streamOff`). It measures no new key; ST1 skips it (no named target).
  - Add `rec-recording-live` after `rec-armed` through `__giState` (`c.rec ({ run: 2, slot: 4, pos: 0.3 })` → `#rec-status-t` = `rec.recording`).
  - Append at the end of the file (no later state is affected), through `__giState` with `c.tel` stub edits, VOICE page first (`drawVoices` returns early elsewhere): `voice-held` (`vstate.held`; header `label.voicesHeld`), `voice-release` (`vstate.release`; `label.voicesState`), `voice-stolen` (`vstate.stolen`), `voice-clamp` (`vstate.clamp` with `{ s: '+48' }`), `menv-newest` (`label.newest`), `write-held` (`label.writeHeld`), `write-per-lap` (`label.writePerLap`); then STREAMS page: `stream-off-head` (s2 enable clicked off → `#fxcount-s2` = `label.streamOff`) and `lane-info-off` (`c.rec ()` so no take is running, s2 off → `#lane-info-2` = `lane.infoOff`). Lane 2 cannot be used before that: the fixture records into it (`run 2, slot 2`), so its info line reads `lane.recording` until `c.rec ()` clears it.
  - 38 − 3 + 1 + 1 + 9 → **46** states; the check-ui-labels banner must equal the file's count. Each state throws if its node is not visible or does not show its key.
- **PD19 UPM6b printed note (D49c).** `ui_gates_31.h:538` "(UPM6: a `slots` array clears unlisted slots)" → "(UPM6: a preset's `slots` array leaves unlisted slots alone; a session's clears them — D45)". Text only; the predicate is right.
- **PD20 Carried rows amended (names unchanged, each PASS, each a recorded, planned deviation).**
  - **UTL32** keeps proving the 3.2-owned contract in the 3.3 encoder (lanes `ovRev / fb / rec`, null-ness, grain rows of 8, counts, the rec scalars, 4 decimals); its 3.3-owned asserts (row count, `newest`, `menv`, `sources`, `limit`, `out`, `modNorm`, `lfoPh`, peak equality) move to UTL33. Title tail drops "voices 20 x 8".
  - **UST1:** `== 20` → `== UiTelemetry::kVoices` (16).
  - **UCN1:** "limit 0 on every frame" → "limit == 32 << grain_limit on every frame" (the row switches 256 → 32 mid-run, so both values are seen).
  - **UPM6b:** detail text (PD19).
  - Fixture self-check `main.cpp:1454`: 16 rows and the 3.3 fixture (not a row).
  - **UTL31, UGD1, UPH1 are not edited.** UGD1 (`dot.voice == vg.voice`) and UPH1 (row k ↔ slot k) hold under PD1 because their voices take rows in note-on order; if either fails, STOP: that is a map defect, not a row to amend.
  - Row **TL2** (`engine_gates_26b.h:1006-1042`) re-read for a frame-size assumption; touched only to keep it compiling (recorded).
- **PD21 Harness rows `tests/render-harness/ui_gates_33.h`** (processor only, `U` prefix, names grepped for collisions first; `--ug33`; `ug33::runAll()` after `ug32` in the full run, existing order unchanged). Every row asserts its stimulus fired before its result; every `n` control runs the SAME predicate on a deliberately broken input and must FAIL it.

  | Row | Checks | Control |
  |---|---|---|
  | UVR1 | 16 rows; every counted voice has a row; a voice keeps its row for its life; polyphony 16 with steal spares in use → a voice in engine slot 16–19 is shown (stimulus: assert a voice really sat in slot ≥ 16) | UVR1n identity map (row = slot) → that voice missing |
  | UVR2 | `newest` = the row of `newestSer` across note-ons, releases, steals, mono / legato; −1 after All Sound Off while a stolen row holds | UVR2n page-style guess (highest row with a state) → mismatch after a steal |
  | UVS1 | polyphony 4, hold 4 notes, play a 5th; polls at 33 ms: victim row state 3 for ≥ 1 poll and ≤ 300 ms + 1 poll, then empty; new voice in its own row | UVS1n hold 0 → state 3 never seen |
  | UVS2 | release → state 2, empty after the env ends; CC64 keeps state 1 after key-up | — |
  | UVS3 | polyphony shrink, All Sound Off and a mono handover leave `stolenCount` unchanged; each real steal adds exactly 1 | — |
  | UWS1 | note 50 st above root: column 8 = 50 (page → `vstate.clamp`), grain bit 2 set on that stream only; a stream with key track 0 → bit clear | UWS1n stream 0's value applied to every stream → bit on the wrong stream |
  | UME1 | `menv` levels / stages == `getVoiceModForTest (k).env[e]` and `getVoice (k).env` for a scripted ADSR, through the map | — |
  | ULF1 | `lfoPh` global == `getLfoPhaseTelemetry`; per-voice == `vm[k].lfo[n]` expression; −1 for Global-mode LFOs and empty rows | — |
  | USRC1 | `sources[i]` == `getSourceTelemetry (i + 1)`; Key / Bend / Slide signs; values held when silent | — |
  | UMN1 | key set == routed set; values == `destTel`; a per-voice route follows the newest voice and rests at base with no voice; a global source moves it without notes | UMN1n all 125 keys sent → extra keys |
  | ULH1 | `lo / hi` present only on a lane with a routed loop dest; == min / max of the two `destTel` | ULH1n always emitted → the unrouted case fails |
  | UMB1 | 17 × 125 pairs × LFO mode {Global, Per-voice}: `srcPerVoiceNowForTest && ! destVoiceNowForTest` == a C++ transcription of the page rule (I:1855, 3077-3085); closure [16] pins the transcription's constants to the page text | — |
  | UMT1 | each poll's `out[L] / out[R]` == the harness's own `max |x|` of the output channels since the previous poll, bit-exact (a held note, two streams panned apart); if the processor alters the buffer after `Engine::process`, compare against the engine output and record it | — |
  | UMT2 | one stream, `s1_pan` −100, `s1_stereo_spread` 0, `reverb_mix` 0: `out[R] / out[L]` ≤ −60 dB, `out[L]` > −40 dBFS | UMT2n `[max, max]` → FAILS |
  | UMT3 | SC sine at fs/4 phase 0 on L, R silent, `rec_gain` 0 dB, A = 0.5 and 0.1: `rec.peak[0]` = A ±0.01 dB, `rec.peak[1]` = 0; 997 Hz at 0.5: within [A·cos (π·997 / fs), A] | — |
  | UMT4 | two polls after one block: the second reads 0 on both feeds (out and SC) | UMT4n a read without the exchange → second read non-zero |
  | ULM1 | `limit` = 32 / 64 / 128 / 256 for indices 0–3 | — |
  | UCM6b | recordings dir pre-seeded with `GI-Rec-<stamp>-S1.wav` for now − 1 … now + 5 s; a `failWav` take → its `rec:` source exists in no file, differs from every seeded name; a succeeding take beside it → also distinct; all `rec:` sources of the row distinct | UCM6bn `legacyTakeName` → the failed take names a seeded file |
  | UAL2 | 0 audio-thread allocations with 256 grains, 16 voices, steals (ring writes), per-voice LFOs, both peak splits publishing | the liveness malloc control fires |
  | UTL33 | `telemetryToVar` 3.3 shape (PD13) through JSON text: 14 keys exact, 16 × 8 voices, 16 × 5 menv, 4 × 17 lfoPh, 17 sources, `out` 2, `rec.peak` 2 (L ≠ R allowed), modNorm keys numeric "1"…"125", `lo / hi` only when present, `frozen / modRouted / stale` absent, 4 decimals | — |
  | INFO UPL2 | widest JSON (16 routes, 256 grains, 16 rows) bytes + build / encode / text cost per pull; > 1 ms reported, not accepted silently | — |
  | INFO UPB2 | `publishTelemetry` cost per block at 20 voices × 4 streams before / after the voice extension (re-enactment) | — |

  Expected harness total **402 + new rows**; the executor records the number.
- **PD22 Overrides fixture (3.3).** `--dump-ui-overrides` regenerated with a 3.3 fixture: revisions / `ovRev` / `fb` / live overview / `dirty` stay real (3.2 run, pinned seed); written by hand into `UiTelemetry`: row 0 held (newest), row 1 released, row 2 stolen-held, `menv` for the three, 17 sources with signs, `modNorm` for two routes (an LFO on lane 1's loop end, Env 2 → `output_gain`), `lfoPh` with LFO 1 per-voice, `lo / hi` on lane 1, `out` [0.5012, 0.1], `rec.peak` [0.25, 0.05], `limit` 256, one grain with bit 2. LF, no clock, no absolute path, twice byte-identical, committed, diff-clean on every gate run.
- **PD23 Closure (`ui_frontend_check.js`).** [12] and [15] amended per PD14 / PD15. New **[16] Phase 3.3 closure**:
  - "emitted and read": `newest` (drawVoices / drawMenvFigure / drawLfos / drawModSources), `menv` (drawAmpFigure / drawMenvFigure), `sources` (drawLfos / drawModSources / updateLiveModulation / drawVoices), `modNorm` (modulatedNorm), `lfoPh` (drawLfos), `out` (ingestTelemetry), `limit` (updateReadouts), `lo / hi` (drawLane), row column 8 (drawVoices `v[7]`), voice states 2 / 3 (drawVoices), grain bit 2 (drawLane);
  - "not emitted": `frozen`, `modRouted`, `stale`;
  - the page's `PER_VOICE_SOURCES` == `[5, 6, 7, 8, 9, 11, 12, 13]` and `isGlobalDest` ranges == `1`, `114–121`, `122–125` by LFO mode (the UMB1 transcription cannot drift from the page);
  - the LFO dot uses the phase offset (`(pp + ph0) % 1`, PD16);
  - **class gate (D49a):** no state in `i18n-states.json` calls `__setLabel` on a node a page reader writes; the list of reader-written ids is derived from the page (every `setLabel (<node>` target inside the poll / rAF functions), not hard-coded to the three.
  - Existing NCs keep failing; new NC: `lfoPh` removed from the encoder FAILS [16].
- **PD24 Probe rows (`ui_binding_probe.js`).** Counts unchanged (261 × 2, VE 14, lanes 12 / 12). New `--mod-only` group on per-row stub telemetry:
  - **VC1** cell / dot classes and `vstt-*` keys for states 0–3; `newest` class only on the counted newest row; **VC2** column 8: +50 → `vstate.clamp` `+48` + `.clamp`, +12 → "+12 st"; **VC3** header `label.voicesHeld` vs `label.voicesState` counts;
  - **MV1** chip glow from `sources` (bipolar mapping), LFO / source read-outs, key view newest colour (pixel); **MV2** ring dot at `modNorm` (pixel / transform) on a routed knob, base when unrouted; **MV3** loop shading from `lo / hi` and the edge tick from `modNorm` (pixel); **MV4** LFO dot on the curve at `lfo1_phase` 90° (pixel, ± 2 px); **MV5** `mrow-N.mono-on` for a matrix of source × dest × LFO mode; **MV6** menv dot on its envelope figure at the stage position and `label.newest` vs `label.idle` (the dot-on-curve class beside MV4);
  - **DM1** drag-to-modulate: each of the 17 dock chips onto one knob and one loop handle by a real pointer sequence (pointerdown on the chip → `page.mouse` moves → pointerup on the target), never a direct `addRoute` call; the matrix is reset (all `modM_source` None) before each drop; reads `modM_source` = chip, `modM_dest` = `data-dest`, `modM_depth` = 40; **DM2** every `.knob-cell[data-dest], .loop-h[data-dest]` (I:3170) once with one chip, page by page; census == the runtime count of that selector (recorded);
  - **MT1** bar tops for `out` [0.5012, 0.1] and `rec.peak` [0.5, 0.1] at `(dB + 48) / 54` of the height, ± 1 px, L and R independent; **HV1** `#heavy.on` at `grain_limit` 3, off at 2;
  - **ST1 (class gate, D49a)** replays `i18n-states.json` in file order on the stub page; every state that names a target (`__giState (name, sel, key, …)` or an inline assert) shows its key at +150 ms **and** at +450 ms (≥ 10 polls of the live stub telemetry).
  - **NCs:** pre-fix LFO expression → MV4 FAILS; one target's `data-dest` removed → DM2 census FAILS; `out` sent `[max, max]` → MT1 FAILS; the old `rec-recording` (`__setLabel` form) inserted into a copy of the state file → ST1 FAILS.
- **PD25 [R§12.11] Meter gate reading.** As the Sign-off default (UMT1–UMT4 + MT1).
- **PD26 Gate order:** regenerate overrides (diff-clean) → `ui_frontend_check.js` → `ui_binding_probe.js` → check-i18n → fr-lint → zh-lint → check-ui-labels → boot-all-uis `--strict-tips`. Exit 77 = not run = stop.
- **PD27 Hands-on (D47).** Execute is not gated on any hands-on file. T14 writes `HANDS-ON-3.3.md` (3.3 rows) and `HANDS-ON-SESSION-3.3.md` (the one human session at 3.3 verify: run order 2.1–2.6 (79) → 3.1 (16) → 3.2 (27) → 3.3 → Logic AU pass; owner phase per file for routing a ❌ to `/plugin-execute O-GranulatorInstrument 3-gui` or the 2-dsp phase; build / install / cache-clear steps; one results table).
- **PD28 Housekeeping.** Version **0.1.0**; **no tag**. No new params; param-dump byte-identical. No edit to `forked/*`, `modules/*`, `scripts/*`, contracts, `CMakeLists.txt` (no new source file in a target; `ui_gates_33.h` is included by `main.cpp`). `PluginProcessor.h` stays free of WebView types. Test hooks under `OGI_TEST_HOOKS` only. No `juce::Timer`, no `emitEvent`; no listener writes a parameter (drag-to-modulate stays 3.1 gesture code). Four path-scoped commits C1–C4 on `main`; `git branch --show-current` + `git status --short` immediately before each; never `git add -A`; foreign dirty paths (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, `plugins/O-GrainScatter/*`) never staged; `Source/ui/public/modules/` never committed.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/dsp/GiTelemetry.h` | `GiUiVoice` fields, `GiUiFrame::newestSerial` (PD3); `outPeakL / R` + `takeOutPeaks` (PD5); steal ring (PD2) |
| `Source/dsp/Engine.h/.cpp` | voice-loop writes + `newestSerial` in `publishTelemetry` (PD3); L / R `pushMax` (PD5); ring write in `stealVoice` (PD2); two test accessors (PD3) |
| `Source/dsp/GiRecorder.h` | `pkL / pkR`, `scPeakL / R`, `takeScPeaks` (PD5) |
| `Source/dsp/SlotLoader.h/.cpp` | sibling before the branch + `legacyTakeName` hook (PD17) |
| `Source/PluginProcessor.h/.cpp` | `GiVoiceRowMap`, steal cursor, `UiTelemetry` 3.3 fields, `buildUiTelemetry` 3.3 (PD1, PD2, PD4–PD12, PD15) |
| `Source/GiUiJson.h` | 3.3 encoder; the 3.1 placeholder untouched (PD13, PD14) |
| `Source/PluginEditor.cpp` | header / `getTelemetry` comments only (one builder; the consuming feeds it owns) |
| `Source/ui/public/index.html`, `mockups/v1-ui.html` | PD16 (identical) |
| `tests/render-harness/ui_gates_33.h` (new), `main.cpp`, `ui_gates_32.h`, `ui_gates_31.h` | PD19–PD22 |
| `tests/ui-stub/generic-overrides.json` | regenerated (PD22) |
| `tests/{ui_frontend_check.js,ui_binding_probe.js,i18n-states.json}` | PD18, PD23, PD24 |

---

## Tasks

**Roles.** **[agent]** = gui-agent writes files. **[run]** = builds, gates, validators, commits. **BG** = `run_in_background`, log to `$S/<name>.log` (`$S` = the executing session's scratchpad); anything over ~5 min runs BG (600 s watchdog). `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. Paths relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-neutral checkpoint (after T2 and after T3 — one per task, not batched: both touch the block path).** Build the harness (BG); `$H --dump-render-hashes "$S/ck-<T>.sha256"` == `tests/golden/renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`; param-dump == `tests/golden/params.tsv`. A move → STOP, attribute to that task. Renders are never re-pinned.

### Wave 0 — pre-flight

1. **T1 — pre-flight + baselines** [run]. Output `$S/preflight.txt`; any mismatch → STOP and ask.
   - Contracts re-hashed against STATUS frontmatter; `git log --oneline 399fd122..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` empty; branch `main`; `git status --short` recorded.
   - `cmp` `index.html` / `v1-ui.html` and `js/i18n.js` / `v1-i18n.js`. Re-grep every Plan-time fact symbol; record drift.
   - Baselines on the unmodified tree: harness **402 / 0** (BG; carried extract = sorted `[PASS]/[FAIL]` rows without timing detail = **B**), parity 168 / 0 + GV 2 / 0, render hashes, param-dump, fork drift 0 / 13.
   - UI baseline: overrides regenerate diff-clean; closure 128; probe 261 × 2 + VE 14 + lanes 12 / 12; five gates green (`default + 38`).
   - Proposed row names (PD21) grepped across `tests/render-harness/*` → 0 hits each. `node -e "require('playwright')"` resolves. Sign-off item: record whether Taylor answered; proceed on the default.
   - Depends on: none

### Wave 1 — engine, recorder, loader, processor (audio-neutral)

2. **T2 — voice frame, steal ring, output peaks L / R** (PD2, PD3, PD5 engine half) [agent]. Checkpoint.
   - Files: `Source/dsp/GiTelemetry.h`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`
   - Depends on: T1
3. **T3 — sidechain peaks L / R + CM6 take naming** (PD5 recorder half, PD17) [agent]. Checkpoint (taken after T2's, so a move is attributable).
   - Files: `Source/dsp/GiRecorder.h`, `Source/dsp/SlotLoader.h`, `Source/dsp/SlotLoader.cpp`
   - Depends on: T1 (files independent of T2; may be written beside it)
4. **T4 — processor surface + 3.3 encoder** (PD1, PD2 reader, PD4–PD13, PD15) [agent]. Message thread only; no block-path change (covered by T6).
   - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/GiUiJson.h`
   - Depends on: T2, T3
5. **T5 — harness rows, carried amendments, overrides** (PD19–PD22) [agent + run]. `ui_gates_33.h`, `--ug33`, the 3.3 fixture; amendments to UTL32 / UST1 / UCN1 / UPM6b text; overrides generated twice, byte-identical.
   - Files: `tests/render-harness/ui_gates_33.h`, `tests/render-harness/main.cpp`, `tests/render-harness/ui_gates_32.h`, `tests/render-harness/ui_gates_31.h`, `tests/ui-stub/generic-overrides.json`
   - Depends on: T4
6. **T6 — neutral proof + commit C1** [run]. Harness full set (BG): carried extract diff vs **B** empty except UTL32 / UST1 / UCN1 (amended, same names, PASS) and UPM6b's detail text; CM6 PASS; new rows PASS; every `n` row FAILS its predicate; 0 NOT FIRED; parity 168 / 0 + GV 2 / 0; renders + legacy byte-identical; param-dump identical; drift 0 / 13. The editor still compiles. Commit path-scoped.
   - Depends on: T5

### Wave 2 — page and validators

7. **T7 — LFO-dot defect + editor notes** (PD16) [agent]. Both HTML files identical; `PluginEditor.cpp` comment lines name the consuming feeds `buildUiTelemetry` owns (frame, out L / R, SC L / R, steal ring); no code change in the editor.
   - Files: `Source/ui/public/index.html`, `.planning/mockups/v1-ui.html`, `Source/PluginEditor.cpp`
   - Depends on: T1 (prepared beside Wave 1, applied after C1)
8. **T8 — build, install, validate + commit C2** [run]. Build VST3 + AU + Standalone + consoles, **0 warnings in plugin sources**. `./scripts/build-and-install.sh O-GranulatorInstrument` (cache clear, dual-variant sweep); `auval` PASS; pluginval 10 VST3 + AU with the sidechain on, then `OGI_GATE_SC_DEFAULT_OFF` (PLAN-2.6 W7 procedure), shipping build restored + reinstalled; X2 = 0. Standalone smoke (window capture only, never a region): open the MOD page and the VOICE page with no input; read both shots (no page error, meters at rest, heavy hint on). Commit.
   - Depends on: T6, T7

### Wave 3 — UI proof

9. **T9 — closure [12] / [15] / [16]** (PD14, PD15, PD23) [agent + run]. Old and new NCs.
   - Files: `tests/ui_frontend_check.js`
   - Depends on: T8
10. **T10 — probe rows** (PD24) [agent + run]. `--mod-only`, VC1–VC3, MV1–MV6, DM1–DM2, MT1, HV1, ST1 + four NCs.
    - Files: `tests/ui_binding_probe.js`
    - Depends on: T8
11. **T11 — i18n states** (PD18) [agent + run]. 38 → 46.
    - Files: `tests/i18n-states.json`
    - Depends on: T8
12. **T12 — full gate run + commit C3** (PD26) [run]. All green with exit codes recorded; ST1 run against T11's file; a new collision or fit failure against T1's baseline is a D48 defect: fixed in both HTML files, the sequence re-run, the install refreshed. Commit.
    - Depends on: T9, T10, T11

### Wave 4 — regression + docs

13. **T13 — regression on the final tree** [run]. Harness full set (BG) vs **B** + new rows; parity; renders + legacy; param-dump; drift 0 / 13; X2 0; **`--cpu26` once (BG): C25m ≤ 40 %, C25tm ≤ 12 %** (the publish loop grew); auval; pluginval 10 VST3 + AU, sidechain on and off; final install of the shipping build; `auval -a | grep -i granulatorinstrument`; `cmp` both pairs.
    - Depends on: T12
14. **T14 — docs + commit C4** [agent + run].
    - `HANDS-ON-3.3.md` (run at 3.3 verify, D47): each of the 17 chips onto a knob and onto a loop handle; rings and the edge tick move with a global LFO (no notes) and with Env 1 on a held note, resting at base after release; mono badge on Env 2 → Output and on LFO 2 per-voice → reverb mix; Alt-drag depth; voice strip with a chord past polyphony 8 (stolen cell visible about 0.3 s, new note in its own cell) and at 16 (note change only, accepted limit PD1); release tails and CC64; key view and newest marker follow the last note; Env scopes and read-outs (`newest` / idle); LFO dots, one per voice, on the curve at phase 90°; a note far above root → `+48` clamp and warn-coloured grain dots on that stream; output meter L / R with a hard-panned stream; SC meter L / R with Logic's side-chain routed, slashed with no bus; loop shading follows a routed loop edge; grain caption `n / limit` and heavy hint at 128 off / 256 on; fr and zh-Hans pass over MOD + VOICE; known limits listed (meters accumulate while the editor is closed; `wantSt` excludes `pitch_rand` and scale picks).
    - `HANDS-ON-SESSION-3.3.md` (PD27) with the Logic AU pass (ROADMAP 3.3): side-chain routing from an audio track, Record-in Manual / Threshold / Next Note into each slot, unrouted side-chain → No signal / discard, project save / reopen restores takes, the SC meter during an idle instrument track reads 0 (expected, R§10).
    - `SUMMARY-3.3.md` (complex template): rows, counts, NC results, UPL2 / UPB2 numbers, `--cpu26`, deviations (incl. PD20's planned amendments and PD14's departure), open items.
    - STATUS.md → `stage_3_phase_3_3_execute_complete`, next `/plugin-verify O-GranulatorInstrument 3-gui`. REQUIREMENTS untouched (UI-02 moves at verify).
    - Depends on: T13

---

## Dependency graph

```
T1 ─┬─ T2 ─┐
    ├─ T3 ─┴─ T4 ── T5 ── T6 ─┐
    └─ T7 ────────────────────┴─ T8 ─┬─ T9 ──┐
                                     ├─ T10 ─┼─ T12 ── T13 ── T14
                                     └─ T11 ─┘
```

Checkpoint order: T2's before T3's (shared build dir; a render move is attributed to one task).

---

## Commit split

| Commit | Content | Tasks |
|---|---|---|
| C1 | engine / recorder / loader / processor surface + 3.3 encoder + `ui_gates_33` + carried amendments + overrides | T2–T6 |
| C2 | LFO-dot page defect (both HTML files) + editor notes; validators green | T7–T8 |
| C3 | closure [12] / [15] / [16], probe rows, 46 states (+ any gate-found D48 fix) | T9–T12 |
| C4 | HANDS-ON-3.3, HANDS-ON-SESSION-3.3, SUMMARY-3.3, STATUS | T14 |

---

## Gate criteria (3.3)

| Gate | Pass |
|---|---|
| Harness | carried extract diff vs B **empty** except UTL32 / UST1 / UCN1 (amended, same names, PASS) and UPM6b's text; CM6 PASS; new `ui_gates_33` rows PASS; every `n` row FAILS its predicate; 0 NOT FIRED |
| Parity | 168 / 0 + GV 2 / 0 |
| Renders | renders-2.4 + legacy byte-identical (after T2, T3, at T6 and on the final tree) |
| param-dump | identical (261) |
| Fork drift | 0 / 13 |
| Allocation | UAL2: 0 on the audio thread (extended frame, steals, peak splits); UAL1 unchanged |
| CPU | `--cpu26`: C25m ≤ 40 %, C25tm ≤ 12 % |
| One reader | `buildUiTelemetry` the only caller of `frames.read`, `takeOutPeaks`, `takeScPeaks`, the steal cursor (code + [12]) |
| Test directories | nothing written under the real `~/Music/Ouaricon/O-GranulatorInstrument/` by any automated step (UCM6b uses the redirected Recordings dir) |
| Build | VST3 + AU + Standalone + consoles, 0 warnings in plugin sources |
| auval; pluginval 10 VST3 + AU | PASS / SUCCESS, sidechain on and off |
| X2 | 0 test-hook symbols in shipping binaries (`legacyTakeName`, `…ForTest` included) |
| `generic-overrides.json` | twice byte-identical; diff-clean vs committed |
| `ui_frontend_check.js` | 0 failures; 176 / 70 / 15; 34 = 34; [12] on the live encoder; [16] green; all NCs FAIL |
| `ui_binding_probe.js` | 261 / 261 × 2; VE 14 / 14; lanes 12 / 12; VC / MV / DM / MT / HV / ST rows PASS; DM2 census == runtime count; NCs FAIL |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 |
| check-ui-labels | 0; banner `(261 rows) + overrides`, `states: default + 46` |
| boot-all-uis `--strict-tips` | completes, 0 DEAD, 0 late |
| `cmp` | `index.html` == `v1-ui.html`; `js/i18n.js` == `v1-i18n.js` |

Exit 77 anywhere = not run = not a pass.

---

## Risks / watch items

- **Row index ≠ engine slot.** `voices`, `menv`, `lfoPh` rows, `newest` and grain `voice` go through the same map in the same poll; mixing one engine index in misplaces dots silently. UVR1 / UME1 / ULF1 compare through the map; UVR1n proves the identity map loses a voice.
- **Steal ring overrun.** More than 16 steals between two polls lose the oldest; the reader's re-read of `stolenCount` stops a half-written entry from becoming a phantom hold. Only `stealVoice` writes (UVS3).
- **The block path is touched twice** (publish loop + steal site; recorder peak split). Each has its own checkpoint; nothing read back by the DSP. The publish loop grows by up to 80 `voiceSemis` calls per block: UPB2 measures, `--cpu26` judges.
- **Carried rows.** UTL32 / UST1 / UCN1 change on purpose (PD20); any other carried diff is a STOP. UGD1 / UPH1 passing depends on note-on-order rows — a failure there is a map defect.
- **Vacuous rows.** UVR1 needs a voice really in slot ≥ 16; UVS1 needs `voiceSteals > 0`; UMT2 needs a loud L; UCM6b needs the seeded collision present before the take. Each asserts its stimulus first.
- **Wall clock in UCM6b.** The take stamp reads the real clock; seeding now − 1 … now + 5 s makes the collision certain whatever second the commit lands in.
- **State order (PD18).** `__giState` exists only after `rec-armed`; the stream clicks stay where they were; new states go to the end. ST1 replays the file in order, so a state that only passes because of an earlier state's leftovers still has to hold on its own node.
- **Stub probe ≠ real WebView / host.** Pointer capture across pages, Logic's side-chain routing, DPR 2 pixel rows, the 0.3 s steal hold read by eye: hands-on only.
- **D48.** Any page change beyond PD16 needs a failing gate as its reason.

## Deviations protocol

Record every departure in SUMMARY-3.3 (what, why, evidence, which gate). PD14 and PD20 are planned and recorded as such. Stop and ask only for: an audio-moving change, a contract edit, a module or `scripts/` change, a forked-file edit, a v1 design change beyond a defect (D48), a carried-row change outside PD20, or a steal-visibility design that needs a timer or a push.

## Carried (not 3.3)

Stage 4 (D50): `WavFailed` surfacing after a take, one toast element for several failing lanes, a save worker joined without timeout on a hung disk, `output_gain` default 7.15e-07 dB, Windows drop / browser check, blobs in presets, factory presets, float WAV for hot takes. fr `vstate.held` "tenue": HANDS-ON-3.2 row 26. All hands-on files + the Logic AU pass: 3.3 verify (D47).

## Success criteria (gate for verify)

- [ ] The MOD page shows the 17 sources from `srcG`; chip glow, read-outs, rings and loop-edge ticks agree with the engine (USRC1, UMN1, MV1–MV3)
- [ ] Drag-to-modulate writes source / dest / depth from every dock chip onto a knob and a loop handle, and every drop target accepts a drop (DM1, DM2 census == runtime count)
- [ ] Mono badge rule equals the engine's per-voice × global predicates for every pair and LFO mode (UMB1, MV5, [16])
- [ ] Rings move for per-voice routes from the newest voice and rest at base with no voice (UMN1, MV2)
- [ ] Voice strip: 16 rows keyed by serial, slot 16–19 voices shown, releases as state 2, steals visible ≥ 1 poll and ≤ 300 ms + 1 poll, `newest` from the engine's rule (UVR1–UVS3, VC1–VC3)
- [ ] Env scopes, LFO views (dot on the curve at any phase), `wantSt` clamp marker and grain warn bit live (UME1, ULF1, UWS1, MV4, MV6, VC2)
- [ ] Output and SC meters L / R, max since read, one reader, known SC levels exact, output bit-exact, bars at the mapped height (UMT1–UMT4, MT1)
- [ ] `limit` truthful; heavy hint at 256 on / 128 off (ULM1, HV1)
- [ ] D49: no vacuous state (ST1 + [16] class gate), CM6 collision-proof (UCM6b + NC), UPM6b note in D45 wording
- [ ] 0 audio-thread allocations; Stage 2, 3.1 and 3.2 unchanged apart from PD20: harness carried diff, parity, renders, param-dump, drift, X2, `--cpu26`, 261 × 2, VE 14, lanes 12 / 12
- [ ] All five UI gates green with 46 states; both file pairs `cmp` clean
- [ ] HANDS-ON-3.3.md and HANDS-ON-SESSION-3.3.md ready for the one session at verify (D47)
