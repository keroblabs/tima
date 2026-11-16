# Tima Bugs — Framework (Volume 03)

Scope: Reusable framework components and their APIs; this volume begins with findings and readiness work for `framework/pool.c` and `framework/pool.h`.  
ID prefix: `TIMA-FWK`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)  
Previous volume: [bug-framework-02.md](bug-framework-02.md)

The summary is the current index; full issue records below are authoritative. Keep status/title changes synchronized between summary and tickets; preserve dated append-only Notes and per-commit review history. This file may contain at most 16 tickets (`TIMA-FWK-0033`–`TIMA-FWK-0048`). All entries are `OPEN` findings/proposals, not implementation approval.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `TIMA-FWK-0033` | `OPEN` | Pool storage layout and declaration macros do not guarantee alignment |
| `TIMA-FWK-0034` | `OPEN` | Returning an element creates a cycle in the free list |
| `TIMA-FWK-0035` | `OPEN` | Queue traversal dereferences the end-of-list NULL pointer |
| `TIMA-FWK-0036` | `OPEN` | Pool source depends on undefined pointer and assertion primitives |
| `TIMA-FWK-0037` | `OPEN` | Pool exhaustion is asserted despite being a normal bounded condition |
| `TIMA-FWK-0038` | `OPEN` | Pool elements can be linked into incompatible ownership states |
| `TIMA-FWK-0039` | `OPEN` | Pool and queue mutations lack interrupt-safe atomicity |
| `TIMA-FWK-0040` | `OPEN` | Unsorted FIFO insertion scans the entire linked list |
| `TIMA-FWK-0041` | `OPEN` | Pool public APIs do not validate inputs or report failure consistently |
| `TIMA-FWK-0042` | `OPEN` | Pool reinitialization and external queue lifetimes lack a contract |
| `TIMA-FWK-0043` | `OPEN` | Pool module lacks independent native invariant and integration tests |

## Issues

### TIMA-FWK-0033

**ID:** `TIMA-FWK-0033`  
**Status:** `OPEN`  
**Title:** Pool storage layout and declaration macros do not guarantee alignment

**Description**  
The pool assumes arbitrary `uint8_t` storage can hold aligned node headers and payloads, and calculates element addresses using `sizeof(pool_element_t)` without a documented alignment contract.

**Details**  
**Confirmed layout risk from `framework/pool.h:10–18,26–31` and `framework/pool.c:13–25,73,138,170,203`:** `DECLARE_POOL()` and `DECLARE_POOL_ATTR()` produce byte arrays; no alignment is requested for the header pointer or user object. The stride is `sizeof(pool_element_t) + element_size` with no rounding, so later nodes can violate either alignment requirement. Header recovery subtracts `sizeof(pool_element_t)` rather than a defined data offset; C flexible-array layout and over-aligned payload types need explicit treatment. A practical failure depends on type/board/compiler alignment; not compiled or reproduced.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Establish supported payload alignment (including any platform maximum), calculate a padded stride with overflow protection, and use `offsetof(pool_element_t, data)` or an equivalent portable offset for header recovery. Update static declaration macros and externally supplied arena requirements so every slot is validly aligned. Add type/layout tests for pointer-sized, 64-bit and stricter-alignment types on available targets.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0034

**ID:** `TIMA-FWK-0034`  
**Status:** `OPEN`  
**Title:** Returning an element creates a cycle in the free list

**Description**  
`pool_free_element()` appends a released node to the free-list tail but leaves the released node's `p_next` pointing to itself, so the list can become cyclic and hand out the same element repeatedly.

**Details**  
**Confirmed by static review of `framework/pool.c:201–220`:** `pool_malloc_element()` and `pool_remove_from_queue()` mark a detached element by setting `p_next = element`; `pool_free_element()` appends it with `last_free->p_next = this_element` (or installs it as the head), then updates `last_free`, but never changes `this_element->p_next` to `NULL`. Once popped to the tail, its self-reference can make subsequent allocation return the same pointer repeatedly. Reproduction and full invariant tests have not been run.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Restore a null-terminated free-list tail during every release and preserve correct head/tail transitions across empty, singleton and multiple-node cases. Add allocate-all, return-in-varied-order, and reallocate-all native tests verifying each returned address is unique and that exhaustion is reached at capacity. Coordinate with the ownership-state ticket.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0035

**ID:** `TIMA-FWK-0035`  
**Status:** `OPEN`  
**Title:** Queue traversal dereferences the end-of-list NULL pointer

**Description**  
`pool_get_chain_element()` treats the final queued node as if it has a successor and attempts to access `NULL->data`.

**Details**  
**Confirmed by static review of `framework/pool.c:127–147`:** `pool_add_to_queue()` terminates a normal queue with `p_next = NULL`, but `pool_get_chain_element()` checks only `this_element->p_next != this_element`. For a tail node, `NULL != this_element` is true and `this_element->p_next->data` dereferences NULL. This is distinct from handling a detached node marked by a self-reference. No runtime test was run.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Return NULL when the current element is the tail or is detached; do not dereference an absent successor. Define behavior for a NULL element (start at head) and for an element that does not belong to the requested queue. Test empty, singleton, multiple nodes, successive traversal to end and detached-node inputs.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0036

**ID:** `TIMA-FWK-0036`  
**Status:** `OPEN`  
**Title:** Pool source depends on undefined pointer and assertion primitives

**Description**  
The new pool module references `n_ptr_t` and `DO_ASSERT` without definitions in its current include path, so it is not yet self-contained for compilation.

**Details**  
**Confirmed by static inspection:** `framework/pool.c` uses `(n_ptr_t)` in four header-recovery expressions and `DO_ASSERT` in `pool_malloc_element()` and `pool_free_element()`. `pool.c` includes only `pool.h`, which includes `system/types.h`; those identifiers are not defined there in the inspected branch. The latest pool commit removed the prior `exception.h` and `arch.h` includes. This is a source-level dependency finding; no compiler command was executed and project-specific global definitions remain possible.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Make `pool.c/.h` independently compilable with the maintained Tima headers, eliminate any nonportable integer-pointer cast in favor of valid byte-pointer/offset arithmetic, and either select an existing supported assertion mechanism or remove assertion dependence from ordinary failure paths. Add a strict native C compile check and at least one representative target build.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0037

**ID:** `TIMA-FWK-0037`  
**Status:** `OPEN`  
**Title:** Pool exhaustion is asserted despite being a normal bounded condition

**Description**  
`pool_malloc_element()` calls `DO_ASSERT(ret != NULL)` when the free list is empty, even though exhausting a fixed-capacity pool is a normal nonblocking outcome.

**Details**  
**Confirmed by static review of `framework/pool.c:29–49`:** when `free_pool == NULL`, `ret` remains NULL and the function reaches `DO_ASSERT(ret != NULL)`. If assertions abort or trap, a full pipe descriptor pool becomes a fatal condition; if disabled, behavior is unclear to callers. This ticket concerns the API's behavior at capacity rather than the compile dependency tracked separately in TIMA-FWK-0036. No runtime behavior of `DO_ASSERT` was verified.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Specify that requesting an element from a full/empty pool returns an observable nonblocking failure (typically NULL) without mutating pool state; reserve assertions for genuine invariant violations. Document exact capacity, allocation and release results. Add tests at zero remaining free slots and after reclaiming slots.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0038

**ID:** `TIMA-FWK-0038`  
**Status:** `OPEN`  
**Title:** Pool elements can be linked into incompatible ownership states

**Description**  
The existing `p_next` sentinel is insufficient to enforce whether an element is free, allocated/unlinked, or owned by a particular queue, permitting the same node to be linked in multiple lists.

**Details**  
**Confirmed unsafe path in `framework/pool.c:70–125,167–220`:** `pool_add_to_queue()` accepts any `p_element`, checks `p_next != this_element`, and tries removal only from the *target* queue. If the element is actually still in the free list, another queue, or an unrelated arena, removal can fail and the function nevertheless inserts it, leaving stale links. `pool_free_element()` trusts `p_next == this_element` but does not verify origin or ownership; double free/foreign pointers remain possible. The same header link is used for free and active lists. Actual misuse by a maintained caller has not been established.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Define and enforce state transitions `FREE → ALLOCATED → QUEUED → ALLOCATED → FREE`, or an approved equivalent, including explicit ownership for multiple queues and pools. Reject duplicate enqueue, free-while-queued and foreign-pointer operations without corrupting either list. Define whether moving between queues is allowed and, if so, how removal from the previous queue occurs. Test every invalid transition and valid transfer.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0039

**ID:** `TIMA-FWK-0039`  
**Status:** `OPEN`  
**Title:** Pool and queue mutations lack interrupt-safe atomicity

**Description**  
The free-list and active-queue operations modify shared links without a critical section, so an ISR producer and main-loop consumer can interleave updates and lose or duplicate nodes.

**Details**  
**Confirmed by static review of `framework/pool.c`:** `pool_malloc_element()`, `pool_free_element()`, `pool_add_to_queue()` and `pool_remove_from_queue()` update list pointers with no `DI()`/`EI()` or documented exclusive-access requirement. `pool_get_from_head()` first peeks and then removes in separate calls, creating an additional interleaving window. Tima's intended pipe usage includes interrupt-accessible message queues, but neither an actual ISR caller of the *new pool* nor a race reproduction was verified.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Decide permitted task/ISR access and provide short, balanced critical sections for free-list transitions and queue insert/remove/dequeue. Ensure head removal is one atomic operation for permitted concurrent callers. Do not execute user callbacks or unbounded searches with interrupts masked unless a bounded contract is explicitly justified; consider exclusive caller ownership for sorted operations. Test deterministic ISR interleavings and nested critical-section restoration.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0040

**ID:** `TIMA-FWK-0040`  
**Status:** `OPEN`  
**Title:** Unsorted FIFO insertion scans the entire linked list

**Description**  
`pool_add_to_queue()` traverses a queue to find its insertion position even when no ordering callback is supplied, making the ordinary FIFO append O(n).

**Details**  
**Confirmed by static review of `framework/pool.c:70–125` and `framework/pool.h:33–37`:** a `queue_data_t` stores only the head pointer. With `add_hdl == NULL`, the search loop traverses every queued element before appending. The prospective pipe manager needs predictable O(1) enqueue/dequeue for message descriptors, whereas comparison-based ordered insertion may legitimately remain O(n). The current queue also has no count field. This is an efficiency and API-contract concern, not a measured bottleneck.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Give each queue a tail pointer (and optionally a `uint32_t` count) so FIFO insertion/dequeue is O(1), while preserving an explicitly separate ordered-insertion path if required. Maintain head/tail/count invariants for empty, singleton, middle removal and final removal. Test FIFO order, sorted insertion if retained, and transition cases; avoid changing sorting semantics silently.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0041

**ID:** `TIMA-FWK-0041`  
**Status:** `OPEN`  
**Title:** Pool public APIs do not validate inputs or report failure consistently

**Description**  
Most operations dereference descriptors, queues or caller-provided elements without checking them and return `void` even when insertion/removal may be rejected.

**Details**  
**Confirmed by static review:** `pool_create()`, `pool_malloc_element()`, `pool_free_element()`, and many queue operations assume non-NULL `pool_data` and valid pointer parameters. `pool_free_element()` dereferences `p_element` and `*p_element` unconditionally; queue operations recover headers from arbitrary `p_element` pointers. `pool_add_to_queue()` and `pool_remove_from_queue()` return `void` and do not disclose failure to locate or validate an element. Pointer validation cannot prove arbitrary foreign pointers are readable; the API must also document caller preconditions. No invalid-input runtime tests were run.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Define the null/invalid-argument and success/failure contract per API, including safe early rejection where possible and explicit preconditions for pointers that cannot be fully validated. Consider `bool_t` or a status enum for create, enqueue, remove and release operations; avoid silent mutation on failure. Add negative tests for NULL descriptors, queue pointers, element pointers, and rejected ownership operations.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0042

**ID:** `TIMA-FWK-0042`  
**Status:** `OPEN`  
**Title:** Pool reinitialization and external queue lifetimes lack a contract

**Description**  
`pool_create()` resets a descriptor's free/active lists without detecting live allocations, and caller-supplied `queue_data_t` objects have no initialization or teardown API.

**Details**  
**Confirmed by static review:** `pool_create()` unconditionally overwrites `free_pool`, `last_free`, and `active.queue` and rebuilds the arena. Existing returned pointers and queue links become invalid if the same backing memory is reinitialized while in use. The API accepts external `queue_data_t *` in queue functions, but only the embedded `active.queue` is initialized by `pool_create()`; callers must initialize external queue heads themselves without a documented rule. The current module has no destruction/reset policy. This is a lifecycle risk; no known production callers or runtime failure were demonstrated.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Define one-time creation, owner-controlled reset, valid arena lifetime and requirements for external queue initialization/destruction. Reject or explicitly forbid recreation while elements are outstanding. Provide a deterministic teardown or reset strategy consistent with statically provided storage and the absence of per-operation heap allocation. Test repeated initialization and multi-queue lifecycle transitions.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---

### TIMA-FWK-0043

**ID:** `TIMA-FWK-0043`  
**Status:** `OPEN`  
**Title:** Pool module lacks independent native invariant and integration tests

**Description**  
The new pool module currently has no dedicated executable regression suite covering its allocator, linked lists and intended role as the pipe descriptor pool.

**Details**  
**Confirmed repository observation on `phase-1-improvements`:** `tests/framework/` currently provides `run_buffer_tests.sh` and `test_buffer.c`, but no `test_pool.c` or pool test runner was present during review. The pool also is not yet build-ready under the currently exposed `types.h` dependencies (TIMA-FWK-0036). Bugs and risks recorded in TIMA-FWK-0032–0042 were identified by static analysis, not by executing pool code. Independent tests should be possible without requiring a complete pipe rewrite.

**Notes**
- **2026-11-16 — Discovery:** Independent static review of `framework/pool.c` and `framework/pool.h` on `phase-1-improvements` following the decision to stabilize the reusable pool before the linked-list pipe redesign. Evidence and remaining uncertainties are stated in Details. No source changes, runtime reproduction, target-board testing, or approved implementation are claimed. Status recorded as `OPEN` for engineering triage.

**Proposed Change**  
Create a portable native pool test target and runner using the project's existing framework test conventions. Validate arena bounds/canaries, alignment, free-list exhaustion/reuse, FIFO order, optional sorted ordering, head/middle/tail removal, cross-queue ownership, invalid inputs, lifecycle invariants and deterministic simulated interrupt interleavings. Run with warnings enabled and sanitizers when supported. Treat this ticket as verification infrastructure; individual behavior fixes remain separately tracked.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.

---
