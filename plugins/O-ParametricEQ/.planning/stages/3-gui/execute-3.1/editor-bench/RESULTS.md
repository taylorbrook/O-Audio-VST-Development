# O-ParametricEQ 3.1 T11: editor bench on the real editor

**Run:** 2026-11-15. Apple M4 Max, macOS 26.6.2, JUCE 8.0.15, WKWebView. Release (LTO) ×5 fresh processes, Debug ×1.

**Under test:** the real `Source/PluginEditor.cpp` (19 natives, 196 relays) and the real UI binary data (the plugin's `UIResources` list, verbatim). `OPEQ_EDITOR_BENCH=1` is defined on the bench target only. The plugin's VST3 / AU / Standalone compile commands contain 0 hits, and only `PluginEditor.h/.cpp` reference the define.

**Driver access:** the driver reaches the editor only through public surfaces: the editor, its `WebBrowserComponent` child, the processor's public members, and the page's DOM and JUCE frontend (dynamic import of the same module URL, so the page's own state objects).

**Reports:** through the bench-only `benchReport` native.

**Home:** each process re-execs with `CFFIXED_USER_HOME` set to `$TMPDIR/opeq-editor-bench-home` (persistent across processes). The presets directory is checked to be under it before the first write. The research bench ran in the real home.

**Source edits:** none. No `Source/` file changed for this bench.

Figures are median / worst. Research figures are from `research-3.1/relay-bench`: the bench editor with the v1 page, recomputed from its JSON.

| Row | Verdict | Figures (this bench) | Research bench |
|---|---|---|---|
| **E-open** constructor → painted page (2nd frame after all 196 states have properties) | **PASS** (gate 500 ms) | Cold **235.4 / 254.3 ms** (n = 5; per process 234, 254, 235, 235, 253). Warm 137.7 / 150.3 (n = 45). Constructor cold 60.0 / 63.4, warm 55.3 / 57.8. All props cold 191.9 / 202.2, warm 115.7 / 119.3. `pageReady` lands 0.1 ms after the 2nd frame. Close 0.3 / 0.7 ms. Debug cold 264.7 ms | Cold 256.7 / 274.1. Warm 123.5 / 147.6. Constructor cold 63.8 / 71.9, warm 33.7 / 57.5. All props cold 204.7 / 220.8 |
| **E-props** | **PASS** | `b01_freq` / `b24_freq`: start 10, end 30000, skew 0.203214481472969, interval 0. `b01_type` / `b24_type`: 8 choices. 196 properties events. Boot: 391 script calls in, 200 messages out. Boot console 0 warnings, 0 errors, 0 "unknown to the backend" (all 50 Release opens). Boot natives exactly 7: `getUiState`, `requestEqSections`, `getParameterDefaults`, `getUiLanguage`, `getCurrentPreset`, `getPresetList`, `pageReady` (once each). Console after all tests: 0 / 0 / 0 | Same properties; 6 boot natives (no `pageReady` then) |
| **E-idle** 10 s after paint | **PASS** (see note 1) | Processes 2–5 and the recheck: **0 script calls, 0 messages**. The 30 Hz timer runs from `pageReady` and emits nothing. Process 1: contaminated by a plot click and drag (note 1) | 0 / 0 (relays only) |
| **E-preset (a)** 196 sets, message thread | **PASS** | 50 runs. Painted frame **1 / 1 frame**, 14.5 / 18.0 ms. Last value in page 6.0 / 10 ms (note 2). Apply 3.0 / 4.0 ms. Host mismatch 0 | 1 frame, 14.7 / 24.0 ms. Last value 2.0 / 3.0 |
| **E-preset (b)** `setStateInformation` | **PASS** | 50 runs. **1 / 1 frame**, 12.2 / 15.2 ms. Last value 6.0 / 11 ms | 1 frame, 12.9 / 19.0. Last value 2.0 / 2.0 |
| **E-preset (c)** another thread | **PASS** | 50 runs. **1 / 1 frame**, 13.1 / 15.6 ms. Last value 6.0 / 11 ms | 1 frame, 14.2 / 18.5. Last value 2.0 / 3 |
| **E-preset (d)** `presetManager.loadPreset` (validated apply: reset to defaults, then 194 values) | **PASS** | 50 runs, 194 targets (`output_gain` and `bypass` excluded and held). **1 / 1 frame**, 12.9 / 16.4 ms. Last value 10.0 / 13.0 ms. Apply 5.5 / 6.8 ms. 0 load failures | not run (stubbed then) |
| (extra) one automated parameter → painted | — | 1 frame, 13.0 / 16.6 ms | 1 frame, 12.8 / 15.8 |
| **E-bind-in** C++ sets all 196 (seeded) → page | **PASS** | 196 / 196 in every process. Normalised error max **5.96e-8** (gate 1e-6). Scaled value bit-equal to the host's for 70 of 74 sliders; max error 6.1e-5 Hz at 10 kHz (float). `getScaledValue()` against the host text's number: max 6.1e-5 (relative tolerance 1e-6) | — |
| **E-bind-out** page sets all 196 through the state API → host | **PASS** (see note 3) | 196 / 196 values. Gestures **196 begin, 196 end, exactly 1 pair per parameter** in every process. Host settled in 4.1–10.8 ms. Slider check in the scaled domain: max error 0.11 × tolerance. Normalised error: 194 within 1e-6; 2 small-Q targets (`b17_q` 0.0328, `b22_q` 0.0506) read 9.6e-6 and 1.8e-6 (float quantum × 1/skew; note 3) | — |
| **E-type** typed entry on the selected band's Frequency field (Enter → type → Enter) | **PASS** (see note 4) | "1000": host normalised **0.5 exactly**, 999.99988 Hz, field shows "1.00 kHz", 1 gesture pair. "42.7": host **0.2500297** (exact skew mapping, error 8.5e-9). The plan's 0.25 ± 1e-6 is unreachable (note 4). "42.68090" (the 0.25 point): host 0.24999999 (error 1.5e-8). Host text is the 7-decimal default with no unit ("999.9998779"), not "1000 Hz" | — |
| **E-disabled** 8 types × 8 slopes, band 1 selected | **PASS** | **64 / 64** in every process and in Debug. Gain field `.off` with `aria-disabled` exactly for Low Cut, High Cut, Notch, Band Pass. Slope `<select>` disabled (and wrap `.off`) exactly for Bell, Notch, Band Pass. Q `.off` exactly at 6 dB/oct on Low Shelf, High Shelf, Low Cut, High Cut, Tilt Shelf. Freq never off; strip, type and slope indices and the "01" lozenge correct | — |
| **E-ui** | **PASS** | **14 / 14** per process. `getUiState` = atomics. Valid sets stored (3 / 30 / 7). Invalid sets refused with atomics unchanged: analyzerMode 4 and −1, displayRange 7 and 0, selectedBand 25 and −1, key "foo". No parameter ID or name for analyzer / display / selected / range (196 parameters). A `presetManager.loadPreset` leaves all six UI atomics unchanged | — |
| **E-listen** | **PASS** | `setListenBand` 5→4, 0→−1, 25→−1, −3→−1, 1→0, 24→23, 7→6. Editor destroyed with listen on 6 → **−1** (all processes) | — |
| **E-cycle** 10 open / close cycles | **PASS** | Release ×5: 50 cycles, no crash, every process exits 0, 0 timeouts. Debug ×1: 10 cycles, **0 JUCE assertions**, no leak report, exit 0 | Release ×5 and Debug: no crash, no leak. Template timer: 21 assertions per 10 cycles |
| **E-res** | **PASS** | Every page reference (page source, module imports, stylesheet `url()`) and all 10 embedded files: **200** with the right MIME. `text/html; charset=utf-8`; `application/javascript; charset=utf-8` ×5; `text/css; charset=utf-8`; `font/woff2` ×3. "Resource not found" log during boot: empty (all 6 processes). Fonts: `document.fonts.check("16px 'EB Garamond'")` true (also 10.5 px and italic); Regular and Italic loaded, Bold unused. Canvas "1k" axis label: ink width 9.0 px = EB Garamond 9.0 (fallback 9.5). **Pixel-identical** to an EB Garamond rendering at the same position (0 differing pixels); the fallback differs on 101 | — |
| **E-size** | **PASS** | Opens at **960 × 600** (fresh processor). `resizeEditor (700, 400)` → **800 × 500**. `(2000, 2000)` → **1600 × 1000**. `(1234, 777)` then close and reopen → **1234 × 777** restored. Atomics follow in every step | — |

Frame period: 16.7 ms (60 Hz display).

## Notes

1. **E-idle, process 1.** During its 10 s idle window, process 1 recorded a band creation on b01 (`type`, `placement`, `bypass`, `q`, `enabled` + one `setUiState`) followed by about 60 `freq` / `gain` writes. That is exactly what a click on the empty plot followed by a drag produces. The bench issues no pointer events. The same signature appeared once in the first smoke run and never in processes 2–5. A sixth Release process (`results/idle-recheck.json`) logs every pointer, wheel and key event reaching the page (`isTrusted`). It read 0 / 0 during idle with no input, and its only events were the bench's own 6 synthetic keydowns (typed entry). Process 1's other rows are identical to processes 2–5: preset, bind and disabled rows all set every parameter themselves.
2. **"Last value in page" is about 4 ms above the research figure.** The arm call now carries a target map of all 196 entries, and settling is checked against the target values on every event. The research bench counted IDs instead, which a reset-then-apply preset would fool. The painted-frame figure, the acceptance figure, is unchanged: 1 frame.
3. **E-bind-out tolerance.** The plan gives no tolerance for bind-out. The JUCE relay carries scaled values, and the host stores a float denormalised value. For a Q target near 0 (skew 0.187), one float quantum of Q is about 1e-5 normalised. The check is therefore in the scaled domain: 4 float ulps + 1e-6 × (value − start) / skew. Measured: 0.11 × that bound, and 2 of 196 above 1e-6 normalised. E-bind-in (host → page) meets 1e-6 normalised everywhere: max 6e-8.
4. **E-type, 42.7 Hz.** At the live skew 0.203214481472969, 0.25 normalised is 42.6809 Hz, and 42.7 Hz is 0.2500297. The checklist's "42.7 → 0.25" is rounded to three figures, so no correct implementation reaches 0.25 ± 1e-6 from "42.7". The page writes the exact mapping (error 8.5e-9), and the 0.25 point typed as "42.68090" lands at 0.25 (error 1.5e-8). The host text has no unit because the parameters use JUCE's default text: 7 decimals, no label. "1000 Hz" as a host string would need a text function, which is a processor change and outside 3.1.

## Files

- **Bench:** `CMakeLists.txt`, `src/Main.cpp` (driver, fake-home re-exec), `src/BenchScript.h` (injected script), `run-all.sh`, `analyze.py` → `results/summary.json`.
- **Raw results:** `results/rel-1..5.json` and `results/dbg.json` (the `run-all.sh` series), and `results/idle-recheck.json` (one extra Release process on the build with the input diagnostic added; otherwise identical).
- **Not committed:** `build-rel/` and `build-dbg/` (local `.gitignore`); `results/*.log` and `run-all.out` (root `.gitignore`).
