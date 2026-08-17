# Changelog — O-Bitrot

All notable changes to O-Bitrot are documented here.

## [1.4.0] — 2026-08-17

Tape authenticity — improvement brief items 2, 3 and 11. The tape family
was the engine's most conspicuous gap against the brief's core promise:
between events it was bit-clean, its stop froze a held sample at full
level, and the most common audible fault of real failing tape had no
event kind at all.

Two new parameters, **both default 0 and both exactly transparent
there**. A v1.3.0 session or preset loads and renders bit-identically —
pinned by a new cross-version digest probe, not asserted. The one
deliberate render change is the tape stop; see "Render-affecting".

### Added
- **`TAPE_DROP` — the dropout event** (0–100%, default 0). Oxide shed
  and creased tape lift the coating off the head for a few milliseconds:
  the level dips *partway* — 10–70%, never to silence — and the top end
  goes with it, over 5–150 ms. It is the third kind of tape event, taking
  a share of the non-stop tape wins.

  The shape is `CDSkip`'s interpolation-conceal rung reused verbatim: a
  triangular log-frequency cutoff sweep, now with a triangular gain dip
  multiplied alongside it. Both endpoints are the exact identity because
  `tri(0) == tri(1) == 0`, so the event is click-free by construction
  with no ramp bookkeeping.

  Unlike a stop or a bend it installs **no rate event**, so a bend
  already in flight keeps ramping underneath it — the first instance in
  this engine of the OVERLAY class that brief item 6 generalises. It
  changes no position and no rate, so it needs no ring headroom and
  touches no part of the `ReadHead` contract.
- **`TAPE_WOW` — the continuous wow/flutter bed** (0–100%, default 0).
  Real decks wow (0.5–6 Hz, capstan and pinch-roller eccentricity) and
  flutter (6–100 Hz) *all the time* — 0.1–1% WRMS on consumer gear,
  several percent when dying. This engine had none: `TapeTransport`
  returns exactly 1.0 while idle and nothing else modulated the
  transport, so a "worn cassette" was a perfect deck that occasionally
  broke. The "Worn Cassette" preset's own comment has claimed wow since
  1.0.0, and the CHANGELOG has marketed the family as "Tape (wow, drag,
  full stops)" for as long.

  Three quasi-periodic partials — two fixed wow frequencies (0.73 Hz,
  2.31 Hz) and one flutter partial whose frequency random-walks across
  7–55 Hz — with slowly drifting amplitudes from a **new dedicated
  `wow` RNG stream**. Measured peak deviation at full knob: **1.14%**,
  against a 2.0% design budget that the partial table asserts against
  itself at `prepare()`.

  The stream is appended to `RngBank`, never inserted: stream *k* is
  seeded from a function of *k* alone, so every pre-existing stream —
  and therefore every render made with an old `SEED` — is untouched.

### Changed
- **A tape stop now dies with speed instead of freezing to DC.** A tape
  head is a `dΦ/dt` transducer: its output is proportional to tape
  *speed*, the high end dies first because the reproduce cutoff scales
  with speed at fixed recorded wavelength, and decks mute at transport
  stop. Before this, the rate ramped to exactly 0 and the read head then
  re-read one held sample forever at full amplitude, feeding a DC step
  straight into Codec and Crush. The stop sounded like a freeze.

  Output is now scaled by `g = (rate / 0.9)^0.8` below a rate of 0.9 —
  exactly 0 at rate 0 — with a speed-tracking one-pole whose cutoff
  falls as `fMax · rate / 0.9`. Measured on a forced-stop render, quietest
  32 ms window: **0.0216 → 0.000000**. (The held DC level is whatever the
  source happened to be worth at freeze time, not its peak — 0.0216 is the
  luckiest of the three freezes in that render, and the probe's 1e-3 bound
  sits an order of magnitude below even that.)

  The law is armed by a stop being *installed*, not by a rate test, and
  that distinction is the whole design. Rate alone cannot tell a stop
  from a bend: the interval table's 0.5× down-bend sits below the same
  0.9 threshold, and a rate-keyed law would have quietly taken 6 dB off
  every down-bend — the tape family's melodic voice. Probe `S2` pins
  this, and pins that the render it checks actually *visited* a
  sub-threshold rate (measured lowest bend ratio 0.500).

### Fixed
- **A one-pole entered at `fMax` is not transparent — it clicks.** Both
  new filters set their cutoff to `fMax` at the endpoints on the
  assumption that a lowpass there is effectively bypassed. It is not: at
  `fMax = 0.45·fs` the TPT gain coefficient is
  `tan(π·0.45)/(1 + tan(π·0.45)) ≈ 0.79`, and at the 0.9·fMax the stop
  law actually enters at, ≈ 0.70 — so a filter engaged with zero state
  drops 30% of the first sample it is handed. Measured as **0.150 and
  0.126 output steps on a 0.5-amplitude sine**, i.e. a click at the exact
  instant each event is supposed to be inaudible. Both filters are now
  *blended* in by the same shape that drives them (`tri` for the
  dropout, `1 - rate/0.9` for the stop), which makes entry, exit and
  bypass one identity. Both probes measured 0.144 → **0.0144** after the
  fix, which is the sine's own maximum derivative — the floor.

### Render-affecting
Anything that fires a **tape stop** renders differently from v1.3.0:
that is item 11, and it is the point. `TAPE_STOP_PROB` defaults to 10%,
so any patch with the tape family enabled is affected.

Everything else is bit-identical, and probe `V1` says so rather than
claiming it: a canonical 4 s render of forced tape *bends* over CD and
vinyl at their defaults digests to `0x3ee4e028900e47ca` under both
v1.3.0 (git `a22ff7c3`, built with this probe injected) and v1.4.0. The
two new parameters are transparent at 0 by two separate mechanisms —
the wow bed returns an offset of exactly `0.0` (and `pos - 0.0` is
bit-identical to `pos`, so `CaptureRing`'s exact-integer fast path still
fires), and the dropout roll is short-circuited before it can consume a
draw from the tape stream, so the bend sequence is unchanged.

### Notes
- **The wow bed modulates a read OFFSET, not the read rate.** The brief
  specifies a rate multiplier; that implementation breaks two
  load-bearing invariants. `ReadHead`'s lag-overflow clamp is suppressed
  while a CD loop or locked groove owns the rate, and the proof that the
  suppression is safe is "such a loop holds rate at *exactly* 1.0" — a
  multiplier falsifies the premise. And the engine's steady state is lag
  0, so any rate above 1.0 drives `pos` into the write-slot pin: at 1.5%
  and 0.7 Hz that is ~8 ms of zero-order hold per wow cycle, which is a
  stutter, not wow. Offsetting the read is the same physics from the
  other end — pitch deviation is the derivative of delay — while `pos`,
  the lag budget and the whole jump contract stay exactly as they were.
  Peak offset is ~5.5 ms, well inside the ring's 100 ms safety margin.
- **`TAPE_DROP` and `TAPE_WOW` are appended to the end of the parameter
  layout**, not inserted into the tape block. Layout order is the
  automation-slot order a host presents; inserting would have shifted
  all 23 later parameters by two slots and silently repointed saved
  automation lanes. APVTS state, the preset bank and the WebView
  bindings are keyed by ID and are order-independent.
- **Found, not fixed:** `CDSkip`'s interpolation-conceal rung has the
  same latent filter-entry click described under "Fixed" — it resets
  `concealFilter` and starts the sweep at `fMax` (`CDSkip.h:137-138`).
  No probe currently measures conceal click, which is why it has gone
  unnoticed since Phase 2.2. Left alone deliberately: fixing it changes
  CD renders, which is brief item 14's territory, and it would also
  invalidate the `V1` digest this release is gated on.

### UI
The Tape panel goes to five controls in a 3 + 2 two-row layout, matching
the existing Crush panel's dense form (`k50` knobs): Prob · Stop · Drop
on the first row, Wow · Ramp on the second.

### Presets
`TAPE_DROP` and `TAPE_WOW` are authored in all eight factory presets.
Three dial them in: **Worn Cassette** (35 / 55 — the preset whose
comment has promised wow since 1.0.0), **Gentle Rot** (20 / 25) and
**Total Media Failure** (45 / 80). The rest hold 0, which for the five
tape-disabled presets is what they already effectively were.

### Testing
`66/66` render-harness probes green, stable over three consecutive runs
(was 57/57); `pluginval --strictness-level 10` SUCCESS three times;
`auval` VALIDATION SUCCEEDED. The FUNC-02 nulls (`B`, `M1`, `M3`) and
every block-size-invariance probe (`F`, `G`, `N`, `Q`, `S2`, `Z2`)
stayed bit-identical throughout. Factory presets regenerated at 1.4.0
with 33 parameters each; `Worn Cassette` verified on disk carrying
`TAPE_DROP` 0.35 and `TAPE_WOW` 0.55 normalised.

Nine probes added:

| Probe | Gates | Result |
|---|---|---|
| `V1 v1.3.0-bit-identity` | defaults are transparent, cross-version | `0x3ee4e028900e47ca` under both versions |
| `W1 wow-live-and-bounded` | bed modulates, inside its budget | 1.138% peak, bound [0.4%, 3.0%] |
| `W2 wow-zero-bit-exact` | the 0 case keeps the integer read path | 9 s bit-exact, tape family ENABLED |
| `D1 dropout-dips-and-returns` | dips partway, returns to unity, no click | ratio [0.168, 1.000], maxDelta 0.0144 |
| `D2 dropout-zero-no-dip` | negative control for `D1`'s floor | min windowed peak 0.5000 |
| `S1 stop-dies-with-speed` | the stop reaches silence, no click | quietest 32 ms peak 0.000000 |
| `S2 bends-keep-loudness` | the gain law does not leak onto bends | peak 0.5000, lowest bend ratio 0.500 |
| `S3 v1.4-determinism` / `-ragged` | block-size invariance with all three on | bit-identical, 512 vs 1,7,64,333,4096 |

`S3` is the one that matters for the wow bed specifically: it is the
only RNG consumer in this engine that is not tick-aligned, drawing on
its own sample counter, so anything tied to a block boundary rather than
a sample count surfaces there as a mismatch under ragged chopping.

Every new positive-claim probe was **run against the reverted code**, in
one build with all three new paths disabled — a probe that passes both
ways is decoration:

| Probe | Reverted result |
|---|---|
| `W1 wow-live-and-bounded` | 0.000% deviation — "BED IS FLAT" |
| `D1 dropout-dips-and-returns` | ratio [1.000, 1.000] — "NO DROPOUT FIRED" |
| `S1 stop-dies-with-speed` | quietest 32 ms peak 0.0216 — "STOP STILL FREEZES TO DC" |

`D1` and `S1` additionally failed on their click bounds before the
filter-blend fix (0.150 and 0.126 against 0.03). `V1`, `W2`, `D2` and
`S2` correctly stayed green through the same revert, which is what they
are for: `V1`'s canonical render touches none of the three paths, and
`W2`/`D2`/`S2` are the negative controls that stop `W1`'s lower bound
and `D1`'s floor bound passing against an engine that attenuates or
modulates for some unrelated reason.

One existing probe was **re-annotated, not re-recorded**: `D DSP-01
stop-no-click` asserts a long run of bit-identical samples during the
hold, and under the new gain law that run is a run of zeros — so the
probe no longer discriminates a working stop from a silenced one on its
own. Its assertions are unchanged and still pass; `S1` and `S2` are what
now pin the stop's amplitude behaviour.

## [1.3.0] — 2026-08-17

Engine quality foundations — improvement brief items 5 and 12. No new
parameters and no state-format change; presets and automation load
unchanged. Long-loop and tape-bend **renders do change** — see
"Render-affecting" below.

### Changed
- **Capture ring 2.5 s → 10 s.** The ring span was the ceiling on every
  sustained loop, and the arithmetic was unforgiving: a locked groove
  re-jumps only while `lag + revolution <= maxLag - 50 ms`, which at
  2.5 s required a *negative* starting lag for a second pass. The
  headline "Locked Groove" preset therefore released after exactly one
  re-pass, every time. At 10 s the groove now runs **6 revolution-spaced
  re-passes** measured (>= 4 guaranteed by the static_assert at 33 1/3
  RPM), and a sustained CD loop runs **24 passes deep** where it
  previously managed 5. Cost is one `prepareToPlay` allocation: stereo
  float, 3.8 MB at 48 kHz, 15.4 MB at 192 kHz.
- **The ring's static_assert now constrains the constant it guards.** The
  old form (`>= one revolution + tape ramp + safety`) is satisfied by
  2.5 s and by 10 s alike, so it could never have caught the one-pass
  ceiling it was nominally protecting against. It is now stated as the
  multi-pass budget a locked groove actually spends, against a
  `kMinLockedGroovePasses` floor — at 2.5 s it fails to compile.
- **Read-head interpolation is 4-point Catmull-Rom** (was a 2-point
  lerp). A mid-sample lerp read retains only `|cos(pi f / fs)|` of the
  amplitude — 0.309 at 0.4x the sample rate, and about −16 dB at 0.9x
  Nyquist — and the tape family runs its entire melodic voice through
  this path at bend rates of 0.5–2.0x. Measured mid-sample retention at
  0.4·fs: **0.294 → 0.427**. The `frac <= 0` exact-integer fast path is
  untouched and still bypasses the polynomial entirely, which is what
  keeps the all-off passthrough bit-transparent (FUNC-02). Cost is two
  extra ring reads and a few FMAs, on the fractional path only; the
  worst-case render ratio was unmoved (0.0045 → 0.0043).

### Fixed
- **Sustained CD loops no longer slip.** A loop ages the read head by one
  segment per pass and had no lag budget of its own, so it kept wrapping
  until `ReadHead`'s lag-overflow clamp teleported the head forward —
  while `CDSkip` still read `state == Loop`, so the very next wrap
  re-jumped from the teleported position. The result was an audible
  unplanned slip attributable to no family. `CDSkip` now gates its wrap
  on the same lag budget the vinyl locked groove uses and self-releases
  through its own forward recovery jump when the ring is spent. Measured
  on the 400 ms-segment probe: **13 recovery jumps landing 68404 samples
  behind live → exactly 1 landing at lag 1**, with the render tracking
  live material afterwards (post-recovery error 4e-1 → 1e-6).
- **The lag-overflow clamp no longer fires under a loop.** It is
  suppressed while a CD loop or locked groove owns the read rate, which
  those transports hold at exactly 1.0 while gating their own jumps on
  the same budget — so the clamp could only ever fire spuriously there.
  The suppression additionally requires the tape transport to be idle: a
  CD or vinyl win starts a tape *release* ramp that keeps running
  underneath the loop for up to `TAPE_RAMP` ms, and during that ramp the
  rate is not 1.0, so lag can genuinely drift and the clamp must stay
  armed.
- **Overflow recovery lands at a fixed 1.2 s, not half the ring.** The
  landing distance was `0.5 * maxLag`, which was 1.2 s behind at the
  2.5 s ring but would have become a 4.95 s teleport into stale material
  once the ring grew. Pinning it as a duration keeps the last-resort
  safety net's behaviour exactly what it has always been.

### Render-affecting
Anything driving a **CD buffer loop**, a **vinyl locked groove**, or a
**tape bend** renders differently from v1.2.1 — deeper loops, and a
cleaner interpolator on every fractional read. Bit-exact passthrough with
all families off is unchanged and still verified by the null probes.

### Testing
`57/57` render-harness probes green, stable over three consecutive runs;
`pluginval --strictness-level 10` SUCCESS three times; `auval` PASS.
FUNC-02 nulls (`B`, `M1`, `M3`) and every block-size-invariance probe
(`F`, `G`, `N`, `Q`, `S2`, `Z2`) stayed bit-identical throughout.

Three probes added, each verified to FAIL against the code it gates
(a probe that passes both ways is decoration):

| Probe | Gates | Reverted result |
|---|---|---|
| `C2 item-12 catmull-rom` | interpolator response + fast-path bit-exactness | lerp scores 0.294, bound 0.40 |
| `L2 item-5 cd-loop-budget` | loop depth, single intentional recovery, lands live | 13 recoveries, lag 68404, err 0.398 |
| `M4 item-5 locked-groove-multipass` | revolution-spaced re-passes | 0 re-passes at the 2.5 s ring |

Two existing probes were **re-recorded**, both for fixture reasons rather
than DSP ones:

- `M DSP-03 vinyl-jumps` / `M2 DSP-03 vinyl-pitch` — the saw position
  marker's period (262144) carried a stated invariant, "> 2x the ring's
  maximum lag", that nothing enforced. Growing the ring took max lag from
  120000 to 475200 samples and silently broke it: backward jumps measured
  from a deeply lagged head wrapped into `(-131072, 131072]` and were
  misread or discarded, which presented as "vinyl stopped jumping." The
  period is now 2^20 with the invariant as a `static_assert`, the
  detection thresholds derive from it instead of being literals tuned to
  the old value, and both probes get a 12 s ring-fill before measuring so
  the backward ladder reads real material rather than pre-history.
  `M` also asserts the deeper ladder it can now see (>= 8 backward jumps,
  measured 13); `M2`'s trackable-hop count went 277/599 → 641/641.

## [1.2.1] — 2026-08-17

Engine-robustness pass — improvement brief items 9 and 13. No new
parameters, no state-format change; presets and automation are untouched.

### Fixed
- **Sync mode is no longer inert while the host transport is stopped.**
  `CLOCK_MODE` defaults to Sync, and `MediaClock` emitted ticks only when
  `isPlaying && wasPlaying`, so out of the box the plugin was pure
  passthrough whenever the DAW was parked — auditioning live input read
  as "the plugin is broken." A stopped transport now falls back to the
  same free-run accumulator already used for a missing playhead /
  position / PPQ. The free phase is rewound on the stopped→playing edge
  so the next stop starts from phase 0. **Playing behaviour is byte-for-
  byte unchanged** (both sync-grid probes still land on their BPM grid).
  Also deleted the dead `lastPPQ` member — written every block, read
  nowhere.
- **Jump-during-crossfade no longer clicks.** `ReadHead::clampAndSchedule`
  `Jump` overwrote `oldPos`/`oldRate` and zeroed `fadeCount` even with a
  fade in flight, so the outgoing head's contribution vanished as an
  output step of `(1 - t) * |newHead - oldHead|` — the *full* jump
  discontinuity when the fade had barely started. Reachable in a single
  tick: `Arbitration`'s kVinyl branch runs `cd.release()` (recovery jump)
  then `vinyl.onWin()` (second jump) with no render between, so `t` was
  exactly 0 and the material actually playing was discarded outright.
  A mid-fade jump now FOLDS into the running crossfade: whichever head
  currently dominates the mix is carried over as the outgoing head at the
  gain already reached. The residual step is bounded by half the
  discontinuity in every case, and is zero in the same-tick collision.
  Measured on the antiphase probe: **0.85 → 0.0145** (the latter being
  just the test sine's own derivative).
- **Deep stops no longer strand the read head.** A tape stop left seconds
  of lag that only the +2% re-approach trim could recover — ~50x the
  stall duration — until the `ReadHead` lag-overflow clamp teleported the
  head mid-normal-playback, attributable to no family. A release ramp
  that lands back on NORMAL with more than 250 ms of lag now takes ONE
  intentional crossfaded jump to live through the same choke point (the
  CD-recovery pattern): "content lost while the transport was stalled."
  Below 250 ms the gentle trim is unchanged.

### Notes
- The recovery jump fires on roughly **39% of tape releases** at stock
  settings (8 seeds x 120 s, all three transports on: 31–52%, mean lag at
  release 500–850 ms). It is not tripping on ordinary bends — the lag
  distribution at release sits well above the threshold, and the observed
  **maximum reached 2.0–2.3 s**, i.e. the pre-fix hidden clamp really was
  being hit in default use. Sub-250 ms releases keep the trim.

### Testing
- Harness 54/54 green (51 pre-existing + 3 new). All FUNC-02 nulls,
  FUNC-04 determinism and QUAL-02 block-size/ragged bit-identity probes
  unchanged and green — the fixes add no RNG draws and are per-sample
  state only.
- New probes, each verified to FAIL against the pre-fix code so none is
  decoration: `I sync-stopped-free-runs` (inert → onset @24064),
  `N2 jump-fade-collision same-tick` (0.85262 → 0.01450) and `mid-fade`
  (0.58809 → 0.22122), `N3 post-stop recovery-jump` (tail correlation vs
  live input −0.0056 → 1.0000, on noise so period-aliasing cannot fake
  alignment).
- The old `I sync-stopped` probe asserted `onset == -1` and **inverts**
  under this change; it was rewritten as a positive free-run probe, and
  its former negative-control role was replaced by a new
  `I sync-stopped-all-off-silent` case (stopped transport, every family
  disabled) so the deviation detector is still proven non-spurious.

## [1.2.0] — 2026-08-16

### Changed
- **PACKET_LOSS now spans clean → true total failure.** Root cause of the
  old ceiling: the Gilbert–Elliott mapping capped stationary Bad occupancy
  at `piB = loss01·0.6` and hard-coded loss probabilities 0.5 (Bad) /
  0.01 (Good), so full knob delivered only ~30% actual loss — and the
  unscaled 1% Good-state floor dropped one packet every ~2 s even at
  PACKET_LOSS = 0 while merely enabled. New mapping: `piB = 0.95·loss01`,
  Bad loss `0.5 + 0.5·loss01`, Good loss `0.01·loss01` plus a top-quartile
  ramp to 0.90 at full knob (the Markov clamp caps Bad occupancy at ~0.89
  even at BURST 100, so near total failure the Good state must drop
  packets too). Measured: 0 lost packets at knob zero; 98.6% at full knob
  (was ~30%). Knob feel changes across the range; the determinism
  convention (exactly 2 packet-stream draws per packet) is untouched.
- **Decay concealment now mutes out like real PLC.** Was −3 dB per
  repetition with no floor (never silent, imprinting a 50 Hz packet buzz
  indefinitely). Now −6 dB per repetition as a per-sample gain ramp,
  hard-flooring to exact silence by the end of the 3rd repetition
  (~60 ms). Decay repeats are also pitch-aligned via the existing AMDF
  path (previously Substitute-only) when the last good packet is
  periodic, with the same auto-degrade to packet-aligned replay when not.
- **Substitute cycle joints are OLA-spliced.** Cyclic replay previously
  wrapped with a bare index reset, landing the −1 dB step exactly at the
  wrap. Each joint now gets a ~1 ms raised-cosine tail→head crossfade
  (capped at period/3) with the gain step inside the fade; the resume
  index skips the pre-blended head samples so the period is preserved.
- **Presets re-tuned for the honest loss range:** Dropped Call
  PACKET_LOSS 45 → 65 (~51% true loss — the call actually drops);
  Total Media Failure 55 → 90 (~86% true loss, up from the ~17% the old
  mapping delivered).

### Testing
- Probe O (GE statistics) bounds re-derived for the new mapping
  (lostFrac 0.268, r̂ 0.472 at LOSS 40 / BURST 30) — measured 0.263 /
  0.469. Three new probes, 50/50 green: O2 knob-zero clean (0 lost of
  1474 packets at PACKET_LOSS 0), O3 full-knob true failure (lostFrac
  0.986), P2 decay mute-out (113 masked runs ≥ 5 lost packets: rep 1
  audible, reps 4+ exactly silent). All QUAL-02 block-size/ragged
  bit-identity and determinism probes unchanged and green.

### Added
- **Mono compatibility** — the plugin now loads on mono→mono and
  mono→stereo bus layouts in addition to stereo→stereo. Root cause of the
  previous behavior: the layout was hard-locked stereo in three places
  (bus constructor, `isBusesLayoutSupported`, and a `< 2`-channel
  early-return in `processBlock` that passed audio through untouched).
  Mono input is captured dual-mono into the ring; mono output takes the
  left engine channel; mono→stereo duplicates the input and runs the
  stereo path unchanged. Stereo→mono remains rejected (no downmix rule).

### Changed
- Nothing in the stereo path — verified bit-identical (all 44 pre-existing
  harness probes unchanged and green).

### Testing
- Three new harness probes (47/47 green): M1 mono→mono delay-compensated
  bit-exact null; M2 mono ch0 bit-identical to a dual-mono stereo render
  under maximum degradation (all families forced, GSM codec, crush with
  jitter/dither — proves RNG stream alignment between layouts); M3
  mono→stereo null on both output channels with junk-filled ch1 input the
  processor must discard.

## [1.0.0] — 2026-08-16

Initial release.

### Added
- **Six degradation families** over a shared capture-ring engine — Tape
  (wow, drag, full stops), CD Skip (buffer loops, restart chirps), Vinyl
  (revolution jumps, pops, locked grooves), Packet Loss (Gilbert–Elliott
  bursty loss with four concealment modes), Codec (Mu-law / GSM 06.10),
  and Crush (fractional bit quantize + sample-rate reduce with jitter,
  envelope depth, and TPDF dither).
- **Shared-buffer stochastic engine** — one media clock arbitrates Tape /
  CD / Vinyl events per tick; Packet, Codec, and Crush run as serial
  post-stages.
- **Seeded determinism** — a 0–9999 SEED parameter drives all 8 RNG
  streams; identical seed + input renders bit-identical output. Dice
  button rerolls from the UI.
- **Sync / free clocking** — tempo-synced divisions (1/16 – 1 bar) or free
  rate (0.1 – 20 Hz); HARD EDGES toggles splice crossfades off.
- **WebView UI** (900 × 620, Ouaricon Naturalist) — 3×2 family plate grid
  with per-family event LEDs, global strip with clock swap slot, seed
  ledger, and mix.
- **Preset system** — shared preset-manager module v1.0.5: save / save-as /
  load / load-from-file / prev / next / two-click delete, factory + user
  banks under `~/Library/Ouaricon Bitrot/Presets/`.
- **Factory bank** — 8 presets: Worn Cassette, Skipping Disc, Locked
  Groove, Dropped Call, Cellphone 1998, Eight-Bit Ruin, Total Media
  Failure, Gentle Rot.
