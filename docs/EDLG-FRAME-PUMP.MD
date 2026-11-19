# EDLG complete-frame pump

Version0.1.0, implemented on main. Import `serial/EdlgFramePump.hpp`
explicitly. The default Serial umbrella remains System-only. The pump
directly imports Logging's `EdlgFrameWriter.hpp`, whose ByteOperations
contract imposes the transitive Memory dependency. See the complete
[Dependency Graph](../DEPENDENCY_GRAPH.MD) and
[progressive consumer example](../README.MD#4-add-a-complete-edlg-frame).

## State, capacity and admission

`EdlgFramePump<TMaximumPayloadBytes>` owns exactly one
`std::array<std::byte, TMaximumPayloadBytes + 20U>` and its size, accepted
offset and pending flag. It is noncopyable and nonmovable. Capacity must
fit the canonical32-bit payload length and must not overflow size_t.

`Begin(codec, encodedPayload, byteOperations)` borrows an existing
application-selected Memory-compatible byte provider during framing.
Logging's canonical writer copies the whole payload into the pump's fixed
EDLG header/CRC32C/payload/trailer representation before Queued is published.
The provider is not retained. The caller can reuse its source after success.

Busy preserves an existing pending frame. InvalidCodec and InvalidPayload
do not install a new frame. There is no unchecked replacement or abort
operation for a partially emitted frame.

## Output and ownership

`Pump(endpoint)` calls TryWriteComplete on the unaccepted suffix only.
It reports NoFrame, Complete, WouldBlock, ProviderFailure or
ProviderContractViolation, plus cumulative accepted-frame bytes and total
frame bytes. Pending state survives noncomplete output. Complete clears
the pending frame after its entire byte range is accepted by the endpoint.

There is no polling sleep, readiness loop, heap allocation or serializer
duplicate. The byte endpoint must copy or consume accepted bytes before
returning. Complete establishes endpoint acceptance, not physical UART
flush, peer receipt or durable storage.

The application reserves the entire physical endpoint for this frame
across all service attempts. Another writer bypassing the pump can
interleave data. Keep the pump and endpoint alive until completion or a
formally handled transport reset/loss event. A partial provider failure
requires the provider's recovery contract before further use.

## Relationship to generated records and queues

The pump frames already encoded CBOR or JSON. It does not generate or
validate a complete typed LogRecord schema or persist an archive.
[BoundedEdlgFrameQueue](BOUNDED-EDLG-FRAME-QUEUE.MD) adds finite complete-frame
admission and synchronization for participating writers. Concrete
EDP-Serial-ESP-IDF supplies a separate native UART endpoint; generic
Serial-Arduino remains a placeholder.

## Validation

EdlgFramePumpTests exercises copied source lifetime, the canonical golden
frame, partial output, WouldBlock/resume, provider failures and invalid
codec/payload/contract outcomes. Run tests/run_logging_frame_host_tests.sh
with the selected owner root through EDP_SERIAL_DEPENDENCY_ROOT and explicit
EDP_LOGGING_SOURCE_DIR/EDP_SYSTEM_SOURCE_DIR where needed.
The [Wiki ledger](../wiki/Build-Test-and-Source.md) records distinct host,
target and composed physical qualification. No hardware proof follows
from a README program or host fake endpoint.
