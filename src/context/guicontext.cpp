#include "../gui/canvas.h"
#include "../gui/overviewitem.h"
#include "../gui/plotitem.h"
#include "../gui/uiscale.h"
#include "contextmanager.h"
#include "practicewrapper.h"
#include "qsgrendererinterface.h"
#include "qsurfaceformat.h"
#include "rendercontext.h"
#include <QColor>
#include <QFontDatabase>
#include "../gui/helpstyle.h"
#include <QQmlContext>
#include <iostream>
#include <chrono>
#include <cstring>

using namespace Main;
using namespace std::chrono_literals;

GuiContext::GuiContext(Config *config, RenderContext *renderContext, SynthWrapper *synthWrapper, DataVisWrapper *dataVisWrapper, SessionWrapper *sessionWrapper)
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
    mQmlEngine->addImportPath(QCoreApplication::applicationDirPath() + "/../Resources/qml");

    mQmlEngine->rootContext()->setContextProperty("appName", "InFormant " INFORMANT_VERSION_STR);

    // The type scale, the spacing scale, and the radii of all windows. The numbers are in gui/uiscale.h.
    {
        QVariantMap type;
        for (const Gui::UiScale::TypeStep &step : Gui::UiScale::typeSteps) {
            type[step.name] = step.size;
            type[QString(step.name) + "Line"] = step.line;
        }
        QVariantMap space;
        for (const Gui::UiScale::SpaceStep &step : Gui::UiScale::spaceSteps) space[step.name] = step.size;
        QVariantMap radius;
        for (const Gui::UiScale::RadiusStep &step : Gui::UiScale::radiusSteps) radius[step.name] = step.size;
        // The fonts of the windows: Roboto for text, and Roboto Mono for numbers and labels that line up.
        // The two files are in the resources, so the windows do not depend on the fonts of the system.
        QVariantMap font { { "sans", "Roboto" }, { "mono", "Roboto Mono" } };
        for (const char *path : { ":/Roboto.ttf", ":/Roboto-Medium.ttf", ":/Roboto-Bold.ttf", ":/RobotoMono-Regular.ttf" }) {
            const int id = QFontDatabase::addApplicationFont(path);
            const QStringList families = QFontDatabase::applicationFontFamilies(id);
            if (id >= 0 && !families.isEmpty() && std::strcmp(path, ":/RobotoMono-Regular.ttf") == 0) font["mono"] = families.first();
        }
        QVariantMap color;
        for (const Gui::UiScale::ColorToken &token : Gui::UiScale::textColors) {
            color[token.name] = QColor::fromRgb(token.rgb);
        }
        using Prose = Gui::UiScale::Prose;
        QVariantMap prose {
            { "bodySize", Prose::bodySize }, { "bodyLine", Prose::bodyLine }, { "measure", Prose::measure }, { "pageMargin", Prose::pageMargin },
        };
        mQmlEngine->rootContext()->setContextProperty("ui", QVariantMap { { "type", type }, { "space", space }, { "radius", radius },
                                                                           { "color", color }, { "font", font }, { "measure", Gui::UiScale::textMeasure }, { "scrollLine", Gui::UiScale::scrollLine },
                                                                           { "prose", prose } });
        mQmlEngine->rootContext()->setContextProperty("helpStyle", new Gui::HelpStyle(mQmlEngine.get()));
    }

    mQmlEngine->rootContext()->setContextProperty("config", mConfig);
    mQmlEngine->rootContext()->setContextProperty("sessionControl", sessionWrapper);
    mPractice = std::make_unique<PracticeWrapper>(mConfig, sessionWrapper);
    mQmlEngine->rootContext()->setContextProperty("practice", mPractice.get());

    Gui::PlotItem::setProvider(Gui::PlotItem::Filter, synthWrapper);
    Gui::PlotItem::setProvider(Gui::PlotItem::Source, synthWrapper);
    Gui::PlotItem::setProvider(Gui::PlotItem::Spectrum, synthWrapper);
    mQmlEngine->rootContext()->setContextProperty("synth", synthWrapper);

    mQmlEngine->load(QUrl("qrc:/MainWindow.qml"));
   
    auto window = static_cast<QQuickWindow *>(mQmlEngine->rootObjects().first());
    auto canvasItem = window->findChild<Gui::CanvasItem *>("IfCanvas");

    canvasItem->setRenderContext(mRenderContext);
    mInputWatcher.canvas = canvasItem;
    mApp->installEventFilter(&mInputWatcher);
    // The canvas draws at the rate of the screen while the capture or a playback runs.
    canvasItem->setRunningProvider([config, sessionWrapper]() {
        return !config->isPaused() || sessionWrapper->playing();
    });
    sessionWrapper->setMainWindow(window);

    canvasItem->installEventFilter(this);
    window->installEventFilter(this);

    window->show();
}

GuiContext::~GuiContext()
{
    // The engine deletes the canvas below. An event during the teardown must not reach it.
    mInputWatcher.canvas = nullptr;
    if (mApp) mApp->removeEventFilter(&mInputWatcher);
}

bool GuiContext::InputWatcher::eventFilter(QObject *, QEvent *event)
{
    // An input of the user, in any window, keeps the canvas at the rate of the screen for a moment. A change of a
    // setting in the Settings window shows at once then.
    switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::HoverMove:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::TouchUpdate:
        if (canvas != nullptr) canvas->noteInput();
        break;
    default:
        break;
    }
    return false;
}

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

