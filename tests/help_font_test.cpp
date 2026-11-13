// The help style and the fonts. The help style needs a Qt Quick text document, so the test makes
// one with the platform "offscreen". No window opens.
#include <gtest/gtest.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <QColor>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTextDocument>

#include "../src/gui/helpstyle.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {

const std::filesystem::path root = INFORMANT_SOURCE_DIR;

QGuiApplication &application()
{
    static int argc = 1;
    static char name[] = "help_font_tests";
    static char *argv[] = { name, nullptr };
    qputenv("QT_QPA_PLATFORM", "offscreen");
    // The application stays until the process ends. A static object would be destroyed in an order that
    // crashes at exit, after the Qt Quick engine of a test is gone.
    static QGuiApplication *app = new QGuiApplication(argc, argv);
    return *app;
}

QString helpText(const QString &file)
{
    QFile f(QString::fromStdString((root / "docs" / file.toStdString()).string()));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

}

// The first build of the help style took 12 seconds for the start of the app. The style of all the help pages
// must take less than 200 ms in total.
TEST(HelpStyle, TP_STP_02_TheStyleOfAllHelpPagesTakesLessThan200Milliseconds)
{
    application();
    QQmlEngine engine;
    Gui::HelpStyle style;

    const char *pages[] = { "reading-the-display.md", "choosing-settings.md", "feminization.md", "masculinization.md",
                            "singing.md", "warmups-and-vocal-health.md", "synthesizer.md", "the-math.md" };

    qint64 total = 0;
    qint64 slowest = 0;
    for (const char *page : pages) {
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { width: 640; textFormat: TextEdit.MarkdownText; wrapMode: TextEdit.WordWrap }",
                          QUrl());
        QObject *object = component.create();
        ASSERT_NE(object, nullptr) << component.errorString().toStdString();
        auto *item = qobject_cast<QQuickItem *>(object);
        ASSERT_NE(item, nullptr);
        const QString text = helpText(page);
        ASSERT_FALSE(text.isEmpty()) << page;
        item->setProperty("text", text);

        auto *document = item->property("textDocument").value<QQuickTextDocument *>();
        ASSERT_NE(document, nullptr);
        ASSERT_NE(document->textDocument(), nullptr);
        ASSERT_GT(document->textDocument()->blockCount(), 5) << page;

        QElapsedTimer timer;
        timer.start();
        style.apply(document, "Roboto", "Roboto Mono", QColor("#E6E1E5"), QColor("#CAC4D0"), QColor("#A8A2AE"),
                    QColor("#b39ddb"), QColor("#2B2930"));
        const qint64 elapsed = timer.elapsed();
        total += elapsed;
        slowest = std::max(slowest, elapsed);
        delete object;
    }
    std::printf("[ INFO ] The style of 8 pages took %lld ms (the slowest page %lld ms).\n", (long long) total, (long long) slowest);
    EXPECT_LT(total, 200);
}

// Roboto Mono gives each digit and the point the same width. So a text like "0.00 ms" and "99.99 ms" has one
// width, when it has the same number of characters. The frame times use the format "%5.2f".
TEST(Fonts, TP_FONT_04_TheFrameTimesTextHasTheSameWidthForEachValue)
{
    FT_Library library;
    ASSERT_EQ(FT_Init_FreeType(&library), 0);
    FT_Face face;
    const std::string path = (root / "resources/RobotoMono-Regular.ttf").string();
    ASSERT_EQ(FT_New_Face(library, path.c_str(), 0, &face), 0) << path;

    for (int size : { 10, 12, 14, 18, 24, 28 }) {
        ASSERT_EQ(FT_Set_Pixel_Sizes(face, 0, (FT_UInt) size), 0);
        long advance = -1;
        for (char c : std::string(" 0123456789.%msf")) {
            ASSERT_EQ(FT_Load_Char(face, (FT_ULong) c, FT_LOAD_DEFAULT), 0) << c;
            const long x = face->glyph->advance.x;
            if (advance < 0) advance = x;
            EXPECT_EQ(x, advance) << "character '" << c << "' at " << size << " pixels";
        }
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    // The format gives the same number of characters for each value from 0.00 to 99.99.
    char a[32], b[32], c[32];
    std::snprintf(a, sizeof a, "%5.2f", 0.0);
    std::snprintf(b, sizeof b, "%5.2f", 99.99);
    std::snprintf(c, sizeof c, "%5.2f", 7.5);
    EXPECT_EQ(std::string(a).size(), std::string(b).size());
    EXPECT_EQ(std::string(a).size(), std::string(c).size());
    EXPECT_EQ(std::string(a), " 0.00");
}
