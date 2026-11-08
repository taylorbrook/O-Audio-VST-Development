# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.3 — CONTEXT

**Date:** 2026-11-08
**Source:** Interactive discuss session (manual mode)
**Participants:** User, Claude
**Contracts:**
- `parameter-spec.md` (LOCKED, 261 params; `sN_filter_type/cutoff/rand/res`, `sN_drive`, `sN_crush`, `sN_crush_prob`, `sN_ring_freq/rand/mix`, dest indices b+17…b+25)
- `REQUIREMENTS.md` (DSP-03, PERF-02; PERF-01 and QUAL-01 arms)
- `research/ARCHITECTURE.md` §14 (Per-Grain FX, forked verbatim), §13 render order, Algorithm §6, Implementation Risks → CPU (budget + levers), Stage-2 Gate Catalogue
- `ROADMAP.md` § Phase 2.3
- `stages/2-dsp/CONTEXT-2.1.md` (D11–D13), `CONTEXT-2.2.md` (D15–D17), `SUMMARY-2.2.md`, `VERIFICATION-2.2.md`

## Goal (ROADMAP Phase 2.3)

DSP-03 (per-grain FX) and the PERF-02 CPU budget verdict at 16 voices × 4 streams.

- Wire the already-forked `Source/dsp/forked/GrainFx.h` (TPT SVF, drive, crush, ring) into the spawner (latch at spawn) and renderer (stages per sample), in the ARCHITECTURE §13 order: read → SVF → drive → crush → ring → window·gain·pan.
- Latched `active` flags per stage; NaN kill on SVF state; FX state reset on pool-slot reuse.
- CPU verdict (D19, D20) with the ARCHITECTURE lever ladder (D21).

Still excluded: feedback / Freeze / Reset / reverb (2.4), modulation / voice modes / MPE / NE / tempo sync (2.5), Record-in / `<Slots>` / factory sources (2.6).

## Decisions (locked this session)

### D18 — DSP-03 proof: parent null + law rows + statistical arm
- **Null arm (extends the 2.2 `--parity` mode):** same derandomised setup as D15 (1 voice, root key, keyTrack 100 %, scan 0, feedback 0, one stream). Every FX random control at its deterministic value: `filter_rand` 0, `ring_rand` 0, `crush_prob` **0 % and 100 %** (two rows, both exact by spec).
  - Rows: each FX alone (filter LP / BP / HP × cutoff sweep incl. the 0.45·fs clamp × res incl. 100 %; drive sweep; crush sweep; ring freq × mix sweep incl. the 0.45·fs ceiling) and **all four chained**.
  - Tolerance **−90 dB** after gain alignment vs O-Granulator @ `82406fce`, fresh out-of-tree parent build (n1/run.sh precedent).
  - Negative control per row family: our side perturbed (e.g. stage order swapped, or cutoff +1 semitone) must fail the null.
- **Law rows:** port the parent's GrainFx unit / law gates (the ones its header cites: D6–D8, R14) onto our forked copy, plus block-size invariance of every stage (32/441/512/4096/var).
- **Statistical arm:** `filter_rand`, `ring_rand`, `crush_prob` (0 < p < 1) compared with the parent on latched per-grain fields via the 2.2 test hook (moments + range; Bernoulli rate for `crush_prob`). Tolerance from the sample count (CI bound), as D15. Each row has a wrong-distribution control that must fail.
- **No state bleed:** a grain spawned into a slot previously used by a resonant / crushed / ring grain renders identically to the same grain in a fresh pool (bitwise), with a control that skips the FX-state reset and must fail.

### D19 — CPU worst-case verdict: ≤ 36 % now, ≤ 40 % at 2.4
- 2.3 gives a **PASS/FAIL** (not INFO) on the worst case available now: 16 notes × 4 streams × 4 slots saturating `grain_limit` 256, **all FX on** (filter BP res 100 %, drive 100 %, crush 100 % / prob 100 %, ring mix 100 %), 48 k / 128.
- Threshold **36 %** of one core — the 40 % ARCHITECTURE budget minus the ~3.5 % reserved for 4 feedback chains + reverb (ARCHITECTURE CPU model).
- 2.4 re-runs the full worst case (feedback 100 % on 4 chains + reverb) against **40 %** and that is the final PERF-02 worst-case verdict.
- Method unchanged from C22: run alone, `CLOCK_THREAD_CPUTIME_ID` around `processBlock`, warm-up + 7 × 10 s, median/min/max, mean live ≥ 240 asserted, duty witness ≥ 0.9 (else "contended — no verdict", which counts as FAIL for gating, per the parent 2.6 finding), bypass ratio from the same session. C22 (no FX, 14.45 %) is re-run in the same session as the FX-off reference so the FX cost is a measured delta.

### D20 — Typical budget: frozen proxy now, real presets in Stage 4
- 2.3 defines a frozen **"typical" proxy** and verdicts it at **≤ 12 %**. Proposed shape (research may refine, then it is frozen in the plan): 6-note held chord × 2 enabled streams, default size / density / cap, stream 1 LP filter (moderate res) + drive ~30 %, others FX off, 48 k / 128.
- Stage 4 re-checks ≤ 12 % against the actual factory presets once they exist (2.6 / Stage 4). The proxy stays as a regression row.

### D21 — Over-budget handling: levers auto, fallback asks
- If D19 or D20 fails, execute applies the ARCHITECTURE levers **in order**, re-measuring after each:
  1. Hermite reads for rate ≤ 1 at mip level 0 — **gated on timbre**: null / spectral-difference row vs the band-limited read with a stated tolerance (research sets it), plus the +24 / +48 alias rows must stay green.
  2. 4-lane SIMD over same-(k,s) grains — must keep the 2.2 parity and render goldens byte-identical (or within the existing null tolerance, stated per row).
  3. FX-stage skip via latched `active` flags (verify they are already effective; tighten if not).
- **The fallback (grain_limit default 256 → 128 + "heavy" UI note above 128) is a locked-spec change: execute stops and asks.** It is not applied automatically.

## Defaults (no decision needed; binding for plan)

- `GrainFx.h` stays byte-identical to the fork pin (fork drift check 0 / 13). Any needed change goes in the Gi-side wrapper, not the forked leaf; if a leaf edit is unavoidable it is recorded in `FORK-MANIFEST.md` with the reason and new sha256.
- Latch math (tan / pow / exp2 / cos / sin) only at spawn; stages advance one sample at a time (block-size invariant), no per-grain allocation (A2 alloc gate extended with all FX on + steals).
- SVF and ring ceilings at 0.45·fs (Nyquist clamp, not 20 kHz) — check at 44.1 / 48 / 96 k.
- No new parameters, no `<Engine>` keys; param-dump stays byte-identical.
- 2.2 baselines carried: harness 117/0, `--parity` 41/0 + GV 2/0, renders-2.2 pinned. With FX at default (filter Off, drive / crush / ring 0) every existing render golden must stay **byte-identical** — FX-off must be a true bypass.
- Reference the **HEAD** PN residuals (−110.9 … −120.2 dB) from VERIFICATION-2.2, not SUMMARY-2.2's stale figures.
- HANDS-ON-2.3 sheet is written (each FX audible per grain, extreme res + drive by ear) but **deferred to post-Stage-3** with 2.1 / 2.2, per the user's 2026-11-08 override.
- Standalone must be rebuilt explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check — `build-and-install.sh` does not rebuild it.

## Test criteria (Phase 2.3 gate — ROADMAP + D18–D21)

- [ ] FX null rows vs parent ≤ −90 dB (each FX alone + chained; crush_prob 0 / 100), controls fail (D18)
- [ ] Parent GrainFx law rows ported and green; block-size invariance per stage (D18)
- [ ] Statistical arm for filter_rand / ring_rand / crush_prob within CI, wrong-distribution controls fail (D18)
- [ ] No state bleed on slot reuse (bitwise vs fresh pool), skip-reset control fails (DSP-03)
- [ ] 0 allocations with all FX on across note on/off, steals, cap change (PERF-01 arm)
- [ ] Max res + max drive + crush + ring stress at 44.1 / 48 / 96 k, extreme pitch: no NaN / Inf / denormal; SVF NaN kill fires on an injected NaN (control) (QUAL-01 arm)
- [ ] FX-off renders byte-identical to renders-2.2; param-dump identical; fork drift 0
- [ ] CPU worst case (16 × 4 saturating 256, all FX) ≤ 36 % with witness; FX-off C22 reference in the same session (D19)
- [ ] Typical proxy ≤ 12 % (D20)
- [ ] auval PASS; pluginval 10 VST3 + AU SUCCESS

## Constraints and risks to flag

- **CPU headroom:** C22 no-FX = 14.45 % at ~243 live grains. Parent worst FX config ≈ 0.11 %/grain → projected ~28 % at 256 before chains/reverb. Expect PASS vs 36 % but with modest margin; 96 k is roughly 2× and is INFO only.
- **Crush × sample-and-hold** is the stage most likely to be block-size sensitive (hold counter) — the per-sample state rule must hold across segment boundaries.
- **Ring carrier phase** must be latched per grain (not a global oscillator) or the null and the slot-reuse row will fail.
- **Gain alignment** in the null must not hide an FX gain law error: the fitted gain per FX row is reported and must match the FX-off fitted gain within a stated band.

## Open questions for research

1. Exact list and IDs of the parent's GrainFx law / unit rows (D6–D8, R14 in the parent's 2.3 docs) and how they are hosted in its harness.
2. Does the parent's `--parity` reference path support FX with the derandomised `freezeNoMinSpray` setup, or does the out-of-tree build need extra hooks?
3. Where the latch currently lives on our side (`GiSpawner.h`) vs the parent `Spawner::plan`, and whether `VoiceGrain` already carries the `GrainFx` state POD.
4. Statistical-arm sample count and CI for a Bernoulli `crush_prob` row and the log-uniform `filter_rand` / `ring_rand` draws.
5. Hermite lever timbre tolerance (only needed if D21 fires).
6. Final typical-proxy configuration (D20).

## Out of scope for 2.3

- Feedback chains, Freeze, Reset, reverb, and the final 40 % worst-case verdict (2.4)
- Modulation of FX destinations (dest indices b+17…b+25 routed in 2.5)
- Factory-preset typical verdict (Stage 4)
- FX UI and visual feedback (Stage 3)

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 2-dsp`)
