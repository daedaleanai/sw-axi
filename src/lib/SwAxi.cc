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
#include "../common/RouterClient.hh"

namespace sw_axi {

Bridge::Bridge(const std::string &name) : client(new RouterClient()), name(name) {}

Bridge::~Bridge() {
    disconnect();
    delete client;
}

Status Bridge::connect(std::string uri) {
    std::pair<SystemInfo *, Status> ret = client->connect(uri, name);
    if (ret.second.isError()) {
        return ret.second;
    }
    routerInfo.reset(ret.first);
    return Status();
}

const SystemInfo *Bridge::getRouterInfo() const {
    return routerInfo.get();
}

Status Bridge::registerSlave(Slave *slave, const IpConfig &config) {
    std::pair<uint64_t, Status> ret = client->registerSlave(config);
    if (ret.second.isError()) {
        return ret.second;
    }
    slaveMap[ret.first] = slave;
    return Status();
}

Status Bridge::commitIp() {
    return client->commitIp();
}

Status Bridge::start() {
    while (true) {
        std::pair<SystemInfo *, Status> ret = client->retrievePeerInfo();
        if (ret.second.isError()) {
            disconnect();
            return ret.second;
        }
        if (!ret.first) {
            break;
        }
        peers.push_back(*ret.first);
        delete ret.first;
    }

    while (true) {
        std::pair<IpConfig *, Status> ret = client->retrieveIpConfig();
        if (ret.second.isError()) {
            disconnect();
            return ret.second;
        }
        if (!ret.first) {
            break;
        }
        ipBlocks.push_back(*ret.first);
        delete ret.first;
    }
    return Status();
}

Status Bridge::enumerateIp(std::vector<IpConfig> &ip) {
    if (client->getState() != RouterClient::State::STARTED) {
        return Status(1, "The bridge needs to be started in order to enumerate IPs");
    }
    ip = ipBlocks;
    return Status();
}

Status Bridge::enumeratePeers(std::vector<SystemInfo> &peers) {
    if (client->getState() != RouterClient::State::STARTED) {
        return Status(1, "The bridge needs to be started in order to enumerate peers");
    }
    peers = this->peers;
    return Status();
}

void Bridge::disconnect() {
    client->disconnect();
    routerInfo.reset(nullptr);
    ipBlocks.clear();

    for (auto &entry : slaveMap) {
        delete entry.second;
    }
    slaveMap.clear();
}

}  // namespace sw_axi
