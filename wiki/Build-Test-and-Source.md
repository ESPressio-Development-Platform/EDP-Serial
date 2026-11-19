# Serial source, demos and test qualification

The maintained C++20 source units are listed individually in [Reference Index](Reference-Index). Host tests: `CXX=g++ bash tests/run_host_tests.sh`, `CXX=g++ bash tests/run_logging_frame_host_tests.sh` (with `EDP_LOGGING_SOURCE_DIR` and `EDP_SYSTEM_SOURCE_DIR`), `CXX=g++ bash tests/run-bounded-edlg-queue-tests.sh` (with `EDP_SERIAL_DEPENDENCY_ROOT`). Repeat under `CXX=clang++`. The fake provider tests cover exact accepted prefixes, native WouldBlock and errors, zero-progress violation, EDLG CRC/partial frame ownership, FIFO ordering, mutex release poisoning and concurrent host producers.

[Three target demo index](../demos/README.MD): `ByteEndpoint`, `EdlgFramePump`, `BoundedEdlgFrameQueue`, each with Arduino IDE, PlatformIO Arduino-ESP32 and ESP-IDF. They compile/run only on explicitly compatible C++20 SDKs and GitHub-resolved EDP library dependencies. The PIO manifests pin one known PIOArduino revision and resolve explicit GitHub library URLs. The examples are **memory-only** and do not replace actual UART or native RTOS provider tests.

The historical Logging post-main run `20261112T092223-3f2f828e` passed eight companion host checks on the earlier feature revisions. The exact current FIFO run `20261112T114409-376d71e0` passed GCC, Clang and both Arduino-ESP32 and ESP-IDF compile/link targets. Package probe `20261112T114843-3df68cf8` compiled an Arduino-ESP32 consumer using only the Serial package declaration and independently verified the resolved EDP-System `main` source. The FIFO's Arduino IDE sketch is byte-identical to the compiled Arduino demo source. Standalone Arduino IDE and physical UART delivery are distinct validation surfaces.

## Progressive README examples — 19 November 2026

The [consumer tutorial](../README.MD#getting-started) provides a complete
memory endpoint and three host programs: byte-writes.cpp, framed-output.cpp
and queued-output.cpp. The queue composition uses the existing real Portable
Mutex and SpinLock through Threading's OrdinaryMutexProvider. The bundled
three-target demo mocks retain their separately documented scope.

RPI400 run20261119T090815-3ed1ef39 extracted the five actual README C++ blocks
from documentation candidate7ce623c2501625e5aa0aae2611058dfbfaa042b0.
All three programs compiled with strict C++20 warnings and ran successfully
under GCC and Clang: six successful executions. The UART replacement fragment
passed two syntax checks against the existing concrete provider with the
owner's fake SDK header; no native UART execution is implied.

The core, optional pump and bounded queue host regressions also passed under
both compilers: six runner executions. The local Markdown check resolved105
links. Runtime source, tests, demos, manifests and workflows are unchanged
from Serial/main b021ff65fb405937ce2d097d17abe0d59422556e.
The subsequent Wiki ledger update does not change any tutorial code block.

## GitHub-resolved PlatformIO demos

Current demo manifests fetch library dependencies through GitHub lib_deps
URLs. The source/profile migration and exact isolated-build receipts are
tracked in the [Policy handoff](https://github.com/ESPressio-Development-Platform/Policy/blob/main/Handoff/2026-11-19/2026-11-19-09-25-Logging-Estate-Demo-GitHub-Dependencies.MD).
These recipe builds are separate from earlier physical UART/archive
qualification. No fresh hardware execution is implied.

All six GitHub-resolved PlatformIO Serial recipes passed on19 November2026.
ByteEndpoint: Arduino20261119T093103-142a603b and native20261119T093103-166205fd.
Frame-pump and bounded-queue Arduino cases passed within20261119T094437-9c5c966d
before that larger matrix's separate Logging failure. Both native cases
passed within20261119T094437-f381dc01. The proof is per-case markers and
actual linked firmware, with matching current manifest bytes.

The isolated inputs contain demo assets, without a local production library
src tree or sibling checkouts. Public dependencies came directly from
GitHub; private Logging used the appliance's authorized GitHub mirror
transport cache. No credentials enter the manifests. This qualifies the
recipes and selected source closure, without a fresh UART execution claim.
