// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/applets/mint.h"
#include "core/hle/service/apt/apt.h"

namespace HLE::Applets {

Result Mint::ReceiveParameterImpl(const Service::APT::MessageParameter& parameter) {
    if (parameter.signal != Service::APT::SignalType::Request) {
        LOG_ERROR(Service_APT, "unsupported signal {}", parameter.signal);
        UNIMPLEMENTED();
        // TODO(Subv): Find the right error code
        return ResultUnknown;
    }

    // The Request message contains a buffer with the size of the framebuffer shared
    // memory.
    // Create the SharedMemory that will hold the framebuffer data
    Service::APT::CaptureBufferInfo capture_info;
    ASSERT(sizeof(capture_info) == parameter.buffer.size());

    std::memcpy(&capture_info, parameter.buffer.data(), sizeof(capture_info));

    // TODO: allocated memory never released
    using Kernel::MemoryPermission;
    // Create a SharedMemory that directly points to this heap block.
    framebuffer_memory = system.Kernel().CreateSharedMemoryForApplet(
        0, capture_info.size, MemoryPermission::ReadWrite, MemoryPermission::ReadWrite,
        "Mint Memory");

    // Send the response message with the newly created SharedMemory
    SendParameter({
        .sender_id = id,
        .destination_id = parent,
        .signal = Service::APT::SignalType::Response,
        .object = framebuffer_memory,
    });

    return ResultSuccess;
}

Result Mint::Start(const Service::APT::MessageParameter& parameter) {
    startup_param = parameter.buffer;

    // Let the application know that we're closing
    Finalize();
    return ResultSuccess;
}

Result Mint::Finalize() {
    // The Mint applet is the Nintendo eShop. A game normally launches it for its
    // one-time Nintendo-Network registration / EULA step and title-ownership check
    // before using an online (commerce-backed) feature. There is no eShop to HLE,
    // so instead of returning an all-zero buffer (which the game reads as "not
    // registered / not owned" and then loops the applet), echo the input buffer
    // back and mark the result as success so the game's nn::ec flow completes.
    //
    // The 0xE30-byte parameter is nn::ec's EShopInParam. The two ops a title issues
    // during this flow are distinguished by whether a title id is present at +0x08:
    //   op1 (EULA / console registration): no title id -> echoing it is accepted.
    //   op2 (title-ownership):             title id present -> the game additionally
    //        reads the u32 result slot at +0x20, which must be non-zero ("owned").
    auto read_u32 = [](const std::vector<u8>& b, std::size_t off) -> u32 {
        return b.size() >= off + 4 ? (u32)b[off] | ((u32)b[off + 1] << 8) |
                                         ((u32)b[off + 2] << 16) | ((u32)b[off + 3] << 24)
                                   : 0;
    };
    auto write_u32 = [](std::vector<u8>& b, std::size_t off, u32 v) {
        if (b.size() >= off + 4) {
            b[off] = v & 0xFF;
            b[off + 1] = (v >> 8) & 0xFF;
            b[off + 2] = (v >> 16) & 0xFF;
            b[off + 3] = (v >> 24) & 0xFF;
        }
    };

    std::vector<u8> buffer = startup_param;
    const bool has_title_id = read_u32(buffer, 0x08) != 0;
    if (has_title_id) {
        // op2: report the title as owned/registered.
        write_u32(buffer, 0x20, 1);
    }

    CloseApplet(nullptr, buffer);
    return ResultSuccess;
}

void Mint::Update() {}

} // namespace HLE::Applets
