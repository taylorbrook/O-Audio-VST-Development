# Validation Cache Utilities

Bash functions for managing validation result caching. Eliminates redundant validations by caching deterministic results with content checksums.

## Usage

Source this file in skills/commands that need caching:

```bash
source .claude/utils/validation-cache.sh
```

## Functions

### init_cache()
Initialize cache file if not exists.

### checksum_file(file)
Generate SHA-256 checksum for file.

### is_cached(validator, plugin, ...input_files)
Check if validation result is cached and valid.
Returns 0 (true) if cache hit, 1 (false) if cache miss.

### get_cached_result(validator, plugin)
Retrieve cached validation result as JSON.

### cache_result(validator, plugin, duration_hours, result_json, ...input_files)
Store validation result in cache with expiry.

### clear_cache(plugin)
Remove all cache entries for a plugin.

### clean_cache()
Remove expired cache entries.

## Cache Schema

```json
{
  "[validator]:[plugin]": {
    "timestamp": "2025-11-13T10:30:00Z",
    "expires_at": "2025-11-14T10:30:00Z",
    "inputs": {
      "path/to/file.md": "sha256-checksum-here"
    },
    "result": {
      "status": "success|failure",
      "findings": [],
      "data": {}
    }
  }
}
```

## Implementation

```bash
#!/bin/bash

CACHE_FILE=".claude/cache/validation-results.json"

# Initialize cache if not exists
init_cache() {
    mkdir -p .claude/cache
    [ ! -f "$CACHE_FILE" ] && echo '{}' > "$CACHE_FILE"
}

# Generate checksum for file
checksum_file() {
    local file=$1
    if [ ! -f "$file" ]; then
        echo "ERROR: File not found: $file" >&2
        return 1
    fi
    shasum -a 256 "$file" | awk '{print $1}'
}

# Check if cache is valid
is_cached() {
    local validator=$1
    local plugin=$2
    shift 2
    local inputs=("$@")  # Remaining args are input files

    init_cache

    # Extract cache entry
    local key="${validator}:${plugin}"
    local entry=$(jq -r ".\"$key\"" "$CACHE_FILE" 2>/dev/null)

    [ "$entry" = "null" ] && return 1  # Not cached
    [ -z "$entry" ] && return 1  # Empty result

    # Check expiry
    local expires_at=$(echo "$entry" | jq -r '.expires_at' 2>/dev/null)
    local now=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    [[ "$now" > "$expires_at" ]] && return 1  # Expired

    # Check input checksums
    for file in "${inputs[@]}"; do
        # Handle glob patterns
        if [[ "$file" == *"*"* ]]; then
            # Expand glob and check each file
            for expanded_file in $file; do
                [ ! -f "$expanded_file" ] && continue
                local current_checksum=$(checksum_file "$expanded_file")
                local cached_checksum=$(echo "$entry" | jq -r ".inputs.\"$expanded_file\"" 2>/dev/null)
                [ "$current_checksum" != "$cached_checksum" ] && return 1  # Changed
            done
        else
            [ ! -f "$file" ] && return 1  # File doesn't exist
            local current_checksum=$(checksum_file "$file")
            local cached_checksum=$(echo "$entry" | jq -r ".inputs.\"$file\"" 2>/dev/null)
            [ "$current_checksum" != "$cached_checksum" ] && return 1  # Changed
        fi
    done

    return 0  # Cache hit
}

# Get cached result
get_cached_result() {
    local validator=$1
    local plugin=$2
    local key="${validator}:${plugin}"

    init_cache
    jq -r ".\"$key\".result" "$CACHE_FILE" 2>/dev/null
}

# Store validation result
cache_result() {
    local validator=$1
    local plugin=$2
    local duration_hours=$3
    local result=$4
    shift 4
    local inputs=("$@")

    init_cache

    local key="${validator}:${plugin}"
    local now=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

    # Handle date command differences (macOS vs Linux)
    local expires_at
    if date -v +1H &>/dev/null; then
        # macOS
        expires_at=$(date -u -v +${duration_hours}H +"%Y-%m-%dT%H:%M:%SZ" 2>/dev/null)
    else
        # Linux
        expires_at=$(date -u -d "+${duration_hours} hours" +"%Y-%m-%dT%H:%M:%SZ" 2>/dev/null)
    fi

    # Build input checksums
    local input_json="{"
    for file in "${inputs[@]}"; do
        # Handle glob patterns
        if [[ "$file" == *"*"* ]]; then
            # Expand glob and add each file
            for expanded_file in $file; do
                [ ! -f "$expanded_file" ] && continue
                local checksum=$(checksum_file "$expanded_file")
                input_json+="\"$expanded_file\":\"$checksum\","
            done
        else
            [ ! -f "$file" ] && continue
            local checksum=$(checksum_file "$file")
            input_json+="\"$file\":\"$checksum\","
        fi
    done
    input_json="${input_json%,}}"  # Remove trailing comma

    # Build cache entry
    local entry=$(jq -n \
        --arg ts "$now" \
        --arg exp "$expires_at" \
        --argjson inputs "$input_json" \
        --argjson result "$result" \
        '{timestamp: $ts, expires_at: $exp, inputs: $inputs, result: $result}' 2>/dev/null)

    # Update cache
    local tmp_file="${CACHE_FILE}.tmp.$$"
    jq ".\"$key\" = $entry" "$CACHE_FILE" > "$tmp_file" 2>/dev/null && mv "$tmp_file" "$CACHE_FILE"
}

# Clear cache for plugin (supports wildcard patterns)
clear_cache() {
    local pattern=$1

    init_cache

    if [ -z "$pattern" ]; then
        echo "ERROR: Plugin name or pattern required" >&2
        return 1
    fi

    # Remove all entries matching pattern
    local tmp_file="${CACHE_FILE}.tmp.$$"
    if [ "$pattern" = "*" ]; then
        # Clear all
        echo '{}' > "$CACHE_FILE"
    else
        # Remove entries matching pattern
        jq "with_entries(select(.key | test(\"${pattern}\") | not))" "$CACHE_FILE" > "$tmp_file" 2>/dev/null && mv "$tmp_file" "$CACHE_FILE"
    fi
}

# Clear expired entries
clean_cache() {
    init_cache

    local now=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

    # Remove expired entries
    local tmp_file="${CACHE_FILE}.tmp.$$"
    jq "with_entries(select(.value.expires_at > \"$now\"))" "$CACHE_FILE" > "$tmp_file" 2>/dev/null && mv "$tmp_file" "$CACHE_FILE"
}

# Export functions if sourced
if [ "${BASH_SOURCE[0]}" != "${0}" ]; then
    export -f init_cache
    export -f checksum_file
    export -f is_cached
    export -f get_cached_result
    export -f cache_result
    export -f clear_cache
    export -f clean_cache
fi
```
