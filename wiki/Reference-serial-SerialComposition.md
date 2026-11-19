# serial/SerialComposition.hpp

**PUBLIC EDP-System composition domain and capability declarations**, package version `0.1.0`, branch `main`.

- `Serial::Framework`: alias of `ESPressio::System::CompositionFramework`, never a distinct Composition system.
- `Serial::Domain final : Framework::Domain`: platform-neutral byte endpoint capability domain.
- `Serial::ByteStreamWriter final : Framework::ExclusiveCapability<Domain>`: exclusive nonblocking borrowed-byte endpoint capability.
- `Serial::ByteStreamWriterRequirement`: exact same-domain `Framework::Requirement<ByteStreamWriter, RequirementScope::SameDomain, ExactlyProviders<1U>>`. Bootstrap must select a unique provider advertising this offer before the app instantiates an endpoint.

Concrete backend class examples must derive from `Framework::Provider<Serial::Domain, Framework::Offers<Framework::Offer<Serial::ByteStreamWriter>>>` and also implement `ByteWriteResult TryWrite(std::span<const std::byte>) noexcept`; the compound `Serial::ByteWriteEndpoint<T>` concept rejects providers lacking the declared Composition offer, even if they happen to expose the same method name.

No implementation in the domain imports Arduino, ESP-IDF, USB, UART, EDP-Logging, a global device singleton or platform concrete. A concrete provider must copy/accept its byte prefix without retaining the borrowed caller pointer past return; async DMA providers must instead advertise a separate explicit lifetime/ownership capability. Actual UART/CDC/USB selection and concurrency are application/Bootstrap responsibilities.

[ByteWrite API](Reference-serial-ByteEndpoint) • [Architecture](../docs/ARCHITECTURE.MD) • [Dependencies](../DEPENDENCY_GRAPH.MD).
