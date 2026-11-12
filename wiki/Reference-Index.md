# EDP-Serial exhaustive C++20 source reference index

| Maintained source file | Source-specific reference and classification |
|---|---|
| `src/ESPressio_Serial.hpp` | [Default umbrella](Reference-serial-ESPressio_Serial) — PUBLIC discovery/aggregation |
| `src/serial/SerialComposition.hpp` | [Composition Domain and exclusive ByteStreamWriter](Reference-serial-SerialComposition) — PUBLIC and INTERNAL concrete provider contract |
| `src/serial/ByteEndpoint.hpp` | [Byte endpoint/typed transfer](Reference-serial-ByteEndpoint) — PUBLIC; concept consumed by external providers |
| `src/serial/EdlgFramePump.hpp` | [Optional complete EDLG frame pump](Reference-serial-EdlgFramePump) — PUBLIC optional; private bounded frame state |
| `src/serial/BoundedEdlgFrameQueue.hpp` | [Optional fixed FIFO](Reference-serial-BoundedEdlgFrameQueue) — PUBLIC optional; internal Threading provider and private FIFO/mutex state |

There are exactly five current production C++ source headers, each with one explicit local reference. [Architecture](Architecture), [Dependency Contracts](Dependency-Contracts), [Resources](Resources-and-Lifecycle), [Compiler Definitions](Compiler-Definitions) and [Tooling](Tooling-Reference) describe their shared cross-domain invariants. Do not invent an Arduino/ESP-IDF concrete backend inside these headers.
