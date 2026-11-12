# EDP-Serial Changelog

All current repository versions remain **0.1.0**. These are untagged development changes; do not create a release tag before the final exact-main, three-target demo and V2 compliance audit.

## Unreleased — 0.1.0

### Added

- C++20 EDP-System Composition domain with exactly one explicit `ByteStreamWriter` provider and a nonblocking, noexcept, byte-span `ByteWriteEndpoint` concept.
- Status-typed `TryWriteComplete` bounded transfer loop preserving the exact accepted prefix on immediate WouldBlock/provider failure, and rejecting illegal zero-progress or oversized provider reports.
- Optional `EdlgFramePump` with a statically owned complete CRC-checked EDP-Logging EDLG frame and reliable partial-byte state across attempts. No hidden retries, dynamic storage or automatic Native provider.
- Optional `BoundedEdlgFrameQueue` using an application-supplied EDP-Threading ordinary-context keyed Mutex and fixed-capacity FIFO of complete EDLG frames. Finite NoWait producer lock, stable FIFO order, typed contention/provider faults, fail-closed poison on uncertain native lock release.
- Strict GCC/Clang host tests (real source, simulated byte endpoint and Threading Mutex), source-specific V2 Wiki and three separate **Arduino IDE/PlatformIO Arduino-ESP32/PlatformIO ESP-IDF** demonstrator families. Each demo uses a single-context fake provider; *no physical serial TX* is implied by its compilation.

### Portability fixes

- Replaced the private FIFO poison latch's `std::atomic<bool>` with the already qualified always-lock-free `std::atomic<std::size_t>` 0/1 representation. This addresses the ESP32-WROOM Arduino core's actual `atomic<bool>::is_always_lock_free == false` compile failure without changing the public API, failure semantics, memory model or 0.1.0 version.

### Deliberate limits

- This platform-neutral repository never selects UART pins, baud, DMA/USB transfer, ESP-IDF native driver, Arduino HardwareSerial or a free-running worker. EDP-Serial-ESP-IDF owns the existing native UART TX FIFO concrete; EDP-Serial-Arduino is a separate future owner.
- `Accepted` means bytes consumed by the selected provider, *not* on-wire transmitted, host-received, acknowledged, archived or durably persisted. A provider fault can occur after a prefix was accepted; neither `TryWriteComplete` nor the frame pump may report rollback.
- The fixed FIFO serializes only callers participating in one queue and endpoint. An independent direct UART or CDC writer can interleave output. Real multi-task Bootstrap must select a genuine EDP-Threading Mutex provider; demo mocks are not cross-task synchronization.
- No physical UART capture, arbitrary hardware performance, Flash endurance, generic PowerLoss atomicity or secure transport guarantee is given. All versions remain 0.1.0.

See [README](README.MD), [demos](demos/README.MD), [Architecture](docs/ARCHITECTURE.MD) and [G1 downstream Logging evidence](https://github.com/ESPressio-Development-Platform/Policy/blob/main/Handoff/2026-11-09/2026-11-09-09-29-EDP-Logging-Dependency-First-Reintegration.MD). Reconcile all entries before any tag.
