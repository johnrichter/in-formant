#include "check.h"
#include "../src/gui/bands.h"

using namespace Gui;

int main()
{
    // A good text gives its four edges.
    {
        const BandEdges e = parseBandEdges("80, 140, 170, 260");
        CHECK(e[0] == 80.0 && e[1] == 140.0 && e[2] == 170.0 && e[3] == 260.0);
    }

    // Spaces and commas both separate the values.
    {
        const BandEdges e = parseBandEdges("60 150,180  300");
        CHECK(e[0] == 60.0 && e[3] == 300.0);
    }

    // Wrong texts give the default edges.
    {
        const char *bad[] = { "", "60,150,180", "60,150,180,300,400", "60,150,150,300", "300,180,150,60",
                              "a,b,c,d", "10,150,180,300", "60,150,180,5000", "60,150,180,x" };
        for (const char *text : bad) {
            const BandEdges e = parseBandEdges(text);
            CHECK(e == defaultBandEdges);
        }
    }

    return TEST_RESULT();
}
