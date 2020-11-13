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
#include <string>

namespace sw_axi {
/**
 * Properties of a connected system
 */
struct SystemInfo {
    std::string name;
    std::string systemName;
    uint64_t pid;
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
    uint64_t address = 0;  //!< Address of the slave
    uint64_t size = 0;  //!< Size of the address space allocated to the slave
    uint16_t firstInterrupt = 0;  //!< Number of the first interrupt allocated to the slave
    uint16_t numInterrupts = 0;  //!< Number of interrupts allocated to the slave
    IpType type = IpType::SLAVE;
    IpImplementation implementation = IpImplementation::SOFTWARE;
};

/**
 * A generic status indicator for operations
 */
class Status {
public:
    Status() {}
    Status(uint32_t code, const std::string &msg) : msg(msg), code(code) {}

    bool isOk() const {
        return code == 0;
    }

    bool isError() const {
        return code != 0;
    }

    const std::string &getMessage() const {
        return msg;
    }

    uint32_t getCode() const {
        return code;
    }

private:
    uint32_t code = 0;
    std::string msg;
};

}  // namespace sw_axi
