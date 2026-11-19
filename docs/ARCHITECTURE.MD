# EDP-Serial architecture and byte endpoint contract

EDP-Serial owns platform-neutral bounded byte-write contracts and optional
complete-frame output composition. Its implementation is on main, version
0.1.0. The [README tutorial](../README.MD#getting-started) introduces the API;
this document defines its endpoint boundary.

## Composition and provider selection

`SerialComposition.hpp` declares an EDP-System Composition domain, the
exclusive ByteStreamWriter capability and the exactly-one, same-domain
ByteStreamWriterRequirement. A conforming provider advertises that offer
and implements noexcept TryWrite(span<const byte>) returning ByteWriteResult.
The ByteWriteEndpoint concept checks the offer and operation signature.

Bootstrap selects the concrete provider, owns its configuration and keeps
it alive for every operation. Serial supplies no global console or implicit
peripheral. EDP-Serial-ESP-IDF/main supplies the separate native UART FIFO
provider; EDP-Serial-Arduino remains a placeholder.

## Accepted-prefix ownership

A provider synchronously consumes or copies the exact accepted prefix before
returning. It never retains the caller's borrowed pointer after return.
Accepted means provider acceptance; on-wire transmission, remote receipt,
flush and durable retention are separate contracts.

TryWriteComplete accepts a finite borrowed span and advances only by validated
provider progress. Accepted must advance at least one byte. WouldBlock must
report zero accepted bytes. Counts above the offered range, unknown status
and illegal progress fail closed with ProviderContractViolation.

The algorithm makes at most bytes.size() provider calls on nonempty input
and returns immediately on WouldBlock or provider failure. A provider
failure can accompany an accepted prefix; its exact count remains observable.
The application decides whether recovery permits another attempt at the
unsent suffix. No rollback or hidden readiness wait is supplied.

## Concurrency and lifetime

The basic endpoint algorithm requires exclusive ownership for its call.
The application must prevent independent writers from interleaving a
multi-call logical frame. DMA/USB implementations retaining source pointers
need a separate lifetime/acknowledgement contract and do not satisfy this
simple synchronous-acceptance boundary.

An EdlgFramePump owns one fixed complete frame across partial writes.
Reserve its endpoint across all attempts until completion or an explicit
transport reset/loss decision. The BoundedEdlgFrameQueue serializes its
participating producers and service context with an existing Threading
ordinary mutex and publishes status through a separate Platform SpinLock.
It creates no worker. Writers bypassing that queue remain outside its
serialization.

## Conditional owner dependencies

The default umbrella consumes only EDP-System. The pump imports Logging's
EdlgFrameWriter.hpp, which consumes EDP-Memory ByteOperations. The queue
also directly imports EDP-Platform, EDP-Threading and EDP-Platform-Portable.
The Portable include remains required with a custom queue byte provider.
See the complete [Dependency Graph](../DEPENDENCY_GRAPH.MD).

## Integration and qualification boundaries

Use [EDLG Frame Pump](EDLG-FRAME-PUMP.MD) and
[Bounded EDLG Frame Queue](BOUNDED-EDLG-FRAME-QUEUE.MD) for their typed
outcomes, resource lifetimes and failure recovery. The
[Wiki build ledger](../wiki/Build-Test-and-Source.md) separates host tests,
target builds and actual application UART evidence.

Physical providers, pins, baud, service timing, interrupt eligibility and
transport recovery require explicit application/native contracts. This
abstract library does not guarantee end-to-end delivery, hard real-time
completion, archive persistence or a generic Arduino transport.
