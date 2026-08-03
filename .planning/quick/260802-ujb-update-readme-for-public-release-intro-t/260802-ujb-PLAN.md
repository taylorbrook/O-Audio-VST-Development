---
phase: 260802-ujb
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - README.md
autonomous: true
quick_task: true
requirements: [RM-01, RM-02, RM-03]
user_setup: []

must_haves:
  truths:
    - "A first-time visitor landing on the repo reads who 0-Audio is and reaches https://oaudio.io/ from the first screen (RM-01)."
    - "Every plugin directory under plugins/ is named somewhere in README.md — no plugin is invisible to a visitor (RM-01)."
    - "Every command file under .claude/commands/ is documented in the README command reference, and the README documents no command that has no file (RM-02)."
    - "A visitor can determine the repo is AGPL-3.0 licensed and why JUCE is used under its AGPLv3 option (RM-03)."
    - "Version and count claims in the README match the values on disk, not stale prior-milestone values."
  artifacts:
    - "README.md — rewritten lead section, reconciled command reference, new licence section"
  key_links:
    - "README plugin catalog ↔ plugins/ directory listing (39 entries)"
    - "README command reference ↔ .claude/commands/ (50 files)"
    - "README licence section ↔ LICENSE (AGPL-3.0) + PUBLIC-RELEASE-READINESS.md §5.2 rationale"
---

<objective>
Rewrite README.md so the repository reads correctly as a public, open-source project for the audio-plugin publisher **0-Audio** (https://oaudio.io/), and reconcile every factual claim in it against the actual repo contents.

Purpose: the repo is about to be made public. Today the README opens as an internal tooling document, names no publisher, omits 12 shipped slash commands, documents 1 command that no longer exists, understates the plugin count, and contains no licence statement at all despite the repo being AGPL-3.0 as of 2026-08-01.

Output: an updated `README.md` at the repo root. No other file changes.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
</execution_context>

<context>
@README.md
@PLUGINS.md
@PUBLIC-RELEASE-READINESS.md
@CLAUDE.md
</context>

<ground_truth>
The following was measured on disk during planning on 2026-08-02. Re-derive each value at execution time — do NOT copy these numbers blindly, and never invent a plugin or command that is not on disk.

| Fact | Measured value | How to re-derive |
|------|----------------|------------------|
| Plugin directories | 39 | `ls -1 plugins/ \| wc -l` |
| PLUGINS.md registry rows | 39 | registry table has a `Type` column usable for grouping |
| Command files | 50 | `ls -1 .claude/commands/` |
| Commands missing from README | 12 | `build-installer`, `dorico`, `generalize-microtones`, `improve-milestone`, `improve-review`, `improve-review-info`, `improve-verify`, `module-upgrade-all`, `plugin-critique`, `plugin-handoff`, `simplify-phase2`, `simplify-phase3` |
| Commands in README with no file | 1 | the validation-cache one under Lifecycle Management |
| Licence | AGPL-3.0 | root `LICENSE`; rationale in `PUBLIC-RELEASE-READINESS.md` §5.2 |
| note-expression module version | 1.1.1 | `modules/tuning/note-expression/module.yaml` |
| Scripts absent from README tree | 3 | `add-agpl-headers.py`, `regen-registry-used-by.sh`, `resolve-target.sh` |
| Template count ("17 templates") | still accurate | 9 code-snippets + 8 prose-patterns |

**Branding note:** the repo's internal C++ namespace and CMake identifiers are `Ouaricon` / `OUARICON_*`, and PKG installer assets already say "Ouaricon Audio | oaudio.io". The public publisher name the user wants in the README is **0-Audio**. Use 0-Audio in the README prose. Do NOT rename any code identifier, CMake variable, installer asset, or namespace — that is out of scope for this task.
</ground_truth>

<tasks>

<task type="tracer">
  <name>Task 1: Public-visitor path — 0-Audio lead section, plugin catalog, and licence</name>
  <files>README.md</files>
  <precondition>Repo root contains `LICENSE` whose first lines identify the GNU Affero General Public License v3, and `PUBLIC-RELEASE-READINESS.md` exists. Halt if either is absent.</precondition>
  <read_first>
    - `README.md` lines 1-20 (current lead) and 916-927 (Acknowledgments / footer)
    - `PLUGINS.md` — the "Plugin Registry" table; the `Type` column is the grouping source
    - `PUBLIC-RELEASE-READINESS.md` §5.2 — the authoritative rationale for electing AGPL-3.0
  </read_first>
  <action>
Restructure the top of README.md so it opens as 0-Audio's public repository, then add a licence section near the end. This is the single end-to-end path a first-time visitor takes; get it fully correct before touching anything else.

**1. New lead section (RM-01).** Replace the current H1 with `# 0-Audio`. Under it write 2-3 tight paragraphs, professional and concise, matching the existing README's register:
   - 0-Audio is an open-source audio software publisher releasing free VST3 and AU plugins. Link the name to https://oaudio.io/ on first mention.
   - This repository holds the full source for the plugin catalog **and** the AI-assisted development system that builds them.
   - Plugins are free / pay-what-you-want and the whole catalog is open source under AGPL-3.0 (one sentence, forward-referencing the licence section).
   Do not invent company history, headcount, founding dates, revenue, or a mission statement that is not supported by repo contents.

**2. Plugin catalog subsection (RM-01).** Immediately after the lead, add `## Plugins`. Derive the list from `ls -1 plugins/` — every directory must appear. Group them under short subheadings using the `Type` column of the PLUGINS.md registry table (e.g. Synths & Instruments / Samplers / Effects & Processors / Utilities); put each plugin's one-line descriptor from that same `Type` column beside its name. Close the subsection with a link to `PLUGINS.md` for per-plugin versions and status. If a `plugins/` directory has no PLUGINS.md row, list it anyway with a descriptor read from its own `plugins/<Name>/.planning/BRIEF.md` or CMakeLists — do not silently drop it, and do not fabricate a descriptor.

**3. Demote the existing system intro.** The current H1 "Claude-assisted VST Development" and its opening paragraph become an H2 section (e.g. `## The Development System`) placed after the plugin catalog. Preserve its wording — including the macOS-primary / cross-platform-CI framing — and preserve every existing H2 below it and their heading levels. This is a re-frame, not a rewrite of the body.

**4. Licence section (RM-03).** Add `## License` immediately before `## Acknowledgments`, stating:
   - The repository and every plugin in it are licensed AGPL-3.0; link the root `LICENSE` file.
   - JUCE is used under its AGPLv3 option (not the free Starter tier), because this repo redistributes JUCE-owned files and Starter's revenue cap counts pay-what-you-want income. Keep this to one or two sentences — `PUBLIC-RELEASE-READINESS.md` §5.2 carries the long form.
   - Source files carry AGPL notice headers applied by `scripts/add-agpl-headers.py`.
   - Anyone distributing a derived plugin inherits AGPL-3.0 obligations.
   Also add a one-line licence mention to the `## Acknowledgments` JUCE bullet so the framework's licensing basis is visible where JUCE is credited.

**5. Keep it public-safe.** Write no machine-local absolute paths into the README — repo-relative paths and `~/Library/...` style paths only.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && MISS=""; for p in $(ls -1 plugins/); do grep -qE "\b$p\b" README.md || MISS="$MISS $p"; done; if [ -n "$MISS" ]; then echo "FAIL missing plugins:$MISS"; exit 1; fi; grep -q "https://oaudio.io/" README.md || { echo "FAIL no oaudio.io link"; exit 1; }; head -1 README.md | grep -q "0-Audio" || { echo "FAIL README does not lead with 0-Audio"; exit 1; }; grep -q "AGPL-3.0" README.md || { echo "FAIL no AGPL-3.0 statement"; exit 1; }; grep -qE '^## Licen[sc]e' README.md || { echo "FAIL no License section"; exit 1; }; grep -nE '/(Users|home)/[a-z]' README.md && { echo "FAIL machine-local path in README"; exit 1; }; echo GATE-PASS</automated>
  </verify>
  <done>README.md begins with an `# 0-Audio` H1 introducing the publisher and linking https://oaudio.io/; a `## Plugins` catalog names all 39 plugin directories with descriptors and links PLUGINS.md; the prior system intro survives verbatim as an H2; a `## License` section states AGPL-3.0 with the JUCE AGPLv3 rationale; no machine-local path appears anywhere in the file.</done>
</task>

<task type="auto">
  <name>Task 2: Reconcile the command reference against .claude/commands/</name>
  <files>README.md</files>
  <read_first>
    - `README.md` "Complete Command Reference" section (currently ~lines 632-752) — all category tables
    - The frontmatter `description:` line of each command file that needs a new row, to source its Purpose text
  </read_first>
  <action>
Bring the README's "Complete Command Reference" to exact parity with `.claude/commands/` (RM-02). The section is organised into category tables (Setup / Starting a Plugin / Context Management / Implementation / Testing & Validation / Post-Completion / Lifecycle Management / Module System / Research & Troubleshooting / System) — keep that organisation.

**Add a row for each of the 12 undocumented commands**, placed in the category it belongs to. Source each row's Purpose from the `description:` frontmatter of the corresponding file in `.claude/commands/` — read the file, do not guess. Suggested placement:
   - `/improve-review`, `/improve-review-info`, `/improve-verify`, `/improve-milestone` → Post-Completion. These form a review→resolve→verify loop around `/improve`; add one short sentence above the table explaining that ordering so the four rows read as a sequence rather than four unrelated entries.
   - `/simplify-phase2`, `/simplify-phase3` → Post-Completion, adjacent to the improve-review rows. Note in their Purpose text that they sweep tiers of a plugin's SIMPLIFICATION-AUDIT.md.
   - `/plugin-critique`, `/plugin-handoff` → Implementation (Stages 1-4), alongside the other `/plugin-*` phase commands.
   - `/build-installer` → Post-Completion, next to `/package` and `/publish` (it produces the Windows EXE installer).
   - `/module-upgrade-all` → Module System, after `/module-upgrade`.
   - `/dorico` → Research & Troubleshooting or a new row group; it is the Dorico integration helper (microtonal playback, expression maps, keyswitch routing).
   - `/generalize-microtones` → System, or beside `/dorico`.

**Remove the one Lifecycle Management row whose command file does not exist.** Its `.claude/commands/` file was deleted; the README is the last place still advertising it. Confirm absence with `ls .claude/commands/` before deleting the row.

Keep the existing "Typical Workflow Example" block and the Stage-0→Stage-4 diagram unchanged — they reference only commands that still exist.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && grep -o '`/[a-z0-9-]*' README.md | tr -d '`/' | grep -v '^$' | grep -vxE 'clear|tmp|Users|Library' | sort -u > /tmp/rm-cmds.txt && ls -1 .claude/commands/ | sed 's/\.md$//' | sort -u > /tmp/act-cmds.txt && if diff /tmp/rm-cmds.txt /tmp/act-cmds.txt; then echo GATE-PASS; else echo "FAIL: '<' = documented but no command file; '>' = command file but undocumented"; exit 1; fi</automated>
  </verify>
  <done>The set of slash commands referenced in README.md (excluding the Claude Code built-in `/clear`) is exactly equal to the set of files in `.claude/commands/` — the diff gate prints GATE-PASS with no output.</done>
</task>

<task type="auto">
  <name>Task 3: Sweep stale factual claims</name>
  <files>README.md</files>
  <read_first>
    - `modules/tuning/note-expression/module.yaml` — the `version:` field
    - `ls -1 scripts/` — the actual script inventory
    - `README.md` "Project Structure" tree and the Multi-Plugin / Best Practices example blocks
  </read_first>
  <action>
Correct the remaining measurable drifts. Each edit below must be grounded in a value re-read from disk at execution time.

**1. Plugin count sentence.** The sentence beginning "N plugins have been built with this system so far" states a count one lower than the true directory count. Set it to the value of `ls -1 plugins/ | wc -l`.

**2. note-expression module version.** The "Microtonal Dorico Playback" subsection cites a module version older than the one in `modules/tuning/note-expression/module.yaml`. Update the citation to the `version:` value read from that file.

**3. scripts/ tree.** The Project Structure tree omits three scripts that exist: `add-agpl-headers.py`, `regen-registry-used-by.sh`, `resolve-target.sh`. Add a line for each with a short comment, matching the tree's existing comment style.

**4. Fictional plugin names in examples.** The Multi-Plugin Parallel Development diagram and several example blocks use invented names that read like shipped products (a reverb, a compressor, a saturator, a simple-gain). On a public README these are indistinguishable from the real catalog. Replace each with either a real plugin name from `plugins/` or an obviously-generic placeholder in the `O-MyPlugin` / `O-NewPlugin` style already used elsewhere in the file. Do not change the surrounding prose or the command syntax being demonstrated.

**5. Leave alone.** Do NOT rewrite the "Implementation Status", "Milestone History", or "17 templates" content — the template count was re-verified accurate during planning (9 code-snippets + 8 prose-patterns), and milestone history is a historical record, not a current-state claim.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && PC=$(ls -1 plugins/ | wc -l | tr -d ' ') && NEV=$(grep -m1 '^version:' modules/tuning/note-expression/module.yaml | awk '{print $2}') && grep -qE "^${PC} plugins have been built" README.md || { echo "FAIL plugin count sentence != $PC"; exit 1; }; grep "note-expression" README.md | grep -q "v${NEV}" || { echo "FAIL note-expression version != v$NEV"; exit 1; }; for s in add-agpl-headers.py regen-registry-used-by.sh resolve-target.sh; do grep -q "$s" README.md || { echo "FAIL script not in tree: $s"; exit 1; }; done; grep -qE 'O-(Reverb|Compressor|Saturator|SimpleGain)\b' README.md && { echo "FAIL invented product-like plugin name still present"; exit 1; }; echo GATE-PASS</automated>
  </verify>
  <done>The plugin-count sentence matches `ls -1 plugins/ | wc -l`; the note-expression citation matches `module.yaml`; all three previously-omitted scripts appear in the Project Structure tree; no invented product-style plugin name remains in the examples.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| private repo → public internet | Making the repo public exposes README.md to anyone; anything written into it is permanently published and mirrored. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-ujb-01 | Information Disclosure | README.md prose | medium | mitigate | Task 1 forbids machine-local absolute paths; the Task 1 gate fails the build if a home-directory path appears in README.md. |
| T-ujb-02 | Information Disclosure | README licence section | medium | mitigate | Licence section summarises only; the detailed AGPL/JUCE rationale stays in `PUBLIC-RELEASE-READINESS.md` rather than being copied into the public front page. |
| T-ujb-03 | Repudiation | AGPL-3.0 statement | high | mitigate | The licence claim is grounded in the root `LICENSE` file and §5.2 of the readiness audit, both read in Task 1 — not written from memory. |
| T-ujb-SC | Tampering | package installs | low | accept | Documentation-only change; this plan installs no npm/pip/cargo packages, so no legitimacy gate applies. |
</threat_model>

<verification>
All three task gates must print `GATE-PASS`. Additionally, confirm the change is documentation-only:

`git diff --name-only` lists `README.md` and nothing else.

Then re-read the rendered README top-to-bottom once as a first-time visitor: heading hierarchy is unbroken (single H1, no skipped levels), the plugin catalog reads as a product list rather than a directory dump, and the licence statement is unambiguous.
</verification>

<success_criteria>
- README.md leads with 0-Audio as an open-source audio-plugin publisher, linking https://oaudio.io/ (RM-01)
- All 39 plugin directories are named in the README with descriptors and a link to PLUGINS.md (RM-01)
- README command reference is in exact set-parity with `.claude/commands/` — 12 additions, 1 removal (RM-02)
- README states AGPL-3.0 licensing with the JUCE AGPLv3 rationale, linking LICENSE (RM-03)
- Plugin count, note-expression version, and scripts/ tree match disk
- `README.md` is the only modified file
</success_criteria>

<output>
Commit the README change with `docs: rewrite README for public release — 0-Audio intro, command parity, AGPL-3.0 licence`, then write `.planning/quick/260802-ujb-update-readme-for-public-release-intro-t/260802-ujb-SUMMARY.md`.
</output>
