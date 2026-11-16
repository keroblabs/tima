# Tima Bugs — Framework (Volume 02)

Scope: Reusable framework components and their APIs; this volume begins with findings from `framework/pipe.c` and `framework/pipe.h`.  
ID prefix: `TIMA-FWK`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)  
Previous volume: [bug-framework.md](bug-framework.md)  
Next volume: [bug-framework-03.md](bug-framework-03.md)

The summary is the current index; the full issue records below are authoritative. Update both when a title or status changes. Keep dated, append-only Notes and per-commit review history. This file holds at most 16 tickets (`TIMA-FWK-0017`–`TIMA-FWK-0032`). All findings are proposed for triage; an `OPEN` ticket does not authorize implementation.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `TIMA-FWK-0017` | `OPEN` | Invalid pipe capacities can corrupt queue indices |
| `TIMA-FWK-0018` | `OPEN` | Overflow eviction leaves total_data overstated |
| `TIMA-FWK-0019` | `OPEN` | Wrapped pipe_get_used() undercounts one message |
| `TIMA-FWK-0020` | `OPEN` | Whole-message reads ignore a partially consumed head |
| `TIMA-FWK-0021` | `OPEN` | pipe_clear() underflows byte accounting after partial reads |
| `TIMA-FWK-0022` | `OPEN` | Pipe copying and deallocation run with interrupts disabled |
| `TIMA-FWK-0023` | `OPEN` | pipe_clear() bypasses queue critical-section protection |
| `TIMA-FWK-0024` | `OPEN` | pipe_send_buffer() can leak or dereference invalid inputs |
| `TIMA-FWK-0025` | `OPEN` | pipe_write() lacks an enforceable ownership and acceptance contract |
| `TIMA-FWK-0026` | `OPEN` | pipe_get() exposes a pointer with no stable lifetime guarantee |
| `TIMA-FWK-0027` | `OPEN` | pipe_get_total_size() reads mutable state without protection |
| `TIMA-FWK-0028` | `OPEN` | pipe_printk() may read past the formatting buffer after truncation |
| `TIMA-FWK-0029` | `OPEN` | Repeated pipe initialization corrupts the global registry |
| `TIMA-FWK-0030` | `OPEN` | pipe_locate() dereferences a null lookup name |
| `TIMA-FWK-0031` | `OPEN` | pipe_read_buffer() lacks destination validation |
| `TIMA-FWK-0032` | `OPEN` | Pool initialization can construct elements beyond backing storage |

## Issues

### TIMA-FWK-0017

**ID:** `TIMA-FWK-0017`  
**Status:** `OPEN`  
**Title:** Invalid pipe capacities can corrupt queue indices

**Description**  
`pipe_init()` permits a queue size of zero or one, even though the ring algorithm requires more than one slot to distinguish empty from full.

**Details**  
**Confirmed by static review:** `framework/pipe.c` accepts `uint16_t size` without a minimum check, assigns it to `pipe_data->max_size`, and uses the result as the wrap boundary in `pipe_write()`, `pipe_read()`, `pipe_clear()`, and related functions. With `size == 0`, `pointer_in` can advance past the fixed `pipe_list[MAX_PIPE_SIZE]` array because equality against zero does not wrap it. With `size == 1`, the only written pointer is immediately discarded by the overwrite condition, leaving no usable queue capacity. Values `>= MAX_PIPE_SIZE` are silently clamped to `MAX_PIPE_SIZE - 1`, so the exact public capacity contract is also unclear. No live failure has been reproduced.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Define valid queue slot counts, including how the sentinel slot affects usable capacity, and reject invalid sizes before registering or using a descriptor. Preserve intentional larger-size clamping only if reviewed and documented. Add native boundary tests for 0, 1, 2, 31, 32 and oversized inputs, including ring wrap.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0018

**ID:** `TIMA-FWK-0018`  
**Status:** `OPEN`  
**Title:** Overflow eviction leaves total_data overstated

**Description**  
When `pipe_write()` overwrites the oldest queued message, it releases the old payload but never removes its byte length from `total_data`.

**Details**  
**Confirmed by static review:** `pipe_write()` first adds the new `size` to `pipe->total_data`, advances `pointer_in`, and on collision with `pointer_out` calls `pipe_release_buffer(pipe_list[pointer_out])` and advances the read index. The matching `pipe_size[pointer_out]` is not subtracted. `pipe_read_buffer()` limits its requested bytes using `total_data`, so an inflated total can cause a request to extend beyond the actual queued messages and encounter stale entries. This is a correctness risk; no overflow/drain test was run.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Ensure eviction reduces the unread-byte total by the discarded message's unread contribution, including the case where the oldest message was partly consumed. Test full-queue writes with varied sizes, wrapped indices, partial head consumption, and complete draining; verify byte and message totals agree.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0019

**ID:** `TIMA-FWK-0019`  
**Status:** `OPEN`  
**Title:** Wrapped pipe_get_used() undercounts one message

**Description**  
The wrapped-index branch of `pipe_get_used()` subtracts one too many from the queued entry count.

**Details**  
**Confirmed by static review:** For `pointer_in < pointer_out`, `framework/pipe.c` computes `(max_size - 1 - pointer_out) + pointer_in`. For a ring whose indices range from `0` to `max_size - 1`, the number of occupied entries is `max_size - pointer_out + pointer_in` when no overflow is represented by equal cursors. Example: `max_size = 4`, `pointer_out = 3`, `pointer_in = 1` means two queued entries; the current code reports one. This can affect clients such as the ADC manager and vocoder that use this count. Not independently tested at runtime.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Correct the wrapped occupancy calculation and cover empty, one entry, full usable capacity, head/tail wrap, and overflow eviction. Confirm caller expectations distinguish message count from total byte count.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0020

**ID:** `TIMA-FWK-0020`  
**Status:** `OPEN`  
**Title:** Whole-message reads ignore a partially consumed head

**Description**  
After `pipe_read_buffer()` consumes part of a queued message, `pipe_read()`, `pipe_get()`, and `pipe_get_buffer_size()` still expose the original pointer and full length.

**Details**  
**Confirmed by static review:** `pipe_read_buffer()` advances `data_index` and decreases `total_data` on a partial read while retaining the same queued head. `pipe_read()` returns `pipe_list[pointer_out]`, reports `pipe_size[pointer_out]`, and subtracts the full original size from `total_data`, disregarding `data_index`. `pipe_get()` and `pipe_get_buffer_size()` likewise report the unadjusted entry. Mixing stream reads with whole-message APIs can duplicate consumed bytes and underflow accounting. Whether mixed use is intended is not documented; runtime reproduction was not attempted.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Decide if whole-message and streaming reads may be mixed on a single pipe. If supported, return/transfer only the unread range while preserving correct allocator ownership and byte accounting; if disallowed, enforce and document the restriction rather than silently returning stale content. Test switching modes after a partial read.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0021

**ID:** `TIMA-FWK-0021`  
**Status:** `OPEN`  
**Title:** pipe_clear() underflows byte accounting after partial reads

**Description**  
`pipe_clear()` subtracts each queued message's original length even when part of the head message was already consumed.

**Details**  
**Confirmed by static review:** After `pipe_read_buffer()` partially reads an entry, `total_data` represents remaining unread bytes and `data_index` is nonzero. `pipe_clear()` subtracts `pipe_size[pointer_out]` (the full message length) before resetting `data_index`; the subtraction exceeds the actual remaining length and can wrap the `uint32_t` total. It also does not normalize `total_data` to zero after the queue becomes empty. This is distinct from the unprotected concurrency behavior of `pipe_clear()` tracked separately.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Make clear/reset accounting consistent with remaining unread bytes and enforce an empty-queue invariant of zero byte count after clearing. Test partial reads followed by clearing, multiple messages and wrapped rings, and verify no underflow or double release.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0022

**ID:** `TIMA-FWK-0022`  
**Status:** `OPEN`  
**Title:** Pipe copying and deallocation run with interrupts disabled

**Description**  
Several pipe operations perform potentially lengthy copying or heap deallocation inside `DI()`/`EI()` critical sections.

**Details**  
**Confirmed by static review:** `pipe_read_buffer()` calls `memcpy()` for up to the requested byte count and `pipe_release_buffer()` for exhausted entries while interrupts are masked. `pipe_write()` releases evicted payloads inside its critical section on overflow. The allocator resolves to `sys_free()` via `MFREE` in the normal configuration. Heap work has variable execution time and no established ISR-safe timing contract; a latency failure is a risk, not a measured result. The buffer module addressed a similar concern under TIMA-FWK-0006, but pipe ownership and concurrency requirements need their own design.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Separate short ring-metadata transitions from large copies and heap releases. Define task/ISR usage and ownership-lifetime rules before moving work outside critical sections, avoiding races with producers and consumers. Add host assertions for allocator use outside masked sections and platform-specific interrupt-latency checks where available.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0023

**ID:** `TIMA-FWK-0023`  
**Status:** `OPEN`  
**Title:** pipe_clear() bypasses queue critical-section protection

**Description**  
`pipe_clear()` mutates queue cursors, accounting and payload ownership without the interrupt protection used by the other queue operations.

**Details**  
**Confirmed by static review:** `pipe_clear()` loops on `pointer_in != pointer_out` and frees queued pointers while updating `pointer_out`, `data_index` and `total_data`, but has no `DI()`/`EI()` or stated exclusive-access precondition. Producers and consumers elsewhere update those same fields in critical sections. When a pipe is accessed from an ISR or other concurrent context, clear can race with read/write and corrupt indices or release payloads still in use; the precise interleaving has not been tested. `pipe_clear(NULL)` also dereferences a null descriptor.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Define the clearing API's concurrency contract and make its metadata updates safe for permitted callers, without placing unbounded frees inside interrupt-disabled regions. Add tests with an interleaved producer/consumer and a null descriptor; coordinate with accounting and latency fixes.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0024

**ID:** `TIMA-FWK-0024`  
**Status:** `OPEN`  
**Title:** pipe_send_buffer() can leak or dereference invalid inputs

**Description**  
The copy-and-enqueue convenience API allocates storage before validating its destination and source arguments, and cannot report an unsuccessful enqueue.

**Details**  
**Confirmed by static review:** `pipe_send_buffer()` allocates `size` bytes, copies from `buffer`, and calls the `void` `pipe_write()`. If `pipe` is NULL but allocation succeeds, `pipe_write()` simply returns, leaking the new allocation. If `buffer` is NULL and `size > 0`, `memcpy()` dereferences it. Allocation failure is silently ignored; zero-length behavior is unspecified. These paths are visible in source but have not been exercised in native tests.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Validate destination, input and size before allocating/copying; define the zero-length policy; and make enqueue success/failure observable so temporary storage is freed on rejected submissions. Preserve nonblocking behavior. Exercise null and allocator-failure cases with leak-detecting test hooks.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0025

**ID:** `TIMA-FWK-0025`  
**Status:** `OPEN`  
**Title:** pipe_write() lacks an enforceable ownership and acceptance contract

**Description**  
`pipe_write()` stores the caller's raw pointer and may later free it on eviction, but the public API neither states ownership transfer requirements nor reports whether the pointer was accepted.

**Details**  
**Confirmed by static review:** `pipe_write()` directly stores the supplied `buffer`, without checking it for NULL when `size > 0`, then owns its lifetime implicitly through `pipe_read_buffer()`, overflow eviction, and `pipe_clear()` calls to `pipe_release_buffer()`. Callers of `pipe_read()` commonly free returned payloads explicitly, indicating a transfer-on-pop convention, but `framework/pipe.h` does not define this contract. Passing non-heap or still-owned memory can cause invalid frees; an accepted NULL payload can later be dereferenced. A NULL `pipe` silently returns with no ownership outcome. No general caller audit or runtime ownership test has been completed.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Document and enforce one consistent ownership-transfer model for direct writes, reading, overflow and clearing; define failure and invalid-payload behavior. Consider an explicit success return and retained caller ownership on rejection. Audit existing producers before source-breaking API changes, and test allocator ownership with borrowed versus heap storage.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0026

**ID:** `TIMA-FWK-0026`  
**Status:** `OPEN`  
**Title:** pipe_get() exposes a pointer with no stable lifetime guarantee

**Description**  
`pipe_get()` exposes the internal queued payload pointer after releasing its critical section, without a lifetime/synchronization guarantee for a peeking consumer.

**Details**  
**Confirmed by static review:** `pipe_get()` returns `pipe_list[pointer_out]` without dequeuing it. A concurrent `pipe_read()`, `pipe_read_buffer()`, `pipe_clear()`, or overflow eviction can remove and/or free that storage after `EI()`. `Applications/instruments/source/obd_control.c` calls `pipe_get()` for an unconsumed response, so the API has an actual consumer. The use-after-free scenario depends on concurrency or lifetime misuse and has not been reproduced. This differs from `pipe_read()`, which transfers a message out of the queue.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Determine whether peeking is required for existing consumers; define an explicit stable-lifetime or exclusive-access contract, or replace raw-pointer peeking with safe copy/borrow APIs. Review OBD usage and add tests for removals/overwrites during the peek lifetime.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0027

**ID:** `TIMA-FWK-0027`  
**Status:** `OPEN`  
**Title:** pipe_get_total_size() reads mutable state without protection

**Description**  
`pipe_get_total_size()` directly reads the 32-bit byte count without a NULL check or the critical-section convention used by related getters.

**Details**  
**Confirmed by static review:** The function is `return pipe->total_data;` with no validation or `DI()`/`EI()`. Read/write/clear paths update that field, some inside critical sections and one outside. On platforms where a 32-bit read is not naturally atomic, an ISR update can produce a torn result; otherwise it can be a stale, unsynchronized observation. `Boards/Nucleo_446/library/socket_library.c` uses the getter to cap an attempted read, so this matters to a maintained consumer. No target-specific race was measured.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Return a documented result for NULL, obtain a consistent protected count snapshot under the appropriate platform contract, and coordinate with fixes to byte-accounting updates. Test null calls and interleaving at read boundaries.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0028

**ID:** `TIMA-FWK-0028`  
**Status:** `OPEN`  
**Title:** pipe_printk() may read past the formatting buffer after truncation

**Description**  
The formatted pipe output path uses the would-have-been formatted length as a copy size even when the fixed output scratch buffer truncates the message.

**Details**  
**Confirmed by static review:** `pipe_printk()` calls `vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args)` and then `pipe_send_buffer(..., printed_len + 1)`. `c_lib/tima_libc.h` sets `VSF_PRINTK_BUF_SIZE` to 2048, while `c_lib/vsnprintk.c` returns the total logical formatted length even after it clips writes to the buffer and inserts a terminating NUL. For an output of 2048 or more characters, `printed_len + 1` exceeds the scratch buffer extent and the subsequent `memcpy()` can overread it; larger values can also narrow on the `uint16_t` pipe parameter. The global scratch buffer has additional concurrency considerations not yet verified. No repro/test was run.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Bound enqueued length to the bytes actually present in the scratch buffer, including the terminator only if required by the format contract; make truncation/error and `uint16_t` narrowing explicit. Add long-format and exact-boundary tests, and assess scratch-buffer reentrancy separately if ISR or multithread use is permitted.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0029

**ID:** `TIMA-FWK-0029`  
**Status:** `OPEN`  
**Title:** Repeated pipe initialization corrupts the global registry

**Description**  
`pipe_init()` can insert the same descriptor into the global linked list more than once and reset live queue state without releasing queued buffers.

**Details**  
**Confirmed by static review:** `pipe_init()` zeroes a descriptor and unconditionally prepends it to the `first_pipe` list. Calling it twice on the current head makes `pipe_data->next_pipe = first_pipe` point back to itself, so a subsequent unsuccessful `pipe_locate()` can loop forever. Reinitializing a descriptor deeper in the list can likewise create cycles, and `memset()` discards prior queued payload pointers without freeing them. `pipe_global_init()` resets `first_pipe` to NULL without a teardown contract. This is a lifecycle risk; actual repeated registration has not been demonstrated.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Define whether registration and initialization are one-time-only, and introduce checked re-registration or explicit unregister/teardown behavior before resetting descriptors. Prevent cycles, leaks and duplicate names where appropriate. Test repeated initialization, failed name searches, teardown and recreation.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0030

**ID:** `TIMA-FWK-0030`  
**Status:** `OPEN`  
**Title:** pipe_locate() dereferences a null lookup name

**Description**  
`pipe_locate()` calls `strcmp()` with the supplied name without validating it, allowing a NULL argument to crash or invoke undefined behavior.

**Details**  
**Confirmed by static review:** After registering any pipe, `pipe_locate(NULL)` evaluates `strcmp(local_pipe->name, name)` with `name == NULL`. With no registered pipes it returns NULL, so behavior depends on registry state. No caller passing NULL was identified in the limited search; this is an API-validation issue, not a reproduced production crash.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Define NULL-name lookup semantics (normally return NULL), avoid passing invalid pointers to `strcmp()`, and add both empty-registry and populated-registry tests. Preserve normal lookup behavior.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0031

**ID:** `TIMA-FWK-0031`  
**Status:** `OPEN`  
**Title:** pipe_read_buffer() lacks destination validation

**Description**  
The streaming read API may call `memcpy()` through a NULL destination when a nonzero read is requested and queued data exists.

**Details**  
**Confirmed by static review:** `pipe_read_buffer(pipe, buffer, max_size)` checks `pipe` for NULL but does not check `buffer`. If a message is queued and `max_size > 0`, it reaches `memcpy(&buffer[copy_index], ...)`, which is invalid for `buffer == NULL`. The function can also be invoked with a destination too small for the requested count because accessible storage length is not tracked; callers must supply a correctly sized region. No runtime test has been run.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pipe.c`, `framework/pipe.h`, relevant implementation dependencies and selected callers on `phase-1-improvements`. Evidence and remaining assumptions are recorded above; no implementation, native regression, target-board test or independent code review of a fix has been performed. The proposal below is for triage and is not yet approved.

**Proposed Change**  
Reject a NULL destination for positive-length reads without consuming queued data, document destination capacity responsibilities and zero-length no-op behavior, and test these boundary cases with the existing read/clear/ownership semantics.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0032

**ID:** `TIMA-FWK-0032`  
**Status:** `OPEN`  
**Title:** Pool initialization can construct elements beyond backing storage

**Description**  
`pool_create()` does not validate the pool descriptor, arena pointer, requested element size or complete element stride before constructing linked-list nodes. A truncated arena can therefore be written beyond its bounds.

**Details**  
**Confirmed by static review of `framework/pool.c:3–27`:** `while (temp_index < buffer_size)` accepts a node whenever its starting offset is within the arena, even if the header or payload does not fit. It writes `element->p_next` immediately and increments `temp_index` by `element_size + sizeof(pool_element_t)` without checking for overflow or a complete remaining stride. A null pool descriptor or null arena with nonzero size is dereferenced. Exact failure conditions depend on arena size and architecture; no execution was attempted. Correct capacity handling is a prerequisite for the fixed-capacity pipe descriptor pool.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Define and validate pool creation inputs and a complete per-element stride, including overflow checks. Initialize only full nodes and expose a clear success/failure result and usable element capacity. Reject zero-capacity and malformed configurations according to an agreed contract. Test empty/null, exact-fit, one-byte-short, oversized, and extreme-size cases, including guard bytes around the arena.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---
