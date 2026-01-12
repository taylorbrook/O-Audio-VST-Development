---
description: Clear validation cache for a plugin or entire system (project)
---

Clear cached validation results to force re-validation.

**Usage:**
- `/clear-cache [PluginName]` - Clear cache for specific plugin
- `/clear-cache --all` - Clear entire cache
- `/clear-cache --expired` - Remove only expired entries

**When to use:**
- After changing validation logic
- When suspecting stale cache results
- After system updates or configuration changes
- To force fresh validation on unchanged content

**Implementation:**

```bash
ARG="${1:-}"
STAGE="${2:-}"

if [ "$ARG" = "--all" ]; then
    # Clear cache for all plugins
    for plugin_dir in plugins/*/; do
        plugin_name=$(basename "$plugin_dir")
        if [ -d "plugins/$plugin_name/.validation-cache" ]; then
            python3 .claude/hooks/validators/validation-cache.py clear "$plugin_name"
        fi
    done
    echo ""
    echo "✓ All plugin validation caches cleared"
    echo "Next validations will run full pluginval tests"

elif [ -n "$ARG" ]; then
    # Clear cache for specific plugin
    PLUGIN_NAME="$ARG"

    # Verify plugin exists
    if [ ! -d "plugins/$PLUGIN_NAME" ]; then
        echo "❌ Plugin not found: $PLUGIN_NAME"
        echo ""
        echo "Available plugins:"
        ls -1 plugins/ | grep -v "^\." | head -5
        exit 1
    fi

    # Show current status
    python3 .claude/hooks/validators/validation-cache.py status "$PLUGIN_NAME"
    echo ""

    # Clear cache (specific stage or all)
    if [ -n "$STAGE" ]; then
        python3 .claude/hooks/validators/validation-cache.py clear "$PLUGIN_NAME" "$STAGE"
    else
        python3 .claude/hooks/validators/validation-cache.py clear "$PLUGIN_NAME"
    fi
    echo ""
    echo "Next validation will run full pluginval tests"

else
    # No arguments - show usage and status
    echo "Clear validation cache to force re-validation"
    echo ""
    echo "Usage:"
    echo "  /clear-cache [PluginName]     - Clear all stage caches for a plugin"
    echo "  /clear-cache [PluginName] 2   - Clear only Stage 2 cache"
    echo "  /clear-cache --all            - Clear all plugin caches"
    echo ""
    echo "Current cache status:"
    for plugin_dir in plugins/*/; do
        plugin_name=$(basename "$plugin_dir")
        if [ -d "plugins/$plugin_name/.validation-cache" ]; then
            python3 .claude/hooks/validators/validation-cache.py status "$plugin_name"
            echo ""
        fi
    done
fi
```

**Cache structure:**
Each plugin stores validation results in `plugins/{PluginName}/.validation-cache/`:
- `stage-1.json` - Foundation validation (smoke test ~10s)
- `stage-2.json` - DSP validation (functional test ~2-3min)
- `stage-3.json` - GUI validation (full test ~5-10min)

**Cache is invalidated automatically when:**
- Source files are modified (hash mismatch)
- Previous validation failed (must re-run after fix)

**Safety:**
- Clearing cache only forces re-validation, doesn't modify plugin files
- Cache is automatically rebuilt on next validation
- Cached PASS results save significant validation time
