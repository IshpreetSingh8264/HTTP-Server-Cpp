#!/usr/bin/env bash
#
# The one command that runs every local test in this repo.
#
#   ./tests/run.sh              # build, then run everything
#   ./tests/run.sh unit         # just the 73 C++ unit assertions
#   ./tests/run.sh integration  # just the 77 behavioural assertions
#
# The integration suite needs port 4221, which the server hardcodes and cannot
# be moved. If something is already listening there the suite refuses to run
# rather than silently sharing the port with a stale server.
#
# Exits non-zero if any suite fails.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO/build}"
PY="${PYTHON:-python3}"

cd "$REPO"

ALL="unit integration"
SUITES="$ALL"
case "${1:-}" in
  unit|integration) SUITES="$1"; shift ;;
esac

CMAKE_ARGS=(-B "$BUILD_DIR" -S .)
if [ -n "${VCPKG_ROOT:-}" ] && [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
  CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
fi

want() {
  case " $SUITES " in *" $1 "*) return 0 ;; *) return 1 ;; esac
}

echo "== configure =="
cmake "${CMAKE_ARGS[@]}" >/dev/null
echo "== build =="
cmake --build "$BUILD_DIR" >/dev/null

rc=0

if want unit; then
  for t in route_matcher negotiation string_utils; do
    echo
    echo "== unit: $t =="
    "$BUILD_DIR/${t}_test" || rc=1
  done
fi

if want integration; then
  echo
  echo "== integration: server behaviour =="
  "$PY" tests/integration/server_behaviour_test.py "$BUILD_DIR/http-server" || rc=1
fi

echo
echo "== suite size budget =="
"$PY" tests/check_size.py tests || rc=1

echo
if [ $rc -eq 0 ]; then
  echo "ALL SUITES PASSED"
else
  echo "SOME SUITES FAILED"
fi
exit $rc
