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

#include "../common/Utils.hh"
#include "IpcStructs_generated.h"

#include "svdpi.h"

#include <cstring>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>

#define STR0(x) #x
#define STR1(x) STR0(x)

#ifndef SIM_ID
#define SIM_ID "unknown"
#endif

namespace {

/**
 * A helper class for communicating IP information to SystemVerilog
 */
struct IpInfo {
    std::string name;
    uint64_t startAddr = 0;
    uint64_t endAddr = 0;
    int type = 0;
    int implementation = 0;
};

/**
 * Bridge is the SystemVerilog DPI counterpart of the `sw_axi::Bridge`.
 *
 * It implements the communication mechanism with the software side of the system.
 */
class Bridge {
public:
    /**
     * Create a server socket at the given randez-vous point and wait for the client to connect
     *
     * @param uri an URI pointing to the rendez-vous point with the software client; currently only UNIX domain sockets
     *            are supported
     *
     * @return 0 on success; -1 on failure
     */
    int connect(const std::string &uri);

    /**
     * Send the info about the system to the client
     *
     * @return 0 on success; -1 on failure
     */
    int sendSystemInfo();

    /**
     * Send IP information to the client
     *
     * @return 0 on success; -1 on failure
     */
    int sendIpInfo(const std::string &name, int type, int implementation, uint64_t startAddr, uint64_t endAddr);

    /**
     * Receive the IP information from the client
     *
     * @return 0 on success; -1 on failure
     */
    std::pair<int, IpInfo *> receiveIpInfo();

    /**
     * Send and acknowledgement message
     *
     * @return 0 on success; -1 on failure
     */
    int acknowledge();

    /**
     * Send an error message
     *
     * @return 0 on success; -1 on failure
     */
    int reportError(const std::string msg);

    /**
     * Close the connection with the client and clean up the associated resources
     */
    void disconnect();

    /**
     * Wait for the client to disconnect and clean up the server-side resouces associated with the connection
     */
    void waitForDisconnect();

private:
    std::string connectedUri;
    int sock = -1;
};

int Bridge::connect(const std::string &uri) {
    LOG << "[SW-AXI] Creating an IPC server at: " << uri << std::endl;

    if (uri.length() < 8 || uri.find("unix://") != 0) {
        LOG << "[SW-AXI] Can only communicate over UNIX domain sockets " << uri << std::endl;
        return -1;
    }
    std::string path = uri.substr(7);

    if (sock != -1) {
        LOG << "[SW-AXI] The bridge is already connected to " << connectedUri << std::endl;
        return -1;
    }

    sockaddr_un addr;
    if (path.length() > sizeof(addr.sun_path) - 1) {
        LOG << "[SW-AXI] Path too long: " << path << std::endl;
        return -1;
    }

    if (remove(path.c_str()) == -1 && errno != ENOENT) {
        LOG << "[SW-AXI] Unable to remove: " << path << std::endl;
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

    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un)) == -1) {
        LOG << "[SW-AXI] Unable to bind the socket: " << strerror(errno) << std::endl;
        disconnect();
        return -1;
    }

    if (listen(sock, 1) == -1) {
        LOG << "[SW-AXI] Unable to listen for incomming connections: " << strerror(errno) << std::endl;
        disconnect();
        return -1;
    }

    int connSock = accept(sock, nullptr, nullptr);
    if (connSock == -1) {
        LOG << "[SW-AXI] Unable to accept a connection: " << strerror(errno) << std::endl;
        disconnect();
        return -1;
    }

    sock = connSock;
    connectedUri = uri;

    std::vector<uint8_t> data;
    if (sw_axi::readFromSocket(sock, data) == -1) {
        LOG << "[SW-AXI] Error while receiving the system info" << std::endl;
        disconnect();
        return -1;
    }

    auto msg = sw_axi::wire::GetMessage(data.data());
    if (msg->type() != sw_axi::wire::Type_SYSTEM_INFO) {
        LOG << "[SW-AXI] Received an unexpected message from the simulator" << std::endl;
        disconnect();
        return -1;
    }

    LOG << "[SW-AXI] Client " << msg->systemInfo()->name()->str() << " connected to: " << uri << std::endl;
    return 0;
}

int Bridge::sendSystemInfo() {
    char hn[256];
    hn[0] = 0;
    gethostname(hn, 256);

    flatbuffers::FlatBufferBuilder builder(1024);
    auto name = builder.CreateString(std::string(STR1(SIM_ID)));
    auto uri = builder.CreateString(connectedUri);
    auto hostname = builder.CreateString(hn);
    sw_axi::wire::SystemInfoBuilder siBuilder(builder);
    siBuilder.add_name(name);
    siBuilder.add_pid(getpid());
    siBuilder.add_uri(uri);
    siBuilder.add_hostname(hostname);
    auto si = siBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_SYSTEM_INFO);
    msgBuilder.add_systemInfo(si);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        return -1;
    }
    return 0;
}

int Bridge::sendIpInfo(const std::string &name, int type, int implementation, uint64_t startAddr, uint64_t endAddr) {
    if (sock == -1) {
        LOG << "[SW-AXI] Client not connected" << std::endl;
        return -1;
    }

    LOG << "[SW-AXI] Registering slave " << name << " " << startAddr << " " << endAddr << std::endl;
    flatbuffers::FlatBufferBuilder builder(1024);
    auto fbName = builder.CreateString(name);
    sw_axi::wire::IpInfoBuilder ipBuilder(builder);
    ipBuilder.add_name(fbName);
    ipBuilder.add_startAddr(startAddr);
    ipBuilder.add_endAddr(endAddr);

    switch (type) {
    case 1:
        ipBuilder.add_type(sw_axi::wire::IpType_SLAVE_LITE);
        break;
    default:
        LOG << "[SW-AXI] Unknown slave type for slave " << name << ": " << type << std::endl;
        return -1;
    }

    auto impl =
            implementation == 0 ? sw_axi::wire::ImplementationType_SOFTWARE : sw_axi::wire::ImplementationType_HARDWARE;
    ipBuilder.add_implementation(impl);
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

    return 0;
}

std::pair<int, IpInfo *> Bridge::receiveIpInfo() {
    std::vector<uint8_t> data;
    if (sw_axi::readFromSocket(sock, data) == -1) {
        LOG << "[SW-AXI] Failed to get the IP Info from software" << std::endl;
        disconnect();
        return std::make_pair(-1, nullptr);
    }

    auto msg = sw_axi::wire::GetMessage(data.data());
    if (msg->type() == sw_axi::wire::Type_COMMIT) {
        return std::make_pair(0, nullptr);
    }

    if (msg->type() != sw_axi::wire::Type_IP_INFO) {
        LOG << "[SW-AXI] Got unexpected message from software, expected IP_INFO" << std::endl;
        disconnect();
        return std::make_pair(-1, nullptr);
    }

    IpInfo *info = new IpInfo();
    info->name = msg->ipInfo()->name()->str();
    info->startAddr = msg->ipInfo()->startAddr();
    info->endAddr = msg->ipInfo()->endAddr();

    switch (msg->ipInfo()->type()) {
    case sw_axi::wire::IpType_SLAVE_LITE:
        info->type = 1;
        break;
    default:
        delete info;
        LOG << "[SW-AXI] Invalid IP type: " << msg->ipInfo()->type() << std::endl;
        return std::make_pair(0, nullptr);
    }
    info->implementation = msg->ipInfo()->implementation() == sw_axi::wire::ImplementationType_SOFTWARE ? 0 : 1;
    return std::make_pair(0, info);
}

int Bridge::acknowledge() {
    flatbuffers::FlatBufferBuilder builder(1024);
    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_ACK);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        LOG << "[SW-AXI] Error while sending an ACK message to the simulator" << std::endl;
        disconnect();
        return -1;
    }

    return 0;
}

int Bridge::reportError(const std::string errorMsg) {
    flatbuffers::FlatBufferBuilder builder(1024);
    auto errorMsgFb = builder.CreateString(errorMsg);
    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_ERROR);
    msgBuilder.add_errorMessage(errorMsgFb);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        LOG << "[SW-AXI] Error while sending an ERROR message to the simulator" << std::endl;
        disconnect();
        return -1;
    }

    return 0;
}

void Bridge::disconnect() {
    if (sock != -1) {
        close(sock);
        sock = -1;
    }
}

void Bridge::waitForDisconnect() {
    if (sock == -1) {
        LOG << "[SW-AXI] Client not connected" << std::endl;
        return;
    }
    LOG << "[SW-AXI] Waiting for the client to disconnect from: " << connectedUri << std::endl;

    // TODO(lj): This is enough to track disconnection, to be replaced by a condition variable later
    char buff;
    read(sock, &buff, 1);
    disconnect();

    LOG << "[SW-AXI] Client disconnected from: " << connectedUri << std::endl;
}
}  // namespace

extern "C" void *sw_axi_connect(const char *uri) {
    Bridge *bridge = new Bridge;
    if (bridge->connect(uri) != 0) {
        return NULL;
    }

    if (bridge->sendSystemInfo() != 0) {
        bridge->disconnect();
        delete bridge;
        return NULL;
    }

    return reinterpret_cast<void *>(bridge);
}

extern "C" void sw_axi_disconnect(void *bridgeHandle) {
    if (!bridgeHandle) {
        return;
    }

    Bridge *bridge = reinterpret_cast<Bridge *>(bridgeHandle);
    bridge->waitForDisconnect();
    delete bridge;
}

extern "C" int sw_axi_ip_info_receive(void *bridgeHandle, void **ip) {
    if (!bridgeHandle || !ip) {
        return -1;
    }

    Bridge *bridge = reinterpret_cast<Bridge *>(bridgeHandle);
    auto si = bridge->receiveIpInfo();
    if (si.first == -1) {
        return -1;
    }

    *ip = si.second;
    return 0;
}

extern "C" const char *sw_axi_ip_info_get_name(void *ip) {
    IpInfo *ipInfo = reinterpret_cast<IpInfo *>(ip);
    return ipInfo->name.c_str();
}

extern "C" void sw_axi_ip_info_get_addr(void *ip, unsigned long long *start, unsigned long long *end) {
    IpInfo *ipInfo = reinterpret_cast<IpInfo *>(ip);
    *start = ipInfo->startAddr;
    *end = ipInfo->endAddr;
}

extern "C" int sw_axi_ip_info_get_type(void *ip) {
    IpInfo *ipInfo = reinterpret_cast<IpInfo *>(ip);
    return ipInfo->type;
}

extern "C" int sw_axi_ip_info_get_implementation(void *ip) {
    IpInfo *ipInfo = reinterpret_cast<IpInfo *>(ip);
    return ipInfo->implementation;
}

extern "C" void sw_axi_ip_info_free(void *ip) {
    IpInfo *ipInfo = reinterpret_cast<IpInfo *>(ip);
    delete ipInfo;
}

extern "C" int sw_axi_acknowledge(void *bridgeHandle) {
    if (!bridgeHandle) {
        return -1;
    }

    Bridge *bridge = reinterpret_cast<Bridge *>(bridgeHandle);
    return bridge->acknowledge();
}

extern "C" int sw_axi_report_error(void *bridgeHandle, const char *msg) {
    if (!bridgeHandle) {
        return -1;
    }

    Bridge *bridge = reinterpret_cast<Bridge *>(bridgeHandle);
    return bridge->reportError(msg);
}

extern "C" int sw_axi_ip_info_send(
        void *bridgeHandle,
        const char *name,
        int type,
        int implementation,
        unsigned long long startAddr,
        unsigned long long endAddr) {
    if (!bridgeHandle) {
        return -1;
    }

    Bridge *bridge = reinterpret_cast<Bridge *>(bridgeHandle);
    return bridge->sendIpInfo(name, type, implementation, startAddr, endAddr);
}
