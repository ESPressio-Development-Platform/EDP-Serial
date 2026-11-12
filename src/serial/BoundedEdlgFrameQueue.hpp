#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <ESPressio_Threading.hpp>

#include "EdlgFramePump.hpp"

namespace ESPressio::Serial {

    enum class EdlgQueueOfferStatus : std::uint8_t {
        Queued = 0U,
        Full = 1U,
        LockUnavailable = 2U,
        MutexProviderFailure = 3U,
        InvalidCodec = 4U,
        InvalidPayload = 5U,
        QueuedButLockReleaseFailed = 6U,
        Poisoned = 7U
    };

    struct EdlgQueueOfferResult final {
        EdlgQueueOfferStatus Status;
        bool FrameWasQueued;
        std::size_t DepthAfterAttempt;
    };

    enum class EdlgQueuePumpStatus : std::uint8_t {
        Empty = 0U,
        Complete = 1U,
        WouldBlock = 2U,
        ProviderFailure = 3U,
        EndpointContractViolation = 4U,
        LockUnavailable = 5U,
        MutexProviderFailure = 6U,
        LockReleaseFailed = 7U,
        Poisoned = 8U
    };

    struct EdlgQueuePumpResult final {
        EdlgQueuePumpStatus Status;
        bool FrameCompleted;
        std::size_t AcceptedFrameBytes;
        std::size_t TotalFrameBytes;
        std::size_t QueueDepth;
    };


    /// Fixed-capacity FIFO of complete EDLG frames, serialized through a
    /// Bootstrap-selected EDP-Threading *ordinary-context* mutex capability.
    /// No platform concrete, ISR claims, malloc, independently reinvented
    /// EDLG writer or indefinite native mutex acquisition is introduced.
    ///
    /// A single queue exclusively owns the ENTIRE composed byte endpoint
    /// across its PumpNext invocations, including WouldBlock between calls.
    /// A direct write or a second independent queue sharing that endpoint
    /// can still interleave bytes; Bootstrap must prohibit that.
    template<
        std::size_t TFrameCapacity,
        std::size_t TMaximumPayloadBytes,
        class TMutexProvider
    >
    class BoundedEdlgFrameQueue final {
        static_assert(TFrameCapacity > 0U);
        static_assert(std::atomic<std::size_t>::is_always_lock_free);
        static_assert(
            std::is_same_v<
                decltype(std::declval<TMutexProvider&>().TryAcquire()),
                ESPressio::Threading::OrdinaryMutexAcquireResult
            > &&
            noexcept(std::declval<TMutexProvider&>().TryAcquire())
        );
        static_assert(
            std::is_same_v<
                decltype(std::declval<TMutexProvider&>().Release()),
                ESPressio::Threading::OrdinaryMutexReleaseResult
            > &&
            noexcept(std::declval<TMutexProvider&>().Release())
        );
        static_assert(
            TMutexProvider::CompositionOffers::template Contains<
                ESPressio::Threading::OrdinaryMutex<
                    typename TMutexProvider::Identity
                >
            >,
            "Frame queue requires the EDP-Threading composition mutex offer"
        );

        TMutexProvider* _mutex;
        std::array<EdlgFramePump<TMaximumPayloadBytes>,TFrameCapacity> _frames{};
        std::size_t _head{0U};
        std::size_t _tail{0U};
        std::atomic<std::size_t> _depth{0U};
        std::atomic<std::size_t> _poisoned{0U};

        [[nodiscard]] bool ReleaseOrPoison() noexcept {
            if (_mutex->Release() == ESPressio::Threading::OrdinaryMutexReleaseResult::Released) {
                return true;
            }
            _poisoned.store(1U,std::memory_order_release);
            return false;
        }

        [[nodiscard]] static constexpr std::size_t Next(std::size_t index) noexcept {
            return (index + 1U) == TFrameCapacity ? 0U : index + 1U;
        }

    public:
        explicit BoundedEdlgFrameQueue(TMutexProvider& mutex) noexcept :
            _mutex(&mutex) {}

        BoundedEdlgFrameQueue(const BoundedEdlgFrameQueue&) = delete;
        BoundedEdlgFrameQueue& operator=(const BoundedEdlgFrameQueue&) = delete;
        BoundedEdlgFrameQueue(BoundedEdlgFrameQueue&&) = delete;
        BoundedEdlgFrameQueue& operator=(BoundedEdlgFrameQueue&&) = delete;

        [[nodiscard]] std::size_t Depth() const noexcept {
            return _depth.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool Poisoned() const noexcept {
            return _poisoned.load(std::memory_order_acquire) != 0U;
        }

        /// Nonblocking producer offer: one TryAcquire (NoWait) only. A
        /// successfully enqueued frame will remain even if native mutex
        /// Release fails; return an explicit "queued but poisoned" status.
        [[nodiscard]] EdlgQueueOfferResult TryEnqueue(
            ESPressio::Logging::EdlgCodec codec,
            std::span<const std::byte> payload
        ) noexcept {
            if (Poisoned()) {
                return {EdlgQueueOfferStatus::Poisoned,false,Depth()};
            }
            const auto lock = _mutex->TryAcquire();
            if (lock == ESPressio::Threading::OrdinaryMutexAcquireResult::TimedOut) {
                return {EdlgQueueOfferStatus::LockUnavailable,false,Depth()};
            }
            if (lock != ESPressio::Threading::OrdinaryMutexAcquireResult::Acquired) {
                return {EdlgQueueOfferStatus::MutexProviderFailure,false,Depth()};
            }
            // Recheck poison after lock: a previous owner may have poisoned
            // it between the first load and this native acquisition.
            if (Poisoned()) {
                static_cast<void>(ReleaseOrPoison());
                return {EdlgQueueOfferStatus::Poisoned,false,Depth()};
            }

            EdlgQueueOfferStatus status = EdlgQueueOfferStatus::Full;
            bool queued = false;

            if (_depth.load(std::memory_order_relaxed) < TFrameCapacity) {
                switch (_frames[_tail].Begin(codec,payload)) {
                    case EdlgPumpBeginStatus::Queued:
                        _tail = Next(_tail);
                        _depth.fetch_add(1U,std::memory_order_release);
                        queued = true;
                        status = EdlgQueueOfferStatus::Queued;
                        break;
                    case EdlgPumpBeginStatus::InvalidCodec:
                        status = EdlgQueueOfferStatus::InvalidCodec;
                        break;
                    case EdlgPumpBeginStatus::InvalidPayload:
                        status = EdlgQueueOfferStatus::InvalidPayload;
                        break;
                    default:
                        // Busy while a free FIFO slot exists is internal
                        // corruption. Poison rather than misreport capacity.
                        _poisoned.store(1U,std::memory_order_release);
                        status = EdlgQueueOfferStatus::Poisoned;
                        break;
                }
            }

            const auto depth = _depth.load(std::memory_order_relaxed);
            if (!ReleaseOrPoison()) {
                return {
                    queued
                        ? EdlgQueueOfferStatus::QueuedButLockReleaseFailed
                        : EdlgQueueOfferStatus::Poisoned,
                    queued,depth
                };
            }
            return {status,queued,depth};
        }

        /// One ordinary-context worker pass. Keeps the same FIFO head and
        /// complete frame across WouldBlock/partial failure. Native mutex
        /// guards BOTH FIFO mutation and byte output, so participating
        /// producer/worker calls cannot interleave any frame bytes.
        template<ByteWriteEndpoint TEndpoint>
        [[nodiscard]] EdlgQueuePumpResult PumpNext(TEndpoint& endpoint) noexcept {
            if (Poisoned()) {
                return {EdlgQueuePumpStatus::Poisoned,false,0U,0U,Depth()};
            }
            const auto lock = _mutex->TryAcquire();
            if (lock == ESPressio::Threading::OrdinaryMutexAcquireResult::TimedOut) {
                return {EdlgQueuePumpStatus::LockUnavailable,false,0U,0U,Depth()};
            }
            if (lock != ESPressio::Threading::OrdinaryMutexAcquireResult::Acquired) {
                return {EdlgQueuePumpStatus::MutexProviderFailure,false,0U,0U,Depth()};
            }
            if (Poisoned()) {
                static_cast<void>(ReleaseOrPoison());
                return {EdlgQueuePumpStatus::Poisoned,false,0U,0U,Depth()};
            }
            if (_depth.load(std::memory_order_relaxed) == 0U) {
                const auto released = ReleaseOrPoison();
                return {
                    released ? EdlgQueuePumpStatus::Empty
                             : EdlgQueuePumpStatus::LockReleaseFailed,
                    false,0U,0U,0U
                };
            }

            const auto frame = _frames[_head].Pump(endpoint);
            const bool completed = frame.Status == EdlgPumpStatus::Complete;
            if (completed) {
                _head = Next(_head);
                _depth.fetch_sub(1U,std::memory_order_release);
            }

            EdlgQueuePumpStatus resultStatus = EdlgQueuePumpStatus::EndpointContractViolation;
            switch (frame.Status) {
                case EdlgPumpStatus::Complete:
                    resultStatus = EdlgQueuePumpStatus::Complete;
                    break;
                case EdlgPumpStatus::WouldBlock:
                    resultStatus = EdlgQueuePumpStatus::WouldBlock;
                    break;
                case EdlgPumpStatus::ProviderFailure:
                    resultStatus = EdlgQueuePumpStatus::ProviderFailure;
                    break;
                default:
                    resultStatus = EdlgQueuePumpStatus::EndpointContractViolation;
                    break;
            }
            const auto depth = _depth.load(std::memory_order_relaxed);
            if (!ReleaseOrPoison()) {
                resultStatus = EdlgQueuePumpStatus::LockReleaseFailed;
            }
            return {
                resultStatus,
                completed,
                frame.AcceptedFrameBytes,
                frame.TotalFrameBytes,
                depth
            };
        }
    };

} // namespace ESPressio::Serial
