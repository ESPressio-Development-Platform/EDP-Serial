# Internal and cross-repository provider contracts

Although `serial/ByteEndpoint.hpp` is public, its `ByteWriteEndpoint<TEndpoint>` compile-time provider check is a **cross-repository INTERNAL PROVIDER CONTRACT** for separate concrete owners such as EDP-Serial-ESP-IDF. It binds the EDP-System Composition `Offers<ByteStreamWriter>` declaration and exact `noexcept TryWrite(span<const byte>) -> ByteWriteResult`, not merely a coincidental method name. The static Type `Serial::Domain` and unique capability identity must never be silently duplicated by an SDK provider.

The optional queue's Type checks are similarly **internal EDP-Threading integration contracts**: one borrowed `TMutexProvider` with `Identity`, `CompositionOffers::Contains<OrdinaryMutex<Identity>>`, typed `TryAcquire()` and `Release()` result and noexcept; `TryAcquire` is NoWait and must not block. EDP-Logging's canonical EDLG frame writer and reader remain internal cross-repo format contracts for optional Serial frame transport. No alternative CRC or frame format is introduced here.


## Native ESP32 SpinLock-publication boundary (12 November 2026)

The FIFO's **fourth** template parameter `TStateSpinLockProvider` is an explicitly application-owned `EDP-Platform::Synchronization::SpinLock` provider, validated by the existing `Detail::SpinLockProviderTraits` (full C++20 Composition, public `Acquire/Release` and ISR methods). Its sole job is to protect bounded numeric `Depth()` and Boolean `IsPoisoned()` state, even after the ordinary Mutex may have failed releasing; it **must not** guard lengthy native IO or wait. No new atomic API is invented and no `std::atomic<...>::is_always_lock_free` portability assumption remains. First two real PIOArduino source builds proved both standard-library `atomic<bool>` and `atomic<size_t>` compile-time lock-free guarantees false. The provider is selected by application, not created by EDP-Serial.
