# ByteEndpoint — Serial 0.1.0 example

This C++20 example isolates one of the source-supported Serial contracts. **Every target is a buildable demonstration of a *memory-only app-composed provider*, NOT a real UART or USB delivery demonstration.** It creates no serial port, native driver, task, interrupt, allocation or persistence store. Each variant computes `Demo::Run()` once and stores its zero-on-success error code in debugger-visible `EdpSerialByteEndpointDemoResult`.

The example shows exactly-one-provider EDP-System Composition, finite partial native acceptance, a completed bounded borrowed span and immediate WouldBlock without spinning. Bytes accepted into this example's RAM are not physically delivered.

## Build targets

- `Arduino_IDE/Arduino_IDE.ino`: install C++20-capable Arduino-ESP32 core and the required EDP packages from current compatible source branches. This file is a single self-contained IDE sketch.
- `PlatformIO_Arduino`: `pio run -d demos/ByteEndpoint/PlatformIO_Arduino` using the declared GitHub dependency URLs.
- `PlatformIO_ESP-IDF`: `pio run -d demos/ByteEndpoint/PlatformIO_ESP-IDF` with the declared GitHub dependency URLs and ESP-IDF toolchain.

All source layouts are explicit and package versions stay `0.1.0`. A successful compile is not proof of physical UART output. These examples are single-context and fail closed with a nonzero result rather than silently declaring a failed transfer successful. The named GCC/Clang host tests and genuine native provider qualification are documented separately; this example does not replace them.

## PlatformIO dependency setup

The two PlatformIO manifests declare GitHub URLs in lib_deps.
Keep this demo's repository-owned source and project assets together,
then run either command from the repository root:

```sh
pio run -d demos/ByteEndpoint/PlatformIO_Arduino
pio run -d demos/ByteEndpoint/PlatformIO_ESP-IDF
```

PlatformIO downloads the libraries; sibling library checkouts and local
dependency path overrides are unnecessary. Arduino-ESP32 selects the
qualified3.3.12 framework and ESP-IDF5.5 library bundle. Project-owned
partition files and build hooks are resolved within the demo itself.
