# EDP-Serial

Platform-neutral, bounded, non-blocking serial byte endpoint contracts for the ESPressio Development Platform.

**Status:** source-complete platform-neutral Serial abstraction plus optional EDLG frame-pump and bounded FIFO, under `feature/logging-v1`, version **0.1.0**. A separate ESP-IDF native UART concrete exists in **EDP-Serial-ESP-IDF**; this abstract library itself does not configure UART/USB, own a console, start a worker or guarantee wire delivery. Target demonstration builds are being independently qualified before any `main` promotion.

## Dependencies

The default `ESPressio_Serial.hpp` imports **only EDP-System** through its static Composition Framework. The opt-in frame pump and queue introduce additional direct dependencies **only when their corresponding headers are explicitly imported**, and application manifests must declare these rather than relying on transitive discovery.

```text
EDP-Serial [default abstract ByteEndpoint]
└── EDP-System [mandatory Composition; no further EDP dependencies]

EDP-Serial [only when <serial/EdlgFramePump.hpp> is imported]
├── EDP-System [mandatory default]
└── EDP-Logging [conditional canonical EDLG; core umbrella has no EDP dependency]

EDP-Serial [only when <serial/BoundedEdlgFrameQueue.hpp> is imported]
├── EDP-System [mandatory default]
├── EDP-Logging [conditional canonical EDLG; no reverse edge]
├── EDP-Platform [conditional short independent SpinLock; depth/poison]
│   └── EDP-System
└── EDP-Threading [conditional ordinary keyed Mutex]
    ├── EDP-System
    ├── EDP-Platform
    │   └── EDP-System
    ├── EDP-Clock
    │   ├── EDP-System
    │   └── EDP-Platform
    │       └── EDP-System
    ├── EDP-BoundedTopology
    └── EDP-Memory
        ├── EDP-System
        ├── EDP-Platform
        │   └── EDP-System
        └── EDP-BoundedTopology
```

All owner versions remain **0.1.0**. EDP-Serial-ESP-IDF is an optional **downstream concrete**, not a dependency of the abstract Serial owner. See the complete [Dependency Graph](./DEPENDENCY_GRAPH.MD) for module-level activation and [Wiki Dependency Contracts](./wiki/Dependency-Contracts.md) for provider invariants.

## Public API

`#include <ESPressio_Serial.hpp>` imports `<serial/ByteEndpoint.hpp>`. The `ESPressio::Serial` domain exposes a unique EDP-System Composition-selected provider (the *only* direct EDP library dependency):

- `ByteWriteStatus`: `Accepted`, `WouldBlock`, `ProviderFailure`
- `ByteWriteResult`: provider status and exact `BytesAccepted` count for an offered borrowed byte range
- `Domain`, exclusive `ByteStreamWriter` capability and `ByteStreamWriterRequirement`: EDP-System Composition requires exactly one selected same-domain provider
- `ByteWriteEndpoint<T>`: C++20 compile-time requirement that a selected EDP-System Composition provider advertise `ByteStreamWriter` and have `ByteWriteResult TryWrite(span<const byte>) noexcept`
- `WriteAttemptStatus`: `Complete`, `WouldBlock`, `ProviderFailure`, `ProviderContractViolation`
- `WriteAttemptResult`: exact validated accepted prefix and strong outcome; `Completed()` predicate
- `TryWriteComplete(endpoint, borrowedBytes)`: attempt to consume the range without sleeps, polling, unbounded retry or owned memory. Each Accepted step advances at least one byte. WouldBlock or provider failure returns immediately, preserving valid prefix progress. An invalid provider result (zero progress Accepted, oversized count, nonzero WouldBlock, unknown status) fails closed.

**Truthful semantics:** `BytesAccepted` means the provider accepted bytes, **not** that another computer received them or that UART/DMA/CDC hardware flushed or persisted them. Failure may leave an already-sent prefix in flight; no rollback is claimed. The algorithm has at most `bytes.size()` provider calls on a non-empty input.

**Concurrency:** this is NOT a multiproducer frame atomic sink. The application must compose exclusive ownership/EDP-Threading serialization around an endpoint and handle partial writes across possible interleavers. An EDP-Logging Serial sink must own complete EDLG frames/queued output, and its UART/USB provider must expose capabilities via Bootstrap.

## Optional complete-frame EDLG Serial sink bridge (partial)

The optional header `<serial/EdlgFramePump.hpp>` composes the **canonical EDP-Logging EDLG frame writer** with this library's own bounded `ByteWriteEndpoint` consumer. It owns one fixed-size complete frame (header/CRC/payload/trailer) across partial native writes and preserves exact accepted-prefix progress across `WouldBlock` or provider failures. `Begin` rejects a new record while a frame is pending, so no record is silently replaced. `Pump` never retries WouldBlock or assumes that accepted bytes reached the host. The application MUST reserve the **whole physical/USB endpoint exclusively across all pump calls**: another writer could still interleave bytes otherwise.

This feature activates a **conditional direct EDP-Serial→EDP-Logging source dependency** ONLY when the header is imported by the application; `ESPressio_Serial.hpp` and the default `library.json` dependency on EDP-System remain unchanged. **No Logging→Serial reverse dependency**. It is NOT a concrete Arduino/ESP-IDF UART sink, complete-frame arbitration, secure archive or HIL proof. GCC/Clang host PASS and Xtensa syntax-only PASS on RPI400, see [optional EDLG frame pump](docs/EDLG-FRAME-PUMP.MD), [Wiki](wiki/Reference-serial-EdlgFramePump.md), and [Dependencies](DEPENDENCY_GRAPH.MD). Run `EDP_LOGGING_SOURCE_DIR=/path/to/EDP-Logging EDP_SYSTEM_SOURCE_DIR=/path/to/EDP-System CXX=g++ bash tests/run_logging_frame_host_tests.sh` (also `clang++`).

## Optional bounded FIFO of complete EDLG frames (additional Serial tranche)

`serial/BoundedEdlgFrameQueue.hpp` composes the **existing EDP-Logging EDLG writer**, the EDP-Serial `EdlgFramePump`, an **EDP-Threading composition-selected ordinary Mutex** for frame/output ownership, and a separately **application-selected EDP-Platform SpinLock** for independent very-short status snapshots and terminal poison publication into a fixed-capacity, noncopyable FIFO of complete framed records. Multiple producer Tasks attempt admission through one native `TryAcquire`/NoWait; one application worker `PumpNext` sends the exact oldest frame byte suffix until completed before emitting bytes from a later frame. Native lock/provider failures are explicit, including truthful "queued but lock release failed" and "frame completed but lock release failed" flags; Mutex-release failure permanently poisons the queue and prevents unsafe reuse. Source is conditional, not imported by the default umbrella.

**Qualification boundary:** only participants of **one** queue are serialized; direct UART `printf`, a second queue, native CDC output or some other writer bypassing this queue could still interleave bytes. No automatic worker, task shutdown/seal, physical delivery/durability, automatic instantiation of any Arduino/ESP-IDF Serial backend, or hard real-time bounds are claimed. GCC/Clang native concurrency and CRC tests PASS RPI400 `20261016T085921-068e380e`; Xtensa C++20 syntax-only PASS `20261016T090015-492b0459`. See [bounded FIFO](docs/BOUNDED-EDLG-FRAME-QUEUE.MD), [Wiki](wiki/Reference-serial-BoundedEdlgFrameQueue.md), [Dependency Graph](DEPENDENCY_GRAPH.MD). Run with `EDP_SERIAL_DEPENDENCY_ROOT` to the pinned EDP owner checkouts and `EDP_LOGGING_SOURCE_DIR` to actual Logging source: `CXX=g++ bash tests/run-bounded-edlg-queue-tests.sh`, repeat with Clang.

## Tests

C++20 strict host suite: `CXX=g++ bash tests/run_host_tests.sh`, repeat with `CXX=clang++`. Fake providers exercise empty input, partial progression, WouldBlock, provider failure after partial progress and invalid-contract reports. Existing strict host tests passed against the exact selected source and the post-Logging-main downstream matrix. New three-layout examples for all three logical features are under [demos](demos/README.MD); target builds and native physical Serial capture remain separate acceptance evidence and must not be inferred merely from committed example source. Existing `main` remains a stub until guarded qualification/promotion.

[Architecture](docs/ARCHITECTURE.MD) • [Dependencies](DEPENDENCY_GRAPH.MD) • [Wiki](wiki/Home.md)

## Release integration and examples — 12 November 2026

The [three V2 demo families](demos/README.MD) demonstrate the exact bounded byte endpoint, EDLG frame pump and ordinary-keyed FIFO contracts, each with Arduino IDE, PlatformIO Arduino-ESP32 and PlatformIO ESP-IDF source entry points. They use **memory-only fake endpoint providers**: this is intentionally not a real UART peripheral/USB output demonstration. Native provider selection belongs to the application and concrete repository. `EDP-Serial-Arduino` is currently a placeholder; do not infer an Arduino Serial concrete from these examples. The ESP-IDF UART concrete is separately implemented under `EDP-Serial-ESP-IDF`.

The [changelog](CHANGELOG.md), [full dependency graph](DEPENDENCY_GRAPH.MD), [normative architecture](docs/ARCHITECTURE.MD), [Wiki API](wiki/Public-API.md) and [build evidence](wiki/Build-Test-and-Source.md) describe the 0.1.0 contract. No release tag is created by these documentation updates. A passing source build establishes syntax/link compatibility on that selected target, not native FIFO drain, connected peer receipt, duration bounds, security or physical PowerLoss durability.
