---
type: design
description: Leaf is a Markdown viewer for Omarchy, forked from Omawrite, for reading and navigating long files produced in LLM sessions, with light editing.
register: project
---

# Design: Leaf

Status: approved

## Problem

The owner produces many Markdown files through LLM sessions: plans, specs, reports,
notes. They open them to read, scan and skip around, and only rarely to edit. On
Omarchy the default app for Markdown is Omawrite, a writing app built for someone
composing prose. For reading, it gets in the way:

- A file opens with the view at the bottom, so every read starts with scrolling back
  to the top.
- There is no way to jump between sections. The only options are scrolling and
  searching with find.
- The only figure about the file is a faint word count in the corner. Size and shape
  cannot be taken in at a glance.
- Tables and code blocks, which LLM output is full of, show as lightly styled raw
  text: the pipes and fences stay visible and the columns do not line up.

The workaround today is to scroll, to search for a heading by name, or to open the
file in something heavier.

## Users

The owner, on their own Omarchy machine, reading Markdown files that LLM sessions
produced. It is not for writers composing long prose, for which Omawrite remains the
better tool, and it is not built or packaged for anyone else.

## Concepts

- **Document** -- one Markdown file open in one window. The file on disk is plain
  text and stays exactly as written unless the owner edits it; Leaf never reformats
  it.
- **Heading** -- a Markdown heading line, one to six `#` characters and a space, at
  levels one to six. A `#` line inside a code block or front matter is not a heading.
  Headings made by underlining a line with `===` or `---` are not recognised.
- **Section** -- a heading and everything under it, up to the next heading of the same
  or a higher level. Sections nest.
- **Outline** -- the list of a document's headings in order, indented by level, shown
  in a pane to the left of the text. Every level is shown and nothing folds; a file
  with no headings shows "No headings", and front matter is never part of it. It
  marks the section currently in view. Picking an entry brings that heading to the
  top of the view.
- **Stats** -- four figures about the document, shown as cards above the text: words,
  lines, estimated tokens, and sections. Estimated tokens is a rough guide to how
  much of an LLM's context the file would take, at about four characters to a token,
  not an exact count for any model. Sections is the number of headings. All four
  describe the whole text, front matter included, and replace the old word count in
  the corner.
- **Reading view** -- how every document opens: the Markdown drawn as a finished
  page, with no `#` marks, `---` lines or fences showing, hard-wrapped lines joined
  back into paragraphs, tables as grids and code as boxes. It only displays; nothing
  typed there reaches the file. Front matter and tag lines such as `<task>`, which are
  written for tools rather than people, are left out of it.
- **Editing view** -- the file's own text, always editable, with styling laid over
  it, as in Omawrite. One key, Ctrl+E, switches between the two views, and the place
  being read is kept.
- **Formatted block** -- a table or a code block in the reading view. A table is a
  grid with a shaded header row and its columns aligned as the file says; a table too
  wide for the window wraps inside its cells. A code block is a shaded box in a
  monospaced font, with long lines wrapped and the language, when the file names one,
  as a small label. Diagrams drawn in text are code blocks and keep their shape.

## Core flows

**Open and read.** The owner opens a Markdown file, from the file manager, from a
terminal or from inside Leaf. The window shows the stats across the top, the outline
on the left and the document in the reading view from its start. Reloading after the file changes on
disk, or recovering after a crash, also starts from the first line. They read by
scrolling, and the outline follows, marking the section they are in. Find works in
the reading view; links there can be clicked.

**Jump to a section.** Without touching the mouse, the owner presses Ctrl+J to move
into the outline, steps through the headings with the arrow keys, and presses Enter.
The text jumps so that heading is at the top, and the outline keeps focus so they can
jump again. Ctrl+J, or Escape, takes them back to the text. Clicking an entry jumps
and puts them in the text.

**Make a small edit.** The owner presses Ctrl+E, and the reading view gives way to
the editing view at the same place. They type, as in Omawrite, and save; Ctrl+E takes
them back to the reading view, which shows the edit. The stats and the outline update as they type. Replace is only in the editing view,
since it changes the text. Everything
Omawrite does to protect work still applies: recovery after a crash, a warning before
a change on disk replaces local edits, a prompt before closing unsaved work.

## Constraints

- Runs on Omarchy (Arch Linux, Hyprland), and follows the system's dark or light mode,
  the live Omarchy theme and the desktop text size, as Omawrite does.
- Replaces Omawrite on the owner's machine. Installing Leaf makes it the owner's
  default app for Markdown files, and only for Markdown; plain text keeps its own
  default. Super+Shift+W, Omarchy's key for its writing app, opens Leaf; that is set
  in the owner's own desktop configuration, not shipped by Leaf.
- A complete break from Omawrite: its own name, icon and package. Omawrite's later
  changes are not pulled in.
- Keeps everything Omawrite had at the fork: its protections for the owner's work, its
  shortcuts, find and replace, printing, theme following and fonts.
- Never rewrites a file's Markdown on its own. The file on disk changes only by the
  owner's edits.

## Premises

All agreed with the owner.

- The problem is finding one's way around long LLM files -- where reading starts,
  moving between sections, sizing a file up at a glance -- not a lack of writing tools.
- Doing nothing has a real cost: these files are read daily, and every one starts
  with scrolling up from the bottom and hunting with find.
- Nothing already at hand does this well. Omawrite has no outline, opens at the bottom
  and shows tables raw. Obsidian, Typora and browser Markdown extensions are heavier,
  are not native to Omarchy, and do not follow its theme. Terminal viewers such as
  glow have no outline to move through and cannot edit.
- Building on Omawrite's code is better than starting over: theme following, file
  safety, fonts, scrolling feel and packaging are already solved and tested there.
- Drawing tables and code blocks properly while keeping the text always editable is
  the hardest part of this, and the choice of approach turns on it.
- Added 2026-09-27, after the first two cycles: styled raw text still reads as raw
  Markdown -- `#` marks, `---` lines, dashes and lines broken mid-sentence -- and the
  owner finds it unpleasant to read. Polishing it cannot join wrapped lines or draw
  real grids without changing the text, so reading needs its own rendered view.
- A rendered view built fresh from the text, shown and thrown away, never saved,
  keeps the file safe in the same way printing does.

## Approaches considered

**A. Stay on Omawrite's editor, and improve formatting in steps.** Keep the existing
Qt editor, which shows the raw Markdown with styling laid over it. First rename to
Leaf, open at the top, add the outline and the stats, shade code blocks and line up
table columns in the monospaced text. Later, draw true table grids and code boxes over
the text, showing the raw text where the cursor is. Low effort at first, medium to
high for the full drawing. Low risk. Reuses all of Omawrite. **Chosen.**

**B. Replace the editor with a web-based one inside Leaf.** Use the editor technology
Obsidian is built on, which draws tables and code boxes and reveals the raw text at
the cursor out of the box, and build the outline and stats in the same web view. High
effort: most of the interface is rewritten, and theme following, scrolling feel and
shortcuts have to be rebuilt. Medium risk: the app carries a small browser and grows
heavier, and Omawrite's polish is lost until it is redone. Not chosen: it pays a
rewrite for polish in editing, which the owner rarely does, and it remains available
if approach A's tables fall short.

**C. Use the toolkit's own rich-text tables.** Load the file into a rich document that
has real tables, and write it back out as Markdown on save. Not chosen: saving would
rewrite the file's Markdown in the toolkit's style rather than as it was written,
which breaks the constraint that Leaf never reformats a file.

**D. Add a rendered reading view beside the editor.** Added 2026-09-27. Keep
approach A's editor as the editing view, and open every file in a reading view built
from the text by the toolkit's own Markdown support -- the one printing already uses
-- then restyled to Leaf's look. Nothing built for it is ever saved. Medium effort,
low to medium risk. Reuses the editor, the outline, the stats, theme following and
find. Its limits are the toolkit's: code boxes are shaded bands rather than rounded
boxes, and wide tables wrap rather than scroll. **Chosen, on top of A.** Rejected
beside it: Leaf drawing every block itself, which gives full control at a much higher
cost and stays open for any part that falls short, and a web page inside Leaf, for
approach B's reasons.

## Chosen approach

Approach A, then D. The problems that cost the most every day -- opening at the bottom, no
way to jump between sections, stats hidden in a corner -- are all quick to fix on the
existing editor. Editing is rare, so Obsidian-grade editing of tables matters less
than it would to a writer. Formatting can improve in steps without giving up the
tested work inherited from Omawrite, and approach B stays open if the drawn tables
are not good enough.

Once A's first two cycles were done, the styled raw text was still unpleasant to read,
and reading is what the owner does most. Approach D gives reading a finished page
without touching the file, and leaves editing as it was.

## Not in scope

- Editing in the reading view. Edits are made in the editing view.
- Remembering a view per file. Every file opens in the reading view.
- A folder or file browser, tabs, or several documents in one window. One document
  per window.
- An exact token count for any particular model.
- Pulling in Omawrite's later changes.
- Publishing or packaging Leaf for anyone but the owner.
- New writing tools beyond those Omawrite already has.
- Making the `xdg-open` command hand Markdown to Leaf. It guesses types from file
  contents and sends Markdown to the plain-text app, as it did for Omawrite.

## Success criteria

- Every file opens with its first line at the top of the view.
- The owner can reach any section of a long file from the keyboard, through the
  outline, without scrolling or using find.
- Words, lines, estimated tokens and sections are readable at the top of the window
  at a glance, and stay correct after an edit.
- In the reading view, headings, lists and paragraphs read as a finished page, with
  no Markdown marks showing and no lines broken mid-sentence; tables read as grids
  and code blocks as boxes.
- Opening a Markdown file from the file manager or a terminal opens it in Leaf, and
  Omawrite can be uninstalled without losing any feature Leaf inherited from it.
- A file opened, read and closed without edits is byte-for-byte unchanged on disk.

## Open questions

- **How are true table grids and code boxes drawn?** Resolved: in the reading view
  (approach D), not over the editable text.
- **What else in LLM files needs formatting** -- nested lists, block quotes, images,
  diagrams? Not discussed; resolved by reading real files in Leaf.
