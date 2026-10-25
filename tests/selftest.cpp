#include "../src/sysbassbackend.h"

#include <cstdio>

static int failures = 0;

static void check(const char *in, const char *expected)
{
    const QString out = dbusMenuLabelToQt(QString::fromUtf8(in));
    if (out != QString::fromUtf8(expected)) {
        std::printf("FAIL: \"%s\" -> \"%s\", expected \"%s\"\n", in, qPrintable(out), expected);
        ++failures;
    }
}

int main()
{
    check("_Quit", "&Quit");
    check("snake__case", "snake_case");
    check("Tom & Jerry", "Tom && Jerry");
    check("Trailing_", "Trailing&");
    check("", "");
    std::printf(failures ? "selftest failed\n" : "selftest ok\n");
    return failures ? 1 : 0;
}
