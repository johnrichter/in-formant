#include <gtest/gtest.h>

#include "gui/imagehold.h"

#include <algorithm>

using Gui::imageHoldEnd;
using Gui::maxImageHold;

// A picture that is a little older than the view gets its newest column held up to the view.
TEST(ImageHold, TP_EDGE_05_AShortLagGetsTheNewestColumn)
{
    EXPECT_DOUBLE_EQ(imageHoldEnd(10.0, 10.1), 10.1);
    EXPECT_DOUBLE_EQ(imageHoldEnd(10.0, 10.0 + maxImageHold), 10.0 + maxImageHold);
}

// A long lag shows the real end of the picture, and a picture that is newer than the view needs no hold.
TEST(ImageHold, TP_EDGE_05_ALongLagAndANewerPictureGetNoHold)
{
    EXPECT_DOUBLE_EQ(imageHoldEnd(10.0, 10.0 + maxImageHold + 0.001), 10.0);
    EXPECT_DOUBLE_EQ(imageHoldEnd(10.0, 12.0), 10.0);
    EXPECT_DOUBLE_EQ(imageHoldEnd(10.0, 9.9), 10.0);
    EXPECT_DOUBLE_EQ(imageHoldEnd(10.0, 10.0), 10.0);
}

// The hold is never shorter than the picture, for any pair of times.
TEST(ImageHold, TP_EDGE_05_TheHoldNeverEndsBeforeThePicture)
{
    for (double image : { -5.0, 0.0, 0.3, 59.99, 3600.0 }) {
        for (double offset = -1.0; offset <= 1.0; offset += 0.05) {
            const double end = imageHoldEnd(image, image + offset);
            EXPECT_GE(end, image);
            EXPECT_LE(end, std::max(image, image + offset));
        }
    }
}
