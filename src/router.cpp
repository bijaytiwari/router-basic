#include "router.hpp"
#include "utils.hpp"
#include "json.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

void Router::load(const std::string &dir) {
    interfaces.clear();
    staticRoutes.clear();
    routes.clear();

    std::ifstream ifs(dir + "/interfaces.json");
    json ifaceJson;
    ifs >> ifaceJson;

    for (auto &i : ifaceJson) {
        Interface iface;
        iface.name = i["name"];
        iface.ip_prefix = i["ip_prefix"];
        iface.admin_state = i["admin_state"];
        iface.oper_state = i["oper_state"];
        iface.rx_packets = i["rx_packets"];
        iface.tx_packets = i["tx_packets"];
        iface.rx_bytes = i["rx_bytes"];
        iface.tx_bytes = i["tx_bytes"];
        iface.drops = i.value("drops", 0);
        iface.errors = i.value("errors", 0);
        interfaces.push_back(iface);
    }

    std::ifstream sfs(dir + "/static_routes.json");
    json routeJson;
    sfs >> routeJson;

    for (auto &r : routeJson) {
        StaticRoute sr;
        sr.prefix = r["prefix"];
        sr.preference = r["preference"];

        if (r.contains("next_hop_ip"))
            sr.next_hop_ip = r["next_hop_ip"];

        if (r.contains("egress_interface"))
            sr.egress_interface = r["egress_interface"];

        staticRoutes.push_back(sr);
    }

    buildRoutingTable();
}

bool Router::isInterfaceUp(const std::string &name) {
    for (auto &i : interfaces) {
        if (i.name == name) {
            return i.admin_state == "up" && i.oper_state == "up";
        }
    }
    return false;
}

bool Router::nextHopReachable(const std::string &ip, std::string &iface) {
    uint32_t target = ipToInt(ip);

    for (auto &r : routes) {
        if (r.source != "connected")
            continue;

        auto [net, prefix] = parseCIDR(r.prefix);

        if (ipInSubnet(target, net, prefix)) {
            iface = r.interface_name;
            return true;
        }
    }

    return false;
}

void Router::buildRoutingTable() {
    routes.clear();

    for (auto &i : interfaces) {
        if (!(i.admin_state == "up" && i.oper_state == "up"))
            continue;

        auto [ip, prefix] = parseCIDR(i.ip_prefix);
        uint32_t mask = maskFromPrefix(prefix);
        uint32_t network = ip & mask;

        struct in_addr addr;
        addr.s_addr = htonl(network);

        Route r;
        r.prefix = std::string(inet_ntoa(addr)) + "/" + std::to_string(prefix);
        r.interface_name = i.name;
        r.next_hop = "direct";
        r.preference = 0;
        r.prefix_len = prefix;
        r.source = "connected";
        r.active = true;

        routes.push_back(r);
    }

    for (auto &s : staticRoutes) {
        Route r;
        r.prefix = s.prefix;
        r.preference = s.preference;
        r.source = "static";
        r.active = false;

        auto [_, prefix] = parseCIDR(s.prefix);
        r.prefix_len = prefix;

        if (!s.egress_interface.empty()) {
            if (isInterfaceUp(s.egress_interface)) {
                r.interface_name = s.egress_interface;
                r.next_hop = "direct";
                r.active = true;
            }
        } else if (!s.next_hop_ip.empty()) {
            std::string iface;
            if (nextHopReachable(s.next_hop_ip, iface)) {
                r.interface_name = iface;
                r.next_hop = s.next_hop_ip;
                r.active = true;
            }
        }

        routes.push_back(r);
    }
}

void Router::showInterfaces() {
    std::cout << "\nInterfaces\n";

    for (auto &i : interfaces) {
        std::cout
            << i.name
            << " state="
            << ((i.admin_state == "up" && i.oper_state == "up") ? "UP" : "DOWN")
            << " rx_pkts=" << i.rx_packets
            << " tx_pkts=" << i.tx_packets
            << " rx_bytes=" << i.rx_bytes
            << " tx_bytes=" << i.tx_bytes
            << " drops=" << i.drops
            << " errors=" << i.errors
            << "\n";
    }
}

void Router::showRoutes() {
    std::cout << "\nRouting Table\n";

    for (auto &r : routes) {
        if (!r.active)
            continue;

        std::cout
            << r.prefix
            << " via " << r.next_hop
            << " dev " << r.interface_name
            << " pref=" << r.preference
            << " [" << r.source << "]"
            << "\n";
    }
}

void Router::lookup(const std::string &ip) {
    uint32_t target = ipToInt(ip);
    Route *best = nullptr;

    for (auto &r : routes) {
        if (!r.active)
            continue;

        auto [net, prefix] = parseCIDR(r.prefix);

        if (ipInSubnet(target, net, prefix)) {
            if (!best || prefix > best->prefix_len ||
                (prefix == best->prefix_len && r.preference < best->preference)) {
                best = &r;
            }
        }
    }

    if (!best) {
        std::cout << "No route found\n";
        return;
    }

    std::cout
        << "Matched route: " << best->prefix
        << " via " << best->next_hop
        << " dev " << best->interface_name
        << "\n";
}

void Router::explainLookup(const std::string &ip) {
    std::cout << "Explain lookup for " << ip << "\n";
    lookup(ip);
}

void Router::replayEvents(const std::string &dir) {
    std::ifstream efs(dir + "/events.json");
    json events;
    efs >> events;

    for (auto &e : events) {
        std::string type = e["type"];

        if (type == "interface_state") {
            std::string target = e["target"];
            std::string newState = e["new_state"];

            for (auto &i : interfaces) {
                if (i.name == target) {
                    i.oper_state = newState;
                }
            }
        }

        buildRoutingTable();
        showRoutes();
    }
}
