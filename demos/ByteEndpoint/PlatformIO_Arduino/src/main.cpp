#include <Arduino.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <ESPressio_Serial.hpp>

namespace Demo {

    namespace Serial = ESPressio::Serial;

    // App-composed, memory-only byte endpoint. No UART or transport is opened.
    struct Endpoint final : Serial::Framework::Provider<
        Serial::Domain,
        Serial::Framework::Offers<
            Serial::Framework::Offer<Serial::ByteStreamWriter>
        >
    > {
        std::array<std::byte, 8U> Output{};
        std::size_t Used{0U};

        [[nodiscard]] Serial::ByteWriteResult TryWrite(
            std::span<const std::byte> bytes
        ) noexcept {
            const std::size_t accepted = bytes.size() > 2U ? 2U : bytes.size();
            if (accepted > Output.size() - Used) {
                return {Serial::ByteWriteStatus::ProviderFailure, 0U};
            }
            for (std::size_t index = 0U; index < accepted; ++index) {
                Output[Used++] = bytes[index];
            }
            return {Serial::ByteWriteStatus::Accepted, accepted};
        }
    };

    static_assert(Serial::ByteWriteEndpoint<Endpoint>);

    // Zero is success; inspect the debugger-visible result in the platform app.
    [[nodiscard]] int Run() noexcept {
        Endpoint endpoint;
        const std::array<std::byte, 3U> offered{
            std::byte{1U}, std::byte{2U}, std::byte{3U}
        };
        const auto complete = Serial::TryWriteComplete(endpoint, offered);
        if (!complete.IsComplete() || complete.BytesAccepted != offered.size()) {
            return 1;
        }
        if (endpoint.Used != offered.size()) {
            return 2;
        }
        for (std::size_t index = 0U; index < offered.size(); ++index) {
            if (endpoint.Output[index] != offered[index]) {
                return 3;
            }
        }

        // Genuine WouldBlock must not spin or claim that data was received.
        struct BlockedEndpoint final : Serial::Framework::Provider<
            Serial::Domain,
            Serial::Framework::Offers<
                Serial::Framework::Offer<Serial::ByteStreamWriter>
            >
        > {
            [[nodiscard]] Serial::ByteWriteResult TryWrite(
                std::span<const std::byte>
            ) noexcept {
                return {Serial::ByteWriteStatus::WouldBlock, 0U};
            }
        };
        BlockedEndpoint blocked;
        const auto refused = Serial::TryWriteComplete(blocked, offered);
        return refused.Status == Serial::WriteAttemptStatus::WouldBlock &&
                       refused.BytesAccepted == 0U ? 0 : 4;
    }

} // namespace Demo

/// Debug-visible zero-on-success single-context demo result.
volatile int EdpSerialByteEndpointDemoResult{0};

void setup() {
    EdpSerialByteEndpointDemoResult = Demo::Run();
}

void loop() {}
