"""Content negotiation end to end, and a real zlib round-trip.

The unit suite checks EncodingNegotiator in isolation. This checks that the
negotiated encoding actually reaches the wire, and -- more importantly -- that
the bytes on the wire are a *valid* gzip container and a *valid* raw deflate
stream that a standard library can inflate back to the original file. A server
can set the right `Content-Encoding` header and still emit garbage; only
inflating it proves anything.
"""
import zlib

import harness as h

# (Accept-Encoding value or None, expected Content-Encoding or None)
NEGOTIATION = [
    (None, None),
    (b"identity", None),
    (b"br", None),
    (b"gzip", "gzip"),
    (b"deflate", "deflate"),
    (b"GZIP", "gzip"),
    (b"  Deflate  ", "deflate"),
    (b"gzip, deflate", "gzip"),
    (b"deflate, gzip", "deflate"),
    (b"*", "gzip"),
    (b"gzip;q=0.5, deflate;q=0.9", "deflate"),
    (b"gzip;q=0.9, deflate;q=0.5", "gzip"),
    (b"gzip;q=0", None),
    (b"gzip;q=0, deflate", "deflate"),
]


def fetch(ae=None, path=b"/files/sample.txt"):
    headers = [] if ae is None else [b"Accept-Encoding: " + ae]
    return h.split_response(h.get(path, headers=headers))


def run(r):
    r.section("Accept-Encoding negotiation over the wire")
    for ae, want in NEGOTIATION:
        label = "(absent)" if ae is None else ae.decode()
        _, headers, _ = fetch(ae)
        got = headers.get("content-encoding")
        r.check(f"Accept-Encoding: {label} -> {want}", got, want)

    r.section("a compressed response is smaller than the original")
    _, plain_headers, plain_body = fetch(None)
    _, gzip_headers, gzip_body = fetch(b"gzip")
    _, defl_headers, defl_body = fetch(b"deflate")
    r.check("the uncompressed body is the file", plain_body, h.SAMPLE_TEXT)
    r.check("gzip compressed it", len(gzip_body) < len(h.SAMPLE_TEXT), True)
    r.check("deflate compressed it", len(defl_body) < len(h.SAMPLE_TEXT), True)
    r.check("Content-Length agrees with the gzip body",
            gzip_headers.get("content-length"), str(len(gzip_body)))

    r.section("zlib round-trip: the bytes must actually inflate")
    # gzip is a zlib deflate stream inside a gzip container -> wbits 16+MAX.
    try:
        back = zlib.decompress(gzip_body, 16 + zlib.MAX_WBITS)
        r.check("gzip inflates back to the original", back, h.SAMPLE_TEXT)
    except zlib.error as e:
        r.check_true("gzip inflates back to the original", False, f"zlib.error: {e}")
    r.check("gzip starts with the 1f 8b magic",
            gzip_body[:2], b"\x1f\x8b")

    # deflate is a RAW deflate stream (RFC 1951) -- negative wbits. Using the
    # default wbits here is the single most common way to get this wrong.
    try:
        back = zlib.decompressobj(-zlib.MAX_WBITS).decompress(defl_body)
        r.check("raw deflate inflates back to the original", back, h.SAMPLE_TEXT)
    except zlib.error as e:
        r.check_true("raw deflate inflates back to the original", False, f"zlib.error: {e}")
    r.check_true("deflate has no zlib/gzip header",
                 defl_body[:2] != b"\x1f\x8b" and defl_body[:1] != b"\x78",
                 f"starts {defl_body[:4]!r}")

    r.section("gzip is exactly deflate plus the 18-byte container")
    # Both are zlib streams over identical input, so the only difference is the
    # gzip header+trailer. If this drifts, something is re-compressing.
    _, _, defl2 = fetch(b"deflate")
    r.check("gzip is 18 bytes larger than the same deflate stream",
            len(gzip_body) - len(defl2), 18)

    r.section("the already-compressed media types are skipped")
    # The policy skips image/, video/, audio/, application/zip and
    # application/gzip. random.bin is application/octet-stream, which is NOT
    # on that list, so it is compressed -- compression is decided by media type,
    # never by "does this look random".
    _, headers, body = fetch(b"gzip", path=b"/files/random.bin")
    r.check("an octet-stream body is compressed, because its type is not skipped",
            headers.get("content-encoding"), "gzip")
    try:
        r.check("the octet-stream body still inflates to the original",
                zlib.decompress(body, 16 + zlib.MAX_WBITS), h.RANDOM_BYTES)
    except zlib.error as e:
        r.check_true("the octet-stream body still inflates to the original", False, str(e))

    _, headers, body = fetch(b"gzip", path=b"/files/pic.png")
    r.check("an image/png body is NOT compressed", headers.get("content-encoding"), None)
    r.check("the image is served byte-identical", body, h.PNG_BYTES)

    r.section("compressed responses survive pipelining")
    # A compressed body must not bleed its length into the next response.
    raw = h.send_raw(
        b"GET /files/sample.txt HTTP/1.1\r\nHost: x\r\nAccept-Encoding: gzip\r\n"
        b"Connection: keep-alive\r\n\r\n"
        b"GET /echo/second HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
    got = h.responses(raw)
    r.check("a gzip GET then a plain GET yields 2 responses", len(got), 2)
    if len(got) == 2:
        r.check("the first response advertises gzip",
                got[0][1].get("content-encoding"), "gzip")
        try:
            r.check("the first response inflates to the original",
                    zlib.decompress(got[0][2], 16 + zlib.MAX_WBITS), h.SAMPLE_TEXT)
        except zlib.error as e:
            r.check_true("the first response inflates to the original", False, str(e))
        r.check("the second response body is not corrupted", got[1][2], b"second")
