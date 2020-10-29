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

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sw_axi {

/**
 * Properties of a connected system
 */
struct SystemInfo {
    std::string name;
    uint64_t pid;
    std::string uri;
    std::string hostname;
};

/**
 * Type of the IP registered IP
 */
enum class IpType { SLAVE, SLAVE_LITE, SLAVE_STREAM, MASTER, MASTER_LITE, MASTER_STREAM };

/**
 * Implementation type of the registered IP
 */
enum class IpImplementation { SOFTWARE, HARDWARE };

/**
 * Information about an IP block
 */
struct IpConfig {
    std::string name;
    uint16_t startInterrupt = 0;  //!< Start of the interrupt range
    uint16_t endInterrupt = 0;  //!< End of the interrupt range
    uint64_t startAddress = 0;  //!< Start of the address range
    uint64_t endAddress = 0;  //!< End of the address range
    IpType type = IpType::SLAVE;
    IpImplementation implementation = IpImplementation::SOFTWARE;
};

/**
 * Bus transaction data
 */
struct Buffer {
    uint8_t *data;  //!< Payload of the transaction for write requests, buffer to be filled by read requests
    uint32_t size;  //!< Size of the data buffer in bytes
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

/**
 * Bridge is the software entry point of the infrastructure.
 *
 * The bridge connects to a server, registers its IP components and receives the list of the components already
 * registered with the server. It can then send and receive requests directed at particular slaves as well as send and
 * receive interrupt messages.
 */
class Bridge {
public:
    ~Bridge();

    /**
     * Connect to the SystemVerilog simulator
     *
     * @param uri an URI pointing to a rendez-vous point with the simulator; currently only UNIX domain sockets are
     *            supported
     *
     * @return 0 on success; -1 on failure
     */
    int connect(std::string uri = "unix:///tmp/sw-axi");

    /**
     * Retrieve the information about the remote system.
     *
     * @return a filled SystemInfo struct if the bridge is connected; a null pointer if it isn't
     */
    const SystemInfo *getRemoteSystemInfo() const;

    /**
     * Register a software slave with the given parameters. The bridge takes the ownership of thw slave object.
     *
     * @return 0 on success; -1 on failure
     */
    int registerSlave(Slave *slave, const IpConfig &config);

    /**
     * Confirm that all IP has been registered.
     *
     * Calling this method will make the subsequent calls to `registerSlave` fail.
     *
     * @return 0 on success; -1 on failure
     */
    int commitIp();

    /**
     * Start the bridge and make it handle the transaction traffic.
     *
     * @return 0 on success; -1 on failure
     */
    int start();

    /**
     * Enumerate the IP connected to the bridge
     *
     * @param ip reference to a vector to be filled with the IP information
     *
     * @return 0 on success; -1 on failure
     */
    int enumerateIp(std::vector<IpConfig> &ip);

    /**
     * Disconnect from the simulator.
     */
    void disconnect();

private:
    enum class State {
        DISCONNECTED,
        CONNECTED,
        COMMITTED,
        STARTED,
    };

    struct SlaveData {
        uint64_t endAddr;
        Slave *slave;
    };

    State state = State::DISCONNECTED;
    std::string connectedUri;
    int sock = -1;
    std::unique_ptr<SystemInfo> systemInfo;
    std::vector<IpConfig> ipBlocks;
    std::map<uint64_t, SlaveData> slaveMap;
};

}  // namespace sw_axi
