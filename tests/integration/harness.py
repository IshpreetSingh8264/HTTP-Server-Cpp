"""Shared plumbing for the HTTP server integration suites.

The server hardcodes port 4221 with no override, which creates a hazard the
suites have to defend against: a stale server left over from a previous run
would answer a share of the requests, and the suite would then fail (or worse,
pass) for reasons that have nothing to do with the code. `ServerFixture`
refuses to start if the port is already busy, rather than silently sharing it.
"""
import os
import pathlib
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

PORT = 4221
HOST = "127.0.0.1"

# A fixed, highly compressible body. Compression sizes are only meaningful
# against something that actually compresses, and 400 bytes of 'A' lets us
# assert "much smaller than the input" without being flaky.
SAMPLE_TEXT = b"A" * 400
RANDOM_BYTES = bytes((i * 37 + 11) % 256 for i in range(200))
# A file whose media type the encoder deliberately refuses to compress. The
# policy skips image/, video/, audio/, application/zip and application/gzip,
# so the extension has to be one of those for the skip path to be reachable.
PNG_BYTES = b"\x89PNG\r\n\x1a\n" + b"not really a png, but it is served as image/png" * 8


class Results:
    """Counts assertions and keeps going after a failure, so one run reports
    every problem instead of only the first."""

    def __init__(self):
        self.passed = 0
        self.failed = 0

    def check(self, what, got, want):
        if got == want:
            self.passed += 1
            print(f"ok   {what}")
        else:
            self.failed += 1
            print(f"FAIL {what}\n       got  {got!r}\n       want {want!r}")
        return got == want

    def check_true(self, what, cond, detail=""):
        return self.check(what, bool(cond), True) if cond else self._bad(what, detail)

    def _bad(self, what, detail):
        self.failed += 1
        print(f"FAIL {what}\n       {detail}")
        return False

    def check_in(self, what, needle, haystack):
        return self.check(what, needle in haystack, True) if needle in haystack \
            else self._bad(what, f"{needle!r} not found in {haystack!r}")

    def check_not_in(self, what, needle, haystack):
        return self.check(what, needle in haystack, False) if needle not in haystack \
            else self._bad(what, f"{needle!r} unexpectedly present")

    def section(self, title):
        print(f"\n--- {title} ---")

    def summary(self):
        print()
        print("==================================================")
        print(f"TOTAL: pass={self.passed} fail={self.failed}")
        return self.failed


def port_is_busy():
    with socket.socket() as s:
        s.settimeout(0.5)
        return s.connect_ex((HOST, PORT)) == 0


class ServerFixture:
    """Boots the server on a scratch directory and tears it down."""

    def __init__(self, binary):
        self.binary = str(pathlib.Path(binary).resolve())
        self.dir = pathlib.Path(tempfile.mkdtemp(prefix="httpserver-test-"))
        self.proc = None
        self.stderr = None

    def __enter__(self):
        if port_is_busy():
            sys.exit(
                f"FATAL: port {PORT} is already in use.\n"
                f"       The server hardcodes that port and cannot be moved, so a\n"
                f"       stale instance would answer a share of these requests.\n"
                f"       Find it with:  ss -tlnp | grep {PORT}"
            )
        (self.dir / "sample.txt").write_bytes(SAMPLE_TEXT)
        (self.dir / "random.bin").write_bytes(RANDOM_BYTES)
        (self.dir / "pic.png").write_bytes(PNG_BYTES)
        self.stderr = open(self.dir / "server.stderr", "wb")
        self.proc = subprocess.Popen(
            [self.binary, "--directory", str(self.dir)],
            stdout=subprocess.DEVNULL, stderr=self.stderr,
        )
        for _ in range(100):
            if port_is_busy():
                return self
            if self.proc.poll() is not None:
                sys.exit(f"FATAL: the server exited immediately (rc={self.proc.returncode})")
            time.sleep(0.05)
        sys.exit(f"FATAL: the server never started listening on {PORT}")

    def __exit__(self, *exc):
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        if self.stderr:
            self.stderr.close()
        shutil.rmtree(self.dir, ignore_errors=True)
        return False


# --------------------------------------------------------------------------
# Raw HTTP helpers. Deliberately no `curl`: the pipelining tests need to put
# several requests in ONE write() and read the raw bytes back, and a client
# library would not let us.
# --------------------------------------------------------------------------

def send_raw(payload, read_all=True, timeout=6.0):
    """One connection, one write of `payload`, read until EOF or timeout."""
    s = socket.create_connection((HOST, PORT), timeout=timeout)
    s.settimeout(timeout)
    try:
        s.sendall(payload)
        buf = b""
        while True:
            try:
                d = s.recv(65536)
            except socket.timeout:
                break
            if not d:
                break
            buf += d
        return buf
    finally:
        s.close()


def get(path, headers=(), method=b"GET", body=b""):
    req = method + b" " + path + b" HTTP/1.1\r\nHost: x\r\n"
    for h in headers:
        req += h + b"\r\n"
    if body:
        req += b"Content-Length: " + str(len(body)).encode() + b"\r\n"
    req += b"Connection: close\r\n\r\n" + body
    return send_raw(req)


def split_response(buf):
    """Split a response byte string into (status_code, headers, body)."""
    head, _, body = buf.partition(b"\r\n\r\n")
    lines = head.split(b"\r\n")
    status = lines[0].decode(errors="replace")
    headers = {}
    for line in lines[1:]:
        k, _, v = line.decode(errors="replace").partition(":")
        headers[k.strip().lower()] = v.strip()
    return status, headers, body


def responses(buf):
    """Split a pipelined byte string into a list of (status, headers, body).

    Splits on the status line, which is unambiguous because a pipelined
    response always starts with `HTTP/1.1 `.
    """
    out = []
    for chunk in buf.split(b"HTTP/1.1 ")[1:]:
        out.append(split_response(b"HTTP/1.1 " + chunk))
    return out


def status_code(status_line):
    m = re.match(r"HTTP/1\.\d (\d+)", status_line)
    return int(m.group(1)) if m else -1
