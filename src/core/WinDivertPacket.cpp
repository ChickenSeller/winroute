#include "WinDivertPacket.h"

#include <cstring>

namespace wdpkt {
namespace {

constexpr uint8_t kProtoTcp = 6;

// Ones-complement 16-bit accumulation over bytes read big-endian (network order).
uint32_t addBytes(const uint8_t *p, int n, uint32_t acc)
{
    for (; n > 1; n -= 2, p += 2)
        acc += (uint32_t(p[0]) << 8) | p[1];
    if (n)
        acc += uint32_t(p[0]) << 8;
    return acc;
}

uint16_t fold(uint32_t acc)
{
    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);
    return uint16_t(~acc);
}

void store16(uint8_t *p, uint16_t v)
{
    p[0] = uint8_t(v >> 8);
    p[1] = uint8_t(v & 0xff);
}

// Locates the IP and TCP headers. Returns false unless the packet is a complete
// TCP-over-IPv4/IPv6 datagram with no IPv6 extension headers.
struct View {
    bool v6 = false;
    uint8_t *ip = nullptr;
    unsigned ipHdrLen = 0;
    uint8_t *tcp = nullptr;
    unsigned tcpLen = 0;   // TCP header + data, for the checksum pseudo-header
};

bool parse(uint8_t *data, unsigned len, View *v)
{
    if (len < 20)
        return false;
    const uint8_t version = data[0] >> 4;
    if (version == 4) {
        const unsigned ihl = (data[0] & 0x0f) * 4u;
        if (ihl < 20 || len < ihl + 20)
            return false;
        if (data[9] != kProtoTcp)
            return false;
        const unsigned total = (uint32_t(data[2]) << 8) | data[3];
        if (total < ihl || total > len)
            return false;
        v->v6 = false;
        v->ip = data;
        v->ipHdrLen = ihl;
        v->tcp = data + ihl;
        v->tcpLen = total - ihl;
        return v->tcpLen >= 20;
    }
    if (version == 6) {
        if (len < 40 + 20)
            return false;
        if (data[6] != kProtoTcp)   // next header; extension headers unsupported
            return false;
        const unsigned payload = (uint32_t(data[4]) << 8) | data[5];
        if (payload < 20 || 40u + payload > len)
            return false;
        v->v6 = true;
        v->ip = data;
        v->ipHdrLen = 40;
        v->tcp = data + 40;
        v->tcpLen = payload;
        return true;
    }
    return false;
}

void recalc(const View &v)
{
    // IPv4 header checksum.
    if (!v.v6) {
        v.ip[10] = v.ip[11] = 0;
        store16(v.ip + 10, fold(addBytes(v.ip, int(v.ipHdrLen), 0)));
    }
    // TCP checksum: pseudo-header + segment.
    v.tcp[16] = v.tcp[17] = 0;
    uint32_t acc = 0;
    if (!v.v6) {
        acc = addBytes(v.ip + 12, 8, acc);          // src + dst
        acc += kProtoTcp;
        acc += v.tcpLen;
    } else {
        acc = addBytes(v.ip + 8, 32, acc);          // src + dst
        acc += (v.tcpLen >> 16) & 0xffff;
        acc += v.tcpLen & 0xffff;
        acc += kProtoTcp;
    }
    acc = addBytes(v.tcp, int(v.tcpLen), acc);
    store16(v.tcp + 16, fold(acc));
}

} // namespace

bool readTcp(const uint8_t *data, unsigned len, Endpoints *out)
{
    View v;
    if (!parse(const_cast<uint8_t *>(data), len, &v))
        return false;
    out->v6 = v.v6;
    std::memset(out->src, 0, sizeof(out->src));
    std::memset(out->dst, 0, sizeof(out->dst));
    if (v.v6) {
        std::memcpy(out->src, v.ip + 8, 16);
        std::memcpy(out->dst, v.ip + 24, 16);
    } else {
        std::memcpy(out->src, v.ip + 12, 4);
        std::memcpy(out->dst, v.ip + 16, 4);
    }
    out->srcPort = (uint16_t(v.tcp[0]) << 8) | v.tcp[1];
    out->dstPort = (uint16_t(v.tcp[2]) << 8) | v.tcp[3];
    const uint8_t flags = v.tcp[13];   // CWR ECE URG ACK PSH RST SYN FIN
    out->syn = (flags & 0x02) != 0 && (flags & 0x10) == 0;
    return true;
}

bool setTcpDst(uint8_t *data, unsigned len, const uint8_t *addr, uint16_t portHost)
{
    View v;
    if (!parse(data, len, &v))
        return false;
    std::memcpy(v.ip + (v.v6 ? 24 : 16), addr, v.v6 ? 16 : 4);
    store16(v.tcp + 2, portHost);
    recalc(v);
    return true;
}

bool setTcpSrc(uint8_t *data, unsigned len, const uint8_t *addr, uint16_t portHost)
{
    View v;
    if (!parse(data, len, &v))
        return false;
    std::memcpy(v.ip + (v.v6 ? 8 : 12), addr, v.v6 ? 16 : 4);
    store16(v.tcp + 0, portHost);
    recalc(v);
    return true;
}

bool recalcTcpChecksums(uint8_t *data, unsigned len)
{
    View v;
    if (!parse(data, len, &v))
        return false;
    recalc(v);
    return true;
}

bool udpPayload(const uint8_t *data, unsigned len, const uint8_t **payload, unsigned *payloadLen)
{
    if (len < 20)
        return false;
    constexpr uint8_t kProtoUdp = 17;
    unsigned ipHdr;
    const uint8_t version = data[0] >> 4;
    if (version == 4) {
        ipHdr = (data[0] & 0x0f) * 4u;
        if (ipHdr < 20 || data[9] != kProtoUdp)
            return false;
    } else if (version == 6) {
        ipHdr = 40;
        if (len < 40 || data[6] != kProtoUdp)
            return false;
    } else {
        return false;
    }
    if (len < ipHdr + 8)
        return false;
    const uint8_t *udp = data + ipHdr;
    const unsigned udpLen = (unsigned(udp[4]) << 8) | udp[5];   // header + data
    if (udpLen < 8 || ipHdr + udpLen > len)
        return false;
    *payload = udp + 8;
    *payloadLen = udpLen - 8;
    return true;
}

} // namespace wdpkt
