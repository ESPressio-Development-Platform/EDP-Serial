# serial/ByteEndpoint.hpp

**PUBLIC** allocation-free C++20 Serial abstraction; single umbrella `<ESPressio_Serial.hpp>`. Namespace `ESPressio::Serial`. EDP-System's static Composition Framework is the **sole direct** EDP dependency.

- `ByteWriteStatus : std::uint8_t`: `Accepted=0`, `WouldBlock=1`, `ProviderFailure=2`.
- `ByteWriteResult`: `Status` and `BytesAccepted`. A concrete provider must never claim more than the offered bytes, may report partial acceptance (even with ProviderFailure), must report zero when WouldBlock, and must progress at least once before returning Accepted. No delivery/durability guarantees.
- `Serial::Domain`, exclusive `Serial::ByteStreamWriter` and `ByteStreamWriterRequirement` (exactly one same-domain EDP-System Composition provider); declared in `serial/SerialComposition.hpp`.
- `ByteWriteEndpoint<TEndpoint>`: compile-time CompositionOffers membership and `noexcept` `TryWrite(std::span<const std::byte>)` returns exactly `ByteWriteResult`. Requires caller-composed concrete provider and stable input.
- `WriteAttemptStatus : std::uint8_t`: `Complete=0`, `WouldBlock=1`, `ProviderFailure=2`, `ProviderContractViolation=3`.
- `WriteAttemptResult`: `Status`, exact **validated** `BytesAccepted` prefix and `IsComplete() const noexcept`.
- `TryWriteComplete<TEndpoint>(endpoint,bytes) noexcept`: one finite transfer pass. Each accepted call reduces remaining bytes; count of calls at most input byte count, except zero length invokes none. Returns immediately on WouldBlock, provider failure or invalid provider report. Captures already accepted bytes even when last provider response reports failure. Never retries WouldBlock or sleeps. No frame-level multi-producer locking or guarantee of remote receipt.

The API makes **no** assumptions about UART hardware, USB/CDC, ISR access, DMA buffer ownership, clock deadlines, platform target, logging schema, frame atomicity, and does not claim data was flushed. Caller must handle both partial prefix and later retry; do not interleave with another writer until the complete frame is accounted for.

**Tests:** `tests/ByteEndpointTests.cpp`, runner `tests/run_host_tests.sh`. Strict host C++20 fake providers verify empty, multi-step partial, WouldBlock, partial failure, no-progress Accepted and oversized/malformed results. Target firmware and real Serial backend unavailable.
