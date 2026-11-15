# Tima Bugs — Framework

Scope: Reusable framework components and their APIs, including interrupt safety, storage ownership, correctness, and portability.  
ID prefix: `TIMA-FWK`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative. Update both the summary and ticket when status or title changes. Keep Notes as a dated, append-only engineering history and track implementation commits and their review statuses in each ticket. Proposed changes are subject to review; an `OPEN` issue is not an approved implementation request.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `TIMA-FWK-0001` | `CLOSED` | Full-buffer rejection exits with interrupts disabled |
| `TIMA-FWK-0002` | `REVIEW` | No-rollover flag fails when combined with other flags |
| `TIMA-FWK-0003` | `REVIEW` | Read-only mode does not prevent writes |
| `TIMA-FWK-0004` | `OPEN` | Reported available capacity disagrees with writable capacity |
| `TIMA-FWK-0005` | `OPEN` | Resize can invalidate cursors and truncate the stored size |
| `TIMA-FWK-0006` | `OPEN` | Allocation and deallocation occur while interrupts are disabled |
| `TIMA-FWK-0007` | `OPEN` | Replacing storage breaks heap ownership and buffer invariants |
| `TIMA-FWK-0008` | `OPEN` | Single-byte read cannot distinguish empty from a zero byte |
| `TIMA-FWK-0009` | `OPEN` | Bulk read checks emptiness outside its protected operation |
| `TIMA-FWK-0010` | `OPEN` | Copy-from-buffer ignores unread range and circular order |
| `TIMA-FWK-0011` | `OPEN` | Mode and offset setters bypass the critical-section contract |
| `TIMA-FWK-0012` | `OPEN` | Returned storage pointers have no stable lifetime guarantee |
| `TIMA-FWK-0013` | `OPEN` | Reference counter can overflow and misrepresent ownership |
| `TIMA-FWK-0014` | `OPEN` | Zero sizes and invalid backing storage are not rejected |
| `TIMA-FWK-0015` | `OPEN` | fill_size metadata is left uninitialized and unused |

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
**Status:** `REVIEW`<br>
**Title:** No-rollover flag fails when combined with other flags

**Description**  
The no-rollover behavior depends on equality with an entire bit mask, rather than testing the no-rollover bit, so unrelated mode flags can silently disable it.

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

**Proposed Change**  
Remove the non-rolling mode consistently: delete its public setter and flag, make read and write paths unconditionally circular, and ensure heap ownership remains independent of buffer behavior. Verify wraparound, overwrite order, and critical-section balance with native tests.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`cf4083c`](https://github.com/keroblabs/tima/commit/cf4083c4f16e8d04cf9c0a77d62ab082a46de255) | `UNREVIEWED` | Remove no-rollover mode and verify circular-buffer behavior. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0003

**ID:** `TIMA-FWK-0003`  
**Status:** `REVIEW`<br>
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

**Proposed Change**  
Remove `buffer_set_read_only()` and `MASK_READ_ONLY` from the public buffer API and implementation. Verify that direct, bulk, and buffer-to-buffer writes continue to work with the remaining buffer API.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |
| `phase-1-improvements` | [`154ad57`](https://github.com/keroblabs/tima/commit/154ad57c45251393843b55893a5166bf5198694f) | `UNREVIEWED` | Remove read-only mode and verify direct and bulk writes. |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0004

**ID:** `TIMA-FWK-0004`  
**Status:** `OPEN`  
**Title:** Reported available capacity disagrees with writable capacity

**Description**  
`buffer_get_available_size()` and `buffer_is_full()` do not consistently describe how many bytes `buffer_write()` can accept, particularly at the final slot and in no-rollover mode.

**Details**  
**Confirmed by static review:** In normal circular mode, an empty ring of size `N` has a maximum unread occupancy of `N-1` because advancing `pointer_in` onto `pointer_out` discards the oldest element. However `buffer_get_available_size()` returns `N - buffer_get_size()` (reports `N` free slots when empty). In the linear no-rollover branch, `buffer_write()` rejects at `pointer_in == size - 1` rather than after the final slot; `buffer_is_full()` uses circular pointer relations regardless of mode. Expected usable capacity and overwrite policy are not documented.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Specify whether each mode uses an overwrite-oldest ring, a reject-on-full ring, or linear storage. Define usable capacity and full semantics for each mode; align `buffer_write()`, `buffer_is_full()`, and `buffer_get_available_size()` and test boundary sizes and final-slot behavior. This decision depends on the no-rollover contract in TIMA-FWK-0002.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0005

**ID:** `TIMA-FWK-0005`  
**Status:** `OPEN`  
**Title:** Resize can invalidate cursors and truncate the stored size

**Description**  
`buffer_resize()` can replace storage without preserving valid cursor positions, and accepts a 32-bit size that is stored in a 16-bit field.

**Details**  
**Confirmed by static review:** `buffer_resize()` copies a raw prefix, frees the old block, and assigns `buffer_data->size = new_size` without adjusting `pointer_in` or `pointer_out`. Shrinking below an active cursor can make a later access out of bounds. For `new_size > UINT16_MAX`, the stored `uint16_t size` truncates even though allocation uses the wider value. It also does not linearize wrapped unread data or report failure to its caller. No runtime test was performed.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Agree on supported resize semantics (preserve unread contents, reset, or reject resizing while active). Validate representable sizes, keep indices within bounds, handle allocation failure explicitly, and add grow/shrink/wrapped-data tests. Coordinate the interrupt masking and heap concerns in TIMA-FWK-0006.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0006

**ID:** `TIMA-FWK-0006`  
**Status:** `OPEN`  
**Title:** Allocation and deallocation occur while interrupts are disabled

**Description**  
Buffer lifecycle operations call the heap allocator inside critical sections, increasing interrupt latency and lacking an established ISR-safe allocation contract.

**Details**  
**Confirmed by static review:** `buffer_destroy()` enters `DI()` then calls `MFREE()` on the payload and descriptor. `buffer_resize()` enters `DI()` and performs `MMALLOC()`, `memcpy()`, and `MFREE()` before `EI()`. `system/types.h` maps these to `sys_malloc()`/`sys_free()`. The custom heap has variable-work paths; neither timing limits nor safe ISR usage are established here. This is a real-time/interrupt-safety design risk, not a measured latency failure.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Define which buffer APIs are callable from interrupt context. Prefer allocation outside the critical section; protect only ownership/index transitions and release old memory after it is no longer observable. Audit allocator reentrancy and callback lifetimes as part of the board/heap review. Do not treat masking interrupts as a general solution for dynamic allocation.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0007

**ID:** `TIMA-FWK-0007`  
**Status:** `OPEN`  
**Title:** Replacing storage breaks heap ownership and buffer invariants

**Description**  
`buffer_set_buffer()` replaces the backing pointer without reconciling ownership, existing cursors, or the buffer's capacity.

**Details**  
**Confirmed by static review:** `buffer_set_buffer()` assigns `buffer_data->buffer = buffer_ptr` under `DI()` / `EI()`, leaving `MASK_ON_HEAP`, `size`, `pointer_in`, and `pointer_out` unchanged. If invoked on a `buffer_create()` instance, it loses the original allocated pointer, and a later `buffer_destroy()` attempts to free the replacement pointer even if that pointer is externally owned. This presents a leak and potential invalid-free path.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Define whether rebinding storage is a supported operation. If retained, require explicit ownership and capacity handling, preserve or reset cursor invariants, and ensure old owned storage is safely released; otherwise remove or restrict the API. Test owned-to-borrowed and borrowed-to-owned transitions.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0008

**ID:** `TIMA-FWK-0008`  
**Status:** `OPEN`  
**Title:** Single-byte read cannot distinguish empty from a zero byte

**Description**  
`buffer_read()` returns a `uint8_t` value of zero when empty, which is also a legitimate byte from the buffer.

**Details**  
**Confirmed by static review:** `buffer_read()` returns `0` both when `buffer_data == NULL` or `pointer_in == pointer_out`, and when the stored byte actually equals `0x00`. A caller that checks `buffer_is_empty()` first does so in a separate operation; the status and read are not one atomic result. This ambiguity affects consumers of arbitrary binary data. Existing callers and compatibility expectations still need review.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Consider an explicit-status API such as a `try_read(buffer, &value)` operation, with distinct empty/invalid outcomes and a well-defined interrupt-safety contract. Review callers before deciding whether to keep `buffer_read()` as a compatibility helper.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0009

**ID:** `TIMA-FWK-0009`  
**Status:** `OPEN`  
**Title:** Bulk read checks emptiness outside its protected operation

**Description**  
`buffer_read_buffer()` performs its empty check without holding a consistent state snapshot and then calls a second read operation, leaving a concurrency window.

**Details**  
**Confirmed by static review:** `buffer_read_buffer()` directly checks `buffer_data->pointer_in == buffer_data->pointer_out` outside `DI()` / `EI()`, then calls `buffer_read()` separately for each byte. **Concurrency impact requires confirmation:** with only one reader and one ISR producer this may be acceptable as a documented non-atomic operation; with concurrent consumers, reset, resize, or cursor adjustment, the check can become stale and a zero-valued empty result can be copied as data. Bulk writes are likewise implemented as individually protected bytes, not an atomic batch.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Define supported producer/consumer patterns and whether bulk operations must be atomic, partial, or explicitly non-atomic. Remove unsynchronized assumptions, use a status-bearing byte read or protect combined checks and transfers as appropriate, and add ISR-interleaving tests. Avoid prolonged interrupt masking for large transfers.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0010

**ID:** `TIMA-FWK-0010`  
**Status:** `OPEN`  
**Title:** Copy-from-buffer ignores unread range and circular order

**Description**  
`buffer_write_buffer_ex()` copies the raw prefix of the source allocation rather than identifying the source buffer's logical readable contents.

**Details**  
**Confirmed by static review:** `buffer_write_buffer_ex(dst, src)` calls `buffer_write_buffer(dst, src->buffer, src->pointer_in)`. It ignores `src->pointer_out` and any wrapped unread interval, so the bytes copied do not generally correspond to data available for reading from a circular buffer. **Intended behavior is unverified:** the helper may have been designed for a narrower linear-storage use case, but `buffer.h` does not describe one.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Determine the original API contract and caller expectations. Either implement copying in logical read order with explicit copy/consume semantics, or rename/restrict the helper to an intentionally raw-prefix operation. Test empty, partially consumed, and wrapped buffers.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0011

**ID:** `TIMA-FWK-0011`  
**Status:** `OPEN`  
**Title:** Mode and offset setters bypass the critical-section contract

**Description**  
Several mutating APIs modify state that may be shared with an ISR without using the same protection as read/write operations.

**Details**  
**Confirmed by static review:** `buffer_set_no_roll_over()` and `buffer_set_read_only()` modify `mask` without `DI()` / `EI()`. `buffer_read_set_offset()` assigns to the 16-bit `pointer_out` without protection, whereas `buffer_reset_read()` and most index getters protect their accesses. Whether a given deployment calls these setters concurrently with an ISR is unknown; the inconsistency undermines a general interrupt-safe API guarantee.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Define whether configuration/cursor mutations are initialization-only or safe during active use. If concurrent use is allowed, protect state transitions using the agreed interrupt-state-preserving primitive and validate cursor range. Document any calls explicitly forbidden in interrupt context.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0012

**ID:** `TIMA-FWK-0012`  
**Status:** `OPEN`  
**Title:** Returned storage pointers have no stable lifetime guarantee

**Description**  
`buffer_get_pointer()` and `buffer_get_pointer_at()` return borrowed pointers whose storage may be replaced or freed immediately after the function releases its critical section.

**Details**  
**Confirmed by static review:** `buffer_get_pointer_at()` calculates a pointer inside `DI()` / `EI()` and returns it after `EI()`. Other APIs can rebind (`buffer_set_buffer()`), resize, or destroy the backing allocation. In circular mode, the offset is clamped against `pointer_in` rather than checked against logical unread data, so it is not a reliable logical data-view API. **Actual use-after-free requires a concurrent lifetime/rebind scenario or caller misuse and has not been reproduced.**

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Specify whether returned pointers refer to stable externally owned storage, require caller-managed lifetime protection, or should be replaced by safe access/copy operations. Clarify offset meaning in circular versus linear mode; test invalid offsets and reconfiguration scenarios. Coordinate with TIMA-FWK-0005 and TIMA-FWK-0007.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0013

**ID:** `TIMA-FWK-0013`  
**Status:** `OPEN`  
**Title:** Reference counter can overflow and misrepresent ownership

**Description**  
The `user_count` field is only eight bits, and `buffer_add_user()` increments it without an overflow check, potentially causing premature release or incorrect lifecycle behavior.

**Details**  
**Confirmed by static review:** `buffer_data_t.user_count` is `uint8_t`; `buffer_init()` sets it to 1; `buffer_add_user()` increments it unconditionally; `buffer_destroy()` decrements only when greater than zero, and frees an owned buffer when it becomes zero. The count wraps after 255 additions. Actual applications may use much smaller counts, but the API has no declared limit or overflow failure result.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Specify ownership and reference-count limits. Use checked increments/decrements or another explicit ownership policy, make lifecycle failures observable, and test overflow/underflow and destruction order. Establish whether reference-count functions can be used in ISR context.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0014

**ID:** `TIMA-FWK-0014`  
**Status:** `OPEN`  
**Title:** Zero sizes and invalid backing storage are not rejected

**Description**  
Buffer creation and initialization allow configurations that cannot safely store a byte, and related APIs do not consistently validate their arguments.

**Details**  
**Confirmed by static review:** `buffer_init()` accepts a zero `size` or a `NULL` backing pointer with positive size; `buffer_write()` can then write to `buffer_data->buffer[pointer_in]` without checking backing storage or capacity. `buffer_create(0)` passes zero to the allocator, whose behavior is not part of the buffer contract. Several helpers dereference their descriptor without a null check (for example, `buffer_get_available_size()`), while others return gracefully. Whether callers are required to satisfy preconditions is undocumented.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Choose a consistent contract for zero capacity and invalid pointers: reject them at initialization/creation or make zero-length buffers explicitly safe. Document caller preconditions and error returns; enforce invariants at public entry points as appropriate. Test sizes 0, 1, normal and maximum, plus invalid arguments.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0015

**ID:** `TIMA-FWK-0015`  
**Status:** `OPEN`  
**Title:** fill_size metadata is left uninitialized and unused

**Description**  
`buffer_data_t` declares `fill_size`, but the current implementation does not initialize or maintain it, leaving dead or undefined metadata in the public structure.

**Details**  
**Confirmed by static review:** `framework/buffer.h` declares `uint16_t fill_size`, while `buffer_init()` initializes the other fields and omits `fill_size`. The reviewed `framework/buffer.c` does not read or update this field. If a caller accesses it after stack-based initialization, the value may be indeterminate; whether historical callers rely on the field is not yet established.

**Notes**
- **2026-11-15 — Discovery:** Static review of `framework/buffer.c` and `framework/buffer.h` on `phase-1-improvements` (Tima 4.2-derived baseline). No runtime tests, fixes, or code reviews of implementation commits have been performed. Finding recorded for triage; proposed changes are not approved.

**Proposed Change**  
Search callers for external use of `fill_size`. Remove the field if obsolete or define its precise meaning and initialize/maintain it consistently. Keep layout or ABI compatibility needs explicit during the Tima 6 API review.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
