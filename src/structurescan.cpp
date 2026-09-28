#include "structurescan.h"

#include <QRegularExpression>

namespace StructureScan {

namespace {

// A fence state packs the open fence's length and character into one int that
// is never 0: lengths start at 3, so the smallest state is 6.
int fenceStateFor(int length, QChar character) {
    return (length << 1) | (character == QLatin1Char('~') ? 1 : 0);
}

}  // namespace

HeadingLine headingLine(const QString &line) {
    if (!line.startsWith(QLatin1Char('#')))
        return {};

    static const QRegularExpression headingRe(QStringLiteral("^(#{1,6})\\s+"));
    const QRegularExpressionMatch match = headingRe.match(line);
    if (!match.hasMatch())
        return {};
    return {static_cast<int>(match.capturedLength(1)), static_cast<int>(match.capturedEnd(0))};
}

LineKind classifyLine(const QString &line, int &fenceState) {
    static const QRegularExpression fenceRe(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    const QRegularExpressionMatch fence = fenceRe.match(line);

    if (fenceState != 0) {
        static const QRegularExpression spacesRe(QStringLiteral("^ *$"));
        if (fence.hasMatch() && spacesRe.match(fence.captured(2)).hasMatch()) {
            const QString marker = fence.captured(1);
            const bool sameCharacter =
                (marker.at(0) == QLatin1Char('~')) == ((fenceState & 1) == 1);
            if (sameCharacter && marker.length() >= fenceState >> 1) {
                fenceState = 0;
                return LineKind::Fence;
            }
        }
        return LineKind::Code;
    }

    if (fence.hasMatch()) {
        const QString marker = fence.captured(1);
        // A backtick fence's info string cannot hold a backtick, or ```x``` on
        // one line would open a block.
        if (marker.at(0) != QLatin1Char('`') || !fence.captured(2).contains(QLatin1Char('`'))) {
            fenceState = fenceStateFor(marker.length(), marker.at(0));
            return LineKind::Fence;
        }
    }

    return headingLine(line).level > 0 ? LineKind::Heading : LineKind::Text;
}

bool opensFrontMatter(const QString &firstLine) {
    static const QRegularExpression openRe(QStringLiteral("^--- *$"));
    return openRe.match(firstLine).hasMatch();
}

bool closesFrontMatter(const QString &line) {
    static const QRegularExpression closeRe(QStringLiteral("^(---|\\.\\.\\.) *$"));
    return closeRe.match(line).hasMatch();
}

Structure scan(const QString &text) {
    Structure structure;
    const QStringList lines = text.split(QLatin1Char('\n'));

    if (!lines.isEmpty() && opensFrontMatter(lines.first())) {
        for (int line = 1; line < lines.size(); ++line) {
            if (closesFrontMatter(lines.at(line))) {
                structure.frontMatterEndLine = line;
                break;
            }
        }
    }

    int fenceState = 0;
    int position = 0;
    for (int line = 0; line < lines.size(); ++line) {
        const QString &content = lines.at(line);
        if (line > structure.frontMatterEndLine
                && classifyLine(content, fenceState) == LineKind::Heading) {
            const HeadingLine heading = headingLine(content);
            structure.headings.append(
                {heading.level, content.mid(heading.textStart), line, position});
        }
        position += content.length() + 1;
    }
    return structure;
}

}  // namespace StructureScan
