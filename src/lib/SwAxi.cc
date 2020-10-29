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
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace sw_axi {

Bridge::~Bridge() {}

int Bridge::connect(std::string uri) {
    if (state != State::DISCONNECTED) {
        LOG << "[SW-AXI] The bridge needs to be disconnected for the connect operation to proceed" << std::endl;
        return -1;
    }

    if (uri.length() < 8 || uri.find("unix://") != 0) {
        LOG << "[SW-AXI] Can only communicate over UNIX domain sockets " << uri << std::endl;
        return -1;
    }
    std::string path = uri.substr(7);

    sockaddr_un addr;
    if (path.length() > sizeof(addr.sun_path) - 1) {
        LOG << "[SW-AXI] Path too long: " << path << std::endl;
        return -1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        LOG << "[SW-AXI] Unable to create a UNIX socket: " << strerror(errno) << std::endl;
        return -1;
    }

    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un)) == -1) {
        LOG << "[SW-AXI] Unable to connect to the UNIX socket " << path << ": " << strerror(errno) << std::endl;
        disconnect();
        return -1;
    }

    connectedUri = uri;

    char hn[256];
    hn[0] = 0;
    gethostname(hn, 256);
    std::ostringstream o;
    utsname sysInfo;
    uname(&sysInfo);
    o << sysInfo.sysname << " " << sysInfo.nodename << " " << sysInfo.release << " " << sysInfo.version;

    flatbuffers::FlatBufferBuilder builder(1024);
    auto name = builder.CreateString(o.str());
    auto uriStr = builder.CreateString(connectedUri);
    auto hostname = builder.CreateString(hn);
    sw_axi::wire::SystemInfoBuilder siBuilder(builder);
    siBuilder.add_name(name);
    siBuilder.add_pid(getpid());
    siBuilder.add_uri(uriStr);
    siBuilder.add_hostname(hostname);
    auto si = siBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_SYSTEM_INFO);
    msgBuilder.add_systemInfo(si);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        LOG << "[SW-AXI] Error while sending the system info" << std::endl;
        disconnect();
        return -1;
    }

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        LOG << "[SW-AXI] Error while receiving the system info" << std::endl;
        disconnect();
        return -1;
    }

    auto msg = wire::GetMessage(data.data());

    if (msg->type() != wire::Type_SYSTEM_INFO) {
        LOG << "[SW-AXI] Received an unexpected message from the simulator" << std::endl;
        disconnect();
        return -1;
    }

    systemInfo = std::make_unique<SystemInfo>();
    systemInfo->name = msg->systemInfo()->name()->str();
    systemInfo->pid = msg->systemInfo()->pid();
    systemInfo->uri = msg->systemInfo()->uri()->str();
    systemInfo->hostname = msg->systemInfo()->hostname()->str();
    state = State::CONNECTED;
    return 0;
}

const SystemInfo *Bridge::getRemoteSystemInfo() const {
    return systemInfo.get();
}

int Bridge::registerSlave(Slave *slave, const IpConfig &config) {
    if (state != State::CONNECTED) {
        LOG << "[SW-AXI] Can register slaves only in CONNECTED mode" << std::endl;
        return -1;
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    auto fbName = builder.CreateString(config.name);
    sw_axi::wire::IpInfoBuilder ipBuilder(builder);
    ipBuilder.add_name(fbName);
    ipBuilder.add_startAddr(config.startAddress);
    ipBuilder.add_endAddr(config.endAddress);
    ipBuilder.add_implementation(wire::ImplementationType_SOFTWARE);

    switch (config.type) {
    case IpType::SLAVE_LITE:
        ipBuilder.add_type(wire::IpType_SLAVE_LITE);
        break;
    default:
        LOG << "[SW-AXI] Unknown slave type for slave " << config.name << ": ";
        LOG << int(config.type) << std::endl;
        return -1;
    }
    auto ip = ipBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_IP_INFO);
    msgBuilder.add_ipInfo(ip);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        LOG << "[SW-AXI] Error while sending the SIMULATOR_INFO message to software" << std::endl;
        disconnect();
        return -1;
    }

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        LOG << "[SW-AXI] Error while receiving response to IP registration" << std::endl;
        disconnect();
        return -1;
    }

    auto msg = wire::GetMessage(data.data());
    if (msg->type() == wire::Type_ERROR) {
        LOG << "[SW-AXI] Cannot register IP " << config.name << ": " << msg->errorMessage()->str() << std::endl;
        disconnect();
        return -1;
    }

    if (msg->type() != wire::Type_ACK) {
        LOG << "[SW-AXI] Got an unexpected response while registering IP " << config.name << ": ";
        LOG << msg->type() << std::endl;
        disconnect();
        return -1;
    }

    slaveMap[config.startAddress] = {config.endAddress, slave};
    return 0;
}

int Bridge::commitIp() {
    if (state != State::CONNECTED) {
        LOG << "[SW-AXI] Can commit slaves only in CONNECTED mode" << std::endl;
        return -1;
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_COMMIT);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        LOG << "[SW-AXI] Error while sending the COMMIT message" << std::endl;
        disconnect();
        return -1;
    }

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        LOG << "[SW-AXI] Error while receiving the simulator info" << std::endl;
        disconnect();
        return -1;
    }

    auto msg = wire::GetMessage(data.data());
    if (msg->type() == wire::Type_ERROR) {
        LOG << "[SW-AXI] Cannot register IP: " << msg->errorMessage()->str() << std::endl;
        disconnect();
        return -1;
    }

    if (msg->type() != wire::Type_ACK) {
        LOG << "[SW-AXI] Got an unexpected response while committing IP: ";
        LOG << msg->type() << std::endl;
        disconnect();
        return -1;
    }

    state = State::COMMITTED;

    return 0;
}

int Bridge::start() {
    if (state != State::COMMITTED) {
        LOG << "[SW-AXI] The bridge needs to be COMMITTED in order to start" << std::endl;
        return -1;
    }

    // Read the IP info reported by the simulator
    std::vector<uint8_t> data;
    const wire::Message *msg;
    while (true) {
        if (readFromSocket(sock, data) == -1) {
            LOG << "[SW-AXI] Error while receiving the simulator info" << std::endl;
            disconnect();
            return -1;
        }
        msg = wire::GetMessage(data.data());

        if (msg->type() != wire::Type_IP_INFO) {
            break;
        }

        IpConfig ip;
        ip.name = msg->ipInfo()->name()->str();
        ip.startAddress = msg->ipInfo()->startAddr();
        ip.endAddress = msg->ipInfo()->endAddr();

        switch (msg->ipInfo()->type()) {
        case wire::IpType_SLAVE_LITE:
            ip.type = IpType::SLAVE_LITE;
            break;
        default:
            continue;
        }

        ip.implementation = msg->ipInfo()->implementation() == wire::ImplementationType_SOFTWARE ?
                IpImplementation::SOFTWARE :
                IpImplementation::HARDWARE;

        ipBlocks.push_back(ip);
    }

    if (msg->type() == wire::Type_ERROR) {
        LOG << "[SW-AXI] Error while receiving the IP list: " << msg->errorMessage()->str() << std::endl;
        disconnect();
        return -1;
    }

    if (msg->type() != wire::Type_ACK) {
        LOG << "[SW-AXI] Got an unexpected response while committing IP: ";
        LOG << msg->type() << std::endl;
        disconnect();
        return -1;
    }

    state = State::STARTED;
    return 0;
}

int Bridge::enumerateIp(std::vector<IpConfig> &ip) {
    if (state != State::STARTED) {
        LOG << "[SW-AXI] The bridge needs to be started in order to enumerate IPs" << std::endl;
        return -1;
    }
    ip = ipBlocks;
    return 0;
}

void Bridge::disconnect() {
    if (sock == -1) {
        LOG << "[SW-AXI] Not connected" << std::endl;
        return;
    }

    close(sock);
    state = State::DISCONNECTED;
    connectedUri = "";
    sock = -1;
    systemInfo.reset(nullptr);
    ipBlocks.clear();

    for (auto &entry : slaveMap) {
        delete entry.second.slave;
    }
    slaveMap.clear();
}

}  // namespace sw_axi