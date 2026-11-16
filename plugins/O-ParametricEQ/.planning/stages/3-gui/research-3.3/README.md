# research-3.3 benches

Scratch benches for RESEARCH-3.3. Nothing in `Source/`, the harnesses, the goldens or the contracts was changed. Nothing was installed.

## probe/

- `build.sh`: the 2.6 verify `build.sh`, renamed for `probe33.cpp`. Builds from the render harness's compile and link commands in `build/` without writing there. `SRC_DIR=<copy of Source>` compiles a modified tree; `OUT_DIR` holds objects and the binary.
- `probe33.cpp [n|a|m]`:
  - `n`: N1e as a gated row (16 cases) and a render hash with no NaN (compare across builds);
  - `a`: AO1 (Off gating), AC1 (analyzer tick + payload cost), AT1 (processBlock time per analyzer mode);
  - `m`: M1 (output-peak reads at 60 Hz under host block sizes, the hold rule).
- `listen-fix.diff`: the scratch fix (Listen.h + PluginProcessor.cpp step 8) the `n` comparison was built with.
- Outputs: `probe-output-base-n.txt` (repo Source, N1e 12 / 16), `probe-output-fix-n.txt` (fix, 16 / 16, same hash), `probe-output-quiet-a.txt`, `probe-output-m.txt`, `probe-output-contended.txt` (overlapped a build; not used).

## draw-bench/

See its README. `draw()` timing in WKWebView with the analyzer on, a per-band cache prototype (`cache-prototype.diff`), real pointer drags. Build dirs and the scratch Source copy (`peq-src/`) were moved out of the repo after the runs; rebuild with `-DPEQ_SRC=<copy of Source with cache-prototype.diff applied>`.
