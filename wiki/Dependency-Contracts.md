# Dependency contracts

**Default direct mandatory EDP dependency:** EDP-System from `main`, as declared in `library.json`, providing `Framework::Domain`, `Provider`, `Offers`, `ExclusiveCapability` and `Requirement<...,ExactlyProviders<1>>`. This owner is not duplicated in Serial.

**Opt-in EdlgFramePump:** imports `logging/EdlgFrame.hpp` from EDP-Logging, which owns the 20-byte overhead, CRC, exact framed payload byte format and codec enum. **No Logging→Serial** reciprocal dependency; default `ESPressio_Serial.hpp` has no Logging import.

**Opt-in BoundedEdlgFrameQueue:** directly imports **EDP-Platform SpinLock** for independent short state access, and EDP-Threading `OrdinaryMutex<Identity>` capability and typed `TryAcquire` / `Release`, expecting the application to supply one stable ordinary Mutex and **one separate, native qualified Platform SpinLock**. The latter never spans native Mutex waiting, frame copying, or endpoint I/O. The two providers must outlive the FIFO and callers must serialize their own endpoint. Threading's recursive graph reaches Platform, System, Clock, Memory and BoundedTopology via their existing owners, not duplicated in this queue. The demo's single-context Mutex intentionally does not implement genuine hardware concurrency.

**Concrete target:** EDP-Serial-ESP-IDF depends on EDP-Serial and native `esp_driver_uart` selected by the application, not by this abstraction; EDP-Serial-Arduino is a separate placeholder. No import of a Serial concrete into the abstract foundation is legal. Full branch-aware graph: [DEPENDENCY_GRAPH.MD](../DEPENDENCY_GRAPH.MD). Application manifests must explicitly list activated opt-in edges: an accidental transitive include does not constitute a valid Bootstrap contract.

The EDLG pump's `Begin(codec, payload, byteOperations)` borrows an
application-selected EDP-Memory ByteOperations provider during framing.
The optional FIFO uses a compile-time byte-copy provider and defaults to
the existing EDP-Platform-Portable provider. Install those owners and
their declared dependencies for the opt-in pump and queue demos.
