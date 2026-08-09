---
phase: quick-260808-uiq
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - README.md
autonomous: true
requirements: [QUICK-260808-UIQ-01]

must_haves:
  truths:
    - "A reader scrolling README.md finds a `## Support` heading immediately after the plugin catalog and before the Microtonal Dorico Integration section."
    - "That section contains exactly one link, pointing at https://oaudio.io/support."
    - "No existing README line is altered or removed — the change is purely additive."
  artifacts:
    - "README.md — one new `## Support` section (heading + one paragraph)."
  key_links:
    - "README.md `## Support` -> https://oaudio.io/support (the site's own Donate page)."
---

<objective>
Add a short, dedicated `## Support` section to `README.md` linking to https://oaudio.io/support — the donate page the oaudio.io site already uses.

Purpose: the repo currently has no donation link anywhere (no `.github/FUNDING.yml`, no Support section). The README already tells readers the plugins are free and pay-what-you-want but gives them no way to act on it.

Output: `README.md` with one new section, 3 added lines plus a blank separator. Nothing else in the repo changes.

Scope note: this is a single-file, single-layer documentation change. There is no stack to trace through — the one task below *is* the whole vertical slice.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
</execution_context>

<context>
@README.md
</context>

<facts_established_before_planning>
These were verified by reading `README.md` at plan time. Do not re-derive them.

1. **The URL is `https://oaudio.io/support`.** It is the site's dedicated Donate page ("Support - Ouaricon Audio" / "Support Ouaricon Audio"), offering suggested amounts of $5 / $10 / $15 / $30 with the framing "If a plugin has earned a place in your work, a contribution of any size helps keep new ones coming."
2. **README.md has NO table of contents.** No heading-link list exists anywhere in the file, so no TOC entry needs adding.
3. **README.md uses NO badges.** No shields.io or equivalent image links. Do not add one.
4. **Line 7 already reads:** `Every plugin is free and pay-what-you-want, and the entire catalog is open source under AGPL-3.0 — see [License](#license).` — this line is NOT modified by this plan (see Decision below).
5. **The Plugins section ends at line 76:** `Per-plugin versions, release state, and history live in the **[plugin registry](PLUGINS.md)**.` Line 77 is blank. Line 78 is `## Microtonal Dorico Integration`.
</facts_established_before_planning>

<decisions>
**D-01 — Placement: immediately after the Plugins section, before `## Microtonal Dorico Integration`.**
Rationale (one line): the reader has just finished the 39-plugin catalog, which is the exact moment the ask makes sense — and it keeps the donation ask *below* the catalog rather than above it, while staying far more discoverable than the License/Acknowledgments tail.

**D-02 — Do NOT link the URL inline on line 7.** One canonical link only. Line 7 already carries a `[License](#license)` link and restating "pay-what-you-want" as a second live donate link 70 lines above the section is the redundancy the scope warns against. Leave line 7 byte-identical.

**D-03 — Copy is one heading + one paragraph, no emoji, no badge.** Voice matches the surrounding declarative register (cf. line 11: "All plugins build as VST3 (macOS and Windows) and AU (macOS), and load in any compatible DAW."). Borrows the site's own "earned a place in your work" phrasing rather than inventing marketing copy.

**D-04 — Out of scope, do not touch:** `.github/FUNDING.yml`, plugin source, UI, CHANGELOGs, and any restructuring of existing README sections.
</decisions>

<tasks>

<task type="auto">
  <name>Task 1: Insert the Support section into README.md</name>
  <files>README.md</files>
  <precondition>`README.md` line 76 is `Per-plugin versions, release state, and history live in the **[plugin registry](PLUGINS.md)**.` and line 78 is `## Microtonal Dorico Integration`. If those lines have moved, locate them by content and insert between them; do not insert blindly by line number.</precondition>
  <action>
Use `Edit` (not `Write`) on `README.md`. Per D-01, insert the new section between the end of the Plugins section and the Microtonal Dorico Integration heading.

Anchor the edit on this exact existing two-line-plus-blank region:

  old_string:
    Per-plugin versions, release state, and history live in the **[plugin registry](PLUGINS.md)**.

    ## Microtonal Dorico Integration

  new_string:
    Per-plugin versions, release state, and history live in the **[plugin registry](PLUGINS.md)**.

    ## Support

    Everything here is free — no paid tier, no license to buy. If a plugin has earned a place in your work, a contribution at [oaudio.io/support](https://oaudio.io/support) helps keep new ones coming.

    ## Microtonal Dorico Integration

Write the paragraph verbatim as given — it is the approved copy per D-03, not a draft to improve on. The em dash is a real em dash (U+2014), matching the rest of the file. The link text is the bare domain-and-path, the target is the full `https://` URL.

Per D-02, make no other edit: line 7 stays byte-identical, the Plugins tables stay byte-identical, and no section is reordered. Per D-04, create no new files.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && grep -c 'https://oaudio.io/support' README.md | grep -qx '1' && grep -qx '## Support' README.md && awk '/^Per-plugin versions, release state, and history live in/{a=NR} /^## Support$/{b=NR} /^## Microtonal Dorico Integration$/{c=NR} END{if(a&&b&&c&&a<b&&b<c) exit 0; else exit 1}' README.md && git diff --numstat -- README.md | awk '{if($1=="4" && $2=="0") exit 0; else exit 1}' && echo GATE-OK</automated>
  </verify>
  <done>
  - `README.md` contains exactly one occurrence of `https://oaudio.io/support`.
  - A line reading exactly `## Support` exists.
  - Section order is: plugin-registry line -> `## Support` -> `## Microtonal Dorico Integration`.
  - `git diff --numstat -- README.md` reports `4` insertions and `0` deletions (three content lines plus one blank separator; zero deletions proves nothing existing was rewritten).
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| README -> external web | A public-repo README sends readers to a third-party URL under the project's own control (oaudio.io). |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-uiq-01 | Tampering | The donate URL in README.md | low | mitigate | URL is pinned verbatim in this plan (`https://oaudio.io/support`), is HTTPS, and is on the project's own already-linked domain (`oaudio.io` appears 16 times in the file). The verify gate asserts exactly one occurrence, so a typo'd or duplicated variant fails the gate. |
| T-uiq-02 | Tampering | Unintended rewrite of surrounding README content | low | mitigate | `git diff --numstat` gate requires zero deletions — any accidental rewrite of the intro, the plugin tables, or the License section fails the task. |

No package-manager installs in this plan, so the package legitimacy gate does not apply.
</threat_model>

<verification>
Run from the repo root:

```bash
grep -n 'oaudio.io/support' README.md          # exactly one hit, inside the Support section
grep -n '^## ' README.md | head -20            # Support sits between Plugins and Microtonal Dorico Integration
git diff -- README.md                          # visually confirm: 4 added lines, 0 removed
```

Manual read-through: the new paragraph should read as part of the same restrained voice as its neighbours — no emoji, no badge, no exclamation, no second link.
</verification>

<success_criteria>
- `README.md` has a `## Support` section placed per D-01.
- It links to `https://oaudio.io/support` exactly once.
- Line 7 and every other pre-existing README line are unchanged (0 deletions in the diff).
- No new files created; `.github/FUNDING.yml` still does not exist.
</success_criteria>

<output>
Create `.planning/quick/260808-uiq-add-oaudio-io-donation-support-link-to-r/260808-uiq-SUMMARY.md` when done.
</output>
