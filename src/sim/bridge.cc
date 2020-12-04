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

#include "../common/RouterClient.hh"

#include <exception>
#include <iostream>

using namespace sw_axi;

extern "C" void *sw_axi_client_new() {
    return new RouterClient();
}

extern "C" void sw_axi_client_delete(void *client) {
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    delete c;
}

extern "C" void
sw_axi_client_connect(void *client, void **status, void **systemInfo, const char *uri, const char *name) {
    if (!client || !status || !systemInfo) {
        std::cerr << "Either of client, status, or systemInfo pointers is null" << std::endl;
        std::terminate();
    }
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    std::pair<SystemInfo *, Status> ret = c->connect(uri, name);
    *systemInfo = ret.first;
    *status = new Status(ret.second);
}

extern "C" void sw_axi_client_register_slave(
        void *client,
        void **status,
        unsigned long long *id,
        const char *name,
        unsigned long long address,
        unsigned long long size,
        unsigned short firstInterrupt,
        unsigned short numInterrupts,
        int type,
        int implementation) {
    if (!client || !status | !id) {
        std::cerr << "Either of client, status, or id pointers is null" << std::endl;
        std::terminate();
    }
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    IpConfig cfg;
    cfg.name = name;
    cfg.address = address;
    cfg.size = size;
    cfg.firstInterrupt = firstInterrupt;
    cfg.numInterrupts = numInterrupts;
    cfg.type = IpType(type);
    cfg.implementation = IpImplementation(implementation);
    std::pair<uint64_t, Status> ret = c->registerSlave(cfg);
    *id = ret.first;
    *status = new Status(ret.second);
}

extern "C" void sw_axi_client_commit_ip(void *client, void **status) {
    if (!client || !status) {
        std::cerr << "Either client or status pointer is null" << std::endl;
        std::terminate();
    }
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    Status st = c->commitIp();
    *status = new Status(st);
}

extern "C" void sw_axi_client_retrieve_peer_info(void *client, void **status, void **systemInfo) {
    if (!client || !status || !systemInfo) {
        std::cerr << "Either of client, status, or systemInfo pointers is null" << std::endl;
        std::terminate();
    }
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    std::pair<SystemInfo *, Status> ret = c->retrievePeerInfo();
    *systemInfo = ret.first;
    *status = new Status(ret.second);
}

extern "C" void sw_axi_client_retrieve_ip_config(void *client, void **status, void **ipConfig) {
    if (!client || !status || !ipConfig) {
        std::cerr << "Either of client, status, or ipConfig pointers is null" << std::endl;
        std::terminate();
    }
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    std::pair<IpConfig *, Status> ret = c->retrieveIpConfig();
    *ipConfig = ret.first;
    *status = new Status(ret.second);
}

extern "C" void sw_axi_client_disconnect(void *client) {
    if (!client) {
        std::cerr << "The client pointer is null" << std::endl;
        std::terminate();
    }
    RouterClient *c = reinterpret_cast<RouterClient *>(client);
    c->disconnect();
}

extern "C" const char *sw_axi_status_get_msg(void *status) {
    if (!status) {
        std::cerr << "The status pointer is null" << std::endl;
        std::terminate();
    }

    Status *st = reinterpret_cast<Status *>(status);
    return st->getMessage().c_str();
}

extern "C" unsigned int sw_axi_status_get_code(void *status) {
    if (!status) {
        std::cerr << "The status pointer is null" << std::endl;
        std::terminate();
    }

    Status *st = reinterpret_cast<Status *>(status);
    return st->getCode();
}

extern "C" void sw_axi_status_delete(void *status) {
    Status *st = reinterpret_cast<Status *>(status);
    delete st;
}

extern "C" const char *sw_axi_system_info_get_name(void *systemInfo) {
    if (!systemInfo) {
        std::cerr << "The systemInfo pointer is null" << std::endl;
        std::terminate();
    }

    SystemInfo *si = reinterpret_cast<SystemInfo *>(systemInfo);
    return si->name.c_str();
}

extern "C" const char *sw_axi_system_info_get_system_name(void *systemInfo) {
    if (!systemInfo) {
        std::cerr << "The systemInfo pointer is null" << std::endl;
        std::terminate();
    }

    SystemInfo *si = reinterpret_cast<SystemInfo *>(systemInfo);
    return si->systemName.c_str();
}
extern "C" const unsigned long long sw_axi_system_info_get_pid(void *systemInfo) {
    if (!systemInfo) {
        std::cerr << "The systemInfo pointer is null" << std::endl;
        std::terminate();
    }

    SystemInfo *si = reinterpret_cast<SystemInfo *>(systemInfo);
    return si->pid;
}

extern "C" const char *sw_axi_system_info_get_hostname(void *systemInfo) {
    if (!systemInfo) {
        std::cerr << "The systemInfo pointer is null" << std::endl;
        std::terminate();
    }

    SystemInfo *si = reinterpret_cast<SystemInfo *>(systemInfo);
    return si->hostname.c_str();
}

extern "C" void sw_axi_system_info_delete(void *systemInfo) {
    SystemInfo *si = reinterpret_cast<SystemInfo *>(systemInfo);
    delete si;
}

extern "C" const char *sw_axi_ip_config_get_name(void *ipConfig) {
    if (!ipConfig) {
        std::cerr << "The ipConfig pointer is null" << std::endl;
        std::terminate();
    }

    IpConfig *ipc = reinterpret_cast<IpConfig *>(ipConfig);
    return ipc->name.c_str();
}

extern "C" const void
sw_axi_ip_config_get_address_and_size(void *ipConfig, unsigned long long *addr, unsigned long long *size) {
    if (!ipConfig || !addr || !size) {
        std::cerr << "Either of ipConfig, addr, or size pointers is null" << std::endl;
        std::terminate();
    }

    IpConfig *ipc = reinterpret_cast<IpConfig *>(ipConfig);
    *addr = ipc->address;
    *size = ipc->size;
}

extern "C" const void sw_axi_ip_config_get_interrupts(void *ipConfig, unsigned short *first, unsigned short *num) {
    if (!ipConfig || !first || !num) {
        std::cerr << "Either of ipConfig, first, or num pointers is null" << std::endl;
        std::terminate();
    }

    IpConfig *ipc = reinterpret_cast<IpConfig *>(ipConfig);
    *first = ipc->firstInterrupt;
    *num = ipc->numInterrupts;
}

extern "C" const void sw_axi_ip_config_get_type(void *ipConfig, int *typ, int *impl) {
    if (!ipConfig || !typ || !impl) {
        std::cerr << "Either of ipConfig, typ, or impl pointers is null" << std::endl;
        std::terminate();
    }

    IpConfig *ipc = reinterpret_cast<IpConfig *>(ipConfig);
    *typ = int(ipc->type);
    *impl = int(ipc->implementation);
}

extern "C" void sw_axi_ip_config_delete(void *ipConfig) {
    IpConfig *ipc = reinterpret_cast<IpConfig *>(ipConfig);
    delete ipc;
}
