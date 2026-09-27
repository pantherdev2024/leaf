---
type: spec
description: Leaf gets its own name, icon and package, becomes the owner's default Markdown app, and opens every file at its first line.
status: approved
---

# Spec: make-it-leaf

## Goal

Turn the fork into Leaf as far as the owner and the system can see, and fix the
annoyance that costs the most every day. Until Leaf has its own identity and is the
default for Markdown files, every file still opens in Omawrite, so nothing else built
for Leaf reaches the owner. Opening at the bottom affects every file read, and it is
a small fix. Doing both first means later cycles never deal with the old name. The
cycle also points Omarchy's writing-app key, Super+Shift+W, at Leaf, so that
uninstalling Omawrite leaves no dead key behind.

## Design fit

Uses from `docs/design.md`: the **Document** concept, the **Open and read** flow
(the text from its first line), and the constraints that Leaf has its own name, icon
and package, replaces Omawrite and is the default app for Markdown files.

Adds to the design, for reconciling at close:

- Reloading a document after an outside change also starts at the first line.
- Super+Shift+W, Omarchy's key for its writing app, opens Leaf. This is set in the
  owner's own desktop configuration, not shipped by Leaf.
- Installing Leaf with the repository's install script makes it the default for
  Markdown, and only for Markdown.

Nothing here contradicts the design or the architecture. The architecture already
says Leaf keeps its own settings and recovery data and does not carry Omawrite's over.

## Behaviour

### Leaf's identity

- The program is called `leaf`. It is started as `leaf`, or `leaf <file>` to open a
  file.
- The window title is the file name followed by "- Leaf", with a leading `*` when
  there are unsaved edits, as today. A new document with no file is titled
  "Untitled.md - Leaf".
- The warnings about a file changing or being removed on disk say "outside Leaf".
- The app launcher lists it as **Leaf**, described as a Markdown viewer, with Leaf's
  icon.
- The icon is new: a simple leaf shape in the same flat style as Omawrite's -- a dark
  rounded square with a light figure on it -- so it sits comfortably among the other
  Omarchy icons.
- Leaf keeps its settings (window size and position, last save folder) and its crash
  recovery in its own place. It starts with default settings the first time; nothing
  of Omawrite's is read or changed.
- The package is called `leaf`, starts at version 0.1.0, names the owner as
  maintainer and the fork's repository as its home. It installs beside Omawrite
  without any clash, so either can be removed independently.
- The build produces `leaf` and the test program is named after Leaf. Nothing a user
  sees -- in the app, the launcher, the package or a message -- says Omawrite.
- Internal names that carry the old identity are renamed too: the application's
  system identity, the project and package files, the test class and files. After
  this cycle the only mentions of Omawrite are the documents' account of where Leaf
  came from and how it differs.

### Default app for Markdown

- The repository's install script already builds Leaf and installs it as a package.
  It now also, after installing, makes Leaf the owner's default application for
  Markdown files. Installing the package by other means does not set the default;
  the setting belongs to the owner, not to the package. Opening a `.md` file from the file manager, or with the desktop's
  "open" command, opens it in Leaf.
- Running the install script again leaves it set the same way; it does no harm to
  repeat.
- Leaf still offers itself for plain text files under "Open with", as Omawrite did,
  but does not become their default.
- If the default cannot be set, the install script says so plainly. Leaf stays
  installed and can be set as default by hand.
- The default is recorded explicitly in the owner's own settings, rather than left to
  whichever installed app claims Markdown files. Normal Omarchy updates do not touch
  those settings, so it survives them and survives Omawrite being reinstalled.

### Super+Shift+W

- Super+Shift+W opens a new Leaf window instead of Omawrite.
- The change is made once, during this cycle and with the owner's go-ahead, in the
  owner's personal key bindings -- not by the install script. It replaces Omarchy's
  default binding for that key rather than adding a second one. It survives normal
  Omarchy updates. Running Omarchy's own Hyprland reset would remove it; the reset
  keeps a backup.

### Opening at the top

- Whenever a document is loaded into a window, the view shows its first line at the
  top and the text cursor sits at the very start of the document. This holds for:
  - a file given on the command line or opened from the file manager;
  - a file chosen with the open dialog, including into a window that was scrolled
    down in another document;
  - reloading after the file changed on disk;
  - recovering unsaved text after a crash.
- Nothing else about scrolling changes. Once the document is open, the view still
  follows the cursor while typing, and the wheel, keys and scroll bar behave as today.
- Opening a document does not change the file on disk.

## Not in scope

- Keeping the reading position when reloading after an outside change. Reload starts
  at the top; keeping the place may come later.
- The prompt that appears on every outside change, even with no local edits. Noted
  for a later change.
- Preserving Windows line endings on save, an accepted gap in the architecture.
- Carrying over Omawrite's settings or recovery data.
- Removing Omawrite. The owner uninstalls it when ready.
- Making Leaf the default for plain text files.
- Publishing Leaf to any package repository, or updating it automatically.
- The outline, the stat cards and table or code formatting, which are later cycles.

## Verification

- The test suite passes, including new checks that loading a long document -- by
  opening, reloading and recovery -- leaves its first line at the top of the view and
  the cursor at the start.
- A search of the repository's code, build, test and packaging files finds no
  "Omawrite". It appears only in the README and the documents under `docs/` and
  `aidlc/`, which describe where Leaf came from.
- After running the install script:
  - `leaf` is installed as a package, and starting it opens a window titled with
    "- Leaf".
  - The desktop reports Leaf as the default application for Markdown files, and the
    default is written in the owner's own settings file.
  - Opening a long `.md` file from the file manager opens it in Leaf, at its first
    line.
  - The launcher shows Leaf with its new icon.
  - Super+Shift+W opens a Leaf window.
- With Omawrite uninstalled, all of the above still works.
- Scrolling by wheel, keys and scroll bar, and the view following the cursor while
  typing, feel as before when tried by hand, and the existing tests still pass.
- A file opened and closed in Leaf without edits is byte-for-byte unchanged.

## Open questions

- The icon's exact look. Resolved by the owner approving it when it is drawn.
