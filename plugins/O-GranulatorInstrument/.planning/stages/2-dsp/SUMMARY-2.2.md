---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.2"
title: "Four streams, randomisation, windows, scale, quotas + thinning"
executed: 2026-11-08
version: 0.1.0 (unchanged, PD16)
fork_pin: 82406fce
result: executed — automated gates green; HANDS-ON-2.1 + HANDS-ON-2.2 deferred by the user until the Stage 3 UI
d14: OVERRIDDEN by the user 2026-11-08 ("skip ahead I'll test everything once UI is in place")
---

# O-GranulatorInstrument — Stage 2 / Phase 2.2 SUMMARY

The engine now has full grain-parameter parity with O-Granulator, across **4 streams × N voices sharing one pool**:
- **Scheduling:** each counted voice runs one Free-mode scheduler per enabled stream (≤ 80) with telescoping jitter. Onsets merge in (offset, serial, stream) order, and stream N reads slot N.
- **Spawner:** latches spray, reverse, stereo spread and size / gain / pitch rand. Pitch rand is continuous, or an `ActiveSet::pick` under a scale with per-stream / per-collection degree masks and the shared custom scale. The spawner also latches the forked `WindowMorphBank` window + skew.
- **Quotas:** the water-fill runs over ≤ 80 (voice, stream) pairs with jitter-aware integer thinning and (k,s) steal priority.
- **Output:** a coherence-(b) output law per (k,s) feeds a 4-stream mixer with 10 ms enable / mute / solo ramps.
- **Scale state:** published lock-free (relaxed atomic masks + the parent's non-blocking try-lock table copy). It round-trips through the sibling `<CustomScale>` / `<DegreeMasks>` children.
- **Proof against the parent:** DSP-01 is proven sample-for-sample against O-Granulator at `82406fce`, built out of tree.
- **Voice gain:** `kVoiceGainDb` is now measured, at **+0.02 dB**; the 2.1 placeholder was −6 dB.

## D14 override (recorded)

D14 gated 2.2 execute on HANDS-ON-2.1 being all ✅. HANDS-ON-2.1 was still blank. On 2026-11-08 the user chose to **skip ahead and run all hands-on testing once the Stage 3 UI is in place**. T1 logged the override. HANDS-ON-2.1 (13 rows) and HANDS-ON-2.2 (10 rows) are both open and will be run together after Stage 3. No 2.1 fix landed after `06d7be7b` (`git log 06d7be7b..HEAD -- Source tests CMakeLists.txt` was empty), so the T1 re-check map was not triggered.

## Results

| Gate | Result |
|---|---|
| Build | render-harness / param-dump / VST3 / AU / Standalone rc 0. **0 warnings** in plugin sources. |
| Default harness | **117 PASS / 0 FAIL**: Stage 1 55 + Phase 2.1 33 + Phase 2.2 29. 0 "NOT FIRED" controls. |
| Parity harness (`--parity`) | **41 PASS / 0 FAIL** (T11), plus GV1 PASS after T12. |
| param-dump | **Byte-identical** to the 2.1 capture and to `.planning/params.tsv`. |
| Fork drift | **0 drift** over 13 leaves. NC-a (byte flipped in a scratch parent tree) → check 1 fires, rc 1. NC-b (byte flipped in a scratch local leaf) → check 2 fires, rc 1. |
| X1 (compdb exclusion) | 151 O-GranulatorInstrument entries → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. Positive control: the parity project's compdb has 91 `O-Granulator/Source` entries. |
| X2 (symbol exclusion) | `nm -C` finds **0** `ogran::` symbols in the installed VST3, the installed AU and the harness. Control: the parity binary has 76. |
| auval | `aumu OuGI OuDv` **AU VALIDATION SUCCEEDED**. `ZZZZ` control rc 2. |
| pluginval 10 | VST3 ×2 + AU **SUCCESS** (2 ch in) |
| Standalone | alive after 6 s |
| Install | `build-and-install.sh` swept both variants; only `O-GranulatorInstrument-dev.{vst3,component}` are on disk |

### Re-baseline (P9 / PD10, RB1 / RB2)

T2 hashed 7 jobs × {pinned, default} on the **unmodified 2.1 engine**, before any `Source/` edit: `tests/golden/renders-2.1.sha256`. Two dumps were byte-identical, pinned = default on 7/7 (2.1 ignores every 2.2 control), and the seed + 1 control fired. The only later change to `render_hashes.h` is the `prepareJob` body (pinned → `coherenceOff`, `voiceGainDbOverride −6`). That was verified with a `diff` against the T2 snapshot.

| Job | `-pinned` 2.1 → 2.2 | `-default` 2.1 → 2.2 | Why the default changed |
|---|---|---|---|
| N1 | same | differs | defaults honoured (spray 10 / jitter 20 / spread 30, coherence (b)) + voice gain +6.02 dB |
| V3 (grain log) | same | differs | defaults honoured only. The log carries no gain, so this line is the "defaults honoured" proof on its own. |
| Q4, E1, H1, P2, CH | same | differs | defaults honoured + voice gain |

The split between "defaults honoured" and "voice gain" is shown by V3 (a log-only hash, which moves with the defaults alone) and by GV1's −6 dB override control, which isolates the gain at 6.02 dB. `tests/golden/renders-2.2.sha256` is the one allowed re-record. An ear trim from HANDS-ON-2.2 row 6 would re-run GV1 + RB2.

### DSP-01 parity (PN / PS / SC, `tests/parity/` + `--parity`)

- **PN01–PN26 null:** residual between **−122.9 and −132.0 dB** (threshold −90 dB) after least-squares gain alignment. Every row has equal grain counts, our `p0` equals the closed form on every grain, and the landed raw size / density are identical on both sides.
  - The rows cover size 20 / 100 / 500 / 2000, density 1–200, window steps 0 / 11 / 21 / 32 / 43 / 53 / 64, skew ±100, pitch −24 … +24 (mip levels 0 / 1 / 2), position 0.25 / 0.75, reverse at levels 0 and 1, and stream s3 (slot 3).
  - The fitted gain is 1.995 (= the −6 dB placeholder) at N ≤ 2 and smaller at N > 2 (the law ratio). It is constant per row, as R3 predicted.
- **Controls, all fired:**
  - PNc1 (our onset + 1 sample): −0.2 dB.
  - PNc2 (our window step + 1): −54.5 dB.
  - PNc3 (parent Δ + 1 at ℓ1, the mip-phase term): −3.6 dB.
- **Parent self-checks:** every PN grain has `anchor == A` and `writeHead == W0`; 28/28 params landed on every row; every reference RMS ≥ −70 dBFS. Our own geometry derivation was never tuned against these. The only geometry change came from the self-check itself; see deviation 1.
- **PS1–PS4 statistics** (n ≥ 4000 per side), every ×0.5 control firing:
  - spray: D2 0.016
  - size_rand: D2 0.014
  - spread: D2 0.011
  - pitch_rand Off: D2 0.028
  - reverse: |Δp| 0.007
  - gain_rand: D2 0.020
  - jitter displacement: D2 0.013, range ±0.250
  - jitter even-k χ²: ours 19.9, parent 15.1, threshold 63.7. The engine-side control `jitterScale 0.5` also fires.
  - scale-on picks: 15 members; χ² uniform 25.4 / 17.9, two-sample 28.9, threshold 55.5.
- **SC1:** 864/864 ActiveSet cases are bitwise equal to the parent (choices 1–30, custom 19-EDO and 64-degree, 3 masks, 3 spans, 3 pitches). Control: our mask bit 0 flipped → 864 differ.
- **SC3:** 5/5 custom texts are bitwise equal (table, error key, badToken), and the bad token keeps 12-EDO.

### D16 voice gain (PD1)

- **GV0 measurement:** target **+0.0212 dB**. The 4 seeds gave +0.025 / +0.010 / +0.019 / +0.031 (spread 0.021 dB, threshold 0.2), and old-vs-old is 0 dB.
  - The reference is the parent s1 at defaults, live, mix 100 %, output 0 dB.
  - Ours is 1 voice at root 48, vel 127, defaults.
  - The material is −30 dBFS RMS noise; RMS is taken over 0.5–20 s.
- **Constant set:** |target| ≤ 1 dB, so PD1's STOP did not fire. `kVoiceGainMeasuredDb = 0.02` (it was −6.0), `kVoiceGainEarTrimDb = 0.0`.
- **GV1 verification:** error **0.001 dB** (threshold 0.5). Control: override −6 dB → 6.020 dB.
- **R7:** a single note is now about 6 dB louder than in 2.1, and chords reach the §18 knee sooner. HANDS-ON-2.2 row 6 confirms this or sets a trim.

### Engine gates (eg22, 29 rows; every control fired)

| Row | Metric · margin | Control |
|---|---|---|
| RB1 | 7/7 pinned hashes = 2.1 | 7/7 default differ |
| W0a / W0c | P2, N1 and a P2 replica = 2.1 bitwise; law 0/1000 unit, 0/2969 engine voice-ticks differ | step 1 → 997/1000 differ |
| W0b | `forceBankPath` hash ≠ 2.1 (P5) | — |
| QS1 | live ≤ cap at every tick (8/8 settings); 0 steady R3 kills at 256 / 128 (J 0, J 20) and 64 / J 0 | `thinningOff` cap 128 → 32000 kills |
| QS2 | q (63, 1) and (60, 4); s2 reaches m; s1 62 ≤ 63 and 59 ≤ 60 | s2 max 1 < 4 |
| QS3 | B 8 ≥ 8 and A 8 ≤ 8 on all 4 streams | 4/4 starved |
| QS4 | 256 → 32 converges (thinned, and unthinned + D25) | stays above cap |
| QS5 | (61, 1, 1, 1) unit test + engine | `noSort` → 0.8 each |
| QS6 | `<80>` ≡ `<20>`: 0/10000 differ | 10000 differ |
| QS7 | muted keeps q bitwise, 156 spawns, output 0, unmute 0.000 dB; disabled q 0, 0 spawns | muted asserts on disabled render fail |
| LF1 | worst −0.03 dB over 10 gated cells · 1.471 dB | `lawOff` 15.04 dB |
| CO1 | 0.061 dB · 1.439 | `coherenceVerbatim` −21.19 dB |
| CO2 | 0.001 dB · 1.499 | `coherenceOff` 21.18 dB |
| MX1 | d2 ratio ≤ 1.00 · 1.000 | `mixerRampZero` 179.7 |
| V2s | 2 steals (serials 2, 3), d2 0.774 · 1.226 | `stealRampZero` 6.07 |
| FS1 | 16/16 rates exact, grain slot = stream, streams −29.7 … −37.6 dB | s3 disabled → silent, 0 grains |
| FS2 | 19 spawns per stream in the release window | release 0 → 0 |
| SC2 | 0/100 s2 picks outside the masked window; (s2, Minor) unaffected | mask on s1 → 54 outside |
| ST1 / ST2 | 124 masks + custom exact, bit 63 kept | decimal + `getLargeIntValue` clamps |
| ST3 | absent → 12-EDO, all-on, seed kept | `testNoStateReset` → stale edo 19 |
| ST4 | second state wins, 0 custom children | `testNoStateStrip` → shadowed, 12 children |
| ST5 | 3 malformed masks ignored | valid mask applied |
| ST6 | 32 try-lock misses while held, copy unchanged, copied next tick | held vs released |
| A2 | **0 audio-thread allocations** (19 steals, 2 custom copies, 5 rebuilds) | liveness malloc → 1 |
| NB1 | blocks 32 / 441 / 512 / 4096 / 8192 / var bit-identical | seed + 1 → 127998 differ |
| V3s | 0/808 grains differ under slot permutation | seed + 1 → 808 |
| N2s | 148 unread IDs randomised → 0 differ (113/113 read IDs bound) | `s1_density`, `s4_window` differ |

INFO:
- **QS1 R3 kill rates** (PD7 / P10, expected): cap 64 / J 20 → 7.6 kills/s; cap 32 / J 0 → 12.8 kills/s; cap 32 / J 20 → 38.0 kills/s.
- **LF1 sparse cells** (N_eff < 1, the law floors at 1): d1 / 100 → −10.03 dB, d1 / 500 → −3.01 dB.

### T13 validators + CPU

- pluginval 10 (`--timeout-ms 600000`, one at a time, Logic closed): VST3 ×2 **SUCCESS**, AU **SUCCESS**, `Main bus num input channels: 2`.
- **C22 (INFO, PD15)**, run alone: 48 k / 128, 16 notes × 4 streams × 4 slots, d 200, size 1000, cap 256, no FX → **median 14.36 %** of real time (min 14.22, max 14.45) over 7 × 10 s. Mean live 243.2 (saturated ≥ 240: yes). Duty witness min 0.993. Bypass (streams disabled) 0.301 % → ratio 47.7×. This is below the 28 % flag. The verdict belongs to 2.3, after the FX are added.

## Plan decisions

PD1 (user), PD2 (user) and PD3 (user) were applied as written. So were PD4 (sibling `<CustomScale>` / `<DegreeMasks>`, which **refines CONTEXT D17's "keys on `<Engine>`"**; `parameter-spec.md` was not edited), PD5–PD9, PD10 (+ the pinned `voiceGainDbOverride −6`), PD11, PD12 (level / pan ramps stay 20 ms), PD13–PD16 (version 0.1.0, no tag, `CMakeLists.txt` not edited: `GiScale.h` and every test file are headers, and the parity build is out of tree).

## Deviations (with reasons)

1. **Parity geometry: buffer_length 8.25 s, F 128 088, T_f 396 000.** The first run used 15 s. `buffer_length` is skewed (0.5), so 15 s landed a few ulps off and S came out non-integer. The anchor self-check caught it on every row (anchor = A + 0.0114). 8.25 = 1 + 29·0.5² lands exactly. F and T_f were re-solved under the same R§2.3 constraints. This is a stimulus fix, not tuning: the closed form is unchanged.
2. **PS3 length 5.02 s** (was 5 s). Displacements drop k = 0 per sequence, so 5 s gave n = 3998 < 4000. **Jitter onset time = `onsetSample − frac`**, per R§3.1. A trial with `+ frac` was rejected: it shifted the range by 0.004.
3. **Engine (T4–T8):**
   - The `[20][4]` readouts are named `*VS`, so the 2.1 `[20]` hooks keep their meaning.
   - Schedulers also run for streams whose slot has no tape: demand 0 and an early return, as in 2.1.
   - The law also recomputes on per-stream coherence-input changes.
   - A disabled (k,s) pair uses uncapped m.
   - `mixerRampZero` is a step at the cell boundary.
   - `jitterScale` also feeds the PD7 bound.
   - When nothing is audible, the mixer outputs stream 0's term, which keeps the 2.1 sign of zero.
   - The epoch fold on an enable edge applies to counted voices only.
   - Reverse uses `reflectInto (x, lo + reach, hi)`, plus a reverse wrap in the spawn, main and crossfade reads.
   - The parent `scaleStateRevision` is not ported; it is editor-only and arrives in Stage 3.
4. **eg22 gates:**
   - MX1 / V2s use spray 30 %: without spray the sine grains cancel to ~1e-9, which made the controls vacuous.
   - QS2 adds an N = 4 arm that carries the control.
   - QS2 / QS5 engine q is checked bitwise against `waterFillN` of the engine's own demands, because the skewed density / size land at 10.0000019.
   - QS7 compares unmute against an unmuted reference cell.
   - FS1 measures stream level by mute-difference.
   - SC2 reads the mask {0, 4, 7} as C / E / G (Major degree indices {0, 2, 4}).
   - ST1 covers 124 masks: choice 0 (Off) has none.
   - ST3's second blob also drops `<Engine>`, so "seed kept" is actually tested.
   - A2 omits A1's rate-change leg; A1 still covers it.
   - LF1 uses spray 10 % so the seeds produce different renders.
5. **eg21 (R21):**
   - `base()` += `pins22()`, and the `Session` ctor sets `coherenceOff`, as planned.
   - N2 now holds every ID outside `readSet22()` (113 read) as unread, with its 0-differ threshold unchanged.
   - `readSet21()` was removed (it was unused).
6. **render_hashes.h** keeps a frozen `base21()` copy, so T9's `base()` change cannot leak into the `-default` jobs.

## Carried

- HANDS-ON-2.1 (13 rows) + HANDS-ON-2.2 (10 rows): deferred by the user to after Stage 3. Both are **not self-certified**.
- `kVoiceGainEarTrimDb` waits on HANDS-ON-2.2 row 6.
- Per-grain FX and the CPU verdict: 2.3. Feedback / freeze / reverb: 2.4. Modulation / MPE / tempo sync: 2.5. Record-in / `<Slots>` / telemetry: 2.6. Scale / mask UI: Stage 3.
- `DevLoaderEditor` is removed in 3.1. Module-gap G5 (`MipTape` halfband duplicate) is unchanged.
