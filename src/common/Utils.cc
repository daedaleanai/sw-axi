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

#include "Utils.hh"

#include <unistd.h>

namespace sw_axi {

std::ostream devNull(0);

int readFromSocket(int sock, std::vector<uint8_t> &buffer) {
    if (sock == -1) {
        return -1;
    }

    uint64_t size = 0;
    if (read(sock, &size, sizeof(size)) != sizeof(size)) {
        return -1;
    }

    buffer.resize(size);
    uint8_t *ptr = buffer.data();

    while (size) {
        ssize_t rd = read(sock, ptr, size);
        if (rd == -1) {
            return -1;
        }
        size -= rd;
        ptr += rd;
    }

    return 0;
}

int writeToSocket(int sock, const uint8_t *buffer, size_t size) {
    if (sock == -1) {
        return -1;
    }

    uint64_t sz = size;
    if (write(sock, &sz, sizeof(sz)) != sizeof(sz)) {
        return -1;
    }

    const uint8_t *ptr = buffer;
    while (size) {
        ssize_t written = write(sock, ptr, size);
        if (written == -1) {
            return -1;
        }
        size -= written;
        ptr += written;
    }

    return 0;
}

}  // namespace sw_axi
