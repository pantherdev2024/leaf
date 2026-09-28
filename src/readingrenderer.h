#pragma once

#include <QColor>
#include <QList>
#include <QRectF>
#include <QString>
#include <QUrl>

class QTextDocument;

// Builds the reading view: the text drawn as a finished page by Qt's Markdown
// importer, then restyled to Leaf's look. What it builds is only ever shown,
// never saved or read back into the text. Text and style in, document out.
namespace ReadingRenderer {

struct Style {
    QColor text;
    QColor accent;
    QColor shade;  // the light background behind quotes, code and table headers
    QColor line;   // table grid lines
    QColor dim;    // a code block's language label
    qreal bodyPixelSize = 17;
    QString proseFamily;
    QString codeFamily;
    QUrl fileUrl;  // the file shown, which relative images resolve against
    // The width the page is laid out in. A code block too wide for it at the
    // code size is set smaller, so a diagram keeps its shape; 0 leaves it.
    qreal columnWidth = 0;
};

struct RenderedHeading {
    int level;
    QString text;  // as shown, with runs of spaces collapsed
    int block;     // the heading's block number in the rendered document
};

// The text with front matter and tag lines (`<task>`, `</context>`) blanked,
// line for line, and elements that never close (`<br>`) written closed, so Qt's
// importer neither shows the lines nor swallows what follows them.
QString displayCopy(const QString &text);

// Replaces the document's contents with the rendered, restyled text.
void render(QTextDocument *document, const QString &text, const Style &style);

QList<RenderedHeading> headings(const QTextDocument *document);

struct OutlineEntry {
    int level;
    QString title;  // as the outline shows it, Markdown marks removed
};

// For each outline entry, the index of its rendered heading, or -1. Matched in
// order, by level and by text with runs of spaces counted as one. A rendered
// heading the outline does not list, such as an underlined one, is passed over;
// an entry with no match leaves the next entry to search from the same place.
QList<int> matchHeadings(const QList<OutlineEntry> &outline,
                         const QList<RenderedHeading> &rendered);

// Where each table's header row sits in the laid-out document, for the view to
// shade: it draws no cell backgrounds of its own.
QList<QRectF> headerRows(const QTextDocument *document);

// Gives each table's header cells the shade as their own background, which Qt's
// printing draws, unlike the reading view.
void shadeHeaderCells(QTextDocument *document, const QColor &shade);

}  // namespace ReadingRenderer
