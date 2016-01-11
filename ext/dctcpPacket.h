#ifndef DCTCP_PACKET_H
#define DCTCP_PACKET_H

#include "../coresim/packet.h"

class DctcpPacket : public Packet {
    public:
        bool ecn;

        DctcpPacket(
            double sending_time, 
            Flow *flow, 
            uint32_t seq_no, 
            uint32_t pf_priority, 
            uint32_t size, 
            Host *src, 
            Host *dst,
            bool ecn
        ) : Packet(sending_time, flow, seq_no, pf_priority, size, src, dst) {
            this->ecn = ecn;
        }
}
