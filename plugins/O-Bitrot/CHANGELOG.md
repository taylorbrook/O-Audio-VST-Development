# Changelog — O-Bitrot

All notable changes to O-Bitrot are documented here.

## [1.8.0] — 2026-08-17

Codec chain depth — improvement brief items 16, 29 and 7. All three are the
same complaint from three angles: `CodecStage` was a *filter chain with a
quantizer in it*, not a telephone link. It had no dynamics, its mu-law was a
curve rather than the table the standard specifies, and it never lost a
frame — the packet stage dropped PCM *before* the encoder, which is not how
cellular loss works and not what it sounds like.

**This release changes the render of the codec family whenever
`CODEC_ENABLE` is on.** It changes nothing anywhere else, and that claim is
measured rather than asserted (see "Render-affecting"). `CODEC_ENABLE`
defaults to off, so a v1.7.0 session that never opened the Codec panel is
bit-identical.

### Added
- **`CODEC_AGC` — the missing half of the phone chain** (item 16). Research
  §3.3's chain is "μ-law OR GSM → fast ~4:1 comp (AGC) → optional noise
  bed", and `CodecStage` ended at the post-LPF with no dynamics anywhere.
  The AGC's pumping is a large part of why a phone sounds like a phone;
  without it a quiet source stays quiet instead of being dragged up into the
  codec noise. A feed-forward compressor now sits **after the mode blend and
  before `post1`**: per-sample one-pole envelope, **1 ms attack, 50 ms
  release, −20 dBFS threshold, 4:1, fixed +10 dB makeup**. Measured on a
  1 kHz tone: a −26 dBFS source is lifted **+10.0 dB**, a −6 dBFS source is
  held to **−0.45 dB** — a 10.45 dB spread, which is the stage. Unity lands
  near −6.7 dBFS.

  The new knob is a **depth**, not a switch: gain is assembled in dB and
  scaled there, so `CODEC_AGC 0` is `exp(0)` — exactly `1.0f`, a
  bit-transparent multiply that restores the v1.7.0 gain structure to the
  bit. Default **100**: on with the section, as the brief asks.

- **Codec-domain GSM frame loss** (item 7). The chain order is Packet →
  Codec, so until now PCM was lost *before* encoding. Real cellular loss
  drops **encoded** frames, and the decoder conceals by repeating the last
  frame's parameters with attenuation — the metallic warble everyone knows
  as bad reception. With **GSM and PACKET both enabled**, a lost frame now
  re-feeds the decoder the previous frame's *bytes*. The encoder still runs
  on the source frame, because it lives at the far end of the link and loss
  happens in transit; the decoder's own LTP history keeps evolving on each
  repeat, which is where the warble comes from — it is not a buffer being
  looped. Per-repeat attenuation is **−3 dB**, ramped across the frame it
  applies to (a step every 20 ms would click), with a hard mute at **16
  consecutive frames — 320 ms, GSM 06.11's own mute point**.

  The PCM-domain stage keeps running underneath for the codec-off / VoIP
  case. The two do not stack: during a lost frame the GSM decoder's output
  does not depend on its input at all, so the codec repeat *masks* whatever
  concealment the packet stage produced for that frame.

### Changed
- **Mu-law is now the segmented G.711 codec** (item 29). `muLawRoundTrip`
  was the continuous `log1p`/`pow` curve with a uniform companded-domain
  quantizer; real landlines quantize on **8 piecewise-linear chords of 16
  steps**. This is the ITU-T G.711 / Sun reference encoder (14-bit domain,
  bias 33, clip 8159) against a 256-entry decode LUT built at compile time
  from the 16-bit-domain bias 132. Pure lookup — RNG-free, zero-latency, so
  the mu-path alignment ring is untouched.

  Two landmarks are properties of the standard, not normalisation choices:
  digital silence encodes to `0xFF` and decodes to **exactly 0.0f**, and
  full scale encodes to `0x80` and decodes to **32124/32768 = −0.17 dBFS**,
  which is where a real landline clips rather than at 1.0. The audible
  difference is the sub-−40 dB tail fizz the brief predicted; the clip point
  is the part you can measure.

### Notes on how these were built

**The two 20 ms grids are coupled by lookup, not by counting.** The packet
grid is `ceil(0.020·fs)` and a GSM frame is 160 slots of the 8 kHz latch —
960 samples each at 48 kHz, nominally identical. They are still not
interchangeable: the latch is **fractional**, so the frame boundary rides
its crossings and is phase-offset from the packet grid, and any scheme that
indexes loss by counting packets drifts against it at other sample rates and
block sizes. Instead the processor hands `CodecStage` the packet's loss flag
on **every** sample and the stage consumes whichever value arrives on the
sample that closes a frame. The flag is read *before* `PacketLossStage::
processSample`, because that call advances the grid at its end — read after,
the value on a boundary sample already describes the *next* packet.

`C4` is the probe this design exists for: the coupled GSM + packet-loss
render is bit-identical at block 512 and at ragged 1/7/64/333/4096. A shared
counter passes `C3` at a fixed block size and fails here.

**The AGC envelope is primed, not zeroed.** A 1 ms attack meeting a cold
envelope sits at full makeup, so the codec's first signal — which arrives
~20 ms in, past the enable fade, and therefore *after* the fade could cover
it — met the whole +10 dB and overshot to **+3.7 dBFS** on a −6 dBFS tone.
The envelope now starts at its unity-gain level (10^((−20 + 10/0.75)/20)),
which makes a cold start flat and lets the AGC *release* up into its makeup
over 50 ms if the material is quiet. What remains is the ordinary compressor
transient — **+1.7 dB**, fed by the 20 ms of structural-delay silence the
envelope releases through before signal arrives. `C1` bounds it at +3 dB, a
number chosen as a principle rather than fitted to the measurement: an
unprimed envelope scores +9.2 dB and fails hard.

Re-entry after a PLC mute still overshoots, and that one stays: it is the
compressor doing its job on a real 60 dB step, and it is part of how a line
coming back sounds.

**The AGC sits downstream of the frame loss, and partly fights it.** A −6 dB
PLC step above threshold comes back ~4.5 dB smaller after 4:1, so the
attenuation ramp is shallower than −3 dB/repeat on loud material. This is
kept rather than worked around — a real handset has exactly this
arrangement, and the mute at 16 frames is absolute regardless.

### Migration
None required. `CODEC_AGC` is a **new** parameter appended at the end of the
layout — the fourth release running, and for the reason the previous three
state: layout order is the automation-slot order a host presents, so
slotting it into the CODEC block where it belongs visually would repoint
every saved automation lane behind it. The WebView puts it in the Codec
panel regardless, because the UI is keyed by parameter ID. No existing
parameter's range, type or choice list moved, so no preset-migration gate is
needed and none was added.

Unlike the v1.4.0, v1.5.0 and v1.7.0 appends, this one does **not** default
to its transparent value. Stated plainly: a v1.7.0 session or preset with
`CODEC_ENABLE` **on** renders differently in v1.8.0 — from the AGC, and
independently from item 29's segmented mu-law, which has no opt-out at all.
Defaulting `CODEC_AGC` to 0 would have preserved nothing, because item 29
moves the same renders either way. `CODEC_AGC 0` restores the v1.7.0 gain
structure exactly for anyone who wants it.

### Render-affecting
Yes, for the codec family, and only there. Both halves of that were
measured.

`N7` — the canonical post-render (packet on at 45% loss, codec on, mu-law,
`CODEC_MIX 100`) — **moved on purpose**, by all three items at once. It was
introduced at v1.5.0 carrying `0x1cf2f80d1f71674c`, produced against the
**v1.4.0** tree at git `2160dd66`, and it held through v1.5.0, v1.6.0 and
v1.7.0. That claim is now **retired rather than re-fitted**: the probe is
re-anchored to `0xb473105611e3ea78` and re-purposed as a *forward* anchor
pinning the v1.8.0 codec chain for every release after this one. It also
asserts the digest is no longer the v1.4.0 one, so a build where items 16
and 29 silently failed to land reads as a failure rather than a pass.

The half of the old claim that survives untouched is now carried by a new
probe, **`N8`**, which runs the identical config with `CODEC_ENABLE` **off**
— exercising the GE chain, all four concealment modes' machinery, comfort
noise and the entire transport engine while `CodecStage` sits on its
bit-transparent bypass rail. Its anchor `0x8eb6e29da5ec2692` was recorded
against the **v1.7.0** tree at git `4d52377e` **before the first v1.8.0
edit**, which is the only ordering that makes a cross-version digest mean
anything. It passes: **the codec edit did not leak outside the codec.**

### Testing
97/97 render-harness probes pass (up from 93). Four are new:

- **`C1 codec-agc-compresses`** — band gain at 1 kHz between `CODEC_AGC` 0
  and 100 on the same input, so the codec's own colour cancels and only the
  compressor survives. +10.00 dB on a −26 dBFS tone, −0.45 dB on a −6 dBFS
  tone, 10.45 dB spread, peak bounded at +3 dB over the AGC-off render. At
  `CODEC_AGC 0` both gains are exactly 0 dB, so plumbing that leaves the AGC
  out of circuit reads as 0/0 rather than as a near miss.
- **`C2 mulaw-segmented-chords`** — reads the encoder and decode table
  directly rather than inferring them from a render, because the post-LPF
  smears the quantizer lattice and a signal-domain probe could only ever
  measure "something changed". Asserts the structure that makes G.711 a
  segmented law: 8 chords of 16 **uniform** steps, each chord's step
  **exactly double** the one below, plus the two landmarks. A continuous log
  curve has neither uniform runs nor exact doublings, so this separates the
  implementations rather than merely detecting an edit.
- **`C3 gsm-frame-loss-mutes`** — GSM at 90% loss with `PACKET_CONCEAL` set
  to **Repeat**, which replays the last good packet verbatim and never
  decays, on a constant-amplitude 1 kHz tone. The PCM stage therefore feeds
  the encoder a full-level signal for the entire render, and silence can
  come from one place only: the new mute. 9562 samples (~200 ms) of it, with
  the render otherwise peaking at 0.96 — and **zero** silent samples in the
  same config with `PACKET_ENABLE` off, which is the negative control that
  keeps the result attributable to the coupling.
- **`C4 frame-loss-ragged`** — described above.

One earlier finding is recorded because the config that produced it is worth
knowing about: at `PACKET_LOSS 100` the GE chain gives `pGB = 1` and
`pLossBad = 1`, so per-packet loss is ~98.75% and the codec sits past its
mute point essentially forever — the line is dead, which is correct for
total failure but makes a useless probe. `C3` runs at 90% for that reason.

## [1.7.0] — 2026-08-17

Vinyl authenticity — improvement brief items 17 and 27. Three of the four
changes here are about the same underlying complaint: the vinyl family was
built from a *single* physical gesture repeated. Every pop was the same
doublet with a random gain. Every forward skip teleported to the present,
whatever the stylus had to work with. The disc itself was perfectly flat and
spun at one of two speeds. Item 17 gives the pop a taxonomy, item 27 gives
the disc physics — a second direction for the groove jump, a warp, and a
third speed.

**This release changes the render of the vinyl family at its defaults.**
`VINYL_POP` defaults to 50, so the pop taxonomy is live out of the box, and
forward revolution-quantization needs no parameter at all. No other family
moves a bit. See "Render-affecting" for what is preserved and how that was
established.

### Added
- **Pop taxonomy: tick / pop / scratch** (item 17). A one-pole cannot ring,
  so no pop in the first six releases had the damped stylus resonance that
  reads as "the stylus hit something" — the whole family was one sample with
  a random gain and ±3 dB / 1–3 kHz of variation. `triggerPop` now draws a
  class at ~**70/25/5**:
  - **tick** — the historical doublet through the `FirstOrderTPT` lowpass,
    cutoff biased **high** (2–4 kHz), so it reads as the dry surface-debris
    click it always was;
  - **pop** — the same doublet exciting a damped 2nd-order resonator
    (`StateVariableTPT` bandpass, 900–1800 Hz, Q 4–8);
  - **scratch** — a 2–8 ms band-limited noise burst under a raised-cosine
    window, plus a 40–80 Hz decaying sine: the tonearm thump under the rasp.

  The brief specified "rings 1–3 ms" alongside "Q 4–8", and those two are
  not compatible: Q 4–8 at 900–1800 Hz decays to −30 dB in **2.4–9.8 ms**
  (measured across the corners of the (f0, Q) box, not asserted from the
  spec). Q is the parameter that was specified, so Q is what shipped, and
  the ring time is reported here rather than restated from the brief. The
  click peak is normalised to the same base level for all three classes —
  `1/g` is within 3% of the true doublet-response peak across the whole box
  — so the ring sits 7–13 dB under the transient, which is the shape a
  stylus impact actually has.

- **`VINYL_WARP`** (item 27b) — a warped disc, default 0. One sinusoid at
  exactly the platter rate (1/1.8 Hz at 33⅓, 0.75 at 45, 1.3 at 78), peak
  rate deviation 0.6% at 100%.

- **78 RPM** (item 27c) — appended to `VINYL_RPM`. Revolution quantum
  0.769 s. The surface comes with it: shellac ticks are 1.6× denser and the
  tick band drops and narrows to 1.5–4 kHz, matching a playback chain that
  rolled off hard above ~5 kHz. Gated on the index, so 33⅓ and 45 keep their
  numbers to the bit.

### Changed
- **Forward groove jumps are revolution-quantized** (item 27a). Research
  §4.1 specifies "exactly ±1 revolution" in *both* directions; the engine
  only ever did it backward, and every forward skip teleported to live. A
  groove that skips backward by a revolution and then returns to the present
  is half a physical model — the stylus jumping a ridge forward lands one
  groove on, not at the present. Forward jumps now move exactly
  `+revSamples` whenever `lag >= revSamples + minLag`, with the historical
  return-to-live as the fallback when the buffer genuinely has no revolution
  of past to spend. Rate is untouched, so BRIEF:16's "pitch never changes"
  holds exactly as it does for the backward jump this mirrors. Draw count
  unchanged.

### Notes on how three of these were built

**The warp is a read offset, not a rate multiplier.** The brief says
"sinusoidal rate multiplier". That is not available in this engine, for the
two reasons `WowFlutter.h` documented in v1.4.0 and which apply here
verbatim: `ReadHead`'s lag-overflow clamp is suppressed while a loop
transport owns the read rate, and the proof that the suppression is safe is
"such a loop holds rate at **exactly** 1.0" — a rate multiplier falsifies
that premise, and a *locked groove* is precisely the case item 27b is about.
Separately, at the engine's lag-0 steady state any rate above 1.0 drives the
head into `ReadHead`'s write-slot pin, which is a zero-order hold: a
stutter, not a warp. Modulating a non-negative read **offset** is the same
physics from the other end — pitch deviation is the derivative of delay — so
`VinylWarp` specifies its excursion by the rate deviation it should produce
and inverts the relation. Every lag budget, loop-wrap test and clamp proof
in the engine is untouched, and the offset simply sums with the wow/flutter
bed's at the single `renderSample` call site.

**The warp LFO's period is an integer, and it is the same integer as the
jump.** The brief's claim is that a warped locked groove wobbles identically
every pass. That is true only if the LFO period equals the groove-jump
distance *exactly*: a locked groove jumps back `revSamples` each pass, so
`pos(t + revSamples) == pos(t)`, and the read repeats only if
`offset(t + revSamples) == offset(t)` too. The phase increment is therefore
`2π / revolutionSamples()`, not `2π·f/fs` — deriving it from the un-rounded
seconds would leave a 0.077-sample residue per pass at 78 RPM / 48 kHz.
Small, but the identity can be exact, so it is. The new `VinylGeometry.h`
holds the one definition of the RPM→samples mapping that the transport, the
warp and the noise bed all read, because three copies of
`rpmIndex == 1 ? 45.0 : 100.0/3.0` is the shape that drifts silently the
next time a speed is added.

**The scratch class needed its own RNG stream.** A 2–8 ms noise burst draws
once per *sample*, and the `artifactSynth` stream it would otherwise share
is consumed at *tick* instants — mixing the two on one stream is the
block-size interleave hazard verbatim. `RngBank` gains an **appended**
`scratch` id (appended, never inserted: stream *k* is seeded from a function
of *k* alone, so every pre-existing stream's seed, and every render made
with an old `SEED`, is bit-identical). `triggerPop` itself keeps the
fixed-draw discipline, now **five** draws on every call — before any branch,
regardless of class, and regardless of level 0 — so a probe that silences
the pops still gets an identical event sequence.

### Migration
`VINYL_RPM` gained a third entry, and appending to an `AudioParameterChoice`
**repoints saved presets**. Presets store the normalised fraction, and index
*i* over *N* choices encodes as *i*/(*N*−1): "45" went out as 1.0 against an
end of 1, and 1.0 against an end of 2 decodes as "78". A pre-1.7.0 preset
that used 45 RPM would silently load as 78.

Handled with the preset-manager **v1.0.6** `setMigrationCallback` hook (the
same one O-Tapestop v1.1.0 added for its `MODE` append): pre-1.7.0 presets
have their `VINYL_RPM` fraction re-decoded against the old two-choice end
and re-encoded against the new three-choice one, before the module's
reset-to-defaults and apply passes. Factory presets regenerate at the
version bump and are stamped with the current version, so they never reach
the gate.

APVTS **session** state needs no equivalent and gets none: it persists the
choice *index*, and 0/1 still mean 33⅓ and 45. Presets are normalised,
sessions are denormalised — only the presets move.

`VINYL_WARP` is appended at the **end** of the parameter layout, not into
the vinyl block where it belongs visually, for the third release running:
layout order is the automation-slot order a host presents, and inserting
would repoint every saved automation lane behind it. The WebView puts it in
the Vinyl panel regardless, because the UI is keyed by parameter ID.

### Render-affecting
Yes, for the vinyl family, at defaults. Two independent changes do it and
neither is a regression: the pop taxonomy widened `triggerPop` from two
`artifactSynth` draws to five, so every pop after the first differs even at
an unchanged level; and forward jumps became revolution-quantized. The
canonical digest probe `V1` runs `VINYL_PROB 60`, so it legitimately moved
(`0x972a5d3807538393` → `0xf8c2080db4e69ec1`).

`V1` was **re-anchored, not quietly re-recorded** — the same treatment
v1.6.0 gave it. It now pins `VINYL_ENABLE 0` in addition to `CD_SEVERITY 0`,
and the new anchor `0x44a5de77d572facd` was produced by compiling the probe
*with those two lines* against the **v1.6.0 tree** at git `16e63620`, in a
detached worktree, and reading the number it printed. That run scored 86/87
— the single failure being `V1` itself measuring the freshly narrowed render
against the anchor the narrowing had just superseded, which is exactly the
run that produces the new number. The current tree then matches it.

What that preserves is the claim worth having, now across four releases:
**tape bends, the post-stop recovery jump, the CD conceal rung and the
lag-overflow clamp are bit-unchanged** — and so is the read path with
`VinylWarp` wired into it and silent, which is the v1.7.0 addition that
claims to be exactly transparent at its default. The vinyl transport's own
invariants are asserted directly by `M`/`M4`/`M5`/`M8` on the revolution
grid rather than through a digest, which is the better place for them.
`N7` still matches its v1.4.0 anchor, so packet and codec are untouched.

### Testing
Harness **92/92**; 5 probes added. auval PASS; pluginval strictness-10
SUCCESS on three consecutive runs.

Every new probe was verified to FAIL against the code it gates, by reverting
that single behaviour and re-running:

| Probe | Gates | Reverted → |
|---|---|---|
| `M5` | item 27a's threshold, both sides | branch removed → "lag 3 rev moved 192000, expect 64000"; gate removed → "lag 65400 moved 64000, expect return-to-live" |
| `M6` | the warp's locked-groove identity | LFO period pinned to 33⅓ regardless of RPM → pass-to-pass diff **0.861** against a 1e-6 bound |
| `M7` | the pop class rings | pop routed back to the one-pole → **0** zero crossings in 5 ms, against 13 |
| `M8` | the 78 RPM quantum | index 2 removed from the table → quantum reads 86400 |
| `M9` | the preset migration gate | version gate removed → a v1.7.0 preset's "78" loads as "45" |

`M8`'s revert is the interesting one, because it is the case a naive probe
would miss: the probe takes its expected quantum from the *same function*
the engine uses, so deleting index 2 changes both together and every event
still lands "on quantum". The `distinct` guard — the quantum must not equal
45's 64000 or 33⅓'s 86400 — is what catches it, and it is the reason the
probe is written that way rather than against a literal 36923.

`M9` runs end-to-end through the module's public `loadPresetFromFile()`, so
the version gate, the reset pass and the apply pass are all in the path, and
it carries its own negative control: a preset stamped 1.7.0 with the same
1.0 must load as "78", because at that version 1.0 genuinely means 78.

`M` changed shape rather than gaining a check. Its forward assertion used to
be "every forward event lands at live", which *was* the whole of the old
behaviour; it now accepts either shape and additionally requires that the
revolution-quantized forward actually fires, so the pre-v1.7.0 engine fails
it at "0 fwd+rev". Its scan window also moved 400 samples past the warmup:
the warmup boundary lands exactly on a clock tick, so a jump fired on the
window's first sample and `scanSawEvents` read that event's `vPre` from
inside its own smear — measuring 63556 where every other event reads
64000 ± 3.2 (the ±3.2 is the +2% re-approach trim over the 160-sample
read-ahead, and is why the tolerance is ±8). That one event was a
measurement artifact, not a transport defect, and it is now skipped rather
than mis-measured.

## [1.6.0] — 2026-08-17

CD skip authenticity — improvement brief items 14 and 18. The CD ladder's
top two rungs were built to the right *shapes* and then flattened by
constants that ignored the one knob meant to drive them. A mute was 2–20 ms
whether the disc was lightly scuffed or dying; a loop window was whatever
`CD_SEGMENT` said, to the sample, which no anti-shock buffer has ever done;
every loop wrap was smoothed by the same 3 ms crossfade a *recovery* jump
uses; and a loop ended by snapping to live in one sample. Four changes, all
in the CD family, all keyed to `CD_SEVERITY`, no new parameters.

**This release changes the render at the default `CD_SEVERITY` of 0.5** —
the first O-Bitrot release that does. Item 14b's mute ceiling is live at
every severity above 0, and 0.5 is where the knob starts. See
"Render-affecting" below for exactly what is and is not preserved.

### Changed
- **Loop wraps splice HARD** (item 14a). A wrap is the artifact; a recovery
  is the player getting its act together. They were both taking the single
  global 3 ms crossfade, which at the Skipping Disc preset's 45 ms segment
  spent **6.7% of every repeat inside the splice** — the buzz sanded off the
  thing that makes it a CD skip. `ReadHead::clampAndScheduleJump` now takes
  an optional per-jump fade length; CD loop entries and wraps pass 0.5 ms
  (24 samples at 48 kHz, 1.1% of that segment), and vinyl revolutions,
  recovery jumps and the overflow clamp keep the 3 ms default.

  The trap here is the **mid-fade fold**, added in v1.2.1 to stop a jump
  arriving mid-crossfade from dropping the outgoing head as a step. Its
  arithmetic divides `fadeCount` by the fade length — so with a per-jump
  length, the *running* fade's length is the only valid denominator. A
  splice request arriving 20 samples into a 144-sample fade must therefore
  SPEND the running length, not adopt the short one: rescaling `t` from
  20/144 to 20/24 collapses the outgoing head's weight from 0.861 to 0 in a
  single sample. Measured, by building it the wrong way on purpose: a
  **0.357 output step against a 0.042 bound**, an 8.5x click. The fade
  length in flight is now a separate member from the two prepared lengths,
  and probe `L5` pins both halves.
- **Mutes lengthen with severity** (item 14b). The rung-1 mute was pinned at
  2–20 ms regardless of `CD_SEVERITY`; a real E32 mute grows as the disc
  worsens. The *span* scales (the 2 ms floor does not), from 18 ms at
  severity 0 to 148 ms by severity 0.6 — where the loop rung takes over —
  for a 150 ms ceiling. Measured 61.2 ms longest at severity 0.2 against
  that severity's 63.3 ms ceiling, and 141.9 ms at 0.6 against 150.0. The
  severity-0 expression is the v1.5.0 one **bit-for-bit**, because
  `0.130 * 0.0` is exactly `0.0`.
- **Loop windows lock to CD sectors above severity 0.5** (item 14c).
  `CD_SEGMENT` is a free 10–400 ms, but an anti-shock buffer re-reads whole
  sectors, and the 1/75 s quantum is why a skipping CD buzzes at a
  75 Hz-family rate rather than at whatever the knob says. Above
  `kSectorSeverity` the window snaps to the nearest multiple of `fs/75`:
  100 ms asks for 4800 samples and gets 5120. Implicit rather than a toggle
  — "how far gone is the disc" is what `CD_SEVERITY` already means. At or
  below 0.5 the free value is used verbatim, and both sides are asserted
  (probe `L3`), because the gate is the claim.

### Added
- **Servo-seek terminal stage above severity 0.85** (item 18). `CD_SEVERITY`
  had three audible regions and its top third was just "region 3, but more
  often". The real end-stage of a dying player is not a loop — the buffer
  runs dry, the sled loses tracking, and there are **hundreds of
  milliseconds of dead silence** before playback resumes somewhere ahead
  with a re-lock chirp. A loop released above `kSeekSeverity` now mutes
  fully for 100–400 ms and only then takes the recovery jump, with a chirp.
  Measured 338.2 ms on the canonical render, tail tracking live to 0.000000
  after it.

  The duration is drawn **at loop entry, not at release**. Both paths that
  end a loop — `release()` and the lag-budget self-release inside
  `processSample` — run without an `RngBank`, and the harness's block-size
  invariance rests on RNG being consumed only at ticks and at deterministic
  jump instants. Drawing at entry also means the draw is **skipped entirely
  at or below 0.85**, so the cd stream's pattern is untouched there.

  It is deliberately NOT entered when a loop gives way to a *different rung*
  on a fresh CD win: the family is still winning ticks, so that is the disc
  changing failure mode, not the terminal release — and 100–400 ms of
  silence would swallow the very rung being installed.

### Render-affecting
Yes, at `CD_SEVERITY` > 0. The canonical cross-version digest probe `V1` was
**re-anchored** rather than quietly re-recorded: it now pins `CD_SEVERITY 0`,
the one severity at which this release is exactly transparent (the rung roll
can only reach conceal, both new thresholds are below their gates, and every
jump still takes the default fade). The new digest `0x972a5d3807538393` was
produced the same way the old one was — by compiling the probe against the
v1.3.0 tree at git `a22ff7c3`, where it ran 57/57 — and the current tree
matches it. What that preserves is the claim worth having: **the per-jump
fade refactor changed nothing for tape bends, vinyl revolutions, the
post-stop recovery jump, the CD conceal rung or the overflow clamp, across
three releases.** `N7` still matches its v1.4.0 anchor, so the packet and
codec chain is likewise untouched.

### Testing
Harness **87/87**, stable over three runs; 6 checks added (`K2`, `L3` x2,
`L4` x2, `L5` x2 — 7 checks across 5 probes). Every one was verified to FAIL
against the code it gates by reverting that single behaviour and re-running:
disabling the sector lock takes `L3` to "0 wraps at 5120, 374 at 4800";
disabling the seek takes `L4` to "mute 0.0 ms"; pinning the mute span takes
`K2` to 18.1 ms at severity 0.6; ignoring the fade argument takes `L5` to
"splice 143, expect 23"; and removing the fold guard produces the 0.357 step
described above.

`L5` measures the read head **directly**, not through the plugin, and that
is not a shortcut: every loop wrap fires a chirp over exactly the window the
splice occupies, three orders of magnitude louder than the marker step being
faded (the existing chirp probe reads a ratio of 2.3e12). Driving `ReadHead`
and its ring from the harness removes the chirp, the clock and the rung roll
and leaves the one number in question.

`L`'s recovery check accepts a build with or without the seek — verified,
not assumed — because its claim is "the loop ends and the head returns to
live". `L2`'s post-recovery window is now clipped to the next clock tick and
its **width is asserted**: the seek pushed the recovery to ~1300 samples
before that tick, and the old fixed 10000-sample window was measuring the
next loop's entry jump (0.018311 — exactly one segment over the saw period)
and calling a working engine broken.

## [1.5.0] — 2026-08-17

Media noise beds — improvement brief items 4 and 19. The engine
synthesised only *event-triggered* artifacts: `ArtifactSynth`'s pop, tick
and chirp all fire at jump instants, and between clock ticks nothing ran
at all. A "dying media" patch was therefore a clean signal punctuated by
breakage, and the illusion collapsed the moment the events stopped. It is
the floor, not the events, that carries the sense of a machine.

Five new parameters, **every one default 0 and exactly transparent
there**. A v1.4.0 session or preset loads and renders bit-identically —
pinned by two digest probes against two different earlier trees, not
asserted. **There are no render-affecting changes in this release.**

### Added
- **`TAPE_HISS` — the tape noise floor** (0–100%, default 0). Decorrelated
  stereo white behind a gentle −9 dB shelf above 3.5 kHz and a 60 Hz
  highpass; **−48 dBFS RMS at full knob** (measured −47.99), roughly a
  Type I cassette with no noise reduction.

  Stereo, unlike every other artifact in this engine, and deliberately so:
  two tape tracks carry two *independent* noise sources, and a mono bed
  added to both channels hits the level target exactly while sounding like
  a fault in the plugin rather than a floor on the tape. Measured L/R
  correlation −0.000, and the probe asserts it — level alone cannot tell
  the two implementations apart.

  It also **rides the transport speed**. A tape head is a `dΦ/dt`
  transducer, so hiss is recorded material like everything else: when
  v1.4.0's `TapeStopGain` takes the programme down to silence, hiss that
  kept running at full level would announce that the noise is synthetic.
  `TapeStopGain` now publishes its current gain and the bed multiplies by
  it. A *bend* must not do this — the bend table's 0.5× interval sits below
  the gain law's threshold — and the discriminating probe for that lives
  next to the one that proves stops do.
- **`VINYL_WEAR` — the record surface** (0–100%, default 0). A Poisson rain
  of micro-ticks over pinked bearing rumble.

  Tick amplitude is a **cubic power law** — `peak · u³`, many tiny and rare
  large, which is the shape real surface noise has. Cubing a uniform is
  *bounded*; an inverse-CDF Pareto draw would not be, and nothing on the
  audio thread should be able to draw an arbitrarily large impulse. The
  harness asserts the law through the tick band's **crest factor** (10.9
  measured, bound ≥ 8): Gaussian noise crests near 4, so a bed emitting
  uniform-amplitude clicks would pass a rate check and fail this one.

  Rumble is **−42 dBFS RMS at full knob** (measured −42.00), three poles
  below 55 Hz, amplitude-modulated once per platter revolution at the
  `VINYL_RPM` rate — bearing rumble is eccentric by construction, and that
  beat is what separates "low noise" from "a turntable".

  Mono, and for a more literal reason than `ArtifactSynth`'s "the failure
  is the player, not the channels": one platter bearing makes one rumble
  and one stylus rides one groove.
- **`CODEC_NOISE` + `CODEC_MAINS` — the phone line** (0–100%, default 0;
  50 Hz / 60 Hz, default 50 Hz). Mains hum — fundamental plus 2nd and 3rd
  harmonics at −40 dBFS RMS full-knob (measured −37.87 for the fundamental,
  against an analytic −37.86) — with Poisson crackle bursts, 3–25 ms,
  band-limited to 300–3000 Hz.

  The hum takes **no RNG draws at all**: three partials off one phase
  accumulator, a pure function of the sample count. Mains hum is the one
  artifact in this plugin that is genuinely not random, and a hum whose
  phase depended on the seed would drift against the programme between
  renders of the same session.

  It is injected **after `CodecStage`, not on the artifact bus with the
  other two**. `CodecStage` is a 300–3400 Hz phone chain, so 50 Hz in front
  of it is annihilated by the passband and the hum would have been
  inaudible at every setting. That is also where the physics puts it — hum
  is induced on the line, not recorded at the source — so it is scaled by
  `CODEC_MIX` as well: Blend is how much phone you are hearing, and the hum
  *is* the phone.
- **`PACKET_COMFORT` — comfort noise under extended concealment**
  (0–100%, default 0). `PacketLossStage` hard-floors Decay to exact silence
  by the end of the third repetition (~60 ms) because that is what real PLC
  does. What real PLC does *next*, which this engine did not, is fill the
  hole: G.711 Appendix II CNG and GSM SID frames both substitute low-level
  spectrally-shaped noise matched to the background. That hiss floor is
  precisely the cue that says the call is still up but dying — without it a
  long burst is indistinguishable from the far end hanging up.

  Two one-pole trackers run on **good packets only** — a level estimate and
  a tilt estimate, the energy split either side of 1 kHz — so a dark source
  gets a dark floor and a bright one gets a bright floor. Each half of the
  noise split is divided by its exact analytic gain, so the tilt weights
  mean what they say. A silent source estimates zero and emits exactly
  nothing: comfort noise under silence would be the plugin talking.

  The bed is **additive under all four concealment modes**, not a
  replacement. Under Decay and Substitute — already at or near silence when
  it arrives — that reads as the crossfade the brief specifies; under
  Repeat, which repeats a packet verbatim forever, replacing the output
  would have dissolved the machine-gun edge that *is* that mode's identity,
  so the bed sits 33 dB beneath it instead (measured). The knob is squared
  before scaling so its useful range spans the travel: 100% = −30 dB
  relative to the tracked programme, 50% = −42 dB (the G.711 figure), 25% =
  −54 dB.

  **Honest limit:** the level tracker follows the *programme*, not the
  background, because this stage never sees a speech/silence decision.
  Sitting the bed far below it is what makes the approximation work.

### Changed
- Four RNG streams — `tapeBed`, `vinylBed`, `codecBed`, `comfort` —
  **appended** to `RngBank`, never inserted. Stream *k* is seeded from a
  function of *k* alone, so every pre-existing stream, and therefore every
  render ever made with an old `SEED`, is bit-identical.
- Vinyl, Packet and Codec panels moved to the dense two-row layout Tape and
  Crush already used. Only CD is still a flat single row.

### Notes
- **Levels are sample-rate invariant, and that took work.** Filtering white
  noise to a fixed bandwidth in Hz gives output power proportional to that
  bandwidth over `fs`, so a bed calibrated with a bare constant is 3 dB
  quieter every time the rate doubles — inaudible in the one render anybody
  tests, and wrong for every user at 96 kHz. Each bed normalises its first
  (and only white-fed) filter stage by that stage's exact analytic noise
  gain `sqrt(a/(2−a))`; everything downstream operates on an
  already-shaped signal whose spectrum is fixed in Hz. Normalising the
  *later* stages the same way would have reintroduced the dependence it
  exists to remove. **Measured 48 kHz vs 96 kHz: 0.13 dB.**
- **Where each bed is injected is load-bearing.** Tape and vinyl join the
  mono artifact bus upstream of the packet stage, so a lost packet conceals
  the media noise along with the programme — on real media they are the
  same signal. The codec bed goes after the codec, for the passband reason
  above.
- **Determinism.** Each bed draws a fixed count per sample from its own
  stream on its own sample schedule, which is block-size invariant; the
  interleave hazard is two subsystems *sharing* a stream at different
  block-relative instants, not per-sample draws as such. That is also why
  the conditional extra draws taken when a tick or a crackle burst fires
  are safe — they depend on the private stream's own position, never on a
  block boundary.

### Testing
- **80/80 render-harness probes pass** (66 → 80; 14 new).
- **`V1` still matches the v1.3.0 digest** `0x3ee4e028900e47ca`, and a new
  **`N7` matches the v1.4.0 digest** `0x1cf2f80d1f71674c` for a canonical
  render with packet *and* codec active. V1 runs with both post-stages off,
  so on its own it says nothing about the two places v1.5.0 actually
  touched downstream code; N7 covers exactly those. The v1.4.0 digest was
  produced by compiling that probe against the v1.4.0 tree (git `2160dd66`)
  in a throwaway worktree — re-rendering the new engine twice would prove
  nothing.
- `N1b` asserts the vinyl bed is **bit-exact zero**, not merely quiet, at
  `VINYL_WEAR` 0 — that exactness is what keeps the FUNC-02 null intact at
  the shipped defaults.
- `N6` re-runs same-seed and ragged-block-size bit-identity with all four
  new streams drawing at once (`{1, 7, 64, 333, 4096}` vs `{512}`).
- auval PASS; pluginval strictness-10 **3/3 SUCCESS**.

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
