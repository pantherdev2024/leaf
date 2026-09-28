#pragma once

#include <QList>
#include <QString>

// The single definition of block-level Markdown in Leaf: which lines are
// headings, which open, close or sit inside a fenced code block, and where front
// matter ends. The highlighter applies the line rules as it restyles; the core
// scans the whole text with them. Text in, structure out, nothing drawn.
namespace StructureScan {

enum class LineKind { Text, Heading, Fence, Code };

struct HeadingLine {
    int level = 0;      // 0 when the line is not a heading
    int textStart = 0;  // where the heading's text begins, after the #s and space
};

// The heading rule on its own, with no knowledge of code or front matter.
HeadingLine headingLine(const QString &line);

// Classifies a line outside front matter. fenceState is what the line before
// left behind -- 0 outside a code block, otherwise the open fence -- and is
// updated for the line after. It is an int so the highlighter can keep it as a
// block state.
LineKind classifyLine(const QString &line, int &fenceState);

// Front matter exists only when the first line opens it and a later line closes it.
bool opensFrontMatter(const QString &firstLine);
bool closesFrontMatter(const QString &line);

struct Heading {
    int level;
    QString text;  // as written, inline markers included
    int line;
    int position;  // of the line's first character, in the whole text
};

struct Structure {
    QList<Heading> headings;
    int frontMatterEndLine = -1;  // the closing line, or -1 with no front matter
};

Structure scan(const QString &text);

}  // namespace StructureScan
