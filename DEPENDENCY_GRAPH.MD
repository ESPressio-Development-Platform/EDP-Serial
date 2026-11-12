# EDP-Serial Dependency Graph

Package version **0.1.0**. The default package resolves EDP-System from its `main` branch.

## Current full mandatory dependency tree

```text
EDP-Serial
└── EDP-System [mandatory]
```

The **only direct EDP dependency** is EDP-System's static Composition Framework. `Serial::Domain` and the exclusive `Serial::ByteStreamWriter` capability define a same-domain, exactly-one-provider requirement. The C++20 `ByteWriteEndpoint` concept checks that a Bootstrap-chosen endpoint advertises `CompositionOffers::Contains<ByteStreamWriter>` and has a conformant `noexcept TryWrite` operation. EDP-System has no further EDP dependency; the current dependency graph is acyclic.

The default umbrella imports only the platform-neutral endpoint contracts. Optional headers select their additional owners explicitly. Host tests use a fake provider deriving from `EDP-System::CompositionFramework::Provider`, with exact source/include root controlled by `EDP_SYSTEM_SOURCE_DIR`.

## Optional EDLG FIFO (activated only by explicit header import)

`<serial/BoundedEdlgFrameQueue.hpp>` enables three direct conditional dependencies of **EDP-Serial** in addition to its mandatory EDP-System domain:

- **EDP-Logging** (existing optional pure C++20 canonical EDLG writer, through the previously introduced `EdlgFramePump.hpp`). The default Logging umbrella remains free of Serial; the direction is **Serial→Logging only**, with no reverse edge.
- **EDP-Platform** `Synchronization::SpinLock` abstract Composition provider for very short independently published depth/poison state snapshots (including after the ordinary Mutex Release failed). Its native concrete must be app-selected, and the SpinLock must never be held while waiting, copying frames, or writing UART.
- **EDP-Threading** `OrdinaryMutex<Identity>` abstract Composition provider, with a single native NoWait TryAcquire and typed Release. Threading's owner graph already includes EDP-System, EDP-Platform, EDP-Clock, EDP-Memory and EDP-BoundedTopology as declared in Threading's dependency graph. This does not select a concrete platform Mutex in EDP-Serial.

```text
EDP-Serial (optional FIFO profile)
├── EDP-System [mandatory baseline]
├── EDP-Logging [conditional EDLG framing, no reverse edge]
├── EDP-Platform [conditional short SpinLock for status publication]
│   └── EDP-System
└── EDP-Threading [conditional ordinary-context mutex abstraction]
    ├── EDP-System
    ├── EDP-Platform -> EDP-System
    ├── EDP-Clock -> EDP-Platform, EDP-System
    ├── EDP-BoundedTopology
    └── EDP-Memory -> EDP-Platform, EDP-System, EDP-BoundedTopology
```

The opt-in application manifest must name all **actually activated direct edges**; the default `library.json` remains System-only. No circular dependencies or platform-specific concrete import were added by the FIFO. The previous native atomic<bool> and atomic<size_t> compile-time lock-free assumptions were **both false** under PIOArduino ESP32-WROOM; the caller must inject the existing qualified EDP-Platform SpinLock as the *fourth template argument* instead. The application must declare this direct EDP-Platform edge rather than rely on Threading's transitive import.

## Concrete consumers and other possible conditional graphs

- `EDP-Serial-Arduino -> EDP-Serial` with platform-specific concrete UART/USB provider and its own necessary platform abstraction/SDK edges
- **CURRENT downstream implementation, not future:** `EDP-Serial-ESP-IDF -> EDP-Serial` with an explicit ESP-IDF UART TX FIFO native concrete. This abstract Serial package does not depend on it and does not automatically configure a UART.
- **NOW ACTIVATED CONDITIONALLY**: `EDP-Serial -> EDP-Logging` only for applications importing `<serial/EdlgFramePump.hpp>`. This optional, direct dependency is on the EDP-Logging **abstract EDLG framing source**; its default umbrella and default `library.json` include remain unmodified. No Logging->Serial reverse edge or platform concrete link. Application build configuration MUST explicitly declare both EDP-Serial and EDP-Logging; it MUST NOT infer the optional edge from transitive includes. The rest of the optional activation's EDP tree matches EDP-Logging's **dependency-free default framing** (C++20 standard library only). The frame pump uses native EDP-Serial's EDP-System Composition-qualified ByteWriteEndpoint. No new EDP-Clock/Memory/Threading edge is activated by this header.

Every eventual application Bootstrap must explicitly compose exact provider capability, source ownership, endpoint concurrency, partial-output framing and lifetime. Actual activated optional edges must be re-audited for cycles, transitive dependency trees and build validity.
