#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <string>

inline uint32_t ipToInt(const std::string &ip) {
    struct in_addr addr;
    inet_pton(AF_INET, ip.c_str(), &addr);
    return ntohl(addr.s_addr);
}

inline std::pair<uint32_t, int> parseCIDR(const std::string &cidr) {
    auto pos = cidr.find('/');
    std::string ip = cidr.substr(0, pos);
    int prefix = std::stoi(cidr.substr(pos + 1));
    return {ipToInt(ip), prefix};
}

inline uint32_t maskFromPrefix(int prefix) {
    if (prefix == 0) return 0;
    return 0xFFFFFFFF << (32 - prefix);
}

inline bool ipInSubnet(uint32_t ip, uint32_t net, int prefix) {
    uint32_t mask = maskFromPrefix(prefix);
    return (ip & mask) == (net & mask);
}
