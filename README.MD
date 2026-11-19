# EDP-Serial

Bounded serial byte output for the [ESPressio Development Platform](https://github.com/ESPressio-Development-Platform).
**C++20 · Version 0.1.0 · Apache-2.0**

EDP-Serial lets application code write to an explicitly selected byte endpoint,
preserve partial progress when it cannot accept more data, and optionally send
complete CRC-checked EDLG frames through a finite FIFO. The endpoint can be a
memory capture, the existing ESP-IDF UART provider, or another conforming
application-selected provider.

| Capability | Entry point |
| --- | --- |
| Write bytes and inspect the exact accepted prefix | `ESPressio_Serial.hpp` / `TryWriteComplete` |
| Retain one complete EDLG frame across partial writes | `serial/EdlgFramePump.hpp` |
| Admit complete frames into a fixed-capacity, synchronized FIFO | `serial/BoundedEdlgFrameQueue.hpp` |

The application owns endpoint selection, resource lifetimes and service
scheduling. Acceptance describes bytes copied or consumed by the endpoint;
physical transmission, remote receipt and persistence require separate
provider/application contracts.

## Getting started

The following steps build three small host programs. Each returns zero on
success and captures bytes in memory, so no UART or connected device is needed.
The final step explains how to select the existing ESP32 UART provider.

### 1. Install the selected dependencies

Check out the EDP libraries as sibling folders under one source root.
For these host examples, obtain EDP-Serial, EDP-System, EDP-Logging,
EDP-Platform, EDP-Platform-Portable, EDP-Memory, EDP-Threading, EDP-Clock
and EDP-BoundedTopology from their `main` branches.

The basic Serial package itself requires only EDP-System. The tutorial's
memory endpoint additionally selects Portable byte operations; later steps
activate Logging and Threading. The [dependency tree](#dependencies)
explains those conditions.

Use a C++20 compiler. For ESP32 PlatformIO projects, use the
[PIOArduino platform](https://github.com/pioarduino/platform-espressif32)
and the [maintained target-specific examples](./demos/README.MD).

### 2. Supply a byte endpoint

Save this as `MemoryEndpoint.hpp`. A provider advertises the existing
`ByteStreamWriter` capability and returns a typed status plus an exact
accepted-byte count. This teaching endpoint copies into 128 fixed bytes,
accepts at most four bytes per provider call, and can simulate backpressure.
It uses the existing Memory byte-copy provider.

```cpp
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include <ESPressio_Serial.hpp>
#include <ESPressio_Platform_Portable_ByteOperations.hpp>

namespace Tutorial {

    namespace Serial = ESPressio::Serial;

    /// Memory-only endpoint: synchronously copies up to four bytes per call.
    class MemoryEndpoint final : public Serial::Framework::Provider<
        Serial::Domain,
        Serial::Framework::Offers<
            Serial::Framework::Offer<Serial::ByteStreamWriter>
        >
    > {

    private:

        // Finite application-owned capture state; access from one context.

        /// Bytes accepted by this endpoint, with no UART or external receiver.
        std::array<std::byte, 128U> _output{};
        /// Number of bytes already copied into the capture.
        std::size_t _used{0U};
        /// Requests one simulated backpressure result before the next copy.
        bool _pauseNext{false};

    public:

        // Endpoint operations and read-only capture inspection.

        /// Makes the next non-empty write attempt report WouldBlock.
        void PauseNextWrite() noexcept {
            _pauseNext = true;
        }

        /// Accepts an independent copy before the caller's borrowed span expires.
        [[nodiscard]] Serial::ByteWriteResult TryWrite(
            std::span<const std::byte> bytes
        ) noexcept {
            if (_pauseNext) {
                _pauseNext = false;
                return {Serial::ByteWriteStatus::WouldBlock, 0U};
            }
            auto count = std::min(
                bytes.size(),
                _output.size() - _used
            );
            count = std::min(
                count,
                std::size_t{4U}
            );
            if (count == 0U) {
                return {Serial::ByteWriteStatus::WouldBlock, 0U};
            }
            const ESPressio::Platform::Portable::Memory::ByteOperationsProvider copy{};
            copy.CopyBytes(
                _output.data() + _used,
                bytes.data(),
                count
            );
            _used += count;
            return {Serial::ByteWriteStatus::Accepted, count};
        }

        /// Borrows the captured bytes; valid while this endpoint remains alive.
        [[nodiscard]] std::span<const std::byte> Captured() const noexcept {
            return {
                _output.data(),
                _used
            };
        }

    };

    static_assert(Serial::ByteWriteEndpoint<MemoryEndpoint>);

} // namespace Tutorial
```

A production provider must consume or copy the accepted prefix before returning.
It must keep no borrowed pointer after `TryWrite` returns. See the
[endpoint contract](./docs/ARCHITECTURE.MD) when adapting another transport.

### 3. Write bytes and resume after backpressure

Save this as `byte-writes.cpp`. `TryWriteComplete` advances over partial
acceptance and stops immediately at `WouldBlock` or failure. A later
application service opportunity resumes the unsent suffix.

```cpp
#include "MemoryEndpoint.hpp"

int main() {
    namespace Serial = ESPressio::Serial;
    Tutorial::MemoryEndpoint endpoint{};
    const std::array<std::byte, 6U> message{
        std::byte{'H'}, std::byte{'e'}, std::byte{'l'},
        std::byte{'l'}, std::byte{'o'}, std::byte{'\n'}
    };

    endpoint.PauseNextWrite();
    const auto first = Serial::TryWriteComplete(
        endpoint,
        message
    );
    if (first.Status != Serial::WriteAttemptStatus::WouldBlock) {
        return 1;
    }

    // A later application service opportunity resumes only the unsent suffix.
    const auto remaining = std::span<const std::byte>(message).subspan(first.BytesAccepted);
    const auto resumed = Serial::TryWriteComplete(
        endpoint,
        remaining
    );
    return resumed.IsComplete() &&
        first.BytesAccepted + resumed.BytesAccepted == message.size() &&
        endpoint.Captured().size() == message.size() ? 0 : 2;
}
```

This example deliberately pauses once, then completes the six-byte message
through multiple provider calls. There is no retry loop waiting for readiness.
On `ProviderFailure`, retain the reported prefix and consult the provider's
recovery contract. A `ProviderContractViolation` requires investigation
before further use of that endpoint.

Compile and run it from the directory containing your example files. Set the
root to your sibling checkouts; reuse these flags for the next two programs.

```bash
EDP_SOURCE_ROOT=/path/to/EDP-checkouts
includes=()
for library in Serial System Logging Platform Platform-Portable Memory Threading Clock BoundedTopology; do
    includes+=("-I$EDP_SOURCE_ROOT/EDP-$library/src")
done
g++ -std=c++20 -pthread "${includes[@]}" byte-writes.cpp -o byte-writes
./byte-writes
```

### 4. Add a complete EDLG frame

Save this as `framed-output.cpp`. The pump copies the encoded payload into
its own fixed frame storage before returning `Queued`, then retains that
frame and its exact accepted prefix across service calls.

```cpp
#include "MemoryEndpoint.hpp"
#include <serial/EdlgFramePump.hpp>
#include <logging/EdlgFrameReader.hpp>

int main() {
    namespace Serial = ESPressio::Serial;
    namespace Logging = ESPressio::Logging;
    Tutorial::MemoryEndpoint endpoint{};
    Serial::EdlgFramePump<16U> pump{};
    const ESPressio::Platform::Portable::Memory::ByteOperationsProvider copy{};
    const std::array<std::byte, 2U> json{
        std::byte{'{'}, std::byte{'}'}
    };

    const auto started = pump.Begin(
        Logging::EdlgCodec::Json,
        json,
        copy
    );
    if (started != Serial::EdlgPumpBeginStatus::Queued) {
        return 1;
    }

    endpoint.PauseNextWrite();
    const auto first = pump.Pump(endpoint);
    if (first.Status != Serial::EdlgPumpStatus::WouldBlock || !pump.IsPending()) {
        return 2;
    }

    // On the next service opportunity, the same pump retains the frame/suffix.
    const auto resumed = pump.Pump(endpoint);
    if (!resumed.IsComplete() || pump.IsPending()) {
        return 3;
    }

    Logging::EdlgFrameView decoded{};
    return Logging::ReadEdlgFrame(
        endpoint.Captured(),
        decoded,
        16U
    ) == Logging::EdlgFrameReadResult::Succeeded &&
        decoded.Codec == Logging::EdlgCodec::Json &&
        decoded.Payload.size() == json.size() ? 0 : 4;
}
```

The two JSON bytes `{}` illustrate framing; they are not a generated
15-field diagnostic LogRecord. The example also validates the captured frame's
CRC and payload boundary with Logging's reader.

```bash
g++ -std=c++20 -pthread "${includes[@]}" framed-output.cpp -o framed-output
./framed-output
```

Reserve the whole endpoint exclusively until the pending frame completes.
A new `Begin` returns `Busy` while a frame is pending. After a provider
failure, retain the pump and follow the recovery contract rather than starting
another frame over an uncertain stream. See [EDLG Frame Pump](./docs/EDLG-FRAME-PUMP.MD).

### 5. Add a bounded output queue

Save this as `queued-output.cpp`. This host composition reuses a real
Portable mutex and SpinLock through the existing Threading adapter. The queue
holds at most two payloads of at most 16 bytes each. It creates no worker;
the application supplies the service context calling `PumpNext`.

```cpp
#include "MemoryEndpoint.hpp"
#include <ESPressio_Platform_Portable.hpp>
#include <ESPressio_Threading.hpp>
#include <serial/BoundedEdlgFrameQueue.hpp>

/// Separates this queue's ordinary mutex from other application mutexes.
struct OutputQueueIdentity final {};

int main() {
    namespace Serial = ESPressio::Serial;
    namespace Logging = ESPressio::Logging;
    namespace Portable = ESPressio::Platform::Portable;
    using OutputMutex = ESPressio::Threading::OrdinaryMutexProvider<
        OutputQueueIdentity,
        Portable::Synchronization::MutexProvider
    >;

    // Declare borrowed owners first so they outlive the queue.
    OutputMutex mutex{};
    Portable::Synchronization::SpinLockProvider stateLock{};
    Serial::BoundedEdlgFrameQueue<
        2U,
        16U,
        OutputMutex,
        Portable::Synchronization::SpinLockProvider
    > output(
        mutex,
        stateLock
    );
    Tutorial::MemoryEndpoint endpoint{};
    const std::array<std::byte, 2U> json{
        std::byte{'{'}, std::byte{'}'}
    };

    const auto offered = output.TryEnqueue(
        Logging::EdlgCodec::Json,
        json
    );
    if (offered.Status != Serial::EdlgQueueOfferStatus::Queued) {
        return 1;
    }

    endpoint.PauseNextWrite();
    const auto first = output.PumpNext(endpoint);
    if (first.Status != Serial::EdlgQueuePumpStatus::WouldBlock ||
        output.Depth() != 1U) {
        return 2;
    }

    // One application-owned service context retries the retained oldest frame.
    const auto resumed = output.PumpNext(endpoint);
    return resumed.Status == Serial::EdlgQueuePumpStatus::Complete &&
        resumed.FrameCompleted && output.Depth() == 0U &&
        !output.IsPoisoned() ? 0 : 3;
}
```

```bash
g++ -std=c++20 -pthread "${includes[@]}" queued-output.cpp -o queued-output
./queued-output
```

On embedded targets, select the native qualified synchronization providers
instead of the host Portable synchronization used here. Keep the providers,
queue and endpoint alive until pending output has been reconciled.

Check every admission and pump result. Capacity exhaustion, mutex contention
and provider failures are distinct outcomes. Native mutex-release failure
poisons the queue; the structured result can report a frame admitted or
completed before that failure. Avoid blind resubmission, which could duplicate
output. [Bounded EDLG Frame Queue](./docs/BOUNDED-EDLG-FRAME-QUEUE.MD)
defines these outcomes and the ownership rules.

### 6. Select a real ESP32 UART endpoint

For an ESP-IDF or compatible IDF-backed Arduino-ESP32 application, add
[EDP-Serial-ESP-IDF](https://github.com/ESPressio-Development-Platform/EDP-Serial-ESP-IDF).
Bootstrap must configure its chosen UART's pins and baud, install the driver
with **TX software ring-buffer size zero**, and own the port exclusively.
The provider does not perform this setup.

After that setup, replace the memory endpoint in the byte-writing example:

```cpp
#include <ESPressio_Serial_ESP_IDF.hpp>

// Inside the application, after UART1 has been configured and installed:
ESPressio::Serial::ESP_IDF::UartFifoByteStreamWriter endpoint(UART_NUM_1);
const auto result = ESPressio::Serial::TryWriteComplete(
    endpoint,
    message
);
```

Use the same endpoint in the pump or queue examples. Adapt the host examples'
`main` entry point to the selected framework and select native synchronization
for a concurrent queue. The concrete provider's
[three-target UART example](https://github.com/ESPressio-Development-Platform/EDP-Serial-ESP-IDF/tree/main/demos/UartFifoByteStreamWriter)
documents the hardware setup. Other console writers must not bypass the
queue on the same UART. Generic `EDP-Serial-Arduino` remains a placeholder.

## Dependencies

`ESPressio_Serial.hpp` requires EDP-System. Additional edges are imposed by
the exact optional headers below; installing the minimal package does not
activate them.

```text
EDP-Serial
├── EDP-System [mandatory: default byte endpoint and Composition]
├── EDP-Logging [optional: import EdlgFramePump.hpp or BoundedEdlgFrameQueue.hpp]
│   └── EDP-Memory [imposed by Logging's EdlgFrameWriter.hpp]
│       ├── EDP-System
│       ├── EDP-Platform
│       │   └── EDP-System
│       └── EDP-BoundedTopology
├── EDP-Platform-Portable [optional: import BoundedEdlgFrameQueue.hpp]
│   ├── EDP-Platform
│   │   └── EDP-System
│   └── EDP-Memory
│       ├── EDP-System
│       ├── EDP-Platform
│       │   └── EDP-System
│       └── EDP-BoundedTopology
├── EDP-Platform [optional: import BoundedEdlgFrameQueue.hpp]
│   └── EDP-System
└── EDP-Threading [optional: import BoundedEdlgFrameQueue.hpp]
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

The queue's Portable header is included even when a custom fifth template
argument supplies byte copying, so that header dependency remains imposed.
The pump accepts a caller-selected Memory-compatible byte provider; the
tutorial selects Portable explicitly at the application boundary.

Concrete Serial providers depend on EDP-Serial. The abstract library never
imports them. See the complete [Dependency Graph](./DEPENDENCY_GRAPH.MD)
for header activation and application-only provider edges.

## Documentation and examples

- [Architecture and byte endpoint contract](./docs/ARCHITECTURE.MD)
- [EDLG Frame Pump](./docs/EDLG-FRAME-PUMP.MD)
- [Bounded EDLG Frame Queue](./docs/BOUNDED-EDLG-FRAME-QUEUE.MD)
- [Three-target demo projects](./demos/README.MD): Arduino IDE, PlatformIO Arduino-ESP32 and PlatformIO ESP-IDF
- [Developer Wiki](./wiki/Home.md), [API reference](./wiki/Public-API.md) and [build/test guidance](./wiki/Build-Test-and-Source.md)
- [Changelog](./CHANGELOG.md) and [Apache-2.0 license](./LICENSE)

The maintained byte, frame and queue demos use memory endpoints. They
demonstrate API behavior without claiming UART delivery. Physical UART
qualification belongs to the concrete provider/application's recorded scope.
