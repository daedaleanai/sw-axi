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

#include "RouterClient.hh"
#include "IpcStructs_generated.h"
#include "Utils.hh"

#include <cstring>
#include <errno.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <unistd.h>

#define STR0(x) #x
#define STR1(x) STR0(x)

namespace sw_axi {

std::pair<SystemInfo *, Status> RouterClient::connect(const std::string &uri, const std::string &name) {
    if (state != State::DISCONNECTED) {
        Status st = Status(1, "The bridge needs to be disconnected for the connect operation to proceed");
        return std::make_pair(nullptr, st);
    }

    if (uri.length() < 8 || uri.find("unix://") != 0) {
        Status st = Status(1, "Can only communicate over UNIX domain sockets:" + uri);
        return std::make_pair(nullptr, st);
    }
    std::string path = uri.substr(7);

    sockaddr_un addr;
    if (path.length() > sizeof(addr.sun_path) - 1) {
        std::make_pair(nullptr, Status(1, "Path too long: " + path));
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        Status st = Status(1, std::string("Unable to create a UNIX socket: ") + strerror(errno));
        return std::make_pair(nullptr, st);
    }

    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_un)) == -1) {
        disconnect();
        Status st = Status(1, std::string("Unable to connect to the UNIX socket ") + path + ": " + strerror(errno));
        return std::make_pair(nullptr, st);
    }

    connectedUri = uri;

    std::ostringstream o;
    utsname sysInfo;
    uname(&sysInfo);

#ifndef SIM_ID
    o << sysInfo.sysname << " C++";
#else
    o << STR1(SIM_ID);
#endif

    flatbuffers::FlatBufferBuilder builder(1024);
    auto bName = builder.CreateString(name);
    auto sysName = builder.CreateString(o.str());
    auto hName = builder.CreateString(sysInfo.nodename);
    sw_axi::wire::SystemInfoBuilder siBuilder(builder);
    siBuilder.add_name(bName);
    siBuilder.add_systemName(sysName);
    siBuilder.add_pid(getpid());
    siBuilder.add_hostname(hName);
    auto si = siBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_SYSTEM_INFO);
    msgBuilder.add_systemInfo(si);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while sending the system info: ") + strerror(errno));
        return std::make_pair(nullptr, st);
    }

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while receiving the router's system info:") + strerror(errno));
        return std::make_pair(nullptr, st);
    }
    auto msg = wire::GetMessage(data.data());

    if (msg->type() != wire::Type_SYSTEM_INFO) {
        disconnect();
        Status st = Status(1, "Received an unexpected message from the router: " + std::to_string(int(msg->type())));
        return std::make_pair(nullptr, st);
    }

    SystemInfo *routerInfo = new SystemInfo();
    routerInfo->name = msg->systemInfo()->name()->str();
    routerInfo->systemName = msg->systemInfo()->systemName()->str();
    routerInfo->pid = msg->systemInfo()->pid();
    routerInfo->hostname = msg->systemInfo()->hostname()->str();
    state = State::CONNECTED;

    return std::make_pair(routerInfo, Status());
}

std::pair<uint64_t, Status> RouterClient::registerIp(const IpConfig &config) {
    if (state != State::CONNECTED) {
        Status st = Status(1, "Can register slaves only in CONNECTED mode");
        return std::make_pair(0, st);
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    auto fbName = builder.CreateString(config.name);
    sw_axi::wire::IpInfoBuilder ipBuilder(builder);
    ipBuilder.add_name(fbName);
    ipBuilder.add_address(config.address);
    ipBuilder.add_size(config.size);

    if (config.implementation == IpImplementation::SOFTWARE) {
        ipBuilder.add_implementation(wire::ImplementationType_SOFTWARE);
    } else {
        ipBuilder.add_implementation(wire::ImplementationType_HARDWARE);
    }

    switch (config.type) {
    case IpType::SLAVE:
        ipBuilder.add_type(wire::IpType_SLAVE);
        break;
    case IpType::SLAVE_LITE:
        ipBuilder.add_type(wire::IpType_SLAVE_LITE);
        break;
    case IpType::SLAVE_STREAM:
        ipBuilder.add_type(wire::IpType_SLAVE_STREAM);
        break;
    case IpType::MASTER:
        ipBuilder.add_type(wire::IpType_MASTER);
        break;
    case IpType::MASTER_LITE:
        ipBuilder.add_type(wire::IpType_MASTER_LITE);
        break;
    case IpType::MASTER_STREAM:
        ipBuilder.add_type(wire::IpType_MASTER_STREAM);
        break;
    default:
        Status st = Status(1, "Unknown IP type: " + std::to_string(int(config.type)));
        return std::make_pair(0, st);
    }
    auto ip = ipBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_IP_INFO);
    msgBuilder.add_ipInfo(ip);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while sending the IP_INFO message to router: ") + strerror(errno));
        return std::make_pair(0, st);
    }

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while receiving response to IP registration ") + strerror(errno));
        return std::make_pair(0, st);
    }

    auto msg = wire::GetMessage(data.data());
    if (msg->type() == wire::Type_ERROR) {
        std::ostringstream o;
        o << "Cannot register IP " << config.name << ": " << msg->errorMessage()->str();
        disconnect();
        return std::make_pair(0, Status(1, o.str()));
    }

    if (msg->type() != wire::Type_IP_ACK) {
        std::ostringstream o;
        o << "Got an unexpected response while registering IP " << config.name << ": " << msg->type();
        disconnect();
        return std::make_pair(0, Status(1, o.str()));
    }

    return std::make_pair(msg->ipId(), Status());
}

Status RouterClient::commitIp() {
    if (state != State::CONNECTED) {
        return Status(1, "Can commit slaves only in CONNECTED mode");
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_COMMIT);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        disconnect();
        return Status(1, std::string("Error while sending the COMMIT message: ") + strerror(errno));
    }

    std::vector<uint8_t> data;
    if (readFromSocket(sock, data) == -1) {
        disconnect();
        return Status(1, std::string("Error while receiving commit acknowledgement: ") + strerror(errno));
    }

    auto msg = wire::GetMessage(data.data());
    if (msg->type() == wire::Type_ERROR) {
        std::ostringstream o;
        o << "Cannot register IP: " << msg->errorMessage()->str();
        disconnect();
        return Status(1, o.str());
    }

    if (msg->type() != wire::Type_ACK) {
        std::ostringstream o;
        o << "Got an unexpected response while committing IP: " << msg->type();
        disconnect();
        return Status(1, o.str());
    }

    state = State::COMMITTED;

    return Status();
}

std::pair<SystemInfo *, Status> RouterClient::retrievePeerInfo() {
    if (state != State::COMMITTED) {
        Status st = Status(1, "The bridge needs to be COMMITTED in order to start");
        return std::make_pair(nullptr, st);
    }

    std::vector<uint8_t> data;
    const wire::Message *msg;

    if (readFromSocket(sock, data) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while receiving the peer info: ") + strerror(errno));
        return std::make_pair(nullptr, st);
    }
    msg = wire::GetMessage(data.data());

    if (msg->type() == wire::Type_SYSTEM_INFO) {
        SystemInfo *si = new SystemInfo();
        si->name = msg->systemInfo()->name()->str();
        si->systemName = msg->systemInfo()->systemName()->str();
        si->pid = msg->systemInfo()->pid();
        si->hostname = msg->systemInfo()->hostname()->str();
        return std::make_pair(si, Status());
    }

    if (msg->type() == wire::Type_ERROR) {
        std::ostringstream o;
        o << "Error while receiving the peer list: " << msg->errorMessage()->str();
        disconnect();
        return std::make_pair(nullptr, Status(1, o.str()));
    }

    if (msg->type() != wire::Type_ACK) {
        std::ostringstream o;
        o << "Got an unexpected response while receiving peer list: " << msg->type();
        disconnect();
        return std::make_pair(nullptr, Status(1, o.str()));
    }

    state = State::PEER_INFO_RECEIVED;

    return std::make_pair(nullptr, Status());
}

std::pair<IpConfig *, Status> RouterClient::retrieveIpConfig() {
    if (state != State::PEER_INFO_RECEIVED) {
        Status st = Status(1, "The client needs to receive all the peer info before receiving IP info");
        return std::make_pair(nullptr, st);
    }

    std::vector<uint8_t> data;
    const wire::Message *msg;

    if (readFromSocket(sock, data) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while receiving the peer info: ") + strerror(errno));
        return std::make_pair(nullptr, st);
    }
    msg = wire::GetMessage(data.data());

    if (msg->type() == wire::Type_IP_INFO) {
        IpConfig *ip = new IpConfig();
        ip->name = msg->ipInfo()->name()->str();
        ip->address = msg->ipInfo()->address();
        ip->size = msg->ipInfo()->size();
        ip->firstInterrupt = msg->ipInfo()->firstInterrupt();
        ip->numInterrupts = msg->ipInfo()->numInterrupts();

        switch (msg->ipInfo()->type()) {
        case wire::IpType_SLAVE:
            ip->type = IpType::SLAVE;
            break;
        case wire::IpType_SLAVE_LITE:
            ip->type = IpType::SLAVE_LITE;
            break;
        case wire::IpType_SLAVE_STREAM:
            ip->type = IpType::SLAVE_STREAM;
            break;
        case wire::IpType_MASTER:
            ip->type = IpType::MASTER;
            break;
        case wire::IpType_MASTER_LITE:
            ip->type = IpType::MASTER_LITE;
            break;
        case wire::IpType_MASTER_STREAM:
            ip->type = IpType::MASTER_STREAM;
            break;
        default:
            Status st = Status(1, "Received a slave of unknown type: " + std::to_string(int(msg->ipInfo()->type())));
            return std::make_pair(nullptr, st);
        }

        ip->implementation = msg->ipInfo()->implementation() == wire::ImplementationType_SOFTWARE ?
                IpImplementation::SOFTWARE :
                IpImplementation::HARDWARE;

        return std::make_pair(ip, Status());
    }

    if (msg->type() == wire::Type_ERROR) {
        std::ostringstream o;
        o << "Error while receiving the IP list: " << msg->errorMessage()->str();
        disconnect();
        return std::make_pair(nullptr, Status(1, o.str()));
    }

    if (msg->type() != wire::Type_ACK) {
        std::ostringstream o;
        o << "Got an unexpected response while receiving IP list: " << msg->type();
        disconnect();
        return std::make_pair(nullptr, Status(1, o.str()));
    }

    state = State::STARTED;

    return std::make_pair(nullptr, Status());
}

std::pair<Transaction *, Status> RouterClient::receiveTransaction() {
    if (state != State::STARTED) {
        Status st = Status(1, "The client needs be started befor receiving transactions");
        return std::make_pair(nullptr, st);
    }

    std::vector<uint8_t> data;
    const wire::Message *msg;

    if (readFromSocket(sock, data) == -1) {
        disconnect();
        Status st = Status(1, std::string("Error while receiving the peer info: ") + strerror(errno));
        return std::make_pair(nullptr, st);
    }
    msg = wire::GetMessage(data.data());

    if (msg->type() == wire::Type_DONE) {
        return std::make_pair(nullptr, Status(DONE, "Done processing"));
    }

    if (msg->type() != wire::Type_TRANSACTION) {
        std::ostringstream o;
        o << "Got an unexpected response instead of a transaction: " << msg->type();
        disconnect();
        return std::make_pair(nullptr, Status(1, o.str()));
    }
    Transaction *txn = new Transaction();

    switch (msg->txn()->type()) {
    case wire::TransactionType_READ_REQ:
        txn->type = TransactionType::READ_REQ;
        break;
    case wire::TransactionType_WRITE_REQ:
        txn->type = TransactionType::WRITE_REQ;
        break;
    case wire::TransactionType_READ_RESP:
        txn->type = TransactionType::READ_RESP;
        break;
    case wire::TransactionType_WRITE_RESP:
        txn->type = TransactionType::WRITE_RESP;
        break;
    default:
        Status st = Status(1, "Received a transaction of unknown type: " + std::to_string(int(msg->txn()->type())));
        return std::make_pair(nullptr, st);
    }

    txn->initiator = msg->txn()->initiator();
    txn->target = msg->txn()->target();
    txn->id = msg->txn()->id();
    txn->address = msg->txn()->address();
    txn->size = msg->txn()->size();
    txn->data.resize(msg->txn()->data()->size());
    memcpy(txn->data.data(), msg->txn()->data()->Data(), msg->txn()->data()->size());
    txn->ok = msg->txn()->ok();
    txn->message = msg->txn()->message()->str();

    return std::make_pair(txn, Status());
}

Status RouterClient::sendTransaction(const Transaction &txn) {
    if (state != State::STARTED) {
        return Status(1, "The client needs be started befor sending transactions");
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    auto errMsg = builder.CreateString(txn.message);
    auto data = builder.CreateVector(txn.data.data(), txn.data.size());

    sw_axi::wire::TransactionBuilder txnBuilder(builder);
    switch (txn.type) {
    case TransactionType::READ_REQ:
        txnBuilder.add_type(wire::TransactionType_READ_REQ);
        break;
    case TransactionType::WRITE_REQ:
        txnBuilder.add_type(wire::TransactionType_WRITE_REQ);
        break;
    case TransactionType::READ_RESP:
        txnBuilder.add_type(wire::TransactionType_READ_RESP);
        break;
    case TransactionType::WRITE_RESP:
        txnBuilder.add_type(wire::TransactionType_WRITE_RESP);
        break;
    default:
        return Status(1, "Unknown transaction type: " + std::to_string(int(txn.type)));
    }

    txnBuilder.add_initiator(txn.initiator);
    txnBuilder.add_target(txn.target);
    txnBuilder.add_id(txn.id);
    txnBuilder.add_address(txn.address);
    txnBuilder.add_size(txn.size);
    txnBuilder.add_data(data);
    txnBuilder.add_ok(txn.ok);
    txnBuilder.add_message(errMsg);
    auto txnData = txnBuilder.Finish();

    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_TRANSACTION);
    msgBuilder.add_txn(txnData);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        disconnect();
        return Status(1, std::string("Error while sending the TERMINATE message: ") + strerror(errno));
    }
    return Status();
}

Status RouterClient::sendTermination(uint64_t id) {
    if (state != State::STARTED) {
        return Status(1, "The client needs be started before sending termination messages");
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_TERMINATE);
    msgBuilder.add_ipId(id);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        disconnect();
        return Status(1, std::string("Error while sending the TERMINATE message: ") + strerror(errno));
    }
    return Status();
}

Status RouterClient::sendLogout() {
    if (state != State::STARTED) {
        return Status(1, "The client needs be started before logging out");
    }

    flatbuffers::FlatBufferBuilder builder(1024);
    sw_axi::wire::MessageBuilder msgBuilder(builder);
    msgBuilder.add_type(sw_axi::wire::Type_DONE);
    builder.Finish(msgBuilder.Finish());

    if (sw_axi::writeToSocket(sock, builder.GetBufferPointer(), builder.GetSize()) == -1) {
        disconnect();
        return Status(1, std::string("Error while sending the DONE message: ") + strerror(errno));
    }

    state = State::LOGGED_OUT;
    return Status();
}

void RouterClient::disconnect() {
    if (sock == -1) {
        return;
    }

    close(sock);
    state = State::DISCONNECTED;
    connectedUri = "";
    sock = -1;
}

}  // namespace sw_axi
