#include "../gui/canvas.h"
#include "../gui/overviewitem.h"
#include "../gui/plotitem.h"
#include "contextmanager.h"
#include "practicewrapper.h"
#include "qsgrendererinterface.h"
#include "qsurfaceformat.h"
#include "rendercontext.h"
#include <QQmlContext>
#include <iostream>
#include <chrono>

using namespace Main;
using namespace std::chrono_literals;

#ifndef WITHOUT_SYNTH
GuiContext::GuiContext(Config *config, RenderContext *renderContext, SynthWrapper *synthWrapper, DataVisWrapper *dataVisWrapper, SessionWrapper *sessionWrapper)
#else
GuiContext::GuiContext(Config *config, RenderContext *renderContext, DataVisWrapper *dataVisWrapper, SessionWrapper *sessionWrapper)
#endif
    : mConfig(config),
      mRenderContext(renderContext),
      mSelectedView(nullptr)
{
    QCoreApplication::setApplicationName("InFormant");
    QCoreApplication::setApplicationVersion(INFORMANT_VERSION_STR);
    QCoreApplication::setOrganizationDomain("in-formant.app");
    QCoreApplication::setOrganizationName("InFormant");

    QQuickStyle::setStyle("Material");
    qmlRegisterType<Gui::CanvasItem>("IfCanvas", 1, 0, "IfCanvas");
    qmlRegisterType<Gui::PlotItem>("IfCanvas", 1, 0, "IfPlot");
    qmlRegisterType<Gui::OverviewItem>("IfCanvas", 1, 0, "IfOverview");

    // The plots of the oscilloscope and of the synthesizer read their data from these objects.
    Gui::PlotItem::setConfig(mConfig);
    Gui::PlotItem::setProvider(Gui::PlotItem::Scope, dataVisWrapper);
    Gui::OverviewItem::setSources(mConfig, sessionWrapper);

    mApp = std::make_unique<QApplication>(argc, argv);

    // Only supports OpenGL for now.
    QQuickWindow::setSceneGraphBackend("rhi");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication::setWindowIcon(QIcon(":/icons/in-formant.png"));

    mQmlEngine = std::make_unique<QQmlApplicationEngine>();
    mQmlEngine->addImportPath(QCoreApplication::applicationDirPath() + "/qml");
#ifdef __APPLE__
    mQmlEngine->addImportPath(QCoreApplication::applicationDirPath() + "/../Resources/qml");
#endif

    mQmlEngine->rootContext()->setContextProperty("appName", "InFormant " INFORMANT_VERSION_STR);

    mQmlEngine->rootContext()->setContextProperty("config", mConfig);
    mQmlEngine->rootContext()->setContextProperty("sessionControl", sessionWrapper);
    mPractice = std::make_unique<PracticeWrapper>(mConfig, sessionWrapper);
    mQmlEngine->rootContext()->setContextProperty("practice", mPractice.get());

#ifndef WITHOUT_SYNTH
    Gui::PlotItem::setProvider(Gui::PlotItem::Filter, synthWrapper);
    Gui::PlotItem::setProvider(Gui::PlotItem::Source, synthWrapper);
    Gui::PlotItem::setProvider(Gui::PlotItem::Spectrum, synthWrapper);
    mQmlEngine->rootContext()->setContextProperty("synth", synthWrapper);
    mQmlEngine->rootContext()->setContextProperty("HAS_SYNTH", true);
#else
    mQmlEngine->rootContext()->setContextProperty("HAS_SYNTH", false);
#endif

#ifdef ENABLE_TORCH
    mQmlEngine->rootContext()->setContextProperty("HAS_TORCH", true);
#else
    mQmlEngine->rootContext()->setContextProperty("HAS_TORCH", false);
#endif

    mQmlEngine->load(QUrl("qrc:/MainWindow.qml"));
   
    auto window = static_cast<QQuickWindow *>(mQmlEngine->rootObjects().first());
    auto canvasItem = window->findChild<Gui::CanvasItem *>("IfCanvas");

    canvasItem->setRenderContext(mRenderContext);
    sessionWrapper->setMainWindow(window);

    canvasItem->installEventFilter(this);
    window->installEventFilter(this);

    window->show();
}

GuiContext::~GuiContext() = default;

int GuiContext::exec()
{
    return mApp->exec();
}

void GuiContext::setView(GuiView *view)
{
    mSelectedView = view;
}

bool GuiContext::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (mSelectedView != nullptr && mSelectedView->onKeyPress(keyEvent)) {
            return true;
        }
        else {
        }
    }
    else if (event->type() == QEvent::KeyRelease) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (mSelectedView != nullptr && mSelectedView->onKeyRelease(keyEvent)) {
            return true;
        }
        else {
        }
    }

    return false;
}

void GuiContext::setShowSpectrogram(bool b)
{
    mConfig->setViewShowSpectrogram(b);
}

void GuiContext::setShowPitch(bool b)
{
    mConfig->setViewShowPitch(b);
}

void GuiContext::setShowFormants(bool b)
{
    mConfig->setViewShowFormants(b);
}
