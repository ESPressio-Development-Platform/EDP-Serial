# EDP-Serial Dependency Graph

**Active branch:** `feature/logging-v1`; package version **0.1.0**.

## Current full mandatory dependency tree

```text
EDP-Serial
└── EDP-System [mandatory]
```

The **only direct EDP dependency** is EDP-System's static Composition Framework. `Serial::Domain` and the exclusive `Serial::ByteStreamWriter` capability define a same-domain, exactly-one-provider requirement. The C++20 `ByteWriteEndpoint` concept checks that a Bootstrap-chosen endpoint advertises `CompositionOffers::Contains<ByteStreamWriter>` and has a conformant `noexcept TryWrite` operation. EDP-System has no further EDP dependency; the current dependency graph is acyclic.

No EDP-Logging, EDP-Memory, EDP-Threading, platform SDK, Arduino/ESP-IDF concrete or deprecated `ESPressio-*` repository is consumed by this domain. Host tests use a fake provider deriving from `EDP-System::CompositionFramework::Provider`, with exact source/include root controlled by `EDP_SYSTEM_SOURCE_DIR`.

## Future conditional graphs — NOT activated by current code

- `EDP-Serial-Arduino -> EDP-Serial` with platform-specific concrete UART/USB provider and its own necessary platform abstraction/SDK edges
- `EDP-Serial-ESP-IDF -> EDP-Serial` with platform-specific concrete UART/USB provider
- **NOW ACTIVATED CONDITIONALLY**: `EDP-Serial -> EDP-Logging` only for applications importing `<serial/EdlgFramePump.hpp>`. This optional, direct dependency is on the EDP-Logging **abstract EDLG framing source**; its default umbrella and default `library.json` include remain unmodified. No Logging->Serial reverse edge or platform concrete link. Application build configuration MUST explicitly declare both EDP-Serial and EDP-Logging; it MUST NOT infer the optional edge from transitive includes. The rest of the optional activation's EDP tree matches EDP-Logging's **dependency-free default framing** (C++20 standard library only). The frame pump uses native EDP-Serial's EDP-System Composition-qualified ByteWriteEndpoint. No new EDP-Clock/Memory/Threading edge is activated by this header.

Every eventual application Bootstrap must explicitly compose exact provider capability, source ownership, endpoint concurrency, partial-output framing and lifetime. Actual activated optional edges must be re-audited for cycles, transitive dependency trees and build validity.
