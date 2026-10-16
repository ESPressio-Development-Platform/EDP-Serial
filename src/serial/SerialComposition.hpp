#pragma once

#include <ESPressio_System.hpp>

namespace ESPressio::Serial {

    namespace Framework = ESPressio::System::CompositionFramework;

    /// Platform-neutral Serial capability domain. Concrete SDKs must be supplied
    /// by Bootstrap, never instantiated directly by this abstract owner.
    struct Domain final : Framework::Domain {};

    /// Exclusive application-selected nonblocking endpoint accepting owned
    /// bytes by value (or synchronously copying caller-borrowed source data).
    /// The endpoint must not retain a span past TryWrite() return.
    struct ByteStreamWriter final : Framework::ExclusiveCapability<Domain> {};

    /// Unique same-domain provider with a stable lifetime is required when
    /// an application instantiates a Serial writer.
    using ByteStreamWriterRequirement = Framework::Requirement<
        ByteStreamWriter,
        Framework::RequirementScope::SameDomain,
        Framework::ExactlyProviders<1U>
    >;

} // namespace ESPressio::Serial
