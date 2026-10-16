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
- A Logging sink adapter may be a downstream consumer `EDP-Serial -> EDP-Logging` as an **optional conditional** dependency only after its own separate design/implementation is approved; the Logging core must **never** depend on EDP-Serial

Every eventual application Bootstrap must explicitly compose exact provider capability, source ownership, endpoint concurrency, partial-output framing and lifetime. Actual activated optional edges must be re-audited for cycles, transitive dependency trees and build validity.
