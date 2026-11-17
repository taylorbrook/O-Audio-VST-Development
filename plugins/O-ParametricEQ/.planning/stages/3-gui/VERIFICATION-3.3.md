# Stage 3: GUI — Phase 3.3 (analyzer, meter, fills, listen) - Verification

## Verification Date

2026-11-16 (verify session). Everything below was run against `488033a8` (0.9.0). Before the run, `git status` showed no changes under `plugins/O-ParametricEQ` outside `.planning/`.

What the verify session did:

- **Independent harness re-run** (`verify-3.3/run-harness.sh`).
  - The harness objects were deleted and recompiled.
  - Then: a session dump, the designer harness, the render harness default and `--long` against that dump. Tallies are in `verify-3.3/harness-tallies.txt`.
  - `research-3.3/probe` rebuilt from the committed tree and `probe33 n` run (`verify-3.3/probe-n.txt`).
- **Independent no-audio-change check.** The 16 XR-lid sessions dumped by the 0.9.0 harness were hashed (`verify-3.3/sessions-090.sha`). The hashes equal `stages/2-dsp/verify-2.6/sessions-050.sha` 16 / 16.
- **Code read:** the full `452a0cd3..488033a8` diff of `Listen.h`, `PluginProcessor.{h,cpp}`, `SpectrumAnalyzer.{h,cpp}`, `PeakHold.h`, `PluginEditor.{h,cpp}`, `index.html` and `CMakeLists.txt`; the processor's listen snapshot.
- **Gates and greps, this session's own runs:**
  - all five UI gates;
  - the native-function diff (page + `preset-manager.js` vs `PluginEditor.cpp`), with this session's grep;
  - `tests/g9b/run.sh` and `tests/meter-ballistics/run.sh`;
  - the static greps (`vu-meter`, `OPEQ_EDITOR_BENCH`, `setListenNative(`, span centre, `meterStep`);
  - `diff -rq` of the installed bundles against the artefacts.
- **Validators** on the installed bundles: pluginval strictness 10 (VST3, AU, `--timeout-ms 600000`) and `auval -v aufx OuPQ OuDv`.
- **Execute's committed bench JSON re-scored:** `verdict33.py` over `results/rel-1…5.json` and `dbg.json` gives ALL-PASS for all 6. `dbg.log` has 0 assertions and 0 leak lines.
- **One fresh Release bench process** on a quiet machine (`verify-3.3/bench-rel-v1.json`): the full 10-cycle run, all rows. This re-reads E-audio's thin margin, as SUMMARY-3.3 asked.
- **Mutation run** (`verify-3.3/mutants33v.py`, `mutants33v-results.json`):
  - 22 one-line mutants, none of them among execute's 13: 7 in the editor, 5 in `PeakHold.h` / `SpectrumAnalyzer.cpp`, 2 in `Listen.h`, 8 in the page.
  - 17 are scored against a named row. 5 are probes (is there any row for this at all?).
  - Scored by execute's runner, unchanged: the bench rebuilt against a scratch `Source/`, the render harness `--rows33` from the scratch tree, or `ballistics.mjs` on the scratch page.
  - The three controls (bench, harness, node) on the unmodified copy all pass. The bench control ran twice (the second time alongside the W22 re-run): ALL-PASS both times.

Nothing in the repo source changed. Scratch builds, logs and the mutable source copy are in the session scratchpad.

Scope: Phase 3.3, which closes Stage 3.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 3.3; PLAN-3.3 Goal)

1. **Analyzer.** Off / Pre / Post / Pre+Post from the `UI` state. Off stops the audio-thread writes. The trace sits at the span centres, and nothing is drawn above Nyquist.
2. **Meter.** Page-drawn, two bars, −60 to +6 dB, fixed zones, peak hold, ruler inside the box. A −6 dBFS sine reads −6 within 0.5 dB at every host block size. Ballistics in dB per second.
3. **Fills.** Each enabled band that is not bypassed is washed in its pigment. Bypassed bands are not.
4. **Listen.** Engages on the dragged band past the threshold. On release it returns to the latch (or off). The strip button latches. Disabling or deleting the listened band, from any source, ends it.
5. **Listen NaN fix.** A NaN inside listen's input ramp is caught at its own chunk (N1e 16 / 16). Listen-off output stays bit-identical.
6. **60 fps.** A drag with Pre+Post on holds 60 fps and does not raise audio-thread time.
7. The four VERIFICATION-3.2 gate gaps are closed. No idle traffic. 0.9.0 builds, installs and validates.

### Deliverables (from SUMMARY-3.3.md, confirmed in the tree)

1. **Processor / DSP.**
   - `Listen.h`: `nonFiniteVoice` is set in `renderVoice` (last sample of L, or of R on stereo). `takeNonFiniteVoice()` returns and clears it, and `goIdle()` also clears it.
   - Step 8: the flag is taken on its own line before `if (! finite)`, with the comment explaining why.
   - The `VUMeterBridge.h` include is gone.
2. **`SpectrumAnalyzer`.** While a tap's `analyzerMode` bit is off, its `valid` count is cleared on every `process` (execute deviation 1). This is message-thread code only.
3. **`PeakHold.h`** (standard library only):
   - `PeakHold`, a 16-deep ring, `depthFor` = ⌈B / fs × 60⌉ + 1;
   - `gainToTenthsDb` (200 log10);
   - `encodeAnalyzerPoints` (tenths, clamped to [−1200, +600], `kAnalyzerNoData` from the first span ≥ fs / 2).
4. **Editor.**
   - The `analyzer` is built after `displayModel` and reset first in the destructor.
   - `emitAnalyzer (dt)` runs every tick: it processes whatever the mode, sends a tap once when the mode adds it, and emits per tap only when the `memcmp` shows a change.
   - `emitOutputLevel` uses the H hold and emits only on change. A mono layout sends one value.
   - `pageReady` clears what was sent before.
   - The `/modules/vu-meter.js` branch is gone. The bench hooks exist only under `OPEQ_EDITOR_BENCH`.
5. **Page.**
   - `decodeAnalyzer` maps `null` to `NaN`, and the trace x is at the span centre.
   - The meter: canvas, `meterStep`, its own rAF loop that stops at rest, colour tokens, `document.fonts.load` repaint.
   - The listen machine: `latch`, `dragListen` and `syncListen`, which is the one `setListenNative` call site. The enabled hook re-checks after 80 ms. `deleteBand` relies on that hook.
   - `drawOncePerFrame`. The strip button's `aria-pressed` follows `latch`.
6. **Tests:** render rows N1e, N1e-u, AO1, AO2 and M1 (`--rows33`); `tests/meter-ballistics/`. **Bench:** `execute-3.3/editor-bench/`. **Registry:** `vu-meter` has `used_by: []`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Analyzer (UI-03) | ✅ Achieved | Committed E-an-modes / E-an-stale / E-an-nyq / E-an-peak pass in all 6 processes and in this session's process. AO1 and AO2 pass in this session's render 171 / 0.<br>• Verify mutants caught: W1 (emit ignores the mode) → E-an-modes. W2 (Pre / Post event names swapped) → E-an-peak. W3 (Nyquist at fs) → E-an-nyq. W8 (Off-rule tap bits swapped) → E-an-nyq, E-an-peak, AO2.<br>• Off stops the writes: E-an-modes, a write-index delta of 0 in mode 0. |
| 2 Meter (UI-06) | ✅ Achieved, by eye against Logic's meter: hands-on | M1 6 / 6 in this session's render. E-meter (48 k / 512 at my(−6); 44.1 k / 2048 range 0 px; mono) passes. `meter-ballistics` PASS, and its control fires.<br>• Caught: W5 (mono sends two values) and W12 (the page's mono right bar silent) → E-meter mono. W6 (power dB) → E-meter, M1. W13 (the loop never rests) → E-meter-rest. W4 (`outputLevel` every tick) → E-idle33, E-meter-rest. W19 (no peak hold) and W20 (10 dB/s release) → meter-ballistics.<br>• **Survivor W7** (H without the +1): see Issue 1. |
| 3 Fills (UI-07) | ✅ Achieved | E-fills passes. W14 (a bypassed band washed) → E-fills. |
| 4 Listen (FUNC-08) | ✅ Achieved in-process; by ear: hands-on | E-listen33 18 / 18 in every process.<br>• Caught: W9 (80 ms settle removed) → (f), as execute deviation 2 predicted. W10 (the drag listen sticks after release) → (a)–(d). W11 (bypass ends listen) → (g).<br>• With execute's three listen mutants, every branch of the machine is held: threshold, release, latch return, the disable hook, the settle, bypass.<br>• **Probe W15** (strip button follows `ui.listen`): no row, see Issue 3. |
| 5 NaN fix | ✅ Achieved | `probe33 n` on the committed tree: **N1e 16 / 16, hash `6274ff9b5b0e488a`**. N1e / N1e-u pass in render 171 / 0.<br>• W18 (only R checked) → N1e.<br>• Probe W17 (`goIdle` leaves the flag set) survives. On reading it is equivalent: step 8 takes the flag in the same chunk that sets it, so `goIdle` never sees it set. The clear is belt-and-braces. |
| 6 60 fps, audio time | ✅ Achieved; margin thin | This session's process: E-fps rAF **median 17 / p99 20 ms**, `draw()` 8 / 9 ms. E-audio closed 314.9 µs vs open + Pre+Post + drag 343.6 µs: **+9.1 %** median (gate 10 %), p99 +17.4 % (gate 25 %), on the first attempt.<br>• Execute measured +8 – 10 %. W16 (double draw per frame restored) → E-fps.<br>• See Issue 4. |
| 7 Gaps closed, idle, validates | ✅ Achieved | E-rate 16.7 ms (V-3.2 Issue 1), E-create-seeded (Issue 2), E-gbypass and E-sumview (Issue 4) pass in all processes. E-idle33: 0 messages of any kind over 10 s.<br>• **16 / 16 XR-lid sessions SHA-256-equal to 0.5.0.** Designer 150 / 0 / 18. Render 171 / 0 default and `--long`.<br>• Installed `-dev` bundles are `diff -rq` identical to the artefacts: 0.9.0, built 10:57, after every source edit. No unsuffixed orphan.<br>• pluginval 10 VST3 / AU: SUCCESS / SUCCESS. `auval`: AU VALIDATION SUCCEEDED. |

## Requirements Verification

**Stage:** stage-3. This phase closes FUNC-08, UI-03, UI-06 and UI-07, which completes every stage-3 requirement.

| Requirement | Priority | Status | Acceptance criteria |
|-------------|----------|--------|---------------------|
| FUNC-08 Band solo / listen | should | ✅ Complete | Engine half Stage 2 (2.6). Transient mapping and the editor-close reset in 3.1 (E-cycle33: −1 after every close). The drag / latch / disable machine is E-listen33 (a)–(j). The NaN in the ramp is N1e 16 / 16. Clean engage by ear is a hands-on row. |
| UI-03 Spectrum analyzer Off / Pre / Post / Pre+Post | should | ✅ Complete | E-an-modes (exact tap sets, 0 writes in Off), E-an-stale, E-an-nyq, E-an-peak; AO1 / AO2. |
| UI-06 Output level meter | nice | ✅ Complete | −6 dBFS within 0.5 dB at 44.1 / 48 / 192 k and blocks 1024 – 4096 (M1). Pixel position at 512 and 2048 (E-meter). Ballistics in dB/s, independent of the frame rate. |
| UI-07 Per-band fills | nice | ✅ Complete | E-fills: hue within 15° of the pigment; bypassed → alpha < 6. |

**Requirements Summary:** Complete 4 (FUNC-08, UI-03, UI-06, UI-07), Partial 0, Failed 0. Stage 3 total: UI-01, UI-02, UI-03, UI-04, UI-06, UI-07, FUNC-08 complete, and DSP-06 (closed in 3.2).

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness rebuild | ✅ | `build rc 0` after the objects were deleted |
| Designer | ✅ 150 / 0 / 18 | |
| Render default / `--long` | ✅ 171 / 0, 171 / 0 | XR-lid on this session's dump |
| Audio vs 0.5.0 | ✅ 16 / 16 | `verify-3.3/sessions-090.sha` = `verify-2.6/sessions-050.sha` |
| Probe `probe33 n` | ✅ 16 / 16 | hash `6274ff9b5b0e488a` (= RESEARCH-3.3, = HEAD before the fix) |
| G9 (b) | ✅ PASS | 1.372e-8 dB, all controls (unchanged from 3.2) |
| meter-ballistics | ✅ PASS | control fires (−16 vs −26 dB) |
| UI gates ×5 | ✅ all 0 | `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` (O-ParametricEQ 52 `mt`), `check-ui-labels --plugin O-ParametricEQ`, `boot-all-uis --strict-tips` (0 DEAD) |
| Native functions | ✅ 19 / 19 | identical |
| `vu-meter` in `CMakeLists.txt` / `Source` | ✅ 0 | `Source/ui/public/modules/` holds only `preset-manager.js`, untracked |
| `OPEQ_EDITOR_BENCH` | ✅ | 0 in `build.ninja`; only the editor files reference it |
| `setListenNative(` call sites | ✅ 1 | in `syncListen` |
| No `i / (n - 1)`; one `meterStep` | ✅ | |
| Install | ✅ | identical, 0.9.0, `-dev` only |
| pluginval 10 VST3 / AU | ✅ SUCCESS / SUCCESS | |
| auval | ✅ | |
| Committed bench JSON re-scored | ✅ 6 / 6 ALL-PASS | Debug 0 assertions, 0 leak lines |
| Fresh Release bench process | ✅ ALL-PASS | `bench-rel-v1.json`; screen unlocked throughout |
| Verify mutation run | ✅ 17 / 17 scored mutants caught; 0 / 5 probes | controls ×3 pass; see below |

### Mutation run (verify)

| Mutant | Target | Result |
|---|---|---|
| W1 analyzer emits regardless of the mode bit | E-an-modes | caught |
| W2 `analyzerPre` / `analyzerPost` event names swapped | E-an-peak | caught |
| W3 Nyquist taken as fs | E-an-nyq | caught |
| W4 `outputLevel` change test removed | E-idle33 | caught: E-idle, E-eq-idle, E-meter-rest, E-idle33 |
| W5 mono layout sends two values | E-meter | caught (mono) |
| W6 `gainToTenthsDb` as power dB (100 log10) | E-meter, M1 | caught: all three E-meter checks, M1 |
| **W7** `depthFor` without the +1 | E-meter, M1 | **survived** (Issue 1) |
| W8 Off-rule tap bits swapped | E-an-peak | caught: E-an-nyq, E-an-peak, AO2 |
| W9 enabled hook without the 80 ms settle | E-listen33 (f) | caught: (f), (g) |
| W10 drag listen not cleared on release | E-listen33 (a) | caught: (a)–(d) |
| W11 bypass ends listen | E-listen33 (g) | caught |
| W12 page meter: mono right bar at the floor | E-meter | caught (mono) |
| W13 meter loop never rests | E-meter-rest | caught |
| W14 fills ignore bypass | E-fills | caught |
| W16 two `draw()` per frame | E-fps | caught |
| W18 voice check reads only R | N1e | caught |
| W19 peak hold 0 s | meter-ballistics | caught |
| W20 release 10 dB/s | meter-ballistics | caught |
| *probe* W15 strip button follows `ui.listen` | — | no row (Issue 3) |
| *probe* W17 `goIdle` leaves the flag | — | no row. Equivalent on reading (Goal 5) |
| *probe* W21 `pageReady` keeps last-sent arrays / level | — | no row (Issue 2) |
| *probe* W22 no re-send when a mode adds a tap | — | no row. Equivalent on reading: a tap re-added with an unchanged array means the page already holds that array (the page keeps `analyzerPre` / `analyzerPost` across mode changes) |

Note: W22 first failed to build (the mutation left an empty `if` body). It was rewritten as `(void) 0;` and re-run with the bench control. No other mutant was re-run.

## Human Verification

For Taylor, in Logic (AU) and one VST3 host. These come from SUMMARY-3.3's carry-forward.

- [ ] Analyzer on program material: Off / Pre / Post / Pre+Post; nothing past Nyquist at 44.1 / 48 kHz.
- [ ] Meter against Logic's channel meter on a −6 dBFS sine. With a 2048 buffer the meter is steady.
- [ ] Drag listen and latch by ear: engage and return are clean, with no click. A click to select does not solo.
- [ ] Disabling the listened band by automation ends it. Loading a preset that keeps it enabled does not.
- [ ] Delete the listened band: listen ends. Expect up to 80 ms plus the 50 ms engine fade of the deleted band's listen (Issue 5).
- [ ] Standalone with live input: the analyzer and meter move, and the console is clean (execute deviation 9).
- [ ] Meter look against the mockup (zones, ruler digits in EB Garamond, peak line) at 800 × 500 and 1600 × 1000.

## Issues Found

1. **The meter hold's +1 margin is not gated (minor; Stage 4 bench edit).**
   - W7 (H = ⌈B / fs × 60⌉, no +1) passes M1 and E-meter. Both deliver blocks at an exact real-time cadence, and there a window of ⌈B / tick⌉ reads always contains a block.
   - The +1 is the margin for host jitter: blocks arriving in bursts, or a late timer tick. It is what stops the sawtooth in a real DAW. The code matches RESEARCH-3.3 Q2.
   - Recommendation: add an M1 variant with jittered block completion (for example ±30 % of a block period), so that H without the +1 shows empty reads.
   - The hands-on 2048-buffer row covers this by eye meanwhile.
2. **`pageReady`'s reset of what was sent is held by code only (minor; info).**
   - W21: if `analyzerSent` and `lastLevelCount` are not cleared, a reloaded page at idle gets no trace or meter level until something changes.
   - A page reload inside a living editor only happens after a WebView content-process restart. A reopen builds a new editor. No row needed now; noted.
3. **The strip Listen button's `aria-pressed` follows the latch: code only (minor; Stage 4 bench).**
   - W15 survives. Execute deviation 4 is right on reading.
   - Recommendation: one E-listen33 check: latch off, drag band 3 → `aria-pressed` stays false during the drag.
4. **E-audio margin is thin, but repeatable.**
   - This session: +9.1 % on the first attempt. Execute: +8 – 10 %, with one re-run needed in two of its processes.
   - The extra cost is the message thread's WebView work competing for the machine, not audio-thread work: `processBlock` is unchanged with listen off (16 / 16 SHA).
   - Stage 4's G10 CPU gate measures the engine itself. Recommendation: keep the gate at 10 % with its one allowed re-run, and read a fail on a busy machine as noise first.
5. **Listen outlives a disable by up to 80 ms (by design; hands-on).**
   - The settle that keeps a preset load from ending listen (execute deviation 2) also delays the end on a real disable or Delete. For those 80 ms, plus the engine's 50 ms fade, the deleted band's listen is still heard.
   - Recommendation: confirm by ear. If it is audible, `deleteBand` can call `endListenOn(b)` at once, because a page delete is not a preset pass-through. The hook then stays for host and preset disables.
6. **Cosmetic, carried from VERIFICATION-3.2 Issue 5:** `.tip-point` is removed at the start of the fade. Stage 4 polish.

Carried forward unchanged:
- Stage 4: the fr / zh-Hans review of `bands-full`; point-tip housekeeping (VERIFICATION-3.2 Issue 3); host text units; the E16 / R2 preset rows.
- Execute deviation 10: one unexplained E-an-nyq pixel count in rel-5 of execute's first run. It did not recur in execute's later runs, this session's process, or the 22 mutant runs.

No blocker. Nothing failed.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 3.3, and with it Stage 3). Every automated criterion in ROADMAP 3.3 is met and was reproduced independently. All stage-3 requirements are complete. The DAW hands-on rows above are owed by Taylor, as for 3.1 and 3.2.

**Ready for next stage:** Yes. Stage 4 (Polish and validation): factory presets, French / zh-Hans, G10 CPU, multi-rate and host compatibility, the full gate battery. Its bench should take in Issues 1 and 3.

**Blockers:** none.
