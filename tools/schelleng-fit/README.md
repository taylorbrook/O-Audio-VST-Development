# tools/schelleng-fit

Offline transcription tool: reads `--matrix-stability` JSON output from the
O-Contrabass render harness and emits `SchellengCalibration.h` constexpr table.

## Usage

```bash
python3 tools/schelleng-fit/emit_table.py \
    plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json \
    --out plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
```

## Re-run conditions

Re-invoke ONLY if the matrix-stability render is re-rendered (e.g., Phase
2.4-bis remediation if v1.0 fallback `0.5` proves inadequate at some combo, or
future plugin recompiles change the underlying friction-junction physics).
The generated header is committed to git; CI never invokes this tool.

## Dependencies

- Python 3.11+ (tested with 3.14.2)

## Architecture

Trilinear interpolation over the 27-point grid (3×3×3 per string × 4 strings).
v1.0 design: assigns 1.0 to passing combos, 0.5 to failing combos (binary
pass/fail derived from `--matrix-stability` `pass_combo` field). Trilinear is
exact at sample points (HR-8: IEEE 754 identity arithmetic), monotonically
bounded off-grid by the 8-corner box.

See `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §17.3-§17.4
for the design rationale.
