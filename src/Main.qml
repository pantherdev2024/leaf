import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import QtQuick.Window
import "EditorMutations.js" as EditorMutations

ApplicationWindow {
    id: win
    width: 1280
    height: 820
    minimumWidth: 720
    minimumHeight: 520
    visible: true
    title: (backend.modified ? "* " : "") + backend.fileName + " - Leaf"

    readonly property bool darkMode: backend.darkMode
    readonly property color pageColor: backend.themeBackground
    readonly property color textColor: backend.themeForeground
    readonly property color strongTextColor: backend.themeForeground
    readonly property color mutedColor: darkMode ? "#909191" : "#aeb1b5"
    readonly property color selectionFill: backend.themeSelection
    // The desktop's text size knob (GNOME's text-scaling-factor, which
    // `omarchy display text size` drives) anchored so its 12px default leaves
    // the app at the sizes it was designed around.
    readonly property real textScale: backend.textScale
    readonly property int editorFontPixelSize: scaledSize(17)
    readonly property int outlineWidth: scaledSize(260)
    // The reading width, fitted to the space beside the outline pane, and never
    // wider than that space in a narrow window.
    readonly property int editorWidth: Math.min(
        Math.round(writerFontMetrics.averageCharacterWidth * 65),
        Math.max(360, width - outlineWidth - Math.round(writerFontMetrics.averageCharacterWidth * 20)),
        editorFlick.width)
    property bool closeConfirmed: false
    property var readingHeaderRows: []
    property color readingShade: "transparent"
    property bool searchOpen: false
    property bool searchUpdating: false
    // Each match's start and end, in the text of the view shown.
    property var searchMatches: []
    // The reading view's highlights, for the matches in and near the view only.
    property var readingMatchBoxes: []
    property int searchMatchIndex: -1
    property url pendingOpenUrl
    property string pendingAction: ""
    property bool replaceOpen: false
    property bool awaitingPendingSave: false
    // The outline entry being read, or -1 above the first heading. After a jump
    // it is held on the picked entry, which may not have reached the top, until
    // the view next moves; an edit alone does not move it.
    property int markedHeading: -1
    property bool markHeld: false
    // Which view is shown: the rendered page, or the editable text. Every loaded
    // document opens reading; a new window with nothing to read opens editing.
    property bool reading: false

    // The find bar stays open across a switch, and its matches are found again in
    // the view shown once it is in place (restorePlace). Replace works on the
    // editable text only, and the editor's highlights go while the page is shown.
    onReadingChanged: {
        if (!reading || !searchOpen)
            return;
        replaceOpen = false;
        searchUpdating = true;
        backend.setSearchHighlight("", -1);
        editor.deselect();
        searchUpdating = false;
    }

    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.accent: backend.themeAccent
    color: pageColor

    onClosing: function(close) {
        if (closeConfirmed || !backend.modified)
            return;

        close.accepted = false;
        pendingAction = "close";
        if (!unsavedChangesDialog.opened)
            unsavedChangesDialog.open();
    }

    function requestOpen(url) {
        if (!backend.modified) {
            backend.open(url);
            return;
        }
        pendingOpenUrl = url;
        pendingAction = "open";
        unsavedChangesDialog.open();
    }

    function completePendingAction() {
        var action = pendingAction;
        pendingAction = "";
        if (action === "close") {
            closeConfirmed = true;
            close();
        } else if (action === "open") {
            backend.open(pendingOpenUrl);
        }
    }

    FontMetrics {
        id: writerFontMetrics
        font.family: "iA Writer Mono S"
        font.pixelSize: win.editorFontPixelSize
    }

    // Whole numbers with commas between thousands, as the stat cards show them.
    function formatCount(count) {
        return Number(count).toLocaleString(Qt.locale("en_US"), 'f', 0);
    }

    // Every hardcoded size in the interface is expressed at text scale 1.
    function scaledSize(pixels) {
        return Math.max(1, Math.round(pixels * win.textScale));
    }

    // Where outline entry `index` starts in the view shown, in the scrolling
    // area's coordinates; undefined when the reading view shows no heading
    // matched to it.
    function headingY(index) {
        if (reading) {
            var position = backend.readingHeadingPositions[index];
            if (position === undefined || position < 0)
                return undefined;
            return reader.y + reader.positionToRectangle(position).y;
        }
        return editor.y + editor.positionToRectangle(
            Math.min(backend.outline[index].position, editor.length)).y;
    }

    // Bring a heading to the top of the view, or as near as the end of the page
    // allows, and mark it; an entry the reading view shows no heading for does
    // nothing. In the editing view the cursor moves to the heading first, so the
    // scroll that follows the cursor does not pull the view off it. The entry is
    // marked and held after the scroll, which would otherwise mark by the usual
    // rule. Focus stays where it is: a click moves it to the text, Enter in the
    // outline leaves it there.
    function jumpToHeading(index) {
        var y = headingY(index);
        if (y === undefined)
            return;
        if (!reading)
            editor.cursorPosition = Math.min(backend.outline[index].position, editor.length);
        editorFlick.scrollTo(editorFlick.clampContentY(y));
        markedHeading = index;
        markHeld = true;
    }

    // The last heading whose line starts at or above the top of the view. Scrolled
    // to the very top, the text starts below the view's edge, so the first line
    // counts as at the top then. A pixel of slack covers the view's snapping.
    // Headings only move down the page, so a binary search will do; in the
    // reading view it runs over the entries matched to a heading there.
    function headingAtTop() {
        var entries = [];
        for (var i = 0; i < backend.outline.length; ++i) {
            if (!reading || backend.readingHeadingPositions[i] >= 0)
                entries.push(i);
        }
        var readingLine = Math.max(editorFlick.contentY, editor.y) + 1;
        var found = -1;
        var low = 0;
        var high = entries.length - 1;
        while (low <= high) {
            var middle = Math.floor((low + high) / 2);
            if (headingY(entries[middle]) <= readingLine) {
                found = entries[middle];
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        }
        return found;
    }

    function updateMark() {
        if (!markHeld)
            markedHeading = headingAtTop();
    }

    // Where a section runs in the view shown: from its heading, or the top above
    // the first heading, to the next heading the view shows, or the page's end.
    function sectionBounds(section) {
        var view = reading ? reader : editor;
        var start = section < 0 ? 0 : headingY(section);
        var end = view.y + view.height;
        for (var next = section + 1; next < backend.outline.length; ++next) {
            var y = headingY(next);
            if (y !== undefined) {
                end = y;
                break;
            }
        }
        return {start: start, end: Math.max(end, start + 1)};
    }

    // The place being read: the section at the top of the view, as the outline
    // entry marking it (-1 above the first heading), and how far through it the
    // view is, as a share of its length. Two layouts of the same text differ in
    // height, so a share of the section finds the same place where a share of
    // the page would not.
    function currentPlace() {
        // At the very top, above the first line, the place is the top itself.
        if (editorFlick.contentY <= 0)
            return {section: -1, share: 0, atTop: true};
        var section = headingAtTop();
        var bounds = sectionBounds(section);
        var share = (editorFlick.contentY - bounds.start) / (bounds.end - bounds.start);
        return {section: section, share: Math.min(1, Math.max(0, share)), atTop: false};
    }

    // Once the view shown has its layout, bring it to the same place and mark the
    // section. A section the reading view has no heading for gives way to the
    // nearest one before it that it has. In the editor the cursor goes to the
    // start of the line at the top first, so its own scrolling does not pull the
    // view off the place.
    function restorePlace(place) {
        Qt.callLater(function() {
            if (place.atTop) {
                if (!reading)
                    editor.cursorPosition = 0;
                editorFlick.scrollTo(0);
                markHeld = false;
                updateMark();
                if (searchOpen)
                    refindFromView();
                return;
            }
            var section = place.section;
            while (section >= 0 && headingY(section) === undefined)
                --section;
            var bounds = sectionBounds(section);
            var y = editorFlick.clampContentY(bounds.start + place.share * (bounds.end - bounds.start));
            if (!reading)
                editor.cursorPosition = editor.positionAt(0, Math.max(0, y - editor.y) + 1);
            editorFlick.scrollTo(y);
            markedHeading = section;
            markHeld = true;
            if (searchOpen)
                refindFromView();
        });
    }

    // Focus goes back to whichever view is shown.
    function focusText() {
        (reading ? reader : editor).forceActiveFocus();
    }

    // Into the outline with the marked entry selected, or the first when none is.
    // With no headings there is nothing to select, and the text keeps the focus.
    function focusOutline() {
        if (backend.outline.length === 0) {
            focusText();
            return;
        }
        outlineList.currentIndex = Math.max(0, markedHeading);
        outlineList.positionViewAtIndex(outlineList.currentIndex, ListView.Contain);
        outlineList.forceActiveFocus();
    }

    function showReading() {
        var place = currentPlace();
        renderPage();
        reading = true;
        reader.forceActiveFocus();
        restorePlace(place);
    }

    function showEditing(keepPlace) {
        var place = currentPlace();
        reading = false;
        editor.forceActiveFocus();
        if (keepPlace)
            restorePlace(place);
        else
            updateMark();
    }

    // The page is laid out at the reader's width, and the header rows' shade is
    // drawn behind it once it is laid out.
    function renderPage() {
        backend.renderReadingAtWidth(reader.width);
        Qt.callLater(showHeaderShades);
    }

    function showHeaderShades() {
        readingShade = backend.readingShade();
        readingHeaderRows = backend.readingHeaderRows();
    }

    // The page is built for one theme, size and width, so a change rebuilds it.
    function rerenderReading() {
        if (!reading)
            return;
        var place = currentPlace();
        renderPage();
        restorePlace(place);
    }

    onMarkedHeadingChanged: showMarkInOutline()

    function showMarkInOutline() {
        if (markedHeading >= 0)
            outlineList.positionViewAtIndex(markedHeading, ListView.Contain);
    }

    function toggleFullScreen() {
        win.visibility = win.visibility === Window.FullScreen
            ? Window.Windowed
            : Window.FullScreen;
    }

    // Matches ignore case. In the editor they are in its text, marks and all; on
    // the page, in the text as it is shown.
    function findMatches() {
        var query = searchField.text;
        if (query.length === 0)
            return [];
        if (reading)
            return backend.findInReading(query);
        var matches = [];
        var haystack = editor.text.toLocaleLowerCase();
        var needle = query.toLocaleLowerCase();
        var position = 0;
        while ((position = haystack.indexOf(needle, position)) !== -1) {
            matches.push({start: position, end: position + needle.length});
            position += Math.max(1, needle.length);
        }
        return matches;
    }

    function updateSearch() {
        searchMatches = findMatches();
        searchMatchIndex = searchMatches.length > 0 ? 0 : -1;
        showSearchMatch();
    }

    // Found again after a switch or on reopening, from the first match at or
    // below the top of the view, so the view does not jump away from its place.
    function refindFromView() {
        searchMatches = findMatches();
        var low = 0;
        var high = searchMatches.length;
        while (low < high) {
            var middle = Math.floor((low + high) / 2);
            if (matchRect(middle).y < editorFlick.contentY)
                low = middle + 1;
            else
                high = middle;
        }
        searchMatchIndex = searchMatches.length === 0 ? -1 : low % searchMatches.length;
        showSearchMatch();
    }

    // Where a match's first character is, in the scrolling area's coordinates.
    function matchRect(index) {
        var view = reading ? reader : editor;
        var rect = view.positionToRectangle(searchMatches[index].start);
        return Qt.rect(view.x + rect.x, view.y + rect.y, rect.width, rect.height);
    }

    function showSearchMatch() {
        var match = searchMatchIndex >= 0 ? searchMatches[searchMatchIndex] : null;
        if (reading) {
            if (match) {
                var rect = matchRect(searchMatchIndex);
                editorFlick.ensureVisible(rect.y, rect.y + rect.height);
            }
            showReadingMatches();
            return;
        }
        searchUpdating = true;
        backend.setSearchHighlight(searchField.text, match ? match.start : -1);
        if (match) {
            editor.select(match.start, match.end);
            editorFlick.ensureCursorVisible();
        }
        searchUpdating = false;
    }

    // Boxes over the matches from a screen above the view to a screen below it,
    // found by a binary search, since a common letter can match thousands of times.
    // A match wrapped onto a second line gets a box on each.
    function showReadingMatches() {
        if (!reading || !searchOpen || searchMatches.length === 0) {
            readingMatchBoxes = [];
            return;
        }
        var top = editorFlick.contentY - editorFlick.height;
        var bottom = editorFlick.contentY + 2 * editorFlick.height;
        var low = 0;
        var high = searchMatches.length;
        while (low < high) {
            var middle = Math.floor((low + high) / 2);
            if (matchRect(middle).y < top)
                low = middle + 1;
            else
                high = middle;
        }
        var boxes = [];
        for (var i = low; i < searchMatches.length; ++i) {
            var first = reader.positionToRectangle(searchMatches[i].start);
            if (reader.y + first.y > bottom)
                break;
            var last = reader.positionToRectangle(searchMatches[i].end);
            var current = i === searchMatchIndex;
            if (Math.abs(first.y - last.y) < 1) {
                boxes.push({x: first.x, y: first.y, width: last.x - first.x,
                            height: first.height, current: current});
            } else {
                boxes.push({x: first.x, y: first.y, width: reader.width - first.x,
                            height: first.height, current: current});
                boxes.push({x: 0, y: last.y, width: last.x, height: last.height,
                            current: current});
            }
        }
        readingMatchBoxes = boxes;
    }

    function moveSearch(direction) {
        if (searchMatches.length === 0)
            return;
        searchMatchIndex = (searchMatchIndex + direction + searchMatches.length)
                           % searchMatches.length;
        showSearchMatch();
    }

    function closeSearch() {
        searchOpen = false;
        readingMatchBoxes = [];
        searchUpdating = true;
        backend.setSearchHighlight("", -1);
        editor.deselect();
        searchUpdating = false;
        replaceOpen = false;
        win.focusText();
    }

    Shortcut {
        sequence: "Ctrl+S"
        context: Qt.ApplicationShortcut
        onActivated: backend.save()
    }

    Shortcut {
        sequence: "Ctrl+E"
        context: Qt.WindowShortcut
        enabled: win.reading || editor.length > 0
        onActivated: win.reading ? win.showEditing(true) : win.showReading()
    }

    Shortcut {
        sequence: "Ctrl+H"
        context: Qt.ApplicationShortcut
        // Replace works on the editable text, so it opens there, in place; the
        // matches are found once the editor is in place.
        onActivated: {
            var switching = win.reading;
            searchOpen = true;
            if (switching)
                win.showEditing(true);
            else
                win.refindFromView();
            replaceOpen = true;
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }

    Shortcut {
        sequence: "Ctrl+B"
        context: Qt.WindowShortcut
        enabled: !win.reading
        onActivated: editor.wrapSelection("**", "**")
    }

    Shortcut {
        sequence: "Ctrl+I"
        context: Qt.WindowShortcut
        enabled: !win.reading
        onActivated: editor.wrapSelection("*", "*")
    }

    Shortcut {
        sequence: "Ctrl+K"
        context: Qt.WindowShortcut
        enabled: !win.reading
        onActivated: editor.insertLink()
    }

    Shortcut {
        sequence: "Ctrl+?"
        context: Qt.ApplicationShortcut
        onActivated: shortcutsDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+O"
        context: Qt.ApplicationShortcut
        onActivated: backend.openDialog()
    }

    Shortcut {
        sequence: "Ctrl+N"
        context: Qt.ApplicationShortcut
        onActivated: backend.newWindow()
    }

    Shortcut {
        sequence: "Ctrl+Shift+S"
        context: Qt.ApplicationShortcut
        onActivated: backend.saveAsDialog()
    }

    Shortcut {
        sequence: "Ctrl+P"
        context: Qt.ApplicationShortcut
        onActivated: backend.printDocument()
    }

    Shortcut {
        sequences: ["Meta+F", "F11"]
        context: Qt.ApplicationShortcut
        onActivated: toggleFullScreen()
    }

    Shortcut {
        sequence: "Ctrl+Z"
        context: Qt.WindowShortcut
        enabled: !win.reading
        onActivated: editor.undo()
    }

    Shortcut {
        sequences: ["Ctrl+Shift+Z", "Ctrl+Y"]
        context: Qt.WindowShortcut
        enabled: !win.reading
        onActivated: editor.redo()
    }

    // Into the outline with the marked entry selected, and back to the text. An
    // open dialog already keeps the key from reaching the outline.
    Shortcut {
        sequence: "Ctrl+J"
        context: Qt.WindowShortcut
        enabled: backend.outline.length > 0
        onActivated: {
            if (outlineList.activeFocus) {
                win.focusText();
                return;
            }
            win.focusOutline();
        }
    }

    Shortcut {
        sequence: "Ctrl+F"
        context: Qt.ApplicationShortcut
        onActivated: {
            searchOpen = true;
            win.refindFromView();
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }

    Shortcut {
        sequence: "Ctrl+G"
        context: Qt.ApplicationShortcut
        enabled: win.searchOpen
        onActivated: win.moveSearch(1)
    }

    Connections {
        target: backend

        function onOpenDialogRequested() {
            openFileDialog.open();
        }

        function onSaveDialogRequested(suggestedUrl) {
            saveFileDialog.selectedFile = suggestedUrl;
            saveFileDialog.open();
        }

        function onCloseAfterSave() {
            win.closeConfirmed = true;
            win.close();
        }

        // A freshly loaded document is read from its first line. Without this the
        // caret is left at the end of the new text and the view follows it there.
        // It opens in the reading view, and the focus the text or the outline had
        // goes to the outline, so the arrows and Enter move through the page at
        // once; the file's outline is already listed by now.
        function onDocumentLoaded() {
            editor.cursorPosition = 0;
            win.renderPage();
            win.reading = true;
            win.updateMark();
            if (editor.activeFocus || reader.activeFocus || outlineList.activeFocus)
                win.focusOutline();
            editorFlick.scrollTo(0);
        }

        function onThemeColorsChanged() {
            win.rerenderReading();
        }

        function onTextScaleChanged() {
            win.rerenderReading();
        }

        // A new outline resets the pane's list to its top, so show the mark again.
        function onOutlineChanged() {
            win.updateMark();
            Qt.callLater(win.showMarkInOutline);
        }

        function onSaveSucceeded() {
            win.awaitingPendingSave = false;
            if (win.pendingAction !== "")
                win.completePendingAction();
        }

        function onExternalChangeDetected(deleted, locallyModified) {
            externalChangeDialog.deleted = deleted;
            externalChangeDialog.locallyModified = locallyModified;
            externalChangeDialog.open();
        }
    }

    Dialogs.FileDialog {
        id: openFileDialog
        title: "Open File"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["Markdown files (*.md *.markdown)", "All files (*)"]
        onAccepted: win.requestOpen(selectedFile)
    }

    Dialogs.FileDialog {
        id: saveFileDialog
        title: "Save File"
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: ["Markdown files (*.md *.markdown)", "All files (*)"]
        onAccepted: backend.saveAs(selectedFile)
        onRejected: {
            backend.fileDialogCanceled();
            win.awaitingPendingSave = false;
            win.pendingAction = "";
        }
    }

    UnsavedChangesDialog {
        id: unsavedChangesDialog
        fileName: backend.fileName
        darkMode: win.darkMode
        textScale: win.textScale
        textColor: win.textColor
        strongTextColor: win.strongTextColor
        activeButtonColor: backend.themeAccent
        containerWidth: win.width
        containerHeight: win.height

        onDiscardRequested: {
            backend.discardRecovery();
            win.completePendingAction();
        }

        onSaveRequested: {
            win.awaitingPendingSave = true;
            backend.save();
        }
        onCancelRequested: win.pendingAction = ""
    }

    ExternalChangeDialog {
        id: externalChangeDialog
        darkMode: win.darkMode
        textScale: win.textScale
        textColor: win.textColor
        strongTextColor: win.strongTextColor
        containerWidth: win.width
        containerHeight: win.height

        onKeepRequested: backend.keepExternalVersion()
        onReloadRequested: backend.reloadFromDisk()
    }

    Dialog {
        id: shortcutsDialog
        objectName: "shortcutsDialog"
        modal: true
        title: "Keyboard shortcuts"
        standardButtons: Dialog.Close
        anchors.centerIn: parent
        contentItem: Label {
            text: "Ctrl+S  Save\nCtrl+Shift+S  Save As\nCtrl+O  Open\nCtrl+N  New Window\nCtrl+F  Find\nCtrl+H  Find and Replace\nCtrl+B  Bold\nCtrl+I  Italic\nCtrl+K  Link\nCtrl+P  Print\nCtrl+J  Outline\nCtrl+E  Reading / Editing\nF11 / Super+F  Fullscreen\nCtrl+?  Shortcuts"
            lineHeight: 1.5
        }
    }

    Item {
        anchors.fill: parent

        component StatCard: Rectangle {
            id: card
            property string name
            property string label
            property string value

            width: statCards.cardWidth
            height: win.scaledSize(66)
            radius: 8
            color: Qt.rgba(win.textColor.r, win.textColor.g, win.textColor.b, 0.06)

            Column {
                anchors.centerIn: parent
                spacing: win.scaledSize(2)

                Label {
                    objectName: card.name + "Value"
                    anchors.horizontalCenter: parent.horizontalCenter
                    // A large figure in a narrow window shrinks to fit its card.
                    width: card.width - win.scaledSize(12)
                    horizontalAlignment: Text.AlignHCenter
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: win.scaledSize(11)
                    text: card.value
                    color: win.textColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(22)
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: card.label
                    // The muted colour is too faint on the card in light themes.
                    color: Qt.rgba(win.textColor.r, win.textColor.g, win.textColor.b, 0.6)
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                }
            }
        }

        Row {
            id: statCards
            anchors.top: parent.top
            anchors.topMargin: win.scaledSize(20)
            // Over the text column, which is centred in the space beside the outline.
            anchors.horizontalCenter: editorFlick.horizontalCenter
            spacing: win.scaledSize(12)

            readonly property int cardWidth: Math.floor((win.editorWidth - 3 * spacing) / 4)

            StatCard { name: "words"; label: "Words"; value: win.formatCount(backend.wordCount) }
            StatCard { name: "lines"; label: "Lines"; value: win.formatCount(backend.lineCount) }
            StatCard { name: "tokens"; label: "Tokens"; value: "≈ " + win.formatCount(backend.tokenEstimate) }
            StatCard { name: "sections"; label: "Sections"; value: win.formatCount(backend.sectionCount) }
        }

        Item {
            id: outlinePane
            anchors.top: statCards.bottom
            anchors.topMargin: win.scaledSize(28)
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            // Clear of the footer's buttons below.
            anchors.bottomMargin: win.scaledSize(40)
            width: win.outlineWidth

            ListView {
                id: outlineList
                objectName: "outlineList"
                anchors.fill: parent
                anchors.leftMargin: win.scaledSize(16)
                anchors.rightMargin: win.scaledSize(8)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: backend.outline
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                // The arrows move the selection by the list's own key navigation,
                // which leaves the text where it is.
                Keys.onReturnPressed: win.jumpToHeading(currentIndex)
                Keys.onEnterPressed: win.jumpToHeading(currentIndex)
                Keys.onEscapePressed: win.focusText()
                // Opening a file without headings from here leaves nothing to select.
                onCountChanged: {
                    if (count === 0 && activeFocus)
                        win.focusText();
                }

                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    readonly property bool untitled: modelData.title === ""
                    readonly property bool marked: index === win.markedHeading
                    readonly property bool selected: ListView.isCurrentItem && ListView.view.activeFocus

                    width: ListView.view.width
                    height: win.scaledSize(30)
                    radius: 6
                    color: marked
                        ? Qt.rgba(win.textColor.r, win.textColor.g, win.textColor.b, 0.10)
                        : entryMouse.containsMouse
                            ? Qt.rgba(win.textColor.r, win.textColor.g, win.textColor.b, 0.05)
                            : "transparent"
                    // The keyboard selection is a ring, apart from the mark's tint.
                    border.width: selected ? 1 : 0
                    border.color: backend.themeAccent

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: win.scaledSize(10) + (modelData.level - 1) * win.scaledSize(14)
                        anchors.rightMargin: win.scaledSize(10)
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        text: untitled ? "Untitled heading" : modelData.title
                        color: marked ? backend.themeAccent
                            : untitled ? win.mutedColor : win.textColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(13)
                        font.weight: modelData.level === 1 ? Font.DemiBold : Font.Normal
                    }

                    MouseArea {
                        id: entryMouse
                        objectName: "outlineEntry"
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            win.jumpToHeading(index);
                            win.focusText();
                        }
                    }
                }
            }

            Label {
                objectName: "noHeadings"
                visible: outlineList.count === 0
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: win.scaledSize(26)
                text: "No headings"
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
            }
        }

        Flickable {
            id: editorFlick
            objectName: "editorFlick"
            anchors.top: statCards.bottom
            anchors.left: outlinePane.right
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            clip: true
            contentWidth: width
            contentHeight: Math.max(height, editor.y
                + (win.reading ? reader.implicitHeight : editor.implicitHeight) + 220)
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                // Wheel scrolling moves contentY directly rather than
                // flicking the Flickable, so the bar has to be told about
                // that activity; linger briefly after the last event.
                active: hovered || pressed || wheelScroll.running || scrollLinger.running
                // Stop above the footer strip so the bar stays clear of it.
                // Padding and inset, not anchors: the attached-ScrollBar layout
                // overrides anchors. Padding stops the thumb, the inset the track.
                bottomPadding: win.scaledSize(32)
                bottomInset: win.scaledSize(32)
            }

            Timer {
                id: scrollLinger
                interval: 600
            }

            // Flickable turns a wheel notch into a flick sized by the small
            // application font, which crawls next to a browser. Reproduce
            // Chromium's wheel physics instead (cc::ScrollOffsetAnimationCurve):
            // each notch moves 3 lines of 40px towards a running target, the
            // animation gets shorter as the outstanding distance grows, and a
            // notch landing mid-animation carries the current velocity into
            // the new curve, so sustained spinning keeps picking up speed.
            readonly property real wheelStep: win.scaledSize(120)

            FrameAnimation {
                id: wheelScroll
                running: false

                property real startY: 0
                property real targetY: 0
                property real duration: 0.2
                // Cubic bezier easing; ease-in-out (0.42, 0, 0.58, 1) for a
                // fresh scroll, with y1 tilted on retarget so the curve's
                // initial slope matches the velocity it inherits.
                property real cx1: 0.42
                property real cy1: 0
                readonly property real cx2: 0.58
                readonly property real cy2: 1

                onTriggered: {
                    var x = elapsedTime / duration;
                    if (x >= 1) {
                        editorFlick.contentY = editorFlick.snapToPixel(targetY);
                        stop();
                        return;
                    }
                    editorFlick.contentY = editorFlick.snapToPixel(
                        startY + (targetY - startY) * curveY(solveCurve(x)));
                }

                function begin(from, to, dur, slope) {
                    startY = from;
                    targetY = to;
                    duration = dur;
                    cx1 = 0.42;
                    cy1 = 0.42 * Math.max(-1000, Math.min(1000, slope));
                    restart();
                }

                function retarget(newTarget) {
                    var s = solveCurve(Math.min(1, elapsedTime / duration));
                    var pos = startY + (targetY - startY) * curveY(s);
                    var delta = newTarget - pos;
                    if (Math.abs(delta) < 0.5) {
                        editorFlick.contentY = newTarget;
                        stop();
                        return;
                    }

                    var velocity = curveDY(s) / Math.max(1e-6, curveDX(s))
                        * (targetY - startY) / duration;
                    var dur = editorFlick.wheelDuration(delta);
                    // When already moving faster than the eased curve would,
                    // bound the duration by the time to target at the current
                    // velocity; the 2.5x covers the ease-out tail.
                    if (velocity !== 0 && delta / velocity > 0)
                        dur = Math.min(dur, delta / velocity * 2.5);
                    begin(pos, newTarget, dur, velocity * dur / delta);
                }

                // Cubic bezier through (0,0), (cx1,cy1), (cx2,cy2), (1,1),
                // evaluated by Newton-solving the curve parameter from x.
                function curveX(s) { return 3 * s * (1 - s) * ((1 - s) * cx1 + s * cx2) + s * s * s; }
                function curveY(s) { return 3 * s * (1 - s) * ((1 - s) * cy1 + s * cy2) + s * s * s; }
                function curveDX(s) { return 3 * (1 - s) * (1 - s) * cx1 + 6 * (1 - s) * s * (cx2 - cx1) + 3 * s * s * (1 - cx2); }
                function curveDY(s) { return 3 * (1 - s) * (1 - s) * cy1 + 6 * (1 - s) * s * (cy2 - cy1) + 3 * s * s * (1 - cy2); }

                function solveCurve(x) {
                    var s = x;
                    for (var i = 0; i < 8; ++i) {
                        var error = curveX(s) - x;
                        if (Math.abs(error) < 0.001)
                            break;
                        var d = curveDX(s);
                        if (Math.abs(d) < 1e-6)
                            break;
                        s = Math.max(0, Math.min(1, s - error / d));
                    }
                    return s;
                }
            }

            WheelHandler {
                // Wayland compositors route every pointer's scroll through
                // one seat device that Qt classifies as a touchpad, so the
                // device type cannot tell a mouse wheel from two-finger
                // scrolling. Distinguish by event shape instead: discrete
                // wheel notches arrive with only angleDelta set, while
                // finger scrolling carries pixel-precise pixelDelta.
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: function(wheel) {
                    scrollLinger.restart();
                    if (wheel.pixelDelta.y !== 0)
                        editorFlick.scrollTo(editorFlick.clampContentY(editorFlick.contentY - wheel.pixelDelta.y));
                    else
                        editorFlick.scrollByWheel(wheel);
                    wheel.accepted = true;
                }
            }

            onMovementStarted: wheelScroll.stop()

            onContentYChanged: {
                if (win.reading && win.searchOpen)
                    Qt.callLater(win.showReadingMatches);
                win.markHeld = false;
                win.updateMark();
            }

            function scrollByWheel(wheel) {
                // High-resolution wheels report fractional notches; feed
                // those through the same animated path, like Chromium does
                // for every wheel-source event.
                var notches = wheel.angleDelta.y / 120;
                if (notches === 0)
                    return;

                if (wheelScroll.running) {
                    wheelScroll.retarget(clampContentY(wheelScroll.targetY - notches * wheelStep));
                    return;
                }

                var target = clampContentY(contentY - notches * wheelStep);
                if (target !== contentY)
                    wheelScroll.begin(contentY, target, wheelDuration(target - contentY), 0);
            }

            // Chromium's inverse-delta duration: 200ms for a single notch,
            // ramping down to 100ms once 480px are outstanding.
            function wheelDuration(delta) {
                var pixels = Math.abs(delta) / win.textScale;
                return Math.max(6, Math.min(12, 14 - pixels / 60)) / 60;
            }

            function clampContentY(y) {
                return Math.max(0, Math.min(Math.max(0, contentHeight - height), y));
            }

            // Whole device pixels keep natively hinted glyphs from
            // re-rasterizing mid-animation, which reads as shimmer.
            function snapToPixel(y) {
                return Math.round(y * Screen.devicePixelRatio) / Screen.devicePixelRatio;
            }

            // Jump to a position, abandoning any wheel animation still running.
            function scrollTo(y) {
                wheelScroll.stop();
                contentY = snapToPixel(y);
            }

            // Keep the editing caret within the viewport so writing past the
            // bottom edge scrolls the page along with the text.
            // Scrolls the least that shows a span of the page with a margin round it.
            function ensureVisible(top, bottom) {
                var margin = win.editorFontPixelSize * 2;
                var maxContentY = Math.max(0, contentHeight - height);
                if (bottom + margin > contentY + height)
                    scrollTo(Math.min(maxContentY, bottom + margin - height));
                else if (top - margin < contentY)
                    scrollTo(Math.max(0, top - margin));
            }

            function ensureCursorVisible() {
                // The editor is hidden while reading, and its cursor must not
                // move the page.
                if (win.reading)
                    return;
                var cursorTop = editor.y + editor.cursorRectangle.y;
                ensureVisible(cursorTop, cursorTop + editor.cursorRectangle.height);
            }

            TextEdit {
                id: editor
                objectName: "sourceEditor"
                visible: !win.reading
                x: Math.round((editorFlick.width - width) / 2)
                y: Math.max(42, Math.round(win.height * 0.05))
                width: win.editorWidth
                height: Math.max(editorFlick.height - y - 96, implicitHeight + 20)
                text: ""
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                persistentSelection: true
                activeFocusOnPress: true
                color: win.textColor
                selectedTextColor: win.strongTextColor
                selectionColor: win.selectionFill
                font.family: "iA Writer Mono S"
                font.pixelSize: win.editorFontPixelSize
                font.weight: Font.Normal
                // Native rendering hints glyphs to the pixel grid, which is
                // crispest at whole scale factors but misplaces and unevenly
                // rasterizes glyphs at fractional ones (and goes stale when
                // the compositor delivers the fractional scale after the
                // first frame). Fall back to Qt's scalable renderer there.
                renderType: Screen.devicePixelRatio % 1 === 0 ? TextEdit.NativeRendering : TextEdit.QtRendering
                cursorDelegate: Rectangle {
                    width: 1
                    color: win.strongTextColor
                }
                onCursorRectangleChanged: editorFlick.ensureCursorVisible()
                // The text has rewrapped by now, so headings are where they will be drawn.
                onWidthChanged: win.updateMark()

                function replaceSelectionWith(replacement) {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    EditorMutations.replaceRange(editor, start, end, replacement);
                }

                function wrapSelection(before, after) {
                    forceActiveFocus();
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    var selected = text.slice(start, end);
                    EditorMutations.replaceRange(editor, start, end,
                                                 before + selected + after,
                                                 before.length,
                                                 before.length + selected.length);
                }

                function insertLink() {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    var selected = text.slice(start, end);
                    var url = backend.clipboardUrl();
                    var label = selected.length > 0 ? selected : "link text";
                    var destination = url.length > 0 ? url : "https://";
                    var escapedLabel = escapeMarkdownLinkText(label);
                    var markdown = "[" + escapedLabel + "](" + escapeMarkdownLinkDestination(destination) + ")";
                    if (selected.length === 0) {
                        EditorMutations.replaceRange(editor, start, end, markdown,
                                                     1, 1 + escapedLabel.length);
                    } else if (url.length === 0) {
                        EditorMutations.replaceRange(editor, start, end, markdown,
                                                     escapedLabel.length + 3,
                                                     markdown.length - 1);
                    } else {
                        EditorMutations.replaceRange(editor, start, end, markdown);
                    }
                }

                function smartReturn(softBreak) {
                    if (softBreak) {
                        replaceSelectionWith("\n");
                        return;
                    }
                    var lineStart = text.lastIndexOf("\n", cursorPosition - 1) + 1;
                    var line = text.slice(lineStart, cursorPosition);
                    var before = text.slice(0, cursorPosition);
                    var fences = (before.match(/^\s*```/gm) || []).length;
                    if ((fences % 2) === 1) {
                        replaceSelectionWith("\n");
                        return;
                    }
                    var match = line.match(/^(\s*)([-+*]|\d+[.)]|>+)\s+(.*)$/);
                    if (match) {
                        if (match[3].length === 0) {
                            EditorMutations.replaceRange(editor, lineStart,
                                                         cursorPosition, "\n");
                        } else {
                            var marker = match[2];
                            if (/^\d/.test(marker))
                                marker = (parseInt(marker) + 1) + marker.slice(-1);
                            replaceSelectionWith("\n" + match[1] + marker + " ");
                        }
                        return;
                    }
                    replaceSelectionWith("\n\n");
                }

                function escapeMarkdownLinkText(linkText) {
                    return linkText.replace(/\\/g, "\\\\")
                                   .replace(/\[/g, "\\[")
                                   .replace(/\]/g, "\\]");
                }

                function escapeMarkdownLinkDestination(linkUrl) {
                    return linkUrl.replace(/\\/g, "\\\\")
                                  .replace(/\(/g, "\\(")
                                  .replace(/\)/g, "\\)");
                }

                function pasteClipboardUrlAsMarkdownLink() {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    if (start === end)
                        return false;

                    var url = backend.clipboardUrl();
                    if (url === "")
                        return false;

                    var selected = text.slice(start, end);
                    var leading = selected.match(/^\s*/)[0];
                    var trailing = selected.match(/\s*$/)[0];
                    var linkText = selected.slice(leading.length,
                                                  selected.length - trailing.length);
                    if (linkText === "")
                        return false;

                    replaceSelectionWith(leading + "[" + escapeMarkdownLinkText(linkText) + "]("
                                         + escapeMarkdownLinkDestination(url) + ")" + trailing);
                    return true;
                }

                function pasteClipboardAsPlainText() {
                    var pastedText = backend.clipboardText();
                    if (pastedText.length > 0)
                        replaceSelectionWith(pastedText);
                }

                function skipHiddenForward(position) {
                    var pos = position;
                    var ranges = backend.hiddenRangesAt(pos);
                    for (var i = 0; i < ranges.length; i++) {
                        if (pos >= ranges[i].start && pos < ranges[i].end) {
                            pos = ranges[i].end;
                            i = -1;
                        }
                    }
                    return pos;
                }

                function skipHiddenBackward(position) {
                    var pos = position;
                    var ranges = backend.hiddenRangesAt(pos);
                    for (var i = ranges.length - 1; i >= 0; i--) {
                        if (pos > ranges[i].start && pos <= ranges[i].end) {
                            pos = ranges[i].start;
                            i = ranges.length;
                        }
                    }
                    return pos;
                }

                function moveCursorVisibly(direction) {
                    if (selectionStart !== selectionEnd) {
                        cursorPosition = direction > 0
                            ? Math.max(selectionStart, selectionEnd)
                            : Math.min(selectionStart, selectionEnd);
                        return;
                    }

                    var pos = Math.max(0, Math.min(text.length, cursorPosition + direction));
                    cursorPosition = direction > 0
                        ? skipHiddenForward(pos)
                        : skipHiddenBackward(pos);
                }

                function movePage(direction, extendSelection) {
                    var pageStep = Math.max(win.editorFontPixelSize,
                                            editorFlick.height - win.editorFontPixelSize * 2);
                    var rect = cursorRectangle;
                    var targetY = rect.y + rect.height / 2 + direction * pageStep;
                    var target = positionAt(rect.x, Math.max(0, targetY));
                    if (extendSelection)
                        moveCursorSelection(target, TextEdit.SelectCharacters);
                    else
                        cursorPosition = target;
                }

                function deleteParagraphBreakBehindCursor() {
                    if (selectionStart !== selectionEnd || cursorPosition < 2)
                        return false;

                    if (text.slice(cursorPosition - 2, cursorPosition) !== "\n\n")
                        return false;

                    var start = cursorPosition - 2;
                    remove(start, cursorPosition);
                    cursorPosition = start;
                    return true;
                }

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: function(event) {
                    var pasteKey = (event.key === Qt.Key_V)
                        && (event.modifiers & Qt.ControlModifier)
                        && !(event.modifiers & (Qt.AltModifier | Qt.MetaModifier | Qt.ShiftModifier));
                    var shiftInsert = (event.key === Qt.Key_Insert)
                        && (event.modifiers & Qt.ShiftModifier)
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier));
                    if (pasteKey || shiftInsert) {
                        if (!pasteClipboardUrlAsMarkdownLink())
                            pasteClipboardAsPlainText();
                        event.accepted = true;
                        return;
                    }

                    var returnKey = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                    var commandModifier = event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier);
                    if (returnKey && !commandModifier) {
                        smartReturn(event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    } else if (!commandModifier && event.key === Qt.Key_Backspace
                               && deleteParagraphBreakBehindCursor()) {
                        event.accepted = true;
                    } else if (!commandModifier && !(event.modifiers & Qt.ShiftModifier)
                               && event.key === Qt.Key_Right) {
                        moveCursorVisibly(1);
                        event.accepted = true;
                    } else if (!commandModifier && !(event.modifiers & Qt.ShiftModifier)
                               && event.key === Qt.Key_Left) {
                        moveCursorVisibly(-1);
                        event.accepted = true;
                    } else if (!commandModifier
                               && (event.key === Qt.Key_PageDown || event.key === Qt.Key_PageUp)) {
                        movePage(event.key === Qt.Key_PageDown ? 1 : -1,
                                 event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    }
                }

                onTextChanged: {
                    if (win.searchUpdating)
                        return;
                    var contentChanged = backend.editorTextChanged();
                    if (win.searchOpen && contentChanged)
                        win.updateSearch();
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    text: "# Start writing"
                    visible: editor.text.length === 0 && !editor.activeFocus
                    color: win.mutedColor
                    font.family: editor.font.family
                    font.pixelSize: editor.font.pixelSize
                    font.weight: editor.font.weight
                }

                Component.onCompleted: {
                    backend.attachDocument(textDocument);
                    forceActiveFocus();
                }
            }

            // The text view draws no table cell backgrounds, so the header rows'
            // shade is drawn behind the page.
            Repeater {
                model: win.reading ? win.readingHeaderRows : []

                Rectangle {
                    x: reader.x + modelData.x
                    y: reader.y + modelData.y
                    width: modelData.width
                    height: modelData.height
                    color: win.readingShade
                }
            }

            // The rendered page. The backend fills its document from the text; it
            // is never edited, and nothing in it is written back.
            TextEdit {
                id: reader
                objectName: "readingView"
                visible: win.reading
                x: editor.x
                y: editor.y
                width: editor.width
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                color: win.textColor
                selectedTextColor: win.strongTextColor
                selectionColor: win.selectionFill
                font.family: "iA Writer Duo S"
                font.pixelSize: win.editorFontPixelSize
                renderType: editor.renderType
                onLinkActivated: function(link) {
                    backend.openExternalUrl(link);
                }
                // Code blocks are fitted to the column, so a new width rebuilds
                // the page, once the width has settled.
                onWidthChanged: readerWidthSettled.restart()
                // A new layout moves the header rows and the headings.
                onContentHeightChanged: {
                    Qt.callLater(win.showHeaderShades);
                    Qt.callLater(win.updateMark);
                    Qt.callLater(win.showReadingMatches);
                }

                Timer {
                    id: readerWidthSettled
                    objectName: "readerWidthSettled"
                    interval: 150
                    onTriggered: win.rerenderReading()
                }

                HoverHandler {
                    cursorShape: reader.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor
                }

                Component.onCompleted: backend.attachReadingDocument(textDocument)
            }

            // Find's highlights on the page, drawn over it and a little clear so the
            // text shows through: the page's own boxes would hide anything beneath.
            Repeater {
                model: win.readingMatchBoxes

                Rectangle {
                    x: reader.x + modelData.x
                    y: reader.y + modelData.y
                    width: modelData.width
                    height: modelData.height
                    opacity: 0.5
                    color: modelData.current
                        ? (win.darkMode ? "#b36b20" : "#ffad42")
                        : (win.darkMode ? "#725b18" : "#ffe58a")
                }
            }
        }

        Row {
            id: footerStatus
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.bottomMargin: 10
            spacing: 12
            opacity: 0.55

            FooterIconButton {
                objectName: "saveButton"
                iconName: "save"
                iconColor: win.mutedColor
                tooltip: "Save"
                onClicked: backend.save()
            }

            FooterIconButton {
                objectName: "openButton"
                iconName: "open"
                iconColor: win.mutedColor
                tooltip: "Open"
                onClicked: backend.openDialog()
            }

            Label {
                text: backend.status
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                visible: text !== ""
                elide: Text.ElideRight
                width: Math.min(360, win.width / 3)
                height: win.scaledSize(16)
                verticalAlignment: Text.AlignVCenter
            }
        }


        Pane {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 12
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: win.scaledSize(win.replaceOpen ? 104 : 56)
            visible: win.searchOpen
            z: 10
            leftPadding: 16
            rightPadding: 8
            topPadding: 0
            bottomPadding: 0
            Material.elevation: 8

            background: Rectangle {
                radius: 9
                color: win.darkMode ? "#22221f" : "#fffef2"
            }

            RowLayout {
                anchors.fill: parent
                spacing: 8

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextInput {
                        id: searchField
                        objectName: "searchField"
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: win.replaceOpen ? parent.height / 2 : parent.height
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        color: win.textColor
                        selectionColor: win.selectionFill
                        selectedTextColor: win.strongTextColor
                        font.pixelSize: win.scaledSize(17)
                        clip: true
                        onTextChanged: win.updateSearch()
                        Keys.onReturnPressed: function(event) {
                            win.moveSearch((event.modifiers & Qt.ShiftModifier) ? -1 : 1);
                            event.accepted = true;
                        }
                        Keys.onEscapePressed: function(event) {
                            win.closeSearch();
                            event.accepted = true;
                        }
                    }

                    TextInput {
                        id: replaceField
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height / 2
                        visible: win.replaceOpen
                        verticalAlignment: TextInput.AlignVCenter
                        color: win.textColor
                        selectionColor: win.selectionFill
                        selectedTextColor: win.strongTextColor
                        font.pixelSize: win.scaledSize(17)
                        Keys.onReturnPressed: replaceCurrentButton.clicked()
                    }

                    Label {
                        anchors.verticalCenter: replaceField.verticalCenter
                        text: "Replace with"
                        visible: win.replaceOpen && replaceField.text.length === 0
                        color: win.mutedColor
                        font.pixelSize: win.scaledSize(17)
                    }

                    Label {
                        anchors.verticalCenter: searchField.verticalCenter
                        text: "Find"
                        visible: searchField.text.length === 0
                        color: win.mutedColor
                        font.pixelSize: win.scaledSize(17)
                    }
                }

                Label {
                    Layout.preferredWidth: win.scaledSize(58)
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: win.searchMatches.length === 0
                        ? "0/0"
                        : (win.searchMatchIndex + 1) + "/" + win.searchMatches.length
                    color: win.darkMode ? win.textColor : "#62635f"
                    font.pixelSize: win.scaledSize(16)
                }

                Button {
                    id: replaceCurrentButton
                    visible: win.replaceOpen
                    text: "Replace"
                    onClicked: {
                        if (win.searchMatchIndex < 0) return;
                        var match = win.searchMatches[win.searchMatchIndex];
                        EditorMutations.replaceRange(editor, match.start, match.end,
                                                     replaceField.text);
                        win.updateSearch();
                    }
                }

                Button {
                    visible: win.replaceOpen
                    text: "All"
                    onClicked: {
                        if (searchField.text.length === 0) return;
                        for (var i = win.searchMatches.length - 1; i >= 0; --i) {
                            var match = win.searchMatches[i];
                            EditorMutations.replaceRange(editor, match.start, match.end,
                                                         replaceField.text);
                        }
                        win.updateSearch();
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 34
                    color: win.darkMode ? "#6f6f62" : "#d5d56e"
                }

                SearchIconButton {
                    iconName: "up"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.moveSearch(-1)
                }

                SearchIconButton {
                    iconName: "down"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.moveSearch(1)
                }

                SearchIconButton {
                    iconName: "close"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.closeSearch()
                }
            }
        }
    }

    Component.onCompleted: {
        var geometry = backend.windowGeometry();
        if (geometry.x >= 0) x = geometry.x;
        if (geometry.y >= 0) y = geometry.y;
        width = geometry.width;
        height = geometry.height;
        if (geometry.maximized) showMaximized();
    }

    Component.onDestruction: backend.saveWindowGeometry(x, y, width, height, visibility === Window.Maximized)

}
