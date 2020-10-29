
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

    Bridge bridge;

    if (bridge.connect() != 0) {
        std::cerr << "Unable to connect to the simulator" << std::endl;
        return 1;
    }

    std::cerr << "Connected to the simulation system" << std::endl;
    const SystemInfo *info = bridge.getRemoteSystemInfo();
    std::cerr << "Name:     " << info->name << std::endl;
    std::cerr << "Pid:      " << info->pid << std::endl;
    std::cerr << "URI:      " << info->uri << std::endl;
    std::cerr << "Hostname: " << info->hostname << std::endl;

    Ram *ram = new Ram;
    IpConfig ramConfig = {
            .name = "Soft-RAM",
            .startInterrupt = 0,
            .endInterrupt = 0,
            .startAddress = 0x1000,
            .endAddress = 0x2000,
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
        std::cerr << "address: ";
        std::cerr << "[0x" << std::hex << std::setfill('0') << std::setw(16) << ip.startAddress << ":";
        std::cerr << "0x" << std::hex << std::setfill('0') << std::setw(16) << ip.endAddress << "] ";
        std::cerr << "interrupts: [";
        std::cerr << std::setfill('0') << std::setw(5) << ip.startInterrupt << ":";
        std::cerr << std::setfill('0') << std::setw(5) << ip.endInterrupt << "] ";
        std::cerr << ip.name << " " << std::endl;
    }

    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
