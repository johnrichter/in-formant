#include "helpstyle.h"
#include "uiscale.h"

#include <QColor>
#include <QFont>
#include <QQuickTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextList>
#include <QTextTable>

using namespace Gui;

namespace {

QTextCharFormat sizeFormat(int pixels, const QString &family, int weight, const QColor &color)
{
    QTextCharFormat f;
    f.setFontFamilies({ family });
    f.setProperty(QTextFormat::FontPixelSize, pixels);
    f.setFontWeight(weight);
    f.setForeground(color);
    return f;
}

}

void HelpStyle::apply(QQuickTextDocument *quickDocument, const QString &fontFamily, const QString &monoFamily,
                      const QColor &headingColor, const QColor &bodyColor, const QColor &mutedColor,
                      const QColor &linkColor, const QColor &codeBackground)
{
    if (!quickDocument) return;
    QTextDocument *doc = quickDocument->textDocument();
    if (!doc) return;

    using P = UiScale::Prose;
    doc->setDocumentMargin(0);
    doc->setIndentWidth(P::listIndent);

    // One edit block for all changes. Without it, each change makes the text item lay out the whole
    // page again, and the start of the application takes about 12 seconds.
    QTextCursor editBlock(doc);
    editBlock.beginEditBlock();

    // Tables first, so the blocks in the cells can see the table.
    QList<QTextTable *> tables;
    for (QTextFrame *frame : doc->rootFrame()->childFrames()) {
        if (auto *table = qobject_cast<QTextTable *>(frame)) tables << table;
    }
    for (QTextTable *table : tables) {
        QTextTableFormat format = table->format();
        format.setBorder(0);
        format.setBorderCollapse(true);
        format.setCellPadding(P::cellPadding);
        format.setCellSpacing(0);
        format.setWidth(QTextLength(QTextLength::PercentageLength, 100));
        format.setBottomMargin(P::paragraphGap);
        table->setFormat(format);

        for (int row = 0; row < table->rows(); ++row) {
            for (int col = 0; col < table->columns(); ++col) {
                QTextTableCell cell = table->cellAt(row, col);
                QTextCharFormat cellFormat = cell.format();
                // A thin line under each row. The first row is the header, and its line is stronger.
                QTextTableCellFormat tcf = cellFormat.toTableCellFormat();
                tcf.setBottomBorder(1);
                tcf.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
                tcf.setBottomBorderBrush(row == 0 ? mutedColor : QColor(255, 255, 255, 40));
                cell.setFormat(tcf);
            }
        }
    }

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat bf = block.blockFormat();
        const int level = bf.headingLevel();
        const bool inTable = QTextCursor(block).currentTable() != nullptr;
        const bool inList = block.textList() != nullptr;
        const bool isCode = bf.hasProperty(QTextFormat::BlockCodeFence) || bf.nonBreakableLines();
        const bool isQuote = bf.hasProperty(QTextFormat::BlockQuoteLevel);

        int size = P::bodySize, line = P::bodyLine, top = 0, bottom = P::paragraphGap;
        int weight = QFont::Normal;
        QColor color = bodyColor;
        QString family = fontFamily;

        if (level == 1) { size = P::h1Size; line = P::h1Line; bottom = P::h1Below; weight = QFont::Bold; color = headingColor; }
        else if (level == 2) { size = P::h2Size; line = P::h2Line; top = P::h2Above - P::paragraphGap; bottom = P::h2Below; weight = QFont::Bold; color = headingColor; }
        else if (level >= 3) { size = P::h3Size; line = P::h3Line; top = P::h3Above - P::paragraphGap; bottom = P::h3Below; weight = QFont::Bold; color = headingColor; }
        else if (isCode) { size = P::codeSize; line = P::codeLine; family = monoFamily; bottom = 0; }
        else if (isQuote) { color = mutedColor; }

        if (inTable) {
            size = P::codeSize; line = P::codeLine; top = 0; bottom = 0;
            // The first row of the table is the header.
            QTextTable *table = QTextCursor(block).currentTable();
            const bool header = table->cellAt(QTextCursor(block)).row() == 0;
            weight = header ? QFont::Bold : QFont::Normal;
            color = header ? headingColor : bodyColor;
        }
        else if (inList) {
            // A gap between the items. The last item of a list has the gap of a paragraph.
            const QTextBlock next = block.next();
            const bool lastItem = !next.isValid() || next.textList() != block.textList();
            bottom = lastItem ? P::paragraphGap : P::listItemGap;
            top = 0;
        }

        bf.setTopMargin(top);
        bf.setBottomMargin(bottom);
        bf.setLineHeight(line, QTextBlockFormat::FixedHeight);
        if (isCode) {
            bf.setBackground(codeBackground);
            bf.setLeftMargin(P::cellPadding);
            bf.setRightMargin(P::cellPadding);
        }
        if (isQuote) bf.setLeftMargin(P::listIndent);

        QTextCursor cursor(block);
        cursor.setBlockFormat(bf);

        // The characters. A fragment keeps its own weight when it is bold, and its own family when it is code.
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid()) continue;
            QTextCharFormat cf = fragment.charFormat();

            const bool inlineCode = cf.fontFixedPitch() || cf.fontFamilies().toStringList().contains(QStringLiteral("monospace"))
                || cf.fontFamilies().toStringList().contains(monoFamily);
            const bool bold = cf.fontWeight() >= QFont::DemiBold;

            QTextCharFormat out = sizeFormat(inlineCode && level == 0 && !isCode ? P::codeSize : size,
                                             inlineCode || isCode ? monoFamily : family,
                                             (bold || weight >= QFont::DemiBold) ? QFont::Bold : QFont::Normal,
                                             (bold && level == 0 && !inTable) ? headingColor : color);
            if (cf.isAnchor()) {
                out.setForeground(linkColor);
                out.setFontUnderline(true);
            }
            if (inlineCode && !isCode) {
                out.setBackground(codeBackground);
            }
            QTextCursor fragmentCursor(doc);
            fragmentCursor.setPosition(fragment.position());
            fragmentCursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            fragmentCursor.mergeCharFormat(out);
        }
    }

    editBlock.endEditBlock();
}
