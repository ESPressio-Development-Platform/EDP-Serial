#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <serial/EdlgFramePump.hpp>

namespace Test {

    namespace Serial = ESPressio::Serial;
    namespace Logging = ESPressio::Logging;

    struct Endpoint final : Serial::Framework::Provider<
        Serial::Domain,
        Serial::Framework::Offers<
            Serial::Framework::Offer<Serial::ByteStreamWriter>
        >
    > {
        std::array<Serial::ByteWriteResult, 12U> Script{};
        std::array<std::byte, 128U> Output{};
        std::size_t ScriptIndex{0U};
        std::size_t Accepted{0U};
        std::size_t Calls{0U};

        Serial::ByteWriteResult TryWrite(
            std::span<const std::byte> offered
        ) noexcept {
            ++Calls;
            const auto result = Script[ScriptIndex++];
            const auto copy = result.BytesAccepted < offered.size()
                ? result.BytesAccepted : offered.size();
            assert(Accepted + copy <= Output.size());
            for (std::size_t index=0U; index<copy; ++index) {
                Output[Accepted++] = offered[index];
            }
            return result;
        }
    };

    void TestPartialFrameAndNoInterleaving() {
        Serial::EdlgFramePump<16U> pump;
        Endpoint endpoint{};
        auto empty = pump.Pump(endpoint);
        assert(empty.Status == Serial::EdlgPumpStatus::NoFrame);
        assert(endpoint.Calls == 0U);

        std::array<std::byte,3U> payload{
            std::byte{'a'},std::byte{'b'},std::byte{'c'}
        };
        assert(pump.Begin(Logging::EdlgCodec::Cbor,payload) ==
            Serial::EdlgPumpBeginStatus::Queued);
        assert(pump.IsPending() && pump.TotalFrameBytes() == 23U);
        // Must be a copy: mutation after Begin never changes the wire frame.
        payload[0]=std::byte{'x'};

        endpoint.Script[0]={Serial::ByteWriteStatus::Accepted,7U};
        endpoint.Script[1]={Serial::ByteWriteStatus::WouldBlock,0U};
        auto first=pump.Pump(endpoint);
        assert(first.Status == Serial::EdlgPumpStatus::WouldBlock);
        assert(first.AcceptedFrameBytes == 7U);
        assert(first.TotalFrameBytes == 23U);
        assert(pump.AcceptedFrameBytes() == 7U);

        std::array<std::byte,1U> next{std::byte{0x55}};
        assert(pump.Begin(Logging::EdlgCodec::Cbor,next) ==
            Serial::EdlgPumpBeginStatus::Busy);
        assert(pump.AcceptedFrameBytes() == 7U);

        endpoint.Script[2]={Serial::ByteWriteStatus::Accepted,16U};
        auto second=pump.Pump(endpoint);
        assert(second.IsComplete());
        assert(second.AcceptedFrameBytes == 23U && second.TotalFrameBytes == 23U);
        assert(!pump.IsPending());
        assert(endpoint.Accepted == 23U && endpoint.Calls == 3U);

        const std::array<std::uint8_t,23U> golden{
            0x45U,0x44U,0x4CU,0x47U,0x01U,0x01U,0x00U,0x00U,
            0x03U,0x00U,0x00U,0x00U,0x80U,0x63U,0x7FU,0xF5U,
            0x61U,0x62U,0x63U,0x47U,0x4CU,0x44U,0x45U
        };
        for(std::size_t i=0U;i<golden.size();++i) {
            assert(std::to_integer<std::uint8_t>(endpoint.Output[i]) == golden[i]);
        }

        assert(pump.Begin(Logging::EdlgCodec::Cbor,next) ==
            Serial::EdlgPumpBeginStatus::Queued);
    }

    void TestPartialFailureRetainsPrefixAndFrame() {
        Serial::EdlgFramePump<16U> pump;
        Endpoint endpoint{};
        const std::array<std::byte,1U> payload{std::byte{0xA0U}};
        assert(pump.Begin(Logging::EdlgCodec::Cbor,payload) ==
            Serial::EdlgPumpBeginStatus::Queued);
        endpoint.Script[0]={Serial::ByteWriteStatus::ProviderFailure,5U};
        const auto failed=pump.Pump(endpoint);
        assert(failed.Status == Serial::EdlgPumpStatus::ProviderFailure);
        assert(failed.AcceptedFrameBytes==5U);
        assert(pump.IsPending());

        endpoint.Script[1]={Serial::ByteWriteStatus::Accepted,0U};
        const auto contract=pump.Pump(endpoint);
        assert(contract.Status == Serial::EdlgPumpStatus::ProviderContractViolation);
        assert(contract.AcceptedFrameBytes == 5U);
        assert(pump.IsPending());

        endpoint.Script[2]={Serial::ByteWriteStatus::Accepted,16U};
        const auto completed=pump.Pump(endpoint);
        assert(completed.IsComplete() && completed.AcceptedFrameBytes == 21U);
        assert(endpoint.Accepted == 21U);
        assert(!pump.IsPending());
    }

    void TestInvalidInputsCannotQueue() {
        Serial::EdlgFramePump<1U> pump;
        const std::array<std::byte,2U> tooLong{std::byte{1},std::byte{2}};
        assert(pump.Begin(Logging::EdlgCodec::Cbor,tooLong) ==
            Serial::EdlgPumpBeginStatus::InvalidPayload);
        assert(!pump.IsPending());
        const std::array<std::byte,1U> payload{std::byte{1}};
        assert(pump.Begin(static_cast<Logging::EdlgCodec>(99U),payload) ==
            Serial::EdlgPumpBeginStatus::InvalidCodec);
        assert(!pump.IsPending());
        const std::span<const std::byte> absent(
            static_cast<const std::byte*>(nullptr),1U
        );
        assert(pump.Begin(Logging::EdlgCodec::Cbor,absent) ==
            Serial::EdlgPumpBeginStatus::InvalidPayload);
        assert(!pump.IsPending());
    }
}

static_assert(ESPressio::Serial::ByteWriteEndpoint<Test::Endpoint>);

int main() {
    Test::TestPartialFrameAndNoInterleaving();
    Test::TestPartialFailureRetainsPrefixAndFrame();
    Test::TestInvalidInputsCannotQueue();
}
