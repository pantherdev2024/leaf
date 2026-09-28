---
type: slice
description: Rename every Omawrite name in code, build, test and packaging to Leaf.
status: done
effort: light
---

# Slice 1-1: rename-to-leaf

## Context

Spec, *Leaf's identity*: the program is `leaf`, the title ends "- Leaf", messages say
"outside Leaf", the launcher lists Leaf as a Markdown viewer, the package is `leaf`
0.1.0 with the owner as maintainer, Leaf keeps its own settings, and internal names
carrying the old identity are renamed. Phase 1, *It's called Leaf*; this slice leaves
behind the `leaf` names the icon and install slices use.

## Intent

Replace the Omawrite identity everywhere outside the documents. The application
identity is set in the entry point as organisation "Omacom" and application
"omawrite", which places settings under `~/.config/Omacom/` and recovery under
`~/.local/share/Omacom/omawrite/`. Naming both the organisation and the application
`leaf` moves them to `~/.config/leaf/leaf.conf` and `~/.local/share/leaf/leaf/`.
The icon file is renamed with the rest but keeps Omawrite's artwork until slice 1-2
draws the new one.

## Done when

`bin/build` produces `build/leaf`; the window title of `build/leaf` ends in "- Leaf";
`bin/test` passes; `git grep -i omawrite` finds nothing outside `README.md`,
`AGENTS.md`, `docs/` and `aidlc/`.

## Tasks

- [x] Rename the application identity, startup error message and window title --
  files: `src/main.cpp`, `src/Main.qml`. Why: the program name, settings location,
  desktop file name, icon name and title a user sees.
- [x] Reword the outside-change messages to "outside Leaf" -- files:
  `src/ExternalChangeDialog.qml`. Why: user-visible text.
- [x] Rename the project file and build target to `leaf`, and the build script's
  messages and project path -- files: `omawrite.pro` to `leaf.pro`, `bin/build`. Why:
  the build must produce `build/leaf`.
- [x] Rename the test program, file and class -- files: `tests/tst_omawrite.cpp` to
  `tests/tst_leaf.cpp`, `tests/tests.pro`, `bin/test`. Why: internal names carrying
  the old identity.
- [x] Rename the package, its desktop entry, install hook and icon file, and update
  maintainer, version, description and home -- files: `pkgbuild/PKGBUILD`,
  `pkgbuild/omawrite.desktop` to `pkgbuild/leaf.desktop`, `pkgbuild/omawrite.install`
  to `pkgbuild/leaf.install`, `pkgbuild/omawrite.svg` to `pkgbuild/leaf.svg`. Why: the
  package is `leaf`, installs `/usr/bin/leaf`, and must not clash with Omawrite's
  files.
- [x] Clean the old build outputs' names from `.gitignore` if any -- files:
  `.gitignore`. Why: keep ignores matching the new names.

## Tests

The existing suite, renamed, runs against the real backend and interface headless and
must pass unchanged in behaviour. No new test: the rename changes names, not
behaviour, and the done-when checks (build output name, window title, repository
search) are direct observations.

## Documents this could invalidate

- `README.md` -- the build output path and the "until the rename is done" note.
- `AGENTS.md` -- the lines naming `omawrite.pro`, `build/omawrite`, and the note that
  the code still says `omawrite`.

## Notes for the next slice

- `.gitignore` held no Omawrite names; nothing to change there.
- The organisation is named `leaf`, not left empty: Qt files settings with no
  organisation under "Unknown Organization" (caught in review). Settings go to
  `~/.config/leaf/leaf.conf` and recovery to `~/.local/share/leaf/leaf/`. Confirmed by
  launching `build/leaf` and closing it: window class `leaf`, title
  "README.md - Leaf", settings written to that file.
- To close a test window cleanly from a script on this Hyprland, use
  `hyprctl dispatch 'hl.dsp.window.close({ window = "pid:<pid>" })'`; the old
  `closewindow` syntax fails. Killing the process skips the settings write.
- `pkgbuild/leaf.svg` still holds Omawrite's artwork; 1-2 replaces it. The package,
  desktop entry and icon name are all `leaf`, so 1-2 only changes the file's contents.
- The maintainer line carries the owner's name only, no email address.
- The desktop entry's categories changed from WordProcessor to Viewer, to match
  "Markdown viewer".
