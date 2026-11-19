# EDP-Serial dependency graph

Version 0.1.0. Dependencies follow the headers selected by the application.
The default package manifest declares EDP-System; optional headers require
the additional owner closure below.

## Direct activation

| Direct owner | Condition | Contract used |
| --- | --- | --- |
| EDP-System | Always, through `ESPressio_Serial.hpp` | Serial domain, provider capability and Composition requirements |
| EDP-Logging | Import `serial/EdlgFramePump.hpp` or `serial/BoundedEdlgFrameQueue.hpp` | `logging/EdlgFrameWriter.hpp`, EDLG framing and codec vocabulary |
| EDP-Platform | Import `serial/BoundedEdlgFrameQueue.hpp` | Short independent SpinLock contract for status publication |
| EDP-Threading | Import `serial/BoundedEdlgFrameQueue.hpp` | Ordinary keyed Mutex contract for frame and endpoint ownership |
| EDP-Platform-Portable | Import `serial/BoundedEdlgFrameQueue.hpp` | Default stateless byte-copy provider, included directly by the header |

The Portable header is included unconditionally inside the optional queue
header. Supplying a custom fifth template argument changes the selected
byte provider but does not remove that include dependency.
EDP-Memory is reached through Logging's optional writer and Portable;
it is not an additional direct source import of EDP-Serial.

## Complete EDP closure

The tree shows the union of the two optional profiles. Without the pump
or queue, only the mandatory System edge applies.

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

EDP-System and EDP-BoundedTopology have no additional EDP dependency in
this selected closure. The graph is acyclic. Header discovery is not a
substitute for declaring every activated direct owner in the consuming
application's build configuration.

## Application-selected concrete providers

The abstract Serial owner imports no concrete UART/USB provider.
EDP-Serial-ESP-IDF depends on EDP-Serial and its native ESP-IDF
`esp_driver_uart` SDK component. A generic EDP-Serial-Arduino concrete
remains unimplemented. Neither creates a reverse abstract dependency.

The [README host tutorial](./README.MD#getting-started) directly selects
Portable byte operations for its memory endpoint. Its queue program also
selects the existing Portable Mutex and SpinLock through the Threading
ordinary-mutex adapter. Those are application choices. Embedded applications
select their qualified native providers explicitly.

Logging's minimal umbrella has no EDP dependencies, but the pump imports
its separate writer header, whose Memory dependency is imposed. The edge
is Serial to Logging; Logging has no reverse Serial dependency.

## Ownership across these edges

The endpoint must copy or consume every accepted byte before returning.
The frame pump owns its framed copy across partial writes. The queue
borrows one keyed ordinary mutex and a separate short SpinLock; both must
outlive it. Frame copying and endpoint I/O occur outside the short state
SpinLock. The application owns service scheduling and exclusive access to
the physical endpoint across all participating writers.

See [Architecture](./docs/ARCHITECTURE.MD), [frame pump](./docs/EDLG-FRAME-PUMP.MD),
[bounded queue](./docs/BOUNDED-EDLG-FRAME-QUEUE.MD) and
[Wiki Dependency Contracts](./wiki/Dependency-Contracts.md).
