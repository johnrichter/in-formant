#ifndef GUI_IMAGE_HOLD_H
#define GUI_IMAGE_HOLD_H

// The picture of the spectrogram for a long live view comes from a thread of the analysis. The picture is
// older than the view by the delay of the analysis. The code has no dependency on the display, so a test can run it.

namespace Gui {

    // The longest span, in seconds, that the newest column of the picture fills.
    constexpr double maxImageHold = 0.25;

    // The end of the span that the picture covers on the screen. A short lag of the picture behind the view gets the
    // newest column. A longer lag shows the real end of the picture, because a long stretch of one column misleads.
    // A view of a past time ends before the picture ends, so it gets no hold.
    inline double imageHoldEnd(double imageEnd, double viewEnd)
    {
        return (viewEnd > imageEnd && viewEnd - imageEnd <= maxImageHold) ? viewEnd : imageEnd;
    }

}

#endif // GUI_IMAGE_HOLD_H
