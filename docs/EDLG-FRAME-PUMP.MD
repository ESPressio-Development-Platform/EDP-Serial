# Optional EDLG Serial complete-frame pump (partial Logging sink)

**Branch:** EDP-Serial `feature/logging-v1`, version **0.1.0**. **Optional source only:** `#include <serial/EdlgFramePump.hpp>`. The default EDP-Serial umbrella and `library.json` dependency on EDP-System are unchanged. This feature additionally requires **EDP-Logging** (for `logging/EdlgFrame.hpp`) at the *application's explicit opt-in build layer*. This is a downstream Serial→Logging edge; there is **no Logging→Serial reverse edge**, and no concrete platform import.

## API and bounded state

`EdlgFramePump<TMaximumPayloadBytes>` owns exactly one static `std::array<std::byte, TMaximumPayloadBytes+20>` plus size, accepted-offset and pending flag. It cannot be copied or moved. The payload maximum is statically restricted to the existing EDLG v1 32-bit payload-length field and checked against `size_t` overflow.

`Begin(codec,encodedPayload)` uses **EDP-Logging's canonical `WriteEdlgFrame`** and copies the entire frame into private fixed storage before publishing `EdlgPumpBeginStatus::Queued`; the caller can immediately reuse/mutate the payload buffer. A pending frame causes `Busy` with **no mutation**, rather than silent replacement; other outcomes `InvalidCodec`, `InvalidPayload` leave no frame pending. Each queued frame has the canonical `EDLG` v1 header, CRC32C, payload and `GLDE` trailer, not a raw in-memory C++ object.

`Pump(applicationComposedByteWriteEndpoint)` calls the existing EDP-Serial `TryWriteComplete` only on the *unaccepted suffix*. It performs no polling sleeps, zero-progress loop, heap allocation, duplicate CRC/CBOR serializer or new clock. Its typed `EdlgPumpResult` reports `NoFrame`, `Complete`, `WouldBlock`, `ProviderFailure` or `ProviderContractViolation`, with **exact validated cumulative accepted-frame bytes** and total frame size. The pending frame remains byte-for-byte owned across `WouldBlock`, partial provider errors and malformed provider responses; no new frame may start until it is complete. The helper deliberately exposes NO unsafe `Abort` of a partially emitted stream: a transport reset and explicit loss-provenance policy must be owned by the eventual concrete sink. `Complete` means only **the endpoint accepted bytes**, not that another device received them, that hardware flushed, or that a persistent archive committed them.

**Concurrency is NOT solved by the pump.** The entire endpoint must be **exclusively reserved** for this pump's pending frame for every native write attempt. A second pump or direct UART/Console writer can still interleave bytes if Bootstrap does not impose a whole-frame exclusivity/queue policy. The code does not assume native UART atomic writes; its `Busy` protection prevents only replacement in this pump. This is NOT the complete D1–D13 Logging sink or durable archive.

**Lifetime:** the pump's internal buffer must outlive each accepted native write. The EDP-Serial `ByteWriteEndpoint` capability explicitly forbids providers from retaining its borrowed pointer after `TryWrite` returns; asynchronous DMA borrowing needs a separate provider ownership/acknowledgement capability. The application must retain the pump and endpoint until the record completes or a formal transport reset/loss event is registered. No generated LogRecord TypeIdentifier, Serial-Arduino provider or Serial-ESP-IDF provider has been introduced here.

## Test results / qualification gaps

`tests/EdlgFramePumpTests.cpp` imports the **actual EDP-Logging EDLG writer** and EDP-System Composition-qualified fake Serial ByteStreamWriter. Tests verify copied source lifetime, exact 23-byte canonical `abc` EDLG golden fixture (matching Logging C++/Python vector), partial 7-byte + WouldBlock + 16-byte resume, rejection of a second frame while pending, provider failure after partial accepted prefix, malformed provider no-progress failure and later successful continuation, invalid codec, oversized payload and null nonempty input. Test runner `tests/run_logging_frame_host_tests.sh` needs the EDP-System and EDP-Logging **direct** source roots supplied via `EDP_SYSTEM_SOURCE_DIR`/`EDP_LOGGING_SOURCE_DIR`; default core `tests/run_host_tests.sh` never pulls Logging into the base library.

GCC/Clang optional frame pump **and core EDP-Serial** host suites PASS, RPI400 detached `20261016T083148-54fa5f8c`. Xtensa ESP32 C++20 compile-only syntax of the same actual optional test PASS `20261016T083207-c1219033`. No linked Arduino/ESP-IDF app, RPI USB/UART output, physical peripheral/ToF/radio or complete-frame concurrent writer/HIL evidence. EDP-Serial-Arduino and EDP-Serial-ESP-IDF remain empty stubs.

The EDLG pump's `Begin(codec, payload, byteOperations)` borrows an
application-selected EDP-Memory ByteOperations provider during framing.
The optional FIFO uses a compile-time byte-copy provider and defaults to
the existing EDP-Platform-Portable provider. Install those owners and
their declared dependencies for the opt-in pump and queue demos.
