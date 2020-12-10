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

#include <condition_variable>
#include <mutex>
#include <queue>

namespace sw_axi {

/**
 * A synchronized queue
 */
template<typename T>
class Queue {
public:
    /**
     * Pop the element from the front of the queue and move it to T; wait until there is an element to pop
     *
     * @return true if an element was popped; false if the finish method has been invoked
     */
    bool pop(T &t) {
        std::unique_lock<std::mutex> scopedLock(mutex);

        while (queue.empty()) {
            condVar.wait(scopedLock);
            if (done) {
                return false;
            }
        }

        t = std::move(queue.front());
        queue.pop();
        return true;
    }

    /**
     * Move the item to the back of the queue
     */
    void push(T &&item) {
        mutex.lock();
        queue.push(std::move(item));
        mutex.unlock();
        condVar.notify_one();
    }

    /**
     * The element processing is done; wake the pop caller if necessary
     */
    void finish() {
        mutex.lock();
        done = true;
        mutex.unlock();
        condVar.notify_one();
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    bool done = false;
    std::condition_variable condVar;
};

}  // namespace sw_axi
