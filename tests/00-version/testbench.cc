
#include <SwAxi.hh>
#include <iostream>

int main(int argc, char **argv) {
    using namespace sw_axi;

    Bridge bridge("00-version");

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
    std::cerr << "Disconnecting" << std::endl;
    bridge.disconnect();

    return 0;
}
