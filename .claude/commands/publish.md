---
name: publish
description: Publish plugin releases via GitHub Actions CI/CD
argument-hint: <PluginName> [--version patch|minor|major]
---

# /publish

Publishes a plugin release by bumping version, updating changelog, creating a git tag, and triggering GitHub Actions to build cross-platform binaries.

<preconditions enforcement="blocking">
  <check target="PLUGINS.md" condition="status_in" required="true">
    Plugin status MUST be 📦 Installed or ✅ Working

    <on_failure status="🚧 In Development">
      Display: "Cannot publish {{PLUGIN_NAME}} - still in development (Stage [N])"
      Guide: "Complete the workflow first with /continue {{PLUGIN_NAME}}"
    </on_failure>

    <on_failure status="💡 Ideated">
      Display: "Cannot publish {{PLUGIN_NAME}} - not implemented yet"
      Guide: "Use /implement {{PLUGIN_NAME}} to build it first"
    </on_failure>
  </check>

  <check target="git" condition="plugin_scope_clean" required="true">
    Publish-scoped paths MUST be clean: `plugins/{{PLUGIN_NAME}}/` and `PLUGINS.md`.
    Uncommitted changes elsewhere in the repo (e.g. another plugin being worked
    on in parallel) do NOT block — the release commit stages only its own paths.

    Check: git status --porcelain -- "plugins/{{PLUGIN_NAME}}/" PLUGINS.md

    <on_failure action="block">
      Display: "Cannot publish {{PLUGIN_NAME}} - it has uncommitted changes in its own files or PLUGINS.md"
      Guide: "Commit or stash changes to plugins/{{PLUGIN_NAME}}/ and PLUGINS.md before publishing"
    </on_failure>

    <on_unrelated_dirty action="warn">
      Display: "Note: unrelated uncommitted changes exist elsewhere in the repo (listed); they will NOT be included in the release commit"
    </on_unrelated_dirty>
  </check>

  <check target="git" condition="remote_exists" required="true">
    Git remote 'origin' MUST exist

    <on_failure action="block">
      Display: "No git remote configured"
      Guide: "Add remote with: git remote add origin <url>"
    </on_failure>
  </check>
</preconditions>

<routing>
  <invoke skill="plugin-publishing" with="$ARGUMENTS" required="true">
    Pass plugin name and optional --version flag to plugin-publishing skill
  </invoke>
</routing>

<state_contracts>
  <reads target="PLUGINS.md">
    Plugin status, current version, type
  </reads>
  <reads target="plugins/{{PLUGIN_NAME}}/CMakeLists.txt">
    VERSION field extraction
  </reads>
  <reads target="plugins/{{PLUGIN_NAME}}/CHANGELOG.md">
    Existing changelog entries
  </reads>
  <reads target=".claude/branding.json">
    Company metadata for release notes
  </reads>
  <writes target="plugins/{{PLUGIN_NAME}}/CMakeLists.txt">
    Updated VERSION field
  </writes>
  <writes target="plugins/{{PLUGIN_NAME}}/CHANGELOG.md">
    New changelog entry for release
  </writes>
  <writes target="PLUGINS.md">
    Updated version and date
  </writes>
  <writes target="git">
    Commit with release changes, tag in format {{PLUGIN_NAME}}-v{{VERSION}}
  </writes>
</state_contracts>

<success_criteria>
  Publishing succeeds when:
  - Version bumped in CMakeLists.txt
  - Changelog entry added
  - PLUGINS.md updated
  - Changes committed
  - Tag created and pushed
  - GitHub Actions workflow triggered
  - User provided with release tracking URL
</success_criteria>

<invocation_examples>
  User input: "/publish OuariconTremolo"
  → Prompts for version type (patch/minor/major), then publishes

  User input: "/publish OuariconTremolo --version minor"
  → Bumps to next minor version and publishes

  User input: "Publish a new release of GainKnob"
  → Natural language routing to plugin-publishing skill
</invocation_examples>
