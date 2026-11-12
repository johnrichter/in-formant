#ifndef MAIN_CONTEXT_VIEWS_SPECTROGRAM_H
#define MAIN_CONTEXT_VIEWS_SPECTROGRAM_H

#include <chrono>
#include <memory>
#include <vector>

#include "../reviewspectrogram.h"
#include "views.h"

namespace Main::View {

    class Spectrogram : public QObject, public AbstractView {
        Q_OBJECT

    public:
        Spectrogram();
        virtual ~Spectrogram();

    protected:
        void render(QPainterWrapper *painter, Config *config, DataStore *dataStore) override;

    private:
        // The spectrogram of the stored sound. It serves long views and views of a past time.
        std::unique_ptr<ReviewSpectrogram> mReview;
        ReviewRequest mLastRequest;
        bool mHasRequest = false;
        std::chrono::steady_clock::time_point mLastRequestTime;

        // The newest values of the statistics, and the time that they were computed.
        std::vector<Gui::HudTrack> mHudCache;
        std::chrono::steady_clock::time_point mHudTime;

        // The source of the slices in the texture: 0 for the live slices, or the number of a result.
        unsigned long long mTextureSource = 0;
    };

}

#endif
