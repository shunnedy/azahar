// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/archives.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nim/nim_aoc.h"
#include "core/memory.h"

SERVICE_CONSTRUCT_IMPL(Service::NIM::NIM_AOC)
SERIALIZE_EXPORT_IMPL(Service::NIM::NIM_AOC)

namespace Service::NIM {

void NIM_AOC::Initialize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(ResultSuccess);
    LOG_WARNING(Service_NIM, "(STUBBED) called");
}

void NIM_AOC::SetApplicationId(Kernel::HLERequestContext& ctx) {
    // Setter used before the content op; just acknowledge.
    u32* cmd_buf = ctx.CommandBuffer();
    cmd_buf[0] = IPC::MakeHeader(0x0003, 1, 0);
    cmd_buf[1] = 0;
    LOG_WARNING(Service_NIM, "(STUBBED) called");
}

void NIM_AOC::SetTin(Kernel::HLERequestContext& ctx) {
    u32* cmd_buf = ctx.CommandBuffer();
    cmd_buf[0] = IPC::MakeHeader(0x0004, 1, 0);
    cmd_buf[1] = 0;
    LOG_WARNING(Service_NIM, "(STUBBED) called");
}

void NIM_AOC::GetBalance(Kernel::HLERequestContext& ctx) {
    // Report a non-zero balance so a commerce ("jewel shop") front-end is not
    // reported as closed.
    u32* cmd_buf = ctx.CommandBuffer();
    cmd_buf[0] = IPC::MakeHeader(0x0018, 3, 0);
    cmd_buf[1] = 0;
    cmd_buf[2] = 1000; // balance
    cmd_buf[3] = 0;
    LOG_WARNING(Service_NIM, "(STUBBED) called");
}

void NIM_AOC::ContentOp000B(Kernel::HLERequestContext& ctx) {
    // Content/"match-result" op used by some online features (e.g. 電波人間のRPG
    // FREE!'s Wi-Fi Colosseum). The caller reads the response's cmd_buf[2] as a
    // pointer to a {array, count, status} struct and treats status == 6 as
    // "service active" (anything else is shown as "service ended"). There is no
    // backend to HLE, so synthesise an empty-but-active result (count = 0,
    // status = 6) in one of the guest buffers the request carries and return its
    // address. Without this the unimplemented-function fallback returns no data
    // and the game reads uninitialised memory.
    u32* cmd_buf = ctx.CommandBuffer();
    IPC::Header header{cmd_buf[0]};
    const u32 normal_params = header.normal_params_size;

    // Pick a guest heap pointer from the normal params to hold the result struct.
    VAddr out = 0;
    for (u32 i = 1; i <= normal_params && i < 16; ++i) {
        if (cmd_buf[i] >= 0x08000000u && cmd_buf[i] < 0x60000000u) {
            out = cmd_buf[i];
            break;
        }
    }

    if (out != 0) {
        auto& memory = system.Memory();
        memory.Write32(out + 0, 0); // array pointer (unused while count == 0)
        memory.Write32(out + 4, 0); // count
        memory.Write32(out + 8, 6); // status == 6 => service active
    } else {
        LOG_WARNING(Service_NIM, "no guest heap pointer in params; returning null result");
    }

    cmd_buf[0] = IPC::MakeHeader(0x000B, 2, 0);
    cmd_buf[1] = 0;   // ResultSuccess
    cmd_buf[2] = out; // pointer to the {array, count, status} result struct
    LOG_WARNING(Service_NIM, "(STUBBED) called");
}

NIM_AOC::NIM_AOC(Core::System& system_) : ServiceFramework("nim:aoc", 2), system(system_) {
    const FunctionInfo functions[] = {
        // clang-format off
        {0x0003, &NIM_AOC::SetApplicationId, "SetApplicationId"},
        {0x0004, &NIM_AOC::SetTin, "SetTin"},
        {0x0009, nullptr, "ListContentSetsEx"},
        {0x000B, &NIM_AOC::ContentOp000B, "ContentOp000B"},
        {0x0018, &NIM_AOC::GetBalance, "GetBalance"},
        {0x001D, nullptr, "GetCustomerSupportCode"},
        {0x0021, &NIM_AOC::Initialize, "Initialize"},
        {0x0024, nullptr, "CalculateContentsRequiredSize"},
        {0x0025, nullptr, "RefreshServerTime"},
        // clang-format on
    };
    RegisterHandlers(functions);
}

NIM_AOC::~NIM_AOC() = default;

} // namespace Service::NIM
