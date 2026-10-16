#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <ESPressio_Serial.hpp>

namespace Test {
    namespace Serial = ESPressio::Serial;

    struct Provider final : public Serial::Framework::Provider<
        Serial::Domain,
        Serial::Framework::Offers<
            Serial::Framework::Offer<Serial::ByteStreamWriter>
        >
    > {
        std::array<Serial::ByteWriteResult, 8U> Script{};
        std::size_t Next{0U};
        std::size_t Calls{0U};
        std::size_t Observed{0U};
        bool ObservedEmpty{false};

        Serial::ByteWriteResult TryWrite(
            std::span<const std::byte> bytes
        ) noexcept {
            ++Calls;
            Observed += bytes.size();
            ObservedEmpty |= bytes.empty();
            return Script[Next++];
        }
    };

    void TestEmptyAndComplete() {
        Provider endpoint{};
        std::array<std::byte, 3U> data{
            std::byte{1U},std::byte{2U},std::byte{3U}
        };
        const auto empty = Serial::TryWriteComplete(
            endpoint, std::span<const std::byte>{}
        );
        assert(empty.Completed() && empty.BytesAccepted == 0U);
        assert(endpoint.Calls == 0U);

        endpoint.Script[0] = {Serial::ByteWriteStatus::Accepted, 1U};
        endpoint.Script[1] = {Serial::ByteWriteStatus::Accepted, 2U};
        const auto result = Serial::TryWriteComplete(
            endpoint, std::span<const std::byte>(data)
        );
        assert(result.Completed() && result.BytesAccepted == 3U);
        assert(endpoint.Calls == 2U);
        assert(endpoint.Observed == 3U + 2U);
        assert(!endpoint.ObservedEmpty);
    }

    void TestNoProgressAndPartialFailure() {
        std::array<std::byte, 4U> data{};
        Provider endpoint{};
        endpoint.Script[0] = {Serial::ByteWriteStatus::Accepted, 2U};
        endpoint.Script[1] = {Serial::ByteWriteStatus::WouldBlock, 0U};
        const auto blocked = Serial::TryWriteComplete(
            endpoint, std::span<const std::byte>(data)
        );
        assert(blocked.Status == Serial::WriteAttemptStatus::WouldBlock);
        assert(blocked.BytesAccepted == 2U);
        assert(endpoint.Calls == 2U);

        Provider failed{};
        failed.Script[0] = {Serial::ByteWriteStatus::ProviderFailure, 3U};
        const auto failure = Serial::TryWriteComplete(
            failed, std::span<const std::byte>(data)
        );
        assert(failure.Status == Serial::WriteAttemptStatus::ProviderFailure);
        assert(failure.BytesAccepted == 3U);
        assert(failed.Calls == 1U);
    }

    void TestProviderContractViolations() {
        std::array<std::byte, 2U> data{};
        Provider zero{};
        zero.Script[0] = {Serial::ByteWriteStatus::Accepted, 0U};
        const auto noProgress = Serial::TryWriteComplete(
            zero, std::span<const std::byte>(data)
        );
        assert(noProgress.Status == Serial::WriteAttemptStatus::ProviderContractViolation);
        assert(noProgress.BytesAccepted == 0U && zero.Calls == 1U);

        Provider oversized{};
        oversized.Script[0] = {Serial::ByteWriteStatus::Accepted, 3U};
        const auto tooMany = Serial::TryWriteComplete(
            oversized, std::span<const std::byte>(data)
        );
        assert(tooMany.Status == Serial::WriteAttemptStatus::ProviderContractViolation);
        assert(tooMany.BytesAccepted == 0U);

        Provider illegalBlocked{};
        illegalBlocked.Script[0] = {Serial::ByteWriteStatus::WouldBlock, 1U};
        const auto inconsistent = Serial::TryWriteComplete(
            illegalBlocked, std::span<const std::byte>(data)
        );
        assert(inconsistent.Status == Serial::WriteAttemptStatus::ProviderContractViolation);
        assert(inconsistent.BytesAccepted == 0U);
    }
}

struct UncomposedProvider final {
    Test::Serial::ByteWriteResult TryWrite(
        std::span<const std::byte>
    ) noexcept {
        return {Test::Serial::ByteWriteStatus::WouldBlock, 0U};
    }
};

static_assert(!Test::Serial::ByteWriteEndpoint<UncomposedProvider>);
static_assert(Test::Serial::ByteWriteEndpoint<Test::Provider>);
static_assert(sizeof(Test::Serial::ByteWriteResult) <= 2U * sizeof(std::size_t));

int main() {
    Test::TestEmptyAndComplete();
    Test::TestNoProgressAndPartialFailure();
    Test::TestProviderContractViolations();
}
