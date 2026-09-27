// StringUtils - the parsing helpers every request path goes through.
//
// The load-bearing behaviour is that split() KEEPS empty tokens, because the
// HTTP request line is space-delimited and "GET  /x HTTP/1.1" has a double
// space in it: a split that collapses empty tokens shifts every field by one
// and the request line parses as garbage. That is the whole reason the first
// block below exists.
#include "utils/StringUtils.hpp"
#include <cstdio>
#include <string>
using namespace utils;
static int fails = 0;
static void eq(const char* what, const std::string& got, const std::string& want) {
    if (got != want) { printf("FAIL %-46s got [%s] want [%s]\n", what, got.c_str(), want.c_str()); ++fails; }
    else printf("ok   %-46s = [%s]\n", what, got.c_str());
}
static void split_is(const char* what, const std::vector<std::string>& got, size_t n, const char* joined) {
    std::string flat;
    for (size_t i = 0; i < got.size(); ++i) { if (i) flat += "|"; flat += got[i]; }
    if (got.size() != n || flat != joined) {
        printf("FAIL %-46s got %zu [%s] want %zu [%s]\n", what, got.size(), flat.c_str(), n, joined);
        ++fails;
    } else {
        printf("ok   %-46s = %zu [%s]\n", what, got.size(), flat.c_str());
    }
}
int main() {
    printf("--- split keeps empty tokens ---\n");
    split_is("split(\"a,,b\", ',')",        StringUtils::split("a,,b", ','), 3, "a||b");
    split_is("split(\",a\", ',')",         StringUtils::split(",a", ','),  2, "|a");
    split_is("split(\"a,\", ',')",         StringUtils::split("a,", ','),   2, "a|");
    split_is("split(\",\", ',')",          StringUtils::split(",", ','),    2, "|");
    split_is("split(\"\", ',')",           StringUtils::split("", ','),     1, "");
    split_is("split(\"GET  /x HTTP/1.1\",' ')",
             StringUtils::split("GET  /x HTTP/1.1", ' '), 4, "GET||/x|HTTP/1.1");
    split_is("split(\"GET /x HTTP/1.1\",' ')",
             StringUtils::split("GET /x HTTP/1.1", ' '), 3, "GET|/x|HTTP/1.1");
    split_is("split(hdr,\";\") multi-char",
             StringUtils::split("gzip;q=0.5, deflate", ','), 2, "gzip;q=0.5| deflate");
    split_is("split(\"a\\r\\n\\r\\nb\", sep)",
             StringUtils::split("a\r\n\r\nb", "\r\n"), 3, "a||b");

    printf("\n--- other helpers ---\n");
    eq("trim(\"  hi  \")",   StringUtils::trim("  hi  "), "hi");
    eq("trim(\"   \")",     StringUtils::trim("   "), "");
    eq("toLower(\"GZIP\")", StringUtils::toLower("GZIP"), "gzip");
    eq("startsWith(\"/files/a\",\"/files/\")", StringUtils::startsWith("/files/a","/files/")?"y":"n", "y");
    eq("startsWith(\"/file\",\"/files/\")",    StringUtils::startsWith("/file","/files/")?"y":"n", "n");
    eq("endsWith(\"a...\",\"...\")",         StringUtils::endsWith("a...","...")?"y":"n", "y");
    eq("endsWith(\"a..\",\"...\")",          StringUtils::endsWith("a..","...")?"y":"n", "n");
    eq("endsWith(\"...\",\"...\")",          StringUtils::endsWith("...","...")?"y":"n", "y");
    eq("endsWith(\"name...\",\"...\")",      StringUtils::endsWith("name...","...")?"y":"n", "y");
    eq("endsWith(\"{name...}\",\"...\")",    StringUtils::endsWith("{name...}","...")?"y":"n", "n");
    eq("endsWith(\"x\",\"longer\")",         StringUtils::endsWith("x","longer")?"y":"n", "n");
    eq("urlDecode(\"a%20b\")",               StringUtils::urlDecode("a%20b"), "a b");
    eq("urlDecode(\"a+b\")",                 StringUtils::urlDecode("a+b"), "a b");
    eq("equalsIgnoreCase",                   StringUtils::equalsIgnoreCase("Content-Type","content-TYPE")?"y":"n", "y");
    eq("substringAfter(\"k: v\",\": \")",     StringUtils::substringAfter("k: v", ": "), "v");
    eq("substringBefore(\"a=b\",\"=\")",     StringUtils::substringBefore("a=b", '='), "a");
    eq("substringBefore(\"abc\",\"=\")",     StringUtils::substringBefore("abc", '='), "abc");

    if (fails) {
        printf("\n%d FAILURES\n", fails);
    } else {
        printf("\nall StringUtils tests passed\n");
    }
    return fails ? 1 : 0;
}
