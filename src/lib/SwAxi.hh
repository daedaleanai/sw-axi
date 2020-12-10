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

#pragma once

#include "../common/Data.hh"
#include "Queue.hh"

#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace sw_axi {

class Bridge;

/**
 * Bus transaction data
 */
struct Buffer {
    uint8_t *data;  //!< Payload of the transaction for write requests, buffer to be filled by read requests
    uint64_t size;  //!< Size of the data buffer in bytes
    uint64_t address;  //!< Address of the transaction
};

/**
 * The interface for implementing software slaves
 */
class Slave {
public:
    virtual ~Slave() {}

    /**
     * Handle the write transaction specified by the argument
     *
     * @return 0 on success; -1 on failure
     */
    virtual int handleWrite(const Buffer *buffer) = 0;

    /**
     * Handle the read transaction specified by the argument and fill the supplied data buffer
     *
     * @return 0 on success; -1 on failure
     */
    virtual int handleRead(Buffer *buffer) = 0;
};

class Master {
    friend class Bridge;

public:
    /**
     * Issue a read transaction specified by the argument
     *
     * @return A future containing status of the operation when it completes; upon success, the buffer
     *         will be filled with the requested data
     */
    std::future<Status> read(Buffer *buffer);

    /**
     * Issue a write transaction specified by the argument
     *
     * @return A future containing status of the operation when it completes; upon success, the buffer
     *         will have been written to the associated slave device
     */
    std::future<Status> write(const Buffer *buffer);

    /**
     * Terminate the master; no further operation will be allowed
     */
    void terminate();

private:
    enum class TxnType { TRANSACTION, TERMINATION };

    struct Txn {
        TxnType type;
        void *buffer = nullptr;
        std::unique_ptr<Transaction> txn;
        std::promise<Status> promise;  //!< Promise to be fulfilled when the response arrives
    };

    Master(uint64_t id, Queue<Txn> *queue) : id(id), queue(queue) {}
    uint64_t id;
    Queue<Txn> *queue;
};

class RouterClient;

/**
 * Bridge is the software entry point of the infrastructure.
 *
 * The bridge connects to a router, registers its IP components and receives the list of the components already
 * registered with the router. It can then send and receive requests directed at particular slaves as well as send and
 * receive interrupt messages.
 */
class Bridge {
public:
    Bridge(const std::string &name = "unnamed");
    ~Bridge();

    /**
     * Connect to the router
     *
     * @param uri an URI pointing to a rendez-vous point with the router; currently only UNIX domain sockets are
     *            supported
     */
    Status connect(std::string uri = "unix:///tmp/sw-axi");

    /**
     * Retrieve the information about the router
     *
     * @return a filled SystemInfo struct if the bridge is connected; a null pointer if it isn't
     */
    const SystemInfo *getRouterInfo() const;

    /**
     * Register a software slave with the given parameters. The bridge takes the ownership of thw slave object.
     */
    Status registerSlave(Slave *slave, const IpConfig &config);

    /**
     * Registers a master. The bridge owns the object.
     */
    std::pair<Master *, Status> registerMaster(const std::string &name);

    /**
     * Confirm that all IP has been registered.
     *
     * Calling this method will make the subsequent calls to `registerSlave` fail.
     */
    Status commitIp();

    /**
     * Start the bridge and make it handle the transaction traffic.
     */
    Status start();

    /**
     * Wait for the processing to finish
     */
    Status waitForCompletion();

    /**
     * Enumerate the available IP blocks
     *
     * @param ip reference to a vector to be filled with the IP information
     */
    Status enumerateIp(std::vector<IpConfig> &ip);

    /**
     * Enumerate the connected peers
     *
     * @param peers reference to a vector to be filled with peer information
     */
    Status enumeratePeers(std::vector<SystemInfo> &si);

    /**
     * Disconnect from the router.
     */
    void disconnect();

private:
    struct MasterMd {
        Master *master = nullptr;
        std::map<uint64_t, Master::Txn> txns;
        uint64_t lastTxnId = 0;
    };

    void reader();
    void writer();
    static void startReader(Bridge *b);
    static void startWriter(Bridge *b);

    RouterClient *client;
    std::unique_ptr<SystemInfo> routerInfo;
    std::vector<SystemInfo> peers;
    std::vector<IpConfig> ipBlocks;
    std::map<uint64_t, Slave *> slaveMap;
    std::map<uint64_t, MasterMd> masterMap;
    std::string name;
    Queue<Master::Txn> queue;
    std::thread readerThread;
    std::thread writerThread;
    Status readerStatus;
    Status writerStatus;
    std::mutex masterMapMutex;
};

}  // namespace sw_axi
