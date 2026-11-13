#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <thread>

#include <ESPressio_Logging.hpp>
#include <serial/BoundedEdlgFrameQueue.hpp>

namespace Test {
    namespace Serial = ESPressio::Serial;
    namespace Threading = ESPressio::Threading;
    namespace Logging = ESPressio::Logging;
    namespace Platform = ESPressio::Platform;

    struct QueueIdentity final {};

    struct MutexProvider final : Threading::Framework::Provider<
        Threading::Domain,
        Threading::Framework::Offers<
            Threading::Framework::Offer<Threading::OrdinaryMutex<QueueIdentity>>
        >
    > {
        using Identity = QueueIdentity;
        std::mutex Mutex;
        std::atomic<bool> FailAcquire{false};
        std::atomic<bool> FailRelease{false};

        Threading::OrdinaryMutexAcquireResult TryAcquire() noexcept {
            if (FailAcquire.exchange(false)) {
                return Threading::OrdinaryMutexAcquireResult::ProviderFailure;
            }
            return Mutex.try_lock()
                ? Threading::OrdinaryMutexAcquireResult::Acquired
                : Threading::OrdinaryMutexAcquireResult::TimedOut;
        }
        Threading::OrdinaryMutexReleaseResult Release() noexcept {
            const bool fail = FailRelease.exchange(false);
            Mutex.unlock();
            return fail
                ? Threading::OrdinaryMutexReleaseResult::ProviderFailure
                : Threading::OrdinaryMutexReleaseResult::Released;
        }
    };

    /// Real cross-thread host synchronization for independently published
    /// queue depth and poison, exercising EDP-Platform's existing SpinLock.
    struct StateSpinLock final : Platform::Framework::Provider<
        Platform::Domain,
        Platform::Framework::Offers<
            Platform::Framework::Offer<
                Platform::Synchronization::SpinLock,
                Platform::Framework::PropertyValue<
                    Platform::Synchronization::SpinLockSupportsInterruptContext,
                    false
                >
            >
        >
    > {
        std::mutex Native;

        void Acquire() noexcept { Native.lock(); }

        [[nodiscard]] Platform::Synchronization::SpinLockReleaseResult Release() noexcept {
            Native.unlock();
            return Platform::Synchronization::SpinLockReleaseResult::Released;
        }

        [[nodiscard]] Platform::Synchronization::SpinLockAcquireResult
        AcquireFromInterrupt() noexcept {
            return Platform::Synchronization::SpinLockAcquireResult::UnsupportedInterruptContext;
        }

        [[nodiscard]] Platform::Synchronization::SpinLockReleaseResult
        ReleaseFromInterrupt() noexcept {
            return Platform::Synchronization::SpinLockReleaseResult::UnsupportedInterruptContext;
        }
    };

    struct Endpoint final : Serial::Framework::Provider<
        Serial::Domain,
        Serial::Framework::Offers<
            Serial::Framework::Offer<Serial::ByteStreamWriter>
        >
    > {
        std::array<std::byte,512U> Output{};
        std::size_t Size{0U};
        std::size_t Calls{0U};
        std::size_t MaxThisCall{512U};
        bool BlockNext{false};
        bool FailNext{false};

        Serial::ByteWriteResult TryWrite(
            std::span<const std::byte> bytes
        ) noexcept {
            ++Calls;
            if (BlockNext) {
                BlockNext=false;
                return {Serial::ByteWriteStatus::WouldBlock,0U};
            }
            const auto limit = bytes.size() < MaxThisCall
                ? bytes.size() : MaxThisCall;
            assert(Size + limit <= Output.size());
            for (std::size_t i=0U;i<limit;++i) {
                Output[Size++]=bytes[i];
            }
            if (FailNext) {
                FailNext=false;
                return {Serial::ByteWriteStatus::ProviderFailure,limit};
            }
            return {Serial::ByteWriteStatus::Accepted,limit};
        }
    };

    void AssertValidFrames(std::span<const std::byte> output,
                           const std::array<bool,8U>& expectedIds) {
        std::array<bool,8U> observed{};
        std::size_t offset{0U};
        std::size_t count{0U};
        while(offset<output.size()) {
            Logging::EdlgFrameView view{};
            const auto result=Logging::ReadEdlgFrame(
                output.subspan(offset),view,1U
            );
            assert(result==Logging::EdlgFrameReadResult::Succeeded);
            assert(view.Payload.size()==1U);
            const auto id=std::to_integer<std::uint8_t>(view.Payload[0]);
            assert(id<8U);
            assert(expectedIds[id]);
            assert(!observed[id]);
            observed[id]=true;
            ++count;
            offset += view.Consumed;
        }
        std::size_t expectedCount=0U;
        for (std::size_t i=0U;i<8U;++i) {
            assert(observed[i]==expectedIds[i]);
            expectedCount += expectedIds[i]?1U:0U;
        }
        assert(count==expectedCount);
    }

    void TestPartialHeadNeverInterleavesLaterFrames() {
        MutexProvider mutex;
        StateSpinLock stateSpin;
        Serial::BoundedEdlgFrameQueue<3U,1U,MutexProvider,StateSpinLock> queue(mutex,stateSpin);
        Endpoint endpoint;
        for(std::uint8_t i=0U;i<3U;++i) {
            const std::array<std::byte,1U> payload{std::byte{i}};
            const auto result=queue.TryEnqueue(Logging::EdlgCodec::Cbor,payload);
            assert(result.Status==Serial::EdlgQueueOfferStatus::Queued);
            assert(result.FrameWasQueued);
            assert(result.DepthAfterAttempt==static_cast<std::size_t>(i+1U));
        }
        assert(queue.Depth()==3U);
        const std::array<std::byte,1U> full{std::byte{7U}};
        const auto denied=queue.TryEnqueue(Logging::EdlgCodec::Cbor,full);
        assert(denied.Status==Serial::EdlgQueueOfferStatus::Full);
        assert(!denied.FrameWasQueued);

        endpoint.MaxThisCall=5U;
        endpoint.BlockNext=true;
        const auto first=queue.PumpNext(endpoint);
        assert(first.Status==Serial::EdlgQueuePumpStatus::WouldBlock);
        assert(first.QueueDepth==3U && !first.FrameCompleted);
        endpoint.FailNext=true;
        const auto partialFailure=queue.PumpNext(endpoint);
        assert(partialFailure.Status==Serial::EdlgQueuePumpStatus::ProviderFailure);
        assert(partialFailure.AcceptedFrameBytes==5U);
        assert(queue.Depth()==3U);
        endpoint.MaxThisCall=512U;
        for (std::size_t index=0U;index<3U;++index) {
            const auto result=queue.PumpNext(endpoint);
            assert(result.Status==Serial::EdlgQueuePumpStatus::Complete);
            assert(result.FrameCompleted);
            assert(result.QueueDepth==2U-index);
        }
        assert(queue.Depth()==0U);
        assert(queue.PumpNext(endpoint).Status==Serial::EdlgQueuePumpStatus::Empty);
        AssertValidFrames(
            std::span<const std::byte>(endpoint.Output.data(),endpoint.Size),
            {true,true,true,false,false,false,false,false}
        );
    }

    void TestConcurrentProducersOneFIFO() {
        MutexProvider mutex;
        StateSpinLock stateSpin;
        Serial::BoundedEdlgFrameQueue<8U,1U,MutexProvider,StateSpinLock> queue(mutex,stateSpin);
        Endpoint endpoint;
        std::atomic<unsigned> failures{0U};
        std::array<std::thread,8U> producers;
        for(std::size_t i=0U;i<8U;++i) {
            producers[i]=std::thread([&,i] {
                const std::array<std::byte,1U> payload{
                    std::byte{static_cast<std::uint8_t>(i)}
                };
                for(unsigned attempts=0U;attempts<10000U;++attempts) {
                    const auto status=queue.TryEnqueue(
                        Logging::EdlgCodec::Cbor,payload
                    );
                    if(status.Status==Serial::EdlgQueueOfferStatus::Queued) return;
                    if(status.Status!=Serial::EdlgQueueOfferStatus::LockUnavailable) {
                        ++failures;
                        return;
                    }
                    std::this_thread::yield();
                }
                ++failures;
            });
        }
        for (auto& producer:producers) producer.join();
        assert(failures.load()==0U);
        assert(queue.Depth()==8U);
        for (unsigned i=0U;i<8U;++i) {
            const auto result=queue.PumpNext(endpoint);
            assert(result.FrameCompleted);
            assert(result.Status==Serial::EdlgQueuePumpStatus::Complete);
        }
        assert(queue.Depth()==0U);
        AssertValidFrames(
            std::span<const std::byte>(endpoint.Output.data(),endpoint.Size),
            {true,true,true,true,true,true,true,true}
        );
    }

    void TestNonblockingContentionAndInvalidOffers() {
        MutexProvider mutex;
        StateSpinLock stateSpin;
        Serial::BoundedEdlgFrameQueue<2U,1U,MutexProvider,StateSpinLock> queue(mutex,stateSpin);
        Endpoint endpoint;
        const std::array<std::byte,1U> payload{std::byte{2U}};
        mutex.Mutex.lock();
        assert(queue.TryEnqueue(Logging::EdlgCodec::Cbor,payload).Status ==
            Serial::EdlgQueueOfferStatus::LockUnavailable);
        assert(queue.PumpNext(endpoint).Status ==
            Serial::EdlgQueuePumpStatus::LockUnavailable);
        mutex.Mutex.unlock();
        assert(queue.Depth()==0U);
        const auto invalid=queue.TryEnqueue(
            static_cast<Logging::EdlgCodec>(200U),payload
        );
        assert(invalid.Status==Serial::EdlgQueueOfferStatus::InvalidCodec);
        const std::array<std::byte,2U> excessive{std::byte{1U},std::byte{2U}};
        const auto oversized=queue.TryEnqueue(Logging::EdlgCodec::Cbor,excessive);
        assert(oversized.Status==Serial::EdlgQueueOfferStatus::InvalidPayload);
        assert(queue.Depth()==0U);
    }

    void TestReleaseFailureAfterFrameAcceptedIsReported() {
        MutexProvider mutex;
        StateSpinLock stateSpin;
        Serial::BoundedEdlgFrameQueue<2U,1U,MutexProvider,StateSpinLock> queue(mutex,stateSpin);
        Endpoint endpoint;
        const std::array<std::byte,1U> payload{std::byte{3U}};
        assert(queue.TryEnqueue(Logging::EdlgCodec::Cbor,payload).Status==
            Serial::EdlgQueueOfferStatus::Queued);
        mutex.FailRelease.store(true);
        const auto result=queue.PumpNext(endpoint);
        assert(result.Status==Serial::EdlgQueuePumpStatus::LockReleaseFailed);
        assert(result.FrameCompleted);
        assert(result.AcceptedFrameBytes==21U);
        assert(result.QueueDepth==0U);
        assert(queue.IsPoisoned());
        assert(queue.PumpNext(endpoint).Status==Serial::EdlgQueuePumpStatus::Poisoned);
        AssertValidFrames(
            std::span<const std::byte>(endpoint.Output.data(),endpoint.Size),
            {false,false,false,true,false,false,false,false}
        );
    }

    void TestLockFailurePoisonCannotLoseQueuedFrame() {
        MutexProvider mutex;
        StateSpinLock stateSpin;
        Serial::BoundedEdlgFrameQueue<2U,1U,MutexProvider,StateSpinLock> queue(mutex,stateSpin);
        Endpoint endpoint;
        const std::array<std::byte,1U> payload{std::byte{3U}};
        mutex.FailAcquire.store(true);
        const auto unavailable=queue.TryEnqueue(Logging::EdlgCodec::Cbor,payload);
        assert(unavailable.Status==Serial::EdlgQueueOfferStatus::MutexProviderFailure);
        assert(queue.Depth()==0U);

        mutex.FailRelease.store(true);
        const auto attempted=queue.TryEnqueue(Logging::EdlgCodec::Cbor,payload);
        assert(attempted.Status==Serial::EdlgQueueOfferStatus::QueuedButLockReleaseFailed);
        assert(attempted.FrameWasQueued && queue.Depth()==1U);
        assert(queue.IsPoisoned());
        assert(queue.PumpNext(endpoint).Status==Serial::EdlgQueuePumpStatus::Poisoned);
        assert(queue.TryEnqueue(Logging::EdlgCodec::Cbor,payload).Status==
            Serial::EdlgQueueOfferStatus::Poisoned);
        assert(endpoint.Calls==0U);
        assert(queue.Depth()==1U);
    }
}

static_assert(ESPressio::Serial::ByteWriteEndpoint<Test::Endpoint>);

int main() {
    Test::TestPartialHeadNeverInterleavesLaterFrames();
    for(unsigned iteration=0U;iteration<12U;++iteration) {
        Test::TestConcurrentProducersOneFIFO();
    }
    Test::TestLockFailurePoisonCannotLoseQueuedFrame();
    Test::TestNonblockingContentionAndInvalidOffers();
    Test::TestReleaseFailureAfterFrameAcceptedIsReported();
}
