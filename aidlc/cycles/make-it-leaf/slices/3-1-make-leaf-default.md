---
type: slice
description: The install script makes Leaf the owner's Markdown default, and Super+Shift+W opens Leaf.
status: done
effort: light
---

# Slice 3-1: make-leaf-default

## Context

Spec, *Default app for Markdown* and *Super+Shift+W*: installing with the install
script makes Leaf the owner's default for Markdown, harmlessly on repeat, reporting
plainly if it cannot; plain text is not taken over; and Super+Shift+W opens Leaf,
changed once in the owner's personal key bindings with their go-ahead. Phase 3,
*Leaf is the default*; its done-when is the spec's whole verification.

## Intent

`bin/install` builds Leaf and hands over to the package builder, which installs it.
Omawrite is the Markdown default today only because nothing else claims Markdown, so
Leaf must be recorded as the default explicitly in the owner's own settings. The
script now installs, then records Leaf as the default for both Markdown types, then
checks the result and says what happened; a failure to set the default does not undo
the install. The key binding is replaced in the owner's personal Hyprland bindings,
unbinding Omarchy's default before binding Leaf, following Omarchy's rules for
personal overrides.

## Done when

After the owner runs `bin/install`, the desktop reports Leaf as the Markdown default,
written in the owner's own settings file; a `.md` file opened from the file manager
opens in Leaf at its first line; the launcher shows Leaf with its icon; and
Super+Shift+W opens Leaf. With Omawrite uninstalled all of this still works, and a
file opened and closed without edits is byte-for-byte unchanged.

## Tasks

- [x] Make the install script set and check the Markdown default after installing,
  and report the outcome plainly -- files: `bin/install`. Why: the spec's default-app
  behaviour, repeatable on every install.
- [x] Say in the README that installing also makes Leaf the Markdown default -- files:
  `README.md`. Why: the command's effect changed.
- [x] The owner runs `bin/install`; confirm the package, icon, desktop entry and
  default -- files: none in the repo. Why: installing needs the owner's password.
- [x] With the owner's go-ahead, unbind Omarchy's Super+Shift+W and bind it to Leaf,
  then validate the Hyprland config -- files: `~/.config/hypr/bindings.lua`, outside
  the repo, backed up first. Why: the spec's key behaviour.
- [x] Run the spec's verification with the owner, including after Omawrite is
  uninstalled -- files: none. Why: the phase's done-when.

## Tests

No automated test: the install script's effect is on the owner's real desktop
settings and needs sudo. Verified directly by querying the desktop's default for
Markdown and reading the owner's settings file after the install, and by the owner
opening a file from the file manager and pressing the key. The existing suite must
still pass.

## Documents this could invalidate

- `README.md` -- what `bin/install` does.
- `docs/design.md` -- the spec's three additions, reconciled at cycle close, not here.

## Notes for the next slice

- Installed and confirmed: package `leaf` 0.1.0 at `/usr/bin/leaf`, with its icon and
  launcher entry; Leaf is recorded in `~/.config/mimeapps.list` for both Markdown
  types; plain text still goes to Neovim. The owner confirmed Super+Shift+W opens
  Leaf and the launcher shows Leaf with its icon.
- Deviation, agreed with the owner: the `xdg-open` command still hands `.md` files to
  Neovim. It guesses types from file contents (Markdown looks like plain text, which
  Omarchy sends to Neovim) unless `perl-file-mimeinfo` is installed. Omawrite had the
  same gap. The owner's file manager (flea) uses `gio open`, which goes by the file
  name, and opens `.md` files in Leaf at the first line. The spec's "or with the
  desktop's open command" is not met; reconcile at close.
- Skipped by the owner's choice: the check with Omawrite uninstalled. The owner keeps
  Omawrite until the Leaf work is further along. Nothing in Leaf depends on it: the
  default is recorded by name and the key no longer points at Omawrite.
- Seen once, not reproduced: the first launch through the file manager marked an
  unedited file "Unsaved" and wrote a recovery copy identical to the file. Six further
  launches, direct and through the file manager, opened clean. Likely keystrokes
  landing as the window took focus. Watch for it.
- Key binding: `~/.config/hypr/bindings.lua` was backed up, then Omarchy's
  Super+Shift+W was unbound and bound to `leaf`; `hyprctl configerrors` was clean.
  Simulated key presses do not reach Hyprland shortcuts; to test a binding's command
  run `hyprctl dispatch 'hl.dsp.exec_cmd("uwsm-app -- leaf")'`.
- Review nit not taken: the install script checks only `text/markdown` after setting
  both types.
