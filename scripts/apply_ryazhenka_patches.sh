#!/usr/bin/env bash
# Re-apply all Ryazhenka customisations on top of the current working tree.
#
# Run this after a merge from upstream (masagrator/FPSLocker) to keep the fork
# consistent with the libryazhahand ecosystem. The script is idempotent — running
# it twice is safe.
#
# What it does:
#   1. Renames any libultrahand / Ultrahand references in Makefile and docs to
#      libryazhahand / Ryazhahand variants.
#   2. Forces the .ovl signature from 'ULTR' to 'RYZH'.
#   3. Keeps APP_VERSION in sync with .ryazhenka-version (default 3.3.3).
#   4. Restores the `include ${TOPDIR}/libs/libryazhahand/ryazhahand.mk` line
#      if upstream replaced it with the masagrator (libtesla) Makefile.
#   5. Ensures .gitmodules points at the libryazhahand submodule.
#
# Protected files (.github/, scripts/, docs/, README.md, etc.) are not touched
# directly — they are restored from the local branch by restore_protected.sh
# before this script runs.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION_FILE="$ROOT/.ryazhenka-version"
if [ -f "$VERSION_FILE" ]; then
  RYAZH_VERSION="$(tr -d '[:space:]' < "$VERSION_FILE")"
else
  RYAZH_VERSION="3.3.3"
fi
echo "[ryazhenka] target version : ${RYAZH_VERSION}"

# ---------------------------------------------------------------------------
# 1) Makefile: APP_VERSION + libryazhahand include + RYZH signature
# ---------------------------------------------------------------------------
if [ -f Makefile ]; then
  echo "[ryazhenka] patching Makefile"

  # APP_VERSION
  sed -i -E "s|^(APP_VERSION[[:space:]]*:=[[:space:]]*).*|\1${RYAZH_VERSION}|" Makefile

  # Replace any masagrator libtesla include with libryazhahand
  sed -i -E 's|libs/libtesla/include|libs/libryazhahand/libtesla/include|g' Makefile

  # Switch ultrahand include line to libryazhahand if not already
  if grep -qE 'libs/libultrahand/ultrahand\.mk' Makefile; then
    sed -i -E 's|libs/libultrahand/ultrahand\.mk|libs/libryazhahand/ryazhahand.mk|g' Makefile
  fi

  # Add the include block if it was stripped by upstream (masa Makefile has none).
  if ! grep -q 'libs/libryazhahand' Makefile; then
    echo "[ryazhenka]   restoring libryazhahand include block"
    python3 - <<'PY'
import re, sys, pathlib
p = pathlib.Path("Makefile")
src = p.read_text()
block = (
    "\n# Ryazhahand ecosystem: libryazhahand provides ryazhahand.mk\n"
    "# (with backwards-compatible fallback to ultrahand.mk if user pulled an older snapshot)\n"
    "ifneq ($(wildcard ${TOPDIR}/libs/libryazhahand/ryazhahand.mk),)\n"
    "include ${TOPDIR}/libs/libryazhahand/ryazhahand.mk\n"
    "else\n"
    "ifneq ($(wildcard ${TOPDIR}/libs/libryazhahand/ultrahand.mk),)\n"
    "include ${TOPDIR}/libs/libryazhahand/ultrahand.mk\n"
    "endif\n"
    "endif\n"
)
# Insert right after the NO_ICON / ICON definitions, before ARCH section.
m = re.search(r'^NO_ICON.*$', src, flags=re.M)
if m:
    insert_at = m.end()
    src = src[:insert_at] + "\n" + block + src[insert_at:]
else:
    src = block + src
p.write_text(src)
PY
  fi

  # Signature: ULTR -> RYZH
  sed -i -E "s|@printf 'ULTR'|@printf 'RYZH'|g" Makefile
  sed -i -E "s|Ultrahand signature has been added\.|Ryazhahand signature has been added.|g" Makefile

  # If upstream stripped the signature step entirely (masagrator Makefile has none),
  # re-add it to the .ovl link rule.
  if ! grep -q "RYZH" Makefile; then
    echo "[ryazhenka]   re-injecting RYZH signature into elf2nro rule"
    python3 - <<'PY'
import re, pathlib
p = pathlib.Path("Makefile")
src = p.read_text()
# Insert after the elf2nro line in the $(OUTPUT).ovl rule.
pat = re.compile(r'(\t@elf2nro \$< \$@ \$\(NROFLAGS\)\n)')
sig = "\t@printf 'RYZH' >> $@\n\t@printf \"Ryazhahand signature has been added.\\n\"\n"
if pat.search(src) and "RYZH" not in src:
    src = pat.sub(r'\1' + sig, src, count=1)
    p.write_text(src)
PY
  fi
fi

# ---------------------------------------------------------------------------
# 2) .gitmodules: ensure libryazhahand submodule entry
# ---------------------------------------------------------------------------
cat > .gitmodules <<'EOF'
[submodule "libs/libryazhahand"]
	path = libs/libryazhahand
	url = https://github.com/dimasick-git/libryazhahand.git
	branch = main
EOF
echo "[ryazhenka] wrote .gitmodules"

# ---------------------------------------------------------------------------
# 3) Strip any leftover libultrahand submodule directory introduced by merge
# ---------------------------------------------------------------------------
if [ -d libs/libultrahand ]; then
  echo "[ryazhenka] removing stray libs/libultrahand directory"
  rm -rf libs/libultrahand
fi
if [ -d libs/libtesla ]; then
  # masa fork ships its own libtesla submodule — drop it; libryazhahand bundles libtesla.
  echo "[ryazhenka] removing stray libs/libtesla directory"
  rm -rf libs/libtesla
fi

# ---------------------------------------------------------------------------
# 4) Ensure include source for tester/ stays self-contained — no rewrites here.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 5) Soft text substitutions in unprotected docs (translation tables etc.)
#    NOTE: README.md is protected by restore_protected.sh, so this only
#    touches stray .md/.txt files imported from upstream.
# ---------------------------------------------------------------------------
find . -type f \
  \( -name "*.md" -o -name "*.txt" \) \
  -not -path "./.git/*" \
  -not -path "./libs/*" \
  -not -path "./.github/*" \
  -not -path "./docs/*" \
  -not -path "./scripts/*" \
  -not -name "README.md" \
  -print0 |
  while IFS= read -r -d '' f; do
    sed -i -E '
      s|libultrahand|libryazhahand|g;
      s|Ultrahand-Overlay|Ryazhahand-Overlay|g;
    ' "$f" || true
  done

echo "[ryazhenka] done."
