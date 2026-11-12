# Public Serial C++20 API

The [Byte Endpoint](Reference-serial-ByteEndpoint) declares `ByteWriteStatus` (Accepted/WouldBlock/ProviderFailure), `ByteWriteResult` with exact `Status` and `BytesAccepted`, `ByteWriteEndpoint<TEndpoint>` requiring Composition offer and noexcept one-call `TryWrite`, `WriteAttemptStatus` (Complete/WouldBlock/ProviderFailure/ProviderContractViolation), `WriteAttemptResult` with `Completed()`, and `TryWriteComplete(endpoint, borrowedBytes)` with finite prefix-preserving progress.

The [Composition](Reference-serial-SerialComposition) declares `Domain`, `ByteStreamWriter`, `ByteStreamWriterRequirement`, and a Framework namespace alias; the application's exact one-provider selection is part of the public contract.

Opt-in [Edlg Frame Pump](Reference-serial-EdlgFramePump) declares `EdlgPumpBeginStatus`, `EdlgPumpStatus`, `EdlgPumpResult` and `EdlgFramePump<TMaximumPayloadBytes>` with `Begin`, `Pump`, `Pending`, `AcceptedFrameBytes`, `TotalFrameBytes`. Opt-in [Bounded FIFO](Reference-serial-BoundedEdlgFrameQueue) declares both typed offer/pump statuses/results, `BoundedEdlgFrameQueue<TFrameCapacity,TMaximumPayloadBytes,TMutexProvider>` and `TryEnqueue`, `PumpNext`, `Depth`, `Poisoned`. All failures must preserve truthful provider/member state; a successful enqueue is not delivery or durability.
