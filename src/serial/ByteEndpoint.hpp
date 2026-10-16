#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "SerialComposition.hpp"

namespace ESPressio::Serial {

    /// Result reported by a Bootstrap-selected, non-blocking byte endpoint.
    enum class ByteWriteStatus : std::uint8_t {
        Accepted = 0U,
        WouldBlock = 1U,
        ProviderFailure = 2U
    };

    /// Exact provider progress; accepted bytes are NOT necessarily received,
    /// flushed or durable at a remote device.
    struct ByteWriteResult final {
        ByteWriteStatus Status{ByteWriteStatus::WouldBlock};
        std::size_t BytesAccepted{0U};
    };

    /// Validates only the abstract app-composed, explicitly nonblocking endpoint.
    template<class TEndpoint>
    concept ByteWriteEndpoint = requires(
        TEndpoint& endpoint,
        std::span<const std::byte> bytes
    ) {
        typename TEndpoint::CompositionOffers;
        requires TEndpoint::CompositionOffers::template Contains<ByteStreamWriter>;
        { endpoint.TryWrite(bytes) } noexcept -> std::same_as<ByteWriteResult>;
    };

    /// Whole-buffer bounded attempt status, with truthful partial progress.
    enum class WriteAttemptStatus : std::uint8_t {
        Complete = 0U,
        WouldBlock = 1U,
        ProviderFailure = 2U,
        ProviderContractViolation = 3U
    };

    struct WriteAttemptResult final {
        WriteAttemptStatus Status{WriteAttemptStatus::Complete};
        std::size_t BytesAccepted{0U};

        [[nodiscard]] constexpr bool Completed() const noexcept {
            return Status == WriteAttemptStatus::Complete;
        }
    };

    /// Attempts to consume one borrowed byte range without indefinite retry.
    ///
    /// Exactly one unique producer must own this endpoint during the call;
    /// the function is NOT a frame-atomic multiproducer/Console guard. A
    /// caller-selected EDP-Threading coordination contract is needed there.
    ///
    /// The maximum number of provider calls is bounded by bytes.size():
    /// each Accepted result MUST advance by at least one byte, otherwise
    /// progress stops with ProviderContractViolation. WouldBlock/Failure
    /// terminate immediately without internal sleeps or polling.
    ///
    /// On error, BytesAccepted reports the exact prefix accepted, even
    /// if a provider failed after partially accepting its last offered range.
    template<ByteWriteEndpoint TEndpoint>
    [[nodiscard]] WriteAttemptResult TryWriteComplete(
        TEndpoint& endpoint,
        std::span<const std::byte> bytes
    ) noexcept {
        std::size_t accepted = 0U;
        while (accepted < bytes.size()) {
            const auto remaining = bytes.subspan(accepted);
            const auto attempt = endpoint.TryWrite(remaining);

            if (attempt.BytesAccepted > remaining.size()) {
                return {WriteAttemptStatus::ProviderContractViolation, accepted};
            }

            if (
                attempt.Status == ByteWriteStatus::WouldBlock &&
                attempt.BytesAccepted != 0U
            ) {
                return {WriteAttemptStatus::ProviderContractViolation, accepted};
            }

            // The provider may have accepted a prefix before reporting failure.
            // Preserve it, rather than pretending the write was transactional.
            accepted += attempt.BytesAccepted;

            switch (attempt.Status) {
                case ByteWriteStatus::Accepted:
                    if (attempt.BytesAccepted == 0U) {
                        return {WriteAttemptStatus::ProviderContractViolation, accepted};
                    }
                    break;

                case ByteWriteStatus::WouldBlock:
                    return {WriteAttemptStatus::WouldBlock, accepted};

                case ByteWriteStatus::ProviderFailure:
                    return {WriteAttemptStatus::ProviderFailure, accepted};

                default:
                    return {WriteAttemptStatus::ProviderContractViolation, accepted};
            }
        }

        return {WriteAttemptStatus::Complete, accepted};
    }

} // namespace ESPressio::Serial
