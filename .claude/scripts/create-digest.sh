#!/bin/bash
# create-digest.sh - Generate DIGEST.json for a plugin
# Usage: bash .claude/scripts/create-digest.sh <PluginName>
#
# Creates a compact JSON digest (< 500 tokens / 2000 chars) of a plugin's
# stage decisions, parameters, and DSP components for fast cross-stage
# context loading by any agent.
#
# Output: plugins/<PluginName>/.planning/DIGEST.json

set -euo pipefail

PLUGIN="${1:?Usage: create-digest.sh <PluginName>}"
PLANNING="plugins/$PLUGIN/.planning"
DIGEST="$PLANNING/DIGEST.json"

# Verify planning directory exists
if [ ! -d "$PLANNING" ]; then
  echo "Error: Planning directory not found: $PLANNING" >&2
  exit 1
fi

# --- Extract stage and phase from STATUS.md frontmatter ---
STAGE="unknown"
PHASE="unknown"

if [ -f "$PLANNING/STATUS.md" ]; then
  STAGE=$(sed -n '/^---$/,/^---$/p' "$PLANNING/STATUS.md" | grep -E '^stage:' | sed 's/^stage:[[:space:]]*//' | head -1 || true)
  PHASE=$(sed -n '/^---$/,/^---$/p' "$PLANNING/STATUS.md" | grep -E '^phase:' | sed 's/^phase:[[:space:]]*//' | head -1 || true)
  [ -z "$STAGE" ] && STAGE="unknown"
  [ -z "$PHASE" ] && PHASE="unknown"
fi

# --- Extract complexity score ---
# Check ROADMAP.md first (newer plugins), then plan.md (older plugins)
COMPLEXITY=0

if [ -f "$PLANNING/ROADMAP.md" ]; then
  COMPLEXITY=$(grep -iE 'complexity.*score|score.*complexity' "$PLANNING/ROADMAP.md" 2>/dev/null | grep -oE '[0-9]+\.?[0-9]*' | head -1 || true)
elif [ -f "$PLANNING/plan.md" ]; then
  COMPLEXITY=$(grep -iE 'complexity.*score|score.*complexity' "$PLANNING/plan.md" 2>/dev/null | grep -oE '[0-9]+\.?[0-9]*' | head -1 || true)
fi

[ -z "$COMPLEXITY" ] && COMPLEXITY=0

# --- Extract parameter IDs from parameter-spec.md ---
# Handle multiple formats:
#   1. ParameterID in code blocks: ParameterID{"NAME", ...}
#   2. Header sections: ### PARAM_NAME
#   3. Table rows: | paramName | ... |
PARAMS_JSON="[]"

if [ -f "$PLANNING/parameter-spec.md" ]; then
  # Try extracting from ParameterID code blocks first (most reliable)
  CODE_PARAMS=$(grep -oE 'ParameterID\{"[^"]+' "$PLANNING/parameter-spec.md" 2>/dev/null | sed 's/ParameterID{"//' | sort -u || true)

  if [ -n "$CODE_PARAMS" ]; then
    PARAMS_JSON=$(echo "$CODE_PARAMS" | jq -R . | jq -s -c .)
  else
    # Try extracting from ### headers
    HEADER_PARAMS=$(grep -E '^###\s+[A-Z]' "$PLANNING/parameter-spec.md" 2>/dev/null | sed 's/^###[[:space:]]*//' | head -20 || true)

    if [ -n "$HEADER_PARAMS" ]; then
      PARAMS_JSON=$(echo "$HEADER_PARAMS" | jq -R . | jq -s -c .)
    else
      # Try extracting from table rows (| paramName | ... |)
      TABLE_PARAMS=$(grep -E '^\|\s*[a-zA-Z]' "$PLANNING/parameter-spec.md" 2>/dev/null | grep -v '^\| Parameter\|^\| ---' | awk -F'|' '{gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2); print $2}' | head -20 || true)

      if [ -n "$TABLE_PARAMS" ]; then
        PARAMS_JSON=$(echo "$TABLE_PARAMS" | jq -R . | jq -s -c .)
      fi
    fi
  fi
fi

# --- Extract DSP component names from architecture files ---
# Check research/ARCHITECTURE.md first, then architecture.md at root
DSP_JSON="[]"
ARCH_FILE=""

if [ -f "$PLANNING/research/ARCHITECTURE.md" ]; then
  ARCH_FILE="$PLANNING/research/ARCHITECTURE.md"
elif [ -f "$PLANNING/architecture.md" ]; then
  ARCH_FILE="$PLANNING/architecture.md"
fi

if [ -n "$ARCH_FILE" ]; then
  DSP_COMPONENTS=$(
    {
      grep -oE 'juce::dsp::[A-Za-z]+' "$ARCH_FILE" 2>/dev/null | sed 's/juce::dsp:://' | sort -u || true
      grep -oE 'class [A-Z][A-Za-z]+' "$ARCH_FILE" 2>/dev/null | sed 's/class //' | sort -u || true
    } | sort -u | head -15
  )

  if [ -n "$DSP_COMPONENTS" ]; then
    DSP_JSON=$(echo "$DSP_COMPONENTS" | jq -R . | jq -s -c .)
  fi
fi

# --- Determine contract file paths (relative to .planning/) ---
BRIEF_PATH=""
if [ -f "$PLANNING/BRIEF.md" ]; then
  BRIEF_PATH=".planning/BRIEF.md"
elif [ -f "$PLANNING/creative-brief.md" ]; then
  BRIEF_PATH=".planning/creative-brief.md"
fi

PARAMS_PATH=""
[ -f "$PLANNING/parameter-spec.md" ] && PARAMS_PATH=".planning/parameter-spec.md"

ARCH_PATH=""
if [ -f "$PLANNING/research/ARCHITECTURE.md" ]; then
  ARCH_PATH=".planning/research/ARCHITECTURE.md"
elif [ -f "$PLANNING/architecture.md" ]; then
  ARCH_PATH=".planning/architecture.md"
fi

ROADMAP_PATH=""
if [ -f "$PLANNING/ROADMAP.md" ]; then
  ROADMAP_PATH=".planning/ROADMAP.md"
elif [ -f "$PLANNING/plan.md" ]; then
  ROADMAP_PATH=".planning/plan.md"
fi

# --- Build DIGEST.json using jq ---
# Build contracts object
CONTRACTS=$(jq -n \
  --arg brief "$BRIEF_PATH" \
  --arg params "$PARAMS_PATH" \
  --arg arch "$ARCH_PATH" \
  --arg roadmap "$ROADMAP_PATH" \
  '{} |
    if $brief != "" then . + {brief: $brief} else . end |
    if $params != "" then . + {params: $params} else . end |
    if $arch != "" then . + {arch: $arch} else . end |
    if $roadmap != "" then . + {roadmap: $roadmap} else . end')

# Build full digest
jq -n \
  --arg plugin "$PLUGIN" \
  --arg stage "$STAGE" \
  --arg phase "$PHASE" \
  --argjson complexity "$COMPLEXITY" \
  --argjson parameters "$PARAMS_JSON" \
  --argjson dsp_components "$DSP_JSON" \
  --argjson contracts "$CONTRACTS" \
  '{
    plugin: $plugin,
    stage: $stage,
    phase: $phase,
    complexity: $complexity,
    parameters: $parameters,
    dsp_components: $dsp_components,
    contracts: $contracts,
    decisions: {}
  }' > "$DIGEST"

# Validate output is valid JSON
if ! jq . "$DIGEST" > /dev/null 2>&1; then
  echo "Error: Generated invalid JSON at $DIGEST" >&2
  rm -f "$DIGEST"
  exit 1
fi

# Check size and truncate if needed
CHAR_COUNT=$(wc -c < "$DIGEST" | tr -d ' ')

if [ "$CHAR_COUNT" -gt 2000 ]; then
  # Truncate parameters and components using python3
  python3 -c "
import json, sys

with open('$DIGEST') as f:
    digest = json.load(f)

output = json.dumps(digest, indent=2)

while len(output) > 1800 and len(digest['parameters']) > 5:
    digest['parameters'] = digest['parameters'][:len(digest['parameters'])//2] + ['...']
    output = json.dumps(digest, indent=2)

while len(output) > 1800 and len(digest['dsp_components']) > 3:
    digest['dsp_components'] = digest['dsp_components'][:len(digest['dsp_components'])//2] + ['...']
    output = json.dumps(digest, indent=2)

with open('$DIGEST', 'w') as f:
    f.write(output + '\n')
"
  CHAR_COUNT=$(wc -c < "$DIGEST" | tr -d ' ')

  if [ "$CHAR_COUNT" -gt 2000 ]; then
    echo "Warning: DIGEST.json exceeds 2000 character budget ($CHAR_COUNT chars)" >&2
  fi
fi

echo "Created $DIGEST ($CHAR_COUNT chars)"
