"""Routing, methods, file serving and error handling over a real socket."""
import harness as h


def run(r):
    r.section("routing")
    status, headers, body = h.split_response(h.get(b"/"))
    r.check("GET / returns 200", h.status_code(status), 200)
    # The root route deliberately returns an empty body. A zero-length body is
    # still framable, and Content-Length: 0 is what lets the client know that.
    r.check("GET / sends Content-Length: 0", headers.get("content-length"), "0")
    r.check("GET / has an empty body", body, b"")

    status, _, body = h.split_response(h.get(b"/echo/abc"))
    r.check("GET /echo/abc returns 200", h.status_code(status), 200)
    r.check("GET /echo/abc echoes the path segment", body, b"abc")

    # A single-segment parameter must not swallow a slash. If it did, this
    # would be 200 with a body instead of 404.
    status, _, _ = h.split_response(h.get(b"/echo/a/b"))
    r.check("GET /echo/a/b does not match /echo/{str}", h.status_code(status), 404)

    status, _, body = h.split_response(h.get(b"/user-agent", headers=[b"User-Agent: probe/1.0"]))
    r.check("GET /user-agent returns 200", h.status_code(status), 200)
    r.check("GET /user-agent echoes the header", body, b"probe/1.0")

    # A literal route must not match a longer path that shares its prefix.
    status, _, _ = h.split_response(h.get(b"/user-agency"))
    r.check("GET /user-agency is not /user-agent", h.status_code(status), 404)

    r.section("files")
    status, headers, body = h.split_response(h.get(b"/files/sample.txt"))
    r.check("GET /files/sample.txt returns 200", h.status_code(status), 200)
    r.check("the file body is byte-identical", body, h.SAMPLE_TEXT)
    r.check("Content-Length matches the body", headers.get("content-length"),
            str(len(h.SAMPLE_TEXT)))
    r.check_true("a Content-Type is sent", "content-type" in headers, str(headers))

    status, _, body = h.split_response(h.get(b"/files/random.bin"))
    r.check("GET /files/random.bin returns 200", h.status_code(status), 200)
    r.check("incompressible bytes survive byte-identical", body, h.RANDOM_BYTES)

    status, _, _ = h.split_response(h.get(b"/files/missing.txt"))
    r.check("GET /files/missing.txt returns 404", h.status_code(status), 404)

    status, _, _ = h.split_response(h.get(b"/nope"))
    r.check("GET /nope returns 404", h.status_code(status), 404)

    r.section("POST /files")
    payload = b"posted-content"
    status, _, _ = h.split_response(
        h.get(b"/files/newfile.txt", method=b"POST", body=payload))
    r.check("POST /files/newfile.txt returns 201", h.status_code(status), 201)
    status, _, body = h.split_response(h.get(b"/files/newfile.txt"))
    r.check("the posted file reads back", body, payload)

    r.section("methods")
    for method, want in ((b"PUT", 405), (b"DELETE", 405), (b"OPTIONS", 405)):
        status, _, _ = h.split_response(
            h.get(b"/echo/abc", method=method))
        r.check(f"{method.decode()} on a read-only route is 405",
                h.status_code(status), want)

    # HEAD must be probed as HEAD, not as `-X HEAD`: a correct server sends no
    # body for it, so a client that waits for one hangs.
    raw = h.send_raw(b"HEAD /echo/abc HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
    head, _, body = raw.partition(b"\r\n\r\n")
    r.check("HEAD /echo/abc returns 200", h.status_code(head.decode(errors="replace")), 200)
    r.check("HEAD sends no body", len(body), 0)
    r.check_true("HEAD still sends Content-Length",
                 b"Content-Length" in head, head.decode(errors="replace"))

    # A GET and a HEAD pipelined into one connection. The first must be
    # keep-alive, otherwise the server is entitled to close after it and there
    # is nothing to pipeline behind.
    raw = h.send_raw(b"GET /echo/abc HTTP/1.1\r\nHost: x\r\nConnection: keep-alive\r\n\r\n"
                     b"HEAD /echo/abc HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
    got = h.responses(raw)
    r.check("a pipelined GET then HEAD yields 2 responses", len(got), 2)
    if len(got) == 2:
        r.check("the pipelined GET is 200", h.status_code(got[0][0]), 200)
        r.check("the pipelined GET carries the body", got[0][2], b"abc")
        r.check("the pipelined HEAD is 200", h.status_code(got[1][0]), 200)
        r.check("the pipelined HEAD has no body", len(got[1][2]), 0)
        r.check("the pipelined HEAD reports the GET's Content-Length",
                got[1][1].get("content-length"), got[0][1].get("content-length"))

    r.section("malformed requests")
    # A Content-Length that is not a number must be rejected, not crash the
    # server -- and the server must still be alive afterwards.
    raw = h.send_raw(b"POST /files/x.txt HTTP/1.1\r\nHost: x\r\n"
                     b"Content-Length: abc\r\nConnection: close\r\n\r\nhello")
    r.check("a non-numeric Content-Length is rejected with 400",
            h.status_code(raw.decode(errors="replace")), 400)

    raw = h.send_raw(b"GET / HTTP/1.1\r\nHost: x\r\nthis-header-has-no-colon\r\n"
                     b"Connection: close\r\n\r\n")
    r.check_true("a header with no colon does not kill the connection",
                 raw.startswith(b"HTTP/1.1 "), raw[:60].decode(errors="replace"))

    for name, cl in (("negative", b"-5"), ("absurd", b"99999999999999999999"),
                     ("padded", b"  12  ")):
        raw = h.send_raw(b"POST /files/n.txt HTTP/1.1\r\nHost: x\r\nContent-Length: "
                         + cl + b"\r\nConnection: close\r\n\r\nhello world!")
        r.check_true(f"a {name} Content-Length does not crash the server",
                     raw.startswith(b"HTTP/1.1 "), raw[:60].decode(errors="replace"))

    r.section("the server survived all of that")
    status, _, _ = h.split_response(h.get(b"/"))
    r.check("GET / still returns 200 at the end", h.status_code(status), 200)
