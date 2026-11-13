#include <Arduino.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>

#include <ESPressio_Serial.hpp>
#include <serial/BoundedEdlgFrameQueue.hpp>
#include <logging/EdlgFrameReader.hpp>

namespace Demo {

    namespace Serial = ESPressio::Serial;
    namespace Logging = ESPressio::Logging;
    namespace Threading = ESPressio::Threading;

    struct MutexIdentity final {};

    // Single-context demonstration of the composition contract ONLY.
    // A concurrent application must supply a real Platform/Threading Mutex.
    struct ExampleMutex final : Threading::Framework::Provider<
        Threading::Domain,
        Threading::Framework::Offers<
            Threading::Framework::Offer<
                Threading::OrdinaryMutex<MutexIdentity>
            >
        >
    > {
        using Identity = MutexIdentity;
        bool Held{false};

        [[nodiscard]] Threading::OrdinaryMutexAcquireResult TryAcquire() noexcept {
            if (Held) {
                return Threading::OrdinaryMutexAcquireResult::TimedOut;
            }
            Held = true;
            return Threading::OrdinaryMutexAcquireResult::Acquired;
        }

        [[nodiscard]] Threading::OrdinaryMutexReleaseResult Release() noexcept {
            if (!Held) {
                return Threading::OrdinaryMutexReleaseResult::ProviderFailure;
            }
            Held = false;
            return Threading::OrdinaryMutexReleaseResult::Released;
        }
    };

    // Single-context fake Platform SpinLock: NOT a real concurrent provider.
    struct ExampleStateSpinLock final : ESPressio::Platform::Framework::Provider<
        ESPressio::Platform::Domain,
        ESPressio::Platform::Framework::Offers<
            ESPressio::Platform::Framework::Offer<
                ESPressio::Platform::Synchronization::SpinLock,
                ESPressio::Platform::Framework::PropertyValue<
                    ESPressio::Platform::Synchronization::SpinLockSupportsInterruptContext,
                    false
                >
            >
        >
    > {
        bool Held{false};

        ExampleStateSpinLock() = default;
        ExampleStateSpinLock(const ExampleStateSpinLock&) = delete;
        ExampleStateSpinLock& operator=(const ExampleStateSpinLock&) = delete;
        ExampleStateSpinLock(ExampleStateSpinLock&&) = delete;
        ExampleStateSpinLock& operator=(ExampleStateSpinLock&&) = delete;

        void Acquire() noexcept {
            if (Held) { std::terminate(); }
            Held = true;
        }
        [[nodiscard]] ESPressio::Platform::Synchronization::SpinLockReleaseResult
        Release() noexcept {
            if (!Held) { std::terminate(); }
            Held = false;
            return ESPressio::Platform::Synchronization::SpinLockReleaseResult::Released;
        }
        [[nodiscard]] ESPressio::Platform::Synchronization::SpinLockAcquireResult
        AcquireFromInterrupt() noexcept {
            return ESPressio::Platform::Synchronization::SpinLockAcquireResult::UnsupportedInterruptContext;
        }
        [[nodiscard]] ESPressio::Platform::Synchronization::SpinLockReleaseResult
        ReleaseFromInterrupt() noexcept {
            return ESPressio::Platform::Synchronization::SpinLockReleaseResult::UnsupportedInterruptContext;
        }
    };
    struct Endpoint final : Serial::Framework::Provider<
        Serial::Domain,
        Serial::Framework::Offers<
            Serial::Framework::Offer<Serial::ByteStreamWriter>
        >
    > {
        std::array<std::byte, 64U> Output{};
        std::size_t Used{0U};
        bool BlockNext{true};

        [[nodiscard]] Serial::ByteWriteResult TryWrite(
            std::span<const std::byte> bytes
        ) noexcept {
            if (BlockNext) {
                BlockNext = false;
                return {Serial::ByteWriteStatus::WouldBlock, 0U};
            }
            const std::size_t accepted = bytes.size() > 4U ? 4U : bytes.size();
            if (accepted > Output.size() - Used) {
                return {Serial::ByteWriteStatus::ProviderFailure, 0U};
            }
            for (std::size_t i = 0U; i < accepted; ++i) {
                Output[Used++] = bytes[i];
            }
            return {Serial::ByteWriteStatus::Accepted, accepted};
        }
    };

    static_assert(Serial::ByteWriteEndpoint<Endpoint>);

    [[nodiscard]] int Run() noexcept {
        ExampleMutex mutex;
        ExampleStateSpinLock stateSpin;
        Serial::BoundedEdlgFrameQueue<
            2U, 2U, ExampleMutex, ExampleStateSpinLock
        > queue(mutex,stateSpin);
        Endpoint endpoint;
        const std::array<std::byte, 2U> first{
            std::byte{'{'}, std::byte{'}'}
        };
        const std::array<std::byte, 2U> second{
            std::byte{'['}, std::byte{']'}
        };
        if (queue.TryEnqueue(Logging::EdlgCodec::Json, first).Status !=
                Serial::EdlgQueueOfferStatus::Queued ||
            queue.TryEnqueue(Logging::EdlgCodec::Json, second).Status !=
                Serial::EdlgQueueOfferStatus::Queued) {
            return 1;
        }
        if (queue.TryEnqueue(Logging::EdlgCodec::Json, first).Status !=
            Serial::EdlgQueueOfferStatus::Full) {
            return 2;
        }
        const auto initial = queue.PumpNext(endpoint);
        if (initial.Status != Serial::EdlgQueuePumpStatus::WouldBlock ||
            queue.Depth() != 2U) {
            return 3;
        }
        for (std::size_t i = 0U; i < 2U; ++i) {
            const auto outcome = queue.PumpNext(endpoint);
            if (outcome.Status != Serial::EdlgQueuePumpStatus::Complete ||
                !outcome.FrameCompleted) {
                return 4;
            }
        }
        if (queue.Depth() != 0U || queue.IsPoisoned()) {
            return 5;
        }
        const std::span<const std::byte> stream(endpoint.Output.data(), endpoint.Used);
        std::size_t offset = 0U;
        for (std::size_t i = 0U; i < 2U; ++i) {
            Logging::EdlgFrameView parsed{};
            if (Logging::ReadEdlgFrame(stream.subspan(offset), parsed, 2U) !=
                Logging::EdlgFrameReadResult::Succeeded ||
                parsed.Consumed != 22U) {
                return 6;
            }
            if (parsed.Payload[0U] != (i == 0U ? first[0U] : second[0U])) {
                return 7;
            }
            offset += parsed.Consumed;
        }
        return offset == stream.size() ? 0 : 8;
    }

} // namespace Demo

/// Debug-visible zero-on-success single-context demo result.
volatile int EdpSerialBoundedEdlgFrameQueueDemoResult{0};

void setup() {
    EdpSerialBoundedEdlgFrameQueueDemoResult = Demo::Run();
}

void loop() {}
