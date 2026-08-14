# Block C — operator runbook (Stage 4 · phase 4.2 · gates 12–25)

**Plugin:** O-Octagon · **Generated:** 2026-08-13 at the Block C boundary
**Runs against the freeze — not against whatever is on disk:**

| Item | Value |
|---|---|
| Commit | `378fb4cdc70ef7e7b4523771dd4f014f189246ec` |
| VST3 binary | `928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42` |
| AU binary | `cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99` |

**A checksum mismatch during the session is a REAL SIGNAL, not build nondeterminism** — the binary
was proven to rebuild byte-identically from source across three independent full builds.

---

## Pre-flight — measured at this boundary, all green

| Check | Result |
|---|---|
| Logic Pro | **12.3** ✅ |
| BlackHole 64ch | **present**, 64 in / 64 out ✅ |
| Installed VST3 binary vs freeze | **exact match** ✅ |
| Installed AU binary vs freeze | **exact match** ✅ |
| Installed variants on disk | `-dev` only — **no alternate variant to shadow the AU slot** ✅ |
| `selftest_analyse_bounce.py` | **24 cases, every clause seen to fire** ✅ |
| `cr-b-permuted.venue` fixture | present, `tests/fixtures/` ✅ |
| Session source material | **generated** — see below ✅ |
| `git status` | clean ✅ |

### Session material — already generated, outside the repo

```
~/Dev/octagon-4.2-session/sources/
    tone-1.wav … tone-8.wav      997 / 1499 / 2003 / 2503 / 3001 / 3499 / 4001 / 4507 Hz
                                 10.0 s, 24-bit mono PCM, each -20.00 dBFS peak
    lfe-multitone.wav            10 Schroeder-phased partials, crest 11.43 dB
    lfe-multitone.txt            sidecar
    audible-probe.wav            CU locator — 6 partials 6301–15101 Hz, 20.0 s
```

**Why outside the repo:** the gitignore rule is `plugins/*/.planning/evidence/**/*.wav`, which does
**not** match this phase's `stages/4-polish/evidence/` path. Bounces dropped into the stage evidence
directory would be committed. **Keep every WAV under `~/Dev/octagon-4.2-session/`.** Only the JSON
manifest and the text artifacts get committed.

### Working directory for every command below

```bash
cd /Users/taylorbrook/Dev/VST-development-octagon/plugins/O-Octagon
```

### The manifest every run appends to

```
.planning/stages/4-polish/evidence/bounce-manifest.json
```

That is `analyse_bounce.py`'s built-in `DEFAULT_MANIFEST` (`analyse_bounce.py:119-121`), so Gate 25's
bare `--check` finds it with no argument. **Pass `--emit-json` on every analysis run** — it is what
makes the recorded figures measured rather than transcribed.

---

## The five things to re-read before starting

1. **`airAmount = 0` on CR-a, CR-b, CT and CS — NEVER on CU.** An HF delta from `airAmount` reads
   exactly like bass management and would trigger D16's re-freeze on nothing.
2. **CS runs under the CR-a identity venue.** Under CR-b, speaker 4 is not the LFE slot.
3. **NC4 runs BEFORE any D16 disposition.**
4. **Gates 12 and 13 stop the phase.** They cost two minutes between them. Do them first.
5. **No `Source/` edit.** Anything needing one re-enters Block B with a second freeze.

---

## Gate 12 — Q2 surround bounce · **STOP-GATE**

Bounce ~4 s of anything through the plugin on a 7.1 surround track, 24-bit PCM WAV.

```bash
python3 tests/tools/analyse_bounce.py \
  --mode probe \
  --input ~/Dev/octagon-4.2-session/q2-probe.wav \
  --expect-channels 8 --expect-rate 48000 --expect-depth 24 \
  --emit-json .planning/stages/4-polish/evidence/bounce-manifest.json
```

**Pass:** exit 0, an 8-channel 24-bit PCM WAV that the tool can read.
**On failure: STOP.** Do not proceed to any other gate.

---

## Gate 13 — `getStatus` pre-flight · **STOP-GATE**

Open the plugin's diagnostics and record **before the first real bounce**:

- [ ] `mapInvalid == false`
- [ ] `numOutputChannels == 8`
- [ ] `safeMode == false`
- [ ] `outputSetName` = ________________________
- [ ] banner state **screenshotted** → `~/Dev/octagon-4.2-session/gate13-banner.png`

**Why this one stops the phase:** if Logic negotiates SDDS or 5.1.2 instead of 7.1, four default
labels are absent, `buildSpeakerToBuffer` fails, the **stale** map is retained — and the bounce still
produces eight channels of something. A green bounce does not clear this. Read it **before**, never
off a bounce. **On failure: STOP.**

---

## Gate 14 — `COMPAT-02` criterion 1

- [ ] Instantiates on a 7.1 surround track
- [ ] **Save → quit Logic entirely → reopen** — does the negotiated set survive session recall?

Stage 2.1 saw a fresh instantiation only. **The recall half has never been observed by anyone.**

---

## Gate 15 — `COMPAT-02` criterion 3

For **each** of `srcX`, `srcY`, `srcZ`, `w1`…`w8` — **11 lanes**:

- [ ] lane exists in the automation list
- [ ] a value can be **written** (latch on a moving playhead)
- [ ] it **reads back**

Visibility alone is what 2.1 had. Tick all three columns per lane, or the criterion is not closed.

---

## Gate 16 — CT · `COMPAT-02` criterion 2

The only assertion in 4.2 that turns criterion 2 from "the meters moved" into sample data.

**Rig:** BlackHole 64ch as **both** input and output. Eight mono tracks armed on inputs 1–8. Verify-
ping in auto cycle. Capture ≥ 12.8 s. **`airAmount = 0`.**

```bash
python3 tests/tools/analyse_bounce.py \
  --mode ping \
  --input ~/Dev/octagon-4.2-session/ct-ch1.wav ~/Dev/octagon-4.2-session/ct-ch2.wav \
          ~/Dev/octagon-4.2-session/ct-ch3.wav ~/Dev/octagon-4.2-session/ct-ch4.wav \
          ~/Dev/octagon-4.2-session/ct-ch5.wav ~/Dev/octagon-4.2-session/ct-ch6.wav \
          ~/Dev/octagon-4.2-session/ct-ch7.wav ~/Dev/octagon-4.2-session/ct-ch8.wav \
  --emit-json .planning/stages/4-polish/evidence/bounce-manifest.json
```

**Pass:** eight 1.6 s windows, exactly one channel energised per window above the floor, sequence
`1..8`. **Record the printed isolation margin.**

**Note N10 — CT cannot be bounced.** It is a realtime loopback capture by construction.
**Note D11 — this closes the criterion's routing half.** The "physical interface" half has
**owner: none** (no 8-out interface attached, and the result would not generalise across interfaces
even with one). **Do not re-word the criterion to fit the rig.**

---

## Gates 17 / 18 — CR-a then CR-b · the bounce-order pair

**Rig for both:** eight surround tracks, eight plugin instances, one-hot `w`, **`airAmount = 0`**,
**identical `srcX/Y/Z`** across all eight. Tone *k* → track *k* from
`~/Dev/octagon-4.2-session/sources/tone-k.wav`. Interleaved Surround Bounce, **24-bit PCM WAV**
(never 32-bit float — Python's `wave` rejects `0x0003`).

### Gate 17 — CR-a, shipped default venue

```bash
python3 tests/tools/analyse_bounce.py \
  --mode order --label CR-a --expect 1,2,3,4,5,6,7,8 \
  --input ~/Dev/octagon-4.2-session/cr-a.wav \
  --emit-json .planning/stages/4-polish/evidence/bounce-manifest.json
```

**Record Logic's canonical interleaved 7.1 bounce order.** This moves it from MEDIUM confidence to
measured.

### Gate 18 — CR-b, permuted venue

Load `tests/fixtures/cr-b-permuted.venue` into **all eight** instances. The 8-cycle is
`2,3,4,5,6,7,8,1` — a true cycle with **no fixed point**, so a venue that silently failed to load
cannot pass.

```bash
python3 tests/tools/analyse_bounce.py \
  --mode order --label CR-b --expect 2,3,4,5,6,7,8,1 \
  --input ~/Dev/octagon-4.2-session/cr-b.wav \
  --emit-json .planning/stages/4-polish/evidence/bounce-manifest.json
```

---

## Gate 19 — NC2 + NC3 · the controls that prove 17/18 can fail

**NC2** — re-run the **CR-b WAV** asserting the identity. The tool must **refuse**, not report a pass:

```bash
python3 tests/tools/analyse_bounce.py \
  --mode order --label CR-b --expect 1,2,3,4,5,6,7,8 \
  --input ~/Dev/octagon-4.2-session/cr-b.wav
```

**Expected: non-zero exit, a refusal** (P105 clause 2). If this *passes*, the whole CR-b check was
bypassed by the one mistake it exists to catch. **Do not `--emit-json` a control that is meant to fail.**

**NC3** — mute one track and re-bounce (or drop one tone). The tool must **fail**, not report 7-of-8
green (clause 3).

---

## Gate 20 — CS · the LFE test, both paths

**Under the CR-a identity venue** — under CR-b, speaker 4 is not the LFE slot.

**Rig:** two tracks, **`airAmount = 0`**, **identical `srcX/Y/Z`**, one-hot on speaker 4 (LFE) and
speaker 1 (reference). `sources/lfe-multitone.wav` into both.

Run **both** paths — the bounce, then **the same project captured through the realtime loopback**:

> ### ⚠️ `--channels 4,1` AS SPELLED IS WRONG — re-derive it per path
>
> This section was written before CT measured the device order. CT (Gate 16) found the
> **Emagic 7.1 device order** to be
>
> ```
> dev 1=L  2=R  3=Lrs  4=Rrs  5=C  6=Lfe  7=Lss  8=Rss
> ```
>
> so in a **device-order capture channel 4 is Rrs, and the LFE is at channel 6.** Run as
> originally spelled, CS would measure a rear surround against L, find no bass-management
> signature, and **report a confident wrong answer.** NC4 does not cover this door — it
> controls for an `airAmount` confound, not for a mis-addressed channel.
>
> **`--channels` MUST be re-derived per path.** The two paths need not agree: a Logic bounce
> writes channels per the **file's layout tag**, which need not equal the **device** order.
> That is exactly why CR-a is a separate gate.

```bash
# path 1 — bounce. --channels comes from CR-a's MEASURED result (Gate 17), not from here.
#   Substitute: <lfe-slot-in-the-BOUNCE>,<reference-slot-in-the-BOUNCE>
python3 tests/tools/analyse_bounce.py --mode lfe --channels <LFE>,<REF> \
  --input ~/Dev/octagon-4.2-session/cs-bounce.wav \
  --emit-json .planning/stages/4-polish/evidence/bounce-manifest.json

# path 2 — realtime loopback, nearly free on CT's rig.
#   Device order is already measured, so this one IS determined: LFE = dev 6, reference L = dev 1.
python3 tests/tools/analyse_bounce.py --mode lfe --channels 6,1 \
  --input ~/Dev/octagon-4.2-session/cs-loopback.wav \
  --emit-json .planning/stages/4-polish/evidence/bounce-manifest.json
```

**Do not pre-load the CT permutation into CR-a.** Run Gate 17 as spelled first and record what
comes back; fitting CR-a's `--expect` to data from a different path is the one mistake that would
make the bounce-order pair vacuous.

Compare per-partial and broadband against P103's verdict table. **The risk this catches:** the LFE
claim being true of the bounce and false of monitoring — which would ship a sentence about "an
ordinary speaker" that the monitor path contradicts.

**If D16 is invoked: STOP and re-enter Block B.** It is a second freeze, not a patch — and NC4 runs
first regardless.

---

## Gate 21 — NC4 · **run BEFORE any D16 disposition**

Set `airAmount` **non-zero** on one of the two sources and re-measure.

**Pass:** a per-band HF delta appears that is **not** bass management.

This is the control that stops a confound costing a re-freeze and eighteen re-run gates. An
`airAmount` HF delta is indistinguishable from bass management if you have not run it.

---

## Gate 22 — CU · the audible clause *(the one gate where `airAmount` is NOT zeroed)*

Two renders of the same project differing **only** in the gesture: one hull-crossing gesture, and a
genuinely **static** source position for the null. Same source file, same start time.

**Half 1 — the locator:** difference signal, soloed, on `sources/audible-probe.wav`.
*Is there a step, and where?* → ________________________
**Audibility here does NOT mean audibility in context.**

**Half 2 — the requirement:** the full bounce, in context, on a **named** ecological source — one
bright commercial or Apple Loop. Name it in `VERIFICATION-4.2.md`; **do not commit it** (licence).

- Ecological source named: ________________________
- **Headphones named:** ________________________

**Name the headphones.** D12's stated residual — a one-sample step of ~15 % of an 8 kHz component is
exactly what a cheap transducer hides, and an unnamed monitoring path makes the clause
unreproducible.

**Either outcome closes the clause** (D17). If it ticks: log it, **ship v1.0, open a v1.1 row.** The
lever is RESEARCH-2.3 H3 and it re-tunes the whole musical air curve — a discuss-boundary change, not
a fix, and explicitly a non-goal for this phase.

---

## Gate 23 — the interactive half + the relabelling

~15 min Standalone launch-and-drive:

- [ ] every control
- [ ] the Venue screen
- [ ] the Room screen
- [ ] preset load / store
- [ ] the banner

**The hidden-editor check:** minimise / ⌘H for 10 s with signal running, re-show — meters follow the
source again.

> **Record it as a throttling-recovery smoke check, with the sentence that it CANNOT drop a
> completion.**

**This relabelling is owed and no artifact yet carries it.** The false premise has been inherited
four times; an unrun gate is exactly how it gets inherited a fifth. Write the sentence, not a
footnote.

---

## Gate 24 — `User/` presets byte-identical

```bash
find ~/Library/O-Octagon/Presets/User -type f 2>/dev/null | sort | xargs shasum -a 256 2>/dev/null | shasum -a 256
```

**Desk half measured:** `e3b0c442…7852b855` — the empty-tree hash; `User/` does not exist.
**Pass:** the same hash after the session. Constraint 9 — `User/` presets are never written.

---

## Gate 25 — the analyser re-check

```bash
python3 tests/tools/analyse_bounce.py --check
```

**Pass:** exit 0 against the committed manifest, **after** the session. This re-derives every recorded
figure rather than reading it back.

> ### This gate could not pass as specified until 2026-08-14 — fixed at the desk, before the session
>
> `--emit-json` recorded each input as a **bare basename** (`analyse_bounce.py:915`), and `--check`
> resolved it **relative to the manifest** (`:805, :821`) — i.e. it looked for every WAV in
> `.planning/stages/4-polish/evidence/`. But this runbook mandates the opposite: **every WAV lives
> outside the repo**, because the gitignore rule `plugins/*/.planning/evidence/**/*.wav` does not
> match this phase's `stages/4-polish/` path and a WAV dropped next to the manifest would be
> committed.
>
> The two requirements were mutually exclusive, so **Gate 25 was unpassable by construction.** Proven
> at this boundary against the one already-recorded run: the bare `--check` exited **1** with
> `input does not exist: …/evidence/q2-probe.wav`.
>
> **Why it had to be fixed BEFORE the session, not at Gate 25:** every `--emit-json` during gates
> 17–22 would have baked in another unresolvable basename. Fixing it afterwards would mean
> hand-editing eight entries — which is exactly the "transcribed rather than measured" failure mode
> `--emit-json` exists to prevent, and the same weakness already conceded for CT.
>
> **The fix:** each run now records the directory it was read from (`input_dir`), and `--check`
> resolves against it; `--check --session-root DIR` overrides for a second person holding the WAVs
> elsewhere. Legacy manifest-relative resolution is the fallback, so the self-test is unaffected.
> **It changes only WHERE a file is looked for — no assertion was touched or loosened.**
>
> Verified at the desk:
>
> | Check | Result |
> |---|---|
> | `selftest_analyse_bounce.py` | ✅ 24 cases, every clause seen to fire |
> | Gate 12's probe re-derived (a re-measurement, not a hand edit) | ✅ 8 ch / 48000 Hz / 24-bit / 19.500 s — reproduced exactly |
> | bare `--check` | ✅ **exit 0** — `--check OK — 1 runs re-derived` |
> | **anti-vacuity control:** `--check --session-root /tmp/nonexistent-octagon` | ✅ **exit 1** — a missing artifact still fails |
>
> The control matters: without it, "Gate 25 now passes" would be indistinguishable from a gate that
> passes regardless of the artifacts.

---

## Close — Task 15

- [ ] `REQUIREMENTS.md`: `COMPAT-02` **3 of 3**, each criterion with **its probe letter and its
      measured figure**; criterion 2 carries D11's scope note and the owner-none residual
- [ ] `QUAL-01` criterion 2's audible clause **concluded**
- [ ] **Ledger 30 complete · 0 partial · 0 pending — of 30.** `openRows:` **empty**
- [ ] `lastVerified:` bumped — **missed twice already** (4.1 Issue 3, 4.2 Issue 3)
- [ ] `STATUS.md` frontmatter updated — **execute did not touch it last time** (4.2 Issue 2)
- [ ] `CHANGELOG.md`, `NOTES.md` (CR-a canonical order, the LFE measurement, P107's v1.1 doc row),
      `PLUGINS.md:68` — **only O-Octagon's row**, it is a shared registry
- [ ] **Count `[x]` against `→ **` per section** before closing
      (`pattern_evidence_line_orphaned_past_next_heading`)
- [ ] No evidence file named `*.log`
- [ ] `analyse_bounce.py --check` committed alongside the artifacts and green

**Already closed at this boundary:** Gate 9's spelling in `desk-gates-4.2.txt` — corrected, with both
forms re-measured first. `PLAN-4.2.md:605` deliberately left alone (P109).

---

## What this runbook does NOT claim

Nothing here has been run. Every gate above is **NOT RUN**, and no result from any of them is
asserted, implied, or inherited anywhere in this file. The pre-flight table at the top is the only
measured content, and every one of its rows was measured at this boundary.
