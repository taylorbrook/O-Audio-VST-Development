# Stage 4 — Polish / Validation — Execute Summary

**Date:** 2026-11-11 · **Plan:** `PLAN.md` (19 tasks, PD1–PD20) · **Commits:** C1 `bc46d052`, C2 `5667a95a`, C3 `63ad0c5d`, C4 `f0516bca`, C5 (this commit). No tag. No forked-leaf edit, no module edit, no `scripts/` edit.

Quality gate 3-gui → 4-polish: PASSED (build, pluginval, DSP + UI critics).

## What was built

| Area | Result |
|---|---|
| Factory sources (PD5–PD7) | 8 generated sources: harmonic, pluck, bell, vowel, noise_bed, dust, drone, pulse. 48 kHz / 24-bit, C3-rooted, **3 455 333 B** total, largest 865 564 B. `tests/render-harness/factory_gen.cpp` (`-ffp-contract=off`, splitmix64); regeneration byte-identical on arm64. Pins `kHash*` + full-PCM `kFull*` in `GiFactory.h`; file sha256 in `tests/golden/factory-sources.sha256`. The 2.6 stubs are harness-only fixtures (`tests/fixtures/factory`, TestFactory target, `OGI_TEST_HOOKS` table tail). |
| Bank (PD1–PD3) | `Source/FactoryPresets.h`: 64 presets, 8 × 8. Lanes are `factory:<id>` without `contentHash`; Live templates have no `slots` key. Registered in the ctor behind a bank-hash stamp (`Factory/.bank-hash`): a stale or renamed bank is cleared and rewritten at a fixed version. The param-dump console never writes it (`OGI_NO_FACTORY_WRITE`). |
| Save guard (PD4) | `savePresetToFileGuarded`: a target in or under `Presets/Factory` is refused (the editor's existing save-failed path). |
| `output_gain` class (PD8 / D55) | Snap-only range on `output_gain`, `rec_gain`, `s1`–`s4_level`. The 0 dB default now reads exactly 0. |
| D55a / D55b | `wavFailed` in the slot JSON, edge-toasted as `toast.wavFailed` (new key, en / fr / zh). Several failing lanes in one pull → one toast per lane, ascending, +1600 ms steps. |
| D55c | The take-save WAV write runs on a detached writer. Closing the plugin on a hung disk now takes 7.6 ms (was unbounded). |
| Release | VERSION 1.0.0. `CHANGELOG.md` has a single `## [1.0.0]` heading. PLUGINS.md row: `🚧 Stage 4 \| 1.0.0`. FORK-MANIFEST line for `e77d29c8`. REQUIREMENTS: FUNC-08 and COMPAT-02 → partial. |
| Human sheet (PD19) | `HANDS-ON-SESSION-4.md`: Setup, Standalone 103 rows, Presets 71, Logic 41, Live 6, Deferred 7, 4 N/A. All 146 carried rows are placed; the old sheets carry a "superseded" header and the stale rows are rewritten. |

## Gate results (final tree)

No source file has changed since the T13 harness build and the T14 install. C5 is docs only. T13 / T14 / T15 are therefore the final-tree regression.

| Gate | Result |
|---|---|
| Harness | **454 PASS / 0 FAIL**, 0 NOT FIRED. The 431 carried rows are identical to baseline **B** (FA1 keeps its name and passes after the planned amendment). 23 new rows. |
| Parity | 168 / 0 (GV0 PASS) |
| Renders | renders-2.4 + legacy byte-identical at every PD20 checkpoint (T3, T5, T8, T11, T13) |
| param-dump | == the T3 re-pin. Exactly the 6 `defaultText` cells moved vs 3.3. |
| Fork drift | 0 / 13; FORK-MANIFEST line present |
| `--cpu4` | **64 / 64 PASS**. T median 4.37 %, p90 6.56 %, max 8.64 % (Drones · Hollow Wind); W max 10.39 %. Top-5 T: Hollow Wind 8.64, Reverse River 8.10, Fifth Bloom 7.93, Swarm 7.22, Night Engine 6.72. Same session: C25m 28.51 % ≤ 40, C25tm 6.78 % ≤ 12 (factor 0.98 vs 6.91). |
| Build | VST3 + AU + Standalone + consoles; **0 warnings in plugin sources** (only the known JUCE SDK `funknown.h` and the note-expression module warning). |
| auval | PASS; warnings == exactly the 4 LFO-rate "did not retain default" lines |
| pluginval 10 | VST3 + AU SUCCESS, SC on and off (VST3 0 ch in when off). AU has the usual single `!!!` "Current program is -1". |
| X2 | 0 hook symbols and 0 `stub_` strings in the shipping VST3 / AU (harness control 19) |
| First run | The first auval instantiation wrote `~/Library/O-GranulatorInstrument/Presets/Factory`: 64 `.json` + `.factory-version` + `.bank-hash` |
| Windows gate | `tests/windows-webview2-gate.sh` exit 0; 6 negative controls each exit 1 |
| UI | overrides twice identical; frontend ALL PASS (`--nc` 4 / 4 fail); probe 261 × 2, VE 14, lanes 12, **toasts 3 / 3**, mod 14 (`--nc` live); check-i18n · fr · zh 0 · 0 · 0; check-ui-labels default + 47; boot-all-uis 0 warn / 0 DEAD / 0 late; **PN1** en / fr / zh-Hans PASS, PN1n overflows |
| `cmp` | both pairs clean |
| CHANGELOG | `grep -c '^## \['` = 1; the CI awk extract gives 26 lines |
| Home hygiene | HM1 green. The real `~/Music/Ouaricon` was never touched. On the real `~/Library`, see deviation 6. |

### New harness rows (23)

| Row | Result |
|---|---|
| AU1 / AU1n | drift set == {lfo1..4_rate}; the six PD8 rows are exactly 0 / control fires |
| FSP1 / FSP1n | 8 / 8 sources (shape, both pins, ids, budget) / a perturbed pin → 7 / 8 |
| FP0 | 336 / 336 table + coverage checks |
| FP1 / FP1n / FP1n2 | 64 / 64 clean / linear `s1_size` fails / a `file:` slot → `fsTouches` 2 |
| FP2 | gated loudness −30.0 .. −23.7 dBFS, spread **6.32 dB**; lowest plain RMS −30.0; max peak 0.523 |
| FP3 | 8 Rhythmic × 120 / 93 bpm: 504 onsets, 0 off-grid (worst 0.968 smp) |
| FP4 | **320 / 320 bitwise**; the 64 fresh renders are distinct |
| FP4b | 64 / 64 (A = B + 1): params + state + listed slots match; the engine plays the new generation |
| FP5 | real-path bank 64 + sentinels; loadPreset read-back; user copy == factory (10 s bitwise); stamp arm (file restored, orphan deleted, stamp rewritten, User/ kept) |
| PG1 / PG1n | factory target refused, nested refused, elsewhere saved / guard off → overwritten (then restored) |
| FP6 / FP6n | worst 0.94× held refs over 8 pairs / `swapKillHard` 9.8× |
| UWF1 / UWF1n | true then false, slotRev moves / field omitted → silent |
| USV3 / USV3n | dtor 7.6 ms, 1 `cancelled`, writer exits once, no `.part` / in-worker write 3012 ms |
| HM1 / HM1n | temp home used, real dirs unchanged / `--no-home-redirect` child → HM1 FAILS |

## Deviations (each with its reason)

1. **Row names.** The plan's PR0–PR6 and FA3 collide with existing rows: parity PR1–PR12 (and the `--rows PR` prefix route) and the 2.3 alloc row FA3. The preset rows are therefore **FP0–FP6 / FP1n / FP1n2 / FP4b**, and the source-pin row is **FSP1 / FSP1n**. HM1n runs in-process as a child process.
2. **FP2 loudness spread is gated.** The plain 2–20 s RMS read a 41.8 dB spread only because percussive Keys decay to silence. The spread now uses RMS over 50 ms frames above −70 dBFS; audibility still uses plain RMS > −60. The bank was then voiced, by stream levels, envelopes, density and velocity (never `output_gain`), to a 6.32 dB spread.
3. **FP4b.** The "slot not dirty" check now asks whether the engine plays the tape of the new generation after 2048 samples (the `settle()` length). The overview dirty bytes are a cached telemetry view and still showed A's tape. Before the take, the dirty check also caught B's own 20 ms feedback ramp.
4. **FP6 pairs** switch only onto sustaining targets. For a percussive target, the B-only reference has decayed by 0.9 s, so the window cannot judge a click. "Switch onto a Keys preset while holding" is a SESSION-4 row.
5. **FP5 tolerance.** The user-copy compare allows 1e-6 normalised. Only the four LFO rates differ, by 1 ulp: the AU1 set, since a save writes `getValue ()`. The audio is bitwise equal.
6. **The param-dump console wrote the real `~/Library` bank once** (17:12, the T11 PD20 checkpoint). The harness never did (HM1 green), but param-dump constructs the processor without a temp HOME.
   - Fixed with `OGI_NO_FACTORY_WRITE=1` on the param-dump target only, in this plugin's CMake.
   - The stray folder was deleted; it was absent at T1.
   - The checkpoint script now checks the folder before and after.
   - The later auval / pluginval runs wrote the bank as the installed plugin should, so the folder exists now.
7. **FP1n2 mutated the cached bank.** A `juce::var` copy shares its DynamicObject, so editing it corrupted every later row and the stamp; this was caught by FP5 in the full run. FP1n / FP1n2 now deep-copy.
8. **Bug fixed in `customStateOf`:** a pointer into a temporary `JSON::parse` result was read after the result was destroyed, which dropped custom scales and masks. **The same pattern is in O-Granulator `Source/FactoryPresets.h:867-868`** and was not edited here (out of scope); worth a `/improve`.
9. **Spec addenda** went into the spec **header**, not the end of the file. The body hash covers everything below `LOCKED BODY`, so appending would have moved it. Body hash unchanged (`c5553e0c…`); whole-file hash re-recorded.
10. **`--cpu4` sweep** is `measure25` at reps 5 (5 s warm-up + 5 × 10 s), not 5 × 6 s + 3 s: measure25 hardcodes its lengths and was kept unchanged. Borderline / NOVERDICT re-runs at reps 7; none were needed. MPE presets were measured with channel-1 input; the member-channel INFO was not run. An advancing 120-bpm playhead drives every preset.
11. **`--gen-factory`** now writes the 8 curated sources; the stub generator moved to `--gen-factory-stubs`.
12. **Standalone smoke:** the window boots with exact 0.0 dB readouts. The preset menu was not opened (WKWebView cannot be clicked from automation); PN1 and FP5 cover its contents, and it is a SESSION-4 row.
13. **The overrides golden** moved twice, both by planned content: the 6 PD8 defaults (C1) and the new `slots[].wavFailed` keys (C2).

## Residual risks (recorded, not fixed)

- A detached WAV writer still running when a host unloads the bundle would crash (macOS hosts rarely unload).
- `~SlotLoader` joins its pool for up to 10 s on a hung disk.
- Mid-apply tearing of a preset load in a real host is not visible to the harness (SESSION-4 Logic / Live rows).
- FLAC regeneration identity is claimed on arm64 only; the checked-in files ship.

## Open items for verify

- **D51:** run `HANDS-ON-SESSION-4.md` (one human session). Every ❌ is triaged into the `PLAN-4.F` gap wave, CHANGELOG known issues, or a row rewrite.
- **D54:** T19 asked 2026-11-11; Taylor chose **hold** (no push, no dispatch). **D54 CI arm pending `/publish`**: the first Windows / x86_64 compile happens when main is pushed or the release is tagged. The grep gate (above) is the local evidence.
- registry.yaml: see STATUS (own-hunk bump in C5).

## Requirements

FUNC-08 → partial (automated arm green; audition at verify). PERF-02 stays partial (automated verdict `--cpu4` PASS; Logic CPU meter row at verify). COMPAT-02 → partial (2.5b harness evidence; DAW pass 1.0.x). Nothing else moved.
