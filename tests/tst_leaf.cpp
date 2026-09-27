#include <QtTest>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickTextDocument>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

#include "backend.h"
#include "markdownhighlighter.h"
#include "structurescan.h"

class LeafTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QVERIFY(m_settingsDirectory.isValid());
        // Recovery snapshots go to a folder deleted after the run, so a failed test
        // cannot leave one behind for the next run's windows to recover.
        QVERIFY(m_dataDirectory.isValid());
        qputenv("XDG_DATA_HOME", m_dataDirectory.path().toLocal8Bit());
        QQuickStyle::setStyle(QStringLiteral("Material"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_settingsDirectory.path());
    }

    void countsWords() {
        QCOMPARE(Backend::countWords(QStringLiteral("one two-three don't 42")), 4);
        QCOMPARE(Backend::countWords(QStringLiteral("你好 世界")), 2);
        QCOMPARE(Backend::countWords(QString()), 0);
    }

    void countsLines() {
        QCOMPARE(Backend::countLines(QString()), 0);
        QCOMPARE(Backend::countLines(QStringLiteral("one")), 1);
        QCOMPARE(Backend::countLines(QStringLiteral("one\n")), 1);
        QCOMPARE(Backend::countLines(QStringLiteral("one\ntwo")), 2);
        QCOMPARE(Backend::countLines(QStringLiteral("one\n\nthree\n")), 3);
    }

    void estimatesTokensAtFourCharactersEachRoundingHalvesUp() {
        QCOMPARE(Backend::estimateTokens(QString()), 0);
        QCOMPARE(Backend::estimateTokens(QStringLiteral("12345")), 1);
        QCOMPARE(Backend::estimateTokens(QStringLiteral("123456")), 2);
        QCOMPARE(Backend::estimateTokens(QStringLiteral("1234567890")), 3);
    }

    void showsStatsOfALoadedDocumentAtOnce() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("stats.md")),
                                       statsDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));

        // Front matter counts toward words, lines and tokens; the fenced # line is
        // not a section.
        QCOMPARE(backend.wordCount(), 5);
        QCOMPARE(backend.lineCount(), 8);
        QCOMPARE(backend.tokenEstimate(), 12);
        QCOMPARE(backend.sectionCount(), 2);
    }

    void countsWindowsLineEndingsAsTheEditorHoldsThem() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QString crlf = statsDocument();
        crlf.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
        const QString path = writeFile(directory.filePath(QStringLiteral("crlf.md")), crlf);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));

        QCOMPARE(backend.lineCount(), 8);
        QCOMPARE(backend.tokenEstimate(), 12);
        QCOMPARE(backend.sectionCount(), 2);
    }

    void updatesStatCardsAfterAnEdit() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("stats.md")),
                                       statsDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        QCOMPARE(cardText(window.data(), "sections"), QStringLiteral("2"));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QTextDocument *document =
            editor->property("textDocument").value<QQuickTextDocument *>()->textDocument();
        QTextCursor cursor(document);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(QStringLiteral("# Three\n"));

        QTRY_COMPARE(cardText(window.data(), "sections"), QStringLiteral("3"));
        QCOMPARE(cardText(window.data(), "words"), QStringLiteral("6"));
        QCOMPARE(cardText(window.data(), "lines"), QStringLiteral("9"));
        QCOMPARE(cardText(window.data(), "tokens"), QStringLiteral("≈ 14"));
    }

    void writesCardNumbersWithCommas() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("long.md")),
                                       QStringLiteral("word ").repeated(1234));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));

        QCOMPARE(cardText(window.data(), "words"), QStringLiteral("1,234"));
        QCOMPARE(cardText(window.data(), "tokens"), QStringLiteral("≈ 1,543"));
    }

    void normalizesLinks() {
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("www.example.com/path")),
                 QStringLiteral("https://www.example.com/path"));
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("mailto:writer@example.com")),
                 QStringLiteral("mailto:writer@example.com"));
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("example.com")).isEmpty());
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("file:///tmp/private")).isEmpty());
    }

    void suggestsSafeNames() {
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("My first draft\nBody")),
                 QStringLiteral("My first draft.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("A/B")), QStringLiteral("A-B.md"));
        QCOMPARE(Backend::suggestedFileName(QString()), QStringLiteral("Untitled.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("Already.md")),
                 QStringLiteral("Already.md"));
    }

    void findsInlineMarkdownRanges() {
        const auto markup = MarkdownHighlighter::inlineMarkup(
            QStringLiteral("**bold** and *italic* and [site](https://example.com)"));
        QCOMPARE(markup.size(), 3);
        QCOMPARE(markup.at(0).content.start, 2);
        QCOMPARE(markup.at(0).content.length, 4);
        QCOMPARE(markup.at(2).content.length, 4);
        QCOMPARE(markup.at(2).markers[0].length, 1);
    }

    void findsHeadingsWithLevelsTextAndPositions() {
        const auto structure = StructureScan::scan(
            QStringLiteral("# One\ntext\n### Three\n######\tSix\n"));
        QCOMPARE(structure.headings.size(), 3);
        QCOMPARE(structure.headings.at(0).level, 1);
        QCOMPARE(structure.headings.at(0).text, QStringLiteral("One"));
        QCOMPARE(structure.headings.at(0).line, 0);
        QCOMPARE(structure.headings.at(0).position, 0);
        QCOMPARE(structure.headings.at(1).level, 3);
        QCOMPARE(structure.headings.at(1).text, QStringLiteral("Three"));
        QCOMPARE(structure.headings.at(1).line, 2);
        QCOMPARE(structure.headings.at(1).position, 11);
        QCOMPARE(structure.headings.at(2).level, 6);
        QCOMPARE(structure.headings.at(2).text, QStringLiteral("Six"));
        QCOMPARE(structure.headings.at(2).position, 21);
        QCOMPARE(structure.frontMatterEndLine, -1);
    }

    void leavesOutLinesThatAreNotHeadings() {
        const auto structure = StructureScan::scan(
            QStringLiteral("#x\n####### seven\n # indented\nplain"));
        QCOMPARE(structure.headings.size(), 0);
    }

    void ignoresHeadingsInsideFencedCode() {
        const auto structure = StructureScan::scan(QStringLiteral(
            "```python\n# comment\n```\n~~~\n# tilde\n~~~\n# After"));
        QCOMPARE(headingTexts(structure), QStringList{QStringLiteral("After")});
        QCOMPARE(structure.headings.at(0).line, 6);
    }

    void closesFenceOnlyWithSameCharacterAtLeastAsLong() {
        const auto structure = StructureScan::scan(QStringLiteral(
            "````\n```\n# inside\n~~~~\n# inside too\n````\t\n# inside still\n```` \n# After"));
        QCOMPARE(headingTexts(structure), QStringList{QStringLiteral("After")});
    }

    void opensFencesOnlyWithUpToThreeSpacesOfIndent() {
        const auto structure = StructureScan::scan(QStringLiteral(
            "   ```\n# inside\n   ```\n    ```\n# After\n```x```\n# Last"));
        QCOMPARE(headingTexts(structure),
                 (QStringList{QStringLiteral("After"), QStringLiteral("Last")}));
    }

    void hidesHeadingsAfterAnUnclosedFence() {
        const auto structure = StructureScan::scan(
            QStringLiteral("# Before\n```\n# Hidden\n# Also hidden"));
        QCOMPARE(headingTexts(structure), QStringList{QStringLiteral("Before")});
    }

    void ignoresHeadingsInsideFrontMatter() {
        const auto dashes = StructureScan::scan(
            QStringLiteral("--- \ntitle: x\n# not\n---  \n# Real"));
        QCOMPARE(dashes.frontMatterEndLine, 3);
        QCOMPARE(headingTexts(dashes), QStringList{QStringLiteral("Real")});

        const auto dots = StructureScan::scan(QStringLiteral("---\n# not\n...\n# Real"));
        QCOMPARE(dots.frontMatterEndLine, 2);
        QCOMPARE(headingTexts(dots), QStringList{QStringLiteral("Real")});
    }

    void readsUnclosedFrontMatterAsMarkdown() {
        const auto unclosed = StructureScan::scan(QStringLiteral("---\n# Real\ntext"));
        QCOMPARE(unclosed.frontMatterEndLine, -1);
        QCOMPARE(headingTexts(unclosed), QStringList{QStringLiteral("Real")});

        const auto notFirst = StructureScan::scan(
            QStringLiteral("text\n---\n# A\n---\n# B"));
        QCOMPARE(notFirst.frontMatterEndLine, -1);
        QCOMPARE(headingTexts(notFirst), (QStringList{QStringLiteral("A"), QStringLiteral("B")}));
    }

    void drawsHashLinesInCodeAndFrontMatterPlain() {
        QTextDocument document;
        // Without a layout, as the editor's document always has, the document does
        // not report edits and the highlighter never restyles.
        document.documentLayout();
        MarkdownHighlighter highlighter(&document);
        // A new highlighter ignores edits until its first pass, queued on the event loop.
        QCoreApplication::processEvents();
        document.setPlainText(QStringLiteral(
            "---\n# meta\n---\n# Real\n```\n# code\n```\n# After"));

        QVERIFY(!drawnAsHeading(document, 1));
        QVERIFY(drawnAsHeading(document, 3));
        QVERIFY(!drawnAsHeading(document, 5));
        QVERIFY(drawnAsHeading(document, 7));
    }

    void restylesLinesBelowWhenAFenceIsTypedOrRemoved() {
        QTextDocument document;
        // Without a layout, as the editor's document always has, the document does
        // not report edits and the highlighter never restyles.
        document.documentLayout();
        MarkdownHighlighter highlighter(&document);
        // A new highlighter ignores edits until its first pass, queued on the event loop.
        QCoreApplication::processEvents();
        document.setPlainText(QStringLiteral("intro\n# Heading"));
        QVERIFY(drawnAsHeading(document, 1));

        QTextCursor cursor(&document);
        cursor.insertText(QStringLiteral("```\n"));
        QVERIFY(!drawnAsHeading(document, 2));

        cursor.setPosition(0);
        cursor.setPosition(4, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        QVERIFY(drawnAsHeading(document, 1));
    }

    void restylesFrontMatterThatMovedWhileTheFirstLineWasEdited() {
        QTextDocument document;
        document.documentLayout();
        MarkdownHighlighter highlighter(&document);
        QCoreApplication::processEvents();
        document.setPlainText(QStringLiteral("---\n---\nx\n# b\n---"));
        QVERIFY(drawnAsHeading(document, 3));

        // Deleting the first closing line moves the end to the last line, and an edit
        // to the first line before the refresh must not hide that from it.
        QTextCursor cursor(document.findBlockByNumber(1));
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        QTextCursor firstLine(document.firstBlock());
        firstLine.movePosition(QTextCursor::EndOfBlock);
        firstLine.insertText(QStringLiteral(" "));
        highlighter.refreshFrontMatter();

        QVERIFY(!drawnAsHeading(document, 2));
    }

    void restylesFrontMatterWhenItsClosingLineIsTyped() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("front.md")),
                                       QStringLiteral("---\n# meta\ntext\n# Real\n"));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QTextDocument *document =
            editor->property("textDocument").value<QQuickTextDocument *>()->textDocument();
        QVERIFY(drawnAsHeading(*document, 1));

        QTextCursor cursor(document->findBlockByNumber(2));
        cursor.insertText(QStringLiteral("---\n"));

        QTRY_VERIFY(!drawnAsHeading(*document, 1));
        QVERIFY(drawnAsHeading(*document, 4));
    }

    void loadsCurrentOmarchyTheme() {
        QTemporaryDir homeDirectory;
        QVERIFY(homeDirectory.isValid());

        const QByteArray originalHome = qgetenv("HOME");
        struct HomeRestorer {
            QByteArray value;
            ~HomeRestorer() { qputenv("HOME", value); }
        } restoreHome{originalHome};
        QVERIFY(qputenv("HOME", homeDirectory.path().toUtf8()));

        const QString themeDirectory = homeDirectory.path()
            + QStringLiteral("/.local/state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDirectory));

        QFile colorsFile(themeDirectory + QStringLiteral("/colors.toml"));
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray palette(
            "mode = \"light\"\n"
            "accent = \"#112233\"\n"
            "selection = \"#445566\"\n"
            "background = \"#fefefe\"\n"
            "foreground = \"#101010\"\n");
        QCOMPARE(colorsFile.write(palette), qint64(palette.size()));
        colorsFile.close();

        Backend backend;
        QCOMPARE(backend.themeBackground(), QStringLiteral("#fefefe"));
        QCOMPARE(backend.themeForeground(), QStringLiteral("#101010"));
        QCOMPARE(backend.themeAccent(), QStringLiteral("#112233"));
        QCOMPARE(backend.themeSelection(), QStringLiteral("#445566"));
        QVERIFY(!backend.darkMode());
    }

    void ignoresFileWatcherEventsForSavedContents() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("first-save.md"));
        Backend backend;
        QSignalSpy externalChangeSpy(&backend, &Backend::externalChangeDetected);

        backend.saveAs(QUrl::fromLocalFile(path));
        QVERIFY(QFileInfo::exists(path));

        QFile sameContents(path);
        QVERIFY(sameContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        sameContents.close();
        QTest::qWait(100);
        QCOMPARE(externalChangeSpy.count(), 0);

        QFile changedContents(path);
        QVERIFY(changedContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(changedContents.write("changed elsewhere"), qint64(17));
        changedContents.close();
        QTRY_COMPARE(externalChangeSpy.count(), 1);
    }

    void keepsCursorAndSelectionStableAcrossInsertions() {
        const QString mutationsPath = QFINDTESTDATA("../src/EditorMutations.js");
        QVERIFY(!mutationsPath.isEmpty());

        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QByteArray harness = R"QML(
            import QtQuick
            import "EditorMutations.js" as EditorMutations

            TextEdit {
                property string insertionText
                property int insertionCursor
                property string wrappedText
                property int wrappedSelectionStart
                property int wrappedSelectionEnd

                Component.onCompleted: {
                    text = "alpha omega";
                    cursorPosition = 5;
                    EditorMutations.replaceRange(this, 5, 5, "one\r\ntwo");
                    insertionText = text;
                    insertionCursor = cursorPosition;

                    text = "alpha beta omega";
                    select(6, 10);
                    EditorMutations.replaceRange(this, selectionStart, selectionEnd,
                                                 "**beta**", 2, 6);
                    wrappedText = text;
                    wrappedSelectionStart = selectionStart;
                    wrappedSelectionEnd = selectionEnd;
                }
            }
        )QML";
        const QUrl harnessUrl = QUrl::fromLocalFile(
            QFileInfo(mutationsPath).absolutePath() + QStringLiteral("/MutationHarness.qml"));
        component.setData(harness, harnessUrl);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        QVERIFY2(editor, qPrintable(component.errorString()));

        QCOMPARE(editor->property("insertionText").toString(),
                 QStringLiteral("alphaone\ntwo omega"));
        QCOMPARE(editor->property("insertionCursor").toInt(), 12);
        QCOMPARE(editor->property("wrappedText").toString(),
                 QStringLiteral("alpha **beta** omega"));
        QCOMPARE(editor->property("wrappedSelectionStart").toInt(), 8);
        QCOMPARE(editor->property("wrappedSelectionEnd").toInt(), 12);
    }

    void savesAndOpensFromFooterButtons() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QVERIFY(window->findChild<QObject *>(QStringLiteral("sourceEditor")));
        QVERIFY(!window->findChild<QObject *>(QStringLiteral("renderedPreview")));
        QVERIFY(!window->findChild<QObject *>(QStringLiteral("modeToggle")));

        QObject *saveButton = window->findChild<QObject *>(QStringLiteral("saveButton"));
        QObject *openButton = window->findChild<QObject *>(QStringLiteral("openButton"));
        QVERIFY(saveButton);
        QVERIFY(openButton);

        QSignalSpy saveDialogSpy(&backend, &Backend::saveDialogRequested);
        QVERIFY(QMetaObject::invokeMethod(saveButton, "clicked"));
        QCOMPARE(saveDialogSpy.count(), 1);

        QSignalSpy openDialogSpy(&backend, &Backend::openDialogRequested);
        QVERIFY(QMetaObject::invokeMethod(openButton, "clicked"));
        QCOMPARE(openDialogSpy.count(), 1);
    }

    void scalesTextWithDesktopTextSize() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QVERIFY(editor);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 20);

        // `omarchy display text size 16` sets the GNOME factor to 16/12.
        backend.setTextScale(16.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 27);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 27);

        backend.setTextScale(9.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 15);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 15);
    }

    void remembersLastSaveDirectory() {
        QTemporaryDir saveDirectory;
        QVERIFY(saveDirectory.isValid());

        const QString savedPath = saveDirectory.filePath(QStringLiteral("first.md"));
        Backend savedDocument;
        savedDocument.saveAs(QUrl::fromLocalFile(savedPath));

        Backend nextDocument;
        QSignalSpy saveDialogSpy(&nextDocument, &Backend::saveDialogRequested);
        nextDocument.saveAsDialog();
        QCOMPARE(saveDialogSpy.count(), 1);

        const QUrl suggestedUrl = saveDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).absolutePath(),
                 saveDirectory.path());
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).fileName(),
                 QStringLiteral("Untitled.md"));

        QSettings().setValue(QStringLiteral("file/lastSaveDirectory"),
                             saveDirectory.filePath(QStringLiteral("missing")));
        Backend fallbackDocument;
        QSignalSpy fallbackDialogSpy(&fallbackDocument, &Backend::saveDialogRequested);
        fallbackDocument.saveAsDialog();
        const QUrl fallbackUrl = fallbackDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(fallbackUrl.toLocalFile()).absolutePath(), QDir::homePath());
    }

    void opensLongDocumentAtFirstLine() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("long.md")),
                                       longDocument(QStringLiteral("Opened")));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);

        backend.open(QUrl::fromLocalFile(path));

        assertAtFirstLine(window.data());
    }

    void opensIntoScrolledWindowAtFirstLine() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString first = writeFile(directory.filePath(QStringLiteral("first.md")),
                                        longDocument(QStringLiteral("First")));
        const QString second = writeFile(directory.filePath(QStringLiteral("second.md")),
                                         longDocument(QStringLiteral("Second")));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(first));
        scrollToEnd(window.data());

        backend.open(QUrl::fromLocalFile(second));

        assertAtFirstLine(window.data());
    }

    void reloadsChangedDocumentAtFirstLine() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("changing.md")),
                                       longDocument(QStringLiteral("Before")));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        scrollToEnd(window.data());
        writeFile(path, longDocument(QStringLiteral("After")));

        backend.reloadFromDisk();

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QVERIFY(editor->property("text").toString().startsWith(QStringLiteral("After")));
        assertAtFirstLine(window.data());
    }

    void recoversLongDocumentAtFirstLine() {
        const QDir stateDirectory(
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        QVERIFY(QDir().mkpath(stateDirectory.path()));
        for (const QString &stale : stateDirectory.entryList({QStringLiteral("recovery-*")}))
            QVERIFY(QFile::remove(stateDirectory.filePath(stale)));
        const QString text = longDocument(QStringLiteral("Recovered"));
        const QJsonObject snapshot{{QStringLiteral("fileUrl"), QString()},
                                   {QStringLiteral("text"), text}};
        writeFile(stateDirectory.filePath(QStringLiteral("recovery-0.json")),
                  QString::fromUtf8(QJsonDocument(snapshot).toJson()));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QCOMPARE(editor->property("text").toString(), text);
        assertAtFirstLine(window.data());
        backend.discardRecovery();
    }

private:
    static QStringList headingTexts(const StructureScan::Structure &structure) {
        QStringList texts;
        for (const StructureScan::Heading &heading : structure.headings)
            texts.append(heading.text);
        return texts;
    }

    static bool drawnAsHeading(const QTextDocument &document, int line) {
        const QTextBlock block = document.findBlockByNumber(line);
        for (const QTextLayout::FormatRange &range : block.layout()->formats()) {
            if (range.format.fontWeight() == QFont::Bold)
                return true;
        }
        return false;
    }

    static QString statsDocument() {
        return QStringLiteral(
            "---\ntitle: Plan\n---\n# One\n```\n# not\n```\n## Two\n");
    }

    static QString cardText(QObject *window, const char *name) {
        QObject *value = window->findChild<QObject *>(QLatin1String(name) + QStringLiteral("Value"));
        return value ? value->property("text").toString() : QString();
    }

    static QString longDocument(const QString &title) {
        QString text = title + QStringLiteral("\n\n");
        for (int line = 1; line <= 400; ++line)
            text += QStringLiteral("Line %1 of a long document.\n").arg(line);
        return text;
    }

    static QString writeFile(const QString &path, const QString &text) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            file.write(text.toUtf8());
        return path;
    }

    static QObject *createWindow(Backend &backend, QQmlEngine &engine) {
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        return component.create();
    }

    // Leave the view and the cursor at the end of the document, as a reader who
    // had scrolled to the bottom would.
    static void scrollToEnd(QObject *window) {
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        editor->setProperty("cursorPosition", editor->property("length"));
        QTRY_VERIFY(flick->property("contentY").toReal() > 0);
    }

    static void assertAtFirstLine(QObject *window) {
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        QVERIFY(editor);
        QVERIFY(flick);
        // Let layout and any scroll that follows the cursor settle first, so a
        // late jump to the bottom is caught rather than raced.
        QTRY_VERIFY(flick->property("contentHeight").toReal()
                    > 2 * flick->property("height").toReal());
        QTest::qWait(100);
        QCOMPARE(editor->property("cursorPosition").toInt(), 0);
        QCOMPARE(flick->property("contentY").toReal(), 0.0);
    }

    QTemporaryDir m_settingsDirectory;
    QTemporaryDir m_dataDirectory;
};

QTEST_MAIN(LeafTest)
#include "tst_leaf.moc"
