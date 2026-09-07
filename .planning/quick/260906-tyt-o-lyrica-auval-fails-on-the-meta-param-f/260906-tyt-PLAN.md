---
task: 260906-tyt
type: quick
mode: execute
autonomous: true
files_modified:
  - plugins/O-Lyrica/Source/PluginProcessor.cpp
  - plugins/O-Lyrica/CMakeLists.txt
  - plugins/O-Lyrica/CHANGELOG.md
  - plugins/O-Lyrica/NOTES.md
  - PLUGINS.md

estimate:
  tokens: 90000
  raw_tokens: 45000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "`auval -v aumu OLyr OuDv` prints the literal line `AU VALIDATION SUCCEEDED` against the freshly installed O-Lyrica-dev.component."
    - "That same transcript contains no failure marker anywhere (unanchored count is zero — auval writes them as `* * FAIL`, so an `^FAIL` anchor would be vacuous) and no `Meta Param Flag` block."
    - "The pre-fix auval run is on record in the SUMMARY, quoting the actual Meta Param Flag assertion block, so the post-fix pass is attributable to THIS change and not to a registry reshuffle."
    - "Both `freeToggle` and `scaleToggle` are declared as meta parameters, so a host is told each one moves the other."
    - "O-Lyrica ships at 2.5.1 in CMakeLists.txt, CHANGELOG.md and the PLUGINS.md row, all three agreeing."
  artifacts:
    - plugins/O-Lyrica/Source/PluginProcessor.cpp
    - plugins/O-Lyrica/CMakeLists.txt
    - plugins/O-Lyrica/CHANGELOG.md
    - plugins/O-Lyrica/NOTES.md
    - PLUGINS.md
  key_links:
    - "AudioParameterBoolAttributes meta flag -> the AU wrapper's global-meta parameter flag -> auval's parameter-stability sweep."
    - "`parameterChanged` (PluginProcessor.cpp:1031-1052) is the programmatic writer that makes the flag necessary; it is NOT changed by this task."
---

<objective>
Clear the `Meta Param Flag` assertion that has failed `auval` for O-Lyrica since v1.30.0 —
carried as "pre-existing and benign" in CHANGELOG.md:540 and NOTES.md:61 and still red at
v2.5.0 — by declaring `freeToggle` and `scaleToggle` as meta parameters, then proving the
fix with a before/after `auval` pair. Ship as 2.5.1.

Purpose: a red `auval` on a shipped plugin is a permanent blind spot. Every future release
of O-Lyrica has to hand-wave past a real assertion failure to call its AU gate green, which
means the gate cannot catch a NEW parameter-stability regression.

Output: a two-line source change, a version bump, a CHANGELOG entry, a corrected NOTES.md
claim, an updated PLUGINS.md row, and a recorded `AU VALIDATION SUCCEEDED`.
</objective>

<context>
@.planning/STATE.md
@CLAUDE.md
@plugins/O-Lyrica/Source/PluginProcessor.cpp
@plugins/O-Lyrica/CMakeLists.txt
@plugins/O-Lyrica/CHANGELOG.md
@plugins/O-Lyrica/NOTES.md

## Facts established at planning time — do NOT re-derive

**The failing parameter is `freeToggle`, proven by hash rather than assumed.**
The AU wrapper hashes the JUCE string param ID into the numeric ID auval prints
(`juce_audio_plugin_client_AU_1.mm:2441` `generateAUParameterID` -> `String::hashCode()` ->
Studio-One top-bit mask). `String::hashCode()` is `result = 31*result + c` over `uint32`
(`juce_String.cpp:562-577`; `multiplier = 31` for 4-byte types) — it is NOT the sdbm
variant that the same-shaped JUCE hash is often mistaken for. Computed over all 70 of
O-Lyrica's declared param IDs, exactly one collides with the reported ID:

| param ID | raw hash | masked (what auval prints) |
|----------|----------|----------------------------|
| `freeToggle` | 3423354080 | **1275870432** |
| `scaleToggle` | 3498643774 | 1351160126 |

So auval's `ParameterID:1275870432` is `freeToggle` — the *victim* whose value drifted when
auval moved `scaleToggle`. Each toggle writes the other, so BOTH need the flag.

**The declarations to change** are at PluginProcessor.cpp:127-138, inside
`createParameterLayout()`: two three-argument `juce::AudioParameterBool` constructions with
no attributes argument.

**The API is available and already used in this repo.** `AudioParameterBool` takes a
trailing `const AudioParameterBoolAttributes& attributes = {}`
(`juce_AudioParameterBool.h:74-77`); the meta setter lives on the shared attributes base
(`juce_RangedAudioParameter.h:75`). O-simplePhysicalModelSynth already fixed the identical
auval assertion this exact way — read
`plugins/O-simplePhysicalModelSynth/Source/PluginProcessor.cpp:118-126` for the working
call shape and its explanatory comment.

**Build/install facts.** `build/` is configured and `build/plugins/O-Lyrica` exists. The
`juce_add_plugin` target is `OLyrica` (the folder is `O-Lyrica`), so the build targets are
`OLyrica_VST3` and `OLyrica_AU`. `scripts/build-and-install.sh` resolves the target from
CMakeLists rather than from the folder name (`resolve_cmake_target_for`, lines 129-162 and
227-237), so NOTES.md:71-73's claim that the script fails for this plugin looks STALE —
but treat that as unconfirmed and keep the fallback in Task 3.

**Install/registry facts.** Local branding is dev: `OUARICON_DEV_SUFFIX "-dev"`,
`OUARICON_MANUFACTURER_CODE OuDv` (root CMakeLists.txt:29-31). Currently installed:
`~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` and
`~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3`; no unsuffixed variants are present, so
there is no shadowing pair to sweep — but run the sweep anyway, it is cheap.
`auval -a` already lists `aumu OLyr OuDv  -  Ouaricon Audio Development: O-Lyrica-dev`.

**Version pins.** `grep -rn '2\.5\.0' plugins/O-Lyrica` across txt/json/cmake/md finds ONLY
`plugins/O-Lyrica/CMakeLists.txt`. The one other pin is the root row `PLUGINS.md:38`:
`| O-Lyrica | 📦 Installed | 2.5.0 | Synth (Physical Modeling Harp) | 2026-09-06 |`.
Nothing under `plugins/O-Lyrica/ui` pins a version.

## Executor rules that are not optional

- **Long commands run in the background.** Builds and `auval` exceed the 600s watchdog, and
  a stalled agent's context cannot be resumed. Use `run_in_background: true` and poll for
  completion. A cold `auval` immediately after an install rescans the AU registry and can
  take ~15 minutes — tolerate that, do not kill it.
- **`auval -v aumu OLyr OuDv` takes three unquoted words** after `-v`. Do not quote them and
  do not join them; a quoted triple is not executable as spelled.
- **Commits are path-scoped:** `git commit -- plugins/O-Lyrica PLUGINS.md`. Never
  `git add -A`, never `git commit -a`. Re-run `git branch --show-current` and
  `git status --short` IMMEDIATELY before each commit — another session shares the index,
  and a session-start snapshot is minutes stale by commit time.
- **`.gsd/dispatch-isolation-sentinel.json` is modified in the working tree and is NOT ours.**
  Do not stage it. Confirm it is absent from `git diff --cached --name-only` before committing.
- **Do NOT create a tag.** Tags belong to `/publish` only.
- Every commit ends with the trailer:
  `Claude-Session: https://claude.ai/code/session_01VesWUWmc6iV9zFc3GFrPgn`
- Export a scratchpad path once and reuse it, e.g.
  `SCRATCH=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/20672289-e5cb-4f91-a8d6-6fb6362ed039/scratchpad`.
  Nothing in this task writes transcripts into the repo.
</context>

<tasks>

<task type="auto">
  <name>Task 1: Record the pre-fix auval assertion (negative control)</name>
  <files>(no repo files modified — writes only to $SCRATCH)</files>
  <precondition>`~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` exists and `auval -a` lists `aumu OLyr OuDv`. Both were true at planning time. If the bundle is absent there is nothing to take a baseline of — halt and report.</precondition>
  <action>
Validate the CURRENTLY INSTALLED v2.5.0 bundle before touching any source. This is the
negative control: without it, a later green run is not attributable to this change.

Run `auval -v aumu OLyr OuDv` in the background, redirecting the complete combined output to
`$SCRATCH/auval-before.txt`, then poll until the process exits. Capture the whole run — do
not pipe it through a filter, the transcript itself is the artifact.

Then extract three things and carry them into the SUMMARY:
1. The `Meta Param Flag` assertion block verbatim, including its `ParameterID:` line with
   the saved and current values.
2. The run's closing verdict line, and the count of lines containing a failure marker.
   Note the marker shape: auval prints results as `* * PASS` at line start and as indented
   `    PASS` for sub-results, so a failure line is `* * FAIL` — a gate anchored with
   `^FAIL` would never match and would be green by construction. Count with an unanchored
   `grep -cE 'FAIL'`.
3. Confirmation that the numeric parameter ID in that block is 1275870432. If it is a
   DIFFERENT number, re-derive which parameter it names before continuing:

   ```
   grep -oE 'juce::ParameterID \{ "[A-Za-z0-9_]+"' plugins/O-Lyrica/Source/PluginProcessor.cpp \
     | sed -E 's/.*"([^"]+)"/\1/' \
     | python3 -c 'import sys,functools
   h=lambda s: functools.reduce(lambda r,c:(31*r+ord(c))&0xFFFFFFFF, s, 0)
   for i in sys.stdin.read().split(): print(i, h(i)&0x7FFFFFFF)'
   ```

   A name other than one of the two toggles means the diagnosis in this plan is wrong —
   report it and halt rather than proceeding to Task 2.

**The discriminator is the presence of the Meta Param Flag block, not the verdict word.**
The sources disagree on severity: CHANGELOG.md:540 and NOTES.md:61 both call it a *warning*,
while the task brief calls it a failure. Resolve that from the transcript, do not assume:

- Block ABSENT entirely -> the premise is false. Stop, edit nothing, and report that the
  assertion no longer reproduces (a stale bundle, or a component rebuilt since the
  CHANGELOG note, would explain it).
- Block PRESENT and the run ends with a failed verdict -> as briefed; proceed.
- Block PRESENT but the run still ends `AU VALIDATION SUCCEEDED` -> it is a warning rather
  than a hard failure. Proceed anyway, and record the severity discrepancy in the SUMMARY as
  a false plan/brief prediction. The before/after discriminator is then the block's presence
  before and its absence after, which Task 3's gate already asserts.
  </action>
  <verify>
    <automated>test -s "$SCRATCH/auval-before.txt" &amp;&amp; grep -q 'Meta Param Flag' "$SCRATCH/auval-before.txt" &amp;&amp; grep -q '1275870432' "$SCRATCH/auval-before.txt"</automated>
  </verify>
  <done>A complete pre-fix auval transcript is on disk, it contains the Meta Param Flag assertion, and the numeric parameter ID in that assertion has been mapped to a named O-Lyrica parameter.</done>
</task>

<task type="auto">
  <name>Task 2: Declare both toggles as meta parameters and bump to 2.5.1</name>
  <files>plugins/O-Lyrica/Source/PluginProcessor.cpp, plugins/O-Lyrica/CMakeLists.txt, plugins/O-Lyrica/CHANGELOG.md, plugins/O-Lyrica/NOTES.md, PLUGINS.md</files>
  <action>
**Source (PluginProcessor.cpp:127-138).** Give BOTH `juce::AudioParameterBool`
constructions — `freeToggle` and `scaleToggle` — a fourth constructor argument:
`juce::AudioParameterBoolAttributes().withMeta (true)`. Copy the call shape from
`plugins/O-simplePhysicalModelSynth/Source/PluginProcessor.cpp:126`.

Add a brief comment above the pair explaining that each toggle programmatically writes the
other in `parameterChanged`, so the AU host has to be told they are global-meta parameters;
keep the existing `v1.30.0:` comment line. **The comment must NOT contain the literal token
that the code line uses for the attribute setter** — describe it in words ("the meta
attribute", "marked as global-meta") rather than repeating the identifier, because the
acceptance gate below counts real call sites and a comment echoing the identifier would
inflate that count and make the gate lie.

Do NOT touch `parameterChanged` (lines 1031-1052), the `isUpdatingToggles` reentrancy guard
(PluginProcessor.h:261), or the listener registrations (lines 589-590). The mutual exclusion
is intentional and stays exactly as it is; the declared default of both toggles stays `false`.
This change tells the host about existing behaviour, it does not alter behaviour.

**Version (CMakeLists.txt:12).** `VERSION "2.5.0"` -> `VERSION "2.5.1"`. This is the single
source of version truth. Confirm nothing else in the plugin pins the old number with
`grep -rn '2\.5\.0' plugins/O-Lyrica --include='*.txt' --include='*.cmake' --include='*.json' --include='*.md'`
— quote each `--include` pattern so zsh does not try to glob it (an unquoted `--include=*.txt`
fails with `no matches found`).

**CHANGELOG.md.** Insert `## [2.5.1] - 2026-09-06` directly after the file's three-line
header and above the existing `## [2.5.0]` entry, matching the surrounding heading style.
This is a PATCH: no parameter, range, default, type or state format changed — only a
host-facing attribute flag. Under `### Fixed`, state what the assertion was, that it dated
from v1.30.0, and that prior releases documented it as benign rather than fixing it. Quote
the assertion observed in Task 1 so the entry carries its own evidence.

**NOTES.md:61-63.** That bullet asserts the warning is pre-existing and benign. It is now
stale — rewrite it rather than deleting it: say it was resolved in 2.5.1 by marking both
toggles as meta parameters, and keep one line on why the mutual exclusion requires that, so
nobody re-derives it later. Separately, NOTES.md:71-73 claims `build-and-install.sh` fails
for this plugin with an unknown-target error; correct that line ONLY IF Task 3's build
actually succeeds through the script, and leave it untouched if you end up on the manual
fallback.

**PLUGINS.md:38.** Version column `2.5.0` -> `2.5.1`. Leave status, description and date as
they are — the date is already 2026-09-06.

Do not build and do not commit in this task.
  </action>
  <verify>
    <automated>test "$(grep -v '^[[:space:]]*//' plugins/O-Lyrica/Source/PluginProcessor.cpp | grep -c 'withMeta')" = 2 &amp;&amp; grep -q 'VERSION "2.5.1"' plugins/O-Lyrica/CMakeLists.txt &amp;&amp; ! grep -q '2\.5\.0' plugins/O-Lyrica/CMakeLists.txt &amp;&amp; grep -q '^## \[2\.5\.1\] - 2026-09-06' plugins/O-Lyrica/CHANGELOG.md &amp;&amp; grep -q '^| O-Lyrica |.*| 2\.5\.1 |' PLUGINS.md</automated>
  </verify>
  <done>Exactly two non-comment call sites in PluginProcessor.cpp set the meta attribute — one on each toggle; CMakeLists reads 2.5.1 and no longer mentions 2.5.0; a 2.5.1 CHANGELOG entry sits at the top of the file; the PLUGINS.md O-Lyrica row reads 2.5.1. No behavioural source line changed.</done>
</task>

<task type="auto">
  <name>Task 3: Rebuild, install, prove auval green, commit</name>
  <files>plugins/O-Lyrica/NOTES.md (conditional line-73 correction only), plus the path-scoped commit</files>
  <precondition>Task 2's edits are in the working tree and unbuilt. The `build/` directory is CMake-configured (`build/CMakeCache.txt` existed at planning time); if it does not, configure it before building rather than assuming a fresh tree.</precondition>
  <action>
**Build and install.** Preferred path: `./scripts/build-and-install.sh O-Lyrica`, run in the
background with output tee'd to `$SCRATCH/build.log`, polled to completion. That script
resolves the CMake target from CMakeLists, clears the AU caches, and performs the
dual-variant sweep (both the `-dev` and unsuffixed bundle names) before installing — which
is exactly the sequence CLAUDE.md requires.

CONDITIONAL FALLBACK, only if that script errors on target resolution (an unknown-target
message naming a hyphenated target): build `OLyrica_VST3` and `OLyrica_AU` directly in
`build/` in the background, then perform CLAUDE.md's manual sequence by hand — kill
`AudioComponentRegistrar`, remove both AudioUnit cache directories, remove BOTH the `-dev`
and unsuffixed `.vst3` and `.component` bundles from `~/Library/Audio/Plug-Ins/`, then copy
the freshly built `O-Lyrica*.vst3` and `O-Lyrica*.component` from
`build/plugins/O-Lyrica/OLyrica_artefacts/Release/` into place. Whichever path you take,
record which one in the SUMMARY — that determines whether NOTES.md:71-73 gets corrected in
Task 2's edit set.

Confirm the installed bundle actually carries the new version before validating; installing
a stale artefact and then reporting a green auval would be a false pass. Read the version
back out of the installed component's `Info.plist`
(`CFBundleShortVersionString` / `CFBundleVersion`) and check it reads 2.5.1.

**Validate.** Run `auval -v aumu OLyr OuDv` in the background — three unquoted words after
`-v` — with the complete combined output redirected to `$SCRATCH/auval-after.txt`. Poll
until it exits. The first run after an install rescans the AU registry and may take around
fifteen minutes; do not kill it and do not shorten it with a timeout.

The verdict is read from the transcript, not inferred: the literal line
`AU VALIDATION SUCCEEDED` must be present, an unanchored failure-marker count over the whole
transcript must be zero, and the `Meta Param Flag` block that Task 1 captured must be gone.
That third condition is the one that attributes the pass to this change — a green verdict
alone would not. **If any assertion is still red, the task is NOT
complete.** Report the actual transcript lines and stop — do not describe a red run as a
pass, and do not fall back to calling the remaining assertion benign, which is precisely
the failure mode this task exists to end.

**Commit.** Re-run `git branch --show-current` and `git status --short` immediately before
committing. Stage and commit path-scoped:
`git commit -- plugins/O-Lyrica PLUGINS.md`. Verify `.gsd/dispatch-isolation-sentinel.json`
is NOT in `git diff --cached --name-only` — it is another session's file. End the message
with the required `Claude-Session:` trailer. Do not create a tag.
  </action>
  <verify>
    <automated>grep -q '^AU VALIDATION SUCCEEDED' "$SCRATCH/auval-after.txt" &amp;&amp; test "$(grep -cE 'FAIL' "$SCRATCH/auval-after.txt")" = 0 &amp;&amp; ! grep -q 'Meta Param Flag' "$SCRATCH/auval-after.txt" &amp;&amp; test -z "$(git status --porcelain -- plugins/O-Lyrica PLUGINS.md)" &amp;&amp; git log -1 --format=%B | grep -q 'Claude-Session:'</automated>
  </verify>
  <done>The post-fix auval transcript contains `AU VALIDATION SUCCEEDED`, no failure lines and no Meta Param Flag assertion; the installed component reports 2.5.1; `plugins/O-Lyrica` and `PLUGINS.md` are clean in the working tree because their changes are committed, path-scoped, with the session trailer; no tag was created.</done>
</task>

</tasks>

<verification>
1. **Before/after pair, both on disk.** `$SCRATCH/auval-before.txt` shows the Meta Param
   Flag assertion on ParameterID 1275870432; `$SCRATCH/auval-after.txt` shows
   `AU VALIDATION SUCCEEDED` with zero failure lines. Both are quoted in the SUMMARY. A
   green after-run without a red before-run is not evidence and does not close this task.
2. **The change is attributable.** The only source edit between the two runs is the meta
   attribute on the two toggles. `git show --stat` on the commit confirms no other source
   file moved.
3. **Behaviour unchanged.** `git diff` over `PluginProcessor.cpp` touches only the two
   parameter declarations and their comment — not `parameterChanged`, not the guard flag,
   not the listener registrations, not the defaults.
4. **Version agreement.** CMakeLists.txt, the CHANGELOG top entry, the PLUGINS.md row and
   the installed component's Info.plist all read 2.5.1.
5. **Commit hygiene.** The commit's file list contains only paths under `plugins/O-Lyrica`
   and `PLUGINS.md`; `.gsd/dispatch-isolation-sentinel.json` is untouched and still
   modified in the working tree afterwards.
</verification>

<success_criteria>
- `auval -v aumu OLyr OuDv` prints `AU VALIDATION SUCCEEDED` and no failure lines, recorded
  verbatim in the SUMMARY alongside the exact command.
- The pre-fix assertion block is quoted in the SUMMARY as the negative control.
- Both toggles are declared as meta parameters; the mutual-exclusion logic is byte-identical.
- O-Lyrica is at 2.5.1 across CMakeLists.txt, CHANGELOG.md, PLUGINS.md and the installed bundle.
- NOTES.md no longer claims the assertion is a benign permanent fixture.
- One path-scoped commit with the session trailer. No tag.
</success_criteria>

<output>
Write the SUMMARY to
`.planning/quick/260906-tyt-o-lyrica-auval-fails-on-the-meta-param-f/260906-tyt-SUMMARY.md`.

It MUST contain, quoted rather than paraphrased:
- the exact validation command as run;
- the pre-fix Meta Param Flag assertion block, including the `ParameterID:` line with its
  saved and current values;
- the post-fix verdict line;
- which build path was used (script or manual fallback), and whether NOTES.md:71-73 was
  therefore corrected;
- any plan prediction that turned out false — including, if it happened, an auval run that
  stayed red, which must be reported as a failure rather than narrated as a partial success.
</output>
