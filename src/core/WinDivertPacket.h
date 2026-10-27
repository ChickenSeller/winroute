#pragma once

#include <cstdint>

// Pure packet helpers for the WinDivert backend: parse a captured IPv4/IPv6 TCP
// packet and rewrite its source or destination endpoint, recomputing the IP and
// TCP checksums. No WinDivert.dll dependency, so these are unit-testable on
// their own (tests/tests.cpp). Buffers start at the IP header, as WinDivert
// delivers them; addresses are raw network-order bytes (4 for IPv4, 16 for
// IPv6) and ports are host order.
namespace wdpkt {

struct Endpoints {
    bool v6 = false;
    uint8_t src[16] = {};   // network order; first 4 bytes for IPv4
    uint8_t dst[16] = {};
    uint16_t srcPort = 0;   // host order
    uint16_t dstPort = 0;
};

// Reads the endpoints of a TCP-over-IPv4/IPv6 packet. Returns false if the
// buffer is too short or not TCP.
bool readTcp(const uint8_t *data, unsigned len, Endpoints *out);

// Rewrites the destination (addr + port) or source of a TCP packet in place and
// recomputes the checksums. addr is 4 bytes for IPv4, 16 for IPv6; it must match
// the packet's family. Returns false if the packet is not parseable TCP.
bool setTcpDst(uint8_t *data, unsigned len, const uint8_t *addr, uint16_t portHost);
bool setTcpSrc(uint8_t *data, unsigned len, const uint8_t *addr, uint16_t portHost);

// Recomputes the IPv4 header checksum (IPv4 only) and the TCP checksum.
bool recalcTcpChecksums(uint8_t *data, unsigned len);

} // namespace wdpkt
