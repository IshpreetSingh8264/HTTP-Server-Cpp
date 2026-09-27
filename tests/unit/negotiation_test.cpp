// Accept-Encoding negotiation and the response encoder.
//
// The negotiator has to handle more than the happy path: whitespace, casing,
// q-values, q=0 meaning "explicitly not acceptable", wildcards, unknown
// codings, and malformed q-values. Those are exactly the inputs the course
// tests never send, and exactly where a hand-rolled parser goes wrong.
//
// The encoder half checks the two things that are easy to get subtly wrong:
// that gzip really is a gzip container (magic 1f 8b) and that deflate really
// is a *raw* deflate stream (no zlib header), plus that gzip's overhead over
// the same deflate stream is exactly the 18-byte container.
#include "compression/encoding_negotiator.hpp"
#include "compression/response_encoder.hpp"
#include <cstdio>
#include <string>
using namespace compression;
static int fails = 0;
static void chk(const char* ae, ContentEncoding want) {
    ContentEncoding got = EncodingNegotiator::negotiate(ae);
    if (got != want) { printf("FAIL negotiate(\"%s\") = %s want %s\n", ae, toHeaderValue(got), toHeaderValue(want)); ++fails; }
    else printf("ok   %-44s -> %s\n", ae, toHeaderValue(got));
}
static void chkbool(const char* what, bool got, bool want) {
    if (got != want) { printf("FAIL %s = %d want %d\n", what, (int)got, (int)want); ++fails; }
    else printf("ok   %-44s = %d\n", what, (int)got);
}
int main() {
    printf("--- Accept-Encoding negotiation ---\n");
    chk("", ContentEncoding::Identity);
    chk("   ", ContentEncoding::Identity);
    chk("gzip", ContentEncoding::Gzip);
    chk("deflate", ContentEncoding::Deflate);
    chk("GZIP", ContentEncoding::Gzip);
    chk("  Deflate  ", ContentEncoding::Deflate);
    chk("identity", ContentEncoding::Identity);
    chk("br", ContentEncoding::Identity);
    chk("gzip, deflate", ContentEncoding::Gzip);
    chk("deflate, gzip", ContentEncoding::Deflate);
    chk("gzip, deflate, br", ContentEncoding::Gzip);
    chk("br, deflate, gzip", ContentEncoding::Deflate);
    chk("gzip;q=0.5, deflate;q=0.9", ContentEncoding::Deflate);
    chk("gzip;q=0.9, deflate;q=0.5", ContentEncoding::Gzip);
    chk("gzip;q=1.0, deflate;q=1.0", ContentEncoding::Gzip);
    chk("gzip;q=0, deflate", ContentEncoding::Deflate);
    chk("gzip;q=0", ContentEncoding::Identity);
    chk("deflate;q=0", ContentEncoding::Identity);
    chk("*", ContentEncoding::Gzip);
    chk("*, gzip", ContentEncoding::Gzip);
    chk("deflate, *", ContentEncoding::Deflate);
    chk("br;q=1.0, gzip;q=0.1", ContentEncoding::Gzip);
    chk("gzip ; q=0.8 , deflate ; q=0.8", ContentEncoding::Gzip);
    chk("x-gzip", ContentEncoding::Gzip);
    chk("gzip,,deflate", ContentEncoding::Gzip);
    chk("gzip;q=bogus", ContentEncoding::Identity);

    printf("\n--- encoder ---\n");
    const std::string data(2000, 'A');
    const std::string marker = "The quick brown fox jumps over the lazy dog. ";
    std::string mixed; for (int i=0;i<50;++i) mixed += marker;

    chkbool("gzip(2000 A) much smaller than input", ResponseEncoder::gzip(data).size() < 100, true);
    chkbool("deflate(2000 A) much smaller than input", ResponseEncoder::deflate(data).size() < 100, true);
    chkbool("gzip size == deflate size + 18", ResponseEncoder::gzip(mixed).size() == ResponseEncoder::deflate(mixed).size() + 18, true);
    chkbool("gzip starts with magic 1f 8b",
            (unsigned char)ResponseEncoder::gzip(mixed)[0] == 0x1f &&
            (unsigned char)ResponseEncoder::gzip(mixed)[1] == 0x8b, true);
    chkbool("deflate has no gzip magic",
            ResponseEncoder::deflate(mixed).size() < 2 ||
            !((unsigned char)ResponseEncoder::deflate(mixed)[0] == 0x1f &&
              (unsigned char)ResponseEncoder::deflate(mixed)[1] == 0x8b), true);
    chkbool("compress(Identity) empty", ResponseEncoder::compress(mixed, ContentEncoding::Identity).empty(), true);
    chkbool("shouldCompress(empty) == false", ResponseEncoder::shouldCompress("", "text/plain"), false);
    chkbool("shouldCompress(image/png) == false", ResponseEncoder::shouldCompress("x", "image/png"), false);
    chkbool("shouldCompress(text/plain) == true", ResponseEncoder::shouldCompress("x", "text/plain"), true);

    if (fails) {
        printf("\n%d FAILURES\n", fails);
    } else {
        printf("\nall negotiation tests passed\n");
    }
    return fails ? 1 : 0;
}
