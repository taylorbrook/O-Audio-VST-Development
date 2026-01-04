# Integration Examples

## Scenario 1: From Stage 1 (Foundation)

**Context:** plugin-workflow invoked build-automation after foundation-shell-agent completed

**Invocation:**
```
Invoking build-automation for LushVerb (Stage 1: Foundation)
Flags: --no-install
```

**Workflow:**
1. Validate plugin directory exists
2. Execute: `./scripts/build-and-install.sh LushVerb --no-install`
3. Build succeeds (foundation code compiles)
4. Present success menu:

```
Foundation verified. What's next?
1. Continue to Stage 2 (Shell/Parameters) (recommended)
2. Review generated code
3. Pause workflow
```

5. User chooses 1
6. Exit skill, return to plugin-workflow
7. plugin-workflow invokes foundation-shell-agent

## Scenario 2: From Stages 2-3

**Context:** plugin-workflow invoked build-automation after dsp-agent/gui-agent completed

**Invocation:**
```
Invoking build-automation for LushVerb (Stage 2: DSP)
Flags: none (full build + install)
```

**Workflow:**
1. Validate plugin directory exists
2. Execute: `./scripts/build-and-install.sh LushVerb`
3. Build succeeds, binaries installed to system folders
4. Present success menu:

```
DSP implementation complete. What's next?
1. Continue with automatic validation (recommended)
2. Test in DAW now
3. Review DSP code
4. Pause workflow
```

5. User chooses 2 (test in DAW)
6. User tests plugin, finds issue
7. User returns: "continue"
8. Present menu again, user chooses 1
9. Exit skill, return to plugin-workflow
10. plugin-workflow invokes gui-agent

## Scenario 3: From plugin-improve

**Context:** plugin-improve invoked build-automation after applying fixes

**Invocation:**
```
Invoking build-automation for LushVerb (plugin-improve Phase 5)
Flags: none (full build + install)
```

**Workflow:**
1. Validate plugin directory exists
2. Execute: `./scripts/build-and-install.sh LushVerb`
3. Build fails (introduced regression)
4. Present failure menu:

```
⚠️ Build failed

What would you like to do?
1. Investigate - Run troubleshooter agent (recommended)
2. Show me the build log
3. Show me the code
4. Wait - I'll fix manually
5. Other
```

5. User chooses 1 (investigate)
6. Invoke troubleshooter agent
7. Troubleshooter finds: "Missing include for <memory> in PluginProcessor.cpp"
8. Display findings, re-present failure menu
9. User chooses 4 (wait, I'll fix manually)
10. Exit skill, return to plugin-improve
11. plugin-improve restores backup, asks user what to do next
