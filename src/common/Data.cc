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

#include "Data.hh"

#include <iomanip>

namespace sw_axi {
void SystemInfo::print(std::ostream &o) {
    o << std::left << std::setfill(' ') << std::setw(20) << name << " ";
    o << std::left << std::setfill(' ') << std::setw(25) << systemName << " ";
    o << hostname << ":" << pid << std::endl;
}

void IpConfig::print(std::ostream &o) {
    o << ((implementation == IpImplementation::SOFTWARE) ? "[SW]" : "[HW]") << " ";
    switch (type) {
    case IpType::SLAVE:
        o << "[SLAVE        ] ";
        break;
    case IpType::SLAVE_LITE:
        o << "[SLAVE LITE   ] ";
        break;
    case IpType::SLAVE_STREAM:
        o << "[SLAVE STREAM ] ";
        break;
    case IpType::MASTER:
        o << "[MASTER       ] ";
        break;
    case IpType::MASTER_LITE:
        o << "[MASTER LITE  ] ";
        break;
    case IpType::MASTER_STREAM:
        o << "[MASTER STREAM] ";
        break;
    }
    o << std::right;
    o << "address: ";
    o << "[0x" << std::hex << std::setfill('0') << std::setw(16) << address << "+";
    o << "0x" << std::hex << std::setfill('0') << std::setw(16) << size << "] ";
    o << "interrupts: [";
    o << std::dec;
    o << std::setfill('0') << std::setw(5) << firstInterrupt << "+";
    o << std::setfill('0') << std::setw(5) << numInterrupts << "] ";
    o << name << " " << std::endl;
}
}  // namespace sw_axi
