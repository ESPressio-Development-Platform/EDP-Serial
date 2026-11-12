# BoundedEdlgFrameQueue — Serial 0.1.0 example

This C++20 example isolates one of the source-supported Serial contracts. **Every target is a buildable demonstration of a *memory-only app-composed provider*, NOT a real UART or USB delivery demonstration.** It creates no serial port, native driver, task, interrupt, allocation or persistence store. Each variant computes `Demo::Run()` once and stores its zero-on-success error code in debugger-visible `EdpSerialBoundedEdlgFrameQueueDemoResult`.

The example composes opt-in EDP-Logging EDLG framing and EDP-Threading's keyed ordinary Mutex capability with a statically bounded two-record FIFO. It demonstrates Full backpressure, one WouldBlock and an ordered CRC-clean pair of complete output frames. **The local example Mutex is intentionally a single-context simulation, never a real concurrent Platform/Threading native synchronization provider.** For multi-producer applications Bootstrap must inject a qualified actual Mutex and reserve exclusive access to the underlying entire endpoint across pump calls.

## Build targets

- `Arduino_IDE/Arduino_IDE.ino`: install C++20-capable Arduino-ESP32 core and the required EDP packages from current compatible source branches. This file is a single self-contained IDE sketch.
- `PlatformIO_Arduino`: `pio run -d demos/BoundedEdlgFrameQueue/PlatformIO_Arduino` using checked-out sibling ESPressio repositories at the actual qualified main refs.
- `PlatformIO_ESP-IDF`: `pio run -d demos/BoundedEdlgFrameQueue/PlatformIO_ESP-IDF` with same sibling dependencies and ESP-IDF toolchain.

All source layouts are explicit and package versions stay `0.1.0`. A successful compile is not proof of physical UART output. These examples are single-context and fail closed with a nonzero result rather than silently declaring a failed transfer successful. The named GCC/Clang host tests and genuine native provider qualification are documented separately; this example does not replace them.
