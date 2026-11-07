# Stage 4 — Polish / Validation — Execute Summary

**Plugin:** O-Granulator · **Stage:** 4 (Polish) · **GSD phase:** execute · **Date:** 2026-11-07
**Plan:** PLAN-4.md (P143–P162, T0–T7) · **Branch:** `main` (trunk) · **No tag.**

| Task | Commit | Result |
|---|---|---|
| T0 entry check + baseline C | — | 520/520 PASS; carried extract == the 3.3-verify extract (masked, see deviation 1); N3/N2/N1/N0 `--strict` + R12 rc 0; HA PASS; codec 9/9 == B; param-dump clean; overrides ×2 identical; UI gates + probe 220 ×2 green |
| T1 D43 + AG gates → baseline **D** | `9253d3d4` | AG1–AG6 + AG1n PASS; 529/529; no sat-0 mover |
| T2 D47 (a)–(e) | `638b86ea` | 533/533; neutral vs D except the attributed movers below |
| T3 + T4 presets, state, layouts | `ef01c549` | PR0–PR5 + PR1n, ST1–ST3 + ST2n, LY1 / LY1n PASS; 547/547 |
| T5 1.0.0 + CHANGELOG + install + host validation | `9866e37a` | auval PASS, pluginval 10 VST3 + AU PASS, installed `Factory/` 16 files at 1.0.0, ST4 12/12, HA PASS, codec 9/9 == B |
| T6 UI gates + PF1 | `21304de0` | all rc 0 (probe NC rc 2 = fails as it must); PF1 / PF1n PASS |
| T7 docs | (this commit) | HANDS-ON-4.md, DAW-4.md, this file, REQUIREMENTS, STATUS |

Logs: `${TMPDIR}/ogran-4/{t0,t1,t2,t3,t5,t6}/` (rc.txt per run). Final gate run (T3 tree, harness unchanged since): **547 PASS, 0 FAIL**; build 0 warnings; harness wall 7–8 min.

---

## D43 — AGC restage (sign-off 1, option 3)

`FeedbackChain::stepAgc (xl, xr, d)`: `peak = d · max (|xl|, |xr|)` (= |tanh (d·x)|, so env ≤ 1); `g = min (1, kAgcCeil / env)` unchanged; `d` is the per-sample ramped value the saturator used on that sample. `testAgcLegacy` (via `hooks.agcLegacy`) restores the un-normalised peak. `Engine.cpp:563` `loopAgcTel` and the LG readout are unchanged.

| Row | Result |
|---|---|
| AG1 (LP-res-100 corner, sat 20 %, fb 100 %, 20 s) | min g_agc (10–20 s) **0.745**; return / (0.7/d) **1.1986** ≤ 1 + m, **m = 0.209** (measured 0.1986 + 0.01); finite |
| AG1n (same, legacy) | g_agc ≡ 1.000000; return / (0.7/d) 1.4095 — fails AG1 |
| AG2 (FB3n ARCH growth, sat 0/20/50/100) | min g 0.864 / 0.863 / 0.863 / 0.863; hold 1.2134 / 1.2058 / 1.2004 / 1.1923 × (0.7/d), **tol 0.224** (0.2134 + 0.01); slope after the hold +0.0056 / +0.0050 / +0.0048 / +0.0045 dB/gen ≤ +0.05 |
| AG3 (sat 0, new vs legacy, bitwise) | **51/51 identical** (FB1, FB2a/b ×4, FB2c/d ×2, FB3 37 corners, FB3n, FB4 ×4, LG1 hot, Z7d 30 s); 4 of them with the AGC engaged |
| **AG4 (sign-off 3)** | (a) −6 dBFS-peak noise bursts: g_agc ≡ 1. (b) 0 dBFS-peak kick/snare loop at defaults + fb 50 %: **min g 1.0000, depth 0.000 dB p-p** ≤ 1.0. The AGC does not engage at fb 50 % on this material |
| AG5 | (a) Z7d config at sat 20 %, 340 s, same seed: last non-zero **295.77 s == legacy 295.77 s** (< 329); (b) freeze render at default sat: bitwise == legacy, engages 1 |
| AG6 | env max over the FB4 corners 0.422 / 0.939 / 0.534 / 0.924 ≤ 1 + 1e-6 (FB4's clause tightened from `0.7 + 1/d` to `1 + 1e-6`) |

### C → D ledger (every moved carried row)

| Row | Old (C) | New (D) | Sat | Cause |
|---|---|---|---|---|
| FB2c (sat 0 → 100 % comparison) | crest Δ 2.58 dB, level Δ 1.32 dB | crest Δ 5.59 dB, level Δ 71.04 dB | the **100 %** arm moved; the 0 % arm is bitwise (AG3) | The AGC now engages on the −6 dBFS burst at sat 100 %; its 400 ms release holds g < 1 across the many generations of the 42-sample loop, so the residual in the 1.1–2.9 s window falls much further. Both arms are far below audibility in that window. Predicate (≥) unchanged |
| FB4 | env 0.168 / 0.379 / 0.192 / 0.376; LPres pk 1.426, crush pk 1.259; clause `env ≤ 0.7 + 1/d` | env 0.404 / 0.824 / 0.462 / 0.855; LPres pk 1.223, crush pk 1.254; clause `env ≤ 1 + 1e-6` | 20 % | env is now normalised (× d = 2.4); the AGC engages on the LP-res and crush corners and lowers their peaks. Clause **tightened** (AG6) |

No other carried row moved (FB2a/b, FB3*, LG*, Z7d stayed inside their recorded values or are not in the default set). **No sat-0 row moved; no threshold was loosened.** No golden file moved (`tests/golden/` holds parameter tables, not render digests). Baseline **D** = the T1 carried extract (`${TMPDIR}/ogran-4/D-norm.txt`) + the N-suite references (unchanged, rc 0) + codec B.

## D47 — carried housekeeping

- **(a) Env pin.** `EnvFollower::process`: `if (! (x <= kAnalysisTapMax)) x = 0` (test hook `envPinOff`). **Z7h3e** (row): after a 1e30 host sample, pinned cells == the zero-sample reference at 4 rates (0 vs 0); the L+R / mono Env traces are bitwise. **Z7h3n**: `envPinOff` → **4125** cells ≥ 0.999.
- **(e) `uiSnap` clear.** End of `Engine::prepare`: `uiSnap.writeBuffer() = UiSnapshot {}; uiSnap.publish();` (hook `noUiSnapClear`). **US1**: 0 grains, not frozen. **US1n**: 30 grains, frozen 1.
- **(b) Event-aware `renderPool`.** The call-site audit (`OGRAN_POOL_AUDIT=1`, a one-off run):

  | Call site | Jobs | With events |
  |---|---|---|
  | `22b:360` (W rows, 2 calls) | 1308 / 1213 | 0 / 0 |
  | `24:853` (FB3) | 38 | 0 |
  | `24:931` (FB4) | 4 | 0 |
  | `24b:577` | 4 | 0 |
  | `25ab:993` (Z5) | 3 | 0 |
  | **`25bb:1099` (Z6a)** | 3 | **3** |
  | Stage 4 (AG1, AG2, AG3, AG4) | 5 / 4 / 102 / 2 | 0 |

  Jobs with events go to `runLoopEvents` (eg::render's event splits). Jobs without events keep `eg22::runLoop`, so their bytes are unchanged by construction. **Z6a** gains `freeze engages ≥ 3` (3 at every rate).
- **(c) C7 calibration.** A 40 M-step dependent FMA chain is timed before and after each run. A ratio > 1.10 vs the session's first calibration counts as contended. Final run: **cal 0.786 ns/op, max ratio 1.006**, 0 slowed; C7 median 17.03 % (spread 0.11), C8 17.07 % (Δ vs C −0.12). The timing thread was already `QOS_CLASS_USER_INTERACTIVE`.
- **(d) W2o** written into the `engine_gates_22b.h` notes: *a seeded row that moves gets a wider seed set, never a wider threshold.*

### D → final movers (P152)

| Row | D | Final | Cause |
|---|---|---|---|
| Z6a | 48 k wet 1.5055 / out 8.1469 / hits 15132, no engage clause | 48 k wet 1.4432 / out 8.4729 / hits 15638, **engages 3** (44.1 k and 192 k likewise) | **By design:** its 6 freeze toggles now fire (P149) |
| M7c (counter row) | scrubs `1/2/2/1` | scrubs `1/1/2/1` | The Tempo/var render injects a non-finite host sample. The env input now pins it to 0 instead of letting the follower go non-finite and scrub (D47 a). This is outside the "bitwise ≤ +80 dBFS" range R151 claimed. Attributed, not a defect |
| new rows | — | AG*, US1/US1n, Z7h3e/Z7h3n, PR*, ST*, LY* | — |

N-suites, R12, HA, codec bytes and the block-size nulls (N9 / N10, in the harness) are unchanged vs D.

## FUNC-09 — 16 factory presets (D42)

`Source/FactoryPresets.h` holds the spec table (engineering units, choice indices, a fixed `int64` seed each, `customJson` for the 19-EDO preset). `makeFactoryPresets` converts each value once through the parameter's own `convertTo0to1`. It is wired in the processor constructor (`initializeFactoryPresets`, after the existing custom-state / excluded / validated-apply setup). `output_gain` is never present.

| Preset | RMS 2–20 s (dBFS) | PR3 |
|---|---|---|
| Ambient · Cloud Drift | −18.3 | tail 3.95 s |
| Ambient · Frozen Halo | −16.6 | tail 3.95 s |
| Ambient · Slow Shimmer | −19.5 | tail 3.95 s |
| Ambient · Dust Bloom | −20.1 | tail 3.95 s |
| Harmoniser · Major Choir | −20.2 | 1278 follow hits, 0 off-scale |
| Harmoniser · Fifths | −20.6 | 878 hits, 0 off-scale |
| Harmoniser · Just Halo | −18.3 | 796 hits, 0 off-scale |
| Harmoniser · 19-EDO Haze | −20.6 | 715 hits, 0 off-scale (masked custom degrees) |
| Rhythmic · Stutter 16 | −18.8 | lock 120 / 93; grid 160/160 |
| Rhythmic · Triplet Dust | −22.2 | lock 120 / 93; grid 120/120 |
| Rhythmic · Gate Pulse | −22.6 | lock 120 / 93; grid 40/40 |
| Rhythmic · Onset Spray | −20.9 | lock 120 / 93 (onset-triggered: no grid check) |
| Cascade · Infinite | −20.6 | g 1.000, LG −0.89 dB |
| Cascade · Rust Spiral | −20.7 | **g 0.907** (D43 engaged), LG −1.36 dB |
| Cascade · Pitch Ladder | −21.1 | g 1.000, LG −2.38 dB |
| Cascade · Dark Bloom | −19.1 | g 1.000, LG −0.89 dB |

**Loudness spread 5.94 dB** (≤ 9). The first voicing was 9.28 dB. Frozen Halo's stream levels came down 3 dB, and Gate Pulse's s1 / s2 levels went up 4 dB. Balanced by voicing, never by `output_gain`.

**Descriptions** (docs only; also in CHANGELOG `## [1.0.0]`): see `CHANGELOG.md` → Added → 16 factory presets.

Gates: **PR0** 16, 4/4/4/4, longest 24 characters, no `/\:`, no `output_gain`, in range, seeded. **PR1** 16/16 × 220 params; `output_gain` −7 dB is kept. **PR1n** a linear-fraction `s1_size` reads back 40.36 → FAIL as required. **PR2** finite, peak ≤ √(in² + A²). **PR4 240/240** A → B == fresh B (10 s bitwise), and the 16 fresh renders are pairwise distinct. **PR5** under the harness temp home: 16 named files + sentinel, `loadPreset` read-back bad 0, user copy of `Cascade · Rust Spiral` == the factory apply (10 s bitwise); `·` survives the sanitiser on disk (macOS; watch at `/publish` for Windows).

## COMPAT-02 — state round trip

**ST1** 3 random full sets (all 16 routes, seeds 2^53 + 1 / 0xF00DFACECAFEBEEF / 0x0123…, edo 19, 2 masks, zh-Hans) → a fresh instance. Decoded params match (worst relative diff 1.77e-7, float text precision), the non-param state matches, and the 10 s renders are bitwise, 3/3. **ST2** two reopens with a change: C holds edo 22 / seed 0x2222. **ST2n** (`testNoStateStrip`): C reads edo 19 / seed 0x1111 → FAIL as required. **ST3** the same sets through `capturePresetData` → `applyPresetData`, 3/3; `output_gain` is kept. **ST4** (`tests/state-roundtrip/two_reopen.py`, installed VST3): 12/12.

## COMPAT-03 — layouts

**LY1** mono→mono, mono→stereo and stereo→stereo × 44.1 / 48 / 96 k, 10 s: accepted, finite, non-silent, L ≠ R on stereo outputs, 0 scrubs (48 k: −19.8 / −19.8, −19.4 / −19.5, −19.4 / −19.7 dBFS). **LY1n**: stereo→mono, 5.1→5.1 and a disabled input are refused. Windows VST3 → release CI at `/publish` (sign-off 4): **complete-pending-CI**.

## UI-04 — gates

Page unchanged (`cmp` with `mockups/v2-ui.html` / `v2-i18n.js`). Overrides ×2 identical and == committed. `ui_frontend_check` + NCs, probe 220/220 ×2 + NC, check-i18n, fr-lint, zh-lint, check-ui-labels, boot-all-uis `--strict-tips`: all rc 0. **PF1** (`tests/preset_name_fit.js`): the widest name, `Harmoniser · 19-EDO Haze`, is 150 / 172 px in EN, FR and zh-Hans; all 16 fit `.pm-item`. **PF1n** a 40-character name is 231 / 172 → overflows.

## Release

`CMakeLists.txt` VERSION **1.0.0**. `CHANGELOG.md` has a single `## [1.0.0] - 2026-11-07`. The bump does not reach the state bytes: **codec 9/9 == B (no B′)**. Install: the factory bank was cleared first. The listing showed only `Factory/`, 16 files at 0.1.0 written by this session's param-dump run, and no `User/`. build-and-install: 0 warnings; Standalone rebuilt; auval PASS; pluginval 10 VST3 + AU PASS. **No tag.**

---

## Deviations

1. **Carried-extract masks (T0).** The 3.3 INFO class (processors seeded from `Random::getSystemRandom()`) covers more rows than the 5 that 3.3 listed: **MT5**'s `normal` and `next` values also vary between runs. `extract4.sh` masks LG1 hot, MT2n fb 80, MT4 / MT4n out, TL33 out and MT5 normal / next. The verdicts are untouched. With the masks, T0 == the 3.3-verify extract exactly.
2. **AG1 corner.** P145 named the FB4 **drive-100** corner. The grain drive's own tanh keeps that loop below unity: env 0.42 and g ≡ 1 under D43 too, so it cannot show the AGC. AG1 / AG1n use the **LP-res-100** corner, the FB4 corner that runs away. The drive corner stays in AG6.
3. **AG5a seeds.** The first run gave 295.93 s vs legacy 295.58 s. The cause was that `make()` seeds each processor from the system, so the two arms drew different grains. Both arms now share the seed, and the times are equal (295.77 s). Z7d's own config is sat 100 %; AG5a runs it at the default 20 %, and the AGC does not engage there.
4. **AG4 stimulus.** The harness had no kick/snare synth. `eg4::DrumLoop` was added: a 120 bpm kick (150 → 45 Hz) on 1 + 3, a snare (190 Hz + hashed noise) on 2 + 4, a hat on the eighths, normalised to a 0 dBFS sample peak, position-deterministic.
5. **Z7h3 row name.** The Env row is **Z7h3e** because the existing host-guard row is already named "Z7h3 host 1e30 …". Its predicate compares the pinned-cell count for the L-only case and requires the trace to be bitwise for L+R / mono. In the L-only case the pin also drops R's half of that one sample, so the trace cannot be bitwise there.
6. **M7c env-scrub counter** moved 2 → 1 (see D → final). R151's "every carried row stays bitwise" did not cover non-finite host input; the change is the approved predicate itself.
7. **C7 `[NOVERDICT]` rule.** P150 says "< 5 valid → NOVERDICT (the existing path)". The existing path is **< 7 valid of 7** (3 replacements). It was kept as is (stricter); slowed runs count as contended.
8. **PR / ST / LY header.** These sections are in `engine_gates_4b.h`, not `engine_gates_4.h`, so the T2 commit stayed clean.
9. **Harness home.** The constructor now writes the factory bank, so the harness re-execs itself under a per-run temp home (`CFFIXED_USER_HOME`; JUCE's `NSHomeDirectory()` ignores `HOME`). It verifies the redirect before building any processor and refuses to run otherwise. The real `~/Library/O-Granulator/Presets` was never touched by the harness. The **param-dump** tool (not the harness) constructs the processor and wrote a 0.1.0 `Factory/` there at T3. That bank was listed and cleared at T5 before the install.
10. **Decoded-value comparisons (PR1 / ST1 / ST3).** An absent skewed parameter re-applied by the manager lands about 2e-7 from its declared default. XML / JSON float text also brings a skewed value back an ulp or two away (worst 1.77e-7 relative), while the 10 s renders stay bitwise. Params are compared decoded within 1e-6 relative; the bitwise render is the strict arm (memory pattern_vst3_raw_value…: round-trip on decoded values).
11. **T3 + T4 share one commit.** `PluginProcessor.cpp` carries both the preset wiring and the ST2n hook.
12. **PF1 location.** `tests/preset_name_fit.js` is a separate Playwright script, not part of `ui_frontend_check.js`, because that script is static (no browser). It uses the same `serve-ui` stub as the probe.
13. **Standalone screencapture with the menu open (P159)** was not taken. The WebView menu needs a click inside the page, which the capture can't drive. It moved to HANDS-ON-4 H1 / H4, where Taylor opens the menu in the real Standalone.
14. **Cascade · Infinite** is at sat 0 %, so the AGC stays at g 1 on the standard stimulus; its PR3 arm passes on LG ≥ −3 dB. Rust Spiral (sat 60 %) shows D43 engaging (g 0.907).

## Negative controls (each FAILs its predicate)

AG1n (g ≡ 1), Z7h3n (4125 pinned cells), US1n (30 grains, frozen), PR1n (s1_size 40.36), ST2n (edo 19 / seed 0x1111), LY1n (all refused), PF1n (231 / 172 px), ufc NC1 / NC2, probe NC (rc 2).

## Requirements

FUNC-09, COMPAT-02 and UI-04 → **complete**. COMPAT-03 → **complete-pending-CI** (Windows at `/publish`). QUAL-04 → **partial** (DAW-4 + HANDS-ON-4 at verify).
