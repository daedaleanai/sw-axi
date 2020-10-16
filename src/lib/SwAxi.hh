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
#include <memory>
#include <string>

namespace sw_axi {

/**
 * Properties of the simulator that runs the hardware side of the system.
 */
struct SimulatorInfo {
    std::string simulatorName;
    uint64_t pid;
    std::string uri;
    std::string hostname;
};

/**
 * Bridge is the entry point of the glue infrastructure between the software and the hardware.
 */
class Bridge {
public:
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
     * Retrieve the information about the hardware simulator.
     *
     * @return a filled SimulatorInfo struct if the bridge is connected; a null pointer if it isn't
     */
    const SimulatorInfo *getSimulatorInfo() const;

    /**
     * Disconnect from the simulator.
     */
    void disconnect();

private:
    std::string connectedUri;
    int sock = -1;
    std::unique_ptr<SimulatorInfo> simulatorInfo;
};

}  // namespace sw_axi
