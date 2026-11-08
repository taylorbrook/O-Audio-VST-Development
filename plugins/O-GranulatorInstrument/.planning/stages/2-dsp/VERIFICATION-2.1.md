# Stage 2 — DSP · Phase 2.1 (Slots, tape, voices, one stream) — Verification

## Verification Date

2026-11-07

I re-ran every automated check below myself in the verify session, against `06d7be7b`. First I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp`, rebuilt the harness and param-dump, and reinstalled with `build-and-install.sh`. No automated result here is copied from SUMMARY-2.1.md. HANDS-ON-2.1 is a human gate and is **not** self-certified here.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.1 / ROADMAP Phase 2.1)

1. The load-bearing skeleton: loader → MipTape → audio-owned swap. This includes the Kaiser offline resample, the 30 s / 2.88 M-frame cap, and the retire + reaper path working while the host idles.
2. The voice allocator (20 slots, 5 ms steal) and the custom amp ADSR.
3. One stream per voice (`s1`): Free scheduler, 3-mode scan, key-tracked pitch (±48 st), region-fit spawner, Hann renderer, the per-voice output law, voice headroom, wet safety and output gain.
4. An absolute 32-sample grid with MIDI applied at sample offsets. No FX and no feedback; the return path is present and carries zero.
5. D11: fork 13 leaves at `82406fce`, with FORK-MANIFEST and a drift check that has working negative controls.
6. D12: water-fill + onset thinning on the voice axis. Live grains ≤ `grain_limit`, no starvation, and a lowered cap converges.
7. D13: `DevLoaderEditor` behind `OGI_DEV_LOADER`, on the plugin target only, using the real load API and safe against a chooser outliving the editor.
8. Stage 1 regression floor (P10): 55 rows green and param-dump byte-identical.

### Deliverables (SUMMARY-2.1.md)

There are 13 new DSP headers plus `SlotLoader` and `Engine` (`.h`/`.cpp`) in `Source/dsp/`, and 13 forked leaves plus `FORK-MANIFEST.md` in `Source/dsp/forked/`. The other deliverables are `PluginEditorDevLoader.{h,cpp}`, `tests/fork-drift-check.sh`, 33 new harness rows (`engine_gates_21.h`) and `HANDS-ON-2.1.md`.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Loader → tape → swap | ✅ Achieved | Harness K1–K3 (DC 0, pass 0.0001 dB, stop −113.3 dB, β 2 control fires), L1/L2 (trim exact; mono L ≡ R), T1/T2 (MipTape ≡ MipRing bitwise), H1–H5 (swap fade, host-idle 100 publishes → pending ≤ 1, 0 deferred, reaper retires with 0 refs, SR crossfade, 200-op stress clean). The H2 no-entry-counter control leaks 19 ✔. Code: `SlotHandoff.h` takes the tape on the audio thread with one `exchange`; `lock` and `deferred.push_back` are off-audio only. |
| 2. Allocator + ADSR | ✅ Achieved | V1–V4 (distinct rates, 2 steals at d2 ratio 0.965, slot-permutation invariant, force-release ramp), M1 (0 stuck notes over the edge-case MIDI script), E1 (stages end within one cell, velocity law 0.0004 dB, click ≤ 1.000). |
| 3. One stream | ✅ Achieved | P1–P4 (latched rate, root ±0.0000 ¢, alias ≤ −105 dB at +24/+48 st, +48 clamp), S1/S2 (scan modes, sorted region, 10 ms minimum), Q4 (law error ≤ 0.394 dB, thinning-off control −39.7 dB). |
| 4. 32-grid + MIDI offsets | ✅ Achieved | N1: 441/512/4096/8192/variable vs 32 give 0 differing samples, and the seed control gives 191 997. N2: 236 unread params randomised → 0 differ (`s1_density` control → 383). |
| 5. D11 fork | ✅ Achieved | `fork-drift-check.sh` → 13 leaves, **0 drift**, rc 0. I re-made both negative controls in the scratchpad. NC-a (byte appended to `Grain.h` in a `--parent-root` copy of the pin blobs) → check 1 DRIFT, rc 1. NC-b (byte appended to `Quotas.h` in a `--local-root` copy) → check 2 DRIFT, rc 1. |
| 6. D12 budget | ✅ Achieved | Q0 (waterFillN bitwise vs reference, `noSort` control 5 590), Q1a/Q1b (liveMax ≤ cap at 32–256 caps; 0 steady-state R3 kills, thinning-off control 8 000), Q2 (no starvation), Q3 (256 → 32 converges; `thinningOff + d25Off` stays at 256). |
| 7. D13 dev loader | ✅ Achieved | `CMakeLists.txt:204-206` defines `OGI_DEV_LOADER=1` on the plugin target only. In the ninja compile commands of the harness and param-dump TUs there are 0 `DevLoader` / `OGI_DEV_LOADER` references. The harness TUs carry `OGI_TEST_HOOKS` (28 commands). The editor calls `proc.requestSlotLoad` (`PluginEditorDevLoader.cpp:146`), which is the same API as `SlotLoader::requestSlotLoad`. Chooser safety: the `launchAsync` callback captures a `SafePointer`, and the dtor resets the chooser. pluginval editor tests pass. The human UAF row is HANDS-ON row 12. |
| 8. Stage 1 floor | ✅ Achieved | Harness **88 PASS / 0 FAIL**, including all 55 Stage 1 rows. param-dump is byte-identical to `.planning/params.tsv`. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.1 of 6). A requirement verified at stage-2 is closed only when all of its acceptance criteria are covered, including the human listening rows. Rows whose automated criteria pass but whose HANDS-ON-2.1 row is still pending are marked partial.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: MIDI instrument, polyphonic voices | must | ⚠️ Partial | Covered: V1 (4 voices, 4 distinct rates) and V2 (steal d2 ratio 0.965 ≤ 2.0, 0 ms control 5.34). Open: "across the enabled streams" needs 4 streams (2.2), plus HANDS-ON rows 2 and 4. |
| FUNC-02: 4 file slots | must | ⚠️ Partial | Covered: all 4 slots load through the real API; swap while sounding is click-gated (H1) with 0 audio-thread allocations (A1). Open: drag-drop and file browser UI (3.2), slots 2–4 audible (2.2), HANDS-ON rows 1 and 8. |
| FUNC-03: Key-tracked pitch | must | ⚠️ Partial | Automated criteria met: P2 root at kt 100 is 0.0000 ¢ (±1 ¢), and P1 kt 0 latches one rate. Open: HANDS-ON row 3. |
| FUNC-04: Scan | must | ⚠️ Partial | Automated criteria met: S1 (100 % real-time traversal, 0 % holds, Retrigger vs Free-run control, ping-pong reflects) and S2 (loop region). Open: HANDS-ON rows 6 and 7. |
| DSP-02: Amp envelope | must | ⚠️ Partial | Automated criteria met: E1 (stage timing, velocity law 0.0004 dB, click ≤ 1.000). Open: HANDS-ON row 5. Grains continuing during release are re-checked with the 2.2 streams. |
| DSP-07: Resampling / AA | must | ⚠️ Partial | Automated criteria met: K1–K3, P3 (96 k session, 44.1 k source, aliases ≤ −105 dB at +24/+48 st, margin 45.2), H4 (SR change mid-note, pitch error 0). Open: HANDS-ON rows 9 and 10. |
| PERF-01: RT safety | must | ⚠️ Partial | Covered: A1 gives 0 allocations over 252 blocks with 15 steals, 3 tape swaps, 1 SR crossfade and 44 kills; the liveness `malloc(64)` control counts 1. Open: reset and freeze (2.4), matrix edits (2.5). |
| PERF-02: Pool + CPU budget | must | ⚠️ Partial | Covered: the shared pool enforces the global cap with water-filled quotas (Q0–Q3). Open: the measured CPU budget at 16 voices × 4 streams (2.2 / 2.6). 2.1 records no CPU figure. |
| PERF-03: Off-thread load + reaper | must | ⚠️ Partial | Covered: decode and resample run on the loader thread. H2: host idle, 0 deferred. H3: retire with 0 refs; the `retireScan256` control gives 23 early retires. Open: the "reset" arm of the shared alloc criterion (2.4). |
| QUAL-01: No artifacts | must | ⚠️ Partial | Covered: click gates for note-off (E1), steal (V2/V4) and file swap (H1). Open: reset and freeze toggle (2.4), plus HANDS-ON rows 4, 5, 8 and 11. |

All other stage-2 requirements stay pending: FUNC-05, FUNC-06, FUNC-07, FUNC-09, DSP-01, DSP-03, DSP-04, DSP-05 and DSP-06.

**Requirements Summary (this phase):**
- ✅ Complete: 0
- ⚠️ Partial: 10 (FUNC-01–04, DSP-02, DSP-07, PERF-01–03, QUAL-01)
- ⏸️ Deferred (later phase or stage): 18
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after touch-rebuild; 0 warnings in the build log. |
| build-and-install.sh | ✅ Pass | rc 0, 19 s. Exactly `O-GranulatorInstrument-dev.{vst3,component}` are installed (22:44). No alternate variant on disk. |
| Render harness | ✅ Pass | `88 PASS, 0 FAIL`, rc 0, Phase 2.1 block 6.7 s. All 33 Phase 2.1 row IDs are present, and 22 `ctrl FIRED` lines appear with none failing to fire. |
| param-dump | ✅ Pass | Byte-identical (`cmp`) to `.planning/params.tsv`. |
| Fork drift check | ✅ Pass | 0 drift / 13 leaves. NC-a and NC-b were re-made independently and both return rc 1. |
| Console exclusion (P4) | ✅ Pass | Harness and param-dump TUs: 0 dev-loader references. Plugin target: `OGI_DEV_LOADER=1`. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. The `ZZZZ` control prints "didn't find the component". |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0; `Main bus num input channels: 2`. |
| pluginval 10 AU (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0; `Main bus num input channels: 2`. The only `!!!` line is the informational "Current program is -1". |
| Git | ✅ Pass | No uncommitted changes under `plugins/O-GranulatorInstrument` before this verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

`stages/2-dsp/HANDS-ON-2.1.md` (13 rows) is **pending**. Run it in the Standalone and in one DAW (Logic, AU). These rows close the partial requirements above:
- [ ] Rows 1–9 and 11–13 in the Standalone and the DAW
- [ ] Row 10 (session rate change) in the Standalone only
- [ ] Row 12, the chooser-UAF row: close the editor while the chooser is open → no crash and no load

## Issues Found

- **None blocking** in the automated scope.
- **Not measured in 2.1: CPU.** PERF-02's budget figure must come from 2.2 (first 4-stream load) or 2.6 at the latest.
- **Carried, by design:**
  - The goldens re-baseline in 2.2 (P11).
  - `kVoiceGainDb −6 dB` is provisional and is tuned in the 2.2 listening pass.
  - `kModSources 12 / kModDests 113 / kSrcTelemetry 11` stay at parent values until 2.5.
  - Slots do not persist until 2.6.
  - `DevLoaderEditor` is removed in 3.1.
  - Module-gap G5: `MipTape` duplicates the grain-engine halfband.
- **Plan deviations reviewed and accepted:** the pitch formula follows ARCHITECTURE :407, Kaiser uses 64 zero-crossings (it is offline only, and with 32 K2 fails), `kMaxReachS` / `kRingSeconds` are adapted and recorded in the manifest, and the full-pool steal of a killing grain now pays its kill (Q3 proves it converges). The gate-stimulus changes in Q1b, Q2, H3, E1 and T1/T2 are stimulus fixes, not threshold changes, and each keeps a firing control.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`) do not belong to this plugin and are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is verified. The HANDS-ON-2.1 human gate is pending.

**Ready for next phase:** once HANDS-ON-2.1 comes back all ✅. Then Phase 2.2: 4 streams, randomisation, window morph, scale, quotas + thinning on the stream axis. Any ❌ → `/plugin-execute O-GranulatorInstrument 2-dsp` to fix 2.1.

**Blockers:** HANDS-ON-2.1 (human).
