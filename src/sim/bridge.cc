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

#define STR0(x) #x
#define STR1(x) STR0(x)

#ifndef SIM_ID
#define SIM_ID "unknown"
#endif

namespace {

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
     * Send the info about the running simulator to the client
     *
     * @return 0 on success; -1 on failure
     */
    int sendSimulatorInfo();

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

    LOG << "[SW-AXI] Client connected to: " << uri << std::endl;
    sock = connSock;
    connectedUri = uri;

    return 0;
}

int Bridge::sendSimulatorInfo() {
    char hn[256];
    hn[0] = 0;
    gethostname(hn, 256);

    flatbuffers::FlatBufferBuilder builder(1024);
    auto name = builder.CreateString(std::string(STR1(SIM_ID)));
    auto uri = builder.CreateString(connectedUri);
    auto hostname = builder.CreateString(hn);
    sw_axi::wire::SimulatorInfoBuilder siBuilder(builder);
    siBuilder.add_simulatorName(name);
    siBuilder.add_pid(getpid());
    siBuilder.add_uri(uri);
    siBuilder.add_hostname(hostname);
    auto si = siBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_SIMULATOR_INFO);
    msgBuilder.add_simulatorInfo(si);
    auto msg = msgBuilder.Finish();
    builder.Finish(msg);

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
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

    if (bridge->sendSimulatorInfo() != 0) {
        bridge->disconnect();
        delete bridge;
        return NULL;
    }

    return reinterpret_cast<void *>(bridge);
}

extern "C" void sw_axi_disconnect(void *data) {
    if (!data) {
        return;
    }

    Bridge *bridge = reinterpret_cast<Bridge *>(data);
    bridge->waitForDisconnect();
    delete bridge;
}
