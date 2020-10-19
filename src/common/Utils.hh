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

#include "flatbuffers/flatbuffers.h"

#include <cstdint>
#include <ostream>
#include <vector>

#ifdef VERBOSE
#define LOG std::cerr
#else
#define LOG sw_axi::devNull
#endif

namespace sw_axi {

extern std::ostream devNull;  //!< An ostream pointing to nowhere

/**
 * Read a variable size message from a socket and put the contents in the buffer vector.
 *
 * The message body need to be preceeded on the wire with an 64-bit little endian unsigned integer indicating the size
 * of the message. The body of the message containing exactly the declared amount of bytes must follow.
 *
 * @return 0 on success; -1 on failure
 */
int readFromSocket(int sock, std::vector<uint8_t> &buffer);

/**
 * Write a message buffer to a socket.
 *
 * The buffer size is written first as a 64-bit little endian unsigned integer; the body of the message follows.
 *
 * @return 0 on success; -1 on failure
 */
int writeToSocket(int sock, const uint8_t *buffer, size_t size);

}  // namespace sw_axi
