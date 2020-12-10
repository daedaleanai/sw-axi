
#include <SwAxi.hh>

#include <cstring>
#include <iostream>
#include <thread>

class Ram : public sw_axi::Slave {
public:
    Ram(uint64_t addr, uint64_t size) {
        data = new uint8_t[size];
    }
    ~Ram() {
        delete[] data;
    }
    virtual int handleWrite(const sw_axi::Buffer *buffer) {
        memcpy(data + (buffer->address - addr), buffer->data, buffer->size);
        return 0;
    }
    virtual int handleRead(sw_axi::Buffer *buffer) {
        memcpy(buffer->data, data + (buffer->address - addr), buffer->size);
        return 0;
    }

private:
    uint8_t *data;
    uint64_t addr;
};

int main(int argc, char **argv) {
    using namespace sw_axi;

    Bridge bridge("02-sw-master-lite");

    Status st = bridge.connect();
    if (st.isError()) {
        std::cerr << "Unable to connect to the router: " << st.getMessage() << std::endl;
        return 1;
    }

    std::cerr << "Connected to the router:" << std::endl;
    const SystemInfo *rInfo = bridge.getRouterInfo();
    std::cerr << "Name:        " << rInfo->name << std::endl;
    std::cerr << "System Name: " << rInfo->systemName << std::endl;
    std::cerr << "Pid:         " << rInfo->pid << std::endl;
    std::cerr << "Hostname:    " << rInfo->hostname << std::endl;
    std::cerr << std::endl;

    const uint64_t RAM_SIZE = 0x2000;
    const uint64_t RAM_ADDR = 0x1000;
    Ram *ram = new Ram(RAM_ADDR, RAM_SIZE);
    IpConfig ramConfig = {
            .name = "Soft-RAM",
            .address = RAM_ADDR,
            .size = RAM_SIZE,
            .firstInterrupt = 0,
            .numInterrupts = 0,
            .type = IpType::SLAVE_LITE,
            .implementation = IpImplementation::SOFTWARE};

    st = bridge.registerSlave(ram, ramConfig);
    if (st.isError()) {
        std::cerr << "Unable to register the Soft-RAM slave: " << st.getMessage() << std::endl;
        return 1;
    }

    std::pair<Master *, Status> ret = bridge.registerMaster("Soft-Master");
    if (ret.second.isError()) {
        std::cerr << "Unable register a master IP: " << ret.second.getMessage() << std::endl;
        return 1;
    }
    Master *master = ret.first;

    st = bridge.commitIp();
    if (st.isError()) {
        std::cerr << "Unable to commit the IP: " << st.getMessage() << std::endl;
        return 1;
    }

    st = bridge.start();
    if (st.isError()) {
        std::cerr << "Unable to start the bridge: " << st.getMessage() << std::endl;
        return 1;
    }

    std::vector<SystemInfo> peers;
    st = bridge.enumeratePeers(peers);
    if (st.isError()) {
        std::cerr << "Unable to enumerate peers: " << st.getMessage() << std::endl;
        return 1;
    }

    std::cerr << "Peers: " << std::endl;
    for (auto &peer : peers) {
        peer.print(std::cerr);
    }
    std::cerr << std::endl;

    std::vector<IpConfig> ipBlocks;
    st = bridge.enumerateIp(ipBlocks);
    if (st.isError()) {
        std::cerr << "Unable to enumerate the IPs: " << st.getMessage() << std::endl;
        return 1;
    }

    if (ipBlocks.empty()) {
        std::cerr << "No IP blocks registered" << std::endl;
    } else {
        std::cerr << "IP blocks:" << std::endl;
    }

    for (auto &ip : ipBlocks) {
        ip.print(std::cerr);
    }

    std::thread t([&]() {
        const char hello[] = "Hello world!";
        uint8_t rBuffer[sizeof(hello)];
        uint8_t wBuffer[sizeof(hello)];
        memcpy(wBuffer, hello, sizeof(hello));
        Buffer writeBuffer = {
                .data = wBuffer,
                .size = sizeof(hello),
                .address = 0x1000,
        };
        Buffer readBuffer = {
                .data = rBuffer,
                .size = sizeof(hello),
                .address = 0x1000,
        };

        auto writeStatus = master->write(&writeBuffer);
        auto readStatus = master->read(&readBuffer);

        auto st = writeStatus.get();
        if (st.isError()) {
            std::cerr << "Write failed: " << st.getMessage() << std::endl;
            master->terminate();
            return;
        }

        st = readStatus.get();
        if (st.isError()) {
            std::cerr << "Read failed: " << st.getMessage() << std::endl;
            master->terminate();
            return;
        }

        std::cout << "Wrote: " << wBuffer << std::endl;
        std::cout << "Read:  " << rBuffer << std::endl;
        std::cout << (std::string((char *)wBuffer) == std::string((char *)rBuffer) ? "MATCH!" : "NO MATCH!");
        std::cout << std::endl;

        std::cerr << "Terminating the master" << std::endl;
        master->terminate();
    });

    st = bridge.waitForCompletion();
    if (st.isError()) {
        std::cerr << "Failed to complete without errors: " << st.getMessage() << std::endl;
        return 1;
    }

    t.join();

    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
