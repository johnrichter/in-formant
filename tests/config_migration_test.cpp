#include "check.h"
#include "../src/context/configmigration.h"

using namespace Main;

static toml::table makeTable(const char *text)
{
    toml::table tbl = toml::parse(text);
    for (const char *name : { "analysis", "view", "options", "meta" }) {
        if (!tbl[name]) tbl.insert(name, toml::table());
    }
    return tbl;
}

int main()
{
    // An old file that holds the old defaults gets the new defaults.
    {
        toml::table tbl = makeTable(
            "[meta]\nversion = 3\n"
            "[view]\nmaxFrequency = 6000\n"
            "[options]\nlineWidth = 2.5\ndotRadius = 3.0\nstatsMask = 203\n");
        migrateConfig(tbl);
        CHECK(tbl["options"]["lineWidth"].value<double>() == 2.0);
        CHECK(tbl["options"]["dotRadius"].value<double>() == 2.0);
        CHECK(tbl["options"]["statsMask"].value<int64_t>() == (kOldStatsMask | kStatMedianBit));
        CHECK(tbl["view"]["maxFrequency"].value<int64_t>() == 5500);
        CHECK(tbl["meta"]["version"].value<int64_t>() == kConfigVersion);
    }

    // A value that the user changed stays.
    {
        toml::table tbl = makeTable(
            "[meta]\nversion = 3\n"
            "[view]\nmaxFrequency = 8000\n"
            "[options]\nlineWidth = 4.0\ndotRadius = 1.5\nstatsMask = 7\n");
        migrateConfig(tbl);
        CHECK(tbl["options"]["lineWidth"].value<double>() == 4.0);
        CHECK(tbl["options"]["dotRadius"].value<double>() == 1.5);
        CHECK(tbl["options"]["statsMask"].value<int64_t>() == 7);
        CHECK(tbl["view"]["maxFrequency"].value<int64_t>() == 8000);
    }

    // A second run changes nothing, and a current file is not touched.
    {
        toml::table tbl = makeTable(
            "[meta]\nversion = 4\n"
            "[view]\nmaxFrequency = 6000\n"
            "[options]\nlineWidth = 2.5\n");
        migrateConfig(tbl);
        CHECK(tbl["options"]["lineWidth"].value<double>() == 2.5);
        CHECK(tbl["view"]["maxFrequency"].value<int64_t>() == 6000);
    }

    // A new file gets no value for the options, so the defaults of the program apply.
    {
        toml::table tbl = makeTable("");
        migrateConfig(tbl);
        CHECK(!tbl["options"]["lineWidth"]);
        CHECK(tbl["analysis"]["granularity"].value<double>() == 10.0);
        CHECK(tbl["view"]["timeSpan"].value<double>() == 15.0);
        CHECK(tbl["meta"]["version"].value<int64_t>() == kConfigVersion);
    }

    return TEST_RESULT();
}
