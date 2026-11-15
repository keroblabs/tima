# Tima — Bug Tracking Methodology

**Document status:** Proposed working draft — awaiting approval  
**Applies to:** Tima 6.x engineering and review of the Tima 4.2 baseline  
**Location:** `docs/bugs/bug-tracking-methodology.md`

## 1. Purpose and guiding principles

This document defines how we identify, describe, discuss, fix, review, and close defects and engineering improvements in Tima. The tracker is part of the repository and is the durable record of decisions, not merely a to-do list.

- **Review before change.** A finding is recorded before implementation, and its proposed remedy is not automatically approved.
- **Preserve history.** The Tima 4.2 snapshot is a reference; fixes belong on an explicitly chosen Tima 6.x working branch. Never rewrite the baseline as part of bug resolution.
- **One issue, one identifiable concern.** Related issues may be linked, but distinct causes or acceptance criteria should not be hidden in a single ticket.
- **Keep records consistent.** Every category file has the same header, summary, and ticket structure.
- **Record decisions and rework.** Notes form a chronological, append-only engineering history, including discarded proposals, review feedback, and reopened issues.
- **Distinguish evidence from hypotheses.** A suspected defect is permissible, but must be labelled as unverified until confirmed by analysis or tests.
- **No silent closure.** A resolved issue must say what changed, how it was verified, and who approved the conclusion.

## 2. Files and issue ownership

Each file under `docs/bugs/` covers one logical subsystem or engineering area, not necessarily one source directory.

Initial files, to be created **after this methodology is approved**:

| File | Scope | ID prefix |
| --- | --- | --- |
| `board-specific.md` | CPU/board portability, interrupt primitives, critical sections, and their contracts, including relevant declarations in `system/types.h` | `TIMA-BRD` |
| `framework.md` | Reusable framework components, initially `framework/buffer.c/.h` and `framework/pipe.c/.h` | `TIMA-FWK` |

Further files may be introduced when warranted (for example, `system.md` or `interface.md`), using a unique agreed prefix. Place an issue where its **primary corrective responsibility** lies; cross-reference other affected files or tickets in Details or Notes. Do not duplicate the same issue in two files.

## 3. Issue identification

IDs have the form `TIMA-<AREA>-NNNN`, for example `TIMA-FWK-0001` and `TIMA-BRD-0001`.

- Numbers start at `0001` and increase within each category file.
- An ID is permanent. Never reuse, renumber, or recycle it, even if the issue is rejected, moved, or closed.
- The ID remains stable if the title, description, or owning category changes; leave a cross-reference in the old file if an issue must move.
- Commit messages, review notes, tests, and related tickets should reference the ID.

## 4. Status model

| Status | Meaning |
| --- | --- |
| `OPEN` | Recorded for assessment. The cause or proposed fix may still be unverified. |
| `IN_PROGRESS` | The agreed investigation or implementation is actively being carried out. |
| `REVIEW` | A proposed implementation or conclusion is ready for examination and testing evidence is recorded. |
| `RESOLVED` | The outcome has been reviewed and accepted, with the resolution recorded; integration or final housekeeping may remain. |
| `CLOSED` | The accepted outcome is integrated into the designated working branch, or a documented no-change disposition has been completed. |
| `DEFERRED` | Deliberately postponed, with a reason and a condition for reconsideration. |

Normal flow: `OPEN → IN_PROGRESS → REVIEW → RESOLVED → CLOSED`.

Investigation may return from `IN_PROGRESS` or `REVIEW` to `OPEN` or `IN_PROGRESS` when the hypothesis or proposed change needs revision. A `DEFERRED`, `RESOLVED`, or `CLOSED` issue may be reopened by returning it to `OPEN`, with an explicit dated note explaining why. No special “reopened” status is needed: the note preserves the history.

Issues may be closed without code changes only when the disposition (for example, duplicate, not reproducible, or intentionally retained behavior) is documented and approved. Record the rationale in Resolution Description; do not delete the ticket.

## 5. Required structure of every category file

Every category file uses exactly this top-level order:

1. **Header:** title, scope, ID prefix, and a brief pointer to this methodology. Include a short statement that the summary is the current index and the issue records below are authoritative.
2. **Summary:** a Markdown table with exactly three columns — `ID`, `Status`, and `Title`. Each issue occupies one row, with a short, single-line title. Sort by issue ID. The table is updated whenever status or title changes.
3. **Issues:** complete tickets, in ascending ID order. Do not create an issue entry without a corresponding summary row, or vice versa.

Do not use the summary to hold technical explanations or discussions. Those belong in the corresponding ticket.

### Category file skeleton (illustrative placeholders only; not real tickets)

```markdown
# Tima Bugs — <Area>

Scope: <logical scope>  
ID prefix: `TIMA-XXX`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and leave Resolution Description as `Pending` until an
outcome has been reviewed. Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `TIMA-XXX-0001` | `OPEN` | <Short single-line title> |

## Issues

### TIMA-XXX-0001

**ID:** `TIMA-XXX-0001`  
**Status:** `OPEN`  
**Title:** <Short descriptive title>

**Description**  
<What is wrong, unclear, unsafe, obsolete, or worth improving.>

**Details**  
<Files/functions, current behavior, expected behavior, evidence, reproduction,
impact, assumptions, and dependencies. Mark unverified assertions explicitly.>

**Notes**
- **YYYY-MM-DD — Discovery:** <Source of finding and initial observations.>

**Proposed Change**  
<Recommended direction, constraints, and alternatives; or 'To be determined'.
This is a proposal until reviewed and agreed.>

**Resolution Description**  
Pending.
```

The required ticket fields must always appear, in this order: **ID, Status, Title, Description, Details, Notes, Proposed Change, Resolution Description**. Additional technical subheadings may be used *within* Details or Notes, but should not replace these fields.

## 6. Writing effective issues

**Title:** name the failure or improvement, not a vague activity such as “review buffer”. Keep it short enough for a one-line summary.

**Description:** state the observable problem, risk, inconsistency, or improvement opportunity. Say why it matters; do not assume a proposed fix is the cause.

**Details:** give relative source paths, relevant function names, version/branch inspected, assumptions, expected versus actual behavior, triggering conditions, ISR/main-context considerations where applicable, and tests or code evidence. Distinguish *confirmed*, *suspected*, and *not yet tested*. Optional descriptors such as defect / safety / cleanup / improvement can live here if useful; they are not mandatory extra fields.

**Proposed Change:** describe a possible remedy and its consequences, not an irreversible instruction. Include open questions and alternatives where appropriate. If no remedy is yet justified, write `To be determined after review`.

**Resolution Description:** initially `Pending`. Once accepted, capture the final disposition, files changed, relevant commits/PRs, verification results and limitations, and the reason for any deviation from the original proposal. Do not claim tests were run unless they were.

## 7. Notes: the chronological engineering log

Notes are **append-only** and contain dated entries. Add a note whenever an issue is materially investigated, its description/proposal is corrected, code is changed, a test is run, a review finds a problem, rework occurs, its status changes, or a decision is made.

Suggested entry syntax:

```markdown
- **YYYY-MM-DD — <Activity>:** <Observation, decision, evidence, or change>
```

Useful activity labels include `Discovery`, `Investigation`, `Decision`, `Implementation`, `Test`, `Code Review`, `Rework`, `Status Change`, and `Reopened`.

Keep current Description, Details, and Proposed Change accurate; when editing their substance, append a note explaining what changed and why. Do not silently erase an earlier interpretation or unsuccessful approach. Link to commits and tests where applicable. Record reviewer approval explicitly; do not infer it from silence.

## 8. Review and resolution workflow

1. **Discover:** inspect the 4.2 code and register an `OPEN` ticket with evidence or a clearly marked hypothesis. Update the summary row.
2. **Triage together:** decide whether it is a real defect, a safety or portability risk, historical residue, or an improvement worth making for Tima 6.0. Clarify expected behavior and scope; log the decision.
3. **Agree on a proposed change:** examine alternatives and dependencies, particularly public API, ownership, nonblocking behavior, interrupt masking, allocator use, and affected boards. If the change is not agreed, keep the issue open or defer it.
4. **Implement:** work only on the designated Tima 6.x branch; make a focused change and reference ticket IDs in commits. Do not modify the archived 4.2 baseline.
5. **Verify:** add or run relevant native tests, build checks, static review, and board/ISR-specific checks as appropriate. Record exactly what passed, failed, or could not be tested. A build alone is not proof of interrupt safety.
6. **Review:** set `REVIEW` and examine implementation, API behavior, edge cases, ownership, interrupt-state restoration, error paths, and evidence. Log review findings and rework.
7. **Accept and close:** after explicit approval, set `RESOLVED` with a complete resolution; set `CLOSED` once integrated into the designated branch or the approved no-change disposition is complete. Update the summary at each transition.

If a test later exposes a regression, reopen the original issue when it concerns the same root cause; otherwise open a new issue and cross-reference it.

## 9. Consistency and scope controls

- Any change to status or title must be reflected in both the summary row and the ticket fields in the same edit.
- There is one source of truth for each issue; links between issues express dependency, duplication, or shared investigation.
- Do not batch unrelated defects into one ticket just because they occur in the same file.
- Keep defect corrections separate from discretionary API redesign when their acceptance criteria differ.
- Do not mark findings as fixed merely because a patch exists; verify the behavior and record review.
- Repository changes are separate from discussion: a proposed ticket or fix is not automatically approved because it was drafted.
- Tima Desktop is out of scope for Tima 6.x unless explicitly revisited.

## 10. Initial application of this methodology

After approval, create `docs/bugs/board-specific.md` and `docs/bugs/framework.md` using the category-file template. Then conduct the first two reviews:

- **Board-specific:** `system/types.h`, the `DI()` / `EI()` contract, and the board implementations of interrupt control.
- **Framework:** `framework/buffer.c/.h`, followed by `framework/pipe.c/.h`.

The initial reviews will create actual tickets with concrete evidence. This methodology document itself does **not** open or number any bugs.