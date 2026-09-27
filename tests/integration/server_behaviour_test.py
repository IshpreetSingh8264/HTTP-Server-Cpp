#!/usr/bin/env python3
"""The HTTP server integration entrypoint.

Boots the server once on a scratch directory and runs the three behaviour
suites against it. Exits non-zero if any assertion fails.

Usage: server_behaviour_test.py <path-to-http-server-binary>
"""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import harness  # noqa: E402
import test_compression  # noqa: E402
import test_pipelining  # noqa: E402
import test_routing  # noqa: E402

SUITES = [
    ("routing / methods / files / errors", test_routing.run),
    ("content negotiation and zlib round-trip", test_compression.run),
    ("pipelining / keep-alive / concurrency", test_pipelining.run),
]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    r = harness.Results()
    with harness.ServerFixture(sys.argv[1]):
        for title, suite in SUITES:
            r.section(title)
            suite(r)
    print()
    return 1 if r.summary() else 0


if __name__ == "__main__":
    sys.exit(main())
