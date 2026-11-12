# Composition and provider wiring

Bootstrap selects exactly one `Serial::ByteStreamWriter` for one `Serial::Domain`, using the existing EDP-System `Framework::Provider<Domain,Framework::Offers<Framework::Offer<ByteStreamWriter>>>` shape. The selected concrete must implement `ByteWriteResult TryWrite(span<const byte>) noexcept` and copy/accept any offered prefix before returning; it may never retain the borrowed bytes past the call. No EDP-Serial static global endpoint is invented.

To add the optional complete EDLG FIFO, Bootstrap separately chooses a real EDP-Threading `OrdinaryMutex<Identity>` concrete, initializes/owns it before the queue, enforces exclusive access to the same physical UART/USB endpoint for *all* writers, and keeps all providers alive until the FIFO is empty and active tasks joined. The example `ExampleMutex` used in Arduino/PIO source exercises the *shape* but **does not provide cross-task memory exclusion**; production code must not reuse it with multiple Tasks.

A separate [EDP-Serial-ESP-IDF](https://github.com/ESPressio-Development-Platform/EDP-Serial-ESP-IDF) provider maps one app-configured UART TX FIFO to `ByteStreamWriter`; its `Accepted` is only FIFO acceptance. It never certifies over-wire delivery, CDC/USB framing, UART drain or persistence.
