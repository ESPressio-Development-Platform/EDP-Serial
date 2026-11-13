#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include <ESPressio_Serial.hpp>
#include <logging/EdlgFrameWriter.hpp>

namespace ESPressio::Serial {

    /// Startup outcome for an application-owned complete portable EDLG frame.
    enum class EdlgPumpBeginStatus : std::uint8_t {
        Queued = 0U,
        Busy = 1U,
        InvalidCodec = 2U,
        InvalidPayload = 3U
    };

    /// Typed state of one synchronous, bounded native endpoint attempt.
    enum class EdlgPumpStatus : std::uint8_t {
        NoFrame = 0U,
        Complete = 1U,
        WouldBlock = 2U,
        ProviderFailure = 3U,
        ProviderContractViolation = 4U
    };

    struct EdlgPumpResult final {
        EdlgPumpStatus Status;
        std::size_t AcceptedFrameBytes;
        std::size_t TotalFrameBytes;

        /// Reports whether the endpoint accepted all bytes of the owned frame.
        [[nodiscard]] constexpr bool IsComplete() const noexcept {
            return Status == EdlgPumpStatus::Complete;
        }
    };


    /// One fixed-capacity, exclusively owned, complete EDLG frame in flight.
    ///
    /// Keeps all frame bytes in this object's storage across partial writes.
    /// The application must reserve exclusive access to ONE composed native
    /// endpoint until this frame completes. Without endpoint arbitration,
    /// other writers could interleave native bytes between pump attempts.
    ///
    /// Explicitly opt-in to EDP-Logging's abstract canonical EDLG writer:
    /// EDP-Serial core does not include Logging or force a dependency on it.
    template<std::size_t TMaximumPayloadBytes>
    class EdlgFramePump final {
        static_assert(
            TMaximumPayloadBytes <=
                static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()),
            "EDLG payload length must fit the canonical 32-bit wire field"
        );
        static_assert(
            TMaximumPayloadBytes <=
                std::numeric_limits<std::size_t>::max() - 20U,
            "EDLG whole-frame capacity must not overflow size_t"
        );

        std::array<std::byte, TMaximumPayloadBytes + 20U> _buffer{};
        std::size_t _size{0U};
        std::size_t _accepted{0U};
        bool _pending{false};

    public:
        EdlgFramePump() = default;

        EdlgFramePump(const EdlgFramePump&) = delete;
        EdlgFramePump& operator=(const EdlgFramePump&) = delete;
        EdlgFramePump(EdlgFramePump&&) = delete;
        EdlgFramePump& operator=(EdlgFramePump&&) = delete;

        /// Reports whether a complete frame is retained for an endpoint attempt.
        [[nodiscard]] bool IsPending() const noexcept { return _pending; }
        [[nodiscard]] std::size_t AcceptedFrameBytes() const noexcept {
            return _pending ? _accepted : 0U;
        }
        [[nodiscard]] std::size_t TotalFrameBytes() const noexcept {
            return _pending ? _size : 0U;
        }

        /// Copies/frames the payload into exclusive fixed storage on success.
        /// Busy/invalid attempts never alter the already-pending frame.
        /// @tparam TByteOperationsProvider Bootstrap-selected Memory copy provider.
        template<class TByteOperationsProvider>
        [[nodiscard]] EdlgPumpBeginStatus Begin(
            ESPressio::Logging::EdlgCodec codec,
            std::span<const std::byte> encodedPayload,
            const TByteOperationsProvider& byteOperations
        ) noexcept {
            if (_pending) return EdlgPumpBeginStatus::Busy;
            std::size_t written{0U};
            const auto result = ESPressio::Logging::WriteEdlgFrame(
                codec,
                encodedPayload,
                _buffer,
                written,
                TMaximumPayloadBytes,
                byteOperations
            );
            switch (result) {
                case ESPressio::Logging::EdlgFrameWriteResult::Succeeded:
                    _size = written;
                    _accepted = 0U;
                    _pending = true;
                    return EdlgPumpBeginStatus::Queued;

                case ESPressio::Logging::EdlgFrameWriteResult::InvalidCodec:
                    return EdlgPumpBeginStatus::InvalidCodec;

                case ESPressio::Logging::EdlgFrameWriteResult::InvalidPayload:
                    return EdlgPumpBeginStatus::InvalidPayload;

                default:
                    // Since this owns enough space for every valid payload,
                    // insufficient capacity indicates an invariant violation.
                    return EdlgPumpBeginStatus::InvalidPayload;
            }
        }

        /// One bounded native write pass, non-polling on WouldBlock.
        ///
        /// A partial failure retains the exact frame prefix and the unsent
        /// suffix in this object; the caller explicitly decides whether to
        /// retry the same endpoint or abort the stream after diagnostics.
        /// "Complete" means accepted by endpoint, NOT flushed/durable/received.
        template<ByteWriteEndpoint TEndpoint>
        [[nodiscard]] EdlgPumpResult Pump(TEndpoint& endpoint) noexcept {
            if (!_pending) {
                return {EdlgPumpStatus::NoFrame, 0U, 0U};
            }

            const auto remaining = std::span<const std::byte>(
                _buffer.data() + _accepted,
                _size - _accepted
            );
            const auto result = TryWriteComplete(endpoint, remaining);
            _accepted += result.BytesAccepted;

            const auto reportedBytes = _accepted;
            const auto frameBytes = _size;

            switch (result.Status) {
                case WriteAttemptStatus::Complete:
                    _pending = false;
                    _accepted = 0U;
                    _size = 0U;
                    return {
                        EdlgPumpStatus::Complete,
                        reportedBytes,
                        frameBytes
                    };

                case WriteAttemptStatus::WouldBlock:
                    return {
                        EdlgPumpStatus::WouldBlock,
                        reportedBytes,
                        frameBytes
                    };

                case WriteAttemptStatus::ProviderFailure:
                    return {
                        EdlgPumpStatus::ProviderFailure,
                        reportedBytes,
                        frameBytes
                    };

                default:
                    return {
                        EdlgPumpStatus::ProviderContractViolation,
                        reportedBytes,
                        frameBytes
                    };
            }
        }
    };

} // namespace ESPressio::Serial
