#ifndef MAIN_GUI_CONTEXT_H
#define MAIN_GUI_CONTEXT_H

#include <memory>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QQuickView>
#include <QScreen>
#include <QTimer>

#include "synthwrapper.h"
#include "dataviswrapper.h"

namespace Gui {
    class CanvasItem;
}

namespace Main {

    class RenderContext;
    class Config;
    class DataStore;
    class SessionWrapper;
    class PracticeWrapper;
    class GuiView;

    class GuiContext : public QObject {
        Q_OBJECT

    public:
        GuiContext(Config *config, RenderContext *renderContext, SynthWrapper *synthWrapper, DataVisWrapper *dataVisWrapper, SessionWrapper *sessionWrapper);

        ~GuiContext();

        int exec();
    
        void setView(GuiView *view);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    public slots:
        void setShowSpectrogram(bool b);
        void setShowPitch(bool b);

    private:
        void setWidth(int width);
        void setHeight(int height);
        void updateDpi(QScreen *screen);

        Config *mConfig;
        RenderContext *mRenderContext;

        GuiView *mSelectedView;

        // Sees the input of the user in all windows. The canvas of the main window is owned by the engine.
        class InputWatcher : public QObject {
        public:
            Gui::CanvasItem *canvas = nullptr;
        protected:
            bool eventFilter(QObject *obj, QEvent *event) override;
        };
        InputWatcher mInputWatcher;

        std::unique_ptr<QApplication> mApp;
        // The wrapper outlives the engine, so no binding reads it after it is gone.
        std::unique_ptr<PracticeWrapper> mPractice;
        std::unique_ptr<QQmlApplicationEngine> mQmlEngine;

        friend class GuiView;
    };

    class GuiView {
    protected:
        virtual bool onKeyPress(QKeyEvent *) { return false; }
        virtual bool onKeyRelease(QKeyEvent *) { return false; }

        friend bool GuiContext::eventFilter(QObject *obj, QEvent *event);
    };

}

#endif // MAIN_GUI_CONTEXT_H
