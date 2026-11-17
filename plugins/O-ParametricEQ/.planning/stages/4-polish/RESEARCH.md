# O-ParametricEQ Stage 4 (Polish): Research

**Researched:** 2026-11-16
**Scope:** Stage 4 CONTEXT P1 – P6, the carry-forward table, and open questions 1 – 5:
- factory presets (FUNC-07) and their rows (E16 / R2);
- localisation and the glossary (UI-05);
- G10 CPU (PERF-02);
- rates, hosts and the Windows dry run (COMPAT-02);
- the full battery and the listening pass (QUAL-01);
- the carried page fixes and host text units;
- the phase split.

**Method:**
- **[BENCH]:** measured on this machine (Apple M4 Max, macOS 26.6.2, JUCE 8.0.15, Release) with the committed 0.9.0 render harness (`build/…/O-ParametricEQ-render-harness`, built 2026-11-16 16:03 from `fde885d7`). Default run, 122 s wall time. The output is kept in `research-4/render-default-0.9.0.txt`.
  - It reads 169 PASS / 2 FAIL. Both FAILs are XR-lid / XR-lidn, which print "no dump given": this run did not pass `--lid-dir`. That is an invocation artefact, not a regression; VERIFICATION-3.3 ran it with the dump (171 / 0).
  - The plan's battery recipe must pass `--lid-dir` (or `OPEQ_LID_DIR`).
- **[ARGUED]:** from reading `Source/` at `786a6c2f`, `preset-manager` 1.1.1, the suite glossaries and lints, the two CI workflows, JUCE 8.0.15, and the O-GranulatorInstrument Stage 4 precedents.

**Contracts read:** Stage 4 CONTEXT; ROADMAP § Stage 4 and the risk table; REQUIREMENTS FUNC-07 / UI-05 / PERF-02 / COMPAT-02 / QUAL-01; ARCHITECTURE § Performance and the gate table (G10, G13); `parameter-spec.md`; VERIFICATION-3.1 / 3.2 / 3.3 carry lists.

## Summary

1. **SIMD is not in play.** The scalar engine at 0.9.0 is far inside G10 [BENCH]:

   | Case (48 kHz stereo, block 512) | Measured | Budget |
   |---|---|---|
   | 24 cuts × 96 dB/oct, settled | 2.88 – 3.02 % | 10 % |
   | 24 cuts × 96 dB/oct, all sweeping | 5.54 % | 35 % |
   | 0 bands | 0.0021 % | 0.3 % |
   | 24 shelves × 96 dB/oct, all sweeping (INFO) | 27.4 % | 35 % line |
   | Worst stack: always fading + auto gain + taps + listen | 6.66 % | — |

   G10 needs a gated mode, not an engine change (Q3).
2. **Presets: build them from engineering units through `convertTo0to1`, and port the bank-hash refresh.** Two traps make the O-AnalogEQ pattern unsafe here [ARGUED]:
   - O-AnalogEQ hand-normalises its presets, and it sets `output_gain`.
   - `initializeFactoryPresets` rewrites only when the version string changes, and it never deletes. A re-voiced preset at the same version never reaches `~/Library`, and a renamed one leaves an undeletable "factory" orphan.

   O-GranulatorInstrument fixed both in its Stage 4 (`initFactoryBank`, `Factory/.bank-hash`). Port that (P1).
3. **13 presets, every gain within ±5 dB, so every node is visible at every display range (±6 / 12 / 30).**
   - One call for Taylor (C2): "Mono Low End" (a Side low cut) removes the whole low end on a mono instance, because on mono every placement processes the channel. Recommended: keep it, add an INFO row, and put it in the hands-on (Q1).
4. **Preset names stay unlocalised** (suite precedent: O-Granulator HANDS-ON-4 H4; names are file stems and the identity key for load / `isFactoryPreset`). Gate the names with a fit row instead: a port of `preset_name_fit.js` PN1 (Q1, Q4).
5. **The glossary exists but does not cover EQ.** `scripts/i18n-{fr,zh}-glossary.js` have `low cut`, `tilt`, `bypass`, `mid`, `sides` and the preset vocabulary. They lack bell, shelf, notch, band pass, slope, placement, listen, analyzer, and Mid / Side *signal*.
   - So the plugin's lint is clean (fr 74 rows 0 findings; zh 52 `mt`, 0 findings) for the wrong reason: there is nothing to check those terms against (`pattern_new_lint_zero_must_agree_with_independent_scan`).
   - Add 17 entries to each glossary first, then revise (Q4).
6. **Windows dry run: `ci-tests.yml` already has the job.** It is a dispatch-only workflow with no secrets: `windows-vst3` with MSVC, WebView2 NuGet, and pluginval 10 on the VST3; `probes-macos` with AU + auval. The only change is adding `O-ParametricEQ` to its `plugin` choice list.
   - **Blocker (call C1):** a dispatch runs what is on origin. Local `main` is 295 commits ahead of `origin/main`: 114 O-GranulatorInstrument, 90 O-Granulator, 49 O-ParametricEQ, and a few others. The repo is public. The dry run therefore needs a push of `main`, which is Taylor's decision.
   - WebView2 relay timing cannot be measured on CI. It stays a named deferral (Q2).
7. **Host text is JUCE's default for every float.** Interval 0 → 7 decimals, no unit: a host shows `B3 Freq 999.9998779` (`juce_AudioParameterFloat.cpp:50-81`).
   - Fix: `AudioParameterFloatAttributes` string functions. Hz / kHz, signed dB, Q, %, with parsers that accept `1k`, `1.2 kHz`, `-3 dB`.
   - No audio change. No change to the G11 golden (`params.tsv` has no text column) (Q6).
8. **The carried page fixes are each a few lines,** with a row each (Q6).
9. **Split Stage 4 into two phases:**
   - **4.1 Build:** source, rows, glossary, revision; 0.10.0.
   - **4.2 Validate:** fresh battery, Windows dry run, one Taylor sitting, French flip; 0.11.0 only if 4.2 changes source (Q5).

Two calls for Taylor (C1 push, C2 Mono Low End). Everything else is the plan's.

---

## Q1. Factory presets (FUNC-07, P1)

### What exists [ARGUED]

- The processor owns `OuariconPresetManager presetManager { parameters, "O-ParametricEQ" }` (`PluginProcessor.h:323`). It is declared after `parameters`, so the constructor body can read the live ranges.
  - Settings: `setExcludedParameters ({ "output_gain", "bypass" })` and `setValidatedApply (true)` (`PluginProcessor.cpp:289-290`). There are no factory presets yet.
- `applyPresetJson` (module 1.1.1):
  1. migration hook;
  2. reset **every** non-excluded parameter to its default, one gesture each;
  3. apply the file's values. With validated apply, a non-number keeps the default, a number is clamped to [0, 1], and each set is its own gesture.
  4. `customLoad` with a void var.

  The reset-to-defaults rule is met by the module itself. The `output_gain` rule is met twice: by the exclusion, and by the defs never naming it.
- `initializeFactoryPresets(defs)` (`OuariconPresetManager.h:702`):
  - returns early while `Factory/.factory-version` equals `JucePlugin_VersionString`;
  - otherwise writes each def as `Factory/<sanitized name>.json` with `"factory": true`, and stamps the sentinel;
  - **never deletes.**
- `getPresetList()`: factory + user stems, de-duplicated, then `sort (true)` (case-insensitive). Factory and user presets interleave alphabetically. The preset bar (`index.html:497`) has prev / next and a name display, no menu. The display is `textContent = currentPreset` (`preset-manager.js:385`), so there is no formatter hook.
- `PresetSaveGuard.h` (3.1) already refuses `Factory/…` targets and factory-name shadows. Today it has nothing to protect (VERIFICATION-3.1 survivors E16 / R2).
- No `customSave` / `customLoad` is registered. Presets carry no UI state: `analyzerMode`, `displayRange` and `selectedBand` live in the state's `UI` child (`PluginProcessor.cpp:1283-1343`). P1's "no UI state" is already true. Keep it gated: no `customState` key in any factory JSON.

### Two traps, and the suite fix (P1-a, P1-b) [ARGUED]

- **Hand normalisation.** `bNN_freq` and `bNN_q` are pow-skewed (0.203214, 0.186652). Choice and bool values normalise as index / (n − 1).
  - Fix: write the defs in engineering units, and convert each value once through `rp->convertTo0to1 (v)` of the live parameter. This is the O-GranulatorInstrument pattern, `FactoryPresets.h:2363-2375`.
  - It removes `pattern_factory_preset_normalized_ignores_skew` by construction.
- **Stale and orphaned factory files.** The sentinel is keyed on the version only. Taylor's listening pass will re-voice presets at an unchanged version, and a stale file would be heard as the new one. A renamed preset leaves `Factory/<old>.json`, which still lists, and `isFactoryPreset` makes it read-only for good.
  - Fix: port O-GranulatorInstrument's `initFactoryBank()` (`PluginProcessor.cpp:140-166`). Compute XXH64 over the canonical bank. On a mismatch or a missing stamp:
    1. delete `Factory/*.json` and both sentinels;
    2. call `initializeFactoryPresets`;
    3. write `.bank-hash` **last**, so an interrupted init re-runs.
  - It touches nothing outside `Factory/`, and never `User/`.
  - It is plugin-side, so `preset-manager` stays at 1.1.1 (no module bump, no consumer churn).
- **Home directory.** The writes go to `~/Library/O-ParametricEQ/Presets/Factory`, through `userHomeDirectory` (= `NSHomeDirectory`).
  - The render harness already re-execs under `CFFIXED_USER_HOME` (`pattern_juce_mac_home_ignores_HOME_use_cffixed_user_home`).
  - **The editor bench and any mutant run must do the same before they construct a processor.** A mutated bank would otherwise prune and rewrite the real `Factory/`.

### Preset set (P1-c)

Thirteen presets. A preset names only the bands it uses; everything else comes from the reset pass. Bands are in ascending frequency order from b01, so chips read left to right. The constraints applied:

- `|gain| ≤ 5 dB` on every gain-bearing band, so every node is on screen at ±6 / ±12 / ±30. `displayRange` is session UI state; the default is 12. Cuts, notch and band pass draw at 0 dB.
- Q is left at the default 1.0 on cuts and shelves (Butterworth) unless stated.
- `auto_gain` is set only where the curve loses a lot of level. `gain_scale` is never set.
- Each feature appears at least once: steep slopes (96, 48, 24, 18, 12 dB/oct), Mid, Side, Left and Right placement, notch, band pass, tilt, and auto gain.

| # | Name | Bands (type, freq, gain, Q, slope, placement) | Shows |
|---|---|---|---|
| 1 | Default | none (all bands off, auto gain off) | The flat state. The bar already shows "Default" at start; this makes it a real entry. There is no "clear all bands" control. |
| 2 | Vocal Presence | Low Cut 90 Hz 18 dB/oct · Bell 280 Hz −2.5 dB Q 1.2 · Bell 3.5 kHz +3 dB Q 0.9 · High Shelf 11 kHz +2 dB 12 dB/oct | 18 dB/oct cut, shelf |
| 3 | Kick Tighten | Low Cut 30 Hz 24 dB/oct · Bell 62 Hz +3 dB Q 1.4 · Bell 380 Hz −5 dB Q 1.8 · Bell 3.8 kHz +4 dB Q 1.6 | |
| 4 | De-Mud | Bell 220 Hz −3 dB Q 1.0 · Bell 450 Hz −2 dB Q 1.6 | |
| 5 | Air Shelf | High Shelf 12 kHz +3 dB 12 dB/oct | |
| 6 | Rumble Cut | Low Cut 28 Hz 96 dB/oct | The steepest slope |
| 7 | Telephone | Low Cut 300 Hz 48 dB/oct · Band Pass 1.2 kHz Q 0.8 · High Cut 3.4 kHz 48 dB/oct · **auto gain on** | Band pass, 48 dB/oct, auto gain |
| 8 | Hum Notch 50 Hz | Notch 50 / 100 / 150 / 200 Hz, Q 30 / 30 / 30 / 25 | Notch, high Q |
| 9 | Hum Notch 60 Hz | Notch 60 / 120 / 180 / 240 Hz, Q 30 / 30 / 30 / 25 | (CONTEXT's "Surgical Notch", split by mains frequency) |
| 10 | Gentle Master Tilt | Low Cut 20 Hz 12 dB/oct · Tilt Shelf 1 kHz +2 dB 6 dB/oct | Tilt. ARCHITECTURE: gain is the full span, so lows −1 dB and highs +1 dB. |
| 11 | Mono Low End | Low Cut 120 Hz 24 dB/oct, **Side** | Side placement (C2) |
| 12 | Side Air | Bell 250 Hz −1.5 dB Q 1.0 **Mid** · High Shelf 9 kHz +2.5 dB 12 dB/oct **Side** | Mid and Side |
| 13 | L/R Pair Match | Bell 180 Hz −1.5 dB Q 1.2 **Left** · High Shelf 6 kHz +1.5 dB 12 dB/oct **Right** | Left and Right |

The values are starting points chosen on paper. The listening pass (P5) is where they get re-voiced, and the bank hash (P1-b) makes each re-voice reach `~/Library` without a version bump.

**C2 (Taylor): Mono Low End on a mono instance.**
- ARCHITECTURE and G13 say that on mono every placement processes the channel. So this preset on a mono track is a 120 Hz, 24 dB/oct low cut on the whole signal.
- The other placed presets are benign on mono: Side Air becomes a −1.5 dB bell and a +2.5 dB shelf, and L/R Pair Match a mild bell + shelf.
- **Recommended: keep it.** The name states a stereo-bus task, and a mono instance is the case the placement rule already documents. Add an INFO row that prints each preset's mono vs stereo broadband level, and add a line to the hands-on.
- Alternative: drop it, and let Side Air carry the Side feature alone.

### Rows the plan should add (P1-d)

| Row | What it checks | Kills |
|---|---|---|
| FP1 | For each def: load through `loadPreset`, read every parameter back in engineering units. Named values must match the def (freq / Q relative ≤ 1e-5 after the round trip, gain ≤ 1e-4 dB, choices / bools exact). Every unnamed parameter must be at its default. | Hand-normalised value, skew ignored, missed reset |
| FP1o | Set `output_gain` +5 dB and `bypass` on, load each preset: both are unchanged. No factory JSON has an `output_gain`, `bypass` or `customState` key. | A def that names `output_gain` |
| FP2 | Bank hash, under a fake home: (a) first construct writes 13 files + `.factory-version` + `.bank-hash`; (b) same bank, same version: no write (mtimes); (c) one value changed, same version: that file is rewritten; (d) one name changed: the old file is gone, the new one exists; (e) `User/x.json` is untouched throughout. | Sentinel-only refresh, an orphan, a `User/` delete |
| FP3 (E16) | `savePreset ("Vocal Presence")` returns false and the factory file is byte-identical. `presetSaveRefusal` is non-empty for `Factory/Kick Tighten.json`, `Factory/sub/x.json` and `User/De-Mud.json`, and empty for `User/Mine.json` and an arbitrary export path. | VERIFICATION-3.1 survivor: native guard |
| FP4 (R2) | Validated apply through a hand-written file in `User/`: `"b01_freq": "abc"` keeps the default; `1.7` clamps to 1; `-3` clamps to 0. A parameter listener counts one begin / end pair per applied parameter. | VERIFICATION-3.1 survivor: validated apply |
| FP5 | Each preset rendered at 44.1 / 48 / 88.2 / 96 / 192 kHz, mono and stereo, blocks 512 and a random sequence, 2 s of pink noise from a load mid-stream. Every sample is finite; local κ stays inside G3's step-row measure for the load transient; the settled damped-circle response matches the designer's closed form (G9 (a) machinery and tolerance). | COMPAT-02 "renders are correct at five rates", on real content |
| FP5i | INFO: each preset's auto-gain value (none may sit at the −18 / +12 clamp) and its mono vs stereo broadband level change. | Auto-gain clamp, C2 |
| PN1 / PN1n | Port of `O-GranulatorInstrument/tests/preset_name_fit.js`. All 13 names fit `#preset-name` without ellipsis at 800 × 500 under the en, fr and zh-Hans page fonts; a 40-character name overflows (control). | A name too long for the bar |
| E-preset-listen | Latch listen on b03, load Kick Tighten (b03 stays enabled) and then De-Mud (b03 off). The outcome follows the 3.3 rule: the 80 ms settle keeps listen while the band stays enabled, and listen ends when the band goes away. | Real factory loads through the preset pass-through |

---

## Q2. Windows dry run and the push (P4, open question 2)

### Mechanism [ARGUED]

| Option | Verdict |
|---|---|
| `ci-tests.yml` (`workflow_dispatch`, input `plugin` choice: O-Octagon, O-Strata) | **Use this.** No secrets, `permissions: contents: read`. `windows-vst3` configures only the dispatched plugin (SKIP list), builds `<target>_VST3` with MSVC, installs the WebView2 NuGet package, runs pinned pluginval 1.0.3 at `--strictness-level 10`, and uploads the log. `probes-macos` also builds the AU and runs auval; its probe steps are gated on `inputs.plugin == 'O-Octagon' / 'O-Strata'` and are skipped. |
| `build-and-release.yml` dispatch with `validate_only: true` | Works too (Windows build + pluginval, no Release). But its macOS job runs next to the eight Apple signing secrets. Not for a dry run. |

- **Change:** one line, `- O-ParametricEQ` under `options:` (`ci-tests.yml:76-79`). Path-scoped into the 4.1 commit with the plugin.
- `OUARICON_BUILD_TESTS` defaults OFF (`CMakeLists.txt:128`), so MSVC sees the plugin TUs only.
- MSVC portability scan of `Source/`: no `__builtin`, `__attribute__`, `M_PI`, `arm_neon`, VLA, POSIX header, or `constexpr` std math [ARGUED].
- WebView2 is wired as the memories require: `withBackend (webview2)` + `withUserDataFolder` under the user's app-data, plus a fallback paint when the runtime is missing (`PluginEditor.cpp:179-249`).
- **Add the static gate:** port `O-GranulatorInstrument/tests/windows-webview2-gate.sh` (NEEDS_WEB_BROWSER / NEEDS_WEBVIEW2 / static linking, backend + data folder inside `#if JUCE_WINDOWS`, resource provider, no SafePointer init-captures; six negative controls). It runs on macOS in seconds and catches the 11-plugin `withBackend` class of miss before CI does.

### C1 (Taylor): the dispatch needs a push

- `gh workflow run ci-tests.yml -f plugin=O-ParametricEQ --ref main` builds **origin's** `main`.
- `git rev-list --count origin/main..main` = **295** (0 behind). The commits by plugin: 114 O-GranulatorInstrument, 90 O-Granulator, 49 O-ParametricEQ, plus a few O-Prism / O-Octagon. The repo has been public since 2026-08-03.
- Pushing any ref that contains the plugin publishes all of them. The last ci-tests run was 2026-09-21.

Options:
- **(a) Recommended:** at 4.2, push `main` once 4.1 is committed, then dispatch. This is trunk-based routine, and nothing in the 295 is marked private.
- (b) Hold the dry run until Taylor's next routine push. 4.2 closes with COMPAT-02's Windows criterion pending.

The plan should not push on its own; the push is the one step in Stage 4 that is outward-facing.

### WebView2 relay timing

- pluginval 10 opens the editor, so a blank or crashing WebView2 fails it (the `ci-tests.yml` header states this claim and its limit). It reports no timing.
- The runner has no human and no display check.
- **Recommendation:** reword the ROADMAP risk row from "WebView2 timing in Stage 4 Windows CI" to "load proven by pluginval 10 on windows-latest; open time and preset settle owed to the first Windows hands-on (no hardware)".
- An env-gated timing log is possible, but the CI log would only show a headless runner's number, which is not worth the code.

---

## Q3. G10 method (P3, open question 3)

### Readings at 0.9.0 [BENCH]

From X-cpu / X-cpu2 – X-cpu5 (20 s each, 48 kHz stereo, block 512, % of one core):

| Case | Settled | Sweeping | Other |
|---|---|---|---|
| 0 bands | 0.0021 | — | — |
| 1 band (bell) | 0.0353 | — | — |
| 24 bells | 0.761 – 0.897 | 4.83 | — |
| 24 cuts × 96 dB/oct | 2.88 – 3.02 | **5.54** | always fading 5.65; + auto gain 6.35; + taps + listen 6.66 |
| 24 shelves × 96 dB/oct | 5.35 – 5.57 | **27.4** | gain swept every block 25.3 (auto gain off) / 26.8 (on) |

Every binding row has ≥ 3× headroom. The INFO row (27.4 %) sits under the 35 % line it is reported against. **No SIMD, no correction hold.** The engine stays frozen, so the XR-lid hashes and probe N1e's `6274ff9b5b0e488a` are not at risk from G10.

### Harness: a `--g10` mode in the render harness (P3-a)

The existing X-cpu rows already time the real `processBlock` on noise; G10 needs them gated, longer, and repeated.

- **Where:** a `--g10` flag, not the default run (about 6 cases × 3 repeats × 60 s of audio ≈ 1 – 2 min). The 4.2 battery runs it.
- **Format:** Release, 48 kHz stereo, 512-sample blocks, 60 s of audio per case (ARCHITECTURE G10), 3 repeats, **median** reported, with min / max printed.
- **Core placement:** macOS has no thread pinning. Set `pthread_set_qos_class_self_np (QOS_CLASS_USER_INTERACTIVE, 0)` on the timing thread so it stays on a P-core. Print the QoS that took effect.
- **Pacing:** the loop is not real-time paced and never sleeps, so `pattern_realtime_paced_bench_core_idle_artefact` does not apply.
- **Cases and gates:**

| Row | Case | Gate |
|---|---|---|
| G10a | 24 High Cuts × 96 dB/oct (the X-cpu layout), settled | ≤ 10 % |
| G10b | the same, all 24 frequencies swept every block (X-cpu2 motion) | ≤ 35 % |
| G10c | 0 bands enabled | ≤ 0.3 % **and** ≤ 1.5 × pass-through |
| G10d | 1 High Cut × 96 dB/oct, settled; compared with G10a | `(G10a − G10c) ≤ 24 × (G10d − G10c) × 1.15` |
| G10i | INFO: 24 High Shelves × 96 dB/oct, all sweeping, against the 35 % line | not gated |

- **Pass-through** (ARCHITECTURE's "pass-through harness"): a minimal `juce::AudioProcessor` whose `processBlock` returns at once, timed in the same loop (the same per-block noise copy). G10c's ratio is then meaningful: the loop's copy dominates both.
- **G10d on net costs.** The formula as written in ARCHITECTURE (gross costs) passes trivially: it gives `B + 24c ≤ 24 (B + c) × 1.15`. Subtracting the 0-band baseline makes it test linearity. This is a reading of the gate, not an amendment; the plan should state it.
- **Busy machine:** per P3, a FAIL on a busy machine is noise first, with one allowed re-run. The harness prints the 1-minute load average beside the rows so the verifier can tell.

---

## Q4. Glossary and localisation (P2, open question 4)

### State [BENCH / ARGUED]

- `i18n.js`: 52 en entries; 53 `fr` at `reviewed: false`, 53 `zh-Hans` at `'mt'`, 0 reviewed.
- `i18n-fr-lint`: O-ParametricEQ 74 rows, 0 findings. `i18n-zh-lint`: 74 rows, 52 `mt`, 0 findings.
- The suite glossaries cover `low cut`, `high cut` (fr only), `tilt`, `bypass`, `mid` (zh: 中频 band / 中置 M/S), `sides` (côtés / 侧向), `autogain` (fr), `cutoff`, and the preset vocabulary.
- They have **no entry** for bell, low / high shelf, notch, band pass, tilt shelf, slope, placement, listen, analyzer, pre / post, gain scale, output level, display range, or Mid / Side *signal*. The lint's G1 code cannot fire on any of them.

### Glossary additions first (P2-a)

These are the plugin's current drafts unless a change is marked. Each is checked against the existing suite entries for consistency.

| en key | fr | zh-Hans | Note |
|---|---|---|---|
| bell | cloche | 钟形 | |
| low shelf | plateau grave | 低频搁架 | zh matches existing `lf shelf`; fr "grave / aigu" matches `eq low gain` → gain grave |
| high shelf | plateau aigu | 高频搁架 | zh matches `hf shelf` |
| high cut | (exists: coupe-haut) | 高切 | zh entry is new |
| notch | réjecteur | 陷波 | |
| band pass | passe-bande | 带通 | |
| tilt shelf | plateau incliné | 倾斜搁架 | composes with `tilt` (inclinaison / 倾斜) |
| slope | pente | 斜率 | |
| placement | placement (sameAsEn) | 声道位置 | |
| listen | écoute | 监听 | |
| analyzer | analyseur | **频谱分析仪** (draft: 频谱分析) | the draft reads "spectrum analysis", the act; the control is the instrument. Change if `check-ui-labels` widths allow; else keep with a termNote. |
| pre / post | pré / post | **处理前 / 处理后** (draft: 前 / 后) | a bare 前 / 后 reads as "front / back". Width-check in the analyzer segment at zh. |
| gain scale | échelle de gain | 增益缩放 | |
| output gain | gain de sortie | 输出增益 | |
| output level | niveau de sortie | 输出电平 | |
| display range | plage d'affichage | 显示范围 | |
| mid signal | **signal central** (draft: signal du milieu) | 中置信号 | "milieu" is literal; "central" is the usual French M/S term. Taylor's spot read decides. |
| side signal | signal des côtés | 侧向信号 | matches `sides` |

- Write them into both `*-glossary.js` files **and** the fr prose companion (`260826-ieq-FR-GLOSSARY.md`), as the file header requires ("change both together").
- Then re-run both lints suite-wide. A new entry can raise findings in other plugins that already use a different rendering. Those are reported, not fixed, in this stage.

### Revision, gates, and the flip (P2-b)

- Claude revises all 53 fr and 53 zh rows against the grown glossary, including `bands-full` and anything new in 4.1. New copy in 4.1 is likely none: the host text strings are C++, and preset names stay English.
- zh stays `'mt'` (`project_i18n_zh_hans_rollout`).
- fr stays `reviewed: false` until Taylor's spot read in 4.2. Then all rows flip to `true` (a source change in 4.2; see Q5).
- Gates, all at 0: `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint`, `check-ui-labels` (widest language; the zh Pre / Post and analyzer changes are the width risks), `boot-all-uis --strict-tips`.
- **Point-tip gate gap** (VERIFICATION-3.2 Issue 3 (a)): `check-ui-labels` [7] / [8] treat the tip as an opaque floating layer, so removing the re-place observer survives the gate. The row belongs in the editor bench (Q6), not in the shared gate.

---

## Q5. Phase split (open question 5)

**Recommendation: two phases.**

| Phase | Content | Version |
|---|---|---|
| **4.1 Build** | Factory bank (defs in units, bank hash, 13 presets) + FP1 – FP5i, PN1. Host text functions + rows. Carried page fixes + rows (Q6). Glossary additions, fr / zh revision, lints. `--g10` mode. `ci-tests.yml` option line. WebView2 static gate. Five UI gates, render (with `--lid-dir`), designer, pluginval / auval. | 0.10.0 |
| **4.2 Validate** | Fresh rebuild; full battery incl. `--g10` and `--long`; C1 push + Windows dispatch; one Taylor sitting (P6 checklist + listening pass + French spot read); re-voiced presets; fr flip to reviewed. | 0.11.0 if anything in source changes (re-voice or fr flip will), else none |

Why two:
- The Taylor sitting needs the final bank and the revised French installed. Doing it inside 4.1's execute would put the human step in the middle of the build.
- The push (C1) should come after 4.1 is committed, not during it.
- One combined phase would end in a long verify with a human gate inside it, which is what 3.3's "hands-on owed" pattern already shows is slow to close.

---

## Q6. Carried page fixes and host text (CONTEXT carry table)

All [ARGUED] from `index.html` at `786a6c2f`.

| Item | Site | Fix | Row (kills the regression) |
|---|---|---|---|
| Listen outlives Delete (3.3 Issue 5) | `deleteBand` (`index.html:1352`) relies on the enabled hook, which waits `LISTEN_DISABLE_SETTLE_MS` = 80 ms (`:667`, `:1000`) | `endListenOn (b)` right after `setValue (false)` in `deleteBand`. The hook stays for host and preset disables. | **E-del-listen:** latch b, delete. `setListenBand (0)` arrives within one frame (< 20 ms), not after ≥ 80 ms. A mutant without the call fails it. |
| Meter hold +1 ungated (3.3 Issue 1) | `PeakHold.h`, H = ⌈block / tick⌉ + 1 | none | **M1j** (`tests/meter-ballistics` or the render harness): block completions jittered ±30 % of a block period at 1024 and 2048 / 44.1 kHz. H shows 0 empty reads; H without the +1 shows > 0 (control). |
| Strip Listen `aria-pressed` (3.3 Issue 3) | `render()` `:1412`, `ui.latch === s` | none | **E-listen33:** latch off, drag band 3: `#btn-listen[aria-pressed]` stays "false" throughout. |
| `.tip-point` removed at fade start (3.2 Issue 5 / 3.3 Issue 6) | `hideTooltip` → `clearPointTip` (`:2393-2401`) drops the class before the opacity transition, so the text re-flows at hover-tip size while fading | Remove the class (and the label binding) after the fade: on `transitionend` for `opacity`, or a timer equal to the fade time. Cancel it if a tip is shown again in between. | **E-tipfade:** sample `getComputedStyle(tip).fontSize` during the fade: 11.5 px throughout, then the class is gone after the fade. |
| Point-tip housekeeping (3.2 Issue 3) | re-place observer; V13 binding | none (code is right) | **E-tip-lang:** switch language while the `bands-full` tip shows: the tip stays inside the viewport and re-measures (`left + width ≤ innerWidth`). **E-tip-v13:** hover a control while the point tip is up: the hover tip has no `data-i18n = "bands-full"`. |
| Host text units (3.3 carry) | `PluginProcessor.cpp:168` builds `AudioParameterFloat (id, name, range, def)`: JUCE's default text has 7 decimals and no unit | `AudioParameterFloatAttributes().withStringFromValueFunction(…).withValueFromStringFunction(…)`. Freq: `< 1 kHz → "440 Hz"` (0 dp below 100 Hz → 1 dp), `≥ 1 kHz → "1.20 kHz"`. Gain / output gain: `"+3.0 dB"`. Q: 2 dp. Gain scale: `"100 %"`. Parsers accept a bare number, `k` / `kHz` / `Hz`, `dB`, `%`, and a leading `+`. No `withLabel` (the unit is in the string, so hosts do not print it twice). Choice and bool already have names. | **HT1** (render harness): for each kind, `getText` on 20 values and `getValueForText (getText (v))` round-trips to within display precision. `"1k"`, `"1.2 kHz"`, `"-3 dB"`, `"150%"` parse. Garbage parses to the default, not 0 Hz. **HT1g:** `--dump-params` golden unchanged (no text column) and XR-lid unchanged. Logic / Reaper display is a hands-on line. |
| `pageReady` resend reset (3.3 Issue 2) | — | Info only, no row (CONTEXT). | — |

---

## Pitfalls for the plan

- **Factory writes and test homes.** Every process that constructs the processor writes `Factory/` on a new bank: harness, bench, pluginval, auval, Standalone. The harness and bench must run under `CFFIXED_USER_HOME`. pluginval / auval writing the real home is intended.
- **Re-voicing in 4.2 needs no version bump** with the bank hash; it does need a reinstall. Close the DAW: Logic caches AU state, not presets, but a live instance holds the old values.
- **`"Default"` becomes a factory name.** `savePreset ("Default")` is then refused, and `currentPresetName` starts as "Default" before any load. FP3 covers the refusal.
- **Preset order** is the case-insensitive sort of factory + user stems combined. User presets interleave. "Default" sorts after "De-Mud": D-e-f > D-e-hyphen. Names are chosen knowing prev / next walks alphabetically.
- **Lint suite-wide side effects** of new glossary entries (Q4). Report them; fix them only in the plugins they belong to, in separate commits, if at all in this stage.
- **Battery recipe:** render harness with `--lid-dir` (or `OPEQ_LID_DIR`), default and `--long`, plus `--g10`. The probe N1e hash. G9 (b) `tests/g9b/run.sh`. `tests/meter-ballistics`. The editor bench. The five UI gates. pluginval 10 VST3 / AU, auval.
- **Commit hygiene:** this checkout has other plugins' uncommitted work and one staged entry. Use path-scoped commits only (`git commit -- plugins/O-ParametricEQ .github/workflows/ci-tests.yml scripts/i18n-fr-glossary.js scripts/i18n-zh-glossary.js <fr glossary md>`), with branch and staging re-checked first.

## Calls for Taylor

| # | Question | Recommendation |
|---|---|---|
| C1 | Push `main` (295 commits, 3 plugins' work, public repo) at 4.2 to dispatch the Windows dry run? | Yes, at 4.2, after 4.1 is committed. Otherwise COMPAT-02's Windows criterion stays pending. |
| C2 | Keep "Mono Low End" (a Side low cut that becomes a full-signal 120 Hz cut on a mono instance)? | Keep it, with the FP5i INFO row and a hands-on line. |

## Files

- `research-4/render-default-0.9.0.txt`: the 0.9.0 render harness default run used for Q3 (XR-lid FAILs are the missing `--lid-dir`, see Method).
