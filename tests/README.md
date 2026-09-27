# Tests

Local test infrastructure for the HTTP server. No network, no test framework —
`cmake`, a C++23 compiler, `python3` and `zlib` (via python) are all you need.

## Run everything

```sh
./tests/run.sh
```

Configures, builds, runs every suite, exits non-zero if anything fails.
**150 assertions**: 73 unit + 77 behavioural.

| Command | What it runs |
|---|---|
| `./tests/run.sh` | build + all suites |
| `./tests/run.sh unit` | the three C++ unit binaries |
| `./tests/run.sh integration` | the behavioural suite (needs port 4221 free) |

`BUILD_DIR` and `PYTHON` are honoured if set. `ctest` also works:

```sh
cmake -B build -S . && cmake --build build && ctest --test-dir build --output-on-failure
```

## What is here

```
tests/
  run.sh                             the entrypoint
  check_size.py                      fails if a suite file passes 500 lines
  unit/
    route_matcher_test.cpp           12 assertions
    negotiation_test.cpp             35 assertions
    string_utils_test.cpp            26 assertions
  integration/
    harness.py                       boots the server, raw-socket HTTP helpers
    test_routing.py                  routing, methods, files, malformed input
    test_compression.py              negotiation over the wire + zlib round-trip
    test_pipelining.py               pipelining, keep-alive, concurrency
    server_behaviour_test.py         the integration entrypoint
```

### `unit/` — 73 assertions against the real code

Each binary links the same static library the server does, so there is one copy
of the code under test rather than a test-local copy that can rot.

- **`route_matcher_test.cpp` (12)** — `pathMatches` against the real route
  table. Weighted towards the negative and boundary cases: a single-segment
  parameter must not swallow a slash, a multi-segment parameter must not match
  empty, and a literal route must not match a longer path that merely shares
  its prefix (`/user-agent` vs `/user-agency`). Those are what an if-chain on
  substrings gets wrong and what the course never probes.

- **`negotiation_test.cpp` (35)** — `EncodingNegotiator::negotiate` over
  whitespace, casing, q-values, `q=0` (explicitly *not* acceptable), `*`,
  unknown codings, `x-gzip`, empty elements and a malformed `q=bogus`. Plus the
  encoder: that gzip really carries the `1f 8b` magic, that deflate is a
  **raw** stream with no zlib header, and that gzip is exactly 18 bytes larger
  than the same deflate stream.

- **`string_utils_test.cpp` (26)** — the parsing helpers. The load-bearing
  behaviour is that `split()` **keeps empty tokens**: the request line is
  space-delimited and `GET  /x HTTP/1.1` has a double space, so a split that
  collapses empty tokens shifts every field by one and the request line parses
  as garbage. Also `endsWith` on the `{name...}` brace form, `urlDecode` for
  both `%20` and `+`, and `substringBefore` with no delimiter present.

### `integration/` — 77 assertions against a running server

`harness.py` boots the binary on a scratch directory and tears it down. It
**refuses to start if port 4221 is already busy** rather than sharing it: the
server hardcodes that port and cannot be moved, so a stale instance from a
previous run would answer a share of the requests and the suite would fail —
or worse, pass — for reasons unrelated to the code.

All HTTP is done with raw sockets, deliberately **not** `curl` or a client
library. The pipelining tests need to put several requests in one `write()` and
read the raw bytes back, and a client library will not let you.

**`test_routing.py`** — the routes, the method table, file serving, and
malformed input. Notable: a non-numeric, negative, absurd or space-padded
`Content-Length` must each produce a response rather than killing the
connection, and the server must still be serving afterwards (asserted at the
end of the suite). `HEAD` is probed as a real `HEAD` — sending `-X HEAD` makes
a client wait for a body a correct server is required not to send, so it is
not a valid probe.

**`test_compression.py`** — the negotiated encoding has to reach the wire, and
the bytes have to be *valid*. Every compressed body is inflated back with
python's `zlib` and compared to the original file, because a server can set a
perfectly correct `Content-Encoding` header and still emit garbage. gzip is
inflated with `wbits=16+MAX_WBITS`, deflate with `-MAX_WBITS` (raw); using
the default `wbits` for deflate is the single most common way to get RFC 1951
vs RFC 1950 wrong. The already-compressed media types (`image/`, `video/`,
`audio/`, `application/zip`, `application/gzip`) are asserted *not* to be
compressed.

Note that compression is decided by **media type only**, never by whether the
bytes look compressible. `application/octet-stream` is not on the skip list, so
`random.bin` *is* compressed and still round-trips. That is the documented
policy, and the suite asserts it rather than the intuition.

**`test_pipelining.py`** — the suite that earns its keep. The old read loop
appended whole 8192-byte buffer reads, so a body larger than the buffer
swallowed the bytes of the request pipelined behind it. The assertion that
catches it is "a 20 KB body and the next request in a single `write()` come
back as two clean responses with the 20 KB byte-identical". Also: three
pipelined GETs return their bodies in order, 21 keep-alive requests on one
connection are all answered, a `GET` pipelined before a `HEAD` frames
correctly and the `HEAD` reports the `GET`'s `Content-Length` with no body, and
10 concurrent connections all return 200.

## What this does NOT cover

- **`codecrafters test` is still the only authority.** This suite is a fast
  local proxy. It cannot be submitted, and `run.sh` passing does not mean the
  stages pass.
- **No TLS, no `Range`/206, no `multipart/form-data`, no chunked
  `Transfer-Encoding`.** These are deliberately absent from the implementation
  (see *Known limitations* in `.github/copilot-instructions.md`) and the suite
  does not assert that they are rejected either.
- **A zero-byte file under `/files/` returns 500** and that behaviour is
  **deliberately not asserted**. It is a known limitation, not a contract.
- **Only port 4221, only loopback, only HTTP/1.1.** No HTTP/2, no upgrade
  handling, no keep-alive *timeout* behaviour, no `Expect: 100-continue`, and
  no test that the server actually closes an idle connection.
- **Response-header *values* are barely checked.** Status codes, `Content-Length`
  and `Content-Encoding` are asserted. `Content-Type`, `Date`, `Connection` and
  header *ordering* and *casing* are not — so a regression that reorders or
  recases headers would pass.
- **The `ThreadPool` is not directly unit-tested.** Concurrency is only
  exercised as "10 clients all get 200". There is no test of queue depth,
  task rejection, or worker lifetime.
- **No timeout or slow-client coverage.** No test that a client which stops
  mid-body is handled, and no test of the read timeout.
- **No load or soak testing.** 10 concurrent connections is not a throughput
  claim, and nothing measures latency.
- **`Logger` output is not asserted.** The suite runs the server at `ERROR`
  level and ignores stderr entirely — including the "INFO flood" check the old
  scratch harness did. Logging regressions are invisible here.
- **`ConnectionHandler`, `FileHandler` and `HttpRequest` have no direct unit
  tests.** They are covered only indirectly, through the socket suite.
