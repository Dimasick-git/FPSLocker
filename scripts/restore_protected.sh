#!/usr/bin/env bash
# Restore the Ryazhenka-protected paths from the target branch after a merge.
#
# Usage: restore_protected.sh <target-branch>
#
# The list lives in scripts/protected_paths.txt (one path / pattern per line,
# lines beginning with # are comments). Every entry is `git checkout`-ed out of
# the target branch so any upstream changes in those paths are dropped.

set -euo pipefail

TARGET_BRANCH="${1:-main}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIST="$ROOT/scripts/protected_paths.txt"

if [ ! -f "$LIST" ]; then
  echo "[restore_protected] no protected list at $LIST — nothing to do"
  exit 0
fi

cd "$ROOT"

# Make sure we have the target branch ref available.
if ! git rev-parse --verify --quiet "origin/${TARGET_BRANCH}" >/dev/null; then
  git fetch origin "${TARGET_BRANCH}" --depth=0 || true
fi
REF="origin/${TARGET_BRANCH}"
if ! git rev-parse --verify --quiet "$REF" >/dev/null; then
  REF="$TARGET_BRANCH"
fi

changed=0
while IFS= read -r raw; do
  pattern="${raw%%#*}"
  pattern="$(echo "$pattern" | sed -E 's/^[[:space:]]+|[[:space:]]+$//g')"
  [ -z "$pattern" ] && continue

  echo "[restore_protected] restoring '$pattern' from $REF"
  if git checkout "$REF" -- "$pattern" 2>/dev/null; then
    changed=1
  else
    # Pattern may not exist upstream yet — try to restore from index of target
    # branch using ls-tree, otherwise skip.
    if git ls-tree -r --name-only "$REF" -- "$pattern" >/dev/null 2>&1; then
      :
    else
      echo "[restore_protected]   (skipped: not present on $REF)"
    fi
  fi
done < "$LIST"

if [ "$changed" -eq 1 ]; then
  echo "[restore_protected] done — protected paths reset to $REF"
else
  echo "[restore_protected] nothing changed"
fi
