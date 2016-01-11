// Akshay Narayan
// 11 January 2016

#include "dctcpQueue.h"
#include "dctcpPacket.h"

#include "../run/params.h"

extern double get_current_time();
extern void add_to_event_queue(Event *ev);
extern DCExpParams params;

DctcpQueue::DctcpQueue(uint32_t id, double rate, uint32_t limit_bytes, int location) : Queue(id, rate, limit_bytes, location) {}

/**
 * ECN marking. Otherwise just a droptail queue.
 * K_min > (C (pkts/s) * RTT (s)) / 7
 * at 10 Gbps recommend K = 65 packets, at 1 Gbps K = 20
 * if queue length < params.dctcp_mark_thresh, don't mark (ECN = 0).
 * if queue length > params.dctcp_mark_thresh, mark (ECN = 1).
 */
void DctcpQueue::enque(Packet *packet) {
    p_arrivals += 1;
    b_arrivals += packet->size;
    if (bytes_in_queue + packet->size <= limit_bytes) {
        packets.push_back(packet);
        bytes_in_queue += packet->size;

        if (packets.size() >= params.dctcp_mark_thresh) {
            ((DctcpPacket*) packet)->ecn = true;
        }
    } 
    else {
        pkt_drop++;
        drop(packet);
    }
}

