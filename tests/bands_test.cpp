#include <gtest/gtest.h>
#include "../src/gui/bands.h"

using namespace Gui;

// A good text gives its four edges.
TEST(Bands, AGoodTextGivesItsFourEdges)
{
    const BandEdges e = parseBandEdges("80, 140, 170, 260");
    EXPECT_TRUE(e[0] == 80.0 && e[1] == 140.0 && e[2] == 170.0 && e[3] == 260.0);
}

// Spaces and commas both separate the values.
TEST(Bands, SpacesAndCommasBothSeparateTheValues)
{
    const BandEdges e = parseBandEdges("60 150,180  300");
    EXPECT_TRUE(e[0] == 60.0 && e[3] == 300.0);
}

// Wrong texts give the default edges.
TEST(Bands, WrongTextsGiveTheDefaultEdges)
{
    const char *bad[] = { "", "60,150,180", "60,150,180,300,400", "60,150,150,300", "300,180,150,60",
                          "a,b,c,d", "10,150,180,300", "60,150,180,5000", "60,150,180,x" };
    for (const char *text : bad) {
        const BandEdges e = parseBandEdges(text);
        EXPECT_TRUE(e == defaultBandEdges);
    }
}

// A text that separates the edges with a comma, a space, a semicolon, or a tab gives the four edges.
TEST(Bands, TheEdgesCanBeSeparatedByCommasSpacesSemicolonsAndTabs)
{
    const BandEdges expected = { 60.0, 150.0, 180.0, 300.0 };
    for (const char *text : { "60,150,180,300", "60 150 180 300", "60;150\t180 300", " 60 , 150 ; 180 , 300 " }) {
        EXPECT_EQ(parseBandEdges(text), expected) << text;
    }
    // A text with one edge too many, an edge that is not a number, or edges that do not rise gives the defaults.
    for (const char *text : { "60,150,180,300,400", "60,150,x,300", "60,150,150,300", "", "nan,1,2,3" }) {
        EXPECT_EQ(parseBandEdges(text), defaultBandEdges) << text;
    }
}
