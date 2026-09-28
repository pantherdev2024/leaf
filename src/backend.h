#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QByteArray>
#include <QFileSystemWatcher>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <memory>

#include "readingrenderer.h"

class MarkdownHighlighter;
class QPrinter;
class QTextDocument;
class QWindow;
class QLockFile;

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileUrlChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY statsChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY statsChanged)
    Q_PROPERTY(int tokenEstimate READ tokenEstimate NOTIFY statsChanged)
    Q_PROPERTY(int sectionCount READ sectionCount NOTIFY statsChanged)
    // Each entry: level, title (as the outline shows it) and position (of the
    // heading line's first character).
    Q_PROPERTY(QVariantList outline READ outline NOTIFY outlineChanged)
    // For each outline entry, where its heading starts in the reading view's
    // document, or -1 when the page shows no heading to match it.
    Q_PROPERTY(QVariantList readingHeadingPositions READ readingHeadingPositions
                   NOTIFY readingHeadingPositionsChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)
    Q_PROPERTY(QString themeBackground READ themeBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeForeground READ themeForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccent READ themeAccent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeSelection READ themeSelection NOTIFY themeColorsChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    void setParentWindow(QWindow *window);

    QUrl fileUrl() const { return m_fileUrl; }
    QString fileName() const;

    bool modified() const { return m_modified; }
    QString status() const { return m_status; }
    int wordCount() const { return m_wordCount; }
    int lineCount() const { return m_lineCount; }
    int tokenEstimate() const { return m_tokenEstimate; }
    int sectionCount() const { return m_sectionCount; }
    QVariantList outline() const { return m_outline; }
    QVariantList readingHeadingPositions() const { return m_readingHeadingPositions; }
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);
    QString themeBackground() const { return m_themeBackground; }
    QString themeForeground() const { return m_themeForeground; }
    QString themeAccent() const { return m_themeAccent; }
    QString themeSelection() const { return m_themeSelection; }
    static int countWords(const QString &text);
    static int countLines(const QString &text);
    static int estimateTokens(const QString &text);
    static QString outlineTitle(const QString &headingText);
    static QString normalizedLinkUrl(const QString &clipboardText);
    static QString suggestedFileName(const QString &text);

    Q_INVOKABLE void attachDocument(QObject *textDocument);
    // The reading view's own document, which renderReading fills from the text.
    Q_INVOKABLE void attachReadingDocument(QObject *textDocument);
    void renderReading();
    // Renders at the reader's column width, which code blocks are fitted to.
    Q_INVOKABLE void renderReadingAtWidth(qreal columnWidth);
    // Where the reading view's table header rows sit, each as x, y, width and
    // height, and the shade they are drawn in: the text view draws no cell
    // backgrounds, so the window draws them behind it.
    Q_INVOKABLE QVariantList readingHeaderRows() const;
    Q_INVOKABLE QColor readingShade() const;
    // Where `query` is found in the reading view's text as shown, ignoring case:
    // each match's start and end in its document, in order.
    Q_INVOKABLE QVariantList findInReading(const QString &query) const;
    Q_INVOKABLE void openDialog();
    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveForClose();
    Q_INVOKABLE void saveAsDialog();
    Q_INVOKABLE void saveAs(const QUrl &url);
    Q_INVOKABLE void fileDialogCanceled();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE void reloadFromDisk();
    Q_INVOKABLE void keepExternalVersion();
    Q_INVOKABLE void printDocument();
    // Fills `document` with the page as printed: the current text through the
    // reading renderer, in light colours whatever the theme, laid out on pages of
    // `pageSize` (in the reading view's pixels).
    void buildPrintDocument(QTextDocument *document, const QSizeF &pageSize) const;
    // Prints the current text as buildPrintDocument lays it out.
    void printTo(QPrinter *printer) const;
    Q_INVOKABLE void newWindow();
    Q_INVOKABLE QString clipboardUrl() const;
    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE bool editorTextChanged();
    Q_INVOKABLE QVariantList hiddenRangesAt(int position) const;
    Q_INVOKABLE void setSearchHighlight(const QString &query, int currentMatchStart);
    Q_INVOKABLE void openExternalUrl(const QUrl &url);
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void fileUrlChanged();
    void modifiedChanged();
    void statusChanged();
    void statsChanged();
    void outlineChanged();
    void readingHeadingPositionsChanged();
    void darkModeChanged();
    void textScaleChanged();
    void themeColorsChanged();
    void closeAfterSave();
    void openDialogRequested();
    void saveDialogRequested(const QUrl &suggestedUrl);
    void saveSucceeded();
    void documentLoaded();
    void externalChangeDetected(bool deleted, bool locallyModified);

private:
    ReadingRenderer::Style readingStyle() const;
    ReadingRenderer::Style readingStyleIn(const QColor &background, const QColor &foreground,
                                          const QColor &accent, bool dark) const;
    void matchReadingHeadings();
    void loadDocumentText(const QString &text);
    void setFileUrl(const QUrl &url);
    void setModified(bool modified);
    void setStatus(const QString &status);
    void saveTo(const QUrl &url);
    QUrl suggestedSaveUrl() const;
    QString currentDocumentText() const;
    void recount(const QString &text);
    void scheduleRecount();
    void applyDocumentTypography();
    void reapplyTypographyToChange();
    void scheduleRecovery();
    void writeRecovery();
    void restoreRecovery();
    void clearRecovery();
    QString recoveryPath() const;
    void watchCurrentFile();
    void loadOmarchyTheme();
    void watchOmarchyTheme();

    QUrl m_fileUrl;
    bool m_modified = false;
    QString m_status;
    int m_wordCount = 0;
    int m_lineCount = 0;
    int m_tokenEstimate = 0;
    int m_sectionCount = 0;
    QVariantList m_outline;
    QVariantList m_readingHeadingPositions;
    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    bool m_loading = false;
    bool m_closeAfterSave = false;
    bool m_formattingTypography = false;
    int m_formattedBlockCount = 0;
    int m_lastChangePos = 0;
    int m_lastChangeAdded = 0;
    QTimer m_recountTimer;
    QTimer m_recoveryTimer;
    QFileSystemWatcher m_fileWatcher;
    QPointer<QTextDocument> m_document;
    QPointer<QTextDocument> m_readingDocument;
    qreal m_readingColumnWidth = 0;
    QPointer<QWindow> m_parentWindow;
    QPointer<MarkdownHighlighter> m_highlighter;
    QString m_lastDocumentText;
    QByteArray m_lastKnownFileContents;
    bool m_hasKnownFileContents = false;
    QString m_recoveryPath;
    std::unique_ptr<QLockFile> m_recoveryLock;

    QString m_themeBackground;
    QString m_themeForeground;
    QString m_themeAccent;
    QString m_themeSelection;
    QFileSystemWatcher m_themeWatcher;
};
