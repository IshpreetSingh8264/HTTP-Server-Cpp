// The route pattern matcher.
//
// The interesting cases are the negative ones and the boundary ones:
// a single-segment parameter must not swallow a slash, a multi-segment
// parameter must not match the empty string, and a literal route must not
// match a longer path that merely starts with the same characters
// ("/user-agent" vs "/user-agency"). Those are what an if-chain on
// substrings gets wrong, and they are what the course never probes.
#include "handlers/routes/route_registry.hpp"
#include <cstdio>
using namespace handlers::routes;
static int fails = 0;
static void chk(const char* path, const char* pat, bool want) {
    bool got = pathMatches(path, pat);
    if (got != want) { printf("FAIL pathMatches(\"%s\",\"%s\") = %d want %d\n", path, pat, got, want); ++fails; }
    else printf("ok   pathMatches(\"%s\",\"%s\") = %d\n", path, pat, got);
}
int main() {
    printf("routeCount = %zu\n", routeCount());
    chk("/", "/", true);
    chk("/nope", "/", false);
    chk("/echo/abc", "/echo/{str}", true);
    chk("/echo/", "/echo/{str}", true);
    chk("/echo/a/b", "/echo/{str}", false);
    chk("/user-agent", "/user-agent", true);
    chk("/user-agency", "/user-agent", false);
    chk("/files/x", "/files/{name...}", true);
    chk("/files/a/b/c", "/files/{name...}", true);
    chk("/files/", "/files/{name...}", true);
    chk("/files", "/files/{name...}", false);
    chk("/filesx/y", "/files/{name...}", false);
    if (fails) {
        printf("\n%d FAILURES\n", fails);
    } else {
        printf("all matcher tests passed\n");
    }
    return fails ? 1 : 0;
}
