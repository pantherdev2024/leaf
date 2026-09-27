#include "readingrenderer.h"

#include <algorithm>

#include <QFileInfo>
#include <QFont>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextList>
#include <QTextTable>

#include "markdownhighlighter.h"
#include "structurescan.h"

namespace ReadingRenderer {

namespace {

// Heading sizes as multiples of the body size, level one first.
constexpr qreal headingScale[] = {2.0, 1.55, 1.3, 1.12, 1.0, 1.0};
constexpr qreal proseLineHeightPercent = 140;
constexpr qreal headingLineHeightPercent = 120;
constexpr qreal codeScale = 0.88;

// A single opening or closing tag, attributes allowed, alone on its line.
bool isTagLine(const QString &line) {
    static const QRegularExpression tagRe(
        QStringLiteral("^\\s*</?[A-Za-z][A-Za-z0-9_-]*(\\s[^<>]*)?>\\s*$"));
    return tagRe.match(line).hasMatch();
}

// Qt's importer drops everything after an HTML element that never closes, such
// as `<br>`, unless it is written closed, `<br/>`. Inline code shows its text as
// written, so tags inside it are left alone.
QString closeVoidTags(const QString &line) {
    static const QRegularExpression voidTagRe(
        QStringLiteral("<(area|base|br|col|embed|hr|img|input|link|meta|source|track|wbr)"
                       "\\b([^<>]*?)(?<!/)>"),
        QRegularExpression::CaseInsensitiveOption);
    if (!line.contains(QLatin1Char('<')))
        return line;

    const QList<MarkdownHighlighter::Span> code = MarkdownHighlighter::inlineCodeSpans(line);
    QString closed = line;
    QRegularExpressionMatchIterator matches = voidTagRe.globalMatch(line);
    QList<int> ends;
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const bool insideCode = std::any_of(code.cbegin(), code.cend(),
            [&match](const MarkdownHighlighter::Span &span) {
                return match.capturedStart() >= span.start
                       && match.capturedStart() < span.start + span.length;
            });
        if (!insideCode)
            ends.append(match.capturedEnd() - 1);
    }
    // From the end, so each insertion leaves the earlier positions where they were.
    for (auto it = ends.crbegin(); it != ends.crend(); ++it)
        closed.insert(*it, QLatin1Char('/'));
    return closed;
}

// Qt's importer gives every code block line a language, empty when none is
// named, fenced or indented alike.
bool isCodeBlock(const QTextBlockFormat &format) {
    return format.hasProperty(QTextFormat::BlockCodeLanguage);
}

bool isInList(const QTextBlock &block) {
    return block.textList() != nullptr;
}

bool isInTable(const QTextBlock &block) {
    return QTextCursor(block).currentTable() != nullptr;
}

// Images load only from this machine. Anything else -- a web image, a missing
// file -- is replaced by its alternative text, so nothing is fetched and the
// page still says what was there.
void resolveImages(QTextDocument *document, const QUrl &fileUrl) {
    struct Image {
        int position;
        QTextImageFormat format;
    };
    QList<Image> images;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (fragment.charFormat().isImageFormat())
                images.append({fragment.position(), fragment.charFormat().toImageFormat()});
        }
    }

    // From the end, so replacing an image with longer text leaves the positions
    // still to visit where they were.
    for (auto it = images.crbegin(); it != images.crend(); ++it) {
        QTextCursor cursor(document);
        cursor.setPosition(it->position);
        cursor.setPosition(it->position + 1, QTextCursor::KeepAnchor);

        const QUrl resolved = fileUrl.resolved(QUrl(it->format.name()));
        if (resolved.isLocalFile() && QFileInfo::exists(resolved.toLocalFile())) {
            QTextImageFormat format = it->format;
            format.setName(resolved.toString());
            cursor.setCharFormat(format);
            continue;
        }

        QString alternative = it->format.stringProperty(QTextFormat::ImageAltText);
        if (alternative.isEmpty())
            alternative = it->format.name();
        QTextCharFormat plain = it->format;
        plain.setObjectType(QTextFormat::NoObject);
        for (const int property : {int(QTextFormat::ImageName), int(QTextFormat::ImageAltText),
                                   int(QTextFormat::ImageTitle), int(QTextFormat::ImageWidth),
                                   int(QTextFormat::ImageHeight)})
            plain.clearProperty(property);
        cursor.insertText(alternative, plain);
    }
}

void restyleBlock(const QTextBlock &block, const Style &style, bool firstBlock) {
    const qreal body = style.bodyPixelSize;
    QTextBlockFormat format = block.blockFormat();
    const int level = format.headingLevel();
    const bool code = isCodeBlock(format);

    // Qt's importer leaves a task item's checkbox marker set on blocks after
    // it, which then draw a checkbox of their own.
    if (!isInList(block) && format.marker() != QTextBlockFormat::MarkerType::NoMarker)
        format.setMarker(QTextBlockFormat::MarkerType::NoMarker);

    if (level > 0) {
        format.setTopMargin(firstBlock ? 0 : body * headingScale[level - 1] * 0.9);
        format.setBottomMargin(body * 0.4);
        format.setLineHeight(headingLineHeightPercent, QTextBlockFormat::ProportionalHeight);
    } else if (code) {
        // Each line of a code block is a block of its own; only the block's
        // outer lines take space from the prose around it.
        const bool codeBefore = block.previous().isValid()
                                && isCodeBlock(block.previous().blockFormat());
        const bool codeAfter = block.next().isValid() && isCodeBlock(block.next().blockFormat());
        format.setTopMargin(codeBefore ? 0 : body * 0.6);
        format.setBottomMargin(codeAfter ? 0 : body * 0.6);
    } else if (isInTable(block)) {
        format.setTopMargin(0);
        format.setBottomMargin(0);
        format.setLineHeight(proseLineHeightPercent, QTextBlockFormat::ProportionalHeight);
    } else if (format.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        format.setTopMargin(body);
        format.setBottomMargin(body);
    } else {
        // Items in one list sit closer together than paragraphs; the last one
        // leaves a paragraph's space before whatever follows the list.
        const bool listContinues = isInList(block) && block.next().isValid()
                                   && isInList(block.next());
        format.setTopMargin(0);
        format.setBottomMargin(listContinues ? body * 0.25 : body * 0.7);
        format.setLineHeight(proseLineHeightPercent, QTextBlockFormat::ProportionalHeight);
        if (format.intProperty(QTextFormat::BlockQuoteLevel) > 0)
            format.setBackground(style.shade);
    }

    QTextCursor cursor(block);
    cursor.setBlockFormat(format);

    struct Span {
        int position;
        int length;
        QTextCharFormat format;
    };
    QList<Span> spans;
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        const QTextFragment fragment = it.fragment();
        QTextCharFormat chars = fragment.charFormat();
        // Sizes are set here in pixels, so the importer's own sizes go: a point
        // size on code, and a heading's size step, which it drops on bold words.
        chars.clearProperty(QTextFormat::FontPointSize);
        chars.clearProperty(QTextFormat::FontSizeAdjustment);
        chars.setForeground(style.text);

        qreal pixelSize = body;
        if (level > 0) {
            pixelSize = body * headingScale[level - 1];
            chars.setFontWeight(QFont::Bold);
        }
        if (chars.fontFixedPitch() || code) {
            chars.setFontFamilies({style.codeFamily});
            pixelSize *= codeScale;
            if (!code)
                chars.setBackground(style.shade);
        }
        if (chars.isAnchor()) {
            chars.setForeground(style.accent);
            chars.setFontUnderline(true);
        }
        chars.setProperty(QTextFormat::FontPixelSize, qRound(pixelSize));
        spans.append({fragment.position(), fragment.length(), chars});
    }

    // Applied after the walk: changing formats merges and splits fragments.
    for (const Span &span : spans) {
        cursor.setPosition(span.position);
        cursor.setPosition(span.position + span.length, QTextCursor::KeepAnchor);
        cursor.setCharFormat(span.format);
    }
}

}  // namespace

QString displayCopy(const QString &text) {
    QStringList lines = text.split(QLatin1Char('\n'));
    const int frontMatterEnd = StructureScan::scan(text).frontMatterEndLine;

    int fenceState = 0;
    for (int line = 0; line < lines.size(); ++line) {
        if (line <= frontMatterEnd) {
            lines[line].clear();
            continue;
        }
        const StructureScan::LineKind kind = StructureScan::classifyLine(lines.at(line), fenceState);
        if (kind == StructureScan::LineKind::Text && isTagLine(lines.at(line)))
            lines[line].clear();
        else if (kind == StructureScan::LineKind::Text || kind == StructureScan::LineKind::Heading)
            lines[line] = closeVoidTags(lines.at(line));
    }
    return lines.join(QLatin1Char('\n'));
}

void render(QTextDocument *document, const QString &text, const Style &style) {
    document->setMarkdown(displayCopy(text), QTextDocument::MarkdownDialectGitHub);

    QFont font(style.proseFamily);
    font.setPixelSize(qRound(style.bodyPixelSize));
    document->setDefaultFont(font);
    document->setIndentWidth(style.bodyPixelSize * 1.5);

    // One edit for the whole restyle, so a view showing the document lays it out
    // once rather than after every change.
    QTextCursor edit(document);
    edit.beginEditBlock();
    resolveImages(document, style.fileUrl);
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
        restyleBlock(block, style, block == document->begin());
    edit.endEditBlock();
}

QList<RenderedHeading> headings(const QTextDocument *document) {
    QList<RenderedHeading> found;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const int level = block.blockFormat().headingLevel();
        if (level == 0)
            continue;
        QString text = block.text();
        text.remove(QChar::ObjectReplacementCharacter);
        found.append({level, text.simplified(), block.blockNumber()});
    }
    return found;
}

}  // namespace ReadingRenderer
