---
type: architecture
description: A single-window Qt Quick app with a C++ core, in which one editable plain-text document is scanned into structure that drives its styling, its outline and its stats, and is rendered into a throwaway reading view.
register: project
---

# Architecture: Leaf

Status: approved

Leaf is a desktop app, one process per window, built on the Qt Quick interface and C++
core it inherits from Omawrite. The core idea is that **the text on screen is the
file's own text**. The editor holds the Markdown exactly as it is on disk, and
everything that makes it pleasant to read -- styled headings, hidden markers, shaded
code, lined-up tables, the outline, the stats -- is derived from that text and laid
over it, never written back into it. One scan of the text for its block structure
(headings, code blocks, tables, front matter) feeds the styling, the outline and the
stats, so the three always agree about what the document contains.

The reading view, where every document opens, is the one exception to drawing over
the text: it is a **rendered copy**, built from the text by Qt's Markdown importer,
restyled, shown, and rebuilt or thrown away. It is never saved and never read back,
so the file's text still has one home, the editor.

```
  command line,          desktop portal            Omarchy theme files
  file manager         (file picker, dark mode,   (colours, watched live)
       |                  text size setting)               |
       |                        |                          |
       |                   System theme                    |
       |                        |                          |
       v                        v                          v
  file on disk  <--->  Document core  <--- colours, dark mode, text size
  recovery slot <--->   (open, save, watch, recover, settings, print,
  settings      <--->    new window, outline, stats)
  printer       <----       |        ^
  new process   <----       |        |
                            v        |
                      Structure scan-+   (whole text, after typing pauses)
                            .
                            . same line rules, applied line by line as text changes
                            v
                       Highlighter          Reading renderer <- text, theme
                            |                (rendered copy, never saved)
                            v                       |
   +-------------------------------- Window --------------------------------+
   |  Stat cards (top)          <- stats from the core                      |
   |  Outline pane (left)       <- outline from the core                    |
   |  Reading view (read-only)  <- rendered copy     } one at a time,       |
   |  Editor (plain text, styled by the highlighter) } Ctrl+E switches      |
   |  Footer (file controls), find and replace, dialogs                     |
   +------------------------------------------------------------------------+
                            |
                            v
                   links handed to the desktop
```

## Components

**Window.** The whole interface, declared in Qt Quick: the editor, the outline pane,
the stat cards, the footer, find and replace, the dialogs, the shortcuts and the
wheel-scrolling behaviour. It owns layout, focus and scroll position, including
opening every document at its start in the reading view, switching between the
reading and editing views with Ctrl+E while keeping the place, and scrolling a picked
heading to the top in either.
It displays what the core computes and calls the core for anything that touches a
file. It does not parse Markdown, count anything or touch the filesystem.

**Document core.** The one document in this window and everything with a side
effect: opening and saving the file, watching it for outside changes, writing and
restoring crash recovery, remembering settings, loading and watching the Omarchy theme
colours, printing, and starting a new window. It exposes the document's state and
the derived outline and stats to the window as properties. It does not know how the
window is laid out.

**Structure scan.** New. The rules that classify a line -- heading (with its level),
opening or closing a code block, inside a code block, table row, front matter, or
prose -- given what came before it. It is the single definition of block-level
Markdown in Leaf, so a `#` line inside a code block is not a heading anywhere. It is
used two ways: the core runs it over the whole text to build the outline and stats,
and the highlighter applies the same rules line by line as it restyles. It is pure:
text in, structure out, no Qt interface, which makes it directly testable. It does
not style or display anything.

**Outline** and **Stats.** New. Both are derived from the structure scan and the text
by the core, and exposed to the window: the outline as the ordered list of headings
with their levels and character positions in the text, the stats as words, lines, estimated tokens and
sections. One recount runs the scan once and derives both, at once on load and
120 ms after typing stops, so a burst of typing does not re-derive them on every
key. Neither holds anything the text does not already say.

**Highlighter.** Styles the text in place: headings, bold, italic, links, code, block
quotes and search matches, and hides inline markers away from the cursor. It remains
the single definition of inline Markdown, which the editor also uses to step the caret
over hidden markers. It gains block awareness by applying the structure scan's rules
as it goes, carrying whether a line is inside a code block on to the next line, so
typing an opening fence restyles everything below it at once. Front matter depends on
a closing line that can be far from an edit, so the highlighter works out its extent
when it styles the first line, and the core's recount asks it to check again after
typing. It changes how text looks, never what the text is.

**Reading renderer.** New. Builds the reading view's document from the text: takes a
display copy with the lines Qt's importer misreads left out (front matter, and lines
that are only an XML-like tag such as `<task>`) and unclosed void HTML tags such as
`<br>` closed, imports it with Qt's Markdown support, then walks the result and
restyles it -- spacing, heading sizes, colours from the theme, table borders and
header shading, code shading, the language label. Images load only from this
machine; any other image shows its alternative text. It also lists the rendered
headings, so each outline entry can be matched to its heading in the rendered copy,
in order, and the outline, the jump and the reading mark work there. It is fed text
and a style and fills the document it is given, which belongs to the view showing
it; it holds nothing the text does not already say, and nothing it builds is ever
written anywhere.

**System theme.** Reads the desktop's dark-mode and text-size settings through the
desktop portal and reports changes as they happen. The core receives them and passes
them on to the window and the highlighter. It knows nothing about documents.

## Data flow

**Opening.** A path arrives from the command line, the file manager or the open
dialog. The core reads the file, keeps its exact bytes as the last known contents,
loads the text into the editor's document, starts watching the file, and clears any
recovery snapshot. The highlighter styles the text; the core derives the outline and
stats; the reading renderer builds the rendered copy. The core then announces that a document was loaded, and the window answers by
showing the reading view at its start, with the editor's cursor at the start too. Every load passes
through that one point -- opening, reloading after an outside change, and restoring
a recovery snapshot -- so all of them start at the top. Without it the cursor is
left at the end of the new text and the view follows it there.

**Reading and jumping.** Scrolling is the window's alone. As the view moves, the
window works out which heading is at the top and marks it in the outline. Picking an
outline entry scrolls the editor so that heading's position is at the top of the view.
Positions come from the last scan, so during a pause in typing the outline can be a
moment behind the text. Nothing here touches the core or the file. Ctrl+J moves focus
between the text and the outline; there, the arrows move a selection without moving
the text, and Enter jumps while keeping focus in the outline. A picked heading stays
marked, even one too near the end to reach the top, until the view next moves.

**Switching views.** Ctrl+E swaps the reading view and the editor. The window notes
the heading at the top of the one being left and brings the same heading, found by
its place in the outline, to the top of the other. Within a section the place is
kept as closely as the two layouts allow. The editor is hidden, not destroyed, while
reading, so its undo history and cursor survive.

**Editing.** A keystroke changes the editor's document. The highlighter restyles the
changed lines; the core marks the document modified, schedules a recovery snapshot,
and schedules a recount of the outline and stats. The file on disk is untouched until
the owner saves. The rendered copy is rebuilt from the text when the owner returns to
the reading view, and after a reload.

**Saving.** The core writes the document's text to a temporary file and atomically
replaces the target with it, records the written bytes as the last known contents,
and clears the recovery snapshot. A change the watcher then reports is compared with
those bytes, so Leaf's own save is not mistaken for an outside edit.

**Where state lives.** The document's text lives only in the editor's document.
Recovery snapshots and settings (window size, last save folder) live in Leaf's own
per-user state directory. Theme colours are read from Omarchy's state and held in the
core. The outline, stats, styling and the rendered copy
are always derived and never stored.

## Decisions

### Why build on Omawrite's Qt editor

Chosen: keep Omawrite's Qt Quick interface and C++ core, and add to it. The design
chose this approach (A) because the daily problems are quick to fix on it, and because
theme following, file safety, fonts, scrolling feel and packaging are already solved
and tested. Rejected: a web-based editor inside Leaf (the design's approach B), which
draws tables well but means rewriting most of the interface and carrying a browser
engine. Reversing it is a rewrite of the Window; the core's file handling could be
kept behind a web view.

### Why reading is a rendered copy beside the editor

Chosen: every document opens in a read-only reading view, a document Qt's Markdown
importer builds from the text and Leaf restyles, and the editor stays as the editing
view, one key away. The design chose this (approach D) because styled raw text still
reads as raw Markdown -- visible marks, lines broken mid-sentence, no grids -- and no
styling can join lines or draw grids without changing the text. The rendered copy is
safe for the same reason printing is: it is built, shown and thrown away, never saved
and never read back into the text. Rejected: drawing grids and boxes over the
editable text (the earlier plan for approach A), which cannot fix wrapped lines;
building a view from Leaf's own widgets block by block, which gives full control at a
much higher cost and stays open for any part the importer falls short on; a web engine,
as before. Reversing it is contained: remove the reading view and the renderer, and
the editor is what it was.

### Why the display copy may leave lines out

Chosen: the renderer imports a copy of the text with front matter and bare tag lines
left out, and unclosed void HTML tags closed (`<br>` as `<br/>`), rather than the text
itself. Qt's importer reads a line such as `<task>` as the start of an HTML block and
swallows the lists and tables after it, which LLM prompt files are full of, and drops
everything after an unclosed `<br>`, which LLM tables use for line breaks. The copy is
only ever displayed, so these changes to it change nothing on disk and nothing in the
editor. They are the only changes made to the text on its way to the screen, each one
there because the importer loses content without it; everything else is styling of
what the importer built.
Rejected: patching the importer's result afterwards, which cannot recover content the
importer has already swallowed.

### Why the editor holds the file's text, not a rendered document

Chosen: the editor holds plain text, and all formatting is styling laid over it.
This is what guarantees Leaf never reformats a file: what is saved is exactly what is
in the editor, which is exactly what was opened plus the owner's edits. Rejected: a
rich document built from the Markdown and written back out on save (the design's
approach C), which rewrites the file's Markdown in the toolkit's style. Printing and the
reading view are the places a rendered copy is built, because it is thrown away and
never saved. Reversing this would put every file at risk of being rewritten, and would need
a round-trip guarantee the toolkit does not give.

### Why formatting only changes how text is drawn

Chosen: in the editing view, styling is produced by display properties -- colours, spacing, backgrounds, shapes painted
over the text -- and never by inserting or removing characters. Inserting padding to
align a table would change the file on save. Rejected: normalising tables on open.
Reversing it would break the never-reformat rule. Grids and boxes are drawn in the
reading view instead, from its rendered copy.

### Why one structure scan feeds the highlighter, the outline and the stats

Chosen: one line-based scan defines headings, code blocks, tables and front matter,
and all three consumers read it. Otherwise the outline could list a heading the
highlighter shows as code, or the section count could disagree with the outline.
Rejected: each consumer finding headings for itself, as the highlighter does today.

### Why a small hand-written scan rather than a Markdown library

Chosen: a line scanner for the handful of block types Leaf needs. Headings, fenced
code, tables and front matter are recognisable line by line, and the scan must report
positions in the editor's own text, which a library that builds its own tree would
need mapping back to. Qt reads Markdown only into its own rich document, not as
positions in plain text; that is why Qt's importer builds the reading view, where no
positions are needed, but not the outline, whose entries are then matched to the
rendered headings in order. Rejected: adding a
full Markdown parser as a dependency. Reversing it is contained: the scan's output
shape stays, and a library could produce it instead.

### Why the outline and stats are computed in C++, not in the interface

Chosen: the core derives them and the window only displays them. The existing test
suite can then check them directly, the same way it checks the word count today, and
the interface stays free of parsing. Rejected: computing them in the interface's
script. Cheap to reverse, but it would move logic out of reach of the tests.

### Why one document per window, one process per window

Chosen: kept from Omawrite. A new window is a new process; each process owns one
document, one recovery slot and one file watch. It keeps every piece of state simple
and a crash confined to one window. Rejected: tabs or a multi-document window, which
the design leaves out of scope. Reversing it would touch the core's single-document
assumption throughout.

### Why Leaf has its own identity, separate from Omawrite

Chosen: its own program name, application identity, desktop entry, icon and package,
so it installs beside Omawrite, keeps its settings and recovery in its own place, and
can be made the default for Markdown without touching Omawrite. Omawrite's saved
settings and recovery snapshots are not carried over. Cheap to reverse in code, but
every name that users or the system see would change again.

## Failure

**The file on disk.** It may fail to open or be unreadable: the core leaves the
current document alone and says so in the footer. A save may fail: the atomic replace
leaves the original file untouched, and the footer says the save failed, with the
document still marked modified. Another program may change or delete the file: the
watcher notices, and the window asks whether to reload or keep the local version,
rather than silently replacing either.

**Line endings and encoding.** Files are read and written as UTF-8 with the
platform's text mode, so a file with Windows line endings is converted to plain line
endings the first time the owner saves an edit to it. A file that is opened and closed
without saving is never written. This falls short of never changing anything the owner
did not edit, and is an accepted gap for LLM output, which uses plain line endings.

**Closing with unsaved work.** Closing a window, or opening another file into it, with
unsaved edits asks whether to save, discard or cancel.

**Leaf itself crashing.** Unsaved text is snapshotted shortly after each edit into a
locked recovery slot. On the next start the first free orphaned snapshot is claimed and
restored, so a crash in one window is recovered even while others run.

**Leaf's own state directory.** If it cannot be created or written, recovery
snapshots and remembered settings are lost silently and editing carries on. If every
recovery slot is locked by running windows, the new window runs without recovery.

**The printer and the desktop.** Printing goes through the system print dialog, and
cancelling it does nothing. Links are handed to the desktop to open, and only web and
mail links are passed on. A new window is a new process; if it cannot start, the
footer says so.

**The desktop portal.** It may be missing or slow. Every query has a short timeout so
the interface never hangs; dark mode and text size fall back to the toolkit's own
guess.

**Setting the Markdown default.** If the desktop refuses it, the install script
says so and how to set it by hand; Leaf stays installed. Programs that open files with
`xdg-open` still get the plain-text app for Markdown, because that command guesses a
file's type from its contents rather than its name; programs that go by the name,
such as the file manager, get Leaf.

**The Omarchy theme.** The theme file may be missing, malformed or mid-switch. Default
colours are set before it is read, unrecognised lines are ignored, and the watcher
re-reads it when the theme changes.

**The reading view.** Anything Qt's importer cannot render well is still shown as
text, never dropped, apart from the front matter and tag lines left out on purpose.
If the outline and the rendered headings disagree -- a heading the importer reads that
the scan does not, such as one underlined with `===` -- entries are matched in order
by level and text, and an entry with no match jumps nowhere rather than to the wrong
place.

**Unusual documents.** A file with no headings has an empty outline, which the pane
says, and a section count of zero. Front matter is left out of the outline but
counted in words, lines and tokens; a first line of `---` with no closing line is not
front matter, and the file is read as ordinary Markdown. An unclosed code fence makes the rest of
the file code, as Markdown itself defines; the outline then lists no headings past it. A table with
ragged rows is lined up as far as its rows allow. Nothing in the file is corrected.

**Large files.** The outline and stats are recomputed after typing pauses rather than
on every key, and the styling works line by line, so cost grows with the file but not
with every keystroke.

## Building and running

Built with qmake and make against Qt 6, by a script that finds the right qmake. The
tests are a Qt Test suite covering the core and the highlighter, which runs headless
against a real instance of the interface. Leaf reaches the owner as an Arch package
built locally from the repository. The package installs the program, the icon and a
desktop entry that declares Markdown files. The install script then records Leaf as
the owner's default application for Markdown, in the owner's own settings, since the
package cannot set a per-user default. It needs Qt 6 with Qt Quick Controls, the
desktop portal with a backend, and, to follow the theme, Omarchy's current-theme state.

## Not here on purpose

- No Markdown library beyond Qt's own, and no web engine. The block types the
  outline and styling need are few and line-based, the reading view uses the importer
  Qt already ships, and the interface is native Qt.
- No editing in the reading view, and no rendered copy that is ever saved.
- No reformatting, normalising or correcting of files, ever.
- No file browser, tabs or multi-document state.
- No network access. External links are handed to the desktop to open.
- No carrying over of Omawrite's settings or recovery data.
