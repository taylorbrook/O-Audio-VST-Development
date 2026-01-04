#!/bin/bash
# Reusable precondition check functions for slash commands

check_contracts() {
  local plugin_name=$1
  local base_path="plugins/${plugin_name}/.ideas"

  local missing=()

  [[ -f "${base_path}/architecture.md" ]] || missing+=("architecture.md")
  [[ -f "${base_path}/plan.md" ]] || missing+=("plan.md")
  [[ -f "${base_path}/parameter-spec.md" ]] || missing+=("parameter-spec.md")

  if [[ ${#missing[@]} -gt 0 ]]; then
    echo "MISSING_CONTRACTS:${missing[*]}"
    return 1
  fi

  return 0
}

check_plugin_status() {
  local plugin_name=$1
  grep "^### ${plugin_name}$" PLUGINS.md -A 2 | grep "Status:" | awk '{print $2, $3}'
}
