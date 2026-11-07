# Stage 3 — GUI · Phase 3.2 (Buffer display + live grains) — Summary

**Plugin:** O-Granulator · **Stage / phase:** 3-gui / 3.2 · **GSD phase:** execute · **Date:** 2026-11-06
**Commits:** 0558099d (T1–T4) · f7d3cd01 (T5–T7) · T8 docs (this commit) · **Logs:** ${TMPDIR}/ogran-32/

## Result
The buffer strip is live. A new `dsp/UiTelemetry.h` holds a wait-free `TripleBuffer<UiSnapshot>`, a 56-byte `UiGrain`, a constructor-allocated `PeakRing` (2^18 entries) and the reader's absolute-cell `binWave` with the D31 dB map. The audio thread only copies:
- **Per piece:** the peak is read back from the ring with `copyLevel`, after `CaptureWriter::write` (whose statements are untouched).
- **At most 60 Hz:** a POD snapshot at the end of `process()`.

`readTelemetry` derives every 3.2 field from one snapshot under a reader-only SpinLock: wave, dots, span, frozen, bands and counts/limit. The torn path is gone. The page change is the single D30 band line, mirrored into v2-ui.html.

Audio did not move. UI-02 is partial; HANDS-ON-3.2.md runs at verify.

## Gate table
| Gate | Result |
|---|---|
| Entry check | PASS: 6 hashes match, `git diff a368b4ae HEAD` on Source/tests/CMake/modules/scripts empty, page + i18n `cmp` clean |
| Baseline B (T0, unmodified tree) | 474 PASS, rc 0, 7:29. Carried extract (463 rows) == 3.1 T11 exactly. Codec bytes == 3.1 B. HA 79/79 |
| Harness default (T4) | **491 PASS / 0 FAIL**, rc 0, 7:34 (474 − TL31 + 18 new rows) |
| Carried diff vs B | **TL31 → TL32 only** (attributed, R126) + the new rows. Nothing else moved; W2o unchanged |
| NCs | TB1n 9 999 torn · WV1n 6/6 missed (lag 512) · WV4n 0/50 scroll · GD1n stream-1 visible 0 · CN1n 40 dots > 32. Each fails its predicate |
| M8 / M8L | 0 allocations (var, and fresh 8192 after a 512 prepare, with freeze edges, 222 / 36 reads with grains) · liveness 1 |
| N3 / N2 / N1 / N0 `--strict`, R12 | rc 0 ×5 (max\|Δ\| 0, every control differs) |
| param-dump | diff-clean vs params.tsv |
| HA · codec bytes | 79/79, 142 untouched · 9/9 == B (installed T6 build) |
| Build | VST3 + AU + Standalone, 0 warnings |
| auval / pluginval 10 VST3 + AU | PASS (Tail Time) / SUCCESS ×2 |
| generic-overrides.json | `bands` added; byte-identical ×2; committed |
| ui_frontend_check.js | ALL PASS (new [12] rows: `bands` emitted and read; no torn path); NC1 exit 3, NC2 exit 2 |
| ui_binding_probe.js | 220/220 both ways; NC rc 2, 219/220 |
| check-i18n · fr · zh | 0 · 0 · 0 (no new strings) |
| check-ui-labels | rc 0; seed = param-dump (220) + overrides; states default + 9 |
| boot-all-uis `--strict-tips` | 0 warn, 0 failed, 0 DEAD, 0 late |
| Page | `index.html` == `v2-ui.html`; the diff vs 3.1 is the band line (1 − / 2 +) |

## New harness rows
- **TB1:** 73 M reads, 0 torn, 0 backwards.
- **WV1:** 0 misses at 44.1 / 48 / 96 / 192 k, 6 bins lit each.
- **WV2:** silence 0, −30 dBFS [0.3750, 0.3750], 0 dBFS 1.0000, −60 dBFS 0.
- **WV3:** early freeze span 2.0067 s (8 s buffer, held 2.0067 s); late freeze 4.0000 s.
- **WV4:** frozen 50/50 identical (464 bins lit); live 50/50 whole-bin scrolls.
- **GD1:** 32 dots checked against the spawn-log reference readAbs, worst 5.9e-8 s; 8 stream-1 dots.
- **GD2:**
  - Spread 0: pan 0, gain 1, pitch 7, all exact; Hann env worst 2e-7.
  - Spread 100 + gain_rand 50: panGains inverse worst 6e-8; bank env exact.
  - S1 pan −100: 8/8 dots at pan −1.
- **CN1:** sum == census every frame; maximum sums 32 / 64 / 96 / 128; the lowered case holds at 32.
- **BD1:** 94 frames; gp mismatches 0; S1 off base in 93 frames; streams 2–4 bitwise equal to the unrouted render.
- **TL32:** 24/24. **TL32u:** false before prepare.
- **INFO FB1:** frozen band [0.30, 0.50] vs measured spawn delays [0.384, 0.578], an offset of +0.08 (R124, accepted).
- **INFO PL1:** widest frame 15.8 KB @ 48 k, 16.0 KB @ 192 k. This is < 32 KB, so **no quantisation** (P119). readTelemetry costs 10 µs @ 48 k and 18 µs @ 192 k (30 s span).
- **C7:** 17.59 % (B) → 17.02 % (T4). The publisher is in the noise.

## Deviations
1. **Census accessor.** CN1's census adds a test-only `Engine::getSpawnerForTest()`. Pending kills are excluded from `Spawner::acquire` pass 1, so an independent census needs them.
2. **CN1 caps.** CN1 uses the real caps, 32 / 64 / 96 / 128. The plan said "16 / 32 / 64 / 128", but `grain_limit` is 32·(1 + choice). The GL3 lowered case is added as a fifth case.
3. **CN1 / BD1 block lengths.** Renders in CN1 / BD1 use whole 1024-sample blocks, and the CN1 lowering event sits on a block boundary. A trailing or split block shorter than fs/60 does not publish, so the snapshot would lag the pool for that frame. This is expected throttle behaviour, not a defect.
4. **Equal-gain pan.** Equal L/R gains map to pan exactly 0 in `readTelemetry`. Otherwise `atan2(1, 1)·4/π − 1` leaves a 1-ulp residue, and GD2 requires an exact 0.
5. **WV4 split.** WV4 has a frozen arm (as planned) plus a live whole-bin-scroll arm. WV4n is the head-anchored fractional control on the live arm, because a frozen head does not move, so the control would be vacuous there.
6. **PL1 measurement.** PL1 reports readTelemetry cost, not the audio-thread publish cost, because `publishUi` is private. The C7 CPU row covers the audio side (17.02 %).
7. **Override counts.** The overrides keep counts at 128 × 4 (the widest per-stream readout for the label gate) instead of the plan's 32 × 4.
8. **Dump change and final run.** The `--dump-ui-overrides` `bands` change landed after the T4 harness binary was built. It is test code only, on the dump path. Overrides were regenerated with the rebuilt binary, and no final full harness re-run was done: the T4 binary's rows are identical code.
9. **T0 N-suites.** N-suites were not re-run at T0. The tree was byte-identical to the 3.1-verified tree, which VERIFICATION-3.1 ran, and the T4 run is the proof. HA and codec bytes ran at T0 on the installed 3.1 build.
10. **Extract cleaning.** The carried extract strips `[PresetManager] Preset saved / loaded` lines first. In T4 they were interleaved into the M7L row text (a stdout buffer boundary); the verdict and value are unchanged.
11. **Muted Standalone screenshot.** In the Standalone screenshot (ref-3.2/standalone-t6.jpg), JUCE mutes the input ("Audio input is muted to avoid feedback loop"). The strip draws the head, ruler and S1 band, but the wave is flat and the readout shows 0 / 128. HANDS-ON-3.2 covers live input.
12. **states count.** i18n states are default + 9, not the plan's "+ 8", matching 3.1 deviation 7.

No module, scripts or contract edit except REQUIREMENTS UI-02 → partial.

## Carried
- **3.2 verify:** HANDS-ON-3.2.md.
- **3.3:** meters, LFO phase, onset, loop gain, the mod-matrix view and rings, source meters, and the DAW pass (D27).
- **Stage 4:** Env 1e30 pin.
- **Harness items:** C7 calibration and the Z6a item.
- **W2o rule.**
