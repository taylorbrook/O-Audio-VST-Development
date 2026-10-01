# Summary: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Phase:** Execute (in progress — stage 0 of 0–4 complete)

Written stage by stage. Each stage adds a section; the final summary is assembled in stage 4.

---

## Stage 0 — Baseline (Tasks 1–2) — complete 2026-09-30

`Source/` is untouched. Everything in this stage is under `tests/` and the milestone directory.

### What exists now

| Item | Where |
|---|---|
| v1.14.0 backup, verified | `backups/O-SimpleReverb/v1.14.0/` (`scripts/verify-backup.sh` passes) |
| Tests build | `cmake-build-tests/` — Release, `OUARICON_BUILD_TESTS=ON`, only O-SimpleReverb configured |
| Measurement library | `tests/render-check/measure.h` |
| v1.14.0 state fixture | `tests/fixtures/state-v1.14.0.bin` (380 bytes) + `.tsv` |
| Baseline | `BASELINE.md` (tool output + hand-written reading notes) |

render-check has two new modes, `--baseline` and `--write-fixture`, and a new section 10 (18
checks: the measurer on known signals, and the state fixture). **44 PASS, 0 FAIL**; the 26
pre-existing checks print the same lines as on the unmodified tree. `params.tsv` from param-dump
is byte-identical.

To rebuild and run:

```bash
ninja -C cmake-build-tests O-SimpleReverb-render-check O-SimpleReverb-param-dump
RC=cmake-build-tests/plugins/O-SimpleReverb/O-SimpleReverb-render-check_artefacts/Release/O-SimpleReverb-render-check
$RC 2>/dev/null            # gates (the preset manager logs to stderr)
$RC --baseline 2>/dev/null # the BASELINE.md table, for the row-by-row comparison at verify
```

### `measure.h`

Works on plain vectors below one line and through `prepareToPlay`/`processBlock` above it, so
stage 1 can point the same functions at an engine header driven directly.

| Function | Reads |
|---|---|
| `impulseResponse` | stereo IR after a 0.5 s silent settle, unit impulse in both channels |
| `midRt60` / `hfRt60` / `lfRt60` | T30 in 354–1414 Hz, the 8 kHz octave, the 125 Hz octave |
| `correlation` | zero-lag L/R over a window |
| `tailSpectrumDb` + `pearson` | whether two tails share resonances (200–2000 Hz, 0.5 Hz, detrended) |
| `onsetIndex`, `earlyTaps`, `tapsWithoutPartner` | first arrival, discrete arrivals, L taps with no R partner |
| `echoDensityAt`, `mixingTimeMs` | Abel & Huang echo density |
| `bandArrivalMs` | when a narrow band first arrives (the spring chirp) |
| `cpuPercent` | wall time in `processBlock`, reported only |

### Departures from PLAN.md

1. **Build directory is `cmake-build-tests/`, not `build-tests/`.** `build-tests/` is not in
   `.gitignore` and would have shown as untracked in every session; `cmake-build-*/` already is.
   It is configured with `SKIP_PLUGINS` set to every other plugin, so configure takes 5 s.
2. **"render-check 32/32" is 26.** The unmodified v1.14.0 render-check prints 26 PASS lines and
   `ALL PASS`. CHANGELOG and NOTES say 32/32. Nothing fails; the count in the docs is off.
3. **The ±2 % measurer check is on the mean over seeds.** One realisation of decaying noise
   scatters 2.8 % at 0.2 s and 2.3 % at 0.4 s (measured over 24 seeds) — that is the signal, not
   the measurer, which reads a decaying tone within 0.01 %. Single-seed first draft: −6.2 %.
4. **Band arrival is the first arrival, not the envelope peak, through a doubled band filter.**
   The plan's envelope-peak metric read Spring +12.5 ms and Room −88.6 ms between 1 and 3 kHz on
   v1.14.0, which has no dispersion: in a tail that is still building, a band's largest sample
   falls anywhere. A v1.14.0 Spring would have passed the chirp gate. The first-arrival reading
   gives −0.6..−2.1 ms on all six types. The band filter runs twice because one pass let a burst
   an octave away through at −9 dB (the self-check caught it: 0.02 ms where 5 ms was built in).
5. **`inject-context.py` failed** (`'stages'` — the stale `.planning/STATUS.md` has no stage
   table). No research context was injected; PLAN.md's own file list was used instead.

### Gates shown to fail

| Gate | Broken how | Result |
|---|---|---|
| state fixture loads | `.tsv` SIZE edited 83 → 84 | FAIL (`SIZE: fixture 84.0000, loaded 83.0000`), restored → PASS |
| measurer, decaying noise within 2 % | single seed instead of the mean | FAIL at 0.2 s (−4.0 %) and 0.4 s (−6.2 %) |
| measurer, chirp of 5 ms | single-pass band filter | FAIL (0.02 ms) |

### For stage 1 (from BASELINE.md)

- "SIZE moves the structure" must read first-arrival or tap times. Mixing time moves with SIZE in
  v1.14.0 (Ambient 85 → 278 ms) with no delay changing, so that reading would pass the old engine.
- Late-tail L/R correlation: v1.14.0 already passes `|r| < 0.3` on four types; only Booth and
  Spring fail. The negative control in Task 13 should not list it as an across-the-board failure.
- Booth at DECAY 0.5× (0.20 s target) sits about 3 standard deviations inside ±10 % on a
  noise-like tail. If the FDN reads near the edge there, the fix is not a wider tolerance.
- v1.14.0 at DECAY 2.0× reads 10.4–11.2 s on every type, Booth included.
- render-check writes the preset manager's log to stderr; redirect it when capturing output.
