//------------------------------------------------------------------------------
// Copyright (C) 2020 Daedalean AG
//
// This file is part of SW-AXI.
//
// SW-AXI is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SW-AXI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SW-AXI.  If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#include "SwAxi.hh"
#include "../common/Utils.hh"
#include "IpcStructs_generated.h"

#include <cstring>
#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace sw_axi {

int Bridge::connect(std::string uri) {
    if (uri.length() < 8 || uri.find("unix://") != 0) {
        LOG << "[SW-AXI] Can only communicate over UNIX domain sockets " << uri << std::endl;
        return 1;
    }
    std::string path = uri.substr(7);

    if (sock != -1) {
        LOG << "[SW-AXI] The bridge is already connected to " << connectedUri << std::endl;
        return 1;
    }

    sockaddr_un addr;
    if (path.length() > sizeof(addr.sun_path) - 1) {
        LOG << "[SW-AXI] Path too long: " << path << std::endl;
        return 1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        LOG << "[SW-AXI] Unable to create a UNIX socket: " << strerror(errno) << std::endl;
        return 1;
    }

    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un)) == -1) {
        LOG << "[SW-AXI] Unable to connect to the UNIX socket " << path << ": " << strerror(errno) << std::endl;
        close(sock);
        sock = -1;
        return 1;
    }

    connectedUri = uri;

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        LOG << "[SW-AXI] Error while receiving the simulator info" << std::endl;
        disconnect();
    }

    auto si = flatbuffers::GetRoot<wire::SimulatorInfo>(data.data());

    simulatorInfo = std::make_unique<SimulatorInfo>();
    simulatorInfo->simulatorName = si->simulatorName()->str();
    simulatorInfo->pid = si->pid();
    simulatorInfo->uri = si->uri()->str();
    simulatorInfo->hostname = si->hostname()->str();
    return 0;
}

const SimulatorInfo *Bridge::getSimulatorInfo() const {
    return simulatorInfo.get();
}

void Bridge::disconnect() {
    if (sock == -1) {
        LOG << "[SW-AXI] Not connected" << std::endl;
        return;
    }

    close(sock);
    connectedUri = "";
    sock = -1;
    simulatorInfo.reset(nullptr);
}

}  // namespace sw_axi
