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

- Both the original `std::atomic<bool>` poison flag and its first replacement `std::atomic<size_t>` failed **real ESP32-WROOM PIOArduino compile-time always-lock-free checks** (the exact negative runs remain in the handoff). Replaced the depth/poison publication with a **separate application-owned EDP-Platform short SpinLock provider** selected as the FIFO's fourth template/second constructor argument, while the existing EDP-Threading ordinary Mutex retains exclusive frame and native endpoint ownership. The short Platform SpinLock guards depth and poison publication. UART I/O stays under the ordinary Mutex. This is a deliberate V2 source-breaking caller migration, no compatibility aliases, versions unchanged at 0.1.0.

### Release integration

- The default PlatformIO manifest resolves EDP-System from `main`. The existing GCC and Clang host workflow checks out the public EDP-System dependency alongside EDP-Serial.
- The three FIFO demo layouts use a noncopyable, nonmovable single-context SpinLock provider. RPI400 run `20261112T114409-376d71e0` passed both FIFO host compilers and both PIOArduino targets with the published provider declarations.

### Deliberate limits

- This platform-neutral repository never selects UART pins, baud, DMA/USB transfer, ESP-IDF native driver, Arduino HardwareSerial or a free-running worker. EDP-Serial-ESP-IDF owns the existing native UART TX FIFO concrete; EDP-Serial-Arduino is a separate future owner.
- `Accepted` means bytes consumed by the selected provider, *not* on-wire transmitted, host-received, acknowledged, archived or durably persisted. A provider fault can occur after a prefix was accepted; neither `TryWriteComplete` nor the frame pump may report rollback.
- The fixed FIFO serializes only callers participating in one queue and endpoint. An independent direct UART or CDC writer can interleave output. Real multi-task Bootstrap must select a genuine EDP-Threading Mutex provider; demo mocks are not cross-task synchronization.
- No physical UART capture, arbitrary hardware performance, Flash endurance, generic PowerLoss atomicity or secure transport guarantee is given. All versions remain 0.1.0.

See [README](README.MD), [demos](demos/README.MD), [Architecture](docs/ARCHITECTURE.MD) and [G1 downstream Logging evidence](https://github.com/ESPressio-Development-Platform/Policy/blob/main/Handoff/2026-11-09/2026-11-09-09-29-EDP-Logging-Dependency-First-Reintegration.MD). Reconcile all entries before any tag.
