`eqmodel.py` is the float64 Python model of the closed-form section designer (ARCHITECTURE "Algorithm Details"), promoted from `.planning/stages/2-dsp/research-2.1/`.
It is the independent reference for `tests/golden/designs.tsv`: the C++ designer must match that file (designer harness row D1).
Regenerate from the repo root: `python3 plugins/O-ParametricEQ/tests/model/dump_designs.py > plugins/O-ParametricEQ/tests/golden/designs.tsv` (deterministic, no RNG; Python 3 with NumPy).
The C++ must never write `designs.tsv`; its `--dump-designs` output goes to stdout for comparison only, because a golden cannot certify itself.
Change the model only together with ARCHITECTURE, then regenerate the golden and re-run the T3 self-check against RESEARCH-2.1 Q1 / Q2.
