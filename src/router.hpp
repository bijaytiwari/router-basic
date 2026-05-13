#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Interface {
    std::string name;
    std::string ip_prefix;
    std::string admin_state;
    std::string oper_state;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t drops;
    uint64_t errors;
};

struct StaticRoute {
    std::string prefix;
    std::string next_hop_ip;
    std::string egress_interface;
    int preference;
};

struct Route {
    std::string prefix;
    std::string next_hop;
    std::string interface_name;
    int prefix_len;
    int preference;
    std::string source;
    bool active;
};

class Router {
public:
    void load(const std::string &dir);
    void buildRoutingTable();
    void showInterfaces();
    void showRoutes();
    void lookup(const std::string &ip);
    void explainLookup(const std::string &ip);
    void replayEvents(const std::string &dir);

private:
    std::vector<Interface> interfaces;
    std::vector<StaticRoute> staticRoutes;
    std::vector<Route> routes;

    bool isInterfaceUp(const std::string &name);
    bool nextHopReachable(const std::string &ip, std::string &iface);
};
