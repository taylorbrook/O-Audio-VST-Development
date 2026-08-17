# Changelog — O-Bitrot

All notable changes to O-Bitrot are documented here.

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
