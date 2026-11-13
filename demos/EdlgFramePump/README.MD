# EdlgFramePump — Serial 0.1.0 example

This C++20 example isolates one of the source-supported Serial contracts. **Every target is a buildable demonstration of a *memory-only app-composed provider*, NOT a real UART or USB delivery demonstration.** It creates no serial port, native driver, task, interrupt, allocation or persistence store. Each variant computes `Demo::Run()` once and stores its zero-on-success error code in debugger-visible `EdpSerialEdlgFramePumpDemoResult`.

The example composes opt-in EDP-Logging EDLG framing with EDP-Serial's fixed-size pending-frame pump. It deliberately blocks one native attempt, then resumes the same frame without duplicating accepted bytes and validates its CRC using the canonical Logging reader. The JSON payload is illustrative `{}`, **not** a generated 15-field LogRecord.

## Build targets

- `Arduino_IDE/Arduino_IDE.ino`: install C++20-capable Arduino-ESP32 core and the required EDP packages from current compatible source branches. This file is a single self-contained IDE sketch.
- `PlatformIO_Arduino`: `pio run -d demos/EdlgFramePump/PlatformIO_Arduino` using checked-out sibling ESPressio repositories at the actual qualified main refs.
- `PlatformIO_ESP-IDF`: `pio run -d demos/EdlgFramePump/PlatformIO_ESP-IDF` with same sibling dependencies and ESP-IDF toolchain.

All source layouts are explicit and package versions stay `0.1.0`. A successful compile is not proof of physical UART output. These examples are single-context and fail closed with a nonzero result rather than silently declaring a failed transfer successful. The named GCC/Clang host tests and genuine native provider qualification are documented separately; this example does not replace them.

The EDLG pump's `Begin(codec, payload, byteOperations)` borrows an
application-selected EDP-Memory ByteOperations provider during framing.
The optional FIFO uses a compile-time byte-copy provider and defaults to
the existing EDP-Platform-Portable provider. Install those owners and
their declared dependencies for the opt-in pump and queue demos.
