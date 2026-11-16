# Tima Bugs — Framework

Scope: Reusable framework components and their APIs, including interrupt safety, storage ownership, correctness, and portability.  
ID prefix: `TIMA-FWK`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative. Update both the summary and ticket when status or title changes. Keep Notes as a dated, append-only engineering history and track implementation commits and their review statuses in each ticket. Proposed changes are subject to review; an `OPEN` issue is not an approved implementation request.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `TIMA-FWK-0001` | `CLOSED` | Full-buffer rejection exits with interrupts disabled |
| `TIMA-FWK-0002` | `CLOSED` | No-rollover flag fails when combined with other flags |
| `TIMA-FWK-0003` | `CLOSED` | Read-only mode does not prevent writes |
| `TIMA-FWK-0004` | `CLOSED` | Reported available capacity disagrees with writable capacity |
| `TIMA-FWK-0005` | `CLOSED` | Resize can invalidate cursors and truncate the stored size |
| `TIMA-FWK-0006` | `CLOSED` | Allocation and deallocation occur while interrupts are disabled |
| `TIMA-FWK-0007` | `CLOSED` | Replacing storage breaks heap ownership and buffer invariants |
| `TIMA-FWK-0008` | `REVIEW` | Single-byte read cannot distinguish empty from a zero byte |
| `TIMA-FWK-0009` | `REVIEW` | Bulk read checks emptiness outside its protected operation |
| `TIMA-FWK-0010` | `REVIEW` | Copy-from-buffer ignores unread range and circular order |
| `TIMA-FWK-0011` | `REVIEW` | Mode and offset setters bypass the critical-section contract |
| `TIMA-FWK-0012` | `REVIEW` | Returned storage pointers have no stable lifetime guarantee |
| `TIMA-FWK-0013` | `REVIEW` | Reference counter can overflow and misrepresent ownership |
| `TIMA-FWK-0014` | `CLOSED` | Zero sizes and invalid backing storage are not rejected |
| `TIMA-FWK-0015` | `CLOSED` | fill_size metadata is left uninitialized and unused |

## Issues

### TIMA-FWK-0001

**ID:** `TIMA-FWK-0001`  
**Status:** `CLOSED`  
**Title:** Full-buffer rejection exits with interrupts disabled

**Description**  
`buffer_write()` may return `FALSE` after entering a critical section without calling `EI()`. On affected boards, this can leave interrupt masking active after a normal write failure.

**Details**  
**Historical finding, confirmed by static review:** In the original implementation, `framework/buffer.c`, `buffer_write()` called `DI()` before a `MASK_NO_ROLL_OVER` / `pointer_in == size - 1` check whose failure branch returned immediately. The successful path called `EI()`, but the rejection path did not. This was reachable for a `buffer_init()` instance followed by `buffer_set_no_roll_over(TRUE)` (assuming a valid positive size). The exact device-level consequence depended on the board's `DI`/`EI` implementation.

**Confirmed during investigation:** `Boards/Nucleo_446/library/cpu_driver.c` implements nested `DI()`/`EI()` calls with an interrupt counter. The missing `EI()` leaves that counter elevated, so later matching enables cannot restore the expected state. The repository does not provide a portable interrupt-state save/restore primitive for all board ports. The focused correction therefore balances the critical section through one cleanup path and does not change the existing board-level contract.

**Current implementation after TIMA-FWK-0002:** The no-rollover mode and its full-buffer rejection branch have been removed. The former early return can no longer occur; valid writes use circular-buffer behavior. The host tests now verify wraparound writes and nested critical-section balance. They do not execute on target hardware.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Investigation:** Confirmed the unbalanced return path and inspected the Nucleo interrupt counter implementation. The host test will verify the existing nested `DI`/`EI` pairing contract; board hardware behavior remains untested.
- **2026-11-15 — Decision:** At the user's direction to fix TIMA-FWK-0001, proceed with a focused single-cleanup-path correction that calls `EI()` after either a successful write or a full-buffer rejection. Narrowed the original entry-state-restoration proposal because the repository has no portable state-save primitive across its board ports. Set status to `IN_PROGRESS`.
- **2026-11-15 — Implementation:** Changed `buffer_write()` to record rejection, skip data and cursor updates on that path, and reach the common `EI()` before returning.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed. It compiles the buffer implementation and host-side tests with `cc -std=c99 -Wall -Wextra -Werror`; both ordinary and nested critical-section cases passed.
- **2026-11-15 — Code Review:** Local review confirms both post-`DI()` outcomes reach one `EI()`, and rejection leaves the backing bytes and cursors unchanged. Hardware execution and independent review have not been performed; the change is ready for review.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording the implementation and passing test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-15 — Commit:** Created implementation commit `e9bf1ae8acc0276b566b1242d2c92dbe4e0fc62b` on `phase-1-improvements`. Its review status is `UNREVIEWED`; no independent review has occurred.
- **2026-11-15 — Rework:** The no-rollover rejection path was removed as part of TIMA-FWK-0002. The current shared buffer tests cover circular writes and critical-section balance; the earlier rejection-specific test evidence applies to commit `e9bf1ae`, before that mode was removed.
- **2026-11-15 — Related Commit:** TIMA-FWK-0002 implementation commit `cf4083c4f16e8d04cf9c0a77d62ab082a46de255` removes the former rejection path. It is tracked here because that deletion also removes the condition described by this issue; the commit is `UNREVIEWED`.

- **2026-11-15 — External Review:** Project owner reviewed the submitted fix and recorded a Gerrit-style `+1` (positive review). This feedback is distinct from the independent review below and does not by itself mean code approval or closure.
- **2026-11-15 — Independent Code Review:** Reviewed [`e9bf1ae`](https://github.com/keroblabs/tima/commit/e9bf1ae8acc0276b566b1242d2c92dbe4e0fc62b) against its parent and inspected `buffer_write()`, the test runner, host stubs, and `test_buffer.c`. **Approved** the focused correction: the function has one post-`DI()` exit via `EI()`, returns `FALSE` on the existing rejection condition, and does not modify bytes or cursors on rejection. The submitted tests cover ordinary and nested critical-section pairing. The reviewer verified the code and test logic but could not rerun the remote suite in this environment; the reported test pass comes from the submission. No on-target interrupt test has been recorded.
- **2026-11-15 — Scope:** The remaining rollover-flag behavior and general interrupt-state/ISR contract are outside this fix and remain subject to their separate issues. This approval does not certify the whole buffer implementation as interrupt-safe.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` following accepted review, then `RESOLVED` → `CLOSED` because the approved implementation commit is already present on the designated `phase-1-improvements` branch. Commit review status updated to `APPROVED`.

**Proposed Change**  
The initial focused fix routed the old rejection and success paths through one cleanup path. Since TIMA-FWK-0002 removed the non-rolling mode, that rejection path no longer exists. Review the removal and current circular-write critical-section behavior together before resolving this issue.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`e9bf1ae`](https://github.com/keroblabs/tima/commit/e9bf1ae8acc0276b566b1242d2c92dbe4e0fc62b) | `APPROVED` | Balance the critical section after a full-buffer write rejection and add host tests. |
| `phase-1-improvements` | [`cf4083c`](https://github.com/keroblabs/tima/commit/cf4083c4f16e8d04cf9c0a77d62ab082a46de255) | `UNREVIEWED` | Remove the no-rollover rejection path and verify circular writes. |

**Resolution Description**  
**Fixed and approved.** The committed change [`e9bf1ae`](https://github.com/keroblabs/tima/commit/e9bf1ae8acc0276b566b1242d2c92dbe4e0fc62b) in `framework/buffer.c` replaces the premature return after `DI()` with a result variable and common `EI()` exit. Rejection retains `FALSE` return semantics without modifying buffer contents or indices. The implementation also adds `tests/framework/test_buffer.c`, host stubs, and `tests/framework/run_buffer_tests.sh` for normal, rejected, and nested critical-section cases. The submission reports its host tests passed with `cc -std=c99 -Wall -Wextra -Werror`; independent static review found the targeted path correctly balanced, but the test suite was not independently rerun and target-board behavior remains unverified. The project owner provided a Gerrit-style `+1`; independent code review approved the implementation. The fix is committed to the designated working branch, so this ticket is `CLOSED`. Other buffer/board interrupt issues remain open.

---

### TIMA-FWK-0002

**ID:** `TIMA-FWK-0002`  
**Status:** `CLOSED`  
**Title:** No-rollover flag fails when combined with other flags

**Description**  
Historically, the no-rollover behavior compared the entire flag mask rather than testing its no-rollover bit, allowing unrelated flags to disable the mode unintentionally. The mode has since been removed by decision.

**Details**  
**Historical finding, confirmed by static review:** `buffer_read()` and `buffer_write()` compared `buffer_data->mask == MASK_NO_ROLL_OVER` (or negated that expression). `buffer_create()` set both `MASK_ON_HEAP` and `MASK_NO_ROLL_OVER`; the combined mask did not equal the no-rollover bit, so created buffers followed the rollover path. The flags mixed behavior with ownership state.

**Decision and implementation:** The no-rollover feature has been removed. All buffers now use circular behavior: writes wrap and discard the oldest unread byte on overflow. `buffer_set_no_roll_over()`, its flag, and the mode-specific read/write branches have been removed from `framework/buffer.h` and `framework/buffer.c`. Tests in `tests/framework/test_buffer.c` cover stack- and heap-created buffers, writes across the end of storage, overwrite order, and critical-section balance.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Decision:** The user decided to remove no-rollover mode. All buffers will use circular behavior, including heap-created buffers. The API setter, flag, and separate linear-mode branches are in scope for removal.
- **2026-11-15 — Status Change:** Moved from `OPEN` to `IN_PROGRESS` to implement the agreed removal and verify circular-buffer behavior.
- **2026-11-15 — Implementation:** Removed the no-rollover setter, flag, and mode-specific read/write logic. Updated the host tests to cover wraparound, overwrite order, and nested critical-section behavior. The previous TIMA-FWK-0001 rejection path is no longer present.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`; stack- and heap-created circular buffers, oldest-entry overwrite, read order, and ordinary/nested critical-section balance passed.
- **2026-11-15 — Code Review:** Local review confirms the no-rollover API and flag are absent from maintained framework source and tests, and every valid write follows the circular path. The host suite passes; target hardware and independent review have not been performed.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and passing native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-15 — Commit:** Created implementation commit `cf4083c4f16e8d04cf9c0a77d62ab082a46de255` on `phase-1-improvements`. Its review status is `UNREVIEWED`; no independent review has occurred.

- **2026-11-15 — Project Review:** The project owner confirmed satisfaction with the removal of the no-rollover feature and authorized closure subject to independent review.
- **2026-11-15 — Commit Reconciliation:** The previously recorded commit SHA `cf4083c4f16e8d04cf9c0a77d62ab082a46de255` cannot be resolved by GitHub. Preserved its entry as `SUPERSEDED` for traceability; the actual implementation in the current branch history is [`cdc6da3`](https://github.com/keroblabs/tima/commit/cdc6da352b0ccda0b80414bfd58cebe244e10ee7). Review approval applies only to the verified SHA, not to the unresolvable historical reference.
- **2026-11-15 — Independent Code Review:** **Approved** the diff for `cdc6da3` and the resulting `framework/buffer.c/.h`: the no-rollover API and flag are removed; `buffer_create()` no longer sets that mode; `buffer_read()`/`buffer_write()` use the circular path; the old full-buffer rejection branch is gone. Inspected test cases for wraparound, overwrite order, heap-created buffers and nested critical-section balance. Reported host test pass is documented in the submission; the test suite was not independently rerun, and hardware behavior has not been verified.
- **2026-11-15 — Scope:** Zero-length buffers, reported capacity, resizing and general ISR correctness remain covered by other open tickets. The original defect description was changed to historical tense; no broader buffer correctness approval is implied.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` on accepted review, then `RESOLVED` → `CLOSED` because the approved source commit is already on `phase-1-improvements`. Verified implementation commit set to `APPROVED`.

**Proposed Change**  
Remove the non-rolling mode consistently: delete its public setter and flag, make read and write paths unconditionally circular, and ensure heap ownership remains independent of buffer behavior. Verify wraparound, overwrite order, and critical-section balance with native tests.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | `cf4083c` | `SUPERSEDED` | Historical recorded SHA; cannot resolve in current repository, not reviewed. |
| `phase-1-improvements` | [`cdc6da3`](https://github.com/keroblabs/tima/commit/cdc6da352b0ccda0b80414bfd58cebe244e10ee7) | `APPROVED` | Remove no-rollover setter/flag/branches; retain circular write behavior and update host tests. |

**Resolution Description**  
**Accepted and closed — feature removed.** [`cdc6da3`](https://github.com/keroblabs/tima/commit/cdc6da352b0ccda0b80414bfd58cebe244e10ee7) removes `MASK_NO_ROLL_OVER`, `buffer_set_no_roll_over()` and related conditional paths from `framework/buffer.c/.h`. Both initialized and heap-created buffers now use circular semantics, retaining overwrite-oldest behavior. Updated `tests/framework/test_buffer.c` covers circular wraps, overwrite order and nesting; the submission reports the native test runner passed with `cc -std=c99 -Wall -Wextra -Werror`. Independent static review approves the targeted change; tests were not independently rerun and no on-target test is recorded. The project owner confirmed the feature-removal decision and acceptance. The issue is `CLOSED` because the reviewed implementation is already on the working branch. Other buffer issues remain open.

---

### TIMA-FWK-0003

**ID:** `TIMA-FWK-0003`  
**Status:** `CLOSED`  
**Title:** Read-only mode does not prevent writes

**Description**  
Historically, `buffer_set_read_only(TRUE)` set a flag that write APIs ignored, so it did not enforce read-only behavior.

**Details**  
**Historical finding, confirmed by static review:** `buffer_set_read_only()` toggled `MASK_READ_ONLY`, but `buffer_write()` never tested the flag. `buffer_write_buffer()` and `buffer_write_buffer_ex()` delegated to `buffer_write()` and had no separate read-only behavior. No read-only contract was documented in `buffer.h`.

**Current implementation:** The setter declaration and implementation and `MASK_READ_ONLY` have been removed from `framework/buffer.h` and `framework/buffer.c`. The remaining `mask` field is used for heap ownership only. Direct and bulk write tests verify the supported write APIs remain usable.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Decision:** The user decided to remove read-only mode. Remove the public setter and its flag; keep buffer write APIs writable without a read-only option.
- **2026-11-15 — Status Change:** Moved from `OPEN` to `IN_PROGRESS` to remove the unused mode and verify direct and bulk write behavior.
- **2026-11-15 — Implementation:** Removed the public setter, implementation, and read-only flag. Added coverage for `buffer_write()`, `buffer_write_buffer()`, and `buffer_write_buffer_ex()`.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`; direct and bulk writes retained their expected byte order and cursor values.
- **2026-11-15 — Code Review:** Local review confirms the read-only setter and flag are absent from maintained framework source and tests, and direct and bulk writes remain functional. Independent review and target hardware execution have not been performed.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and passing native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-15 — Commit:** Created implementation commit `154ad57c45251393843b55893a5166bf5198694f` on `phase-1-improvements`. Its review status is `UNREVIEWED`; no independent review has occurred.

- **2026-11-15 — Project Review:** The project owner confirmed satisfaction with removal of the read-only feature and authorized closure subject to independent review.
- **2026-11-15 — Commit Reconciliation:** The previously recorded commit SHA `154ad57c45251393843b55893a5166bf5198694f` cannot be resolved by GitHub. Preserved its entry as `SUPERSEDED` for traceability; the actual implementation in the current branch history is [`e3d9ee9`](https://github.com/keroblabs/tima/commit/e3d9ee91d36aa15f43c3643d0b6e07b568c53e59). Approval applies only to the verified SHA.
- **2026-11-15 — Independent Code Review:** **Approved** `e3d9ee9` and its resulting `framework/buffer.c/.h`: the `MASK_READ_ONLY` constant and `buffer_set_read_only()` definition/declaration are removed; write paths remain supported; `MASK_ON_HEAP` is the only remaining buffer flag in the implementation. Reviewed newly added direct/bulk/buffer-to-buffer write checks. Reported host tests passed in the submission, but were not independently rerun, and no target-board test has been recorded.
- **2026-11-15 — Scope:** The `buffer_write_buffer_ex()` regression case copies a raw source prefix and does not establish correct logical circular-copy behavior; that separate question remains tracked by `TIMA-FWK-0010`. The removal intentionally changes the public API and does not establish that external callers have been migrated.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` upon accepted review, then `RESOLVED` → `CLOSED` because the approved implementation is already on `phase-1-improvements`. Verified implementation commit set to `APPROVED`.

**Proposed Change**  
Remove `buffer_set_read_only()` and `MASK_READ_ONLY` from the public buffer API and implementation. Verify that direct, bulk, and buffer-to-buffer writes continue to work with the remaining buffer API.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | `154ad57` | `SUPERSEDED` | Historical recorded SHA; cannot resolve in current repository, not reviewed. |
| `phase-1-improvements` | [`e3d9ee9`](https://github.com/keroblabs/tima/commit/e3d9ee91d36aa15f43c3643d0b6e07b568c53e59) | `APPROVED` | Remove read-only setter/flag; add direct and bulk-write regression checks. |

**Resolution Description**  
**Accepted and closed — feature removed.** [`e3d9ee9`](https://github.com/keroblabs/tima/commit/e3d9ee91d36aa15f43c3643d0b6e07b568c53e59) removes the public `buffer_set_read_only()` declaration/definition and `MASK_READ_ONLY` from `framework/buffer.h` and `framework/buffer.c`. Existing direct and bulk-write APIs remain available. The implementation adds host test coverage for those supported calls. The submission reports tests passed with `cc -std=c99 -Wall -Wextra -Werror`; independent code review approves the targeted removal, but did not rerun tests or validate a target build. The project owner accepted the feature removal. Ticket `CLOSED` on the designated working branch, with unrelated buffer API/copy correctness tracked separately.

---

### TIMA-FWK-0004

**ID:** `TIMA-FWK-0004`  
**Status:** `CLOSED`  
**Title:** Reported available capacity disagrees with writable capacity

**Description**  
`buffer_get_available_size()` and `buffer_is_full()` must report the remaining no-overwrite capacity of the overwrite-oldest circular buffer. The current available-size result counts the reserved sentinel slot as usable.

**Details**  
**Finding at discovery, confirmed by static review:** The circular buffer reserves one slot to distinguish empty from full. A buffer of allocated size `N` holds at most `N-1` unread bytes; writes continue when full and discard the oldest unread byte. `buffer_get_available_size()` returned `N - buffer_get_size()`, overstating remaining no-overwrite capacity by one. The discovery note about the linear no-rollover branch became obsolete when TIMA-FWK-0002 removed that mode.

**Current implementation:** `buffer_get_available_size()` reports `max(0, size - 1 - current_size)` and `buffer_is_full()` reports whether occupancy has reached `size - 1`, using a shared occupancy calculation under the existing critical-section contract. `buffer_write()` continues to accept data while full and discard the oldest unread byte, as documented in `framework/buffer.h`.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Decision:** At the user's direction, apply the existing overwrite-oldest circular policy consistently: usable capacity is `size - 1`, available size is remaining no-overwrite capacity, and full means the next write will evict the oldest unread byte. `buffer_write()` continues to accept valid bytes while full and overwrites the oldest. Set status to `IN_PROGRESS`.
- **2026-11-15 — Implementation:** Added one protected occupancy calculation shared by size, available-size, and full queries. Available size now excludes the reserved sentinel slot; full means all `size - 1` unread slots are occupied. Documented that writes still succeed when full by evicting the oldest byte.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. Added checks for the empty, intermediate, full, overwrite, and two-slot ring boundaries, including `buffer_write()` retaining its overwrite-oldest behavior.
- **2026-11-15 — Code Review:** Local review confirms the reported available capacity and full status follow the same occupancy and `size - 1` capacity definition, while writes retain the established overwrite-oldest behavior. Independent review and target-board validation have not occurred.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and passing native regression evidence. Resolution remains pending acceptance.
- **2026-11-15 — Commit:** Created implementation commit [`f8ac6e7`](https://github.com/keroblabs/tima/commit/f8ac6e7a91454e209fe0be0e1573ae5e4cebc6b3) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

- **2026-11-15 — Project Review:** The project owner indicated the submitted changes were satisfactory and requested independent review and closure if accepted.
- **2026-11-15 — Independent Code Review:** **Approved.** Independently reviewed the implementation and current buffer API. `_buffer_get_size_unlocked()` computes occupancy within the caller's existing critical section; `buffer_get_available_size()` returns `size - 1 - occupancy` (bounded below by zero) and `buffer_is_full()` compares occupancy with `size - 1`. `buffer_write()` still overwrites the oldest unread byte instead of rejecting writes. Reviewed tests for empty, partially occupied, full, overwrite, and two-byte allocated storage.
- **2026-11-15 — Verification and Scope:** Host tests are reported as passing in the submitted ticket; they were not independently executed in this review. Hardware verification is not recorded. Invalid-descriptor behavior is handled separately by TIMA-FWK-0014.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` on accepted review; `RESOLVED` → `CLOSED` because the approved implementation commit(s) are already on the designated `phase-1-improvements` branch. Associated implementation commit review status changed to `APPROVED`.

**Proposed Change**  
Document the overwrite-oldest policy, define usable capacity as `size - 1`, return remaining no-overwrite capacity from `buffer_get_available_size()`, and define full as occupancy reaching that capacity. Keep `buffer_write()` accepting data while full and verify that it evicts the oldest unread byte. Cover the first, last, and overwrite boundary slots.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`f8ac6e7`](https://github.com/keroblabs/tima/commit/f8ac6e7a91454e209fe0be0e1573ae5e4cebc6b3) | `APPROVED` | Report circular-ring capacity excluding the reserved slot; align `is_full` with occupancy. |

**Resolution Description**  
**Approved and closed.** Implementation commit [`f8ac6e7`](https://github.com/keroblabs/tima/commit/f8ac6e7a91454e209fe0be0e1573ae5e4cebc6b3) aligns capacity reporting and full detection with the circular buffer's `size - 1` usable slots while retaining overwrite-oldest writes. Independent static review verifies the expressions, protected occupancy calculation and boundary regression tests. The ticket documents passing native tests, which were not independently rerun; no hardware run is documented. The accepted implementation is already committed on `phase-1-improvements`. Related invalid-input checks are tracked in TIMA-FWK-0014.

---

### TIMA-FWK-0005

**ID:** `TIMA-FWK-0005`  
**Status:** `CLOSED`  
**Title:** Resize can invalidate cursors and truncate the stored size

**Description**  
`buffer_resize()` can replace storage without preserving valid cursor positions, and accepts a 32-bit size that is stored in a 16-bit field. For Tima 6.x, standardize all buffer sizes, capacities, cursor indices, offsets, and size-related API parameters and return values on `uint32_t`, eliminating mixed 16-bit/32-bit size handling. Review the affected functions and callers so the type change is consistent throughout the buffer API.

**Details**  
**Finding at discovery, confirmed by static review:** `buffer_resize()` copied a raw prefix, freed the old block, and assigned `buffer_data->size = new_size` without adjusting `pointer_in` or `pointer_out`. Shrinking below an active cursor could make a later access out of bounds. For `new_size > UINT16_MAX`, the stored 16-bit size truncated even though allocation used the wider value. It also did not linearize wrapped unread data or report failure to its caller.

**Current implementation:** Buffer capacities, cursor indices, size parameters, bulk counts, and size return values use `uint32_t`; the unused `fill_size` member was later removed under TIMA-FWK-0015. The Nucleo buffer configuration capacity is widened and affected board callers no longer narrow 32-bit read lengths to `uint16_t`. `buffer_resize()` linearizes unread data in FIFO order; shrinking retains the newest bytes that fit within the ring's `new_size - 1` capacity, and allocation failure leaves the old storage and cursors intact. The heap allocator's internal allocation-size parameter is also `uint32_t` and rejects sizes whose alignment adjustment would overflow. A zero resize request is ignored.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Decision:** At the user's direction, standardize every buffer size, capacity, cursor, offset, bulk count, and size return on `uint32_t`. Resize preserves unread byte order, retaining the newest bytes that fit when shrinking and normalizing cursors; allocation failure leaves the original buffer unchanged. Set status to `IN_PROGRESS`.
- **2026-11-15 — Implementation:** Widened all buffer API and `buffer_data_t` size fields to `uint32_t`, widened the Nucleo buffer configuration capacity, and removed 16-bit narrowing casts from buffer read callers. Resize now linearizes unread bytes, retaining the newest bytes that fit when shrinking and resetting cursors; a failed allocation leaves the original storage and cursor state intact.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. Added grow/shrink tests for wrapped unread data and a 70,000-byte buffer test that writes, resizes, and reads 65,540 bytes while checking wide cursor and count values. No target-board build or hardware test was run.
- **2026-11-15 — Code Review:** Local review confirms the buffer API, its struct size fields, the Nucleo capacity member, and affected read callers consistently use 32-bit sizes. Resize cursor positions remain valid after grow and shrink. Independent review and target-board validation have not occurred.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and passing native regression evidence. Resolution remains pending acceptance.
- **2026-11-15 — Commit:** Created implementation commit [`0fb2a1f`](https://github.com/keroblabs/tima/commit/0fb2a1f0f4601d11072f8f26380461e8ad3c0218) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.
- **2026-11-15 — Rework:** A follow-up target-path review found `system/heap.c` narrowed the `uint32_t` `heap_malloc()` request to `uint16_t` in `_internal_malloc()`. Large buffer allocations could therefore still be truncated on the embedded heap. Reopened the issue to remove that narrowing and verify buffer creation/resizing against the real heap implementation.
- **2026-11-15 — Status Change:** Reopened from `REVIEW` to `IN_PROGRESS` after the allocator size narrowing was found.
- **2026-11-15 — Rework:** Widened `_internal_malloc()` to `uint32_t` and reject requests above `UINT32_MAX - 3` before four-byte alignment. Updated the native buffer runner to link the project's actual heap implementation, so large-buffer tests exercise the same allocation path used on target.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror` while linking `system/heap.c`. The 70,000-byte create/write/resize/read regression passed; `UINT32_MAX` create and resize requests fail cleanly, leave no leaked allocation, and preserve the original buffer contents after resize failure.
- **2026-11-15 — Code Review:** Local review confirms no 16-bit allocation-size intermediary remains in the project heap path, alignment addition cannot wrap, and failed resize allocation preserves storage and cursors. Independent review and target-board execution have not occurred.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after rework and heap-backed regression evidence. Resolution remains pending acceptance.
- **2026-11-15 — Commit:** Created heap-path rework commit [`d280517`](https://github.com/keroblabs/tima/commit/d2805174fc1c7b4a8e6c6a5d2c1036fd60fcc268) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.
- **2026-11-15 — Cross-reference:** TIMA-FWK-0015 later removed the unreferenced `fill_size` member; the `uint32_t` fields and API size policy from this ticket remain in effect.

- **2026-11-15 — Project Review:** The project owner indicated the submitted changes were satisfactory and requested independent review and closure if accepted.
- **2026-11-15 — Independent Code Review:** **Approved.** Independently reviewed both source commits, the current `buffer_data_t` declarations and the `heap_malloc()` → `_internal_malloc()` path. Buffer size, cursor, offset, bulk counts and size-result APIs now use `uint32_t`; the heap no longer truncates allocation requests to 16 bits, and it checks for overflow before 4-byte alignment. `buffer_resize()` restores a valid linearized ring and preserves FIFO order, keeping newest unread bytes on shrink; failed allocation leaves buffer ownership and cursors unchanged. Reviewed submitted tests for wrapped grow/shrink, 70,000-byte capacity, 65,540-byte read/write counts, and failed maximum-size creation/resize.
- **2026-11-15 — Verification and Scope:** On-target board builds and hardware tests were not performed during this review; submitted native test passes were not independently rerun. Heap allocation, copying and freeing remain inside the interrupt-disabled region in `buffer_resize()`, explicitly tracked in TIMA-FWK-0006; no general real-time safety certification is implied. The buffer public structure/API changed, so application integration must rebuild affected callers.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` on accepted review; `RESOLVED` → `CLOSED` because the approved implementation commit(s) are already on the designated `phase-1-improvements` branch. Associated implementation commit review status changed to `APPROVED`.

**Proposed Change**  
Use `uint32_t` consistently for buffer sizes, capacities, cursors, offsets, bulk counts, size results, and the heap's internal allocation-size path. Resize preserves unread bytes in FIFO order; when shrinking, retain the newest unread bytes up to the new circular capacity and reset cursors to a valid linearized layout. If allocation fails, leave the original storage and indices intact. Reject allocator requests that overflow alignment arithmetic. Allocation remains protected as in the existing implementation; interrupt/heap timing is tracked by TIMA-FWK-0006.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`0fb2a1f`](https://github.com/keroblabs/tima/commit/0fb2a1f0f4601d11072f8f26380461e8ad3c0218) | `APPROVED` | Widen buffer sizes to `uint32_t`; preserve valid unread data during resize. |
| `phase-1-improvements` | [`d280517`](https://github.com/keroblabs/tima/commit/d2805174fc1c7b4a8e6c6a5d2c1036fd60fcc268) | `APPROVED` | Remove the heap's 16-bit request truncation and test buffer allocation against the real heap. |

**Resolution Description**  
**Approved and closed for the defined scope.** Commits [`0fb2a1f`](https://github.com/keroblabs/tima/commit/0fb2a1f0f4601d11072f8f26380461e8ad3c0218) and [`d280517`](https://github.com/keroblabs/tima/commit/d2805174fc1c7b4a8e6c6a5d2c1036fd60fcc268) standardize buffer sizes and indices on `uint32_t`, preserve unread data and valid cursors across successful resize, and remove the heap allocator's internal 16-bit size narrowing with alignment-overflow rejection. The later TIMA-FWK-0015 change removes obsolete metadata. The submitted native suite reports passing wrapped-data, large-size and failure-path tests. Independent static review accepts the targeted behavior; tests were not independently rerun and target hardware has not been validated. The accepted commits are already on `phase-1-improvements`. TIMA-FWK-0006 remains OPEN for allocation/free operations and copy loops inside critical sections.

---

### TIMA-FWK-0006

**ID:** `TIMA-FWK-0006`  
**Status:** `CLOSED`  
**Title:** Allocation and deallocation occur while interrupts are disabled

**Description**  
Buffer lifecycle operations call the heap allocator inside critical sections, increasing interrupt latency and lacking an established ISR-safe allocation contract.

**Details**  
**At discovery, confirmed by static review:** `buffer_destroy()` entered `DI()` then called `MFREE()` on the payload and descriptor. `buffer_resize()` entered `DI()` and performed `MMALLOC()`, copying, and `MFREE()` before `EI()`. `system/types.h` maps these to `sys_malloc()`/`sys_free()`. The custom heap has variable-work paths; neither timing limits nor safe ISR usage are established here. This was a real-time/interrupt-safety design risk, not a measured latency failure.

**Current implementation:** Buffer lifecycle APIs are task-context-only. `buffer_resize()` snapshots and commits metadata in short critical sections, with allocation, FIFO copying, and old- or replacement-storage frees outside those sections. Resizing requires caller-provided exclusive access; final destruction also requires no concurrent buffer access or outstanding pointers to its storage. The allocator's broader reentrancy and ISR-safety contract remains unverified.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's request to fix this issue, move allocation, deallocation, and resize copying out of interrupt-disabled regions. Buffer lifecycle APIs are task-context-only; `buffer_resize()` additionally requires exclusive access and no retained pointers from the pointer getters. Short critical sections snapshot and commit metadata only. This does not certify the system allocator as reentrant or ISR-safe. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** `buffer_resize()` now allocates and copies outside critical sections, snapshots and commits buffer metadata under short critical sections, and frees either the discarded replacement or old storage after interrupts are re-enabled. `buffer_destroy()` detaches owned storage under its critical section and releases it afterward. The header documents task-context and exclusive-access requirements.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. Test allocator hooks assert allocations and frees occur with interrupts enabled; wrapped grow/shrink cases also verify FIFO data and exactly two balanced critical sections per resize.
- **2026-11-16 — Local Code Review:** The resize path preserves the prior ring ordering and failure behavior, and all heap calls now follow `EI()`. The source and host test were reviewed locally; independent review and on-target timing/ISR tests have not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`9444cf9`](https://github.com/keroblabs/tima/commit/9444cf915acf906ddafd58861629cc81ef5157f2) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

- **2026-11-16 — Independent Code Review:** ChatGPT reviewed the implementation diff and current `buffer.c`/`buffer.h`, including the two-phase resize metadata snapshot/commit, FIFO-preserving copy, allocation-failure path, deferred frees, and balanced interrupt masking. The test source was inspected: allocator hooks assert no heap operations while interrupts are disabled, and wrapped grow/shrink tests check data and critical-section balance. Exclusive task-context access remains a documented precondition. The recorded native test pass was not independently rerun; no on-target timing or ISR-concurrency tests were performed. Implementation commit `9444cf9` is `APPROVED`.
- **2026-11-16 — Status Change:** `REVIEW` → `RESOLVED` following accepted review; `RESOLVED` → `CLOSED` because the approved implementation is already integrated on the designated `phase-1-improvements` working branch.

**Proposed Change**  
Define which buffer APIs are callable from interrupt context. Keep allocation, deallocation, and resize copying outside interrupt-disabled regions. Buffer lifecycle APIs are task-context-only, and resize requires exclusive access so its source storage and cursor snapshot remain stable while copying. Use short critical sections to snapshot and commit metadata, detach owned storage before release, and free old or unused allocations after leaving the critical section. Broader allocator reentrancy and ISR-safety remain a separate board/heap review; do not treat masking interrupts as a general solution for dynamic allocation.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`9444cf9`](https://github.com/keroblabs/tima/commit/9444cf915acf906ddafd58861629cc81ef5157f2) | `APPROVED` | Move heap work and resize copying outside interrupt-disabled sections. |

**Resolution Description**  
**Approved and closed.** Implementation commit [`9444cf9`](https://github.com/keroblabs/tima/commit/9444cf915acf906ddafd58861629cc81ef5157f2) moves `buffer_resize()` allocation, FIFO copying, and storage release outside interrupt-disabled regions; `buffer_destroy()` detaches owned storage under protection and frees it after interrupts are enabled. Short critical sections retain metadata snapshot and commit. Independent static review accepts the change and has inspected the regression-test assertions for balanced `DI()`/`EI()`, allocation/free outside the critical section, and wrapped grow/shrink data preservation. The implementation record reports passing native tests, which were not independently rerun, and no on-target timing/ISR verification. Resize and final destruction require exclusive task-context access, as documented; general allocator reentrancy and ISR safety are not certified. Accepted commit is on `phase-1-improvements`.

---

### TIMA-FWK-0007

**ID:** `TIMA-FWK-0007`  
**Status:** `CLOSED`  
**Title:** Replacing storage breaks heap ownership and buffer invariants

**Description**  
The public `buffer_set_buffer()` API allowed replacement of the backing pointer without reconciling ownership, existing cursors, or buffer capacity, risking leaks and invalid frees.

**Details**  
**At discovery, confirmed by static review:** `buffer_set_buffer()` assigned `buffer_data->buffer = buffer_ptr` under `DI()` / `EI()`, leaving `MASK_ON_HEAP`, `size`, `pointer_in`, and `pointer_out` unchanged. On a `buffer_create()` instance, it lost the original allocated pointer, and a later `buffer_destroy()` attempted to free the replacement pointer even if that pointer was externally owned. This presented a leak and potential invalid-free path.

**Decision and current behavior:** Remove the public setter rather than inventing implicit ownership-transfer or capacity semantics. Repository search found no production call sites; the only non-documentation references were its declaration, definition, and two null-argument test calls. `buffer_init()` and `buffer_create()` now establish the backing pointer and ownership, which callers must not reassign directly; a heap-created descriptor must not be reinitialized with `buffer_init()`. This is an intentional public API removal. External callers that used the setter must migrate; the repository contains no such production caller to update.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's direction, remove `buffer_set_buffer()` instead of adding ownership-transfer and capacity arguments. Repository search found no production callers, so there is no in-repository migration; the header will document that storage and ownership are fixed by initialization/creation and that heap-created descriptors must not be reinitialized. This is an intentional, source-breaking API removal. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Removed the setter's declaration and definition, removed the tests that invoked it, and documented the fixed-storage and ownership contract in `framework/buffer.h`. The ticket's Details and Proposed Change record why removal was selected and the lack of production callers.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`; all maintained framework tests compile and run without the removed API.
- **2026-11-16 — Local Code Review:** Confirmed the supported API no longer contains a storage-rebind operation, while `buffer_init()` and `buffer_create()` retain their borrowed/heap-owned storage behavior. Independent review has not occurred.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`92d7108`](https://github.com/keroblabs/tima/commit/92d71084f7f1fa973fa2f1154643b8beee7d64d6) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

- **2026-11-16 — Independent Code Review:** ChatGPT reviewed commit `92d7108`, the current public header and buffer implementation, and the maintained native test source. `buffer_set_buffer()` is absent from the implementation and public declarations, its obsolete test invocations were removed, and ownership/storage constraints are documented for `buffer_init()` and `buffer_create()`. The change intentionally breaks the unsupported API; external users must migrate. The recorded native test pass was not independently rerun; no on-target build was performed. Implementation commit `92d7108` is `APPROVED`.
- **2026-11-16 — Status Change:** `REVIEW` → `RESOLVED` following accepted review; `RESOLVED` → `CLOSED` because the approved implementation is already integrated on the designated `phase-1-improvements` working branch.

**Proposed Change**  
Remove `buffer_set_buffer()` from the public header and implementation. Storage is fixed by `buffer_init()` for caller-owned buffers or `buffer_create()` for heap-owned buffers; do not directly replace the public struct's backing pointer or reinitialize a heap-created descriptor. This intentionally removes the unsupported API rather than introducing a new ownership-transfer/capacity contract.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`92d7108`](https://github.com/keroblabs/tima/commit/92d71084f7f1fa973fa2f1154643b8beee7d64d6) | `APPROVED` | Remove the public storage-rebinding API and document fixed ownership. |

**Resolution Description**  
**Approved and closed.** Implementation commit [`92d7108`](https://github.com/keroblabs/tima/commit/92d71084f7f1fa973fa2f1154643b8beee7d64d6) removes `buffer_set_buffer()` from `framework/buffer.c` and `framework/buffer.h`, removes obsolete test calls, and documents that backing storage and ownership are fixed by `buffer_init()` or `buffer_create()`. Independent static review confirms the public rebind path was eliminated and the revised ownership documentation is consistent with the supported constructors. The implementation record reports no production repository callers and passing native tests, which were not independently rerun. The API removal is intentionally source-breaking for external users; no target-board build was performed. Borrowed pointer lifetime across resize and destruction remains addressed separately under TIMA-FWK-0012. Accepted commit is on `phase-1-improvements`.

---

### TIMA-FWK-0008

**ID:** `TIMA-FWK-0008`  
**Status:** `REVIEW`  
**Title:** Single-byte read cannot distinguish empty from a zero byte

**Description**  
`buffer_read()` returns a `uint8_t` value of zero when empty, which is also a legitimate byte from the buffer.

**Details**  
**At discovery, confirmed by static review:** `buffer_read()` returned `0` both when `buffer_data == NULL` or the buffer was empty, and when the stored byte actually equaled `0x00`. A caller that checked `buffer_is_empty()` first did so in a separate operation; the status and read were not one atomic result. This ambiguity affected consumers of arbitrary binary data.

**Current implementation:** Added `buffer_try_read(buffer_data, out_data)`, which atomically consumes one byte and reports success separately from its value. It returns `TRUE` for a stored zero and `FALSE` for empty/invalid buffers or null arguments without modifying the output. `buffer_read()` remains as a compatibility wrapper with its original zero-on-empty behavior.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's request to fix this issue, add a status-bearing `buffer_try_read()` operation and retain `buffer_read()` unchanged as a compatibility helper. The new operation will test emptiness and consume the byte within one critical section. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Added `buffer_try_read()` to the public API and made `buffer_read()` delegate to it, preserving the legacy return value while providing an unambiguous alternative. Added native cases for empty, invalid/null arguments, a stored zero byte, and legacy behavior.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`; the zero byte is reported as a successful read and the empty failure leaves the output unchanged.
- **2026-11-16 — Local Code Review:** Verified the status check, byte fetch, and cursor advance occur under one balanced critical section; the compatibility wrapper retains zero-on-empty behavior. Independent review and target-board testing have not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`591973d`](https://github.com/keroblabs/tima/commit/591973d8a6dcd0b86f3b492b85b14d38b85cfeaa) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

**Proposed Change**  
Add an explicit-status `buffer_try_read(buffer, &value)` operation with an atomic empty check and read. Retain `buffer_read()` as a compatibility helper and document that it still cannot distinguish empty from a stored zero.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`591973d`](https://github.com/keroblabs/tima/commit/591973d8a6dcd0b86f3b492b85b14d38b85cfeaa) | `UNREVIEWED` | Add status-bearing single-byte read while preserving the legacy wrapper. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0009

**ID:** `TIMA-FWK-0009`  
**Status:** `REVIEW`  
**Title:** Bulk read checks emptiness outside its protected operation

**Description**  
`buffer_read_buffer()` performs its empty check without holding a consistent state snapshot and then calls a second read operation, leaving a concurrency window.

**Details**  
**At discovery, confirmed by static review:** `buffer_read_buffer()` directly checked `buffer_data->pointer_in == buffer_data->pointer_out` outside `DI()` / `EI()`, then called `buffer_read()` separately for each byte. With concurrent consumers, the check could become stale and a zero-valued empty result could be copied as data. Bulk writes were likewise implemented as individually protected bytes, not an atomic batch.

**Current implementation:** `buffer_read_buffer()` now calls `buffer_try_read()` for each output byte, so the empty check, byte fetch, and cursor advance are one protected operation. Bulk reads and writes are explicitly documented as partial, non-atomic batches; other buffer operations may interleave between bytes, avoiding a long interrupt-disabled transfer.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's request to fix this issue, define bulk operations as partial and non-atomic: each byte is protected individually, and concurrent operations may interleave. This preserves ISR responsiveness; callers needing batch-level atomicity must provide external synchronization. `buffer_read_buffer()` will use the status-bearing single-byte operation from TIMA-FWK-0008. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Removed the bulk reader's unprotected validity/empty checks and routed each byte through `buffer_try_read()`. Documented partial/non-atomic semantics for bulk reads and writes. Added a deterministic host interleaving case where another consumer takes the last byte before the read critical section begins.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. Partial counts, zero-byte data, unchanged output beyond the returned count, balanced per-byte critical sections, and an interleaved consumer were verified.
- **2026-11-16 — Local Code Review:** Confirmed the emptiness check and consumption are now indivisible for each byte; the batch remains intentionally non-atomic and does not hold interrupts disabled for the full transfer. Independent review and target-board ISR testing have not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`60ebc41`](https://github.com/keroblabs/tima/commit/60ebc4113e16135b08686e2d6eaa7260f24c48d8) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

**Proposed Change**  
Define bulk reads and writes as partial, non-atomic operations with per-byte critical sections. Use the status-bearing `buffer_try_read()` operation for each byte so empty detection and consumption are atomic, and avoid prolonged interrupt masking for large transfers. Callers requiring batch-level atomicity must provide external synchronization.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`60ebc41`](https://github.com/keroblabs/tima/commit/60ebc4113e16135b08686e2d6eaa7260f24c48d8) | `UNREVIEWED` | Read bulk data through the status-bearing byte operation; document partial batches. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0010

**ID:** `TIMA-FWK-0010`  
**Status:** `REVIEW`  
**Title:** Copy-from-buffer ignores unread range and circular order

**Description**  
`buffer_write_buffer_ex()` copies the raw prefix of the source allocation rather than identifying the source buffer's logical readable contents.

**Details**  
**At discovery, confirmed by static review:** `buffer_write_buffer_ex(dst, src)` called `buffer_write_buffer(dst, src->buffer, src->pointer_in)`. It ignored `src->pointer_out` and any wrapped unread interval, so the bytes copied did not generally correspond to data available for reading from a circular buffer.

**Decision and current behavior:** Copy the source's logical unread bytes in FIFO order without consuming them. Preserving the source cursor follows the previous implementation, which did not advance it; no non-test call sites were identified. The operation snapshots the source storage and cursor metadata under a short critical section, then writes bytes individually to the destination. Source cursor/storage must remain stable during the call, the storage regions must not overlap, and destination writes remain partial/non-atomic as specified by TIMA-FWK-0009.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's request to fix this issue, implement `buffer_write_buffer_ex()` as a non-consuming copy of the source's logical unread range in FIFO order. Keeping the source cursor unchanged preserves the old helper's non-consuming behavior; this logical interpretation is chosen because the helper accepts a `buffer_data_t` and the bug is specifically the ignored unread range. Source stability and non-overlapping storage are required, while destination writes follow TIMA-FWK-0009's partial/non-atomic contract. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Replaced the raw-prefix copy with a protected source metadata snapshot and a wrap-aware FIFO loop. The helper now rejects self-copy and identical backing pointers, does not consume the source, and writes destination bytes through `buffer_write()`. Documented the copy and synchronization contract.
- **2026-11-16 — Test:** The first `tests/framework/run_buffer_tests.sh` run failed in an existing test because its source descriptor used `pointer_in == size`, which violates the circular-buffer cursor invariant. The failure exposed that the fixture relied on the former raw-prefix interpretation.
- **2026-11-16 — Rework:** Changed the existing fixture to represent the same two bytes in a valid three-byte circular buffer. No implementation change was needed; the fixture now follows the documented invariant `pointer_in < size`.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. The new partially consumed/wrapped source case verifies FIFO bytes and source cursor preservation; empty and self-copy cases also pass.
- **2026-11-16 — Local Code Review:** Verified the source unread count/order calculation, unchanged source indices, per-byte destination writes, and rejection of empty, invalid, self, and identical-storage inputs. Independent review and target-board testing have not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`a30c6ce`](https://github.com/keroblabs/tima/commit/a30c6cefd55e3900457202e492db2e1737eeef46) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

**Proposed Change**  
Implement `buffer_write_buffer_ex()` as a non-consuming copy of the source's unread bytes in FIFO order. Preserve the source cursor, require a stable source and non-overlapping storage, and retain partial/non-atomic destination writes. Test empty, partially consumed, wrapped, and self-copy cases.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`a30c6ce`](https://github.com/keroblabs/tima/commit/a30c6cefd55e3900457202e492db2e1737eeef46) | `UNREVIEWED` | Copy unread source bytes in FIFO order without consuming them. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0011

**ID:** `TIMA-FWK-0011`  
**Status:** `REVIEW`  
**Title:** Mode and offset setters bypass the critical-section contract

**Description**  
Several mutating APIs modify state that may be shared with an ISR without using the same protection as read/write operations.

**Details**  
**At discovery, confirmed by static review:** `buffer_set_no_roll_over()` and `buffer_set_read_only()` modified `mask` without `DI()` / `EI()`. `buffer_read_set_offset()` assigned to `pointer_out` without protection, whereas `buffer_reset_read()` and most index getters protected their accesses. The earlier reference to a 16-bit cursor predates TIMA-FWK-0005, which standardized it to `uint32_t`.

**Current implementation:** The no-rollover and read-only setters were removed under TIMA-FWK-0002 and TIMA-FWK-0003. `buffer_read_set_offset()` is the remaining supported setter in this finding; it validates the descriptor and updates the cursor under one balanced critical section, preserving its existing clamp-to-write-index behavior.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's request to fix this issue, preserve the current `buffer_read_set_offset()` clamp semantics while protecting its validation and cursor update with a balanced `DI()` / `EI()` pair. The two mode setters in the discovery record have already been removed under TIMA-FWK-0002 and TIMA-FWK-0003; no replacement mode setter is needed. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Moved the offset setter's descriptor validation and `pointer_out` update into a critical section. Documented its clamp behavior and added nested-critical and upper-bound regression coverage.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. The setter preserved an outer critical section, balanced its own disable/enable pair, and clamped an oversized offset to `pointer_in`.
- **2026-11-16 — Local Code Review:** Confirmed the null check remains outside the critical section, invalid descriptors do not mutate state, and valid validation/update operations are protected together. Independent review and target-board testing have not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`84204e6`](https://github.com/keroblabs/tima/commit/84204e6eae69046c880b227f39fd29fcc9fd942c) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

**Proposed Change**  
Keep the removed no-rollover/read-only modes absent. Protect `buffer_read_set_offset()` validation and cursor mutation with the repository's balanced `DI()` / `EI()` convention, preserve its existing clamp-to-write-index behavior, and document the lifecycle/interrupt-context contract.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`84204e6`](https://github.com/keroblabs/tima/commit/84204e6eae69046c880b227f39fd29fcc9fd942c) | `UNREVIEWED` | Protect offset validation and cursor update with a critical section. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0012

**ID:** `TIMA-FWK-0012`  
**Status:** `REVIEW`  
**Title:** Returned storage pointers have no stable lifetime guarantee

**Description**  
The former `buffer_get_pointer()` and `buffer_get_pointer_at()` APIs returned borrowed storage pointers whose lifetime could not be guaranteed after the functions released their critical section. These pointer-returning APIs have been removed rather than exposing storage that resize or destruction can invalidate.

**Details**  
**At discovery, confirmed by static review:** `buffer_get_pointer_at()` calculated a pointer inside `DI()` / `EI()` and returned it after `EI()`. `buffer_resize()` and final destruction can replace or release the backing allocation; the former `buffer_set_buffer()` path was removed under TIMA-FWK-0007. The offset was clamped against `pointer_in` rather than checked against logical unread data. A stale pointer required concurrent use during resize/final destruction or unsupported direct mutation of public storage fields and was not reproduced.

**Decision and current behavior:** Remove both pointer getter APIs instead of attempting to promise a storage lifetime the interface cannot enforce. Source search across maintained `.c` and `.h` files found no production call sites. The repository has older generated `.dis`/`.lst` build snapshots that still contain the former symbol names; those are not live source callers and were not regenerated for this change. This is an intentional public API removal. Callers should use the supported byte/bulk read operations for data access and must not directly access or mutate the public `buffer_data_t.buffer` field; ownership and storage are fixed by initialization/creation.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Related Change:** `buffer_set_buffer()` was removed under TIMA-FWK-0007 at the user's direction because its ownership and capacity semantics were unsafe. The supported rebind path is gone; pointer lifetime across resize/final destruction and offset semantics remain open.
- **2026-11-16 — Decision:** At the user's direction, remove both `buffer_get_pointer()` and `buffer_get_pointer_at()` rather than documenting an unenforceable lifetime guarantee or adding a replacement raw-pointer API. Search of maintained C sources/headers found no production callers to migrate. Older generated `.dis`/`.lst` snapshots retain symbol names but are not source call sites; they were not regenerated. The ticket and header explicitly record this intentional public API break and direct users to supported read APIs instead of accessing the backing field. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Removed both declarations and definitions and deleted obsolete null-pointer-getter assertions. Updated the general API comments and this ticket to explain the removal, its rationale, the absence of production callers, and the required migration away from direct storage access.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`; all maintained framework tests compile without the removed API.
- **2026-11-16 — Local Code Review:** Confirmed the source and public header no longer expose either pointer getter and the repository has no production call sites. Independent review and target-board testing have not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.

**Proposed Change**  
Remove `buffer_get_pointer()` and `buffer_get_pointer_at()` from the public header and implementation. They cannot guarantee storage lifetime across resize or destruction, and no production repository caller uses them. Treat the removal as an intentional source-breaking API change; callers should use byte/bulk read operations and not access the backing field directly. No replacement raw-pointer API is proposed.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0013

**ID:** `TIMA-FWK-0013`  
**Status:** `REVIEW`  
**Title:** Reference counter can overflow and misrepresent ownership

**Description**  
The `user_count` field is only eight bits, and `buffer_add_user()` increments it without an overflow check, potentially causing premature release or incorrect lifecycle behavior.

**Details**  
**At discovery, confirmed by static review:** `buffer_data_t.user_count` is `uint8_t`; `buffer_init()` sets it to 1; `buffer_add_user()` incremented it unconditionally; `buffer_destroy()` decremented only when greater than zero and freed an owned buffer when it became zero. The count could wrap after 255 additions.

**Current implementation:** The 8-bit limit is explicit. `buffer_add_user()` now returns `FALSE` without changing the count when passed `NULL` or when `user_count == UINT8_MAX`; otherwise it increments under a critical section and returns `TRUE`. `fsm_clone_event()` allocates the clone before acquiring a reference, checks the result, and frees the unreturned clone if acquisition fails, so allocation or reference failure does not leak or miscount an owned buffer.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-16 — Decision:** At the user's request to fix this issue, retain the existing `uint8_t` layout and make its existing ceiling explicit instead of widening every buffer descriptor. `buffer_add_user()` will report failure at `UINT8_MAX` without wrapping, and callers must handle failure before retaining another reference. `buffer_destroy()` already guards against decrementing below zero. Status moved to `IN_PROGRESS`.
- **2026-11-16 — Implementation:** Changed `buffer_add_user()` to return `bool_t` and reject null/max-count calls under the same critical-section contract. Updated `fsm_clone_event()` to allocate first, check reference acquisition, and free the clone on acquisition failure.
- **2026-11-16 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`. Added checks for successful increments, `UINT8_MAX` rejection without wrap, null input, balanced critical sections, and reference-release order.
- **2026-11-16 — Test:** An initial standalone `fsm.c` syntax check mixed the buffer test stub headers with `system/types.h` and failed on conflicting macro definitions and existing FSM warnings. A retry using system headers passed as a syntax-only check with existing unused-parameter and implicit-declaration warnings suppressed; no FSM link or runtime test was performed.
- **2026-11-16 — Local Code Review:** Verified the count remains capped, failure is observable, and FSM clone failure does not leave an uncounted buffer reference or leak its event allocation. Independent review has not been performed.
- **2026-11-16 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and native test evidence. Resolution remains pending independent review and acceptance.
- **2026-11-16 — Commit:** Created implementation commit [`8f679b6`](https://github.com/keroblabs/tima/commit/8f679b6d9679702b339be6f16cee918c392264cf) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

**Proposed Change**  
Document the `uint8_t` reference-count ceiling (`UINT8_MAX`), make `buffer_add_user()` return an observable failure rather than wrap, and ensure callers such as `fsm_clone_event()` release allocations when reference acquisition fails. Preserve the existing guarded decrement in `buffer_destroy()` and test successful increments, overflow, null input, and reference-release behavior.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`8f679b6`](https://github.com/keroblabs/tima/commit/8f679b6d9679702b339be6f16cee918c392264cf) | `UNREVIEWED` | Make buffer reference increments checked and handle FSM acquisition failure. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0014

**ID:** `TIMA-FWK-0014`  
**Status:** `CLOSED`  
**Title:** Zero sizes and invalid backing storage are not rejected

**Description**  
Buffer creation and initialization allow configurations that cannot safely store a byte, and related APIs do not consistently validate their arguments.

**Details**  
**Finding at discovery, confirmed by static review:** `buffer_init()` accepted a zero `size` or a `NULL` backing pointer with positive size; `buffer_write()` could then write through invalid storage. `buffer_create(0)` passed zero to the allocator, and several helpers dereferenced null descriptors.

**Current implementation:** Valid buffers require non-null storage and `size >= 2`. Invalid `buffer_init()` inputs leave a zero-sized inert descriptor, `buffer_create()` rejects sizes below two, and `buffer_resize()` ignores sizes below two. Byte, size, and pointer operations validate storage/cursor invariants and handle null descriptors or bulk pointers with zero, `FALSE`, `NULL`, empty/not-full, or no-op results, as documented in `framework/buffer.h`. `buffer_init()` callers must provide at least `size` accessible bytes.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Decision:** At the user's direction, require a non-null backing pointer and a size of at least two bytes, which provides one usable circular-buffer slot plus the sentinel. Invalid `buffer_init()` configurations become inert descriptors; `buffer_create()` returns `NULL`; public data operations return their documented empty/failure result or no-op. Set status to `IN_PROGRESS`.
- **2026-11-15 — Implementation:** Made invalid `buffer_init()` descriptors inert; rejected zero/one-byte create and resize requests; added null and backing-storage checks across public buffer operations; documented the minimum storage size and failure results.
- **2026-11-15 — Test / Rework:** The first regression run caught `buffer_write_buffer_ex()` using the legacy raw-prefix convention where `pointer_in == size`. Adjusted source validation to allow that one-past-end length while requiring non-null storage and in-range `pointer_out`; logical circular-copy behavior remains tracked separately by TIMA-FWK-0010.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`, linking the real heap. Added zero, one-byte, minimum valid, `UINT32_MAX`, null descriptor, null backing, null bulk pointer, and invalid resize checks; the heap-backed maximum-size allocation failure leaves no allocation behind.
- **2026-11-15 — Code Review:** Local review confirms invalid descriptors cannot reach byte access, public null arguments follow the documented result convention, and the existing source-prefix bulk-write behavior remains intact. Independent review and target-board execution have not occurred.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and passing invalid-argument regressions. Resolution remains pending acceptance.
- **2026-11-15 — Commit:** Created implementation commit [`ac40283`](https://github.com/keroblabs/tima/commit/ac402830f1468ab00797ee224960ec5998c2e9b1) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

- **2026-11-15 — Project Review:** The project owner indicated the submitted changes were satisfactory and requested independent review and closure if accepted.
- **2026-11-15 — Independent Code Review:** **Approved.** Independently reviewed `_buffer_is_valid()`, initialization/creation/resize guards, documented invalid-argument behavior and test additions. Invalid initialization produces an inert descriptor; circular buffers require non-null storage and `size >= 2`; creation and resizing reject undersized requests; byte, size, pointer and bulk APIs handle tested null/invalid descriptors with documented results. Reviewed regression coverage for zero/one/minimum capacity, null pointers and maximum-size allocation failure.
- **2026-11-15 — Verification and Scope:** `buffer_init()` relies on the caller to supply at least `size` accessible bytes and cannot prove physical backing allocation length; that precondition is documented. `buffer_write_buffer_ex()` retains an explicitly permitted raw-prefix source `pointer_in == size` case for legacy behavior, which is tracked separately under TIMA-FWK-0010. `_buffer_is_valid()` in some compound operations occurs outside a single critical section; concurrent access policy remains a separate ISR review matter. Submitted tests were not independently rerun and hardware testing is not recorded.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` on accepted review; `RESOLVED` → `CLOSED` because the approved implementation commit(s) are already on the designated `phase-1-improvements` branch. Associated implementation commit review status changed to `APPROVED`.

**Proposed Change**  
Require non-null storage and a size of at least two for valid buffers. Since `buffer_init()` has no status return, leave its descriptor inert on invalid configuration; reject invalid `buffer_create()` sizes with `NULL`. Make public operations tolerate null descriptors, invalid backing pointers, and null bulk arguments with documented no-op, zero, null-pointer, or false/empty results. Test sizes 0, 1, normal and maximum, plus invalid arguments.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`ac40283`](https://github.com/keroblabs/tima/commit/ac402830f1468ab00797ee224960ec5998c2e9b1) | `APPROVED` | Reject invalid buffer sizes/storage and guard public operations against null or invalid data. |

**Resolution Description**  
**Approved and closed.** [`ac40283`](https://github.com/keroblabs/tima/commit/ac402830f1468ab00797ee224960ec5998c2e9b1) establishes and documents the minimum two-byte circular backing store, inert invalid initialization, invalid-size rejection and consistent checked failure behavior across the reviewed public operations. Regression tests exercise null and undersized configurations, maximum-size allocation failure and the existing legacy raw-prefix source case. Independent static review confirms the focused safeguards; the submitted native test pass was not independently rerun and no hardware verification is recorded. The source commit is on `phase-1-improvements`. Other ownership and concurrent-reconfiguration concerns remain separate tickets.

---

### TIMA-FWK-0015

**ID:** `TIMA-FWK-0015`  
**Status:** `CLOSED`  
**Title:** fill_size metadata is left uninitialized and unused

**Description**  
`buffer_data_t` declares `fill_size`, but the current implementation does not initialize or maintain it, leaving dead or undefined metadata in the public structure.

**Details**  
**Finding at discovery, confirmed by static review:** `framework/buffer.h` declared `fill_size`, while `buffer_init()` omitted it and the framework did not read or update it. A caller reading it after stack-based initialization could observe an indeterminate value. Repository search found no consumer outside the declaration/initialization and regression test.

**Current implementation:** Removed the unused `fill_size` member from `buffer_data_t` and removed its initialization. `buffer_init()` tests now verify that every remaining member is initialized.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.
- **2026-11-15 — Decision:** Repository search found no `fill_size` consumer. Remove the obsolete member instead of defining and maintaining new occupancy metadata. This changes the public `buffer_data_t` layout; in-repository users will be rebuilt, while downstream binary-layout dependencies cannot be assessed from this repository. Set status to `IN_PROGRESS`.
- **2026-11-15 — Implementation:** Removed `fill_size` from `buffer_data_t` and `buffer_init()`. Added a test that pre-fills the descriptor with nonzero bytes and verifies every remaining member is initialized.
- **2026-11-15 — Test:** `tests/framework/run_buffer_tests.sh` passed with `cc -std=c99 -Wall -Wextra -Werror`, linking `system/heap.c`. The suite verifies valid and inert initialization states and confirms no source/test code depends on the removed member.
- **2026-11-15 — Code Review:** Local search confirms there are no remaining `fill_size` references in C/header files; public struct consumers in this repository compile through the updated definition. Independent review and target-board validation have not occurred.
- **2026-11-15 — Status Change:** Moved from `IN_PROGRESS` to `REVIEW` after recording implementation and passing native regression evidence. Resolution remains pending acceptance.
- **2026-11-15 — Commit:** Created implementation commit [`72664c8`](https://github.com/keroblabs/tima/commit/72664c87175b89c7d8cf573cfea0eaf6d8880775) on `phase-1-improvements`. Its review status is `UNREVIEWED`; independent review has not occurred.

- **2026-11-15 — Project Review:** The project owner indicated the submitted changes were satisfactory and requested independent review and closure if accepted.
- **2026-11-15 — Independent Code Review:** **Approved.** Independently reviewed the removal diff and current `buffer_data_t` / `buffer_init()` source. The obsolete `fill_size` field and its initialization have been removed together, leaving fields that `buffer_init()` explicitly initializes. Reviewed the new regression that pre-fills a descriptor with nonzero bytes and checks every remaining declared member. The earlier submission records a repository reference search finding no consumers.
- **2026-11-15 — Verification and Scope:** The public structure layout changes; external binary compatibility cannot be certified without rebuilding downstream users. The submitted native test pass was not independently rerun and there is no target-board build evidence. Removal does not alter the agreed `uint32_t` size policy.
- **2026-11-15 — Status Change:** `REVIEW` → `RESOLVED` on accepted review; `RESOLVED` → `CLOSED` because the approved implementation commit(s) are already on the designated `phase-1-improvements` branch. Associated implementation commit review status changed to `APPROVED`.

**Proposed Change**  
Remove `fill_size` because no in-repository caller uses it and no reliable update contract exists. Record the public struct layout change and rebuild all in-repository users as part of Tima 6 API integration.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`72664c8`](https://github.com/keroblabs/tima/commit/72664c87175b89c7d8cf573cfea0eaf6d8880775) | `APPROVED` | Remove unused `fill_size` field and verify remaining initialization. |

**Resolution Description**  
**Approved and closed.** [`72664c8`](https://github.com/keroblabs/tima/commit/72664c87175b89c7d8cf573cfea0eaf6d8880775) removes the unused `fill_size` member and its initialization from the buffer API. Independent review confirms that the remaining fields are explicitly initialized and that the new regression covers the descriptor layout. The submitted record reports no in-repository consumers and passing native tests; the test suite was not independently rerun and downstream binary users must be rebuilt for the changed struct. Commit already present on `phase-1-improvements`.
