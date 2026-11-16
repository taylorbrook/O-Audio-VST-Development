# research-3.2 benches

Scratch benches for RESEARCH-3.2. Nothing in `Source/`, the harnesses, the goldens or the contracts was changed.

## g9b-bench

- `make_dump_main.py <out.cpp>` writes a scratch copy of `tests/render-harness/main.cpp` with two env switches:
  - `OPEQ_G9_DUMP=<dir>`: every XR-g9 job (250 stereo + 10 mono) writes `{ rate, stereo, header, packets, grid, cpp }`, where `grid` is the 192 G9 log points plus each active band's frequency, and `cpp` is `composeFromPackets` on the unit circle (LL, RR, MM, SS as re / im pairs). Then it exits.
  - `OPEQ_POLL_BENCH=1`: `DisplayModel::poll()` + the `eqSections` payload + `JSON::toString`, timed for a 24-band change, a one-band frequency change, and no change, at 48 and 192 kHz.
- Build: compile the scratch main with the render harness's own ninja compile command and relink with the other harness objects (the session used `ninja -t commands O-ParametricEQ-render-harness`, as `verify-2.6/mutation_run.py` does).
- `g9b.js <index.html> <dumpdir> [packet|raw-mono]` lifts `evalBand`, `magDb` and `composeSum` verbatim from the page and scores them against `cpp`, with the XR-g9 floors (within 80 dB of the largest maximum, at or above -120 dB).
  - `packet`: compose with each packet's `active` and `placement`.
  - `raw-mono`: on the mono files, compose with a relay placement the engine ignores on mono (odd bands Left, even Right).
- `evalcost.js <index.html> <dumpdir>`: per-frame cost of the page's curve maths on the draw with the most sections.
