#include <QtTest>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickTextDocument>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextList>
#include <QTextTable>

#include "backend.h"
#include "markdownhighlighter.h"
#include "readingrenderer.h"
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

    void titlesOutlineEntriesAsTheyRead() {
        QCOMPARE(Backend::outlineTitle(QStringLiteral("Plan")), QStringLiteral("Plan"));
        QCOMPARE(Backend::outlineTitle(QStringLiteral("Closing ##")), QStringLiteral("Closing"));
        QCOMPARE(Backend::outlineTitle(QStringLiteral("C#")), QStringLiteral("C#"));
        QCOMPARE(Backend::outlineTitle(QStringLiteral("The **big** and *small* idea")),
                 QStringLiteral("The big and small idea"));
        QCOMPARE(Backend::outlineTitle(QStringLiteral("Use `code` and [a link](https://x.org)")),
                 QStringLiteral("Use code and a link"));
        QCOMPARE(Backend::outlineTitle(QStringLiteral("Keep `**stars**` in code")),
                 QStringLiteral("Keep **stars** in code"));
        QCOMPARE(Backend::outlineTitle(QStringLiteral("[docs](https://x.org/my_page_name) guide")),
                 QStringLiteral("docs guide"));
        QCOMPARE(Backend::outlineTitle(QString()), QString());
        QCOMPARE(Backend::outlineTitle(QStringLiteral("##")), QString());
    }

    void exposesTheOutlineOnLoadAndAfterAnEdit() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("stats.md")),
                                       statsDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));

        QCOMPARE(backend.outline().size(), 2);
        assertEntry(backend.outline().at(0), 1, QStringLiteral("One"), 20);
        assertEntry(backend.outline().at(1), 2, QStringLiteral("Two"), 40);

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QTextDocument *document =
            editor->property("textDocument").value<QQuickTextDocument *>()->textDocument();
        QTextCursor cursor(document);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(QStringLiteral("### The **third**\n"));

        QTRY_COMPARE(backend.outline().size(), 3);
        assertEntry(backend.outline().at(2), 3, QStringLiteral("The third"), 47);
    }

    void listsHeadingsInThePaneOrSaysThereAreNone() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString headed = writeFile(directory.filePath(QStringLiteral("headed.md")),
                                         statsDocument());
        const QString plain = writeFile(directory.filePath(QStringLiteral("plain.md")),
                                        QStringLiteral("Just prose.\n```\n# code\n```\n"));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));
        QObject *noHeadings = window->findChild<QObject *>(QStringLiteral("noHeadings"));
        QVERIFY(list);
        QVERIFY(noHeadings);

        backend.open(QUrl::fromLocalFile(headed));
        QCOMPARE(list->property("count").toInt(), 2);
        QVERIFY(!noHeadings->property("visible").toBool());

        backend.open(QUrl::fromLocalFile(plain));
        QCOMPARE(list->property("count").toInt(), 0);
        QVERIFY(noHeadings->property("visible").toBool());
    }

    void jumpsToAHeadingWithItsLineAtTheTop() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument();
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());

        const int position = text.indexOf(QStringLiteral("## Section 4"));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "jumpToHeading", Q_ARG(QVariant, 4)));
        QTest::qWait(100);

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        QCOMPARE(editor->property("cursorPosition").toInt(), position);
        QVERIFY(editor->property("activeFocus").toBool());
        // The view snaps to whole pixels, so the line lands within one of its top.
        QVERIFY(qAbs(flick->property("contentY").toReal() - lineTop(editor, position)) <= 1);
    }

    void jumpsToAHeadingClickedInTheOutline() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument();
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());

        // The entries in order: "Headed", then "Section 1" onwards.
        QQuickItem *entry = nullptr;
        QTRY_VERIFY((entry = outlineEntry(window.data(), QStringLiteral("Section 3"))));
        auto *quickWindow = qobject_cast<QQuickWindow *>(window.data());
        QVERIFY(quickWindow);
        const QPointF centre = entry->mapToScene(QPointF(entry->width() / 2, entry->height() / 2));
        QTest::mouseClick(quickWindow, Qt::LeftButton, {}, centre.toPoint());

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QTRY_COMPARE(editor->property("cursorPosition").toInt(),
                     text.indexOf(QStringLiteral("## Section 3")));
        QVERIFY(editor->property("activeFocus").toBool());
    }

    void jumpsToAHeadingNearTheEndAsFarAsTheTextScrolls() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument() + QStringLiteral("## Last\nThe end.\n");
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());

        const int position = text.indexOf(QStringLiteral("## Last"));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "jumpToHeading", Q_ARG(QVariant, 9)));
        QTest::qWait(100);

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        const qreal furthest = flick->property("contentHeight").toReal()
            - flick->property("height").toReal();
        QCOMPARE(editor->property("cursorPosition").toInt(), position);
        QVERIFY(qAbs(flick->property("contentY").toReal() - furthest) <= 1);
        QVERIFY(furthest < lineTop(editor, position));

        // The picked heading is marked though it could not reach the top, until the
        // view scrolls again.
        QCOMPARE(window->property("markedHeading").toInt(), 9);
        flick->setProperty("contentY", furthest - 10);
        QCOMPARE(window->property("markedHeading").toInt(), 8);
    }

    void marksTheHeadingBeingRead() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument();
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());
        QCOMPARE(window->property("markedHeading").toInt(), 0);

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        const qreal sectionThree = lineTop(editor, text.indexOf(QStringLiteral("## Section 3")));
        flick->setProperty("contentY", sectionThree);
        QCOMPARE(window->property("markedHeading").toInt(), 3);

        flick->setProperty("contentY", sectionThree - 10);
        QCOMPARE(window->property("markedHeading").toInt(), 2);
    }

    void remarksAfterTheTextRewraps() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QString text;
        for (int section = 1; section <= 6; ++section) {
            text += QStringLiteral("## Section %1\n").arg(section);
            for (int paragraph = 1; paragraph <= 8; ++paragraph)
                text += QStringLiteral("A paragraph long enough to wrap onto several lines "
                                       "whenever the text column is made narrower than it "
                                       "starts out, which moves every heading below it.\n\n");
        }
        const QString path = writeFile(directory.filePath(QStringLiteral("wrapped.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        flick->setProperty("contentY",
                           lineTop(editor, text.indexOf(QStringLiteral("## Section 3"))));
        QCOMPARE(window->property("markedHeading").toInt(), 2);

        window->setProperty("width", 700);

        QTRY_VERIFY(window->property("markedHeading").toInt() < 2);
        QVariant atTop;
        QVERIFY(QMetaObject::invokeMethod(window.data(), "headingAtTop",
                                          Q_RETURN_ARG(QVariant, atTop)));
        QCOMPARE(window->property("markedHeading").toInt(), atTop.toInt());
    }

    void holdsAJumpsMarkThroughAnEdit() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument() + QStringLiteral("## Last\nThe end.\n");
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "jumpToHeading", Q_ARG(QVariant, 9)));
        QCOMPARE(window->property("markedHeading").toInt(), 9);

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QTextDocument *document =
            editor->property("textDocument").value<QQuickTextDocument *>()->textDocument();
        QTextCursor cursor(document->findBlock(text.indexOf(QStringLiteral("## Last"))));
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.insertText(QStringLiteral(" words"));

        QTRY_COMPARE(backend.outline().at(9).toMap().value(QStringLiteral("title")).toString(),
                     QStringLiteral("Last words"));
        QCOMPARE(window->property("markedHeading").toInt(), 9);
    }

    void entersTheOutlineAtTheMarkWithCtrlJ() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument();
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));
        flick->setProperty("contentY",
                           lineTop(editor, text.indexOf(QStringLiteral("## Section 3"))));
        QCOMPARE(window->property("markedHeading").toInt(), 3);

        press(window.data(), Qt::Key_J, Qt::ControlModifier);

        QVERIFY(list->property("activeFocus").toBool());
        QCOMPARE(list->property("currentIndex").toInt(), 3);
    }

    void movesTheSelectionWithoutMovingTheText() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument();
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));

        press(window.data(), Qt::Key_J, Qt::ControlModifier);
        press(window.data(), Qt::Key_Down);
        press(window.data(), Qt::Key_Down);
        press(window.data(), Qt::Key_Down);
        press(window.data(), Qt::Key_Up);

        QCOMPARE(list->property("currentIndex").toInt(), 2);
        QCOMPARE(flick->property("contentY").toReal(), 0.0);
        QCOMPARE(window->property("markedHeading").toInt(), 0);

        press(window.data(), Qt::Key_Return);

        // Enter jumps and stays in the outline, so the owner can keep stepping.
        const int position = text.indexOf(QStringLiteral("## Section 2"));
        QVERIFY(list->property("activeFocus").toBool());
        QCOMPARE(editor->property("cursorPosition").toInt(), position);
        QCOMPARE(window->property("markedHeading").toInt(), 2);
        QVERIFY(qAbs(flick->property("contentY").toReal() - lineTop(editor, position)) <= 1);

        press(window.data(), Qt::Key_Down);
        press(window.data(), Qt::Key_Return);
        const int next = text.indexOf(QStringLiteral("## Section 3"));
        QCOMPARE(window->property("markedHeading").toInt(), 3);
        QVERIFY(qAbs(flick->property("contentY").toReal() - lineTop(editor, next)) <= 1);

        // Ctrl+J returns to the text at the last heading jumped to.
        press(window.data(), Qt::Key_J, Qt::ControlModifier);
        QVERIFY(editor->property("activeFocus").toBool());
        QCOMPARE(editor->property("cursorPosition").toInt(), next);
    }

    void returnsToTheTextUnmovedWithEscapeOrCtrlJ() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString text = headedDocument();
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));
        editor->setProperty("cursorPosition", 5);

        for (const Qt::Key key : {Qt::Key_Escape, Qt::Key_J}) {
            press(window.data(), Qt::Key_J, Qt::ControlModifier);
            QVERIFY(list->property("activeFocus").toBool());
            press(window.data(), Qt::Key_Down);
            press(window.data(), key, key == Qt::Key_J ? Qt::ControlModifier : Qt::NoModifier);

            QVERIFY(editor->property("activeFocus").toBool());
            QCOMPARE(editor->property("cursorPosition").toInt(), 5);
            QCOMPARE(flick->property("contentY").toReal(), 0.0);
        }
    }

    void selectsTheFirstEntryWhenNothingIsMarked() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("stats.md")),
                                       statsDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        QCOMPARE(window->property("markedHeading").toInt(), -1);
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));

        press(window.data(), Qt::Key_J, Qt::ControlModifier);

        QVERIFY(list->property("activeFocus").toBool());
        QCOMPARE(list->property("currentIndex").toInt(), 0);
    }

    void entersTheOutlineFromTheFindBar() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("headed.md")),
                                       headedDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));

        press(window.data(), Qt::Key_F, Qt::ControlModifier);
        QVERIFY(window->property("searchOpen").toBool());
        press(window.data(), Qt::Key_J, Qt::ControlModifier);

        QVERIFY(list->property("activeFocus").toBool());
    }

    void ignoresCtrlJWithoutHeadingsOrWithADialogOpen() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString plain = writeFile(directory.filePath(QStringLiteral("plain.md")),
                                        QStringLiteral("Just prose.\n"));
        const QString headed = writeFile(directory.filePath(QStringLiteral("headed.md")),
                                         headedDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));

        backend.open(QUrl::fromLocalFile(plain));
        press(window.data(), Qt::Key_J, Qt::ControlModifier);
        QVERIFY(!list->property("activeFocus").toBool());
        QVERIFY(editor->property("activeFocus").toBool());

        backend.open(QUrl::fromLocalFile(headed));
        QObject *dialog = window->findChild<QObject *>(QStringLiteral("shortcutsDialog"));
        QVERIFY(dialog);
        QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        press(window.data(), Qt::Key_J, Qt::ControlModifier);
        QVERIFY(!list->property("activeFocus").toBool());

        // The same key works once the dialog is closed, so the checks above are real.
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        press(window.data(), Qt::Key_J, Qt::ControlModifier);
        QTRY_VERIFY(list->property("activeFocus").toBool());
    }

    void leavesAnOutlineThatEmptiesForTheText() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString headed = writeFile(directory.filePath(QStringLiteral("headed.md")),
                                         headedDocument());
        const QString plain = writeFile(directory.filePath(QStringLiteral("plain.md")),
                                        QStringLiteral("Just prose.\n"));

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createActiveWindow(backend, engine));
        QVERIFY(window);
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));
        backend.open(QUrl::fromLocalFile(headed));
        press(window.data(), Qt::Key_J, Qt::ControlModifier);
        QVERIFY(list->property("activeFocus").toBool());

        backend.open(QUrl::fromLocalFile(plain));

        QTRY_VERIFY(editor->property("activeFocus").toBool());
    }

    void marksNothingAboveTheFirstHeading() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = writeFile(directory.filePath(QStringLiteral("stats.md")),
                                       statsDocument());

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));

        QCOMPARE(window->property("markedHeading").toInt(), -1);
    }

    void keepsTheMarkInViewInALongOutline() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QString text;
        for (int section = 1; section <= 80; ++section)
            text += QStringLiteral("## Part %1\nA line.\nAnother line.\n\n").arg(section);
        const QString path = writeFile(directory.filePath(QStringLiteral("parts.md")), text);

        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(path));
        assertAtFirstLine(window.data());

        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));
        flick->setProperty("contentY", flick->property("contentHeight").toReal()
                                           - flick->property("height").toReal());
        const int marked = window->property("markedHeading").toInt();
        QVERIFY(marked > 70);

        QObject *list = window->findChild<QObject *>(QStringLiteral("outlineList"));
        QQuickItem *entry = nullptr;
        QTRY_VERIFY(QMetaObject::invokeMethod(list, "itemAtIndex", Q_RETURN_ARG(QQuickItem *, entry),
                                              Q_ARG(int, marked))
                    && entry);
        QVERIFY(entryInView(list, entry));

        // A changed outline resets the list to its top; the mark is shown again.
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QTextDocument *document =
            editor->property("textDocument").value<QQuickTextDocument *>()->textDocument();
        QTextCursor cursor(document->findBlock(text.indexOf(QStringLiteral("## Part 80"))));
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.insertText(QStringLiteral(" and last"));
        QTRY_COMPARE(backend.outline().at(79).toMap().value(QStringLiteral("title")).toString(),
                     QStringLiteral("Part 80 and last"));
        QTRY_VERIFY(QMetaObject::invokeMethod(list, "itemAtIndex", Q_RETURN_ARG(QQuickItem *, entry),
                                              Q_ARG(int, marked))
                    && entry && entryInView(list, entry));
    }

    void keepsTheTextColumnBesideTheOutlineInANarrowWindow() {
        Backend backend;
        QQmlEngine engine;
        QScopedPointer<QObject> window(createWindow(backend, engine));
        QVERIFY(window);
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *flick = window->findChild<QObject *>(QStringLiteral("editorFlick"));

        window->setProperty("width", 600);

        QTRY_COMPARE(flick->property("width").toInt(), 600 - 260 - 48);
        QVERIFY(editor->property("x").toReal() >= 0);
        QVERIFY(editor->property("width").toReal() <= flick->property("width").toReal());
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
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 17);

        // `omarchy display text size 16` sets the GNOME factor to 16/12.
        backend.setTextScale(16.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 23);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 23);

        backend.setTextScale(9.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 13);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 13);
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

    void displayCopyBlanksFrontMatterLineForLine() {
        QCOMPARE(ReadingRenderer::displayCopy(QStringLiteral("---\ntype: x\n---\n# Title\n")),
                 QStringLiteral("\n\n\n# Title\n"));
    }

    void displayCopyKeepsAFirstRuleThatNeverCloses() {
        const QString text = QStringLiteral("---\nnot front matter\n");
        QCOMPARE(ReadingRenderer::displayCopy(text), text);
    }

    void displayCopyBlanksTagLinesOutsideCodeOnly() {
        const QString text = QStringLiteral(
            "<task>\nDo it.\n</output_format>\n  <example type=\"bad\">  \n"
            "```\n<task>\n```\nSee <b>this</b>\n<b>bold</b> text\n");
        QCOMPARE(ReadingRenderer::displayCopy(text),
                 QStringLiteral("\nDo it.\n\n\n```\n<task>\n```\nSee <b>this</b>\n<b>bold</b> text\n"));
    }

    void displayCopyClosesVoidTagsOutsideCode() {
        const QString text = QStringLiteral(
            "| a<br>b | c<BR>d |\nrule <hr> and <img src=\"x.png\" alt=\"x\"> and <br/>\n"
            "keep `<br>` here and <bra>\n```\n<br>\n```\n## Title<br>\n");
        QCOMPARE(ReadingRenderer::displayCopy(text), QStringLiteral(
            "| a<br/>b | c<BR/>d |\nrule <hr/> and <img src=\"x.png\" alt=\"x\"/> and <br/>\n"
            "keep `<br>` here and <bra>\n```\n<br>\n```\n## Title<br/>\n"));
    }

    void rendersEverythingAfterABreakInATableCell() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral(
            "| A | B |\n|---|---|\n| x<br>y | z |\n| two | w |\n\nAfter the table.\n"),
            readingStyle());
        QVERIFY(findRenderedBlock(document, QStringLiteral("two")).isValid());
        QVERIFY(findRenderedBlock(document, QStringLiteral("After the table.")).isValid());
    }

    void rendersAListAndATableAfterATagLineWhole() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral(
            "---\ntype: workflow\n---\n\n# Brainstorm\n\n<context>\n\n"
            "- first item\n- second item\n\n"
            "| Kind | Feeds |\n|---|---|\n| cycle | a spec |\n\n</context>\n"), readingStyle());

        QCOMPARE(findRenderedBlock(document, QStringLiteral("first item")).textList()->count(), 2);
        QTextTable *table = QTextCursor(findRenderedBlock(document, QStringLiteral("a spec")))
                                .currentTable();
        QVERIFY(table);
        QCOMPARE(table->rows(), 2);
        QCOMPARE(table->columns(), 2);
        QVERIFY(!document.toPlainText().contains(QStringLiteral("workflow")));
        QVERIFY(!document.toPlainText().contains(QStringLiteral("context")));
    }

    void rendersATagLineBetweenParagraphsAsTwoParagraphs() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral("One\n<note>\nTwo\n"), readingStyle());
        QVERIFY(findRenderedBlock(document, QStringLiteral("One")).isValid());
        QVERIFY(findRenderedBlock(document, QStringLiteral("Two")).isValid());
    }

    void rendersHeadingsSteppingDownInSizeInTheTextColour() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral(
            "# One\n\n## Two **bold**\n\n### Three\n\nBody\n"), readingStyle());

        QCOMPARE(pixelSizesIn(findRenderedBlock(document, QStringLiteral("One"))), QList<int>{34});
        // The bold word takes the heading's size, so the whole line is one run.
        QCOMPARE(pixelSizesIn(findRenderedBlock(document, QStringLiteral("Two bold"))),
                 QList<int>{26});
        QCOMPARE(pixelSizesIn(findRenderedBlock(document, QStringLiteral("Three"))), QList<int>{22});
        const QTextBlock body = findRenderedBlock(document, QStringLiteral("Body"));
        QCOMPARE(pixelSizesIn(body), QList<int>{17});
        QCOMPARE(body.begin().fragment().charFormat().foreground().color(), readingStyle().text);
    }

    void rendersLinksInTheAccentAndInlineCodeInTheCodeFont() {
        QTextDocument document;
        const ReadingRenderer::Style style = readingStyle();
        ReadingRenderer::render(&document, QStringLiteral(
            "See [site](https://example.com) and `code`.\n"), style);

        const QTextBlock block = document.begin();
        QTextCharFormat link;
        QTextCharFormat code;
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            if (it.fragment().text() == QStringLiteral("site"))
                link = it.fragment().charFormat();
            if (it.fragment().text() == QStringLiteral("code"))
                code = it.fragment().charFormat();
        }
        QCOMPARE(link.foreground().color(), style.accent);
        QVERIFY(link.fontUnderline());
        QCOMPARE(code.fontFamilies().toStringList(), QStringList{style.codeFamily});
        QCOMPARE(code.background().color(), style.shade);
    }

    void rendersAWebImageAsItsAlternativeText() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral(
            "![a chart](https://example.com/chart.png)\n"), readingStyle());
        QCOMPARE(document.toPlainText(), QStringLiteral("a chart"));
        QCOMPARE(imageNames(document), QStringList());
    }

    void rendersALocalImageFromTheFilesFolder() {
        QTemporaryDir directory;
        QImage(2, 2, QImage::Format_RGB32).save(directory.filePath(QStringLiteral("pic.png")));
        ReadingRenderer::Style style = readingStyle();
        style.fileUrl = QUrl::fromLocalFile(directory.filePath(QStringLiteral("notes.md")));

        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral("![pic](pic.png)\n"), style);
        QCOMPARE(imageNames(document),
                 QStringList{QUrl::fromLocalFile(directory.filePath(QStringLiteral("pic.png"))).toString()});
    }

    void clearsATaskMarkerQtLeavesOnBlocksAfterTheList() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral("- [ ] task\n\nAfter\n"), readingStyle());
        QCOMPARE(findRenderedBlock(document, QStringLiteral("task")).blockFormat().marker(),
                 QTextBlockFormat::MarkerType::Unchecked);
        QCOMPARE(findRenderedBlock(document, QStringLiteral("After")).blockFormat().marker(),
                 QTextBlockFormat::MarkerType::NoMarker);
    }

    void listsRenderedHeadingsInOrderAsShown() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral(
            "# One\n\n## Use **bold**  and `code`\n\nUnder\n===\n"), readingStyle());

        const QList<ReadingRenderer::RenderedHeading> headings = ReadingRenderer::headings(&document);
        QCOMPARE(headings.size(), 3);
        QCOMPARE(headings.at(0).level, 1);
        QCOMPARE(headings.at(0).text, QStringLiteral("One"));
        QCOMPARE(headings.at(1).level, 2);
        QCOMPARE(headings.at(1).text, QStringLiteral("Use bold and code"));
        QCOMPARE(headings.at(2).level, 1);
        QCOMPARE(headings.at(2).text, QStringLiteral("Under"));
        QCOMPARE(document.findBlockByNumber(headings.at(1).block).text().simplified(),
                 QStringLiteral("Use bold and code"));
    }

    void spacesACodeBlockFromTheProseButNotBetweenItsLines() {
        QTextDocument document;
        ReadingRenderer::render(&document, QStringLiteral(
            "Before\n\n```\nfirst\nmiddle\nlast\n```\n\nAfter\n"), readingStyle());
        const QTextBlockFormat first = findRenderedBlock(document, QStringLiteral("first")).blockFormat();
        const QTextBlockFormat middle = findRenderedBlock(document, QStringLiteral("middle")).blockFormat();
        const QTextBlockFormat last = findRenderedBlock(document, QStringLiteral("last")).blockFormat();
        QVERIFY(first.topMargin() > 0);
        QCOMPARE(first.bottomMargin(), 0.0);
        QCOMPARE(middle.topMargin(), 0.0);
        QCOMPARE(middle.bottomMargin(), 0.0);
        QCOMPARE(last.topMargin(), 0.0);
        QVERIFY(last.bottomMargin() > 0);
    }

    void rendersAFiveThousandLineFileQuickly() {
        QString text;
        for (int section = 0; section < 270; ++section) {
            text += QStringLiteral("## Section %1\n\n").arg(section);
            text += QStringLiteral("Some **prose** with `code` and a [link](https://x.org),\n"
                                   "wrapped by hand across two lines.\n\n");
            text += QStringLiteral("- one\n- two\n  - nested\n\n");
            text += QStringLiteral("| A | B |\n|---|---|\n| 1 | 2 |\n| 3 | 4 |\n\n");
            text += QStringLiteral("```cpp\nint x = 1;\nint y = 2;\n```\n\n");
        }
        QVERIFY(text.count(QLatin1Char('\n')) >= 5000);

        QTextDocument document;
        QElapsedTimer timer;
        timer.start();
        ReadingRenderer::render(&document, text, readingStyle());
        const qint64 elapsed = timer.elapsed();
        qInfo("Rendered %lld lines in %lld ms", qint64(text.count(QLatin1Char('\n'))), elapsed);
        QVERIFY2(elapsed < 250, qPrintable(QStringLiteral("took %1 ms").arg(elapsed)));
    }

private:
    static ReadingRenderer::Style readingStyle() {
        ReadingRenderer::Style style;
        style.text = QColor(QStringLiteral("#222222"));
        style.accent = QColor(QStringLiteral("#1144aa"));
        style.shade = QColor(QStringLiteral("#eeeeee"));
        style.bodyPixelSize = 17;
        style.proseFamily = QStringLiteral("iA Writer Duo S");
        style.codeFamily = QStringLiteral("iA Writer Mono S");
        return style;
    }

    static QTextBlock findRenderedBlock(const QTextDocument &document, const QString &text) {
        for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
            if (block.text().simplified() == text)
                return block;
        }
        return {};
    }

    static QList<int> pixelSizesIn(const QTextBlock &block) {
        QList<int> sizes;
        for (auto it = block.begin(); !it.atEnd(); ++it)
            sizes.append(it.fragment().charFormat().intProperty(QTextFormat::FontPixelSize));
        return sizes;
    }

    static QStringList imageNames(const QTextDocument &document) {
        QStringList names;
        for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                if (it.fragment().charFormat().isImageFormat())
                    names.append(it.fragment().charFormat().toImageFormat().name());
            }
        }
        return names;
    }

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

    static void assertEntry(const QVariant &entry, int level, const QString &title,
                            int position) {
        const QVariantMap map = entry.toMap();
        QCOMPARE(map.value(QStringLiteral("level")).toInt(), level);
        QCOMPARE(map.value(QStringLiteral("title")).toString(), title);
        QCOMPARE(map.value(QStringLiteral("position")).toInt(), position);
    }

    // Eight sections of fifty lines, each opened by "## Section N".
    static QString headedDocument() {
        QString text = QStringLiteral("# Headed\n\n");
        for (int section = 1; section <= 8; ++section) {
            text += QStringLiteral("## Section %1\n").arg(section);
            for (int line = 1; line <= 50; ++line)
                text += QStringLiteral("Line %1 of section %2.\n").arg(line).arg(section);
        }
        return text;
    }

    // The outline pane's clickable entry showing a title, once it is laid out. List
    // delegates are not QObject children, so this walks the visual items.
    static QQuickItem *outlineEntry(QObject *window, const QString &title) {
        auto *quickWindow = qobject_cast<QQuickWindow *>(window);
        QList<QQuickItem *> items{quickWindow->contentItem()};
        while (!items.isEmpty()) {
            QQuickItem *item = items.takeFirst();
            if (item->objectName() == QStringLiteral("outlineEntry")) {
                const QVariantMap data = item->parentItem()->property("modelData").toMap();
                if (data.value(QStringLiteral("title")).toString() == title)
                    return item;
            }
            items.append(item->childItems());
        }
        return nullptr;
    }

    // A window that receives key presses: shortcuts need it active.
    static QObject *createActiveWindow(Backend &backend, QQmlEngine &engine) {
        QObject *window = createWindow(backend, engine);
        auto *quickWindow = qobject_cast<QQuickWindow *>(window);
        if (!quickWindow)
            return window;
        quickWindow->requestActivate();
        if (!QTest::qWaitForWindowActive(quickWindow)) {
            delete window;
            return nullptr;
        }
        return window;
    }

    static void press(QObject *window, Qt::Key key,
                      Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QTest::keyClick(qobject_cast<QQuickWindow *>(window), key, modifiers);
    }

    static bool entryInView(QObject *list, QQuickItem *entry) {
        const qreal listTop = list->property("contentY").toReal();
        const qreal listBottom = listTop + list->property("height").toReal();
        return entry->y() >= listTop && entry->y() + entry->height() <= listBottom;
    }

    // Where a position's line starts, in the scrolled view's coordinates.
    static qreal lineTop(QObject *editor, int position) {
        QRectF line;
        QMetaObject::invokeMethod(editor, "positionToRectangle", Q_RETURN_ARG(QRectF, line),
                                  Q_ARG(int, position));
        return editor->property("y").toReal() + line.y();
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
