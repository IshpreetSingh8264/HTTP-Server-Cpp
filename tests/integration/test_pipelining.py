"""HTTP pipelining, keep-alive and concurrency.

These are the tests that exist because of a bug that already happened: the old
read loop appended whole 8192-byte buffer reads, so a request body larger
than the buffer swallowed the bytes of the request pipelined behind it. The
assertion that catches it is "a 20 KB body and the next request in a single
write() come back as two clean responses" -- a per-request read cannot produce
that, and a buffer-appending one cannot either.
"""
import concurrent.futures

import harness as h


def pipeline(name, payloads, want_codes, want_bodies):
    """Put every request in ONE write() and read everything back."""
    raw = h.send_raw(b"".join(payloads))
    got = h.responses(raw)
    codes = [h.status_code(s) for s, _, _ in got]
    ok = codes == want_codes and [b for _, _, b in got] == want_bodies
    print(f"       {name}: {len(raw)} bytes back, "
          f"{len(got)} response(s), statuses={codes} -> {'PASS' if ok else 'FAIL'}")
    return raw, got


def run(r):
    r.section("pipelining: several requests in a single write")

    small = b"XXXXBODY"
    _, got = pipeline(
        "POST then GET, one write",
        [b"POST /files/p1.txt HTTP/1.1\r\nHost: x\r\nContent-Length: "
         + str(len(small)).encode() + b"\r\nConnection: keep-alive\r\n\r\n" + small,
         b"GET /files/p1.txt HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"],
        [201, 200], [b"", small])
    r.check("a pipelined POST then GET returns [201, 200]",
            [h.status_code(s) for s, _, _ in got], [201, 200])
    r.check("the POSTed body reads back through the pipelined GET",
            got[1][2] if len(got) == 2 else None, small)

    # 20000 bytes is more than the old 8192-byte read buffer, so this is the
    # case that used to glue request 2 into request 1's body.
    big = bytes((i % 251) for i in range(20000))
    _, got = pipeline(
        "20KB POST (spans several recvs) then GET, one write",
        [b"POST /files/p2.bin HTTP/1.1\r\nHost: x\r\nContent-Length: "
         + str(len(big)).encode() + b"\r\nConnection: keep-alive\r\n\r\n" + big,
         b"GET /files/p2.bin HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"],
        [201, 200], [b"", big])
    r.check("a 20KB pipelined body does not absorb the next request",
            [h.status_code(s) for s, _, _ in got], [201, 200])
    r.check("the 20KB body round-trips byte-identical",
            got[1][2] if len(got) == 2 else None, big)

    _, got = pipeline(
        "three GETs, no bodies",
        [b"GET /echo/one HTTP/1.1\r\nHost: x\r\nConnection: keep-alive\r\n\r\n",
         b"GET /echo/two HTTP/1.1\r\nHost: x\r\nConnection: keep-alive\r\n\r\n",
         b"GET /echo/three HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"],
        [200, 200, 200], [b"one", b"two", b"three"])
    r.check("three pipelined GETs return three bodies in order",
            [b for _, _, b in got], [b"one", b"two", b"three"])

    r.section("keep-alive: many requests down one connection")
    reqs = b"".join(
        b"GET /echo/" + str(i).encode() + b" HTTP/1.1\r\nHost: x\r\n"
        b"Connection: keep-alive\r\n\r\n" for i in range(20))
    raw = h.send_raw(reqs + b"GET /echo/last HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
    got = h.responses(raw)
    r.check("21 keep-alive requests on one connection all answered", len(got), 21)
    r.check("the last keep-alive response is the last request",
            got[-1][2] if got else None, b"last")
    r.check("every keep-alive response is 200",
            sorted({h.status_code(s) for s, _, _ in got}), [200])

    r.section("concurrency")
    def one(i):
        status, _, _ = h.split_response(h.get(b"/echo/c%d" % i))
        return h.status_code(status)

    with concurrent.futures.ThreadPoolExecutor(max_workers=10) as pool:
        codes = list(pool.map(one, range(10)))
    r.check("10 concurrent requests all return 200", codes, [200] * 10)
