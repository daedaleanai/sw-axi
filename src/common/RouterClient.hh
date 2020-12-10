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

#include "Data.hh"

#include <cstdint>
#include <utility>

namespace sw_axi {

class RouterClient {
public:
    /**
     * Valid states of the client
     */
    enum class State {
        DISCONNECTED,
        CONNECTED,
        COMMITTED,
        PEER_INFO_RECEIVED,
        STARTED,
        LOGGED_OUT,
    };

    /**
     * Signal for the server completion message
     */
    const uint32_t DONE = 0xffffffff;

    /**
     * Connect to the SystemVerilog simulator
     *
     * @param uri  an URI pointing to a rendez-vous point with the simulator; currently only UNIX domain sockets are
     *             supported
     * @param name name of the client
     *
     * @return     a system info-status pair; the system info pointer is null on failure; the user is responsible
     *             for freeing the systeminfo object
     */
    std::pair<SystemInfo *, Status> connect(const std::string &uri, const std::string &name);

    /**
     * Register an IP block with the given parameters
     *
     * @return a slave id-status pair; the ID is invalid if the status indicates failure
     */
    std::pair<uint64_t, Status> registerIp(const IpConfig &config);

    /**
     * Confirm that all IP has been registered.
     *
     * Calling this method will make the subsequent calls to `registerSlave` fail.
     */
    Status commitIp();

    /**
     * Retrieve the system information of a peer
     *
     * @return a system info-status pair; the user takes the ownership of the system info object; the system info
     *         pointer is null on failure; if there is no failure and the system info pointer is null then the
     *         peer info list is finished
     */
    std::pair<SystemInfo *, Status> retrievePeerInfo();

    /**
     * Retrieve the information about all the IP registered with the router
     *
     * @return a IP config-status pair; the user takes the ownership of the IP config object; the IP config
     *         pointer is null on failure; if there is no failure and the IP config pointer is null then the
     *         IP list is finished
     */
    std::pair<IpConfig *, Status> retrieveIpConfig();

    /**
     * Retrieves a transaction sent by the router
     *
     * @return the status of the operation and the transaction upon success; if the status code is equal to TERMINATED
     *         then no new transaction will arrive; the caller takes the ownership of the transaction object
     */
    std::pair<Transaction *, Status> receiveTransaction();

    /**
     * Sends a transaction to the router
     */
    Status sendTransaction(const Transaction &txn);

    /**
     * Send a master termination notification
     *
     * @param id id of the master that has been terminated
     */
    Status sendTermination(uint64_t id);

    /**
     * Send a logout message
     */
    Status sendLogout();

    /**
     * Disconnect from server
     *
     * It is safe to call this method multiple times.
     */
    void disconnect();

    /**
     * Get the current state of the client
     */
    State getState() const {
        return state;
    }

private:
    State state = State::DISCONNECTED;
    std::string connectedUri;
    int sock = -1;
};

}  // namespace sw_axi
