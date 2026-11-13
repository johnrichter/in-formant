#include "canvas.h"
#include <stdexcept>

using namespace Gui;

CanvasItem::CanvasItem()
    : mRenderContext(nullptr),
      mActivity(std::make_shared<CanvasActivity>())
{
    setObjectName("IfCanvas");
    setTextureFollowsItemSize(true);

    // The timer reads the running state, and it draws the frames of an idle canvas.
    mTimer.setInterval(canvasTickMs);
    connect(&mTimer, &QTimer::timeout, this, &CanvasItem::tick);
    mTimer.start();
}

void CanvasItem::setRenderContext(Main::RenderContext *renderContext)
{
    mRenderContext = renderContext;
}

void CanvasItem::setRunningProvider(std::function<bool()> provider)
{
    mRunning = std::move(provider);
}

void CanvasItem::noteInput()
{
    mActivity->lastInputMs = canvasNowMs();
    update();
}

void CanvasItem::tick()
{
    mActivity->running = mRunning && mRunning();

    // A busy canvas draws the next frame by itself, and the timer only restarts the chain. An idle canvas draws one
    // frame for each period of canvasIdleFrameMs, so a change of a setting shows soon.
    const long long now = canvasNowMs();
    if (mActivity->busy(now)) {
        update();
    }
    else if (now - mLastIdleFrame >= canvasIdleFrameMs) {
        mLastIdleFrame = now;
        update();
    }
}

class CanvasInFboRenderer : public QQuickFramebufferObject::Renderer
{
public:
    CanvasInFboRenderer(Main::RenderContext *renderContext, std::shared_ptr<CanvasActivity> activity)
        : mActivity(std::move(activity)) {
        mRenderer.initialize(renderContext);
    }

    ~CanvasInFboRenderer() {
        mRenderer.cleanup();
    }

protected:
    void render() override {
        mRenderer.render();
        // Ask for the next frame at once only when something moves. The timer of the item draws the idle frames.
        if (mActivity->busy(canvasNowMs())) update();
    }
    
    void synchronize(QQuickFramebufferObject *item) override {
        mRenderer.synchronize(item);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    CanvasRenderer mRenderer;
    std::shared_ptr<CanvasActivity> mActivity;
};

QQuickFramebufferObject::Renderer *CanvasItem::createRenderer() const
{
    return new CanvasInFboRenderer(mRenderContext, mActivity);
}
