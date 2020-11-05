
#include <iomanip>
#include <iostream>

#include <SwAxi.hh>

class Ram : public sw_axi::Slave {
public:
    virtual int handleWrite(const sw_axi::Buffer *buffer) {
        return 0;
    }
    virtual int handleRead(sw_axi::Buffer *buffer) {
        return 0;
    }
};

int main(int argc, char **argv) {
    using namespace sw_axi;

    Bridge bridge("01-handshake-cpp");

    if (bridge.connect() != 0) {
        std::cerr << "Unable to connect to the router" << std::endl;
        return 1;
    }

    std::cerr << "Connected to the router:" << std::endl;
    const SystemInfo *rInfo = bridge.getRouterInfo();
    std::cerr << "Name:        " << rInfo->name << std::endl;
    std::cerr << "System Name: " << rInfo->systemName << std::endl;
    std::cerr << "Pid:         " << rInfo->pid << std::endl;
    std::cerr << "Hostname:    " << rInfo->hostname << std::endl;
    std::cerr << std::endl;

    Ram *ram = new Ram;
    IpConfig ramConfig = {
            .name = "Soft-RAM",
            .address = 0x1000,
            .size = 0x2000,
            .firstInterrupt = 0,
            .numInterrupts = 0,
            .type = IpType::SLAVE_LITE,
            .implementation = IpImplementation::SOFTWARE};

    if (bridge.registerSlave(ram, ramConfig) != 0) {
        std::cerr << "Unable to register the Soft-RAM slave" << std::endl;
        return 1;
    }

    if (bridge.commitIp() != 0) {
        std::cerr << "Unable to commit the IP" << std::endl;
        return 1;
    }

    if (bridge.start() != 0) {
        std::cerr << "Unable to start the bridge" << std::endl;
        return 1;
    }

    std::vector<SystemInfo> peers;
    if (bridge.enumeratePeers(peers) != 0) {
        std::cerr << "Unable to enumerate peers" << std::endl;
        return 1;
    }

    std::cerr << "Peers: " << std::endl;
    for (auto &peer : peers) {
        std::cerr << std::left << std::setfill(' ') << std::setw(20) << peer.name << " ";
        std::cerr << std::left << std::setfill(' ') << std::setw(20) << peer.systemName << " ";
        std::cerr << peer.hostname << ":" << peer.pid << std::endl;
    }
    std::cerr << std::endl;

    std::vector<IpConfig> ipBlocks;
    if (bridge.enumerateIp(ipBlocks) != 0) {
        std::cerr << "Unable to enumerate the IP" << std::endl;
        return 1;
    }

    if (ipBlocks.empty()) {
        std::cerr << "No IP blocks registered" << std::endl;
    } else {
        std::cerr << "IP blocks:" << std::endl;
    }

    for (auto &ip : ipBlocks) {
        std::cerr << ((ip.implementation == IpImplementation::SOFTWARE) ? "[SW]" : "[HW]") << " ";
        switch (ip.type) {
        case IpType::SLAVE_LITE:
            std::cerr << "[SLAVE LITE] ";
            break;
        }
        std::cerr << std::right;
        std::cerr << "address: ";
        std::cerr << "[0x" << std::hex << std::setfill('0') << std::setw(16) << ip.address << "+";
        std::cerr << "0x" << std::hex << std::setfill('0') << std::setw(16) << ip.size << "] ";
        std::cerr << "interrupts: [";
        std::cerr << std::dec;
        std::cerr << std::setfill('0') << std::setw(5) << ip.firstInterrupt << "+";
        if (ip.numInterrupts == 0)
            std::cerr << std::setfill('0') << std::setw(5) << ip.numInterrupts << "] ";
        std::cerr << ip.name << " " << std::endl;
    }

    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
