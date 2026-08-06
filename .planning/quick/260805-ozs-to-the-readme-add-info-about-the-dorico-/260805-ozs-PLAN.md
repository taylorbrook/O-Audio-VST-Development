---
phase: quick-260805-ozs
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - README.md
autonomous: true
requirements: [QUICK-260805-ozs]

must_haves:
  truths:
    - "A reader of the public README learns that the Ouaricon suite plays microtonal scores from Dorico via VST3 Note Expression, which plugins support it, and how to activate it."
    - "A reader learns that O-MicrotonalSampler and O-Contrabass additionally ship Dorico playback templates and expression maps, and where the install instructions live."
    - "Every in-repo path cited in the new Dorico section resolves to a file that exists on disk."
    - "Every plugin in the README Plugins tables that has a live oaudio.io product page links to it; plugins without a page stay plain text."
    - "Every oaudio.io URL added to the README is present in the site's own sitemap.xml (no fabricated links)."
    - "The plugin tables still list exactly 39 plugins — no rows added, removed, or renamed."
  artifacts:
    - README.md
  key_links:
    - "README Dorico section -> research/microtonal-dorico-integration.md (developer reference)"
    - "README Dorico section -> modules/tuning/note-expression/README.md (module docs)"
    - "README Dorico section -> plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md and plugins/O-Contrabass/Resources/dorico/INSTALL-DORICO.md"
    - "README plugin table rows -> https://oaudio.io/products/<slug> product pages"
---

<objective>
Add a grounded, top-level Dorico microtonal integration section to the repo root README, and link every plugin in the Plugins tables to its oaudio.io product page where one exists.

Purpose: The README is the public front door of the O-Audio-VST-Development repo. Dorico microtonal playback is the suite's most distinctive capability and is currently buried as a subsection under "Modern Interface Design" (a WebView/UI section it has nothing to do with), is missing three cohort plugins, and says nothing about the expression maps, playback templates, or keyswitch routing that two plugins ship. Separately, readers have no path from the catalog table to the product pages that already exist on oaudio.io.

Output: A single modified README.md, landed as two atomic commits.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@README.md

Ground-truth sources for the Dorico content (read the ones you cite; do not paraphrase from memory):
@research/microtonal-dorico-integration.md
@modules/tuning/note-expression/README.md
@plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md

All commands and paths below are relative to the repo root: /Users/taylorbrook/Dev/VST-development
</context>

<verified_facts>
These were verified against the working tree and the live site during planning on 2026-08-05. Treat them as established; re-verify only the two items explicitly marked CONFIRM.

**Note Expression module**
- Module lives at `modules/tuning/note-expression`, version **1.1.1** per `modules/registry.yaml`. Header-only, namespace `Ouaricon::NoteExpression`.
- It owns the Note Expression Controller (`kTuningTypeID`), drains raw NE events from a patched JUCE wrapper, and applies per-note semitone offsets at the voice call site. It composes with the `scala-tuning-engine` module.
- The JUCE patch on disk is `scripts/juce-patches/note-expression-juce-8.0.14.patch` (an older `-8.0.9.patch` also sits in that directory; 8.0.14 is the current pin — see README Requirements section).
- Steinberg symbols are VST3-only: the capability requires the **VST3** build of a plugin. Dorico loads VST3 on both macOS and Windows.

**Cohort — `note-expression` `used_by` in `modules/registry.yaml` lists ELEVEN plugins (the README currently names only eight):**
O-Bassoon, O-Bells, O-Bowed, O-Contrabass, O-Formant, O-IntonationPad, O-Lyrica, O-MicrotonalSampler, O-Prism, O-Reed, O-Wind.

**Activation flow (per-note tuning)**
- The canonical library file is `Ouaricon-VST3-NoteExpression.doricolib`, authored at `modules/tuning/note-expression/resources/library/`.
- Installers bundle it: PKG on macOS, EXE on Windows. The user imports it once via Dorico -> Library -> Library Manager -> Import; tuned pitches then play back automatically.

**Playback templates / expression maps — only TWO plugins ship these bundles:**
- `plugins/O-MicrotonalSampler/Resources/dorico/` — `INSTALL-DORICO.md`, `SMOKE-TEST.md`, `PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml`, and four `EndpointConfigs/` folders (`O-MicrotonalSampler`, `-Winds`, `-Brass`, `-Generic`) plus a shared `playbacktemplatedeps.doricolib` carrying 4 expression maps.
- `plugins/O-Contrabass/Resources/dorico/` — same shape, single endpoint config.
- Per `INSTALL-DORICO.md`: the O-MicrotonalSampler template covers **four instrument families** (Strings, Winds, Brass, Generic fallback) in one Playback Template, and Dorico routes each stave to the matching family map by instrument family. Standalone `.doricoexpmap` files are **not** auto-ingested by Dorico's library scanner — the validated path is the multi-folder layout plus `DefaultLibraryAdditions/`.

**O-MicrotonalSampler specifics (CONFIRM both against `plugins/O-MicrotonalSampler/CHANGELOG.md` before writing them; drop the sentence rather than guess if the CHANGELOG does not support it):**
- Keyswitches are **opt-in** — `ks_enabled` defaults to `false` on fresh instances (introduced in a v1.23.x patch release; read the CHANGELOG heading for the exact version, or omit the version number).
- A `dynamics_mode` parameter offers `Velocity` or `CC Crossfade`, with a Dynamic Range control for the crossfade mode.

**Developer reference** — `research/microtonal-dorico-integration.md` (554 lines). Its H2 sections are: Module Architecture, Canonical Dorico Setup Procedure, Host-Side Behavior Quirks, Troubleshooting Signatures.

**Slash command** — `/dorico [Name] [question?]` already appears in the README "Research & Troubleshooting" command table. Reference it; do not duplicate the table row.

**oaudio.io product pages — the site's `sitemap.xml` lists exactly 16 `/products/<slug>` pages. Each slug below was fetched during planning and its page body was confirmed to name the mapped repo plugin:**

| README plugin | Product page URL |
|---|---|
| `O-Bells` | https://oaudio.io/products/ouaricon-bells |
| `O-Lyrica` | https://oaudio.io/products/ouaricon-lyrica |
| `O-Prism` | https://oaudio.io/products/ouaricon-prism |
| `O-IntonationPad` | https://oaudio.io/products/ouaricon-intonation-pad |
| `O-SimpleReverb` | https://oaudio.io/products/ouaricon-simple-reverb |
| `O-DigiDelay` | https://oaudio.io/products/ouaricon-delay |
| `O-Chorus` | https://oaudio.io/products/ouaricon-chorus |
| `O-Tremolo` | https://oaudio.io/products/ouaricon-tremolo |
| `O-Detune` | https://oaudio.io/products/ouaricon-detune |
| `O-AnalogEQ` | https://oaudio.io/products/ouaricon-analog-eq |
| `O-AnalogSaturation` | https://oaudio.io/products/ouaricon-saturation |
| `O-Comp` | https://oaudio.io/products/ouaricon-compressor |
| `O-Polystutter` | https://oaudio.io/products/ouaricon-polystutter |
| `O-Freeze` | https://oaudio.io/products/ouaricon-freeze |
| `O-FreqPulse` | https://oaudio.io/products/ouaricon-frequency-pulse |

The 16th page, `https://oaudio.io/products/ohands` (O-Hands), maps to no plugin in the README tables — a gestural controller, not a plugin in this repo. Leave it out.

The remaining 24 plugins in the tables have **no** product page. They stay plain text. Do not invent, guess, or pattern-extend a URL for any of them.
</verified_facts>

<tasks>

<task type="tracer">
  <name>Task 1: Promote and rewrite the Dorico integration section end-to-end</name>
  <files>README.md</files>
  <precondition>`research/microtonal-dorico-integration.md`, `modules/tuning/note-expression/README.md`, `plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md`, and `plugins/O-Contrabass/Resources/dorico/INSTALL-DORICO.md` all exist on disk (they are the link targets the verify gate resolves).</precondition>
  <read_first>
    - `README.md` lines 129-155 — the current `## Modern Interface Design` section containing the misplaced `### Microtonal Dorico Playback (v1.5)` subsection and the `### GUI-Optional Workflow` subsection that follows it.
    - `README.md` lines 74-79 — the end of the `## Plugins` section (plugin-registry line) and the start of `## The Development System`; this is the insertion point.
    - `plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md` — the four-family routing description and the distribution-mechanism table.
    - `plugins/O-MicrotonalSampler/CHANGELOG.md` — search for `ks_enabled` and `dynamics_mode` to CONFIRM the two flagged claims.
  </read_first>
  <action>
Move the Dorico material out of `## Modern Interface Design` (it is a WebView/UI section and has no relationship to Dorico) and rewrite it as a standalone H2 section placed immediately after the `## Plugins` section — that is, after the plugin-registry line and before `## The Development System`. Delete the old `### Microtonal Dorico Playback (v1.5)` subsection in place so `## Modern Interface Design` runs straight from its bullet list into `### GUI-Optional Workflow`.

Title the new section `## Microtonal Dorico Integration`. Drop the `(v1.5)` version marker from the heading — the capability has grown past that milestone and a version-stamped heading goes stale.

Write it in the README's existing register: declarative, specific, no marketing adjectives, em dashes for asides, plugin names in backticks. Target roughly 35-55 lines. Structure it as a short lead paragraph plus three H3 subsections:

1. Lead paragraph — Dorico drives per-note microtonal playback in these plugins directly from the score: quarter-tones, just intonation, and custom tunings, without MIDI pitch-bend workarounds or per-track lane hacks.

2. `### Per-Note Tuning (VST3 Note Expression)` — the shared module at `modules/tuning/note-expression` (v1.1.1) and what it does (owns the Note Expression Controller for `kTuningTypeID`, drains raw NE events from a patched JUCE wrapper at `scripts/juce-patches/note-expression-juce-8.0.14.patch`, applies per-note semitone offsets at the voice call site, composes with `scala-tuning-engine`). State that this is the VST3 path and Dorico loads VST3 on both macOS and Windows. Then list all **eleven** cohort plugins from the verified facts above — the current README text names only eight, and the three additions (`O-Bassoon`, `O-Contrabass`, `O-MicrotonalSampler`) are the correction this task must land. Close with the activation flow: installers bundle `Ouaricon-VST3-NoteExpression.doricolib`; the user runs Dorico -> Library -> Library Manager -> Import once, after which tuned pitches play back automatically.

3. `### Playback Templates, Expression Maps, and Keyswitches` — scope this honestly to the two plugins that actually ship bundles: `O-MicrotonalSampler` and `O-Contrabass`, each under `plugins/<Name>/Resources/dorico/` with playback template specs, endpoint configs, an install guide, and a smoke test. Note that O-MicrotonalSampler's template covers four instrument families (Strings, Winds, Brass, and a Generic fallback) in one Playback Template with Dorico routing each stave by instrument family. Add one sentence on the distribution mechanism, sourced from `INSTALL-DORICO.md`: a loose expression-map file is not picked up by Dorico's library scanner, so the bundles use the multi-folder layout plus `DefaultLibraryAdditions/`. Then the two O-MicrotonalSampler specifics you CONFIRMED against its CHANGELOG — keyswitching is opt-in on fresh instances, and dynamics can follow a continuous CC crossfading recorded layers instead of velocity. If the CHANGELOG does not clearly support a claim, omit that sentence entirely.

4. `### Further Reading` — a short list linking, with relative paths: `research/microtonal-dorico-integration.md` (describe it by its real H2s: module architecture, the canonical Dorico setup procedure, host-side behavior quirks, and troubleshooting signatures), `modules/tuning/note-expression/README.md`, and both `INSTALL-DORICO.md` files. Add one line pointing at the existing `/dorico` command for in-repo help; do not duplicate the command-reference table row.

Two things you must not touch: the `## Milestone History` v1.5 row (its "8 cohort plugins" and "v1.1.0" figures are an accurate record of what that milestone shipped and are not stale facts), and the plugin tables (Task 2 owns those).

Write only claims traceable to the verified facts above or to a file you read this task. Do not assert Dorico version compatibility, DAW behavior, or install paths that you have not read in the repo.
  </action>
  <verify>
    <automated>python3 - &lt;&lt;'PY'
import re, os
s = open('README.md').read()
h = '## Microtonal Dorico Integration'
assert h in s, 'H2 section missing'
# section must sit between the Plugins section and The Development System
assert s.index('## Plugins') &lt; s.index(h) &lt; s.index('## The Development System'), 'section misplaced'
# old misplaced subsection is gone
assert 'Microtonal Dorico Playback' not in s, 'old subsection still present'
sec = s[s.index(h):]
nxt = sec.find('\n## ', 3)
sec = sec[:nxt] if nxt &gt; 0 else sec
cohort = ['O-Bassoon','O-Bells','O-Bowed','O-Contrabass','O-Formant','O-IntonationPad','O-Lyrica','O-MicrotonalSampler','O-Prism','O-Reed','O-Wind']
missing = [p for p in cohort if p not in sec]
assert not missing, f'cohort names missing from section: {missing}'
links = re.findall(r'\]\((?!https?:|#)([^)#]+)\)', sec)
bad = [l for l in links if not os.path.exists(l)]
assert not bad, f'relative links do not resolve: {bad}'
assert len(links) &gt;= 4, f'expected at least 4 in-repo links, found {len(links)}'
print(f'OK — section placed, {len(cohort)} cohort names, {len(links)} in-repo links all resolve')
PY</automated>
    <human-check>Read the new section top to bottom. It should read as product documentation a Dorico user can act on, not as a changelog.</human-check>
  </verify>
  <done>`## Microtonal Dorico Integration` exists as an H2 between the Plugins section and The Development System; the old subsection under Modern Interface Design is gone; all eleven cohort plugins are named; every relative link in the section resolves to a real file. Committed.</done>
</task>

<task type="auto">
  <name>Task 2: Link plugin table rows to their oaudio.io product pages</name>
  <files>README.md</files>
  <read_first>
    - `README.md` lines 13-73 — the four plugin tables (Instruments &amp; Synths, the `simple` series, Effects &amp; Processors, Utilities).
  </read_first>
  <action>
For each of the 15 plugins in the verified mapping table above, turn the plugin-name cell of its table row into a markdown link to that plugin's product page, keeping the backticks inside the link text — the cell becomes the plugin name as link text wrapped in backticks, followed by the URL in parentheses. Leave the description cell untouched.

Apply this to exactly those 15 rows. The other 24 plugin rows keep their plain backticked name — no page exists for them, and the user's instruction is explicitly "when available". Do not extend the slug pattern to unlisted plugins, do not link to the `/products/` index as a stand-in, and do not link `ohands` (no matching row).

Do not add, remove, reorder, or rename any table row, and do not change any description text. The tables must still total 39 plugins.

Add one short sentence directly under the `## Plugins` intro line noting that linked names go to the plugin's page on the O-Audio site — so a reader understands why some names are links and some are not.

If any URL fails the sitemap gate below, remove that link rather than substituting a guess, and report it.
  </action>
  <verify>
    <automated>python3 - &lt;&lt;'PY'
import re, urllib.request
s = open('README.md').read()
rows = re.findall(r'^\| \[?`(O-[A-Za-z]+)`', s, re.M)
assert len(rows) == 39, f'plugin row count changed: {len(rows)} (expected 39)'
expected = {
 'O-Bells':'ouaricon-bells','O-Lyrica':'ouaricon-lyrica','O-Prism':'ouaricon-prism',
 'O-IntonationPad':'ouaricon-intonation-pad','O-SimpleReverb':'ouaricon-simple-reverb',
 'O-DigiDelay':'ouaricon-delay','O-Chorus':'ouaricon-chorus','O-Tremolo':'ouaricon-tremolo',
 'O-Detune':'ouaricon-detune','O-AnalogEQ':'ouaricon-analog-eq',
 'O-AnalogSaturation':'ouaricon-saturation','O-Comp':'ouaricon-compressor',
 'O-Polystutter':'ouaricon-polystutter','O-Freeze':'ouaricon-freeze',
 'O-FreqPulse':'ouaricon-frequency-pulse'}
linked = dict(re.findall(r'^\| \[`(O-[A-Za-z]+)`\]\((https://oaudio\.io/products/[a-z0-9-]+)\)', s, re.M))
assert set(linked) == set(expected), f'linked set mismatch: extra={set(linked)-set(expected)} missing={set(expected)-set(linked)}'
for name, url in linked.items():
    assert url.endswith('/' + expected[name]), f'{name} points at {url}'
sitemap = urllib.request.urlopen('https://oaudio.io/sitemap.xml', timeout=30).read().decode()
for name, url in sorted(linked.items()):
    assert url in sitemap, f'{name}: {url} is NOT in the live sitemap'
print(f'OK — 39 rows intact, {len(linked)} product links, all present in live sitemap')
PY</automated>
  </verify>
  <done>Exactly the 15 mapped plugin rows carry a product-page link; all 39 rows and every description are unchanged; every URL in the README appears in oaudio.io's live sitemap.xml. Committed.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| repo -> public reader | README.md is the public front page of a public repo; anything written here is published |
| README -> third-party web (oaudio.io) | outbound links leave the repo's trust domain |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-ozs-01 | Information Disclosure | new Dorico section in README.md | low | mitigate | Cite only paths that are already tracked and public in this repo; no local absolute paths, machine names, credentials, or unpublished internal locations |
| T-ozs-02 | Tampering | outbound oaudio.io product links | medium | mitigate | Every URL is gated against the site's own `sitemap.xml` in Task 2's verify; a URL that fails is removed, never guessed or pattern-extended |
| T-ozs-03 | Spoofing | fabricated capability claims (Dorico versions, DAW behavior, install paths) | medium | mitigate | Every factual claim traces to a repo file read during the task; the two flagged O-MicrotonalSampler claims must be CONFIRMED against CHANGELOG.md or omitted |
| T-ozs-SC | Tampering | package-manager installs | low | accept | No npm/pip/cargo install occurs in this plan — documentation edit only, no dependency surface |
</threat_model>

<verification>
- `git diff --stat` shows README.md as the only modified file.
- `grep -c '^## ' README.md` increases by exactly 1 relative to HEAD (the new Dorico H2; the old H3 was removed, not converted in place elsewhere).
- Both task-level automated gates pass.
</verification>

<success_criteria>
- README carries a standalone `## Microtonal Dorico Integration` section covering per-note VST3 Note Expression tuning, the full eleven-plugin cohort, the Library Manager activation flow, and the playback-template/expression-map/keyswitch bundles shipped by O-MicrotonalSampler and O-Contrabass.
- Every in-repo link in that section resolves; every fact is traceable to a repo file.
- 15 plugin rows link to verified oaudio.io product pages; 24 stay plain; 39 rows total, descriptions unchanged.
- Two atomic commits, one per task.
</success_criteria>

<output>
Create `.planning/quick/260805-ozs-to-the-readme-add-info-about-the-dorico-/260805-ozs-SUMMARY.md` when done.
</output>
