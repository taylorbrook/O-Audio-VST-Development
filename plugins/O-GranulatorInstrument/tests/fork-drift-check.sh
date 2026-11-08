#!/usr/bin/env bash
# ==============================================================================
#  O-GranulatorInstrument — fork drift check (CONTEXT-2.1 D11, PLAN-2.1 T3,
#  RESEARCH-2.1 §2.2). Run by every Stage 2 verify.
#
#  Reads the leaf list, pin, sha256 columns and class from
#  Source/dsp/forked/FORK-MANIFEST.md and runs two checks per leaf:
#
#   Check 1 — upstream drift (every leaf):
#     sha256 (git show PIN:<parent path>) must equal the manifest's
#     sha256(pin blob) column, AND the parent at HEAD:<path>, AND the parent
#     working-tree file. With --parent-root DIR the parent file is read from
#     DIR/<leaf> instead (HEAD and the working tree are not consulted).
#
#   Check 2 — local fidelity:
#     VERBATIM and "ADAPTED (pending 2.x)": the renamed pin blob
#       (perl: s/\bogran\b/ogi/g; s/OGRAN_TEST_HOOKS/OGI_TEST_HOOKS/g)
#       equals the local file byte for byte.
#     ADAPTED: sha256 of the local file equals the manifest's sha256(local)
#       column, so an unrecorded local edit fires.
#
#  Usage:
#    tests/fork-drift-check.sh [--pin SHA] [--parent-root DIR] [--local-root DIR]
#                              [--manifest FILE]
#      --pin          override the manifest pin column (default: per-row pin)
#      --parent-root  directory holding the parent leaves (a scratch copy of
#                     plugins/O-Granulator/Source/dsp) — negative control NC-a
#      --local-root   directory holding the local leaves (default
#                     Source/dsp/forked) — negative control NC-b
#      --manifest     manifest path (default: the real Source/dsp/forked/
#                     FORK-MANIFEST.md, also when --local-root is given)
#
#  Prints one line per leaf and check, then "N drift". Exit 0 iff N == 0,
#  1 on any drift, 2 on a usage / manifest error. Read-only: never writes to
#  any tree.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${PLUGIN_DIR}/../.." && pwd)"

PIN_OVERRIDE=""
PARENT_ROOT=""
LOCAL_ROOT="${PLUGIN_DIR}/Source/dsp/forked"
MANIFEST="${PLUGIN_DIR}/Source/dsp/forked/FORK-MANIFEST.md"
EXPECTED_LEAVES=13

usage() {
    sed -n '24,36p' "${BASH_SOURCE[0]}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --pin)          PIN_OVERRIDE="${2:?--pin needs a value}"; shift 2 ;;
        --parent-root)  PARENT_ROOT="${2:?--parent-root needs a directory}"; shift 2 ;;
        --local-root)   LOCAL_ROOT="${2:?--local-root needs a directory}"; shift 2 ;;
        --manifest)     MANIFEST="${2:?--manifest needs a file}"; shift 2 ;;
        -h|--help)      usage; exit 0 ;;
        *)              echo "unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

if [[ ! -f "${MANIFEST}" ]]; then
    echo "manifest not found: ${MANIFEST}" >&2
    exit 2
fi
if [[ -n "${PARENT_ROOT}" && ! -d "${PARENT_ROOT}" ]]; then
    echo "--parent-root is not a directory: ${PARENT_ROOT}" >&2
    exit 2
fi
if [[ ! -d "${LOCAL_ROOT}" ]]; then
    echo "--local-root is not a directory: ${LOCAL_ROOT}" >&2
    exit 2
fi

sha256() { shasum -a 256 | awk '{print $1}'; }
rename_pin() { perl -pe 's/\bogran\b/ogi/g; s/OGRAN_TEST_HOOKS/OGI_TEST_HOOKS/g'; }
trim() {
    local s="$1"
    s="${s#"${s%%[![:space:]]*}"}"
    s="${s%"${s##*[![:space:]]}"}"
    printf '%s' "${s}"
}
blob_exists() { git -C "${REPO_ROOT}" cat-file -e "$1" 2>/dev/null; }

drift=0
rows=0

note_drift() {
    drift=$((drift + 1))
    echo "  [DRIFT] $*"
}
note_ok() {
    echo "  [ok]    $*"
}

echo "fork drift check — manifest ${MANIFEST}"
[[ -n "${PARENT_ROOT}" ]] && echo "  parent root override: ${PARENT_ROOT}"
echo "  local root: ${LOCAL_ROOT}"

while IFS= read -r line; do
    [[ "${line}" =~ ^\|[[:space:]]*[A-Za-z0-9_]+\.h[[:space:]]*\| ]] || continue

    IFS='|' read -r _ c_leaf c_path c_pin c_shapin c_shalocal c_class _c_edits _c_phase _rest <<< "${line}"
    leaf="$(trim "${c_leaf}")"
    ppath="$(trim "${c_path}")"
    pin="$(trim "${c_pin}")"
    sha_pin_manifest="$(trim "${c_shapin}")"
    sha_local_manifest="$(trim "${c_shalocal}")"
    class="$(trim "${c_class}")"
    [[ -n "${PIN_OVERRIDE}" ]] && pin="${PIN_OVERRIDE}"
    rows=$((rows + 1))

    echo "${leaf}  (${class}, pin ${pin})"

    # ── Check 1: upstream drift ─────────────────────────────────────────────
    if ! blob_exists "${pin}:${ppath}"; then
        note_drift "check 1 ${leaf}: pin blob ${pin}:${ppath} does not exist"
        continue
    fi
    sha_pin="$(git -C "${REPO_ROOT}" show "${pin}:${ppath}" | sha256)"

    if [[ "${sha_pin_manifest}" == "SHA_PIN" || "${sha_pin_manifest}" != "${sha_pin}" ]]; then
        note_drift "check 1 ${leaf}: manifest sha256(pin blob) '${sha_pin_manifest}' != git ${sha_pin}"
    else
        note_ok "check 1 ${leaf}: pin blob sha256 matches the manifest"
    fi

    if [[ -n "${PARENT_ROOT}" ]]; then
        pfile="${PARENT_ROOT}/${leaf}"
        if [[ ! -f "${pfile}" ]]; then
            note_drift "check 1 ${leaf}: parent-root file missing: ${pfile}"
        else
            sha_p="$(sha256 < "${pfile}")"
            if [[ "${sha_p}" != "${sha_pin}" ]]; then
                changed="$( (diff <(git -C "${REPO_ROOT}" show "${pin}:${ppath}") "${pfile}" || true) | grep -c '^[<>]' || true)"
                note_drift "check 1 ${leaf}: parent-root differs from pin (${changed} changed line(s))"
            else
                note_ok "check 1 ${leaf}: parent-root == pin"
            fi
        fi
    else
        if ! blob_exists "HEAD:${ppath}"; then
            note_drift "check 1 ${leaf}: HEAD:${ppath} does not exist (parent file removed upstream)"
        else
            sha_head="$(git -C "${REPO_ROOT}" show "HEAD:${ppath}" | sha256)"
            if [[ "${sha_head}" != "${sha_pin}" ]]; then
                note_drift "check 1 ${leaf}: parent HEAD differs from pin"
                git -C "${REPO_ROOT}" diff --stat "${pin}" HEAD -- "${ppath}" | sed 's/^/            /' || true
            else
                note_ok "check 1 ${leaf}: parent HEAD == pin"
            fi
        fi

        wt="${REPO_ROOT}/${ppath}"
        if [[ ! -f "${wt}" ]]; then
            note_drift "check 1 ${leaf}: parent working-tree file missing: ${wt}"
        else
            sha_wt="$(sha256 < "${wt}")"
            if [[ "${sha_wt}" != "${sha_pin}" ]]; then
                note_drift "check 1 ${leaf}: parent working tree differs from pin"
                git -C "${REPO_ROOT}" diff --stat "${pin}" -- "${ppath}" | sed 's/^/            /' || true
            else
                note_ok "check 1 ${leaf}: parent working tree == pin"
            fi
        fi
    fi

    # ── Check 2: local fidelity ─────────────────────────────────────────────
    lfile="${LOCAL_ROOT}/${leaf}"
    if [[ ! -f "${lfile}" ]]; then
        note_drift "check 2 ${leaf}: local file missing: ${lfile}"
        continue
    fi

    case "${class}" in
        "VERBATIM"|"ADAPTED (pending "*")")
            if cmp -s <(git -C "${REPO_ROOT}" show "${pin}:${ppath}" | rename_pin) "${lfile}"; then
                note_ok "check 2 ${leaf}: local == renamed pin blob (byte for byte)"
            else
                changed="$( (diff <(git -C "${REPO_ROOT}" show "${pin}:${ppath}" | rename_pin) "${lfile}" || true) | grep -c '^[<>]' || true)"
                note_drift "check 2 ${leaf}: local differs from the renamed pin blob (${changed} changed line(s))"
            fi
            ;;
        "ADAPTED")
            sha_l="$(sha256 < "${lfile}")"
            if [[ "${sha_local_manifest}" == "SHA_LOCAL" || "${sha_local_manifest}" != "${sha_l}" ]]; then
                note_drift "check 2 ${leaf}: local sha256 ${sha_l} != manifest sha256(local) '${sha_local_manifest}' (unrecorded edit?)"
            else
                note_ok "check 2 ${leaf}: local sha256 matches the manifest (recorded adaptation)"
            fi
            ;;
        *)
            note_drift "check 2 ${leaf}: unknown class '${class}'"
            ;;
    esac
done < "${MANIFEST}"

if [[ "${rows}" -eq 0 ]]; then
    echo "no leaf rows parsed from ${MANIFEST}" >&2
    exit 2
fi
if [[ "${rows}" -ne "${EXPECTED_LEAVES}" ]]; then
    note_drift "manifest lists ${rows} leaves, expected ${EXPECTED_LEAVES}"
fi

echo
echo "${rows} leaves checked"
echo "${drift} drift"
if [[ "${drift}" -ne 0 ]]; then
    exit 1
fi
exit 0
