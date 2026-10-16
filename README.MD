# EDP-Serial

Platform-neutral, bounded, non-blocking serial byte endpoint contracts for the ESPressio Development Platform.

**Status:** initial **abstraction-only** feature on `feature/logging-v1`, version **0.1.0**. There is **no Arduino/ESP-IDF concrete provider, UART setup, USB/CDC deployment, Logging sink, hardware demo or device qualification** yet. Not an operational serial transport in isolation.

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

## Tests

C++20 strict host suite: `CXX=g++ bash tests/run_host_tests.sh`, repeat with `CXX=clang++`. Fake providers exercise empty input, partial progression, WouldBlock, provider failure after partial progress and invalid-contract reports. Real ESP32 target builds and physical Serial capture are future tranches. Existing `main` stub is unchanged.

[Architecture](docs/ARCHITECTURE.MD) • [Dependencies](DEPENDENCY_GRAPH.MD) • [Wiki](wiki/Home.md)
