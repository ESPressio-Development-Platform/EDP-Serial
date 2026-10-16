# serial/EdlgFramePump.hpp — optional complete EDLG frame sink bridge

**PUBLIC opt-in adapter**, EDP-Serial `feature/logging-v1`, `0.1.0`. Conditional direct **EDP-Serial→EDP-Logging** dependency. Do not add reverse EDP-Logging→EDP-Serial or SDK-specific imports. No modification to the EDP-Serial default umbrella.

- `EdlgPumpBeginStatus : std::uint8_t`: `Queued=0`, `Busy=1`, `InvalidCodec=2`, `InvalidPayload=3`.
- `EdlgPumpStatus : std::uint8_t`: `NoFrame=0`, `Complete=1`, `WouldBlock=2`, `ProviderFailure=3`, `ProviderContractViolation=4`.
- `EdlgPumpResult`: strong Status, exact cumulative `AcceptedFrameBytes`, `TotalFrameBytes` plus `Completed() const noexcept`. Accepted does **not** equal flushed/received/durable.
- `EdlgFramePump<TMaximumPayloadBytes>`: noncopyable/nonmovable, owns fixed `std::array<std::byte,MaxPayload+20>`, size, offset and pending flag; no native Buffer/heap/Signal/global singleton.
- `Begin(Logging::EdlgCodec, span<const byte> encodedPayload) noexcept`: computes canonical Logging `WriteEdlgFrame` into PRIVATE owned buffer before exposing Queued; mutating the original payload after Begin cannot change the wire image. Reports Busy without replacing any partial frame, rejects codec/payload failures.
- `Pump<ByteWriteEndpoint>(ownerSelectedEndpoint) noexcept`: bounded finite `TryWriteComplete` of only the unaccepted suffix, records exact accepted prefix, retains the entire frame across WouldBlock/ProviderFailure/contract violation and releases frame slot only after Complete.
- `Pending()`, `AcceptedFrameBytes()`, `TotalFrameBytes()`: state snapshots requiring caller-level exclusive owner coordination. No partially emitted frame Abort API.

**Critical preconditions:** the application must hold **exclusive access to the physical/USB/UART output endpoint across all Pump calls of the same pending frame**, not just around individual calls. Another pump or native UART printf can interleave bytes otherwise; this helper does not automatically serialize two producers. Underlying Serial provider must synchronously copy/accept bytes and not retain borrowed pointers. Never mistake BytesAccepted for delivered or durable; no concrete Arduino/ESP-IDF Serial provider exists yet. Physical target qualification and transport reset/loss-provenance path remain outstanding.

**Tests:** `tests/EdlgFramePumpTests.cpp` with `tests/run_logging_frame_host_tests.sh`, exact EDP-Logging 23-byte golden EDLG frame, mutation after Begin, partial/write-block/resume, failure, invalid/Busy and provider contract violation. GCC+Clang source host PASS RPI400 `20261016T083148-54fa5f8c`; Xtensa C++20 syntax PASS `20261016T083207-c1219033`. See [detailed lifecycle and gaps](../docs/EDLG-FRAME-PUMP.MD) and [Dependency Graph](../DEPENDENCY_GRAPH.MD).
