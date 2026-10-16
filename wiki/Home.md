# EDP-Serial Wiki

**Version 0.1.0**, EDP-System Composition-aware abstraction feature branch `feature/logging-v1`, **abstract contract only**. [Byte Endpoint reference](Reference-serial-ByteEndpoint), [optional EDLG frame pump](Reference-serial-EdlgFramePump), and [optional bounded EDLG FIFO](Reference-serial-BoundedEdlgFrameQueue) and [Composition contracts](Reference-serial-SerialComposition) covers exact result/status invariants, accepted-prefix semantics and finite retry limits. [Architecture](../docs/ARCHITECTURE.MD) and [Dependency Graph](../DEPENDENCY_GRAPH.MD) describe the current and *unimplemented future* concrete-provider graph. [Host Tests](../tests/run_host_tests.sh) run strict native GCC/Clang C++20 fake endpoint contract tests.

No Arduino/ESP-IDF target endpoints, GPIO/UART bootstrap, EDLG Logging adapter or on-device performance proof exists on this feature branch.
