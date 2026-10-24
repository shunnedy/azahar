// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included..

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::NIM {

class NIM_AOC final : public ServiceFramework<NIM_AOC> {
public:
    explicit NIM_AOC(Core::System& system);
    ~NIM_AOC();

private:
    // STUBBED: some games (電波人間のRPG FREE! Wi-Fi Colosseum) init nim:aoc during
    // the online-connection flow; returning success lets them proceed.
    void Initialize(Kernel::HLERequestContext& ctx);

    // Setters the Wi-Fi Colosseum connect flow calls before the content op; they
    // only need to report success.
    void SetApplicationId(Kernel::HLERequestContext& ctx);
    void SetTin(Kernel::HLERequestContext& ctx);

    // The colosseum "match result" content op (command 0x000B). The game reads the
    // response's cmd_buf[2] as a pointer to a {array, count, status} struct and
    // treats status==6 as "service active". See nim_aoc.cpp for details.
    void ContentOp000B(Kernel::HLERequestContext& ctx);

    // Wallet balance query (jewel shop). Report a non-zero balance so the shop is
    // not reported as closed.
    void GetBalance(Kernel::HLERequestContext& ctx);

    Core::System& system;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        DEBUG_SERIALIZATION_POINT;
        ar& boost::serialization::base_object<Kernel::SessionRequestHandler>(*this);
    }
    friend class boost::serialization::access;
    friend class ::construct_access;
};

} // namespace Service::NIM

SERVICE_CONSTRUCT(Service::NIM::NIM_AOC)
BOOST_CLASS_EXPORT_KEY(Service::NIM::NIM_AOC)
