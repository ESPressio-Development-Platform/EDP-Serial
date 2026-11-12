# EDP-Serial — developer Wiki

**Version 0.1.0**, current source on `feature/logging-v1` until guarded `main` promotion. This is the *platform-neutral* EDP-System Composition-owned bounded Serial byte endpoint, opt-in canonical EDLG framing/pumping and one fixed-capacity ordinary-context FIFO. It does not own UART, CDC, task scheduling, peripheral setup, physical delivery or persistence.

The default [public umbrella](Reference-serial-ESPressio_Serial) requires only EDP-System; opt-in [EDLG Pump](Reference-serial-EdlgFramePump) adds EDP-Logging; opt-in [FIFO](Reference-serial-BoundedEdlgFrameQueue) also adds the EDP-Threading ordinary Mutex. The native UART TX concrete belongs to a separate EDP-Serial-ESP-IDF repository.

Read the [Architecture](Architecture), [Dependency Contracts](Dependency-Contracts), [Public API](Public-API), [Internal API](Internal-API), [Private Implementation](Private-Implementation), [Composition and Providers](Composition-and-Providers), [Resources and Lifecycle](Resources-and-Lifecycle), [Build/Test/Source](Build-Test-and-Source), [Compiler Definitions](Compiler-Definitions), [Tooling](Tooling-Reference) and comprehensive [Reference Index](Reference-Index). The [three-target demos](../demos/README.MD) use memory-only examples, not a real UART peripheral.

**No acceptance beyond evidence:** native accepted-prefix state is never on-wire receipt; a fake native mutex is never a concurrent Platform/Threading provider. All versions remain 0.1.0.
