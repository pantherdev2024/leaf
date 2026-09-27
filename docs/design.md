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
- **Formatted block** -- a table or a code block, drawn so it reads as a table or a
  code box rather than as the raw Markdown behind it. Where the cursor is, the raw
  text shows so it can be edited.

## Core flows

**Open and read.** The owner opens a Markdown file, from the file manager, from a
terminal or from inside Leaf. The window shows the stats across the top, the outline
on the left and the text from its first line. Reloading after the file changes on
disk, or recovering after a crash, also starts from the first line. They read by
scrolling, and the outline follows, marking the section they are in.

**Jump to a section.** Without touching the mouse, the owner presses Ctrl+J to move
into the outline, steps through the headings with the arrow keys, and presses Enter.
The text jumps so that heading is at the top, and the outline keeps focus so they can
jump again. Ctrl+J, or Escape, takes them back to the text. Clicking an entry jumps
and puts them in the text.

**Make a small edit.** The text is always editable. The owner clicks into it and types,
as in Omawrite, and saves. The stats and the outline update as they type. Everything
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

## Chosen approach

Approach A. The problems that cost the most every day -- opening at the bottom, no
way to jump between sections, stats hidden in a corner -- are all quick to fix on the
existing editor. Editing is rare, so Obsidian-grade editing of tables matters less
than it would to a writer. Formatting can improve in steps without giving up the
tested work inherited from Omawrite, and approach B stays open if the drawn tables
are not good enough.

## Not in scope

- A read-only mode or a separate edit mode. The text is always editable, as in
  Omawrite.
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
- In the first step, table columns line up and code blocks are shaded apart from the
  prose. When the later formatting cycle is done, tables read as grids and code
  blocks as boxes.
- Opening a Markdown file from the file manager or a terminal opens it in Leaf, and
  Omawrite can be uninstalled without losing any feature Leaf inherited from it.
- A file opened, read and closed without edits is byte-for-byte unchanged on disk.

## Open questions

- **How are true table grids and code boxes drawn over the text?** Resolved in the
  later formatting cycle, on real LLM files.
- **What else in LLM files needs formatting** -- nested lists, block quotes, images,
  diagrams? Not discussed; resolved by reading real files in Leaf.
