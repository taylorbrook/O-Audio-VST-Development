# Common Issues

## Build script not found

**Symptom:** `./scripts/build-and-install.sh: No such file or directory`

**Cause:** Skill invoked from wrong directory or script missing

**Fix:**
1. Verify working directory is plugin-freedom-system root
2. Check script exists: `ls -la scripts/build-and-install.sh`
3. Ensure script executable: `chmod +x scripts/build-and-install.sh`

## Plugin directory doesn't exist

**Symptom:** `plugins/[PluginName]: No such file or directory`

**Cause:** Skill invoked before plugin created or wrong name used

**Fix:**
1. Verify plugin exists: `ls plugins/`
2. Check exact name (case-sensitive)
3. If missing, run `/dream` and `/implement` first

## CMakeLists.txt missing

**Symptom:** `CMake Error: The source directory does not appear to contain CMakeLists.txt`

**Cause:** Foundation stage not completed or file deleted

**Fix:**
1. Check if file exists: `ls plugins/[PluginName]/CMakeLists.txt`
2. If missing, re-run Stage 2 (foundation-shell-agent)
3. Or restore from backup in `plugins/[PluginName]/.backup/`

## Build succeeds but menu not displayed

**Symptom:** Build completes but no "What's next?" menu appears

**Cause:** Success protocol not executed (line 209+ not reached)

**Fix:**
1. Check if skill exited early
2. Verify exit code capture logic (line 87-91)
3. Ensure Success Protocol section executed

## Troubleshooter invocation fails

**Symptom:** "Failed to invoke troubleshooter agent"

**Cause:** Task tool error or agent not found

**Fix:**
1. Verify troubleshooter agent exists in `.claude/agents/`
2. Check Task tool permissions
3. Try manual troubleshooting (option 2 or 3) instead
