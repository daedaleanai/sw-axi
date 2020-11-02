
#include <SwAxi.hh>
#include <iostream>

int main(int argc, char **argv) {
    using namespace sw_axi;

    Bridge bridge;

    if (bridge.connect() != 0) {
        std::cerr << "Unable to connect to the simulation system" << std::endl;
        return 1;
    }

    std::cerr << "Connected simulation system" << std::endl;
    const SystemInfo *info = bridge.getRemoteSystemInfo();
    std::cerr << "Name:     " << info->name << std::endl;
    std::cerr << "Pid:      " << info->pid << std::endl;
    std::cerr << "URI:      " << info->uri << std::endl;
    std::cerr << "Hostname: " << info->hostname << std::endl;

    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
