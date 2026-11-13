#ifndef GUI_HELPSTYLE_H
#define GUI_HELPSTYLE_H

#include <QObject>

class QQuickTextDocument;

namespace Gui {

// The typography of the help pages. A page is Markdown, and Qt shows Markdown with plain defaults. This
// class sets the sizes, the lines, the gaps, the colours, and the table cells of a document that Qt
// has parsed, with the numbers of UiScale::Prose. The call happens one time, after the text is set.
class HelpStyle : public QObject
{
    Q_OBJECT

public:
    explicit HelpStyle(QObject *parent = nullptr) : QObject(parent) {}

    // Style the document of a TextEdit. The colours are text colours in the form #rrggbb.
    Q_INVOKABLE void apply(QQuickTextDocument *document, const QString &fontFamily, const QString &monoFamily,
                           const QColor &headingColor, const QColor &bodyColor, const QColor &mutedColor,
                           const QColor &linkColor, const QColor &codeBackground);
};

}

#endif // GUI_HELPSTYLE_H
