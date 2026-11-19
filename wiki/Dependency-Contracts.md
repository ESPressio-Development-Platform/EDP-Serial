# Dependency contracts

The default EDP-System/main dependency owns Composition Domain, Provider,
Offers, ExclusiveCapability and exactly-one same-domain Requirement.
Serial reuses these contracts without a second composition framework.

## Optional framing

EdlgFramePump imports Logging's EdlgFrameWriter.hpp. It owns the canonical
EDLG wire representation and consumes Memory's ByteOperations contract.
Begin(codec,payload,byteOperations) borrows the selected copy provider for
that call. Logging's dependency-free umbrella does not make its writer
dependency-free. Serial has the optional Logging edge; Logging has no
reverse Serial edge.

## Optional queue

BoundedEdlgFrameQueue directly imports Platform SpinLock, Threading's
keyed ordinary Mutex and the Portable ByteOperations header. Portable is a
direct include dependency even if the application specializes the fifth
template parameter with a different copy provider. The complete
[recursive dependency graph](../DEPENDENCY_GRAPH.MD) identifies all owners.

The separate short SpinLock protects depth and poison publication.
Frame copying and endpoint output remain outside that short critical
section. The existing ordinary mutex serializes participating queue
operations. Both borrowed providers must outlive the queue. Source and
endpoint lifetime follow the owners' explicit contracts.

## Application provider selection

The [README](../README.MD#getting-started) demonstrates an application-owned
memory endpoint and real Portable/Threading host synchronization.
The bundled three-target queue demo instead uses explicit single-context
demonstration providers. Embedded concurrent applications select native
qualified providers; a host or single-context example supplies no universal
target timing/interrupt guarantee.

EDP-Serial-ESP-IDF/main supplies a native UART FIFO concrete. It depends on
Serial and its SDK; the abstract Serial owner never imports that concrete.
The generic Arduino concrete remains a placeholder. Endpoint configuration,
service scheduling and exclusive port ownership belong to Bootstrap.
