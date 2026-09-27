#pragma once

#include <QColor>
#include <QList>
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
    QColor shade;  // the light background behind quotes and inline code
    qreal bodyPixelSize = 17;
    QString proseFamily;
    QString codeFamily;
    QUrl fileUrl;  // the file shown, which relative images resolve against
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

}  // namespace ReadingRenderer
