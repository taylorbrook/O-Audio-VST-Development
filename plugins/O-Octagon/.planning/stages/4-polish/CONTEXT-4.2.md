# Stage 4 — Polish · Phase 4.2 (host-and-ear) — Context

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation
**Phase:** 4.2 of 2 — Logic Pro, bounce path, `COMPAT-02`, Gate 13's interactive half, Q5, the audible clause
**GSD phase:** discuss
**Date:** 2026-08-13
**Branch:** `feat/o-octagon` @ `4952a8ca` (freeze commit `fba35081` + the SUMMARY commit)
**Participants:** Taylor Brook, Claude

> **4.2 is the last phase of the project.** Every criterion below is host- or human-bound; nothing
> here can be closed at the desk by a machine alone, which is exactly why D1 split the stage.

---

## Entry Check — contracts, and the frozen binary

### The four contracts

Per the standing rule this boundary inherits (VERIFICATION-4.1 Issue 2): **an arrival check compares
against `STATUS.md`'s live `contract_checksums` block — the ledger — never against a value quoted in
a prior artifact's prose.** The 4.1-discuss check failed precisely by doing the latter. Measured
here against `STATUS.md:1053-1056`:

| Contract | `shasum -a 256` at arrival | `STATUS.md` ledger | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ **eleven consecutive phases unmoved** |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ 17 parameters, unmoved since Stage 1 |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…57bceb17` | ✅ the 4.1-discuss pin |
| `ROADMAP.md` | `90c651318ac7a1cc…fa562c92` | `90c65131…fa562c92` | ✅ the 4.1-plan re-pin |

**No drift on arrival.** The rule was followed as written and it cost nothing to follow.

**`ROADMAP.md` is then amended at this boundary** — see §Contract amendments. `BRIEF.md`,
`parameter-spec.md` and `research/ARCHITECTURE.md` are untouched and their pins are unmoved.

### Contract amendments at this boundary

**`ROADMAP.md` → `ea50d991d1a6b158…1063d424`** (was `90c65131…fa562c92`, measured exact on arrival
above — this is an authored change, not drift). Three sites in §Stage 4:

| Site | Change | Decision |
|---|---|---|
| The D2 amendment block + the stage goal line | The rig **is not an 8-channel interface** — none is attached. Re-stated as Logic Pro 12.3 + BlackHole 64ch, with both of D2's leans discharged explicitly | **D11** |
| Phase 4.2 bullet — bounce-order | Split into the **CR-a / CR-b pair**; as written it passes vacuously on the identity default | **D20** |
| Phase 4.2 bullet — LFE-gain | **Widened** to gain *and* low-pass, per band | **D15** |

`REQUIREMENTS.md` is not a checksummed contract, but `COMPAT-02` criterion 2 carries the same
"physical channels" wording and gains D11's scope note in the same edit.

> **This amendment exists because the stage's own parting rule was applied, not because the
> premise announced itself.** *When an amendment corrects a claim, grep the other contracts for
> the same claim before closing.* It fired at D7 one boundary ago; it fired again here. D11 was
> decided from a `system_profiler` device list, and only the grep revealed that `ROADMAP.md:403`
> and the stage goal line were still asserting the interface as fact.

### The binary 4.2 runs against — verified present, not assumed

4.1 verify's parting instruction was that 4.2 runs against `fba35081` and two bundle checksums, and
that the freeze is **bit-reproducible from source**, so a mismatch here is a real signal rather than
a build-nondeterminism excuse. Measured on the installed bundles:

| Bundle | Installed on disk now | Freeze record (VERIFICATION-4.1) | Result |
|---|---|---|---|
| VST3 | `c0fdd8f217f37e51…dce29844a` | `c0fdd8f2…dce29844a` | ✅ |
| AU | `1e04f0a8928ac4e5…5f7bda007` | `1e04f0a8…5f7bda007` | ✅ |

**The frozen 4.1 binary is what Logic will load.** Only `-dev` variants are on disk; no alternate
variant to shadow the AU registry slot. This was measured, not inherited — a sibling session checked
the shared tree out to another branch during 4.1 verify, so "it was installed then" was not
sufficient grounds.

### Environment

| Item | Value |
|---|---|
| Logic Pro | **12.3** |
| Output devices present | BlackHole 2ch, **BlackHole 64ch**, MacBook Pro Speakers (2), MBP Microphone (1 in), Teams, Zoom |
| Physical ≥8-out interface | **none attached** |
| Aggregate devices | **none configured** |

That last pair is the fact that shapes this whole phase — see **D11**.

---

## Carried obligations arriving at this boundary

| Carried from | Obligation | Disposition here |
|---|---|---|
| 4.1 verify, Issue 1 | **Gate 16b's literal does not match the code it guards** — `presetManager.loadPreset (` counts **zero** call sites; the parameter is named `manager` | **TAKEN as 4.2 work — D19.** One line, three sites |
| 4.1 verify, Issue 2 | The arrival-check rule | **APPLIED above.** Not carried further; it is now standing practice |
| 4.1 verify, Carried #3 | 4.2 runs against `fba35081` + the two bundle checksums | **DISCHARGED above** — both match on disk |
| 3.1 discuss (D2) → 4.1 (D2) | **QUAL-01 criterion 2's audible clause** | **TAKEN — D12, D17** |
| 3.3 verify → 4.1 (D9) | **Gate 13's interactive half** (~15 min Standalone launch-and-drive) | **TAKEN — D18** |
| 3.3 verify → 4.1 (D9) | **Q5 — a 30 Hz meter poll against a HIDDEN WKWebView.** Unrun by four consecutive phases | **TAKEN — D18.** `js/meters.js:227` exposes `diagnostics: () => ({ dropped, inFlight })`, confirmed present in shipped source |
| 3.3 verify → 4.1 (D10) | RT-safety beyond allocation (locks, file I/O) | **Unchanged, still not closed.** `-fsanitize=realtime` unsupported by Apple clang 17.0.0. Owner: none |
| 4.1 close | The two JS gates stay local-only | **Unchanged.** Blocked on headless-render determinism, not effort |
| 4.1 verify | Windows **UI correctness** | **Unchanged.** Owner: none, blocked on hardware |
| — (new, found at this boundary) | **The 4.1 verify artifacts are uncommitted** — `VERIFICATION-4.1.md` untracked, `REQUIREMENTS.md` and `STATUS.md` modified | **TAKEN — D21.** Commit before the host session, per `pattern_uncommitted_improve_versions_lost` |

### Numbering

The D-series restarts each stage; **Stage 4's 4.1 ran D1–D10, so 4.2's decisions are D11–D21.**
The P-series and probe letters do not restart: 4.1 closed at **P100** and probe **CQ**, so 4.2's
first plan decision is **P101** and its first new probe is **CR**.

---

## Discussion Summary

Four things were decided about *how* 4.2 runs, and three about *what happens when a test fails* —
the second group settled here rather than in the session, because this project puts dispositions at
discuss boundaries and not under time pressure at the end of a run.

---

## Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D11** | The rig | **BlackHole 64ch**, not a physical interface | None is attached. See below — the criterion this bends is named, not re-worded |
| **D12** | How the ear judgement is made | **Offline bounce, auditioned on headphones** | BlackHole is silent. The bounce is bit-identical to what realtime would render |
| **D13** | What the bounce tests need | BlackHole 64ch to **configure** the project; **no device to run the bounce** | Bounce is offline. The device only gates whether Logic offers a 7.1 output at all |
| **D14** | Evidence standard | **Committed analysis script + the bounced WAV** | `tests/tools/` precedent. A prose read-off is not re-derivable |
| **D15** | LFE test scope | **Widened — gain AND low-pass** | §6a as written is blind to half the claim it checks |
| **D16** | If the LFE test fails | **Fix, re-freeze, re-run 4.1's 18 gates** | A 10 dB hot speaker is a broken default, not an artifact |
| **D17** | If the audible clause ticks | **Log it, ship v1.0**, open a v1.1 row | The lever re-tunes the whole musical curve — a discuss-boundary change, not a fix |
| **D18** | Session scope | **One session: Logic + Gate 13 + Q5** | All three need the same thing — a human with signal running. No 4.3 |
| **D19** | Gate 16b | **Re-spell receiver-agnostically** | Carried from 4.1 verify Issue 1 |
| **D20** | Bounce-order anti-vacuity | **A non-identity label map is mandatory** | The shipped default is the identity — a map-driven test on it alone proves nothing |
| **D21** | Housekeeping | **Commit the 4.1 verify artifacts first** | An uncommitted verify is a verify nobody else can see |

---

### D11 — The rig is BlackHole 64ch, and the criterion it bends is named

The ROADMAP bullet reads:

> *Verify-ping confirms all 8 outputs reach distinct **physical** channels*

and 4.1 verify's human list sharpens it to *"distinct channels, not merely 8 moving meters."*

**What BlackHole 64ch does deliver, and it is more than a physical interface would:** it is a real
CoreAudio device with 64 output channels, so Logic's surround output assignment, its I/O routing and
the driver boundary are all exercised exactly as they would be for hardware. And unlike speakers,
its output can be **captured and measured per channel** — the verify-ping becomes an assertion about
sample data, not a judgement about which cone moved. That is strictly stronger evidence for the
routing claim, which is the claim the plugin is responsible for.

**What it does not deliver:** proof that one particular interface's driver behaves. That is a
property of a piece of hardware, not of O-Octagon, and no gate run against any single interface
would generalise to another.

**Disposition — stated as a scope, not as a re-wording.** Criterion 2 closes **against the CoreAudio
device boundary**, evidenced by per-channel capture. The residual — *one specific hardware driver* —
is recorded with **owner: none**, in the same register 4.1 used for Windows UI correctness and
RT-safety beyond allocation. It is not a deferral waiting on a future phase, because there is no
phase that could discharge it.

> This is deliberately **not** handled by editing the word "physical" out of the criterion. This
> project has been caught three times by a check that stopped looking at what it claimed to look at
> (`pattern_criterion_discriminator_states_outcome_backwards`, Gate 16b, the 4.1-discuss arrival
> tick). The criterion keeps its wording; the phase records what it did and did not prove.

If an 8-out interface becomes available before ship, the physical half can be closed in ten minutes
against the same frozen binary. That is worth doing and is not worth waiting for.

### D12 — The ear judgement runs off a bounce, and the method has two halves

BlackHole makes no sound. Rather than build an aggregate to fold 8 channels down to the built-in
output, the audible clause is judged on a **bounced file auditioned on headphones**.

**Why this is sound and not a workaround.** QUAL-03 proved block-size invariance and 4.1 proved the
binary is bit-reproducible; the bounce therefore contains *the same samples* a realtime pass would
produce. Nothing about the artifact under test — a one-sample step of ~15 % of an 8 kHz component on
a single hull-crossing gesture — is realtime-dependent.

**Why it is better.** A file can be looped on the exact gesture, and a **difference signal** can be
built (gesture render minus a null-gesture render) to establish whether there is anything there at
all.

**The two halves, and they are not interchangeable:**

| Half | What it answers | Status of the answer |
|---|---|---|
| Difference signal, soloed | *Is there any step at all, and where?* | A **locator**. Soloing removes masking, so audibility here does **not** mean audibility in context |
| Full bounce, in context, HF-rich material | *Does it tick?* | **The requirement.** This is the clause QUAL-01/2 actually carries |

If the difference signal is inaudible even soloed, the clause closes decisively. If it is audible
soloed but not in the full bounce, that is the honest middle answer and it is recorded as such.

**Residual introduced by D12:** the monitoring path is now a stated variable. The headphones used
must be named in VERIFICATION-4.2. A one-sample HF step is exactly the kind of thing a cheap
transducer hides.

### D14 + D20 — The bounce-order test, and the way it can pass vacuously

Evidence is a committed script plus the bounced WAV, following the `tests/tools/gen_dbap_reference.py`
precedent (which already ships a `--check` mode, and whose 102-case self-check is Gate 15).

**The trap, and it is written in the plugin's own source.** `VenueModel.cpp:87-89`:

> *"because this default is the identity under all three accepted 8-channel containers, a channel-map
> test driven by it alone is VACUOUS — a hardcoded 0..7 map would pass it (RESEARCH-2.1 C1/G5).
> Every map probe must drive a NON-identity assignment."*

The §6a bounce-order test as written drives exactly that default. Run only that way it confirms
Logic's canonical bounce order and says **nothing** about the label map — while looking like it
confirmed both.

**Therefore the test is a pair, and both runs are required:**

| Run | Label map | Expected | What it proves |
|---|---|---|---|
| **CR-a** | Shipped identity default | channel N carries speaker N's tone | Logic's interleaved bounce order is canonical for 7.1 (the §6 MEDIUM-confidence claim) |
| **CR-b** | A **non-identity** permutation | channels carry the permuted tones, matching the map | The label map is actually determining bounce order — the clause CR-a cannot see |

Source material: **eight distinct tones**, one per slot, chosen so the analysis script can identify
each channel unambiguously. Eight copies of one tone would make CR-b unreadable.

### D15 — The LFE test is widened, because half of a shipped claim is unverified

`Source/Data/VenueModel.cpp:84` states as fact:

> *"Speaker 4 → LFE is intentional and safe: Logic applies no automatic bass management or LFE
> low-pass, so that slot carries a full-range feed to an ordinary speaker."*

The locked research doc rates this **MEDIUM-LOW**, explicitly: *"No evidence found… This is absence
of evidence, not proof."* A shipped comment is asserting something a contract records as unproven.

The §6a test — one −20 dBFS tone into the LFE slot and one other — catches a **gain offset** and is
**blind to a low-pass** unless the tone happens to sit above the crossover. It would close half the
claim and read as if it closed all of it.

**Widened:** a multi-tone or log sweep into the LFE slot and a reference slot, compared **per band**.
Same bounce, better source material, both failure modes covered:

| Failure mode | Caught by |
|---|---|
| +10 dB LFE offset | Broadband level delta |
| Bass-management low-pass | Per-band delta, HF bands attenuated |
| Neither | Both deltas ≈ 0 → the `:84` comment is confirmed and can cite a measurement |

### D16 — What a failing LFE test costs, decided now

A delta means Logic touches slot 4, which makes the shipped default wrong. The disposition is
**fix, re-freeze, re-run**, and it loops the stage:

1. Add the compensating default trim for speaker 4 (FUNC-07's trim path already exists and is
   venue-scoped, applied after the DBAP solve — no new mechanism).
2. Correct `VenueModel.cpp:84` — it must state the measurement, not the assumption.
3. Correct `research/logic-pro-multichannel-octaphonic-dbap.md` §6's confidence row.
4. Re-cut the freeze; **re-run 4.1's 18 gates against the new binary** (they are scripted and
   reproducible, so this is a cost in hours, not a re-plan).
5. Re-run 4.2 against the new freeze.

The identity label map is **not** the lever — moving speaker 4 off the LFE slot would break
*channel N = speaker N*, which §6a calls the entire reason for the default.

### D17 — What an audible tick costs, decided now

**Log it and ship v1.0.** Record the observation with the material and monitoring named, open a v1.1
row, close the clause as *measured-and-heard, bounded*. The lever — RESEARCH-2.3 H3, raising
`fc(d_hull = 0)` toward Nyquist — re-tunes the entire musical air curve and is a **discuss-boundary
change, not a fix**; the ROADMAP already says so at D2.

Deciding this now is the point. Deciding it at the end of a listening session is how a curve gets
re-tuned to satisfy a judgement made once, tired, on one gesture.

---

## Requirements in scope

| Requirement | Priority | Status arriving | Target at 4.2 close |
|---|---|---|---|
| `COMPAT-02` — Logic Pro, 8 discrete channels | must | ⏸️ pending — **the only open row of 30** | ✅ complete, criterion 2 with D11's stated scope |
| `QUAL-01` — no artifacts | must | ✅ complete, audible clause bounded | ✅ clause concluded (either outcome, per D17) |

`COMPAT-02`'s three criteria and what closes each:

| # | Criterion | Closed by | Rig-limited? |
|---|---|---|---|
| 1 | Instantiates on a surround track with 7.1 output | Logic 12.3 on BlackHole 64ch + **session-recall stability**, which 2.1 could not check | No |
| 2 | Verify-ping reaches 8 distinct channels | Per-channel capture, measured by script | **Yes — D11.** Discrete/CoreAudio boundary closes; one hardware driver residual, owner none |
| 3 | `srcX`/`srcY`/`srcZ` + `w1..w8` visible **and writable** per-parameter in automation lanes | Logic automation lanes; 2.1 saw visibility only | No |

**Ledger at 4.2 close (if all pass): 30 complete · 0 partial · 0 pending — of 30.**

---

## Constraints

1. **The binary is frozen.** `fba35081`, both bundles matching on disk. Any code change (D16, D19)
   re-cuts the freeze and re-runs 4.1's gates. D19 touches no shipped behaviour but does touch a
   doc-comment in `Source/`, so it is a re-freeze too — plan it *before* the host session, not after.
2. **No hall, no physical 8-out interface.** D2 already retired the hall; D11 retires the interface
   for this pass. Spatial-coherence judgement is asked for by **no requirement row**.
3. **`-25208`.** Synthetic clicks are unavailable in this environment — Gate 13's interactive half is
   a human at a keyboard, which is why it is here and not in 4.1.
4. **Logic effects can never multi-out** (`critical_logic_only_named_surround_formats`) and Logic
   exposes only 10 named surround formats. 8 channels means 7.1 / 7.1-SDDS / 5.1.2. The plugin
   accepts all three; 2.1 observed plain `create7point1()`.
5. **One session.** D18 batches Logic, Gate 13 and Q5. Estimated 60–90 min including the widened LFE
   test.

---

## Open Questions — for the research phase

| # | Question | Why it blocks the plan |
|---|---|---|
| Q1 | Does Logic 12.3 offer a **7.1 output format** when the output device is BlackHole 64ch, and does it need an explicit I/O Assignment step? §6 rates per-channel custom I/O assignment **MEDIUM** — "unverified whether all 8 channels of 7.1 can be freely reassigned" | If Logic will not enter 7.1 on a virtual device, D11 collapses and the whole phase needs a different rig |
| Q2 | Does **Surround Bounce** work with BlackHole as the device? Bounce is offline and should be device-independent, but that is inference, not observation | D13 rests on it |
| Q3 | How is a non-identity label map applied and captured for **CR-b** — through the Venue screen UI, or a saved venue file? | Determines whether CR-b is a UI gesture inside the session or a fixture loaded before it |
| Q4 | Q5's protocol: how is the WKWebView **hidden** while the poll keeps running, and how is `diagnostics().dropped` read out of a hidden view? | `pattern_webview_completion_gated_on_isvisible` says completions are dropped when hidden — the readout mechanism must not itself require visibility |
| Q5 | What **HF-rich material** is used for the audible clause, and where does it come from? | D12's ecological half is only as good as its source material; "HF-rich" needs to be a named file, not an adjective |
| Q6 | Exact analysis contract for `tests/tools/analyse_bounce.py` — inputs, per-channel tone identification method, per-band decomposition for D15, and a `--check` mode matching `gen_dbap_reference.py` | D14's evidence standard is only re-derivable if the tool is specified before it is written |

---

## What 4.2 will NOT close — stated at the boundary, with owners

Per Stage 4's own rule that prose is not a third option, every item is a named deferral with an owner
or an explicit **owner: none**.

| Item | Owner | Blocked on |
|---|---|---|
| One specific hardware interface driver (`COMPAT-02`/2's "physical" half) | **none** | Hardware; and ungeneralisable across interfaces even if present |
| Windows **UI correctness** | **none** | Hardware. CI's ceiling is pluginval 10 opening the editor without timing out |
| RT-safety beyond allocation (locks, file I/O) | **none** (D10) | `-fsanitize=realtime` unsupported by Apple clang 17.0.0 |
| The two JS gates in CI | **none** | Headless-render determinism |
| Spatial coherence in a hall | **none** | No requirement row asks for it (D2) |
| `ARCHITECTURE.md`'s three intermediate checksums | **none, permanently** | Unreconstructible from git (4.1 verify Issue 2). Recorded once; not to be re-investigated |

---

## Next Phase

**Ready for:** research phase — Q1–Q6, against the frozen binary `fba35081` and Logic Pro 12.3.

Q1 is the gating question: if Logic will not negotiate 7.1 on BlackHole 64ch, D11 fails and the rig
decision reopens before anything else is planned.
