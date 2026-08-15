# Stage 4 — Polish · Phase 4.2 (host-and-ear) — Plan

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.2 of 2 — the last phase of the project**
**GSD phase:** plan
**Date:** 2026-08-13
**Branch:** `feat/o-octagon` @ `4952a8ca`
**Inputs:** `CONTEXT-4.2.md` (D11–D21, Q1–Q6), `RESEARCH-4.2.md` (N7–N15, P101–P108 requested)
**Closes:** `COMPAT-02` (3 criteria) and `QUAL-01` criterion 2's audible clause
**Ledger target at 4.2 close: 30 complete · 0 partial · 0 pending — of 30**

---

## Entry Check — contract checksums

Per the standing rule this boundary inherits (VERIFICATION-4.1 Issue 2, applied at both 4.2
boundaries so far): **compare against `STATUS.md`'s live `contract_checksums` block — the ledger —
never against a value quoted in a prior artifact's prose.** Measured against `STATUS.md:1169-1178`:

| Contract | `shasum -a 256` at arrival | `STATUS.md` ledger | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ **thirteen consecutive phases unmoved** |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ 17 parameters; **4.2 adds none** |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…57bceb17` | ✅ |
| `ROADMAP.md` | `ea50d991d1a6b158…1063d424` | `ea50d991…1063d424` | ✅ the 4.2-discuss re-pin (D11/D15/D20) |

**No drift. No contract is amended at this boundary** — the first plan boundary in Stage 4 where that
is true. P107 and P109 both touch *source prose*, not a checksummed contract, and P107 defers.

### Numbering

The D-series belongs to discuss (4.2 ran D11–D21). **The P-series continues from P100: 4.2 holds
P101–P112.** Phase probe letters continue from **CQ** — 4.2 holds **CR-a, CR-b, CS, CT, CU**, none of
which is a C++ probe. **The C++ probe count does not move: 95, unchanged**, and that is an assertion
(Gate 2), not an omission. **JS gate sections move once, deliberately: 69 → 70** (P101, P110).

Negative controls restart per phase, as they did at 3.3 (NC1–NC8) and 4.1 (NC1–NC5). **4.2 holds
NC1–NC4.**

---

## Goal

**Close the last requirement row with measurements a second person could re-derive, and be honest in
the artifact about which half of each claim the rig could reach.**

4.2 has no code to write beyond one doc-comment and one test section. What it has is nine
observations that have never been made, four of them carried across four or more phases, and a rig
that can reach most but not all of what the criteria say. The phase's whole risk is **overclaiming**:
running a test that looks like it proves something and does not.

### What "works" means concretely at 4.2

- **`COMPAT-02` criterion 2 is per-channel sample data**, asserted by a committed script — not eight
  moving meters, and not a bounce (N10 killed that).
- **The bounce-order test is a PAIR, and the analyser refuses to grade CR-b against the identity**
  (D20, N13). The anti-vacuity rule is enforced by the tool, not by the operator remembering it.
- **The LFE claim in `VenueModel.cpp:84` either cites a measurement or triggers D16** — and the
  measurement covers gain *and* low-pass, on *both* the render path and the monitor path (P111).
- **Q5's mechanism is executed, not read** — for the first time in five phases — and a negative
  control proves the execution can fail (NC1).
- **Every named deferral in the artifact has an owner**, including the three that acquire one here.

### The four findings this plan must not lose

1. **N10 — the verify-ping cannot be bounced.** `prepareToPlay` → `verifyPing.prepare()`
   (`PluginProcessor.cpp:258`) sets `phase = idle`, `activeFlag = false` (`VerifyPing.cpp:79-86`), and
   Logic calls `prepareToPlay` at the start of every offline bounce. Criterion 2 needs a **realtime**
   capture (**P102**).
2. **N8/N9 — Gate 13's Q5 item is vacuous AND its readout is unreachable.** The completion gate is
   `Component::isVisible()`, the component's own flag, set once at `PluginEditor.cpp:1139` and never
   cleared; and `meters` is a module-scope binding never attached to `window` (`app.js:639`). The test
   as written passes on a build with a permanently latched guard (**P101**).
3. **N11 — `airAmount` puts a position-dependent low-pass in the path** (`HullProcessor.h:123`,
   confirmed at this boundary). Two slots fed from instances at different positions differ in `fc`,
   which reads exactly like bass management and would trigger D16's re-freeze **on nothing**
   (**P103**, **NC4**).
4. **N13 — the CR-b trap reappears inside the tool.** A `--expect` that is not asserted non-identity
   lets a CR-b run against a venue that silently failed to load report a CR-a pass (**P105**).

### The premise correction that shapes the phase

`CONTEXT-4.2` D18 scoped 4.2 as **one session**. After N8 and N10 that is no longer true, and the
correction is structural rather than cosmetic: **the phase is now two desk blocks around one session**,
and the first desk block ends in a **re-freeze**. D19 alone forced that (it edits `Source/`), but N10
and N8 add work to the same block, and P110 exists to make sure the block closes exactly once.

**One further correction, made here so a task is not wasted on it.** `RESEARCH-4.2` N15 reports three
Stage-3.3 evidence files existing "today only as ignored files". Measured at this boundary: on
`feat/o-octagon` the Stage-3.3 evidence directory holds
`ao-residual-attribution-3.3-verify.txt`, `room-screen-3.3.png` and `standalone-verify-3.3.png` —
**all committed, none ignored**. The three `*.log` files are untracked debris in the *shared* checkout
left by a sibling session, not artifacts of this branch. **N15's rule is prospective and correct
(P108); the remediation it implies is not needed.**

---

## Requirement staging — read this before writing the verify report

| Row | Entering 4.2 | 4.2 does | At 4.2 close |
|---|---|---|---|
| `COMPAT-02` | ⏸️ pending, 0 of 3 — **the only open row of 30** | criterion 1 (instantiation **+ session recall**), criterion 2 (**CT**, realtime capture), criterion 3 (automation **writability**) | ✅ **complete, 3 of 3**, criterion 2 with D11's stated scope |
| `QUAL-01` | ✅ complete, audible clause **bounded** | **CU** — the clause concluded either way (D17) | ✅ complete, clause **concluded** |
| `COMPAT-01` | ✅ complete, re-confirmed at 4.1 | **re-run on the new freeze** — P110 replaces the binary | ✅ complete, re-confirmed **twice** |
| `COMPAT-04` | ✅ complete, 3 of 3 | **re-run on the new freeze** (95 probes, `auval`) | ✅ complete, re-confirmed |

**Ledger: 29 complete · 1 pending → 30 complete · 0 pending.** `COMPAT-01` and `COMPAT-04` are not
re-opened; they are re-evidenced against the binary that actually ships, because P110 makes the 4.1
freeze no longer the shipping binary.

### `COMPAT-02` — how each criterion closes, and what it does not close

| # | Criterion | Closed by | Residual |
|---|---|---|---|
| 1 | Instantiates on a surround track with 7.1 output | Logic 12.3 on BlackHole 64ch; the `getStatus` pre-flight; **plus a save / quit / reopen recall check**, which 2.1 could not do | none |
| 2 | Verify-ping reaches 8 distinct **physical** channels | **CT** — realtime loopback capture, per-channel energy in eight 1.6 s windows, sequence `1..8` | **one specific hardware driver — owner: none** (D11). The wording is deliberately unchanged |
| 3 | `srcX`/`srcY`/`srcZ` + `w1..w8` visible **and writable** per-parameter in automation lanes | Logic automation lanes, per parameter, latch-write on a moving playhead and read back | none |

> **Criterion 2's residual is a scope, not a re-wording** (D11). This project has been caught three
> times by a check that stopped looking at what it claimed to look at. The criterion keeps its words;
> the artifact records what the rig reached.

---

## Plan Decisions

### P101 — Q5 splits, and the mechanism half runs as a **layout-check section over its own instance** *(N8, N9)*

`RESEARCH-4.2` §3.5 recommends the split and names `tests/ui-stub/` as the home. Two mechanics have to
be decided before that is executable, and both were measured at this boundary.

**Home: a new section in `tests/ui_layout_check.js`, not a new file.** That gate already builds the
stub tree and serves it itself (`:56`, and its header states the split from the static sibling
deliberately), already drives Playwright, and already owns the browser lifecycle. A new file would
duplicate all of it and add a second thing a human must remember to run.

**The readout: the section constructs its own `createMeters` instance.** N9 is correct that the app's
live `meters` binding is unreachable — it is a module-scope `let` in `app.js:639` with no `window`
handle, and Playwright's `page.evaluate` runs in page scope, so it cannot reach it either.
`js/meters.js:116` exports `createMeters(deps)`, a factory taking `deps.nativeFn`. The section
therefore does, in page scope:

```js
const m = (await import('./js/meters.js')).createMeters({ nativeFn: (n) => stubs[n], onLevels: () => {} });
```

— a fresh instance whose `getMeters` the test controls and whose `diagnostics()` it holds directly.
**No source change, no `window` handle, and the module under test is the shipped file byte-for-byte.**

**The stimulus and the assertion.** Measured constants: `METER_POLL_MS = 33`,
`GUARD_DEADLINE_TICKS = 5` (`meters.js:70`, `:75`), so the deadline is **165 ms**. The section:

1. starts the poll with a `getMeters` that returns a **never-settling** promise on the first call and
   resolves normally afterwards;
2. waits ~400 ms (> 2 deadlines);
3. asserts `diagnostics().dropped >= 1` — **the guard released on the deadline**;
4. asserts `getMeters` was called again *after* the drop — **the poll continued**.

Clause 4 is the one that matters. `dropped` incrementing proves the counter moved; the *call count
rising afterwards* proves the poll is not latched, which is the property `pattern_webview_completion_gated_on_isvisible`
is about.

**Why this is not §33 again.** Frontend §33 is a **static** section: it greps `meters.js` for
`performance.now()` and for a deadline-shaped comparison. It proves the deadline is *written*. It
cannot prove it *releases*. That gap is exactly the four-phase-old premise, and it closes here.

**The host half is kept and RELABELLED.** Minimise / ⌘H for 10 s in Logic, re-show, confirm the meters
follow the source again. `VERIFICATION-4.2.md` **must call it a throttling-recovery smoke check**, and
must state that it cannot drop a completion (the table at `RESEARCH-4.2` §3.3: `isVisible()` stays
true under minimise, ⌘H, occlusion and Spaces). Without that sentence the next reader inherits the
false premise a fifth time.

**Rejected: exposing `meters.diagnostics` on `window`.** It would make the app's live instance
readable and could ride P110's re-freeze. It also widens a re-freeze that otherwise touches one
doc-comment into one that touches shipped page behaviour, to buy a convenience the dynamic import
already supplies.

### P102 — Criterion 2 is a **realtime loopback capture**, because the ping cannot be bounced *(N10)*

Rig, needing no new tooling and no `ffmpeg`/`sox` install:

- **BlackHole 64ch as Logic's device for both input and output** — measured at this boundary via
  `system_profiler SPAudioDataType`: **Input Channels 64, Output Channels 64**. Its outputs appear as
  its inputs.
- Eight mono audio tracks on inputs 1–8, record-armed.
- Roll the transport; start the ping in **auto cycle** (`VerifyPing.h:79` — `kAuto = -1`; `:115` mode 2).

**The timing is derivable, which is what makes the capture assertable rather than observed:** 1.2 s on
/ 0.4 s gap, and `kAutoCycleSeconds = kNumSpeakers * (kOnSeconds + kGapSeconds)` = **12.8 s**
(`VerifyPing.h:90`). Signal is band-limited pink noise, 200 Hz HP / 8 kHz LP, steady peak near
−7.5 dBFS (`VerifyPing.h:96-102`).

**Assertion:** in each of the eight consecutive 1.6 s windows, exactly one of the eight captured
channels carries energy above a stated floor, and the energised sequence is `1..8` in order.

**Two consequences the analyser must carry:**

1. **Window boundaries come from the energy envelope, not from sample zero.** Logic is recording the
   device it is playing to, so every file carries the round-trip delay.
2. **Logic records eight mono files, not one interleaved file.** See P104's second clause.

### P103 — `airAmount = 0` on every bounce test except the audible clause, and the LFE sources are co-located *(N11)*

Measured at this boundary: `HullProcessor.h:123` —
`fc = clamp (20000 · 2^(−airAmount · dHull / 3), 500, min(20000, 0.45·fs))`. The filter is applied to
the **source**, before the gain vector (`GainStage.cpp:226-227`), so it is common-mode across all
eight outputs **only when both slots under comparison are fed from the same instance at the same
source position.** Eight instances at eight positions have eight corner frequencies.

**Rule: `airAmount = 0` on CR-a, CR-b, CT and CS.** It pins `fc` at the ceiling independently of
position and takes the filter out of the path. **Belt and braces for CS: both sources at identical
`srcX/srcY/srcZ`.**

**And the rule is inverted for CU** — the air filter is the artifact under test there. Stated so it is
not applied where it would erase the measurement.

**The verdict table gains a row (D15 + this):**

| Observation | Reading |
|---|---|
| Broadband delta ≈ 0 **and** every partial delta ≈ 0 | `VenueModel.cpp:84` **confirmed**, citing a measurement |
| Broadband delta ≈ +10 dB, flat across partials | Logic applies the LFE gain offset → **D16** |
| Delta ≈ 0 at 31–125 Hz, increasingly negative above | Bass-management low-pass → **D16** |
| **Anything else** | **Check `airAmount == 0` and both sources co-located BEFORE invoking D16.** NC4 makes this concrete |

### P104 — Bounce **24-bit integer PCM WAV**; the analyser accepts one 8-channel file **or** eight mono files *(N12, P102)*

**Format.** Python 3.14's `wave` accepts only `WAVE_FORMAT_PCM` and `WAVE_FORMAT_EXTENSIBLE`
(`wave.py:386-387`); 32-bit float is `0x0003` → `Error: unknown format: 3`. 8-channel WAV *is*
EXTENSIBLE and reads fine. `audioop` was removed in 3.13, so 24-bit needs a manual 3-byte
little-endian sign-extending unpack (~8 lines). Local interpreter measured: **Python 3.14.2**. At
−20 dBFS the 24-bit floor is ~124 dB down; nothing here is near it.

**Input shape.** CT's rig produces **eight mono files** (Logic records per track); CR/CS produce **one
8-channel file**. Rather than adding an export step that could itself re-order channels — the exact
class of thing this phase exists to measure — `analyse_bounce.py --input` takes **one or more paths**:

- one file → its channels are the channels;
- N files → channel *k* is channel 0 of file *k*, in the order given on the command line.

Six lines in the tool, and it removes a whole re-ordering step from between the host and the evidence.

### P105 — `analyse_bounce.py`'s six anti-vacuity clauses are the tool's contract, adopted verbatim *(N13, §7.4)*

Mirroring `gen_dbap_reference.py`'s stated discipline — *"a vacuous oracle is worse than no oracle,
because it reports green"*:

1. **`--expect` is mandatory in `order` mode.** No default; a default would be the identity.
2. **A CR-b invocation refuses an identity `--expect`.** **This is N13.** Without it, a CR-b run
   against a venue that silently failed to load reports a clean CR-a pass — the check bypassed by the
   one mistake it exists to catch.
3. **Every expected tone must be found somewhere.** A missing tone is a **failure**, not a 7-of-8
   partial pass — it means a track was muted or an instance was not one-hot.
4. **Isolation floor**, ≥ 40 dB argmax over runner-up (generous, given `v_i` is *exactly* `0.0f` when
   `w_i == 0`). **Print the measured margin**, so the number is in the record and not only the
   threshold.
5. **Non-zero exit on any failure**, and no "OK" line on zero cases analysed.
6. **`--check`** re-runs the committed expectation table against the committed artifacts, so the pair
   stays verifiable after the session — the `gen_dbap_reference.py --check` precedent, which is a 4.1
   gate.

**Interface:**

```
analyse_bounce.py --input <wav> [<wav> …] --mode {probe,order,lfe,ping}
                  [--expect <perm>] [--label {CR-a,CR-b}] [--tones <csv>]
                  [--partials <csv>] [--channels <a,b>] [--check]
```

`probe` prints channel count / rate / depth / duration (Q2's two-minute pre-flight). `order` is the
8 × 8 Goertzel matrix. `lfe` is the per-partial table for two named channels. `ping` is P102's
envelope-segmented window assertion.

**Stdlib only.** All five existing `tests/tools/` scripts are; numpy and scipy are installed locally
but adding them would make this the one tool that cannot run on a bare interpreter. Goertzel is six
lines.

**Tones**, bin-exact at `N = fs` (1 Hz bins, no windowing choice enters the measurement):
`997, 1499, 2003, 2503, 3001, 3499, 4001, 4507` Hz.
**LFE partials**, log-spaced and bracketing any plausible crossover:
`31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000` Hz, Schroeder phases, composite peak ≤ −12 dBFS,
with the generator recording the exact per-partial level into the artifact.

### P106 — CR-b's fixture is a committed **8-cycle**, and the permutation is stated here *(N14, §5.4)*

`tests/fixtures/cr-b-permuted.venue` — the `VENUE`/`SPEAKER × 8` subtree as XML
(`VenueModel.h:64-66`, `VenueFile.h:79-89`), identical to the shipped default venue except for the
eight `@label` abbreviations. Loaded via `btn-venue-load` → `loadVenue` → `venuefile::load` into a
**fresh** model, then `applyVenueEditChecked()`.

**A derangement, not a transposition.** A two-element swap leaves six speakers on the identity, so six
columns prove nothing — D20's vacuity at one-quarter scale. The 8-cycle, with buffer indices derived
from ascending enum-bit order (`critical_audiochannelset_is_a_bitset_not_an_order`;
`ChannelMap.cpp:105-151` asserts it):

| Speaker | Default label (CR-a) | CR-b label | CR-b buffer idx | **CR-b bounce channel** |
|---|---|---|---|---|
| 1 | `L` (idx 0) | `R` | 1 | **2** |
| 2 | `R` (idx 1) | `C` | 2 | **3** |
| 3 | `C` (idx 2) | `Lfe` | 3 | **4** |
| 4 | `Lfe` (idx 3) | `Lss` | 4 | **5** |
| 5 | `Lss` (idx 4) | `Rss` | 5 | **6** |
| 6 | `Rss` (idx 5) | `Lrs` | 6 | **7** |
| 7 | `Lrs` (idx 6) | `Rrs` | 7 | **8** |
| 8 | `Rrs` (idx 7) | `L` | 0 | **1** |

`--expect` is *the channel carrying tone k, for k = 1..8*:
**CR-a → `1,2,3,4,5,6,7,8`. CR-b → `2,3,4,5,6,7,8,1`.** Every speaker moves.

**All eight CR-b labels are members of `create7point1()`** — verified against the same enum table; the
permutation is a re-assignment within the set, so `buildSpeakerToBuffer` cannot fail `labelNotInSet`
on it.

> **Consequence for CS:** under CR-b, speaker 4 carries `Lss` and speaker 3 carries `Lfe`. **The LFE
> test runs under the CR-a identity venue**, where speaker 4 is the LFE slot. Stated because loading
> the CR-b fixture and then running CS would measure the wrong slot and read as a clean result.

**Failure is detected, not silent:** if one of the eight instances fails to take the fixture, the
matrix is mixed and the analyser reports a mismatch. That is why eight manual loads is acceptable work.

### P107 — `VenueModel.cpp:87-89`'s "all three containers" prose is a **v1.1 doc row**, not a re-freeze *(N7 §1.4)*

The comment says the default is *"the identity under all three accepted 8-channel containers"*. It is
the identity under `create7point1()`; under `create7point1SDDS()` and `create5point1point2()` four of
its eight label types are absent from the set, so it does not resolve at all — the plugin's own probe
E says so (`tests/unit/main.cpp:69`).

The *operative* claim in that comment — that a map test driven by the default alone is vacuous — is
correct, and is the reason D20 exists. The behaviour is right, tested, and banner'd. **Log a v1.1 doc
row; do not widen the re-freeze for a sentence.**

### P108 — Evidence naming: **no `*.log`** *(N15)*

`.gitignore:217` is `*.log`. Verified at this boundary via `git check-ignore`: `.wav`, `.venue`,
`.txt`, `.py` are **not ignored**; `.log` is. All 4.2 evidence goes to
`stages/4-polish/evidence/` as `.txt` / `.wav` / `.png`, and tooling to `tests/tools/` and
`tests/fixtures/`. **N15's implied remediation is not needed** — see the premise correction above.

### P109 — D19's re-spell must also **de-match its own doc-comment** *(VERIFICATION-4.1 Issue 1)*

Measured at this boundary on `4952a8ca`:

| Search | Hits in `Source/` |
|---|---|
| `presetManager.loadPreset (` — **the gate exactly as PLAN-4.1 spells it** | **0** |
| `\.loadPreset[[:space:]]*\(` — receiver-agnostic | **2**: `PresetPolicy.h:202` (the doc-comment describing the gate) and `:222` (`manager.loadPreset (presetName)`, the real call) |

So a naive re-spell to "exactly one hit" is **off by one on day one**, because the gate's own
description matches the gate's own pattern. The next reader would "fix" that by loosening the count to
two — and a real second call site would then be invisible.

**Therefore the edit is two lines, not one:**

1. `PresetPolicy.h:202` — restate the gate receiver-agnostically **and write it so it does not match
   its own regex** (name the method without the opening paren).
2. The gate becomes: `grep -rnE '\.loadPreset[[:space:]]*\(' Source/` returns **exactly 1** hit, and
   it is the call inside `loadPreserving`.

**And the parting rule applies:** the same literal is carried in `PLAN-4.1.md` Gate 16 and Task 6.
Those are history and are **not edited** — 4.1's own rule is that history is not rewritten to look
correct. The correction lives here, in `VERIFICATION-4.2.md`, and in the source comment.

### P110 — **One re-freeze, scheduled once, before the session** — and the JS section count moves with it

D19 edits `Source/`. P101 edits `tests/`. Constraint 1 of `CONTEXT-4.2` is that any code change
re-cuts the freeze and re-runs 4.1's gates. **So the freeze is re-cut exactly once, at the end of the
first desk block, and nothing in `Source/` is touched after it.** If a session finding forces a change
(D16), that is a *second* freeze and it re-runs the whole gate set again — which 4.1 proved is a cost
in hours, not a re-plan, because the build is bit-reproducible.

**Re-freeze obligations, all measured on the new SHA:**

- forced full recompile, **95 probes / 0 failures**, zero `warning:`;
- **JS gates 42 + 28 = 70 sections** — see below;
- pluginval s10 VST3 ×3 / AU ×3, then `auval -v aufx OuOc OuDv`;
- `ci-tests.yml` green **with a run URL whose `headSha` is the new freeze commit**;
- new bundle checksums recorded, and **re-derived by a second forced recompile** — 4.1 established
  bit-reproducibility, so a mismatch is a real signal;
- install via `./scripts/build-and-install.sh O-Octagon`, `⚠ Sweeping ALTERNATE-variant` recorded
  present or absent.

**The JS section count moves 69 → 70, and that is stated rather than discovered.** 4.1's Gates 3 and 4
pass at "42 and 27, unchanged". P101 adds one layout section. A re-run against the old numbers would
report a failure that is actually the plan working. **New invariant: frontend 42, layout 28, total 70.**

### P111 — CS runs on **both** the render path and the monitor path, because they can differ

A bounce measures Logic's **render** path. `VenueModel.cpp:84`'s claim — *"Logic applies no automatic
bass management or LFE low-pass, so that slot carries a full-range feed to an ordinary speaker"* — is
about what reaches an output. A host that applied LFE handling only in monitoring would satisfy a
bounce test and still be wrong about the sentence.

**The second measurement is nearly free**: P102 already builds a loopback rig on the same device. Run
the same two-track project through it in realtime and capture, then compare the two per-partial
tables.

| Result | Reading |
|---|---|
| Both paths flat | `:84` confirmed **for both paths** — the strongest available form of the claim |
| Both paths show the same delta | Logic touches slot 4 → **D16** |
| **Bounce flat, realtime not** | The claim is true of delivery and false of monitoring. **Not D16** — a doc correction naming which path, and a named residual |

The third row is the one worth having; without the second capture it would be invisible and the
sentence would ship overclaimed.

### P112 — The session is a **stop-list**, and two gates halt the phase

Cheap gates first, and their failure dispositions decided now rather than under time pressure at the
end of a run — the same reason D16 and D17 were settled at discuss.

| Order | Gate | If it fails |
|---|---|---|
| 1 | **Q2 — surround bounce works on BlackHole.** A 4 s bounce of a 7.1 project, `--mode probe` reads 8 channels / 24-bit / the expected rate | **STOP.** D13 fails; the rig decision reopens. Two minutes spent |
| 2 | **The `getStatus` pre-flight** — `mapInvalid == false`, `numOutputChannels == 8`, `safeMode == false`, `outputSetName` recorded (all four already on the 2 Hz status poll and rendered in the UI; `PluginEditor.cpp:468-471`) | **STOP and fix the format selection** before anything else. Under SDDS or 5.1.2 four of the eight default labels are absent from the set, `buildSpeakerToBuffer` fails `labelNotInSet`, and the map retained is **stale** — every later test would measure it while producing eight channels of something |

Gate 2 is Q1's real answer. Logic's ten named surround formats are fixed and are not a function of the
device (`critical_logic_only_named_surround_formats`); what the device gates is whether the I/O
Assignment page has eight outputs to assign, and BlackHole 64ch has sixty-four. **The question that
can actually fail is which 7.1**, and it is one line in the plugin's own UI.

---

## Tasks

**Three blocks. The block boundaries are hard**: Block A ends in a commit, Block B ends in the
re-freeze, and **no task in Block C may touch `Source/`.**

### Block A — desk, before the freeze

#### Task 1 — Commit the 4.1 verify artifacts *(D21)*

- **Files:** `VERIFICATION-4.1.md` (untracked), `REQUIREMENTS.md`, `STATUS.md` (modified), plus this
  phase's `CONTEXT-4.2.md`, `RESEARCH-4.2.md`, `PLAN-4.2.md` and the amended `ROADMAP.md`
- **Depends on:** none
- An uncommitted verify is a verify nobody else can see
  (`pattern_uncommitted_improve_versions_lost`). This lands **first** so nothing later is at risk.

#### Task 2 — D19 / P109: re-spell Gate 16b, two lines

- **Files:** `Source/Data/PresetPolicy.h:202`
- **Depends on:** none
- Receiver-agnostic, **and written so the comment does not match the gate's own regex**.
- Immediately verify: `grep -rnE '\.loadPreset[[:space:]]*\(' Source/` returns **exactly 1**, and it
  is `:222`.
- **Touches no behaviour.** It is still a `Source/` edit and therefore still a re-freeze (P110).

#### Task 3 — `tests/tools/analyse_bounce.py` *(P104, P105)*

- **Depends on:** none
- Four modes (`probe`, `order`, `lfe`, `ping`), stdlib only, 24-bit sign-extending unpack, Goertzel at
  `N = fs`.
- **One-or-many `--input`** (P104), envelope-based window discovery for `ping` (P102).
- **All six anti-vacuity clauses**, especially clause 2's non-identity assertion.
- `--check` mode from the outset — it is what keeps the artifacts verifiable after the session.

#### Task 4 — `tests/tools/gen_bounce_sources.py` + `tests/tools/gen_audible_source.py` *(P105, §8)*

- **Depends on:** none
- `gen_bounce_sources.py` emits the **eight tones** (one 24-bit mono WAV each) and the **LFE
  multitone** (Schroeder phases, composite peak ≤ −12 dBFS, **per-partial level written into a
  sidecar `.txt`** so the expected deltas are derivable rather than remembered).
- `gen_audible_source.py` emits the deterministic bright signal for CU — sustained 6–16 kHz content
  plus transients. Byte-identical on re-run, which is what makes CU's difference signal exact.

#### Task 5 — `tests/fixtures/cr-b-permuted.venue` *(P106)*

- **Depends on:** none
- The 8-cycle of P106's table; otherwise identical to the shipped default venue.
- **Verify before use:** load it in Standalone and confirm `mapInvalid == false` — all eight labels are
  members of `create7point1()`, and this is the cheapest possible check of that.

#### Task 6 — P101: the layout-check section, and NC1 *(N8, N9)*

- **Files:** `tests/ui_layout_check.js` (new section 28)
- **Depends on:** none
- Dynamic-import a **fresh** `createMeters` instance in page scope with a controlled `nativeFn`;
  never-settling promise for one tick; wait ~400 ms (deadline is 33 × 5 = **165 ms**); assert
  `dropped >= 1` **and** that `getMeters` was called again afterwards.
- **NC1 in the same task**: delete the deadline release (`meters.js:151-152`), confirm section 28
  **fails**, revert, confirm the file is byte-identical. A section that has never failed is not yet a
  gate.

### Block B — desk, the re-freeze

#### Task 7 — Re-freeze and re-run 4.1's 18 gates *(P110)*

- **Depends on:** Tasks 1–6
- Commit Block A. Forced full recompile of all five targets.
- **95 probes / 0 failures** (unit 45, harness 50) — 4.2 adds no C++ probe and this asserts it.
- **JS: 42 + 28 = 70**, neither SKIPped. This is the new invariant (P110).
- pluginval s10 VST3 ×3 / AU ×3, then `auval -v aufx OuOc OuDv` (**AU VALIDATION SUCCEEDED** + the six
  `AUChannelInfo` configs).
- Push; `ci-tests.yml` green, **run URL recorded, `headSha` == the new freeze commit**.
- `./scripts/build-and-install.sh O-Octagon`; record the `⚠ Sweeping ALTERNATE-variant` line present or
  absent.
- Record the new freeze: **SHA + both bundle checksums + probe count + CI run URL**, and confirm both
  checksums reproduce from a second forced recompile.

### Block C — the session (Logic 12.3, BlackHole 64ch). **No `Source/` edits.**

#### Task 8 — The two stop-gates *(P112)*

- **Depends on:** Task 7
- Q2: 4 s surround bounce → `analyse_bounce.py --mode probe`. **Stop on failure.**
- `getStatus` pre-flight: `mapInvalid == false`, 8 channels, `safeMode == false`, `outputSetName`
  recorded, **banner state screenshotted** to `evidence/`. **Stop on failure.**

#### Task 9 — `COMPAT-02` criteria 1 and 3

- **Depends on:** Task 8
- Criterion 1: instantiation on a 7.1 surround track, **plus save / quit Logic / reopen** — does the
  negotiated set survive session recall? Unobserved by anyone; 2.1 saw a fresh instantiation only.
- Criterion 3: for **each** of `srcX`, `srcY`, `srcZ`, `w1..w8` — the lane exists, a value can be
  **written** (latch on a moving playhead), and it reads back. Visibility alone is what 2.1 had.

#### Task 10 — **CT**: the realtime ping capture — `COMPAT-02` criterion 2 *(P102)*

- **Depends on:** Task 8
- BlackHole 64ch in **and** out; 8 mono tracks armed on inputs 1–8; ping in auto cycle; ≥ 12.8 s.
- `analyse_bounce.py --mode ping --input ch1.wav … ch8.wav`.
- **Assert:** eight windows, exactly one channel energised per window above the floor, sequence
  `1..8`. Print the measured isolation margin.

#### Task 11 — **CR-a** then **CR-b**: the bounce-order pair *(D20, P106)*

- **Depends on:** Task 8
- Eight surround tracks, eight instances, one-hot `w`, **`airAmount = 0`**, identical `srcX/Y/Z`; tone
  *k* on track *k*. Interleaved Surround Bounce, **24-bit PCM WAV**.
- **CR-a:** shipped default venue → `--mode order --label CR-a --expect 1,2,3,4,5,6,7,8`.
- **CR-b:** `cr-b-permuted.venue` loaded into **all eight** instances →
  `--mode order --label CR-b --expect 2,3,4,5,6,7,8,1`.
- **NC2 here:** re-run the CR-b WAV as `--label CR-b --expect 1,2,3,4,5,6,7,8`. The tool must
  **refuse**, not report a pass (P105 clause 2).
- **NC3 here:** mute one track and re-bounce (or drop one tone from the run). The tool must **fail**,
  not report 7-of-8 green (clause 3).

#### Task 12 — **CS**: the LFE test, widened and doubled *(D15, P103, P111)*

- **Depends on:** Tasks 8, 11
- **Under the CR-a identity venue** (P106's note). Two tracks, `airAmount = 0`, **identical
  `srcX/Y/Z`**, one-hot on speaker 4 (LFE) and speaker 1 (reference). LFE multitone into both.
- **Bounce**, then **the same project captured through the realtime loopback** (P111).
- `--mode lfe --channels 4,1` on each. Compare per-partial and broadband against P103's verdict table.
- **NC4 here, before any D16 disposition:** set `airAmount` non-zero on one of the two sources and
  re-measure. A per-band HF delta must appear that is **not** bass management. This is the control
  that stops a confound costing a re-freeze and eighteen re-run gates.
- If D16 is invoked: **stop, and re-enter Block B.** It is a second freeze, not a patch.

#### Task 13 — **CU**: the audible clause *(D12, D17, §8)*

- **Depends on:** Task 8
- Two renders of the same project differing **only** in the gesture: one hull-crossing gesture, and a
  genuinely **static** source position for the null. Same source file, same start time.
- **Half 1 — the locator:** difference signal, soloed. *Is there a step, and where?* Audibility here
  does **not** mean audibility in context.
- **Half 2 — the requirement:** the full bounce, in context, on the **named** ecological source
  (one bright commercial/Apple Loop, named in `VERIFICATION-4.2.md`, not committed — licence).
- **Name the headphones in the artifact.** D12's stated residual; a one-sample step of ~15 % of an
  8 kHz component is exactly what a cheap transducer hides, and an unnamed monitoring path makes the
  clause unreproducible.
- **Either outcome closes the clause** (D17). If it ticks: log it, ship v1.0, open a v1.1 row. The
  lever is RESEARCH-2.3 H3 and it re-tunes the whole musical air curve — a discuss-boundary change.

#### Task 14 — Gate 13's interactive half + the relabelled throttling check *(D18, P101)*

- **Depends on:** Task 8
- ~15 min Standalone launch-and-drive: every control, the Venue screen, the Room screen, preset
  load/store, the banner.
- The hidden-editor check: minimise / ⌘H for 10 s with signal running, re-show, meters follow the
  source again. **Recorded as a throttling-recovery smoke check**, with the sentence stating it cannot
  drop a completion.

#### Task 15 — `REQUIREMENTS.md`, `STATUS.md`, docs, and the close

- **Depends on:** Tasks 8–14
- `COMPAT-02` 3 of 3, each criterion with **its probe letter and its measured figure**; criterion 2
  carries D11's scope note and the owner-none residual. `QUAL-01` criterion 2's clause concluded.
- **Ledger 30 complete · 0 partial · 0 pending — of 30.** `openRows:` empty.
- `pattern_evidence_line_orphaned_past_next_heading`: **count `[x]` against `→ **` per section** before
  closing.
- `CHANGELOG.md`, `NOTES.md` (the CR-a canonical-order result, the LFE measurement, P107's v1.1 row),
  `PLUGINS.md:68`.
- `analyse_bounce.py --check` committed alongside the artifacts and green.

---

## Gates

Every gate is **run at execute and RE-RUN FROM SCRATCH at verify**, never read out of a summary. That
discipline has caught eleven mis-attributions across 2.3, 3.1, 3.2, 3.3 and 4.1 — most recently Gate
16b, which the summary had silently re-worded.

### Pre-session (desk)

| # | Gate | Pass condition |
|---|---|---|
| 1 | Contract checksums + `STATUS.md` agrees | four measured exact against the **ledger**; no amendment this phase |
| 2 | Forced full recompile + both C++ targets | exit 0, zero `warning:`/`error:`/`FAILED`; **95 probes, 0 failures** (unit 45, harness 50) — 4.2 adds none |
| 3 | `node tests/ui_frontend_check.js` | exit 0, **42 sections — unchanged** |
| 4 | `node tests/ui_layout_check.js` | exit 0, **28 sections — 27 + P101's new one**, must not SKIP |
| 5 | **Gate 16b, re-spelled** | `grep -rnE '\.loadPreset[[:space:]]*\(' Source/` returns **exactly 1**, and it is the call in `loadPreserving` — **not** the doc-comment (P109) |
| 6 | **NC1** | section 28 **fails** with the deadline release removed; tree byte-identical after revert |
| 7 | pluginval s10 VST3 ×3 / AU ×3 + `auval` | all six exit 0, zero `FAILED`; **AU VALIDATION SUCCEEDED** + the six `AUChannelInfo` configs |
| 8 | `ci-tests.yml` green on the **new** freeze SHA | a run URL whose `headSha` is the new freeze commit |
| 9 | `gen_dbap_reference.py --check` | exit 0, **102 cases** — 4.2 does not touch the solver, and this is what proves it |
| 10 | Install + dual-variant sweep | both bundles installed; `⚠ Sweeping ALTERNATE-variant` **recorded present or absent** |
| 11 | The re-freeze is **bit-reproducible** | a second forced recompile reproduces both bundle checksums byte-for-byte |

### Session (host-and-ear)

| # | Gate | Pass condition |
|---|---|---|
| 12 | **Q2 — surround bounce on BlackHole** | an 8-channel 24-bit PCM WAV, read by `--mode probe`. **STOP on failure** |
| 13 | **The `getStatus` pre-flight** | `mapInvalid == false`, `numOutputChannels == 8`, `safeMode == false`, `outputSetName` recorded + screenshot. **STOP on failure** |
| 14 | `COMPAT-02`/1 | instantiates on 7.1, **and the set survives save / quit / reopen** |
| 15 | `COMPAT-02`/3 | all 11 lanes (`srcX/Y/Z`, `w1..w8`) **visible and written and read back**, per parameter |
| 16 | **CT** — `COMPAT-02`/2 | eight 1.6 s windows, exactly one channel energised per window, sequence `1..8`, isolation margin printed |
| 17 | **CR-a** | `--expect 1,…,8` passes; Logic's canonical interleaved 7.1 bounce order recorded |
| 18 | **CR-b** | `--expect 2,3,4,5,6,7,8,1` passes with the non-identity assertion active |
| 19 | **NC2 + NC3** | NC2: the tool **refuses** an identity `--expect` on a CR-b label. NC3: a missing tone is a **failure**, not 7-of-8 |
| 20 | **CS** — LFE, both paths | per-partial + broadband tables for bounce **and** realtime loopback, read against P103's verdict table |
| 21 | **NC4** | with `airAmount` non-zero on one source, a per-band HF delta appears that is **not** bass management — run **before** any D16 disposition |
| 22 | **CU** — the audible clause | both halves run, **headphones named**, outcome recorded either way (D17) |
| 23 | Gate 13's interactive half | ~15 min drive complete; the hidden-editor check **recorded as throttling-recovery**, with the sentence that it cannot drop a completion |
| 24 | `~/Library/O-Octagon/Presets/User/` byte-identical | across the whole phase, desk and session |
| 25 | `analyse_bounce.py --check` | exit 0 against the committed artifacts, after the session |

**Gates 12 and 13 are the two that stop the phase**, and they cost two minutes between them. **Gate 6
is the one that makes P101 a measurement** — without NC1, section 28 is an assertion about test design.

---

## Execution Constraints

1. **`airAmount = 0` on CR-a, CR-b, CT and CS. Never on CU** (P103, N11).
2. **CS runs under the CR-a identity venue.** Under CR-b, speaker 4 is not the LFE slot (P106).
3. **The analyser is stdlib-only.** numpy/scipy are installed locally; using them would make this the
   one tool in the repo that cannot run on a bare interpreter.
4. **Bounce 24-bit integer PCM, never 32-bit float.** Python's `wave` rejects `0x0003` (N12).
5. **Never edit `modules/persistence/preset-manager/`.** Nine plugins include that header and seven
   more carry vendored copies.
6. **No `Source/` edit after Task 7.** Anything later is a second freeze and re-enters Block B.
7. **Do not add a `window` handle for `meters.diagnostics`** (P101). The dynamic import supplies it.
8. **`parameter-spec.md` does not move.** 4.2 adds no parameter.
9. **`User/` presets are never written** (Gate 24).
10. **`--expect` is never defaulted, and a CR-b label never accepts the identity** (P105 clauses 1–2).
11. **A missing tone fails the run.** 7-of-8 is not a partial pass (clause 3).
12. **NC4 runs before any D16 disposition.** A confound here costs hours and lands a compensating trim
    the plugin does not need.
13. **Name the headphones, name the ecological source file** (D12's stated residual).
14. **pluginval ×3 per format before any conclusion** (`pattern_ci_pluginval10_catches_latent_nan`).
15. **`juce::String` construction:** any new non-ASCII user-facing string is built with `<<` onto a
    named local. 4.2 writes no such string; the constraint stands for the D16 path.

---

## Non-goals for Phase 4.2 — must not appear

- **New DSP, new UI, new parameters.** If CU ticks, the lever is RESEARCH-2.3 H3 — a
  **discuss-boundary change opening v1.1**, not a fix (D17).
- **Re-wording `COMPAT-02` criterion 2 to fit the rig.** D11 is explicit: the criterion keeps its
  words and the phase records what it reached.
- **A `window` handle for `meters.diagnostics`**, or any other convenience riding P110's re-freeze
  (P101).
- **Editing `PLAN-4.1.md`'s Gate 16 literal.** History is not rewritten to look correct; the
  correction lives here and in the source comment (P109).
- **Fixing `VenueModel.cpp:87-89`'s prose in this freeze.** v1.1 doc row (P107).
- **Building an aggregate device to fold 8 channels to the built-in output.** D12 settled the ear
  judgement on a bounce, and gave two reasons it is better.
- **Claiming Windows UI correctness**, **RT-safety beyond allocation**, **the two JS gates in CI**, or
  **spatial coherence in a hall.** All four are named deferrals with **owner: none**.
- **A 4.3.** D18 batches the session; a D16 finding re-enters Block B rather than opening a phase.

---

## Success Criteria

- [ ] **`COMPAT-02` closed — 3 of 3.** Criterion 1 including **session recall**; criterion 2 via **CT**
      with D11's stated scope and the owner-none hardware residual; criterion 3 **written and read
      back** on all 11 lanes
- [ ] **`QUAL-01` criterion 2's audible clause concluded** — both halves of D12 run, headphones and
      source named, outcome recorded either way
- [ ] **Ledger 30 complete · 0 partial · 0 pending — of 30**, `openRows:` empty
- [ ] **CR-a and CR-b both pass**, CR-b under the 8-cycle with the non-identity assertion active —
      Logic's canonical bounce order moves from MEDIUM confidence to measured
- [ ] **NC2 and NC3 behaved as declared** — the analyser refuses the identity on a CR-b label, and a
      missing tone fails
- [ ] **The LFE claim is settled on BOTH paths** — `VenueModel.cpp:84` cites a measurement, or D16 is
      invoked with NC4 already run
- [ ] **Q5's mechanism executed for the first time** — section 28 green, and **NC1 proves it can fail**
- [ ] **Gate 16b re-spelled and its own comment de-matched** — exactly one hit, and it is the call
- [ ] **The re-freeze is recorded and bit-reproducible** — SHA + both bundle checksums + 95 probes +
      70 JS sections + a CI run URL whose `headSha` is the freeze commit
- [ ] **`COMPAT-01` and `COMPAT-04` re-confirmed on the shipping binary**, not inherited from 4.1
- [ ] **`User/` presets byte-identical** across desk and session
- [ ] **No evidence file named `*.log`**; `analyse_bounce.py --check` green against the committed
      artifacts
- [ ] **Every deferral in `VERIFICATION-4.2.md` has an owner**, and the hidden-editor check is labelled
      as throttling-recovery

---

## Risks Active in This Phase

| Risk | Signature if it lands | What catches it |
|---|---|---|
| **Logic negotiates SDDS or 5.1.2** | Four default labels absent from the set; `buildSpeakerToBuffer` fails; the **stale** map is retained and the bounce still produces eight channels of something | **Gate 13.** `mapInvalid == false` read **before** the first bounce, never off one |
| **CR-b runs against a venue that silently failed to load** | A clean CR-a pass reported as CR-b — the check bypassed by the one mistake it exists to catch | **P105 clause 2**, enforced in the tool. **NC2** proves it fires |
| **`airAmount` confounds the LFE per-band delta** | An HF-band delta indistinguishable from bass management → D16 → a re-freeze and eighteen re-run gates, for nothing | **P103**'s rule + co-location. **NC4** run *before* any disposition |
| **The LFE claim is true of bounce and false of monitoring** | A flat bounce ships a sentence about "an ordinary speaker" that the monitor path contradicts | **P111** — the second capture, nearly free on CT's rig |
| **Section 28 passes without testing anything** | Q5 "closed" for the fifth time, by a probe whose stimulus never reaches the mechanism | **NC1.** A section that has never failed is not a gate |
| **The hidden-editor check is ticked as a completion-drop test** | The same false premise inherited a fifth time; WebKit throttling read as the mechanism | **P101's relabelling**, written into `VERIFICATION-4.2.md` as a sentence, not a footnote |
| **The re-spelled Gate 16b counts its own comment** | Off by one on day one; the next reader loosens it to 2 and a real second call site goes invisible | **P109** — de-match the comment, then assert exactly 1 |
| **The JS gate count is read against 69** | A re-run reports a failure that is the plan working | **P110**, stated at plan: 42 + 28 = **70** |
| **A `Source/` edit lands after the freeze** | The session runs against a binary that is about to be replaced | **Constraint 6.** A D16 finding re-enters Block B by design |
| **A criterion is quietly re-worded to fit the rig** | The artifact reads as if the physical half closed | **D11**, restated in `REQUIREMENTS.md` and `ROADMAP.md`; the residual carries **owner: none** |
| **A result is recorded without its provenance** | Nobody can re-derive it; the same hazard 4.1 verify recorded about the shared checkout | Every artifact names the freeze SHA and the tool invocation that produced it |

---

## Next Phase

**Ready for:** `execute`

**15 tasks in three blocks.** The two that carry the phase are **Task 10 (CT)** and **Task 12 (CS)** —
CT because it is the only assertion in 4.2 that turns criterion 2 from a judgement about moving meters
into sample data, and CS because it is the only one whose failure loops the stage. **Task 6 (NC1)** is
the smallest and the most overdue: it is the first execution of a mechanism five phases have described.

**The one ordering constraint that cannot bend:** Block B's re-freeze completes before Block C begins,
and nothing in `Source/` moves after it.

**At 4.2 close the ledger is 30 of 30 and the project is done** — with three residuals carrying
**owner: none** (one hardware driver, Windows UI correctness, RT-safety beyond allocation), each of
which is a property of an absent machine rather than of work left undone.
