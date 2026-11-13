#ifndef GUI_CANVAS_H
#define GUI_CANVAS_H

#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QTimer>

#include <functional>
#include <memory>

#include "canvasactivity.h"

namespace Main {
    class RenderContext;
}

namespace Gui
{   
    class CanvasRenderer;

    class CanvasItem : public QQuickFramebufferObject
    {
        Q_OBJECT
    public:
        CanvasItem();

        void setRenderContext(Main::RenderContext *renderContext);

        // The function tells whether the capture or a playback runs. The item asks it from the GUI thread.
        void setRunningProvider(std::function<bool()> provider);

        // The user did something. The canvas draws at the rate of the screen for a short time.
        void noteInput();
        
        Renderer *createRenderer() const override;
    
    private:
        void tick();

        Main::RenderContext *mRenderContext;
        std::shared_ptr<CanvasActivity> mActivity;
        std::function<bool()> mRunning;
        long long mLastIdleFrame = 0;
        QTimer mTimer;
    };
}

#include "canvas_renderer.h"

#endif // GUI_CANVAS_H
