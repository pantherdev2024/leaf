---
type: scope-change
description: Scope changes for make-it-leaf.
---

# Scope changes: make-it-leaf

Append-only. Newest last.

## 2026-09-27 -- The desktop's open command is left as it was

- Discovered in: 3-1
- What changed: opening a `.md` file with the `xdg-open` command still hands it to
  Neovim. Leaf is the Markdown default for everything that goes by file name,
  including the owner's file manager.
- Why: `xdg-open` guesses a file's type from its contents unless an extra package is
  installed; Markdown reads as plain text, which Omarchy sends to Neovim. Omawrite had
  the same gap. The owner chose not to install the package.
- Affects: spec *Default app for Markdown*, *Not in scope* and *Verification*; plan
  phase 3 done-when.
- Agreed: yes, by the owner
- Applied to the plan: yes

## 2026-09-27 -- Omawrite stays installed for now

- Discovered in: 3-1
- What changed: the check that everything still works with Omawrite uninstalled was
  not performed. The owner keeps Omawrite installed until the Leaf work is further
  along.
- Why: the owner's choice. Nothing in Leaf depends on Omawrite: the Markdown default
  names Leaf, and Super+Shift+W no longer points at Omawrite.
- Affects: spec *Verification*; plan phase 3 done-when.
- Agreed: yes, by the owner
- Applied to the plan: yes
