---
type: scope-change
description: Scope changes for find-your-way.
---

# Scope changes: find-your-way

Append-only. Newest last.

## 2026-09-27 -- Enter in the outline keeps focus there

- Discovered in: 2-3
- What changed: pressing Enter on an outline entry jumps to the heading but keeps
  focus in the outline. Only Ctrl+J or a mouse click in the text or on an entry takes
  the owner back to the text. Escape still returns to the text, as specified.
- Why: trying it, the owner wanted to step through sections from the outline without
  being sent back to the text after every jump.
- Affects: spec *Jumping to a section* (the Enter line); slice 2-3. No other slice,
  and the plan's lines still hold.
- Agreed: yes, by the owner
- Applied to the plan: yes (no plan line changed; the spec's Behaviour was updated)
