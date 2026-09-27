---
type: scope-change
description: Scope changes for reading-view.
---

# Scope changes: reading-view

Append-only. Newest last.

## 2026-09-27 -- The display copy closes void HTML tags

- Discovered in: 1-1
- What changed: besides leaving out front matter and tag lines, the display copy
  closes an HTML void tag written without a closing slash -- `<br>` becomes `<br/>`,
  and likewise `<hr>`, `<img ...>`, `<input>` and the other void elements, in any
  case -- outside code blocks and inline code. The file and the editor are untouched.
- Why: Qt's importer drops everything after an unclosed void tag, to the end of the
  file. `<br>` inside table cells is common in LLM output. The closed forms render.
- Affects: spec *What the reading view shows* and *Verification*; architecture
  "Why the display copy may leave lines out" and the Reading renderer component;
  slice 1-1 (one more rule and its test). No plan line changes.
- Agreed: yes, by the owner
- Applied to the plan: yes (no plan line changed; the spec and architecture were
  updated)
