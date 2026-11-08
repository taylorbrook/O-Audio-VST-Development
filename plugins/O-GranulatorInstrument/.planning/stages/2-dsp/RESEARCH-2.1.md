# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.1 — RESEARCH

**Researched:** 2026-11-07
**Scope:** Phase 2.1 only. The work is the slots, the MipTape, the voices, one stream (`s1`), the 32-grid pass engine, the pool, the fork step and the dev-loader editor.
**Confidence:** HIGH. Each claim is tagged **VERIFIED** (read this session from the repo at the given path or commit, or from `~/JUCE` 8.0.15) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.1.md` (D11–D13, defaults, gates, 5 open questions)
- `research/ARCHITECTURE.md` §2–§12, §18, Processing Chain, §A1/§A4, Memory, Gate Catalogue
- ROADMAP § Phase 2.1
- `parameter-spec.md` stream table
- the parent leaves at `82406fce`
- grain-engine 1.1.0: `BandlimitedReader.h`, `GrainPool.h`, `GrainScheduler.h`
- `O-simpleWavetable/Source/WtSynthesiser.h`
- `O-simpleGrain/Source/dsp/GrainSource.h` and its loader
- the `O-simpleSampler` FileChooser path
- `~/JUCE` `juce_FileChooser.cpp` and `juce_MidiBuffer.cpp`
- the parent harness allocation hook (`engine_gates.h`)
- this plugin's Stage 1 `Source/` and `CMakeLists.txt`

<user_constraints>
## User Constraints (CONTEXT-2.1, binding; not reopened)
- **D11:** fork pin `82406fce`, with the **full** §A1 leaf list copied at 2.1 start. Uncompiled leaves are covered by the drift check. Rename `ogran`→`ogi` and `OGRAN_TEST_HOOKS`→`OGI_TEST_HOOKS`. Ship the drift-check script with a negative control.
- **D12:** water-fill + onset thinning on the voice axis (≤ 16 demands × `s1`). Transitions use `obtain(progress)` with priority `4·stolenVoice + 2·overQuota + spawning + age·invLen`, victims go to 32 fade spares, a lowered cap converges, and the output law uses `N_eff = min(N,q)` with full windows and Hann only. 2.2 only widens this code.
- **D13:** `OGI_DEV_LOADER` (default ON until 3.1). `DevLoaderEditor` wraps `GenericAudioProcessorEditor` and adds 4 load buttons and per-slot status. It goes through the **same** message-thread load API that 3.2 will use. It is excluded from the harness and consoles. pluginval passes with it in place. HANDS-ON-2.1 exercises it.
- Defaults block (loader, MipTape, handoff, allocator, env, pitch, scan, scheduler, pool, mixer −6 dB provisional, thread rules) as written in CONTEXT-2.1.
</user_constraints>

## Project Constraints
- Trunk on `main`. Commits are path-scoped: `git commit -- plugins/O-GranulatorInstrument`. Re-check `git branch --show-current && git status --short` right before every commit. Foreign dirty paths are present right now and must not be swept in: `.claude/agent-memory/*`, `modules/registry.yaml` (O-Granulator owner), `.claude/compaction-snapshot.md`, and the GrainScatter `nielsen-dafx26.txt`.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument`. Run every long command in the background. Never tag.

---

## Summary (what the plan must absorb)

1. **The parent `waterFill` is hard-wired to exactly 4 demands.** It takes `const double (&)[kMaxStreams]` and the shares are `remaining / (kMaxStreams − i)` (`82406fce:…/dsp/Quotas.h:53-99`). It **cannot** take 16 demands, so it cannot take 80 either. **VERIFIED.** Fork `Quotas.h` VERBATIM (unused by the TU or used only by a parity test). Add a plugin-owned `VoiceQuotas.h` with `template <int N> waterFillN (const double (&)[N], int cap)`: the same algorithm, ties broken by index, with `N = 20` slots × 1 stream in 2.1 and `20 × 4` in 2.2. Gate it with `waterFillN<4> ≡ waterFill`, bitwise over a random demand corpus. Then 2.2 only changes the size of `N`, as D12 requires.
2. **Real-valued quota thinning does not by itself guarantee "never truncated".** A periodic grain train with gap `T` and length `L`, where `L/T = q` is not an integer, has a live count that alternates between ⌊q⌋ and ⌈q⌉. Across k voices with unrelated phases, the sum reaches `k·⌈q⌉`. Example: Q = 32 with 3 voices gives 33 > 32. The R3 cap kill then truncates grains in steady state. **INFERRED** (arithmetic). The fix (§1.3) is to thin against an **integer** quota `m_k = ⌊q_k⌋` and the **maximum** latched length, plus a 1-sample guard. Then `Σ live ≤ Σ m_k ≤ Q` holds exactly, and R3 kills become a transitions-only path that the gate can count as zero in steady state. `q_min = 32/16 = 2`, so `m_k ≥ 2` always.
3. **2.1 has no per-voice demand asymmetry.** Density and size are per-stream parameters, and modulation arrives in 2.5. Every sounding voice therefore has the **same** `N`. **VERIFIED** (spec stream table: no per-voice source in 2.1). "A dense voice cannot starve the others" can only be stimulated by *time* asymmetry: voice A fills the pool alone, then B arrives. The gate is: **B reaches `m_B` live grains within one grain length, A converges down to `m_A`, and the run with the `overQuota` term zeroed differs.** Default demand is `N = 10/s × 0.1 s = 1` per voice, so every cap gate needs density 200 and size 2000 ms (`N = 400`) to be non-vacuous (memory `pattern_gate_stimulus_below_threshold_is_vacuous`).
4. **Q3 (MIDI tie order): do not re-sort host MIDI.** JUCE `MidiBuffer::addEvent` inserts after the existing events with the same time (`findEventAfter … <= samplePosition`, `juce_MidiBuffer.cpp:85-91,153`), so iteration is already sorted by offset in host order. **VERIFIED.** Forcing "note-off before note-on at the same offset" turns a zero-length note (on, then off, same sample: emitted at loop seams and by step sequencers) into off-then-on. **That is a stuck note.** **INFERRED.** Iterate the `MidiBuffer` in place with a per-piece cursor: no copy, no 512 cap, no overflow, no dropped note-off. Clamp offsets to `[0, n−1]`.
5. **Q2 (MipTape reuse):** the public, reusable symbols are `blr::*` constants + `kHalfbandOdd` + `dot16/dot16Sum` + `besselI0`, `SincBank`/`sincBank()`/`setFor`, `latchLevel(rate, 4)` (**pass 4; the default is 3**), and the static `MipRing<4>::delayOf/margin/levelPos`. Only the private `MipRing::halfband()` must be **duplicated**: 8 lines, double accumulation, the same fold. **VERIFIED** (`BandlimitedReader.h:253-258,270-290,418-427`). Store level l **exactly as MipRing does**: index j = the half-band of parent indices `2j+1−126 … 2j+1`, with no delay baked into the index. Read through `MipRing<4>::levelPos`. The "writeRun ≡ MipRing" gate then compares arrays index for index.
6. **Q5:** no Kaiser offline resampler exists in the suite. O-simpleGrain, O-simpleSampler and O-TextureForge all use `juce::LagrangeInterpolator` (**VERIFIED**). Write it fresh. Use a 4096-phase double table (lerp) built with `blr::besselI0`. **Normalise every output sample's weights by their sum**, so the DC gain is exactly 1 (≈1e-15) at any ratio. Skip resampling entirely when the rates are equal, a bit-exact copy.
7. **Q4:** the suite precedent is O-simpleSampler and O-simpleGrain: a `std::unique_ptr<juce::FileChooser>` held as a member and re-made on each click, then `launchAsync`. **VERIFIED.** `FileChooser::~FileChooser` sets `asyncCallback = nullptr` (`juce_FileChooser.cpp:130-133`), and `finished()` calls it only if it is non-null (`:269-279`). **VERIFIED.** So an editor-owned chooser is safe by destruction order. Add a `SafePointer` in the lambda anyway.
8. **D11 reality check: about half of the leaves cannot be byte-verbatim**, because they hard-code parent sizes or param IDs. This changes what the manifest and the drift script must say (§2). `EngineConstants.h` has `kPoolLive 128`, `kPoolFade 16`, `kMaxSemis 24`, `kMaxRate 4`, `kMaxLevel 2`, `kModDests 113` and `kModSources 12`. `ControlGrid.h` holds the parent `ParamPointers`/`GridParams`. `ScaleQuantiser.h` and `ModMatrix.h` name `GranulatorParamIds`. `UiTelemetry.h` has `kMaxGrains 128`. **VERIFIED** (grep at `82406fce`). `Grain.h` lacks `voice/serial/tape`. Keep it VERBATIM and wrap it (§2.3).

---

## 1. Grain budget on the voice axis (Open Q1, D12)

### 1.1 Demand source — **VERIFIED**
The parent demand is `streamDemand (density, sizeMs, fs) = density · clamp(size,5,2000)·1e-3` (`OutputLaw.h:110-116`, a forked leaf, used as is). In 2.1:
- `N_k = streamDemand (s1_density, s1_size)` for every **counted** voice: gated, or releasing, which still spawns (spec: "grains keep spawning during the release").
- **Stolen voices** (the 4 steal spares in their 5 ms ramp) have **demand 0** and **spawn nothing**. Their live grains render under the ramp and are the first victims (`4·stolenVoice`). This keeps the demand count ≤ 16, as CONTEXT says. **INFERRED** (the only reading consistent with "≤ 16 demands").
- Index the demand array by **voice slot (20)**, not by note order, with 0 for empty or stolen slots. The water-fill sorts with ties broken by index, so a tie between equal demands resolves by slot. This is harmless because **equal demands get equal quotas whatever their order** (λ is symmetric). The random content still keys on the serial. **INFERRED.**

### 1.2 `waterFillN<N>` — shape
```cpp
template <int N> struct QuotasN { double q[N]{}; int m[N]{}; double lambda = 0; bool capped = false; };
template <int N> QuotasN<N> waterFillN (const double (&d)[N], int cap) noexcept;  // parent body, kMaxStreams → N
```
- Insertion sort: N = 20 means ≤ 190 compares per grid tick. N = 80 in 2.2 means ≤ 3160, about 0.1 µs/sample amortised over 32 samples. That is fine, but prefer `std::sort` on a fixed `int idx[N]` with a (demand, index) comparator in 2.2. **INFERRED.**
- The parent's comment says "zero demands sort first and subtract 0.0 exactly → single-demand λ == Q exactly". This holds for any N and gives a 1-voice equivalence row. **VERIFIED** (algorithm text, `Quotas.h:40-42`).
- Recompute only when an input changes: the voice set, `grain_limit`, or density/size. Store the result in the grid snapshot.

### 1.3 Thinning that actually bounds the live count
- Per counted voice k: `L_max = size·(1 + 0.5·sizeRand)·fs·1e-3` (the longest grain the spawner can latch; `size_rand` is 0 when unwired). Then `m_k = capped ? ⌊q_k⌋ : ∞`, and
  `T_k = max (fs/density, (L_max + 1) / m_k)`.
  Each grain lives ≤ L_max samples, and the onsets are ≥ L_max/m_k + 1/m_k apart. So at most m_k grains of voice k are live at any sample, and `Σ live ≤ Σ⌊q_k⌋ ≤ Q`. **INFERRED** (pigeonhole).
- The wasted capacity is < 1 grain per voice: at most 16 of 256, at worst 16 of 32 when 16 voices share cap 32. `q = 2` exactly there, so `m = 2` and none is wasted. **INFERRED.**
- **The law must use what thinning produces**, not `min(N,q)`. That means `N_eff,k = len̄ / T_k` (Hann μ₁/μ₂ full windows). Using `q` instead of `⌊q⌋` would misstate the power by `10·log10(q/⌊q⌋)` dB, about 0.28 dB at q = 10.67. Call the parent law as `streamPower (n_eff, n_eff, hannSel, …)`: with `n ≤ q` it takes the full-window branch `s1 = n·½, s2 = n·μ₂` (`OutputLaw.h:136-145`). Do **not** pass `(N, q)` with `N > q`: that is the parent's truncation model (`hannCumW(x)/x`), which is wrong under thinning. **VERIFIED** (code) / **INFERRED** (consequence).
- **Quota drops mid-gap:** `GrainScheduler` latches the gap at each onset (`GrainScheduler.h:44-47`). A quota cut therefore reaches the gap after the next onset, which takes up to 2 s at density 0.5. On a quota **decrease**, call `rescheduleNext (T_new)` (`:145-152`): "if that instant has already passed, the onset fires on the next sample". It is block-size invariant because quotas change only at absolute grid ticks. On an increase, let the latch roll. **VERIFIED** (API) / **INFERRED** (policy).
- The interval callback stays a pure function of (onset index, grid snapshot). This is the block-size-invariance contract at `GrainScheduler.h:93-97`. **VERIFIED.**

### 1.4 Acquire (transitions) — new code, not a fork
The parent `Spawner::acquire` (`Spawner.h:428-540`) is keyed on `stream` with `live[kMaxStreams]` and `pk[kMaxStreams]`. Spawner is **not** in the leaf list (ARCHITECTURE §10 calls it "rewritten"), so `GiSpawner::acquire` re-derives it over a `(voice)` key in 2.1 and `(voice, stream)` in 2.2. **VERIFIED.** Keep these parent mechanics exactly:
- **lexicographic** argmax on `(pri, age·invLen)`, never `pri + x` as a float sum, with ties to the lowest slot
- `progress' = (killing ? 3 : 0) + (pri − minPri) + age·invLen` for `GrainPool::obtain`
- the R3 path: at `alive ≥ limit` with a free slot, the victim gets a 5 ms `killStep` and is applied by `applyPendingKill` before its next sample
- D25 convergence: while `alive > cap`, each acquire also kill-fades one more over-quota victim

New: `pri = 4·stolenVoice(g.voice) + 2·(live_k > m_k) + (k == spawning)`. Use `m_k` (the thinning integer) as the over-quota bound. The parent uses `qCeil` (R8), and the real-valued `live > q` compare is its negative control (`Engine.h:216`, `Spawner.h:132`). With integer thinning, `m_k` is the exact bound. **INFERRED.**

### 1.5 D12 gates — concrete stimuli
| Gate | Stimulus | Pass | Control |
|---|---|---|---|
| Σ live ≤ cap | 16 notes, d 200, size 2000 ms, cap 32/64/128/256 | non-killing live ≤ cap at every tick; **0 R3 kills after the transition window** | thinning off (`m = ∞` hook) → R3 kills > 0 |
| no starvation | A alone for 3 s (fills cap), B on; cap 64 | B live ≥ m_B within L_max + 1 cell; A ≤ m_A after one L_max | `overQuota` term zeroed → B < m_B at the same tick |
| cap lowered live | 8 notes over-demand, 256 → 32 at t | alive ≤ 32 within L_max (+5 ms fades) | D25 hook off → alive stays > 32 |
| level under thinning | stationary noise, 1 / 4 / 16 voices, over-demand | per-voice RMS within ±0.5 dB of the law | old-vs-old row (same render twice) |

---

## 2. Fork step (D11)

### 2.1 Verified pin facts
- `git diff --stat 9866e37a 82406fce -- plugins/O-Granulator/Source/dsp` is **empty**, and `git diff --stat 82406fce -- Source/dsp` is also empty: no uncommitted parent DSP changes. **VERIFIED** this session.
- Fork from the blob: `git show "${PIN}:plugins/O-Granulator/Source/dsp/${f}"`. Brace the variables (zsh `:P`/`:h` modifier trap).

### 2.2 Leaf classes — what "zero drift" can honestly mean
| Leaf | Class | Why (at `82406fce`) |
|---|---|---|
| `GrainFx.h`, `WindowMorphBank.h`, `OutputLaw.h`, `FeedbackChain.h`, `TempoTracker.h`, `Grain.h`, `Quotas.h` | **VERBATIM** (rename only) | no parent sizes or IDs referenced (grep) |
| `EngineConstants.h` | **ADAPTED** | `kPoolLive 128→256`, `kPoolFade 16→32`, `kMaxSemis 24→48`, `kMaxRate 4→16`, `kMaxLevel 2→4`; mod counts (`kModDests 113`, `kModSources 12`) → spec values in 2.5. None of the VERBATIM leaves reads the changed pool, pitch or level constants (grep: 0 hits) |
| `ControlGrid.h` | **ADAPTED** | `CellLerp`, `ExactRamp` and `GainPair` (lines 57-165) are generic. `ParamPointers`/`GridParams` (169-487) are the parent's param layout. Keep the ramps and rewrite the param structs (2.1: the s1 subset + global) |
| `ScaleQuantiser.h` | **ADAPTED** (2.2) | names `GranulatorParamIds::scaleChoiceToId`, which needs a `GiParamIds` equivalent |
| `ModMatrix.h`, `ModSources.h` | **ADAPTED** (2.5) | `GranulatorParamIds` and `kModDests`/`kModSources` sizes |
| `UiTelemetry.h` | **ADAPTED** (2.6) | `kMaxGrains 128` → 288 |

**Drift check = two checks** (script `tests/fork-drift-check.sh`):
1. **Upstream drift**, for every file: sha256 of `git show PIN:path` vs sha256 of the parent at `HEAD:path` **and** in the working tree. If either differs, report DRIFT with the diff stat. This is what Stage 4's "fork drift review" consumes.
2. **Local fidelity**, VERBATIM files only: `git show PIN:path | sed -E 's/\bogran\b/ogi/g; s/OGRAN_TEST_HOOKS/OGI_TEST_HOOKS/g'` must equal the local file byte for byte. ADAPTED files list their edits in the manifest instead.
- Negative controls: (a) one byte flipped in a scratch copy of the parent file passed via `--parent-root` must fire check 1; (b) one byte flipped in a scratch copy of a local VERBATIM file must fire check 2. The script needs a `--parent-root` and a `--local-root` override so the controls never touch real trees.
- Manifest columns: `leaf | parent path | pin | sha256(pin blob) | sha256(local) | class | local edits | first-used phase`. Note "D43 (`stepAgc` d-arg) and D47a (R96 predicate) included at pin" on `FeedbackChain.h` and `ModSources.h`.
- Grep `OGRAN_TEST_HOOKS` across the leaves: 29 hits in the parent `Source/dsp/*.h` (**VERIFIED**). The CMake test target must define `OGI_TEST_HOOKS=1`, which already exists on the harness target (`CMakeLists.txt:210-214`).
- Include closure: `EngineConstants.h` includes `"BandlimitedReader.h"`; `WindowMorphBank.h` → `"WindowLut.h"`; `ScaleQuantiser.h`/`ModSources.h` → `"CounterRng.h"`, `"PitchCollections.h"`. All are on the include path via `ouaricon_add_module` (grain-engine, pitch-collections, at `CMakeLists.txt:75-78`). `ControlGrid.h` pulls `juce_audio_basics` (Decibels) and `ScaleQuantiser.h` pulls `juce_core` (SpinLock). **VERIFIED.** Uncompiled leaves are never included, so ADAPTED-later files can sit unedited until their phase.

### 2.3 Grain layout: wrap, don't edit
`ogi::Grain` (VERBATIM) is the FX/window POD. 2.1 uses `struct VoiceGrain { Grain g; int voice; std::uint64_t serial; const SlotTape* tape; int slot; };` with `static_assert (std::is_trivially_copyable_v<VoiceGrain>)`, and the pool is `GrainPool<VoiceGrain, 256, 32>`. `GrainFx` functions take `Grain&`, so pass `vg.g`. This keeps `Grain.h` VERBATIM forever. **INFERRED.** `GrainPool` value-initialises a slot on activate and copies the grain into fade slots (`GrainPool.h:236-250`), so a POD wrapper works. Note that `Grain::level`'s comment says `latchLevel (rate, 2)`. The instrument calls `latchLevel (rate, 4)` and the comment stays as the parent wrote it. **VERIFIED.**

---

## 3. MipTape<4> (Open Q2)

### 3.1 Reuse vs duplicate — **VERIFIED** (`BandlimitedReader.h`)
| Symbol | Access | Use |
|---|---|---|
| `blr::kTaps/kTapsBack/kTapsAhead/kHbDelay/kHbLength/kHbHistory/kHistCentreOffset`, `kHalfbandOdd[32]`, `kSincSets` | public `inline constexpr` | direct |
| `blr::dot16`, `dot16Sum`, `besselI0` | public inline | direct (besselI0 also serves the resampler, §5) |
| `SincBank`, `sincBank()`, `SincBank::setFor` | public | direct; call `sincBank()` once off-thread in `MipTape::prepare` |
| `latchLevel (rate, maxLevel)` | public | **`latchLevel (rate, 4)`**: default arg is 3 |
| `MipRing<4>::delayOf / margin / levelPos / levelIndex` | public **static** constexpr | direct, no ring instance needed |
| `MipRing::halfband` | **private** static | **duplicate** (8 lines; keep `double acc`, `0.5·c[0]` + 32 folded odd taps) |
| `MipRing::locateStart` | private | rewrite positionally (MipSnapshot's `locateStart` at :543 is the closer model: `levelPos (level, origin + ⌊pos⌋, pos − ⌊pos⌋)`) |

### 3.2 Layout and writer
- **Storage convention = MipRing's.** Level-l index j holds the half-band output when the level-(l−1) index `2j+1` arrives. This is the cascade in `push()` (`:322-344`): the child is taken on odd parent indices only. The group delay is applied **at read time** by `levelPos` (`q = t + D(l) − (2^l−1)`). The ARCHITECTURE §7 phrase "update level l at index (p − 63·(2^l−1))/2^l" bakes the delay into the index. **Don't**: it breaks index-for-index equality with MipRing and doubles the compensation if you also read via `levelPos`. **INFERRED.**
- **History before 0 = zeros** (MipRing's cleared `hist`). This gives bitwise MipRing equality and the physically correct band-limited onset. O-simpleGrain pads with held edge samples instead (`GrainSource.h:36-41`, kPreRoll 16384), but a positional tape that the record head overwrites in 2.4 cannot hold edges consistently. **INFERRED.**
- **Flush:** to settle the tail, the writer runs the cascade for `MipRing<4>::margin(4) = 945 + 129 = 1074` zero frames past `F` (it stores them; reads never go past F). Level-l size = `((F + 1074) >> l) + 2·kTaps + 4`. ARCHITECTURE's `ceil(F/2^l)` is **too small**: a pitched read in the last ~1 k frames would index past the stored level. **VERIFIED** (margin arithmetic, `:270-273`) / **INFERRED** (consequence).
- `writeRun (pos, n, L, R)`: writes level 0 at `[pos, pos+n)`. Then for each level l = 1..4 it computes every child index j whose parent index `2j+1` lies inside the newly written parent span. The halfband history for that is read **from the tape's own level-(l−1) array** (indices < 0 → 0). In 2.1 the only caller is the loader's sequential build. 2.4's record head reuses it.
- **Gate (writeRun ≡ MipRing):** feed the same stereo noise into `MipRing<4>` (capacity ≥ F + 1074, no wrap) via `push`, and into `MipTape` via `writeRun` in random run lengths (1…4096). For every level l and every j with `2^l·j + delayOf(l) + 2^l < F`, `ring.levelSample(l,ch,j) == tape.level(l,ch,j)` bitwise. Add a `readStereo` equality row at 1 k random (level, pos, set) points, via a MipSnapshot captured from the same ring (positional ↔ positional). The control is one tap of the duplicated `halfband` perturbed by 1 ulp, which must fail.
- `readStereo (level, pos, set, l, r)`: `pos` is a double in [0, F). `p = levelPos (level, (int64)⌊pos⌋, pos − ⌊pos⌋)`, kernel `bank->kernel (set, p.frac)`, start `p.index − kTapsBack`, clamped to `[0, size − kTaps]` (MipSnapshot rule). Reads stay well inside because of the region fit and the flush, so the clamp is a guard, not mirroring. **INFERRED.**
- **Aliasing during an SR-mismatch window:** while an old tape is read at `tapeRate/fs ≠ 1`, the rate can exceed 16. For example, 96 k→44.1 k gives ×2.18, so +48 st becomes rate 34.8, and level 4 takes residual 2.18 → **S7** (best effort, ≈ −20 dB at 3.99; `:124-126`). This is transient (until the rebuild lands) and acceptable. The DSP-07 gate measures after the rebuild. **VERIFIED** (set table) / **INFERRED** (impact).

### 3.3 DSP-07 stimulus (non-vacuous)
- 96 k session, 44.1 k source resampled. +48 st = ×16: a 4 kHz sine → 64 kHz, which folds to 32 kHz. Measure the 32 kHz bin ≤ −60 dB relative to the 4 kHz-at-+0 reference level. A positive row checks that 2.5 kHz → 40 kHz is **present** (in band). This proves the measurement can see content up there.
- +24 st = ×4: a 15 kHz source (below the resampler cutoff, 0.95·22.05 k = 20.9 k) → 60 kHz, which folds to 36 kHz. Measure the 36 k bin ≤ −60 dB.
- Pitch probe (FUNC-03 ±1 ¢): **not** on a dense cloud. At scan 0, every grain starts at the same `c`, so the output is a periodic train of identical windowed fragments. Its autocorrelation peak sits at the onset period, not at the sine. Use (a) a hook that reads the latched `rate` and checks `== exp2(st/12)·tapeRate/fs` to 1e-12, and (b) audio with **one long grain** (size 2000 ms, density 0.5), with autocorrelation over the window's central 50 %. Do it at root, at root ± 12 and with keyTrack 0 across 3 keys. **INFERRED** (memory `O-simplePhysicalModelSynth autocorr lesson` + window geometry).

---

## 4. MIDI front end (Open Q3)

- **VERIFIED:** `MidiBuffer` stays sorted by time, and same-time events keep insertion order (`findEventAfter` skips while `time <= samplePosition`, then inserts; `juce_MidiBuffer.cpp:85-91,153`). The JUCE VST3/AU wrappers fill it in host order.
- **Recommendation:** no copy into a fixed list. Each 32-grid piece `[a,b)` walks a `MidiBufferIterator` cursor while `pos < b`, applying events at `clamp(pos, 0, n−1)`. The iteration is allocation-free. This removes the 512 cap and the "overflow drops latest" policy. That policy is dangerous because a dropped **note-off** is a stuck voice. The overflow-telemetry question then disappears: there is nothing to count. **INFERRED.**
- **Do not reorder same-offset events.** "Off before on" breaks zero-length notes (on, then off, same sample → off finds no voice, then on starts a voice with no off). Host order is deterministic, so the block-size null is unaffected. Harness rows:
  - same-offset on→off(60) → the voice starts and enters release, with no stuck note
  - same-offset off→on(60) with 60 already sounding → retrigger (same-pitch steal path)
  - 600 events in one block → all applied, 0 stuck
- **Engine-generated** events at the same sample (CC64 is 2.5; the `polyphony`-lowered force-release is applied at the grid tick) run **before** that sample's host events. This is the only place an explicit tie rule is needed. **INFERRED.**
- Voice start mid-piece: the voice's `GrainScheduler.reset()` happens at the event and `process()` runs only over `[o, b)`, with onset offsets shifted by `o`. The env is 0 for `[a, o)`. Since reset fires "on the next processed sample at frac 0" (`GrainScheduler.h:84-91`), the first onset lands exactly at the note-on sample. **VERIFIED** (API).

## 5. Voice allocator port — **VERIFIED** (`WtSynthesiser.h:33-115`)
WtSynthesiser is a `juce::Synthesiser` subclass, so port **only the policy order** over the fixed array:
1. Sort the stealable voices oldest-first by note serial.
2. Protect `low` and `top`: the lowest and highest **non-released** notes. If they are the same voice, protect only `low`.
3. Pick the first match in this order:
   1. the oldest voice already playing the target pitch
   2. the oldest released voice
   3. the oldest voice with no key down
   4. the oldest unprotected voice
   5. `top`
   6. `low`

Our "voice" = a counted voice. Stolen voices are already out of the candidate set.

Gate trap: rule 3.1 means a steal test that replays a pitch already sounding steals that pitch's voice, so pitch changes go untested (memory `pattern_voice_steal_returns_same_pitch_hides_string_change`). **The FUNC-01 chord-overflow gate must use distinct pitches for every note**, and assert which serial was stolen.

## 6. Dev-loader editor (Open Q4, D13)
- **VERIFIED** precedent: `O-simpleSampler/Source/PluginProcessor.cpp:749-770` (also O-simpleGrain `:964-978`). It re-makes `fileChooser = std::make_unique<juce::FileChooser>(…)` on each click, then `launchAsync (openMode|canSelectFiles, …)`, and the lambda early-outs on an empty result.
- **VERIFIED** lifetime: `~FileChooser` nulls `asyncCallback`, and `finished()` calls it only if non-null. So a `std::unique_ptr<FileChooser>` member of `DevLoaderEditor` dies with the editor, and the callback can never run on a dead editor. `launchAsync` jasserts if one chooser is launched twice (`:196`). Re-making it per click (with the previous dialog destroyed and its callback nulled) avoids that. Keep **one** chooser for all 4 buttons, plus `int pendingSlot`.
- Add `[safe = juce::Component::SafePointer<DevLoaderEditor>(this), slot]` and return early if `safe == nullptr`, as belt and braces. The callback then calls `proc.requestSlotLoad (slot, file)`. That is a message-thread API shared with 3.2, and it enqueues the loader job.
- Status lines: a 10 Hz `juce::Timer` in the editor reads `proc.getSlotInfo (s)`, which returns a copy of `SlotInfo` taken under its `CriticalSection`. **Do not** have the processor call into the editor.
- pluginval's editor tests open and close the editor but never click, so the UAF path is **gate-blind**. Cover it by code review plus a HANDS-ON-2.1 row: open the chooser, close the plugin window, then pick a file, and confirm no crash and no load.
- TU exclusion: `#if OGI_DEV_LOADER && JUCE_WEB_BROWSER`, or a dedicated `OGI_DEV_LOADER` define absent from the harness targets. The source file is listed only in the plugin target's `target_sources`.

## 7. Slot loader (Open Q5 + §5)
- **Kaiser sinc, fresh code:** half-width = 32 zero-crossings of the cutoff `fc = 0.95·min(fsIn,fsOut)/2`. That is `32·fsIn/(2fc)` input taps per side: ≈ 34 for 44.1→96 k, ≈ 73 for 96→44.1 k.
  - Table: 4096 phases × taps (double), β = 9, built once per job.
  - Per output sample: lerp two phase rows, accumulate in double, divide by the sum of the weights.
  - Cost for 30 s at 96 k stereo is ≈ 2.9 M × 70 × 2 ≈ 0.4 G MAC, < 1 s on the worker. **INFERRED.**
- **Gates:** DC (a constant 0.5 in) → |out − 0.5| ≤ 1e-6 over the interior; passband ripple ≤ 0.01 dB to 0.9·min/2; stopband ≥ 90 dB (β 9 sidelobes); equal rates → bit-exact copy.
- **Edges:** treat out-of-range input as zero, so the tape starts and ends with the file's own band-limited onset and release. **INFERRED.**
- **Trim:** cap at `min(30 s·fsIn native frames, 2.88 M session frames ↔ native)`. At 192 k the frame cap (15 s) binds before the 30 s rule. Set `trimmed` from either limit, and check that the 192 k row reports trimmed at 15 s.
- `ThreadPool` with 1 worker. A job checks `shouldExit()` between decode, resample and tape-build chunks (every 64 k frames). A newer (slot, generation) calls `removeJob (old, true, 0)`, which interrupts without waiting. `AudioFormatManager::registerBasicFormats()` lives on the processor (it is created in the ctor; reader creation happens on the worker).

## 8. Handoff / tape lifetime — risks the plan must gate
- **Kill-on-swap reference count:** a retiring tape may be pushed to the retire ring only once **no live or fade grain** holds `vg.tape == old`. That includes the 32 fade spares (`GrainPool` copies the whole POD into a fade slot, so the tape pointer goes with it). Scan all 288 slots at the grid tick. **INFERRED.**
- Host-idle gate: 100 `requestSlotLoad` + 0 `processBlock` → `pending[s]` ≤ 1 and every superseded tape deleted on the message thread. The loader's own job output publishes, so pump the message loop in the harness (`MessageManager::runDispatchLoopUntil`). **INFERRED.**

## 9. Determinism / seed control
**Every 2.1 random draw the context lists is out of scope** (jitter, spray, reverse, spread, gain_rand and pitch_rand with the quantiser all arrive in 2.2). If 2.1 makes no draw, the block-size null's "different-seed control differs" is **vacuous**: both seeds render identically. **INFERRED** (CONTEXT out-of-scope list). Recommend: wire **`s1_size_rand`** in 2.1. It is a single draw on a fixed CounterRng slot keyed `(serial << 24) | onsetIndex`, with no dependency, and it is already in `L_max` (§1.3). The harness sets it to 50 % for the null and the seed control. The alternative is to accept a "different note-on offset differs" control. That proves less, because it does not exercise the serial-keyed RNG that the steal-invariance gate depends on.

Steal invariance row: the same MIDI script rendered with polyphony 16, and again with prior notes forcing different slot assignment. Each note's grains must be identical (compare per-serial grain logs via a hook), even though the slot indices differ.

## 10. Allocation gate
Reuse the parent harness hook (`O-Granulator/tests/render-harness/engine_gates.h:65-102,854-870`):
- `malloc_logger` with `volatile` flag and counter
- scoped by `pthread_equal(self, audioThread)`
- a liveness probe (`malloc(64)` while armed must count 1)
- `backtrace_symbols_fd` on the first hit

Rows:
- note on/off
- a 20-note chord at polyphony 8 (steals)
- file swap mid-note
- SR change mid-note
- `grain_limit` 256→32 live

Make sure `waterFillN` and the merged onset sort use fixed arrays (no `std::vector` in the per-piece path). **VERIFIED** (hook source).

---

## Pitfalls checklist (for PLAN must-haves)
| # | Pitfall | Guard |
|---|---|---|
| P1 | `waterFill` hard-wired to 4 | `waterFillN<N>` + `≡ waterFill` parity |
| P2 | real-q thinning still truncates | integer `m_k`, `L_max`, +1 guard; "0 R3 kills steady state" row |
| P3 | starvation gate vacuous in 2.1 (equal demands) | time-asymmetric stimulus + `overQuota`-off control |
| P4 | default N = 1 never caps | d 200, size 2000 ms in every cap gate |
| P5 | off-before-on re-sort → stuck zero-length note | iterate MidiBuffer in place; zero-length rows |
| P6 | `latchLevel` default maxLevel 3 | always pass 4; ±48 st row reads level 4 |
| P7 | delay baked into MipTape index | MipRing storage convention + `levelPos` at read |
| P8 | level arrays sized `F/2^l` | `(F + margin(4)) >> l` + flush |
| P9 | same-pitch steal hides the steal test | distinct pitches; assert the stolen serial |
| P10 | seed control vacuous with no draws | wire `s1_size_rand` in 2.1 |
| P11 | dense-cloud autocorr locks to the onset period | rate hook + single-long-grain probe |
| P12 | retire while a **fade spare** still holds the tape | scan 288 slots, not 256 |
| P13 | "verbatim" leaves that cannot be | VERBATIM/ADAPTED classes; two-part drift check |
| P14 | async chooser UAF is pluginval-blind | dtor nulls the callback + SafePointer + HANDS-ON row |
| P15 | 2.1 ignores spray 10 % / jitter 20 % / spread 30 % defaults | goldens re-baseline in 2.2 is **expected**; say so in SUMMARY so 2.2 does not read it as a regression |

## Open items for the planner (no user decision needed unless flagged)
- **[flag]** P2/§1.3 integer thinning refines D12's "thinned to its water-filled quota". It keeps D12's intent ("never truncated") and its gates. If the user prefers the literal real-q reading, the "0 R3 kills" row must be dropped.
- **[flag]** P10 adds `s1_size_rand` to 2.1's read set, which moves it out of the "unread params" control row.
- Everything else is implementation detail within the locked decisions.
