#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <ESPressio_Serial.hpp>
#include <serial/EdlgFramePump.hpp>
#include <logging/EdlgFrameReader.hpp>

namespace Demo {

    namespace Serial = ESPressio::Serial;
    namespace Logging = ESPressio::Logging;

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
            const std::size_t count = bytes.size() > 5U ? 5U : bytes.size();
            if (count > Output.size() - Used) {
                return {Serial::ByteWriteStatus::ProviderFailure, 0U};
            }
            for (std::size_t i = 0U; i < count; ++i) {
                Output[Used++] = bytes[i];
            }
            return {Serial::ByteWriteStatus::Accepted, count};
        }
    };

    static_assert(Serial::ByteWriteEndpoint<Endpoint>);

    [[nodiscard]] int Run() noexcept {
        Serial::EdlgFramePump<16U> pump;
        const std::array<std::byte, 2U> illustrativeJson{
            std::byte{'{'}, std::byte{'}'}
        };
        if (pump.Begin(Logging::EdlgCodec::Json, illustrativeJson) !=
            Serial::EdlgPumpBeginStatus::Queued) {
            return 1;
        }
        Endpoint endpoint;
        const auto initial = pump.Pump(endpoint);
        if (initial.Status != Serial::EdlgPumpStatus::WouldBlock ||
            !pump.Pending() || initial.AcceptedFrameBytes != 0U) {
            return 2;
        }
        const auto result = pump.Pump(endpoint);
        if (!result.Completed() || pump.Pending() ||
            result.AcceptedFrameBytes != 22U || endpoint.Used != 22U) {
            return 3;
        }
        Logging::EdlgFrameView frame{};
        if (Logging::ReadEdlgFrame(
            std::span<const std::byte>(endpoint.Output.data(), endpoint.Used),
            frame, 16U
        ) != Logging::EdlgFrameReadResult::Succeeded) {
            return 4;
        }
        return frame.Codec == Logging::EdlgCodec::Json &&
                       frame.Payload.size() == 2U &&
                       frame.Consumed == 22U ? 0 : 5;
    }

} // namespace Demo

/// Debug-visible zero-on-success single-context demo result.
volatile int EdpSerialEdlgFramePumpDemoResult{0};

extern "C" void app_main() {
    EdpSerialEdlgFramePumpDemoResult = Demo::Run();
}
