
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

    Ram *ram = new Ram;
    IpConfig ramConfig = {
            .name = "Soft-RAM",
            .address = 0x1000,
            .size = 0x2000,
            .firstInterrupt = 0,
            .numInterrupts = 0,
            .type = IpType::SLAVE_LITE,
            .implementation = IpImplementation::SOFTWARE};

    st = bridge.registerSlave(ram, ramConfig);
    if (st.isError()) {
        std::cerr << "Unable to register the Soft-RAM slave: " << st.getMessage() << std::endl;
        return 1;
    }

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
        std::cerr << "Unable to enumerate the IP " << st.getMessage() << std::endl;
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

    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
