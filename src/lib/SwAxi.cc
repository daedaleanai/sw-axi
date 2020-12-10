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

#include <cstring>

namespace sw_axi {

std::future<Status> Master::read(Buffer *buffer) {
    Txn txn;
    txn.type = TxnType::TRANSACTION;
    txn.buffer = buffer->data;
    txn.txn.reset(new Transaction);
    txn.txn->type = TransactionType::READ_REQ;
    txn.txn->initiator = id;
    txn.txn->address = buffer->address;
    txn.txn->size = buffer->size;
    txn.txn->ok = true;
    queue->push(std::move(txn));
    return txn.promise.get_future();
}

std::future<Status> Master::write(const Buffer *buffer) {
    Txn txn;
    txn.type = TxnType::TRANSACTION;
    txn.txn.reset(new Transaction);
    txn.txn->type = TransactionType::WRITE_REQ;
    txn.txn->initiator = id;
    txn.txn->address = buffer->address;
    txn.txn->size = buffer->size;
    txn.txn->data.resize(buffer->size);
    memcpy(txn.txn->data.data(), buffer->data, buffer->size);
    txn.txn->ok = true;
    queue->push(std::move(txn));
    return txn.promise.get_future();
}

void Master::terminate() {
    Txn txn;
    txn.type = TxnType::TERMINATION, txn.txn.reset(new Transaction);
    txn.txn->id = id;
    queue->push(std::move(txn));
}

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
    std::pair<uint64_t, Status> ret = client->registerIp(config);
    if (ret.second.isError()) {
        return ret.second;
    }
    slaveMap[ret.first] = slave;
    return Status();
}

std::pair<Master *, Status> Bridge::registerMaster(const std::string &name) {
    IpConfig config = {.name = name, .type = IpType::MASTER};
    std::pair<uint64_t, Status> ret = client->registerIp(config);
    if (ret.second.isError()) {
        return std::make_pair(nullptr, ret.second);
    }

    Master *m = new Master(ret.first, &queue);
    masterMap[ret.first].master = m;
    return std::make_pair(m, Status());
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

    readerThread = std::thread(startReader, this);
    writerThread = std::thread(startWriter, this);
    return Status();
}

Status Bridge::waitForCompletion() {
    readerThread.join();
    writerThread.join();

    if (readerStatus.isError()) {
        return readerStatus;
    }

    if (writerStatus.isError()) {
        return writerStatus;
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

    for (auto &entry : masterMap) {
        delete entry.second.master;
    }
    masterMap.clear();
}

void Bridge::reader() {
    while (true) {
        auto ret = client->receiveTransaction();
        if (ret.second.isError()) {
            if (ret.second.getCode() != client->DONE) {
                readerStatus = ret.second;
            } else {
                readerStatus = Status();
            }
            queue.finish();
            return;
        }

        std::unique_ptr<Transaction> txn(ret.first);

        if (txn->type == TransactionType::READ_RESP || txn->type == TransactionType::WRITE_RESP) {
            const std::lock_guard<std::mutex> lock(masterMapMutex);
            if (masterMap.find(txn->initiator) == masterMap.end()) {
                readerStatus = Status(1, "Got a response for an unknown master: " + std::to_string(txn->initiator));
                queue.finish();
                return;
            }
            auto &masterMd = masterMap[txn->initiator];

            if (masterMd.txns.find(txn->id) == masterMd.txns.end()) {
                readerStatus = Status(1, "Got a response for an unknown request: " + std::to_string(txn->id));
                queue.finish();
                return;
            }
            auto mTxn = std::move(masterMd.txns[txn->id]);
            masterMd.txns.erase(txn->id);

            auto st = Status();
            if (!txn->ok) {
                st = Status(1, txn->message);
            }

            if (txn->type == TransactionType::READ_RESP && txn->ok) {
                memcpy(mTxn.buffer, txn->data.data(), txn->data.size());
            }

            mTxn.promise.set_value(st);
        } else if (txn->type == TransactionType::READ_REQ || txn->type == TransactionType::WRITE_REQ) {
            if (slaveMap.find(txn->target) == slaveMap.end()) {
                readerStatus = Status(1, "Got a request meant for an unknown slave: " + std::to_string(txn->target));
                queue.finish();
                return;
            }

            Transaction *respTxn = new Transaction;
            ;
            Slave *s = slaveMap[txn->target];
            int ret = 0;
            Buffer b = {.size = txn->size, .address = txn->address};

            if (txn->type == TransactionType::WRITE_REQ) {
                b.data = txn->data.data();
                respTxn->type = TransactionType::WRITE_RESP;
                ret = s->handleWrite(&b);
            } else {
                respTxn->data.resize(txn->size);
                b.data = respTxn->data.data();
                respTxn->type = TransactionType::READ_RESP;
                ret = s->handleRead(&b);
            }

            respTxn->initiator = txn->initiator;
            respTxn->target = txn->target;
            respTxn->id = txn->id;
            respTxn->address = txn->address;
            respTxn->size = txn->size;
            respTxn->ok = true;

            if (ret) {
                respTxn->data.clear();
                respTxn->ok = false;
                respTxn->message = "Slave operation failed";
            }

            Master::Txn mTxn;
            mTxn.type = Master::TxnType::TRANSACTION;
            mTxn.txn.reset(respTxn);
            queue.push(std::move(mTxn));
        }
    }
}

void Bridge::writer() {
    while (true) {
        Master::Txn txn;
        if (!queue.pop(txn)) {
            writerStatus = Status();
            writerStatus = client->sendLogout();
            return;
        }

        if (txn.type == Master::TxnType::TERMINATION) {
            Status st = client->sendTermination(txn.txn->id);
            if (st.isError()) {
                writerStatus = st;
                return;
            }
            continue;
        }

        if (txn.txn->type == TransactionType::READ_REQ || txn.txn->type == TransactionType::WRITE_REQ) {
            const std::lock_guard<std::mutex> lock(masterMapMutex);
            txn.txn->id = masterMap[txn.txn->initiator].lastTxnId++;
            masterMap[txn.txn->initiator].txns[txn.txn->id] = std::move(txn);
        }

        Status st = client->sendTransaction(*txn.txn);
        if (st.isError()) {
            writerStatus = st;
            return;
        }
    }
}

void Bridge::startReader(sw_axi::Bridge *b) {
    b->reader();
}
void Bridge::startWriter(sw_axi::Bridge *b) {
    b->writer();
}

}  // namespace sw_axi
