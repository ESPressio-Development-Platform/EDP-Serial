# Resources, concurrency and lifecycle

**Byte transfer:** `TryWriteComplete` borrows a caller-owned span and retains only a size_t prefix count. Its finite loop performs at most N native calls for N offered bytes and returns immediately on WouldBlock/fault/contract violation. No retry task, DMA descriptor, timeout clock, Heap or platform mutex is owned here.

**Frame pump:** one `EdlgFramePump<N>` owns exactly `N+20` fixed frame bytes; `Begin` copies and CRC-frames the source, after which the source may die. Pending output continues from the real accepted suffix after each provider attempt. A new frame cannot replace pending data. The application must explicitly resolve a repeated provider failure or deliberately reset an entire damaged transport at a higher level; this class does not silently discard the in-flight frame.

**FIFO:** `BoundedEdlgFrameQueue<Capacity,Payload,Mutex>` owns exactly Capacity complete pumps, fixed head/tail and lock-free published depth/poison facts. Its application-owned Threading Mutex must outlive every producer/consumer call, and a producer cannot assume fairness when full or contended. The FIFO only preserves byte ordering among participants of this single Mutex and endpoint. A native mutex release failure is terminal Poisoned even if the frame was already fully admitted or consumed; it never invents a clean retry. ISR use and arbitrary blocking sender callbacks are not qualified.

No Flash, clock correlation, durable archive, native UART ring-buffer, network interface or long-term storage is installed by these contracts. See [normative bounded queue](../docs/BOUNDED-EDLG-FRAME-QUEUE.MD) and [source declarations](Reference-Index).
