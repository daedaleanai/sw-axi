
#include <SwAxi.hh>
#include <iostream>

int main(int argc, char **argv) {
    using namespace sw_axi;

    Bridge bridge;

    if (bridge.connect() != 0) {
        std::cerr << "Unable to connect to the simulator" << std::endl;
        return 1;
    }

    std::cerr << "Connected to the simulator" << std::endl;
    const SimulatorInfo *info = bridge.getSimulatorInfo();
    std::cerr << "Simulator: " << info->simulatorName << std::endl;
    std::cerr << "Pid:       " << info->pid << std::endl;
    std::cerr << "URI:       " << info->uri << std::endl;
    std::cerr << "Hostname:  " << info->hostname << std::endl;

    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
