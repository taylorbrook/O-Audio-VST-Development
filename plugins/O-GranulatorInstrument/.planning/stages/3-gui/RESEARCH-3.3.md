# O-GranulatorInstrument — Stage 3 (GUI) — Phase 3.3 — RESEARCH

**Researched:** 2026-11-10
**Scope:** Phase 3.3 only (D47–D50, closes UI-02): MOD sources, drag-to-modulate proof, mono badges, mod rings from the newest voice, the voice / amp section, the voice strip, the output and sidechain meters (max since read), the `grain_limit` heavy hint, and the three 3.2 carry-overs (D49).
**Tree:** `main` at `ce413b23`. Parent O-Granulator at its current head.
**Confidence:** HIGH on code facts (read this session, `path:line`). MEDIUM on the designs in §3–§9 (designed here, not built). Nothing was built or run; the page was not opened. Each claim is **VERIFIED** (read) or **INFERRED** (reasoning).
**Abbreviations:** I = `Source/ui/public/index.html`; T = `.planning/mockups/v1-PluginEditor.cpp`; P = `plugins/O-Granulator`; D = `Source/dsp`; J = `Source/GiUiJson.h`; PP = `Source/PluginProcessor.cpp`.

**Inputs read:** `CONTEXT-3.3.md`, `RESEARCH-3.2.md`, `VERIFICATION-3.2.md` (§ carry-overs), `SUMMARY-3.2.md` (dev. 1), ROADMAP 3.3, `parameter-spec.md:54-59`, `v1-ui.yaml` (S1, heavy hint); J (3.1 + 3.2 encoders); PP `buildUiTelemetry` (`:1172-1325`); `PluginProcessor.h:360-450`; `D/GiTelemetry.h`; `D/Engine.cpp` (`publishTelemetry :752-823`, `updateNewest` / `writeTelemetry :1700-1786`, `allocateVoice :2423`, `forceReleaseVoiceMs :2628`, `stealVoice :3019`, `voiceSemis :287`, `destVoiceNow / srcPerVoiceNow :1891-1899`); `D/Engine.h:515-530, 915-923`; `D/VoiceAllocator.h`; `D/AmpEnv.h`, `D/GiVoiceMod.h` (stages); `D/GiModMatrix.h` (`GiSrc`, `GiDest`, `writeTelemetry :353-374`); `D/GiRecorder.h` (peak, `:296-376, 427, 571`); `D/SlotLoader.cpp:1085-1125` (take naming); I (constants `:1848-1856`, rings `:2550-2602`, mono `:3075-3100`, heavy `:2989-2992`, ingest `:3895-3911`, figures `:4180-4225`, lanes `:4229-4340`, meters `:4341-4373`, LFOs `:4386-4415`, sources `:4424-4458`, voices `:4461-4518`, readouts `:4533`, live mod `:4577-4590`); T `:340-350, 590-710`; P `PluginProcessor.cpp:680-745`; `tests/i18n-states.json` (38 states); `tests/ui_frontend_check.js:639-700` ([15]); `tests/render-harness/engine_gates_26a.h:884-925` (CM5 / CM6); `ui_gates_31.h:538` (UPM6b).

---

## Summary (what the plan must absorb)

1. **The page shows 16 voices; the engine has 20 slots.** `vdot-0…15`, `vcell-0…15`, `drawVoices` loops `i < 16` (I:4467, 4492); T sends 16 rows "counted voices shown in the UI (steal spares are not)" (T:344). The engine's free-slot search is first-free in identity order (`VoiceAllocator.h` `findFree`), and a stolen voice keeps its slot while it fades, so at full polyphony the new note lands in slot 16–19 and is **invisible**. The `newest`, `menv`, `lfoPh` and voice-row indices are all page-row indices. A **display-row map** (engine serial → row 0…15) is needed in `buildUiTelemetry`. §3. **VERIFIED** (gap) / **INFERRED** (fix). **[decision for plan]**
2. **A steal lasts 5 ms; the poll is 33 ms.** `forceReleaseVoiceMs` marks the victim `stolen` with `kStealMs` (`Engine.cpp:2628-2639`); the frame carries only counted voices (`note = −1` otherwise, `Engine.cpp:805`). A steal is visible today only by luck. Recommended: a sticky steal ring in `GiTelemetry` written by `stealVoice` (the only true steal site; `forceReleaseVoice` is also used for polyphony shrink and mono modes) and a 300 ms row hold in the reader, as `GiRecRunHold` does for run 4 / 5. §3. **VERIFIED** / **INFERRED.** **[decision for plan]**
3. **The engine already computes every MOD value; only the frame and encoder lack them.** `srcTel[1…17]` (= `srcG`, the newest-voice ramped value, `Engine.cpp:1778`), `destTel[1…125]` (destination-normalised 0…1 through the APVTS range, per-voice part from the newest voice's `offCur()`, `GiModMatrix.h:356-374`), `lfoPhaseTel[4]`. The page's source indices match `GiSrc` exactly (1 LFO 1 … 17 Macro 4) and its polarity list `[1,2,3,4,8,12,13]` matches the engine's ranges (Key = (note − 60)/64, Slide = (CC74 − 64)/64, Bend ±1). Per-voice data the page reads (Env 1 / 2 level + stage, amp stage, per-voice LFO phases, `wantSt`) is **not** in `GiUiVoice` and needs a frame extension. §4. **VERIFIED.**
4. **`modNorm` = the parent's routed-only fill, unchanged.** Routed = any slot with `source > 0` and `dest == d` (the page's `routesFor` also ignores depth). `destTel[d]` is already in `placeRingDot`'s units (it uses `setDestRange (d, f->getNormalisableRange())`, `PP:235`, the same range the WebSlider normalises with). The loop-handle "reach" tick in `drawLane` reads `modNorm` for dests 27/28, 55/56, 83/84, 111/112 (I:4326-4331), so loop handles are covered by the same key. §5. **VERIFIED.**
5. **Meters: split both peaks into L / R; the audio path cannot move.** `takeOutPeak()` is `max (|L|, |R|)` (`Engine.cpp:756-759`), the recorder's `scPeak` the same (`GiRecorder.h:296, 375`). Both are telemetry-only accumulators; splitting them changes no audio sample (renders stay byte-identical by construction). The parent did exactly this (`outPkL / outPkR`, P:718-721). Sending one value twice would leave the two bars locked together, which the v1 page draws as stereo. §6. **VERIFIED** / **INFERRED.**
6. **The steady-state answers are small:** `limit` = the `grain_limit` value (32 · 2^index; the heavy hint stays parameter-driven, S1 "visible when value > 128", `v1-ui.yaml:214`); `frozen` is **not read** by the page (`#strip.frozen` comes from the `freeze` toggle, I:2984-2987) → do not emit it; `lfoPh` = `[global, row0…row15]`. §7. **VERIFIED.**
7. **CM6 is a production bug, not only a flaky row.** On the WAV-fail path the take keeps the *intended* name without `getNonexistentSibling` (`SlotLoader.cpp:1104-1118`: the sibling call sits inside the success branch). When another take already holds that second's name, the failed take's `rec:` source points at **another take's file**; a reload resolves rung 1 to the wrong audio (or a hash mismatch). Fix the naming, not the row. §8. **VERIFIED.**
8. **Two page defects in v1 code that 3.3 makes live (D48: defects only):** (a) the LFO dot's Y ignores the phase offset — the curve is drawn at `shape (p + ph0)` but the dot at `shape (pp)` (I:4400-4412), so with `lfoN_phase ≠ 0` the dot leaves the curve; (b) none other found. §7. **VERIFIED** (code) / **INFERRED** (visible effect; a probe row proves it).
9. **The closure gate [15] pins the key set.** It asserts the 3.2 encoder emits exactly 13 top-level, 3 lane and 8 rec keys and equals the 3.1 placeholder's set (`ui_frontend_check.js:664-667`). Adding `lfoPh` (and `lo` / `hi` if chosen) fails it by design; 3.3 amends the expected set and adds its "emitted and read" rows. §9. **VERIFIED.**

---

## 1. Current state — VERIFIED

| Key | Page reader | 3.2 sends | 3.3 source |
|---|---|---|---|
| `voices` | `drawVoices`, `drawLane`, `drawAmpFigure`, `drawModSources` (key view) | 20 rows, state 1 / 0, `wantSt` 0 | 16 display rows, state 0–3, real `wantSt` (§3) |
| `newest` | voice dots / cells, menv dots, LFO dots, key view, `label.newest` | −1 | row of the engine's `newestSer` (§3) |
| `menv` | `drawAmpFigure` (`[i][4]`), `drawMenvFigure` (`[i][0…3]`) | `[]` | `[e1, st1, e2, st2, ampSt]` per row (§4) |
| `sources` | `drawLfos` read-outs, `drawModSources`, chip glow, menv read-outs | `[]` | `srcTel[1…17]` (§4) |
| `modNorm` | `placeRingDot`, loop-edge tick, write read-out (dest 114) | `{}` | routed `destTel` (§5) |
| `lfoPh` | `drawLfos` | omitted | `[global, row0…15]` × 4 (§7) |
| `out` | `ingestTelemetry` → output meter | `[0, 0]` | `[L, R]` consuming (§6) |
| `rec.peak` | `ingestTelemetry` → SC meter | `[p, p]` | `[L, R]` consuming (§6) |
| `limit` | `updateReadouts` caption / bars | 0 (page parses the choice) | `grain_limit` value (§7) |
| grain `flags` bit 2 | lane dot colour (S6 clamp) | 0 | voice × stream clamp (§3) |
| `frozen` | none | omitted | stays omitted |
| `lanes[].lo / .hi` | `drawLane` loop shading | omitted | **[decision]** §5 |

Thread facts unchanged from 3.2: one `TripleBuffer<GiUiFrame>` frame per host block; `buildUiTelemetry` is the single reader of the frame and of every consuming peak, under `telemetryLock` (PP:1172-1176).

## 2. Engine-side additions (audio thread, telemetry only)

All writes below happen in `publishTelemetry` (once per block, after the block's audio is final) or at the existing steal site. None of them is read back by the DSP, so the carried audio set (renders-2.4 + legacy byte-identical, parity, param-dump) cannot move; still checkpoint after the task, as 3.2 did.

| Addition | Where | Cost |
|---|---|---|
| `GiUiVoice`: `state` (0 free · 1 gated · 2 released · 3 stolen), `serial` (u64), `menv[2]` + `menvStage[2]`, `ampStage`, `lfoPh[4]` (−1 when the LFO is not per-voice), `wantSt[4]` (per stream, before the ±48 clamp) | `publishTelemetry` voice loop (`Engine.cpp:801-821`) | ~60 B × 20; `voiceSemis` is `const`, 80 calls per block |
| `GiUiFrame::newestSerial` | same, from `newestSer` | 8 B |
| `GiTelemetry::outPeakL / outPeakR` (replace `outPeak`) | `publishTelemetry` (`:756-759`) | one more `pushMax` |
| `GiTelemetry` steal ring: `std::array<std::atomic<std::uint64_t>, 16> stolen`, `std::atomic<std::uint32_t> stolenCount` | `stealVoice` (`:3019`), outside `OGI_TEST_HOOKS` | two relaxed stores per steal |
| `GiRecorder::scPeakL / scPeakR` (replace `scPeak`) | `runPiece` (`:296, 375`) | one more CAS loop per block |

Notes:
- Stage mapping for the page (1 A · 2 D · 3 S · 4 R · 0 off): `AmpEnv::Stage {idle, attack, decay, sustain, release, forced}` → 0, 1, 2, 3, 4, 4; `GiModEnv::Stage` the same without `forced` (`AmpEnv.h:58`, `GiVoiceMod.h:59`).
- Per-voice LFO phase uses the global path's own expression: `vm[k].lfo[n].position() − getPhaseDeg() / 360`, wrapped to [0, 1) (`Engine.cpp:1781-1784`). Write −1 when `lfoPerVoiceActive (n)` is false.
- `wantSt[s]` = `voiceSemis (v, pairSp (k, s), absPos)` for a counted voice and a stream with a tape, else 0. It excludes the grain-level `pitch_rand` term (the ±48 clamp applies after it, `Engine.cpp:3377`); the row and the S6 marker therefore show the voice's wanted pitch, not every grain's. **[accept, recommended — the per-grain draw is not stored on the Grain]**
- The recorder's presence logic keeps using `max (L, R)`: the reader takes both and maxes them for `rec.signal` (one reader, both values).
- Mono sidechain (`channels == 1`): `xR` is the L sample in `runPiece`, so the two bars move together, which is correct.
- `GiRecorder.h`, `Engine.*`, `GiTelemetry.h` are not forked; drift stays 0 / 13.

## 3. Voice rows, steals, `newest`, `wantSt`

**Display-row map** (processor, message thread, under `telemetryLock`; state like `recRunHold`):

```
rows[16] = { serial, shownState, holdUntilMs }
per poll:
  1. read new steals: stolen[(c) % 16] for c in [lastStolenCount, stolenCount) (at most 16; older ones are lost and only shorten a hold)
     → a row holding that serial: shownState = 3, holdUntil = now + kStealHoldMs
  2. counted voices in the frame (state 1 / 2), by serial:
     row exists → update state / note / amp / scans / wantSt
     else      → lowest row that is empty, or whose hold expired; if none, the stolen-held row with the
                 oldest hold (a counted voice always wins a row)
  3. a row whose serial is no longer counted and not in a hold → empty
  4. newest = row of frame.newestSerial (−1 when no row holds it)
```

- Emit **16** rows (T:344, the page draws 16). Counted voices ≤ `polyphony` ≤ 16 (`parameter-spec.md:54`), so a counted voice always has a row.
- `kStealHoldMs` = 300 (≈ 9 polls). A held stolen row shows the victim's note, `amp` decaying (the frame still carries it while the voice is in the slot), state 3 → `vstate.stolen`, class `rel`.
- **Limit to accept:** at full 16-voice polyphony every row is counted, so the new voice takes the victim's row at once and the steal shows only as the note change. Below 16 the victim's row holds and the new note takes a free row. **[flag for HANDS-ON-3.3]**
- `newest` comes from the engine's `newestSer` (the PD8 rule: highest serial among counted voices, never a stolen spare, `Engine.cpp:1700-1745`), not from a page or reader guess, as CONTEXT requires. Between a note-on and the next mod tick the frame's `newestSerial` can lag one tick; harmless at 30 Hz.
- Playheads (`drawLane`) iterate whatever rows arrive; stolen rows carry scans −1 (no playhead for a fading voice). **[INFERRED — matches T's "steal spares are not shown"]**
- Grain `voice` field: map through the row map (−1 if unmapped). The page does not read it today (I:4313-4324), so this is consistency only.
- Grain `flags` bit 2 (S6 clamp, warn colour): set when `|wantSt[voice][stream]| > 48`. The killing bit stays unsent (3.2 PD1).
- **Steal visibility proof:** a harness row with polyphony 4: hold 4 notes, play a fifth; polls at 33 ms on a scripted clock show the victim's row in state 3 for ≥ 1 poll and ≤ 300 ms + 1 poll, then empty; the new voice in its own row. Negative control: hold off → state 3 never observed.

## 4. `sources`, `menv`, LFO phases

**`sources[i]` = `engine.getSourceTelemetry (i + 1)`, i = 0…16.** This is `srcG`: for per-voice sources the newest-voice value through the 20 ms handover ramp (`monoVal`), held at its last value when no voice sounds; for global sources the live value (`Engine.cpp:1750-1764`). It is what the global destinations are modulated by, so the chip glow, read-outs and rings agree. Units: LFO / Key / Slide / Bend −1…1; Env, Velocity, Note Random, Wheel, Pressure, Macros 0…1 (page clamps both, I:4404-4406, 4428-4432). **VERIFIED.**
- Held-when-silent: the page already shows `label.idle` for the Env read-outs and "—" for Key when `newest < 0` (I:4448, 4484-4489); the chip glow keeps the last value. Accept (v1 behaviour).
- Alternative (newest voice's raw `vm[nk].src`, no ramp) — rejected: it would disagree with the ring dots on global routes during a handover.

**`menv`** — 16 rows `[e1Level, e1Stage, e2Level, e2Stage, ampStage]` from the frame through the row map (T:639-646; page reads `[i][0…3]` and `[i][4]`, I:4198, 4219-4221). Stolen rows: `ampStage` 4 (forced), mod envs as they are. Cost at 16 rows: 80 numbers per poll; negligible next to the grains.

**LFO views** — `lfoPh[n] = [global, row0 … row15]`; global from `getLfoPhaseTelemetry (n)`, per-voice from the frame (−1 for an empty row or a Global-mode LFO). Page draws one dot per row ≥ 0 when `lfoN_mode` = Per-voice, ochre for `newest` (I:4407-4412).

**Page defect (D48).** The curve is plotted at `lfoShapeAt (shape, (2·i/120 + ph0) % 1)` but the dot at `Y = lfoShapeAt (shape, pp)` with `X = pp / 2` (I:4400-4410). With the engine's phase (offset removed) the dot's X is right and its Y is right only when `lfoN_phase` = 0. Fix: `Y = lfoShapeAt (shape, (pp + ph0) % 1)`. Mirror in `mockups/v1-ui.html` (byte-identical rule). Probe row: phase 90°, a stub `lfoPh` of 0.25 → the dot pixel lies on the curve.

## 5. `modNorm`, rings, mono badges, drag-to-modulate

- **Fill** (port of P:688-701): routed[d] from the APVTS raw values of `modM_source / modM_dest`, M = 1…16; `modNorm[String (d)] = engine.getDestTelemetry (d)` for routed d only. Unrouted keys are absent, so `modulatedNorm` falls back to the knob (I:4240-4243).
- **Units.** `destTel[d]` = `clamp (range.convertTo0to1 (raw) + offset (d) + voiceOff[d], 0, 1)` (`GiModMatrix.h:342-366`), with `range` the parameter's own `NormalisableRange` (`PP:235`). The ring's `k.norm` is the WebSlider's normalised value, the same range. **VERIFIED.** Skewed destinations therefore match.
- **Per-voice routes:** the voice part is the newest voice's `offCur()` (`Engine.cpp:1770-1776`); with no counted voice it is `nullptr` and the dot sits at base + global offsets, as CONTEXT wants. Global sources move it without notes. Written per mod tick (`writeTelemetry`, `Engine.cpp:1881`). **VERIFIED.**
- **Rings' range** is drawn from the APVTS routes by the page (`renderModRing`); nothing to send.
- **Loop handles.** The live edge tick reads `modNorm` (I:4326-4331). `lanes[].lo / .hi` (T:603-615, "the region the scan uses now") would also shade the loop region by its modulated edges. **[decision for plan: emit `lo / hi` = `min / max` of the two loop dests' `modNorm` only while either is routed (recommended — v1 draws it, one line each), or leave absent]**. If emitted, absent stays absent when unrouted (the page then uses the handles).
- **Mono badges** are page-computed (I:3075-3098). The page rule equals the engine's: per-voice sources 5–9, 11–13 + LFO N in Per-voice mode (`srcPerVoiceNow`, `Engine.cpp:1891`), global destinations 1, 114–121 + 122–125 while LFO N is Global (`destVoiceNow`, `:1896`). Note the engine also counts an LFO as per-voice during its mode crossfade (`lfoXf` not settled); the page uses the mode only — a 20 ms difference, accept. **Proof row (UMB1):** enumerate 17 × 125 pairs × LFO modes {Global, Per-voice} through the engine predicates and compare to a C++ transcription of the page rule; plus a probe row that sets each combination on the stub and reads `mrow-N.mono-on`.
- **Drag-to-modulate** is 3.1 code (`attachSourceDrag → addRoute`, gesture writes). 3.3 proves "every chip onto every target": probe rows that drop each of the 17 dock chips onto one knob and one loop handle and read the three written parameters; and every `[data-dest]` target (133 nodes) once with one chip. Negative control: a target whose `data-dest` is removed is skipped by the census → count mismatch.

## 6. Meters

**Output.** `outPeakL / outPeakR` (pushMax per block in `publishTelemetry`); the reader exchanges both to 0 and sends `out: [L, R]`. Post `output_gain` (`og` is applied inside the engine, `Engine.cpp:4085`), which is what the user hears. **VERIFIED.**

**Sidechain.** `rec.peak: [L, R]` post `rec_gain` (3.2 PD10 semantics kept). The raw `scInPeak` stays unread.

**Window.** Max since read; the page's ~30 Hz pull is the window; the editor-closed period accumulates and the first open shows that max (CONTEXT; same as 3.2's SC peak). Note: the parent zeroes a read older than 250 ms (R130, P:713-722). CONTEXT chose the other behaviour; keep it, the page ballistics (0.88 per poll, 0.94 per frame) clear it in about half a second.

**Gate design (Q8).** The page's bars are `(dB + 48) / 54` of the height (I:4347-4348).

| Row | Stimulus | Assertion | Tolerance |
|---|---|---|---|
| UMT1 out exactness | any scripted render (a held note, 2 streams panned apart) | each poll's `out[L]`, `out[R]` == the harness's own `max |x|` of the processBlock output channels since the previous poll | bit-exact |
| UMT2 out L / R separation | one stream, `s1_pan` −100, `stereo_spread` 0 | `out[R] / out[L]` ≤ −60 dB (equal-power pan at −100 leaves cos (π/2) ≈ 6e-17) | — |
| UMT3 SC known level | SC sine at fs/4, phase 0 (samples 0, A, 0, −A — the peak sample IS the amplitude), A = −6.02 and −20 dBFS on L, R silent; `rec_gain` 0 dB | `rec.peak[0]` = A, `rec.peak[1]` = 0 | ±0.01 dB |
| UMT3b non-aligned | 997 Hz sine at −6 dBFS | `rec.peak[0]` within [A · cos (π · 997 / fs), A] | −0.02 dB … 0 |
| UMT4 one reader | two polls back to back after one block | second poll reads 0 | exact |
| UMT1n / UMT3n | NCs: send `[max, max]` (no split) → UMT2 fails; skip the exchange → UMT4 fails | | |
| probe pixel rows | stub `out` = [0.5012, 0.1] (−6 / −20 dBFS) | bar tops at `(−6+48)/54` and `(−20+48)/54` of the canvas height on L and R | ±1 px |

A known-level **output** row through the synth is not a clean stimulus (grain overlap-add sets the level), so the output meter is proved by exactness against the rendered buffer (UMT1) plus the pixel mapping; the SC meter carries the "known sine level" criterion. **[flag: this is how "meters match known sine levels" is read; confirm at plan]**

## 7. `limit`, heavy hint, `frozen`, `lfoPh`

- **`limit`** = `32 << grain_limit` from the parameter (the engine's cap `gp.grainLimit` is the same number latched per block). The page already falls back to the choice when `limit` is 0 (I:4536), so this only makes the encoder truthful. Onset thinning lowers per-voice quotas, not the shared cap (`Engine.cpp:1388`), so an "effective" limit would mislead the "total / limit" caption. **VERIFIED.**
- **Heavy hint** stays parameter-driven: `comboIndex ('grain_limit') > 2` (I:2991), S1 "visible when value > 128" (`v1-ui.yaml:214`). Nothing to add. Default index 3 → the hint is on in the default state (gates already see it).
- **`frozen`**: no page reader → do not emit (the closure gate's rule is "emitted AND read").
- **`lfoPh`**: §4.

## 8. 3.2 carry-overs (D49)

**(a) Vacuous i18n states.** `rec-wait-level`, `rec-no-sidechain`, `rec-recording` set labels with `__setLabel` that the live poll overwrites within 200 ms. Live twins exist for the first two (`rec-wait-level-live`, `rec-no-sidechain-live`). Plan:
- **Delete** `rec-wait-level` and `rec-no-sidechain`; their side effects move: the drag-over cleanup goes into the next state's setup; `#heavy.on` is already on by default (index 3).
- **Rewrite** `rec-recording` through `__giState` (`c.rec ({ run: 2, slot: 4, pos: 0.3 })`), asserting `#rec-status-t` shows `rec.recording`; its stream-enable clicks move to a dedicated `lanes-streams-off` state that asserts `label.streamOff` / `lane.infoOff` (both live and uncovered today).
- State order matters: `__giState` is installed by `rec-armed`; any rewritten state must come after it.

**(b) CM6.** Fix in `SlotLoader.cpp:1101-1118`: call `getNonexistentSibling (false)` **before** the write branch, so the failed take's intended path is unique too (the directory may not exist on that path; then the name is unique anyway). Optionally add milliseconds to the stamp (`%Y%m%d-%H%M%S` + `-mmm`) so names stay readable in order; existing `rec:` paths are full paths and stay resolvable (no parser reads the stamp). The CM6 row stays as is and stops depending on CM5's timing. New row **UCM6b**: two takes in one wall second, the second with `failWav` → its source differs from the first take's path and does not exist. NC: sibling call moved back inside the branch → UCM6b fails.

**(c) UPM6b printed note** (`ui_gates_31.h:538`): "a `slots` array clears unlisted slots" → D45 wording, e.g. "a session `slots` array clears unlisted slots; a preset's leaves them (D45)". Text only; the predicate is right.

## 9. Verification method

**Harness rows (new `ui_gates_33.h`, U-prefixed).**

| Row | Checks | NC |
|---|---|---|
| UVR1 | rows: every counted voice has a row; 16 rows; a voice keeps its row for its life; with polyphony 16 and steal spares in use, a voice in engine slot 16–19 is shown | identity map (row = slot) → slot-16 voice missing |
| UVR2 | `newest` = row of the engine's `newestSer` across note-ons, releases, steals, mono / legato | page-style guess (highest row with state) → mismatch after a steal |
| UVS1 | steal visibility (§3) | no hold → state 3 never seen |
| UVS2 | release: state 2 after note-off, empty after the env ends; CC64 hold keeps state 1 | |
| UWS1 | `wantSt` = `voiceSemis` for a note 50 st above root (clamp 48 → `vstate.clamp`), grain flag bit 2 on that stream only | |
| UME1 | `menv` levels / stages vs `vm[k].env[e]` and `voices[k].env` for a scripted ADSR | |
| ULF1 | `lfoPh` global and per-voice phases vs `lfos[n]` / `vm[k].lfo[n]`; −1 for Global-mode rows | |
| USR1 | `sources[i]` == `getSourceTelemetry (i + 1)`; Key / Bend / Slide signs; held when silent | |
| UMN1 | `modNorm` keys == routed set; values == `destTel`; per-voice route follows the newest voice and rests at base with no voice | all keys sent → extra keys |
| UMB1 | mono-badge rule parity (§5) | |
| UMT1–4 | meters (§6) | UMT1n, UMT3n |
| ULM1 | `limit` = `32 << index` for each index | |
| UCM6b | §8 | |
| UAL1 (amended) | 0 audio-thread allocations with 256 grains, 16 voices, steals and per-voice LFOs publishing | carried control |
| INFO UPL2 | widest JSON (16 rows + menv + lfoPh + 125-key modNorm) and read cost | |

Carried set after every engine-touching task: harness 402 / 0 carried rows unchanged, parity 168 / 0 + GV 2 / 0, renders-2.4 + legacy byte-identical, param-dump identical, drift 0 / 13, X2 0; auval; pluginval 10 VST3 + AU with the sidechain on and off. `GiUiFrame` grows again → re-read TL2 (`engine_gates_26b.h:1006-1042`) for a size assumption, as in 3.2.

**Page proof.**
- `--dump-ui-overrides`: 3.3-shaped telemetry (two voices with one stolen row, `newest`, `menv`, 17 sources, a routed `modNorm`, `lfoPh`, `out`, `rec.peak` L ≠ R), byte-identical twice.
- `ui_frontend_check.js` [15]: update the expected key set (top 13 → 14 with `lfoPh`; lane 3 → 5 if `lo / hi`), keep "3.1 placeholder == 3.2 set" as "placeholder ⊆ live set" or update the placeholder too (**recommended: update the placeholder** — UTL31 then still pins it); add "emitted and read" rows for `newest`, `menv`, `sources`, `modNorm`, `lfoPh`, `out`, `limit`, `lo / hi`, voice state 2 / 3, `wantSt`; add a "not emitted" row for `frozen`.
- `ui_binding_probe.js`: counts unchanged (261 × 2, VE 14); add rows: voice cells / dots per state 0–3 and `newest`, key view newest colour, menv read-out `label.newest` vs `label.idle`, chip glow from `sources`, ring dot position from `modNorm` (pixel), LFO dot on the curve with phase ≠ 0 (pixel, the §4 defect), meter bar heights (pixel), mono badge per rule, drag-to-modulate census (§5).
- **i18n states** (`tests/i18n-states.json`, 38 today; D42): add through `__giState` with stub telemetry (VOICE page first — `drawVoices` returns early elsewhere, I:4481): `voice-held` (`vstate.held`, `label.voicesHeld`), `voice-release` (`vstate.release`, `label.voicesState`), `voice-stolen` (`vstate.stolen`), `voice-clamp` (`vstate.clamp` with `{ s: '+48' }`), `menv-newest` (`label.newest`), `write-held` / `write-per-lap` (`label.writeHeld`, `label.writePerLap`), `lanes-streams-off` (`label.streamOff`, `lane.infoOff`), `lane-info` (`lane.info`). Run the five gates on the unmodified tree first (baseline), as in 3.1 / 3.2.

**Human (HANDS-ON-3.3, run with all others at 3.3 verify, D47).** Drag each chip onto a knob and a loop handle; rings and the edge tick move with an LFO and with Env 1 on a held note; mono badge on Env 2 → Output; voice strip with a chord past polyphony 8 (steal visible) and at 16 (note change only); release tails; key view and newest marker follow the last note; LFO dots on the curve at a non-zero phase; output meter L / R with a hard-panned stream; SC meter with Logic's side-chain routing, plus Record-in end to end (the ROADMAP Logic AU pass); heavy hint at 128 off / 256 on.

## 10. Pitfalls specific to 3.3

- **Row index ≠ engine slot.** Every per-voice key (`voices`, `menv`, `lfoPh` row entries, `newest`, grain `voice`) must go through the same map in the same poll; mixing engine indices into one of them misplaces dots silently.
- **One reader per consuming feed.** `outPeakL / R`, `scPeakL / R`, the steal ring cursor, the frame: only `buildUiTelemetry`. The harness reads through its own processor instance and a scripted clock.
- **`stealVoice` vs `forceReleaseVoice`:** only `stealVoice` writes the steal ring; polyphony shrink and mono handovers are not steals.
- **`OGI_TEST_HOOKS`:** the steal ring is production telemetry (outside the hook block); the existing `hooks.stolenSerials` stays for the 2.x rows. X2 stays 0.
- **`newestSer` lag:** set at the mod tick, the frame at block end; a note-on in the last cell of a block shows `newest` one block late. Do not "fix" it reader-side.
- **Page edits** (LFO dot) go to `index.html` and `mockups/v1-ui.html` together; `js/i18n.js` == `v1-i18n.js`. No new string keys are expected in 3.3 (every key above already exists, en / fr / zh-Hans).
- **No `juce::Timer`, no `emitEvent`** (CONTEXT): every value rides the existing pull.
- **Not verified here:** the JSON size with 125 routed `modNorm` keys (bounded: at most 16 routes → ≤ 16 keys); Logic's behaviour with the side-chain meter while the instrument track is idle (the meter then reads 0 — expected, as `scPeakPush` runs only in `runPiece`).

## 11. Answers to the CONTEXT open questions

| # | Question | Answer |
|---|---|---|
| 1 | Output meter shape | Split into `outPeakL / R` (and the SC peak likewise); telemetry-only, audio byte-identical by construction (§6). |
| 2 | `sources[17]` value, units, polarity | `getSourceTelemetry (1…17)` = the newest-ramped `srcG`; indices and polarity match the page exactly (§4). |
| 3 | `menv` shape, cost | 16 rows `[e1, st1, e2, st2, ampSt]` through the row map; needs a `GiUiVoice` extension; negligible cost (§4). |
| 4 | `modNorm` / `modRouted` | Routed-only port of the parent; `destTel` is already destination-normalised in the WebSlider's range (§5). |
| 5 | `wantSt`, steal / release visibility | Frame carries state + serial + `wantSt[4]`; sticky steal ring + 300 ms row hold; a 16-row display map (§3). |
| 6 | `limit` | The parameter value `32 << index`; heavy hint stays parameter-driven (§7). |
| 7 | `frozen`, `lfoPh` | `frozen` unread → not emitted; `lfoPh` from `lfoPhaseTel` + per-voice phases in the frame (§4, §7). |
| 8 | Sine-level gate | SC: fs/4 sine, exact peak, ±0.01 dB; output: exactness vs the rendered buffer + L / R separation; pixel rows for the bar mapping (§6). |
| 9 | CM6 | Production bug on the WAV-fail path; move `getNonexistentSibling` before the branch; new row UCM6b (§8). |
| 10 | Proof plan | §9: `ui_gates_33.h` rows + NCs, overrides dump, [15] key set update, probe rows, nine i18n states. |

## 12. Decisions for the plan

1. **Voice rows:** a 16-row display map keyed by serial in `buildUiTelemetry` (recommended) — the alternative (send 20 engine slots) leaves slot 16–19 voices invisible on a 16-cell page.
2. **Steal visibility:** sticky steal ring in `GiTelemetry` + 300 ms row hold (recommended); counted voices always win a row.
3. **Frame extension:** `GiUiVoice` += state, serial, menv, stages, per-voice LFO phases, `wantSt[4]`; `GiUiFrame` += `newestSerial` (recommended; nothing else carries them).
4. **Meters:** split out and SC peaks into L / R (recommended) vs `[p, p]`.
5. **`sources`:** `srcG` (ramped newest) (recommended) vs the newest voice's raw value.
6. **`lanes[].lo / .hi`:** emit while a loop dest is routed (recommended) or leave absent.
7. **Grain clamp bit:** voice × stream `wantSt` beyond ±48, `pitch_rand` excluded (recommended) or leave 0.
8. **CM6:** fix the naming (recommended, plus optional ms in the stamp), not the row.
9. **i18n carry-over:** delete two vacuous states, rewrite `rec-recording` live, move their side effects (recommended).
10. **[15] key set:** update the 3.1 placeholder to the 3.3 set so UTL31 keeps pinning it (recommended).
11. **Meter gate reading of "known sine levels":** SC exact-level rows + output exactness rows (§6) — **worth a one-line confirmation from Taylor at plan time.**
