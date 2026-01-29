# Precondition Checks

## Contract Verification

Before starting Stage 1, verify all required planning documents exist and check cache:

```bash
# Source cache utilities
source .claude/utils/validation-cache.sh

# Define contract files
CONTRACT_FILES=(
    "plugins/$PLUGIN_NAME/.planning/architecture.md"
    "plugins/$PLUGIN_NAME/.planning/plan.md"
    "plugins/$PLUGIN_NAME/.planning/BRIEF.md"
    "plugins/$PLUGIN_NAME/.planning/parameter-spec.md"
)

# Check if contract verification is cached
if is_cached "contract-checksums" "$PLUGIN_NAME" "${CONTRACT_FILES[@]}"; then
    echo "✓ Contract integrity verified (cached)"
    # Skip re-verification
else
    # Verify all contracts exist
    MISSING_FILES=()
    for file in "${CONTRACT_FILES[@]}"; do
        if [ ! -f "$file" ]; then
            MISSING_FILES+=("$file")
        fi
    done

    if [ ${#MISSING_FILES[@]} -gt 0 ]; then
        echo "❌ Missing required contracts:"
        printf '%s\n' "${MISSING_FILES[@]}"
        exit 1
    fi

    # All contracts exist - cache the verification
    RESULT_JSON='{"status":"success","verified":true}'
    cache_result "contract-checksums" "$PLUGIN_NAME" 24 "$RESULT_JSON" "${CONTRACT_FILES[@]}"
    echo "✓ Contract integrity verified and cached"
fi
```

## Status Verification

Read PLUGINS.md to verify current plugin status:

```bash
# Extract plugin section
grep "^### $PLUGIN_NAME$" PLUGINS.md
```

Parse the Status line to determine current state:
- 📋 Planning → Planning just completed, OK to proceed
- 🔨 Building System → Implementation in progress, OK to resume
- 🎵 Processing Audio → DSP in progress, OK to resume
- 🎨 Designing Interface → UI in progress, OK to resume
- 💡 Concept Ready → Planning not started, BLOCK
- ✅ Ready to Install → Plugin complete, BLOCK (suggest /improve)
- 📦 Installed → Plugin deployed, BLOCK (suggest /improve)

## Block Messages

### Missing Contracts
```
Cannot start implementation - planning incomplete

[PluginName] is missing required planning documents:
- architecture.md (DSP design)
- plan.md (implementation strategy)
- BRIEF.md (vision document)

Complete planning first:
   Run /plan [PluginName] to create these documents

Then resume with /implement [PluginName]
```

### Wrong Status
```
[PluginName] needs planning before implementation.
Run /plan [PluginName] to complete stage 0.
```

OR

```
[PluginName] is already complete.
Use /improve [PluginName] to make changes.
```
