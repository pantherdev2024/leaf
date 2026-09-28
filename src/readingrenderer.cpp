#include "readingrenderer.h"

#include <algorithm>
#include <limits>

#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
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
// How far a code block too wide for the column may shrink to fit, as a share
// of the code size; one that would need more wraps at the code size.
constexpr qreal codeFitFloor = 0.75;
constexpr qreal labelScale = 0.72;

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

int quoteLevel(const QTextBlockFormat &format) {
    return format.intProperty(QTextFormat::BlockQuoteLevel);
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

QString bulletFamily() {
    return QStringLiteral("sans-serif");
}

int bulletPixelSize(const Style &style) {
    QFont prose(style.proseFamily);
    prose.setPixelSize(qRound(style.bodyPixelSize));
    QFont bullet(bulletFamily());
    bullet.setPixelSize(qRound(style.bodyPixelSize));
    return qRound(style.bodyPixelSize * QFontMetricsF(prose).ascent()
                  / QFontMetricsF(bullet).ascent());
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
        // Each line of a code block is a block of its own, and the box around
        // them spaces the block from the prose. Long lines wrap in the box.
        format.setTopMargin(0);
        format.setBottomMargin(0);
        format.setNonBreakableLines(false);
    } else if (isInTable(block)) {
        format.setTopMargin(0);
        format.setBottomMargin(0);
        format.setLineHeight(proseLineHeightPercent, QTextBlockFormat::ProportionalHeight);
    } else if (format.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        format.setTopMargin(body);
        format.setBottomMargin(body);
    } else {
        // Items in one list sit closer together than paragraphs; the last one
        // leaves a paragraph's space before whatever follows the list. A
        // quote's last paragraph leaves it to the quote's box.
        const bool listContinues = isInList(block) && block.next().isValid()
                                   && isInList(block.next());
        const int quote = quoteLevel(format);
        const bool quoteEnds = quote > 0
            && !(block.next().isValid() && quoteLevel(block.next().blockFormat()) > 0);
        format.setTopMargin(0);
        format.setBottomMargin(listContinues ? body * 0.25 : quoteEnds ? 0 : body * 0.7);
        format.setLineHeight(proseLineHeightPercent, QTextBlockFormat::ProportionalHeight);
        // The box sets a quote apart; only a quote within a quote is indented.
        if (quote > 0)
            format.setLeftMargin((quote - 1) * body * 1.5);
    }

    QTextCursor cursor(block);
    cursor.setBlockFormat(format);

    // A list's bullet is drawn in the block's own character format, which the
    // importer leaves empty. The prose font's bullet is a small square, so it
    // comes from the desktop's sans-serif font, sized so its baseline, which
    // the view places by the font's ascent, meets the text's. A number stays
    // in the prose font.
    if (isInList(block)) {
        const QTextListFormat::Style listStyle = block.textList()->format().style();
        const bool numbered = listStyle != QTextListFormat::ListDisc
                              && listStyle != QTextListFormat::ListCircle
                              && listStyle != QTextListFormat::ListSquare;
        QTextCharFormat bullet;
        bullet.setFontFamilies({numbered ? style.proseFamily : bulletFamily()});
        bullet.setProperty(QTextFormat::FontPixelSize,
                           numbered ? qRound(body) : bulletPixelSize(style));
        bullet.setForeground(style.text);
        cursor.mergeBlockCharFormat(bullet);
    }

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

// Every table, including those inside a list or a quote.
QList<QTextTable *> tablesIn(const QTextDocument *document) {
    QList<QTextTable *> tables;
    QList<QTextFrame *> frames = document->rootFrame()->childFrames();
    while (!frames.isEmpty()) {
        QTextFrame *frame = frames.takeFirst();
        frames.append(frame->childFrames());
        if (auto *table = qobject_cast<QTextTable *>(frame))
            tables.append(table);
    }
    return tables;
}

// A grid in faint lines, its header row bold, every header cell aligned as its
// column, and a short row shown with its missing cells empty. The view draws no
// cell backgrounds, so the header's shade is drawn behind it by the window.
void restyleTable(QTextTable *table, const Style &style) {
    const qreal body = style.bodyPixelSize;
    QTextTableFormat format = table->format();
    format.setBorder(1);
    format.setBorderBrush(style.line);
    format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    format.setBorderCollapse(true);
    format.setCellSpacing(0);
    format.setCellPadding(body * 0.4);
    format.setTopMargin(body * 0.3);
    format.setBottomMargin(body * 0.8);
    table->setFormat(format);

    // The importer spans a short row's last cell across the missing ones.
    for (int row = 1; row < table->rows(); ++row) {
        for (int column = 0; column < table->columns(); ++column) {
            const QTextTableCell cell = table->cellAt(row, column);
            if (cell.columnSpan() > 1)
                table->splitCell(row, cell.column(), 1, 1);
        }
    }

    for (int column = 0; column < table->columns(); ++column) {
        const QTextTableCell header = table->cellAt(0, column);
        // The importer sets the file's alignment on body cells only.
        Qt::Alignment alignment = Qt::AlignLeft;
        if (table->rows() > 1) {
            alignment = table->cellAt(1, column).firstCursorPosition().blockFormat().alignment()
                        & Qt::AlignHorizontal_Mask;
        }
        QTextCursor cursor = header.firstCursorPosition();
        cursor.setPosition(header.lastCursorPosition().position(), QTextCursor::KeepAnchor);
        QTextBlockFormat aligned;
        aligned.setAlignment(alignment);
        cursor.mergeBlockFormat(aligned);
        QTextCharFormat bold;
        bold.setFontWeight(QFont::Bold);
        cursor.mergeCharFormat(bold);
    }
}

// A run of blocks that share one box: the lines of one code block, or one
// block quote's paragraphs.
struct Box {
    int start;
    int end;
    bool code;
    QString language;
};

// Qt's importer marks no boundary between two code blocks that follow each
// other, so a change of language or of fencing is taken as one.
QList<Box> findBoxes(QTextDocument *document) {
    QList<Box> boxes;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const QTextBlockFormat format = block.blockFormat();
        const bool quote = quoteLevel(format) > 0;
        const bool code = !quote && isCodeBlock(format);
        if ((!quote && !code) || isInTable(block))
            continue;
        const QString language = format.stringProperty(QTextFormat::BlockCodeLanguage);
        const bool fenced = format.hasProperty(QTextFormat::BlockCodeFence);
        const int end = block.position() + block.length() - 1;
        const QTextBlock previous = block.previous();
        const bool continues = !boxes.isEmpty() && previous.isValid()
            && boxes.last().end == previous.position() + previous.length() - 1
            && boxes.last().code == code
            && (!code || (boxes.last().language == language
                          && previous.blockFormat().hasProperty(QTextFormat::BlockCodeFence) == fenced));
        if (continues)
            boxes.last().end = end;
        else
            boxes.append({block.position(), end, code, language});
    }
    return boxes;
}

// A code block's lines are set smaller when its widest line would not fit the
// box at the code size but would at the floor or above; wrapping would break a
// diagram.
void fitCode(QTextDocument *document, QTextFrame *frame, const Style &style, qreal padding) {
    if (style.columnWidth <= 0)
        return;
    const qreal available = style.columnWidth - 2 * document->documentMargin() - 2 * padding;
    const int codeSize = qRound(style.bodyPixelSize * codeScale);
    const int floorSize = qRound(codeSize * codeFitFloor);

    QString widest;
    qreal widestAdvance = 0;
    QFont font(style.codeFamily);
    font.setPixelSize(codeSize);
    const QFontMetricsF codeMetrics(font);
    for (QTextBlock block = document->findBlock(frame->firstPosition());
         block.isValid() && block.position() < frame->lastPosition(); block = block.next()) {
        const qreal advance = codeMetrics.horizontalAdvance(block.text());
        if (advance > widestAdvance) {
            widestAdvance = advance;
            widest = block.text();
        }
    }
    if (widestAdvance <= available)
        return;

    int size = codeSize;
    do {
        --size;
        font.setPixelSize(size);
    } while (size > floorSize && QFontMetricsF(font).horizontalAdvance(widest) > available);
    // Past the floor the block would be small and still wrap, so it wraps at
    // the code size instead: most such blocks are long lines, not drawings.
    if (QFontMetricsF(font).horizontalAdvance(widest) > available)
        return;
    QTextCursor cursor(document);
    cursor.setPosition(frame->firstPosition());
    cursor.setPosition(frame->lastPosition(), QTextCursor::KeepAnchor);
    QTextCharFormat smaller;
    smaller.setProperty(QTextFormat::FontPixelSize, size);
    cursor.mergeCharFormat(smaller);
}

// Code blocks and quotes are set in shaded boxes. The view does not draw a
// block's own background but does draw a frame's. A frame that is not a table
// must have no border: the view reads any bordered frame as a table and crashes.
void boxBlocks(QTextDocument *document, const Style &style) {
    const qreal body = style.bodyPixelSize;
    const QList<Box> boxes = findBoxes(document);
    // From the end, so each frame leaves the positions still to visit where
    // they were.
    for (auto box = boxes.crbegin(); box != boxes.crend(); ++box) {
        const qreal padding = body * (box->code ? 0.6 : 0.5);
        QTextFrameFormat format;
        format.setBackground(style.shade);
        format.setPadding(padding);
        format.setMargin(0);
        format.setWidth(QTextLength(QTextLength::PercentageLength, 100));

        // The frame takes the first block's format for the empty block it
        // leaves in front of itself, so the block gets it back.
        const QTextBlockFormat firstFormat = document->findBlock(box->start).blockFormat();
        QTextCursor cursor(document);
        cursor.setPosition(box->start);
        cursor.setPosition(box->end, QTextCursor::KeepAnchor);
        QTextFrame *frame = cursor.insertFrame(format);
        frame->firstCursorPosition().setBlockFormat(firstFormat);
        if (!box->code)
            continue;

        fitCode(document, frame, style, padding);
        if (box->language.isEmpty())
            continue;

        // The language, small and dim, on a line of its own at the top right.
        cursor.setPosition(frame->firstPosition());
        const QTextBlockFormat codeFormat = cursor.blockFormat();
        cursor.insertBlock(codeFormat, cursor.charFormat());
        cursor.movePosition(QTextCursor::PreviousBlock);
        QTextBlockFormat labelFormat;
        labelFormat.setAlignment(Qt::AlignRight);
        cursor.setBlockFormat(labelFormat);
        QTextCharFormat label;
        label.setFontFamilies({style.codeFamily});
        label.setProperty(QTextFormat::FontPixelSize, qRound(body * labelScale));
        label.setForeground(style.dim);
        cursor.insertText(box->language, label);
    }
}

// The document keeps an empty block after every table and box, and before
// every box. The view draws a frame's background over its margins too, so these
// blocks, set to a fixed height, are the space between a box and its
// neighbours: a little before it and more after it. A table has no
// background, and its own margins space it.
void spaceFrames(QTextDocument *document, qreal body) {
    QTextFrame *root = document->rootFrame();
    const auto inFrame = [root](const QTextBlock &block) {
        return block.isValid() && QTextCursor(block).currentFrame() != root;
    };
    QTextCharFormat tiny;
    tiny.setProperty(QTextFormat::FontPixelSize, 1);
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const bool beforeFrame = inFrame(block.next());
        if (block.length() > 1 || inFrame(block) || !(beforeFrame || inFrame(block.previous())))
            continue;
        const bool afterTable = QTextCursor(block.previous()).currentTable() != nullptr;
        QTextBlockFormat gap;
        gap.setTopMargin(0);
        gap.setBottomMargin(0);
        gap.setLineHeight(body * (beforeFrame ? 0.4 : afterTable ? 0 : 0.8),
                          QTextBlockFormat::FixedHeight);
        QTextCursor cursor(block);
        cursor.setBlockFormat(gap);
        cursor.setBlockCharFormat(tiny);
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
    for (QTextTable *table : tablesIn(document))
        restyleTable(table, style);
    boxBlocks(document, style);
    spaceFrames(document, style.bodyPixelSize);
    edit.endEditBlock();
}

QList<QRectF> headerRows(const QTextDocument *document) {
    QList<QRectF> rows;
    QAbstractTextDocumentLayout *layout = document->documentLayout();
    for (QTextTable *table : tablesIn(document)) {
        if (table->rows() == 0)
            continue;

        // Qt's public layout gives no cell's box, only its lines', so the row
        // is found from its cells' lines and the padding around them, and ends
        // where the next row's lines start.
        const qreal padding = table->format().cellPadding();
        const auto cellLines = [&](int row, int column) {
            const QTextTableCell cell = table->cellAt(row, column);
            QRectF lines;
            for (QTextBlock block = cell.firstCursorPosition().block();
                 block.isValid() && block.position() <= cell.lastPosition(); block = block.next())
                lines = lines.united(layout->blockBoundingRect(block));
            return lines;
        };
        QRectF row;
        for (int column = 0; column < table->columns(); ++column)
            row = row.united(cellLines(0, column));
        row.adjust(-padding, -padding, padding, padding);
        if (table->rows() > 1)
            row.setBottom(cellLines(1, 0).top() - padding);
        rows.append(row);
    }
    return rows;
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

QList<int> matchHeadings(const QList<OutlineEntry> &outline,
                         const QList<RenderedHeading> &rendered) {
    QList<int> matches;
    int next = 0;
    for (const OutlineEntry &entry : outline) {
        const QString title = entry.title.simplified();
        int found = -1;
        for (int candidate = next; candidate < rendered.size(); ++candidate) {
            if (rendered.at(candidate).level == entry.level && rendered.at(candidate).text == title) {
                found = candidate;
                break;
            }
        }
        matches.append(found);
        if (found >= 0)
            next = found + 1;
    }
    return matches;
}

}  // namespace ReadingRenderer
