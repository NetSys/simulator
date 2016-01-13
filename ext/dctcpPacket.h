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
};

class DctcpAck : public Ack {
    public:
        bool ecn;
        //uint32_t delayed_num;

        DctcpAck(
            Flow *flow, 
            uint32_t seq_no_acked, 
            std::vector<uint32_t> sack_list,
            uint32_t size,
            Host* src,
            Host* dst,
            bool ecn
        //    uint32_t delayed_num
        ) : Ack(flow, seq_no_acked, sack_list, size, src, dst) {
            this->ecn = ecn;
        //    this->delayed_num = delayed_num;
        }
};


#endif
