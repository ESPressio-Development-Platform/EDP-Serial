# EDP-Serial three-platform demo index (0.1.0)

V2 source examples are in the independent `ByteEndpoint`, `EdlgFramePump`, and `BoundedEdlgFrameQueue` directories. Each has **Arduino_IDE**, **PlatformIO_Arduino**, and **PlatformIO_ESP-IDF** entry points, a target manifest and a separate bounded contract/qualification README. All exercise **memory-only fake composed byte providers** to keep the portable Serial owner free of UART/USB hardware assumptions.

- [ByteEndpoint](ByteEndpoint/README.MD) — provider Composition and exact partial acceptance/WouldBlock.
- [EdlgFramePump](EdlgFramePump/README.MD) — canonical Logging EDLG fixed frame and accepted-prefix retry.
- [BoundedEdlgFrameQueue](BoundedEdlgFrameQueue/README.MD) — fixed FIFO, Threading keyed mutex **single-context mock**, full backpressure and complete ordered frames.

These are **build demos**, not real multithreaded native mutex, UART TX wire, host receipt, flash durability or hardware power-cut validation. The app selects and qualifies a real native port/Mutex separately. PlatformIO targets fetch their declared library dependencies from GitHub URLs; `library.json` of core Serial remains System-only and the optional Logging/Threading dependencies are explicit per demo.

Private GitHub dependencies require a Git account with repository access. Configure authentication in Git outside the demo manifest; the URLs contain no credentials.
